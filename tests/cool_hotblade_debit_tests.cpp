// SPDX-License-Identifier: MIT
#include "ubc/cool_hotblade_debit.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <type_traits>

namespace d = ubc::cool_hotblade_debit;
namespace v = ubc::alchemy_flask_debit;
namespace p = ubc::cool_parser_prefix;
int checks = 0, failures = 0;
#define CHECK(value) do { ++checks; if (!(value)) { ++failures; std::fprintf(stderr, "line %d: %s\n", __LINE__, #value); } } while (false)

struct Fixture {
  d::Job job{{1, 8, ubc::lhiver::game_parser, 0x100000, 0x200000, 0x300000, 0x400000}, 10, 20, 30, d::callback, d::owner, 2};
  d::Admission admission{};
  d::Observation before{}, original{}, late{}, debit{}, end{};
  d::EndWitness witness{};
  explicit Fixture(std::int64_t initial_cold = 5, std::uint32_t print = 1, std::int32_t tmp = 1) {
    admission = {job.context, job.id, job.writer_token, job.fence_token, 1, true, true, true, true, true, true, true, tmp};
    before.context = job.context;
    before.scope = {job.id, job.writer_token, job.callback, 1, 0, 0, true};
    before.serial = 1;
    before.stocks = {true, {{{16612, 2, true, true}, {16613, initial_cold, true, false}}}};
    before.hand = {true, true, 0x600000, 0, d::hot, 1, 0x600000, tmp};
    before.stack = {true, true, true, true, 0, 1, 0x500000, d::hot, 2, 1, 0x83c804, 1, 0x200001};
    auto& parser = before.parser;
    parser.known = parser.instance_bindings_verified = true;
    parser.current_parser = job.context.parser;
    parser.code_base = 0x10000000; parser.code_size = ubc::lhiver::code_length;
    parser.pop_pos = 0x1234;
    parser.self_symbol = 0x700000; parser.hero_symbol = 0x700100;
    parser.self = parser.hero = parser.use_address = job.context.hero;
    parser.use_symbol = parser.self_symbol;
    parser.other = 0x800000; parser.victim = 0;
    before.item = {true, true, job.context.parser, 0x700200, 0x900000, parser.code_base,
      parser.code_size, ubc::lhiver::symbol_count, 1682, 7, 1, 0, 0x999000, 1584};
    before.fence = {true, true, job.fence_token, 1, 0, 0};
    before.production = 13; before.invincible = before.start_smithing = 1;
    before.native_contract = before.stable_read = true;
    original = before; original.serial = original.fence.serial = 2;
    original.production = original.invincible = 0;
    original.stocks.stocks[1].amount += 2;
    original.parser.data_depth = 2; original.parser.data_words_known = true;
    original.parser.data_words[0] = print; original.parser.data_words[1] = 0x40;
    original.parser.pop_pos = 0x2345;
    original.item.value = 0x888000; // The original output may have changed ITEM.
    late = original; late.serial = late.fence.serial = 3;
    debit = late; debit.serial = debit.fence.serial = 5; debit.scope.debit_depth = 1;
    debit.stack.amount = 1; debit.stocks.stocks[0].amount = 1;
    end = debit; end.serial = end.fence.serial = 8;
    end.context.mob = 0; end.scope = {}; end.hand = {true, true};
    end.stack = {true, true, true}; end.stack.packed = 0;
    end.stocks.stocks[0].amount = 0;
    end.fence.excludes_dispatch = false; end.fence.destroy_started = end.fence.destroy_completed = 1;
    witness = {job.context, job.id, job.writer_token, job.fence_token, 6, 7,
      true, true, true, before.hand.left, d::hot, 1, 0x7204ba};
  }
};

