// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/anvil_recipe_catalog.hpp"

// Our eight canonical ore profiles. Original symbols and DAT remain
// unchanged. No engine call, registration or crafting authorization.
namespace ubc::ore_recipe_catalog {
using recipe_catalog::View;using recipe_catalog::view;
using recipe_catalog::Owner;using recipe_catalog::Callback;
using recipe_catalog::Requirement;using recipe_catalog::RequirementKind;using recipe_catalog::RequirementRole;
using recipe_catalog::ItemAmount;using recipe_catalog::MaterialEffect;using recipe_catalog::MaterialAction;
using recipe_catalog::Actor;using recipe_catalog::Invocation;
using station_recipe_catalog::HandSource;using station_recipe_catalog::Completion;
using station_recipe_catalog::AmountRole;using station_recipe_catalog::OperandUse;
using station_recipe_catalog::RequirementCheck;using station_recipe_catalog::CheckSite;
using lhiver::Function;using lhiver::SymbolBinding;using lhiver::digest_literal;
enum class RecipeId : std::uint32_t {ore_1h_01=0x6001,ore_2h_01,ore_1h_02,ore_2h_02,ore_1h_03,ore_2h_03,ore_1h_04,ore_2h_04};
using anvil_recipe_catalog::Station;using anvil_recipe_catalog::HandSide;
using anvil_recipe_catalog::HandContract;using anvil_recipe_catalog::Page;
using anvil_recipe_catalog::Effect;using anvil_recipe_catalog::EffectKind;
using anvil_recipe_catalog::PrintFingerprint;using anvil_recipe_catalog::NativeRange;
using anvil_recipe_catalog::Navigation;using anvil_recipe_catalog::StationProfile;
struct Descriptor {
  RecipeId id;const char* name;Station station;std::int32_t production_context;
  View<PackedInventoryTarget> targets;View<SymbolBinding> bindings;
  View<Requirement> requirements;View<RequirementCheck> checks;
  Owner owner;Callback callback;
  View<ItemAmount> costs,outputs;HandContract hand;Completion completion;
  const StationProfile* station_profile;View<OperandUse> operand_uses;
  Actor minimum_actor,material_actor;bool require_self_is_hero;
  View<Effect> success,failure;View<PrintFingerprint> prints;
  View<const Function*> item_constructors;
  View<NativeRange> native_dependencies;
};

inline constexpr bool runtime_ready=false;
inline constexpr auto anvil_hand=anvil_recipe_catalog::anvil_hand;
inline constexpr auto& station=anvil_recipe_catalog::station;
inline constexpr auto& navigation=anvil_recipe_catalog::navigation;
inline constexpr auto& native_dependencies=anvil_recipe_catalog::native_dependencies;
inline constexpr auto& prints=anvil_recipe_catalog::prints;
inline constexpr auto common_bindings=anvil_recipe_catalog::common_bindings;
using anvil_recipe_catalog::with_common;
inline constexpr std::array<Function,34> functions{{
  {"ITAT_DRAGONBLOOD",16156,302816,166,digest_literal("8c59c2f7cd93013a05a96e18fb20c356b3c6eb9ccb8d203a43370d3d482a8ee3"),0,7,1,0,1584},
  {"ITMW_1H_SPECIAL_01",16492,351022,252,digest_literal("98410b523286be0e3628c4fb19d5d011379ed9c37b2b816c567ab10949131e4c"),0,7,1,0,1584},
  {"ITMW_2H_SPECIAL_01",16494,351526,254,digest_literal("f0621aff3cbe0cc71385d5ebd6ab7a487214a858b8aa899bf642cd1c2c249b54"),0,7,1,0,1584},
  {"ITMW_1H_SPECIAL_02",16508,354992,254,digest_literal("6026c6e8f0e9355b555cd38d6e69dacfad9c4be2cc9002f681e0a981a5ae3f6f"),0,7,1,0,1584},
  {"ITMW_2H_SPECIAL_02",16510,355500,254,digest_literal("ca19b62d344e550cb745162cd5d3369e0704b8a3a11b5db7085363f5d383f1f8"),0,7,1,0,1584},
  {"ITMW_1H_SPECIAL_03",16522,358468,254,digest_literal("fc0b8b4034a209119e4e45b89643cb866258634a9a4cd5c8d2facdd09550312c"),0,7,1,0,1584},
  {"ITMW_2H_SPECIAL_03",16524,358976,252,digest_literal("b41f8eec3dd88aed1036ed2af3a26e2c437f2aca66524909be55ca53a401eb9c"),0,7,1,0,1584},
  {"ITMW_1H_SPECIAL_04",16528,359968,254,digest_literal("6811333493e039e7d32dfd300f99f4f6c593dbd92017fd024c8e6e3ee2c40963"),0,7,1,0,1584},
  {"ITMW_2H_SPECIAL_04",16530,360476,254,digest_literal("0c39151d1336d3241754fbb89c3f6772e9bc30cc623a380078e6959d6b9d2517"),0,7,1,0,1584},
  {"ITMI_NUGGET",16626,366951,135,digest_literal("473eb11c87258aca2c2a054487cf4988a19a08309f0f7394ee0b4e2a9bce055f"),0,7,1,0,1584},
  {"PC_ITMW_1H_SPECIAL_01",20743,789583,45,digest_literal("2ba2e1204a5d9ef0b290bca4c474299faf05098232224456ac93eda60be3320a"),0,7,1,0,1649},
  {"PC_ITMW_1H_SPECIAL_01_CONDITION",20744,789628,48,digest_literal("a0e45f1afd68789da912b89f82a5bc00546561b18a9c64da0d54337a78612a31"),0,5,3,2,-1},
  {"PC_ITMW_1H_SPECIAL_01_INFO",20745,789676,190,digest_literal("b618186fc100b3b60d8b9b7787bb6f5317026eb497b49d85c7b7d44e3d242608"),0,5,1,0,-1},
  {"PC_ITMW_2H_SPECIAL_01",20746,789866,45,digest_literal("f66f20ac3a747ef8e49be49b48410226021a3f43b36d35286043d4ccf3455804"),0,7,1,0,1649},
  {"PC_ITMW_2H_SPECIAL_01_CONDITION",20747,789911,48,digest_literal("b23b1a549647ceeb61bec01ea056d6a147b7c5cee1a0c2c0c6949ebc82625291"),0,5,3,2,-1},
  {"PC_ITMW_2H_SPECIAL_01_INFO",20748,789959,190,digest_literal("38df7f29097374f7a1b2930812d70e4821c99d9eafcff5a7fcce447f81bf7890"),0,5,1,0,-1},
  {"PC_ITMW_1H_SPECIAL_02",20749,790149,45,digest_literal("af9f0972dfe979d9e9beb3eb720557bafad0bc6efd54c3c16ec7db30661b5bbd"),0,7,1,0,1649},
  {"PC_ITMW_1H_SPECIAL_02_CONDITION",20750,790194,48,digest_literal("d73b9f0183458dee40db2fa76bba3576b4d15a7cd02c9d95211baf0717d2db49"),0,5,3,2,-1},
  {"PC_ITMW_1H_SPECIAL_02_INFO",20751,790242,190,digest_literal("66fd47556b5a549be2e8a4a097d1ea50ff29f9d8f0d44f402ccf0b677a042fac"),0,5,1,0,-1},
  {"PC_ITMW_2H_SPECIAL_02",20752,790432,45,digest_literal("30bc8f7d1c396f68b70f16270dfffd48ffaa0d1a9f538e82c82e4ea37a1dc920"),0,7,1,0,1649},
  {"PC_ITMW_2H_SPECIAL_02_CONDITION",20753,790477,48,digest_literal("c574d27d99c32553f57ed1777628e69dc051453e986f9b34b07d8a09a8b1c5f6"),0,5,3,2,-1},
  {"PC_ITMW_2H_SPECIAL_02_INFO",20754,790525,190,digest_literal("13ecd5cb73c40395311122453cc66c41ddb0a1be33e66143c3d96fbe0126fd7e"),0,5,1,0,-1},
  {"PC_ITMW_1H_SPECIAL_03",20755,790715,45,digest_literal("cb34a527c57b5a9c0c981aca98c7609a4734c4fe9b73cc9712706bbadb737613"),0,7,1,0,1649},
  {"PC_ITMW_1H_SPECIAL_03_CONDITION",20756,790760,48,digest_literal("40d5a46a259cb33c6310bae776ce235daa29dc6e284c12dd97ba90ed8c0d8744"),0,5,3,2,-1},
  {"PC_ITMW_1H_SPECIAL_03_INFO",20757,790808,190,digest_literal("62c6470cefc6814b5768cb5827431461c65cab46d56c45493f38c0ea08af8d71"),0,5,1,0,-1},
  {"PC_ITMW_2H_SPECIAL_03",20758,790998,45,digest_literal("7fd7c7db8135b2ad121335b4928ce10740212091468b36cf06b0fa05a23be3c2"),0,7,1,0,1649},
  {"PC_ITMW_2H_SPECIAL_03_CONDITION",20759,791043,48,digest_literal("3e4a9c4e7035f9856861d81ca134f571f7635fe6d34739d3236cc4f468d8b871"),0,5,3,2,-1},
  {"PC_ITMW_2H_SPECIAL_03_INFO",20760,791091,190,digest_literal("d62cd5dbeb87f36b56f8c3b3b9c3d93457163b0580292746fa378d01fe059394"),0,5,1,0,-1},
  {"PC_ITMW_1H_SPECIAL_04",20761,791281,45,digest_literal("2edad6253b85c735ee851a92adc3f5dddb616d2084b3c90c502b8da563d33011"),0,7,1,0,1649},
  {"PC_ITMW_1H_SPECIAL_04_CONDITION",20762,791326,48,digest_literal("761ee4004f488c2c4b6fe9de135237336a0d637e6c98c2df6b133bffcf3a9f52"),0,5,3,2,-1},
  {"PC_ITMW_1H_SPECIAL_04_INFO",20763,791374,232,digest_literal("d7e50e8468a78ba9deabac7c60cc1f431cd9c3251666d56472e35cf6fbfc1bc8"),0,5,1,0,-1},
  {"PC_ITMW_2H_SPECIAL_04",20764,791606,45,digest_literal("74c0c9ae995732d804c78de5348ad9347de5e3445d0bb08ed86152d7b03d0321"),0,7,1,0,1649},
  {"PC_ITMW_2H_SPECIAL_04_CONDITION",20765,791651,48,digest_literal("72272fd8fd843673c37681ec81502191039eb928ba249ca9d85ea7ba4b9c0083"),0,5,3,2,-1},
  {"PC_ITMW_2H_SPECIAL_04_INFO",20766,791699,232,digest_literal("29f07262384be76de516be56f3415b3e5fcf39ba24816af8bf199cde4ae460b8"),0,5,1,0,-1},
}};
inline constexpr const Function* function_for(std::uint32_t symbol) noexcept {
  for(const auto& value:functions)if(value.symbol==symbol)return &value;
  for(const auto& value:anvil_recipe_catalog::functions)if(value.symbol==symbol)return &value;
  return nullptr;
}
inline constexpr std::array<PackedInventoryTarget,5> ore_0_targets{{
  {"ITMISWORDRAWHOT",16611},
  {"ITMI_NUGGET",16626},
  {"ITMW_1H_SPECIAL_01",16492},
  {"ITMI_PLIERS_SMLT",16666},
  {"ITMW_1H_MACE_L_04",16435},
}};
inline constexpr std::array<SymbolBinding,27> ore_0_own_bindings{{
  {"ITMISWORDRAWHOT",16611,7,1,0,1584,false,0,false,0},
  {"ITMI_NUGGET",16626,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_SPECIAL_01",16492,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT",16666,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_MACE_L_04",16435,7,1,0,1584,false,0,false,0},
  {"PC_ITMW_1H_SPECIAL_01",20743,7,1,0,1649,false,0,false,0},
  {"PC_SMITHWEAPON_END",20698,7,1,0,1649,false,0,false,0},
  {"PC_COMMON",20701,7,1,0,1649,false,0,false,0},
  {"PC_ORE",20704,7,1,0,1649,false,0,false,0},
  {"PC_COMMONBACK",20707,7,1,0,1649,false,0,false,0},
  {"PC_OREBACK",20710,7,1,0,1649,false,0,false,0},
  {"PC_ITMW_1H_COMMON",20713,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_01",20719,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_02",20725,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_03",20731,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_04",20737,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_SMITH",1496,2,0,30,-1,false,0,false,0},
  {"NORMALWAFFEN",10552,2,0,1,-1,false,0,false,0},
  {"ERZWAFFEN",10551,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
  {"MOBSI_SMITHWEAPON",12311,2,1,1,-1,true,1,false,0},
  {"PRINT_SMITHSUCCESS",14370,3,1,1,-1,false,0,false,0},
  {"PRINT_PRODITEMSMISSING",14374,3,1,1,-1,false,0,false,0},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"ITEM_KAT_NF",1032,2,1,1,-1,true,2,false,0},
  {"ITEM_SWD",1044,2,1,1,-1,true,16384,false,0},
}};
inline constexpr auto ore_0_bindings=with_common(ore_0_own_bindings);
inline constexpr std::array<Requirement,3> ore_0_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,1,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
}};
inline constexpr std::array<Requirement,6> ore_0_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,1,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,1},
}};
inline constexpr std::array<RequirementCheck,6> ore_0_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,1},CheckSite::owner_condition,function_for(20744)},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,1496,1,1},CheckSite::owner_condition,function_for(20744)},
  {{RequirementKind::integer_equal,RequirementRole::context,10551,0,1},CheckSite::owner_condition,function_for(20744)},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,function_for(20768)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},CheckSite::callback,function_for(20745)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,1},CheckSite::callback,function_for(20745)},
}};
inline constexpr Owner ore_0_owner={20743,-1,22243,false,false,true,function_for(20743),function_for(20744),function_for(20745),view(ore_0_conditions)};
inline constexpr std::array<ItemAmount,2> ore_0_costs{{
  {16611,1},
  {16626,1},
}};
inline constexpr std::array<ItemAmount,1> ore_0_outputs{{
  {16492,1},
}};
inline constexpr std::array<MaterialEffect,3> ore_0_effects{{
  {MaterialAction::remove,16611,1,0},
  {MaterialAction::remove,16626,1,0},
  {MaterialAction::create,16492,1,0},
}};
inline constexpr std::array<AmountOperand,5> ore_0_operands{{
  {1,1,1,0,false},
  {22,1,1,0,false},
  {59,1,1,0,false},
  {79,1,1,0,false},
  {99,1,1,0,false},
}};
inline constexpr std::array<OperandUse,5> ore_0_uses{{
  {1,AmountRole::minimum_inventory,16611},
  {22,AmountRole::minimum_inventory,16626},
  {59,AmountRole::remove_count,16611},
  {79,AmountRole::remove_count,16626},
  {99,AmountRole::create_count,16492},
}};
inline constexpr std::array<const Function*,5> ore_0_items{{
  function_for(16611),
  function_for(16626),
  function_for(16492),
  function_for(16666),
  function_for(16435),
}};
inline constexpr std::array<Effect,9> ore_0_success{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,1,Actor::hero},
  {EffectKind::remove,63,184,16611,1,Actor::hero},
  {EffectKind::remove,83,184,16626,1,Actor::hero},
  {EffectKind::create,103,170,16492,1,Actor::hero},
  {EffectKind::print,113,12,14370,0,Actor::hero},
  {EffectKind::say,133,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<Effect,6> ore_0_failure{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,1,Actor::hero},
  {EffectKind::print,148,12,14374,0,Actor::hero},
  {EffectKind::say,168,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget,5> ore_1_targets{{
  {"ITMISWORDRAWHOT",16611},
  {"ITMI_NUGGET",16626},
  {"ITMW_2H_SPECIAL_01",16494},
  {"ITMI_PLIERS_SMLT",16666},
  {"ITMW_1H_MACE_L_04",16435},
}};
inline constexpr std::array<SymbolBinding,27> ore_1_own_bindings{{
  {"ITMISWORDRAWHOT",16611,7,1,0,1584,false,0,false,0},
  {"ITMI_NUGGET",16626,7,1,0,1584,false,0,false,0},
  {"ITMW_2H_SPECIAL_01",16494,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT",16666,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_MACE_L_04",16435,7,1,0,1584,false,0,false,0},
  {"PC_ITMW_2H_SPECIAL_01",20746,7,1,0,1649,false,0,false,0},
  {"PC_SMITHWEAPON_END",20698,7,1,0,1649,false,0,false,0},
  {"PC_COMMON",20701,7,1,0,1649,false,0,false,0},
  {"PC_ORE",20704,7,1,0,1649,false,0,false,0},
  {"PC_COMMONBACK",20707,7,1,0,1649,false,0,false,0},
  {"PC_OREBACK",20710,7,1,0,1649,false,0,false,0},
  {"PC_ITMW_1H_COMMON",20713,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_01",20719,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_02",20725,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_03",20731,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_04",20737,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_SMITH",1496,2,0,30,-1,false,0,false,0},
  {"NORMALWAFFEN",10552,2,0,1,-1,false,0,false,0},
  {"ERZWAFFEN",10551,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
  {"MOBSI_SMITHWEAPON",12311,2,1,1,-1,true,1,false,0},
  {"PRINT_SMITHSUCCESS",14370,3,1,1,-1,false,0,false,0},
  {"PRINT_PRODITEMSMISSING",14374,3,1,1,-1,false,0,false,0},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"ITEM_KAT_NF",1032,2,1,1,-1,true,2,false,0},
  {"ITEM_2HD_SWD",1046,2,1,1,-1,true,65536,false,0},
}};
inline constexpr auto ore_1_bindings=with_common(ore_1_own_bindings);
inline constexpr std::array<Requirement,3> ore_1_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,2,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
}};
inline constexpr std::array<Requirement,6> ore_1_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,2,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,2},
}};
inline constexpr std::array<RequirementCheck,6> ore_1_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,1},CheckSite::owner_condition,function_for(20747)},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,1496,2,1},CheckSite::owner_condition,function_for(20747)},
  {{RequirementKind::integer_equal,RequirementRole::context,10551,0,1},CheckSite::owner_condition,function_for(20747)},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,function_for(20768)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},CheckSite::callback,function_for(20748)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,2},CheckSite::callback,function_for(20748)},
}};
inline constexpr Owner ore_1_owner={20746,-1,22243,false,false,true,function_for(20746),function_for(20747),function_for(20748),view(ore_1_conditions)};
inline constexpr std::array<ItemAmount,2> ore_1_costs{{
  {16611,1},
  {16626,2},
}};
inline constexpr std::array<ItemAmount,1> ore_1_outputs{{
  {16494,1},
}};
inline constexpr std::array<MaterialEffect,3> ore_1_effects{{
  {MaterialAction::remove,16611,1,0},
  {MaterialAction::remove,16626,2,0},
  {MaterialAction::create,16494,1,0},
}};
inline constexpr std::array<AmountOperand,5> ore_1_operands{{
  {1,1,1,0,false},
  {22,2,2,0,false},
  {59,1,1,0,false},
  {79,2,2,0,false},
  {99,1,1,0,false},
}};
inline constexpr std::array<OperandUse,5> ore_1_uses{{
  {1,AmountRole::minimum_inventory,16611},
  {22,AmountRole::minimum_inventory,16626},
  {59,AmountRole::remove_count,16611},
  {79,AmountRole::remove_count,16626},
  {99,AmountRole::create_count,16494},
}};
inline constexpr std::array<const Function*,5> ore_1_items{{
  function_for(16611),
  function_for(16626),
  function_for(16494),
  function_for(16666),
  function_for(16435),
}};
inline constexpr std::array<Effect,9> ore_1_success{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,2,Actor::hero},
  {EffectKind::remove,63,184,16611,1,Actor::hero},
  {EffectKind::remove,83,184,16626,2,Actor::hero},
  {EffectKind::create,103,170,16494,1,Actor::hero},
  {EffectKind::print,113,12,14370,0,Actor::hero},
  {EffectKind::say,133,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<Effect,6> ore_1_failure{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,2,Actor::hero},
  {EffectKind::print,148,12,14374,0,Actor::hero},
  {EffectKind::say,168,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget,5> ore_2_targets{{
  {"ITMISWORDRAWHOT",16611},
  {"ITMI_NUGGET",16626},
  {"ITMW_1H_SPECIAL_02",16508},
  {"ITMI_PLIERS_SMLT",16666},
  {"ITMW_1H_MACE_L_04",16435},
}};
inline constexpr std::array<SymbolBinding,27> ore_2_own_bindings{{
  {"ITMISWORDRAWHOT",16611,7,1,0,1584,false,0,false,0},
  {"ITMI_NUGGET",16626,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_SPECIAL_02",16508,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT",16666,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_MACE_L_04",16435,7,1,0,1584,false,0,false,0},
  {"PC_ITMW_1H_SPECIAL_02",20749,7,1,0,1649,false,0,false,0},
  {"PC_SMITHWEAPON_END",20698,7,1,0,1649,false,0,false,0},
  {"PC_COMMON",20701,7,1,0,1649,false,0,false,0},
  {"PC_ORE",20704,7,1,0,1649,false,0,false,0},
  {"PC_COMMONBACK",20707,7,1,0,1649,false,0,false,0},
  {"PC_OREBACK",20710,7,1,0,1649,false,0,false,0},
  {"PC_ITMW_1H_COMMON",20713,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_01",20719,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_02",20725,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_03",20731,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_04",20737,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_SMITH",1496,2,0,30,-1,false,0,false,0},
  {"NORMALWAFFEN",10552,2,0,1,-1,false,0,false,0},
  {"ERZWAFFEN",10551,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
  {"MOBSI_SMITHWEAPON",12311,2,1,1,-1,true,1,false,0},
  {"PRINT_SMITHSUCCESS",14370,3,1,1,-1,false,0,false,0},
  {"PRINT_PRODITEMSMISSING",14374,3,1,1,-1,false,0,false,0},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"ITEM_KAT_NF",1032,2,1,1,-1,true,2,false,0},
  {"ITEM_SWD",1044,2,1,1,-1,true,16384,false,0},
}};
inline constexpr auto ore_2_bindings=with_common(ore_2_own_bindings);
inline constexpr std::array<Requirement,3> ore_2_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,3,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
}};
inline constexpr std::array<Requirement,6> ore_2_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,3,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,2},
}};
inline constexpr std::array<RequirementCheck,6> ore_2_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,1},CheckSite::owner_condition,function_for(20750)},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,1496,3,1},CheckSite::owner_condition,function_for(20750)},
  {{RequirementKind::integer_equal,RequirementRole::context,10551,0,1},CheckSite::owner_condition,function_for(20750)},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,function_for(20768)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},CheckSite::callback,function_for(20751)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,2},CheckSite::callback,function_for(20751)},
}};
inline constexpr Owner ore_2_owner={20749,-1,22243,false,false,true,function_for(20749),function_for(20750),function_for(20751),view(ore_2_conditions)};
inline constexpr std::array<ItemAmount,2> ore_2_costs{{
  {16611,1},
  {16626,2},
}};
inline constexpr std::array<ItemAmount,1> ore_2_outputs{{
  {16508,1},
}};
inline constexpr std::array<MaterialEffect,3> ore_2_effects{{
  {MaterialAction::remove,16611,1,0},
  {MaterialAction::remove,16626,2,0},
  {MaterialAction::create,16508,1,0},
}};
inline constexpr std::array<AmountOperand,5> ore_2_operands{{
  {1,1,1,0,false},
  {22,2,2,0,false},
  {59,1,1,0,false},
  {79,2,2,0,false},
  {99,1,1,0,false},
}};
inline constexpr std::array<OperandUse,5> ore_2_uses{{
  {1,AmountRole::minimum_inventory,16611},
  {22,AmountRole::minimum_inventory,16626},
  {59,AmountRole::remove_count,16611},
  {79,AmountRole::remove_count,16626},
  {99,AmountRole::create_count,16508},
}};
inline constexpr std::array<const Function*,5> ore_2_items{{
  function_for(16611),
  function_for(16626),
  function_for(16508),
  function_for(16666),
  function_for(16435),
}};
inline constexpr std::array<Effect,9> ore_2_success{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,2,Actor::hero},
  {EffectKind::remove,63,184,16611,1,Actor::hero},
  {EffectKind::remove,83,184,16626,2,Actor::hero},
  {EffectKind::create,103,170,16508,1,Actor::hero},
  {EffectKind::print,113,12,14370,0,Actor::hero},
  {EffectKind::say,133,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<Effect,6> ore_2_failure{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,2,Actor::hero},
  {EffectKind::print,148,12,14374,0,Actor::hero},
  {EffectKind::say,168,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget,5> ore_3_targets{{
  {"ITMISWORDRAWHOT",16611},
  {"ITMI_NUGGET",16626},
  {"ITMW_2H_SPECIAL_02",16510},
  {"ITMI_PLIERS_SMLT",16666},
  {"ITMW_1H_MACE_L_04",16435},
}};
inline constexpr std::array<SymbolBinding,27> ore_3_own_bindings{{
  {"ITMISWORDRAWHOT",16611,7,1,0,1584,false,0,false,0},
  {"ITMI_NUGGET",16626,7,1,0,1584,false,0,false,0},
  {"ITMW_2H_SPECIAL_02",16510,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT",16666,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_MACE_L_04",16435,7,1,0,1584,false,0,false,0},
  {"PC_ITMW_2H_SPECIAL_02",20752,7,1,0,1649,false,0,false,0},
  {"PC_SMITHWEAPON_END",20698,7,1,0,1649,false,0,false,0},
  {"PC_COMMON",20701,7,1,0,1649,false,0,false,0},
  {"PC_ORE",20704,7,1,0,1649,false,0,false,0},
  {"PC_COMMONBACK",20707,7,1,0,1649,false,0,false,0},
  {"PC_OREBACK",20710,7,1,0,1649,false,0,false,0},
  {"PC_ITMW_1H_COMMON",20713,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_01",20719,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_02",20725,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_03",20731,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_04",20737,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_SMITH",1496,2,0,30,-1,false,0,false,0},
  {"NORMALWAFFEN",10552,2,0,1,-1,false,0,false,0},
  {"ERZWAFFEN",10551,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
  {"MOBSI_SMITHWEAPON",12311,2,1,1,-1,true,1,false,0},
  {"PRINT_SMITHSUCCESS",14370,3,1,1,-1,false,0,false,0},
  {"PRINT_PRODITEMSMISSING",14374,3,1,1,-1,false,0,false,0},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"ITEM_KAT_NF",1032,2,1,1,-1,true,2,false,0},
  {"ITEM_2HD_SWD",1046,2,1,1,-1,true,65536,false,0},
}};
inline constexpr auto ore_3_bindings=with_common(ore_3_own_bindings);
inline constexpr std::array<Requirement,3> ore_3_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,4,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
}};
inline constexpr std::array<Requirement,6> ore_3_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,4,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,3},
}};
inline constexpr std::array<RequirementCheck,6> ore_3_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,1},CheckSite::owner_condition,function_for(20753)},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,1496,4,1},CheckSite::owner_condition,function_for(20753)},
  {{RequirementKind::integer_equal,RequirementRole::context,10551,0,1},CheckSite::owner_condition,function_for(20753)},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,function_for(20768)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},CheckSite::callback,function_for(20754)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,3},CheckSite::callback,function_for(20754)},
}};
inline constexpr Owner ore_3_owner={20752,-1,22243,false,false,true,function_for(20752),function_for(20753),function_for(20754),view(ore_3_conditions)};
inline constexpr std::array<ItemAmount,2> ore_3_costs{{
  {16611,1},
  {16626,3},
}};
inline constexpr std::array<ItemAmount,1> ore_3_outputs{{
  {16510,1},
}};
inline constexpr std::array<MaterialEffect,3> ore_3_effects{{
  {MaterialAction::remove,16611,1,0},
  {MaterialAction::remove,16626,3,0},
  {MaterialAction::create,16510,1,0},
}};
inline constexpr std::array<AmountOperand,5> ore_3_operands{{
  {1,1,1,0,false},
  {22,3,3,0,false},
  {59,1,1,0,false},
  {79,3,3,0,false},
  {99,1,1,0,false},
}};
inline constexpr std::array<OperandUse,5> ore_3_uses{{
  {1,AmountRole::minimum_inventory,16611},
  {22,AmountRole::minimum_inventory,16626},
  {59,AmountRole::remove_count,16611},
  {79,AmountRole::remove_count,16626},
  {99,AmountRole::create_count,16510},
}};
inline constexpr std::array<const Function*,5> ore_3_items{{
  function_for(16611),
  function_for(16626),
  function_for(16510),
  function_for(16666),
  function_for(16435),
}};
inline constexpr std::array<Effect,9> ore_3_success{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,3,Actor::hero},
  {EffectKind::remove,63,184,16611,1,Actor::hero},
  {EffectKind::remove,83,184,16626,3,Actor::hero},
  {EffectKind::create,103,170,16510,1,Actor::hero},
  {EffectKind::print,113,12,14370,0,Actor::hero},
  {EffectKind::say,133,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<Effect,6> ore_3_failure{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,3,Actor::hero},
  {EffectKind::print,148,12,14374,0,Actor::hero},
  {EffectKind::say,168,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget,5> ore_4_targets{{
  {"ITMISWORDRAWHOT",16611},
  {"ITMI_NUGGET",16626},
  {"ITMW_1H_SPECIAL_03",16522},
  {"ITMI_PLIERS_SMLT",16666},
  {"ITMW_1H_MACE_L_04",16435},
}};
inline constexpr std::array<SymbolBinding,27> ore_4_own_bindings{{
  {"ITMISWORDRAWHOT",16611,7,1,0,1584,false,0,false,0},
  {"ITMI_NUGGET",16626,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_SPECIAL_03",16522,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT",16666,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_MACE_L_04",16435,7,1,0,1584,false,0,false,0},
  {"PC_ITMW_1H_SPECIAL_03",20755,7,1,0,1649,false,0,false,0},
  {"PC_SMITHWEAPON_END",20698,7,1,0,1649,false,0,false,0},
  {"PC_COMMON",20701,7,1,0,1649,false,0,false,0},
  {"PC_ORE",20704,7,1,0,1649,false,0,false,0},
  {"PC_COMMONBACK",20707,7,1,0,1649,false,0,false,0},
  {"PC_OREBACK",20710,7,1,0,1649,false,0,false,0},
  {"PC_ITMW_1H_COMMON",20713,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_01",20719,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_02",20725,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_03",20731,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_04",20737,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_SMITH",1496,2,0,30,-1,false,0,false,0},
  {"NORMALWAFFEN",10552,2,0,1,-1,false,0,false,0},
  {"ERZWAFFEN",10551,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
  {"MOBSI_SMITHWEAPON",12311,2,1,1,-1,true,1,false,0},
  {"PRINT_SMITHSUCCESS",14370,3,1,1,-1,false,0,false,0},
  {"PRINT_PRODITEMSMISSING",14374,3,1,1,-1,false,0,false,0},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"ITEM_KAT_NF",1032,2,1,1,-1,true,2,false,0},
  {"ITEM_SWD",1044,2,1,1,-1,true,16384,false,0},
}};
inline constexpr auto ore_4_bindings=with_common(ore_4_own_bindings);
inline constexpr std::array<Requirement,3> ore_4_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,5,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
}};
inline constexpr std::array<Requirement,6> ore_4_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,5,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,3},
}};
inline constexpr std::array<RequirementCheck,6> ore_4_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,1},CheckSite::owner_condition,function_for(20756)},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,1496,5,1},CheckSite::owner_condition,function_for(20756)},
  {{RequirementKind::integer_equal,RequirementRole::context,10551,0,1},CheckSite::owner_condition,function_for(20756)},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,function_for(20768)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},CheckSite::callback,function_for(20757)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,3},CheckSite::callback,function_for(20757)},
}};
inline constexpr Owner ore_4_owner={20755,-1,22243,false,false,true,function_for(20755),function_for(20756),function_for(20757),view(ore_4_conditions)};
inline constexpr std::array<ItemAmount,2> ore_4_costs{{
  {16611,1},
  {16626,3},
}};
inline constexpr std::array<ItemAmount,1> ore_4_outputs{{
  {16522,1},
}};
inline constexpr std::array<MaterialEffect,3> ore_4_effects{{
  {MaterialAction::remove,16611,1,0},
  {MaterialAction::remove,16626,3,0},
  {MaterialAction::create,16522,1,0},
}};
inline constexpr std::array<AmountOperand,5> ore_4_operands{{
  {1,1,1,0,false},
  {22,3,3,0,false},
  {59,1,1,0,false},
  {79,3,3,0,false},
  {99,1,1,0,false},
}};
inline constexpr std::array<OperandUse,5> ore_4_uses{{
  {1,AmountRole::minimum_inventory,16611},
  {22,AmountRole::minimum_inventory,16626},
  {59,AmountRole::remove_count,16611},
  {79,AmountRole::remove_count,16626},
  {99,AmountRole::create_count,16522},
}};
inline constexpr std::array<const Function*,5> ore_4_items{{
  function_for(16611),
  function_for(16626),
  function_for(16522),
  function_for(16666),
  function_for(16435),
}};
inline constexpr std::array<Effect,9> ore_4_success{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,3,Actor::hero},
  {EffectKind::remove,63,184,16611,1,Actor::hero},
  {EffectKind::remove,83,184,16626,3,Actor::hero},
  {EffectKind::create,103,170,16522,1,Actor::hero},
  {EffectKind::print,113,12,14370,0,Actor::hero},
  {EffectKind::say,133,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<Effect,6> ore_4_failure{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,3,Actor::hero},
  {EffectKind::print,148,12,14374,0,Actor::hero},
  {EffectKind::say,168,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget,5> ore_5_targets{{
  {"ITMISWORDRAWHOT",16611},
  {"ITMI_NUGGET",16626},
  {"ITMW_2H_SPECIAL_03",16524},
  {"ITMI_PLIERS_SMLT",16666},
  {"ITMW_1H_MACE_L_04",16435},
}};
inline constexpr std::array<SymbolBinding,27> ore_5_own_bindings{{
  {"ITMISWORDRAWHOT",16611,7,1,0,1584,false,0,false,0},
  {"ITMI_NUGGET",16626,7,1,0,1584,false,0,false,0},
  {"ITMW_2H_SPECIAL_03",16524,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT",16666,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_MACE_L_04",16435,7,1,0,1584,false,0,false,0},
  {"PC_ITMW_2H_SPECIAL_03",20758,7,1,0,1649,false,0,false,0},
  {"PC_SMITHWEAPON_END",20698,7,1,0,1649,false,0,false,0},
  {"PC_COMMON",20701,7,1,0,1649,false,0,false,0},
  {"PC_ORE",20704,7,1,0,1649,false,0,false,0},
  {"PC_COMMONBACK",20707,7,1,0,1649,false,0,false,0},
  {"PC_OREBACK",20710,7,1,0,1649,false,0,false,0},
  {"PC_ITMW_1H_COMMON",20713,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_01",20719,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_02",20725,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_03",20731,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_04",20737,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_SMITH",1496,2,0,30,-1,false,0,false,0},
  {"NORMALWAFFEN",10552,2,0,1,-1,false,0,false,0},
  {"ERZWAFFEN",10551,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
  {"MOBSI_SMITHWEAPON",12311,2,1,1,-1,true,1,false,0},
  {"PRINT_SMITHSUCCESS",14370,3,1,1,-1,false,0,false,0},
  {"PRINT_PRODITEMSMISSING",14374,3,1,1,-1,false,0,false,0},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"ITEM_KAT_NF",1032,2,1,1,-1,true,2,false,0},
  {"ITEM_2HD_SWD",1046,2,1,1,-1,true,65536,false,0},
}};
inline constexpr auto ore_5_bindings=with_common(ore_5_own_bindings);
inline constexpr std::array<Requirement,3> ore_5_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,6,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
}};
inline constexpr std::array<Requirement,6> ore_5_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,6,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,4},
}};
inline constexpr std::array<RequirementCheck,6> ore_5_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,1},CheckSite::owner_condition,function_for(20759)},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,1496,6,1},CheckSite::owner_condition,function_for(20759)},
  {{RequirementKind::integer_equal,RequirementRole::context,10551,0,1},CheckSite::owner_condition,function_for(20759)},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,function_for(20768)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},CheckSite::callback,function_for(20760)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,4},CheckSite::callback,function_for(20760)},
}};
inline constexpr Owner ore_5_owner={20758,-1,22243,false,false,true,function_for(20758),function_for(20759),function_for(20760),view(ore_5_conditions)};
inline constexpr std::array<ItemAmount,2> ore_5_costs{{
  {16611,1},
  {16626,4},
}};
inline constexpr std::array<ItemAmount,1> ore_5_outputs{{
  {16524,1},
}};
inline constexpr std::array<MaterialEffect,3> ore_5_effects{{
  {MaterialAction::remove,16611,1,0},
  {MaterialAction::remove,16626,4,0},
  {MaterialAction::create,16524,1,0},
}};
inline constexpr std::array<AmountOperand,5> ore_5_operands{{
  {1,1,1,0,false},
  {22,4,4,0,false},
  {59,1,1,0,false},
  {79,4,4,0,false},
  {99,1,1,0,false},
}};
inline constexpr std::array<OperandUse,5> ore_5_uses{{
  {1,AmountRole::minimum_inventory,16611},
  {22,AmountRole::minimum_inventory,16626},
  {59,AmountRole::remove_count,16611},
  {79,AmountRole::remove_count,16626},
  {99,AmountRole::create_count,16524},
}};
inline constexpr std::array<const Function*,5> ore_5_items{{
  function_for(16611),
  function_for(16626),
  function_for(16524),
  function_for(16666),
  function_for(16435),
}};
inline constexpr std::array<Effect,9> ore_5_success{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,4,Actor::hero},
  {EffectKind::remove,63,184,16611,1,Actor::hero},
  {EffectKind::remove,83,184,16626,4,Actor::hero},
  {EffectKind::create,103,170,16524,1,Actor::hero},
  {EffectKind::print,113,12,14370,0,Actor::hero},
  {EffectKind::say,133,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<Effect,6> ore_5_failure{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16626,4,Actor::hero},
  {EffectKind::print,148,12,14374,0,Actor::hero},
  {EffectKind::say,168,14646,-1,0,Actor::self},
  {EffectKind::end_production,173,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,188,0,10551,0,Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget,6> ore_6_targets{{
  {"ITMISWORDRAWHOT",16611},
  {"ITMI_NUGGET",16626},
  {"ITAT_DRAGONBLOOD",16156},
  {"ITMW_1H_SPECIAL_04",16528},
  {"ITMI_PLIERS_SMLT",16666},
  {"ITMW_1H_MACE_L_04",16435},
}};
inline constexpr std::array<SymbolBinding,28> ore_6_own_bindings{{
  {"ITMISWORDRAWHOT",16611,7,1,0,1584,false,0,false,0},
  {"ITMI_NUGGET",16626,7,1,0,1584,false,0,false,0},
  {"ITAT_DRAGONBLOOD",16156,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_SPECIAL_04",16528,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT",16666,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_MACE_L_04",16435,7,1,0,1584,false,0,false,0},
  {"PC_ITMW_1H_SPECIAL_04",20761,7,1,0,1649,false,0,false,0},
  {"PC_SMITHWEAPON_END",20698,7,1,0,1649,false,0,false,0},
  {"PC_COMMON",20701,7,1,0,1649,false,0,false,0},
  {"PC_ORE",20704,7,1,0,1649,false,0,false,0},
  {"PC_COMMONBACK",20707,7,1,0,1649,false,0,false,0},
  {"PC_OREBACK",20710,7,1,0,1649,false,0,false,0},
  {"PC_ITMW_1H_COMMON",20713,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_01",20719,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_02",20725,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_03",20731,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_04",20737,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_SMITH",1496,2,0,30,-1,false,0,false,0},
  {"NORMALWAFFEN",10552,2,0,1,-1,false,0,false,0},
  {"ERZWAFFEN",10551,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
  {"MOBSI_SMITHWEAPON",12311,2,1,1,-1,true,1,false,0},
  {"PRINT_SMITHSUCCESS",14370,3,1,1,-1,false,0,false,0},
  {"PRINT_PRODITEMSMISSING",14374,3,1,1,-1,false,0,false,0},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"ITEM_KAT_NF",1032,2,1,1,-1,true,2,false,0},
  {"ITEM_SWD",1044,2,1,1,-1,true,16384,false,0},
}};
inline constexpr auto ore_6_bindings=with_common(ore_6_own_bindings);
inline constexpr std::array<Requirement,3> ore_6_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,7,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
}};
inline constexpr std::array<Requirement,7> ore_6_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,7,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,4},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16156,0,5},
}};
inline constexpr std::array<RequirementCheck,7> ore_6_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,1},CheckSite::owner_condition,function_for(20762)},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,1496,7,1},CheckSite::owner_condition,function_for(20762)},
  {{RequirementKind::integer_equal,RequirementRole::context,10551,0,1},CheckSite::owner_condition,function_for(20762)},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,function_for(20768)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},CheckSite::callback,function_for(20763)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,4},CheckSite::callback,function_for(20763)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16156,0,5},CheckSite::callback,function_for(20763)},
}};
inline constexpr Owner ore_6_owner={20761,-1,22243,false,false,true,function_for(20761),function_for(20762),function_for(20763),view(ore_6_conditions)};
inline constexpr std::array<ItemAmount,3> ore_6_costs{{
  {16611,1},
  {16626,4},
  {16156,5},
}};
inline constexpr std::array<ItemAmount,1> ore_6_outputs{{
  {16528,1},
}};
inline constexpr std::array<MaterialEffect,4> ore_6_effects{{
  {MaterialAction::remove,16611,1,0},
  {MaterialAction::remove,16626,4,0},
  {MaterialAction::remove,16156,5,0},
  {MaterialAction::create,16528,1,0},
}};
inline constexpr std::array<AmountOperand,7> ore_6_operands{{
  {1,1,1,0,false},
  {22,5,5,0,false},
  {43,4,4,0,false},
  {81,1,1,0,false},
  {101,4,4,0,false},
  {121,5,5,0,false},
  {141,1,1,0,false},
}};
inline constexpr std::array<OperandUse,7> ore_6_uses{{
  {1,AmountRole::minimum_inventory,16611},
  {22,AmountRole::minimum_inventory,16156},
  {43,AmountRole::minimum_inventory,16626},
  {81,AmountRole::remove_count,16611},
  {101,AmountRole::remove_count,16626},
  {121,AmountRole::remove_count,16156},
  {141,AmountRole::create_count,16528},
}};
inline constexpr std::array<const Function*,6> ore_6_items{{
  function_for(16611),
  function_for(16626),
  function_for(16156),
  function_for(16528),
  function_for(16666),
  function_for(16435),
}};
inline constexpr std::array<Effect,11> ore_6_success{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16156,5,Actor::hero},
  {EffectKind::minimum,57,160,16626,4,Actor::hero},
  {EffectKind::remove,85,184,16611,1,Actor::hero},
  {EffectKind::remove,105,184,16626,4,Actor::hero},
  {EffectKind::remove,125,184,16156,5,Actor::hero},
  {EffectKind::create,145,170,16528,1,Actor::hero},
  {EffectKind::print,155,12,14370,0,Actor::hero},
  {EffectKind::say,175,14646,-1,0,Actor::self},
  {EffectKind::end_production,215,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,230,0,10551,0,Actor::hero},
}};
inline constexpr std::array<Effect,7> ore_6_failure{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16156,5,Actor::hero},
  {EffectKind::minimum,57,160,16626,4,Actor::hero},
  {EffectKind::print,190,12,14374,0,Actor::hero},
  {EffectKind::say,210,14646,-1,0,Actor::self},
  {EffectKind::end_production,215,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,230,0,10551,0,Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget,6> ore_7_targets{{
  {"ITMISWORDRAWHOT",16611},
  {"ITMI_NUGGET",16626},
  {"ITAT_DRAGONBLOOD",16156},
  {"ITMW_2H_SPECIAL_04",16530},
  {"ITMI_PLIERS_SMLT",16666},
  {"ITMW_1H_MACE_L_04",16435},
}};
inline constexpr std::array<SymbolBinding,28> ore_7_own_bindings{{
  {"ITMISWORDRAWHOT",16611,7,1,0,1584,false,0,false,0},
  {"ITMI_NUGGET",16626,7,1,0,1584,false,0,false,0},
  {"ITAT_DRAGONBLOOD",16156,7,1,0,1584,false,0,false,0},
  {"ITMW_2H_SPECIAL_04",16530,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT",16666,7,1,0,1584,false,0,false,0},
  {"ITMW_1H_MACE_L_04",16435,7,1,0,1584,false,0,false,0},
  {"PC_ITMW_2H_SPECIAL_04",20764,7,1,0,1649,false,0,false,0},
  {"PC_SMITHWEAPON_END",20698,7,1,0,1649,false,0,false,0},
  {"PC_COMMON",20701,7,1,0,1649,false,0,false,0},
  {"PC_ORE",20704,7,1,0,1649,false,0,false,0},
  {"PC_COMMONBACK",20707,7,1,0,1649,false,0,false,0},
  {"PC_OREBACK",20710,7,1,0,1649,false,0,false,0},
  {"PC_ITMW_1H_COMMON",20713,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_01",20719,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_02",20725,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_03",20731,7,1,0,1649,false,0,false,0},
  {"PC_WEAPON_1H_HARAD_04",20737,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_SMITH",1496,2,0,30,-1,false,0,false,0},
  {"NORMALWAFFEN",10552,2,0,1,-1,false,0,false,0},
  {"ERZWAFFEN",10551,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
  {"MOBSI_SMITHWEAPON",12311,2,1,1,-1,true,1,false,0},
  {"PRINT_SMITHSUCCESS",14370,3,1,1,-1,false,0,false,0},
  {"PRINT_PRODITEMSMISSING",14374,3,1,1,-1,false,0,false,0},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"ITEM_KAT_NF",1032,2,1,1,-1,true,2,false,0},
  {"ITEM_2HD_SWD",1046,2,1,1,-1,true,65536,false,0},
}};
inline constexpr auto ore_7_bindings=with_common(ore_7_own_bindings);
inline constexpr std::array<Requirement,3> ore_7_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,8,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
}};
inline constexpr std::array<Requirement,7> ore_7_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,1},
  {RequirementKind::integer_equal,RequirementRole::knowledge,1496,8,1},
  {RequirementKind::integer_equal,RequirementRole::context,10551,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,5},
  {RequirementKind::inventory_at_least,RequirementRole::resource,16156,0,5},
}};
inline constexpr std::array<RequirementCheck,7> ore_7_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,1},CheckSite::owner_condition,function_for(20765)},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,1496,8,1},CheckSite::owner_condition,function_for(20765)},
  {{RequirementKind::integer_equal,RequirementRole::context,10551,0,1},CheckSite::owner_condition,function_for(20765)},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,function_for(20768)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16611,0,1},CheckSite::callback,function_for(20766)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16626,0,5},CheckSite::callback,function_for(20766)},
  {{RequirementKind::inventory_at_least,RequirementRole::resource,16156,0,5},CheckSite::callback,function_for(20766)},
}};
inline constexpr Owner ore_7_owner={20764,-1,22243,false,false,true,function_for(20764),function_for(20765),function_for(20766),view(ore_7_conditions)};
inline constexpr std::array<ItemAmount,3> ore_7_costs{{
  {16611,1},
  {16626,5},
  {16156,5},
}};
inline constexpr std::array<ItemAmount,1> ore_7_outputs{{
  {16530,1},
}};
inline constexpr std::array<MaterialEffect,4> ore_7_effects{{
  {MaterialAction::remove,16611,1,0},
  {MaterialAction::remove,16626,5,0},
  {MaterialAction::remove,16156,5,0},
  {MaterialAction::create,16530,1,0},
}};
inline constexpr std::array<AmountOperand,7> ore_7_operands{{
  {1,1,1,0,false},
  {22,5,5,0,false},
  {43,5,5,0,false},
  {81,1,1,0,false},
  {101,5,5,0,false},
  {121,5,5,0,false},
  {141,1,1,0,false},
}};
inline constexpr std::array<OperandUse,7> ore_7_uses{{
  {1,AmountRole::minimum_inventory,16611},
  {22,AmountRole::minimum_inventory,16156},
  {43,AmountRole::minimum_inventory,16626},
  {81,AmountRole::remove_count,16611},
  {101,AmountRole::remove_count,16626},
  {121,AmountRole::remove_count,16156},
  {141,AmountRole::create_count,16530},
}};
inline constexpr std::array<const Function*,6> ore_7_items{{
  function_for(16611),
  function_for(16626),
  function_for(16156),
  function_for(16530),
  function_for(16666),
  function_for(16435),
}};
inline constexpr std::array<Effect,11> ore_7_success{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16156,5,Actor::hero},
  {EffectKind::minimum,57,160,16626,5,Actor::hero},
  {EffectKind::remove,85,184,16611,1,Actor::hero},
  {EffectKind::remove,105,184,16626,5,Actor::hero},
  {EffectKind::remove,125,184,16156,5,Actor::hero},
  {EffectKind::create,145,170,16530,1,Actor::hero},
  {EffectKind::print,155,12,14370,0,Actor::hero},
  {EffectKind::say,175,14646,-1,0,Actor::self},
  {EffectKind::end_production,215,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,230,0,10551,0,Actor::hero},
}};
inline constexpr std::array<Effect,7> ore_7_failure{{
  {EffectKind::minimum,15,160,16611,1,Actor::self},
  {EffectKind::minimum,36,160,16156,5,Actor::hero},
  {EffectKind::minimum,57,160,16626,5,Actor::hero},
  {EffectKind::print,190,12,14374,0,Actor::hero},
  {EffectKind::say,210,14646,-1,0,Actor::self},
  {EffectKind::end_production,215,19965,-1,0,Actor::hero},
  {EffectKind::integer_store,230,0,10551,0,Actor::hero},
}};
inline constexpr std::array<Descriptor,8> recipes{{
  {RecipeId::ore_1h_01,"Erz-Langschwert",Station::anvil,1,view(ore_0_targets),view(ore_0_bindings),view(ore_0_requirements),view(ore_0_checks),ore_0_owner,{function_for(20745),20743,Invocation::info,1,1,2147483647,{190,function_for(20745)->body_sha256,ore_0_operands.data(),ore_0_operands.size()},view(ore_0_effects),{},true,true,false,true},view(ore_0_costs),view(ore_0_outputs),anvil_hand,Completion::end_production,&station,view(ore_0_uses),Actor::self,Actor::hero,true,view(ore_0_success),view(ore_0_failure),view(prints),view(ore_0_items),view(native_dependencies)},
  {RecipeId::ore_2h_01,"Erz-Zweihaender",Station::anvil,1,view(ore_1_targets),view(ore_1_bindings),view(ore_1_requirements),view(ore_1_checks),ore_1_owner,{function_for(20748),20746,Invocation::info,1,1,1073741823,{190,function_for(20748)->body_sha256,ore_1_operands.data(),ore_1_operands.size()},view(ore_1_effects),{},true,true,false,true},view(ore_1_costs),view(ore_1_outputs),anvil_hand,Completion::end_production,&station,view(ore_1_uses),Actor::self,Actor::hero,true,view(ore_1_success),view(ore_1_failure),view(prints),view(ore_1_items),view(native_dependencies)},
  {RecipeId::ore_1h_02,"Erz-Bastardschwert",Station::anvil,1,view(ore_2_targets),view(ore_2_bindings),view(ore_2_requirements),view(ore_2_checks),ore_2_owner,{function_for(20751),20749,Invocation::info,1,1,1073741823,{190,function_for(20751)->body_sha256,ore_2_operands.data(),ore_2_operands.size()},view(ore_2_effects),{},true,true,false,true},view(ore_2_costs),view(ore_2_outputs),anvil_hand,Completion::end_production,&station,view(ore_2_uses),Actor::self,Actor::hero,true,view(ore_2_success),view(ore_2_failure),view(prints),view(ore_2_items),view(native_dependencies)},
  {RecipeId::ore_2h_02,"Schwerer Erz-Zweihaender",Station::anvil,1,view(ore_3_targets),view(ore_3_bindings),view(ore_3_requirements),view(ore_3_checks),ore_3_owner,{function_for(20754),20752,Invocation::info,1,1,715827882,{190,function_for(20754)->body_sha256,ore_3_operands.data(),ore_3_operands.size()},view(ore_3_effects),{},true,true,false,true},view(ore_3_costs),view(ore_3_outputs),anvil_hand,Completion::end_production,&station,view(ore_3_uses),Actor::self,Actor::hero,true,view(ore_3_success),view(ore_3_failure),view(prints),view(ore_3_items),view(native_dependencies)},
  {RecipeId::ore_1h_03,"Erz-Schlachtklinge",Station::anvil,1,view(ore_4_targets),view(ore_4_bindings),view(ore_4_requirements),view(ore_4_checks),ore_4_owner,{function_for(20757),20755,Invocation::info,1,1,715827882,{190,function_for(20757)->body_sha256,ore_4_operands.data(),ore_4_operands.size()},view(ore_4_effects),{},true,true,false,true},view(ore_4_costs),view(ore_4_outputs),anvil_hand,Completion::end_production,&station,view(ore_4_uses),Actor::self,Actor::hero,true,view(ore_4_success),view(ore_4_failure),view(prints),view(ore_4_items),view(native_dependencies)},
  {RecipeId::ore_2h_03,"Schwere Erz-Schlachtklinge",Station::anvil,1,view(ore_5_targets),view(ore_5_bindings),view(ore_5_requirements),view(ore_5_checks),ore_5_owner,{function_for(20760),20758,Invocation::info,1,1,536870911,{190,function_for(20760)->body_sha256,ore_5_operands.data(),ore_5_operands.size()},view(ore_5_effects),{},true,true,false,true},view(ore_5_costs),view(ore_5_outputs),anvil_hand,Completion::end_production,&station,view(ore_5_uses),Actor::self,Actor::hero,true,view(ore_5_success),view(ore_5_failure),view(prints),view(ore_5_items),view(native_dependencies)},
  {RecipeId::ore_1h_04,"Erz-Drachentoeter",Station::anvil,1,view(ore_6_targets),view(ore_6_bindings),view(ore_6_requirements),view(ore_6_checks),ore_6_owner,{function_for(20763),20761,Invocation::info,1,1,429496729,{232,function_for(20763)->body_sha256,ore_6_operands.data(),ore_6_operands.size()},view(ore_6_effects),{},true,true,false,true},view(ore_6_costs),view(ore_6_outputs),anvil_hand,Completion::end_production,&station,view(ore_6_uses),Actor::self,Actor::hero,true,view(ore_6_success),view(ore_6_failure),view(prints),view(ore_6_items),view(native_dependencies)},
  {RecipeId::ore_2h_04,"Grosser Erz-Drachentoeter",Station::anvil,1,view(ore_7_targets),view(ore_7_bindings),view(ore_7_requirements),view(ore_7_checks),ore_7_owner,{function_for(20766),20764,Invocation::info,1,1,429496729,{232,function_for(20766)->body_sha256,ore_7_operands.data(),ore_7_operands.size()},view(ore_7_effects),{},true,true,false,true},view(ore_7_costs),view(ore_7_outputs),anvil_hand,Completion::end_production,&station,view(ore_7_uses),Actor::self,Actor::hero,true,view(ore_7_success),view(ore_7_failure),view(prints),view(ore_7_items),view(native_dependencies)},
}};
inline constexpr std::size_t catalog_target_capacity=[]() constexpr {
  std::size_t n=0;for(const auto& r:recipes)if(r.targets.count>n)n=r.targets.count;return n;
}();
inline const Descriptor* find(RecipeId id) noexcept {for(const auto& r:recipes)if(r.id==id)return &r;return nullptr;}
inline const Descriptor* find_callback(std::uint32_t symbol) noexcept {
  for(const auto& r:recipes)if(r.callback.function->symbol==symbol)return &r;return nullptr;
}
inline const SymbolBinding* binding_for(const Descriptor& r,std::uint32_t symbol) noexcept {
  if(!r.bindings.data||r.bindings.count>64)return nullptr;
  for(const auto& b:r.bindings)if(b.symbol==symbol)return &b;return nullptr;
}
namespace detail = anvil_recipe_catalog::detail;

