// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/text/ui_language.hpp"
#include "ubc/ui/crafting_block_text.hpp"

namespace equipped_test {
inline bool localized(const ubc::CraftingState& row) {
  using Reason = ubc::CraftingBlockReason;
  using Label = ubc::ui_text::Label;
  const auto gate = ubc::crafting_block_reason(row);
  const auto reason = ubc::crafting_display_reason(gate, &row);
  if (row.maximum || row.material_verified || gate != Reason::material_unverified ||
      reason != Reason::equipped_ingredient)
    return false;
  for (const auto& locale : ubc::ui_text::locales) {
    const auto& text = *locale.texts;
    const auto shown = ubc::crafting_ui::block_text(reason, text);
    if (std::string_view(shown.hint) != text[Label::hint_equipped_ingredient] ||
        std::string_view(shown.detail) != text[Label::equipped_ingredient])
      return false;
  }
  return true;
}
} // namespace equipped_test
