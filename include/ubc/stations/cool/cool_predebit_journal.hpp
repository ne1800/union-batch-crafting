// SPDX-License-Identifier: MIT
#pragma once
#include <atomic>
#include <cstdint>

namespace ubc::cool_predebit {
// All entry/exit of the destroy hooks, even nested or on a foreign thread.
// Only the exclusive writer opens/closes the single interval lease.
class DestroyJournal {
public:
  void entered() noexcept { advance(started_); }
  void exited() noexcept { advance(completed_); }
  std::uint64_t open() noexcept {
    if (active_ || sequence_ == UINT64_MAX)
      return 0;
    base_ = started_.load();
    if (base_ == UINT32_MAX || completed_.load() != base_ || started_.load() != base_)
      return 0;
    active_ = ++sequence_;
    return active_;
  }
  bool sample(std::uint64_t token, std::uint32_t& started,
              std::uint32_t& completed) const noexcept {
    if (!token || token != active_)
      return false;
    completed = completed_.load() - base_;
    started = started_.load() - base_;
    return !started && !completed;
  }
  bool close(std::uint64_t token) noexcept {
    if (!token || token != active_)
      return false;
    std::uint32_t started = 0, completed = 0;
    const bool quiet = sample(token, started, completed);
    active_ = 0;
    return quiet;
  }

private:
  static void advance(std::atomic<std::uint32_t>& counter) noexcept {
    auto n = counter.load();
    while (n != UINT32_MAX && !counter.compare_exchange_weak(n, n + 1)) {
    }
  }
  std::atomic<std::uint32_t> started_{0}, completed_{0};
  std::uint32_t base_ = 0;
  std::uint64_t sequence_ = 0, active_ = 0;
};
}
