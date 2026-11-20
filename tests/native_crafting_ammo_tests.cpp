// SPDX-License-Identifier: MIT
#include <windows.h>
#include "ubc/engine/native_crafting_ammo.hpp"
#include "bridges/ammo_presentation_bridge.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Isolated Windows-x86 host with actual adapter and naked bridge paths,
// synthetic original counters and an explicit environment seam. Only in
// this host does that seam replace RAM, code and installation evidence. It
// does not authorize engine use or establish world, payload or original hash
// evidence. The fixed RAM and code verifiers have separate ammo_runtime_tests.
namespace p = ubc::ammo_presentation;
namespace r = ubc::ammo_runtime;
namespace b = ubc::ammo_presentation_bridge;
namespace n = ubc::native_crafting_ammo;
namespace {
unsigned checks = 0, failures = 0;
void check(bool value, const char* name) {
  ++checks;
  if (!value) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}
constexpr std::uint32_t message = 0x3100000, foreign_message = 0x3101000, manager = 0x3200000,
                        hero = 0x3300000;
constexpr DWORD input_error = 0x12345678, original_error = 0x22334455, host_exception = 0xe043b715;
p::Context current{1, 0, ubc::lhiver::game_parser, 0x3400000, 0x3500000, hero};
p::Scope scope{};
p::Payload payload{};
p::Job job{};
unsigned originals[3]{}, reads[3]{}, original_guards = 0, installed_guards = 0, contract_guards = 0,
                                     prepare_calls = 0, install_calls = 0;
unsigned context_calls = 0, scope_calls = 0;
bool original_valid = true, prepared = true, installed_valid = true, install_ok = true,
     contract_valid = true, current_valid = true;
bool context_ok = true, scope_ok = true, raise_context = false, raise_scope = false,
     reenter = false, immediate = false;
bool change_capture_scope = false, change_display_context = false, change_context_reader = false,
     change_scope_reader = false;
bool payload_valid = true, capture_valid = true, destruction_valid = true, install_raises = false;
std::uint32_t pc = 0;
r::Issue capture_issue = r::Issue::unreadable;
std::uint32_t capture_caller = p::external_dispatch_return;
std::array<std::uintptr_t, 3> destinations{};
b::CaptureSite captured{};
void invoke_capture();
void __cdecl original_capture();
int __thiscall original_print(void*, void*);
void __thiscall original_destroy(void*);
}
namespace ubc::native_crafting_ammo_host {
struct Backend {
  static r::Verification original() noexcept;
  static r::Verification installed(const r::InstalledHooks&) noexcept;
  static r::Verification contract(std::uint32_t, const r::InstalledHooks&) noexcept;
  static bool current(const p::Context&) noexcept;
  static bool prepare(const std::array<std::uintptr_t, 3>&) noexcept;
  static bool install(std::uintptr_t);
  static void* continuation(std::size_t) noexcept;
  static r::Read<p::Capture> capture(const p::Context&, const p::Scope&, const r::CaptureSite&,
                                     const r::InstalledHooks&) noexcept;
  static r::Read<p::Display> display(const p::Context&, const r::PrintSite&,
                                     const r::InstalledHooks&) noexcept;
  static r::Read<p::Destruction> destruction(const p::Context&, const r::DestructorSite&,
                                             const r::InstalledHooks&) noexcept;
};
}
#define UBC_NATIVE_CRAFTING_AMMO_HOST
#include "../src/plugin/native_crafting_ammo.cpp"

