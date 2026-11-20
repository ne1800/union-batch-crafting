// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/station_recipe_catalog.hpp"

// Our own facts for exactly five ordinary L'Hiver anvil weapons.
// No original scripts or bytes, registration or runtime authorization.
// DAT-SHA: 71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4.
namespace ubc::anvil_recipe_catalog {
using lhiver::digest_literal;
using lhiver::Function;
using lhiver::SymbolBinding;
using recipe_catalog::Actor;
using recipe_catalog::Callback;
using recipe_catalog::Invocation;
using recipe_catalog::ItemAmount;
using recipe_catalog::MaterialAction;
using recipe_catalog::MaterialEffect;
using recipe_catalog::Owner;
using recipe_catalog::Requirement;
using recipe_catalog::RequirementKind;
using recipe_catalog::RequirementRole;
using recipe_catalog::View;
using recipe_catalog::view;
using station_recipe_catalog::AmountRole;
using station_recipe_catalog::CheckSite;
using station_recipe_catalog::Completion;
using station_recipe_catalog::HandSource;
using station_recipe_catalog::OperandUse;
using station_recipe_catalog::RequirementCheck;
enum class RecipeId : std::uint32_t {
  common_sword = 0x5001,
  harad_01,
  harad_02,
  harad_03,
  harad_04
};
enum class Station : std::uint32_t { anvil = 1 };
enum class HandSide { unknown, left, right };
struct HandContract {
  station_recipe_catalog::HandItem left, right;
  HandSide anchor = HandSide::unknown;
  bool static_lifecycle_verified = false, observed_in_world = false;
  std::int32_t left_tmp_level = -1, right_tmp_level = -1;
};
enum class Page { root, common, ore };
enum class EffectKind { minimum, remove, create, print, say, end_production, integer_store };
struct Effect {
  EffectKind kind;
  std::uint32_t offset, callee;
  std::int32_t symbol, value;
  Actor actor;
};
struct OriginalOffer {
  const Function* function;
  std::uint32_t quantity;
};
struct PrintFingerprint {
  std::uint32_t symbol, length;
  lhiver::Digest sha256;
};
struct NativeRange {
  const char* name;
  std::uint32_t address, length;
  lhiver::Digest sha256;
};
struct Navigation {
  Owner owner;
  Page before, after;
  std::int32_t flag, before_value, after_value;
};
struct StationProfile {
  Station station;
  std::int32_t production_context;
  const char *on_state, *scheme, *visual, *use_with_item, *condition_name, *vob_name;
  std::int32_t state_num, rewind;
  const Function *entry, *mob_condition;
  Owner end;
};
struct Descriptor {
  RecipeId id;
  const char* name;
  Station station;
  std::int32_t production_context;
  View<PackedInventoryTarget> targets;
  View<SymbolBinding> bindings;
  View<Requirement> requirements;
  View<RequirementCheck> checks;
  Owner owner;
  const Function* back;
  Callback callback;
  OriginalOffer original_three;
  View<ItemAmount> costs, outputs;
  HandContract hand;
  Completion completion;
  const StationProfile* station_profile;
  View<OperandUse> operand_uses;
  Actor minimum_actor, material_actor;
  bool require_self_is_hero;
  View<Effect> success, failure;
  View<PrintFingerprint> prints;
  View<const Function*> item_constructors;
  View<NativeRange> native_dependencies;
};
inline constexpr bool runtime_ready = false;
inline constexpr std::int32_t raw_symbol = 16611, left_prop_symbol = 16666, hammer_symbol = 16435;
// Original anvil entry 2026-11-01: both slots tmpLevel1/amount1, right
// 16435 is InteractItem, left 16666 is a separate prop. Both hand
// objects were separated from the fully read inventory list. This
// observation replaces neither the native stable point nor a crafting
// test. EV_DestroyInteractItem stores only the instance of the anchor
// for the trailing stored debit: therefore hammer store completely
// empty, also no zero-quantity objects; the left requisite may be
// stored. MSB chunk order alone remains NO dispatch evidence. Complete
// slot/inventory alias check, exact tmpLevel and native hand contract
// remain required. The static total trailing is bound separately.
// No steel reserve, no production permission from observed_in_world.
inline constexpr HandContract anvil_hand{{16666, 1, HandSource::generated, false},
                                         {16435, 1, HandSource::generated, true},
                                         HandSide::right,
                                         false,
                                         true,
                                         1,
                                         1};

inline constexpr std::array<Function, 57> functions{{
    {"B_SAY", 14646, 218446, 39,
     digest_literal("b8b93bf2920074b9958b398d5d642935220402bd20ff5044c51f9a416675d6ad"), 3, 5, 1, 0,
     -1},
    {"ITMW_1H_MACE_L_04", 16435, 337048, 244,
     digest_literal("ff6e32001cefc007093dc16c576a78332ec5b68a83ccd382393584acee714d99"), 0, 7, 1, 0,
     1584},
    {"ITMW_1H_COMMON_01", 16477, 347322, 254,
     digest_literal("d3bc30a90776d1bdb7c860250df3341e3561a7bc490430618db0f25fee362f88"), 0, 7, 1, 0,
     1584},
    {"ITMW_SCHWERT1", 16483, 348796, 254,
     digest_literal("143288508b8cde18573fd4c0154fee988224c47ee53f3a28c64d6a668c09cfaf"), 0, 7, 1, 0,
     1584},
    {"ITMW_SCHWERT4", 16491, 350768, 254,
     digest_literal("1fd308171fddfc004862ea970bfd001273586e5e6e048ede4e89a3cfefa2543c"), 0, 7, 1, 0,
     1584},
    {"ITMW_RUBINKLINGE", 16497, 352288, 254,
     digest_literal("c69c842dbe3cd3f0c75478a7475ef8f5f0d645a17d20c5dca1f8fe1eeb5909b6"), 0, 7, 1, 0,
     1584},
    {"ITMW_ELBASTARDO", 16512, 356008, 254,
     digest_literal("8dab0071a0a2f2b75d10913d7475ea9df383b68d5ccb8287bdb0c357653013b8"), 0, 7, 1, 0,
     1584},
    {"ITMISWORDRAWHOT", 16611, 365701, 102,
     digest_literal("7ee63a8d9add6278c32ec76aa59e2242311eb313031378583aa9d646dc29bcf8"), 0, 7, 1, 0,
     1584},
    {"ITMI_PLIERS_SMLT", 16666, 371884, 78,
     digest_literal("9469e348623d0ab4151ce52cd4d0c3551935ac0690a95525f329c482663052da"), 0, 7, 1, 0,
     1584},
    {"B_ENDPRODUCTIONDIALOG", 19965, 672156, 49,
     digest_literal("e9fcd2f7db76bab4ed7c5ceb6e5a98c3ab3b8166a3614a4dda2513092da34e64"), 0, 5, 1, 0,
     -1},
    {"PC_SMITHWEAPON_END", 20698, 786629, 67,
     digest_literal("8810463fb30a664a82ba3015b3e46a3bd0d6158709642b610e295530de11e759"), 0, 7, 1, 0,
     1649},
    {"PC_SMITHWEAPON_END_CONDITION", 20699, 786696, 23,
     digest_literal("c2b78a81d70a781ccdcc7458207edfd16e0d329dddb7b6c8d51a4b17d530526b"), 0, 5, 3, 2,
     -1},
    {"PC_SMITHWEAPON_END_INFO", 20700, 786719, 28,
     digest_literal("1df61d52c2edcacff78c6f9d9ab3f4b345462fc718db29b535c89a15d92a2cad"), 0, 5, 1, 0,
     -1},
    {"PC_COMMON", 20701, 786747, 56,
     digest_literal("cdbf1ab15d2620a16434806aab5879d824e0a82a74835ec15b291ddd2860044d"), 0, 7, 1, 0,
     1649},
    {"PC_COMMON_CONDITION", 20702, 786803, 47,
     digest_literal("60a276a2a25de819374ed63bde9d451f57d60770216ba2efd56786530179c472"), 0, 5, 3, 2,
     -1},
    {"PC_COMMON_INFO", 20703, 786850, 12,
     digest_literal("a2ace5d829a4f16358ac9a480ff4b19b8ddad469f09521a5a15b8e6cbb6ad433"), 0, 5, 1, 0,
     -1},
    {"PC_ORE", 20704, 786862, 56,
     digest_literal("8f23ed155555cadc5caf63f033f0c36315bcf0e66196a55bdc8214a302bbacc3"), 0, 7, 1, 0,
     1649},
    {"PC_ORE_CONDITION", 20705, 786918, 47,
     digest_literal("a92b266a30f484ffdcc4ec320f6941dfecc97c1b27e0a203fb84f330063401c1"), 0, 5, 3, 2,
     -1},
    {"PC_ORE_INFO", 20706, 786965, 12,
     digest_literal("638849c0619ddb32975a7d15e8bb3813e9dacf1dce6f4eb98c32426028f9c88c"), 0, 5, 1, 0,
     -1},
    {"PC_COMMONBACK", 20707, 786977, 67,
     digest_literal("e388b80e05c3167842107998d6b6e92c0caf40332fbfaefefc283cc1da12d020"), 0, 7, 1, 0,
     1649},
    {"PC_COMMONBACK_CONDITION", 20708, 787044, 47,
     digest_literal("6475ba11fc2917b98b5245060ce45403b7b604133d1e9bfd419a0206ab151806"), 0, 5, 3, 2,
     -1},
    {"PC_COMMONBACK_INFO", 20709, 787091, 12,
     digest_literal("72fe493e410dea4ef58e6a4d387153c6710c6869156d2a3c2a4e4012d0fa39ea"), 0, 5, 1, 0,
     -1},
    {"PC_OREBACK", 20710, 787103, 67,
     digest_literal("d94feb141446d7b8967b0053efc28ba910d610a29974cc1b31c6999d7f91702c"), 0, 7, 1, 0,
     1649},
    {"PC_OREBACK_CONDITION", 20711, 787170, 47,
     digest_literal("b3dfef8af4a728ca6f28eb9d3dece888b30c9bb5bcdfcaabf684f9d2406aad52"), 0, 5, 3, 2,
     -1},
    {"PC_OREBACK_INFO", 20712, 787217, 12,
     digest_literal("a5f381a0aed557e29cdfce09bb88386881f5cb7347949b85549709c69a6f98e7"), 0, 5, 1, 0,
     -1},
    {"PC_ITMW_1H_COMMON", 20713, 787229, 45,
     digest_literal("ba5f7cdf9c3e88e02a3ba6d2c693d3fe0b235e754748f0de0072b865444e587d"), 0, 7, 1, 0,
     1649},
    {"PC_ITMW_1H_COMMON_CONDITION", 20714, 787274, 47,
     digest_literal("ed033c7f38c762fed0364687481bce9f60b2e2ebf92885afb3256acb1b5b8cdd"), 0, 5, 3, 2,
     -1},
    {"PC_ITMW_1H_COMMON_INFO", 20715, 787321, 71,
     digest_literal("1c58253ad9c96063c5e87b3063369a2c992548d84bca10a2c30d7a6a756c447c"), 0, 5, 1, 0,
     -1},
    {"PC_ITMW_1H_COMMON_BACK1", 20716, 787392, 11,
     digest_literal("33ef1515faf81d2a65ae3f2b7c06d17166cb2490a246a09bc89ff8b078631ac4"), 0, 5, 1, 0,
     -1},
    {"PC_ITMW_1H_COMMON_1", 20717, 787403, 148,
     digest_literal("4ba145a0104d994ee1833ba04ef677ea9262054fdc994949ff098949af4b299d"), 0, 5, 1, 0,
     -1},
    {"PC_ITMW_1H_COMMON_3", 20718, 787551, 148,
     digest_literal("b4007b78d13866a6f214fb5572c45e6da2139151309d7216840a1bf29d93b6fb"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_01", 20719, 787699, 45,
     digest_literal("af92cc9c775d3324e37b3052e50e13bc88c76afa1b4270bc403d53113b3497cf"), 0, 7, 1, 0,
     1649},
    {"PC_WEAPON_1H_HARAD_01_CONDITION", 20720, 787744, 48,
     digest_literal("0feded9e89dec511795f6569834c74e8376daadf02959ddef07b6370931f3108"), 0, 5, 3, 2,
     -1},
    {"PC_WEAPON_1H_HARAD_01_INFO", 20721, 787792, 71,
     digest_literal("b55915434821cf48ad4a267e7c4b8a283c9684629d7f394c96aea5ce29f7f19c"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_01_BACK1", 20722, 787863, 11,
     digest_literal("a1063b650a09715a8a2e60e15b034395227485f1b1865e591bc1f220ca2a14da"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_01_1", 20723, 787874, 148,
     digest_literal("c6ef67aa4edb6c44bebaa096f3a0efa7e1cbdacc99b87e2e67df2eae090e57cd"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_01_3", 20724, 788022, 148,
     digest_literal("e3d8c1b155c9a06fd30849eb06bcef9f14a89544de4a108bce912e8e9a7498f1"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_02", 20725, 788170, 45,
     digest_literal("cac0b625325256a657c43d634992d67d765cb868fb764171bd72d4ef42e1ca78"), 0, 7, 1, 0,
     1649},
    {"PC_WEAPON_1H_HARAD_02_CONDITION", 20726, 788215, 48,
     digest_literal("fceecc805209a31940cc0eb09a04b2a6992416269f36a0dca4780f497869916e"), 0, 5, 3, 2,
     -1},
    {"PC_WEAPON_1H_HARAD_02_INFO", 20727, 788263, 71,
     digest_literal("6192a7110fa2ee1f4358ecad8840f4c5fedbb683ad2617840314018587d0e4cb"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_02_BACK1", 20728, 788334, 11,
     digest_literal("18c56608f20f732767a18a48242910681dfc965ab7f9bb0f5d4a6a39c872319a"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_02_1", 20729, 788345, 148,
     digest_literal("2ce8b6c334062913b70f03888466037795de4da62658015558f88e3b57cfda56"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_02_3", 20730, 788493, 148,
     digest_literal("cd866af13b6d2e2e4ae484d6cc415dfebcb9ac2abd384158c84532978e16fe14"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_03", 20731, 788641, 45,
     digest_literal("e42385c8ff1f931b5521d9124232735e29d54ca807199046e36f18d4bf6c7b40"), 0, 7, 1, 0,
     1649},
    {"PC_WEAPON_1H_HARAD_03_CONDITION", 20732, 788686, 48,
     digest_literal("154aa4100753b9334d6bfb6c1c874614f85dd72d56fba1f62f76c3b69cd31c45"), 0, 5, 3, 2,
     -1},
    {"PC_WEAPON_1H_HARAD_03_INFO", 20733, 788734, 71,
     digest_literal("e740e0c611b53d99690b2ac72a0e676cade543eebb562cb993283956877d5536"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_03_BACK", 20734, 788805, 11,
     digest_literal("67a0bff4ac177c67261f40d819c87408e918b70aabac2e72b403e3af82d73915"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_03_1", 20735, 788816, 148,
     digest_literal("d83c050b8cf6cc290d9929da5efd0d6907ac6e6db9daf837650703d81c5f1d36"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_03_3", 20736, 788964, 148,
     digest_literal("9160e4bfd8ae9e5617121ca8d65c2c5e3175ed07475105641b81da87a64a07bd"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_04", 20737, 789112, 45,
     digest_literal("95678e9e1ad5b255f1ced37ed4d81334334820a08f3b4c2a891d337f2a0b2c55"), 0, 7, 1, 0,
     1649},
    {"PC_WEAPON_1H_HARAD_04_CONDITION", 20738, 789157, 48,
     digest_literal("ea8eb6bd9bfe595da772e15418207c538224181cee8930eb80a82214eba61e0e"), 0, 5, 3, 2,
     -1},
    {"PC_WEAPON_1H_HARAD_04_INFO", 20739, 789205, 71,
     digest_literal("978bc6fcff72cfc47b3b3980baaa271ab4cf586396957d86c3adeb94bab586c1"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_04_BACK", 20740, 789276, 11,
     digest_literal("9a293ae423c70543d8bc0a38454458cf8f9dff0c8deb3497bb895532ed706b7e"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_04_1", 20741, 789287, 148,
     digest_literal("770d10a0d889561391e91ab5050e22f3d37ae648749ad4a658b042762fb4777b"), 0, 5, 1, 0,
     -1},
    {"PC_WEAPON_1H_HARAD_04_3", 20742, 789435, 148,
     digest_literal("578ac94fad04ed82804d83dbeada30445652ec8f138707d12cb62ecd526265ba"), 0, 5, 1, 0,
     -1},
    {"SMITHWEAPON_S1", 20767, 791931, 353,
     digest_literal("02d992f756c2b3d3e29bd2318ae13dc9ce798113cfe5bf72466c62b79fa94038"), 0, 5, 1, 0,
     -1},
    {"B_USEANVIL", 20768, 792284, 23,
     digest_literal("b80fb114b3f24d442b23fd9a54ac2a3ee7c0bf2eff83237f1f23c7f25917cbc9"), 0, 5, 3, 2,
     -1},
}};
inline constexpr auto common_bindings = station_recipe_catalog::common_bindings;
template <std::size_t N>
constexpr auto with_common(const std::array<SymbolBinding, N>& own) noexcept {
  std::array<SymbolBinding, common_bindings.size() + N> result{};
  for (std::size_t i = 0; i < common_bindings.size(); ++i)
    result[i] = common_bindings[i];
  for (std::size_t i = 0; i < N; ++i)
    result[common_bindings.size() + i] = own[i];
  return result;
}
inline constexpr std::array<Requirement, 1> production_condition{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
}};
inline constexpr std::array<Requirement, 3> root_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 0},
    {RequirementKind::integer_equal, RequirementRole::context, 10551, 0, 0},
}};
inline constexpr std::array<Requirement, 3> common_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10551, 0, 0},
}};
inline constexpr std::array<Requirement, 3> ore_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 0},
    {RequirementKind::integer_equal, RequirementRole::context, 10551, 0, 1},
}};
// No NR store in recipe/category instances: oCInfo ctor sets -1 at 7033FC.
inline constexpr Owner end_owner = {20698,          999,
                                    22243,          false,
                                    false,          true,
                                    &functions[10], &functions[11],
                                    &functions[12], view(production_condition)};
inline constexpr std::array<Navigation, 4> navigation{{
    {{20701, -1, 22243, false, false, true, &functions[13], &functions[14], &functions[15],
      view(root_conditions)},
     Page::root,
     Page::common,
     10552,
     0,
     1},
    {{20704, -1, 22243, false, false, true, &functions[16], &functions[17], &functions[18],
      view(root_conditions)},
     Page::root,
     Page::ore,
     10551,
     0,
     1},
    {{20707, 99, 22243, false, false, true, &functions[19], &functions[20], &functions[21],
      view(common_conditions)},
     Page::common,
     Page::root,
     10552,
     1,
     0},
    {{20710, 99, 22243, false, false, true, &functions[22], &functions[23], &functions[24],
      view(ore_conditions)},
     Page::ore,
     Page::root,
     10551,
     1,
     0},
}};
inline constexpr StationProfile station{Station::anvil,
                                        1,
                                        "SMITHWEAPON",
                                        "BSANVIL",
                                        "BSANVIL_OC.MDS",
                                        "",
                                        "B_USEANVIL",
                                        "",
                                        1,
                                        0,
                                        &functions[55],
                                        &functions[56],
                                        end_owner};
inline constexpr std::array<NativeRange, 7> native_dependencies{{
    {"oCItem::InitByScript", 0x711bd0, 1290,
     digest_literal("715545cd2e1ce153c6d202d04061ced94d6bcaad61cbe8e4eca421af312e8232")},
    {"oCItem::MultiSlot", 0x7125a0, 99,
     digest_literal("dd0405c11373e734e96da95c78810e0f052a0d11abd2433f7383b074f6fc9c11")},
    {"oCNpc::CreateItems", 0x730800, 445,
     digest_literal("29308fddee492fe52ddcbf5f382e9e98a4d5043c7d74ff0df07b14a707d38256")},
    {"oCNpc::PutInInv(oCItem*)", 0x749350, 357,
     digest_literal("047d8df6c814ae187d00d7d5186c124eb13e7e7bae6c27babae88cb809ad684c")},
    {"oCNpc::EV_CreateInteractItem", 0x754890, 687,
     digest_literal("ab10f3df64ff7d210e9c8fe3b11e4b2163f580cba10a30aa824f033eae31ec64")},
    {"oCNpc::SetInteractItem", 0x74acc0, 564,
     digest_literal("e307a7e94552949f0fca7e31cb36b374bf058560f244ec3fa4c07f537983a707")},
    {"oCNpc::EV_DestroyInteractItem", 0x754b40, 460,
     digest_literal("493ffa795e2d6826e86c3fb039f5091bb38e14ff2afa9b7362800534fbee4564")},
}};
// Fresh weapons: mainflag2 | Scriptflags0x4000 = 0x4002. The verified
// MultiSlot/CreateItems path creates quantity N without executing OnEquip.
inline constexpr std::array<PrintFingerprint, 2> prints{{
    {14370, 18, digest_literal("243fe6c556a065251ebe81eb0b36e72dd1fd9d52d2ab6df3d3e3ca7581bca544")},
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
}};
inline constexpr std::array<AmountOperand, 3> operands{{
    {1, 1, 1, 0, false},
    {37, 1, 1, 0, false},
    {57, 1, 1, 0, false},
}};
inline constexpr std::array<PackedInventoryTarget, 4> common_sword_targets{{
    {"ITMISWORDRAWHOT", 16611},
    {"ITMW_1H_COMMON_01", 16477},
    {"ITMI_PLIERS_SMLT", 16666},
    {"ITMW_1H_MACE_L_04", 16435},
}};
inline constexpr std::array<SymbolBinding, 17> common_sword_own_bindings{{
    {"ITMISWORDRAWHOT", 16611, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMW_1H_COMMON_01", 16477, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_PLIERS_SMLT", 16666, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMW_1H_MACE_L_04", 16435, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_ITMW_1H_COMMON", 20713, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_SMITHWEAPON_END", 20698, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_COMMON", 20701, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_ORE", 20704, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_COMMONBACK", 20707, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_OREBACK", 20710, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_SMITH", 1496, 2, 0, 30, -1, false, 0, false, 0},
    {"NORMALWAFFEN", 10552, 2, 0, 1, -1, false, 0, false, 0},
    {"ERZWAFFEN", 10551, 2, 0, 1, -1, false, 0, false, 0},
    {"STARTSMITHING", 10536, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_SMITHWEAPON", 12311, 2, 1, 1, -1, true, 1, false, 0},
    {"PRINT_SMITHSUCCESS", 14370, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto common_sword_bindings = with_common(common_sword_own_bindings);
inline constexpr std::array<Requirement, 3> common_sword_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
}};
inline constexpr std::array<Requirement, 5> common_sword_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10536, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16611, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 5> common_sword_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
     CheckSite::owner_condition,
     &functions[26]},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 0, 1},
     CheckSite::owner_condition,
     &functions[26]},
    {{RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
     CheckSite::owner_condition,
     &functions[26]},
    {{RequirementKind::integer_equal, RequirementRole::context, 10536, 0, 1},
     CheckSite::mob_condition,
     &functions[56]},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16611, 0, 1},
     CheckSite::callback,
     &functions[29]},
}};
inline constexpr Owner common_sword_owner = {20713,          -1,
                                             22243,          false,
                                             false,          true,
                                             &functions[25], &functions[26],
                                             &functions[27], view(common_sword_conditions)};
inline constexpr std::array<ItemAmount, 1> common_sword_costs{{
    {16611, 1},
}};
inline constexpr std::array<ItemAmount, 1> common_sword_outputs{{
    {16477, 1},
}};
inline constexpr std::array<MaterialEffect, 2> common_sword_effects{{
    {MaterialAction::remove, 16611, 1, 0},
    {MaterialAction::create, 16477, 1, 0},
}};
inline constexpr std::array<OperandUse, 3> common_sword_operand_uses{{
    {1, AmountRole::minimum_inventory, 16611},
    {37, AmountRole::remove_count, 16611},
    {57, AmountRole::create_count, 16477},
}};
inline constexpr std::array<const Function*, 4> common_sword_items{{
    &functions[7],
    &functions[2],
    &functions[8],
    &functions[1],
}};
inline constexpr std::array<Effect, 7> common_sword_success{{
    {EffectKind::minimum, 15, 160, 16611, 1, Actor::self},
    {EffectKind::remove, 41, 184, 16611, 1, Actor::hero},
    {EffectKind::create, 61, 170, 16477, 1, Actor::hero},
    {EffectKind::print, 71, 12, 14370, 0, Actor::hero},
    {EffectKind::say, 91, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 131, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 146, 0, 10552, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 5> common_sword_failure{{
    {EffectKind::minimum, 15, 160, 16611, 1, Actor::self},
    {EffectKind::print, 106, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 126, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 131, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 146, 0, 10552, 0, Actor::hero},
}};

inline constexpr std::array<PackedInventoryTarget, 4> harad_01_targets{{
    {"ITMISWORDRAWHOT", 16611},
    {"ITMW_SCHWERT1", 16483},
    {"ITMI_PLIERS_SMLT", 16666},
    {"ITMW_1H_MACE_L_04", 16435},
}};
inline constexpr std::array<SymbolBinding, 17> harad_01_own_bindings{{
    {"ITMISWORDRAWHOT", 16611, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMW_SCHWERT1", 16483, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_PLIERS_SMLT", 16666, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMW_1H_MACE_L_04", 16435, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_WEAPON_1H_HARAD_01", 20719, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_SMITHWEAPON_END", 20698, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_COMMON", 20701, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_ORE", 20704, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_COMMONBACK", 20707, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_OREBACK", 20710, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_SMITH", 1496, 2, 0, 30, -1, false, 0, false, 0},
    {"NORMALWAFFEN", 10552, 2, 0, 1, -1, false, 0, false, 0},
    {"ERZWAFFEN", 10551, 2, 0, 1, -1, false, 0, false, 0},
    {"STARTSMITHING", 10536, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_SMITHWEAPON", 12311, 2, 1, 1, -1, true, 1, false, 0},
    {"PRINT_SMITHSUCCESS", 14370, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto harad_01_bindings = with_common(harad_01_own_bindings);
inline constexpr std::array<Requirement, 3> harad_01_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 9, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
}};
inline constexpr std::array<Requirement, 5> harad_01_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 9, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10536, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16611, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 5> harad_01_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
     CheckSite::owner_condition,
     &functions[32]},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 9, 1},
     CheckSite::owner_condition,
     &functions[32]},
    {{RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
     CheckSite::owner_condition,
     &functions[32]},
    {{RequirementKind::integer_equal, RequirementRole::context, 10536, 0, 1},
     CheckSite::mob_condition,
     &functions[56]},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16611, 0, 1},
     CheckSite::callback,
     &functions[35]},
}};
inline constexpr Owner harad_01_owner = {20719,          -1,
                                         22243,          false,
                                         false,          true,
                                         &functions[31], &functions[32],
                                         &functions[33], view(harad_01_conditions)};
inline constexpr std::array<ItemAmount, 1> harad_01_costs{{
    {16611, 1},
}};
inline constexpr std::array<ItemAmount, 1> harad_01_outputs{{
    {16483, 1},
}};
inline constexpr std::array<MaterialEffect, 2> harad_01_effects{{
    {MaterialAction::remove, 16611, 1, 0},
    {MaterialAction::create, 16483, 1, 0},
}};
inline constexpr std::array<OperandUse, 3> harad_01_operand_uses{{
    {1, AmountRole::minimum_inventory, 16611},
    {37, AmountRole::remove_count, 16611},
    {57, AmountRole::create_count, 16483},
}};
inline constexpr std::array<const Function*, 4> harad_01_items{{
    &functions[7],
    &functions[3],
    &functions[8],
    &functions[1],
}};
inline constexpr std::array<Effect, 7> harad_01_success{{
    {EffectKind::minimum, 15, 160, 16611, 1, Actor::self},
    {EffectKind::remove, 41, 184, 16611, 1, Actor::hero},
    {EffectKind::create, 61, 170, 16483, 1, Actor::hero},
    {EffectKind::print, 71, 12, 14370, 0, Actor::hero},
    {EffectKind::say, 91, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 131, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 146, 0, 10552, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 5> harad_01_failure{{
    {EffectKind::minimum, 15, 160, 16611, 1, Actor::self},
    {EffectKind::print, 106, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 126, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 131, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 146, 0, 10552, 0, Actor::hero},
}};

inline constexpr std::array<PackedInventoryTarget, 4> harad_02_targets{{
    {"ITMISWORDRAWHOT", 16611},
    {"ITMW_SCHWERT4", 16491},
    {"ITMI_PLIERS_SMLT", 16666},
    {"ITMW_1H_MACE_L_04", 16435},
}};
inline constexpr std::array<SymbolBinding, 17> harad_02_own_bindings{{
    {"ITMISWORDRAWHOT", 16611, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMW_SCHWERT4", 16491, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_PLIERS_SMLT", 16666, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMW_1H_MACE_L_04", 16435, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_WEAPON_1H_HARAD_02", 20725, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_SMITHWEAPON_END", 20698, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_COMMON", 20701, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_ORE", 20704, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_COMMONBACK", 20707, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_OREBACK", 20710, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_SMITH", 1496, 2, 0, 30, -1, false, 0, false, 0},
    {"NORMALWAFFEN", 10552, 2, 0, 1, -1, false, 0, false, 0},
    {"ERZWAFFEN", 10551, 2, 0, 1, -1, false, 0, false, 0},
    {"STARTSMITHING", 10536, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_SMITHWEAPON", 12311, 2, 1, 1, -1, true, 1, false, 0},
    {"PRINT_SMITHSUCCESS", 14370, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto harad_02_bindings = with_common(harad_02_own_bindings);
inline constexpr std::array<Requirement, 3> harad_02_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 10, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
}};
inline constexpr std::array<Requirement, 5> harad_02_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 10, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10536, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16611, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 5> harad_02_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
     CheckSite::owner_condition,
     &functions[38]},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 10, 1},
     CheckSite::owner_condition,
     &functions[38]},
    {{RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
     CheckSite::owner_condition,
     &functions[38]},
    {{RequirementKind::integer_equal, RequirementRole::context, 10536, 0, 1},
     CheckSite::mob_condition,
     &functions[56]},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16611, 0, 1},
     CheckSite::callback,
     &functions[41]},
}};
inline constexpr Owner harad_02_owner = {20725,          -1,
                                         22243,          false,
                                         false,          true,
                                         &functions[37], &functions[38],
                                         &functions[39], view(harad_02_conditions)};
