// SPDX-License-Identifier: MIT
#include "ubc/stations/forge/forge_stock.hpp"
#include "ubc/ui/station_quantity.hpp"
#include <cstdio>
#include <cstdlib>

namespace {
using namespace ubc::dndr::forge;
void check(bool condition, int line) {
  if (!condition) {
    std::fprintf(stderr, "forge check failed at line %d\n", line);
    std::abort();
  }
}
#define REQUIRE(x) check((x), __LINE__)
Stock storage(int c, int h, Hand hand = Hand::empty) { return {c, h, {}, hand, true}; }
constexpr std::array<Witness, 6> events{Witness::reservation, Witness::insert, Witness::exchange,
                                        Witness::remove,      Witness::end,    Witness::neutral};
constexpr std::array<Hand, 6> hands{Hand::reserved, Hand::left_cold, Hand::left_hot,
                                    Hand::empty,    Hand::empty,     Hand::empty};
Stock observed(int c, int h, unsigned i) { return storage(c - 1, h + (i >= 3), hands.at(i)); }
void replay(unsigned chosen, int initial, int hot_stock) {
  Order order;
  REQUIRE(order.open(storage(initial, hot_stock), chosen) == (chosen > 0));
  if (!chosen) {
    REQUIRE(!order.active());
    return;
  }
  for (unsigned piece = 0; piece < chosen; ++piece) {
    const auto n = static_cast<int>(piece);
    REQUIRE(order.start(storage(initial - n, hot_stock + n)));
    for (unsigned i = 0; i < events.size(); ++i)
      REQUIRE(
          order.observe(events[i], observed(initial - n, hot_stock + n, i), 1 + 10 * piece + i));
    REQUIRE(order.completed() == piece + 1);
  }
  REQUIRE(order.phase() == Phase::complete);
  REQUIRE(!order.observe(Witness::end, storage(initial, hot_stock), 100));
}
void faults() {
  for (unsigned target = 0; target < events.size(); ++target) {
    for (unsigned issue = 0; issue < 6; ++issue) {
      Order order;
      REQUIRE(order.open(storage(3, 1), 3) && order.start(storage(3, 1)));
      for (unsigned i = 0; i < target; ++i)
        REQUIRE(order.observe(events[i], observed(3, 1, i), 1 + i));
      auto s = observed(3, 1, target);
      Witness ev = events[target];
      switch (issue) {
      case 0:
        order.stop();
        break;
      case 1:
        ev = events[(target + 5) % 6];
        break;
      case 2:
        s.other[0] = 1;
        break;
      case 3:
        s.hand = Hand::unknown;
        break;
      case 4:
        ++s.hot_count;
        break;
      default:
        s.unambiguous = false;
      }
      REQUIRE(!order.observe(ev, s, 12));
      REQUIRE(order.completed() == 0 && !order.active());
      REQUIRE(!order.observe(events[target], observed(3, 1, target), 13));
    }
  }
  Order order;
  REQUIRE(order.open(storage(3, 0), 2) && order.start(storage(3, 0)));
  for (unsigned i = 0; i < 6; ++i)
    REQUIRE(order.observe(events[i], observed(3, 0, i), 1 + i));
  REQUIRE(order.start(storage(2, 1)));
  REQUIRE(!order.observe(Witness::remove, observed(3, 0, 3), 8));
  REQUIRE(order.completed() == 1);
  REQUIRE(order.open(storage(2, 1), 2) && order.start(storage(2, 1)));
  for (unsigned i = 0; i < 5; ++i)
    REQUIRE(order.observe(events[i], observed(2, 1, i), 2));
  REQUIRE(!order.observe(Witness::neutral, observed(2, 1, 5), 2));
  REQUIRE(order.completed() == 0);
}
void packed() {
  static ubc::PackedInventoryWorkspace work{};
  ubc::PackedInventorySnapshotT<2> inv;
  inv.storage_totals_known = inv.snapshot_unchanged = inv.stable_boundary_verified = true;
  inv.total = {3, 0};
  inv.packed_occurrences[0] = 1;
  std::memcpy(work.packed_bytes.data(), "ITMISWORDRAW:3,OTHER:2", sizeof("ITMISWORDRAW:3,OTHER:2"));
  auto before = stock_from(inv, work, Hand::empty);
  REQUIRE(maximum(before) == 3);
  std::memcpy(work.packed_bytes.data(), "OTHER:2", sizeof("OTHER:2"));
  inv.total = {2, 0};
  inv.packed_occurrences[0] = 0;
  inv.listed_occurrences[0] = 1;
  const auto reserved = stock_from(inv, work, Hand::reserved);
  Order order;
  REQUIRE(order.open(before, 2) && order.start(before) &&
          order.observe(Witness::reservation, reserved, 2));
  std::memcpy(work.packed_bytes.data(), "OTHER:3", sizeof("OTHER:3"));
  REQUIRE(!order.observe(Witness::insert, stock_from(inv, work, Hand::left_cold), 3));
  inv.mixed_representation[0] = true;
  REQUIRE(maximum(stock_from(inv, work, Hand::empty)) == 0);
  inv.mixed_representation[0] = false;
  inv.packed_occurrences[0] = 2;
  REQUIRE(maximum(stock_from(inv, work, Hand::empty)) == 0);
}
void menu() {
  using A = ubc::CraftingMenuAction;
  ubc::StationQuantity picker;
  picker.open(3);
  for (unsigned i = 1; i < 20; ++i) {
    picker.observe(i, true, true);
    picker.handle(A::confirm);
  }
  REQUIRE(picker.decision() < 0 && picker.selected() == 1);
  picker.observe(20, false, true);
  picker.observe(21, false, true);
  picker.observe(22, false, true);
  picker.handle(A::right);
  REQUIRE(picker.selected() == 2);
  picker.handle(A::down);
  REQUIRE(picker.selected() == 3);
  picker.handle(A::confirm);
  picker.handle(A::confirm);
  REQUIRE(picker.decision() == 3);
  picker.open(1);
  picker.observe(1, false, true);
  picker.observe(2, false, true);
  picker.handle(A::back);
  REQUIRE(picker.decision() == 0);
  picker.open(3);
  picker.observe(1, false, false);
  REQUIRE(picker.decision() == 0);
}
void return_focus() {
  constexpr std::uint64_t start = 5000;
  ReturnFocus focus;
  focus.reset(start);
  REQUIRE(focus.observe(start, Focus::forge) == NeutralResult::waiting);
  REQUIRE(focus.observe(start + 1, Focus::forge) == NeutralResult::ready);

  const auto returns_after_missing = [&](std::uint64_t first, std::uint64_t next) {
    focus.reset(start);
    REQUIRE(focus.observe(start, Focus::missing) == NeutralResult::waiting);
    REQUIRE(focus.observe(start + first, Focus::forge) == NeutralResult::waiting);
    REQUIRE(focus.observe(start + next, Focus::forge) == NeutralResult::ready);
  };
  returns_after_missing(246, 256);
  returns_after_missing(600, 700);
  returns_after_missing(999, 1000);

  focus.reset(start);
  REQUIRE(focus.observe(start + 1000, Focus::forge) == NeutralResult::waiting);
  REQUIRE(focus.observe(start + 1001, Focus::forge) == NeutralResult::rejected);
  focus.reset(start);
  REQUIRE(focus.observe(start + 1001, Focus::forge) == NeutralResult::rejected);
  focus.reset(start);
  REQUIRE(focus.observe(start + 1000, Focus::missing) == NeutralResult::waiting);
  REQUIRE(focus.observe(start + 1001, Focus::missing) == NeutralResult::rejected);

  focus.reset(start);
  REQUIRE(focus.observe(start, Focus::other) == NeutralResult::rejected);
  for (const auto lost : {Focus::missing, Focus::other}) {
    focus.reset(start);
    REQUIRE(focus.observe(start + 246, Focus::forge) == NeutralResult::waiting);
    REQUIRE(focus.observe(start + 256, lost) == NeutralResult::rejected);
  }
}
} // namespace
int main() {
  replay(0, 3, 0);
  replay(1, 1, 1);
  replay(2, 3, 1);
  replay(maximum(storage(3, 1)), 3, 1);
  REQUIRE(maximum(storage(0, 0)) == 0 && maximum(storage(3, INT32_MAX)) == 0);
  REQUIRE(maximum(storage(3, INT32_MAX - 1)) == 1);
  replay(1, 4, INT32_MAX - 1);
  faults();
  packed();
  menu();
  return_focus();
}
