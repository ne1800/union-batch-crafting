// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/player_engine_sites.hpp"
#include "ubc/pe_guard.hpp"
#include "ubc/sha256.hpp"
#include <array>

namespace ubc::player_engine {
struct Result {
  // First failing file offset per station. Offset 1 identifies an invalid image.
  std::array<unsigned, 9> failure{};
  bool any() const noexcept {
    for (auto offset : failure) if (!offset) return true;
    return false;
  }
};
inline Result inspect(const std::uint8_t* file, std::size_t size,
                      const Site* checks, std::size_t count) noexcept {
  Result result;
  if (!matches_gothic2_header(file, size, false) || !checks || !count) {
    result.failure.fill(1);
    return result;
  }
  for (std::size_t i = 0; i < count; ++i) {
    const Site& site = checks[i];
    if (site.offset <= size && site.length <= size - site.offset) {
      Sha256 hash;
      hash.update(file + site.offset, site.length);
      constexpr char digits[] = "0123456789abcdef";
      const auto digest = hash.finish();
      bool matches = true;
      for (std::size_t j = 0; j < digest.size(); ++j) {
        if (digits[digest[j] >> 4] != site.hex[2*j] || digits[digest[j] & 15] != site.hex[2*j+1]) {
          matches = false;
          break;
        }
      }
      if (matches) continue;
    }
    for (unsigned station = 0; station < result.failure.size(); ++station)
      if ((site.stations & (1u << station)) && !result.failure[station])
        result.failure[station] = site.offset;
  }
  return result;
}
} // namespace ubc::player_engine
