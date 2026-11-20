// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/cool_destroy_abi.hpp"
namespace ubc::cool_destroy_dispatch_profile {
// Full normal control-flow route for real Manipulate9 under the
// explicit reader guards. No masked foreign hook, no overall OnMessage.
using CodeRange=cool_destroy_abi::CodeRange;
inline constexpr std::array<CodeRange,14> ranges{{
  // entry_skip_gate
  {0x74b020,95,lhiver::digest_literal("c58153caafaafa8724986fc64425dd545020e2480b4eddf0c8edc75fa7b47229")},
  // npc_ready_gate
  {0x74b090,18,lhiver::digest_literal("6576b4cb5a22d1595f3611cf2079a08199e7c671ab71f5948d57933ac435d67c")},
  // animation_ref_gate
  {0x74b0e7,31,lhiver::digest_literal("34def6270e126210ffe5de9c80452783f30bb4ac1630f0e31435fb44afe34c76")},
  // cutscene_damage_gates
  {0x74b1a4,83,lhiver::digest_literal("d0b60350ea9eb35f7a0732652b73da5fba617710dad8335a7615a441d4726977")},
  // movement_attack_gates
  {0x74ba9c,63,lhiver::digest_literal("09a9e40532740e7301334e602525f04681a632135e70ba2051c1dbc88bf77cfc")},
  // conversation_weapon_gates
  {0x74bce0,71,lhiver::digest_literal("6745617bbd4f6098e7a7d00ac9fa4d065efaf057e0a5138ec3b96f26c227bcfa")},
  // manipulate_gate
  {0x74c329,25,lhiver::digest_literal("42ad745028ce9e5b2b546812d06ac3f0c8fa3d0e241305b978ddd858d46c3884")},
  // subtype_dispatch
  {0x74c368,20,lhiver::digest_literal("f7e787509cda76415cbb124ff11df37d371291dc144986625e61a1ed0342a388")},
  // destroy_call_return
  {0x74c57e,13,lhiver::digest_literal("d4e5303f61899752445d874e38985cf57181a345c79af846003f0f28593086c6")},
  // delete_gate
  {0x74c6c4,21,lhiver::digest_literal("abed98dd479412b3c30cc82c5eb8e3b309bceea9541337e3945d2e933843e63f")},
  // release_epilogue
  {0x74c9e5,50,lhiver::digest_literal("592a0f939e243321e9064066c58d51640df33651a29f5033ad124fc83067bf89")},
  // class_getter
  {0x764780,6,lhiver::digest_literal("37891f44b16a4b468d888d3a2d6234cf499aaf574323fdf160438691631e0e27")},
  // message_delete
  {0x763100,5,lhiver::digest_literal("edf971bd33c72bb590c3922272230e158144dc5ae626e2c5627cfa7a8d463707")},
  // complete_manipulate_table
  {0x74cb38,72,lhiver::digest_literal("8f24d6c1929a1fd13b4aff17ab6f5e1c3df518104fedf88dd5142d973b33fc1b")},
}};
inline constexpr std::array<std::uint32_t,4> class_chain{{0xab2860,0xab2b60,0x9a3698,0x8d8cd8}};
inline constexpr std::uint32_t npc_vtable=0x83d724,message_vtable=0x83dd9c;
inline constexpr std::uint32_t on_message=0x74b020,class_getter=0x764780,message_delete=0x763100;
} // namespace ubc::cool_destroy_dispatch_profile
