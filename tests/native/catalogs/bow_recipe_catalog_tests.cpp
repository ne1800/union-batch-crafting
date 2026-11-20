// SPDX-License-Identifier: MIT
#include "ubc/catalogs/bow_recipe_catalog.hpp"
#include <cstdio>
#include <set>
#include <cstring>
namespace c = ubc::bow_recipe_catalog;
namespace {
unsigned checks = 0, failures = 0;
void check(bool b, int line) {
  ++checks;
  if (!b) {
    ++failures;
    std::fprintf(stderr, "failure line %d\n", line);
  }
}
#define CHECK(v) check((v), __LINE__)
}
int main() {
  constexpr std::array<unsigned, 20> steel{
      {1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 2, 1, 3, 4, 6, 8, 10}};
  constexpr std::array<unsigned, 20> leather{
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 2, 1, 3, 4, 6, 8, 12}};
  constexpr std::array<int, 20> weapons{{16810, 16812, 16808, 16814, 16816, 16818, 16820,
                                         16822, 16824, 16826, 16828, 16830, 16832, 16806,
                                         16834, 16836, 16838, 16840, 16842, 16844}};
  CHECK(c::recipes.size() == 20 && !c::runtime_ready);
  CHECK(c::catalog_target_capacity == 4);
  std::set<std::uint32_t> ids, owners, callbacks;
  for (std::size_t i = 0; i < c::recipes.size(); ++i) {
    const auto& r = c::recipes[i];
    CHECK(c::structurally_valid(r));
    CHECK(ids.insert(static_cast<std::uint32_t>(r.id)).second);
    CHECK(owners.insert(static_cast<unsigned>(r.owner.instance)).second &&
          callbacks.insert(r.callback.function->symbol).second);
    CHECK(r.callback.function->symbol == 20873 + i * 3 &&
          r.owner.instance == 20871 + static_cast<int>(i) * 3);
    CHECK(r.owner.information == r.callback.function &&
          r.callback.invocation == c::Invocation::info && r.callback.original_quantity == 1);
    CHECK(c::units(r.costs, 16610) == steel[i] && c::units(r.costs, 16599) == leather[i] &&
          c::units(r.costs, weapons[i]) == 1);
    CHECK(r.outputs.count == 1 && r.outputs[0].symbol == weapons[i] + 1 && r.outputs[0].units == 1);
    CHECK(r.minima.count == 3 && c::units(r.minima, weapons[i]) == 1);
    CHECK(c::units(r.minima, 16610) == (i == 19 ? 12u : steel[i]) &&
          c::units(r.minima, 16599) == (i == 19 ? 10u : leather[i]));
    CHECK(r.callback.amount.count == 7 && r.operand_uses.count == 7);
    for (std::size_t j = 0; j < r.callback.amount.count; ++j) {
      const auto& o = r.callback.amount.operands[j];
      const auto& use = r.operand_uses[j];
      CHECK(o.offset == use.offset && o.original > 0 &&
            o.units == static_cast<unsigned>(o.original) && !o.allow_zero && !o.reserved);
      CHECK(o.offset > 0 && o.offset + 4 <= r.callback.amount.length);
      if (use.role == c::AmountRole::minimum_inventory)
        CHECK(o.units == c::units(r.minima, use.symbol));
      else if (use.role == c::AmountRole::remove_count)
        CHECK(o.units == c::units(r.costs, use.symbol));
      else
        CHECK(o.units == c::units(r.outputs, use.symbol));
    }
    CHECK(c::plan_quantity(r, 1).valid && !c::plan_quantity(r, 1).patch_original &&
          c::plan_quantity(r, 2).patch_original);
    CHECK(c::plan_quantity(r, r.callback.maximum_quantity).valid && !c::plan_quantity(r, 0).valid &&
          !c::plan_quantity(r, r.callback.maximum_quantity + 1).valid);
    bool learned = false;
    for (const auto& q : r.checks)
      if (q.requirement.role == c::RequirementRole::knowledge) {
        learned = true;
        CHECK(q.function->symbol == 20870 && q.site == c::CheckSite::owner_information &&
              q.requirement.symbol == 10484 && q.requirement.value == 1);
      }
    CHECK(learned);
    for (const auto& q : r.owner.conditions)
      CHECK(q.role != c::RequirementRole::knowledge);
    for (const auto& path : {r.success, r.failure}) {
      unsigned ends = 0, resets = 0;
      for (const auto& e : path) {
        ends += e.kind == c::EffectKind::end_production ? 1u : 0u;
        if (e.kind == c::EffectKind::integer_store) {
          ++resets;
          CHECK(e.symbol == 20862 && e.value == 0);
        }
      }
      CHECK(ends == 1 && resets == 1 && path[path.count - 1].kind == c::EffectKind::integer_store);
    }
    for (const auto& item : r.items) {
      CHECK(item.constructor && item.native_flags == (item.mainflag | item.script_flags) &&
            !item.constructor_calls);
      if (item.symbol == weapons[i] || item.symbol == weapons[i] + 1)
        CHECK(item.mainflag == 4 &&
              (item.script_flags == 0x80000 || item.script_flags == 0x100000));
    }
    for (unsigned mutation = 0; mutation < 29; ++mutation) {
      auto bad = r;
      auto cb = r.callback;
      auto function = *r.callback.function;
      auto hand = r.hand;
      auto station = *r.station_profile;
      std::array<ubc::AmountOperand, 7> operands{};
      std::array<c::OperandUse, 7> uses{};
      std::array<c::ItemAmount, 3> costs{}, minima{};
      std::array<c::ItemShape, 4> items{};
      for (std::size_t j = 0; j < 7; ++j) {
        operands[j] = cb.amount.operands[j];
        uses[j] = r.operand_uses[j];
      }
      for (std::size_t j = 0; j < 3; ++j) {
        costs[j] = r.costs[j];
        minima[j] = r.minima[j];
      }
      for (std::size_t j = 0; j < 4; ++j)
        items[j] = r.items[j];
      switch (mutation) {
      case 0:
        bad.id = static_cast<c::RecipeId>(1);
        break;
      case 1:
        bad.owner.number++;
        break;
      case 2:
        bad.owner.npc++;
        break;
      case 3:
        function.byte_offset++;
        bad.callback.function = &function;
        break;
      case 4:
        function.body_sha256[0] ^= 1;
        bad.callback.function = &function;
        break;
      case 5:
        bad.callback.maximum_quantity++;
        break;
      case 6:
        operands[0].offset++;
        bad.callback.amount.operands = operands.data();
        break;
      case 7:
        operands[0].units++;
        bad.callback.amount.operands = operands.data();
        break;
      case 8:
        operands[0].original++;
        bad.callback.amount.operands = operands.data();
        break;
      case 9:
        bad.callback.amount.count = 6;
        break;
      case 10:
        uses[0].role = c::AmountRole::create_count;
        bad.operand_uses = {uses.data(), uses.size()};
        break;
      case 11:
        costs[0].units++;
        bad.costs = {costs.data(), costs.size()};
        break;
      case 12:
        minima[0].units++;
        bad.minima = {minima.data(), minima.size()};
        break;
      case 13:
        bad.targets.count = 3;
        break;
      case 14:
        bad.bindings.count--;
        break;
      case 15:
        bad.requirements.count--;
        break;
      case 16:
        bad.checks.count--;
        break;
      case 17:
        bad.require_self_is_hero = false;
        break;
      case 18:
        bad.hand.left_empty = false;
        break;
      case 19:
        bad.hand.anchor_zero = false;
        break;
      case 20:
        bad.hand.observed_in_world = true;
        break;
      case 21:
        station.production_context = 17;
        bad.station_profile = &station;
        break;
      case 22:
        station.on_state = "WOODSAW";
        bad.station_profile = &station;
        break;
      case 23:
        bad.failure.count--;
        break;
      case 24:
        bad.prints.count--;
        break;
      case 25:
        items[0].native_flags ^= 4;
        bad.items = {items.data(), items.size()};
        break;
      case 26:
        bad.quantity_text_reviewed = !bad.quantity_text_reviewed;
        break;
      case 27:
        bad.callback.invocation = c::Invocation::choice;
        break;
      case 28:
        bad.success.count--;
        break;
      }
      (void)hand;
      CHECK(!c::structurally_valid(bad) && !c::plan_quantity(bad, 1).valid);
    }
  }
  CHECK(c::station.entry->symbol == 20864 && c::station.end.information->symbol == 20867 &&
        c::station.category.information->symbol == 20870);
  CHECK(std::strcmp(c::station.on_state, "MAKEBOWS") == 0 &&
        std::strcmp(c::station.visual, "BOWSDW_WBENCH.MDS") == 0 && c::station.state_num == 1 &&
        !c::station.rewind);
  CHECK(c::hand.left_empty && c::hand.right_empty && c::hand.anchor_zero &&
        c::hand.static_animation_no_items && !c::hand.observed_in_world);
  std::printf("bow_recipe_catalog_tests: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
