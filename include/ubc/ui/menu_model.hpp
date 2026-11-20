// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <limits>

#include "ubc/core/quantity.hpp"

namespace ubc {

enum class MenuAction { None, Left, Right, Up, Down, Confirm, Back };

// Pure operation test: the model knows neither engine objects nor
// recipes. observe() is called once after the events of a frame.
class MenuModel {
public:
  void reset() noexcept { *this = MenuModel{}; }

  // Error exit without fabricated key event; the same availability limit
  // applies as for the user. Repeated requests do not restart them.
  void request_close() noexcept {
    if (!closing_ && !close_ready_)
      begin_close();
  }

  bool on_action(MenuAction action) noexcept {
    bool was_latched = false;
    if (action == MenuAction::Confirm) {
      was_latched = confirm_latched_;
      confirm_latched_ = true;
      confirm_event_ = true;
      confirm_neutral_ = 0;
    } else if (action == MenuAction::Back) {
      was_latched = back_latched_;
      back_latched_ = true;
      back_event_ = true;
      back_neutral_ = 0;
    }
    if (!focused_ || !armed_ || closing_ || was_latched)
      return false;

    switch (action) {
    case MenuAction::Left:
      quantity_.adjust(-1);
      return true;
    case MenuAction::Right:
      quantity_.adjust(1);
      return true;
    case MenuAction::Up:
      row_ = (row_ + 6) % 7;
      return true;
    case MenuAction::Down:
      row_ = (row_ + 1) % 7;
      return true;
    case MenuAction::Confirm:
      switch (row_) {
      case 1:
        quantity_.preset(1);
        break;
      case 2:
        quantity_.preset(5);
        break;
      case 3:
        quantity_.preset(10);
        break;
      case 4:
        quantity_.preset(quantity_.maximum());
        break;
      case 5:
        confirmed_quantity_ = quantity_.value();
        if (confirmation_count_ != std::numeric_limits<std::uint32_t>::max())
          ++confirmation_count_;
        break;
      case 6:
        begin_close();
        break;
      default:
        break;
      }
      return true;
    case MenuAction::Back:
      begin_close();
      return true;
    case MenuAction::None:
      return false;
    }
    return false;
  }

  void observe(bool confirm_down, bool back_down, bool focused) noexcept {
    if (!focused) {
      focused_ = false;
      armed_ = false;
      closing_ = false;
      close_ready_ = false;
      confirm_latched_ = back_latched_ = true;
      neutral_frames_ = confirm_neutral_ = back_neutral_ = 0;
      confirm_event_ = back_event_ = false;
      return;
    }
    focused_ = true;
    const bool confirm_active = confirm_down || confirm_event_;
    const bool back_active = back_down || back_event_;
    advance_neutral(confirm_neutral_, confirm_active);
    advance_neutral(back_neutral_, back_active);
    advance_neutral(neutral_frames_, confirm_active || back_active);
    if (confirm_neutral_ == 2)
      confirm_latched_ = false;
    if (back_neutral_ == 2)
      back_latched_ = false;
    if (neutral_frames_ == 2) {
      if (closing_)
        close_ready_ = true;
      else
        armed_ = true;
    }
    confirm_event_ = back_event_ = false;
  }

  std::uint32_t quantity() const noexcept { return quantity_.value(); }
  std::uint32_t maximum() const noexcept { return quantity_.maximum(); }
  unsigned row() const noexcept { return row_; }
  std::uint32_t confirmation_count() const noexcept { return confirmation_count_; }
  std::uint32_t confirmed_quantity() const noexcept { return confirmed_quantity_; }
  bool armed() const noexcept { return armed_; }
  bool focused() const noexcept { return focused_; }
  bool closing() const noexcept { return closing_; }
  bool close_ready() const noexcept { return close_ready_; }

private:
  static void advance_neutral(unsigned& count, bool active) noexcept {
    if (active)
      count = 0;
    else if (count < 2)
      ++count;
  }
  void begin_close() noexcept {
    closing_ = true;
    armed_ = false;
    neutral_frames_ = 0;
  }

  Quantity quantity_{23};
  unsigned row_ = 0;
  std::uint32_t confirmation_count_ = 0;
  std::uint32_t confirmed_quantity_ = 0;
  bool focused_ = false;
  bool armed_ = false;
  bool closing_ = false;
  bool close_ready_ = false;
  bool confirm_latched_ = true;
  bool back_latched_ = true;
  bool confirm_event_ = false;
  bool back_event_ = false;
  unsigned neutral_frames_ = 0;
  unsigned confirm_neutral_ = 0;
  unsigned back_neutral_ = 0;
};

} // namespace ubc
