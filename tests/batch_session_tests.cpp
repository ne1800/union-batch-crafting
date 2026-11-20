// SPDX-License-Identifier: MIT
#include "ubc/batch_session.hpp"

#include <cstdio>
#include <utility>

namespace {
unsigned checks = 0, failures = 0;
void check(bool ok, const char* message) {
  ++checks;
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}
ubc::BatchSnapshot example(std::uint32_t maximum = 23, bool alchemy = false) {
  ubc::BatchSnapshot s;
  s.amount_contract = true;
  s.maximum = maximum;
  auto& o = s.selection;
  o.selection_contract = o.stock_at_rest = true;
  o.key = {1,2,3,4,5,6,7,ubc::SelectionMode::info,-1,8};
  o.stored = {23,2,22,50,40,6};
  o.offers[0] = {110,210,4,1};
  o.count = 1;
  if (alchemy) {
    o.key.mode = ubc::SelectionMode::choice; o.key.owner_instance = 110;
    o.offers[1] = {110,211,6,5}; o.count = 2; o.reserved[2] = 1;
  }
  return s;
}
void neutral(ubc::BatchSession& s, std::uint64_t& frame) {
  for (int i = 0; i < 3; ++i) s.observe(++frame, false, true);
}
}

int main() {
  using ubc::SelectionIssue;
  using ubc::SelectionPhase;
  for (const auto maximum : {0u,1u,4u,5u,9u,10u,23u,static_cast<unsigned>(INT32_MAX)}) {
    ubc::BatchSession s; std::uint64_t frame = 0;
    const auto snap = example(maximum);
    check(s.open(snap), "known zero and finite stock open a preview");
    check(s.maximum() == maximum && s.quantity() == (maximum ? 1u : 0u), "opening begins at one or disabled zero");
    neutral(s, frame);
    for (const auto preset : {0u,1u,5u,10u,maximum,UINT32_MAX}) {
      const auto before = s.quantity();
      const bool allowed = preset && preset <= maximum;
      check(s.preset(preset) == allowed, "only exact available presets are accepted");
      check(s.quantity() == (allowed ? preset : before), "invalid preset never clamps or mutates");
      check(s.phase() == SelectionPhase::selecting, "preset never makes a request");
      check(!s.preset(1), "even disabled preset consumes its action edge");
      neutral(s, frame);
    }
    check(s.adjust(INT32_MAX) == (maximum != 0), "large positive step is bounded");
    check(s.quantity() == maximum, "positive overflow cannot wrap");
    check(s.adjust(INT32_MIN) == (maximum != 0), "large negative step is bounded");
    check(s.quantity() == (maximum ? 1u : 0u), "negative overflow cannot wrap");
    s.adjust(5);
    check(s.quantity() == (maximum > 6 ? 6u : maximum), "five step saturates at maximum");
    s.adjust(-5);
    check(s.quantity() == (maximum ? 1u : 0u), "minus five saturates at one");
    s.adjust(1); s.adjust(-1);
    check(s.quantity() == (maximum ? 1u : 0u), "unit steps remain bounded");
    check(s.confirm() == (maximum != 0), "zero maximum can never confirm");
    neutral(s, frame);
    const auto request = s.take(snap, true);
    check(bool(request) == (maximum != 0), "zero preview cannot dispatch");
  }

  for (unsigned bad = 0; bad < 9; ++bad) {
    auto snap = example();
    switch (bad) {
      case 0: snap.amount_contract = false; break;
      case 1: snap.maximum = UINT32_MAX; break;
      case 2: snap.selection.selection_contract = false; break;
      case 3: snap.selection.stock_at_rest = false; break;
      case 4: snap.selection.key.hero = 0; break;
      case 5: snap.selection.count = 0; break;
      case 6: snap.selection.count = 3; break;
      case 7: snap.selection.offers[0].function = -1; break;
      case 8: snap.selection.stored[0] = -1; break;
    }
    ubc::BatchSession s;
    check(!s.open(snap) && s.phase() == SelectionPhase::original, "invalid amount or native contract falls back");
  }

  for (bool alchemy : {false, true}) for (std::uint32_t amount : {1u,2u,7u,23u}) {
    const auto snap = example(23, alchemy);
    ubc::BatchSession s; std::uint64_t frame = 0;
    s.open(snap); neutral(s, frame);
    s.preset(amount); neutral(s, frame);
    check(s.confirm(), "free amount confirms independently of native quantity");
    check(!s.adjust(1) && !s.preset(1), "confirmed amount is frozen");
    check(!s.take(snap, true), "confirmation waits for release");
    s.observe(++frame, false, true);
    for (int i = 0; i < 10; ++i) s.observe(frame, false, true);
    s.observe(++frame, false, true);
    check(!s.take(snap, true), "event frame and repeated frame never fake release");
    s.observe(++frame, false, true);
    check(!s.take(snap, false), "UI must close before handoff");
    auto remapped = snap;
    remapped.selection.offers[0].row = 8;
    if (alchemy) {
      remapped.selection.offers[1].row = 9;
      std::swap(remapped.selection.offers[0], remapped.selection.offers[1]);
    }
    const auto request = s.take(remapped, true);
    const bool multi = alchemy && amount > 1;
    check(request && request->quantity == amount && request->original.function == (multi ? 211 : 210),
          "roles remap by identity without nearest-amount inference");
    check(request && request->original.quantity == (multi ? 5u : 1u) &&
          request->original.row == (multi ? 9 : 8), "request retains unmodified native quantity and fresh row");
    check(!s.take(remapped, true) && s.phase() == SelectionPhase::dispatched, "take consumes once before native effect");
    s.original(SelectionIssue::context_changed);
    check(!s.open(snap) && s.phase() == SelectionPhase::dispatched, "stale open and error cannot revive dispatched ticket");
  }
  {
    // The native quantity-5 path is also a valid explicit adapter path for a
    // request of 2 at a maximum of 2. Its original quantity stays unchanged.
    const auto snap = example(2, true);
    ubc::BatchSession s; std::uint64_t frame = 0;
    s.open(snap); neutral(s, frame); s.preset(2); neutral(s, frame); s.confirm(); neutral(s, frame);
    const auto request = s.take(snap, true);
    check(request && request->quantity == 2 && request->original.quantity == 5, "native source quantity is not a stock bound");
  }

  for (bool pending : {false, true}) for (unsigned change = 0; change < 5; ++change) {
    const auto snap = example(23, true);
    ubc::BatchSession s; std::uint64_t frame = 0;
    s.open(snap); neutral(s, frame); s.preset(7); neutral(s, frame);
    if (pending) { s.confirm(); neutral(s, frame); }
    auto current = snap;
    switch (change) {
      case 0: ++current.selection.stored[0]; break;
      case 1: ++current.selection.stored[5]; break;
      case 2: current.selection.reserved[2] = 0; break;
      case 3: current.maximum = 24; break;
      case 4: current.maximum = 0; break;
    }
    check(!s.refresh(current), "all material changes discard the request");
    check(s.phase() == SelectionPhase::selecting && s.issue() == SelectionIssue::stock_changed &&
          s.quantity() == (current.maximum ? 1u : 0u) && !s.armed(), "material change resets amount and release gate");
    neutral(s, frame);
    check(!s.take(current, true), "release alone never renews changed-stock confirmation");
  }

  for (unsigned change = 0; change < 9; ++change) {
    const auto snap = example(23, true);
    ubc::BatchSession s; std::uint64_t frame = 0;
    s.open(snap); neutral(s, frame); s.preset(7); neutral(s, frame); s.confirm(); neutral(s, frame);
    auto current = snap;
    switch (change) {
      case 0: ++current.selection.key.world; break;
      case 1: ++current.selection.key.hero; break;
      case 2: ++current.selection.key.station; break;
      case 3: ++current.selection.key.collection; break;
      case 4: current.amount_contract = false; break;
      case 5: current.selection.stock_at_rest = false; break;
      case 6: current.selection.count = 1; break;
      case 7: ++current.selection.offers[0].function; current.selection.offers[0].function += 5; break;
      case 8: ++current.selection.offers[1].quantity; break;
    }
    check(!s.refresh(current) && s.phase() == SelectionPhase::original, "context or missing/changed role fails closed");
    neutral(s, frame);
    check(!s.take(current, true), "context and role failure cannot dispatch later");
  }

  for (bool poll_first : {false, true}) {
    const auto snap = example();
    ubc::BatchSession s; std::uint64_t frame = 0;
    s.open(snap);
    for (int i = 0; i < 4; ++i) {
      if (poll_first) s.observe(++frame, true, true);
      check(!s.confirm(), "held opener cannot confirm in either order");
      if (!poll_first) s.observe(++frame, true, true);
    }
    neutral(s, frame);
    if (poll_first) s.observe(++frame, true, true);
    check(s.confirm(), "fresh confirm works before or after held poll");
    if (!poll_first) s.observe(++frame, true, true);
    check(!s.confirm() && !s.armed(), "one action accepted per release cycle");
    for (int i = 0; i < 5; ++i) s.observe(++frame, true, true);
    check(!s.take(snap, true), "held action cannot dispatch");
    s.observe(++frame, false, false);
    neutral(s, frame);
    check(s.phase() == SelectionPhase::selecting && !s.take(snap, true), "focus loss cancels confirmed request permanently");
    check(s.confirm(), "fresh action after focus recovery is required and accepted");
    check(s.back(), "back cancels while confirmation is held");
    check(!s.return_ready(), "cancel never passes held input through");
    for (int i = 0; i < 3; ++i) { s.observe(++frame, false, true); s.original(SelectionIssue::context_changed); }
    check(s.return_ready(), "repeated original request does not restart release gate");
  }
  std::printf("BatchSession: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
