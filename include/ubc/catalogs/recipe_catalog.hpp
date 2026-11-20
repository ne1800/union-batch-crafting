// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/engine/amount_patch.hpp"
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/engine/packed_inventory.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>

// Own static facts for L'Hiver DE 2.0.4, 01.11.2026. No original scripts/bytes.
// DAT-SHA: 71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4.
// These data activate nothing: native bindings, known recipes, current
// stock, hand/station variant, CallFunc lifetime and Restore are own gates.
namespace ubc::recipe_catalog {
using lhiver::digest_literal;
using lhiver::Function;
using lhiver::SymbolBinding;

template <class T> struct View {
  const T* data = nullptr;
  std::size_t count = 0;
  constexpr const T* begin() const noexcept { return data; }
  constexpr const T* end() const noexcept { return count ? data + count : data; }
  constexpr const T& operator[](std::size_t i) const noexcept { return data[i]; }
};
template <class T, std::size_t N> constexpr View<T> view(const std::array<T, N>& values) noexcept {
  return {values.data(), N};
}

enum class RecipeId {
  meat,
  health_01,
  health_02,
  health_03,
  mana_01,
  mana_02,
  mana_03,
  molerat_dish,
  mushroom_dish
};
enum class Station { stove, alchemy };
enum class HandEvidence { unknown, inferred_same_station, observed_original };
enum class RequirementKind { integer_equal, inventory_at_least };
enum class RequirementRole { context, knowledge, resource };
enum class Invocation { info, choice };
enum class MaterialAction { remove, create };
enum class Actor { hero, self };
struct Requirement {
  RequirementKind kind;
  RequirementRole role;
  std::int32_t symbol;
  std::uint32_t element;
  std::int32_t value;
};
struct ItemAmount {
  std::int32_t symbol;
  std::uint32_t units;
};
struct HandContract {
  std::int32_t start_item_symbol, active_item_symbol;
  std::uint32_t amount;
  bool from_inventory, consumed_on_success;
  std::uint32_t failure_refund;
  HandEvidence evidence;
  bool left_must_be_empty = true; // Active object always in the right hand.
};
struct MaterialEffect {
  MaterialAction action;
  std::int32_t symbol;
  std::uint32_t units, reserved; // Inventory effect = units*N-reserved; hand separate.
};
struct FixedEffect {
  MaterialAction action;
  std::int32_t symbol;
  std::uint32_t amount;
  Actor actor;
};
struct Owner {
  std::int32_t instance, number, npc;
  bool important, trade, permanent;
  const Function *constructor, *condition, *information;
  View<Requirement> conditions; // Exact AND conditions of C_INFO, no release.
};
struct Callback {
  const Function* function;
  std::int32_t owner_instance;
  Invocation invocation;
  std::uint32_t original_quantity;                  // 0 denotes only the original All-path.
  std::uint32_t minimum_quantity, maximum_quantity; // 0/0: no quantity template.
  AmountBody amount;
  View<MaterialEffect> effects;      // Original order, relative to N.
  View<FixedEffect> failure_effects; // Unchanged, never multiply by N.
  bool body_checks_resources, ends_production, clears_choices, quantity_text_reviewed;
};
struct Descriptor {
  RecipeId id;
  const char* name; // Own label, not a copied original dialog.
  Station station;
  std::int32_t production_context;
  View<PackedInventoryTarget> targets; // Fully per recipe, no global six-item cap.
  View<SymbolBinding> bindings;
  View<Requirement> requirements; // Shared context/learning conditions for all owners.
  View<Owner> owners;
  View<Callback> callbacks;
  View<ItemAmount> costs, outputs; // Total consumption per N, including hand reserve.
  HandContract hand;
  const Function *station_entry, *station_end;
};
inline constexpr bool runtime_ready = false;

// Shared native/script bindings are checked before a
// later integration alongside the recipe-related bindings.
// Dynamic values are not DAT initial-value checks.
inline constexpr std::array<SymbolBinding, 23> common_bindings{
    {{"HERO", 1683, 7, 1, 0, 1537},
     {"SELF", 1679, 7, 1, 0, 1537},
     {"OTHER", 1680, 7, 1, 0, 1537},
     {"PC_HERO", 22243, 7, 1, 0, 21074},
     {"C_NPC", 1537, 4, 0, 32, -1},
     {"C_ITEM", 1584, 4, 0, 44, -1},
     {"C_NPC.AIVAR", 1563, 2, 4, 100, 1537, false, 0, true, 648},
     {"AIV_INVINCIBLE", 11998, 2, 1, 1, -1, true, 4},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1},
     {"MOBSI_NONE", 12310, 2, 1, 1, -1, true, 0},
     {"MOBSI_CAMPFIRE", 12324, 2, 1, 1, -1, true, 14},
     {"MOBSI_POTIONALCHEMY", 12314, 2, 1, 1, -1, true, 4},
     {"TRUE", 12297, 2, 1, 1, -1, true, 1},
     {"FALSE", 12298, 2, 1, 1, -1, true, 0},
     {"C_INFO", 1649, 4, 0, 8, -1},
     {"C_INFO.NPC", 1650, 2, 4, 1, 1649},
     {"C_INFO.NR", 1651, 2, 4, 1, 1649},
     {"C_INFO.IMPORTANT", 1652, 2, 4, 1, 1649},
     {"C_INFO.CONDITION", 1653, 5, 4, 1, 1649},
     {"C_INFO.INFORMATION", 1654, 5, 4, 1, 1649},
     {"C_INFO.DESCRIPTION", 1655, 3, 4, 1, 1649},
     {"C_INFO.TRADE", 1656, 2, 4, 1, 1649},
     {"C_INFO.PERMANENT", 1657, 2, 4, 1, 1649}}};

