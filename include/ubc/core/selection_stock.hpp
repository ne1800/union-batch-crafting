// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/engine/hand_snapshot.hpp"
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/engine/packed_inventory.hpp"

#include <array>
#include <cstdint>

namespace ubc {

enum class OriginalRecipe { meat, healing_essence };
enum class StockIssue { none, unknown, invalid, ambiguous_ingredient, hands, overflow };

struct SelectionStock {
  StockIssue issue = StockIssue::unknown;
  std::array<std::int32_t, 6> stored{};
  std::array<std::int32_t, 6> reserved{};
  std::uint32_t maximum = 0;
  bool known() const noexcept { return issue == StockIssue::none; }
};

// Only for the verified stove and lab table states. The campfire has
// different hand and station effects and lies outside this adapter contract.
inline SelectionStock selection_stock(OriginalRecipe recipe,
                                      const PackedInventorySnapshot& inventory,
                                      const HandsSnapshot& hands) noexcept {
  SelectionStock result;
  if (!inventory.header_known || !inventory.listed_known || !inventory.packed_known ||
      !inventory.storage_totals_known || !inventory.stable_boundary_verified ||
      !inventory.snapshot_unchanged || inventory.issue != PackedInventoryIssue::none)
    return result;
  for (std::size_t i = 0; i < inventory.total.size(); ++i) {
    if (inventory.total[i] < 0 || inventory.listed[i] < 0 || inventory.packed[i] < 0 ||
        inventory.listed[i] > INT32_MAX - inventory.packed[i] ||
        inventory.total[i] != inventory.listed[i] + inventory.packed[i]) {
      result.issue = StockIssue::invalid;
      return result;
    }
  }
  if (recipe != OriginalRecipe::meat && recipe != OriginalRecipe::healing_essence) {
    result.issue = StockIssue::invalid;
    return result;
  }
  const std::array<bool, 6> ingredients =
      recipe == OriginalRecipe::meat
          ? std::array<bool, 6>{{true, false, false, false, false, false}}
          : std::array<bool, 6>{{false, false, true, true, true, false}};
  for (std::size_t i = 0; i < ingredients.size(); ++i) {
    if (!ingredients[i])
      continue;
    // Native GetAmount can count a mixed MultiSlot stack twice. A recipe
    // result in both the list and packed inventory is a valid starting
    // state (world run: +1 materialized and 37 packed healing potions).
    if (inventory.duplicate_packed_target[i] || inventory.has_active_marker[i] ||
        inventory.mixed_representation[i] || inventory.packed_occurrences[i] > 1 ||
        inventory.listed_occurrences[i] > 1 ||
        (inventory.listed[i] > 0 && inventory.packed[i] > 0)) {
      result.issue = StockIssue::ambiguous_ingredient;
      return result;
    }
  }
  if (hands.left.state != 0 || hands.right.state != 1 || hands.right.amount != 1 ||
      hands.right.symbol != (recipe == OriginalRecipe::meat ? 16608 : lhiver::item_symbols[2])) {
    result.issue = StockIssue::hands;
    return result;
  }
  result.stored = inventory.total;
  if (recipe == OriginalRecipe::meat) {
    result.maximum = static_cast<std::uint32_t>(inventory.total[0]);
    if (result.maximum > static_cast<std::uint32_t>(INT32_MAX - inventory.total[1])) {
      result.issue = StockIssue::overflow;
      return result;
    }
  } else {
    if (inventory.total[2] == INT32_MAX) {
      result.issue = StockIssue::overflow;
      return result;
    }
    result.reserved[2] = 1;
    auto maximum = inventory.total[2] + 1;
    for (const auto i : {3u, 4u})
      if (inventory.total[i] < maximum)
        maximum = inventory.total[i];
    result.maximum = static_cast<std::uint32_t>(maximum);
    if (result.maximum > static_cast<std::uint32_t>(INT32_MAX - inventory.total[5])) {
      result.issue = StockIssue::overflow;
      return result;
    }
  }
  result.issue = StockIssue::none;
  return result;
}

struct MaterialPreview {
  bool valid = false;
  std::array<std::int32_t, 6> available{}, consumed{}, remaining{}, produced{};
};

inline MaterialPreview material_preview(OriginalRecipe recipe, const SelectionStock& stock,
                                        std::uint32_t quantity) noexcept {
  MaterialPreview result;
  if (!stock.known() || !quantity || quantity > stock.maximum || quantity > INT32_MAX ||
      (recipe != OriginalRecipe::meat && recipe != OriginalRecipe::healing_essence))
    return result;
  for (std::size_t i = 0; i < result.available.size(); ++i) {
    if (stock.stored[i] < 0 || stock.reserved[i] < 0 ||
        stock.stored[i] > INT32_MAX - stock.reserved[i])
      return {};
    result.available[i] = result.remaining[i] = stock.stored[i] + stock.reserved[i];
  }
  const auto amount = static_cast<std::int32_t>(quantity);
  const std::array<bool, 6> ingredients =
      recipe == OriginalRecipe::meat
          ? std::array<bool, 6>{{true, false, false, false, false, false}}
          : std::array<bool, 6>{{false, false, true, true, true, false}};
  for (std::size_t i = 0; i < ingredients.size(); ++i) {
    if (!ingredients[i])
      continue;
    if (result.available[i] < amount)
      return {};
    result.consumed[i] = amount;
    result.remaining[i] -= amount;
  }
  const std::size_t output = recipe == OriginalRecipe::meat ? 1 : 5;
  if (result.remaining[output] > INT32_MAX - amount)
    return {};
  result.produced[output] = amount;
  result.remaining[output] += amount;
  result.valid = true;
  return result;
}

} // namespace ubc
