// SPDX-License-Identifier: MIT
#include "ubc/cool_nofx_reader.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <memory>
namespace allocation_probe {bool watching=false;unsigned count=0;}
void* operator new(std::size_t n){if(allocation_probe::watching)++allocation_probe::count;
  if(auto* p=std::malloc(n?n:1))return p;std::abort();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace d=ubc::alchemy_extra_dependencies;
namespace nf=ubc::cool_nofx;
namespace {
unsigned checks=0,failures=0;
void check(bool ok,int line){++checks;if(!ok){++failures;std::fprintf(stderr,"failure line %d\n",line);}}
#define CHECK(x) check((x),__LINE__)
// Reuse the existing MIT PE/ReadOnlyMemory fixture. No live access.
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
constexpr std::uint32_t game=0x2000000,world=0x2010000,hero=0x2020000,mob=0x2030000,vob=0x2040000;
constexpr std::uint32_t root=world+0x24,n1=0x3000000,n2=0x3000040,n3=0x3000080;
constexpr std::uint32_t table=0x4000000,code=0x10000000,string_object=0x7000000,buffer=0x7000100;
constexpr std::uint32_t symbol(std::uint32_t i){return 0x5000000+i*0x40;}
constexpr std::uint32_t name(std::uint32_t i){return 0x6000000+i*0x40;}
struct Fixture {
  Memory memory;nf::Request request{};nf::Workspace<16> workspace;nf::Reader<16> reader{7,workspace};
  Fixture(const PE& engine,const PE& core,const PE& crt,const std::vector<std::uint8_t>& script){
    request.job={{1,7,ubc::lhiver::game_parser,game,world,hero,mob},1,2,3,20775,20773,2};
    request.serial=1;request.frame=19;request.current_thread=7;request.stack={true,7,0x70000000,0x70100000};
    request.engine={0x400000,0x752000,nf::profile::engine_sha256,true};
    request.union_core={0x60000000,0x438000,ubc::zbug_end_compat::union_file_sha256,true};
    request.crt={0x68000000,0xbe000,d::crt_sha256,true};
    engine.map(memory,request.engine.base,request.engine.image_size);
    core.map(memory,request.union_core.base,request.union_core.image_size);
    crt.map(memory,request.crt.base,request.crt.image_size);
    memory.put(0x82e308,request.union_core.base+0x35460);
    memory.put(request.union_core.base+0x812ec,request.crt.base+0x1014e);
    memory.block(game,12);memory.put(game+8,world);
    memory.block(world,0x628c);memory.put(world,0x83e174);
    memory.block(hero,0xbc);memory.put(hero,0x83d724);memory.put(hero+0xb8,world);
    memory.block(mob,4);memory.block(vob,4);memory.put(vob,0x83dfe4);
    memory.put(ubc::selection_abi::game_pointer,game);memory.put(ubc::station_abi::hero_pointer,hero);
    memory.put(ubc::alchemy_flask_runtime::current_parser_pointer,ubc::lhiver::game_parser);
    memory.put(0xab642c,1);
    memory.put(root+4,n1);node(n1,root,n2,0,hero);node(n2,root,0,n1,vob);
    const auto p=ubc::lhiver::game_parser;
    memory.block(table,ubc::lhiver::symbol_count*4);memory.block(code,ubc::lhiver::code_length);
    memory.put(p+0x18,table);memory.put(p+0x1c,ubc::lhiver::symbol_count);memory.put(p+0x20,ubc::lhiver::symbol_count);
    memory.put(p+0x48,code);memory.put(p+0x50,code+ubc::lhiver::code_length);memory.put(p+0x54,ubc::lhiver::code_length);
    put_symbol(1537,"C_NPC",4,0,32,0,0,0);
    put_symbol(1679,"SELF",7,1,0,symbol(1537),0,hero);put_symbol(1683,"HERO",7,1,0,symbol(1537),0,hero);
    put_symbol(19965,"B_ENDPRODUCTIONDIALOG",5,1,0,0,672156,0);
    memory.bytes(code+672156,script.data(),script.size());
    put_symbol(742,"WLD_STOPEFFECT",5,9,1,0,0x6e32b0,0);
    // Deliberately different anonymous runtime name. The index, body, type and value are required.
    put_symbol(51135,"runtime-anonymous",3,1,1,0,string_object,0);
    memory.block(string_object,20);memory.put(string_object,0x82e6f0);
    memory.put(string_object+8,buffer+1);memory.put(string_object+12,11);memory.put(string_object+16,15);
    memory.block(buffer,13);memory.bytes(buffer+1,"DEMENTOR_FX",12);
  }
  void put_symbol(std::uint32_t i,const char* text,std::uint32_t type,std::uint32_t flags,
                  std::uint32_t elements,std::uint32_t parent,std::uint32_t data,std::uint32_t offset){
    memory.block(symbol(i),0x3c);memory.block(name(i),std::strlen(text)+1);
    memory.bytes(name(i),text,std::strlen(text)+1);memory.put(table+i*4,symbol(i));
    memory.put(symbol(i)+8,name(i));memory.put(symbol(i)+12,static_cast<std::uint32_t>(std::strlen(text)));
    memory.put(symbol(i)+0x18,data);memory.put(symbol(i)+0x1c,offset);
    memory.put(symbol(i)+0x20,elements|(type<<12)|(flags<<16));memory.put(symbol(i)+0x38,parent);
  }
  void node(std::uint32_t a,std::uint32_t parent,std::uint32_t next,std::uint32_t previous,std::uint32_t data){
    memory.block(a,20);memory.put(a,parent);memory.put(a+8,next);memory.put(a+12,previous);memory.put(a+16,data);
  }
  nf::Capture capture(){const auto before=allocation_probe::count;allocation_probe::watching=true;
    const auto r=reader.capture(memory.view(),request);allocation_probe::watching=false;
    CHECK(allocation_probe::count==before);return r;}
  std::uint32_t export_slot(const nf::Module& module,std::uint32_t ordinal){
    const auto pe=number(memory.at(module.base+60,4));
    const auto exp=number(memory.at(module.base+pe+120,4));
    const auto base=number(memory.at(module.base+exp+16,4));
    const auto table_rva=number(memory.at(module.base+exp+28,4));
    return module.base+table_rva+(ordinal-base)*4;
  }
};
nf::Reader<16>* reenter_reader=nullptr;nf::Request reenter_request{};nf::Issue reenter_issue{};
void reenter(Memory& memory) noexcept {reenter_issue=reenter_reader->capture(memory.view(),reenter_request).issue;}

void portable(){
  CHECK(!nf::runtime_ready&&!nf::profile::runtime_ready&&!nf::Capture{}.known&&!nf::Capture::native_effect_authorized);
  Memory memory;nf::Workspace<4> workspace;nf::Reader<4> reader(7,workspace);nf::Request q{};
  CHECK(reader.capture(memory.view(),q).issue==nf::Issue::thread&&memory.reads==0);
  q.current_thread=7;q.job.context.thread=7;
  CHECK(reader.capture(memory.view(),q).issue==nf::Issue::request&&memory.reads==0);
  q.job={{1,7,ubc::lhiver::game_parser,game,world,hero,mob},1,2,3,20775,20773,2};
  q.frame=1;q.serial=1;q.stack={true,7,0x70000000,0x70100000};
  CHECK(reader.capture(memory.view(),q).issue==nf::Issue::module&&memory.reads==0);
  // Leaf and node reader on synthetic RAM only. No original bodies.
  memory.block(n1,20);memory.put(n1+16,vob);memory.block(vob,4);memory.put(vob,0x83dfe4);
  memory.block(0x83dfe4,4);memory.put(0x83dfe4,0x77b6c0);memory.block(0x77b6c0,6);
  std::array<std::uint8_t,6> leaf{{0xb8,0,0,0,0,0xc3}};d::detail::put(leaf.data()+1,0xab3510);
  memory.bytes(0x77b6c0,leaf.data(),leaf.size());nf::Literal literal{};literal.data=buffer+1;
  nf::detail::Node node{};node.address=n1;
  CHECK(nf::detail::node(memory.view(),q,literal,node)==nf::Issue::none&&node.classdef==0xab3510);
  CHECK(!nf::Capture{}.known); // Verifying one leaf never establishes the full tree.
  for(unsigned i=0;i<leaf.size();++i){auto changed=leaf;changed[i]^=1;
    memory.bytes(0x77b6c0,changed.data(),changed.size());node={};node.address=n1;
    CHECK(nf::detail::node(memory.view(),q,literal,node)==nf::Issue::getter);}
  memory.bytes(0x77b6c0,leaf.data(),leaf.size());memory.put(n1+12,n2);node={};node.address=n1;
  CHECK(nf::detail::node(memory.view(),q,literal,node)==nf::Issue::tree);
  memory.put(n1+12,0);memory.deny=n1+16;node={};node.address=n1;
  CHECK(nf::detail::node(memory.view(),q,literal,node)==nf::Issue::unreadable);
  memory.deny=0;literal.data=n1+1;node={};node.address=n1;
  CHECK(nf::detail::node(memory.view(),q,literal,node)==nf::Issue::alias);
  for(std::size_t i=1;i<nf::profile::constant_returns.size();++i)
    CHECK(nf::profile::constant_returns[i-1].entry<nf::profile::constant_returns[i].entry);
  for(std::size_t i=0;i<nf::profile::constant_slots.size();++i){const auto& slot=nf::profile::constant_slots[i];
    CHECK(nf::detail::constant_return(slot.entry)&&(!i||nf::profile::constant_slots[i-1].address<slot.address));}
}

void run(const PE& engine,const PE& core,const PE& crt,const std::vector<std::uint8_t>& script){
  CHECK(!nf::runtime_ready&&!nf::Capture::native_effect_authorized);
  auto valid=[&](){return std::make_unique<Fixture>(engine,core,crt,script);};
  {auto f=valid();const auto r=f->capture();
    if(!r.known)std::fprintf(stderr,"positive issue=%u failed=%x\n",unsigned(r.issue),r.failed_address);
    CHECK(r.known&&r.issue==nf::Issue::none&&r.nodes==3&&r.vobs==2&&r.depth==1&&!r.first_fx);
    CHECK(r.literal.data==buffer+1&&r.literal.actor.self==hero&&r.literal.actor.hero==hero);
    CHECK(r.free_entry==f->request.union_core.base+0x35460&&r.free_forward==f->request.crt.base+0x1014e);
    CHECK(f->capture().issue==nf::Issue::serial);++f->request.serial;CHECK(f->capture().known);
  }
  // Count the root and all descendants. A matching class provider blocks the path without
  // reading FX names, the stop target or FX children, or calling an engine getter.
  for(bool nested:{false,true}){auto f=valid();f->memory.put(vob,0x83029c);
    if(nested){f->memory.put(n1+4,n3);f->node(n3,n1,0,0,vob);f->memory.put(n2+16,0);}
    auto r=f->capture();CHECK(!r.known&&r.issue==nf::Issue::fx_present&&r.first_fx==vob);}
  {auto f=valid();f->memory.put(root+16,vob);f->memory.put(vob,0x83029c);CHECK(f->capture().issue==nf::Issue::fx_present);}
  for(unsigned bad=0;bad<9;++bad){auto f=valid();
    switch(bad){case 0:f->memory.put(n2+8,n1);break;case 1:f->memory.put(n1+4,n1);break;
      case 2:f->memory.put(n2,hero);break;case 3:f->memory.put(n2+12,0);break;
      case 4:f->memory.put(root+8,n1);break;case 5:f->memory.put(n1+4,n2);break;
      case 6:f->memory.put(n2+16,hero);break;case 7:f->memory.put(n2+8,0xfffffffcu);break;
      case 8:f->memory.deny=n2+8;break;}
    CHECK(!f->capture().known);
  }
  {auto f=valid();f->memory.put(root+4,0);CHECK(f->capture().issue==nf::Issue::hero_missing);}
  {auto f=valid();nf::Workspace<2> w;nf::Reader<2> reader(7,w);CHECK(reader.capture(f->memory.view(),f->request).issue==nf::Issue::capacity);}
  {auto f=valid();nf::Workspace<256> w;nf::Reader<256> reader(7,w);
    f->memory.put(root+4,n1);f->memory.put(n1+8,0);std::uint32_t previous=n1;
    for(unsigned i=0;i<130;++i){const auto a=0x3100000+i*0x40;f->node(a,previous,0,0,0);f->memory.put(previous+4,a);previous=a;}
    CHECK(reader.capture(f->memory.view(),f->request).issue==nf::Issue::tree);
  }
  for(unsigned bad=0;bad<5;++bad){auto f=valid();
    switch(bad){case 0:f->memory.put(vob,0x50010000);f->memory.block(0x50010000,4);f->memory.put(0x50010000,0x77b6c0);break;
      case 1:f->memory.put(0x83dfe4,0x72e370);break;case 2:*f->memory.at(0x77b6c0,1)=0xe9;break;
      case 3:f->memory.put(0x83dfe4,0x77b6c1);break;case 4:f->memory.deny=0x77b6c0;break;}
    CHECK(!f->capture().known);
  }
  // Check each generated slot/leaf combination against the actual file
  // bytes. An unbound test pattern cannot authorize a positive result.
  for(const auto& slot:nf::profile::constant_slots){auto f=valid();f->memory.put(vob,slot.address);
    const auto r=f->capture();const auto* leaf=nf::detail::constant_return(slot.entry);
    CHECK(leaf&&((leaf->value==nf::profile::fx_class)?(!r.known&&r.issue==nf::Issue::fx_present):r.known));}
  {auto f=valid();f->memory.watch=0x77b6c0;f->memory.change_on=2;
    f->memory.change_address=0x77b6c0;f->memory.change_value=0x90909090;
    const auto r=f->capture();CHECK(!r.known&&r.issue==nf::Issue::changed);}
  for(unsigned bad=0;bad<14;++bad){auto f=valid();
    switch(bad){case 0:f->memory.put(table+51135*4,symbol(742));break;
      case 1:f->memory.put(symbol(51135)+0x20,1|(2<<12)|(1<<16));break;
      case 2:f->memory.put(symbol(51135)+0x38,symbol(1537));break;
      case 3:f->memory.put(symbol(51135)+0x1c,4);break;case 4:f->memory.put(string_object+12,10);break;
      case 5:*f->memory.at(buffer+1,1)='X';break;case 6:*f->memory.at(buffer+12,1)='X';break;
      case 7:*f->memory.at(buffer,1)=0xfe;break;case 8:*f->memory.at(buffer,1)=0xff;break;
      case 9:f->memory.put(0xab642c,0);break;case 10:f->memory.put(symbol(742)+0x18,0x6e3070);break;
      case 11:*f->memory.at(code+672166,1)=0x43;break;case 12:f->memory.put(symbol(1679)+0x1c,mob);break;
      case 13:f->memory.put(string_object+8,0);break;}
    CHECK(!f->capture().known);
  }
  for(unsigned ref:{0u,1u,128u,253u}){auto f=valid();*f->memory.at(buffer,1)=static_cast<std::uint8_t>(ref);CHECK(f->capture().known);}
  for(unsigned bad=0;bad<7;++bad){auto f=valid();
    switch(bad){case 0:f->request.stack.verified=false;break;case 1:f->request.stack.thread=8;break;
      case 2:f->request.stack.low=buffer;f->request.stack.high=buffer+0x100;break;
      case 3:f->request.stack.low=string_object;f->request.stack.high=string_object+0x100;break;
      case 4:f->request.stack.low=n1;f->request.stack.high=n1+0x100;break;
      case 5:f->memory.put(string_object+8,n2+1);f->memory.bytes(n2,"\0DEMENTOR_FX",13);break;
      case 6:f->request.stack.high=f->request.stack.low;break;}
    CHECK(!f->capture().known);
  }
  for(unsigned bad=0;bad<10;++bad){auto f=valid();
    switch(bad){case 0:f->request.engine.mapping_verified=false;break;case 1:f->request.union_core.file_sha256[0]^=1;break;
      case 2:f->request.crt.base=f->request.union_core.base;break;case 3:f->memory.put(0x82e308,f->request.union_core.base+0x35450);break;
      case 4:f->memory.put(f->request.union_core.base+0x812ec,f->request.crt.base+0x10233);break;
      case 5:*f->memory.at(f->request.union_core.base+0x35460,1)=0x90;break;
      case 6:*f->memory.at(f->request.crt.base+0x10157,1)=0x75;break;
      case 7:*f->memory.at(f->request.crt.base+0x10172,1)=0x90;break;
      case 8:f->memory.put(f->request.crt.base+0x3c,0xfffffff0);break;
      case 9:*f->memory.at(0x624250+100,1)^=1;break;}
    CHECK(!f->capture().known);
  }
  for(unsigned bad=0;bad<3;++bad){auto f=valid();
    if(bad==0)f->memory.put(f->export_slot(f->request.union_core,259),0x35450);
    if(bad==1)f->memory.put(f->export_slot(f->request.crt,1420),0x10233);
    if(bad==2){const auto module=f->request.crt;const auto pe=number(f->memory.at(module.base+60,4));
      const auto export_rva=number(f->memory.at(module.base+pe+120,4));
      f->memory.put(f->export_slot(module,1420),export_rva);}
    const auto r=f->capture();CHECK(!r.known&&r.issue==nf::Issue::free_route);
  }
  for(unsigned bad=0;bad<5;++bad){auto f=valid();
    switch(bad){case 0:f->request.current_thread=8;break;case 1:f->request.job.quantity=1;break;
      case 2:f->request.serial=0;break;case 3:f->request.frame=0;break;case 4:f->request.job.context.world=0xfffffffcu;break;}
    const auto before=f->memory.reads;CHECK(!f->capture().known&&f->memory.reads==before);
  }
  for(unsigned mutation=0;mutation<7;++mutation){auto f=valid();f->memory.watch=n2;f->memory.change_on=2;
    switch(mutation){case 0:f->memory.change_address=n2+16;f->memory.change_value=0;break;
      case 1:f->memory.change_address=n2+8;f->memory.change_value=n3;break;
      case 2:f->memory.change_address=string_object+16;f->memory.change_value=16;break;
      case 3:f->memory.change_address=0x82e308;f->memory.change_value=f->request.union_core.base+0x35450;break;
      case 4:f->memory.change_address=0xab642c;f->memory.change_value=3;break;
      case 5:f->memory.change_address=0x83dfe4;f->memory.change_value=0x72e370;break;
      case 6:f->memory.change_address=code+672156;f->memory.change_value=0;break;}
    CHECK(!f->capture().known);
  }
  {auto f=valid();reenter_reader=&f->reader;reenter_request=f->request;f->memory.callback=reenter;
    const auto r=f->capture();CHECK(!r.known&&r.issue==nf::Issue::reentry&&reenter_issue==nf::Issue::reentry);
    CHECK(!f->workspace.busy);++f->request.serial;CHECK(f->capture().known);}
}
} // namespace
int main(int argc,char** argv){
  if(argc==1){portable();std::printf("NoFX portable: %u checks, %u failures\n",checks,failures);return failures?1:0;}
  if(argc!=5){std::fprintf(stderr,"usage: %s [EXE SHW CRT END_SCRIPT_49_BYTES]\n",argv[0]);return 2;}
  const PE engine(argv[1],nf::profile::engine_sha256),core(argv[2],ubc::zbug_end_compat::union_file_sha256),crt(argv[3],d::crt_sha256);
  std::ifstream f(argv[4],std::ios::binary);std::vector<std::uint8_t> script(49);
  if(!f.read(reinterpret_cast<char*>(script.data()),49)||f.peek()!=std::char_traits<char>::eof()||
     d::detail::digest(script.data(),script.size())!=ubc::cool_recipe_catalog::functions[14].body_sha256)return 2;
  run(engine,core,crt,script);std::printf("NoFX reader: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
