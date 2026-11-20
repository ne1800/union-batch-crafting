// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/grind/grind_item_binding.hpp"
#include "ubc/stations/grind/grind_check.hpp"
#include "ubc/stations/grind/grind_recipe_navigation.hpp"
#include "ubc/stations/cool/cool_destroy_observation.hpp"

namespace ubc::grind_destroy_observation {
// Shared pure original event values, no cool-key-/material conversion.
using Destroy = cool_destroy_observation::Destroy;
using Remove = cool_destroy_observation::Remove;
using Issue = cool_destroy_observation::Issue;
using Result = cool_destroy_observation::Result;
inline constexpr bool native_effect_authorized = false;
inline constexpr bool normal_end_verified = false;

// Copy of the actually bound fresh grind plan before the callback. No
// Prepared bool replaces writer lease, SELF, loaded item path, or End.
struct Prepared {
  GrindRecipeKey key{};
  std::uint64_t job = 0;
  grind_recipe::Evidence before{};
  grind_recipe::Plan amount{};
  StationRecipeHands hands{};
};
struct Binding {
  // key is the current Destroy-Key, including its real frame.
  GrindRecipeKey key{};
  std::uint64_t job = 0, frame = 0, serial_floor = 0;
  std::uint32_t callback = 0, quantity = 0;
};
struct Stamp {
  GrindRecipeKey key{};
  std::uint64_t job = 0, frame = 0, serial = 0;
  std::uint32_t actual_thread = 0;
  bool stable_boundary_verified = false, native_code_verified = false, hooks_owned = false;
};
struct Stocks {
  // Real inventory totals, excluding generated hand blade. No guild/skill
  // results: these remain in the existing shared result contract.
  bool stable_boundary_verified = false;
  std::array<grind_recipe::Stock, 3> stocks{};
};
struct Snapshot {
  Stamp stamp{};
  Stocks stocks{};
  StationRecipeHands hands{};
};
struct Observation {
  Snapshot before_destroy{}, before_remove{}, after_remove{}, after_destroy{};
  Destroy destroy{};
  Remove remove{};
  std::uint32_t destroy_entries = 0, destroy_exits = 0, remove_entries = 0, remove_exits = 0;
  bool reentry_seen = false;
  bool generated_left_verified = false, storage_alias_scan_known = false, generated_storage_alias = false;
  // Only from a normal SetInteractItem(null) in the verified Destroy
  // path and the actual inner CALL. The fresh capture shows empty hands.
  bool set_interact_item_normal_return_verified = false;
};

namespace detail {
inline bool same_identity(const GrindRecipeKey& a, const GrindRecipeKey& b) noexcept {
  // An earlier Prepared-Frame is not rewritten onto the later frame.
  // All remaining components stay exactly bound to the same job context.
  return a.generation == b.generation && a.thread == b.thread && a.world == b.world &&
    a.hero == b.hero && a.station == b.station && a.manager == b.manager &&
    a.production_kind == b.production_kind;
}
inline bool active_grind_hand(const StationRecipeHands& h) noexcept {
  // Explicitly use the existing grind_recipe_material hand contract: generated cold
  // blade 16613, quantity 1 on the left, tmp1/2, right hand empty and the same anchor.
  const auto& left = h.hands.left;
  const auto& right = h.hands.right;
  return h.left_slot_known && h.right_slot_known && h.interact_item_known &&
    left.state == 1 && left.object && left.object % 4u == 0 &&
    left.symbol == grind_recipe::lifecycle_item && left.amount == 1 &&
    right.state == 0 && !right.object && h.interact_item == left.object &&
    (h.left_tmp_level == 1 || h.left_tmp_level == 2);
}
inline bool stock_matches(const Stocks& actual, const std::array<grind_recipe::Stock, 3>& wanted,
    GrindCheck& check, const char* point) noexcept {
  check.point = point;
  if (!check.equal("stock.stable",true,actual.stable_boundary_verified)) return false;
  for (const auto& target : wanted) {
    check.symbol = target.symbol;
    const grind_recipe::Stock* found = nullptr;
    for (const auto& row : actual.stocks) if (row.symbol == target.symbol) {
      if (!check.equal("stock.duplicate",false,found!=nullptr)) return false;
      found = &row;
    }
    if (!check.equal("stock.present",true,found!=nullptr) || !check.equal("stock.known",true,found->known) ||
        !check.equal("stock.amount",target.amount,found->amount) ||
        !check.equal("stock.in_range",true,found->amount>=0 && found->amount<=INT32_MAX)) return false;
  }
  check.point = "";
  check.symbol = 0;
  return true;
}
inline Issue stamp(const Stamp& point, const Binding& expected, std::uint64_t previous, GrindCheck& check) noexcept {
  if (!check.equal("stamp.valid_key",true,grind_recipe_snapshot_detail::valid_key(point.key)) ||
      !check.equal("stamp.same_key",true,grind_recipe_snapshot_detail::same_key(point.key,expected.key))) return Issue::key;
  if (!check.equal_unsigned("stamp.job",expected.job,point.job)) return Issue::job;
  if (!check.equal("stamp.thread",expected.key.thread,point.actual_thread)) return Issue::thread;
  if (!check.equal_unsigned("stamp.frame",expected.frame,point.frame) ||
      !check.equal_unsigned("stamp.key_frame",point.frame,point.key.frame)) return Issue::frame;
  if (point.serial <= previous) {
    if (previous == UINT64_MAX) check.equal("stamp.serial_exhausted",false,true);
    else check.equal_unsigned("stamp.minimum_serial",previous+1,point.serial);
    return Issue::serial;
  }
  if (!check.equal("stamp.stable",true,point.stable_boundary_verified)) return Issue::boundary;
  if (!check.equal("stamp.native",true,point.native_code_verified)) return Issue::native_code;
  if (!check.equal("stamp.hooks",true,point.hooks_owned)) return Issue::hook_ownership;
  return Issue::none;
}
}

inline Result verify(const Prepared& prepared, const Binding& expected,
    const Observation& observed, GrindCheck& check) noexcept {
  namespace g = grind_recipe;
  namespace abi = cool_destroy_abi;
  namespace call = destroy_remove_callsite;
  Result out;
  const auto fail = [&](Issue issue) noexcept {
    check.equal("destroy.issue", static_cast<int>(Issue::none), static_cast<int>(issue));
    out.issue = issue;
    return out;
  };
  const auto* recipe = g::find(expected.callback);
  if (!recipe) {
    check.equal("destroy.known_callback",true,false);
    return fail(Issue::binding);
  }
  if (!check.equal("destroy.valid_key",true,grind_recipe_snapshot_detail::valid_key(expected.key)) ||
      !check.equal("destroy.job_present",true,expected.job!=0) || !check.equal("destroy.frame_present",true,expected.frame!=0) ||
      !check.equal_unsigned("destroy.key_frame",expected.frame,expected.key.frame) ||
      !check.equal("destroy.quantity_in_range",true,expected.quantity>0 && expected.quantity<=INT32_MAX) ||
      !check.equal("destroy.hero_alignment",0,expected.key.hero%4u) ||
      !check.equal("destroy.inventory_address_in_range",true,expected.key.hero<=UINT32_MAX-abi::npc_inventory_offset))
    return fail(Issue::binding);
  if (!check.equal("destroy.prepared_key",true,grind_recipe_snapshot_detail::valid_key(prepared.key)) ||
      !check.equal("destroy.identity",true,detail::same_identity(prepared.key,expected.key))) return fail(Issue::key);
  if (!check.equal_unsigned("destroy.job",expected.job,prepared.job)) return fail(Issue::job);
  if (prepared.key.frame > expected.frame) {
    check.equal_unsigned("destroy.maximum_prepared_frame",expected.frame,prepared.key.frame);
    return fail(Issue::frame);
  }
  // Repeat the real original invoice; copied deltas/operands or a valid-Bool
  // are no substitute. The constant Descriptor pointer remains its own.
  const auto canonical = g::plan(*recipe, prepared.before, expected.quantity);
  if (!check.equal("destroy.plan_valid",true,canonical.valid) || !check.equal("destroy.plan_known",true,canonical.known) ||
      !check.equal("destroy.plan_issue",static_cast<int>(g::Issue::none),static_cast<int>(canonical.issue)) ||
      !check.equal("destroy.same_plan",true,grind_recipe_navigation::same_amount(prepared.amount,canonical)))
    return fail(Issue::canonical_plan);

  const auto& destroy = observed.destroy;
  const auto& remove = observed.remove;
  if (!check.equal("destroy.exception",false,destroy.exception) || !check.equal("remove.exception",false,remove.exception)) return fail(Issue::exception);
  if (!check.equal("destroy.entries",1,observed.destroy_entries) || !check.equal("destroy.exits",1,observed.destroy_exits) ||
      !check.equal("destroy.original_calls",1,destroy.original_calls))
    return fail(Issue::destroy_count);
  if (!check.equal("remove.entries",1,observed.remove_entries) || !check.equal("remove.exits",1,observed.remove_exits) ||
      !check.equal("remove.original_calls",1,remove.original_calls))
    return fail(Issue::remove_count);
  if (!check.equal("destroy.reentry",false,observed.reentry_seen) || !check.equal("destroy.depth",1,destroy.depth) || !check.equal("remove.depth",1,remove.depth)) return fail(Issue::reentry);
  auto previous = expected.serial_floor;
  constexpr const char* phases[]={"before_destroy","before_remove","after_remove","after_destroy"};
  unsigned phase = 0;
  for (const auto* point : {&observed.before_destroy, &observed.before_remove,
                            &observed.after_remove, &observed.after_destroy}) {
    const auto issue = detail::stamp(point->stamp, expected, previous, check);
    if (issue != Issue::none) {
      check.point = phases[phase];
      return fail(issue);
    }
    ++phase;
    previous = point->stamp.serial;
  }
  if (!check.equal("destroy.caller",abi::destroy_return,destroy.caller) || !check.equal("destroy.hero",expected.key.hero,destroy.npc))
    return fail(Issue::destroy_call);
  if (!check.equal("destroy.message_present",true,destroy.message!=0) ||
      !check.equal("destroy.message_alignment",0,destroy.message%4u) ||
      !check.equal("destroy.message_range",true,destroy.message<=UINT32_MAX-abi::message_subtype_offset-4u) ||
      !check.equal("destroy.message_class",true,destroy.message_class_verified) || !check.equal("destroy.classdef",abi::manipulate_classdef,destroy.classdef) ||
      !check.equal("destroy.subtype",abi::destroy_subtype,destroy.subtype)) return fail(Issue::message);
  if (!check.equal("destroy.normal_return",true,destroy.normal_return) || !check.equal("destroy.return_value",1,destroy.return_value)) return fail(Issue::destroy_return);
  if (!check.equal("destroy.generated_left",true,observed.generated_left_verified) || !check.equal("destroy.prepared_hand",true,detail::active_grind_hand(prepared.hands)) ||
      !check.equal("destroy.active_hand",true,detail::active_grind_hand(observed.before_destroy.hands)) ||
      !check.equal("destroy.same_hands",true,grind_item_binding_detail::same_hands(prepared.hands,observed.before_destroy.hands)))
    return fail(Issue::generated_hand);
  if (!check.equal("destroy.alias_known",true,observed.storage_alias_scan_known) || !check.equal("destroy.storage_alias",false,observed.generated_storage_alias))
    return fail(Issue::inventory_alias);
  // Weapons already -N/+N; ColdBlade here original +1, independent of N.
  if (!detail::stock_matches(observed.before_destroy.stocks, canonical.after_callback, check, "before_destroy") ||
      !detail::stock_matches(observed.before_remove.stocks, canonical.after_callback, check, "before_remove")) return fail(Issue::before_stock);
  if (!check.equal("remove.set_interact_return",true,observed.set_interact_item_normal_return_verified) ||
      !check.equal("before_remove.empty_hands",true,cool_destroy_observation::detail::empty_hands(observed.before_remove.hands))) return fail(Issue::set_interact_item);
  if (!check.equal("remove.caller",abi::inventory_remove_return,remove.caller) || !check.equal("remove.inventory",expected.key.hero + abi::npc_inventory_offset,remove.inventory) ||
      !check.equal("remove.vtable",call::vtable,remove.loaded_vtable) || !check.equal("remove.target",call::target,remove.target)) return fail(Issue::remove_call);
  if (!check.equal("remove.instance",g::lifecycle_item,remove.instance) || !check.equal("remove.amount",1,remove.amount)) return fail(Issue::remove_arguments);
  if (!check.equal("remove.normal_return",true,remove.normal_return) || !check.equal("remove.nonzero_return",true,remove.opaque_return != 0)) return fail(Issue::remove_return);
  // The original Remove changes only 16613 by -1; weapons stay identical.
  if (!detail::stock_matches(observed.after_remove.stocks, canonical.after_end, check, "after_remove")) return fail(Issue::inner_stock);
  if (!detail::stock_matches(observed.after_destroy.stocks, canonical.after_end, check, "after_destroy")) return fail(Issue::outer_stock);
  if (!check.equal("after_remove.empty_hands",true,cool_destroy_observation::detail::empty_hands(observed.after_remove.hands)) ||
      !check.equal("after_destroy.empty_hands",true,cool_destroy_observation::detail::empty_hands(observed.after_destroy.hands))) return fail(Issue::hands);
  out.valid = true;
  out.issue = Issue::none;
  out.serial = observed.after_destroy.stamp.serial;
  out.frame = expected.frame;
  out.instance = remove.instance;
  out.amount = remove.amount;
  return out;
}
inline Result verify(const Prepared& prepared, const Binding& expected,
    const Observation& observed) noexcept {
  GrindCheck check;
  return verify(prepared, expected, observed, check);
}
} // namespace ubc::grind_destroy_observation
