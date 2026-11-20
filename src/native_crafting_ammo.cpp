// SPDX-License-Identifier: MIT
#include <windows.h>
#include "ubc/native_crafting_ammo.hpp"
#include "ubc/windows_probe.hpp"
#include "ubc/windows_module_identity.hpp"
#include "ubc/ammo_compat_memory.hpp"
#include "ammo_presentation_bridge.hpp"

#if !defined(_WIN32) || !defined(_M_IX86)
#error Der Munitionsadapter benoetigt Windows x86.
#endif

namespace ubc::native_crafting_ammo {
namespace {
namespace p = ammo_presentation;
namespace r = ammo_runtime;
namespace b = ammo_presentation_bridge;
namespace w = windows_probe;

// Only the isolated host compiles for a different environment. In the
// DLL build, all RAM, code and installation functions are bound to this
// implementation. No configured API can replace their verification.
#if !defined(UBC_NATIVE_CRAFTING_AMMO_HOST)
struct Backend {
  static w::Hooks<r::HookSite,r::hook_count> hooks;
  static std::array<w::LoadedModuleIdentity,vob_locator_compat::module_count> modules;
  static ammo_compat_memory::Reader reader;
  static bool attempted;
  static bool bind(void*,vob_locator_compat::Modules& result) noexcept {
    for(std::size_t i=0;i<modules.size();++i){
      if(!modules[i].current())return false;
      result.bases[i]=modules[i].base();
    }
    return true;
  }
  static void resolve() noexcept {
    if(attempted)return;attempted=true;
    constexpr std::array<const wchar_t*,5> names{{L"UNION_VOB_LOCATOR.DLL",L"ZBUGFIXES.DLL",
      L"UNION_EXT_STAT.DLL",L"shw32.dll",L"ZTORCHCONTROL.DLL"}};
    for(std::size_t i=0;i<modules.size();++i){const auto& profile=vob_locator_compat::module_profiles[i];
      modules[i].acquire(names[i],profile.image_size,profile.file_sha256);}
  }
  static ReadOnlyMemory memory() noexcept {
    return reader.memory();
  }
  static r::Verification original() noexcept { resolve();return r::verify_original_code(memory()); }
  static r::Verification installed(const r::InstalledHooks& proof) noexcept {
    return r::verify_installed_code(memory(),proof);
  }
  static r::Verification contract(std::uint32_t parser,const r::InstalledHooks& proof) noexcept {
    return r::verify_contract(memory(),parser,proof);
  }
  static bool current(const p::Context& value) noexcept { return r::detail::live_context(memory(),value); }
  static bool prepare(const std::array<std::uintptr_t,r::hook_count>& destinations) noexcept { return hooks.prepare(destinations); }
  static bool install(std::uintptr_t address) noexcept { return hooks.install(address); }
  static void* continuation(std::size_t index) noexcept { return hooks.original(index); }
  static r::Read<p::Capture> capture(const p::Context& c,const p::Scope& s,const r::CaptureSite& site,const r::InstalledHooks& proof) noexcept {
    return r::read_capture(memory(),c,s,site,proof);
  }
  static r::Read<p::Display> display(const p::Context& c,const r::PrintSite& site,const r::InstalledHooks& proof) noexcept {
    return r::read_display(memory(),c,site,proof);
  }
  static r::Read<p::Destruction> destruction(const p::Context& c,const r::DestructorSite& site,const r::InstalledHooks& proof) noexcept {
    return r::read_destruction(memory(),c,site,proof);
  }
};
w::Hooks<r::HookSite,r::hook_count> Backend::hooks(r::hook_sites);
std::array<w::LoadedModuleIdentity,vob_locator_compat::module_count> Backend::modules{};
bool Backend::attempted=false;
ammo_compat_memory::Reader Backend::reader({nullptr,w::copy_memory},nullptr,Backend::bind);
#else
using Backend = native_crafting_ammo_host::Backend;
#endif

Readers readers{};
r::InstalledHooks installed_hooks{};
p::Tracker tracker;
p::Scope entered_scope{};
CaptureDiagnostic capture_diagnostic{};
bool effect_entered = false;
volatile LONG configured = 0, attempt = 0, installed = 0, owner = 0;
volatile LONG fault = 0, runtime_issue = 0, active = 0;
LONG read(volatile LONG& value) noexcept { return InterlockedCompareExchange(&value,0,0); }
void block(Fault reason,r::Issue issue=r::Issue::none) noexcept {
  InterlockedCompareExchange(&fault,static_cast<LONG>(reason),0);
  if(issue!=r::Issue::none)InterlockedCompareExchange(&runtime_issue,static_cast<LONG>(issue),0);
  b::disable();
}
bool bridge_healthy() noexcept {
  const auto state=b::diagnostics();
  return state.configured&&!state.disabled;
}
bool own_thread() noexcept {
  return read(owner)!=0&&static_cast<std::uint32_t>(read(owner))==GetCurrentThreadId();
}
bool healthy_internal() noexcept {
  return read(installed)==1&&!read(fault)&&bridge_healthy();
}
bool sync_tracker() noexcept {
  if(tracker.status().blocked){block(Fault::tracker);return false;}
  if(!healthy_internal()){
    tracker.observe_context({},false);
    if(!read(fault))block(Fault::bridge);
    return false;
  }
  return true;
}
bool same_scope(const p::Scope& a,const p::Scope& z) noexcept {
  return a.job==z.job&&a.token==z.token&&a.callback==z.callback&&a.depth==z.depth&&
    a.active==z.active&&a.patched_body_verified==z.patched_body_verified;
}
bool idle_scope(const p::Scope& value) noexcept { return same_scope(value,{}); }
bool job_scope(const p::Scope& value) noexcept {
  const auto& job=tracker.job();
  return value.active&&value.patched_body_verified&&value.depth==1&&value.job==job.id&&
    value.token==job.scope_token&&value.callback==job.callback;
}
bool current_context(p::Context& out) {
  p::Context first{},second{};
  if(!readers.context(readers.user,first)||!p::valid(first)||first.thread!=GetCurrentThreadId()||
    !Backend::current(first)||!readers.context(readers.user,second)||!p::same(first,second)||
    !Backend::current(second))return false;
  out=second;return true;
}
bool current_scope(p::Scope& out) {
  p::Scope first{},second{};
  if(!readers.scope(readers.user,first)||!readers.scope(readers.user,second)||!same_scope(first,second))return false;
  out=second;return true;
}
bool bound_context(p::Context& out) {
  if(!current_context(out)||!p::same(out,tracker.job().context)) {block(Fault::context);return false;}
  if(!tracker.observe_context(out,healthy_internal()))return sync_tracker();
  return true;
}
bool scope_matches(const p::Scope& expected) {
  p::Scope value{};
  if(!current_scope(value)||!same_scope(value,expected)||!job_scope(value)) {block(Fault::scope);return false;}
  return true;
}
bool verify_contract(const p::Context& context) noexcept {
  const auto result=Backend::contract(context.parser,installed_hooks);
  if(!result.valid){block(Fault::installed_contract,result.issue);return false;}
  return healthy_internal();
}
void diagnostic_count(std::uint32_t& value) noexcept {if(value!=UINT32_MAX)++value;}
void diagnostic_capture(const r::Read<p::Capture>& value) noexcept {
  diagnostic_count(capture_diagnostic.reads);
  const bool previous_good=capture_diagnostic.reader_valid&&capture_diagnostic.first_issue==r::Issue::none;
  if(!capture_diagnostic.reader_seen||(previous_good&&(!value.valid||value.issue!=r::Issue::none))){
    capture_diagnostic.reader_seen=true;capture_diagnostic.reader_valid=value.valid;
    capture_diagnostic.first_issue=value.issue;capture_diagnostic.vm_pc=value.value.vm_pc;
    capture_diagnostic.caller=value.value.caller;
  }
}

struct Admit { const p::Job* job;const p::Admission* proof; };
struct Effect { const p::Context* context;const p::Scope* scope; };
struct Ack {std::uint64_t job,receipt;bool registered;};
enum class Operation { admit,enter,close,result,take,ack,capture,display,destroy };

bool implementation(Operation op,void* raw) {
  if(!sync_tracker())return false;
  if(op==Operation::admit) {
    const auto& args=*static_cast<const Admit*>(raw);
    if(tracker.status().stage!=p::Stage::idle||effect_entered)return false;
    p::Context context{},after{};p::Scope scope{},scope_after{};
    if(!current_context(context)||!p::same(context,args.job->context)||!current_scope(scope)||!idle_scope(scope))return false;
    if(!verify_contract(context))return false;
    if(!current_context(after)||!p::same(context,after)||!current_scope(scope_after)||!idle_scope(scope_after))return false;
    auto proof=*args.proof;
    proof.native_contract=true; // Entirely from verify_contract above
    proof.lifetime_observers=healthy_internal(); // Our own registry of three entries, no external booleans
    const bool accepted=tracker.arm(*args.job,proof);
    if(accepted){
      capture_diagnostic={};capture_diagnostic.job=args.job->id;
      capture_diagnostic.scope_token=args.job->scope_token;capture_diagnostic.callback=args.job->callback;
      const auto* profile=p::find(args.job->callback);
      if(profile)capture_diagnostic.expected_pc=profile->success_pc;
      capture_diagnostic.expected_caller=p::external_dispatch_return;
    }
    return accepted;
  }
  if(tracker.status().stage==p::Stage::idle)return false;
  if(op==Operation::capture) {
    // Do not read ordinary messages without an active effect of our own.
    if(!effect_entered)return false;
    const bool diagnose=capture_diagnostic.job&&capture_diagnostic.job==entered_scope.job&&
      capture_diagnostic.scope_token==entered_scope.token&&capture_diagnostic.callback==entered_scope.callback;
    if(diagnose)diagnostic_count(capture_diagnostic.attempts);
    const auto& site=*static_cast<const b::CaptureSite*>(raw);
    p::Context context{},after{};
    if(!bound_context(context)||!scope_matches(entered_scope))return false;
    const auto value=Backend::capture(context,entered_scope,
      {site.thread,site.parser,site.message,site.event_manager,site.stack},installed_hooks);
    if(diagnose)diagnostic_capture(value);
    if(!bound_context(after)||!p::same(context,after)||!scope_matches(entered_scope))return false;
    // For an ordinary error or unrelated PC, keep the original behavior. Once the expected
    // PC is recognized, an invalid reader value must also cause the receipt to be rejected.
    auto observed=value.value;observed.native_contract=observed.native_contract&&value.valid&&healthy_internal();
    const bool accepted=tracker.capture(observed);
    if(diagnose&&accepted)diagnostic_count(capture_diagnostic.accepted);
    return sync_tracker()&&accepted;
  }
  if(op==Operation::display||op==Operation::destroy) {
    const auto message=op==Operation::display?static_cast<const b::PrintSite*>(raw)->message:
      static_cast<const b::DestructorSite*>(raw)->message;
    // No payload reads for unrelated, retired or recycled addresses.
    if(!tracker.status().message||message!=tracker.status().message)return false;
    p::Context context{},after{};if(!bound_context(context))return false;
    if(op==Operation::display) {
      const auto& site=*static_cast<const b::PrintSite*>(raw);
      const auto value=Backend::display(context,{site.thread,site.receiver,site.message,site.caller},installed_hooks);
      if(!bound_context(after)||!p::same(context,after))return false;
      auto observed=value.value;observed.native_contract=observed.native_contract&&value.valid&&healthy_internal();
      const auto decision=tracker.display(observed);
      return sync_tracker()&&decision==p::DisplayDecision::suppress_owned;
    }
    const auto& site=*static_cast<const b::DestructorSite*>(raw);
    const auto value=Backend::destruction(context,{site.thread,site.message,site.caller},installed_hooks);
    if(!bound_context(after)||!p::same(context,after))return false;
    auto observed=value.value;observed.native_contract=observed.native_contract&&value.valid&&healthy_internal();
    const bool accepted=tracker.destroy(observed);return sync_tracker()&&accepted;
  }
  p::Context context{},after{};
  if(!bound_context(context)||!verify_contract(context)||!bound_context(after)||!p::same(context,after))return false;
  if(op==Operation::enter||op==Operation::close) {
    const auto& args=*static_cast<const Effect*>(raw);
    if(!p::same(*args.context,context)||!scope_matches(*args.scope)){block(Fault::scope);return false;}
    if(op==Operation::enter) {
      if(effect_entered||tracker.status().effect_closed||tracker.status().stage!=p::Stage::armed){block(Fault::scope);return false;}
      entered_scope=*args.scope;effect_entered=true;return true;
    }
    if(!effect_entered||!same_scope(entered_scope,*args.scope)){block(Fault::scope);return false;}
    const bool accepted=tracker.close_effect(context,*args.scope,healthy_internal());
    effect_entered=false;entered_scope={};return sync_tracker()&&accepted;
  }
  if(effect_entered){block(Fault::scope);return false;}
  if(op==Operation::result) {
    auto value=*static_cast<const p::Result*>(raw);
    if(!p::same(value.context,context)){block(Fault::context);return false;}
    value.bridge_healthy=value.bridge_healthy&&healthy_internal();
    const bool accepted=tracker.confirm_result(value);return sync_tracker()&&accepted;
  }
  if(op==Operation::take) {
    auto& out=*static_cast<p::Notice*>(raw);out=tracker.take_notice(context,healthy_internal());
    return sync_tracker()&&out.valid;
  }
  const auto& args=*static_cast<const Ack*>(raw);
  const bool accepted=tracker.acknowledge_notice(context,args.job,args.receipt,args.registered,healthy_internal());
  return sync_tracker()&&accepted;
}

// Even if an integrator reader raises an SEH exception, release the local
// call guard. From then on the bridge always sees "Original". Never wait
// for other threads or access owner-only values before checking the thread.
__declspec(noinline) bool run(Operation operation,void* args) noexcept {
  const DWORD error=GetLastError();bool acquired=false,result=false;
  __try {
    __try {
      if(read(installed)!=1||read(fault))__leave;
      if(!own_thread()){block(Fault::thread);__leave;}
      if(InterlockedCompareExchange(&active,1,0)!=0){block(Fault::reentry);__leave;}
      acquired=true;result=implementation(operation,args);
      if(!healthy_internal())result=false;
    } __except(EXCEPTION_EXECUTE_HANDLER) {block(Fault::reader_exception);result=false;}
  } __finally {
    if(acquired)InterlockedExchange(&active,0);
    SetLastError(error);
  }
  return result;
}
void __cdecl capture(void*,const b::CaptureSite& site) { (void)run(Operation::capture,const_cast<b::CaptureSite*>(&site)); }
p::DisplayDecision __cdecl display(void*,const b::PrintSite& site) {
  return run(Operation::display,const_cast<b::PrintSite*>(&site))?p::DisplayDecision::suppress_owned:p::DisplayDecision::original;
}
void __cdecl destroy(void*,const b::DestructorSite& site) { (void)run(Operation::destroy,const_cast<b::DestructorSite*>(&site)); }

bool install_impl(std::uint32_t thread) {
  if(read(configured)!=1||!thread||thread!=GetCurrentThreadId()){block(Fault::configuration);return false;}
  InterlockedExchange(&owner,static_cast<LONG>(thread));
  const auto first=Backend::original();
  if(!first.valid){block(Fault::original_contract,first.issue);return false;}
  const std::array<std::uintptr_t,r::hook_count> destinations{{reinterpret_cast<std::uintptr_t>(&UbcAmmoCaptureBridge),
    reinterpret_cast<std::uintptr_t>(&UbcAmmoPrintBridge),reinterpret_cast<std::uintptr_t>(&UbcAmmoDestructorBridge)}};
  if(!Backend::prepare(destinations)){block(Fault::prepare);return false;}
  const b::Config configuration{thread,nullptr,Backend::continuation(0),
    reinterpret_cast<b::OriginalPrint>(Backend::continuation(1)),reinterpret_cast<b::OriginalDestructor>(Backend::continuation(2)),
    capture,display,destroy};
  if(!b::configure(configuration)||!bridge_healthy()){block(Fault::bridge);return false;}
  const auto before=Backend::original();
  if(!before.valid){block(Fault::original_contract,before.issue);return false;}
  if(!Backend::install(reinterpret_cast<std::uintptr_t>(&install))){block(Fault::install);return false;}
  // Establish provenance solely from our completed installation.
  for(std::size_t i=0;i<r::hook_count;++i)installed_hooks.hooks[i]={true,static_cast<std::uint32_t>(destinations[i]),
    static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(Backend::continuation(i)))};
  const auto after=Backend::installed(installed_hooks);
  if(!after.valid){block(Fault::installed_contract,after.issue);return false;}
  if(!bridge_healthy()){block(Fault::bridge);return false;}
  InterlockedExchange(&installed,1);return true;
}
} // namespace

