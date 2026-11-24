// SPDX-License-Identifier: MIT
#include "ubc/core/crafting_handoff.hpp"
#include "ubc/stations/anvil/dndr_anvil.hpp"
#include <map>
#include <vector>

using namespace ubc;
namespace d = dndr;
namespace a = d::anvil;
struct Weapon {
  int effect, element, product, ore, blood;
};
constexpr Weapon weapons[]{
    {10268, 0, 6805, 0, 0},  {10271, 9, 6811, 0, 0},  {10274, 10, 6817, 0, 0},
    {10277, 11, 6821, 0, 0}, {10280, 12, 6832, 0, 0}, {10283, 1, 6818, 1, 0},
    {10286, 2, 6819, 2, 0},  {10289, 3, 6830, 2, 0},  {10292, 4, 6831, 3, 0},
    {10295, 5, 6842, 3, 0},  {10298, 6, 6843, 4, 0},  {10301, 7, 6846, 4, 5},
    {10304, 8, 6847, 5, 5}};
using Stocks = std::map<int, int>;

// Interpret the prepared window. At each original END entry check its exact stack
// contract, then model its output and termination. The hand blank is already reserved.
static bool simulate(const d::AlchemyBody& body, const d::AlchemyBody& aux, unsigned entry,
                     Stocks& stock) {
  std::vector<int> stack;
  const auto refund = a::owners[0].information->byte_offset;
  auto* window = &body;
  for (std::size_t pos = 0; pos < window->prefix;) {
    const auto op = window->prepared[pos++];
    if (op == 18) {
      if (stack.size() < 2)
        return false;
      const int held = stack.back();
      stack.pop_back();
      stack.back() = held >= stack.back();
      continue;
    }
    if (pos + 4 > window->prefix)
      return false;
    const auto arg = runtime_script::detail::u32(window->prepared.data() + pos);
    pos += 4;
    if (op == 75 && arg == a::spare.byte_offset + a::helper_begin && window == &body) {
      window = &aux;
      pos = a::helper_begin;
      continue;
    }
    if (op == 75 && arg == entry + 5 && window == &aux) {
      window = &body;
      pos = 5;
      continue;
    }
    if (op == 64 || op == 67) {
      stack.push_back(static_cast<int>(arg));
    } else if (op == 76) {
      if (stack.empty() || arg != refund)
        return false;
      const auto ok = stack.back();
      stack.pop_back();
      if (!ok) {
        ++stock[a::cold];
        return true;
      }
    } else if ((op == 62 && arg == 184) || (op == 75 && arg == refund + 15)) {
      if (stack.size() < 3)
        return false;
      auto count = stack.back();
      stack.pop_back();
      const auto item = stack.back();
      stack.pop_back();
      if (stack.back() != static_cast<int>(d::hero) || count <= 0)
        return false;
      stack.pop_back();
      if (op == 75) {
        stock[item] += count;
        return pos == body.prefix;
      }
      const int removed = (std::min)(count, stock[item]);
      stock[item] -= removed;
      stack.push_back(removed > 0);
    } else if (op == 62 && arg == 160) {
      if (stack.size() < 2)
        return false;
      const auto item = stack.back();
      stack.pop_back();
      if (stack.back() != static_cast<int>(d::hero))
        return false;
      stack.back() = stock[item];
    } else
      return false;
  }
  return false;
}

static bool verify(const Weapon& w) {
  const auto* r = d::callback(static_cast<unsigned>(w.effect));
  if (!r || !r->at_anvil() || r->talent != 1436 || r->element != static_cast<unsigned>(w.element) ||
      r->outputs[0].symbol != w.product || r->category_flag != (w.ore ? 10249u : 10250u))
    return false;
  const auto* descriptor = crafting_catalog::find(r->id);
  if (!descriptor || descriptor->station != crafting_catalog::Station::anvil)
    return false;
  const auto key = CraftingPageKind::dndr_anvil;
  for (const auto start : {0u, 10249u, 10250u}) {
    const auto plan = make_crafting_plan({key, start}, r->id, 2);
    if (!plan.valid() || plan.effect()->function_symbol != w.effect ||
        plan.count != (start == r->category_flag ? 1u
                       : start                   ? 3u
                                                 : 2u))
      return false;
  }
  if (make_crafting_plan({CraftingPageKind::dndr_alchemy, 0}, r->id, 1).valid())
    return false;
  d::AlchemyBody body;
  body.length = r->effect->information->byte_length;
  if (d::prepare_anvil_batch(body, *r, 0) || !d::prepare_anvil_batch(body, *r, 1) || body.prefix ||
      body.prepared != body.original)
    return false;
  for (const int n : {2, 7}) {
    d::AlchemyBody aux;
    aux.length = a::spare.byte_length;
    if (!d::prepare_anvil_batch(body, *r, static_cast<unsigned>(n), &aux))
      return false;
    const Stocks initial{{a::hot, n - 1},     {a::cold, 8},   {6928, n * w.ore},
                         {6572, n * w.blood}, {a::hammer, 1}, {w.product, 4}};
    for (const int shortage : {0, a::hot, 6928, 6572}) {
      if (shortage && !initial.at(shortage))
        continue;
      auto stock = initial;
      if (shortage)
        --stock[shortage];
      auto expected = stock;
      if (shortage)
        ++expected[a::cold];
      else {
        expected[a::hot] -= n - 1;
        expected[6928] -= n * w.ore;
        expected[6572] -= n * w.blood;
        expected[w.product] += n;
      }
      std::array<std::int32_t, crafting_catalog::target_capacity> values{};
      for (std::size_t j = 0; j < r->targets.count; ++j)
        values[j] = stock.at(r->targets[j].symbol);
      if (d::maximum(*r, values.data()) != static_cast<unsigned>(n - bool(shortage)) ||
          !simulate(body, aux, r->effect->information->byte_offset, stock) || stock != expected)
        return false;
    }
  }
  StationRecipeHands hands{};
  hands.left_slot_known = hands.right_slot_known = hands.interact_item_known = true;
  hands.hands.left = {1, a::hot, 1, 11};
  hands.hands.right = {1, a::hammer, 1, 12};
  hands.left_tmp_level = 0;
  hands.right_tmp_level = 1;
  hands.interact_item = 11;
  if (!crafting_detail::bound_hands(*descriptor, hands))
    return false;
  hands.interact_item = 12;
  return !crafting_detail::bound_hands(*descriptor, hands);
}
int main() {
  if (!runtime_script::detail::valid_profile(a::profile))
    return 1;
  for (const auto& owner : a::owners) {
    const int number = owner.instance == 10251
                           ? 999
                           : (owner.instance == 10260 || owner.instance == 10263 ? 99 : -1);
    if (owner.number != number)
      return 16;
  }
  for (const auto& w : weapons)
    if (!verify(w))
      return w.element + 2;
  return 0;
}
