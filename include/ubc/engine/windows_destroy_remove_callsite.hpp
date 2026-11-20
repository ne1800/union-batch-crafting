// SPDX-License-Identifier: MIT
#pragma once
#if !defined(_WIN32) || !defined(_M_IX86)
#error Dieser Adapter verlangt Windows x86 und die MSVC-thiscall-ABI.
#endif
#include <windows.h>
#include <intrin.h>
#include <cstring>
#include "ubc/engine/destroy_remove_callsite.hpp"
#include "ubc/engine/windows_probe.hpp"
#include "ubc/stations/alchemy/alchemy_flask_runtime.hpp"

namespace ubc::windows_destroy_remove_callsite {
namespace contract = destroy_remove_callsite;
static_assert(sizeof(void*) == 4 && sizeof(int) == 4, "32-Bit-Windows-ABI");
// Pinned Gothic_II_Addon/API/oInventory.h: Remove(int,int) -> oCItem*.
// This=ECX; Instance, count on stack; callee removes 8 bytes; result=EAX.
using Remove = void*(__thiscall*)(void*, std::int32_t, std::int32_t);
using Before = std::uint64_t(__cdecl*)(void*, const contract::Invocation&);
using After = void(__cdecl*)(void*, const contract::Invocation&, std::uint64_t, std::uint32_t);
struct Observer {
  void* context = nullptr;
  Before before = nullptr;
  After after = nullptr;
};
// before must bind its own current job; 0 means no observation. No
// engine call, no input, no I/O and no allocation in observers. after
// receives only its own value copies and the opaque return. No further
// Dereference of old item/hand/inventory pointers after the original!
struct Diagnostic {
  LONG invocations = 0, observations = 0, returned = 0, observer_faults = 0;
};
enum class Issue {
  none,
  lifecycle,
  image,
  context,
  native_code,
  pin,
  protect,
  flush,
  foreign,
  busy
};

// No automatic install path. Root must bind the idle thread, further
// Destroy/job/end contracts and lifetime of the static adapter.
// Patch/Unpatch are NOT atomic; they require exclusive idle execution.
class Adapter {
public:
  Adapter() noexcept = default;
  Adapter(const Adapter&) = delete;
  Adapter& operator=(const Adapter&) = delete;
#if defined(UBC_DESTROY_REMOVE_HOST_TEST)
  // Only separately compiled own host, no runtime switch in the plugin.
  Adapter(std::uint32_t site, std::uint32_t table, std::uint32_t original) noexcept
      : site_(site), admission_{site + 6, table, original, 0}, host_(true) {}
#endif
  bool prepare(DWORD owner_thread, Observer observer,
               const alchemy_flask_runtime::NativeCodePolicy& policy = {}) noexcept {
    const auto error = GetLastError();
    bool ok = false;
    __try {
      if (prepared_ || attempted_ || active_ || !owner_thread || !observer.before ||
          !observer.after) {
        issue_ = Issue::lifecycle;
        __leave;
      }
      attempted_ = true;
      policy_ = policy;
      if (!image_and_native())
        __leave;
      plan_ = contract::plan(site_, reinterpret_cast<std::uint32_t>(&entry));
      if (!matches(false)) {
        issue_ = Issue::context;
        __leave;
      }
      admission_.thread = owner_thread;
      observer_ = observer;
      prepared_ = ok = true;
      issue_ = Issue::none;
    } __finally {
      SetLastError(error);
    }
    return ok;
  }
  bool install() noexcept {
    const auto error = GetLastError();
    bool ok = false;
    __try {
      if (!prepared_ || install_attempted_ || active_) {
        issue_ = Issue::lifecycle;
        __leave;
      }
      install_attempted_ = true;
      if (!image_and_native())
        __leave;
      if (!matches(false)) {
        issue_ = Issue::context;
        __leave;
      }
      HMODULE pinned = nullptr;
      if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                  GET_MODULE_HANDLE_EX_FLAG_PIN,
                              reinterpret_cast<LPCWSTR>(&entry), &pinned)) {
        issue_ = Issue::pin;
        __leave;
      }
      auto* bytes = reinterpret_cast<void*>(site_);
      DWORD old = 0;
      if (!VirtualProtect(bytes, 6, PAGE_EXECUTE_READWRITE, &old)) {
        issue_ = Issue::protect;
        __leave;
      }
      original_protection_ = old;
      if (!matches(false)) {
        DWORD ignored = 0;
        VirtualProtect(bytes, 6, old, &ignored);
        issue_ = Issue::foreign;
        __leave;
      }
      active_ = this; // visible before, before its own CALL becomes reachable
      std::memcpy(bytes, plan_.bytes.data(), 6);
      written_ = true;
      DWORD ignored = 0;
      const bool restored = VirtualProtect(bytes, 6, old, &ignored) != FALSE;
      const bool flushed = FlushInstructionCache(GetCurrentProcess(), bytes, 6) != FALSE;
      if (!restored || !flushed || !matches(true)) {
        issue_ = !restored ? Issue::protect : !flushed ? Issue::flush : Issue::foreign;
        const auto failed = issue_;
        if (rollback())
          issue_ = failed;
        __leave;
      }
      issue_ = Issue::none;
      ok = true;
    } __finally {
      SetLastError(error);
    }
    return ok;
  }
  bool remove() noexcept {
    const auto error = GetLastError();
    bool ok = false;
    __try {
      if (!written_ || active_ != this) {
        issue_ = Issue::lifecycle;
        __leave;
      }
      if (InterlockedCompareExchange(&in_flight_, 0, 0)) {
        issue_ = Issue::busy;
        __leave;
      }
      ok = rollback();
    } __finally {
      SetLastError(error);
    }
    return ok;
  }
  // Patch ownership only, no effect/job/engine health evidence.
  bool owns_patch() const noexcept {
    const auto error = GetLastError();
    const bool result = written_ && active_ == this && matches(true);
    SetLastError(error);
    return result;
  }
  bool retained() const noexcept { return written_ || active_ == this; }
  Issue issue() const noexcept { return issue_; }
  Diagnostic diagnostic() const noexcept { return diagnostic_; } // only in the idle thread
  LONG in_flight() noexcept { return InterlockedCompareExchange(&in_flight_, 0, 0); }
  contract::Plan patch_plan() const noexcept { return plan_; }

