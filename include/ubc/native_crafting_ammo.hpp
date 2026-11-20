// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/ammo_presentation.hpp"
#include "ubc/ammo_runtime.hpp"
#include "ubc/ammo_compat_memory.hpp"

#if defined(_WIN32) && defined(_M_IX86)
namespace ubc::native_crafting_ammo {
namespace presentation = ammo_presentation;
using ContextReader = bool(__cdecl*)(void*,presentation::Context&);
using ScopeReader = bool(__cdecl*)(void*,presentation::Scope&);
struct Readers {
  void* user = nullptr;
  ContextReader context = nullptr;
  ScopeReader scope = nullptr;
};
enum class Fault : std::uint32_t {
  none, configuration, thread, reentry, original_contract, prepare,
  bridge, install, installed_contract, reader_exception, context, scope, tracker
};
// Diagnostics only, no capture/permission evidence. Reset only after
// successful new admission. Counters saturate, not overflow. The
// sample shows the first reader value; the first erroneous reader value
// replaces one previously successful value once and persists thereafter.
struct CaptureDiagnostic {
  std::uint64_t job=0,scope_token=0;
  std::uint32_t callback=0,attempts=0,reads=0,accepted=0;
  bool reader_seen=false,reader_valid=false;
  ammo_runtime::Issue first_issue=ammo_runtime::Issue::none;
  std::uint32_t vm_pc=0,expected_pc=0,caller=0,expected_caller=0;
};
struct Status {
  bool configured = false, install_attempted = false, installed = false;
  bool healthy = false, owner_thread = false, effect_entered = false;
  Fault fault = Fault::none;
  ammo_runtime::Issue runtime_issue = ammo_runtime::Issue::none;
  ammo_compat_memory::Diagnostic compatibility{};
  presentation::Status tracker{};
  CaptureDiagnostic capture{};
};

// A single process adapter. The reader registry and all three
// bridges remain valid after configure/install until process end; no
// reset/unload. Readers copy only current, bounded own values on the
// owner thread: no engine calls, writes, allocation, I/O, or reentry.
// context provides the current world/hero/parser binding plus epoch.
// scope provides the ACTUALLY active own quantity-patch call; without
// an active call the known answer is Scope{}. No last-job fallback.
bool configure(const Readers&) noexcept;

// Only explicitly in the idle own sawing-station dialog before the first
// own surface, never DllMain or the still-incomplete world initialization.
// The integrator has verified the fixed engine/DLL target contract
// beforehand. One attempt: full original guards, trampolines, bridge
// configuration, then the three writes and full installed guards. No log
// or file I/O. On error own ammunition stays locked. Already reachable
// bridges are always configured and execute only their original path.
bool install(std::uint32_t owner_thread) noexcept;

// native_contract/lifetime_observers of the admission are derived
// here from the own installed guards. The three remaining evidences
// come from the immediately confirmed recipe/material/UI adapter.
// Admission occurs BEFORE the patch with known inactive scope.
bool admit(const presentation::Job&,const presentation::Admission&) noexcept;
// After successful patch check, before exactly one original effect.
// The scope stays active until close_effect. patched_body_verified
// attests the just-used own patch transaction, not an old job copy.
bool enter_effect(const presentation::Context&,const presentation::Scope&) noexcept;
bool close_effect(const presentation::Context&,const presentation::Scope&) noexcept;
// Result only after confirmed material/station completion AND restore.
// The brought bridge_healthy evidence is additionally linked with
// the current real bridge state AND the renewed native contract.
bool observe_result(const presentation::Result&) noexcept;
presentation::Notice take_notice() noexcept;
// Only with receipt after genuine successful native text submit.
bool ack_notice(std::uint64_t job,std::uint64_t ui_receipt,bool registered) noexcept;

// Not replacing native RAM verification: this occurs at every
// admission/effect/receipt point and at the bound native callbacks.
// Foreign threads receive no unprotected copy of the owner state.
bool healthy() noexcept;
bool permits_next() noexcept;
Status status() noexcept;
} // namespace ubc::native_crafting_ammo
#endif
