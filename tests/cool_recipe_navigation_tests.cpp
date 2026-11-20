// SPDX-License-Identifier: MIT
#include "ubc/stations/cool/cool_recipe_navigation.hpp"
#include <cstdio>
namespace {unsigned checks=0,failures=0;void check(bool x,const char* n){++checks;if(!x){++failures;std::fprintf(stderr,"FAIL: %s\n",n);}}
namespace nav=ubc::cool_recipe_navigation;namespace cat=ubc::cool_recipe_catalog;
std::array<nav::Offer,3> offers{{{20770,20771,20772},{20773,20774,20775},{20776,20777,20778}}};
ubc::cool_recipe::Evidence stock(int n){return {true,{{{16612,n,true,true},{16613,0,true,true}}}};}
}
int main(){
  const auto page=nav::identify_page(13,offers.data(),3,true);check(page.valid&&page.single&&page.triple,"all three original direct infos form root");
  for(int hot=0;hot<7;++hot)for(unsigned n=0;n<8;++n){const auto p=nav::make_plan(cat::recipe,stock(hot),n,page);check(p.valid==(n>0&&n<=static_cast<unsigned>(hot)&&!(hot==2&&n==2)),"one positive original effect only");if(p.valid){check(p.count==1&&p.step.invocation==ubc::recipe_catalog::Invocation::info&&p.step.function==p.amount.callback&&p.step.owner==p.amount.owner,"no fake Choice or owner reinterpretation");check(nav::valid_plan(p,cat::recipe,stock(hot),page),"canonical reconstruction");auto changed=p;changed.step.function=20772;check(!nav::valid_plan(changed,cat::recipe,stock(hot),page),"ENDE cannot replace effect");changed=p;++changed.quantity;check(!nav::valid_plan(changed,cat::recipe,stock(hot),page),"quantity ticket is exact");}}
  const auto two=nav::identify_page(13,offers.data(),2,true);check(nav::make_plan(cat::recipe,stock(2),2,two).amount.requires_extra_debit,"zwei heiße Klingen binden den Vorentzug an die einzelne INFO");check(!nav::make_plan(cat::recipe,stock(4),2,two).valid,"selected real triple owner must exist in fresh collection");
  for(unsigned m=0;m<7;++m){auto bad=offers;switch(m){case 0:bad[1].owner=20776;break;case 1:bad[1].function=20778;break;case 2:bad[1].condition=20777;break;case 3:bad[2]=bad[1];break;case 4:bad[0]=bad[1];break;case 5:bad[1].owner=1;break;case 6:bad[0].function=20775;break;}check(!nav::identify_page(13,bad.data(),3,true).valid,"unknown duplicate or incorrectly mapped info cannot identify own root");}
  check(!nav::identify_page(1,offers.data(),3,true).valid&&!nav::identify_page(13,nullptr,3,true).valid&&!nav::identify_page(13,offers.data(),4,true).valid&&!nav::identify_page(13,offers.data(),3,false).valid,"wrong context missing boundary and count rejected before read");
  std::array<nav::Offer,2> impossible{{offers[0],offers[2]}};check(!nav::identify_page(13,impossible.data(),2,true).valid,"triple-only is inconsistent with original condition subset");
  check(nav::identify_page(13,offers.data(),1,true).valid&&!nav::make_plan(cat::recipe,stock(0),1,nav::identify_page(13,offers.data(),1,true)).valid,"known empty stock keeps ENDE-only root, never a production");
  std::printf("cool_recipe_navigation_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
