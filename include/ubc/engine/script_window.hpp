// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/engine/amount_patch.hpp"

namespace ubc {
template <std::size_t Size> struct ScriptWindow {
  AmountMemory memory{};
  std::uint32_t address = 0;
  std::array<std::uint8_t, Size> original{}, prepared{};
  std::size_t prefix = 0, length = Size;
  bool attempted = false;

  bool capture(const AmountMemory& adapter, std::uint32_t start, const lhiver::Function& f,
               std::size_t body_length = Size) noexcept {
    if (attempted)
      return false;
    length = 0;
    if (!body_length || body_length > Size || f.byte_length != body_length)
      return false;
    memory = adapter;
    address = start;
    if (!memory.identity || !memory.writable || !memory.replace ||
        !memory.identity(memory.context, address, body_length) ||
        !memory.reader.copy(address, original.data(), body_length))
      return false;
    Sha256 hash;
    hash.update(original.data(), body_length);
    if (hash.finish() != f.body_sha256)
      return false;
    length = body_length;
    prepared = original;
    return true;
  }
  bool matches(const std::array<std::uint8_t, Size>& expected) const noexcept {
    std::array<std::uint8_t, Size> found{};
    return length && length <= Size && memory.identity &&
           memory.identity(memory.context, address, length) &&
           memory.reader.copy(address, found.data(), length) &&
           std::equal(found.begin(), found.begin() + length, expected.begin());
  }
  bool ready() const noexcept {
    return prefix && prefix <= length && matches(original) &&
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
