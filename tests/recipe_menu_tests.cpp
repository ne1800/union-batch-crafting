// SPDX-License-Identifier: MIT
#include "ubc/ui/recipe_menu.hpp"

#include <cstdio>

namespace {
using A = ubc::RecipeMenuAction;
using R = ubc::RecipeMenuRow;
using P = ubc::SelectionPhase;
using Id = ubc::recipe_catalog::RecipeId;
unsigned checks = 0, failures = 0;
void check(bool ok, const char* message) {
  ++checks;
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}
ubc::CatalogueSnapshot example(std::uint32_t maximum = 23, bool multiple = false) {
  ubc::CatalogueSnapshot result;
  result.key = {1,2,3,4,5,6,ubc::recipe_catalog::Station::stove};
  for (auto& entry : result.recipes) {
    if (entry.id != Id::meat && (!multiple || entry.id != Id::molerat_dish)) continue;
    entry.known = entry.contract = entry.hand_known = entry.material_known = true;
    entry.maximum = entry.id == Id::meat ? maximum : 2;
    for (std::size_t i = 0; i < entry.target_count; ++i) {
      entry.materials[i].stored = static_cast<std::int32_t>(entry.materials[i].unit_cost * entry.maximum);
      entry.materials[i].reserved = 0;
    }
  }
  return result;
}
void neutral(ubc::RecipeSession& session, std::uint64_t& frame) {
  for (int i = 0; i < 3; ++i) session.observe(++frame, false, true);
}
}

