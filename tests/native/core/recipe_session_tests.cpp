// SPDX-License-Identifier: MIT
#include "ubc/core/recipe_session.hpp"

#include <cstdio>
#include <cstdlib>
#include <utility>

namespace {
using Id = ubc::recipe_catalog::RecipeId;
using Kind = ubc::recipe_catalog::Station;
using Phase = ubc::SelectionPhase;
using Reason = ubc::RecipeBlockReason;
unsigned checks = 0, failures = 0;
void check(bool ok, const char* message) {
  ++checks;
  if (!ok) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", message);
  }
}
ubc::RecipeState& state(ubc::CatalogueSnapshot& snapshot, Id id) {
  for (auto& entry : snapshot.recipes)
    if (entry.id == id)
      return entry;
  std::abort();
}
void available(ubc::CatalogueSnapshot& snapshot, Id id, std::uint32_t maximum) {
  auto& entry = state(snapshot, id);
  entry.known = entry.contract = entry.hand_known = entry.material_known = true;
  entry.maximum = maximum;
  const auto* descriptor = ubc::recipe_catalog::find(id);
  for (std::size_t i = 0; i < entry.target_count; ++i) {
    auto& row = entry.materials[i];
    row.stored = static_cast<std::int32_t>(std::uint64_t{row.unit_cost} * maximum);
    row.reserved = 0;
    if (descriptor->hand.from_inventory && row.symbol == descriptor->hand.active_item_symbol) {
      row.reserved = 1;
      if (row.stored)
        --row.stored;
    }
  }
}
ubc::CatalogueSnapshot example(std::uint32_t maximum = 23) {
  ubc::CatalogueSnapshot result;
  result.key = {1, 2, 3, 4, 5, 6, Kind::stove};
  available(result, Id::meat, maximum);
  return result;
}
void neutral(ubc::RecipeSession& session, std::uint64_t& frame) {
  for (int i = 0; i < 3; ++i)
    session.observe(++frame, false, true);
}

void visibility_and_values() {
  auto snapshot = example();
  for (const auto& descriptor : ubc::recipe_catalog::recipes)
    available(snapshot, descriptor.id, 23);
  ubc::RecipeSession session;
  check(session.open(snapshot) && session.visible_count() == 3,
        "all known catalogue values filter down to this station's three recipes");
  check(session.visible(0)->id == Id::meat && session.visible(1)->id == Id::molerat_dish &&
            session.visible(2)->id == Id::mushroom_dish && !session.visible(3),
        "visible indices never include another station or nonexistent recipe");
  std::uint64_t frame = 0;
  neutral(session, frame);
  session.preset(23);
  neutral(session, frame);
  check(!session.select_recipe(Id::health_01) && session.selected()->id == Id::meat,
        "known recipe from another station cannot be selected");
  check(session.select_recipe(Id::molerat_dish) && session.quantity() == 1 && !session.armed(),
        "recipe change preserves identity but resets amount and release gate");
  check(!session.confirm(), "recipe change and confirmation cannot share a held action");
  neutral(session, frame);
  check(session.select_relative(1) && session.selected()->id == Id::mushroom_dish,
        "relative selection reaches next visible recipe");
  neutral(session, frame);
  check(!session.select_relative(1) && !session.select_relative(0),
        "recipe selection stops at list edge");
  check(session.select_relative(-1) && session.selected_index() == 1,
        "reverse selection uses visible indices");

  auto alchemy = snapshot;
  alchemy.key.kind = Kind::alchemy;
  ubc::RecipeSession other;
  check(other.open(alchemy) && other.visible_count() == 6 && other.selected()->id == Id::health_01,
        "alchemy independently lists all six known supported recipes");
  for (std::size_t i = 0; i < other.visible_count(); ++i)
    check(other.visible(i)->target_count ==
              ubc::recipe_catalog::find(other.visible(i)->id)->targets.count,
          "every displayed recipe keeps its actual descriptor target count");

  auto empty = example();
  state(empty, Id::meat).known = false;
  ubc::RecipeSession blank;
  frame = 0;
  check(blank.open(empty) && blank.visible_count() == 0 && !blank.selected() &&
            blank.quantity() == 0 && blank.reason() == Reason::no_known_recipes,
        "empty known-recipe list is a valid explicit UI state");
  neutral(blank, frame);
  check(!blank.confirm() && !blank.select_relative(1),
        "empty list cannot create a native-independent request");
  available(empty, Id::molerat_dish, 2);
  check(!blank.refresh(empty) && blank.selected()->id == Id::molerat_dish &&
            blank.quantity() == 1 && !blank.armed(),
        "newly learned first recipe becomes visible but needs fresh input");
  neutral(blank, frame);
  blank.confirm();
  neutral(blank, frame);
  auto changed = empty;
  state(changed, Id::molerat_dish).known = false;
  check(!blank.refresh(changed) && blank.phase() == Phase::original && !blank.take(empty, true),
        "losing selected knowledge cancels instead of manufacturing another visible recipe");
}

