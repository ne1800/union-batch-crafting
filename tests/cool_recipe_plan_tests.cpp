// SPDX-License-Identifier: MIT
#include "ubc/cool_recipe_plan.hpp"
#include <cstdio>
#include <utility>
namespace {unsigned checks=0,failures=0;void check(bool v,const char* n){++checks;if(!v){++failures;std::fprintf(stderr,"FAIL: %s\n",n);}}
namespace c=ubc::cool_recipe_catalog;namespace p=ubc::cool_recipe;
p::Evidence stock(std::int64_t hot,std::int64_t cold){return {true,{{{16612,hot,true,true},{16613,cold,true,true}}}};}
}
int main(){
  check(c::structurally_valid(c::recipe)&&!c::runtime_ready,"isolated canonical family is not runtime authorization");
  for(int hot=0;hot<24;++hot)for(int cold=0;cold<4;++cold)for(unsigned n=0;n<27;++n){
    const auto x=p::plan(c::recipe,stock(hot,cold),n);check(x.known&&x.maximum==static_cast<unsigned>(hot),"generated hand never increases storage maximum");
    check(x.valid==(n>0&&n<=static_cast<unsigned>(hot)),"positive quantity bounded by real storage");
    if(!x.valid)continue;
    check(x.after_end[0].amount==std::int64_t{hot}-n&&x.after_end[1].amount==std::int64_t{cold}+n,"one exact end debit included in final conservation");
    check(x.before_end[0].amount==std::int64_t{hot}-(n-1)&&x.before_end[1].amount==std::int64_t{cold}+n,"normal-end unit retained before lifecycle");
    check(x.native_end_debit==1,"one original generated-anchor storage debit");
    if(n==1)check(x.callback==20775&&!x.count&&!x.extra_debit&&!x.requires_quantity_receipt,"single remains original and never removes zero");
    else if(hot>=3){check(x.callback==20778&&x.count==2&&!x.extra_debit,"original triple owner only with original minimum three");
      check(x.operands[0].offset==62&&x.operands[0].original==2&&x.operands[0].reserved==1&&x.operands[1].offset==82&&x.operands[1].original==3,"only original triple debit and creation operands");
      check(x.requires_quantity_receipt==(n!=3),"fixed triple text needs distinct receipt outside three");}
    else check(hot==2&&n==2&&x.callback==20775&&x.extra_debit==1&&x.requires_extra_debit&&x.after_callback[0].amount==2,"Lager2/N2 benötigt den zusätzlichen Entzug und den Einzelcallback");
    auto copy=x;check(copy.body().operands==copy.operands.data(),"body view points into current plan copy");
  }
  for(const auto hot:{0,1,2,3,INT32_MAX})for(const auto cold:{0,INT32_MAX-2,INT32_MAX}){
    const auto x=p::plan(c::recipe,stock(hot,cold),0);const auto expected=static_cast<unsigned>(hot<INT32_MAX-cold?hot:INT32_MAX-cold);
    check(x.known&&x.maximum==expected&&!x.valid,"output room caps maximum without rejecting otherwise valid smaller quantities");
    check(p::plan(c::recipe,stock(hot,cold),expected).valid==(expected>0),"exact numeric maximum is representable");
    check(!p::plan(c::recipe,stock(hot,cold),UINT32_MAX).valid,"uint32 overflow quantity rejected");
  }
  for(unsigned mutation=0;mutation<7;++mutation){auto e=stock(5,3);switch(mutation){
    case 0:e.stable_boundary_verified=false;break;case 1:e.stocks[0].known=false;break;case 2:e.stocks[0].unambiguous=false;break;
    case 3:e.stocks[0].amount=-1;break;case 4:e.stocks[1].amount=std::int64_t{INT32_MAX}+1;break;case 5:e.stocks[1].symbol=16612;break;case 6:e.stocks[0].symbol=16611;break;}
    check(!p::plan(c::recipe,e,2).valid,"unknown misbound ambiguous or invalid stock cannot plan production");}
  auto e=stock(2,3);std::swap(e.stocks[0],e.stocks[1]);check(p::plan(c::recipe,e,2).valid,"symbol mapping instead of positional quantities");
  e=stock(5,3);e.stocks[1].unambiguous=false;check(p::plan(c::recipe,e,5).valid,"mixed output counting does not invent ingredient ambiguity");
  for(unsigned m=0;m<6;++m){auto bad=c::recipe;switch(m){case 0:bad.id=static_cast<c::RecipeId>(1);break;case 1:bad.single=&c::triple;break;case 2:bad.hot_symbol=16611;break;case 3:bad.generated_left=false;break;case 4:bad.native_end_debit=0;break;case 5:bad.production_context=1;break;}
    check(!c::structurally_valid(bad)&&!p::plan(bad,stock(5,0),2).valid,"descriptor mutation cannot weaken original semantics");}
  const auto cancel=p::cancel_plan(stock(5,2));check(cancel.valid&&cancel.after_callback[0].amount==6&&cancel.after_end[0].amount==5&&cancel.after_end[1].amount==2,"original ENDE refund followed by storage debit cancels exactly");
  check(!p::cancel_plan(stock(INT32_MAX,0)).valid,"cancel callback still needs temporary hot output space");
  std::printf("cool_recipe_plan_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
