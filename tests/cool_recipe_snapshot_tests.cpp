// SPDX-License-Identifier: MIT
#include "ubc/cool_recipe_snapshot.hpp"
#include "ubc/cool_listed_input.hpp"
#define UBC_COOL_RUNTIME_FIXTURE_ONLY
#include "cool_recipe_runtime_tests.cpp"
namespace allocation_probe {bool watching=false;std::size_t count=0;}
void* operator new(std::size_t size){if(allocation_probe::watching)++allocation_probe::count;if(auto* p=std::malloc(size?size:1))return p;std::abort();}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace {
using namespace cool_test;
constexpr std::uint32_t hero=0x2000,pack=0x40000,slots=0x110000,left_slot=0x120000,right_slot=0x120080,left_name=0x130000,right_name=0x130050,hand_object=0x150000;
constexpr std::uint32_t node(std::uint32_t i){return 0x10000+i*16;}constexpr std::uint32_t object(std::uint32_t i){return 0x90000+i*0x400;}
struct Stored{std::int32_t symbol,amount;};
void text_at(Image& m,std::uint32_t address,const std::string& text){m.block(address,text.size()+1);std::memcpy(m.at(address,text.size()+1),text.c_str(),text.size()+1);}
ubc::CoolRecipeKey key(){return {1,7,0x8000,hero,0x8800,0x9000,13};}
struct CaptureFixture:Fixture {
  CaptureFixture(){image.block(hero,0xa00);image.block(slots,12);image.block(left_slot,0x38);image.block(right_slot,0x38);
    text_at(image,left_name,"ZS_LEFTHAND");text_at(image,right_name,"ZS_RIGHTHAND");image.put(hero+0x9b0,slots);image.put(hero+0x9b4,3);image.put(hero+0x9b8,2);image.put(slots,left_slot);image.put(slots+4,right_slot);
    image.put(left_slot+8,left_name);image.put(left_slot+12,11);image.put(right_slot+8,right_name);image.put(right_slot+12,12);image.put(left_slot+0x18,1);image.put(left_slot+0x30,hand_object);image.put(hero+0x968,hand_object);
    item(hand_object,16612,1);image.block(ubc::hand_detail::item_vtable,4);image.put(ubc::hand_detail::item_vtable,ubc::hand_detail::item_class_method);image.block(ubc::hand_detail::item_class_method,ubc::hand_detail::item_class_bytes.size());std::memcpy(image.at(ubc::hand_detail::item_class_method,ubc::hand_detail::item_class_bytes.size()),ubc::hand_detail::item_class_bytes.data(),ubc::hand_detail::item_class_bytes.size());stock({},"ITMISWORDBLADEHOT:5,ITMISWORDBLADE:2");}
  void item(std::uint32_t p,int id,int amount){image.block(p,0x338);image.put(p,ubc::hand_detail::item_vtable);image.put(p+0x154,1);image.put(p+0x158,0x200001);image.put(p+0x32c,static_cast<std::uint32_t>(amount));image.put(p+0x330,static_cast<std::uint32_t>(id));}
  void stock(const std::vector<Stored>& listed,const std::string& packed){image.put(hero+0x708,hero);image.put(hero+0x70c,1);image.put(hero+0x718,listed.empty()?0:node(0));image.put(hero+0x724,packed.empty()?0:pack);image.put(hero+0x728,static_cast<std::uint32_t>(packed.size()));text_at(image,pack,packed);
    for(std::uint32_t i=0;i<listed.size();++i){image.block(node(i),12);image.put(node(i)+4,object(i));image.put(node(i)+8,i+1<listed.size()?node(i+1):0);item(object(i),listed[i].symbol,listed[i].amount);}}
};
ubc::CoolRecipeCatalogueReader reader;
ubc::CoolRecipeCatalogueCapture capture(CaptureFixture& f,ubc::CoolCaptureProofs p={true,true,1}){return reader.capture(f.image.memory(),ubc::lhiver::game_parser,key(),p);}
void nested(Image& m) noexcept {check(reader.capture(m.memory(),ubc::lhiver::game_parser,key(),{true,true,1}).diagnostic.issue==ubc::CoolCaptureIssue::reentry,"reentrant capture does not corrupt workspace");}
}
#ifndef UBC_COOL_SNAPSHOT_FIXTURE_ONLY
namespace {
ubc::PackedInventoryWorkspace listed_input_inventory;
ubc::station_recipe_snapshot_detail::HandWorkspace listed_input_hands;
constexpr std::uint32_t other_slot=0x120100,other_name=0x130080;
void extra_slot(CaptureFixture& f,const char* name,std::uint32_t object,int level=0){
  f.image.block(other_slot,0x38);text_at(f.image,other_name,name);
  f.image.put(hero+0x9b8,3);f.image.put(slots+8,other_slot);
  f.image.put(other_slot+8,other_name);f.image.put(other_slot+12,static_cast<std::uint32_t>(std::strlen(name)));
  f.image.put(other_slot+0x18,static_cast<std::uint32_t>(level));f.image.put(other_slot+0x30,object);
}
bool listed_input(CaptureFixture& f,const ubc::CoolRecipeCatalogueCapture& source){
  return ubc::verify_cool_listed_input(f.image.memory(),source,listed_input_inventory,listed_input_hands);
}
void listed_input_tests(){
  {CaptureFixture f;f.stock({{16612,6},{16613,6}},"");const auto source=capture(f);
    const auto reads=f.image.inventory_reads,hand_reads=f.image.hand_reads;
    allocation_probe::count=0;allocation_probe::watching=true;const bool valid=listed_input(f,source);allocation_probe::watching=false;
    check(valid&&!allocation_probe::count,"listed input is passive and heap-free with genuine capture values");
    check(f.image.inventory_reads==reads+2&&f.image.hand_reads==hand_reads+2,"fresh whole inventory and hands use original comparison readers");
    check(!source.recipe.contract,"input proof never changes failed canonical script-body contract");}
  for(const auto* slot_name:{"ZS_SWORD","ZS_BOW","ZS_ARMOR","UNKNOWN_SLOT"}){
    CaptureFixture f;f.stock({{16612,6},{1001,1},{16613,6}},"");extra_slot(f,slot_name,object(1));
    const auto source=capture(f);
    check(source.recipe.material.maximum==6&&listed_input(f,source),"foreign listed equipment in its own ordinary slot does not block Hot input");
    check(station_test::rs::detail::u32(listed_input_inventory.records[1].node.data()+4)==object(1)&&
      listed_input_hands.count==3,"positive case actually contains full foreign list/slot evidence");
    for(const auto level:{1,2,-1,INT32_MIN,INT32_MAX}){
      f.image.put(other_slot+0x18,static_cast<std::uint32_t>(level));
      check(!listed_input(f,source),"foreign temporary or unknown-level listed-slot alias remains blocked");
    }
    f.image.put(other_slot+0x18,0);
    f.image.put(other_slot+0x30,object(0));
    check(!listed_input(f,source),"Hot input sharing any named or unknown slot stays blocked");
  }
  {CaptureFixture f;f.stock({{1001,1},{16613,6},{16612,6}},"");extra_slot(f,"ZS_SWORD",object(0));
    check(listed_input(f,capture(f)),"input is found by copied symbol, never by position or first object");}
  {CaptureFixture f;f.stock({{16612,6},{1001,1},{16613,6}},"");extra_slot(f,"ZS_SWORD",object(1));
    f.image.put(hero+0x968,object(1));const auto source=capture(f);
    check(source.hands.interact_item==object(1)&&!listed_input(f,source),"foreign listed interactItem alias stays blocked even for matching fresh source hands");}
  // Take fresh snapshots after stock changes. Zero, multiple, packed and
  // mixed hot-blade forms do not qualify for an unambiguous direct list path.
  for(unsigned bad=0;bad<8;++bad){CaptureFixture f;
    switch(bad){
      case 0:f.stock({{16613,6},{1001,1}},"");break;
      case 1:f.stock({{16612,0},{16613,6}},"");break;
      case 2:f.stock({{16612,3},{16612,3}},"");break;
      case 3:f.stock({},"ITMISWORDBLADEHOT:6");break;
      case 4:f.stock({{16612,3}},"ITMISWORDBLADEHOT:3");break;
      case 5:f.stock({},"ITMISWORDBLADEHOT:6E");break;
      case 6:f.stock({},"ITMISWORDBLADEHOT:3,ITMISWORDBLADEHOT:3");break;
      default:f.stock({{16612,6},{16613,6}},"");f.image.put(node(1)+4,object(0));break;
    }
    check(!listed_input(f,capture(f)),"no target or ambiguous/packed/mixed/duplicate representation remains blocked");
  }
  for(unsigned bad=0;bad<9;++bad){CaptureFixture f;f.stock({{16612,6},{16613,6},{1001,1}},"");
    const auto source=capture(f);
    switch(bad){
      case 0:f.image.put(hero+0x968,object(0));break;
      case 1:f.image.put(left_slot+0x30,object(0));break;
      case 2:f.image.put(right_slot+0x30,object(0));break;
      case 3:extra_slot(f,"UNKNOWN_SLOT",object(0));break;
      case 4:f.image.put(object(0)+0x32c,5);break;
      case 5:f.image.put(object(1)+0x32c,7);break;
      case 6:f.image.put(object(0)+0x330,1002);break;
      case 7:f.image.put(node(0)+4,hand_object);break;
      default:f.image.put(hero+0x708,hero+4);break;
    }
    check(!listed_input(f,source),"current target/anchor/hands/owner and both material totals stay bound");
  }
  for(unsigned bad=0;bad<10;++bad){CaptureFixture f;f.stock({{16612,6},{1001,1},{16613,6}},"");extra_slot(f,"ZS_SWORD",object(1));
    const auto source=capture(f);
    const std::array<std::uint32_t,10> addresses{{node(0),object(0)+0x32c,node(1),object(1)+0x32c,
      hero+0x708,hero+0x9b0,other_slot,other_name,hero+0x968,hand_object+0x32c}};
    f.image.denied=addresses[bad];f.image.denied_size=1;
    check(!listed_input(f,source),"even unrelated list and slot data must remain completely readable");
  }
  for(unsigned bad=0;bad<8;++bad){CaptureFixture f;f.stock({{16612,6},{1001,1},{16613,6}},"");extra_slot(f,"ZS_SWORD",object(1));
    const auto source=capture(f);
    const std::array<std::uint32_t,8> address{{node(0),object(0)+0x32c,node(1),object(1)+0x32c,hero+0x708,hero+0x9b0,other_slot,hero+0x968}};
    const std::array<std::uint32_t,8> target{{node(0)+4,object(0)+0x32c,node(1)+4,object(1)+0x330,hero+0x70c,hero+0x9b8,other_slot+0x30,hero+0x968}};
    f.image.change_address=address[bad];f.image.change_target=target[bad];f.image.change_on_read=2;
    check(!listed_input(f,source),"second-pass mutation in target or foreign records remains rejected");
  }
}
}
int main(){
  CaptureFixture baseline;const auto first=capture(baseline);check(first.recipe.known&&first.recipe.context_matches&&first.recipe.actor_matches&&!first.recipe.contract,"knowledge context actor and body verification separated");
  check(first.recipe.material.known()&&first.recipe.material.maximum==5&&first.recipe.material.numerical.maximum==5,"five stored plus generated hand remains maximum five");
  check(first.recipe.material.rows[0].held==1&&first.recipe.material.rows[0].reserved==0&&first.recipe.material.rows[0].available==5,"hand is generated not reserved inventory");
  check(baseline.image.hand_reads==2&&first.diagnostic.inventory_unchanged&&first.diagnostic.hands_unchanged,"one bounded capture plus comparison");
  {CaptureFixture f;f.stock({},"");const auto c=capture(f);check(c.recipe.known&&c.recipe.material.known()&&!c.recipe.material.maximum&&c.recipe.material.rows[0].missing_for_one==1,"missing hot stock despite generated hand");}
  {CaptureFixture f;const auto c=capture(f,{true,false,-1});check(c.recipe.material.inventory_known&&!c.recipe.material.hand_known&&!c.recipe.material.maximum&&c.recipe.material.numerical.maximum==5,"unproven hand cannot grant maximum");}
  {CaptureFixture f;f.stock({},"ITMISWORDBLADEHOT:2");const auto c=capture(f);check(c.recipe.material.maximum==2&&c.recipe.material.direct_maximum==1,"two stock needs separate extra debit");}
  {CaptureFixture f;f.stock({},"ITMISWORDBLADEHOT:5,ITMISWORDBLADE:2147483645");check(capture(f).recipe.material.maximum==2,"output room preserves smaller quantity");}
  {CaptureFixture f;f.stock({{16612,2}},"ITMISWORDBLADEHOT:3");const auto c=capture(f);check(c.recipe.material.inventory_known&&!c.recipe.material.maximum&&c.recipe.material.issue==ubc::CoolMaterialIssue::ambiguous_ingredient,"mixed hot counted but blocked");}
  {CaptureFixture f;f.stock({{16613,2}},"ITMISWORDBLADEHOT:5,ITMISWORDBLADE:3");const auto c=capture(f);check(c.recipe.material.known()&&c.recipe.material.rows[1].stored==5,"mixed output countable");}
  {CaptureFixture f;f.stock({{16612,2},{16612,3}},"");check(capture(f).recipe.material.issue==ubc::CoolMaterialIssue::ambiguous_ingredient,"multiple hot objects remain blocked");}
  {CaptureFixture f;f.stock({},"ITMISWORDBLADEHOT:2,ITMISWORDBLADEHOT:3");check(capture(f).recipe.material.issue==ubc::CoolMaterialIssue::ambiguous_ingredient,"duplicate packed hot entries remain blocked");}
  {CaptureFixture f;f.stock({},"ITMISWORDBLADEHOT:5E");check(!capture(f).recipe.material.maximum,"active packed ingredient or invalid syntax cannot plan");}
  for(unsigned mutation=0;mutation<13;++mutation){CaptureFixture f;f.stock({{16612,5}},"");switch(mutation){
    case 0:f.image.put(left_slot+0x18,0);break;case 1:f.image.put(left_slot+0x18,2);break;case 2:f.image.put(hand_object+0x330,16613);break;case 3:f.image.put(hand_object+0x32c,2);break;
    case 4:f.image.put(hero+0x968,object(0));break;case 5:f.image.put(right_slot+0x30,object(0));break;case 6:f.image.put(node(0)+4,hand_object);break;case 7:f.image.put(object(0)+0x158,0x40200001);break;
    case 8:f.image.put(object(0),ubc::hand_detail::item_vtable+4);break;case 9:f.image.put(object(0)+0x154,2);break;case 10:f.image.put(slots+4,left_slot);break;
    case 11:f.image.at(ubc::hand_detail::item_class_method,1)[0]^=1;break;case 12:f.image.put(left_slot+8,0xfffffff0);break;}
    check(!capture(f).recipe.material.maximum,"wrong generated hand level identity alias type flags or code fails closed");}
  {CaptureFixture f;f.image.put(left_slot+0x18,2);check(capture(f,{true,true,2}).recipe.material.maximum==5,"tmp two needs exact current native proof");}
  for(unsigned change=0;change<6;++change){CaptureFixture f;f.stock({{16612,5}},"");switch(change){
    case 0:f.image.change_address=object(0)+0x158;f.image.change_target=object(0)+0x158;f.image.change_on_read=2;break;
    case 1:f.image.change_address=symbol(12330);f.image.change_target=symbol(12330)+0x18;f.image.change_on_read=3;break;
    case 2:f.image.change_address=symbol(1679);f.image.change_target=symbol(1679)+0x1c;f.image.change_on_read=3;break;
    case 3:f.image.change_address=hero+0x708;f.image.change_target=hero+0x70c;f.image.change_on_read=4;break;
    case 4:f.image.change_address=left_slot;f.image.change_target=left_slot+0x18;f.image.change_on_read=2;break;
    case 5:f.image.denied=object(0)+0x158;f.image.denied_size=4;break;}
    const auto c=capture(f);check(!c.recipe.material.maximum||!c.recipe.context_matches||!c.recipe.actor_matches,"late mutation cannot retain all gates");}
  {CaptureFixture f;f.image.put(symbol(10536)+0x18,0);const auto c=capture(f);check(c.recipe.known&&c.recipe.material.maximum==5&&!c.recipe.context_matches,"entry flag is not learning or stock");}
  {CaptureFixture f;const auto reads=f.image.reads;auto k=key();k.production_kind=1;check(reader.capture(f.image.memory(),ubc::lhiver::game_parser,k,{true,true,1}).diagnostic.issue==ubc::CoolCaptureIssue::context&&f.image.reads==reads,"other station not read");check(capture(f,{false,true,1}).diagnostic.issue==ubc::CoolCaptureIssue::boundary&&f.image.reads==reads,"absent boundary never reads");}
  {CaptureFixture f;f.image.on_read=nested;check(capture(f).recipe.material.maximum==5,"outer snapshot survives reentry");f.stock({},"");check(!capture(f).recipe.material.maximum&&first.recipe.material.maximum==5,"capture owns copies");}
  {CaptureFixture f;allocation_probe::watching=true;const auto c=capture(f);allocation_probe::watching=false;check(c.recipe.material.maximum==5&&!allocation_probe::count,"capture allocates no heap memory");}
  listed_input_tests();
  std::printf("cool_recipe_snapshot_tests: %u checks, %u failures\n",station_test::checks,station_test::failures);return station_test::failures?1:0;
}
#endif
