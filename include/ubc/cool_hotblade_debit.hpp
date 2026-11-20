// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/cool_recipe_plan.hpp"
#include "ubc/cool_parser_prefix.hpp"

// Portable value core only for the still-open case Lager2/N2.
// No engine calls, process readers, hooks, or runtime release.
// Intake booleans are obligations of a future own cooling adapter.
namespace ubc::cool_hotblade_debit {
namespace values = alchemy_flask_debit;
namespace prefix = cool_parser_prefix;
using Context = values::Context;
using Scope = values::Scope;
using ParserState = values::ParserState;
inline constexpr bool runtime_ready = false;
inline constexpr std::uint32_t hot = 16612, cold = 16613;
inline constexpr std::uint32_t callback = 20775, owner = 20773;

struct Job {
  Context context{};
  std::uint64_t id = 0, writer_token = 0, fence_token = 0;
  std::uint32_t callback = 0, owner = 0, quantity = 0;
};
inline bool valid(const Job& j) noexcept {
  return values::valid(j.context) && j.id && j.writer_token && j.fence_token &&
    j.callback == callback && j.owner == owner && j.quantity == 2;
}
inline bool scope_matches(const Job& j, const Scope& s, std::uint32_t depth) noexcept {
  return s.writer_active && s.writer_depth == 1 && s.vm_depth == 0 && s.debit_depth == depth &&
    s.job == j.id && s.writer_token == j.writer_token && s.callback == j.callback;
}

// Do not derive a counter from callback return or unchanged hand. known
// requires its own current native evidence; serial binds exactly the
// intake. excludes_dispatch applies to the entire synchronous writer
// window, including nested engine calls, not just between two reads.
struct EndFence {
  bool known = false, excludes_dispatch = false;
  std::uint64_t token = 0, serial = 0;
  std::uint32_t destroy_started = UINT32_MAX, destroy_completed = UINT32_MAX;
};
inline bool pending(const EndFence& f, const Job& j, std::uint64_t serial) noexcept {
  return f.known && f.excludes_dispatch && f.token == j.fence_token &&
    f.serial == serial && f.destroy_started == 0 && f.destroy_completed == 0;
}

struct Admission {
  Context context{};
  std::uint64_t job = 0, writer_token = 0, fence_token = 0, serial = 0;
  bool local_script_and_amount = false, actual_info_owner = false;
  bool hot_cold_constructors = false, native_split_and_restore = false;
  bool physical_cool_hand = false, normal_end_path = false;
  bool native_parser_return_path = false;
  std::int32_t verified_left_tmp_level = -1;
};
struct Hand {
  bool known = false, all_slots_checked = false;
  std::uint32_t left = 0, right = 0, symbol = 0, amount = 0, anchor = 0;
  std::int32_t left_tmp_level = -1;
};
inline bool active_hand(const Hand& h) noexcept {
  return h.known && h.all_slots_checked && h.left && !h.right && h.symbol == hot &&
    h.amount == 1 && h.anchor == h.left && (h.left_tmp_level == 1 || h.left_tmp_level == 2);
}
inline bool same_hand(const Hand& a, const Hand& b) noexcept {
  return active_hand(a) && active_hand(b) && a.left == b.left && a.left_tmp_level == b.left_tmp_level;
}
struct HotStack {
  bool known = false, all_inventory_aliases_excluded = false, all_slot_aliases_excluded = false;
  bool native_split_path_verified = false;
  std::int64_t packed = -1;
  std::uint32_t listed_stacks = 0, object = 0, instance = 0, amount = 0;
  std::uint32_t refcount = 0, vtable = 0, mainflag = 0, flags = 0;
};
inline bool listed_stack(const HotStack& s, std::uint32_t amount) noexcept {
  return s.known && s.all_inventory_aliases_excluded && s.all_slot_aliases_excluded &&
    s.native_split_path_verified && s.packed == 0 && s.listed_stacks == 1 && s.object &&
    s.instance == hot && s.amount == amount && s.refcount && s.refcount <= INT32_MAX &&
    s.vtable == 0x83c804 && s.mainflag == 1 && s.flags == 0x200001;
}

// Own ITEM1682 binding; no call to the flask-specific reader.
// metadata_verified also binds the exact name ITEM in the current parser.
struct ItemBinding {
  bool known = false, metadata_verified = false;
  std::uint32_t parser = 0, symbol_object = 0, table = 0, code = 0, code_size = 0, symbol_count = 0;
  std::uint32_t index = 0, type = 0, flags = 0, elements = 0, value = 0;
  std::int32_t parent = -1;
};
inline bool valid_item(const ItemBinding& b, const ParserState& p) noexcept {
  return b.known && b.metadata_verified && b.parser == lhiver::game_parser && b.parser == p.current_parser &&
    b.symbol_object && b.symbol_object % 4 == 0 && b.symbol_object <= UINT32_MAX - 0x20u && b.table &&
    b.symbol_count >= lhiver::symbol_count && std::uint64_t{b.table} + std::uint64_t{b.symbol_count} * 4 <= UINT32_MAX &&
    b.code == p.code_base && b.code_size == p.code_size && b.code && b.code_size &&
    std::uint64_t{b.code} + b.code_size <= UINT32_MAX && b.index == 1682 && b.type == 7 &&
    b.flags == 1 && b.elements == 0 && b.parent == 1584;
}
inline bool same_item_target(const ItemBinding& a, const ItemBinding& b, const ParserState& p) noexcept {
  return valid_item(a, p) && valid_item(b, p) && a.symbol_object == b.symbol_object &&
    a.table == b.table && a.symbol_count == b.symbol_count;
}
inline bool same_item(const ItemBinding& a, const ItemBinding& b, const ParserState& p) noexcept {
  return same_item_target(a, b, p) && a.value == b.value;
}
inline bool same_parser_identity(const ParserState& a, const ParserState& b) noexcept {
  return a.known && b.known && a.instance_bindings_verified && b.instance_bindings_verified &&
    a.current_parser == b.current_parser && a.code_base == b.code_base && a.code_size == b.code_size &&
    a.self_symbol == b.self_symbol && a.hero_symbol == b.hero_symbol && a.self == b.self &&
    a.hero == b.hero && a.other == b.other && a.victim == b.victim;
}

struct Observation {
  Context context{};
  Scope scope{};
  std::uint64_t serial = 0;
  cool_recipe::Evidence stocks{};
  Hand hand{};
  HotStack stack{};
  ParserState parser{};
  ItemBinding item{};
  EndFence fence{};
  std::int32_t production = -1, invincible = -1, start_smithing = -1;
  bool native_contract = false, stable_read = false;
};
enum class Issue {
  none, context, scope, observation, hand, stack, parser, item, fence, sequence,
  original_exception, amount_restore, original_result, debit_exception,
  restore, debit_result, end_witness, terminal
};
enum class Stage { idle, armed, awaiting_debit, calling, debit_verified, complete };
struct Status { Stage stage = Stage::idle; Issue issue = Issue::none; bool blocked = false; };
struct Call {
  bool valid = false;
  Job job{};
  std::uint64_t serial = 0;
  std::uint32_t instance = 0, quantity = 0, stack_object = 0, generated_left = 0;
  ParserState parser{};
  ItemBinding item{};
};
inline bool valid_call(const Call& c) noexcept {
  return c.valid && valid(c.job) && c.serial && c.instance == hot && c.quantity == 1 &&
    c.stack_object && c.generated_left && c.stack_object != c.generated_left &&
    prefix::classify(c.parser, c.job.context, c.job.callback, prefix::Phase::after_original).recognized &&
    valid_item(c.item, c.parser);
}

struct RestoreObservation {
  values::RestoreObservation targets{};
  ParserState current{};
  ItemBinding item{};
  EndFence fence{};
  std::uint64_t serial = 0;
  bool native_item_set_offset_contract = false;
};
struct RestorePlan {
  bool valid = false, write_item = false;
  std::uint32_t pop_entry = 0, pop_this = 0, pop_value = 0;
  std::uint32_t instance_entry = 0, instance_symbol = 0;
  std::uint32_t item_entry = 0, item_symbol = 0, item_value = 0;
};
// Pure value plan, NO restore execution. Read/plan separately before
// every setter and then compare parser + ITEM exactly. PopPos and
// UseInstance may differ internally; active data words/actors never.
inline RestorePlan restore_plan(const Call& c, const RestoreObservation& now) noexcept {
  if (!valid_call(c) || now.serial <= c.serial || !pending(now.fence, c.job, now.serial) ||
      !now.native_item_set_offset_contract) return {};
  const values::Call neutral{true, c.job.id, c.job.writer_token, c.job.context, c.job.callback,
    c.instance, c.quantity, c.stack_object, c.parser};
  if (!values::restorable(neutral, now.targets)) return {};
  const auto& p = now.current;
  const auto& t = now.targets;
  if (!p.known || !p.instance_bindings_verified || p.current_parser != t.current_parser ||
      p.code_base != t.code_base || p.code_size != t.code_size || p.self_symbol != t.self_symbol ||
      p.hero_symbol != t.hero_symbol || p.self != t.self || p.hero != t.hero ||
      p.other != t.other || p.victim != t.victim || !values::same_data_stack(c.parser, p) ||
      !same_item_target(c.item, now.item, c.parser)) return {};
  return {true, c.item.value != now.item.value, 0x7a4b20, c.job.context.parser + 0x48u, c.parser.pop_pos,
    0x7a1550, c.parser.use_symbol, 0x7a2f20, c.item.symbol_object, c.item.value};
}

// Require two separate positive observations. Never infer EndInteraction-AFTER
// from an empty hand or Destroy from the overall balance. The
// relative order of the two end events is deliberately unspecified.
struct EndWitness {
  Context departed{};
  std::uint64_t job = 0, writer_token = 0, fence_token = 0;
  std::uint64_t destroy_serial = 0, normal_end_serial = 0;
  bool destroy_after = false, normal_end_after = false, native_paths_verified = false;
  std::uint32_t generated_anchor = 0, instance = 0, native_debit = 0, completed_return = 0;
};

// One original call and at most one positive split debit. After an error
// this instance is locked; no retry, refund, or compensating write.
class Tracker {
public:
  Tracker() noexcept = default;
  Tracker(const Tracker&) = delete;
  Tracker& operator=(const Tracker&) = delete;
  Status status() const noexcept { return status_; }
  bool quarantine(Issue issue) noexcept {
    if (!status_.blocked) { status_.blocked = true; status_.issue = issue; }
    return false;
  }
  bool arm(const Job& j, const Admission& a, const Observation& before) noexcept {
    if (status_.blocked || (status_.stage != Stage::idle && status_.stage != Stage::complete) ||
        !valid(j) || j.id <= last_job_ || j.writer_token <= last_writer_ || j.fence_token <= last_fence_ ||
        !values::same(a.context, j.context) || a.job != j.id || a.writer_token != j.writer_token ||
        a.fence_token != j.fence_token || a.serial != before.serial || !a.local_script_and_amount ||
        !a.actual_info_owner || !a.hot_cold_constructors || !a.native_split_and_restore ||
        !a.physical_cool_hand || !a.normal_end_path || !a.native_parser_return_path ||
        a.verified_left_tmp_level != before.hand.left_tmp_level ||
        !values::same(j.context, before.context) || !scope_matches(j, before.scope, 0) ||
        !base_read(before) || before.serial <= last_serial_ || before.production != 13 ||
        before.invincible != 1 || before.start_smithing != 1 || !pending(before.fence, j, before.serial) ||
        !active_hand(before.hand) || !listed_stack(before.stack, 2) || before.stack.object == before.hand.left ||
        !prefix::classify(before.parser, j.context, j.callback, prefix::Phase::before_original).recognized ||
        !valid_item(before.item, before.parser)) return false;
    const auto plan = cool_recipe::plan(cool_recipe_catalog::recipe, before.stocks, j.quantity);
    if (!plan.valid || !plan.requires_extra_debit || plan.callback != callback || plan.owner != owner ||
        plan.extra_debit != 1 || plan.native_end_debit != 1 || plan.after_callback[0].amount != 2) return false;
    job_ = j; before_ = before; plan_ = plan;
    last_job_ = j.id; last_writer_ = j.writer_token; last_fence_ = j.fence_token; last_serial_ = before.serial;
    status_ = {Stage::armed, Issue::none, false}; call_ = {}; post_parser_ = {}; post_item_ = {};
    return true;
  }
  bool original_returned(const Observation& after, bool normal, bool operands_restored) noexcept {
    if (status_.blocked) return false;
    if (status_.stage != Stage::armed) return quarantine(Issue::sequence);
    if (!normal) return quarantine(Issue::original_exception);
    if (!operands_restored) return quarantine(Issue::amount_restore);
    if (!common(after, 0, 2)) return false;
    if (!rows_equal(after.stocks, plan_.after_callback)) return quarantine(Issue::original_result);
    if (!same_parser_identity(before_.parser, after.parser) ||
        !prefix::classify(after.parser, job_.context, job_.callback, prefix::Phase::after_original).recognized)
      return quarantine(Issue::parser);
    if (!same_item_target(before_.item, after.item, after.parser)) return quarantine(Issue::item);
    post_parser_ = after.parser; post_item_ = after.item; last_serial_ = after.serial;
    status_.stage = Stage::awaiting_debit; return true;
  }
  Call begin_debit(const Observation& late) noexcept {
    if (status_.blocked) return {};
    if (status_.stage != Stage::awaiting_debit) { quarantine(Issue::sequence); return {}; }
    if (!common(late, 0, 2)) return {};
    if (!rows_equal(late.stocks, plan_.after_callback)) { quarantine(Issue::original_result); return {}; }
    if (!prefix::unchanged(post_parser_, late.parser, job_.context, job_.callback, prefix::Phase::after_original)) {
      quarantine(Issue::parser); return {};
    }
    if (!same_item(post_item_, late.item, post_parser_)) { quarantine(Issue::item); return {}; }
    call_ = {true, job_, late.serial, hot, 1, before_.stack.object, before_.hand.left, late.parser, late.item};
    last_serial_ = late.serial; status_.stage = Stage::calling; return call_;
  }
  bool debit_returned(const Observation& after, bool normal, bool restore_completed) noexcept {
    if (status_.blocked) return false;
    if (status_.stage != Stage::calling || !valid_call(call_)) return quarantine(Issue::sequence);
    if (!normal) return quarantine(Issue::debit_exception);
    if (!restore_completed) return quarantine(Issue::restore);
    if (!common(after, 1, 1)) return false;
    if (!rows_equal(after.stocks, plan_.before_end)) return quarantine(Issue::debit_result);
    if (!prefix::unchanged(post_parser_, after.parser, job_.context, job_.callback, prefix::Phase::after_original))
      return quarantine(Issue::parser);
    if (!same_item(post_item_, after.item, post_parser_)) return quarantine(Issue::item);
    last_serial_ = after.serial; status_.stage = Stage::debit_verified; return true;
  }
  bool finish(const Observation& after, const EndWitness& w) noexcept {
    if (status_.blocked) return false;
    if (status_.stage != Stage::debit_verified) return quarantine(Issue::sequence);
    if (!w.destroy_after || !w.normal_end_after || !w.native_paths_verified ||
        !values::same(w.departed, job_.context) || w.job != job_.id || w.writer_token != job_.writer_token ||
        w.fence_token != job_.fence_token || w.generated_anchor != before_.hand.left ||
        w.instance != hot || w.native_debit != 1 || w.completed_return != 0x7204ba ||
        w.destroy_serial <= last_serial_ || w.normal_end_serial <= last_serial_ ||
        w.destroy_serial == w.normal_end_serial || w.destroy_serial >= after.serial || w.normal_end_serial >= after.serial)
      return quarantine(Issue::end_witness);
    const auto& h = after.hand;
    const auto& s = after.stack;
    const auto& scope = after.scope;
    const auto& f = after.fence;
    if (!base_read(after) || after.serial <= last_serial_ || !values::same(after.context, job_.context, false) ||
        after.context.mob || after.production || after.invincible || scope.writer_active || scope.writer_depth ||
        scope.vm_depth || scope.debit_depth || scope.job || scope.writer_token || scope.callback ||
        !h.known || !h.all_slots_checked || h.left || h.right || h.symbol || h.amount || h.anchor ||
        !s.known || !s.all_inventory_aliases_excluded || !s.all_slot_aliases_excluded ||
        s.packed != 0 || s.listed_stacks || s.object || s.instance || s.amount || s.refcount || s.vtable || s.mainflag || s.flags ||
        !f.known || f.excludes_dispatch || f.token != job_.fence_token || f.serial != after.serial ||
        f.destroy_started != 1 || f.destroy_completed != 1 || !rows_equal(after.stocks, plan_.after_end))
      return quarantine(Issue::terminal);
    last_serial_ = after.serial; status_.stage = Stage::complete; return true;
  }
private:
  static bool base_read(const Observation& v) noexcept { return v.serial && v.native_contract && v.stable_read; }
  static bool rows_equal(const cool_recipe::Evidence& got, const std::array<cool_recipe::Stock, 2>& wanted) noexcept {
    const auto values = cool_recipe::inspect(got);
    return values.known && values.after_end[0].amount == wanted[0].amount && values.after_end[1].amount == wanted[1].amount;
  }
  bool common(const Observation& v, std::uint32_t depth, std::uint32_t amount) noexcept {
    if (!values::same(v.context, job_.context)) return quarantine(Issue::context);
    if (!scope_matches(job_, v.scope, depth)) return quarantine(Issue::scope);
    if (!base_read(v) || v.serial <= last_serial_) return quarantine(Issue::observation);
    if (!pending(v.fence, job_, v.serial)) return quarantine(Issue::fence);
    if (v.production || v.invincible || v.start_smithing != 1) return quarantine(Issue::original_result);
    if (!same_hand(before_.hand, v.hand)) return quarantine(Issue::hand);
    if (!listed_stack(v.stack, amount) || v.stack.object != before_.stack.object || v.stack.object == v.hand.left)
      return quarantine(Issue::stack);
    return true;
  }
  Status status_{};
  Job job_{};
  Observation before_{};
  cool_recipe::Plan plan_{};
  ParserState post_parser_{};
  ItemBinding post_item_{};
  Call call_{};
  std::uint64_t last_job_ = 0, last_writer_ = 0, last_fence_ = 0, last_serial_ = 0;
};
} // namespace ubc::cool_hotblade_debit
