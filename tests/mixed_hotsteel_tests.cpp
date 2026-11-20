// SPDX-License-Identifier: MIT
#include "ubc/stations/mixed_hotsteel.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mix=ubc::mixed_hotsteel;
namespace {
unsigned checks=0,failures=0;
void check(bool value,int line){++checks;if(!value){++failures;std::fprintf(stderr,"failure line %d\n",line);}}
#define CHECK(value) check((value),__LINE__)
constexpr std::uint32_t thread=7,hero=0x2000,pack=0x60000,slots=0x50000;
constexpr std::uint32_t node(std::uint32_t i){return 0x10000+i*16;}
constexpr std::uint32_t object(std::uint32_t i){return 0x100000+i*0x400;}
constexpr std::uint32_t slot(std::uint32_t i){return 0x51000+i*0x80;}
constexpr std::uint32_t slot_name(std::uint32_t i){return 0x52000+i*0x80;}
constexpr std::uint32_t blade=static_cast<std::uint32_t>(ubc::station_recipe_catalog::RecipeId::form_hot_blade);
using Bytes=std::vector<std::uint8_t>;
std::uint32_t u32(const std::uint8_t* p){return ubc::hand_detail::u32(p);}
ubc::lhiver::Digest digest(const void* p,std::size_t n){ubc::Sha256 h;h.update(p,n);return h.finish();}
struct Image {
  std::map<std::uint32_t,Bytes> blocks;
  std::uint32_t denied=0,watch=0,change_target=0,change_value=0;
  unsigned reads=0,watch_reads=0,change_on=0;
  void (*callback)(Image&) noexcept=nullptr;
  void* user=nullptr;
  void block(std::uint32_t a,std::size_t n){blocks[a].resize(n);}
  std::uint8_t* at(std::uint32_t a,std::size_t n){
    auto it=blocks.upper_bound(a);if(it==blocks.begin())return nullptr;--it;
    const auto offset=static_cast<std::size_t>(a-it->first);
    return offset<=it->second.size() && n<=it->second.size()-offset?it->second.data()+offset:nullptr;
  }
  void put(std::uint32_t a,std::uint32_t value){auto* p=at(a,4);if(!p)std::abort();for(unsigned i=0;i<4;++i)p[i]=static_cast<std::uint8_t>(value>>(i*8));}
  void text(std::uint32_t a,const std::string& value){block(a,value.size()+1);std::memcpy(at(a,value.size()+1),value.c_str(),value.size()+1);}
  static bool read(void* p,std::uint32_t a,void* out,std::size_t n) noexcept {
    auto& self=*static_cast<Image*>(p);++self.reads;
    if(self.callback){auto f=self.callback;self.callback=nullptr;f(self);}
    if(a==self.watch && ++self.watch_reads==self.change_on)self.put(self.change_target,self.change_value);
    if(self.denied && a<=self.denied && std::uint64_t{self.denied}<std::uint64_t{a}+n)return false;
    const auto* bytes=self.at(a,n);if(!bytes)return false;std::memcpy(out,bytes,n);return true;
  }
  ubc::ReadOnlyMemory memory(){return {this,&read};}
};
struct Fixture {
  Image image;
  mix::Boundary boundary{{1,thread,0x1000,0x1800,hero,0x4000,0x5000,ubc::lhiver::game_parser},100,1,thread,true};
  std::unique_ptr<mix::Reader> reader=std::make_unique<mix::Reader>(thread);
  Fixture(){
    image.block(hero,0xa00);image.put(hero+0x668,mix::inventory_vtable);image.put(hero+0x708,hero);image.put(hero+0x70c,1);
    image.block(slots,12);image.put(hero+0x9b0,slots);image.put(hero+0x9b4,3);image.put(hero+0x9b8,3);
    for(std::uint32_t i=0;i<3;++i){
      image.block(slot(i),0x38);image.put(slots+i*4,slot(i));
      const std::string name=i==0?"ZS_LEFTHAND":i==1?"ZS_RIGHTHAND":"ZS_OTHER";
      image.text(slot_name(i),name);image.put(slot(i)+8,slot_name(i));image.put(slot(i)+12,static_cast<std::uint32_t>(name.size()));
      image.put(slot(i)+16,static_cast<std::uint32_t>(name.size()+1));
    }
    storage({{16611,2},{123,4}},"OTHER:7,ITMISWORDRAWHOT:1,AFTER:3");
  }
  void storage(const std::vector<std::pair<std::int32_t,std::int32_t>>& list,const std::string& packed){
    image.put(hero+0x718,list.empty()?0:node(0));image.put(hero+0x724,pack);
    image.put(hero+0x728,static_cast<std::uint32_t>(packed.size()));image.text(pack,packed);
    for(std::uint32_t i=0;i<list.size();++i){
      image.block(node(i),12);image.put(node(i)+4,object(i));image.put(node(i)+8,i+1<list.size()?node(i+1):0);
      image.block(object(i),0x334);image.put(object(i),ubc::hand_detail::item_vtable);
      image.put(object(i)+0x154,1);image.put(object(i)+0x158,mix::item_multi);
      image.put(object(i)+0x32c,static_cast<std::uint32_t>(list[i].second));image.put(object(i)+0x330,static_cast<std::uint32_t>(list[i].first));
    }
  }
  mix::Capture capture(){return reader->capture(image.memory(),boundary);}
};

// Optional original file profile. Read only via ifstream (O_RDONLY) and verify
// the full EXE SHA before PE mapping. No dump or execution of original functions.
struct Original {
  Bytes file;std::uint32_t sections=0,count=0;
  explicit Original(const char* path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)std::abort();const auto size=f.tellg();
    if(size<=0 || static_cast<std::uint64_t>(size)>16*1024*1024)std::abort();
    file.resize(static_cast<std::size_t>(size));f.seekg(0);f.read(reinterpret_cast<char*>(file.data()),size);
    if(!f || digest(file.data(),file.size())!=ubc::station_abi::executable_sha256 || file.size()<0x100)std::abort();
    const auto pe=u32(file.data()+0x3c);if(std::uint64_t{pe}+24>file.size() || u32(file.data()+pe)!=0x4550)std::abort();
    count=file[pe+6]|(std::uint32_t{file[pe+7]}<<8);sections=pe+24+file[pe+20]+(std::uint32_t{file[pe+21]}<<8);
    if(!count || count>32 || std::uint64_t{sections}+count*40>file.size())std::abort();
  }
  const std::uint8_t* native(std::uint32_t address,std::size_t n) const {
    if(address<0x400000)std::abort();const auto rva=address-0x400000;
    for(std::uint32_t i=0;i<count;++i){
      const auto* s=file.data()+sections+i*40;const auto start=u32(s+12),length=u32(s+16),raw=u32(s+20);
      if(rva>=start && rva-start<=length && n<=length-(rva-start)){
        const auto off=std::uint64_t{raw}+rva-start;if(off+n>file.size())std::abort();return file.data()+off;
      }
    }
    std::abort();
  }
  void copy(Image& image) const {
    for(const auto& r:mix::original_ranges){image.block(r.address,r.length);std::memcpy(image.at(r.address,r.length),native(r.address,r.length),r.length);}
    for(const auto& s:mix::original_vslots){image.block(s.address,4);std::memcpy(image.at(s.address,4),native(s.address,4),4);}
  }
};
mix::WriterProof simulated_proof(const mix::Capture& late,std::uint32_t recipe=blade,std::uint32_t callback=21003){
  // Synthetic integrator contract only. Neither a game process
  // snapshot nor original file hashes establish these flags.
  mix::WriterProof p;p.boundary=late.boundary;p.job=9;p.writer_token=17;
  p.recipe=recipe;p.owner=mix::contract_for(recipe,callback).owner;p.callback=callback;p.quantity=2;
  p.hero_binding=p.self_binding=hero;p.own_context_before_after=p.canonical_callback_and_writer=true;
  p.full_recipe_and_item_constructors=p.amount_operands_equal_quantity=p.actors_verified=true;
  p.all_other_materials_and_output_room=p.station_hand_and_normal_end=true;
  p.external_bindings_and_parameters=p.getamount_pack_createvob_factory_allocator=true;
  p.insert_merge_and_release_dependencies=p.remove_dependencies=p.output_create_dependencies=true;return p;
}
void contracts(){
  CHECK(!mix::contract_for(0,21003).definition);CHECK(!mix::contract_for(blade,21004).definition);
  CHECK(!mix::contract_for(blade-1,21003).definition);
  const auto blade_contract=mix::contract_for(blade,21003);
  CHECK(blade_contract.definition && blade_contract.owner==20999 && blade_contract.output==16612 && !blade_contract.require_self_is_hero);
  for(const auto& r:ubc::anvil_recipe_catalog::recipes){
    const auto id=static_cast<std::uint32_t>(r.id),callback=r.callback.function->symbol;
    const auto c=mix::contract_for(id,callback);CHECK(c.definition==&r.callback);CHECK(c.require_self_is_hero);
    CHECK(c.owner==static_cast<std::uint32_t>(r.owner.instance) && c.output==r.outputs[0].symbol);
    CHECK(!mix::contract_for(id,r.original_three.function->symbol).definition);
    CHECK(!mix::contract_for(id,21003).definition);CHECK(!mix::contract_for(blade,callback).definition);
  }
  for(std::uint32_t id=0x6001;id<0x6009;++id)CHECK(!mix::contract_for(id,21003).definition);
}
void physical(){
  Fixture f;const auto good=f.capture();CHECK(good.valid);CHECK(good.issue==mix::Issue::none);
  CHECK(good.listed==2 && good.packed==1 && good.total==3 && good.object==object(0));
  CHECK(good.listed_nodes==2 && good.packed_entries==3);CHECK(f.capture().fingerprint==good.fingerprint);
  CHECK(mix::assess(blade,21003,3,good).input_eligible);
  for(const auto packed:{"ITMISWORDRAWHOT:1","A:1,ITMISWORDRAWHOT:1","ITMISWORDRAWHOT:1,Z:1"}){
    Fixture x;x.storage({{16611,2}},packed);CHECK(x.capture().valid);
  }
  for(const auto packed:{"ITMISWORDRAWHOT:01","ITMISWORDRAWHOT:0","ITMISWORDRAWHOT:1E",
      "ITMISWORDRAWHOT:1,ITMISWORDRAWHOT:1","ITMISWORDRAWHOT:1,PRE_ITMISWORDRAWHOT:2",
      "PRE_ITMISWORDRAWHOT:2,ITMISWORDRAWHOT:1","ITMISWORDRAWHOT:1,ITMISWORDRAWHOT_EXTRA:2",
      "ITMISWORDRAWHOT:1,OTHER:2E","ITMISWORDRAWHOT:1,","itmiswordrawhot:1",
      "ITMISWORDRAWHOT:2147483648","ITMISWORDRAWHOT:+1","ITMISWORDRAWHOT:-1","OTHER:1",""}){
    Fixture x;x.storage({{16611,2}},packed);CHECK(!x.capture().valid);
  }
  const std::vector<std::vector<std::pair<std::int32_t,std::int32_t>>> bad_lists{
    {},{{16611,0}},{{16611,-1}},{{16611,1},{16611,1}},{{16611,2},{16611,0}},{{123,2}},{{16611,2},{-1,1}}};
  for(const auto& list:bad_lists){Fixture x;x.storage(list,"ITMISWORDRAWHOT:1");CHECK(!x.capture().valid);}
  for(auto active:{mix::item_active,mix::item_multi|mix::item_active,0u}){
    Fixture x;x.image.put(object(0)+0x158,active);CHECK(!x.capture().valid);
  }
  for(std::uint32_t flag:{0u,2u,UINT32_MAX}){Fixture x;x.image.put(object(0)+0x154,flag);CHECK(!x.capture().valid);}
  for(std::uint32_t i=0;i<2;++i){
    Fixture x;x.image.put(object(i),0xabcdef);CHECK(!x.capture().valid);
    Fixture y;y.image.put(slot(2)+0x30,object(i));CHECK(!y.capture().valid);
    Fixture z;z.image.put(hero+0x968,object(i));CHECK(!z.capture().valid);
  }
  for(std::uint32_t i=0;i<3;++i){Fixture x;x.image.put(slot(i)+0x30,object(0));CHECK(!x.capture().valid);}
  {Fixture x;x.image.put(node(1)+4,object(0));CHECK(!x.capture().valid);}
  {Fixture x;x.image.put(node(1)+8,node(0));CHECK(!x.capture().valid);}
  for(auto at:{hero+0x668,hero+0x708,hero+0x70c}){Fixture x;x.image.put(at,0);CHECK(!x.capture().valid);}
  for(auto at:{hero+0x708,object(0)+0x154,object(1),pack,slot(2),slot_name(2),hero+0x968}){
    Fixture x;x.image.denied=at;CHECK(!x.capture().valid);
  }
  {Fixture x;x.image.put(hero+0x9b4,257);CHECK(!x.capture().valid);}
  {Fixture x;x.image.put(slots+8,slot(0));CHECK(!x.capture().valid);}
  {Fixture x;x.image.put(slot(2)+8,slot_name(0));x.image.put(slot(2)+12,11);CHECK(!x.capture().valid);}
  {Fixture x;x.boundary.quiescent_verified=false;CHECK(!x.capture().valid);CHECK(x.image.reads==0);}
  {Fixture x;x.boundary.current_thread=thread+1;CHECK(x.capture().issue==mix::Issue::thread);CHECK(x.image.reads==0);}
  for(unsigned which=0;which<11;++which){
    Fixture x;switch(which){
      case 0:x.boundary.serial=0;break;case 1:x.boundary.frame=0;break;case 2:x.boundary.identity.generation=0;break;
      case 3:x.boundary.identity.game=0;break;case 4:x.boundary.identity.world=0;break;case 5:x.boundary.identity.hero=0;break;
      case 6:x.boundary.identity.station=0;break;case 7:x.boundary.identity.manager=0;break;case 8:x.boundary.identity.parser=1;break;
      case 9:x.boundary.identity.thread=0;break;default:x.boundary.current_thread=0;break;
    }CHECK(!x.capture().valid);CHECK(x.image.reads==0);
  }
  {Fixture x;x.storage({{16611,INT32_MAX}},"ITMISWORDRAWHOT:1");CHECK(!x.capture().valid);}
  {Fixture x;x.storage({{16611,INT32_MAX-1}},"ITMISWORDRAWHOT:1");const auto c=x.capture();CHECK(c.valid);CHECK(mix::assess(blade,21003,1,c).issue==mix::Issue::count_overflow);}
  // Full 4096-entry list. The last object alias still matters.
  {Fixture x;std::vector<std::pair<std::int32_t,std::int32_t>> list(ubc::max_packed_inventory_nodes,{123,1});list[0]={16611,2};
    x.storage(list,"ITMISWORDRAWHOT:1");CHECK(x.capture().valid);
    x.image.put(slot(2)+0x30,object(ubc::max_packed_inventory_nodes-1));CHECK(!x.capture().valid);
  }
}
void stability(){
  for(const auto address:{object(0)+0x154,object(1)+0x154}){
    Fixture x;x.image.watch=address;x.image.change_on=2;x.image.change_target=address+4;x.image.change_value=0;
    CHECK(x.capture().issue==mix::Issue::changed);
  }
  for(const auto address:{node(0),object(0)+0x32c,pack}){
    Fixture x;x.image.watch=address;x.image.change_on=address==pack?2:3;x.image.change_target=object(0)+0x32c;x.image.change_value=3;
    CHECK(!x.capture().valid);
  }
  {Fixture x;x.image.watch=slot(2);x.image.change_on=2;x.image.change_target=slot(2)+0x18;x.image.change_value=1;CHECK(!x.capture().valid);}
  {Fixture x;x.image.watch=hero+0x708;x.image.change_on=4;x.image.change_target=hero+0x70c;x.image.change_value=2;CHECK(!x.capture().valid);}
  struct Reenter {Fixture* fixture;bool attempted=false,blocked=false;};
  Fixture f;Reenter re{&f};f.image.user=&re;
  f.image.callback=[](Image& image) noexcept {auto& r=*static_cast<Reenter*>(image.user);r.attempted=true;r.blocked=r.fixture->capture().issue==mix::Issue::reentry;};
  CHECK(f.capture().valid);CHECK(re.attempted && re.blocked);CHECK(f.capture().valid);
}
void arithmetic(){
  Fixture f;auto c=f.capture();
  for(std::int32_t l=1;l<=24;++l)for(std::int32_t p=1;p<=24;++p){
    c.listed=l;c.packed=p;c.total=l+p;
    for(std::uint32_t q=1;q<=static_cast<std::uint32_t>(l+p);++q){
      const auto a=mix::assess(blade,21003,q,c);CHECK(a.input_eligible);
      CHECK(a.maximum==l+p && a.original_first_count==2*l+p && a.expected_remaining==l+p-static_cast<std::int32_t>(q));
    }
    CHECK(!mix::assess(blade,21003,0,c).input_eligible);
    CHECK(!mix::assess(blade,21003,static_cast<std::uint32_t>(l+p+1),c).input_eligible);
  }
  c.listed=1;c.packed=INT32_MAX-2;c.total=INT32_MAX-1;
  CHECK(mix::assess(blade,21003,static_cast<std::uint32_t>(c.total),c).input_eligible);
  c.packed=INT32_MAX-1;c.total=INT32_MAX;CHECK(mix::assess(blade,21003,1,c).issue==mix::Issue::count_overflow);
  c.listed=INT32_MAX;c.packed=INT32_MAX;c.total=INT32_MAX;CHECK(mix::assess(blade,21003,1,c).issue==mix::Issue::physical_overflow);
  c=f.capture();CHECK(!mix::assess(blade,21003,UINT32_MAX,c).input_eligible);
  for(const auto& r:ubc::anvil_recipe_catalog::recipes){
    const auto a=mix::assess(static_cast<std::uint32_t>(r.id),r.callback.function->symbol,3,c);
    CHECK(a.input_eligible && a.maximum==3 && a.original_first_count==5 && a.expected_remaining==0);
  }
}
void admission(const Original* original){
  Fixture f;if(original)original->copy(f.image);const auto early=f.capture();
  ++f.boundary.serial;++f.boundary.frame;const auto late=f.capture();auto proof=simulated_proof(late);
  const auto positive=mix::admit(f.image.memory(),early,late,proof);
  // Without actual original file bytes there is NO positive native guard.
  CHECK(positive.allowed==bool(original));
  if(!original)CHECK(positive.issue==mix::Issue::native_unreadable);
  const std::array<bool mix::WriterProof::*,12> required{{
    &mix::WriterProof::own_context_before_after,&mix::WriterProof::canonical_callback_and_writer,
    &mix::WriterProof::full_recipe_and_item_constructors,&mix::WriterProof::amount_operands_equal_quantity,
    &mix::WriterProof::actors_verified,&mix::WriterProof::all_other_materials_and_output_room,
    &mix::WriterProof::station_hand_and_normal_end,&mix::WriterProof::external_bindings_and_parameters,
    &mix::WriterProof::getamount_pack_createvob_factory_allocator,&mix::WriterProof::insert_merge_and_release_dependencies,
    &mix::WriterProof::remove_dependencies,&mix::WriterProof::output_create_dependencies}};
  for(const auto field:required){auto p=proof;p.*field=false;CHECK(!mix::admit(f.image.memory(),early,late,p).allowed);}
  for(unsigned which=0;which<13;++which){
    auto p=proof;switch(which){case 0:p.job=0;break;case 1:p.writer_token=0;break;case 2:++p.owner;break;
      case 3:++p.callback;break;case 4:++p.recipe;break;case 5:p.quantity=0;break;case 6:p.quantity=4;break;
      case 7:++p.hero_binding;break;case 8:++p.boundary.serial;break;case 9:++p.boundary.frame;break;
      case 10:++p.boundary.current_thread;break;case 11:++p.boundary.identity.generation;break;default:++p.boundary.identity.hero;break;}
    CHECK(!mix::admit(f.image.memory(),early,late,p).allowed);
  }
  for(unsigned which=0;which<15;++which){
    auto c=early;switch(which){case 0:c.valid=false;break;case 1:++c.object;break;case 2:++c.listed;break;case 3:++c.packed;break;
      case 4:++c.total;break;case 5:++c.mainflag;break;case 6:c.flags|=mix::item_active;break;case 7:c.fingerprint[0]^=1;break;
      case 8:++c.boundary.identity.world;break;case 9:++c.boundary.identity.station;break;case 10:++c.boundary.identity.manager;break;
      case 11:c.boundary.serial=late.boundary.serial;break;case 12:c.boundary.frame=late.boundary.frame+1;break;
      case 13:++c.boundary.identity.thread;break;default:++c.boundary.identity.parser;break;}
    CHECK(!mix::admit(f.image.memory(),c,late,proof).allowed);
  }
  for(const auto& r:ubc::anvil_recipe_catalog::recipes){
    auto p=simulated_proof(late,static_cast<std::uint32_t>(r.id),r.callback.function->symbol);
    CHECK(mix::admit(f.image.memory(),early,late,p).allowed==bool(original));
    ++p.self_binding;CHECK(mix::admit(f.image.memory(),early,late,p).issue==mix::Issue::actor);
  }
  // Reject equal totals with a different representation and any unrelated stock
  // change before the original call. Afterward the result boundary applies.
  f.storage({{16611,1},{123,4}},"ITMISWORDRAWHOT:2");++f.boundary.serial;
  auto changed=f.capture();CHECK(changed.valid);auto p=simulated_proof(changed);
  CHECK(mix::admit(f.image.memory(),early,changed,p).issue==mix::Issue::changed);
  f.storage({{16611,2},{123,5}},"OTHER:7,ITMISWORDRAWHOT:1,AFTER:3");++f.boundary.serial;
  changed=f.capture();p=simulated_proof(changed);CHECK(mix::admit(f.image.memory(),early,changed,p).issue==mix::Issue::changed);
}
void native_code(const Original& original){
  Image image;original.copy(image);const auto good=mix::verify_original_code(image.memory());
  CHECK(good.original_ranges_match && good.original_vslots_match);
  for(const auto& range:mix::original_ranges)for(auto offset:{0u,range.length/2,range.length-1}){
    auto* byte=image.at(range.address+offset,1);const auto before=*byte;*byte^=1;
    const auto bad=mix::verify_original_code(image.memory());CHECK(!bad.original_ranges_match && bad.failed_address==range.address);*byte=before;
  }
  for(const auto& slot_entry:mix::original_vslots){
    image.put(slot_entry.address,slot_entry.target+1);const auto bad=mix::verify_original_code(image.memory());
    CHECK(!bad.original_vslots_match && bad.issue==mix::Issue::vtable && bad.failed_address==slot_entry.address);
    image.put(slot_entry.address,slot_entry.target);
  }
  for(const auto& range:mix::original_ranges){image.denied=range.address;CHECK(!mix::verify_original_code(image.memory()).original_ranges_match);}image.denied=0;
  image.watch=mix::original_ranges[0].address;image.change_on=2;
  image.change_target=image.watch;image.change_value=u32(image.at(image.watch,4))^1;
  CHECK(mix::verify_original_code(image.memory()).issue==mix::Issue::native_changed);
}
} // namespace
int main(int argc,char** argv){
  if(argc>2)return 2;std::unique_ptr<Original> original;if(argc==2)original=std::make_unique<Original>(argv[1]);
  contracts();physical();stability();arithmetic();admission(original.get());if(original)native_code(*original);
  std::printf("MixedHotSteel: %u checks, %u failures; original EXE fixture %s (no runtime proof)\n",checks,failures,original?"checked":"not requested");
  return failures?1:0;
}
