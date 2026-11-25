// SPDX-License-Identifier: MIT
#include "ubc/stations/forge/forge_stock.hpp"
#include "ubc/ui/station_quantity.hpp"
#include <cstdio>
#include <cstdlib>

namespace {
using namespace ubc::dndr::forge;
void require(bool ok, int line) {
  if (!ok) {
    std::fprintf(stderr, "stove check failed at line %d\n", line);
    std::abort();
  }
}
#define REQUIRE(x) require((x), __LINE__)

constexpr char mounting[] = "T_STOVE_STAND_2_S0", rising[] = "T_STOVE_S0_2_STAND";
constexpr ubc::HandObjectSnapshot empty{0, -1, -1, 0}, raw{1, 6719, 1, 100},
    pan_full{1, 6910, 1, 200}, cooked{1, 6721, 1, 300};
ubc::StationRecipeHands slots(ubc::HandObjectSnapshot left, ubc::HandObjectSnapshot right,
                              ubc::HandObjectSnapshot anchor, int temporary) {
  return {{left, right}, true, true, 0, temporary, true, anchor.object};
}
void identities() {
  auto h = slots(empty, pan_full, raw, 1);
  REQUIRE(hand_state(h, raw, stove) == Hand::right_pan);
  h.right_tmp_level = 0;
  REQUIRE(hand_state(h, raw, stove) == Hand::unknown);
  h.right_tmp_level = 2;
  REQUIRE(hand_state(h, raw, stove) == Hand::unknown);
  h = slots(empty, pan_full, raw, 1);
  h.hands.right.object = raw.object;
  REQUIRE(hand_state(h, raw, stove) == Hand::unknown);
  h = slots(empty, raw, raw, 1);
  REQUIRE(hand_state(h, raw, stove) == Hand::unknown);
  h = slots(raw, empty, raw, 0);
  REQUIRE(hand_state(h, raw, stove) == Hand::unknown);
  h = slots(cooked, empty, cooked, 0);
  REQUIRE(hand_state(h, cooked, stove) == Hand::left_cooked);
  h.hands.left.object++;
  REQUIRE(hand_state(h, cooked, stove) == Hand::unknown);
  h = slots(empty, empty, raw, 0);
  REQUIRE(hand_state(h, raw, stove) == Hand::reserved);
  h.left_slot_known = false;
  REQUIRE(hand_state(h, raw, stove) == Hand::unknown);
  h = slots(empty, empty, empty, 0);
  REQUIRE(hand_state(h, empty, stove) == Hand::empty);
}

// Use the production reader for both packed and materialized stacks.
constexpr std::uint32_t hero = 0x1000, header = hero + 0x708, text = 0x2000, node = 0x3000,
                        item = 0x4000;
std::array<std::uint8_t, 0x5000> bytes{};
ubc::PackedInventoryWorkspace workspace;
bool read(void*, std::uint32_t address, void* out, std::size_t length) noexcept {
  if (address >= bytes.size() || length > bytes.size() - address)
    return false;
  std::memcpy(out, bytes.data() + address, length);
  return true;
}
void put(std::uint32_t address, std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i)
    bytes[address + i] = static_cast<std::uint8_t>(value >> (8 * i));
}
struct Listed {
  std::uint32_t symbol = 0, amount = 0;
};
Stock packed(const char* entries, Hand hand = Hand::empty, Listed listed = {},
             std::uint32_t anchor = 0, std::uint32_t prop = 0) {
  bytes.fill(0);
  const auto length = std::strlen(entries);
  put(header, hero);
  put(header + 4, 1);
  put(header + 20, 0x82e6f0);
  put(header + 28, text);
  put(header + 32, static_cast<unsigned>(length));
  std::memcpy(bytes.data() + text, entries, length + 1);
  if (listed.symbol) {
    put(header + 16, node);
    put(node + 4, item);
    put(item + 0x32c, listed.amount);
    put(item + 0x330, listed.symbol);
  }
  const auto inv = ubc::read_packed_inventory({nullptr, read}, hero, stove_items, true, workspace);
  return stock_from(inv, workspace, hand, stove, anchor, prop);
}
void packed_order() {
  auto before = packed("ITFOMUTTONRAW:2,ITFOMUTTON:3");
  Order order;
  REQUIRE(maximum(before, Station::stove) == 2 && order.open(before, 2, Station::stove));
  const char* stages[] = {"ITFOMUTTON:3", "ITFOMUTTON:4", "ITFOMUTTON:5"};
  for (unsigned i = 0; i < 2; ++i) {
    const auto remaining = i == 0 ? Listed{6719, 1} : Listed{};
    REQUIRE(order.start(before));
    auto reserved = packed(stages[i], Hand::reserved, remaining, raw.object);
    REQUIRE(order.observe(Witness::reservation, reserved, 10 * i));
    REQUIRE(order.stove_before(Witness::create, reserved));
    auto created = packed(stages[i], Hand::right_pan, remaining, raw.object, pan_full.object);
    REQUIRE(order.observe(Witness::create, created, 10 * i + 1, mounting));
    REQUIRE(order.awaiting_forward() && order.stove_before(Witness::exchange, created));
    auto exchanged = packed(stages[i], Hand::left_cooked, remaining, cooked.object);
    REQUIRE(order.observe(Witness::exchange, exchanged, 10 * i + 2, rising));
    REQUIRE(order.stove_before(Witness::remove, exchanged));
    before = packed(stages[i + 1], Hand::empty, remaining);
    REQUIRE(order.observe(Witness::remove, before, 10 * i + 3, rising));
    REQUIRE(order.observe(Witness::end, before, 10 * i + 4));
    REQUIRE(order.observe(Witness::neutral, before, 10 * i + 5));
  }
  REQUIRE(order.completed() == 2 && !order.active());
  REQUIRE(before.input_count == 0 && before.output_count == 5 && before.stove_pan_absent());
  REQUIRE(!packed("ITFOMUTTONRAW:1", Hand::empty, {6719, 1}).unambiguous);
  REQUIRE(!packed("ITFOMUTTONRAW:1,ITFOMUTTONRAW:1").unambiguous);
  REQUIRE(!packed("ITFOMUTTONRAW:1E").unambiguous);
  REQUIRE(!packed("", Hand::right_pan, {6719, 1}, item, pan_full.object).unambiguous);
  REQUIRE(!packed("", Hand::right_pan, {6719, 1}, raw.object, item).unambiguous);
}

