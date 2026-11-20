// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_recipe_snapshot.hpp"

namespace ubc {
enum class CoolActorPurpose {unknown,hero_preview,original_entry};
struct CoolActorBinding {
  CoolActorPurpose purpose=CoolActorPurpose::unknown;
  bool known=false,matches=false;
};
// Only actor projection of own capture values. Inventory/hands are
// already read by CatalogueReader via key.hero. Raw SELF/HERO evidence
// remains unchanged; known foreign SELF blocks only the original entry.
// physical_hero_preview_verified must be proven by the native adapter
// from current world, HERO, station ownership, dialog NPC/player and
// idle boundary. It is not a learn/material/code/end or writer release.
inline CoolActorBinding cool_actor_binding(const CoolRecipeCatalogueCapture& source,
    CoolActorPurpose purpose,bool physical_hero_preview_verified=false) noexcept {
  CoolActorBinding out;out.purpose=purpose;
  if(purpose!=CoolActorPurpose::hero_preview&&purpose!=CoolActorPurpose::original_entry)return out;
  const auto& k=source.key;const auto& s=source.recipe;const auto& d=source.diagnostic;
  const auto& before=d.actor_before;const auto& after=d.actor_after;
  if(!k.generation||!k.thread||!k.world||!k.hero||!k.station||!k.manager||k.production_kind!=13||
    s.id!=cool_recipe_catalog::RecipeId::blade||!s.evaluated||d.issue!=CoolCaptureIssue::none)return out;
  out.known=s.actor_known&&d.actor_unchanged&&
    before.issue==runtime_script::Issue::none&&after.issue==runtime_script::Issue::none&&
    before.requires_self&&after.requires_self&&
    cool_recipe_runtime::same_actor_evidence(before,after);
  if(!out.known||before.hero!=k.hero||after.hero!=k.hero)return out;
  if(purpose==CoolActorPurpose::hero_preview)out.matches=physical_hero_preview_verified;
  else out.matches=s.actor_matches&&before.matches&&after.matches&&
    before.self==k.hero&&after.self==k.hero;
  return out;
}
} // namespace ubc
