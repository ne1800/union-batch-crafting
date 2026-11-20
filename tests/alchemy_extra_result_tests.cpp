// SPDX-License-Identifier: MIT
#include "ubc/alchemy_extra_result.hpp"
// Shared value fixture only. No test seam in production.
#define UBC_ALCHEMY_EXTRA_FIXTURE_ONLY
#include "alchemy_extra_snapshot_tests.cpp"
#include <type_traits>

namespace result_test {
namespace cat=ubc::alchemy_recipe_catalog;
namespace debit=ubc::alchemy_flask_debit;
using Stage=ubc::AlchemyExtraResultStage;
struct Flow {
  extra_test::Fixture fixture;
  ubc::AlchemyExtraSnapshot snapshot;
  debit::Job job{};
  debit::Admission admission{true,true,true,true,true,true};
  debit::Observation before{},original{},late{},extra{},terminal{};
  debit::EndWitness end{};
  explicit Flow(const cat::Descriptor& r,std::uint32_t quantity,std::uint64_t id=1,std::uint64_t serial=10):fixture(r) {
    fixture.input.serial=serial;snapshot=fixture.snapshot(quantity);
    job={snapshot.context,id,id,r.callback.function->symbol,static_cast<std::uint32_t>(r.owner.instance),quantity};
    before.context=job.context;before.scope={id,id,job.callback,1,0,0,true};before.serial=serial+1;
    before.native_contract=before.stable_read=true;before.production=4;before.invincible=1;
    before.hand=snapshot.hand;before.flask=snapshot.flask;
    before.parser={true,true,ubc::lhiver::game_parser,0x500000,ubc::lhiver::code_length,100,0,
      0x80000,job.context.hero,0x80000,0x81000,job.context.hero,job.context.hero,0x7000,0};
    before.stocks.count=r.targets.count;before.stocks.complete_unambiguous=true;
    for(std::size_t i=0;i<r.targets.count;++i)before.stocks.rows[i]={static_cast<std::uint32_t>(r.targets[i].symbol),snapshot.rows[i].stored,true};
    original=before;original.serial=serial+2;original.production=original.invincible=0;original.parser.pop_pos=200;
    original.parser.data_depth=debit::original_return_words(job.callback);original.parser.data_words_known=true;
    for(std::uint32_t i=0;i<original.parser.data_depth;++i)original.parser.data_words[i]=i%2?0x40u:1u;
    for(std::size_t i=0;i<r.targets.count;++i) {
      auto& row=original.stocks.rows[i];const auto symbol=static_cast<std::int32_t>(row.symbol);
      if(symbol!=16604)row.stored-=std::int64_t{cat::units(r.costs,symbol)}*quantity;
      row.stored+=std::int64_t{cat::units(r.outputs,symbol)}*quantity;
    }
    late=original;late.serial=serial+3;extra=original;extra.serial=serial+4;extra.scope.debit_depth=1;
    for(std::size_t i=0;i<extra.stocks.count;++i)if(extra.stocks.rows[i].symbol==16604)extra.stocks.rows[i].stored-=quantity-1;
    if(quantity>1)extra.flask.amount-=quantity-1;
    if(!extra.flask.amount) {extra.flask.object=0;extra.flask.listed_stacks=0;extra.flask.flags=0;}
    terminal=quantity==1?original:extra;terminal.serial=serial+6;terminal.context.mob=0;terminal.scope={};terminal.hand={true,true,0,0,0,0};
    end={true,id,id,serial+5,job.context};
  }
  bool start(ubc::AlchemyExtraResult& result) {return result.begin(snapshot,job,admission,before);}
  bool original_done(ubc::AlchemyExtraResult& result) {return result.original_returned(original,true,true);}
  bool debit_done(ubc::AlchemyExtraResult& result) {
    auto call=result.tracker_for_bridge().begin_debit(late);
    CHECK(call.valid && call.quantity==job.quantity-1);
    if(!call.valid)return false;
    if(!result.tracker_for_bridge().debit_returned(extra,true,true))return false;
    return result.acknowledge_extra_debit(extra);
  }
  void all_before_exit(ubc::AlchemyExtraResult& result) {CHECK(start(result));CHECK(original_done(result));if(job.quantity>1)CHECK(debit_done(result));}
};
void successful_paths() {
  for(const auto& recipe:cat::recipes)for(auto quantity:{1u,2u,6u}) {
    Flow f(recipe,quantity);ubc::AlchemyExtraResult result;
    CHECK(f.start(result));CHECK(result.status().stage==Stage::original_pending && !result.receipt().verified);
    CHECK(f.original_done(result));CHECK(result.status().original_success_verified && !result.receipt().verified);
    if(quantity==1) {
      CHECK(result.status().extra==ubc::AlchemyExtraDebitProof::not_required_for_one);
      CHECK(result.tracker_for_bridge().status().stage==debit::Stage::debit_verified);
    } else {
      CHECK(result.status().stage==Stage::extra_pending && result.status().extra==ubc::AlchemyExtraDebitProof::pending);
      CHECK(f.debit_done(result));CHECK(result.status().extra==ubc::AlchemyExtraDebitProof::verified);
    }
    CHECK(result.status().stage==Stage::closing && !result.status().normal_station_end_verified && !result.receipt().verified);
    CHECK(result.observe_completion(f.terminal,f.end,false)==Stage::closing);
    auto unknown=f.terminal;unknown.stable_read=false;CHECK(result.observe_completion(unknown,f.end,true)==Stage::closing);
    unknown=f.terminal;unknown.stocks.complete_unambiguous=false;CHECK(result.observe_completion(unknown,f.end,true)==Stage::closing);
    unknown=f.terminal;unknown.hand.known=false;CHECK(result.observe_completion(unknown,f.end,true)==Stage::closing);
    auto foreign=f.end;++foreign.job;CHECK(result.observe_completion(f.terminal,foreign,true)==Stage::closing);
    auto stale=f.terminal;stale.serial=f.original.serial;CHECK(result.observe_completion(stale,f.end,true)==Stage::closing);
    CHECK(result.observe_completion(f.terminal,f.end,true)==Stage::complete);
    const auto& receipt=result.receipt();CHECK(receipt.verified && receipt.quantity==quantity && receipt.id==recipe.id && receipt.count==recipe.targets.count);
    CHECK(result.status().original_success_verified && result.status().normal_station_end_verified);
    for(std::size_t i=0;i<receipt.count;++i) {
      const auto& row=receipt.rows[i];CHECK(row.actual==row.expected && row.expected==row.before+row.reserved_hand-row.consumed+row.produced);
      if(row.symbol==16604)CHECK(row.consumed==quantity && row.reserved_hand==1 && row.before==5 && row.actual==6-quantity);
      if(row.symbol==static_cast<std::uint32_t>(recipe.outputs[0].symbol))CHECK(row.produced==quantity && row.actual==2+quantity);
    }
    CHECK(result.observe_completion(f.terminal,f.end,true)==Stage::complete); // No repeated effect.
    Flow next(recipe,1,2,30);CHECK(next.start(result));CHECK(!result.receipt().verified && result.status().stage==Stage::original_pending);
    CHECK(next.original_done(result));CHECK(result.observe_completion(next.terminal,next.end,true)==Stage::complete);
  }
}
void reject_admission() {
  for(const auto& r:cat::recipes) {
    auto rejected=[&](auto mutate) {Flow f(r,2);ubc::AlchemyExtraResult result;mutate(f);CHECK(!f.start(result));CHECK(result.status().stage==Stage::idle);};
    rejected([](Flow& f){f.snapshot.visible=false;});
    rejected([](Flow& f){f.snapshot.learned_known=false;});
    rejected([](Flow& f){f.snapshot.owner_conditions_match=false;});
    rejected([](Flow& f){f.snapshot.effect_ready=false;});
    rejected([](Flow& f){f.snapshot.native_debit_verified=false;});
    rejected([](Flow& f){f.snapshot.extra_flask_stack_verified=false;});
    rejected([](Flow& f){f.snapshot.context.epoch+=1;});
    rejected([](Flow& f){f.before.serial=f.snapshot.serial;});
    rejected([](Flow& f){f.before.stocks.rows[0].stored-=1;});
    rejected([](Flow& f){f.snapshot.rows[0].remaining+=1;});
    rejected([](Flow& f){f.snapshot.rows[0].unit_cost+=1;});
    rejected([](Flow& f){f.snapshot.rows[0].held=1;});
    rejected([](Flow& f){f.admission.flask_constructor=false;});
    rejected([](Flow& f){f.before.parser.data_depth=1;});
    rejected([](Flow& f){f.job.owner+=1;});
    rejected([](Flow& f){f.job.callback+=1;});
  }
}
void reject_effect_evidence() {
  const auto& r=cat::recipes[3];
  auto failed_original=[&](auto mutate,bool normal=true,bool restore=true) {
    Flow f(r,3);ubc::AlchemyExtraResult result;CHECK(f.start(result));mutate(f);
    CHECK(!result.original_returned(f.original,normal,restore));CHECK(result.status().stage==Stage::blocked && !result.receipt().verified);
    Flow next(r,1,2,40);CHECK(!next.start(result));
  };
  failed_original([](Flow& f){f.original.stocks.rows[0].stored+=1;});
  failed_original([](Flow& f){f.original.stocks.rows[0].stored+=6;}); // Original quantity 1 instead of N3 with triple costs.
  failed_original([](Flow& f){f.original.stocks.rows[0].known=false;});
  failed_original([](Flow& f){f.original.hand.right+=4;});
  failed_original([](Flow& f){f.original.context.mob+=4;});
  failed_original([](Flow&){},false,true);
  failed_original([](Flow&){},true,false);
  {
    Flow f(r,3);ubc::AlchemyExtraResult result;CHECK(f.start(result));CHECK(f.original_done(result));
    CHECK(!result.acknowledge_extra_debit(f.extra));CHECK(result.status().stage==Stage::blocked); // No evidence of native success.
  }
  {
    Flow f(r,3);ubc::AlchemyExtraResult result;CHECK(f.start(result));CHECK(f.original_done(result));
    CHECK(result.tracker_for_bridge().begin_debit(f.late).valid);
    CHECK(!result.tracker_for_bridge().debit_returned(f.extra,true,false));
    CHECK(!result.acknowledge_extra_debit(f.extra) && result.status().stage==Stage::blocked);
  }
  {
    Flow f(r,3);ubc::AlchemyExtraResult result;CHECK(f.start(result));CHECK(f.original_done(result));
    CHECK(result.tracker_for_bridge().begin_debit(f.late).valid);
    auto partial=f.extra;partial.stocks.rows[0].stored+=1;
    CHECK(!result.tracker_for_bridge().debit_returned(partial,true,true));CHECK(!result.acknowledge_extra_debit(f.extra));
  }
  {
    Flow f(r,3);ubc::AlchemyExtraResult result;CHECK(f.start(result));CHECK(f.original_done(result));
    CHECK(result.observe_completion(f.terminal,f.end,true)==Stage::blocked); // Physically gone before the required debit.
  }
  {
    Flow f(r,1);ubc::AlchemyExtraResult result;CHECK(f.start(result));CHECK(f.original_done(result));
    CHECK(!result.acknowledge_extra_debit(f.extra));CHECK(result.status().stage==Stage::blocked); // N1 never counts as a debit receipt.
  }
  {
    Flow f(r,3);ubc::AlchemyExtraResult result;f.all_before_exit(result);
    CHECK(!result.acknowledge_extra_debit(f.extra));CHECK(result.status().stage==Stage::blocked); // Once.
  }
}
void reject_terminal() {
  for(const auto& r:cat::recipes) {
    auto bad=[&](auto mutate) {Flow f(r,2);ubc::AlchemyExtraResult result;f.all_before_exit(result);mutate(f);
      CHECK(result.observe_completion(f.terminal,f.end,true)==Stage::blocked);CHECK(!result.receipt().verified);};
    bad([](Flow& f){f.terminal.context.world+=4;});
    bad([](Flow& f){f.terminal.context.hero+=4;});
    bad([](Flow& f){f.terminal.context.mob=f.job.context.mob;});
    bad([](Flow& f){f.terminal.scope.writer_active=true;});
    bad([](Flow& f){f.terminal.hand.interact_item=0x9000;});
    bad([](Flow& f){f.terminal.hand.right=0x9000;});
    bad([](Flow& f){f.terminal.production=4;});
    bad([](Flow& f){f.terminal.invincible=1;});
    bad([](Flow& f){f.terminal.stocks.rows[0].stored+=1;});
  }
}
} // namespace result_test
int main() {
  static_assert(!std::is_copy_constructible<ubc::AlchemyExtraResult>::value,"bound tracker has one owner");
  result_test::successful_paths();result_test::reject_admission();result_test::reject_effect_evidence();result_test::reject_terminal();
  std::printf("%u checks, %u failures\n",extra_test::checks,extra_test::failures);return extra_test::failures?1:0;
}