bool configure(const Readers& value) noexcept {
  if(!value.context||!value.scope||read(attempt)||InterlockedCompareExchange(&configured,-1,0)!=0)return false;
  readers=value;InterlockedExchange(&configured,1);return true;
}
bool install(std::uint32_t thread) noexcept {
  const DWORD error=GetLastError();bool result=false;
  __try {
    __try {
      if(InterlockedCompareExchange(&attempt,1,0)!=0)__leave;
      result=install_impl(thread);
    } __except(EXCEPTION_EXECUTE_HANDLER) {block(Fault::reader_exception);}
  } __finally {SetLastError(error);}
  return result;
}
bool admit(const p::Job& job,const p::Admission& proof) noexcept {
  Admit args{&job,&proof};return run(Operation::admit,&args);
}
bool enter_effect(const p::Context& context,const p::Scope& scope) noexcept {
  Effect args{&context,&scope};return run(Operation::enter,&args);
}
bool close_effect(const p::Context& context,const p::Scope& scope) noexcept {
  Effect args{&context,&scope};return run(Operation::close,&args);
}
bool observe_result(const p::Result& value) noexcept {return run(Operation::result,const_cast<p::Result*>(&value));}
p::Notice take_notice() noexcept {p::Notice out{};if(!run(Operation::take,&out))out={};return out;}
bool ack_notice(std::uint64_t job,std::uint64_t receipt,bool registered) noexcept {
  Ack args{job,receipt,registered};return run(Operation::ack,&args);
}
bool healthy() noexcept {return own_thread()&&healthy_internal();}
bool permits_next() noexcept {
  return healthy()&&!read(active)&&!effect_entered&&tracker.status().stage==p::Stage::idle&&!tracker.status().blocked;
}
Status status() noexcept {
  Status out;out.configured=read(configured)==1;out.install_attempted=read(attempt)!=0;out.installed=read(installed)==1;
  out.fault=static_cast<Fault>(read(fault));out.runtime_issue=static_cast<r::Issue>(read(runtime_issue));
  out.owner_thread=own_thread();out.healthy=out.owner_thread&&healthy_internal();
  if(out.owner_thread&&!read(active)){
    out.effect_entered=effect_entered;out.tracker=tracker.status();out.capture=capture_diagnostic;
#if !defined(UBC_NATIVE_CRAFTING_AMMO_HOST)
    out.compatibility=Backend::reader.diagnostic();
#endif
  }
  return out;
}
} // namespace ubc::native_crafting_ammo
