// SPDX-License-Identifier: MIT
#define UBC_ALCHEMY_EXTRA_EXECUTION_TEST_EMBEDDED
#include "alchemy_extra_execution_tests.cpp"
#include "ubc/stations/alchemy/crafting_alchemy_extra_reader.hpp"

namespace catalogue_test {
namespace detail=ubc::crafting_alchemy_extra_detail;
using Point=ubc::AlchemyExtraReadPoint;
using Issue=ubc::CraftingAlchemyExtraReadIssue;
struct AllFixture:execution_test::ExecutionFixture {
  ubc::CraftingAlchemyExtraInput input{};
  std::vector<std::pair<std::int32_t,std::int32_t>> totals;
  explicit AllFixture(const LocalFacts* facts=nullptr,const std::array<std::string,4>* texts=nullptr)
    :ExecutionFixture(cat::recipes[0],facts,texts) {
    for(const auto& r:cat::recipes) {
      install(execution_test::e::recipe_profile(r),facts);
      for(const auto& q:r.requirements) {
        const auto* b=cat::binding_for(r,static_cast<std::uint32_t>(q.symbol));CHECK(b!=nullptr);
        if(b->elements>1) {
          const auto array=execution_test::arrays+static_cast<std::uint32_t>(q.symbol)*0x80;
          if(!image.at(array,b->elements*4))image.block(array,b->elements*4);
          image.put(symbol(static_cast<std::uint32_t>(q.symbol))+0x18,array);image.put(array+q.element*4,static_cast<std::uint32_t>(q.value));
        } else image.put(symbol(static_cast<std::uint32_t>(q.symbol))+0x18,static_cast<std::uint32_t>(q.value));
      }
      for(const auto& t:r.targets) {
        bool exists=false;for(const auto& pair:totals)if(pair.first==t.symbol)exists=true;
        if(!exists)totals.push_back({t.symbol,t.symbol==16604?5:100});
      }
    }
    for(const auto& b:execution_test::nav::base::flag_bindings)image.put(symbol(b.symbol)+0x18,0);
    set_production(4);image.put(symbol(1679)+0x1c,hero);image.put(symbol(1683)+0x1c,hero);
    storage(totals,"");image.block(ubc::station_abi::information_manager,0x58);
    image.put(ubc::station_abi::information_manager+0x20,hero);image.put(ubc::station_abi::information_manager+0x24,hero);
    input.context=boundary.context;input.key={1,7,world,hero,mob,ubc::station_abi::information_manager,ubc::crafting_catalog::Station::alchemy};
    input.serial=20;input.current_thread=7;input.boundary={Point::post_render,true,true,0,0,0,0};
    input.targets=ubc::alchemy_extra_dependencies::expected_targets;input.restore_item=0x7a2f20;
  }
  std::uint32_t item(std::int32_t target) const {
    for(std::uint32_t i=0;i<totals.size();++i)if(totals[i].first==target)return object(i);
    std::abort();
  }
};
ubc::CraftingAlchemyExtraReader reader{7};
ubc::CraftingAlchemyExtraInput nested_input{};
void reenter_catalogue(Image& image) noexcept {
  const auto before=image.read_count;const auto nested=reader.capture(image.memory(),nested_input);
  CHECK(nested.diagnostic.issue==Issue::reentry && image.read_count==before);
}
void boundary_cases() {
  auto reject=[](auto mutate,Issue issue) {
    AllFixture f;mutate(f);const auto before=f.image.read_count;const auto out=reader.capture(f.image.memory(),f.input);
    CHECK(out.diagnostic.issue==issue && f.image.read_count==before);
    for(const auto& snapshot:out.capture.recipes)CHECK(!snapshot.can_select && !snapshot.maximum_executable);
  };
  reject([](AllFixture& f){f.input.current_thread=8;},Issue::thread);
  reject([](AllFixture& f){f.input.boundary.verified=false;},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary.point=Point::unknown;},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary.vm_depth=1;},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary.writer_depth=1;},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary.debit_depth=1;},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary.callback_depth=1;},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary.point=Point::before_original;},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary.point=Point::selection_resolved;},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary.resolve_depth=1;},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary={Point::selection_resolved,true,true,0,0,0,0,2};},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary={Point::selection_resolved,true,true,0,0,1,0,1};},Issue::boundary);
  reject([](AllFixture& f){f.input.boundary={Point::before_original,true,true,2,0,1,0};},Issue::boundary);
  reject([](AllFixture& f){f.input.key.station++;},Issue::boundary);
  reject([](AllFixture& f){f.input.key.manager++;},Issue::boundary);
  reject([](AllFixture& f){f.input.key.generation++;},Issue::boundary);
  reject([](AllFixture& f){f.input.key.kind=ubc::crafting_catalog::Station::forge;},Issue::boundary);
  reject([](AllFixture& f){f.input.serial=0;},Issue::boundary);
  AllFixture f;nested_input=f.input;f.image.callback=reenter_catalogue;
  CHECK(reader.capture(f.image.memory(),f.input).diagnostic.issue==Issue::native);
}
void reader_cases(const LocalFacts* facts=nullptr,const std::array<std::string,4>* strings_ptr=nullptr) {
  AllFixture f(facts,strings_ptr);auto out=reader.capture(f.image.memory(),f.input);
  CHECK(ubc::crafting_alchemy_extra_target_capacity==31 && out.diagnostic.target_count==31);
  CHECK(out.capture.boundary_verified && out.capture.context_unchanged && out.capture.actor_unchanged && out.capture.hands_unchanged);
  CHECK(out.diagnostic.inventory_unchanged && out.diagnostic.ingredient_objects_verified);
  CHECK(out.capture.actor_before.requires_self && out.capture.actor_before.matches && out.capture.actor_after.matches);
  CHECK(out.capture.actor_purpose==ubc::AlchemyExtraActorPurpose::hero_preview && out.capture.physical_hero_preview_verified);
  CHECK(out.capture.production_before==4 && out.capture.production_after==4);
  CHECK(out.capture.manager_before==ubc::station_abi::information_manager && out.capture.manager_after==out.capture.manager_before);
  for(std::size_t i=0;i<cat::recipes.size();++i) {
    const auto& s=out.capture.recipes[i];CHECK(out.capture.evaluated[i] && s.id==cat::recipes[i].id && s.quantity==0 && s.serial==20);
    CHECK(s.learned_known && s.learned && s.material.known() && s.material.maximum==6 && s.extra_flask_stack_verified);
    CHECK(s.profile_verified==(facts!=nullptr));CHECK(!s.native_debit_verified && s.maximum_executable==0 && !s.can_select);
    CHECK(s.rows[s.material.count-1].stored==100);
    const auto n=ubc::quantify_alchemy_extra_snapshot(s,3);CHECK(!n.can_select && n.quantity==3);
  }
  // Original files can establish local profiles. They cannot establish the
  // missing loaded export, allocator or mod dispatchers in this fixture.
  CHECK(out.diagnostic.issue==Issue::native && !out.diagnostic.dependencies.valid);
  const auto initial=ubc::project_capture(out.capture);
  f.image.put(symbol(20361)+0x18,1);auto next=reader.capture(f.image.memory(),f.input);
  const auto alternate=ubc::project_capture(next.capture);
  for(std::size_t i=ubc::crafting_catalog::alchemy_extra_begin;i<ubc::crafting_catalog::anvil_begin;++i)
    CHECK(ubc::same_crafting_state(initial.recipes[i],alternate.recipes[i]));
  f.image.put(symbol(20361)+0x18,0);f.input.boundary={Point::before_original,true,true,1,0,1,0};
  CHECK(reader.capture(f.image.memory(),f.input).capture.context_unchanged);
  f.input.boundary={Point::selection_resolved,true,true,0,0,0,0,1};
  CHECK(reader.capture(f.image.memory(),f.input).capture.context_unchanged);
  f.input.boundary={Point::post_render,true,true,0,0,0,0};
  f.image.put(symbol(1679)+0x1c,hero+4);next=reader.capture(f.image.memory(),f.input);
  CHECK(next.diagnostic.issue==Issue::native && next.capture.actor_unchanged && !next.capture.actor_before.matches);
  CHECK(next.capture.actor_before.self==hero+4 && next.capture.actor_after.self==hero+4 && next.capture.physical_hero_preview_verified);
  const auto preview_state=ubc::project_capture(next.capture);
  for(std::size_t i=ubc::crafting_catalog::alchemy_extra_begin;i<ubc::crafting_catalog::anvil_begin;++i)
    CHECK(preview_state.recipes[i].actor_known && preview_state.recipes[i].actor_matches);
  for(const auto& s:next.capture.recipes)CHECK(s.learned_known && s.learned && !s.native_debit_verified);
  CHECK(!reader.capture_recipe(f.image.memory(),f.input,cat::recipes[0].id,2).effect_ready);
  f.input.boundary={Point::selection_resolved,true,true,0,0,0,0,1};next=reader.capture(f.image.memory(),f.input);
  CHECK(next.capture.actor_purpose==ubc::AlchemyExtraActorPurpose::hero_preview && next.capture.physical_hero_preview_verified && !next.capture.actor_before.matches);
  f.input.boundary={Point::before_original,true,true,1,0,1,0};next=reader.capture(f.image.memory(),f.input);
  CHECK(next.diagnostic.issue==Issue::actor && next.capture.actor_purpose==ubc::AlchemyExtraActorPurpose::original_entry &&
    !next.capture.physical_hero_preview_verified && !next.capture.actor_before.matches);
  CHECK(!reader.capture_recipe(f.image.memory(),f.input,cat::recipes[0].id,2).effect_ready);
  f.input.boundary={Point::post_render,true,true,0,0,0,0};f.image.put(symbol(1683)+0x1c,hero+4);
  CHECK(!reader.capture(f.image.memory(),f.input).capture.physical_hero_preview_verified);
  f.image.put(symbol(1683)+0x1c,hero);f.input.boundary.physical_lab_hand_contract_verified=false;
  CHECK(!reader.capture(f.image.memory(),f.input).capture.physical_hero_preview_verified);
  f.input.boundary.physical_lab_hand_contract_verified=true;
  f.image.put(symbol(1679)+0x1c,hero);
  // Keep the known limitation visible when the local profiles are verified.
  const auto ingredient=cat::recipes[0].costs[0].symbol;f.image.put(f.item(ingredient)+0x32c,0);
  next=reader.capture(f.image.memory(),f.input);CHECK(next.capture.recipes[0].material.known() && next.capture.recipes[0].material.maximum==0);
  CHECK(next.capture.recipes[0].visible==(facts!=nullptr));f.image.put(f.item(ingredient)+0x32c,100);
  f.image.put(extra_slot+0x30,f.item(16604));f.image.put(hero+0x9b8,3);
  next=reader.capture(f.image.memory(),f.input);CHECK(!next.diagnostic.ingredient_objects_verified && !next.capture.recipes[0].extra_flask_stack_verified);
  f.image.put(extra_slot+0x30,0);f.image.put(hero+0x9b8,2);f.image.put(f.item(ingredient)+0x158,0x40200001);
  CHECK(!reader.capture(f.image.memory(),f.input).diagnostic.ingredient_objects_verified);
  f.image.put(f.item(ingredient)+0x158,0x200001);
  // Change after the inventory snapshot, during the object metadata read.
  f.image.watch=f.item(ingredient);f.image.change_on=1;f.image.change_target=f.item(ingredient)+0x32c;f.image.change_value=7;
  next=reader.capture(f.image.memory(),f.input);CHECK(!next.diagnostic.inventory_unchanged);
  AllFixture changed(facts,strings_ptr);changed.image.watch=changed.item(ingredient);changed.image.change_on=1;changed.image.change_target=hero+0x968;changed.image.change_value=0;
  CHECK(!reader.capture(changed.image.memory(),changed.input).capture.hands_unchanged);
  AllFixture unlearned(facts,strings_ptr);const auto& q=cat::recipes[0].requirements[0];
  // The local catalog supplies the actual knowledge condition. No new skill flag.
  bool changed_knowledge=false;
  for(const auto& requirement:cat::recipes[0].requirements)if(requirement.role==cat::RequirementRole::knowledge) {
    const auto* binding=cat::binding_for(cat::recipes[0],static_cast<std::uint32_t>(requirement.symbol));
    const auto addr=binding->elements>1?execution_test::arrays+static_cast<std::uint32_t>(requirement.symbol)*0x80+requirement.element*4:
      symbol(static_cast<std::uint32_t>(requirement.symbol))+0x18;
    unlearned.image.put(addr,0);changed_knowledge=true;break;
  }
  (void)q;CHECK(changed_knowledge);next=reader.capture(unlearned.image.memory(),unlearned.input);
  CHECK(next.capture.recipes[0].learned_known && !next.capture.recipes[0].learned && !next.capture.recipes[0].visible);
  AllFixture blocked(facts,strings_ptr);blocked.image.denied=hero+0x724;
  next=reader.capture(blocked.image.memory(),blocked.input);CHECK(!next.capture.recipes[0].material.known());
  blocked.image.denied=0;blocked.image.put(game+8,world+4);
  CHECK(reader.capture(blocked.image.memory(),blocked.input).diagnostic.issue==Issue::context);
  CHECK(!reader.capture_recipe(blocked.image.memory(),blocked.input,cat::recipes[0].id,2).can_select);
}

