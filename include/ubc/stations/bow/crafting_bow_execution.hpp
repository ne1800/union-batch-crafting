// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/core/crafting_handoff.hpp"
#include "ubc/stations/bow/bow_item_runtime.hpp"

namespace ubc {
// Only the binding of an already fully read item path evidence to
// the specific quantity order. No module release from a bool alone.
inline bool matching_bow_item_path(const bow_item_runtime::Evidence& proof,
    const CraftingRequest& request,std::uint64_t frame) noexcept {
  const auto* d=crafting_catalog::find(request.id);
  if(!d||!d->bow||!crafting_handoff_detail::valid_request(request)||!frame||
     !proof.item_path_verified||proof.issue!=bow_item_runtime::Issue::none||!proof.stock.captured||
     !proof.modules.observed||!proof.constructors.values_verified||!proof.pool.verified||
     !proof.sort.stable||!proof.world.stable||!proof.renderer.verified||
     !proof.stock.plan.valid||proof.stock.plan.id!=d->bow->id||proof.stock.plan.quantity!=request.quantity||
     proof.stock.plan.callback!=d->bow->callback.function->symbol||
     !same_crafting_hands(proof.stock.hands,request.hands))return false;
  const auto& boundary=proof.stock.boundary;const auto& a=boundary.key;const auto& b=request.key;
  if(!boundary.stable_boundary_verified||boundary.frame!=frame||boundary.current_thread!=b.thread||
     a.generation!=b.generation||a.thread!=b.thread||a.world!=b.world||a.hero!=b.hero||a.station!=b.station||
     a.manager!=b.manager||a.production_kind!=16||b.kind!=crafting_catalog::Station::bow)return false;
  for(std::size_t i=0;i<d->targets.count;++i){
    const auto& stock=proof.stock.materials[i];const auto* expected=crafting_detail::material(request.material_snapshot,stock.symbol);
    if(stock.symbol!=d->targets[i].symbol||!expected||stock.total!=expected->stored)return false;
    const auto amount=crafting_amount_preview(*expected,request.quantity);
    if(!amount.end_known||amount.missing||amount.end_storage!=proof.expected_totals[i])return false;
  }
  return true;
}
} // namespace ubc
