// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace ubc {
// Copy only the first rejected comparison. Formatting
// happens when writing the event, outside the quantity patch.
struct GrindCheck {
  const char* field = nullptr;
  std::uint64_t expected = 0, actual = 0;
  bool unsigned_values = false;
  const char* point = "";
  int symbol = 0;

  bool same(const GrindCheck& other) const noexcept {
    return std::string_view(field ? field : "") == (other.field ? other.field : "") &&
           std::string_view(point) == other.point && symbol == other.symbol &&
           expected == other.expected && actual == other.actual &&
           unsigned_values == other.unsigned_values;
  }

  bool equal_unsigned(const char* name, std::uint64_t wanted, std::uint64_t observed) noexcept {
    if (wanted == observed)
      return true;
    if (!field) {
      field = name;
      expected = wanted;
      actual = observed;
      unsigned_values = true;
    }
    return false;
  }
  bool equal(const char* name, std::int64_t wanted, std::int64_t observed) noexcept {
    if (wanted == observed)
      return true;
    if (!field) {
      equal_unsigned(name, static_cast<std::uint64_t>(wanted),
                     static_cast<std::uint64_t>(observed));
      unsigned_values = false;
    }
    return false;
  }
  void format(char (&fields)[176]) const noexcept {
    const char* separator = *point ? "." : "";
    if (unsigned_values)
      std::snprintf(fields, sizeof(fields),
                    ",\"field\":\"%s%s%s\",\"soll\":\"%llu\",\"ist\":\"%llu\"", point, separator,
                    field, static_cast<unsigned long long>(expected),
                    static_cast<unsigned long long>(actual));
    else
      std::snprintf(fields, sizeof(fields),
                    ",\"field\":\"%s%s%s\",\"soll\":\"%lld\",\"ist\":\"%lld\"", point, separator,
                    field, static_cast<long long>(expected), static_cast<long long>(actual));
  }
};
}