d::Call through_debit_call(d::Tracker& tracker, const Fixture& f) {
  CHECK(tracker.arm(f.job, f.admission, f.before));
  CHECK(tracker.status().stage == d::Stage::armed);
  CHECK(tracker.original_returned(f.original, true, true));
  CHECK(tracker.status().stage == d::Stage::awaiting_debit);
  const auto call = tracker.begin_debit(f.late);
  CHECK(d::valid_call(call));
  CHECK(tracker.status().stage == d::Stage::calling);
  return call;
}
void through_verified(d::Tracker& tracker, const Fixture& f) {
  (void)through_debit_call(tracker, f);
  CHECK(tracker.debit_returned(f.debit, true, true));
  CHECK(tracker.status().stage == d::Stage::debit_verified);
}
d::RestoreObservation restore_observation(const d::Call& call) {
  d::RestoreObservation r;
  const auto& s = call.parser;
  r.serial = call.serial + 1;
  r.fence = {true, true, call.job.fence_token, r.serial, 0, 0};
  r.native_item_set_offset_contract = true;
  r.targets = {call.job.context, {call.job.id, call.job.writer_token, call.job.callback, 1, 0, 1, true},
    true, true, s.current_parser, s.code_base, s.code_size, s.use_symbol, s.use_address,
    s.self_symbol, s.hero_symbol, s.self, s.hero, s.other, s.victim};
  r.current = s; r.current.pop_pos = 10; r.current.use_symbol = call.item.symbol_object;
  r.current.use_address = 0x123400; // The temporary hot-blade constructor context.
  r.item = call.item; r.item.value = 0x123400;
  return r;
}

void happy_and_limits() {
  static_assert(!d::runtime_ready);
  static_assert(!std::is_copy_constructible_v<d::Tracker>);
  static_assert(!std::is_copy_assignable_v<d::Tracker>);
  for (const auto cold : {INT64_C(0), INT64_C(1), INT64_C(24), INT64_C(2147483644), INT64_C(2147483645)}) {
    for (std::uint32_t print = 0; print <= 1; ++print) for (const auto tmp : {1, 2}) {
      Fixture f(cold, print, tmp);
      d::Tracker tracker;
      const auto call = through_debit_call(tracker, f);
      CHECK(call.instance == d::hot && call.quantity == 1 && call.stack_object == f.before.stack.object);
      CHECK(call.generated_left != call.stack_object);
      const auto restore = d::restore_plan(call, restore_observation(call));
      CHECK(restore.valid && restore.write_item);
      CHECK(restore.pop_entry == 0x7a4b20 && restore.pop_this == ubc::lhiver::game_parser + 0x48u);
      CHECK(restore.pop_value == call.parser.pop_pos && restore.instance_entry == 0x7a1550);
      CHECK(restore.instance_symbol == call.parser.use_symbol && restore.item_entry == 0x7a2f20);
      CHECK(restore.item_symbol == call.item.symbol_object && restore.item_value == call.item.value);
      CHECK(tracker.debit_returned(f.debit, true, true));
      CHECK(tracker.status().stage != d::Stage::complete);
      CHECK(tracker.finish(f.end, f.witness));
      CHECK(tracker.status().stage == d::Stage::complete && !tracker.status().blocked);
    }
  }
  for (const auto cold : {INT64_C(-1), INT64_C(2147483646), INT64_C(2147483647), INT64_C(2147483648)}) {
    Fixture f(cold); d::Tracker t; CHECK(!t.arm(f.job, f.admission, f.before));
  }
  // The two stock rows are bound by symbol and may appear in either order.
  Fixture reversed;
  for (auto* o : {&reversed.before, &reversed.original, &reversed.late, &reversed.debit, &reversed.end})
    std::swap(o->stocks.stocks[0], o->stocks.stocks[1]);
  d::Tracker t; through_verified(t, reversed); CHECK(t.finish(reversed.end, reversed.witness));
  // Do not impose an unverified relative order on Destroy and EndInteraction.
  Fixture opposite; std::swap(opposite.witness.destroy_serial, opposite.witness.normal_end_serial);
  d::Tracker t2; through_verified(t2, opposite); CHECK(t2.finish(opposite.end, opposite.witness));
}

