// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/recipe_catalog.hpp"
#include <limits>

// Own metadata, L'Hiver DE 2.0.4, 01.11.2026. No original scripts, texts, or bytecode
// bytes. DAT-SHA: 71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4.
// Separate addendum catalog, NO runtime permission of the 13 paths.
namespace ubc::alchemy_recipe_catalog {
using recipe_catalog::View;
using recipe_catalog::view;
using recipe_catalog::Owner;
using recipe_catalog::Callback;
using recipe_catalog::Requirement;
using recipe_catalog::RequirementKind;
using recipe_catalog::RequirementRole;
using recipe_catalog::ItemAmount;
using recipe_catalog::MaterialEffect;
using recipe_catalog::MaterialAction;
using recipe_catalog::FixedEffect;
using recipe_catalog::Actor;
using recipe_catalog::Invocation;
using recipe_catalog::HandContract;
using recipe_catalog::HandEvidence;
using lhiver::Function;
using lhiver::SymbolBinding;
using lhiver::digest_literal;
enum class RecipeId : std::uint32_t {
  lou_spirit = 0x4001,
  sleep_spirit = 0x4002,
  fish_spirit = 0x4003,
  combined_mana = 0x4004,
  combined_health = 0x4005,
  permanent_health = 0x4006,
  permanent_mana = 0x4007,
  permanent_dexterity = 0x4008,
  permanent_strength = 0x4009,
  speed = 0x400a,
  swamp_roll = 0x400b,
  strong_swamp_roll = 0x400c,
  dragon_egg_potion = 0x400d,
};
enum class Category { root, booze, mana, health, special, tobacco };
enum class AmountRole { minimum_inventory, remove_count, create_count };
struct OperandUse { std::uint32_t offset; AmountRole role; std::int32_t symbol; };
enum class EffectKind { remove, create, print, say, end_production };
struct Effect {
  EffectKind kind;
  std::uint32_t callee, offset;
  std::int32_t item;
  std::uint32_t units;
  Actor actor; // Effective only for material effects.
};
struct PrintFingerprint { std::uint32_t symbol, length; lhiver::Digest digest; };
struct FlaskDebit {
  std::int32_t symbol;
  std::uint32_t units, reserved_hand, failure_refund;
  // Only after positive original delta AND operand restore,
  // synchronous in own writer, before the normal station run-through.
  bool after_verified_original, original_removes_stored_flasks;
};
struct Descriptor {
  RecipeId id;
  const char* name; // Own designation.
  Category category;
  recipe_catalog::Station station;
  std::int32_t production_context;
  View<PackedInventoryTarget> targets;
  View<SymbolBinding> bindings;
  View<Requirement> requirements; // Only original owner condition, no invented learning flags.
  Owner owner;
  Callback callback; // One original INFO, no choice and no callback loop.
  View<ItemAmount> costs, outputs; // Total consumption per N, including flask.
  Actor material_actor; // SELF recipes require separately positive SELF==HERO.
  HandContract hand;
  View<OperandUse> operand_uses;
  View<Effect> success, failure; // Original order; deficit refund remains 1.
  View<PrintFingerprint> prints; // No original texts; known messages contain no quantity.
  View<const Function*> item_constructors; // No calls/global writes in these constructors.
  const Function *station_entry, *station_end, *end_production, *say;
  FlaskDebit extra_flasks;
};
inline constexpr bool runtime_ready = false;
inline constexpr std::int32_t flask_symbol = 16604;
inline constexpr const char* station_scheme = "LAB";
inline constexpr const char* station_on_state = "POTIONALCHEMY";
inline constexpr const char* station_use_with_item = "ITMI_FLASK";
// The hand lifetime comes from the exactly known LAB path. These 13 recipes
// themselves were neither individually nor with N released during world runtime.
inline constexpr HandContract lab_hand{16604,16604,1,true,true,1,HandEvidence::inferred_same_station,true};
inline constexpr FlaskDebit flask_debit{16604,1,1,1,true,false};

inline constexpr std::array<Function,74> functions{{
  {"B_SAY",14646,218446,39,digest_literal("b8b93bf2920074b9958b398d5d642935220402bd20ff5044c51f9a416675d6ad"),3,5,1,0,-1},
  {"ITFO_ADDON_RUM",15673,253534,148,digest_literal("7529b2173d7c407d80f6dddffeaf8650fba83146aae38d982ee6b6a3302535dc"),0,7,1,0,1584},
  {"ITFO_ADDON_LOUSHAMMER",15678,253877,136,digest_literal("13e78bbff10c191c8e1fcf17e932737edf5979c427966add2f6aa3865333c284"),0,7,1,0,1584},
  {"ITFO_ADDON_SCHLAFHAMMER",15680,254061,136,digest_literal("54025799b620f3bbc13c09894fe3fbcc35a593d6d804d745e0cd7c46b464254f"),0,7,1,0,1584},
  {"ITMI_ADDON_JOINT_01",15704,256475,135,digest_literal("1530fca3c44dd740507bc92d34fe27df91ba9926bdd069203b12822f4d4d71de"),0,7,1,0,1584},
  {"ITFO_ADDON_SCHNELLERHERING",15736,264801,148,digest_literal("16d9a1f6b36f70079fe5484ed69cf88bf37c740c2c666bf99a2514df3131f9a7"),0,7,1,0,1584},
  {"ITPO_HEALTH_ADDON_04",15743,265477,158,digest_literal("ea90c9882688a1a71695ac70b9eb3d0284352046975cde0b814e351472799328"),0,7,1,0,1584},
  {"ITPO_MANA_ADDON_04",15745,265673,158,digest_literal("b63b8b942c0e7783d77fe4f0bb19778990804b0853db2b5849b2b2150ca419da"),0,7,1,0,1584},
  {"ITAT_SHARKTEETH",16143,300809,149,digest_literal("3f9c1d8809bc7a02161ccaf677ccb7fe1d4aa6d877be4a8ae3a6df9246609ac5"),0,7,1,0,1584},
  {"ITFO_FISH",16375,329848,148,digest_literal("35b8d0b1b975f3dd5ee13cf9f28f6c5a5d60fe2dd1df16cb5dc7595e57acaa09"),0,7,1,0,1584},
  {"ITMI_FLASK",16604,364987,102,digest_literal("c11d966476cbbe17ba211e07e822e245c415ad766f6c02d47382584666902ab0"),0,7,1,0,1584},
  {"ITMI_JOINT",16620,366451,135,digest_literal("12ac6b2ffd0acffab2dc36fbe29327748ea987415cc491f422020dfdfdbcebe9"),0,7,1,0,1584},
  {"ITMI_SULFUR",16652,370365,113,digest_literal("aae638154bd62c304fac4abc558ddef4f65b500f26125ea607ca18f8cfab5511"),0,7,1,0,1584},
  {"ITMI_DARKPEARL",16659,371123,113,digest_literal("8903c0765e046dbd33d6063ca039760015f2120ba778c86792acb2e1bba2497d"),0,7,1,0,1584},
  {"ITPL_BEET",16704,372153,124,digest_literal("d7b82e86bc27b5cadad3f22bf73198e88816adf4692c58e2a448315f36ad0be1"),0,7,1,0,1584},
  {"ITPL_SWAMPHERB",16706,372303,124,digest_literal("4324807ab6d5b46bcd5ed887f6483e9b4802e5193a2e6a6cf2581905bcaac07e"),0,7,1,0,1584},
  {"ITPL_MANA_HERB_03",16712,372821,148,digest_literal("0982534dc0fcdc0140794c3dd7fb5ffeab4d5f89466b7cf07ca3d4983910b053"),0,7,1,0,1584},
  {"ITPL_HEALTH_HERB_03",16718,373338,148,digest_literal("2e6bd977e79c2db07a76b15648de6e6023c1dd0d9c960b0902e6273a58d1f7ff"),0,7,1,0,1584},
  {"ITPL_DEX_HERB_01",16720,373512,148,digest_literal("ff8f4489d24737217a6f76dab09e680fd2cd45269fd759409a3e0e8acd0b07cc"),0,7,1,0,1584},
  {"ITPL_STRENGTH_HERB_01",16722,373681,148,digest_literal("bc1c0ad96017cc2d26732104d24e02da50e0753533f3719d2a199465f23185e4"),0,7,1,0,1584},
  {"ITPL_SPEED_HERB_01",16724,373850,154,digest_literal("994484915faac49e4dd4a698ed6189adc9ff1a554493806436caa8fbd2fd5669"),0,7,1,0,1584},
  {"ITPL_TEMP_HERB",16736,375043,154,digest_literal("66e1cedbce6537fdcd125c7819dba7dc5a4727f58a688e3827ed5a9a97cfd581"),0,7,1,0,1584},
  {"ITPL_PERM_HERB",16738,375223,154,digest_literal("408fcd672078dd2b3dfbe14d8bf5041fcef01fa88a536cd6937450b2124d46ac"),0,7,1,0,1584},
  {"ITPO_MANA_01",16766,375403,194,digest_literal("c8ac7508452947e2bca0bc315a8db9e5ae4f8637aab22f0db7dcec6da2d2f5ce"),0,7,1,0,1584},
  {"ITPO_HEALTH_01",16775,376336,194,digest_literal("ae4105a34ccb79b0f9565a574ce560816331c3e5bc602904ad4da91fd4e5a5fc"),0,7,1,0,1584},
  {"ITPO_PERM_STR",16784,377278,170,digest_literal("870548afcd0fe94b3a0f14764d957a9d7559a2b59d37bed6fb85d8632ea29518"),0,7,1,0,1584},
  {"ITPO_PERM_DEX",16787,377638,170,digest_literal("28caa55f21be075f1be7940d8aa04353cb529fbed8e86edc155e5b80fd88feb3"),0,7,1,0,1584},
  {"ITPO_PERM_HEALTH",16790,377998,170,digest_literal("ae4aff1a191200528d3e1aae1320af23f3b0dc95163fba47632a1d8c59e4c3a7"),0,7,1,0,1584},
  {"ITPO_PERM_MANA",16794,378533,170,digest_literal("394f82ddf1a9e25e592df74ebe360e1c8a9acd5dc7e7a9dde50b5977c92d3c09"),0,7,1,0,1584},
  {"ITPO_SPEED",16798,379058,188,digest_literal("c0272a2252e21ec9b01987ce7361e99dea8cc1d31f84c655fe4f7bed8ef21944"),0,7,1,0,1584},
  {"ITPO_MEGADRINK",16800,379277,158,digest_literal("8719f3d3622c772d387d9d7124f01c5e10bb46e6162b75954564d6c070127dc3"),0,7,1,0,1584},
  {"ITAT_DRAGONEGG_MIS",17585,473908,155,digest_literal("d599dc840c05471a500fafca59e7c975da6af6d15c42c990dd21e6ded2eaf08f"),0,7,1,0,1584},
  {"B_ENDPRODUCTIONDIALOG",19965,672156,49,digest_literal("e9fcd2f7db76bab4ed7c5ceb6e5a98c3ab3b8166a3614a4dda2513092da34e64"),0,5,1,0,-1},
  {"POTIONALCHEMY_S1",20276,723073,65,digest_literal("3ee4af5cf7c6c967d039c7585d9f4a085da7a638ee43ee02fe0c142c1f8d00ab"),0,5,1,0,-1},
  {"PC_POTIONALCHEMY_END_INFO",20279,723288,26,digest_literal("6a30e7f39ca34bd02e28e4e2be7c2932e1ddf2ba6c0eb69fa37542c3b74a0262"),0,5,1,0,-1},
  {"PC_BOOZE_LOU",20287,723590,67,digest_literal("b55decd82f0ef6c78073b69a5df45799318c86aca88f10757362236d6349ea88"),0,7,1,0,1649},
  {"PC_BOOZE_LOU_CONDITION",20288,723657,47,digest_literal("70972ca699c0e7999096afc6b58cf300fda6906131b926dcfbb11b26ccfcfb34"),0,5,3,2,-1},
  {"PC_BOOZE_LOU_INFO",20289,723704,283,digest_literal("f0af88c388326f589e3361bf00a6af50354e1e8808f51bb8456ba6b0b4e4528e"),0,5,1,0,-1},
  {"PC_BOOZE_SCHLAF",20290,723987,67,digest_literal("9ec64d8b0487f908d2479d36ce234c1d313566dc9fd660a7e8a37f246bb4927d"),0,7,1,0,1649},
  {"PC_BOOZE_SCHLAF_CONDITION",20291,724054,47,digest_literal("7184debafe98e27a17c1bbcae318dad92b0a6409ffb6a36d2f266554d1594cea"),0,5,3,2,-1},
  {"PC_BOOZE_SCHLAF_INFO",20292,724101,283,digest_literal("9ea9c76fe8d31e051e5a1b45036bfeee615e782c213d5de6570b9958a59b02fd"),0,5,1,0,-1},
  {"PC_BOOZE_SCHNELLERHERING",20293,724384,67,digest_literal("5a400666c6f08fa00e191c07ab567b04fc1d3986d6b61ff774a014bbd47cbe13"),0,7,1,0,1649},
  {"PC_BOOZE_SCHNELLERHERING_CONDITION",20294,724451,47,digest_literal("34b3a512644bacecfc08086e9247e4e55e9b3d2348979d434a8c68282a364019"),0,5,3,2,-1},
  {"PC_BOOZE_SCHNELLERHERING_INFO",20295,724498,241,digest_literal("ba028d0c7dd26bbb09fcb92d9da5c895fef8587761e6222369162a7f1333e282"),0,5,1,0,-1},
  {"PC_ITPO_MANA_04",20327,728206,67,digest_literal("2404ae7da9e17c4ad914a5f04b0942bd1431bd2a0f4b699025506f4a0dca767e"),0,7,1,0,1649},
  {"PC_ITPO_MANA_04_CONDITION",20328,728273,48,digest_literal("f66921729f54f355207cc64fc97d28cb1f6c257fd854a713861c66bec4796bfc"),0,5,3,2,-1},
  {"PC_ITPO_MANA_04_INFO",20329,728321,199,digest_literal("62b52dc5b0de13029a8fce8a4fcf96262ee6826ee1b65c4cc2506d7a7bc8caf6"),0,5,1,0,-1},
  {"PC_ITPO_HEALTH_04",20358,731357,67,digest_literal("76b0da5e59fbf7f10480ad9456a9765ae69e1055b0e5861bef9f329ce29dc8bb"),0,7,1,0,1649},
  {"PC_ITPO_HEALTH_04_CONDITION",20359,731424,48,digest_literal("b1550e745d64c43e247ce4de23d959804b693d26395918001a3013a11161f228"),0,5,3,2,-1},
  {"PC_ITPO_HEALTH_04_INFO",20360,731472,199,digest_literal("52c15d352331a8a441c74bb1748d86a769cbd940fd97c222ca0d91bff6b0c504"),0,5,1,0,-1},
  {"PC_ITPO_PERM_HEALTH",20371,732385,67,digest_literal("11160daf842cf80ef59ff403314dc0fd9d7caf23a43d82a225d0d1af189f446c"),0,7,1,0,1649},
  {"PC_ITPO_PERM_HEALTH_CONDITION",20372,732452,48,digest_literal("3a6b9a8f2c94fe32cf44d177b5485d0067ef1ea6aca1ccfedfee5899676340ea"),0,5,3,2,-1},
  {"PC_ITPO_PERM_HEALTH_INFO",20373,732500,199,digest_literal("bc165cdf9b920c1d60003c3a3488315135c217db8c1f75774f1bffea26a5e6bc"),0,5,1,0,-1},
  {"PC_ITPO_PERM_MANA",20374,732699,67,digest_literal("139480a2adbc7a926935053fc11df2a3d3aa13790f74571e0f2a07fadca98fa1"),0,7,1,0,1649},
  {"PC_ITPO_PERM_MANA_CONDITION",20375,732766,48,digest_literal("b0091b930a9579faddd83e1a2056b810e5d1fc3c678f13e9ed507370e01dee62"),0,5,3,2,-1},
  {"PC_ITPO_PERM_MANA_INFO",20376,732814,199,digest_literal("b8c99a3bdad7b40685b55eaf50aa3dc2916dda2ce5dbbeddbcfc662100527572"),0,5,1,0,-1},
  {"PC_ITPO_DEX",20377,733013,67,digest_literal("e6583375a478148246bfce56118d83dce59ccab2e5e5c356419a9b1239fe235d"),0,7,1,0,1649},
  {"PC_ITPO_DEX_CONDITION",20378,733080,48,digest_literal("3aec4cc39dd662703ce32edad99587f2fddc2d3d3a41d833bf02119a2523feea"),0,5,3,2,-1},
  {"PC_ITPO_DEX_INFO",20379,733128,199,digest_literal("319ac555e79a959421831030cfba64058b99d91f80cb9e8b0830d84f223c90b7"),0,5,1,0,-1},
  {"PC_ITPO_STRG",20380,733327,67,digest_literal("1c81f11600b178a285194752992295e899e131ca177a465498a4a5ec11bf54f5"),0,7,1,0,1649},
  {"PC_ITPO_STRG_CONDITION",20381,733394,48,digest_literal("0a19cb30cbd0ff64af975d5cca854812c1166a0cb85d35ffcd35136e1723b002"),0,5,3,2,-1},
  {"PC_ITPO_STRG_INFO",20382,733442,199,digest_literal("e697531812266652032fd2baa4dad1b306f97eec7f608f08fc6b7e197c71ecc2"),0,5,1,0,-1},
  {"PC_ITPO_SPEED",20383,733641,67,digest_literal("fc07e51caed0700c299eaa57442a8568fb87e0e4177cd08b8d463f114a72dd93"),0,7,1,0,1649},
  {"PC_ITPO_SPEED_CONDITION",20384,733708,48,digest_literal("30a01039dc79711e20e2123f383e67334e8601c996777118796fb222e9dde0d9"),0,5,3,2,-1},
  {"PC_ITPO_SPEED_INFO",20385,733756,199,digest_literal("9e0b44aa3089815556e6f2a81f10ccccc971f4df2249a3e694408ae63c53eed3"),0,5,1,0,-1},
  {"PC_ITMI_JOINT",20393,734231,67,digest_literal("e4b95670314a126fa959e01fb1f26422fca356aa95aedd25b0f8b94dcd2afe40"),0,7,1,0,1649},
  {"PC_ITMI_JOINT_CONDITION",20394,734298,35,digest_literal("ea71f02f13494f80425ebc1f0cbce1c0c6d659249a0ca584b4a5c129c314e43b"),0,5,3,2,-1},
  {"PC_ITMI_JOINT_INFO",20395,734333,137,digest_literal("55d5a3949583621a83388493fb2656a49c74cdac8740859f49962fb2ae7aa6ab"),0,5,1,0,-1},
  {"PC_ITMI_ADDON_JOINT_01",20396,734470,67,digest_literal("069dddc03b03637a0ca8515a6b49f1b5b1fa68f74c30d6b1884ed1fc1248d9ff"),0,7,1,0,1649},
  {"PC_ITMI_ADDON_JOINT_01_CONDITION",20397,734537,47,digest_literal("fd2b3c5ab2515c77bba7e9db8e42b51d937da28a01e1b913009f379068a3e981"),0,5,3,2,-1},
  {"PC_ITMI_ADDON_JOINT_01_INFO",20398,734584,179,digest_literal("624d0022c0415ffc75349de0cb9bd6568550d37bd141643f344eb1cc8243cd34"),0,5,1,0,-1},
  {"PC_ITPO_MEGADRINK",20406,735195,67,digest_literal("b5fb96c2a70534280b22f6462eb505dd3cc40e18aa427eeb33e40cb9711dffc1"),0,7,1,0,1649},
  {"PC_ITPO_MEGADRINK_CONDITION",20407,735262,96,digest_literal("6eca627ecccf55629d4ee488faf61d060a1bae4810fd4729d380baa317e19344"),0,5,3,2,-1},
  {"PC_ITPO_MEGADRINK_INFO",20408,735358,241,digest_literal("2afecd9612a3420b4e9ca3e43ef2f58e2fe6daae51bd98a4b30f48196151b6bd"),0,5,1,0,-1},
}};
inline constexpr const Function* function_for(std::uint32_t symbol) noexcept {
  for (const auto& f:functions) if (f.symbol==symbol) return &f;
  return nullptr;
}
inline constexpr std::array<SymbolBinding,23> common_bindings{{
  {"SELF",1679,7,1,0,1537,false,0,false,0},
  {"OTHER",1680,7,1,0,1537,false,0,false,0},
  {"VICTIM",1681,7,1,0,1537,false,0,false,0},
  {"HERO",1683,7,1,0,1537,false,0,false,0},
  {"PC_HERO",22243,7,1,0,21074,false,0,false,0},
  {"C_NPC",1537,4,0,32,-1,false,0,false,0},
  {"C_ITEM",1584,4,0,44,-1,false,0,false,0},
  {"C_NPC.AIVAR",1563,2,4,100,1537,false,0,true,648},
  {"AIV_INVINCIBLE",11998,2,1,1,-1,true,4,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"MOBSI_NONE",12310,2,1,1,-1,true,0,false,0},
  {"MOBSI_POTIONALCHEMY",12314,2,1,1,-1,true,4,false,0},
  {"TRUE",12297,2,1,1,-1,true,1,false,0},
  {"FALSE",12298,2,1,1,-1,true,0,false,0},
  {"C_INFO",1649,4,0,8,-1,false,0,false,0},
  {"C_INFO.NPC",1650,2,4,1,1649,false,0,true,0},
  {"C_INFO.NR",1651,2,4,1,1649,false,0,true,4},
  {"C_INFO.IMPORTANT",1652,2,4,1,1649,false,0,true,8},
  {"C_INFO.CONDITION",1653,5,4,1,1649,false,0,true,12},
  {"C_INFO.INFORMATION",1654,5,4,1,1649,false,0,true,16},
  {"C_INFO.DESCRIPTION",1655,3,4,1,1649,false,0,true,20},
  {"C_INFO.TRADE",1656,2,4,1,1649,false,0,true,40},
  {"C_INFO.PERMANENT",1657,2,4,1,1649,false,0,true,44},
}};
inline constexpr std::array<Requirement,3> r20289_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::context,20280,0,1},{RequirementKind::integer_equal,RequirementRole::knowledge,11325,0,1}}};
inline constexpr std::array<SymbolBinding,10> r20289_bindings{{
  {"ITPL_SWAMPHERB",16706,7,1,0,1584,false,0,false,0},
  {"ITPL_BEET",16704,7,1,0,1584,false,0,false,0},
  {"ITAT_SHARKTEETH",16143,7,1,0,1584,false,0,false,0},
  {"ITFO_ADDON_RUM",15673,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITFO_ADDON_LOUSHAMMER",15678,7,1,0,1584,false,0,false,0},
  {"PC_BOOZE_LOU",20287,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"BOOZESTART",20280,2,0,1,-1,false,0,false,0},
  {"KNOWS_LOUSHAMMER",11325,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,6> r20289_targets{{{"ITPL_SWAMPHERB",16706},{"ITPL_BEET",16704},{"ITAT_SHARKTEETH",16143},{"ITFO_ADDON_RUM",15673},{"ITMI_FLASK",16604},{"ITFO_ADDON_LOUSHAMMER",15678}}};
inline constexpr std::array<ItemAmount,5> r20289_costs{{{16706,1},{16704,2},{16143,1},{15673,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20289_outputs{{{15678,1}}};
inline constexpr std::array<AmountOperand,9> r20289_operands{{{1,1,1,0,false},{22,1,1,0,false},{43,2,2,0,false},{64,1,1,0,false},{103,1,1,0,false},{123,2,2,0,false},{143,1,1,0,false},{163,1,1,0,false},{213,1,1,0,false}}};
inline constexpr std::array<OperandUse,9> r20289_roles{{{1,AmountRole::minimum_inventory,15673},{22,AmountRole::minimum_inventory,16143},{43,AmountRole::minimum_inventory,16704},{64,AmountRole::minimum_inventory,16706},{103,AmountRole::remove_count,16706},{123,AmountRole::remove_count,16704},{143,AmountRole::remove_count,16143},{163,AmountRole::remove_count,15673},{213,AmountRole::create_count,15678}}};
inline constexpr std::array<Effect,8> r20289_success{{
  {EffectKind::remove,184,107,16706,1,Actor::self},
  {EffectKind::remove,184,127,16704,2,Actor::self},
  {EffectKind::remove,184,147,16143,1,Actor::self},
  {EffectKind::remove,184,167,15673,1,Actor::self},
  {EffectKind::print,12,177,-1,0,Actor::hero},
  {EffectKind::say,14646,197,-1,0,Actor::hero},
  {EffectKind::create,170,217,15678,1,Actor::self},
  {EffectKind::end_production,19965,277,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20289_failure{{
  {EffectKind::print,12,232,-1,0,Actor::hero},
  {EffectKind::say,14646,252,-1,0,Actor::hero},
  {EffectKind::create,170,272,16604,1,Actor::self},
  {EffectKind::end_production,19965,277,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,5> r20289_effects{{{MaterialAction::remove,16706,1,0},{MaterialAction::remove,16704,2,0},{MaterialAction::remove,16143,1,0},{MaterialAction::remove,15673,1,0},{MaterialAction::create,15678,1,0}}};
inline constexpr std::array<FixedEffect,1> r20289_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20289_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,6> r20289_constructors{{function_for(16706),function_for(16704),function_for(16143),function_for(15673),function_for(16604),function_for(15678)}};
inline constexpr std::array<Requirement,3> r20292_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::context,20280,0,1},{RequirementKind::integer_equal,RequirementRole::knowledge,11326,0,1}}};
inline constexpr std::array<SymbolBinding,10> r20292_bindings{{
  {"ITPL_SWAMPHERB",16706,7,1,0,1584,false,0,false,0},
  {"ITPL_BEET",16704,7,1,0,1584,false,0,false,0},
  {"ITAT_SHARKTEETH",16143,7,1,0,1584,false,0,false,0},
  {"ITFO_ADDON_RUM",15673,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITFO_ADDON_SCHLAFHAMMER",15680,7,1,0,1584,false,0,false,0},
  {"PC_BOOZE_SCHLAF",20290,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"BOOZESTART",20280,2,0,1,-1,false,0,false,0},
  {"KNOWS_SCHLAFHAMMER",11326,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,6> r20292_targets{{{"ITPL_SWAMPHERB",16706},{"ITPL_BEET",16704},{"ITAT_SHARKTEETH",16143},{"ITFO_ADDON_RUM",15673},{"ITMI_FLASK",16604},{"ITFO_ADDON_SCHLAFHAMMER",15680}}};
inline constexpr std::array<ItemAmount,5> r20292_costs{{{16706,1},{16704,2},{16143,1},{15673,2},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20292_outputs{{{15680,1}}};
inline constexpr std::array<AmountOperand,9> r20292_operands{{{1,2,2,0,false},{22,1,1,0,false},{43,2,2,0,false},{64,1,1,0,false},{103,1,1,0,false},{123,2,2,0,false},{143,1,1,0,false},{163,2,2,0,false},{213,1,1,0,false}}};
inline constexpr std::array<OperandUse,9> r20292_roles{{{1,AmountRole::minimum_inventory,15673},{22,AmountRole::minimum_inventory,16143},{43,AmountRole::minimum_inventory,16704},{64,AmountRole::minimum_inventory,16706},{103,AmountRole::remove_count,16706},{123,AmountRole::remove_count,16704},{143,AmountRole::remove_count,16143},{163,AmountRole::remove_count,15673},{213,AmountRole::create_count,15680}}};
inline constexpr std::array<Effect,8> r20292_success{{
  {EffectKind::remove,184,107,16706,1,Actor::self},
  {EffectKind::remove,184,127,16704,2,Actor::self},
  {EffectKind::remove,184,147,16143,1,Actor::self},
  {EffectKind::remove,184,167,15673,2,Actor::self},
  {EffectKind::print,12,177,-1,0,Actor::hero},
  {EffectKind::say,14646,197,-1,0,Actor::hero},
  {EffectKind::create,170,217,15680,1,Actor::self},
  {EffectKind::end_production,19965,277,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20292_failure{{
  {EffectKind::print,12,232,-1,0,Actor::hero},
  {EffectKind::say,14646,252,-1,0,Actor::hero},
  {EffectKind::create,170,272,16604,1,Actor::self},
  {EffectKind::end_production,19965,277,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,5> r20292_effects{{{MaterialAction::remove,16706,1,0},{MaterialAction::remove,16704,2,0},{MaterialAction::remove,16143,1,0},{MaterialAction::remove,15673,2,0},{MaterialAction::create,15680,1,0}}};
inline constexpr std::array<FixedEffect,1> r20292_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20292_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,6> r20292_constructors{{function_for(16706),function_for(16704),function_for(16143),function_for(15673),function_for(16604),function_for(15680)}};
inline constexpr std::array<Requirement,3> r20295_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::context,20280,0,1},{RequirementKind::integer_equal,RequirementRole::knowledge,11327,0,1}}};
inline constexpr std::array<SymbolBinding,9> r20295_bindings{{
  {"ITPL_SPEED_HERB_01",16724,7,1,0,1584,false,0,false,0},
  {"ITFO_ADDON_RUM",15673,7,1,0,1584,false,0,false,0},
  {"ITFO_FISH",16375,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITFO_ADDON_SCHNELLERHERING",15736,7,1,0,1584,false,0,false,0},
  {"PC_BOOZE_SCHNELLERHERING",20293,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"BOOZESTART",20280,2,0,1,-1,false,0,false,0},
  {"KNOWS_SCHNELLERHERING",11327,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,5> r20295_targets{{{"ITPL_SPEED_HERB_01",16724},{"ITFO_ADDON_RUM",15673},{"ITFO_FISH",16375},{"ITMI_FLASK",16604},{"ITFO_ADDON_SCHNELLERHERING",15736}}};
inline constexpr std::array<ItemAmount,4> r20295_costs{{{16724,1},{15673,1},{16375,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20295_outputs{{{15736,1}}};
inline constexpr std::array<AmountOperand,7> r20295_operands{{{1,1,1,0,false},{22,1,1,0,false},{43,1,1,0,false},{81,1,1,0,false},{101,1,1,0,false},{121,1,1,0,false},{171,1,1,0,false}}};
inline constexpr std::array<OperandUse,7> r20295_roles{{{1,AmountRole::minimum_inventory,15673},{22,AmountRole::minimum_inventory,16375},{43,AmountRole::minimum_inventory,16724},{81,AmountRole::remove_count,16724},{101,AmountRole::remove_count,15673},{121,AmountRole::remove_count,16375},{171,AmountRole::create_count,15736}}};
inline constexpr std::array<Effect,7> r20295_success{{
  {EffectKind::remove,184,85,16724,1,Actor::self},
  {EffectKind::remove,184,105,15673,1,Actor::self},
  {EffectKind::remove,184,125,16375,1,Actor::self},
  {EffectKind::print,12,135,-1,0,Actor::hero},
  {EffectKind::say,14646,155,-1,0,Actor::hero},
  {EffectKind::create,170,175,15736,1,Actor::self},
  {EffectKind::end_production,19965,235,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20295_failure{{
  {EffectKind::print,12,190,-1,0,Actor::hero},
  {EffectKind::say,14646,210,-1,0,Actor::hero},
  {EffectKind::create,170,230,16604,1,Actor::self},
  {EffectKind::end_production,19965,235,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,4> r20295_effects{{{MaterialAction::remove,16724,1,0},{MaterialAction::remove,15673,1,0},{MaterialAction::remove,16375,1,0},{MaterialAction::create,15736,1,0}}};
inline constexpr std::array<FixedEffect,1> r20295_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20295_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,5> r20295_constructors{{function_for(16724),function_for(15673),function_for(16375),function_for(16604),function_for(15736)}};
inline constexpr std::array<Requirement,3> r20329_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::knowledge,1464,13,1},{RequirementKind::integer_equal,RequirementRole::context,20299,0,1}}};
inline constexpr std::array<SymbolBinding,8> r20329_bindings{{
  {"ITPO_MANA_01",16766,7,1,0,1584,false,0,false,0},
  {"ITPL_TEMP_HERB",16736,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITPO_MANA_ADDON_04",15745,7,1,0,1584,false,0,false,0},
  {"PC_ITPO_MANA_04",20327,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"PLAYER_TALENT_ALCHEMY",1464,2,0,15,-1,false,0,false,0},
  {"MANASTART",20299,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,4> r20329_targets{{{"ITPO_MANA_01",16766},{"ITPL_TEMP_HERB",16736},{"ITMI_FLASK",16604},{"ITPO_MANA_ADDON_04",15745}}};
inline constexpr std::array<ItemAmount,3> r20329_costs{{{16766,3},{16736,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20329_outputs{{{15745,1}}};
inline constexpr std::array<AmountOperand,5> r20329_operands{{{1,1,1,0,false},{22,3,3,0,false},{59,3,3,0,false},{79,1,1,0,false},{99,1,1,0,false}}};
inline constexpr std::array<OperandUse,5> r20329_roles{{{1,AmountRole::minimum_inventory,16736},{22,AmountRole::minimum_inventory,16766},{59,AmountRole::remove_count,16766},{79,AmountRole::remove_count,16736},{99,AmountRole::create_count,15745}}};
inline constexpr std::array<Effect,6> r20329_success{{
  {EffectKind::remove,184,63,16766,3,Actor::hero},
  {EffectKind::remove,184,83,16736,1,Actor::hero},
  {EffectKind::create,170,103,15745,1,Actor::hero},
  {EffectKind::print,12,113,-1,0,Actor::hero},
  {EffectKind::say,14646,133,-1,0,Actor::hero},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20329_failure{{
  {EffectKind::print,12,148,-1,0,Actor::hero},
  {EffectKind::say,14646,168,-1,0,Actor::hero},
  {EffectKind::create,170,188,16604,1,Actor::self},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,3> r20329_effects{{{MaterialAction::remove,16766,3,0},{MaterialAction::remove,16736,1,0},{MaterialAction::create,15745,1,0}}};
inline constexpr std::array<FixedEffect,1> r20329_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20329_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,4> r20329_constructors{{function_for(16766),function_for(16736),function_for(16604),function_for(15745)}};
inline constexpr std::array<Requirement,3> r20360_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::knowledge,1464,14,1},{RequirementKind::integer_equal,RequirementRole::context,20330,0,1}}};
inline constexpr std::array<SymbolBinding,8> r20360_bindings{{
  {"ITPO_HEALTH_01",16775,7,1,0,1584,false,0,false,0},
  {"ITPL_TEMP_HERB",16736,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITPO_HEALTH_ADDON_04",15743,7,1,0,1584,false,0,false,0},
  {"PC_ITPO_HEALTH_04",20358,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"PLAYER_TALENT_ALCHEMY",1464,2,0,15,-1,false,0,false,0},
  {"HEALTHSTART",20330,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,4> r20360_targets{{{"ITPO_HEALTH_01",16775},{"ITPL_TEMP_HERB",16736},{"ITMI_FLASK",16604},{"ITPO_HEALTH_ADDON_04",15743}}};
inline constexpr std::array<ItemAmount,3> r20360_costs{{{16775,3},{16736,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20360_outputs{{{15743,1}}};
inline constexpr std::array<AmountOperand,5> r20360_operands{{{1,1,1,0,false},{22,3,3,0,false},{59,3,3,0,false},{79,1,1,0,false},{99,1,1,0,false}}};
inline constexpr std::array<OperandUse,5> r20360_roles{{{1,AmountRole::minimum_inventory,16736},{22,AmountRole::minimum_inventory,16775},{59,AmountRole::remove_count,16775},{79,AmountRole::remove_count,16736},{99,AmountRole::create_count,15743}}};
inline constexpr std::array<Effect,6> r20360_success{{
  {EffectKind::remove,184,63,16775,3,Actor::hero},
  {EffectKind::remove,184,83,16736,1,Actor::hero},
  {EffectKind::create,170,103,15743,1,Actor::hero},
  {EffectKind::print,12,113,-1,0,Actor::hero},
  {EffectKind::say,14646,133,-1,0,Actor::hero},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20360_failure{{
  {EffectKind::print,12,148,-1,0,Actor::hero},
  {EffectKind::say,14646,168,-1,0,Actor::hero},
  {EffectKind::create,170,188,16604,1,Actor::self},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,3> r20360_effects{{{MaterialAction::remove,16775,3,0},{MaterialAction::remove,16736,1,0},{MaterialAction::create,15743,1,0}}};
inline constexpr std::array<FixedEffect,1> r20360_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20360_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,4> r20360_constructors{{function_for(16775),function_for(16736),function_for(16604),function_for(15743)}};
inline constexpr std::array<Requirement,3> r20373_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::knowledge,1464,10,1},{RequirementKind::integer_equal,RequirementRole::context,20361,0,1}}};
inline constexpr std::array<SymbolBinding,8> r20373_bindings{{
  {"ITPL_HEALTH_HERB_03",16718,7,1,0,1584,false,0,false,0},
  {"ITPL_PERM_HERB",16738,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITPO_PERM_HEALTH",16790,7,1,0,1584,false,0,false,0},
  {"PC_ITPO_PERM_HEALTH",20371,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"PLAYER_TALENT_ALCHEMY",1464,2,0,15,-1,false,0,false,0},
  {"SPECIALSTART",20361,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,4> r20373_targets{{{"ITPL_HEALTH_HERB_03",16718},{"ITPL_PERM_HERB",16738},{"ITMI_FLASK",16604},{"ITPO_PERM_HEALTH",16790}}};
inline constexpr std::array<ItemAmount,3> r20373_costs{{{16718,1},{16738,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20373_outputs{{{16790,1}}};
inline constexpr std::array<AmountOperand,5> r20373_operands{{{1,1,1,0,false},{22,1,1,0,false},{59,1,1,0,false},{79,1,1,0,false},{99,1,1,0,false}}};
inline constexpr std::array<OperandUse,5> r20373_roles{{{1,AmountRole::minimum_inventory,16738},{22,AmountRole::minimum_inventory,16718},{59,AmountRole::remove_count,16718},{79,AmountRole::remove_count,16738},{99,AmountRole::create_count,16790}}};
inline constexpr std::array<Effect,6> r20373_success{{
  {EffectKind::remove,184,63,16718,1,Actor::hero},
  {EffectKind::remove,184,83,16738,1,Actor::hero},
  {EffectKind::create,170,103,16790,1,Actor::hero},
  {EffectKind::print,12,113,-1,0,Actor::hero},
  {EffectKind::say,14646,133,-1,0,Actor::hero},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20373_failure{{
  {EffectKind::print,12,148,-1,0,Actor::hero},
  {EffectKind::say,14646,168,-1,0,Actor::hero},
  {EffectKind::create,170,188,16604,1,Actor::self},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,3> r20373_effects{{{MaterialAction::remove,16718,1,0},{MaterialAction::remove,16738,1,0},{MaterialAction::create,16790,1,0}}};
inline constexpr std::array<FixedEffect,1> r20373_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20373_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,4> r20373_constructors{{function_for(16718),function_for(16738),function_for(16604),function_for(16790)}};
inline constexpr std::array<Requirement,3> r20376_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::knowledge,1464,9,1},{RequirementKind::integer_equal,RequirementRole::context,20361,0,1}}};
inline constexpr std::array<SymbolBinding,8> r20376_bindings{{
  {"ITPL_MANA_HERB_03",16712,7,1,0,1584,false,0,false,0},
  {"ITPL_PERM_HERB",16738,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITPO_PERM_MANA",16794,7,1,0,1584,false,0,false,0},
  {"PC_ITPO_PERM_MANA",20374,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"PLAYER_TALENT_ALCHEMY",1464,2,0,15,-1,false,0,false,0},
  {"SPECIALSTART",20361,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,4> r20376_targets{{{"ITPL_MANA_HERB_03",16712},{"ITPL_PERM_HERB",16738},{"ITMI_FLASK",16604},{"ITPO_PERM_MANA",16794}}};
inline constexpr std::array<ItemAmount,3> r20376_costs{{{16712,1},{16738,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20376_outputs{{{16794,1}}};
inline constexpr std::array<AmountOperand,5> r20376_operands{{{1,1,1,0,false},{22,1,1,0,false},{59,1,1,0,false},{79,1,1,0,false},{99,1,1,0,false}}};
inline constexpr std::array<OperandUse,5> r20376_roles{{{1,AmountRole::minimum_inventory,16738},{22,AmountRole::minimum_inventory,16712},{59,AmountRole::remove_count,16712},{79,AmountRole::remove_count,16738},{99,AmountRole::create_count,16794}}};
inline constexpr std::array<Effect,6> r20376_success{{
  {EffectKind::remove,184,63,16712,1,Actor::hero},
  {EffectKind::remove,184,83,16738,1,Actor::hero},
  {EffectKind::create,170,103,16794,1,Actor::hero},
  {EffectKind::print,12,113,-1,0,Actor::hero},
  {EffectKind::say,14646,133,-1,0,Actor::hero},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20376_failure{{
  {EffectKind::print,12,148,-1,0,Actor::hero},
  {EffectKind::say,14646,168,-1,0,Actor::hero},
  {EffectKind::create,170,188,16604,1,Actor::self},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,3> r20376_effects{{{MaterialAction::remove,16712,1,0},{MaterialAction::remove,16738,1,0},{MaterialAction::create,16794,1,0}}};
inline constexpr std::array<FixedEffect,1> r20376_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20376_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,4> r20376_constructors{{function_for(16712),function_for(16738),function_for(16604),function_for(16794)}};
inline constexpr std::array<Requirement,3> r20379_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::knowledge,1464,8,1},{RequirementKind::integer_equal,RequirementRole::context,20361,0,1}}};
inline constexpr std::array<SymbolBinding,8> r20379_bindings{{
  {"ITPL_DEX_HERB_01",16720,7,1,0,1584,false,0,false,0},
  {"ITPL_PERM_HERB",16738,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITPO_PERM_DEX",16787,7,1,0,1584,false,0,false,0},
  {"PC_ITPO_DEX",20377,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"PLAYER_TALENT_ALCHEMY",1464,2,0,15,-1,false,0,false,0},
  {"SPECIALSTART",20361,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,4> r20379_targets{{{"ITPL_DEX_HERB_01",16720},{"ITPL_PERM_HERB",16738},{"ITMI_FLASK",16604},{"ITPO_PERM_DEX",16787}}};
inline constexpr std::array<ItemAmount,3> r20379_costs{{{16720,1},{16738,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20379_outputs{{{16787,1}}};
inline constexpr std::array<AmountOperand,5> r20379_operands{{{1,1,1,0,false},{22,1,1,0,false},{59,1,1,0,false},{79,1,1,0,false},{99,1,1,0,false}}};
inline constexpr std::array<OperandUse,5> r20379_roles{{{1,AmountRole::minimum_inventory,16738},{22,AmountRole::minimum_inventory,16720},{59,AmountRole::remove_count,16720},{79,AmountRole::remove_count,16738},{99,AmountRole::create_count,16787}}};
inline constexpr std::array<Effect,6> r20379_success{{
  {EffectKind::remove,184,63,16720,1,Actor::hero},
  {EffectKind::remove,184,83,16738,1,Actor::hero},
  {EffectKind::create,170,103,16787,1,Actor::hero},
  {EffectKind::print,12,113,-1,0,Actor::hero},
  {EffectKind::say,14646,133,-1,0,Actor::hero},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20379_failure{{
  {EffectKind::print,12,148,-1,0,Actor::hero},
  {EffectKind::say,14646,168,-1,0,Actor::hero},
  {EffectKind::create,170,188,16604,1,Actor::self},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,3> r20379_effects{{{MaterialAction::remove,16720,1,0},{MaterialAction::remove,16738,1,0},{MaterialAction::create,16787,1,0}}};
inline constexpr std::array<FixedEffect,1> r20379_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20379_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,4> r20379_constructors{{function_for(16720),function_for(16738),function_for(16604),function_for(16787)}};
inline constexpr std::array<Requirement,3> r20382_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::knowledge,1464,7,1},{RequirementKind::integer_equal,RequirementRole::context,20361,0,1}}};
inline constexpr std::array<SymbolBinding,8> r20382_bindings{{
  {"ITPL_STRENGTH_HERB_01",16722,7,1,0,1584,false,0,false,0},
  {"ITPL_PERM_HERB",16738,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITPO_PERM_STR",16784,7,1,0,1584,false,0,false,0},
  {"PC_ITPO_STRG",20380,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"PLAYER_TALENT_ALCHEMY",1464,2,0,15,-1,false,0,false,0},
  {"SPECIALSTART",20361,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,4> r20382_targets{{{"ITPL_STRENGTH_HERB_01",16722},{"ITPL_PERM_HERB",16738},{"ITMI_FLASK",16604},{"ITPO_PERM_STR",16784}}};
inline constexpr std::array<ItemAmount,3> r20382_costs{{{16722,1},{16738,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20382_outputs{{{16784,1}}};
inline constexpr std::array<AmountOperand,5> r20382_operands{{{1,1,1,0,false},{22,1,1,0,false},{59,1,1,0,false},{79,1,1,0,false},{99,1,1,0,false}}};
inline constexpr std::array<OperandUse,5> r20382_roles{{{1,AmountRole::minimum_inventory,16738},{22,AmountRole::minimum_inventory,16722},{59,AmountRole::remove_count,16722},{79,AmountRole::remove_count,16738},{99,AmountRole::create_count,16784}}};
inline constexpr std::array<Effect,6> r20382_success{{
  {EffectKind::remove,184,63,16722,1,Actor::hero},
  {EffectKind::remove,184,83,16738,1,Actor::hero},
  {EffectKind::create,170,103,16784,1,Actor::hero},
  {EffectKind::print,12,113,-1,0,Actor::hero},
  {EffectKind::say,14646,133,-1,0,Actor::hero},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20382_failure{{
  {EffectKind::print,12,148,-1,0,Actor::hero},
  {EffectKind::say,14646,168,-1,0,Actor::hero},
  {EffectKind::create,170,188,16604,1,Actor::self},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,3> r20382_effects{{{MaterialAction::remove,16722,1,0},{MaterialAction::remove,16738,1,0},{MaterialAction::create,16784,1,0}}};
inline constexpr std::array<FixedEffect,1> r20382_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20382_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,4> r20382_constructors{{function_for(16722),function_for(16738),function_for(16604),function_for(16784)}};
inline constexpr std::array<Requirement,3> r20385_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::knowledge,1464,6,1},{RequirementKind::integer_equal,RequirementRole::context,20361,0,1}}};
inline constexpr std::array<SymbolBinding,8> r20385_bindings{{
  {"ITPL_SPEED_HERB_01",16724,7,1,0,1584,false,0,false,0},
  {"ITPL_TEMP_HERB",16736,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITPO_SPEED",16798,7,1,0,1584,false,0,false,0},
  {"PC_ITPO_SPEED",20383,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"PLAYER_TALENT_ALCHEMY",1464,2,0,15,-1,false,0,false,0},
  {"SPECIALSTART",20361,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,4> r20385_targets{{{"ITPL_SPEED_HERB_01",16724},{"ITPL_TEMP_HERB",16736},{"ITMI_FLASK",16604},{"ITPO_SPEED",16798}}};
inline constexpr std::array<ItemAmount,3> r20385_costs{{{16724,1},{16736,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20385_outputs{{{16798,1}}};
inline constexpr std::array<AmountOperand,5> r20385_operands{{{1,1,1,0,false},{22,1,1,0,false},{59,1,1,0,false},{79,1,1,0,false},{99,1,1,0,false}}};
inline constexpr std::array<OperandUse,5> r20385_roles{{{1,AmountRole::minimum_inventory,16736},{22,AmountRole::minimum_inventory,16724},{59,AmountRole::remove_count,16724},{79,AmountRole::remove_count,16736},{99,AmountRole::create_count,16798}}};
inline constexpr std::array<Effect,6> r20385_success{{
  {EffectKind::remove,184,63,16724,1,Actor::hero},
  {EffectKind::remove,184,83,16736,1,Actor::hero},
  {EffectKind::create,170,103,16798,1,Actor::hero},
  {EffectKind::print,12,113,-1,0,Actor::hero},
  {EffectKind::say,14646,133,-1,0,Actor::hero},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20385_failure{{
  {EffectKind::print,12,148,-1,0,Actor::hero},
  {EffectKind::say,14646,168,-1,0,Actor::hero},
  {EffectKind::create,170,188,16604,1,Actor::self},
  {EffectKind::end_production,19965,193,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,3> r20385_effects{{{MaterialAction::remove,16724,1,0},{MaterialAction::remove,16736,1,0},{MaterialAction::create,16798,1,0}}};
inline constexpr std::array<FixedEffect,1> r20385_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20385_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,4> r20385_constructors{{function_for(16724),function_for(16736),function_for(16604),function_for(16798)}};
inline constexpr std::array<Requirement,2> r20395_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::context,20386,0,1}}};
inline constexpr std::array<SymbolBinding,6> r20395_bindings{{
  {"ITPL_SWAMPHERB",16706,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITMI_JOINT",16620,7,1,0,1584,false,0,false,0},
  {"PC_ITMI_JOINT",20393,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"TABAKSTART",20386,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,3> r20395_targets{{{"ITPL_SWAMPHERB",16706},{"ITMI_FLASK",16604},{"ITMI_JOINT",16620}}};
inline constexpr std::array<ItemAmount,2> r20395_costs{{{16706,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20395_outputs{{{16620,1}}};
inline constexpr std::array<AmountOperand,3> r20395_operands{{{1,1,1,0,false},{37,1,1,0,false},{57,1,1,0,false}}};
inline constexpr std::array<OperandUse,3> r20395_roles{{{1,AmountRole::minimum_inventory,16706},{37,AmountRole::remove_count,16706},{57,AmountRole::create_count,16620}}};
inline constexpr std::array<Effect,4> r20395_success{{
  {EffectKind::remove,184,41,16706,1,Actor::hero},
  {EffectKind::create,170,61,16620,1,Actor::hero},
  {EffectKind::print,12,71,-1,0,Actor::hero},
  {EffectKind::end_production,19965,131,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20395_failure{{
  {EffectKind::print,12,86,-1,0,Actor::hero},
  {EffectKind::say,14646,106,-1,0,Actor::hero},
  {EffectKind::create,170,126,16604,1,Actor::self},
  {EffectKind::end_production,19965,131,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,2> r20395_effects{{{MaterialAction::remove,16706,1,0},{MaterialAction::create,16620,1,0}}};
inline constexpr std::array<FixedEffect,1> r20395_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20395_prints{{{14376,29,digest_literal("cae39a66ed1db2d6e661b8768c9e8d40bfd9236dbb55765e50156b482bd09bf1")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,3> r20395_constructors{{function_for(16706),function_for(16604),function_for(16620)}};
inline constexpr std::array<Requirement,3> r20398_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::context,20386,0,1},{RequirementKind::integer_equal,RequirementRole::knowledge,11211,0,1}}};
inline constexpr std::array<SymbolBinding,8> r20398_bindings{{
  {"ITPL_SWAMPHERB",16706,7,1,0,1584,false,0,false,0},
  {"ITPL_TEMP_HERB",16736,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITMI_ADDON_JOINT_01",15704,7,1,0,1584,false,0,false,0},
  {"PC_ITMI_ADDON_JOINT_01",20396,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"TABAKSTART",20386,2,0,1,-1,false,0,false,0},
  {"GREEN_EXTREM",11211,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,4> r20398_targets{{{"ITPL_SWAMPHERB",16706},{"ITPL_TEMP_HERB",16736},{"ITMI_FLASK",16604},{"ITMI_ADDON_JOINT_01",15704}}};
inline constexpr std::array<ItemAmount,3> r20398_costs{{{16706,2},{16736,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20398_outputs{{{15704,1}}};
inline constexpr std::array<AmountOperand,5> r20398_operands{{{1,1,1,0,false},{22,2,2,0,false},{59,2,2,0,false},{79,1,1,0,false},{99,1,1,0,false}}};
inline constexpr std::array<OperandUse,5> r20398_roles{{{1,AmountRole::minimum_inventory,16736},{22,AmountRole::minimum_inventory,16706},{59,AmountRole::remove_count,16706},{79,AmountRole::remove_count,16736},{99,AmountRole::create_count,15704}}};
inline constexpr std::array<Effect,5> r20398_success{{
  {EffectKind::remove,184,63,16706,2,Actor::hero},
  {EffectKind::remove,184,83,16736,1,Actor::hero},
  {EffectKind::create,170,103,15704,1,Actor::hero},
  {EffectKind::print,12,113,-1,0,Actor::hero},
  {EffectKind::end_production,19965,173,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20398_failure{{
  {EffectKind::print,12,128,-1,0,Actor::hero},
  {EffectKind::say,14646,148,-1,0,Actor::hero},
  {EffectKind::create,170,168,16604,1,Actor::self},
  {EffectKind::end_production,19965,173,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,3> r20398_effects{{{MaterialAction::remove,16706,2,0},{MaterialAction::remove,16736,1,0},{MaterialAction::create,15704,1,0}}};
inline constexpr std::array<FixedEffect,1> r20398_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20398_prints{{{14377,22,digest_literal("472db713cf0c2fb20d161c828ef81bf9893050f013d6659a20cf52a455a75eb1")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,4> r20398_constructors{{function_for(16706),function_for(16736),function_for(16604),function_for(15704)}};
inline constexpr std::array<Requirement,7> r20408_requirements{{{RequirementKind::integer_equal,RequirementRole::context,12330,0,4},{RequirementKind::integer_equal,RequirementRole::knowledge,1464,11,1},{RequirementKind::integer_equal,RequirementRole::context,20280,0,0},{RequirementKind::integer_equal,RequirementRole::context,20386,0,0},{RequirementKind::integer_equal,RequirementRole::context,20330,0,0},{RequirementKind::integer_equal,RequirementRole::context,20299,0,0},{RequirementKind::integer_equal,RequirementRole::context,20361,0,0}}};
inline constexpr std::array<SymbolBinding,13> r20408_bindings{{
  {"ITAT_DRAGONEGG_MIS",17585,7,1,0,1584,false,0,false,0},
  {"ITMI_DARKPEARL",16659,7,1,0,1584,false,0,false,0},
  {"ITMI_SULFUR",16652,7,1,0,1584,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"ITPO_MEGADRINK",16800,7,1,0,1584,false,0,false,0},
  {"PC_ITPO_MEGADRINK",20406,7,1,0,1649,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"PLAYER_TALENT_ALCHEMY",1464,2,0,15,-1,false,0,false,0},
  {"BOOZESTART",20280,2,0,1,-1,false,0,false,0},
  {"TABAKSTART",20386,2,0,1,-1,false,0,false,0},
  {"HEALTHSTART",20330,2,0,1,-1,false,0,false,0},
  {"MANASTART",20299,2,0,1,-1,false,0,false,0},
  {"SPECIALSTART",20361,2,0,1,-1,false,0,false,0},
}};
inline constexpr std::array<PackedInventoryTarget,5> r20408_targets{{{"ITAT_DRAGONEGG_MIS",17585},{"ITMI_DARKPEARL",16659},{"ITMI_SULFUR",16652},{"ITMI_FLASK",16604},{"ITPO_MEGADRINK",16800}}};
inline constexpr std::array<ItemAmount,4> r20408_costs{{{17585,10},{16659,1},{16652,1},{16604,1}}};
inline constexpr std::array<ItemAmount,1> r20408_outputs{{{16800,1}}};
inline constexpr std::array<AmountOperand,7> r20408_operands{{{1,1,1,0,false},{22,1,1,0,false},{43,10,10,0,false},{81,10,10,0,false},{101,1,1,0,false},{121,1,1,0,false},{141,1,1,0,false}}};
inline constexpr std::array<OperandUse,7> r20408_roles{{{1,AmountRole::minimum_inventory,16652},{22,AmountRole::minimum_inventory,16659},{43,AmountRole::minimum_inventory,17585},{81,AmountRole::remove_count,17585},{101,AmountRole::remove_count,16659},{121,AmountRole::remove_count,16652},{141,AmountRole::create_count,16800}}};
inline constexpr std::array<Effect,7> r20408_success{{
  {EffectKind::remove,184,85,17585,10,Actor::hero},
  {EffectKind::remove,184,105,16659,1,Actor::hero},
  {EffectKind::remove,184,125,16652,1,Actor::hero},
  {EffectKind::create,170,145,16800,1,Actor::hero},
  {EffectKind::print,12,155,-1,0,Actor::hero},
  {EffectKind::say,14646,175,-1,0,Actor::hero},
  {EffectKind::end_production,19965,235,-1,0,Actor::hero},
}};
inline constexpr std::array<Effect,4> r20408_failure{{
  {EffectKind::print,12,190,-1,0,Actor::hero},
  {EffectKind::say,14646,210,-1,0,Actor::hero},
  {EffectKind::create,170,230,16604,1,Actor::self},
  {EffectKind::end_production,19965,235,-1,0,Actor::hero},
}};
inline constexpr std::array<MaterialEffect,4> r20408_effects{{{MaterialAction::remove,17585,10,0},{MaterialAction::remove,16659,1,0},{MaterialAction::remove,16652,1,0},{MaterialAction::create,16800,1,0}}};
inline constexpr std::array<FixedEffect,1> r20408_refund{{{MaterialAction::create,16604,1,Actor::self}}};
inline constexpr std::array<PrintFingerprint,2> r20408_prints{{{14372,18,digest_literal("5840f533d99f4a9a7a2f09718f408b07fa350743e1db4b464ba20e4bc8fbe5ec")},{14374,18,digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")}}};
inline constexpr std::array<const Function*,5> r20408_constructors{{function_for(17585),function_for(16659),function_for(16652),function_for(16604),function_for(16800)}};
inline constexpr std::array<Descriptor,13> recipes{{
  {RecipeId::lou_spirit,"Lous Schnaps",Category::booze,recipe_catalog::Station::alchemy,4,
    view(r20289_targets),view(r20289_bindings),view(r20289_requirements),
    {20287,5,22243,false,false,true,function_for(20287),function_for(20288),function_for(20289),view(r20289_requirements)},
    {function_for(20289),20287,Invocation::info,1,1,1073741823,{function_for(20289)->byte_length,function_for(20289)->body_sha256,r20289_operands.data(),r20289_operands.size()},
      view(r20289_effects),view(r20289_refund),true,true,false,true},
    view(r20289_costs),view(r20289_outputs),Actor::self,lab_hand,view(r20289_roles),view(r20289_success),view(r20289_failure),view(r20289_prints),view(r20289_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::sleep_spirit,"Schlafschnaps",Category::booze,recipe_catalog::Station::alchemy,4,
    view(r20292_targets),view(r20292_bindings),view(r20292_requirements),
    {20290,6,22243,false,false,true,function_for(20290),function_for(20291),function_for(20292),view(r20292_requirements)},
    {function_for(20292),20290,Invocation::info,1,1,1073741823,{function_for(20292)->byte_length,function_for(20292)->body_sha256,r20292_operands.data(),r20292_operands.size()},
      view(r20292_effects),view(r20292_refund),true,true,false,true},
    view(r20292_costs),view(r20292_outputs),Actor::self,lab_hand,view(r20292_roles),view(r20292_success),view(r20292_failure),view(r20292_prints),view(r20292_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::fish_spirit,"Fischschnaps",Category::booze,recipe_catalog::Station::alchemy,4,
    view(r20295_targets),view(r20295_bindings),view(r20295_requirements),
    {20293,5,22243,false,false,true,function_for(20293),function_for(20294),function_for(20295),view(r20295_requirements)},
    {function_for(20295),20293,Invocation::info,1,1,2147483647,{function_for(20295)->byte_length,function_for(20295)->body_sha256,r20295_operands.data(),r20295_operands.size()},
      view(r20295_effects),view(r20295_refund),true,true,false,true},
    view(r20295_costs),view(r20295_outputs),Actor::self,lab_hand,view(r20295_roles),view(r20295_success),view(r20295_failure),view(r20295_prints),view(r20295_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::combined_mana,"Zusammengesetzter Manatrank",Category::mana,recipe_catalog::Station::alchemy,4,
    view(r20329_targets),view(r20329_bindings),view(r20329_requirements),
    {20327,4,22243,false,false,true,function_for(20327),function_for(20328),function_for(20329),view(r20329_requirements)},
    {function_for(20329),20327,Invocation::info,1,1,715827882,{function_for(20329)->byte_length,function_for(20329)->body_sha256,r20329_operands.data(),r20329_operands.size()},
      view(r20329_effects),view(r20329_refund),true,true,false,true},
    view(r20329_costs),view(r20329_outputs),Actor::hero,lab_hand,view(r20329_roles),view(r20329_success),view(r20329_failure),view(r20329_prints),view(r20329_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::combined_health,"Zusammengesetzter Heiltrank",Category::health,recipe_catalog::Station::alchemy,4,
    view(r20360_targets),view(r20360_bindings),view(r20360_requirements),
    {20358,4,22243,false,false,true,function_for(20358),function_for(20359),function_for(20360),view(r20360_requirements)},
    {function_for(20360),20358,Invocation::info,1,1,715827882,{function_for(20360)->byte_length,function_for(20360)->body_sha256,r20360_operands.data(),r20360_operands.size()},
      view(r20360_effects),view(r20360_refund),true,true,false,true},
    view(r20360_costs),view(r20360_outputs),Actor::hero,lab_hand,view(r20360_roles),view(r20360_success),view(r20360_failure),view(r20360_prints),view(r20360_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::permanent_health,"Permanente Gesundheit",Category::special,recipe_catalog::Station::alchemy,4,
    view(r20373_targets),view(r20373_bindings),view(r20373_requirements),
    {20371,3,22243,false,false,true,function_for(20371),function_for(20372),function_for(20373),view(r20373_requirements)},
    {function_for(20373),20371,Invocation::info,1,1,2147483647,{function_for(20373)->byte_length,function_for(20373)->body_sha256,r20373_operands.data(),r20373_operands.size()},
      view(r20373_effects),view(r20373_refund),true,true,false,true},
    view(r20373_costs),view(r20373_outputs),Actor::hero,lab_hand,view(r20373_roles),view(r20373_success),view(r20373_failure),view(r20373_prints),view(r20373_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::permanent_mana,"Permanentes Mana",Category::special,recipe_catalog::Station::alchemy,4,
    view(r20376_targets),view(r20376_bindings),view(r20376_requirements),
    {20374,3,22243,false,false,true,function_for(20374),function_for(20375),function_for(20376),view(r20376_requirements)},
    {function_for(20376),20374,Invocation::info,1,1,2147483647,{function_for(20376)->byte_length,function_for(20376)->body_sha256,r20376_operands.data(),r20376_operands.size()},
      view(r20376_effects),view(r20376_refund),true,true,false,true},
    view(r20376_costs),view(r20376_outputs),Actor::hero,lab_hand,view(r20376_roles),view(r20376_success),view(r20376_failure),view(r20376_prints),view(r20376_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::permanent_dexterity,"Permanente Geschicklichkeit",Category::special,recipe_catalog::Station::alchemy,4,
    view(r20379_targets),view(r20379_bindings),view(r20379_requirements),
    {20377,3,22243,false,false,true,function_for(20377),function_for(20378),function_for(20379),view(r20379_requirements)},
    {function_for(20379),20377,Invocation::info,1,1,2147483647,{function_for(20379)->byte_length,function_for(20379)->body_sha256,r20379_operands.data(),r20379_operands.size()},
      view(r20379_effects),view(r20379_refund),true,true,false,true},
    view(r20379_costs),view(r20379_outputs),Actor::hero,lab_hand,view(r20379_roles),view(r20379_success),view(r20379_failure),view(r20379_prints),view(r20379_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::permanent_strength,"Permanente Staerke",Category::special,recipe_catalog::Station::alchemy,4,
    view(r20382_targets),view(r20382_bindings),view(r20382_requirements),
    {20380,3,22243,false,false,true,function_for(20380),function_for(20381),function_for(20382),view(r20382_requirements)},
    {function_for(20382),20380,Invocation::info,1,1,2147483647,{function_for(20382)->byte_length,function_for(20382)->body_sha256,r20382_operands.data(),r20382_operands.size()},
      view(r20382_effects),view(r20382_refund),true,true,false,true},
    view(r20382_costs),view(r20382_outputs),Actor::hero,lab_hand,view(r20382_roles),view(r20382_success),view(r20382_failure),view(r20382_prints),view(r20382_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::speed,"Geschwindigkeitstrank",Category::special,recipe_catalog::Station::alchemy,4,
    view(r20385_targets),view(r20385_bindings),view(r20385_requirements),
    {20383,3,22243,false,false,true,function_for(20383),function_for(20384),function_for(20385),view(r20385_requirements)},
    {function_for(20385),20383,Invocation::info,1,1,2147483647,{function_for(20385)->byte_length,function_for(20385)->body_sha256,r20385_operands.data(),r20385_operands.size()},
      view(r20385_effects),view(r20385_refund),true,true,false,true},
    view(r20385_costs),view(r20385_outputs),Actor::hero,lab_hand,view(r20385_roles),view(r20385_success),view(r20385_failure),view(r20385_prints),view(r20385_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::swamp_roll,"Sumpfkrautrolle",Category::tobacco,recipe_catalog::Station::alchemy,4,
    view(r20395_targets),view(r20395_bindings),view(r20395_requirements),
    {20393,10,22243,false,false,true,function_for(20393),function_for(20394),function_for(20395),view(r20395_requirements)},
    {function_for(20395),20393,Invocation::info,1,1,2147483647,{function_for(20395)->byte_length,function_for(20395)->body_sha256,r20395_operands.data(),r20395_operands.size()},
      view(r20395_effects),view(r20395_refund),true,true,false,true},
    view(r20395_costs),view(r20395_outputs),Actor::hero,lab_hand,view(r20395_roles),view(r20395_success),view(r20395_failure),view(r20395_prints),view(r20395_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::strong_swamp_roll,"Starke Sumpfkrautrolle",Category::tobacco,recipe_catalog::Station::alchemy,4,
    view(r20398_targets),view(r20398_bindings),view(r20398_requirements),
    {20396,11,22243,false,false,true,function_for(20396),function_for(20397),function_for(20398),view(r20398_requirements)},
    {function_for(20398),20396,Invocation::info,1,1,1073741823,{function_for(20398)->byte_length,function_for(20398)->body_sha256,r20398_operands.data(),r20398_operands.size()},
      view(r20398_effects),view(r20398_refund),true,true,false,true},
    view(r20398_costs),view(r20398_outputs),Actor::hero,lab_hand,view(r20398_roles),view(r20398_success),view(r20398_failure),view(r20398_prints),view(r20398_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
  {RecipeId::dragon_egg_potion,"Dracheneiertrank",Category::root,recipe_catalog::Station::alchemy,4,
    view(r20408_targets),view(r20408_bindings),view(r20408_requirements),
    {20406,2,22243,false,false,true,function_for(20406),function_for(20407),function_for(20408),view(r20408_requirements)},
    {function_for(20408),20406,Invocation::info,1,1,214748364,{function_for(20408)->byte_length,function_for(20408)->body_sha256,r20408_operands.data(),r20408_operands.size()},
      view(r20408_effects),view(r20408_refund),true,true,false,true},
    view(r20408_costs),view(r20408_outputs),Actor::hero,lab_hand,view(r20408_roles),view(r20408_success),view(r20408_failure),view(r20408_prints),view(r20408_constructors),
    function_for(20276),function_for(20279),function_for(19965),function_for(14646),flask_debit},
}};
constexpr std::size_t target_capacity() noexcept { std::size_t n=0; for(const auto& r:recipes)if(r.targets.count>n)n=r.targets.count; return n; }
inline constexpr auto catalog_target_capacity=target_capacity();

inline const Descriptor* find(RecipeId id) noexcept {
  for(const auto& recipe:recipes) if(recipe.id==id) return &recipe;
  return nullptr;
}
inline const Descriptor* find_callback(std::uint32_t symbol) noexcept {
  for(const auto& recipe:recipes) if(recipe.callback.function->symbol==symbol) return &recipe;
  return nullptr;
}
inline const SymbolBinding* binding_for(const Descriptor& recipe,std::uint32_t symbol) noexcept {
  for(const auto& b:recipe.bindings) if(b.symbol==symbol) return &b;
  for(const auto& b:common_bindings) if(b.symbol==symbol) return &b;
  return nullptr;
}
inline std::uint32_t units(View<ItemAmount> items,std::int32_t symbol) noexcept {
  for(const auto& item:items) if(item.symbol==symbol) return item.units;
  return 0;
}
inline bool same_requirement(const Requirement& a,const Requirement& b) noexcept {
  return a.kind==b.kind && a.role==b.role && a.symbol==b.symbol && a.element==b.element && a.value==b.value;
}
inline bool valid_function(const Function* f) noexcept {
  if(!f || !f->name || !*f->name || std::strlen(f->name)>63 || !f->byte_length ||
     f->byte_offset>=lhiver::code_length || f->byte_length>lhiver::code_length-f->byte_offset) return false;
  const auto* expected=function_for(f->symbol);
  return expected && std::strcmp(f->name,expected->name)==0 && f->byte_offset==expected->byte_offset &&
    f->byte_length==expected->byte_length && f->body_sha256==expected->body_sha256 &&
    f->argc==expected->argc && f->type==expected->type && f->flags==expected->flags &&
    f->return_type==expected->return_type && f->parent_index==expected->parent_index;
}
inline bool valid_requirement(const Descriptor& recipe,const Requirement& r) noexcept {
  if(r.kind!=RequirementKind::integer_equal ||
     (r.role!=RequirementRole::context && r.role!=RequirementRole::knowledge) || r.symbol<=0) return false;
  const auto* b=binding_for(recipe,static_cast<std::uint32_t>(r.symbol));
  return b && b->type==2 && b->flags==0 && b->parent_index==-1 && r.element<b->elements;
}
inline bool valid_path(const Descriptor& recipe,View<Effect> path,bool success) noexcept {
  if(!path.data || path.count<3 || path.count>10 || path[path.count-1].kind!=EffectKind::end_production) return false;
  const auto* canonical=find(recipe.id); if(!canonical) return false;
  const auto expected=success?canonical->success:canonical->failure;
  if(path.count!=expected.count) return false;
  std::size_t material=0,prints=0,says=0,ends=0;
  for(std::size_t i=0;i<path.count;++i) {
    const auto& e=path[i];
    const auto& wanted=expected[i];
    if(e.kind!=wanted.kind || e.callee!=wanted.callee || e.offset!=wanted.offset || e.item!=wanted.item ||
       e.units!=wanted.units || e.actor!=wanted.actor) return false;
    if(e.offset>=recipe.callback.function->byte_length || recipe.callback.function->byte_length-e.offset<5 ||
       (i && path[i-1].offset>=e.offset)) return false;
    if(e.kind==EffectKind::remove || e.kind==EffectKind::create) {
      if(e.callee!=(e.kind==EffectKind::remove?184u:170u) || !e.units || e.item<=0) return false;
      const auto action=e.kind==EffectKind::remove?MaterialAction::remove:MaterialAction::create;
      if(success) {
        if(material>=recipe.callback.effects.count) return false;
        const auto& x=recipe.callback.effects[material++];
        if(x.action!=action || x.symbol!=e.item || x.units!=e.units || x.reserved || e.actor!=recipe.material_actor) return false;
      } else {
        if(material>=recipe.callback.failure_effects.count) return false;
        const auto& x=recipe.callback.failure_effects[material++];
        if(x.action!=action || x.symbol!=e.item || x.amount!=e.units || x.actor!=e.actor) return false;
      }
    } else {
      if(e.item!=-1 || e.units) return false;
      if(e.kind==EffectKind::print) { if(e.callee!=12) return false; ++prints; }
      else if(e.kind==EffectKind::say) { if(e.callee!=14646 || !recipe.say) return false; ++says; }
      else if(e.kind==EffectKind::end_production) { if(e.callee!=19965 || i+1!=path.count) return false; ++ends; }
      else return false;
    }
  }
  return material==(success?recipe.callback.effects.count:recipe.callback.failure_effects.count) &&
    prints==1 && says<=1 && ends==1;
}

// Structural consistency of own facts. Neither learning release nor
// runtime/material evidence; in particular, the debit is never executed here.
inline bool structurally_valid(const Descriptor& r) noexcept {
  const auto* canonical=find(r.id);
  if(!canonical || !r.name || !*r.name || !r.targets.data || !r.targets.count || r.targets.count>catalog_target_capacity ||
     !r.bindings.data || !r.bindings.count || r.bindings.count>64 || !r.requirements.data || !r.requirements.count || r.requirements.count>8 ||
     !r.costs.data || !r.costs.count || r.costs.count>catalog_target_capacity || !r.outputs.data || r.outputs.count!=1 ||
     !r.success.data || r.success.count<3 || r.success.count>10 || !r.failure.data || r.failure.count<3 || r.failure.count>10 ||
     r.targets.count!=r.costs.count+r.outputs.count || r.station!=recipe_catalog::Station::alchemy || r.production_context!=4 ||
     r.category!=canonical->category || r.material_actor!=canonical->material_actor ||
     !valid_function(r.station_entry) || r.station_entry->symbol!=20276 ||
     !valid_function(r.station_end) || r.station_end->symbol!=20279 ||
     !valid_function(r.end_production) || r.end_production->symbol!=19965 ||
     (r.say && (!valid_function(r.say) || r.say->symbol!=14646))) return false;
  for(std::size_t i=0;i<r.bindings.count;++i) {
    const auto& b=r.bindings[i];
    if(!b.name || !*b.name || std::strlen(b.name)>63 || !b.symbol || b.symbol>=lhiver::symbol_count) return false;
    for(std::size_t j=0;j<i;++j) if(b.symbol==r.bindings[j].symbol || std::strcmp(b.name,r.bindings[j].name)==0) return false;
  }
  for(std::size_t i=0;i<r.targets.count;++i) {
    const auto& t=r.targets[i];
    if(!t.name || !*t.name || std::strlen(t.name)>63 || t.symbol<=0) return false;
    for(std::size_t j=0;j<i;++j) if(t.symbol==r.targets[j].symbol || std::strcmp(t.name,r.targets[j].name)==0) return false;
    const auto* b=binding_for(r,static_cast<std::uint32_t>(t.symbol));
    if(!b || std::strcmp(b->name,t.name)!=0 || b->type!=7 || b->flags!=1 || b->elements || b->parent_index!=1584) return false;
    if(!units(r.costs,t.symbol) && !units(r.outputs,t.symbol)) return false;
  }
  for(const auto list:{r.costs,r.outputs}) for(std::size_t i=0;i<list.count;++i) {
    const auto& value=list[i]; bool found=false;
    for(const auto& t:r.targets) if(t.symbol==value.symbol) found=true;
    if(!found || !value.units || value.units>INT32_MAX) return false;
    for(std::size_t j=0;j<i;++j) if(list[j].symbol==value.symbol) return false;
  }
  for(const auto& input:r.costs) if(units(r.outputs,input.symbol)) return false;
  if(units(r.costs,flask_symbol)!=1 || units(r.outputs,flask_symbol) ||
     r.extra_flasks.symbol!=flask_symbol || r.extra_flasks.units!=1 || r.extra_flasks.reserved_hand!=1 ||
     r.extra_flasks.failure_refund!=1 || !r.extra_flasks.after_verified_original || r.extra_flasks.original_removes_stored_flasks) return false;
  const auto& h=r.hand;
  if(h.start_item_symbol!=flask_symbol || h.active_item_symbol!=flask_symbol || h.amount!=1 ||
     !h.from_inventory || !h.consumed_on_success || h.failure_refund!=1 || !h.left_must_be_empty ||
     h.evidence!=HandEvidence::inferred_same_station) return false;
  const auto& o=r.owner; const auto& cb=r.callback;
  if(o.instance!=canonical->owner.instance || o.number!=canonical->owner.number || o.npc!=22243 || o.important || o.trade || !o.permanent ||
     !valid_function(o.constructor) || !valid_function(o.condition) || !valid_function(o.information) ||
     o.constructor->symbol!=static_cast<std::uint32_t>(o.instance) || o.constructor->type!=7 || o.constructor->parent_index!=1649 ||
     o.condition->symbol!=canonical->owner.condition->symbol || o.information->symbol!=canonical->owner.information->symbol ||
     o.condition->type!=5 || o.condition->flags!=3 || o.condition->return_type!=2 || o.condition->argc ||
     !o.conditions.data || o.conditions.count!=r.requirements.count || r.requirements.count!=canonical->requirements.count) return false;
  for(std::size_t i=0;i<r.requirements.count;++i) {
    const auto& q=r.requirements[i];
    if(!valid_requirement(r,q) || !same_requirement(q,o.conditions[i]) || !same_requirement(q,canonical->requirements[i])) return false;
    for(std::size_t j=0;j<i;++j) if(q.symbol==r.requirements[j].symbol && q.element==r.requirements[j].element) return false;
  }
  if(!valid_function(cb.function) || cb.function->symbol!=canonical->callback.function->symbol ||
     cb.function->symbol!=o.information->symbol || cb.function->type!=5 || cb.function->flags!=1 || cb.function->argc || cb.function->return_type ||
     cb.owner_instance!=o.instance || cb.invocation!=Invocation::info || cb.original_quantity!=1 || cb.minimum_quantity!=1 ||
     cb.maximum_quantity!=canonical->callback.maximum_quantity || cb.amount.length!=cb.function->byte_length ||
     cb.amount.digest!=cb.function->body_sha256 || !cb.amount.operands || !cb.amount.count || cb.amount.count>AmountPatch::max_operands ||
     cb.amount.count!=canonical->callback.amount.count ||
     !cb.body_checks_resources || !cb.ends_production || cb.clears_choices || !cb.quantity_text_reviewed ||
     !cb.effects.data || cb.effects.count!=r.costs.count || !cb.failure_effects.data || cb.failure_effects.count!=1 ||
     !r.operand_uses.data || r.operand_uses.count!=cb.amount.count || cb.amount.count!=2*(r.costs.count-1)+r.outputs.count) return false;
  const auto& refund=cb.failure_effects[0];
  if(refund.action!=MaterialAction::create || refund.symbol!=flask_symbol || refund.amount!=1 || refund.actor!=Actor::self) return false;
  for(std::size_t i=0;i<cb.amount.count;++i) {
    const auto& op=cb.amount.operands[i]; const auto& role=r.operand_uses[i];
    const auto& wanted=canonical->callback.amount.operands[i];
    const auto& wanted_role=canonical->operand_uses[i];
    if(op.offset!=wanted.offset || op.original!=wanted.original || op.units!=wanted.units ||
       op.reserved!=wanted.reserved || op.allow_zero!=wanted.allow_zero || role.offset!=wanted_role.offset ||
       role.role!=wanted_role.role || role.symbol!=wanted_role.symbol) return false;
    const auto cost=units(r.costs,role.symbol), output=units(r.outputs,role.symbol);
    const auto expected=role.role==AmountRole::create_count?output:cost;
    if(role.symbol==flask_symbol || role.offset!=op.offset || !op.offset || op.offset>=cb.amount.length || cb.amount.length-op.offset<4 ||
       !expected || op.original<=0 || op.units!=expected || static_cast<std::uint32_t>(op.original)!=expected || op.reserved || op.allow_zero ||
       (role.role!=AmountRole::create_count && role.role!=AmountRole::remove_count && role.role!=AmountRole::minimum_inventory)) return false;
    for(std::size_t j=0;j<i;++j) {
      const auto& other=cb.amount.operands[j];
      if((op.offset<other.offset+4 && other.offset<op.offset+4) ||
         (role.role==r.operand_uses[j].role && role.symbol==r.operand_uses[j].symbol)) return false;
    }
    if(role.role!=AmountRole::minimum_inventory) {
      bool effect=false;
      for(const auto& e:r.success) if(e.item==role.symbol && e.offset==op.offset+4 && e.units==op.units &&
        e.kind==(role.role==AmountRole::remove_count?EffectKind::remove:EffectKind::create)) effect=true;
      if(!effect) return false;
    }
  }
  // Every input has exactly one original guard and debit; the output exactly
  // one create operand. The fixed deficit refund is never a patch target.
  for(const auto& input:r.costs) if(input.symbol!=flask_symbol) {
    std::size_t guards=0,removes=0;
    for(const auto& role:r.operand_uses) if(role.symbol==input.symbol) {
      guards+=role.role==AmountRole::minimum_inventory; removes+=role.role==AmountRole::remove_count;
    }
    if(guards!=1 || removes!=1) return false;
  }
  if(!valid_path(r,r.success,true) || !valid_path(r,r.failure,false) || !r.prints.data || r.prints.count!=2 ||
     !r.item_constructors.data || r.item_constructors.count!=r.targets.count) return false;
  for(std::size_t i=0;i<r.prints.count;++i)
    if(r.prints[i].symbol!=canonical->prints[i].symbol || r.prints[i].length!=canonical->prints[i].length ||
       r.prints[i].digest!=canonical->prints[i].digest) return false;
  for(std::size_t i=0;i<r.item_constructors.count;++i) {
    const auto* f=r.item_constructors[i];
    if(!valid_function(f) || f->symbol!=static_cast<std::uint32_t>(r.targets[i].symbol) || f->type!=7 || f->parent_index!=1584) return false;
  }
  return true;
}

struct QuantityPlan {
  const Callback* callback = nullptr;
  std::uint32_t quantity = 0, extra_flasks = 0;
  bool valid = false, patch_original = false;
};
// Only mathematical template selection. Unknown stock/knowledge/hand or native
// evidence remains locked by the integrator. N1 does not perform a Remove(0).
inline QuantityPlan plan_quantity(const Descriptor& recipe,std::uint32_t n) noexcept {
  if(!n || !structurally_valid(recipe) || n>recipe.callback.maximum_quantity) return {};
  for(const auto entries:{recipe.costs,recipe.outputs}) for(const auto& e:entries)
    if(std::uint64_t{e.units}*n>INT32_MAX) return {};
  return {&recipe.callback,n,n-1,true,n>1};
}
} // namespace ubc::alchemy_recipe_catalog
