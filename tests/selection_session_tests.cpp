// SPDX-License-Identifier: MIT
#include "ubc/selection_session.hpp"

#include <cstdio>
#include <utility>

namespace {
unsigned checks = 0, failures = 0;
void check(bool ok, const char* name) {
  ++checks;
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", name); }
}
ubc::SelectionSnapshot example() {
  ubc::SelectionSnapshot s;
  s.selection_contract = s.stock_at_rest = true;
  s.key = {1, 2, 3, 4, 5, 6, 7, ubc::SelectionMode::info, -1};
  s.stored = {41, 0, 23, 50, 30, 37};
  s.offers = {{{110, 210, 0, 1}, {113, 213, 2, 5}, {116, 216, 3, 10}, {119, 219, 4, 41}}};
  s.count = 4;
  return s;
}
void neutral(ubc::SelectionSession& s, std::uint64_t& frame) {
  for (int i = 0; i < 3; ++i) s.observe(++frame, false, true);
}
void ready(ubc::SelectionSession& session, std::uint64_t& frame) {
  neutral(session, frame);
  check(session.confirm(), "fresh confirmation accepted");
  neutral(session, frame);
  check(session.phase() == ubc::SelectionPhase::ready, "release makes handoff ready");
}
}

int main() {
  using ubc::SelectionPhase;
  using ubc::SelectionIssue;
  const auto base = example();
  check(ubc::valid_selection_snapshot(base), "valid cooking snapshot");
  for (unsigned variant = 0; variant < 22; ++variant) {
    auto s = base;
    switch (variant) {
      case 0: s.selection_contract = false; break;
      case 1: s.stock_at_rest = false; break;
      case 2: s.key.generation = 0; break;
      case 3: s.key.thread = 0; break;
      case 4: s.key.world = 0; break;
      case 5: s.key.hero = 0; break;
      case 6: s.key.station = 0; break;
      case 7: s.key.manager = 0; break;
      case 8: s.key.view = 0; break;
      case 9: s.key.mode = static_cast<ubc::SelectionMode>(99); break;
      case 10: s.key.mode = ubc::SelectionMode::choice; break;
      case 11: s.stored[0] = -1; break;
      case 12: s.reserved[2] = -1; break;
      case 13: s.stored[2] = INT32_MAX; s.reserved[2] = 1; break;
      case 14: s.count = 0; break;
      case 15: s.count = 5; break;
      case 16: s.offers[0].info_instance = -1; break;
      case 17: s.offers[0].function = -1; break;
      case 18: s.offers[0].row = -1; break;
      case 19: s.offers[0].row = 256; break;
      case 20: s.offers[0].quantity = 0; break;
      case 21: s.offers[0].quantity = UINT32_MAX; break;
    }
    ubc::SelectionSession session;
    check(!session.open(s), "invalid input cannot open");
    check(session.phase() == SelectionPhase::original, "invalid input retains original handling");
    check(!session.take(base, true), "invalid snapshot cannot dispatch");
  }
  for (bool function : {false, true}) {
    auto s = base;
    if (function) s.offers[1].function = s.offers[0].function;
    else s.offers[1].row = s.offers[0].row;
    check(!ubc::valid_selection_snapshot(s), "ambiguous offer rejected");
  }
  auto alchemy = base;
  alchemy.key.mode = ubc::SelectionMode::choice;
  alchemy.key.owner_instance = 300;
  alchemy.count = 3;
  for (std::size_t i = 0; i < alchemy.count; ++i) alchemy.offers[i].info_instance = 300;
  alchemy.reserved[2] = 1;
  check(ubc::valid_selection_snapshot(alchemy), "valid choice with reserved flask");
  alchemy.offers[1].info_instance = 301;
  check(!ubc::valid_selection_snapshot(alchemy), "choice must belong to current owner");

  ubc::SelectionSession session;
  std::uint64_t frame = 0;
  check(session.open(base), "open initial session");
  check(!session.armed() && session.selected() == 0, "open requires release and defaults to one");
  for (int i = 0; i < 12; ++i) {
    session.observe(++frame, true, true);
    check(!session.confirm(), "held opener cannot confirm");
    check(!session.select(2), "held opener cannot select");
    check(!session.take(base, true), "held opener cannot dispatch");
  }
  neutral(session, frame);
  check(session.select(2), "select original ten");
  check(session.confirm(), "fresh confirmation for ten");
  check(!session.confirm(), "duplicate confirmation ignored");
  check(session.phase() == SelectionPhase::releasing, "confirm waits for release");
  session.observe(++frame, false, true);
  check(session.phase() == SelectionPhase::releasing, "event frame not counted as released");
  for (int i = 0; i < 10; ++i) session.observe(frame, false, true);
  check(session.phase() == SelectionPhase::releasing, "same frame cannot advance release");
  session.observe(++frame, false, true);
  check(!session.take(base, true), "one neutral frame cannot dispatch");
  session.observe(++frame, false, true);
  check(!session.take(base, false), "own UI must be closed");
  auto reordered = base;
  std::swap(reordered.offers[0], reordered.offers[2]);
  reordered.offers[0].row = 12;
  const auto selected = session.take(reordered, true);
  check(selected && selected->function == 216 && selected->quantity == 10 && selected->row == 12,
        "fresh function mapping uses new row, never stale index");
  check(session.phase() == SelectionPhase::dispatched, "ticket consumed before caller executes");
  for (int i = 0; i < 12; ++i) {
    neutral(session, frame);
    check(!session.confirm() && !session.take(base, true), "consumed ticket cannot execute again");
  }
  check(!session.open(base), "same generation cannot reopen consumed ticket");
  check(session.phase() == SelectionPhase::dispatched, "stale open preserves dispatched terminal state");
  auto invalid_generation = base; invalid_generation.key.generation = 0;
  check(!session.open(invalid_generation), "zero generation cannot reopen consumed ticket");
  session.original(SelectionIssue::context_changed);
  neutral(session, frame);
  check(session.phase() == SelectionPhase::dispatched && !session.return_ready() &&
        !session.take(base, true), "error fallback after dispatch cannot reactivate original input");
  auto later = base; later.key.generation = 2;
  check(session.open(later), "new generation can open");

  for (unsigned variant = 0; variant < 15; ++variant) {
    ubc::SelectionSession s;
    std::uint64_t tick = 0;
    s.open(base); ready(s, tick);
    auto changed = base;
    switch (variant) {
      case 0: --changed.stored[0]; break;
      case 1: ++changed.stored[0]; break;
      case 2: ++changed.reserved[2]; break;
      case 3: ++changed.key.generation; break;
      case 4: ++changed.key.thread; break;
      case 5: ++changed.key.world; break;
      case 6: ++changed.key.hero; break;
      case 7: ++changed.key.station; break;
      case 8: ++changed.key.manager; break;
      case 9: ++changed.key.view; break;
      case 10: ++changed.offers[0].function; break;
      case 11: ++changed.offers[0].info_instance; break;
      case 12: ++changed.offers[0].quantity; break;
      case 13: changed.stock_at_rest = false; break;
      case 14: ++changed.key.collection; break;
    }
    check(!s.take(changed, true), "changed preview never executes silently");
    check(!s.take(changed, true), "changed preview cannot retry without new confirmation");
    if (variant < 3 || (variant >= 10 && variant <= 12)) {
      check(s.phase() == SelectionPhase::selecting && !s.armed(), "refresh requires new release and confirmation");
      ready(s, tick);
      check(s.take(changed, true).has_value(), "fresh confirmation accepts updated preview");
    } else check(s.phase() == SelectionPhase::original, "context loss returns original mode");
  }
  {
    ubc::SelectionSession s; std::uint64_t tick = 0;
    s.open(base); neutral(s,tick);
    check(s.preset(0),"quantity-row confirmation consumes input even when amount stays one");
    check(!s.confirm()&&!s.preset(0),"held quantity-row confirmation cannot also manufacture");
    neutral(s,tick);
    check(s.phase()==SelectionPhase::selecting&&!s.take(base,true),"preset never creates a production ticket");
    check(s.confirm(),"separate confirmation after preset release accepted");
  }
  {
    ubc::SelectionSession s; std::uint64_t tick = 0;
    s.open(base); neutral(s, tick); s.select(3); s.confirm(); neutral(s, tick);
    auto changed = base; changed.count = 3;
    check(!s.take(changed, true) && s.issue() == SelectionIssue::offer_changed,
          "removed offer invalidates ticket");
    check(s.selected() == 0, "removed offer resets quantity to one");
  }
  for (bool ready_before_focus : {false, true}) {
    ubc::SelectionSession s; std::uint64_t tick = 0;
    s.open(base); neutral(s, tick); s.confirm();
    if (ready_before_focus) neutral(s, tick);
    s.observe(++tick, false, false);
    check(s.phase() == SelectionPhase::selecting && s.issue() == SelectionIssue::focus_lost,
          "focus loss cancels pending ticket");
    s.observe(++tick, true, true);
    check(!s.confirm(), "held focus regain cannot confirm");
    neutral(s, tick);
    check(!s.take(base, true), "focus regain cannot revive ticket");
    check(s.confirm(), "fresh confirmation after focus regain allowed");
    neutral(s, tick);
    check(s.take(base, true).has_value(), "only new confirmation can dispatch");
  }
  for (bool pending : {false, true}) {
    ubc::SelectionSession s; std::uint64_t tick = 0;
    s.open(base); neutral(s, tick);
    if (pending) s.confirm();
    check(s.back(), "back cancels even while confirm still held");
    check(!s.return_ready() && !s.take(base, true), "back never confirms and waits for release");
    neutral(s, tick);
    check(s.return_ready(), "return to original after release");
  }
  {
    ubc::SelectionSession s;std::uint64_t tick=0;
    s.open(base);neutral(s,tick);s.original(SelectionIssue::context_changed);
    for(int i=0;i<5;++i){
      s.observe(++tick,true,true);s.original(SelectionIssue::context_changed);
      check(!s.return_ready(),"persistent failure keeps held confirmation consumed");
    }
    for(int i=0;i<3;++i){s.observe(++tick,false,true);s.original(SelectionIssue::context_changed);}
    check(s.return_ready(),"persistent failure cannot perpetually reset release progress");
  }
  {
    ubc::SelectionInput input;
    input.observe(1, false, true); input.observe(2, false, true);
    check(input.armed(), "two independent frames arm input");
    input.observe(2, true, true);
    check(!input.armed(), "held state in duplicate frame still disarms");
    input.observe(3, false, true); input.observe(4, false, true); input.observe(5, false, true);
    check(input.armed(), "release after duplicate held frame");
    input.observe(4, false, true);
    check(!input.armed(), "backwards frame disarms");
    input.observe(6, false, true);
    check(!input.armed(), "backwards frame cannot reduce release wait");
    input.observe(7, false, true);
    check(input.armed(), "fresh monotonic frames restore release");
    check(input.press() && !input.press(), "one accepted press per release");
    input.observe(8, false, true); input.observe(9, false, true);
    check(!input.armed(), "queued event needs two later frames");
    input.observe(10, false, true);
    check(input.armed(), "event release completes");
  }
  // Actual engine order: KeyPressed may report the new key press
  // before the buffered HandleEvent. Both orders must accept
  // exactly one event and allow handoff only after release.
  for(bool poll_first:{false,true})for(unsigned action=0;action<3;++action){
    ubc::SelectionSession s;std::uint64_t tick=0;
    s.open(base);neutral(s,tick);
    const auto press=[&](){return action==0?s.confirm():(action==1?s.back():s.preset(2));};
    if(poll_first){
      for(int n=0;n<4;++n){s.observe(++tick,true,true);s.observe(tick,true,true);}
      check(!s.armed()&&!s.take(base,true),"held poll immediately closes handoff gate");
    }
    check(press(),"fresh confirm/back/preset survives either event order");
    s.observe(++tick,true,true);
    check(!press(),"same held cycle cannot deliver a second accepted event");
    check(!s.return_ready()&&!s.take(base,true),"accepted event cannot escape while held");
    neutral(s,tick);
    if(action==0)check(s.take(base,true).has_value(),"confirmed ticket dispatches only after release");
    else if(action==1)check(s.return_ready(),"back returns only after release");
    else check(s.phase()==SelectionPhase::selecting&&s.selected()==2&&!s.take(base,true),
               "preset changes quantity without creating a ticket");
  }
  for(unsigned invalidation=0;invalidation<5;++invalidation){
    ubc::SelectionInput input;
    input.observe(10,false,true);input.observe(11,false,true);input.observe(12,true,true);
    switch(invalidation){
      case 0:input.disarm();break;
      case 1:input.observe(13,true,false);break;
      case 2:input.observe(11,true,true);break;
      case 3:input.observe(0,true,true);break;
      case 4:input.reset();break;
    }
    check(!input.press(),"context/focus/frame invalidation destroys preserved event permission");
  }
  for(bool poll_first:{false,true}){
    ubc::SelectionSession s;s.open(base);
    for(std::uint64_t tick=1;tick<8;++tick){
      if(poll_first)s.observe(tick,true,true);
      check(!s.confirm()&&!s.back()&&!s.preset(0),"held opening never gains event permission");
      if(!poll_first)s.observe(tick,true,true);
    }
  }
  std::printf("SelectionSession: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
