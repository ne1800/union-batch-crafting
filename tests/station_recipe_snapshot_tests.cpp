// SPDX-License-Identifier: MIT
#include "ubc/stations/station_recipe_snapshot.hpp"
// Our shared synthetic memory image. No test seam in production,
// original script bytes or additional runtime dependency.
#define UBC_STATION_RECIPE_FIXTURE_ONLY
#include "station_recipe_runtime_tests.cpp"
#include <type_traits>

namespace allocation_probe {
bool observing = false;
std::size_t requests = 0;
}
void* operator new(std::size_t size) {
  if (allocation_probe::observing)
    ++allocation_probe::requests;
  if (auto* pointer = std::malloc(size ? size : 1))
    return pointer;
  std::abort();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

namespace {
using namespace station_test;
constexpr std::uint32_t hero = 0x2000, pack = 0x40000, slots = 0x110000;
constexpr std::uint32_t left_slot = 0x120000, right_slot = 0x120080;
constexpr std::uint32_t left_name = 0x130000, right_name = 0x130050;
constexpr std::uint32_t left_object = 0x140000, right_object = 0x150000;
constexpr std::uint32_t node(std::uint32_t i) { return 0x10000 + i * 16; }
constexpr std::uint32_t object(std::uint32_t i) { return 0x90000 + i * 0x400; }
struct Stored {
  std::int32_t symbol, amount;
};
ubc::StationRecipeKey key(c::Station kind) { return {1, 7, 0x8000, hero, 0x8800, 0x9000, kind}; }
std::array<bool, c::recipes.size()> verified_hands() {
  std::array<bool, c::recipes.size()> result{};
  result.fill(true);
  return result;
}
std::size_t index_of(c::RecipeId id) {
  for (std::size_t i = 0; i < c::recipes.size(); ++i)
    if (c::recipes[i].id == id)
      return i;
  std::abort();
}
const ubc::StationRecipeState& state(const ubc::StationRecipeCatalogueCapture& value,
                                     c::RecipeId id) {
  return value.recipes[index_of(id)];
}
const ubc::StationRecipeMaterialRow& row(const ubc::StationRecipeState& value, std::int32_t id) {
  for (std::size_t i = 0; i < value.target_count; ++i)
    if (value.rows[i].symbol == id)
      return value.rows[i];
  std::abort();
}
const char* item_name(std::int32_t id) {
  for (const auto& recipe : c::recipes)
    for (const auto& target : recipe.targets)
      if (target.symbol == id)
        return target.name;
  std::abort();
}
std::string entry(std::int32_t id, std::int32_t amount) {
  return std::string(item_name(id)) + ":" + std::to_string(amount);
}
void text_at(Image& image, std::uint32_t address, const std::string& value) {
  image.block(address, value.size() + 1);
  std::memcpy(image.at(address, value.size() + 1), value.c_str(), value.size() + 1);
}
struct CaptureFixture : Fixture {
  const c::Descriptor& descriptor;
  explicit CaptureFixture(const c::Descriptor& recipe) : Fixture(recipe), descriptor(recipe) {
    for (const auto& value : c::recipes)
      populate(value);
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
    image.put(left_slot + 0xc, 11);
    image.put(right_slot + 8, right_name);
    image.put(right_slot + 0xc, 12);
    image.block(ubc::hand_detail::item_vtable, 4);
    image.put(ubc::hand_detail::item_vtable, ubc::hand_detail::item_class_method);
    image.block(ubc::hand_detail::item_class_method, ubc::hand_detail::item_class_bytes.size());
    std::memcpy(
        image.at(ubc::hand_detail::item_class_method, ubc::hand_detail::item_class_bytes.size()),
        ubc::hand_detail::item_class_bytes.data(), ubc::hand_detail::item_class_bytes.size());
    for (const auto left : {true, false}) {
      const auto& hand = left ? recipe.hand.left : recipe.hand.right;
      const auto slot = left ? left_slot : right_slot, item = left ? left_object : right_object;
      if (hand.source == c::HandSource::empty)
        continue;
      image.block(item, 0x338);
      image.put(item, ubc::hand_detail::item_vtable);
      image.put(item + 0x32c, hand.amount);
      image.put(item + 0x330, static_cast<std::uint32_t>(hand.symbol));
      image.put(slot + 0x30, item);
      image.put(slot + 0x18, hand.source == c::HandSource::generated ? 1 : 0);
    }
    image.put(hero + 0x968, recipe.hand.anchor == c::HandSide::left ? left_object : right_object);
    stock({}, "");
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
      image.put(object(i) + 0x32c, static_cast<std::uint32_t>(listed[i].amount));
      image.put(object(i) + 0x330, static_cast<std::uint32_t>(listed[i].symbol));
    }
  }
  void plentiful() {
    std::string packed;
    for (const auto& cost : descriptor.costs) {
      if (!packed.empty())
        packed += ',';
      packed += entry(cost.symbol, static_cast<std::int32_t>(cost.units * 7));
    }
    std::vector<Stored> listed;
    for (const auto& output : descriptor.outputs)
      listed.push_back({output.symbol, 3});
    if (descriptor.station == c::Station::saw)
      listed.push_back({16609, 6});
    if (descriptor.station == c::Station::forge)
      listed.push_back({16666, 5});
    stock(listed, packed);
  }
  void change(std::uint32_t address, unsigned occurrence, std::uint32_t target) {
    image.change_address = address;
    image.change_on_read = occurrence;
    image.change_target = target;
    image.watched_reads = 0;
  }
};
ubc::StationRecipeCatalogueReader reader;
ubc::StationRecipeCatalogueCapture
capture(CaptureFixture& fixture, bool boundary = true,
        std::array<bool, c::recipes.size()> verified = verified_hands()) {
  return reader.capture(fixture.image.memory(), ubc::lhiver::game_parser,
                        key(fixture.descriptor.station), boundary, verified);
}
void try_reentry(Image& image) noexcept {
  const auto before = image.reads;
  const auto nested = reader.capture(image.memory(), ubc::lhiver::game_parser, key(c::Station::saw),
                                     true, verified_hands());
  check(nested.diagnostic.issue == ubc::StationRecipeCaptureIssue::reentry && image.reads == before,
        "same-thread capture reentry is rejected without disturbing shared workspaces");
}
}

