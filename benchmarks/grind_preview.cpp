// SPDX-License-Identifier: MIT
#include "ubc/stations/grind/grind_preview_batch.hpp"
#include "../tests/native/helpers/grind_preview_capacity_fixture.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <vector>

namespace {
namespace batch = ubc::grind_preview_batch;
using Clock = std::chrono::steady_clock;
struct Range {
  std::uint32_t address, length;
};
struct Source {
  std::vector<unsigned char> bytes;
  std::size_t calls = 0;
  static bool read(void* raw, std::uint32_t address, void* out, std::size_t length) noexcept {
    auto& source = *static_cast<Source*>(raw);
    if (address > source.bytes.size() || length > source.bytes.size() - address)
      return false;
    std::memcpy(out, source.bytes.data() + address, length);
    ++source.calls;
    return true;
  }
};

bool measure(unsigned extra, unsigned calls, unsigned sample) {
  std::vector<Range> ranges;
  std::uint32_t end = 1024;
  auto append = [&](unsigned length) {
    // Include partial intersections as well as adjacent and disjoint spans.
    const auto start = end - (ranges.size() % 17 == 0 ? 2u : 0u);
    ranges.push_back({start, length});
    end = std::max(end, start + length) + (ranges.size() % 3);
  };
  for (auto [length, count] : grind_preview_capacity_fixture::lengths)
    for (unsigned i = 0; i < count; ++i)
      append(length);
  for (unsigned i = 0; i < extra; ++i)
    append(12);
  std::mt19937 random(73);
  std::shuffle(ranges.begin(), ranges.end(), random);
  std::vector<Range> requests = ranges;
  while (requests.size() < calls)
    requests.push_back(ranges[random() % ranges.size()]);
  Source source{std::vector<unsigned char>(end)};
  for (auto& byte : source.bytes)
    byte = static_cast<unsigned char>(random());
  batch::FrameObservation frame{{1, 2, 7, 3, 4, 5, 6, 11}, 1, 7, 1, true, false, 0, true};
  const batch::FrameSource frames{&frame, [](void* raw, batch::FrameObservation& out) noexcept {
                                    out = *static_cast<batch::FrameObservation*>(raw);
                                    return true;
                                  }};
  auto ledger = std::make_unique<batch::detail::ReadSet>();
  auto copy = ledger->memory();
  std::array<unsigned char, batch::max_span_bytes> scratch{};
  const auto read = [&](std::size_t begin, std::size_t limit) {
    for (auto i = begin; i < limit; ++i)
      if (!copy.copy(requests[i].address, scratch.data(), requests[i].length))
        return false;
    return true;
  };
  const auto start = Clock::now();
  if (!ledger->begin({&source, Source::read}, frames, frame, 7) || !read(0, ranges.size()))
    return false;
  const auto unique = Clock::now();
  if (!read(ranges.size(), calls) || !ledger->freeze())
    return false;
  const auto recorded = Clock::now();
  if (!read(0, calls))
    return false;
  const auto replayed = Clock::now();
  if (!ledger->seal() || !ledger->published())
    return false;
  const auto sealed = Clock::now();
  const auto us = [](auto a, auto b) {
    return std::chrono::duration<double, std::micro>(b - a).count();
  };
  const auto& stats = ledger->statistics();
  std::printf("%u,%zu,%zu,%u,%zu,%zu,%.1f,%.1f,%.1f,%.1f,%.1f\n", sample, stats.spans,
              stats.stored_bytes, calls, stats.overlaps, source.calls, us(start, unique),
              us(unique, recorded), us(recorded, replayed), us(replayed, sealed),
              us(start, sealed));
  return stats.spans == ranges.size() && source.calls == 2 * stats.spans &&
         stats.recording_calls == calls && stats.replay_calls == calls;
}
} // namespace

int main() {
  std::puts(
      "sample,spans,stored_bytes,calls_per_pass,overlaps,source_reads,unique_us,duplicates_us,replay_us,seal_us,total_us");
  for (unsigned sample = 1; sample <= 7; ++sample)
    for (auto workload :
         {std::pair{0u, 328517u}, std::pair{0u, 400560u}, std::pair{4096u, 400560u}})
      if (!measure(workload.first, workload.second, sample))
        return EXIT_FAILURE;
}