void stored_pans_block_orders() {
  const auto listed_zero = packed("ITFOMUTTONRAW:2", Hand::empty, {6910, 0});
  for (const auto& stock : {packed("ITFOMUTTONRAW:2,ITMI_PANFULL:4,ITFOMUTTON:3"),
                            packed("ITFOMUTTONRAW:2", Hand::empty, {6910, 4}), listed_zero,
                            packed("ITFOMUTTONRAW:2,ITMI_PANFULL:1", Hand::empty, {6910, 0}),
                            packed("ITFOMUTTONRAW:2,ITMI_PANFULL:1,ITMI_PANFULL:1"),
                            packed("ITFOMUTTONRAW:2,ITMI_PANFULL:1E")}) {
    REQUIRE(material_maximum(stock) == 2 && maximum(stock, Station::stove) == 0);
    Order order;
    REQUIRE(!order.open(stock, 2, Station::stove));
    REQUIRE(!order.open(stock, 1, Station::stove));
  }
  Order order;
  const auto invalid = packed("ITFOMUTTONRAW:2,ITMI_PANFULL:0");
  REQUIRE(!invalid.unambiguous && !order.open(invalid, 2, Station::stove));
  auto clear = packed("ITFOMUTTONRAW:2");
  auto unknown = clear;
  unknown.stored_pans = -1;
  REQUIRE(!order.open(unknown, 2, Station::stove));
  REQUIRE(order.open(clear, 2, Station::stove) && !order.start(listed_zero));

  using Action = ubc::CraftingMenuAction;
  ubc::StationQuantity menu;
  for (const auto action : {Action::confirm, Action::back}) {
    menu.open(maximum(listed_zero, Station::stove));
    REQUIRE(menu.original_only());
    menu.observe(1, false, true);
    menu.observe(2, false, true);
    menu.handle(Action::right);
    menu.handle(Action::down);
    REQUIRE(menu.row() == 0 && menu.selected() == 1);
    menu.handle(action);
    REQUIRE(menu.decision() == (action == Action::confirm ? 1 : 0));
  }
}

