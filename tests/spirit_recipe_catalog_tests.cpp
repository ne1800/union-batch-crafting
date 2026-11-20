// SPDX-License-Identifier: MIT
#include "ubc/catalogs/spirit_recipe_catalog.hpp"
#include <cstdio>
#include <vector>
namespace c=ubc::spirit_recipe_catalog;
namespace { unsigned checks=0,failures=0;void check(bool value,const char* why){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}} }
int main(){
  const auto& r=c::recipes[0];
  check(!c::runtime_ready&&c::structurally_valid(r),"isolated canonical profile grants no native execution");
  check(r.targets.count==7&&r.costs.count==5,"all ingredients, reserved flask and both actual outputs are represented");
  check(r.owner.instance==20368&&r.owner.condition->symbol==20369&&r.callback->symbol==20370&&r.owner.information==r.callback,
    "one exact original direct info owns both output branches");
  check(r.amount.count==10&&r.amount.length==324&&r.maximum_quantity==static_cast<unsigned>(INT32_MAX)/2,
    "both success branches share one complete bounded amount plan");
  unsigned creates=0;for(const auto& use:r.operand_uses){
    check(use.offset!=309,"original missing-material flask refund stays unpatched");
    if(use.role==c::AmountRole::create_count){++creates;check(use.symbol==15739||use.symbol==15741,"only actual output operands are scaled");}
  }
  check(creates==2,"both mutually exclusive create counts are in the same restoration scope");
  for(const auto v:{INT32_MIN,-1,0,1,2,INT32_MAX}) {
    const auto branch=c::branch_for(true,v);const auto* selected=c::variant(r,branch);
    check(selected&&selected->output.symbol==(v==1?15741:15739),"exact comparison, not truthiness, selects original output");
    for(const auto n:{1u,2u,r.maximum_quantity}){
      const auto p=c::plan_quantity(r,n,branch);
      check(p.valid&&p.output==selected->output.symbol&&p.quantity==n&&p.patch_original==(n>1),"one original callback handles bounded N");
      for(std::size_t i=0;i<r.amount.count;++i)check(std::uint64_t{r.amount.operands[i].units}*n<=INT32_MAX,"each int32 operand is representable");
    }
  }
  check(c::branch_for(false,0)==c::Branch::unknown&&!c::plan_quantity(r,1,c::Branch::unknown).valid,
    "an unreadable flag is never the original else branch");
  check(!c::plan_quantity(r,0,c::Branch::ordinary).valid&&!c::plan_quantity(r,r.maximum_quantity+1,c::Branch::ordinary).valid,
    "zero and overflow are never amounts");
  check(r.refund.symbol==16604&&r.refund.amount==1&&r.refund.actor==c::Actor::self&&r.extra_flasks.reserved_hand==1,
    "unchanged failure refund and separate N-1 debit retain the actual hand lifetime");
  for(unsigned mutation=0;mutation<22;++mutation){
    auto x=r;auto callback=*r.callback;auto owner=r.owner;auto hand=r.hand;
    std::vector<ubc::AmountOperand> amounts(r.amount.operands,r.amount.operands+r.amount.count);
    std::vector<c::OperandUse> uses(r.operand_uses.begin(),r.operand_uses.end());
    std::vector<c::Requirement> requirements(r.requirements.begin(),r.requirements.end());
    std::vector<c::ItemAmount> costs(r.costs.begin(),r.costs.end());
    switch(mutation){
      case 0:x.id=static_cast<c::RecipeId>(0);break;case 1:x.name="false";break;
      case 2:++callback.byte_offset;x.callback=&callback;break;case 3:owner.condition=owner.information;x.owner=owner;break;
      case 4:x.branch_symbol=11198;break;case 5:x.branch_equal=2;break;
      case 6:x.variants[0].output.symbol=15741;break;case 7:x.variants[1].output.units=2;break;
      case 8:x.amount.count--;break;case 9:x.amount.digest[0]^=1;break;
      case 10:amounts.back().offset=309;x.amount.operands=amounts.data();break;
      case 11:amounts[0].allow_zero=true;x.amount.operands=amounts.data();break;
      case 12:uses.back().symbol=16604;x.operand_uses={uses.data(),uses.size()};break;
      case 13:requirements[1].symbol=11967;x.requirements={requirements.data(),requirements.size()};break;
      case 14:costs[0].units=1;x.costs={costs.data(),costs.size()};break;
      case 15:hand.consumed_on_success=false;x.hand=hand;break;
      case 16:x.extra_flasks.after_verified_original=false;break;case 17:x.refund.amount=2;break;
      case 18:x.targets.count=6;break;case 19:x.maximum_quantity=INT32_MAX;break;
      case 20:x.variants[0].success.count=0;break;case 21:x.prints.count=0;break;
    }
    check(!c::structurally_valid(x)&&!c::plan_quantity(x,1,c::Branch::ordinary).valid,"changed own metadata cannot weaken branch or amount rules");
  }
  std::printf("spirit_recipe_catalog_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
