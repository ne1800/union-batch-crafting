// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/core/recipe_session.hpp"

namespace ubc {

enum class RecipeMenuRow { recipe, quantity, presets, steps, manufacture, original };
enum class RecipeMenuAction {
  none,
  left,
  right,
  up,
  down,
  confirm,
  back,
  next,
  previous,
  page_up,
  page_down
};

class RecipeMenu {
public:
  void reset() noexcept { *this = RecipeMenu{}; }
  RecipeMenuRow row() const noexcept { return row_; }
  unsigned column() const noexcept {
    return row_ == RecipeMenuRow::presets ? preset_ : row_ == RecipeMenuRow::steps ? step_ : 0;
  }
  static std::uint32_t preset_value(unsigned index, std::uint32_t maximum) noexcept {
    switch (index) {
    case 0:
      return 1;
    case 1:
      return 5;
    case 2:
      return 10;
    case 3:
      return maximum;
    }
    return 0;
  }
  static bool preset_enabled(unsigned index, std::uint32_t maximum) noexcept {
    const auto value = preset_value(index, maximum);
    return value && value <= maximum;
  }
  bool on_action(RecipeMenuAction action, RecipeSession& session) noexcept {
    if (action == RecipeMenuAction::back) {
      if (session.phase() != SelectionPhase::selecting || row_ == RecipeMenuRow::recipe)
        return session.back();
      if (!session.acknowledge())
        return false;
      row_ = RecipeMenuRow::recipe;
      return true;
    }
    if (action == RecipeMenuAction::confirm) {
      switch (row_) {
      case RecipeMenuRow::recipe:
        if (!session.acknowledge() || !session.maximum())
          return false;
        row_ = RecipeMenuRow::quantity;
        return true;
      case RecipeMenuRow::quantity:
        if (!session.acknowledge() || !session.can_confirm())
          return false;
        row_ = RecipeMenuRow::manufacture;
        return true;
      case RecipeMenuRow::presets:
        return session.preset(preset_value(preset_, session.maximum()));
      case RecipeMenuRow::steps:
        return session.adjust(step_ ? 5 : -5, true);
      case RecipeMenuRow::manufacture:
        return session.confirm();
      case RecipeMenuRow::original:
        return session.back();
      }
    }
    if (!session.armed() || session.phase() != SelectionPhase::selecting)
      return false;
    switch (action) {
    case RecipeMenuAction::left:
      return horizontal(-1, session);
    case RecipeMenuAction::right:
      return horizontal(1, session);
    case RecipeMenuAction::up:
      return vertical(-1, session);
    case RecipeMenuAction::down:
      return vertical(1, session);
    case RecipeMenuAction::next:
      return tab(1, session);
    case RecipeMenuAction::previous:
      return tab(-1, session);
    case RecipeMenuAction::page_up:
      return session.adjust(5);
    case RecipeMenuAction::page_down:
      return session.adjust(-5);
    default:
      return false;
    }
  }

private:
  bool horizontal(int direction, RecipeSession& session) noexcept {
    if (row_ == RecipeMenuRow::recipe) {
      if (direction > 0 && session.maximum()) {
        row_ = RecipeMenuRow::quantity;
        return true;
      }
      return false;
    }
    if (row_ == RecipeMenuRow::quantity)
      return session.adjust(direction);
    if (row_ == RecipeMenuRow::presets) {
      for (int candidate = static_cast<int>(preset_) + direction; candidate >= 0 && candidate < 4;
           candidate += direction)
        if (preset_enabled(static_cast<unsigned>(candidate), session.maximum())) {
          preset_ = static_cast<unsigned>(candidate);
          return true;
        }
    } else if (row_ == RecipeMenuRow::steps) {
      const auto candidate = static_cast<int>(step_) + direction;
      if (candidate >= 0 && candidate < 2) {
        step_ = static_cast<unsigned>(candidate);
        return true;
      }
    }
    return false;
  }
  static bool enabled_row(RecipeMenuRow row, const RecipeSession& session) noexcept {
    if (row == RecipeMenuRow::recipe || row == RecipeMenuRow::original)
      return true;
    if (row == RecipeMenuRow::manufacture)
      return session.can_confirm();
    return session.maximum() != 0;
  }
  bool vertical(int direction, RecipeSession& session) noexcept {
    if (row_ == RecipeMenuRow::recipe)
      return session.select_relative(direction);
    for (int candidate = static_cast<int>(row_) + direction; candidate >= 0 && candidate < 6;
         candidate += direction) {
      const auto next = static_cast<RecipeMenuRow>(candidate);
      if (!enabled_row(next, session))
        continue;
      row_ = next;
      if (row_ == RecipeMenuRow::presets && !preset_enabled(preset_, session.maximum()))
        preset_ = 0;
      return true;
    }
    return false;
  }
  int tab_position() const noexcept {
    switch (row_) {
    case RecipeMenuRow::recipe:
      return 0;
    case RecipeMenuRow::quantity:
      return 1;
    case RecipeMenuRow::presets:
      return 2 + static_cast<int>(preset_);
    case RecipeMenuRow::steps:
      return 6 + static_cast<int>(step_);
    case RecipeMenuRow::manufacture:
      return 8;
    case RecipeMenuRow::original:
      return 9;
    }
    return 0;
  }
  static bool enabled_tab(int position, const RecipeSession& session) noexcept {
    if (position == 0 || position == 9)
      return true;
    if (position == 8)
      return session.can_confirm();
    if (position >= 2 && position <= 5)
      return preset_enabled(static_cast<unsigned>(position - 2), session.maximum());
    return session.maximum() != 0;
  }
  bool tab(int direction, const RecipeSession& session) noexcept {
    int position = tab_position();
    for (int n = 0; n < 10; ++n) {
      position = (position + direction + 10) % 10;
      if (!enabled_tab(position, session))
        continue;
      if (position == 0)
        row_ = RecipeMenuRow::recipe;
      else if (position == 1)
        row_ = RecipeMenuRow::quantity;
      else if (position <= 5) {
        row_ = RecipeMenuRow::presets;
        preset_ = static_cast<unsigned>(position - 2);
      } else if (position <= 7) {
        row_ = RecipeMenuRow::steps;
        step_ = static_cast<unsigned>(position - 6);
      } else if (position == 8)
        row_ = RecipeMenuRow::manufacture;
      else
        row_ = RecipeMenuRow::original;
      return true;
    }
    return false;
  }
  RecipeMenuRow row_ = RecipeMenuRow::recipe;
  unsigned preset_ = 0, step_ = 0;
};

} // namespace ubc