private:
  inline static Adapter* active_ = nullptr;
  std::uint32_t site_ = contract::address;
  contract::Admission admission_{};
  contract::Plan plan_{};
  Observer observer_{};
  Diagnostic diagnostic_{};
  alchemy_flask_runtime::NativeCodePolicy policy_{};
  volatile LONG in_flight_ = 0;
  DWORD original_protection_ = 0;
  Issue issue_ = Issue::none;
  bool attempted_ = false, prepared_ = false, install_attempted_ = false, written_ = false;
#if defined(UBC_DESTROY_REMOVE_HOST_TEST)
  bool host_ = false;
#endif
  bool engine_image() const noexcept {
#if defined(UBC_DESTROY_REMOVE_HOST_TEST)
    return !host_;
#else
    return true;
#endif
  }
  bool image_and_native() noexcept {
#if defined(UBC_DESTROY_REMOVE_HOST_TEST)
    if (host_)
      return true;
#endif
#ifdef UBC_PLAYER_RELEASE
    // The release adapter is reached only after the station's disk-site admission.
    const bool valid_image = windows_probe::player_image_header();
#else
    const bool valid_image = windows_probe::target_matches();
#endif
    if (!valid_image) {
      issue_ = Issue::image;
      return false;
    }
    const ReadOnlyMemory memory{nullptr, &windows_probe::copy_memory};
    const auto proof = alchemy_flask_runtime::verify_native(memory, policy_);
    if (!proof.reviewed_ranges_match || !proof.vtable_slots_match ||
        proof.issue != alchemy_flask_runtime::Issue::none) {
      issue_ = Issue::native_code;
      return false;
    }
    return true;
  }
  bool matches(bool owned) const noexcept {
    std::array<std::uint8_t, 25> a{}, b{};
    const auto start = site_ - static_cast<std::uint32_t>(contract::prefix_size);
    if (!plan_.valid ||
        !windows_probe::readable(reinterpret_cast<const void*>(start), a.size(), true,
                                 engine_image()) ||
        !windows_probe::copy_memory(nullptr, start, a.data(), a.size()) ||
        !windows_probe::copy_memory(nullptr, start, b.data(), b.size()) || a != b)
      return false;
    return contract::context_matches(a, plan_, owned);
  }
  bool rollback() noexcept {
    if (InterlockedCompareExchange(&in_flight_, 0, 0)) {
      issue_ = Issue::busy;
      return false;
    }
    if (!written_ || active_ != this || !matches(true)) {
      issue_ = Issue::foreign;
      return false;
    }
    auto* bytes = reinterpret_cast<void*>(site_);
    DWORD old = 0;
    if (!VirtualProtect(bytes, 6, PAGE_EXECUTE_READWRITE, &old)) {
      issue_ = Issue::protect;
      return false;
    }
    if (!matches(true)) {
      DWORD ignored = 0;
      VirtualProtect(bytes, 6, old, &ignored);
      issue_ = Issue::foreign;
      return false;
    }
    std::memcpy(bytes, contract::original_context.data() + contract::prefix_size, 6);
    DWORD ignored = 0;
    const bool restored = VirtualProtect(bytes, 6, original_protection_, &ignored) != FALSE;
    const bool flushed = FlushInstructionCache(GetCurrentProcess(), bytes, 6) != FALSE;
    if (!restored || !flushed || !matches(false)) {
      // Code may already be original; no second unchecked write action.
      issue_ = !restored ? Issue::protect : !flushed ? Issue::flush : Issue::foreign;
      return false;
    }
    written_ = false;
    active_ = nullptr;
    issue_ = Issue::none;
    return true;
  }
  static std::uint64_t before(Adapter& self, const contract::Invocation& input) noexcept {
    __try {
      return self.observer_.before(self.observer_.context, input);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      InterlockedIncrement(&self.diagnostic_.observer_faults);
      return 0;
    }
  }
  static void after(Adapter& self, const contract::Invocation& input, std::uint64_t token,
                    std::uint32_t result) noexcept {
    __try {
      self.observer_.after(self.observer_.context, input, token, result);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      InterlockedIncrement(&self.diagnostic_.observer_faults);
    }
  }
  __declspec(noinline) static void* __fastcall entry(void* inventory, void* loaded_vtable,
                                                     std::int32_t instance, std::int32_t amount) {
    const DWORD incoming_error = GetLastError();
    // Exactly the slot loaded on entry in EDX. No second
    // inv->vtable and no fallback to a known but not actual call.
    const auto original = *reinterpret_cast<Remove*>(
        reinterpret_cast<std::uintptr_t>(loaded_vtable) + contract::slot);
    const contract::Invocation input{reinterpret_cast<std::uint32_t>(inventory),
                                     reinterpret_cast<std::uint32_t>(loaded_vtable),
                                     reinterpret_cast<std::uint32_t>(original),
                                     reinterpret_cast<std::uint32_t>(_ReturnAddress()),
                                     GetCurrentThreadId(),
                                     instance,
                                     amount};
    Adapter* const self = active_; // Process lifetime/idle Unpatch are mandatory.
    const LONG depth = self ? InterlockedIncrement(&self->in_flight_) : 0;
    void* result = nullptr;
    __try {
      std::uint64_t token = 0;
      if (self)
        InterlockedIncrement(&self->diagnostic_.invocations);
      if (self && depth == 1 && contract::observable(input, self->admission_)) {
        token = before(*self, input);
        if (token)
          InterlockedIncrement(&self->diagnostic_.observations);
      }
      SetLastError(incoming_error);
      result = original(inventory, instance, amount); // exactly once, even without own job
      const DWORD original_error = GetLastError();
      if (token) {
        InterlockedIncrement(&self->diagnostic_.returned);
        after(*self, input, token, reinterpret_cast<std::uint32_t>(result));
      }
      SetLastError(original_error);
    } __finally {
      // Original SEH is NOT caught/converted; no after on Unwind.
      const DWORD error = GetLastError();
      if (self)
        InterlockedDecrement(&self->in_flight_);
      SetLastError(error);
    }
    return result;
  }
};
} // namespace ubc::windows_destroy_remove_callsite
