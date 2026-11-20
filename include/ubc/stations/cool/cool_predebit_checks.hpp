// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace ubc::cool_predebit::check {
struct Sample {
  const char* field = nullptr;
  char expected[72]{}, actual[80]{};
  bool matches = false;
  int reason = 0;
};
struct Report {
  // Fixed storage of the plugin owner. No heap and no world-list log per object.
  std::array<Sample, 640> rows{};
  std::size_t count = 0;
  bool overflow = false;
  void add(const Sample& sample) noexcept {
    // Summarize repetitions (two reads, inventory rows, pool
    // nodes). Keep all different errors per check point.
    for (std::size_t i = 0; i < count; ++i) {
      auto& prior = rows[i];
      if (std::strcmp(prior.field, sample.field))
        continue;
      if (sample.matches || (!prior.matches && !std::strcmp(prior.actual, sample.actual) &&
                             !std::strcmp(prior.expected, sample.expected)))
        return;
      if (prior.matches) {
        prior = sample;
        return;
      }
    }
    if (count < rows.size())
      rows[count++] = sample;
    else
      overflow = true;
  }
};
class Checks {
public:
  explicit Checks(Report* report = nullptr) noexcept : report_(report) {}
  Sample first{};
  int reason = 0;
  bool collecting() const noexcept { return report_ != nullptr; }
  bool ok() const noexcept { return !first.field; }
  bool record(const char* field, const char* want, const char* actual, bool matches) noexcept {
    if (matches && !report_)
      return true;
    Sample row;
    row.field = field;
    row.matches = matches;
    row.reason = reason;
    std::snprintf(row.expected, sizeof(row.expected), "%s", want);
    std::snprintf(row.actual, sizeof(row.actual), "%s", actual);
    if (!matches && !first.field)
      first = row;
    if (report_)
      report_->add(row);
    return matches || collecting();
  }
  template <class Value>
  bool test(const char* field, const char* want, Value actual, bool matches,
            bool known = true) noexcept {
    if (matches && known && !report_)
      return true;
    char value[32]{};
    number(value, actual);
    return record(field, want, known ? value : "unlesbar", known && matches);
  }
  template <class Expected, class Actual>
  bool equal(const char* field, Expected want, Actual actual, bool known = true) noexcept {
    const bool matches = negative(want) == negative(actual) &&
                         static_cast<std::uint64_t>(want) == static_cast<std::uint64_t>(actual);
    if (known && matches && !report_)
      return true;
    char expected[32]{};
    number(expected, want);
    return test(field, expected, actual, matches, known);
  }
  bool flag(const char* field, bool value) noexcept { return equal(field, 1, value); }
  template <class Digest>
  bool digest(const char* field, const Digest& want, const Digest& actual) noexcept {
    if (want == actual && !report_)
      return true;
    char expected[65]{}, value[65]{};
    const char* digits = "0123456789abcdef";
    for (std::size_t i = 0; i < 32; ++i) {
      expected[i * 2] = digits[want[i] >> 4];
      expected[i * 2 + 1] = digits[want[i] & 15];
      value[i * 2] = digits[actual[i] >> 4];
      value[i * 2 + 1] = digits[actual[i] & 15];
    }
    return record(field, expected, value, want == actual);
  }

private:
  template <class T> static bool negative(T value) noexcept {
    if constexpr (std::is_signed_v<T>)
      return value < 0;
    else
      return false;
  }
  template <class T> static void number(char (&text)[32], T value) noexcept {
    if constexpr (std::is_signed_v<T>)
      std::snprintf(text, sizeof(text), "%lld", static_cast<long long>(value));
    else
      std::snprintf(text, sizeof(text), "%llu", static_cast<unsigned long long>(value));
  }
  Report* report_;
};
} // namespace ubc::cool_predebit::check
