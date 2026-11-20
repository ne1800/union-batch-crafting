// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/recipe_catalog.hpp"

// Our metadata for L'Hiver DE 2.0.4. No original scripts or bytes.
// DAT: 71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4.
// Separate follow-up catalog, with no native registration or runtime authorization.
namespace ubc::station_recipe_catalog {
using recipe_catalog::View;
using recipe_catalog::view;
using recipe_catalog::ItemAmount;
using recipe_catalog::MaterialEffect;
using recipe_catalog::MaterialAction;
using recipe_catalog::Requirement;
using recipe_catalog::RequirementKind;
using recipe_catalog::RequirementRole;
using recipe_catalog::Owner;
using recipe_catalog::Callback;
using recipe_catalog::Invocation;
using recipe_catalog::Actor;
using lhiver::Function;
using lhiver::SymbolBinding;
using lhiver::digest_literal;

enum class Station : std::uint32_t { leatherwork=12, saw=17, forge=10 };
enum class RecipeId : std::uint32_t {
  leather_sheep=0x1001, leather_wolf, leather_lurker, leather_boar,
  leather_warg, leather_shark, leather_shadowbeast, leather_troll, leather_black_troll,
  arrows=0x2001, fire_arrows, magic_arrows, bolts, magic_bolts,
  heat_raw_steel=0x3001, form_hot_blade
};
enum class HandSource { empty, reserved_inventory_tool, generated };
enum class HandSide { left, right };
enum class Completion { end_production, ready_same_station };
enum class CheckSite { owner_condition, owner_information, callback, mob_condition };
enum class AmountRole { minimum_inventory, remove_count, create_count };
struct HandItem {
  std::int32_t symbol=-1;
  std::uint32_t amount=0;
  HandSource source=HandSource::empty;
  bool require_stored_absent=false;
};
struct HandContract {
  HandItem left, right;
  HandSide anchor=HandSide::right;
  bool static_lifecycle_verified=false, observed_in_world=false;
};
struct RequirementCheck {
  Requirement requirement;
  CheckSite site;
  const Function* function;
};
struct OperandUse { std::uint32_t offset; AmountRole role; std::int32_t symbol; };
struct StationProfile {
  Station station;
  std::int32_t production_context;
  const char *on_state, *scheme, *use_with_item, *condition_name;
  const Function *entry, *mob_condition;
  Owner end;
};
struct Descriptor {
  RecipeId id;
  const char* name;
  Station station;
  std::int32_t production_context;
  View<PackedInventoryTarget> targets;
  View<SymbolBinding> bindings; // All common and recipe-related bindings.
  View<Requirement> requirements;
  View<RequirementCheck> checks; // Original check point, no invented C_INFO rule.
  Owner owner;
  const Function* back;
  Callback callback; // Exactly one selected linear single template for N.
  View<ItemAmount> costs, outputs;
  HandContract hand;
  Completion completion;
  const StationProfile* station_profile;
  View<OperandUse> operand_uses;
  const char* quantity_unit; // Ammo: loose, output 10*N or 30*N.
  Actor material_actor; // SELF only with positive current SELF==HERO context.
};
inline constexpr bool runtime_ready=false;

// Each active item in hand must appear exactly once and must alias neither
// a listed inventory object nor the other hand. Generated requires a current
// tmpLevel reading of 1/2. A reserved saw requires 0. INSERT does not reset
// tmpLevel: 0 is a prerequisite, not a guarantee about a pre-existing external
// state. Bind the slot, anchor and animation natively. Saw: stored stock
// stays constant at Ready. A normal exit returns 1. Forge: DoDoAniEvents sets
// CREATE jobs to HighPriority (0x742E79), then InsertInList places each at
// the front (0x7874D6). ProcessMessageList starts at index 0 (0x78709A), so
// the right CREATE runs before the left. The first CREATE sets InteractItem
// (0x754AFB/0x754B06). Currently, 16667 on the right must be positively
// identified as the anchor. The MSB chunk sequence alone cannot prove
// this. Normal DESTROY releases both temporary slots. The extra inventory
// debit uses the old anchor instance 16667 (0x754B76/0x754B92). Account
// for stored 16666 separately. No additional material or tool return.
inline constexpr HandContract leather_hand{{},{17342,1,HandSource::generated,true},HandSide::right,true,false};
inline constexpr HandContract saw_hand{{},{16609,1,HandSource::reserved_inventory_tool,false},HandSide::right,true,false};
inline constexpr HandContract forge_hand{{16666,1,HandSource::generated,false},{16667,1,HandSource::generated,true},HandSide::right,true,false};

inline constexpr std::array<Function,51> functions{{
  {"B_SAY",14646,218446,39,digest_literal("b8b93bf2920074b9958b398d5d642935220402bd20ff5044c51f9a416675d6ad"),3,5,1,0,-1},
  {"B_ENDPRODUCTIONDIALOG",19965,672156,49,digest_literal("e9fcd2f7db76bab4ed7c5ceb6e5a98c3ab3b8166a3614a4dda2513092da34e64"),0,5,1,0,-1},
  {"WOODSAW_S1",20083,691810,65,digest_literal("4c127621078925ef27934aef75f64d2547bb8364d3bac8f89ded955edad656a8"),0,5,1,0,-1},
  {"PC_MAKEARROWS_END",20084,691875,67,digest_literal("695b67ac9be0041d94f98f680755a80988a1fee8bb41c69f5388b8756c7e7514"),0,7,1,0,1649},
  {"PC_MAKEARROWS_END_CONDITION",20085,691942,23,digest_literal("1b894043170d92f52cfc60a786ae0f80040074f77f1a0486797d08e2dbb3fffe"),0,5,3,2,-1},
  {"PC_MAKEARROWS_END_INFO",20086,691965,6,digest_literal("88e225fb843204b57529955367a14efd2314c9788ac29aff28032b3a2a8cd982"),0,5,1,0,-1},
  {"PC_MAKEARROWS_ADDON_HOUR",20087,691971,67,digest_literal("c24bd62e31ba94ef69ef98317a07ece4d33cc4b4148e8ec169ffb13dd610fe27"),0,7,1,0,1649},
  {"PC_MAKEARROWS_ADDON_HOUR_CONDITION",20088,692038,23,digest_literal("810cb1f585ca7b36de734a47ff29420da6178a39b3ab0cb97206f8c229f8d883"),0,5,3,2,-1},
  {"PC_MAKEARROWS_ADDON_HOUR_INFO",20089,692061,222,digest_literal("46c6b0e09e34f58faf7ddf49ddf2d6f03bff538404ac1cedf9966df010ec8805"),0,5,1,0,-1},
  {"PC_MAKEARROWS_ADDON_HOUR_BACK_INFO",20090,692283,11,digest_literal("156d05d4ff4fa2e12248dea9b14a7a26517df3ab4b65b850e0278a54c6b57b32"),0,5,1,0,-1},
  {"PC_MAKEARROWS_ADDON_HOUR_30_INFO",20091,692294,236,digest_literal("3534c932091e25c90e0d59ac07f8850b03c072ef475475366df00e2166acbd40"),0,5,1,0,-1},
  {"PC_MAKEARROWS_ADDON_HOUR4_INFO",20094,693002,278,digest_literal("6ae46f4d058fc6a6228971729eeb35588d503c9b5e59bd0181d32d381447a4a2"),0,5,1,0,-1},
  {"PC_MAKEARROWS_ADDON_HOUR5_INFO",20096,693558,278,digest_literal("f6b9cf688ef342a70e993d84b6f44c74148b3862e222127d94dcaf9b0f142cbe"),0,5,1,0,-1},
  {"PC_MAKEARROWS_ADDON_HOUR2",20098,694114,67,digest_literal("7425f43e02806560abe230a436dac55a851e2c4463d1a8e92f5bdb96b09164aa"),0,7,1,0,1649},
  {"PC_MAKEARROWS_ADDON_HOUR2_CONDITION",20099,694181,23,digest_literal("e99c0568811378423a81f7f234dba0ad30d0774d08b284b8dabc125a0cca749b"),0,5,3,2,-1},
  {"PC_MAKEARROWS_ADDON_HOUR2_INFO",20100,694204,182,digest_literal("113ac2607dc10ebf1c976626c94f224ebd1415e17ae5f3a010ce86035b49c00e"),0,5,1,0,-1},
  {"PC_MAKEARROWS_ADDON_HOUR2_BACK_INFO",20101,694386,11,digest_literal("732de8e628c574a75d2c9bce0821beb43b878e50aa21d7d87370b5d250c18406"),0,5,1,0,-1},
  {"PC_MAKEARROWS_ADDON_HOUR2_30_INFO",20102,694397,236,digest_literal("b057451eb212a9354b65db25fa4443e345f2c43427a4a8facb299040cd8b32f8"),0,5,1,0,-1},
  {"PC_MAKEARROWS_ADDON_HOUR2_MBOLT10_INFO",20105,695105,278,digest_literal("108c3a237916af5ad6d4469d3a037523839aab0560bed9ca5db400fa23b6d652"),0,5,1,0,-1},
  {"LEATHERWORK_S1",20844,801124,65,digest_literal("adcd3c7cef52fc1c5da93212e9fa9564c23e158d027b792f0ad7d69423a9fcd5"),0,5,1,0,-1},
  {"PC_LEATHERWORK_END",20845,801189,67,digest_literal("0879da74f2ddbfd772e1777ea7faa20d151a42439846eec166a3fcfdc54afaee"),0,7,1,0,1649},
  {"PC_LEATHERWORK_END_CONDITION",20846,801256,23,digest_literal("2267ccf01356adc5981af773cf67b305dc091a805d12f6ef86e56b44d8cdc9d7"),0,5,3,2,-1},
  {"PC_LEATHERWORK_END_INFO",20847,801279,6,digest_literal("88e225fb843204b57529955367a14efd2314c9788ac29aff28032b3a2a8cd982"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON",20848,801285,67,digest_literal("e3788a654129803eb57db8598d823bf3a3d47382a6fc772b672274e9ce7decbc"),0,7,1,0,1649},
  {"PC_LEATHERWORK_ADDON_CONDITION",20849,801352,23,digest_literal("8b2ee38971447fdaa7013568c81a8e1ced746731ac94e631c4f9627f30729802"),0,5,3,2,-1},
  {"PC_LEATHERWORK_ADDON_INFO",20850,801375,211,digest_literal("1d40bc2ba0b21fa6c102684f930dcca86414a3373863e73bfa08488c2f2a7764"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON_BACK",20851,801586,11,digest_literal("919dd0288c92c9334457eeb54cd9c7455bd42dd050112639b01f90b6db996b32"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON_SHEEPFUR",20852,801597,219,digest_literal("d51d6ef6566107489a52e6f7a854fd20fa507dd874e5d6a0aa287b15d4b547ec"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON_WOLFFUR",20853,801816,219,digest_literal("810e1f075b050e1bc11e51aca768e0ce99606ab52cb12b46d125eeb82d55c727"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON_LURKERSKIN",20854,802035,219,digest_literal("68b283c8aa67020cc5faaa65c99278443dfd4593d31499fe263b666b0b42bcc1"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON_KEILERFUR",20855,802254,219,digest_literal("455b9d19eccad4e35bb04f57c0f243686914514b4f7e1210da69bba6783006d4"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON_WARGFUR",20856,802473,219,digest_literal("c7043654b219ffe1674c351b8de888d6549ff6f248eab394e9bff35bb945f894"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON_SHARKSKIN",20857,802692,219,digest_literal("eae4c8887c5a932db6fa93281ae566d7d710364003a41f1f5feeedc1289312ba"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON_SHADOWFUR",20858,802911,219,digest_literal("4f26107098eb7af6988b4968313247d247bb77361c85c7f3d31510c1afb0ab3e"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON_TROLLFUR",20859,803130,219,digest_literal("0cc6024a21a01b0e62336305f94aa5508a6ca8b28a7924b76d42dc8000f4fc6c"),0,5,1,0,-1},
  {"PC_LEATHERWORK_ADDON_TROLLBLACKFUR",20860,803349,219,digest_literal("76c2326582ce56fb274bffe088fa919413aa5cd2556589d0d289180840d4c418"),0,5,1,0,-1},
  {"B_USELWBENCH",20861,803568,23,digest_literal("1c6532be24ae3fa0b3a7ea4f19eea79295ccc904d23607a1cf02b0c768070cd9"),0,5,3,2,-1},
  {"ORESMELTING_S1",20987,823665,65,digest_literal("9e9716c064ab94dfffa2f009229b665f05e88bc770c4face1e46f590011570e8"),0,5,1,0,-1},
  {"PC_ORESMELTING_END",20988,823730,67,digest_literal("e429f83bfdd9e018d951331cdd4329bf251cb660c878eb69c74f96b917ab9a6e"),0,7,1,0,1649},
  {"PC_ORESMELTING_END_CONDITION",20989,823797,23,digest_literal("fc0128bc6e3cca1a7f566dcd5d4bee4a74111bd4f9c14d47a5e3833de871a623"),0,5,3,2,-1},
  {"PC_ORESMELTING_END_INFO",20990,823820,6,digest_literal("88e225fb843204b57529955367a14efd2314c9788ac29aff28032b3a2a8cd982"),0,5,1,0,-1},
  {"PC_ORESMELTING_ADDON",20991,823826,67,digest_literal("5e23e29250eb9bee928aea24cf042650d4f730baed44a066c639777a90fe24d1"),0,7,1,0,1649},
  {"PC_ORESMELTING_ADDON_CONDITION",20992,823893,23,digest_literal("32dff79a834a501e6af381838191dc92ca8e199f43d6042bdae20e8f37efc7c4"),0,5,3,2,-1},
  {"PC_ORESMELTING_ADDON_INFO",20993,823916,91,digest_literal("83e911cf42f9b8a02b60b3d8224bb0b460416f40cd652d0a2c2b39303ad0d873"),0,5,1,0,-1},
  {"PC_ORESMELTING_ADDON_BACK",20994,824007,11,digest_literal("8ca631fb331a6c9d67fcc8f03b2737c835b4a04a67b4aee3897f244d6e284192"),0,5,1,0,-1},
  {"PC_ORESMELTING_ADDON_ONE",20995,824018,137,digest_literal("c964bd1c4fcd3c025a5feb7ae6725e574b58401a7b24a47fcd4537ceecffb454"),0,5,1,0,-1},
  {"PC_ORESMELTING_ADDON_X20",20999,824465,67,digest_literal("2efbe9f22790d3437133b22a3d2aadabbe8a3c74122eb39936852a044b5e9dda"),0,7,1,0,1649},
  {"PC_ORESMELTING_ADDON_X20_CONDITION",21000,824532,23,digest_literal("bcf4caa9ac9060845caaa18875215285783be37e6957d5c2cf2f21e152ef5bbf"),0,5,3,2,-1},
  {"PC_ORESMELTING_ADDON_X20_INFO",21001,824555,91,digest_literal("5c83e62204994b8fab7b1eaa51f5426dac7431d894ab9ae832da30a1fd1fade9"),0,5,1,0,-1},
  {"PC_ORESMELTING_ADDON_X20_BACK",21002,824646,11,digest_literal("65eee5a4d77c7b1ed81eeb91330564ec6b11a741d6dab008db2332e93217daf3"),0,5,1,0,-1},
  {"PC_ORESMELTING_ADDON_X20_ONE",21003,824657,137,digest_literal("71a855c0666bc3a279bf1d54d0d337b981cd424df8b0bc4c006c467f21a7e0bf"),0,5,1,0,-1},
}};

inline constexpr std::array<SymbolBinding,21> common_bindings{{
  {"C_NPC",1537,4,0,32,-1,false,0,false,0},
  {"C_ITEM",1584,4,0,44,-1,false,0,false,0},
  {"C_NPC.AIVAR",1563,2,4,100,1537,false,0,true,648},
  {"SELF",1679,7,1,0,1537,false,0,false,0},
  {"OTHER",1680,7,1,0,1537,false,0,false,0},
  {"HERO",1683,7,1,0,1537,false,0,false,0},
  {"PC_HERO",22243,7,1,0,21074,false,0,false,0},
  {"AIV_INVINCIBLE",11998,2,1,1,-1,true,4,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"MOBSI_NONE",12310,2,1,1,-1,true,0,false,0},
  {"TRUE",12297,2,1,1,-1,true,1,false,0},
  {"FALSE",12298,2,1,1,-1,true,0,false,0},
  {"C_INFO",1649,4,0,8,-1,false,0,false,0},
  {"C_INFO.NPC",1650,2,4,1,1649,false,0,true,0},
  {"C_INFO.NR",1651,2,4,1,1649,false,0,true,4},
  {"C_INFO.IMPORTANT",1652,2,4,1,1649,false,0,true,8},
  {"C_INFO.CONDITION",1653,5,4,1,1649,false,0,false,0},
  {"C_INFO.INFORMATION",1654,5,4,1,1649,false,0,false,0},
  {"C_INFO.DESCRIPTION",1655,3,4,1,1649,false,0,false,0},
  {"C_INFO.TRADE",1656,2,4,1,1649,false,0,true,40},
  {"C_INFO.PERMANENT",1657,2,4,1,1649,false,0,true,44},
}};
template<std::size_t N>
constexpr auto with_common(const std::array<SymbolBinding,N>& own) noexcept {
  std::array<SymbolBinding,common_bindings.size()+N> result{};
  for(std::size_t i=0;i<common_bindings.size();++i) result[i]=common_bindings[i];
  for(std::size_t i=0;i<N;++i) result[common_bindings.size()+i]=own[i];
  return result;
}

inline constexpr std::array<Requirement,1> leatherwork_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,12},
}};
inline constexpr std::array<Requirement,1> saw_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,17},
}};
inline constexpr std::array<Requirement,1> forge_conditions{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,10},
}};
inline constexpr Owner owner_20848{20848,2,22243,false,false,true,&functions[23],&functions[24],&functions[25],view(leatherwork_conditions)};
inline constexpr Owner owner_20087{20087,2,22243,false,false,true,&functions[6],&functions[7],&functions[8],view(saw_conditions)};
inline constexpr Owner owner_20098{20098,2,22243,false,false,true,&functions[13],&functions[14],&functions[15],view(saw_conditions)};
inline constexpr Owner owner_20991{20991,1,22243,false,false,true,&functions[41],&functions[42],&functions[43],view(forge_conditions)};
inline constexpr Owner owner_20999{20999,4,22243,false,false,true,&functions[46],&functions[47],&functions[48],view(forge_conditions)};

inline constexpr std::array<StationProfile,3> stations{{
  {Station::leatherwork,12,"LEATHERWORK","LEATHERWORK","","B_USELWBENCH",&functions[19],&functions[36],{20845,999,22243,false,false,true,&functions[20],&functions[21],&functions[22],view(leatherwork_conditions)}},
  {Station::saw,17,"WOODSAW","BAUMSAEGE","ITMI_SAW","",&functions[2],nullptr,{20084,999,22243,false,false,true,&functions[3],&functions[4],&functions[5],view(saw_conditions)}},
  {Station::forge,10,"ORESMELTING","BSFIRE","","",&functions[37],nullptr,{20988,999,22243,false,false,true,&functions[38],&functions[39],&functions[40],view(forge_conditions)}},
}};

inline constexpr std::array<PackedInventoryTarget,3> leather_sheep_targets{{
  {"ITAT_SHEEPFUR",16121},
  {"ITMI_LEATHER",16599},
  {"ITMI_LWORK_01",17342},
}};
inline constexpr std::array<SymbolBinding,8> leather_sheep_own_bindings{{
  {"ITAT_SHEEPFUR",16121,7,1,0,1584,false,0,false,0},
  {"ITMI_LEATHER",16599,7,1,0,1584,false,0,false,0},
  {"ITMI_LWORK_01",17342,7,1,0,1584,false,0,false,0},
  {"MOBSI_LEATHERWORK",12322,2,1,1,-1,true,12,false,0},
  {"PC_LEATHERWORK_ADDON",20848,7,1,0,1649,false,0,false,0},
  {"PC_LEATHERWORK_END",20845,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_LEATHER",10483,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto leather_sheep_bindings=with_common(leather_sheep_own_bindings);
inline constexpr std::array<Requirement,3> leather_sheep_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,12},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
}};
inline constexpr std::array<RequirementCheck,3> leather_sheep_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,12},CheckSite::owner_condition,&functions[24]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},CheckSite::callback,&functions[27]},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,&functions[36]},
}};
inline constexpr std::array<ItemAmount,1> leather_sheep_costs{{
  {16121,1},
}};
inline constexpr std::array<ItemAmount,1> leather_sheep_outputs{{
  {16599,1},
}};
inline constexpr std::array<MaterialEffect,2> leather_sheep_effects{{
  {MaterialAction::remove,16121,1,0},
  {MaterialAction::create,16599,1,0},
}};
inline constexpr std::array<AmountOperand,3> leather_sheep_operands{{
  {17,1,1,0,false},
  {53,1,1,0,false},
  {73,1,1,0,false},
}};
inline constexpr std::array<OperandUse,3> leather_sheep_uses{{
  {17,AmountRole::minimum_inventory,16121},
  {53,AmountRole::remove_count,16121},
  {73,AmountRole::create_count,16599},
}};
inline constexpr Callback leather_sheep_callback{&functions[27],20848,Invocation::choice,1,1,INT32_MAX,{functions[27].byte_length,functions[27].body_sha256,leather_sheep_operands.data(),leather_sheep_operands.size()},view(leather_sheep_effects),{},true,true,false,false};

