// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/dndr_alchemy_profile.hpp"
#include "ubc/engine/script_window.hpp"
#include <algorithm>

namespace ubc::dndr {
inline std::uint32_t maximum(const AlchemyRecipe& recipe,
                             const std::array<std::int32_t, 4>& stock) noexcept {
  std::uint64_t limit = INT32_MAX;
  for (std::size_t i = 0; i < stock.size(); ++i) {
    if (stock[i] < 0)
      return 0;
    const auto symbol = recipe.targets[i].symbol;
    for (const auto& cost : recipe.costs)
      if (cost.symbol == symbol) {
        const auto available =
            static_cast<std::uint64_t>(stock[i]) + (symbol == static_cast<int>(flask));
        limit = (std::min)(limit, (std::min)(available, std::uint64_t{INT32_MAX}) / cost.units);
      }
    for (const auto& output : recipe.outputs)
      if (output.symbol == symbol)
        limit = (std::min)(limit, static_cast<std::uint64_t>(INT32_MAX - stock[i]) / output.units);
  }
  return static_cast<std::uint32_t>(limit);
}
using ManaBody = ScriptWindow<159>;
// Both paths use the original CreateInvItems and station end. In a batch
// the spare message instructions debit the additional flasks before the
// product is created. The conditional consumes the native INT result.
inline bool prepare_mana(ManaBody& body, std::uint32_t n) noexcept {
  if (!n || n > INT32_MAX / 2)
    return false;
  if (n == 1)
    return true;
  auto word = [&](unsigned offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
      body.prepared[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
  };
  for (const auto& op : mana_operands)
    word(op.offset, op.units * n);
  const auto start = recipes[0].effect->information->byte_offset;
  std::copy_n(body.original.data() + 48, 20, body.prepared.data() + 88);
  word(94, flask);
  word(99, n - 1);
  body.prepared[108] = 76; // BZ: on a failed debit, refund the interaction flask.
  word(109, start + 133);
  std::copy_n(body.original.data() + 88, 15, body.prepared.data() + 113);
  word(124, n);
  body.prepared[128] = 75; // Success skips the three refund arguments.
  word(129, start + 148);
  word(44, start + 133);
  body.prefix = 133;
  return true;
}
} // namespace ubc::dndr
