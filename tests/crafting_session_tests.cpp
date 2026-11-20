// SPDX-License-Identifier: MIT
#include "ubc/crafting_session.hpp"
#define UBC_CRAFTING_FIXTURE_ONLY
#include "crafting_catalog_tests.cpp"
int main() {
  using namespace crafting_test;using Phase=ubc::SelectionPhase;using Reason=ubc::CraftingBlockReason;
  for(const auto& d:c::recipes) {
    auto snapshot=example(d.station,7,d.grind?d.grind->guilds[0]:1);ubc::CraftingSession session;std::uint64_t frame=0;
    check(session.phase()==Phase::inactive&&session.open(snapshot),"each catalogue family opens one value-only session");
    check(!session.confirm(),"opening cannot consume a held confirm event");neutral(session,frame);
    if(session.selected()->id!=d.id) {
      check(session.select_recipe(d.id)&&!session.armed(),"selecting another visible recipe resets release gate");neutral(session,frame);
    }
    check(session.selected()->id==d.id&&session.quantity()==1&&session.maximum()==7&&session.can_confirm(),
          "all fifty-one canonical fixture states support the same bounded selector");
    check(session.preset(7)&&!session.preset(1),"one preset activation consumes its edge");neutral(session,frame);
    check(session.confirm()&&!session.take(snapshot,true),"confirmed job still waits for release");
    session.observe(++frame,false,true);check(!session.take(snapshot,true),"event frame cannot count as first neutral frame");
    session.observe(++frame,false,true);session.observe(frame,false,true);
    check(!session.take(snapshot,true),"duplicate same-frame polls cannot supply second neutral frame");
    session.observe(++frame,false,true);check(!session.take(snapshot,false),"external handoff boundary is independently required");
    if(d.bow||d.cool||d.grind){
      check(!session.take(snapshot,true),"Bow release also waits for the current quantity proof");
      check(session.observe_quantity_check({d.id,session.quantity(),session.preview().key,*session.selected(),session.preview().hands},frame,true),
        "current Bow proof can authorize the otherwise ready request");
    }
    const auto request=session.take(snapshot,true);
    check(request&&request->id==d.id&&request->quantity==7&&request->key==snapshot.key&&
      ubc::same_crafting_state(request->material_snapshot,state(snapshot,d.id))&&ubc::same_crafting_hands(request->hands,snapshot.hands),
      "request owns recipe quantity context materials and exact available hand evidence");
    check(session.phase()==Phase::dispatched&&!session.take(snapshot,true),"take consumes intent before any native effect");
    session.original(ubc::SelectionIssue::context_changed);session.observe(++frame,false,false);neutral(session,frame);
    check(!session.confirm()&&!session.back()&&!session.take(snapshot,true)&&!session.open(snapshot)&&session.phase()==Phase::dispatched,
      "focus return stale reopen and fallback cannot resurrect consumed request");
  }
  for(const auto& d:c::recipes) {
    auto snapshot=example(d.station,7,d.grind?d.grind->guilds[0]:1);ubc::CraftingSession session;std::uint64_t frame=0;
    check(session.open(snapshot,d.id)&&session.selected()->id==d.id&&session.quantity()==1&&
      !session.armed()&&!session.confirm(),"preferred recipe only restores focus and quantity one, never confirmation");
    neutral(session,frame);session.preset(7);neutral(session,frame);session.confirm();neutral(session,frame);
    check(session.take(snapshot,true).has_value(),"preferred focus still needs fresh normal confirmation");
    ++snapshot.key.generation;for(auto& state:snapshot.recipes)if(state.evaluated)state.generation=snapshot.key.generation;
    check(session.open(snapshot,d.id)&&session.quantity()==1&&!session.armed()&&!session.confirm(),
      "next successful job reopens fresh at quantity one without held input repeat");
    ++snapshot.key.generation;for(auto& state:snapshot.recipes)if(state.evaluated)state.generation=snapshot.key.generation;
    if(d.grind){for(auto& value:snapshot.recipes)if(value.grind.known){value.grind.talent=0;value.known=false;}}
    else state(snapshot,d.id).known=false;
    check(session.open(snapshot,d.id)&&(!session.selected()||session.selected()->id!=d.id),
      "remembered focus cannot reveal a recipe that is no longer known");
  }
  for(const auto kind:{Kind::stove,Kind::alchemy,Kind::leather,Kind::saw,Kind::forge,Kind::anvil,Kind::bow}) {
    auto snapshot=example(kind);ubc::CraftingSession session;std::uint64_t frame=0;
    const std::size_t wanted=kind==Kind::stove?3u:kind==Kind::alchemy?23u:kind==Kind::leather?9u:kind==Kind::saw?5u:kind==Kind::anvil?13u:kind==Kind::bow?20u:2u;
    check(session.open(snapshot)&&session.visible_count()==wanted&&session.selected_index()==0,"visible indices filter the correct station partition");
    neutral(session,frame);
    for(std::size_t i=1;i<wanted;++i) {
      check(session.select_relative(1)&&session.selected_index()==i,"relative navigation reaches every known recipe including ninth leather entry");
      neutral(session,frame);
    }
    check(!session.select_relative(1)&&!session.select_relative(0)&&session.visible(wanted)==nullptr,"visible selection never wraps or indexes past end");
    for(auto& value:snapshot.recipes)if(value.known)value.known=false;
    check(!session.refresh(snapshot)&&session.phase()==Phase::original,"loss of selected knowledge returns without substituting another recipe");
    ubc::CraftingSession blank;frame=0;check(blank.open(snapshot)&&!blank.selected()&&!blank.visible_count()&&!blank.maximum(),"empty learned set is valid explicit menu state");
    neutral(blank,frame);check(!blank.confirm()&&blank.reason()==Reason::no_known_recipes,"unknown recipes stay hidden and cannot confirm");
    const auto id=std::find_if(c::recipes.begin(),c::recipes.end(),[&](const auto& d){return d.station==kind;})->id;
    state(snapshot,id).known=true;
    check(!blank.refresh(snapshot)&&blank.selected()->id==id&&!blank.armed(),"new first learned recipe requires fresh input");
  }
  for(const auto& d:c::recipes)if(c::requires_context(d.route))for(bool pending:{false,true})for(bool unknown:{false,true}) {
    auto snapshot=example(d.station,7,d.grind?d.grind->guilds[0]:1);ubc::CraftingSession session;std::uint64_t frame=0;
    check(session.open(snapshot,d.id),"new route opens with explicit current context");neutral(session,frame);
    if(pending){session.confirm();neutral(session,frame);}
    auto& selected=state(snapshot,d.id);selected.context_matches=false;if(unknown)selected.context_known=false;
    check(!session.refresh(snapshot)&&!session.can_confirm()&&!session.armed(),
      "changed or unknown new-route category context disarms and cancels pending intent");
    neutral(session,frame);check(!session.take(snapshot,true),"old new-route confirmation never survives context loss");
  }
  for(unsigned mode=0;mode<8;++mode) {
    auto snapshot=example(Kind::saw);auto& s=state(snapshot,0x2001);Reason reason=Reason::none;
    switch(mode) {
      case 0:s.contract=false;reason=Reason::contract_unverified;break;
      case 1:s.actor_known=s.actor_matches=false;reason=Reason::actor_unknown;break;
      case 2:s.actor_matches=false;reason=Reason::actor_changed;break;
      case 3:s.material_known=false;reason=Reason::material_unknown;break;
      case 4:s.hand_known=false;reason=Reason::hand_unknown;break;
      case 5:s.material_verified=false;reason=Reason::material_unverified;break;
      case 6:s.maximum=0;s.materials[0].stored=0;reason=Reason::missing_material;break;
      default:s.maximum=0;for(auto& row:s.materials)if(row.unit_output)row.stored=INT32_MAX;reason=Reason::output_capacity;break;
    }
    ubc::CraftingSession session;std::uint64_t frame=0;
    check(session.open(snapshot)&&session.visible_count()==5&&session.reason()==reason,"known blocked recipes stay visible with distinct missing evidence");
    neutral(session,frame);check(!session.can_confirm()&&!session.confirm(),"no missing gate is promoted by UI interaction");
    neutral(session,frame);check(!session.take(snapshot,true),"blocked visibility never emits a request");
  }
  for(bool pending:{false,true})for(unsigned mode=0;mode<9;++mode) {
    auto snapshot=example(Kind::saw);ubc::CraftingSession session;std::uint64_t frame=0;
    session.open(snapshot);neutral(session,frame);session.preset(7);neutral(session,frame);
    if(pending){session.confirm();neutral(session,frame);}
    auto& s=state(snapshot,0x2001);
    switch(mode) {
      case 0:++s.materials[0].stored;break;
      case 1:--s.maximum;break;
      case 2:s.contract=false;break;
      case 3:s.actor_matches=false;break;
      case 4:s.hand_known=false;break;
      case 5:s.material_verified=false;break;
      case 6:state(snapshot,0x2002).known=false;break;
      case 7:snapshot.hands.hands.right.object+=16;snapshot.hands.interact_item+=16;break;
      default:s.material_known=false;break;
    }
    check(!session.refresh(snapshot)&&session.phase()==Phase::selecting&&!session.armed(),"evidence amount or hand-identity change cancels pending and disarms selection");
    neutral(session,frame);check(!session.take(snapshot,true),"refresh never silently recreates prior confirmation");
  }
  for(unsigned mode=0;mode<6;++mode) {
    auto snapshot=example();ubc::CraftingSession session;std::uint64_t frame=0;
    session.open(snapshot);neutral(session,frame);session.confirm();neutral(session,frame);
    switch(mode) {
      case 0:++snapshot.key.world;break;case 1:++snapshot.key.hero;break;case 2:++snapshot.key.station;break;
      case 3:++snapshot.key.thread;break;case 4:++snapshot.key.manager;break;default:++snapshot.key.generation;break;
    }
    check(!session.take(snapshot,true)&&session.phase()==Phase::original,"any bound context change rejects before request consumption");
  }
  for(bool ready:{false,true}) {
    auto snapshot=example();ubc::CraftingSession session;std::uint64_t frame=0;
    session.open(snapshot);neutral(session,frame);session.confirm();if(ready)neutral(session,frame);
    session.observe(++frame,false,false);check(session.phase()==Phase::selecting&&session.issue()==ubc::SelectionIssue::focus_lost,"focus loss cancels confirmed intent in releasing and ready");
    neutral(session,frame);check(!session.take(snapshot,true),"focus return never auto-retries");
    check(session.confirm(),"explicit new confirm is possible after fresh neutral frames");
    check(session.back()&&session.phase()==Phase::original&&!session.take(snapshot,true),"Back cancels pending before material effect even while confirm event is latched");
    check(!session.return_ready(),"return to original menu waits for held-input release");neutral(session,frame);
    check(session.return_ready(),"original input resumes after neutral boundary");
  }
  {
    auto snapshot=example();ubc::CraftingSession session;std::uint64_t frame=0;
    session.open(snapshot);neutral(session,frame);session.confirm();neutral(session,frame);
    std::swap(state(snapshot,0).materials[0],state(snapshot,0).materials[2]);
    check(session.refresh(snapshot)&&session.take(snapshot,true).has_value(),"identical symbol quantities in a new order are not a false stock change");
    auto copy=state(snapshot,0);copy.materials[0].symbol=copy.materials[1].symbol;
    check(!ubc::same_crafting_state(copy,state(snapshot,0))&&!ubc::same_crafting_state(state(snapshot,0),copy),"resource equality rejects duplicates in both directions");
  }
  for(const auto maximum:{0u,1u,4u,5u,9u,10u,23u,static_cast<unsigned>(INT32_MAX)}) {
    auto snapshot=example();state(snapshot,0)=ready_state(0,maximum);
    ubc::CraftingSession session;std::uint64_t frame=0;session.open(snapshot);neutral(session,frame);
    for(const auto n:{0u,1u,5u,10u,maximum,UINT32_MAX}) {
      const auto prior=session.quantity();const bool accepted=n&&n<=maximum;
      check(session.preset(n)==accepted&&session.quantity()==(accepted?n:prior),"presets preserve exact allowed values across numeric limits");neutral(session,frame);
    }
    check(session.adjust(INT32_MAX)==bool(maximum)&&session.quantity()==maximum,"large positive adjustment saturates at proven maximum");
    check(session.adjust(INT32_MIN)==bool(maximum)&&session.quantity()==(maximum?1u:0u),"large negative adjustment avoids integer wrap");
  }
  {
    auto snapshot=example();state(snapshot,0).materials[0].consumed_reserve=1;
    ubc::CraftingSession session;
    check(!session.open(snapshot)&&session.phase()==Phase::original&&!session.can_confirm(),"arbitrary reserve on unreserved item invalidates snapshot");
  }
  std::printf("crafting_session_tests: %u checks, %u failures\n",checks,failures);
  return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
