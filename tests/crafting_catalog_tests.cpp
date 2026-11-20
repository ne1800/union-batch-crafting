// SPDX-License-Identifier: MIT
#include "ubc/catalogs/crafting_catalog.hpp"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <set>
namespace crafting_test {
namespace c=ubc::crafting_catalog;
using Kind=c::Station;
unsigned checks=0,failures=0;
void check(bool value,const char* message) {
  ++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",message);}
}
ubc::CraftingKey key(Kind kind) { return {1,2,3,4,5,6,kind}; }
ubc::StationRecipeHands hands(Kind kind) {
  ubc::StationRecipeHands value;value.hands.left={0,-1,-1,0};
  const auto symbol=kind==Kind::stove?16608:kind==Kind::alchemy?16604:kind==Kind::leather?17342:kind==Kind::saw?16609:kind==Kind::anvil?16435:16667;
  value.hands.right={1,symbol,1,0x9000};
  if(kind==Kind::forge||kind==Kind::anvil)value.hands.left={1,16666,1,0x8000};
  if(kind!=Kind::stove) {
    value.left_slot_known=value.right_slot_known=value.interact_item_known=true;
    value.left_tmp_level=kind==Kind::forge||kind==Kind::anvil?1:0;
    value.right_tmp_level=kind==Kind::saw||kind==Kind::alchemy?0:kind==Kind::anvil?1:2;
    value.interact_item=0x9000;
  }
  if(kind==Kind::bow){value.hands={{0,-1,-1,0},{0,-1,-1,0}};value.interact_item=0;value.left_tmp_level=value.right_tmp_level=0;}
  if(kind==Kind::cool){value.hands={{1,16612,1,0x8000},{0,-1,-1,0}};value.interact_item=0x8000;value.left_tmp_level=1;value.right_tmp_level=0;}
  if(kind==Kind::grind){value.hands={{1,16613,1,0x8000},{0,-1,-1,0}};value.interact_item=0x8000;value.left_tmp_level=1;value.right_tmp_level=0;}
  return value;
}
std::size_t index_of(c::RecipeId id) {
  for(std::size_t i=0;i<c::recipes.size();++i)if(c::recipes[i].id==id)return i;
  std::abort();
}
ubc::CraftingState& state(ubc::CraftingSnapshot& value,c::RecipeId id) { return value.recipes[index_of(id)]; }
const ubc::CraftingState& state(const ubc::CraftingSnapshot& value,c::RecipeId id) { return value.recipes[index_of(id)]; }
ubc::CraftingState ready_state(c::RecipeId id,std::uint32_t maximum=7) {
  auto value=ubc::make_crafting_state(id);const auto& d=*c::find(id);
  value.generation=1;value.evaluated=value.knowledge_read_valid=value.known=value.contract=true;
  value.hand_known=value.material_known=value.material_verified=true;
  value.actor_known=value.actor_matches=d.route!=c::Route::base;value.maximum=maximum;
  value.context_known=value.context_matches=c::requires_context(d.route);
  if(d.spirit){value.branch_known=true;value.branch_value=1;value.branch_output=15741;}
  if(d.grind)value.grind={true,1,d.grind->guilds[0],98};
  for(std::size_t i=0;i<value.target_count;++i) {
    auto& row=value.materials[i];const auto h=c::hand_amounts(d,row.symbol);
    row.unit_output=c::units(c::outputs_for(d,value.branch_known,value.branch_value),row.symbol);
    row.consumed_reserve=h.consumed;row.tool_reserve=h.tool;row.generated_prop=h.generated;
    row.stored=static_cast<std::int32_t>(std::uint64_t{row.unit_cost>row.unit_minimum?row.unit_cost:row.unit_minimum}*maximum);
    if(h.consumed&&row.stored)row.stored-=h.consumed;
    if(row.fixed_minimum)row.stored=1;
    if(h.tool)row.stored=6;
    if(h.generated&&!h.stored_absent&&!row.unit_cost)row.stored=5;
    if(row.symbol==16607)row.stored=6;
  }
  return value;
}
ubc::CraftingSnapshot example(Kind kind=Kind::stove,std::uint32_t maximum=7,std::int32_t guild=1) {
  ubc::CraftingSnapshot value;value.key=key(kind);value.hands=hands(kind);
  for(const auto& d:c::recipes)if(d.station==kind)state(value,d.id)=ready_state(d.id,maximum);
  if(kind==Kind::grind)for(const auto& d:c::recipes)if(d.grind){auto& s=state(value,d.id);
    s.grind.guild=guild;s.known=guild==d.grind->guilds[0]||guild==d.grind->guilds[1];if(!s.known)s.maximum=0;}
  return value;
}
template<class Session>void neutral(Session& session,std::uint64_t& frame) {
  for(unsigned i=0;i<3;++i)session.observe(++frame,false,true);
  // Explicit synthetic UI values only. No native item path test.
  if(session.selected()&&(session.preview().key.kind==Kind::bow||session.preview().key.kind==Kind::cool||session.preview().key.kind==Kind::grind))
    session.observe_quantity_check({session.selected()->id,session.quantity(),session.preview().key,*session.selected(),session.preview().hands},frame,true);
}
ubc::RecipeCatalogueCapture base_capture(Kind kind) {
  ubc::RecipeCatalogueCapture value;const auto k=key(kind);value.hands=hands(kind).hands;
  value.snapshot.key={k.generation,k.thread,k.world,k.hero,k.station,k.manager,
    kind==Kind::stove?ubc::recipe_catalog::Station::stove:ubc::recipe_catalog::Station::alchemy};
  auto& diag=value.diagnostic;
  diag.inventory_known=diag.listed_known=diag.packed_known=diag.snapshot_unchanged=diag.hands_read_valid=true;
  for(std::size_t i=0;i<ubc::recipe_catalog::recipes.size();++i) {
    const auto& d=c::recipes[i];if(d.station!=kind)continue;
    const auto s=ready_state(d.id);auto& out=value.snapshot.recipes[i];auto& rd=diag.recipes[i];
    rd.applicable=rd.evaluated=rd.profile.matches=rd.knowledge.read_valid=rd.knowledge.known=true;
    out.known=out.contract=out.hand_known=out.material_known=true;out.maximum=s.maximum;
    for(std::size_t j=0;j<s.target_count;++j) {
      out.materials[j].stored=s.materials[j].stored;out.materials[j].reserved=s.materials[j].consumed_reserve;
    }
  }
  return value;
}
ubc::StationRecipeCatalogueCapture station_capture(Kind kind) {
  ubc::StationRecipeCatalogueCapture value;const auto k=key(kind);value.hands=hands(kind);
  value.key={k.generation,k.thread,k.world,k.hero,k.station,k.manager,
    kind==Kind::leather?ubc::station_recipe_catalog::Station::leatherwork:
    kind==Kind::saw?ubc::station_recipe_catalog::Station::saw:ubc::station_recipe_catalog::Station::forge};
  auto& diag=value.diagnostic;diag.inventory_known=diag.hands_unchanged=diag.actor_unchanged=true;
  diag.actor_before.read_valid=diag.actor_before.matches=true;diag.actor_before.hero=k.hero;
  diag.actor_before.requires_self=kind==Kind::saw;diag.actor_before.self=kind==Kind::saw?k.hero:0;
  diag.actor_after=diag.actor_before;
  for(std::size_t i=0;i<ubc::station_recipe_catalog::recipes.size();++i) {
    const auto& d=c::recipes[ubc::recipe_catalog::recipes.size()+i];auto& out=value.recipes[i];
    out.id=d.station_descriptor->id;out.target_count=d.targets.count;out.material.completion=d.station_descriptor->completion;
    for(std::size_t j=0;j<out.target_count;++j) {
      out.rows[j].symbol=d.targets[j].symbol;out.rows[j].unit_cost=c::units(d.costs,out.rows[j].symbol);
      out.rows[j].unit_output=c::units(d.outputs,out.rows[j].symbol);
    }
    if(d.station!=kind)continue;
    const auto s=ready_state(d.id);auto& rd=diag.recipes[i];rd.assembly=ubc::station_recipe_runtime::AssemblyIssue::none;
    rd.profile.matches=rd.knowledge.read_valid=rd.knowledge.known=true;
    out.applicable=out.evaluated=out.known=out.contract=out.actor_known=out.actor_matches=true;
    out.material.inventory_known=out.material.hand_known=true;out.material.maximum=s.maximum;out.material.count=s.target_count;
    for(std::size_t j=0;j<s.target_count;++j) {
      out.rows[j].stored=s.materials[j].stored;out.rows[j].reserved_tool=s.materials[j].tool_reserve;
      out.rows[j].generated_hand=s.materials[j].generated_prop;
    }
  }
  return value;
}
template<class Capture,class Recipes>
Capture anvil_capture_values(const Recipes& recipes) {
  Capture value;const auto k=key(Kind::anvil);value.hands=hands(Kind::anvil);
  value.key={k.generation,k.thread,k.world,k.hero,k.station,k.manager,ubc::anvil_recipe_catalog::Station::anvil};
  auto& diag=value.diagnostic;diag.inventory_known=diag.hands_unchanged=diag.actor_unchanged=diag.context_unchanged=true;
  diag.actor_before.read_valid=diag.actor_before.matches=diag.actor_before.requires_self=true;
  diag.actor_before.hero=diag.actor_before.self=k.hero;diag.actor_after=diag.actor_before;
  auto& context=diag.context_before;context.read_valid=true;context.production=context.start_smithing=1;
  context.normal_weapons=0;context.ore_weapons=1;
  for(std::size_t i=0;i<context.readings.size();++i)context.readings[i].value=i<2?1:static_cast<int>(i==3);
  diag.context_after=context;
  for(std::size_t i=0;i<recipes.size();++i) {
    const auto& d=recipes[i];auto& out=value.recipes[i];const auto s=ready_state(static_cast<c::RecipeId>(d.id));
    out.id=d.id;out.target_count=d.targets.count;out.evaluated=out.known=out.contract=out.actor_known=out.actor_matches=out.context_known=out.context_matches=true;
    auto& rd=diag.recipes[i];rd.assembly=ubc::station_recipe_runtime::AssemblyIssue::none;
    rd.profile.matches=rd.knowledge.read_valid=rd.knowledge.known=true;
    out.material.inventory_known=out.material.hand_known=true;out.material.count=s.target_count;out.material.maximum=s.maximum;
    for(std::size_t j=0;j<s.target_count;++j) {
      auto& row=out.rows[j];const auto& input=s.materials[j];row.symbol=input.symbol;row.unit_cost=input.unit_cost;row.unit_output=input.unit_output;
      row.stored=input.stored;
      if(row.unit_cost)for(const auto& family:recipes)for(const auto& cost:family.costs)
        if(cost.symbol==row.symbol)row.stored=std::max(row.stored,std::int64_t{cost.units}*7);
      row.available=row.stored;row.available_known=true;row.generated_hand=input.generated_prop;
    }
  }
  return value;
}
ubc::AnvilRecipeCatalogueCapture anvil_capture() {
  return anvil_capture_values<ubc::AnvilRecipeCatalogueCapture>(ubc::anvil_recipe_catalog::recipes);
}
ubc::OreRecipeCatalogueCapture ore_capture() {
  return anvil_capture_values<ubc::OreRecipeCatalogueCapture>(ubc::ore_recipe_catalog::recipes);
}
ubc::AlchemyExtraCatalogueCapture extra_capture() {
  namespace cat=ubc::alchemy_recipe_catalog;ubc::AlchemyExtraCatalogueCapture value;
  value.key=key(Kind::alchemy);value.serial=9;
  value.actor_purpose=ubc::AlchemyExtraActorPurpose::original_entry;
  value.boundary_verified=value.context_unchanged=value.hands_unchanged=value.actor_unchanged=true;
  value.context_before={value.key.generation,value.key.thread,ubc::lhiver::game_parser,7,value.key.world,value.key.hero,value.key.station};
  value.context_after=value.context_before;value.manager_before=value.manager_after=value.key.manager;
  value.production_before=value.production_after=4;value.hands_before=value.hands_after=hands(Kind::alchemy);
  value.actor_before.read_valid=value.actor_before.matches=value.actor_before.requires_self=true;
  value.actor_before.hero=value.actor_before.self=value.key.hero;value.actor_after=value.actor_before;
  for(std::size_t i=0;i<cat::recipes.size();++i) {
    const auto& d=cat::recipes[i];ubc::AlchemyExtraSnapshotInput input;
    input.context=value.context_before;input.serial=value.serial;input.boundary_verified=true;
    input.local_recipe_contract_verified=input.hand_contract_verified=input.native_debit_contract_verified=true;
    input.hands=value.hands_before.hands;input.interact_item_known=true;input.interact_item=input.hands.right.object;
    input.knowledge.bindings_verified=input.knowledge.unchanged=true;input.knowledge.count=d.requirements.count;
    for(std::size_t j=0;j<d.requirements.count;++j) {
      const auto& q=d.requirements[j];input.knowledge.values[j]={q.symbol,q.element,q.value};
    }
    auto& inventory=input.inventory;inventory.header_known=inventory.listed_known=inventory.packed_known=inventory.storage_totals_known=true;
    inventory.stable_boundary_verified=inventory.snapshot_unchanged=true;
    std::array<ubc::RecipeInventoryItem,cat::catalog_target_capacity> items{};
    std::array<std::uint32_t,cat::catalog_target_capacity> objects{};std::size_t listed=0;
    const auto state=ready_state(static_cast<c::RecipeId>(d.id));
    for(std::size_t j=0;j<d.targets.count;++j) {
      const auto& row=state.materials[j];items[j].symbol=row.symbol;auto stored=row.stored;
      if(row.symbol!=16604)for(const auto& family:cat::recipes)for(const auto& cost:family.costs)
        if(cost.symbol==row.symbol)stored=std::max(stored,static_cast<std::int32_t>(cost.units*7));
      items[j].listed=items[j].total=stored;items[j].packed=0;
      if(stored) {items[j].listed_occurrences=1;objects[listed++]=0x100000+static_cast<std::uint32_t>(row.symbol)*4;}
      if(row.symbol==16604)input.flask={true,true,true,0,1,objects[listed-1],static_cast<std::uint32_t>(stored),0x200000};
    }
    inventory.items={items.data(),d.targets.count};inventory.listed_objects={objects.data(),listed};inventory.listed_nodes=static_cast<std::uint32_t>(listed);
    value.recipes[i]=ubc::alchemy_extra_snapshot(d,input,0);value.evaluated[i]=true;
  }
  return value;
}
}
#ifndef UBC_CRAFTING_FIXTURE_ONLY
int main() {
  using namespace crafting_test;
  check(ubc::crafting_catalog::recipes.size()==98,"one catalogue contains ninety-eight real recipes");
  std::set<c::RecipeId> ids;std::array<unsigned,9> counts{};
  for(const auto& d:c::recipes) {
    check(ids.insert(d.id).second&&c::find(d.id)==&d,"all uint32 identifiers are unique and stable");
    ++counts[static_cast<std::size_t>(d.station)];
    check(d.name&&*d.name&&d.quantity_unit&&*d.quantity_unit&&d.targets.count<=c::target_capacity,
          "all normalized names units and bounded target views exist");
    if(d.route==c::Route::base)check(d.id<9&&d.base&&!d.station_descriptor&&d.targets.data==d.base->targets.data&&
      d.costs.data==d.base->costs.data&&d.outputs.data==d.base->outputs.data,"base route retains exact static descriptor views");
    else if(d.route==c::Route::station)check(d.id>=0x1001&&d.station_descriptor&&!d.base&&d.targets.data==d.station_descriptor->targets.data&&
      d.quantity_unit==d.station_descriptor->quantity_unit,"station route never reinterprets a base descriptor");
    else if(d.route==c::Route::alchemy_extra)check(d.alchemy_extra&&!d.base&&!d.station_descriptor&&!d.anvil&&!d.ore&&
      d.targets.data==d.alchemy_extra->targets.data&&d.costs.data==d.alchemy_extra->costs.data,"extra route retains the real thirteen-recipe descriptor");
    else if(d.route==c::Route::anvil)check(d.anvil&&!d.base&&!d.station_descriptor&&!d.alchemy_extra&&!d.ore&&
      d.targets.data==d.anvil->targets.data&&d.costs.data==d.anvil->costs.data,"anvil route never fabricates a station descriptor");
    else if(d.route==c::Route::spirit)check(d.spirit&&!d.alchemy_extra&&!d.base&&!d.station_descriptor&&!d.anvil&&!d.ore&&d.targets.count==7&&d.outputs.count==0,"spirit branch is explicit and keeps both output stocks");
    else if(d.route==c::Route::bow)check(d.bow&&!d.spirit&&!d.ore&&!d.base&&d.targets.data==d.bow->targets.data&&d.costs.data==d.bow->costs.data,"bow uses its actual descriptor including distinct minima");
    else if(d.route==c::Route::cool)check(d.cool&&!d.bow&&!d.base&&!d.station_descriptor&&d.id==0xa001&&d.targets.count==2&&d.cool==&ubc::cool_recipe_catalog::recipe,"cool uses its own canonical generated-hand descriptor");
    else if(d.route==c::Route::grind)check(d.grind&&!d.cool&&!d.bow&&!d.base&&!d.station_descriptor&&
      d.id>=0x9001&&d.id<=0x9016&&d.targets.count==3&&d.grind==ubc::grind_recipe::find(d.grind->function.symbol),"grind uses its canonical profile and lifecycle row");
    else if(d.tobacco)check(d.route==c::Route::tobacco&&d.targets.data==d.tobacco->targets.data(),"tobacco binds the paid owner and a fixed additive");
    else check(d.route==c::Route::ore&&d.ore&&!d.base&&!d.station_descriptor&&!d.alchemy_extra&&!d.anvil&&
      d.targets.data==d.ore->targets.data&&d.costs.data==d.ore->costs.data,"ore route retains its own complete material coefficients");
    const auto s=ready_state(d.id);check(ubc::valid_crafting_state(s),"all normalized material states validate");
    for(std::size_t i=0;i<s.target_count;++i) {
      const auto p=ubc::crafting_amount_preview(s.materials[i],7);
      check(p.available_known&&p.ready_known&&p.end_known&&!p.missing&&!p.output_shortfall&&!p.end_overflow,
            "exact maximum preview retains safe per-recipe consumption and output");
    }
  }
  check(counts==std::array<unsigned,9>{{3,23,9,5,2,13,20,1,22}},"nine physical station kinds partition all recipes without route confusion");
  check(!c::find(UINT32_MAX)&&!c::find(9),"unregistered identifiers never alias an existing recipe");
  for(const auto kind:{Kind::stove,Kind::alchemy,Kind::leather,Kind::saw,Kind::forge}) {
    auto snapshot=kind==Kind::stove||kind==Kind::alchemy?ubc::project_capture(base_capture(kind)):
      ubc::project_capture(station_capture(kind));
    check(ubc::valid_crafting_snapshot(snapshot),"one existing capture projects to one valid twenty-five-value snapshot");
    for(const auto& d:c::recipes) {
      const auto& s=state(snapshot,d.id);
      const bool expected=d.station==kind&&(d.route==c::Route::base||d.route==c::Route::station);
      check(s.known==expected&&s.contract==expected&&s.material_known==expected,
            "single old capture never invents evidence for a newly added route at the same physical station");
      if(expected)check(s.generation==snapshot.key.generation&&s.maximum==7&&
        ubc::crafting_block_reason(s)==ubc::CraftingBlockReason::none,"projected gates are copied and generation-bound");
    }
  }
  {
    auto source=base_capture(Kind::alchemy);auto projected=ubc::project_capture(source);
    const auto& potion=state(projected,1);const auto* flask=ubc::crafting_detail::material(potion,16604);
    const auto p=ubc::crafting_amount_preview(*flask,3);
    check(flask->consumed_reserve==1&&!flask->tool_reserve&&p.available==7&&p.required==3&&p.ready_storage==4&&p.end_storage==4,
          "reserved flask is consumed exactly once as part of total recipe cost");
    source.snapshot.recipes[1].materials[0].stored=0;
    check(state(projected,1).materials[0].stored!=0,"projection owns values after source mutation");
    source=base_capture(Kind::alchemy);source.diagnostic.recipes[1].profile.matches=false;
    projected=ubc::project_capture(source);check(state(projected,1).known&&!state(projected,1).contract,
          "missing runtime profile leaves learned recipe visible without confirm permission");
    source=base_capture(Kind::alchemy);source.diagnostic.recipes[1].knowledge.read_valid=false;
    check(!state(ubc::project_capture(source),1).known,"unreadable learning never preserves a stale learned grant");
    source=base_capture(Kind::alchemy);std::swap(source.snapshot.recipes[1].materials[0],source.snapshot.recipes[1].materials[2]);
    check(ubc::valid_crafting_snapshot(ubc::project_capture(source)),"base material order maps by symbol rather than position");
    source.snapshot.recipes[1].materials[0].symbol=source.snapshot.recipes[1].materials[1].symbol;
    check(ubc::project_capture(source).issue==ubc::CraftingProjectionIssue::source,"duplicate base material symbol is rejected");
  }
  {
    auto source=station_capture(Kind::saw);auto projected=ubc::project_capture(source);
    const auto& ammo=state(projected,0x2001);const auto* saw=ubc::crafting_detail::material(ammo,16609);
    const auto p=ubc::crafting_amount_preview(*saw,3);
    check(saw->tool_reserve==1&&!saw->consumed_reserve&&!saw->unit_cost&&p.available==6&&!p.required&&p.ready_storage==6&&p.end_storage==7,
          "saw is never an ingredient and returns only at normal end");
    const auto* output=ubc::crafting_detail::material(ammo,16804);
    check(ubc::crafting_amount_preview(*output,3).produced==90&&!std::strcmp(c::find(0x2001)->quantity_unit,"Lose"),
          "three arrow lots mean ninety output items");
    source.diagnostic.actor_after.self=99;projected=ubc::project_capture(source);
    check(state(projected,0x2001).known&&!state(projected,0x2001).actor_known,
          "actor mismatch cannot be hidden by stale aggregate actor_unchanged boolean");
    source=station_capture(Kind::saw);source.diagnostic.actor_before.hero=99;source.diagnostic.actor_after=source.diagnostic.actor_before;
    projected=ubc::project_capture(source);check(state(projected,0x2001).actor_known&&!state(projected,0x2001).actor_matches,
          "stable but foreign actor is separate from unknown reads and never authorizes keyed hero");
    source=station_capture(Kind::saw);source.diagnostic.hands_unchanged=false;
    projected=ubc::project_capture(source);check(state(projected,0x2001).known&&!state(projected,0x2001).hand_known,
          "lost hand witness retains passive learned state but blocks reserve permission");
    source=station_capture(Kind::saw);auto& raw=source.recipes[9];std::swap(raw.rows[0],raw.rows[2]);
    check(ubc::valid_crafting_snapshot(ubc::project_capture(source)),"station capture rows also map by exact symbol");
    raw.rows[0].reserved_tool=2;
    check(ubc::project_capture(source).issue==ubc::CraftingProjectionIssue::source,"unexpected reserve cannot inflate numerical maximum");
  }
  {
    auto source=station_capture(Kind::forge);source.diagnostic.issue=ubc::StationRecipeCaptureIssue::boundary;
    check(!ubc::valid_crafting_snapshot(ubc::project_capture(source)),"unverified capture boundary has no usable UI snapshot");
    auto snapshot=example(Kind::saw);state(snapshot,0x2001).maximum=8;
    check(!ubc::valid_crafting_snapshot(snapshot),"source maximum may never exceed copied numeric ingredient limit");
    snapshot=example(Kind::saw);state(snapshot,0)=ready_state(0);
    check(!ubc::valid_crafting_snapshot(snapshot),"foreign station learned flags are not accepted in unified snapshots");
    snapshot=example(Kind::saw);++state(snapshot,0x2001).generation;
    check(!ubc::valid_crafting_snapshot(snapshot),"fresh snapshot cannot smuggle stale or future per-recipe evidence");
  }
  {
    auto source=base_capture(Kind::stove);source.diagnostic.inventory_issue=ubc::PackedInventoryIssue::changed_during_read;
    auto snapshot=ubc::project_capture(source);
    check(ubc::valid_crafting_snapshot(snapshot)&&state(snapshot,0).known&&!state(snapshot,0).material_known,
          "inventory issue overrides inconsistent aggregate known flag without hiding genuine knowledge");
    auto station=station_capture(Kind::saw);station.recipes[9].material.count=0;
    check(ubc::project_capture(station).issue==ubc::CraftingProjectionIssue::source,"partial material result is not a full recipe capture");
    station=station_capture(Kind::saw);station.recipes[9].material.completion=ubc::station_recipe_catalog::Completion::end_production;
    check(ubc::project_capture(station).issue==ubc::CraftingProjectionIssue::source,"mismatched ready/end source contract is rejected");
    snapshot=example(Kind::saw);snapshot.hands.right_tmp_level=1;
    check(!ubc::valid_crafting_snapshot(snapshot),"temporary saw cannot satisfy nonconsumed native tool contract");
    snapshot=example(Kind::stove);check(!snapshot.hands.interact_item_known&&ubc::valid_crafting_snapshot(snapshot),
      "base route does not invent or require uncaptured anchor metadata");
  }
  {
    ubc::CraftingMaterialValue value;value.symbol=1;value.stored=INT32_MAX;value.consumed_reserve=INT32_MAX;
    value.tool_reserve=INT32_MAX;value.unit_cost=UINT32_MAX;value.unit_output=UINT32_MAX;
    const auto p=ubc::crafting_amount_preview(value,INT32_MAX);
    check(p.available_known&&p.ready_known&&p.ready_storage==2LL*INT32_MAX&&p.end_known&&p.end_storage==3LL*INT32_MAX,
          "independent preview arithmetic remains representable for hostile integer inputs");
    check(!ubc::crafting_amount_preview(value,UINT32_MAX).available_known,"out-of-range quantity does not produce a claimed preview");
    value={16609,6,0,-1,0,0,0};const auto tool=ubc::crafting_amount_preview(value,3);
    check(tool.available_known&&tool.available==6&&tool.ready_known&&tool.ready_storage==6&&!tool.end_known,
          "unknown reserved tool does not falsify known storage but leaves end return unknown");
    value={16604,6,-1,0,0,1,0};const auto flask=ubc::crafting_amount_preview(value,3);
    check(!flask.available_known&&!flask.ready_known&&!flask.end_known&&flask.required==3,
          "unknown consumed reserve cannot be replaced with a zero flask");
  }
  {
    auto source=extra_capture();auto projected=ubc::project_capture(source);
    check(ubc::valid_crafting_snapshot(projected),"thirteen actual extra snapshots project with full collection context");
    for(const auto& d:c::recipes) {
      const auto& s=state(projected,d.id);const bool extra=d.route==c::Route::alchemy_extra;
      check(s.known==extra,"extra projection never fills the six base potion recipes without their capture");
      if(extra)check(s.maximum==7&&s.context_known&&s.context_matches&&s.actor_matches&&
        ubc::crafting_block_reason(s)==ubc::CraftingBlockReason::none,"extra executable maximum remains evidence-bound");
    }
    auto changed=source;changed.recipes[0].profile_verified=false;changed.recipes[0].visible=false;
    projected=ubc::project_capture(changed);
    check(state(projected,0x4001).known&&!state(projected,0x4001).contract,"known extra recipe remains passively visible without a local profile");
    changed=source;changed.recipes[0].native_debit_verified=false;changed.recipes[0].maximum_executable=0;
    projected=ubc::project_capture(changed);
    check(state(projected,0x4001).known&&state(projected,0x4001).material_known&&!state(projected,0x4001).contract&&
      !state(projected,0x4001).maximum,"missing debit closure preserves stocks but cannot authorize manufacture");
    changed=source;changed.recipes[0].owner_conditions_match=false;
    projected=ubc::project_capture(changed);
    check(ubc::same_crafting_state(state(projected,0x4001),state(ubc::project_capture(source),0x4001)),
      "current category or owner conditions do not freeze a cross-category request");
    changed=source;changed.recipes[0].extra_flask_stack_verified=false;changed.recipes[0].maximum_executable=1;
    projected=ubc::project_capture(changed);
    check(ubc::valid_crafting_snapshot(projected)&&state(projected,0x4001).maximum==1,
      "packed-only extra flask restriction is never expanded to the mathematical seven");
    changed.recipes[0].maximum_executable=7;
    check(ubc::project_capture(changed).issue==ubc::CraftingProjectionIssue::source,
      "unverified extra debit stack cannot carry an executable maximum above one");
    for(unsigned mutation=0;mutation<16;++mutation) {
      changed=source;
      switch(mutation) {
        case 0:changed.context_after.game++;break;
        case 1:changed.context_after.mob++;break;
        case 2:changed.context_after.epoch++;break;
        case 3:changed.manager_after++;break;
        case 4:changed.serial=0;break;
        case 5:changed.boundary_verified=false;break;
        case 6:changed.context_unchanged=false;break;
        case 7:changed.recipes[0].serial++;break;
        case 8:changed.recipes[0].context.hero++;break;
        case 9:changed.recipes[0].id=changed.recipes[1].id;break;
        case 10:changed.recipes[0].quantity=1;break;
        case 11:changed.recipes[0].material.quantity=1;break;
        case 12:changed.recipes[0].material.count--;break;
        case 13:changed.recipes[0].rows[0].symbol=changed.recipes[0].rows[1].symbol;break;
        case 14:changed.recipes[0].rows[0].unit_cost++;break;
        case 15:changed.recipes[0].maximum_executable=8;break;
      }
      check(!ubc::valid_crafting_snapshot(ubc::project_capture(changed)),"mismatched extra collection identity, quantity or row cannot be admitted");
    }
    changed=source;changed.production_after=0;
    projected=ubc::project_capture(changed);
    check(state(projected,0x4001).known&&!state(projected,0x4001).context_known,
      "production instability retains learning but blocks the exact context proof");
    changed=source;changed.production_before=changed.production_after=0;
    projected=ubc::project_capture(changed);
    check(state(projected,0x4001).context_known&&!state(projected,0x4001).context_matches&&
      ubc::crafting_block_reason(state(projected,0x4001))!=ubc::CraftingBlockReason::none,
      "stable wrong production never authorizes an extra recipe");
    changed=source;changed.actor_before.self=changed.actor_after.self=99;
    projected=ubc::project_capture(changed);
    check(state(projected,0x4004).known&&state(projected,0x4004).actor_known&&!state(projected,0x4004).actor_matches,
      "HERO success recipes still require SELF for the original failure flask refund");
    changed.actor_before.matches=changed.actor_after.matches=false;
    changed.actor_purpose=ubc::AlchemyExtraActorPurpose::hero_preview;
    projected=ubc::project_capture(changed);
    check(!state(projected,0x4004).actor_matches,"preview intent alone is not physical Hero evidence");
    changed.physical_hero_preview_verified=true;
    projected=ubc::project_capture(changed);
    check(state(projected,0x4004).actor_matches&&changed.actor_before.self==99&&!changed.actor_before.matches,
      "physical Hero preview permits planning while raw SELF evidence remains false");
    changed.actor_purpose=ubc::AlchemyExtraActorPurpose::original_entry;
    check(!state(ubc::project_capture(changed),0x4004).actor_matches,"physical preview cannot admit the original callback");
    changed.actor_purpose=ubc::AlchemyExtraActorPurpose::unknown;
    check(!state(ubc::project_capture(changed),0x4004).actor_matches,"unknown actor purpose remains blocked");
    changed=source;changed.hands_after.hands.right.object++;
    projected=ubc::project_capture(changed);
    check(state(projected,0x4001).known&&!state(projected,0x4001).hand_known,
      "changed extra hand object cannot retain a consumed reserve");
    changed=source;changed.recipes[0].hand.right++;
    projected=ubc::project_capture(changed);
    check(!state(projected,0x4001).hand_known,"snapshot flask identity must match the common stable hand observation");
  }
  {
    auto normal=anvil_capture();auto ore=ore_capture();auto combined=ubc::project_capture(normal,ore);
    check(ubc::valid_crafting_snapshot(combined),"one combined anvil snapshot contains ordinary and ore routes");
    for(const auto& d:c::recipes) {
      const auto& s=state(combined,d.id);check(s.known==(d.station==Kind::anvil),"only thirteen physical anvil recipes receive evidence");
      if(d.station==Kind::anvil)check(s.maximum==7&&ubc::crafting_block_reason(s)==ubc::CraftingBlockReason::none,
        "right hammer, both temporary slots and material limits remain exact");
    }
    for(const auto self:{0u,99u}){
      auto idle_normal=normal;auto idle_ore=ore;
      const auto idle_actor=[&](auto& value){
        value.diagnostic.actor_before.self=self;value.diagnostic.actor_before.matches=false;
        value.diagnostic.actor_after=value.diagnostic.actor_before;
        for(auto& recipe:value.recipes)recipe.actor_matches=false;
      };
      idle_actor(idle_normal);idle_actor(idle_ore);
      const auto strict=ubc::project_capture(idle_normal,idle_ore);
      const auto preview=ubc::project_capture(idle_normal,idle_ore,true);
      check(ubc::valid_crafting_snapshot(strict)&&ubc::valid_crafting_snapshot(preview),
        "stable foreign or null SELF retains a bounded anvil capture");
      for(const auto& d:c::recipes)if(d.station==Kind::anvil){
        auto expected=state(strict,d.id);
        check(expected.actor_known&&!expected.actor_matches&&ubc::crafting_block_reason(expected)==ubc::CraftingBlockReason::actor_changed,
          "default ordinary and ore projections retain strict actual SELF proof");
        expected.actor_matches=true;
        check(ubc::same_crafting_state(expected,state(preview,d.id))&&ubc::crafting_block_reason(state(preview,d.id))==ubc::CraftingBlockReason::none,
          "physical Hero preview changes only actor admission, never material hand or context evidence");
      }
      check(!idle_normal.diagnostic.actor_before.matches&&!idle_ore.diagnostic.actor_before.matches,
        "preview never rewrites raw actor evidence into an original admission");
      auto changed_between=idle_ore;
      changed_between.diagnostic.actor_before.self=self?0u:99u;
      changed_between.diagnostic.actor_after=changed_between.diagnostic.actor_before;
      check(!ubc::valid_crafting_snapshot(ubc::project_capture(idle_normal,changed_between,true)),
        "physical preview cannot hide an actor change between ordinary and ore captures");
      for(unsigned bad=0;bad<6;++bad){
        auto invalid=idle_ore;
        if(bad==0)invalid.diagnostic.actor_after.self=123;
        if(bad==1)invalid.diagnostic.actor_before.read_valid=false;
        if(bad==2)invalid.diagnostic.actor_before.issue=ubc::runtime_script::Issue::symbol_signature;
        if(bad==3)invalid.diagnostic.actor_before.hero=invalid.diagnostic.actor_after.hero=123;
        if(bad==4)invalid.diagnostic.actor_before.requires_self=invalid.diagnostic.actor_after.requires_self=false;
        if(bad==5)invalid.recipes[0].actor_known=false;
        check(!state(ubc::project_capture(invalid,true),0x6001).actor_matches,
          "physical preview cannot hide unknown changed foreign or non-SELF actor metadata");
      }
    }
    auto other=ore;other.diagnostic.context_before.normal_weapons=1;other.diagnostic.context_before.ore_weapons=0;
    other.diagnostic.context_before.readings[2].value=1;other.diagnostic.context_before.readings[3].value=0;
    other.diagnostic.context_after=other.diagnostic.context_before;
    check(ubc::same_crafting_state(state(ubc::project_capture(other),0x6001),state(ubc::project_capture(ore),0x6001)),
      "category navigation changes no generic material or physical context binding");
    other=ore;other.diagnostic.context_after.start_smithing=0;
    check(!state(ubc::project_capture(other),0x6001).context_known,"changed START proof cannot be overwritten by aggregate context_unchanged");
    other=ore;other.diagnostic.actor_after.self=99;
    check(!state(ubc::project_capture(other),0x6001).actor_known,"ore SELF read is compared before and after capture");
    other=ore;other.hands.right_tmp_level=2;
    check(!ubc::valid_crafting_snapshot(ubc::project_capture(other)),"anvil tmp2 is not silently accepted as the proven tmp1 contract");
    other=ore;other.hands.interact_item=other.hands.hands.left.object;
    check(!ubc::valid_crafting_snapshot(ubc::project_capture(other)),"left tool cannot replace the actual hammer anchor");
    other=ore;other.key.generation++;
    check(!ubc::valid_crafting_snapshot(ubc::project_capture(normal,other)),"ordinary and ore captures must share one exact generation");
    other=ore;other.recipes[0].rows[0].stored++;
    check(!ubc::valid_crafting_snapshot(ubc::project_capture(normal,other)),"contradictory shared hot steel cannot be merged by recipe order");
    other=ore;std::swap(other.recipes[0].rows[0],other.recipes[0].rows[2]);
    check(ubc::valid_crafting_snapshot(ubc::project_capture(normal,other)),"independent target order remains symbol-based after merging");
    other=ore;other.recipes[0].rows[0].generated_hand=1;
    check(!ubc::valid_crafting_snapshot(ubc::project_capture(other)),"an ordinary ingredient cannot become a fabricated hand reserve");
  }
  {
    auto base=base_capture(Kind::alchemy);auto extra=extra_capture();
    // Reconcile one synthetic common inventory: combined potions consume the
    // basic output potions, so their existing stocks must agree across routes.
    for(auto& recipe:base.snapshot.recipes)for(auto& row:recipe.materials)
      for(const auto& snapshot:extra.recipes)for(std::size_t j=0;j<snapshot.material.count;++j)
        if(snapshot.rows[j].symbol==row.symbol&&snapshot.rows[j].stored>=0)row.stored=static_cast<std::int32_t>(snapshot.rows[j].stored);
    auto combined=ubc::project_capture(base,extra);
    check(ubc::valid_crafting_snapshot(combined),"base object-only hands merge with proven extra slot and anchor metadata");
    for(const auto& d:c::recipes)check(state(combined,d.id).known==(d.station==Kind::alchemy&&!d.spirit&&!d.tobacco),"all nineteen alchemy recipes share the same validated capture boundary");
    check(!ubc::project_capture(base).hands.interact_item_known&&combined.hands.interact_item_known,
      "full hand metadata comes only from the extra capture, never from the base route");
    auto wrong=extra;wrong.hands_before.hands.right.object++;wrong.hands_after=wrong.hands_before;
    check(!ubc::valid_crafting_snapshot(ubc::project_capture(base,wrong)),"different positive flask objects cannot be merged");
    wrong=extra;wrong.key.manager++;
    check(!ubc::valid_crafting_snapshot(ubc::project_capture(base,wrong)),"foreign manager cannot lend proof to base recipes");
    for(auto& row:base.snapshot.recipes[1].materials)if(row.symbol==16775)++row.stored;
    check(!ubc::valid_crafting_snapshot(ubc::project_capture(base,extra)),"known contradictory common stocks reject the collection merge");
  }
  std::printf("crafting_catalog_tests: %u checks, %u failures\n",checks,failures);
  return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
#endif
