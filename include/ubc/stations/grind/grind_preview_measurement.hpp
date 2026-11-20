// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/grind/grind_preview_batch.hpp"
#include <map>
#include <vector>

// Diagnostic only. Dynamic memory, no production publication, no
// engine calls/writer. The reader-owned list/string limits remain.
// The old budget limits are observed but do not end the measurement.
namespace ubc::grind_preview_measurement {
namespace batch = grind_preview_batch;
inline constexpr std::size_t legacy_spans = 8192, legacy_bytes = 128u * 1024u,
                             legacy_span_bytes = 4096;
using Range = std::pair<std::uint32_t, std::uint32_t>;
// The measurement mode does not open a recipe menu. It must therefore execute
// its generation start itself before entering the frame and station readers.
inline bool start_generation(std::uint64_t& generation) noexcept {
  if (generation == UINT64_MAX)
    return false;
  ++generation;
  return true;
}
struct RejectedRequest {
  batch::Issue issue = batch::Issue::none;
  std::uint32_t address = 0, length = 0;
  std::size_t spans = 0, bytes = 0;
};
class ReadSet {
public:
  static constexpr bool diagnostic_only = true;
  bool begin(const ReadOnlyMemory& original, batch::FrameSource frames,
             const batch::FrameObservation& expected, std::uint32_t owner) noexcept {
    if (active_)
      return reject(batch::Issue::reentry);
    phase_ = Phase::recording;
    original_ = original;
    statistics_ = {};
    rejected_ = {};
    spans_.clear();
    order_.clear();
    trace_ = Sha256{};
    first_trace_ = {};
    receipt_ = {};
    largest_ = 0;
    return guard_.begin(original, frames, expected, owner);
  }
  ReadOnlyMemory memory() noexcept { return {this, copy}; }
  bool current() noexcept { return guard_.current(); }
  bool reject(batch::Issue issue) noexcept {
    phase_ = Phase::failed;
    return guard_.reject(issue);
  }
  batch::Issue issue() const noexcept { return guard_.issue(); }
  const batch::Statistics& statistics() const noexcept { return statistics_; }
  const RejectedRequest& rejected() const noexcept { return rejected_; }
  const std::vector<Range>& ranges() const noexcept { return order_; }
  const lhiver::Digest& receipt() const noexcept { return receipt_; }
  bool freeze() noexcept {
    if (active_ || phase_ != Phase::recording)
      return reject(active_ ? batch::Issue::reentry : batch::Issue::closed);
    if (!current() || spans_.empty())
      return reject(batch::Issue::scope);
    first_trace_ = trace_.finish();
    trace_ = Sha256{};
    phase_ = Phase::replay;
    return true;
  }
  bool seal() noexcept {
    if (active_ || phase_ != Phase::replay)
      return reject(active_ ? batch::Issue::reentry : batch::Issue::closed);
    if (statistics_.recording_calls != statistics_.replay_calls ||
        statistics_.recording_bytes != statistics_.replay_bytes || trace_.finish() != first_trace_)
      return reject(batch::Issue::replay_changed);
    active_ = true;
    struct Guard {
      bool& value;
      ~Guard() { value = false; }
    } guard{active_};
    phase_ = Phase::checking;
    try {
      std::vector<std::uint8_t> scratch(largest_);
      Sha256 hash;
      for (const auto& range : order_) {
        if (!current())
          return false;
        const auto& bytes = spans_.at(range);
        ++statistics_.final_reads;
        statistics_.final_bytes += bytes.size();
        if (!original_.copy(range.first, scratch.data(), bytes.size()))
          return reject(batch::Issue::unreadable);
        if (phase_ != Phase::checking || !current())
          return false;
        if (std::memcmp(scratch.data(), bytes.data(), bytes.size()))
          return reject(batch::Issue::changed);
        batch::detail::word(hash, range.first);
        batch::detail::word(hash, range.second);
        hash.update(bytes.data(), bytes.size());
      }
      receipt_ = hash.finish();
      original_ = {};
      phase_ = Phase::published;
      return current();
    } catch (...) {
      return reject(batch::Issue::bounds);
    }
  }
  bool published() noexcept { return phase_ == Phase::published && current(); }
  void close() noexcept {
    phase_ = Phase::failed;
    guard_.close();
    original_ = {};
  }
  std::pair<std::size_t, std::uint64_t> union_size() const noexcept {
    std::size_t count = 0;
    std::uint64_t bytes = 0, end = 0;
    for (const auto& entry : spans_) {
      const auto& range = entry.first;
      const auto next = std::uint64_t{range.first} + range.second;
      if (range.first > end || !count) {
        ++count;
        bytes += range.second;
      } else if (next > end)
        bytes += next - end;
      end = std::max(end, next);
    }
    return {count, bytes};
  }

private:
  enum class Phase { recording, replay, checking, published, failed };
  static bool copy(void* self, std::uint32_t address, void* output, std::size_t size) noexcept {
    return static_cast<ReadSet*>(self)->read(address, output, size);
  }
  bool read(std::uint32_t address, void* output, std::size_t size) noexcept {
    using batch::Issue;
    if (active_)
      return reject(Issue::reentry);
    if (phase_ != Phase::recording && phase_ != Phase::replay)
      return reject(Issue::closed);
    active_ = true;
    struct Guard {
      bool& value;
      ~Guard() { value = false; }
    } guard{active_};
    if (!current())
      return false;
    if (!address || !output || !size || size > UINT32_MAX ||
        std::uint64_t{address} + size > (std::uint64_t{1} << 32))
      return reject(Issue::bounds);
    auto& calls =
        phase_ == Phase::recording ? statistics_.recording_calls : statistics_.replay_calls;
    auto& bytes =
        phase_ == Phase::recording ? statistics_.recording_bytes : statistics_.replay_bytes;
    ++calls;
    bytes += size;
    batch::detail::word(trace_, address);
    batch::detail::word(trace_, static_cast<std::uint32_t>(size));
    const Range range{address, static_cast<std::uint32_t>(size)};
    try {
      auto found = spans_.find(range);
      if (phase_ == Phase::recording && rejected_.issue == Issue::none) {
        const auto problem =
            size > legacy_span_bytes ? Issue::bounds
            : calls > batch::max_calls_per_pass || bytes > batch::max_bytes_per_pass
                ? Issue::call_limit
            : found == spans_.end() && statistics_.spans == legacy_spans ? Issue::span_limit
            : found == spans_.end() && size > legacy_bytes - statistics_.stored_bytes
                ? Issue::byte_limit
                : Issue::none;
        if (problem != Issue::none)
          rejected_ = {problem, address, range.second, statistics_.spans, statistics_.stored_bytes};
      }
      if (found != spans_.end()) {
        std::memcpy(output, found->second.data(), size);
        return current();
      }
      if (phase_ == Phase::replay)
        return reject(Issue::replay_miss);
      std::vector<std::uint8_t> value(size);
      ++statistics_.first_reads;
      statistics_.first_bytes += size;
      if (!original_.copy(address, value.data(), size))
        return reject(Issue::unreadable);
      if (phase_ != Phase::recording || !current())
        return false;
      const auto end = std::uint64_t{address} + size;
      for (auto i = spans_.lower_bound(
               {address > largest_ ? address - static_cast<std::uint32_t>(largest_) : 0u, 0u});
           i != spans_.end() && i->first.first < end; ++i) {
        const auto low = std::max<std::uint64_t>(address, i->first.first);
        const auto high = std::min(end, std::uint64_t{i->first.first} + i->first.second);
        if (low >= high)
          continue;
        ++statistics_.overlaps;
        if (std::memcmp(value.data() + low - address, i->second.data() + low - i->first.first,
                        high - low))
          return reject(Issue::overlap);
      }
      order_.push_back(range);
      spans_.emplace(range, std::move(value));
      ++statistics_.spans;
      statistics_.stored_bytes += size;
      largest_ = std::max(largest_, size);
      statistics_.largest_span = largest_;
      std::memcpy(output, spans_.at(range).data(), size);
      return current();
    } catch (...) {
      return reject(Issue::bounds);
    }
  }
  batch::detail::ReadSet guard_; // Scope/reentry check only, no cache access
  ReadOnlyMemory original_{};
  bool active_ = false;
  Phase phase_ = Phase::failed;
  batch::Statistics statistics_{};
  RejectedRequest rejected_{};
  std::map<Range, std::vector<std::uint8_t>> spans_;
  std::vector<Range> order_;
  Sha256 trace_;
  lhiver::Digest first_trace_{}, receipt_{};
  std::size_t largest_ = 0;
};
using Reader = batch::BasicReader<ReadSet>;
} // namespace ubc::grind_preview_measurement
