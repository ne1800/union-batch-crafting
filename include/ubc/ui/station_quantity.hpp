// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/ui/crafting_menu.hpp"
#include "ubc/core/quantity.hpp"

namespace ubc {
class StationQuantity {
public:
  void open(unsigned maximum) noexcept {
    amount_.reset(maximum);
    row_ = 0;
    decision_ = -1;
    input_ = {};
  }
  void observe(std::uint64_t frame, bool held, bool focused) noexcept {
    input_.observe(frame, held, focused);
    if (!focused)
      decision_ = 0;
  }
  void handle(CraftingMenuAction action) noexcept {
    using A = CraftingMenuAction;
    if (decision_ >= 0)
      return;
    if (action == A::back || action == A::confirm) {
      if (input_.press())
        decision_ = action == A::back ? 0 : static_cast<int>(selected());
      return;
    }
    if (!input_.armed())
      return;
    switch (action) {
    case A::up:
    case A::previous:
      row_ = (row_ + 2) % 3;
      break;
    case A::down:
    case A::next:
      row_ = (row_ + 1) % 3;
      break;
    case A::left:
    case A::right:
      row_ = 1;
      amount_.adjust(action == A::right ? 1 : -1);
      break;
    default:
      break;
    }
  }
  unsigned row() const noexcept { return row_; }
  unsigned amount() const noexcept { return amount_.value(); }
  unsigned maximum() const noexcept { return amount_.maximum(); }
  unsigned selected() const noexcept { return row_ == 0 ? 1 : row_ == 2 ? maximum() : amount(); }
  int decision() const noexcept { return decision_; }
  bool released() const noexcept { return input_.armed(); }

private:
  Quantity amount_;
  SelectionInput input_;
  unsigned row_ = 0;
  int decision_ = -1;
};
} // namespace ubc
