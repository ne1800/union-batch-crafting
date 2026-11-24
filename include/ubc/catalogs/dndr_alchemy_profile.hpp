// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/recipe_catalog.hpp"

namespace ubc::dndr {
using lhiver::digest_literal;
using recipe_catalog::view;
inline constexpr long dat_size = 7587412;
inline constexpr auto dat_digest =
    digest_literal("b9e18ac81e08f9eac5942e23adff180f8ca239730eb4c4937b87a4f5bf7beda8");
inline constexpr unsigned hero = 1620, self = 1616, production = 1954, flask = 6906;
// On-disk facts. The Union parser may append symbols and code after loading.
inline constexpr std::array<lhiver::Function, 14> functions{{
    {"POTIONALCHEMY_S1", 10033, 456274, 81,
     digest_literal("7acbb7b1c9c14e7450df592013ae34a7e8af499b1ad0de8d9f9f47bf8678b423"), 0, 5, 1, 0,
     -1},
    {"B_ENDPRODUCTIONDIALOG", 9805, 420245, 49,
     digest_literal("167ac60f205a6f0b2a4e3a31049699f9a488197fc74f089579f6f0de2c162ed7"), 0, 5, 1, 0,
     -1},
    {"PC_POTIONALCHEMY_END", 10035, 456355, 67,
     digest_literal("5163022ec5b40fe1f9eafc578e266724e61babc5749034142b6fc4cb0a5d9016"), 0, 7, 1, 0,
     1586},
    {"PC_POTIONALCHEMY_END_CONDITION", 10036, 456422, 83,
     digest_literal("245dd6dc9fa5b0f968b2103f2b364eb2fc6fc543593c948809e49ade46598516"), 0, 5, 3, 2,
     -1},
    {"PC_POTIONALCHEMY_END_INFO", 10037, 456505, 26,
     digest_literal("ea4b646c97bd20bb64be254d231e0ba77b86d256ef4318b9da7137d3a832bb98"), 0, 5, 1, 0,
     -1},
    {"PC_MANA_START", 10058, 458425, 67,
     digest_literal("7932742d953ed4cb4e77076ee36ac13a61c1cf63d52c0f31c18c2119b9f4a44e"), 0, 7, 1, 0,
     1586},
    {"PC_MANA_START_CONDITION", 10059, 458492, 83,
     digest_literal("69ed68015186883e409e736112fa75c9ffaad63cdff7b6edc51808271a9a5f7d"), 0, 5, 3, 2,
     -1},
    {"PC_MANA_START_INFO", 10060, 458575, 12,
     digest_literal("95f31c61ee020c630d1b6d4e7125381b88a1fbff9397f271e7c25c3083a1b7f4"), 0, 5, 1, 0,
     -1},
    {"PC_MANA_STOP", 10061, 458587, 67,
     digest_literal("e0890d10f95c4bde0c4b2e77f48b4ecaa96f8b9441443aa8c2c6ea949279695c"), 0, 7, 1, 0,
     1586},
    {"PC_MANA_STOP_CONDITION", 10062, 458654, 35,
     digest_literal("bef2e78168a2e122bcc13145a8531e98059b465613af142246304c8baa3d01af"), 0, 5, 3, 2,
     -1},
    {"PC_MANA_STOP_INFO", 10063, 458689, 12,
     digest_literal("90ef76017a67271b1f67ef0e8f80e2d85361980c52ca92340245535864f9284e"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_01", 10064, 458701, 67,
     digest_literal("0d71464f244b935e7f6b2891f11e455c59e164c43662d35279c92cb71388c3cf"), 0, 7, 1, 0,
     1586},
    {"PC_ITPO_MANA_01_CONDITION", 10065, 458768, 48,
     digest_literal("b57bac6b7ad97595940cda5fab63c35b1e19165f758837dbcef10f3d66483672"), 0, 5, 3, 2,
     -1},
    {"PC_ITPO_MANA_01_INFO", 10066, 458816, 159,
     digest_literal("d11e09a4e14783f6d0d8fd5d850e18a0d621f066bd166d4524ac1ba6ab42a3c7"), 0, 5, 1, 0,
     -1},
}};
inline constexpr std::array<lhiver::SymbolBinding, 21> bindings{{
    {"HERO", 1620, 7, 0, 0, 1474},
    {"SELF", 1616, 7, 0, 0, 1474},
    {"PC_HERO", 11471, 7, 1, 0, 10346},
    {"PLAYER_MOBSI_PRODUCTION", 1954, 2, 0, 1, -1},
    {"MOBSI_POTIONALCHEMY", 1950, 2, 1, 1, -1, true, 4},
    {"MOBSI_NONE", 1946, 2, 1, 1, -1, true, 0},
    {"PLAYER_TALENT_ALCHEMY", 1421, 2, 0, 15, -1},
    {"POTION_MANA_01", 1408, 2, 1, 1, -1, true, 3},
    {"MANASTART", 10057, 2, 0, 1, -1},
    {"SPECIALSTART", 10095, 2, 0, 1, -1},
    {"HEALTHSTART", 10076, 2, 0, 1, -1},
    {"TABAKSTART", 10120, 2, 0, 1, -1},
    {"BOOZESTART", 10038, 2, 0, 1, -1},
    {"ITMI_FLASK", 6906, 7, 1, 0, 1521},
    {"ITPL_MANA_HERB_01", 7001, 7, 1, 0, 1521},
    {"ITPL_TEMP_HERB", 7029, 7, 1, 0, 1521},
    {"ITPO_MANA_01", 7059, 7, 1, 0, 1521},
    {"NPC_HASITEMS", 160, 5, 11, 2, -1, false, 0, true, 2},
    {"NPC_REMOVEINVITEMS", 184, 5, 11, 3, -1, false, 0, true, 2},
    {"CREATEINVITEMS", 170, 5, 9, 3, -1, false, 0, true, 0},
    {"PRINT", 12, 5, 9, 1, -1, false, 0, true, 0},
}};
inline constexpr lhiver::Profile profile{
    lhiver::game_parser,
    74651,
    2248542,
    digest_literal("91e2b37d789129e8d68b221f3b0fac6374c8d98f93bbf558b0c6bbe39c3bda48"),
    functions.data(),
    functions.size(),
    bindings.data(),
    bindings.size()};
inline constexpr std::array<recipe_catalog::Owner, 4> owners{{
    {10035, 999, 11471, false, false, true, &functions[2], &functions[3], &functions[4], {}},
    {10058, 6, 11471, false, false, true, &functions[5], &functions[6], &functions[7], {}},
    {10061, 99, 11471, false, false, true, &functions[8], &functions[9], &functions[10], {}},
    {10064, 2, 11471, false, false, true, &functions[11], &functions[12], &functions[13], {}},
}};
inline constexpr std::array<PackedInventoryTarget, 4> mana_targets{{{"ITPL_MANA_HERB_01", 7001},
                                                                    {"ITPL_TEMP_HERB", 7029},
                                                                    {"ITMI_FLASK", 6906},
                                                                    {"ITPO_MANA_01", 7059}}};
inline constexpr std::array<recipe_catalog::ItemAmount, 3> mana_costs{
    {{7001, 2}, {7029, 1}, {6906, 1}}};
inline constexpr std::array<recipe_catalog::ItemAmount, 1> mana_outputs{{{7059, 1}}};
// Both original minima, both herb debits and CreateInvItems. The failure refund stays one.
inline constexpr std::array<AmountOperand, 5> mana_operands{
    {{1, 1, 1, 0}, {22, 2, 2, 0}, {59, 2, 2, 0}, {79, 1, 1, 0}, {99, 1, 1, 0}}};
struct AlchemyRecipe {
  std::uint32_t id;
  const char* name;
  const recipe_catalog::Owner *category, *effect;
  std::uint32_t category_flag, talent, element;
  recipe_catalog::View<PackedInventoryTarget> targets;
  recipe_catalog::View<recipe_catalog::ItemAmount> costs, outputs;
  recipe_catalog::View<AmountOperand> amounts;
};
inline constexpr std::array<AlchemyRecipe, 1> recipes{
    {{0x10004, "Mana essence", &owners[1], &owners[3], 10057, 1421, 3, view(mana_targets),
      view(mana_costs), view(mana_outputs), view(mana_operands)}}};
inline constexpr std::array<unsigned, 5> category_flags{{10038, 10057, 10076, 10095, 10120}};
inline constexpr bool supported_category(unsigned flag) noexcept {
  if (!flag)
    return true;
  for (const auto& recipe : recipes)
    if (recipe.category_flag == flag)
      return true;
  return false;
}
inline constexpr const AlchemyRecipe* callback(std::uint32_t symbol) noexcept {
  for (const auto& recipe : recipes)
    if (recipe.effect->information->symbol == symbol)
      return &recipe;
  return nullptr;
}
} // namespace ubc::dndr
