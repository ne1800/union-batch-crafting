// SPDX-License-Identifier: MIT
#include "ubc/spirit_recipe_navigation.hpp"
#include <cstdio>
namespace n=ubc::spirit_recipe_navigation;
namespace c=ubc::spirit_recipe_catalog;
namespace {unsigned checks=0,failures=0;
void check(bool value,int line){++checks;if(!value){++failures;std::fprintf(stderr,"failure line %d\n",line);}}
#define CHECK(value) check((value),__LINE__)
}
int main() {
  std::size_t longest=0;
  for(const auto source:{n::Page::root,n::Page::health_info,n::Page::mana_info,n::Page::booze_info,n::Page::special_info,n::Page::tobacco_info,
      n::Page::health_01_choices,n::Page::health_02_choices,n::Page::health_03_choices,n::Page::mana_01_choices,n::Page::mana_02_choices,n::Page::mana_03_choices})
    for(const auto branch:{c::Branch::ordinary,c::Branch::enriched})for(const auto amount:{1u,2u,c::maximum_quantity}) {
      const auto p=n::make_plan(source,c::RecipeId::spirit,amount,branch);
      CHECK(p.valid()&&n::structurally_valid(p));if(p.count>longest)longest=p.count;
      CHECK(p.effect()&&p.effect()->owner_instance==20368&&p.effect()->function_symbol==20370&&p.effect()->invocation==n::Invocation::info);
      CHECK(p.effect()->before==n::Page::special_info&&p.effect()->after==n::Page::unknown);
      unsigned effects=0;auto page=source;
      for(std::size_t i=0;i<p.count;++i) {
        const auto& step=p.steps[i];effects+=step.is_effect()?1u:0u;
        CHECK(step.before==page&&n::owner_for(step.owner_instance)&&n::function_for(static_cast<std::uint32_t>(step.function_symbol)));
        const auto profile=n::step_profile(p,i);CHECK(profile.valid());const auto view=profile.view();bool found=false;
        for(std::size_t j=0;j<view.function_count;++j)found|=view.functions[j].symbol==static_cast<std::uint32_t>(step.function_symbol);
        CHECK(found&&view.function_count<=64&&view.binding_count<=64);page=step.after;
      }
      CHECK(effects==1&&page==n::Page::unknown);
      CHECK(!n::step_profile(p,p.count).valid());
      for(unsigned mutation=0;mutation<8;++mutation) {
        auto bad=p;switch(mutation){case 0:++bad.steps[0].owner_instance;break;case 1:++bad.steps[p.count-1].function_symbol;break;
          case 2:bad.steps[0].before=n::Page::unknown;break;case 3:bad.steps[0].invocation=static_cast<n::Invocation>(99);break;
          case 4:bad.branch=c::Branch::unknown;break;case 5:bad.quantity=0;break;case 6:bad.count=5;break;case 7:bad.target=static_cast<c::RecipeId>(1);break;}
        CHECK(!n::structurally_valid(bad)&&!n::step_profile(bad,0).valid());
      }
      if(source==n::Page::special_info)CHECK(p.count==1);
    }
  CHECK(longest==4);
  CHECK(!n::make_plan(n::Page::unknown,c::RecipeId::spirit,1,c::Branch::ordinary).valid());
  CHECK(!n::make_plan(n::Page::root,c::RecipeId::spirit,c::maximum_quantity+1,c::Branch::ordinary).valid());
  CHECK(!n::make_plan(n::Page::root,c::RecipeId::spirit,1,c::Branch::unknown).valid());
  n::PageState state{true,4,0,0,0,1,0,n::Invocation::choice,20400};CHECK(n::classify_page(state)==n::Page::unknown);
  state.invocation=n::Invocation::info;CHECK(n::classify_page(state)==n::Page::tobacco_info);
  state.special=1;CHECK(n::classify_page(state)==n::Page::unknown);
  std::printf("spirit_recipe_navigation_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
