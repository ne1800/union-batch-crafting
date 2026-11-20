// SPDX-License-Identifier: MIT
#include "ubc/stations/cool/cool_predebit_contract.hpp"
#include "ubc/stations/cool/cool_predebit_journal.hpp"
#include "ubc/stations/cool/cool_predebit_admission.hpp"
#include <cstdio>
#include <type_traits>
namespace p=ubc::cool_predebit;
namespace v=ubc::alchemy_flask_debit;
int checks=0,failures=0;
#define CHECK(x) do{++checks;if(!(x)){++failures;std::fprintf(stderr,"%d: %s\n",__LINE__,#x);}}while(false)
struct Fixture {
  p::Job job{{1,7,ubc::lhiver::game_parser,0x100000,0x200000,0x300000,0x400000},10,20,30,40,0x700000,0xaac458};
  p::Observation before{},late{},after{},ready{},returned{};
  explicit Fixture(std::int64_t cold=5,std::int32_t tmp=1){
    before.job=job;before.scope={{job.id,job.writer_token,p::callback,1,0,0,true},job.info_token,job.actual_info,true};
    before.serial=1;before.frame=8;before.stocks={true,{{{16612,2,true,true},{16613,cold,true,false}}}};
    before.hand={true,true,0x600000,0,16612,1,0x600000,tmp};
    before.stack={true,true,true,true,0,1,0x500000,16612,2,1,0x83c804,1,0x200001};
    auto& s=before.parser;s.known=s.instance_bindings_verified=true;s.current_parser=job.context.parser;
    s.code_base=0x10000000;s.code_size=ubc::lhiver::code_length;s.pop_pos=0x1234;
    s.self_symbol=0x710000;s.hero_symbol=0x710100;s.self=s.hero=s.use_address=job.context.hero;
    s.use_symbol=s.self_symbol;s.other=0x800000;
    before.item={true,true,job.context.parser,0x710200,0x900000,s.code_base,s.code_size,ubc::lhiver::symbol_count,1682,7,1,0,0x999000,1584};
    before.fence={true,true,job.fence_token,1,0x720000,0,0,{}};
    before.production=13;before.invincible=before.start_smithing=1;
    before.stable_read=before.native_split_restore=before.generated_hand_profile=before.complete_inventory=true;
    late=before;late.serial=late.fence.serial=2;
    after=late;after.serial=after.fence.serial=4;after.scope.writer.debit_depth=1;
    after.stocks.stocks[0].amount=after.stack.amount=1;
    ready=after;ready.serial=ready.fence.serial=5;ready.scope.writer.debit_depth=0;
    returned=ready;returned.serial=6;returned.production=returned.invincible=0;
    returned.stocks.stocks[1].amount+=2;returned.parser.data_depth=2;returned.parser.data_words_known=true;
    returned.parser.data_words[0]=1;returned.parser.data_words[1]=0x40;returned.parser.pop_pos=0x2345;
    returned.item={};returned.fence={}; // After the original call, neither the old pointers nor the short fence is required.
  }
};
p::Call begin(p::Tracker& t,const Fixture& f){CHECK(t.admit(f.job,f.before));CHECK(!t.call().valid);auto c=t.enter_extra(f.late);CHECK(p::valid_call(c));CHECK(t.call().valid);return c;}
p::RestoreObservation restoring(const p::Call& c){
  p::RestoreObservation r;r.job=c.job;r.scope={{c.job.id,c.job.writer_token,p::callback,1,0,1,true},c.job.info_token,c.job.actual_info,true};
  r.serial=c.serial+1;r.fence=c.fence;r.fence.serial=r.serial;r.native_set_item=true;
  r.inventory_identity=c.inventory_identity;r.hand_identity=c.hand_identity;r.frame=c.frame;
  const auto& s=c.parser;r.targets={c.job.context,r.scope.writer,true,true,s.current_parser,s.code_base,s.code_size,
    s.use_symbol,s.use_address,s.self_symbol,s.hero_symbol,s.self,s.hero,s.other,s.victim};
  r.current=s;r.current.pop_pos=800;r.current.use_symbol=c.item.symbol_object;r.current.use_address=0x123400;
  r.item=c.item;r.item.value=0x123400;return r;
}
int main(){
  {
    p::DestroyJournal journal;std::uint32_t entered=0,exited=0;
    journal.entered();CHECK(!journal.open());journal.exited();
    const auto first=journal.open();CHECK(first&&!journal.open()&&journal.sample(first,entered,exited));
    journal.entered();journal.entered();journal.exited();
    CHECK(!journal.sample(first,entered,exited)&&entered==2&&exited==1);
    journal.exited();CHECK(!journal.close(first)&&!journal.sample(first,entered,exited));
    const auto next=journal.open();CHECK(next>first&&!journal.close(first)&&journal.close(next));
  }

  static_assert(!p::native_effect_authorized);
  static_assert(!std::is_copy_constructible_v<p::Tracker>);
  {
    using namespace p::admission;
    static_assert(!std::is_copy_constructible_v<InfoLease>);
    Fixture f;const auto id=identity(f.job,8);InfoLease lease;
    const auto first=lease.open(id);CHECK(first==1&&lease.current(id,first));
    for(unsigned field=0;field<10;++field){auto wrong=id;
      switch(field){case 0:wrong.context.epoch++;break;case 1:wrong.context.thread++;break;
      case 2:wrong.context.parser++;break;case 3:wrong.context.game++;break;case 4:wrong.context.world++;break;
      case 5:wrong.context.hero++;break;case 6:wrong.context.mob++;break;case 7:wrong.frame++;break;
      case 8:wrong.actual_info+=4;break;case 9:wrong.manager+=4;break;}
      CHECK(!lease.current(wrong,first));
    }
    CHECK(!lease.open(id)&&!lease.current(id,first)); // Reentry invalidates the old lease.
    const auto second=lease.open(id);CHECK(second>first&&!lease.current(id,first));
    lease.close(first);CHECK(lease.current(id,second)); // An old finally block must not close a newer lease.
    lease.close(second);CHECK(!lease.current(id,second));
    const auto third=lease.open(id);CHECK(third>second);
    lease.invalidate();CHECK(!lease.current(id,third));
    auto foreign=id;foreign.context.thread++;
    CHECK(!lease.open(foreign));CHECK(lease.open(id)>third);
    InfoLease exhausted(UINT64_MAX-1);const auto last=exhausted.open(id);
    CHECK(last==UINT64_MAX);exhausted.close(last);CHECK(!exhausted.open(id)&&!exhausted.current(id,last));
    for(unsigned field=0;field<6;++field){InfoLease bad;auto missing=id;
      switch(field){case 0:missing.context.epoch=0;break;case 1:missing.context.thread=0;break;
      case 2:missing.frame=0;break;case 3:missing.actual_info=0;break;
      case 4:missing.actual_info=0xfffffffcu;break;case 5:missing.manager=0;break;}
      CHECK(!bad.open(missing));
    }
  }
  for(const auto cold:{INT64_C(0),INT64_C(5),INT64_C(2147483645)})for(int tmp:{1,2}){
    Fixture f(cold,tmp);const auto amount=p::plan(f.before.stocks,2);
    CHECK(amount.valid&&amount.maximum==2&&amount.direct_maximum==1&&amount.quantity_receipt);
    CHECK(amount.before[0].amount==2&&amount.before_original[0].amount==1&&amount.after_callback[0].amount==1&&amount.after_end[0].amount==0);
    CHECK(amount.before_original[1].amount==cold&&amount.after_callback[1].amount==cold+2&&amount.after_end[1].amount==cold+2);
    CHECK(amount.body().length==77&&amount.body().count==1&&amount.body().operands[0].offset==62);
    const auto legacy=ubc::cool_recipe::plan(ubc::cool_recipe_catalog::recipe,f.before.stocks,2);
    CHECK(legacy.after_callback[0].amount==2&&!legacy.requires_quantity_receipt); // Deliberately leave the old model unchanged.
    p::Tracker tracker;const auto c=begin(tracker,f);const auto restore=p::restore_plan(c,restoring(c));
    CHECK(restore.valid&&restore.write_item&&restore.pop_value==c.parser.pop_pos&&restore.item_value==c.item.value);
    CHECK(tracker.extra_returned(f.after,true,true));CHECK(!tracker.witness().valid);
    CHECK(tracker.enter_original(f.ready));CHECK(!tracker.witness().valid);
    CHECK(tracker.original_returned(f.returned,true,true));const auto witness=tracker.witness();
    CHECK(witness.valid&&witness.instance==16612&&witness.extra_debit==1&&witness.original_calls==1);
    CHECK(witness.extra_serial==4&&witness.original_serial==6&&witness.amount.quantity_receipt);
    CHECK(!tracker.enter_original(f.ready)&&tracker.status().blocked&&tracker.call().valid&&!tracker.witness().valid);
  }
  for(unsigned n:{0u,1u,3u,UINT32_MAX}){Fixture f;CHECK(!p::plan(f.before.stocks,n).valid);}
  for(auto hot:{-1,0,1,3,INT32_MAX}){Fixture f;f.before.stocks.stocks[0].amount=hot;CHECK(!p::plan(f.before.stocks,2).valid);}
  {Fixture f(INT64_C(2147483646));CHECK(!p::plan(f.before.stocks,2).valid);}
  for(unsigned field=0;field<16;++field){Fixture f;p::Tracker t;auto bad=f.before;
    switch(field){case 0:bad.job.info_token++;break;case 1:bad.scope.info_lease_active=false;break;case 2:bad.parser.data_depth=2;break;
    case 3:bad.fence.path_excludes_dispatch=false;break;case 4:bad.fence.event_manager=f.job.information_manager;break;
    case 5:bad.fence.destroy_started=1;break;case 6:bad.stack.flags=0xa00001;break;case 7:bad.stack.packed=1;break;
    case 8:bad.stack.object=bad.hand.left;break;case 9:bad.generated_hand_profile=false;break;case 10:bad.production=0;break;
    case 11:bad.scope.writer.vm_depth=1;break;case 12:bad.scope.writer.callback=20778;break;case 13:bad.item.index=1683;break;case 14:bad.serial=0;break;case 15:bad.frame=0;break;}
    CHECK(!t.admit(f.job,bad));CHECK(!t.call().valid&&!t.witness().valid);
  }
  for(unsigned field=0;field<16;++field){Fixture f;p::Tracker t;const auto c=begin(t,f);auto bad=f.after;
    switch(field){case 0:bad.stocks.stocks[0].amount=2;break;case 1:bad.stocks.stocks[1].amount++;break;case 2:bad.stack.amount=2;break;
    case 3:bad.parser.pop_pos++;break;case 4:bad.item.value++;break;case 5:bad.fence.queue_fingerprint[0]++;break;
    case 6:bad.fence.destroy_completed=1;break;case 7:bad.serial=2;break;case 8:bad.hand.left_tmp_level=2;break;
    case 9:bad.scope.info_token++;break;case 10:bad.stable_read=false;break;case 11:bad.native_split_restore=false;break;
    case 12:bad.complete_inventory=false;break;case 13:bad.inventory_identity[0]++;break;case 14:bad.hand_identity[0]++;break;case 15:bad.frame++;break;}
    CHECK(!t.extra_returned(bad,true,true));CHECK(t.call().valid&&t.status().blocked);
    CHECK(!t.enter_original(f.ready)&&!t.witness().valid);CHECK(p::valid_call(c));
  }
  for(bool normal:{false,true}){Fixture f;p::Tracker t;begin(t,f);CHECK(!t.extra_returned(f.after,normal,false));
    CHECK(t.call().valid&&!t.enter_original(f.ready));}
  for(unsigned field=0;field<12;++field){Fixture f;p::Tracker t;auto c=begin(t,f);auto now=restoring(c);
    switch(field){case 0:now.targets.saved_use_address++;break;case 1:now.current.data_depth=2;break;case 2:now.item.symbol_object+=4;break;
    case 3:now.scope.info_token++;break;case 4:now.fence.event_manager+=4;break;case 5:now.serial=c.serial;break;
    case 6:now.native_set_item=false;break;case 7:now.current.hero++;break;case 8:now.targets.native_restore_contract=false;break;
    case 9:now.inventory_identity[0]++;break;case 10:now.hand_identity[0]++;break;case 11:now.frame++;break;}
    CHECK(!p::restore_plan(c,now).valid);
  }
  for(unsigned field=0;field<5;++field){Fixture f;p::Tracker t;begin(t,f);CHECK(t.extra_returned(f.after,true,true));auto late=f.ready;
    switch(field){case 0:late.stocks.stocks[0].amount=2;break;case 1:late.scope.info_lease_active=false;break;
    case 2:late.job.context.epoch++;break;case 3:late.fence.destroy_started=1;break;case 4:late.parser.use_address++;break;}
    CHECK(!t.enter_original(late)&&t.call().valid&&!t.witness().valid);
  }
  for(unsigned mode=0;mode<6;++mode){Fixture f;p::Tracker t;begin(t,f);CHECK(t.extra_returned(f.after,true,true));CHECK(t.enter_original(f.ready));auto after=f.returned;
    if(mode==2)after.stocks.stocks[0].amount=0; if(mode==3)after.stocks.stocks[1].amount--;
    if(mode==4)after.parser.data_depth=0;if(mode==5)after.scope.info_lease_active=false;
    CHECK(!t.original_returned(after,mode!=0,mode!=1));CHECK(t.call().valid&&!t.witness().valid);
  }
  {Fixture f;p::Tracker t;CHECK(t.admit(f.job,f.before));CHECK(!t.enter_original(f.ready));CHECK(!t.call().valid);}
  std::printf("cool_predebit_contract: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
