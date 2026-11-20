// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/stations/grind/grind_recipe_material.hpp"
#include "ubc/stations/grind/grind_recipe_runtime.hpp"
#include "ubc/stations/station_recipe_snapshot.hpp"

namespace ubc {
inline constexpr std::size_t grind_recipe_target_count = grind_recipe_profile::items.size();
struct GrindRecipeKey {
  std::uint64_t generation = 0, frame = 0;
  std::uint32_t thread = 0, world = 0, hero = 0, station = 0, manager = 0;
  std::int32_t production_kind = 11;
};
enum class GrindCaptureIssue { none, context, boundary, catalogue, reentry };

// Our observations do not authorize item, Remove or Create operations. In particular,
// mainflag/flags alone cannot establish the canonical form of a weapon for grinding.
struct GrindItemObservation {
  std::int32_t symbol = -1;
  std::uint32_t listed_count = 0, object = 0, type = 0, mainflag = 0, flags = 0;
  bool fields_known = false, exact_item_type = false;
  bool active_bit_seen = false, slot_alias_seen = false;
};
struct GrindNativeBindingRequest {
  // Return to a future concrete binder. There is intentionally
  // no callback and no bool release that closes these gaps.
  GrindRecipeKey key{};
  std::uint32_t callback = 0;
  GrindItemObservation input{};
  StationRecipeHands hands{};
  static constexpr bool guild_object_contract_missing = true;
  static constexpr bool weapon_semantics_contract_missing = true;
  static constexpr bool station_end_contract_missing = true;
};
struct GrindRecipeState {
  std::uint32_t callback = 0;
  bool evaluated = false, contract = false, learned = false;
  bool actor_known = false, actor_matches = false, state_known = false, context_matches = false;
  GrindMaterial material{};
  GrindNativeBindingRequest binding_request{};
};
struct GrindCaptureDiagnostic {
  GrindCaptureIssue issue = GrindCaptureIssue::none;
  PackedInventoryIssue inventory_issue = PackedInventoryIssue::none;
  StationHandReadIssue hand_issue = StationHandReadIssue::none;
  bool inventory_unchanged = false, item_fields_unchanged = false, item_class_verified = false;
  bool hands_unchanged = false, actor_unchanged = false, state_unchanged = false;
  bool hand_inventory_alias = false;
  std::size_t target_count = 0;
  std::uint32_t listed_nodes = 0, packed_entries = 0;
  grind_recipe_runtime::ActorEvidence actor_before{}, actor_after{};
  grind_recipe_runtime::StateEvidence state_before{}, state_after{};
  std::array<grind_recipe_runtime::Verification, grind_recipe::recipes.size()> profiles{};
};
struct GrindRecipeCatalogueCapture {
  GrindRecipeKey key{};
  std::array<GrindRecipeState, grind_recipe::recipes.size()> recipes{};
  std::array<RecipeInventoryItem, grind_recipe_target_count> stocks{};
  std::array<GrindItemObservation, grind_recipe_target_count> items{};
  StationRecipeHands hands{};
  // Complete own copy for the existing material/alias
  // checker, including non-recipe-related listed objects.
  std::array<std::uint32_t, max_packed_inventory_nodes> listed_objects{};
  std::size_t listed_object_count = 0;
  bool listed_objects_known = false;
  GrindRecipeKey listed_objects_key{};
  GrindCaptureDiagnostic diagnostic{};
};
namespace grind_recipe_snapshot_detail {
inline bool valid_key(const GrindRecipeKey& k) noexcept {
  return k.generation && k.frame && k.thread && k.world && k.hero && k.station && k.manager &&
         k.production_kind == 11;
}
inline bool same_key(const GrindRecipeKey& a, const GrindRecipeKey& b) noexcept {
  return a.generation == b.generation && a.frame == b.frame && a.thread == b.thread &&
         a.world == b.world && a.hero == b.hero && a.station == b.station &&
         a.manager == b.manager && a.production_kind == b.production_kind;
}
inline bool item_class(const ReadOnlyMemory& memory) noexcept {
  std::uint32_t method = 0;
  std::array<std::uint8_t, hand_detail::item_class_bytes.size()> bytes{};
  return alchemy_flask_runtime::detail::word(memory, hand_detail::item_vtable, 0, method) &&
         method == hand_detail::item_class_method &&
         memory.copy(method, bytes.data(), bytes.size()) && bytes == hand_detail::item_class_bytes;
}
}

// Read-only preparation. Pre/post capture the adapter binds the actual
// game thread, the world, and the physical station to key/frame. No
// getter, no native quantity query, no NPC guild field offset, and
// no writer. A persistent reader belongs to a game thread; no heap
// allocation. The result holds all values and survives the next read.
class GrindRecipeCatalogueReader {
public:
  GrindRecipeCatalogueReader() noexcept {
    namespace facts = grind_recipe_profile;
    for (std::size_t i = 0; i < grind_recipe::recipes.size(); ++i) {
      profiles_[i] = grind_recipe_runtime::assemble(grind_recipe::recipes[i]);
      if (!profiles_[i].valid())
        return;
    }
    for (std::size_t i = 0; i < facts::items.size(); ++i) {
      const auto* f = facts::items[i].function;
      if (!f || f->symbol > INT32_MAX)
        return;
      targets_[i] = {f->name, static_cast<std::int32_t>(f->symbol)};
    }
    std::array<std::size_t, grind_recipe_target_count> lengths{};
    valid_ = packed_detail::validate_targets(targets_.data(), targets_.size(), lengths);
  }
  GrindRecipeCatalogueReader(const GrindRecipeCatalogueReader&) = delete;
  GrindRecipeCatalogueReader& operator=(const GrindRecipeCatalogueReader&) = delete;
  GrindRecipeCatalogueReader(GrindRecipeCatalogueReader&&) = delete;
  GrindRecipeCatalogueReader& operator=(GrindRecipeCatalogueReader&&) = delete;