template<class Mutation> void reject_arm(Mutation mutate) {
  Fixture f; mutate(f); d::Tracker t;
  CHECK(!t.arm(f.job, f.admission, f.before));
  CHECK(t.status().stage == d::Stage::idle);
  CHECK(!t.begin_debit(f.late).valid);
}
void entry_guards() {
  for (const auto quantity : {0u, 1u, 3u, 7u, std::uint32_t{INT32_MAX}, UINT32_MAX})
    reject_arm([=](Fixture& f) { f.job.quantity = quantity; });
  for (const auto amount : {INT64_C(0), INT64_C(1), INT64_C(3), INT64_C(2147483647)})
    reject_arm([=](Fixture& f) { f.before.stocks.stocks[0].amount = amount; });
  for (const auto cb : {0u, 20772u, 20778u, 20370u, 20289u})
    reject_arm([=](Fixture& f) { f.job.callback = cb; });
  for (const auto field : {&d::Admission::local_script_and_amount, &d::Admission::actual_info_owner,
       &d::Admission::hot_cold_constructors, &d::Admission::native_split_and_restore,
       &d::Admission::physical_cool_hand, &d::Admission::normal_end_path, &d::Admission::native_parser_return_path})
    reject_arm([=](Fixture& f) { f.admission.*field = false; });
  reject_arm([](Fixture& f) { ++f.admission.context.epoch; });
  reject_arm([](Fixture& f) { ++f.admission.job; });
  reject_arm([](Fixture& f) { ++f.admission.serial; });
  reject_arm([](Fixture& f) { ++f.admission.writer_token; });
  reject_arm([](Fixture& f) { ++f.admission.fence_token; });
  reject_arm([](Fixture& f) { f.admission.verified_left_tmp_level = -1; });
  reject_arm([](Fixture& f) { f.admission.verified_left_tmp_level = 2; });
  reject_arm([](Fixture& f) { f.job.id = 0; });
  reject_arm([](Fixture& f) { f.job.writer_token = 0; });
  reject_arm([](Fixture& f) { f.job.fence_token = 0; });
  reject_arm([](Fixture& f) { f.job.owner = 20776; });
  reject_arm([](Fixture& f) { f.before.production = 4; });
  reject_arm([](Fixture& f) { f.before.invincible = 0; });
  reject_arm([](Fixture& f) { f.before.start_smithing = 0; });
  reject_arm([](Fixture& f) { f.before.parser.data_depth = 2; });
  reject_arm([](Fixture& f) { f.before.stack.packed = 2; });
  reject_arm([](Fixture& f) { f.before.stack.object = f.before.hand.left; });
  reject_arm([](Fixture& f) { f.before.stack.refcount = 0; });
  reject_arm([](Fixture& f) { f.before.stack.refcount = UINT32_MAX; });
  reject_arm([](Fixture& f) { f.before.stack.mainflag = 2; });
  reject_arm([](Fixture& f) { f.before.stack.flags |= 0x40000000; });
  reject_arm([](Fixture& f) { f.before.stack.flags ^= 0x1; });
  reject_arm([](Fixture& f) { f.before.stack.vtable = 0; });
  reject_arm([](Fixture& f) { f.before.stack.instance = 16604; });
  reject_arm([](Fixture& f) { f.before.stack.listed_stacks = 2; });
  reject_arm([](Fixture& f) { f.before.hand.left_tmp_level = 0; });
  reject_arm([](Fixture& f) { f.before.hand.symbol = 16604; });
  reject_arm([](Fixture& f) { f.before.hand.anchor = f.before.stack.object; });
  reject_arm([](Fixture& f) { f.before.hand.right = 0x333; });
  reject_arm([](Fixture& f) { f.before.fence.known = false; });
  reject_arm([](Fixture& f) { f.before.fence.excludes_dispatch = false; });
  reject_arm([](Fixture& f) { f.before.fence.destroy_started = 1; });
  reject_arm([](Fixture& f) { f.before.fence.destroy_completed = 1; });
  reject_arm([](Fixture& f) { f.before.stocks.stable_boundary_verified = false; });
  reject_arm([](Fixture& f) { f.before.stocks.stocks[0].unambiguous = false; });
  reject_arm([](Fixture& f) { f.before.stocks.stocks[1].symbol = 16612; });
}

