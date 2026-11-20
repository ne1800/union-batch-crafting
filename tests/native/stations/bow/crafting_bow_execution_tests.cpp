// SPDX-License-Identifier: MIT
// Binding and UI value fixtures only. The existing bow_item_runtime
// host reads actual local code paths separately with --local-files.
#include "ubc/stations/bow/crafting_bow_execution.hpp"
#include "ubc/stations/bow/crafting_bow_projection.hpp"
#include "ubc/core/crafting_session.hpp"
#define UBC_CRAFTING_FIXTURE_ONLY
#include "../../catalogs/crafting_catalog_tests.cpp"

namespace {
using namespace crafting_test;
ubc::CraftingRequest request(c::RecipeId id, unsigned quantity = 2) {
  return {id, quantity, key(Kind::bow), ready_state(id), hands(Kind::bow)};
}
ubc::bow_item_runtime::Evidence evidence(const ubc::CraftingRequest& r) {
  ubc::bow_item_runtime::Evidence e;
  e.item_path_verified = e.stock.captured = e.modules.observed = e.constructors.values_verified =
      true;
  e.pool.verified = e.sort.stable = e.world.stable = e.renderer.verified = true;
  e.stock.plan = ubc::bow_item_path::make_plan(*c::find(r.id)->bow, r.quantity);
  const auto& k = r.key;
  e.stock.boundary = {
      {k.generation, k.thread, k.world, k.hero, k.station, k.manager, 16}, 100, k.thread, true};
  e.stock.hands = r.hands;
  for (std::size_t i = 0; i < r.material_snapshot.target_count; ++i) {
    const auto& m = r.material_snapshot.materials[i];
    e.stock.materials[i].symbol = m.symbol;
    e.stock.materials[i].total = m.stored;
    e.expected_totals[i] =
        static_cast<std::int32_t>(ubc::crafting_amount_preview(m, r.quantity).end_storage);
  }
  return e;
}
ubc::CraftingRequest selected(const ubc::CraftingSession& s) {
  return {s.selected()->id, s.quantity(), s.preview().key, *s.selected(), s.preview().hands};
}
}
int main() {
  using namespace crafting_test;
  for (const auto& d : c::recipes)
    if (d.bow)
      for (unsigned quantity : {1u, 2u, 7u}) {
        const auto r = request(d.id, quantity);
        const auto good = evidence(r);
        check(ubc::matching_bow_item_path(good, r, 100),
              "complete synthetic witness binds one exact canonical quantity and four end stocks");
        for (unsigned bad = 0; bad < 31; ++bad) {
          auto p = good;
          switch (bad) {
          case 0:
            p.item_path_verified = false;
            break;
          case 1:
            p.stock.captured = false;
            break;
          case 2:
            p.modules.observed = false;
            break;
          case 3:
            p.constructors.values_verified = false;
            break;
          case 4:
            p.pool.verified = false;
            break;
          case 5:
            p.sort.stable = false;
            break;
          case 6:
            p.world.stable = false;
            break;
          case 7:
            p.renderer.verified = false;
            break;
          case 8:
            p.stock.plan.valid = false;
            break;
          case 9:
            p.stock.plan.quantity++;
            break;
          case 10:
            p.stock.plan.id = static_cast<ubc::bow_recipe_catalog::RecipeId>(0);
            break;
          case 11:
            p.stock.plan.callback++;
            break;
          case 12:
            p.stock.boundary.frame++;
            break;
          case 13:
            p.stock.boundary.current_thread++;
            break;
          case 14:
            p.stock.boundary.key.generation++;
            break;
          case 15:
            p.stock.boundary.key.thread++;
            break;
          case 16:
            p.stock.boundary.key.world++;
            break;
          case 17:
            p.stock.boundary.key.hero++;
            break;
          case 18:
            p.stock.boundary.key.station++;
            break;
          case 19:
            p.stock.boundary.key.manager++;
            break;
          case 20:
            p.stock.boundary.key.production_kind++;
            break;
          case 21:
            p.stock.boundary.stable_boundary_verified = false;
            break;
          case 22:
            p.stock.hands.interact_item = 1;
            break;
          case 23:
            p.stock.materials[0].total++;
            break;
          case 24:
            p.stock.materials[1].symbol = p.stock.materials[0].symbol;
            break;
          case 25:
            p.expected_totals[0]++;
            break;
          case 26:
            p.expected_totals[1]++;
            break;
          case 27:
            p.expected_totals[2]++;
            break;
          case 28:
            p.expected_totals[3]++;
            break;
          case 29:
            p.issue = ubc::bow_item_runtime::Issue::changed;
            break;
          default:
            p.stock.hands.left_slot_known = false;
            break;
          }
          check(!ubc::matching_bow_item_path(p, r, 100),
                "stale partial or cross-bound item proof never releases a requested amount");
        }
        auto other = r;
        other.quantity = quantity == 1 ? 2u : 1u;
        check(!ubc::matching_bow_item_path(good, other, 100),
              "N1 evidence cannot authorize another amount");
      }
  const auto snapshot = example(Kind::bow);
  ubc::CraftingSession session;
  check(session.open(snapshot), "Bow UI opens separately from amount availability");
  for (unsigned f = 1; f <= 3; ++f)
    session.observe(f, false, true);
  check(session.armed() && !session.can_confirm() && session.maximum() == 7,
        "material maximum alone never enables manufacture");
  auto checked = selected(session);
  check(session.observe_quantity_check(checked, 3, true) && session.can_confirm(),
        "fresh current amount enables confirmation");
  for (unsigned bad = 0; bad < 10; ++bad) {
    auto r = checked;
    std::uint64_t frame = 3;
    bool available = true;
    switch (bad) {
    case 0:
      frame = 2;
      break;
    case 1:
      frame = 4;
      break;
    case 2:
      r.quantity++;
      break;
    case 3:
      r.id++;
      break;
    case 4:
      r.key.generation++;
      break;
    case 5:
      r.material_snapshot.materials[0].stored++;
      break;
    case 6:
      r.hands.interact_item = 1;
      break;
    case 7:
      available = false;
      break;
    case 8:
      r.material_snapshot.materials[0].unit_minimum++;
      break;
    default:
      r.key.station++;
      break;
    }
    check(!session.observe_quantity_check(r, frame, available) && !session.can_confirm(),
          "UI rejects stale quantity frame context stock and hand evidence");
    check(session.observe_quantity_check(checked, 3, true) && session.can_confirm(),
          "new correctly bound observation can restore availability");
  }
  check(session.preset(2) && !session.can_confirm(),
        "changing amount immediately invalidates previous preview proof");
  for (unsigned f = 4; f <= 6; ++f)
    session.observe(f, false, true);
  checked = selected(session);
  check(session.observe_quantity_check(checked, 6, true) && session.confirm(),
        "exact new amount can be confirmed once");
  for (unsigned f = 7; f <= 9; ++f)
    session.observe(f, false, true);
  check(!session.observe_quantity_check(checked, 9, false) &&
            session.phase() == ubc::SelectionPhase::selecting && !session.take(snapshot, true),
        "loss of fresh availability cancels an already confirmed pending intent");
  for (unsigned f = 10; f <= 12; ++f)
    session.observe(f, false, true);
  checked = selected(session);
  check(session.observe_quantity_check(checked, 12, true),
        "next fresh check resumes ordinary selection");
  session.observe(12, false, true);
  check(!session.observe_quantity_check(checked, 12, true) && !session.can_confirm(),
        "replayed observation frame cannot reauthorize quantity");
  ubc::CraftingSession handoff;
  check(handoff.open(snapshot), "fresh Bow handoff session opens");
  for (unsigned f = 1; f <= 3; ++f)
    handoff.observe(f, false, true);
  checked = selected(handoff);
  check(handoff.observe_quantity_check(checked, 3, true) && handoff.confirm(),
        "verified Bow intent enters release gate");
  for (unsigned f = 4; f <= 6; ++f)
    handoff.observe(f, false, true);
  check(handoff.phase() == ubc::SelectionPhase::ready && !handoff.take(snapshot, true),
        "ready handoff still requires current-frame quantity evidence");
  check(
      handoff.observe_quantity_check(checked, 6, true) && handoff.refresh(snapshot) &&
          handoff.observe_quantity_check(checked, 6, true),
      "fresh second check after simulated UI restoration remains valid in the same observed frame");
  const auto dispatched = handoff.take(snapshot, true);
  check(dispatched && dispatched->id == checked.id && dispatched->quantity == checked.quantity &&
            !handoff.take(snapshot, true),
        "twice verified restored-UI handoff consumes intent exactly once");
  auto gated = snapshot;
  ubc::gate_bow_modules(gated, false);
  check(ubc::valid_crafting_snapshot(gated),
        "known unsupported modules retain a valid read-only catalogue");
  for (std::size_t i = c::bow_begin; i < c::cool_begin; ++i)
    check(gated.recipes[i].known && !gated.recipes[i].contract && !gated.recipes[i].maximum,
          "module rejection visibly blocks every Bow entry before user intent");
  auto unrelated = gated;
  unrelated.recipes[c::cool_begin].contract = true;
  unrelated.recipes[c::cool_begin].maximum = 7;
  ubc::gate_bow_modules(unrelated, false);
  check(unrelated.recipes[c::cool_begin].contract && unrelated.recipes[c::cool_begin].maximum == 7,
        "Bow module gate never changes another family");
  std::printf("crafting_bow_execution_tests: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
