// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>

namespace ubc {
namespace detail {

// Call only after full range check. The byte-wise read
// also works for unaligned data and on other hosts.
inline std::uint16_t read_le16_unchecked(const std::uint8_t* bytes) noexcept {
  return static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[0]) |
                                    (static_cast<std::uint16_t>(bytes[1]) << 8));
}

inline std::uint32_t read_le32_unchecked(const std::uint8_t* bytes) noexcept {
  return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8) |
         (static_cast<std::uint32_t>(bytes[2]) << 16) |
         (static_cast<std::uint32_t>(bytes[3]) << 24);
}

} // namespace detail

// Preliminary identity filter for the examined Gothic-II head.
// No full PE validator and no release for engine hooks. data must
// be readable for size bytes; the caller limits process memory
// for example with VirtualQuery before invoking this filter.
inline bool matches_gothic2_header(const std::uint8_t* data, std::size_t size,
                                   bool require_timestamp = true) noexcept {
  constexpr std::size_t dos_header_size = 64;
  constexpr std::size_t nt_prefix_size = 24; // Signature + COFF header.
  constexpr std::uint16_t minimum_optional_size = 60;

  if (data == nullptr || size < dos_header_size) {
    return false;
  }
  if (data[0] != 'M' || data[1] != 'Z') {
    return false;
  }

  const auto pe_offset = static_cast<std::size_t>(detail::read_le32_unchecked(data + 60));
  if (pe_offset < dos_header_size || pe_offset > size || size - pe_offset < nt_prefix_size) {
    return false;
  }

  const auto* nt = data + pe_offset;
  if (detail::read_le32_unchecked(nt) != 0x00004550U ||
      detail::read_le16_unchecked(nt + 4) != 0x014cU ||
      (require_timestamp && detail::read_le32_unchecked(nt + 8) != 0x3f966d87U)) {
    return false;
  }

  const auto optional_size = detail::read_le16_unchecked(nt + 20);
  // The addition is only allowed after the remaining-length check above.
  const auto optional_offset = pe_offset + nt_prefix_size;
  if (optional_size < minimum_optional_size ||
      static_cast<std::size_t>(optional_size) > size - optional_offset) {
    return false;
  }

  const auto* optional = data + optional_offset;
  return detail::read_le16_unchecked(optional) == 0x010bU &&
         detail::read_le32_unchecked(optional + 28) == 0x00400000U &&
         detail::read_le32_unchecked(optional + 56) == 0x00752000U;
}

} // namespace ubc