inline constexpr std::array<PackedInventoryTarget,3> leather_wolf_targets{{
  {"ITAT_WOLFFUR",16122},
  {"ITMI_LEATHER",16599},
  {"ITMI_LWORK_01",17342},
}};
inline constexpr std::array<SymbolBinding,8> leather_wolf_own_bindings{{
  {"ITAT_WOLFFUR",16122,7,1,0,1584,false,0,false,0},
  {"ITMI_LEATHER",16599,7,1,0,1584,false,0,false,0},
  {"ITMI_LWORK_01",17342,7,1,0,1584,false,0,false,0},
  {"MOBSI_LEATHERWORK",12322,2,1,1,-1,true,12,false,0},
  {"PC_LEATHERWORK_ADDON",20848,7,1,0,1649,false,0,false,0},
  {"PC_LEATHERWORK_END",20845,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_LEATHER",10483,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto leather_wolf_bindings=with_common(leather_wolf_own_bindings);
inline constexpr std::array<Requirement,3> leather_wolf_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,12},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
}};
inline constexpr std::array<RequirementCheck,3> leather_wolf_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,12},CheckSite::owner_condition,&functions[24]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},CheckSite::callback,&functions[28]},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,&functions[36]},
}};
inline constexpr std::array<ItemAmount,1> leather_wolf_costs{{
  {16122,1},
}};
inline constexpr std::array<ItemAmount,1> leather_wolf_outputs{{
  {16599,1},
}};
inline constexpr std::array<MaterialEffect,2> leather_wolf_effects{{
  {MaterialAction::remove,16122,1,0},
  {MaterialAction::create,16599,1,0},
}};
inline constexpr std::array<AmountOperand,3> leather_wolf_operands{{
  {17,1,1,0,false},
  {53,1,1,0,false},
  {73,1,1,0,false},
}};
inline constexpr std::array<OperandUse,3> leather_wolf_uses{{
  {17,AmountRole::minimum_inventory,16122},
  {53,AmountRole::remove_count,16122},
  {73,AmountRole::create_count,16599},
}};
inline constexpr Callback leather_wolf_callback{&functions[28],20848,Invocation::choice,1,1,INT32_MAX,{functions[28].byte_length,functions[28].body_sha256,leather_wolf_operands.data(),leather_wolf_operands.size()},view(leather_wolf_effects),{},true,true,false,false};

