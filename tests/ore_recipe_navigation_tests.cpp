// SPDX-License-Identifier: MIT
#include "ubc/ore_recipe_navigation.hpp"
#include <cstdio>
namespace n=ubc::ore_recipe_navigation;namespace c=ubc::ore_recipe_catalog;
namespace {unsigned checks=0,failures=0;void check(bool b,const char* why){++checks;if(!b){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}}}
int main(){
  check(!n::runtime_ready&&n::structurally_valid(),"static routes are valid without execution authority");
  for(const auto source:n::pages)for(const auto& r:c::recipes)
    for(const auto quantity:{1u,2u,3u,300u,r.callback.maximum_quantity}){
      const auto p=n::make_plan(source,r.id,quantity);
      const auto count=source==n::Page::ore?1u:source==n::Page::root?2u:source==n::Page::common?3u:4u;
      check(p.valid()&&n::structurally_valid(p)&&p.count==count,"each source takes only the necessary original steps");
      auto current=source;unsigned effects=0;
      for(std::size_t i=0;i<p.count;++i){const auto& s=p.steps[i];
        check(s.before==current,"source page is bound at each step");
        if(s.is_effect()){
          ++effects;check(i+1==p.count&&current==n::Page::ore&&s.after==n::Page::unknown&&
            s.invocation==c::Invocation::info&&s.owner_instance==r.owner.instance&&
            s.function_symbol==static_cast<int>(r.callback.function->symbol),"only one terminal direct Info produces weapons");
        }else if(s.invocation==c::Invocation::choice){
          const auto* original=ubc::anvil_recipe_navigation::recipe_for(current);
          check(original&&s.owner_instance==original->owner.instance&&s.function_symbol==static_cast<int>(original->back->symbol)&&
            s.after==n::Page::common,"owned ordinary choices close through their actual BACK");
        }else if(current==n::Page::common){
          check(s.owner_instance==20707&&s.function_symbol==20709&&s.after==n::Page::root,"common flag is reset by its own Info");
        }else{
          check(current==n::Page::root&&s.owner_instance==20704&&s.function_symbol==20706&&s.after==n::Page::ore,
            "ore category is opened by original PC_ORE_INFO");
        }
        current=s.after;
      }
      check(effects==1,"no repeat or second item effect exists");
      for(unsigned bad=0;bad<9;++bad){auto x=p;
        switch(bad){case 0:++x.steps[0].owner_instance;break;case 1:++x.steps[0].function_symbol;break;
          case 2:x.steps[0].before=n::Page::unknown;break;case 3:x.steps[p.count-1].invocation=c::Invocation::choice;break;
          case 4:x.steps[p.count-1].after=n::Page::ore;break;case 5:x.steps[p.count-1].kind=n::Kind::back;break;
          case 6:x.count=5;break;case 7:x.quantity=0;break;case 8:x.target=static_cast<c::RecipeId>(0);break;}
        check(!n::structurally_valid(x),"altered ticket contents cannot become a canonical route");
      }
    }
  for(const auto& r:c::recipes){
    check(!n::make_plan(n::Page::root,r.id,r.callback.maximum_quantity+1).valid(),"multiplicative int32 limit also gates navigation");
    check(!n::make_plan(n::Page::unknown,r.id,1).valid(),"unknown source has no guessed back action");
    n::PageState ore{true,1,1,0,1,c::Invocation::choice,r.owner.instance};
    check(n::classify_page(ore)==n::Page::unknown,"a direct ore Info is not an invented choice page");
  }
  for(const auto& source:ubc::anvil_recipe_catalog::recipes){
    n::PageState page{true,1,1,1,0,c::Invocation::choice,source.owner.instance};
    check(n::classify_page(page)==ubc::anvil_recipe_navigation::page_for(source.id),"only the five known source choices are recognized");
    page.ore_weapons=1;check(n::classify_page(page)==n::Page::unknown,"contradictory category flags invalidate source");
  }
  std::printf("ore_recipe_navigation_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
