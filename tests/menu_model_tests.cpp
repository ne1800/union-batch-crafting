#include "ubc/menu_model.hpp"
#include <cstdio>

namespace {
unsigned checks = 0, failures = 0;
void check(bool ok, const char* name) {
  ++checks;
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", name); }
}
void neutral(ubc::MenuModel& model) {
  // An Observe immediately after the event may belong to the event frame.
  for (int i = 0; i < 3; ++i) model.observe(false, false, true);
}
void choose_row(ubc::MenuModel& model, unsigned target) {
  for (int limit = 0; model.row() != target && limit < 7; ++limit)
    model.on_action(ubc::MenuAction::Down);
  check(model.row() == target, "select bounded row");
}
}
int main() {
  using ubc::MenuAction;
  ubc::MenuModel model;
  model.reset();
  check(model.quantity() == 1 && model.maximum() == 23 && model.row() == 0, "opening defaults");
  check(!model.armed(), "opening is disarmed");
  model.observe(true, false, true);
  for (int i = 0; i < 8; ++i) {
    check(!model.on_action(MenuAction::Confirm), "held opener ignored");
    model.observe(true, false, true);
  }
  check(model.confirmation_count() == 0 && !model.closing(), "opener has no effect");
  neutral(model);
  check(model.armed(), "release arms menu");
  model.on_action(MenuAction::Up);
  check(model.row() == 6, "up wraps");
  model.on_action(MenuAction::Down);
  check(model.row() == 0, "down wraps");
  for (int i = 0; i < 100; ++i) model.on_action(MenuAction::Left);
  check(model.quantity() == 1, "quantity lower bound");
  for (int i = 0; i < 100; ++i) model.on_action(MenuAction::Right);
  check(model.quantity() == 23, "quantity upper bound");
  constexpr unsigned amounts[] = {1, 5, 10, 23};
  for (unsigned row = 1; row <= 4; ++row) {
    choose_row(model, row);
    model.on_action(MenuAction::Confirm);
    check(model.quantity() == amounts[row - 1], "preset value");
    check(model.confirmation_count() == 0, "preset does not confirm");
    neutral(model);
  }
  choose_row(model, 5);
  model.on_action(MenuAction::Confirm);
  check(model.confirmation_count() == 1 && model.confirmed_quantity() == 23, "explicit confirmation records selection");
  for (int i = 0; i < 10; ++i) {
    model.observe(true, false, true);
    model.on_action(MenuAction::Confirm);
  }
  check(model.confirmation_count() == 1, "held confirm cannot repeat");
  neutral(model);
  model.on_action(MenuAction::Confirm);
  check(model.confirmation_count() == 2, "fresh confirm allowed");
  neutral(model);
  model.on_action(MenuAction::Back);
  check(model.closing() && !model.close_ready(), "back waits for release");
  for (int i = 0; i < 8; ++i) model.observe(false, true, true);
  check(!model.close_ready(), "held back cannot escape into underlying menu");
  neutral(model);
  check(model.close_ready(), "neutral frames finish close");

  model.reset(); neutral(model);
  choose_row(model, 6);
  model.on_action(MenuAction::Confirm);
  check(model.closing() && !model.close_ready(), "return row starts closing");
  model.observe(true, false, false);
  check(!model.focused() && !model.armed() && !model.closing() && !model.close_ready(), "focus loss cancels pending close");
  const auto old_quantity = model.quantity();
  check(!model.on_action(MenuAction::Right), "input ignored while unfocused");
  check(model.quantity() == old_quantity, "focus loss preserves quantity");
  model.observe(true, false, true);
  check(!model.on_action(MenuAction::Confirm), "focus regain with held key ignored");
  neutral(model);
  check(model.armed() && model.focused(), "focus regained after release");
  model.on_action(MenuAction::Confirm);
  model.observe(false, false, true); // Event frame
  check(!model.close_ready(), "event frame is not a release frame");
  model.observe(false, false, true);
  check(!model.close_ready(), "one neutral frame insufficient");
  model.observe(false, false, true);
  check(model.close_ready(), "two neutral frames sufficient");
  model.reset();
  check(model.confirmation_count() == 0 && model.quantity() == 1 && !model.close_ready(), "reset clears old session");

  // A display error must close safely even without a key event.
  model.observe(true, false, true);
  model.request_close();
  check(model.closing() && !model.close_ready() && !model.armed(), "system close can start before input is armed");
  for (int i = 0; i < 8; ++i) model.observe(true, false, true);
  check(!model.close_ready() && model.confirmation_count() == 0, "system close waits for held confirm without acting");
  model.observe(false, false, false);
  check(!model.closing() && !model.close_ready() && !model.focused(), "focus loss cancels system close progression");
  model.observe(true, false, true);
  model.request_close(); // A persistent display error requests closure again.
  model.observe(true, false, true);
  check(model.closing() && !model.close_ready(), "persistent failure can restart closing after focus recovery");
  model.observe(false, false, true);
  check(!model.close_ready(), "system close still needs two neutral frames");
  model.request_close(); // Repeated requests must not postpone release.
  model.observe(false, false, true);
  check(model.close_ready(), "system close has no synthetic escape event and request is idempotent");
  model.request_close();
  check(model.close_ready() && model.confirmation_count() == 0, "completed system close remains ready without actions");
  std::printf("MenuModel: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
