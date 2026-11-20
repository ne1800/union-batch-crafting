// SPDX-License-Identifier: MIT
// Our synthetic menu values. No engine or physical input.
#include "ubc/stations/grind/grind_ui_readiness.hpp"
#define UBC_CRAFTING_FIXTURE_ONLY
#include "crafting_catalog_tests.cpp"
using namespace crafting_test;
namespace r = ubc::grind_ui_readiness;
namespace d = ubc::crafting_ui_diagnostics;
using Action = ubc::CraftingMenuAction;
using Row = ubc::CraftingMenuRow;
using Phase = ubc::SelectionPhase;

r::Render render(std::uint64_t frame = 2) {
  r::Render v;
  v.epoch = 1;
  v.frame = frame;
  v.before = {Row::recipe,
              0,
              Phase::selecting,
              ubc::SelectionIssue::none,
              ubc::CraftingBlockReason::none,
              true,
              true,
              0x900b,
              1,
              11};
  v.after = v.before;
  v.quantity = {true, 1, frame, 0x900b, 1};
  v.outer_quiet = v.context_bound = v.rendered = v.owns_input = v.focused = v.poll_known = true;
  return v;
}
d::Action input(int key = 28, Action action = Action::confirm, bool accepted = true) {
  d::Action a;
  a.observed = a.focused = true;
  a.key = key;
  a.action = action;
  a.accepted = accepted;
  return a;
}
bool named(const r::Batch& b, const char* name) {
  return b.count && std::strcmp(b.records[0].name, name) == 0;
}
void positive(const r::Batch& b) {
  check(b.count == 5, "complete five-record state");
  const char* names[] = {"grind_ui_ready_epoch", "grind_ui_ready_state", "grind_ui_ready_quantity",
                         "grind_ui_ready_input", "grind_ui_ready_input_frame"};
  for (std::size_t i = 0; i < b.count; ++i)
    check(std::strcmp(b.records[i].name, names[i]) == 0, "fixed group order");
}
void rejected(const r::Batch& b, r::Issue issue) {
  check(named(b, "grind_ui_ready_revoked") && b.count == 1 &&
            b.records[0].detail == static_cast<int>(issue),
        "explicit reason on revoke");
}
int main() {
  {
    r::Trace trace;
    check(trace.begin(1, 1), "bow menu epoch");
    auto bow = render();
    bow.before.recipe = bow.after.recipe = bow.quantity.recipe = 0x8001;
    const auto records = trace.after_render(bow);
    check(records.count == 5 && records.records[1].quantity == 0x8001,
          "bow recipe bound to actual rendered state");
  }
  {
    r::Trace t;
    check(t.after_render(render()).count == 0, "closed observer emits nothing");
    check(t.begin(1, 1), "new epoch");
    const auto b = t.after_render(render());
    positive(b);
    check(b.records[0].detail == 1 && b.records[0].quantity == 0 &&
              b.records[1].detail == 5243680 && b.records[1].quantity == 36875 &&
              b.records[2].detail == 1 && b.records[2].quantity == 11 && b.records[3].detail == 0 &&
              b.records[3].quantity == 0 && b.records[4].detail == 0,
          "literal initial wire fixture");
    for (unsigned frame = 3; frame < 1003; ++frame)
      check(t.after_render(render(frame)).count == 0, "stable frames produce no records");
    check(t.groups() == 1 && t.records() == 5, "sparse output does not inherit 96-frame cap");
    rejected(t.close(), r::Issue::closed);
    check(t.close().count == 0, "one close revoke");
  }
  {
    for (unsigned which = 0; which < 19; ++which) {
      r::Trace t;
      t.begin(1, 1);
      t.after_render(render());
      auto v = render(3);
      auto want = r::Issue::state;
      switch (which) {
      case 0:
        v.epoch = 2;
        want = r::Issue::epoch;
        break;
      case 1:
        v.frame = 2;
        want = r::Issue::frame;
        break;
      case 2:
        v.outer_quiet = false;
        want = r::Issue::reentrant;
        break;
      case 3:
        v.context_bound = false;
        want = r::Issue::binding;
        break;
      case 4:
        v.owns_input = false;
        want = r::Issue::binding;
        break;
      case 5:
        v.rendered = false;
        want = r::Issue::render;
        break;
      case 6:
        v.focused = false;
        want = r::Issue::focus;
        break;
      case 7:
        v.poll_known = false;
        want = r::Issue::focus;
        break;
      case 8:
        v.input_serial_before = 1;
        want = r::Issue::reentrant;
        break;
      case 9:
        ++v.after.quantity;
        want = r::Issue::reentrant;
        break;
      case 10:
        v.before.column = v.after.column = 1;
        break;
      case 11:
        v.before.recipe = v.after.recipe = 8;
        break;
      case 12:
        v.before.maximum = v.after.maximum = UINT32_MAX;
        break;
      case 13:
        v.quantity.observed = false;
        want = r::Issue::quantity;
        break;
      case 14:
        v.quantity.epoch = 2;
        want = r::Issue::quantity;
        break;
      case 15:
        v.quantity.frame = 2;
        want = r::Issue::quantity;
        break;
      case 16:
        v.quantity.recipe = 0x9001;
        want = r::Issue::quantity;
        break;
      case 17:
        v.quantity.quantity = 2;
        want = r::Issue::quantity;
        break;
      case 18:
        v.held = true;
        break;
      }
      rejected(t.after_render(v), want);
      check(t.after_render(render(4)).count == 0, "no revival in failed epoch");
    }
  }
  {
    r::Trace t;
    t.begin(1, 1);
    t.after_render(render());
    t.observe_input(1, 3, input());
    auto v = render(3);
    v.input_serial_before = 1;
    v.before.row = v.after.row = Row::quantity;
    v.before.armed = v.after.armed = false;
    check(t.after_render(v).count == 0, "same outer frame after accepted input cannot publish");
    v.frame = v.quantity.frame = 4;
    auto b = t.after_render(v);
    positive(b);
    check(b.records[3].detail == 1 && b.records[3].quantity == 28 && b.records[4].detail == 3 &&
              (b.records[1].detail & (1 << 27)) && !(b.records[1].detail & (1 << 8)),
          "actual accepted input and unarmed wait are separate");
    v.frame = v.quantity.frame = 5;
    check(t.after_render(v).count == 0, "unchanged wait state is sparse");
    v.frame = v.quantity.frame = 6;
    v.before.armed = v.after.armed = true;
    positive(t.after_render(v));
    t.observe_input(1, 7, input(205, Action::right));
    v.input_serial_before = 2;
    v.frame = v.quantity.frame = 7;
    v.before.quantity = v.after.quantity = v.quantity.quantity = 2;
    check(t.after_render(v).count == 0, "quantity adjust retaining armed still needs later frame");
    v.frame = v.quantity.frame = 8;
    positive(t.after_render(v));
  }
  {
    r::Trace t;
    t.begin(1, 1);
    auto v = render();
    v.before.can_confirm = v.after.can_confirm = false;
    v.before.reason = v.after.reason = ubc::CraftingBlockReason::unavailable;
    auto b = t.after_render(v);
    positive(b);
    check(!(b.records[1].detail & (1 << 9)) && (b.records[1].detail & (1 << 8)),
          "negative evaluated quantity leaves navigation armed but manufacturing blocked");
    v = render(3);
    positive(t.after_render(v));
    auto a = input(28, Action::confirm, false);
    t.observe_input(1, 4, a);
    v = render(5);
    v.input_serial_before = 1;
    b = t.after_render(v);
    positive(b);
    check(!(b.records[1].detail & (1 << 27)) && b.records[3].detail == 1,
          "rejected input is counted and never relabelled accepted");
    t.observe_input(1, 6, input(-1, Action::none, false));
    v = render(7);
    v.input_serial_before = 2;
    b = t.after_render(v);
    positive(b);
    check(b.records[3].quantity == -1, "unknown raw key is not dropped");
  }
  {
    for (unsigned which = 0; which < 4; ++which) {
      r::Trace t;
      t.begin(1, 1);
      t.after_render(render());
      auto a = input();
      if (which == 0)
        a.observed = false;
      if (which == 1)
        a.action = static_cast<Action>(11);
      if (which == 2)
        a.focused = false;
      if (which == 3)
        a.action = Action::none;
      t.observe_input(1, 3, a);
      rejected(t.after_render(render(4)), r::Issue::input);
    }
    r::Trace t;
    t.begin(1, 1);
    t.after_render(render());
    t.observe_input(2, 3, input());
    rejected(t.after_render(render(4)), r::Issue::epoch);
    r::Trace old;
    old.begin(1, 1);
    old.after_render(render());
    old.observe_input(1, 1, input());
    rejected(old.after_render(render(3)), r::Issue::frame);
  }
  {
    r::Trace t;
    t.begin(1, 1);
    for (unsigned i = 0; i < r::Trace::max_inputs; ++i)
      t.observe_input(1, 2, input());
    auto v = render(3);
    v.input_serial_before = 64;
    positive(t.after_render(v));
    t.observe_input(1, 4, input());
    auto b = t.after_render(render(5));
    check(named(b, "grind_ui_ready_limit") && b.records[0].detail == 1 &&
              b.records[0].quantity == 64,
          "input cap includes all callbacks");
    for (unsigned i = 0; i < 1000; ++i)
      check(t.after_render(render(6 + i)).count == 0, "input limit cannot flood or reset");
    r::Trace g;
    g.begin(1, 1);
    for (unsigned i = 0; i < r::Trace::max_groups; ++i) {
      v = render(2 + i);
      v.before.armed = v.after.armed = (i % 2) == 0;
      positive(g.after_render(v));
    }
    v = render(130);
    auto b2 = g.after_render(v);
    check(named(b2, "grind_ui_ready_limit") && b2.records[0].detail == 2,
          "group cap before extra group");
  }
  {
    r::Trace t;
    unsigned count = 0, limits = 0;
    for (std::uint64_t epoch = 1; epoch <= 10; ++epoch) {
      if (!t.begin(epoch, epoch * 1000))
        break;
      for (unsigned i = 0; i < 128; ++i) {
        auto v = render(epoch * 1000 + 1 + i);
        v.epoch = v.quantity.epoch = epoch;
        v.before.armed = v.after.armed = (i % 2) == 0;
        const auto b = t.after_render(v);
        count += static_cast<unsigned>(b.count);
        if (named(b, "grind_ui_ready_limit") && b.records[0].detail == 3)
          ++limits;
      }
      const auto b = t.close();
      count += static_cast<unsigned>(b.count);
      if (named(b, "grind_ui_ready_limit") && b.records[0].detail == 3)
        ++limits;
    }
    check(limits == 1 && count == t.records() && count <= 2048,
          "lifetime records bounded with one terminal marker");
  }
  {
    r::Trace t;
    check(t.begin(1, 1), "first epoch before monotonic tests");
    t.after_render(render());
    t.close();
    check(!t.begin(1, 3), "old epoch cannot be reopened");
    check(!t.begin(2, 2), "new epoch cannot reset outer frame");
    check(t.begin(2, 3), "later genuine epoch retains global frame monotonicity");
    auto v = render(4);
    v.epoch = v.quantity.epoch = 2;
    positive(t.after_render(v));
    check(!t.begin(3, 5), "active epoch cannot be silently replaced");
    rejected(t.after_render(v), r::Issue::epoch);
  }
  {
    auto snapshot = example(Kind::grind, 11, 7);
    ubc::CraftingSession session;
    ubc::CraftingMenu menu;
    check(session.open(snapshot), "real model opens");
    r::Trace t;
    t.begin(1, 1);
    for (std::uint64_t frame = 2; frame <= 3; ++frame) {
      session.observe(frame, false, true);
      const auto* s = session.selected();
      check(session.observe_quantity_check(
                {s->id, session.quantity(), session.preview().key, *s, session.preview().hands},
                frame, true),
            "real frame quantity proof");
      auto v = render(frame);
      v.before = v.after = r::state(menu, session);
      v.quantity.recipe = v.after.recipe;
      positive(t.after_render(v));
    }
    const auto a = d::dispatch(menu, session, 28, Action::confirm, true);
    t.observe_input(1, 3, a);
    check(a.accepted && menu.row() == Row::quantity && !session.armed(),
          "actual Return latches input");
    session.observe(4, false, true);
    const auto* s = session.selected();
    session.observe_quantity_check(
        {s->id, session.quantity(), session.preview().key, *s, session.preview().hands}, 4, true);
    auto v = render(4);
    v.input_serial_before = 1;
    v.before = v.after = r::state(menu, session);
    v.quantity.recipe = v.after.recipe;
    auto b = t.after_render(v);
    positive(b);
    check(!(b.records[1].detail & (1 << 8)) && (b.records[1].detail & (1 << 9)),
          "real can_confirm true is not a readiness token while input remains latched");
  }
  std::printf("grind_ui_readiness: %u checks, %u failures\n", checks, failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
