// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/anvil_recipe_runtime.hpp"
#include "ubc/station_recipe_snapshot.hpp"

namespace ubc {
// A current native contract, not a flag derived from historical
// world snapshots. station_hand_end_verified means: physical
// SMITHWEAPON/BSANVIL profile, both bound tmp1 slots, and normal
// Destroy/End path are currently positively checked. It allows NO recipe.
struct AnvilCaptureProofs {bool stable_boundary_verified=false,station_hand_end_verified=false;};
struct AnvilRecipeKey {
  std::uint64_t generation=0;
  std::uint32_t thread=0,world=0,hero=0,station=0,manager=0;
  anvil_recipe_catalog::Station kind=anvil_recipe_catalog::Station::anvil;
};

namespace anvil_recipe_snapshot_detail {
namespace cat=anvil_recipe_catalog;
inline StationRecipeHandIssue hands(const cat::Descriptor& recipe,const RecipeInventory& inventory,
    const StationRecipeHands& actual,bool native_proof) noexcept {
  namespace shared=station_recipe_material_detail;using Issue=StationRecipeHandIssue;
  if(!native_proof)return Issue::unverified_contract;
  auto issue=shared::check_hand(recipe.hand.left,actual.hands.left,actual.left_slot_known,actual.left_tmp_level);
  if(issue!=Issue::none)return issue;
  issue=shared::check_hand(recipe.hand.right,actual.hands.right,actual.right_slot_known,actual.right_tmp_level);
  if(issue!=Issue::none)return issue;
  if(actual.left_tmp_level!=recipe.hand.left_tmp_level||actual.right_tmp_level!=recipe.hand.right_tmp_level)
    return Issue::slot_lifetime;
  if(actual.hands.left.object==actual.hands.right.object)return Issue::hand_alias;
  if(!actual.interact_item_known||actual.interact_item!=actual.hands.right.object)return Issue::anchor;
  if(!recipe_material_detail::no_hand_alias(inventory,actual.hands))return Issue::inventory_alias;
  if(!shared::stored_absent(inventory,recipe.hand.right))return Issue::stored_prop;
  // static_lifecycle_verified=false names the missing general animation
  // order. Only the stronger current native_proof together with the
  // right anchor read exactly here fulfills this tight contract.
  return Issue::none;
}
inline bool valid_key(const AnvilRecipeKey& key) noexcept {
  return key.generation&&key.thread&&key.world&&key.hero&&key.station&&key.manager&&key.kind==cat::Station::anvil;
}
constexpr std::size_t target_capacity() noexcept {
  std::size_t count=0;for(const auto& r:cat::recipes)count+=r.targets.count;return count;
}
}

// Same result rows as the previously generated two-hand stations, but own
// canonical descriptor and exactly tmpLevel1. No engine calls. N0 explains
// costs/shortages; output slot limits N instead of rejecting everything.
inline StationRecipeMaterial anvil_recipe_material(const anvil_recipe_catalog::Descriptor& recipe,
    const RecipeInventory& inventory,const StationRecipeHands& observed,std::uint32_t quantity,
    StationRecipeMaterialRow* rows,std::size_t capacity,bool station_hand_end_verified=false) noexcept {
  namespace common=recipe_material_detail;using Issue=StationRecipeMaterialIssue;
  StationRecipeMaterial out;out.quantity=quantity;out.completion=station_recipe_catalog::Completion::end_production;
  if(!anvil_recipe_catalog::structurally_valid(recipe)){out.issue=Issue::descriptor;return out;}
  if(!rows||capacity<recipe.targets.count){out.issue=Issue::output_buffer;return out;}
  out.count=recipe.targets.count;
  for(std::size_t i=0;i<out.count;++i) {
    rows[i]={};rows[i].symbol=recipe.targets[i].symbol;
    rows[i].unit_cost=common::units(recipe.costs,rows[i].symbol);rows[i].unit_output=common::units(recipe.outputs,rows[i].symbol);
  }
  if(!inventory.header_known||!inventory.listed_known||!inventory.packed_known||!inventory.storage_totals_known||
     !inventory.stable_boundary_verified||!inventory.snapshot_unchanged||inventory.issue!=PackedInventoryIssue::none) {
    out.issue=Issue::unknown_inventory;return out;
  }
  if(inventory.listed_nodes>max_packed_inventory_nodes||inventory.packed_entries>max_packed_inventory_entries){out.issue=Issue::invalid_inventory;return out;}
  if(!inventory.items.data||inventory.items.count!=out.count){out.issue=Issue::target_mapping;return out;}
  std::uint64_t listed=0,packed=0;
  for(std::size_t i=0;i<out.count;++i) {
    const auto* item=common::item(inventory,rows[i].symbol);if(!item){out.issue=Issue::target_mapping;return out;}
    if(item->listed<0||item->packed<0||item->total<0||std::int64_t{item->listed}+item->packed!=item->total||
       (item->listed>0&&!item->listed_occurrences)||((item->packed>0)!=(item->packed_occurrences>0))||
       item->listed_occurrences>inventory.listed_nodes||item->packed_occurrences>inventory.packed_entries) {
      out.issue=Issue::invalid_inventory;return out;
    }
    listed+=item->listed_occurrences;packed+=item->packed_occurrences;
  }
  if(listed>inventory.listed_nodes||packed>inventory.packed_entries){out.issue=Issue::invalid_inventory;return out;}
  out.inventory_known=true;
  for(std::size_t i=0;i<out.count;++i) {
    auto& row=rows[i];const auto& item=*common::item(inventory,row.symbol);
    row.stored=row.available=item.total;row.available_known=true;
    if(row.unit_cost&&(item.duplicate_packed_target||item.has_active_marker||item.mixed_representation||
       item.listed_occurrences>1||item.packed_occurrences>1||(item.listed_occurrences&&item.packed_occurrences)))
      out.issue=Issue::ambiguous_ingredient;
  }
  out.hand_issue=anvil_recipe_snapshot_detail::hands(recipe,inventory,observed,station_hand_end_verified);
  out.hand_known=out.hand_issue==StationRecipeHandIssue::none;
  if(!out.hand_known&&out.issue==Issue::none)out.issue=Issue::hand_contract;
  std::uint64_t maximum=INT32_MAX;const bool valid_quantity=quantity<=static_cast<std::uint32_t>(INT32_MAX);
  for(std::size_t i=0;i<out.count;++i) {
    auto& row=rows[i];
    station_recipe_material_detail::copy_hand_amount(recipe.hand.left,out.hand_known,row);
    station_recipe_material_detail::copy_hand_amount(recipe.hand.right,out.hand_known,row);
    row.missing_for_one=common::positive(std::int64_t{row.unit_cost}-row.available);
    row.output_space=INT32_MAX-row.stored;row.output_shortfall_for_one=common::positive(std::int64_t{row.unit_output}-row.output_space);
    if(row.unit_cost)common::limit(maximum,static_cast<std::uint64_t>(row.available)/row.unit_cost);
    if(row.unit_output)common::limit(maximum,static_cast<std::uint64_t>(row.output_space)/row.unit_output);
    if(valid_quantity) {
      row.required=std::int64_t{row.unit_cost}*quantity;row.produced=std::int64_t{row.unit_output}*quantity;
      row.missing=common::positive(row.required-row.available);row.output_shortfall=common::positive(row.produced-row.output_space);
      row.expected_ready_storage=row.expected_end_storage=row.stored-row.required+row.produced;
      row.ready_storage_known=row.end_storage_known=true;row.end_overflow=common::positive(row.expected_end_storage-INT32_MAX);
    }
  }
  if(out.known())out.maximum=static_cast<std::uint32_t>(maximum);
  if(!valid_quantity&&out.issue==Issue::none)out.issue=Issue::quantity;
  out.craftable=out.known()&&quantity&&quantity<=out.maximum;return out;
}

enum class AnvilCaptureIssue {none,context,catalogue,reentry,boundary};
struct AnvilRecipeState {
  anvil_recipe_catalog::RecipeId id=static_cast<anvil_recipe_catalog::RecipeId>(0);
  bool evaluated=false,known=false,contract=false,actor_known=false,actor_matches=false,context_known=false,context_matches=false;
  std::size_t target_count=0;
  StationRecipeMaterial material{};
  std::array<StationRecipeMaterialRow,anvil_recipe_catalog::catalog_target_capacity> rows{};
};
struct AnvilReadDiagnostic {
  anvil_recipe_runtime::AssemblyIssue assembly=anvil_recipe_runtime::AssemblyIssue::descriptor;
  runtime_script::RuntimeContract profile{};anvil_recipe_runtime::Knowledge knowledge{};
};
struct AnvilCaptureDiagnostic {
  AnvilCaptureIssue issue=AnvilCaptureIssue::none;PackedInventoryIssue inventory_issue=PackedInventoryIssue::none;
  StationHandReadIssue hand_issue=StationHandReadIssue::none;
  bool inventory_known=false,hands_unchanged=false,actor_unchanged=false,context_unchanged=false;
  std::size_t target_count=0;std::uint32_t listed_nodes=0,packed_entries=0;
  anvil_recipe_runtime::ActorEvidence actor_before{},actor_after{};
  anvil_recipe_runtime::ContextEvidence context_before{},context_after{};
  std::array<AnvilReadDiagnostic,anvil_recipe_catalog::recipes.size()> recipes{};
};
struct AnvilRecipeCatalogueCapture {
  AnvilRecipeKey key{};
  std::array<AnvilRecipeState,anvil_recipe_catalog::recipes.size()> recipes{};
  StationRecipeHands hands{};AnvilCaptureDiagnostic diagnostic{};
};
inline constexpr std::size_t anvil_recipe_target_capacity=anvil_recipe_snapshot_detail::target_capacity();

// Persistent workspace on the owner thread, without heap/engine call. Before and after
// capture the native adapter must check world/hero/mob/manager/thread/generation.
// Double reads are not protection against foreign async or ABA writes.
class AnvilRecipeCatalogueReader {
public:
  AnvilRecipeCatalogueReader() noexcept {
    namespace cat=anvil_recipe_catalog;
    for(std::size_t i=0;i<cat::recipes.size();++i) {
      const auto& r=cat::recipes[i];profiles_[i]=anvil_recipe_runtime::assemble(r);if(!profiles_[i].valid())return;
      for(const auto& target:r.targets) {
        bool found=false;
        for(std::size_t j=0;j<count_;++j) {
          const bool symbol=target.symbol==targets_[j].symbol,name=std::strcmp(target.name,targets_[j].name)==0;
          if(symbol!=name)return;if(symbol){found=true;break;}
        }
        if(!found){if(count_==targets_.size())return;targets_[count_++]=target;}
      }
    }
    std::array<std::size_t,anvil_recipe_target_capacity> lengths{};
    valid_=count_&&packed_detail::validate_targets(targets_.data(),count_,lengths);
  }
  AnvilRecipeCatalogueReader(const AnvilRecipeCatalogueReader&)=delete;
  AnvilRecipeCatalogueReader& operator=(const AnvilRecipeCatalogueReader&)=delete;
  AnvilRecipeCatalogueReader(AnvilRecipeCatalogueReader&&)=delete;
  AnvilRecipeCatalogueReader& operator=(AnvilRecipeCatalogueReader&&)=delete;
  AnvilRecipeCatalogueCapture capture(const ReadOnlyMemory& memory,std::uint32_t parser,const AnvilRecipeKey& key,
      AnvilCaptureProofs proofs={}) noexcept {
    namespace cat=anvil_recipe_catalog;namespace rt=anvil_recipe_runtime;
    namespace hand=station_recipe_snapshot_detail;
    AnvilRecipeCatalogueCapture out;out.key=key;auto& d=out.diagnostic;d.target_count=count_;
    for(std::size_t i=0;i<cat::recipes.size();++i) {
      const auto& r=cat::recipes[i];auto& s=out.recipes[i];s.id=r.id;s.target_count=r.targets.count;
      d.recipes[i].assembly=profiles_[i].issue();
      for(std::size_t j=0;j<r.targets.count;++j) {
        s.rows[j].symbol=r.targets[j].symbol;s.rows[j].unit_cost=recipe_material_detail::units(r.costs,r.targets[j].symbol);
        s.rows[j].unit_output=recipe_material_detail::units(r.outputs,r.targets[j].symbol);
      }
    }
    if(capturing_){d.issue=AnvilCaptureIssue::reentry;return out;}
    if(!valid_){d.issue=AnvilCaptureIssue::catalogue;return out;}
    if(!anvil_recipe_snapshot_detail::valid_key(key)){d.issue=AnvilCaptureIssue::context;return out;}
    capturing_=true;struct Scope{bool& flag;~Scope(){flag=false;}}scope{capturing_};
    if(!proofs.stable_boundary_verified){d.issue=AnvilCaptureIssue::boundary;return out;}
    d.actor_before=rt::actor(memory,parser,profiles_[0],key.hero);d.context_before=rt::context(memory,parser,profiles_[0]);
    const auto inventory=read_packed_inventory<anvil_recipe_target_capacity>(memory,key.hero,targets_.data(),count_,true,inventory_work_);
    d.inventory_issue=inventory.issue;d.inventory_known=inventory.storage_totals_known;
    d.listed_nodes=inventory.listed_nodes;d.packed_entries=inventory.packed_entries;
    d.hand_issue=hand::begin_hands(memory,key.hero,hand_work_);
    for(std::size_t i=0;i<cat::recipes.size();++i) {
      auto& s=out.recipes[i];auto& diag=d.recipes[i];s.evaluated=true;
      diag.profile=rt::verify(memory,parser,profiles_[i]);diag.knowledge=rt::knowledge(memory,parser,profiles_[i]);
      s.contract=diag.profile.matches;s.known=diag.knowledge.read_valid&&diag.knowledge.known;
    }
    d.actor_after=rt::actor(memory,parser,profiles_[0],key.hero);d.context_after=rt::context(memory,parser,profiles_[0]);
    d.actor_unchanged=rt::same_actor_evidence(d.actor_before,d.actor_after);
    d.context_unchanged=rt::same_context_evidence(d.context_before,d.context_after);
    d.hands_unchanged=hand::verify_hands(memory,hand_work_);
    if(hand_work_.captured&&!d.hands_unchanged)d.hand_issue=StationHandReadIssue::changed;
    if(d.hands_unchanged)out.hands=hand_work_.before;
    std::size_t objects=0;
    if(inventory.listed_known&&inventory.snapshot_unchanged&&inventory.listed_nodes<=listed_objects_.size()) {
      objects=inventory.listed_nodes;
      for(std::size_t i=0;i<objects;++i)listed_objects_[i]=station_detail::little_u32(inventory_work_.records[i].node.data()+4);
    }
    RecipeInventory projected;
    projected.header_known=inventory.header_known;projected.listed_known=inventory.listed_known;projected.packed_known=inventory.packed_known;
    projected.storage_totals_known=inventory.storage_totals_known;projected.stable_boundary_verified=inventory.stable_boundary_verified;
    projected.snapshot_unchanged=inventory.snapshot_unchanged;projected.issue=inventory.issue;
    projected.listed_nodes=inventory.listed_nodes;projected.packed_entries=inventory.packed_entries;
    projected.listed_objects={listed_objects_.data(),objects};
    for(std::size_t i=0;i<cat::recipes.size();++i) {
      const auto& r=cat::recipes[i];auto& s=out.recipes[i];s.actor_known=d.actor_unchanged;
      s.actor_matches=s.actor_known&&d.actor_before.matches&&d.actor_after.matches;
      s.context_known=d.context_unchanged;s.context_matches=s.context_known&&d.context_before.station_matches()&&d.context_after.station_matches();
      bool mapped=true;
      for(std::size_t j=0;j<r.targets.count;++j) {
        std::size_t at=count_;for(std::size_t k=0;k<count_;++k)if(targets_[k].symbol==r.targets[j].symbol){at=k;break;}
        if(at==count_){mapped=false;break;}
        items_[j]={r.targets[j].symbol,inventory.listed[at],inventory.packed[at],inventory.total[at],inventory.listed_occurrences[at],
          inventory.packed_occurrences[at],inventory.duplicate_packed_target[at],inventory.has_active_marker[at],inventory.mixed_representation[at]};
      }
      if(!mapped){s.material.issue=StationRecipeMaterialIssue::target_mapping;continue;}
      projected.items={items_.data(),r.targets.count};
      s.material=anvil_recipe_material(r,projected,out.hands,0,s.rows.data(),s.rows.size(),proofs.station_hand_end_verified&&d.hands_unchanged);
    }
    return out;
  }
private:
  PackedInventoryWorkspace inventory_work_{};station_recipe_snapshot_detail::HandWorkspace hand_work_{};
  std::array<anvil_recipe_runtime::ProfileAssembly,anvil_recipe_catalog::recipes.size()> profiles_{};
  std::array<PackedInventoryTarget,anvil_recipe_target_capacity> targets_{};
  std::array<std::uint32_t,max_packed_inventory_nodes> listed_objects_{};
  std::array<RecipeInventoryItem,anvil_recipe_catalog::catalog_target_capacity> items_{};
  std::size_t count_=0;bool valid_=false,capturing_=false;
};
} // namespace ubc