inline constexpr std::array<Function, 63> functions{{
    {"B_SAY", 14646, 218446, 39,
     digest_literal("b8b93bf2920074b9958b398d5d642935220402bd20ff5044c51f9a416675d6ad"), 3, 5, 1, 0,
     -1},
    {"B_ENDPRODUCTIONDIALOG", 19965, 672156, 49,
     digest_literal("e9fcd2f7db76bab4ed7c5ceb6e5a98c3ab3b8166a3614a4dda2513092da34e64"), 0, 5, 1, 0,
     -1},
    {"POTIONALCHEMY_S1", 20276, 723073, 65,
     digest_literal("3ee4af5cf7c6c967d039c7585d9f4a085da7a638ee43ee02fe0c142c1f8d00ab"), 0, 5, 1, 0,
     -1},
    {"PC_POTIONALCHEMY_END_INFO", 20279, 723288, 26,
     digest_literal("6a30e7f39ca34bd02e28e4e2be7c2932e1ddf2ba6c0eb69fa37542c3b74a0262"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_01", 20306, 725644, 67,
     digest_literal("434c19848a709204a8e292f786cc262674028748a670481b09e20d994206ee37"), 0, 7, 1, 0,
     1649},
    {"PC_ITPO_MANA_01_CONDITION", 20307, 725711, 48,
     digest_literal("a60a0c3e3fe0d1fabbc44aa8c07668e480bb50429ffc5302c133180e64a80087"), 0, 5, 3, 2,
     -1},
    {"PC_ITPO_MANA_01_INFO", 20308, 725759, 91,
     digest_literal("0313457537654cd5a817b3f85a15b698752a6f2587281e2a9920bff8bb4d247e"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_01_1", 20310, 725861, 199,
     digest_literal("6903cf56fef2e95fcd40137451a5456e2561e7eb288bd14762b7b4351db26199"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_01_3", 20311, 726060, 219,
     digest_literal("8b22abd727e1ab26e8264ae1962ab10bfecfac2601561ba4d3f31faa04cdb139"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_01_5", 20312, 726279, 219,
     digest_literal("96dd9b61b821b92c90a1dd0b188cce0d7fde017c85ecedabb275056a15ff5479"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_02", 20313, 726498, 67,
     digest_literal("6a877bd0614e275554271fc7f43fe6dc7823ba148b9d38c0ad10381b6c64fcbf"), 0, 7, 1, 0,
     1649},
    {"PC_ITPO_MANA_02_CONDITION", 20314, 726565, 48,
     digest_literal("caf79eb888d61cdb4098c3d4211fea767fdc4086ea9309683cb03245d62ed05c"), 0, 5, 3, 2,
     -1},
    {"PC_ITPO_MANA_02_INFO", 20315, 726613, 91,
     digest_literal("91b7ccb010cfa953c7357b62dc586298cb0481048dd9ed302a09a6e5dca21666"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_02_1", 20317, 726715, 199,
     digest_literal("c622ad2e1755ce30b73cf9ed44a3bf96b49aa5f290994e4f56ad0c7185538bf0"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_02_3", 20318, 726914, 219,
     digest_literal("417bb5a29ef487188c170671a92680744ca88aff2604f5bb498dcc48eb35398c"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_02_5", 20319, 727133, 219,
     digest_literal("5e09d010b04c8e7defe9ef679181d13374f47e034e4c5045e4d39871a477212a"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_03", 20320, 727352, 67,
     digest_literal("371e1ea38e9d8869d88aebeedfbc846fbe5de0390814197f3fd4294fccd0cbfd"), 0, 7, 1, 0,
     1649},
    {"PC_ITPO_MANA_03_CONDITION", 20321, 727419, 48,
     digest_literal("c806863e79b3af8b83ad8bb03c6a426fd83e8c5ca14c849a506dcaec62d8343b"), 0, 5, 3, 2,
     -1},
    {"PC_ITPO_MANA_03_INFO", 20322, 727467, 91,
     digest_literal("2c50e400a2ed8de2cd96a531b382b320806257d5eb1196f42b163b02459d2bb9"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_03_1", 20324, 727569, 199,
     digest_literal("942f2b3f76b5dd9e71378403b0245e7873091e9059089fb52827f4b52c2ebbe6"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_03_3", 20325, 727768, 219,
     digest_literal("c7a51694d23ad2e01ef8473da8a631c39c0946fce6e1d947c2f554ae57bf0ed2"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_03_5", 20326, 727987, 219,
     digest_literal("4b49de802b40c4d49af84eb2566672ce97b3c5c22464bd07be40c3481a103f91"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_01", 20337, 728796, 67,
     digest_literal("f7bae58e8348b276afd8b39a04f9b9f3832bc8a14ae11d86318afba6d71fc565"), 0, 7, 1, 0,
     1649},
    {"PC_ITPO_HEALTH_01_CONDITION", 20338, 728863, 47,
     digest_literal("7210a1a44b9deb74fed19bcdc698b5ce3e35cf1149091d769c48994a1955333c"), 0, 5, 3, 2,
     -1},
    {"PC_ITPO_HEALTH_01_INFO", 20339, 728910, 91,
     digest_literal("a1f2394a7cba7208cff17bfbac18a1c0d0208e97c78997b035b715c270cd2b1b"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_01_1", 20341, 729012, 199,
     digest_literal("2c7e223e790509502e65b00f4781be32e48105aedf4e9bc4518ee5918e4f0625"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_01_3", 20342, 729211, 219,
     digest_literal("45f0edd07cd827c4e0fc9ffe09c6f70b6a7ef64d06e976af787f3e60697055ef"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_01_5", 20343, 729430, 219,
     digest_literal("37d19a0819ff7a81e56d58ac2512f04aefb6d74851bc439888eb430a49994394"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_02", 20344, 729649, 67,
     digest_literal("1f6d173df5bc4b155f1ecf0003f185b6e1fe27319f47b928e2ca00161679c7c9"), 0, 7, 1, 0,
     1649},
    {"PC_ITPO_HEALTH_02_CONDITION", 20345, 729716, 48,
     digest_literal("f87e77964cc692d93584025eb547d70238e90daa71d6002334bd438c9c3bbf00"), 0, 5, 3, 2,
     -1},
    {"PC_ITPO_HEALTH_02_INFO", 20346, 729764, 91,
     digest_literal("739f1b4f37cb9a7beb3460762fccd8e815abe7cdad1ae59721baee50be83656e"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_02_1", 20348, 729866, 199,
     digest_literal("c896616c7bf1139249505dd36b3929a7a90f0544db87346649af15a7e2ba4898"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_02_3", 20349, 730065, 219,
     digest_literal("19f338c113ed163472fff540f45d53b9be018e6adabdeaeb1e50c8c389bffeb7"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_02_5", 20350, 730284, 219,
     digest_literal("847d6c9af6c7a3b5bee4374a7f590079c4f1d2b7422a4f43ed35e23070cd8dd7"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_03", 20351, 730503, 67,
     digest_literal("09f892e24e45134dfb303375374b9ad5478d1d987e4d0c169dd09305acccda2e"), 0, 7, 1, 0,
     1649},
    {"PC_ITPO_HEALTH_03_CONDITION", 20352, 730570, 48,
     digest_literal("c0c2cf29937f9c7b0bc3cd1309008c741c547f70fcb5abf102eb55005fb99bdc"), 0, 5, 3, 2,
     -1},
    {"PC_ITPO_HEALTH_03_INFO", 20353, 730618, 91,
     digest_literal("e1a0db64a0157aaadddd49b53e912a8c150319fcf774db538c89d66ca9ec41fe"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_03_1", 20355, 730720, 199,
     digest_literal("e143e73c5c89521b6747c6e23b9b92c474272e73afd3d3f1df8f2ec77101a0fc"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_03_3", 20356, 730919, 219,
     digest_literal("670dd8db5f54a32a169664b249c122ff9a0be10355cb0cbb4a51bb9fca781ed9"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_03_5", 20357, 731138, 219,
     digest_literal("f2280306f40850703ec347abea1758c5882f57a5cbfe56ef2d0f8c76477c964c"), 0, 5, 1, 0,
     -1},
    {"FRYINGMEAT_S1", 21007, 825104, 65,
     digest_literal("755218d948763e83dd0668e19a1adcfb5669c737a40b537fe4d6058d8295b647"), 0, 5, 1, 0,
     -1},
    {"PC_FRYINGMEAT_ADDON", 21008, 825169, 67,
     digest_literal("02271e8965c080662a14ca2f002248f9b79f49b5c93dc3172e0b1d5fede5a550"), 0, 7, 1, 0,
     1649},
    {"PC_FRYINGMEAT_ADDON_CONDITION", 21009, 825236, 45,
     digest_literal("2344beb9de819f9071cc97be8ceafed539b2f1771784bd8fd27f64c07b3515ec"), 0, 5, 3, 2,
     -1},
    {"PC_FRYINGMEAT_ADDON_INFO", 21010, 825281, 107,
     digest_literal("46429f2bf11e60c85e3012e1ba180e3c3eddbfbcc8001228e803c11186e088bc"), 0, 5, 1, 0,
     -1},
    {"PC_FRYINGMEAT_ADDON_X5", 21011, 825388, 67,
     digest_literal("2c83d72db6b7294da684007a97c2e18a033adab9577a1fff0887e7f671f69ff8"), 0, 7, 1, 0,
     1649},
    {"PC_FRYINGMEAT_ADDON_X5_CONDITION", 21012, 825455, 45,
     digest_literal("d837c8b76e1c4f085ec2a7ceb8f23878a34f3e5538c86eb41e6dc47ca5604b11"), 0, 5, 3, 2,
     -1},
    {"PC_FRYINGMEAT_ADDON_X5_INFO", 21013, 825500, 107,
     digest_literal("2082c66ed4d448c9ade3c8028a7431aa92608ab3097ec2b5d1d8f2502837737b"), 0, 5, 1, 0,
     -1},
    {"PC_FRYINGMEAT_ADDON_X10", 21014, 825607, 67,
     digest_literal("5453970cfccd3ef1717f19fcd5cfec482193459f51091577fab06b7a2e50e35b"), 0, 7, 1, 0,
     1649},
    {"PC_FRYINGMEAT_ADDON_X10_CONDITION", 21015, 825674, 45,
     digest_literal("ea66faf52228bb5433e751cd05a3011e18114acf9c5d42be04256ab622cb3990"), 0, 5, 3, 2,
     -1},
    {"PC_FRYINGMEAT_ADDON_X10_INFO", 21016, 825719, 107,
     digest_literal("faf66ccf2ccb0d4974f3fbe03ece78112117e15c6a36df690937cd59daac2b85"), 0, 5, 1, 0,
     -1},
    {"PC_FRYINGMEAT_ADDON_ALL", 21017, 825826, 67,
     digest_literal("5199aee5d218f3d101427837dcfcb8ae6be7003110760270b1e60042d4529e8a"), 0, 7, 1, 0,
     1649},
    {"PC_FRYINGMEAT_ADDON_ALL_CONDITION", 21018, 825893, 45,
     digest_literal("c800a423c37003ed0079b476d20f02168aba067c180a8ee2e8a06e8c9ba5f994"), 0, 5, 3, 2,
     -1},
    {"PC_FRYINGMEAT_ADDON_ALL_INFO", 21019, 825938, 117,
     digest_literal("96f2ac3d3bfd65b50f8078ff216b7cbb15b735054f14e33b8ba6e5cb4477b221"), 0, 5, 1, 0,
     -1},
    {"PC_FRYINGMEAT_MOLERATKEULE", 21021, 826055, 67,
     digest_literal("ed7f1a2a40d1672c78df7f46e2e58e933f2e01385a777654b4bf3ca451b1ae9d"), 0, 7, 1, 0,
     1649},
    {"PC_FRYINGMEAT_MOLERATKEULE_CONDITION", 21022, 826122, 101,
     digest_literal("f7dfe2d763569eccb98987f45e75405a74ccd5bd067ff7cd907e3e4858d7eab9"), 0, 5, 3, 2,
     -1},
    {"PC_FRYINGMEAT_MOLERATKEULE_INFO", 21023, 826223, 121,
     digest_literal("19fd87910afd87575694dadf8ee284bddddb4fb6c3215ce319d402894b72ed90"), 0, 5, 1, 0,
     -1},
    {"PC_FRYINGMEAT_MOLERATKEULE_2", 21024, 826344, 67,
     digest_literal("4c01576a3bb6f517046532a20ade7b09bcb424fb0a8f5090d7460845174fd4ae"), 0, 7, 1, 0,
     1649},
    {"PC_FRYINGMEAT_MOLERATKEULE_2_CONDITION", 21025, 826411, 101,
     digest_literal("fe15bbee1d1e82bbe8c34e8a585a73ac44f57dfc278df8f137a688bad09e15b7"), 0, 5, 3, 2,
     -1},
    {"PC_FRYINGMEAT_MOLERATKEULE_2_INFO", 21026, 826512, 121,
     digest_literal("c1ea67c047961f3ebc1933041694776e3db480c84ac4238a1e29fae0481a4aec"), 0, 5, 1, 0,
     -1},
    {"PC_FRYINGMEAT_PILZBRATEN", 21027, 826633, 67,
     digest_literal("0a9aeb71e352aabd75603c42c88d51b8e1870bcc4925a070a025885b0376aa83"), 0, 7, 1, 0,
     1649},
    {"PC_FRYINGMEAT_PILZBRATEN_CONDITION", 21028, 826700, 45,
     digest_literal("96c443a502759e84903de6f8252f4fc714ec773bfeac66556714691c83061fe4"), 0, 5, 3, 2,
     -1},
    {"PC_FRYINGMEAT_PILZBRATEN_INFO", 21029, 826745, 81,
     digest_literal("ed69890b044c5fd601758641d520d659ecc07392ce35f6b332330a2187b8be45"), 0, 5, 1, 0,
     -1},
    {"PC_FRYINGMEAT_END_INFO", 21032, 826916, 6,
     digest_literal("88e225fb843204b57529955367a14efd2314c9788ac29aff28032b3a2a8cd982"), 0, 5, 1, 0,
     -1},
}};

inline constexpr std::array<FixedEffect, 1> flask_failure{
    {{MaterialAction::create, 16604, 1, Actor::self}}};

inline constexpr std::array<ItemAmount, 1> meat_costs{{{16377, 1}}};
inline constexpr std::array<ItemAmount, 1> meat_outputs{{{16379, 1}}};
inline constexpr std::array<PackedInventoryTarget, 4> meat_targets{{{"ITFOMUTTONRAW", 16377},
                                                                    {"ITFOMUTTON", 16379},
                                                                    {"ITMI_PAN", 16607},
                                                                    {"ITMI_PANFULL", 16608}}};
inline constexpr std::array<Requirement, 1> meat_requirements{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 14}}};
inline constexpr std::array<SymbolBinding, 9> meat_bindings{
    {{"ITFOMUTTONRAW", 16377, 7, 1, 0, 1584},
     {"ITFOMUTTON", 16379, 7, 1, 0, 1584},
     {"ITMI_PAN", 16607, 7, 1, 0, 1584},
     {"ITMI_PANFULL", 16608, 7, 1, 0, 1584},
     {"PC_FRYINGMEAT_ADDON", 21008, 7, 1, 0, 1649},
     {"PC_FRYINGMEAT_ADDON_X5", 21011, 7, 1, 0, 1649},
     {"PC_FRYINGMEAT_ADDON_X10", 21014, 7, 1, 0, 1649},
     {"PC_FRYINGMEAT_ADDON_ALL", 21017, 7, 1, 0, 1649},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1}}};
inline constexpr std::array<Requirement, 2> owner_21008_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 14},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16377, 0, 1}}};
inline constexpr std::array<Requirement, 2> owner_21011_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 14},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16377, 0, 5}}};
inline constexpr std::array<Requirement, 2> owner_21014_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 14},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16377, 0, 10}}};
inline constexpr std::array<Requirement, 2> owner_21017_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 14},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16377, 0, 1}}};
inline constexpr std::array<Owner, 4> meat_owners{{
    {21008, 1, 22243, false, false, true, &functions[41], &functions[42], &functions[43],
     view(owner_21008_conditions)},
    {21011, 2, 22243, false, false, true, &functions[44], &functions[45], &functions[46],
     view(owner_21011_conditions)},
    {21014, 3, 22243, false, false, true, &functions[47], &functions[48], &functions[49],
     view(owner_21014_conditions)},
    {21017, 4, 22243, false, false, true, &functions[50], &functions[51], &functions[52],
     view(owner_21017_conditions)},
}};
inline constexpr std::array<AmountOperand, 3> operands_21010{
    {{1, 1, 1, 0}, {37, 1, 1, 0}, {57, 1, 1, 0}}};
inline constexpr std::array<MaterialEffect, 2> effects_21010{
    {{MaterialAction::create, 16379, 1, 0}, {MaterialAction::remove, 16377, 1, 0}}};
inline constexpr std::array<MaterialEffect, 2> effects_21013{
    {{MaterialAction::create, 16379, 1, 0}, {MaterialAction::remove, 16377, 1, 0}}};
inline constexpr std::array<MaterialEffect, 2> effects_21016{
    {{MaterialAction::create, 16379, 1, 0}, {MaterialAction::remove, 16377, 1, 0}}};
inline constexpr std::array<MaterialEffect, 2> effects_21019{
    {{MaterialAction::remove, 16377, 1, 0}, {MaterialAction::create, 16379, 1, 0}}};
inline constexpr std::array<Callback, 4> meat_callbacks{{
    {&functions[43],
     21008,
     Invocation::info,
     1,
     1,
     2147483647,
     {functions[43].byte_length, functions[43].body_sha256, operands_21010.data(), 3},
     view(effects_21010),
     {},
     true,
     false,
     true,
     true},
    {&functions[46],
     21011,
     Invocation::info,
     5,
     0,
     0,
     {functions[46].byte_length, functions[46].body_sha256, nullptr, 0},
     view(effects_21013),
     {},
     true,
     false,
     true,
     false},
    {&functions[49],
     21014,
     Invocation::info,
     10,
     0,
     0,
     {functions[49].byte_length, functions[49].body_sha256, nullptr, 0},
     view(effects_21016),
     {},
     true,
     false,
     true,
     false},
    {&functions[52],
     21017,
     Invocation::info,
     0,
     0,
     0,
     {functions[52].byte_length, functions[52].body_sha256, nullptr, 0},
     view(effects_21019),
     {},
     false,
     false,
     true,
     false},
}};

inline constexpr std::array<ItemAmount, 3> health_01_costs{{{16714, 1}, {16736, 1}, {16604, 1}}};
inline constexpr std::array<ItemAmount, 1> health_01_outputs{{{16775, 1}}};
inline constexpr std::array<PackedInventoryTarget, 4> health_01_targets{
    {{"ITPL_HEALTH_HERB_01", 16714},
     {"ITPL_TEMP_HERB", 16736},
     {"ITMI_FLASK", 16604},
     {"ITPO_HEALTH_01", 16775}}};
inline constexpr std::array<Requirement, 3> health_01_requirements{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 0, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20330, 0, 1}}};
inline constexpr std::array<SymbolBinding, 9> health_01_bindings{
    {{"ITPL_HEALTH_HERB_01", 16714, 7, 1, 0, 1584},
     {"ITPL_TEMP_HERB", 16736, 7, 1, 0, 1584},
     {"ITMI_FLASK", 16604, 7, 1, 0, 1584},
     {"ITPO_HEALTH_01", 16775, 7, 1, 0, 1584},
     {"PC_ITPO_HEALTH_01", 20337, 7, 1, 0, 1649},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1},
     {"PLAYER_TALENT_ALCHEMY", 1464, 2, 0, 15, -1},
     {"HEALTHSTART", 20330, 2, 0, 1, -1},
     {"POTION_HEALTH_01", 1448, 2, 1, 1, -1, true, 0}}};
inline constexpr std::array<Requirement, 3> owner_20337_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 0, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20330, 0, 1}}};
inline constexpr std::array<Owner, 1> health_01_owners{{
    {20337, 2, 22243, false, false, true, &functions[22], &functions[23], &functions[24],
     view(owner_20337_conditions)},
}};
inline constexpr std::array<MaterialEffect, 3> effects_20341{
    {{MaterialAction::remove, 16714, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16775, 1, 0}}};
