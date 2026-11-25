// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/alchemy_recipe_catalog.hpp"
#include "ubc/catalogs/dndr_tobacco_contract.hpp"
#include <algorithm>

namespace ubc::tobacco {
using namespace recipe_catalog;
inline constexpr int apple = 16660, flask = 16604;
// Checked function boundaries and fingerprints from the L'Hiver DAT.
inline constexpr std::array<lhiver::Function, 12> functions{{
    {"PC_ITMI_TABAK", 20399, 734763, 67,
     lhiver::digest_literal("1a03fe2b82ab9bb8ad771db4545195efcbd2079fd2e1a4dad5b04e7bb4f25904"), 0,
     7, 1, 0, 1649},
    {"PC_ITMI_TABAK_CONDITION", 20400, 734830, 57,
     lhiver::digest_literal("ec3b534b44da3bc0fc17933aee27dbcc62ebec8e42e12648190bc8df2c3b5b1b"), 0,
     5, 3, 2, -1},
    {"PC_ITMI_TABAK_INFO", 20401, 734887, 189,
     lhiver::digest_literal("5a5e517fff872a16351c88fdccd8f4307e7e7b5e1116b80d2e6c92d8738160a0"), 0,
     5, 1, 0, -1},
    {"PC_ITMI_TABAK_BACK", 20402, 735076, 11,
     lhiver::digest_literal("88d4cc7d8b2e2321634533da70061257e93488a4e0c25f9d69ca6c5578b3af77"), 0,
     5, 1, 0, -1},
    {"PC_ITMI_TABAK_HONEY", 20403, 735087, 36,
     lhiver::digest_literal("2f1e7835e4a9e2c23e8dff997daa4c2784cd6bf88e8cf64f20f0f0341edf7123"), 0,
     5, 1, 0, -1},
    {"PC_ITMI_TABAK_SWAMPHERB", 20404, 735123, 36,
     lhiver::digest_literal("9b0768b033ab2fbd1c612e5df9189c02491b919f9f071fd4566498b940a2ab75"), 0,
     5, 1, 0, -1},
    {"PC_ITMI_TABAK_MUSHROOM_01", 20405, 735159, 36,
     lhiver::digest_literal("af5e33831aeebf3a0f6bfda708015b21575c54bf7f868a001dca340a07151a05"), 0,
     5, 1, 0, -1},
    {"ITMI_APFELTABAK", 16660, 371236, 114,
     lhiver::digest_literal("2924f51cd4be6d2d256272436cfaddc51f77adfd89b0c35d59d194384a948c3e"), 0,
     7, 1, 0, 1584},
    {"ITMI_PILZTABAK", 16661, 371350, 114,
     lhiver::digest_literal("78a150f2eb29b2a91ad111080b20f801cfeb1c5d2da2a6a5254af847bca04987"), 0,
     7, 1, 0, 1584},
    {"ITMI_HONIGTABAK", 16663, 371578, 114,
     lhiver::digest_literal("b9a043ea14c66c21ef440ebf97c28dfda42311426c61ed294562bdf49245cf20"), 0,
     7, 1, 0, 1584},
    {"ITMI_SUMPFTABAK", 16664, 371692, 114,
     lhiver::digest_literal("2a06c8f9f888714612ef4c24ac3c5c47e4e6ae538bdf4f9debe0e06eb505424c"), 0,
     7, 1, 0, 1584},
    {"ITFO_HONEY", 16393, 331680, 148,
     lhiver::digest_literal("fbb782bd8cf994eaf18a09e20255586feced047d4a1e570f637e1914f06c50cc"), 0,
     7, 1, 0, 1584},
}};
inline constexpr std::array<Requirement, 3> conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::inventory_at_least, RequirementRole::resource, apple, 0, 1},
     {RequirementKind::integer_equal, RequirementRole::context, 20386, 0, 1}}};
inline constexpr Owner owner{
    20399,           10, 22243, false, false, true, &functions[0], &functions[1], &functions[2],
    view(conditions)};
inline constexpr std::array<ItemAmount, 2> costs{{{apple, 1}, {flask, 1}}};
inline constexpr std::array<AmountOperand, 1> output_operand{{{11, 1, 1, 0}}};
struct Profile {
  const Owner& owner;
  int back;
  unsigned hero;
  std::array<ItemAmount, 2> costs;
  constexpr int info() const noexcept { return static_cast<int>(owner.information->symbol); }
  constexpr bool choice(int symbol) const noexcept { return symbol > back && symbol <= back + 3; }
};
inline constexpr Profile marcello_profile{owner, 20402, 1683, costs};
inline constexpr Profile dndr_profile{
    dndr_tobacco::owner, 10136, dndr::hero, {{{6956, 1}, {dndr::flask, 1}}}};
struct Descriptor {
  std::uint32_t id;
  const char* name;
  std::array<PackedInventoryTarget, 4> targets;
  std::array<ItemAmount, 1> outputs;
  Callback callback;
  const Profile& profile;
};
constexpr Descriptor recipe(std::uint32_t id, const char* name, PackedInventoryTarget additive,
                            const lhiver::Function& product, const lhiver::Function& choice,
                            const Profile& profile = marcello_profile) noexcept {
  return {id,
          name,
          {{{"ITMI_APFELTABAK", profile.costs[0].symbol},
            {"ITMI_FLASK", profile.costs[1].symbol},
            additive,
            {product.name, static_cast<int>(product.symbol)}}},
          {{{static_cast<int>(product.symbol), 1}}},
          {&choice,
           profile.owner.instance,
           Invocation::choice,
           1,
           1,
           INT32_MAX,
           {choice.byte_length, choice.body_sha256, output_operand.data(), output_operand.size()},
           {},
           {},
           false,
           true,
           false,
           true},
          profile};
}
inline constexpr std::array<Descriptor, 3> recipes{
    {recipe(0xb001, "Honigtabak", {"ITFO_HONEY", 16393}, functions[9], functions[4]),
     recipe(0xb002, "Sumpftabak", {"ITPL_SWAMPHERB", 16706}, functions[10], functions[5]),
     recipe(0xb003, "Pilztabak", {"ITPL_MUSHROOM_01", 16726}, functions[8], functions[6])}};
inline constexpr std::array<Descriptor, 3> dndr_recipes{
    {recipe(0x1b001, "Honey tobacco", {"ITFO_HONEY", 6734}, dndr_tobacco::functions[9],
            dndr_tobacco::functions[4], dndr_profile),
     recipe(0x1b002, "Swampweed tobacco", {"ITPL_SWAMPHERB", 6999}, dndr_tobacco::functions[10],
            dndr_tobacco::functions[5], dndr_profile),
     recipe(0x1b003, "Mushroom tobacco", {"ITPL_MUSHROOM_01", 7019}, dndr_tobacco::functions[8],
            dndr_tobacco::functions[6], dndr_profile)}};
inline bool canonical(const Descriptor& recipe) noexcept {
  for (const auto& known : (&recipe.profile == &dndr_profile ? dndr_recipes : recipes))
    if (&known == &recipe)
      return true;
  return false;
}
// Warehouse stocks from entry at the LAB. A flask is already in hand.
using Stock = std::array<std::int32_t, 4>;
inline std::uint32_t maximum(const Stock& stock) noexcept {
  if (*std::min_element(stock.begin(), stock.end()) < 0 || !stock[2])
    return 0;
  return static_cast<std::uint32_t>(
      std::min<std::int64_t>({stock[0], std::int64_t{stock[1]} + 1, INT32_MAX - stock[3]}));
}
} // namespace ubc::tobacco
