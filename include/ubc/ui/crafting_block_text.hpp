// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/text/table.hpp"

namespace ubc::crafting_ui {
struct BlockText {
  const char* detail;
  const char* hint;
};
inline BlockText block_text(CraftingBlockReason reason, const ui_text::Table& text) noexcept {
  using L = ui_text::Label;
  switch (reason) {
  case CraftingBlockReason::none:
    return {"", ""};
  case CraftingBlockReason::no_known_recipes:
    return {text[L::no_recipes], text[L::hint_no_known_recipes]};
  case CraftingBlockReason::contract_unverified:
    return {text[L::unverified], text[L::hint_contract_unverified]};
  case CraftingBlockReason::actor_unknown:
    return {text[L::no_actor], text[L::hint_actor_unknown]};
  case CraftingBlockReason::actor_changed:
    return {text[L::wrong_actor], text[L::hint_actor_changed]};
  case CraftingBlockReason::hand_unknown:
    return {text[L::hand_not_ready], text[L::hint_hand_unknown]};
  case CraftingBlockReason::material_unknown:
    return {text[L::stock_unreadable], text[L::hint_material_unknown]};
  case CraftingBlockReason::material_unverified:
    return {text[L::stock_unverified], text[L::hint_material_unknown]};
  case CraftingBlockReason::equipped_ingredient:
    return {text[L::equipped_ingredient], text[L::hint_equipped_ingredient]};
  case CraftingBlockReason::missing_material:
    return {text[L::missing_material], text[L::hint_missing_material]};
  case CraftingBlockReason::output_capacity:
    return {text[L::output_full], text[L::hint_output_capacity]};
  case CraftingBlockReason::tool_return_capacity:
    return {text[L::tool_full], text[L::hint_output_capacity]};
  case CraftingBlockReason::unavailable:
    return {text[L::unavailable], text[L::hint_unavailable]};
  case CraftingBlockReason::quantity_unsupported:
    return {text[L::unsupported_amount], text[L::hint_quantity_unsupported]};
  }
  return {text[L::unavailable], text[L::hint_unavailable]};
}
} // namespace ubc::crafting_ui