inline constexpr std::array<MaterialEffect, 4> effects_20342{
    {{MaterialAction::remove, 16714, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16775, 1, 0},
     {MaterialAction::remove, 16604, 1, 1}}};
inline constexpr std::array<AmountOperand, 6> operands_20343{
    {{1, 5, 1, 0}, {22, 5, 1, 0}, {59, 5, 1, 0}, {79, 5, 1, 0}, {99, 5, 1, 0}, {119, 4, 1, 1}}};
inline constexpr std::array<MaterialEffect, 4> effects_20343{
    {{MaterialAction::remove, 16714, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16775, 1, 0},
     {MaterialAction::remove, 16604, 1, 1}}};
inline constexpr std::array<Callback, 3> health_01_callbacks{{
    {&functions[25],
     20337,
     Invocation::choice,
     1,
     1,
     1,
     {functions[25].byte_length, functions[25].body_sha256, nullptr, 0},
     view(effects_20341),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[26],
     20337,
     Invocation::choice,
     3,
     0,
     0,
     {functions[26].byte_length, functions[26].body_sha256, nullptr, 0},
     view(effects_20342),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[27],
     20337,
     Invocation::choice,
     5,
     2,
     2147483647,
     {functions[27].byte_length, functions[27].body_sha256, operands_20343.data(), 6},
     view(effects_20343),
     view(flask_failure),
     true,
     true,
     false,
     true},
}};

