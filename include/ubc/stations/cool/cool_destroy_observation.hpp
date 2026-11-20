// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_predebit_admission.hpp"
#include "ubc/stations/cool/cool_destroy_abi.hpp"
#include "ubc/engine/destroy_remove_callsite.hpp"

namespace ubc::cool_destroy_observation {
// Pure own value copies. No engine/memory read, no writer and no end
// automaton. Every Verified value must come from the native producer.
inline constexpr bool native_effect_authorized = false;
inline constexpr bool normal_end_verified = false;
enum class Issue {
  none,
  binding,
  prepared,
  canonical_plan,
  extra_debit_unsupported,
  key,
  job,
  thread,
  frame,
  serial,
  boundary,
  native_code,
  hook_ownership,
  reentry,
  exception,
  destroy_count,
  destroy_call,
  message,
  destroy_return,
  generated_hand,
  inventory_alias,
  before_stock,
  set_interact_item,
  remove_count,
  remove_call,
  remove_arguments,
  remove_return,
  inner_stock,
  outer_stock,
  hands
};
struct Binding {
  CoolRecipeKey key{};
  // job is the own fresh job identity; serial_floor the already-consumed
  // serial limit before this Destroy. The caller consumes Result.
  std::uint64_t job = 0, frame = 0, serial_floor = 0;
  std::uint32_t recipe = UINT32_MAX, quantity = 0;
};
struct Stamp {
  CoolRecipeKey key{};
  std::uint64_t job = 0, frame = 0, serial = 0;
  std::uint32_t actual_thread = 0;
  bool stable_boundary_verified = false, native_code_verified = false, hooks_owned = false;
};
struct Snapshot {
  Stamp stamp{};
  cool_recipe::Evidence stocks{};
  StationRecipeHands hands{};
};
struct Destroy {
  // npc/message/class/subtype come solely from the entry. After
  // the original, message is neither re-read nor dereferenced.
  std::uint32_t caller = 0, npc = 0, message = 0, classdef = 0, subtype = 0;
  std::uint32_t original_calls = 0, depth = 0;
  bool message_class_verified = false, normal_return = false, exception = false;
  std::int32_t return_value = 0;
};
struct Remove {
  std::uint32_t caller = 0, inventory = 0, loaded_vtable = 0, target = 0;
  std::int32_t instance = -1, amount = 0;
  std::uint32_t original_calls = 0, depth = 0, opaque_return = 0;
  bool normal_return = false, exception = false;
};
struct Observation {
  Snapshot before_destroy{}, before_remove{}, after_remove{}, after_destroy{};
  Destroy destroy{};
  Remove remove{};
  std::uint32_t destroy_entries = 0, destroy_exits = 0, remove_entries = 0, remove_exits = 0;
  bool reentry_seen = false;
  bool generated_left_verified = false, storage_alias_scan_known = false,
       generated_storage_alias = false;
  // From verified destroy code path and the actual inner CALL:
  // SetInteractItem(nullptr) returned normally. No Return1 substitute.
  // The fresh before_remove capture must also show empty hands/Anchor.
  bool set_interact_item_normal_return_verified = false;
};
struct Result {
  bool valid = false;
  Issue issue = Issue::binding;
  std::uint64_t serial = 0, frame = 0;
  std::int32_t instance = -1, amount = 0;
};

namespace detail {
inline bool same_stock(const cool_recipe::Stock& a, const cool_recipe::Stock& b) noexcept {
  return a.symbol == b.symbol && a.amount == b.amount && a.known == b.known &&
         a.unambiguous == b.unambiguous;
}
inline bool same_amount_plan(const cool_recipe::Plan& a, const cool_recipe::Plan& b) noexcept {
  if (!a.valid || !a.known || a.issue != b.issue || a.quantity != b.quantity ||
      a.maximum != b.maximum || a.direct_maximum != b.direct_maximum || a.callback != b.callback ||
      a.owner != b.owner || a.count != b.count || a.count > a.operands.size() ||
      a.extra_debit != b.extra_debit || a.native_end_debit != b.native_end_debit ||
      a.requires_extra_debit != b.requires_extra_debit ||
      a.requires_quantity_receipt != b.requires_quantity_receipt)
    return false;
  for (std::size_t i = 0; i < a.count; ++i) {
    const auto& x = a.operands[i];
    const auto& y = b.operands[i];
    if (x.offset != y.offset || x.original != y.original || x.units != y.units ||
        x.reserved != y.reserved || x.allow_zero != y.allow_zero)
      return false;
  }
  for (std::size_t i = 0; i < 2; ++i)
    if (!same_stock(a.after_callback[i], b.after_callback[i]) ||
        !same_stock(a.before_end[i], b.before_end[i]) ||
        !same_stock(a.after_end[i], b.after_end[i]))
      return false;
  return true;
}
inline bool empty_hands(const StationRecipeHands& h) noexcept {
  if (!h.left_slot_known || !h.right_slot_known || !h.interact_item_known || h.interact_item)
    return false;
  for (const auto* item : {&h.hands.left, &h.hands.right})
    if (item->state != 0 || item->object || item->symbol != -1 || item->amount != -1)
      return false;
  return true;
}
inline bool stock_matches(const cool_recipe::Evidence& actual,
                          const std::array<cool_recipe::Stock, 2>& wanted) noexcept {
  const auto copied = cool_recipe::inspect(actual);
  if (!copied.known)
    return false;
  // Resolution/ambiguity of the Hot store is mandatory. For Cold
  // only the fresh known total stock suffices; its count of physical
  // stacks may have changed through the preceding original Create.
  for (std::size_t i = 0; i < 2; ++i)
    if (copied.after_end[i].symbol != wanted[i].symbol ||
        copied.after_end[i].amount != wanted[i].amount)
      return false;
  return true;
}
inline Issue stamp(const Stamp& s, const Binding& e, std::uint64_t previous) noexcept {
  if (!cool_direct_contract::detail::valid_key(s.key) ||
      !cool_direct_contract::detail::same_key(s.key, e.key))
    return Issue::key;
  if (s.job != e.job)
    return Issue::job;
  if (s.actual_thread != e.key.thread)
    return Issue::thread;
  if (s.frame != e.frame)
    return Issue::frame;
  if (s.serial <= previous)
    return Issue::serial;
  if (!s.stable_boundary_verified)
    return Issue::boundary;
  if (!s.native_code_verified)
    return Issue::native_code;
  if (!s.hooks_owned)
    return Issue::hook_ownership;
  return Issue::none;
}
}

namespace detail {
inline bool valid_binding(const Binding& expected) noexcept {
  namespace abi = cool_destroy_abi;
  return cool_direct_contract::detail::valid_key(expected.key) && expected.job && expected.frame &&
         expected.recipe == static_cast<std::uint32_t>(cool_recipe_catalog::RecipeId::blade) &&
         expected.quantity && expected.quantity <= INT32_MAX && expected.key.hero % 4u == 0 &&
         expected.key.hero <= UINT32_MAX - abi::npc_inventory_offset;
}
inline Result verify_destroy(const StationRecipeHands& hands,
                             const cool_predebit::Stocks& before_end,
                             const cool_predebit::Stocks& after_end, const Binding& expected,
                             const Observation& observed) noexcept {
  namespace direct = cool_direct_contract;
  namespace abi = cool_destroy_abi;
  namespace call = destroy_remove_callsite;
  Result out;
  const auto fail = [&out](Issue issue) noexcept {
    out.issue = issue;
    return out;
  };
  const auto& destroy = observed.destroy;
  const auto& remove = observed.remove;
  if (destroy.exception || remove.exception)
    return fail(Issue::exception);
  if (observed.destroy_entries != 1 || observed.destroy_exits != 1 || destroy.original_calls != 1)
    return fail(Issue::destroy_count);
  if (observed.remove_entries != 1 || observed.remove_exits != 1 || remove.original_calls != 1)
    return fail(Issue::remove_count);
  if (observed.reentry_seen || destroy.depth != 1 || remove.depth != 1)
    return fail(Issue::reentry);
  const Snapshot* points[]{&observed.before_destroy, &observed.before_remove,
                           &observed.after_remove, &observed.after_destroy};
  auto previous = expected.serial_floor;
  for (const auto* point : points) {
    const auto issue = detail::stamp(point->stamp, expected, previous);
    if (issue != Issue::none)
      return fail(issue);
    previous = point->stamp.serial;
  }
  if (destroy.caller != abi::destroy_return || destroy.npc != expected.key.hero)
    return fail(Issue::destroy_call);
  if (!destroy.message || destroy.message % 4u ||
      destroy.message > UINT32_MAX - abi::message_subtype_offset - 4u ||
      !destroy.message_class_verified || destroy.classdef != abi::manipulate_classdef ||
      destroy.subtype != abi::destroy_subtype)
    return fail(Issue::message);
  if (!destroy.normal_return || destroy.return_value != 1)
    return fail(Issue::destroy_return);
  if (!observed.generated_left_verified ||
      !direct::detail::same_hand(hands, observed.before_destroy.hands) ||
      observed.before_destroy.hands.hands.left.object % 4u)
    return fail(Issue::generated_hand);
  if (!observed.storage_alias_scan_known || observed.generated_storage_alias)
    return fail(Issue::inventory_alias);
  if (!detail::stock_matches(observed.before_destroy.stocks, before_end) ||
      !detail::stock_matches(observed.before_remove.stocks, before_end))
    return fail(Issue::before_stock);
  if (!observed.set_interact_item_normal_return_verified ||
      !detail::empty_hands(observed.before_remove.hands))
    return fail(Issue::set_interact_item);
  if (remove.caller != abi::inventory_remove_return ||
      remove.inventory != expected.key.hero + abi::npc_inventory_offset ||
      remove.loaded_vtable != call::vtable || remove.target != call::target)
    return fail(Issue::remove_call);
  if (remove.instance != cool_recipe_catalog::recipe.hot_symbol || remove.amount != 1)
    return fail(Issue::remove_arguments);
  // Only an opaque return value, no further object/pointer read.
  if (!remove.normal_return || !remove.opaque_return)
    return fail(Issue::remove_return);
  if (!detail::stock_matches(observed.after_remove.stocks, after_end))
    return fail(Issue::inner_stock);
  if (!detail::stock_matches(observed.after_destroy.stocks, after_end))
    return fail(Issue::outer_stock);
  if (!detail::empty_hands(observed.after_remove.hands) ||
      !detail::empty_hands(observed.after_destroy.hands))
    return fail(Issue::hands);
  out.valid = true;
  out.issue = Issue::none;
  out.serial = observed.after_destroy.stamp.serial;
  out.frame = expected.frame;
  out.instance = remove.instance;
  out.amount = remove.amount;
  return out;
}
} // namespace detail

inline Result verify(const cool_direct_contract::Prepared& prepared, const Binding& expected,
                     const Observation& observed) noexcept {
  namespace direct = cool_direct_contract;
  Result out;
  const auto fail = [&out](Issue issue) noexcept {
    out.issue = issue;
    return out;
  };
  if (!detail::valid_binding(expected))
    return out;
  if (!direct::detail::same_key(prepared.key, expected.key))
    return fail(Issue::key);
  // No trust in copied before_end/after_end or a valid-Bool.
  const auto canonical =
      cool_recipe::plan(cool_recipe_catalog::recipe, prepared.stocks, expected.quantity);
  if (!canonical.valid || !canonical.known || canonical.issue != cool_recipe::Issue::none)
    return fail(Issue::canonical_plan);
  if (canonical.extra_debit || canonical.requires_extra_debit)
    return fail(Issue::extra_debit_unsupported);
  if (!prepared.valid || !prepared.stock_known || prepared.issue != direct::Issue::none)
    return fail(Issue::prepared);
  if (!prepared.path.valid || prepared.path.quantity != expected.quantity ||
      prepared.path.count != 1 ||
      prepared.path.step.invocation != recipe_catalog::Invocation::info ||
      prepared.path.step.owner != canonical.owner ||
      prepared.path.step.function != canonical.callback || prepared.maximum != canonical.maximum ||
      prepared.direct_maximum != canonical.direct_maximum || canonical.native_end_debit != 1 ||
      !detail::same_amount_plan(prepared.path.amount, canonical))
    return fail(Issue::canonical_plan);
  return detail::verify_destroy(prepared.hands, canonical.before_end, canonical.after_end, expected,
                                observed);
}
inline Result verify(const cool_predebit::admission::Prepared& prepared,
                     const cool_predebit::Witness& witness, const Binding& expected,
                     const Observation& observed) noexcept {
  if (!detail::valid_binding(expected) || !prepared.matches_witness(witness) ||
      expected.quantity != cool_predebit::quantity ||
      !cool_direct_contract::detail::same_key(prepared.key(), expected.key) ||
      expected.job != prepared.job().id || expected.frame < witness.frame ||
      expected.serial_floor < witness.original_serial)
    return {};
  return detail::verify_destroy(prepared.hands(), prepared.amount().after_callback,
                                prepared.amount().after_end, expected, observed);
}
} // namespace ubc::cool_destroy_observation
