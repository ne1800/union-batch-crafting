// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/core/sha256.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>

namespace ubc::dndr::forge {
constexpr int cold = 6912, hot = 6913;
// Largest order in the initial forge qualification matrix.
constexpr unsigned order_limit = 3;
enum class Hand { unknown, empty, reserved, left_cold, left_hot };
struct Stock {
  int cold_count = -1, hot_count = -1;
  std::array<std::uint8_t, 32> other{};
  Hand hand = Hand::unknown;
  bool unambiguous = false;
};

inline unsigned maximum(const Stock& s) noexcept {
  if (!s.unambiguous || s.hand != Hand::empty || s.cold_count <= 0 || s.hot_count < 0)
    return 0;
  return static_cast<unsigned>(
      std::min({s.cold_count, std::numeric_limits<int>::max() - s.hot_count,
                static_cast<int>(order_limit)}));
}

enum class Phase {
  idle,
  ready,
  dispatched,
  reserved,
  inserted,
  exchanged,
  returned,
  ended,
  complete,
  stopped
};
enum class Witness { reservation, insert, exchange, remove, end, neutral };

// EndInteraction briefly clears the hero's focus before the engine targets the forge again.
enum class Focus { missing, forge, other };
enum class NeutralResult { waiting, ready, rejected };
class ReturnFocus {
public:
  void reset(std::uint64_t tick) noexcept {
    start_ = tick;
    seen_ = false;
  }
  NeutralResult observe(std::uint64_t tick, Focus focus) noexcept {
    if (tick < start_ || tick - start_ > 250 || focus == Focus::other)
      return NeutralResult::rejected;
    if (focus == Focus::missing)
      return seen_ ? NeutralResult::rejected : NeutralResult::waiting;
    if (seen_)
      return NeutralResult::ready;
    seen_ = true;
    return NeutralResult::waiting;
  }

private:
  std::uint64_t start_ = 0;
  bool seen_ = false;
};

// Each transition needs the observed inventory and hand state at that boundary.
// Interruptions leave the entire material decision to the original game.
class Order {
public:
  bool open(const Stock& s, unsigned count) noexcept {
    if (!count || count > maximum(s) || active())
      return false;
    baseline_ = s;
    selected_ = count;
    completed_ = 0;
    phase_ = Phase::ready;
    return true;
  }
  bool start(const Stock& s) noexcept {
    if (phase_ != Phase::ready || !matches(s, 0, 0, Hand::empty) ||
        maximum(s) < selected_ - completed_)
      return stop();
    phase_ = Phase::dispatched;
    return true;
  }
  bool observe(Witness w, const Stock& s, std::uint64_t frame) noexcept {
    if (!active())
      return false;
    switch (w) {
    case Witness::reservation:
      return advance(Phase::dispatched, Phase::reserved, s, 0, Hand::reserved);
    case Witness::insert:
      return advance(Phase::reserved, Phase::inserted, s, 0, Hand::left_cold);
    case Witness::exchange:
      return advance(Phase::inserted, Phase::exchanged, s, 0, Hand::left_hot);
    case Witness::remove:
      return advance(Phase::exchanged, Phase::returned, s, 1, Hand::empty);
    case Witness::end:
      end_frame_ = frame;
      return advance(Phase::returned, Phase::ended, s, 1, Hand::empty);
    case Witness::neutral:
      if (phase_ != Phase::ended || frame <= end_frame_ || !matches(s, -1, 1, Hand::empty))
        return stop();
      ++completed_;
      phase_ = completed_ == selected_ ? Phase::complete : Phase::ready;
      return true;
    }
    return stop();
  }
  bool stop() noexcept {
    phase_ = Phase::stopped;
    return false;
  }
  bool active() const noexcept { return phase_ >= Phase::ready && phase_ <= Phase::ended; }
  bool neutral_stock(const Stock& s) const noexcept {
    return phase_ == Phase::ended && matches(s, -1, 1, Hand::empty);
  }
  Phase phase() const noexcept { return phase_; }
  unsigned selected() const noexcept { return selected_; }
  unsigned completed() const noexcept { return completed_; }

private:
  bool matches(const Stock& s, int debit, int credit, Hand hand) const noexcept {
    const int n = static_cast<int>(completed_);
    return s.unambiguous && s.hand == hand && s.other == baseline_.other &&
           s.cold_count == baseline_.cold_count - n + debit &&
           s.hot_count == baseline_.hot_count + n + credit;
  }
  bool advance(Phase from, Phase to, const Stock& s, int credit, Hand hand) noexcept {
    if (phase_ != from || !matches(s, -1, credit, hand))
      return stop();
    phase_ = to;
    return true;
  }
  Stock baseline_{};
  unsigned selected_ = 0, completed_ = 0;
  std::uint64_t end_frame_ = 0;
  Phase phase_ = Phase::idle;
};
} // namespace ubc::dndr::forge
