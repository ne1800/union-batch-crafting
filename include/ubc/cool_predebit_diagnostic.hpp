// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/cool_predebit_reader.hpp"

namespace ubc::cool_predebit {
// Stable log codes for the check points outside the composed reader.
enum class Gate {writer_scope=1,handoff,executor_phase,prepared,operand,context,page,actor,
  dispatch,modules,hooks,journal,serial,late_gate,reader_missing,closing,executor_entry,
  capture_open,capture_close,admission_entry,info_lease,read_exception};

// Record only values belonging to the order. The INFO writes them to the log after the writer.
struct Rejection {
  const char* event=nullptr;
  reader::Phase phase=reader::Phase::admission;
  int reason=0;
  std::uint32_t address=0;
  bool native_restore_failed=false;
  check::Sample failure{};

  bool gate(reader::Phase at,Gate why,std::uint32_t where=0) noexcept {
    record("cool_predebit_gate_rejected",at,static_cast<int>(why),where);return false;
  }
  void reader_failed(reader::Phase at,const reader::Capture& capture) noexcept {
    if(!event)failure=capture.failure;
    record("cool_predebit_reader_rejected",at,static_cast<int>(capture.issue),capture.failed_address);
  }
  void tracker_failed(reader::Phase at,Issue issue) noexcept {
    record("cool_predebit_tracker_rejected",at,static_cast<int>(issue),0);
  }
private:
  void record(const char* name,reader::Phase at,int why,std::uint32_t where) noexcept {
    if(!event){event=name;phase=at;reason=why;address=where;}
  }
};
} // namespace ubc::cool_predebit
