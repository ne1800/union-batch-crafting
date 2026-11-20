// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/grind_item_shape.hpp"
#include "ubc/grind_recipe_snapshot.hpp"
#include "ubc/grind_guild_snapshot.hpp"

namespace ubc {
enum class GrindItemBindingIssue {
  capture, key, recipe, profile, context, inventory, mapping, representation,
  object_type, shape, active, slot_alias, copied_values, none
};

// A value finding for exactly ONE input symbol, callback and Capture-Key.
// Only writer pre-finding: fresh SELF==HERO is mandatory here. A later
// menu preview needs its own physical HERO binding, no SELF lock.
// No remove/create release: the loaded native item path, further
// active NPC weapon references, guild and hand/end contract remain
// own boundaries. Never convert directly into input_objects_verified.
struct GrindInputObjectBinding {
  GrindItemBindingIssue issue = GrindItemBindingIssue::capture;
  GrindRecipeKey key{};
  std::uint32_t callback = 0;
  GrindItemObservation item{};
  RecipeInventoryItem stock{};
  StationRecipeHands hands{};
  lhiver::Digest constructor_sha256{};
  bool observed() const noexcept { return issue == GrindItemBindingIssue::none; }
  static constexpr bool native_effect_authorized = false;
};
// Own preview value type. No writer actor proof; the writer binds
// SELF further independently and fresh via bind_grind_input_object.
struct GrindPreviewInputObjectBinding {
  GrindInputObjectBinding values{};
  bool observed() const noexcept { return values.observed(); }
  static constexpr bool native_effect_authorized = false;
};

namespace grind_item_binding_detail {
using grind_recipe_snapshot_detail::same_key;
inline bool same_item(const GrindItemObservation& a, const GrindItemObservation& b) noexcept {
  return a.symbol == b.symbol && a.listed_count == b.listed_count && a.object == b.object &&
    a.type == b.type && a.mainflag == b.mainflag && a.flags == b.flags && a.fields_known == b.fields_known &&
    a.exact_item_type == b.exact_item_type && a.active_bit_seen == b.active_bit_seen && a.slot_alias_seen == b.slot_alias_seen;
}
inline bool same_hands(const StationRecipeHands& a, const StationRecipeHands& b) noexcept {
  return station_recipe_snapshot_detail::same_object(a.hands.left, b.hands.left) &&
    station_recipe_snapshot_detail::same_object(a.hands.right, b.hands.right) &&
    a.left_slot_known == b.left_slot_known && a.right_slot_known == b.right_slot_known &&
    a.left_tmp_level == b.left_tmp_level && a.right_tmp_level == b.right_tmp_level &&
    a.interact_item_known == b.interact_item_known && a.interact_item == b.interact_item;
}
}

// Evaluate only existing, own Capture copies; neither Memory nor
// Engine access and no heap allocation. current must be re-bound
// by the adapter before every use. Even an identical Key does
// not replace a fresh intake after a native effect or an input.
namespace grind_item_binding_detail {
inline GrindInputObjectBinding bind_values(const GrindRecipeCatalogueCapture& capture,
    const GrindRecipeKey& current, std::uint32_t callback) noexcept {
  namespace g = grind_recipe;
  namespace detail = grind_item_binding_detail;
  using Issue = GrindItemBindingIssue;
  GrindInputObjectBinding out;
  out.key = capture.key; out.callback = callback;
  if (!grind_recipe_snapshot_detail::valid_key(current) || !detail::same_key(capture.key, current)) {
    out.issue = Issue::key; return out;
  }
  const auto* recipe = g::find(callback);
  if (!recipe) { out.issue = Issue::recipe; return out; }
  std::size_t recipe_index = 0;
  while (recipe_index < g::recipes.size() && g::recipes[recipe_index].function.symbol != callback) ++recipe_index;
  if (recipe_index == g::recipes.size()) { out.issue = Issue::recipe; return out; }
  const auto& d = capture.diagnostic;
  const auto& state = capture.recipes[recipe_index];
  if (d.issue != GrindCaptureIssue::none || d.target_count != grind_recipe_target_count ||
      !d.hands_unchanged || d.hand_issue != StationHandReadIssue::none) return out;
  const auto& verified = d.profiles[recipe_index];
  if (!state.evaluated || state.callback != callback || !state.contract || !verified.valid ||
      !verified.local.matches || verified.local.issue != runtime_script::Issue::none ||
      verified.string_issue != alchemy_flask_runtime::Issue::none || verified.failed_symbol) {
    out.issue = Issue::profile; return out;
  }
  for (const auto symbol : {recipe->input, recipe->output, g::lifecycle_item}) {
    const auto* shape = grind_item_shape::find(static_cast<std::uint32_t>(symbol));
    if (!shape || !grind_item_shape::bound_to_profile(*shape)) { out.issue = Issue::shape; return out; }
  }
  if (!d.state_unchanged || !state.state_known || !state.context_matches ||
      !grind_recipe_runtime::same_state(d.state_before, d.state_after) ||
      !d.state_before.station_postcondition() || !d.state_after.station_postcondition()) {
    out.issue = Issue::context; return out;
  }
  if (d.inventory_issue != PackedInventoryIssue::none || !d.inventory_unchanged ||
      !d.item_fields_unchanged || !d.item_class_verified || !state.material.inventory_known ||
      d.listed_nodes > max_packed_inventory_nodes || d.packed_entries > max_packed_inventory_entries) {
    out.issue = Issue::inventory; return out;
  }
  // Copies in the recipe-related Request must also belong to this
  // catalog. No single foreign Observation-Struct as a supplier Boolean.
  const GrindItemObservation* input = nullptr;
  const RecipeInventoryItem* stock = nullptr;
  for (const auto& item : capture.items) if (item.symbol == recipe->input) {
    if (input) { out.issue = Issue::mapping; return out; } input = &item;
  }
  for (const auto& item : capture.stocks) if (item.symbol == recipe->input) {
    if (stock) { out.issue = Issue::mapping; return out; } stock = &item;
  }
  if (!input || !stock) { out.issue = Issue::mapping; return out; }
  if (state.binding_request.callback != callback || !detail::same_key(state.binding_request.key, current) ||
      !detail::same_item(state.binding_request.input, *input) ||
      !detail::same_hands(state.binding_request.hands, capture.hands) ||
      state.material.rows[0].symbol != recipe->input || state.material.rows[0].stored != stock->total) {
    out.issue = Issue::copied_values; return out;
  }
  // Pack-only is a set without an observed object. Hybrid
  // forms and multiple same-named objects are not merged here.
  if (input->listed_count != 1 || stock->listed_occurrences != 1 ||
      stock->listed <= 0 || stock->total != stock->listed || stock->packed != 0 ||
      stock->packed_occurrences || stock->duplicate_packed_target || stock->mixed_representation ||
      !d.listed_nodes) {
    out.issue = Issue::representation; return out;
  }
  if (!input->fields_known || !input->exact_item_type || !input->object || input->type != hand_detail::item_vtable) {
    out.issue = Issue::object_type; return out;
  }
  if (input->active_bit_seen || (input->flags & 0x40000000u) || stock->has_active_marker) {
    out.issue = Issue::active; return out;
  }
  if (d.hand_inventory_alias || input->slot_alias_seen || !capture.hands.left_slot_known ||
      !capture.hands.right_slot_known || !capture.hands.interact_item_known ||
      input->object == capture.hands.hands.left.object || input->object == capture.hands.hands.right.object ||
      input->object == capture.hands.interact_item) {
    out.issue = Issue::slot_alias; return out;
  }
  const auto* shape = grind_item_shape::find(static_cast<std::uint32_t>(recipe->input));
  if (input->mainflag != shape->mainflag || input->flags != shape->native_flags) {
    out.issue = Issue::shape; return out;
  }
  out.item = *input; out.stock = *stock; out.hands = capture.hands;
  out.constructor_sha256 = shape->constructor_sha256;
  out.issue = Issue::none;
  return out;
}
} // namespace grind_item_binding_detail

inline GrindInputObjectBinding bind_grind_input_object(const GrindRecipeCatalogueCapture& capture,
    const GrindRecipeKey& current, std::uint32_t callback) noexcept {
  auto out = grind_item_binding_detail::bind_values(capture, current, callback);
  if (!out.observed()) return out;
  const auto& d = capture.diagnostic;
  bool selected_actor = false;
  for (const auto& state : capture.recipes) if (state.callback == callback)
    selected_actor = state.actor_known && state.actor_matches;
  if (!selected_actor || !d.actor_unchanged || !station_recipe_runtime::same_actor_evidence(d.actor_before, d.actor_after) ||
      !d.actor_before.requires_self || !d.actor_after.requires_self ||
      d.actor_before.hero != current.hero || d.actor_before.self != current.hero ||
      d.actor_after.hero != current.hero || d.actor_after.self != current.hero) out.issue = GrindItemBindingIssue::context;
  return out;
}
inline GrindPreviewInputObjectBinding bind_grind_input_for_preview(const GrindRecipeCatalogueCapture& capture,
    const GrindGuildSnapshot& physical, const GrindRecipeKey& current, std::uint32_t callback) noexcept {
  GrindPreviewInputObjectBinding out;
  if (!grind_preview_knowledge(capture, physical, current).guild_known) {
    out.values.issue = GrindItemBindingIssue::context; return out;
  }
  // Only the physical HERO-symbol evidence is compared; SELF is
  // neither rewritten nor synthesized as successful for the preview.
  if (!station_recipe_runtime::detail::same_symbol(physical.hero_symbol, capture.diagnostic.actor_before.hero_symbol) ||
      !station_recipe_runtime::detail::same_symbol(physical.hero_symbol, capture.diagnostic.actor_after.hero_symbol)) {
    out.values.issue = GrindItemBindingIssue::context; return out;
  }
  out.values = grind_item_binding_detail::bind_values(capture, current, callback);
  return out;
}
} // namespace ubc
