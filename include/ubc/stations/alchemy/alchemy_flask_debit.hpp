// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

// Isolated post-debit, with no engine calls or runtime authorization. Covers only
// the 13 statically linear individual infos. Spirit, tobacco choices and the Eye of
// Innos are deliberately omitted. Facts: L'Hiver DE2.0.4, DAT-SHA71a7875a...907fbb4.
namespace ubc::alchemy_flask_debit {
inline constexpr bool runtime_ready = false;
inline constexpr std::uint32_t flask_symbol = 16604;
struct Cost {
  std::uint32_t symbol = 0, units = 0;
};
struct Profile {
  std::uint32_t callback, owner, output;
  std::array<Cost, 4> costs;
  std::size_t cost_count;
};
inline constexpr std::array<Profile, 13> profiles{
    {{20289, 20287, 15678, {{{16706, 1}, {16704, 2}, {16143, 1}, {15673, 1}}}, 4},
     {20292, 20290, 15680, {{{16706, 1}, {16704, 2}, {16143, 1}, {15673, 2}}}, 4},
     {20295, 20293, 15736, {{{16724, 1}, {15673, 1}, {16375, 1}, {}}}, 3},
     {20329, 20327, 15745, {{{16766, 3}, {16736, 1}, {}, {}}}, 2},
     {20360, 20358, 15743, {{{16775, 3}, {16736, 1}, {}, {}}}, 2},
     {20373, 20371, 16790, {{{16718, 1}, {16738, 1}, {}, {}}}, 2},
     {20376, 20374, 16794, {{{16712, 1}, {16738, 1}, {}, {}}}, 2},
     {20379, 20377, 16787, {{{16720, 1}, {16738, 1}, {}, {}}}, 2},
     {20382, 20380, 16784, {{{16722, 1}, {16738, 1}, {}, {}}}, 2},
     {20385, 20383, 16798, {{{16724, 1}, {16736, 1}, {}, {}}}, 2},
     {20395, 20393, 16620, {{{16706, 1}, {}, {}, {}}}, 1},
     {20398, 20396, 15704, {{{16706, 2}, {16736, 1}, {}, {}}}, 2},
     {20408, 20406, 16800, {{{17585, 10}, {16659, 1}, {16652, 1}, {}}}, 3}}};
// Local capacity of these exact 13 profiles: four costs + output + flask.
// No limit for the generic inventory reader or other recipe catalogs.
inline constexpr std::size_t row_capacity = 6;
inline const Profile* find(std::uint32_t callback) noexcept {
  for (const auto& p : profiles)
    if (p.callback == callback)
      return &p;
  return nullptr;
}
inline bool valid(const Profile& p) noexcept {
  if (!p.callback || !p.owner || !p.output || p.output == flask_symbol || !p.cost_count ||
      p.cost_count > p.costs.size())
    return false;
  for (std::size_t i = 0; i < p.cost_count; ++i) {
    const auto& c = p.costs[i];
    if (!c.symbol || !c.units || c.units > INT32_MAX || c.symbol == p.output ||
        c.symbol == flask_symbol)
      return false;
    for (std::size_t j = 0; j < i; ++j)
      if (p.costs[j].symbol == c.symbol)
        return false;
  }
  for (std::size_t i = p.cost_count; i < p.costs.size(); ++i)
    if (p.costs[i].symbol || p.costs[i].units)
      return false;
  return true;
}
struct Context {
  std::uint64_t epoch = 0;
  std::uint32_t thread = 0, parser = 0, game = 0, world = 0, hero = 0, mob = 0;
};
inline bool valid(const Context& c) noexcept {
  return c.epoch && c.thread && c.parser == lhiver::game_parser && c.game && c.world && c.hero &&
         c.mob;
}
inline bool same(const Context& a, const Context& b, bool require_mob = true) noexcept {
  return a.epoch == b.epoch && a.thread == b.thread && a.parser == b.parser && a.game == b.game &&
         a.world == b.world && a.hero == b.hero && (!require_mob || a.mob == b.mob);
}
struct Job {
  Context context{};
  std::uint64_t id = 0, writer_token = 0;
  std::uint32_t callback = 0, owner = 0, quantity = 0;
};
struct Scope {
  std::uint64_t job = 0, writer_token = 0;
  std::uint32_t callback = 0, writer_depth = 0, vm_depth = 0, debit_depth = 0;
  bool writer_active = false;
};
struct Admission {
  bool local_script_contract = false, original_amount_contract = false;
  bool native_debit_and_restore_contract = false, flask_constructor = false;
  bool owner_and_knowledge = false, physical_lab_hand_contract = false;
};
struct Row {
  std::uint32_t symbol = 0;
  std::int64_t stored = -1;
  bool known = false;
};
struct Stocks {
  std::array<Row, row_capacity> rows{};
  std::size_t count = 0;
  // Complete stable list+pack read, no mixed/duplicated ingredients,
  // all list/slot aliases checked. Do not derive from mere quantity.
  bool complete_unambiguous = false;
};
struct FlaskStack {
  bool known = false, all_slot_aliases_excluded = false, native_item_contract = false;
  std::int64_t packed = -1;
  std::uint32_t listed_stacks = 0, object = 0, amount = 0, flags = 0;
};
struct Hand {
  bool known = false, left_empty = false;
  std::uint32_t right = 0, symbol = 0, amount = 0, interact_item = 0;
};
inline bool same(const Hand& a, const Hand& b) noexcept {
  return a.known && b.known && a.left_empty && b.left_empty && a.right == b.right &&
         a.symbol == b.symbol && a.amount == b.amount && a.interact_item == b.interact_item;
}
struct ParserState {
  bool known = false, instance_bindings_verified = false;
  std::uint32_t current_parser = 0, code_base = 0, code_size = 0, pop_pos = 0;
  std::uint32_t data_depth = 0, use_symbol = 0, use_address = 0;
  std::uint32_t self_symbol = 0, hero_symbol = 0, self = 0, hero = 0, other = 0, victim = 0;
  // Only active words, at most the separately evidenced 13
  // AFTER-prefix. No general admission of a non-empty VM data stack.
  std::array<std::uint32_t, 8> data_words{};
  bool data_words_known = false;
};
inline bool valid_parser_bindings(const ParserState& s, const Context& c) noexcept {
  return s.known && s.instance_bindings_verified && s.current_parser == c.parser && s.code_base &&
         s.code_size && s.code_size <= 0x1000000u && s.pop_pos <= s.code_size &&
         static_cast<std::uint64_t>(s.code_base) + s.code_size <= UINT32_MAX &&
         s.data_depth <= 2048 && s.self_symbol && s.hero_symbol && s.self_symbol != s.hero_symbol &&
         s.self == c.hero && s.hero == c.hero && s.use_address == c.hero &&
         (s.use_symbol == s.self_symbol || s.use_symbol == s.hero_symbol);
}
inline bool valid(const ParserState& s, const Context& c) noexcept {
  return valid_parser_bindings(s, c) && s.data_depth == 0;
}
// Freshly derived from DAT per success path, independent of N. Every unused
// Npc_RemoveInvItems-INT-Return leaves [1,0x40]. BEFORE remains empty.
inline std::uint32_t original_return_words(std::uint32_t callback) noexcept {
  switch (callback) {
  case 20289:
  case 20292:
    return 8;
  case 20295:
  case 20408:
    return 6;
  case 20329:
  case 20360:
  case 20373:
  case 20376:
  case 20379:
  case 20382:
  case 20385:
  case 20398:
    return 4;
  case 20395:
    return 2;
  default:
    return 0;
  }
}
inline bool valid_after_original(const ParserState& s, const Context& c,
                                 std::uint32_t callback) noexcept {
  // The three tobacco choices only call functions without INT return.
  if (callback >= 20403 && callback <= 20405)
    return valid(s, c);
  const auto count = original_return_words(callback);
  if (!count || !valid_parser_bindings(s, c) || !s.data_words_known || s.data_depth != count)
    return false;
  for (std::uint32_t i = 0; i < count; ++i)
    if (s.data_words[i] != (i % 2 ? 0x40u : 1u))
      return false;
  return true;
}
inline bool same_data_stack(const ParserState& a, const ParserState& b) noexcept {
  if (a.data_depth != b.data_depth)
    return false;
  if (!a.data_depth)
    return true;
  if (!a.data_words_known || !b.data_words_known || a.data_depth > a.data_words.size())
    return false;
  for (std::uint32_t i = 0; i < a.data_depth; ++i)
    if (a.data_words[i] != b.data_words[i])
      return false;
  return true;
}
inline bool same(const ParserState& a, const ParserState& b) noexcept {
  return a.known && b.known && a.instance_bindings_verified && b.instance_bindings_verified &&
         a.current_parser == b.current_parser && a.code_base == b.code_base &&
         a.code_size == b.code_size && a.pop_pos == b.pop_pos && same_data_stack(a, b) &&
         a.use_symbol == b.use_symbol && a.use_address == b.use_address &&
         a.self_symbol == b.self_symbol && a.hero_symbol == b.hero_symbol && a.self == b.self &&
         a.hero == b.hero && a.other == b.other && a.victim == b.victim;
}
struct Observation {
  Context context{};
  Scope scope{};
  std::uint64_t serial = 0;
  Stocks stocks{};
  FlaskStack flask{};
  Hand hand{};
  ParserState parser{};
  std::int32_t production = -1, invincible = -1;
  bool native_contract = false, stable_read = false;
};
enum class Issue {
  none,
  context,
  scope,
  observation,
  stocks,
  hand,
  flask,
  parser,
  original_result,
  original_exception,
  amount_restore,
  sequence,
  debit_exception,
  debit_result,
  context_restore,
  bridge,
  terminal
};
enum class Stage { idle, armed, awaiting_debit, calling, debit_verified, complete };
struct Status {
  Stage stage = Stage::idle;
  Issue issue = Issue::none;
  bool blocked = false;
};
struct Call {
  bool valid = false;
  std::uint64_t job = 0, writer_token = 0;
  Context context{};
  std::uint32_t callback = 0, instance = 0, quantity = 0, stack_object = 0;
  ParserState parser{};
};
// The reader must re-check the stored restoration symbol pointer again
// based on index/metadata and its current instance address. Do not blindly
// describe old pointers. Current UseInstance/PopPos may differ internally.
struct RestoreObservation {
  Context context{};
  Scope scope{};
  bool known = false, native_restore_contract = false;
  std::uint32_t current_parser = 0, code_base = 0, code_size = 0;
  std::uint32_t saved_use_symbol = 0, saved_use_address = 0;
  std::uint32_t self_symbol = 0, hero_symbol = 0, self = 0, hero = 0, other = 0, victim = 0;
};
inline bool restorable(const Call& call, const RestoreObservation& v) noexcept {
  const auto& p = call.parser;
  return call.valid && v.known && v.native_restore_contract && same(call.context, v.context) &&
         v.scope.writer_active && v.scope.writer_depth == 1 && v.scope.vm_depth == 0 &&
         v.scope.debit_depth == 1 && v.scope.job == call.job &&
         v.scope.writer_token == call.writer_token && v.scope.callback == call.callback &&
         v.current_parser == call.context.parser && v.code_base == p.code_base &&
         v.code_size == p.code_size && v.saved_use_symbol == p.use_symbol &&
         v.saved_use_address == p.use_address && v.self_symbol == p.self_symbol &&
         v.hero_symbol == p.hero_symbol && v.self == p.self && v.hero == p.hero &&
         v.other == p.other && v.victim == p.victim;
}
struct EndWitness {
  bool after = false;
  std::uint64_t job = 0, writer_token = 0, serial = 0;
  Context departed{}; // Old physical station, not the later Mob0 value.
};

inline bool read_rows(const Profile& p, const Stocks& v,
                      std::array<std::int64_t, row_capacity>& result) noexcept {
  if (!v.complete_unambiguous || v.count != p.cost_count + 2 || v.count > v.rows.size())
    return false;
  for (std::size_t i = 0; i < v.count; ++i) {
    const auto symbol =
        i < p.cost_count ? p.costs[i].symbol : (i == p.cost_count ? p.output : flask_symbol);
    const Row* found = nullptr;
    for (std::size_t j = 0; j < v.count; ++j)
      if (v.rows[j].symbol == symbol) {
        if (found)
          return false;
        found = &v.rows[j];
      }
    if (!found || !found->known || found->stored < 0 || found->stored > INT32_MAX)
      return false;
    result[i] = found->stored;
  }
  return true;
}
inline bool listed_flask(const FlaskStack& s, std::int64_t total,
                         std::uint32_t old_object = 0) noexcept {
  if (!s.known || !s.all_slot_aliases_excluded || !s.native_item_contract || s.packed != 0)
    return false;
  if (total == 0)
    return s.listed_stacks == 0 && s.object == 0 && s.amount == 0;
  return total > 0 && total <= INT32_MAX && s.listed_stacks == 1 && s.object &&
         (!old_object || s.object == old_object) && s.amount == static_cast<std::uint32_t>(total) &&
         (s.flags & 0x00200000u) != 0 && (s.flags & 0x40000000u) == 0;
}

// A single writer window, no repetition/compensation. Except
// arm() after a fully completed order there is no reset. Native
// adapters must truly set the admission booleans and hold the same
// bridge permanently; this value core reads no process itself.
class Tracker {
public:
  Tracker() noexcept = default;
  Tracker(const Tracker&) = delete;
  Tracker& operator=(const Tracker&) = delete;
  Status status() const noexcept { return status_; }
  const Job& job() const noexcept { return job_; }
  bool quarantine(Issue issue) noexcept {
    if (!status_.blocked) {
      status_.blocked = true;
      status_.issue = issue;
    }
    return false;
  }
  bool arm(const Job& j, const Admission& a, const Observation& before) noexcept {
    if (status_.blocked || (status_.stage != Stage::idle && status_.stage != Stage::complete))
      return false;
    const auto* p = find(j.callback);
    if (!p || !valid(*p) || !valid(j.context) || j.owner != p->owner || !j.id ||
        j.id <= last_job_ || !j.writer_token || j.writer_token <= last_token_ || !j.quantity ||
        j.quantity > INT32_MAX || !a.local_script_contract || !a.original_amount_contract ||
        !a.native_debit_and_restore_contract || !a.flask_constructor || !a.owner_and_knowledge ||
        !a.physical_lab_hand_contract)
      return false;
    if (!same(j.context, before.context) || !scope_matches(j, before.scope, 0) ||
        !base_read(before) || before.serial <= last_serial_ || before.production != 4 ||
        before.invincible != 1 || !valid_hand(before.hand) || !valid(before.parser, j.context))
      return false;
    std::array<std::int64_t, row_capacity> stocks{};
    if (!read_rows(*p, before.stocks, stocks))
      return false;
    for (std::size_t i = 0; i < p->cost_count; ++i)
      if (static_cast<std::uint64_t>(j.quantity) * p->costs[i].units >
          static_cast<std::uint64_t>(stocks[i]))
        return false;
    if (stocks[p->cost_count] + j.quantity > INT32_MAX ||
        stocks[p->cost_count + 1] < j.quantity - 1)
      return false;
    if (j.quantity > 1 && (!listed_flask(before.flask, stocks[p->cost_count + 1]) ||
                           before.flask.object == before.hand.right))
      return false;
    job_ = j;
    profile_ = p;
    before_ = before;
    expected_ = stocks;
    for (std::size_t i = 0; i < p->cost_count; ++i)
      expected_[i] -= static_cast<std::int64_t>(j.quantity) * p->costs[i].units;
    expected_[p->cost_count] += j.quantity;
    status_ = {Stage::armed, Issue::none, false};
    last_serial_ = before.serial;
    last_job_ = j.id;
    last_token_ = j.writer_token;
    call_ = {};
    post_parser_ = {};
    return true;
  }
  bool original_returned(const Observation& after, bool normal, bool amount_restored) noexcept {
    if (status_.blocked)
      return false;
    if (status_.stage != Stage::armed)
      return quarantine(Issue::sequence);
    if (!normal)
      return quarantine(Issue::original_exception);
    if (!amount_restored)
      return quarantine(Issue::amount_restore);
    if (!common_after(after, 0))
      return false;
    if (!rows_equal(after.stocks, expected_))
      return quarantine(Issue::original_result);
    if (!valid_after_original(after.parser, job_.context, job_.callback))
      return quarantine(Issue::parser);
    if (job_.quantity > 1 && !same_flask_before(after))
      return quarantine(Issue::flask);
    post_parser_ = after.parser;
    last_serial_ = after.serial;
    status_.stage = job_.quantity == 1 ? Stage::debit_verified : Stage::awaiting_debit;
    return true;
  }
  Call begin_debit(const Observation& late) noexcept {
    if (status_.blocked)
      return {};
    if (status_.stage != Stage::awaiting_debit) {
      quarantine(Issue::sequence);
      return {};
    }
    if (!common_after(late, 0))
      return {};
    if (!rows_equal(late.stocks, expected_) || !same_flask_before(late)) {
      quarantine(Issue::stocks);
      return {};
    }
    if (!same(post_parser_, late.parser)) {
      quarantine(Issue::parser);
      return {};
    }
    call_ = {true,         job_.id,           job_.writer_token,    job_.context, job_.callback,
             flask_symbol, job_.quantity - 1, before_.flask.object, late.parser};
    last_serial_ = late.serial;
    status_.stage = Stage::calling;
    return call_;
  }
  bool debit_returned(const Observation& after, bool normal, bool context_restored) noexcept {
    if (status_.blocked)
      return false;
    if (status_.stage != Stage::calling)
      return quarantine(Issue::sequence);
    if (!normal)
      return quarantine(Issue::debit_exception);
    if (!context_restored)
      return quarantine(Issue::context_restore);
    if (!common_after(after, 1))
      return false;
    if (!same(post_parser_, after.parser))
      return quarantine(Issue::parser);
    auto expected = expected_;
    expected[profile_->cost_count + 1] -= job_.quantity - 1;
    if (!rows_equal(after.stocks, expected))
      return quarantine(Issue::debit_result);
    if (!listed_flask(after.flask, expected[profile_->cost_count + 1], before_.flask.object))
      return quarantine(Issue::flask);
    expected_ = expected;
    last_serial_ = after.serial;
    status_.stage = Stage::debit_verified;
    return true;
  }
  bool finish(const Observation& after, const EndWitness& witness) noexcept {
    if (status_.blocked)
      return false;
    if (status_.stage != Stage::debit_verified)
      return quarantine(Issue::sequence);
    if (!base_read(after) || after.serial <= last_serial_ ||
        !same(job_.context, after.context, false) || after.context.mob || after.production != 0 ||
        after.invincible != 0 || after.scope.writer_active || after.scope.writer_depth ||
        after.scope.vm_depth || after.scope.debit_depth || !after.hand.known ||
        !after.hand.left_empty || after.hand.right || after.hand.symbol || after.hand.amount ||
        after.hand.interact_item || !witness.after || witness.job != job_.id ||
        witness.writer_token != job_.writer_token || !same(witness.departed, job_.context) ||
        witness.serial <= last_serial_ || witness.serial >= after.serial)
      return quarantine(Issue::terminal);
    if (!rows_equal(after.stocks, expected_))
      return quarantine(Issue::terminal);
    last_serial_ = after.serial;
    status_.stage = Stage::complete;
    return true;
  }

private:
  static bool base_read(const Observation& v) noexcept {
    return v.serial && v.native_contract && v.stable_read;
  }
  static bool valid_hand(const Hand& h) noexcept {
    return h.known && h.left_empty && h.right && h.symbol == flask_symbol && h.amount == 1 &&
           h.interact_item == h.right;
  }
  static bool scope_matches(const Job& j, const Scope& s, std::uint32_t debit_depth) noexcept {
    return s.writer_active && s.writer_depth == 1 && s.vm_depth == 0 &&
           s.debit_depth == debit_depth && s.job == j.id && s.writer_token == j.writer_token &&
           s.callback == j.callback;
  }
  bool common_after(const Observation& v, std::uint32_t debit_depth) noexcept {
    if (!same(job_.context, v.context))
      return quarantine(Issue::context);
    if (!scope_matches(job_, v.scope, debit_depth))
      return quarantine(Issue::scope);
    if (!base_read(v) || v.serial <= last_serial_)
      return quarantine(Issue::observation);
    if (v.production != 0 || v.invincible != 0)
      return quarantine(Issue::original_result);
    if (!valid_hand(v.hand) || !same(before_.hand, v.hand))
      return quarantine(Issue::hand);
    return true;
  }
  bool rows_equal(const Stocks& values,
                  const std::array<std::int64_t, row_capacity>& expected) const noexcept {
    std::array<std::int64_t, row_capacity> got{};
    if (!profile_ || !read_rows(*profile_, values, got))
      return false;
    for (std::size_t i = 0; i < profile_->cost_count + 2; ++i)
      if (got[i] != expected[i])
        return false;
    return true;
  }
  bool same_flask_before(const Observation& v) const noexcept {
    return listed_flask(v.flask, expected_[profile_->cost_count + 1], before_.flask.object) &&
           v.flask.flags == before_.flask.flags && v.flask.object != v.hand.right;
  }
  Status status_{};
  Job job_{};
  const Profile* profile_ = nullptr;
  Observation before_{};
  ParserState post_parser_{};
  Call call_{};
  std::array<std::int64_t, row_capacity> expected_{};
  std::uint64_t last_serial_ = 0, last_job_ = 0, last_token_ = 0;
};
} // namespace ubc::alchemy_flask_debit
