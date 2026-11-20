// SPDX-License-Identifier: MIT
#include "ubc/spirit_recipe_snapshot.hpp"
#define UBC_SPIRIT_RUNTIME_FIXTURE_ONLY
#include "spirit_recipe_runtime_tests.cpp"
#include <type_traits>
namespace allocation_probe {bool watching=false;std::size_t count=0;}
void* operator new(std::size_t size){if(allocation_probe::watching)++allocation_probe::count;if(auto* p=std::malloc(size?size:1))return p;std::abort();}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace {
using namespace spirit_test;
constexpr std::uint32_t hero=0x2000,pack=0x40000,slots=0x110000,left_slot=0x120000,right_slot=0x120080;
constexpr std::uint32_t left_name=0x130000,right_name=0x130050,right_object=0x150000;
constexpr std::uint32_t node(std::uint32_t i){return 0x10000+i*16;}
constexpr std::uint32_t object(std::uint32_t i){return 0x90000+i*0x400;}
struct Stored{std::int32_t symbol,amount;};
ubc::SpiritRecipeKey key(){return {1,7,0x8000,hero,0x8800,0x9000,ubc::recipe_catalog::Station::alchemy};}
const char* target_name(std::int32_t id){for(const auto& t:cat::targets)if(t.symbol==id)return t.name;std::abort();}
std::string entry(std::int32_t id,int n){return std::string(target_name(id))+":"+std::to_string(n);}
std::string materials(int n,int flask=9){std::string out;for(const auto& c:cat::costs){const auto amount=c.symbol==16604?flask:n*static_cast<int>(c.units);if(amount>0){if(!out.empty())out+=",";out+=entry(c.symbol,amount);}}return out;}
void text_at(Image& image,std::uint32_t address,const std::string& text){image.block(address,text.size()+1);std::memcpy(image.at(address,text.size()+1),text.c_str(),text.size()+1);}
struct CaptureFixture:Fixture {
  CaptureFixture():Fixture(cat::recipes[0]) {
    image.block(hero,0xa00);image.block(slots,8);image.block(left_slot,0x38);image.block(right_slot,0x38);
    text_at(image,left_name,"ZS_LEFTHAND");text_at(image,right_name,"ZS_RIGHTHAND");
    image.put(hero+0x9b0,slots);image.put(hero+0x9b4,2);image.put(hero+0x9b8,2);image.put(slots,left_slot);image.put(slots+4,right_slot);
    image.put(left_slot+8,left_name);image.put(left_slot+12,11);image.put(left_slot+0x18,0);
    image.put(right_slot+8,right_name);image.put(right_slot+12,12);image.put(right_slot+0x18,1);image.put(right_slot+0x30,right_object);
    image.block(ubc::hand_detail::item_vtable,4);image.put(ubc::hand_detail::item_vtable,ubc::hand_detail::item_class_method);
    image.block(ubc::hand_detail::item_class_method,ubc::hand_detail::item_class_bytes.size());
    std::memcpy(image.at(ubc::hand_detail::item_class_method,ubc::hand_detail::item_class_bytes.size()),ubc::hand_detail::item_class_bytes.data(),ubc::hand_detail::item_class_bytes.size());
    image.block(right_object,0x338);image.put(right_object,ubc::hand_detail::item_vtable);image.put(right_object+0x32c,1);image.put(right_object+0x330,16604);
    image.put(hero+0x968,right_object);stock({},materials(10));
  }
  void stock(const std::vector<Stored>& listed,const std::string& packed) {
    image.put(hero+0x708,hero);image.put(hero+0x70c,1);image.put(hero+0x718,listed.empty()?0:node(0));
    image.put(hero+0x724,packed.empty()?0:pack);image.put(hero+0x728,static_cast<std::uint32_t>(packed.size()));text_at(image,pack,packed);
    for(std::uint32_t i=0;i<listed.size();++i) {
      image.block(node(i),12);image.block(object(i),0x338);image.put(node(i)+4,object(i));image.put(node(i)+8,i+1<listed.size()?node(i+1):0);
      image.put(object(i),ubc::hand_detail::item_vtable);image.put(object(i)+0x158,0x200000);
      image.put(object(i)+0x32c,static_cast<std::uint32_t>(listed[i].amount));image.put(object(i)+0x330,static_cast<std::uint32_t>(listed[i].symbol));
    }
  }
};
ubc::SpiritRecipeReader reader;
ubc::SpiritRecipeCapture capture(CaptureFixture& f,ubc::SpiritCaptureProofs proof={true,true}){return reader.capture(f.image.memory(),ubc::lhiver::game_parser,key(),proof);}
const ubc::RecipeMaterialRow& row(const ubc::SpiritRecipeState& state,std::int32_t symbol){for(const auto& r:state.rows)if(r.symbol==symbol)return r;std::abort();}
struct MaterialFixture {
  std::array<ubc::RecipeInventoryItem,7> items{};ubc::RecipeInventory inventory{};ubc::StationRecipeHands hands{};
  std::array<ubc::RecipeMaterialRow,7> rows{};
  MaterialFixture(){for(std::size_t i=0;i<items.size();++i){items[i].symbol=cat::targets[i].symbol;items[i].listed=items[i].packed=items[i].total=0;}
    inventory.items={items.data(),items.size()};inventory.header_known=inventory.listed_known=inventory.packed_known=inventory.storage_totals_known=inventory.stable_boundary_verified=inventory.snapshot_unchanged=true;
    hands.hands.left={0,-1,-1,0};hands.hands.right={1,16604,1,right_object};hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;hands.right_tmp_level=1;hands.interact_item=right_object;
  }
  void amount(std::int32_t id,int n){for(auto& i:items)if(i.symbol==id){i.packed=i.total=n;i.packed_occurrences=n?1u:0u;}inventory.packed_entries=0;for(const auto& i:items)inventory.packed_entries+=i.packed_occurrences;}
  ubc::SpiritMaterial eval(unsigned n,cat::Branch b=cat::Branch::ordinary,bool hand=true,bool objects=true){return ubc::spirit_recipe_material(cat::recipes[0],inventory,hands,b,n,rows.data(),rows.size(),hand,objects);}
};
void nested(Image& image) noexcept {const auto out=reader.capture(image.memory(),ubc::lhiver::game_parser,key(),{true,true});check(out.diagnostic.issue==ubc::SpiritCaptureIssue::reentry,"nested read rejects without changing outer workspace");}
}
int main() {
  static_assert(!std::is_copy_constructible<ubc::SpiritRecipeReader>::value,"reader owns one persistent workspace");
  CaptureFixture f;const auto first=capture(f);
  check(first.state.evaluated&&first.state.known&&first.state.branch_known&&first.state.output==15739,"learned branch has independent exact identity");
  check(first.state.material.known()&&first.state.material.maximum==10&&!first.state.contract,"synthetic capture has positive material but cannot grant original profile");
  check(row(first.state,16604).stored==9&&row(first.state,16604).reserved==1&&row(first.state,16604).available==10,"hand flask contributes one actual consumed reserve");
  check(first.diagnostic.target_count==7&&f.image.hand_reads==2,"both output targets use one stable full hand capture");
  check(!ubc::spirit_recipe_catalog::runtime_ready&&!ubc::spirit_debit_integrated,"isolated arithmetic does not authorize missing debit writer");
  for(const auto value:{INT32_MIN,-1,0,1,2}){CaptureFixture x;x.image.put(symbol(11967)+0x18,static_cast<std::uint32_t>(value));const auto out=capture(x);
    check(out.state.known&&out.state.branch_known&&out.state.output==(value==1?15741:15739),"output branch is exact one else original fallback");}
  {CaptureFixture x;x.image.denied=symbol(11967);x.image.denied_size=4;const auto out=capture(x);check(out.state.known&&!out.state.branch_known&&out.state.material.maximum==0&&row(out.state,16130).stored==20,"unknown branch preserves resources and learned state but grants no quantity");}
  {CaptureFixture x;x.image.denied=symbol(11198);x.image.denied_size=4;const auto out=capture(x);check(!out.state.known&&out.state.branch_known&&out.state.material.maximum==10,"unreadable learning is hidden without inventing missing resources");}
  {CaptureFixture x;x.stock({},"");const auto out=capture(x);check(out.state.known&&out.state.material.known()&&out.state.material.maximum==0&&row(out.state,16130).missing_for_one==2,"known empty inventory explains each required ingredient");}
  {CaptureFixture x;x.image.put(symbol(20361)+0x18,0);const auto out=capture(x);check(out.state.context_matches&&!out.state.owner_conditions_match&&out.state.known,"navigation may open Special from root");}
  {CaptureFixture x;x.image.put(symbol(1679)+0x1c,0x3000);const auto out=capture(x);check(out.state.actor_known&&!out.state.actor_matches&&out.state.known,"refund actor mismatch stays separate from learned state");}
  {CaptureFixture x;const auto out=capture(x,{true,false});check(out.state.known&&!out.state.material.hand_known&&out.state.material.maximum==0,"current lab hand contract is never inferred from name");}
  {CaptureFixture x;const auto reads=x.image.reads;auto bad=key();bad.kind=ubc::recipe_catalog::Station::stove;const auto out=reader.capture(x.image.memory(),ubc::lhiver::game_parser,bad,{true,true});check(out.diagnostic.issue==ubc::SpiritCaptureIssue::context&&x.image.reads==reads,"foreign station does not trigger a read");}
  {CaptureFixture x;const auto out=capture(x,{false,true});check(out.diagnostic.issue==ubc::SpiritCaptureIssue::boundary&&!out.state.evaluated,"no boundary no capture");}
  {CaptureFixture x;x.stock({{16130,2}},materials(10));const auto out=capture(x);check(out.state.material.issue==ubc::SpiritMaterialIssue::ambiguous_ingredient&&row(out.state,16130).stored==22,"mixed input remains measured but blocked");}
  {CaptureFixture x;x.stock({{15739,2}},materials(10)+","+entry(15739,3));const auto out=capture(x);check(out.state.material.known()&&row(out.state,15739).stored==5,"mixed output is safely summed without native counting");}
  for(const auto flags:{0u,0x40200000u}){CaptureFixture x;x.stock({{16130,20}},materials(0));x.image.put(object(0)+0x158,flags);const auto out=capture(x);check(!out.diagnostic.ingredient_objects_verified&&out.state.material.maximum==0,"active or nonmultislot input cannot grant arithmetic maximum");}
  {CaptureFixture x;x.stock({{999,1}},materials(10));x.image.put(node(0)+4,right_object);const auto out=capture(x);check(!out.state.material.hand_known,"all listed pointers exclude held flask double counting");}
  for(unsigned change=0;change<5;++change){CaptureFixture x;x.stock({{16130,20}},materials(0));
    switch(change){case 0:x.image.change_address=symbol(11967);x.image.put(symbol(11967)+0x18,1);x.image.change_target=symbol(11967)+0x18;x.image.change_on_read=3;break;
      case 1:x.image.change_address=symbol(11198);x.image.change_target=symbol(11198)+0x18;x.image.change_on_read=3;break;
      case 2:x.image.change_address=right_slot;x.image.change_target=right_slot+0x30;x.image.change_on_read=2;break;
      case 3:x.image.change_address=object(0)+0x158;x.image.change_target=object(0)+0x158;x.image.change_on_read=2;break;
      case 4:x.image.change_address=hero+0x708;x.image.change_target=hero+0x724;x.image.change_on_read=4;break;}
    const auto out=capture(x);check(!out.state.branch_known||!out.state.known||!out.state.material.known(),"changed branch learning hand or inventory cannot remain a complete proof");
  }
  {CaptureFixture x;x.image.on_read=nested;allocation_probe::watching=true;const auto out=capture(x);allocation_probe::watching=false;check(out.state.material.known()&&allocation_probe::count==0,"capture uses no heap and rejects reentry");}
  for(const auto branch:{cat::Branch::ordinary,cat::Branch::enriched}) {
    MaterialFixture x;for(const auto& cost:cat::costs)x.amount(cost.symbol,static_cast<int>(cost.units*9));
    const auto chosen=branch==cat::Branch::ordinary?15739:15741;const auto untouched=chosen==15739?15741:15739;
    x.amount(chosen,INT32_MAX-2);x.amount(untouched,INT32_MAX);
    auto m=x.eval(2,branch);check(m.known()&&m.craftable&&m.maximum==2&&m.additional_flasks==1,"only selected output room limits quantity and extra debit is N minus one");
    for(const auto& r:x.rows){if(r.symbol==chosen)check(r.remaining==INT32_MAX&&r.produced==2,"selected branch receives N");if(r.symbol==untouched)check(r.remaining==INT32_MAX&&r.produced==0,"other branch is unchanged even at max capacity");if(r.symbol==16604)check(r.remaining==8,"end expectation includes consumed hand plus exact extra debit");}
    m=x.eval(3,branch);check(!m.craftable&&m.maximum==2,"output shortage refuses only excessive quantity");
    m=x.eval(0,branch);check(m.known()&&!m.craftable&&m.additional_flasks==0,"zero preview never invents a zero native debit");
    m=x.eval(UINT32_MAX,branch);check(!m.craftable&&m.issue==ubc::SpiritMaterialIssue::quantity,"arithmetic refuses unrepresentable N");
    std::swap(x.items[0],x.items[6]);check(x.eval(2,branch).craftable,"pure material maps by symbols not positions");
    x.inventory.items.count=6;check(x.eval(1,branch).issue==ubc::SpiritMaterialIssue::target_mapping,"both output stocks are obligatory");
  }
  {MaterialFixture x;for(const auto& c:cat::costs)x.amount(c.symbol,INT32_MAX);auto m=x.eval(cat::maximum_quantity);check(m.craftable&&m.maximum==cat::maximum_quantity,"two stings impose exact positive int32 maximum");}
  {MaterialFixture x;for(const auto& c:cat::costs)x.amount(c.symbol,10);check(!x.eval(1,cat::Branch::unknown).craftable&&x.eval(1,cat::Branch::unknown).maximum==0,"unknown branch never becomes ordinary through material preview");}
  {MaterialFixture x;std::array<ubc::RecipeMaterialRow,7> before=x.rows;const auto m=ubc::spirit_recipe_material(cat::recipes[0],x.inventory,x.hands,cat::Branch::ordinary,1,x.rows.data(),6,true,true);check(m.issue==ubc::SpiritMaterialIssue::output_buffer&&x.rows[0].symbol==before[0].symbol,"short buffer is rejected before writing");}
  std::printf("spirit_recipe_snapshot_tests: %u checks, %u failures\n",station_test::checks,station_test::failures);return station_test::failures?1:0;
}
