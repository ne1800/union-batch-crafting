// SPDX-License-Identifier: MIT
#define UBC_BOW_ITEM_NATIVE_FIXTURE_MAIN bow_native_fixture_main
#include "bow_item_native_tests.cpp"

namespace vob_insert_test {
namespace vi=ubc::vob_inventory_insert_compat;
using namespace bow_item_test;
constexpr std::uint32_t locator_base=0x6c000000,record=0x63000000;
// Reusable test RAM seam. PE::load checks the file hash and copies actual
// sections and relocations. Runtime fields are explicit fixtures. They
// do not establish an actually loaded DLL or native leases of our own.
inline vi::Binding install(Image& image,const PE& locator,const vi::Module& core,
    std::uint32_t base=locator_base,std::uint32_t object=record){
  locator.load(image,base);
  const vi::Binding b{{base,vi::locator_profile.image_size,vi::locator_profile.file_sha256,true},core};
  image.put(vi::slot_address,base+vi::wrapper_rva);
  image.put(base+vi::record_rva,object);image.put(base+vi::resolver_import_rva,core.base+vi::resolver.rva);
  image.block(object,28);image.put(object+0xc,0);image.put(object+0x14,vi::original_target);
  image.put(object+0x18,base+vi::wrapper_rva);return b;
}
struct Switch {
  Image& image;unsigned count=0,at=0;std::uint32_t target=0,value=0;
  static bool read(void* raw,std::uint32_t address,void* out,std::size_t size) noexcept {
    auto& s=*static_cast<Switch*>(raw);
    if(address==vi::slot_address&&++s.count==s.at)s.image.put(s.target,s.value);
    return Image::read(&s.image,address,out,size);
  }
  ubc::ReadOnlyMemory memory(){return {this,read};}
};
void defaults(){
  Image image;check(!vi::verify(image.memory()).valid,"empty memory cannot bind insert");
  image.block(vi::slot_address,4);image.put(vi::slot_address,vi::original_target);
  check(!vi::verify(image.memory()).valid,"slot alone cannot bind original code");
  image.put(vi::slot_address,locator_base+vi::wrapper_rva);
  check(!vi::verify(image.memory()).valid,"unbound locator never accepts a foreign slot");
  check(!vi::same({},{}),"empty evidence is not equality evidence");
}
void facts(const char* engine_path,const char* locator_path,const char* zbug_path,const char* core_path){
  const PE engine(engine_path,ubc::station_abi::executable_sha256),locator(locator_path,vi::locator_profile.file_sha256),
    zbug(zbug_path,ubc::zbug_end_compat::zbug_file_sha256),core(core_path,vi::union_profile.file_sha256);
  ModuleFixture f(engine,zbug,core);
  const auto original=vi::verify(f.image.memory());
  check(original.valid&&original.route==vi::Route::original,"complete original code permits original slot without locator");
  check(n::verify_routed_code(f.image.memory(),f.modules).verified,"existing strict native route remains positive on original slot");
  const vi::Binding absent{};vi::Verification evidence;
  check(n::verify_routed_code(f.image.memory(),f.modules,absent,&evidence).verified&&vi::same(original,evidence),
    "explicit absent locator still permits original and returns its evidence");
  const auto binding=install(f.image,locator,f.modules.union_core);
  const auto live=vi::verify(f.image.memory(),&binding);
  check(live.valid&&live.route==vi::Route::vob_direct&&live.record==record,"full relocated 91-byte wrapper and exact original record pass");
  check(!vi::same(original,live)&&vi::same(live,vi::verify(f.image.memory(),&binding)),"route fingerprint is stable and distinguishes original from locator");
  check(!n::verify_routed_code(f.image.memory(),f.modules).verified,"default native route rejects hooked slot");
  check(n::verify_routed_code(f.image.memory(),f.modules,binding,&evidence).verified&&vi::same(live,evidence),
    "only explicit native overload accepts locator and returns its actual evidence");
  for(const auto& range:std::array<std::pair<std::uint32_t,std::size_t>,3>{{
      {locator_base+vi::wrapper_rva,vi::wrapper.length},{binding.union_core.base+vi::resolver.rva,vi::resolver.length},
      {vi::original_target,vi::original.length}}}){
    for(std::size_t i=0;i<range.second;++i){auto* byte=f.image.at(range.first+static_cast<std::uint32_t>(i),1);*byte^=1;
      check(!vi::verify(f.image.memory(),&binding).valid,"each byte of wrapper, resolver and original is bound");*byte^=1;}
  }
  for(const auto& module:{binding.locator,binding.union_core}){
    for(std::uint32_t offset:{0u,0x88u,1023u}){auto* byte=f.image.at(module.base+offset,1);*byte^=1;
      check(!vi::verify(f.image.memory(),&binding).valid,"complete genuine header rejects mutations");*byte^=1;}
  }
  for(unsigned which=0;which<12;++which){auto b=binding;
    if(which==0)b.locator.mapping_verified=false;if(which==1)b.union_core.mapping_verified=false;
    if(which==2)b.locator.file_sha256[0]^=1;if(which==3)b.union_core.file_sha256[0]^=1;
    if(which==4)--b.locator.image_size;if(which==5)--b.union_core.image_size;
    if(which==6)++b.locator.base;if(which==7)b.locator.base=0xffff0000;
    if(which==8)b.locator.base=0x400000;if(which==9)b.locator.base=b.union_core.base;
    if(which==10)b.locator.base=0;if(which==11)b.union_core.base=0;
    check(!vi::verify(f.image.memory(),&b).valid,"wrong lease, file, extent, alignment, overlap or range rejects");
  }
  const std::array<std::pair<std::uint32_t,std::uint32_t>,10> bad{{
    {vi::slot_address,locator_base+vi::wrapper_rva+1},{locator_base+vi::resolver_import_rva,binding.union_core.base+vi::resolver.rva+1},
    {locator_base+vi::record_rva,0},{locator_base+vi::record_rva,record+1},{locator_base+vi::record_rva,0xfffffff0},
    {locator_base+vi::record_rva,0x400000},{locator_base+vi::record_rva,locator_base},
    {record+0xc,record+64},{record+0x14,vi::original_target+1},{record+0x18,locator_base+vi::wrapper_rva+1}}};
  for(const auto& mutation:bad){const auto saved=u32(f.image.at(mutation.first,4));f.image.put(mutation.first,mutation.second);
    check(!vi::verify(f.image.memory(),&binding).valid,"wrong slot/import/null or chained record rejects");f.image.put(mutation.first,saved);}
  for(const auto address:{vi::slot_address,vi::original_target,locator_base,binding.union_core.base,
      locator_base+vi::wrapper_rva,binding.union_core.base+vi::resolver.rva,locator_base+vi::resolver_import_rva,
      locator_base+vi::record_rva,record}){
    f.image.denied=address;f.image.denied_size=1;check(!vi::verify(f.image.memory(),&binding).valid,"every unavailable capture region rejects");f.image.denied_size=0;
  }
  const auto other=record+64;f.image.block(other,28);std::memcpy(f.image.at(other,28),f.image.at(record,28),28);
  {Switch s{f.image,0,2,locator_base+vi::record_rva,other};const auto result=vi::verify(s.memory(),&binding);
    check(!result.valid&&result.issue==vi::Issue::changed,"two separately valid records cannot change between captures");}
  const auto relocated_record=vi::verify(f.image.memory(),&binding);
  check(relocated_record.valid&&!vi::same(live,relocated_record),"record identity is part of public comparison");
  f.image.put(locator_base+vi::record_rva,record);
  {Switch s{f.image,0,2,vi::slot_address,vi::original_target};const auto result=vi::verify(s.memory(),&binding);
    check(!result.valid&&result.issue==vi::Issue::changed,"two valid route alternatives cannot change between captures");}
  f.image.put(vi::slot_address,locator_base+vi::wrapper_rva);
  {Switch s{f.image,0,3,locator_base+vi::record_rva,other};evidence=live;
    const auto result=n::verify_routed_code(s.memory(),f.modules,binding,&evidence);
    check(!result.verified&&result.issue==n::Issue::changed&&!evidence.valid,"native outer passes compare actual record and clear failed output");}
  f.image.put(locator_base+vi::record_rva,record);
  {Switch s{f.image,0,3,vi::slot_address,vi::original_target};evidence=live;
    const auto result=n::verify_routed_code(s.memory(),f.modules,binding,&evidence);
    check(!result.verified&&result.issue==n::Issue::changed&&!evidence.valid,"native outer passes also reject a route switch");}
  f.image.put(vi::slot_address,locator_base+vi::wrapper_rva);
  for(const auto& slot:ubc::bow_item_code::slots)if(slot.address!=vi::slot_address){
    const auto saved=u32(f.image.at(slot.address,4));f.image.put(slot.address,saved+1);evidence=live;
    check(!n::verify_routed_code(f.image.memory(),f.modules,binding,&evidence).verified&&!evidence.valid,
      "every other native slot remains strict and cannot leak prior evidence");f.image.put(slot.address,saved);
  }
  auto other_core=binding;other_core.union_core.base=0x6d000000;core.load(f.image,other_core.union_core.base);
  f.image.put(locator_base+vi::resolver_import_rva,other_core.union_core.base+vi::resolver.rva);
  check(vi::verify(f.image.memory(),&other_core).valid,"independent valid Union copy is a different concrete binding");
  check(!n::verify_routed_code(f.image.memory(),f.modules,other_core,&evidence).verified&&!evidence.valid,
    "optional native binding must use exactly its existing Union lease");
}
} // namespace vob_insert_test
#ifndef UBC_VOB_INSERT_FIXTURE_ONLY
int main(int argc,char** argv){
  vob_insert_test::defaults();
  if(argc==6&&std::string(argv[1])=="--files")vob_insert_test::facts(argv[2],argv[3],argv[4],argv[5]);
  else if(argc!=1)return 2;
  std::printf("%u checks, %u failures\n",station_test::checks,station_test::failures);return station_test::failures?1:0;
}
#endif
