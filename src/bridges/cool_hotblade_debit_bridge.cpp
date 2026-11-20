// SPDX-License-Identifier: MIT
#include "cool_hotblade_debit_bridge.hpp"
#include <intrin.h>
#if !defined(_WIN32) || !defined(_M_IX86)
#error Der isolierte Hotblade-Entzug benoetigt Windows x86.
#endif

namespace ubc::cool_hotblade_debit_bridge {
Executor::Executor(debit::Tracker& tracker, const Config& config) noexcept
    : tracker_(tracker), config_(config) {
  configured_ = config.owner_thread && config.remove && config.restore_pop &&
                config.restore_instance && config.restore_item && config.read &&
                config.read_restore;
}
Diagnostics Executor::diagnostics() const noexcept {
  return {configured_, InterlockedCompareExchange(&active_, 0, 0) != 0,
          InterlockedCompareExchange(&disabled_, 0, 0) != 0,
          InterlockedCompareExchange(&native_scope_, 0, 0) != 0};
}
void Executor::disable() noexcept { InterlockedExchange(&disabled_, 1); }
bool Executor::read(ReadStage stage, debit::Observation& value) noexcept {
  const DWORD error = GetLastError();
  alignas(16) unsigned char fx[512];
  _fxsave(fx);
  bool ok = false;
  __try {
    __try {
      ok = config_.read(config_.context, stage, value);
    } __finally {
      _fxrstor(fx);
      SetLastError(error);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    disable();
  }
  return ok && !InterlockedCompareExchange(&disabled_, 0, 0);
}
bool Executor::read_restore(const debit::Call& call, debit::RestoreObservation& value,
                            debit::RestorePlan& plan) noexcept {
  const DWORD error = GetLastError();
  alignas(16) unsigned char fx[512];
  _fxsave(fx);
  bool ok = false;
  value = {};
  plan = {};
  __try {
    __try {
      ok = config_.read_restore(config_.context, call, value);
    } __finally {
      _fxrstor(fx);
      SetLastError(error);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    disable();
  }
  if (!ok || InterlockedCompareExchange(&disabled_, 0, 0) || value.serial <= last_serial_)
    return false;
  plan = debit::restore_plan(call, value);
  if (!plan.valid || plan.pop_entry != restore_pop_entry ||
      plan.instance_entry != restore_instance_entry || plan.item_entry != restore_item_entry)
    return false;
  last_serial_ = value.serial;
  return true;
}
bool Executor::restore(const debit::Call& call) noexcept {
  debit::RestoreObservation value{};
  debit::RestorePlan plan{};
  if (!read_restore(call, value, plan))
    return false;
  bool pop = false, instance = false, item = false;
  __try {
    config_.restore_pop(reinterpret_cast<void*>(plan.pop_this), static_cast<int>(plan.pop_value));
    pop = true;
  } __except (
      EXCEPTION_EXECUTE_HANDLER) { /* Attempt remaining parts only after a fresh check of each target. */
  }
  if (!read_restore(call, value, plan))
    return false;
  __try {
    config_.restore_instance(reinterpret_cast<void*>(plan.instance_symbol));
    instance = true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  if (!read_restore(call, value, plan))
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
  // Even a no-op requires fresh, exact readback of all three parts.
  return read_restore(call, value, plan) && pop && instance && item &&
         debit::values::same(call.parser, value.current) &&
         debit::same_item(call.item, value.item, call.parser);
}
Outcome Executor::run() {
  const DWORD entry_error = GetLastError();
  if (!configured_)
    return Outcome::rejected;
  if (GetCurrentThreadId() != config_.owner_thread) {
    disable();
    SetLastError(entry_error);
    return Outcome::rejected;
  }
  if (InterlockedCompareExchange(&active_, 1, 0) != 0) {
    disable();
    SetLastError(entry_error);
    return Outcome::rejected;
  }
  Outcome outcome = Outcome::rejected;
  bool attempted = false, normal = false, restored = false;
  debit::Call call{};
  debit::Observation before{}, after{};
  debit::RestoreObservation gate{};
  debit::RestorePlan plan{};
  DWORD error = entry_error;
  __try {
    if (InterlockedCompareExchange(&disabled_, 0, 0)) {
      tracker_.quarantine(debit::Issue::scope);
      __leave;
    }
    if (!read(ReadStage::before_debit, before) || before.context.thread != config_.owner_thread ||
        before.serial <= last_serial_) {
      tracker_.quarantine(debit::Issue::observation);
      __leave;
    }
    call = tracker_.begin_debit(before);
    if (!call.valid)
      __leave;
    last_serial_ = before.serial;
    InterlockedExchange(&native_scope_, 1);
    // Before Remove the entire saved context must still match exactly. Only
    // the split constructor may change the cursor, UseInstance or ITEM.
    if (!read_restore(call, gate, plan) || !debit::values::same(call.parser, gate.current) ||
        !debit::same_item(call.item, gate.item, call.parser)) {
      tracker_.quarantine(debit::Issue::restore);
      __leave;
    }
    attempted = true;
    __try {
      SetLastError(error);
      (void)config_.remove(reinterpret_cast<void*>(call.job.context.hero),
                           static_cast<int>(call.instance), static_cast<int>(call.quantity));
      error = GetLastError();
      normal = true;
    } __finally {
      if (AbnormalTermination())
        error = GetLastError();
      alignas(16) unsigned char fx[512];
      _fxsave(fx);
      restored = restore(call);
      if (!normal)
        tracker_.quarantine(debit::Issue::debit_exception);
      else if (!restored)
        tracker_.quarantine(debit::Issue::restore);
      _fxrstor(fx);
      SetLastError(error);
    }
    if (!restored || InterlockedCompareExchange(&disabled_, 0, 0)) {
      tracker_.quarantine(debit::Issue::restore);
      __leave;
    }
    if (!read(ReadStage::after_debit, after) || after.context.thread != config_.owner_thread ||
        after.serial <= last_serial_) {
      tracker_.quarantine(debit::Issue::observation);
      __leave;
    }
    last_serial_ = after.serial;
    if (tracker_.debit_returned(after, normal, restored))
      outcome = Outcome::debit_verified;
  } __finally {
    if (AbnormalTermination()) {
      error = GetLastError();
      tracker_.quarantine(attempted ? debit::Issue::debit_exception : debit::Issue::observation);
    }
    if (tracker_.status().blocked)
      disable();
    InterlockedExchange(&native_scope_, 0);
    InterlockedExchange(&active_, 0);
    SetLastError(error);
  }
  return outcome;
}
} // namespace ubc::cool_hotblade_debit_bridge
