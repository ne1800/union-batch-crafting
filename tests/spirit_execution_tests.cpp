// SPDX-License-Identifier: MIT
// Existing optional file facts remain in RAM only. No original bytes in the repository.
#define UBC_ALCHEMY_EXTRA_EXECUTION_TEST_EMBEDDED
#include "alchemy_extra_execution_tests.cpp"
#include "ubc/stations/spirit/spirit_execution.hpp"
#include "ubc/stations/spirit/spirit_flask_runtime.hpp"
[[maybe_unused]] auto* const spirit_fixture_keep_local=&execution_test::local;

namespace spirit_execution_test {
namespace sc=ubc::spirit_recipe_catalog;namespace sr=ubc::spirit_recipe_runtime;
namespace se=ubc::spirit_execution;namespace sd=ubc::spirit_debit;namespace sf=ubc::spirit_flask_runtime;
struct SpiritFixture:execution_test::ExecutionFixture {
  SpiritFixture(const LocalFacts* facts=nullptr,const std::array<std::string,4>* texts=nullptr,std::int32_t branch=1)
    :ExecutionFixture(cat::recipes[0],facts,texts){
    const auto assembled=sr::assemble(sc::recipes[0]);const auto p=assembled.profile();ubc::alchemy_extra_execution::Profile shared;
    for(std::size_t i=0;i<p.function_count;++i)shared.add(&p.functions[i]);
    for(std::size_t i=0;i<p.binding_count;++i)shared.add(p.bindings[i]);install(shared,facts);
    for(const auto& flag:ubc::alchemy_extra_navigation::base::flag_bindings)image.put(symbol(flag.symbol)+0x18,0);
    info(sc::recipes[0].owner);image.put(symbol(11198)+0x18,1);image.put(symbol(11967)+0x18,static_cast<std::uint32_t>(branch));
    image.put(symbol(20361)+0x18,1);set_production(4);boundary.scope.callback=20370;boundary.serial=10;
    std::vector<std::pair<std::int32_t,std::int32_t>> values;
    for(std::size_t i=0;i<7;++i)values.push_back({sc::targets[i].symbol,i<5?100:static_cast<std::int32_t>(i-2)});storage(values,"");
  }
  sd::Job job(std::uint32_t n=3)const {sd::Job j;static_cast<debit::Job&>(j)={boundary.context,1,1,20370,20368,n};j.branch_value=static_cast<std::int32_t>(u32(const_cast<Image&>(image).at(symbol(11967)+0x18,4)));return j;}
  ubc::SpiritRecipeCapture snapshot(){ubc::SpiritRecipeReader reader;return reader.capture(image.memory(),ubc::lhiver::game_parser,
    {1,boundary.current_thread,world,hero,mob,0xA5000,ubc::recipe_catalog::Station::alchemy},{true,true});}
  void parser_stack(std::uint32_t count){
    image.put(ubc::lhiver::game_parser+0x2058,count);
    for(std::uint32_t i=0;i<8;++i)image.put(ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack+i*4,i%2?64u:1u);
  }
  void success(unsigned n){
    for(std::uint32_t i=0;i<4;++i)image.put(object(i)+0x32c,u32(image.at(object(i)+0x32c,4))-n*(i?1u:2u));
    const auto index=u32(image.at(symbol(11967)+0x18,4))==1?6u:5u;image.put(object(index)+0x32c,u32(image.at(object(index)+0x32c,4))+n);
    set_production(0);image.put(hero+0x298,0);parser_stack(8);
  }
};
sf::CoreReader reader(7);
void structural(){SpiritFixture f;CHECK(!se::verify_entry(f.image.memory(),execution_test::actual_info,1).valid);
  auto b=f.boundary;b.current_thread=8;CHECK(!reader.capture(f.image.memory(),b,1).valid);
  b=f.boundary;b.scope.callback=20373;CHECK(!reader.capture(f.image.memory(),b,1).valid);
}
void local(const LocalFacts& facts,const std::array<std::string,4>& texts){
  for(const auto branch:{1,0,2,INT32_MIN,INT32_MAX}){SpiritFixture f(&facts,&texts,branch);
    const auto entry=se::verify_entry(f.image.memory(),execution_test::actual_info,branch);CHECK(entry.valid&&entry.code_address==code+732061);
    const auto before=reader.capture(f.image.memory(),f.boundary,branch);CHECK(before.valid&&before.observation.stocks.count==7&&before.observation.branch_value==branch);
    CHECK(f.snapshot().state.contract&&f.snapshot().state.material.maximum==50);
    CHECK(!se::verify_entry(f.image.memory(),execution_test::actual_info,branch==1?0:1).valid);
    f.success(3);f.boundary.serial++;
    const auto after=reader.capture(f.image.memory(),f.boundary,branch,{},sf::ParserStackMode::original_returns);CHECK(after.valid&&after.observation.production==0);
  }
  for(unsigned mode=0;mode<16;++mode){SpiritFixture f(&facts,&texts);auto b=f.boundary;
    switch(mode){case 0:f.image.put(symbol(11198)+0x18,0);break;case 1:f.image.put(symbol(20361)+0x18,0);break;
      case 2:f.image.put(execution_test::actual_info+0x58,1);break;case 3:f.image.put(execution_test::actual_info+0x50,20371);break;
      case 4:f.image.put(code+732061,0);break;case 5:f.image.denied=symbol(11967);break;
      case 6:f.image.put(symbol(11967)+0x20,1|(3u<<12));break;case 7:f.image.put(symbol(11967)+0x18,2);break;
      case 8:f.image.put(object(0)+0x158,0x40200001);break;case 9:f.image.put(object(4)+0x32c,99);break;
      case 10:f.image.put(hero+0x9b8,3);f.image.put(extra_slot+0x30,object(0));break;
      case 11:f.image.put(hero+0x968,0);break;case 12:f.image.put(rt::code_ranges[0].address,0);break;
      case 13:b.native_dependencies_and_configured_targets_verified=false;break;case 14:f.image.watch=symbol(11967);f.image.change_on=3;f.image.change_target=symbol(11967)+0x18;f.image.change_value=2;break;
      case 15:f.image.put(symbol(1683)+0x1c,hero+4);break;}
    if(mode<8)CHECK(!se::verify_entry(f.image.memory(),execution_test::actual_info,1).valid);
    else if(mode==9){const auto x=reader.capture(f.image.memory(),b,1);CHECK(x.valid&&x.observation.flask.amount==99);}
    else CHECK(!reader.capture(f.image.memory(),b,1).valid);
  }
}
void parser_phases(){
  constexpr auto stack=ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack;
  SpiritFixture f;
  const auto before=sf::read_parser(f.image.memory(),f.boundary.context);CHECK(before.valid);
  CHECK(!sf::read_parser(f.image.memory(),f.boundary.context,sf::ParserStackMode::original_returns).valid);
  f.parser_stack(8);
  const auto after=sf::read_parser(f.image.memory(),f.boundary.context,sf::ParserStackMode::original_returns);
  CHECK(after.valid&&after.value.data_words_known&&after.value.data_depth==8);
  CHECK(!sf::read_parser(f.image.memory(),f.boundary.context).valid);
  CHECK(!rt::read_parser(f.image.memory(),f.boundary.context,rt::ParserStackMode::original_returns,20370).valid);
  CHECK(!sd::valid_after_original(after.value,f.boundary.context,20395));
  for(const auto count:{0u,1u,2u,4u,6u,7u,9u,2048u,2049u}){
    f.parser_stack(count);CHECK(!sf::read_parser(f.image.memory(),f.boundary.context,sf::ParserStackMode::original_returns).valid);
  }
  for(std::uint32_t i=0;i<8;++i){
    f.parser_stack(8);f.image.put(stack+i*4,9);CHECK(!sf::read_parser(f.image.memory(),f.boundary.context,sf::ParserStackMode::original_returns).valid);
    f.parser_stack(8);f.image.denied=stack+i*4;CHECK(!sf::read_parser(f.image.memory(),f.boundary.context,sf::ParserStackMode::original_returns).valid);f.image.denied=0;
    f.parser_stack(8);f.image.watch=stack;f.image.watch_reads=0;f.image.change_on=2;f.image.change_target=stack+i*4;f.image.change_value=7;
    CHECK(!sf::read_parser(f.image.memory(),f.boundary.context,sf::ParserStackMode::original_returns).valid);
    f.image.watch=0;f.image.change_on=0;
  }
  f.parser_stack(8);CHECK(!sf::read_parser(f.image.memory(),f.boundary.context,static_cast<sf::ParserStackMode>(99)).valid);
}
[[maybe_unused]] void restore_prefix(const LocalFacts& facts,const std::array<std::string,4>& texts){
  constexpr auto stack=ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack;
  SpiritFixture f(&facts,&texts);f.success(3);
  const auto current=reader.capture(f.image.memory(),f.boundary,1,{},sf::ParserStackMode::original_returns);CHECK(current.valid);
  sd::Call call{true,1,1,f.boundary.context,20370,16604,2,object(4),current.observation.parser};
  f.boundary.scope.debit_depth=1;
  // Only PopPos/UseInstance were changed by the internal constructor.
  f.image.put(ubc::lhiver::game_parser+0x4c,code+900);
  f.image.put(ubc::station_abi::instance_symbol_pointer,symbol(16604));f.image.put(ubc::station_abi::instance_address_pointer,0xDEADBEEF);
  CHECK(reader.restore_targets(f.image.memory(),call,f.boundary).valid);
  for(std::uint32_t i=0;i<8;++i){f.parser_stack(8);f.image.put(stack+i*4,5);
    CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid);
  }
  f.parser_stack(0);CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid);
  f.parser_stack(8);f.image.watch=stack;f.image.watch_reads=0;f.image.change_on=2;f.image.change_target=stack+28;f.image.change_value=3;
  CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid);
  f.image.watch=0;f.parser_stack(8);
  auto stale=call;stale.parser.data_words_known=false;CHECK(!reader.restore_targets(f.image.memory(),stale,f.boundary).valid);
  stale=call;stale.callback=20289;CHECK(!reader.restore_targets(f.image.memory(),stale,f.boundary).valid);
}
}
#ifndef UBC_SPIRIT_EXECUTION_TEST_EMBEDDED
int main(int argc,char** argv){spirit_execution_test::structural();spirit_execution_test::parser_phases();
  if(argc==4&&std::strcmp(argv[1],"--local-facts")==0){LocalFacts facts(argv[2],argv[3]);const auto texts=execution_test::read_prints(argv[3]);spirit_execution_test::local(facts,texts);spirit_execution_test::restore_prefix(facts,texts);}
  else if(argc!=1)return 2;
  std::printf("spirit_execution_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;}
#endif
