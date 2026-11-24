// SPDX-License-Identifier: MIT
#include "ubc/core/crafting_handoff.hpp"
#include "ubc/core/crafting_session.hpp"
#include "ubc/core/crafting_result.hpp"
#include "ubc/stations/alchemy/dndr_alchemy.hpp"

namespace d = ubc::dndr;
using namespace ubc;
int main() {
  const auto& r = d::recipes.front();
  const auto* effect = r.effect->information;
  if (d::callback(20341) || d::callback(21010) || d::callback(10060) ||
      d::callback(effect->symbol) != &r)
    return 1;
  if (d::maximum(r, {6, 3, 2, 0}) != 3 || d::maximum(r, {3, 3, 2, 0}) != 1 ||
      d::maximum(r, {4, 2, 0, 0}) != 1 || d::maximum(r, {1, 2, 2, 0}) != 0 ||
      d::maximum(r, {6, 3, 2, INT32_MAX - 2}) != 2 || d::maximum(r, {-1, 3, 2, 0}))
    return 2;
  const CraftingPage root{CraftingPageKind::dndr_alchemy, 0};
  for (auto n : {1u, 2u, 3u}) {
    const auto plan = make_crafting_plan(root, r.id, n);
    if (!plan.valid() || plan.count != 2 || plan.steps[0].function_symbol != 10060 ||
        plan.steps[1].function_symbol != 10066 || !plan.steps[1].is_effect() ||
        plan.steps[1].invocation != recipe_catalog::Invocation::info)
      return 3;
  }
  if (make_crafting_plan(crafting_page(recipe_navigation::Page::mana_info), r.id, 1).valid() ||
      make_crafting_plan(root, 4, 1).valid() || make_crafting_plan(root, r.id, 0).valid())
    return 4;
  d::ManaBody body;
  for (std::size_t i = 0; i < body.original.size(); ++i)
    body.original[i] = static_cast<std::uint8_t>(i);
  body.prepared = body.original;
  if (!d::prepare_mana(body, 1) || body.original != body.prepared || body.prefix)
    return 5;
  if (d::prepare_mana(body, 0) || d::prepare_mana(body, INT32_MAX) || !d::prepare_mana(body, 3))
    return 6;
  const auto word = [&](unsigned at) {
    return runtime_script::detail::u32(body.prepared.data() + at);
  };
  if (word(1) != 3 || word(22) != 6 || word(59) != 6 || word(79) != 3 || word(124) != 3 ||
      word(94) != d::flask || word(99) != 2 || body.prepared[108] != 76 ||
      word(109) != effect->byte_offset + 133 || word(129) != effect->byte_offset + 148 ||
      word(44) != effect->byte_offset + 133 ||
      !std::equal(body.original.begin() + 133, body.original.end(), body.prepared.begin() + 133))
    return 7;
  CraftingSnapshot shot;
  shot.key = {1, 2, 3, 4, 5, 6, crafting_catalog::Station::alchemy};
  auto& row = shot.recipes.front();
  row = make_crafting_state(r.id);
  row.generation = 1;
  row.evaluated = row.knowledge_read_valid = true;
  CraftingSession unlearned;
  if (!unlearned.open(shot) || unlearned.visible_count())
    return 8;
  row.known = row.hand_known = row.material_known = row.contract = row.material_verified = true;
  row.actor_known = row.actor_matches = row.context_known = row.context_matches = true;
  shot.hands.left_slot_known = shot.hands.right_slot_known = shot.hands.interact_item_known = true;
  shot.hands.hands.left = {0, -1, -1, 0};
  shot.hands.hands.right = {1, static_cast<int>(d::flask), 1, 77};
  shot.hands.interact_item = 77;
  for (auto& m : row.materials) {
    m.stored = 0;
    m.consumed_reserve = m.symbol == static_cast<int>(d::flask);
  }
  CraftingSession missing;
  if (!missing.open(shot) || missing.visible_count() != 1 || missing.can_confirm() ||
      crafting_block_reason(row) != CraftingBlockReason::missing_material)
    return 9;
  row.materials[0].stored = 4;
  row.materials[1].stored = 2;
  row.materials[2].stored = 1;
  row.maximum = 2;
  CraftingSession quantity;
  if (!quantity.open(shot) || quantity.quantity() != 1 || quantity.maximum() != 2 ||
      !quantity.can_confirm())
    return 12;
  std::uint64_t frame = 0;
  const auto release = [&]() {
    for (unsigned i = 0; i < 4; ++i)
      quantity.observe(++frame, false, true);
  };
  release();
  if (!quantity.adjust(1) || quantity.quantity() != 2 || !quantity.can_confirm() ||
      !quantity.adjust(-1) || quantity.quantity() != 1 || !quantity.preset(quantity.maximum()) ||
      quantity.quantity() != 2)
    return 13;
  release();
  if (!quantity.confirm())
    return 14;
  release();
  const auto selected = quantity.take(shot, true);
  if (!selected || selected->id != r.id || selected->quantity != 2)
    return 15;
  CraftingRequest request{r.id, 2, shot.key, row, shot.hands};
  CraftingResult result;
  if (!crafting_handoff_detail::valid_request(request) ||
      !result.begin(request, {1, 1, 1, 0, 3, 4, 5}) || !result.restored(true))
    return 10;
  for (std::size_t i = 0; i < 4; ++i) {
    const auto preview = crafting_amount_preview(row.materials[i], 2);
    if (!preview.end_known || preview.end_storage != (i == 3 ? 2 : 0))
      return 11;
  }
  return 0;
}
