// SPDX-License-Identifier: MIT
// Our actual reader logic on synthetic RAM. Only our own value
// fixtures set matching profile flags. No game, code or hook evidence.
#define main ubc_cool_snapshot_fixture_cases
#include "cool_recipe_snapshot_tests.cpp"
#undef main
#include "ubc/stations/cool/crafting_cool_projection.hpp"
#include "ubc/core/crafting_handoff.hpp"
#include "ubc/core/crafting_result.hpp"
#include "ubc/ui/native_crafting_ui.hpp"
#include "../../helpers/equipped_ingredient.hpp"

namespace shared_cool_test {
namespace cat = ubc::crafting_catalog;
using R = ubc::CraftingResultState;
using P = ubc::CoolActorPurpose;
ubc::CoolRecipeCatalogueCapture source(int hot = 7, int cold = 2, std::uint32_t self = hero) {
  CaptureFixture f;
  std::string packed;
  if (hot)
    packed = "ITMISWORDBLADEHOT:" + std::to_string(hot);
  if (cold)
    packed += (packed.empty() ? "" : ",") + std::string("ITMISWORDBLADE:") + std::to_string(cold);
  f.stock({}, packed);
  f.image.put(symbol(1679) + 0x1c, self);
  auto out = capture(f);
  out.recipe.contract = true;
  out.diagnostic.profile.valid = out.diagnostic.profile.local.matches = true;
  return out;
}
ubc::CraftingSnapshot snapshot(int hot = 7, int cold = 2) {
  return ubc::project_capture(source(hot, cold));
}
ubc::CraftingRequest request(const ubc::CraftingSnapshot& s, unsigned n) {
  return {0xa001, n, s.key, s.recipes[cat::cool_begin], s.hands};
}
ubc::CraftingResultBinding binding(const ubc::CraftingRequest& r) {
  return {r.key.generation, 10, 4, 0, r.key.world, r.key.hero, r.key.station};
}
ubc::CraftingResultEndWitness ended(const ubc::CraftingRequest& r) {
  return {true, true, r.key.generation, 1, 11, r.key.world, r.key.hero, r.key.station};
}
ubc::CraftingResultDestroyWitness destroyed(const ubc::CraftingRequest& r) {
  return {true, true, r.key, r.id, r.quantity, 1, 11, 16612, 1};
}
struct Observation {
  std::array<ubc::RecipeResultItem, 2> totals{};
  ubc::CraftingResultObservation value{};
  explicit Observation(const ubc::CraftingRequest& r) {
    for (std::size_t i = 0; i < 2; ++i) {
      const auto& row = r.material_snapshot.materials[i];
      totals[i] = {row.symbol, ubc::crafting_amount_preview(row, r.quantity).end_storage};
    }
    value.generation = r.key.generation;
    value.frame = 12;
    value.stable_boundary_verified = value.identity_known = true;
    value.world = r.key.world;
    value.hero = r.key.hero;
    value.station_known = value.production_known = value.stock_known = value.hands_known = true;
    value.items = {totals.data(), totals.size()};
    value.hands.hands = {{0, -1, -1, 0}, {0, -1, -1, 0}};
    value.hands.left_slot_known = value.hands.right_slot_known = value.hands.interact_item_known =
        true;
  }
};
bool contains(const ubc::crafting_ui::Frame& f, const char* text) {
  for (std::size_t i = 0; i < f.count; ++i)
    if (std::strstr(f.lines[i].text.data(), text))
      return true;
  return false;
}
void profiles() {
  const auto* d = cat::find(0xa001);
  check(d && d->station == cat::Station::cool && static_cast<unsigned>(d->station) == 7 &&
            d->route == cat::Route::cool,
        "canonical id and append-only station7");
  check(d && d->cool == &ubc::cool_recipe_catalog::recipe &&
            d->completion == cat::Completion::end_production,
        "own original cooling descriptor and terminal model");
  check(cat::recipe_count >= 73 && cat::cool_begin == 72 && cat::target_capacity >= 2,
        "Cool position remains unchanged when later families append");
  const auto hand = cat::hand_amounts(*d, 16612);
  check(hand.generated == 1 && !hand.consumed && !hand.tool && !hand.stored_absent,
        "hot hand generated separately from genuine stored ingredient");
  check(std::strcmp(ubc::ui_text::de::table.item(16613), "Klinge") == 0,
        "cold label bound to original NAME text46465");
  CaptureFixture f;
  const auto real = capture(f);
  const auto blocked = ubc::project_capture(real);
  check(ubc::valid_crafting_snapshot(blocked) &&
            blocked.recipes[cat::cool_begin].material_verified &&
            !blocked.recipes[cat::cool_begin].contract,
        "real reader fixture projects stock but keeps bad script bodies closed");
  f.stock({{16612, 5}}, "ITMISWORDBLADE:2");
  f.image.put(object(0) + 0x158, 0x40200001u);
  auto equipped = capture(f);
  equipped.recipe.contract = true;
  equipped.diagnostic.profile.valid = equipped.diagnostic.profile.local.matches = true;
  check(
      listed_input(f, equipped) &&
          equipped_test::localized(ubc::project_capture(equipped).recipes[cat::cool_begin]),
      "ITEM_ACTIVE on the stored hot blade explains the lock independently of the generated hand blade");
  const auto foreign = source(7, 2, 0x3000);
  const auto strict = ubc::project_capture(foreign),
             preview = ubc::project_capture(foreign, P::hero_preview, true);
  check(!strict.recipes[cat::cool_begin].actor_matches &&
            preview.recipes[cat::cool_begin].actor_matches,
        "physical hero preview is separate from original SELF binding");
  check(!ubc::project_capture(foreign, P::hero_preview).recipes[cat::cool_begin].actor_matches,
        "physical hero requires explicit current adapter proof");
  check(!foreign.recipe.actor_matches && foreign.diagnostic.actor_before.self == 0x3000,
        "projection preserves raw foreign SELF evidence");
  for (unsigned bad = 0; bad < 27; ++bad) {
    auto altered = source();
    switch (bad) {
    case 0:
      altered.key.production_kind = 12;
      break;
    case 1:
      altered.key.world = 0;
      break;
    case 2:
      altered.diagnostic.issue = ubc::CoolCaptureIssue::boundary;
      break;
    case 3:
      altered.recipe.material.eligible = true;
      break;
    case 4:
      altered.recipe.material.numerical.quantity = 1;
      break;
    case 5:
      altered.recipe.material.rows[0].unit_cost = 2;
      break;
    case 6:
      altered.recipe.material.rows[0].symbol = 16613;
      break;
    case 7:
      altered.recipe.material.rows[0].reserved = 1;
      break;
    case 8:
      altered.recipe.material.rows[0].available++;
      break;
    case 9:
      altered.recipe.material.rows[0].held = 0;
      break;
    case 10:
      altered.recipe.material.maximum = 3;
      break;
    case 11:
      altered.recipe.material.direct_maximum = 3;
      break;
    case 12:
      altered.recipe.material.numerical.maximum = 3;
      break;
    case 13:
      altered.hands.interact_item = 0;
      break;
    case 14:
      altered.hands.left_tmp_level = 0;
      break;
    case 15:
      altered.diagnostic.actor_after.self++;
      break;
    case 16:
      altered.diagnostic.context_after.start_smithing = 0;
      break;
    case 17:
      altered.diagnostic.knowledge_after.known = false;
      break;
    case 18:
      altered.diagnostic.profile.local.matches = false;
      break;
    case 19:
      altered.diagnostic.inventory_unchanged = false;
      break;
    case 20:
      altered.diagnostic.hands_unchanged = false;
      break;
    case 21:
      altered.recipe.material.ingredients_verified = false;
      break;
    case 22:
      altered.recipe.actor_known = false;
      break;
    case 23:
      altered.recipe.context_matches = false;
      break;
    case 24:
      altered.recipe.material.rows[1].stored++;
      altered.recipe.material.rows[1].available++;
      break;
    case 25:
      altered.recipe.material.numerical.after_callback[0].amount++;
      break;
    case 26:
      altered.recipe.material.numerical.native_end_debit = 1;
      break;
    }
    const auto projected = ubc::project_capture(altered, P::hero_preview, true);
    check(!ubc::valid_crafting_snapshot(projected) ||
              ubc::crafting_block_reason(projected.recipes[cat::cool_begin]) !=
                  ubc::CraftingBlockReason::none,
          "contradictory source cannot retain an executable projection");
  }
}
void quantities() {
  static_assert(ubc::cool_predebit::runtime_ready, "der weltgeprüfte Vorentzug ist freigeschaltet");
  for (int hot = 1; hot <= 9; ++hot)
    for (const int cold : {0, 2, INT32_MAX - 2}) {
      const auto snap = snapshot(hot, cold);
      const auto& state = snap.recipes[cat::cool_begin];
      check(ubc::valid_crafting_snapshot(snap),
            "current known cooling material projects valid snapshot");
      const auto maximum = static_cast<unsigned>(std::min(hot, INT32_MAX - cold));
      check(state.maximum == maximum,
            "die Kühlvorschau berücksichtigt verfügbares Material und Ausgaberaum");
      for (unsigned n = 1; n <= maximum; ++n) {
        const auto r = request(snap, n);
        check(ubc::crafting_handoff_detail::valid_request(r),
              "geprüfter Lagerbestand und Hände erlauben die angefragte Menge");
        const auto reason = ubc::crafting_quantity_block_reason(state, n);
        check(reason == ubc::CraftingBlockReason::none,
              "jede gültige Menge behält ihre rechnerische Kapazität");
        const auto numerical = ubc::crafting_cool_plan(state, n);
        check(numerical.valid && numerical.after_end[0].amount == std::int64_t{hot} - n &&
                  numerical.after_end[1].amount == std::int64_t{cold} + n,
              "existing plan supplies exact full end balance");
        const auto page =
            ubc::crafting_page(ubc::cool_recipe_navigation::Page{true, true, hot >= 3});
        const auto plan = ubc::make_crafting_plan(page, r);
        const bool single = n == 1 || hot == 2;
        check(ubc::valid_crafting_plan(plan) && plan.count == 1 && plan.effect() &&
                  plan.effect()->function_symbol == (single ? 20775 : 20778) &&
                  plan.effect()->owner_instance == (single ? 20773 : 20776) &&
                  plan.effect()->invocation == ubc::recipe_catalog::Invocation::info,
              "genau eine zum Lager und zur Menge passende Original-INFO");
        check(!ubc::make_crafting_plan(page, r.id, n).valid(),
              "ohne Materialbindung keine Wahl des Kühlcallbacks");
      }
    }
  const auto h2 = snapshot(2, 6);
  ubc::CraftingSession session;
  check(session.open(h2), "Kühlvorschau öffnet bei Lager 2/6");
  session.observe(1, false, true);
  session.observe(2, false, true);
  session.adjust(1);
  session.preset(10);
  check(session.maximum() == 2 && session.quantity() == 2,
        "Schritt und Schnellwahl erreichen zwei Klingen");
  const ubc::cool_recipe_navigation::Page single{true, true, false};
  const auto ready = []() { return true; };
  const auto r = request(h2, 2);
  check(ubc::crafting_handoff_detail::valid_request(r) &&
            session.observe_quantity_check(r, 2,
                                           ubc::cool_quantity_available(r, h2, single, ready)) &&
            session.can_confirm(),
        "Zweierauftrag mit aktuellem Hand- und Lagerbeleg bestätigbar");
  ubc::crafting_ui::Frame frame;
  ubc::CraftingMenu menu;
  check(ubc::crafting_ui::compose(frame, session, menu, true, 1280, 720, ubc::ui_text::de::table) &&
            contains(frame, "Menge: 2 / 2") && contains(frame, "Herstellen"),
        "reguläres Menü zeigt die bestätigte Menge 2");
  check(!ubc::cool_quantity_available(r, h2, single, []() { return false; }),
        "fehlender Writer sperrt den Vorentzug");
  check(!ubc::cool_quantity_matches(r, h2, {true, false, false}, ready) &&
            !ubc::cool_quantity_matches(r, h2, {true, true, true}, ready) &&
            !ubc::cool_quantity_matches(r, snapshot(3, 0), single, ready),
        "falsche Seite oder anderer Bestand sperrt den Zweierpfad");
  const auto scarce = snapshot(3, INT32_MAX - 2);
  const auto triple = ubc::make_crafting_plan(
      ubc::crafting_page(ubc::cool_recipe_navigation::Page{true, true, true}), request(scarce, 2));
  check(triple.effect() && triple.effect()->owner_instance == 20776 &&
            triple.effect()->function_symbol == 20778,
        "nur Ausgaberaum zwei bei Hot3 bleibt Dreier-INFO");
  auto bad = ubc::make_crafting_plan(ubc::crafting_page(single), r);
  bad.steps[0].owner_instance = 20776;
  bad.steps[0].function_symbol = 20778;
  check(!ubc::valid_crafting_plan(bad),
        "umetikettierte Dreier-INFO scheitert bei der Rekonstruktion");
  ubc::CraftingHandoff handoff;
  check(!handoff.begin(r, triple, {}) && handoff.issue() == ubc::CraftingHandoffIssue::plan,
        "gültiger Plan aus fremdem Lager wird vor dem Ticket verworfen");
  const auto h7 = snapshot(7, 0);
  ubc::CraftingSession normal;
  check(normal.open(h7), "arbitrary direct quantities enter common UI");
  normal.observe(1, false, true);
  normal.observe(2, false, true);
  check(normal.adjust(6) && normal.quantity() == 7, "N7 is not clamped to3");
  check(normal.observe_quantity_check(request(h7, 7), 2, true) && normal.can_confirm(),
        "fresh N7 frame evidence enables requested quantity");
  normal.observe(3, false, true);
  check(!normal.can_confirm(), "next frame invalidates old quantity proof");
  check(!normal.observe_quantity_check(request(h7, 1), 3, true) && !normal.can_confirm(),
        "different quantity cannot lend current proof");
}
void results() {
  for (const auto n : {1u, 2u, 3u, 7u}) {
    const auto r = request(snapshot(8, 2), n);
    Observation observed(r);
    ubc::CraftingResult guard;
    check(guard.begin(r, binding(r)) && guard.restored(true) && guard.end_after(ended(r)),
          "existing normal end and restore accepted for cooling");
    check(guard.observe(observed.value) == R::waiting && !guard.permits_next(),
          "correct final totals and normal end cannot invent Destroy");
    check(guard.destroy_after(destroyed(r)),
          "explicit positive original Destroy witness binds current job");
    ++observed.value.frame;
    check(guard.observe(observed.value) == R::verified,
          "later complete quiet frame verifies after real Destroy and end");
    ubc::CraftingResultSuccess success;
    const bool receipt = n != 1 && n != 3;
    check(guard.success(success) && success.quantity_receipt_required == receipt &&
              !success.quantity_receipt_verified &&
              success.terminal == ubc::CraftingResultTerminal::ended &&
              success.produced_count == 1 && success.produced[0].symbol == 16613 &&
              success.produced[0].amount == static_cast<int>(n),
          "verified material result carries exact output and separate receipt obligation");
    check(guard.permits_next() == !receipt,
          "required receipt gates next job independently from material success");
    ubc::NativeCraftingSuccess notice;
    notice.result_verified = notice.restore_verified = true;
    notice.receipt = success.receipt;
    notice.key = r.key;
    notice.id = r.id;
    notice.quantity = n;
    notice.produced_count = 1;
    notice.produced[0] = {16613, static_cast<int>(n)};
    notice.stock_count = 2;
    for (std::size_t i = 0; i < 2; ++i)
      notice.stocks[i] = {success.stocks[i].symbol, success.stocks[i].stored};
    ubc::crafting_ui::Frame frame;
    char amount[64]{};
    std::snprintf(amount, sizeof(amount), "%u Klingen abgeschlossen", n);
    check(
        ubc::crafting_ui::valid_success(notice) &&
            ubc::crafting_ui::compose_success(frame, notice, 1280, 720, ubc::ui_text::de::table) &&
            contains(frame, amount) && contains(frame, "Klinge: +"),
        "existing success UI renders true N with bound cold name");
    check(!guard.acknowledge_quantity_receipt(0) &&
              !guard.acknowledge_quantity_receipt(success.receipt + 1),
          "missing or foreign render receipt never acknowledges job");
    if (receipt) {
      auto next = r;
      next.key.generation++;
      next.material_snapshot.generation++;
      auto next_binding = binding(next);
      next_binding.start_frame = observed.value.frame + 1;
      check(!guard.begin(next, next_binding) && !guard.permits_next(),
            "unacknowledged result cannot be overwritten by next generation");
      check(guard.acknowledge_quantity_receipt(success.receipt) && guard.permits_next(),
            "actual matching submitted receipt completes presentation duty");
      check(guard.success(success) && success.quantity_receipt_verified &&
                !guard.acknowledge_quantity_receipt(success.receipt),
            "receipt is one-shot and persisted separately");
    } else
      check(!guard.acknowledge_quantity_receipt(success.receipt) && guard.permits_next(),
            "N1 N3 do not invent mandatory extra receipt");
  }
  {
    const auto r = request(snapshot(2, 0), 2);
    ubc::CraftingResult guard;
    check(!guard.begin(r, binding(r)), "result model rejects H2N2 even if caller bypassed UI");
  }
  for (unsigned mutation = 0; mutation < 16; ++mutation) {
    const auto r = request(snapshot(), 2);
    ubc::CraftingResult guard;
    auto witness = destroyed(r);
    check(guard.begin(r, binding(r)), "independent invalid witness fixture starts");
    switch (mutation) {
    case 0:
      witness.after_original = false;
      break;
    case 1:
      witness.normal_remove_verified = false;
      break;
    case 2:
      witness.key.generation++;
      break;
    case 3:
      witness.key.thread++;
      break;
    case 4:
      witness.key.world++;
      break;
    case 5:
      witness.key.hero++;
      break;
    case 6:
      witness.key.station++;
      break;
    case 7:
      witness.key.manager++;
      break;
    case 8:
      witness.key.kind = cat::Station::anvil;
      break;
    case 9:
      witness.recipe++;
      break;
    case 10:
      witness.quantity++;
      break;
    case 11:
      witness.serial = 0;
      break;
    case 12:
      witness.frame = 9;
      break;
    case 13:
      witness.instance = 16613;
      break;
    case 14:
      witness.amount = 0;
      break;
    case 15:
      witness.amount = 2;
      break;
    }
    check(!guard.destroy_after(witness) && guard.state() == R::failed &&
              !guard.destroy_after(destroyed(r)),
          "invalid Destroy quarantines instead of accepting later replacement");
  }
  for (unsigned mutation = 0; mutation < 5; ++mutation) {
    const auto r = request(snapshot(), 2);
    Observation observed(r);
    ubc::CraftingResult guard;
    check(guard.begin(r, binding(r)) && guard.restored(true) && guard.end_after(ended(r)) &&
              guard.observe(observed.value) == R::waiting,
          "missing Destroy leaves inner end guard unfrozen");
    check(guard.destroy_after(destroyed(r)), "late positive witness binds original event frame");
    ++observed.value.frame;
    if (mutation == 1)
      ++observed.totals[0].total;
    if (mutation == 2)
      observed.value.hands_known = false;
    if (mutation == 3)
      observed.value.hands.hands.left = {1, 16612, 1, hand_object};
    if (mutation == 4)
      observed.value.hands.interact_item = hand_object;
    check(guard.observe(observed.value) == (mutation == 0   ? R::verified
                                            : mutation == 2 ? R::waiting
                                                            : R::failed),
          "late positive witness rechecks current totals hands and anchor");
  }
  {
    const auto r = request(snapshot(), 1);
    Observation observed(r);
    ubc::CraftingResult guard;
    check(guard.begin(r, binding(r)) && guard.destroy_after(destroyed(r)) &&
              !guard.destroy_after(destroyed(r)) && guard.state() == R::failed,
          "second Destroy is a permanent failure");
  }
  {
    const auto r = request(snapshot(), 1);
    ubc::CraftingResult guard;
    auto witness = destroyed(r);
    witness.frame = 12;
    check(guard.begin(r, binding(r)) && guard.end_after(ended(r)) &&
              !guard.destroy_after(witness) && guard.state() == R::failed,
          "Destroy after already observed normal end is inconsistent");
  }
}
}
int main() {
  shared_cool_test::profiles();
  shared_cool_test::quantities();
  shared_cool_test::results();
  std::printf("crafting_cool_shared_tests: %u checks, %u failures\n", station_test::checks,
              station_test::failures);
  return station_test::failures ? 1 : 0;
}
