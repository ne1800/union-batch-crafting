// SPDX-License-Identifier: MIT
#include <windows.h>
#include "ammo_presentation_bridge.hpp"

#if !defined(_WIN32) || !defined(_M_IX86)
#error Die Praesentationsbridge benoetigt Windows x86.
#endif

extern "C" {
// Written only by configure before installation.
void* UbcAmmoCaptureContinuation = nullptr;
void* UbcAmmoPrintOriginal = nullptr;
void* UbcAmmoDestructorOriginal = nullptr;
}
namespace {
namespace b = ubc::ammo_presentation_bridge;
b::Config configuration{};
volatile LONG ready = 0, disabled = 0, active = 0, fault = 0;
LONG read(volatile LONG& value) noexcept { return InterlockedCompareExchange(&value,0,0); }
void block(b::Fault reason) noexcept {
  InterlockedCompareExchange(&fault,static_cast<LONG>(reason),0);
  InterlockedExchange(&disabled,1);
}
struct Registers {
  std::uint32_t edi,esi,ebp,saved_esp,ebx,edx,ecx,eax,eflags;
};
static_assert(sizeof(Registers)==36,"PUSHFD/PUSHAD frame");
}
namespace ubc::ammo_presentation_bridge {
bool configure(const Config& config) noexcept {
  if (!config.owner_thread || !config.capture_continuation || !config.original_print || !config.original_destructor ||
      !config.capture || !config.print || !config.destroy || read(ready)) return false;
  if (InterlockedCompareExchange(&ready,-1,0)!=0) return false;
  configuration=config;
  UbcAmmoCaptureContinuation=config.capture_continuation;
  UbcAmmoPrintOriginal=reinterpret_cast<void*>(config.original_print);
  UbcAmmoDestructorOriginal=reinterpret_cast<void*>(config.original_destructor);
  InterlockedExchange(&ready,1);
  return true;
}
Diagnostics diagnostics() noexcept {
  return {read(ready)==1,read(disabled)!=0,read(active)!=0,static_cast<Fault>(read(fault))};
}
void disable() noexcept { block(Fault::disabled); }
}

extern "C" {
// Observers receive only copies of the native registers and arguments,
// never the writable saved state. The caller saves the FXSAVE state before
// entering C++. Return 1 only for an affirmative print decision of our own.
__declspec(noinline) unsigned __cdecl UbcAmmoObserve(unsigned kind,const Registers* registers) {
  const DWORD error=GetLastError();
  unsigned suppress=0; bool acquired=false;
  __try {
    __try {
      if (read(ready)!=1 || read(disabled)) __leave;
      const auto thread=GetCurrentThreadId();
      if (thread!=configuration.owner_thread) { block(b::Fault::foreign_thread); __leave; }
      if (InterlockedCompareExchange(&active,1,0)!=0) { block(b::Fault::reentry); __leave; }
      acquired=true;
      const auto stack=reinterpret_cast<std::uintptr_t>(registers)+sizeof(Registers);
      if (kind==0) {
        const b::CaptureSite site{thread,registers->edi,registers->esi,registers->eax,static_cast<std::uint32_t>(stack)};
        configuration.capture(configuration.context,site);
      } else if (kind==1) {
        const auto* entry=reinterpret_cast<const std::uint32_t*>(stack);
        const b::PrintSite site{thread,registers->ecx,entry[1],entry[0]};
        const auto decision=configuration.print(configuration.context,site);
        if (decision==ubc::ammo_presentation::DisplayDecision::suppress_owned) suppress=1;
        else if (decision!=ubc::ammo_presentation::DisplayDecision::original) block(b::Fault::invalid_decision);
      } else if (kind==2) {
        const b::DestructorSite site{thread,registers->ecx,*reinterpret_cast<const std::uint32_t*>(stack)};
        configuration.destroy(configuration.context,site);
      } else block(b::Fault::invalid_decision);
      if (read(disabled)) suppress=0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
      block(b::Fault::observer_exception); suppress=0;
    }
  } __finally {
    if (acquired) InterlockedExchange(&active,0);
    SetLastError(error);
  }
  return suppress;
}

// At all three points restore the exact integer registers, EFLAGS, x87, MMX,
// XMM, MXCSR and stack state before the unchanged original transfer. CLD
// applies only to our C++ observer. Its LastError and SEH side effects also
// remain local. Capture changes neither the arguments nor the queue operation.
__declspec(naked) void UbcAmmoCaptureBridge() {
  __asm {
    pushfd
    pushad
    cld
    mov ebx,esp
    sub esp,544
    and esp,-16
    fxsave [esp+16]
    mov dword ptr [esp+4],ebx
    mov dword ptr [esp],0
    call UbcAmmoObserve
    fxrstor [esp+16]
    mov esp,ebx
    popad
    popfd
    jmp dword ptr [UbcAmmoCaptureContinuation]
  }
}
__declspec(naked) int __thiscall UbcAmmoPrintBridge(void*,void*) {
  __asm {
    pushfd
    pushad
    cld
    mov ebx,esp
    sub esp,544
    and esp,-16
    fxsave [esp+16]
    mov dword ptr [esp+4],ebx
    mov dword ptr [esp],1
    call UbcAmmoObserve
    cmp eax,1
    je own_print
    fxrstor [esp+16]
    mov esp,ebx
    popad
    popfd
    jmp dword ptr [UbcAmmoPrintOriginal]
  own_print:
    fxrstor [esp+16]
    mov esp,ebx
    popad
    popfd
    mov eax,1
    ret 4
  }
}
__declspec(naked) void __thiscall UbcAmmoDestructorBridge(void*) {
  __asm {
    pushfd
    pushad
    cld
    mov ebx,esp
    sub esp,544
    and esp,-16
    fxsave [esp+16]
    mov dword ptr [esp+4],ebx
    mov dword ptr [esp],2
    call UbcAmmoObserve
    fxrstor [esp+16]
    mov esp,ebx
    popad
    popfd
    jmp dword ptr [UbcAmmoDestructorOriginal]
  }
}
}
