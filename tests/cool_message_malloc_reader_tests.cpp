// SPDX-License-Identifier: MIT
#include "ubc/stations/cool/cool_message_malloc_reader.hpp"
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
namespace mm=ubc::cool_message_malloc;
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
constexpr std::uint32_t game=0x2000000,world=0x2010000,hero=0x2020000,mob=0x2030000;
mm::Request request(std::uint32_t crt_base=0x68000000) {
  mm::Request q;q.job={{23,7,ubc::lhiver::game_parser,game,world,hero,mob},91,92,93,20775,20773,2};
  q.frame=100;q.serial=1;q.current_thread=7;
  q.engine={0x400000,0x752000,mm::profile::engine_sha256,true};
  q.union_core={0x60000000,0x438000,ubc::zbug_end_compat::union_file_sha256,true};
  q.crt={crt_base,0xbe000,d::crt_sha256,true};return q;
}
struct Fixture {
  Memory memory;mm::Request q;mm::Reader reader{7};
  Fixture(const PE& engine,const PE& core,const PE& crt,std::uint32_t crt_base=0x68000000):q(request(crt_base)) {
    engine.map(memory,q.engine.base,q.engine.image_size);core.map(memory,q.union_core.base,q.union_core.image_size);
    crt.map(memory,q.crt.base,q.crt.image_size);
    memory.put(0x82e30c,q.union_core.base+0x35450);
    memory.put(q.union_core.base+0x812e8,q.crt.base+0x10233);
    memory.put(q.crt.base+0xb38b0,0x50010000);memory.put(q.crt.base+0xb45f4,0);
    // Only an observed IAT value. Do not invent OS target verification.
    memory.put(q.crt.base+0x11f8,0x77001234);
    memory.block(game,12);memory.put(game,0x83c28c);memory.put(game+8,world);
    memory.block(world,4);memory.put(world,0x83e174);
    memory.block(hero,0x968);memory.put(hero+0xb8,world);memory.put(hero+0x964,mob);memory.block(mob,4);
    memory.put(ubc::selection_abi::game_pointer,game);memory.put(ubc::station_abi::hero_pointer,hero);
    memory.put(ubc::alchemy_flask_runtime::current_parser_pointer,ubc::lhiver::game_parser);
  }
  mm::Capture capture() {
    const auto allocations=allocation_probe::count;allocation_probe::watching=true;
    const auto out=reader.capture(memory.view(),q);allocation_probe::watching=false;
    CHECK(allocation_probe::count==allocations);return out;
  }
  std::uint32_t export_directory(const mm::Module& module) {
    const auto pe=number(memory.at(module.base+60,4));return module.base+number(memory.at(module.base+pe+120,4));
  }
  std::uint32_t export_slot(const mm::Module& module,std::uint32_t ordinal) {
    const auto exp=export_directory(module);
    return module.base+number(memory.at(exp+28,4))+(ordinal-number(memory.at(exp+16,4)))*4;
  }
};
mm::Reader* nested_reader=nullptr;mm::Request nested_request{};mm::Issue nested_issue{};
mm::Request* mutable_request=nullptr;
void nested(Memory& m) noexcept {nested_issue=nested_reader->capture(m.view(),nested_request).issue;}
void change_request(Memory&) noexcept {++mutable_request->job.writer_token;mutable_request->crt.base+=0x10000;}