inline constexpr std::array<ItemAmount, 3> health_02_costs{{{16716, 1}, {16736, 1}, {16604, 1}}};
inline constexpr std::array<ItemAmount, 1> health_02_outputs{{{16778, 1}}};
inline constexpr std::array<PackedInventoryTarget, 4> health_02_targets{
    {{"ITPL_HEALTH_HERB_02", 16716},
     {"ITPL_TEMP_HERB", 16736},
     {"ITMI_FLASK", 16604},
     {"ITPO_HEALTH_02", 16778}}};
inline constexpr std::array<Requirement, 3> health_02_requirements{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 1, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20330, 0, 1}}};
inline constexpr std::array<SymbolBinding, 9> health_02_bindings{
    {{"ITPL_HEALTH_HERB_02", 16716, 7, 1, 0, 1584},
     {"ITPL_TEMP_HERB", 16736, 7, 1, 0, 1584},
     {"ITMI_FLASK", 16604, 7, 1, 0, 1584},
     {"ITPO_HEALTH_02", 16778, 7, 1, 0, 1584},
     {"PC_ITPO_HEALTH_02", 20344, 7, 1, 0, 1649},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1},
     {"PLAYER_TALENT_ALCHEMY", 1464, 2, 0, 15, -1},
     {"HEALTHSTART", 20330, 2, 0, 1, -1},
     {"POTION_HEALTH_02", 1449, 2, 1, 1, -1, true, 1}}};
inline constexpr std::array<Requirement, 3> owner_20344_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 1, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20330, 0, 1}}};
inline constexpr std::array<Owner, 1> health_02_owners{{
    {20344, 3, 22243, false, false, true, &functions[28], &functions[29], &functions[30],
     view(owner_20344_conditions)},
}};
inline constexpr std::array<MaterialEffect, 3> effects_20348{
    {{MaterialAction::remove, 16716, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16778, 1, 0}}};
inline constexpr std::array<MaterialEffect, 4> effects_20349{
    {{MaterialAction::remove, 16716, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16778, 1, 0},
     {MaterialAction::remove, 16604, 1, 1}}};
inline constexpr std::array<AmountOperand, 6> operands_20350{
    {{1, 5, 1, 0}, {22, 5, 1, 0}, {59, 5, 1, 0}, {79, 5, 1, 0}, {99, 5, 1, 0}, {119, 4, 1, 1}}};
inline constexpr std::array<MaterialEffect, 4> effects_20350{
    {{MaterialAction::remove, 16716, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16778, 1, 0},
     {MaterialAction::remove, 16604, 1, 1}}};
inline constexpr std::array<Callback, 3> health_02_callbacks{{
    {&functions[31],
     20344,
     Invocation::choice,
     1,
     1,
     1,
     {functions[31].byte_length, functions[31].body_sha256, nullptr, 0},
     view(effects_20348),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[32],
     20344,
     Invocation::choice,
     3,
     0,
     0,
     {functions[32].byte_length, functions[32].body_sha256, nullptr, 0},
     view(effects_20349),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[33],
     20344,
     Invocation::choice,
     5,
     2,
     2147483647,
     {functions[33].byte_length, functions[33].body_sha256, operands_20350.data(), 6},
     view(effects_20350),
     view(flask_failure),
     true,
     true,
     false,
     true},
}};

inline constexpr std::array<ItemAmount, 3> health_03_costs{{{16718, 1}, {16736, 1}, {16604, 1}}};
inline constexpr std::array<ItemAmount, 1> health_03_outputs{{{16781, 1}}};
inline constexpr std::array<PackedInventoryTarget, 4> health_03_targets{
    {{"ITPL_HEALTH_HERB_03", 16718},
     {"ITPL_TEMP_HERB", 16736},
     {"ITMI_FLASK", 16604},
     {"ITPO_HEALTH_03", 16781}}};
inline constexpr std::array<Requirement, 3> health_03_requirements{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 2, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20330, 0, 1}}};
inline constexpr std::array<SymbolBinding, 9> health_03_bindings{
    {{"ITPL_HEALTH_HERB_03", 16718, 7, 1, 0, 1584},
     {"ITPL_TEMP_HERB", 16736, 7, 1, 0, 1584},
     {"ITMI_FLASK", 16604, 7, 1, 0, 1584},
     {"ITPO_HEALTH_03", 16781, 7, 1, 0, 1584},
     {"PC_ITPO_HEALTH_03", 20351, 7, 1, 0, 1649},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1},
     {"PLAYER_TALENT_ALCHEMY", 1464, 2, 0, 15, -1},
     {"HEALTHSTART", 20330, 2, 0, 1, -1},
     {"POTION_HEALTH_03", 1450, 2, 1, 1, -1, true, 2}}};