inline constexpr std::array<PackedInventoryTarget,3> leather_lurker_targets{{
  {"ITAT_LURKERSKIN",16132},
  {"ITMI_LEATHER",16599},
  {"ITMI_LWORK_01",17342},
}};
inline constexpr std::array<SymbolBinding,8> leather_lurker_own_bindings{{
  {"ITAT_LURKERSKIN",16132,7,1,0,1584,false,0,false,0},
  {"ITMI_LEATHER",16599,7,1,0,1584,false,0,false,0},
  {"ITMI_LWORK_01",17342,7,1,0,1584,false,0,false,0},
  {"MOBSI_LEATHERWORK",12322,2,1,1,-1,true,12,false,0},
  {"PC_LEATHERWORK_ADDON",20848,7,1,0,1649,false,0,false,0},
  {"PC_LEATHERWORK_END",20845,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_LEATHER",10483,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto leather_lurker_bindings=with_common(leather_lurker_own_bindings);
inline constexpr std::array<Requirement,3> leather_lurker_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,12},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
}};
inline constexpr std::array<RequirementCheck,3> leather_lurker_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,12},CheckSite::owner_condition,&functions[24]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},CheckSite::callback,&functions[29]},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,&functions[36]},
}};
inline constexpr std::array<ItemAmount,1> leather_lurker_costs{{
  {16132,1},
}};
inline constexpr std::array<ItemAmount,1> leather_lurker_outputs{{
  {16599,1},
}};
inline constexpr std::array<MaterialEffect,2> leather_lurker_effects{{
  {MaterialAction::remove,16132,1,0},
  {MaterialAction::create,16599,1,0},
}};
inline constexpr std::array<AmountOperand,3> leather_lurker_operands{{
  {17,1,1,0,false},
  {53,1,1,0,false},
  {73,1,1,0,false},
}};
inline constexpr std::array<OperandUse,3> leather_lurker_uses{{
  {17,AmountRole::minimum_inventory,16132},
  {53,AmountRole::remove_count,16132},
  {73,AmountRole::create_count,16599},
}};
inline constexpr Callback leather_lurker_callback{&functions[29],20848,Invocation::choice,1,1,INT32_MAX,{functions[29].byte_length,functions[29].body_sha256,leather_lurker_operands.data(),leather_lurker_operands.size()},view(leather_lurker_effects),{},true,true,false,false};

inline constexpr std::array<PackedInventoryTarget,3> leather_boar_targets{{
  {"ITAT_ADDON_KEILERFUR",16134},
  {"ITMI_LEATHER",16599},
  {"ITMI_LWORK_01",17342},
}};
inline constexpr std::array<SymbolBinding,8> leather_boar_own_bindings{{
  {"ITAT_ADDON_KEILERFUR",16134,7,1,0,1584,false,0,false,0},
  {"ITMI_LEATHER",16599,7,1,0,1584,false,0,false,0},
  {"ITMI_LWORK_01",17342,7,1,0,1584,false,0,false,0},
  {"MOBSI_LEATHERWORK",12322,2,1,1,-1,true,12,false,0},
  {"PC_LEATHERWORK_ADDON",20848,7,1,0,1649,false,0,false,0},
  {"PC_LEATHERWORK_END",20845,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_LEATHER",10483,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto leather_boar_bindings=with_common(leather_boar_own_bindings);
inline constexpr std::array<Requirement,3> leather_boar_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,12},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
}};
inline constexpr std::array<RequirementCheck,3> leather_boar_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,12},CheckSite::owner_condition,&functions[24]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},CheckSite::callback,&functions[30]},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,&functions[36]},
}};
inline constexpr std::array<ItemAmount,1> leather_boar_costs{{
  {16134,1},
}};
inline constexpr std::array<ItemAmount,1> leather_boar_outputs{{
  {16599,1},
}};
inline constexpr std::array<MaterialEffect,2> leather_boar_effects{{
  {MaterialAction::remove,16134,1,0},
  {MaterialAction::create,16599,1,0},
}};
inline constexpr std::array<AmountOperand,3> leather_boar_operands{{
  {17,1,1,0,false},
  {53,1,1,0,false},
  {73,1,1,0,false},
}};
inline constexpr std::array<OperandUse,3> leather_boar_uses{{
  {17,AmountRole::minimum_inventory,16134},
  {53,AmountRole::remove_count,16134},
  {73,AmountRole::create_count,16599},
}};
inline constexpr Callback leather_boar_callback{&functions[30],20848,Invocation::choice,1,1,INT32_MAX,{functions[30].byte_length,functions[30].body_sha256,leather_boar_operands.data(),leather_boar_operands.size()},view(leather_boar_effects),{},true,true,false,false};

inline constexpr std::array<PackedInventoryTarget,3> leather_warg_targets{{
  {"ITAT_WARGFUR",16133},
  {"ITMI_LEATHER",16599},
  {"ITMI_LWORK_01",17342},
}};
inline constexpr std::array<SymbolBinding,8> leather_warg_own_bindings{{
  {"ITAT_WARGFUR",16133,7,1,0,1584,false,0,false,0},
  {"ITMI_LEATHER",16599,7,1,0,1584,false,0,false,0},
  {"ITMI_LWORK_01",17342,7,1,0,1584,false,0,false,0},
  {"MOBSI_LEATHERWORK",12322,2,1,1,-1,true,12,false,0},
  {"PC_LEATHERWORK_ADDON",20848,7,1,0,1649,false,0,false,0},
  {"PC_LEATHERWORK_END",20845,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_LEATHER",10483,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto leather_warg_bindings=with_common(leather_warg_own_bindings);
inline constexpr std::array<Requirement,3> leather_warg_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,12},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
}};
inline constexpr std::array<RequirementCheck,3> leather_warg_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,12},CheckSite::owner_condition,&functions[24]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},CheckSite::callback,&functions[31]},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,&functions[36]},
}};
inline constexpr std::array<ItemAmount,1> leather_warg_costs{{
  {16133,1},
}};
inline constexpr std::array<ItemAmount,1> leather_warg_outputs{{
  {16599,1},
}};
inline constexpr std::array<MaterialEffect,2> leather_warg_effects{{
  {MaterialAction::remove,16133,1,0},
  {MaterialAction::create,16599,1,0},
}};
inline constexpr std::array<AmountOperand,3> leather_warg_operands{{
  {17,1,1,0,false},
  {53,1,1,0,false},
  {73,1,1,0,false},
}};
inline constexpr std::array<OperandUse,3> leather_warg_uses{{
  {17,AmountRole::minimum_inventory,16133},
  {53,AmountRole::remove_count,16133},
  {73,AmountRole::create_count,16599},
}};
inline constexpr Callback leather_warg_callback{&functions[31],20848,Invocation::choice,1,1,INT32_MAX,{functions[31].byte_length,functions[31].body_sha256,leather_warg_operands.data(),leather_warg_operands.size()},view(leather_warg_effects),{},true,true,false,false};

