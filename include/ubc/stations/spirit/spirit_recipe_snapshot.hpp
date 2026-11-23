// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/spirit/spirit_recipe_runtime.hpp"
#include "ubc/stations/station_recipe_snapshot.hpp"

namespace ubc {
// This seven-line balance requires its own N-1 debit/result adapter.
// The previous linear 13er writer must not silently execute it.
inline constexpr bool spirit_debit_integrated = false;
enum class SpiritMaterialIssue {
  none,
  descriptor,
  output_buffer,
  unknown_inventory,
  invalid_inventory,
  target_mapping,
  ambiguous_ingredient,
  hand_contract,
  hand_object,
  hand_alias,
  unknown_branch,
  ingredient_objects,
  quantity
};
struct SpiritMaterial {
  SpiritMaterialIssue issue = SpiritMaterialIssue::none;
  bool inventory_known = false, hand_known = false, branch_known = false,
       ingredients_verified = false, craftable = false;
  bool equipped_ingredient = false;
  std::uint32_t maximum = 0, quantity = 0, additional_flasks = 0;
  std::int32_t output = -1;
  std::size_t count = 0;
  bool known() const noexcept {
    return issue == SpiritMaterialIssue::none && inventory_known && hand_known && branch_known &&
           ingredients_verified;
  }
};
// Pure total balance including the N-1 debit still to be integrated
// separately. The unselected potion type is a mandatory unchanged target.
inline SpiritMaterial spirit_recipe_material(
    const spirit_recipe_catalog::Descriptor& descriptor, const RecipeInventory& inventory,
    const StationRecipeHands& hands, spirit_recipe_catalog::Branch branch, std::uint32_t quantity,
    RecipeMaterialRow* rows, std::size_t capacity, bool hand_contract_verified = false,
    bool listed_ingredient_contract_verified = false) noexcept {
  namespace cat = spirit_recipe_catalog;
  namespace math = recipe_material_detail;
  using Issue = SpiritMaterialIssue;
  SpiritMaterial out;
  out.quantity = quantity;
  if (!cat::structurally_valid(descriptor)) {
    out.issue = Issue::descriptor;
    return out;
  }
  if (!rows || capacity < descriptor.targets.count) {
    out.issue = Issue::output_buffer;
    return out;
  }
  const auto& d = *cat::find(descriptor.id);
  const auto* selected = cat::variant(d, branch);
  out.count = d.targets.count;
  out.branch_known = selected != nullptr;
  out.output = selected ? selected->output.symbol : -1;
  for (std::size_t i = 0; i < out.count; ++i) {
    auto& row = rows[i];
    row = {};
    row.symbol = d.targets[i].symbol;
    row.unit_cost = math::units(d.costs, row.symbol);
    row.unit_output =
        selected && row.symbol == selected->output.symbol ? selected->output.units : 0;
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
  if (inventory.items.count != out.count || !inventory.items.data) {
    out.issue = Issue::target_mapping;
    return out;
  }
  std::uint64_t listed = 0, packed = 0;
  for (std::size_t i = 0; i < out.count; ++i) {
    auto& row = rows[i];
    const auto* item = math::item(inventory, row.symbol);
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
    if (row.unit_cost &&
        (item->listed_occurrences > 1 || item->packed_occurrences > 1 ||
         item->duplicate_packed_target || item->has_active_marker || item->mixed_representation ||
         (item->listed_occurrences && item->packed_occurrences)))
      out.issue = Issue::ambiguous_ingredient;
  }
  if (listed > inventory.listed_nodes || packed > inventory.packed_entries) {
    out.issue = Issue::invalid_inventory;
    return out;
  }
  out.inventory_known = true;
  out.equipped_ingredient =
      out.issue == Issue::ambiguous_ingredient && math::equipped_ingredient(inventory, d.costs);
  auto hand_issue = Issue::none;
  if (!hand_contract_verified || !hands.left_slot_known || !hands.right_slot_known ||
      !hands.interact_item_known)
    hand_issue = Issue::hand_contract;
  else if (hands.hands.left.state != 0 || hands.hands.left.object || hands.hands.right.state != 1 ||
           !hands.hands.right.object || hands.hands.right.symbol != 16604 ||
           hands.hands.right.amount != 1 || hands.interact_item != hands.hands.right.object)
    hand_issue = Issue::hand_object;
  else if (!math::no_hand_alias(inventory, hands.hands))
    hand_issue = Issue::hand_alias;
  else
    out.hand_known = true;
  if (out.issue == Issue::none)
    out.issue = hand_issue;
  out.ingredients_verified = listed_ingredient_contract_verified;
  if (out.issue == Issue::none && !out.branch_known)
    out.issue = Issue::unknown_branch;
  if (out.issue == Issue::none && !out.ingredients_verified)
    out.issue = Issue::ingredient_objects;
  std::uint64_t maximum = d.maximum_quantity;
  const bool representable = quantity <= d.maximum_quantity;
  for (std::size_t i = 0; i < out.count; ++i) {
    auto& row = rows[i];
    if (row.symbol == 16604) {
      row.held = row.reserved = out.hand_known ? 1 : -1;
      row.available_known = out.hand_known;
      row.available = out.hand_known ? row.stored + 1 : -1;
    }
    row.output_space = INT32_MAX - row.stored;
    row.output_shortfall_for_one = math::positive(std::int64_t{row.unit_output} - row.output_space);
    row.missing_for_one =
        row.available_known ? math::positive(std::int64_t{row.unit_cost} - row.available) : -1;
    if (row.unit_cost && row.available_known)
      math::limit(maximum, static_cast<std::uint64_t>(row.available) / row.unit_cost);
    if (row.unit_output)
      math::limit(maximum, static_cast<std::uint64_t>(row.output_space) / row.unit_output);
    if (representable) {
      row.required = std::int64_t{row.unit_cost} * quantity;
      row.produced = std::int64_t{row.unit_output} * quantity;
      row.missing = row.available_known ? math::positive(row.required - row.available) : -1;
      row.remaining = row.available_known ? row.available - row.required + row.produced : -1;
      row.output_shortfall = math::positive(row.produced - row.output_space);
    }
  }
  if (out.known())
    out.maximum = static_cast<std::uint32_t>(maximum);
  if (!representable && out.issue == Issue::none)
    out.issue = Issue::quantity;
  out.additional_flasks = representable && quantity ? quantity - 1 : 0;
  out.craftable = out.known() && quantity && quantity <= out.maximum;
  return out;
}

struct SpiritRecipeKey {
  std::uint64_t generation = 0;
  std::uint32_t thread = 0, world = 0, hero = 0, station = 0, manager = 0;
  recipe_catalog::Station kind = recipe_catalog::Station::alchemy;
};
struct SpiritCaptureProofs {
  bool stable_boundary_verified = false;
  // LAB/hand/end contract bound to the same key currently, not a historical bool.
  bool station_hand_end_verified = false;
};
enum class SpiritCaptureIssue { none, context, boundary, reentry, catalogue };
struct SpiritRecipeState {
  spirit_recipe_catalog::RecipeId id = spirit_recipe_catalog::RecipeId::spirit;
  bool evaluated = false, known = false, contract = false, branch_known = false,
       actor_known = false, actor_matches = false;
  bool context_known = false, context_matches = false, owner_conditions_match = false;
  std::int32_t branch_value = 0, output = -1;
  spirit_recipe_catalog::Branch branch = spirit_recipe_catalog::Branch::unknown;
  SpiritMaterial material{};
  std::array<RecipeMaterialRow, spirit_recipe_catalog::target_capacity> rows{};
};
struct SpiritCaptureDiagnostic {
  SpiritCaptureIssue issue = SpiritCaptureIssue::none;
  PackedInventoryIssue inventory_issue = PackedInventoryIssue::none;
  StationHandReadIssue hand_issue = StationHandReadIssue::none;
  bool inventory_unchanged = false, hands_unchanged = false, actor_unchanged = false,
       context_unchanged = false, branch_unchanged = false, knowledge_unchanged = false;
  bool ingredient_objects_verified = false;
  std::size_t target_count = spirit_recipe_catalog::target_capacity;
  spirit_recipe_runtime::Verification profile{};
  spirit_recipe_runtime::Knowledge knowledge_before{}, knowledge_after{};
  spirit_recipe_runtime::BranchEvidence branch_before{}, branch_after{};
  spirit_recipe_runtime::ActorEvidence actor_before{}, actor_after{};
  spirit_recipe_runtime::ContextEvidence context_before{}, context_after{};
};
struct SpiritRecipeCapture {
  SpiritRecipeKey key{};
  SpiritRecipeState state{};
  StationRecipeHands hands{};
  SpiritCaptureDiagnostic diagnostic{};
};

// Persistent own workspace; pre/post checks of world/thread/mob/manager
// identity lie with the native adapter. No heap, engine call, or
// claimed async/ABA protection. The capture values authorize no writer.
class SpiritRecipeReader {
public:
  SpiritRecipeReader() noexcept
      : profile_(spirit_recipe_runtime::assemble(spirit_recipe_catalog::recipes[0])) {}
  SpiritRecipeReader(const SpiritRecipeReader&) = delete;
  SpiritRecipeReader& operator=(const SpiritRecipeReader&) = delete;
  SpiritRecipeCapture capture(const ReadOnlyMemory& memory, std::uint32_t parser,
                              const SpiritRecipeKey& key, SpiritCaptureProofs proof = {}) noexcept {
    namespace cat = spirit_recipe_catalog;
    namespace rt = spirit_recipe_runtime;
    namespace hand = station_recipe_snapshot_detail;
    SpiritRecipeCapture out;
    out.key = key;
    auto& d = out.diagnostic;
    auto& s = out.state;
    for (std::size_t i = 0; i < cat::targets.size(); ++i) {
      s.rows[i].symbol = cat::targets[i].symbol;
      s.rows[i].unit_cost =
          recipe_material_detail::units(cat::view(cat::costs), cat::targets[i].symbol);
    }
    if (reading_) {
      d.issue = SpiritCaptureIssue::reentry;
      return out;
    }
    if (!profile_.valid()) {
      d.issue = SpiritCaptureIssue::catalogue;
      return out;
    }
    if (!key.generation || !key.thread || !key.world || !key.hero || !key.station || !key.manager ||
        key.kind != recipe_catalog::Station::alchemy) {
      d.issue = SpiritCaptureIssue::context;
      return out;
    }
    if (!proof.stable_boundary_verified) {
      d.issue = SpiritCaptureIssue::boundary;
      return out;
    }
    reading_ = true;
    struct Lease {
      bool& flag;
      ~Lease() { flag = false; }
    } lease{reading_};
    s.evaluated = true;
    d.actor_before = rt::actor(memory, parser, profile_, key.hero);
    d.context_before = rt::context(memory, parser, profile_);
    d.knowledge_before = rt::knowledge(memory, parser, profile_);
    d.branch_before = rt::branch(memory, parser, profile_);
    packed_detail::Header initial{}, final{};
    const bool header = packed_detail::at_offset(memory, key.hero, packed_detail::header_offset,
                                                 initial.data(), initial.size());
    auto inventory = read_packed_inventory(memory, key.hero, cat::targets, true, inventory_);
    d.hand_issue = hand::begin_hands(memory, key.hero, hands_);
    d.ingredient_objects_verified = inventory.listed_known && hands_.captured;
    std::size_t objects = 0;
    if (inventory.listed_known && inventory.snapshot_unchanged &&
        inventory.listed_nodes <= objects_.size()) {
      objects = inventory.listed_nodes;
      for (std::size_t i = 0; i < objects; ++i) {
        const auto& record = inventory_.records[i];
        auto& fields = fields_[i];
        fields = {};
        fields.object = objects_[i] = runtime_script::detail::u32(record.node.data() + 4);
        fields.known =
            alchemy_flask_runtime::detail::word(memory, fields.object, 0, fields.type) &&
            alchemy_flask_runtime::detail::word(memory, fields.object, 0x158, fields.flags);
        const auto symbol = station_detail::little_i32(record.item_fields.data() + 4);
        if (recipe_material_detail::units(cat::view(cat::costs), symbol)) {
          bool alias = false;
          for (std::size_t j = 0; j < hands_.count; ++j)
            if (runtime_script::detail::u32(hands_.records[j].fields.data() + 0x30) ==
                fields.object)
              alias = true;
          if (!fields.known || fields.type != hand_detail::item_vtable ||
              !(fields.flags & 0x200000u) || (fields.flags & 0x40000000u) || alias)
            d.ingredient_objects_verified = false;
        }
      }
    }
    d.profile = rt::verify_recipe(memory, parser, profile_);
    d.knowledge_after = rt::knowledge(memory, parser, profile_);
    d.branch_after = rt::branch(memory, parser, profile_);
    d.knowledge_unchanged = rt::same_integer_evidence(d.knowledge_before, d.knowledge_after);
    d.branch_unchanged = rt::same_branch_evidence(d.branch_before, d.branch_after);
    d.actor_after = rt::actor(memory, parser, profile_, key.hero);
    d.context_after = rt::context(memory, parser, profile_);
    d.actor_unchanged = rt::same_actor_evidence(d.actor_before, d.actor_after);
    d.context_unchanged = rt::same_context_evidence(d.context_before, d.context_after);
    d.hands_unchanged =
        d.hand_issue == StationHandReadIssue::none && hand::verify_hands(memory, hands_);
    if (hands_.captured && !d.hands_unchanged)
      d.hand_issue = StationHandReadIssue::changed;
    if (d.hands_unchanged)
      out.hands = hands_.before;
    bool metadata = true;
    for (std::size_t i = 0; i < objects; ++i) {
      std::uint32_t type = 0, flags = 0;
      const auto& f = fields_[i];
      if (!f.known || !alchemy_flask_runtime::detail::word(memory, f.object, 0, type) ||
          !alchemy_flask_runtime::detail::word(memory, f.object, 0x158, flags) || type != f.type ||
          flags != f.flags)
        metadata = false;
    }
    d.inventory_unchanged =
        header && inventory.storage_totals_known && inventory.snapshot_unchanged && metadata &&
        packed_detail::compare_list(memory, inventory_, inventory.listed_nodes) ==
            PackedInventoryIssue::none &&
        packed_detail::compare_pack(
            memory, inventory_, runtime_script::detail::u32(initial.data() + 28),
            runtime_script::detail::u32(initial.data() + 32)) == PackedInventoryIssue::none &&
        packed_detail::at_offset(memory, key.hero, packed_detail::header_offset, final.data(),
                                 final.size()) &&
        initial == final;
    if (!d.inventory_unchanged) {
      inventory.storage_totals_known = inventory.snapshot_unchanged = false;
      if (inventory.issue == PackedInventoryIssue::none)
        inventory.issue = PackedInventoryIssue::changed_during_read;
    }
    d.inventory_issue = inventory.issue;
    d.ingredient_objects_verified =
        d.ingredient_objects_verified && d.inventory_unchanged && d.hands_unchanged;
    RecipeInventory projected;
    projected.header_known = inventory.header_known;
    projected.listed_known = inventory.listed_known;
    projected.packed_known = inventory.packed_known;
    projected.storage_totals_known = inventory.storage_totals_known;
    projected.stable_boundary_verified = inventory.stable_boundary_verified;
    projected.snapshot_unchanged = inventory.snapshot_unchanged;
    projected.issue = inventory.issue;
    projected.listed_nodes = inventory.listed_nodes;
    projected.packed_entries = inventory.packed_entries;
    projected.listed_objects = {objects_.data(), objects};
    for (std::size_t i = 0; i < cat::targets.size(); ++i)
      items_[i] = {cat::targets[i].symbol,
                   inventory.listed[i],
                   inventory.packed[i],
                   inventory.total[i],
                   inventory.listed_occurrences[i],
                   inventory.packed_occurrences[i],
                   inventory.duplicate_packed_target[i],
                   inventory.has_active_marker[i],
                   inventory.mixed_representation[i]};
    projected.items = {items_.data(), items_.size()};
    s.known = d.knowledge_unchanged && d.knowledge_before.known && d.knowledge_after.known;
    s.contract = d.profile.valid;
    s.branch_known = d.branch_unchanged;
    if (s.branch_known) {
      s.branch = d.branch_before.branch;
      s.branch_value = d.branch_before.value;
      const auto* v = cat::variant(cat::recipes[0], s.branch);
      if (v)
        s.output = v->output.symbol;
    }
    s.actor_known = d.actor_unchanged;
    s.actor_matches = s.actor_known && d.actor_before.matches && d.actor_after.matches;
    s.context_known = d.context_unchanged;
    s.context_matches =
        s.context_known && d.context_before.station_matches() && d.context_after.station_matches();
    s.owner_conditions_match =
        s.context_known && d.context_before.owner_matches() && d.context_after.owner_matches();
    s.material = spirit_recipe_material(
        cat::recipes[0], projected, out.hands, s.branch, 0, s.rows.data(), s.rows.size(),
        proof.station_hand_end_verified && d.hands_unchanged, d.ingredient_objects_verified);
    return out;
  }

private:
  struct ItemFields {
    std::uint32_t object = 0, type = 0, flags = 0;
    bool known = false;
  };
  spirit_recipe_runtime::ProfileAssembly profile_{};
  PackedInventoryWorkspace inventory_{};
  station_recipe_snapshot_detail::HandWorkspace hands_{};
  std::array<std::uint32_t, max_packed_inventory_nodes> objects_{};
  std::array<ItemFields, max_packed_inventory_nodes> fields_{};
  std::array<RecipeInventoryItem, spirit_recipe_catalog::target_capacity> items_{};
  bool reading_ = false;
};
} // namespace ubc
