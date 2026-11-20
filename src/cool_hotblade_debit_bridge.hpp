// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/cool_hotblade_debit.hpp"
#if defined(_WIN32) && defined(_M_IX86)
#include <windows.h>

namespace ubc::cool_hotblade_debit_bridge {
namespace debit = cool_hotblade_debit;
inline constexpr std::uintptr_t remove_entry = 0x7495f0;
inline constexpr std::uintptr_t restore_pop_entry = 0x7a4b20;
inline constexpr std::uintptr_t restore_instance_entry = 0x7a1550;
inline constexpr std::uintptr_t restore_item_entry = 0x7a2f20;
using Remove = void*(__thiscall*)(void*, int, int);
using RestorePop = void(__thiscall*)(void*, int);
using RestoreInstance = void(__cdecl*)(void*);
using RestoreItem = void(__thiscall*)(void*, int);
enum class ReadStage { before_debit, after_debit };
using Read = bool(__cdecl*)(void*, ReadStage, debit::Observation&);
using ReadRestore = bool(__cdecl*)(void*, const debit::Call&, debit::RestoreObservation&);
struct Config {
  std::uint32_t owner_thread = 0;
  void* context = nullptr;
  Remove remove = nullptr;
  RestorePop restore_pop = nullptr;
  RestoreInstance restore_instance = nullptr;
  RestoreItem restore_item = nullptr;
  Read read = nullptr;
  ReadRestore read_restore = nullptr;
};
enum class Outcome { rejected, debit_verified };
struct Diagnostics { bool configured = false, active = false, disabled = false, native_scope = false; };

// Isolated executor for exactly Lager2/N2. NO runtime authorization, hooks, effect
// admission or end-state machine. cool_hotblade_debit::runtime_ready remains
// false. configured indicates only a complete set of configured function pointers.
//
// Before run(), the existing tracker must have accepted arm() and
// original_returned(normal, operands_restored) on fresh actual snapshots. run()
// stays in the same exclusive writer and thread before station cleanup. Each reader
// must independently verify native code and module bindings, scope, objects and
// the entire EndFence. Zero counters alone do NOT establish excludes_dispatch.
// ReadRestore binds its own ITEM1682 contract. It does not bind the flask
// reader. Readers perform bounded memory reads without engine calls, allocation
// or I/O. All snapshots share a single strictly increasing serial source.
//
// The integrator binds only the four currently verified original functions.
// Synthetic hosts may use their own functions. One RemoveFromInv(hero,16612,1)
// call. Never dereference its opaque return value. Reread before each restore
// setter. No DataStack write or fallback. Native Remove SEH propagates after
// finally, and restore SEH cannot replace it. Uncertain effects permanently
// block execution. No retry or refund. The tracker and executor survive across
// subsequent orders. Destroy and normal completion remain separate requirements
// of the existing tracker/result contract and both need positive evidence.
class Executor {
public:
  Executor(debit::Tracker&, const Config&) noexcept;
  Executor(const Executor&) = delete;
  Executor& operator=(const Executor&) = delete;
  Diagnostics diagnostics() const noexcept;
  void disable() noexcept;
  Outcome run();
private:
  bool read(ReadStage, debit::Observation&) noexcept;
  bool read_restore(const debit::Call&, debit::RestoreObservation&, debit::RestorePlan&) noexcept;
  bool restore(const debit::Call&) noexcept;
  debit::Tracker& tracker_;
  Config config_{};
  bool configured_ = false;
  std::uint64_t last_serial_ = 0;
  mutable volatile LONG active_ = 0, disabled_ = 0, native_scope_ = 0;
};
} // namespace ubc::cool_hotblade_debit_bridge
#endif
