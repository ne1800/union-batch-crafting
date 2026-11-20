// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/core/crafting_session.hpp"

namespace ubc {

enum class CraftingMenuRow { recipe, quantity, presets, steps, manufacture, original };
enum class CraftingMenuAction { none, left, right, up, down, confirm, back,
                              next, previous, page_up, page_down };
struct CraftingViewport {
  std::size_t first=0,count=0,selected=0;
  bool has_selection=false;
};

class CraftingMenu {
public:
  void reset() noexcept { *this = CraftingMenu{}; }
  CraftingMenuRow row() const noexcept { return row_; }
  // Only visible, known recipes. The index refers to the filtered
  // list of the session, not the global catalog position. Even nine
  // leather recipes remain reachable with a smaller native text window.
  CraftingViewport viewport(const CraftingSession& session,std::size_t lines) noexcept {
    const auto total=session.visible_count(),selected=session.selected_index();
    const auto generation=session.preview().key.generation;
    if(generation!=viewport_generation_){viewport_generation_=generation;first_=0;}
    if(!lines||!total){first_=0;return {};}
    const auto count=lines<total?lines:total;
    if(first_>total-count)first_=total-count;
    if(selected<total) {
      if(selected<first_)first_=selected;
      else if(selected-first_>=count)first_=selected-count+1;
    }
    return {first_,count,selected<total?selected-first_:0,selected<total};
  }
  unsigned column() const noexcept {
    return row_ == CraftingMenuRow::presets ? preset_ : row_ == CraftingMenuRow::steps ? step_ : 0;
  }
  static std::uint32_t preset_value(unsigned index, std::uint32_t maximum) noexcept {
    switch (index) { case 0: return 1; case 1: return 5; case 2: return 10; case 3: return maximum; }
    return 0;
  }
  static bool preset_enabled(unsigned index, std::uint32_t maximum) noexcept {
    const auto value = preset_value(index, maximum);
    return value && value <= maximum;
  }
  bool on_action(CraftingMenuAction action, CraftingSession& session) noexcept {
    if (action == CraftingMenuAction::back) {
      if (session.phase() != SelectionPhase::selecting || row_ == CraftingMenuRow::recipe) return session.back();
      if (!session.acknowledge()) return false;
      row_ = CraftingMenuRow::recipe; return true;
    }
    if (action == CraftingMenuAction::confirm) {
      switch (row_) {
        case CraftingMenuRow::recipe:
          if (!session.acknowledge() || !session.maximum()) return false;
          row_ = CraftingMenuRow::quantity; return true;
        case CraftingMenuRow::quantity:
          if (!session.acknowledge() || !session.can_confirm()) return false;
          row_ = CraftingMenuRow::manufacture; return true;
        case CraftingMenuRow::presets: return session.preset(preset_value(preset_, session.maximum()));
        case CraftingMenuRow::steps: return session.adjust(step_ ? 5 : -5, true);
        case CraftingMenuRow::manufacture: return session.confirm();
        case CraftingMenuRow::original: return session.back();
      }
    }
    if (!session.armed() || session.phase() != SelectionPhase::selecting) return false;
    switch (action) {
      case CraftingMenuAction::left: return horizontal(-1, session);
      case CraftingMenuAction::right: return horizontal(1, session);
      case CraftingMenuAction::up: return vertical(-1, session);
      case CraftingMenuAction::down: return vertical(1, session);
      case CraftingMenuAction::next: return tab(1, session);
      case CraftingMenuAction::previous: return tab(-1, session);
      case CraftingMenuAction::page_up: return session.adjust(5);
      case CraftingMenuAction::page_down: return session.adjust(-5);
      default: return false;
    }
  }

private:
  bool horizontal(int direction, CraftingSession& session) noexcept {
    if (row_ == CraftingMenuRow::recipe) {
      if (direction > 0 && session.maximum()) { row_ = CraftingMenuRow::quantity; return true; }
      return false;
    }
    if (row_ == CraftingMenuRow::quantity) return session.adjust(direction);
    if (row_ == CraftingMenuRow::presets) {
      for (int candidate = static_cast<int>(preset_) + direction; candidate >= 0 && candidate < 4; candidate += direction)
        if (preset_enabled(static_cast<unsigned>(candidate), session.maximum())) {
          preset_ = static_cast<unsigned>(candidate); return true;
        }
    } else if (row_ == CraftingMenuRow::steps) {
      const auto candidate = static_cast<int>(step_) + direction;
      if (candidate >= 0 && candidate < 2) { step_ = static_cast<unsigned>(candidate); return true; }
    }
    return false;
  }
  static bool enabled_row(CraftingMenuRow row, const CraftingSession& session) noexcept {
    if (row == CraftingMenuRow::recipe || row == CraftingMenuRow::original) return true;
    if (row == CraftingMenuRow::manufacture) return session.can_confirm();
    return session.maximum() != 0;
  }
  bool vertical(int direction, CraftingSession& session) noexcept {
    if (row_ == CraftingMenuRow::recipe) return session.select_relative(direction);
    for (int candidate = static_cast<int>(row_) + direction; candidate >= 0 && candidate < 6; candidate += direction) {
      const auto next = static_cast<CraftingMenuRow>(candidate);
      if (!enabled_row(next, session)) continue;
      row_ = next;
      if (row_ == CraftingMenuRow::presets && !preset_enabled(preset_, session.maximum())) preset_ = 0;
      return true;
    }
    return false;
  }
  int tab_position() const noexcept {
    switch (row_) {
      case CraftingMenuRow::recipe: return 0;
      case CraftingMenuRow::quantity: return 1;
      case CraftingMenuRow::presets: return 2 + static_cast<int>(preset_);
      case CraftingMenuRow::steps: return 6 + static_cast<int>(step_);
      case CraftingMenuRow::manufacture: return 8;
      case CraftingMenuRow::original: return 9;
    }
    return 0;
  }
  static bool enabled_tab(int position, const CraftingSession& session) noexcept {
    if (position == 0 || position == 9) return true;
    if (position == 8) return session.can_confirm();
    if (position >= 2 && position <= 5) return preset_enabled(static_cast<unsigned>(position - 2), session.maximum());
    return session.maximum() != 0;
  }
  bool tab(int direction, const CraftingSession& session) noexcept {
    int position = tab_position();
    for (int n = 0; n < 10; ++n) {
      position = (position + direction + 10) % 10;
      if (!enabled_tab(position, session)) continue;
      if (position == 0) row_ = CraftingMenuRow::recipe;
      else if (position == 1) row_ = CraftingMenuRow::quantity;
      else if (position <= 5) { row_ = CraftingMenuRow::presets; preset_ = static_cast<unsigned>(position - 2); }
      else if (position <= 7) { row_ = CraftingMenuRow::steps; step_ = static_cast<unsigned>(position - 6); }
      else if (position == 8) row_ = CraftingMenuRow::manufacture;
      else row_ = CraftingMenuRow::original;
      return true;
    }
    return false;
  }
  CraftingMenuRow row_ = CraftingMenuRow::recipe;
  unsigned preset_ = 0, step_ = 0;
  std::size_t first_=0;
  std::uint64_t viewport_generation_=0;
};

} // namespace ubc
