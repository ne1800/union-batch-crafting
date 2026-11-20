// SPDX-License-Identifier: MIT
#include "ubc/catalogs/ore_recipe_catalog.hpp"
#include <cstdio>
#include <vector>

namespace c = ubc::ore_recipe_catalog;
namespace {
unsigned checks = 0, failures = 0;
void check(bool value, const char* why) {
  ++checks;
  if (!value) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", why);
  }
}
}
int main() {
  constexpr std::array<std::uint32_t, 8> outputs{
      {16492, 16494, 16508, 16510, 16522, 16524, 16528, 16530}};
  constexpr std::array<std::uint32_t, 8> ore{{1, 2, 2, 3, 3, 4, 4, 5}};
  check(!c::runtime_ready, "data profiles grant no native execution");
  for (std::size_t i = 0; i < c::recipes.size(); ++i) {
    const auto& r = c::recipes[i];
    const auto callback = 20745u + static_cast<unsigned>(i) * 3u;
    check(c::structurally_valid(r) && c::find(r.id) == &r, "canonical independent recipe resolves");
    check(r.callback.invocation == c::Invocation::info &&
              r.callback.function == r.owner.information &&
              r.callback.function->symbol == callback &&
              r.owner.instance == static_cast<int>(callback) - 2,
          "original direct info is the sole effect");
    check(r.costs.count == (i < 6 ? 2u : 3u) && r.costs[0].symbol == 16611 &&
              r.costs[0].units == 1 && r.costs[1].symbol == 16626 && r.costs[1].units == ore[i],
          "hot steel and each original ore coefficient remain exact");
    check(r.outputs.count == 1 && r.outputs[0].symbol == static_cast<int>(outputs[i]) &&
              r.outputs[0].units == 1,
          "weapon identity is not inferred from list order");
    if (i >= 6)
      check(r.costs[2].symbol == 16156 && r.costs[2].units == 5,
            "both dragon weapons consume five blood per operation");
    unsigned learned = 0;
    for (const auto& q : r.requirements)
      if (q.role == c::RequirementRole::knowledge) {
        ++learned;
        check(q.symbol == 1496 && q.element == i + 1 && q.value == 1,
              "each ore weapon binds its actual learning cell");
      }
    check(learned == 1 && r.require_self_is_hero, "mixed SELF/HERO minimum actor remains bound");
    check(r.hand.anchor == c::HandSide::right && r.hand.right.symbol == 16435 &&
              r.hand.right.require_stored_absent && !r.hand.left.require_stored_absent &&
              r.hand.left_tmp_level == 1 && r.hand.right_tmp_level == 1,
          "actual right hammer anchor and both temporary slots retain their narrow contract");
    check(!r.hand.static_lifecycle_verified && r.hand.observed_in_world,
          "observed shared physical entry is no manufacture proof");
    const auto maximum = static_cast<unsigned>(INT32_MAX) / (i >= 6 ? 5u : ore[i]);
    for (const auto n : {1u, 2u, 5u, maximum}) {
      const auto p = c::plan_quantity(r, n);
      check(p.valid && p.callback == &r.callback && p.quantity == n && p.patch_original == (n > 1),
            "one typed original info handles representable N");
      for (std::size_t j = 0; j < r.callback.amount.count; ++j)
        check(std::uint64_t{r.callback.amount.operands[j].units} * n <= INT32_MAX,
              "every emitted int32 amount is representable");
    }
    check(!c::plan_quantity(r, 0).valid && !c::plan_quantity(r, maximum + 1).valid &&
              !c::plan_quantity(r, UINT32_MAX).valid,
          "unit costs constrain numeric maximum before a writer is possible");
    check(r.callback.amount.count == (i < 6 ? 5u : 7u) &&
              r.callback.function->byte_length == (i < 6 ? 190u : 232u),
          "each independent body has its own full amount contract");
    for (unsigned mutation = 0; mutation < 22; ++mutation) {
      auto wrong = r;
      auto fn = *r.callback.function;
      auto hand = r.hand;
      auto owner = r.owner;
      auto req = r.requirements[1];
      std::vector<ubc::AmountOperand> operands(
          r.callback.amount.operands, r.callback.amount.operands + r.callback.amount.count);
      std::vector<c::ItemAmount> costs(r.costs.begin(), r.costs.end());
      std::vector<c::Requirement> requirements(r.requirements.begin(), r.requirements.end());
      std::vector<c::OperandUse> uses(r.operand_uses.begin(), r.operand_uses.end());
      switch (mutation) {
      case 0:
        wrong.id = static_cast<c::RecipeId>(0);
        break;
      case 1:
        wrong.id = c::recipes[(i + 1) % c::recipes.size()].id;
        break;
      case 2:
        wrong.production_context = 4;
        break;
      case 3:
        wrong.name = "unproven";
        break;
      case 4:
        owner.condition = owner.information;
        wrong.owner = owner;
        break;
      case 5:
        wrong.callback.invocation = c::Invocation::choice;
        break;
      case 6:
        ++fn.byte_offset;
        wrong.callback.function = &fn;
        break;
      case 7:
        wrong.callback.amount.digest[0] ^= 1;
        break;
      case 8:
        ++operands[0].offset;
        wrong.callback.amount.operands = operands.data();
        break;
      case 9:
        ++operands[0].units;
        wrong.callback.amount.operands = operands.data();
        break;
      case 10:
        operands[0].allow_zero = true;
        wrong.callback.amount.operands = operands.data();
        break;
      case 11:
        ++costs[1].units;
        wrong.costs = {costs.data(), costs.size()};
        break;
      case 12:
        req.element = 9;
        requirements[1] = req;
        wrong.requirements = {requirements.data(), requirements.size()};
        break;
      case 13:
        hand.anchor = c::HandSide::left;
        wrong.hand = hand;
        break;
      case 14:
        hand.right.require_stored_absent = false;
        wrong.hand = hand;
        break;
      case 15:
        hand.right_tmp_level = 2;
        wrong.hand = hand;
        break;
      case 16:
        wrong.require_self_is_hero = false;
        break;
      case 17:
        wrong.callback.maximum_quantity = UINT32_MAX;
        break;
      case 18:
        wrong.callback.quantity_text_reviewed = false;
        break;
      case 19:
        uses[0].symbol = 10551;
        wrong.operand_uses = {uses.data(), uses.size()};
        break;
      case 20:
        wrong.targets.count = 0;
        break;
      case 21:
        wrong.success.count = 0;
        break;
      }
      check(!c::structurally_valid(wrong) && !c::plan_quantity(wrong, 1).valid,
            "changed own metadata cannot weaken the recipe contract");
    }
  }
  check(!c::find_callback(20717) && !c::find_callback(20706),
        "navigation and old ordinary weapons cannot become ore effects");
  std::printf("ore_recipe_catalog_tests: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