void portable() {
  CHECK(!mm::runtime_ready&&!mm::profile::runtime_ready&&!mm::Capture::native_effect_authorized&&!mm::Capture::excludes_dispatch);
  CHECK(!mm::Capture{}.known&&!mm::Capture{}.direct_newhandler_edges_unreachable_current);
  CHECK(mm::profile::allocation_site+5==mm::profile::call&&mm::profile::call+5==mm::profile::return_address);
  CHECK(mm::profile::size==152&&mm::profile::size<=mm::profile::maximum_size);
  std::size_t bytes=0,relocs=0;
  for(const auto& r:mm::profile::malloc_code){
    CHECK(r.length&&r.length<=512&&r.rva+r.length<=mm::profile::crt_size);
    for(std::size_t i=0;i<r.relocation_count;++i)CHECK(r.relocations[i]+4u<=r.length&&(!i||r.relocations[i]>=r.relocations[i-1]+4u));
    bytes+=r.length;relocs+=r.relocation_count;
  }
  CHECK(bytes==173&&relocs==4&&mm::profile::preferred_base==0x78aa0000);
  for(unsigned fault=0;fault<17;++fault){Memory m;mm::Reader reader(7);auto q=request();
    switch(fault){case 0:q.current_thread=8;break;case 1:q.job.context.thread=8;break;
      case 2:q.job.context.epoch=0;break;case 3:q.job.id=0;break;case 4:q.job.writer_token=0;break;
      case 5:q.job.fence_token=0;break;case 6:q.job.callback=20778;break;case 7:q.job.owner=20776;break;
      case 8:q.job.quantity=1;break;case 9:q.frame=0;break;case 10:q.serial=0;break;
      case 11:q.job.context.hero=world;break;case 12:q.job.context.mob|=1;break;
      case 13:q.job.context.parser=0;break;case 14:q.job.context.hero=0xfffffffc;break;
      case 15:q.job.context.game=0;break;case 16:q.job.context.world=0;break;}
    const auto c=reader.capture(m.view(),q);
    CHECK(!c.known&&!c.direct_newhandler_edges_unreachable_current&&m.reads==0);
    CHECK(c.issue==(fault<2?mm::Issue::thread:mm::Issue::request));
  }
  for(unsigned fault=0;fault<10;++fault){Memory m;mm::Reader reader(7);auto q=request();
    switch(fault){case 0:q.engine.mapping_verified=false;break;case 1:q.union_core.mapping_verified=false;break;
      case 2:q.crt.mapping_verified=false;break;case 3:q.engine.file_sha256[0]^=1;break;
      case 4:q.union_core.file_sha256[0]^=1;break;case 5:q.crt.file_sha256[0]^=1;break;
      case 6:q.crt.image_size-=1;break;case 7:q.crt.base=q.union_core.base;break;
      case 8:q.crt.base=0x400000;break;case 9:q.crt.base=0xffff0000;break;}
    const auto c=reader.capture(m.view(),q);CHECK(c.issue==mm::Issue::module&&!c.known&&m.reads==0);
  }
  Memory m;mm::Reader reader(7);auto q=request();const auto c=reader.capture(m.view(),q);
  CHECK(c.issue==mm::Issue::module&&!c.known&&!mm::matches(c,q));
  CHECK(reader.capture(m.view(),q).issue==mm::Issue::serial);
  CHECK(sizeof(mm::Capture)<1024&&sizeof(mm::Reader)<=32);
}

