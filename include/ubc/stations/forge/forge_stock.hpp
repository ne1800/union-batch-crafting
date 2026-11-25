// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/forge/dndr_forge.hpp"
#include "ubc/engine/packed_inventory.hpp"
#include "ubc/stations/station_recipe_snapshot.hpp"
#include <string_view>

namespace ubc::dndr::forge {
struct StationProfile {
  Station kind;
  const char* log_prefix;
  const char* scheme;
  const char* working_animation;
  std::array<PackedInventoryTarget, 2> items;
};
inline constexpr StationProfile smithy{Station::forge,
                                       "forge",
                                       "BSFIRE",
                                       "S_BSFIRE_S1",
                                       {{{"ITMISWORDRAW", cold}, {"ITMISWORDRAWHOT", hot}}}};
inline constexpr StationProfile pan{
    Station::pan, "pan", "PAN", "S_PAN_S1", {{{"ITFOMUTTONRAW", 6719}, {"ITFOMUTTON", 6721}}}};
inline constexpr StationProfile stove{Station::stove, "stove", "STOVE", "S_STOVE_S1", pan.items};
inline constexpr std::array<PackedInventoryTarget, 3> stove_items{
    {stove.items[0], stove.items[1], {"ITMI_PANFULL", 6910}}};

inline Hand hand_state(const StationRecipeHands& h, const HandObjectSnapshot& anchor,
                       const StationProfile& profile) noexcept {
  const auto& left = h.hands.left;
  const auto& right = h.hands.right;
  if (!h.interact_item_known || left.state < 0 || right.state < 0 || anchor.state < 0)
    return Hand::unknown;
  if (profile.kind == Station::stove) {
    // The Night of the Raven stove creates the right-hand pan at level 1.
    // Its recorded CREATE and S1 states only establish that lifetime.
    if (!h.left_slot_known || !h.right_slot_known || h.left_tmp_level != 0 ||
        h.right_tmp_level != (right.state ? 1 : 0) || h.interact_item != anchor.object)
      return Hand::unknown;
    if (anchor.state == 1 && anchor.symbol == stove.items[0].symbol) {
      if (!anchor.object || anchor.amount != 1 || left.state)
        return Hand::unknown;
      if (!right.state)
        return Hand::reserved;
      return right.state == 1 && right.symbol == stove_items[2].symbol && right.amount == 1 &&
                     right.object && right.object != anchor.object
                 ? Hand::right_pan
                 : Hand::unknown;
    }
  }
  if (!left.state && !right.state && !anchor.state)
    return Hand::empty;
  if (anchor.state != 1 || anchor.amount != 1)
    return Hand::unknown;
  const bool raw = anchor.symbol == profile.items[0].symbol;
  if (!left.state && !right.state && raw)
    return Hand::reserved;
  const bool in_right = raw && profile.kind == Station::pan;
  const auto& held = in_right ? right : left;
  const auto& free = in_right ? left : right;
  if (free.state || held.state != 1 || held.object != anchor.object || held.amount != 1 ||
      held.symbol != anchor.symbol)
    return Hand::unknown;
  if (raw)
    return inserted_hand(profile.kind);
  return held.symbol == profile.items[1].symbol ? exchanged_hand(profile.kind) : Hand::unknown;
}

template <std::size_t N>
inline Stock stock_from(const PackedInventorySnapshotT<N>& inv,
                        const PackedInventoryWorkspace& work, Hand hand,
                        const StationProfile& profile = smithy, std::uint32_t anchor = 0,
                        std::uint32_t prop = 0) noexcept {
  Stock out;
  out.hand = hand;
  if (inv.issue != PackedInventoryIssue::none || !inv.storage_totals_known ||
      !inv.snapshot_unchanged || !inv.stable_boundary_verified || hand == Hand::unknown)
    return out;
  if (inv.target_count != (profile.kind == Station::stove ? 3u : 2u) || inv.target_count > N)
    return out;
  for (std::size_t i = 0; i < profile.items.size(); ++i)
    if (inv.mixed_representation[i] || inv.packed_occurrences[i] > 1 ||
        inv.listed_occurrences[i] > 1 || inv.has_active_marker[i])
      return out;
  // Compare all unrelated entries, including packed names, amounts and equipped markers.
  // Removing a material entry can change list links and packed-string offsets.
  Sha256 digest;
  for (std::uint32_t i = 0; i < inv.listed_nodes; ++i) {
    if (profile.kind == Station::stove) {
      const auto item = packed_detail::little_u32(work.records[i].node.data() + 4);
      if (item == anchor || item == prop)
        return out;
    }
    const auto& fields = work.records[i].item_fields;
    const int symbol = packed_detail::little_i32(fields.data() + 4);
    if (symbol != profile.items[0].symbol && symbol != profile.items[1].symbol)
      digest.update(fields.data(), fields.size());
  }
  constexpr char separator = '\0';
  digest.update(&separator, 1);
  std::string_view packed(reinterpret_cast<const char*>(work.packed_bytes.data()));
  while (!packed.empty()) {
    const auto comma = packed.find(',');
    const auto entry = packed.substr(0, comma);
    const auto name = entry.substr(0, entry.find(':'));
    if (name != profile.items[0].name && name != profile.items[1].name) {
      digest.update(entry.data(), entry.size());
      digest.update(&separator, 1);
    }
    if (comma == packed.npos)
      break;
    packed.remove_prefix(comma + 1);
  }
  out.other = digest.finish();
  out.input_count = inv.total[0];
  out.output_count = inv.total[1];
  if (profile.kind == Station::stove) {
    out.stored_pans = inv.total[2];
    // DestroyInteractItem can also remove a stored instance. Even a listed
    // zero-quantity PANFULL is an entry, independent of the material totals.
    out.stored_pan_entry = inv.listed_occurrences[2] || inv.packed_occurrences[2] ||
                           inv.duplicate_packed_target[2] || inv.has_active_marker[2] ||
                           inv.mixed_representation[2];
    out.anchor = anchor;
    out.prop = prop;
  }
  out.unambiguous = true;
  return out;
}
} // namespace ubc::dndr::forge
