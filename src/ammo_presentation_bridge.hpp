// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/ammo_presentation.hpp"
#include <cstdint>

#if defined(_WIN32) && defined(_M_IX86)
namespace ubc::ammo_presentation_bridge {
inline constexpr std::uintptr_t capture_site = 0x6e320a;
inline constexpr std::uintptr_t capture_resume = 0x6e3210;
inline constexpr std::uintptr_t print_entry = 0x759270;
inline constexpr std::uintptr_t destructor_entry = 0x76a4a0;
// Capture ESP: the external function also saved ESI. The actual
// dispatcher return address is therefore at +0x5C here (previously +0x58).
inline constexpr std::size_t external_return_stack_offset = 0x5c;

struct CaptureSite {
  std::uint32_t thread = 0, parser = 0, message = 0, event_manager = 0, stack = 0;
};
struct PrintSite {
  std::uint32_t thread = 0, receiver = 0, message = 0, caller = 0;
};
struct DestructorSite {
  std::uint32_t thread = 0, message = 0, caller = 0;
};
using CaptureObserver = void(__cdecl*)(void*,const CaptureSite&);
using PrintObserver = ammo_presentation::DisplayDecision(__cdecl*)(void*,const PrintSite&);
using DestructorObserver = void(__cdecl*)(void*,const DestructorSite&);
using OriginalPrint = int(__thiscall*)(void*,void*);
using OriginalDestructor = void(__thiscall*)(void*);
struct Config {
  std::uint32_t owner_thread = 0;
  void* context = nullptr;
  // The integrator supplies ready-to-use trampolines and function pointers. Capture
  // must execute exactly the entire skipped instruction and reach +0x6E3210.
  // No installation, byte writes, allocation or original calls in Config.
  void* capture_continuation = nullptr;
  OriginalPrint original_print = nullptr;
  OriginalDestructor original_destructor = nullptr;
  CaptureObserver capture = nullptr;
  PrintObserver print = nullptr;
  DestructorObserver destroy = nullptr;
};
enum class Fault : std::uint32_t { none, foreign_thread, reentry, observer_exception, invalid_decision, disabled };
struct Diagnostics {
  bool configured = false, disabled = false, observer_active = false;
  Fault fault = Fault::none;
};
// Configure once BEFORE installation, with no bridge running. The registry
// is copied and cannot be replaced for the lifetime of the process.
// Calling the naked bridges without successful configuration is invalid.
// Callbacks may use only bounded local values and ReadOnlyMemory. No
// engine calls, allocation or forwarding the original call. Catch callback
// exceptions locally, permanently disable our presentation and forward
// the original. Original exceptions are unchanged. All three original
// paths tail-jump with their original stack, SEH and return address.
//
// A foreign thread or observer reentry disables our suppression.
// Third-party game code is never stopped. This does not establish
// the normal game thread or guarantee safety against arbitrary
// concurrent third-party plugins. Include diagnostics().disabled in
// the tracker's admission, result and UI acknowledgment. Otherwise a
// discarded observer decision could count as successful suppression.
bool configure(const Config&) noexcept;
Diagnostics diagnostics() noexcept;
void disable() noexcept;
} // namespace ubc::ammo_presentation_bridge
extern "C" {
void UbcAmmoCaptureBridge();
int __thiscall UbcAmmoPrintBridge(void* receiver,void* message);
void __thiscall UbcAmmoDestructorBridge(void* message);
}
#endif
