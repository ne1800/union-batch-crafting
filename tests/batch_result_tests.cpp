// SPDX-License-Identifier: MIT
#include "ubc/core/batch_result.hpp"
#include <cstdio>
namespace {
unsigned checks=0,failures=0;
void check(bool value,int line){++checks;if(!value){++failures;std::printf("FAIL %d\n",line);}}
#define CHECK(x) check((x),__LINE__)
ubc::SelectionStock initial(ubc::OriginalRecipe recipe){
  ubc::SelectionStock stock;stock.issue=ubc::StockIssue::none;
  stock.stored={{400,12,22,60,60,37}};stock.maximum=recipe==ubc::OriginalRecipe::meat?400:23;
  if(recipe==ubc::OriginalRecipe::healing_essence)stock.reserved[2]=1;
  return stock;
}
ubc::BatchResultObservation done(const ubc::MaterialPreview& expected){
  ubc::BatchResultObservation result;
  result.identity_known=result.stock_known=result.hands_known=result.station_known=result.ended=true;
  result.world=1;result.hero=2;result.stored=expected.remaining;
  result.hands.left.state=result.hands.right.state=0;
  return result;
}
}
int main(){
  using R=ubc::OriginalRecipe;using S=ubc::BatchResultState;using I=ubc::BatchResultIssue;
  for(const auto recipe:{R::meat,R::healing_essence})for(const auto quantity:{1u,2u,7u,23u}){
    const auto expected=ubc::material_preview(recipe,initial(recipe),quantity);
    ubc::BatchResult result;CHECK(result.permits_next());CHECK(result.begin(recipe,expected,1,2,3));
    CHECK(!result.permits_next());CHECK(!result.begin(recipe,expected,1,2,3));
    auto observation=done(expected);observation.ended=false;observation.station=3;
    observation.hands.right={1,recipe==R::meat?16608:16604,1};
    CHECK(result.observe(observation)==S::waiting);CHECK(!result.permits_next());
    observation.ended=true;CHECK(result.observe(observation)==S::waiting); // An end event while the station is still bound is insufficient.
    observation=done(expected);CHECK(result.observe(observation)==S::verified);CHECK(result.permits_next());
    CHECK(result.begin(recipe,expected,1,2,3));CHECK(result.observe(observation)==S::verified);
  }
  for(unsigned bad=0;bad<7;++bad){
    auto expected=ubc::material_preview(R::healing_essence,initial(R::healing_essence),7);
    ubc::BatchResult result;CHECK(result.begin(R::healing_essence,expected,1,2,3));
    auto observation=done(expected);
    if(bad<6)++observation.stored[bad];else observation.hands.right={1,16604,1};
    CHECK(result.observe(observation)==S::failed);CHECK(!result.permits_next());
    CHECK(!result.begin(R::healing_essence,expected,1,2,3));
    CHECK(result.observe(done(expected))==S::failed);
  }
  for(unsigned missing=0;missing<4;++missing){
    auto expected=ubc::material_preview(R::meat,initial(R::meat),7);
    ubc::BatchResult result;CHECK(result.begin(R::meat,expected,1,2,3));auto observation=done(expected);
    if(missing==0)observation.identity_known=false;
    if(missing==1)observation.stock_known=false;
    if(missing==2)observation.hands_known=false;
    if(missing==3)observation.station_known=false;
    CHECK(result.observe(observation)==S::waiting);CHECK(!result.permits_next());
    CHECK(result.fail(I::timeout)==S::failed);CHECK(result.issue()==I::timeout);
  }
  for(unsigned changed=0;changed<3;++changed){
    auto expected=ubc::material_preview(R::meat,initial(R::meat),7);
    ubc::BatchResult result;CHECK(result.begin(R::meat,expected,1,2,3));auto observation=done(expected);
    if(changed==0)observation.world=99;
    if(changed==1)observation.hero=99;
    if(changed==2)observation.station=99;
    CHECK(result.observe(observation)==S::failed);CHECK(result.issue()==I::context);
  }
  for(const auto recipe:{R::meat,R::healing_essence}){
    auto expected=ubc::material_preview(recipe,initial(recipe),7);
    ubc::BatchResult result;CHECK(result.begin(recipe,expected,1,2,3));auto observation=done(expected);
    observation.ended=false;observation.reopened=true;observation.station=3;
    observation.hands.right={1,16608,1};
    CHECK(result.observe(observation)==(recipe==R::meat?S::verified:S::waiting));
  }
  std::printf("BatchResult: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
