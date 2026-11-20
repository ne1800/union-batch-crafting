// SPDX-License-Identifier: MIT
#include "ubc/bow_recipe_runtime.hpp"
#define UBC_STATION_RECIPE_FIXTURE_ONLY
#include "station_recipe_runtime_tests.cpp"

namespace bow_test {
namespace cat=ubc::bow_recipe_catalog;
namespace rt=ubc::bow_recipe_runtime;
namespace rs=ubc::runtime_script;
using station_test::Image;using station_test::check;using station_test::symbol;
using station_test::name_at;using station_test::array_at;using station_test::table;using station_test::code;
struct Fixture {
  rt::ProfileAssembly assembly;
  Image image;
  std::vector<ubc::lhiver::Function> synthetic;
  explicit Fixture(const cat::Descriptor& recipe):assembly(rt::assemble(recipe)) {
    image.block(ubc::lhiver::game_parser,0x58);
    image.block(table,static_cast<std::size_t>(ubc::lhiver::symbol_count+2)*4);
    image.put(ubc::lhiver::game_parser+0x18,table);
    image.put(ubc::lhiver::game_parser+0x1c,ubc::lhiver::symbol_count+2);
    image.put(ubc::lhiver::game_parser+0x20,ubc::lhiver::symbol_count);
    image.put(ubc::lhiver::game_parser+0x48,code);
    image.put(ubc::lhiver::game_parser+0x50,code+ubc::lhiver::code_length);
    image.put(ubc::lhiver::game_parser+0x54,ubc::lhiver::code_length+128);
    populate(recipe);
  }
  void make_symbol(std::uint32_t id,const char* name,std::uint32_t count,std::uint32_t type,
      std::uint32_t flags,std::uint32_t data,std::uint32_t offset,std::int32_t parent) {
    image.block(symbol(id),0x3c);image.block(name_at(id),std::strlen(name)+1);
    image.put(table+id*4,symbol(id));image.put(symbol(id)+8,name_at(id));
    image.put(symbol(id)+12,static_cast<std::uint32_t>(std::strlen(name)));
    image.put(symbol(id)+16,static_cast<std::uint32_t>(std::strlen(name)+1));
    image.put(symbol(id)+0x18,data);image.put(symbol(id)+0x1c,offset);
    image.put(symbol(id)+0x20,station_test::metadata(count,type,flags));
    image.put(symbol(id)+0x38,parent<0?0:symbol(static_cast<std::uint32_t>(parent)));
    std::memcpy(image.at(name_at(id),std::strlen(name)+1),name,std::strlen(name)+1);
    if(parent>=0&&!image.at(symbol(static_cast<std::uint32_t>(parent)),0x3c)) {
      image.block(symbol(static_cast<std::uint32_t>(parent)),0x3c);
      image.put(table+static_cast<std::uint32_t>(parent)*4,symbol(static_cast<std::uint32_t>(parent)));
    }
  }
  void populate(const cat::Descriptor& recipe) {
    const auto own=rt::assemble(recipe);const auto p=own.profile();
    for(std::size_t i=0;i<p.binding_count;++i) {
      const auto& b=p.bindings[i];
      make_symbol(b.symbol,b.name,b.elements,b.type,b.flags,b.check_value?static_cast<std::uint32_t>(b.value):0,
        static_cast<std::uint32_t>(b.offset),b.parent_index);
      if(b.type==2&&b.elements>1&&b.parent_index==-1) {
        image.block(array_at(b.symbol),static_cast<std::size_t>(b.elements)*4);
        image.put(symbol(b.symbol)+0x18,array_at(b.symbol));
        for(std::uint32_t j=0;j<b.elements;++j)image.put(array_at(b.symbol)+j*4,1);
      }
    }
    for(std::size_t i=0;i<p.function_count;++i) {
      const auto& f=p.functions[i];
      make_symbol(f.symbol,f.name,f.argc,f.type,f.flags,f.byte_offset,
        f.type==7?0x60000000+f.symbol*0x100:f.return_type,f.parent_index);
      image.block(code+f.byte_offset,f.byte_length);auto* bytes=image.at(code+f.byte_offset,f.byte_length);
      for(std::uint32_t j=0;j<f.byte_length;++j)bytes[j]=static_cast<std::uint8_t>((f.symbol+j*37u)%256u);
    }
    image.put(symbol(1683)+0x1c,0x2000);image.put(symbol(1679)+0x1c,0x2000);
    image.put(symbol(12330)+0x18,16);image.put(symbol(20862)+0x18,0);image.put(symbol(10484)+0x18,1);
  }
  void learned(const cat::Descriptor& recipe,std::int32_t value) {
    for(const auto& r:recipe.requirements)if(r.role==cat::RequirementRole::knowledge)
      image.put(symbol(static_cast<std::uint32_t>(r.symbol))+0x18,static_cast<std::uint32_t>(value));
  }
  rs::RuntimeContract synthetic_verify() {
    auto p=assembly.profile();synthetic.clear();
    for(std::size_t i=0;i<p.function_count;++i) {
      auto f=p.functions[i];ubc::Sha256 hash;hash.update(image.at(code+f.byte_offset,f.byte_length),f.byte_length);
      f.body_sha256=hash.finish();synthetic.push_back(f);
    }
    p.functions=synthetic.data();return rs::verify_local_contract(image.memory(),ubc::lhiver::game_parser,p);
  }
};
}

