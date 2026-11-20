// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/spirit_recipe_snapshot.hpp"
#include "ubc/spirit_execution.hpp"
#include "ubc/spirit_flask_debit.hpp"
#include "ubc/crafting_alchemy_extra_reader.hpp"

namespace ubc {
// The native adapter delivers the currently bound observe_info(info)-parameter
// from its real callback lease. The accessor may read only. It re-checks
// Thread/Ticket/Callback depth/Owner/View/Generation and returns 0
// on expired lease. Manager.info is NOT a substitute. The pointer of
// this record must never live beyond the end of the callback lease.
using SpiritActualInfoReader=std::uint32_t(*)(void*,const CraftingAlchemyExtraInput&) noexcept;
struct SpiritCraftingInput {
  CraftingAlchemyExtraInput common{};
  void* info_context=nullptr;
  SpiritActualInfoReader read_actual_info=nullptr;
};
enum class SpiritCraftingIssue {none,thread,reentry,boundary,context,actor,hands,inventory,profile,branch,lease,entry,native};
struct SpiritCraftingDiagnostic {
  SpiritCraftingIssue issue=SpiritCraftingIssue::none;
  alchemy_extra_dependency_reader::Guard dependencies_before{},dependencies_after{};
  alchemy_flask_runtime::NativeEvidence native_before{},native_after{};
  alchemy_flask_runtime::ScriptEvidence flask_before{},flask_after{};
  alchemy_flask_debit::FlaskStack flask{};
  alchemy_extra_dependencies::ListEvidence split{},whole{};
  spirit_execution::Entry entry{};
};
struct SpiritCraftingCapture {
  SpiritRecipeCapture recipe{};
  std::uint64_t serial=0;
  alchemy_flask_debit::Context context_before{},context_after{};
  AlchemyExtraActorPurpose actor_purpose=AlchemyExtraActorPurpose::unknown;
  bool boundary_verified=false,context_unchanged=false,physical_hero_preview_verified=false,actor_verified=false;
  bool native_debit_verified=false,actual_info_verified=false;
  std::uint32_t actual_info=0,maximum_executable=0;
  SpiritCraftingDiagnostic diagnostic{};
};
struct SpiritCraftingQuantity {
  bool valid=false,selectable=false,effect_ready=false;
  std::uint32_t quantity=0,additional_flasks=0;
  std::array<RecipeMaterialRow,spirit_recipe_catalog::target_capacity> rows{};
};
namespace spirit_crafting_detail {
namespace base=crafting_alchemy_extra_detail;
namespace cat=spirit_recipe_catalog;
namespace native=alchemy_flask_runtime;
namespace dependencies=alchemy_extra_dependencies;
inline bool boundary(const CraftingAlchemyExtraInput& in,std::uint32_t owner) noexcept {
  return in.context.parser==lhiver::game_parser&&base::boundary(in,owner);
}
inline base::ContextRead context(const ReadOnlyMemory& memory,const CraftingAlchemyExtraInput& in) noexcept {
  auto value=base::context(memory,in);
  // The physical LAB preview applies only in the own production dialog.
  // The third word (Manager.info) remains only a stability date.
  if(runtime_script::detail::u32(value.dialog.data())!=in.context.hero)value.valid=false;
  return value;
}
inline std::uint32_t executable(std::uint32_t maximum,const alchemy_flask_debit::FlaskStack& flask,
    const dependencies::ListEvidence& split,const dependencies::ListEvidence& whole) noexcept {
  if(maximum<=1)return maximum;
  if(!flask.known||!flask.all_slot_aliases_excluded||!flask.native_item_contract||flask.packed||
     flask.listed_stacks!=1||!flask.object||!flask.amount||flask.amount>INT32_MAX||
     !(flask.flags&0x200000u)||(flask.flags&0x40000000u))return 1;
  return base::executable(maximum,flask,split,whole);
}
// Material planning is required before the actual Info() callback.
// Its upper bound is NOT an effect release: actual_info/Entry
// remain reserved for the later effect_ready- and Writer contract.
inline std::uint32_t material_maximum(const SpiritCraftingCapture& capture) noexcept {
  const auto& s=capture.recipe.state;const auto& d=capture.diagnostic;
  if(!capture.boundary_verified||!capture.context_unchanged||!capture.actor_verified||
     !capture.native_debit_verified||!s.material.known()||!s.known||!s.contract||!s.context_matches)return 0;
  return executable(s.material.maximum,d.flask,d.split,d.whole);
}
inline bool same(const dependencies::ListEvidence& a,const dependencies::ListEvidence& b) noexcept {
  return dependencies::same_list_evidence(a,b);
}
} // namespace spirit_crafting_detail

// Quantity balance only, from our own unchanged N0 snapshot. No new
// runtime, end or ITEM evidence and no substitute for Writer/Result. Both
// output types remain in the seven rows. Only the exact branch receives N.
inline SpiritCraftingQuantity spirit_crafting_quantity(const SpiritCraftingCapture& capture,std::uint32_t quantity) noexcept {
  namespace cat=spirit_recipe_catalog;namespace math=recipe_material_detail;
  SpiritCraftingQuantity out;out.quantity=quantity;const auto& s=capture.recipe.state;
  if(s.id!=cat::RecipeId::spirit||!s.evaluated||!s.branch_known||!s.material.known()||s.material.count!=7||
     s.material.quantity||s.material.additional_flasks||s.material.craftable||quantity>cat::maximum_quantity||
     s.branch!=cat::branch_for(true,s.branch_value)||s.output!=(s.branch_value==1?15741:15739)||
     s.material.output!=s.output||capture.maximum_executable>s.material.maximum)return out;
  std::array<std::int64_t,7> stocks{};
  for(std::size_t i=0;i<7;++i){const auto& row=s.rows[i];const auto reserve=i==4?1:0;
    const auto cost=math::units(cat::view(cat::costs),cat::targets[i].symbol);
    const auto produced=cat::targets[i].symbol==s.output?1u:0u;
    if(row.symbol!=cat::targets[i].symbol||row.unit_cost!=cost||row.unit_output!=produced||
       row.stored<0||row.stored>INT32_MAX||!row.available_known||row.available!=row.stored+reserve||
       row.held!=reserve||row.reserved!=reserve||row.required||row.produced||row.remaining!=row.available||
       row.output_space!=INT32_MAX-row.stored)return out;
    stocks[i]=row.stored;
  }
  if(spirit_debit::maximum(stocks,s.branch_value)!=s.material.maximum)return out;
  out.rows=s.rows;out.additional_flasks=quantity?quantity-1:0;
  for(auto& row:out.rows){row.required=std::int64_t{row.unit_cost}*quantity;row.produced=std::int64_t{row.unit_output}*quantity;
    row.missing=math::positive(row.required-row.available);row.remaining=row.available-row.required+row.produced;
    row.output_shortfall=math::positive(row.produced-row.output_space);}
  out.valid=true;
  out.selectable=quantity&&quantity<=capture.maximum_executable&&capture.boundary_verified&&capture.context_unchanged&&
    capture.actor_verified&&capture.native_debit_verified&&s.known&&s.contract&&s.context_known&&s.context_matches;
  out.effect_ready=out.selectable&&capture.actor_purpose==AlchemyExtraActorPurpose::original_entry&&
    capture.actual_info_verified&&capture.actual_info&&s.actor_matches&&s.owner_conditions_match;
  return out;
}

// Independent record for the real seven-goal profile. A limited inventory
// walk, a full hand walk and their comparison; no heap/engine call.
// Additionally the unchanged positive Native-/Module-/ITEM readers
// and complete list paths for the possible N-1 debit. The physical
// Boundary-/Hand-/End records must originate from the same current
// native lease. This class installs nothing and starts no order.
class SpiritCraftingReader {
public:
  explicit SpiritCraftingReader(std::uint32_t owner) noexcept:owner_(owner),profile_(spirit_recipe_runtime::assemble(spirit_recipe_catalog::recipes[0])),list_(owner){}
  SpiritCraftingReader(const SpiritCraftingReader&)=delete;
  SpiritCraftingReader& operator=(const SpiritCraftingReader&)=delete;
  SpiritCraftingCapture capture(const ReadOnlyMemory& memory,const SpiritCraftingInput& input) noexcept {
    namespace cat=spirit_recipe_catalog;namespace rt=spirit_recipe_runtime;namespace h=station_recipe_snapshot_detail;
    namespace detail=spirit_crafting_detail;namespace dr=alchemy_extra_dependency_reader;namespace native=alchemy_flask_runtime;
    using Issue=SpiritCraftingIssue;
    const auto& in=input.common;SpiritCraftingCapture out;out.serial=in.serial;
    out.recipe.key={in.key.generation,in.key.thread,in.key.world,in.key.hero,in.key.station,in.key.manager,recipe_catalog::Station::alchemy};
    auto& s=out.recipe.state;auto& d=out.recipe.diagnostic;auto& x=out.diagnostic;
    for(std::size_t i=0;i<cat::targets.size();++i){s.rows[i].symbol=cat::targets[i].symbol;s.rows[i].unit_cost=recipe_material_detail::units(cat::view(cat::costs),cat::targets[i].symbol);}
    if(!owner_||in.current_thread!=owner_||in.context.thread!=owner_){x.issue=Issue::thread;return out;}
    if(reading_){x.issue=Issue::reentry;return out;}
    if(!profile_.valid()||!detail::boundary(in,owner_)){x.issue=Issue::boundary;return out;}
    reading_=true;struct Lease{bool& b;~Lease(){b=false;}} lease{reading_};out.boundary_verified=true;
    const bool entry=in.boundary.point==AlchemyExtraReadPoint::before_original;
    out.actor_purpose=entry?AlchemyExtraActorPurpose::original_entry:AlchemyExtraActorPurpose::hero_preview;
    const auto physical_before=detail::context(memory,in);
    if(!physical_before.valid){x.issue=Issue::context;return out;}out.context_before=in.context;s.evaluated=true;
    const auto info_before=entry&&input.read_actual_info?input.read_actual_info(input.info_context,in):0;
    d.actor_before=rt::actor(memory,in.context.parser,profile_,in.context.hero);d.context_before=rt::context(memory,in.context.parser,profile_);
    d.knowledge_before=rt::knowledge(memory,in.context.parser,profile_);d.branch_before=rt::branch(memory,in.context.parser,profile_);
    const dr::Environment environment{in.context,in.modules,in.targets,in.restore_item};
    const auto policy=dr::code_policy(in.modules);
    x.dependencies_before=dr::verify_environment(memory,environment);
    x.native_before=native::verify_native(memory,policy);x.flask_before=native::verify_flask_constructor(memory);
    packed_detail::Header initial{},final{};
    const bool header=packed_detail::at_offset(memory,in.context.hero,packed_detail::header_offset,initial.data(),initial.size());
    auto inventory=read_packed_inventory(memory,in.context.hero,cat::targets,true,inventory_);
    d.hand_issue=h::begin_hands(memory,in.context.hero,hands_);
    d.ingredient_objects_verified=inventory.listed_known&&hands_.captured;
    auto& flask=x.flask;
    if(inventory.storage_totals_known){flask.known=true;flask.packed=inventory.packed[4];flask.listed_stacks=inventory.listed_occurrences[4];}
    std::size_t objects=0;bool flask_objects=true;
    if(inventory.listed_known&&inventory.snapshot_unchanged&&inventory.listed_nodes<=objects_.size()){
      objects=inventory.listed_nodes;
      for(std::size_t i=0;i<objects;++i){const auto& record=inventory_.records[i];auto& fields=fields_[i];fields={};
        fields.object=objects_[i]=runtime_script::detail::u32(record.node.data()+4);
        fields.known=native::detail::word(memory,fields.object,0,fields.type)&&native::detail::word(memory,fields.object,0x158,fields.flags);
        const auto symbol=station_detail::little_i32(record.item_fields.data()+4);
        if(recipe_material_detail::units(cat::view(cat::costs),symbol)){
          bool alias=false;for(std::size_t j=0;j<hands_.count;++j)if(runtime_script::detail::u32(hands_.records[j].fields.data()+0x30)==fields.object)alias=true;
          const bool valid=fields.known&&fields.type==hand_detail::item_vtable&&(fields.flags&0x200000u)&&!(fields.flags&0x40000000u)&&!alias;
          if(!valid)d.ingredient_objects_verified=false;
          if(symbol==16604){flask_objects=flask_objects&&valid;flask.object=fields.object;flask.flags=fields.flags;flask.amount=runtime_script::detail::u32(record.item_fields.data());}
        }
      }
    }
    flask.all_slot_aliases_excluded=flask_objects&&hands_.captured&&inventory.listed_known;
    flask.native_item_contract=flask.all_slot_aliases_excluded;
    const bool list_candidate=flask.known&&!flask.packed&&flask.listed_stacks==1&&flask.object&&flask.amount&&
      flask.native_item_contract&&x.dependencies_before.valid;
    if(list_candidate){x.split=list_.capture(memory,owner_,{in.context.hero,flask.object,1,in.context.world});x.whole=list_.capture(memory,owner_,{in.context.hero,flask.object,flask.amount,in.context.world});}
    d.profile=rt::verify_recipe(memory,in.context.parser,profile_);
    x.dependencies_after=dr::verify_environment(memory,environment);
    x.native_after=native::verify_native(memory,policy);x.flask_after=native::verify_flask_constructor(memory);
    if(list_candidate){const auto split=list_.capture(memory,owner_,{in.context.hero,flask.object,1,in.context.world});
      const auto whole=list_.capture(memory,owner_,{in.context.hero,flask.object,flask.amount,in.context.world});
      if(!detail::same(x.split,split))x.split={};if(!detail::same(x.whole,whole))x.whole={};}
    const auto info_after=entry&&input.read_actual_info?input.read_actual_info(input.info_context,in):0;
    if(entry&&info_before&&info_before==info_after&&d.branch_before.read_valid){
      x.entry=spirit_execution::verify_entry(memory,info_after,d.branch_before.value);
      out.actual_info_verified=x.entry.valid;if(out.actual_info_verified)out.actual_info=info_after;
    }
    d.knowledge_after=rt::knowledge(memory,in.context.parser,profile_);d.branch_after=rt::branch(memory,in.context.parser,profile_);
    d.knowledge_unchanged=rt::same_integer_evidence(d.knowledge_before,d.knowledge_after);
    d.branch_unchanged=rt::same_branch_evidence(d.branch_before,d.branch_after);
    d.actor_after=rt::actor(memory,in.context.parser,profile_,in.context.hero);d.context_after=rt::context(memory,in.context.parser,profile_);
    d.actor_unchanged=rt::same_actor_evidence(d.actor_before,d.actor_after);d.context_unchanged=rt::same_context_evidence(d.context_before,d.context_after);
    d.hands_unchanged=d.hand_issue==StationHandReadIssue::none&&h::verify_hands(memory,hands_);
    if(hands_.captured&&!d.hands_unchanged)d.hand_issue=StationHandReadIssue::changed;
    if(d.hands_unchanged)out.recipe.hands=hands_.before;
    bool metadata=true;for(std::size_t i=0;i<objects;++i){std::uint32_t type=0,flags=0;const auto& f=fields_[i];
      if(!f.known||!native::detail::word(memory,f.object,0,type)||!native::detail::word(memory,f.object,0x158,flags)||type!=f.type||flags!=f.flags)metadata=false;}
    d.inventory_unchanged=header&&inventory.storage_totals_known&&inventory.snapshot_unchanged&&metadata&&
      packed_detail::compare_list(memory,inventory_,inventory.listed_nodes)==PackedInventoryIssue::none&&
      packed_detail::compare_pack(memory,inventory_,runtime_script::detail::u32(initial.data()+28),runtime_script::detail::u32(initial.data()+32))==PackedInventoryIssue::none&&
      packed_detail::at_offset(memory,in.context.hero,packed_detail::header_offset,final.data(),final.size())&&initial==final;
    if(!d.inventory_unchanged){inventory.storage_totals_known=inventory.snapshot_unchanged=false;if(inventory.issue==PackedInventoryIssue::none)inventory.issue=PackedInventoryIssue::changed_during_read;}
    d.inventory_issue=inventory.issue;d.ingredient_objects_verified=d.ingredient_objects_verified&&d.inventory_unchanged&&d.hands_unchanged;
    const auto physical_after=detail::context(memory,in);out.context_unchanged=detail::base::same(physical_before,physical_after);
    if(physical_after.valid)out.context_after=in.context;
    RecipeInventory projected;projected.header_known=inventory.header_known;projected.listed_known=inventory.listed_known;projected.packed_known=inventory.packed_known;
    projected.storage_totals_known=inventory.storage_totals_known;projected.stable_boundary_verified=inventory.stable_boundary_verified;projected.snapshot_unchanged=inventory.snapshot_unchanged;
    projected.issue=inventory.issue;projected.listed_nodes=inventory.listed_nodes;projected.packed_entries=inventory.packed_entries;projected.listed_objects={objects_.data(),objects};
    for(std::size_t i=0;i<7;++i)items_[i]={cat::targets[i].symbol,inventory.listed[i],inventory.packed[i],inventory.total[i],inventory.listed_occurrences[i],inventory.packed_occurrences[i],inventory.duplicate_packed_target[i],inventory.has_active_marker[i],inventory.mixed_representation[i]};
    projected.items={items_.data(),items_.size()};
    s.known=d.knowledge_unchanged&&d.knowledge_before.known&&d.knowledge_after.known;s.contract=d.profile.valid;
    s.branch_known=d.branch_unchanged;if(s.branch_known){s.branch=d.branch_before.branch;s.branch_value=d.branch_before.value;const auto* variant=cat::variant(cat::recipes[0],s.branch);if(variant)s.output=variant->output.symbol;}
    s.actor_known=d.actor_unchanged;s.actor_matches=s.actor_known&&d.actor_before.matches&&d.actor_after.matches;
    s.context_known=d.context_unchanged;s.context_matches=s.context_known&&d.context_before.station_matches()&&d.context_after.station_matches()&&out.context_unchanged;
    s.owner_conditions_match=s.context_known&&d.context_before.owner_matches()&&d.context_after.owner_matches();
    s.material=spirit_recipe_material(cat::recipes[0],projected,out.recipe.hands,s.branch,0,s.rows.data(),s.rows.size(),
      in.boundary.physical_lab_hand_contract_verified&&out.context_unchanged&&d.hands_unchanged,d.ingredient_objects_verified);
    const bool hero_bound=d.actor_unchanged&&d.actor_before.hero==in.context.hero&&d.actor_after.hero==in.context.hero;
    out.physical_hero_preview_verified=!entry&&out.context_unchanged&&hero_bound&&s.material.hand_known;
    out.actor_verified=entry?hero_bound&&s.actor_matches:out.physical_hero_preview_verified;
    const bool dependencies=x.dependencies_before.valid&&x.dependencies_after.valid&&
      alchemy_extra_dependencies::same(x.dependencies_before.item,x.dependencies_after.item)&&
      x.dependencies_before.code.zbug_unpack_call==x.dependencies_after.code.zbug_unpack_call&&
      x.dependencies_before.code.zpe_init_call==x.dependencies_after.code.zpe_init_call;
    out.native_debit_verified=dependencies&&x.native_before.reviewed_ranges_match&&x.native_before.vtable_slots_match&&
      x.native_after.reviewed_ranges_match&&x.native_after.vtable_slots_match&&x.flask_before.constructor_and_bindings_match&&x.flask_before.strings_match&&
      x.flask_after.constructor_and_bindings_match&&x.flask_after.strings_match&&d.ingredient_objects_verified;
    out.maximum_executable=detail::material_maximum(out);
    if(!out.context_unchanged)x.issue=Issue::context;else if(!out.actor_verified)x.issue=Issue::actor;
    else if(!s.material.hand_known)x.issue=Issue::hands;else if(!d.inventory_unchanged)x.issue=Issue::inventory;
    else if(!s.contract)x.issue=Issue::profile;else if(!s.branch_known)x.issue=Issue::branch;
    else if(entry&&(!info_before||info_before!=info_after))x.issue=Issue::lease;
    else if(entry&&!out.actual_info_verified)x.issue=Issue::entry;else if(!out.native_debit_verified)x.issue=Issue::native;
    return out;
  }
private:
  struct ItemFields{std::uint32_t object=0,type=0,flags=0;bool known=false;};
  std::uint32_t owner_=0;bool reading_=false;
  spirit_recipe_runtime::ProfileAssembly profile_{};
  PackedInventoryWorkspace inventory_{};station_recipe_snapshot_detail::HandWorkspace hands_{};
  alchemy_extra_dependencies::ListReader list_;
  std::array<std::uint32_t,max_packed_inventory_nodes> objects_{};
  std::array<ItemFields,max_packed_inventory_nodes> fields_{};
  std::array<RecipeInventoryItem,spirit_recipe_catalog::target_capacity> items_{};
};
} // namespace ubc
