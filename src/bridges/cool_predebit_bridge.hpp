// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_predebit_admission.hpp"
#if defined(_WIN32) && defined(_M_IX86)
#include <windows.h>
#include "ubc/core/batch_amount_call.hpp"
#include "ubc/engine/batch_call_bridge.hpp"
#include "ubc/stations/cool/cool_predebit_journal.hpp"
#include "ubc/core/crafting_result.hpp"

namespace ubc::cool_predebit_bridge {
namespace debit=cool_predebit;
namespace reader=debit::reader;
using Remove=void*(__thiscall*)(void*,int,int);
using RestorePop=void(__thiscall*)(void*,int);
using RestoreInstance=void(__cdecl*)(void*);
using RestoreItem=void(__thiscall*)(void*,int);
// Only the owner can supply genuine writer, INFO and module leases and the
// current stack bounds. The executor checks their values on EVERY read.
// No engine calls, allocation, I/O or input in the boundary callback.
using Boundary=bool(__cdecl*)(void*,reader::Phase,reader::Input&);
struct Config {
  std::uint32_t owner_thread=0;
  ReadOnlyMemory memory{};
  void* context=nullptr;
  Boundary boundary=nullptr;
  Remove remove=nullptr;
  RestorePop restore_pop=nullptr;
  RestoreInstance restore_instance=nullptr;
  RestoreItem restore_item=nullptr;
};
struct Request {
  debit::Job job{};std::uint64_t frame=0;const AmountPatch* patch=nullptr;
  const debit::admission::Prepared* prepared=nullptr;
  const debit::admission::InfoLease* info_lease=nullptr;
  debit::Rejection* rejection=nullptr;
};
struct Diagnostics {bool configured=false,active=false,disabled=false,native_scope=false;};

// Immediately before memory access. Loss of ownership blocks the forward
// swap. Restoration still requires the bound target and our own patch bytes.
template<class CurrentInfo>
bool permits_operand_replace(const debit::admission::Prepared& prepared,const debit::admission::InfoLease& info,
    const batch_call::WriterScope* writer,const debit::DestroyJournal& journal,std::uint64_t fence,
    std::uint32_t address,const void* before,const void* after,std::size_t size,CurrentInfo current_info) noexcept {
  if(!prepared.valid()||size!=sizeof(std::uint32_t)||address!=prepared.body_address()+prepared.amount().operands[0].offset)return false;
  std::uint32_t prior=0,next=0;
  std::memcpy(&prior,before,size);std::memcpy(&next,after,size);
  if(next==1)return prior==debit::quantity;
  const auto& job=prepared.job();std::uint32_t started=0,completed=0;
  return prior==1&&next==debit::quantity&&current_info()==job.actual_info&&prepared.current(info)&&
    writer&&writer->token==job.writer_token&&batch_call::writer_undisturbed(*writer)&&
    fence==job.fence_token&&journal.sample(fence,started,completed);
}

// One exclusive writer holds the result guard and the same INFO lease across
// Extra, Original and AFTER. The amount finally block covers both native calls.
// Once entered, Extra can have a partial effect even with zero original calls.
class Executor {
public:
  Executor(debit::Tracker&,const Config&) noexcept;
  Executor(const Executor&)=delete;Executor& operator=(const Executor&)=delete;
  Diagnostics diagnostics()const noexcept;
  void disable()noexcept;
  void* run(batch_call::WriterScope&,AmountPatch&,const AmountMemory&,const Request&,
            CraftingResult&,const CraftingRequest&,const CraftingResultBinding&);
  bool extra_entered()const noexcept{return extra_entered_;}
  bool operands_restored()const noexcept{return operands_restored_;}
  bool original_entered()const noexcept{return original_entered_;}
  bool original_returned()const noexcept{return original_returned_;}
  reader::Issue read_issue()const noexcept{return read_issue_;} // Owner thread only.
private:
  static void* __cdecl effect(void*);
  bool extra();
  bool boundary(reader::Phase,reader::Input&)noexcept;
  bool capture(reader::Phase,const debit::Call*,reader::Capture&)noexcept;
  bool restore_read(const debit::Call&,debit::RestoreObservation&,debit::old::RestorePlan&,
    debit::Observation* before_remove=nullptr)noexcept;
  bool restore(const debit::Call&)noexcept;
  bool unavailable()const noexcept;
  debit::Tracker& tracker_;const Config config_;
  reader::Reader reader_;
  Request request_{};std::uint64_t last_serial_=0;
  debit::admission::Prepared prepared_{};
  reader::Issue read_issue_=reader::Issue::none;
  batch_call::WriterScope* writer_=nullptr;
  bool configured_=false,extra_entered_=false,original_entered_=false,original_returned_=false,operands_restored_=false;
  alignas(16) unsigned char effect_fx_[512]{};
  mutable volatile LONG active_=0,disabled_=0,native_scope_=0;
};
} // namespace ubc::cool_predebit_bridge
#endif
