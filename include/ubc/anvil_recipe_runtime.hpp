// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/anvil_recipe_catalog.hpp"
#include "ubc/station_recipe_runtime.hpp"

namespace ubc::anvil_recipe_runtime {
namespace cat=anvil_recipe_catalog;
namespace rs=runtime_script;
namespace shared=station_recipe_runtime::detail;
using AssemblyIssue=station_recipe_runtime::AssemblyIssue;
using Knowledge=station_recipe_runtime::Knowledge;
using ActorEvidence=station_recipe_runtime::ActorEvidence;
inline constexpr std::size_t profile_capacity=64;

class ProfileAssembly;
inline ProfileAssembly assemble(const cat::Descriptor&) noexcept;

// Own tables and names; profile() is only valid during the lifetime
// of this unchanged assembly. No new full-code or runtime proof.
class ProfileAssembly {
public:
  ProfileAssembly() noexcept=default;
  ProfileAssembly(const ProfileAssembly& other) noexcept {*this=other;}
  ProfileAssembly& operator=(const ProfileAssembly& other) noexcept {
    if(this!=&other) {
      issue_=other.issue_;functions_=other.functions_;bindings_=other.bindings_;
      function_names_=other.function_names_;binding_names_=other.binding_names_;
      knowledge_=other.knowledge_;nf_=other.nf_;nb_=other.nb_;nk_=other.nk_;
      for(std::size_t i=0;i<nf_;++i)functions_[i].name=function_names_[i].data();
      for(std::size_t i=0;i<nb_;++i)bindings_[i].name=binding_names_[i].data();
    }
    return *this;
  }
  bool valid() const noexcept {return issue_==AssemblyIssue::none;}
  AssemblyIssue issue() const noexcept {return issue_;}
  std::size_t function_count() const noexcept {return nf_;}
  std::size_t binding_count() const noexcept {return nb_;}
  std::size_t knowledge_count() const noexcept {return nk_;}
  const cat::Requirement* knowledge_requirement(std::size_t i) const noexcept {return valid()&&i<nk_?&knowledge_[i]:nullptr;}
  const lhiver::SymbolBinding* binding(std::uint32_t symbol) const noexcept {
    if(valid())for(std::size_t i=0;i<nb_;++i)if(bindings_[i].symbol==symbol)return &bindings_[i];
    return nullptr;
  }
  lhiver::Profile profile() const & noexcept {
    if(!valid())return {};
    return {lhiver::profile.parser_address,lhiver::profile.symbol_count,lhiver::profile.code_length,
      lhiver::profile.code_sha256,functions_.data(),nf_,bindings_.data(),nb_};
  }
  lhiver::Profile profile() const &&=delete;
private:
  friend ProfileAssembly assemble(const cat::Descriptor&) noexcept;
  bool reject(AssemblyIssue issue) noexcept {issue_=issue;return false;}
  bool add(const lhiver::Function& f) noexcept {
    const auto length=shared::name_length(f.name);
    if(!length||length>63)return reject(AssemblyIssue::function_contract);
    for(std::size_t i=0;i<nf_;++i)if(functions_[i].symbol==f.symbol)
      return shared::same_function(functions_[i],f)?true:reject(AssemblyIssue::function_conflict);
    if(nf_==profile_capacity)return reject(AssemblyIssue::capacity);
    functions_[nf_]=f;std::memcpy(function_names_[nf_].data(),f.name,length+1);
    functions_[nf_].name=function_names_[nf_].data();++nf_;return true;
  }
  bool add(const lhiver::SymbolBinding& b) noexcept {
    const auto length=shared::name_length(b.name);
    if(!length||length>63)return reject(AssemblyIssue::binding_contract);
    for(std::size_t i=0;i<nb_;++i)if(bindings_[i].symbol==b.symbol)
      return shared::same_binding(bindings_[i],b)?true:reject(AssemblyIssue::binding_conflict);
    if(nb_==profile_capacity)return reject(AssemblyIssue::capacity);
    bindings_[nb_]=b;std::memcpy(binding_names_[nb_].data(),b.name,length+1);
    bindings_[nb_].name=binding_names_[nb_].data();++nb_;return true;
  }
  bool add_owner(const cat::Owner& owner) noexcept {
    if(!owner.constructor||!owner.condition||!owner.information)return reject(AssemblyIssue::function_contract);
    return add(*owner.constructor)&&add(*owner.condition)&&add(*owner.information);
  }
  bool add_knowledge(const cat::Requirement& requirement) noexcept {
    if(requirement.role!=cat::RequirementRole::knowledge)return true;
    const auto* b=requirement.symbol>0?binding(static_cast<std::uint32_t>(requirement.symbol)):nullptr;
    if(requirement.kind!=cat::RequirementKind::integer_equal||!b||b->type!=2||b->flags>1||
       b->parent_index!=-1||!b->elements||requirement.element>=b->elements)
      return reject(AssemblyIssue::knowledge_contract);
    for(std::size_t i=0;i<nk_;++i)if(knowledge_[i].symbol==requirement.symbol&&knowledge_[i].element==requirement.element)
      return knowledge_[i].value==requirement.value?true:reject(AssemblyIssue::knowledge_contract);
    if(nk_==profile_capacity)return reject(AssemblyIssue::capacity);
    knowledge_[nk_++]=requirement;return true;
  }
  AssemblyIssue issue_=AssemblyIssue::descriptor;
  std::size_t nf_=0,nb_=0,nk_=0;
  std::array<lhiver::Function,profile_capacity> functions_{};
  std::array<lhiver::SymbolBinding,profile_capacity> bindings_{};
  std::array<std::array<char,64>,profile_capacity> function_names_{},binding_names_{};
  std::array<cat::Requirement,profile_capacity> knowledge_{};
};

inline ProfileAssembly assemble(const cat::Descriptor& descriptor) noexcept {
  ProfileAssembly out;
  if(!cat::structurally_valid(descriptor))return out;
  out.issue_=AssemblyIssue::none;
  for(const auto& b:descriptor.bindings)if(!out.add(b))return out;
  if(!out.add_owner(descriptor.owner)||!out.add_owner(descriptor.station_profile->end))return out;
  for(const auto& nav:cat::navigation)if(!out.add_owner(nav.owner))return out;
  for(const auto* f:{descriptor.back,descriptor.callback.function,descriptor.original_three.function,
      descriptor.station_profile->entry,descriptor.station_profile->mob_condition,
      cat::function_for(14646),cat::function_for(19965)})
    if(!f){out.reject(AssemblyIssue::function_contract);return out;}else if(!out.add(*f))return out;
  for(const auto& check:descriptor.checks) {
    if(!check.function){out.reject(AssemblyIssue::function_contract);return out;}
    if(!out.add(*check.function))return out;
  }
  for(const auto* f:descriptor.item_constructors) {
    if(!f){out.reject(AssemblyIssue::function_contract);return out;}
    if(!out.add(*f))return out;
  }
  for(const auto& requirement:descriptor.requirements)if(!out.add_knowledge(requirement))return out;
  for(const auto& requirement:descriptor.owner.conditions)if(!out.add_knowledge(requirement))return out;
  // Same symbol index must also match between function/binding tables.
  // C_INFO instances have a dynamic +0x1C; the existing profile check
  // handles this as instance address, never as invented VOID return value.
  if(!rs::detail::valid_profile(out.profile()))out.reject(AssemblyIssue::descriptor);
  return out;
}
inline rs::RuntimeContract verify(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& assembly) noexcept {
  return assembly.valid()?rs::verify_local_contract(memory,parser,assembly.profile()):rs::RuntimeContract{false,rs::Issue::invalid_profile,0};
}
inline rs::RuntimeContract verify(const ReadOnlyMemory& memory,std::uint32_t parser,const cat::Descriptor& recipe) noexcept {
  const auto assembly=assemble(recipe);return verify(memory,parser,assembly);
}

// Exclusively real learning predicates; categories, minimum raw steel and
// STARTSMITHING are not reinterpreted as knowledge. No condition to execute.
inline Knowledge knowledge(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& assembly) noexcept {
  Knowledge out;if(!assembly.valid()){out.issue=rs::Issue::invalid_profile;return out;}
  const auto p=assembly.profile();rs::detail::Layout layout{};
  out.issue=rs::detail::read_layout(memory,parser,p,layout);if(out.issue!=rs::Issue::none)return out;
  std::array<shared::IntegerRead,profile_capacity> before{};bool learned=true;
  for(std::size_t i=0;i<assembly.knowledge_count();++i) {
    const auto& r=*assembly.knowledge_requirement(i);const auto* b=assembly.binding(static_cast<std::uint32_t>(r.symbol));
    before[i]=shared::read_integer(memory,layout,*b,r.element);
    if(before[i].issue!=rs::Issue::none){out.issue=before[i].issue;out.failed_symbol=b->symbol;return out;}
    ++out.checked;learned=learned&&before[i].value==r.value;
  }
  for(std::size_t i=0;i<assembly.knowledge_count();++i) {
    const auto& r=*assembly.knowledge_requirement(i);const auto* b=assembly.binding(static_cast<std::uint32_t>(r.symbol));
    const auto after=shared::read_integer(memory,layout,*b,r.element);
    if(after.issue!=rs::Issue::none||after.value!=before[i].value||!shared::same_symbol(after.symbol,before[i].symbol)) {
      out.issue=after.issue==rs::Issue::none?rs::Issue::changed_during_read:after.issue;out.failed_symbol=b->symbol;return out;
    }
  }
  out.issue=rs::detail::stable_layout(memory,parser,p,layout);if(out.issue!=rs::Issue::none)return out;
  out.read_valid=true;out.known=learned;return out;
}
inline Knowledge knowledge(const ReadOnlyMemory& memory,std::uint32_t parser,const cat::Descriptor& recipe) noexcept {
  const auto assembly=assemble(recipe);return knowledge(memory,parser,assembly);
}
inline bool same_actor_evidence(const ActorEvidence& a,const ActorEvidence& b) noexcept {
  return station_recipe_runtime::same_actor_evidence(a,b);
}
namespace detail {
inline ActorEvidence actor_once(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& assembly,std::uint32_t hero) noexcept {
  ActorEvidence out;out.requires_self=true;
  if(!assembly.valid()||!hero){out.issue=rs::Issue::invalid_profile;return out;}
  const auto p=assembly.profile();out.issue=rs::detail::read_layout(memory,parser,p,out.layout);if(out.issue!=rs::Issue::none)return out;
  out.issue=shared::instance(memory,out.layout,p,1683,out.hero_symbol);
  if(out.issue!=rs::Issue::none){out.failed_symbol=1683;return out;}
  out.issue=shared::instance(memory,out.layout,p,1679,out.self_symbol);
  if(out.issue!=rs::Issue::none){out.failed_symbol=1679;return out;}
  out.hero=out.hero_symbol.offset;out.self=out.self_symbol.offset;
  out.issue=rs::detail::stable_layout(memory,parser,p,out.layout);if(out.issue!=rs::Issue::none)return out;
  out.read_valid=true;out.matches=out.hero==hero&&out.self==hero;return out;
}
}
// The minimum test runs on SELF, the effects on HERO. Therefore SELF==HERO
// is mandatory here, even though material_actor in the descriptor is HERO.
inline ActorEvidence actor(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& assembly,std::uint32_t hero) noexcept {
  auto before=detail::actor_once(memory,parser,assembly,hero);if(!before.read_valid)return before;
  const auto after=detail::actor_once(memory,parser,assembly,hero);if(!after.read_valid)return after;
  if(!anvil_recipe_runtime::same_actor_evidence(before,after)){before.read_valid=before.matches=false;before.issue=rs::Issue::changed_during_read;}
  return before;
}

struct ContextEvidence {
  bool read_valid=false;
  std::int32_t production=-1,start_smithing=-1,normal_weapons=-1,ore_weapons=-1;
  rs::Issue issue=rs::Issue::none;std::uint32_t failed_symbol=0;
  rs::detail::Layout layout{};
  std::array<shared::IntegerRead,4> readings{};
  bool station_matches() const noexcept {
    return read_valid&&production==1&&start_smithing==1&&
      (normal_weapons==0||normal_weapons==1)&&(ore_weapons==0||ore_weapons==1)&&!(normal_weapons&&ore_weapons);
  }
};
inline bool same_context_evidence(const ContextEvidence& a,const ContextEvidence& b) noexcept {
  if(!a.read_valid||!b.read_valid||!rs::detail::same_layout(a.layout,b.layout))return false;
  for(std::size_t i=0;i<a.readings.size();++i)
    if(a.readings[i].value!=b.readings[i].value||!shared::same_symbol(a.readings[i].symbol,b.readings[i].symbol))return false;
  return a.production==b.production&&a.start_smithing==b.start_smithing&&a.normal_weapons==b.normal_weapons&&a.ore_weapons==b.ore_weapons;
}
namespace detail {
inline ContextEvidence context_once(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& assembly) noexcept {
  ContextEvidence out;if(!assembly.valid()){out.issue=rs::Issue::invalid_profile;return out;}
  const auto p=assembly.profile();out.issue=rs::detail::read_layout(memory,parser,p,out.layout);if(out.issue!=rs::Issue::none)return out;
  constexpr std::array<std::uint32_t,4> ids{{12330,10536,10552,10551}};
  for(std::size_t i=0;i<ids.size();++i) {
    const auto* b=assembly.binding(ids[i]);if(!b){out.issue=rs::Issue::invalid_profile;return out;}
    out.readings[i]=shared::read_integer(memory,out.layout,*b,0);
    if(out.readings[i].issue!=rs::Issue::none){out.issue=out.readings[i].issue;out.failed_symbol=ids[i];return out;}
  }
  out.issue=rs::detail::stable_layout(memory,parser,p,out.layout);if(out.issue!=rs::Issue::none)return out;
  out.production=out.readings[0].value;out.start_smithing=out.readings[1].value;
  out.normal_weapons=out.readings[2].value;out.ore_weapons=out.readings[3].value;out.read_valid=true;return out;
}
}
inline ContextEvidence context(const ReadOnlyMemory& memory,std::uint32_t parser,const ProfileAssembly& assembly) noexcept {
  auto before=detail::context_once(memory,parser,assembly);if(!before.read_valid)return before;
  const auto after=detail::context_once(memory,parser,assembly);if(!after.read_valid)return after;
  if(!same_context_evidence(before,after)){before.read_valid=false;before.issue=rs::Issue::changed_during_read;}
  return before;
}
} // namespace ubc::anvil_recipe_runtime
