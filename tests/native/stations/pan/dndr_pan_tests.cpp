// SPDX-License-Identifier: MIT
#include "ubc/stations/forge/forge_stock.hpp"
#include <cstdio>
#include <cstdlib>

namespace {
using namespace ubc::dndr::forge;
void require(bool value, int line) {
  if (!value) {
    std::fprintf(stderr, "pan check failed at line %d\n", line);
    std::abort();
  }
}
#define REQUIRE(x) require((x), __LINE__)

struct Step {
  Witness witness;
  Hand hand;
  int credit;
  const char* animation;
};
constexpr std::array<Step, 6> piece{{
    {Witness::reservation, Hand::reserved, 0, "T_PAN_STAND_2_S0"},
    {Witness::insert, Hand::right_raw, 0, "T_PAN_S0_2_S1"},
    {Witness::exchange, Hand::left_cooked, 0, "T_PAN_S1_2_S0"},
    {Witness::remove, Hand::empty, 1, "T_PAN_S1_2_S0"},
    {Witness::end, Hand::empty, 1, "unknown"},
    {Witness::neutral, Hand::empty, 1, "unknown"},
}};

Stock stock(int raw, int cooked, Hand hand = Hand::empty) { return {raw, cooked, {}, hand, true}; }
void quantities(unsigned selected, int initial) {
  Order order;
  auto current = stock(initial, 4);
  REQUIRE(order.open(current, selected, Station::pan) == (selected != 0));
  for (unsigned n = 0; n < selected; ++n) {
    REQUIRE(order.start(current));
    for (unsigned i = 0; i < piece.size(); ++i) {
      const auto& step = piece[i];
      auto sample = stock(current.input_count - 1, current.output_count + step.credit, step.hand);
      REQUIRE(order.observe(step.witness, sample, n * 10 + i, step.animation));
      REQUIRE(order.awaiting_forward() == (i == 1 && selected > 1));
      REQUIRE(order.pan_workpiece(sample) == (i == 1));
    }
    --current.input_count;
    ++current.output_count;
  }
  REQUIRE(order.completed() == selected && !order.active());
}

void interruptions() {
  for (unsigned i = 0; i < piece.size(); ++i) {
    Order order;
    REQUIRE(order.open(stock(3, 0), 3, Station::pan));
    REQUIRE(order.start(stock(3, 0)));
    for (const auto& step : piece)
      REQUIRE(order.observe(step.witness, stock(2, step.credit, step.hand),
                            step.witness == Witness::neutral ? 2 : 1, step.animation));
    REQUIRE(order.start(stock(2, 1)));
    for (unsigned j = 0; j < i; ++j) {
      const auto& step = piece[j];
      REQUIRE(
          order.observe(step.witness, stock(1, 1 + step.credit, step.hand), 3 + j, step.animation));
    }
    const auto& next = piece[i];
    const auto valid = stock(1, 1 + next.credit, next.hand);
    auto reject = [&](Order clone, Witness witness, Stock sample, const char* animation) {
      REQUIRE(!clone.observe(witness, sample, 9, animation));
      REQUIRE(clone.completed() == 1 && !clone.active() && !clone.start(stock(2, 1)));
    };
    auto stopped = order;
    stopped.stop();
    reject(stopped, next.witness, valid, next.animation);
    reject(order, piece[(i + piece.size() - 1) % piece.size()].witness, valid, next.animation);
    auto changed = valid;
    ++changed.other[0];
    reject(order, next.witness, changed, next.animation);
    changed = valid;
    ++changed.output_count;
    reject(order, next.witness, changed, next.animation);
    changed = valid;
    changed.hand = Hand::left_cold;
    reject(order, next.witness, changed, next.animation);
    if (i >= 1 && i <= 3)
      reject(order, next.witness, valid, "S_PAN_S1");
  }
}

void hands() {
  const ubc::HandObjectSnapshot empty{0, -1, -1, 0}, raw{1, 6719, 1, 100}, cooked{1, 6721, 1, 200};
  auto read = [&](auto left, auto right, auto anchor) {
    ubc::StationRecipeHands h;
    h.interact_item_known = true;
    h.hands = {left, right};
    return hand_state(h, anchor, pan);
  };
  REQUIRE(read(empty, empty, empty) == Hand::empty);
  REQUIRE(read(empty, empty, raw) == Hand::reserved);
  REQUIRE(read(empty, raw, raw) == Hand::right_raw);
  REQUIRE(read(cooked, empty, cooked) == Hand::left_cooked);
  REQUIRE(read(raw, empty, raw) == Hand::unknown);
  REQUIRE(read(empty, cooked, cooked) == Hand::unknown);
  REQUIRE(read(cooked, raw, cooked) == Hand::unknown);
  auto copy = raw;
  ++copy.object;
  REQUIRE(read(empty, copy, raw) == Hand::unknown);
  ++copy.amount;
  REQUIRE(read(empty, copy, copy) == Hand::unknown);
}

void packed() {
  static ubc::PackedInventoryWorkspace work{};
  const auto set_packed = [&](const auto& literal) {
    std::memcpy(work.packed_bytes.data(), literal, sizeof(literal));
  };
  ubc::PackedInventorySnapshotT<2> inv;
  inv.storage_totals_known = inv.snapshot_unchanged = inv.stable_boundary_verified = true;
  inv.total = {3, 1};
  inv.packed_occurrences = {1, 1};
  set_packed("ITFOMUTTONRAW:3,OTHER:2,ITFOMUTTON:1");
  const auto sample = [&](Hand hand) { return stock_from(inv, work, hand, pan); };
  auto before = sample(Hand::empty);
  REQUIRE(maximum(before) == 3);
  inv.total[0] = 2;
  inv.packed_occurrences[0] = 0;
  inv.listed_occurrences[0] = 1;
  set_packed("OTHER:2,ITFOMUTTON:1");
  Order order;
  REQUIRE(order.open(before, 2, Station::pan) && order.start(before));
  REQUIRE(order.observe(Witness::reservation, sample(Hand::reserved), 1));
  REQUIRE(order.observe(Witness::insert, sample(Hand::right_raw), 2, "T_PAN_S0_2_S1"));
  REQUIRE(order.pan_workpiece(sample(Hand::right_raw)));
  set_packed("OTHER:3,ITFOMUTTON:1");
  REQUIRE(!order.pan_workpiece(sample(Hand::right_raw)));
  REQUIRE(!order.observe(Witness::exchange, sample(Hand::left_cooked), 3, "T_PAN_S1_2_S0"));
  for (unsigned i = 0; i < 2; ++i) {
    inv.mixed_representation[i] = true;
    REQUIRE(maximum(sample(Hand::empty)) == 0);
    inv.mixed_representation[i] = false;
    inv.has_active_marker[i] = true;
    REQUIRE(maximum(sample(Hand::empty)) == 0);
    inv.has_active_marker[i] = false;
  }
}
} // namespace

int main() {
  quantities(0, 3);
  quantities(1, 3);
  quantities(1, 1);
  quantities(2, 2);
  quantities(maximum(stock(7, 0)), 7);
  REQUIRE(!maximum(stock(0, 0)) && !maximum(stock(3, INT32_MAX)));
  REQUIRE(maximum(stock(3, INT32_MAX - 1)) == 1);
  interruptions();
  hands();
  packed();
}
