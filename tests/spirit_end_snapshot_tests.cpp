// SPDX-License-Identifier: MIT
#define UBC_SPIRIT_RESULT_TEST_EMBEDDED
#include "spirit_result_tests.cpp"
#include "ubc/stations/spirit/spirit_end_snapshot.hpp"

namespace spirit_end_test {
using namespace spirit_result_test;
struct EndFixture {
  Fixture source{};
  ubc::SpiritEndFrame frame{};
  std::array<ubc::RecipeInventoryItem, 7> items{};
  EndFixture() {
    const auto after = source.end();
    frame.before = frame.after = after.context;
    frame.serial = after.serial;
    frame.current_thread = 7;
    frame.boundary_verified = true;
    frame.production_before = frame.production_after = frame.invincible_before =
        frame.invincible_after = 0;
    frame.hands_before.hands = {{0, -1, -1, 0}, {0, -1, -1, 0}};
    frame.hands_before.interact_item_known = true;
    frame.hands_after = frame.hands_before;
    auto& branch = frame.branch_before;
    branch.read_valid = true;
    branch.value = source.job.branch_value;
    branch.branch = c::branch_for(true, branch.value);
    branch.readings[0].value = branch.value;
    frame.branch_after = branch;
    auto& inv = frame.inventory;
    inv.header_known = inv.listed_known = inv.packed_known = inv.storage_totals_known =
        inv.stable_boundary_verified = inv.snapshot_unchanged = true;
    for (std::size_t i = 0; i < 7; ++i) {
      const auto value = static_cast<std::int32_t>(after.stocks.rows[i].stored);
      items[i] = {c::targets[i].symbol, 0, value, value, 0, value ? 1u : 0u, false, false, false};
      inv.packed_entries += value ? 1u : 0u;
    }
    inv.items = {items.data(), items.size()};
  }
};
void run() {
  EndFixture f;
  const auto captured = ubc::spirit_end_snapshot(f.source.job, f.frame);
  check(captured.valid && captured.observation.stocks.count == 7 &&
            captured.observation.branch_value == 1,
        "real seven-row end projection");
  check(!captured.observation.parser.known && !captured.observation.flask.known &&
            !captured.observation.scope.writer_active,
        "end projection invents no writer parser/debit evidence");
  ubc::SpiritResult result;
  check(through_debit(result, f.source), "end projection result armed");
  check(result.observe_completion(captured.observation, f.source.witness(), false) ==
            ubc::SpiritResultStage::closing,
        "projection is not a normal end witness");
  check(result.observe_completion(captured.observation, f.source.witness(), true) ==
            ubc::SpiritResultStage::verified,
        "separate witness plus exact projection completes");
  for (unsigned mode = 0; mode < 19; ++mode) {
    EndFixture x;
    switch (mode) {
    case 0:
      x.frame.branch_before.read_valid = false;
      break;
    case 1:
      x.frame.branch_after.value = 2;
      break;
    case 2:
      x.frame.branch_before.value = x.frame.branch_after.value = 2;
      break;
    case 3:
      x.frame.branch_after.readings[0].symbol.address = 1;
      break;
    case 4:
      x.frame.before.mob = 1;
      break;
    case 5:
      x.frame.after.world++;
      break;
    case 6:
      x.frame.current_thread++;
      break;
    case 7:
      x.frame.boundary_verified = false;
      break;
    case 8:
      x.frame.scope_after.debit_depth = 1;
      break;
    case 9:
      x.frame.production_after = 4;
      break;
    case 10:
      x.frame.invincible_before = 1;
      break;
    case 11:
      x.frame.hands_after.interact_item = 1;
      break;
    case 12:
      x.frame.hands_before.hands.right = {1, 16604, 1, 0x6000};
      break;
    case 13:
      x.frame.inventory.items.count = 6;
      break;
    case 14:
      x.items[6].symbol = 15739;
      break;
    case 15:
      x.items[0].total++;
      break;
    case 16:
      x.frame.inventory.snapshot_unchanged = false;
      break;
    case 17:
      x.source.job.callback = 20373;
      break;
    case 18:
      x.frame.hands_after.interact_item_known = false;
      break;
    }
    check(!ubc::spirit_end_snapshot(x.source.job, x.frame).valid,
          "end proof cannot use stale branch/context/slots/targets");
  }
  {
    EndFixture x;
    x.items[5].packed++;
    x.items[5].total++;
    const auto v = ubc::spirit_end_snapshot(x.source.job, x.frame);
    ubc::SpiritResult r;
    check(v.valid && through_debit(r, x.source), "observed contradiction remains representable");
    check(r.observe_completion(v.observation, x.source.witness(), true) ==
              ubc::SpiritResultStage::blocked,
          "untouched output contradiction blocks result");
  }
}
}
int main() {
  spirit_end_test::run();
  std::printf("spirit_end_snapshot_tests: %u checks, %u failures\n", spirit_result_test::checks,
              spirit_result_test::failures);
  return spirit_result_test::failures ? 1 : 0;
}