namespace {
bool __cdecl read_context(void*, p::Context& out) {
  ++context_calls;
  SetLastError(123);
  if (raise_context)
    RaiseException(host_exception, 0, 0, nullptr);
  if (reenter) {
    reenter = false;
    (void)n::admit(job, {false, false, true, true, true});
  }
  out = current;
  if (change_context_reader && context_calls % 2 == 0)
    ++out.epoch;
  return context_ok;
}
bool __cdecl read_scope(void*, p::Scope& out) {
  ++scope_calls;
  if (raise_scope)
    RaiseException(host_exception, 0, 0, nullptr);
  out = scope;
  if (change_scope_reader && scope_calls % 2 == 0)
    ++out.token;
  return scope_ok;
}
void __cdecl original_capture() {
  ++originals[0];
  if (immediate) {
    const auto result =
        UbcAmmoPrintBridge(reinterpret_cast<void*>(hero), reinterpret_cast<void*>(message));
    check(result == 1, "immediate native queue display suppresses owned print exactly once");
    UbcAmmoDestructorBridge(reinterpret_cast<void*>(message));
  }
  SetLastError(original_error);
}
int __thiscall original_print(void*, void*) {
  ++originals[1];
  SetLastError(original_error);
  return 77;
}
void __thiscall original_destroy(void*) {
  ++originals[2];
  SetLastError(original_error);
}
__declspec(naked) void invoke_capture() {
  __asm {
    pushad
    mov edi,0ab40c0h
    mov esi,3100000h
    mov eax,3200000h
    call UbcAmmoCaptureBridge
    popad
    ret
  }
}
bool proof_matches(const r::InstalledHooks& proof) {
  for (std::size_t i = 0; i < 3; ++i)
    if (!proof.hooks[i].installed || proof.hooks[i].destination != destinations[i] ||
        proof.hooks[i].trampoline != static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(
                                         ubc::native_crafting_ammo_host::Backend::continuation(i))))
      return false;
  return true;
}
bool setup() {
  current.thread = GetCurrentThreadId();
  check(!n::healthy() && !n::permits_next(), "uninstalled adapter cannot admit a job");
  check(!n::configure({nullptr, nullptr, read_scope}),
        "missing reader rejected before immutable configuration");
  check(n::configure({nullptr, read_context, read_scope}),
        "bounded own readers configured exactly once");
  check(!n::configure({nullptr, read_context, read_scope}), "reader registry cannot be replaced");
  SetLastError(input_error);
  const bool ok = n::install(current.thread);
  check(GetLastError() == input_error, "installation preserves caller LastError");
  return ok;
}
void set_job(std::size_t index, std::uint64_t id) {
  const auto& profile = p::templates[index];
  job = {current, id, id + 100, profile.callback, profile.owner, 7};
  scope = {};
  pc = profile.success_pc;
  payload = {true, p::conversation_vtable, 21, profile.text, p::font, -1, 30, 0x40000000};
}
bool arm(std::size_t index = 0, std::uint64_t id = 1) {
  set_job(index, id);
  SetLastError(input_error);
  const auto before_contract = contract_guards;
  const bool ok = n::admit(job, {false, false, true, true, true});
  check(ok, "admission derives native/lifetime proof from own installed guards");
  check(contract_guards == before_contract + 1,
        "every admitted job freshly verifies complete native contract");
  check(GetLastError() == input_error, "admission preserves LastError despite reader Win32 calls");
  check(!n::permits_next(), "admitted job excludes another own ammo job");
  return ok;
}
bool enter() {
  scope = {job.id, job.scope_token, job.callback, 1, true, true};
  const bool ok = n::enter_effect(current, scope);
  check(ok, "one active current verified patch scope enters");
  return ok;
}
void capture_owned() {
  const auto before = originals[0];
  SetLastError(input_error);
  invoke_capture();
  check(originals[0] == before + 1,
        "capture always forwards original queue continuation exactly once");
  check(GetLastError() == original_error, "capture preserves original continuation LastError");
  check(captured.parser == current.parser && captured.message == message &&
            captured.event_manager == manager && captured.stack,
        "actual bridge supplies EDI parser, ESI message, EAX manager and entry stack");
}
void close() {
  check(n::close_effect(current, scope),
        "close consumes the same still-active synchronous effect scope");
  scope = {};
}
p::Result result(p::ResultState state = p::ResultState::matched_ready) {
  const auto* profile = p::find(job.callback);
  return {current,         job.id,
          job.callback,    job.lots,
          profile->output, static_cast<std::int64_t>(job.lots) * profile->units_per_lot,
          state,           true};
}
void print_owned() {
  const auto before = originals[1];
  SetLastError(input_error);
  const auto value =
      UbcAmmoPrintBridge(reinterpret_cast<void*>(hero), reinterpret_cast<void*>(message));
  check(value == 1 && originals[1] == before,
        "only owned intact message skips original print and returns one");
  check(GetLastError() == input_error, "owned suppression retains incoming LastError");
}
void destroy_owned() {
  const auto before = originals[2];
  UbcAmmoDestructorBridge(reinterpret_cast<void*>(message));
  check(originals[2] == before + 1, "owned destructor always executes original exactly once");
  check(!n::status().tracker.message,
        "destructor entry retires address before original frees object");
}
void foreign_print() {
  const auto before = originals[1], before_reads = reads[1];
  check(UbcAmmoPrintBridge(reinterpret_cast<void*>(hero),
                           reinterpret_cast<void*>(foreign_message)) == 77,
        "foreign message follows original print");
  check(originals[1] == before + 1 && reads[1] == before_reads,
        "foreign message is not dereferenced by own payload reader");
}
void retired_print() {
  const auto before = originals[1], before_reads = reads[1];
  check(UbcAmmoPrintBridge(reinterpret_cast<void*>(hero), reinterpret_cast<void*>(message)) == 77,
        "reused retired address cannot suppress an unrelated new message");
  check(originals[1] == before + 1 && reads[1] == before_reads,
        "retired message pointer is not read again");
}
void rejected_owned_print() {
  const auto before = originals[1];
  check(UbcAmmoPrintBridge(reinterpret_cast<void*>(hero), reinterpret_cast<void*>(message)) == 77 &&
            originals[1] == before + 1,
        "unverified or duplicate owned message follows original exactly once");
}
void expect_blocked() {
  check(!n::healthy() && !n::permits_next() && !n::take_notice().valid,
        "failure permanently blocks ammo and own success notice");
  check(!n::admit(job, {true, true, true, true, true}), "failure has no implicit retry or rearm");
  foreign_print();
  check(!b::diagnostics().observer_active,
        "observer lease returns even after failed bounded callback");
}
void positive() {
  check(setup(), "complete explicit installation succeeds in synthetic host environment");
  check(original_guards == 2 && prepare_calls == 1 && install_calls == 1 && installed_guards == 1,
        "original guards surround prepare/configure; installed guards follow own writes");
  check(!n::install(current.thread) && n::healthy() && n::permits_next(),
        "second install cannot mutate or reset healthy transport");
  for (std::size_t i = 0; i < p::templates.size(); ++i)
    for (unsigned order = 0; order < 3; ++order) {
      if (!arm(i, 1 + 3 * i + order))
        return;
      const auto fresh = n::status().capture;
      check(fresh.job == job.id && fresh.scope_token == job.scope_token &&
                fresh.callback == job.callback && fresh.expected_pc == pc &&
                fresh.expected_caller == p::external_dispatch_return && !fresh.attempts &&
                !fresh.reads && !fresh.accepted && !fresh.reader_seen,
            "successful admission binds and resets diagnostics to exactly this new job");
      const auto before_reads = reads[0];
      invoke_capture();
      check(reads[0] == before_reads && n::status().tracker.stage == p::Stage::armed,
            "capture before explicit effect entry remains ordinary without reading payload");
      check(!n::status().capture.attempts,
            "unrelated capture before effect does not enter own diagnostic counters");
      auto competing = job;
      ++competing.id;
      ++competing.scope_token;
      check(!n::admit(competing, {true, true, true, true, true}),
            "busy adapter cannot replace pending receipt");
      if (!enter())
        return;
      const auto expected_pc = pc;
      pc = expected_pc - 5;
      invoke_capture();
      pc = expected_pc;
      check(n::healthy() && n::status().tracker.stage == p::Stage::armed,
            "ordinary error or foreign VM PC is not captured");
      immediate = order == 2;
      capture_owned();
      immediate = false;
      const auto recorded = n::status().capture;
      check(
          recorded.job == job.id && recorded.attempts == 2 && recorded.reads == 2 &&
              recorded.accepted == 1 && recorded.reader_seen && recorded.reader_valid &&
              recorded.first_issue == r::Issue::none && recorded.vm_pc == expected_pc - 5 &&
              recorded.caller == p::external_dispatch_return,
          "diagnostic counters distinguish observed foreign PC from tracker admission without changing either decision");
      check(n::status().tracker.stage == (order == 2 ? p::Stage::destroyed : p::Stage::captured),
            "capture or immediate display/destruction yields precise lifecycle stage");
      close();
      foreign_print();
      check(!n::observe_result(result(p::ResultState::unknown)) && n::healthy(),
            "unknown material result remains pending");
      check(!n::take_notice().valid, "no own success notice before positive result");
      check(n::observe_result(result()), "verified output delta confirms one closed effect");
      if (order == 0) {
        check(!n::take_notice().valid,
              "result alone cannot replace a still unhandled native print");
        print_owned();
      } else if (order == 1)
        destroy_owned();
      const auto notice = n::take_notice();
      check(notice.valid && notice.job == job.id &&
                notice.produced == job.lots * p::templates[i].units_per_lot,
            "own notice carries confirmed actual lot output, not a stale static message amount");
      check(!n::take_notice().valid, "own notice is offered at most once");
      check(n::ack_notice(job.id, job.id + 1000, true),
            "actual successful UI registration can acknowledge one notice");
      if (order == 0) {
        check(!n::permits_next(), "UI receipt alone does not forget live original queue object");
        destroy_owned();
      }
      check(n::healthy() && n::permits_next() && n::status().tracker.stage == p::Stage::idle,
            "next job requires closed effect, result, real UI ack and retired native message");
      retired_print();
    }
}
void diagnostic_cases() {
  if (!setup() || !arm() || !enter())
    return;
  const auto expected_pc = pc;
  pc = expected_pc - 5;
  capture_caller = p::external_dispatch_return + 4;
  invoke_capture();
  auto value = n::status().capture;
  check(value.reader_seen && value.reader_valid && value.first_issue == r::Issue::none &&
            value.vm_pc == pc && value.caller == capture_caller,
        "first returned reader sample is available even without an accepted success PC");
  capture_valid = false;
  capture_issue = r::Issue::owner;
  pc = 0;
  capture_caller = p::external_dispatch_return + 8;
  invoke_capture();
  value = n::status().capture;
  check(
      n::healthy() && value.attempts == 2 && value.reads == 2 && !value.accepted &&
          !value.reader_valid && value.first_issue == r::Issue::owner && value.vm_pc == 0 &&
          value.caller == capture_caller && value.expected_pc == expected_pc &&
          value.expected_caller == p::external_dispatch_return,
      "first reader failure replaces successful sample without inventing a capture or weakening fail-closed classification");
  capture_issue = r::Issue::unreadable;
  capture_caller = p::external_dispatch_return + 12;
  invoke_capture();
  capture_valid = true;
  pc = expected_pc;
  capture_caller = p::external_dispatch_return;
  capture_owned();
  value = n::status().capture;
  check(
      value.attempts == 4 && value.reads == 4 && value.accepted == 1 && !value.reader_valid &&
          value.first_issue == r::Issue::owner && value.vm_pc == 0 &&
          value.caller == p::external_dispatch_return + 8,
      "later reader failure and genuine success preserve the first failed sample while counters continue");
  auto competing = job;
  ++competing.id;
  ++competing.scope_token;
  check(!n::admit(competing, {true, true, true, true, true}) && n::status().capture.job == job.id &&
            n::status().capture.first_issue == r::Issue::owner,
        "rejected competing admission cannot erase prior diagnostic evidence");
  close();
  destroy_owned();
  check(n::observe_result(result()),
        "diagnostics do not change independently verified result admission");
  check(n::take_notice().valid && n::ack_notice(job.id, 123, true),
        "diagnostic errors do not replace tracker lifetime and notice guards");
  if (!arm(1, 2))
    return;
  value = n::status().capture;
  check(value.job == 2 && value.callback == p::templates[1].callback &&
            value.expected_pc == p::templates[1].success_pc && !value.attempts && !value.reads &&
            !value.accepted && !value.reader_seen && value.first_issue == r::Issue::none &&
            !value.vm_pc && !value.caller,
        "only a successfully admitted next job clears the previous failed-reader sample");
}
void diagnostic_context_case() {
  if (!setup() || !arm() || !enter())
    return;
  context_ok = false;
  const auto before = originals[0];
  invoke_capture();
  const auto value = n::status().capture;
  check(
      originals[0] == before + 1 && value.job == job.id && value.attempts == 1 && !value.reads &&
          !value.reader_seen && !value.accepted && n::status().fault == n::Fault::context,
      "capture seen before failed context gate is distinguished from a missing hook and from an attempted payload read");
  expect_blocked();
}
DWORD WINAPI foreign_bridge(void*) {
  UbcAmmoPrintBridge(reinterpret_cast<void*>(hero), reinterpret_cast<void*>(foreign_message));
  return 0;
}
DWORD WINAPI foreign_public(void*) {
  (void)n::observe_result(result());
  return 0;
}
void thread_case(LPTHREAD_START_ROUTINE function) {
  const auto thread = CreateThread(nullptr, 0, function, nullptr, 0, nullptr);
  check(thread != nullptr, "isolated foreign test thread created");
  if (thread) {
    check(WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0,
          "isolated foreign test thread terminates");
    CloseHandle(thread);
  }
}
void negative(unsigned which) {
  if (!setup() || !arm())
    return;
  if (which == 0) {
    scope_ok = false;
    check(!n::enter_effect(current, {}), "unknown actual scope cannot enter");
    expect_blocked();
    return;
  }
  if (which == 1) {
    raise_scope = true;
    scope = {job.id, job.scope_token, job.callback, 1, true, true};
    check(!n::enter_effect(current, scope), "reader SEH is caught before original effect");
    expect_blocked();
    return;
  }
  if (which == 2) {
    contract_valid = false;
    scope = {job.id, job.scope_token, job.callback, 1, true, true};
    check(!n::enter_effect(current, scope), "changed installed code cannot enter effect");
    expect_blocked();
    return;
  }
  if (which == 3) {
    reenter = true;
    scope = {job.id, job.scope_token, job.callback, 1, true, true};
    check(!n::enter_effect(current, scope), "reader reentry blocks outer and inner operation");
    expect_blocked();
    return;
  }
  if (which == 4) {
    thread_case(foreign_public);
    expect_blocked();
    return;
  }
  if (which == 5) {
    thread_case(foreign_bridge);
    expect_blocked();
    return;
  }
  if (which == 6) {
    scope = {job.id, job.scope_token + 1, job.callback, 1, true, true};
    check(!n::enter_effect(current, scope), "foreign scope token cannot enter existing job");
    expect_blocked();
    return;
  }
  if (!enter())
    return;
  if (which == 7) {
    check(!n::enter_effect(current, scope), "effect entry is one shot");
    expect_blocked();
    return;
  }
  if (which == 8) {
    check(!n::close_effect(current, scope), "missing captured success message blocks close");
    const auto value = n::status();
    check(value.tracker.issue == p::Issue::missing_capture && !value.capture.attempts &&
              !value.capture.reader_seen,
          "missing synchronous message keeps explicit zero-capture evidence");
    expect_blocked();
    return;
  }
  if (which == 9) {
    change_capture_scope = true;
    capture_owned();
    const auto value = n::status().capture;
    check(
        value.attempts == 1 && value.reads == 1 && value.reader_valid && !value.accepted &&
            n::status().fault == n::Fault::scope,
        "returned reader evidence survives subsequent scope rejection without becoming an accepted capture");
    expect_blocked();
    return;
  }
  if (which == 10) {
    capture_valid = false;
    capture_owned();
    const auto value = n::status().capture;
    check(value.attempts == 1 && value.reads == 1 && !value.reader_valid &&
              value.first_issue == r::Issue::unreadable && value.vm_pc == pc && !value.accepted,
          "invalid expected-PC reader keeps its first issue through tracker quarantine");
    expect_blocked();
    return;
  }
  if (which == 11) {
    payload.y = 31;
    capture_owned();
    expect_blocked();
    return;
  }
  capture_owned();
  if (which == 12) {
    capture_owned();
    expect_blocked();
    return;
  }
  if (which == 13) {
    scope = {};
    check(!n::close_effect(current, scope),
          "last-job copy cannot replace disappeared active scope");
    expect_blocked();
    return;
  }
  if (which == 14) {
    raise_context = true;
    check(!n::close_effect(current, scope), "context reader SEH releases operation guard");
    expect_blocked();
    return;
  }
  if (which == 15) {
    change_context_reader = true;
    check(!n::close_effect(current, scope), "changing context copy cannot close effect");
    expect_blocked();
    return;
  }
  if (which == 16) {
    change_scope_reader = true;
    check(!n::close_effect(current, scope), "changing current scope copy cannot close effect");
    expect_blocked();
    return;
  }
  close();
  if (which == 17) {
    payload_valid = false;
    rejected_owned_print();
    expect_blocked();
    return;
  }
  if (which == 18) {
    change_display_context = true;
    rejected_owned_print();
    expect_blocked();
    return;
  }
  if (which == 19) {
    ++current.epoch;
    rejected_owned_print();
    expect_blocked();
    return;
  }
  if (which == 20) {
    destruction_valid = false;
    UbcAmmoDestructorBridge(reinterpret_cast<void*>(message));
    expect_blocked();
    return;
  }
  if (which == 21) {
    contract_valid = false;
    check(!n::observe_result(result()), "changed code after effect blocks result");
    expect_blocked();
    return;
  }
  if (which == 22) {
    auto wrong = result();
    ++wrong.actual_output_delta;
    check(!n::observe_result(wrong), "wrong real output delta cannot authorize notice");
    expect_blocked();
    return;
  }
  if (which == 23) {
    auto wrong = result();
    wrong.bridge_healthy = false;
    check(!n::observe_result(wrong),
          "known unhealthy bridge cannot be replaced by current healthy bool");
    expect_blocked();
    return;
  }
  print_owned();
  if (which == 24) {
    rejected_owned_print();
    expect_blocked();
    return;
  }
  check(n::observe_result(result()),
        "positive result established before acknowledgement negative case");
  check(n::take_notice().valid, "single notice exists before acknowledgement negative case");
  if (which == 25)
    check(!n::ack_notice(job.id, 0, true), "zero UI receipt cannot finish lifecycle");
  if (which == 26)
    check(!n::ack_notice(job.id, 77, false), "failed UI registration cannot finish lifecycle");
  if (which == 27)
    check(!n::ack_notice(job.id + 1, 77, true), "different job cannot acknowledge notice");
  if (which == 28) {
    b::disable();
    check(!n::ack_notice(job.id, 77, true),
          "bridge failure after suppression invalidates later UI acknowledgement");
  }
  expect_blocked();
}
void install_negative(unsigned which) {
  if (which == 0)
    original_valid = false;
  if (which == 1)
    prepared = false;
  if (which == 2)
    install_ok = false;
  if (which == 3)
    installed_valid = false;
  if (which == 4)
    install_raises = true;
  check(!setup(), "failed installation cannot publish a usable adapter");
  check(!n::install(current.thread), "failed installation is one shot and cannot retry later");
  check(!n::healthy() && !n::permits_next(), "installation failure keeps own ammo unavailable");
  if (which < 2)
    check(!install_calls && !b::diagnostics().configured,
          "preparation failure does not publish any bridge or patch");
  else {
    check(b::diagnostics().configured && b::diagnostics().disabled,
          "any reachable failed-install bridge was configured and now disabled");
    foreign_print();
    const auto before = originals[0];
    invoke_capture();
    check(originals[0] == before + 1,
          "partially installed capture retains valid ordinary continuation");
    const auto destroyed_before = originals[2];
    UbcAmmoDestructorBridge(reinterpret_cast<void*>(message));
    check(originals[2] == destroyed_before + 1,
          "partially installed destructor retains original lifetime path");
  }
}
void admission_negative() {
  if (!setup())
    return;
  set_job(0, 1);
  for (unsigned which = 0; which < 12; ++which) {
    auto candidate = job;
    p::Admission proof{true, true, true, true, true};
    switch (which) {
    case 0:
      proof.ui_available = false;
      break;
    case 1:
      proof.original_body_and_amount_contract = false;
      break;
    case 2:
      proof.self_equals_hero = false;
      break;
    case 3:
      ++candidate.owner;
      break;
    case 4:
      candidate.lots = 1;
      break;
    case 5:
      candidate.lots = UINT32_MAX;
      break;
    case 6:
      candidate.id = 0;
      break;
    case 7:
      candidate.scope_token = 0;
      break;
    case 8:
      ++candidate.context.world;
      break;
    case 9:
      scope = {1, 2, job.callback, 1, true, true};
      break;
    case 10:
      context_ok = false;
      break;
    case 11:
      scope_ok = false;
      break;
    }
    check(!n::admit(candidate, proof), "missing current admission proof cannot arm or capture");
    scope = {};
    context_ok = scope_ok = true;
    check(n::healthy() && n::permits_next(),
          "rejected idle request does not poison future fully proved admission");
  }
}
}

