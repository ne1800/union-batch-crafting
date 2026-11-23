// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/core/recipe_material.hpp"
#include "ubc/catalogs/station_recipe_catalog.hpp"

#include <cstddef>
#include <cstdint>

namespace ubc {

// Own copies from the same stable admission as RecipeInventory. An
// exact item alone proves neither slot lifetime nor InteractItem anchor.
// No pointer in this structure is dereferenced by the material core.
struct StationRecipeHands {
  HandsSnapshot hands{};
  bool left_slot_known = false, right_slot_known = false;
  std::int32_t left_tmp_level = -1, right_tmp_level = -1;
  bool interact_item_known = false;
  std::uint32_t interact_item = 0;
};
enum class StationRecipeHandIssue {
  none,
  unverified_contract,
  unsupported_contract,
  hand_object,
  unreadable_slot,
  slot_lifetime,
  hand_alias,
  inventory_alias,
  anchor,
  stored_prop
};
enum class StationRecipeMaterialIssue {
  none,
  descriptor,
  output_buffer,
  unknown_inventory,
  invalid_inventory,
  target_mapping,
  ambiguous_ingredient,
  hand_contract,
  return_capacity,
  quantity
};
struct StationRecipeMaterialRow {
  std::int32_t symbol = -1;
  std::uint32_t unit_cost = 0, unit_output = 0;
  std::int64_t stored = -1, reserved_tool = 0, generated_hand = 0;
  // Only stored ingredients are available. A reserved saw is NOT
  // a raw ingredient; produced requisites are NOT inventory stock.
  std::int64_t available = -1;
  bool available_known = false;
  std::int64_t required = -1, produced = -1, missing = -1, missing_for_one = -1;
  std::int64_t output_space = -1, output_shortfall = -1, output_shortfall_for_one = -1;
  // Planned stock values, no observed effect. With missing ingredients
  // the planned remainder can be negative. None of these numbers is a
  // closing receipt; native Witness/Hand/Context checks remain separate.
  std::int64_t expected_ready_storage = -1, expected_end_storage = -1;
  bool ready_storage_known = false, end_storage_known = false;
  std::int64_t end_overflow = -1;
};
struct StationRecipeMaterial {
  StationRecipeMaterialIssue issue = StationRecipeMaterialIssue::none;
  StationRecipeHandIssue hand_issue = StationRecipeHandIssue::none;
  bool inventory_known = false, hand_known = false, craftable = false;
  bool equipped_ingredient = false;
  std::uint32_t maximum = 0, quantity = 0;
  std::size_t count = 0;
  station_recipe_catalog::Completion completion =
      station_recipe_catalog::Completion::end_production;
  bool known() const noexcept {
    return issue == StationRecipeMaterialIssue::none && inventory_known && hand_known;
  }
};

namespace station_recipe_material_detail {
namespace cat = station_recipe_catalog;
using HandIssue = StationRecipeHandIssue;

inline HandIssue check_hand(const cat::HandItem& expected, const HandObjectSnapshot& actual,
                            bool slot_known, std::int32_t tmp_level) noexcept {
  if (expected.source == cat::HandSource::empty)
    return actual.state == 0 && !actual.object ? HandIssue::none : HandIssue::hand_object;
  if ((expected.source != cat::HandSource::reserved_inventory_tool &&
       expected.source != cat::HandSource::generated) ||
      expected.symbol < 0 || !expected.amount ||
      expected.amount > static_cast<std::uint32_t>(INT32_MAX))
    return HandIssue::unsupported_contract;
  if (actual.state != 1 || !actual.object || actual.symbol != expected.symbol ||
      actual.amount != static_cast<std::int32_t>(expected.amount))
    return HandIssue::hand_object;
  if (!slot_known)
    return HandIssue::unreadable_slot;
  // INSERT does not guarantee tmpLevel=0 for arbitrary old slot. The
  // normal non-temporary path must currently be read. CREATE sets 1 or 2.
  const bool temporary = expected.source == cat::HandSource::generated;
  return (temporary ? tmp_level == 1 || tmp_level == 2 : tmp_level == 0) ? HandIssue::none
                                                                         : HandIssue::slot_lifetime;
}
inline bool stored_absent(const RecipeInventory& inventory, const cat::HandItem& hand) noexcept {
  if (!hand.require_stored_absent)
    return true;
  const auto* item = recipe_material_detail::item(inventory, hand.symbol);
  return item && item->total == 0 && !item->listed_occurrences && !item->packed_occurrences &&
         !item->duplicate_packed_target && !item->has_active_marker && !item->mixed_representation;
}
inline HandIssue check_hands(const cat::Descriptor& descriptor, const RecipeInventory& inventory,
                             const StationRecipeHands& observed, bool contract_verified) noexcept {
  if (!contract_verified)
    return HandIssue::unverified_contract;
  if (!descriptor.hand.static_lifecycle_verified)
    return HandIssue::unsupported_contract;
  auto issue = check_hand(descriptor.hand.left, observed.hands.left, observed.left_slot_known,
                          observed.left_tmp_level);
  if (issue != HandIssue::none)
    return issue;
  issue = check_hand(descriptor.hand.right, observed.hands.right, observed.right_slot_known,
                     observed.right_tmp_level);
  if (issue != HandIssue::none)
    return issue;
  if (observed.hands.left.object && observed.hands.left.object == observed.hands.right.object)
    return HandIssue::hand_alias;
  const auto anchor = descriptor.hand.anchor == cat::HandSide::left ? observed.hands.left.object
                                                                    : observed.hands.right.object;
  if (!observed.interact_item_known || !anchor || observed.interact_item != anchor)
    return HandIssue::anchor;
  if (!recipe_material_detail::no_hand_alias(inventory, observed.hands))
    return HandIssue::inventory_alias;
  // The normal EV_DestroyInteractItem follow-up removes after release
  // temporary slots possibly ONE stored instance of the previous anchor.
  // Leather: right 17342. Forge: right 16667. The left forge tongs 16666
  // are not a second destroy anchor and remain unchanged per separately
  // booked normal SetInteractItem follow-up in the store. Both stocks
  // are still expected in Ready/End; abort paths are thus NOT proven.
  if (!stored_absent(inventory, descriptor.hand.left) ||
      !stored_absent(inventory, descriptor.hand.right))
    return HandIssue::stored_prop;
  return HandIssue::none;
}
inline void copy_hand_amount(const cat::HandItem& hand, bool known,
                             StationRecipeMaterialRow& row) noexcept {
  if (hand.source == cat::HandSource::empty || hand.symbol != row.symbol)
    return;
  if (hand.source == cat::HandSource::reserved_inventory_tool)
    row.reserved_tool = known ? static_cast<std::int64_t>(hand.amount) : -1;
  if (hand.source == cat::HandSource::generated)
    row.generated_hand = known ? static_cast<std::int64_t>(hand.amount) : -1;
}
} // namespace station_recipe_material_detail

// Pure value evaluation, no learning/parser/animation release and no
// engine action. Descriptor/Views/output buffers are own valid objects.
// Inventory needs all listed item pointers, even recipe-foreign, as well
// as complete listed/packed target quantities at the same verified point.
// hand_contract_verified confirms the current native station contract;
// neither static_lifecycle_verified nor observed_in_world grant it themselves.
// With material_actor=SELF the native adapter additionally needs the
// current positive SELF==HERO receipt; this pure stock is not remapped.
//
// Ammo ends at Ready state with still reserved saw. Only a
// separately booked normal station end expects their return.
// Leather/Forge need the normal native end. No own reimbursement,
// prop deletion, abort compensation or automatic repetition.
inline StationRecipeMaterial
station_recipe_material(const station_recipe_catalog::Descriptor& descriptor,
                        const RecipeInventory& inventory, const StationRecipeHands& observed,
                        std::uint32_t quantity, StationRecipeMaterialRow* rows,
                        std::size_t capacity, bool hand_contract_verified = false) noexcept {
  namespace common = recipe_material_detail;
  namespace detail = station_recipe_material_detail;
  using Issue = StationRecipeMaterialIssue;
  StationRecipeMaterial result;
  result.quantity = quantity;
  result.completion = descriptor.completion;
  if (!rows || capacity < descriptor.targets.count) {
    result.issue = Issue::output_buffer;
    return result;
  }
  if (!station_recipe_catalog::structurally_valid(descriptor)) {
    result.issue = Issue::descriptor;
    return result;
  }
  result.count = descriptor.targets.count;
  for (std::size_t i = 0; i < result.count; ++i) {
    rows[i] = {};
    rows[i].symbol = descriptor.targets[i].symbol;
    rows[i].unit_cost = common::units(descriptor.costs, rows[i].symbol);
    rows[i].unit_output = common::units(descriptor.outputs, rows[i].symbol);
  }
  if (!inventory.header_known || !inventory.listed_known || !inventory.packed_known ||
      !inventory.storage_totals_known || !inventory.stable_boundary_verified ||
      !inventory.snapshot_unchanged || inventory.issue != PackedInventoryIssue::none) {
    result.issue = Issue::unknown_inventory;
    return result;
  }
  if (inventory.listed_nodes > max_packed_inventory_nodes ||
      inventory.packed_entries > max_packed_inventory_entries) {
    result.issue = Issue::invalid_inventory;
    return result;
  }
  if (!inventory.items.data || inventory.items.count != result.count) {
    result.issue = Issue::target_mapping;
    return result;
  }
  std::uint64_t listed_occurrences = 0, packed_occurrences = 0;
  for (std::size_t i = 0; i < result.count; ++i) {
    const auto* item = common::item(inventory, rows[i].symbol);
    if (!item) {
      result.issue = Issue::target_mapping;
      return result;
    }
    if (item->listed < 0 || item->packed < 0 || item->total < 0 ||
        std::int64_t{item->listed} + item->packed != item->total ||
        (item->listed > 0 && !item->listed_occurrences) ||
        ((item->packed > 0) != (item->packed_occurrences > 0)) ||
        item->listed_occurrences > inventory.listed_nodes ||
        item->packed_occurrences > inventory.packed_entries) {
      result.issue = Issue::invalid_inventory;
      return result;
    }
    listed_occurrences += item->listed_occurrences;
    packed_occurrences += item->packed_occurrences;
  }
  if (listed_occurrences > inventory.listed_nodes ||
      packed_occurrences > inventory.packed_entries) {
    result.issue = Issue::invalid_inventory;
    return result;
  }
  result.inventory_known = true;
  for (std::size_t i = 0; i < result.count; ++i) {
    auto& row = rows[i];
    const auto& item = *common::item(inventory, row.symbol);
    row.stored = item.total;
    row.available = row.stored;
    row.available_known = true;
    if (row.unit_cost &&
        (item.duplicate_packed_target || item.has_active_marker || item.mixed_representation ||
         item.listed_occurrences > 1 || item.packed_occurrences > 1 ||
         (item.listed_occurrences && item.packed_occurrences) ||
         (item.listed > 0 && item.packed > 0)))
      result.issue = Issue::ambiguous_ingredient;
  }
  result.equipped_ingredient = result.issue == Issue::ambiguous_ingredient &&
                               common::equipped_ingredient(inventory, descriptor.costs);
  result.hand_issue = detail::check_hands(descriptor, inventory, observed, hand_contract_verified);
  result.hand_known = result.hand_issue == StationRecipeHandIssue::none;
  if (!result.hand_known && result.issue == Issue::none)
    result.issue = Issue::hand_contract;

  std::uint64_t maximum = INT32_MAX;
  const bool valid_quantity = quantity <= static_cast<std::uint32_t>(INT32_MAX);
  for (std::size_t i = 0; i < result.count; ++i) {
    auto& row = rows[i];
    detail::copy_hand_amount(descriptor.hand.left, result.hand_known, row);
    detail::copy_hand_amount(descriptor.hand.right, result.hand_known, row);
    row.missing_for_one = common::positive(std::int64_t{row.unit_cost} - row.available);
    row.output_space = INT32_MAX - row.stored;
    row.output_shortfall_for_one =
        common::positive(std::int64_t{row.unit_output} - row.output_space);
    if (row.unit_cost) {
      common::limit(maximum, static_cast<std::uint64_t>(row.available) / row.unit_cost);
      common::limit(maximum, static_cast<std::uint32_t>(INT32_MAX) / row.unit_cost);
    }
    if (row.unit_output)
      common::limit(maximum, static_cast<std::uint64_t>(row.output_space) / row.unit_output);
    // The reserved tool is per catalog disjoint from cost/output. Its
    // safe normal end remainder is thus independent of the raw quantity.
    if (row.reserved_tool > 0 && row.stored + row.reserved_tool > INT32_MAX &&
        result.issue == Issue::none)
      result.issue = Issue::return_capacity;
    if (valid_quantity) {
      row.required = std::int64_t{row.unit_cost} * quantity;
      row.produced = std::int64_t{row.unit_output} * quantity;
      row.missing = common::positive(row.required - row.available);
      row.output_shortfall = common::positive(row.produced - row.output_space);
      row.expected_ready_storage = row.stored - row.required + row.produced;
      row.ready_storage_known = true;
      if (row.reserved_tool >= 0) {
        row.expected_end_storage = row.expected_ready_storage + row.reserved_tool;
        row.end_storage_known = true;
        row.end_overflow = common::positive(row.expected_end_storage - INT32_MAX);
      }
    }
  }
  if (result.known())
    result.maximum = static_cast<std::uint32_t>(maximum);
  if (!valid_quantity && result.issue == Issue::none)
    result.issue = Issue::quantity;
  result.craftable = result.known() && quantity && quantity <= result.maximum;
  return result;
}

} // namespace ubc
