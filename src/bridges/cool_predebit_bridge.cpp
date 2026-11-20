// SPDX-License-Identifier: MIT
#include "cool_predebit_bridge.hpp"
#include <intrin.h>
#if !defined(_WIN32) || !defined(_M_IX86)
#error Der native Vorentzug benoetigt Windows x86.
#endif

namespace ubc::cool_predebit_bridge {
Executor::Executor(debit::Tracker& tracker, const Config& config) noexcept
    : tracker_(tracker), config_(config), reader_(config.owner_thread) {
  configured_ = config.owner_thread && config.memory.read && config.boundary && config.remove &&
                config.restore_pop && config.restore_instance && config.restore_item;
#if !defined(UBC_COOL_PREDEBIT_HOST_TEST)
  configured_ = configured_ && reinterpret_cast<std::uintptr_t>(config.remove) == 0x7495f0 &&
                reinterpret_cast<std::uintptr_t>(config.restore_pop) == 0x7a4b20 &&
                reinterpret_cast<std::uintptr_t>(config.restore_instance) == 0x7a1550 &&
                reinterpret_cast<std::uintptr_t>(config.restore_item) == 0x7a2f20;
#endif
}
Diagnostics Executor::diagnostics() const noexcept {
  return {configured_, InterlockedCompareExchange(&active_, 0, 0) != 0, unavailable(),
          InterlockedCompareExchange(&native_scope_, 0, 0) != 0};
}
bool Executor::unavailable() const noexcept {
  return InterlockedCompareExchange(&disabled_, 0, 0) != 0;
}
void Executor::disable() noexcept { InterlockedExchange(&disabled_, 1); }
bool Executor::boundary(reader::Phase phase, reader::Input& q) noexcept {
  const auto& job = request_.job;
  if (!writer_ || !batch_call::writer_undisturbed(*writer_) || !writer_->original ||
      writer_->token != job.writer_token ||
      reinterpret_cast<std::uintptr_t>(writer_->call.parser) != job.context.parser ||
      writer_->call.symbol != static_cast<int>(debit::callback) ||
      writer_->call.caller != batch_call::info_call_return ||
      writer_->call.thread != config_.owner_thread || unavailable() ||
      !prepared_.current(*request_.info_lease) || !config_.boundary(config_.context, phase, q) ||
      !batch_call::writer_undisturbed(*writer_) || unavailable() ||
      !prepared_.current(*request_.info_lease))
    return false;
  return debit::same_job(q.job, job) && q.frame == request_.frame &&
         q.amount_patch == request_.patch && q.phase == phase &&
         q.current_thread == config_.owner_thread && q.serial > last_serial_ &&
         !q.destroy_started && !q.destroy_completed &&
         debit::scope_matches(job, q.scope,
                              InterlockedCompareExchange(&native_scope_, 0, 0) ? 1u : 0u);
}
bool Executor::capture(reader::Phase phase, const debit::Call* call,
                       reader::Capture& out) noexcept {
  const DWORD error = GetLastError();
  alignas(16) unsigned char fx[512];
  _fxsave(fx);
  if (request_.rejection && !request_.rejection->event)
    request_.rejection->phase = phase;
  bool ok = false;
  out = {};
  __try {
    __try {
      reader::Input q;
      if (boundary(phase, q)) {
        q.call = call;
        out = reader_.capture(config_.memory, q);
        read_issue_ = out.issue;
        if (out.known) {
          // A destroy event or lease loss DURING the read also invalidates it.
          last_serial_ = q.serial;
          reader::Input closing;
          if (boundary(phase, closing)) {
            last_serial_ = closing.serial;
            ok = true;
          } else if (request_.rejection)
            request_.rejection->gate(phase, debit::Gate::capture_close);
        } else if (request_.rejection) {
          request_.rejection->reader_failed(phase, out);
        }
      } else if (request_.rejection)
        request_.rejection->gate(phase, debit::Gate::capture_open);
    } __finally {
      _fxrstor(fx);
      SetLastError(error);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    if (request_.rejection)
      request_.rejection->gate(phase, debit::Gate::read_exception);
    disable();
  }
  return ok;
}
bool Executor::restore_read(const debit::Call& call, debit::RestoreObservation& out,
                            debit::old::RestorePlan& plan,
                            debit::Observation* before_remove) noexcept {
  reader::Capture read;
  const auto phase = before_remove ? reader::Phase::before_remove : reader::Phase::restore;
  if (!capture(phase, &call, read))
    return false;
  out = read.restore;
  plan = debit::restore_plan(call, out);
  if (before_remove)
    *before_remove = read.observation;
  return plan.valid && plan.pop_entry == 0x7a4b20 && plan.instance_entry == 0x7a1550 &&
         plan.item_entry == 0x7a2f20;
}
bool Executor::restore(const debit::Call& call) noexcept {
  debit::RestoreObservation now;
  debit::old::RestorePlan plan;
  if (!restore_read(call, now, plan))
    return false;
  bool pop = false, instance = false, item = false;
  __try {
    config_.restore_pop(reinterpret_cast<void*>(plan.pop_this), static_cast<int>(plan.pop_value));
    pop = true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  if (!restore_read(call, now, plan))
    return false;
  __try {
    config_.restore_instance(reinterpret_cast<void*>(plan.instance_symbol));
    instance = true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  if (!restore_read(call, now, plan))
    return false;
  if (plan.write_item) {
    __try {
      config_.restore_item(reinterpret_cast<void*>(plan.item_symbol),
                           static_cast<int>(plan.item_value));
      item = true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  } else
    item = true;
  return restore_read(call, now, plan) && pop && instance && item &&
         debit::value::same(call.parser, now.current) &&
         debit::old::same_item(call.item, now.item, call.parser);
}
bool Executor::extra() {
  bool verified = false, normal = false, restored = false;
  DWORD after = GetLastError();
  debit::Call call{};
  reader::Capture first{}, late{}, post{};
  debit::RestoreObservation gate;
  debit::old::RestorePlan plan;
  debit::Observation before_remove;
  __try {
    debit::check::Checks matching;
    if (!capture(reader::Phase::before_extra, nullptr, first) ||
        !prepared_.matches(first.observation, &matching) ||
        !tracker_.admit(request_.job, first.observation)) {
      if (!matching.ok() && request_.rejection && !request_.rejection->event)
        request_.rejection->failure = matching.first;
      tracker_.fail(debit::Issue::observation);
      __leave;
    }
    if (!capture(reader::Phase::before_extra, nullptr, late)) {
      tracker_.fail(debit::Issue::observation);
      __leave;
    }
    call = tracker_.enter_extra(late.observation);
    if (!call.valid)
      __leave;
    InterlockedExchange(&native_scope_, 1);
    // Restore deliberately allows Hot1 OR Hot2. Before the first
    // Remove, the exact initial Hot2 and Cold stocks must still match.
    // Otherwise the bound split would have become a WholeStack operation.
    if (!restore_read(call, gate, plan, &before_remove) ||
        !debit::rows(before_remove.stocks, late.observation.stocks.stocks) ||
        !debit::old::listed_stack(before_remove.stack, 2) ||
        !debit::value::same(call.parser, gate.current) ||
        !debit::old::same_item(call.item, gate.item, call.parser)) {
      tracker_.fail(debit::Issue::restore);
      __leave;
    }
    __try {
      SetLastError(after);
      extra_entered_ = true;
      (void)config_.remove(reinterpret_cast<void*>(call.job.context.hero),
                           static_cast<int>(call.instance), static_cast<int>(call.amount));
      after = GetLastError();
      normal = true;
    } __finally {
      if (AbnormalTermination())
        after = GetLastError();
      alignas(16) unsigned char fx[512];
      _fxsave(fx);
      if (!normal && request_.rejection)
        request_.rejection->tracker_failed(reader::Phase::before_remove,
                                           debit::Issue::debit_exception);
      restored = restore(call);
      if (!restored && request_.rejection)
        request_.rejection->native_restore_failed = true;
      if (!normal)
        tracker_.fail(debit::Issue::debit_exception);
      else if (!restored)
        tracker_.fail(debit::Issue::restore);
      _fxrstor(fx);
      SetLastError(after);
    }
    if (!restored || unavailable()) {
      tracker_.fail(debit::Issue::restore);
      __leave;
    }
    if (!capture(reader::Phase::after_extra, nullptr, post)) {
      tracker_.fail(debit::Issue::observation);
      __leave;
    }
    verified = tracker_.extra_returned(post.observation, normal, restored);
  } __finally {
    if (AbnormalTermination()) {
      after = GetLastError();
      tracker_.fail(extra_entered_ ? debit::Issue::debit_exception : debit::Issue::observation);
    }
    InterlockedExchange(&native_scope_, 0);
    SetLastError(after);
  }
  return verified;
}
void* __cdecl Executor::effect(void* raw) {
  auto& self = *static_cast<Executor*>(raw);
  void* value = nullptr;
  _fxrstor(self.effect_fx_); // Preserve entry state across patch.begin as well.
  __try {
    if (!self.extra())
      __leave;
    reader::Capture before;
    if (!self.capture(reader::Phase::before_original, nullptr, before) ||
        !self.tracker_.enter_original(before.observation)) {
      self.tracker_.fail(debit::Issue::info);
      __leave;
    }
    auto& scope = *self.writer_;
    self.original_entered_ = true;
    value = scope.original(scope.call.parser, scope.call.symbol);
    self.original_returned_ = true;
  } __finally {
    // Before the outer amount finally block, even when unwinding the original call.
    _fxsave(self.effect_fx_);
    if (auto* rejection = self.request_.rejection) {
      const auto status = self.tracker_.status();
      if (status.blocked)
        rejection->tracker_failed(rejection->phase, status.issue);
      else if (self.original_entered_ && !self.original_returned_)
        rejection->tracker_failed(reader::Phase::before_original, debit::Issue::original_exception);
    }
  }
  return value;
}
void* Executor::run(batch_call::WriterScope& scope, AmountPatch& patch, const AmountMemory& memory,
                    const Request& request, CraftingResult& guard, const CraftingRequest& craft,
                    const CraftingResultBinding& binding) {
  DWORD error = GetLastError();
  if (GetCurrentThreadId() != config_.owner_thread || InterlockedCompareExchange(&active_, 1, 0)) {
    disable();
    SetLastError(error);
    return nullptr;
  }
  void* value = nullptr;
  AmountOutcome amount;
  bool accepted = false;
  _fxsave(effect_fx_);
  extra_entered_ = original_entered_ = original_returned_ = false;
  operands_restored_ = !patch.active();
  __try {
    debit::check::Checks result_checks;
    if (!configured_ || unavailable() || !batch_call::writer_undisturbed(scope) ||
        !request.prepared || !request.info_lease || request.patch != &patch || patch.active() ||
        !request.prepared->current(*request.info_lease) ||
        !debit::same_job(request.job, request.prepared->job()) ||
        request.frame != request.prepared->frame() || scope.token != request.job.writer_token ||
        !scope.original || scope.call.thread != config_.owner_thread ||
        reinterpret_cast<std::uintptr_t>(scope.call.parser) != request.job.context.parser ||
        scope.call.symbol != static_cast<int>(debit::callback) ||
        scope.call.caller != batch_call::info_call_return ||
        !guard.begin_cool_predebit(craft, binding, *request.prepared, *request.info_lease,
                                   &result_checks)) {
      if (!result_checks.ok() && request.rejection && !request.rejection->event)
        request.rejection->failure = result_checks.first;
      if (request.rejection)
        request.rejection->gate(reader::Phase::admission, debit::Gate::executor_entry);
      __leave;
    }
    prepared_ = *request.prepared;
    request_ = request;
    writer_ = &scope;
    // Address and operand come from the same immutable admission.
    SetLastError(error);
    value = run_amount_call(patch, memory, prepared_.body_address(), prepared_.amount().body(),
                            debit::quantity, &effect, this, operands_restored_, amount);
    error = GetLastError();
    if (!original_returned_ || !operands_restored_ || !amount.entered || !amount.restored)
      __leave;
    reader::Capture after;
    if (capture(reader::Phase::after_original, nullptr, after) &&
        tracker_.original_returned(after.observation, true, true))
      accepted = guard.cool_predebit_restored(tracker_.witness(), after.observation.frame);
  } __finally {
    if (AbnormalTermination())
      error = GetLastError();
    accepted = accepted && !unavailable() && batch_call::writer_undisturbed(scope);
    // UbcBatchWriter blocks 20775 before releasing the exclusive gate. Restoring
    // the operand alone cannot resolve an uncertain parser or ITEM effect.
    scope.restore_verified = operands_restored_ && (!extra_entered_ || accepted);
    if (!accepted) {
      tracker_.fail(original_entered_ && !original_returned_ ? debit::Issue::original_exception
                    : !operands_restored_                    ? debit::Issue::amount_restore
                                                             : debit::Issue::original_result);
      if (request.rejection && patch.issue() == AmountIssue::none &&
          amount.issue == AmountIssue::none)
        request.rejection->tracker_failed(request.rejection->phase, tracker_.status().issue);
      guard.fail(scope.restore_verified ? CraftingResultIssue::contract
                                        : CraftingResultIssue::restore);
      disable();
    }
    writer_ = nullptr;
    request_ = {};
    InterlockedExchange(&active_, 0);
    _fxrstor(effect_fx_);
    SetLastError(error);
  }
  return value;
}
} // namespace ubc::cool_predebit_bridge
