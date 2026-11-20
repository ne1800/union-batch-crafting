// SPDX-License-Identifier: MIT
#include "ubc/core/recipe_material.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {
namespace c=ubc::recipe_catalog;
unsigned checks=0,failures=0;
void check(bool value,const char* name){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",name);}}
struct Fixture {
  c::Descriptor descriptor;
  std::vector<ubc::RecipeInventoryItem> items;
  std::vector<std::uint32_t> objects;
  std::vector<ubc::RecipeMaterialRow> rows;
  ubc::HandsSnapshot hands;
  explicit Fixture(const c::Descriptor& recipe):descriptor(recipe),items(recipe.targets.count),rows(recipe.targets.count){
    hands={{0,-1,-1,0},{1,recipe.hand.active_item_symbol,static_cast<std::int32_t>(recipe.hand.amount),0x008ff000}};
    for(std::size_t i=0;i<items.size();++i){items[i].symbol=recipe.targets[i].symbol;set(items[i].symbol,0,0);}
    for(const auto& input:recipe.costs)set(input.symbol,static_cast<int>(
      std::min<std::uint64_t>(INT32_MAX,std::uint64_t{input.units}*7)),0);
    for(const auto& output:recipe.outputs)set(output.symbol,3,0);
    if(recipe.hand.from_inventory)set(recipe.hand.active_item_symbol,6,0);
  }
  ubc::RecipeInventoryItem& item(int symbol){
    for(auto& entry:items)if(entry.symbol==symbol)return entry;
    std::abort();
  }
  ubc::RecipeMaterialRow& row(int symbol){
    for(auto& entry:rows)if(entry.symbol==symbol)return entry;
    std::abort();
  }
  void set(int symbol,int listed,int packed){
    auto& entry=item(symbol);entry.listed=listed;entry.packed=packed;
    entry.total=static_cast<int>(static_cast<std::int64_t>(listed)+packed);
    entry.listed_occurrences=listed>0?1:0;entry.packed_occurrences=packed>0?1:0;
    entry.duplicate_packed_target=false;entry.has_active_marker=false;
    entry.mixed_representation=entry.listed_occurrences&&entry.packed_occurrences;
  }
  ubc::RecipeInventory snapshot(){
    ubc::RecipeInventory result;
    result.header_known=result.listed_known=result.packed_known=result.storage_totals_known=true;
    result.stable_boundary_verified=result.snapshot_unchanged=true;
    objects.clear();
    for(const auto& entry:items){
      for(unsigned i=0;i<entry.listed_occurrences;++i)objects.push_back(0x10000u+static_cast<unsigned>(objects.size())*16u);
      result.packed_entries+=entry.packed_occurrences;
    }
    result.listed_nodes=static_cast<std::uint32_t>(objects.size());
    result.items={items.data(),items.size()};result.listed_objects={objects.data(),objects.size()};
    return result;
  }
  ubc::RecipeMaterial evaluate(unsigned quantity,bool verified=true){
    return ubc::recipe_material(descriptor,snapshot(),hands,quantity,rows.data(),rows.size(),verified);
  }
};
}

