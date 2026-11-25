// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/ui/crafting_menu.hpp"
#include "ubc/ui/crafting_menu_code.hpp"
#include "ubc/ui/crafting_menu_view.hpp"
#include "ubc/ui/crafting_ui_diagnostics.hpp"
#include "ubc/stations/grind/grind_ui_readiness.hpp"
#include "ubc/ui/native_selection_ui.hpp"

namespace ubc {
class StationQuantity;
struct CraftingResultSuccess;
struct NativeCraftingProduced {
  std::int32_t symbol = -1, amount = -1;
};
struct NativeCraftingStock {
  std::int32_t symbol = -1, stored = -1;
};
struct NativeCraftingSuccess {
  // Only after confirmed material completion AND byte restore.
  // Callback return alone must not set these proofs. Quantities come
  // from the result watcher, never from a repeated crafting call.
  bool result_verified = false, restore_verified = false;
  std::uint64_t receipt = 0;
  CraftingKey key{};
  crafting_catalog::RecipeId id = UINT32_MAX;
  std::uint32_t quantity = 0;
  std::size_t produced_count = 0, stock_count = 0;
  std::array<NativeCraftingProduced, crafting_catalog::target_capacity> produced{};
  std::array<NativeCraftingStock, crafting_catalog::target_capacity> stocks{};
  bool branch_known = false;
  std::int32_t branch_value = 0, branch_output = -1;
};

namespace crafting_ui {
// Pure bounded text layout, verifiable even without Windows/Engine. Pixel
// values only determine the width variant; x/y use the existing 8192 view
// coordinates. A passed layout test is not an in-game visibility proof.
struct Line {
  int x = 0, y = 0;
  std::size_t character_limit = 0;
  std::array<char, 192> text{};
};
struct Frame {
  bool valid = false;
  std::size_t count = 0;
  std::array<Line, 64> lines{};
};
bool valid_success(const NativeCraftingSuccess&) noexcept;
NativeCraftingSuccess success_notice(const CraftingResultSuccess&) noexcept;
bool compose(Frame&, const CraftingSession&, CraftingMenu&, bool execution_enabled, unsigned width,
             unsigned height, const ui_text::Table& text) noexcept;
bool compose_success(Frame&, const NativeCraftingSuccess&, unsigned width, unsigned height,
                     const ui_text::Table& text) noexcept;
template <class Submit>
std::uint64_t submit_success(Frame& frame, const NativeCraftingSuccess& notice, unsigned width,
                             unsigned height, const ui_text::Table& text, Submit submit) noexcept {
  return compose_success(frame, notice, width, height, text) && submit(frame) ? notice.receipt : 0;
}
bool compose_failure(Frame&, unsigned width, unsigned height, const ui_text::Table& text) noexcept;
} // namespace crafting_ui

// Same already-tested native menu context. No hooks, own render loops,
// script/inventory/input configuration changes. Root binds the game
// thread and context before/after rendering and closes on context end.
class NativeCraftingUi {
public:
  void inspect_open(BowMenuTrace&) const noexcept;
  bool open(BowMenuTrace* trace = nullptr, BowMenuTrace* opened_trace = nullptr) noexcept;
  const char* open_rejection() const noexcept { return window_.open_rejection(); }
  bool uses_workshop_screen_init() const noexcept { return window_.uses_workshop_screen_init(); }
  crafting_ui::BackdropResult apply_backdrop() noexcept;
  bool has_valid_view() noexcept { return menu_view().get() != nullptr; }
  bool text_view_rejected() const noexcept { return text_view_rejected_; }
  const char* text_rejection() const noexcept { return text_rejection_; }
  bool open_failure() noexcept;
  bool open_success(const NativeCraftingSuccess&) noexcept;
  void close() noexcept;
  bool render(const CraftingSession&, bool execution_enabled) noexcept;
#ifdef UBC_ENABLE_DNDR
  bool render_station_quantity(const StationQuantity&) noexcept;
  static CraftingMenuAction station_action(int key) noexcept;
  void show_forge_progress(unsigned completed, unsigned selected) noexcept;
#endif
  bool render_failure() noexcept;
  bool render_success() noexcept;
  int handle(CraftingSession&, int key,
             crafting_ui_diagnostics::Action* diagnostic = nullptr) noexcept;
  crafting_ui_diagnostics::State diagnostic_state(const CraftingSession& session) const noexcept {
    return crafting_ui_diagnostics::state(menu_, session);
  }
  grind_ui_readiness::State readiness_state(const CraftingSession& session) const noexcept {
    return grind_ui_readiness::state(menu_, session);
  }
  bool failure_notice() const noexcept { return mode_ == Mode::failure; }
  bool success_notice() const noexcept { return mode_ == Mode::success; }
  bool notice_dismissed() const noexcept { return dismiss_; }
  void dismiss_notice() noexcept {
    if (!dismiss_) {
      dismiss_ = true;
      notice_input_.disarm();
    }
  }
  void observe_notice(std::uint64_t frame, bool held, bool focused) noexcept {
    notice_input_.observe(frame, held, focused);
  }
  bool notice_return_ready() const noexcept {
    return mode_ != Mode::selection && dismiss_ && notice_input_.armed();
  }
  // Only after successful native text submission. Root may report only registration
  // with the UI. This does not establish physical visibility or controller operation.
  std::uint64_t success_receipt() const noexcept {
    return mode_ == Mode::success ? submitted_receipt_ : 0;
  }
  const NativeCraftingSuccess& success() const noexcept { return success_; }
  void* object() noexcept { return window_.object(); }
  bool opened() const noexcept { return window_.opened(); }
  bool owns_input() const noexcept { return window_.owns_input(); }
  static bool focused() noexcept { return NativeSelectionUi::focused(); }
  static bool buttons_held(bool* known = nullptr) noexcept {
    return NativeSelectionUi::buttons_held(known);
  }
  static bool callback_is_top(std::uint32_t callback) noexcept {
    return NativeSelectionUi::callback_is_top(callback);
  }

private:
  enum class Mode { selection, failure, success };
  crafting_ui::MenuView menu_view() noexcept;
  bool notice_window() noexcept;
  bool update_size() noexcept;
  bool submit() noexcept;
  NativeSelectionUi window_{};
  CraftingMenu menu_{};
  SelectionInput notice_input_{};
  crafting_ui::Frame frame_{};
  NativeCraftingSuccess success_{};
  std::uint64_t last_receipt_ = 0, submitted_receipt_ = 0;
  unsigned width_ = 0, height_ = 0;
  Mode mode_ = Mode::selection;
  bool dismiss_ = false;
  bool text_view_rejected_ = false;
  const char* text_rejection_ = nullptr;
};
} // namespace ubc
