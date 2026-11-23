// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/text/codepage.hpp"
#include <iterator>
#include <type_traits>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ubc::ui_text {
enum class Label {
  title,
  no_recipes,
  unverified,
  no_actor,
  wrong_actor,
  hand_not_ready,
  stock_unreadable,
  stock_unverified,
  equipped_ingredient,
  missing_material,
  output_full,
  tool_full,
  unavailable,
  unsupported_amount,
  max_short,
  selected_recipes,
  known_recipes,
  hint_no_known_recipes,
  hint_contract_unverified,
  hint_actor_unknown,
  hint_actor_changed,
  hint_hand_unknown,
  hint_material_unknown,
  hint_equipped_ingredient,
  hint_missing_material,
  hint_output_capacity,
  hint_unavailable,
  hint_quantity_unsupported,
  list_range,
  select_recipe,
  quantity,
  bow_quantity,
  zero_quantity,
  no_quantity,
  grind_progress,
  grind_skill,
  tool_held,
  hand_needed,
  hand_unknown,
  stock_and_hand_unknown,
  hand_cost,
  possession,
  minimum,
  output_suffix,
  tool_stock_unknown,
  return_unknown,
  stock_unknown,
  output,
  required,
  output_balance,
  missing_minimum,
  shortfall,
  one_batch,
  original_controls,
  craft,
  test_amount,
  locked_suffix,
  original_action,
  controls,
  controls_short,
  key_choose,
  key_continue,
  key_preset,
  key_step,
  key_confirm,
  key_original,
  browse_recipes,
  adjust_amount,
  browse_buttons,
  change_field,
  release_continue,
  release_back,
  release_buttons,
  stock_changed,
  focus_lost,
  success,
  completed,
  updated_stock,
  tool_suffix,
  back_to_recipes,
  no_repeat,
  failure,
  uncertain,
  locked_session,
  quit_unsaved,
  dismiss,
  failure_title,
  uncertain_recipe,
  diagnostic_log,
  dismiss_probe,
  batch_amount,
  maximum,
  disabled_preset,
  remaining,
  after,
  not_enough,
  test_only,
  disabled_suffix,
  batch_controls,
  release_craft,
  release_test,
  choose_again,
  reconfirm,
  material,
  station_no_recipes,
  station_unavailable,
  station_not_ready,
  wait_stock,
  marked_unavailable,
  recipe_amount,
  adjust,
  item_unknown,
  item_missing,
  item_after,
  recipe_controls,
  original_amounts,
  available,
  test_selection,
  selection_controls,
  selection_changed,
  probe_title,
  probe_scope,
  probe_demo,
  probe_amount,
  preset_one,
  preset_five,
  preset_ten,
  preset_max,
  probe_confirm,
  probe_main,
  probe_release_close,
  probe_release,
  probe_result,
  probe_controls,
  unknown_item,
  batches,
  lots,
  blades,
  weapons,
  flasks,
  healing_plants,
  healing_potions,
  healing_herbs,
  healing_roots,
  fire_nettles,
  fire_weeds,
  fire_roots,
  healing_essences,
  healing_extracts,
  healing_elixirs,
  mana_essences,
  mana_extracts,
  mana_elixirs,
  raw_molerat_legs,
  molerat_dishes,
  dark_mushrooms,
  mushroom_dishes,
  count
};

struct Message {
  Label key;
  const char* text;
};
struct Name {
  std::uint32_t id;
  const char* text;
};

template <unsigned Page, auto& Rows>
inline constexpr auto encoded_rows = [] {
  using Entry = std::conditional_t<std::is_same_v<decltype(Rows[0].key), Label>, Message, Name>;
  std::array<Entry, std::size(Rows)> result{};
  std::size_t index = 0, offset = 0;
  for (const auto& row : Rows) {
    result[index++] = {row.key, encoded_storage<Page, Rows>.data() + offset};
    offset += row.text.size() + 1;
  }
  return result;
}();

struct Names {
  const Name* entries;
  std::size_t count;
  constexpr const char* find(std::uint32_t id, const char* fallback) const noexcept {
    std::size_t first = 0, last = count;
    while (first < last) {
      const auto middle = first + (last - first) / 2;
      if (entries[middle].id < id)
        first = middle + 1;
      else
        last = middle;
    }
    return first < count && entries[first].id == id ? entries[first].text : fallback;
  }
};

struct Table {
  std::array<const char*, static_cast<std::size_t>(Label::count)> labels{};
  Names items, recipes;

  template <std::size_t M, std::size_t I, std::size_t R>
  constexpr Table(const std::array<Message, M>& values, const std::array<Name, I>& materials,
                  const std::array<Name, R>& names)
      : items{materials.data(), I}, recipes{names.data(), R} {
    static_assert(M == static_cast<std::size_t>(Label::count));
    for (const auto& entry : values)
      labels[static_cast<std::size_t>(entry.key)] = entry.text;
  }
  constexpr const char* operator[](Label key) const noexcept {
    return labels[static_cast<std::size_t>(key)];
  }
  constexpr const char* item(std::int32_t symbol) const noexcept {
    return items.find(static_cast<std::uint32_t>(symbol), (*this)[Label::unknown_item]);
  }
  constexpr const char* recipe(std::uint32_t id) const noexcept {
    return recipes.find(id, (*this)[Label::select_recipe]);
  }
};
} // namespace ubc::ui_text
