// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/dndr_alchemy_profile.hpp"
#include "ubc/engine/script_window.hpp"
#include "ubc/engine/runtime_script.hpp"
#include <algorithm>

namespace ubc::dndr {
inline constexpr auto body_capacity = [] {
  std::size_t result = 0;
  for (const auto& r : recipes)
    result = (std::max)(result, static_cast<std::size_t>(r.effect->information->byte_length));
  return result;
}();
using AlchemyBody = ScriptWindow<body_capacity>;

inline std::uint32_t maximum(const AlchemyRecipe& recipe, const std::int32_t* stock,
                             bool branch_known = false, int branch = 0) noexcept {
  const auto outputs = recipe.selected_outputs(branch_known, branch);
  if (!outputs.count)
    return 0;
  std::uint64_t limit = INT32_MAX;
  for (std::size_t i = 0; i < recipe.targets.count; ++i) {
    if (stock[i] < 0)
      return 0;
    const auto symbol = recipe.targets[i].symbol;
    for (const auto& cost : recipe.costs)
      if (cost.symbol == symbol) {
        const auto available =
            static_cast<std::uint64_t>(stock[i]) + (symbol == static_cast<int>(flask));
        limit = (std::min)(limit, (std::min)(available, std::uint64_t{INT32_MAX}) / cost.units);
      }
    for (const auto& output : outputs)
      if (output.symbol == symbol)
        limit = (std::min)(limit, static_cast<std::uint64_t>(INT32_MAX - stock[i]) / output.units);
  }
  return static_cast<std::uint32_t>(limit);
}

// Prepare a batch inside the captured, hash-verified INFO. The ordinary single stays byte-identical.
// Keep the original ingredient tests, debits, branch and station end. Use the space occupied by
// the two status messages for the extra flask debit. Its failure precedes all ingredient debits.
// Both failure paths call the verified original END info, which returns the hand flask once.
inline bool prepare_batch(AlchemyBody& body, const AlchemyRecipe& r,
                          std::uint32_t quantity) noexcept {
  const auto& f = *r.effect->information;
  if (!quantity || body.length != f.byte_length || body.length > body_capacity)
    return false;
  for (const auto& cost : r.costs)
    if (quantity > INT32_MAX / cost.units)
      return false;
  if (quantity == 1)
    return true;
  constexpr std::uint8_t push_int = 64, call_external = 62, call_script = 61, branch_zero = 76,
                         ret = 60;
  const auto start = f.byte_offset;
  const auto first_debit = r.guard_jump + 5;
  const auto debits_size = (r.costs.count - 1) * 20;
  const auto source_output = r.branched() ? r.product_begin - 16 : r.product_begin;
  const auto product_size = r.branched() ? 61u : 20u;
  const auto debits_at = first_debit + 25;
  const auto output_at = debits_at + debits_size;
  const auto end_at = output_at + product_size;
  const auto failure = end_at + 6;
  const auto used = failure + 6;
  if (used > body.length || first_debit + debits_size > source_output ||
      source_output + product_size - (r.form == OutputForm::single ? 5 : 0) > body.length ||
      body.original[r.guard_jump] != branch_zero)
    return false;
  auto word = [&](std::size_t at, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
      body.prepared[at + i] = static_cast<std::uint8_t>(v >> (8 * i));
  };
  auto instruction = [&](std::size_t at, std::uint8_t op, std::uint32_t v) {
    body.prepared[at] = op;
    word(at + 1, v);
  };
  auto copy = [&](std::size_t to, std::size_t from, std::size_t length) {
    std::copy_n(body.original.data() + from, length, body.prepared.data() + to);
  };
  auto original_word = [&](std::size_t at) {
    return runtime_script::detail::u32(body.original.data() + at);
  };
  auto checked_units = [&](std::size_t symbol_at, std::size_t units_at) -> unsigned {
    for (const auto& cost : r.ingredients)
      if (cost.units && static_cast<unsigned>(cost.symbol) == original_word(symbol_at) &&
          original_word(units_at) == cost.units)
        return cost.units;
    return 0;
  };
  copy(debits_at, first_debit, debits_size);
  for (std::size_t i = 0; i + 1 < r.costs.count; ++i) {
    const auto guard = 21 * i, debit = first_debit + 20 * i;
    const auto minimum = checked_units(guard + 11, guard + 1),
               cost = checked_units(debit + 6, debit + 11);
    if (!minimum || !cost)
      return false;
    word(guard + 1, minimum * quantity);
    word(debits_at + 20 * i + 11, cost * quantity);
  }
  const auto fail_address = start + static_cast<std::uint32_t>(failure);
  word(r.guard_jump + 1, fail_address);
  copy(first_debit, first_debit, 20);
  word(first_debit + 6, flask);
  word(first_debit + 11, quantity - 1);
  instruction(first_debit + 20, branch_zero, fail_address);

  if (r.branched()) {
    copy(output_at, source_output, product_size);
    // Rebase both internal jumps in the unchanged KNOWS_BLOODFLY condition.
    word(output_at + 12, start + static_cast<unsigned>(output_at + 41));
    word(output_at + 37, start + static_cast<unsigned>(end_at));
    word(output_at + 27, quantity);
    word(output_at + 52, quantity);
  } else {
    copy(output_at, source_output, 10); // Keep the actor and product of the original call.
    instruction(output_at + 10, push_int, quantity);
    instruction(output_at + 15, call_external,
                170); // CreateInvItems also handles the singular recipe.
  }
  copy(end_at, body.length - 6, 6);
  instruction(failure, call_script, owners.front().information->byte_offset);
  body.prepared[failure + 5] = ret;
  body.prefix = used;
  return true;
}
} // namespace ubc::dndr