int main() {
  for (bool poll_first : {false,true}) {
    ubc::RecipeMenu menu; ubc::RecipeSession session; std::uint64_t frame = 0;
    const auto snapshot = example(); session.open(snapshot);
    check(menu.row() == R::recipe && !menu.on_action(A::confirm, session), "recipe focus ignores held opening");
    neutral(session, frame);
    for (auto destination : {R::quantity,R::manufacture}) {
      if (poll_first) session.observe(++frame, true, true);
      check(menu.on_action(A::confirm, session) && menu.row() == destination,
            "fresh A changes focus in both poll/event orders");
      if (!poll_first) session.observe(++frame, true, true);
      check(session.phase() == P::selecting && !menu.on_action(A::confirm, session),
            "one focus press never manufactures or advances twice");
      neutral(session, frame);
    }
    check(menu.on_action(A::confirm, session) && session.phase() == P::releasing,
          "third fresh A on manufacture creates intent");
    check(!menu.on_action(A::right, session) && menu.on_action(A::back, session),
          "pending intent freezes changes but accepts cancellation");
    neutral(session, frame);
    check(session.return_ready() && !session.take(snapshot, true), "cancellation retains original input release gate");
  }

  {
    auto snapshot = example(23, true); ubc::RecipeMenu menu; ubc::RecipeSession session; std::uint64_t frame = 0;
    session.open(snapshot); neutral(session, frame); session.preset(23); neutral(session, frame);
    check(menu.on_action(A::down, session) && menu.row() == R::recipe &&
          session.selected()->id == Id::molerat_dish && session.quantity() == 1,
          "recipe-list down switches recipe without carrying max amount or entering quantity");
    check(!menu.on_action(A::confirm, session), "same held input cannot confirm new recipe");
    neutral(session, frame);
    check(!menu.on_action(A::down, session) && menu.on_action(A::up, session) && session.selected()->id == Id::meat,
          "recipe navigation clamps and returns through only known station recipes");
    neutral(session, frame); menu.on_action(A::right, session);
    check(menu.row() == R::quantity && menu.on_action(A::right, session) && session.quantity() == 2,
          "same horizontal input changes quantity only in the quantity row");
    check(menu.on_action(A::back, session) && menu.row() == R::recipe && session.phase() == P::selecting,
          "B in a subrow only returns to recipes");
    check(!menu.on_action(A::back, session), "held B cannot also leave the recipe list");
    neutral(session, frame);
    check(menu.on_action(A::back, session) && session.phase() == P::original, "fresh B from recipes returns original handling");
  }

  for (const auto maximum : {0u,1u,4u,5u,9u,10u,23u}) {
    ubc::RecipeMenu menu; ubc::RecipeSession session; std::uint64_t frame = 0;
    auto snapshot = example(maximum); session.open(snapshot); neutral(session, frame);
    for (unsigned i = 0; i < 4; ++i) {
      const auto amount = menu.preset_value(i, maximum);
      check(menu.preset_enabled(i, maximum) == (amount && amount <= maximum), "quick button availability is exact");
    }
    check(!menu.preset_enabled(4, maximum), "unknown quick button is disabled");
    if (!maximum) {
      menu.on_action(A::next, session);
      check(menu.row() == R::original && session.visible_count() == 1 && session.quantity() == 0,
            "known zero-stock recipe stays visible while amount actions are skipped");
      menu.on_action(A::up, session);
      check(menu.row() == R::recipe && !menu.on_action(A::confirm, session), "A on zero maximum cannot leave recipe focus");
      continue;
    }
    menu.on_action(A::right, session);
    check(menu.row() == R::quantity, "quantity follows recipe row");
    menu.on_action(A::page_up, session); menu.on_action(A::page_down, session);
    check(session.quantity() == 1, "plus/minus five obey low bounds");
    menu.on_action(A::down, session);
    const auto previous = session.quantity(); menu.on_action(A::right, session);
    check(menu.row() == R::presets && menu.preset_enabled(menu.column(), maximum) && session.quantity() == previous,
          "focusing a quick button skips unavailable choices and does not activate it");
    check(menu.on_action(A::confirm, session) && session.quantity() == menu.preset_value(menu.column(), maximum),
          "fresh A applies exactly the focused quick amount");
    check(!menu.on_action(A::confirm, session), "quick button cannot repeat from a held A");
    neutral(session, frame); menu.on_action(A::down, session);
    check(menu.row() == R::steps && menu.column() == 0, "minus-five and plus-five have their own action row");
    const auto before = session.quantity(); session.observe(++frame, true, true);
    check(menu.on_action(A::confirm, session) && session.quantity() == (before > 5 ? before - 5 : 1),
          "step button applies once after a held poll");
    check(!menu.on_action(A::confirm, session), "held step button does not repeat");
    neutral(session, frame); menu.on_action(A::right, session);
    check(menu.column() == 1, "controller directions reach plus-five");
    menu.on_action(A::down, session);
    check(menu.row() == R::manufacture, "valid manufacture remains direction-accessible");
  }

  {
    ubc::RecipeMenu menu; ubc::RecipeSession session; std::uint64_t frame = 0;
    auto snapshot = example(); session.open(snapshot); neutral(session, frame);
    const R rows[] = {R::quantity,R::presets,R::presets,R::presets,R::presets,R::steps,R::steps,R::manufacture,R::original,R::recipe};
    const unsigned columns[] = {0,0,1,2,3,0,1,0,0,0};
    for (unsigned i = 0; i < 10; ++i)
      check(menu.on_action(A::next, session) && menu.row() == rows[i] && menu.column() == columns[i],
            "Tab visits every available control and wraps");
    check(menu.on_action(A::previous, session) && menu.row() == R::original,
          "Shift-Tab reverses the same focus order");
    menu.reset(); check(menu.row() == R::recipe && menu.column() == 0, "fresh UI resets focus positions");
    menu.on_action(A::right, session); menu.on_action(A::down, session); menu.on_action(A::right, session);
    snapshot.recipes[0].maximum = 4; session.refresh(snapshot); neutral(session, frame);
    check(!menu.on_action(A::confirm, session) && session.quantity() == 1,
          "stale focused five is rejected rather than silently becoming four");
    check(!menu.on_action(A::confirm, session), "disabled stale action consumes its edge");
    neutral(session, frame); session.observe(++frame, false, false);
    check(!menu.on_action(A::down, session) && !menu.on_action(A::confirm, session),
          "focus loss blocks both navigation and activation");
  }
  {
    auto snapshot = example(); snapshot.recipes[0].contract = false;
    ubc::RecipeMenu menu; ubc::RecipeSession session; std::uint64_t frame = 0;
    session.open(snapshot); neutral(session, frame);
    for (unsigned i = 0; i < 20; ++i) {
      menu.on_action(A::next, session);
      check(menu.row() != R::manufacture, "unverified recipe contract permits preview but never focuses manufacture");
    }
    snapshot.recipes[0].known = false; ++snapshot.key.generation;
    check(session.open(snapshot) && session.visible_count() == 0, "empty list opens without invented native offers");
    menu.reset(); neutral(session, frame);
    check(menu.on_action(A::next, session) && menu.row() == R::original && menu.on_action(A::confirm, session),
          "empty catalogue still exposes original handling");
  }
  std::printf("RecipeMenu: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
