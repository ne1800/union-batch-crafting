// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>

namespace ubc {

// The reader copies exclusively from a 32-bit address space. It must
// check the readability of the entire region; this interface checks
// its representation and performs no native memory accesses itself.
struct ReadOnlyMemory {
  void* context;
  bool (*read)(void*, std::uint32_t, void*, std::size_t) noexcept;

  bool copy(std::uint32_t address, void* output, std::size_t size) const noexcept {
    if (!address || !read || (size && !output)) return false;
    const std::uint64_t available = (std::uint64_t{1} << 32) - address;
    if (size > available) return false;
    return size == 0 || read(context, address, output, size);
  }
};

}  // namespace ubc
