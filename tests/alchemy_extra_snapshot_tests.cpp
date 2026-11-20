// SPDX-License-Identifier: MIT
#include "ubc/stations/alchemy/alchemy_extra_snapshot.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace extra_test {
namespace cat=ubc::alchemy_recipe_catalog;
unsigned checks=0,failures=0;
void check(bool value,int line) {++checks;if(!value){++failures;std::fprintf(stderr,"failure line %d\n",line);}}
#define CHECK(value) extra_test::check((value),__LINE__)
struct Fixture {
  const cat::Descriptor& recipe;
  ubc::AlchemyExtraSnapshotInput input;
  std::vector<ubc::RecipeInventoryItem> items;
  std::vector<std::uint32_t> objects;
  explicit Fixture(const cat::Descriptor& r):recipe(r) {
    input.context={1,7,ubc::lhiver::game_parser,0x1000,0x2000,0x3000,0x4000};input.serial=10;
    input.boundary_verified=input.local_recipe_contract_verified=input.hand_contract_verified=true;
    input.interact_item_known=input.native_debit_contract_verified=true;input.interact_item=0x9000;
    input.hands={{0,-1,-1,0},{1,16604,1,0x9000}};
    input.knowledge.bindings_verified=input.knowledge.unchanged=true;input.knowledge.count=r.requirements.count;
    for(std::size_t i=0;i<r.requirements.count;++i) {const auto& q=r.requirements[i];input.knowledge.values[i]={q.symbol,q.element,q.value};}
    for(const auto& t:r.targets) {
      const auto amount=t.symbol==16604?5:(cat::units(r.costs,t.symbol)?cat::units(r.costs,t.symbol)*20:2);
      items.push_back({t.symbol,static_cast<std::int32_t>(amount),0,static_cast<std::int32_t>(amount),1,0,false,false,false});
    }
    input.inventory.header_known=input.inventory.listed_known=input.inventory.packed_known=input.inventory.storage_totals_known=true;
    input.inventory.stable_boundary_verified=input.inventory.snapshot_unchanged=true;refresh();
  }
  Fixture(const Fixture&)=delete;Fixture& operator=(const Fixture&)=delete;
  ubc::RecipeInventoryItem& item(std::int32_t symbol) {for(auto& i:items)if(i.symbol==symbol)return i;std::abort();}
  void set_total(std::int32_t symbol,std::int32_t count) {auto& i=item(symbol);i.listed=i.total=count;i.packed=0;i.packed_occurrences=0;i.listed_occurrences=count>0?1u:0u;refresh();}
  void packed(std::int32_t symbol,std::int32_t count) {auto& i=item(symbol);i.packed=i.total=count;i.listed=0;i.listed_occurrences=0;i.packed_occurrences=count>0?1u:0u;refresh();}
  void refresh() {
    objects.clear();input.inventory.listed_nodes=0;input.inventory.packed_entries=0;
    for(std::size_t i=0;i<items.size();++i) {
      const auto& item=items[i];input.inventory.listed_nodes+=item.listed_occurrences;input.inventory.packed_entries+=item.packed_occurrences;
      if(item.listed_occurrences)objects.push_back(0x10000+static_cast<std::uint32_t>(i)*0x400);
      if(item.symbol==16604)input.flask={true,true,true,item.packed,item.listed_occurrences,
        item.listed_occurrences?0x10000+static_cast<std::uint32_t>(i)*0x400:0,
        static_cast<std::uint32_t>(item.listed),item.listed_occurrences?0x200001u:0u};
    }
    input.inventory.items={items.data(),items.size()};input.inventory.listed_objects={objects.data(),objects.size()};
  }
  ubc::AlchemyExtraSnapshot snapshot(std::uint32_t n) const {return ubc::alchemy_extra_snapshot(recipe,input,n);}
};

