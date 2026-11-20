// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/crafting_catalog.hpp"

namespace ubc {
// Tight passive evidence of the hot input object to the existing
// intake. Both workspaces belong to the same resting caller; no
// recursion or concurrent use. No engine call, no allocation, no blanket
// recipe/writer/destroy/end release, especially not for foreign tmp>0.
inline bool verify_cool_listed_input(const ReadOnlyMemory& memory,
    const CoolRecipeCatalogueCapture& source,PackedInventoryWorkspace& inventory,
    station_recipe_snapshot_detail::HandWorkspace& hands) noexcept {
  namespace cat=cool_recipe_catalog;
  const auto inv=read_packed_inventory(memory,source.key.hero,cat::targets,true,inventory);
  if(!inv.storage_totals_known||!inv.snapshot_unchanged||inv.issue!=PackedInventoryIssue::none||
    inv.total[0]<=0||inv.listed_occurrences[0]!=1||inv.packed_occurrences[0]||inv.packed[0]||
    inv.mixed_representation[0]||inv.has_active_marker[0]||inv.duplicate_packed_target[0])return false;
  for(std::size_t i=0;i<cat::targets.size();++i)
    if(inv.total[i]!=source.recipe.material.rows[i].stored)return false;
  namespace hand=station_recipe_snapshot_detail;
  if(hand::begin_hands(memory,source.key.hero,hands)!=StationHandReadIssue::none||
    !hand::verify_hands(memory,hands)||!same_crafting_hands(source.hands,hands.before))return false;
  std::size_t targets=0;
  for(std::size_t i=0;i<inv.listed_nodes;++i){const auto& record=inventory.records[i];
    const bool target=station_detail::little_i32(record.item_fields.data()+4)==cat::targets[0].symbol;
    if(target&&++targets!=1)return false;
    const auto object=station_detail::little_u32(record.node.data()+4);
    // Anchor must never be the same list object regardless of the instance.
    if(object==source.hands.interact_item)return false;
    for(std::size_t j=0;j<hands.count;++j){const auto& fields=hands.records[j].fields;
      if(object!=station_detail::little_u32(fields.data()+0x30))continue;
      // records contains ALL objects/slots. Only normal foreign equipment
      // with tmp0 is harmless: SetInteract compares the instance there.
      // Foreign tmp>0 takes without instance filter World/Release; unknown
      // negative levels remain closed as well. Hot never shares slots.
      if(target||station_detail::little_i32(fields.data()+0x18)!=0)return false;
    }
  }
  return targets==1;
}
} // namespace ubc
