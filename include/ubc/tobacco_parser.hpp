// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/alchemy_flask_runtime.hpp"
#include "ubc/cool_predebit_checks.hpp"
#include <algorithm>

namespace ubc::tobacco {
using Checks=cool_predebit::check::Checks;
struct AdmissionChecks : Checks {
  using Checks::Checks;
  int symbol=0;
};
// The script call pushes HERO. The native post-debit needs the global Use
// instance for its Restore additionally. The diagnosis separates both.
inline bool parser_checks(const ReadOnlyMemory& memory,const alchemy_flask_debit::Context& ctx,
    Checks& checks,Checks& legacy) noexcept {
  namespace runtime=alchemy_flask_runtime;
  namespace script=runtime_script;
  struct Sample {
    script::detail::Layout layout{};
    std::array<std::uint32_t,5> address{},value{};
    std::uint32_t current=0,cursor=0,depth=0,use_symbol=0,use_address=0;
    std::array<std::uint32_t,8> words{};
  } first;
  for(unsigned pass=0;pass<2;++pass){
    Sample s;
    checks.flag("parser.context",alchemy_flask_debit::valid(ctx));
    const auto layout=script::detail::read_layout(memory,ctx.parser,runtime::flask_profile,s.layout);
    checks.equal("parser.layout",0,static_cast<int>(layout));
    const bool known=layout==script::Issue::none;
    constexpr const char* names[]={"binding.self","binding.other","binding.victim","binding.hero","binding.npc"};
    for(unsigned i=0;i<5;++i){
      const auto& b=runtime::flask_bindings[i];script::detail::Symbol symbol{};
      const auto issue=known?script::detail::check_binding(memory,s.layout,b):layout;
      checks.equal(names[i],0,static_cast<int>(issue),known);
      constexpr const char* lookups[]={"lookup.self","lookup.other","lookup.victim","lookup.hero","lookup.npc"};
      const auto lookup=known?script::detail::lookup(memory,s.layout,b.symbol,symbol):layout;
      checks.equal(lookups[i],0,static_cast<int>(lookup),known);
      if(known&&lookup==script::Issue::none){
        s.address[i]=symbol.address;s.value[i]=symbol.offset;
      }
    }
    bool distinct=s.address[0]!=0;
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<i;++j)
      if(!s.address[i]||s.address[i]==s.address[j])distinct=false;
    checks.flag("binding.distinct",distinct);
    checks.equal("actor.self",ctx.hero,s.value[0],s.address[0]!=0);
    checks.equal("actor.hero",ctx.hero,s.value[3],s.address[3]!=0);
    const auto word=[&](std::uint32_t address,std::uint32_t& value){return runtime::detail::word(memory,address,0,value);};
    const bool cur=word(runtime::current_parser_pointer,s.current);
    checks.equal("parser.current",ctx.parser,s.current,cur);
    const bool cursor=runtime::detail::word(memory,ctx.parser,0x4c,s.cursor);
    checks.test("parser.code_base","positiv",s.layout.code,s.layout.code!=0,known);
    checks.test("parser.code_length","1..16777216",s.layout.code_length,
      s.layout.code_length>0&&s.layout.code_length<=0x1000000u,known);
    checks.test("parser.code_end","darstellbar",s.layout.code_end,
      std::uint64_t{s.layout.code}+s.layout.code_length==s.layout.code_end,known);
    checks.test("parser.cursor","Codeanfang bis Codeende",s.cursor,
      s.cursor>=s.layout.code&&s.cursor<=s.layout.code_end,known&&cursor);
    const bool depth=runtime::detail::word(memory,ctx.parser,0x2058,s.depth);
    checks.test("parser.stack_capacity","0..2048",s.depth,s.depth<=station_abi::parser_data_stack_capacity,depth);
    // The original pre-debit may leave its INT return on the stack.
    checks.test("parser.stack_depth","0 oder 2",s.depth,s.depth==0||s.depth==2,depth);
    if(!depth||s.depth){
      const auto n=std::min<std::size_t>(s.depth,s.words.size());
      const bool words=depth&&n&&ctx.parser&&ctx.parser<=UINT32_MAX-station_abi::parser_data_stack&&
        memory.copy(ctx.parser+station_abi::parser_data_stack,s.words.data(),n*sizeof(s.words[0]));
      checks.flag("parser.stack_read",words);
      checks.equal("parser.stack_value",1,s.words[0],words);
      checks.equal("parser.stack_type",64,s.words[1],words&&n>=2);
    }
    const bool sym=word(station_abi::instance_symbol_pointer,s.use_symbol);
    const bool addr=word(station_abi::instance_address_pointer,s.use_address);
    checks.flag("parser.use_symbol_read",sym);checks.flag("parser.use_address_read",addr);
    legacy.test("legacy.use_symbol","SELF oder HERO",s.use_symbol,
      s.use_symbol&&(s.use_symbol==s.address[0]||s.use_symbol==s.address[3]),sym);
    legacy.equal("legacy.use_address",ctx.hero,s.use_address,addr);
    legacy.test("legacy.self_symbol","vorhanden",s.address[0],s.address[0]!=0,known);
    legacy.test("legacy.hero_symbol","vorhanden",s.address[3],s.address[3]!=0,known);
    checks.equal("parser.layout_stable",0,known?static_cast<int>(script::detail::stable_layout(memory,ctx.parser,runtime::flask_profile,s.layout)):-1,known);
    if(pass)checks.flag("parser.stable",script::detail::same_layout(first.layout,s.layout)&&s.address==first.address&&
      s.value==first.value&&s.current==first.current&&s.cursor==first.cursor&&s.depth==first.depth&&
      s.words==first.words&&s.use_symbol==first.use_symbol&&s.use_address==first.use_address);
    first=s;
  }
  return checks.ok();
}
} // namespace ubc::tobacco