template<class Mutation> void reject_after(Mutation mutate, int phase) {
  Fixture f; d::Tracker t;
  CHECK(t.arm(f.job, f.admission, f.before));
  bool result = false;
  if (phase == 0) { mutate(f.original); result = t.original_returned(f.original, true, true); }
  if (phase >= 1) {
    CHECK(t.original_returned(f.original, true, true));
    if (phase == 1) { mutate(f.late); result = t.begin_debit(f.late).valid; }
    if (phase == 2) {
      CHECK(t.begin_debit(f.late).valid); mutate(f.debit); result = t.debit_returned(f.debit, true, true);
    }
  }
  CHECK(!result); CHECK(t.status().blocked);
  CHECK(!t.arm(f.job, f.admission, f.before));
  CHECK(!t.begin_debit(f.late).valid);
  CHECK(!t.debit_returned(f.debit, true, true));
  CHECK(!t.finish(f.end, f.witness));
}
void phase_guards() {
  for (int phase = 0; phase < 3; ++phase) {
    reject_after([](d::Observation& o) { ++o.context.thread; }, phase);
    reject_after([](d::Observation& o) { ++o.context.mob; }, phase);
    reject_after([](d::Observation& o) { ++o.context.epoch; }, phase);
    reject_after([](d::Observation& o) { ++o.scope.writer_token; }, phase);
    reject_after([](d::Observation& o) { ++o.scope.job; }, phase);
    reject_after([](d::Observation& o) { o.scope.vm_depth = 1; }, phase);
    reject_after([](d::Observation& o) { o.scope.writer_depth = 2; }, phase);
    reject_after([](d::Observation& o) { o.scope.writer_active = false; }, phase);
    reject_after([](d::Observation& o) { o.native_contract = false; }, phase);
    reject_after([](d::Observation& o) { o.stable_read = false; }, phase);
    reject_after([](d::Observation& o) { o.serial = 1; }, phase);
    reject_after([](d::Observation& o) { o.fence.serial = 1; }, phase);
    reject_after([](d::Observation& o) { ++o.fence.token; }, phase);
    reject_after([](d::Observation& o) { o.fence.known = false; }, phase);
    reject_after([](d::Observation& o) { o.fence.excludes_dispatch = false; }, phase);
    reject_after([](d::Observation& o) { o.fence.destroy_started = 1; }, phase);
    reject_after([](d::Observation& o) { o.fence.destroy_completed = 1; }, phase);
    reject_after([](d::Observation& o) { o.production = 13; }, phase);
    reject_after([](d::Observation& o) { o.invincible = 1; }, phase);
    reject_after([](d::Observation& o) { o.start_smithing = 0; }, phase);
    reject_after([](d::Observation& o) { ++o.hand.left; ++o.hand.anchor; }, phase);
    reject_after([](d::Observation& o) { o.hand.left_tmp_level = 2; }, phase);
    reject_after([](d::Observation& o) { o.hand.right = 1; }, phase);
    reject_after([](d::Observation& o) { o.hand.all_slots_checked = false; }, phase);
    reject_after([](d::Observation& o) { ++o.stack.object; }, phase);
    reject_after([](d::Observation& o) { o.stack.native_split_path_verified = false; }, phase);
    reject_after([](d::Observation& o) { o.stack.all_inventory_aliases_excluded = false; }, phase);
    reject_after([](d::Observation& o) { o.stack.all_slot_aliases_excluded = false; }, phase);
    reject_after([](d::Observation& o) { ++o.stack.amount; }, phase);
    reject_after([](d::Observation& o) { --o.stocks.stocks[0].amount; }, phase);
    reject_after([](d::Observation& o) { --o.stocks.stocks[1].amount; }, phase);
    reject_after([](d::Observation& o) { o.stocks.stocks[0].known = false; }, phase);
    reject_after([](d::Observation& o) { o.parser.data_words_known = false; }, phase);
    reject_after([](d::Observation& o) { o.parser.self = 0; }, phase);
    reject_after([](d::Observation& o) { o.item.metadata_verified = false; }, phase);
    reject_after([](d::Observation& o) { ++o.item.index; }, phase);
    reject_after([](d::Observation& o) { ++o.item.table; }, phase);
    reject_after([](d::Observation& o) { ++o.item.code_size; }, phase);
    reject_after([](d::Observation& o) { ++o.item.symbol_object; }, phase);
  }
  // Recognizing both print values does not establish an unchanged prefix.
  // The original must not silently replace known actor target pointers.
  reject_after([](d::Observation& o) { o.parser.self_symbol += 4; o.parser.use_symbol = o.parser.self_symbol; }, 0);
  reject_after([](d::Observation& o) { o.parser.hero_symbol += 4; }, 0);
  reject_after([](d::Observation& o) { ++o.parser.other; }, 0);
  reject_after([](d::Observation& o) { ++o.parser.victim; }, 0);
  for (const auto phase : {1, 2}) {
    reject_after([](d::Observation& o) { o.parser.data_words[0] ^= 1; }, phase);
    reject_after([](d::Observation& o) { ++o.parser.pop_pos; }, phase);
    reject_after([](d::Observation& o) { o.parser.use_symbol = o.parser.hero_symbol; }, phase);
    reject_after([](d::Observation& o) { ++o.item.value; }, phase);
  }
  // A normal return without restoration is insufficient.
  Fixture f;
  { d::Tracker t; CHECK(t.arm(f.job, f.admission, f.before)); CHECK(!t.original_returned(f.original, false, true)); CHECK(t.status().issue == d::Issue::original_exception); }
  { d::Tracker t; CHECK(t.arm(f.job, f.admission, f.before)); CHECK(!t.original_returned(f.original, true, false)); CHECK(t.status().issue == d::Issue::amount_restore); }
  { d::Tracker t; (void)through_debit_call(t, f); CHECK(!t.debit_returned(f.debit, false, true)); CHECK(t.status().issue == d::Issue::debit_exception); CHECK(!t.begin_debit(f.late).valid); }
  { d::Tracker t; (void)through_debit_call(t, f); CHECK(!t.debit_returned(f.debit, true, false)); CHECK(t.status().issue == d::Issue::restore); }
  // No zero debit, second original call or second debit ticket submission.
  { d::Tracker t; CHECK(!t.begin_debit(f.late).valid); CHECK(t.status().blocked); }
  { d::Tracker t; CHECK(t.arm(f.job, f.admission, f.before)); CHECK(!t.begin_debit(f.late).valid); }
  { d::Tracker t; CHECK(t.arm(f.job, f.admission, f.before)); CHECK(t.original_returned(f.original, true, true)); CHECK(!t.original_returned(f.original, true, true)); }
  { d::Tracker t; (void)through_debit_call(t, f); CHECK(!t.begin_debit(f.late).valid); CHECK(!t.debit_returned(f.debit, true, true)); }
  { d::Tracker t; (void)through_debit_call(t, f); CHECK(!t.finish(f.end, f.witness)); }
}

