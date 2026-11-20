// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/lhiver_recipe_profile.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

// Own facts/values, no original texts, scripts, or engine calls. Native
// intake: before AI_PRINTSCREEN enqueue 0x6E320A; Print 0x759270;
// destructor 0x76A4A0. The queue and its object lifetime remain original.
namespace ubc::ammo_presentation {
using Digest = lhiver::Digest;
struct Fingerprint {
  std::uint32_t length = 0;
  Digest sha256{};
};
inline bool same(const Fingerprint& a, const Fingerprint& b) noexcept {
  return a.length == b.length && a.sha256 == b.sha256;
}
struct Template {
  std::uint32_t callback, owner, output, units_per_lot, success_pc, text_symbol;
  Fingerprint text;
};
inline constexpr Fingerprint font{21, lhiver::digest_literal(
  "b0ee6624c33f4f973067ef10217f6a33f0df22be9cb7974647680b34f065a20f")};
inline constexpr std::array<Template, 5> templates{{
  {20091,20087,16804,30,692433,51741,{22,lhiver::digest_literal("4b04dd2e61589e64b90dfe5c4535183322f5960cddb450aa6f8036eef0c43908")}},
  {20094,20087,15946,10,693183,51750,{27,lhiver::digest_literal("acb7f0d5fe7115b2756ea5e2040a7304dc51e44670295de446515641a86ec87c")}},
  {20096,20087,15945,10,693739,51756,{31,lhiver::digest_literal("9132020f657f19015ce7dbf3a59ee390bc1fdd432dfb73bd178dabaa234feb91")}},
  {20102,20098,16805,30,694536,51770,{22,lhiver::digest_literal("dfdce4a8416c56cbb0f2dc18d760d950b02fd034a59708911788c2aa4bd7c90a")}},
  {20105,20098,15949,10,695286,51779,{31,lhiver::digest_literal("f73cd80ffa951e4a7de4ab8bb54af6863b0b56897a2b0b0eef907a5c1b35c1e1")}}
}};
inline constexpr std::uint32_t external_dispatch_return = 0x792568;
inline constexpr std::uint32_t conversation_vtable = 0x83de0c;
inline constexpr std::size_t string_read_bound = 128;
inline const Template* find(std::uint32_t callback) noexcept {
  for (const auto& value : templates) if (value.callback == callback) return &value;
  return nullptr;
}

struct Context {
  std::uint64_t epoch = 0;
  std::uint32_t thread = 0, parser = 0, game = 0, world = 0, hero = 0;
};
inline bool valid(const Context& value) noexcept {
  return value.epoch && value.thread && value.parser == lhiver::game_parser &&
    value.game && value.world && value.hero;
}
inline bool same(const Context& a, const Context& b) noexcept {
  return a.epoch == b.epoch && a.thread == b.thread && a.parser == b.parser &&
    a.game == b.game && a.world == b.world && a.hero == b.hero;
}
struct Job {
  Context context{};
  std::uint64_t id = 0, scope_token = 0;
  std::uint32_t callback = 0, owner = 0, lots = 0;
};
struct Admission {
  bool native_contract = false, lifetime_observers = false, ui_available = false;
  bool original_body_and_amount_contract = false, self_equals_hero = false;
};
// Current, real TLS evidence of the synchronous own effect. For
// later queue processing, no active effect TLS is required anymore.
// A token must never be reconstructed from a global "last order".
struct Scope {
  std::uint64_t job = 0, token = 0;
  std::uint32_t callback = 0, depth = 0;
  bool active = false, patched_body_verified = false;
};
struct Payload {
  bool known = false;
  std::uint32_t vtable = 0, subtype = 0;
  Fingerprint text{}, font{};
  std::int32_t x = 0, y = 0;
  std::uint32_t seconds_bits = 0;
};
inline bool same(const Payload& a, const Payload& b) noexcept {
  return a.known && b.known && a.vtable == b.vtable && a.subtype == b.subtype &&
    same(a.text,b.text) && same(a.font,b.font) && a.x == b.x && a.y == b.y &&
    a.seconds_bits == b.seconds_bits;
}
struct Capture {
  Context context{};
  Scope scope{};
  std::uint32_t message = 0, event_manager = 0, target = 0, vm_pc = 0, caller = 0;
  Payload payload{};
  bool native_contract = false, manager_owner_verified = false;
};
struct Display {
  Context context{};
  std::uint32_t message = 0, receiver = 0;
  Payload payload{};
  bool native_contract = false;
};
struct Destruction {
  Context context{};
  std::uint32_t message = 0;
  Payload payload{};
  bool native_contract = false;
};
enum class DisplayDecision { original, suppress_owned };
enum class ResultState { unknown, matched_ready, mismatch };
struct Result {
  Context context{};
  std::uint64_t job = 0;
  std::uint32_t callback = 0, lots = 0, output = 0;
  std::int64_t actual_output_delta = -1;
  ResultState state = ResultState::unknown;
  bool bridge_healthy = false;
};
struct Notice {
  bool valid = false;
  Context context{};
  std::uint64_t job = 0;
  std::uint32_t callback = 0, lots = 0, output = 0, produced = 0;
};
enum class Issue {
  none, busy, admission, job, quantity, scope, context, capture, payload,
  duplicate_capture, duplicate_display, lifetime, missing_capture, result, ui
};
enum class Stage { idle, armed, captured, suppressed, destroyed };
struct Status {
  Stage stage = Stage::idle;
  Issue issue = Issue::none;
  bool blocked = false, effect_closed = false, result_confirmed = false;
  bool notice_offered = false, notice_registered = false;
  std::uint32_t message = 0;
};

// Exactly one own receipt on a populated game thread. No atomic guarantee
// against arbitrary foreign memory writes. The native adapter checks
// boundary/thread/class/code and reads payloads limited and stable. This
// value core replaces neither these readers nor the material/result watch.
//
// A print decision is only together with a further healthy bridge
// evidence of suppression. Every bridge error locks later result and
// UI releases. There is no reset, time limit, or pointer-only fallback.
class Tracker {
public:
  Tracker() noexcept = default;
  Tracker(const Tracker&) = delete;
  Tracker& operator=(const Tracker&) = delete;
  Tracker(Tracker&&) = delete;
  Tracker& operator=(Tracker&&) = delete;
  Status status() const noexcept { return state_; }
  const Job& job() const noexcept { return job_; }
  bool arm(const Job& value, const Admission& proof) noexcept {
    if (state_.blocked) return false;
    if (state_.stage != Stage::idle) return false; // A new job does not disturb the old one.
    if (!proof.native_contract || !proof.lifetime_observers || !proof.ui_available ||
        !proof.original_body_and_amount_contract || !proof.self_equals_hero) return false;
    const auto* profile = find(value.callback);
    if (!valid(value.context) || !profile || value.owner != profile->owner ||
        !value.id || value.id <= last_job_ || !value.scope_token || value.scope_token <= last_scope_) return false;
    if (value.lots <= 1 || value.lots > static_cast<std::uint32_t>(INT32_MAX) / profile->units_per_lot) return false;
    job_ = value; profile_ = profile; payload_ = {};
    last_job_ = value.id; last_scope_ = value.scope_token;
    state_ = {}; state_.stage = Stage::armed;
    return true;
  }
  bool capture(const Capture& value) noexcept {
    if (state_.blocked || state_.stage == Stage::idle || !profile_) return false;
    if (state_.message && value.message == state_.message) return fail(Issue::duplicate_capture);
    // Foreign/error PCs are ordinary original messages, even in own
    // callback. Only the concrete success point can create a receipt.
    if (value.vm_pc != profile_->success_pc) return false;
    if (!same(job_.context,value.context)) return fail(Issue::context);
    if (!scope_matches(value.scope)) return fail(Issue::scope);
    if (state_.stage != Stage::armed) return fail(Issue::duplicate_capture);
    if (!value.native_contract || !value.manager_owner_verified || !value.message || !value.event_manager ||
        value.target != job_.context.hero || value.caller != external_dispatch_return) return fail(Issue::capture);
    if (!expected(value.payload)) return fail(Issue::payload);
    state_.message = value.message;
    payload_ = value.payload; state_.stage = Stage::captured;
    return true;
  }
  DisplayDecision display(const Display& value) noexcept {
    if (state_.blocked || !state_.message || value.message != state_.message) return DisplayDecision::original;
    if (state_.stage != Stage::captured) { fail(Issue::duplicate_display); return DisplayDecision::original; }
    if (!same(job_.context,value.context) || value.receiver != job_.context.hero) {
      fail(Issue::context); return DisplayDecision::original;
    }
    if (!value.native_contract || !same(payload_,value.payload)) {
      fail(Issue::lifetime); return DisplayDecision::original;
    }
    state_.stage = Stage::suppressed;
    return DisplayDecision::suppress_owned;
  }
  bool destroy(const Destruction& value) noexcept {
    if (!state_.message || value.message != state_.message) return false;
    if (state_.blocked) return false; // Quarantine is permanent even after deletion.
    if ((state_.stage != Stage::captured && state_.stage != Stage::suppressed) ||
        !same(job_.context,value.context) || !value.native_contract || !same(payload_,value.payload)) return fail(Issue::lifetime);
    // On destructor ENTRY, before release: the old address must never suppress
    // a display again. Explicitly retire our messages that were not displayed.
    // New jobs also require completion and an actual UI registration.
    state_.message = 0; state_.stage = Stage::destroyed; complete_if_ready();
    return true;
  }
  bool close_effect(const Context& context, const Scope& scope, bool bridge_healthy) noexcept {
    if (state_.blocked || state_.stage == Stage::idle) return false;
    if (!bridge_healthy || !same(job_.context,context)) return fail(Issue::context);
    if (!scope_matches(scope) || state_.effect_closed) return fail(Issue::scope);
    if (state_.stage == Stage::armed) return fail(Issue::missing_capture);
    state_.effect_closed = true; complete_if_ready(); return true;
  }
  bool confirm_result(const Result& value) noexcept {
    if (state_.blocked || state_.stage == Stage::idle) return false;
    if (!value.bridge_healthy || !same(job_.context,value.context)) return fail(Issue::context);
    if (value.job != job_.id || value.callback != job_.callback || value.lots != job_.lots ||
        value.output != profile_->output) return fail(Issue::result);
    if (value.state == ResultState::unknown) return false;
    if (!state_.effect_closed || value.state != ResultState::matched_ready ||
        value.actual_output_delta != static_cast<std::int64_t>(job_.lots) * profile_->units_per_lot) return fail(Issue::result);
    state_.result_confirmed = true; return true;
  }
  Notice take_notice(const Context& context, bool bridge_healthy) noexcept {
    Notice result;
    if (state_.blocked || state_.stage == Stage::idle) return result;
    if (!bridge_healthy || !same(job_.context,context)) { fail(Issue::context); return result; }
    if (!state_.effect_closed || !state_.result_confirmed || state_.notice_offered ||
        (state_.stage != Stage::suppressed && state_.stage != Stage::destroyed)) return result;
    state_.notice_offered = true;
    return {true,job_.context,job_.id,job_.callback,job_.lots,profile_->output,job_.lots*profile_->units_per_lot};
  }
  // Only after successful registration with our UI. A nonzero
  // receipt must come from this UI, never a stub. It does
  // not establish physical visibility or a crafting result.
  bool acknowledge_notice(const Context& context, std::uint64_t job, std::uint64_t ui_receipt,
                          bool registered, bool bridge_healthy) noexcept {
    if (state_.blocked || state_.stage == Stage::idle) return false;
    if (!bridge_healthy || !same(job_.context,context)) return fail(Issue::context);
    if (job != job_.id || !state_.notice_offered || state_.notice_registered || !registered || !ui_receipt) return fail(Issue::ui);
    state_.notice_registered = true; complete_if_ready(); return true;
  }
  bool observe_context(const Context& current, bool bridge_healthy) noexcept {
    if (state_.blocked) return false;
    if (!bridge_healthy) return fail(Issue::context);
    if (state_.stage == Stage::idle) return true;
    return bridge_healthy && same(job_.context,current) ? true : fail(Issue::context);
  }
private:
  bool scope_matches(const Scope& scope) const noexcept {
    return scope.active && scope.patched_body_verified && scope.depth == 1 &&
      scope.job == job_.id && scope.token == job_.scope_token && scope.callback == job_.callback;
  }
  bool expected(const Payload& value) const noexcept {
    return value.known && value.vtable == conversation_vtable && value.subtype == 21 &&
      same(value.text,profile_->text) && same(value.font,font) && value.x == -1 &&
      value.y == 30 && value.seconds_bits == 0x40000000; // float32(2), no NaN tolerance.
  }
  bool fail(Issue issue) noexcept {
    state_.blocked = true; state_.issue = issue; return false;
  }
  void complete_if_ready() noexcept {
    if (state_.stage == Stage::destroyed && state_.effect_closed && state_.result_confirmed && state_.notice_registered) {
      state_.stage = Stage::idle; state_.message = 0;
    }
  }
  Job job_{};
  const Template* profile_ = nullptr;
  Payload payload_{};
  std::uint64_t last_job_ = 0, last_scope_ = 0;
  Status state_{};
};
} // namespace ubc::ammo_presentation
