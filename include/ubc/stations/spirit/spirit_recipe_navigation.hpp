// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/spirit/spirit_recipe_runtime.hpp"

namespace ubc::spirit_recipe_navigation {
namespace cat=spirit_recipe_catalog;namespace shared=alchemy_extra_navigation;
using shared::Page;using shared::PageState;using shared::Step;using shared::Kind;using shared::Invocation;using shared::Issue;
using shared::classify_page;using shared::supported_page;
inline constexpr bool runtime_ready=false;
inline constexpr std::size_t maximum_steps=4;
inline const recipe_catalog::Owner* owner_for(std::int32_t symbol) noexcept {
  return symbol==cat::owner.instance?&cat::owner:shared::owner_for(symbol);
}
inline const lhiver::Function* function_for(std::uint32_t symbol) noexcept {
  if(const auto* value=cat::function_for(symbol))return value;return shared::function_for(symbol);
}
struct Plan {
  Issue issue=Issue::contract;Page source=Page::unknown;cat::RecipeId target=static_cast<cat::RecipeId>(0);
  std::uint32_t quantity=0;cat::Branch branch=cat::Branch::unknown;
  std::array<Step,maximum_steps> steps{};std::size_t count=0;
  bool valid() const noexcept {return issue==Issue::none&&count&&count<=steps.size();}
  const Step* effect() const noexcept {return valid()&&steps[count-1].is_effect()?&steps[count-1]:nullptr;}
};
// A single original INFO effect. Before each stage the native adapter must
// read the fresh owner/collection binding as well as the source-profile
// contract. The exact branch INT additionally belongs to the confirmed
// order, not merely its here-stored output category. No paid tobacco choices.
inline Plan make_plan(Page source,cat::RecipeId target,std::uint32_t quantity,cat::Branch branch) noexcept {
  Plan out;out.source=source;out.target=target;out.quantity=quantity;out.branch=branch;
  if(!supported_page(source)){out.issue=Issue::unknown_page;return out;}
  const auto* d=cat::find(target);if(!d){out.issue=Issue::unsupported_recipe;return out;}
  if(!cat::plan_quantity(*d,quantity,branch).valid){out.issue=Issue::quantity;return out;}
  if(!shared::structurally_valid())return out;
  Page current=source;
  const auto add=[&](Kind kind,Invocation mode,std::int32_t owner,std::uint32_t function,Page next) noexcept {
    if(out.count==out.steps.size())return false;
    out.steps[out.count++]={kind,mode,owner,static_cast<std::int32_t>(function),current,next};current=next;return true;
  };
  if(const auto* route=shared::base::route_for(current))
    if(!add(Kind::back,Invocation::choice,route->owner->instance,route->back->symbol,route->category))return Plan{};
  if(current!=Page::special_info) {
    if(const auto* from=shared::category_for(current))
      if(!add(Kind::stop,Invocation::info,from->stop->instance,from->stop->information->symbol,Page::root))return Plan{};
    const auto* to=shared::category_for(Page::special_info);
    if(current!=Page::root||!to||!add(Kind::start,Invocation::info,to->start->instance,to->start->information->symbol,Page::special_info))return Plan{};
  }
  if(!add(Kind::effect,Invocation::info,d->owner.instance,d->callback->symbol,Page::unknown))return Plan{};
  out.issue=Issue::none;return out;
}
inline bool structurally_valid(const Plan& plan) noexcept {
  if(!plan.valid())return false;const auto expected=make_plan(plan.source,plan.target,plan.quantity,plan.branch);
  if(!expected.valid()||expected.count!=plan.count)return false;
  for(std::size_t i=0;i<plan.count;++i){const auto& a=plan.steps[i];const auto& b=expected.steps[i];
    if(a.kind!=b.kind||a.invocation!=b.invocation||a.owner_instance!=b.owner_instance||a.function_symbol!=b.function_symbol||a.before!=b.before||a.after!=b.after)return false;}
  return true;
}
inline alchemy_extra_execution::Profile step_profile(const Plan& plan,std::size_t index) noexcept {
  alchemy_extra_execution::Profile out;if(!structurally_valid(plan)||index>=plan.count){out.reject();return out;}
  const auto& step=plan.steps[index];
  if(step.is_effect()) {
    const auto own=spirit_recipe_runtime::assemble(*cat::find(plan.target));if(!own.valid()){out.reject();return out;}
    const auto p=own.profile();for(std::size_t i=0;i<p.function_count;++i)out.add(&p.functions[i]);
    for(std::size_t i=0;i<p.binding_count;++i)out.add(p.bindings[i]);return out;
  }
  alchemy_extra_execution::common(out);const auto* owner=owner_for(step.owner_instance);
  if(!owner){out.reject();return out;}
  alchemy_extra_execution::owner(out,*owner);out.add(function_for(static_cast<std::uint32_t>(step.function_symbol)));
  if(const auto* source=shared::base::route_for(step.before)) {
    const auto* recipe=recipe_catalog::find(source->recipe);if(!recipe){out.reject();return out;}
    for(const auto& binding:recipe->bindings)out.add(binding);
  }
  if(out.valid()&&!runtime_script::detail::valid_profile(out.view()))out.reject();return out;
}
inline runtime_script::RuntimeContract verify_step(const ReadOnlyMemory& memory,std::uint32_t parser,const Plan& plan,std::size_t index) noexcept {
  const auto p=step_profile(plan,index);return p.valid()?runtime_script::verify_local_contract(memory,parser,p.view()):
    runtime_script::RuntimeContract{false,runtime_script::Issue::invalid_profile,0};
}
} // namespace ubc::spirit_recipe_navigation
