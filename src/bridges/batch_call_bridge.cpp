// SPDX-License-Identifier: MIT
#include <windows.h>
#include <intrin.h>
#include <atomic>
#include "ubc/engine/batch_call_bridge.hpp"

#if !defined(_WIN32) || !defined(_M_IX86)
#error Der CallFunc-Adapter benoetigt Windows x86.
#endif

namespace {
namespace b = ubc::batch_call;
b::Config config;
volatile LONG ready = 0, gate = 0, disabled = 0;
volatile LONG blocked[b::max_symbols]{};
// Read and write only under the exclusive writer gate.
std::uint64_t writer_sequence = 0;
std::uint32_t writer_rejection_floor = UINT32_MAX;
std::atomic<std::uint32_t> rejected[b::max_symbols]{};
constexpr LONG writer_bit = static_cast<LONG>(0x80000000u);
const int denied_value = 0;

LONG atomic_read(volatile LONG& value) noexcept { return InterlockedCompareExchange(&value, 0, 0); }
int target_index(std::int32_t symbol) noexcept {
  for (std::size_t i = 0; i < config.symbol_count; ++i)
    if (config.symbols[i] == symbol)
      return static_cast<int>(i);
  return -1;
}
void denied_by_writer(int index) noexcept {
  auto& counter = rejected[index];
  auto value = counter.load();
  while (value != UINT32_MAX && !counter.compare_exchange_weak(value, value + 1)) {
  }
}
bool acquire_reader(int index) noexcept {
  LONG before = atomic_read(gate);
  for (;;) {
    if (before == writer_bit) {
      denied_by_writer(index);
      return false;
    }
    if (before < 0 || before == 0x7fffffffL)
      return false;
    const LONG observed = InterlockedCompareExchange(&gate, before + 1, before);
    if (observed == before)
      return true;
    before = observed;
  }
}
void release_reader() noexcept { InterlockedDecrement(&gate); }
void release_writer() noexcept { InterlockedExchange(&gate, 0); }
}

namespace ubc::batch_call {
bool configure(const Config& incoming) noexcept {
  if (atomic_read(gate) != 0 || !UbcBatchCallOriginal || !incoming.parser ||
      !incoming.symbol_count || incoming.symbol_count > max_symbols || !incoming.signature ||
      !incoming.select || !incoming.writer)
    return false;
  for (std::size_t i = 0; i < incoming.symbol_count; ++i) {
    if (incoming.symbols[i] < 0)
      return false;
    for (std::size_t j = 0; j < i; ++j)
      if (incoming.symbols[j] == incoming.symbols[i])
        return false;
  }
  InterlockedExchange(&ready, 0);
  config = incoming;
  for (auto& value : blocked)
    InterlockedExchange(&value, 0);
  InterlockedExchange(&disabled, 0);
  InterlockedExchange(&ready, 1);
  return true;
}
Diagnostics diagnostics() noexcept {
  const auto value = atomic_read(gate);
  return {value > 0 ? static_cast<std::uint32_t>(value) : 0, value == writer_bit,
          atomic_read(disabled) != 0};
}
bool writer_undisturbed(const WriterScope& scope) noexcept {
  if (scope.call.thread != GetCurrentThreadId() || scope.call.parser != config.parser ||
      atomic_read(gate) != writer_bit || !scope.token || scope.token != writer_sequence ||
      scope.rejection_floor == UINT32_MAX)
    return false;
  const int index = target_index(scope.call.symbol);
  return index >= 0 && rejected[index].load() == scope.rejection_floor;
}
void disable_writes() noexcept { InterlockedExchange(&disabled, 1); }
bool quarantine(std::int32_t symbol) noexcept {
  const auto index = target_index(symbol);
  if (index < 0)
    return false;
  InterlockedExchange(&blocked[index], 1);
  disable_writes();
  return true;
}
bool quarantined(std::int32_t symbol) noexcept {
  const auto index = target_index(symbol);
  return index >= 0 && atomic_read(blocked[index]) != 0;
}
}

