// SPDX-License-Identifier: MIT
#include "ubc/stations/spirit/spirit_result.hpp"
#include "ubc/stations/spirit/spirit_item_restore.hpp"
#include <cstdio>
#include <climits>

namespace spirit_result_test {
namespace d = ubc::spirit_debit;
namespace c = ubc::spirit_recipe_catalog;
unsigned checks = 0, failures = 0;
void check(bool value, const char* why) {
  ++checks;
  if (!value) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", why);
  }
}
struct Fixture {
  ubc::SpiritRecipeCapture captured{};
  d::Job job{};
  d::Observation before{};
  d::Admission admission{true, true, true, true, true, true};
  Fixture(std::uint32_t quantity = 3, std::int32_t branch = 1) {
    job.context = {1, 7, ubc::lhiver::game_parser, 0x1000, 0x2000, 0x3000, 0x4000};
    job.id = 1;
    job.writer_token = 2;
    job.callback = 20370;
    job.owner = 20368;
    job.quantity = quantity;
    job.branch_value = branch;
    captured.key = {1, 7, 0x2000, 0x3000, 0x4000, 0x5000, ubc::recipe_catalog::Station::alchemy};
    auto& s = captured.state;
    s.evaluated = s.known = s.contract = s.branch_known = s.actor_known = s.actor_matches =
        s.context_known = s.context_matches = s.owner_conditions_match = true;
    s.branch_value = branch;
    s.branch = c::branch_for(true, branch);
    s.output = branch == 1 ? 15741 : 15739;
    auto& h = captured.hands;
    h.hands = {{0, -1, -1, 0}, {1, 16604, 1, 0x6000}};
    h.left_slot_known = h.right_slot_known = h.interact_item_known = true;
    h.interact_item = 0x6000;
    h.right_tmp_level = 1;
    std::array<ubc::RecipeInventoryItem, 7> items{};
    std::array<std::uint32_t, 7> objects{};
    for (std::size_t i = 0; i < 7; ++i) {
      const auto amount = i < 5 ? 100 : static_cast<int>(i - 2);
      items[i] = {c::targets[i].symbol, amount, 0, amount, 1, 0, false, false, false};
      objects[i] = 0x7000 + static_cast<std::uint32_t>(i) * 0x100;
      before.stocks.rows[i] = {static_cast<std::uint32_t>(c::targets[i].symbol), amount, true};
    }
    ubc::RecipeInventory inv;
    inv.header_known = inv.listed_known = inv.packed_known = inv.storage_totals_known =
        inv.stable_boundary_verified = inv.snapshot_unchanged = true;
    inv.listed_nodes = 7;
    inv.items = {items.data(), 7};
    inv.listed_objects = {objects.data(), 7};
    s.material = ubc::spirit_recipe_material(c::recipes[0], inv, h, s.branch, 0, s.rows.data(), 7,
                                             true, true);
    before.context = job.context;
    before.scope = {job.id, job.writer_token, 20370, 1, 0, 0, true};
    before.serial = 10;
    before.stocks.count = 7;
    before.stocks.complete_unambiguous = true;
    before.flask = {true, true, true, 0, 1, objects[4], 100, 0x200001};
    before.hand = {true, true, 0x6000, 16604, 1, 0x6000};
    before.parser = {true,
                     true,
                     ubc::lhiver::game_parser,
                     0x1000000,
                     ubc::lhiver::code_length,
                     100,
                     0,
                     0x8000,
                     0x3000,
                     0x8000,
                     0x8100,
                     0x3000,
                     0x3000,
                     0x9000,
                     0xA000};
    before.production = 4;
    before.invincible = 1;
    before.native_contract = before.stable_read = before.branch_known = true;
    before.branch_value = branch;
  }
  d::Observation original() const {
    auto v = before;
    v.serial = 11;
    v.production = v.invincible = 0;
    // Four original Npc_RemoveInvItems INT returns in VOID callback 20370.
    v.parser.data_depth = 8;
    v.parser.data_words_known = true;
    v.parser.data_words = {1, 64, 1, 64, 1, 64, 1, 64};
    for (std::size_t i = 0; i < 4; ++i)
      v.stocks.rows[i].stored -= static_cast<std::int64_t>(job.quantity) * (i ? 1 : 2);
    v.stocks.rows[job.branch_value == 1 ? 6 : 5].stored += job.quantity;
    return v;
  }
  d::Observation debit_after() const {
    auto v = original();
    v.serial = 13;
    v.scope.debit_depth = 1;
    v.stocks.rows[4].stored -= job.quantity - 1;
    v.flask.amount -= job.quantity - 1;
    if (!v.flask.amount) {
      v.flask.object = 0;
      v.flask.listed_stacks = 0;
    }
    return v;
  }
  d::Observation end() const {
    auto v = job.quantity == 1 ? original() : debit_after();
    v.serial = 16;
    v.context.mob = 0;
    v.scope = {};
    v.hand = {true, true, 0, 0, 0, 0};
    return v;
  }
  d::EndWitness witness() const { return {true, job.id, job.writer_token, 15, job.context}; }
};
bool through_debit(ubc::SpiritResult& r, const Fixture& f) {
  if (!r.begin(f.captured, f.job, f.admission, f.before) ||
      !r.original_returned(f.original(), true, true))
    return false;
  if (f.job.quantity == 1)
    return true;
  auto late = f.original();
  late.serial = 12;
  const auto call = r.tracker_for_bridge().begin_debit(late);
  check(call.valid && call.quantity == f.job.quantity - 1 && call.instance == 16604,
        "positive N-1 call");
  return call.valid && r.tracker_for_bridge().debit_returned(f.debit_after(), true, true) &&
         r.acknowledge_extra_debit(f.debit_after());
}
void run() {
  for (const auto branch : {1, 0, 2, INT_MIN, INT_MAX})
    for (const auto n : {1u, 2u, 3u, 50u}) {
      Fixture f(n, branch);
      ubc::SpiritResult r;
      check(through_debit(r, f), "both outputs and full N range");
      check(r.status().stage == ubc::SpiritResultStage::closing && !r.receipt().verified,
            "return/debit is not terminal success");
      check(r.observe_completion(f.end(), f.witness(), true) == ubc::SpiritResultStage::verified,
            "fresh normal end verifies");
      check(r.receipt().count == 7 && r.receipt().output == f.captured.state.output &&
                r.receipt().branch_value == branch,
            "receipt binds exact branch and seven rows");
      for (std::size_t i = 0; i < 7; ++i)
        check(r.receipt().rows[i].actual == f.end().stocks.rows[i].stored,
              "every observed terminal amount copied");
    }
  for (unsigned mode = 0; mode < 24; ++mode) {
    Fixture f;
    ubc::SpiritResult r;
    switch (mode) {
    case 0:
      f.job.quantity = 0;
      break;
    case 1:
      f.job.quantity = 51;
      break;
    case 2:
      f.job.owner++;
      break;
    case 3:
      f.job.callback++;
      break;
    case 4:
      f.captured.state.known = false;
      break;
    case 5:
      f.captured.state.contract = false;
      break;
    case 6:
      f.captured.state.branch_known = false;
      break;
    case 7:
      f.captured.state.branch_value = 0;
      break;
    case 8:
      f.before.branch_value = 0;
      break;
    case 9:
      f.before.branch_known = false;
      break;
    case 10:
      f.before.stocks.count = 6;
      break;
    case 11:
      f.before.stocks.rows[6].symbol = 15739;
      break;
    case 12:
      f.before.stocks.rows[6].stored++;
      break;
    case 13:
      f.captured.state.material.maximum++;
      break;
    case 14:
      f.captured.hands.hands.right.object++;
      break;
    case 15:
      f.before.flask.packed = 1;
      break;
    case 16:
      f.admission.owner_and_knowledge = false;
      break;
    case 17:
      f.before.parser.data_depth = 1;
      break;
    case 18:
      f.before.scope.vm_depth = 1;
      break;
    case 19:
      f.captured.key.world++;
      break;
    case 20:
      f.captured.state.id = static_cast<c::RecipeId>(0x4001);
      break;
    case 21:
      f.captured.state.material.output = 15739;
      break;
    case 22:
      f.captured.state.material.additional_flasks = 1;
      break;
    case 23:
      f.captured.state.material.craftable = true;
      break;
    }
    check(!r.begin(f.captured, f.job, f.admission, f.before), "bad admission never arms");
  }
  {
    Fixture f;
    ubc::SpiritResult r;
    f.captured.state.owner_conditions_match = false;
    f.captured.state.actor_matches = false;
    check(
        r.begin(f.captured, f.job, f.admission, f.before),
        "preview category and SELF may differ before original navigation; fresh writer proves current actor and owner");
  }
  for (unsigned mode = 0; mode < 23; ++mode) {
    Fixture f(3, 2);
    ubc::SpiritResult r;
    check(r.begin(f.captured, f.job, f.admission, f.before), "arm original negative");
    auto v = f.original();
    switch (mode) {
    case 0:
      v.branch_value = 3;
      break;
    case 1:
      v.branch_known = false;
      break;
    case 2:
      v.stocks.rows[6].stored++;
      break;
    case 3:
      v.stocks.rows[5].stored--;
      break;
    case 4:
      v.stocks.rows[4].stored++;
      break;
    case 5:
      v.production = 4;
      break;
    case 6:
      v.hand.right++;
      break;
    case 7:
      v.context.mob++;
      break;
    case 8:
      v.parser.self++;
      break;
    case 9:
      v.serial = 10;
      break;
    case 10:
      v.scope.vm_depth = 1;
      break;
    case 11:
      v.stable_read = false;
      break;
    case 12:
      v.parser.data_depth = 0;
      break;
    case 13:
      v.parser.data_depth = 2;
      break;
    case 14:
      v.parser.data_depth = 4;
      break;
    case 15:
      v.parser.data_depth = 6;
      break;
    case 16:
      v.parser.data_depth = 7;
      break;
    case 17:
      v.parser.data_depth = 9;
      break;
    case 18:
      v.parser.data_words_known = false;
      break;
    case 19:
      v.parser.data_words[0] = 0;
      break;
    case 20:
      v.parser.data_words[7] = 1;
      break;
    case 21:
      v.parser.data_words[6] = 64;
      break;
    case 22:
      v.parser.data_depth = 2048;
      break;
    }
    check(!r.original_returned(v, true, true) &&
              r.status().stage == ubc::SpiritResultStage::blocked,
          "original mismatch quarantines before debit");
  }
  for (unsigned mode = 0; mode < 8; ++mode) {
    Fixture f;
    ubc::SpiritResult r;
    check(through_debit(r, f), "arm terminal negative");
    auto v = f.end();
    auto w = f.witness();
    if (mode == 0) {
      v.branch_known = false;
      check(r.observe_completion(v, w, true) == ubc::SpiritResultStage::closing,
            "unknown branch waits");
      v = f.end();
    } else if (mode == 1) {
      v.serial = 14;
      check(r.observe_completion(v, w, true) == ubc::SpiritResultStage::closing,
            "stale terminal waits");
      v = f.end();
    } else if (mode == 2) {
      check(r.observe_completion(v, w, false) == ubc::SpiritResultStage::closing,
            "generic after is not normal end");
    } else if (mode == 3) {
      w.writer_token++;
      check(r.observe_completion(v, w, true) == ubc::SpiritResultStage::closing,
            "foreign witness waits");
      w = f.witness();
    } else if (mode == 4)
      v.stocks.rows[5].stored++;
    else if (mode == 5)
      v.hand.interact_item = 1;
    else if (mode == 6)
      v.branch_value = 0;
    else if (mode == 7)
      v.context.world++;
    const auto expected =
        mode < 4 ? ubc::SpiritResultStage::verified : ubc::SpiritResultStage::blocked;
    check(r.observe_completion(v, w, true) == expected,
          "positive terminal mismatch blocks, unknown can recover");
  }
  {
    Fixture f;
    ubc::SpiritResult r;
    check(r.begin(f.captured, f.job, f.admission, f.before), "fault arm");
    check(!r.original_returned(f.original(), false, true), "original exception quarantines");
    check(!r.begin(f.captured, f.job, f.admission, f.before), "no auto retry");
  }
  {
    Fixture f;
    ubc::SpiritResult r;
    check(r.begin(f.captured, f.job, f.admission, f.before), "restore arm");
    check(!r.original_returned(f.original(), true, false), "unclear amount restore blocks");
  }
  {
    Fixture f;
    ubc::SpiritResult r;
    check(through_debit(r, f), "timeout arm");
    r.timeout();
    check(r.status().stage == ubc::SpiritResultStage::blocked, "explicit timeout is permanent");
  }
  {
    Fixture f(3, 2);
    ubc::SpiritResult r;
    check(r.begin(f.captured, f.job, f.admission, f.before) &&
              r.original_returned(f.original(), true, true),
          "late branch arm");
    auto late = f.original();
    late.serial = 12;
    late.branch_value = 3;
    check(!r.tracker_for_bridge().begin_debit(late).valid,
          "same-output exact-INT drift prevents debit");
    check(r.status().stage == ubc::SpiritResultStage::blocked, "late branch mismatch quarantines");
  }
  {
    Fixture f;
    ubc::SpiritResult r;
    check(r.begin(f.captured, f.job, f.admission, f.before) &&
              r.original_returned(f.original(), true, true),
          "duplicate debit arm");
    auto late = f.original();
    late.serial = 12;
    check(r.tracker_for_bridge().begin_debit(late).valid, "first debit call admitted");
    late.serial++;
    check(!r.tracker_for_bridge().begin_debit(late).valid &&
              r.status().stage == ubc::SpiritResultStage::blocked,
          "second debit call never admitted");
  }
  for (unsigned word = 0; word < 8; ++word) {
    Fixture f;
    d::Tracker t;
    check(t.arm(f.job, f.admission, f.before) && t.original_returned(f.original(), true, true),
          "prefix freshness arm");
    auto late = f.original();
    late.serial = 12;
    late.parser.data_words[word] ^= 1;
    check(!t.begin_debit(late).valid && t.status().blocked,
          "each changed active return word prevents debit");
    d::Tracker after;
    check(after.arm(f.job, f.admission, f.before) &&
              after.original_returned(f.original(), true, true),
          "post-debit prefix arm");
    late = f.original();
    late.serial = 12;
    check(after.begin_debit(late).valid, "unchanged return prefix admits once");
    auto changed = f.debit_after();
    changed.parser.data_words[word] ^= 1;
    check(!after.debit_returned(changed, true, true) && after.status().blocked,
          "each changed active return word prevents completion");
  }
  for (unsigned mode = 0; mode < 5; ++mode) {
    Fixture f;
    ubc::SpiritResult r;
    check(r.begin(f.captured, f.job, f.admission, f.before) &&
              r.original_returned(f.original(), true, true),
          "acknowledgement binds same original context");
    auto late = f.original();
    late.serial = 12;
    check(r.tracker_for_bridge().begin_debit(late).valid, "ack debit begun");
    auto after = f.debit_after();
    check(r.tracker_for_bridge().debit_returned(after, true, true),
          "bridge has exact post-debit proof");
    if (mode == 0)
      after.hand.right++;
    else if (mode == 1)
      after.parser.use_symbol++;
    else if (mode == 2)
      after.production = 4;
    else if (mode == 3)
      after.invincible = 1;
    else
      after.flask.object++;
    check(!r.acknowledge_extra_debit(after) && r.status().stage == ubc::SpiritResultStage::blocked,
          "different copied acknowledgement must not certify restore");
  }
  {
    Fixture f(1);
    ubc::SpiritResult r;
    check(through_debit(r, f), "completed sequence arm");
    check(r.observe_completion(f.end(), f.witness(), true) == ubc::SpiritResultStage::verified,
          "completed sequence first end");
    auto next = f;
    next.job.id++;
    next.job.writer_token++;
    next.before.scope.job = next.job.id;
    next.before.scope.writer_token = next.job.writer_token;
    check(!r.begin(next.captured, next.job, next.admission, next.before),
          "new job cannot reuse stale pre-observation");
    next.before.serial = 20;
    check(r.begin(next.captured, next.job, next.admission, next.before),
          "new authorized fresh job can reuse persistent result");
  }
  {
    Fixture f;
    d::Tracker tracker;
    check(tracker.arm(f.job, f.admission, f.before) &&
              tracker.original_returned(f.original(), true, true),
          "own ITEM plan arm");
    auto late = f.original();
    late.serial = 12;
    const auto original = tracker.begin_debit(late);
    auto scope = f.before.scope;
    scope.debit_depth = 1;
    ubc::alchemy_extra_dependencies::ItemBinding saved{true,
                                                       ubc::lhiver::game_parser,
                                                       0xA000,
                                                       0xB000,
                                                       0xC000,
                                                       original.parser.code_base,
                                                       original.parser.code_size,
                                                       ubc::lhiver::symbol_count};
    auto changed = saved;
    changed.value = 0xD000;
    check(!ubc::alchemy_extra_dependencies::item_restore_plan(original, saved, changed,
                                                              f.job.context, scope)
               .valid,
          "13-profile ITEM plan must not accept Spirit");
    const auto positive =
        ubc::spirit_item_restore::plan(original, saved, changed, f.job.context, scope);
    check(positive.valid && positive.requires_write && positive.symbol_object == saved.symbol &&
              positive.saved_value == saved.value && positive.native_entry == 0x7a2f20,
          "own exact callback binds same ITEM primitive");
    check(ubc::spirit_item_restore::plan(original, saved, saved, f.job.context, scope).valid &&
              !ubc::spirit_item_restore::plan(original, saved, saved, f.job.context, scope)
                   .requires_write,
          "same ITEM is read-only no-op");
    for (unsigned mode = 0; mode < 18; ++mode) {
      auto call = original;
      auto item = changed;
      auto s = scope;
      auto context = f.job.context;
      switch (mode) {
      case 0:
        call.callback = 20373;
        break;
      case 1:
        call.instance = 16602;
        break;
      case 2:
        call.quantity = 0;
        break;
      case 3:
        call.quantity = c::maximum_quantity;
        break;
      case 4:
        call.parser.data_depth = 1;
        break;
      case 5:
        context.hero++;
        break;
      case 6:
        s.callback++;
        break;
      case 7:
        s.writer_token++;
        break;
      case 8:
        s.vm_depth = 1;
        break;
      case 9:
        s.debit_depth = 0;
        break;
      case 10:
        item.symbol += 4;
        break;
      case 11:
        item.table += 4;
        break;
      case 12:
        item.code += 4;
        break;
      case 13:
        item.symbol_count++;
        break;
      case 14:
        item.known = false;
        break;
      case 15:
        call.parser.data_words_known = false;
        break;
      case 16:
        call.parser.data_words[7] = 1;
        break;
      case 17:
        call.parser.data_depth = 0;
        break;
      }
      check(!ubc::spirit_item_restore::plan(call, saved, item, context, s).valid,
            "ITEM target/scope/quantity mismatch rejects");
    }
  }
}
}
#ifndef UBC_SPIRIT_RESULT_TEST_EMBEDDED
int main() {
  spirit_result_test::run();
  std::printf("spirit_result_tests: %u checks, %u failures\n", spirit_result_test::checks,
              spirit_result_test::failures);
  return spirit_result_test::failures ? 1 : 0;
}
#endif
