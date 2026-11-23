// SPDX-License-Identifier: MIT
#include "ubc/stations/anvil/anvil_recipe_snapshot.hpp"
#include "../../helpers/equipped_ingredient.hpp"
#define UBC_ANVIL_RUNTIME_FIXTURE_ONLY
#include "anvil_recipe_runtime_tests.cpp"
#include <type_traits>

namespace allocation_probe {
bool observing = false;
std::size_t count = 0;
}
void* operator new(std::size_t size) {
  if (allocation_probe::observing)
    ++allocation_probe::count;
  if (auto* p = std::malloc(size ? size : 1))
    return p;
  std::abort();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
using namespace anvil_test;
constexpr std::uint32_t hero = 0x2000, pack = 0x40000, slots = 0x110000, left_slot = 0x120000,
                        right_slot = 0x120080;
constexpr std::uint32_t left_name = 0x130000, right_name = 0x130050, left_object = 0x140000,
                        right_object = 0x150000;
constexpr std::uint32_t node(std::uint32_t i) { return 0x10000 + i * 16; }
constexpr std::uint32_t object(std::uint32_t i) { return 0x90000 + i * 0x400; }
struct Stored {
  std::int32_t symbol, amount;
};
ubc::AnvilRecipeKey key() { return {1, 7, 0x8000, hero, 0x8800, 0x9000, cat::Station::anvil}; }
const char* target_name(std::int32_t id) {
  for (const auto& r : cat::recipes)
    for (const auto& t : r.targets)
      if (t.symbol == id)
        return t.name;
  std::abort();
}
std::string entry(std::int32_t id, std::int32_t n) {
  return std::string(target_name(id)) + ":" + std::to_string(n);
}
void text_at(Image& image, std::uint32_t address, const std::string& text) {
  image.block(address, text.size() + 1);
  std::memcpy(image.at(address, text.size() + 1), text.c_str(), text.size() + 1);
}
struct CaptureFixture : Fixture {
  CaptureFixture() : Fixture(cat::recipes[0]) {
    for (const auto& r : cat::recipes)
      populate(r);
    image.block(hero, 0xa00);
    image.block(slots, 8);
    image.block(left_slot, 0x38);
    image.block(right_slot, 0x38);
    text_at(image, left_name, "ZS_LEFTHAND");
    text_at(image, right_name, "ZS_RIGHTHAND");
    image.put(hero + 0x9b0, slots);
    image.put(hero + 0x9b4, 2);
    image.put(hero + 0x9b8, 2);
    image.put(slots, left_slot);
    image.put(slots + 4, right_slot);
    image.put(left_slot + 8, left_name);
    image.put(left_slot + 12, 11);
    image.put(left_slot + 0x18, 1);
    image.put(left_slot + 0x30, left_object);
    image.put(right_slot + 8, right_name);
    image.put(right_slot + 12, 12);
    image.put(right_slot + 0x18, 1);
    image.put(right_slot + 0x30, right_object);
    image.block(ubc::hand_detail::item_vtable, 4);
    image.put(ubc::hand_detail::item_vtable, ubc::hand_detail::item_class_method);
    image.block(ubc::hand_detail::item_class_method, ubc::hand_detail::item_class_bytes.size());
    std::memcpy(
        image.at(ubc::hand_detail::item_class_method, ubc::hand_detail::item_class_bytes.size()),
        ubc::hand_detail::item_class_bytes.data(), ubc::hand_detail::item_class_bytes.size());
    for (const auto left : {true, false}) {
      const auto address = left ? left_object : right_object;
      image.block(address, 0x338);
      image.put(address, ubc::hand_detail::item_vtable);
      image.put(address + 0x32c, 1);
      image.put(address + 0x330, left ? 16666 : 16435);
    }
    image.put(hero + 0x968, right_object);
    stock({}, entry(16611, 7));
  }
  void stock(const std::vector<Stored>& listed, const std::string& packed) {
    image.put(hero + 0x708, hero);
    image.put(hero + 0x70c, 1);
    image.put(hero + 0x718, listed.empty() ? 0 : node(0));
    image.put(hero + 0x724, packed.empty() ? 0 : pack);
    image.put(hero + 0x728, static_cast<std::uint32_t>(packed.size()));
    text_at(image, pack, packed);
    for (std::uint32_t i = 0; i < listed.size(); ++i) {
      image.block(node(i), 12);
      image.block(object(i), 0x338);
      image.put(node(i) + 4, object(i));
      image.put(node(i) + 8, i + 1 < listed.size() ? node(i + 1) : 0);
      image.put(object(i), ubc::hand_detail::item_vtable);
      image.put(object(i) + 0x32c, static_cast<std::uint32_t>(listed[i].amount));
      image.put(object(i) + 0x330, static_cast<std::uint32_t>(listed[i].symbol));
    }
  }
};
ubc::AnvilRecipeCatalogueReader reader;
ubc::AnvilRecipeCatalogueCapture capture(CaptureFixture& f,
                                         ubc::AnvilCaptureProofs proof = {true, true}) {
  return reader.capture(f.image.memory(), ubc::lhiver::game_parser, key(), proof);
}
const ubc::StationRecipeMaterialRow& row(const ubc::AnvilRecipeState& state, std::int32_t id) {
  for (std::size_t i = 0; i < state.target_count; ++i)
    if (state.rows[i].symbol == id)
      return state.rows[i];
  std::abort();
}
void nested(Image& image) noexcept {
  const auto before = image.reads;
  const auto result = reader.capture(image.memory(), ubc::lhiver::game_parser, key(), {true, true});
  check(result.diagnostic.issue == ubc::AnvilCaptureIssue::reentry && image.reads == before,
        "reentry cannot reuse or disturb the outer workspaces");
}
struct MaterialFixture {
  std::array<ubc::RecipeInventoryItem, 4> items{};
  ubc::RecipeInventory inventory{};
  ubc::StationRecipeHands hands{};
  std::array<ubc::StationRecipeMaterialRow, 4> rows{};
  explicit MaterialFixture(const cat::Descriptor& r) {
    for (std::size_t i = 0; i < items.size(); ++i)
      items[i].symbol = r.targets[i].symbol;
    inventory.header_known = inventory.listed_known = inventory.packed_known =
        inventory.storage_totals_known = true;
    inventory.stable_boundary_verified = inventory.snapshot_unchanged = true;
    inventory.items = {items.data(), items.size()};
    hands.hands.left = {1, 16666, 1, left_object};
    hands.hands.right = {1, 16435, 1, right_object};
    hands.left_slot_known = hands.right_slot_known = hands.interact_item_known = true;
    hands.left_tmp_level = hands.right_tmp_level = 1;
    hands.interact_item = right_object;
    for (auto& item : items) {
      item.listed = item.packed = item.total = 0;
    }
  }
  void amount(std::int32_t id, std::int32_t n) {
    for (auto& item : items)
      if (item.symbol == id) {
        item.packed = item.total = n;
        item.packed_occurrences = n ? 1u : 0u;
      }
    inventory.packed_entries = 0;
    for (const auto& item : items)
      inventory.packed_entries += item.packed_occurrences;
  }
  ubc::StationRecipeMaterial evaluate(const cat::Descriptor& r, std::uint32_t n,
                                      bool proof = true) {
    return ubc::anvil_recipe_material(r, inventory, hands, n, rows.data(), rows.size(), proof);
  }
};
}
int main() {
  static_assert(!std::is_copy_constructible<ubc::AnvilRecipeCatalogueReader>::value,
                "persistent reader cannot alias copied workspaces");
  {
    CaptureFixture f;
    f.stock({}, entry(16611, 7) + "E");
    auto raw = capture(f);
    raw.recipes[0].contract = raw.diagnostic.recipes[0].profile.matches = true;
    const auto snapshot = ubc::project_capture(raw);
    check(ubc::valid_crafting_snapshot(snapshot) &&
              equipped_test::localized(snapshot.recipes[ubc::crafting_catalog::anvil_begin]),
          "the anvil recipe identifies marked hot steel independently of the held tools");
  }
  CaptureFixture baseline;
  const auto first = capture(baseline);
  check(first.diagnostic.target_count == 8,
        "one deduplicated inventory covers raw five outputs and both props");
  check(first.diagnostic.inventory_known && first.diagnostic.hands_unchanged &&
            first.diagnostic.actor_unchanged && first.diagnostic.context_unchanged,
        "all stable evidence domains remain distinct");
  check(baseline.image.inventory_reads == 2 && baseline.image.hand_reads == 2,
        "one inventory and hand capture each has exactly one comparison pass");
  for (std::size_t i = 0; i < cat::recipes.size(); ++i) {
    const auto& s = first.recipes[i];
    check(s.id == cat::recipes[i].id && s.evaluated && s.known && s.actor_known &&
              s.actor_matches && s.context_known && s.context_matches,
          "all five learned families bind values and current SELF HERO context");
    check(!s.contract && s.material.known() && s.material.maximum == 7,
          "own synthetic code is not original contract while material evidence is available");
    check(row(s, 16435).generated_hand == 1 && row(s, 16435).reserved_tool == 0 &&
              row(s, 16611).available == 7,
          "hammer and tongs are generated props and never raw material reserve");
  }
  {
    CaptureFixture f;
    f.stock({}, "");
    const auto v = capture(f);
    for (const auto& s : v.recipes)
      check(s.known && s.material.inventory_known && s.material.maximum == 0 &&
                row(s, 16611).missing_for_one == 1,
            "known missing raw remains visible for every learned recipe");
  }
  {
    CaptureFixture f;
    f.learned(cat::recipes[2], 0);
    const auto v = capture(f);
    check(!v.recipes[2].known && v.recipes[1].known,
          "only chosen learned-array element hides its recipe");
  }
  {
    CaptureFixture f;
    f.image.denied = array_at(1496) + 9 * 4;
    f.image.denied_size = 4;
    const auto v = capture(f);
    check(!v.recipes[1].known && v.recipes[0].known,
          "unreadable individual learning cell is not learned or invented zero");
  }
  {
    CaptureFixture f;
    f.image.put(symbol(12330) + 0x18, 0);
    const auto v = capture(f);
    check(v.recipes[0].known && v.recipes[0].context_known && !v.recipes[0].context_matches,
          "production phase is not knowledge");
  }
  {
    CaptureFixture f;
    f.image.put(symbol(1679) + 0x1c, 0x3000);
    const auto v = capture(f);
    check(v.recipes[0].known && v.recipes[0].actor_known && !v.recipes[0].actor_matches,
          "different SELF cannot borrow HERO resources");
  }
  {
    CaptureFixture f;
    const auto v = capture(f, {true, false});
    check(v.recipes[0].known && v.recipes[0].material.inventory_known &&
              !v.recipes[0].material.hand_known && v.recipes[0].material.maximum == 0,
          "observed hand history alone cannot substitute current station/end proof");
  }
  {
    CaptureFixture f;
    const auto v = capture(f, {false, true});
    check(v.diagnostic.issue == ubc::AnvilCaptureIssue::boundary && !v.recipes[0].known &&
              !v.recipes[0].material.inventory_known,
          "no stable boundary grants no capture proof");
  }
  {
    CaptureFixture f;
    f.stock({{16611, 2}}, entry(16611, 3));
    const auto v = capture(f);
    check(v.recipes[0].material.issue == ubc::StationRecipeMaterialIssue::ambiguous_ingredient &&
              row(v.recipes[0], 16611).stored == 5 && v.recipes[0].material.maximum == 0,
          "mixed ingredient remains measurable but blocked");
  }
  {
    CaptureFixture f;
    f.stock({{16477, 2}}, entry(16611, 7) + "," + entry(16477, 3));
    const auto v = capture(f);
    check(v.recipes[0].material.known() && row(v.recipes[0], 16477).stored == 5,
          "mixed output is allowed without native ingredient counting");
  }
  {
    CaptureFixture f;
    f.stock({}, entry(16611, 7) + "," + entry(16666, 3));
    const auto v = capture(f);
    check(v.recipes[0].material.known() && row(v.recipes[0], 16666).expected_end_storage == 3,
          "left nonanchor prop remains stored and bilanced");
  }
  for (const auto n : {0, 1}) {
    CaptureFixture f;
    f.stock({{16435, n}}, entry(16611, 7));
    const auto v = capture(f);
    check(v.recipes[0].material.hand_issue == ubc::StationRecipeHandIssue::stored_prop,
          "right destroy anchor forbids stored amount and zero object");
  }
  for (const auto bad : {0, 2, -1})
    for (const auto slot : {left_slot, right_slot}) {
      CaptureFixture f;
      f.image.put(slot + 0x18, static_cast<std::uint32_t>(bad));
      const auto v = capture(f);
      check(!v.recipes[0].material.hand_known && v.recipes[0].material.maximum == 0,
            "only both exact observed temporary levels are accepted");
    }
  {
    CaptureFixture f;
    f.image.put(hero + 0x968, left_object);
    const auto v = capture(f);
    check(v.recipes[0].material.hand_issue == ubc::StationRecipeHandIssue::anchor,
          "left anchor cannot reuse forge or archive-order assumptions");
  }
  {
    CaptureFixture f;
    f.image.put(right_slot + 0x30, left_object);
    const auto v = capture(f);
    check(!v.recipes[0].material.hand_known,
          "one hand object cannot prove two separate generated props");
  }
  {
    CaptureFixture f;
    f.image.denied = pack;
    f.image.denied_size = 4;
    const auto v = capture(f);
    check(!v.diagnostic.inventory_known && !v.recipes[0].material.inventory_known &&
              v.recipes[0].known,
          "unreadable packing is not zero and does not erase learned status");
  }
  {
    CaptureFixture f;
    f.image.change_address = hero + 0x708;
    f.image.change_target = hero + 0x724;
    f.image.change_on_read = 2;
    const auto v = capture(f);
    check(!v.diagnostic.inventory_known && v.recipes[0].material.maximum == 0,
          "packing-header transition cannot produce known totals");
  }
  {
    CaptureFixture f;
    f.stock({{999, 1}}, entry(16611, 7));
    f.image.put(node(0) + 4, left_object);
    const auto v = capture(f);
    check(v.recipes[0].material.hand_issue == ubc::StationRecipeHandIssue::inventory_alias,
          "full inventory objects catch hand alias beyond ingredient targets");
  }
  {
    CaptureFixture f;
    f.image.change_address = right_slot;
    f.image.change_target = right_slot + 0x18;
    f.image.change_on_read = 2;
    const auto v = capture(f);
    check(!v.diagnostic.hands_unchanged && !v.recipes[0].material.hand_known,
          "hand metadata mutation before compare invalidates its proof");
  }
  {
    CaptureFixture f;
    f.image.change_address = symbol(1679);
    f.image.change_target = symbol(1679) + 0x1c;
    f.image.change_on_read = 3;
    const auto v = capture(f);
    check(!v.diagnostic.actor_unchanged && !v.recipes[0].actor_matches,
          "actor transition across capture invalidates all recipes");
  }
  {
    CaptureFixture f;
    f.image.change_address = symbol(10536);
    f.image.change_target = symbol(10536) + 0x18;
    f.image.change_on_read = 3;
    const auto v = capture(f);
    check(!v.diagnostic.context_unchanged && !v.recipes[0].context_matches,
          "context transition across capture invalidates handoff evidence");
  }
  {
    CaptureFixture f;
    auto bad = key();
    bad.kind = static_cast<cat::Station>(99);
    const auto before = f.image.reads;
    const auto v = reader.capture(f.image.memory(), ubc::lhiver::game_parser, bad, {true, true});
    check(v.diagnostic.issue == ubc::AnvilCaptureIssue::context && f.image.reads == before &&
              !v.recipes[0].known,
          "foreign physical station is never evaluated");
  }
  {
    CaptureFixture f;
    f.image.on_read = &nested;
    allocation_probe::observing = true;
    const auto v = capture(f);
    allocation_probe::observing = false;
    check(v.recipes[0].material.known() && allocation_probe::count == 0,
          "outer capture uses no heap and survives rejected reentry");
  }
  {
    CaptureFixture f;
    f.stock({}, entry(16611, 1));
    const auto changed = capture(f);
    check(first.recipes[0].material.maximum == 7 && changed.recipes[0].material.maximum == 1,
          "older capture remains an independent value snapshot");
  }
  for (const auto& r : cat::recipes) {
    MaterialFixture f(r);
    f.amount(16611, 300);
    f.amount(r.outputs[0].symbol, INT32_MAX - 2);
    auto m = f.evaluate(r, 2);
    check(m.known() && m.craftable && m.maximum == 2,
          "output room reduces all-stock request to real positive maximum");
    check(f.rows[0].expected_end_storage == 298 && f.rows[1].expected_end_storage == INT32_MAX,
          "one native batch has exact end raw/output values");
    m = f.evaluate(r, 3);
    check(!m.craftable && f.rows[1].output_shortfall == 1,
          "above output capacity is blocked with measurable shortfall");
    m = f.evaluate(r, 0);
    check(m.known() && !m.craftable && m.maximum == 2 && f.rows[0].missing_for_one == 0,
          "N0 is explanation only");
    m = f.evaluate(r, UINT32_MAX);
    check(!m.craftable && m.issue == ubc::StationRecipeMaterialIssue::quantity,
          "unrepresentable amount cannot overflow previews");
    f.inventory.items.count = 3;
    check(f.evaluate(r, 1).issue == ubc::StationRecipeMaterialIssue::target_mapping,
          "missing prop/output target is not implicit zero");
    for (unsigned change = 0; change < 4; ++change) {
      MaterialFixture ambiguous(r);
      ambiguous.amount(16611, 7);
      switch (change) {
      case 0:
        ambiguous.items[0].has_active_marker = true;
        break;
      case 1:
        ambiguous.items[0].duplicate_packed_target = true;
        break;
      case 2:
        ambiguous.items[0].packed_occurrences = 2;
        ambiguous.inventory.packed_entries = 2;
        break;
      case 3:
        ambiguous.items[0].mixed_representation = true;
        break;
      }
      const auto rejected = ambiguous.evaluate(r, 1);
      check(rejected.inventory_known && !rejected.craftable && rejected.maximum == 0 &&
                ambiguous.rows[0].stored == 7,
            "ambiguous raw stays visible but cannot grant a batch");
    }
    MaterialFixture full(r);
    full.amount(16611, INT32_MAX);
    auto large = full.evaluate(r, INT32_MAX);
    check(large.craftable && large.maximum == static_cast<unsigned>(INT32_MAX) &&
              full.rows[1].produced == INT32_MAX,
          "maximum legal native count stays in signed range");
    std::swap(full.items[0], full.items[3]);
    check(full.evaluate(r, 1).craftable,
          "inventory projection matches symbols rather than accidental positions");
    check(ubc::anvil_recipe_material(r, full.inventory, full.hands, 1, full.rows.data(), 3, true)
                  .issue == ubc::StationRecipeMaterialIssue::output_buffer,
          "short caller buffer is rejected before writing");
    full.items[0].total = 1;
    check(full.evaluate(r, 1).issue == ubc::StationRecipeMaterialIssue::invalid_inventory,
          "contradictory physical sum cannot form a result preview");
  }
  std::printf("anvil_recipe_snapshot_tests: %u checks, %u failures\n", station_test::checks,
              station_test::failures);
  return station_test::failures ? 1 : 0;
}
