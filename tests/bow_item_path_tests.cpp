// SPDX-License-Identifier: MIT
#include "ubc/stations/bow/bow_item_path.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <type_traits>
#define UBC_STATION_RECIPE_FIXTURE_ONLY
#include "station_recipe_runtime_tests.cpp"

namespace allocation_probe {bool active=false;unsigned count=0;}
void* operator new(std::size_t n){if(allocation_probe::active)++allocation_probe::count;if(auto* p=std::malloc(n?n:1))return p;std::abort();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}

namespace {
unsigned checks=0,failures=0;
void check(bool value,const char* message) {++checks;if(!value){++failures;std::fprintf(stderr,"FAIL %s\n",message);}}
namespace p=ubc::bow_item_path;namespace cat=ubc::bow_recipe_catalog;
using Image=station_test::Image;
constexpr std::uint32_t hero=0x2000,pack=0x40000,slots=0x60000,left=0x61000,right=0x61100,extra=0x61200;
constexpr std::uint32_t node(unsigned i){return 0x10000+i*16;}
constexpr std::uint32_t object(unsigned i){return 0x100000+i*0x400;}
p::Reader reader(7);
p::Boundary boundary(){return {{1,7,0x8000,hero,0x8800,0x9000,16},8,7,true};}
struct Stored{std::int32_t symbol,amount;};
void text_at(Image& m,std::uint32_t address,const std::string& value){m.block(address,value.size()+1);std::memcpy(m.at(address,value.size()+1),value.c_str(),value.size()+1);}
const cat::ItemShape* shape(const cat::Descriptor& d,int id){for(const auto& item:d.items)if(item.symbol==id)return &item;return nullptr;}
const char* name(const cat::Descriptor& d,int id){for(const auto& t:d.targets)if(t.symbol==id)return t.name;std::abort();}
std::string entry(const cat::Descriptor& d,int id,int amount){return std::string(name(d,id))+":"+std::to_string(amount);}
struct Fixture {
  Image image;const cat::Descriptor& recipe;
  explicit Fixture(const cat::Descriptor& d=cat::recipes.front()):recipe(d){
    image.block(hero,0x1000);image.block(0x8a2558,4);image.block(slots,12);image.block(left,0x34);image.block(right,0x34);
    text_at(image,0x62000,"ZS_LEFTHAND");text_at(image,0x62100,"ZS_RIGHTHAND");
    image.put(hero+0x9b0,slots);image.put(hero+0x9b4,3);image.put(hero+0x9b8,2);
    image.put(slots,left);image.put(slots+4,right);image.put(left+8,0x62000);image.put(left+12,11);image.put(right+8,0x62100);image.put(right+12,12);
    image.put(hero+0x668,p::inventory_vtable);image.put(hero+0x66c,hero+0x710);image.put(hero+0x710,p::sorter);
    stock({},"");
  }
  void stock(const std::vector<Stored>& listed,const std::string& packed){
    image.put(hero+0x708,hero);image.put(hero+0x70c,1);image.put(hero+0x718,listed.empty()?0:node(0));
    image.put(hero+0x724,packed.empty()?0:pack);image.put(hero+0x728,static_cast<std::uint32_t>(packed.size()));text_at(image,pack,packed);
    for(std::uint32_t i=0;i<listed.size();++i){const auto value=listed[i];image.block(node(i),12);image.block(object(i),0x338);
      image.put(node(i)+4,object(i));image.put(node(i)+8,i+1<listed.size()?node(i+1):0);image.put(object(i),ubc::hand_detail::item_vtable);
      image.put(object(i)+4,5);const auto* s=shape(recipe,value.symbol);image.put(object(i)+0x154,s?s->mainflag:1);
      image.put(object(i)+0x158,s?s->native_flags:0x200001);image.put(object(i)+0x32c,static_cast<std::uint32_t>(value.amount));image.put(object(i)+0x330,static_cast<std::uint32_t>(value.symbol));
    }
  }
  void sufficient(unsigned quantity,unsigned packed_mask=0,bool whole=false){
    std::vector<Stored> listed{{100,3}};std::string packed;
    for(unsigned i=0;i<recipe.costs.count;++i){const auto cost=recipe.costs[i];
      const auto minimum=cat::units(recipe.minima,cost.symbol);const auto units=minimum>cost.units?minimum:cost.units;
      const auto amount=static_cast<int>(units*quantity+(whole?0:3));
      if(packed_mask&(1u<<i)){if(!packed.empty())packed+=",";packed+=entry(recipe,cost.symbol,amount);}else listed.push_back({cost.symbol,amount});
    }stock(listed,packed);
  }
  p::Snapshot capture(unsigned n=2,p::Boundary b=boundary()){allocation_probe::active=true;const auto out=reader.capture(image.memory(),recipe,n,b);allocation_probe::active=false;return out;}
};
void nested(Image& image) noexcept {const auto result=reader.capture(image.memory(),cat::recipes.front(),2,boundary());check(result.issue==p::Issue::reentry,"recursive observation never reuses workspace");}
void data_cases(){
  for(const auto& d:cat::recipes)for(unsigned n:{1u,2u,7u})for(unsigned mask=0;mask<8;++mask){
    Fixture f(d);f.sufficient(n,mask);const auto out=f.capture(n);
    check(out.captured&&out.issue==p::Issue::none,"all twenty recipes accept coherent listed and pure packed data");
    check(out.requirements.unpack_create==(mask!=0)&&out.requirements.materialized_object_evidence==(mask!=0),"packed needs actual materialization evidence");
    check(out.requirements.new_output_node&&!out.requirements.merge_output&&out.insertion==p::Insertion::new_stack,"no invented existing output object");
    for(unsigned i=0;i<3;++i)check(out.removals[i].operation.symbol==out.plan.removals[i].symbol&&out.removals[i].path==p::Removal::split,"ordered split semantics from actual original callback");
    auto next=boundary();++next.frame;const auto late=f.capture(n,next);check(p::same_capture(out,late),"identical later stable capture");
    ++next.key.generation;check(!p::same_capture(out,f.capture(n,next)),"new request generation is not same evidence");
  }
  for(unsigned mask=0;mask<8;++mask){Fixture f;f.sufficient(2,mask,true);const auto out=f.capture();check(out.captured,"whole-removal data capture");
    check(out.removals[0].nodes_before==4&&out.removals[2].nodes_after==1,"unpack insertion and three removals accounted in actual order");
    for(const auto& r:out.removals)check(r.path==p::Removal::whole&&r.existing_whole_short_path==(r.source==p::Source::listed),"future packed refcounts never synthesized");
  }
  for(unsigned variant=0;variant<7;++variant){Fixture f;f.sufficient(2,0,true);
    if(variant==0)f.image.put(hero+0x66c,0);
    if(variant==1)f.image.put(hero+0x700,1);
    if(variant==2)f.image.put(0x8a2558,1);
    if(variant==3)f.image.put(hero+0x68c,3);
    if(variant==4)f.image.put(object(1)+4,1);
    if(variant==5)f.image.put(object(1)+4,INT32_MAX);
    if(variant==6)f.image.put(hero+0x710,p::sorter+1);
    const auto out=f.capture();check(out.captured,"data stays separate from a missing short native path");
    if(variant!=6)check(!out.removals[0].existing_whole_short_path||!out.removals[2].existing_whole_short_path,"no inferred whole shortcut with unsuitable fields");
    else check(!out.original_sorter_bound,"foreign comparator does not gain original binding");
  }
  {Fixture f(cat::recipes.back());f.stock({{16844,2},{16610,20},{16599,24}},"");check(f.capture().issue==p::Issue::resources,"actual twenty lacks original minimum twenty-four steel");}
  {Fixture f;f.sufficient(2);f.image.put(object(0),ubc::hand_detail::item_vtable+4);check(f.capture().issue==p::Issue::item,"foreign unrelated list vptr rejects entire virtual scan");}
  {Fixture f;f.sufficient(2);f.image.put(object(1)+0x158,0x40200001);check(f.capture().issue==p::Issue::item,"active ingredient rejected");}
  {Fixture f;f.sufficient(2);f.image.put(object(3)+0x154,2);check(f.capture().issue==p::Issue::item,"category two cannot substitute for ranged four");}
  {Fixture f;f.sufficient(2);f.image.block(extra,0x34);text_at(f.image,0x62200,"ZS_BOW");f.image.put(extra+8,0x62200);f.image.put(extra+12,6);f.image.put(extra+0x30,object(3));f.image.put(slots+8,extra);f.image.put(hero+0x9b8,3);check(f.capture().issue==p::Issue::alias,"third slot alias rejects unequipped-looking weapon");}
  {Fixture f;f.sufficient(2);f.image.put(hero+0x968,object(3));check(f.capture().issue==p::Issue::hands,"anchor must be positively null");}
  {Fixture f;f.sufficient(2);f.image.denied=hero+0x968;f.image.denied_size=4;check(f.capture().issue==p::Issue::hands,"unknown anchor is not zero");}
  {Fixture f;f.stock({{16610,20},{16599,20},{16810,20},{16810,1}},"");check(f.capture().issue==p::Issue::ambiguous,"duplicate original weapon rejected");}
  {Fixture f;f.stock({{16610,20},{16599,20},{16810,20}},entry(f.recipe,16810,2));check(f.capture().issue==p::Issue::ambiguous,"no mixed-policy relaxation");}
  {Fixture f;f.sufficient(2);f.image.put(hero+0x724,pack);const std::string collision="XITRW_BOW_L_01:4";text_at(f.image,pack,collision);f.image.put(hero+0x728,static_cast<std::uint32_t>(collision.size()));check(f.capture().issue==p::Issue::pack_search,"foreign suffix search is not a known absent packed target");}
  {Fixture f;f.stock({{16610,20},{16599,20}},entry(f.recipe,16810,4)+",ITRW_BOW_L_01_V1:2");check(f.capture().captured,"canonical first native search wins over longer output name");}
  {Fixture f;f.stock({{16610,20},{16599,20}},"XITRW_BOW_L_01:2,"+entry(f.recipe,16810,4));check(f.capture().issue==p::Issue::pack_search,"ambiguous native NAME-colon occurrence rejected");}
  {Fixture f;f.stock({{16610,20},{16599,20},{16810,20},{16811,4}},"");const auto out=f.capture();check(out.captured&&out.insertion==p::Insertion::merge&&out.requirements.merge_output,"actual inactive output follows merge route");
    f.image.put(object(3)+0x158,0x40080004);const auto active=f.capture();check(active.captured&&active.insertion==p::Insertion::new_stack,"active existing output cannot absorb new stack");}
  {Fixture f;f.stock({{16610,20},{16599,20},{16810,20},{16811,4},{16811,5}},"");f.image.put(object(3)+0x158,0x40080004);
    const auto out=f.capture();check(out.captured&&out.output_merge_object==object(4),"merge destination is first actual inactive output not last representative");
    f.image.put(object(3)+0x158,0x80004);check(f.capture().output_merge_object==object(3),"ordered native first matching output remains bound");}
  {Fixture f;f.stock({{16610,20},{16599,20},{16810,20},{16811,INT32_MAX}},"");check(f.capture().issue==p::Issue::overflow,"output int32 overflow rejected");}
  for(unsigned field:{0u,4u,0x154u,0x158u,0x16cu}){Fixture f;f.sufficient(2);if(field==0x16c)f.image.put(object(0)+field,4);f.image.change_address=object(0)+field;f.image.change_target=object(0)+field;f.image.change_on_read=2;
    check(f.capture().issue==p::Issue::changed,"comparison reads bind nonrecipe object identity metadata and damage");}
  {Fixture f;f.sufficient(2);f.image.change_address=hero+0x710;f.image.change_target=hero+0x710;f.image.change_on_read=2;check(f.capture().issue==p::Issue::changed,"comparator cannot change across snapshot");}
  {Fixture f;f.sufficient(2);f.image.on_read=&nested;check(f.capture().captured,"outer observation survives rejected recursion");}
  {Fixture f;f.sufficient(2);const auto before=f.capture();auto after=before;++after.materials[0].total;
    check(!p::same_capture(before,after),"value copies remain bound even if old digest was copied");after=before;after.requirements.unpack_create=!after.requirements.unpack_create;
    check(!p::same_capture(before,after),"path requirements cannot be changed under old fingerprint");after=before;++after.removals[0].nodes_after;
    check(!p::same_capture(before,after),"planned native node transitions belong to evidence");after=before;after.hands.interact_item=4;
    check(!p::same_capture(before,after),"copied hand values remain bound");}
  {Fixture f;std::vector<Stored> list(4092,{100,1});list.push_back({16610,20});list.push_back({16599,20});list.push_back({16810,20});list.push_back({16811,1});
    f.stock(list,"");const auto out=f.capture();check(out.captured&&out.listed_nodes==4096&&out.insertion==p::Insertion::merge,"full observed inventory remains supported with output merge");
    f.image.put(object(4095)+0x330,101);check(f.capture().issue==p::Issue::overflow,"new output cannot exceed result reader observation capacity");}
  for(unsigned change=0;change<5;++change){Fixture f;f.sufficient(2);auto b=boundary();if(change==0)b.stable_boundary_verified=false;if(change==1)b.current_thread=8;if(change==2)b.key.thread=8;if(change==3)b.frame=0;if(change==4)b.key.production_kind=4;
    check(!f.capture(2,b).captured,"unknown foreign or wrong physical boundary rejects before observation");}
}
}
int main() {
  namespace p=ubc::bow_item_path;namespace cat=ubc::bow_recipe_catalog;
  const auto first=p::make_plan(cat::recipes.front(),2);
  check(first.valid&&first.removals[0].symbol==16610&&first.removals[2].symbol==16810,"first canonical removal order");
  const auto last=p::make_plan(cat::recipes.back(),2);
  check(last.valid&&last.removals[0].symbol==16844&&last.removals[1].amount==20&&last.minima[2].amount==24,"last minimum remains distinct from consumption");
  check(!p::make_plan(cat::recipes.front(),0).valid,"zero is no execution plan");
  check(!p::complete_execution_contract,"reader does not invent unproved native closure");
  static_assert(!std::is_copy_constructible<p::Reader>::value,"reader owns workspace");
  data_cases();
  check(allocation_probe::count==0,"all inventory and hand captures use persistent workspace without C++ heap allocations");
  std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