void unavailable_and_preview() {
  for (unsigned mode = 0; mode < 5; ++mode) {
    auto snapshot = example(mode < 3 ? 23 : 0);
    auto& meat = state(snapshot, Id::meat);
    Reason expected = Reason::missing_material;
    if (mode == 0) {
      meat.contract = false;
      expected = Reason::contract_unverified;
    }
    if (mode == 1) {
      meat.hand_known = false;
      expected = Reason::hand_unknown;
    }
    if (mode == 2) {
      meat.material_known = false;
      expected = Reason::material_unknown;
    }
    if (mode == 4) {
      meat.materials[0].stored = 3;
      meat.materials[1].stored = INT32_MAX;
      expected = Reason::output_capacity;
    }
    ubc::RecipeSession session;
    std::uint64_t frame = 0;
    check(session.open(snapshot) && session.visible_count() == 1 && session.reason() == expected,
          "known unavailable recipe remains visible with its distinct reason");
    neutral(session, frame);
    check(!session.can_confirm() && !session.confirm(),
          "each missing manufacture gate blocks confirmation");
    neutral(session, frame);
    check(!session.take(snapshot, true), "unavailable preview cannot become an executable request");
  }
  auto snapshot = example();
  available(snapshot, Id::molerat_dish, 0);
  const auto& meal = state(snapshot, Id::molerat_dish);
  for (std::size_t i = 0; i < meal.target_count; ++i) {
    const auto preview = ubc::recipe_amount_preview(meal.materials[i], 1);
    check(preview.available_known && preview.required == meal.materials[i].unit_cost &&
              preview.missing == meal.materials[i].unit_cost &&
              preview.produced == meal.materials[i].unit_output,
          "zero-maximum recipe still explains exact missing quantities for one cycle");
  }
  auto row = meal.materials[0];
  row.stored = 12;
  row.reserved = 0;
  auto preview = ubc::recipe_amount_preview(row, 2);
  check(preview.required == 10 && preview.missing == 0 && preview.remaining == 2,
        "material preview counts recipe cycles rather than output items");
  row.stored = -1;
  preview = ubc::recipe_amount_preview(row, 2);
  check(!preview.available_known && preview.available == -1 && preview.missing == -1 &&
            preview.required == 10,
        "unknown storage does not turn into a manufactured zero inventory");
  row = {16604, 0, 1, 1, 0};
  preview = ubc::recipe_amount_preview(row, 1);
  check(preview.available == 1 && preview.missing == 0 && preview.remaining == 0,
        "copied hand reservation participates exactly once in preview");
  row = {1, INT32_MAX, INT32_MAX, UINT32_MAX, UINT32_MAX};
  preview = ubc::recipe_amount_preview(row, INT32_MAX);
  check(preview.available == 2LL * INT32_MAX && preview.remaining == 2LL * INT32_MAX,
        "large pure value preview avoids signed intermediate overflow");
  check(!ubc::recipe_amount_preview(row, UINT32_MAX).available_known,
        "out-of-range quantity has no claimed preview");
}

