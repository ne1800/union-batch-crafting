// SPDX-License-Identifier: MIT
#include "ubc/stations/anvil/anvil_recipe_navigation.hpp"
#include <cstdio>

namespace {
namespace n=ubc::anvil_recipe_navigation;namespace c=ubc::anvil_recipe_catalog;
unsigned checks=0,failures=0;
void check(bool value,const char* name){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",name);}}
}
int main() {
  check(!n::runtime_ready,"navigation never authorizes native execution");
  check(n::structurally_valid(),"all canonical reversible routes validate");
  for(const auto source:n::pages)for(const auto& recipe:c::recipes)for(const auto quantity:{1u,2u,3u,300u,static_cast<unsigned>(INT32_MAX)}) {
    const auto p=n::make_plan(source,recipe.id,quantity);
    check(p.valid()&&p.count<=4&&n::structurally_valid(p),"every admitted source reaches each target by at most four original steps");
    auto current=source;unsigned effects=0;
    for(std::size_t i=0;i<p.count;++i) {
      const auto& s=p.steps[i];check(s.before==current,"each step binds its immediate source page");
      check(s.owner_instance>0&&s.function_symbol>0,"steps identify original owner and function by value");
      if(s.is_effect()) {
        ++effects;check(i+1==p.count&&s.after==n::Page::unknown,"one terminal effect has no invented ready page");
        check(s.function_symbol==static_cast<std::int32_t>(recipe.callback.function->symbol)&&s.invocation==c::Invocation::choice,"quantity three also executes original single template once");
      } else {
        check(s.function_symbol!=static_cast<std::int32_t>(recipe.callback.function->symbol),"navigation has no production callback");
        if(s.invocation==c::Invocation::choice) {
          const auto* from=n::recipe_for(s.before);
          check(from&&s.owner_instance==from->owner.instance&&s.function_symbol==static_cast<std::int32_t>(from->back->symbol)&&s.after==n::Page::common,"choice back clears only the owned five-family page");
        } else if(s.before==n::Page::ore) {
          check(s.owner_instance==20710&&s.function_symbol==20712&&s.after==n::Page::root,"ore category back uses the exact original flag reset");
        } else if(s.before==n::Page::root) {
          check(s.owner_instance==20701&&s.function_symbol==20703&&s.after==n::Page::common,"ordinary category opens once from root");
        } else {
          check(s.before==n::Page::common&&s.owner_instance==recipe.owner.instance&&s.function_symbol==static_cast<std::int32_t>(recipe.owner.information->symbol)&&s.after==n::page_for(recipe.id),"learned target owner opens its own choices");
        }
      }
      current=s.after;
    }
    check(effects==1,"no recipe loop or automatic retry");
    for(unsigned change=0;change<8;++change) {
      auto altered=p;
      switch(change){case 0:++altered.steps[0].owner_instance;break;case 1:++altered.steps[0].function_symbol;break;
        case 2:altered.steps[0].invocation=static_cast<c::Invocation>(99);break;case 3:altered.steps[0].before=n::Page::unknown;break;
        case 4:altered.steps[p.count-1].after=n::Page::root;break;case 5:altered.count=0;break;
        case 6:altered.count=5;break;case 7:altered.quantity=0;break;}
      check(!n::structurally_valid(altered),"canonical reconstruction rejects stale or altered step metadata");
    }
  }
  for(const auto source:n::pages)for(const auto& recipe:c::recipes) {
    for(const auto bad:{0u,static_cast<unsigned>(INT32_MAX)+1u,UINT32_MAX})check(!n::make_plan(source,recipe.id,bad).valid(),"invalid quantities do not form a route");
    const auto p=n::make_plan(source,recipe.id,1);
    check(p.count==(source==n::page_for(recipe.id)?1u:source==n::Page::root?3u:source==n::Page::common?2u:source==n::Page::ore?4u:3u),"route has no redundant reset/open steps");
  }
  for(unsigned normal=0;normal<3;++normal)for(unsigned ore=0;ore<3;++ore) {
    n::PageState s{true,1,1,static_cast<int>(normal),static_cast<int>(ore),c::Invocation::info,-1};
    const auto expected=normal==0&&ore==0?n::Page::root:normal==1&&ore==0?n::Page::common:normal==0&&ore==1?n::Page::ore:n::Page::unknown;
    check(n::classify_page(s)==expected,"category flags have exact disjoint page semantics");
    for(const auto& r:c::recipes) {
      s.invocation=c::Invocation::choice;s.choice_owner=r.owner.instance;
      check(n::classify_page(s)==(normal==1&&ore==0?n::page_for(r.id):n::Page::unknown),"choice owner cannot override contradictory category context");
    }
  }
  n::PageState root{true,1,1,0,0,c::Invocation::info,-1};
  for(unsigned change=0;change<6;++change){auto s=root;
    switch(change){case 0:s.known=false;break;case 1:s.production=3;break;case 2:s.start_smithing=0;break;
      case 3:s.invocation=static_cast<c::Invocation>(99);break;case 4:s.invocation=c::Invocation::choice;s.choice_owner=20744;break;
      case 5:s.normal_weapons=-1;break;}
    check(n::classify_page(s)==n::Page::unknown,"foreign ore choices or unknown context never invent a back function");
  }
  check(!n::make_plan(n::Page::unknown,c::RecipeId::common_sword,1).valid(),"unknown source cannot dispatch");
  check(!n::make_plan(n::Page::root,static_cast<c::RecipeId>(0),1).valid(),"foreign target cannot dispatch");
  std::printf("anvil_recipe_navigation_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