inline constexpr std::array<PackedInventoryTarget,3> leather_shark_targets{{
  {"ITAT_SHARKSKIN",16138},
  {"ITMI_LEATHER",16599},
  {"ITMI_LWORK_01",17342},
}};
inline constexpr std::array<SymbolBinding,8> leather_shark_own_bindings{{
  {"ITAT_SHARKSKIN",16138,7,1,0,1584,false,0,false,0},
  {"ITMI_LEATHER",16599,7,1,0,1584,false,0,false,0},
  {"ITMI_LWORK_01",17342,7,1,0,1584,false,0,false,0},
  {"MOBSI_LEATHERWORK",12322,2,1,1,-1,true,12,false,0},
  {"PC_LEATHERWORK_ADDON",20848,7,1,0,1649,false,0,false,0},
  {"PC_LEATHERWORK_END",20845,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_LEATHER",10483,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto leather_shark_bindings=with_common(leather_shark_own_bindings);
inline constexpr std::array<Requirement,3> leather_shark_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,12},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
}};
inline constexpr std::array<RequirementCheck,3> leather_shark_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,12},CheckSite::owner_condition,&functions[24]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},CheckSite::callback,&functions[32]},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,&functions[36]},
}};
inline constexpr std::array<ItemAmount,1> leather_shark_costs{{
  {16138,1},
}};
inline constexpr std::array<ItemAmount,1> leather_shark_outputs{{
  {16599,2},
}};
inline constexpr std::array<MaterialEffect,2> leather_shark_effects{{
  {MaterialAction::remove,16138,1,0},
  {MaterialAction::create,16599,2,0},
}};
inline constexpr std::array<AmountOperand,3> leather_shark_operands{{
  {17,1,1,0,false},
  {53,1,1,0,false},
  {73,2,2,0,false},
}};
inline constexpr std::array<OperandUse,3> leather_shark_uses{{
  {17,AmountRole::minimum_inventory,16138},
  {53,AmountRole::remove_count,16138},
  {73,AmountRole::create_count,16599},
}};
inline constexpr Callback leather_shark_callback{&functions[32],20848,Invocation::choice,1,1,INT32_MAX,{functions[32].byte_length,functions[32].body_sha256,leather_shark_operands.data(),leather_shark_operands.size()},view(leather_shark_effects),{},true,true,false,false};

inline constexpr std::array<PackedInventoryTarget,3> leather_shadowbeast_targets{{
  {"ITAT_SHADOWFUR",16137},
  {"ITMI_LEATHER",16599},
  {"ITMI_LWORK_01",17342},
}};
inline constexpr std::array<SymbolBinding,8> leather_shadowbeast_own_bindings{{
  {"ITAT_SHADOWFUR",16137,7,1,0,1584,false,0,false,0},
  {"ITMI_LEATHER",16599,7,1,0,1584,false,0,false,0},
  {"ITMI_LWORK_01",17342,7,1,0,1584,false,0,false,0},
  {"MOBSI_LEATHERWORK",12322,2,1,1,-1,true,12,false,0},
  {"PC_LEATHERWORK_ADDON",20848,7,1,0,1649,false,0,false,0},
  {"PC_LEATHERWORK_END",20845,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_LEATHER",10483,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto leather_shadowbeast_bindings=with_common(leather_shadowbeast_own_bindings);
inline constexpr std::array<Requirement,3> leather_shadowbeast_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,12},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
}};
inline constexpr std::array<RequirementCheck,3> leather_shadowbeast_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,12},CheckSite::owner_condition,&functions[24]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},CheckSite::callback,&functions[33]},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,&functions[36]},
}};
inline constexpr std::array<ItemAmount,1> leather_shadowbeast_costs{{
  {16137,1},
}};
inline constexpr std::array<ItemAmount,1> leather_shadowbeast_outputs{{
  {16599,3},
}};
inline constexpr std::array<MaterialEffect,2> leather_shadowbeast_effects{{
  {MaterialAction::remove,16137,1,0},
  {MaterialAction::create,16599,3,0},
}};
inline constexpr std::array<AmountOperand,3> leather_shadowbeast_operands{{
  {17,1,1,0,false},
  {53,1,1,0,false},
  {73,3,3,0,false},
}};
inline constexpr std::array<OperandUse,3> leather_shadowbeast_uses{{
  {17,AmountRole::minimum_inventory,16137},
  {53,AmountRole::remove_count,16137},
  {73,AmountRole::create_count,16599},
}};
inline constexpr Callback leather_shadowbeast_callback{&functions[33],20848,Invocation::choice,1,1,INT32_MAX,{functions[33].byte_length,functions[33].body_sha256,leather_shadowbeast_operands.data(),leather_shadowbeast_operands.size()},view(leather_shadowbeast_effects),{},true,true,false,false};

inline constexpr std::array<PackedInventoryTarget,3> leather_troll_targets{{
  {"ITAT_TROLLFUR",16139},
  {"ITMI_LEATHER",16599},
  {"ITMI_LWORK_01",17342},
}};
inline constexpr std::array<SymbolBinding,8> leather_troll_own_bindings{{
  {"ITAT_TROLLFUR",16139,7,1,0,1584,false,0,false,0},
  {"ITMI_LEATHER",16599,7,1,0,1584,false,0,false,0},
  {"ITMI_LWORK_01",17342,7,1,0,1584,false,0,false,0},
  {"MOBSI_LEATHERWORK",12322,2,1,1,-1,true,12,false,0},
  {"PC_LEATHERWORK_ADDON",20848,7,1,0,1649,false,0,false,0},
  {"PC_LEATHERWORK_END",20845,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_LEATHER",10483,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto leather_troll_bindings=with_common(leather_troll_own_bindings);
inline constexpr std::array<Requirement,3> leather_troll_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,12},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
}};
inline constexpr std::array<RequirementCheck,3> leather_troll_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,12},CheckSite::owner_condition,&functions[24]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},CheckSite::callback,&functions[34]},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,&functions[36]},
}};
inline constexpr std::array<ItemAmount,1> leather_troll_costs{{
  {16139,1},
}};
inline constexpr std::array<ItemAmount,1> leather_troll_outputs{{
  {16599,4},
}};
inline constexpr std::array<MaterialEffect,2> leather_troll_effects{{
  {MaterialAction::remove,16139,1,0},
  {MaterialAction::create,16599,4,0},
}};
inline constexpr std::array<AmountOperand,3> leather_troll_operands{{
  {17,1,1,0,false},
  {53,1,1,0,false},
  {73,4,4,0,false},
}};
inline constexpr std::array<OperandUse,3> leather_troll_uses{{
  {17,AmountRole::minimum_inventory,16139},
  {53,AmountRole::remove_count,16139},
  {73,AmountRole::create_count,16599},
}};
inline constexpr Callback leather_troll_callback{&functions[34],20848,Invocation::choice,1,1,INT32_MAX,{functions[34].byte_length,functions[34].body_sha256,leather_troll_operands.data(),leather_troll_operands.size()},view(leather_troll_effects),{},true,true,false,false};

inline constexpr std::array<PackedInventoryTarget,3> leather_black_troll_targets{{
  {"ITAT_TROLLBLACKFUR",16140},
  {"ITMI_LEATHER",16599},
  {"ITMI_LWORK_01",17342},
}};
inline constexpr std::array<SymbolBinding,8> leather_black_troll_own_bindings{{
  {"ITAT_TROLLBLACKFUR",16140,7,1,0,1584,false,0,false,0},
  {"ITMI_LEATHER",16599,7,1,0,1584,false,0,false,0},
  {"ITMI_LWORK_01",17342,7,1,0,1584,false,0,false,0},
  {"MOBSI_LEATHERWORK",12322,2,1,1,-1,true,12,false,0},
  {"PC_LEATHERWORK_ADDON",20848,7,1,0,1649,false,0,false,0},
  {"PC_LEATHERWORK_END",20845,7,1,0,1649,false,0,false,0},
  {"PLAYER_TALENT_LEATHER",10483,2,0,1,-1,false,0,false,0},
  {"STARTSMITHING",10536,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto leather_black_troll_bindings=with_common(leather_black_troll_own_bindings);
inline constexpr std::array<Requirement,3> leather_black_troll_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,12},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},
  {RequirementKind::integer_equal,RequirementRole::context,10536,0,1},
}};
inline constexpr std::array<RequirementCheck,3> leather_black_troll_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,12},CheckSite::owner_condition,&functions[24]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10483,0,1},CheckSite::callback,&functions[35]},
  {{RequirementKind::integer_equal,RequirementRole::context,10536,0,1},CheckSite::mob_condition,&functions[36]},
}};
inline constexpr std::array<ItemAmount,1> leather_black_troll_costs{{
  {16140,1},
}};
inline constexpr std::array<ItemAmount,1> leather_black_troll_outputs{{
  {16599,6},
}};
inline constexpr std::array<MaterialEffect,2> leather_black_troll_effects{{
  {MaterialAction::remove,16140,1,0},
  {MaterialAction::create,16599,6,0},
}};
inline constexpr std::array<AmountOperand,3> leather_black_troll_operands{{
  {17,1,1,0,false},
  {53,1,1,0,false},
  {73,6,6,0,false},
}};
inline constexpr std::array<OperandUse,3> leather_black_troll_uses{{
  {17,AmountRole::minimum_inventory,16140},
  {53,AmountRole::remove_count,16140},
  {73,AmountRole::create_count,16599},
}};
inline constexpr Callback leather_black_troll_callback{&functions[35],20848,Invocation::choice,1,1,INT32_MAX,{functions[35].byte_length,functions[35].body_sha256,leather_black_troll_operands.data(),leather_black_troll_operands.size()},view(leather_black_troll_effects),{},true,true,false,false};

