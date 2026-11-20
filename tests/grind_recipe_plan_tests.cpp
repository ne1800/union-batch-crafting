// SPDX-License-Identifier: MIT
// Optional original file facts in RAM only. No game access.
#define main grind_fixture_main
#include "alchemy_flask_runtime_tests.cpp"
#undef main
#include "ubc/grind_recipe_plan.hpp"

namespace grind_test {
namespace g=ubc::grind_recipe;
g::Evidence evidence(const g::Descriptor& d,std::int32_t skill=1){
  g::Evidence e;e.callback=d.function.symbol;e.stable_boundary_verified=true;
  e.talent_known=e.guild_known=e.skill_known=true;e.talent=1;e.guild=d.guilds[0];e.skill=skill;
  e.stocks={g::Stock{d.input,100,true,true},g::Stock{d.output,3,true,true},g::Stock{16613,4,true,true}};
  return e;
}
void arithmetic(){
  CHECK(g::recipes.size()==22);CHECK(!g::runtime_authorized);
  for(const auto& d:g::recipes){
    CHECK(g::find(d.function.symbol)==&d&&g::structurally_valid(d));
    for(const auto guild:d.guilds)for(const auto skill:{-10,0,1,49,98,99,100,101,INT32_MAX})
      for(const auto n:{1u,2u,5u,50u,100u}){
        auto e=evidence(d,skill);e.guild=guild;const auto p=g::plan(d,e,n);
        CHECK(p.known&&p.learned&&p.guild_allowed&&p.valid&&p.maximum==100&&p.quantity==n);
        auto sequential=skill;
        for(std::uint32_t i=0;i<n;++i)if(sequential<100)++sequential;
        CHECK(p.skill_after==sequential&&p.skill_before==skill);
        CHECK(p.after_callback[0].amount==100-n&&p.after_callback[1].amount==3+n&&p.after_callback[2].amount==5);
        CHECK(p.after_end[0].amount==100-n&&p.after_end[1].amount==3+n&&p.after_end[2].amount==4);
        CHECK(p.count==(skill<100?4u:3u));
        for(std::size_t i=0;i<p.count;++i){const auto& op=p.operands[i];
          CHECK(op.offset!=151&&op.offset!=293&&op.original==1&&!op.allow_zero);
          const auto replacement=std::uint64_t{op.units}*n-op.reserved;
          CHECK(replacement==(i==3?static_cast<std::uint32_t>(sequential-skill):n));
        }
        const auto copy=p;CHECK(copy.body().operands==copy.operands.data()&&copy.body().operands!=p.operands.data());
      }
    auto e=evidence(d);e.stocks[0].amount=0;const auto zero=g::plan(d,e,0);
    CHECK(zero.known&&zero.maximum==0&&!zero.valid);
    e=evidence(d);e.stocks[1].amount=INT32_MAX-2;CHECK(g::plan(d,e,2).valid&&g::plan(d,e,2).maximum==2);
    CHECK(!g::plan(d,e,3).valid);
    e.stocks[2].amount=INT32_MAX;CHECK(!g::plan(d,e,1).valid&&g::plan(d,e,1).maximum==0);
    e=evidence(d,INT32_MIN);e.stocks[0].amount=INT32_MAX;e.stocks[1].amount=0;
    const auto edge=g::plan(d,e,INT32_MAX);CHECK(edge.valid&&edge.skill_after==-1&&edge.skill_increment==INT32_MAX);
    e=evidence(d);std::swap(e.stocks[0],e.stocks[2]);CHECK(g::plan(d,e,2).valid); // Match by symbol, never by incidental row position.
    for(unsigned mode=0;mode<15;++mode){auto bad=evidence(d);
      switch(mode){case 0:bad.callback++;break;case 1:bad.stable_boundary_verified=false;break;
        case 2:bad.talent_known=false;break;case 3:bad.guild_known=false;break;case 4:bad.skill_known=false;break;
        case 5:bad.talent=0;break;case 6:bad.talent=2;break;case 7:bad.guild=0;break;
        case 8:bad.stocks[0].known=false;break;case 9:bad.stocks[0].unambiguous=false;break;
        case 10:bad.stocks[0].amount=-1;break;case 11:bad.stocks[2].amount=std::int64_t{INT32_MAX}+1;break;
        case 12:bad.stocks[1].symbol=d.input;break;case 13:bad.stocks[2].symbol=16612;break;case 14:bad.stocks[2].known=false;break;}
      CHECK(!g::plan(d,bad,1).valid);
    }
    e=evidence(d);CHECK(!g::plan(d,e,0).valid&&g::plan(d,e,0).known);
    CHECK(!g::plan(d,e,UINT32_MAX).valid);
    for(unsigned mode=0;mode<10;++mode){auto bad=d;
      switch(mode){case 0:bad.function.symbol++;break;case 1:bad.input++;break;case 2:bad.output++;break;
        case 3:bad.guilds[0]=0;break;case 4:bad.category++;break;case 5:bad.function.byte_offset++;break;
        case 6:bad.function.byte_length--;break;case 7:bad.function.body_sha256[0]^=1;break;
        case 8:bad.function.argc=1;break;case 9:bad.function.name="foreign";break;}
      CHECK(!g::structurally_valid(bad)&&!g::plan(bad,e,1).valid);
    }
  }
  CHECK(!g::find(20940)&&!g::find(20947)&&!g::find(20987));
}
struct PatchFixture {
  Image image;std::uint32_t address=0,length=0;
  static bool identity(void* raw,std::uint32_t a,std::size_t n)noexcept{const auto& p=*static_cast<PatchFixture*>(raw);return a==p.address&&n==p.length;}
  static bool writable(void* raw,std::uint32_t a,std::size_t n)noexcept{return static_cast<PatchFixture*>(raw)->image.at(a,n)!=nullptr;}
  static bool replace(void* raw,std::uint32_t a,const void* old,const void* next,std::size_t n)noexcept{
    auto* p=static_cast<PatchFixture*>(raw)->image.at(a,n);if(!p||std::memcmp(p,old,n))return false;std::memcpy(p,next,n);return true;}
};
void local(const LocalFacts& facts){
  for(const auto& d:g::recipes){const auto& f=d.function;
    CHECK(hash(facts.bytecode.data()+f.byte_offset,f.byte_length)==f.body_sha256);
    for(const auto skill:{0,98,99,100,INT32_MAX})for(const auto n:{1u,2u,50u}){
      const auto p=g::plan(d,evidence(d,skill),n);CHECK(p.valid);
      PatchFixture fixture;fixture.address=code+f.byte_offset;fixture.length=f.byte_length;
      fixture.image.block(fixture.address,f.byte_length);std::memcpy(fixture.image.at(fixture.address,f.byte_length),facts.bytecode.data()+f.byte_offset,f.byte_length);
      ubc::AmountMemory memory{fixture.image.memory(),&fixture,PatchFixture::identity,PatchFixture::writable,PatchFixture::replace};
      ubc::AmountPatch patch;CHECK(patch.begin(memory,fixture.address,p.body(),n));
      CHECK(u32(fixture.image.at(fixture.address+151,4))==100&&u32(fixture.image.at(fixture.address+293,4))==1);
      for(std::size_t i=0;i<p.count;++i)CHECK(u32(fixture.image.at(fixture.address+p.operands[i].offset,4))==n-p.operands[i].reserved);
      if(skill>=100)CHECK(u32(fixture.image.at(fixture.address+167,4))==1);
      CHECK(patch.finish()&&!patch.active());CHECK(hash(fixture.image.at(fixture.address,f.byte_length),f.byte_length)==f.body_sha256);
    }
  }
}
}
int main(int argc,char** argv){grind_test::arithmetic();
  if(argc==4&&std::strcmp(argv[1],"--local-facts")==0){LocalFacts facts(argv[2],argv[3]);grind_test::local(facts);}
  else if(argc!=1)return 2;
  std::printf("grind_recipe_plan_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
