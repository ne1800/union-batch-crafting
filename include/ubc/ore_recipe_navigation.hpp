// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/ore_recipe_catalog.hpp"
#include "ubc/anvil_recipe_navigation.hpp"

namespace ubc::ore_recipe_navigation {
namespace cat=ore_recipe_catalog;
using cat::RecipeId;using cat::Invocation;
using anvil_recipe_navigation::Page;using anvil_recipe_navigation::PageState;
using anvil_recipe_navigation::classify_page;using anvil_recipe_navigation::supported_page;
using anvil_recipe_navigation::Kind;using anvil_recipe_navigation::Step;
inline constexpr auto pages=anvil_recipe_navigation::pages;
inline constexpr bool runtime_ready=false;
inline constexpr std::size_t maximum_steps=4;
enum class Issue {none,unknown_page,unsupported_recipe,quantity,contract};
struct Plan {
  Issue issue=Issue::contract;Page source=Page::unknown;
  RecipeId target=static_cast<RecipeId>(0);std::uint32_t quantity=0;
  std::array<Step,maximum_steps> steps{};std::size_t count=0;
  bool valid() const noexcept {return issue==Issue::none&&count&&count<=maximum_steps;}
  const Step* effect() const noexcept {return valid()&&steps[count-1].is_effect()?&steps[count-1]:nullptr;}
};
inline bool structurally_valid() noexcept {
  if(!anvil_recipe_navigation::structurally_valid())return false;
  for(const auto& r:cat::recipes)
    if(!cat::structurally_valid(r)||r.callback.invocation!=Invocation::info||
      r.callback.owner_instance!=r.owner.instance||r.callback.function!=r.owner.information)return false;
  const auto& back=cat::navigation[2];const auto& open=cat::navigation[1];
  return back.owner.instance==20707&&back.owner.information->symbol==20709&&back.flag==10552&&
    back.before==cat::Page::common&&back.after==cat::Page::root&&back.before_value==1&&back.after_value==0&&
    open.owner.instance==20704&&open.owner.information->symbol==20706&&open.flag==10551&&
    open.before==cat::Page::root&&open.after==cat::Page::ore&&open.before_value==0&&open.after_value==1;
}
// Only value planning. Fresh source/owner/learn/hand/collection proofs
// remain necessary before every stage; an open menu proves no original call.
inline Plan make_plan(Page source,RecipeId target,std::uint32_t quantity) noexcept {
  Plan out;out.source=source;out.target=target;out.quantity=quantity;
  if(!supported_page(source)){out.issue=Issue::unknown_page;return out;}
  const auto* r=cat::find(target);if(!r){out.issue=Issue::unsupported_recipe;return out;}
  if(!cat::plan_quantity(*r,quantity).valid){out.issue=Issue::quantity;return out;}
  if(!structurally_valid())return out;
  auto current=source;
  const auto add=[&](Kind kind,Invocation invocation,std::int32_t owner,std::uint32_t function,Page after) noexcept {
    if(out.count==maximum_steps)return false;
    out.steps[out.count++]={kind,invocation,owner,static_cast<std::int32_t>(function),current,after};current=after;return true;
  };
  if(const auto* from=anvil_recipe_navigation::recipe_for(current))
    if(!add(Kind::back,Invocation::choice,from->owner.instance,from->back->symbol,Page::common))return Plan{};
  if(current==Page::common){const auto& back=cat::navigation[2];
    if(!add(Kind::back,Invocation::info,back.owner.instance,back.owner.information->symbol,Page::root))return Plan{};}
  if(current==Page::root){const auto& open=cat::navigation[1];
    if(!add(Kind::open_owner,Invocation::info,open.owner.instance,open.owner.information->symbol,Page::ore))return Plan{};}
  if(current!=Page::ore||!add(Kind::effect,Invocation::info,r->owner.instance,r->callback.function->symbol,Page::unknown))return Plan{};
  out.issue=Issue::none;return out;
}
inline bool structurally_valid(const Plan& value) noexcept {
  if(!value.valid())return false;
  const auto expected=make_plan(value.source,value.target,value.quantity);
  if(!expected.valid()||expected.count!=value.count)return false;
  for(std::size_t i=0;i<value.count;++i){const auto& a=value.steps[i];const auto& b=expected.steps[i];
    if(a.kind!=b.kind||a.invocation!=b.invocation||a.owner_instance!=b.owner_instance||a.function_symbol!=b.function_symbol||
      a.before!=b.before||a.after!=b.after)return false;}
  return true;
}
} // namespace ubc::ore_recipe_navigation
