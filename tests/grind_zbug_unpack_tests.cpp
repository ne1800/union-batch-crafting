// SPDX-License-Identifier: MIT
#include "ubc/grind_zbug_unpack.hpp"
#include "bow_item_test_values.hpp"

namespace {
using namespace bow_item_test;
namespace g=ubc::grind_zbug_unpack;
constexpr std::uint32_t hero=0x2000,empty=0x50000;
ubc::GrindRecipeKey key(){return {1,8,7,0x8000,hero,0x8800,0x9000,11};}
g::Binding binding(){return {{0x74fe0000,g::z::zbug_image_size,g::z::zbug_file_sha256,true},
  {0x69000000,g::z::union_image_size,g::z::union_file_sha256,true}};}
struct Fixture {
  Image image;g::Binding modules=binding();
  Fixture(const PE& zbug,const PE& core,bool patched=true,bool pointer=true){
    zbug.load(image,modules.zbug.base);core.load(image,modules.union_core.base);
    image.block(hero,0x1000);put(image,ubc::station_abi::hero_pointer,hero);
    image.put(hero+g::inventory_offset,g::inventory_vtable);image.put(hero+ubc::packed_detail::header_offset,hero);
    image.put(hero+0x70c,1);image.put(hero+0x724,pointer?empty:0);image.put(hero+0x728,0);
    text(image,empty,"");text(image,empty+16,"");image.put(modules.zbug.base+g::cookie_rva,0xa17439b5);
    if(patched)image.put(modules.zbug.base+g::thunk_rva+11,modules.zbug.base+g::wrapper_rva);
  }
  g::Verification capture(){return g::verify(image.memory(),modules,key());}
};
struct Change {
  Image& image;std::uint32_t watched,address,value;unsigned at=2,seen=0;
  static bool read(void* raw,std::uint32_t address,void* out,std::size_t count) noexcept {
    auto& s=*static_cast<Change*>(raw);if(address==s.watched&&++s.seen==s.at)s.image.put(s.address,s.value);
    return Image::read(&s.image,address,out,count);
  }
};
void synthetic(){
  Image image;check(!g::verify(image.memory(),{},key()).valid,"missing module leases cannot authorize a thunk");
  const auto baseline=image.reads;
  for(unsigned fault=0;fault<12;++fault){auto b=binding();
    switch(fault){case 0:b.zbug.mapping_verified=false;break;case 1:b.union_core.mapping_verified=false;break;
      case 2:b.zbug.file_sha256[0]^=1;break;case 3:b.union_core.file_sha256[0]^=1;break;
      case 4:--b.zbug.image_size;break;case 5:--b.union_core.image_size;break;
      case 6:b.zbug.base=0;break;case 7:b.union_core.base=0;break;
      case 8:b.zbug.base=0x400000;break;case 9:b.union_core.base=b.zbug.base;break;
      case 10:b.zbug.base=UINT32_MAX-8;break;case 11:b.union_core.base=UINT32_MAX-8;break;}
    check(!g::verify(image.memory(),b,key()).valid&&image.reads==baseline,"bad identity/range rejects before reading");}
  auto bad=key();bad.production_kind=16;check(!g::verify(image.memory(),binding(),bad).valid&&image.reads==baseline,"Bow key cannot acquire Grind alternative");
  bad=key();bad.hero=UINT32_MAX-4;check(!g::verify(image.memory(),binding(),bad).valid&&image.reads==baseline,"hero end range checked before arithmetic");
  check(!g::Verification::native_effect_authorized&&!g::same({},{}),"no native effect or equality from empty evidence");
}
void facts(const char* zbug_path,const char* core_path){
  const PE zbug(zbug_path,g::z::zbug_file_sha256),core(core_path,g::z::union_file_sha256);
  for(const bool patched:{false,true})for(const bool pointer:{false,true}){Fixture f(zbug,core,patched,pointer);
    const auto e=f.capture();check(e.valid&&g::matches(e,key(),f.modules)&&e.nul_read==pointer&&
      e.route==(patched?g::Route::empty_zbug:g::Route::original),"actual pinned full bytes admit exactly original or empty ZBug route");
    if(patched)check(e.thunk_sha256==ubc::lhiver::digest_literal("72d5fb57dd7845ff6e71087dbf03782b3b9c782d1d6f2b0aa83da81bb5e0c338"),
      "own RAM thunk equals all23 bytes of the two stable Grind26 runtime copies");
    check(g::same(e,f.capture()),"same actual values bind both complete observations");
    for(unsigned field=0;field<10;++field){auto bad=e;switch(field){case 0:bad.valid=false;break;case 1:bad.key.frame++;break;
      case 2:bad.binding.zbug.base+=0x10000;break;case 3:bad.header[32]=1;break;case 4:bad.nul_read=!bad.nul_read;break;
      case 5:bad.cookie=0;break;case 6:bad.thunk_target^=1;break;case 7:bad.fingerprint[0]^=1;break;
      case 8:bad.normalized_thunk_sha256[0]^=1;break;case 9:bad.thunk_sha256[0]^=1;break;}
      check(!g::matches(bad,key(),f.modules)&&!g::same(e,bad),"mixed key/header/code/module evidence rejects");}}
  {Fixture f(zbug,core);const auto before=f.capture();f.image.put(hero+0x724,empty+16);const auto after=f.capture();
    check(before.valid&&after.valid&&!g::same(before,after),"different valid NUL pointers are not the same observation");
    f.image.put(f.modules.zbug.base+g::thunk_rva+11,g::original_target);const auto original=f.capture();
    check(original.valid&&!g::same(after,original),"different valid routes are not the same observation");}
  for(unsigned byte=0;byte<g::thunk.length;++byte){Fixture f(zbug,core);*f.image.at(f.modules.zbug.base+g::thunk.rva+byte,1)^=1;
    check(!f.capture().valid,"every byte of complete23 including MOV operand and tail remains covered");}
  for(const auto& c:g::code)for(unsigned byte=0;byte<c.length;++byte){Fixture f(zbug,core);*f.image.at(f.modules.zbug.base+c.rva+byte,1)^=1;
    check(!f.capture().valid,"every byte of full747 wrapper and14 cookie remains covered");}
  for(unsigned fault=0;fault<11;++fault){Fixture f(zbug,core);const auto z=f.modules.zbug.base;
    switch(fault){case 0:f.image.put(ubc::station_abi::hero_pointer,hero+4);break;
      case 1:f.image.put(hero+g::inventory_offset,g::inventory_vtable+4);break;
      case 2:f.image.put(hero+ubc::packed_detail::header_offset,hero+4);break;
      case 3:f.image.put(hero+0x728,1);break;case 4:f.image.put(hero+0x728,UINT32_MAX);break;
      case 5:*f.image.at(empty,1)=1;break;case 6:f.image.put(z+g::cookie_rva,0);break;
      case 7:f.image.put(z+g::thunk_rva+11,z+g::wrapper_rva+1);break;
      case 8:f.image.put(z+g::thunk_rva+11,0x70000000+g::wrapper_rva);break;
      case 9:f.modules.zbug.base++;break;case 10:*f.image.at(f.modules.union_core.base,1)^=1;break;}
    check(!f.capture().valid,"wrong hero, inventory, length, NUL, cookie, target or PE rejects");}
  const auto b=binding();
  for(const auto address:{ubc::station_abi::hero_pointer,hero+g::inventory_offset,hero+ubc::packed_detail::header_offset,
      empty,b.zbug.base+g::cookie_rva,b.zbug.base+g::thunk_rva,b.zbug.base+g::wrapper_rva,
      b.zbug.base+g::wrapper_rva+512,b.zbug.base+0x2297a,b.zbug.base,b.union_core.base}){
    Fixture f(zbug,core);f.image.denied=address;f.image.denied_size=1;check(!f.capture().valid,"each required actual memory region must be readable");}
  for(unsigned fault=0;fault<6;++fault){Fixture f(zbug,core);const auto z=f.modules.zbug.base;
    std::uint32_t changed=hero+0x724,value=empty+16,watch=hero+ubc::packed_detail::header_offset;
    if(fault==1){changed=hero+0x70c;value=0;}
    if(fault==2){changed=z+g::cookie_rva;value=0x12345678;}
    if(fault==3){changed=z+g::thunk_rva+11;value=g::original_target;}
    if(fault==4){changed=hero+0x72c;value=123;}
    if(fault==5){watch=z+g::wrapper_rva;changed=z+g::wrapper_rva;value=0;}
    Change change{f.image,watch,changed,value};const auto e=g::verify({&change,Change::read},f.modules,key());
    check(!e.valid&&e.issue==g::Issue::changed,"changed whole observation or second code copy cannot form evidence");}
}
}
int main(int argc,char** argv){synthetic();if(argc==4&&std::string(argv[1])=="--local-files")facts(argv[2],argv[3]);else if(argc!=1)return 2;
  std::printf("grind_zbug_unpack_tests: %u checks, %u failures\n",station_test::checks,station_test::failures);return station_test::failures?1:0;}