ubc::AlchemyExtraSnapshot preview(const cat::Descriptor& r) {
  ubc::AlchemyExtraSnapshotInput in;in.context={1,7,ubc::lhiver::game_parser,game,world,hero,mob};in.serial=1;
  in.boundary_verified=in.local_recipe_contract_verified=in.hand_contract_verified=in.interact_item_known=in.native_debit_contract_verified=true;
  in.hands={{0,-1,-1,0},{1,16604,1,hand_object}};in.interact_item=hand_object;
  in.knowledge.bindings_verified=in.knowledge.unchanged=true;in.knowledge.count=r.requirements.count;
  for(std::size_t i=0;i<r.requirements.count;++i) {const auto& q=r.requirements[i];in.knowledge.values[i]={q.symbol,q.element,q.value};}
  std::array<ubc::RecipeInventoryItem,cat::catalog_target_capacity> items{};std::array<std::uint32_t,cat::catalog_target_capacity> objects{};
  for(std::size_t i=0;i<r.targets.count;++i) {
    const auto target=r.targets[i].symbol,amount=target==16604?5:100;
    items[i]={target,amount,0,amount,1,0,false,false,false};objects[i]=object(static_cast<std::uint32_t>(i));
    if(target==16604)in.flask={true,true,true,0,1,objects[i],5,0x200001};
  }
  in.inventory.header_known=in.inventory.listed_known=in.inventory.packed_known=in.inventory.storage_totals_known=true;
  in.inventory.stable_boundary_verified=in.inventory.snapshot_unchanged=true;in.inventory.listed_nodes=static_cast<std::uint32_t>(r.targets.count);
  in.inventory.items={items.data(),r.targets.count};in.inventory.listed_objects={objects.data(),r.targets.count};
  return ubc::alchemy_extra_snapshot(r,in,0);
}
void quantity_cases() {
  for(const auto& recipe:cat::recipes) {
    const auto p=preview(recipe);CHECK(p.material.known() && p.maximum_executable==6);
    for(const auto n:{0u,1u,2u,5u,6u,7u,static_cast<unsigned>(INT32_MAX),UINT32_MAX}) {
      const auto q=ubc::quantify_alchemy_extra_snapshot(p,n);CHECK(q.quantity==n && q.material.quantity==n);
      CHECK(q.can_select==(n>0 && n<=6) && q.effect_ready==q.can_select);
      CHECK(q.profile_verified==p.profile_verified && q.native_debit_verified==p.native_debit_verified);
      if(n<=INT32_MAX)for(std::size_t i=0;i<recipe.targets.count;++i) {
        const auto& row=q.rows[i];CHECK(row.required==std::int64_t{row.unit_cost}*n && row.produced==std::int64_t{row.unit_output}*n);
        CHECK(row.remaining==row.available-row.required+row.produced);
      }
    }
    auto changed=p;changed.maximum_executable=4;CHECK(ubc::quantify_alchemy_extra_snapshot(changed,4).can_select && !ubc::quantify_alchemy_extra_snapshot(changed,5).can_select);
    changed=p;changed.owner_conditions_match=false;CHECK(ubc::quantify_alchemy_extra_snapshot(changed,3).can_select && !ubc::quantify_alchemy_extra_snapshot(changed,3).effect_ready);
    changed=p;changed.native_debit_verified=false;CHECK(!ubc::quantify_alchemy_extra_snapshot(changed,3).can_select);
    changed=p;changed.rows[0].unit_cost++;CHECK(!ubc::quantify_alchemy_extra_snapshot(changed,2).maximum_executable);
    changed=p;changed.rows[0].stored++;CHECK(!ubc::quantify_alchemy_extra_snapshot(changed,2).maximum_executable);
    changed=p;changed.quantity=1;CHECK(!ubc::quantify_alchemy_extra_snapshot(changed,2).can_select);
  }
  namespace dep=ubc::alchemy_extra_dependencies;debit::FlaskStack flask{true,true,true,0,1,0x8000,5,0x200001};
  dep::ListEvidence split;split.stable_complete=split.unpack_unreachable=true;split.path=dep::RemovalPath::split;split.amount=5;
  dep::ListEvidence whole=split;whole.path=dep::RemovalPath::whole;whole.whole_short_path=true;
  CHECK(detail::executable(0,flask,{},whole)==0);CHECK(detail::executable(6,flask,{},whole)==1);
  CHECK(detail::executable(6,flask,split,{})==5);CHECK(detail::executable(6,flask,split,whole)==6);
  CHECK(detail::executable(3,flask,split,{})==3);whole.amount=4;CHECK(detail::executable(6,flask,split,whole)==5);
  flask.amount=1;split.amount=1;split.path=dep::RemovalPath::whole;CHECK(detail::executable(2,flask,split,{})==1);
  split.whole_short_path=true;whole=split;CHECK(detail::executable(2,flask,split,whole)==2);
}
}
int main(int argc,char** argv) {
  (void)&execution_test::local; // Reuse the fixture without rerunning the other suite.
  catalogue_test::boundary_cases();catalogue_test::reader_cases();catalogue_test::quantity_cases();
  if(argc==4 && std::strcmp(argv[1],"--local-facts")==0) {
    LocalFacts facts(argv[2],argv[3]);const auto texts=execution_test::read_prints(argv[3]);catalogue_test::reader_cases(&facts,&texts);
  } else if(argc!=1)return 2;
  std::printf("%u checks, %u failures; no positive loaded module dispatch fabricated\n",checks,failures);return failures?1:0;
}