inline constexpr std::array<PackedInventoryTarget,4> arrows_targets{{
  {"ITMI_WOODBAR_BIG",16595},
  {"ITMI_SAW",16609},
  {"ITMISWORDRAW",16610},
  {"ITRW_ARROW",16804},
}};
inline constexpr std::array<SymbolBinding,8> arrows_own_bindings{{
  {"ITMI_WOODBAR_BIG",16595,7,1,0,1584,false,0,false,0},
  {"ITMI_SAW",16609,7,1,0,1584,false,0,false,0},
  {"ITMISWORDRAW",16610,7,1,0,1584,false,0,false,0},
  {"ITRW_ARROW",16804,7,1,0,1584,false,0,false,0},
  {"MOBSI_WOODSAW",12327,2,1,1,-1,true,17,false,0},
  {"PC_MAKEARROWS_ADDON_HOUR",20087,7,1,0,1649,false,0,false,0},
  {"PC_MAKEARROWS_END",20084,7,1,0,1649,false,0,false,0},
  {"KNOWHOWTOMAKE_SPECIALARROWS",10491,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto arrows_bindings=with_common(arrows_own_bindings);
inline constexpr std::array<Requirement,2> arrows_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,17},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10491,0,1},
}};
inline constexpr std::array<RequirementCheck,2> arrows_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,17},CheckSite::owner_condition,&functions[7]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10491,0,1},CheckSite::owner_information,&functions[8]},
}};
inline constexpr std::array<ItemAmount,2> arrows_costs{{
  {16610,1},
  {16595,1},
}};
inline constexpr std::array<ItemAmount,1> arrows_outputs{{
  {16804,30},
}};
inline constexpr std::array<MaterialEffect,3> arrows_effects{{
  {MaterialAction::remove,16610,1,0},
  {MaterialAction::remove,16595,1,0},
  {MaterialAction::create,16804,30,0},
}};
inline constexpr std::array<AmountOperand,5> arrows_operands{{
  {1,1,1,0,false},
  {22,1,1,0,false},
  {59,1,1,0,false},
  {79,1,1,0,false},
  {99,30,30,0,false},
}};
inline constexpr std::array<OperandUse,5> arrows_uses{{
  {1,AmountRole::minimum_inventory,16595},
  {22,AmountRole::minimum_inventory,16610},
  {59,AmountRole::remove_count,16610},
  {79,AmountRole::remove_count,16595},
  {99,AmountRole::create_count,16804},
}};
inline constexpr Callback arrows_callback{&functions[10],20087,Invocation::choice,1,1,INT32_MAX,{functions[10].byte_length,functions[10].body_sha256,arrows_operands.data(),arrows_operands.size()},view(arrows_effects),{},true,false,true,false};

inline constexpr std::array<PackedInventoryTarget,5> fire_arrows_targets{{
  {"ITRW_ADDON_FIREARROW",15946},
  {"ITMI_WOODBAR_BIG",16595},
  {"ITMI_SAW",16609},
  {"ITMISWORDRAW",16610},
  {"ITMI_PITCH",16654},
}};
inline constexpr std::array<SymbolBinding,9> fire_arrows_own_bindings{{
  {"ITRW_ADDON_FIREARROW",15946,7,1,0,1584,false,0,false,0},
  {"ITMI_WOODBAR_BIG",16595,7,1,0,1584,false,0,false,0},
  {"ITMI_SAW",16609,7,1,0,1584,false,0,false,0},
  {"ITMISWORDRAW",16610,7,1,0,1584,false,0,false,0},
  {"ITMI_PITCH",16654,7,1,0,1584,false,0,false,0},
  {"MOBSI_WOODSAW",12327,2,1,1,-1,true,17,false,0},
  {"PC_MAKEARROWS_ADDON_HOUR",20087,7,1,0,1649,false,0,false,0},
  {"PC_MAKEARROWS_END",20084,7,1,0,1649,false,0,false,0},
  {"KNOWHOWTOMAKE_SPECIALARROWS",10491,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto fire_arrows_bindings=with_common(fire_arrows_own_bindings);
inline constexpr std::array<Requirement,2> fire_arrows_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,17},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10491,0,1},
}};
inline constexpr std::array<RequirementCheck,2> fire_arrows_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,17},CheckSite::owner_condition,&functions[7]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10491,0,1},CheckSite::owner_information,&functions[8]},
}};
inline constexpr std::array<ItemAmount,3> fire_arrows_costs{{
  {16610,1},
  {16654,1},
  {16595,1},
}};
inline constexpr std::array<ItemAmount,1> fire_arrows_outputs{{
  {15946,10},
}};
inline constexpr std::array<MaterialEffect,4> fire_arrows_effects{{
  {MaterialAction::remove,16610,1,0},
  {MaterialAction::remove,16654,1,0},
  {MaterialAction::remove,16595,1,0},
  {MaterialAction::create,15946,10,0},
}};
inline constexpr std::array<AmountOperand,7> fire_arrows_operands{{
  {1,1,1,0,false},
  {22,1,1,0,false},
  {43,1,1,0,false},
  {81,1,1,0,false},
  {101,1,1,0,false},
  {121,1,1,0,false},
  {141,10,10,0,false},
}};
inline constexpr std::array<OperandUse,7> fire_arrows_uses{{
  {1,AmountRole::minimum_inventory,16595},
  {22,AmountRole::minimum_inventory,16654},
  {43,AmountRole::minimum_inventory,16610},
  {81,AmountRole::remove_count,16610},
  {101,AmountRole::remove_count,16654},
  {121,AmountRole::remove_count,16595},
  {141,AmountRole::create_count,15946},
}};
inline constexpr Callback fire_arrows_callback{&functions[11],20087,Invocation::choice,1,1,INT32_MAX,{functions[11].byte_length,functions[11].body_sha256,fire_arrows_operands.data(),fire_arrows_operands.size()},view(fire_arrows_effects),{},true,false,true,false};

inline constexpr std::array<PackedInventoryTarget,5> magic_arrows_targets{{
  {"ITRW_ADDON_MAGICARROW",15945},
  {"ITMI_WOODBAR_BIG",16595},
  {"ITMI_SAW",16609},
  {"ITMISWORDRAW",16610},
  {"ITMI_ROCKCRYSTAL",16655},
}};
inline constexpr std::array<SymbolBinding,9> magic_arrows_own_bindings{{
  {"ITRW_ADDON_MAGICARROW",15945,7,1,0,1584,false,0,false,0},
  {"ITMI_WOODBAR_BIG",16595,7,1,0,1584,false,0,false,0},
  {"ITMI_SAW",16609,7,1,0,1584,false,0,false,0},
  {"ITMISWORDRAW",16610,7,1,0,1584,false,0,false,0},
  {"ITMI_ROCKCRYSTAL",16655,7,1,0,1584,false,0,false,0},
  {"MOBSI_WOODSAW",12327,2,1,1,-1,true,17,false,0},
  {"PC_MAKEARROWS_ADDON_HOUR",20087,7,1,0,1649,false,0,false,0},
  {"PC_MAKEARROWS_END",20084,7,1,0,1649,false,0,false,0},
  {"KNOWHOWTOMAKE_SPECIALARROWS",10491,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto magic_arrows_bindings=with_common(magic_arrows_own_bindings);
inline constexpr std::array<Requirement,2> magic_arrows_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,17},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10491,0,1},
}};
inline constexpr std::array<RequirementCheck,2> magic_arrows_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,17},CheckSite::owner_condition,&functions[7]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10491,0,1},CheckSite::owner_information,&functions[8]},
}};
inline constexpr std::array<ItemAmount,3> magic_arrows_costs{{
  {16610,1},
  {16655,1},
  {16595,1},
}};
inline constexpr std::array<ItemAmount,1> magic_arrows_outputs{{
  {15945,10},
}};
inline constexpr std::array<MaterialEffect,4> magic_arrows_effects{{
  {MaterialAction::remove,16610,1,0},
  {MaterialAction::remove,16655,1,0},
  {MaterialAction::remove,16595,1,0},
  {MaterialAction::create,15945,10,0},
}};
inline constexpr std::array<AmountOperand,7> magic_arrows_operands{{
  {1,1,1,0,false},
  {22,1,1,0,false},
  {43,1,1,0,false},
  {81,1,1,0,false},
  {101,1,1,0,false},
  {121,1,1,0,false},
  {141,10,10,0,false},
}};
inline constexpr std::array<OperandUse,7> magic_arrows_uses{{
  {1,AmountRole::minimum_inventory,16595},
  {22,AmountRole::minimum_inventory,16655},
  {43,AmountRole::minimum_inventory,16610},
  {81,AmountRole::remove_count,16610},
  {101,AmountRole::remove_count,16655},
  {121,AmountRole::remove_count,16595},
  {141,AmountRole::create_count,15945},
}};
inline constexpr Callback magic_arrows_callback{&functions[12],20087,Invocation::choice,1,1,INT32_MAX,{functions[12].byte_length,functions[12].body_sha256,magic_arrows_operands.data(),magic_arrows_operands.size()},view(magic_arrows_effects),{},true,false,true,false};

