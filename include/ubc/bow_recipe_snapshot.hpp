// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/bow_recipe_runtime.hpp"
#include "ubc/station_recipe_snapshot.hpp"

namespace ubc {
struct BowMaterialRow:RecipeMaterialRow {
  std::uint32_t minimum_per_unit=0;
  std::int64_t original_minimum=-1,minimum_missing=-1,minimum_missing_for_one=-1;
};
enum class BowMaterialIssue {none,descriptor,output_buffer,unknown_inventory,invalid_inventory,target_mapping,
  ambiguous_ingredient,hand_contract,hand_object,hand_alias,ingredient_objects,quantity};
struct BowMaterial {
  BowMaterialIssue issue=BowMaterialIssue::none;
  bool inventory_known=false,hand_known=false,ingredients_verified=false,craftable=false;
  std::uint32_t maximum=0,quantity=0;std::size_t count=0;
  bool known() const noexcept {return issue==BowMaterialIssue::none&&inventory_known&&hand_known&&ingredients_verified;}
};
// Original minimum and actual consumption remain separate. In particular,
// Callback20930 requires Steel12/Leather10 and debits Steel10/Leather12.
// The quantity adapters receive both operands; no silent swap/cost bugfix.
inline BowMaterial bow_recipe_material(const bow_recipe_catalog::Descriptor& descriptor,const RecipeInventory& inventory,
    const StationRecipeHands& hands,std::uint32_t quantity,BowMaterialRow* rows,std::size_t capacity,
    bool station_hand_end_verified=false,bool listed_ingredient_contract_verified=false) noexcept {
  namespace cat=bow_recipe_catalog;namespace math=recipe_material_detail;using Issue=BowMaterialIssue;
  BowMaterial out;out.quantity=quantity;if(!cat::structurally_valid(descriptor)){out.issue=Issue::descriptor;return out;}
  if(!rows||capacity<descriptor.targets.count){out.issue=Issue::output_buffer;return out;}
  const auto& d=*cat::find(descriptor.id);out.count=d.targets.count;
  for(std::size_t i=0;i<out.count;++i){auto& row=rows[i];row={};row.symbol=d.targets[i].symbol;row.unit_cost=cat::units(d.costs,row.symbol);
    row.unit_output=cat::units(d.outputs,row.symbol);row.minimum_per_unit=cat::units(d.minima,row.symbol);}
  if(!inventory.header_known||!inventory.listed_known||!inventory.packed_known||!inventory.storage_totals_known||!inventory.stable_boundary_verified||
    !inventory.snapshot_unchanged||inventory.issue!=PackedInventoryIssue::none){out.issue=Issue::unknown_inventory;return out;}
  if(inventory.listed_nodes>max_packed_inventory_nodes||inventory.packed_entries>max_packed_inventory_entries){out.issue=Issue::invalid_inventory;return out;}
  if(inventory.items.count!=out.count||!inventory.items.data){out.issue=Issue::target_mapping;return out;}
  std::uint64_t listed=0,packed=0;
  for(std::size_t i=0;i<out.count;++i){auto& row=rows[i];const auto* item=math::item(inventory,row.symbol);if(!item){out.issue=Issue::target_mapping;return out;}
    if(item->listed<0||item->packed<0||item->total<0||std::int64_t{item->listed}+item->packed!=item->total||
      (item->listed>0&&!item->listed_occurrences)||((item->packed>0)!=(item->packed_occurrences>0))||
      item->listed_occurrences>inventory.listed_nodes||item->packed_occurrences>inventory.packed_entries){out.issue=Issue::invalid_inventory;return out;}
    listed+=item->listed_occurrences;packed+=item->packed_occurrences;row.stored=row.available=item->total;row.available_known=true;
    if((row.unit_cost||row.minimum_per_unit)&&(item->listed_occurrences>1||item->packed_occurrences>1||item->duplicate_packed_target||item->has_active_marker||
      item->mixed_representation||(item->listed_occurrences&&item->packed_occurrences)))out.issue=Issue::ambiguous_ingredient;
  }
  if(listed>inventory.listed_nodes||packed>inventory.packed_entries){out.issue=Issue::invalid_inventory;return out;}
  out.inventory_known=true;auto hand_issue=Issue::none;
  if(!station_hand_end_verified||!hands.left_slot_known||!hands.right_slot_known||!hands.interact_item_known)hand_issue=Issue::hand_contract;
  else if(hands.hands.left.state!=0||hands.hands.right.state!=0||hands.hands.left.object||hands.hands.right.object||hands.interact_item)hand_issue=Issue::hand_object;
  else if(!math::no_hand_alias(inventory,hands.hands))hand_issue=Issue::hand_alias;
  else out.hand_known=true;
  if(out.issue==Issue::none)out.issue=hand_issue;
  out.ingredients_verified=listed_ingredient_contract_verified;if(out.issue==Issue::none&&!out.ingredients_verified)out.issue=Issue::ingredient_objects;
  std::uint64_t maximum=d.callback.maximum_quantity;const bool representable=quantity<=d.callback.maximum_quantity;
  for(std::size_t i=0;i<out.count;++i){auto& row=rows[i];
    row.missing_for_one=math::positive(std::int64_t{row.unit_cost}-row.available);row.minimum_missing_for_one=math::positive(std::int64_t{row.minimum_per_unit}-row.available);
    row.output_space=INT32_MAX-row.stored;row.output_shortfall_for_one=math::positive(std::int64_t{row.unit_output}-row.output_space);
    if(row.unit_cost)math::limit(maximum,static_cast<std::uint64_t>(row.available)/row.unit_cost);
    if(row.minimum_per_unit)math::limit(maximum,static_cast<std::uint64_t>(row.available)/row.minimum_per_unit);
    if(row.unit_output)math::limit(maximum,static_cast<std::uint64_t>(row.output_space)/row.unit_output);
    if(representable){row.required=std::int64_t{row.unit_cost}*quantity;row.produced=std::int64_t{row.unit_output}*quantity;
      row.original_minimum=std::int64_t{row.minimum_per_unit}*quantity;row.minimum_missing=math::positive(row.original_minimum-row.available);
      row.missing=math::positive(row.required-row.available);row.remaining=row.available-row.required+row.produced;row.output_shortfall=math::positive(row.produced-row.output_space);}
  }
  if(out.known())out.maximum=static_cast<std::uint32_t>(maximum);if(!representable&&out.issue==Issue::none)out.issue=Issue::quantity;
  out.craftable=out.known()&&quantity&&quantity<=out.maximum;return out;
}

struct BowRecipeKey {std::uint64_t generation=0;std::uint32_t thread=0,world=0,hero=0,station=0,manager=0;std::int32_t production_kind=16;};
struct BowCaptureProofs {bool stable_boundary_verified=false,station_hand_end_verified=false;};
enum class BowCaptureIssue {none,context,boundary,reentry,catalogue};
struct BowRecipeState {
  bow_recipe_catalog::RecipeId id=static_cast<bow_recipe_catalog::RecipeId>(0);
  bool evaluated=false,known=false,contract=false,actor_known=false,actor_matches=false,context_known=false,context_matches=false,owner_conditions_match=false;
  bool ingredient_objects_verified=false;
  BowMaterial material{};std::array<BowMaterialRow,bow_recipe_catalog::catalog_target_capacity> rows{};
};
struct BowCaptureDiagnostic {
  BowCaptureIssue issue=BowCaptureIssue::none;PackedInventoryIssue inventory_issue=PackedInventoryIssue::none;StationHandReadIssue hand_issue=StationHandReadIssue::none;
  bool inventory_unchanged=false,hands_unchanged=false,actor_unchanged=false,context_unchanged=false,knowledge_unchanged=false;
  std::size_t target_count=0;bow_recipe_runtime::Knowledge knowledge_before{},knowledge_after{};
  bow_recipe_runtime::ActorEvidence actor_before{},actor_after{};bow_recipe_runtime::ContextEvidence context_before{},context_after{};
  std::array<bow_recipe_runtime::Verification,bow_recipe_catalog::recipes.size()> profiles{};
};
struct BowRecipeCatalogueCapture {
  BowRecipeKey key{};std::array<BowRecipeState,bow_recipe_catalog::recipes.size()> recipes{};StationRecipeHands hands{};BowCaptureDiagnostic diagnostic{};
};
namespace bow_recipe_snapshot_detail {
constexpr std::size_t target_capacity() noexcept {std::size_t n=0;for(const auto& r:bow_recipe_catalog::recipes)n+=r.targets.count;return n;}
inline bool item_class(const ReadOnlyMemory& memory) noexcept {
  std::uint32_t method=0;std::array<std::uint8_t,hand_detail::item_class_bytes.size()> bytes{};
  return alchemy_flask_runtime::detail::word(memory,hand_detail::item_vtable,0,method)&&method==hand_detail::item_class_method&&
    memory.copy(method,bytes.data(),bytes.size())&&bytes==hand_detail::item_class_bytes;
}
}
inline constexpr std::size_t bow_recipe_target_capacity=bow_recipe_snapshot_detail::target_capacity();

// No native release solely from this reader: station_hand_end_verified
// is a current bound MAKEBOWS-/normal-end proof to the same key/frame.
// New physical station and item/remove/createclosure must be separately
// positively checked before root integration. No active unequip.
class BowRecipeCatalogueReader {
public:
  BowRecipeCatalogueReader() noexcept {
    namespace cat=bow_recipe_catalog;
    for(std::size_t i=0;i<cat::recipes.size();++i){const auto& r=cat::recipes[i];profiles_[i]=bow_recipe_runtime::assemble(r);if(!profiles_[i].valid())return;
      for(const auto& t:r.targets){bool found=false;for(std::size_t j=0;j<count_;++j){const bool symbol=t.symbol==targets_[j].symbol,name=std::strcmp(t.name,targets_[j].name)==0;
        if(symbol!=name)return;if(symbol){found=true;break;}}
        if(!found){if(count_==targets_.size())return;targets_[count_++]=t;}}
    }
    std::array<std::size_t,bow_recipe_target_capacity> lengths{};valid_=count_&&packed_detail::validate_targets(targets_.data(),count_,lengths);
  }
  BowRecipeCatalogueReader(const BowRecipeCatalogueReader&)=delete;BowRecipeCatalogueReader& operator=(const BowRecipeCatalogueReader&)=delete;
  BowRecipeCatalogueCapture capture(const ReadOnlyMemory& memory,std::uint32_t parser,const BowRecipeKey& key,BowCaptureProofs proof={}) noexcept {
    namespace cat=bow_recipe_catalog;namespace rt=bow_recipe_runtime;namespace hand=station_recipe_snapshot_detail;
    BowRecipeCatalogueCapture out;out.key=key;auto& d=out.diagnostic;d.target_count=count_;
    for(std::size_t i=0;i<cat::recipes.size();++i){const auto& r=cat::recipes[i];auto& s=out.recipes[i];s.id=r.id;
      for(std::size_t j=0;j<r.targets.count;++j){auto& row=s.rows[j];row.symbol=r.targets[j].symbol;row.unit_cost=cat::units(r.costs,row.symbol);row.unit_output=cat::units(r.outputs,row.symbol);row.minimum_per_unit=cat::units(r.minima,row.symbol);}}
    if(reading_){d.issue=BowCaptureIssue::reentry;return out;}if(!valid_){d.issue=BowCaptureIssue::catalogue;return out;}
    if(!key.generation||!key.thread||!key.world||!key.hero||!key.station||!key.manager||key.production_kind!=16){d.issue=BowCaptureIssue::context;return out;}
    if(!proof.stable_boundary_verified){d.issue=BowCaptureIssue::boundary;return out;}
    reading_=true;struct Lease{bool& value;~Lease(){value=false;}}lease{reading_};
    d.actor_before=rt::actor(memory,parser,profiles_[0],key.hero);d.context_before=rt::context(memory,parser,profiles_[0]);d.knowledge_before=rt::knowledge(memory,parser,profiles_[0]);
    packed_detail::Header initial{},final{};const bool header=packed_detail::at_offset(memory,key.hero,packed_detail::header_offset,initial.data(),initial.size());
    auto inventory=read_packed_inventory<bow_recipe_target_capacity>(memory,key.hero,targets_.data(),count_,true,inventory_);
    d.hand_issue=hand::begin_hands(memory,key.hero,hands_);
    std::array<bool,bow_recipe_target_capacity> items_valid{};items_valid.fill(true);std::size_t objects=0;
    const bool class_verified=!inventory.listed_nodes||bow_recipe_snapshot_detail::item_class(memory);
    if(inventory.listed_known&&inventory.snapshot_unchanged&&inventory.listed_nodes<=objects_.size()){
      objects=inventory.listed_nodes;
      for(std::size_t i=0;i<objects;++i){const auto& record=inventory_.records[i];auto& field=fields_[i];field={};field.object=objects_[i]=runtime_script::detail::u32(record.node.data()+4);
        field.known=alchemy_flask_runtime::detail::word(memory,field.object,0,field.type)&&alchemy_flask_runtime::detail::word(memory,field.object,0x154,field.mainflag)&&alchemy_flask_runtime::detail::word(memory,field.object,0x158,field.flags);
        const auto symbol=station_detail::little_i32(record.item_fields.data()+4);bool alias=false;
        for(std::size_t j=0;j<hands_.count;++j)if(runtime_script::detail::u32(hands_.records[j].fields.data()+0x30)==field.object)alias=true;
        const cat::ItemShape* expected=nullptr;for(const auto& r:cat::recipes)for(const auto& item:r.items)if(item.symbol==symbol)expected=&item;
        // InitByScript at 711CA0..711CBC combines MainFlag into flags with bitwise OR.
        // MultiSlot at 7125A0 accepts inactive category 4 weapons without ITEM_MULTI.
        // This narrow contract binds the exact canonical flags and all slot aliases.
        const bool matches=class_verified&&field.known&&expected&&field.type==hand_detail::item_vtable&&field.mainflag==expected->mainflag&&field.flags==expected->native_flags&&!(field.flags&0x40000000u)&&!alias&&hands_.captured;
        for(std::size_t j=0;j<count_;++j)if(targets_[j].symbol==symbol)items_valid[j]=items_valid[j]&&matches;
      }
    }
    for(std::size_t i=0;i<cat::recipes.size();++i){out.recipes[i].evaluated=true;d.profiles[i]=rt::verify_recipe(memory,parser,cat::recipes[i],profiles_[i]);}
    d.knowledge_after=rt::knowledge(memory,parser,profiles_[0]);d.knowledge_unchanged=d.knowledge_before.read_valid&&d.knowledge_after.read_valid&&d.knowledge_before.known==d.knowledge_after.known;
    d.actor_after=rt::actor(memory,parser,profiles_[0],key.hero);d.context_after=rt::context(memory,parser,profiles_[0]);
    d.actor_unchanged=rt::same_actor_evidence(d.actor_before,d.actor_after);d.context_unchanged=rt::same_context_evidence(d.context_before,d.context_after);
    d.hands_unchanged=d.hand_issue==StationHandReadIssue::none&&hand::verify_hands(memory,hands_);if(hands_.captured&&!d.hands_unchanged)d.hand_issue=StationHandReadIssue::changed;if(d.hands_unchanged)out.hands=hands_.before;
    bool metadata=true;for(std::size_t i=0;i<objects;++i){std::uint32_t type=0,mainflag=0,flags=0;const auto& f=fields_[i];
      if(!f.known||!alchemy_flask_runtime::detail::word(memory,f.object,0,type)||!alchemy_flask_runtime::detail::word(memory,f.object,0x154,mainflag)||!alchemy_flask_runtime::detail::word(memory,f.object,0x158,flags)||type!=f.type||mainflag!=f.mainflag||flags!=f.flags)metadata=false;}
    d.inventory_unchanged=header&&inventory.storage_totals_known&&inventory.snapshot_unchanged&&metadata&&(!inventory.listed_nodes||bow_recipe_snapshot_detail::item_class(memory))&&
      packed_detail::compare_list(memory,inventory_,inventory.listed_nodes)==PackedInventoryIssue::none&&
      packed_detail::compare_pack(memory,inventory_,runtime_script::detail::u32(initial.data()+28),runtime_script::detail::u32(initial.data()+32))==PackedInventoryIssue::none&&
      packed_detail::at_offset(memory,key.hero,packed_detail::header_offset,final.data(),final.size())&&initial==final;
    if(!d.inventory_unchanged){inventory.storage_totals_known=inventory.snapshot_unchanged=false;if(inventory.issue==PackedInventoryIssue::none)inventory.issue=PackedInventoryIssue::changed_during_read;}d.inventory_issue=inventory.issue;
    RecipeInventory projected;projected.header_known=inventory.header_known;projected.listed_known=inventory.listed_known;projected.packed_known=inventory.packed_known;
    projected.storage_totals_known=inventory.storage_totals_known;projected.stable_boundary_verified=inventory.stable_boundary_verified;projected.snapshot_unchanged=inventory.snapshot_unchanged;
    projected.issue=inventory.issue;projected.listed_nodes=inventory.listed_nodes;projected.packed_entries=inventory.packed_entries;projected.listed_objects={objects_.data(),objects};
    for(std::size_t i=0;i<cat::recipes.size();++i){const auto& r=cat::recipes[i];auto& s=out.recipes[i];s.contract=d.profiles[i].valid;
      s.known=d.knowledge_unchanged&&d.knowledge_before.known&&d.knowledge_after.known;s.actor_known=d.actor_unchanged;s.actor_matches=s.actor_known&&d.actor_before.matches&&d.actor_after.matches;
      s.context_known=d.context_unchanged;s.context_matches=s.context_known&&d.context_before.station_matches()&&d.context_after.station_matches();
      s.owner_conditions_match=s.context_known&&d.context_before.owner_matches()&&d.context_after.owner_matches();
      s.ingredient_objects_verified=d.inventory_unchanged&&d.hands_unchanged&&class_verified;
      bool mapped=true;for(std::size_t j=0;j<r.targets.count;++j){std::size_t index=0;while(index<count_&&targets_[index].symbol!=r.targets[j].symbol)++index;if(index==count_){mapped=false;break;}
        items_[j]={r.targets[j].symbol,inventory.listed[index],inventory.packed[index],inventory.total[index],inventory.listed_occurrences[index],inventory.packed_occurrences[index],inventory.duplicate_packed_target[index],inventory.has_active_marker[index],inventory.mixed_representation[index]};
        if(cat::units(r.costs,r.targets[j].symbol)||cat::units(r.minima,r.targets[j].symbol))s.ingredient_objects_verified=s.ingredient_objects_verified&&items_valid[index];}
      if(!mapped){s.material.issue=BowMaterialIssue::target_mapping;continue;}
      projected.items={items_.data(),r.targets.count};s.material=bow_recipe_material(r,projected,out.hands,0,s.rows.data(),s.rows.size(),proof.station_hand_end_verified&&d.hands_unchanged,s.ingredient_objects_verified);
    }
    return out;
  }
private:
  struct ItemFields {std::uint32_t object=0,type=0,mainflag=0,flags=0;bool known=false;};
  std::array<bow_recipe_runtime::ProfileAssembly,bow_recipe_catalog::recipes.size()> profiles_{};
  std::array<PackedInventoryTarget,bow_recipe_target_capacity> targets_{};std::array<RecipeInventoryItem,bow_recipe_catalog::catalog_target_capacity> items_{};
  PackedInventoryWorkspace inventory_{};station_recipe_snapshot_detail::HandWorkspace hands_{};
  std::array<std::uint32_t,max_packed_inventory_nodes> objects_{};std::array<ItemFields,max_packed_inventory_nodes> fields_{};
  std::size_t count_=0;bool valid_=false,reading_=false;
};
} // namespace ubc