void quantity_and_request() {
  for (const auto maximum : {0u, 1u, 4u, 5u, 9u, 10u, 23u, static_cast<unsigned>(INT32_MAX)}) {
    const auto snapshot = example(maximum);
    ubc::RecipeSession session;
    std::uint64_t frame = 0;
    check(session.open(snapshot) && session.quantity() == (maximum ? 1u : 0u) &&
              session.maximum() == maximum,
          "opening resets every finite maximum to one or disabled zero");
    neutral(session, frame);
    for (const auto preset : {0u, 1u, 5u, 10u, maximum, UINT32_MAX}) {
      const auto previous = session.quantity();
      const bool allowed = preset && preset <= maximum;
      check(session.preset(preset) == allowed &&
                session.quantity() == (allowed ? preset : previous),
            "quick amounts are exact and do not silently clamp");
      check(!session.preset(1), "even rejected quick amount consumes its activation edge");
      neutral(session, frame);
    }
    check(session.adjust(INT32_MAX) == (maximum != 0) && session.quantity() == maximum,
          "positive adjustment saturates without wrap");
    check(session.adjust(INT32_MIN) == (maximum != 0) && session.quantity() == (maximum ? 1u : 0u),
          "negative adjustment saturates without wrap");
    check(session.confirm() == (maximum != 0), "quantity zero can never confirm");
    check(!session.take(snapshot, true), "confirmed intent still waits for release");
    neutral(session, frame);
    check(!session.take(snapshot, false),
          "native adapter must explicitly report a handoff boundary");
    const auto request = session.take(snapshot, true);
    check(bool(request) == (maximum != 0),
          "one native-independent request is available after release");
    if (request) {
      check(request->id == Id::meat && request->quantity == 1 && request->key == snapshot.key &&
                request->material_snapshot.target_count == 4 &&
                request->material_snapshot.materials[0].stored ==
                    snapshot.recipes[0].materials[0].stored,
            "request binds recipe amount station and owned material values");
      check(!session.take(snapshot, true) && session.phase() == Phase::dispatched,
            "taking consumes the request before any adapter effect");
      session.original(ubc::SelectionIssue::context_changed);
      check(!session.open(snapshot) && session.phase() == Phase::dispatched,
            "error and stale open cannot revive dispatched intent");
    }
  }
  for (const auto& descriptor : ubc::recipe_catalog::recipes) {
    auto snapshot = example();
    snapshot.key.kind = descriptor.station;
    available(snapshot, descriptor.id, 7);
    ubc::RecipeSession session;
    std::uint64_t frame = 0;
    session.open(snapshot);
    neutral(session, frame);
    if (session.selected()->id != descriptor.id) {
      session.select_recipe(descriptor.id);
      neutral(session, frame);
    }
    session.preset(7);
    neutral(session, frame);
    session.confirm();
    neutral(session, frame);
    const auto request = session.take(snapshot, true);
    check(request && request->id == descriptor.id && request->quantity == 7 &&
              request->material_snapshot.target_count == descriptor.targets.count,
          "all catalogue recipes produce independent value requests without original offer rows");
    if (request)
      for (std::size_t i = 0; i < request->material_snapshot.target_count; ++i) {
        const auto& row = request->material_snapshot.materials[i];
        const auto preview = ubc::recipe_amount_preview(row, request->quantity);
        check(
            preview.available_known && preview.missing == 0 && preview.output_shortfall == 0,
            "every target in each handed-off recipe retains a sufficient copied stock/reservation");
      }
  }
  {
    auto snapshot = example();
    auto& meat = state(snapshot, Id::meat);
    for (auto i = meat.target_count; i < meat.materials.size(); ++i)
      meat.materials[i] = {-99, -99, -99, UINT32_MAX, UINT32_MAX};
    ubc::RecipeSession session;
    check(session.open(snapshot) && session.maximum() == 23 &&
              session.selected()->target_count == 4,
          "unused target capacity is never validated as padded recipe ingredients");
  }
}

void refresh_contract() {
  for (bool pending : {false, true})
    for (unsigned change = 0; change < 8; ++change) {
      auto snapshot = example();
      ubc::RecipeSession session;
      std::uint64_t frame = 0;
      session.open(snapshot);
      neutral(session, frame);
      session.preset(7);
      neutral(session, frame);
      if (pending) {
        session.confirm();
        neutral(session, frame);
      }
      auto& meat = state(snapshot, Id::meat);
      switch (change) {
      case 0:
        ++meat.materials[0].stored;
        break;
      case 1:
        ++meat.materials[1].stored;
        break;
      case 2:
        meat.materials[0].reserved = 1;
        break;
      case 3:
        meat.maximum = 22;
        break;
      case 4:
        meat.contract = false;
        break;
      case 5:
        meat.hand_known = false;
        break;
      case 6:
        meat.material_known = false;
        break;
      case 7:
        available(snapshot, Id::molerat_dish, 1);
        break;
      }
      check(
          !session.refresh(snapshot) && session.phase() == Phase::selecting && !session.armed() &&
              session.quantity() == (session.maximum() ? 1u : 0u),
          "stock, knowledge, maximum and eligibility changes reset selected amount and confirmation");
      neutral(session, frame);
      check(!session.take(snapshot, true),
            "new neutral frames do not recreate invalidated confirmation");
    }
  {
    auto snapshot = example();
    available(snapshot, Id::molerat_dish, 2);
    ubc::RecipeSession session;
    std::uint64_t frame = 0;
    session.open(snapshot);
    neutral(session, frame);
    session.preset(7);
    neutral(session, frame);
    session.confirm();
    neutral(session, frame);
    std::swap(snapshot.recipes[0], snapshot.recipes.back());
    auto& meat = state(snapshot, Id::meat);
    std::swap(meat.materials[0], meat.materials[3]);
    check(
        session.refresh(snapshot) && session.selected()->id == Id::meat && session.quantity() == 7,
        "reordered recipe and material copies remap by identity without selecting a different item");
    auto request = session.take(snapshot, true);
    check(request && request->quantity == 7 && request->material_snapshot.materials[3].stored == 23,
          "request contains the exact fresh material mapping rather than borrowed memory");
    meat.materials[3].stored = 0;
    check(request && request->material_snapshot.materials[3].stored == 23,
          "caller mutations cannot change an already handed off value snapshot");
  }
  for (unsigned change = 0; change < 7; ++change) {
    auto snapshot = example();
    ubc::RecipeSession session;
    std::uint64_t frame = 0;
    session.open(snapshot);
    neutral(session, frame);
    session.confirm();
    neutral(session, frame);
    switch (change) {
    case 0:
      ++snapshot.key.generation;
      break;
    case 1:
      ++snapshot.key.thread;
      break;
    case 2:
      ++snapshot.key.world;
      break;
    case 3:
      ++snapshot.key.hero;
      break;
    case 4:
      ++snapshot.key.station;
      break;
    case 5:
      ++snapshot.key.manager;
      break;
    case 6:
      snapshot.key.kind = Kind::alchemy;
      break;
    }
    check(!session.refresh(snapshot) && session.phase() == Phase::original,
          "every station identity component binds the lifetime of a request");
    neutral(session, frame);
    check(!session.take(snapshot, true), "changed station cannot dispatch after release");
  }
}