// Only own valid C++ memory regions, no foreign pointers. Each copy remains
// bound to exactly one canonical family; in particular no reinterpretation
// of an item/flag operand to a quantity field, no foreign effect, no
// invented learning and no release solely from the static hand data.
inline bool structurally_valid(const Descriptor& r) noexcept {
  namespace d=detail;const auto* c=find(r.id);if(!c)return false;
  if(!d::text(r.name,c->name)||r.station!=c->station||r.production_context!=c->production_context||
    r.completion!=c->completion||r.minimum_actor!=c->minimum_actor||r.material_actor!=c->material_actor||
    r.require_self_is_hero!=c->require_self_is_hero||!d::owner(r.owner,c->owner)||
    !d::callback(r.callback,c->callback)||!d::hand(r.hand,c->hand)||!d::station(r.station_profile,c->station_profile))return false;
  if(!d::same(r.targets,c->targets,[](const auto& x,const auto& y){return x.symbol==y.symbol&&d::text(x.name,y.name);})||
    !d::same(r.bindings,c->bindings,d::same_binding)||!d::same(r.requirements,c->requirements,d::same_requirement)||
    !d::same(r.checks,c->checks,[](const auto& x,const auto& y){return d::same_requirement(x.requirement,y.requirement)&&x.site==y.site&&d::same_function(x.function,y.function);})||
    !d::same(r.costs,c->costs,[](const auto& x,const auto& y){return x.symbol==y.symbol&&x.units==y.units;})||
    !d::same(r.outputs,c->outputs,[](const auto& x,const auto& y){return x.symbol==y.symbol&&x.units==y.units;})||
    !d::same(r.operand_uses,c->operand_uses,[](const auto& x,const auto& y){return x.offset==y.offset&&x.role==y.role&&x.symbol==y.symbol;})||
    !d::same(r.success,c->success,d::effect)||!d::same(r.failure,c->failure,d::effect)||
    !d::same(r.prints,c->prints,[](const auto& x,const auto& y){return x.symbol==y.symbol&&x.length==y.length&&x.sha256==y.sha256;})||
    !d::same(r.item_constructors,c->item_constructors,[](const auto* x,const auto* y){return d::same_function(x,y);})||
    !d::same(r.native_dependencies,c->native_dependencies,[](const auto& x,const auto& y){return d::text(x.name,y.name)&&x.address==y.address&&x.length==y.length&&x.sha256==y.sha256;}))return false;
  return true;
}
struct QuantityPlan {const Callback* callback=nullptr;std::uint32_t quantity=0;bool valid=false,patch_original=false;};
inline QuantityPlan plan_quantity(const Descriptor& r,std::uint32_t quantity) noexcept {
  if(!quantity||!structurally_valid(r)||quantity>r.callback.maximum_quantity)return {};
  return {&r.callback,quantity,true,quantity>1};
}
// All cost/minimum/output products must fit in int32. The material
// preview additionally binds the actual stock and output slots.
} // namespace ubc::ore_recipe_catalog

