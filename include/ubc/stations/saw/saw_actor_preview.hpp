// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/crafting_catalog.hpp"

namespace ubc {

// Only the five canonical saw recipes: idle preview and navigation refer
// to the positively bound physical hero. The original OnChoice sets
// SELF only after entering our outer wrapper. Therefore the readable,
// fully checked SELF-symbol contract remains mandatory; its current
// instance value here is not an actor release for a script effect. The
// integrator must demand actor().matches again directly at the inner
// CallFunc. This helper writes neither script symbols nor inventory.
inline void project_saw_actor_preview(CraftingSnapshot& snapshot,
                                      const station_recipe_runtime::ActorEvidence& before,
                                      const station_recipe_runtime::ActorEvidence& after,
                                      bool actor_unchanged, bool physical_hero_verified) noexcept {
  namespace cat = crafting_catalog;
  if (snapshot.key.kind != cat::Station::saw)
    return;
  const bool metadata =
      actor_unchanged && station_recipe_runtime::same_actor_evidence(before, after) &&
      before.issue == runtime_script::Issue::none && after.issue == runtime_script::Issue::none &&
      before.requires_self && after.requires_self;
  const bool hero = physical_hero_verified && snapshot.key.hero &&
                    before.hero == snapshot.key.hero && after.hero == snapshot.key.hero;
  for (std::size_t i = 0; i < cat::recipes.size(); ++i) {
    const auto& d = cat::recipes[i];
    if (d.route != cat::Route::station || d.station != cat::Station::saw || !d.station_descriptor ||
        d.station_descriptor->material_actor != recipe_catalog::Actor::self)
      continue;
    auto& state = snapshot.recipes[i];
    state.actor_known = state.actor_known && metadata;
    state.actor_matches =
        state.actor_known && hero && state.evaluated && state.generation == snapshot.key.generation;
  }
}

} // namespace ubc