inline constexpr std::array<PackedInventoryTarget,4> bolts_targets{{
  {"ITMI_WOODBAR_BIG",16595},
  {"ITMI_SAW",16609},
  {"ITMISWORDRAW",16610},
  {"ITRW_BOLT",16805},
}};
inline constexpr std::array<SymbolBinding,8> bolts_own_bindings{{
  {"ITMI_WOODBAR_BIG",16595,7,1,0,1584,false,0,false,0},
  {"ITMI_SAW",16609,7,1,0,1584,false,0,false,0},
  {"ITMISWORDRAW",16610,7,1,0,1584,false,0,false,0},
  {"ITRW_BOLT",16805,7,1,0,1584,false,0,false,0},
  {"MOBSI_WOODSAW",12327,2,1,1,-1,true,17,false,0},
  {"PC_MAKEARROWS_ADDON_HOUR2",20098,7,1,0,1649,false,0,false,0},
  {"PC_MAKEARROWS_END",20084,7,1,0,1649,false,0,false,0},
  {"KNOWHOWTOMAKE_SPECIALARROWS",10491,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto bolts_bindings=with_common(bolts_own_bindings);
inline constexpr std::array<Requirement,2> bolts_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,17},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10491,0,1},
}};
inline constexpr std::array<RequirementCheck,2> bolts_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,17},CheckSite::owner_condition,&functions[14]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10491,0,1},CheckSite::owner_information,&functions[15]},
}};
inline constexpr std::array<ItemAmount,2> bolts_costs{{
  {16610,1},
  {16595,1},
}};
inline constexpr std::array<ItemAmount,1> bolts_outputs{{
  {16805,30},
}};
inline constexpr std::array<MaterialEffect,3> bolts_effects{{
  {MaterialAction::remove,16610,1,0},
  {MaterialAction::remove,16595,1,0},
  {MaterialAction::create,16805,30,0},
}};
inline constexpr std::array<AmountOperand,5> bolts_operands{{
  {1,1,1,0,false},
  {22,1,1,0,false},
  {59,1,1,0,false},
  {79,1,1,0,false},
  {99,30,30,0,false},
}};
inline constexpr std::array<OperandUse,5> bolts_uses{{
  {1,AmountRole::minimum_inventory,16595},
  {22,AmountRole::minimum_inventory,16610},
  {59,AmountRole::remove_count,16610},
  {79,AmountRole::remove_count,16595},
  {99,AmountRole::create_count,16805},
}};
inline constexpr Callback bolts_callback{&functions[17],20098,Invocation::choice,1,1,INT32_MAX,{functions[17].byte_length,functions[17].body_sha256,bolts_operands.data(),bolts_operands.size()},view(bolts_effects),{},true,false,true,false};

inline constexpr std::array<PackedInventoryTarget,5> magic_bolts_targets{{
  {"ITRW_ADDON_MAGICBOLT",15949},
  {"ITMI_WOODBAR_BIG",16595},
  {"ITMI_SAW",16609},
  {"ITMISWORDRAW",16610},
  {"ITMI_ROCKCRYSTAL",16655},
}};
inline constexpr std::array<SymbolBinding,9> magic_bolts_own_bindings{{
  {"ITRW_ADDON_MAGICBOLT",15949,7,1,0,1584,false,0,false,0},
  {"ITMI_WOODBAR_BIG",16595,7,1,0,1584,false,0,false,0},
  {"ITMI_SAW",16609,7,1,0,1584,false,0,false,0},
  {"ITMISWORDRAW",16610,7,1,0,1584,false,0,false,0},
  {"ITMI_ROCKCRYSTAL",16655,7,1,0,1584,false,0,false,0},
  {"MOBSI_WOODSAW",12327,2,1,1,-1,true,17,false,0},
  {"PC_MAKEARROWS_ADDON_HOUR2",20098,7,1,0,1649,false,0,false,0},
  {"PC_MAKEARROWS_END",20084,7,1,0,1649,false,0,false,0},
  {"KNOWHOWTOMAKE_SPECIALARROWS",10491,2,0,1,-1,false,0,false,0},
}};
inline constexpr auto magic_bolts_bindings=with_common(magic_bolts_own_bindings);
inline constexpr std::array<Requirement,2> magic_bolts_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,17},
  {RequirementKind::integer_equal,RequirementRole::knowledge,10491,0,1},
}};
inline constexpr std::array<RequirementCheck,2> magic_bolts_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,17},CheckSite::owner_condition,&functions[14]},
  {{RequirementKind::integer_equal,RequirementRole::knowledge,10491,0,1},CheckSite::owner_information,&functions[15]},
}};
inline constexpr std::array<ItemAmount,3> magic_bolts_costs{{
  {16610,1},
  {16655,1},
  {16595,1},
}};
inline constexpr std::array<ItemAmount,1> magic_bolts_outputs{{
  {15949,10},
}};
inline constexpr std::array<MaterialEffect,4> magic_bolts_effects{{
  {MaterialAction::remove,16610,1,0},
  {MaterialAction::remove,16655,1,0},
  {MaterialAction::remove,16595,1,0},
  {MaterialAction::create,15949,10,0},
}};
inline constexpr std::array<AmountOperand,7> magic_bolts_operands{{
  {1,1,1,0,false},
  {22,1,1,0,false},
  {43,1,1,0,false},
  {81,1,1,0,false},
  {101,1,1,0,false},
  {121,1,1,0,false},
  {141,10,10,0,false},
}};
inline constexpr std::array<OperandUse,7> magic_bolts_uses{{
  {1,AmountRole::minimum_inventory,16595},
  {22,AmountRole::minimum_inventory,16655},
  {43,AmountRole::minimum_inventory,16610},
  {81,AmountRole::remove_count,16610},
  {101,AmountRole::remove_count,16655},
  {121,AmountRole::remove_count,16595},
  {141,AmountRole::create_count,15949},
}};
inline constexpr Callback magic_bolts_callback{&functions[18],20098,Invocation::choice,1,1,INT32_MAX,{functions[18].byte_length,functions[18].body_sha256,magic_bolts_operands.data(),magic_bolts_operands.size()},view(magic_bolts_effects),{},true,false,true,false};

inline constexpr std::array<PackedInventoryTarget,4> heat_raw_steel_targets{{
  {"ITMISWORDRAW",16610},
  {"ITMISWORDRAWHOT",16611},
  {"ITMI_PLIERS_SMLT",16666},
  {"ITMI_PLIERS_SMLT_2",16667},
}};
inline constexpr std::array<SymbolBinding,7> heat_raw_steel_own_bindings{{
  {"ITMISWORDRAW",16610,7,1,0,1584,false,0,false,0},
  {"ITMISWORDRAWHOT",16611,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT",16666,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT_2",16667,7,1,0,1584,false,0,false,0},
  {"MOBSI_ORESMELTING",12320,2,1,1,-1,true,10,false,0},
  {"PC_ORESMELTING_ADDON",20991,7,1,0,1649,false,0,false,0},
  {"PC_ORESMELTING_END",20988,7,1,0,1649,false,0,false,0},
}};
inline constexpr auto heat_raw_steel_bindings=with_common(heat_raw_steel_own_bindings);
inline constexpr std::array<Requirement,1> heat_raw_steel_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,10},
}};
inline constexpr std::array<RequirementCheck,1> heat_raw_steel_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,10},CheckSite::owner_condition,&functions[42]},
}};
inline constexpr std::array<ItemAmount,1> heat_raw_steel_costs{{
  {16610,1},
}};
inline constexpr std::array<ItemAmount,1> heat_raw_steel_outputs{{
  {16611,1},
}};
inline constexpr std::array<MaterialEffect,2> heat_raw_steel_effects{{
  {MaterialAction::create,16611,1,0},
  {MaterialAction::remove,16610,1,0},
}};
inline constexpr std::array<AmountOperand,3> heat_raw_steel_operands{{
  {1,1,1,0,false},
  {37,1,1,0,false},
  {57,1,1,0,false},
}};
inline constexpr std::array<OperandUse,3> heat_raw_steel_uses{{
  {1,AmountRole::minimum_inventory,16610},
  {37,AmountRole::create_count,16611},
  {57,AmountRole::remove_count,16610},
}};
inline constexpr Callback heat_raw_steel_callback{&functions[45],20991,Invocation::choice,1,1,INT32_MAX,{functions[45].byte_length,functions[45].body_sha256,heat_raw_steel_operands.data(),heat_raw_steel_operands.size()},view(heat_raw_steel_effects),{},true,true,false,false};