inline constexpr std::array<ItemAmount, 1> harad_02_costs{{
    {16611, 1},
}};
inline constexpr std::array<ItemAmount, 1> harad_02_outputs{{
    {16491, 1},
}};
inline constexpr std::array<MaterialEffect, 2> harad_02_effects{{
    {MaterialAction::remove, 16611, 1, 0},
    {MaterialAction::create, 16491, 1, 0},
}};
inline constexpr std::array<OperandUse, 3> harad_02_operand_uses{{
    {1, AmountRole::minimum_inventory, 16611},
    {37, AmountRole::remove_count, 16611},
    {57, AmountRole::create_count, 16491},
}};
inline constexpr std::array<const Function*, 4> harad_02_items{{
    &functions[7],
    &functions[4],
    &functions[8],
    &functions[1],
}};
inline constexpr std::array<Effect, 7> harad_02_success{{
    {EffectKind::minimum, 15, 160, 16611, 1, Actor::self},
    {EffectKind::remove, 41, 184, 16611, 1, Actor::hero},
    {EffectKind::create, 61, 170, 16491, 1, Actor::hero},
    {EffectKind::print, 71, 12, 14370, 0, Actor::hero},
    {EffectKind::say, 91, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 131, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 146, 0, 10552, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 5> harad_02_failure{{
    {EffectKind::minimum, 15, 160, 16611, 1, Actor::self},
    {EffectKind::print, 106, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 126, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 131, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 146, 0, 10552, 0, Actor::hero},
}};

inline constexpr std::array<PackedInventoryTarget, 4> harad_03_targets{{
    {"ITMISWORDRAWHOT", 16611},
    {"ITMW_RUBINKLINGE", 16497},
    {"ITMI_PLIERS_SMLT", 16666},
    {"ITMW_1H_MACE_L_04", 16435},
}};
inline constexpr std::array<SymbolBinding, 17> harad_03_own_bindings{{
    {"ITMISWORDRAWHOT", 16611, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMW_RUBINKLINGE", 16497, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_PLIERS_SMLT", 16666, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMW_1H_MACE_L_04", 16435, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_WEAPON_1H_HARAD_03", 20731, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_SMITHWEAPON_END", 20698, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_COMMON", 20701, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_ORE", 20704, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_COMMONBACK", 20707, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_OREBACK", 20710, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_SMITH", 1496, 2, 0, 30, -1, false, 0, false, 0},
    {"NORMALWAFFEN", 10552, 2, 0, 1, -1, false, 0, false, 0},
    {"ERZWAFFEN", 10551, 2, 0, 1, -1, false, 0, false, 0},
    {"STARTSMITHING", 10536, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_SMITHWEAPON", 12311, 2, 1, 1, -1, true, 1, false, 0},
    {"PRINT_SMITHSUCCESS", 14370, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto harad_03_bindings = with_common(harad_03_own_bindings);
inline constexpr std::array<Requirement, 3> harad_03_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 11, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
}};
inline constexpr std::array<Requirement, 5> harad_03_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 11, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10536, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16611, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 5> harad_03_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
     CheckSite::owner_condition,
     &functions[44]},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 11, 1},
     CheckSite::owner_condition,
     &functions[44]},
    {{RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
     CheckSite::owner_condition,
     &functions[44]},
    {{RequirementKind::integer_equal, RequirementRole::context, 10536, 0, 1},
     CheckSite::mob_condition,
     &functions[56]},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16611, 0, 1},
     CheckSite::callback,
     &functions[47]},
}};
inline constexpr Owner harad_03_owner = {20731,          -1,
                                         22243,          false,
                                         false,          true,
                                         &functions[43], &functions[44],
                                         &functions[45], view(harad_03_conditions)};