inline constexpr std::array<Requirement, 3> owner_20351_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 2, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20330, 0, 1}}};
inline constexpr std::array<Owner, 1> health_03_owners{{
    {20351, 4, 22243, false, false, true, &functions[34], &functions[35], &functions[36],
     view(owner_20351_conditions)},
}};
inline constexpr std::array<MaterialEffect, 3> effects_20355{
    {{MaterialAction::remove, 16718, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16781, 1, 0}}};
inline constexpr std::array<MaterialEffect, 4> effects_20356{
    {{MaterialAction::remove, 16718, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16781, 1, 0},
     {MaterialAction::remove, 16604, 1, 1}}};
inline constexpr std::array<AmountOperand, 6> operands_20357{
    {{1, 5, 1, 0}, {22, 5, 1, 0}, {59, 5, 1, 0}, {79, 5, 1, 0}, {99, 5, 1, 0}, {119, 4, 1, 1}}};
inline constexpr std::array<MaterialEffect, 4> effects_20357{
    {{MaterialAction::remove, 16718, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16781, 1, 0},
     {MaterialAction::remove, 16604, 1, 1}}};
inline constexpr std::array<Callback, 3> health_03_callbacks{{
    {&functions[37],
     20351,
     Invocation::choice,
     1,
     1,
     1,
     {functions[37].byte_length, functions[37].body_sha256, nullptr, 0},
     view(effects_20355),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[38],
     20351,
     Invocation::choice,
     3,
     0,
     0,
     {functions[38].byte_length, functions[38].body_sha256, nullptr, 0},
     view(effects_20356),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[39],
     20351,
     Invocation::choice,
     5,
     2,
     2147483647,
     {functions[39].byte_length, functions[39].body_sha256, operands_20357.data(), 6},
     view(effects_20357),
     view(flask_failure),
     true,
     true,
     false,
     true},
}};

inline constexpr std::array<ItemAmount, 3> mana_01_costs{{{16708, 1}, {16736, 1}, {16604, 1}}};
inline constexpr std::array<ItemAmount, 1> mana_01_outputs{{{16766, 1}}};
inline constexpr std::array<PackedInventoryTarget, 4> mana_01_targets{{{"ITPL_MANA_HERB_01", 16708},
                                                                       {"ITPL_TEMP_HERB", 16736},
                                                                       {"ITMI_FLASK", 16604},
                                                                       {"ITPO_MANA_01", 16766}}};
inline constexpr std::array<Requirement, 3> mana_01_requirements{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 3, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20299, 0, 1}}};
inline constexpr std::array<SymbolBinding, 9> mana_01_bindings{
    {{"ITPL_MANA_HERB_01", 16708, 7, 1, 0, 1584},
     {"ITPL_TEMP_HERB", 16736, 7, 1, 0, 1584},
     {"ITMI_FLASK", 16604, 7, 1, 0, 1584},
     {"ITPO_MANA_01", 16766, 7, 1, 0, 1584},
     {"PC_ITPO_MANA_01", 20306, 7, 1, 0, 1649},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1},
     {"PLAYER_TALENT_ALCHEMY", 1464, 2, 0, 15, -1},
     {"MANASTART", 20299, 2, 0, 1, -1},
     {"POTION_MANA_01", 1451, 2, 1, 1, -1, true, 3}}};
inline constexpr std::array<Requirement, 3> owner_20306_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 3, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20299, 0, 1}}};
inline constexpr std::array<Owner, 1> mana_01_owners{{
    {20306, 2, 22243, false, false, true, &functions[4], &functions[5], &functions[6],
     view(owner_20306_conditions)},
}};
inline constexpr std::array<MaterialEffect, 3> effects_20310{
    {{MaterialAction::remove, 16708, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16766, 1, 0}}};
inline constexpr std::array<MaterialEffect, 4> effects_20311{
    {{MaterialAction::remove, 16708, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::remove, 16604, 1, 1},
     {MaterialAction::create, 16766, 1, 0}}};
inline constexpr std::array<AmountOperand, 6> operands_20312{
    {{1, 5, 1, 0}, {22, 5, 1, 0}, {59, 5, 1, 0}, {79, 5, 1, 0}, {99, 4, 1, 1}, {119, 5, 1, 0}}};
inline constexpr std::array<MaterialEffect, 4> effects_20312{
    {{MaterialAction::remove, 16708, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::remove, 16604, 1, 1},
     {MaterialAction::create, 16766, 1, 0}}};
inline constexpr std::array<Callback, 3> mana_01_callbacks{{
    {&functions[7],
     20306,
     Invocation::choice,
     1,
     1,
     1,
     {functions[7].byte_length, functions[7].body_sha256, nullptr, 0},
     view(effects_20310),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[8],
     20306,
     Invocation::choice,
     3,
     0,
     0,
     {functions[8].byte_length, functions[8].body_sha256, nullptr, 0},
     view(effects_20311),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[9],
     20306,
     Invocation::choice,
     5,
     2,
     2147483647,
     {functions[9].byte_length, functions[9].body_sha256, operands_20312.data(), 6},
     view(effects_20312),
     view(flask_failure),
     true,
     true,
     false,
     true},
}};

inline constexpr std::array<ItemAmount, 3> mana_02_costs{{{16710, 1}, {16736, 1}, {16604, 1}}};
inline constexpr std::array<ItemAmount, 1> mana_02_outputs{{{16769, 1}}};
inline constexpr std::array<PackedInventoryTarget, 4> mana_02_targets{{{"ITPL_MANA_HERB_02", 16710},
                                                                       {"ITPL_TEMP_HERB", 16736},
                                                                       {"ITMI_FLASK", 16604},
                                                                       {"ITPO_MANA_02", 16769}}};
inline constexpr std::array<Requirement, 3> mana_02_requirements{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 4, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20299, 0, 1}}};
inline constexpr std::array<SymbolBinding, 9> mana_02_bindings{
    {{"ITPL_MANA_HERB_02", 16710, 7, 1, 0, 1584},
     {"ITPL_TEMP_HERB", 16736, 7, 1, 0, 1584},
     {"ITMI_FLASK", 16604, 7, 1, 0, 1584},
     {"ITPO_MANA_02", 16769, 7, 1, 0, 1584},
     {"PC_ITPO_MANA_02", 20313, 7, 1, 0, 1649},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1},
     {"PLAYER_TALENT_ALCHEMY", 1464, 2, 0, 15, -1},
     {"MANASTART", 20299, 2, 0, 1, -1},
     {"POTION_MANA_02", 1452, 2, 1, 1, -1, true, 4}}};
inline constexpr std::array<Requirement, 3> owner_20313_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 4, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20299, 0, 1}}};
inline constexpr std::array<Owner, 1> mana_02_owners{{
    {20313, 3, 22243, false, false, true, &functions[10], &functions[11], &functions[12],
     view(owner_20313_conditions)},
}};
inline constexpr std::array<MaterialEffect, 3> effects_20317{
    {{MaterialAction::remove, 16710, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16769, 1, 0}}};
inline constexpr std::array<MaterialEffect, 4> effects_20318{
    {{MaterialAction::remove, 16710, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16769, 1, 0},
     {MaterialAction::remove, 16604, 1, 1}}};
inline constexpr std::array<AmountOperand, 6> operands_20319{
    {{1, 5, 1, 0}, {22, 5, 1, 0}, {59, 5, 1, 0}, {79, 5, 1, 0}, {99, 5, 1, 0}, {119, 4, 1, 1}}};
inline constexpr std::array<MaterialEffect, 4> effects_20319{
    {{MaterialAction::remove, 16710, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16769, 1, 0},
     {MaterialAction::remove, 16604, 1, 1}}};
inline constexpr std::array<Callback, 3> mana_02_callbacks{{
    {&functions[13],
     20313,
     Invocation::choice,
     1,
     1,
     1,
     {functions[13].byte_length, functions[13].body_sha256, nullptr, 0},
     view(effects_20317),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[14],
     20313,
     Invocation::choice,
     3,
     0,
     0,
     {functions[14].byte_length, functions[14].body_sha256, nullptr, 0},
     view(effects_20318),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[15],
     20313,
     Invocation::choice,
     5,
     2,
     2147483647,
     {functions[15].byte_length, functions[15].body_sha256, operands_20319.data(), 6},
     view(effects_20319),
     view(flask_failure),
     true,
     true,
     false,
     true},
}};

