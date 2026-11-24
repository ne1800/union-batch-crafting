// SPDX-License-Identifier: MIT
#include "runtime.hpp"
#include "ubc/stations/alchemy/dndr_alchemy.hpp"
#include "ubc/stations/anvil/dndr_anvil.hpp"

namespace ubc_native {
namespace d = ubc::dndr;
static constexpr auto inventory_targets = [] {
  std::array<ubc::PackedInventoryTarget, d::items.size()> targets{};
  for (std::size_t i = 0; i < targets.size(); ++i)
    targets[i] = d::items[i].target;
  return targets;
}();
static rt::SymbolInt knowledge(const d::Recipe& r) noexcept {
  if (!r.talent)
    return {true, 1, rt::Issue::none}; // The ordinary joint has no knowledge condition.
  return rt::symbol_int_element(memory, s::game_parser, r.talent, r.element, d::layout_profile);
}

void capture_dndr(Capture& captured, const Context& c, bool boundary) noexcept {
  if (!c.valid || !boundary)
    return;
  const bool at_anvil = c.kind == cat::Station::anvil;
  captured.snapshot.key = key(c);
  static_assert(d::recipes.size() <= cat::recipe_count);
  std::array<rt::SymbolInt, d::recipes.size()> learned{};
  for (std::size_t i = 0; i < d::recipes.size(); ++i) {
    const auto& r = d::recipes[i];
    if (r.at_anvil() != at_anvil)
      continue;
    auto& row = captured.snapshot.recipes[i];
    row = ubc::make_crafting_state(r.id);
    row.generation = generation;
    row.evaluated = true;
    learned[i] = knowledge(r);
    row.knowledge_read_valid = learned[i].valid;
    row.known = learned[i].valid && learned[i].value == 1;
  }
  const auto branch = rt::symbol_int(memory, s::game_parser, d::bloodfly, d::layout_profile);
  static ubc::PackedInventoryWorkspace inventory;
  static ubc::station_recipe_snapshot_detail::HandWorkspace hands;
  if (ubc::station_recipe_snapshot_detail::begin_hands(memory, c.hero, hands) !=
          ubc::StationHandReadIssue::none ||
      !ubc::crafting_detail::bound_hands(
          cat::dndr_recipes[at_anvil ? d::alchemy_formulas.size() : 0], hands.before))
    return;
  const auto inv = ubc::read_packed_inventory<inventory_targets.size()>(
      memory, c.hero, inventory_targets.data(), inventory_targets.size(), true, inventory);
  if (inv.issue != ubc::PackedInventoryIssue::none || !inv.storage_totals_known ||
      !inv.snapshot_unchanged || !inv.stable_boundary_verified)
    return;
  std::array<bool, d::items.size()> unsafe_debit{};
  for (std::size_t i = 0; i < inv.listed_nodes; ++i) {
    const auto object = rt::detail::u32(inventory.records[i].node.data() + 4);
    if (object == hands.before.interact_item ||
        (at_anvil && object == hands.before.hands.right.object))
      return;
    const auto symbol =
        ubc::station_detail::little_i32(inventory.records[i].item_fields.data() + 4);
    const auto index = d::item_index(symbol);
    if (index == d::items.size())
      continue;
    std::uint32_t type = 0, flags = 0;
    unsafe_debit[index] = !field(object, 0, type) || type != ubc::hand_detail::item_vtable ||
                          !field(object, 0x158, flags) || !(flags & 0x00200000u) ||
                          (flags & 0x40000000u);
    for (std::size_t slot = 0; slot < hands.count; ++slot)
      if (rt::detail::u32(hands.records[slot].fields.data() + 0x30) == object)
        unsafe_debit[index] = true;
  }
  if (!ubc::station_recipe_snapshot_detail::verify_hands(memory, hands) ||
      !same_context(c, context()))
    return;
  const auto final_branch = rt::symbol_int(memory, s::game_parser, d::bloodfly, d::layout_profile);
  captured.snapshot.hands = hands.before;
  for (std::size_t i = 0; i < d::recipes.size(); ++i) {
    auto& row = captured.snapshot.recipes[i];
    const auto& r = d::recipes[i];
    if (r.at_anvil() != at_anvil)
      continue;
    const auto fresh = knowledge(r);
    if (!fresh.valid || !learned[i].valid || fresh.value != learned[i].value) {
      row.knowledge_read_valid = row.known = false;
      continue;
    }
    if (!row.known)
      continue;
    if (r.branched()) {
      if (!branch.valid || !final_branch.valid || branch.value != final_branch.value)
        continue;
      row.branch_value = branch.value;
      row.branch_known = true;
      row.branch_output = r.selected_outputs(true, branch.value)[0].symbol;
      for (std::size_t j = 0; j < row.target_count; ++j)
        row.materials[j].unit_output = row.materials[j].symbol == row.branch_output;
    }
    std::array<std::int32_t, cat::target_capacity> stock{};
    bool eligible = true;
    for (std::size_t j = 0; j < row.target_count; ++j) {
      const auto at = d::item_index(row.materials[j].symbol);
      stock[j] = inv.total[at];
      if (inv.has_active_marker[at] || inv.duplicate_packed_target[at] ||
          inv.mixed_representation[at] ||
          (inv.listed_occurrences[at] > 1 && !(at_anvil && row.materials[j].unit_output)) ||
          inv.packed_occurrences[at] > 1 || (row.materials[j].unit_cost && unsafe_debit[at]))
        eligible = false;
    }
    if (!eligible)
      continue;
    for (std::size_t j = 0; j < row.target_count; ++j) {
      row.materials[j].stored = stock[j];
      row.materials[j].consumed_reserve = row.materials[j].symbol == r.reserve;
      row.materials[j].generated_prop = at_anvil && row.materials[j].symbol == d::anvil::hammer;
    }
    row.hand_known = row.material_known = row.material_verified = row.contract = true;
    row.actor_known = row.actor_matches = row.context_known = row.context_matches = true;
    row.maximum = d::maximum(r, stock.data(), row.branch_known, row.branch_value);
    // The extra debit can use only a verified, listed reserve stack.
    if (at_anvil) {
      unsigned unpacked = 0;
      for (const auto& cost : r.costs)
        if (cost.symbol != r.reserve && inv.packed[d::item_index(cost.symbol)])
          ++unpacked;
      const auto used = inv.listed_nodes + unpacked;
      row.maximum = (std::min)(row.maximum,
                               used >= ubc::max_packed_inventory_nodes
                                   ? 0u
                                   : static_cast<unsigned>(ubc::max_packed_inventory_nodes - used));
    }
    if (inv.packed[d::item_index(r.reserve)])
      row.maximum = (std::min)(row.maximum, 1u);
  }
}

void* run_dndr_amount(bc::WriterScope& scope, const ubc::AmountMemory& writable,
                      const NativeAmountContext& context, const ubc::dndr::Recipe& recipe,
                      ubc::AmountOutcome& outcome) {
  const auto n = handoff.request().quantity;
  const auto& f = *recipe.effect->information;
  const auto address = context.layout.code + f.byte_offset;
  const bool needs_reserve = recipe.at_anvil() && recipe.costs.count == 1 && n > 1;
  NativeAmountContext reserve_context{context.layout, &d::anvil::spare, false};
  auto reserve_memory = writable;
  reserve_memory.context = &reserve_context;
  d::AlchemyBody body, reserve_body;
  void* result = nullptr;
  const auto error = GetLastError();
  DWORD saved = error;
  __try {
    if (bc::writer_undisturbed(scope) && body.capture(writable, address, f, f.byte_length) &&
        (!needs_reserve ||
         reserve_body.capture(reserve_memory, context.layout.code + d::anvil::spare.byte_offset,
                              d::anvil::spare, d::anvil::spare.byte_length)) &&
        (recipe.at_anvil()
             ? d::prepare_anvil_batch(body, recipe, n, needs_reserve ? &reserve_body : nullptr)
             : d::prepare_batch(body, recipe, n)) &&
        (!needs_reserve || reserve_body.install()) &&
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
    const bool restored = body.restore();
    scope.restore_verified = reserve_body.restore() && restored;
    if (!scope.restore_verified ||
        (effect_executed && (!bc::writer_undisturbed(scope) || AbnormalTermination())))
      result_guard.fail(ubc::CraftingResultIssue::contract);
    outcome.restored = scope.restore_verified;
    SetLastError(saved);
  }
  return result;
}
} // namespace ubc_native
