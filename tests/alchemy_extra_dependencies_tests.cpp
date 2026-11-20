// SPDX-License-Identifier: MIT
#include "ubc/stations/alchemy/alchemy_extra_dependencies.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace d=ubc::alchemy_extra_dependencies;
namespace rt=ubc::alchemy_flask_runtime;
namespace {
unsigned checks=0,failures=0;
d::ListReader* reentrant_reader=nullptr;
d::Issue reentrant_issue=d::Issue::none;
void check(bool yes,int line){++checks;if(!yes){++failures;std::fprintf(stderr,"failure line %d\n",line);}}
#define CHECK(x) check((x),__LINE__)
constexpr std::uint32_t game=0x2000000,world=0x2001000,hero=0x2008000,mob=0x200a000,factory=0x200b000,record=0x200c000;
constexpr std::uint32_t table=0x3000000,item_symbol=0x3100000,class_symbol=0x3100100,item_name=0x3101000,class_name=0x3101100;
constexpr std::uint32_t node(std::size_t i){return 0x4000000+static_cast<std::uint32_t>(i)*16u;}
constexpr std::uint32_t item(std::size_t i){return 0x5000000+static_cast<std::uint32_t>(i)*0x400u;}
struct Memory {
  std::map<std::uint32_t,std::vector<std::uint8_t>> blocks;
  std::uint32_t deny=0,watch=0,change_address=0,change_value=0;
  unsigned reads=0,watch_reads=0,change_on=0;
  void (*callback)(Memory&) noexcept=nullptr;
  void block(std::uint32_t a,std::size_t n){blocks[a].resize(n);}
  std::uint8_t* at(std::uint32_t a,std::size_t n){
    auto f=blocks.upper_bound(a);if(f==blocks.begin())return nullptr;--f;
    const auto off=static_cast<std::size_t>(a-f->first);
    return off<=f->second.size()&&n<=f->second.size()-off?f->second.data()+off:nullptr;
  }
  void put(std::uint32_t a,std::uint32_t value){auto* p=at(a,4);if(!p)std::abort();d::detail::put(p,value);}
  void bytes(std::uint32_t a,const void* p,std::size_t n){auto* dest=at(a,n);if(!dest)std::abort();std::memcpy(dest,p,n);}
  static bool read(void* p,std::uint32_t a,void* dest,std::size_t n) noexcept {
    auto& m=*static_cast<Memory*>(p);++m.reads;
    if(m.callback){const auto callback=m.callback;m.callback=nullptr;callback(m);}
    if(a==m.watch&&++m.watch_reads==m.change_on)m.put(m.change_address,m.change_value);
    if(m.deny&&a<=m.deny&&std::uint64_t{m.deny}<std::uint64_t{a}+n)return false;
    const auto* data=m.at(a,n);if(!data)return false;std::memcpy(dest,data,n);return true;
  }
  ubc::ReadOnlyMemory view(){return {this,&read};}
};
std::uint32_t number(const std::uint8_t* p){return ubc::runtime_script::detail::u32(p);}
struct PE {
  std::vector<std::uint8_t> bytes;
  std::uint32_t pe=0,opt=0,sections=0,count=0,preferred=0;
  explicit PE(const char* path,const ubc::lhiver::Digest& wanted){
    std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)std::abort();const auto n=f.tellg();
    if(n<=0||n>32*1024*1024)std::abort();bytes.resize(static_cast<std::size_t>(n));f.seekg(0);
    f.read(reinterpret_cast<char*>(bytes.data()),n);if(!f||d::detail::digest(bytes.data(),bytes.size())!=wanted)std::abort();
    if(bytes.size()<0x100)std::abort();pe=number(bytes.data()+60);if(pe>bytes.size()-248||number(bytes.data()+pe)!=0x4550)std::abort();
    count=bytes[pe+6]|(std::uint32_t{bytes[pe+7]}<<8u);opt=pe+24;
    const auto width=bytes[pe+20]|(std::uint32_t{bytes[pe+21]}<<8u);sections=opt+width;
    preferred=number(bytes.data()+opt+28);if(!count||count>32||std::uint64_t{sections}+count*40>bytes.size())std::abort();
  }
  std::uint32_t offset(std::uint32_t rva,std::size_t n) const {
    for(std::uint32_t i=0;i<count;++i){const auto* s=bytes.data()+sections+i*40;const auto va=number(s+12),size=number(s+16),raw=number(s+20);
      if(rva>=va&&rva-va<=size&&n<=size-(rva-va)&&std::uint64_t{raw}+rva-va+n<=bytes.size())return raw+rva-va;}
    if(rva<4096&&n<=4096-rva)return rva;std::abort();
  }
  void map(Memory& memory,std::uint32_t base,std::uint32_t size) const {
    memory.block(base,size);memory.bytes(base,bytes.data(),std::min<std::size_t>(4096,bytes.size()));
    for(std::uint32_t i=0;i<count;++i){const auto* s=bytes.data()+sections+i*40;const auto va=number(s+12),n=number(s+16),raw=number(s+20);
      if(n){if(va>size||n>size-va||std::uint64_t{raw}+n>bytes.size())std::abort();memory.bytes(base+va,bytes.data()+raw,n);}}
    const auto reloc=number(bytes.data()+opt+96+5*8),length=number(bytes.data()+opt+100+5*8);
    if(length){std::uint32_t consumed=0;while(consumed<length){const auto o=offset(reloc+consumed,8);const auto page=number(bytes.data()+o),n=number(bytes.data()+o+4);
      if(n<8||n>length-consumed)std::abort();const auto source=offset(reloc+consumed,n);
      for(std::uint32_t j=8;j<n;j+=2){const auto x=bytes[source+j]|(std::uint32_t{bytes[source+j+1]}<<8u);
        if((x>>12)==3){const auto a=base+page+(x&4095u);auto* p=memory.at(a,4);if(!p)std::abort();memory.put(a,number(p)+base-preferred);}}
      consumed+=n;}}
  }
};
void pe_header(Memory& m,std::uint32_t base,std::uint32_t size,std::uint32_t timestamp){
  m.block(base,4096);m.put(base,0x5a4d);m.put(base+60,128);m.put(base+128,0x4550);m.put(base+132,0x14c);
  m.put(base+136,timestamp);m.put(base+148,224);m.put(base+152,0x10b);m.put(base+180,base);m.put(base+208,size);
}
struct Fixture {
  Memory memory;d::Modules modules;
  ubc::alchemy_flask_debit::Context context{1,7,ubc::lhiver::game_parser,game,world,hero,mob};
  Fixture(const PE* exe=nullptr,const PE* zpe=nullptr){
    modules={{0x50000000,0x67000,ubc::zbug_end_compat::zbug_file_sha256,true},
      {0x60000000,0x438000,ubc::zbug_end_compat::union_file_sha256,true},
      {0x65000000,0xdd000,d::parser_extender_sha256,true},
      {0x68000000,0xbe000,d::crt_sha256,true}};
    if(exe)exe->map(memory,0x400000,0x752000);else memory.block(0x400000,0x752000);
    pe_header(memory,modules.zbug.base,modules.zbug.image_size,0x68bd012a);
    pe_header(memory,modules.union_core.base,modules.union_core.image_size,0x62b8c69e);
    pe_header(memory,modules.runtime_crt.base,modules.runtime_crt.image_size,0x4ba1dbbe);
    if(zpe)zpe->map(memory,modules.parser_extender.base,modules.parser_extender.image_size);
    else {memory.block(modules.parser_extender.base,modules.parser_extender.image_size);
      auto& block=memory.blocks[modules.parser_extender.base];std::fill(block.begin(),block.end(),0);
      // Set the header separately without shrinking the module space already allocated.
      memory.put(modules.parser_extender.base,0x5a4d);memory.put(modules.parser_extender.base+60,128);
      memory.put(modules.parser_extender.base+128,0x4550);memory.put(modules.parser_extender.base+132,0x14c);
      memory.put(modules.parser_extender.base+136,0x62b4dbf2);memory.put(modules.parser_extender.base+148,224);
      memory.put(modules.parser_extender.base+152,0x10b);memory.put(modules.parser_extender.base+180,modules.parser_extender.base);
      memory.put(modules.parser_extender.base+208,modules.parser_extender.image_size);}
    for(auto a:{game,world,hero,mob,factory,record})memory.block(a,0x1000);
    memory.put(ubc::selection_abi::game_pointer,game);memory.put(game,0x83c28c);memory.put(game+8,world);
    memory.put(world,0x83e174);memory.put(ubc::station_abi::hero_pointer,hero);memory.put(hero+0x964,mob);
    memory.put(hero+0x668,0x83c714);memory.put(0x8d8df0,factory);memory.put(factory,0x82f104);
    memory.put(0xab1168+0x3c,0xab3510);memory.put(0xab3510+0x3c,0x9a34d8);
    memory.put(0x9a34d8+0x3c,0x8d8cd8);memory.put(0x8d8cd8+0x3c,0);
    memory.put(rt::current_parser_pointer,ubc::lhiver::game_parser);
    memory.block(table,(ubc::lhiver::symbol_count+20)*4);
    memory.put(ubc::lhiver::game_parser+0x18,table);memory.put(ubc::lhiver::game_parser+0x1c,ubc::lhiver::symbol_count+20);
    memory.put(ubc::lhiver::game_parser+0x20,ubc::lhiver::symbol_count+13);
    memory.put(ubc::lhiver::game_parser+0x48,0x10000000);memory.put(ubc::lhiver::game_parser+0x4c,0x10000000);
    memory.put(ubc::lhiver::game_parser+0x50,0x10000000+ubc::lhiver::code_length+68);
    memory.put(ubc::lhiver::game_parser+0x54,ubc::lhiver::code_length+128);
    symbol(1584,class_symbol,class_name,"C_ITEM",4,0,44,0,0);symbol(1682,item_symbol,item_name,"ITEM",7,1,0,class_symbol,0x2200000);
    const auto z=modules.parser_extender.base,u=modules.union_core.base;
    memory.put(z+0xc0b1c,ubc::lhiver::game_parser);memory.put(z+0xc44c0,record);
    memory.put(z+0xa91fc,u+0x31aa0);memory.put(z+0xa9230,u+0x2d670);
    memory.block(u+0x812e8,4);memory.put(u+0x812e8,modules.runtime_crt.base+0x10233);
    memory.put(z+0xa92e4,u+0x3e96c8);memory.block(u+0x3e96c8,12);memory.put(u+0x3e96c8+8,4);
    memory.put(record,u+0x81df8);memory.put(record+12,0);memory.put(record+20,0x711bd0);
    memory.put(0x82e30c,u+0x35450);
  }
  void symbol(std::uint32_t index,std::uint32_t address,std::uint32_t name,const char* text,std::uint32_t type,
      std::uint32_t flags,std::uint32_t elements,std::uint32_t parent,std::uint32_t value){
    memory.block(address,0x3c);memory.block(name,std::strlen(text)+1);memory.bytes(name,text,std::strlen(text)+1);
    memory.put(table+index*4,address);memory.put(address+8,name);memory.put(address+12,static_cast<std::uint32_t>(std::strlen(text)));
    memory.put(address+16,static_cast<std::uint32_t>(std::strlen(text))+1);memory.put(address+0x1c,value);
    memory.put(address+0x20,elements|(type<<12u)|(flags<<16u));memory.put(address+0x38,parent);
  }
  void list(std::size_t n=3,std::size_t target=1,std::uint32_t amount=10){
    memory.put(hero+0x668+4,hero+0x668+0xa8);
    memory.put(hero+0x718,n?node(0):0);memory.put(hero+0x668+0x24,0);memory.put(hero+0x668+0x98,0);memory.put(0x8a2558,0);
    for(std::size_t i=0;i<n;++i){memory.block(node(i),12);memory.block(item(i),0x338);
      memory.put(node(i)+4,item(i));memory.put(node(i)+8,i+1<n?node(i+1):0);
      memory.put(item(i),0x83c804);memory.put(item(i)+4,3);memory.put(item(i)+0x158,0x200000);
      memory.put(item(i)+0x32c,i==target?amount:1);memory.put(item(i)+0x330,i==target?16604:1000+static_cast<std::uint32_t>(i));}
  }
};
void ikarus_binding_cases(){
  // Our symbol data checks all four address bindings. No copied Ikarus
  // or game bodies. This positive case establishes only ikarus_targets.
  constexpr std::uint32_t handler=0x3600000,base=0x3500000;
  Fixture f;std::array<std::uint32_t,4> expected{},actual{};
  for(std::size_t i=0;i<d::ikarus_bindings.size();++i){const auto& b=d::ikarus_bindings[i];
    const auto object=base+static_cast<std::uint32_t>(i)*0x100;
    f.symbol(b.symbol,object,object+0x80,b.name,b.type,b.flags,b.elements,0,0);
    f.memory.put(object+0x18,i==2?d::ikarus_functions[0].byte_offset:i==3?ubc::lhiver::game_parser:i==4?handler:0);
    if(i<expected.size())expected[i]=object+0x18;
  }
  CHECK(d::detail::ikarus_targets(f.memory.view(),handler,actual).valid&&actual==expected);
  CHECK(!d::detail::ikarus_handler(f.memory.view(),handler).valid); // Do not invent evidence for the body or handler.
  for(std::size_t i=0;i<d::ikarus_bindings.size();++i){
    const auto object=base+static_cast<std::uint32_t>(i)*0x100;
    for(const auto field:{0xcu,0x20u,0x38u}){
      const auto old=number(f.memory.at(object+field,4));f.memory.put(object+field,UINT32_MAX);
      CHECK(!d::detail::ikarus_targets(f.memory.view(),handler,actual).valid);f.memory.put(object+field,old);
    }
    const auto old=f.memory.at(object+0x80,1)[0];f.memory.at(object+0x80,1)[0]='X';
    CHECK(!d::detail::ikarus_targets(f.memory.view(),handler,actual).valid);f.memory.at(object+0x80,1)[0]=old;
    f.memory.deny=object+0x18;CHECK(!d::detail::ikarus_targets(f.memory.view(),handler,actual).valid);f.memory.deny=0;
  }
  for(const auto i:{2u,3u,4u}){
    const auto address=base+i*0x100+0x18,old=number(f.memory.at(address,4));f.memory.put(address,old+1);
    CHECK(!d::detail::ikarus_targets(f.memory.view(),handler,actual).valid);f.memory.put(address,old);
  }
  f.memory.watch=base;f.memory.change_on=3;f.memory.change_address=base+0x18;f.memory.change_value=7;
  CHECK(!d::detail::ikarus_targets(f.memory.view(),handler,actual).valid);f.memory.watch=0;
  CHECK(!d::detail::ikarus_targets(f.memory.view(),UINT32_MAX-40u,actual).valid);
  // The same symbols at fresh addresses are valid. Old absolute
  // immediates are not. The handler guard compares every operand.
  constexpr std::uint32_t moved=base+0x1000;const auto& b=d::ikarus_bindings[0];
  f.symbol(b.symbol,moved,moved+0x80,b.name,b.type,b.flags,b.elements,0,0);
  CHECK(d::detail::ikarus_targets(f.memory.view(),handler,actual).valid&&actual[0]==moved+0x18&&actual[0]!=expected[0]);
}
void portable(){
  CHECK(!d::execution_authorized);
  Fixture f;const auto item_before=d::read_item_binding(f.memory.view(),ubc::lhiver::game_parser);
  CHECK(item_before.known);CHECK(item_before.symbol==item_symbol);CHECK(item_before.value==0x2200000);
  CHECK(d::same(item_before,d::read_item_binding(f.memory.view(),ubc::lhiver::game_parser)));
  ubc::alchemy_flask_debit::Call call;call.valid=true;call.job=1;call.writer_token=2;call.context=f.context;
  call.callback=20289;call.instance=16604;call.quantity=1;call.parser.code_base=item_before.code;call.parser.code_size=item_before.code_size;
  call.parser.known=true;call.parser.instance_bindings_verified=true;call.parser.current_parser=f.context.parser;
  call.parser.self_symbol=0x3300000;call.parser.hero_symbol=0x3300040;call.parser.self=hero;call.parser.hero=hero;
  call.parser.use_symbol=call.parser.self_symbol;call.parser.use_address=hero;
  call.parser.data_depth=8;call.parser.data_words_known=true;call.parser.data_words={1,0x40,1,0x40,1,0x40,1,0x40};
  const ubc::alchemy_flask_debit::Scope scope{1,2,20289,1,0,1,true};
  auto restored=d::item_restore_plan(call,item_before,item_before,f.context,scope);CHECK(restored.valid&&!restored.requires_write);
  auto changed=item_before;changed.value=0x123456;
  restored=d::item_restore_plan(call,item_before,changed,f.context,scope);
  CHECK(restored.valid&&restored.requires_write&&restored.symbol_object==item_symbol&&restored.saved_value==item_before.value&&restored.native_entry==0x7a2f20);
  for(unsigned i=0;i<10;++i){auto invalid=call;
    if(i<8)invalid.parser.data_words[i]^=1;else if(i==8)invalid.parser.data_depth=0;else invalid.parser.data_words_known=false;
    CHECK(!d::item_restore_plan(invalid,item_before,changed,f.context,scope).valid);}
  for(unsigned field=0;field<10;++field){auto current=changed;auto context=f.context;auto active=scope;
    switch(field){case 0:current.known=false;break;case 1:++current.symbol;break;case 2:++current.table;break;
      case 3:++current.code;break;case 4:++current.code_size;break;case 5:++current.symbol_count;break;
      case 6:++context.epoch;break;case 7:++context.hero;break;case 8:active.debit_depth=0;break;case 9:active.writer_token=3;break;}
    CHECK(!d::item_restore_plan(call,item_before,current,context,active).valid);}
  for(auto field:{0x20u,0x38u,0xcu}){
    const auto old=number(f.memory.at(item_symbol+field,4));f.memory.put(item_symbol+field,0xffffffff);
    CHECK(!d::read_item_binding(f.memory.view(),ubc::lhiver::game_parser).known);f.memory.put(item_symbol+field,old);}
  f.memory.put(item_symbol+0x1c,0x2300000);CHECK(!d::same(item_before,d::read_item_binding(f.memory.view(),ubc::lhiver::game_parser)));
  f.memory.put(item_symbol+0x1c,0);CHECK(d::read_item_binding(f.memory.view(),ubc::lhiver::game_parser).known);
  f.memory.put(item_symbol+0x1c,item_before.value);f.memory.deny=item_symbol+0x1c;
  CHECK(!d::read_item_binding(f.memory.view(),ubc::lhiver::game_parser).known);f.memory.deny=0;
  CHECK(!d::read_item_binding(f.memory.view(),1234).known);
  const auto good=d::read_objects(f.memory.view(),f.context);CHECK(good.stable&&good.flask_class_is_vob_item);
  for(auto address:{game+8,world,factory,hero+0x964,0xab1168u+0x3c}){
    const auto old=number(f.memory.at(address,4));f.memory.put(address,0);
    CHECK(!d::read_objects(f.memory.view(),f.context).stable);f.memory.put(address,old);}
  f.memory.put(0xab3510+0x3c,0xab1e20);CHECK(!d::read_objects(f.memory.view(),f.context).stable);
  f.memory.put(0xab3510+0x3c,0xab1168);CHECK(!d::read_objects(f.memory.view(),f.context).stable);f.memory.put(0xab3510+0x3c,0);
  const auto fields=[&](){return d::read_pending_facts(f.memory.view(),f.modules,d::ReadDepth::fields_only);};
  auto pending=fields();
  CHECK(pending.read_stable&&pending.allocator_target_matches_export&&pending.wrapper_parser_matches&&pending.original_dispatch_fields_match&&pending.item_known);
  CHECK(!pending.allocator_boundary_verified&&!pending.engine_version_verified&&!pending.wrapper_dispatch_verified);
  CHECK(!d::read_pending_facts(f.memory.view(),f.modules).read_stable); // Unverified export and code windows remain blocked.
  CHECK(!d::execution_authorized);f.memory.put(record+20,0x777777);
  pending=fields();CHECK(pending.read_stable&&!pending.original_dispatch_fields_match);
  f.memory.put(record+20,0x711bd0);f.memory.put(0x82e30c,f.modules.union_core.base+0x1a40);
  CHECK(!fields().read_stable);f.memory.put(0x82e30c,f.modules.union_core.base+0x35450);
  f.memory.deny=f.modules.union_core.base+0x812e8;CHECK(!fields().read_stable);f.memory.deny=0;
  f.memory.watch=0x82e30c;f.memory.change_on=2;f.memory.change_address=record+20;f.memory.change_value=1;
  CHECK(fields().issue==d::Issue::changed);
  f.memory.watch=0;f.memory.put(record+20,0x711bd0);f.memory.watch=item_symbol;f.memory.watch_reads=0;
  f.memory.change_on=3;f.memory.change_address=item_symbol+0x1c;f.memory.change_value=0x123456;
  CHECK(!d::read_item_binding(f.memory.view(),ubc::lhiver::game_parser).known);
  f.memory.watch=0;f.memory.put(item_symbol+0x1c,item_before.value);
  f.memory.watch=game+8;f.memory.watch_reads=0;f.memory.change_on=2;f.memory.change_address=world;f.memory.change_value=1;
  CHECK(!d::read_objects(f.memory.view(),f.context).stable);f.memory.watch=0;f.memory.put(world,0x83e174);
  static d::ListReader reader(7);CHECK(reader.capture(f.memory.view(),8,{hero,item(1),2}).issue==d::Issue::thread);
  CHECK(reader.capture(f.memory.view(),7,{hero,item(1),0}).issue==d::Issue::range);
  auto target=d::expected_targets;++target.remove;
  CHECK(d::verify_known_code(f.memory.view(),f.modules,target).issue==d::Issue::targets);
  auto modules=f.modules;modules.parser_extender.mapping_verified=false;
  CHECK(d::verify_known_code(f.memory.view(),modules,d::expected_targets).issue==d::Issue::module);
  modules=f.modules;modules.zbug.base=modules.union_core.base;
  CHECK(d::verify_known_code(f.memory.view(),modules,d::expected_targets).issue==d::Issue::module);
  CHECK(!d::verify_known_code(f.memory.view(),f.modules,d::expected_targets).known_ranges_match); // Our zero-filled bodies cannot establish original code identity.
  std::array<std::uint8_t,4> output{};CHECK(d::detail::twice(f.memory.view(),UINT32_MAX,output.data(),4).issue==d::Issue::range);
}
void local(const PE& exe,const PE& zpe){
  Fixture f(&exe,&zpe);
  CHECK(d::verify_known_code(f.memory.view(),f.modules,d::expected_targets).known_ranges_match);
  const auto call=[&](std::uint32_t address,std::uint32_t target){f.memory.put(address+1,target-(address+5u));};
  call(0x70d0e6,f.modules.zbug.base+0x11e30);call(0x76fe7f,f.modules.parser_extender.base+0x7d560);call(0x7926fc,f.modules.parser_extender.base+0x71c70);
  auto code=d::verify_known_code(f.memory.view(),f.modules,d::expected_targets);
  CHECK(code.known_ranges_match&&code.zbug_unpack_call&&code.zpe_init_call&&code.zpe_error_call&&!code.ikarus_handler);
  for(auto address:{0x70d0e7u,0x76fe80u,0x7926fdu,0x711bd0u,0x7495f0u,0x791970u}){
    auto* p=f.memory.at(address,1);const auto old=*p;*p^=1;
    CHECK(!d::verify_known_code(f.memory.view(),f.modules,d::expected_targets).known_ranges_match);*p=old;}
  for(const auto& c:d::zpe_ranges){auto* p=f.memory.at(f.modules.parser_extender.base+c.rva,1);const auto old=*p;*p^=1;
    CHECK(!d::verify_known_code(f.memory.view(),f.modules,d::expected_targets).known_ranges_match);*p=old;}
  for(const auto& s:d::additional_slots){const auto old=number(f.memory.at(s.address,4));f.memory.put(s.address,old+1);
    CHECK(!d::verify_known_code(f.memory.view(),f.modules,d::expected_targets).known_ranges_match);f.memory.put(s.address,old);}
  const auto handler=number(f.memory.at(0x791963,4));f.memory.put(0x791963,0x200d000);
  CHECK(!d::verify_known_code(f.memory.view(),f.modules,d::expected_targets).known_ranges_match);f.memory.put(0x791963,handler);
  f.memory.deny=0x711bd0;CHECK(!d::verify_known_code(f.memory.view(),f.modules,d::expected_targets).known_ranges_match);f.memory.deny=0;
  f.memory.watch=0x70d080;f.memory.change_on=3;f.memory.change_address=0x70d0e7;f.memory.change_value=0x70f8f0u-0x70d0ebu;
  CHECK(d::verify_known_code(f.memory.view(),f.modules,d::expected_targets).issue==d::Issue::changed);f.memory.watch=0;
  call(0x70d0e6,f.modules.zbug.base+0x11e30);
  f.list();static d::ListReader reader(7);const d::ListRequest q{hero,item(1),2};
  auto stock=reader.capture(f.memory.view(),7,q);CHECK(stock.stable_complete&&stock.unpack_unreachable&&stock.path==d::RemovalPath::split&&stock.nodes==3);
  reentrant_reader=&reader;f.memory.callback=[](Memory& m) noexcept{reentrant_issue=reentrant_reader->capture(m.view(),7,{hero,item(1),2}).issue;};
  CHECK(reader.capture(f.memory.view(),7,q).stable_complete);CHECK(reentrant_issue==d::Issue::reentry);reentrant_reader=nullptr;
  // The previous cost reader skipped unrelated items BEFORE the flask
  // stack. Here such an item must block the native GetInstance scan.
  f.memory.put(item(0),0xdeadbeef);CHECK(reader.capture(f.memory.view(),7,q).issue==d::Issue::item);f.memory.put(item(0),0x83c804);
  for(auto address:{item(1)+0x158,item(1)+4,item(1)+0x32c}){
    const auto old=number(f.memory.at(address,4));f.memory.put(address,0);CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);f.memory.put(address,old);}
  f.memory.put(item(1)+0x158,0x40200000);CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);f.memory.put(item(1)+0x158,0x200000);
  f.memory.put(item(2)+0x330,16604);CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);f.memory.put(item(2)+0x330,1002);
  f.memory.put(node(2)+8,node(0));CHECK(reader.capture(f.memory.view(),7,q).issue==d::Issue::duplicate);f.memory.put(node(2)+8,0);
  f.memory.put(node(2)+4,item(0));CHECK(reader.capture(f.memory.view(),7,q).issue==d::Issue::duplicate);f.memory.put(node(2)+4,item(2));
  f.memory.deny=item(0);CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);f.memory.deny=0;
  f.memory.put(0x83c804+0x78,0x711421);CHECK(reader.capture(f.memory.view(),7,q).issue==d::Issue::vtable);f.memory.put(0x83c804+0x78,0x711420);
  f.memory.put(item(1)+0x32c,2);stock=reader.capture(f.memory.view(),7,q);
  CHECK(stock.stable_complete&&stock.path==d::RemovalPath::whole&&stock.whole_short_path);
  for(const auto& change:std::array<std::pair<std::uint32_t,std::uint32_t>,8>{{
      {hero+0x668+0x24,2},{hero+0x668+0x98,1},{0x8a2558,1},{item(1)+4,1},{item(1)+4,INT32_MAX},
      {item(1)+4,INT32_MAX-1},{hero+0x668+4,0},{hero+0x668+4,node(0)}}}){
    const auto old=number(f.memory.at(change.first,4));f.memory.put(change.first,change.second);
    CHECK(reader.capture(f.memory.view(),7,q).issue==d::Issue::whole_stack);f.memory.put(change.first,old);}
  f.memory.put(item(1)+4,INT32_MAX-2);CHECK(reader.capture(f.memory.view(),7,q).stable_complete);f.memory.put(item(1)+4,3);
  f.memory.watch=item(1)+0x32c;f.memory.watch_reads=0;f.memory.change_on=2;f.memory.change_address=item(1)+0x32c;f.memory.change_value=3;
  CHECK(reader.capture(f.memory.view(),7,q).issue==d::Issue::changed);f.memory.watch=0;f.memory.put(item(1)+0x32c,2);
  f.memory.callback=[](Memory& m) noexcept{m.put(hero+0x718,0);};CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);
  f.list(1,0,2);CHECK(reader.capture(f.memory.view(),7,{hero,item(0),2}).issue==d::Issue::whole_stack);
  f.list(d::list_capacity,d::list_capacity-1,10);CHECK(reader.capture(f.memory.view(),7,{hero,item(d::list_capacity-1),2}).stable_complete);
  f.list(d::list_capacity+1,d::list_capacity,10);CHECK(reader.capture(f.memory.view(),7,{hero,item(d::list_capacity),2}).issue==d::Issue::list);
  CHECK(!d::execution_authorized);
}
void whole_local(const PE& exe){
  // Native bodies come only from the SHA-verified local EXE in RAM. All
  // object, list and pool data here is synthetic. No world authorization.
  constexpr std::uint32_t headers=0x6100000,world_nodes=0x6200000,free_nodes=0x6300000;
  constexpr std::uint32_t pool=0x8d83b4;
  Fixture f(&exe);f.memory.block(world,0x7000);f.memory.put(world,0x83e174);
  f.memory.block(headers,32);f.memory.block(world_nodes,64);f.memory.block(free_nodes,48);
  const auto prepare=[&](bool present0=false,bool present1=false){
    f.list(3,1,2);f.memory.put(item(1)+4,1);
    f.memory.put(world+0x6280,headers);f.memory.put(world+0x6288,headers+16);
    for(unsigned i=0;i<2;++i){f.memory.put(headers+i*16,i?0x77ef90:0x77ef50);f.memory.put(headers+i*16+4,0);
      f.memory.put(headers+i*16+8,world_nodes+i*16);f.memory.put(world_nodes+i*16,0);
      f.memory.put(world_nodes+i*16+4,(i?present1:present0)?item(1):0x6400000+i*16);f.memory.put(world_nodes+i*16+8,0);}
    f.memory.put(pool,12);f.memory.put(pool+4,4096);f.memory.put(pool+8,0);f.memory.put(pool+12,free_nodes);f.memory.put(pool+16,2);
    f.memory.put(free_nodes,free_nodes+16);f.memory.put(free_nodes+4,pool+12);f.memory.put(free_nodes+16,0);f.memory.put(free_nodes+20,free_nodes);
    f.memory.put(0x8d8780,0);f.memory.put(0x8a2558,0);f.memory.deny=f.memory.watch=0;
  };
  // Persistent workspace as in the DLL. Inlining must not put
  // several such readers together on the small native x86 stack.
  static d::ListReader reader(7);const d::ListRequest q{hero,item(1),2,world};
  // World run 1: six listed apples with one reference. Any missing
  // base class blocks full removal. Splitting remains possible.
  prepare();f.memory.put(item(1)+0x330,16660);f.memory.put(item(1)+0x32c,6);
  const d::ListRequest apples{hero,item(1),6,world,16660};
  CHECK(reader.capture(f.memory.view(),7,apples).whole_short_path);
  for(auto cls:{0xab1168u,0xab3510u,0x9a34d8u}){
    const auto old=number(f.memory.at(cls+0x3c,4));f.memory.put(cls+0x3c,0);
    CHECK(!reader.capture(f.memory.view(),7,apples).stable_complete);
    CHECK(reader.capture(f.memory.view(),7,{hero,item(1),5,world,16660}).stable_complete);
    f.memory.put(cls+0x3c,old);
  }
  for(unsigned mask=0;mask<4;++mask){prepare((mask&1)!=0,(mask&2)!=0);const auto x=reader.capture(f.memory.view(),7,q);
    CHECK(x.stable_complete==(mask!=3));CHECK(x.whole_short_path==(mask!=3));
    CHECK(x.whole_world.verified==(mask!=3));
    if(mask!=3){CHECK(x.whole_world.world==world);CHECK(x.whole_world.added_references==2u-unsigned{bool(mask&1)}-unsigned{bool(mask&2)});
      CHECK(x.whole_world.ref_before_release==1u+x.whole_world.added_references);CHECK(x.whole_world.nodes[0]==1&&x.whole_world.nodes[1]==1);}
  }
  prepare();CHECK(!reader.capture(f.memory.view(),7,{hero,item(1),2}).stable_complete);
  CHECK(!reader.capture(f.memory.view(),7,{hero,item(1),2,world+4}).stable_complete);
  CHECK(!reader.capture(f.memory.view(),7,{hero,item(1),2,UINT32_MAX}).stable_complete);
  CHECK(reader.capture(f.memory.view(),7,{hero,item(1),1,0}).stable_complete); // Splitting is unchanged.
  prepare();f.memory.put(headers+8,0);f.memory.put(headers+24,0);
  auto empty_world=reader.capture(f.memory.view(),7,q);CHECK(empty_world.stable_complete);
  CHECK((empty_world.whole_world.nodes==std::array<std::uint32_t,2>{}));
  CHECK(empty_world.whole_world.added_references==2&&empty_world.whole_world.ref_before_release==3);
  prepare();
  const auto positive=reader.capture(f.memory.view(),7,q);CHECK(positive.stable_complete);
  CHECK(d::same_list_evidence(positive,reader.capture(f.memory.view(),7,q)));
  auto altered=positive;++altered.whole_world.added_references;CHECK(!d::same_list_evidence(positive,altered));
  altered=positive;altered.whole_world.fingerprint[0]^=1;CHECK(!d::same_list_evidence(positive,altered));
  for(const auto& change:std::array<std::pair<std::uint32_t,std::uint32_t>,24>{{
    {hero+0x668+4,0},{hero+0x668+0x24,2},{hero+0x668+0x98,1},{0x8a2558,1},{0x8d8780,1},
    {world+0x6280,0},{world+0x6288,headers},{headers,0x77ef90},{headers+16,0x77ef50},
    {pool,16},{pool+4,1024},{pool+12,0},{pool+16,1},{free_nodes+4,0},{free_nodes+20,0},
    {world_nodes+4,0},{world_nodes+8,world_nodes},{headers+24,world_nodes},
    {0xab1168+0x3c,0xab1e20},{0xab3510+0x3c,0xab1168},{0xab3510+0x3c,0},{0x9a34d8+0x3c,0},{0x8d8cd8+0x3c,0xab1168},{world,0x83e175}}}){
    prepare();const auto old=number(f.memory.at(change.first,4));f.memory.put(change.first,change.second);
    CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);f.memory.put(change.first,old);
  }
  // A world node must never alias an inventory or pool node that will be freed later.
  prepare();f.memory.put(headers+8,node(0));CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);
  prepare();f.memory.put(headers+8,free_nodes);f.memory.put(free_nodes+8,0);CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);
  prepare();f.memory.put(pool+12,node(1));CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);
  prepare();f.memory.block(0x6400000,12);f.memory.put(0x6400000+4,0x7100000);f.memory.put(0x6400000+8,0);
  f.memory.put(world_nodes+8,0x6400000);f.memory.put(world_nodes+12,0);f.memory.put(headers+24,world_nodes+4);
  CHECK(reader.capture(f.memory.view(),7,q).issue==d::Issue::duplicate); // Partial overlap of 4 bytes.
  // Duplicate targets cannot be assumed to be a normal world list.
  prepare(true,false);f.memory.put(world_nodes+8,world_nodes+32);f.memory.put(world_nodes+32,0);
  f.memory.put(world_nodes+36,item(1));f.memory.put(world_nodes+40,0);CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);
  prepare();for(const auto& range:d::whole_world_ranges){auto* byte=f.memory.at(range.address,1);const auto saved=*byte;*byte^=1;
    CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);*byte=saved;}
  for(const auto& slot:d::whole_world_slots){const auto old=number(f.memory.at(slot.address,4));f.memory.put(slot.address,old+1);
    CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);f.memory.put(slot.address,old);}
  prepare();for(const auto address:{world+0x6280,headers,world_nodes,pool,free_nodes,item(1)+4}){
    f.memory.deny=address;CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);f.memory.deny=0;}
  prepare();f.memory.watch=world+0x6280;f.memory.change_on=2;f.memory.watch_reads=0;
  f.memory.change_address=world_nodes+4;f.memory.change_value=item(1);CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);
  prepare();f.memory.watch=item(1)+4;f.memory.change_on=3;f.memory.watch_reads=0;
  f.memory.change_address=item(1)+4;f.memory.change_value=2;CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);
  prepare();f.memory.watch=0x5664e0;f.memory.change_on=3;f.memory.watch_reads=0;
  f.memory.change_address=0x5664e0;f.memory.change_value=0;CHECK(!reader.capture(f.memory.view(),7,q).stable_complete);
  // Test the exact world list limit, then a longer list. Both
  // are read-only. A cycle reaches the same limit at the latest.
  Fixture bounded(&exe);bounded.memory.block(world,0x7000);bounded.memory.put(world,0x83e174);
  bounded.list(3,1,2);bounded.memory.put(item(1)+4,1);
  constexpr std::uint32_t long_nodes=0x6800000;
  bounded.memory.block(headers,32);bounded.memory.block(free_nodes,48);
  bounded.memory.block(long_nodes,(d::whole_world_node_capacity+1)*12);
  bounded.memory.put(world+0x6280,headers);bounded.memory.put(world+0x6288,headers+16);
  bounded.memory.put(headers,0x77ef50);bounded.memory.put(headers+8,long_nodes);
  bounded.memory.put(headers+16,0x77ef90);bounded.memory.put(headers+24,0);
  bounded.memory.put(pool,12);bounded.memory.put(pool+4,4096);bounded.memory.put(pool+12,free_nodes);bounded.memory.put(pool+16,2);
  bounded.memory.put(free_nodes,free_nodes+16);bounded.memory.put(free_nodes+4,pool+12);
  bounded.memory.put(free_nodes+16,0);bounded.memory.put(free_nodes+20,free_nodes);bounded.memory.put(0x8d8780,0);
  for(std::uint32_t i=0;i<=d::whole_world_node_capacity;++i){const auto a=long_nodes+i*12;
    bounded.memory.put(a+4,0x7000000+i*16);bounded.memory.put(a+8,i+1<d::whole_world_node_capacity?a+12:0);}
  auto longest=reader.capture(bounded.memory.view(),7,q);CHECK(longest.stable_complete);
  CHECK(longest.whole_world.nodes[0]==d::whole_world_node_capacity);
  bounded.memory.put(long_nodes+(static_cast<std::uint32_t>(d::whole_world_node_capacity)-1)*12+8,long_nodes+static_cast<std::uint32_t>(d::whole_world_node_capacity)*12);
  CHECK(reader.capture(bounded.memory.view(),7,q).issue==d::Issue::list);
  // Ref>=2 keeps its existing evidence requirements. The new Ref1
  // world option must not add unrelated prerequisites to the old path.
  Fixture old(&exe);old.list(3,1,2);CHECK(reader.capture(old.memory.view(),7,{hero,item(1),2}).stable_complete);
}
} // namespace
int main(int argc,char** argv){
  portable();ikarus_binding_cases();
  if(argc==4&&std::string(argv[1])=="--local-facts"){
    const PE exe(argv[2],ubc::station_abi::executable_sha256),zpe(argv[3],d::parser_extender_sha256);local(exe,zpe);whole_local(exe);
  }else if(argc==3&&std::string(argv[1])=="--whole-local-facts"){
    const PE exe(argv[2],ubc::station_abi::executable_sha256);whole_local(exe);
  }else if(argc!=1)return 2;
  std::printf("alchemy-extra-dependencies: %u checks, %u failures%s\n",checks,failures,argc==1?" (synthetic negative guards only)":" (local files read as RAM-only data)");
  return failures?1:0;
}