inline constexpr std::array<ItemAmount, 3> mana_03_costs{{{16712, 1}, {16736, 1}, {16604, 1}}};
inline constexpr std::array<ItemAmount, 1> mana_03_outputs{{{16772, 1}}};
inline constexpr std::array<PackedInventoryTarget, 4> mana_03_targets{{{"ITPL_MANA_HERB_03", 16712},
                                                                       {"ITPL_TEMP_HERB", 16736},
                                                                       {"ITMI_FLASK", 16604},
                                                                       {"ITPO_MANA_03", 16772}}};
inline constexpr std::array<Requirement, 3> mana_03_requirements{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 5, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20299, 0, 1}}};
inline constexpr std::array<SymbolBinding, 9> mana_03_bindings{
    {{"ITPL_MANA_HERB_03", 16712, 7, 1, 0, 1584},
     {"ITPL_TEMP_HERB", 16736, 7, 1, 0, 1584},
     {"ITMI_FLASK", 16604, 7, 1, 0, 1584},
     {"ITPO_MANA_03", 16772, 7, 1, 0, 1584},
     {"PC_ITPO_MANA_03", 20320, 7, 1, 0, 1649},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1},
     {"PLAYER_TALENT_ALCHEMY", 1464, 2, 0, 15, -1},
     {"MANASTART", 20299, 2, 0, 1, -1},
     {"POTION_MANA_03", 1453, 2, 1, 1, -1, true, 5}}};
inline constexpr std::array<Requirement, 3> owner_20320_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 1464, 5, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20299, 0, 1}}};
inline constexpr std::array<Owner, 1> mana_03_owners{{
    {20320, 4, 22243, false, false, true, &functions[16], &functions[17], &functions[18],
     view(owner_20320_conditions)},
}};
inline constexpr std::array<MaterialEffect, 3> effects_20324{
    {{MaterialAction::remove, 16712, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16772, 1, 0}}};
inline constexpr std::array<MaterialEffect, 4> effects_20325{
    {{MaterialAction::remove, 16712, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16772, 1, 0},
     {MaterialAction::remove, 16604, 1, 1}}};
inline constexpr std::array<AmountOperand, 6> operands_20326{
    {{1, 5, 1, 0}, {22, 5, 1, 0}, {59, 5, 1, 0}, {79, 5, 1, 0}, {99, 5, 1, 0}, {119, 4, 1, 1}}};
inline constexpr std::array<MaterialEffect, 4> effects_20326{
    {{MaterialAction::remove, 16712, 1, 0},
     {MaterialAction::remove, 16736, 1, 0},
     {MaterialAction::create, 16772, 1, 0},
     {MaterialAction::remove, 16604, 1, 1}}};
inline constexpr std::array<Callback, 3> mana_03_callbacks{{
    {&functions[19],
     20320,
     Invocation::choice,
     1,
     1,
     1,
     {functions[19].byte_length, functions[19].body_sha256, nullptr, 0},
     view(effects_20324),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[20],
     20320,
     Invocation::choice,
     3,
     0,
     0,
     {functions[20].byte_length, functions[20].body_sha256, nullptr, 0},
     view(effects_20325),
     view(flask_failure),
     true,
     true,
     false,
     true},
    {&functions[21],
     20320,
     Invocation::choice,
     5,
     2,
     2147483647,
     {functions[21].byte_length, functions[21].body_sha256, operands_20326.data(), 6},
     view(effects_20326),
     view(flask_failure),
     true,
     true,
     false,
     true},
}};

inline constexpr std::array<ItemAmount, 3> molerat_dish_costs{{{16363, 5}, {16401, 5}, {16395, 5}}};
inline constexpr std::array<ItemAmount, 1> molerat_dish_outputs{{{16365, 1}}};
inline constexpr std::array<PackedInventoryTarget, 6> molerat_dish_targets{
    {{"ITFO_MOLERATMUTTONRAW", 16363},
     {"ITFO_WINE", 16401},
     {"ITFO_WATER", 16395},
     {"ITFO_NICLAS", 16365},
     {"ITMI_PAN", 16607},
     {"ITMI_PANFULL", 16608}}};
inline constexpr std::array<Requirement, 2> molerat_dish_requirements{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 14},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 10535, 0, 1}}};
inline constexpr std::array<SymbolBinding, 10> molerat_dish_bindings{
    {{"ITFO_MOLERATMUTTONRAW", 16363, 7, 1, 0, 1584},
     {"ITFO_WINE", 16401, 7, 1, 0, 1584},
     {"ITFO_WATER", 16395, 7, 1, 0, 1584},
     {"ITFO_NICLAS", 16365, 7, 1, 0, 1584},
     {"ITMI_PAN", 16607, 7, 1, 0, 1584},
     {"ITMI_PANFULL", 16608, 7, 1, 0, 1584},
     {"PC_FRYINGMEAT_MOLERATKEULE", 21021, 7, 1, 0, 1649},
     {"PC_FRYINGMEAT_MOLERATKEULE_2", 21024, 7, 1, 0, 1649},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1},
     {"READ_MOLERATKEULE", 10535, 2, 0, 1, -1}}};
inline constexpr std::array<Requirement, 5> owner_21021_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 14},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 10535, 0, 1},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16363, 0, 5},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16401, 0, 5},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16395, 0, 5}}};
inline constexpr std::array<Requirement, 5> owner_21024_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 14},
     {RequirementKind::integer_equal, RequirementRole::knowledge, 10535, 0, 1},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16363, 0, 10},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16401, 0, 10},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16395, 0, 10}}};
inline constexpr std::array<Owner, 2> molerat_dish_owners{{
    {21021, 5, 22243, false, false, true, &functions[53], &functions[54], &functions[55],
     view(owner_21021_conditions)},
    {21024, 6, 22243, false, false, true, &functions[56], &functions[57], &functions[58],
     view(owner_21024_conditions)},
}};
inline constexpr std::array<AmountOperand, 4> operands_21023{
    {{11, 5, 5, 0}, {31, 5, 5, 0}, {51, 5, 5, 0}, {71, 1, 1, 0}}};
inline constexpr std::array<MaterialEffect, 4> effects_21023{
    {{MaterialAction::remove, 16363, 5, 0},
     {MaterialAction::remove, 16401, 5, 0},
     {MaterialAction::remove, 16395, 5, 0},
     {MaterialAction::create, 16365, 1, 0}}};
inline constexpr std::array<MaterialEffect, 4> effects_21026{
    {{MaterialAction::remove, 16363, 5, 0},
     {MaterialAction::remove, 16401, 5, 0},
     {MaterialAction::remove, 16395, 5, 0},
     {MaterialAction::create, 16365, 1, 0}}};
inline constexpr std::array<Callback, 2> molerat_dish_callbacks{{
    {&functions[55],
     21021,
     Invocation::info,
     1,
     1,
     429496729,
     {functions[55].byte_length, functions[55].body_sha256, operands_21023.data(), 4},
     view(effects_21023),
     {},
     false,
     false,
     true,
     false},
    {&functions[58],
     21024,
     Invocation::info,
     2,
     0,
     0,
     {functions[58].byte_length, functions[58].body_sha256, nullptr, 0},
     view(effects_21026),
     {},
     false,
     false,
     true,
     false},
}};

inline constexpr std::array<ItemAmount, 1> mushroom_dish_costs{{{16726, 50}}};
inline constexpr std::array<ItemAmount, 1> mushroom_dish_outputs{{{16361, 1}}};
inline constexpr std::array<PackedInventoryTarget, 4> mushroom_dish_targets{
    {{"ITPL_MUSHROOM_01", 16726},
     {"ITFO_PILZBRATEN", 16361},
     {"ITMI_PAN", 16607},
     {"ITMI_PANFULL", 16608}}};
inline constexpr std::array<Requirement, 1> mushroom_dish_requirements{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 14}}};
inline constexpr std::array<SymbolBinding, 6> mushroom_dish_bindings{
    {{"ITPL_MUSHROOM_01", 16726, 7, 1, 0, 1584},
     {"ITFO_PILZBRATEN", 16361, 7, 1, 0, 1584},
     {"ITMI_PAN", 16607, 7, 1, 0, 1584},
     {"ITMI_PANFULL", 16608, 7, 1, 0, 1584},
     {"PC_FRYINGMEAT_PILZBRATEN", 21027, 7, 1, 0, 1649},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1}}};
