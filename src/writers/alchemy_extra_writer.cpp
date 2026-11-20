// SPDX-License-Identifier: MIT
#include "alchemy_extra_writer.hpp"
#include "ubc/core/batch_amount_call.hpp"
#include <intrin.h>
#if !defined(_WIN32) || !defined(_M_IX86)
#error Der isolierte Zusatzalchemie-Writer benoetigt Windows x86.
#endif

namespace ubc::alchemy_extra_writer {
namespace bridge=alchemy_flask_debit_bridge;
Executor::Executor(const Config& config) noexcept
  :config_(config),reader_(config.owner_thread),
#if defined(UBC_ALCHEMY_EXTRA_HOST_TEST)
    host_reader_(config.owner_thread),
#endif
    debit_(result_.tracker_for_bridge(),
    {config.owner_thread,this,config.remove,config.restore_pop,config.restore_instance,read_debit,read_restore,config.restore_item,read_item}) {
  ready_=config.owner_thread && config.memory.read && config.boundary && debit_.diagnostics().ready;
}
Diagnostics Executor::diagnostics() const noexcept {
  return {ready_,InterlockedCompareExchange(&active_,0,0)!=0,unavailable()};
}
ScopeEvidence Executor::scope_evidence() const noexcept {
  if(GetCurrentThreadId()!=config_.owner_thread || !InterlockedCompareExchange(&active_,0,0) ||
     !scope_ || !recipe_)return {};
  return {true,{request_.job.id,request_.job.writer_token,request_.job.callback,1,
    original_active_?1u:0u,debit_.diagnostics().native_scope?1u:0u,true}};
}
bool Executor::unavailable() const noexcept {return InterlockedCompareExchange(&disabled_,0,0)!=0;}
void Executor::disable() noexcept {InterlockedExchange(&disabled_,1);debit_.disable();}
bool Executor::boundary(Phase phase,Boundary& out) noexcept {
  if(unavailable() || GetCurrentThreadId()!=config_.owner_thread || !recipe_)return false;
  if(!config_.boundary(config_.context,phase,request_.job,out))return false;
  const auto& b=out.runtime;const auto& s=b.scope;
  const auto actual=scope_evidence();
  const bool in_debit=phase==Phase::after_debit || phase==Phase::restore_targets || phase==Phase::item_targets;
  return actual.valid && s.vm_depth==actual.scope.vm_depth && s.debit_depth==actual.scope.debit_depth &&
    !unavailable() && b.current_thread==config_.owner_thread && b.context.thread==config_.owner_thread &&
    debit::same(b.context,request_.job.context) && s.job==request_.job.id && s.writer_token==request_.job.writer_token &&
    s.callback==request_.job.callback && s.writer_active && s.writer_depth==1 && !s.vm_depth &&
    s.debit_depth==(in_debit?1u:0u) && b.synchronous_writer_verified &&
    b.physical_lab_hand_contract_verified &&
    (phase!=Phase::before_original || (out.native_handoff_verified && out.actual_info==request_.actual_info && out.actual_info));
}
alchemy_extra_dependency_reader::Input Executor::dependency_input(const Boundary& current) const noexcept {
  alchemy_extra_dependency_reader::Input out;
  out.boundary=current.runtime;out.modules=current.modules;out.quantity=request_.job.quantity;
  out.targets={static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(config_.remove)),
    static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(config_.restore_pop)),
    static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(config_.restore_instance))};
  out.restore_item=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(config_.restore_item));
  return out;
}
void Executor::record_parser_failure(Phase phase,const runtime::Capture& read) noexcept {
  if(read.valid || read.issue!=runtime::Issue::parser || outcome_.parser_failure.captured)return;
  outcome_.parser_failure={true,phase,
    runtime::parser_failure_snapshot(config_.memory,request_.job.context,read.observation.parser)};
}
bool Executor::capture(Phase phase,debit::Observation& out) noexcept {
  const DWORD error=GetLastError();alignas(16) unsigned char fx[512];_fxsave(fx);bool ok=false;
  __try {
    __try {
      Boundary current;
      if(boundary(phase,current)) {
#if defined(UBC_ALCHEMY_EXTRA_HOST_TEST)
        if(config_.host_dependencies && config_.host_dependencies(config_.context,phase,request_.job)) {
          current.runtime.native_dependencies_and_configured_targets_verified=true;
          const auto read=host_reader_.capture(config_.memory,*recipe_,current.runtime,{},
            phase==Phase::before_original?runtime::ParserStackMode::empty:runtime::ParserStackMode::original_returns);
          outcome_.read_issue=read.issue;
          record_parser_failure(phase,read);
          if(read.valid && !unavailable()) {out=read.observation;ok=true;}
        }
#else
        using P=alchemy_extra_dependency_reader::Phase;
        const auto selected=phase==Phase::before_original?P::before_original:phase==Phase::after_original?P::after_original:
          phase==Phase::before_debit?P::before_debit:P::after_debit;
        const auto read=reader_.capture(config_.memory,*recipe_,dependency_input(current),selected);
        outcome_.read_issue=read.runtime.issue;outcome_.dependency_issue=read.guard.issue;
        record_parser_failure(phase,read.runtime);
        if(read.valid && !unavailable()) {out=read.runtime.observation;ok=true;}
#endif
      }
    } __finally {_fxrstor(fx);SetLastError(error);}
  } __except(EXCEPTION_EXECUTE_HANDLER) {disable();}
  return ok;
}
bool Executor::restore_targets(const debit::Call& call,debit::RestoreObservation& out) noexcept {
  // The existing debit bridge already protects this observer with SEH/FX.
  Boundary current;if(!boundary(Phase::restore_targets,current))return false;
#if defined(UBC_ALCHEMY_EXTRA_HOST_TEST)
  if(!config_.host_dependencies || !config_.host_dependencies(config_.context,Phase::restore_targets,request_.job))return false;
  current.runtime.native_dependencies_and_configured_targets_verified=true;
  const auto read=host_reader_.restore_targets(config_.memory,call,current.runtime);
  outcome_.read_issue=read.issue;if(!read.valid || unavailable())return false;out=read.observation;
#else
  const auto read=reader_.restore_targets(config_.memory,call,dependency_input(current));
  outcome_.read_issue=read.runtime.issue;outcome_.dependency_issue=read.guard.issue;
  if(!read.valid || unavailable())return false;out=read.runtime.observation;
#endif
  return true;
}
bool Executor::item_targets(const debit::Call& call,bridge::ItemObservation& out) noexcept {
  Boundary current;if(!boundary(Phase::item_targets,current))return false;
#if defined(UBC_ALCHEMY_EXTRA_HOST_TEST)
  if(!config_.host_dependencies || !config_.host_dependencies(config_.context,Phase::item_targets,request_.job))return false;
  current.runtime.native_dependencies_and_configured_targets_verified=true;
  const auto target=host_reader_.restore_targets(config_.memory,call,current.runtime);
  if(!target.valid)return false;
  out.item=alchemy_extra_dependencies::read_item_binding(config_.memory,call.context.parser);
  if(!out.item.known)return false;
#else
  const auto target=reader_.restore_targets(config_.memory,call,dependency_input(current));
  outcome_.read_issue=target.runtime.issue;outcome_.dependency_issue=target.guard.issue;
  if(!target.valid)return false;out.item=target.guard.item;
#endif
  out.context=current.runtime.context;out.scope=current.runtime.scope;
  out.native_set_offset_contract=!unavailable();return out.native_set_offset_contract;
}
bool Executor::entry(alchemy_extra_execution::Entry& out) noexcept {
  const DWORD error=GetLastError();alignas(16) unsigned char fx[512];_fxsave(fx);bool ok=false;
  __try {
    __try {
      out=alchemy_extra_execution::verify_entry(config_.memory,*recipe_,request_.actual_info);
      // After the local reads, reverify the actual native handoff/TLS
      // boundary. A callback return or cached collection is insufficient.
      Boundary current;ok=out.valid && boundary(Phase::before_original,current) && !unavailable();
    } __finally {_fxrstor(fx);SetLastError(error);}
  } __except(EXCEPTION_EXECUTE_HANDLER) {disable();}
  return ok;
}
bool Executor::local_restored() noexcept {
  const DWORD error=GetLastError();alignas(16) unsigned char fx[512];_fxsave(fx);bool ok=false;
  __try {
    __try {ok=alchemy_extra_execution::verify_recipe(config_.memory,*recipe_).valid && !unavailable();}
    __finally {_fxrstor(fx);SetLastError(error);}
  } __except(EXCEPTION_EXECUTE_HANDLER) {disable();}
  return ok;
}
bool __cdecl Executor::read_debit(void* raw,bridge::ReadStage stage,debit::Observation& out) {
  auto& self=*static_cast<Executor*>(raw);
  if(!self.capture(stage==bridge::ReadStage::before_debit?Phase::before_debit:Phase::after_debit,out))return false;
  if(stage==bridge::ReadStage::after_debit) {self.debit_after_=out;self.debit_after_read_=true;}
  return true;
}
bool __cdecl Executor::read_restore(void* raw,const debit::Call& call,debit::RestoreObservation& out) {
  return static_cast<Executor*>(raw)->restore_targets(call,out);
}
bool __cdecl Executor::read_item(void* raw,const debit::Call& call,bridge::ItemObservation& out) {
  return static_cast<Executor*>(raw)->item_targets(call,out);
}
void* __cdecl Executor::original(void* raw) {
  auto& self=*static_cast<Executor*>(raw);
  if(self.unavailable() || !self.scope_ || self.outcome_.original_entered) {
    self.disable();return nullptr;
  }
  self.outcome_.original_entered=true;
  self.original_active_=true;
  void* result=nullptr;
  __try {
    result=self.scope_->original(self.scope_->call.parser,self.scope_->call.symbol);
    self.outcome_.original_returned=true;
  } __finally {
    const auto error=GetLastError();self.original_active_=false;_fxsave(self.original_fx_);self.original_fx_captured_=true;SetLastError(error);
  }
  return result;
}
void* Executor::run(batch_call::WriterScope& scope,AmountPatch& patch,const AmountMemory& memory,const Request& incoming) {
  // Another thread must never read or write non-atomic order fields.
  if(!ready_ || GetCurrentThreadId()!=config_.owner_thread) {disable();scope.restore_verified=false;return nullptr;}
  if(InterlockedCompareExchange(&active_,1,0)) {disable();scope.restore_verified=false;return nullptr;}
  DWORD error=GetLastError();void* value=nullptr;
  bool armed=false;
  scope.restore_verified=false;outcome_={};debit_after_={};debit_after_read_=false;original_fx_captured_=false;original_active_=false;
  __try {
    if(unavailable())__leave;
    request_=incoming;recipe_=alchemy_recipe_catalog::find(request_.snapshot.id);scope_=&scope;
    const auto& job=request_.job;
    if(!recipe_ || !alchemy_recipe_catalog::plan_quantity(*recipe_,job.quantity).valid || !scope.original || patch.active() ||
       scope.call.thread!=config_.owner_thread || scope.call.caller!=batch_call::info_call_return ||
       reinterpret_cast<std::uintptr_t>(scope.call.parser)!=lhiver::game_parser ||
       scope.call.symbol!=static_cast<std::int32_t>(recipe_->callback.function->symbol) ||
       job.callback!=recipe_->callback.function->symbol || job.owner!=static_cast<std::uint32_t>(recipe_->owner.instance) ||
       memory.reader.context!=config_.memory.context || memory.reader.read!=config_.memory.read ||
       !request_.actual_info || result_.status().stage==AlchemyExtraResultStage::blocked)__leave;
    debit::Observation before{};alchemy_extra_execution::Entry evidence;
    if(!capture(Phase::before_original,before) || !entry(evidence))__leave;
    const debit::Admission admission{true,true,true,true,true,true};
    // All six checks above passed: the exact profile and amount body, runtime native and
    // flask constructor guards, plus explicit dependencies, knowledge, owner and hand.
    if(!result_.begin(request_.snapshot,job,admission,before))__leave;
    armed=true;AmountOutcome amount;
    SetLastError(error);
    __try {
      value=run_amount_call(patch,memory,evidence.code_address,recipe_->callback.amount,job.quantity,
        original,this,scope.restore_verified,amount);
      error=GetLastError();
    } __finally {
      if(AbnormalTermination())error=GetLastError();
      outcome_.operands_restored=scope.restore_verified;outcome_.amount_issue=amount.issue;
    }
    if(!amount.entered || !outcome_.original_returned || !scope.restore_verified || unavailable() || !local_restored())__leave;
    debit::Observation after{};
    if(!capture(Phase::after_original,after) || !result_.original_returned(after,true,true))__leave;
    if(job.quantity>1) {
      SetLastError(error);
      if(debit_.run()!=bridge::Outcome::verified || !debit_after_read_ ||
         !result_.acknowledge_extra_debit(debit_after_))__leave;
      outcome_.vm_restore=outcome_.item_restore=RestoreProof::verified;
    } else outcome_.vm_restore=outcome_.item_restore=RestoreProof::not_required_for_one;
    outcome_.closing=result_.status().stage==AlchemyExtraResultStage::closing;
  } __finally {
    if(AbnormalTermination()) {
      // If the additional call fails, keep its own error state. Do not substitute
      // the LastError/FX state from the preceding successful original call.
      error=GetLastError();_fxsave(original_fx_);original_fx_captured_=true;
    }
    if(AbnormalTermination() || !outcome_.closing || unavailable()) {
      // Even a known abort after arm must not be retried automatically.
      if(armed)result_.tracker_for_bridge().quarantine(outcome_.original_entered?debit::Issue::bridge:debit::Issue::observation);
      disable();outcome_.blocked=true;
      // The conservative bridge check also locks the global writer when only operand
      // restoration is confirmed and the remaining VM/effect state is unknown.
      scope.restore_verified=false;
    }
    scope_=nullptr;InterlockedExchange(&active_,0);
    if(original_fx_captured_)_fxrstor(original_fx_);
    SetLastError(error);
  }
  return value;
}
AlchemyExtraResultStage Executor::observe_completion(const debit::Observation& value,const debit::EndWitness& witness,bool normal) noexcept {
  if(GetCurrentThreadId()!=config_.owner_thread || InterlockedCompareExchange(&active_,1,0)) {disable();return AlchemyExtraResultStage::blocked;}
  auto stage=AlchemyExtraResultStage::blocked;
  if(!unavailable())stage=result_.observe_completion(value,witness,normal);
  if(stage==AlchemyExtraResultStage::blocked)disable();
  InterlockedExchange(&active_,0);return stage;
}
} // namespace ubc::alchemy_extra_writer
