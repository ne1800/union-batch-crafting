// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>

namespace ubc {

// Labels the first consumer of a recorded preview dependency. Nested readers
// restore their caller's label and ordinary readers never consult it.
struct ReadOrigin {
  inline static thread_local const char* current = "context";
  inline static thread_local std::uint32_t subject = 0;
  const char* previous;
  std::uint32_t previous_subject;
  explicit ReadOrigin(const char* name, std::uint32_t index = 0) noexcept
      : previous(current), previous_subject(subject) {
    current = name;
    subject = index;
  }
  ~ReadOrigin() {
    current = previous;
    subject = previous_subject;
  }
  ReadOrigin(const ReadOrigin&) = delete;
  ReadOrigin& operator=(const ReadOrigin&) = delete;
};

// The reader copies exclusively from a 32-bit address space. It must
// check the readability of the entire region; this interface checks
// its representation and performs no native memory accesses itself.
struct ReadOnlyMemory {
  void* context;
  bool (*read)(void*, std::uint32_t, void*, std::size_t) noexcept;

  // A display reader may request just the parser fields consumed by read_layout.
  // Full live readers retain their original contiguous observation.
  bool (*precise_parser_reads)() noexcept = nullptr;

  bool copy(std::uint32_t address, void* output, std::size_t size) const noexcept {
    if (!address || !read || (size && !output))
      return false;
    const std::uint64_t available = (std::uint64_t{1} << 32) - address;
    if (size > available)
      return false;
    return size == 0 || read(context, address, output, size);
  }
};

} // namespace ubc
