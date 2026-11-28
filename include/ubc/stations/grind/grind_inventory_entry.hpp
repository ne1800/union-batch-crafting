// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/alchemy/alchemy_extra_dependencies.hpp"

namespace ubc::grind_inventory_entry {
namespace dep = alchemy_extra_dependencies;
namespace z = zbug_end_compat;
inline constexpr dep::CodeRange unpack{
    0x710a20, 0x553,
    lhiver::digest_literal("cd313570e6834dfedb613224c15a31f528235f844d7b2dda8518a5aa3e2d5784"),
    dep::RangeKind::complete_function};
inline constexpr std::uint32_t put_call = 0x710e74;

inline bool verify(const ReadOnlyMemory& memory, const z::Binding& binding) noexcept {
  const auto forward = binding.zbug.base + z::wrapper_rva;
  const auto read_body = [&](bool& patched) {
    return dep::detail::patched_call(memory, unpack, put_call - unpack.address,
                                     z::original_put_in_inv, forward, patched)
        .valid;
  };
  bool patched = false;
  if (!read_body(patched))
    return false;
  if (!patched)
    return true;
  // The same PutInInv wrapper serves station exit and unpacking a saved inventory.
  // Recheck the complete unpack routine after verifying the wrapper and its dispatch.
  const auto route = z::verify(memory, binding);
  return route.valid && route.call_target == forward &&
         route.original_target == z::original_put_in_inv && read_body(patched) && patched;
}
} // namespace ubc::grind_inventory_entry