void invalid_snapshots_and_input() {
  for (unsigned bad = 0; bad < 13; ++bad) {
    auto snapshot = example();
    auto& meat = state(snapshot, Id::meat);
    switch (bad) {
    case 0:
      snapshot.key.generation = 0;
      break;
    case 1:
      snapshot.key.hero = 0;
      break;
    case 2:
      snapshot.key.kind = static_cast<Kind>(99);
      break;
    case 3:
      meat.maximum = UINT32_MAX;
      break;
    case 4:
      meat.target_count = 0;
      break;
    case 5:
      meat.target_count = meat.materials.size() + 1;
      break;
    case 6:
      meat.materials[0].symbol = -1;
      break;
    case 7:
      meat.materials[1].symbol = meat.materials[0].symbol;
      break;
    case 8:
      meat.materials[0].unit_cost = 2;
      break;
    case 9:
      meat.materials[0].stored = -1;
      break;
    case 10:
      meat.materials[0].reserved = -1;
      break;
    case 11:
      ++meat.maximum;
      break;
    case 12:
      snapshot.recipes[1].id = Id::meat;
      break;
    }
    ubc::RecipeSession session;
    check(!session.open(snapshot) && session.phase() == Phase::original,
          "malformed key metadata material count and impossible maximum fail closed");
  }
  for (bool poll_first : {false, true}) {
    const auto snapshot = example();
    ubc::RecipeSession session;
    std::uint64_t frame = 0;
    session.open(snapshot);
    for (int i = 0; i < 4; ++i) {
      if (poll_first)
        session.observe(++frame, true, true);
      check(!session.confirm(), "held opener cannot manufacture in either event order");
      if (!poll_first)
        session.observe(++frame, true, true);
    }
    neutral(session, frame);
    if (poll_first)
      session.observe(++frame, true, true);
    check(session.confirm(), "fresh event works before or after its held poll");
    if (!poll_first)
      session.observe(++frame, true, true);
    check(!session.confirm() && !session.adjust(1), "confirmed amount cannot repeat or change");
    session.observe(++frame, false, true);
    for (int i = 0; i < 10; ++i)
      session.observe(frame, false, true);
    check(!session.take(snapshot, true), "repeated frame does not fake release");
    session.observe(++frame, false, false);
    neutral(session, frame);
    check(session.phase() == Phase::selecting && !session.take(snapshot, true) &&
              session.issue() == ubc::SelectionIssue::focus_lost,
          "focus loss permanently discards pending intent");
    check(session.confirm() && session.back(), "fresh confirm can be canceled even while held");
    check(!session.return_ready(), "cancel never passes held buttons to original handling");
    for (int i = 0; i < 3; ++i) {
      session.observe(++frame, false, true);
      session.original(ubc::SelectionIssue::context_changed);
    }
    check(session.return_ready() && !session.take(snapshot, true),
          "repeated cancellation cannot postpone neutral exit forever");
  }
  {
    const auto snapshot = example();
    ubc::RecipeSession session;
    std::uint64_t frame = 0;
    session.open(snapshot);
    neutral(session, frame);
    session.confirm();
    neutral(session, frame);
    check(session.phase() == Phase::ready,
          "request reaches ready before a separate focus-loss case");
    session.observe(++frame, false, false);
    neutral(session, frame);
    check(session.phase() == Phase::selecting && !session.take(snapshot, true),
          "focus loss after complete release also permanently discards the request");
  }
}
} // namespace

int main() {
  visibility_and_values();
  unavailable_and_preview();
  quantity_and_request();
  refresh_contract();
  invalid_snapshots_and_input();
  std::printf("RecipeSession: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
