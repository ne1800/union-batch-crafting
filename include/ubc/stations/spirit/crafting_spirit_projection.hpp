// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/stations/spirit/spirit_crafting_reader.hpp"
namespace ubc {
// The old base capture does not have these fields. The native adapter
// therefore explicitly delivers the actual pre/post receipt of YOUR intake
// area. Never fabricate this receipt retroactively from Extra/Spirit.
struct CraftingBaseAlchemyEvidence {
  std::uint64_t serial=0;
  bool boundary_verified=false,context_unchanged=false,actor_unchanged=false;
  alchemy_flask_debit::Context context_before{},context_after{};
  std::uint32_t manager_before=0,manager_after=0;
  std::int32_t production_before=-1,production_after=-1;
  station_recipe_runtime::ActorEvidence actor_before{},actor_after{};
};
namespace crafting_spirit_detail {
namespace cat=crafting_catalog;namespace rt=spirit_recipe_runtime;namespace dep=alchemy_extra_dependencies;
inline CraftingKey key(const SpiritCraftingCapture& c) noexcept {
  const auto& k=c.recipe.key;return {k.generation,k.thread,k.world,k.hero,k.station,k.manager,cat::Station::alchemy};
}
inline bool context(const CraftingKey& k,const alchemy_flask_debit::Context& a,const alchemy_flask_debit::Context& b) noexcept {
  return crafting_detail::valid_key(k)&&k.kind==cat::Station::alchemy&&alchemy_flask_debit::valid(a)&&alchemy_flask_debit::valid(b)&&alchemy_flask_debit::same(a,b)&&a.epoch==k.generation&&a.thread==k.thread&&a.world==k.world&&a.hero==k.hero&&a.mob==k.station;
}
inline bool actor(const station_recipe_runtime::ActorEvidence& a,const station_recipe_runtime::ActorEvidence& b,std::uint32_t hero) noexcept {
  return a.issue==runtime_script::Issue::none&&b.issue==runtime_script::Issue::none&&rt::same_actor_evidence(a,b)&&a.requires_self&&a.hero==hero&&a.hero_symbol.offset==a.hero&&a.self_symbol.offset==a.self&&
    a.matches==(a.hero==hero&&a.self==hero)&&b.matches==(b.hero==hero&&b.self==hero);
}
inline bool native(const SpiritCraftingCapture& c) noexcept {
  const auto& d=c.diagnostic;return c.native_debit_verified&&d.dependencies_before.valid&&d.dependencies_after.valid&&
    dep::same(d.dependencies_before.item,d.dependencies_after.item)&&d.dependencies_before.code.zbug_unpack_call==d.dependencies_after.code.zbug_unpack_call&&d.dependencies_before.code.zpe_init_call==d.dependencies_after.code.zpe_init_call&&
    d.native_before.reviewed_ranges_match&&d.native_before.vtable_slots_match&&d.native_after.reviewed_ranges_match&&d.native_after.vtable_slots_match&&
    d.flask_before.constructor_and_bindings_match&&d.flask_before.strings_match&&d.flask_after.constructor_and_bindings_match&&d.flask_after.strings_match&&c.recipe.diagnostic.ingredient_objects_verified;
}
inline bool entry(const SpiritCraftingCapture& c) noexcept {
  const auto& e=c.diagnostic.entry;const auto& d=c.recipe.diagnostic;const auto& s=c.recipe.state;
  return c.actual_info_verified&&c.actual_info&&e.valid&&e.contract.valid&&e.contract.local.matches&&e.knowledge.known&&
    rt::same_integer_evidence(e.knowledge,d.knowledge_after)&&rt::same_branch_evidence(e.branch,d.branch_after)&&rt::same_context_evidence(e.context,d.context_after)&&e.context.owner_matches()&&
    e.code_address&&e.branch.value==s.branch_value&&e.branch.layout.code<=UINT32_MAX-spirit_recipe_catalog::recipes[0].callback->byte_offset&&e.code_address==e.branch.layout.code+spirit_recipe_catalog::recipes[0].callback->byte_offset;
}
inline bool rows(const SpiritCraftingCapture& c,CraftingState& out) noexcept {
  const auto& s=c.recipe.state;const auto& m=s.material;const auto& d=cat::recipes[cat::spirit_begin];
  if(m.count!=7||m.quantity||m.additional_flasks||m.craftable||m.maximum>spirit_recipe_catalog::maximum_quantity||m.branch_known!=s.branch_known||m.output!=s.output)return false;
  const auto outputs=cat::outputs_for(d,out.branch_known,out.branch_value);std::array<std::int64_t,7> stock{};
  for(std::size_t i=0;i<7;++i){const auto& r=s.rows[i];auto& v=out.materials[i];
    const auto cost=cat::units(d.costs,d.targets[i].symbol),produced=cat::units(outputs,d.targets[i].symbol);const bool flask=r.symbol==16604;
    if(r.symbol!=d.targets[i].symbol||r.unit_cost!=cost||r.unit_output!=produced||r.stored< -1||r.stored>INT32_MAX)return false;
    v.unit_output=produced;
    if(m.inventory_known){const auto reserve=flask?(m.hand_known?1:-1):0;const bool available=!flask||m.hand_known;const auto amount=available?r.stored+reserve:-1;
      if(r.stored<0||r.held!=reserve||r.reserved!=reserve||r.available_known!=available||r.available!=amount||r.required||r.produced||r.remaining!=amount||r.missing!=(available?0:-1)||
         r.output_space!=INT32_MAX-r.stored||r.output_shortfall||r.missing_for_one!=(available?recipe_material_detail::positive(std::int64_t{cost}-amount):-1)||
         r.output_shortfall_for_one!=recipe_material_detail::positive(std::int64_t{produced}-(INT32_MAX-r.stored)))return false;
      v.stored=static_cast<std::int32_t>(r.stored);stock[i]=r.stored;
    }else if(r.available_known||r.stored!=-1||r.available!=-1)return false;
    if(flask)v.consumed_reserve=out.hand_known?1:-1;
  }
  if(m.known()){if(spirit_debit::maximum(stock,s.branch_value)!=m.maximum)return false;}
  else if(m.maximum)return false;
  if(c.maximum_executable>m.maximum||c.maximum_executable>spirit_recipe_catalog::maximum_quantity)return false;
  if(c.maximum_executable){
    if(!out.known||!out.contract||!out.actor_matches||!out.context_matches||!out.material_verified||!m.known())return false;
    const auto& flask=c.diagnostic.flask;
    if(c.maximum_executable>1&&(!alchemy_flask_debit::listed_flask(flask,stock[4])||flask.object==c.recipe.hands.hands.right.object))return false;
    if(c.maximum_executable>spirit_crafting_detail::executable(m.maximum,flask,c.diagnostic.split,c.diagnostic.whole))return false;
  }
  out.maximum=c.maximum_executable;return true;
}
inline bool base_evidence(const CraftingKey& k,const CraftingBaseAlchemyEvidence& e) noexcept {
  return e.serial&&e.boundary_verified&&e.context_unchanged&&e.actor_unchanged&&context(k,e.context_before,e.context_after)&&e.manager_before==k.manager&&e.manager_after==k.manager&&e.production_before==4&&e.production_after==4&&actor(e.actor_before,e.actor_after,k.hero);
}
}

// Only own value copies. Missing knowledge/profile/material remains
// separate and may stay visible as a locked recipe. Contradictory positive
// fields reject admission; no Native-/Writer proof is created here.
inline CraftingSnapshot project_capture(const SpiritCraftingCapture& source) noexcept {
  namespace detail=crafting_spirit_detail;namespace rt=spirit_recipe_runtime;namespace cat=crafting_catalog;
  CraftingSnapshot result;result.key=detail::key(source);const auto& k=result.key;const auto& s=source.recipe.state;const auto& d=source.recipe.diagnostic;auto& out=result.recipes[cat::spirit_begin];
  const auto bad=[&](CraftingProjectionIssue issue){result.issue=issue;return result;};
  if(source.recipe.key.kind!=recipe_catalog::Station::alchemy||!source.serial||!source.boundary_verified||!source.context_unchanged||!detail::context(k,source.context_before,source.context_after))return bad(CraftingProjectionIssue::context);
  if(d.issue!=SpiritCaptureIssue::none||s.id!=spirit_recipe_catalog::RecipeId::spirit||!s.evaluated||d.target_count!=7)return bad(CraftingProjectionIssue::source);
  out.generation=k.generation;out.evaluated=true;
  const bool knowledge=d.knowledge_unchanged&&d.knowledge_before.issue==runtime_script::Issue::none&&d.knowledge_after.issue==runtime_script::Issue::none&&rt::same_integer_evidence(d.knowledge_before,d.knowledge_after);
  out.knowledge_read_valid=knowledge;out.known=knowledge&&d.knowledge_before.known&&d.knowledge_after.known&&d.knowledge_before.readings[0].value==1;
  if(s.known!=out.known)return bad(CraftingProjectionIssue::source);
  const bool branch=d.branch_unchanged&&d.branch_before.issue==runtime_script::Issue::none&&d.branch_after.issue==runtime_script::Issue::none&&rt::same_branch_evidence(d.branch_before,d.branch_after);
  if(branch){out.branch_known=true;out.branch_value=d.branch_before.value;out.branch_output=out.branch_value==1?15741:15739;
    if(d.branch_before.readings[0].value!=out.branch_value||d.branch_before.branch!=spirit_recipe_catalog::branch_for(true,out.branch_value))return bad(CraftingProjectionIssue::source);}
  if(s.branch_known!=branch||s.branch_value!=out.branch_value||s.output!=out.branch_output||s.branch!=spirit_recipe_catalog::branch_for(branch,out.branch_value))return bad(CraftingProjectionIssue::source);
  const bool profile=d.profile.valid&&d.profile.local.matches&&d.profile.local.issue==runtime_script::Issue::none;
  if(s.contract!=profile||(source.native_debit_verified&&!detail::native(source)))return bad(CraftingProjectionIssue::source);out.contract=profile&&detail::native(source);
  out.actor_known=d.actor_unchanged&&detail::actor(d.actor_before,d.actor_after,k.hero);
  if(s.actor_known!=out.actor_known||s.actor_matches!=(out.actor_known&&d.actor_before.matches&&d.actor_after.matches))return bad(CraftingProjectionIssue::source);
  const bool preview=source.actor_purpose==AlchemyExtraActorPurpose::hero_preview;
  const bool original=source.actor_purpose==AlchemyExtraActorPurpose::original_entry;
  if(!preview&&!original)return bad(CraftingProjectionIssue::source);
  out.actor_matches=out.actor_known&&((preview&&source.physical_hero_preview_verified)||(original&&s.actor_matches));
  // ActorVerified in Reader binds the actor at Entry; the Lease is separate.
  const bool actor_claim=out.actor_known&&(preview?source.physical_hero_preview_verified:s.actor_matches);
  if(source.actor_verified!=actor_claim||(original&&source.physical_hero_preview_verified))return bad(CraftingProjectionIssue::source);
  if(source.actual_info_verified!=(source.actual_info!=0))return bad(CraftingProjectionIssue::source);
  if(original&&source.actual_info_verified&&!detail::entry(source))return bad(CraftingProjectionIssue::source);
  if(preview&&(source.actual_info_verified||source.actual_info))return bad(CraftingProjectionIssue::source);
  out.context_known=d.context_unchanged&&d.context_before.issue==runtime_script::Issue::none&&d.context_after.issue==runtime_script::Issue::none&&rt::same_context_evidence(d.context_before,d.context_after);
  out.context_matches=out.context_known&&d.context_before.station_matches()&&d.context_after.station_matches();
  if(s.context_known!=out.context_known||s.context_matches!=out.context_matches||s.owner_conditions_match!=(out.context_known&&d.context_before.owner_matches()&&d.context_after.owner_matches()))return bad(CraftingProjectionIssue::source);
  if(out.context_known&&(d.context_before.readings[0].value!=d.context_before.production||d.context_before.readings[1].value!=d.context_before.special))return bad(CraftingProjectionIssue::source);
  const bool hand=d.hands_unchanged&&d.hand_issue==StationHandReadIssue::none;
  if(hand)result.hands=source.recipe.hands;
  out.material_known=branch&&s.material.inventory_known&&d.inventory_unchanged&&d.inventory_issue==PackedInventoryIssue::none;
  out.hand_known=s.material.hand_known&&hand&&crafting_detail::bound_hands(cat::recipes[cat::spirit_begin],result.hands)&&result.hands.left_tmp_level>=0&&result.hands.right_tmp_level>=0;
  if(s.material.hand_known&&!out.hand_known)return bad(CraftingProjectionIssue::source);
  out.material_verified=out.material_known&&out.hand_known&&s.material.known()&&d.ingredient_objects_verified;
  if(!detail::rows(source,out)||!valid_crafting_snapshot(result))return bad(CraftingProjectionIssue::source);
  return result;
}

inline CraftingSnapshot project_capture(const RecipeCatalogueCapture& base,const CraftingBaseAlchemyEvidence& evidence,
    const AlchemyExtraCatalogueCapture& extra,const SpiritCraftingCapture& spirit) noexcept {
  namespace detail=crafting_spirit_detail;namespace cat=crafting_catalog;namespace rt=spirit_recipe_runtime;
  auto first=project_capture(base,extra);const auto second=project_capture(spirit);
  const auto fail=[&](CraftingProjectionIssue issue){first.issue=issue;return first;};
  const auto& k=first.key;const auto& sd=spirit.recipe.diagnostic;
  if(!valid_crafting_snapshot(first)||!valid_crafting_snapshot(second)||!(k==second.key)||!detail::base_evidence(k,evidence)||evidence.serial!=extra.serial||extra.serial!=spirit.serial||
     !alchemy_flask_debit::same(evidence.context_before,extra.context_before)||!alchemy_flask_debit::same(extra.context_before,spirit.context_before)||
     !alchemy_flask_debit::same(evidence.context_after,extra.context_after)||!alchemy_flask_debit::same(extra.context_after,spirit.context_after)||
     extra.production_before!=evidence.production_before||extra.production_after!=evidence.production_after||extra.production_before!=sd.context_before.production||extra.production_after!=sd.context_after.production||
     !extra.actor_unchanged||!detail::actor(extra.actor_before,extra.actor_after,k.hero)||
     !rt::same_actor_evidence(evidence.actor_before,extra.actor_before)||!rt::same_actor_evidence(evidence.actor_after,extra.actor_after)||
     !rt::same_actor_evidence(extra.actor_before,sd.actor_before)||!rt::same_actor_evidence(extra.actor_after,sd.actor_after)||extra.actor_purpose!=spirit.actor_purpose||
     extra.physical_hero_preview_verified!=spirit.physical_hero_preview_verified)return fail(CraftingProjectionIssue::context);
  if(!same_crafting_hands(first.hands,second.hands))return fail(CraftingProjectionIssue::source);
  // Compare all numerically shared stored inventory values, even with
  // unknown branch or locked profile. Unknown is not a positive equal proof.
  const auto& own=second.recipes[cat::spirit_begin];
  for(const auto& state:first.recipes)if(state.evaluated&&state.id!=own.id)
    for(std::size_t i=0;i<state.target_count;++i)for(std::size_t j=0;j<own.target_count;++j)if(state.materials[i].symbol==own.materials[j].symbol)
      if(state.materials[i].stored<0||own.materials[j].stored<0||state.materials[i].stored!=own.materials[j].stored||state.materials[i].consumed_reserve!=own.materials[j].consumed_reserve)return fail(CraftingProjectionIssue::source);
  return crafting_detail::merge(first,second,cat::Route::spirit,false);
}
} // namespace ubc
