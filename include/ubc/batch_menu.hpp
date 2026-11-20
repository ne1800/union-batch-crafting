// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/batch_session.hpp"

namespace ubc {

enum class BatchMenuRow { recipe, quantity, presets, steps, manufacture, original };
enum class BatchMenuAction { none, left, right, up, down, confirm, back,
                             next, previous, page_up, page_down };

// A recipe in the current station binding. Native events are mapped
// only to these actions; the model knows neither key codes nor engine.
class BatchMenu {
public:
  void reset() noexcept { *this = BatchMenu{}; }
  BatchMenuRow row() const noexcept { return row_; }
  unsigned column() const noexcept {
    if (row_ == BatchMenuRow::presets) return preset_;
    if (row_ == BatchMenuRow::steps) return step_;
    return 0;
  }
  static std::uint32_t preset_value(unsigned index, std::uint32_t maximum) noexcept {
    switch (index) { case 0: return 1; case 1: return 5; case 2: return 10; case 3: return maximum; }
    return 0;
  }
  static bool preset_enabled(unsigned index, std::uint32_t maximum) noexcept {
    const auto value = preset_value(index, maximum);
    return value && value <= maximum;
  }

  bool on_action(BatchMenuAction action, BatchSession& session) noexcept {
    if (action == BatchMenuAction::back) {
      if (session.phase() != SelectionPhase::selecting || row_ == BatchMenuRow::recipe)
        return session.back();
      if (!session.acknowledge()) return false;
      row_ = BatchMenuRow::recipe;
      return true;
    }
    if (action == BatchMenuAction::confirm) {
      switch (row_) {
        case BatchMenuRow::recipe:
          if (!session.acknowledge() || !session.maximum()) return false;
          row_ = BatchMenuRow::quantity;
          return true;
        case BatchMenuRow::quantity:
          if (!session.acknowledge() || !session.maximum()) return false;
          row_ = BatchMenuRow::manufacture;
          return true;
        case BatchMenuRow::presets:
          return session.preset(preset_value(preset_, session.maximum()));
        case BatchMenuRow::steps:
          return session.adjust(step_ ? 5 : -5, true);
        case BatchMenuRow::manufacture:
          return session.confirm();
        case BatchMenuRow::original:
          return session.back();
      }
    }
    if (!session.armed() || session.phase() != SelectionPhase::selecting) return false;
    switch (action) {
      case BatchMenuAction::left: return horizontal(-1, session);
      case BatchMenuAction::right: return horizontal(1, session);
      case BatchMenuAction::up: return vertical(-1, session.maximum());
      case BatchMenuAction::down: return vertical(1, session.maximum());
      case BatchMenuAction::next: return tab(1, session.maximum());
      case BatchMenuAction::previous: return tab(-1, session.maximum());
      case BatchMenuAction::page_up: return session.adjust(5);
      case BatchMenuAction::page_down: return session.adjust(-5);
      default: return false;
    }
  }

private:
  bool horizontal(int direction, BatchSession& session) noexcept {
    if (row_ == BatchMenuRow::quantity) return session.adjust(direction);
    if (row_ == BatchMenuRow::recipe && direction > 0 && session.maximum()) {
      row_ = BatchMenuRow::quantity;
      return true;
    }
    if (row_ == BatchMenuRow::presets) {
      for (int candidate = static_cast<int>(preset_) + direction;
           candidate >= 0 && candidate < 4; candidate += direction) {
        const auto index = static_cast<unsigned>(candidate);
        if (preset_enabled(index, session.maximum())) { preset_ = index; return true; }
      }
    } else if (row_ == BatchMenuRow::steps) {
      const int candidate = static_cast<int>(step_) + direction;
      if (candidate >= 0 && candidate < 2) { step_ = static_cast<unsigned>(candidate); return true; }
    }
    return false;
  }
  static bool enabled_row(BatchMenuRow row, std::uint32_t maximum) noexcept {
    return maximum || row == BatchMenuRow::recipe || row == BatchMenuRow::original;
  }
  bool vertical(int direction, std::uint32_t maximum) noexcept {
    for (int candidate = static_cast<int>(row_) + direction;
         candidate >= 0 && candidate < 6; candidate += direction) {
      const auto next = static_cast<BatchMenuRow>(candidate);
      if (!enabled_row(next, maximum)) continue;
      row_ = next;
      if (row_ == BatchMenuRow::presets && !preset_enabled(preset_, maximum)) preset_ = 0;
      return true;
    }
    return false;
  }
  int tab_position() const noexcept {
    switch (row_) {
      case BatchMenuRow::recipe: return 0;
      case BatchMenuRow::quantity: return 1;
      case BatchMenuRow::presets: return 2 + static_cast<int>(preset_);
      case BatchMenuRow::steps: return 6 + static_cast<int>(step_);
      case BatchMenuRow::manufacture: return 8;
      case BatchMenuRow::original: return 9;
    }
    return 0;
  }
  static bool enabled_tab(int position, std::uint32_t maximum) noexcept {
    if (position == 0 || position == 9) return true;
    if (position >= 2 && position <= 5)
      return preset_enabled(static_cast<unsigned>(position - 2), maximum);
    return maximum != 0;
  }
  bool tab(int direction, std::uint32_t maximum) noexcept {
    int position = tab_position();
    for (int n = 0; n < 10; ++n) {
      position = (position + direction + 10) % 10;
      if (!enabled_tab(position, maximum)) continue;
      if (position == 0) row_ = BatchMenuRow::recipe;
      else if (position == 1) row_ = BatchMenuRow::quantity;
      else if (position <= 5) { row_ = BatchMenuRow::presets; preset_ = static_cast<unsigned>(position - 2); }
      else if (position <= 7) { row_ = BatchMenuRow::steps; step_ = static_cast<unsigned>(position - 6); }
      else if (position == 8) row_ = BatchMenuRow::manufacture;
      else row_ = BatchMenuRow::original;
      return true;
    }
    return false;
  }

  BatchMenuRow row_ = BatchMenuRow::recipe;
  unsigned preset_ = 0, step_ = 0;
};

} // namespace ubc
