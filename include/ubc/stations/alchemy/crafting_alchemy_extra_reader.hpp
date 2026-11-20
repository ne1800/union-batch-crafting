// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/stations/alchemy/alchemy_extra_execution.hpp"
#include "ubc/stations/alchemy/alchemy_extra_dependency_reader.hpp"

namespace ubc {
// This boundary comes from the real native caller. Here neither a writer/job
// scope is created nor is anything derived from two identical memory regions.
enum class AlchemyExtraReadPoint { unknown,post_render,before_original,selection_resolved };
struct AlchemyExtraReadBoundary {
  AlchemyExtraReadPoint point=AlchemyExtraReadPoint::unknown;
  bool verified=false,physical_lab_hand_contract_verified=false;
  std::uint32_t callback_depth=0,vm_depth=0,writer_depth=0,debit_depth=0;
  std::uint32_t resolve_depth=0;
};
struct CraftingAlchemyExtraInput {
  CraftingKey key{};
  alchemy_flask_debit::Context context{};
  std::uint64_t serial=0;
  std::uint32_t current_thread=0;
  AlchemyExtraReadBoundary boundary{};
  alchemy_extra_dependencies::Modules modules{};
  alchemy_extra_dependencies::Targets targets{};
  std::uint32_t restore_item=0;
};
enum class CraftingAlchemyExtraReadIssue {none,thread,reentry,boundary,context,catalogue,inventory,hands,actor,changed,native};
struct CraftingAlchemyExtraReadDiagnostic {
  CraftingAlchemyExtraReadIssue issue=CraftingAlchemyExtraReadIssue::none;
  PackedInventoryIssue inventory_issue=PackedInventoryIssue::none;
  StationHandReadIssue hand_issue=StationHandReadIssue::none;
  alchemy_extra_dependency_reader::Guard dependencies{};
  alchemy_flask_runtime::NativeEvidence native{};
  alchemy_flask_runtime::ScriptEvidence flask_constructor{};
  alchemy_extra_dependencies::ListEvidence split{},whole{};
  std::size_t target_count=0;
  std::uint32_t listed_nodes=0,packed_entries=0;
  bool inventory_unchanged=false,ingredient_objects_verified=false;
  std::array<alchemy_extra_execution::Verification,alchemy_recipe_catalog::recipes.size()> profiles{};
  std::array<AlchemyExtraKnowledge,alchemy_recipe_catalog::recipes.size()> knowledge{};
};
struct CraftingAlchemyExtraRead {
  AlchemyExtraCatalogueCapture capture{};
  CraftingAlchemyExtraReadDiagnostic diagnostic{};
};

namespace crafting_alchemy_extra_detail {
namespace cat=alchemy_recipe_catalog;
namespace rt=alchemy_flask_runtime;
namespace dep=alchemy_extra_dependencies;
constexpr std::size_t target_capacity() noexcept {
  std::size_t count=0;
  for(std::size_t r=0;r<cat::recipes.size();++r)for(const auto& t:cat::recipes[r].targets) {
    bool prior=false;
    for(std::size_t q=0;q<r;++q)for(const auto& old:cat::recipes[q].targets)if(old.symbol==t.symbol)prior=true;
    if(!prior)++count;
  }
  return count;
}
inline bool boundary(const CraftingAlchemyExtraInput& in,std::uint32_t owner) noexcept {
  const auto& b=in.boundary;const auto& k=in.key;const auto& c=in.context;
  return owner && in.current_thread==owner && c.thread==owner && k.thread==owner &&
    crafting_detail::valid_key(k) && k.kind==crafting_catalog::Station::alchemy &&
    alchemy_flask_debit::valid(c) && c.epoch==k.generation && c.world==k.world && c.hero==k.hero &&
    c.mob==k.station && k.manager==station_abi::information_manager && in.serial && b.verified &&
    !b.vm_depth && !b.debit_depth &&
    ((b.point==AlchemyExtraReadPoint::post_render && !b.callback_depth && !b.writer_depth && !b.resolve_depth) ||
     (b.point==AlchemyExtraReadPoint::before_original && b.callback_depth==1 && b.writer_depth<=1 && !b.resolve_depth) ||
     (b.point==AlchemyExtraReadPoint::selection_resolved && !b.callback_depth && !b.writer_depth && b.resolve_depth==1));
}
struct ContextRead {
  bool valid=false;
  std::array<std::uint8_t,12> dialog{};
  std::int32_t production=-1,invincible=-1;
};
inline ContextRead context(const ReadOnlyMemory& memory,const CraftingAlchemyExtraInput& in) noexcept {
  ContextRead out;
  if(!rt::detail::context(memory,in.context) || !rt::detail::lab(memory,in.context) ||
     !memory.copy(in.key.manager+0x20,out.dialog.data(),out.dialog.size()) ||
     !runtime_script::detail::u32(out.dialog.data()) || runtime_script::detail::u32(out.dialog.data()+4)!=in.context.hero)return out;
  runtime_script::detail::Layout layout{};
  if(runtime_script::detail::read_layout(memory,in.context.parser,rt::flask_profile,layout)!=runtime_script::Issue::none)return out;
  for(const auto symbol:{12330u,11998u}) {
    const lhiver::SymbolBinding* binding=nullptr;
    for(const auto& b:rt::flask_bindings)if(b.symbol==symbol)binding=&b;
    if(!binding || runtime_script::detail::check_binding(memory,layout,*binding)!=runtime_script::Issue::none)return out;
  }
  const auto production=runtime_script::symbol_int(memory,in.context.parser,12330);
  std::uint32_t invincible=0;
  if(!production.valid || !rt::detail::word(memory,in.context.hero,0x298,invincible) || invincible>1 ||
     runtime_script::detail::stable_layout(memory,in.context.parser,rt::flask_profile,layout)!=runtime_script::Issue::none)return out;
  out.production=production.value;out.invincible=static_cast<std::int32_t>(invincible);
  out.valid=out.production==4 && out.invincible==1;return out;
}
inline bool same(const ContextRead& a,const ContextRead& b) noexcept {
  return a.valid && b.valid && a.dialog==b.dialog && a.production==b.production && a.invincible==b.invincible;
}
inline station_recipe_runtime::ActorEvidence actor(const ReadOnlyMemory& memory,const alchemy_flask_debit::Context& c) noexcept {
  namespace rs=runtime_script;namespace sr=station_recipe_runtime;
  sr::ActorEvidence first;
  for(unsigned pass=0;pass<2;++pass) {
    sr::ActorEvidence value;value.requires_self=true;
    value.issue=rs::detail::read_layout(memory,c.parser,rt::flask_profile,value.layout);
    if(value.issue==rs::Issue::none)value.issue=rs::detail::check_binding(memory,value.layout,rt::flask_bindings[4]);
    if(value.issue==rs::Issue::none) {value.failed_symbol=1683;value.issue=sr::detail::instance(memory,value.layout,rt::flask_profile,1683,value.hero_symbol);}
    if(value.issue==rs::Issue::none) {value.failed_symbol=1679;value.issue=sr::detail::instance(memory,value.layout,rt::flask_profile,1679,value.self_symbol);}
    if(value.issue==rs::Issue::none)value.issue=rs::detail::stable_layout(memory,c.parser,rt::flask_profile,value.layout);
    if(value.issue!=rs::Issue::none)return value;
    value.hero=value.hero_symbol.offset;value.self=value.self_symbol.offset;value.failed_symbol=0;value.read_valid=true;
    value.matches=value.hero==c.hero && value.self==c.hero;
    if(pass && !sr::same_actor_evidence(first,value)) {value.read_valid=value.matches=false;value.issue=rs::Issue::changed_during_read;return value;}
    first=value;
  }
  return first;
}
inline bool same(const AlchemyExtraKnowledge& a,const AlchemyExtraKnowledge& b) noexcept {
  if(!a.bindings_verified || !b.bindings_verified || !a.unchanged || !b.unchanged || a.count!=b.count ||
     a.count>a.values.size() || a.issue!=runtime_script::Issue::none || b.issue!=runtime_script::Issue::none)return false;
  for(std::size_t i=0;i<a.count;++i)if(a.values[i].symbol!=b.values[i].symbol ||
    a.values[i].element!=b.values[i].element || a.values[i].value!=b.values[i].value)return false;
  return true;
}
// Upper bound from a real full list path. An unsafe WholeStack
// case may only truncate the last N value, never release.
inline std::uint32_t executable(std::uint32_t maximum,const alchemy_flask_debit::FlaskStack& flask,
    const dep::ListEvidence& split,const dep::ListEvidence& whole) noexcept {
  if(maximum<=1)return maximum;
  const bool one=split.stable_complete && split.unpack_unreachable && split.amount==flask.amount &&
    (split.path==dep::RemovalPath::split || (split.path==dep::RemovalPath::whole && split.whole_short_path));
  if(!one)return 1;
  const bool all=whole.stable_complete && whole.unpack_unreachable && whole.whole_short_path &&
    whole.path==dep::RemovalPath::whole && whole.amount==flask.amount;
  const auto cap=static_cast<std::uint64_t>(flask.amount)+(all?1u:0u);
  return cap<maximum?static_cast<std::uint32_t>(cap):maximum;
}
} // namespace crafting_alchemy_extra_detail

inline constexpr std::size_t crafting_alchemy_extra_target_capacity=crafting_alchemy_extra_detail::target_capacity();

// Pure quantity projection of a freshly read N0 recording. Existing
// evidence/maximum values remain unchanged; no runtime evidence is created here.
inline AlchemyExtraSnapshot quantify_alchemy_extra_snapshot(const AlchemyExtraSnapshot& preview,std::uint32_t quantity) noexcept {
  namespace cat=alchemy_recipe_catalog;
  AlchemyExtraSnapshot out=preview;out.can_select=out.effect_ready=false;out.quantity=quantity;out.material.quantity=quantity;out.material.craftable=false;
  const auto* recipe=cat::find(preview.id);
  if(!recipe || !cat::structurally_valid(*recipe) || preview.quantity || preview.material.quantity || preview.can_select || preview.effect_ready ||
     preview.material.count!=recipe->targets.count || quantity>INT32_MAX || preview.maximum_executable>preview.material.maximum) {
    out.issue=AlchemyExtraSnapshotIssue::quantity;out.maximum_executable=0;return out;
  }
  std::uint64_t maximum=recipe->callback.maximum_quantity;
  for(std::size_t i=0;i<recipe->targets.count;++i) {
    auto& row=out.rows[i];const auto cost=cat::units(recipe->costs,row.symbol),output=cat::units(recipe->outputs,row.symbol);
    if(row.symbol!=recipe->targets[i].symbol || row.unit_cost!=cost || row.unit_output!=output ||
       (preview.material.known() && (row.stored<0 || row.stored>INT32_MAX || !row.available_known ||
        row.reserved!=(row.symbol==16604?1:0) || row.held!=row.reserved || row.available!=row.stored+row.reserved ||
        row.required || row.produced || row.remaining!=row.available || row.output_space!=INT32_MAX-row.stored))) {
      out.issue=AlchemyExtraSnapshotIssue::quantity;out.maximum_executable=0;return out;
    }
    row.required=std::int64_t{cost}*quantity;row.produced=std::int64_t{output}*quantity;
    row.missing=row.available_known?recipe_material_detail::positive(row.required-row.available):-1;
    row.remaining=row.available_known?row.available-row.required+row.produced:-1;
    row.output_shortfall=row.output_space>=0?recipe_material_detail::positive(row.produced-row.output_space):-1;
    if(preview.material.known()) {
      if(cost)recipe_material_detail::limit(maximum,static_cast<std::uint64_t>(row.available)/cost);
      if(output)recipe_material_detail::limit(maximum,static_cast<std::uint64_t>(row.output_space)/output);
    }
  }
  if(preview.material.known() && maximum!=preview.material.maximum) {out.issue=AlchemyExtraSnapshotIssue::quantity;out.maximum_executable=0;return out;}
  out.material.craftable=preview.material.known() && quantity && quantity<=preview.material.maximum;
  out.can_select=preview.visible && preview.profile_verified && preview.learned_known && preview.learned &&
    preview.native_debit_verified && out.material.craftable && quantity<=preview.maximum_executable;
  out.effect_ready=out.can_select && preview.owner_conditions_match;
  if(quantity && preview.material.known() && (!out.material.craftable || quantity>preview.maximum_executable))out.issue=AlchemyExtraSnapshotIssue::quantity;
  return out;
}

// Persistent workspace. Exactly one deduplicated inventory read, then
// byte comparison of the same recording. No engine calls, VM/inventory
// writes, heap allocation, or return of borrowed memory views.
class CraftingAlchemyExtraReader {
public:
  explicit CraftingAlchemyExtraReader(std::uint32_t owner) noexcept:owner_(owner),list_(owner) {
    for(const auto& r:alchemy_recipe_catalog::recipes) {
      if(!alchemy_recipe_catalog::structurally_valid(r)) {valid_=false;return;}
      for(const auto& t:r.targets) {
        std::size_t i=0;while(i<count_ && targets_[i].symbol!=t.symbol)++i;
        if(i<count_) {if(std::strcmp(targets_[i].name,t.name))valid_=false;}
        else if(count_==targets_.size())valid_=false;
        else targets_[count_++]=t;
      }
    }
  }
  CraftingAlchemyExtraReader(const CraftingAlchemyExtraReader&)=delete;
  CraftingAlchemyExtraReader& operator=(const CraftingAlchemyExtraReader&)=delete;
  CraftingAlchemyExtraRead capture(const ReadOnlyMemory& memory,const CraftingAlchemyExtraInput& in) noexcept {
    namespace detail=crafting_alchemy_extra_detail;namespace cat=alchemy_recipe_catalog;
    namespace rt=alchemy_flask_runtime;namespace dr=alchemy_extra_dependency_reader;
    using Issue=CraftingAlchemyExtraReadIssue;
    CraftingAlchemyExtraRead out;auto& result=out.capture;auto& diag=out.diagnostic;
    result.key=in.key;result.serial=in.serial;diag.target_count=count_;
    for(std::size_t i=0;i<cat::recipes.size();++i) {result.recipes[i].id=cat::recipes[i].id;result.recipes[i].context=in.context;result.recipes[i].serial=in.serial;}
    if(!owner_ || in.current_thread!=owner_ || in.context.thread!=owner_) {diag.issue=Issue::thread;return out;}
    if(reading_) {diag.issue=Issue::reentry;return out;}
    if(!valid_ || !count_) {diag.issue=Issue::catalogue;return out;}
    if(!detail::boundary(in,owner_)) {diag.issue=Issue::boundary;return out;}
    const bool preview=in.boundary.point==AlchemyExtraReadPoint::post_render||in.boundary.point==AlchemyExtraReadPoint::selection_resolved;
    result.actor_purpose=preview?AlchemyExtraActorPurpose::hero_preview:AlchemyExtraActorPurpose::original_entry;
    reading_=true;struct Lease {bool& value;~Lease(){value=false;}} lease{reading_};
    result.boundary_verified=true;const auto before=detail::context(memory,in);
    if(!before.valid) {diag.issue=Issue::context;return out;}
    result.context_before=in.context;result.manager_before=in.key.manager;result.production_before=before.production;
    result.actor_before=detail::actor(memory,in.context);
    const dr::Environment environment{in.context,in.modules,in.targets,in.restore_item};
    diag.dependencies=dr::verify_environment(memory,environment);
    const auto policy=dr::code_policy(in.modules);diag.native=rt::verify_native(memory,policy);diag.flask_constructor=rt::verify_flask_constructor(memory);
    packed_detail::Header inventory_header{};
    const bool header=packed_detail::at_offset(memory,in.context.hero,packed_detail::header_offset,inventory_header.data(),inventory_header.size());
    auto stock=read_packed_inventory<crafting_alchemy_extra_target_capacity>(memory,in.context.hero,targets_.data(),count_,true,inventory_);
    diag.inventory_issue=stock.issue;diag.listed_nodes=stock.listed_nodes;diag.packed_entries=stock.packed_entries;
    diag.hand_issue=station_recipe_snapshot_detail::begin_hands(memory,in.context.hero,hands_);result.hands_before=hands_.before;
    std::array<bool,crafting_alchemy_extra_target_capacity> item_known{};item_known.fill(true);
    std::size_t objects=0,flask_index=count_;for(std::size_t i=0;i<count_;++i)if(targets_[i].symbol==16604)flask_index=i;
    alchemy_flask_debit::FlaskStack flask{};
    if(stock.storage_totals_known && flask_index<count_) {
      flask.known=true;flask.packed=stock.packed[flask_index];flask.listed_stacks=stock.listed_occurrences[flask_index];
    }
    if(stock.listed_known && stock.snapshot_unchanged && stock.listed_nodes<=objects_.size()) {
      objects=stock.listed_nodes;
      for(std::size_t i=0;i<objects;++i) {
        const auto& node=inventory_.records[i];const auto object=runtime_script::detail::u32(node.node.data()+4);objects_[i]=object;
        const auto symbol=station_detail::little_i32(node.item_fields.data()+4);
        item_fields_[i]={};auto& fields=item_fields_[i];fields.object=object;
        fields.known=rt::detail::word(memory,object,0,fields.type) && rt::detail::word(memory,object,0x158,fields.flags);
        bool alias=false;for(std::size_t slot=0;slot<hands_.count;++slot)
          if(runtime_script::detail::u32(hands_.records[slot].fields.data()+0x30)==object)alias=true;
        const bool valid=fields.known && fields.type==hand_detail::item_vtable && (fields.flags&0x200000u) && !(fields.flags&0x40000000u) && !alias;
        for(std::size_t t=0;t<count_;++t)if(targets_[t].symbol==symbol)item_known[t]=item_known[t]&&valid;
        if(symbol==16604) {flask.object=object;flask.flags=fields.flags;flask.amount=runtime_script::detail::u32(node.item_fields.data());}
      }
    }
    flask.all_slot_aliases_excluded=flask_index<count_ && item_known[flask_index] && hands_.captured;
    flask.native_item_contract=flask.all_slot_aliases_excluded;
    if(flask.known && flask.listed_stacks==1 && !flask.packed && flask.amount && flask.object &&
       flask.native_item_contract && diag.dependencies.valid) {
      diag.split=list_.capture(memory,in.current_thread,{in.context.hero,flask.object,1,in.context.world});
      diag.whole=list_.capture(memory,in.current_thread,{in.context.hero,flask.object,flask.amount,in.context.world});
    }
    for(std::size_t i=0;i<cat::recipes.size();++i) {
      diag.profiles[i]=alchemy_extra_execution::verify_recipe(memory,cat::recipes[i]);
      diag.knowledge[i]=read_alchemy_extra_knowledge(memory,in.context.parser,cat::recipes[i]);
    }
    for(std::size_t i=0;i<cat::recipes.size();++i) {
      const auto knowledge=read_alchemy_extra_knowledge(memory,in.context.parser,cat::recipes[i]);
      if(!detail::same(diag.knowledge[i],knowledge)) {diag.knowledge[i].unchanged=false;diag.knowledge[i].issue=runtime_script::Issue::changed_during_read;}
    }
    bool metadata_unchanged=true;
    for(std::size_t i=0;i<objects;++i) {
      const auto& fields=item_fields_[i];std::uint32_t type=0,flags=0;
      if(!fields.known || !rt::detail::word(memory,fields.object,0,type) || !rt::detail::word(memory,fields.object,0x158,flags) ||
         type!=fields.type || flags!=fields.flags)metadata_unchanged=false;
    }
    packed_detail::Header final_header{};
    diag.inventory_unchanged=header && stock.storage_totals_known && stock.snapshot_unchanged && metadata_unchanged &&
      packed_detail::compare_list(memory,inventory_,stock.listed_nodes)==PackedInventoryIssue::none &&
      packed_detail::compare_pack(memory,inventory_,runtime_script::detail::u32(inventory_header.data()+28),runtime_script::detail::u32(inventory_header.data()+32))==PackedInventoryIssue::none &&
      packed_detail::at_offset(memory,in.context.hero,packed_detail::header_offset,final_header.data(),final_header.size()) && inventory_header==final_header;
    if(!diag.inventory_unchanged) {stock.storage_totals_known=stock.snapshot_unchanged=false;if(stock.issue==PackedInventoryIssue::none)stock.issue=PackedInventoryIssue::changed_during_read;diag.inventory_issue=stock.issue;}
    result.hands_unchanged=diag.hand_issue==StationHandReadIssue::none && station_recipe_snapshot_detail::verify_hands(memory,hands_);
    if(result.hands_unchanged)result.hands_after=hands_.before;
    result.actor_after=detail::actor(memory,in.context);
    result.actor_unchanged=station_recipe_runtime::same_actor_evidence(result.actor_before,result.actor_after);
    const auto after=detail::context(memory,in);result.context_unchanged=detail::same(before,after);
    if(after.valid) {result.context_after=in.context;result.manager_after=in.key.manager;result.production_after=after.production;}
    // In the resting menu, SELF may point to the last foreign script actor.
    // Raw values/matches remain unchanged. Only the physically bound HERO
    // is accounted for the preview; before original, SELF still applies.
    const bool hero_bound=result.actor_unchanged&&result.actor_before.hero==in.context.hero&&result.actor_after.hero==in.context.hero;
    const auto& hand=hands_.before;
    result.physical_hero_preview_verified=preview&&hero_bound&&result.context_unchanged&&result.hands_unchanged&&
      in.boundary.physical_lab_hand_contract_verified&&hand.left_slot_known&&hand.right_slot_known&&hand.interact_item_known&&
      hand.hands.left.state==0&&!hand.hands.left.object&&hand.hands.right.state==1&&hand.hands.right.object&&
      hand.hands.right.symbol==16604&&hand.hands.right.amount==1&&hand.interact_item==hand.hands.right.object;
    const bool actor_bound=preview?result.physical_hero_preview_verified:
      hero_bound&&result.actor_before.matches&&result.actor_after.matches;
    const auto environment_after=dr::verify_environment(memory,environment);
    const auto native_after=rt::verify_native(memory,policy);const auto flask_after=rt::verify_flask_constructor(memory);
    // The new world list evidence must not precede the entire UI
    // recording. Missing or changed lists are not treated as empty lists.
    if(diag.whole.whole_world.verified||diag.split.whole_world.verified){
      const auto whole_after=list_.capture(memory,in.current_thread,{in.context.hero,flask.object,flask.amount,in.context.world});
      if(diag.split.whole_world.verified&&!alchemy_extra_dependencies::same_list_evidence(diag.split,whole_after))diag.split={};
      if(!alchemy_extra_dependencies::same_list_evidence(diag.whole,whole_after))diag.whole={};
    }
    const bool native=diag.dependencies.valid && environment_after.valid &&
      alchemy_extra_dependencies::same(diag.dependencies.item,environment_after.item) &&
      diag.dependencies.code.zbug_unpack_call==environment_after.code.zbug_unpack_call &&
      diag.dependencies.code.zpe_init_call==environment_after.code.zpe_init_call &&
      diag.native.reviewed_ranges_match && diag.native.vtable_slots_match && native_after.reviewed_ranges_match && native_after.vtable_slots_match &&
      diag.flask_constructor.constructor_and_bindings_match && diag.flask_constructor.strings_match &&
      flask_after.constructor_and_bindings_match && flask_after.strings_match;
    RecipeInventory projected;
    projected.header_known=stock.header_known;projected.listed_known=stock.listed_known;projected.packed_known=stock.packed_known;
    projected.storage_totals_known=stock.storage_totals_known;projected.stable_boundary_verified=stock.stable_boundary_verified;
    projected.snapshot_unchanged=stock.snapshot_unchanged;projected.issue=stock.issue;projected.listed_nodes=stock.listed_nodes;projected.packed_entries=stock.packed_entries;
    projected.listed_objects={objects_.data(),objects};diag.ingredient_objects_verified=true;
    for(std::size_t i=0;i<cat::recipes.size();++i) {
      const auto& recipe=cat::recipes[i];bool item_contract=true;
      for(std::size_t t=0;t<recipe.targets.count;++t) {
        std::size_t index=0;while(index<count_ && targets_[index].symbol!=recipe.targets[t].symbol)++index;
        if(index==count_) {diag.issue=Issue::catalogue;return out;}
        items_[t]={recipe.targets[t].symbol,stock.listed[index],stock.packed[index],stock.total[index],stock.listed_occurrences[index],
          stock.packed_occurrences[index],stock.duplicate_packed_target[index],stock.has_active_marker[index],stock.mixed_representation[index]};
        if(cat::units(recipe.costs,recipe.targets[t].symbol))item_contract=item_contract&&item_known[index];
      }
      diag.ingredient_objects_verified=diag.ingredient_objects_verified&&item_contract;projected.items={items_.data(),recipe.targets.count};
      AlchemyExtraSnapshotInput input;input.context=in.context;input.serial=in.serial;input.inventory=projected;input.hands=hands_.before.hands;
      input.knowledge=diag.knowledge[i];input.flask=flask;input.boundary_verified=result.context_unchanged;
      input.local_recipe_contract_verified=diag.profiles[i].valid;
      input.hand_contract_verified=in.boundary.physical_lab_hand_contract_verified && result.hands_unchanged &&
        hands_.before.left_slot_known && hands_.before.right_slot_known;
      input.interact_item_known=hands_.before.interact_item_known;input.interact_item=hands_.before.interact_item;
      input.native_debit_contract_verified=native && item_contract && actor_bound;
      result.recipes[i]=alchemy_extra_snapshot(recipe,input,0);result.evaluated[i]=true;
      auto& snapshot=result.recipes[i];snapshot.maximum_executable=detail::executable(snapshot.maximum_executable,flask,diag.split,diag.whole);
    }
    if(!result.context_unchanged)diag.issue=Issue::context;
    else if(!actor_bound)diag.issue=Issue::actor;
    else if(!result.hands_unchanged)diag.issue=Issue::hands;
    else if(!diag.inventory_unchanged)diag.issue=Issue::inventory;
    else if(!native || !diag.ingredient_objects_verified)diag.issue=Issue::native;
    return out;
  }
  AlchemyExtraSnapshot capture_recipe(const ReadOnlyMemory& memory,const CraftingAlchemyExtraInput& in,
      alchemy_recipe_catalog::RecipeId id,std::uint32_t quantity) noexcept {
    const auto all=capture(memory,in);
    for(std::size_t i=0;i<all.capture.recipes.size();++i)if(all.capture.evaluated[i] && all.capture.recipes[i].id==id){
      auto snapshot=quantify_alchemy_extra_snapshot(all.capture.recipes[i],quantity);
      // A preview recording is not a positive actor evidence for the
      // immediate original entry, even with readable owner conditions.
      if(all.capture.actor_purpose!=AlchemyExtraActorPurpose::original_entry||
         !all.capture.actor_unchanged||!all.capture.actor_before.matches||!all.capture.actor_after.matches)snapshot.effect_ready=false;
      return snapshot;
    }
    AlchemyExtraSnapshot out;out.id=id;out.quantity=quantity;out.context=in.context;out.serial=in.serial;out.issue=AlchemyExtraSnapshotIssue::context;return out;
  }
private:
  struct ItemFields {std::uint32_t object=0,type=0,flags=0;bool known=false;};
  std::uint32_t owner_=0;bool valid_=true,reading_=false;std::size_t count_=0;
  std::array<PackedInventoryTarget,crafting_alchemy_extra_target_capacity> targets_{};
  PackedInventoryWorkspace inventory_{};
  station_recipe_snapshot_detail::HandWorkspace hands_{};
  alchemy_extra_dependencies::ListReader list_;
  std::array<ItemFields,max_packed_inventory_nodes> item_fields_{};
  std::array<std::uint32_t,max_packed_inventory_nodes> objects_{};
  std::array<RecipeInventoryItem,alchemy_recipe_catalog::catalog_target_capacity> items_{};
};
} // namespace ubc
