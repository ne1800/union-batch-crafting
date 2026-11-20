// SPDX-License-Identifier: MIT
#include "ubc/cool_direct_contract.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace d=ubc::cool_direct_contract;
namespace c=ubc::cool_recipe_catalog;
namespace n=ubc::cool_recipe_navigation;
namespace {
unsigned checks=0,failures=0;
void check(bool v,const char* message){++checks;if(!v){++failures;std::fprintf(stderr,"FAIL: %s\n",message);}}
constexpr std::uint32_t info_address=0x900000,hand_address=0x910000;
n::Page page(bool triple=true){return {true,true,triple};}
ubc::CoolRecipeCatalogueCapture capture(std::int32_t hot=7,std::int32_t cold=11){
  // Our synthetic value copies. No claim of native evidence.
  ubc::CoolRecipeCatalogueCapture out;
  out.key={23,7,0x400000,0x500000,0x600000,0x700000,13};
  auto& s=out.recipe;auto& diag=out.diagnostic;
  s.evaluated=s.known=s.contract=s.actor_known=s.actor_matches=s.context_known=s.context_matches=true;
  diag.inventory_unchanged=diag.hands_unchanged=diag.actor_unchanged=diag.context_unchanged=diag.knowledge_unchanged=true;
  diag.profile.valid=diag.profile.local.matches=true;
  auto& h=out.hands;h.left_slot_known=h.right_slot_known=h.interact_item_known=true;
  h.left_tmp_level=1;h.right_tmp_level=0;h.interact_item=hand_address;
  h.hands.left={1,16612,1,hand_address};h.hands.right={0,-1,-1,0};
  const std::array<ubc::RecipeInventoryItem,2> items{{
    {16612,hot,0,hot,hot>0?1u:0u,0,false,false,false},
    {16613,cold,0,cold,cold>0?1u:0u,0,false,false,false}}};
  const std::array<std::uint32_t,2> objects{{0x920000,0x930000}};
  ubc::RecipeInventory inventory;
  inventory.header_known=inventory.listed_known=inventory.packed_known=inventory.storage_totals_known=true;
  inventory.stable_boundary_verified=inventory.snapshot_unchanged=true;
  inventory.listed_nodes=static_cast<std::uint32_t>((hot>0)+(cold>0));
  inventory.items={items.data(),items.size()};inventory.listed_objects={objects.data(),inventory.listed_nodes};
  s.material=ubc::cool_recipe_material(inventory,h,0,{true,true,1},true);
  return out;
}
struct Memory {
  std::array<std::uint8_t,0x5c> first{},second{};
  unsigned reads=0,fail_on=0;
  void put(std::array<std::uint8_t,0x5c>& b,std::size_t offset,std::uint32_t v){
    for(unsigned i=0;i<4;++i)b[offset+i]=static_cast<std::uint8_t>(v>>(8u*i));
  }
  explicit Memory(const ubc::recipe_catalog::Owner& owner){
    put(first,0,ubc::info_detail::info_vtable);
    put(first,0x1c,static_cast<std::uint32_t>(owner.npc));put(first,0x20,static_cast<std::uint32_t>(owner.number));
    put(first,0x24,owner.important?1u:0u);put(first,0x28,owner.condition->symbol);put(first,0x2c,owner.information->symbol);
    put(first,0x44,owner.trade?1u:0u);put(first,0x48,owner.permanent?1u:0u);
    put(first,0x50,static_cast<std::uint32_t>(owner.instance));second=first;
  }
  static bool read(void* raw,std::uint32_t address,void* output,std::size_t length) noexcept {
    auto& self=*static_cast<Memory*>(raw);++self.reads;
    if(self.reads==self.fail_on||address!=info_address||length!=self.first.size())return false;
    const auto& bytes=self.reads==1?self.first:self.second;
    std::memcpy(output,bytes.data(),length);return true;
  }
  ubc::ReadOnlyMemory memory(){return {this,read};}
};
}
int main(){
  static_assert(!d::runtime_ready&&!c::runtime_ready,"portable values do not admit native calls");
  static_assert(std::is_trivially_copyable<d::Prepared>::value,"owned value result");
  for(std::int32_t hot=0;hot<=12;++hot)for(const auto cold:{0,11,INT32_MAX-2,INT32_MAX}){
    const auto source=capture(hot,cold);
    const auto maximum=static_cast<std::uint32_t>(hot<INT32_MAX-cold?hot:INT32_MAX-cold);
    for(std::uint32_t q=0;q<=static_cast<std::uint32_t>(hot)+1;++q){
      const auto prepared=d::prepare(source,page(hot>=3),q);
      const bool valid=q&&q<=maximum&&!(hot==2&&q==2);
      check(prepared.stock_known&&prepared.maximum==maximum,"full maximum remains visible");
      check(prepared.direct_maximum==(hot<3&&maximum>1?1:maximum),"only Hot2 maximum separates direct path");
      check(prepared.valid==valid,"canonical direct quantity domain");
      if(hot==2&&q==2&&q<=maximum)check(prepared.issue==d::Issue::extra_debit_unsupported,"Hot2 N2 explicit rejection");
      if(!valid)continue;
      const auto& plan=prepared.path.amount;const auto body=prepared.body();const auto needs=prepared.requirements();
      check(plan.quantity==q&&!plan.extra_debit&&!plan.requires_extra_debit&&plan.native_end_debit==1,"no own extra remover");
      check(needs.positive_destroy_after&&needs.normal_end_after&&needs.native_end_debit==1,"existing end obligations retained");
      check(needs.quantity_receipt==(q>1&&q!=3),"fixed triple text never accepted as arbitrary quantity");
      check(plan.after_end[0].amount==hot-static_cast<std::int64_t>(q)&&plan.after_end[1].amount==cold+static_cast<std::int64_t>(q),"exact final stock");
      check(plan.after_callback[0].amount==hot-static_cast<std::int64_t>(q-1),"script and normal end debit distinct");
      if(q==1)check(plan.callback==20775&&plan.owner==20773&&!body.count&&body.digest==c::single.information->body_sha256,"single uses unchanged exact body");
      else check(plan.callback==20778&&plan.owner==20776&&body.count==2&&body.operands[0].offset==62&&body.operands[0].reserved==1&&body.operands[1].offset==82&&body.operands[1].reserved==0,"triple uses existing N-1 and N operands");
      Memory mem(q==1?c::single:c::triple);
      const auto entry=d::bind(mem.memory(),prepared,source,page(hot>=3),info_address);
      check(entry.valid&&entry.actual_info==info_address&&mem.reads==2,"exact INFO bound twice through existing reader");
      const auto copy=entry;
      check(copy.valid&&copy.body().count==body.count&&
        (!body.count||copy.body().operands==copy.prepared.path.amount.operands.data()),"copied body view belongs to copy");
    }
  }
  const auto source=capture();const auto prepared=d::prepare(source,page(),2);
  check(prepared.valid&&prepared.maximum==7&&prepared.direct_maximum==7&&prepared.requirements().quantity_receipt,"Hot7 N2 admitted numerically with explicit receipt duty");
  check(d::prepare(source,page(),7).valid&&d::prepare(source,page(),7).requirements().quantity_receipt,"no artificial maximum3");
  check(!d::prepare(source,page(),UINT32_MAX).valid,"uint32 quantity overflow rejected");
  check(!d::prepare(source,{false,true,true},1).valid,"unknown navigation rejected");
  check(!d::prepare(source,{true,false,true},1).valid,"triple without single invalid");
  check(!d::prepare(source,page(false),2).valid&&d::prepare(source,page(false),1).valid,"actual absent triple not invented");
  {const auto hot2=capture(2);const auto p=d::prepare(hot2,page(false),2);Memory m(c::single);
    check(p.stock_known&&p.maximum==2&&p.direct_maximum==1&&!d::bind(m.memory(),p,hot2,page(false),info_address).valid&&!m.reads,"Hot2 N2 never reads INFO or starts an effect");}
  {const auto limited=capture(5,INT32_MAX-2);check(d::prepare(limited,page(),2).valid&&d::prepare(limited,page(),2).direct_maximum==2,"output room2 still permits original triple N2 with Hot5");}
  {const auto limit=capture(INT32_MAX,0);const auto p=d::prepare(limit,page(),INT32_MAX);
    check(p.valid&&p.maximum==INT32_MAX&&p.direct_maximum==INT32_MAX&&p.requirements().quantity_receipt,"INT32 quantity limit remains a direct numerical path");
    check(p.path.amount.after_end[0].amount==0&&p.path.amount.after_end[1].amount==INT32_MAX,"limit arithmetic without overflow");}
  for(unsigned mutation=0;mutation<44;++mutation){auto bad=source;
    switch(mutation){
      case 0:bad.key.generation=0;break;case 1:bad.key.thread=0;break;case 2:bad.key.world=0;break;
      case 3:bad.key.hero=0;break;case 4:bad.key.station=0;break;case 5:bad.key.manager=0;break;case 6:bad.key.production_kind=4;break;
      case 7:bad.recipe.evaluated=false;break;case 8:bad.recipe.known=false;break;case 9:bad.recipe.contract=false;break;
      case 10:bad.recipe.actor_known=false;break;case 11:bad.recipe.actor_matches=false;break;
      case 12:bad.recipe.context_known=false;break;case 13:bad.recipe.context_matches=false;break;
      case 14:bad.diagnostic.inventory_unchanged=false;break;case 15:bad.diagnostic.hands_unchanged=false;break;
      case 16:bad.diagnostic.actor_unchanged=false;break;case 17:bad.diagnostic.context_unchanged=false;break;
      case 18:bad.diagnostic.knowledge_unchanged=false;break;case 19:bad.diagnostic.profile.valid=false;break;
      case 20:bad.diagnostic.profile.local.matches=false;break;case 21:bad.recipe.material.inventory_known=false;break;
      case 22:bad.recipe.material.hand_known=false;break;case 23:bad.recipe.material.ingredients_verified=false;break;
      case 24:bad.hands.left_slot_known=false;break;case 25:bad.hands.right_slot_known=false;break;
      case 26:bad.hands.interact_item_known=false;break;case 27:bad.hands.interact_item+=4;break;
      case 28:bad.hands.hands.left.symbol=16613;break;case 29:bad.hands.hands.left.amount=2;break;
      case 30:bad.hands.hands.right.object=hand_address;break;case 31:bad.hands.left_tmp_level=0;break;
      case 32:bad.recipe.material.rows[0].available+=1;break;case 33:bad.recipe.material.rows[0].held=0;break;
      case 34:bad.recipe.material.rows[0].reserved=1;break;case 35:bad.recipe.material.rows[0].symbol=16613;break;
      case 36:bad.recipe.material.rows[0].unit_cost=2;break;case 37:bad.recipe.material.rows[1].unit_output=0;break;
      case 38:bad.recipe.material.maximum=3;break;case 39:bad.recipe.material.direct_maximum=3;break;
      case 40:bad.recipe.material.rows[1].available_known=false;break;case 41:bad.recipe.material.rows[0].stored=-1;break;
      case 42:bad.diagnostic.issue=ubc::CoolCaptureIssue::boundary;break;case 43:bad.hands.hands.right.symbol=16612;break;
    }
    Memory mem(c::triple);
    check(!d::prepare(bad,page(),2).valid,"incomplete capture cannot prepare direct effect");
    check(!d::bind(mem.memory(),prepared,bad,page(),info_address).valid&&!mem.reads,"bad late capture never reads INFO");
  }
  for(unsigned mutation=0;mutation<12;++mutation){auto late=source;
    switch(mutation){
      case 0:++late.key.generation;break;case 1:++late.key.thread;break;case 2:++late.key.world;break;
      case 3:++late.key.hero;break;case 4:++late.key.station;break;case 5:++late.key.manager;break;
      case 6:late=capture(8,11);break;case 7:late=capture(7,12);break;
      case 8:late.hands.hands.left.object+=4;late.hands.interact_item+=4;break;
      case 9:late.hands.left_tmp_level=2;break;case 10:late.hands.right_tmp_level=1;break;
      case 11:late=capture(2,11);break;
    }
    Memory mem(c::triple);check(!d::bind(mem.memory(),prepared,late,page(),info_address).valid&&!mem.reads,"different late job or stocks cannot rebind old request");
  }
  for(unsigned mutation=0;mutation<15;++mutation){auto bad=prepared;
    switch(mutation){case 0:bad.valid=false;break;case 1:bad.path.quantity=3;break;case 2:bad.path.count=2;break;
      case 3:bad.path.step.owner=20773;break;case 4:bad.path.step.function=20775;break;case 5:bad.path.amount.callback=20775;break;
      case 6:bad.path.amount.owner=20773;break;case 7:bad.path.amount.operands[0].reserved=0;break;
      case 8:bad.path.amount.requires_quantity_receipt=false;break;case 9:bad.path.amount.native_end_debit=0;break;
      case 10:bad.path.amount.after_end[0].amount++;break;case 11:bad.maximum=3;break;case 12:bad.path.amount.extra_debit=1;break;
      case 13:bad.stock_known=false;break;case 14:bad.issue=d::Issue::changed;break;}
    Memory mem(c::triple);check(!d::bind(mem.memory(),bad,source,page(),info_address).valid&&!mem.reads,"tampered cached plan cannot bind");
  }
  for(const auto offset:{0u,0x1cu,0x20u,0x24u,0x28u,0x2cu,0x44u,0x48u,0x50u,0x58u})
    for(const auto pass:{1u,2u}){Memory mem(c::triple);auto& bytes=pass==1?mem.first:mem.second;
      bytes[offset]^=1u;check(!d::bind(mem.memory(),prepared,source,page(),info_address).valid,"all actual INFO metadata and late changes fail closed");}
  for(const auto failure:{1u,2u}){Memory mem(c::triple);mem.fail_on=failure;check(!d::bind(mem.memory(),prepared,source,page(),info_address).valid,"unreadable actual INFO rejected");}
  for(const auto pointer:{0u,info_address+1u,0xffffffd0u,info_address+4u}){Memory mem(c::triple);check(!d::bind(mem.memory(),prepared,source,page(),pointer).valid,"unknown misaligned overflow or foreign INFO rejected");}
  {Memory wrong(c::single);check(!d::bind(wrong.memory(),prepared,source,page(),info_address).valid,"single owner cannot execute triple even with same display number");}
  {Memory mem(c::triple);const auto before=mem.first;const auto entry=d::bind(mem.memory(),prepared,source,page(),info_address);
    check(entry.valid&&mem.first==before&&mem.second==before,"binding is read only");}
  std::printf("cool_direct_contract_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
