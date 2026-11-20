// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/cool_recipe_catalog.hpp"
#include "ubc/stations/station_recipe_runtime.hpp"
#include "ubc/stations/alchemy/alchemy_flask_runtime.hpp"

namespace ubc::cool_recipe_runtime {
namespace cat=cool_recipe_catalog;namespace rs=runtime_script;namespace shared=station_recipe_runtime::detail;
using Knowledge=station_recipe_runtime::Knowledge;using ActorEvidence=station_recipe_runtime::ActorEvidence;
// A single canonical family: Views point exclusively to own
// immutable static tables, never into a foreign descriptor.
class ProfileAssembly {
public:
  bool valid() const noexcept{return valid_;}
  std::size_t function_count() const noexcept{return valid_?cat::functions.size():0;}
  std::size_t binding_count() const noexcept{return valid_?cat::bindings.size():0;}
  lhiver::Profile profile() const noexcept{return valid_?lhiver::Profile{lhiver::profile.parser_address,lhiver::profile.symbol_count,lhiver::profile.code_length,lhiver::profile.code_sha256,cat::functions.data(),cat::functions.size(),cat::bindings.data(),cat::bindings.size()}:lhiver::Profile{};}
  const lhiver::SymbolBinding* binding(std::uint32_t id) const noexcept {if(valid_)for(const auto& b:cat::bindings)if(b.symbol==id)return &b;return nullptr;}
private:
  friend ProfileAssembly assemble(const cat::Descriptor&) noexcept;bool valid_=false;
};
inline ProfileAssembly assemble(const cat::Descriptor& d) noexcept {
  ProfileAssembly a;if(!cat::structurally_valid(d))return a;a.valid_=true;
  if(cat::functions.size()>64||cat::bindings.size()>64||!rs::detail::valid_profile(a.profile()))a.valid_=false;return a;
}
inline rs::RuntimeContract verify(const ReadOnlyMemory& m,std::uint32_t parser,const ProfileAssembly& a) noexcept {
  return a.valid()?rs::verify_local_contract(m,parser,a.profile()):rs::RuntimeContract{false,rs::Issue::invalid_profile,0};
}
struct Verification {bool valid=false;rs::RuntimeContract local{};alchemy_flask_runtime::Issue string_issue=alchemy_flask_runtime::Issue::none;std::uint32_t failed_symbol=0;};
inline Verification verify_recipe(const ReadOnlyMemory& m,std::uint32_t parser,const cat::Descriptor& d) noexcept {
  Verification out;const auto a=assemble(d);if(!a.valid()){out.local.issue=rs::Issue::invalid_profile;return out;}
  const auto p=a.profile();rs::detail::Layout layout{};out.local.issue=rs::detail::read_layout(m,parser,p,layout);if(out.local.issue!=rs::Issue::none)return out;
  for(unsigned pass=0;pass<2;++pass){out.local=verify(m,parser,a);if(!out.local.matches)return out;
    // Symbol index/STRING metadata/content are bound. Anonymous DAT
    // names are not fabricated proof for same-named runtime symbols.
    for(const auto& text:cat::texts){const auto r=alchemy_flask_runtime::detail::string_once(m,layout,{text.symbol,text.length,text.sha256});if(!r.valid){out.string_issue=r.issue;out.failed_symbol=text.symbol;return out;}}
    out.local.issue=rs::detail::stable_layout(m,parser,p,layout);if(out.local.issue!=rs::Issue::none){out.local.matches=false;return out;}}
  out.valid=true;return out;
}
// In these three original conditions there is no learn filter. An empty
// knowledge predicate is neither context, material, nor code release.
inline Knowledge knowledge(const ReadOnlyMemory& m,std::uint32_t parser,const ProfileAssembly& a) noexcept {
  Knowledge k;if(!a.valid()){k.issue=rs::Issue::invalid_profile;return k;}rs::detail::Layout l{};const auto p=a.profile();
  k.issue=rs::detail::read_layout(m,parser,p,l);if(k.issue!=rs::Issue::none)return k;k.issue=rs::detail::stable_layout(m,parser,p,l);
  k.read_valid=k.known=k.issue==rs::Issue::none;return k;
}
inline bool same_actor_evidence(const ActorEvidence& a,const ActorEvidence& b) noexcept{return station_recipe_runtime::same_actor_evidence(a,b);}
namespace detail {
inline ActorEvidence actor_once(const ReadOnlyMemory& m,std::uint32_t parser,const ProfileAssembly& a,std::uint32_t hero) noexcept {
  ActorEvidence r;r.requires_self=true;if(!a.valid()||!hero){r.issue=rs::Issue::invalid_profile;return r;}const auto p=a.profile();
  r.issue=rs::detail::read_layout(m,parser,p,r.layout);if(r.issue!=rs::Issue::none)return r;
  r.issue=shared::instance(m,r.layout,p,1683,r.hero_symbol);if(r.issue!=rs::Issue::none){r.failed_symbol=1683;return r;}
  r.issue=shared::instance(m,r.layout,p,1679,r.self_symbol);if(r.issue!=rs::Issue::none){r.failed_symbol=1679;return r;}
  r.hero=r.hero_symbol.offset;r.self=r.self_symbol.offset;r.issue=rs::detail::stable_layout(m,parser,p,r.layout);
  r.read_valid=r.issue==rs::Issue::none;r.matches=r.read_valid&&r.hero==hero&&r.self==hero;return r;
}
}
inline ActorEvidence actor(const ReadOnlyMemory& m,std::uint32_t parser,const ProfileAssembly& a,std::uint32_t hero) noexcept {
  auto r=detail::actor_once(m,parser,a,hero);if(!r.read_valid)return r;const auto next=detail::actor_once(m,parser,a,hero);if(!next.read_valid)return next;
  if(!cool_recipe_runtime::same_actor_evidence(r,next)){r.read_valid=r.matches=false;r.issue=rs::Issue::changed_during_read;}return r;
}
struct ContextEvidence {
  bool read_valid=false;std::int32_t production=-1,start_smithing=-1;
  rs::Issue issue=rs::Issue::none;std::uint32_t failed_symbol=0;rs::detail::Layout layout{};std::array<shared::IntegerRead,2> readings{};
  bool station_matches() const noexcept{return read_valid&&production==13&&start_smithing==1;}
};
inline bool same_context_evidence(const ContextEvidence& a,const ContextEvidence& b) noexcept {
  if(!a.read_valid||!b.read_valid||!rs::detail::same_layout(a.layout,b.layout))return false;
  for(std::size_t i=0;i<2;++i)if(a.readings[i].value!=b.readings[i].value||!shared::same_symbol(a.readings[i].symbol,b.readings[i].symbol))return false;
  return a.production==b.production&&a.start_smithing==b.start_smithing;
}
namespace detail {
inline ContextEvidence context_once(const ReadOnlyMemory& m,std::uint32_t parser,const ProfileAssembly& a) noexcept {
  ContextEvidence r;if(!a.valid()){r.issue=rs::Issue::invalid_profile;return r;}const auto p=a.profile();r.issue=rs::detail::read_layout(m,parser,p,r.layout);if(r.issue!=rs::Issue::none)return r;
  constexpr std::array<std::uint32_t,2> ids{{12330,10536}};
  for(std::size_t i=0;i<2;++i){const auto* b=a.binding(ids[i]);if(!b){r.issue=rs::Issue::invalid_profile;return r;}r.readings[i]=shared::read_integer(m,r.layout,*b,0);if(r.readings[i].issue!=rs::Issue::none){r.issue=r.readings[i].issue;r.failed_symbol=ids[i];return r;}}
  r.issue=rs::detail::stable_layout(m,parser,p,r.layout);if(r.issue!=rs::Issue::none)return r;r.production=r.readings[0].value;r.start_smithing=r.readings[1].value;r.read_valid=true;return r;
}
}
inline ContextEvidence context(const ReadOnlyMemory& m,std::uint32_t parser,const ProfileAssembly& a) noexcept {
  auto r=detail::context_once(m,parser,a);if(!r.read_valid)return r;const auto next=detail::context_once(m,parser,a);if(!next.read_valid)return next;
  if(!same_context_evidence(r,next)){r.read_valid=false;r.issue=rs::Issue::changed_during_read;}return r;
}
} // namespace ubc::cool_recipe_runtime
