// SPDX-License-Identifier: MIT
#include "ubc/ui/batch_menu.hpp"

#include <cstdio>

namespace {
unsigned checks = 0, failures = 0;
void check(bool ok, const char* message) {
  ++checks;
  if (!ok) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", message);
  }
}
ubc::BatchSnapshot example(std::uint32_t maximum = 23) {
  ubc::BatchSnapshot s;
  s.amount_contract = true;
  s.maximum = maximum;
  s.selection.selection_contract = s.selection.stock_at_rest = true;
  s.selection.key = {1, 2, 3, 4, 5, 6, 7, ubc::SelectionMode::info, -1, 8};
  s.selection.stored[0] = static_cast<std::int32_t>(maximum);
  s.selection.count = 1;
  s.selection.offers[0] = {110, 210, 4, 1};
  return s;
}
void neutral(ubc::BatchSession& s, std::uint64_t& frame) {
  for (int i = 0; i < 3; ++i)
    s.observe(++frame, false, true);
}
}

int main() {
  using A = ubc::BatchMenuAction;
  using R = ubc::BatchMenuRow;
  using P = ubc::SelectionPhase;
  for (bool poll_first : {false, true}) {
    ubc::BatchMenu menu;
    ubc::BatchSession s;
    std::uint64_t frame = 0;
    const auto snap = example();
    s.open(snap);
    check(menu.row() == R::recipe, "opening focuses recipe");
    check(!menu.on_action(A::confirm, s) && menu.row() == R::recipe,
          "held opening cannot advance focus");
    neutral(s, frame);
    for (const auto destination : {R::quantity, R::manufacture}) {
      if (poll_first)
        s.observe(++frame, true, true);
      check(menu.on_action(A::confirm, s) && menu.row() == destination,
            "confirm changes focus in both poll/event orders");
      if (!poll_first)
        s.observe(++frame, true, true);
      check(s.phase() == P::selecting && !menu.on_action(A::confirm, s),
            "focus change never manufactures or repeats");
      neutral(s, frame);
    }
    if (poll_first)
      s.observe(++frame, true, true);
    check(menu.on_action(A::confirm, s) && s.phase() == P::releasing,
          "only a new action on manufacture confirms");
    if (!poll_first)
      s.observe(++frame, true, true);
    check(!menu.on_action(A::right, s), "directions cannot alter a confirmed request");
    check(menu.on_action(A::back, s) && s.phase() == P::original,
          "back during release cancels directly");
    neutral(s, frame);
    check(!s.take(snap, true) && s.return_ready(), "canceled manufacture waits for release");
  }

  for (const auto maximum : {0u, 1u, 4u, 5u, 9u, 10u, 23u}) {
    ubc::BatchMenu menu;
    ubc::BatchSession s;
    std::uint64_t frame = 0;
    const auto snap = example(maximum);
    s.open(snap);
    neutral(s, frame);
    for (unsigned i = 0; i < 4; ++i) {
      const auto amount = menu.preset_value(i, maximum);
      check(menu.preset_enabled(i, maximum) == (amount && amount <= maximum),
            "preset availability remains exact at small maxima");
    }
    check(menu.preset_value(4, maximum) == 0 && !menu.preset_enabled(4, maximum),
          "out-of-range preset is disabled");
    menu.on_action(A::down, s);
    if (!maximum) {
      check(menu.row() == R::original && s.quantity() == 0,
            "zero maximum skips disabled manufacturing controls");
      menu.on_action(A::up, s);
      check(menu.row() == R::recipe && !menu.on_action(A::confirm, s),
            "known unavailable recipe stays visible and unexecutable");
      neutral(s, frame);
      check(menu.on_action(A::back, s) && s.phase() == P::original,
            "zero-stock view can return to original");
      continue;
    }
    check(menu.row() == R::quantity, "down focuses amount");
    for (unsigned i = 0; i < maximum + 2; ++i)
      menu.on_action(A::right, s);
    check(s.quantity() == maximum, "quantity arrows saturate without wrap");
    menu.on_action(A::page_down, s);
    check(s.quantity() == (maximum > 5 ? maximum - 5 : 1), "page down subtracts five");
    menu.on_action(A::page_up, s);
    check(s.quantity() == maximum, "page up adds five within maximum");
    menu.on_action(A::down, s);
    check(menu.row() == R::presets, "presets follow quantity");
    const auto before = s.quantity();
    menu.on_action(A::right, s);
    check(s.quantity() == before && s.phase() == P::selecting, "focusing preset never applies it");
    check(menu.preset_enabled(menu.column(), maximum),
          "horizontal navigation skips unavailable presets");
    check(menu.on_action(A::confirm, s) &&
              s.quantity() == menu.preset_value(menu.column(), maximum),
          "preset applies only through confirm");
    check(!menu.on_action(A::confirm, s) && s.phase() == P::selecting,
          "held preset does not repeat or produce");
    neutral(s, frame);
    menu.on_action(A::down, s);
    check(menu.row() == R::steps && menu.column() == 0, "separate minus-five plus-five row");
    const auto prior = s.quantity();
    s.observe(++frame, true, true);
    check(menu.on_action(A::confirm, s) && s.quantity() == (prior > 5 ? prior - 5 : 1),
          "step button consumes post-poll confirm once");
    check(!menu.on_action(A::confirm, s), "held step button never repeats");
    neutral(s, frame);
    menu.on_action(A::right, s);
    check(menu.column() == 1, "plus-five is direction-accessible");
    check(menu.on_action(A::back, s) && menu.row() == R::recipe && s.phase() == P::selecting,
          "back from subrow returns to recipe only");
    check(!menu.on_action(A::back, s), "held back cannot also leave recipe");
    neutral(s, frame);
    check(menu.on_action(A::back, s) && s.phase() == P::original,
          "second fresh back returns original handling");
  }

  {
    ubc::BatchMenu menu;
    ubc::BatchSession s;
    std::uint64_t frame = 0;
    s.open(example());
    neutral(s, frame);
    const R rows[] = {R::quantity, R::presets, R::presets,     R::presets,  R::presets,
                      R::steps,    R::steps,   R::manufacture, R::original, R::recipe};
    const unsigned columns[] = {0, 0, 1, 2, 3, 0, 1, 0, 0, 0};
    for (unsigned i = 0; i < 10; ++i) {
      check(menu.on_action(A::next, s) && menu.row() == rows[i] && menu.column() == columns[i],
            "tab reaches every control and wraps");
    }
    menu.on_action(A::previous, s);
    check(menu.row() == R::original, "shift tab reverses focus order");
    check(menu.on_action(A::confirm, s) && s.phase() == P::original,
          "explicit original action remains available");
    menu.reset();
    check(menu.row() == R::recipe && menu.column() == 0, "new menu resets focus");
  }
  {
    ubc::BatchMenu menu;
    ubc::BatchSession s;
    std::uint64_t frame = 0;
    auto snap = example();
    s.open(snap);
    neutral(s, frame);
    menu.on_action(A::down, s);
    menu.on_action(A::down, s);
    menu.on_action(A::right, s);
    check(menu.column() == 1, "focus five before stock loss");
    snap.maximum = 4;
    snap.selection.stored[0] = 4;
    s.refresh(snap);
    neutral(s, frame);
    check(!menu.on_action(A::confirm, s) && s.quantity() == 1,
          "disabled focused five does not silently become maximum");
    check(!menu.on_action(A::confirm, s), "disabled preset still consumes action edge");
    neutral(s, frame);
    s.observe(++frame, false, false);
    check(!menu.on_action(A::down, s) && !menu.on_action(A::confirm, s),
          "unfocused menu accepts neither focus nor action changes");
  }
  std::printf("BatchMenu: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
