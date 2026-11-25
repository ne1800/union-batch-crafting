// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/core/sha256.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string_view>

namespace ubc::dndr::forge {
constexpr int cold = 6912, hot = 6913;
// Largest order in the station qualification matrix.
constexpr unsigned order_limit = 3;
enum class Station { forge, pan };
enum class Hand { unknown, empty, reserved, left_cold, left_hot, right_raw, left_cooked };
constexpr Hand inserted_hand(Station station) noexcept {
  return station == Station::pan ? Hand::right_raw : Hand::left_cold;
}
constexpr Hand exchanged_hand(Station station) noexcept {
  return station == Station::pan ? Hand::left_cooked : Hand::left_hot;
}
struct Stock {
  int input_count = -1, output_count = -1;
  std::array<std::uint8_t, 32> other{};
  Hand hand = Hand::unknown;
  bool unambiguous = false;
};

inline unsigned maximum(const Stock& s) noexcept {
  if (!s.unambiguous || s.hand != Hand::empty || s.input_count <= 0 || s.output_count < 0)
    return 0;
  return static_cast<unsigned>(
      std::min({s.input_count, std::numeric_limits<int>::max() - s.output_count,
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
constexpr std::uint64_t return_focus_window_ms = 1000;
class ReturnFocus {
public:
  void reset(std::uint64_t tick) noexcept {
    start_ = tick;
    seen_ = false;
  }
  NeutralResult observe(std::uint64_t tick, Focus focus) noexcept {
    if (tick < start_ || tick - start_ > return_focus_window_ms || focus == Focus::other)
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
  bool open(const Stock& s, unsigned count, Station station = Station::forge) noexcept {
    if (!count || count > maximum(s) || active())
      return false;
    station_ = station;
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
  bool observe(Witness w, const Stock& s, std::uint64_t frame,
               std::string_view animation = {}) noexcept {
    if (!active())
      return false;
    if (station_ == Station::pan) {
      if ((w == Witness::insert && animation != "T_PAN_S0_2_S1") ||
          ((w == Witness::exchange || w == Witness::remove) && animation != "T_PAN_S1_2_S0"))
        return stop();
    }
    switch (w) {
    case Witness::reservation:
      return advance(Phase::dispatched, Phase::reserved, s, 0, Hand::reserved);
    case Witness::insert:
      return advance(Phase::reserved, Phase::inserted, s, 0, inserted_hand(station_));
    case Witness::exchange:
      return advance(Phase::inserted, Phase::exchanged, s, 0, exchanged_hand(station_));
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
  bool awaiting_forward() const noexcept {
    return selected_ > 1 &&
           phase_ == (station_ == Station::pan ? Phase::inserted : Phase::exchanged);
  }
  bool pan_workpiece(const Stock& s) const noexcept {
    return station_ == Station::pan && phase_ == Phase::inserted &&
           matches(s, -1, 0, Hand::right_raw);
  }
  Phase phase() const noexcept { return phase_; }
  unsigned selected() const noexcept { return selected_; }
  unsigned completed() const noexcept { return completed_; }

private:
  bool matches(const Stock& s, int debit, int credit, Hand hand) const noexcept {
    const int n = static_cast<int>(completed_);
    return s.unambiguous && s.hand == hand && s.other == baseline_.other &&
           s.input_count == baseline_.input_count - n + debit &&
           s.output_count == baseline_.output_count + n + credit;
  }
  bool advance(Phase from, Phase to, const Stock& s, int credit, Hand hand) noexcept {
    if (phase_ != from || !matches(s, -1, credit, hand))
      return stop();
    phase_ = to;
    return true;
  }
  Station station_ = Station::forge;
  Stock baseline_{};
  unsigned selected_ = 0, completed_ = 0;
  std::uint64_t end_frame_ = 0;
  Phase phase_ = Phase::idle;
};
} // namespace ubc::dndr::forge
