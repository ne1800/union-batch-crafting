// SPDX-License-Identifier: MIT
// Our actual session and menu models with synthetic material and quantity
// evidence. No engine, key delivery, game world or crafting authorization.
#include "ubc/ui/crafting_ui_diagnostics.hpp"
#define UBC_CRAFTING_FIXTURE_ONLY
#include "crafting_catalog_tests.cpp"
#include <limits>
using namespace crafting_test;
namespace d = ubc::crafting_ui_diagnostics;
using Action = ubc::CraftingMenuAction;
using Row = ubc::CraftingMenuRow;
using Phase = ubc::SelectionPhase;

void record_bounds(const d::Batch& batch) {
  check(batch.count <= batch.records.size(), "diagnostic record count is bounded");
  for (std::size_t i = 0; i < batch.count; ++i) {
    const auto& r = batch.records[i];
    check(r.name && std::strlen(r.name) <= 64, "diagnostic event has bounded name");
    for (const char* c = r.name; *c; ++c)
      check((*c >= 'a' && *c <= 'z') || *c == '_',
            "diagnostic event obeys existing parser alphabet");
  }
}
int main() {
  {
    auto snapshot = example(Kind::grind, 11, 7);
    ubc::CraftingSession session;
    ubc::CraftingMenu menu;
    std::uint64_t frame = 0;
    check(session.open(snapshot), "grind fixture opens through actual session");
    neutral(session, frame);
    d::Trace trace;
    const auto epoch = trace.begin(1, 100);
    record_bounds(epoch);
    check(epoch.count == 1 && epoch.records[0].detail == 1 && epoch.records[0].quantity == 0,
          "epoch is exact low/high uint64 pair");
    for (unsigned i = 0; i < 12; ++i) {
      const auto a = d::dispatch(menu, session, 200, Action::up, true);
      check(!a.accepted && a.before.row == Row::recipe && a.after.row == Row::recipe &&
                session.armed(),
            "Up at first row neither advances nor consumes input");
    }
    const auto first = d::dispatch(menu, session, 28, Action::confirm, true);
    const auto logged = trace.input(1, first);
    record_bounds(logged);
    check(first.observed && first.accepted && first.before.armed && !first.after.armed &&
              first.after.row == Row::quantity,
          "first accepted Enter reaches quantity and explicitly latches");
    check(logged.count == 4 && logged.records[0].detail == 28 &&
              logged.records[0].quantity == (static_cast<int>(Action::confirm) | 256) &&
              logged.records[1].detail == d::flags(first.before, true) &&
              logged.records[1].quantity == d::flags(first.after, true),
          "native action result and both states retained");
    check(!d::dispatch(menu, session, 203, Action::left, true).accepted,
          "immediate Left is blocked by existing latch");
    for (unsigned i = 0; i < 2; ++i) {
      const auto late = d::dispatch(menu, session, 28, Action::confirm, true);
      check(
          !late.accepted && late.after.row == Row::quantity && late.after.quantity == 1 &&
              session.phase() == Phase::selecting,
          "same-frame later Enter is rejected and reproduces quantity-row mechanism without any effect");
    }
    session.observe(++frame, false, true);
    check(!session.armed(), "event clearing frame is not a neutral frame");
    session.observe(++frame, false, true);
    check(!session.armed(), "one neutral frame is insufficient");
    session.observe(frame, false, true);
    check(!session.armed(), "duplicate native frame cannot rearm");
    const auto before = d::state(menu, session);
    session.observe(++frame, false, true);
    const auto observed = d::state(menu, session);
    check(observed.armed && !observed.can_confirm,
          "second neutral frame rearms but does not invent fresh quantity proof");
    const auto* selected = session.selected();
    check(session.observe_quantity_check({selected->id, session.quantity(), session.preview().key,
                                          *selected, session.preview().hands},
                                         frame, true),
          "separate synthetic chosen-quantity proof admitted only in fresh frame");
    const d::Frame f{before, observed, d::state(menu, session), true, false, 1000, 1001,
                     1601,   1801};
    const auto polled = trace.frame(1, f);
    record_bounds(polled);
    check(polled.count == 4 && polled.records[0].detail == d::flags(before, true, false, true) &&
              polled.records[0].quantity == d::flags(observed, true, false, true) &&
              polled.records[2].detail == 600 && polled.records[2].quantity == 200 &&
              polled.records[3].detail == 900,
          "poll/rearm and existing capture/quantity timings remain separate");
    check(d::dispatch(menu, session, 28, Action::confirm, true).accepted &&
              menu.row() == Row::manufacture,
          "fresh second Enter reaches manufacture");
    check(!d::dispatch(menu, session, 28, Action::confirm, true).accepted &&
              session.phase() == Phase::selecting,
          "same-frame manufacture Enter remains blocked");
    neutral(session, frame);
    check(d::dispatch(menu, session, 28, Action::confirm, true).accepted &&
              session.phase() == Phase::releasing,
          "third fresh Enter only creates original pending intent");
    check(!d::dispatch(menu, session, 28, Action::confirm, true).accepted &&
              session.phase() == Phase::releasing,
          "no repeated pending effect from diagnostic dispatch");
  }
  {
    auto snapshot = example(Kind::grind, 11, 7);
    ubc::CraftingSession session;
    ubc::CraftingMenu menu;
    std::uint64_t frame = 0;
    session.open(snapshot);
    neutral(session, frame);
    d::dispatch(menu, session, 28, Action::confirm, true);
    neutral(session, frame);
    const auto left = d::dispatch(menu, session, 203, Action::left, true);
    check(left.accepted && left.before.quantity == 1 && left.after.quantity == 1 &&
              left.before.can_confirm && !left.after.can_confirm,
          "accepted Left clamp leaves amount unchanged but invalidates current quantity proof");
    const auto failed = d::dispatch(menu, session, 28, Action::confirm, true);
    check(!failed.accepted && failed.before.armed && !failed.before.can_confirm &&
              !failed.after.armed && failed.after.row == Row::quantity,
          "proof rejection differs from latch rejection without weakening either");
    neutral(session, frame);
    check(d::dispatch(menu, session, 28, Action::confirm, true).accepted,
          "fresh proof after two neutral frames allows normal continuation");
  }
  {
    auto snapshot = example(Kind::grind, 11, 7);
    ubc::CraftingSession session;
    ubc::CraftingMenu menu;
    std::uint64_t frame = 0;
    session.open(snapshot);
    neutral(session, frame);
    for (unsigned i = 0; i < 10; ++i)
      check(session.refresh(snapshot) && session.armed(),
            "identical per-frame refresh does not reset latch");
    const auto unknown = d::dispatch(menu, session, -1, Action::none, true);
    check(!unknown.accepted && session.armed() && unknown.key == -1,
          "unknown action is visible but cannot change gates");
    const auto focus = d::dispatch(menu, session, 28, Action::none, false);
    check(focus.observed && !focus.focused && !focus.accepted && !focus.after.armed &&
              focus.after.row == Row::recipe,
          "real existing focus-loss branch disarms without interpreting key");
    neutral(session, frame);
    check(session.armed(), "fresh focus and neutral frames restore existing input admission");
    const auto id = session.selected()->id;
    ++state(snapshot, id).materials[0].stored;
    check(!session.refresh(snapshot) && !session.armed() &&
              session.issue() == ubc::SelectionIssue::stock_changed,
          "actual state change still resets selection latch");
  }
  {
    d::Trace trace;
    d::Action a;
    a.observed = true;
    a.focused = true;
    a.key = 28;
    a.action = Action::confirm;
    check(trace.input(1, a).count == 0 && trace.frame(1, {}).count == 0,
          "unopened epoch records nothing");
    trace.begin(1, 10);
    for (unsigned i = 0; i < d::Trace::max_actions; ++i)
      check(trace.input(1, a).count == 4, "all bounded action samples retained");
    const auto limit = trace.input(1, a);
    check(limit.count == 1 && limit.records[0].detail == 1 && limit.records[0].quantity == 64,
          "input limit explicitly reported once");
    for (unsigned i = 0; i < 1000; ++i)
      check(trace.input(1, a).count == 0, "exhausted action budget never floods log");
    check(trace.begin(1, 0).count == 0 && trace.input(1, a).count == 0,
          "same epoch cannot reset budget");
    for (unsigned i = 0; i < d::Trace::max_frames; ++i)
      check(trace.frame(1, {}).count == 4, "frame budget independent of action budget");
    const auto frame_limit = trace.frame(1, {});
    check(frame_limit.count == 1 && frame_limit.records[0].detail == 2,
          "frame limit explicitly reported");
    check(trace.frame(1, {}).count == 0, "frame truncation reported only once");
    trace.end();
    check(trace.input(1, a).count == 0 && trace.begin(1, 0).count == 0,
          "closed epoch cannot be replayed");
    const auto fresh = trace.begin(UINT64_C(0x100000002), 100);
    check(fresh.count == 1 && fresh.records[0].detail == 2 && fresh.records[0].quantity == 1,
          "next epoch preserves full generation");
    check(trace.input(1, a).count == 0 && trace.frame(1, {}).count == 0,
          "foreign old generation cannot consume current budget");
    check(trace.input(UINT64_C(0x100000002), a).count == 4,
          "fresh epoch has its own admission budget");
    a.observed = false;
    check(trace.input(UINT64_C(0x100000002), a).count == 0,
          "notices lacking selection event are never misreported");
  }
  {
    d::Trace trace;
    d::Action a;
    a.observed = true;
    unsigned count = 0, global_limits = 0;
    const auto collect = [&](const d::Batch& batch) {
      record_bounds(batch);
      count += static_cast<unsigned>(batch.count);
      for (std::size_t i = 0; i < batch.count; ++i)
        if (std::strcmp(batch.records[i].name, "ui_diag_limit") == 0 &&
            batch.records[i].detail == 3)
          ++global_limits;
    };
    for (std::uint64_t e = 1; e <= 100; ++e) {
      collect(trace.begin(e, 0));
      for (unsigned i = 0; i < 100; ++i) {
        collect(trace.input(e, a));
        collect(trace.frame(e, {}));
      }
    }
    check(count == trace.records() && count <= d::Trace::max_records && global_limits == 1,
          "process budget is finite across arbitrarily many UI epochs");
    check(d::milliseconds(100, 99) == -1 && d::milliseconds(1, 1) == 0 &&
              d::milliseconds(1, UINT64_MAX) == INT32_MAX,
          "invalid/backward and oversized clocks have explicit bounded signed values");
    check(d::word(UINT32_MAX) == -1 && d::word(0x80000000u) == INT32_MIN &&
              d::word(INT32_MAX) == INT32_MAX,
          "unsigned generation recipe and quantity words survive signed log fields");
  }
  std::printf("crafting_ui_diagnostics: %u checks, %u failures\n", checks, failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
