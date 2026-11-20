// SPDX-License-Identifier: MIT
// Our own memory fixture. Optional original data stays exclusively in RAM.
#define UBC_SPIRIT_EXECUTION_TEST_EMBEDDED
#include "spirit_execution_tests.cpp"
#include "ubc/spirit_crafting_reader.hpp"
#include <type_traits>

namespace spirit_crafting_test {
namespace sc=ubc::spirit_recipe_catalog;
namespace detail=ubc::spirit_crafting_detail;
using Point=ubc::AlchemyExtraReadPoint;
using Issue=ubc::SpiritCraftingIssue;
[[maybe_unused]] auto* const old_cases=&spirit_execution_test::local;
struct Fixture:spirit_execution_test::SpiritFixture {
  ubc::SpiritCraftingInput input{};
  unsigned lease_calls=0;std::uint32_t lease_value=execution_test::actual_info;
  bool expire=false,change_branch=false,replace_info=false;std::uint32_t changed_branch=2;
  explicit Fixture(const LocalFacts* facts=nullptr,const std::array<std::string,4>* texts=nullptr,std::int32_t branch=1)
    :SpiritFixture(facts,texts,branch) {
    image.block(ubc::station_abi::information_manager,0x58);
    image.put(ubc::station_abi::information_manager+0x20,hero);
    image.put(ubc::station_abi::information_manager+0x24,hero);
    input.common.context=boundary.context;
    input.common.key={1,7,world,hero,mob,ubc::station_abi::information_manager,ubc::crafting_catalog::Station::alchemy};
    input.common.serial=20;input.common.current_thread=7;
    input.common.boundary={Point::post_render,true,true,0,0,0,0};
    input.common.targets=ubc::alchemy_extra_dependencies::expected_targets;input.common.restore_item=0x7a2f20;
    input.info_context=this;input.read_actual_info=lease;
  }
  static std::uint32_t lease(void* p,const ubc::CraftingAlchemyExtraInput& in) noexcept {
    auto& self=*static_cast<Fixture*>(p);++self.lease_calls;
    CHECK(in.context.hero==hero&&in.key.station==mob&&in.serial==20);
    if(self.lease_calls==2&&self.change_branch)self.image.put(symbol(11967)+0x18,self.changed_branch);
    if(self.lease_calls==2&&self.replace_info)return self.lease_value+4;
    return self.expire&&self.lease_calls==2?0:self.lease_value;
  }
  void entry(){input.common.boundary={Point::before_original,true,true,1,0,1,0};}
};
ubc::SpiritCraftingReader reader{7};
ubc::SpiritCraftingInput reentry_input{};
void reenter(Image& image) noexcept {
  const auto reads=image.read_count;const auto out=reader.capture(image.memory(),reentry_input);
  CHECK(out.diagnostic.issue==Issue::reentry&&image.read_count==reads);
}
void boundaries(){
  static_assert(!std::is_copy_constructible<ubc::SpiritCraftingReader>::value,"persistent own workspace");
  for(unsigned mode=0;mode<12;++mode){Fixture f;
    switch(mode){case 0:f.input.common.current_thread=8;break;case 1:f.input.common.boundary.verified=false;break;
      case 2:f.input.common.key.generation=2;break;case 3:f.input.common.context.parser++;break;
      case 4:f.input.common.key.kind=ubc::crafting_catalog::Station::stove;break;case 5:f.input.common.serial=0;break;
      case 6:f.input.common.boundary.vm_depth=1;break;case 7:f.input.common.boundary.debit_depth=1;break;
      case 8:f.input.common.boundary.writer_depth=1;break;case 9:f.input.common.boundary.callback_depth=1;break;
      case 10:f.input.common.boundary.point=Point::selection_resolved;break;case 11:f.input.common.key.manager++;break;}
    const auto reads=f.image.read_count;const auto out=reader.capture(f.image.memory(),f.input);
    CHECK(out.diagnostic.issue==Issue::thread||out.diagnostic.issue==Issue::boundary);
    CHECK(f.image.read_count==reads&&!out.maximum_executable&&!out.recipe.state.evaluated);
  }
  Fixture f;reentry_input=f.input;f.image.callback=reenter;
  CHECK(reader.capture(f.image.memory(),f.input).recipe.state.evaluated);
}
const ubc::RecipeMaterialRow& row(const ubc::SpiritCraftingCapture& c,int symbol_id){for(const auto& r:c.recipe.state.rows)if(r.symbol==symbol_id)return r;std::abort();}
void captures(const LocalFacts* facts=nullptr,const std::array<std::string,4>* texts=nullptr){
  Fixture f(facts,texts);auto out=reader.capture(f.image.memory(),f.input);
  CHECK(out.boundary_verified&&out.context_unchanged&&out.recipe.state.known&&out.recipe.state.branch_known);
  CHECK(out.recipe.state.contract==(facts!=nullptr)&&out.recipe.state.material.known()&&out.recipe.state.material.maximum==50);
  CHECK(out.physical_hero_preview_verified&&out.actor_verified&&out.actor_purpose==ubc::AlchemyExtraActorPurpose::hero_preview);
  CHECK(!out.native_debit_verified&&!out.maximum_executable&&!out.actual_info_verified&&f.lease_calls==0);
  CHECK(out.diagnostic.flask.amount==100&&out.diagnostic.flask.object==object(4));
  CHECK(row(out,16604).stored==100&&row(out,16604).reserved==1&&row(out,15739).stored==3&&row(out,15741).stored==4);
  // SELF does not establish HERO in an idle menu. Keep the raw value visible.
  f.image.put(symbol(1679)+0x1c,hero+4);out=reader.capture(f.image.memory(),f.input);
  CHECK(out.actor_verified&&out.physical_hero_preview_verified&&!out.recipe.state.actor_matches);
  f.entry();out=reader.capture(f.image.memory(),f.input);CHECK(!out.actor_verified&&!out.maximum_executable);
  f.image.put(symbol(1679)+0x1c,hero);f.lease_calls=0;
  out=reader.capture(f.image.memory(),f.input);CHECK(out.actual_info_verified==(facts!=nullptr)&&f.lease_calls==2);
  CHECK(!out.physical_hero_preview_verified&&out.actor_verified);
  // The actual Info() parameter, never Manager.info. Even an old owner
  // can only be a stability observation. It cannot authorize the effect.
  for(const auto manager_info:{0u,0xDEAD1234u,execution_test::actual_info}){
    f.image.put(ubc::station_abi::information_manager+0x28,manager_info);f.lease_calls=0;
    out=reader.capture(f.image.memory(),f.input);CHECK(out.actual_info_verified==(facts!=nullptr)&&out.actual_info==(facts?execution_test::actual_info:0u));
  }
  for(unsigned mode=0;mode<9;++mode){Fixture x(facts,texts);x.entry();
    switch(mode){case 0:x.input.read_actual_info=nullptr;break;case 1:x.lease_value=0;break;
      case 2:x.expire=true;break;case 3:x.image.put(execution_test::actual_info+0x50,20371);break;
      case 4:x.image.put(execution_test::actual_info+0x2c,20373);break;case 5:x.image.put(execution_test::actual_info+0x58,1);break;
      case 6:x.image.put(symbol(20361)+0x18,0);break;case 7:x.change_branch=true;break;
      case 8:x.image.put(code+732061,0);break;}
    const auto bad=reader.capture(x.image.memory(),x.input);CHECK(!bad.actual_info_verified&&!bad.maximum_executable);
    CHECK(!ubc::spirit_crafting_quantity(bad,1).effect_ready);
  }
  {Fixture x(facts,texts);x.image.put(symbol(20361)+0x18,0);const auto c=reader.capture(x.image.memory(),x.input);
    CHECK(c.recipe.state.known&&c.recipe.state.context_matches&&!c.recipe.state.owner_conditions_match&&c.recipe.state.material.maximum==50);}
  {Fixture x(facts,texts,2);x.entry();x.change_branch=true;x.changed_branch=3;
    const auto c=reader.capture(x.image.memory(),x.input);CHECK(!c.recipe.state.branch_known&&!c.actual_info_verified&&!c.maximum_executable);}
  {Fixture x(facts,texts);x.entry();x.replace_info=true;const auto c=reader.capture(x.image.memory(),x.input);
    CHECK(!c.actual_info_verified&&!c.maximum_executable);}
  {Fixture x(facts,texts);x.input.common.boundary={Point::selection_resolved,true,true,0,0,0,0,1};
    x.image.put(symbol(1679)+0x1c,hero+4);const auto c=reader.capture(x.image.memory(),x.input);
    CHECK(c.physical_hero_preview_verified&&c.actor_verified&&!c.recipe.state.actor_matches&&x.lease_calls==0);}
  {Fixture x(facts,texts);x.image.put(ubc::station_abi::information_manager+0x20,hero+4);
    const auto c=reader.capture(x.image.memory(),x.input);CHECK(c.diagnostic.issue==Issue::context&&!c.recipe.state.evaluated&&!c.maximum_executable);}
  for(const auto branch:{INT32_MIN,-1,0,1,2,INT32_MAX}){Fixture x(facts,texts,branch);const auto c=reader.capture(x.image.memory(),x.input);
    CHECK(c.recipe.state.branch_value==branch&&c.recipe.state.output==(branch==1?15741:15739));}
  for(unsigned mode=0;mode<8;++mode){Fixture x(facts,texts);
    switch(mode){case 0:x.image.denied=symbol(11967);break;case 1:x.image.denied=symbol(11198);break;
      case 2:x.image.put(object(0)+0x32c,0);break;case 3:x.input.common.boundary.physical_lab_hand_contract_verified=false;break;
      case 4:x.image.put(hero+0x968,0);break;case 5:x.image.put(object(0)+0x158,0x40200001);break;
      case 6:x.image.put(hero+0x9b8,3);x.image.put(extra_slot+0x30,object(4));break;
      case 7:x.image.put(symbol(1683)+0x1c,hero+4);break;}
    const auto c=reader.capture(x.image.memory(),x.input);CHECK(!c.maximum_executable);
    if(mode==0)CHECK(c.recipe.state.known&&!c.recipe.state.branch_known&&row(c,16130).stored==100);
    if(mode==1)CHECK(!c.recipe.state.known&&c.recipe.state.material.known());
    if(mode==2)CHECK(c.recipe.state.known&&c.recipe.state.material.known()&&!c.recipe.state.material.maximum&&row(c,16130).missing_for_one==2);
    if(mode==3||mode==4||mode==6)CHECK(!c.recipe.state.material.hand_known||!c.recipe.state.material.ingredients_verified);
    if(mode==5)CHECK(!c.recipe.state.material.ingredients_verified);
    if(mode==7)CHECK(!c.actor_verified);
  }
  {Fixture x(facts,texts);x.storage({{16130,20},{16604,5}},"ITAT_STING:2,ITPO_MANA_02:20,ITPO_HEALTH_01:20,ITFO_ADDON_PFEFFER_01:20");
    const auto c=reader.capture(x.image.memory(),x.input);CHECK(c.recipe.state.material.issue==ubc::SpiritMaterialIssue::ambiguous_ingredient&&row(c,16130).stored==22);}
  {Fixture x(facts,texts);x.storage({{15739,2}},"ITAT_STING:20,ITPO_MANA_02:10,ITPO_HEALTH_01:10,ITFO_ADDON_PFEFFER_01:10,ITMI_FLASK:9,ITPO_ADDON_GEIST_01:3");
    const auto c=reader.capture(x.image.memory(),x.input);CHECK(c.recipe.state.material.known()&&row(c,15739).stored==5&&c.recipe.state.material.maximum==10);}
  {Fixture x(facts,texts);x.image.watch=object(0);x.image.change_on=1;x.image.change_target=object(0)+0x32c;x.image.change_value=7;
    CHECK(!reader.capture(x.image.memory(),x.input).recipe.diagnostic.inventory_unchanged);}
  CHECK(row(out,16604).stored==100); // The previous return value owns its data.
}
void material_phase(){
  // Value contract for the material gate. Do NOT fabricate module admission
  // for the actual reader. The existing captures() cases verify that it blocks.
  Fixture f;auto c=reader.capture(f.image.memory(),f.input);
  CHECK(!detail::material_maximum(c));
  c.recipe.state.contract=true;c.native_debit_verified=true;
  auto& split=c.diagnostic.split;split.stable_complete=split.unpack_unreachable=true;
  split.path=ubc::alchemy_extra_dependencies::RemovalPath::split;split.amount=c.diagnostic.flask.amount;
  CHECK(detail::material_maximum(c)==50);
  c.actor_purpose=ubc::AlchemyExtraActorPurpose::original_entry;c.physical_hero_preview_verified=false;
  c.maximum_executable=detail::material_maximum(c);
  CHECK(c.maximum_executable==50&&!c.actual_info_verified&&!c.actual_info);
  CHECK(ubc::spirit_crafting_quantity(c,2).selectable&&!ubc::spirit_crafting_quantity(c,2).effect_ready);
  c.actual_info_verified=true;c.actual_info=execution_test::actual_info;
  CHECK(ubc::spirit_crafting_quantity(c,2).effect_ready);
  for(unsigned mode=0;mode<7;++mode){auto bad=c;switch(mode){
    case 0:bad.actual_info_verified=false;break;case 1:bad.actual_info=0;break;
    case 2:bad.actor_purpose=ubc::AlchemyExtraActorPurpose::hero_preview;break;
    case 3:bad.recipe.state.actor_matches=false;break;case 4:bad.recipe.state.owner_conditions_match=false;break;
    case 5:bad.actor_verified=false;break;case 6:bad.native_debit_verified=false;break;}
    CHECK(!ubc::spirit_crafting_quantity(bad,2).effect_ready);
  }
  c.actual_info_verified=false;c.actual_info=0;c.actor_verified=false;
  CHECK(!detail::material_maximum(c));
}
void projection(){
  // Value contract only. No synthetic native authorization for the reader.
  Fixture f;auto c=reader.capture(f.image.memory(),f.input);
  c.recipe.state.contract=true;c.native_debit_verified=true;c.maximum_executable=5;
  for(unsigned n=0;n<=6;++n){const auto q=ubc::spirit_crafting_quantity(c,n);
    CHECK(q.valid&&q.selectable==(n>0&&n<=5)&&!q.effect_ready&&q.additional_flasks==(n?n-1:0));
    for(std::size_t i=0;i<7;++i)CHECK(q.rows[i].remaining==c.recipe.state.rows[i].available-std::int64_t{q.rows[i].unit_cost}*n+std::int64_t{q.rows[i].unit_output}*n);
  }
  c.actor_purpose=ubc::AlchemyExtraActorPurpose::original_entry;c.actual_info_verified=true;c.actual_info=execution_test::actual_info;
  CHECK(ubc::spirit_crafting_quantity(c,2).effect_ready);
  for(unsigned mode=0;mode<9;++mode){auto bad=c;
    switch(mode){case 0:bad.recipe.state.rows[0].stored++;break;case 1:bad.recipe.state.rows[0].unit_cost++;break;
      case 2:bad.recipe.state.rows[4].reserved=0;break;case 3:bad.recipe.state.branch_value=2;break;
      case 4:bad.recipe.state.material.maximum++;break;case 5:bad.maximum_executable=51;break;
      case 6:bad.recipe.state.rows[5].unit_output=1;break;case 7:bad.recipe.state.material.quantity=1;break;
      case 8:bad.recipe.state.rows[1].symbol=16130;break;}
    CHECK(!ubc::spirit_crafting_quantity(bad,2).valid);
  }
  CHECK(!ubc::spirit_crafting_quantity(c,UINT32_MAX).valid);
  namespace dep=ubc::alchemy_extra_dependencies;debit::FlaskStack flask{true,true,true,0,1,object(4),5,0x200001};
  dep::ListEvidence split;split.stable_complete=split.unpack_unreachable=true;split.path=dep::RemovalPath::split;split.amount=5;
  auto whole=split;whole.path=dep::RemovalPath::whole;whole.whole_short_path=true;
  whole.whole_world.verified=true;whole.whole_world.world=world;whole.whole_world.added_references=2;
  whole.whole_world.ref_before_release=3;whole.whole_world.pool_free_count=2;
  CHECK(detail::same(whole,whole));
  for(unsigned change=0;change<10;++change){auto changed=whole;
    switch(change){case 0:changed.whole_world.verified=false;break;case 1:changed.whole_world.world++;break;
      case 2:changed.whole_world.added_references--;break;case 3:changed.whole_world.ref_before_release--;break;
      case 4:changed.whole_world.pool_free_count--;break;case 5:changed.whole_world.nodes[0]++;break;
      case 6:changed.whole_world.nodes[1]++;break;case 7:changed.whole_world.contains_target[0]=true;break;
      case 8:changed.whole_world.contains_target[1]=true;break;case 9:changed.whole_world.fingerprint[0]^=1;break;}
    CHECK(!detail::same(whole,changed));}
  CHECK(detail::executable(6,flask,split,whole)==6&&detail::executable(6,flask,split,{})==5);
  CHECK(detail::executable(6,flask,{},whole)==1);flask.packed=1;CHECK(detail::executable(6,flask,split,whole)==1);
  flask.packed=0;flask.listed_stacks=2;CHECK(detail::executable(6,flask,split,whole)==1);
}
}
int main(int argc,char** argv){spirit_crafting_test::boundaries();spirit_crafting_test::captures();spirit_crafting_test::material_phase();spirit_crafting_test::projection();
  if(argc==4&&std::strcmp(argv[1],"--local-facts")==0){LocalFacts facts(argv[2],argv[3]);const auto texts=execution_test::read_prints(argv[3]);spirit_crafting_test::captures(&facts,&texts);}
  else if(argc!=1)return 2;
  std::printf("spirit_crafting_reader_tests: %u checks, %u failures; no fabricated loaded dependency admission\n",checks,failures);return failures?1:0;}
