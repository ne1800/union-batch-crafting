// SPDX-License-Identifier: MIT
// Optional pinned local original files, read into RAM only. No game process.
#define main cool_original_fixture_main
#include "alchemy_flask_runtime_tests.cpp"
#undef main
#define UBC_COOL_RUNTIME_FIXTURE_ONLY
#include "cool_recipe_runtime_tests.cpp"
#include "ubc/stations/cool/cool_recipe_plan.hpp"
namespace cool_facts {
namespace catalog=ubc::cool_recipe_catalog;namespace runtime=ubc::cool_recipe_runtime;
struct PatchFixture {
  Image image;std::uint32_t address=0,length=0;
  static bool identity(void* raw,std::uint32_t a,std::size_t n)noexcept{const auto& p=*static_cast<PatchFixture*>(raw);return a==p.address&&n==p.length;}
  static bool writable(void* raw,std::uint32_t a,std::size_t n)noexcept{return static_cast<PatchFixture*>(raw)->image.at(a,n)!=nullptr;}
  static bool replace(void* raw,std::uint32_t a,const void* old,const void* next,std::size_t n)noexcept{auto* p=static_cast<PatchFixture*>(raw)->image.at(a,n);if(!p||std::memcmp(p,old,n))return false;std::memcpy(p,next,n);return true;}
};
void run(const LocalFacts& facts){
  cool_test::Fixture f;
  for(const auto& fn:catalog::functions){CHECK(hash(facts.bytecode.data()+fn.byte_offset,fn.byte_length)==fn.body_sha256);std::memcpy(f.image.at(cool_test::code+fn.byte_offset,fn.byte_length),facts.bytecode.data()+fn.byte_offset,fn.byte_length);}
  const std::array<const char*,5> texts{{"$ITEMREADY","Klinge abgek\xfc""hlt!","$ITEMREADY","3 Klingen abgek\xfc""hlt!","FONT_OLD_10_WHITE.TGA"}};
  for(std::size_t i=0;i<texts.size();++i){const auto& t=catalog::texts[i];const auto p=0x70000000+static_cast<std::uint32_t>(i)*0x100;
    f.make(t.symbol,t.symbol==964?"FONT_SCREENSMALL":"anonymous",1,3,1,p,0,-1);f.image.block(p,20);f.image.block(p+32,t.length+1);f.image.put(p+8,p+32);f.image.put(p+12,t.length);f.image.put(p+16,t.length+1);CHECK(std::strlen(texts[i])==t.length);std::memcpy(f.image.at(p+32,t.length+1),texts[i],t.length+1);CHECK(hash(texts[i],t.length)==t.sha256);}
  CHECK(runtime::verify_recipe(f.image.memory(),ubc::lhiver::game_parser,catalog::recipe).valid);
  // A complete positive image of real pinned code rejects every body's mutation.
  for(const auto& fn:catalog::functions){auto* b=f.image.at(cool_test::code+fn.byte_offset,1);*b^=1;CHECK(!runtime::verify_recipe(f.image.memory(),ubc::lhiver::game_parser,catalog::recipe).valid);*b^=1;}
  for(std::size_t i=0;i<texts.size();++i){auto* b=f.image.at(0x70000020+static_cast<std::uint32_t>(i)*0x100,1);*b^=1;CHECK(!runtime::verify_recipe(f.image.memory(),ubc::lhiver::game_parser,catalog::recipe).valid);*b^=1;}
  f.image.change_address=0x70000000;f.image.change_target=0x7000000c;f.image.change_on_read=2;CHECK(!runtime::verify_recipe(f.image.memory(),ubc::lhiver::game_parser,catalog::recipe).valid);
  for(const auto hot:{2,3,7,INT32_MAX})for(const auto n:{1u,2u,3u,7u,static_cast<unsigned>(INT32_MAX)}){
    const ubc::cool_recipe::Evidence e{true,{{{16612,hot,true,true},{16613,0,true,true}}}};const auto p=ubc::cool_recipe::plan(catalog::recipe,e,n);if(!p.valid)continue;
    const auto* fn=p.callback==20775?catalog::single.information:catalog::triple.information;PatchFixture fixture;fixture.address=code+fn->byte_offset;fixture.length=fn->byte_length;fixture.image.block(fixture.address,fn->byte_length);std::memcpy(fixture.image.at(fixture.address,fn->byte_length),facts.bytecode.data()+fn->byte_offset,fn->byte_length);
    ubc::AmountMemory m{fixture.image.memory(),&fixture,PatchFixture::identity,PatchFixture::writable,PatchFixture::replace};ubc::AmountPatch patch;CHECK(patch.begin(m,fixture.address,p.body(),n));
    for(std::size_t i=0;i<p.count;++i)CHECK(u32(fixture.image.at(fixture.address+p.operands[i].offset,4))==n-p.operands[i].reserved);
    CHECK(patch.finish()&&!patch.active()&&hash(fixture.image.at(fixture.address,fn->byte_length),fn->byte_length)==fn->body_sha256);
  }
}
}
int main(int argc,char** argv){
  if(argc==4&&std::strcmp(argv[1],"--local-facts")==0){LocalFacts facts(argv[2],argv[3]);cool_facts::run(facts);}
  else if(argc!=1)return 2;
  std::printf("cool_recipe_facts_tests: %u checks, %u failures%s\n",checks,failures,argc==1?" (local facts not requested)":"");return failures?1:0;
}
