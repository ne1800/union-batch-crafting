// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/stations/grind/grind_item_runtime.hpp"
#include "ubc/stations/grind/grind_recipe_navigation.hpp"

namespace ubc {
struct GrindSharedMaterial {
  bool valid=false;
  GrindRecipeKey key{};
  grind_item_path::Scope scope=grind_item_path::Scope::writer;
  std::uint32_t callback=0,item_path_quantity=0;
  GrindMaterial material{};
};
// Only the native adapter may positively bind the current station/end
// path. input_objects_verified in argument is not used as admission.
inline GrindSharedMaterial bind_grind_material(const GrindRecipeCatalogueCapture& catalogue,
    const grind_item_runtime::Evidence& item,GrindMaterialProofs station) noexcept {
  namespace runtime=grind_item_runtime;namespace path=grind_item_path;
  GrindSharedMaterial out;out.key=catalogue.key;out.scope=item.scope;out.callback=item.callback;
  const auto* recipe=grind_recipe::find(item.callback);const auto& d=catalogue.diagnostic;
  if(!recipe||!runtime::matches(item,catalogue.key,item.callback,1,item.scope)||
    (item.scope!=path::Scope::preview&&item.scope!=path::Scope::writer)||
    d.issue!=GrindCaptureIssue::none||!d.inventory_unchanged||d.inventory_issue!=PackedInventoryIssue::none||
    !catalogue.listed_objects_known||!grind_recipe_snapshot_detail::same_key(catalogue.listed_objects_key,catalogue.key)||
    catalogue.listed_object_count!=d.listed_nodes||catalogue.listed_object_count>catalogue.listed_objects.size()||
    !d.hands_unchanged||d.hand_issue!=StationHandReadIssue::none||d.hand_inventory_alias||
    !grind_item_binding_detail::same_hands(item.stock.input.hands,catalogue.hands))return out;
  const auto knowledge=item.scope==path::Scope::preview?
    grind_preview_knowledge(catalogue,item.physical,catalogue.key):
    grind_writer_knowledge(catalogue,item.writer,catalogue.key);
  if(!knowledge.talent_known||!knowledge.guild_known||!knowledge.skill_known)return out;
  for(const auto& row:item.stock.materials){const RecipeInventoryItem* current=nullptr;
    for(const auto& stock:catalogue.stocks)if(stock.symbol==row.symbol){if(current)return out;current=&stock;}
    if(!current||!path::detail::same_stock(*current,row))return out;
  }
  RecipeInventory inventory;inventory.header_known=inventory.listed_known=inventory.packed_known=true;
  inventory.storage_totals_known=inventory.snapshot_unchanged=true;
  inventory.stable_boundary_verified=station.stable_boundary_verified;
  inventory.listed_nodes=d.listed_nodes;inventory.packed_entries=d.packed_entries;
  inventory.items={item.stock.materials.data(),item.stock.materials.size()};
  // Actual COMPLETE list of the stable scan, including foreign items.
  // No artificially shortened count to three targets and no empty probe.
  inventory.listed_objects={catalogue.listed_objects.data(),catalogue.listed_object_count};
  for(const auto object:{item.stock.input.item.object,item.stock.outputs[0].object,item.stock.outputs[1].object}){
    if(!object)continue;bool found=false;
    for(const auto listed:inventory.listed_objects)if(listed==object)found=true;
    if(!found||object==catalogue.hands.hands.left.object||object==catalogue.hands.hands.right.object)return out;
  }
  station.input_objects_verified=true; // exclusively after matches(full runtime Evidence)
  out.material=grind_recipe_material(*recipe,inventory,catalogue.hands,knowledge,0,station);
  const auto one=grind_recipe::plan(*recipe,out.material.evidence,1);
  if(!out.material.known()||!one.valid)return out;
  for(std::size_t i=0;i<3;++i)
    if(one.after_callback[i].amount!=item.expected_after_callback[i])return out;
  out.item_path_quantity=1;out.valid=true;return out;
}

struct GrindProjectionInput {
  GrindRecipeKey key{};
  grind_item_path::Scope scope=grind_item_path::Scope::writer;
  GrindGuildSnapshot physical{};
  GrindMaterialProofs station{};
  bool physical_hero_preview_verified=false;
  std::array<GrindSharedMaterial,grind_recipe::recipes.size()> materials{};
};

// The raw SELF snapshot remains unchanged. N1 line evidence is not merged
// into an Nmax item path: CraftingSession requires for each chosen quantity
// its own fresh quantity-check and the writer its original SELF binding.
inline CraftingSnapshot project_capture(const GrindRecipeCatalogueCapture& source,
    const GrindProjectionInput& input) noexcept {
  namespace cat=crafting_catalog;namespace rt=grind_recipe_runtime;namespace path=grind_item_path;
  CraftingSnapshot out;const auto& k=source.key;const auto& d=source.diagnostic;
  out.key={k.generation,k.thread,k.world,k.hero,k.station,k.manager,cat::Station::grind};out.hands=source.hands;
  if(!grind_recipe_snapshot_detail::valid_key(k)||!grind_recipe_snapshot_detail::same_key(k,input.key)||
    d.issue!=GrindCaptureIssue::none||d.target_count!=grind_recipe_target_count||
    (input.scope!=path::Scope::preview&&input.scope!=path::Scope::writer)){
    out.issue=CraftingProjectionIssue::context;return out;
  }
  const auto knowledge=grind_preview_knowledge(source,input.physical,k);
  const bool knowledge_known=knowledge.talent_known&&knowledge.guild_known&&knowledge.skill_known;
  const auto& a=d.actor_before;const auto& b=d.actor_after;
  const bool actor_known=d.actor_unchanged&&a.read_valid&&b.read_valid&&a.requires_self&&b.requires_self&&
    a.issue==runtime_script::Issue::none&&b.issue==runtime_script::Issue::none&&
    station_recipe_runtime::same_actor_evidence(a,b);
  const bool actor_matches=actor_known&&a.hero==k.hero&&b.hero==k.hero&&
    (input.scope==path::Scope::preview?input.physical_hero_preview_verified:
      a.matches&&b.matches&&a.self==k.hero&&b.self==k.hero);
  for(std::size_t i=0;i<grind_recipe::recipes.size();++i){
    const auto& raw=source.recipes[i];const auto& recipe=grind_recipe::recipes[i];
    auto& value=out.recipes[cat::grind_begin+i];const auto& bound=input.materials[i];
    if(raw.callback!=recipe.function.symbol||raw.material.quantity||raw.material.craftable){
      out.issue=CraftingProjectionIssue::source;return out;
    }
    if(!raw.evaluated)continue;
    value.evaluated=true;value.generation=k.generation;value.knowledge_read_valid=knowledge_known;
    if(knowledge_known)value.grind={true,knowledge.talent,knowledge.guild,knowledge.skill};
    value.known=knowledge_known&&knowledge.talent==1&&(knowledge.guild==recipe.guilds[0]||knowledge.guild==recipe.guilds[1]);
    const auto& profile=d.profiles[i];
    value.contract=raw.contract&&profile.valid&&profile.local.matches&&profile.local.issue==runtime_script::Issue::none&&
      profile.string_issue==alchemy_flask_runtime::Issue::none&&!profile.failed_symbol;
    value.actor_known=raw.actor_known&&actor_known;value.actor_matches=value.actor_known&&actor_matches;
    value.context_known=raw.state_known&&d.state_unchanged&&rt::same_state(d.state_before,d.state_after);
    value.context_matches=value.context_known&&raw.context_matches&&d.state_before.station_postcondition()&&d.state_after.station_postcondition();
    value.material_known=raw.material.inventory_known&&d.inventory_unchanged&&d.inventory_issue==PackedInventoryIssue::none;
    value.hand_known=d.hands_unchanged&&d.hand_issue==StationHandReadIssue::none&&!d.hand_inventory_alias&&
      input.station.stable_boundary_verified&&input.station.station_hand_end_verified&&
      source.hands.left_tmp_level==input.station.verified_left_tmp_level&&
      crafting_detail::bound_hands(cat::recipes[cat::grind_begin+i],source.hands);
    for(std::size_t j=0;j<3;++j){auto& row=value.materials[j];const RecipeInventoryItem* stock=nullptr;
      for(const auto& entry:source.stocks)if(entry.symbol==row.symbol){if(stock){out.issue=CraftingProjectionIssue::source;return out;}stock=&entry;}
      if(!stock||stock->listed<0||stock->packed<0||stock->total<0||
        std::int64_t{stock->listed}+stock->packed!=stock->total){out.issue=CraftingProjectionIssue::source;return out;}
      if(value.material_known)row.stored=stock->total;
      if(value.hand_known)row.generated_prop=j==2?1:0;
    }
    if(bound.valid){const auto& m=bound.material;
      if(!grind_recipe_snapshot_detail::same_key(bound.key,k)||bound.scope!=input.scope||bound.callback!=recipe.function.symbol||
        bound.item_path_quantity!=1||!m.known()||m.quantity||m.craftable||m.numerical.valid||
        !knowledge_known||!m.evidence.talent_known||!m.evidence.guild_known||!m.evidence.skill_known||
        m.evidence.talent!=knowledge.talent||m.evidence.guild!=knowledge.guild||m.evidence.skill!=knowledge.skill){
        out.issue=CraftingProjectionIssue::source;return out;
      }
      for(std::size_t j=0;j<3;++j){const auto& row=value.materials[j];const auto& original=m.rows[j];
        const auto& stock=m.evidence.stocks[j];
        if(original.symbol!=row.symbol||original.unit_cost!=row.unit_cost||original.unit_output!=row.unit_output||
          original.stored!=row.stored||!original.available_known||original.available!=row.stored||original.reserved||
          original.held!=(j==2?1:0)||stock.symbol!=row.symbol||stock.amount!=row.stored||!stock.known||(!j&&!stock.unambiguous)){
          out.issue=CraftingProjectionIssue::source;return out;}}
      const auto numerical=grind_recipe::plan(recipe,m.evidence,0);
      if(!grind_recipe_navigation::same_amount(numerical,m.numerical)||m.maximum!=(value.known?numerical.maximum:0u)){
        out.issue=CraftingProjectionIssue::source;return out;
      }
      value.material_verified=value.material_known&&value.hand_known&&value.contract&&value.context_matches&&value.actor_matches;
      if(value.material_verified)value.maximum=m.maximum;
    }
  }
  if(!valid_crafting_snapshot(out)||!crafting_detail::stocks_agree(out))out.issue=CraftingProjectionIssue::source;
  return out;
}
} // namespace ubc