#ifndef UBC_BOW_RUNTIME_FIXTURE_ONLY
int main(){using namespace bow_test;
  for(const auto& r:cat::recipes){const auto a=rt::assemble(r);check(a.valid()&&a.function_count()<=64&&a.binding_count()<=64,"bounded exact bow profile");
    check(a.knowledge_count()==1,"shared category knowledge is retained although absent from local owner condition");
    const auto p=a.profile();for(const auto* required:{r.owner.constructor,r.owner.condition,r.callback.function,cat::station.entry,cat::station.category.information,cat::station.end.information,cat::function_for(14646),cat::function_for(19965)}){
      bool found=false;for(std::size_t i=0;i<p.function_count;++i)found|=p.functions[i].symbol==required->symbol;check(found,"complete category and effect lifecycle closure");}
    for(const auto& item:r.items){bool found=false;for(std::size_t i=0;i<p.function_count;++i)found|=p.functions[i].symbol==item.constructor->symbol;check(found,"all exact input and output constructors bound");}
    auto copy=a;check(copy.profile().functions!=p.functions&&copy.profile().functions[0].name!=p.functions[0].name,"profile copy owns tables and names");
    Fixture f(r);check(f.synthetic_verify().matches,"own positive synthetic body hashes are validated");check(!rt::verify(f.image.memory(),ubc::lhiver::game_parser,f.assembly).matches,"original hashes are never bypassed for synthetic fixtures");
    check(rt::knowledge(f.image.memory(),ubc::lhiver::game_parser,f.assembly).known,"genuine category talent is learned");
    for(const auto v:{0,2,-1,INT32_MAX}){f.learned(r,v);const auto k=rt::knowledge(f.image.memory(),ubc::lhiver::game_parser,f.assembly);check(k.read_valid&&!k.known,"exact talent value one only");}
    f.learned(r,1);f.image.put(symbol(20862)+0x18,0);auto c=rt::context(f.image.memory(),ubc::lhiver::game_parser,f.assembly);
    check(c.station_matches()&&!c.owner_matches()&&rt::knowledge(f.image.memory(),ubc::lhiver::game_parser,f.assembly).known,"root navigation permitted without already-set category");
    f.image.put(symbol(20862)+0x18,1);c=rt::context(f.image.memory(),ubc::lhiver::game_parser,f.assembly);check(c.owner_matches(),"target category exact one");
    f.image.put(symbol(12330)+0x18,0);check(rt::knowledge(f.image.memory(),ubc::lhiver::game_parser,f.assembly).known&&!rt::context(f.image.memory(),ubc::lhiver::game_parser,f.assembly).station_matches(),"physical context is never learning");
    f.image.put(symbol(1679)+0x1c,0x3000);check(!rt::actor(f.image.memory(),ubc::lhiver::game_parser,f.assembly,0x2000).matches,"SELF used by station and B_SAY binds actual HERO");
    f.image.put(symbol(1679)+0x1c,0x2000);f.image.put(symbol(1679)+0x18,0xabcdef00);check(rt::actor(f.image.memory(),ubc::lhiver::game_parser,f.assembly,0x2000).matches,"instance offset is not data field");
  }
  for(const auto id:{10484u,12330u,20862u})for(unsigned change=0;change<7;++change){Fixture f(cat::recipes[0]);
    switch(change){case 0:f.image.put(symbol(id)+0x20,station_test::metadata(1,2,1));break;case 1:f.image.put(symbol(id)+0x20,station_test::metadata(2,2,0));break;
      case 2:f.image.put(symbol(id)+0x38,symbol(1537));break;case 3:f.image.at(name_at(id),1)[0]='X';break;
      case 4:f.image.denied=symbol(id)+0x18;f.image.denied_size=4;break;
      case 5:f.image.put(symbol(id)+0x18,1);f.image.change_address=symbol(id);f.image.change_target=symbol(id)+0x18;f.image.change_on_read=2;break;
      case 6:f.image.change_layout=true;break;}
    const auto valid=id==10484?rt::knowledge(f.image.memory(),ubc::lhiver::game_parser,f.assembly).read_valid:rt::context(f.image.memory(),ubc::lhiver::game_parser,f.assembly).read_valid;
    check(!valid,"wrong metadata parent name unreadable or unstable INT fails closed");}
  for(unsigned change=0;change<5;++change){Fixture f(cat::recipes[0]);switch(change){case 0:f.image.put(symbol(20870)+0x18,803855);break;
    case 1:f.image.put(symbol(20870)+0x1c,2);break;case 2:f.image.put(symbol(16811)+0x38,symbol(1537));break;
    case 3:f.image.put(symbol(1591)+0x1c,345);break;case 4:f.image.put(symbol(12326)+0x18,17);break;}
    check(!f.synthetic_verify().matches,"exact category item flags member offset and production constant are guarded");}
  Fixture actor_change(cat::recipes[0]);actor_change.image.change_address=symbol(1683);actor_change.image.change_target=symbol(1683)+0x1c;actor_change.image.change_on_read=2;
  check(!rt::actor(actor_change.image.memory(),ubc::lhiver::game_parser,actor_change.assembly,0x2000).read_valid,"changed actor invalidates proof");
  Fixture wrong(cat::recipes[0]);check(!rt::knowledge(wrong.image.memory(),1,wrong.assembly).read_valid&&!rt::context(wrong.image.memory(),1,wrong.assembly).read_valid,"wrong parser rejected");
  auto altered=cat::recipes[0];altered.require_self_is_hero=false;check(!rt::assemble(altered).valid(),"copy cannot weaken actor requirements");
  std::printf("bow_recipe_runtime_tests: %u checks, %u failures\n",station_test::checks,station_test::failures);return station_test::failures?1:0;
}
#endif
