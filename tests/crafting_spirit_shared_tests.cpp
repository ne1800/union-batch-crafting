// SPDX-License-Identifier: MIT
#include "ubc/crafting_handoff.hpp"
#include "ubc/crafting_result.hpp"
#include "ubc/native_crafting_ui.hpp"
#include <cstdio>
#include <cstring>
using namespace ubc;
unsigned checks=0,failures=0;
#define CHECK(v) do{++checks;if(!(v)){++failures;std::printf("FAIL line %d: %s\n",__LINE__,#v);}}while(false)
CraftingSnapshot snapshot(std::int32_t branch){
 CraftingSnapshot x;x.key={1,7,0x9000,0xa000,0xb000,0xc000,crafting_catalog::Station::alchemy};
 x.hands.hands={{0,-1,-1,0},{1,16604,1,0xd000}};
 x.hands.left_slot_known=x.hands.right_slot_known=x.hands.interact_item_known=true;x.hands.interact_item=0xd000;
 auto& s=x.recipes[crafting_catalog::spirit_begin];s.generation=1;
 s.evaluated=s.knowledge_read_valid=s.known=s.contract=s.actor_known=s.actor_matches=s.hand_known=s.material_known=s.material_verified=s.context_known=s.context_matches=true;
 s.maximum=50;s.branch_known=true;s.branch_value=branch;s.branch_output=branch==1?15741:15739;
 const auto& d=crafting_catalog::recipes[crafting_catalog::spirit_begin];const auto outputs=crafting_catalog::outputs_for(d,true,branch);
 for(std::size_t i=0;i<s.target_count;++i){auto& r=s.materials[i];r.stored=100;r.consumed_reserve=r.symbol==16604?1:0;r.unit_output=crafting_catalog::units(outputs,r.symbol);}
 return x;
}
int main(){
 CHECK(crafting_catalog::recipe_count>=73&&crafting_catalog::recipes[crafting_catalog::spirit_begin].id==0x7001);
 CHECK(std::strcmp(ubc::ui_text::de::table.item(15682),"Pfefferbeutel")==0);
 CHECK(std::strcmp(ubc::ui_text::de::table.item(16130),"Stachel")==0);
 for(const auto branch:{0,1,2,3,INT32_MIN,INT32_MAX}){
  auto snap=snapshot(branch);auto& state=snap.recipes[crafting_catalog::spirit_begin];
  CHECK(valid_crafting_snapshot(snap));CHECK(crafting_block_reason(state)==CraftingBlockReason::none);
  const CraftingRequest request{0x7001,2,snap.key,state,snap.hands};
  const auto source=crafting_page(recipe_navigation::Page::root);
  const auto plan=make_crafting_plan(source,request.id,2,true,branch);
  CHECK(valid_crafting_plan(plan));CHECK(plan.effect()&&plan.effect()->function_symbol==20370);
  CHECK(!make_crafting_plan(source,request.id,2).valid());
  auto other=make_crafting_plan(source,request.id,2,true,branch==2?3:2);
  CraftingHandoff rejected;CHECK(!rejected.begin(request,other,{}));CHECK(rejected.issue()==CraftingHandoffIssue::plan);
  auto unknown=state;unknown.branch_known=false;CHECK(!valid_crafting_state(unknown));
  auto wrong=state;wrong.branch_output=branch==1?15739:15741;CHECK(!valid_crafting_state(wrong));
  const CraftingResultBinding binding{1,10,4,0,0x9000,0xa000,0xb000};
  const CraftingResultEndWitness end{true,true,1,1,11,0x9000,0xa000,0xb000};
  std::array<RecipeResultItem,crafting_catalog::target_capacity> totals{};
  for(std::size_t i=0;i<state.target_count;++i)totals[i]={state.materials[i].symbol,crafting_amount_preview(state.materials[i],2).end_storage};
  CraftingResultObservation obs;obs.generation=1;obs.frame=12;obs.stable_boundary_verified=obs.identity_known=obs.station_known=obs.production_known=obs.stock_known=obs.hands_known=true;
  obs.world=0x9000;obs.hero=0xa000;obs.items={totals.data(),state.target_count};
  obs.hands.hands={{0,-1,-1,0},{0,-1,-1,0}};
  obs.hands.left_slot_known=obs.hands.right_slot_known=obs.hands.interact_item_known=true;
  obs.branch={true,true,1,0x7001,2,branch};
  for(unsigned mode=0;mode<7;++mode){CraftingResult guard;CHECK(guard.begin(request,binding));
   CHECK(guard.restored(true,{1,0x7001,2,true,true}));CHECK(guard.end_after(end));auto current=obs;
   if(mode==1)current.branch.known=false;
   if(mode==2)current.branch.writer_result_verified=false;
   if(mode==3)current.branch.value=branch==2?3:2;
   if(mode==4)current.branch.generation++;
   if(mode==5)current.branch.recipe++;
   if(mode==6)current.branch.quantity++;
   const auto result=guard.observe(current);
   CHECK(result==(mode==0?CraftingResultState::verified:mode==2?CraftingResultState::waiting:CraftingResultState::failed));
   CraftingResultSuccess receipt;
   if(mode){CHECK(!guard.success(receipt));continue;}
   CHECK(guard.success(receipt)&&receipt.produced_count==1&&receipt.produced[0].symbol==state.branch_output&&receipt.produced[0].amount==2);
   NativeCraftingSuccess ui;ui.result_verified=ui.restore_verified=true;ui.receipt=1;ui.key=request.key;ui.id=request.id;ui.quantity=2;ui.produced_count=1;ui.stock_count=receipt.stock_count;
   ui.branch_known=true;ui.branch_value=branch;ui.branch_output=state.branch_output;ui.produced[0]={receipt.produced[0].symbol,2};
   for(std::size_t i=0;i<ui.stock_count;++i)ui.stocks[i]={receipt.stocks[i].symbol,receipt.stocks[i].stored};
   CHECK(crafting_ui::valid_success(ui));crafting_ui::Frame frame;CHECK(crafting_ui::compose_success(frame,ui,1280,720,ubc::ui_text::de::table));
   bool named=false;for(std::size_t line=0;line<frame.count;++line)named=named||std::strstr(frame.lines[line].text.data(),"Trank der Geistesveraenderung");CHECK(named);
   ui.branch_known=false;CHECK(!crafting_ui::valid_success(ui));ui.branch_known=true;ui.produced[0].symbol=branch==1?15739:15741;CHECK(!crafting_ui::valid_success(ui));
  }
  for(unsigned late=0;late<4;++late){CraftingResult guard;CHECK(guard.begin(request,binding));CHECK(guard.restored(true,{1,0x7001,2,true,true}));CHECK(guard.end_after(end));
   auto first=obs;first.branch.writer_result_verified=false;CHECK(guard.observe(first)==CraftingResultState::waiting);
   auto next=obs;next.frame++;auto changed_totals=totals;
   if(late==1){changed_totals[0].total++;next.items={changed_totals.data(),state.target_count};}
   if(late==2)next.hands_known=false;
   if(late==3)next.hands.hands.right={1,16604,1,0xd000};
   CHECK(guard.observe(next)==(late==0?CraftingResultState::verified:late==2?CraftingResultState::waiting:CraftingResultState::failed));
  }
  CraftingResult no_restore;CHECK(no_restore.begin(request,binding));CHECK(!no_restore.restored(true));
 }
 auto a=snapshot(2),b=snapshot(3);CHECK(!same_crafting_state(a.recipes[crafting_catalog::spirit_begin],b.recipes[crafting_catalog::spirit_begin]));
 CraftingSession session;CHECK(session.open(a,0x7001));session.observe(1,false,true);session.observe(2,false,true);CHECK(session.adjust(3));CHECK(session.quantity()==4);CHECK(!session.refresh(b));CHECK(session.quantity()==1&&!session.armed());
 std::printf("spirit shared: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
