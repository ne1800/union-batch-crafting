// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/lhiver_recipe_profile.hpp"
#include <array>
#include <cstdint>

namespace ubc::cool_destroy_abi {
// Only freshly proven identity data; no native release checker.
// Unchanged complete body plus separate Manipulate jump table. The
// remaining OnMessage jump tables are NOT bound here. These data
// prove neither dispatch exclusion nor AFTER or storage debit.
// Foreign hooks and later own hooks need a separate positive
// registry/trampoline proof; never omit the prefix en masse.
inline constexpr bool runtime_ready=false;
inline constexpr auto engine_file_sha256=lhiver::digest_literal("0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f");
struct CodeRange {std::uint32_t address,length;lhiver::Digest sha256;};
inline constexpr std::array<CodeRange,10> identity_ranges{{
  // oCNpc::OnMessage [function body segment +0/6647]
  {0x74b020,2048,lhiver::digest_literal("18bf0a7d33aa961c8263c1ef665ea28f03909f3304c05e567d6634266cbf926d")},
  // oCNpc::OnMessage [function body segment +2048/6647]
  {0x74b820,2048,lhiver::digest_literal("cd086cbdb9d682cc1f97b92ba79642016e1d6654a5e07284635dfe2b4e83984f")},
  // oCNpc::OnMessage [function body segment +4096/6647]
  {0x74c020,2048,lhiver::digest_literal("11abd62583c0d568f329cdfdc70f3ff599bc7662ba8e61fe1574c949c47c603a")},
  // oCNpc::OnMessage [function body segment +6144/6647]
  {0x74c820,503,lhiver::digest_literal("5dbc631d2529d320c950ca846023327301dcf29bf1347a302fe431a46ae24f6b")},
  // oCNpc::EV_DestroyInteractItem
  {0x754b40,460,lhiver::digest_literal("493ffa795e2d6826e86c3fb039f5091bb38e14ff2afa9b7362800534fbee4564")},
  // oCNpc::EV_CreateInteractItem
  {0x754890,687,lhiver::digest_literal("ab10f3df64ff7d210e9c8fe3b11e4b2163f580cba10a30aa824f033eae31ec64")},
  // oCNpc::SetInteractItem
  {0x74acc0,564,lhiver::digest_literal("e307a7e94552949f0fca7e31cb36b374bf058560f244ec3fa4c07f537983a707")},
  // oCItem::GetInstance
  {0x711420,7,lhiver::digest_literal("edefe40dfb0c9a0883df7eb4dab972f5fb212583cb6da3a8a9e00bf4f112f448")},
  // oCMsgManipulate::_GetClassDef
  {0x764780,6,lhiver::digest_literal("37891f44b16a4b468d888d3a2d6234cf499aaf574323fdf160438691631e0e27")},
  // oCNpc::OnMessage manipulate dispatch table (18 entries)
  {0x74cb38,72,lhiver::digest_literal("8f24d6c1929a1fd13b4aff17ab6f5e1c3df518104fedf88dd5142d973b33fc1b")},
}};
inline constexpr std::uint32_t destroy_entry=0x754b40;
inline constexpr std::uint32_t destroy_call=0x74c581;
inline constexpr std::uint32_t destroy_return=0x74c586;
inline constexpr std::uint32_t manipulate_classdef=0xab2860;
inline constexpr std::uint32_t manipulate_get_class=0x764780;
inline constexpr std::uint32_t message_subtype_offset=0x24;
inline constexpr std::uint32_t destroy_subtype=0x9;
inline constexpr std::uint32_t npc_anchor_offset=0x968;
inline constexpr std::uint32_t npc_inventory_offset=0x668;
inline constexpr std::uint32_t item_instance_offset=0x330;
inline constexpr std::uint32_t item_get_instance=0x711420;
inline constexpr std::uint32_t set_interact_item_call=0x754b76;
inline constexpr std::uint32_t set_interact_item_return=0x754b7b;
inline constexpr std::uint32_t inventory_remove_call=0x754b92;
inline constexpr std::uint32_t inventory_remove_return=0x754b98;
inline constexpr std::uint32_t inventory_remove_amount=0x1;
} // namespace ubc::cool_destroy_abi