struct KnowledgeMemory {
  static constexpr std::uint32_t table=0x100000,code=0x200000;
  std::map<std::uint32_t,std::vector<std::uint8_t>> blocks;
  std::uint32_t denied=0,watch=0,change=0;
  unsigned watches=0,change_on=0;
  static std::uint32_t symbol(std::uint32_t i) {return 0x10000000+i*0x80;}
  static std::uint32_t array(std::uint32_t i) {return 0x20000000+i*0x80;}
  static std::uint32_t name(std::uint32_t i) {return 0x30000000+i*0x80;}
  void block(std::uint32_t a,std::size_t n) {blocks[a].resize(n);}
  std::uint8_t* at(std::uint32_t a,std::size_t n) {
    auto it=blocks.upper_bound(a);if(it==blocks.begin())return nullptr;--it;const auto off=static_cast<std::size_t>(a-it->first);
    return off<=it->second.size() && n<=it->second.size()-off?it->second.data()+off:nullptr;
  }
  void put(std::uint32_t a,std::uint32_t v) {auto* p=at(a,4);if(!p)std::abort();for(unsigned i=0;i<4;++i)p[i]=static_cast<std::uint8_t>(v>>(8*i));}
  explicit KnowledgeMemory(const cat::Descriptor& r) {
    block(ubc::lhiver::game_parser,0x58);block(table,ubc::lhiver::symbol_count*4);
    put(ubc::lhiver::game_parser+0x18,table);put(ubc::lhiver::game_parser+0x1c,ubc::lhiver::symbol_count);put(ubc::lhiver::game_parser+0x20,ubc::lhiver::symbol_count);
    put(ubc::lhiver::game_parser+0x48,code);put(ubc::lhiver::game_parser+0x50,code+ubc::lhiver::code_length);put(ubc::lhiver::game_parser+0x54,ubc::lhiver::code_length);
    for(const auto& q:r.requirements) {
      const auto n=static_cast<std::uint32_t>(q.symbol);const auto* b=cat::binding_for(r,n);if(!b)std::abort();
      block(symbol(n),0x3c);put(table+n*4,symbol(n));const auto len=std::strlen(b->name);block(name(n),len+1);
      std::memcpy(at(name(n),len+1),b->name,len+1);put(symbol(n)+8,name(n));put(symbol(n)+12,static_cast<std::uint32_t>(len));
      put(symbol(n)+0x20,b->elements|(b->type<<12)|(b->flags<<16));
      if(b->elements>1) {block(array(n),b->elements*4);put(symbol(n)+0x18,array(n));put(array(n)+q.element*4,static_cast<std::uint32_t>(q.value));}
      else put(symbol(n)+0x18,static_cast<std::uint32_t>(q.value));
    }
  }
  static bool read(void* p,std::uint32_t a,void* out,std::size_t n) noexcept {
    auto& self=*static_cast<KnowledgeMemory*>(p);
    if(a==self.watch && ++self.watches==self.change_on)self.put(self.change,0);
    if(self.denied && a<=self.denied && std::uint64_t{self.denied}<std::uint64_t{a}+n)return false;
    auto* bytes=self.at(a,n);if(!bytes)return false;std::memcpy(out,bytes,n);return true;
  }
  ubc::ReadOnlyMemory memory() {return {this,&read};}
};

