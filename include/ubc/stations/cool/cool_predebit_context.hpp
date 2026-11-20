// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_predebit_contract.hpp"
#include "ubc/stations/cool/cool_recipe_runtime.hpp"
#include "ubc/stations/cool/cool_predebit_checks.hpp"

namespace ubc::cool_predebit::context_check {
namespace rt = alchemy_flask_runtime;
namespace cr = cool_recipe_runtime;
namespace rs = runtime_script;

using Sample=check::Sample;
using Report=check::Report;

class Checks : public check::Checks {
public:
  Checks(const ReadOnlyMemory& memory, Report* report = nullptr) noexcept : check::Checks(report),memory_(memory) {}
  bool identity(const value::Context& c) noexcept {
    return nonzero("job.epoch",c.epoch) && nonzero("job.thread",c.thread) &&
      number("job.parser",lhiver::game_parser,c.parser) && nonzero("job.game",c.game) &&
      nonzero("job.world",c.world) && nonzero("job.hero",c.hero) && nonzero("job.mob",c.mob) &&
      word("game.pointer",selection_abi::game_pointer,0,c.game) &&
      word("game.vtable",c.game,0,selection_abi::game_vtable) &&
      word("game.world",c.game,selection_abi::game_world,c.world) &&
      word("hero.pointer",station_abi::hero_pointer,0,c.hero) &&
      word("hero.interact_mob",c.hero,station_abi::npc_interact_mob,c.mob) &&
      word("hero.inventory_vtable",c.hero,station_abi::npc_inventory,rt::inventory_vtable);
  }
  bool station(const value::Context& c) noexcept {
    const auto& names = cool_recipe_catalog::station;
    if (!word("hero.home_world",c.hero,0xb8,c.world)) return false;
    unsigned owners = 0;
    const auto positions = rt::detail::station_owners(memory_,c.mob,c.hero,owners);
    return number("station.hero_slots",1,owners,positions==nullptr,positions) &&
      word("station.state",c.mob,0x1f4,1) && word("station.target",c.mob,0x1fc,1,Rule::at_most) &&
      word("station.count",c.mob,0x1f8,1) && word("station.rewind",c.mob,0x200,0) &&
      name("station.scheme",c.mob,0x1b8,names.scheme) && name("station.on_state",c.mob,0x1e0,names.on_state) &&
      name("station.condition",c.mob,0x1cc,names.condition) && name("station.use_with",c.mob,0x1a4,names.use_with);
  }
  bool production(const value::Context& c,const cr::ProfileAssembly& assembly,bool after,
                  Observation& observation) noexcept {
    const auto state = cr::context(memory_,c.parser,assembly);
    const auto issue = static_cast<unsigned>(state.issue);
    if (!number("script.context_read",0,issue,state.read_valid || issue!=0)) return false;
    const auto script_value = [&](const char* field,std::uint32_t symbol,int expected,int actual) {
      if (collecting() && !state.read_valid) {
        // A missing first script symbol must not obscure the second in the report.
        rs::detail::Layout layout{};
        const auto* binding = assembly.binding(symbol);
        if (!binding || rs::detail::read_layout(memory_,c.parser,assembly.profile(),layout)!=rs::Issue::none)
          return signed_number(field,expected,0,false);
        const auto read = cr::shared::read_integer(memory_,layout,*binding,0);
        return signed_number(field,expected,read.value,read.issue==rs::Issue::none);
      }
      return signed_number(field,expected,actual,state.read_valid);
    };
    observation.production=state.production;
    observation.start_smithing=state.start_smithing;
    std::uint32_t invincible=0;
    const bool known=rt::detail::word(memory_,c.hero,0x298,invincible);
    observation.invincible=static_cast<std::int32_t>(invincible);
    return script_value("script.production",12330,after?0:13,state.production) &&
      script_value("script.start_smithing",10536,1,state.start_smithing) &&
      number("hero.invincible",after?0:1,invincible,known);
  }
  bool flags(const value::Context& c) noexcept {
    if (!word("parser.debug",c.parser,0x2198,0) || !word("engine.init",0xab642c,0,1,Rule::bit_set) ||
        !word("engine.refs",0xab11e4,0,1)) return false;
    std::uint8_t report=0;
    const bool known=memory_.copy(0x8cdcf0,&report,1);
    // CreateVob/RemoveVob jump with JL over the synchronous report. 0xff
    // corresponds to -1. Only the actual report branch from +10 remains locked.
    const int level=report<128?report:static_cast<int>(report)-256;
    char actual[8]{};
    std::snprintf(actual,sizeof(actual),"%d",level);
    return record("engine.report","<10",known?actual:"unlesbar",known && level<10);
  }

private:
  enum class Rule {equal,at_most,bit_set,nonzero};
  const ReadOnlyMemory& memory_;
  bool numeric(const char* field,std::uint64_t expected,std::uint64_t actual,bool known,Rule rule,
               const char* error=nullptr) noexcept {
    bool matches=false;
    const char* prefix="";
    switch (rule) {
      case Rule::equal: matches=actual==expected;break;
      case Rule::at_most: prefix="<=";matches=actual<=expected;break;
      case Rule::bit_set: prefix="bit_set:";matches=(actual&expected)==expected;break;
      case Rule::nonzero: prefix=">";matches=actual>expected;break;
    }
    if (!collecting() && known && matches) return true;
    char want[24]{}, got[24]{};
    std::snprintf(want,sizeof(want),"%s%llu",prefix,static_cast<unsigned long long>(expected));
    std::snprintf(got,sizeof(got),"%llu",static_cast<unsigned long long>(actual));
    return record(field,want,known?got:(error?error:"unlesbar"),known && matches);
  }
  bool number(const char* field,std::uint32_t expected,std::uint32_t actual,bool known=true,
              const char* error=nullptr) noexcept {
    return numeric(field,expected,actual,known,Rule::equal,error);
  }
  bool signed_number(const char* field,int expected,int actual,bool known) noexcept {
    if(!collecting() && known && actual==expected)return true;
    char want[16]{},got[16]{};
    std::snprintf(want,sizeof(want),"%d",expected);
    std::snprintf(got,sizeof(got),"%d",actual);
    return record(field,want,known?got:"unlesbar",known && actual==expected);
  }
  bool nonzero(const char* field,std::uint64_t actual) noexcept {
    return numeric(field,0,actual,true,Rule::nonzero);
  }
  bool word(const char* field,std::uint32_t base,std::uint32_t offset,std::uint32_t expected,
            Rule rule=Rule::equal) noexcept {
    std::uint32_t actual=0;
    const bool known=rt::detail::word(memory_,base,offset,actual);
    return numeric(field,expected,actual,known,rule);
  }
  bool name(const char* field,std::uint32_t mob,std::uint32_t offset,const char* expected) noexcept {

    std::array<std::uint8_t,20> header{};
    if (!mob || offset>UINT32_MAX-mob || !memory_.copy(mob+offset,header.data(),header.size()))
      return record(field,expected,"unlesbar",false);
    const auto length=rs::detail::u32(header.data()+12);
    char text[sizeof(Sample::actual)]{};
    if (length>=sizeof(text)) {
      std::snprintf(text,sizeof(text),"Laenge %u",length);
      return record(field,expected,text,false);
    }
    if (length && !memory_.copy(rs::detail::u32(header.data()+8),text,length+1))
      return record(field,expected,"unlesbar",false);
    if (text[length]) return record(field,expected,"NUL fehlt",false);
    for (std::uint32_t i=0;i<length;++i) {
      const unsigned char byte=static_cast<unsigned char>(text[i]);
      if (byte<32 || byte>126 || byte=='"' || byte=='\\') {
        std::snprintf(text,sizeof(text),"Byte %u an %u",byte,i);
        return record(field,expected,text,false);
      }
    }
    return record(field,expected,text,length==std::strlen(expected) && !std::strcmp(text,expected));
  }
};

inline Report diagnose(const ReadOnlyMemory& memory,const value::Context& c) noexcept {
  Report report;
  Checks checks(memory,&report);
  Observation observation;
  checks.identity(c);
  checks.station(c);
  checks.production(c,cr::assemble(cool_recipe_catalog::recipe),false,observation);
  checks.flags(c);
  return report;
}
} // namespace ubc::cool_predebit::context_check
