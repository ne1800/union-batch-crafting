// SPDX-License-Identifier: MIT
#include "ubc/core/crafting_handoff.hpp"
#include <cstdio>
#include <utility>

namespace {
namespace nav=ubc::recipe_navigation;
namespace extra=ubc::station_recipe_navigation;
namespace cat=ubc::crafting_catalog;
using State=ubc::CraftingHandoffState;
using Decision=ubc::CraftingHandoffDecision;
using Page=nav::Page;
unsigned checks=0,failures=0;
void check(bool ok,const char* name){++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",name);}}
void page(ubc::CraftingHandoffObservation& value,ubc::CraftingPage current){
  value.page=current;value.page_known=true;value.choice_owner=-1;
  value.mode=ubc::recipe_catalog::Invocation::info;value.count=8;
  const ubc::recipe_catalog::Owner* owner=nullptr;
  if(current.kind==ubc::CraftingPageKind::alchemy){
    if(const auto* route=nav::route_for(static_cast<nav::Page>(current.value)))owner=route->owner;
  }else if(current.kind==ubc::CraftingPageKind::station){
    if(const auto* route=extra::route_for(static_cast<extra::Page>(current.value)))owner=route->owner;
  }else if(current.kind==ubc::CraftingPageKind::anvil){
    if(const auto* recipe=ubc::anvil_recipe_navigation::recipe_for(static_cast<ubc::anvil_recipe_navigation::Page>(current.value)))owner=&recipe->owner;
  }
  if(current.kind==ubc::CraftingPageKind::tobacco_choices)owner=&ubc::tobacco::owner;
  if(owner){value.mode=ubc::recipe_catalog::Invocation::choice;value.choice_owner=owner->instance;value.count=4;}
  if(current.kind==ubc::CraftingPageKind::grind&&ubc::crafting_grind_page(current).page!=ubc::grind_recipe_navigation::Page::info){
    value.mode=ubc::recipe_catalog::Invocation::choice;value.choice_owner=20957;value.count=13;}
}
void page(ubc::CraftingHandoffObservation& value,Page current){page(value,ubc::crafting_page(current));}
struct Example {
  ubc::CraftingRequest request;
  ubc::CraftingPlan plan;
  ubc::CraftingHandoffObservation observation;
  explicit Example(Page source=Page::health_02_choices,
      cat::RecipeId target=static_cast<cat::RecipeId>(ubc::recipe_catalog::RecipeId::mana_03),std::uint32_t quantity=2){
    const auto* descriptor=cat::find(target);
    request.id=target;request.quantity=quantity;request.key={1,2,3,4,5,6,descriptor->station};
    auto& material=request.material_snapshot;material=ubc::make_crafting_state(target);
    material.generation=1;material.evaluated=material.knowledge_read_valid=material.known=material.contract=true;
    material.material_known=material.material_verified=material.hand_known=true;material.maximum=23;
    material.actor_known=material.actor_matches=descriptor->route!=cat::Route::base;
    material.context_known=material.context_matches=cat::requires_context(descriptor->route);
    if(descriptor->spirit){material.branch_known=true;material.branch_value=1;material.branch_output=15741;}
    if(descriptor->grind)material.grind={true,1,descriptor->grind->guilds[0],98};
    for(std::size_t i=0;i<material.target_count;++i){
      auto& row=material.materials[i];const auto hand=cat::hand_amounts(*descriptor,row.symbol);
      row.unit_output=cat::units(cat::outputs_for(*descriptor,material.branch_known,material.branch_value),row.symbol);
      row.stored=static_cast<std::int32_t>((row.unit_cost>row.unit_minimum?row.unit_cost:row.unit_minimum)*material.maximum)-hand.consumed;
      if(row.fixed_minimum)row.stored=static_cast<int>(row.fixed_minimum);
      row.consumed_reserve=hand.consumed;row.tool_reserve=hand.tool;row.generated_prop=hand.generated;
    }
    ubc::CraftingPage start=ubc::crafting_page(source);
    auto& hands=request.hands;
    if(descriptor->route==cat::Route::base){
      hands.hands.left={0,-1,-1,0};hands.hands.right={1,descriptor->base->hand.active_item_symbol,1,900};
      if(descriptor->station==cat::Station::stove)start=ubc::crafting_stove_page();
    }else if(descriptor->tobacco||descriptor->route==cat::Route::alchemy_extra||descriptor->route==cat::Route::spirit){
      hands.hands.left={0,-1,-1,0};hands.hands.right={1,16604,1,900};
      hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;
      hands.interact_item=900;
    }else if(descriptor->bow){
      hands.hands={{0,-1,-1,0},{0,-1,-1,0}};hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;
      start=ubc::crafting_page(ubc::bow_recipe_navigation::Page::root);
    }else if(descriptor->cool){
      hands.hands={{1,16612,1,800},{0,-1,-1,0}};
      hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;
      hands.left_tmp_level=1;hands.right_tmp_level=0;hands.interact_item=800;
      start=ubc::crafting_page(ubc::cool_recipe_navigation::Page{true,true,true});
    }else if(descriptor->grind){
      hands.hands={{1,16613,1,800},{0,-1,-1,0}};
      hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;
      hands.left_tmp_level=1;hands.right_tmp_level=0;hands.interact_item=800;
      start=ubc::crafting_page(ubc::grind_recipe_navigation::PageEvidence{true,true,ubc::grind_recipe_navigation::Page::info,0});
    }else if(descriptor->route==cat::Route::anvil||descriptor->route==cat::Route::ore){
      hands.hands.left={1,16666,1,800};hands.hands.right={1,16435,1,900};
      hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;
      hands.left_tmp_level=hands.right_tmp_level=1;hands.interact_item=900;
      start=ubc::crafting_page(ubc::anvil_recipe_navigation::Page::harad_04);
    }else{
      const auto& contract=descriptor->station_descriptor->hand;
      const auto hand=[](const ubc::station_recipe_catalog::HandItem& want,std::uint32_t object){
        return want.source==ubc::station_recipe_catalog::HandSource::empty?ubc::HandObjectSnapshot{0,-1,-1,0}:
          ubc::HandObjectSnapshot{1,want.symbol,static_cast<std::int32_t>(want.amount),object};
      };
      hands.hands={hand(contract.left,800),hand(contract.right,900)};
      hands.left_slot_known=hands.right_slot_known=true;
      hands.left_tmp_level=contract.left.source==ubc::station_recipe_catalog::HandSource::generated?1:0;
      hands.right_tmp_level=contract.right.source==ubc::station_recipe_catalog::HandSource::generated?2:0;
      hands.interact_item_known=true;
      hands.interact_item=contract.anchor==ubc::station_recipe_catalog::HandSide::left?hands.hands.left.object:hands.hands.right.object;
      const auto* destination=extra::route_for(descriptor->station_descriptor->id);
      auto from=destination->root;
      for(const auto& route:extra::routes)if(route.station==destination->station&&route.choices!=destination->choices)from=route.choices;
      start=ubc::crafting_page(from);
    }
    plan=ubc::make_crafting_plan(start,request);
    observation.key=request.key;observation.view=42;observation.frame=100;observation.collection=10;
    observation.context_known=observation.complete=observation.ready=true;
    observation.outer_frame=observation.quiescent=observation.focused=true;
    observation.materials=material;observation.hands=request.hands;page(observation,plan.source);
  }
  ubc::CraftingHandoffOffer offer(std::size_t step,std::int32_t row=1) const {
    const auto& value=plan.steps[step];
    return {true,value.invocation,value.owner_instance,value.function_symbol,row,observation.collection};
  }
};
ubc::CraftingHandoffCallbackProof proof() { return {true,true,true,1,1,0,0,0}; }
ubc::CraftingHandoffTicket dispatched(ubc::CraftingHandoff& handoff, Example& example) {
  check(handoff.begin(example.request,example.plan,example.observation),"fixture begins a valid one-shot request");
  const auto ticket = handoff.ticket(example.observation,example.offer(0));
  check(ticket && handoff.dispatch(*ticket,example.observation,example.offer(0)),"fixture dispatches exactly its fresh ticket");
  return ticket ? *ticket : ubc::CraftingHandoffTicket{};
}
ubc::CraftingHandoffObservation callback_view(const Example& example) {
  auto result = example.observation;
  result.outer_frame = result.quiescent = result.ready = false;
  return result;
}

void successful_chains() {
  for (const auto& descriptor : cat::recipes) for (const auto quantity : {1u,7u}) {
    Example example(Page::health_02_choices,descriptor.id,quantity);
    ubc::CraftingHandoff handoff;
    check(handoff.begin(example.request,example.plan,example.observation),"all catalogue recipes have a valid direct or staged handoff");
    check(!handoff.begin(example.request,example.plan,example.observation),"begin cannot replace an active request");
    std::uint64_t previous_ticket = 0;
    unsigned effects = 0;
    for (std::size_t i = 0; i < example.plan.count; ++i) {
      const auto fresh_offer = example.offer(i);
      const auto ticket = handoff.ticket(example.observation,fresh_offer);
      check(ticket && ticket->id > previous_ticket && ticket->step == i,
            "each step obtains a new monotonic ticket with a freshly bound row");
      if (!ticket) break;
      previous_ticket = ticket->id;
      check(!handoff.ticket(example.observation,fresh_offer),"polling the same step cannot issue a second ticket");
      check(handoff.dispatch(*ticket,example.observation,fresh_offer) &&
            !handoff.dispatch(*ticket,example.observation,fresh_offer),"synthetic confirmation permission is consumed exactly once");
      const auto actual = callback_view(example);
      check(handoff.matches_pending(*ticket,actual,fresh_offer),"Info preflight can inspect without consuming callback execution");
      const auto execution = handoff.enter_callback(*ticket,actual,fresh_offer,proof());
      check(execution.decision == Decision::execute && execution.execution_id && handoff.state() == State::executing,
            "only canonical bound callback enters the current original function");
      if (example.plan.steps[i].is_effect()) ++effects;
      check(!handoff.callback_return({}) && handoff.state() == State::executing,
            "ordinary callback cannot complete a real wrapper");
      check(handoff.callback_return(execution) && !handoff.callback_return(execution),"wrapper return witness is consumed once");
      check(handoff.step_index() == i,"wrapper return never advances the selection chain");
      if (!example.plan.steps[i].is_effect()) {
        check(handoff.state() == State::awaiting_collection,"navigation waits for its native Collect/Open continuation");
        const auto old = example.observation;
        page(example.observation,example.plan.steps[i].after);
        ++example.observation.collection;
        handoff.observe(example.observation);
        check(handoff.state() == State::awaiting_collection,"new collection in the same frame cannot advance inside a wrapper");
        example.observation = old; ++example.observation.frame;
        page(example.observation,example.plan.steps[i].after);
        handoff.observe(example.observation);
        check(handoff.state() == State::awaiting_collection,"fresh frame with old collection is not a new selection page");
        ++example.observation.collection; handoff.observe(example.observation);
        check(handoff.state() == State::awaiting_step && handoff.step_index() == i+1,
              "only a later complete quiescent matching page unlocks one next step");
        handoff.observe(example.observation);
        check(handoff.step_index() == i+1,"repeated frame cannot skip another route step");
      }
    }
    check(effects == 1 && handoff.state() == State::result_pending,"one final effect transfers to separate result verification");
    auto next = example.request; ++next.key.generation;
    check(!handoff.ticket(example.observation,example.offer(example.plan.count-1)) &&
          !handoff.begin(next,example.plan,example.observation) && !handoff.acknowledge_result(false),
          "callback return is not permission to start another recipe");
    check(handoff.acknowledge_result(true) && handoff.state() == State::finished && !handoff.guards_input(example.observation),
          "explicit positive external result verification retires the chain");
    check(!handoff.begin(example.request,example.plan,example.observation),"completed request generation cannot be replayed");
  }
}

void stale_and_incomplete_boundaries() {
  {
    Example example; ubc::CraftingHandoff handoff; const auto old_ticket = dispatched(handoff,example);
    const auto original = handoff.enter_callback(old_ticket,callback_view(example),example.offer(0),proof());
    handoff.callback_return(original);
    ++example.observation.frame; ++example.observation.collection;
    page(example.observation,example.plan.steps[0].after);
    handoff.observe(example.observation);
    const auto fresh = example.offer(1);
    const auto next = handoff.ticket(example.observation,fresh);
    check(next && handoff.dispatch(*next,example.observation,fresh),"next step has its own dispatch permission");
    check(!handoff.callback_return(original) && handoff.state() == State::dispatched,
          "old return witness cannot complete a later pending native selection");
    check(handoff.enter_callback(old_ticket,callback_view(example),fresh,proof()).decision == Decision::block &&
          handoff.state() == State::quarantined,
          "replaying an earlier ticket blocks the newer step without running any callback");
  }
  for (unsigned missing = 0; missing < 2; ++missing) {
    Example example; ubc::CraftingHandoff handoff; const auto ticket = dispatched(handoff,example);
    const auto original = handoff.enter_callback(ticket,callback_view(example),example.offer(0),proof());
    handoff.callback_return(original);
    ++example.observation.frame; ++example.observation.collection;
    page(example.observation,example.plan.steps[0].after);
    if (missing == 0) example.observation.complete = false;
    else example.observation.ready = false;
    handoff.observe(example.observation);
    check(handoff.state() == State::awaiting_collection && !handoff.ticket(example.observation,example.offer(1)),
          "new page while collection is incomplete or native dialog animates never unlocks a step");
    example.observation.complete = example.observation.ready = true;
    handoff.observe(example.observation);
    check(handoff.state() == State::awaiting_step && handoff.step_index() == 1,
          "positive ready completion later in the outer frame permits exactly the planned step");
  }
  {
    Example example; ubc::CraftingHandoff handoff; dispatched(handoff,example); handoff.cancel();
    auto unknown = example.observation; unknown.context_known = false;
    check(handoff.guards_input(unknown),"unreadable ownership of the bound view retains its input quarantine");
    ++example.observation.frame; ++example.observation.collection;
    for (const auto invalid_count : {-1,257}) {
      example.observation.count = invalid_count; handoff.observe(example.observation);
      check(handoff.state() == State::quarantined,"invalid native row count cannot establish quiescent cancellation");
    }
    example.observation.count = 0; handoff.observe(example.observation);
    check(handoff.state() == State::cancelled,"complete empty ready collection can retire a canceled old ticket");
    example.observation.count = 4; ++example.request.key.generation; example.observation.key = example.request.key;
    example.request.material_snapshot.generation=example.request.key.generation;
    example.observation.materials=example.request.material_snapshot;
    check(handoff.begin(example.request,example.plan,example.observation),
          "only a fresh explicit request generation can start after positive cancellation retirement");
  }
}

void invalid_requests_and_plans() {
  for (unsigned bad = 0; bad < 14; ++bad) {
    Example example; ubc::CraftingHandoff handoff;
    switch (bad) {
      case 0: example.request.quantity = 0; break;
      case 1: example.request.quantity = 24; break;
      case 2: example.request.material_snapshot.known = false; break;
      case 3: example.request.material_snapshot.hand_known = false; break;
      case 4: example.request.material_snapshot.contract = false; break;
      case 5: example.request.key.hero = 0; break;
      case 6: example.plan.count = 0; break;
      case 7: example.plan.count = 6; break;
      case 8: example.plan.steps[0].function_symbol = 20340; break;
      case 9: example.plan.steps[0].after = ubc::crafting_page(Page::root); break;
      case 10: std::swap(example.plan.steps[1],example.plan.steps[2]); break;
      case 11: example.observation.outer_frame = false; break;
      case 12: example.observation.quiescent = false; break;
      case 13: example.observation.frame = 0; break;
    }
    check(!handoff.begin(example.request,example.plan,example.observation) && !handoff.active(),
          "invalid intent plan or initial boundary cannot create a ticket");
  }
  for (unsigned bad = 0; bad < 6; ++bad) {
    Example example(Page::unknown,static_cast<cat::RecipeId>(ubc::recipe_catalog::RecipeId::meat),2); ubc::CraftingHandoff handoff;
    switch (bad) {
      case 0: example.plan.source = ubc::crafting_page(Page::root); break;
      case 1: example.plan.steps[0].before = ubc::crafting_page(Page::root); break;
      case 2: example.plan.steps[0].invocation = ubc::recipe_catalog::Invocation::choice; break;
      case 3: example.plan.steps[0].owner_instance = 21017; break;
      case 4: example.plan.steps[0].function_symbol = 21019; break;
      case 5: example.plan.steps[0].kind = ubc::CraftingStepKind::open_owner; break;
    }
    check(!handoff.begin(example.request,example.plan,example.observation),"direct stove plan is canonical and cannot select an alternative effect");
  }
  {
    Example example; ubc::CraftingHandoff handoff;
    check(!handoff.begin(example.request,example.plan,example.observation,0),"zero timeout budget cannot start");
  }
}

void offer_and_dispatch_guards() {
  for (unsigned bad = 0; bad < 9; ++bad) {
    Example example; ubc::CraftingHandoff handoff; handoff.begin(example.request,example.plan,example.observation);
    auto offer = example.offer(0);
    switch (bad) {
      case 0: offer.valid = false; break;
      case 1: ++offer.owner; break;
      case 2: ++offer.function; break;
      case 3: offer.row = -1; break;
      case 4: offer.row = example.observation.count; break;
      case 5: ++offer.collection; break;
      case 6: offer.mode = ubc::recipe_catalog::Invocation::info; break;
      case 7: example.observation.count = 257; break;
      case 8: example.observation.complete = false; break;
    }
    check(!handoff.ticket(example.observation,offer) && handoff.state() == State::cancelled,
          "wrong owner function row collection mode or incomplete page cannot issue a ticket");
    check(!handoff.ticket(example.observation,example.offer(0)),"failed issue is never retried automatically");
  }
  for (unsigned bad = 0; bad < 8; ++bad) {
    Example example; ubc::CraftingHandoff handoff; handoff.begin(example.request,example.plan,example.observation);
    auto ticket = handoff.ticket(example.observation,example.offer(0));
    if (!ticket) { check(false,"dispatch mutation fixture issued"); continue; }
    auto fresh = example.offer(0);
    switch (bad) {
      case 0: ++ticket->id; break;
      case 1: ++ticket->step; break;
      case 2: ++ticket->offer.row; break;
      case 3: ++fresh.row; break;
      case 4: ++example.observation.frame; break;
      case 5: ++example.observation.collection; ++fresh.collection; break;
      case 6: example.observation.focused = false; break;
      case 7: example.observation.outer_frame = false; break;
    }
    check(!handoff.dispatch(*ticket,example.observation,fresh) && handoff.state() == State::cancelled,
          "dispatch requires the exact ticket and same fresh outer frame offer");
  }
}

void callback_guards() {
  for (unsigned bad = 0; bad < 20; ++bad) {
    Example example; ubc::CraftingHandoff handoff; auto ticket = dispatched(handoff,example);
    auto actual = callback_view(example); auto offer = example.offer(0); auto witness = proof();
    switch (bad) {
      case 0: witness.own_thread = false; break;
      case 1: witness.canonical_caller = false; break;
      case 2: witness.native_update = false; break;
      case 3: witness.update_depth = 2; break;
      case 4: witness.callback_depth = 2; break;
      case 5: witness.other_effect_depth = 1; break;
      case 6: witness.collection_depth = 1; break;
      case 7: witness.station_depth = 1; break;
      case 8: ++offer.owner; break;
      case 9: ++offer.function; break;
      case 10: ++offer.row; break;
      case 11: ++offer.collection; ++actual.collection; break;
      case 12: ++ticket.id; break;
      case 13: actual.focused = false; break;
      case 14: actual.context_known = false; break;
      case 15: actual.page_known = false; break;
      case 16: actual.choice_owner = 20337; break;
      case 17: actual.complete = false; break;
      case 18: ++actual.hands.hands.right.object; break;
      case 19: ++actual.materials.materials[0].stored; break;
    }
    const auto decision = handoff.enter_callback(ticket,actual,offer,witness);
    check(decision.decision == Decision::block && !decision.execution_id && handoff.state() == State::quarantined,
          "unproven callback fails closed without an execution or return witness");
    check(!handoff.callback_return(decision),"rejected wrapper cannot falsely finish a pending ticket");
    const auto restored = handoff.enter_callback(*handoff.current_ticket(),callback_view(example),example.offer(0),proof());
    check(restored.decision == Decision::block,"restoring matching values does not implicitly retry a canceled callback");
  }
  {
    Example example; ubc::CraftingHandoff handoff; const auto ticket = dispatched(handoff,example);
    const auto first = handoff.enter_callback(ticket,callback_view(example),example.offer(0),proof());
    auto nested = proof(); nested.callback_depth = 2;
    const auto second = handoff.enter_callback(ticket,callback_view(example),example.offer(0),nested);
    check(first.decision == Decision::execute && second.decision == Decision::block,
          "nested original wrapper cannot enter the same ticket twice");
    check(!handoff.callback_return(second) && handoff.state() == State::quarantined,
          "inner rejected return cannot retire the outer active execution");
    ubc::CraftingHandoffEndWitness end{example.request.key,42,101,true,true,true};
    check(!handoff.context_ended(end),"even a claimed end cannot retire an active outer callback");
    check(handoff.callback_return(first) && handoff.state() == State::quarantined,
          "outer return remains quarantined after reentry rejection");
  }
  {
    Example example; ubc::CraftingHandoff handoff; const auto ticket = dispatched(handoff,example);
    const auto first = handoff.enter_callback(ticket,callback_view(example),example.offer(0),proof());
    auto false_return = first; ++false_return.execution_id;
    check(!handoff.callback_return(false_return) && handoff.state() == State::executing,
          "wrong return witness cannot end an active callback");
    handoff.callback_return(first);
    check(handoff.enter_callback(ticket,callback_view(example),example.offer(0),proof()).decision == Decision::block,
          "repeated callback after return is blocked until native collection completes");
  }
}

void cancellation_and_progress() {
  for (unsigned change = 0; change < 9; ++change) {
    Example example; ubc::CraftingHandoff handoff; const auto ticket = dispatched(handoff,example);
    auto observed = example.observation; ++observed.frame;
    switch (change) {
      case 0: observed.focused = false; break;
      case 1: observed.context_known = false; break;
      case 2: ++observed.materials.materials[0].stored; break;
      case 3: ++observed.materials.materials[0].consumed_reserve; break;
      case 4: --observed.materials.maximum; break;
      case 5: observed.materials.known = false; break;
      case 6: ++observed.hands.hands.right.object; break;
      case 7: observed.hands.hands.left = {1,123,1,901}; break;
      case 8: handoff.cancel(); break;
    }
    handoff.observe(observed);
    check(handoff.state() == State::quarantined && handoff.guards_input(example.observation),
          "focus B unknown context stock and hand changes quarantine already dispatched selection");
    check(!handoff.ticket(example.observation,example.offer(0)) &&
          handoff.enter_callback(ticket,callback_view(example),example.offer(0),proof()).decision == Decision::block,
          "recovery of old values cannot authorize any further effect");
    auto stable = example.observation; ++stable.frame;
    handoff.observe(stable);
    check(handoff.state() == State::quarantined,"same old collection does not prove cancellation consumed IsDone");
    ++stable.collection; handoff.observe(stable);
    check(handoff.state() == State::cancelled && !handoff.guards_input(stable),
          "positive fresh quiet ready collection retires old ticket without another step");
  }
  for (unsigned change = 0; change < 4; ++change) {
    Example example; ubc::CraftingHandoff handoff; const auto ticket = dispatched(handoff,example);
    auto foreign = callback_view(example);
    switch (change) {
      case 0: ++foreign.view; break;
      case 1: ++foreign.key.world; break;
      case 2: ++foreign.key.hero; break;
      case 3: ++foreign.key.manager; break;
    }
    auto actual_offer = example.offer(0);
    check(handoff.enter_callback(ticket,foreign,actual_offer,proof()).decision == Decision::block,
          "foreign context never frees a delayed old bound callback");
    ++actual_offer.row; ++actual_offer.collection;
    check(handoff.enter_callback(ticket,foreign,actual_offer,proof()).decision == Decision::block,
          "changed row and collection cannot disguise the old bound callback");
    actual_offer.mode = actual_offer.mode == ubc::recipe_catalog::Invocation::info ? ubc::recipe_catalog::Invocation::choice : ubc::recipe_catalog::Invocation::info;
    check(handoff.enter_callback(ticket,foreign,actual_offer,proof()).decision == Decision::block,
          "changed mode cannot disguise the old bound owner and function");
    actual_offer = example.offer(0); ++actual_offer.function; actual_offer.valid = false;
    check(handoff.enter_callback(ticket,foreign,actual_offer,proof()).decision == Decision::block,
          "unreadable actual callback is no positive foreign callback evidence");
    actual_offer.valid = true;
    check(handoff.enter_callback(ticket,foreign,actual_offer,proof()).decision == Decision::ordinary,
          "positively identified different foreign callback passes through untouched");
    check(handoff.cancelled() && !handoff.guards_input(foreign),
          "foreign dialog is not captured even when it reuses the old native view address");
    auto end = ubc::CraftingHandoffEndWitness{example.request.key,42,101,true,true,true};
    ++end.key.world;
    check(!handoff.context_ended(end),"end witness for another world cannot retire the old context");
    end.key = example.request.key; end.confirmed = false;
    check(!handoff.context_ended(end),"unconfirmed context loss is not a positive end witness");
    end.confirmed = true;
    check(handoff.context_ended(end) && handoff.state() == State::cancelled,
          "positive bound context end releases quarantine without implicit retry");
  }
  {
    Example example; ubc::CraftingHandoff handoff;
    handoff.begin(example.request,example.plan,example.observation);
    const auto ticket = handoff.ticket(example.observation,example.offer(0));
    auto foreign = callback_view(example); ++foreign.key.world;
    check(ticket && handoff.enter_callback(*ticket,foreign,{},proof()).decision == Decision::ordinary && !handoff.active(),
          "positive foreign context before any native dispatch cancels only the own intent");
  }
  {
    Example example; ubc::CraftingHandoff handoff;
    handoff.begin(example.request,example.plan,example.observation,3);
    example.observation.frame += 4; handoff.observe(example.observation);
    check(handoff.state() == State::cancelled && handoff.issue() == ubc::CraftingHandoffIssue::timeout,
          "timeout before dispatch cancels intent without a pending engine ticket");
  }
  {
    Example example; ubc::CraftingHandoff handoff;
    handoff.begin(example.request,example.plan,example.observation,3);
    const auto ticket = handoff.ticket(example.observation,example.offer(0));
    handoff.dispatch(*ticket,example.observation,example.offer(0));
    example.observation.frame += 4; handoff.observe(example.observation);
    check(handoff.state() == State::quarantined && handoff.issue() == ubc::CraftingHandoffIssue::timeout,
          "timeout after dispatch retains protection against a delayed original effect");
  }
  for (unsigned change = 0; change < 3; ++change) {
    Example example; ubc::CraftingHandoff handoff; const auto ticket = dispatched(handoff,example);
    const auto entered = handoff.enter_callback(ticket,callback_view(example),example.offer(0),proof());
    handoff.callback_return(entered); ++example.observation.frame; ++example.observation.collection;
    page(example.observation,example.plan.steps[0].after);
    if (change == 0) page(example.observation,Page::root);
    if (change == 1) example.observation.outer_frame = false;
    if (change == 2) example.observation.quiescent = false;
    handoff.observe(example.observation);
    check(handoff.state() == State::quarantined && handoff.step_index() == 0,
          "wrong destination or non-outer boundary cannot advance even after real return");
  }
  {
    Example example(Page::unknown,static_cast<cat::RecipeId>(ubc::recipe_catalog::RecipeId::meat),2); ubc::CraftingHandoff handoff;
    const auto ticket = dispatched(handoff,example);
    const auto entered = handoff.enter_callback(ticket,callback_view(example),example.offer(0),proof());
    handoff.cancel();
    check(handoff.callback_return(entered) && handoff.state() == State::result_pending,
          "cancellation during already entered effect still requires its separate result verification");
    ubc::CraftingHandoffEndWitness end{example.request.key,42,101,true,true,true};
    check(!handoff.context_ended(end) && !handoff.acknowledge_result(false),
          "station end alone cannot claim material result success");
  }
}
void normalized_graphs() {
  std::array<ubc::CraftingPage,33> sources{};sources[0]=ubc::crafting_stove_page();
  std::size_t at=1;
  for(unsigned value=1;value<=12;++value)sources[at++]=ubc::crafting_page(static_cast<nav::Page>(value));
  for(unsigned value=1;value<=8;++value)sources[at++]=ubc::crafting_page(static_cast<extra::Page>(value));
  for(const auto source:ubc::anvil_recipe_navigation::pages)sources[at++]=ubc::crafting_page(source);
  for(const auto source:{ubc::bow_recipe_navigation::Page::root,ubc::bow_recipe_navigation::Page::upgrades})sources[at++]=ubc::crafting_page(source);
  for(const auto source:{ubc::cool_recipe_navigation::Page{true,true,false},ubc::cool_recipe_navigation::Page{true,true,true}})sources[at++]=ubc::crafting_page(source);
  check(at==sources.size(),"all supported original page families are enumerated independently");
  unsigned accepted=0,effects=0;
  for(const auto source:sources)for(const auto& descriptor:cat::recipes)for(const auto quantity:{1u,7u}){
    Example example(Page::health_02_choices,descriptor.id,quantity);
    example.plan=ubc::make_crafting_plan(source,example.request);page(example.observation,source);
    bool compatible=false;
    if(source.kind==ubc::CraftingPageKind::stove_info)compatible=descriptor.station==cat::Station::stove;
    if(source.kind==ubc::CraftingPageKind::alchemy)compatible=descriptor.station==cat::Station::alchemy;
    if(source.kind==ubc::CraftingPageKind::anvil)compatible=descriptor.station==cat::Station::anvil;
    if(source.kind==ubc::CraftingPageKind::bow)compatible=descriptor.station==cat::Station::bow;
    if(source.kind==ubc::CraftingPageKind::cool)compatible=descriptor.station==cat::Station::cool&&(quantity==1||source.value==3);
    if(source.kind==ubc::CraftingPageKind::station&&descriptor.station_descriptor){
      const auto* station=extra::station_for(static_cast<extra::Page>(source.value));
      compatible=station&&station->station==descriptor.station_descriptor->station;
    }
    check(example.plan.valid()==compatible&&ubc::valid_crafting_plan(example.plan)==compatible,
      "normalized graph accepts only compatible physical station families");
    ubc::CraftingHandoff handoff;
    check(handoff.begin(example.request,example.plan,example.observation)==compatible,
      "runtime request and normalized source route agree before any ticket");
    if(!compatible)continue;
    ++accepted;unsigned local_effects=0;
    for(std::size_t i=0;i<example.plan.count;++i){
      const auto offer=example.offer(i);const auto ticket=handoff.ticket(example.observation,offer);
      check(ticket&&handoff.dispatch(*ticket,example.observation,offer),"every normalized step dispatches one exact freshly resolved offer");
      if(!ticket)break;
      const auto callback=handoff.enter_callback(*ticket,callback_view(example),offer,proof());
      check(callback.decision==Decision::execute&&handoff.callback_return(callback),"every normalized original callback has one paired return");
      if(example.plan.steps[i].is_effect()){++local_effects;++effects;}
      else{
        ++example.observation.frame;++example.observation.collection;
        page(example.observation,example.plan.steps[i].after);handoff.observe(example.observation);
        check(handoff.state()==State::awaiting_step&&handoff.step_index()==i+1,"actual planned next page and fresh collection unlock next stage");
      }
    }
    check(local_effects==1&&handoff.state()==State::result_pending&&example.plan.effect()->after==ubc::CraftingPage{},
      "all normalized chains end at exactly one effect with no invented ready/end result");
  }
  check(accepted==927&&effects==927,"all 927 compatible page-recipe-quantity routes complete exactly once");
  for(const auto bad:std::array<ubc::CraftingPage,8>{{{}, {ubc::CraftingPageKind::stove_info,1},
    {ubc::CraftingPageKind::alchemy,0},{ubc::CraftingPageKind::station,0},
    {ubc::CraftingPageKind::alchemy,999},{ubc::CraftingPageKind::station,UINT32_MAX},
    {ubc::CraftingPageKind::anvil,0},{ubc::CraftingPageKind::anvil,999}}}){
    check(!ubc::valid_crafting_page(bad)&&!ubc::make_crafting_plan(bad,0,1).valid(),"unknown or malformed page tags cannot alias another family's valid page");
  }
  {
    Example e;auto plan=e.plan;plan.count=static_cast<std::size_t>(-1);
    check(!ubc::valid_crafting_plan(plan)&&!plan.effect(),"unbounded plan count is rejected without accessing steps");
    plan=e.plan;plan.steps[0].before.kind=ubc::CraftingPageKind::station;
    check(!ubc::valid_crafting_plan(plan),"same numeric page index in another domain cannot replace canonical source");
    plan=ubc::make_crafting_plan(ubc::crafting_stove_page(),static_cast<cat::RecipeId>(ubc::recipe_catalog::RecipeId::meat),2);
    plan.steps.back()={ubc::CraftingStepKind::effect,ubc::recipe_catalog::Invocation::choice,-7,-9,
      {ubc::CraftingPageKind::station,999},{ubc::CraftingPageKind::alchemy,999}};
    check(ubc::valid_crafting_plan(plan),"unused plan capacity carries no extra executable step");
  }
}

void new_contract_gates() {
  for(const auto& descriptor:cat::recipes)for(unsigned bad=0;bad<9;++bad){
    Example e(Page::health_02_choices,descriptor.id);ubc::CraftingHandoff handoff;
    switch(bad){
      case 0:++e.request.material_snapshot.generation;break;
      case 1:e.request.material_snapshot.evaluated=false;break;
      case 2:e.request.material_snapshot.knowledge_read_valid=false;break;
      case 3:e.request.material_snapshot.material_verified=false;break;
      case 4:e.request.material_snapshot.material_known=false;break;
      case 5:++e.request.hands.hands.right.object;break;
      case 6:e.request.material_snapshot.maximum=0;break;
      case 7:e.request.quantity=UINT32_MAX;break;
      case 8:e.request.material_snapshot.materials[1].symbol=e.request.material_snapshot.materials[0].symbol;break;
    }
    check(!handoff.begin(e.request,e.plan,e.observation)&&!handoff.active(),
      "confirmed capture generation, all material gates and confirmed hand identity are required for every recipe");
  }
  for(const auto& descriptor:cat::recipes)if(descriptor.route!=cat::Route::base)for(unsigned bad=0;bad<10;++bad){
    Example e(Page::root,descriptor.id);ubc::CraftingHandoff handoff;const auto ticket=dispatched(handoff,e);
    auto actual=callback_view(e);
    switch(bad){
      case 0:actual.materials.actor_known=false;break;
      case 1:actual.materials.actor_matches=false;break;
      case 2:actual.materials.material_verified=false;break;
      case 3:actual.hands.right_slot_known=false;break;
      case 4:++actual.hands.right_tmp_level;break;
      case 5:actual.hands.interact_item_known=false;break;
      case 6:++actual.hands.interact_item;break;
      case 7:++actual.materials.materials[0].tool_reserve;break;
      case 8:++actual.materials.materials[0].generated_prop;break;
      case 9:actual.hands.hands.left.state=-1;break;
    }
    check(handoff.enter_callback(ticket,actual,e.offer(0),proof()).decision==Decision::block&&handoff.cancelled(),
      "station actor, tool/prop accounting, slot lifetime and anchor changes cannot reach a native step");
    check(handoff.guards_input(actual),"changed own context remains guarded after native ticket dispatch");
  }
  {
    Example e;ubc::CraftingHandoff handoff;
    check(handoff.begin(e.request,e.plan,e.observation),"new generation frame monotonic case starts");
    e.observation.frame+=5;handoff.cancel();
    check(!handoff.active(),"undispatched intent can be cancelled immediately");
    ++e.request.key.generation;e.request.material_snapshot.generation=e.request.key.generation;
    e.observation.key=e.request.key;e.observation.materials=e.request.material_snapshot;e.observation.frame=99;
    check(!handoff.begin(e.request,e.plan,e.observation)&&handoff.issue()==ubc::CraftingHandoffIssue::boundary,
      "new generation does not excuse backwards global frame identity");
    ++e.request.key.generation;e.request.material_snapshot.generation=e.request.key.generation;
    e.observation.key=e.request.key;e.observation.materials=e.request.material_snapshot;
    check(!handoff.begin(e.request,e.plan,e.observation)&&handoff.issue()==ubc::CraftingHandoffIssue::boundary,
      "rejected backwards frame cannot lower the preserved frame boundary for another generation");
  }
  {
    Example e(Page::unknown,static_cast<cat::RecipeId>(ubc::recipe_catalog::RecipeId::meat),2);
    ubc::CraftingHandoff handoff;const auto ticket=dispatched(handoff,e);
    auto actual=callback_view(e);actual.frame+=5;
    const auto callback=handoff.enter_callback(ticket,actual,e.offer(0),proof());
    check(callback.decision==Decision::execute&&handoff.callback_return(callback)&&handoff.acknowledge_result(true),
      "later native callback frame is accepted within the deadline");
    ++e.request.key.generation;e.request.material_snapshot.generation=e.request.key.generation;
    e.observation.key=e.request.key;e.observation.materials=e.request.material_snapshot;
    check(!handoff.begin(e.request,e.plan,e.observation)&&handoff.issue()==ubc::CraftingHandoffIssue::boundary,
      "callback frame also remains part of the next generation's monotonic boundary");
  }
}
void unpaid_tobacco_callback(){
  Example e(Page::tobacco_info,ubc::tobacco::recipes[0].id,1);
  ubc::CraftingHandoff handoff;
  {
    ubc::CraftingHandoff denied;
    const auto ticket=dispatched(denied,e);
    const auto callback=denied.enter_callback(ticket,callback_view(e),e.offer(0),proof());
    denied.cancel(ubc::CraftingHandoffIssue::resources);
    check(denied.reject_unpaid_tobacco(callback)&&!denied.active(),
      "owner rejection before any script leaves the original menu usable on the same page");
  }
  const auto owner=dispatched(handoff,e);
  const auto opened=handoff.enter_callback(owner,callback_view(e),e.offer(0),proof());
  check(handoff.callback_return(opened),"deferred owner opens the original choices");
  ++e.observation.frame;++e.observation.collection;page(e.observation,e.plan.steps[0].after);
  const auto child=handoff.ticket(e.observation,e.offer(1));
  check(child&&handoff.dispatch(*child,e.observation,e.offer(1)),"unpaid child binds its own fresh ticket");
  if(!child)return;
  const auto execution=handoff.enter_callback(*child,callback_view(e),e.offer(1),proof());
  handoff.cancel(ubc::CraftingHandoffIssue::resources);
  auto wrong=execution;++wrong.ticket_id;
  check(!handoff.reject_unpaid_tobacco(wrong)&&handoff.reject_unpaid_tobacco(execution)&&
    !handoff.active()&&!handoff.current_ticket()&&!handoff.callback_return(execution),
    "only the same unpaid callback can finish without waiting for a result");
}
} // namespace

int main() {
  unpaid_tobacco_callback();successful_chains(); invalid_requests_and_plans(); offer_and_dispatch_guards();
  callback_guards(); cancellation_and_progress(); stale_and_incomplete_boundaries();
  normalized_graphs();new_contract_gates();
  std::printf("CraftingHandoff: %u checks, %u failures\n",checks,failures);
  return failures ? 1 : 0;
}
