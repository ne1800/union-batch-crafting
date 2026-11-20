// SPDX-License-Identifier: MIT
#include "ubc/stations/station_recipe_navigation.hpp"
#include <cstdio>

namespace {
namespace n=ubc::station_recipe_navigation;
namespace c=ubc::station_recipe_catalog;
unsigned checks=0,failures=0;
void check(bool value,int line) {++checks;if(!value){++failures;std::printf("FAIL line %d\n",line);}}
#define CHECK(x) check((x),__LINE__)
constexpr std::array<n::Page,8> pages{{n::Page::leather_info,n::Page::saw_info,n::Page::forge_info,
  n::Page::leather_choices,n::Page::arrow_choices,n::Page::bolt_choices,
  n::Page::raw_steel_choices,n::Page::hot_blade_choices}};

void metadata_and_pages() {
  CHECK(!n::runtime_ready&&n::maximum_steps==3&&n::routes.size()==5);
  CHECK(n::structurally_valid());
  CHECK(!n::supported_page(n::Page::unknown));
  CHECK(!n::supported_page(static_cast<n::Page>(999)));
  CHECK(n::root_for(static_cast<c::Station>(999))==n::Page::unknown);
  CHECK(n::route_for(static_cast<c::RecipeId>(999))==nullptr);
  CHECK(n::owner_for(-1)==nullptr&&n::owner_for(20399)==nullptr&&n::owner_for(20845)==nullptr);
  CHECK(n::function_for(0)==nullptr);
  struct Expected {std::int32_t owner,number;std::uint32_t back; c::Station station;n::Page root,choices;};
  const std::array<Expected,5> expected{{
    {20848,2,20851,c::Station::leatherwork,n::Page::leather_info,n::Page::leather_choices},
    {20087,2,20090,c::Station::saw,n::Page::saw_info,n::Page::arrow_choices},
    {20098,2,20101,c::Station::saw,n::Page::saw_info,n::Page::bolt_choices},
    {20991,1,20994,c::Station::forge,n::Page::forge_info,n::Page::raw_steel_choices},
    {20999,4,21002,c::Station::forge,n::Page::forge_info,n::Page::hot_blade_choices}
  }};
  for(const auto& e:expected) {
    const auto* route=n::route_for(e.choices);CHECK(route!=nullptr);if(!route)continue;
    CHECK(n::structurally_valid(*route));
    CHECK(route->station==e.station&&route->root==e.root);
    CHECK(route->owner->instance==e.owner&&route->owner->number==e.number&&route->back->symbol==e.back);
    CHECK(route->back->byte_length==11&&!route->back->argc&&!route->back->return_type);
    CHECK(n::owner_for(e.owner)==route->owner&&n::function_for(e.back)==route->back);
    CHECK(route->owner->conditions.count==1&&route->owner->conditions[0].symbol==12330);
    CHECK(route->owner->conditions[0].value==static_cast<std::int32_t>(e.station));
    n::PageState state{true,e.station,static_cast<std::int32_t>(e.station),c::Invocation::choice,e.owner};
    CHECK(n::classify_page(state)==e.choices);
    state.invocation=c::Invocation::info;
    CHECK(n::classify_page(state)==e.root); // A stale choice owner has no significance in INFO mode.
    state.choice_owner=20399;
    CHECK(n::classify_page(state)==e.root);
    state.invocation=c::Invocation::choice;
    CHECK(n::classify_page(state)==n::Page::unknown);
    for(const auto bad:{-1,0,14646,20845,20084,20988,20399,INT32_MAX}) {
      state.choice_owner=bad;CHECK(n::classify_page(state)==n::Page::unknown);
    }
    state.choice_owner=e.owner;
    state.known=false;CHECK(n::classify_page(state)==n::Page::unknown);state.known=true;
    state.production=-1;CHECK(n::classify_page(state)==n::Page::unknown);
    state.production=static_cast<std::int32_t>(e.station)+1;CHECK(n::classify_page(state)==n::Page::unknown);
    state.production=static_cast<std::int32_t>(e.station);
    state.invocation=static_cast<c::Invocation>(99);CHECK(n::classify_page(state)==n::Page::unknown);
    state.invocation=c::Invocation::choice;
    for(const auto& other:expected) {
      state.choice_owner=other.owner;
      CHECK(n::classify_page(state)==(e.station==other.station?other.choices:n::Page::unknown));
    }
    state.station=static_cast<c::Station>(999);state.production=999;
    CHECK(n::classify_page(state)==n::Page::unknown);
  }
  for(const auto page:pages) {
    CHECK(n::supported_page(page));CHECK(n::station_for(page)!=nullptr);
    const auto* station=n::station_for(page);if(!station)continue;
    const auto* route=n::route_for(page);
    const n::PageState state{true,station->station,station->production_context,
      route?c::Invocation::choice:c::Invocation::info,route?route->owner->instance:-1};
    CHECK(n::classify_page(state)==page);
  }
}

void all_routes() {
  unsigned valid=0,cross_station=0;
  for(const auto source:pages) for(const auto& recipe:c::recipes) {
    const auto* station=n::station_for(source);const auto* route=n::route_for(recipe.id);
    CHECK(station&&route);if(!station||!route)continue;
    CHECK(route->owner->instance==recipe.owner.instance&&route->station==recipe.station);
    for(const auto quantity:{1u,2u,7u}) {
      const auto plan=n::make_plan(source,recipe.id,quantity);
      if(station->station!=recipe.station) {
        ++cross_station;
        CHECK(!plan.valid()&&!n::structurally_valid(plan)&&plan.count==0);
        CHECK(plan.issue==n::Issue::station&&plan.effect()==nullptr);
        continue;
      }
      ++valid;
      CHECK(plan.valid()&&n::structurally_valid(plan)&&plan.issue==n::Issue::none);
      CHECK(plan.source==source&&plan.target==recipe.id&&plan.quantity==quantity);
      const auto* source_route=n::route_for(source);
      const auto expected_count=source==route->choices?1u:source_route?3u:2u;
      CHECK(plan.count==expected_count&&plan.count<=3);
      std::size_t at=0;
      if(expected_count==3) {
        const auto& back=plan.steps[at++];
        CHECK(back.kind==n::Kind::back&&!back.is_effect()&&back.invocation==c::Invocation::choice);
        CHECK(back.owner_instance==source_route->owner->instance&&back.function_symbol==static_cast<std::int32_t>(source_route->back->symbol));
        CHECK(back.before==source&&back.after==route->root);
      }
      if(expected_count>1) {
        const auto& owner=plan.steps[at++];
        CHECK(owner.kind==n::Kind::open_owner&&!owner.is_effect()&&owner.invocation==c::Invocation::info);
        CHECK(owner.owner_instance==recipe.owner.instance&&owner.function_symbol==static_cast<std::int32_t>(recipe.owner.information->symbol));
        CHECK(owner.before==route->root&&owner.after==route->choices);
      }
      const auto& effect=plan.steps[at++];
      CHECK(effect.kind==n::Kind::effect&&effect.is_effect()&&effect.invocation==c::Invocation::choice);
      CHECK(effect.owner_instance==recipe.owner.instance&&effect.function_symbol==static_cast<std::int32_t>(recipe.callback.function->symbol));
      CHECK(effect.before==route->choices&&effect.after==n::Page::unknown);
      CHECK(at==plan.count&&plan.effect()==&plan.steps[plan.count-1]);
      for(std::size_t i=1;i<plan.count;++i) CHECK(plan.steps[i-1].after==plan.steps[i].before);
      unsigned effects=0;for(std::size_t i=0;i<plan.count;++i) effects+=plan.steps[i].is_effect()?1u:0u;
      CHECK(effects==1);
    }
  }
  CHECK(valid==117&&cross_station==267);
  // Nine leather targets share the same owner. No unnecessary BACK/reopen.
  for(const auto& recipe:c::recipes) if(recipe.station==c::Station::leatherwork)
    CHECK(n::make_plan(n::Page::leather_choices,recipe.id,7).count==1);
}

void quantities_and_unknowns() {
  for(const auto& recipe:c::recipes) {
    const auto* route=n::route_for(recipe.id);CHECK(route!=nullptr);if(!route)continue;
    const auto limit=static_cast<std::uint32_t>(INT32_MAX)/recipe.outputs[0].units;
    for(const auto source:{route->root,route->choices}) {
      for(const auto good:{1u,5u,10u,limit}) {
        const auto plan=n::make_plan(source,recipe.id,good);
        CHECK(n::structurally_valid(plan)&&plan.effect());
        CHECK(plan.effect()->function_symbol==static_cast<std::int32_t>(recipe.callback.function->symbol));
      }
      for(const auto bad:{0u,limit+1,UINT32_MAX}) {
        const auto plan=n::make_plan(source,recipe.id,bad);
        CHECK(plan.issue==n::Issue::quantity&&!plan.valid()&&plan.count==0&&plan.effect()==nullptr);
      }
      const auto missing=n::make_plan(source,static_cast<c::RecipeId>(0),1);
      CHECK(missing.issue==n::Issue::unsupported_recipe&&!n::structurally_valid(missing));
    }
    for(const auto bad:{n::Page::unknown,static_cast<n::Page>(999)}) {
      const auto plan=n::make_plan(bad,recipe.id,1);
      CHECK(plan.issue==n::Issue::unknown_page&&!plan.valid()&&plan.effect()==nullptr);
    }
  }
  for(const auto issue:{n::Issue::none,n::Issue::unknown_page,n::Issue::unsupported_recipe,
      n::Issue::station,n::Issue::quantity,n::Issue::contract})
    CHECK(std::strcmp(n::issue_name(issue),"invalid_issue")!=0);
  CHECK(std::strcmp(n::issue_name(static_cast<n::Issue>(99)),"invalid_issue")==0);
}

void altered_contracts() {
  for(const auto& route:n::routes) {
    auto changed=route;changed.station=static_cast<c::Station>(999);CHECK(!n::structurally_valid(changed));
    changed=route;changed.root=n::Page::unknown;CHECK(!n::structurally_valid(changed));
    changed=route;changed.choices=n::Page::unknown;CHECK(!n::structurally_valid(changed));
    changed=route;changed.owner=nullptr;CHECK(!n::structurally_valid(changed));
    changed=route;changed.back=nullptr;CHECK(!n::structurally_valid(changed));
    auto owner=*route.owner;changed=route;changed.owner=&owner;
    ++owner.number;CHECK(!n::structurally_valid(changed));
    owner=*route.owner;owner.permanent=false;CHECK(!n::structurally_valid(changed));
    owner=*route.owner;owner.condition=owner.information;CHECK(!n::structurally_valid(changed));
    owner=*route.owner;owner.information=route.back;CHECK(!n::structurally_valid(changed));
    auto back=*route.back;changed=route;changed.back=&back;
    ++back.byte_offset;CHECK(!n::structurally_valid(changed));
    back=*route.back;back.body_sha256[0]^=1;CHECK(!n::structurally_valid(changed));
    back=*route.back;back.argc=1;CHECK(!n::structurally_valid(changed));
    back=*route.back;back.name="UNKNOWN";CHECK(!n::structurally_valid(changed));
  }
  for(const auto source:pages) for(const auto& recipe:c::recipes) {
    const auto original=n::make_plan(source,recipe.id,7);if(!original.valid())continue;
    auto changed=original;changed.issue=n::Issue::contract;CHECK(!n::structurally_valid(changed));
    changed=original;changed.count=0;CHECK(!n::structurally_valid(changed)&&!changed.effect());
    changed=original;changed.count=4;CHECK(!n::structurally_valid(changed)&&!changed.effect());
    changed=original;changed.quantity=0;CHECK(changed.valid()&&!n::structurally_valid(changed));
    changed=original;changed.target=static_cast<c::RecipeId>(999);CHECK(!n::structurally_valid(changed));
    changed=original;changed.source=n::Page::unknown;CHECK(!n::structurally_valid(changed));
    for(std::size_t i=0;i<original.count;++i) {
      changed=original;changed.steps[i].kind=static_cast<n::Kind>(999);CHECK(!n::structurally_valid(changed));
      changed=original;changed.steps[i].invocation=static_cast<c::Invocation>(999);CHECK(!n::structurally_valid(changed));
      changed=original;++changed.steps[i].owner_instance;CHECK(!n::structurally_valid(changed));
      changed=original;++changed.steps[i].function_symbol;CHECK(!n::structurally_valid(changed));
      changed=original;changed.steps[i].before=n::Page::unknown;CHECK(!n::structurally_valid(changed));
      changed=original;changed.steps[i].after=changed.steps[i].before;CHECK(!n::structurally_valid(changed));
    }
    if(original.count>1) {
      changed=original;std::swap(changed.steps[0],changed.steps[original.count-1]);CHECK(!n::structurally_valid(changed));
      changed=original;--changed.count;CHECK(!n::structurally_valid(changed));
    }
    // A plan establishes neither knowledge nor SELF or stock. Its final
    // state must never substitute for the required native ready/end witness.
    changed=original;changed.steps[changed.count-1].after=n::root_for(recipe.station);
    CHECK(!n::structurally_valid(changed));
  }
}
} // namespace

int main() {
  metadata_and_pages();all_routes();quantities_and_unknowns();altered_contracts();
  std::printf("station_recipe_navigation: %u checks, %u failures\n",checks,failures);
  return failures?1:0;
}
