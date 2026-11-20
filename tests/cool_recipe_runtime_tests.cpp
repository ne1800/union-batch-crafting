// SPDX-License-Identifier: MIT
#include "ubc/cool_recipe_runtime.hpp"
#define UBC_STATION_RECIPE_FIXTURE_ONLY
#include "station_recipe_runtime_tests.cpp"
namespace cool_test {
namespace cat=ubc::cool_recipe_catalog;namespace rt=ubc::cool_recipe_runtime;namespace rs=ubc::runtime_script;
using station_test::Image;using station_test::check;using station_test::symbol;using station_test::name_at;using station_test::table;using station_test::code;
struct Fixture {
  Image image;rt::ProfileAssembly assembly=rt::assemble(cat::recipe);std::vector<ubc::lhiver::Function> synthetic;
  Fixture(){image.block(ubc::lhiver::game_parser,0x58);image.block(table,static_cast<std::size_t>(ubc::lhiver::symbol_count+2)*4);
    image.put(ubc::lhiver::game_parser+0x18,table);image.put(ubc::lhiver::game_parser+0x1c,ubc::lhiver::symbol_count+2);image.put(ubc::lhiver::game_parser+0x20,ubc::lhiver::symbol_count);
    image.put(ubc::lhiver::game_parser+0x48,code);image.put(ubc::lhiver::game_parser+0x50,code+ubc::lhiver::code_length);image.put(ubc::lhiver::game_parser+0x54,ubc::lhiver::code_length+128);
    const auto p=assembly.profile();for(std::size_t i=0;i<p.binding_count;++i){const auto& b=p.bindings[i];make(b.symbol,b.name,b.elements,b.type,b.flags,b.check_value?static_cast<std::uint32_t>(b.value):0,static_cast<std::uint32_t>(b.offset),b.parent_index);}
    for(std::size_t i=0;i<p.function_count;++i){const auto& f=p.functions[i];make(f.symbol,f.name,f.argc,f.type,f.flags,f.byte_offset,f.type==7?0x61000000:f.return_type,f.parent_index);
      image.block(code+f.byte_offset,f.byte_length);auto* b=image.at(code+f.byte_offset,f.byte_length);for(std::uint32_t j=0;j<f.byte_length;++j)b[j]=static_cast<std::uint8_t>((f.symbol+j*37u)%256u);}
    image.put(symbol(1683)+0x1c,0x2000);image.put(symbol(1679)+0x1c,0x2000);image.put(symbol(12330)+0x18,13);image.put(symbol(10536)+0x18,1);
  }
  void make(std::uint32_t id,const char* name,std::uint32_t count,std::uint32_t type,std::uint32_t flags,std::uint32_t data,std::uint32_t offset,std::int32_t parent){
    image.block(symbol(id),0x3c);image.block(name_at(id),std::strlen(name)+1);image.put(table+id*4,symbol(id));image.put(symbol(id)+8,name_at(id));image.put(symbol(id)+12,static_cast<std::uint32_t>(std::strlen(name)));image.put(symbol(id)+16,static_cast<std::uint32_t>(std::strlen(name)+1));
    std::memcpy(image.at(name_at(id),std::strlen(name)+1),name,std::strlen(name)+1);image.put(symbol(id)+0x18,data);image.put(symbol(id)+0x1c,offset);image.put(symbol(id)+0x20,station_test::metadata(count,type,flags));image.put(symbol(id)+0x38,parent<0?0:symbol(static_cast<std::uint32_t>(parent)));
    if(parent>=0&&!image.at(symbol(static_cast<std::uint32_t>(parent)),0x3c)){image.block(symbol(static_cast<std::uint32_t>(parent)),0x3c);image.put(table+static_cast<std::uint32_t>(parent)*4,symbol(static_cast<std::uint32_t>(parent)));}
  }
  rs::RuntimeContract synthetic_verify(){auto p=assembly.profile();synthetic.clear();for(std::size_t i=0;i<p.function_count;++i){auto f=p.functions[i];ubc::Sha256 h;h.update(image.at(code+f.byte_offset,f.byte_length),f.byte_length);f.body_sha256=h.finish();synthetic.push_back(f);}p.functions=synthetic.data();return rs::verify_local_contract(image.memory(),ubc::lhiver::game_parser,p);}
};
}
#ifndef UBC_COOL_RUNTIME_FIXTURE_ONLY
int main(){using namespace cool_test;
  Fixture f;check(f.assembly.valid()&&f.assembly.function_count()==15,"complete original single triple end entry helpers and items");
  auto copy=f.assembly;f.assembly={};check(copy.valid()&&copy.profile().functions==cat::functions.data(),"immutable canonical static profile has independent safe lifetime");f.assembly=copy;
  check(f.synthetic_verify().matches&&!rt::verify(f.image.memory(),ubc::lhiver::game_parser,f.assembly).matches,"synthetic profile positive, canonical hash negative");
  check(rt::knowledge(f.image.memory(),ubc::lhiver::game_parser,f.assembly).known,"no invented smith skill learning condition");
  check(rt::context(f.image.memory(),ubc::lhiver::game_parser,f.assembly).station_matches(),"real production and usecool state");
  for(const auto value:{0,2,-1,INT32_MAX}){f.image.put(symbol(10536)+0x18,static_cast<std::uint32_t>(value));check(!rt::context(f.image.memory(),ubc::lhiver::game_parser,f.assembly).station_matches()&&rt::knowledge(f.image.memory(),ubc::lhiver::game_parser,f.assembly).known,"station entry condition does not become learning");}
  for(const auto id:{12330u,10536u})for(unsigned change=0;change<7;++change){Fixture x;switch(change){case 0:x.image.put(symbol(id)+0x20,station_test::metadata(1,2,1));break;case 1:x.image.put(symbol(id)+0x20,station_test::metadata(2,2,0));break;case 2:x.image.put(symbol(id)+0x38,symbol(1537));break;case 3:x.image.at(name_at(id),1)[0]='X';break;case 4:x.image.denied=symbol(id)+0x18;x.image.denied_size=4;break;case 5:x.image.change_address=symbol(id);x.image.change_target=symbol(id)+0x18;x.image.change_on_read=2;break;case 6:x.image.change_layout=true;break;}check(!rt::context(x.image.memory(),ubc::lhiver::game_parser,x.assembly).read_valid,"bad or changing context integer rejected");}
  for(unsigned change=0;change<6;++change){Fixture x;switch(change){case 0:x.image.put(symbol(20775)+0x18,792601);break;case 1:x.image.put(symbol(20778)+0x1c,2);break;case 2:x.image.put(symbol(20774)+0x20,station_test::metadata(1,5,3));break;case 3:x.image.put(symbol(16612)+0x38,symbol(1537));break;case 4:x.image.put(symbol(12323)+0x18,1);break;case 5:x.image.at(name_at(16613),1)[0]='X';break;}check(!x.synthetic_verify().matches,"canonical function C_INFO item and constant metadata stay exact");}
  Fixture actor;check(rt::actor(actor.image.memory(),ubc::lhiver::game_parser,actor.assembly,0x2000).matches,"SELF and HERO use instance offsets");actor.image.put(symbol(1679)+0x1c,0x3000);check(!rt::actor(actor.image.memory(),ubc::lhiver::game_parser,actor.assembly,0x2000).matches,"foreign SELF is not an effect boundary");
  Fixture changing;changing.image.change_address=symbol(1683);changing.image.change_target=symbol(1683)+0x1c;changing.image.change_on_read=2;check(!rt::actor(changing.image.memory(),ubc::lhiver::game_parser,changing.assembly,0x2000).read_valid,"two-pass actor mutation rejected");
  check(!rt::knowledge(actor.image.memory(),1,actor.assembly).read_valid,"wrong parser never creates knowledge");
  std::printf("cool_recipe_runtime_tests: %u checks, %u failures\n",station_test::checks,station_test::failures);return station_test::failures?1:0;
}
#endif
