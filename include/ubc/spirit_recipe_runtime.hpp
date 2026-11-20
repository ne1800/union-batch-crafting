// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/spirit_recipe_catalog.hpp"
#include "ubc/alchemy_extra_execution.hpp"
#include "ubc/station_recipe_runtime.hpp"

namespace ubc::spirit_recipe_runtime {
namespace cat=spirit_recipe_catalog;
namespace rs=runtime_script;
namespace shared=station_recipe_runtime::detail;
using ActorEvidence=station_recipe_runtime::ActorEvidence;
using AssemblyIssue=station_recipe_runtime::AssemblyIssue;

// Tables are value copies. Names come from own constexpr data only
// after the canonical comparison, never from the passed descriptor.
class ProfileAssembly {
public:
  bool valid() const noexcept {return issue_==AssemblyIssue::none&&value_.valid();}
  AssemblyIssue issue() const noexcept {return issue_;}
  std::size_t function_count() const noexcept {return profile().function_count;}
  std::size_t binding_count() const noexcept {return profile().binding_count;}
  lhiver::Profile profile() const & noexcept {return valid()?value_.view():lhiver::Profile{};}
  lhiver::Profile profile() const &&=delete;
  const lhiver::SymbolBinding* binding(std::uint32_t symbol) const noexcept {
    const auto p=profile();for(std::size_t i=0;i<p.binding_count;++i)if(p.bindings[i].symbol==symbol)return &p.bindings[i];
    return nullptr;
  }
private:
  friend ProfileAssembly assemble(const cat::Descriptor&) noexcept;
  alchemy_extra_execution::Profile value_{};
  AssemblyIssue issue_=AssemblyIssue::descriptor;
};
inline ProfileAssembly assemble(const cat::Descriptor& descriptor) noexcept {
  ProfileAssembly out;if(!cat::structurally_valid(descriptor))return out;
  const auto& d=*cat::find(descriptor.id);auto& p=out.value_;
  alchemy_extra_execution::common(p);
  for(const auto& b:d.bindings)p.add(b);
  alchemy_extra_execution::owner(p,d.owner);
  for(const auto* f:{d.station_entry,d.station_end,d.end_production,d.say})p.add(f);
  for(const auto* f:d.item_constructors)p.add(f);
  if(p.valid()&&rs::detail::valid_profile(p.view()))out.issue_=AssemblyIssue::none;
  return out;
}
inline rs::RuntimeContract verify(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& a) noexcept {
  return a.valid()?rs::verify_local_contract(memory,parser,a.profile()):rs::RuntimeContract{false,rs::Issue::invalid_profile,0};
}
inline rs::RuntimeContract verify(const ReadOnlyMemory& memory,std::uint32_t parser,const cat::Descriptor& d) noexcept {
  const auto a=assemble(d);return verify(memory,parser,a);
}
struct Verification {
  bool valid=false;rs::RuntimeContract local{};
  alchemy_flask_runtime::Issue string_issue=alchemy_flask_runtime::Issue::none;
  std::uint32_t failed_symbol=0;
};
// Includes both print texts without quantities, but no new full-DAT identity
// and no native/writer release. The extra flask effect remains separate.
inline Verification verify_recipe(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& a) noexcept {
  Verification out;if(!a.valid()){out.local.issue=rs::Issue::invalid_profile;return out;}
  const auto p=a.profile();rs::detail::Layout initial{};
  out.local.issue=rs::detail::read_layout(memory,parser,p,initial);if(out.local.issue!=rs::Issue::none)return out;
  for(unsigned pass=0;pass<2;++pass) {
    out.local=verify(memory,parser,a);if(!out.local.matches)return out;
    for(const auto& text:cat::recipes[0].prints) {
      const auto value=alchemy_flask_runtime::detail::string_once(memory,initial,{text.symbol,text.length,text.digest});
      if(!value.valid){out.string_issue=value.issue;out.failed_symbol=text.symbol;return out;}
    }
    out.local.issue=rs::detail::stable_layout(memory,parser,p,initial);if(out.local.issue!=rs::Issue::none){out.local.matches=false;return out;}
  }
  out.valid=true;return out;
}

template<std::size_t N>struct IntegerEvidence {
  bool read_valid=false;rs::Issue issue=rs::Issue::none;std::uint32_t failed_symbol=0;
  rs::detail::Layout layout{};std::array<shared::IntegerRead,N> readings{};
};
template<std::size_t N>inline bool same_integer_evidence(const IntegerEvidence<N>& a,const IntegerEvidence<N>& b) noexcept {
  if(!a.read_valid||!b.read_valid||!rs::detail::same_layout(a.layout,b.layout))return false;
  for(std::size_t i=0;i<N;++i)if(a.readings[i].value!=b.readings[i].value||
    !shared::same_symbol(a.readings[i].symbol,b.readings[i].symbol))return false;
  return true;
}
namespace detail {
template<std::size_t N>inline IntegerEvidence<N> integers(const ReadOnlyMemory& memory,std::uint32_t parser,
    const ProfileAssembly& a,const std::array<std::uint32_t,N>& symbols) noexcept {
  IntegerEvidence<N> out;if(!a.valid()){out.issue=rs::Issue::invalid_profile;return out;}
  const auto p=a.profile();out.issue=rs::detail::read_layout(memory,parser,p,out.layout);if(out.issue!=rs::Issue::none)return out;
  for(unsigned pass=0;pass<2;++pass)for(std::size_t i=0;i<N;++i) {
    const auto* binding=a.binding(symbols[i]);if(!binding){out.issue=rs::Issue::invalid_profile;return out;}
    const auto value=shared::read_integer(memory,out.layout,*binding,0);
    if(value.issue!=rs::Issue::none){out.issue=value.issue;out.failed_symbol=symbols[i];return out;}
    if(pass&&(value.value!=out.readings[i].value||!shared::same_symbol(value.symbol,out.readings[i].symbol))) {
      out.issue=rs::Issue::changed_during_read;out.failed_symbol=symbols[i];return out;
    }
    out.readings[i]=value;
  }
  out.issue=rs::detail::stable_layout(memory,parser,p,out.layout);out.read_valid=out.issue==rs::Issue::none;return out;
}
inline ActorEvidence actor_once(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& a,std::uint32_t hero) noexcept {
  ActorEvidence out;out.requires_self=true;if(!a.valid()||!hero){out.issue=rs::Issue::invalid_profile;return out;}
  const auto p=a.profile();out.issue=rs::detail::read_layout(memory,parser,p,out.layout);if(out.issue!=rs::Issue::none)return out;
  out.issue=shared::instance(memory,out.layout,p,1683,out.hero_symbol);if(out.issue!=rs::Issue::none){out.failed_symbol=1683;return out;}
  out.issue=shared::instance(memory,out.layout,p,1679,out.self_symbol);if(out.issue!=rs::Issue::none){out.failed_symbol=1679;return out;}
  out.hero=out.hero_symbol.offset;out.self=out.self_symbol.offset;
  out.issue=rs::detail::stable_layout(memory,parser,p,out.layout);if(out.issue!=rs::Issue::none)return out;
  out.read_valid=true;out.matches=out.hero==hero&&out.self==hero;return out;
}
}
struct Knowledge:IntegerEvidence<1> {bool known=false;};
inline Knowledge knowledge(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& a) noexcept {
  Knowledge out;static_cast<IntegerEvidence<1>&>(out)=detail::integers(memory,parser,a,std::array<std::uint32_t,1>{{11198}});
  out.known=out.read_valid&&out.readings[0].value==1;return out;
}
struct BranchEvidence:IntegerEvidence<1> {cat::Branch branch=cat::Branch::unknown;std::int32_t value=0;};
inline BranchEvidence branch(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& a) noexcept {
  BranchEvidence out;static_cast<IntegerEvidence<1>&>(out)=detail::integers(memory,parser,a,std::array<std::uint32_t,1>{{11967}});
  if(out.read_valid){out.value=out.readings[0].value;out.branch=cat::branch_for(true,out.value);}return out;
}
inline bool same_branch_evidence(const BranchEvidence& a,const BranchEvidence& b) noexcept {
  return same_integer_evidence(a,b)&&a.value==b.value&&a.branch==b.branch;
}
struct ContextEvidence:IntegerEvidence<2> {
  std::int32_t production=-1,special=-1;
  bool station_matches() const noexcept {return read_valid&&production==4&&(special==0||special==1);}
  bool owner_matches() const noexcept {return station_matches()&&special==1;}
};
inline ContextEvidence context(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& a) noexcept {
  ContextEvidence out;static_cast<IntegerEvidence<2>&>(out)=detail::integers(memory,parser,a,std::array<std::uint32_t,2>{{12330,20361}});
  if(out.read_valid){out.production=out.readings[0].value;out.special=out.readings[1].value;}return out;
}
inline bool same_context_evidence(const ContextEvidence& a,const ContextEvidence& b) noexcept {
  return same_integer_evidence(a,b)&&a.production==b.production&&a.special==b.special;
}
inline bool same_actor_evidence(const ActorEvidence& a,const ActorEvidence& b) noexcept {return station_recipe_runtime::same_actor_evidence(a,b);}
// Success uses HERO, while the unchanged deficit refund uses SELF.
inline ActorEvidence actor(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& a,std::uint32_t hero) noexcept {
  auto before=detail::actor_once(memory,parser,a,hero);if(!before.read_valid)return before;
  const auto after=detail::actor_once(memory,parser,a,hero);if(!after.read_valid)return after;
  if(!spirit_recipe_runtime::same_actor_evidence(before,after)){before.read_valid=before.matches=false;before.issue=rs::Issue::changed_during_read;}
  return before;
}
} // namespace ubc::spirit_recipe_runtime
