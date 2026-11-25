// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/ui/crafting_menu_code.hpp"
namespace ubc_native::forge_profile {
// Original entry and message paths, including the player input branches before the debit.
inline constexpr std::array<ubc::crafting_ui::CodeSite, 15> routes{{
    {0x721f00, 1025,
     ubc::lhiver::digest_literal(
         "8cdcb13a6dcd4c8e8fbacf93be57d22fc88663dc4810f987cb5403a599f9241c"),
     "forge_entry_code"},
    {0x6990b8, 135,
     ubc::lhiver::digest_literal(
         "700877cf9ea37e901dd3fda9057c1e4da283ee598bf5488f21609007e1ed3350"),
     "forge_input_code"},
    {0x6993c4, 124,
     ubc::lhiver::digest_literal(
         "4db7d4fccb0adfc30999a35997b422a0beaab6e4e08b94fcc66c6200f53c605d"),
     "forge_input_alt_code"},
    {0x698610, 69,
     ubc::lhiver::digest_literal(
         "30966d35a1196e8569270e7454b5d3ca71f4adaffe59acbb2d85c68959bc4d5d"),
     "forge_direction_code"},
    {0x71f218, 964,
     ubc::lhiver::digest_literal(
         "ffc46a0aad24c29baf6418a914d92bf52fc2a4c2c53be80594c2a64f52f39638"),
     "forge_interact_code"},
    {0x71dc30, 790,
     ubc::lhiver::digest_literal(
         "2e833032dd86a9172cd74b67a8f9933be94a63ea67c62aa807d35cccce058c81"),
     "forge_place_scan_code"},
    {0x7544e0, 518,
     ubc::lhiver::digest_literal(
         "3a9b0527dc924147f1afe2f599fd92de2bf129758240713cc0cc26b8c1860485"),
     "forge_insert_code"},
    {0x7546f0, 171,
     ubc::lhiver::digest_literal(
         "20432ffaa339b0b7ee8ef77f54f8f45d16e25afcc2f659fe24e7163673dee940"),
     "forge_exchange_entry_code"},
    {0x7547a0, 101,
     ubc::lhiver::digest_literal(
         "f8926689e19632e21358029f19882d8164399b28525d544149fb851ed48991a3"),
     "forge_exchange_slot_code"},
    {0x75480a, 108,
     ubc::lhiver::digest_literal(
         "ecbe39888dbed733e0db10d6f48b337539d641a941ee0db90d188b9b420fd379"),
     "forge_exchange_return_code"},
    {0x74acc0, 246,
     ubc::lhiver::digest_literal(
         "3049fe53afda58129a80a8dd284eb435857b206d1e1bc17a8e9cc71e547fae88"),
     "forge_anchor_entry_code"},
    {0x74adbb, 313,
     ubc::lhiver::digest_literal(
         "72edf1d6a8dc8a80a0c4d396b2f5000acf752995a8fc1515e034de5afe464ee1"),
     "forge_anchor_return_code"},
    {0x74cb38, 72,
     ubc::lhiver::digest_literal(
         "8f24d6c1929a1fd13b4aff17ab6f5e1c3df518104fedf88dd5142d973b33fc1b"),
     "forge_dispatch_code"},
    {0x754890, 687,
     ubc::lhiver::digest_literal(
         "ab10f3df64ff7d210e9c8fe3b11e4b2163f580cba10a30aa824f033eae31ec64"),
     "forge_remove_code"},
    {0x749350, 357,
     ubc::lhiver::digest_literal(
         "047d8df6c814ae187d00d7d5186c124eb13e7e7bae6c27babae88cb809ad684c"),
     "forge_put_in_inventory_code"},
}};
} // namespace ubc_native::forge_profile
