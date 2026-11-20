// SPDX-License-Identifier: MIT
#include "ubc/stations/grind/crafting_grind_projection.hpp"
#include "ubc/core/crafting_handoff.hpp"
#include "ubc/core/crafting_result.hpp"
#include "ubc/ui/native_crafting_ui.hpp"
#include <cstdio>
#include <cstring>
#include <limits>

namespace {
namespace cat=ubc::crafting_catalog;namespace g=ubc::grind_recipe;
namespace nav=ubc::grind_recipe_navigation;namespace run=ubc::grind_item_runtime;
using Scope=ubc::grind_item_path::Scope;using ResultState=ubc::CraftingResultState;
unsigned checks=0,failures=0;
void check(bool value,const char* message){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",message);}}
// Only our synthetic values for the shared seam. The actual code,
// module and world readers have separate tests at the runtime adapter.
struct Fixture {
  std::size_t index=0;
  ubc::GrindRecipeCatalogueCapture source{};
  ubc::GrindProjectionInput input{};
  run::Evidence runtime{};
  explicit Fixture(std::size_t n=0,std::int32_t skill=98):index(n){
    const auto& recipe=g::recipes[n];
    source.key={1,10,7,0x3000,0x4000,0x5000,0x6000,11};input.key=source.key;
    input.scope=Scope::preview;input.physical_hero_preview_verified=true;input.station={true,true,false,1};
    source.hands.hands={{1,16613,1,0x9000},{0,-1,-1,0}};
    source.hands.left_slot_known=source.hands.right_slot_known=source.hands.interact_item_known=true;
    source.hands.left_tmp_level=1;source.hands.interact_item=0x9000;
    source.listed_object_count=3;source.listed_objects[0]=0xa000;source.listed_objects[1]=0xa400;source.listed_objects[2]=0xa800;
    source.listed_objects_known=true;source.listed_objects_key=source.key;
    auto& d=source.diagnostic;d.target_count=ubc::grind_recipe_target_count;d.listed_nodes=3;
    d.inventory_unchanged=d.item_fields_unchanged=d.item_class_verified=d.hands_unchanged=d.actor_unchanged=d.state_unchanged=true;
    d.actor_before.read_valid=d.actor_before.requires_self=d.actor_before.matches=true;
    d.actor_before.hero=d.actor_before.self=source.key.hero;d.actor_after=d.actor_before;
    d.state_before.read_valid=true;d.state_before.production=11;d.state_before.first_values=1;
    d.state_before.talent=1;d.state_before.skill=skill;
    for(std::size_t i=0;i<4;++i)d.state_before.readings[i].value=i==0?11:i==1||i==2?1:skill;
    d.state_after=d.state_before;
    for(std::size_t i=0;i<source.stocks.size();++i)source.stocks[i]={static_cast<std::int32_t>(ubc::grind_recipe_profile::items[i].function->symbol),0,0,0,0,0,false,false,false};
    for(std::size_t i=0;i<source.recipes.size();++i){auto& s=source.recipes[i];s.callback=g::recipes[i].function.symbol;
      s.evaluated=s.contract=s.learned=s.actor_known=s.actor_matches=s.state_known=s.context_matches=true;
      s.material.inventory_known=true;d.profiles[i].valid=d.profiles[i].local.matches=true;}
    const std::array<std::int32_t,3> symbols{{recipe.input,recipe.output,16613}};
    const std::array<std::int32_t,3> amounts{{7,3,5}};
    for(std::size_t i=0;i<3;++i)for(auto& s:source.stocks)if(s.symbol==symbols[i])s={symbols[i],amounts[i],0,amounts[i],1,0,false,false,false};
    input.physical.issue=ubc::GrindGuildIssue::none;input.physical.key=source.key;input.physical.hero=source.key.hero;
    input.physical.guild_address=source.key.hero+0x230;input.physical.guild=recipe.guilds[0];input.physical.layout=d.state_before.layout;
    runtime.item_path_verified=true;runtime.issue=run::Issue::none;runtime.scope=Scope::preview;
    runtime.modules.observed=runtime.constructors.values_verified=runtime.pool.verified=true;
    runtime.sort.stable=runtime.world.stable=runtime.renderer.verified=true;runtime.pool.required=2;
    runtime.inventory_insert.valid=true;
    runtime.inventory_insert.route=ubc::vob_inventory_insert_compat::Route::original;
    runtime.inventory_insert.slot_target=ubc::vob_inventory_insert_compat::original_target;
    // Synthetic observation of original-route values only, like the other
    // partial checks of shared values. A separate host tests actual 23B/747B
    // reads. This contributes neither original bytes nor a reader bypass.
    auto& unpack=runtime.unpack;namespace u=ubc::grind_zbug_unpack;
    unpack.valid=true;unpack.issue=u::Issue::none;unpack.route=u::Route::original;unpack.key=source.key;
    unpack.binding={{0x68000000,u::z::zbug_image_size,u::z::zbug_file_sha256,true},
      {0x69000000,u::z::union_image_size,u::z::union_file_sha256,true}};
    unpack.hero=source.key.hero;unpack.vtable=u::inventory_vtable;unpack.cookie=0xa17439b5;
    unpack.thunk_target=u::original_target;unpack.thunk_sha256=unpack.normalized_thunk_sha256=u::thunk.sha256;
    u::z::detail::word(unpack.header.data(),source.key.hero);
    unpack.fingerprint=u::detail::fingerprint(unpack);
    runtime.key=source.key;runtime.callback=recipe.function.symbol;runtime.quantity=1;runtime.physical=input.physical;
    runtime.writer.issue=ubc::GrindGuildIssue::none;runtime.writer.physical=input.physical;
    runtime.writer.actor_before=d.actor_before;runtime.writer.actor_after=d.actor_after;
    runtime.stock.issue=ubc::grind_item_path::Issue::none;runtime.stock.scope=Scope::preview;runtime.stock.quantity=1;
    runtime.stock.input.issue=ubc::GrindItemBindingIssue::none;runtime.stock.input.key=source.key;
    runtime.stock.input.callback=runtime.callback;runtime.stock.input.hands=source.hands;runtime.stock.input.item.object=0xa000;
    for(std::size_t i=0;i<3;++i)for(const auto& s:source.stocks)if(s.symbol==symbols[i])runtime.stock.materials[i]=s;
    runtime.stock.outputs[0].object=0xa400;runtime.stock.outputs[1].object=0xa800;
    runtime.expected_after_callback={{6,4,6}};bind();
  }
  void bind(){input.materials[index]=ubc::bind_grind_material(source,runtime,input.station);}
  ubc::CraftingSnapshot snapshot() const {return ubc::project_capture(source,input);}
  ubc::CraftingRequest request(std::uint32_t quantity=2) const {
    const auto s=snapshot();return {cat::recipes[cat::grind_begin+index].id,quantity,s.key,s.recipes[cat::grind_begin+index],s.hands};
  }
};
ubc::CraftingResultBinding binding(const ubc::CraftingRequest& r){return {r.key.generation,20,4,8,r.key.world,r.key.hero,r.key.station};}
ubc::CraftingResultSkillWitness skill(const ubc::CraftingRequest& r){
  auto after=r.material_snapshot.grind;after.skill=ubc::crafting_grind_plan(r.material_snapshot,r.quantity).skill_after;
  return {true,true,r.key,r.id,r.quantity,1,20,r.material_snapshot.grind,after};
}
ubc::CraftingResultDestroyWitness destroy(const ubc::CraftingRequest& r){return {true,true,r.key,r.id,r.quantity,2,21,16613,1};}
ubc::CraftingResultEndWitness end(const ubc::CraftingRequest& r){return {true,true,r.key.generation,9,21,r.key.world,r.key.hero,r.key.station};}
struct EndObservation {
  std::array<ubc::RecipeResultItem,3> stocks{};ubc::CraftingResultObservation value{};
  explicit EndObservation(const ubc::CraftingRequest& r){
    const auto p=ubc::crafting_grind_plan(r.material_snapshot,r.quantity);
    for(std::size_t i=0;i<3;++i)stocks[i]={p.after_end[i].symbol,p.after_end[i].amount};
    value.generation=r.key.generation;value.frame=22;value.stable_boundary_verified=value.identity_known=true;
    value.world=r.key.world;value.hero=r.key.hero;value.station_known=value.production_known=true;
    value.selection_known=value.selection_owner_verified=value.selection_done=value.collection_known=true;value.collection=5;
    value.stock_known=value.hands_known=true;value.hands.hands={{0,-1,-1,0},{0,-1,-1,0}};
    value.hands.left_slot_known=value.hands.right_slot_known=value.hands.interact_item_known=true;
    value.grind=skill(r).after;
  }
  const ubc::CraftingResultObservation& observation(){value.items={stocks.data(),stocks.size()};return value;}
};
void catalogue_and_projection(){
  check(cat::tobacco_begin==95&&cat::recipe_count==98&&cat::cool_begin==72&&cat::grind_begin==73,"22 recipes append without moving Cool or any existing index");
  check(static_cast<unsigned>(cat::Station::grind)==8&&static_cast<unsigned>(cat::Route::grind)==8,"station and route append8");
  for(std::size_t i=0;i<g::recipes.size();++i){Fixture f(i);const auto snapshot=f.snapshot();const auto& s=snapshot.recipes[cat::grind_begin+i];
    const auto& d=cat::recipes[cat::grind_begin+i];
    check(d.id==0x9001+i&&d.grind==&g::recipes[i]&&d.targets.count==3&&d.targets[2].symbol==16613,"canonical22 descriptors retain target order and IDs");
    check(f.input.materials[i].valid&&ubc::valid_crafting_snapshot(snapshot)&&s.known&&s.material_verified&&s.maximum==7,"typed N1 material observation projects all22 recipes");
    check(s.grind==ubc::CraftingGrindValues{true,1,g::recipes[i].guilds[0],98},"current original guild talent skill values are retained");
    check(ubc::crafting_quantity_block_reason(s,7)==ubc::CraftingBlockReason::none&&ubc::crafting_grind_plan(s,7).skill_after==100,"maximum and capped skill come from existing plan");
    check(cat::hand_amounts(d,16613).generated==1&&!cat::hand_amounts(d,16613).consumed&&s.materials[2].unit_cost==0,"one generated hand is not a Cold inventory cost");
    check(std::strcmp(ubc::ui_text::de::table.item(g::recipes[i].output),"Unbekannter Gegenstand")!=0,"every output has a verified original name transcription");
  }
  Fixture original;const auto id=original.request().id;
  {
    auto raw_only=original;raw_only.input.materials={};const auto snapshot=raw_only.snapshot();
    const auto& row=snapshot.recipes[cat::grind_begin];
    check(ubc::valid_crafting_snapshot(snapshot)&&row.known&&row.grind==original.request().material_snapshot.grind,
      "empty material binders preserve real knowledge without any per-recipe runtime checks");
    check(row.material_known&&row.materials[0].stored==7&&!row.material_verified&&!row.maximum,
      "cheap candidate projection preserves stock and cannot authorize an effect");
    for(auto& stock:raw_only.source.stocks)if(stock.symbol==g::recipes[0].input)
      stock={stock.symbol,0,0,0,0,0,false,false,false};
    const auto empty=raw_only.snapshot();const auto& missing=empty.recipes[cat::grind_begin];
    check(ubc::valid_crafting_snapshot(empty)&&missing.known&&missing.material_known&&missing.materials[0].stored==0,
      "materialless learned profile needs no native item runtime observation");
    check(!missing.material_verified&&!missing.maximum&&ubc::crafting_amount_preview(missing.materials[0],1).missing==1,
      "materialless row remains blocked and displays actual missing ingredient");
  }
  for(unsigned fault=0;fault<36;++fault){auto f=original;
    switch(fault){
      case 0:f.runtime.item_path_verified=false;break;case 1:f.runtime.quantity=2;break;
      case 2:++f.runtime.key.frame;break;case 3:++f.runtime.stock.materials[0].total;break;
      case 4:++f.runtime.expected_after_callback[2];break;case 5:f.runtime.stock.input.item.object=0x9000;break;
      case 6:f.runtime.stock.outputs[0].object=0x9000;break;case 7:f.source.diagnostic.hand_inventory_alias=true;break;
      case 8:f.input.station.station_hand_end_verified=false;break;case 9:f.input.station.stable_boundary_verified=false;break;
      case 10:f.input.station.verified_left_tmp_level=2;break;case 11:f.runtime.physical.issue=ubc::GrindGuildIssue::changed;break;
      case 12:f.runtime.stock.input.hands.interact_item=0;break;case 13:++f.runtime.callback;break;
      case 14:f.runtime.scope=Scope::writer;break;case 15:f.runtime.stock.scope=Scope::writer;break;
      case 16:f.source.diagnostic.inventory_unchanged=false;break;case 17:f.source.diagnostic.hands_unchanged=false;break;
      case 18:f.source.diagnostic.actor_before.read_valid=false;break;case 19:f.source.diagnostic.profiles[0].valid=false;break;
      case 20:f.source.recipes[0].contract=false;break;case 21:++f.input.key.frame;break;
      case 22:f.input.physical_hero_preview_verified=false;break;case 23:++f.input.physical.key.frame;break;
      case 24:f.input.physical.guild=0;break;case 25:f.source.diagnostic.state_before.first_values=0;break;
      case 26:f.source.listed_objects_known=false;break;case 27:++f.source.listed_objects_key.frame;break;
      case 28:--f.source.listed_object_count;break;case 29:f.source.listed_objects[2]=0x9000;break;
      case 30:f.runtime.inventory_insert.valid=false;break;
      case 31:f.runtime.unpack.valid=false;break;
      case 32:f.runtime.unpack.binding.zbug.mapping_verified=false;break;
      case 33:f.runtime.unpack.binding.union_core.file_sha256[0]^=1;break;
      case 34:f.runtime.unpack.normalized_thunk_sha256[0]^=1;break;
      case 35:f.runtime.unpack.header[32]=1;break;
    }
    f.bind();const auto s=f.snapshot();
    check(!ubc::valid_crafting_snapshot(s)||ubc::crafting_quantity_block_reason(s.recipes[cat::grind_begin],1)!=ubc::CraftingBlockReason::none,"broken copied runtime or station evidence never admits a quantity");
  }
  for(unsigned fault=0;fault<9;++fault){auto f=original;auto& b=f.input.materials[0];
    switch(fault){case 0:++b.material.maximum;break;case 1:--b.material.maximum;break;case 2:b.item_path_quantity=7;break;
      case 3:++b.material.evidence.stocks[0].amount;break;case 4:++b.material.evidence.guild;break;
      case 5:++b.material.evidence.skill;break;case 6:++b.material.rows[2].stored;break;
      case 7:++b.key.frame;break;case 8:b.scope=Scope::writer;break;}
    check(!ubc::valid_crafting_snapshot(f.snapshot()),"projector rejects inflated clamped and mismatched material copies");
  }
  auto foreign=original;foreign.source.diagnostic.actor_before.self=0xdead;foreign.source.diagnostic.actor_before.matches=false;
  foreign.source.diagnostic.actor_after=foreign.source.diagnostic.actor_before;
  foreign.source.recipes[0].actor_matches=false;foreign.bind();
  check(foreign.snapshot().recipes[cat::grind_begin].material_verified,"physical HERO preview permits stable foreign SELF");
  foreign.input.scope=Scope::writer;
  check(!ubc::valid_crafting_snapshot(foreign.snapshot())||!foreign.snapshot().recipes[cat::grind_begin].actor_matches,"preview cannot lend SELF proof to writer");
  auto mixed=original.snapshot();auto& other=mixed.recipes[cat::grind_begin+10];other.grind.guild=6;other.known=true;
  check(!ubc::valid_crafting_snapshot(mixed),"one catalogue cannot contain invented per-recipe guild values");
  auto hidden=original;hidden.source.diagnostic.state_before.talent=0;hidden.source.diagnostic.state_after=hidden.source.diagnostic.state_before;
  hidden.input.materials={};const auto unlearned=hidden.snapshot();
  check(ubc::valid_crafting_snapshot(unlearned)&&!unlearned.recipes[cat::grind_begin].known,"unlearned original value remains excluded");
  check(cat::find(id)->grind!=nullptr,"canonical ID remains bound after local copy mutations");
  auto foreign_object=original;foreign_object.source.listed_objects[3]=0xb000;
  foreign_object.source.listed_object_count=4;foreign_object.source.diagnostic.listed_nodes=4;foreign_object.bind();
  check(foreign_object.input.materials[0].valid,"complete catalogue object list includes unrelated inventory items");
}
void navigation_and_session(){
  const std::array<nav::PageEvidence,7> pages{{{true,false,nav::Page::info,0},{true,true,nav::Page::info,0},
    {true,true,nav::Page::categories,0},{true,true,nav::Page::categories,20961},{true,true,nav::Page::categories,20962},
    {true,true,nav::Page::pal_mil,20961},{true,true,nav::Page::sld_djg,20962}}};
  for(std::size_t i=0;i<g::recipes.size();++i)for(const auto p:pages)for(const auto n:{1u,2u,7u}){
    Fixture f(i);const auto r=f.request(n);const auto page=ubc::crafting_page(p);
    check(ubc::valid_crafting_page(page)&&ubc::crafting_page(ubc::crafting_grind_page(page))==page,"page retains owner and guild-category availability");
    const auto plan=ubc::make_crafting_plan(page,r);
    const bool supported=p.page!=nav::Page::info||p.owner_present;
    check(plan.valid()==supported,"normal existing navigation decides missing owner and category back paths");
    if(supported){check(ubc::valid_crafting_plan(plan)&&plan.effect()->function_symbol==static_cast<std::int32_t>(g::recipes[i].function.symbol)&&
      plan.effect()->invocation==ubc::recipe_catalog::Invocation::choice,"all22 plans end at actual original choice callback");
      auto bad=plan;++bad.material_snapshot.grind.skill;
      // A different consistent value copy represents a different plan.
      // It cannot satisfy a request with the original skill values.
      ubc::CraftingHandoff h;ubc::CraftingHandoffObservation o;
      check(!h.begin(r,bad,o),"different plan knowledge cannot bind the old request");}
    check(!ubc::make_crafting_plan(page,r.id,n).valid(),"Grind graph never fabricates material evidence for old unbound overload");
  }
  Fixture f;const auto s=f.snapshot();ubc::CraftingSession session;check(session.open(s),"grind session opens known original recipes");
  session.observe(1,false,true);session.observe(2,false,true);
  check(!session.can_confirm()&&session.maximum()==7,"N1 row leaves arbitrary-quantity admission closed");
  const auto r=f.request(1);check(session.observe_quantity_check(r,2,true)&&session.can_confirm(),"explicit current N1 evidence opens only current quantity");
  check(session.preset(7),"maximum can be selected without clamp3");session.observe(3,false,true);session.observe(4,false,true);
  check(!session.observe_quantity_check(r,4,true)&&!session.can_confirm(),"old N1 proof does not authorize Nmax");
  check(session.observe_quantity_check(f.request(7),4,true)&&session.can_confirm(),"own current Nmax evidence allows confirmation");
  auto next=s;for(auto& state:next.recipes)if(state.grind.known)++state.grind.skill;
  check(!session.refresh(next)&&session.quantity()==1&&!session.can_confirm(),"skill changes discard selected quantity and its proof");
}
void result_contract(){
  for(std::size_t i=0;i<g::recipes.size();++i)for(const auto initial:{-2,0,98,99,100,101,INT32_MIN,INT32_MAX})for(const auto n:{1u,2u,7u}){
    Fixture f(i,initial);const auto r=f.request(n);ubc::CraftingResult result;
    check(result.begin(r,binding(r))&&result.restored(true),"all22 quantities begin with real copied raw progress values");
    const auto expected=initial>=100?initial:static_cast<std::int32_t>(std::min<std::int64_t>(100,std::int64_t{initial}+n));
    const auto witness=skill(r);check(witness.after.skill==expected,"negative skill and cap100 retain original semantics");
    EndObservation observed(r);check(result.skill_after(witness)&&result.destroy_after(destroy(r))&&result.end_after(end(r)),"skill Destroy and normal End are three positive distinct witnesses");
    check(result.observe(observed.observation())==ResultState::verified,"fresh later stock hands and skill complete the result");
    ubc::CraftingResultSuccess success;check(result.success(success)&&success.grind_before.skill==initial&&success.grind_after.skill==expected&&
      success.deltas[2].symbol==16613&&success.deltas[2].delta==0&&result.permits_next(),"Cold netzero never replaces Destroy and skill result is exported");
  }
  Fixture f;const auto r=f.request();
  for(unsigned missing=0;missing<4;++missing){ubc::CraftingResult result;EndObservation observed(r);
    check(result.begin(r,binding(r))&&result.restored(true),"missing witness fixture begins");
    if(missing!=0)result.skill_after(skill(r));if(missing!=1)result.destroy_after(destroy(r));if(missing!=2)result.end_after(end(r));
    if(missing==3)observed.value.grind={};
    check(result.observe(observed.observation())==ResultState::waiting&&!result.permits_next(),"correct balances alone do not replace any missing progress Destroy End or fresh skill witness");
    if(missing==0)result.skill_after(skill(r));if(missing==1)result.destroy_after(destroy(r));
    if(missing==2){auto late=end(r);late.frame=22;result.end_after(late);}
    if(missing==3)observed.value.grind=skill(r).after;
    ++observed.stocks[0].total;++observed.value.frame;
    check(result.observe(observed.observation())==ResultState::failed,"late witness cannot freeze an earlier matching balance");
  }
  for(unsigned bad=0;bad<13;++bad){ubc::CraftingResult result;check(result.begin(r,binding(r))&&result.restored(true),"negative skill fixture begins");auto w=skill(r);
    switch(bad){case 0:w.after_original=false;break;case 1:w.original_progress_verified=false;break;case 2:++w.key.hero;break;
      case 3:++w.recipe;break;case 4:++w.quantity;break;case 5:w.serial=0;break;case 6:w.frame=19;break;
      case 7:++w.before.skill;break;case 8:--w.after.skill;break;case 9:++w.after.guild;break;case 10:w.after.known=false;break;
      case 11:w.after.talent=0;break;case 12:result.skill_after(w);break;}
    check(!result.skill_after(w)&&result.state()==ResultState::failed,"wrong or duplicate original progress fails permanently");}
  for(unsigned bad=0;bad<4;++bad){ubc::CraftingResult result;result.begin(r,binding(r));result.restored(true);result.skill_after(skill(r));auto w=destroy(r);
    if(bad==0)w.instance=16612;if(bad==1)w.amount=2;if(bad==2)w.normal_remove_verified=false;if(bad==3)result.destroy_after(w);
    check(!result.destroy_after(w)&&result.state()==ResultState::failed,"Grind cannot borrow Cool Destroy or duplicate it");}
  {ubc::CraftingResult result;result.begin(r,binding(r));result.restored(true);result.skill_after(skill(r));result.destroy_after(destroy(r));result.end_after(end(r));
    EndObservation observed(r);--observed.value.grind.skill;check(result.observe(observed.observation())==ResultState::failed,"wrong fresh final skill is a visible contradiction");}
}
}
int main(){catalogue_and_projection();navigation_and_session();result_contract();
  std::printf("GrindRecipeCatalogueCapture bytes: %zu\n",sizeof(ubc::GrindRecipeCatalogueCapture));
  std::printf("crafting_grind_shared_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;}
