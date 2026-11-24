// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/engine/amount_patch.hpp"

namespace ubc {
template <std::size_t Size> struct ScriptWindow {
  AmountMemory memory{};
  std::uint32_t address = 0;
  std::array<std::uint8_t, Size> original{}, prepared{};
  std::size_t prefix = 0;
  bool attempted = false;

  bool capture(const AmountMemory& adapter, std::uint32_t start,
               const lhiver::Function& f) noexcept {
    memory = adapter;
    address = start;
    if (f.byte_length != Size || !memory.identity || !memory.writable || !memory.replace ||
        !memory.identity(memory.context, address, Size) ||
        !memory.reader.copy(address, original.data(), Size))
      return false;
    Sha256 hash;
    hash.update(original.data(), Size);
    prepared = original;
    return hash.finish() == f.body_sha256;
  }
  bool matches(const std::array<std::uint8_t, Size>& expected) const noexcept {
    std::array<std::uint8_t, Size> found{};
    return memory.identity && memory.identity(memory.context, address, Size) &&
           memory.reader.copy(address, found.data(), Size) && found == expected;
  }
  bool ready() const noexcept {
    return prefix && prefix <= Size && matches(original) &&
           memory.writable(memory.context, address, prefix);
  }
  bool install() noexcept {
    if (!ready())
      return false;
    attempted = true;
    return memory.replace(memory.context, address, original.data(), prepared.data(), prefix) &&
           matches(prepared);
  }
  bool restore() noexcept {
    if (!attempted || matches(original))
      return true;
    return matches(prepared) &&
           memory.replace(memory.context, address, prepared.data(), original.data(), prefix) &&
           matches(original);
  }
};

} // namespace ubc