Stock sample(int input, int output, Hand hand) {
  const auto anchor = hand == Hand::left_cooked ? cooked.object
                      : hand == Hand::empty     ? 0u
                                                : raw.object;
  return {input, output, {}, hand, true, anchor, hand == Hand::right_pan ? pan_full.object : 0u, 0};
}
struct Step {
  Witness event;
  Hand hand;
  int credit;
  const char* animation;
};
constexpr std::array<Step, 6> piece{{{Witness::reservation, Hand::reserved, 0, mounting},
                                     {Witness::create, Hand::right_pan, 0, mounting},
                                     {Witness::exchange, Hand::left_cooked, 0, rising},
                                     {Witness::remove, Hand::empty, 1, rising},
                                     {Witness::end, Hand::empty, 1, ""},
                                     {Witness::neutral, Hand::empty, 1, ""}}};
void orders(unsigned selected, int initial) {
  Order order;
  REQUIRE(order.open(sample(initial, 3, Hand::empty), selected, Station::stove) == (selected != 0));
  for (unsigned i = 0; i < selected; ++i) {
    const int input = initial - static_cast<int>(i), output = 3 + static_cast<int>(i);
    REQUIRE(order.start(sample(input, output, Hand::empty)));
    unsigned frame = 10 * i;
    for (const auto& step : piece) {
      const auto current = sample(input - 1, output + step.credit, step.hand);
      auto check_stop = [&](Order copy) {
        REQUIRE(copy.completed() == i && !copy.active() && !copy.start(current));
      };
      auto stopped = order;
      stopped.stop();
      REQUIRE(!stopped.observe(step.event, current, frame, step.animation));
      check_stop(stopped);
      auto reject = [&](Witness event, const Stock& stock, const char* animation) {
        auto copy = order;
        REQUIRE(!copy.observe(event, stock, frame, animation));
        check_stop(copy);
      };
      auto wrong = current;
      ++wrong.stored_pans;
      reject(step.event, wrong, step.animation);
      wrong = current;
      wrong.stored_pan_entry = true;
      reject(step.event, wrong, step.animation);
      wrong = current;
      ++wrong.other[0];
      reject(step.event, wrong, step.animation);
      reject(Witness::insert, current, step.animation);
      if (step.event == Witness::create || step.event == Witness::exchange ||
          step.event == Witness::remove) {
        reject(step.event, current, "S_STOVE_S1");
        reject(step.event, sample(input - 1, output, Hand::empty), step.animation);
      }
      if (step.event == Witness::create) {
        wrong = current;
        ++wrong.anchor;
        reject(step.event, wrong, step.animation);
      }
      REQUIRE(order.observe(step.event, current, frame++, step.animation));
      REQUIRE(order.awaiting_forward() == (selected > 1 && step.event == Witness::create));
      if (step.event == Witness::create) {
        wrong = current;
        ++wrong.prop;
        REQUIRE(!order.stove_before(Witness::exchange, wrong));
        const auto destroy = material_message("DEF_DESTROY_ITEM", Station::stove);
        REQUIRE(destroy == Witness::none && !order.stove_before(destroy, current));
        auto interrupted = order;
        REQUIRE(!interrupted.observe(destroy, current, frame, rising));
        REQUIRE(interrupted.phase() == Phase::stopped);
        // Even an apparent return after the cleanup cannot credit or restart the order.
        const auto returned = sample(input - 1, output + 1, Hand::empty);
        REQUIRE(!interrupted.observe(Witness::remove, returned, frame + 1, rising));
        REQUIRE(!interrupted.observe(Witness::neutral, returned, frame + 2));
        REQUIRE(!interrupted.start(returned) && interrupted.completed() == i);
      }
    }
  }
  REQUIRE(order.completed() == selected && !order.active());
}
} // namespace

int main() {
  identities();
  packed_order();
  stored_pans_block_orders();
  orders(0, 0);
  orders(0, 3);
  orders(1, 1);
  orders(1, 3);
  orders(2, 2);
  orders(maximum(sample(10, 3, Hand::empty)), 10);
  REQUIRE(!maximum(sample(0, 0, Hand::empty)));
  REQUIRE(!maximum(sample(3, INT32_MAX, Hand::empty)));
  REQUIRE(maximum(sample(3, INT32_MAX - 1, Hand::empty)) == 1);
}
