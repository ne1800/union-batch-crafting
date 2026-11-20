// SPDX-License-Identifier: MIT
#include "ubc/crafting_result.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace {
namespace c=ubc::crafting_catalog;
using State=ubc::CraftingResultState;
using Issue=ubc::CraftingResultIssue;
unsigned checks=0,failures=0;
void check(bool ok,const char* message) {
  ++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",message);}
}
struct Fixture {
  const c::Descriptor& recipe;
  ubc::CraftingRequest request{};
  ubc::CraftingResultBinding binding{};
  std::array<ubc::RecipeResultItem,c::target_capacity> ready{},ended{};
  explicit Fixture(const c::Descriptor& d,std::uint32_t n=2):recipe(d) {
    request.id=d.id;request.quantity=n;
    request.key={7,11,0x1000,0x2000,0x3000,0x4000,d.station};
    auto& state=request.material_snapshot;state=ubc::make_crafting_state(d.id);
    state.generation=7;state.evaluated=state.knowledge_read_valid=state.known=state.contract=true;
    state.material_known=state.material_verified=state.hand_known=true;
    state.actor_known=state.actor_matches=d.route!=c::Route::base;
    state.context_known=state.context_matches=true;state.maximum=9;
    if(d.spirit){state.branch_known=true;state.branch_value=1;state.branch_output=15741;}
    if(d.grind)state.grind={true,1,d.grind->guilds[0],98};
    for(std::size_t i=0;i<state.target_count;++i) {
      auto& row=state.materials[i];const auto hand=c::hand_amounts(d,row.symbol);
      row.unit_output=c::units(c::outputs_for(d,state.branch_known,state.branch_value),row.symbol);
      row.consumed_reserve=hand.consumed;row.tool_reserve=hand.tool;row.generated_prop=hand.generated;
      row.stored=static_cast<std::int32_t>((row.unit_cost>row.unit_minimum?row.unit_cost:row.unit_minimum)*9u)-hand.consumed;
      if(row.fixed_minimum)row.stored=static_cast<int>(row.fixed_minimum);
      if(row.unit_output)row.stored=3;
      if(hand.tool)row.stored=2;
      if(hand.generated&&!hand.stored_absent&&!row.unit_cost)row.stored=4;
      if(row.symbol==16607)row.stored=6;
      const auto expected=std::int64_t{row.stored}+hand.consumed-
        std::int64_t{row.unit_cost}*n+std::int64_t{row.unit_output}*n;
      ready[i]={row.symbol,expected};ended[i]={row.symbol,expected+hand.tool};
    }
    auto& h=request.hands;
    if(d.base)h.hands={{0,-1,-1,0},{1,d.base->hand.active_item_symbol,1,0x9000}};
    else if(d.station_descriptor) {
      using Source=ubc::station_recipe_catalog::HandSource;
      const auto make=[](const auto& want,std::uint32_t object) {
        return want.source==Source::empty?ubc::HandObjectSnapshot{0,-1,-1,0}:
          ubc::HandObjectSnapshot{1,want.symbol,static_cast<std::int32_t>(want.amount),object};
      };
      const auto& want=d.station_descriptor->hand;
      h.hands={make(want.left,0x8000),make(want.right,0x9000)};
      h.left_slot_known=h.right_slot_known=h.interact_item_known=true;
      h.left_tmp_level=want.left.source==Source::generated?1:0;
      h.right_tmp_level=want.right.source==Source::generated?2:0;
      h.interact_item=want.anchor==ubc::station_recipe_catalog::HandSide::left?h.hands.left.object:h.hands.right.object;
    } else if(d.alchemy_extra||d.spirit||d.tobacco) {
      h.hands={{0,-1,-1,0},{1,16604,1,0x9000}};
      h.left_slot_known=h.right_slot_known=h.interact_item_known=true;
      h.left_tmp_level=h.right_tmp_level=0;h.interact_item=h.hands.right.object;
    } else if(d.bow){
      h.hands={{0,-1,-1,0},{0,-1,-1,0}};h.left_slot_known=h.right_slot_known=h.interact_item_known=true;
    } else if(d.cool){
      h.hands={{1,16612,1,0x8000},{0,-1,-1,0}};h.left_slot_known=h.right_slot_known=h.interact_item_known=true;
      h.left_tmp_level=1;h.right_tmp_level=0;h.interact_item=0x8000;
    } else if(d.grind){
      h.hands={{1,16613,1,0x8000},{0,-1,-1,0}};h.left_slot_known=h.right_slot_known=h.interact_item_known=true;
      h.left_tmp_level=1;h.right_tmp_level=0;h.interact_item=0x8000;
    } else {
      const auto& want=d.anvil?d.anvil->hand:d.ore->hand;
      h.hands={{1,want.left.symbol,static_cast<std::int32_t>(want.left.amount),0x8000},
        {1,want.right.symbol,static_cast<std::int32_t>(want.right.amount),0x9000}};
      h.left_slot_known=h.right_slot_known=h.interact_item_known=true;
      h.left_tmp_level=want.left_tmp_level;h.right_tmp_level=want.right_tmp_level;
      h.interact_item=want.anchor==ubc::anvil_recipe_catalog::HandSide::left?h.hands.left.object:h.hands.right.object;
    }
    binding={7,100,20,4,request.key.world,request.key.hero,request.key.station};
  }
  ubc::CraftingResultObservation observation(bool end=false) const {
    ubc::CraftingResultObservation v;
    v.generation=binding.generation;v.frame=102;
    v.stable_boundary_verified=v.identity_known=v.station_known=v.production_known=true;
    v.world=binding.world;v.hero=binding.hero;v.station=end?0:binding.station;
    const auto production=recipe.base?recipe.base->production_context:recipe.station_descriptor?recipe.station_descriptor->production_context:
      recipe.grind?11:recipe.cool?13:recipe.bow?16:(recipe.spirit||recipe.tobacco)?4:recipe.alchemy_extra?recipe.alchemy_extra->production_context:recipe.anvil?recipe.anvil->production_context:recipe.ore->production_context;
    v.production=end?0:production;
    if(recipe.spirit)v.branch={true,true,binding.generation,recipe.id,request.quantity,request.material_snapshot.branch_value};
    if(recipe.grind){v.grind=request.material_snapshot.grind;v.grind.skill=ubc::crafting_grind_plan(request.material_snapshot,request.quantity).skill_after;}
    v.selection_known=v.selection_owner_verified=v.selection_open=v.selection_waiting=v.collection_known=true;
    v.collection=binding.collection+1;
    v.stock_known=v.hands_known=true;
    v.items={(end?ended:ready).data(),recipe.targets.count};v.hands=request.hands;
    if(end){v.selection_open=v.selection_waiting=false;v.selection_done=true;
      v.hands.hands={{0,-1,-1,0},{0,-1,-1,0}};v.hands.interact_item_known=true;v.hands.interact_item=0;}
    return v;
  }
  ubc::CraftingResultEndWitness witness() const {
    return {true,true,binding.generation,binding.end_sequence+1,101,binding.world,binding.hero,binding.station};
  }
  ubc::CraftingResultExtraRestore extra_restore() const {
    // Value-core evidence from a synthetic case. No claim about a runtime DLL.
    return {binding.generation,recipe.id,request.quantity,true,true};
  }
  bool restore(ubc::CraftingResult& result) const {
    if(!result.restored(true,extra_restore()))return false;
    if(recipe.grind){auto after=request.material_snapshot.grind;after.skill=ubc::crafting_grind_plan(request.material_snapshot,request.quantity).skill_after;
      return result.skill_after({true,true,request.key,request.id,request.quantity,1,100,request.material_snapshot.grind,after})&&
        result.destroy_after({true,true,request.key,request.id,request.quantity,2,101,16613,1});}
    return !recipe.cool||result.destroy_after({true,true,request.key,request.id,request.quantity,1,101,16612,1});
  }
};
const c::Descriptor& descriptor(c::Station station) {
  for(const auto& d:c::recipes)if(d.station==station)return d;
  std::abort();
}
void all_routes() {
  for(const auto& d:c::recipes)for(const auto n:{1u,2u,9u})for(const bool end:{false,true}) {
    Fixture f(d,n);ubc::CraftingResult result;ubc::CraftingResultSuccess success{};
    check(result.state()==State::idle&&result.permits_next(),"new adapter is idle");
    check(!result.success(success),"no receipt before a real result");
    check(result.begin(f.request,f.binding),"all catalogue routes bind their exact original material contract");
    check(!result.begin(f.request,f.binding)&&!result.permits_next(),"pending request cannot be replaced");
    const auto value=f.observation(end);
    auto early=value;early.frame=101;
    check(result.observe(early)==State::waiting,"neither bare callback quantities nor missing restore prove success");
    check(f.restore(result),"explicit bound restore and required extra writer proof recorded even for N1");
    if(end)check(result.end_after(f.witness()),"positive normal EndInteraction-after is bound");
    const auto expected=end||d.completion==c::Completion::ready_same_station?State::verified:State::waiting;
    check(result.observe(value)==expected,"only correct station completion semantics authorize success");
    if(expected!=State::verified){check(!result.success(success),"nonterminal hands never issue a receipt");continue;}
    check(result.success(success)&&success.result_verified&&success.restore_verified,"receipt needs both result and restore");
    check(success.receipt==f.binding.generation&&success.id==d.id&&success.quantity==n&&success.key==f.request.key,
      "receipt binds one exact request and monotone identity");
    check(success.stock_count==d.targets.count&&success.produced_count==c::outputs_for(d,f.request.material_snapshot.branch_known,f.request.material_snapshot.branch_value).count&&success.frame==value.frame,
      "success copies actual winning observation extent and frame");
    check(success.terminal==(end?ubc::CraftingResultTerminal::ended:ubc::CraftingResultTerminal::ready),"ready and normal end remain distinct");
    for(std::size_t i=0;i<success.stock_count;++i) {
      const auto& actual=(end?f.ended:f.ready)[i];
      check(success.stocks[i].symbol==actual.symbol&&success.stocks[i].stored==actual.total,"reported stocks are actual symbol-bound values");
      const auto old=f.request.material_snapshot.materials[i].stored;
      check(success.deltas[i].symbol==actual.symbol&&success.deltas[i].before==old&&
        success.deltas[i].after==actual.total&&success.deltas[i].delta==actual.total-old,"reported deltas use storage before and after, not a guessed inventory repair");
    }
    const auto outputs=c::outputs_for(d,f.request.material_snapshot.branch_known,f.request.material_snapshot.branch_value);
    for(std::size_t i=0;i<success.produced_count;++i)
      check(success.produced[i].symbol==outputs[i].symbol&&success.produced[i].amount==static_cast<std::int32_t>(outputs[i].units*n),
        "actual positive output delta preserves ammo lots and leather yields");
    if(success.quantity_receipt_required)check(!result.permits_next()&&result.acknowledge_quantity_receipt(success.receipt),"synthetic matching display receipt is additional to material success");
    check(result.timeout()==State::verified&&result.permits_next(),"verified receipt does not auto-retry or time out later");
    auto stale=value;stale.stock_known=false;
    check(result.observe(stale)==State::verified&&result.success(success),"later unknown reads do not rewrite the successful observation");
  }
}
void invalid_requests() {
  for(unsigned bad=0;bad<29;++bad) {
    Fixture f(descriptor(c::Station::stove));ubc::CraftingResult result;
    auto& r=f.request;auto& s=r.material_snapshot;
    switch(bad) {
      case 0:r.id=UINT32_MAX;break;
      case 1:r.quantity=0;break;
      case 2:r.quantity=10;break;
      case 3:r.quantity=UINT32_MAX;break;
      case 4:r.key.thread=0;break;
      case 5:r.key.manager=0;break;
      case 6:r.key.kind=c::Station::forge;break;
      case 7:s.id=1;break;
      case 8:s.generation++;break;
      case 9:s.evaluated=false;break;
      case 10:s.known=false;break;
      case 11:s.knowledge_read_valid=false;break;
      case 12:s.contract=false;break;
      case 13:s.material_known=false;break;
      case 14:s.material_verified=false;break;
      case 15:s.hand_known=false;break;
      case 16:s.maximum=0;break;
      case 17:s.maximum=UINT32_MAX;break;
      case 18:s.materials[0].stored=-1;break;
      case 19:s.materials[0].unit_cost++;break;
      case 20:s.materials[1].symbol=s.materials[0].symbol;break;
      case 21:s.target_count=std::numeric_limits<std::size_t>::max();break;
      case 22:r.hands.hands.right.object=0;break;
      case 23:r.hands.hands.left={1,16604,1,0x9000};break;
      case 24:f.binding.generation++;break;
      case 25:f.binding.world++;break;
      case 26:f.binding.hero++;break;
      case 27:f.binding.station++;break;
      case 28:f.binding.collection=0;break;
    }
    check(!result.begin(r,f.binding)&&result.state()==State::failed,"invalid fresh contract permanently fails before effect");
    Fixture other(descriptor(c::Station::leather));other.binding.generation=other.request.key.generation=other.request.material_snapshot.generation=8;
    check(!result.begin(other.request,other.binding)&&!result.permits_next(),"route switch cannot bypass a global failure");
  }
  for(const auto kind:{c::Station::leather,c::Station::saw,c::Station::forge})for(unsigned bad=0;bad<5;++bad) {
    Fixture f(descriptor(kind));ubc::CraftingResult result;
    switch(bad){case 0:f.request.material_snapshot.actor_known=false;break;
      case 1:f.request.material_snapshot.actor_matches=false;break;
      case 2:f.request.hands.right_slot_known=false;break;
      case 3:f.request.hands.interact_item_known=false;break;
      case 4:f.request.hands.interact_item++;break;}
    check(!result.begin(f.request,f.binding)&&result.state()==State::failed,"station route requires actual actor slot and anchor proofs");
  }
}
void observations_and_failure() {
  for(const auto kind:{c::Station::stove,c::Station::saw})for(unsigned variant=0;variant<17;++variant) {
    Fixture f(descriptor(kind));ubc::CraftingResult result;
    check(result.begin(f.request,f.binding)&&result.restored(true),"ready-observation test binds");
    auto v=f.observation();bool fails=false;
    switch(variant){
      case 0:v.generation--;break;
      case 1:v.generation++;fails=true;break;
      case 2:v.frame=f.binding.start_frame;break;
      case 3:v.stable_boundary_verified=false;break;
      case 4:v.identity_known=false;break;
      case 5:v.world++;v.stock_known=false;fails=true;break;
      case 6:v.hero=0;fails=true;break;
      case 7:v.station++;fails=true;break;
      case 8:v.production++;fails=true;break;
      case 9:v.collection=f.binding.collection;break;
      case 10:v.selection_owner_verified=false;break;
      case 11:v.selection_waiting=false;break;
      case 12:v.stock_known=false;break;
      case 13:v.hands_known=false;break;
      case 14:v.hands.hands.right.state=-1;break;
      case 15:v.hands.hands.right.object++;fails=true;break;
      case 16:f.ready[0].total++;fails=true;break;
    }
    check(result.observe(v)==(fails?State::failed:State::waiting),"unknown stale ownerless and positively conflicting observations stay distinct");
    if(!fails)check(result.observe(f.observation())==State::verified,"later complete matching ready frame can finish waiting work");
  }
  for(const auto& d:c::recipes)for(unsigned bad=0;bad<8;++bad) {
    Fixture f(d);ubc::CraftingResult result;check(result.begin(f.request,f.binding)&&f.restore(result),"malformed terminal fixture starts");
    check(result.end_after(f.witness()),"terminal stock fixture receives genuine end witness");auto v=f.observation(true);
    switch(bad){case 0:v.items.data=nullptr;break;
      case 1:v.items.count--;break;case 2:v.items.count=SIZE_MAX;break;
      case 3:f.ended[0].total=-1;break;case 4:f.ended[0].total=std::int64_t{INT32_MAX}+1;break;
      case 5:f.ended[1].symbol=f.ended[0].symbol;break;
      case 6:f.ended[0].symbol=99999;break;case 7:f.ended[0].total++;break;}
    check(result.observe(v)==State::failed&&result.issue()==Issue::stock,"bad or foreign item list is never mapped by position");
  }
  for(const auto& d:c::recipes) {
    Fixture f(d);ubc::CraftingResult result;
    check(result.begin(f.request,f.binding),"restore regression starts");
    check(!result.restored(false)&&result.state()==State::failed&&result.issue()==Issue::restore,"unproved restore globally locks both routes");
    check(!result.restored(true)&&!result.end_after(f.witness()),"later positive calls cannot revive restore failure");
    ubc::CraftingResultSuccess out;check(!result.success(out),"restore failure cannot issue prior or new success");
  }
}
void lifecycle_and_copies() {
  for(const auto kind:{c::Station::stove,c::Station::saw}) {
    Fixture f(descriptor(kind));ubc::CraftingResult result;
    check(result.begin(f.request,f.binding),"known terminal contradiction before restore starts");
    auto v=f.observation();f.ready[0].total++;
    check(result.observe(v)==State::failed&&result.issue()==Issue::stock,
      "missing restore proof must not hide a positively wrong terminal stock");
  }
  for(const auto kind:{c::Station::stove,c::Station::alchemy,c::Station::leather,c::Station::saw,c::Station::forge}) {
    Fixture f(descriptor(kind));ubc::CraftingResult result;
    auto copy=f.request;check(result.begin(copy,f.binding)&&result.restored(true),"owned contract starts");
    copy.material_snapshot.materials={};copy.hands={};copy.key={};
    auto witness=f.witness();witness.normal_exit_verified=false;
    check(!result.end_after(witness)&&result.observe(f.observation(true))==State::waiting,"callback or abnormal exit cannot forge normal end");
    witness=f.witness();witness.frame=102;
    check(result.end_after(witness),"native witness later than processed frame is accepted");
    check(result.observe(f.observation(true))==State::waiting,"same-frame end observation is not completed native follow-up");
    auto v=f.observation(true);v.frame=103;
    std::reverse(f.ended.begin(),f.ended.begin()+static_cast<std::ptrdiff_t>(f.recipe.targets.count));
    check(result.observe(v)==State::verified,"later symbol-mapped observation uses the copied request");
    ubc::CraftingResultSuccess success;check(result.success(success),"owned success can be copied");
    const auto saved=success.stocks[0].stored;f.ended[0].total=999;
    check(result.success(success)&&success.stocks[0].stored==saved,"receipt never retains caller observation pointers");
    check(result.fail(Issue::restore)==State::failed&&!result.success(success),"late positive restoration contradiction revokes receipt globally");
  }
  Fixture first(descriptor(c::Station::stove));ubc::CraftingResult result;
  check(result.begin(first.request,first.binding)&&result.restored(true),"cross-route sequence starts");
  check(result.observe(first.observation())==State::verified,"first route ends at ready");
  Fixture second(descriptor(c::Station::saw));second.request.key.generation=second.request.material_snapshot.generation=8;
  second.binding.generation=8;second.binding.start_frame=103;
  check(result.begin(second.request,second.binding),"fresh generation across routes is allowed after verified result");
  check(result.timeout()==State::failed&&result.issue()==Issue::timeout&&!result.permits_next(),"explicit timeout permanently blocks all routes");
  check(!result.begin(first.request,first.binding),"no auto-retry after timeout");
  Fixture replay(descriptor(c::Station::stove));ubc::CraftingResult monotone;
  check(monotone.begin(replay.request,replay.binding)&&monotone.restored(true)&&monotone.observe(replay.observation())==State::verified,
    "generation replay test finishes first request");
  check(!monotone.begin(replay.request,replay.binding)&&monotone.state()==State::failed,"same generation cannot issue the same receipt twice");
}
void extreme_amounts_and_reserves() {
  for(const auto& d:c::recipes) {
    Fixture f(d);auto& state=f.request.material_snapshot;
    std::uint32_t maximum=INT32_MAX;
    for(std::size_t i=0;i<state.target_count;++i) {
      auto& row=state.materials[i];
      if(row.unit_cost){row.stored=INT32_MAX-row.consumed_reserve;
        maximum=std::min(maximum,static_cast<std::uint32_t>(INT32_MAX)/row.unit_cost);}
      if(row.unit_output)maximum=std::min(maximum,static_cast<std::uint32_t>(INT32_MAX-row.stored)/row.unit_output);
    }
    state.maximum=maximum;f.request.quantity=maximum;
    for(std::size_t i=0;i<state.target_count;++i) {
      const auto& row=state.materials[i];const auto total=std::int64_t{row.stored}+row.consumed_reserve-
        std::int64_t{row.unit_cost}*maximum+std::int64_t{row.unit_output}*maximum;
      f.ready[i]={row.symbol,total};f.ended[i]={row.symbol,total+row.tool_reserve};
    }
    ubc::CraftingResult result;ubc::CraftingResultSuccess success;
    check(maximum>1000000&&result.begin(f.request,f.binding)&&f.restore(result),"large valid quantity is bounded by real costs and output room, not arbitrary small cap");
    check(result.end_after(f.witness())&&result.observe(f.observation(true))==State::verified&&result.success(success),
      "int32-edge quantities preserve every route end balance without overflow");
    for(std::size_t i=0;i<success.produced_count;++i)
      check(success.produced[i].amount>0&&success.produced[i].amount<=INT32_MAX,"actual receipt amount stays native representable");
  }
  for(const auto kind:{c::Station::saw,c::Station::forge})for(unsigned variant=0;variant<4;++variant) {
    Fixture f(descriptor(kind));ubc::CraftingResult result;
    check(result.begin(f.request,f.binding)&&result.restored(true)&&result.end_after(f.witness()),"end anchor test binds actual hands");
    auto v=f.observation(true);
    switch(variant){case 0:v.hands.interact_item_known=false;break;
      case 1:v.hands.interact_item=0x9000;break;
      case 2:v.hands.hands.right=f.request.hands.hands.right;break;
      case 3:v.hands.hands.left={-1,-1,-1,0};break;}
    check(result.observe(v)==((variant==0||variant==3)?State::waiting:State::failed),
      "normal exit still waits for unknown hand or anchor and rejects positively occupied remnants");
  }
  {
    Fixture f(descriptor(c::Station::saw));ubc::CraftingResult result;
    check(result.begin(f.request,f.binding)&&result.restored(true),"saw-ready slot lifetime starts");
    auto v=f.observation();v.hands.right_tmp_level=1;
    check(result.observe(v)==State::failed&&result.issue()==Issue::hands,"returned temporary saw is not original reserved tool");
  }
  {
    Fixture f(descriptor(c::Station::alchemy),1);ubc::CraftingResult result;
    check(result.begin(f.request,f.binding)&&result.restored(true)&&result.end_after(f.witness()),"flask N1 accounting starts");
    for(auto& row:f.ended)if(row.symbol==16604)++row.total;
    check(result.observe(f.observation(true))==State::failed,"destroyed hand flask is not counted again as returned storage");
  }
  {
    Fixture f(descriptor(c::Station::saw));ubc::CraftingResult result;
    check(result.begin(f.request,f.binding)&&result.restored(true)&&result.end_after(f.witness()),"saw returned storage starts");
    for(auto& row:f.ended)if(row.symbol==16609)--row.total;
    check(result.observe(f.observation(true))==State::failed,"normal saw end must include exactly the one reserved tool return");
  }
  {
    Fixture f(descriptor(c::Station::forge));ubc::CraftingResult result;
    check(result.begin(f.request,f.binding)&&result.restored(true)&&result.end_after(f.witness()),"stored left forge prop starts");
    for(auto& row:f.ended)if(row.symbol==16666)--row.total;
    check(result.observe(f.observation(true))==State::failed,"loss of a permitted stored left prop is not hidden by generated-hand cleanup");
  }
  {
    Fixture f(descriptor(c::Station::saw));ubc::CraftingResult result;
    std::reverse(f.request.material_snapshot.materials.begin(),f.request.material_snapshot.materials.begin()+
      static_cast<std::ptrdiff_t>(f.recipe.targets.count));
    check(result.begin(f.request,f.binding)&&result.restored(true)&&result.observe(f.observation())==State::verified,
      "request material order is normalized by symbols before inner row validation");
  }
  {
    Fixture f(descriptor(c::Station::stove));ubc::CraftingResult result;
    check(result.begin(f.request,f.binding)&&result.observe(f.observation())==State::waiting,"complete good result before restore remains uncommitted");
    check(result.restored(true),"late positive restore does not create success on its own");
    auto v=f.observation();v.frame++;f.ready[0].total++;
    check(result.observe(v)==State::failed,"new wrong observation is revalidated after an earlier good unrestored candidate");
  }
}
void later_base_end_anchor() {
  for(const auto& d:c::recipes)if(d.route==c::Route::base){
    for(unsigned variant=0;variant<4;++variant){
      Fixture f(d);ubc::CraftingResult result;
      check(result.begin(f.request,f.binding)&&result.end_after(f.witness()),"base end anchor regression starts with real earlier witness");
      if(variant!=3)check(result.restored(true),"normal restore is independent of later anchor proof");
      auto v=f.observation(true);
      if(variant<2)v.hands.interact_item_known=false;
      else{v.hands.interact_item=0x9000;v.stock_known=false;}
      const auto expected=variant<2?State::waiting:State::failed;
      check(result.observe(v)==expected,"base route end needs current anchor and rejects known nonzero despite unknown stock or restore");
      ubc::CraftingResultSuccess success;
      check(!result.success(success),"old zero anchor in end witness cannot supply a later receipt");
      if(variant<2){
        v=f.observation(true);++v.frame;
        if(variant==1)++f.ended[0].total;
        check(result.observe(v)==(variant?State::failed:State::verified),
          "fresh known anchor rechecks whole result instead of reusing a prior uncommitted verified inner state");
      }else check(result.issue()==Issue::anchor,"positive nonzero anchor has a dedicated persistent failure");
    }
  }
  Fixture f(descriptor(c::Station::stove));ubc::CraftingResult ready;
  check(!f.observation().hands.interact_item_known&&ready.begin(f.request,f.binding)&&ready.restored(true)&&
    ready.observe(f.observation())==State::verified,"ready stove keeps its actually bound base hand contract without invented anchor proof");
}
bool new_route(const c::Descriptor& d){return d.route==c::Route::alchemy_extra||d.route==c::Route::anvil||d.route==c::Route::ore||d.route==c::Route::bow;}
void new_route_end_boundary(){
  std::size_t extras=0,anvils=0,ores=0,bows=0;
  for(const auto& d:c::recipes)if(new_route(d)){
    if(d.alchemy_extra)++extras;if(d.anvil)++anvils;if(d.ore)++ores;if(d.bow)++bows;
    check(!d.base&&!d.station_descriptor,"new routes retain their own canonical descriptor types");
    for(unsigned bad=0;bad<10;++bad){
      Fixture f(d);ubc::CraftingResult result;
      check(result.begin(f.request,f.binding)&&f.restore(result),"new-route normal-end witness fixture starts");
      auto w=f.witness();switch(bad){
        case 0:w.after_original=false;break;case 1:w.normal_exit_verified=false;break;case 2:w.generation--;break;
        case 3:w.generation++;break;case 4:w.sequence=f.binding.end_sequence;break;case 5:w.sequence=0;break;
        case 6:w.frame=f.binding.start_frame-1;break;case 7:w.world++;break;case 8:w.hero++;break;case 9:w.station++;break;
      }
      check(!result.end_after(w),"wrong or abnormal end witness is never adopted");
      check(result.observe(f.observation(true))==State::waiting,"zero mob/production/stock alone is no end witness");
      w=f.witness();w.frame=102;
      check(result.end_after(w)&&!result.end_after(w),"one fresh normal end witness is consumed exactly once");
      check(result.observe(f.observation(true))==State::waiting,"same-frame witness cannot prove completed end cleanup");
      auto v=f.observation(true);v.frame=103;
      check(result.observe(v)==State::verified,"later exact normal-end evidence releases one success");
    }
    for(unsigned variant=0;variant<28;++variant){
      Fixture f(d);ubc::CraftingResult result;
      check(result.begin(f.request,f.binding)&&f.restore(result)&&result.end_after(f.witness()),"new-route terminal evidence fixture starts");
      auto v=f.observation(true);bool fails=false;
      switch(variant){
        case 0:v.stable_boundary_verified=false;break;case 1:v.identity_known=false;break;
        case 2:v.station_known=false;break;case 3:v.production_known=false;break;
        case 4:v.hands_known=false;break;case 5:v.stock_known=false;break;
        case 6:v.hands.interact_item_known=false;break;case 7:v.hands.hands.left.state=-1;break;
        case 8:v.hands.hands.right.state=-1;break;case 9:v.world++;fails=true;break;
        case 10:v.hero++;fails=true;break;case 11:v.station=f.binding.station+1;fails=true;break;
        case 12:v.production=999;fails=true;break;case 13:v.hands.hands.left={1,16666,1,0x8000};fails=true;break;
        case 14:v.hands.hands.right={1,16604,1,0x9000};fails=true;break;
        case 15:v.hands.hands.left.object=0x8000;fails=true;break;
        case 16:v.hands.interact_item=0x9000;fails=true;break;
        case 17:v.stock_known=false;v.hands.interact_item=0x9000;fails=true;break;
        case 18:v.hands_known=false;f.ended[0].total++;fails=true;break;
        case 19:v.station=f.binding.station;break;case 20:v.production=f.observation().production;break;
        case 21:v.frame=100;break;case 22:v.frame=101;break;case 23:v.generation--;break;
        case 24:v.generation++;fails=true;break;case 25:v.items.count=0;fails=true;break;
        case 26:v.hands.hands.left={2,-1,-1,0x8000};fails=true;break;
        case 27:v.hands.hands.right.state=-2;fails=true;break;
      }
      check(result.observe(v)==(fails?State::failed:State::waiting),"unknown evidence waits while positively conflicting completed state fails closed");
      ubc::CraftingResultSuccess success;check(!result.success(success),"incomplete or rejected new route has no receipt");
      if(!fails){auto good=f.observation(true);good.frame=103;check(result.observe(good)==State::verified,"fresh complete end evidence can resolve waiting unknowns");}
      else check(result.observe(f.observation(true))==State::failed&&!result.permits_next(),"positive contradiction cannot be retried implicitly");
    }
    // Ready, collection and owner proof are never substituted for a new
    // normal end, even with a perfectly matching prospective final stock.
    {
      Fixture f(d);ubc::CraftingResult result;check(result.begin(f.request,f.binding)&&f.restore(result),"new route has no ready terminal");
      auto v=f.observation();v.items={f.ended.data(),d.targets.count};v.hands.hands={{0,-1,-1,0},{0,-1,-1,0}};
      v.hands.interact_item_known=true;v.hands.interact_item=0;v.collection=UINT64_MAX;
      check(result.observe(v)==State::waiting,"new complete collection does not stand in for end production");
      check(result.timeout()==State::failed&&!result.permits_next(),"missing real end eventually locks instead of reporting success");
    }
    for(unsigned kind=0;kind<3;++kind){
      Fixture f(d);ubc::CraftingResult result;
      check(result.begin(f.request,f.binding)&&result.end_after(f.witness()),"new unrestored result regression begins");
      auto v=f.observation(true);
      if(kind==0)f.ended[0].total++;
      if(kind==1){v.stock_known=false;v.hands.interact_item=0x9000;}
      if(kind<2)check(result.observe(v)==State::failed,"missing restore never conceals positive stock or anchor failures");
      else {
        check(result.observe(v)==State::waiting,"good unrestored final state is only a candidate");
        check(f.restore(result),"later checked restore is accepted");
        ++v.frame;f.ended[0].total++;
        check(result.observe(v)==State::failed,"candidate result is recomputed after later restore instead of cached as success");
      }
    }
    for(unsigned bad=0;bad<4;++bad){
      Fixture f(d);ubc::CraftingResult result;
      switch(bad){case 0:f.request.material_snapshot.context_known=false;break;case 1:f.request.material_snapshot.context_matches=false;break;
        case 2:f.request.material_snapshot.actor_known=false;break;default:f.request.material_snapshot.actor_matches=false;break;}
      check(!result.begin(f.request,f.binding)&&result.state()==State::failed,"new route requires current canonical actor and station context");
    }
  }
  check(extras==13&&anvils==5&&ores==8&&bows==20,"all forty-six new genuine descriptors are covered");
}
void extra_restore_contract(){
  for(const auto& d:c::recipes)if(d.alchemy_extra)for(auto quantity:{1u,2u,9u}){
    {
      Fixture f(d,quantity);ubc::CraftingResult result;
      check(result.begin(f.request,f.binding),"extra restoration negative test begins");
      check(!result.restored(true)&&result.state()==State::failed&&result.issue()==Issue::restore,
        "AmountPatch restored true alone never proves extra debit or ITEM restoration, including N1");
      check(!f.restore(result)&&!result.end_after(f.witness()),"later proof cannot revive a failed restoration job");
    }
    for(unsigned bad=0;bad<6;++bad){
      Fixture f(d,quantity);ubc::CraftingResult result;auto proof=f.extra_restore();
      check(result.begin(f.request,f.binding),"bound extra restore mismatch fixture begins");
      switch(bad){case 0:++proof.generation;break;case 1:++proof.recipe;break;case 2:++proof.quantity;break;
        case 3:proof.writer_result_verified=false;break;case 4:proof.item_restore_verified=false;break;case 5:proof.generation=0;break;}
      check(!result.restored(true,proof)&&result.issue()==Issue::restore,"extra receipt must match exact job recipe quantity and both real proofs");
    }
    {
      Fixture f(d,quantity);ubc::CraftingResult result;
      check(result.begin(f.request,f.binding)&&f.restore(result)&&result.end_after(f.witness()),"extra flask accounting positive fixture starts");
      for(auto& row:f.ended)if(row.symbol==16604)++row.total;
      check(result.observe(f.observation(true))==State::failed,"extra hand flask is consumed once and may not reappear as stored output");
    }
  }
  for(const auto& d:c::recipes)if(d.anvil||d.ore){
    Fixture f(d);ubc::CraftingResult result;
    check(result.begin(f.request,f.binding)&&result.restored(true)&&result.end_after(f.witness()),"anvil and ore keep the ordinary byte-restore API");
    for(auto& row:f.ended)if(row.symbol==16435)++row.total;
    check(result.observe(f.observation(true))==State::failed,"generated hammer cannot be returned into empty protected storage");
  }
}
}
int main(){all_routes();invalid_requests();observations_and_failure();lifecycle_and_copies();extreme_amounts_and_reserves();later_base_end_anchor();
  new_route_end_boundary();extra_restore_contract();
  std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;}
