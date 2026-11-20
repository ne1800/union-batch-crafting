// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/alchemy_extra_snapshot.hpp"
#include "ubc/station_recipe_material.hpp"

namespace ubc {
// Projection of ONE already fully read resting end-frame capture. No
// writer context is fabricated. The separate normal end AFTER witness
// remains input to observe_completion and is not replaced here.
struct AlchemyExtraEndFrame {
  alchemy_flask_debit::Context before{},after{};
  alchemy_flask_debit::Scope scope_before{},scope_after{};
  std::uint64_t serial=0;
  std::uint32_t current_thread=0;
  bool boundary_verified=false;
  std::int32_t production_before=-1,production_after=-1,invincible_before=-1,invincible_after=-1;
  StationRecipeHands hands_before{},hands_after{};
  RecipeInventory inventory{};
};
enum class AlchemyExtraEndIssue {none,descriptor,context,boundary,hands,inventory};
struct AlchemyExtraEndSnapshot {
  bool valid=false;AlchemyExtraEndIssue issue=AlchemyExtraEndIssue::none;
  alchemy_flask_debit::Observation observation{};
};
inline AlchemyExtraEndSnapshot alchemy_extra_end_snapshot(const alchemy_recipe_catalog::Descriptor& recipe,
    const alchemy_flask_debit::Job& job,const AlchemyExtraEndFrame& frame) noexcept {
  namespace d=alchemy_flask_debit;using I=AlchemyExtraEndIssue;
  AlchemyExtraEndSnapshot out;
  if(!alchemy_recipe_catalog::structurally_valid(recipe) || job.callback!=recipe.callback.function->symbol ||
     job.owner!=static_cast<std::uint32_t>(recipe.owner.instance) || !job.id || !job.writer_token || !job.quantity) {out.issue=I::descriptor;return out;}
  if(!d::valid(job.context) || !d::same(job.context,frame.before,false) || !d::same(frame.before,frame.after) ||
     frame.before.mob || frame.after.mob || frame.current_thread!=job.context.thread) {out.issue=I::context;return out;}
  const auto quiet=[](const d::Scope& s){return !s.writer_active && !s.writer_depth && !s.vm_depth && !s.debit_depth;};
  if(!frame.boundary_verified || !frame.serial || !quiet(frame.scope_before) || !quiet(frame.scope_after) ||
     frame.production_before || frame.production_after || frame.invincible_before || frame.invincible_after) {out.issue=I::boundary;return out;}
  const auto empty=[](const StationRecipeHands& h){return h.interact_item_known && !h.interact_item &&
    h.hands.left.state==0 && !h.hands.left.object && h.hands.right.state==0 && !h.hands.right.object;};
  if(!empty(frame.hands_before) || !empty(frame.hands_after)) {out.issue=I::hands;return out;}
  const auto& inventory=frame.inventory;
  if(!inventory.header_known || !inventory.listed_known || !inventory.packed_known || !inventory.storage_totals_known ||
     !inventory.stable_boundary_verified || !inventory.snapshot_unchanged || inventory.issue!=PackedInventoryIssue::none ||
     inventory.listed_nodes>max_packed_inventory_nodes || inventory.packed_entries>max_packed_inventory_entries ||
     inventory.items.count!=recipe.targets.count || !inventory.items.data) {out.issue=I::inventory;return out;}
  auto& v=out.observation;v.context=frame.after;v.scope=frame.scope_after;v.serial=frame.serial;
  v.production=v.invincible=0;v.hand={true,true,0,0,0,0};v.stocks.count=recipe.targets.count;
  std::uint64_t listed=0,packed=0;
  for(std::size_t i=0;i<recipe.targets.count;++i) {
    const auto symbol=recipe.targets[i].symbol;const auto* item=recipe_material_detail::item(inventory,symbol);
    if(!item || item->listed<0 || item->packed<0 || item->total<0 ||
       std::int64_t{item->listed}+item->packed!=item->total ||
       (item->listed>0 && !item->listed_occurrences) || ((item->packed>0)!=(item->packed_occurrences>0)) ||
       item->listed_occurrences>inventory.listed_nodes || item->packed_occurrences>inventory.packed_entries) {out.issue=I::inventory;return out;}
    listed+=item->listed_occurrences;packed+=item->packed_occurrences;
    v.stocks.rows[i]={static_cast<std::uint32_t>(symbol),item->total,true};
  }
  if(listed>inventory.listed_nodes || packed>inventory.packed_entries) {out.issue=I::inventory;return out;}
  // Parser/FlaskStack remain {}: finish() uses them outside of
  // writers. This is a read end state, not a new debit contract.
  v.stocks.complete_unambiguous=true;v.native_contract=v.stable_read=true;out.valid=true;return out;
}
} // namespace ubc