int main(){
  for(const auto& recipe:c::recipes){
    Fixture f(recipe);auto result=f.evaluate(3);
    check(result.known()&&result.craftable&&result.maximum==7,"all nine descriptors use their exact per-recipe costs");
    check(result.count==recipe.targets.count,"all descriptor targets have one output row");
    for(const auto& cost:recipe.costs){
      const auto& row=f.row(cost.symbol);
      check(row.available_known&&row.required==static_cast<std::int64_t>(cost.units)*3&&row.missing==0,
            "requested ingredient costs are scaled exactly");
      check(row.remaining==row.available-row.required,"ingredient remainder conserves stored plus reserve");
    }
    for(const auto& output:recipe.outputs){
      const auto& row=f.row(output.symbol);
      check(row.produced==static_cast<std::int64_t>(output.units)*3&&row.remaining==row.stored+row.produced,
            "per-recipe output and remainder are independent of old global six targets");
    }
    const auto& hand=f.row(recipe.hand.active_item_symbol);
    check(hand.held==1&&hand.reserved==(recipe.hand.from_inventory?1:0),"one verified hand object reserved only when it is an ingredient");
    result=f.evaluate(0);
    check(result.known()&&!result.craftable&&result.maximum==7,"zero amount has known stock but never authorizes production");
    result=f.evaluate(8);
    check(result.known()&&!result.craftable&&result.maximum==7,"unavailable amount stays explainable without granting production");
    std::reverse(f.items.begin(),f.items.end());
    result=f.evaluate(7);
    check(result.known()&&result.craftable,"inventory targets are matched by symbol, not accidental position");
    result=f.evaluate(1,false);
    check(result.inventory_known&&!result.hand_known&&!result.craftable&&result.maximum==0,
          "descriptor evidence never substitutes explicit runtime hand proof");
    if(recipe.hand.from_inventory)
      check(!f.row(recipe.hand.active_item_symbol).available_known&&f.row(recipe.hand.active_item_symbol).stored==6,
            "unknown hand reserve does not erase known stored flask count or invent availability");
    check(f.row(recipe.hand.active_item_symbol).held==-1,"unverified held amount stays signed unknown");
    if(recipe.hand.from_inventory)
      check(f.row(recipe.hand.active_item_symbol).reserved==-1&&f.row(recipe.hand.active_item_symbol).missing_for_one==-1,
            "unverified hand reserve never becomes an invented known shortage");
  }
  const auto& meat=*c::find(c::RecipeId::meat);
  const auto& health=*c::find(c::RecipeId::health_01);
  {
    Fixture f(meat);f.set(16377,99,0);f.set(16379,INT32_MAX-3,0);
    auto r=f.evaluate(3);
    check(r.known()&&r.maximum==3&&r.craftable,"output headroom caps valid quantity instead of rejecting all stock");
    check(f.row(16379).remaining==INT32_MAX,"capped maximum reaches exact int32 output bound");
    r=f.evaluate(4);
    check(r.known()&&!r.craftable&&f.row(16379).output_shortfall==1,"one above output limit is explained precisely");
    f.set(16379,INT32_MAX,0);r=f.evaluate(0);
    check(r.known()&&r.maximum==0&&!r.craftable&&f.row(16379).output_shortfall_for_one==1,
          "full output stack remains known at N0");
  }
  {
    Fixture f(health);for(const auto& input:health.costs)f.set(input.symbol,INT32_MAX,0);
    f.set(health.outputs[0].symbol,0,0);auto r=f.evaluate(INT32_MAX);
    check(r.known()&&r.maximum==INT32_MAX&&r.craftable,"large stored count plus hand is computed in int64");
    check(f.row(16604).available==static_cast<std::int64_t>(INT32_MAX)+1&&f.row(16604).remaining==1,
          "logical flask total beyond int32 never overflows or double counts the hand");
    f.set(16604,0,0);r=f.evaluate(1);
    check(r.known()&&r.maximum==1&&r.craftable,"single reserved hand flask suffices without stored flasks");
    const auto herb=health.costs[0].symbol;f.set(herb,0,0);r=f.evaluate(0);
    check(r.known()&&r.maximum==0&&!r.craftable,"known zero stock is distinct from unknown inventory");
    check(f.row(herb).available_known&&f.row(herb).available==0&&f.row(herb).missing_for_one==health.costs[0].units,
          "N0 exposes the exact ingredient missing for one production");
    check(f.row(herb).required==0&&f.row(herb).produced==0,"N0 never claims a material effect");
  }
  {
    const auto& descriptor=*c::find(c::RecipeId::molerat_dish);Fixture f(descriptor);
    f.set(descriptor.costs[1].symbol,14,0);auto r=f.evaluate(3);
    check(r.known()&&r.maximum==2&&!r.craftable,"molerat cost five is not confused with five output portions");
    check(f.row(descriptor.costs[1].symbol).required==15&&f.row(descriptor.costs[1].symbol).missing==1,
          "molerat deficient ingredient remains individually explainable");
    for(const auto& cost:descriptor.costs)f.set(cost.symbol,INT32_MAX,0);
    f.set(descriptor.outputs[0].symbol,0,0);r=f.evaluate(static_cast<unsigned>(INT32_MAX)/5);
    check(r.known()&&r.maximum==static_cast<unsigned>(INT32_MAX)/5&&r.craftable,
          "native signed amount bound applies to scaled ingredient consumption");
    check(f.row(descriptor.costs[0].symbol).required==INT32_MAX-2,"large scaled cost remains exact");
    r=f.evaluate(static_cast<unsigned>(INT32_MAX)/5+1);
    check(!r.craftable&&f.row(descriptor.costs[0].symbol).missing==3,"first overflowing native ingredient amount rejected without wrap");
  }
  {
    const auto& descriptor=*c::find(c::RecipeId::mushroom_dish);Fixture f(descriptor);
    f.set(descriptor.costs[0].symbol,49,0);const auto r=f.evaluate(0);
    check(r.known()&&r.maximum==0&&f.row(descriptor.costs[0].symbol).missing_for_one==1,
          "known N0 explains forty-nine mushrooms against recipe cost fifty");
  }
  for(unsigned variant=0;variant<7;++variant){
    Fixture f(meat);auto inv=f.snapshot();
    switch(variant){
      case 0:inv.header_known=false;break;case 1:inv.listed_known=false;break;
      case 2:inv.packed_known=false;break;case 3:inv.storage_totals_known=false;break;
      case 4:inv.stable_boundary_verified=false;break;case 5:inv.snapshot_unchanged=false;break;
      case 6:inv.issue=ubc::PackedInventoryIssue::changed_during_read;break;
    }
    auto r=ubc::recipe_material(meat,inv,f.hands,1,f.rows.data(),f.rows.size(),true);
    check(!r.inventory_known&&!r.known()&&!r.craftable,"all complete/stable reader flags are required");
  }
  for(unsigned variant=0;variant<8;++variant){
    Fixture f(meat);auto& entry=f.item(16377);
    switch(variant){
      case 0:entry.total=-1;break;case 1:entry.listed=-1;break;case 2:entry.packed=-1;break;
      case 3:entry.total+=1;break;case 4:entry.listed=INT32_MAX;entry.packed=1;entry.total=INT32_MAX;break;
      case 5:entry.listed_occurrences=0;break;case 6:entry.symbol=9876;break;
      case 7:f.items.back().symbol=f.items.front().symbol;break;
    }
    const auto r=f.evaluate(1);check(!r.inventory_known&&!r.craftable,"corrupt sums, provenance or target mapping rejected");
  }
  for(const auto& input:health.costs)for(unsigned variant=0;variant<6;++variant){
    Fixture f(health);auto& entry=f.item(input.symbol);
    switch(variant){
      case 0:entry.duplicate_packed_target=true;break;
      case 1:entry.has_active_marker=true;break;
      case 2:entry.mixed_representation=true;break;
      case 3:entry.packed=2;entry.total+=2;entry.packed_occurrences=2;entry.duplicate_packed_target=true;entry.mixed_representation=true;break;
      case 4:entry.listed_occurrences=2;break;
      case 5:entry.packed=1;entry.total+=1;entry.packed_occurrences=1;entry.mixed_representation=true;break;
    }
    check(!f.evaluate(1).craftable,"ambiguous ingredients never authorize native GetAmount-dependent recipes");
  }
  {
    Fixture f(health);const auto output=health.outputs[0].symbol;f.set(output,1,37);
    auto& entry=f.item(output);entry.packed_occurrences=2;entry.duplicate_packed_target=true;entry.has_active_marker=true;
    const auto r=f.evaluate(3);
    check(r.known()&&r.craftable&&f.row(output).stored==38&&f.row(output).remaining==41,
          "mixed or duplicate output storage is summed and does not masquerade as an ingredient");
  }
  for(const auto id:{c::RecipeId::meat,c::RecipeId::molerat_dish,c::RecipeId::mushroom_dish}){
    const auto& descriptor=*c::find(id);
    for(unsigned storage=0;storage<3;++storage){
      Fixture f(descriptor);
      f.set(16607,storage==0?6:0,storage==1?6:0);
      if(storage==2)f.set(16607,2,4);
      const auto r=f.evaluate(3);
      check(r.known()&&r.hand_known&&r.craftable&&r.maximum==7,
            "stored ordinary pans remain valid tools and are not the generated PANFULL cleanup target");
      check(f.row(16607).stored==6&&f.row(16607).remaining==6&&f.row(16607).reserved==0,
            "ordinary PAN inventory is neither consumed nor reserved by this recipe");
    }
    for(unsigned storage=0;storage<4;++storage){
      Fixture f(descriptor);f.set(16607,0,6);
      if(storage==0)f.set(16608,1,0);
      if(storage==1)f.set(16608,0,1);
      if(storage==2)f.set(16608,1,1);
      if(storage==3)f.item(16608).listed_occurrences=1;
      const auto r=f.evaluate(3);
      check(r.inventory_known&&!r.hand_known&&!r.craftable&&r.maximum==0&&
            r.issue==ubc::RecipeMaterialIssue::hand_prop_stock,
            "stored generated PANFULL cannot pass the original DestroyInteractItem cleanup contract");
      check(f.row(16607).stored==6&&f.row(16608).stored==(storage==2?2:storage==3?0:1),
            "blocked generated-prop state retains the separately known storage amounts");
      const auto preview=f.evaluate(0);
      check(preview.inventory_known&&!preview.known()&&!preview.craftable&&preview.maximum==0,
            "N0 explains storage but does not certify an unsafe generated hand-prop lifecycle");
    }
  }
  for(unsigned variant=0;variant<15;++variant){
    Fixture f(health);auto inv=f.snapshot();
    switch(variant){
      case 0:f.hands.left.state=-1;break;case 1:f.hands.left={1,16604,1,0x88000};break;
      case 2:f.hands.right.state=0;break;case 3:f.hands.right.state=2;break;
      case 4:f.hands.right.amount=2;break;case 5:f.hands.right.symbol=16608;break;
      case 6:f.hands.right.object=0;break;case 7:f.objects[0]=f.hands.right.object;break;
      case 8:inv.listed_objects.count-=1;break;case 9:f.objects[1]=f.objects[0];break;
      case 10:f.hands.left.object=f.hands.right.object;break;
      case 11:f.hands.right.state=-1;break;
      case 12:inv.listed_objects.data=nullptr;break;
      case 13:inv.listed_objects.count+=1;break;
      case 14:f.objects[0]=0;break;
    }
    const auto r=ubc::recipe_material(health,inv,f.hands,1,f.rows.data(),f.rows.size(),true);
    check(!r.hand_known&&!r.craftable,"exact hands and complete nonaliasing unique item list required");
  }
  {
    Fixture f(meat);auto inv=f.snapshot();
    check(!ubc::recipe_material(meat,inv,f.hands,1,f.rows.data(),f.rows.size()-1,true).known(),"short output buffer rejected");
    check(!ubc::recipe_material(meat,inv,f.hands,1,nullptr,f.rows.size(),true).known(),"null output buffer rejected");
    inv.items.count-=1;
    check(!ubc::recipe_material(meat,inv,f.hands,1,f.rows.data(),f.rows.size(),true).known(),"missing target is unknown, not zero");
    check(!f.evaluate(UINT32_MAX).craftable&&f.row(16379).produced==-1,"quantity beyond VM int32 range rejected without arithmetic");
    auto alias=f.snapshot();f.objects[0]=f.hands.right.object;
    check(!ubc::recipe_material(meat,alias,f.hands,1,f.rows.data(),f.rows.size(),true).hand_known,
          "tool hand alias is rejected even when tool is not a reserved ingredient");
  }
  for(const auto evidence:{c::HandEvidence::unknown,c::HandEvidence::inferred_same_station,c::HandEvidence::observed_original}){
    auto descriptor=health;descriptor.hand.evidence=evidence;Fixture f(descriptor);
    check(!f.evaluate(1,false).craftable,"every evidence label requires separate current runtime hand verification");
    check(f.evaluate(1,true).craftable,"explicit verified common hand contract does not pretend to be a new world-test label");
  }
  {
    auto descriptor=meat;descriptor.hand.amount=2;Fixture f(descriptor);
    check(!f.evaluate(1,true).hand_known&&!f.evaluate(1,true).craftable,
          "descriptor plus boolean does not invent an unproved two-item hand contract");
    descriptor=meat;descriptor.hand.start_item_symbol=16608;Fixture wrong(descriptor);
    check(!wrong.evaluate(1,true).hand_known&&!wrong.evaluate(1,true).craftable,
          "unproved pan transition rejected independently of actual current item");
  }
  for(unsigned variant=0;variant<4;++variant){
    Fixture f(meat);auto inv=f.snapshot();
    if(variant==0)inv.listed_nodes=ubc::max_packed_inventory_nodes+1;
    if(variant==1)inv.packed_entries=ubc::max_packed_inventory_entries+1;
    if(variant==2)inv.listed_nodes=0;
    if(variant==3){f.item(16377).listed=0;f.item(16377).packed=7;f.item(16377).packed_occurrences=0;}
    check(!ubc::recipe_material(meat,inv,f.hands,1,f.rows.data(),f.rows.size(),true).inventory_known,
          "reader counts and occurrence provenance cannot contradict known storage");
  }
  {
    // More than six targets: explicit synthetic metadata of our own.
    // No claim about authorization for additional actual scripts.
    std::vector<ubc::PackedInventoryTarget> targets(meat.targets.begin(),meat.targets.end());
    std::vector<ubc::lhiver::SymbolBinding> bindings(meat.bindings.begin(),meat.bindings.end());
    const char* names[]={"SYNTHETIC_A","SYNTHETIC_B","SYNTHETIC_C","SYNTHETIC_D"};
    for(unsigned i=0;i<4;++i){targets.push_back({names[i],static_cast<int>(30000+i)});
      bindings.push_back({names[i],30000+i,7,1,0,1584});}
    auto descriptor=meat;descriptor.targets={targets.data(),targets.size()};descriptor.bindings={bindings.data(),bindings.size()};
    Fixture f(descriptor);auto r=f.evaluate(7);
    check(r.known()&&r.craftable&&r.count==8,"per-recipe evaluator has no inherited global six-target limit");
    const std::array<c::ItemAmount,2> outputs{{{16379,2},{30000,3}}};
    descriptor.outputs=c::view(outputs);Fixture two(descriptor);
    two.set(16377,99,0);two.set(16379,INT32_MAX-21,0);two.set(30000,INT32_MAX-14,0);
    r=two.evaluate(4);
    check(r.known()&&r.craftable&&r.maximum==4,"minimum headroom across every output limits quantity");
    check(two.row(16379).produced==8&&two.row(30000).produced==12,"different output multipliers evaluated independently");
    r=two.evaluate(5);
    check(!r.craftable&&two.row(30000).output_shortfall==1,"multi-output overflow is exact at first invalid N");
  }
  std::printf("RecipeMaterial: %u checks, %u failures\n",checks,failures);
  return failures?1:0;
}
