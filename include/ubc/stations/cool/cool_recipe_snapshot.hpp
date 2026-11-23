// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_recipe_plan.hpp"
#include "ubc/stations/cool/cool_recipe_runtime.hpp"
#include "ubc/stations/station_recipe_snapshot.hpp"
#include <algorithm>
namespace ubc {
struct CoolCaptureProofs {
  bool stable_boundary_verified = false, station_hand_end_verified = false;
  // No value derived from the MSB text sequence: the current native contract
  // for exactly this station/generation/hand must bind the read temporary level.
  std::int32_t verified_left_tmp_level = -1;
};
enum class CoolMaterialIssue {
  none,
  inventory,
  target_mapping,
  ambiguous_ingredient,
  hand_contract,
  hand_object,
  hand_alias,
  ingredient_objects,
  quantity
};
struct CoolMaterial {
  CoolMaterialIssue issue = CoolMaterialIssue::none;
  bool inventory_known = false, hand_known = false, ingredients_verified = false, eligible = false;
  bool equipped_ingredient = false;
  std::uint32_t maximum = 0, direct_maximum = 0;
  std::array<RecipeMaterialRow, 2> rows{};
  cool_recipe::Plan numerical{};
  bool known() const noexcept {
    return issue == CoolMaterialIssue::none && inventory_known && hand_known &&
           ingredients_verified;
  }
};
// Only current own value copies. eligible is a material/hand proof, not a
// writer/info/native-end ticket and no release for the additional debit.
inline CoolMaterial cool_recipe_material(const RecipeInventory& inventory,
                                         const StationRecipeHands& hands, std::uint32_t quantity,
                                         CoolCaptureProofs proof = {},
                                         bool ingredient_objects_verified = false) noexcept {
  CoolMaterial out;
  namespace math = recipe_material_detail;
  namespace cat = cool_recipe_catalog;
  for (std::size_t i = 0; i < 2; ++i) {
    out.rows[i].symbol = cat::targets[i].symbol;
    out.rows[i].unit_cost = i ? 0u : 1u;
    out.rows[i].unit_output = i ? 1u : 0u;
  }
  if (!inventory.header_known || !inventory.listed_known || !inventory.packed_known ||
      !inventory.storage_totals_known || !inventory.stable_boundary_verified ||
      !inventory.snapshot_unchanged || inventory.issue != PackedInventoryIssue::none ||
      inventory.listed_nodes > max_packed_inventory_nodes ||
      inventory.packed_entries > max_packed_inventory_entries) {
    out.issue = CoolMaterialIssue::inventory;
    return out;
  }
  if (!inventory.items.data || inventory.items.count != 2) {
    out.issue = CoolMaterialIssue::target_mapping;
    return out;
  }
  cool_recipe::Evidence values;
  values.stable_boundary_verified = true;
  std::uint64_t listed = 0, packed = 0;
  for (std::size_t i = 0; i < 2; ++i) {
    auto& row = out.rows[i];
    const auto* s = math::item(inventory, row.symbol);
    if (!s) {
      out.issue = CoolMaterialIssue::target_mapping;
      return out;
    }
    if (s->listed < 0 || s->packed < 0 || s->total < 0 ||
        std::int64_t{s->listed} + s->packed != s->total ||
        (s->listed > 0 && !s->listed_occurrences) ||
        ((s->packed > 0) != (s->packed_occurrences > 0)) ||
        s->listed_occurrences > inventory.listed_nodes ||
        s->packed_occurrences > inventory.packed_entries) {
      out.issue = CoolMaterialIssue::inventory;
      return out;
    }
    listed += s->listed_occurrences;
    packed += s->packed_occurrences;
    row.stored = row.available = s->total;
    row.available_known = true;
    const bool unambiguous = s->listed_occurrences <= 1 && s->packed_occurrences <= 1 &&
                             !s->duplicate_packed_target && !s->has_active_marker &&
                             !s->mixed_representation &&
                             !(s->listed_occurrences && s->packed_occurrences);
    if (!i) {
      out.equipped_ingredient = math::ingredient_problem(*s) == IngredientProblem::equipped;
      if (!unambiguous)
        out.issue = CoolMaterialIssue::ambiguous_ingredient;
    }
    values.stocks[i] = {row.symbol, s->total, true, unambiguous};
    row.missing_for_one = math::positive(std::int64_t{row.unit_cost} - row.available);
    row.output_space = INT32_MAX - row.stored;
    row.output_shortfall_for_one = math::positive(std::int64_t{row.unit_output} - row.output_space);
    if (quantity <= INT32_MAX) {
      row.required = std::int64_t{row.unit_cost} * quantity;
      row.produced = std::int64_t{row.unit_output} * quantity;
      row.missing = math::positive(row.required - row.available);
      row.remaining = row.available - row.required + row.produced;
      row.output_shortfall = math::positive(row.produced - row.output_space);
    }
  }
  if (listed > inventory.listed_nodes || packed > inventory.packed_entries) {
    out.issue = CoolMaterialIssue::inventory;
    return out;
  }
  out.inventory_known = true;
  out.numerical = cool_recipe::plan(cat::recipe, values, quantity);
  const auto& left = hands.hands.left;
  const auto& right = hands.hands.right;
  if (left.state == 1 && left.symbol == 16612 && left.amount == 1)
    out.rows[0].held = 1; // Information, never available += held.
  CoolMaterialIssue hand_issue = CoolMaterialIssue::none;
  if (!proof.stable_boundary_verified || !proof.station_hand_end_verified ||
      (proof.verified_left_tmp_level != 1 && proof.verified_left_tmp_level != 2) ||
      !hands.left_slot_known || !hands.right_slot_known || !hands.interact_item_known)
    hand_issue = CoolMaterialIssue::hand_contract;
  else if (left.state != 1 || !left.object || left.symbol != 16612 || left.amount != 1 ||
           right.state != 0 || right.object || hands.interact_item != left.object ||
           hands.left_tmp_level != proof.verified_left_tmp_level)
    hand_issue = CoolMaterialIssue::hand_object;
  else if (!math::no_hand_alias(inventory, hands.hands))
    hand_issue = CoolMaterialIssue::hand_alias;
  else
    out.hand_known = true;
  if (out.issue == CoolMaterialIssue::none)
    out.issue = hand_issue;
  out.ingredients_verified = ingredient_objects_verified;
  if (out.issue == CoolMaterialIssue::none && !out.ingredients_verified)
    out.issue = CoolMaterialIssue::ingredient_objects;
  if (out.known()) {
    out.maximum = out.numerical.maximum;
    out.direct_maximum = out.numerical.direct_maximum;
  }
  if (out.issue == CoolMaterialIssue::none && quantity &&
      (!out.numerical.valid || quantity > out.maximum))
    out.issue = CoolMaterialIssue::quantity;
  out.eligible = out.known() && out.numerical.valid;
  return out;
}
struct CoolRecipeKey {
  std::uint64_t generation = 0;
  std::uint32_t thread = 0, world = 0, hero = 0, station = 0, manager = 0;
  std::int32_t production_kind = 13;
};
enum class CoolCaptureIssue { none, context, boundary, reentry, catalogue };
struct CoolRecipeState {
  cool_recipe_catalog::RecipeId id = cool_recipe_catalog::RecipeId::blade;
  bool evaluated = false, known = false, contract = false, actor_known = false,
       actor_matches = false, context_known = false, context_matches = false;
  CoolMaterial material{};
};
struct CoolCaptureDiagnostic {
  CoolCaptureIssue issue = CoolCaptureIssue::none;
  PackedInventoryIssue inventory_issue = PackedInventoryIssue::none;
  StationHandReadIssue hand_issue = StationHandReadIssue::none;
  bool inventory_unchanged = false, hands_unchanged = false, actor_unchanged = false,
       context_unchanged = false, knowledge_unchanged = false;
  cool_recipe_runtime::ActorEvidence actor_before{}, actor_after{};
  cool_recipe_runtime::ContextEvidence context_before{}, context_after{};
  cool_recipe_runtime::Knowledge knowledge_before{}, knowledge_after{};
  cool_recipe_runtime::Verification profile{};
};
struct CoolRecipeCatalogueCapture {
  CoolRecipeKey key{};
  CoolRecipeState recipe{};
  StationRecipeHands hands{};
  CoolCaptureDiagnostic diagnostic{};
};
namespace cool_recipe_snapshot_detail {
inline bool item_class(const ReadOnlyMemory& m) noexcept {
  std::uint32_t method = 0;
  std::array<std::uint8_t, hand_detail::item_class_bytes.size()> bytes{};
  return alchemy_flask_runtime::detail::word(m, hand_detail::item_vtable, 0, method) &&
         method == hand_detail::item_class_method && m.copy(method, bytes.data(), bytes.size()) &&
         bytes == hand_detail::item_class_bytes;
}
}
// Persistent own game-thread workspace. A shared full inventory and
// handle read, then comparison of all read list/package/slot and item
// fields. World/thread/mob strings before/after capture are bound by the
// adapter; two reads alone are not synchronization against foreign writers.
class CoolRecipeCatalogueReader {
public:
  CoolRecipeCatalogueReader() noexcept = default;
  CoolRecipeCatalogueReader(const CoolRecipeCatalogueReader&) = delete;
  CoolRecipeCatalogueReader& operator=(const CoolRecipeCatalogueReader&) = delete;
  CoolRecipeCatalogueCapture capture(const ReadOnlyMemory& m, std::uint32_t parser,
                                     const CoolRecipeKey& key,
                                     CoolCaptureProofs proof = {}) noexcept {
    namespace cat = cool_recipe_catalog;
    namespace rt = cool_recipe_runtime;
    namespace hand = station_recipe_snapshot_detail;
    CoolRecipeCatalogueCapture out;
    out.key = key;
    auto& d = out.diagnostic;
    auto& s = out.recipe;
    for (std::size_t i = 0; i < 2; ++i) {
      s.material.rows[i].symbol = cat::targets[i].symbol;
      s.material.rows[i].unit_cost = i ? 0u : 1u;
      s.material.rows[i].unit_output = i ? 1u : 0u;
    }
    if (reading_) {
      d.issue = CoolCaptureIssue::reentry;
      return out;
    }
    if (!profile_.valid()) {
      d.issue = CoolCaptureIssue::catalogue;
      return out;
    }
    if (!key.generation || !key.thread || !key.world || !key.hero || !key.station || !key.manager ||
        key.production_kind != 13) {
      d.issue = CoolCaptureIssue::context;
      return out;
    }
    if (!proof.stable_boundary_verified) {
      d.issue = CoolCaptureIssue::boundary;
      return out;
    }
    reading_ = true;
    struct Lease {
      bool& value;
      ~Lease() { value = false; }
    } lease{reading_};
    d.actor_before = rt::actor(m, parser, profile_, key.hero);
    d.context_before = rt::context(m, parser, profile_);
    d.knowledge_before = rt::knowledge(m, parser, profile_);
    packed_detail::Header initial{}, final{};
    const bool header = packed_detail::at_offset(m, key.hero, packed_detail::header_offset,
                                                 initial.data(), initial.size());
    auto inventory = read_packed_inventory(m, key.hero, cat::targets, true, inventory_);
    d.hand_issue = hand::begin_hands(m, key.hero, hands_);
    std::size_t objects = 0;
    auto listed_problem = IngredientProblem::none;
    const bool class_valid = cool_recipe_snapshot_detail::item_class(m);
    if (inventory.listed_known && inventory.snapshot_unchanged &&
        inventory.listed_nodes <= objects_.size()) {
      objects = inventory.listed_nodes;
      for (std::size_t i = 0; i < objects; ++i) {
        const auto& r = inventory_.records[i];
        auto& f = fields_[i];
        f = {};
        f.object = objects_[i] = runtime_script::detail::u32(r.node.data() + 4);
        f.known = alchemy_flask_runtime::detail::word(m, f.object, 0, f.type) &&
                  alchemy_flask_runtime::detail::word(m, f.object, 0x154, f.mainflag) &&
                  alchemy_flask_runtime::detail::word(m, f.object, 0x158, f.flags);
        if (station_detail::little_i32(r.item_fields.data() + 4) != 16612)
          continue;
        bool alias = false;
        for (std::size_t j = 0; j < hands_.count; ++j)
          if (runtime_script::detail::u32(hands_.records[j].fields.data() + 0x30) == f.object)
            alias = true;
        const bool known_type = class_valid && f.known && f.type == hand_detail::item_vtable &&
                                f.mainflag == 1 && hands_.captured;
        listed_problem = std::max(listed_problem, recipe_material_detail::listed_ingredient_problem(
                                                      known_type, f.flags, 0x200001, alias));
      }
    }
    s.evaluated = true;
    d.profile = rt::verify_recipe(m, parser, cat::recipe);
    d.knowledge_after = rt::knowledge(m, parser, profile_);
    d.actor_after = rt::actor(m, parser, profile_, key.hero);
    d.context_after = rt::context(m, parser, profile_);
    d.knowledge_unchanged = d.knowledge_before.read_valid && d.knowledge_after.read_valid &&
                            d.knowledge_before.known == d.knowledge_after.known;
    d.actor_unchanged = rt::same_actor_evidence(d.actor_before, d.actor_after);
    d.context_unchanged = rt::same_context_evidence(d.context_before, d.context_after);
    d.hands_unchanged = d.hand_issue == StationHandReadIssue::none && hand::verify_hands(m, hands_);
    if (hands_.captured && !d.hands_unchanged)
      d.hand_issue = StationHandReadIssue::changed;
    if (d.hands_unchanged)
      out.hands = hands_.before;
    bool metadata = true;
    for (std::size_t i = 0; i < objects; ++i) {
      std::uint32_t type = 0, mainflag = 0, flags = 0;
      const auto& f = fields_[i];
      if (!f.known || !alchemy_flask_runtime::detail::word(m, f.object, 0, type) ||
          !alchemy_flask_runtime::detail::word(m, f.object, 0x154, mainflag) ||
          !alchemy_flask_runtime::detail::word(m, f.object, 0x158, flags) || type != f.type ||
          mainflag != f.mainflag || flags != f.flags)
        metadata = false;
    }
    d.inventory_unchanged =
        header && inventory.storage_totals_known && inventory.snapshot_unchanged && metadata &&
        cool_recipe_snapshot_detail::item_class(m) &&
        packed_detail::compare_list(m, inventory_, inventory.listed_nodes) ==
            PackedInventoryIssue::none &&
        packed_detail::compare_pack(m, inventory_, runtime_script::detail::u32(initial.data() + 28),
                                    runtime_script::detail::u32(initial.data() + 32)) ==
            PackedInventoryIssue::none &&
        packed_detail::at_offset(m, key.hero, packed_detail::header_offset, final.data(),
                                 final.size()) &&
        initial == final;
    if (!d.inventory_unchanged) {
      inventory.storage_totals_known = inventory.snapshot_unchanged = false;
      if (inventory.issue == PackedInventoryIssue::none)
        inventory.issue = PackedInventoryIssue::changed_during_read;
    }
    d.inventory_issue = inventory.issue;
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
    for (std::size_t i = 0; i < 2; ++i)
      items_[i] = {cat::targets[i].symbol,
                   inventory.listed[i],
                   inventory.packed[i],
                   inventory.total[i],
                   inventory.listed_occurrences[i],
                   inventory.packed_occurrences[i],
                   inventory.duplicate_packed_target[i],
                   inventory.has_active_marker[i],
                   inventory.mixed_representation[i],
                   i == 0 ? listed_problem : IngredientProblem::none};
    projected.items = {items_.data(), items_.size()};
    s.known = d.knowledge_unchanged && d.knowledge_before.known;
    s.contract = d.profile.valid;
    s.actor_known = d.actor_unchanged;
    s.actor_matches = s.actor_known && d.actor_before.matches && d.actor_after.matches;
    s.context_known = d.context_unchanged;
    s.context_matches =
        s.context_known && d.context_before.station_matches() && d.context_after.station_matches();
    s.material = cool_recipe_material(projected, out.hands, 0, proof,
                                      listed_problem == IngredientProblem::none && class_valid &&
                                          d.inventory_unchanged && d.hands_unchanged);
    return out;
  }

private:
  struct Fields {
    std::uint32_t object = 0, type = 0, mainflag = 0, flags = 0;
    bool known = false;
  };
  cool_recipe_runtime::ProfileAssembly profile_ =
      cool_recipe_runtime::assemble(cool_recipe_catalog::recipe);
  PackedInventoryWorkspace inventory_{};
  station_recipe_snapshot_detail::HandWorkspace hands_{};
  std::array<std::uint32_t, max_packed_inventory_nodes> objects_{};
  std::array<Fields, max_packed_inventory_nodes> fields_{};
  std::array<RecipeInventoryItem, 2> items_{};
  bool reading_ = false;
};
} // namespace ubc