inline constexpr std::array<Requirement, 2> owner_21027_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 14},
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16726, 0, 50}}};
inline constexpr std::array<Owner, 1> mushroom_dish_owners{{
    {21027, 7, 22243, false, false, true, &functions[59], &functions[60], &functions[61],
     view(owner_21027_conditions)},
}};
inline constexpr std::array<AmountOperand, 2> operands_21029{{{11, 50, 50, 0}, {31, 1, 1, 0}}};
inline constexpr std::array<MaterialEffect, 2> effects_21029{
    {{MaterialAction::remove, 16726, 50, 0}, {MaterialAction::create, 16361, 1, 0}}};
inline constexpr std::array<Callback, 1> mushroom_dish_callbacks{{
    {&functions[61],
     21027,
     Invocation::info,
     1,
     1,
     42949672,
     {functions[61].byte_length, functions[61].body_sha256, operands_21029.data(), 2},
     view(effects_21029),
     {},
     false,
     false,
     true,
     false},
}};

inline constexpr std::array<Descriptor, 9> recipes{
    {{RecipeId::meat,
      "Fleisch braten",
      Station::stove,
      14,
      view(meat_targets),
      view(meat_bindings),
      view(meat_requirements),
      view(meat_owners),
      view(meat_callbacks),
      view(meat_costs),
      view(meat_outputs),
      {16607, 16608, 1, false, false, 0, HandEvidence::observed_original, true},
      &functions[40],
      &functions[62]},
     {RecipeId::health_01,
      "Heiltrank Stufe 1",
      Station::alchemy,
      4,
      view(health_01_targets),
      view(health_01_bindings),
      view(health_01_requirements),
      view(health_01_owners),
      view(health_01_callbacks),
      view(health_01_costs),
      view(health_01_outputs),
      {16604, 16604, 1, true, true, 1, HandEvidence::observed_original, true},
      &functions[2],
      &functions[3]},
     {RecipeId::health_02,
      "Heiltrank Stufe 2",
      Station::alchemy,
      4,
      view(health_02_targets),
      view(health_02_bindings),
      view(health_02_requirements),
      view(health_02_owners),
      view(health_02_callbacks),
      view(health_02_costs),
      view(health_02_outputs),
      {16604, 16604, 1, true, true, 1, HandEvidence::inferred_same_station, true},
      &functions[2],
      &functions[3]},
     {RecipeId::health_03,
      "Heiltrank Stufe 3",
      Station::alchemy,
      4,
      view(health_03_targets),
      view(health_03_bindings),
      view(health_03_requirements),
      view(health_03_owners),
      view(health_03_callbacks),
      view(health_03_costs),
      view(health_03_outputs),
      {16604, 16604, 1, true, true, 1, HandEvidence::inferred_same_station, true},
      &functions[2],
      &functions[3]},
     {RecipeId::mana_01,
      "Manatrank Stufe 1",
      Station::alchemy,
      4,
      view(mana_01_targets),
      view(mana_01_bindings),
      view(mana_01_requirements),
      view(mana_01_owners),
      view(mana_01_callbacks),
      view(mana_01_costs),
      view(mana_01_outputs),
      {16604, 16604, 1, true, true, 1, HandEvidence::inferred_same_station, true},
      &functions[2],
      &functions[3]},
     {RecipeId::mana_02,
      "Manatrank Stufe 2",
      Station::alchemy,
      4,
      view(mana_02_targets),
      view(mana_02_bindings),
      view(mana_02_requirements),
      view(mana_02_owners),
      view(mana_02_callbacks),
      view(mana_02_costs),
      view(mana_02_outputs),
      {16604, 16604, 1, true, true, 1, HandEvidence::inferred_same_station, true},
      &functions[2],
      &functions[3]},
     {RecipeId::mana_03,
      "Manatrank Stufe 3",
      Station::alchemy,
      4,
      view(mana_03_targets),
      view(mana_03_bindings),
      view(mana_03_requirements),
      view(mana_03_owners),
      view(mana_03_callbacks),
      view(mana_03_costs),
      view(mana_03_outputs),
      {16604, 16604, 1, true, true, 1, HandEvidence::inferred_same_station, true},
      &functions[2],
      &functions[3]},
     {RecipeId::molerat_dish,
      "Moleratgericht",
      Station::stove,
      14,
      view(molerat_dish_targets),
      view(molerat_dish_bindings),
      view(molerat_dish_requirements),
      view(molerat_dish_owners),
      view(molerat_dish_callbacks),
      view(molerat_dish_costs),
      view(molerat_dish_outputs),
      {16607, 16608, 1, false, false, 0, HandEvidence::inferred_same_station, true},
      &functions[40],
      &functions[62]},
     {RecipeId::mushroom_dish,
      "Pilzgericht",
      Station::stove,
      14,
      view(mushroom_dish_targets),
      view(mushroom_dish_bindings),
      view(mushroom_dish_requirements),
      view(mushroom_dish_owners),
      view(mushroom_dish_callbacks),
      view(mushroom_dish_costs),
      view(mushroom_dish_outputs),
      {16607, 16608, 1, false, false, 0, HandEvidence::inferred_same_station, true},
      &functions[40],
      &functions[62]}}};

constexpr std::size_t target_capacity() noexcept {
  std::size_t result = 0;
  for (const auto& recipe : recipes)
    if (recipe.targets.count > result)
      result = recipe.targets.count;
  return result;
}
// Only a workspace size derived from the current catalog,
// no limit for other descriptors or the inventory reader.
inline constexpr std::size_t catalog_target_capacity = target_capacity();
inline const Descriptor* find(RecipeId id) noexcept {
  for (const auto& recipe : recipes)
    if (recipe.id == id)
      return &recipe;
  return nullptr;
}
inline const Owner* owner_for(const Descriptor& recipe, std::int32_t instance) noexcept {
  for (const auto& owner : recipe.owners)
    if (owner.instance == instance)
      return &owner;
  return nullptr;
}
inline bool has_target(const Descriptor& recipe, std::int32_t symbol) noexcept {
  if (!recipe.targets.data)
    return false;
  for (const auto& item : recipe.targets)
    if (item.symbol == symbol)
      return true;
  return false;
}
inline bool valid_requirement(const Descriptor& recipe, const Requirement& requirement) noexcept {
  if (requirement.role != RequirementRole::context &&
      requirement.role != RequirementRole::knowledge &&
      requirement.role != RequirementRole::resource)
    return false;
  if (requirement.kind == RequirementKind::inventory_at_least)
    return requirement.role == RequirementRole::resource && requirement.element == 0 &&
           requirement.value > 0 && has_target(recipe, requirement.symbol);
  if (requirement.kind != RequirementKind::integer_equal || requirement.symbol <= 0 ||
      requirement.role == RequirementRole::resource)
    return false;
  for (const auto& binding : recipe.bindings)
    if (binding.symbol == static_cast<std::uint32_t>(requirement.symbol) && binding.type == 2 &&
        !(binding.flags & 4u) && requirement.element < binding.elements)
      return true;
  return false;
}

