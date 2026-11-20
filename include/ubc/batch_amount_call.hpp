// SPDX-License-Identifier: MIT
#pragma once
#include <windows.h>
#include "ubc/amount_patch.hpp"

namespace ubc {
struct AmountOutcome {
  bool entered = false, restored = false;
  AmountIssue issue = AmountIssue::none;
};
using AmountEffect = void*(__cdecl*)(void*);

// The entire mutating window lies in finally, including begin
// itself. The context of the memory adapter must survive the call.
// No logging, allocation, or UI between first exchange and return.
inline void* run_amount_call(AmountPatch& patch, const AmountMemory& memory,
    std::uint32_t address, const AmountBody& body, std::uint32_t quantity,
    AmountEffect effect, void* context, bool& restore_verified, AmountOutcome& outcome) {
  void* result = nullptr;
  DWORD after = GetLastError();
  restore_verified = false;
  outcome = {};
  __try {
    if (patch.begin(memory, address, body, quantity)) {
      outcome.entered = true;
      SetLastError(after);
      result = effect(context);
      after = GetLastError();
    } else outcome.issue = patch.issue();
  } __finally {
    if (AbnormalTermination()) after = GetLastError();
    restore_verified = (!patch.active() && !patch.mutation_attempted()) || patch.finish();
    outcome.restored = restore_verified;
    SetLastError(after);
  }
  return result;
}
} // namespace ubc
