// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/forge/dndr_forge.hpp"
#include "ubc/engine/packed_inventory.hpp"
#include "ubc/stations/station_recipe_snapshot.hpp"
#include <string_view>

namespace ubc::dndr::forge {
inline constexpr std::array<PackedInventoryTarget, 2> steel{
    {{"ITMISWORDRAW", cold}, {"ITMISWORDRAWHOT", hot}}};

inline Hand hand_state(const StationRecipeHands& h, const HandObjectSnapshot& anchor) noexcept {
  const auto& left = h.hands.left;
  if (!h.interact_item_known || h.hands.right.state != 0 || left.state < 0 || anchor.state < 0)
    return Hand::unknown;
  if (!left.state && !anchor.state)
    return Hand::empty;
  if (anchor.state != 1 || anchor.amount != 1)
    return Hand::unknown;
  if (!left.state && anchor.symbol == cold)
    return Hand::reserved;
  if (left.state != 1 || left.object != anchor.object || left.amount != 1 ||
      left.symbol != anchor.symbol)
    return Hand::unknown;
  if (left.symbol == cold)
    return Hand::left_cold;
  return left.symbol == hot ? Hand::left_hot : Hand::unknown;
}

inline Stock stock_from(const PackedInventorySnapshotT<2>& inv,
                        const PackedInventoryWorkspace& work, Hand hand) noexcept {
  Stock out;
  out.hand = hand;
  if (inv.issue != PackedInventoryIssue::none || !inv.storage_totals_known ||
      !inv.snapshot_unchanged || !inv.stable_boundary_verified || hand == Hand::unknown)
    return out;
  for (std::size_t i = 0; i < steel.size(); ++i)
    if (inv.mixed_representation[i] || inv.packed_occurrences[i] > 1 ||
        inv.listed_occurrences[i] > 1 || inv.has_active_marker[i])
      return out;
  // Compare all unrelated entries, including packed names, amounts and equipped markers.
  // Removing a steel entry can change list links and packed-string offsets.
  Sha256 digest;
  for (std::uint32_t i = 0; i < inv.listed_nodes; ++i) {
    const auto& fields = work.records[i].item_fields;
    const int symbol = packed_detail::little_i32(fields.data() + 4);
    if (symbol != cold && symbol != hot)
      digest.update(fields.data(), fields.size());
  }
  constexpr char separator = '\0';
  digest.update(&separator, 1);
  std::string_view packed(reinterpret_cast<const char*>(work.packed_bytes.data()));
  while (!packed.empty()) {
    const auto comma = packed.find(',');
    const auto entry = packed.substr(0, comma);
    const auto name = entry.substr(0, entry.find(':'));
    if (name != steel[0].name && name != steel[1].name) {
      digest.update(entry.data(), entry.size());
      digest.update(&separator, 1);
    }
    if (comma == packed.npos)
      break;
    packed.remove_prefix(comma + 1);
  }
  out.other = digest.finish();
  out.cold_count = inv.total[0];
  out.hot_count = inv.total[1];
  out.unambiguous = true;
  return out;
}
} // namespace ubc::dndr::forge
