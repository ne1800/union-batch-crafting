// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/engine/hand_snapshot.hpp"

namespace ubc::generated_blade_hand_shape {
// Only generated hot/cold blades in an NPC slot. The stored constructor
// state remains 0x200001. PutInSlot(TNpcSlot*,oCVob*,0) sets
// ITM_FLAG_NFOCUS (API-Pin 65fb327c76f58c751468c8e7df917a86a89b9ed1):
// EXE 0655b481...47c7f, Create-Call 0x754AE7 -> 0x749D80,
// SetFlag-Call 0x749FB0 -> 0x712710 (OR item+0x158,0x800000).
// RemoveFromSlot clears exactly this bit via 0x74A38D -> 0x7126F0.
// No masking of flags read and no evidence of Create, Destroy or end.
inline constexpr std::uint32_t stored_flags=0x200001;
inline constexpr std::uint32_t flags=0xa00001;
inline bool matches(std::int32_t symbol,std::uint32_t type,
    std::uint32_t mainflag,std::uint32_t actual_flags) noexcept {
  return (symbol==16612||symbol==16613)&&type==hand_detail::item_vtable&&
    mainflag==1&&actual_flags==flags;
}
} // namespace ubc::generated_blade_hand_shape
