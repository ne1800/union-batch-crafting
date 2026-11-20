// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/alchemy_extra_navigation.hpp"

// Own immutable spirit profile; two original outputs in ONE callback.
// No reinterpretation as a linear additional alchemy descriptor.
namespace ubc::spirit_recipe_catalog {
using recipe_catalog::View;using recipe_catalog::view;using recipe_catalog::ItemAmount;
using recipe_catalog::Owner;using recipe_catalog::Requirement;using recipe_catalog::RequirementKind;using recipe_catalog::RequirementRole;
using recipe_catalog::Actor;using recipe_catalog::HandContract;using recipe_catalog::FixedEffect;
using alchemy_recipe_catalog::AmountRole;using alchemy_recipe_catalog::OperandUse;using alchemy_recipe_catalog::Effect;
using alchemy_recipe_catalog::EffectKind;using alchemy_recipe_catalog::FlaskDebit;using alchemy_recipe_catalog::PrintFingerprint;
using lhiver::Function;using lhiver::SymbolBinding;using lhiver::digest_literal;
enum class RecipeId : std::uint32_t { spirit=0x7001 };
enum class Branch { unknown,ordinary,enriched };
inline constexpr bool runtime_ready=false;
inline constexpr std::size_t target_capacity=7;
inline constexpr std::uint32_t maximum_quantity=static_cast<std::uint32_t>(INT32_MAX)/2;
struct Variant { Branch branch;ItemAmount output;View<Effect> success; };
struct Descriptor {
  RecipeId id;const char* name;Owner owner;const Function* callback;AmountBody amount;
  View<PackedInventoryTarget> targets;View<SymbolBinding> bindings;View<Requirement> requirements;View<ItemAmount> costs;
  std::uint32_t maximum_quantity;std::int32_t branch_symbol,branch_equal;std::array<Variant,2> variants;
  HandContract hand;FlaskDebit extra_flasks;FixedEffect refund;View<Effect> failure;
  View<OperandUse> operand_uses;View<PrintFingerprint> prints;View<const Function*> item_constructors;
  const Function *station_entry,*station_end,*end_production,*say;bool require_self_is_hero;
};
inline constexpr std::array<Function,10> functions{{
  {"ITFO_ADDON_PFEFFER_01",15682,254251,161,digest_literal("38919c819a4df2ce1caaccf74328b603523aae113f4198ace87be448f4603c8c"),0,7,1,0,1584},
  {"ITPO_ADDON_GEIST_01",15739,265033,182,digest_literal("b2161c75e455df97abd5a973dda1910a6efefb3de0a4ec3b77091cae70c29d0b"),0,7,1,0,1584},
  {"ITPO_ADDON_GEIST_02",15741,265253,182,digest_literal("82f36b86caf356c175aae16c46acec077238d48c95a12ce99f0098763711e1ce"),0,7,1,0,1584},
  {"ITAT_STING",16130,298825,171,digest_literal("b8037c06440b2cca9d76138bdce7f097c71872a7ea9e3dca9e646f2b0612623a"),0,7,1,0,1584},
  {"ITMI_FLASK",16604,364987,102,digest_literal("c11d966476cbbe17ba211e07e822e245c415ad766f6c02d47382584666902ab0"),0,7,1,0,1584},
  {"ITPO_MANA_02",16769,375714,194,digest_literal("706867e451bfb85608061c1121ee9404e8054d45522cce036def34b705408a8e"),0,7,1,0,1584},
  {"ITPO_HEALTH_01",16775,376336,194,digest_literal("ae4105a34ccb79b0f9565a574ce560816331c3e5bc602904ad4da91fd4e5a5fc"),0,7,1,0,1584},
  {"PC_ITPO_ADDON_GEIST",20368,731947,67,digest_literal("6eb3031edd2320d8c281b200916465897d663a420b29bf31001af608e0aee2b8"),0,7,1,0,1649},
  {"PC_ITPO_ADDON_GEIST_CONDITION",20369,732014,47,digest_literal("241b4421ab690625f914b78203ca01919ab7069451de6a683a7d4146daee9fdf"),0,5,3,2,-1},
  {"PC_ITPO_ADDON_GEIST_INFO",20370,732061,324,digest_literal("f914d8abf7170458253e1bf7ca368c71122288b8525259d09e8257f1ce8ea96b"),0,5,1,0,-1},
}};
inline constexpr const Function* function_for(std::uint32_t symbol) noexcept {
  for(const auto& f:functions)if(f.symbol==symbol)return &f;
  return alchemy_recipe_catalog::function_for(symbol);
}
inline constexpr std::array<PackedInventoryTarget,7> targets{{
  {"ITAT_STING",16130},
  {"ITPO_MANA_02",16769},
  {"ITPO_HEALTH_01",16775},
  {"ITFO_ADDON_PFEFFER_01",15682},
  {"ITMI_FLASK",16604},
  {"ITPO_ADDON_GEIST_01",15739},
  {"ITPO_ADDON_GEIST_02",15741},
}};
inline constexpr std::array<SymbolBinding,14> bindings{{
  {"ITAT_STING",16130,7,1,0,1584,false,0,false,0},
  {"ITPO_MANA_02",16769,7,1,0,1584,false,0,false,0},
  {"ITPO_HEALTH_01",16775,7,1,0,1584,false,0,false,0},
  {"ITFO_ADDON_PFEFFER_01",15682,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITPO_ADDON_GEIST_01",15739,7,1,0,1584,false,0,false,0},
  {"ITPO_ADDON_GEIST_02",15741,7,1,0,1584,false,0,false,0},
  {"PC_ITPO_ADDON_GEIST",20368,7,1,0,1649,false,0,false,0},
  {"KNOWS_MCELIXIER",11198,2,0,1,-1,false,0,false,0},
  {"KNOWS_BLOODFLY",11967,2,0,1,-1,false,0,false,0},
  {"SPECIALSTART",20361,2,0,1,-1,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"PRINT_ALCHEMYSUCCESS",14372,3,1,1,-1,false,0,false,0},
  {"PRINT_PRODITEMSMISSING",14374,3,1,1,-1,false,0,false,0},
}};
inline constexpr std::array<Requirement,3> requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,4},
  {RequirementKind::integer_equal,RequirementRole::knowledge,11198,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,20361,0,1},
}};
inline constexpr std::array<ItemAmount,5> costs{{
  {16130,2},
  {16769,1},
  {16775,1},
  {15682,1},
  {16604,1},
}};
inline constexpr std::array<AmountOperand,10> operands{{
  {1,1,1,0,false},
  {22,1,1,0,false},
  {43,1,1,0,false},
  {64,2,2,0,false},
  {103,2,2,0,false},
  {123,1,1,0,false},
  {143,1,1,0,false},
  {163,1,1,0,false},
  {199,1,1,0,false},
  {224,1,1,0,false},
}};
inline constexpr std::array<OperandUse,10> operand_uses{{
  {1,AmountRole::minimum_inventory,15682},
  {22,AmountRole::minimum_inventory,16775},
  {43,AmountRole::minimum_inventory,16769},
  {64,AmountRole::minimum_inventory,16130},
  {103,AmountRole::remove_count,16130},
  {123,AmountRole::remove_count,16769},
  {143,AmountRole::remove_count,16775},
  {163,AmountRole::remove_count,15682},
  {199,AmountRole::create_count,15741},
  {224,AmountRole::create_count,15739},
}};
inline constexpr std::array<Effect,8> enriched_effects{{
  {EffectKind::remove,184,107,16130,2,Actor::hero},
  {EffectKind::remove,184,127,16769,1,Actor::hero},
  {EffectKind::remove,184,147,16775,1,Actor::hero},
  {EffectKind::remove,184,167,15682,1,Actor::hero},
  {EffectKind::create,170,203,15741,1,Actor::hero},
  {EffectKind::print,12,238,-1,0,Actor::hero},
  {EffectKind::say,14646,258,-1,0,Actor::hero},
  {EffectKind::end_production,19965,318,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,8> ordinary_effects{{
  {EffectKind::remove,184,107,16130,2,Actor::hero},
  {EffectKind::remove,184,127,16769,1,Actor::hero},
  {EffectKind::remove,184,147,16775,1,Actor::hero},
  {EffectKind::remove,184,167,15682,1,Actor::hero},
  {EffectKind::create,170,228,15739,1,Actor::hero},
  {EffectKind::print,12,238,-1,0,Actor::hero},
  {EffectKind::say,14646,258,-1,0,Actor::hero},
  {EffectKind::end_production,19965,318,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> failure_effects{{
  {EffectKind::print,12,273,-1,0,Actor::hero},
  {EffectKind::say,14646,293,-1,0,Actor::hero},
  {EffectKind::create,170,313,16604,1,Actor::self},
  {EffectKind::end_production,19965,318,-1,0,Actor::hero},
}};
inline constexpr std::array<PrintFingerprint,2> prints{{
  {14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},
  {14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
}};
inline constexpr std::array<const Function*,7> item_constructors{{
  function_for(16130),
  function_for(16769),
  function_for(16775),
  function_for(15682),
  function_for(16604),
  function_for(15739),
  function_for(15741),
}};
inline constexpr Owner owner{20368,2,22243,false,false,true,function_for(20368),function_for(20369),function_for(20370),view(requirements)};
inline constexpr std::array<Descriptor,1> recipes{{
  {RecipeId::spirit,"Geisttrank",owner,function_for(20370),{324,function_for(20370)->body_sha256,operands.data(),operands.size()},
    view(targets),view(bindings),view(requirements),view(costs),maximum_quantity,11967,1,
    {{{Branch::ordinary,{15739,1},view(ordinary_effects)},{Branch::enriched,{15741,1},view(enriched_effects)}}},
    alchemy_recipe_catalog::lab_hand,alchemy_recipe_catalog::flask_debit,{recipe_catalog::MaterialAction::create,16604,1,Actor::self},
    view(failure_effects),view(operand_uses),view(prints),view(item_constructors),function_for(20276),function_for(20279),function_for(19965),function_for(14646),true}
}};
inline const Descriptor* find(RecipeId id) noexcept {return id==RecipeId::spirit?&recipes[0]:nullptr;}
inline const SymbolBinding* binding_for(const Descriptor& d,std::uint32_t symbol) noexcept {
  if(!d.bindings.data||d.bindings.count>64)return nullptr;
  for(const auto& b:d.bindings)if(b.symbol==symbol)return &b;
  for(const auto& b:alchemy_recipe_catalog::common_bindings)if(b.symbol==symbol)return &b;return nullptr;
}
inline Branch branch_for(bool known,std::int32_t value) noexcept {return !known?Branch::unknown:value==1?Branch::enriched:Branch::ordinary;}
inline const Variant* variant(const Descriptor& d,Branch b) noexcept {for(const auto& v:d.variants)if(v.branch==b)return &v;return nullptr;}
namespace detail {
inline bool text(const char* a,const char* b) noexcept {if(!a||!b)return false;for(unsigned i=0;i<64;++i){if(a[i]!=b[i])return false;if(!a[i])return true;}return false;}
inline bool function(const Function* a,const Function* b) noexcept {return recipe_navigation::detail::same_function(a,b);}
inline bool requirement(const Requirement& a,const Requirement& b) noexcept {return a.kind==b.kind&&a.role==b.role&&a.symbol==b.symbol&&a.element==b.element&&a.value==b.value;}
inline bool binding(const SymbolBinding& a,const SymbolBinding& b) noexcept {return text(a.name,b.name)&&a.symbol==b.symbol&&a.type==b.type&&a.flags==b.flags&&a.elements==b.elements&&a.parent_index==b.parent_index&&a.check_value==b.check_value&&a.value==b.value&&a.check_offset==b.check_offset&&a.offset==b.offset;}
inline bool effect(const Effect& a,const Effect& b) noexcept {return a.kind==b.kind&&a.callee==b.callee&&a.offset==b.offset&&a.item==b.item&&a.units==b.units&&a.actor==b.actor;}
inline bool item(const ItemAmount& a,const ItemAmount& b) noexcept {return a.symbol==b.symbol&&a.units==b.units;}
template<class T,class Equal>bool same(View<T> a,View<T> b,Equal equal) noexcept {if(a.count!=b.count||(a.count&&!a.data))return false;for(std::size_t i=0;i<a.count;++i)if(!equal(a[i],b[i]))return false;return true;}
inline bool same_owner(const Owner& a,const Owner& b) noexcept {return a.instance==b.instance&&a.number==b.number&&a.npc==b.npc&&a.important==b.important&&a.trade==b.trade&&a.permanent==b.permanent&&function(a.constructor,b.constructor)&&function(a.condition,b.condition)&&function(a.information,b.information)&&same(a.conditions,b.conditions,requirement);}
}
// Only own valid C++ memory regions; foreign pointers belong in Reader.
inline bool structurally_valid(const Descriptor& d) noexcept {
  const auto* expected=find(d.id);if(!expected)return false;const auto& e=*expected;namespace x=detail;
  if(!x::text(d.name,e.name)||!x::same_owner(d.owner,e.owner)||!x::function(d.callback,e.callback)||
     d.maximum_quantity!=e.maximum_quantity||d.branch_symbol!=e.branch_symbol||d.branch_equal!=e.branch_equal||
     d.require_self_is_hero!=e.require_self_is_hero||d.amount.length!=e.amount.length||d.amount.digest!=e.amount.digest||
     d.amount.count!=e.amount.count||!d.amount.operands)return false;
  for(std::size_t i=0;i<e.amount.count;++i){const auto& a=d.amount.operands[i];const auto& b=e.amount.operands[i];
    if(a.offset!=b.offset||a.original!=b.original||a.units!=b.units||a.reserved!=b.reserved||a.allow_zero!=b.allow_zero)return false;}
  if(!x::same(d.targets,e.targets,[](const auto& a,const auto& b){return a.symbol==b.symbol&&detail::text(a.name,b.name);})||
     !x::same(d.bindings,e.bindings,x::binding)||!x::same(d.requirements,e.requirements,x::requirement)||!x::same(d.costs,e.costs,x::item)||
     !x::same(d.failure,e.failure,x::effect)||!x::same(d.operand_uses,e.operand_uses,[](const auto& a,const auto& b){return a.offset==b.offset&&a.role==b.role&&a.symbol==b.symbol;})||
     !x::same(d.prints,e.prints,[](const auto& a,const auto& b){return a.symbol==b.symbol&&a.length==b.length&&a.digest==b.digest;})||
     !x::same(d.item_constructors,e.item_constructors,x::function))return false;
  for(std::size_t i=0;i<e.variants.size();++i)if(d.variants[i].branch!=e.variants[i].branch||!x::item(d.variants[i].output,e.variants[i].output)||
    !x::same(d.variants[i].success,e.variants[i].success,x::effect))return false;
  const auto& h=d.hand;const auto& k=e.hand;const auto& f=d.extra_flasks;const auto& g=e.extra_flasks;
  return h.start_item_symbol==k.start_item_symbol&&h.active_item_symbol==k.active_item_symbol&&h.amount==k.amount&&
    h.from_inventory==k.from_inventory&&h.consumed_on_success==k.consumed_on_success&&h.failure_refund==k.failure_refund&&h.evidence==k.evidence&&h.left_must_be_empty==k.left_must_be_empty&&
    f.symbol==g.symbol&&f.units==g.units&&f.reserved_hand==g.reserved_hand&&f.failure_refund==g.failure_refund&&f.after_verified_original==g.after_verified_original&&f.original_removes_stored_flasks==g.original_removes_stored_flasks&&
    d.refund.action==e.refund.action&&d.refund.symbol==e.refund.symbol&&d.refund.amount==e.refund.amount&&d.refund.actor==e.refund.actor&&
    x::function(d.station_entry,e.station_entry)&&x::function(d.station_end,e.station_end)&&x::function(d.end_production,e.end_production)&&x::function(d.say,e.say);
}
struct QuantityPlan {bool valid=false,patch_original=false;std::uint32_t quantity=0;std::int32_t output=-1;const AmountBody* amount=nullptr;};
inline QuantityPlan plan_quantity(const Descriptor& d,std::uint32_t quantity,Branch branch) noexcept {
  if(!quantity||quantity>d.maximum_quantity||!structurally_valid(d))return {};const auto* v=variant(d,branch);
  return v?QuantityPlan{true,quantity>1,quantity,v->output.symbol,&d.amount}:QuantityPlan{};
}
} // namespace ubc::spirit_recipe_catalog
