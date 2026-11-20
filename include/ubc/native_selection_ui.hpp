// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/native_menu_abi.hpp"
#include "ubc/ui_language.hpp"
#include "ubc/selection_session.hpp"
#include "ubc/selection_stock.hpp"

namespace ubc {
struct BowMenuTrace;
// On the bound game thread, outside the input callback iteration.
// No own render/event loop. HandleEvent only changes the model.
class NativeSelectionUi {
public:
  const ui_text::Table& texts() const noexcept { return ui_text::table(language_); }
  bool can_open(BowMenuTrace* trace) const noexcept;
  bool open(BowMenuTrace* trace=nullptr,BowMenuTrace* opened_trace=nullptr) noexcept;
  void close() noexcept;
  bool render(const SelectionSession&, OriginalRecipe, bool execution_enabled) noexcept;
  int handle(SelectionSession&, int key) noexcept;
  void* object() noexcept { return created_ ? storage_.bytes.data() : nullptr; }
  bool opened() const noexcept { return initialized_; }
  bool owns_input() const noexcept;
  static bool focused() noexcept;
  static bool buttons_held(bool* known=nullptr) noexcept;
  static bool callback_is_top(std::uint32_t callback) noexcept;
  static std::uint32_t top_callback() noexcept;
private:
  native_menu::MenuStorage storage_{};
  bool created_ = false, initialized_ = false;
  ui_text::Language language_ = &ui_text::en::table;
  unsigned row_ = 0;
};
} // namespace ubc
