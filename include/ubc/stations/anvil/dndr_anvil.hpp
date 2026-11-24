// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/alchemy/dndr_alchemy.hpp"

namespace ubc::dndr {
namespace anvil {
inline constexpr auto& spare = *owners[10].information;
inline constexpr auto& sibling = *owners[11].information;
inline constexpr unsigned helper_begin = 71;

struct Instructions {
  AlchemyBody& body;
  std::size_t pos;
  void emit(std::uint8_t op, std::uint32_t operand) noexcept {
    body.prepared[pos++] = op;
    for (unsigned i = 0; i < 4; ++i)
      body.prepared[pos++] = static_cast<std::uint8_t>(operand >> (8 * i));
  }
  void args(int item, std::uint32_t units) noexcept {
    emit(67, hero);
    emit(64, static_cast<std::uint32_t>(item));
    emit(64, units);
  }
  void minimum(int item, std::uint32_t count, std::uint32_t failure) noexcept {
    emit(64, count);
    emit(67, hero);
    emit(64, static_cast<std::uint32_t>(item));
    emit(62, 160);             // Npc_HasItems
    body.prepared[pos++] = 18; // greater or equal
    emit(76, failure);
  }
  void reserve(std::uint32_t count, std::uint32_t failure) noexcept {
    // Npc_RemoveInvItems can partially remove a positive undersized stack.
    minimum(hot, count, failure);
    args(hot, count);
    emit(62, 184);
    emit(76, failure);
  }
};
} // namespace anvil

// Both exits use the original END: its entry refunds one cold blank. Its CreateInvItems
// instruction accepts three success arguments instead. Both close and clear the categories.
inline bool prepare_anvil_batch(AlchemyBody& body, const Recipe& r, std::uint32_t quantity,
                                AlchemyBody* reserve_body = nullptr) noexcept {
  if (!r.at_anvil() || !quantity || body.length != r.effect->information->byte_length ||
      body.length > body_capacity || r.outputs.count != 1 || !r.costs.count || r.costs.count > 3)
    return false;
  for (const auto& cost : r.costs)
    if (!cost.units || quantity > INT32_MAX / cost.units)
      return false;
  if (quantity == 1)
    return true;
  const auto refund = anvil::owners.front().information->byte_offset;
  const auto ingredients = r.costs.count - 1;
  anvil::Instructions tx{body, 0};
  if (!ingredients) {
    // A normal INFO has only 47 bytes. Keep the auxiliary ore INFO callable at its
    // regular entry by redirecting its identical success and failure tails to its
    // sibling. No other original entry or jump targets the resulting spare suffix.
    if (!reserve_body || body.length != 47 || reserve_body->length != anvil::spare.byte_length ||
        reserve_body->length != 128 || anvil::sibling.byte_length != 128)
      return false;
    anvil::Instructions aux{*reserve_body, 21};
    aux.emit(76, anvil::sibling.byte_offset + 81);
    aux.pos = 66;
    aux.emit(75, anvil::sibling.byte_offset + 66);
    tx.emit(75, anvil::spare.byte_offset + anvil::helper_begin);
    aux.reserve(quantity - 1, refund);
    aux.emit(75, r.effect->information->byte_offset + static_cast<unsigned>(tx.pos));
    reserve_body->prefix = aux.pos;
  } else {
    if (71 + ingredients * 46 > body.length)
      return false;
    for (std::size_t i = 0; i < ingredients; ++i)
      tx.minimum(r.costs[i].symbol, r.costs[i].units * quantity, refund);
    tx.reserve(quantity - 1, refund);
    for (std::size_t i = 0; i < ingredients; ++i) {
      tx.args(r.costs[i].symbol, r.costs[i].units * quantity);
      tx.emit(62, 184);
    }
  }
  tx.args(r.outputs[0].symbol, quantity);
  tx.emit(75, refund + 15);
  body.prefix = tx.pos;
  return true;
}
} // namespace ubc::dndr
