// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/engine/hand_snapshot.hpp"
#include "ubc/catalogs/recipe_catalog.hpp"
#include <cstddef>
#include <cstdint>

namespace ubc {

// Copies from ONE fully verified inventory snapshot. The symbol
// belongs to the respective target list; the order may differ.
struct RecipeInventoryItem {
  std::int32_t symbol = -1, listed = -1, packed = -1, total = -1;
  std::uint32_t listed_occurrences = 0, packed_occurrences = 0;
  bool duplicate_packed_target = false, has_active_marker = false, mixed_representation = false;
};
struct RecipeInventory {
  bool header_known = false, listed_known = false, packed_known = false;
  bool storage_totals_known = false, stable_boundary_verified = false, snapshot_unchanged = false;
  PackedInventoryIssue issue = PackedInventoryIssue::none;
  std::uint32_t listed_nodes = 0, packed_entries = 0;
  recipe_catalog::View<RecipeInventoryItem> items;
  // All item pointers of the same complete list, including foreign items.
  // Not only recipe-related hits! Do not dereference foreign pointers.
  recipe_catalog::View<std::uint32_t> listed_objects;
};
enum class RecipeMaterialIssue {
  none,
  descriptor,
  output_buffer,
  unknown_inventory,
  invalid_inventory,
  target_mapping,
  ambiguous_ingredient,
  hand_contract_unverified,
  unsupported_hand_contract,
  hand_object,
  hand_alias,
  quantity,
  hand_prop_stock
};
struct RecipeMaterialRow {
  std::int32_t symbol = -1;
  std::uint32_t unit_cost = 0, unit_output = 0;
  std::int64_t stored = -1, held = 0, reserved = 0, available = -1;
  bool available_known = false;
  // Pure planning, no already executed material effects. A negative
  // planned remainder at available_known shows a stock shortage.
  std::int64_t required = -1, missing = -1, produced = -1, remaining = -1;
  std::int64_t missing_for_one = -1, output_space = -1;
  std::int64_t output_shortfall = -1, output_shortfall_for_one = -1;
};
struct RecipeMaterial {
  RecipeMaterialIssue issue = RecipeMaterialIssue::none;
  bool inventory_known = false, hand_known = false, craftable = false;
  std::uint32_t maximum = 0, quantity = 0;
  std::size_t count = 0;
  bool known() const noexcept {
    return issue == RecipeMaterialIssue::none && inventory_known && hand_known;
  }
};

namespace recipe_material_detail {
inline std::uint32_t units(recipe_catalog::View<recipe_catalog::ItemAmount> list,
                           std::int32_t symbol) noexcept {
  for (const auto& item : list)
    if (item.symbol == symbol)
      return item.units;
  return 0;
}
inline const RecipeInventoryItem* item(const RecipeInventory& inventory,
                                       std::int32_t symbol) noexcept {
  const RecipeInventoryItem* match = nullptr;
  for (const auto& entry : inventory.items)
    if (entry.symbol == symbol) {
      if (match)
        return nullptr;
      match = &entry;
    }
  return match;
}
inline bool supported_hand(const recipe_catalog::Descriptor& descriptor) noexcept {
  const auto& h = descriptor.hand;
  if (!h.left_must_be_empty || h.amount != 1)
    return false;
  // Only the examined pan/flask contracts. Evidence or a
  // randomly matching item produces no further station release.
  if (descriptor.station == recipe_catalog::Station::stove)
    return h.start_item_symbol == 16607 && h.active_item_symbol == 16608 && !h.from_inventory &&
           !h.consumed_on_success && !h.failure_refund;
  if (descriptor.station == recipe_catalog::Station::alchemy)
    return h.start_item_symbol == 16604 && h.active_item_symbol == 16604 && h.from_inventory &&
           h.consumed_on_success && h.failure_refund == 1;
  return false;
}
inline bool no_hand_alias(const RecipeInventory& inventory, const HandsSnapshot& hands) noexcept {
  if (inventory.listed_objects.count != inventory.listed_nodes ||
      (inventory.listed_objects.count && !inventory.listed_objects.data))
    return false;
  // Fixed existing list bound, efficient reuse of the pure
  // pointer sets. No heap allocation and no quadratic 4096 check.
  station_detail::AddressSet seen;
  for (const auto object : inventory.listed_objects)
    if (!seen.insert(object) || object == hands.left.object || object == hands.right.object)
      return false;
  return true;
}
inline bool no_stored_generated_prop(const recipe_catalog::Descriptor& descriptor,
                                     const RecipeInventory& inventory) noexcept {
  if (descriptor.station != recipe_catalog::Station::stove)
    return true;
  // The occupied STOVE path produces PANFULL (16608) directly. The normal
  // return uses EV_DestroyInteractItem 0x754B40: after SetInteractItem(0)
  // an additional inventory item of the previous InteractItem instance
  // is searched/removed (0x754B76/0x754B92 -> Remove(int,int) 0x70D080).
  // Therefore this produced prop must be fully absent in the store, even
  // as a materialized zero-quantity object. PAN (16607) remains allowed:
  // B_USESTOVE requires it, this animation path does not produce/destroy it.
  const auto* prop = item(inventory, descriptor.hand.active_item_symbol);
  return prop && prop->total == 0 && !prop->listed_occurrences && !prop->packed_occurrences &&
         !prop->duplicate_packed_target && !prop->has_active_marker && !prop->mixed_representation;
}
inline std::int64_t positive(std::int64_t value) noexcept { return value > 0 ? value : 0; }
inline void limit(std::uint64_t& maximum, std::uint64_t candidate) noexcept {
  if (candidate < maximum)
    maximum = candidate;
}
}

// All views and the output buffer are their own valid C++ memory regions,
// no unvalidated engine pointers. Their length limits the work; there
// is no inherited six-limit and no new fixed recipe ingredient upper
// bound. Native script/learn/station/execution releases remain separate
// gates. hand_contract_verified is an explicit current adapter evidence;
// neither observed_original nor inferred_same_station set this parameter.
inline RecipeMaterial recipe_material(const recipe_catalog::Descriptor& descriptor,
                                      const RecipeInventory& inventory, const HandsSnapshot& hands,
                                      std::uint32_t quantity, RecipeMaterialRow* rows,
                                      std::size_t capacity,
                                      bool hand_contract_verified = false) noexcept {
  namespace detail = recipe_material_detail;
  using Issue = RecipeMaterialIssue;
  RecipeMaterial result;
  result.quantity = quantity;
  if (!rows || capacity < descriptor.targets.count) {
    result.issue = Issue::output_buffer;
    return result;
  }
  if (!recipe_catalog::structurally_valid(descriptor)) {
    result.issue = Issue::descriptor;
    return result;
  }
  result.count = descriptor.targets.count;
  for (std::size_t i = 0; i < result.count; ++i) {
    rows[i] = {};
    rows[i].symbol = descriptor.targets[i].symbol;
    rows[i].unit_cost = detail::units(descriptor.costs, rows[i].symbol);
    rows[i].unit_output = detail::units(descriptor.outputs, rows[i].symbol);
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
  if (inventory.items.count != result.count || !inventory.items.data) {
    result.issue = Issue::target_mapping;
    return result;
  }
  std::uint64_t listed_occurrences = 0, packed_occurrences = 0;
  for (std::size_t i = 0; i < result.count; ++i) {
    const auto* entry = detail::item(inventory, rows[i].symbol);
    if (!entry) {
      result.issue = Issue::target_mapping;
      return result;
    }
    if (entry->listed < 0 || entry->packed < 0 || entry->total < 0 ||
        std::int64_t{entry->listed} + entry->packed != entry->total ||
        (entry->listed > 0 && !entry->listed_occurrences) ||
        ((entry->packed > 0) != (entry->packed_occurrences > 0)) ||
        entry->listed_occurrences > inventory.listed_nodes ||
        entry->packed_occurrences > inventory.packed_entries) {
      result.issue = Issue::invalid_inventory;
      return result;
    }
    listed_occurrences += entry->listed_occurrences;
    packed_occurrences += entry->packed_occurrences;
  }
  if (listed_occurrences > inventory.listed_nodes ||
      packed_occurrences > inventory.packed_entries) {
    result.issue = Issue::invalid_inventory;
    return result;
  }
  result.inventory_known = true;
  for (std::size_t i = 0; i < result.count; ++i) {
    auto& row = rows[i];
    const auto& entry = *detail::item(inventory, row.symbol);
    row.stored = entry.total;
    row.available = row.stored;
    row.available_known = true;
    if (row.unit_cost &&
        (entry.duplicate_packed_target || entry.has_active_marker || entry.mixed_representation ||
         entry.listed_occurrences > 1 || entry.packed_occurrences > 1 ||
         (entry.listed_occurrences && entry.packed_occurrences) ||
         (entry.listed > 0 && entry.packed > 0)))
      result.issue = Issue::ambiguous_ingredient;
  }
  Issue hand_issue = Issue::none;
  if (!hand_contract_verified)
    hand_issue = Issue::hand_contract_unverified;
  else if (!detail::supported_hand(descriptor))
    hand_issue = Issue::unsupported_hand_contract;
  else if (hands.left.state != 0 || hands.left.object || hands.right.state != 1 ||
           !hands.right.object || hands.right.symbol != descriptor.hand.active_item_symbol ||
           hands.right.amount != static_cast<std::int32_t>(descriptor.hand.amount))
    hand_issue = Issue::hand_object;
  else if (!detail::no_hand_alias(inventory, hands))
    hand_issue = Issue::hand_alias;
  else if (!detail::no_stored_generated_prop(descriptor, inventory))
    hand_issue = Issue::hand_prop_stock;
  else
    result.hand_known = true;
  if (result.issue == Issue::none)
    result.issue = hand_issue;
  for (std::size_t i = 0; i < result.count; ++i)
    if (rows[i].symbol == descriptor.hand.active_item_symbol) {
      auto& row = rows[i];
      row.held = result.hand_known ? static_cast<std::int64_t>(descriptor.hand.amount) : -1;
      if (descriptor.hand.from_inventory) {
        row.reserved = result.hand_known ? static_cast<std::int64_t>(descriptor.hand.amount) : -1;
        row.available_known = result.hand_known;
        row.available = result.hand_known ? row.stored + row.reserved : -1;
      }
    }

  std::uint64_t maximum = INT32_MAX;
  const bool valid_quantity = quantity <= INT32_MAX;
  for (std::size_t i = 0; i < result.count; ++i) {
    auto& row = rows[i];
    row.output_space = INT32_MAX - row.stored;
    row.output_shortfall_for_one =
        detail::positive(std::int64_t{row.unit_output} - row.output_space);
    row.missing_for_one =
        row.available_known ? detail::positive(std::int64_t{row.unit_cost} - row.available) : -1;
    if (row.unit_cost) {
      if (row.available_known)
        detail::limit(maximum, static_cast<std::uint64_t>(row.available) / row.unit_cost);
      detail::limit(maximum, static_cast<std::uint32_t>(INT32_MAX) / row.unit_cost);
    }
    if (row.unit_output)
      detail::limit(maximum, static_cast<std::uint64_t>(row.output_space) / row.unit_output);
    if (valid_quantity) {
      row.required = std::int64_t{row.unit_cost} * quantity;
      row.produced = std::int64_t{row.unit_output} * quantity;
      row.missing = row.available_known ? detail::positive(row.required - row.available)
                                        : (row.required ? -1 : 0);
      row.remaining = row.available_known ? row.available - row.required + row.produced : -1;
      row.output_shortfall =
          row.unit_output ? detail::positive(row.produced - row.output_space) : 0;
    }
  }
  if (result.known())
    result.maximum = static_cast<std::uint32_t>(maximum);
  if (!valid_quantity && result.issue == Issue::none)
    result.issue = Issue::quantity;
  result.craftable = result.known() && quantity > 0 && quantity <= result.maximum;
  return result;
}
} // namespace ubc
