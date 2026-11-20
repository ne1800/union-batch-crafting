// SPDX-License-Identifier: MIT
#include "ubc/anvil_recipe_catalog.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
namespace a=ubc::anvil_recipe_catalog;
unsigned checks=0,failures=0;
void check(bool value,const char* name){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",name);}}
void quantity_tests(){
  check(!a::runtime_ready,"isolated catalogue is never runtime permission");
  check(a::find(static_cast<a::RecipeId>(0))==nullptr,"foreign recipe ID absent");
  for(const auto& r:a::recipes){
    check(a::structurally_valid(r),"canonical exact profile is internally consistent");
    check(a::find(r.id)==&r&&a::find_callback(r.callback.function->symbol)==&r,"only owned chosen callback resolves");
    check(!a::find_callback(r.original_three.function->symbol),"original three-offer is not a second writer template");
    check(r.owner.number==-1,"omitted C_INFO.NR retains proven constructor default minus one");
    for(const auto n:{1u,2u,3u,5u,300u,static_cast<unsigned>(INT32_MAX)}){
      const auto q=a::plan_quantity(r,n);
      check(q.valid&&q.quantity==n&&q.callback==&r.callback&&q.patch_original==(n>1),"one linear callback covers N without retry or callback loop");
      check(q.callback->function->symbol!=r.original_three.function->symbol,"N3 still uses single chosen original template");
    }
    for(const auto n:{0u,static_cast<unsigned>(INT32_MAX)+1u,UINT32_MAX})
      check(!a::plan_quantity(r,n).valid,"zero and out-of-int32 quantities cannot form a plan");
    for(const auto raw:{0,1,5,300,INT32_MAX})for(const auto output:{0,1,INT32_MAX-1,INT32_MAX}){
      const auto limit=a::numeric_limit(r,raw,output);
      const auto expected=static_cast<std::uint32_t>(raw<INT32_MAX-output?raw:INT32_MAX-output);
      check(limit.valid&&limit.maximum==expected,"actual available raw and remaining output room both constrain N");
    }
    check(a::numeric_limit(r,INT32_MAX,INT32_MAX-2).maximum==2,"overflowing all-stock does not reject a valid smaller N");
    check(a::numeric_limit(r,0,0).valid&&a::numeric_limit(r,0,0).maximum==0,"known zero ingredients remain known zero");
    for(const auto invalid:{INT64_C(-1),INT64_C(2147483648)}){
      check(!a::numeric_limit(r,invalid,0).valid,"invalid raw input is not a known zero");
      check(!a::numeric_limit(r,1,invalid).valid,"invalid output input is not a known zero");
    }
    check(r.hand.left.symbol==16666&&!r.hand.left.require_stored_absent&&r.hand.right.symbol==16435&&
      r.hand.right.require_stored_absent,"only the observed right hammer anchor must be absent from storage");
    check(r.hand.anchor==a::HandSide::right,"original anvil entry binds the observed right hand anchor");
    check(r.hand.left_tmp_level==1&&r.hand.right_tmp_level==1,
      "both observed temporary slots are exact and cannot become a reserved inventory tool");
    check(r.minimum_actor==a::Actor::self&&r.material_actor==a::Actor::hero&&r.require_self_is_hero,
      "original SELF minimum and HERO effects require explicit current identity binding");
    check(r.completion==a::Completion::end_production&&!r.hand.static_lifecycle_verified&&r.hand.observed_in_world,
      "observed original entry is separate from a completed static lifecycle and production permission");
  }
}
struct Copy {
  a::Descriptor r;
  std::vector<ubc::PackedInventoryTarget> targets;
  std::vector<ubc::lhiver::SymbolBinding> bindings;
  std::vector<a::Requirement> requirements,conditions;
  std::vector<a::RequirementCheck> checks_at;
  std::vector<a::ItemAmount> costs,outputs;
  std::vector<ubc::AmountOperand> operands;
  std::vector<a::OperandUse> uses;
  std::vector<a::MaterialEffect> effects;
  std::vector<a::Effect> success,failure;
  std::vector<a::PrintFingerprint> prints;
  std::vector<const a::Function*> items;
  std::vector<a::NativeRange> native;
  a::StationProfile station;
  a::Function callback;
  explicit Copy(const a::Descriptor& from):r(from),targets(from.targets.begin(),from.targets.end()),
    bindings(from.bindings.begin(),from.bindings.end()),requirements(from.requirements.begin(),from.requirements.end()),
    conditions(from.owner.conditions.begin(),from.owner.conditions.end()),checks_at(from.checks.begin(),from.checks.end()),
    costs(from.costs.begin(),from.costs.end()),outputs(from.outputs.begin(),from.outputs.end()),
    operands(from.callback.amount.operands,from.callback.amount.operands+from.callback.amount.count),
    uses(from.operand_uses.begin(),from.operand_uses.end()),effects(from.callback.effects.begin(),from.callback.effects.end()),
    success(from.success.begin(),from.success.end()),failure(from.failure.begin(),from.failure.end()),
    prints(from.prints.begin(),from.prints.end()),items(from.item_constructors.begin(),from.item_constructors.end()),
    native(from.native_dependencies.begin(),from.native_dependencies.end()),station(*from.station_profile),callback(*from.callback.function){
    r.targets={targets.data(),targets.size()};r.bindings={bindings.data(),bindings.size()};
    r.requirements={requirements.data(),requirements.size()};r.owner.conditions={conditions.data(),conditions.size()};
    r.checks={checks_at.data(),checks_at.size()};r.costs={costs.data(),costs.size()};r.outputs={outputs.data(),outputs.size()};
    r.callback.amount.operands=operands.data();r.operand_uses={uses.data(),uses.size()};r.callback.effects={effects.data(),effects.size()};
    r.success={success.data(),success.size()};r.failure={failure.data(),failure.size()};r.prints={prints.data(),prints.size()};
    r.item_constructors={items.data(),items.size()};r.native_dependencies={native.data(),native.size()};
    r.station_profile=&station;r.callback.function=&callback;
  }
  Copy(const Copy&)=delete;Copy& operator=(const Copy&)=delete;
};
void corruption_tests(){
  for(const auto& source:a::recipes){
    Copy clone(source);check(a::structurally_valid(clone.r),"value-identical independently owned metadata is accepted");
    for(unsigned change=0;change<88;++change){Copy c(source);
      switch(change){
        case 0:c.r.id=static_cast<a::RecipeId>(0);break;
        case 1:c.r.id=a::recipes[(static_cast<std::size_t>(&source-&a::recipes[0])+1)%a::recipes.size()].id;break;
        case 2:c.r.station=static_cast<a::Station>(10);break;
        case 3:c.r.production_context=10;break;
        case 4:c.r.name=nullptr;break;
        case 5:c.r.completion=a::Completion::ready_same_station;break;
        case 6:c.r.minimum_actor=a::Actor::hero;break;
        case 7:c.r.material_actor=a::Actor::self;break;
        case 8:c.r.require_self_is_hero=false;break;
        case 9:c.r.owner.number=0;break;
        case 10:++c.r.owner.instance;break;
        case 11:++c.r.owner.npc;break;
        case 12:c.r.owner.important=true;break;
        case 13:c.r.owner.trade=true;break;
        case 14:c.r.owner.permanent=false;break;
        case 15:c.r.owner.condition=c.r.owner.information;break;
        case 16:c.r.back=c.r.callback.function;break;
        case 17:c.r.callback.function=c.r.original_three.function;break;
        case 18:c.r.callback.invocation=a::Invocation::info;break;
        case 19:c.r.callback.original_quantity=3;break;
        case 20:c.r.callback.minimum_quantity=0;break;
        case 21:c.r.callback.maximum_quantity=UINT32_MAX;break;
        case 22:c.r.callback.ends_production=false;break;
        case 23:c.r.callback.body_checks_resources=false;break;
        case 24:c.r.callback.clears_choices=true;break;
        case 25:c.r.callback.quantity_text_reviewed=false;break;
        case 26:c.r.callback.amount.digest[31]^=1;break;
        case 27:--c.r.callback.amount.length;break;
        case 28:c.r.original_three.quantity=1;break;
        case 29:c.r.original_three.function=c.r.callback.function;break;
        case 30:c.r.hand.anchor=a::HandSide::left;break;
        case 31:c.r.hand.anchor=a::HandSide::unknown;break;
        case 32:c.r.hand.static_lifecycle_verified=true;break;
        case 33:c.r.hand.observed_in_world=false;break;
        case 34:c.r.hand.left.require_stored_absent=true;break;
        case 35:c.r.hand.right.require_stored_absent=false;break;
        case 36:c.r.hand.right.source=a::HandSource::reserved_inventory_tool;break;
        case 37:c.r.hand.right.symbol=16667;break;
        case 38:c.r.hand.left.amount=0;break;
        case 39:c.station.state_num=2;break;
        case 40:c.station.rewind=1;break;
        case 41:c.station.use_with_item="ITMISWORDRAWHOT";break;
        case 42:c.station.on_state="ORESMELTING";break;
        case 43:c.station.scheme="BSFIRE";break;
        case 44:c.station.visual="BSANVIL_OTHER.MDS";break;
        case 45:c.station.mob_condition=c.r.owner.condition;break;
        case 46:c.station.entry=c.r.callback.function;break;
        case 47:c.station.end.number=-1;break;
        case 48:c.conditions[1].element=30;break;
        case 49:c.conditions[1].role=a::RequirementRole::context;break;
        case 50:c.requirements[1].value=0;break;
        case 51:c.requirements[3].role=a::RequirementRole::knowledge;break;
        case 52:c.requirements[4].value=0;break;
        case 53:c.requirements[4].kind=a::RequirementKind::integer_equal;break;
        case 54:c.checks_at[1].site=a::CheckSite::callback;break;
        case 55:c.checks_at[4].function=c.r.owner.condition;break;
        case 56:c.costs[0].symbol=c.outputs[0].symbol;break;
        case 57:c.outputs[0].symbol=16611;break;
        case 58:c.costs[0].units=2;break;
        case 59:c.outputs[0].units=0;break;
        case 60:c.targets[1]=c.targets[0];break;
        case 61:c.targets[2].name="ITMI_PLIERS";break;
        case 62:c.operands[0].offset=11;c.uses[0].offset=11;break;
        case 63:c.operands[1].units=2;break;
        case 64:c.operands[2].reserved=1;break;
        case 65:c.operands[0].allow_zero=true;break;
        case 66:c.operands[0].original=0;break;
        case 67:c.uses[1].role=a::AmountRole::create_count;break;
        case 68:c.effects[0].action=a::MaterialAction::create;break;
        case 69:c.effects[1].symbol=16611;break;
        case 70:c.success[1].actor=a::Actor::self;break;
        case 71:c.success[2].value=2;break;
        case 72:c.success[6].value=1;break;
        case 73:c.failure[1].kind=a::EffectKind::create;break;
        case 74:c.failure[2].offset=127;break;
        case 75:c.prints[0].symbol=14374;break;
        case 76:c.prints[1].sha256[0]^=1;break;
        case 77:c.items[1]=c.items[0];break;
        case 78:c.native[0].length-=1;break;
        case 79:c.native[1].sha256[31]^=1;break;
        case 80:c.r.callback.failure_effects={nullptr,1};break;
        case 81:c.bindings[0].flags^=1;break;
        case 82:c.bindings.back().type=2;break;
        case 83:c.r.station_profile=nullptr;break;
        case 84:c.r.hand.left_tmp_level=0;break;
        case 85:c.r.hand.right_tmp_level=0;break;
        case 86:c.r.hand.left_tmp_level=2;break;
        case 87:c.r.hand.right_tmp_level=-1;break;
      }
      check(!a::structurally_valid(c.r),"mutated semantic evidence cannot redefine its canonical recipe");
      check(!a::plan_quantity(c.r,2).valid&&!a::numeric_limit(c.r,9,1).valid,"invalid evidence cannot produce even a numeric recipe plan");
    }
    for(unsigned field=0;field<11;++field){Copy c(source);switch(field){
      case 0:c.callback.name=nullptr;break;case 1:++c.callback.symbol;break;case 2:++c.callback.byte_offset;break;
      case 3:--c.callback.byte_length;break;case 4:c.callback.body_sha256[7]^=1;break;case 5:c.callback.argc=1;break;
      case 6:c.callback.type=7;break;case 7:c.callback.flags=9;break;case 8:c.callback.return_type=2;break;
      case 9:c.callback.parent_index=1649;break;case 10:c.callback.name="OTHER_VOID_FUNCTION";break;
    }
      c.r.callback.amount.digest=c.callback.body_sha256;c.r.callback.amount.length=c.callback.byte_length;
      check(!a::structurally_valid(c.r),"internally matching altered callback hash/metadata does not bypass canonical identity");
    }
    for(unsigned which=0;which<13;++which)for(bool huge:{false,true}){Copy c(source);
      const auto count=huge?SIZE_MAX:std::size_t{0};
      switch(which){
        case 0:c.r.targets={nullptr,count};break;case 1:c.r.bindings={nullptr,count};break;
        case 2:c.r.requirements={nullptr,count};break;case 3:c.r.owner.conditions={nullptr,count};break;
        case 4:c.r.checks={nullptr,count};break;case 5:c.r.costs={nullptr,count};break;case 6:c.r.outputs={nullptr,count};break;
        case 7:c.r.callback.amount.operands=nullptr;c.r.callback.amount.count=count;break;
        case 8:c.r.operand_uses={nullptr,count};break;case 9:c.r.success={nullptr,count};break;
        case 10:c.r.failure={nullptr,count};break;case 11:c.r.item_constructors={nullptr,count};break;
        case 12:c.r.native_dependencies={nullptr,count};break;
      }
      check(!a::structurally_valid(c.r),"empty/oversized missing metadata is bounded before dereferencing views");
    }
  }
}
void export_facts(){
  for(const auto& f:a::functions){std::printf("F\t%s\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t",f.name,f.symbol,f.byte_offset,f.byte_length,f.argc,f.type,f.flags,f.return_type,f.parent_index);
    for(auto byte:f.body_sha256)std::printf("%02x",unsigned{byte});std::puts("");}
  for(const auto& r:a::recipes){
    for(const auto& b:r.bindings)std::printf("B\t%s\t%u\t%u\t%u\t%u\t%d\t%d\t%d\t%d\t%d\n",b.name,b.symbol,b.type,b.flags,b.elements,b.parent_index,b.check_value?1:0,b.value,b.check_offset?1:0,b.offset);
    for(std::size_t i=0;i<r.callback.amount.count;++i){const auto& op=r.callback.amount.operands[i];
      std::printf("O\t%u\t%u\t%d\t%u\t%u\t%d\n",r.callback.function->symbol,op.offset,op.original,op.units,op.reserved,op.allow_zero?1:0);}
    for(const auto& p:r.prints){std::printf("P\t%u\t%u\t",p.symbol,p.length);for(auto byte:p.sha256)std::printf("%02x",unsigned{byte});std::puts("");}
  }
  for(const auto& n:a::native_dependencies){std::printf("N\t%u\t%u\t",n.address,n.length);for(auto byte:n.sha256)std::printf("%02x",unsigned{byte});std::puts("");}
}
} // namespace
int main(int argc,char** argv){
  if(argc==2&&std::strcmp(argv[1],"--export-facts")==0){export_facts();return 0;}
  quantity_tests();corruption_tests();std::printf("anvil_recipe_catalog_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
