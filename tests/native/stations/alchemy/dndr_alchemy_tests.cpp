// SPDX-License-Identifier: MIT
#include "ubc/core/crafting_handoff.hpp"
#include "ubc/core/crafting_session.hpp"
#include "ubc/core/crafting_result.hpp"
#include "ubc/text/locales/de.hpp"
#include "ubc/stations/alchemy/dndr_alchemy.hpp"

namespace d = ubc::dndr;
using namespace ubc;

struct Expected {
  unsigned effect, category, talent, element;
  std::array<recipe_catalog::ItemAmount, 4> ingredients;
  int first, second;
};
static constexpr std::array<Expected, 20> expected{{
    {10085, 10076, 1421, 0, {{{7007, 2}, {7029, 1}}}, 7065, 0},
    {10088, 10076, 1421, 1, {{{7009, 2}, {7029, 1}}}, 7067, 0},
    {10091, 10076, 1421, 2, {{{7011, 2}, {7029, 1}}}, 7069, 0},
    {10066, 10057, 1421, 3, {{{7001, 2}, {7029, 1}}}, 7059, 0},
    {10069, 10057, 1421, 4, {{{7003, 2}, {7029, 1}}}, 7061, 0},
    {10072, 10057, 1421, 5, {{{7005, 2}, {7029, 1}}}, 7063, 0},
    {10047, 10038, 3199, 0, {{{6999, 1}, {6997, 2}, {6559, 1}, {6117, 1}}}, 6122, 0},
    {10050, 10038, 3200, 0, {{{6999, 1}, {6997, 2}, {6559, 1}, {6117, 2}}}, 6124, 0},
    {10053, 10038, 3201, 0, {{{7017, 1}, {6117, 1}, {6717, 1}}}, 6126, 0},
    {10075, 10057, 1421, 13, {{{7059, 3}, {7029, 1}}}, 6183, 0},
    {10094, 10076, 1421, 14, {{{7065, 3}, {7029, 1}}}, 6181, 0},
    {10107, 10095, 1421, 10, {{{7011, 1}, {7031, 1}}}, 7075, 0},
    {10110, 10095, 1421, 9, {{{7005, 1}, {7031, 1}}}, 7077, 0},
    {10113, 10095, 1421, 8, {{{7013, 1}, {7031, 1}}}, 7073, 0},
    {10116, 10095, 1421, 7, {{{7015, 1}, {7031, 1}}}, 7071, 0},
    {10119, 10095, 1421, 6, {{{7017, 1}, {7029, 1}}}, 7079, 0},
    {10129, 10120, 0, 0, {{{6999, 1}}}, 6922, 0},
    {10132, 10120, 3085, 0, {{{6999, 2}, {7029, 1}}}, 6149, 0},
    {10142, 0, 1421, 11, {{{7676, 10}, {6955, 1}, {6948, 1}}}, 7081, 0},
    {10104, 10095, 3072, 0, {{{6546, 2}, {7061, 1}, {7065, 1}, {6129, 1}}}, 6177, 6179},
}};
// Synthetic instruction windows exercise every shaped write without embedding game script bodies.
static bool batch_window(const d::AlchemyRecipe& r) {
  d::AlchemyBody body;
  const auto& f = *r.effect->information;
  body.length = f.byte_length;
  body.original.fill(0xA5);
  auto put = [&](std::size_t at, unsigned value) {
    for (unsigned i = 0; i < 4; ++i)
      body.original[at + i] = static_cast<std::uint8_t>(value >> (8 * i));
  };
  const auto debit = r.guard_jump + 5;
  body.original[r.guard_jump] = 76;
  for (std::size_t i = 0; i + 1 < r.costs.count; ++i) {
    const auto& cost = r.costs[i];
    put(21 * i + 1, cost.units);
    put(21 * i + 11, static_cast<unsigned>(cost.symbol));
    put(debit + 20 * i + 6, static_cast<unsigned>(cost.symbol));
    put(debit + 20 * i + 11, cost.units);
  }
  body.prepared = body.original;
  if (d::prepare_batch(body, r, 0) || !d::prepare_batch(body, r, 1) ||
      body.prepared != body.original || body.prefix || !d::prepare_batch(body, r, 3))
    return false;
  auto word = [&](std::size_t at) {
    return runtime_script::detail::u32(body.prepared.data() + at);
  };
  const auto out = debit + 25 + (r.costs.count - 1) * 20;
  const auto failure = out + (r.branched() ? 61 : 20) + 6;
  if (body.prefix != failure + 6 || body.prefix > body.length ||
      word(r.guard_jump + 1) != f.byte_offset + failure || word(debit + 6) != d::flask ||
      word(debit + 11) != 2 || word(debit + 21) != f.byte_offset + failure ||
      body.prepared[failure] != 61 || word(failure + 1) != d::owners[0].information->byte_offset ||
      body.prepared[failure + 5] != 60)
    return false;
  for (std::size_t i = 0; i + 1 < r.costs.count; ++i)
    if (word(21 * i + 1) != 3 * r.costs[i].units ||
        word(debit + 25 + 20 * i + 11) != 3 * r.costs[i].units)
      return false;
  if (r.branched())
    return word(out + 12) == f.byte_offset + out + 41 &&
           word(out + 37) == f.byte_offset + out + 61 && word(out + 27) == 3 && word(out + 52) == 3;
  return body.prepared[out + 10] == 64 && word(out + 11) == 3 && body.prepared[out + 15] == 62 &&
         word(out + 16) == 170;
}
static bool profiles() {
  const CraftingPage root{CraftingPageKind::dndr_alchemy, 0};
  if (!runtime_script::detail::valid_profile(d::profile) || expected.size() != d::recipes.size())
    return false;
  constexpr int numbers[]{999, 10, 99, 5,  6, 5, 6, 99, 2, 3, 4,  4,  7,  99, 2, 3,
                          4,   4,  9,  99, 2, 3, 3, 3,  3, 3, 11, 99, 10, 11, 2};
  static_assert(std::size(numbers) == d::owners.size());
  for (std::size_t i = 0; i < d::owners.size(); ++i)
    if (d::owners[i].npc != 11471 || d::owners[i].number != numbers[i])
      return false;
  d::AlchemyBody invalid;
  if (invalid.capture({}, 42, *d::recipes.front().effect->information, d::body_capacity + 1) ||
      invalid.matches(invalid.original) || invalid.ready())
    return false;
  for (const auto& want : expected) {
    const auto* r = d::callback(want.effect);
    if (!r || !batch_window(*r) || r->talent != want.talent || r->element != want.element ||
        r->category_flag != want.category || r->products[0] != want.first ||
        r->products[1] != want.second ||
        ui_text::locale_de::table.recipe(r->id) ==
            ui_text::locale_de::table[ui_text::Label::select_recipe])
      return false;
    std::array<std::int32_t, crafting_catalog::target_capacity> stock{};
    for (std::size_t i = 0; i < want.ingredients.size(); ++i) {
      const auto cost = want.ingredients[i];
      if (r->ingredients[i].symbol != cost.symbol || r->ingredients[i].units != cost.units)
        return false;
      if (cost.units)
        stock[i] = static_cast<int>(cost.units * 3);
    }
    stock[r->costs.count - 1] = 2; // One more flask in the station hand.
    for (int b : {0, 1, 2}) {
      const bool branched = want.second != 0;
      const auto branch = branched ? b : 0;
      const auto outputs = r->selected_outputs(branched, branch);
      if (outputs.count != 1 ||
          outputs[0].symbol != (branched && b == 1 ? want.second : want.first) ||
          d::maximum(*r, stock.data(), branched, branch) != 3)
        return false;
      for (const auto& target : r->targets)
        if (ui_text::locale_de::table.item(target.symbol) ==
            ui_text::locale_de::table[ui_text::Label::unknown_item])
          return false;
      for (std::size_t i = 0; i < r->costs.count; ++i) {
        auto missing = stock;
        missing[i] = 0;
        if (d::maximum(*r, missing.data(), branched, branch) != (i + 1 == r->costs.count ? 1u : 0u))
          return false;
      }
      for (unsigned n : {1u, 2u, 3u})
        for (const auto& c : d::categories) {
          const CraftingPage from{CraftingPageKind::dndr_alchemy, c.flag};
          const auto plan = make_crafting_plan(from, r->id, n, branched, branch);
          if (!plan.valid() || plan.effect()->function_symbol != static_cast<int>(want.effect) ||
              plan.count != (want.category == c.flag ? 1u
                             : want.category         ? 3u
                                                     : 2u))
            return false;
        }
      const auto plan = make_crafting_plan(root, r->id, 2, branched, branch);
      if (!plan.valid() || plan.count != (want.category ? 2u : 1u))
        return false;
    }
    if (want.second && (r->selected_outputs(false, 0).count || d::maximum(*r, stock.data()) ||
                        make_crafting_plan(root, r->id, 2).valid()))
      return false;
  }
  for (unsigned excluded : {10056u, 10135u, 10136u, 10137u, 10138u, 10139u, 20341u, 21010u})
    if (d::callback(excluded))
      return false;
  return true;
}
int main() {
  if (!profiles())
    return 20;
  const auto& r = *d::callback(10066);
  const auto* effect = r.effect->information;
  if (d::callback(20341) || d::callback(21010) || d::callback(10060) ||
      d::callback(effect->symbol) != &r)
    return 1;
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