namespace ubc::native_crafting_ammo_host {
r::Verification Backend::original() noexcept {
  ++original_guards;
  return {original_valid, original_valid ? r::Issue::none : r::Issue::code_hash};
}
r::Verification Backend::installed(const r::InstalledHooks& proof) noexcept {
  ++installed_guards;
  check(proof_matches(proof),
        "installed provenance uses only own prepared destinations and continuations");
  return {installed_valid, installed_valid ? r::Issue::none : r::Issue::trampoline};
}
r::Verification Backend::contract(std::uint32_t parser, const r::InstalledHooks& proof) noexcept {
  ++contract_guards;
  const bool ok = contract_valid && parser == ubc::lhiver::game_parser && proof_matches(proof);
  return {ok, ok ? r::Issue::none : r::Issue::code_hash};
}
bool Backend::current(const p::Context& value) noexcept {
  return current_valid && p::same(value, ::current);
}
bool Backend::prepare(const std::array<std::uintptr_t, 3>& value) noexcept {
  ++prepare_calls;
  destinations = value;
  check(original_guards == 1, "full original guard precedes trampoline preparation");
  return prepared;
}
bool Backend::install(std::uintptr_t own_code) {
  ++install_calls;
  check(own_code && original_guards == 2 && b::diagnostics().configured,
        "bridge receives all continuations and final original guard BEFORE first hook write");
  const auto before = originals[1];
  UbcAmmoPrintBridge(reinterpret_cast<void*>(hero), reinterpret_cast<void*>(message));
  check(originals[1] == before + 1 && !reads[1],
        "early callback during incomplete installation is always ordinary");
  if (install_raises)
    RaiseException(host_exception, 0, 0, nullptr);
  return install_ok;
}
void* Backend::continuation(std::size_t index) noexcept {
  return index == 0   ? reinterpret_cast<void*>(&original_capture)
         : index == 1 ? reinterpret_cast<void*>(&original_print)
                      : reinterpret_cast<void*>(&original_destroy);
}
r::Read<p::Capture> Backend::capture(const p::Context& c, const p::Scope& s,
                                     const r::CaptureSite& site,
                                     const r::InstalledHooks& proof) noexcept {
  ++reads[0];
  captured = {site.thread, site.parser, site.message, site.event_manager, site.stack};
  const auto nested = n::status().capture;
  check(!nested.job && !nested.reader_seen && !nested.attempts,
        "status does not expose owner data during an active adapter operation");
  const bool valid = capture_valid && proof_matches(proof);
  p::Capture value{c,       s,     site.message, site.event_manager, hero, pc, capture_caller,
                   payload, valid, valid};
  if (change_capture_scope)
    ++scope.token;
  return {value, valid, valid ? r::Issue::none : capture_issue};
}
r::Read<p::Display> Backend::display(const p::Context& c, const r::PrintSite& site,
                                     const r::InstalledHooks& proof) noexcept {
  ++reads[1];
  const bool valid = payload_valid && proof_matches(proof);
  if (change_display_context)
    ++::current.epoch;
  return {{c, site.message, site.receiver, payload, valid},
          valid,
          valid ? r::Issue::none : r::Issue::changed};
}
r::Read<p::Destruction> Backend::destruction(const p::Context& c, const r::DestructorSite& site,
                                             const r::InstalledHooks& proof) noexcept {
  ++reads[2];
  const bool valid = destruction_valid && proof_matches(proof);
  return {{c, site.message, payload, valid}, valid, valid ? r::Issue::none : r::Issue::unreadable};
}
}
int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(
        stderr,
        "Use one isolated host case: positive, admission, diagnostic, diagnostic-context, negative-N, install-N\n");
    return 2;
  }
  if (!std::strcmp(argv[1], "positive"))
    positive();
  else if (!std::strcmp(argv[1], "admission"))
    admission_negative();
  else if (!std::strcmp(argv[1], "diagnostic"))
    diagnostic_cases();
  else if (!std::strcmp(argv[1], "diagnostic-context"))
    diagnostic_context_case();
  else if (!std::strncmp(argv[1], "negative-", 9))
    negative(static_cast<unsigned>(std::strtoul(argv[1] + 9, nullptr, 10)));
  else if (!std::strncmp(argv[1], "install-", 8))
    install_negative(static_cast<unsigned>(std::strtoul(argv[1] + 8, nullptr, 10)));
  else
    return 2;
  std::printf("%u native ammo adapter checks, %u failures (%s)\n", checks, failures, argv[1]);
  return failures ? 1 : 0;
}
