// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/alchemy/alchemy_extra_navigation.hpp"
#include "ubc/stations/alchemy/alchemy_extra_snapshot.hpp"
#include "ubc/stations/alchemy/alchemy_flask_runtime.hpp"
#include "ubc/core/selection_snapshot.hpp"

namespace ubc::alchemy_extra_execution {
namespace catalog=alchemy_recipe_catalog;
namespace nav=alchemy_extra_navigation;
namespace script=runtime_script;
inline constexpr std::array<lhiver::SymbolBinding,4> print_bindings{{
  {"PRINT_ALCHEMYSUCCESS",14372,3,1,1,-1},
  {"PRINT_PRODITEMSMISSING",14374,3,1,1,-1},
  {"PRINT_JOINTSUCCESS",14376,3,1,1,-1},
  {"PRINT_ADDON_JOINT_01_SUCCESS",14377,3,1,1,-1},
}};

// Names come only from the immutable own catalogs. The view is
// bound to this object during inspection; no borrowed DAT bytes.
class Profile {
public:
  bool valid() const noexcept {return valid_ && function_count_ && binding_count_;}
  bool add(const lhiver::Function* value) noexcept {
    if(!value) {valid_=false;return false;}
    for(std::size_t i=0;i<function_count_;++i)if(functions_[i].symbol==value->symbol) {
      if(!nav::base::detail::same_function(&functions_[i],value))valid_=false;
      return valid_;
    }
    if(function_count_==functions_.size()) {valid_=false;return false;}
    functions_[function_count_++]=*value;return valid_;
  }
  bool add(const lhiver::SymbolBinding& value) noexcept {
    for(std::size_t i=0;i<binding_count_;++i)if(bindings_[i].symbol==value.symbol) {
      const auto& a=bindings_[i];
      if(!a.name || !value.name || std::strcmp(a.name,value.name) || a.type!=value.type || a.flags!=value.flags ||
         a.elements!=value.elements || a.parent_index!=value.parent_index || a.check_value!=value.check_value ||
         a.value!=value.value || a.check_offset!=value.check_offset || a.offset!=value.offset)valid_=false;
      return valid_;
    }
    if(binding_count_==bindings_.size()) {valid_=false;return false;}
    bindings_[binding_count_++]=value;return valid_;
  }
  void reject() noexcept {valid_=false;}
  lhiver::Profile view() const & noexcept {
    if(!valid())return {};
    return {lhiver::game_parser,lhiver::symbol_count,lhiver::code_length,lhiver::code_sha256,
      functions_.data(),function_count_,bindings_.data(),binding_count_};
  }
  lhiver::Profile view() const &&=delete;
private:
  bool valid_=true;
  std::size_t function_count_=0,binding_count_=0;
  std::array<lhiver::Function,64> functions_{};
  std::array<lhiver::SymbolBinding,64> bindings_{};
};
inline void common(Profile& profile) noexcept {
  for(const auto& b:catalog::common_bindings)profile.add(b);
  for(const auto& b:nav::base::flag_bindings)profile.add(b);
}
inline void owner(Profile& profile,const recipe_catalog::Owner& value) noexcept {
  profile.add(value.constructor);profile.add(value.condition);profile.add(value.information);
}
inline Profile recipe_profile(const catalog::Descriptor& recipe) noexcept {
  Profile result;
  if(!catalog::structurally_valid(recipe)) {result.reject();return result;}
  common(result);for(const auto& binding:recipe.bindings)result.add(binding);
  owner(result,recipe.owner);result.add(recipe.station_entry);result.add(recipe.station_end);
  result.add(recipe.end_production);result.add(recipe.say);
  for(const auto* f:recipe.item_constructors)result.add(f);
  for(const auto& p:recipe.prints) {
    bool found=false;for(const auto& b:print_bindings)if(b.symbol==p.symbol){result.add(b);found=true;}
    if(!found)result.reject();
  }
  return result;
}
inline Profile step_profile(const nav::Plan& plan,std::size_t index) noexcept {
  Profile result;
  if(!nav::structurally_valid(plan) || index>=plan.count) {result.reject();return result;}
  const auto& step=plan.steps[index];
  if(step.is_effect())return recipe_profile(*catalog::find(plan.target));
  common(result);const auto* bound=nav::owner_for(step.owner_instance);
  if(!bound) {result.reject();return result;}
  owner(result,*bound);result.add(nav::function_for(static_cast<std::uint32_t>(step.function_symbol)));
  // A pure BACK has one of the six existing potion owners. Its condition
  // reads real learned knowledge: include profile metadata, never set.
  if(const auto* route=nav::base::route_for(step.before)) {
    const auto* recipe=recipe_catalog::find(route->recipe);
    if(!recipe) {result.reject();return result;}
    for(const auto& b:recipe->bindings)result.add(b);
  }
  return result;
}
struct Verification {
  bool valid=false;
  script::RuntimeContract local{};
  alchemy_flask_runtime::Issue string_issue=alchemy_flask_runtime::Issue::none;
  std::uint32_t failed_symbol=0;
};
// Local bodies/signatures/bindings and the countless print texts.
// No full-DAT trust and no release of arbitrary native dependencies.
inline Verification verify_recipe(const ReadOnlyMemory& memory,const catalog::Descriptor& recipe) noexcept {
  Verification result;const auto assembled=recipe_profile(recipe);
  if(!assembled.valid()) {result.local.issue=script::Issue::invalid_profile;return result;}
  const auto profile=assembled.view();
  for(unsigned pass=0;pass<2;++pass) {
    result.local=script::verify_local_contract(memory,lhiver::game_parser,profile);
    if(!result.local.matches)return result;
    script::detail::Layout layout{};
    result.local.issue=script::detail::read_layout(memory,lhiver::game_parser,profile,layout);
    if(result.local.issue!=script::Issue::none) {result.local.matches=false;return result;}
    for(const auto& p:recipe.prints) {
      const auto read=alchemy_flask_runtime::detail::string_once(memory,layout,{p.symbol,p.length,p.digest});
      if(!read.valid) {result.string_issue=read.issue;result.failed_symbol=p.symbol;return result;}
    }
    result.local.issue=script::detail::stable_layout(memory,lhiver::game_parser,profile,layout);
    if(result.local.issue!=script::Issue::none) {result.local.matches=false;return result;}
  }
  result.valid=true;return result;
}
inline script::RuntimeContract verify_step(const ReadOnlyMemory& memory,const nav::Plan& plan,std::size_t index) noexcept {
  const auto assembled=step_profile(plan,index);
  if(!assembled.valid()) {script::RuntimeContract out;out.issue=script::Issue::invalid_profile;return out;}
  return script::verify_local_contract(memory,lhiver::game_parser,assembled.view());
}
inline nav::Page read_page(const ReadOnlyMemory& memory) noexcept {
  nav::PageState state;state.invocation=recipe_catalog::Invocation::info;
  std::array<std::int32_t*,6> fields{{&state.production,&state.health,&state.mana,&state.booze,&state.tobacco,&state.special}};
  std::array<std::uint32_t,6> symbols{{12330,20330,20299,20280,20386,20361}};
  for(unsigned pass=0;pass<2;++pass)for(std::size_t i=0;i<fields.size();++i) {
    const auto value=script::symbol_int(memory,lhiver::game_parser,symbols[i]);
    if(!value.valid || (pass && *fields[i]!=value.value))return nav::Page::unknown;
    *fields[i]=value.value;
  }
  state.known=true;return nav::classify_page(state);
}
inline bool info_matches(const SelectionInfo& actual,const recipe_catalog::Owner& expected) noexcept {
  return actual.valid && actual.npc==expected.npc && actual.number==expected.number &&
    actual.important==static_cast<int>(expected.important) && actual.condition==static_cast<std::int32_t>(expected.condition->symbol) &&
    actual.function==static_cast<std::int32_t>(expected.information->symbol) && actual.trade==static_cast<int>(expected.trade) &&
    actual.permanent==static_cast<int>(expected.permanent) && actual.instance==expected.instance;
}
struct Entry {
  bool valid=false;
  Verification contract{};
  AlchemyExtraKnowledge knowledge{};
  nav::Page page=nav::Page::unknown;
  std::uint32_t code_address=0;
};
inline Entry verify_entry(const ReadOnlyMemory& memory,const catalog::Descriptor& recipe,std::uint32_t actual_info) noexcept {
  Entry result;result.contract=verify_recipe(memory,recipe);
  if(!result.contract.valid || !actual_info)return result;
  for(unsigned pass=0;pass<2;++pass) {
    const auto info=read_selection_info(memory,actual_info);
    if(!info_matches(info,recipe.owner) || info.first_choice)return result;
    result.page=read_page(memory);
    if(result.page!=nav::target_page(recipe.category))return result;
    result.knowledge=read_alchemy_extra_knowledge(memory,lhiver::game_parser,recipe);
    bool learned=false,conditions=false,active=false;
    if(!alchemy_extra_detail::knowledge(recipe,result.knowledge,learned,conditions,active) || !learned || !conditions || !active)return result;
  }
  script::detail::Layout layout{};const auto assembled=recipe_profile(recipe);
  if(script::detail::read_layout(memory,lhiver::game_parser,assembled.view(),layout)!=script::Issue::none ||
     recipe.callback.function->byte_offset>UINT32_MAX-layout.code)return result;
  result.code_address=layout.code+recipe.callback.function->byte_offset;result.valid=true;return result;
}
} // namespace ubc::alchemy_extra_execution
