// SPDX-License-Identifier: MIT
// The existing RAM fixture exercises our actual reader logic. Its dummy script
// bodies deliberately fail the original hash contract. The positive profile
// flags below are value fixtures only. They do not establish runtime state.
#define main ubc_bow_snapshot_fixture_cases
#include "bow_recipe_snapshot_tests.cpp"
#undef main
#include "ubc/stations/bow/crafting_bow_projection.hpp"
#include "../../helpers/equipped_ingredient.hpp"

namespace {
void synthetic_profiles(ubc::BowRecipeCatalogueCapture& value) {
  for (std::size_t i = 0; i < value.recipes.size(); ++i) {
    value.recipes[i].contract = true;
    auto& p = value.diagnostic.profiles[i];
    p.valid = true;
    p.local = {};
    p.local.matches = true;
    p.string_issue = ubc::alchemy_flask_runtime::Issue::none;
  }
}

void equipped_oak_bow() {
  using Reason = ubc::CraftingBlockReason;
  namespace c = ubc::crafting_catalog;
  CaptureFixture f;
  f.stock({{16828, 1}}, materials(0, 28, 72));
  f.image.put(object(0) + 0x158, 0x40880004u);
  f.image.block(extra_slot, 0x38);
  text_at(f.image, extra_name, "ZS_BOW");
  f.image.put(extra_slot + 8, extra_name);
  f.image.put(extra_slot + 12, 6);
  f.image.put(extra_slot + 0x30, object(0));
  f.image.put(slots + 8, extra_slot);
  f.image.put(hero + 0x9b8, 3);
  const auto read = [&] {
    auto raw = capture(f);
    synthetic_profiles(raw);
    return raw;
  };
  auto raw = read();
  auto snapshot = ubc::project_capture(raw);
  auto row = snapshot.recipes[c::bow_begin + 10];
  check(row.id == 0x800b && static_cast<int>(raw.recipes[10].material.issue) == 10 &&
            ubc::valid_crafting_snapshot(snapshot) && equipped_test::localized(row),
        "the equipped oak bow keeps material issue 10 and explains the lock in every locale");
  check(!snapshot.recipes[c::bow_begin].equipped_ingredient,
        "a different recipe does not inherit the oak bow's equipment slot");
  row.hand_known = false;
  check(ubc::crafting_display_reason(ubc::crafting_block_reason(row), &row) == Reason::hand_unknown,
        "an unverified station hand still takes precedence over its equipped ingredient");
  row.contract = false;
  check(ubc::crafting_display_reason(ubc::crafting_block_reason(row), &row) ==
            Reason::contract_unverified,
        "an unverified contract still precedes both hand and ingredient problems");

  f.stock({{16828, 1}, {16610, 1}}, materials(0, 28, 72));
  f.image.put(object(0) + 0x158, 0x40880004u);
  row = ubc::project_capture(read()).recipes[c::bow_begin + 10];
  check(ubc::crafting_display_reason(ubc::crafting_block_reason(row), &row) ==
            Reason::material_unverified,
        "an equipped bow plus mixed steel retains the ambiguous stock message");
  f.stock({{16828, 1}}, materials(0, 28, 72));
  row = ubc::project_capture(read()).recipes[c::bow_begin + 10];
  check(!row.equipped_ingredient && ubc::crafting_block_reason(row) == Reason::material_unverified,
        "a slot alias without ITEM_ACTIVE stays unclear");
  f.image.put(extra_slot + 0x30, 0);
  for (const auto flags : {0x00880004u, 0x40880005u}) {
    f.image.put(object(0) + 0x158, flags);
    row = ubc::project_capture(read()).recipes[c::bow_begin + 10];
    check(ubc::crafting_display_reason(ubc::crafting_block_reason(row), &row) ==
              Reason::material_unverified,
          "ITEM_NFOCUS alone or additional unexpected flags keep the stock warning");
  }
  f.image.put(object(0) + 0x158, 0x00080004u);
  row = ubc::project_capture(read()).recipes[c::bow_begin + 10];
  check(ubc::crafting_block_reason(row) == Reason::none && row.maximum == 1,
        "unequipping the oak bow restores the original maximum");
}
}
int main() {
  namespace c = ubc::crafting_catalog;
  equipped_oak_bow();
  CaptureFixture fixture;
  const auto actual = capture(fixture);
  auto projected = ubc::project_capture(actual);
  check(ubc::valid_crafting_snapshot(projected),
        "actual Bow reader projects stable material capture");
  for (std::size_t i = 0; i < cat::recipes.size(); ++i) {
    const auto& s = projected.recipes[c::bow_begin + i];
    check(s.known && s.actor_known && s.actor_matches && s.context_matches,
          "actual common talent actor and context are projected");
    check(s.material_verified && s.maximum == 10,
          "real read-only inventory produces exact shared maximum");
    check(!s.contract &&
              ubc::crafting_block_reason(s) == ubc::CraftingBlockReason::contract_unverified,
          "noncanonical dummy bodies never gain execution contract");
  }
  auto positive = actual;
  synthetic_profiles(positive);
  projected = ubc::project_capture(positive);
  check(ubc::valid_crafting_snapshot(projected),
        "synthetic positive profile evidence combines with actual material capture");
  for (std::size_t i = 0; i < cat::recipes.size(); ++i) {
    const auto& s = projected.recipes[c::bow_begin + i];
    check(ubc::crafting_block_reason(s) == ubc::CraftingBlockReason::none,
          "each independently bounded recipe exposes materials");
    for (std::size_t j = 0; j < s.target_count; ++j) {
      const auto& row = s.materials[j];
      const auto p = ubc::crafting_amount_preview(row, 10);
      check(p.available_known && p.end_known && !p.missing && p.end_storage >= 0,
            "all four exact target deltas remain representable");
    }
  }
  for (unsigned bad = 0; bad < 17; ++bad) {
    auto changed = positive;
    switch (bad) {
    case 0:
      changed.key.production_kind = 15;
      break;
    case 1:
      changed.key.generation = 0;
      break;
    case 2:
      changed.diagnostic.issue = ubc::BowCaptureIssue::boundary;
      break;
    case 3:
      changed.recipes[0].id = cat::recipes[1].id;
      break;
    case 4:
      changed.recipes[0].material.quantity = 1;
      break;
    case 5:
      changed.recipes[0].material.craftable = true;
      break;
    case 6:
      changed.recipes[0].rows[0].unit_cost++;
      break;
    case 7:
      changed.recipes[0].rows[0].minimum_per_unit++;
      break;
    case 8:
      changed.hands.interact_item = 1;
      break;
    case 9:
      changed.diagnostic.actor_after.self++;
      break;
    case 10:
      changed.diagnostic.context_after.bows++;
      break;
    case 11:
      changed.diagnostic.knowledge_after.known = false;
      break;
    case 12:
      changed.diagnostic.profiles[0].local.matches = false;
      break;
    case 13:
      changed.recipes[0].rows[0].symbol = changed.recipes[0].rows[1].symbol;
      break;
    case 14:
      changed.recipes[0].rows[0].stored--;
      break;
    case 15:
      changed.diagnostic.inventory_unchanged = false;
      break;
    default:
      changed.diagnostic.hands_unchanged = false;
      break;
    }
    const auto result = ubc::project_capture(changed);
    check(!ubc::valid_crafting_snapshot(result) ||
              ubc::crafting_block_reason(result.recipes[c::bow_begin]) !=
                  ubc::CraftingBlockReason::none,
          "contradictory source facts cannot keep a craftable shared entry");
  }
  fixture.image.put(symbol(1679) + 0x1c, hero + 4);
  auto foreign = capture(fixture);
  synthetic_profiles(foreign);
  const auto strict = ubc::project_capture(foreign), preview = ubc::project_capture(foreign, true);
  check(strict.recipes[c::bow_begin].actor_known && !strict.recipes[c::bow_begin].actor_matches,
        "foreign SELF stays unusable as an original actor");
  check(ubc::valid_crafting_snapshot(preview) && preview.recipes[c::bow_begin].actor_matches,
        "explicit physical Hero preview is separate from actual original actor");
  foreign.diagnostic.actor_after.self++;
  check(!ubc::project_capture(foreign, true).recipes[c::bow_begin].actor_matches,
        "preview cannot hide a changing actor binding");

  CaptureFixture minimum;
  minimum.stock({}, materials(10, 24, 24));
  auto limited = capture(minimum);
  synthetic_profiles(limited);
  const auto two = ubc::project_capture(limited);
  const auto& last = two.recipes[c::cool_begin - 1];
  check(ubc::valid_crafting_snapshot(two) && last.maximum == 2,
        "last bow retains both distinct original constraints");
  const auto* steel = ubc::crafting_detail::material(last, 16610);
  const auto* leather = ubc::crafting_detail::material(last, 16599);
  const auto sp = ubc::crafting_amount_preview(*steel, 2),
             lp = ubc::crafting_amount_preview(*leather, 2);
  check(sp.required == 20 && sp.original_minimum == 24 && sp.end_storage == 4 && !sp.missing,
        "steel minimum is not consumed as invented cost");
  check(lp.required == 24 && lp.original_minimum == 20 && !lp.end_storage && !lp.missing,
        "leather actual cost remains higher than minimum");
  auto less = *steel;
  less.stored = 20;
  const auto shortfall = ubc::crafting_amount_preview(less, 2);
  check(shortfall.required == 20 && shortfall.original_minimum == 24 && shortfall.missing == 4,
        "surplus cost capacity cannot bypass original minimum");
  auto inflated = last;
  inflated.maximum = 3;
  check(!ubc::valid_crafting_state(inflated), "minimum also caps common numeric validation");
  auto altered = last;
  altered.materials[0].unit_minimum++;
  check(!ubc::valid_crafting_state(altered) && !ubc::same_crafting_state(altered, last),
        "minimum coefficient is canonical and request-bound");
  std::printf("crafting_bow_projection_tests: %u checks, %u failures\n", station_test::checks,
              station_test::failures);
  return station_test::failures ? 1 : 0;
}