inline constexpr std::array<ItemAmount, 1> harad_03_costs{{
    {16611, 1},
}};
inline constexpr std::array<ItemAmount, 1> harad_03_outputs{{
    {16497, 1},
}};
inline constexpr std::array<MaterialEffect, 2> harad_03_effects{{
    {MaterialAction::remove, 16611, 1, 0},
    {MaterialAction::create, 16497, 1, 0},
}};
inline constexpr std::array<OperandUse, 3> harad_03_operand_uses{{
    {1, AmountRole::minimum_inventory, 16611},
    {37, AmountRole::remove_count, 16611},
    {57, AmountRole::create_count, 16497},
}};
inline constexpr std::array<const Function*, 4> harad_03_items{{
    &functions[7],
    &functions[5],
    &functions[8],
    &functions[1],
}};
inline constexpr std::array<Effect, 7> harad_03_success{{
    {EffectKind::minimum, 15, 160, 16611, 1, Actor::self},
    {EffectKind::remove, 41, 184, 16611, 1, Actor::hero},
    {EffectKind::create, 61, 170, 16497, 1, Actor::hero},
    {EffectKind::print, 71, 12, 14370, 0, Actor::hero},
    {EffectKind::say, 91, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 131, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 146, 0, 10552, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 5> harad_03_failure{{
    {EffectKind::minimum, 15, 160, 16611, 1, Actor::self},
    {EffectKind::print, 106, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 126, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 131, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 146, 0, 10552, 0, Actor::hero},
}};

inline constexpr std::array<PackedInventoryTarget, 4> harad_04_targets{{
    {"ITMISWORDRAWHOT", 16611},
    {"ITMW_ELBASTARDO", 16512},
    {"ITMI_PLIERS_SMLT", 16666},
    {"ITMW_1H_MACE_L_04", 16435},
}};
inline constexpr std::array<SymbolBinding, 17> harad_04_own_bindings{{
    {"ITMISWORDRAWHOT", 16611, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMW_ELBASTARDO", 16512, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_PLIERS_SMLT", 16666, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMW_1H_MACE_L_04", 16435, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_WEAPON_1H_HARAD_04", 20737, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_SMITHWEAPON_END", 20698, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_COMMON", 20701, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_ORE", 20704, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_COMMONBACK", 20707, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_OREBACK", 20710, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_SMITH", 1496, 2, 0, 30, -1, false, 0, false, 0},
    {"NORMALWAFFEN", 10552, 2, 0, 1, -1, false, 0, false, 0},
    {"ERZWAFFEN", 10551, 2, 0, 1, -1, false, 0, false, 0},
    {"STARTSMITHING", 10536, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_SMITHWEAPON", 12311, 2, 1, 1, -1, true, 1, false, 0},
    {"PRINT_SMITHSUCCESS", 14370, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto harad_04_bindings = with_common(harad_04_own_bindings);
inline constexpr std::array<Requirement, 3> harad_04_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 12, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
}};
inline constexpr std::array<Requirement, 5> harad_04_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 12, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::context, 10536, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16611, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 5> harad_04_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 1},
     CheckSite::owner_condition,
     &functions[50]},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 1496, 12, 1},
     CheckSite::owner_condition,
     &functions[50]},
    {{RequirementKind::integer_equal, RequirementRole::context, 10552, 0, 1},
     CheckSite::owner_condition,
     &functions[50]},
    {{RequirementKind::integer_equal, RequirementRole::context, 10536, 0, 1},
     CheckSite::mob_condition,
     &functions[56]},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16611, 0, 1},
     CheckSite::callback,
     &functions[53]},
}};
inline constexpr Owner harad_04_owner = {20737,          -1,
                                         22243,          false,
                                         false,          true,
                                         &functions[49], &functions[50],
                                         &functions[51], view(harad_04_conditions)};
