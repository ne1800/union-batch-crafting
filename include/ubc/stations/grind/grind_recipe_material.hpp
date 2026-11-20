// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/stations/grind/grind_recipe_plan.hpp"
#include "ubc/stations/station_recipe_material.hpp"

namespace ubc {

// Values from the same stable admission as inventory and hands. The
// adapter must bind the existing script/actor checks and the native
// guild reader to be assigned separately. These are not engine pointers.
struct GrindMaterialKnowledge {
  bool talent_known = false, guild_known = false, skill_known = false;
  std::int32_t talent = 0, guild = 0, skill = 0;
};
struct GrindMaterialProofs {
  bool stable_boundary_verified = false;
  // Current SMITHSHARP/Link anchor/normal end contract at the same context
  // and frame. Script production 11, matching assets or a tmp value alone
  // are not positive evidence. The projection does not produce this proof.
  bool station_hand_end_verified = false;
  // Current input weapons: native item type, canonical inactive
  // flags and no alias in any equipment slot. The stock value alone
  // does not prove this still-to-be-bound object/slot contract.
  bool input_objects_verified = false;
  std::int32_t verified_left_tmp_level = 0;
};
enum class GrindMaterialIssue {
  none,
  descriptor,
  unknown_inventory,
  invalid_inventory,
  target_mapping,
  ambiguous_ingredient,
  hand_contract,
  hand_object,
  hand_alias,
  ingredient_objects,
  knowledge,
  quantity
};
struct GrindMaterial {
  GrindMaterialIssue issue = GrindMaterialIssue::none;
  bool inventory_known = false, hand_known = false, ingredients_verified = false;
  bool knowledge_known = false, learned = false, guild_allowed = false, craftable = false;
  std::uint32_t quantity = 0, maximum = 0;
  // Canonical rows: old weapon, new weapon, cold blade 16613. The third
  // row shows the final net demand 0. Fixed intermediate output 1 and
  // end debit 1 are kept separate in numerical.after_callback/after_end.
  std::array<RecipeMaterialRow, 3> rows{};
  grind_recipe::Evidence evidence{};
  grind_recipe::Plan numerical{};
  bool known() const noexcept {
    return issue == GrindMaterialIssue::none && inventory_known && hand_known &&
           ingredients_verified && knowledge_known;
  }
};

// Pure value projection: no memory/native calls, no writes, no
// GetAmount materialization. Views must contain their own valid
// C++ memory areas. The result holds its values; no view lives
// on in it. It is neither writer lease nor runtime authorization.
inline GrindMaterial
grind_recipe_material(const grind_recipe::Descriptor& descriptor, const RecipeInventory& inventory,
                      const StationRecipeHands& hands, const GrindMaterialKnowledge& knowledge,
                      std::uint32_t quantity, GrindMaterialProofs proof = {}) noexcept {
  namespace g = grind_recipe;
  namespace math = recipe_material_detail;
  using Issue = GrindMaterialIssue;
  GrindMaterial out;
  out.quantity = quantity;
  if (!g::structurally_valid(descriptor)) {
    out.issue = Issue::descriptor;
    return out;
  }
  const auto& recipe = *g::find(descriptor.function.symbol);
  const std::array<std::int32_t, 3> targets{{recipe.input, recipe.output, g::lifecycle_item}};
  out.evidence.callback = recipe.function.symbol;
  for (std::size_t i = 0; i < targets.size(); ++i) {
    out.rows[i].symbol = targets[i];
    out.rows[i].unit_cost = i == 0 ? 1u : 0u;
    out.rows[i].unit_output = i == 1 ? 1u : 0u;
  }
  if (!inventory.header_known || !inventory.listed_known || !inventory.packed_known ||
      !inventory.storage_totals_known || !inventory.stable_boundary_verified ||
      !inventory.snapshot_unchanged || inventory.issue != PackedInventoryIssue::none) {
    out.issue = Issue::unknown_inventory;
    return out;
  }
  if (inventory.listed_nodes > max_packed_inventory_nodes ||
      inventory.packed_entries > max_packed_inventory_entries) {
    out.issue = Issue::invalid_inventory;
    return out;
  }
  if (!inventory.items.data || inventory.items.count != targets.size()) {
    out.issue = Issue::target_mapping;
    return out;
  }
  std::uint64_t listed = 0, packed = 0;
  bool input_unambiguous = false;
  for (std::size_t i = 0; i < targets.size(); ++i) {
    auto& row = out.rows[i];
    const auto* item = math::item(inventory, targets[i]);
    if (!item) {
      out.issue = Issue::target_mapping;
      return out;
    }
    if (item->listed < 0 || item->packed < 0 || item->total < 0 ||
        std::int64_t{item->listed} + item->packed != item->total ||
        (item->listed > 0 && !item->listed_occurrences) ||
        ((item->packed > 0) != (item->packed_occurrences > 0)) ||
        item->listed_occurrences > inventory.listed_nodes ||
        item->packed_occurrences > inventory.packed_entries) {
      out.issue = Issue::invalid_inventory;
      return out;
    }
    listed += item->listed_occurrences;
    packed += item->packed_occurrences;
    row.stored = row.available = item->total;
    row.available_known = true;
    const bool unambiguous = item->listed_occurrences <= 1 && item->packed_occurrences <= 1 &&
                             !item->duplicate_packed_target && !item->has_active_marker &&
                             !item->mixed_representation &&
                             !(item->listed_occurrences && item->packed_occurrences);
    if (!i)
      input_unambiguous = unambiguous;
    out.evidence.stocks[i] = {row.symbol, item->total, true, unambiguous};
  }
  if (listed > inventory.listed_nodes || packed > inventory.packed_entries) {
    out.issue = Issue::invalid_inventory;
    return out;
  }
  out.inventory_known = true;
  if (!input_unambiguous) {
    out.issue = Issue::ambiguous_ingredient;
    return out;
  }

  if (!proof.stable_boundary_verified || !proof.station_hand_end_verified ||
      (proof.verified_left_tmp_level != 1 && proof.verified_left_tmp_level != 2) ||
      !hands.left_slot_known || !hands.right_slot_known || !hands.interact_item_known) {
    out.issue = Issue::hand_contract;
    return out;
  }
  const auto& left = hands.hands.left;
  const auto& right = hands.hands.right;
  if (left.state != 1 || !left.object || left.symbol != g::lifecycle_item || left.amount != 1 ||
      right.state != 0 || right.object || hands.interact_item != left.object ||
      hands.left_tmp_level != proof.verified_left_tmp_level) {
    out.issue = Issue::hand_object;
    return out;
  }
  // Check the full list including foreign items. The generated
  // left blade must not be stored or duplicated at the same time.
  if (!math::no_hand_alias(inventory, hands.hands)) {
    out.issue = Issue::hand_alias;
    return out;
  }
  out.hand_known = true;
  out.rows[2].held = 1; // Information only; available remains the pure stored inventory.
  out.ingredients_verified = proof.input_objects_verified;
  if (!out.ingredients_verified) {
    out.issue = Issue::ingredient_objects;
    return out;
  }

  out.knowledge_known = knowledge.talent_known && knowledge.guild_known && knowledge.skill_known;
  if (!out.knowledge_known) {
    out.issue = Issue::knowledge;
    return out;
  }
  if (quantity > INT32_MAX) {
    out.issue = Issue::quantity;
    return out;
  }
  out.evidence.talent_known = out.evidence.guild_known = out.evidence.skill_known = true;
  out.evidence.talent = knowledge.talent;
  out.evidence.guild = knowledge.guild;
  out.evidence.skill = knowledge.skill;
  // Only now may the downstream reusable evidence reach the
  // plan. At each of the above rejections this gate stays closed.
  out.evidence.stable_boundary_verified = true;
  out.numerical = g::plan(recipe, out.evidence, quantity);
  out.learned = out.numerical.learned;
  out.guild_allowed = out.numerical.guild_allowed;
  if (out.known() && out.learned && out.guild_allowed)
    out.maximum = out.numerical.maximum;
  for (std::size_t i = 0; i < out.rows.size(); ++i) {
    auto& row = out.rows[i];
    row.required = std::int64_t{row.unit_cost} * quantity;
    row.produced = std::int64_t{row.unit_output} * quantity;
    row.missing = math::positive(row.required - row.available);
    row.missing_for_one = math::positive(std::int64_t{row.unit_cost} - row.available);
    row.remaining = row.available - row.required + row.produced;
    row.output_space = INT32_MAX - row.stored;
    // The cold blade needs exactly one free slot between callback
    // and normal end, independent of N. N0 performs no effect.
    const auto callback_output = i == 2 ? (quantity ? std::int64_t{1} : 0) : row.produced;
    const auto one_output = i == 2 ? std::int64_t{1} : row.unit_output;
    row.output_shortfall = math::positive(callback_output - row.output_space);
    row.output_shortfall_for_one = math::positive(one_output - row.output_space);
  }
  out.craftable = out.known() && out.numerical.valid;
  return out;
}
} // namespace ubc
