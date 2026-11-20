// SPDX-License-Identifier: MIT
#include "spirit_flask_debit_bridge.hpp"
#include <intrin.h>
#if !defined(_WIN32) || !defined(_M_IX86)
#error Der isolierte Flaschenentzug benoetigt Windows x86.
#endif

namespace ubc::spirit_flask_debit_bridge {
Executor::Executor(debit::Tracker& tracker,const Config& config) noexcept
  :tracker_(tracker),config_(config) {
  ready_=config.owner_thread&&config.remove&&config.restore_pop&&
    config.restore_instance&&config.read&&config.read_restore&&config.restore_item&&config.read_item;
}
Diagnostics Executor::diagnostics() const noexcept {
  return {ready_,InterlockedCompareExchange(&active_,0,0)!=0,
    InterlockedCompareExchange(&disabled_,0,0)!=0,InterlockedCompareExchange(&native_scope_,0,0)!=0};
}
void Executor::disable() noexcept {InterlockedExchange(&disabled_,1);}
bool Executor::read(ReadStage stage,debit::Observation& value) noexcept {
  const DWORD error=GetLastError();alignas(16) unsigned char fx[512];
  _fxsave(fx);bool ok=false;
  __try {
    __try {ok=config_.read(config_.context,stage,value);}
    __finally {_fxrstor(fx);SetLastError(error);}
  } __except(EXCEPTION_EXECUTE_HANDLER) {disable();}
  return ok;
}
bool Executor::can_restore(const debit::Call& call) noexcept {
  const DWORD error=GetLastError();alignas(16) unsigned char fx[512];
  _fxsave(fx);bool ok=false;debit::RestoreObservation value{};
  __try {
    __try {
      ok=config_.read_restore(config_.context,call,value)&&debit::restorable(call,value);
    } __finally {_fxrstor(fx);SetLastError(error);}
  } __except(EXCEPTION_EXECUTE_HANDLER) {disable();}
  return ok;
}
bool Executor::read_item(const debit::Call& call,ItemObservation& out) noexcept {
  const DWORD error=GetLastError();alignas(16) unsigned char fx[512];
  _fxsave(fx);bool ok=false;
  __try {
    __try {
      ok=config_.read_item(config_.context,call,out) && out.native_set_offset_contract &&
        spirit_item_restore::plan(call,out.item,out.item,out.context,out.scope).valid;
    } __finally {_fxrstor(fx);SetLastError(error);}
  } __except(EXCEPTION_EXECUTE_HANDLER) {disable();}
  return ok;
}
bool Executor::restore_item(const debit::Call& call,const alchemy_extra_dependencies::ItemBinding& saved) noexcept {
  namespace dep=alchemy_extra_dependencies;
  if(!can_restore(call))return false;
  ItemObservation current{};
  if(!read_item(call,current))return false;
  const auto plan=spirit_item_restore::plan(call,saved,current.item,current.context,current.scope);
  if(!plan.valid || plan.native_entry!=restore_item_entry)return false;
  if(plan.requires_write) {
    __try {config_.restore_item(reinterpret_cast<void*>(plan.symbol_object),static_cast<int>(plan.saved_value));}
    __except(EXCEPTION_EXECUTE_HANDLER) {disable();return false;}
  }
  // Even the no-op path needs current evidence of ownership and values.
  ItemObservation after{};
  return can_restore(call) && read_item(call,after) && dep::same(saved,after.item);
}
bool Executor::restore(const debit::Call& call,const alchemy_extra_dependencies::ItemBinding& saved_item) noexcept {
  if(!can_restore(call))return false;
  bool pop=false,instance=false;
  // Even if SetPopPos fails exceptionally, try the second, independently
  // verified part of the context. No direct memory writes or fallbacks.
  __try {
    config_.restore_pop(reinterpret_cast<void*>(call.context.parser+0x48u),
      static_cast<int>(call.parser.pop_pos));pop=true;
  } __except(EXCEPTION_EXECUTE_HANDLER) {disable();}
  // Reread the targets after the first call to exclude stale pointers.
  if(!can_restore(call))return false;
  __try {
    config_.restore_instance(reinterpret_cast<void*>(call.parser.use_symbol));instance=true;
  } __except(EXCEPTION_EXECUTE_HANDLER) {disable();}
  const bool item=restore_item(call,saved_item);
  return pop&&instance&&item;
}
Outcome Executor::run() {
  if(!ready_)return Outcome::rejected;
  if(GetCurrentThreadId()!=config_.owner_thread){disable();return Outcome::rejected;}
  if(InterlockedCompareExchange(&active_,1,0)!=0){disable();return Outcome::rejected;}
  Outcome outcome=Outcome::rejected;
  bool attempted=false,normal=false,restored=false;
  debit::Call call{};debit::Observation before{},after{};
  ItemObservation saved_item{};
  DWORD error=GetLastError();
  __try {
    if(InterlockedCompareExchange(&disabled_,0,0)){
      tracker_.quarantine(debit::Issue::bridge);__leave;
    }
    if(tracker_.status().stage==debit::Stage::debit_verified&&tracker_.job().quantity==1){
      if(!tracker_.status().blocked)outcome=Outcome::no_extra_debit;__leave;
    }
    if(!read(ReadStage::before_debit,before)||before.context.thread!=config_.owner_thread||
      InterlockedCompareExchange(&disabled_,0,0)){
      tracker_.quarantine(debit::Issue::observation);__leave;
    }
    call=tracker_.begin_debit(before);
    if(!call.valid)__leave;
    InterlockedExchange(&native_scope_,1);
    // Bind ITEM only in the valid calling scope, immediately before the
    // single Remove. An unreadable write target must have no native effect.
    if(!can_restore(call) || !read_item(call,saved_item) || InterlockedCompareExchange(&disabled_,0,0)) {
      tracker_.quarantine(debit::Issue::context_restore);__leave;
    }
    attempted=true; // From here the native path may already have had a partial effect.
    __try {
      SetLastError(error);
      (void)config_.remove(reinterpret_cast<void*>(call.context.hero),
        static_cast<int>(call.instance),static_cast<int>(call.quantity));
      error=GetLastError();normal=true;
    } __finally {
      if(AbnormalTermination())error=GetLastError();
      alignas(16) unsigned char fx[512];_fxsave(fx);
      restored=restore(call,saved_item.item);
      if(!normal)tracker_.quarantine(debit::Issue::debit_exception);
      else if(!restored)tracker_.quarantine(debit::Issue::context_restore);
      _fxrstor(fx);SetLastError(error);
    }
    if(!restored||InterlockedCompareExchange(&disabled_,0,0)){
      tracker_.quarantine(debit::Issue::bridge);__leave;
    }
    if(!read(ReadStage::after_debit,after)||after.context.thread!=config_.owner_thread||
      InterlockedCompareExchange(&disabled_,0,0)){
      tracker_.quarantine(debit::Issue::observation);__leave;
    }
    ItemObservation final_item{};
    if(!read_item(call,final_item) || !alchemy_extra_dependencies::same(saved_item.item,final_item.item)) {
      tracker_.quarantine(debit::Issue::context_restore);__leave;
    }
    if(tracker_.debit_returned(after,normal,restored))outcome=Outcome::verified;
  } __finally {
    if(AbnormalTermination()){
      error=GetLastError();tracker_.quarantine(attempted?debit::Issue::debit_exception:debit::Issue::bridge);
    }
    if(tracker_.status().blocked)disable();
    InterlockedExchange(&native_scope_,0);InterlockedExchange(&active_,0);SetLastError(error);
  }
  return outcome;
}
} // namespace ubc::spirit_flask_debit_bridge
