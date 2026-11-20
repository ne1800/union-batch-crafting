// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/crafting_catalog.hpp"

namespace ubc {
inline void gate_bow_modules(CraftingSnapshot& snapshot, bool verified) noexcept {
  if (verified || snapshot.key.kind != crafting_catalog::Station::bow)
    return;
  for (std::size_t i = crafting_catalog::bow_begin; i < crafting_catalog::cool_begin; ++i) {
    snapshot.recipes[i].contract = false;
    snapshot.recipes[i].maximum = 0;
  }
}
// Value projection of the already bound N0 recording. A physical hero preview
// point does not replace SELF at the original CallFunc: there the native
// writer actor().matches must be read positively again. No engine calls.
inline CraftingSnapshot project_capture(const BowRecipeCatalogueCapture& source,
                                        bool physical_hero_preview_verified = false) noexcept {
  namespace cat = crafting_catalog;
  namespace rt = bow_recipe_runtime;
  CraftingSnapshot out;
  const auto& k = source.key;
  const auto& d = source.diagnostic;
  out.key = {k.generation, k.thread, k.world, k.hero, k.station, k.manager, cat::Station::bow};
  out.hands = source.hands;
  if (!crafting_detail::valid_key(out.key) || k.production_kind != 16 ||
      d.issue != BowCaptureIssue::none) {
    out.issue = CraftingProjectionIssue::context;
    return out;
  }
  for (std::size_t i = 0; i < source.recipes.size(); ++i) {
    const auto& s = source.recipes[i];
    const auto& descriptor = cat::recipes[cat::bow_begin + i];
    auto& value = out.recipes[cat::bow_begin + i];
    if (static_cast<cat::RecipeId>(s.id) != descriptor.id || s.material.quantity ||
        s.material.craftable || (s.evaluated && s.material.count != descriptor.targets.count)) {
      out.issue = CraftingProjectionIssue::source;
      return out;
    }
    if (!s.evaluated)
      continue;
    value.evaluated = true;
    value.generation = k.generation;
    value.knowledge_read_valid = d.knowledge_unchanged && d.knowledge_before.read_valid &&
                                 d.knowledge_after.read_valid &&
                                 d.knowledge_before.issue == runtime_script::Issue::none &&
                                 d.knowledge_after.issue == runtime_script::Issue::none &&
                                 d.knowledge_before.known == d.knowledge_after.known;
    value.known = value.knowledge_read_valid && s.known && d.knowledge_before.known;
    value.contract = s.contract && d.profiles[i].valid && d.profiles[i].local.matches;
    const auto& a = d.actor_before;
    const auto& b = d.actor_after;
    value.actor_known = s.actor_known && d.actor_unchanged && a.read_valid && b.read_valid &&
                        a.issue == runtime_script::Issue::none &&
                        b.issue == runtime_script::Issue::none && a.requires_self &&
                        b.requires_self && rt::same_actor_evidence(a, b);
    value.actor_matches =
        value.actor_known && a.hero == k.hero && b.hero == k.hero &&
        (physical_hero_preview_verified ||
         (s.actor_matches && a.matches && b.matches && a.self == k.hero && b.self == k.hero));
    value.context_known = s.context_known && d.context_unchanged &&
                          d.context_before.issue == runtime_script::Issue::none &&
                          d.context_after.issue == runtime_script::Issue::none &&
                          rt::same_context_evidence(d.context_before, d.context_after);
    // M_BOWS may change from 0 to 1 through the original category INFO.
    value.context_matches = value.context_known && s.context_matches &&
                            d.context_before.station_matches() && d.context_after.station_matches();
    value.material_known = s.material.inventory_known && d.inventory_unchanged &&
                           d.inventory_issue == PackedInventoryIssue::none;
    value.hand_known =
        s.material.hand_known && d.hands_unchanged && d.hand_issue == StationHandReadIssue::none;
    value.material_verified = value.material_known && value.hand_known && s.material.known() &&
                              s.ingredient_objects_verified;
    value.maximum = s.material.maximum;
    for (std::size_t j = 0; j < value.target_count; ++j) {
      auto& row = value.materials[j];
      const BowMaterialRow* original = nullptr;
      for (std::size_t n = 0; n < s.material.count; ++n)
        if (s.rows[n].symbol == row.symbol) {
          if (original) {
            out.issue = CraftingProjectionIssue::source;
            return out;
          }
          original = &s.rows[n];
        }
      if (!original || original->unit_cost != row.unit_cost ||
          original->unit_output != row.unit_output ||
          original->minimum_per_unit != row.unit_minimum || original->stored < -1 ||
          original->stored > INT32_MAX) {
        out.issue = CraftingProjectionIssue::source;
        return out;
      }
      if (value.material_known)
        row.stored = static_cast<std::int32_t>(original->stored);
    }
  }
  if (!valid_crafting_snapshot(out) || !crafting_detail::stocks_agree(out))
    out.issue = CraftingProjectionIssue::source;
  return out;
}
} // namespace ubc
