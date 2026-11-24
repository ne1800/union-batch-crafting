// SPDX-License-Identifier: MIT
#include "runtime.hpp"
#include "ubc/stations/alchemy/dndr_alchemy.hpp"

namespace ubc_native {
static void capture_recipe(Capture& captured, const Context& c, std::size_t index) noexcept {
  namespace d = ubc::dndr;
  const auto& recipe = d::recipes[index];
  auto& row = captured.snapshot.recipes[index];
  row = ubc::make_crafting_state(recipe.id);
  row.generation = generation;
  row.evaluated = true;
  auto talent = [&]() {
    return rt::symbol_int_element(memory, s::game_parser, recipe.talent, recipe.element,
                                  d::profile);
  };
  const auto learned = talent();
  if (!learned.valid || (learned.value != 0 && learned.value != 1))
    return;
  row.knowledge_read_valid = true;
  row.known = learned.value == 1;
  if (!row.known)
    return;
  static ubc::PackedInventoryWorkspace inventory;
  static ubc::station_recipe_snapshot_detail::HandWorkspace hands;
  const auto& descriptor = cat::dndr_recipes[index];
  if (ubc::station_recipe_snapshot_detail::begin_hands(memory, c.hero, hands) !=
          ubc::StationHandReadIssue::none ||
      !ubc::crafting_detail::bound_hands(descriptor, hands.before))
    return;
  const auto inv = ubc::read_packed_inventory<4>(memory, c.hero, recipe.targets.data,
                                                 recipe.targets.count, true, inventory);
  if (!inv.storage_totals_known || !inv.snapshot_unchanged || !inv.stable_boundary_verified ||
      inv.issue != ubc::PackedInventoryIssue::none)
    return;
  for (std::size_t i = 0; i < 4; ++i)
    if (inv.has_active_marker[i] || inv.duplicate_packed_target[i] || inv.mixed_representation[i] ||
        inv.listed_occurrences[i] > 1 || inv.packed_occurrences[i] > 1)
      return;
  for (std::size_t i = 0; i < inv.listed_nodes; ++i) {
    const auto object = rt::detail::u32(inventory.records[i].node.data() + 4);
    if (object == hands.before.interact_item)
      return;
    const auto symbol =
        ubc::station_detail::little_i32(inventory.records[i].item_fields.data() + 4);
    if (!cat::units(recipe.costs, symbol))
      continue;
    std::uint32_t type = 0, flags = 0;
    if (!field(object, 0, type) || type != ubc::hand_detail::item_vtable ||
        !field(object, 0x158, flags) || !(flags & 0x00200000u) || (flags & 0x40000000u))
      return;
    for (std::size_t slot = 0; slot < hands.count; ++slot)
      if (rt::detail::u32(hands.records[slot].fields.data() + 0x30) == object)
        return;
  }
  const auto final_talent = talent();
  if (!ubc::station_recipe_snapshot_detail::verify_hands(memory, hands) || !final_talent.valid ||
      final_talent.value != learned.value || !same_context(c, context()))
    return;
  captured.snapshot.hands = hands.before;
  row.hand_known = row.material_known = row.material_verified = row.contract = true;
  row.actor_known = row.actor_matches = row.context_known = row.context_matches = true;
  for (std::size_t i = 0; i < 4; ++i) {
    row.materials[i].stored = inv.total[i];
    row.materials[i].consumed_reserve = row.materials[i].symbol == static_cast<int>(d::flask);
  }
  row.maximum = d::maximum(recipe, inv.total);
  // The ingredient checks enter the original HasItems path. The extra
  // flask debit has no prior HasItems call to unpack a stored flask.
  for (std::size_t i = 0; i < recipe.targets.count; ++i)
    if (recipe.targets[i].symbol == static_cast<int>(d::flask) && inv.packed[i])
      row.maximum = (std::min)(row.maximum, 1u);
}

void capture_dndr_alchemy(Capture& captured, const Context& c, bool boundary) noexcept {
  if (!c.valid || !boundary)
    return;
  captured.snapshot.key = key(c);
  static_assert(ubc::dndr::recipes.size() <= cat::recipe_count);
  for (std::size_t i = 0; i < ubc::dndr::recipes.size(); ++i)
    capture_recipe(captured, c, i);
}

void* run_dndr_amount(bc::WriterScope& scope, const ubc::AmountMemory& writable,
                      std::uint32_t address, const ubc::dndr::AlchemyRecipe& recipe,
                      ubc::AmountOutcome& outcome) {
  namespace d = ubc::dndr;
  const auto n = handoff.request().quantity;
  const auto& f = *recipe.effect->information;
  d::ManaBody body;
  void* result = nullptr;
  const auto error = GetLastError();
  DWORD saved = error;
  __try {
    if (recipe.id == d::recipes[0].id && bc::writer_undisturbed(scope) &&
        body.capture(writable, address, f) && d::prepare_mana(body, n) &&
        (n == 1 ? body.matches(body.original) : body.install()) && bc::writer_undisturbed(scope)) {
      SetLastError(error);
      result = execute_original_amount(&scope);
      saved = GetLastError();
      outcome.entered = effect_executed;
    } else
      outcome.issue = ubc::AmountIssue::contract;
  } __finally {
    if (AbnormalTermination())
      saved = GetLastError();
    scope.restore_verified = body.restore();
    if (effect_executed &&
        (!scope.restore_verified || !bc::writer_undisturbed(scope) || AbnormalTermination()))
      result_guard.fail(ubc::CraftingResultIssue::contract);
    outcome.restored = scope.restore_verified;
    SetLastError(saved);
  }
  return result;
}
} // namespace ubc_native