void files(const PE& engine,const PE& core,const PE& crt) {
  for(auto base:{0x68000000u,0x78aa0000u,0x7b000000u}){Fixture f(engine,core,crt,base);const auto c=f.capture();
    CHECK(c.known&&c.direct_newhandler_edges_unreachable_current&&c.issue==mm::Issue::none&&mm::matches(c,f.q));
    CHECK(c.values.allocation_size==152&&c.values.heap_handle==0x50010000&&!c.values.new_mode&&c.values.heap_alloc==0x77001234);
    CHECK(c.values.allocator_entry==0x60035450&&c.values.allocator_forward==base+0x10233);
    CHECK(!c.native_effect_authorized&&!c.excludes_dispatch&&c.fingerprint!=ubc::lhiver::Digest{});
    CHECK(f.reader.capture(f.memory.view(),f.q).issue==mm::Issue::serial);
    for(unsigned fault=0;fault<11;++fault){auto q=f.q;
      switch(fault){case 0:++q.job.context.epoch;break;case 1:++q.job.id;break;case 2:++q.job.writer_token;break;
        case 3:++q.job.fence_token;break;case 4:++q.frame;break;case 5:++q.serial;break;
        case 6:++q.current_thread;break;case 7:q.job.callback=20778;break;
        case 8:q.job.context.hero+=4;break;case 9:q.crt.base+=0x1000;break;case 10:q.crt.mapping_verified=false;break;}
      CHECK(!mm::matches(c,q));
    }
  }
  {Fixture f(engine,core,crt);
    for(const auto& range:mm::profile::engine_code)for(auto off:{0u,range.length/2,range.length-1}){
      auto* byte=f.memory.at(range.address+off,1);*byte^=1;++f.q.serial;
      const auto c=f.capture();CHECK(c.issue==mm::Issue::code&&!c.known);*byte^=1;
    }
    // Every byte of all three actual malloc blocks must remain bound,
    // including cold error paths and all four relocation fields.
    for(const auto& range:mm::profile::malloc_code)for(std::uint32_t off=0;off<range.length;++off){
      auto* byte=f.memory.at(f.q.crt.base+range.rva+off,1);*byte^=1;++f.q.serial;
      const auto c=f.capture();CHECK(c.issue==mm::Issue::code&&!c.known);*byte^=1;
    }
    for(unsigned off=0;off<10;++off){auto* byte=f.memory.at(f.q.union_core.base+0x35450+off,1);*byte^=1;++f.q.serial;
      const auto c=f.capture();CHECK(c.issue==mm::Issue::code&&!c.known);*byte^=1;}
  }
  for(unsigned fault=0;fault<13;++fault){Fixture f(engine,core,crt);const auto cbase=f.q.crt.base;
    switch(fault){case 0:f.memory.put(cbase+0xb38b0,0);break;case 1:f.memory.put(cbase+0xb45f4,1);break;
      case 2:f.memory.put(cbase+0xb45f4,0xffffffff);break;case 3:f.memory.put(cbase+0x11f8,0);break;
      case 4:f.memory.put(0x82e30c,f.q.union_core.base+0x35460);break;
      case 5:f.memory.put(f.q.union_core.base+0x812e8,cbase+0x1014e);break;
      case 6:f.memory.put(f.export_slot(f.q.union_core,258),0x35460);break;
      case 7:f.memory.put(f.export_slot(f.q.crt,1477),0x1014e);break;
      case 8:f.memory.put(hero+0x964,0);break;case 9:f.memory.put(hero+0xb8,world+4);break;
      case 10:f.memory.put(game+8,world+4);break;case 11:f.memory.put(ubc::station_abi::hero_pointer,hero+4);break;
      case 12:f.memory.put(ubc::alchemy_flask_runtime::current_parser_pointer,0);break;}
    const auto c=f.capture();CHECK(!c.known&&!c.direct_newhandler_edges_unreachable_current&&c.issue!=mm::Issue::none);
    if(fault==0)CHECK(c.issue==mm::Issue::heap);
    if(fault==1||fault==2)CHECK(c.issue==mm::Issue::new_mode);
    if(fault==3)CHECK(c.issue==mm::Issue::heap_alloc);
  }
  for(auto off:{0xb38b0u,0xb45f4u,0x11f8u,0x10233u,0x102bcu,0x3f22fu}){Fixture f(engine,core,crt);
    f.memory.deny=f.q.crt.base+off;const auto c=f.capture();CHECK(!c.known&&c.issue==mm::Issue::unreadable);
  }
  // Between full snapshots, change values to others of the same kind that
  // are still individually valid. The combined comparison must reject them.
  for(unsigned fault=0;fault<3;++fault){Fixture f(engine,core,crt);
    if(fault<2){const auto off=fault==0?0xb38b0u:0x11f8u;
      f.memory.watch=f.q.crt.base+off;f.memory.change_on=2;f.memory.change_address=f.memory.watch;
      f.memory.change_value=fault==0?0x50020000:0x77005678;
    }else{const auto pe=number(f.memory.at(f.q.crt.base+60,4));
      f.memory.watch=f.q.crt.base+pe;f.memory.change_on=3;f.memory.change_address=f.memory.watch+52;f.memory.change_value=f.q.crt.base;}
    const auto c=f.capture();CHECK(!c.known&&c.issue==mm::Issue::changed);
  }
  {Fixture f(engine,core,crt);const auto first=f.capture();++f.q.serial;
    f.memory.put(f.q.crt.base+0x11f8,0x12345678);const auto second=f.capture();
    CHECK(second.known&&second.values.heap_alloc==0x12345678&&second.fingerprint!=first.fingerprint);
    CHECK(!second.native_effect_authorized&&!second.excludes_dispatch); // Do not invent evidence for the OS target.
  }
  {Fixture f(engine,core,crt);nested_reader=&f.reader;nested_request=f.q;f.memory.callback=nested;
    const auto c=f.capture();CHECK(nested_issue==mm::Issue::reentry&&!c.known&&c.issue==mm::Issue::reentry);
    ++f.q.serial;CHECK(f.capture().known);
  }
  {Fixture f(engine,core,crt);const auto original=f.q;mutable_request=&f.q;f.memory.callback=change_request;
    const auto c=f.capture();CHECK(c.known&&mm::matches(c,original)&&!mm::matches(c,f.q));
    CHECK(c.request.job.writer_token==original.job.writer_token&&c.request.crt.base==original.crt.base);
  }
}
} // namespace
int main(int argc,char** argv) {
  portable();
  if(argc==4){PE engine(argv[1],mm::profile::engine_sha256),core(argv[2],ubc::zbug_end_compat::union_file_sha256),crt(argv[3],d::crt_sha256);files(engine,core,crt);}
  else if(argc!=1){std::fprintf(stderr,"usage: %s [EXE UNION_CORE CRT]\n",argv[0]);return 2;}
  std::printf("cool_message_malloc_reader_tests: %u checks, %u failures; file_facts=%s\n",checks,failures,argc==4?"yes":"no");
  return failures?1:0;
}
