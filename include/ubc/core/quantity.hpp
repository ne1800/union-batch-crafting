// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace ubc {

// The selection lies in [1, maximum] when material is available, otherwise
// at 0. An invalid quick-choice button does not change the current value.
class Quantity {
public:
  explicit Quantity(std::uint32_t maximum = 0) noexcept
      : maximum_(maximum), value_(maximum == 0 ? 0U : 1U) {}

  void reset(std::uint32_t maximum) noexcept {
    maximum_ = maximum;
    value_ = maximum == 0 ? 0U : 1U;
  }

  void update_maximum(std::uint32_t maximum) noexcept {
    maximum_ = maximum;
    if (maximum == 0) {
      value_ = 0;
    } else if (value_ == 0) {
      value_ = 1;
    } else if (value_ > maximum) {
      value_ = maximum;
    }
  }

  void adjust(std::int32_t delta) noexcept {
    if (maximum_ == 0) {
      return;
    }
    // Even UINT32_MAX + INT32_MAX and INT32_MIN are representable here.
    const auto requested = static_cast<std::int64_t>(value_) +
                           static_cast<std::int64_t>(delta);
    if (requested < 1) {
      value_ = 1;
    } else if (requested > static_cast<std::int64_t>(maximum_)) {
      value_ = maximum_;
    } else {
      value_ = static_cast<std::uint32_t>(requested);
    }
  }

  bool preset(std::uint32_t amount) noexcept {
    if (amount == 0 || amount > maximum_) {
      return false;
    }
    value_ = amount;
    return true;
  }

  std::uint32_t value() const noexcept { return value_; }
  std::uint32_t maximum() const noexcept { return maximum_; }

private:
  std::uint32_t maximum_;
  std::uint32_t value_;
};

// Keys already held when opening are taken over without action.
// Only releasing and a new key press produce the next event.
class PressEdge {
public:
  explicit PressEdge(bool already_down = false) noexcept
      : was_down_(already_down) {}

  bool update(bool down) noexcept {
    const bool pressed = down && !was_down_;
    was_down_ = down;
    return pressed;
  }

  void reset(bool down) noexcept { was_down_ = down; }

private:
  bool was_down_;
};

}  // namespace ubc
