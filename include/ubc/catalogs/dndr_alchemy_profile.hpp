// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/dndr_anvil_contract.hpp"
#include "ubc/catalogs/dndr_alchemy_items.hpp"

namespace ubc::dndr {
using recipe_catalog::ItemAmount;
using recipe_catalog::View;
inline constexpr unsigned bloodfly = 3841;
struct Category {
  unsigned flag;
  const recipe_catalog::Owner *start, *stop;
};
inline constexpr std::array<Category, 5> categories{{
    {10038, &owners[1], &owners[2]},
    {10057, &owners[6], &owners[7]},
    {10076, &owners[12], &owners[13]},
    {10095, &owners[18], &owners[19]},
    {10120, &owners[26], &owners[27]},
}};
inline constexpr std::array<Category, 2> anvil_categories{{
    {10250, &anvil::owners[1], &anvil::owners[3]},
    {10249, &anvil::owners[2], &anvil::owners[4]},
}};
inline constexpr View<Category> station_categories(bool at_anvil) noexcept {
  return at_anvil ? View<Category>{anvil_categories.data(), anvil_categories.size()}
                  : View<Category>{categories.data(), categories.size()};
}
inline constexpr const Category* category(unsigned flag, bool at_anvil = false) noexcept {
  for (const auto& c : station_categories(at_anvil))
    if (c.flag == flag)
      return &c;
  return nullptr;
}
inline constexpr bool supported_category(unsigned flag, bool at_anvil = false) noexcept {
  return !flag || category(flag, at_anvil);
}

enum class OutputForm { multiple, single, branched };
struct Formula {
  unsigned id;
  const char* name;
  const recipe_catalog::Owner* effect;
  unsigned category_flag, talent, element;
  std::array<ItemAmount, 4> ingredients;
  std::array<int, 2> products; // For the spirit: ordinary first, bloodfly second.
  unsigned guard_jump, product_begin;
  OutputForm form;
  int reserve = flask;
  constexpr bool at_anvil() const noexcept { return reserve == anvil::hot; }
};
inline constexpr std::array<Formula, 20> alchemy_formulas{{
    {0x10001,
     "Healing essence",
     &owners[14],
     10076,
     1421,
     0,
     {{{7007, 2}, {7029, 1}}},
     {7065},
     43,
     88,
     OutputForm::single},
    {0x10002,
     "Healing extract",
     &owners[15],
     10076,
     1421,
     1,
     {{{7009, 2}, {7029, 1}}},
     {7067},
     43,
     88,
     OutputForm::multiple},
    {0x10003,
     "Healing elixir",
     &owners[16],
     10076,
     1421,
     2,
     {{{7011, 2}, {7029, 1}}},
     {7069},
     43,
     88,
     OutputForm::multiple},
    {0x10004,
     "Mana essence",
     &owners[8],
     10057,
     1421,
     3,
     {{{7001, 2}, {7029, 1}}},
     {7059},
     43,
     88,
     OutputForm::multiple},
    {0x10005,
     "Mana extract",
     &owners[9],
     10057,
     1421,
     4,
     {{{7003, 2}, {7029, 1}}},
     {7061},
     43,
     88,
     OutputForm::multiple},
    {0x10006,
     "Mana elixir",
     &owners[10],
     10057,
     1421,
     5,
     {{{7005, 2}, {7029, 1}}},
     {7063},
     43,
     88,
     OutputForm::multiple},
    {0x14001,
     "Lou's Hammer",
     &owners[3],
     10038,
     3199,
     0,
     {{{6999, 1}, {6997, 2}, {6559, 1}, {6117, 1}}},
     {6122},
     87,
     182,
     OutputForm::multiple},
    {0x14002,
     "Sleep Hammer",
     &owners[4],
     10038,
     3200,
     0,
     {{{6999, 1}, {6997, 2}, {6559, 1}, {6117, 2}}},
     {6124},
     87,
     182,
     OutputForm::multiple},
    {0x14003,
     "Fast Herring",
     &owners[5],
     10038,
     3201,
     0,
     {{{7017, 1}, {6117, 1}, {6717, 1}}},
     {6126},
     65,
     140,
     OutputForm::multiple},
    {0x14004,
     "Pure mana",
     &owners[11],
     10057,
     1421,
     13,
     {{{7059, 3}, {7029, 1}}},
     {6183},
     43,
     88,
     OutputForm::multiple},
    {0x14005,
     "Pure health",
     &owners[17],
     10076,
     1421,
     14,
     {{{7065, 3}, {7029, 1}}},
     {6181},
     43,
     88,
     OutputForm::multiple},
    {0x14006,
     "Permanent health",
     &owners[21],
     10095,
     1421,
     10,
     {{{7011, 1}, {7031, 1}}},
     {7075},
     43,
     88,
     OutputForm::multiple},
    {0x14007,
     "Permanent mana",
     &owners[22],
     10095,
     1421,
     9,
     {{{7005, 1}, {7031, 1}}},
     {7077},
     43,
     88,
     OutputForm::multiple},
    {0x14008,
     "Permanent dexterity",
     &owners[23],
     10095,
     1421,
     8,
     {{{7013, 1}, {7031, 1}}},
     {7073},
     43,
     88,
     OutputForm::multiple},
    {0x14009,
     "Permanent strength",
     &owners[24],
     10095,
     1421,
     7,
     {{{7015, 1}, {7031, 1}}},
     {7071},
     43,
     88,
     OutputForm::multiple},
    {0x1400a,
     "Speed potion",
     &owners[25],
     10095,
     1421,
     6,
     {{{7017, 1}, {7029, 1}}},
     {7079},
     43,
     88,
     OutputForm::multiple},
    {0x1400b,
     "Swampweed joint",
     &owners[28],
     10120,
     0,
     0,
     {{{6999, 1}}},
     {6922},
     21,
     46,
     OutputForm::multiple},
    {0x1400c,
     "Green Novice",
     &owners[29],
     10120,
     3085,
     0,
     {{{6999, 2}, {7029, 1}}},
     {6149},
     43,
     88,
     OutputForm::multiple},
    {0x1400d,
     "Dragon egg potion",
     &owners[30],
     0,
     1421,
     11,
     {{{7676, 10}, {6955, 1}, {6948, 1}}},
     {7081},
     65,
     130,
     OutputForm::multiple},
    {0x17001,
     "Spirit potion",
     &owners[20],
     10095,
     3072,
     0,
     {{{6546, 2}, {7061, 1}, {7065, 1}, {6129, 1}}},
     {6177, 6179},
     87,
     188,
     OutputForm::branched},
}};
inline constexpr std::array<Formula, 13> anvil_formulas{{
    {0x15001,
     "Sword",
     &anvil::owners[5],
     10250,
     1436,
     0,
     {{}},
     {6805},
     0,
     0,
     OutputForm::multiple,
     anvil::hot},
    {0x15002,
     "Fine sword",
     &anvil::owners[6],
     10250,
     1436,
     9,
     {{}},
     {6811},
     0,
     0,
     OutputForm::multiple,
     anvil::hot},
    {0x15003,
     "Fine longsword",
     &anvil::owners[7],
     10250,
     1436,
     10,
     {{}},
     {6817},
     0,
     0,
     OutputForm::multiple,
     anvil::hot},
    {0x15004,
     "Ruby blade",
     &anvil::owners[8],
     10250,
     1436,
     11,
     {{}},
     {6821},
     0,
     0,
     OutputForm::multiple,
     anvil::hot},
    {0x15005,
     "El Bastardo",
     &anvil::owners[9],
     10250,
     1436,
     12,
     {{}},
     {6832},
     0,
     0,
     OutputForm::multiple,
     anvil::hot},
    {0x16001,
     "Ore longsword",
     &anvil::owners[10],
     10249,
     1436,
     1,
     {{{6928, 1}}},
     {6818},
     21,
     46,
     OutputForm::multiple,
     anvil::hot},
    {0x16002,
     "Ore two-hander",
     &anvil::owners[11],
     10249,
     1436,
     2,
     {{{6928, 2}}},
     {6819},
     21,
     46,
     OutputForm::multiple,
     anvil::hot},
    {0x16003,
     "Ore bastard sword",
     &anvil::owners[12],
     10249,
     1436,
     3,
     {{{6928, 2}}},
     {6830},
     21,
     46,
     OutputForm::multiple,
     anvil::hot},
    {0x16004,
     "Heavy ore two-hander",
     &anvil::owners[13],
     10249,
     1436,
     4,
     {{{6928, 3}}},
     {6831},
     21,
     46,
     OutputForm::multiple,
     anvil::hot},
    {0x16005,
     "Ore battle blade",
     &anvil::owners[14],
     10249,
     1436,
     5,
     {{{6928, 3}}},
     {6842},
     21,
     46,
     OutputForm::multiple,
     anvil::hot},
    {0x16006,
     "Heavy ore battle blade",
     &anvil::owners[15],
     10249,
     1436,
     6,
     {{{6928, 4}}},
     {6843},
     21,
     46,
     OutputForm::multiple,
     anvil::hot},
    {0x16007,
     "Ore dragon slayer",
     &anvil::owners[16],
     10249,
     1436,
     7,
     {{{6928, 4}, {6572, 5}}},
     {6846},
     43,
     88,
     OutputForm::multiple,
     anvil::hot},
    {0x16008,
     "Large ore dragon slayer",
     &anvil::owners[17],
     10249,
     1436,
     8,
     {{{6928, 5}, {6572, 5}}},
     {6847},
     43,
     88,
     OutputForm::multiple,
     anvil::hot},
}};
inline constexpr auto formulas = [] {
  std::array<Formula, alchemy_formulas.size() + anvil_formulas.size()> all{};
  std::size_t index = 0;
  for (const auto& r : alchemy_formulas)
    all[index++] = r;
  for (const auto& r : anvil_formulas)
    all[index++] = r;
  return all;
}();
struct RecipeStorage {
  std::array<PackedInventoryTarget, 7> targets{};
  std::array<ItemAmount, 5> costs{};
  std::array<ItemAmount, 2> products{};
  std::size_t target_count = 0, cost_count = 0, product_count = 0;
};
inline constexpr auto storage = [] {
  std::array<RecipeStorage, formulas.size()> rows{};
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const auto& f = formulas[i];
    auto& row = rows[i];
    for (const auto& ingredient : f.ingredients)
      if (ingredient.units)
        row.costs[row.cost_count++] = ingredient;
    row.costs[row.cost_count++] = {f.reserve, 1};
    for (std::size_t j = 0; j < row.cost_count; ++j)
      row.targets[row.target_count++] = items[item_index(row.costs[j].symbol)].target;
    for (auto id : f.products)
      if (id) {
        row.products[row.product_count++] = {id, 1};
        row.targets[row.target_count++] = items[item_index(id)].target;
      }
    if (f.at_anvil()) {
      row.targets[row.target_count++] = items[item_index(anvil::cold)].target;
      row.targets[row.target_count++] = items[item_index(anvil::hammer)].target;
    }
  }
  return rows;
}();
struct Recipe : Formula {
  View<PackedInventoryTarget> targets;
  View<ItemAmount> costs, outputs;
  constexpr bool branched() const noexcept { return form == OutputForm::branched; }
  constexpr View<ItemAmount> selected_outputs(bool known, int value) const noexcept {
    if (!branched())
      return outputs;
    return known ? View<ItemAmount>{outputs.data + (value == 1), 1} : View<ItemAmount>{};
  }
};
inline constexpr auto recipes = [] {
  std::array<Recipe, formulas.size()> result{};
  for (std::size_t i = 0; i < result.size(); ++i) {
    const auto& row = storage[i];
    result[i] = {formulas[i],
                 {row.targets.data(), row.target_count},
                 {row.costs.data(), row.cost_count},
                 {row.products.data(), row.product_count}};
  }
  return result;
}();
inline constexpr const Recipe* callback(unsigned symbol) noexcept {
  for (const auto& recipe : recipes)
    if (recipe.effect->information->symbol == symbol)
      return &recipe;
  return nullptr;
}
} // namespace ubc::dndr