  GrindRecipeCatalogueCapture capture(const ReadOnlyMemory& memory, std::uint32_t parser,
                                      const GrindRecipeKey& key,
                                      bool stable_boundary_verified) noexcept {
    namespace rt = grind_recipe_runtime;
    namespace hand = station_recipe_snapshot_detail;
    namespace word = alchemy_flask_runtime::detail;
    GrindRecipeCatalogueCapture out;
    out.key = key;
    auto& d = out.diagnostic;
    d.target_count = targets_.size();
    for (std::size_t i = 0; i < out.recipes.size(); ++i) {
      out.recipes[i].callback = grind_recipe::recipes[i].function.symbol;
      out.recipes[i].binding_request.key = key;
      out.recipes[i].binding_request.callback = out.recipes[i].callback;
    }
    for (std::size_t i = 0; i < targets_.size(); ++i)
      out.stocks[i].symbol = out.items[i].symbol = targets_[i].symbol;
    if (capturing_) {
      d.issue = GrindCaptureIssue::reentry;
      return out;
    }
    if (!valid_) {
      d.issue = GrindCaptureIssue::catalogue;
      return out;
    }
    if (!grind_recipe_snapshot_detail::valid_key(key) || parser != lhiver::game_parser) {
      d.issue = GrindCaptureIssue::context;
      return out;
    }
    if (!stable_boundary_verified) {
      d.issue = GrindCaptureIssue::boundary;
      return out;
    }
    capturing_ = true;
    struct Scope {
      bool& active;
      ~Scope() { active = false; }
    } scope{capturing_};
    d.actor_before = rt::actor(memory, parser, profiles_[0], key.hero);
    d.state_before = rt::state(memory, parser, profiles_[0]);
    packed_detail::Header initial{}, final{};
    const bool header = packed_detail::at_offset(memory, key.hero, packed_detail::header_offset,
                                                 initial.data(), initial.size());
    auto inventory = read_packed_inventory<grind_recipe_target_count>(
        memory, key.hero, targets_.data(), targets_.size(), true, inventory_);
    d.listed_nodes = inventory.listed_nodes;
    d.packed_entries = inventory.packed_entries;
    d.hand_issue = hand::begin_hands(memory, key.hero, hands_);
    const bool initial_class = grind_recipe_snapshot_detail::item_class(memory);
    std::size_t objects = 0;
    bool fields_read = true;
    for (auto& item : out.items) {
      item.fields_known = true;
      item.exact_item_type = initial_class;
    }
    if (inventory.listed_known && inventory.snapshot_unchanged &&
        inventory.listed_nodes <= objects_.size()) {
      objects = inventory.listed_nodes;
      for (std::size_t i = 0; i < objects; ++i) {
        const auto& record = inventory_.records[i];
        auto& field = fields_[i];
        field = {};
        field.object = objects_[i] = runtime_script::detail::u32(record.node.data() + 4);
        field.known = word::word(memory, field.object, 0, field.type) &&
                      word::word(memory, field.object, 0x154, field.mainflag) &&
                      word::word(memory, field.object, 0x158, field.flags);
        fields_read = fields_read && field.known;
        bool slot_alias = false;
        if (hands_.captured)
          for (std::size_t j = 0; j < hands_.count; ++j)
            if (runtime_script::detail::u32(hands_.records[j].fields.data() + 0x30) == field.object)
              slot_alias = true;
        const auto symbol = station_detail::little_i32(record.item_fields.data() + 4);
        const auto index = target_index(symbol);
        if (index != targets_.size()) {
          auto& item = out.items[index];
          ++item.listed_count;
          // A pointer is uniquely determined only for exactly one hit.
          // With multiple hits the aggregated negative features remain.
          item.object = item.listed_count == 1 ? field.object : 0;
          item.type = item.listed_count == 1 ? field.type : 0;
          item.mainflag = item.listed_count == 1 ? field.mainflag : 0;
          item.flags = item.listed_count == 1 ? field.flags : 0;
          item.fields_known = item.fields_known && field.known;
          item.exact_item_type =
              item.exact_item_type && field.known && field.type == hand_detail::item_vtable;
          item.active_bit_seen =
              item.active_bit_seen || (field.known && (field.flags & 0x40000000u));
          item.slot_alias_seen = item.slot_alias_seen || slot_alias;
        }
      }
    } else
      fields_read = false;
    for (std::size_t i = 0; i < out.recipes.size(); ++i) {
      out.recipes[i].evaluated = true;
      d.profiles[i] = rt::verify_recipe(memory, parser, grind_recipe::recipes[i]);
    }
    d.state_after = rt::state(memory, parser, profiles_[0]);
    d.actor_after = rt::actor(memory, parser, profiles_[0], key.hero);
    d.state_unchanged = rt::same_state(d.state_before, d.state_after);
    d.actor_unchanged = station_recipe_runtime::same_actor_evidence(d.actor_before, d.actor_after);
    d.hands_unchanged =
        d.hand_issue == StationHandReadIssue::none && hand::verify_hands(memory, hands_);
    if (hands_.captured && !d.hands_unchanged)
      d.hand_issue = StationHandReadIssue::changed;
    if (d.hands_unchanged)
      out.hands = hands_.before;
    d.item_fields_unchanged = fields_read;
    for (std::size_t i = 0; i < objects; ++i) {
      const auto& field = fields_[i];
      std::uint32_t type = 0, mainflag = 0, flags = 0;
      if (!field.known || !word::word(memory, field.object, 0, type) ||
          !word::word(memory, field.object, 0x154, mainflag) ||
          !word::word(memory, field.object, 0x158, flags) || type != field.type ||
          mainflag != field.mainflag || flags != field.flags)
        d.item_fields_unchanged = false;
      if (d.hands_unchanged && (field.object == out.hands.hands.left.object ||
                                field.object == out.hands.hands.right.object))
        d.hand_inventory_alias = true;
    }
    d.item_class_verified = initial_class && grind_recipe_snapshot_detail::item_class(memory);
    d.inventory_unchanged =
        header && inventory.storage_totals_known && inventory.snapshot_unchanged &&
        d.item_fields_unchanged &&
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
    if (d.inventory_unchanged) {
      out.listed_objects_known = true;
      out.listed_objects_key = key;
      out.listed_object_count = objects;
      for (std::size_t i = 0; i < objects; ++i)
        out.listed_objects[i] = objects_[i];
    }
    for (std::size_t i = 0; i < targets_.size(); ++i) {
      out.stocks[i] = {targets_[i].symbol,
                       inventory.listed[i],
                       inventory.packed[i],
                       inventory.total[i],
                       inventory.listed_occurrences[i],
                       inventory.packed_occurrences[i],
                       inventory.duplicate_packed_target[i],
                       inventory.has_active_marker[i],
                       inventory.mixed_representation[i]};
      out.items[i].fields_known =
          out.items[i].fields_known && d.item_fields_unchanged && d.inventory_unchanged;
      out.items[i].exact_item_type =
          out.items[i].exact_item_type && d.item_class_verified && out.items[i].fields_known;
      // For exclusively packed weapons no native object was observed.
      // Empty all-quantification would be no type proof here.
      if (!out.items[i].listed_count)
        out.items[i].fields_known = out.items[i].exact_item_type = false;
    }
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
    if (d.hand_inventory_alias)
      projected.storage_totals_known = false;
    GrindMaterialKnowledge knowledge;
    knowledge.talent_known = knowledge.skill_known = d.state_unchanged;
    knowledge.talent = d.state_before.talent;
    knowledge.skill = d.state_before.skill;
    // No hero+0x230 read access from mere parser field metadata. Guild
    // and both native proofs remain closed until an own binder exists.
    GrindMaterialProofs proof;
    proof.stable_boundary_verified =
        stable_boundary_verified && d.inventory_unchanged && d.hands_unchanged;
    for (std::size_t i = 0; i < out.recipes.size(); ++i) {
      const auto& recipe = grind_recipe::recipes[i];
      auto& state = out.recipes[i];
      state.contract = d.profiles[i].valid;
      state.state_known = d.state_unchanged;
      state.context_matches = state.state_known && d.state_before.station_postcondition() &&
                              d.state_after.station_postcondition();
      state.learned = state.state_known && d.state_before.learned() && d.state_after.learned();
      state.actor_known = d.actor_unchanged;
      state.actor_matches = state.actor_known && d.actor_before.matches && d.actor_after.matches;
      const std::array<std::int32_t, 3> symbols{
          {recipe.input, recipe.output, grind_recipe::lifecycle_item}};
      bool mapped = true;
      for (std::size_t j = 0; j < symbols.size(); ++j) {
        const auto index = target_index(symbols[j]);
        if (index == targets_.size()) {
          mapped = false;
          break;
        }
        items_[j] = out.stocks[index];
        if (!j)
          state.binding_request.input = out.items[index];
      }
      state.binding_request.hands = out.hands;
      if (!mapped) {
        state.material.issue = GrindMaterialIssue::target_mapping;
        continue;
      }
      projected.items = {items_.data(), items_.size()};
      state.material = grind_recipe_material(recipe, projected, out.hands, knowledge, 0, proof);
    }
    return out;
  }

private:
  std::size_t target_index(std::int32_t symbol) const noexcept {
    for (std::size_t i = 0; i < targets_.size(); ++i)
      if (targets_[i].symbol == symbol)
        return i;
    return targets_.size();
  }
  struct ItemFields {
    std::uint32_t object = 0, type = 0, mainflag = 0, flags = 0;
    bool known = false;
  };
  std::array<grind_recipe_runtime::ProfileAssembly, grind_recipe::recipes.size()> profiles_{};
  std::array<PackedInventoryTarget, grind_recipe_target_count> targets_{};
  PackedInventoryWorkspace inventory_{};
  station_recipe_snapshot_detail::HandWorkspace hands_{};
  std::array<ItemFields, max_packed_inventory_nodes> fields_{};
  std::array<std::uint32_t, max_packed_inventory_nodes> objects_{};
  std::array<RecipeInventoryItem, 3> items_{};
  bool valid_ = false, capturing_ = false;
};
} // namespace ubc
