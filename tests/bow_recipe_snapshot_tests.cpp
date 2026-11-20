// SPDX-License-Identifier: MIT
#include "ubc/stations/bow/bow_recipe_snapshot.hpp"
#define UBC_BOW_RUNTIME_FIXTURE_ONLY
#include "bow_recipe_runtime_tests.cpp"
#include <type_traits>
namespace allocation_probe {bool watching=false;std::size_t count=0;}
void* operator new(std::size_t size){if(allocation_probe::watching)++allocation_probe::count;if(auto* p=std::malloc(size?size:1))return p;std::abort();}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace {using namespace bow_test;
constexpr std::uint32_t hero=0x2000,pack=0x40000,slots=0x110000,left_slot=0x120000,right_slot=0x120080;
constexpr std::uint32_t left_name=0x130000,right_name=0x130050,extra_slot=0x120100,extra_name=0x130100;
constexpr std::uint32_t node(std::uint32_t i){return 0x10000+i*16;}
constexpr std::uint32_t object(std::uint32_t i){return 0x90000+i*0x400;}
struct Stored{std::int32_t symbol,amount;};
ubc::BowRecipeKey key(){return {1,7,0x8000,hero,0x8800,0x9000,16};}
const char* target_name(std::int32_t id){for(const auto& r:cat::recipes)for(const auto& t:r.targets)if(t.symbol==id)return t.name;std::abort();}
const cat::ItemShape* shape(std::int32_t id){for(const auto& r:cat::recipes)for(const auto& s:r.items)if(s.symbol==id)return &s;return nullptr;}
std::string entry(std::int32_t id,int n){return std::string(target_name(id))+":"+std::to_string(n);}
std::string materials(int weapons=10,int steel=130,int leather=130){std::string out;
  for(const auto& r:cat::recipes){const auto old=r.outputs[0].symbol-1;if(weapons>0){if(!out.empty())out+=",";out+=entry(old,weapons);}}
  for(const auto value:{Stored{16610,steel},Stored{16599,leather}})if(value.amount>0){if(!out.empty())out+=",";out+=entry(value.symbol,value.amount);}return out;}
void text_at(Image& image,std::uint32_t address,const std::string& text){image.block(address,text.size()+1);std::memcpy(image.at(address,text.size()+1),text.c_str(),text.size()+1);}
struct CaptureFixture:Fixture {
  CaptureFixture():Fixture(cat::recipes[0]){for(const auto& r:cat::recipes)populate(r);
    image.block(hero,0xa00);image.block(slots,12);image.block(left_slot,0x38);image.block(right_slot,0x38);
    text_at(image,left_name,"ZS_LEFTHAND");text_at(image,right_name,"ZS_RIGHTHAND");image.put(hero+0x9b0,slots);image.put(hero+0x9b4,3);image.put(hero+0x9b8,2);
    image.put(slots,left_slot);image.put(slots+4,right_slot);image.put(left_slot+8,left_name);image.put(left_slot+12,11);image.put(right_slot+8,right_name);image.put(right_slot+12,12);
    image.block(ubc::hand_detail::item_vtable,4);image.put(ubc::hand_detail::item_vtable,ubc::hand_detail::item_class_method);
    image.block(ubc::hand_detail::item_class_method,ubc::hand_detail::item_class_bytes.size());std::memcpy(image.at(ubc::hand_detail::item_class_method,ubc::hand_detail::item_class_bytes.size()),ubc::hand_detail::item_class_bytes.data(),ubc::hand_detail::item_class_bytes.size());
    stock({},materials());
  }
  void stock(const std::vector<Stored>& listed,const std::string& packed){image.put(hero+0x708,hero);image.put(hero+0x70c,1);image.put(hero+0x718,listed.empty()?0:node(0));
    image.put(hero+0x724,packed.empty()?0:pack);image.put(hero+0x728,static_cast<std::uint32_t>(packed.size()));text_at(image,pack,packed);
    for(std::uint32_t i=0;i<listed.size();++i){image.block(node(i),12);image.block(object(i),0x338);image.put(node(i)+4,object(i));image.put(node(i)+8,i+1<listed.size()?node(i+1):0);
      image.put(object(i),ubc::hand_detail::item_vtable);const auto* s=shape(listed[i].symbol);image.put(object(i)+0x154,s?s->mainflag:1);image.put(object(i)+0x158,s?s->native_flags:0x200001);
      image.put(object(i)+0x32c,static_cast<std::uint32_t>(listed[i].amount));image.put(object(i)+0x330,static_cast<std::uint32_t>(listed[i].symbol));}
  }
};
ubc::BowRecipeCatalogueReader reader;
ubc::BowRecipeCatalogueCapture capture(CaptureFixture& f,ubc::BowCaptureProofs proof={true,true}){return reader.capture(f.image.memory(),ubc::lhiver::game_parser,key(),proof);}
const ubc::BowMaterialRow& row(const ubc::BowRecipeState& s,int symbol){for(const auto& r:s.rows)if(r.symbol==symbol)return r;std::abort();}
struct MaterialFixture {
  const cat::Descriptor& recipe;std::array<ubc::RecipeInventoryItem,4> items{};ubc::RecipeInventory inventory{};
  ubc::StationRecipeHands hands{};std::array<ubc::BowMaterialRow,4> rows{};
  explicit MaterialFixture(const cat::Descriptor& r):recipe(r){for(std::size_t i=0;i<4;++i){items[i].symbol=r.targets[i].symbol;items[i].listed=items[i].packed=items[i].total=0;}
    inventory.items={items.data(),items.size()};inventory.header_known=inventory.listed_known=inventory.packed_known=inventory.storage_totals_known=inventory.stable_boundary_verified=inventory.snapshot_unchanged=true;
    hands.hands.left={0,-1,-1,0};hands.hands.right={0,-1,-1,0};hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;
  }
  void amount(int symbol,int n){for(auto& i:items)if(i.symbol==symbol){i.packed=i.total=n;i.packed_occurrences=n?1u:0u;}inventory.packed_entries=0;for(const auto& i:items)inventory.packed_entries+=i.packed_occurrences;}
  ubc::BowMaterial eval(unsigned n,bool hands_verified=true,bool objects=true){return ubc::bow_recipe_material(recipe,inventory,hands,n,rows.data(),rows.size(),hands_verified,objects);}
};
void nested(Image& image) noexcept{const auto out=reader.capture(image.memory(),ubc::lhiver::game_parser,key(),{true,true});check(out.diagnostic.issue==ubc::BowCaptureIssue::reentry,"reentry preserves outer inventory and profile workspace");}
}
int main(){
  static_assert(!std::is_copy_constructible<ubc::BowRecipeCatalogueReader>::value,"own persistent workspace");
  CaptureFixture baseline;const auto first=capture(baseline);check(first.diagnostic.target_count==42,"twenty old and new weapons plus two materials deduplicate once");
  check(baseline.image.hand_reads==2&&first.diagnostic.inventory_unchanged&&first.diagnostic.hands_unchanged,"one common inventory and hand capture with comparison pass");
  for(const auto& s:first.recipes){check(s.evaluated&&s.known&&s.context_matches&&s.actor_matches&&!s.contract,"real knowledge distinct from synthetic noncanonical code");
    check(s.material.known()&&s.material.maximum==10,"exact cost and minimum permit ten upgrades");for(const auto& r:s.rows)check(r.held==0&&r.reserved==0,"no invented station weapon or tool reserve");}
  {CaptureFixture f;f.stock({},"");const auto v=capture(f);for(const auto& s:v.recipes)check(s.known&&s.material.known()&&!s.material.maximum,"all learned recipes remain measurable with known missing resources");}
  {CaptureFixture f;f.image.put(symbol(10484)+0x18,0);f.image.put(symbol(20862)+0x18,1);const auto v=capture(f);for(const auto& s:v.recipes)check(!s.known&&s.context_matches,"open category is not learned talent");}
  {CaptureFixture f;f.image.denied=symbol(10484);f.image.denied_size=4;const auto v=capture(f);for(const auto& s:v.recipes)check(!s.known&&s.material.known(),"unreadable shared learning does not invent missing materials");}
  {CaptureFixture f;const auto v=capture(f,{true,false});for(const auto& s:v.recipes)check(s.known&&!s.material.hand_known&&!s.material.maximum,"static event-free animation is not current native end proof");}
  {CaptureFixture f;f.image.put(symbol(20862)+0x18,1);const auto v=capture(f);check(v.recipes[0].owner_conditions_match,"category one matches local owner");}
  {CaptureFixture f;f.image.put(symbol(1679)+0x1c,0x3000);const auto v=capture(f);check(v.recipes[0].actor_known&&!v.recipes[0].actor_matches,"material HERO and B_SAY SELF cannot be mixed");}
  {CaptureFixture f;f.stock({{16810,2}},materials());const auto v=capture(f);check(v.recipes[0].material.issue==ubc::BowMaterialIssue::ambiguous_ingredient&&!v.recipes[0].material.maximum&&v.recipes[1].material.maximum==10,"mixed old weapon blocks only its target");}
  {CaptureFixture f;f.stock({{16811,2}},materials()+","+entry(16811,3));const auto v=capture(f);check(v.recipes[0].material.known()&&row(v.recipes[0],16811).stored==5,"new output mixed representation can still be counted");}
  for(unsigned change=0;change<5;++change){CaptureFixture f;f.stock({{16810,10}},materials(0));
    switch(change){case 0:f.image.put(object(0)+0x158,0x40080004);break;case 1:f.image.put(object(0)+0x154,2);break;
      case 2:f.image.put(object(0)+0x158,0x100004);break;case 3:f.image.put(object(0),ubc::hand_detail::item_vtable+4);break;
      case 4:f.image.block(extra_slot,0x38);text_at(f.image,extra_name,"ZS_BOW");f.image.put(extra_slot+8,extra_name);f.image.put(extra_slot+12,6);f.image.put(extra_slot+0x30,object(0));f.image.put(slots+8,extra_slot);f.image.put(hero+0x9b8,3);break;}
    const auto v=capture(f);check(!v.recipes[0].ingredient_objects_verified&&!v.recipes[0].material.maximum,"active wrong-category wrong-type or any equipped-slot alias rejects old weapon");}
  {CaptureFixture f;f.stock({{16810,10}},materials(0));const auto v=capture(f);check(v.recipes[0].ingredient_objects_verified&&v.recipes[0].material.maximum==10,"inactive exact native ranged flags are eligible without ITEM_MULTI bit");}
  {CaptureFixture f;f.stock({{16810,5},{16810,5}},materials(0));const auto v=capture(f);check(v.recipes[0].material.issue==ubc::BowMaterialIssue::ambiguous_ingredient,"multiple old weapon objects are not anonymously combined");}
  for(unsigned change=0;change<5;++change){CaptureFixture f;f.stock({{16810,10}},materials(0));switch(change){
    case 0:f.image.change_address=object(0)+0x158;f.image.change_target=object(0)+0x158;f.image.change_on_read=2;break;
    case 1:f.image.change_address=hero+0x708;f.image.change_target=hero+0x724;f.image.change_on_read=4;break;
    case 2:f.image.change_address=symbol(10484);f.image.change_target=symbol(10484)+0x18;f.image.change_on_read=3;break;
    case 3:f.image.change_address=symbol(12330);f.image.change_target=symbol(12330)+0x18;f.image.change_on_read=3;break;
    case 4:f.image.change_address=symbol(1679);f.image.change_target=symbol(1679)+0x1c;f.image.change_on_read=3;break;}
    const auto v=capture(f);check(!v.recipes[0].known||!v.recipes[0].material.known()||!v.recipes[0].context_matches||!v.recipes[0].actor_matches,"state transitions cannot retain complete capture evidence");}
  {CaptureFixture f;f.image.put(hero+0x968,0x1234);const auto v=capture(f);check(!v.recipes[0].material.hand_known,"empty slots with nonzero interact anchor do not prove event-free hand lifecycle");}
  {CaptureFixture f;const auto reads=f.image.reads;const auto v=capture(f,{false,true});check(v.diagnostic.issue==ubc::BowCaptureIssue::boundary&&!v.recipes[0].evaluated&&f.image.reads==reads,"missing stable boundary does not inspect game memory");}
  {CaptureFixture f;f.stock({{16810,10}},materials(0));f.image.at(ubc::hand_detail::item_class_method,1)[0]^=1;const auto v=capture(f);check(!v.recipes[0].ingredient_objects_verified&&!v.recipes[0].material.maximum,"native exact item identity includes method bytes");}
  {CaptureFixture f;f.image.put(right_slot+0x30,0x1234);const auto v=capture(f);check(!v.diagnostic.hands_unchanged&&!v.recipes[0].material.hand_known,"unreadable occupied hand is not empty");}
  {CaptureFixture f;f.stock({},materials(1));const auto v=capture(f);check(v.recipes[0].material.maximum==1&&first.recipes[0].material.maximum==10,"old capture is a value copy independent of reader reuse");}
  {CaptureFixture f;auto bad=key();bad.production_kind=17;const auto before=f.image.reads;const auto v=reader.capture(f.image.memory(),ubc::lhiver::game_parser,bad,{true,true});check(v.diagnostic.issue==ubc::BowCaptureIssue::context&&f.image.reads==before,"other station is not read");}
  {CaptureFixture f;f.image.on_read=nested;allocation_probe::watching=true;const auto v=capture(f);allocation_probe::watching=false;check(v.recipes[0].material.known()&&!allocation_probe::count,"bounded capture allocates nothing");}
  for(const auto& r:cat::recipes){MaterialFixture f(r);for(const auto& c:r.costs)f.amount(c.symbol,static_cast<int>(c.units*20));for(const auto& m:r.minima)if(m.units>cat::units(r.costs,m.symbol))f.amount(m.symbol,static_cast<int>(m.units*20));
    f.amount(r.outputs[0].symbol,INT32_MAX-2);auto m=f.eval(2);check(m.known()&&m.craftable&&m.maximum==2,"actual output room bounds otherwise available batch");
    for(const auto& row:f.rows)if(row.unit_cost)check(row.required==std::int64_t{row.unit_cost}*2&&row.original_minimum==std::int64_t{row.minimum_per_unit}*2,"minimum and consumption have distinct quantities");
    check(!f.eval(3).craftable&&f.eval(0).known()&&!f.eval(0).craftable,"zero preview versus excessive output quantity");check(!f.eval(UINT32_MAX).craftable,"no overflow amount");
    std::swap(f.items[0],f.items[3]);check(f.eval(2).craftable,"target identity independent of order");f.inventory.items.count=3;check(f.eval(1).issue==ubc::BowMaterialIssue::target_mapping,"missing target is not zero");
    MaterialFixture huge(r);for(const auto& c:r.costs)huge.amount(c.symbol,INT32_MAX);const auto maximum=r.callback.maximum_quantity;
    check(huge.eval(maximum).craftable&&huge.eval(maximum).maximum==maximum,"numeric bound includes every original minimum and consume multiplier");}
  for(unsigned change=0;change<8;++change){MaterialFixture f(cat::recipes[0]);for(const auto& c:f.recipe.costs)f.amount(c.symbol,10);
    switch(change){case 0:f.items[0].has_active_marker=true;break;case 1:f.items[0].duplicate_packed_target=true;break;
      case 2:f.items[0].packed_occurrences=2;++f.inventory.packed_entries;break;case 3:f.items[0].listed=1;break;
      case 4:f.items[0].total=-1;break;case 5:f.items[1].symbol=f.items[0].symbol;break;
      case 6:f.inventory.snapshot_unchanged=false;break;case 7:f.inventory.listed_nodes=1;break;}
    check(!f.eval(1).craftable&&!f.eval(1).maximum,"ambiguous contradictory unmapped or incompletely aliased input never grants quantity");}
  {MaterialFixture f(cat::recipes[0]);for(const auto& c:f.recipe.costs)f.amount(c.symbol,10);check(!f.eval(1,true,false).craftable&&f.eval(1,true,false).issue==ubc::BowMaterialIssue::ingredient_objects,"pure caller must supply current listed-object evidence");}
  {MaterialFixture f(cat::recipes.back());f.amount(16844,10);f.amount(16610,10);f.amount(16599,12);auto m=f.eval(1);check(m.known()&&!m.craftable&&!m.maximum,"last original raw minimum twelve retained despite consuming ten");
    for(const auto& r:f.rows)if(r.symbol==16610)check(r.missing==0&&r.minimum_missing==2,"original minimum shortfall differs from debit shortfall");
    f.amount(16610,12);f.amount(16599,10);m=f.eval(1);check(!m.craftable&&!m.maximum,"actual leather cost twelve also required despite minimum ten");
    f.amount(16610,24);f.amount(16599,24);m=f.eval(2);check(m.craftable&&m.maximum==2,"both exact scaled constraints satisfied together");for(const auto& r:f.rows)if(r.symbol==16610)check(r.remaining==4,"surplus raw minimum is not consumed as invented cost");}
  std::printf("bow_recipe_snapshot_tests: %u checks, %u failures\n",station_test::checks,station_test::failures);return station_test::failures?1:0;
}
