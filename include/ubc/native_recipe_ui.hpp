// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/recipe_menu.hpp"
#include "ubc/native_selection_ui.hpp"

namespace ubc {
// Same tested non-modal menu context, separate quantity/focus model.
class NativeRecipeUi {
public:
  bool open() noexcept { failure_=false; menu_.reset(); return window_.open(); }
  bool open_failure() noexcept { failure_=true; dismiss_=false; failure_input_={}; return window_.open(); }
  bool failure_notice() const noexcept { return failure_; }
  void dismiss_failure() noexcept { if(!dismiss_){dismiss_=true;failure_input_.disarm();} }
  void observe_failure(std::uint64_t frame,bool held,bool focused) noexcept { failure_input_.observe(frame,held,focused); }
  bool failure_return_ready() const noexcept { return dismiss_&&failure_input_.armed(); }
  bool render_failure() noexcept;
  void close() noexcept { window_.close(); }
  bool render(const RecipeSession&, bool execution_enabled) noexcept;
  int handle(RecipeSession&, int key) noexcept;
  void* object() noexcept { return window_.object(); }
  bool opened() const noexcept { return window_.opened(); }
  bool owns_input() const noexcept { return window_.owns_input(); }
  static bool focused() noexcept { return NativeSelectionUi::focused(); }
  static bool buttons_held() noexcept { return NativeSelectionUi::buttons_held(); }
  static bool callback_is_top(std::uint32_t callback) noexcept { return NativeSelectionUi::callback_is_top(callback); }
private:
  NativeSelectionUi window_{};
  RecipeMenu menu_{};
  SelectionInput failure_input_{};
  bool failure_=false,dismiss_=false;
};
} // namespace ubc
