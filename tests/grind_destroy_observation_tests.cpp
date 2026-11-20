// SPDX-License-Identifier: MIT
// Our own synthetic values only. No native or game test and no memory reads.
#include "ubc/grind_result_diagnostic.hpp"
#include "ubc/generated_blade_hand_shape.hpp"
#include "ubc/grind_item_shape.hpp"
#include <cstdio>
#include <string_view>
#include <type_traits>
namespace d = ubc::grind_destroy_observation;
namespace g = ubc::grind_recipe;
namespace handshape = ubc::generated_blade_hand_shape;
namespace {
unsigned checks = 0, failures = 0;
void check(bool value, const char* label) {
  ++checks;
  if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", label); }
}
ubc::StationRecipeHands active() {
  ubc::StationRecipeHands h;
  h.left_slot_known = h.right_slot_known = h.interact_item_known = true;
  h.hands.left = {1,16613,1,0x910000}; h.hands.right = {0,-1,-1,0};
  h.left_tmp_level = 1; h.right_tmp_level = 0; h.interact_item = 0x910000;
  return h;
}
ubc::StationRecipeHands empty() {
  auto h = active(); h.hands.left = {0,-1,-1,0}; h.interact_item = 0; return h;
}
d::Stocks stocks(const g::Descriptor& recipe, std::int64_t input,
    std::int64_t output, std::int64_t cold) {
  return {true, {{{recipe.input,input,true,true}, {recipe.output,output,true,true},
                  {16613,cold,true,true}}}};
}
struct Fixture {
  d::Prepared prepared{};
  d::Binding binding{};
  d::Observation observed{};
  Fixture(const g::Descriptor& recipe = g::recipes.front(), std::uint32_t quantity = 2,
      std::int64_t input = 5, std::int64_t output = 3, std::int64_t cold = 7,
      std::int32_t skill = 97) {
    prepared.key = {23,80,7,0x400000,0x500000,0x600000,0x700000,11};
    prepared.job = 91; prepared.hands = active();
    prepared.before.callback = recipe.function.symbol;
    prepared.before.stable_boundary_verified = true;
    prepared.before.talent_known = prepared.before.guild_known = prepared.before.skill_known = true;
    prepared.before.talent = 1; prepared.before.guild = recipe.guilds[0]; prepared.before.skill = skill;
    prepared.before.stocks = stocks(recipe,input,output,cold).stocks;
    prepared.amount = g::plan(recipe,prepared.before,quantity);
    binding.key = {23,92,7,0x400000,0x500000,0x600000,0x700000,11};
    binding.job = 91; binding.frame = 92; binding.serial_floor = 100;
    binding.callback = recipe.function.symbol; binding.quantity = quantity;
    d::Snapshot* points[]{&observed.before_destroy,&observed.before_remove,&observed.after_remove,&observed.after_destroy};
    for (unsigned i=0;i<4;++i) {
      auto& p = *points[i];
      p.stamp = {binding.key,binding.job,binding.frame,101u+i,binding.key.thread,true,true,true};
      // Deliberately calculate independently of Plan.after_callback/after_end.
      p.stocks = stocks(recipe,input-quantity,output+quantity,cold+(i<2?1:0));
      p.hands = i?empty():active();
    }
    observed.destroy = {0x74c586,binding.key.hero,0x920000,0xab2860,9,1,1,true,true,false,1};
    observed.remove = {0x754b98,binding.key.hero+0x668,0x83c714,0x70d080,16613,1,1,1,0x930000,true,false};
    observed.destroy_entries = observed.destroy_exits = observed.remove_entries = observed.remove_exits = 1;
    observed.generated_left_verified = handshape::matches(16613,0x83c804,1,0xa00001);
    observed.storage_alias_scan_known = true;
    observed.set_interact_item_normal_return_verified = true;
  }
  d::Result verify() const { return d::verify(prepared,binding,observed); }
};
void rejected(const Fixture& f, const char* why) {
  const auto r = f.verify();
  check(!r.valid && r.issue!=d::Issue::none && !r.serial && !r.frame && r.instance==-1 && !r.amount,why);
}
void generated_hand_shape_tests() {
  const auto* stored=ubc::grind_item_shape::find(16613);
  check(stored&&ubc::grind_item_shape::bound_to_profile(*stored)&&stored->mainflag==1&&
    stored->native_flags==0x200001,"ColdBlade stored constructor shape remains exact and unchanged");
  {Fixture f;check(f.verify().valid,"exact NFOCUS generated ColdBlade shape reaches existing typed contract");}
  for(unsigned bit=0;bit<32;++bit){Fixture f;
    f.observed.generated_left_verified=handshape::matches(16613,0x83c804,1,0xa00001u^(1u<<bit));
    check(f.verify().issue==d::Issue::generated_hand&&!f.verify().valid,
      "every missing or additional generated ColdBlade flag bit fails despite exact weapon and Cold balances");
  }
  for(unsigned fault=0;fault<4;++fault){Fixture f;
    f.observed.generated_left_verified=handshape::matches(fault==0?16614:16613,
      fault==1?0u:0x83c804u,fault==2?2u:1u,fault==3?0x200001u:0xa00001u);
    check(f.verify().issue==d::Issue::generated_hand&&!f.verify().valid,
      "wrong ColdBlade identity or stored shape cannot supply a generated hand proof");
  }
}
}
int main() {
  {
    Fixture f;
    ubc::GrindCheck first;
    f.observed.remove.amount=2;
    auto result=d::verify(f.prepared,f.binding,f.observed,first);
    check(!result.valid && result.issue==d::Issue::remove_arguments && first.field &&
      std::string_view(first.field)=="remove.amount" && first.expected==1 && first.actual==2,
      "wrong Remove amount reports the measured operand before the generic issue");
    ubc::GrindCheck wide;
    wide.equal_unsigned("destroy.job",UINT64_MAX,UINT64_MAX-1);
    char fields[176]{};
    wide.format(fields);
    check(std::string_view(fields).find("18446744073709551615")!=std::string_view::npos,
      "unsigned job generation keeps all 64 bits in the rejection");
    ubc::GrindCheck clean;
    f.observed.remove.amount=1;
    check(d::verify(f.prepared,f.binding,f.observed,clean).valid && !clean.field,
      "a fresh valid observation has no stale rejection");
  }
  generated_hand_shape_tests();
  {
    Fixture f;
    ubc::GrindCheck check_stock, check_frame;
    auto& row=f.observed.after_remove.stocks.stocks[1];
    const auto wanted=row.amount;
    ++row.amount;
    auto result=d::verify(f.prepared,f.binding,f.observed,check_stock);
    check(result.issue==d::Issue::inner_stock && check_stock.field &&
      std::string_view(check_stock.point)=="after_remove" && std::string_view(check_stock.field)=="stock.amount" &&
      check_stock.symbol==row.symbol && check_stock.expected==static_cast<std::uint64_t>(wanted) &&
      check_stock.actual==static_cast<std::uint64_t>(wanted+1),"Remove reports measured stock and item at the inner return");
    ++f.observed.before_remove.stamp.frame;
    result=d::verify(f.prepared,f.binding,f.observed,check_frame);
    check(result.issue==d::Issue::frame && check_frame.field && std::string_view(check_frame.point)=="before_remove" &&
      std::string_view(check_frame.field)=="stamp.frame" && check_frame.actual==f.binding.frame+1 &&
      check_frame.expected==f.binding.frame,"stamp names point and conflicting frame");
    auto different=check_frame;
    check(different.same(check_frame),"same captured rejection is deduplicated");
    ++different.actual;
    check(!different.same(check_frame),"changed actual is retained");
    different=check_frame; different.field="stamp.key_frame";
    check(!different.same(check_frame),"different field with the same phase and values is retained");

    ubc::CraftingResultObservation value;
    std::array<ubc::RecipeResultItem,3> actual{};
    for(std::size_t i=0;i<actual.size();++i)
      actual[i]={f.prepared.amount.after_end[i].symbol,f.prepared.amount.after_end[i].amount};
    value.stock_known=true;value.items={actual.data(),actual.size()};
    ++actual[0].total;
    const auto balance=ubc::diagnose_grind_result(ubc::CraftingResultIssue::stock,f.prepared,value,12);
    check(balance.field && std::string_view(balance.field)=="stock.amount" && std::string_view(balance.point)=="result" &&
      balance.symbol==actual[0].symbol && balance.actual==balance.expected+1,"final stock reports real totals");
    value.grind={true,f.prepared.before.talent,f.prepared.before.guild,f.prepared.amount.skill_after-1};
    const auto skill=ubc::diagnose_grind_result(ubc::CraftingResultIssue::skill,f.prepared,value,12);
    check(skill.field && std::string_view(skill.field)=="skill.value" && skill.actual+1==skill.expected,
      "final skill reports the original capped expectation");
    ubc::StationEndCall end;end.before.production=0;end.after.production=11;
    const auto refused=ubc::diagnose_grind_end(ubc::StationEndIssue::production,{},end);
    check(refused.field && std::string_view(refused.point)=="after_end" && refused.expected==0 && refused.actual==11,
      "end rejection retains before/after and production value");
  }
  static_assert(std::is_trivially_copyable<d::Prepared>::value &&
                std::is_trivially_copyable<d::Observation>::value &&
                std::is_trivially_copyable<d::Result>::value,"owned copied values only");
  static_assert(std::is_same<d::Destroy,ubc::cool_destroy_observation::Destroy>::value &&
                std::is_same<d::Remove,ubc::cool_destroy_observation::Remove>::value &&
                std::is_same<d::Result,ubc::cool_destroy_observation::Result>::value,"shared event values");
  static_assert(!d::native_effect_authorized && !d::normal_end_verified,"no writer or separate end authority");
  for (const auto& recipe:g::recipes) for (const auto quantity:{1u,2u,5u})
    for (const auto output:{0,INT32_MAX-5}) for (const auto cold:{0,7,INT32_MAX-1})
      for (const auto skill:{INT32_MIN,0,99,100,INT32_MAX}) {
        Fixture f(recipe,quantity,5,output,cold,skill);
        const auto r=f.verify();
        check(r.valid && r.issue==d::Issue::none,"all22 original profiles: N weapons, exactly one Cold lifecycle debit");
        check(r.serial==104 && r.frame==92 && r.instance==16613 && r.amount==1,
              "positive witness contains only actual Destroy/Remove result");
      }
  {Fixture f(g::recipes.front(),INT32_MAX,INT32_MAX,0,0,INT32_MIN);
    check(f.verify().valid,"INT32 boundary quantity preserves fixed one-Cold intermediate");}
  {Fixture f(g::recipes.front(),1,3,9,0);
    check(f.observed.before_destroy.stocks.stocks[2].amount==1 &&
          f.observed.after_destroy.stocks.stocks[2].amount==0 && f.verify().valid,
          "Cold0 really means 1 before Remove and 0 after");}
  {Fixture f(g::recipes.front(),5,5,9,7);
    check(f.observed.before_destroy.stocks.stocks[2].amount==8 &&
          f.observed.after_destroy.stocks.stocks[2].amount==7 && f.verify().valid,
          "N5 Cold7 means 8 to7, never N-fold lifecycle or assumed before1");
    f.observed.before_destroy.stocks.stocks[2].amount=12;
    rejected(f,"N-fold Cold callback output is not the original plan");}
  {Fixture f; f.prepared.key.frame=f.binding.frame; check(f.verify().valid,"same-frame original path remains allowed");
    ++f.prepared.key.frame; check(f.verify().issue==d::Issue::frame,"future prepared frame cannot bind old Destroy");}
  {Fixture f; f.observed.remove.opaque_return=1; check(f.verify().valid,"opaque return is never dereferenced");}
  {Fixture f; f.observed.after_destroy.stamp.serial=UINT64_MAX; check(f.verify().valid,"final serial uses no overflowing increment");
    f.binding.serial_floor=UINT64_MAX; rejected(f,"consumed serial floor rejects replay");}
  {Fixture f; for(auto* p:{&f.observed.before_destroy,&f.observed.before_remove,&f.observed.after_remove,&f.observed.after_destroy})
      for(auto& s:p->stocks.stocks)s.unambiguous=false;
    check(f.verify().valid,"fresh total quantities do not invent post-callback stack uniqueness");}
  {Fixture f; f.prepared.before.stocks[0].unambiguous=false;
    rejected(f,"original weapon input still requires canonical input guard");}
  {Fixture f; f.prepared.hands.left_tmp_level=f.observed.before_destroy.hands.left_tmp_level=2;
    check(f.verify().valid,"existing Grind generated tmp2 hand contract");}
  {Fixture f; f.observed.remove={}; f.observed.remove_entries=f.observed.remove_exits=0;
    f.observed.before_remove={}; f.observed.after_remove={};
    check(f.verify().issue==d::Issue::remove_count,"Destroy Return1 alone has no positive Remove witness");}
  {Fixture f; f.observed.remove.exception=true; f.observed.after_remove={}; f.observed.after_destroy={};
    check(f.verify().issue==d::Issue::exception,"exception reported before incomplete after-snapshots");}

  for (unsigned point=0;point<4;++point) for (unsigned mutation=0;mutation<16;++mutation) {
    Fixture f; d::Snapshot* points[]{&f.observed.before_destroy,&f.observed.before_remove,&f.observed.after_remove,&f.observed.after_destroy};
    auto& s=points[point]->stamp;
    switch(mutation) {
      case 0:++s.key.generation;break;case 1:++s.key.frame;break;
      case 2:++s.key.thread;break;case 3:++s.key.world;break;case 4:++s.key.hero;break;
      case 5:++s.key.station;break;case 6:++s.key.manager;break;case 7:s.key.production_kind=13;break;
      case 8:++s.job;break;case 9:++s.frame;break;case 10:++s.actual_thread;break;
      case 11:s.serial=100+point;break;case 12:s.serial=0;break;
      case 13:s.stable_boundary_verified=false;break;case 14:s.native_code_verified=false;break;case 15:s.hooks_owned=false;break;
    }
    rejected(f,"every fresh boundary binds own Grind identity, frame, serial and actual native ownership");
  }
  for (unsigned point=0;point<4;++point) for(unsigned row=0;row<3;++row) for(unsigned mutation=0;mutation<7;++mutation) {
    Fixture f; d::Snapshot* points[]{&f.observed.before_destroy,&f.observed.before_remove,&f.observed.after_remove,&f.observed.after_destroy};
    auto& all=points[point]->stocks; auto& s=all.stocks[row];
    switch(mutation) {
      case 0:--s.amount;break;case 1:++s.amount;break;case 2:s.amount=-1;break;
      case 3:s.amount=std::int64_t{INT32_MAX}+1;break;case 4:s.known=false;break;
      case 5:s.symbol=all.stocks[(row+1)%3].symbol;break;case 6:all.stable_boundary_verified=false;break;
    }
    rejected(f,"real input/output/Cold totals are exact at all four boundaries");
  }
  for(unsigned mutation=0;mutation<36;++mutation) {
    Fixture f;
    switch(mutation) {
      case 0:f.binding.callback=0;break;case 1:f.binding.callback=g::recipes[1].function.symbol;break;
      case 2:f.binding.quantity=0;break;case 3:++f.binding.quantity;break;case 4:f.binding.quantity=UINT32_MAX;break;
      case 5:++f.binding.frame;break;case 6:f.binding.job=0;break;case 7:++f.prepared.job;break;
      case 8:f.prepared.key.frame=0;break;case 9:++f.prepared.key.generation;break;
      case 10:++f.prepared.key.thread;break;case 11:++f.prepared.key.world;break;case 12:++f.prepared.key.hero;break;
      case 13:++f.prepared.key.station;break;case 14:++f.prepared.key.manager;break;case 15:f.prepared.key.production_kind=13;break;
      case 16:f.prepared.before.callback=0;break;case 17:f.prepared.before.stable_boundary_verified=false;break;
      case 18:f.prepared.before.talent_known=false;break;case 19:f.prepared.before.guild_known=false;break;
      case 20:f.prepared.before.skill_known=false;break;case 21:f.prepared.before.talent=0;break;case 22:f.prepared.before.guild=99;break;
      case 23:++f.prepared.before.skill;break;case 24:f.prepared.before.stocks[0].amount=0;break;
      case 25:f.prepared.before.stocks[2].amount=INT32_MAX;break;case 26:f.prepared.amount.valid=false;break;
      case 27:f.prepared.amount.recipe=&g::recipes[1];break;case 28:++f.prepared.amount.maximum;break;
      case 29:++f.prepared.amount.skill_after;break;case 30:f.prepared.amount.count=5;break;
      case 31:f.prepared.amount.operands[0].offset=293;break;case 32:++f.prepared.amount.operands[1].units;break;
      case 33:++f.prepared.amount.after_callback[2].amount;break;case 34:++f.prepared.amount.after_end[1].amount;break;
      case 35:f.prepared.amount.learned=false;break;
    }
    rejected(f,"copied plan/key/knowledge/operands never replace fresh canonical Grind calculation");
  }
  for(unsigned mutation=0;mutation<44;++mutation) {
    Fixture f; auto& o=f.observed;
    switch(mutation) {
      case 0:o.destroy_entries=0;break;case 1:++o.destroy_entries;break;case 2:o.destroy_exits=0;break;case 3:++o.destroy.original_calls;break;
      case 4:o.remove_entries=0;break;case 5:++o.remove_entries;break;case 6:o.remove_exits=0;break;case 7:++o.remove.original_calls;break;
      case 8:o.reentry_seen=true;break;case 9:++o.destroy.depth;break;case 10:++o.remove.depth;break;
      case 11:++o.destroy.caller;break;case 12:++o.destroy.npc;break;case 13:o.destroy.message=0;break;
      case 14:++o.destroy.message;break;case 15:o.destroy.message_class_verified=false;break;
      case 16:++o.destroy.classdef;break;case 17:++o.destroy.subtype;break;case 18:o.destroy.normal_return=false;break;
      case 19:o.destroy.return_value=0;break;case 20:o.generated_left_verified=false;break;
      case 21:o.storage_alias_scan_known=false;break;case 22:o.generated_storage_alias=true;break;
      case 23:o.before_destroy.hands.hands.left.symbol=16612;break;case 24:++o.before_destroy.hands.hands.left.amount;break;
      case 25:o.before_destroy.hands.interact_item=0;break;case 26:o.before_destroy.hands.left_tmp_level=0;break;
      case 27:o.before_destroy.hands.hands.right={1,16613,1,0x900004};break;
      case 28:o.before_destroy.hands.hands.left.object+=4;o.before_destroy.hands.interact_item+=4;break;
      case 29:o.set_interact_item_normal_return_verified=false;break;case 30:o.before_remove.hands=active();break;
      case 31:++o.remove.caller;break;case 32:++o.remove.inventory;break;case 33:++o.remove.loaded_vtable;break;
      case 34:++o.remove.target;break;case 35:o.remove.instance=16612;break;case 36:o.remove.amount=2;break;
      case 37:o.remove.normal_return=false;break;case 38:o.remove.opaque_return=0;break;
      case 39:o.after_remove.hands=active();break;case 40:o.after_destroy.hands.interact_item=1;break;
      case 41:o.after_destroy.hands.right_slot_known=false;break;case 42:o.destroy.exception=true;break;
      case 43:o.after_destroy.hands.hands.left.symbol=16613;break;
    }
    rejected(f,"precise original calls, generated16613 hand, alias exclusion and empty after-hands required");
  }
  {Fixture f; for(auto* p:{&f.observed.before_destroy,&f.observed.before_remove,&f.observed.after_remove,&f.observed.after_destroy}) {
      const auto first=p->stocks.stocks[0];p->stocks.stocks[0]=p->stocks.stocks[2];p->stocks.stocks[2]=first;
    }
    check(f.verify().valid,"stock mapping uses actual symbols rather than accidental row order");}
  std::printf("grind_destroy_observation_tests: %u checks, %u failures\n",checks,failures);
  return failures?1:0;
}
