// SPDX-License-Identifier: MIT
// Only synthetic value copies. No claim of native evidence.
#include "ubc/cool_destroy_observation.hpp"
#include "ubc/cool_destroy_hand_diagnostic.hpp"
#include "ubc/generated_blade_hand_shape.hpp"
#include "ubc/cool_hotblade_debit.hpp"
#include <cstdio>
#include <type_traits>
namespace d=ubc::cool_destroy_observation;
namespace direct=ubc::cool_direct_contract;
namespace cat=ubc::cool_recipe_catalog;
namespace diag=ubc::cool_destroy_hand_diagnostic;
namespace handshape=ubc::generated_blade_hand_shape;
namespace {
unsigned checks=0,failures=0;
void check(bool value,const char* label){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",label);}}
ubc::StationRecipeHands active() {
  ubc::StationRecipeHands h;
  h.left_slot_known=h.right_slot_known=h.interact_item_known=true;
  h.hands.left={1,16612,1,0x910000};h.hands.right={0,-1,-1,0};
  h.left_tmp_level=1;h.right_tmp_level=0;h.interact_item=0x910000;return h;
}
ubc::StationRecipeHands empty() {
  auto h=active();h.hands.left={0,-1,-1,0};h.interact_item=0;return h;
}
ubc::cool_recipe::Evidence stock(std::int64_t hot,std::int64_t cold) {
  return {true,{{{16612,hot,true,true},{16613,cold,true,true}}}};
}
struct Fixture {
  direct::Prepared prepared{};d::Binding binding{};d::Observation observed{};
  Fixture(std::int64_t hot=7,std::int64_t cold=11,std::uint32_t quantity=2) {
    binding.key={23,7,0x400000,0x500000,0x600000,0x700000,13};
    binding.job=91;binding.frame=92;binding.serial_floor=100;
    binding.recipe=static_cast<std::uint32_t>(cat::RecipeId::blade);binding.quantity=quantity;
    prepared.key=binding.key;prepared.hands=active();prepared.stocks=stock(hot,cold);
    prepared.path=ubc::cool_recipe_navigation::make_plan(cat::recipe,prepared.stocks,quantity,{true,true,true});
    prepared.maximum=prepared.path.amount.maximum;prepared.direct_maximum=prepared.path.amount.direct_maximum;
    prepared.stock_known=true;prepared.valid=prepared.path.valid&&!prepared.path.amount.requires_extra_debit;
    const std::int64_t before_hot=hot-static_cast<std::int64_t>(quantity)+1;
    const std::int64_t after_hot=hot-static_cast<std::int64_t>(quantity),after_cold=cold+quantity;
    d::Snapshot* points[]{&observed.before_destroy,&observed.before_remove,&observed.after_remove,&observed.after_destroy};
    for(unsigned i=0;i<4;++i){auto& p=*points[i];
      p.stamp={binding.key,binding.job,binding.frame,101u+i,binding.key.thread,true,true,true};
      p.stocks=stock(i<2?before_hot:after_hot,after_cold);p.hands=i?empty():active();
    }
    observed.destroy={0x74c586,binding.key.hero,0x920000,0xab2860,9,1,1,true,true,false,1};
    observed.remove={0x754b98,binding.key.hero+0x668,0x83c714,0x70d080,16612,1,1,1,0x930000,true,false};
    observed.destroy_entries=observed.destroy_exits=observed.remove_entries=observed.remove_exits=1;
    observed.generated_left_verified=handshape::matches(16612,0x83c804,1,0xa00001);
    observed.storage_alias_scan_known=true;
    observed.set_interact_item_normal_return_verified=true;
  }
  d::Result verify()const{return d::verify(prepared,binding,observed);}
};
void rejected(const Fixture& f,const char* label){const auto r=f.verify();check(!r.valid&&r.issue!=d::Issue::none&&!r.serial&&!r.frame,label);}
void generated_hand_shape_tests() {
  // Separate current hand-shape evidence. Unchanged balances cannot replace it.
  {Fixture f;check(f.verify().valid,"exact NFOCUS generated HotBlade shape reaches existing typed contract");}
  for(unsigned bit=0;bit<32;++bit){Fixture f;
    f.observed.generated_left_verified=handshape::matches(16612,0x83c804,1,0xa00001u^(1u<<bit));
    check(f.verify().issue==d::Issue::generated_hand&&!f.verify().valid,
      "every missing or additional generated HotBlade flag bit fails with exact otherwise valid balances");
  }
  for(unsigned fault=0;fault<5;++fault){Fixture f;
    const auto symbol=fault==0?16611:16612;
    const auto type=fault==1?0u:0x83c804u;
    const auto main=fault==2?2u:1u;
    const auto flags=fault==3?0x200001u:fault==4?0u:0xa00001u;
    f.observed.generated_left_verified=handshape::matches(symbol,type,main,flags);
    check(f.verify().issue==d::Issue::generated_hand&&!f.verify().valid,
      "foreign type or symbol mainflag stored shape and unreadable sentinel cannot become generated hand proof");
  }
  ubc::cool_hotblade_debit::HotStack stored;
  stored.known=stored.all_inventory_aliases_excluded=stored.all_slot_aliases_excluded=stored.native_split_path_verified=true;
  stored.packed=0;stored.listed_stacks=1;stored.object=0x950000;stored.instance=16612;
  stored.amount=2;stored.refcount=1;stored.vtable=0x83c804;stored.mainflag=1;stored.flags=0x200001;
  check(ubc::cool_hotblade_debit::listed_stack(stored,2),"unchanged stored HotBlade debit profile remains valid");
  stored.flags=0xa00001;
  check(!ubc::cool_hotblade_debit::listed_stack(stored,2),"generated NFOCUS hand shape is still rejected as stored debit stack");
}
diag::Record diagnostic(const Fixture& f) {
  diag::Record r;r.captured=r.hands_read_verified=r.profile_checked=r.profile_matches=true;
  r.stamp=f.observed.before_destroy.stamp;r.prepared=f.prepared.hands;r.observed=f.observed.before_destroy.hands;
  r.generated_left_verified=f.observed.generated_left_verified;
  r.shape_attempted=r.shape_known=63;r.shape={{0x83c804,1,0xa00001,0x83c804,1,0xa00001}};
  return r;
}
void hand_diagnostic_tests() {
  static_assert(!diag::execution_authorized&&std::is_trivially_copyable<diag::Record>::value,"owned diagnostic values only");
  {Fixture f;const auto r=diagnostic(f);
    const std::array<std::int32_t,16> expected{{1,16612,1,1,0,0,16612,1,1,0,0x83c804,1,0xa00001,0x83c804,1,0xa00001}};
    check(diag::values(r)==expected,"fixed diagnostic wire selectors1..16");
    check(diag::flags(r)==0x3f3f1fff,"fixed comparison and attempted/known bit positions");
    check(!diag::ready(r,f.verify().issue,f.observed.before_destroy.stamp),"positive destroy never emits failure diagnosis");
    for(auto value:diag::values(r))check(value!=static_cast<std::int32_t>(r.observed.hands.left.object),"no hand-object identity in log fields");
  }
  for(unsigned cause=0;cause<5;++cause){Fixture f;
    auto& hand=f.observed.before_destroy.hands;
    switch(cause){
      case 0:hand.left_tmp_level=2;break;
      case 1:hand.right_tmp_level=1;break;
      case 2:hand.hands.left.object+=4;hand.interact_item+=4;break;
      case 3:hand.hands.left.symbol=16613;f.observed.generated_left_verified=false;break;
      case 4:f.observed.generated_left_verified=false;break;
    }
    const auto before=f.verify();auto r=diagnostic(f);
    check(before.issue==d::Issue::generated_hand&&!before.valid,"real typed guard separates hand failure from balance");
    if(cause==4){r.shape_attempted=3;r.shape_known=1;r.shape[1]=0;
      r.shape[2]=r.shape[3]=r.shape[4]=r.shape[5]=INT32_MIN;}
    check(diag::ready(r,before.issue,f.observed.before_destroy.stamp),"only bound failed hand has diagnostic packet");
    const auto bits=diag::flags(r);const auto values=diag::values(r);
    if(cause==0)check(!(bits&128u)&&!(bits&512u)&&values[3]==2&&values[8]==1,"left tmp change remains visible and rejected");
    if(cause==1)check(!(bits&128u)&&!(bits&1024u)&&values[5]==1&&values[9]==0,"empty-right tmp change remains visible and rejected");
    if(cause==2)check(!(bits&128u)&&!(bits&256u)&&!(bits&4096u),"different hand object diagnosed without pointer output");
    if(cause==3)check(!(bits&4u)&&values[1]==16613&&values[6]==16612,"changed hand symbol is distinct from prepared symbol");
    if(cause==4)check(!(bits&64u)&&((bits>>16)&63u)==3&&((bits>>24)&63u)==1&&
      values[11]==0&&values[12]==INT32_MIN,"failed read zero is not marked known; unattempted fields stay distinct");
    check(f.verify().issue==before.issue&&!f.verify().valid,"reading diagnostic values never changes typed guard");
    r.emitted=true;check(!diag::ready(r,before.issue,f.observed.before_destroy.stamp),"one packet at most");
  }
  for(unsigned fault=0;fault<14;++fault){Fixture f;f.observed.generated_left_verified=false;
    auto r=diagnostic(f);auto current=f.observed.before_destroy.stamp;
    switch(fault){
      case 0:r.captured=false;break;case 1:++current.job;break;case 2:++current.frame;break;
      case 3:++current.serial;break;case 4:++current.actual_thread;break;case 5:++current.key.hero;break;
      case 6:++current.key.generation;break;case 7:current.stable_boundary_verified=false;break;
      case 8:current.native_code_verified=false;break;case 9:current.hooks_owned=false;break;
      case 10:r.stamp.stable_boundary_verified=false;break;case 11:r.stamp.native_code_verified=false;break;
      case 12:r.stamp.hooks_owned=false;break;case 13:r.stamp.serial=0;break;
    }
    check(!diag::ready(r,d::Issue::generated_hand,current),"foreign stale incomplete or unbound packet never logs");
  }
  check(diag::signed_word(0)==0&&diag::signed_word(INT32_MAX)==INT32_MAX&&
    diag::signed_word(0x80000000u)==INT32_MIN&&diag::signed_word(UINT32_MAX)==-1,"raw shape words retain all32bits in signed log fields");
}
}
int main() {
  generated_hand_shape_tests();
  hand_diagnostic_tests();
  static_assert(std::is_trivially_copyable<d::Observation>::value&&std::is_trivially_copyable<d::Result>::value,"owned values only");
  static_assert(!d::native_effect_authorized&&!d::normal_end_verified,"no writer or normal-end authority");
  for(std::int64_t hot=1;hot<=10;++hot)for(const auto cold:{0,7,INT32_MAX-2})for(std::uint32_t q=1;q<=static_cast<std::uint32_t>(hot);++q){
    Fixture f(hot,cold,q);const bool expected=std::int64_t{q}<=std::int64_t{INT32_MAX}-cold&&!(hot==2&&q==2);
    const auto result=f.verify();check(result.valid==expected,"independent direct quantity domain");
    if(expected)check(result.issue==d::Issue::none&&result.serial==104&&result.frame==92&&result.instance==16612&&result.amount==1,"only positive destroy/remove result");
  }
  {Fixture f(3,9,1);check(f.observed.before_destroy.stocks.stocks[0].amount==3&&f.observed.after_destroy.stocks.stocks[0].amount==2&&f.verify().valid,"N1 Hot3 really means 3 to 2");
    f.observed.before_destroy.stocks.stocks[0].amount=1;rejected(f,"never assume HotBefore equals one");}
  {Fixture f(2,9,2);f.prepared.valid=true;f.prepared.path.amount.requires_extra_debit=false;f.prepared.path.amount.extra_debit=0;
    check(f.verify().issue==d::Issue::extra_debit_unsupported,"Hot2 N2 rejected even with forged prepared flags");}
  {Fixture f(INT32_MAX,0,INT32_MAX);check(f.verify().valid,"bounded INT32 quantity at exact capacity");}
  {Fixture f;f.observed.remove.opaque_return=1;check(f.verify().valid,"return remains opaque; no pointer inspection");}
  {Fixture f;for(auto* p:{&f.observed.before_destroy,&f.observed.before_remove,&f.observed.after_remove,&f.observed.after_destroy})p->stocks.stocks[1].unambiguous=false;
    check(f.verify().valid,"known Cold totals may span multiple stacks");}
  {Fixture f;f.observed.after_destroy.stamp.serial=UINT64_MAX;check(f.verify().valid,"last representable serial has no arithmetic overflow");
    f.binding.serial_floor=UINT64_MAX;rejected(f,"consumed serial floor prevents replay");}
  {Fixture f;f.observed.remove={};f.observed.remove_entries=f.observed.remove_exits=0;
    f.observed.before_remove={};f.observed.after_remove={};
    check(f.verify().issue==d::Issue::remove_count,"Destroy Return1 alone reports missing Remove precisely");}
  {Fixture f;f.observed.destroy.exception=true;f.observed.after_destroy={};
    check(f.verify().issue==d::Issue::exception,"original exception reports its cause before incomplete after-snapshot");}
  for(unsigned point=0;point<4;++point)for(unsigned mutation=0;mutation<15;++mutation){
    Fixture f;d::Snapshot* points[]{&f.observed.before_destroy,&f.observed.before_remove,&f.observed.after_remove,&f.observed.after_destroy};
    auto& p=*points[point];
    switch(mutation){
      case 0:++p.stamp.key.generation;break;case 1:++p.stamp.key.thread;break;
      case 2:++p.stamp.key.world;break;case 3:++p.stamp.key.hero;break;
      case 4:++p.stamp.key.station;break;case 5:++p.stamp.key.manager;break;
      case 6:p.stamp.key.production_kind=1;break;case 7:++p.stamp.job;break;
      case 8:++p.stamp.actual_thread;break;case 9:++p.stamp.frame;break;
      case 10:p.stamp.serial=100+point;break;case 11:p.stamp.stable_boundary_verified=false;break;
      case 12:p.stamp.native_code_verified=false;break;case 13:p.stamp.hooks_owned=false;break;
      case 14:p.stamp.serial=0;break;
    }
    rejected(f,"every point binds exact key job thread frame serial and verified ownership");
  }
  for(unsigned point=0;point<4;++point)for(unsigned mutation=0;mutation<12;++mutation){
    Fixture f;d::Snapshot* points[]{&f.observed.before_destroy,&f.observed.before_remove,&f.observed.after_remove,&f.observed.after_destroy};
    auto& s=points[point]->stocks;
    switch(mutation){
      case 0:s.stable_boundary_verified=false;break;case 1:--s.stocks[0].amount;break;
      case 2:++s.stocks[0].amount;break;case 3:--s.stocks[1].amount;break;case 4:++s.stocks[1].amount;break;
      case 5:s.stocks[0].known=false;break;case 6:s.stocks[1].known=false;break;
      case 7:s.stocks[0].unambiguous=false;break;case 8:s.stocks[1].symbol=16612;break;
      case 9:s.stocks[0].amount=-1;break;case 10:s.stocks[1].amount=std::int64_t{INT32_MAX}+1;break;
      case 11:s.stocks[0].symbol=-1;break;
    }
    rejected(f,"fresh exact hot and cold stocks at every boundary");
  }
  for(unsigned mutation=0;mutation<49;++mutation){Fixture f;auto& o=f.observed;
    switch(mutation){
      case 0:o.destroy_entries=0;break;case 1:o.destroy_entries=2;break;case 2:o.destroy_exits=0;break;case 3:o.destroy_exits=2;break;
      case 4:o.destroy.original_calls=0;break;case 5:o.destroy.original_calls=2;break;
      case 6:o.destroy.normal_return=false;break;case 7:o.destroy.return_value=0;break;case 8:o.destroy.return_value=2;break;
      case 9:o.destroy.exception=true;break;case 10:o.destroy.depth=2;break;case 11:o.destroy.caller=0x74c581;break;
      case 12:++o.destroy.npc;break;case 13:o.destroy.message=0;break;case 14:++o.destroy.message;break;
      case 15:o.destroy.classdef=0xab1168;break;case 16:o.destroy.subtype=8;break;case 17:o.destroy.message_class_verified=false;break;
      case 18:o.generated_left_verified=false;break;case 19:o.storage_alias_scan_known=false;break;case 20:o.generated_storage_alias=true;break;
      case 21:o.set_interact_item_normal_return_verified=false;break;case 22:o.before_remove.hands=active();break;
      case 23:o.remove_entries=0;break;case 24:o.remove_entries=2;break;case 25:o.remove_exits=0;break;case 26:o.remove_exits=2;break;
      case 27:o.remove.original_calls=0;break;case 28:o.remove.original_calls=2;break;
      case 29:o.remove.caller=0x754b97;break;case 30:++o.remove.inventory;break;case 31:++o.remove.loaded_vtable;break;
      case 32:o.remove.target=0x70ce20;break;case 33:o.remove.instance=16613;break;case 34:o.remove.amount=0;break;
      case 35:o.remove.amount=2;break;case 36:o.remove.opaque_return=0;break;case 37:o.remove.normal_return=false;break;
      case 38:o.remove.exception=true;break;case 39:o.remove.depth=2;break;case 40:o.reentry_seen=true;break;
      case 41:o.after_remove.hands=active();break;case 42:o.after_destroy.hands=active();break;
      case 43:o.after_destroy.hands.interact_item_known=false;break;case 44:o.after_destroy.hands.right_slot_known=false;break;
      case 45:o.after_destroy.hands.hands.right={2,-1,-1,0x960000};break;
      case 46:o.before_destroy.hands.hands.left.amount=2;break;case 47:o.before_destroy.hands.interact_item=0x910004;break;
      case 48:o.remove={};break;
    }
    rejected(f,"missing duplicate foreign aliased exceptional or incomplete effects never certify Destroy");
  }
  for(unsigned mutation=0;mutation<24;++mutation){Fixture f;auto& p=f.prepared;
    switch(mutation){
      case 0:p.valid=false;break;case 1:p.stock_known=false;break;case 2:p.issue=direct::Issue::hand;break;
      case 3:p.path.valid=false;break;case 4:++p.path.quantity;break;case 5:p.path.count=2;break;
      case 6:p.path.step.owner=20773;break;case 7:p.path.step.function=20775;break;
      case 8:++p.maximum;break;case 9:++p.direct_maximum;break;
      case 10:p.path.amount.native_end_debit=0;break;case 11:p.path.amount.requires_quantity_receipt=false;break;
      case 12:++p.path.amount.before_end[0].amount;break;case 13:++p.path.amount.after_end[1].amount;break;
      case 14:p.path.amount.count=3;break;case 15:++p.path.amount.operands[0].original;break;
      case 16:p.path.amount.operands[0].allow_zero=true;break;case 17:p.path.amount.valid=false;break;
      case 18:p.path.amount.extra_debit=1;break;case 19:p.stocks.stocks[0].amount=2;break;
      case 20:p.hands.hands.left.object+=4;p.hands.interact_item+=4;break;
      case 21:f.binding.recipe=0xa002;break;case 22:f.binding.quantity=0;break;case 23:f.binding.key.hero=UINT32_MAX-4;break;
    }
    rejected(f,"canonical plan freshly recomputed; prepared flags and copied numbers are insufficient");
  }
  check(!d::verify({}, {}, {}).valid,"empty inputs never pass");
  std::printf("cool_destroy_observation_tests: %u checks, %u failures\n",checks,failures);
  return failures?1:0;
}