inline constexpr std::array<ItemAmount, 1> harad_04_costs{{
    {16611, 1},
}};
inline constexpr std::array<ItemAmount, 1> harad_04_outputs{{
    {16512, 1},
}};
inline constexpr std::array<MaterialEffect, 2> harad_04_effects{{
    {MaterialAction::remove, 16611, 1, 0},
    {MaterialAction::create, 16512, 1, 0},
}};
inline constexpr std::array<OperandUse, 3> harad_04_operand_uses{{
    {1, AmountRole::minimum_inventory, 16611},
    {37, AmountRole::remove_count, 16611},
    {57, AmountRole::create_count, 16512},
}};
inline constexpr std::array<const Function*, 4> harad_04_items{{
    &functions[7],
    &functions[6],
    &functions[8],
    &functions[1],
}};
inline constexpr std::array<Effect, 7> harad_04_success{{
    {EffectKind::minimum, 15, 160, 16611, 1, Actor::self},
    {EffectKind::remove, 41, 184, 16611, 1, Actor::hero},
    {EffectKind::create, 61, 170, 16512, 1, Actor::hero},
    {EffectKind::print, 71, 12, 14370, 0, Actor::hero},
    {EffectKind::say, 91, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 131, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 146, 0, 10552, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 5> harad_04_failure{{
    {EffectKind::minimum, 15, 160, 16611, 1, Actor::self},
    {EffectKind::print, 106, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 126, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 131, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 146, 0, 10552, 0, Actor::hero},
}};

inline constexpr std::array<Descriptor, 5> recipes{{
    {RecipeId::common_sword,
     "Schwert",
     Station::anvil,
     1,
     view(common_sword_targets),
     view(common_sword_bindings),
     view(common_sword_requirements),
     view(common_sword_checks),
     common_sword_owner,
     &functions[28],
     {&functions[29],
      20713,
      Invocation::choice,
      1,
      1,
      INT32_MAX,
      {148, functions[29].body_sha256, operands.data(), operands.size()},
      view(common_sword_effects),
      {},
      true,
      true,
      false,
      true},
     {&functions[30], 3},
     view(common_sword_costs),
     view(common_sword_outputs),
     anvil_hand,
     Completion::end_production,
     &station,
     view(common_sword_operand_uses),
     Actor::self,
     Actor::hero,
     true,
     view(common_sword_success),
     view(common_sword_failure),
     view(prints),
     view(common_sword_items),
     view(native_dependencies)},
    {RecipeId::harad_01,
     "Edles Schwert",
     Station::anvil,
     1,
     view(harad_01_targets),
     view(harad_01_bindings),
     view(harad_01_requirements),
     view(harad_01_checks),
     harad_01_owner,
     &functions[34],
     {&functions[35],
      20719,
      Invocation::choice,
      1,
      1,
      INT32_MAX,
      {148, functions[35].body_sha256, operands.data(), operands.size()},
      view(harad_01_effects),
      {},
      true,
      true,
      false,
      true},
     {&functions[36], 3},
     view(harad_01_costs),
     view(harad_01_outputs),
     anvil_hand,
     Completion::end_production,
     &station,
     view(harad_01_operand_uses),
     Actor::self,
     Actor::hero,
     true,
     view(harad_01_success),
     view(harad_01_failure),
     view(prints),
     view(harad_01_items),
     view(native_dependencies)},
    {RecipeId::harad_02,
     "Edles Langschwert",
     Station::anvil,
     1,
     view(harad_02_targets),
     view(harad_02_bindings),
     view(harad_02_requirements),
     view(harad_02_checks),
     harad_02_owner,
     &functions[40],
     {&functions[41],
      20725,
      Invocation::choice,
      1,
      1,
      INT32_MAX,
      {148, functions[41].body_sha256, operands.data(), operands.size()},
      view(harad_02_effects),
      {},
      true,
      true,
      false,
      true},
     {&functions[42], 3},
     view(harad_02_costs),
     view(harad_02_outputs),
     anvil_hand,
     Completion::end_production,
     &station,
     view(harad_02_operand_uses),
     Actor::self,
     Actor::hero,
     true,
     view(harad_02_success),
     view(harad_02_failure),
     view(prints),
     view(harad_02_items),
     view(native_dependencies)},
    {RecipeId::harad_03,
     "Rubinklinge",
     Station::anvil,
     1,
     view(harad_03_targets),
     view(harad_03_bindings),
     view(harad_03_requirements),
     view(harad_03_checks),
     harad_03_owner,
     &functions[46],
     {&functions[47],
      20731,
      Invocation::choice,
      1,
      1,
      INT32_MAX,
      {148, functions[47].body_sha256, operands.data(), operands.size()},
      view(harad_03_effects),
      {},
      true,
      true,
      false,
      true},
     {&functions[48], 3},
     view(harad_03_costs),
     view(harad_03_outputs),
     anvil_hand,
     Completion::end_production,
     &station,
     view(harad_03_operand_uses),
     Actor::self,
     Actor::hero,
     true,
     view(harad_03_success),
     view(harad_03_failure),
     view(prints),
     view(harad_03_items),
     view(native_dependencies)},
    {RecipeId::harad_04,
     "El Bastardo",
     Station::anvil,
     1,
     view(harad_04_targets),
     view(harad_04_bindings),
     view(harad_04_requirements),
     view(harad_04_checks),
     harad_04_owner,
     &functions[52],
     {&functions[53],
      20737,
      Invocation::choice,
      1,
      1,
      INT32_MAX,
      {148, functions[53].body_sha256, operands.data(), operands.size()},
      view(harad_04_effects),
      {},
      true,
      true,
      false,
      true},
     {&functions[54], 3},
     view(harad_04_costs),
     view(harad_04_outputs),
     anvil_hand,
     Completion::end_production,
     &station,
     view(harad_04_operand_uses),
     Actor::self,
     Actor::hero,
     true,
     view(harad_04_success),
     view(harad_04_failure),
     view(prints),
     view(harad_04_items),
     view(native_dependencies)},
}};
inline constexpr std::size_t catalog_target_capacity = []() constexpr {
  std::size_t n = 0;
  for (const auto& r : recipes)
    if (r.targets.count > n)
      n = r.targets.count;
  return n;
}();
inline const Descriptor* find(RecipeId id) noexcept {
  for (const auto& r : recipes)
    if (r.id == id)
      return &r;
  return nullptr;
}
inline const Descriptor* find_callback(std::uint32_t symbol) noexcept {
  for (const auto& r : recipes)
    if (r.callback.function->symbol == symbol)
      return &r;
  return nullptr;
}
inline const Function* function_for(std::uint32_t symbol) noexcept {
  for (const auto& f : functions)
    if (f.symbol == symbol)
      return &f;
  return nullptr;
}
inline const SymbolBinding* binding_for(const Descriptor& r, std::uint32_t symbol) noexcept {
  if (!r.bindings.data || r.bindings.count > 64)
    return nullptr;
  for (const auto& b : r.bindings)
    if (b.symbol == symbol)
      return &b;
  return nullptr;
}
namespace detail {
using station_recipe_catalog::detail::same_binding;
using station_recipe_catalog::detail::same_function;
using station_recipe_catalog::detail::same_requirement;
using station_recipe_catalog::detail::text;
template <class T, class Equal> inline bool same(View<T> a, View<T> b, Equal equal) noexcept {
  if (a.count != b.count || (a.count && !a.data))
    return false;
  for (std::size_t i = 0; i < a.count; ++i)
    if (!equal(a[i], b[i]))
      return false;
  return true;
}
inline bool owner(const Owner& a, const Owner& b) noexcept {
  return a.instance == b.instance && a.number == b.number && a.npc == b.npc &&
         a.important == b.important && a.trade == b.trade && a.permanent == b.permanent &&
         same_function(a.constructor, b.constructor) && same_function(a.condition, b.condition) &&
         same_function(a.information, b.information) &&
         same(a.conditions, b.conditions, same_requirement);
}
inline bool hand(const HandContract& a, const HandContract& b) noexcept {
  return station_recipe_catalog::detail::hand_item(a.left, b.left) &&
         station_recipe_catalog::detail::hand_item(a.right, b.right) && a.anchor == b.anchor &&
         a.static_lifecycle_verified == b.static_lifecycle_verified &&
         a.observed_in_world == b.observed_in_world && a.left_tmp_level == b.left_tmp_level &&
         a.right_tmp_level == b.right_tmp_level;
}
inline bool station(const StationProfile* a, const StationProfile* b) noexcept {
  return a && b && a->station == b->station && a->production_context == b->production_context &&
         text(a->on_state, b->on_state) && text(a->scheme, b->scheme) &&
         text(a->visual, b->visual) && text(a->use_with_item, b->use_with_item) &&
         text(a->condition_name, b->condition_name) && text(a->vob_name, b->vob_name) &&
         a->state_num == b->state_num && a->rewind == b->rewind &&
         same_function(a->entry, b->entry) && same_function(a->mob_condition, b->mob_condition) &&
         owner(a->end, b->end);
}
inline bool callback(const Callback& a, const Callback& b) noexcept {
  if (!same_function(a.function, b.function) || a.owner_instance != b.owner_instance ||
      a.invocation != b.invocation || a.original_quantity != b.original_quantity ||
      a.minimum_quantity != b.minimum_quantity || a.maximum_quantity != b.maximum_quantity ||
      a.body_checks_resources != b.body_checks_resources ||
      a.ends_production != b.ends_production || a.clears_choices != b.clears_choices ||
      a.quantity_text_reviewed != b.quantity_text_reviewed || a.amount.length != b.amount.length ||
      a.amount.digest != b.amount.digest || a.failure_effects.count)
    return false;
  if (!same(View<AmountOperand>{a.amount.operands, a.amount.count},
            View<AmountOperand>{b.amount.operands, b.amount.count},
            [](const auto& x, const auto& y) {
              return x.offset == y.offset && x.original == y.original && x.units == y.units &&
                     x.reserved == y.reserved && x.allow_zero == y.allow_zero;
            }))
    return false;
  return same(a.effects, b.effects, [](const auto& x, const auto& y) {
    return x.action == y.action && x.symbol == y.symbol && x.units == y.units &&
           x.reserved == y.reserved;
  });
}
inline bool effect(const Effect& a, const Effect& b) noexcept {
  return a.kind == b.kind && a.offset == b.offset && a.callee == b.callee && a.symbol == b.symbol &&
         a.value == b.value && a.actor == b.actor;
}
} // namespace detail

// Exclusively own valid C++ memory regions, no foreign pointers. Each copy
// remains bound to exactly one canonical family; in particular no reinterpretation
// of an item/flag operand to a quantity field, no _3 replacement effect,
// no invented learning and no permission solely from static hand data.
inline bool structurally_valid(const Descriptor& r) noexcept {
  namespace d = detail;
  const auto* c = find(r.id);
  if (!c)
    return false;
  if (!d::text(r.name, c->name) || r.station != c->station ||
      r.production_context != c->production_context || r.completion != c->completion ||
      r.minimum_actor != c->minimum_actor || r.material_actor != c->material_actor ||
      r.require_self_is_hero != c->require_self_is_hero || !d::owner(r.owner, c->owner) ||
      !d::same_function(r.back, c->back) || !d::callback(r.callback, c->callback) ||
      !d::same_function(r.original_three.function, c->original_three.function) ||
      r.original_three.quantity != c->original_three.quantity || !d::hand(r.hand, c->hand) ||
      !d::station(r.station_profile, c->station_profile))
    return false;
  if (!d::same(r.targets, c->targets,
               [](const auto& x, const auto& y) {
                 return x.symbol == y.symbol && d::text(x.name, y.name);
               }) ||
      !d::same(r.bindings, c->bindings, d::same_binding) ||
      !d::same(r.requirements, c->requirements, d::same_requirement) ||
      !d::same(r.checks, c->checks,
               [](const auto& x, const auto& y) {
                 return d::same_requirement(x.requirement, y.requirement) && x.site == y.site &&
                        d::same_function(x.function, y.function);
               }) ||
      !d::same(r.costs, c->costs,
               [](const auto& x, const auto& y) {
                 return x.symbol == y.symbol && x.units == y.units;
               }) ||
      !d::same(r.outputs, c->outputs,
               [](const auto& x, const auto& y) {
                 return x.symbol == y.symbol && x.units == y.units;
               }) ||
      !d::same(r.operand_uses, c->operand_uses,
               [](const auto& x, const auto& y) {
                 return x.offset == y.offset && x.role == y.role && x.symbol == y.symbol;
               }) ||
      !d::same(r.success, c->success, d::effect) || !d::same(r.failure, c->failure, d::effect) ||
      !d::same(r.prints, c->prints,
               [](const auto& x, const auto& y) {
                 return x.symbol == y.symbol && x.length == y.length && x.sha256 == y.sha256;
               }) ||
      !d::same(r.item_constructors, c->item_constructors,
               [](const auto* x, const auto* y) { return d::same_function(x, y); }) ||
      !d::same(r.native_dependencies, c->native_dependencies, [](const auto& x, const auto& y) {
        return d::text(x.name, y.name) && x.address == y.address && x.length == y.length &&
               x.sha256 == y.sha256;
      }))
    return false;
  return true;
}
struct QuantityPlan {
  const Callback* callback = nullptr;
  std::uint32_t quantity = 0;
  bool valid = false, patch_original = false;
};
inline QuantityPlan plan_quantity(const Descriptor& r, std::uint32_t quantity) noexcept {
  if (!quantity || quantity > INT32_MAX || !structurally_valid(r))
    return {};
  return {&r.callback, quantity, true, quantity > 1};
}
struct NumericLimit {
  bool valid = false;
  std::uint32_t maximum = 0;
};
// Only quantity arithmetic on already known, unique stored numbers.
// No knowledge, actor, hand, slot, inventory, or runtime permission.
inline NumericLimit numeric_limit(const Descriptor& r, std::int64_t raw,
                                  std::int64_t output) noexcept {
  if (!structurally_valid(r) || raw < 0 || raw > INT32_MAX || output < 0 || output > INT32_MAX)
    return {};
  const auto room = INT32_MAX - output;
  return {true, static_cast<std::uint32_t>(raw < room ? raw : room)};
}
} // namespace ubc::anvil_recipe_catalog