// Trusted, own C++ metadata; no check of arbitrary pointers.
// Costs/expenses are disjoint for this contract. A later family
// with the same input/output item needs its own ordering contract.
inline bool structurally_valid(const Descriptor& recipe) noexcept {
  if (!recipe.name || !*recipe.name || !recipe.targets.data || !recipe.targets.count ||
      !recipe.costs.data || !recipe.costs.count || !recipe.outputs.data || !recipe.outputs.count ||
      !recipe.bindings.data || !recipe.bindings.count || !recipe.requirements.data ||
      !recipe.requirements.count || !recipe.owners.data || !recipe.owners.count ||
      !recipe.callbacks.data || !recipe.callbacks.count || !recipe.station_entry ||
      !recipe.station_end)
    return false;
  if ((recipe.station != Station::stove && recipe.station != Station::alchemy) ||
      recipe.production_context != (recipe.station == Station::stove ? 14 : 4))
    return false;
  for (std::size_t i = 0; i < recipe.targets.count; ++i) {
    const auto& target = recipe.targets[i];
    if (!target.name || !*target.name || std::strlen(target.name) > 63 || target.symbol <= 0)
      return false;
    for (std::size_t j = 0; j < i; ++j)
      if (target.symbol == recipe.targets[j].symbol ||
          std::strcmp(target.name, recipe.targets[j].name) == 0)
        return false;
    bool bound = false;
    for (const auto& binding : recipe.bindings)
      if (binding.symbol == static_cast<std::uint32_t>(target.symbol) && binding.name &&
          std::strcmp(binding.name, target.name) == 0 && binding.type == 7 && binding.flags == 1 &&
          binding.elements == 0 && binding.parent_index == 1584)
        bound = true;
    if (!bound)
      return false;
  }
  for (const auto entries : {recipe.costs, recipe.outputs}) {
    for (std::size_t i = 0; i < entries.count; ++i) {
      if (!entries[i].units || entries[i].units > INT32_MAX ||
          !has_target(recipe, entries[i].symbol))
        return false;
      for (std::size_t j = 0; j < i; ++j)
        if (entries[i].symbol == entries[j].symbol)
          return false;
    }
  }
  for (const auto& input : recipe.costs)
    for (const auto& output : recipe.outputs)
      if (input.symbol == output.symbol)
        return false;
  const auto& hand = recipe.hand;
  if (!hand.amount || hand.amount > INT32_MAX || !has_target(recipe, hand.active_item_symbol) ||
      !has_target(recipe, hand.start_item_symbol) || !hand.left_must_be_empty ||
      (hand.evidence != HandEvidence::unknown &&
       hand.evidence != HandEvidence::inferred_same_station &&
       hand.evidence != HandEvidence::observed_original))
    return false;
  if (hand.from_inventory) {
    bool cost = false;
    for (const auto& input : recipe.costs)
      if (input.symbol == hand.active_item_symbol && input.units >= hand.amount)
        cost = true;
    if (!cost || !hand.consumed_on_success || hand.failure_refund != hand.amount)
      return false;
  } else if (hand.consumed_on_success || hand.failure_refund)
    return false;
  if (hand.amount != 1 || (recipe.station == Station::alchemy
                               ? (hand.start_item_symbol != 16604 ||
                                  hand.active_item_symbol != 16604 || !hand.from_inventory)
                               : (hand.start_item_symbol != 16607 ||
                                  hand.active_item_symbol != 16608 || hand.from_inventory)))
    return false;
  for (const auto& requirement : recipe.requirements)
    if (!valid_requirement(recipe, requirement))
      return false;
  for (std::size_t i = 0; i < recipe.owners.count; ++i) {
    const auto& owner = recipe.owners[i];
    if (owner.instance <= 0 || !owner.constructor || !owner.condition || !owner.information ||
        owner.constructor->symbol != static_cast<std::uint32_t>(owner.instance) ||
        owner.constructor->type != 7 || owner.constructor->flags != 1 || owner.constructor->argc ||
        owner.constructor->parent_index != 1649 || owner.condition->type != 5 ||
        owner.condition->flags != 3 || owner.condition->argc || owner.condition->return_type != 2 ||
        owner.information->type != 5 || owner.information->flags != 1 || owner.information->argc ||
        owner.information->return_type || !owner.conditions.data || !owner.conditions.count)
      return false;
    for (std::size_t j = 0; j < i; ++j)
      if (owner.instance == recipe.owners[j].instance)
        return false;
    for (const auto& requirement : owner.conditions)
      if (!valid_requirement(recipe, requirement))
        return false;
  }
  for (std::size_t at = 0; at < recipe.callbacks.count; ++at) {
    const auto& callback = recipe.callbacks[at];
    const auto* function = callback.function;
    const auto* owner = owner_for(recipe, callback.owner_instance);
    if (!function || !owner || function->type != 5 || function->flags != 1 || function->argc ||
        function->return_type || function->parent_index != -1 || !function->byte_length ||
        function->byte_length > AmountPatch::max_body ||
        function->byte_offset >= lhiver::code_length ||
        function->byte_length > lhiver::code_length - function->byte_offset ||
        callback.amount.length != function->byte_length ||
        callback.amount.digest != function->body_sha256 || !callback.effects.data ||
        !callback.effects.count ||
        (callback.failure_effects.count && !callback.failure_effects.data))
      return false;
    if (callback.invocation != Invocation::info && callback.invocation != Invocation::choice)
      return false;
    if (callback.invocation == Invocation::info && owner->information->symbol != function->symbol)
      return false;
    if ((!callback.minimum_quantity) != (!callback.maximum_quantity) ||
        callback.minimum_quantity > callback.maximum_quantity ||
        callback.maximum_quantity > INT32_MAX ||
        (callback.amount.count && !callback.amount.operands) ||
        callback.amount.count > AmountPatch::max_operands)
      return false;
    if (callback.minimum_quantity &&
        (!callback.original_quantity ||
         (!callback.amount.count &&
          (callback.maximum_quantity != 1 || callback.original_quantity != 1))))
      return false;
    if (callback.amount.count && !callback.minimum_quantity)
      return false;
    for (std::size_t j = 0; j < at; ++j) {
      const auto& earlier = recipe.callbacks[j];
      if (earlier.function->symbol == function->symbol)
        return false;
      if (callback.minimum_quantity && earlier.minimum_quantity &&
          callback.minimum_quantity <= earlier.maximum_quantity &&
          earlier.minimum_quantity <= callback.maximum_quantity)
        return false;
    }
    for (std::size_t i = 0; i < callback.amount.count; ++i) {
      const auto& operand = callback.amount.operands[i];
      if (!operand.offset || operand.offset >= function->byte_length ||
          function->byte_length - operand.offset < 4 || operand.original < 0 || !operand.units ||
          operand.allow_zero)
        return false;
      const auto original = std::uint64_t{operand.units} * callback.original_quantity;
      if (original < operand.reserved ||
          original - operand.reserved != static_cast<std::uint32_t>(operand.original))
        return false;
      for (std::size_t j = 0; j < i; ++j) {
        const auto other = callback.amount.operands[j].offset;
        if (operand.offset < other + 4 && other < operand.offset + 4)
          return false;
      }
    }
    for (const auto& effect : callback.effects)
      if (!has_target(recipe, effect.symbol) || !effect.units || effect.units > INT32_MAX ||
          (effect.action != MaterialAction::remove && effect.action != MaterialAction::create))
        return false;
    for (const auto& effect : callback.failure_effects)
      if (!has_target(recipe, effect.symbol) || !effect.amount || effect.amount > INT32_MAX ||
          (effect.action != MaterialAction::remove && effect.action != MaterialAction::create) ||
          (effect.actor != Actor::hero && effect.actor != Actor::self))
        return false;
  }
  return true;
}

// Only selection of a statically matching template; NO inventory, learning, or
// runtime release. N=1 in alchemy stays original and never produces Remove(0).
inline const Callback* callback_for_quantity(const Descriptor& recipe,
                                             std::uint32_t quantity) noexcept {
  if (!quantity || quantity > INT32_MAX || !structurally_valid(recipe))
    return nullptr;
  for (const auto& callback : recipe.callbacks) {
    if (!callback.minimum_quantity || quantity < callback.minimum_quantity ||
        quantity > callback.maximum_quantity)
      continue;
    bool fits = true;
    for (const auto& cost : recipe.costs)
      if (std::uint64_t{cost.units} * quantity > INT32_MAX)
        fits = false;
    for (const auto& output : recipe.outputs)
      if (std::uint64_t{output.units} * quantity > INT32_MAX)
        fits = false;
    for (std::size_t i = 0; i < callback.amount.count; ++i) {
      const auto& operand = callback.amount.operands[i];
      const auto total = std::uint64_t{operand.units} * quantity;
      if (total <= operand.reserved || total - operand.reserved > INT32_MAX)
        fits = false;
    }
    if (fits)
      return &callback;
  }
  return nullptr;
}
} // namespace ubc::recipe_catalog