int main() {
  static_assert(!std::is_copy_constructible<ubc::StationRecipeCatalogueReader>::value,
                "workspace must not be copied");
  for (const auto& recipe : c::recipes) {
    CaptureFixture f(recipe);
    f.plentiful();
    const auto value = capture(f);
    const auto& s = state(value, recipe.id);
    check(value.diagnostic.issue == ubc::StationRecipeCaptureIssue::none,
          "valid station captures without synthetic context claims");
    check(value.diagnostic.target_count > 6 &&
              value.diagnostic.target_count < ubc::station_recipe_target_capacity,
          "all sixteen recipe target sets deduplicate beyond the old six-item limit");
    check(s.applicable && s.evaluated && s.known && !s.contract,
          "knowledge remains visible while own bodies fail pinned profile hash");
    check(s.actor_known && s.actor_matches,
          "current HERO/SELF instances are positively bound across whole capture");
    check(s.material.inventory_known && s.material.hand_known && s.material.maximum == 7 &&
              !s.material.craftable,
          "complete listed/packed inventory and exact hands give N0 preview maximum seven");
    check(f.image.inventory_reads == 2 && f.image.hand_reads == 2 && f.image.anchor_reads == 2,
          "one inventory and enriched-hand capture each has exactly one comparison pass");
    for (const auto& cost : recipe.costs)
      check(row(s, cost.symbol).stored == static_cast<std::int64_t>(cost.units) * 7 &&
                row(s, cost.symbol).missing_for_one == 0,
            "cost rows map by symbol and retain available amounts");
    if (recipe.station == c::Station::saw)
      check(row(s, 16609).reserved_tool == 1 && row(s, 16609).expected_ready_storage == 6 &&
                row(s, 16609).expected_end_storage == 7,
            "saw stays reserved at Ready and returns only in expected normal End balance");
    if (recipe.station == c::Station::forge)
      check(row(s, 16667).stored == 0 && row(s, 16666).stored == 5 &&
                row(s, 16666).expected_end_storage == 5,
            "only right forge destroy anchor is absent; stored left tong remains legitimate");
    for (const auto& other : c::recipes)
      if (other.station != recipe.station) {
        const auto& foreign = state(value, other.id);
        check(!foreign.applicable && !foreign.evaluated && !foreign.known && !foreign.contract &&
                  !foreign.actor_known,
              "foreign station recipes are populated but never evaluated");
      }
    f.stock({}, "");
    const auto empty = capture(f);
    check(state(empty, recipe.id).material.maximum == 0 && s.material.maximum == 7,
          "returned values own quantities independently of reused workspace");
  }
  const auto& ammo = *c::find(c::RecipeId::arrows);
  {
    CaptureFixture f(ammo);
    f.plentiful();
    f.image.put(symbol(10491) + 0x18, 0);
    auto value = capture(f);
    check(!state(value, ammo.id).known && state(value, ammo.id).material.maximum == 7,
          "unlearned recipe stays hidden without falsifying material preview");
    f.image.denied = symbol(10491);
    f.image.denied_size = 0x3c;
    value = capture(f);
    check(!state(value, ammo.id).known &&
              !value.diagnostic.recipes[index_of(ammo.id)].knowledge.read_valid,
          "unreadable learned scalar is not known");
  }
  {
    CaptureFixture f(ammo);
    f.plentiful();
    f.image.put(symbol(1679) + 0x1c, 0x2500);
    const auto value = capture(f);
    check(state(value, ammo.id).actor_known && !state(value, ammo.id).actor_matches,
          "positive different SELF is readable but never authorizes HERO ammo");
  }
  {
    CaptureFixture f(ammo);
    f.plentiful();
    f.change(symbol(1679), 3, symbol(1679) + 0x1c);
    const auto value = capture(f);
    check(!value.diagnostic.actor_unchanged && !state(value, ammo.id).actor_known,
          "actor change between before and after observations invalidates the capture witness");
  }
  {
    CaptureFixture f(ammo);
    f.plentiful();
    const auto value = capture(f, false);
    check(value.diagnostic.issue == ubc::StationRecipeCaptureIssue::boundary &&
              !state(value, ammo.id).known && !state(value, ammo.id).material.inventory_known &&
              !state(value, ammo.id).actor_known,
          "without actual stable boundary neither knowledge nor material/actor grants are issued");
    const auto before = f.image.reads;
    auto bad = key(ammo.station);
    bad.hero = 0;
    check(reader.capture(f.image.memory(), ubc::lhiver::game_parser, bad, true).diagnostic.issue ==
                  ubc::StationRecipeCaptureIssue::context &&
              f.image.reads == before,
          "missing key identity is rejected before memory access");
  }
  for (const auto id :
       {c::RecipeId::leather_sheep, c::RecipeId::arrows, c::RecipeId::heat_raw_steel}) {
    const auto& recipe = *c::find(id);
    for (unsigned mode = 0; mode < 10; ++mode) {
      CaptureFixture f(recipe);
      f.plentiful();
      switch (mode) {
      case 0:
        f.image.put(right_slot + 0x18, 3);
        break;
      case 1:
        f.image.put(hero + 0x968, 0);
        break;
      case 2:
        f.image.put(hero + 0x9b4, 257);
        f.image.put(hero + 0x9b8, 257);
        break;
      case 3:
        f.image.put(slots + 4, left_slot);
        break;
      case 4:
        f.image.put(right_slot + 8, left_name);
        f.image.put(right_slot + 0xc, 11);
        break;
      case 5:
        f.image.put(left_slot + 0x30, right_object);
        break;
      case 6:
        f.image.denied = right_slot;
        f.image.denied_size = 0x34;
        break;
      case 7:
        f.image.put(right_object, 0x12345678);
        break;
      case 8:
        f.image.put(right_slot + 8, UINT32_MAX - 3);
        break;
      default:
        f.image.put(right_object + 0x32c, 2);
        break;
      }
      const auto value = capture(f);
      const auto& s = state(value, id);
      check(
          s.known && s.material.inventory_known && !s.material.hand_known && !s.material.maximum,
          "bad slot lifetime/anchor/duplicates/reads/type/range/amount blocks hand contract only");
    }
  }
  for (unsigned mode = 0; mode < 4; ++mode) {
    CaptureFixture f(ammo);
    f.plentiful();
    if (mode == 0)
      f.change(right_slot, 2, right_slot + 0x30);
    if (mode == 1)
      f.change(hero + 0x968, 2, hero + 0x968);
    if (mode == 2)
      f.change(hero + 0x9b0, 2, hero + 0x9b8);
    if (mode == 3)
      f.change(right_object + 0x32c, 2, right_object + 0x32c);
    const auto value = capture(f);
    check(!value.diagnostic.hands_unchanged &&
              value.diagnostic.hand_issue == ubc::StationHandReadIssue::changed &&
              !state(value, ammo.id).material.hand_known,
          "slot/anchor/header/item change during capture invalidates enriched hand snapshot");
  }
  {
    CaptureFixture f(ammo);
    f.plentiful();
    const auto value = capture(f, true, {});
    check(!state(value, ammo.id).material.hand_known &&
              state(value, ammo.id).material.hand_issue ==
                  ubc::StationRecipeHandIssue::unverified_contract,
          "static lifecycle evidence never replaces explicit current native hand contract");
    f.stock({{99000, 1}, {16609, 1}}, entry(ammo.costs[0].symbol, 7));
    f.image.put(node(1) + 4, right_object);
    const auto alias = capture(f);
    check(
        state(alias, ammo.id).material.hand_issue == ubc::StationRecipeHandIssue::inventory_alias,
        "complete inventory objects include nonleading hand alias even when other rows are irrelevant");
  }
  {
    const auto& forge = *c::find(c::RecipeId::heat_raw_steel);
    CaptureFixture f(forge);
    f.plentiful();
    f.stock({{16667, 1}}, entry(16610, 7));
    const auto value = capture(f);
    check(state(value, forge.id).material.hand_issue == ubc::StationRecipeHandIssue::stored_prop,
          "stored right forge destroy anchor blocks even when left tong storage is absent");
  }
  {
    const auto& leather = *c::find(c::RecipeId::leather_sheep);
    CaptureFixture f(leather);
    f.stock({{leather.costs[0].symbol, 2}}, entry(leather.costs[0].symbol, 3));
    auto value = capture(f);
    check(state(value, leather.id).material.issue ==
                  ubc::StationRecipeMaterialIssue::ambiguous_ingredient &&
              row(state(value, leather.id), leather.costs[0].symbol).stored == 5,
          "mixed ingredient stays measurable but blocks numerical maximum");
    f.stock({{leather.outputs[0].symbol, 2}},
            entry(leather.outputs[0].symbol, 3) + "," + entry(leather.costs[0].symbol, 7));
    value = capture(f);
    check(state(value, leather.id).material.maximum == 7 &&
              row(state(value, leather.id), leather.outputs[0].symbol).stored == 5,
          "mixed output is known and permitted without unsafe native ingredient counting");
    f.stock({{leather.outputs[0].symbol, INT32_MAX - 2}}, entry(leather.costs[0].symbol, 7));
    value = capture(f);
    check(state(value, leather.id).material.maximum == 2,
          "actual output headroom bounds preview instead of rejecting all amounts");
    f.stock({}, entry(leather.costs[0].symbol, 7));
    f.change(pack, 2, pack);
    value = capture(f);
    check(state(value, leather.id).known && !state(value, leather.id).material.inventory_known,
          "changing packed bytes invalidate inventory independently of learned knowledge");
    CaptureFixture empty(leather);
    value = capture(empty);
    check(state(value, leather.id).known && state(value, leather.id).material.known() &&
              state(value, leather.id).material.maximum == 0 &&
              row(state(value, leather.id), leather.costs[0].symbol).missing_for_one == 1,
          "known N0 with no ingredient remains explainable and cannot craft");
  }
  {
    CaptureFixture f(ammo);
    f.plentiful();
    f.image.on_read = &try_reentry;
    allocation_probe::requests = 0;
    allocation_probe::observing = true;
    const auto value = capture(f);
    allocation_probe::observing = false;
    check(state(value, ammo.id).material.maximum == 7 && allocation_probe::requests == 0,
          "outer capture survives rejected reentry and performs no heap allocation");
    check(capture(f).diagnostic.issue == ubc::StationRecipeCaptureIssue::none,
          "capture guard releases after return");
  }
  {
    CaptureFixture f(ammo);
    f.plentiful();
    f.image.put(right_slot + 0x30, 0);
    f.image.put(hero + 0x968, 0);
    const auto value = capture(f);
    check(
        value.diagnostic.hands_unchanged && value.hands.hands.left.state == 0 &&
            value.hands.hands.right.state == 0 && value.hands.interact_item_known &&
            !value.hands.interact_item && !state(value, ammo.id).material.hand_known,
        "normal-end empty hands and null anchor remain observable separately from active material hand grant");
  }
  {
    CaptureFixture f(ammo);
    f.plentiful();
    const auto third = right_slot + 0x80, third_name = right_name + 0x50;
    f.image.block(slots, 12);
    f.image.put(hero + 0x9b4, 3);
    f.image.put(hero + 0x9b8, 3);
    f.image.put(slots + 8, third);
    f.image.block(third, 0x34);
    text_at(f.image, third_name, "OTHER_SLOT");
    f.image.put(third + 8, third_name);
    f.image.put(third + 0xc, 10);
    f.image.put(third + 0x30, right_object);
    const auto value = capture(f);
    check(value.diagnostic.hand_issue == ubc::StationHandReadIssue::duplicate &&
              !state(value, ammo.id).material.hand_known,
          "hand object aliased in a differently named third slot is rejected");
  }
  {
    CaptureFixture f(ammo);
    f.plentiful();
    f.image.put(symbol(1679) + 0x20, metadata(0, 7, 0));
    const auto value = capture(f);
    check(
        !state(value, ammo.id).actor_known && !state(value, ammo.id).actor_matches &&
            state(value, ammo.id).known,
        "wrong SELF binding metadata cannot become an actor witness while learned state stays independent");
  }
  {
    // Check all 4096 objects through the last alias, including those unrelated
    // to the recipe. The fixture has separate address ranges of its own.
    CaptureFixture f(ammo);
    f.plentiful();
    constexpr std::uint32_t nodes = 0x68000000, objects = 0x71000000;
    f.image.put(hero + 0x718, nodes);
    for (std::uint32_t i = 0; i < ubc::max_packed_inventory_nodes; ++i) {
      const auto n = nodes + i * 16, o = objects + i * 0x400;
      f.image.block(n, 12);
      f.image.block(o, 0x338);
      f.image.put(n + 4, o);
      f.image.put(n + 8, i + 1 < ubc::max_packed_inventory_nodes ? n + 16 : 0);
      f.image.put(o + 0x32c, 1);
      f.image.put(o + 0x330, 99000);
    }
    auto value = capture(f);
    check(value.diagnostic.listed_nodes == ubc::max_packed_inventory_nodes &&
              state(value, ammo.id).material.maximum == 7,
          "complete unrelated list at exact 4096-node bound remains valid");
    const auto last = nodes + (ubc::max_packed_inventory_nodes - 1) * 16;
    f.image.put(last + 4, right_object);
    value = capture(f);
    check(state(value, ammo.id).material.hand_issue == ubc::StationRecipeHandIssue::inventory_alias,
          "last object at full list bound cannot alias the reserved saw");
    f.image.put(last + 8, last + 16);
    value = capture(f);
    check(value.diagnostic.inventory_issue == ubc::PackedInventoryIssue::list_node_limit &&
              !state(value, ammo.id).material.inventory_known,
          "one extra node fails closed before following an unbounded list");
  }
  std::printf("station_recipe_snapshot_tests: %u checks, %u failures\n", checks, failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
