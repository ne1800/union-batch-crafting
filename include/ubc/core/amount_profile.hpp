// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/engine/amount_patch.hpp"
#include "ubc/catalogs/lhiver_recipe_profile.hpp"

// Separate quantity contract. Original selection alone does not authorize
// parameterization. Only verified PUSHINT operands within an exclusive
// synchronous CallFunc order; no original code blocks in the source project.
namespace ubc::amount_profile {
inline constexpr std::array<AmountOperand, 3> meat{{{1, 1, 1, 0}, {37, 1, 1, 0}, {57, 1, 1, 0}}};
inline constexpr std::array<AmountOperand, 6> healing{
    {{1, 5, 1, 0}, {22, 5, 1, 0}, {59, 5, 1, 0}, {79, 5, 1, 0}, {99, 5, 1, 0}, {119, 4, 1, 1}}};
struct Entry {
  std::uint32_t function_index;
  AmountBody body;
};
inline constexpr std::array<Entry, 3> entries{
    {{1,
      {lhiver::functions[1].byte_length, lhiver::functions[1].body_sha256, meat.data(),
       meat.size()}},
     {8, {lhiver::functions[8].byte_length, lhiver::functions[8].body_sha256, nullptr, 0}},
     {10,
      {lhiver::functions[10].byte_length, lhiver::functions[10].body_sha256, healing.data(),
       healing.size()}}}};
inline const Entry* find(std::uint32_t symbol) noexcept {
  for (const auto& entry : entries)
    if (lhiver::functions[entry.function_index].symbol == symbol)
      return &entry;
  return nullptr;
}
} // namespace ubc::amount_profile
