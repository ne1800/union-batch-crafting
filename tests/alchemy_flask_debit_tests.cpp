// SPDX-License-Identifier: MIT
#include "ubc/alchemy_flask_debit.hpp"
#include <cstdio>
#include <utility>

namespace d=ubc::alchemy_flask_debit;
namespace {
unsigned checks=0,failures=0;
void check(bool result,const char* description){
  ++checks;if(!result){++failures;std::fprintf(stderr,"FAIL: %s\n",description);}
}
struct Fixture {
  const d::Profile& profile;
  d::Job job{};
  d::Admission admission{true,true,true,true,true,true};
  d::Observation before{};
  Fixture(const d::Profile& p=d::profiles[0],std::uint32_t quantity=7):profile(p){
    job={{10,11,ubc::lhiver::game_parser,0x1000,0x2000,0x3000,0x4000},20,30,p.callback,p.owner,quantity};
    before.context=job.context;before.scope={job.id,job.writer_token,p.callback,1,0,0,true};
    before.serial=100;before.production=4;before.invincible=1;before.native_contract=true;before.stable_read=true;
    before.hand={true,true,0x5000,d::flask_symbol,1,0x5000};
    before.flask={true,true,true,0,1,0x6000,30,0x00200001};
    before.parser={true,true,job.context.parser,0x1000000,3000000,500,0,0x7000,0x3000,
      0x7000,0x7100,0x3000,0x3000,0x8000,0x9000};
    before.stocks.complete_unambiguous=true;before.stocks.count=p.cost_count+2;
    for(std::size_t i=0;i<p.cost_count;++i)before.stocks.rows[i]={p.costs[i].symbol,1000,true};
    before.stocks.rows[p.cost_count]={p.output,23,true};
    before.stocks.rows[p.cost_count+1]={d::flask_symbol,30,true};
  }
  d::Observation original() const {
    auto v=before;v.serial=101;v.production=0;v.invincible=0;
    v.parser.data_depth=d::original_return_words(job.callback);v.parser.data_words_known=true;
    for(std::uint32_t i=0;i<v.parser.data_depth;++i)v.parser.data_words[i]=i%2?0x40u:1u;
    for(std::size_t i=0;i<profile.cost_count;++i)
      v.stocks.rows[i].stored-=static_cast<std::int64_t>(job.quantity)*profile.costs[i].units;
    v.stocks.rows[profile.cost_count].stored+=job.quantity;return v;
  }
  d::Observation late() const {auto v=original();v.serial=102;return v;}
  d::Observation debit() const {
    auto v=original();v.serial=103;v.scope.debit_depth=1;
    v.stocks.rows[profile.cost_count+1].stored-=job.quantity-1;
    v.flask.amount-=job.quantity-1;
    if(!v.flask.amount){v.flask.object=0;v.flask.listed_stacks=0;}
    return v;
  }
  d::Observation terminal() const {
    auto v=debit();v.serial=105;v.scope={};v.context.mob=0;v.hand={true,true,0,0,0,0};return v;
  }
  d::EndWitness witness() const {return {true,job.id,job.writer_token,104,job.context};}
};
d::RestoreObservation restore_observation(const d::Call& call){
  const auto& p=call.parser;
  return {call.context,{call.job,call.writer_token,call.callback,1,0,1,true},true,true,
    call.context.parser,p.code_base,p.code_size,p.use_symbol,p.use_address,
    p.self_symbol,p.hero_symbol,p.self,p.hero,p.other,p.victim};
}
bool arm(d::Tracker& t,const Fixture& f){return t.arm(f.job,f.admission,f.before);}
bool original(d::Tracker& t,const Fixture& f){return t.original_returned(f.original(),true,true);}
d::Call begin(d::Tracker& t,const Fixture& f){return t.begin_debit(f.late());}
void happy(){
  for(const auto& p:d::profiles){
    check(d::valid(p),"all reviewed profiles are structurally disjoint");
    for(const auto n:{1u,2u,7u,31u}){
      Fixture f(p,n);d::Tracker t;
      check(arm(t,f),"all thirteen recipes bind exact materials and one reserved flask");
      check(original(t,f),"positive scaled original delta required");
      if(n>1){
        const auto call=begin(t,f);check(call.valid,"one debit is prepared");
        check(call.instance==16604&&call.quantity==n-1&&call.context.hero==f.job.context.hero,
          "debit only additional flasks for exact bound hero");
        check(d::restorable(call,restore_observation(call)),"known restore targets match exact saved parser context");
        check(t.debit_returned(f.debit(),true,true),"exact positive debit delta accepted");
      }else check(t.status().stage==d::Stage::debit_verified,"N1 has no zero-removal call");
      check(t.status().stage!=d::Stage::complete,"debit success is not station completion");
      check(t.finish(f.terminal(),f.witness()),"bound EndInteraction after-witness completes exactly once");
      check(!t.finish(f.terminal(),f.witness())&&t.status().blocked,"terminal reentry cannot reopen completion");
    }
  }
}
using Mutate=void(*)(d::Observation&);
constexpr Mutate bad_before[]{
  [](auto& v){v.context.thread++;},[](auto& v){v.context.epoch++;},
  [](auto& v){v.context.hero++;},[](auto& v){v.context.mob++;},
  [](auto& v){v.scope.writer_token++;},[](auto& v){v.scope.job++;},
  [](auto& v){v.scope.callback++;},[](auto& v){v.scope.writer_depth=2;},
  [](auto& v){v.scope.vm_depth=1;},[](auto& v){v.scope.debit_depth=1;},
  [](auto& v){v.scope.writer_active=false;},[](auto& v){v.serial=0;},
  [](auto& v){v.stable_read=false;},[](auto& v){v.native_contract=false;},
  [](auto& v){v.production=0;},[](auto& v){v.invincible=0;},
  [](auto& v){v.hand.known=false;},[](auto& v){v.hand.left_empty=false;},
  [](auto& v){v.hand.right=0;},[](auto& v){v.hand.symbol++;},
  [](auto& v){v.hand.amount=2;},[](auto& v){v.hand.interact_item++;},
  [](auto& v){v.flask.known=false;},[](auto& v){v.flask.all_slot_aliases_excluded=false;},
  [](auto& v){v.flask.native_item_contract=false;},[](auto& v){v.flask.packed=-1;},
  [](auto& v){v.flask.packed=1;},[](auto& v){v.flask.listed_stacks=2;},
  [](auto& v){v.flask.object=0;},[](auto& v){v.flask.object=v.hand.right;},
  [](auto& v){v.flask.amount--;},[](auto& v){v.flask.flags|=0x40000000;},
  [](auto& v){v.flask.flags&=~0x00200000u;},
  [](auto& v){v.stocks.complete_unambiguous=false;},[](auto& v){v.stocks.count--;},
  [](auto& v){v.stocks.count=100;},[](auto& v){v.stocks.rows[0].known=false;},
  [](auto& v){v.stocks.rows[0].stored=-1;},[](auto& v){v.stocks.rows[0].stored=0;},
  [](auto& v){v.stocks.rows[0].stored=static_cast<std::int64_t>(INT32_MAX)+1;},
  [](auto& v){v.stocks.rows[0].symbol++;},[](auto& v){v.stocks.rows[0].symbol=v.stocks.rows[1].symbol;},
  [](auto& v){v.parser.known=false;},[](auto& v){v.parser.instance_bindings_verified=false;},
  [](auto& v){v.parser.current_parser++;},[](auto& v){v.parser.code_base=0;},
  [](auto& v){v.parser.code_size=0;},[](auto& v){v.parser.code_size=0x1000001;},
  [](auto& v){v.parser.code_base=UINT32_MAX;},[](auto& v){v.parser.pop_pos=v.parser.code_size+1;},
  [](auto& v){v.parser.data_depth=1;},[](auto& v){v.parser.use_symbol=0x1234;},
  [](auto& v){v.parser.use_address++;},[](auto& v){v.parser.self_symbol=0;},
  [](auto& v){v.parser.hero_symbol=v.parser.self_symbol;},[](auto& v){v.parser.self++;},
  [](auto& v){v.parser.hero++;}
};
void admission(){
  for(auto mutate:bad_before){Fixture f;d::Tracker t;mutate(f.before);
    check(!arm(t,f),"invalid admission denies before any effect");
    check(!t.status().blocked&&t.status().stage==d::Stage::idle,"pre-effect rejection leaves no pending debit");}
  Fixture f;
  for(unsigned k=0;k<6;++k){auto a=f.admission;
    switch(k){case 0:a.local_script_contract=false;break;case 1:a.original_amount_contract=false;break;
      case 2:a.native_debit_and_restore_contract=false;break;case 3:a.flask_constructor=false;break;
      case 4:a.owner_and_knowledge=false;break;default:a.physical_lab_hand_contract=false;}
    d::Tracker t;check(!t.arm(f.job,a,f.before),"each separate positive contract is mandatory");}
  for(auto id:{20370u,20403u,20404u,20405u,20298u,0u}){
    d::Tracker t;auto j=f.job;j.callback=id;check(!t.arm(j,f.admission,f.before),"branching paid-owner and quest recipes excluded");}
  for(auto n:{0u,0xffffffffu,32u}){d::Tracker t;auto j=f.job;j.quantity=n;
    check(!t.arm(j,f.admission,f.before),"zero overflow and insufficient additional flasks rejected");}
  f.before.stocks.rows[f.profile.cost_count].stored=INT32_MAX;
  d::Tracker t;check(!arm(t,f),"output int32 room checked before original effect");
}
void positive_and_late(){
  Fixture f;
  for(unsigned k=0;k<8;++k){d::Tracker t;check(arm(t,f),"prepare altered original result");auto v=f.original();
    switch(k){case 0:v.stocks.rows[0].stored++;break;case 1:v.stocks.rows[4].stored--;break;
      case 2:v.stocks.rows[5].stored++;break;case 3:v.stocks.rows[5].stored--;break;
      case 4:v.hand.right++;v.hand.interact_item=v.hand.right;break;case 5:v.production=4;break;
      case 6:v.invincible=1;break;default:v.stocks.complete_unambiguous=false;}
    check(!t.original_returned(v,true,true)&&t.status().blocked,"no normal-return-only or partial original success");
    check(!begin(t,f).valid,"unknown original effect forbids extra debit");}
  for(unsigned k=0;k<2;++k){d::Tracker t;check(arm(t,f),"prepare original exception/restore fault");
    check(!t.original_returned(f.original(),k!=0,k!=1)&&t.status().blocked,"original exception or amount restore failure blocks");}
  constexpr Mutate bad_late[]{
    [](auto& v){v.context.world++;},[](auto& v){v.context.game++;},[](auto& v){v.context.parser++;},
    [](auto& v){v.context.mob++;},[](auto& v){v.context.thread++;},[](auto& v){v.scope.writer_depth++;},
    [](auto& v){v.scope.writer_token++;},[](auto& v){v.scope.callback++;},[](auto& v){v.scope.debit_depth++;},
    [](auto& v){v.serial=101;},[](auto& v){v.stocks.rows[0].stored--;},
    [](auto& v){v.flask.object++;},[](auto& v){v.flask.flags|=4;},[](auto& v){v.flask.packed=1;},
    [](auto& v){v.hand.right++;v.hand.interact_item=v.hand.right;},
    [](auto& v){v.parser.code_base++;},[](auto& v){v.parser.code_size++;},
    [](auto& v){v.parser.pop_pos++;},[](auto& v){v.parser.use_symbol=v.parser.hero_symbol;},
    [](auto& v){v.parser.other++;},[](auto& v){v.parser.victim++;},[](auto& v){v.parser.data_depth=1;}
  };
  for(auto mutate:bad_late){d::Tracker t;check(arm(t,f)&&original(t,f),"prepare late gate fault");auto v=f.late();mutate(v);
    check(!t.begin_debit(v).valid&&t.status().blocked,"late context inventory or parser mutation blocks before extra call");}
}
void debit_failures(){
  Fixture f;
  constexpr Mutate bad_after[]{
    [](auto& v){v.stocks.rows[5].stored++;},[](auto& v){v.stocks.rows[5].stored--;},
    [](auto& v){v.stocks.rows[0].stored--;},[](auto& v){v.stocks.rows[4].stored++;},
    [](auto& v){v.stocks.rows[5].known=false;},[](auto& v){v.flask.object++;},
    [](auto& v){v.flask.amount++;},[](auto& v){v.flask.packed=-1;},
    [](auto& v){v.flask.all_slot_aliases_excluded=false;},[](auto& v){v.flask.flags|=0x40000000;},
    [](auto& v){v.context.mob=0;},[](auto& v){v.hand.right=0;},
    [](auto& v){v.parser.pop_pos++;},[](auto& v){v.parser.use_address++;},
    [](auto& v){v.parser.data_depth=1;},[](auto& v){v.parser.current_parser++;},
    [](auto& v){v.scope.debit_depth=0;},[](auto& v){v.scope.writer_active=false;},
    [](auto& v){v.serial=102;},[](auto& v){v.stable_read=false;}
  };
  for(auto mutate:bad_after){d::Tracker t;check(arm(t,f)&&original(t,f)&&begin(t,f).valid,"prepare postdebit fault");
    auto v=f.debit();mutate(v);check(!t.debit_returned(v,true,true)&&t.status().blocked,"partial or unknown debit is sticky failure");
    check(!begin(t,f).valid&&!t.arm(f.job,f.admission,f.before),"no retry new job or compensation after possible mutation");}
  for(unsigned k=0;k<2;++k){d::Tracker t;check(arm(t,f)&&original(t,f)&&begin(t,f).valid,"prepare native exception/restore fault");
    check(!t.debit_returned(f.debit(),k!=0,k!=1)&&t.status().blocked,"positive quantities do not override native/restore exception");}
  {d::Tracker t;check(arm(t,f)&&original(t,f)&&begin(t,f).valid,"prepare direct reentry");
    check(!begin(t,f).valid&&t.status().blocked,"debit call can only be claimed once");}
  Fixture all(d::profiles[0],31);d::Tracker t;check(arm(t,all)&&original(t,all)&&begin(t,all).valid,"prepare whole-stack removal");
  auto zero=all.debit();zero.flask.object=all.before.flask.object;
  check(!t.debit_returned(zero,true,true),"zero amount requires fresh absence not stale stack pointer");
}
void restore_and_terminal(){
  Fixture f;d::Tracker t;check(arm(t,f)&&original(t,f),"prepare restore identity tests");const auto call=begin(t,f);
  for(unsigned k=0;k<20;++k){auto v=restore_observation(call);
    switch(k){case 0:v.known=false;break;case 1:v.native_restore_contract=false;break;
      case 2:v.context.hero++;break;case 3:v.context.mob++;break;case 4:v.scope.writer_token++;break;
      case 5:v.scope.debit_depth=0;break;case 6:v.scope.writer_depth++;break;case 7:v.scope.vm_depth++;break;
      case 8:v.current_parser++;break;case 9:v.code_base++;break;case 10:v.code_size++;break;
      case 11:v.saved_use_symbol++;break;case 12:v.saved_use_address++;break;case 13:v.self_symbol++;break;
      case 14:v.hero_symbol++;break;case 15:v.self++;break;case 16:v.hero++;break;case 17:v.other++;break;
      case 18:v.victim++;break;default:v.scope.writer_active=false;}
    check(!d::restorable(call,v),"unknown changed or stale restoration targets are never written");}
  for(unsigned k=0;k<11;++k){d::Tracker terminal;check(arm(terminal,f)&&original(terminal,f)&&begin(terminal,f).valid&&
      terminal.debit_returned(f.debit(),true,true),"prepare terminal fault");
    auto v=f.terminal();auto w=f.witness();
    switch(k){case 0:w.after=false;break;case 1:w.job++;break;case 2:w.writer_token++;break;
      case 3:w.departed.mob++;break;case 4:w.serial=103;break;case 5:w.serial=105;break;
      case 6:v.context.world++;break;case 7:v.context.mob=f.job.context.mob;break;
      case 8:v.hand.right=f.before.hand.right;break;case 9:v.scope.writer_active=true;break;
      default:v.stocks.rows[0].stored--;}
    check(!terminal.finish(v,w)&&terminal.status().blocked,"only exact later normal station completion accepted");}
}
void limits_and_lifecycle(){
  Fixture f(d::profiles[10],INT32_MAX);f.before.stocks.rows[0].stored=INT32_MAX;
  f.before.stocks.rows[1].stored=0;f.before.stocks.rows[2].stored=INT32_MAX;
  f.before.flask.amount=INT32_MAX;d::Tracker t;
  check(arm(t,f)&&original(t,f)&&begin(t,f).valid,"signed-int32 boundary uses wide arithmetic");
  check(t.debit_returned(f.debit(),true,true)&&t.finish(f.terminal(),f.witness()),"boundary debit leaves exactly one stored flask");
  Fixture next(d::profiles[10],1);next.job.id=f.job.id+1;next.job.writer_token=f.job.writer_token+1;
  next.before.scope.job=next.job.id;next.before.scope.writer_token=next.job.writer_token;
  check(!t.arm(next.job,next.admission,next.before),"new job cannot recycle a snapshot predating the previous completion");
  next.before.serial=f.terminal().serial+1;
  check(t.arm(next.job,next.admission,next.before),"new monotonic job allowed only after complete");
  check(!t.arm(next.job,next.admission,next.before),"pending job cannot be replaced");
  Fixture n1(d::profiles[10],1);n1.before.stocks.rows[2].stored=0;n1.before.flask={};d::Tracker single;
  check(arm(single,n1)&&original(single,n1),"N1 needs only reserved handflask and never a zero native removal");
  Fixture shuffled;std::swap(shuffled.before.stocks.rows[0],shuffled.before.stocks.rows[1]);
  d::Tracker reorder;check(arm(reorder,shuffled),"rows are identified by symbols not positions");
  for(const auto& p:d::profiles){auto invalid=p;invalid.output=invalid.costs[0].symbol;
    check(!d::valid(invalid),"shared input-output symbol invalid without ordering contract");
    invalid=p;invalid.costs[0].symbol=d::flask_symbol;check(!d::valid(invalid),"reserve is never an ordinary body cost");}
}
void exact_return_prefixes(){
  // Independent expectations for the 13 successful paths. An arbitrary even
  // stack depth or a cost count from the profile cannot authorize a path.
  constexpr std::array<std::uint32_t,13> counts{{8,8,6,4,4,4,4,4,4,4,2,4,6}};
  for(std::size_t n=0;n<d::profiles.size();++n){
    Fixture f(d::profiles[n],2);const auto good=f.original();
    check(d::original_return_words(f.job.callback)==counts[n],"exact per-callback return count");
    check(d::valid_after_original(good.parser,f.job.context,f.job.callback),"exact known return words accepted only AFTER");
    check(!d::valid(good.parser,f.job.context),"BEFORE never accepts the AFTER prefix");
    auto pre=f.before;pre.parser=good.parser;d::Tracker before;
    check(!before.arm(f.job,f.admission,pre),"stale original return prefix denies a new original effect");
    for(const auto depth:{0u,1u,2u,4u,6u,8u,9u,2048u,2049u,UINT32_MAX})if(depth!=counts[n]){
      auto v=good;v.parser.data_depth=depth;d::Tracker t;check(arm(t,f),"prepare wrong return depth");
      check(!t.original_returned(v,true,true)&&t.status().blocked&&!begin(t,f).valid,"different depth blocks before any extra debit");
    }
    for(std::uint32_t word=0;word<counts[n];++word)for(unsigned bit=0;bit<32;++bit){
      auto v=good;v.parser.data_words[word]^=1u<<bit;
      check(!d::valid_after_original(v.parser,f.job.context,f.job.callback),"every active-bit deviation invalidates original prefix");
      check(!d::same_data_stack(good.parser,v.parser),"every active-bit deviation invalidates bytewise preservation");
    }
    auto unknown=good;unknown.parser.data_words_known=false;d::Tracker t;
    check(arm(t,f)&&!t.original_returned(unknown,true,true)&&t.status().blocked,"unread words are never fabricated as expected returns");
    for(std::uint32_t word=0;word<counts[n];++word){
      d::Tracker late;check(arm(late,f)&&original(late,f),"prepare late prefix mutation");
      auto changed=f.late();changed.parser.data_words[word]^=1;
      check(!late.begin_debit(changed).valid&&late.status().blocked,"late prefix changes block native call");
      d::Tracker after;check(arm(after,f)&&original(after,f)&&begin(after,f).valid,"prepare changed native prefix");
      auto result=f.debit();result.parser.data_words[word]^=1;
      check(!after.debit_returned(result,true,true)&&after.status().blocked&&!begin(after,f).valid,"native prefix changes block success and retry");
    }
    if(counts[n]<8){auto inactive=good;inactive.parser.data_words[counts[n]]=0xdeadbeef;
      check(d::same_data_stack(good.parser,inactive.parser),"constructor temporary cells above the active prefix are not active state");}
    for(const auto callback:{0u,20370u,20298u,20403u})
      check(!d::valid_after_original(good.parser,f.job.context,callback),"unknown or separate Spirit callback has no 13-family permission");
  }
}
}
int main(){happy();admission();positive_and_late();debit_failures();restore_and_terminal();limits_and_lifecycle();exact_return_prefixes();
  std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;}
