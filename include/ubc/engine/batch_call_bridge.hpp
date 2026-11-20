// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#if defined(_WIN32) && defined(_M_IX86)
namespace ubc::batch_call {

inline constexpr std::size_t max_symbols = 256;
inline constexpr std::uintptr_t callfunc_entry = 0x007929f0;
inline constexpr std::size_t callfunc_patch_size = 7;
inline constexpr std::array<std::uint8_t, 27> callfunc_guard{
    {0x6a, 0xff, 0x68, 0x00, 0xa9, 0x82, 0x00, 0x64, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x50,
     0x64, 0x89, 0x25, 0x00, 0x00, 0x00, 0x00, 0x81, 0xec, 0x14, 0x01, 0x00, 0x00}};
inline constexpr std::uintptr_t info_call_return = 0x00703981;
inline constexpr std::uintptr_t choice_call_return = 0x00662b6f;

struct Call {
  void* parser = nullptr;
  std::int32_t symbol = -1;
  std::uintptr_t caller = 0;
  std::uint32_t thread = 0;
};
enum class Decision { ordinary, writer, reject };
// This signature is correct only for previously verified VOID/0Arg
// symbols. The generic cdecl-Varargs entry is never called back this way.
using Original = void*(__cdecl*)(void*, int);
struct WriterScope {
  Call call{};
  Original original = nullptr;
  // Monotonically assigned by the exclusive bridge owner, never
  // derived from job/frame. Null means: no writer may enter.
  std::uint64_t token = 0;
  // The callback sets this only after positive evidence: old body
  // fully restored or demonstrably unchanged. Without evidence
  // the bridge also quarantines the target on normal return.
  bool restore_verified = false;
  // Counter taken upon acquiring the writer lease for exactly this symbol.
  std::uint32_t rejection_floor = UINT32_MAX;
};
using Signature = bool(__cdecl*)(const Call&) noexcept;
using Select = Decision(__cdecl*)(const Call&) noexcept;
using Writer = void*(__cdecl*)(WriterScope&);
struct Config {
  void* parser = nullptr;
  std::array<std::int32_t, max_symbols> symbols{};
  std::size_t symbol_count = 0;
  Signature signature = nullptr;
  Select select = nullptr;
  Writer writer = nullptr;
  bool(__cdecl* reject_unexpected)(const Call&) noexcept = nullptr;
};
struct Diagnostics {
  std::uint32_t readers = 0;
  bool writer = false;
  bool writes_disabled = false;
};

// Only before hook installation, without simultaneously active bridge
// calls. The registry is copied; callbacks and original pointer remain
// fixed thereafter. signature reads type/parameters/return, never the
// changed body. select must be thread-safe: foreign threads must not
// read unprotected order variables. Both callbacks must not call any
// engine function. writer is called exclusively; only it re-checks
// body/stock/order, patches, and calls scope.original exactly zero or
// once with scope.call. Its restoration needs its own __finally block.
// The bridge restores no bytecode and catches no engine exception.
bool configure(const Config&) noexcept;
Diagnostics diagnostics() noexcept;
// Called only by the current writer. Any locked re-request of the
// same symbol invalidates the pre-debit, even on a foreign thread.
bool writer_undisturbed(const WriterScope&) noexcept;
void disable_writes() noexcept;
bool quarantine(std::int32_t symbol) noexcept;
bool quarantined(std::int32_t symbol) noexcept;

// A rejected known VOID/0Arg call yields a stable pointer to its own
// Integer null; parser stack and global engine return value remain
// untouched. For unknown signature the full original stack is preserved
// via tail jump, while further writers remain permanently locked.
// The gate covers CallFunc, not direct foreign DoStack/BL entries.
}
extern "C" {
extern void* UbcBatchCallOriginal;
void UbcBatchCallBridge();
}
#endif
