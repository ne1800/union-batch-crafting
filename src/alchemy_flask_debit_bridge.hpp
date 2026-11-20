// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/alchemy_flask_debit.hpp"
#include "ubc/alchemy_extra_dependencies.hpp"
#if defined(_WIN32) && defined(_M_IX86)
#include <windows.h>

namespace ubc::alchemy_flask_debit_bridge {
namespace debit=alchemy_flask_debit;
inline constexpr std::uintptr_t remove_entry=0x7495f0;
inline constexpr std::uintptr_t restore_pop_entry=0x7a4b20;
inline constexpr std::uintptr_t restore_instance_entry=0x7a1550;
inline constexpr std::uintptr_t restore_item_entry=0x7a2f20;
using Remove=void*(__thiscall*)(void*,int,int);
using RestorePop=void(__thiscall*)(void*,int);
using RestoreInstance=void(__cdecl*)(void*);
using RestoreItem=void(__thiscall*)(void*,int);
enum class ReadStage { before_debit, after_debit };
using Read=bool(__cdecl*)(void*,ReadStage,debit::Observation&);
using ReadRestore=bool(__cdecl*)(void*,const debit::Call&,debit::RestoreObservation&);
struct ItemObservation {
  alchemy_extra_dependencies::ItemBinding item{};
  debit::Context context{};
  debit::Scope scope{};
  bool native_set_offset_contract=false;
};
using ReadItem=bool(__cdecl*)(void*,const debit::Call&,ItemObservation&);
struct Config {
  std::uint32_t owner_thread=0;
  void* context=nullptr;
  Remove remove=nullptr;
  RestorePop restore_pop=nullptr;
  RestoreInstance restore_instance=nullptr;
  Read read=nullptr;
  ReadRestore read_restore=nullptr;
  RestoreItem restore_item=nullptr;
  ReadItem read_item=nullptr;
};
enum class Outcome { rejected, no_extra_debit, verified };
struct Diagnostics { bool ready=false, active=false, disabled=false, native_scope=false; };

// No hook, installer or fixed engine pointers in the execution path. The
// integrator binds and verifies the four native functions above and all
// object and script guards. Test hosts can bind their own synthetic functions.
// read/read_restore may only provide bounded memory reads and values. No
// engine calls or allocation. Their SEH exceptions are caught locally.
//
// run() executes AFTER exactly one successful original call and quantity
// restoration, still in the same exclusive writer/TLS and before station
// cleanup. Both readers keep the physical station, thread and original
// callback bound. Native Remove exceptions propagate after finally. No
// retry or refund. Never use the returned item pointer. Record restoration
// exceptions and block further execution without replacing an original
// exception. Keep the tracker and executor for all subsequent orders. Recreating
// them after an uncertain effect does not authorize further execution.
class Executor {
public:
  Executor(debit::Tracker&,const Config&) noexcept;
  Executor(const Executor&)=delete;
  Executor& operator=(const Executor&)=delete;
  Diagnostics diagnostics() const noexcept;
  void disable() noexcept;
  Outcome run();
private:
  bool read(ReadStage,debit::Observation&) noexcept;
  bool can_restore(const debit::Call&) noexcept;
  bool read_item(const debit::Call&,ItemObservation&) noexcept;
  bool restore_item(const debit::Call&,const alchemy_extra_dependencies::ItemBinding&) noexcept;
  bool restore(const debit::Call&,const alchemy_extra_dependencies::ItemBinding&) noexcept;
  debit::Tracker& tracker_;
  Config config_{};
  bool ready_=false;
  mutable volatile LONG active_=0,disabled_=0,native_scope_=0;
};
} // namespace ubc::alchemy_flask_debit_bridge
#endif