inline constexpr std::array<PackedInventoryTarget,4> form_hot_blade_targets{{
  {"ITMISWORDRAWHOT",16611},
  {"ITMISWORDBLADEHOT",16612},
  {"ITMI_PLIERS_SMLT",16666},
  {"ITMI_PLIERS_SMLT_2",16667},
}};
inline constexpr std::array<SymbolBinding,7> form_hot_blade_own_bindings{{
  {"ITMISWORDRAWHOT",16611,7,1,0,1584,false,0,false,0},
  {"ITMISWORDBLADEHOT",16612,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT",16666,7,1,0,1584,false,0,false,0},
  {"ITMI_PLIERS_SMLT_2",16667,7,1,0,1584,false,0,false,0},
  {"MOBSI_ORESMELTING",12320,2,1,1,-1,true,10,false,0},
  {"PC_ORESMELTING_ADDON_X20",20999,7,1,0,1649,false,0,false,0},
  {"PC_ORESMELTING_END",20988,7,1,0,1649,false,0,false,0},
}};
inline constexpr auto form_hot_blade_bindings=with_common(form_hot_blade_own_bindings);
inline constexpr std::array<Requirement,1> form_hot_blade_requirements{{
  {RequirementKind::integer_equal,RequirementRole::context,12330,0,10},
}};
inline constexpr std::array<RequirementCheck,1> form_hot_blade_checks{{
  {{RequirementKind::integer_equal,RequirementRole::context,12330,0,10},CheckSite::owner_condition,&functions[47]},
}};
inline constexpr std::array<ItemAmount,1> form_hot_blade_costs{{
  {16611,1},
}};
inline constexpr std::array<ItemAmount,1> form_hot_blade_outputs{{
  {16612,1},
}};
inline constexpr std::array<MaterialEffect,2> form_hot_blade_effects{{
  {MaterialAction::create,16612,1,0},
  {MaterialAction::remove,16611,1,0},
}};
inline constexpr std::array<AmountOperand,3> form_hot_blade_operands{{
  {1,1,1,0,false},
  {37,1,1,0,false},
  {57,1,1,0,false},
}};
inline constexpr std::array<OperandUse,3> form_hot_blade_uses{{
  {1,AmountRole::minimum_inventory,16611},
  {37,AmountRole::create_count,16612},
  {57,AmountRole::remove_count,16611},
}};
inline constexpr Callback form_hot_blade_callback{&functions[50],20999,Invocation::choice,1,1,INT32_MAX,{functions[50].byte_length,functions[50].body_sha256,form_hot_blade_operands.data(),form_hot_blade_operands.size()},view(form_hot_blade_effects),{},true,true,false,false};

inline constexpr std::array<Descriptor,16> recipes{{
  {RecipeId::leather_sheep,"Leder aus Schaffell",Station::leatherwork,12,view(leather_sheep_targets),view(leather_sheep_bindings),view(leather_sheep_requirements),view(leather_sheep_checks),owner_20848,&functions[26],leather_sheep_callback,view(leather_sheep_costs),view(leather_sheep_outputs),leather_hand,Completion::end_production,&stations[0],view(leather_sheep_uses),"Durchgaenge",Actor::hero},
  {RecipeId::leather_wolf,"Leder aus Wolfsfell",Station::leatherwork,12,view(leather_wolf_targets),view(leather_wolf_bindings),view(leather_wolf_requirements),view(leather_wolf_checks),owner_20848,&functions[26],leather_wolf_callback,view(leather_wolf_costs),view(leather_wolf_outputs),leather_hand,Completion::end_production,&stations[0],view(leather_wolf_uses),"Durchgaenge",Actor::hero},
  {RecipeId::leather_lurker,"Leder aus Lurkerhaut",Station::leatherwork,12,view(leather_lurker_targets),view(leather_lurker_bindings),view(leather_lurker_requirements),view(leather_lurker_checks),owner_20848,&functions[26],leather_lurker_callback,view(leather_lurker_costs),view(leather_lurker_outputs),leather_hand,Completion::end_production,&stations[0],view(leather_lurker_uses),"Durchgaenge",Actor::hero},
  {RecipeId::leather_boar,"Leder aus Keilerfell",Station::leatherwork,12,view(leather_boar_targets),view(leather_boar_bindings),view(leather_boar_requirements),view(leather_boar_checks),owner_20848,&functions[26],leather_boar_callback,view(leather_boar_costs),view(leather_boar_outputs),leather_hand,Completion::end_production,&stations[0],view(leather_boar_uses),"Durchgaenge",Actor::hero},
  {RecipeId::leather_warg,"Leder aus Wargfell",Station::leatherwork,12,view(leather_warg_targets),view(leather_warg_bindings),view(leather_warg_requirements),view(leather_warg_checks),owner_20848,&functions[26],leather_warg_callback,view(leather_warg_costs),view(leather_warg_outputs),leather_hand,Completion::end_production,&stations[0],view(leather_warg_uses),"Durchgaenge",Actor::hero},
  {RecipeId::leather_shark,"Leder aus Sumpfhaihaut",Station::leatherwork,12,view(leather_shark_targets),view(leather_shark_bindings),view(leather_shark_requirements),view(leather_shark_checks),owner_20848,&functions[26],leather_shark_callback,view(leather_shark_costs),view(leather_shark_outputs),leather_hand,Completion::end_production,&stations[0],view(leather_shark_uses),"Durchgaenge",Actor::hero},
  {RecipeId::leather_shadowbeast,"Leder aus Schattenlaeuferfell",Station::leatherwork,12,view(leather_shadowbeast_targets),view(leather_shadowbeast_bindings),view(leather_shadowbeast_requirements),view(leather_shadowbeast_checks),owner_20848,&functions[26],leather_shadowbeast_callback,view(leather_shadowbeast_costs),view(leather_shadowbeast_outputs),leather_hand,Completion::end_production,&stations[0],view(leather_shadowbeast_uses),"Durchgaenge",Actor::hero},
  {RecipeId::leather_troll,"Leder aus Trollfell",Station::leatherwork,12,view(leather_troll_targets),view(leather_troll_bindings),view(leather_troll_requirements),view(leather_troll_checks),owner_20848,&functions[26],leather_troll_callback,view(leather_troll_costs),view(leather_troll_outputs),leather_hand,Completion::end_production,&stations[0],view(leather_troll_uses),"Durchgaenge",Actor::hero},
  {RecipeId::leather_black_troll,"Leder aus schwarzem Trollfell",Station::leatherwork,12,view(leather_black_troll_targets),view(leather_black_troll_bindings),view(leather_black_troll_requirements),view(leather_black_troll_checks),owner_20848,&functions[26],leather_black_troll_callback,view(leather_black_troll_costs),view(leather_black_troll_outputs),leather_hand,Completion::end_production,&stations[0],view(leather_black_troll_uses),"Durchgaenge",Actor::hero},
  {RecipeId::arrows,"Pfeile fertigen",Station::saw,17,view(arrows_targets),view(arrows_bindings),view(arrows_requirements),view(arrows_checks),owner_20087,&functions[9],arrows_callback,view(arrows_costs),view(arrows_outputs),saw_hand,Completion::ready_same_station,&stations[1],view(arrows_uses),"Lose",Actor::self},
  {RecipeId::fire_arrows,"Feuerpfeile fertigen",Station::saw,17,view(fire_arrows_targets),view(fire_arrows_bindings),view(fire_arrows_requirements),view(fire_arrows_checks),owner_20087,&functions[9],fire_arrows_callback,view(fire_arrows_costs),view(fire_arrows_outputs),saw_hand,Completion::ready_same_station,&stations[1],view(fire_arrows_uses),"Lose",Actor::self},
  {RecipeId::magic_arrows,"Magiepfeile fertigen",Station::saw,17,view(magic_arrows_targets),view(magic_arrows_bindings),view(magic_arrows_requirements),view(magic_arrows_checks),owner_20087,&functions[9],magic_arrows_callback,view(magic_arrows_costs),view(magic_arrows_outputs),saw_hand,Completion::ready_same_station,&stations[1],view(magic_arrows_uses),"Lose",Actor::self},
  {RecipeId::bolts,"Bolzen fertigen",Station::saw,17,view(bolts_targets),view(bolts_bindings),view(bolts_requirements),view(bolts_checks),owner_20098,&functions[16],bolts_callback,view(bolts_costs),view(bolts_outputs),saw_hand,Completion::ready_same_station,&stations[1],view(bolts_uses),"Lose",Actor::self},
  {RecipeId::magic_bolts,"Magiebolzen fertigen",Station::saw,17,view(magic_bolts_targets),view(magic_bolts_bindings),view(magic_bolts_requirements),view(magic_bolts_checks),owner_20098,&functions[16],magic_bolts_callback,view(magic_bolts_costs),view(magic_bolts_outputs),saw_hand,Completion::ready_same_station,&stations[1],view(magic_bolts_uses),"Lose",Actor::self},
  {RecipeId::heat_raw_steel,"Rohstahl erhitzen",Station::forge,10,view(heat_raw_steel_targets),view(heat_raw_steel_bindings),view(heat_raw_steel_requirements),view(heat_raw_steel_checks),owner_20991,&functions[44],heat_raw_steel_callback,view(heat_raw_steel_costs),view(heat_raw_steel_outputs),forge_hand,Completion::end_production,&stations[2],view(heat_raw_steel_uses),"Durchgaenge",Actor::hero},
  {RecipeId::form_hot_blade,"Heissen Stahl zur Klinge verarbeiten",Station::forge,10,view(form_hot_blade_targets),view(form_hot_blade_bindings),view(form_hot_blade_requirements),view(form_hot_blade_checks),owner_20999,&functions[49],form_hot_blade_callback,view(form_hot_blade_costs),view(form_hot_blade_outputs),forge_hand,Completion::end_production,&stations[2],view(form_hot_blade_uses),"Durchgaenge",Actor::hero},
}};

constexpr std::size_t target_capacity() noexcept {
  std::size_t result=0;
  for(const auto& recipe:recipes) if(recipe.targets.count>result) result=recipe.targets.count;
  return result;
}
// Derived workspace size for this catalog, no ingredient cap.
inline constexpr std::size_t catalog_target_capacity=target_capacity();
inline const Descriptor* find(RecipeId id) noexcept {
  for(const auto& recipe:recipes) if(recipe.id==id) return &recipe;
  return nullptr;
}
inline const Function* function_for(std::uint32_t symbol) noexcept {
  for(const auto& function:functions) if(function.symbol==symbol) return &function;
  return nullptr;
}
inline const SymbolBinding* binding_for(const Descriptor& recipe,std::int32_t symbol) noexcept {
  if(!recipe.bindings.data) return nullptr;
  for(const auto& binding:recipe.bindings)
    if(binding.symbol==static_cast<std::uint32_t>(symbol)) return &binding;
  return nullptr;
}
inline bool has_target(const Descriptor& recipe,std::int32_t symbol) noexcept {
  if(!recipe.targets.data) return false;
  for(const auto& target:recipe.targets) if(target.symbol==symbol) return true;
  return false;
}

namespace detail {
inline bool text(const char* a,const char* b) noexcept {
  return a&&b&&std::strcmp(a,b)==0;
}
inline bool same_function(const Function* a,const Function* b) noexcept {
  return a&&b&&text(a->name,b->name)&&a->symbol==b->symbol&&
    a->byte_offset==b->byte_offset&&a->byte_length==b->byte_length&&
    a->body_sha256==b->body_sha256&&a->argc==b->argc&&a->type==b->type&&
    a->flags==b->flags&&a->return_type==b->return_type&&a->parent_index==b->parent_index;
}
inline bool known_function(const Function* value) noexcept {
  return value&&value->name&&*value->name&&std::strlen(value->name)<=127&&
    value->byte_length&&value->byte_offset<lhiver::code_length&&
    value->byte_length<=lhiver::code_length-value->byte_offset&&
    same_function(value,function_for(value->symbol));
}
inline bool void_function(const Function* value) noexcept {
  return known_function(value)&&value->type==5&&value->flags==1&&!value->argc&&
    !value->return_type&&value->parent_index==-1;
}
inline bool same_binding(const SymbolBinding& a,const SymbolBinding& b) noexcept {
  return text(a.name,b.name)&&a.symbol==b.symbol&&a.type==b.type&&a.flags==b.flags&&
    a.elements==b.elements&&a.parent_index==b.parent_index&&a.check_value==b.check_value&&
    a.value==b.value&&a.check_offset==b.check_offset&&a.offset==b.offset;
}
inline bool same_requirement(const Requirement& a,const Requirement& b) noexcept {
  return a.kind==b.kind&&a.role==b.role&&a.symbol==b.symbol&&a.element==b.element&&a.value==b.value;
}
inline bool requirements(View<Requirement> values,View<Requirement> expected,
                         const Descriptor& recipe) noexcept {
  if(!values.data||values.count!=expected.count||!values.count) return false;
  for(std::size_t i=0;i<values.count;++i) {
    const auto& value=values[i];
    const auto* binding=binding_for(recipe,value.symbol);
    if(value.kind!=RequirementKind::integer_equal||value.element||
      (value.role!=RequirementRole::context&&value.role!=RequirementRole::knowledge)||
      !binding||binding->type!=2||binding->flags||binding->elements!=1) return false;
    bool found=false;
    for(const auto& want:expected) if(same_requirement(value,want)) found=true;
    if(!found) return false;
    for(std::size_t j=0;j<i;++j)
      if(value.symbol==values[j].symbol&&value.element==values[j].element) return false;
  }
  return true;
}
inline bool owner(const Owner& value,const Owner& expected,const Descriptor& recipe) noexcept {
  return value.instance==expected.instance&&value.number==expected.number&&value.npc==22243&&
    !value.important&&!value.trade&&value.permanent&&
    known_function(value.constructor)&&known_function(value.condition)&&void_function(value.information)&&
    same_function(value.constructor,expected.constructor)&&
    same_function(value.condition,expected.condition)&&same_function(value.information,expected.information)&&
    value.constructor->symbol==static_cast<std::uint32_t>(value.instance)&&
    value.constructor->type==7&&value.constructor->flags==1&&!value.constructor->argc&&
    !value.constructor->return_type&&value.constructor->parent_index==1649&&
    value.condition->type==5&&value.condition->flags==3&&!value.condition->argc&&
    value.condition->return_type==2&&value.condition->parent_index==-1&&
    value.conditions.count==1&&requirements(value.conditions,expected.conditions,recipe)&&
    value.conditions[0].symbol==12330&&value.conditions[0].value==recipe.production_context&&
    value.conditions[0].role==RequirementRole::context;
}
inline bool hand_item(const HandItem& value,const HandItem& expected) noexcept {
  return value.symbol==expected.symbol&&value.amount==expected.amount&&
    value.source==expected.source&&value.require_stored_absent==expected.require_stored_absent;
}
inline bool amounts(View<ItemAmount> values,View<ItemAmount> expected,
                    const Descriptor& recipe) noexcept {
  if(!values.data||values.count!=expected.count||!values.count) return false;
  for(std::size_t i=0;i<values.count;++i) {
    const auto& value=values[i];
    if(!value.units||value.units>INT32_MAX||!has_target(recipe,value.symbol)) return false;
    bool found=false;
    for(const auto& want:expected) if(value.symbol==want.symbol&&value.units==want.units) found=true;
    if(!found) return false;
    for(std::size_t j=0;j<i;++j) if(value.symbol==values[j].symbol) return false;
  }
  return true;
}
} // namespace detail

// Own valid C++ memory regions, no arbitrary pointer checks. Copies
// must not reinterpret the registered facts. Thus, for example, a
// modified learning checkpoint is not merely a formally valid predicate.
// Neither this comparison nor a SHA field reads the current game parser.
inline bool structurally_valid(const Descriptor& recipe) noexcept {
  namespace d=detail;
  const auto* expected=find(recipe.id);
  if(!expected||!recipe.name||!*recipe.name||!recipe.quantity_unit||!*recipe.quantity_unit||
    recipe.station!=expected->station||recipe.production_context!=expected->production_context||
    recipe.completion!=expected->completion||recipe.material_actor!=expected->material_actor||
    !recipe.targets.data||recipe.targets.count!=expected->targets.count||
    !recipe.bindings.data||recipe.bindings.count!=expected->bindings.count||
    !recipe.station_profile) return false;
  for(std::size_t i=0;i<recipe.bindings.count;++i) {
    const auto& binding=recipe.bindings[i];
    const auto* want=binding_for(*expected,static_cast<std::int32_t>(binding.symbol));
    if(!want||!d::same_binding(binding,*want)) return false;
    for(std::size_t j=0;j<i;++j)
      if(binding.symbol==recipe.bindings[j].symbol||d::text(binding.name,recipe.bindings[j].name)) return false;
  }
  for(std::size_t i=0;i<recipe.targets.count;++i) {
    const auto& target=recipe.targets[i];
    if(!target.name||!*target.name||std::strlen(target.name)>63||target.symbol<=0) return false;
    bool found=false;
    for(const auto& want:expected->targets)
      if(target.symbol==want.symbol&&d::text(target.name,want.name)) found=true;
    const auto* binding=binding_for(recipe,target.symbol);
    if(!found||!binding||!d::text(binding->name,target.name)||binding->type!=7||
      binding->flags!=1||binding->elements||binding->parent_index!=1584) return false;
    for(std::size_t j=0;j<i;++j)
      if(target.symbol==recipe.targets[j].symbol||d::text(target.name,recipe.targets[j].name)) return false;
  }
  if(!d::amounts(recipe.costs,expected->costs,recipe)||
    !d::amounts(recipe.outputs,expected->outputs,recipe)||
    !d::requirements(recipe.requirements,expected->requirements,recipe)||
    !d::owner(recipe.owner,expected->owner,recipe)||
    !d::void_function(recipe.back)||!d::same_function(recipe.back,expected->back)) return false;
  for(const auto& input:recipe.costs) for(const auto& output:recipe.outputs)
    if(input.symbol==output.symbol) return false;
  if(!d::hand_item(recipe.hand.left,expected->hand.left)||
    !d::hand_item(recipe.hand.right,expected->hand.right)||recipe.hand.anchor!=expected->hand.anchor) return false;
  for(const auto* hand:{&recipe.hand.left,&recipe.hand.right}) {
    if(hand->source==HandSource::empty) {
      if(hand->symbol!=-1||hand->amount||hand->require_stored_absent) return false;
    } else {
      if(hand->amount!=1||!has_target(recipe,hand->symbol)) return false;
      for(const auto& input:recipe.costs) if(input.symbol==hand->symbol) return false;
      for(const auto& output:recipe.outputs) if(output.symbol==hand->symbol) return false;
    }
  }
  const auto& station=*recipe.station_profile;
  const auto& want_station=*expected->station_profile;
  if(station.station!=recipe.station||station.production_context!=recipe.production_context||
    !d::text(station.on_state,want_station.on_state)||!d::text(station.scheme,want_station.scheme)||
    !d::text(station.use_with_item,want_station.use_with_item)||
    !d::text(station.condition_name,want_station.condition_name)||
    !d::void_function(station.entry)||!d::same_function(station.entry,want_station.entry)||
    !d::owner(station.end,want_station.end,recipe)) return false;
  if(want_station.mob_condition) {
    if(!d::known_function(station.mob_condition)||
      !d::same_function(station.mob_condition,want_station.mob_condition)) return false;
  } else if(station.mob_condition) return false;
  if(!recipe.checks.data||recipe.checks.count!=expected->checks.count) return false;
  for(std::size_t i=0;i<recipe.checks.count;++i) {
    const auto& check=recipe.checks[i];
    if(!d::known_function(check.function)) return false;
    bool found=false;
    for(const auto& want:expected->checks)
      if(d::same_requirement(check.requirement,want.requirement)&&check.site==want.site&&
        d::same_function(check.function,want.function)) found=true;
    if(!found) return false;
    for(std::size_t j=0;j<i;++j)
      if(d::same_requirement(check.requirement,recipe.checks[j].requirement)) return false;
  }
  const auto& callback=recipe.callback;
  const auto& want=expected->callback;
  if(!d::void_function(callback.function)||!d::same_function(callback.function,want.function)||
    callback.owner_instance!=recipe.owner.instance||callback.invocation!=Invocation::choice||
    callback.original_quantity!=1||callback.minimum_quantity!=1||callback.maximum_quantity!=INT32_MAX||
    !callback.body_checks_resources||callback.ends_production!=want.ends_production||
    callback.clears_choices!=want.clears_choices||callback.quantity_text_reviewed!=want.quantity_text_reviewed||
    callback.failure_effects.count||callback.amount.length!=callback.function->byte_length||
    callback.amount.length>AmountPatch::max_body||callback.amount.digest!=callback.function->body_sha256||
    !callback.amount.operands||callback.amount.count!=want.amount.count||
    callback.amount.count>AmountPatch::max_operands||
    !callback.effects.data||callback.effects.count!=want.effects.count||
    !recipe.operand_uses.data||recipe.operand_uses.count!=callback.amount.count) return false;
  for(std::size_t i=0;i<callback.amount.count;++i) {
    const auto& operand=callback.amount.operands[i];
    const auto& use=recipe.operand_uses[i];
    const auto& want_operand=want.amount.operands[i];
    const auto& want_use=expected->operand_uses[i];
    if(!operand.offset||operand.offset>=callback.amount.length||callback.amount.length-operand.offset<4||
      operand.original<=0||!operand.units||operand.units>INT32_MAX||operand.reserved||operand.allow_zero||
      operand.original!=static_cast<std::int32_t>(operand.units)||
      operand.offset!=want_operand.offset||operand.original!=want_operand.original||
      use.offset!=operand.offset||use.offset!=want_use.offset||use.role!=want_use.role||use.symbol!=want_use.symbol)
      return false;
    for(std::size_t j=0;j<i;++j) {
      const auto other=callback.amount.operands[j].offset;
      if(operand.offset<other+4&&other<operand.offset+4) return false;
    }
  }
  for(std::size_t i=0;i<callback.effects.count;++i) {
    const auto& effect=callback.effects[i];
    const auto& want_effect=want.effects[i];
    if(effect.action!=want_effect.action||effect.symbol!=want_effect.symbol||
      effect.units!=want_effect.units||effect.reserved||!has_target(recipe,effect.symbol)) return false;
  }
  return true;
}

// Only an arithmetically matching static template. Unknown current stock,
// SELF context, learning, hand/animation, messages and runtime binding
// remain own gates. In particular quantity_text_reviewed is false here.
inline const Callback* callback_for_quantity(const Descriptor& recipe,std::uint32_t quantity) noexcept {
  if(!quantity||quantity>INT32_MAX||!structurally_valid(recipe)) return nullptr;
  for(const auto amounts:{recipe.costs,recipe.outputs}) for(const auto& amount:amounts)
    if(std::uint64_t{amount.units}*quantity>INT32_MAX) return nullptr;
  for(std::size_t i=0;i<recipe.callback.amount.count;++i)
    if(std::uint64_t{recipe.callback.amount.operands[i].units}*quantity>INT32_MAX) return nullptr;
  return &recipe.callback;
}
} // namespace ubc::station_recipe_catalog
