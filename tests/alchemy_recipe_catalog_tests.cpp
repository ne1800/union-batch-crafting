// SPDX-License-Identifier: MIT
#include "ubc/catalogs/alchemy_recipe_catalog.hpp"
#include "ubc/stations/alchemy/alchemy_flask_debit.hpp"
#include <cstdio>
#include <cstring>
#include <vector>

namespace cat=ubc::alchemy_recipe_catalog;
namespace debit=ubc::alchemy_flask_debit;
namespace {
unsigned checks=0,failures=0;
void check(bool value,int line) {
  ++checks; if(!value) {++failures;std::fprintf(stderr,"failure line %d\n",line);}
}
#define CHECK(value) check((value),__LINE__)
const cat::Descriptor& recipe(cat::RecipeId id) { const auto* p=cat::find(id);CHECK(p!=nullptr);return *p; }
void facts() {
  // Separate facts output for private DAT comparison. No game bytes.
  std::puts("{\"functions\":["); bool first=true;
  for(const auto& f:cat::functions) {
    if(!first)std::puts(",");first=false;
    std::printf("{\"name\":\"%s\",\"symbol\":%u,\"start\":%u,\"length\":%u,\"type\":%u,\"flags\":%u,\"argc\":%u,\"return\":%u,\"parent\":%d,\"sha256\":\"",
      f.name,f.symbol,f.byte_offset,f.byte_length,f.type,f.flags,f.argc,f.return_type,f.parent_index);
    for(auto b:f.body_sha256)std::printf("%02x",static_cast<unsigned>(b));std::printf("\"}");
  }
  std::puts("],\"recipes\":[");first=true;
  for(const auto& r:cat::recipes) {
    if(!first)std::puts(",");first=false;
    std::printf("{\"id\":%u,\"callback\":%u,\"owner\":%d,\"number\":%d,\"costs\":[",static_cast<unsigned>(r.id),r.callback.function->symbol,r.owner.instance,r.owner.number);
    for(std::size_t i=0;i<r.costs.count;++i)std::printf("%s[%d,%u]",i?",":"",r.costs[i].symbol,r.costs[i].units);
    std::printf("],\"output\":[%d,%u],\"operands\":[",r.outputs[0].symbol,r.outputs[0].units);
    for(std::size_t i=0;i<r.callback.amount.count;++i) {const auto& a=r.callback.amount.operands[i];std::printf("%s[%u,%d,%u,%u]",i?",":"",a.offset,a.original,a.units,a.reserved);}
    std::printf("],\"requirements\":[");
    for(std::size_t i=0;i<r.requirements.count;++i) {const auto& q=r.requirements[i];std::printf("%s[%d,%u,%d]",i?",":"",q.symbol,q.element,q.value);}
    std::printf("]}");
  }
  std::puts("]}");
}
void inventory_and_amount_contracts() {
  CHECK(!cat::runtime_ready);CHECK(cat::recipes.size()==13);CHECK(cat::catalog_target_capacity==6);
  constexpr std::array<std::uint32_t,13> callbacks{{20289,20292,20295,20329,20360,20373,20376,20379,20382,20385,20395,20398,20408}};
  constexpr std::array<std::uint32_t,13> maximum{{1073741823,1073741823,2147483647,715827882,715827882,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,1073741823,214748364}};
  for(std::size_t i=0;i<cat::recipes.size();++i) {
    const auto& r=cat::recipes[i]; const auto* p=debit::find(callbacks[i]);
    CHECK(cat::structurally_valid(r));CHECK(p && debit::valid(*p));
    CHECK(static_cast<std::uint32_t>(r.id)==0x4001u+i);CHECK(r.callback.function->symbol==callbacks[i]);
    CHECK(cat::find_callback(callbacks[i])==&r);CHECK(r.owner.instance==static_cast<std::int32_t>(p->owner));
    CHECK(r.callback.invocation==cat::Invocation::info);CHECK(r.owner.information==r.callback.function);
    CHECK(r.outputs.count==1 && r.outputs[0].units==1 && r.outputs[0].symbol==static_cast<std::int32_t>(p->output));
    CHECK(r.costs.count==p->cost_count+1);CHECK(r.targets.count==p->cost_count+2);
    for(std::size_t j=0;j<p->cost_count;++j)CHECK(cat::units(r.costs,static_cast<std::int32_t>(p->costs[j].symbol))==p->costs[j].units);
    CHECK(cat::units(r.costs,16604)==1);
    CHECK(r.hand.from_inventory && r.hand.amount==1 && r.hand.left_must_be_empty && r.hand.consumed_on_success);
    CHECK(r.hand.evidence==cat::HandEvidence::inferred_same_station);
    CHECK(r.extra_flasks.after_verified_original && !r.extra_flasks.original_removes_stored_flasks);
    CHECK(r.callback.maximum_quantity==maximum[i]);
    CHECK(!cat::plan_quantity(r,0).valid);CHECK(!cat::plan_quantity(r,UINT32_MAX).valid);
    for(auto n:{1u,2u,7u,maximum[i]}) {
      auto plan=cat::plan_quantity(r,n);CHECK(plan.valid && plan.callback==&r.callback);
      CHECK(plan.extra_flasks==n-1 && plan.patch_original==(n>1));
      for(std::size_t j=0;j<r.callback.amount.count;++j) {
        const auto& op=r.callback.amount.operands[j];
        CHECK(std::uint64_t{op.units}*n<=INT32_MAX && !op.reserved && !op.allow_zero);
        CHECK(r.operand_uses[j].symbol!=16604); // Neither the refund nor the flask in hand is a patch target.
      }
    }
    CHECK(!cat::plan_quantity(r,maximum[i]+1).valid);
    CHECK(r.callback.failure_effects.count==1 && r.callback.failure_effects[0].symbol==16604 && r.callback.failure_effects[0].amount==1);
    CHECK(r.success[r.success.count-1].callee==19965 && r.failure[r.failure.count-1].callee==19965);
    CHECK(r.station_end->symbol==20279 && r.end_production->symbol==19965); // The abort refund follows its own path.
    for(const auto& f:r.item_constructors)CHECK(f->type==7 && f->parent_index==1584);
  }
  CHECK(cat::find(static_cast<cat::RecipeId>(0))==nullptr);CHECK(cat::find_callback(20401)==nullptr); // Paid tobacco is excluded.
}
void specific_rules() {
  using cat::RecipeId;using cat::EffectKind;
  const auto& lou=recipe(RecipeId::lou_spirit);
  CHECK(lou.material_actor==cat::Actor::self && lou.requirements[2].symbol==11325);
  CHECK(lou.success[4].kind==EffectKind::print && lou.success[5].kind==EffectKind::say && lou.success[6].kind==EffectKind::create);
  CHECK(cat::units(lou.costs,16704)==2 && cat::units(lou.costs,15673)==1);
  CHECK(cat::units(recipe(RecipeId::sleep_spirit).costs,15673)==2);
  for(auto id:{RecipeId::combined_health,RecipeId::combined_mana}) {
    const auto& r=recipe(id);CHECK(r.success[2].kind==EffectKind::create && r.success[3].kind==EffectKind::print);
    CHECK(r.requirements[1].symbol==1464 && (r.requirements[1].element==13 || r.requirements[1].element==14));
  }
  const auto& plain=recipe(RecipeId::swamp_roll);CHECK(plain.requirements.count==2 && plain.say);
  bool success_say=false;for(const auto& e:plain.success)success_say|=e.kind==EffectKind::say;
  CHECK(!success_say && plain.failure[1].kind==EffectKind::say);
  CHECK(plain.requirements[1].symbol==20386 && plain.requirements[1].role==cat::RequirementRole::context);
  CHECK(recipe(RecipeId::strong_swamp_roll).requirements[2].symbol==11211);
  const auto& egg=recipe(RecipeId::dragon_egg_potion);CHECK(egg.category==cat::Category::root);
  CHECK(cat::units(egg.costs,17585)==10 && cat::units(egg.costs,16659)==1 && cat::units(egg.costs,16652)==1);
  CHECK(egg.requirements.count==7);unsigned categories=0;
  for(const auto& q:egg.requirements)if(q.symbol==20280 || q.symbol==20299 || q.symbol==20330 || q.symbol==20361 || q.symbol==20386) {++categories;CHECK(q.value==0);}
  CHECK(categories==5);
  CHECK(std::strcmp(cat::station_scheme,"LAB")==0 && std::strcmp(cat::station_use_with_item,"ITMI_FLASK")==0);
}
void reject_broken_contracts() {
  {
    // An actual refund PUSHI(1) cannot substitute for a resource guard.
    const auto& source=cat::recipes[0];auto r=source;
    std::vector<ubc::AmountOperand> ops(source.callback.amount.operands,source.callback.amount.operands+source.callback.amount.count);
    std::vector<cat::OperandUse> roles(source.operand_uses.begin(),source.operand_uses.end());
    ops[0].offset=268;roles[0].offset=268;r.callback.amount.operands=ops.data();r.operand_uses={roles.data(),roles.size()};
    CHECK(!cat::structurally_valid(r));CHECK(!cat::plan_quantity(r,2).valid);
  }
  for(const auto& source:cat::recipes) {
    auto reject=[&](const auto& value){CHECK(!cat::structurally_valid(value));CHECK(!cat::plan_quantity(value,2).valid);};
    auto r=source;r.id=static_cast<cat::RecipeId>(0x5001);reject(r);
    r=source;r.owner.information=r.station_end;reject(r);
    r=source;r.owner.number+=1;reject(r);
    r=source;r.owner.condition=cat::recipes[(static_cast<std::uint32_t>(source.id)-0x4001u+1u)%cat::recipes.size()].owner.condition;reject(r);
    r=source;r.callback.invocation=cat::Invocation::choice;reject(r);
    r=source;r.callback.ends_production=false;reject(r);
    r=source;r.callback.amount.count+=1;reject(r);
    r=source;r.hand.evidence=cat::HandEvidence::observed_original;reject(r);
    r=source;r.extra_flasks.original_removes_stored_flasks=true;reject(r);
    r=source;r.extra_flasks.reserved_hand=0;reject(r);
    r=source;r.success={nullptr,4};reject(r);
    r=source;r.failure={nullptr,4};reject(r);
    r=source;r.costs={nullptr,4};reject(r);
    std::vector<cat::ItemAmount> costs(source.costs.begin(),source.costs.end());
    costs[0].symbol=source.outputs[0].symbol;r=source;r.costs={costs.data(),costs.size()};reject(r);
    costs.assign(source.costs.begin(),source.costs.end());costs.back().units=2;r=source;r.costs={costs.data(),costs.size()};reject(r);
    std::vector<cat::Requirement> req(source.requirements.begin(),source.requirements.end());
    req.back().value^=1;r=source;r.requirements={req.data(),req.size()};r.owner.conditions=r.requirements;reject(r);
    std::vector<ubc::AmountOperand> ops(source.callback.amount.operands,source.callback.amount.operands+source.callback.amount.count);
    ops[0].allow_zero=true;r=source;r.callback.amount.operands=ops.data();reject(r);
    ops[0]=source.callback.amount.operands[0];ops[0].reserved=1;reject(r);
    ops[0]=source.callback.amount.operands[0];ops[0].units+=1;reject(r);
    std::vector<cat::FixedEffect> refund(source.callback.failure_effects.begin(),source.callback.failure_effects.end());
    refund[0].amount=2;r=source;r.callback.failure_effects={refund.data(),refund.size()};reject(r);
    auto f=*source.callback.function;f.body_sha256[0]^=1;r=source;r.callback.function=&f;reject(r);
    f=*source.callback.function;f.argc=1;reject(r);
    std::vector<cat::OperandUse> roles(source.operand_uses.begin(),source.operand_uses.end());roles[0].symbol=16604;
    r=source;r.operand_uses={roles.data(),roles.size()};reject(r);
    std::vector<cat::Effect> effects(source.success.begin(),source.success.end());effects.back().callee=14646;
    r=source;r.success={effects.data(),effects.size()};reject(r);
    effects.assign(source.success.begin(),source.success.end());effects[0].actor=source.material_actor==cat::Actor::self?cat::Actor::hero:cat::Actor::self;reject(r);
    std::vector<cat::PrintFingerprint> prints(source.prints.begin(),source.prints.end());prints[0].digest[0]^=1;
    r=source;r.prints={prints.data(),prints.size()};reject(r);
  }
}
}
int main(int argc,char** argv) {
  if(argc==2 && std::strcmp(argv[1],"--facts")==0) {facts();return 0;}
  inventory_and_amount_contracts();specific_rules();reject_broken_contracts();
  std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