void material_cases() {
  for(const auto& r:cat::recipes) {
    Fixture f(r);
    for(auto n:{0u,1u,2u,6u}) {
      const auto s=f.snapshot(n);CHECK(s.visible && s.learned_known && s.learned && s.owner_conditions_match);
      CHECK(s.material.known() && s.material.maximum==6 && s.maximum_executable==6);
      CHECK(s.can_select==(n>0) && s.effect_ready==(n>0));
      CHECK(s.extra_flask_stack_verified && s.hand.known && s.hand.right==0x9000);
      for(std::size_t i=0;i<s.material.count;++i) {
        const auto& row=s.rows[i];CHECK(row.required==std::int64_t{row.unit_cost}*n && row.produced==std::int64_t{row.unit_output}*n);
        CHECK(row.remaining==row.stored+(row.symbol==16604?1:0)-row.required+row.produced);
        if(row.symbol==16604)CHECK(row.available==6 && row.reserved==1 && row.remaining==6-n);
      }
    }
    CHECK(!f.snapshot(7).can_select && f.snapshot(7).visible);CHECK(!f.snapshot(UINT32_MAX).can_select);
    f.set_total(r.costs[0].symbol,0);auto s=f.snapshot(1);CHECK(s.visible && s.material.known() && s.material.maximum==0 && !s.can_select);
    CHECK(s.rows[0].missing_for_one==r.costs[0].units);
    f.set_total(r.costs[0].symbol,static_cast<std::int32_t>(r.costs[0].units*20));f.set_total(r.outputs[0].symbol,INT32_MAX);
    s=f.snapshot(0);CHECK(s.visible && s.material.maximum==0 && s.rows[r.targets.count-1].output_shortfall_for_one==1);
    f.set_total(r.outputs[0].symbol,INT32_MAX-1);CHECK(f.snapshot(1).can_select && f.snapshot(1).material.maximum==1);
    f.set_total(r.outputs[0].symbol,2);f.packed(16604,5);
    s=f.snapshot(1);CHECK(s.visible && s.material.maximum==6 && s.maximum_executable==1 && s.can_select && !s.extra_flask_stack_verified);
    s=f.snapshot(2);CHECK(s.visible && !s.can_select && s.issue==ubc::AlchemyExtraSnapshotIssue::extra_flask_stack);
    f.set_total(16604,0);s=f.snapshot(1);CHECK(s.can_select && s.material.maximum==1 && s.extra_flask_stack_verified);
    f.input.hand_contract_verified=false;s=f.snapshot(1);CHECK(s.visible && s.material.inventory_known && !s.material.hand_known && !s.can_select);
  }
}
void knowledge_and_gates() {
  for(const auto& r:cat::recipes) {
    Fixture f(r);KnowledgeMemory memory(r);const auto read=ubc::read_alchemy_extra_knowledge(memory.memory(),ubc::lhiver::game_parser,r);
    CHECK(read.bindings_verified && read.unchanged && read.count==r.requirements.count);
    f.input.knowledge=read;CHECK(f.snapshot(2).effect_ready);
    bool has_learning=false;
    for(std::size_t i=0;i<r.requirements.count;++i)if(r.requirements[i].role==cat::RequirementRole::knowledge) {
      has_learning=true;f.input.knowledge.values[i].value=0;auto s=f.snapshot(2);CHECK(s.learned_known && !s.learned && !s.visible && s.material.known());
      f.input.knowledge=read;break;
    }
    CHECK(has_learning==(r.id!=cat::RecipeId::swamp_roll));
    for(std::size_t i=0;i<r.requirements.count;++i)if(r.requirements[i].symbol!=12330 && r.requirements[i].role==cat::RequirementRole::context) {
      f.input.knowledge.values[i].value^=1;const auto s=f.snapshot(2);
      CHECK(s.visible && s.learned && !s.owner_conditions_match && s.can_select && !s.effect_ready);break;
    }
    f.input.knowledge=read;f.input.knowledge.bindings_verified=false;CHECK(!f.snapshot(2).visible);
    f.input.knowledge=read;f.input.knowledge.count-=1;CHECK(!f.snapshot(2).visible);
    f.input.knowledge=read;f.input.knowledge.values[1]=f.input.knowledge.values[0];CHECK(!f.snapshot(2).visible);
    f.input.knowledge=read;f.input.knowledge.values[0].value=0;CHECK(!f.snapshot(2).visible);
    f.input.knowledge=read;f.input.local_recipe_contract_verified=false;CHECK(!f.snapshot(2).visible);
    f.input.local_recipe_contract_verified=true;f.input.native_debit_contract_verified=false;
    auto s=f.snapshot(2);CHECK(s.visible && s.material.known() && !s.can_select && s.issue==ubc::AlchemyExtraSnapshotIssue::native_contract);
    f.input.native_debit_contract_verified=true;f.input.flask.all_slot_aliases_excluded=false;CHECK(!f.snapshot(2).can_select && f.snapshot(1).can_select);
    const auto wanted=static_cast<std::uint32_t>(r.requirements[r.requirements.count-1].symbol);
    memory.denied=KnowledgeMemory::name(wanted);CHECK(!ubc::read_alchemy_extra_knowledge(memory.memory(),ubc::lhiver::game_parser,r).bindings_verified);
    memory.denied=0;memory.put(KnowledgeMemory::symbol(wanted)+0x20,0);CHECK(!ubc::read_alchemy_extra_knowledge(memory.memory(),ubc::lhiver::game_parser,r).bindings_verified);
  }
  const auto& r=cat::recipes[0];KnowledgeMemory changed(r);
  changed.watch=KnowledgeMemory::symbol(11325);changed.change_on=3;changed.change=KnowledgeMemory::symbol(11325)+0x18;
  CHECK(!ubc::read_alchemy_extra_knowledge(changed.memory(),ubc::lhiver::game_parser,r).unchanged);
  KnowledgeMemory array(cat::recipes[3]);array.put(KnowledgeMemory::symbol(1464)+0x18,0);
  CHECK(!ubc::read_alchemy_extra_knowledge(array.memory(),ubc::lhiver::game_parser,cat::recipes[3]).bindings_verified);
}
void invalid_inventory() {
  const auto& r=cat::recipes[0];
  auto reject=[&](auto mutate) {Fixture f(r);mutate(f);const auto s=f.snapshot(2);CHECK(!s.can_select);};
  reject([](Fixture& f){f.input.inventory.storage_totals_known=false;});
  reject([](Fixture& f){f.input.inventory.items={nullptr,0};});
  reject([](Fixture& f){f.items[0].symbol=f.items[1].symbol;});
  reject([](Fixture& f){f.items[0].total=-1;});
  reject([](Fixture& f){f.items[0].packed=1;});
  reject([](Fixture& f){f.items[0].has_active_marker=true;});
  reject([](Fixture& f){f.items[0].mixed_representation=true;});
  reject([](Fixture& f){f.items[0].duplicate_packed_target=true;});
  reject([](Fixture& f){f.items[0].listed_occurrences=2;f.refresh();});
  reject([](Fixture& f){f.input.inventory.listed_nodes=4097;});
  reject([](Fixture& f){f.input.inventory.packed_entries=8193;});
  reject([](Fixture& f){f.objects[0]=f.input.hands.right.object;});
  reject([](Fixture& f){f.input.interact_item=0;});
  reject([](Fixture& f){f.input.hands.right.amount=2;});
  reject([](Fixture& f){f.input.hands.left={1,16604,1,0x9800};});
  reject([](Fixture& f){f.input.boundary_verified=false;});
  reject([](Fixture& f){f.input.context.mob=0;});
  reject([](Fixture& f){f.input.serial=0;});
  reject([](Fixture& f){f.input.flask.object=0x7777;});
  reject([](Fixture& f){f.input.flask.flags|=0x40000000;});
}
} // namespace extra_test

#ifndef UBC_ALCHEMY_EXTRA_FIXTURE_ONLY
int main() {
  extra_test::material_cases();extra_test::knowledge_and_gates();extra_test::invalid_inventory();
  std::printf("%u checks, %u failures\n",extra_test::checks,extra_test::failures);return extra_test::failures?1:0;
}
#endif
