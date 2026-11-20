// SPDX-License-Identifier: MIT
#include "ubc/cool_destroy_dispatch.hpp"
#include "ubc/cool_native_path.hpp"
#include <cstdio>
#include <map>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
namespace {
namespace d=ubc::cool_destroy_dispatch;
namespace p=ubc::cool_destroy_dispatch_profile;
unsigned checks=0,failures=0;
void check(bool ok,const char* name){++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",name);}}
struct Fixture {
  std::map<std::uint32_t,std::uint8_t> bytes;
  d::Context context{7,41,0x220000,0x200000,0x230000,0x240000,13};
  d::Invocation invocation{0x200000,0x250000,0x74c586,1};
  std::uint32_t denied=0,flip_after=0,flip_target=0;
  bool flipped=false,reentered=false;
  d::Reader* reader=nullptr;d::Evidence nested{};
  void word(std::uint32_t address,std::uint32_t value){for(unsigned i=0;i<4;++i)bytes[address+i]=static_cast<std::uint8_t>(value>>(8u*i));}
  explicit Fixture(const std::vector<std::uint8_t>& code){
    std::size_t at=0;for(const auto& r:p::ranges)for(std::uint32_t i=0;i<r.length;++i)bytes[r.address+i]=code[at++];
    word(0xab2684,context.hero);word(context.hero,p::npc_vtable);word(p::npc_vtable+0x28,p::on_message);
    word(context.hero+0xc8,0x260000);word(context.hero+0x97c,0x270000);word(0x270004,1);
    word(0xab2668,0);word(0x82e8ac,0);
    for(std::size_t i=0;i<p::class_chain.size();++i)word(p::class_chain[i]+0x3c,i+1<p::class_chain.size()?p::class_chain[i+1]:0u);
    word(p::message_vtable,p::class_getter);word(p::message_vtable+0x24,p::message_delete);
    word(context.hero+0x980,0x270000);word(invocation.message,p::message_vtable);word(invocation.message+4,2);
    word(invocation.message+0x24,9);word(invocation.message+0x40,0);
  }
  static bool read(void* raw,std::uint32_t address,void* out,std::size_t size) noexcept {
    auto& f=*static_cast<Fixture*>(raw);
    auto* dest=static_cast<std::uint8_t*>(out);
    for(std::size_t i=0;i<size;++i){
      const auto a=address+static_cast<std::uint32_t>(i);if(a==f.denied)return false;
      const auto found=f.bytes.find(a);if(found==f.bytes.end())return false;dest[i]=found->second;
    }
    if(f.reader&&!f.reentered){f.reentered=true;f.nested=f.reader->capture_before_writer(f.memory(),f.context,18,41,true);}
    if(!f.flipped&&address==f.flip_after){f.flipped=true;f.bytes[f.flip_target]^=1u;}
    return true;
  }
  ubc::ReadOnlyMemory memory() noexcept{return {this,read};}
  d::Evidence writer(d::Reader& r) noexcept{return r.capture_before_writer(memory(),context,18,41,true);}
  d::Evidence destroy(d::Reader& r) noexcept{return r.capture_before_destroy(memory(),context,18,41,true,invocation);}
};
void native_tests(const std::vector<std::uint8_t>& code){
  d::Reader reader(41);
  for(const auto kind:{11,13}){
    Fixture f(code);f.context.production_kind=kind;
    const auto before=f.writer(reader);check(before.path_identity_verified,"writer code and infrastructure");
    check(d::matches(before,f.context,18,d::Scope::before_writer),"writer matching context");
    const auto active=f.destroy(reader);check(active.path_identity_verified,"actual manipulate9");
    check(d::matches(active,f.context,18,d::Scope::before_destroy,f.invocation),"destroy matching invocation");
    check(!d::matches(before,f.context,18,d::Scope::before_destroy,f.invocation),"writer never actual message proof");
    check(!d::matches(active,f.context,18,d::Scope::before_writer),"message scope never writer");
    check(!d::matches(active,f.context,19,d::Scope::before_destroy,f.invocation),"frame bound");
    auto changed=f.context;++changed.generation;
    check(!d::matches(active,changed,18,d::Scope::before_destroy,f.invocation),"generation bound");
    auto call=f.invocation;++call.caller;
    check(!d::matches(active,f.context,18,d::Scope::before_destroy,call),"caller bound");
  }
  // The other modified branches remain unreadable and unused. No
  // byte is normalized or presented as evidence of hook trust.
  {Fixture f(code);const auto old=f.writer(reader);for(const auto address:{0x74c193u,0x74c237u,0x74c7cfu})f.word(address,0xcccccccc);
   const auto now=f.writer(reader);check(now.path_identity_verified&&old.fingerprint==now.fingerprint,"unentered branches excluded");
   check(f.destroy(reader).path_identity_verified,"unentered hooks irrelevant to exact message");
   f.word(p::class_chain[0]+0x3c,0xab2cc0);check(!f.writer(reader).path_identity_verified,"weapon ancestry never excluded");}
  const std::array<std::pair<std::uint32_t,std::uint32_t>,17> bad{{
    {0xab2684,0x200004},{0x200000,0},{p::npc_vtable+0x28,0x5cc5a9},{0x2000c8,0},
    {0x20097c,0},{0x20097c,1},{0x20097c,UINT32_MAX},{0x270004,0},{0x270004,0x80000000},
    {0xab2668,1},{0xab2668,0x80000000},{0xab2668,0x7fc00000},{0x82e8ac,1},
    {p::message_vtable,0x764781},{p::message_vtable+0x24,0x763101},
    {p::class_chain[0]+0x3c,p::class_chain[0]},{p::class_chain[3]+0x3c,p::class_chain[0]}}};
  for(const auto& row:bad){Fixture f(code);f.word(row.first,row.second);const auto e=f.writer(reader);
    check(!e.path_identity_verified&&e.failed_address==row.first,"infrastructure rejects exact address");}
  for(const auto node:p::class_chain){Fixture f(code);f.word(node+0x3c,0xab2cc0);check(!f.writer(reader).path_identity_verified,"every parent bound");}
  const std::array<std::pair<std::uint32_t,std::uint32_t>,8> messages{{
    {0x200980,0},{0x250000,0},{0x250004,0},{0x250004,1},{0x250004,0x80000000},
    {0x250024,8},{0x250024,10},{0x250024,0x10009}}};
  for(std::size_t i=0;i<messages.size();++i){Fixture f(code);f.word(messages[i].first,messages[i].second);
    const auto e=f.destroy(reader);check(e.path_identity_verified==(i==7)&&(i==7||e.failed_address==messages[i].first),"exact unsigned16 subtype and message ref");}
  for(unsigned which=0;which<7;++which){Fixture f(code);
    if(which==0)f.invocation.npc+=4;if(which==1)++f.invocation.caller;if(which==2)f.invocation.depth=2;
    if(which==3)f.invocation.message=0;if(which==4)f.invocation.message=1;
    if(which==5)f.invocation.message=0xfffffffcu;if(which==6)f.invocation.depth=0;
    check(!f.destroy(reader).path_identity_verified,"bad invocation");}
  for(unsigned which=0;which<10;++which){Fixture f(code);
    if(which==0)f.context.generation=0;if(which==1)f.context.thread=0;if(which==2)f.context.world=0;
    if(which==3)f.context.hero=0;if(which==4)f.context.station=0;if(which==5)f.context.manager=0;
    if(which==6)f.context.production_kind=12;if(which==7)f.context.production_kind=0;
    if(which==8)f.context.hero=0xfffffffcu;if(which==9)f.context.hero=1;
    check(!f.writer(reader).path_identity_verified,"bad context");}
  {Fixture f(code);check(!reader.capture_before_writer(f.memory(),f.context,18,42,true).path_identity_verified,"wrong actual thread");
   check(!reader.capture_before_writer(f.memory(),f.context,18,41,false).path_identity_verified,"no boundary");}
  // All 573 bound bytes individually, including the entire jump table.
  for(const auto& r:p::ranges)for(std::uint32_t i=0;i<r.length;++i){Fixture f(code);f.bytes[r.address+i]^=1u;const auto e=f.writer(reader);
    check(!e.path_identity_verified&&e.failed_address==r.address,"each bound code byte rejects mutation");}
  for(const auto a:{0xab2684u,0x20097cu,0x270004u,0xab289cu,0x8d8d14u,0x250024u,0x250040u}){
    Fixture f(code);f.denied=a;check(!f.destroy(reader).path_identity_verified,"unreadable actual field");}
  {Fixture f(code);f.flip_after=p::message_vtable+0x24;f.flip_target=0x2000c8;
   check(f.writer(reader).issue==d::Issue::changed,"valid visual changes between full samples");}
  {Fixture f(code);f.flip_after=0x250024;f.flip_target=0x250040;
   check(f.destroy(reader).issue==d::Issue::changed,"actual message changes between samples");}
  {Fixture f(code);f.reader=&reader;check(f.writer(reader).path_identity_verified&&f.nested.issue==d::Issue::reentry,"reader rejects reentry");}
  {Fixture f(code);f.word(0x250004,INT32_MAX);f.word(0x250040,UINT32_MAX);
   check(f.destroy(reader).path_identity_verified,"positive signed max ref and flag bits");}
}
struct NativeArea {std::uint32_t address=0;std::vector<std::uint8_t> bytes;};
using NativeAreas=std::vector<NativeArea>;
bool read_native_areas(NativeAreas& areas){
  const auto append=[&](std::uint32_t address,std::size_t size){
    NativeArea area{address,std::vector<std::uint8_t>(size)};
    if(std::fread(area.bytes.data(),1,size,stdin)!=size)return false;
    areas.push_back(std::move(area));return true;
  };
  for(const auto& range:ubc::cool_destroy_abi::identity_ranges)if(!append(range.address,range.length))return false;
  for(const auto& range:ubc::alchemy_flask_runtime::code_ranges)if(!append(range.address,range.length))return false;
  for(const auto& entry:ubc::alchemy_flask_runtime::vtable_entries)if(!append(entry.address,4))return false;
  for(const auto address:{p::npc_vtable+0x28,p::message_vtable,p::message_vtable+0x24})if(!append(address,4))return false;
  return true;
}
void native_path_tests(const std::vector<std::uint8_t>& code,const NativeAreas& areas){
  namespace cp=ubc::cool_native_path;
  d::Reader reader(41);
  const auto fixture=[&](){
    Fixture f(code);
    for(const auto& area:areas)for(std::size_t i=0;i<area.bytes.size();++i)f.bytes[area.address+static_cast<std::uint32_t>(i)]=area.bytes[i];
    return f;
  };
  const auto dispatch=[&](Fixture& f,const d::Invocation* invocation=nullptr,bool boundary=true){
    return cp::verify_dispatch(f.memory(),{}, {},reader,f.context,18,41,boundary,invocation);
  };
  for(const auto kind:{11,13}){
    auto f=fixture();f.context.production_kind=kind;
    check(cp::verify(f.memory()).valid,"original complete guard with actual EXE areas");
    check(dispatch(f).valid,"original dispatch overload before writer");
    check(dispatch(f,&f.invocation).valid,"original dispatch overload actual invocation");
    for(const auto address:{0x74c193u,0x74c237u,0x74c7cfu})f.word(address,0xcccccccc);
    const auto strict=cp::verify(f.memory());
    check(!strict.valid&&strict.address==0x74c020,"three unreachable changes still fail strict full guard");
    check(dispatch(f).valid,"unreachable changes pass real writer dispatch reader");
    check(dispatch(f,&f.invocation).valid,"unreachable changes pass exact actual dispatch reader");
    f.bytes[0x74c581]^=1u;const auto relevant=dispatch(f);
    check(!relevant.valid&&relevant.issue==cp::Issue::dispatch&&relevant.address==0x74c57e,"changed actual CALL rejects overload");
  }
  {auto f=fixture();check(!dispatch(f,nullptr,false).valid,"overload never ignores missing boundary");
   auto bad=f.invocation;++bad.caller;check(!dispatch(f,&bad).valid,"overload never ignores wrong invocation");
   f.word(f.invocation.message+4,1);check(!dispatch(f,&f.invocation).valid,"overload actual low ref rejected");
   check(dispatch(f).valid,"writer cannot claim future actual message ref");}
  for(std::size_t i=4;i<ubc::cool_destroy_abi::identity_ranges.size();++i){
    auto f=fixture();const auto& area=ubc::cool_destroy_abi::identity_ranges[i];f.bytes[area.address+area.length/2]^=1u;
    check(!dispatch(f).valid,"all remaining six original identities retained");
  }
  for(const auto& area:ubc::alchemy_flask_runtime::code_ranges){auto f=fixture();f.bytes[area.address+area.length/2]^=1u;
    check(!dispatch(f).valid,"all native original code guards retained");}
  for(const auto& entry:ubc::alchemy_flask_runtime::vtable_entries){auto f=fixture();f.bytes[entry.address]^=1u;
    check(!dispatch(f).valid,"all native original vslots retained");}
  {auto f=fixture();f.flip_after=0x7495f0;f.flip_target=f.context.hero+0xc8;
   const auto changed=dispatch(f);check(!changed.valid&&changed.issue==cp::Issue::changed,"changed valid values around native guards reject");}
}
}
int main(int argc,char** argv){
  check(!d::Evidence{}.path_identity_verified&&!d::Evidence::native_effect_authorized,"defaults closed");
  check(!d::detail::valid(d::Context{}),"empty context");
  const bool full=argc==2&&std::strcmp(argv[1],"--native-stdin")==0;
  if(argc==2&&(std::strcmp(argv[1],"--code-stdin")==0||full)){
#ifdef _WIN32
    if(_setmode(_fileno(stdin),_O_BINARY)==-1)return 2;
#endif
    std::size_t total=0;for(const auto& r:p::ranges)total+=r.length;
    std::vector<std::uint8_t> code(total);
    if(std::fread(code.data(),1,total,stdin)!=total)return 2;
    NativeAreas areas;if(full&&!read_native_areas(areas))return 2;
    if(std::fgetc(stdin)!=EOF)return 2;
    native_tests(code);
    if(full)native_path_tests(code,areas);
  }else if(argc!=1)return 2;
  std::printf("cool_destroy_dispatch_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
