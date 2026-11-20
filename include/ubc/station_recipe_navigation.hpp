// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/station_recipe_catalog.hpp"

// Pure path planning for the three separate follow-up stations. No engine,
// learning, SELF, material, hand or runtime release. A native order binds
// each stage freshly to the same physical station and new collection.
namespace ubc::station_recipe_navigation {
namespace catalog=station_recipe_catalog;
using catalog::Station;
using catalog::RecipeId;
using catalog::Owner;
using catalog::Function;
using catalog::Invocation;

enum class Page {
  unknown, leather_info, saw_info, forge_info,
  leather_choices, arrow_choices, bolt_choices, raw_steel_choices, hot_blade_choices
};
enum class Kind { back, open_owner, effect };
enum class Issue { none, unknown_page, unsupported_recipe, station, quantity, contract };
inline constexpr bool runtime_ready=false;
inline constexpr std::size_t maximum_steps=3;

constexpr const Function* function_for(std::uint32_t symbol) noexcept {
  for(const auto& function:catalog::functions) if(function.symbol==symbol) return &function;
  return nullptr;
}
struct Route {
  Station station;
  Page root,choices;
  const Owner* owner;
  const Function* back; // Checked: only ClearChoices(own owner), no material/end.
};
inline constexpr std::array<Route,5> routes{{
  {Station::leatherwork,Page::leather_info,Page::leather_choices,&catalog::owner_20848,function_for(20851)},
  {Station::saw,Page::saw_info,Page::arrow_choices,&catalog::owner_20087,function_for(20090)},
  {Station::saw,Page::saw_info,Page::bolt_choices,&catalog::owner_20098,function_for(20101)},
  {Station::forge,Page::forge_info,Page::raw_steel_choices,&catalog::owner_20991,function_for(20994)},
  {Station::forge,Page::forge_info,Page::hot_blade_choices,&catalog::owner_20999,function_for(21002)}
}};

inline Page root_for(Station station) noexcept {
  switch(station) {
    case Station::leatherwork:return Page::leather_info;
    case Station::saw:return Page::saw_info;
    case Station::forge:return Page::forge_info;
  }
  return Page::unknown;
}
inline const Route* route_for(Page page) noexcept {
  for(const auto& route:routes) if(route.choices==page) return &route;
  return nullptr;
}
inline const Route* route_for(RecipeId id) noexcept {
  const auto* recipe=catalog::find(id);
  if(!recipe) return nullptr;
  for(const auto& route:routes)
    if(route.station==recipe->station&&route.owner->instance==recipe->owner.instance) return &route;
  return nullptr;
}
inline const Owner* owner_for(std::int32_t instance) noexcept {
  for(const auto& route:routes) if(route.owner->instance==instance) return route.owner;
  return nullptr;
}
inline const catalog::StationProfile* station_for(Page page) noexcept {
  for(const auto& station:catalog::stations) {
    if(page==root_for(station.station)) return &station;
    const auto* route=route_for(page);
    if(route&&route->station==station.station) return &station;
  }
  return nullptr;
}
inline bool supported_page(Page page) noexcept { return station_for(page)!=nullptr; }

inline bool structurally_valid(const Route& route) noexcept {
  const auto* canonical=route_for(route.choices);
  if(!canonical||route.station!=canonical->station||route.root!=root_for(route.station)||
    route.root!=canonical->root||!route.owner||!route.back||
    !catalog::detail::same_function(route.back,canonical->back)||
    !catalog::detail::void_function(route.back)||route.back->byte_length!=11) return false;
  const catalog::Descriptor* recipe=nullptr;
  for(const auto& value:catalog::recipes)
    if(value.station==route.station&&value.owner.instance==canonical->owner->instance) {recipe=&value;break;}
  return recipe&&catalog::structurally_valid(*recipe)&&
    catalog::detail::owner(*route.owner,*canonical->owner,*recipe)&&
    catalog::detail::same_function(route.back,recipe->back);
}
inline bool structurally_valid() noexcept {
  for(std::size_t i=0;i<routes.size();++i) {
    if(!structurally_valid(routes[i])) return false;
    for(std::size_t j=0;j<i;++j)
      if(routes[i].choices==routes[j].choices||routes[i].owner->instance==routes[j].owner->instance||
        routes[i].back->symbol==routes[j].back->symbol) return false;
  }
  for(const auto& recipe:catalog::recipes)
    if(!catalog::structurally_valid(recipe)||!route_for(recipe.id)) return false;
  return true;
}

struct PageState {
  bool known=false; // Current station, production, mode and possibly choice owner read.
  Station station=static_cast<Station>(0);
  std::int32_t production=-1;
  Invocation invocation=Invocation::info;
  std::int32_t choice_owner=-1;
};
// INFO uses no old choice owner. Choices instead need exactly
// one of the five current owners in the positively bound station.
inline Page classify_page(const PageState& value) noexcept {
  if(!value.known||root_for(value.station)==Page::unknown||
    value.production!=static_cast<std::int32_t>(value.station)) return Page::unknown;
  if(value.invocation==Invocation::info) return root_for(value.station);
  if(value.invocation!=Invocation::choice) return Page::unknown;
  for(const auto& route:routes)
    if(route.station==value.station&&route.owner->instance==value.choice_owner) return route.choices;
  return Page::unknown;
}

struct Step {
  Kind kind=Kind::back;
  Invocation invocation=Invocation::info;
  std::int32_t owner_instance=-1,function_symbol=-1;
  Page before=Page::unknown,after=Page::unknown;
  bool is_effect() const noexcept { return kind==Kind::effect; }
};
struct Plan {
  Issue issue=Issue::contract;
  Page source=Page::unknown;
  RecipeId target=static_cast<RecipeId>(0);
  std::uint32_t quantity=0;
  std::array<Step,maximum_steps> steps{};
  std::size_t count=0;
  bool valid() const noexcept { return issue==Issue::none&&count>0&&count<=maximum_steps; }
  const Step* effect() const noexcept {
    return valid()&&steps[count-1].is_effect()?&steps[count-1]:nullptr;
  }
};
// Ammo: Owner-INFO has a knowledge branch. Only with positive current knowledge
// gate is its successful navigation purely Clear/AddChoices. The plan invents
// no C_INFO learning condition and does not release the step. SELF==HERO, stock,
// callscope and effect gates remain also at the runtime adapter. Effect.after
// remains unknown: Ready/End is separately proven after the original follow-up.
inline Plan make_plan(Page source,RecipeId target,std::uint32_t quantity) noexcept {
  Plan out;out.source=source;out.target=target;out.quantity=quantity;
  const auto* from_station=station_for(source);
  if(!from_station) {out.issue=Issue::unknown_page;return out;}
  const auto* recipe=catalog::find(target);
  const auto* destination=route_for(target);
  if(!recipe||!destination) {out.issue=Issue::unsupported_recipe;return out;}
  if(from_station->station!=recipe->station) {out.issue=Issue::station;return out;}
  if(!structurally_valid()) return out;
  const auto* callback=catalog::callback_for_quantity(*recipe,quantity);
  if(!callback) {out.issue=Issue::quantity;return out;}
  if(callback->invocation!=Invocation::choice||callback->owner_instance!=destination->owner->instance) return out;
  Page current=source;
  auto add=[&](Kind kind,Invocation invocation,std::int32_t owner,std::uint32_t function,Page after) noexcept {
    if(out.count>=maximum_steps) return false;
    out.steps[out.count++]={kind,invocation,owner,static_cast<std::int32_t>(function),current,after};
    current=after;return true;
  };
  if(const auto* from=route_for(current)) {
    if(from->owner->instance!=destination->owner->instance&&
      !add(Kind::back,Invocation::choice,from->owner->instance,from->back->symbol,from->root)) return Plan{};
  }
  if(current!=destination->choices) {
    if(current!=destination->root||
      !add(Kind::open_owner,Invocation::info,destination->owner->instance,
        destination->owner->information->symbol,destination->choices)) return Plan{};
  }
  if(!add(Kind::effect,Invocation::choice,destination->owner->instance,
    callback->function->symbol,Page::unknown)) return Plan{};
  out.issue=Issue::none;return out;
}
inline bool structurally_valid(const Plan& plan) noexcept {
  if(!plan.valid()) return false;
  const auto expected=make_plan(plan.source,plan.target,plan.quantity);
  if(!expected.valid()||expected.count!=plan.count) return false;
  for(std::size_t i=0;i<plan.count;++i) {
    const auto& a=plan.steps[i];const auto& b=expected.steps[i];
    if(a.kind!=b.kind||a.invocation!=b.invocation||a.owner_instance!=b.owner_instance||
      a.function_symbol!=b.function_symbol||a.before!=b.before||a.after!=b.after) return false;
  }
  return true;
}
inline const char* issue_name(Issue issue) noexcept {
  switch(issue) {
    case Issue::none:return "none";
    case Issue::unknown_page:return "unknown_page";
    case Issue::unsupported_recipe:return "unsupported_recipe";
    case Issue::station:return "station";
    case Issue::quantity:return "quantity";
    case Issue::contract:return "contract";
  }
  return "invalid_issue";
}
} // namespace ubc::station_recipe_navigation