extern "C" {
void* UbcBatchCallOriginal = nullptr;

// 0: unchanged tail jump. 1/2: typed reader/writer. 3: rejection.
// The fixed parser/symbol registry decides first. Handle an
// active writer or quarantine BEFORE any signature callback so
// changed bytes can never accidentally enter the generic path.
__declspec(noinline) unsigned __cdecl UbcBatchPrepare(void* parser, int symbol,
                                                      std::uintptr_t caller) {
  const DWORD error = GetLastError();
  unsigned lease = 0;
  bool transferred = false;
  unsigned route = 0;
  __try {
    if (!atomic_read(ready) || parser != config.parser)
      __leave;
    const b::Call call{parser, symbol, caller, GetCurrentThreadId()};
    if (config.reject_unexpected && config.reject_unexpected(call)) {
      route = 3;
      __leave;
    }
    const auto index = target_index(symbol);
    if (index < 0)
      __leave;
    route = 3;
    if (atomic_read(gate) == writer_bit) {
      denied_by_writer(index);
      __leave;
    }
    if (atomic_read(blocked[index]))
      __leave;
    const auto choice = config.select(call);
    if (choice == b::Decision::reject)
      __leave;
    if (choice == b::Decision::writer) {
      if (atomic_read(disabled))
        __leave;
      const auto floor = rejected[index].load();
      const auto prior = InterlockedCompareExchange(&gate, writer_bit, 0);
      if (prior == writer_bit) {
        denied_by_writer(index);
        __leave;
      }
      if (prior != 0)
        __leave;
      lease = 2;
      writer_rejection_floor = floor;
      if (atomic_read(blocked[index]) || atomic_read(disabled))
        __leave;
      if (!config.signature(call)) {
        b::disable_writes();
        __leave;
      }
      route = 2;
      transferred = true;
    } else if (choice == b::Decision::ordinary) {
      if (!acquire_reader(index))
        __leave;
      lease = 1;
      if (atomic_read(blocked[index]))
        __leave;
      if (!config.signature(call)) {
        // An unknown varargs contract must not go through the two-argument helper.
        // Block writers while still holding the reader, before the tail jump.
        b::disable_writes();
        route = 0;
        __leave;
      }
      route = 1;
      transferred = true;
    }
  } __finally {
    if (!transferred) {
      if (lease == 1)
        release_reader();
      else if (lease == 2)
        release_writer();
    }
    SetLastError(error);
  }
  return route;
}

__declspec(noinline) void* __cdecl UbcBatchReader(void* parser, int symbol) {
  DWORD error = GetLastError();
  void* result = nullptr;
  __try {
    SetLastError(error);
    result = reinterpret_cast<b::Original>(UbcBatchCallOriginal)(parser, symbol);
    error = GetLastError();
  } __finally {
    if (AbnormalTermination())
      error = GetLastError();
    release_reader();
    SetLastError(error);
  }
  return result;
}
__declspec(noinline) void* __cdecl UbcBatchWriter(void* parser, int symbol) {
  DWORD error = GetLastError();
  void* result = nullptr;
  b::WriterScope scope;
  scope.call = {parser, symbol, reinterpret_cast<std::uintptr_t>(_ReturnAddress()),
                GetCurrentThreadId()};
  scope.original = reinterpret_cast<b::Original>(UbcBatchCallOriginal);
  __try {
    if (writer_sequence == UINT64_MAX) {
      b::disable_writes();
      scope.restore_verified = true;
      __leave;
    }
    scope.token = ++writer_sequence;
    scope.rejection_floor = writer_rejection_floor;
    SetLastError(error);
    result = config.writer(scope);
    error = GetLastError();
  } __finally {
    if (AbnormalTermination())
      error = GetLastError();
    if (!scope.restore_verified)
      b::quarantine(symbol);
    release_writer();
    SetLastError(error);
  }
  return result;
}
__declspec(noinline) void* __cdecl UbcBatchRejected(void*, int) {
  return const_cast<int*>(&denied_value);
}

// On entry, [ESP]=actual caller, [ESP+4]=parser, [ESP+8]=symbol, followed
// by unchanged varargs. Do not reinterpret ECX as a thiscall parameter.
// The decision function may clobber registers, flags and FPU state. Restore
// everything before each transfer. At the tail jump the native SEH prologue
// sees exactly its original stack. Do not alter DoStack or its frames.
__declspec(naked) void UbcBatchCallBridge() {
  __asm {
    pushfd
    pushad
    cld
    mov ebx,esp
    sub esp,544
    and esp,-16
    fxsave [esp+16]
    mov eax,dword ptr [ebx+36]
    mov dword ptr [esp+8],eax
    mov eax,dword ptr [ebx+44]
    mov dword ptr [esp+4],eax
    mov eax,dword ptr [ebx+40]
    mov dword ptr [esp],eax
    call UbcBatchPrepare
    cmp eax,1
    je reader_path
    cmp eax,2
    je writer_path
    cmp eax,3
    je reject_path
    fxrstor [esp+16]
    mov esp,ebx
    popad
    popfd
    jmp dword ptr [UbcBatchCallOriginal]
  reader_path:
    fxrstor [esp+16]
    mov esp,ebx
    popad
    popfd
    jmp UbcBatchReader
  writer_path:
    fxrstor [esp+16]
    mov esp,ebx
    popad
    popfd
    jmp UbcBatchWriter
  reject_path:
    fxrstor [esp+16]
    mov esp,ebx
    popad
    popfd
    jmp UbcBatchRejected
  }
}
}