template<class Mutation> void reject_restore(Mutation mutate) {
  Fixture f; d::Tracker t; auto call = through_debit_call(t, f); auto r = restore_observation(call);
  mutate(call, r); CHECK(!d::restore_plan(call, r).valid);
}
void restoration_guards() {
  Fixture f; d::Tracker t; const auto call = through_debit_call(t, f);
  auto r = restore_observation(call);
  r.item.value = call.item.value;
  const auto noop = d::restore_plan(call, r); CHECK(noop.valid && !noop.write_item);
  // Any change to an active bit blocks execution, even if the print value 0 or 1 is individually recognized.
  for (std::size_t word = 0; word < 2; ++word) for (unsigned bit = 0; bit < 32; ++bit) {
    auto mutated = r; mutated.current.data_words[word] ^= std::uint32_t{1} << bit;
    CHECK(!d::restore_plan(call, mutated).valid);
  }
  for (std::size_t word = 2; word < 8; ++word) {
    auto inactive = r; inactive.current.data_words[word] = UINT32_MAX;
    CHECK(d::restore_plan(call, inactive).valid);
  }
  for (const auto n : {0u, 1u, 3u, 4u, 8u, 2048u, UINT32_MAX}) {
    auto bad = r; bad.current.data_depth = n; CHECK(!d::restore_plan(call, bad).valid);
  }
  reject_restore([](d::Call& c, d::RestoreObservation&) { c.valid = false; });
  reject_restore([](d::Call& c, d::RestoreObservation&) { c.instance = 16604; });
  reject_restore([](d::Call& c, d::RestoreObservation&) { c.quantity = 0; });
  reject_restore([](d::Call& c, d::RestoreObservation&) { c.quantity = 2; });
  reject_restore([](d::Call& c, d::RestoreObservation&) { c.job.callback = 20778; });
  reject_restore([](d::Call& c, d::RestoreObservation&) { c.stack_object = c.generated_left; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.serial = 3; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.native_item_set_offset_contract = false; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.fence.excludes_dispatch = false; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.fence.destroy_started = 1; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.fence.token; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.targets.native_restore_contract = false; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.targets.known = false; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.targets.context.epoch; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.targets.scope.job; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.targets.scope.writer_token; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.targets.scope.debit_depth = 0; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.targets.scope.vm_depth = 1; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.targets.saved_use_symbol; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.targets.saved_use_address; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.targets.self; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.current.self; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.current.hero_symbol; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.current.code_base; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.current.data_words_known = false; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.item.known = false; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.item.metadata_verified = false; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.item.parser; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.item.symbol_object = 0xfffffffcu; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.item.table = UINT32_MAX; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { o.item.symbol_count = 0; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.item.type; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.item.flags; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.item.elements; });
  reject_restore([](d::Call&, d::RestoreObservation& o) { ++o.item.parent; });
  // Replan after a setter. A changed ITEM target blocks execution.
  auto next = r; CHECK(d::restore_plan(call, next).valid);
  next.item.symbol_object += 4; ++next.serial; next.fence.serial = next.serial;
  CHECK(!d::restore_plan(call, next).valid);
}

template<class Mutation> void reject_finish(Mutation mutate) {
  Fixture f; d::Tracker t; through_verified(t, f); mutate(f.end, f.witness);
  CHECK(!t.finish(f.end, f.witness)); CHECK(t.status().blocked);
  Fixture fresh; CHECK(!t.arm(fresh.job, fresh.admission, fresh.before));
}
void completion_guards() {
  reject_finish([](d::Observation&, d::EndWitness& w) { w.normal_end_after = false; });
  reject_finish([](d::Observation&, d::EndWitness& w) { w.destroy_after = false; });
  reject_finish([](d::Observation&, d::EndWitness& w) { w.native_paths_verified = false; });
  reject_finish([](d::Observation&, d::EndWitness& w) { ++w.departed.mob; });
  reject_finish([](d::Observation&, d::EndWitness& w) { ++w.job; });
  reject_finish([](d::Observation&, d::EndWitness& w) { ++w.writer_token; });
  reject_finish([](d::Observation&, d::EndWitness& w) { ++w.fence_token; });
  reject_finish([](d::Observation&, d::EndWitness& w) { ++w.generated_anchor; });
  reject_finish([](d::Observation&, d::EndWitness& w) { w.instance = 16604; });
  reject_finish([](d::Observation&, d::EndWitness& w) { w.native_debit = 2; });
  reject_finish([](d::Observation&, d::EndWitness& w) { w.completed_return = 0; });
  for (const auto serial : {UINT64_C(0), UINT64_C(4), UINT64_C(5), UINT64_C(7), UINT64_C(8), UINT64_MAX})
    reject_finish([=](d::Observation&, d::EndWitness& w) { w.destroy_serial = serial; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.serial = 7; o.fence.serial = 7; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.context.mob = 0x400000; });
  reject_finish([](d::Observation& o, d::EndWitness&) { ++o.context.thread; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.production = 13; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.invincible = 1; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.scope.job = 10; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.scope.writer_active = true; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.hand.left = 0x600000; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.hand.right = 1; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.hand.anchor = 0x600000; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.hand.known = false; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.stack.object = 0x500000; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.stack.listed_stacks = 1; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.stack.packed = 1; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.stack.known = false; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.fence.excludes_dispatch = true; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.fence.destroy_started = 2; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.fence.destroy_completed = 0; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.stocks.stocks[0].amount = 1; });
  reject_finish([](d::Observation& o, d::EndWitness&) { --o.stocks.stocks[1].amount; });
  reject_finish([](d::Observation& o, d::EndWitness&) { o.stocks.stocks[0].unambiguous = false; });
  // A new complete order requires new monotonic IDs and fresh reads.
  Fixture first; d::Tracker tracker; through_verified(tracker, first); CHECK(tracker.finish(first.end, first.witness));
  CHECK(!tracker.arm(first.job, first.admission, first.before));
  Fixture second;
  ++second.job.id; ++second.job.writer_token; ++second.job.fence_token;
  second.admission.job = second.job.id; second.admission.writer_token = second.job.writer_token;
  second.admission.fence_token = second.job.fence_token; second.admission.serial += 10;
  for (auto* o : {&second.before, &second.original, &second.late, &second.debit}) {
    o->scope.job = second.job.id; o->scope.writer_token = second.job.writer_token;
  }
  for (auto* o : {&second.before, &second.original, &second.late, &second.debit, &second.end}) {
    o->serial += 10; o->fence.serial = o->serial; o->fence.token = second.job.fence_token;
  }
  second.witness.job = second.job.id; second.witness.writer_token = second.job.writer_token;
  second.witness.fence_token = second.job.fence_token;
  second.witness.destroy_serial += 10; second.witness.normal_end_serial += 10;
  through_verified(tracker, second); CHECK(tracker.finish(second.end, second.witness));
}

int main() {
  happy_and_limits(); entry_guards(); phase_guards(); restoration_guards(); completion_guards();
  std::printf("cool_hotblade_debit: %d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
