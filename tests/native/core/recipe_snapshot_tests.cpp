// SPDX-License-Identifier: MIT
#include "ubc/core/recipe_snapshot.hpp"
#include "../helpers/equipped_ingredient.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <new>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

namespace allocation_probe {
bool observing = false;
std::size_t requests = 0;
}
void* operator new(std::size_t size) {
  if (allocation_probe::observing)
    ++allocation_probe::requests;
  if (auto* result = std::malloc(size ? size : 1))
    return result;
  std::abort();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

namespace {
namespace c = ubc::recipe_catalog;
namespace rr = ubc::recipe_runtime;
namespace rs = ubc::runtime_script;
unsigned checks = 0, failures = 0;
void check(bool value, const char* name) {
  ++checks;
  if (!value) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}
constexpr std::uint32_t hero = 0x2000, inventory_header = hero + 0x708;
constexpr std::uint32_t pack = 0x40000, slot_array = 0x110000;
constexpr std::uint32_t right_slot = 0x120000, left_slot = 0x120080;
constexpr std::uint32_t right_name = 0x130000, left_name = 0x130050;
constexpr std::uint32_t right_object = 0x140000;
constexpr std::uint32_t table = 0x10000000, code = 0x20000000;
constexpr std::uint32_t node(std::uint32_t i) { return 0x10000 + i * 0x10; }
constexpr std::uint32_t object(std::uint32_t i) { return 0x90000 + i * 0x400; }
constexpr std::uint32_t symbol(std::uint32_t i) { return 0x30000000 + i * 0x80; }
constexpr std::uint32_t name_at(std::uint32_t i) { return 0x40000000 + i * 0x80; }
constexpr std::uint32_t array_at(std::uint32_t i) { return 0x50000000 + i * 0x100; }
std::uint32_t metadata(std::uint32_t count, std::uint32_t type, std::uint32_t flags) {
  return count | (type << 12u) | (flags << 16u);
}
ubc::StationKey key(c::Station station = c::Station::stove) {
  return {1, 7, 0x8000, hero, 0x8800, 0x9000, station};
}
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
const ubc::RecipeState& state(const ubc::RecipeCatalogueCapture& value, c::RecipeId id) {
  return value.snapshot.recipes[index_of(id)];
}
const ubc::RecipeMaterialValue& row(const ubc::RecipeState& value, std::int32_t item) {
  const auto* result = ubc::recipe_session_detail::material(value, item);
  if (!result)
    std::abort();
  return *result;
}
const char* item_name(std::int32_t item) {
  for (const auto& recipe : c::recipes)
    for (const auto& target : recipe.targets)
      if (target.symbol == item)
        return target.name;
  std::abort();
}
std::string entry(std::int32_t item, std::int32_t amount) {
  return std::string(item_name(item)) + ":" + std::to_string(amount);
}

// The production reader does not allocate. read() also uses only previously
// allocated ranges. Fixture setup deliberately uses ordinary test infrastructure.
struct Image {
  std::map<std::uint32_t, std::vector<std::uint8_t>> blocks;
  std::uint32_t denied = 0, change_on_address = 0, change_target = 0, change_value = 0;
  std::size_t denied_size = 0, reads = 0, parser_reads = 0, code_reads = 0;
  unsigned inventory_reads = 0, hand_reads = 0, learning_reads = 0, scalar_learning_reads = 0;
  unsigned change_occurrence = 0, watched_reads = 0;
  void (*on_read)(Image&) noexcept = nullptr;
  void block(std::uint32_t address, std::size_t size) { blocks[address].resize(size); }
  std::uint8_t* at(std::uint32_t address, std::size_t size) {
    auto found = blocks.upper_bound(address);
    if (found == blocks.begin())
      return nullptr;
    --found;
    const auto offset = static_cast<std::size_t>(address - found->first);
    if (offset > found->second.size() || size > found->second.size() - offset)
      return nullptr;
    return found->second.data() + offset;
  }
  void put(std::uint32_t address, std::uint32_t value) {
    auto* bytes = at(address, 4);
    if (!bytes)
      std::abort();
    for (unsigned i = 0; i < 4; ++i)
      bytes[i] = static_cast<std::uint8_t>(value >> (8u * i));
  }
  void text(std::uint32_t address, const std::string& value) {
    block(address, value.size() + 1);
    std::memcpy(at(address, value.size() + 1), value.c_str(), value.size() + 1);
  }
  void mutate(std::uint32_t trigger, unsigned occurrence, std::uint32_t target,
              std::uint32_t value) {
    change_on_address = trigger;
    change_occurrence = occurrence;
    change_target = target;
    change_value = value;
    watched_reads = 0;
  }
  void clear_counts() {
    reads = parser_reads = code_reads = 0;
    inventory_reads = hand_reads = learning_reads = scalar_learning_reads = 0;
  }
  static bool read(void* context, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& self = *static_cast<Image*>(context);
    ++self.reads;
    if (address == inventory_header && size == 40)
      ++self.inventory_reads;
    if (address == hero + 0x9b0 && size == 12)
      ++self.hand_reads;
    if (address == ubc::lhiver::game_parser && size == 0x58)
      ++self.parser_reads;
    if (address >= code && address < code + ubc::lhiver::code_length)
      ++self.code_reads;
    if (address >= array_at(1464) && address < array_at(1464) + 15 * 4)
      ++self.learning_reads;
    if (address == symbol(10535))
      ++self.scalar_learning_reads;
    if (self.on_read) {
      const auto callback = self.on_read;
      self.on_read = nullptr;
      callback(self);
    }
    if (address == self.change_on_address && ++self.watched_reads == self.change_occurrence)
      self.put(self.change_target, self.change_value);
    if (self.denied_size && std::uint64_t{address} + size > self.denied &&
        std::uint64_t{self.denied} + self.denied_size > address)
      return false;
    const auto* bytes = self.at(address, size);
    if (!bytes)
      return false;
    std::memcpy(output, bytes, size);
    return true;
  }
  ubc::ReadOnlyMemory memory() { return {this, &read}; }
};

struct Stored {
  std::int32_t symbol, amount;
};
struct Fixture {
  Image image;
  explicit Fixture(c::Station station = c::Station::stove) {
    image.block(ubc::lhiver::game_parser, 0x58);
    image.block(table, static_cast<std::size_t>(ubc::lhiver::symbol_count + 2) * 4);
    image.put(ubc::lhiver::game_parser + 0x18, table);
    image.put(ubc::lhiver::game_parser + 0x1c, ubc::lhiver::symbol_count + 2);
    image.put(ubc::lhiver::game_parser + 0x20, ubc::lhiver::symbol_count);
    image.put(ubc::lhiver::game_parser + 0x48, code);
    image.put(ubc::lhiver::game_parser + 0x50, code + ubc::lhiver::code_length);
    image.put(ubc::lhiver::game_parser + 0x54, ubc::lhiver::code_length + 128);
    for (const auto& descriptor : c::recipes) {
      const auto assembly = rr::assemble(descriptor);
      const auto profile = assembly.profile();
      for (std::size_t i = 0; i < profile.binding_count; ++i) {
        const auto& b = profile.bindings[i];
        make_symbol(b.symbol, b.name, b.elements, b.type, b.flags,
                    b.check_value ? static_cast<std::uint32_t>(b.value) : 0,
                    static_cast<std::uint32_t>(b.offset), b.parent_index);
        if (b.type == 2 && b.parent_index == -1 && b.elements > 1) {
          image.block(array_at(b.symbol), static_cast<std::size_t>(b.elements) * 4);
          image.put(symbol(b.symbol) + 0x18, array_at(b.symbol));
          for (std::uint32_t at = 0; at < b.elements; ++at)
            image.put(array_at(b.symbol) + at * 4, 1);
        }
      }
    }
    for (const auto& descriptor : c::recipes) {
      const auto assembly = rr::assemble(descriptor);
      const auto profile = assembly.profile();
      for (std::size_t i = 0; i < profile.function_count; ++i) {
        const auto& f = profile.functions[i];
        make_symbol(f.symbol, f.name, f.argc, f.type, f.flags, f.byte_offset, f.return_type,
                    f.parent_index);
        image.block(code + f.byte_offset, f.byte_length);
        auto* body = image.at(code + f.byte_offset, f.byte_length);
        // Our pattern MUST fail the actual catalog hashes. Separate
        // positive hash and profile tests are in recipe_runtime_tests.
        for (std::uint32_t at = 0; at < f.byte_length; ++at)
          body[at] = static_cast<std::uint8_t>((f.symbol + at * 37u) % 256u);
      }
    }
    image.put(symbol(10535) + 0x18, 1);
    image.block(hero, 0xa00);
    image.block(slot_array, 8);
    image.block(right_slot, 0x38);
    image.block(left_slot, 0x38);
    image.text(right_name, "ZS_RIGHTHAND");
    image.text(left_name, "ZS_LEFTHAND");
    image.put(hero + 0x9b0, slot_array);
    image.put(hero + 0x9b4, 2);
    image.put(hero + 0x9b8, 2);
    image.put(slot_array, left_slot);
    image.put(slot_array + 4, right_slot);
    image.put(left_slot + 8, left_name);
    image.put(left_slot + 0xc, 11);
    image.put(right_slot + 8, right_name);
    image.put(right_slot + 0xc, 12);
    image.put(right_slot + 0x30, right_object);
    image.block(right_object, 0x338);
    image.put(right_object, ubc::hand_detail::item_vtable);
    image.put(right_object + 0x32c, 1);
    image.put(right_object + 0x330, station == c::Station::stove ? 16608 : 16604);
    image.block(ubc::hand_detail::item_vtable, 4);
    image.put(ubc::hand_detail::item_vtable, ubc::hand_detail::item_class_method);
    image.block(ubc::hand_detail::item_class_method, ubc::hand_detail::item_class_bytes.size());
    std::memcpy(
        image.at(ubc::hand_detail::item_class_method, ubc::hand_detail::item_class_bytes.size()),
        ubc::hand_detail::item_class_bytes.data(), ubc::hand_detail::item_class_bytes.size());
    stock({}, "");
  }
  void make_symbol(std::uint32_t id, const char* name, std::uint32_t elements, std::uint32_t type,
                   std::uint32_t flags, std::uint32_t data, std::uint32_t offset,
                   std::int32_t parent) {
    image.block(symbol(id), 0x3c);
    image.text(name_at(id), name);
    image.put(table + id * 4, symbol(id));
    image.put(symbol(id) + 8, name_at(id));
    image.put(symbol(id) + 0xc, static_cast<std::uint32_t>(std::strlen(name)));
    image.put(symbol(id) + 0x10, static_cast<std::uint32_t>(std::strlen(name) + 1));
    image.put(symbol(id) + 0x18, data);
    image.put(symbol(id) + 0x1c, offset);
    image.put(symbol(id) + 0x20, metadata(elements, type, flags));
    image.put(symbol(id) + 0x38, parent < 0 ? 0 : symbol(static_cast<std::uint32_t>(parent)));
    if (parent >= 0 && !image.at(symbol(static_cast<std::uint32_t>(parent)), 0x3c)) {
      image.block(symbol(static_cast<std::uint32_t>(parent)), 0x3c);
      image.put(table + static_cast<std::uint32_t>(parent) * 4,
                symbol(static_cast<std::uint32_t>(parent)));
    }
  }
  void stock(std::initializer_list<Stored> listed, const std::string& packed) {
    image.put(inventory_header, hero);
    image.put(inventory_header + 4, 1);
    image.put(inventory_header + 16, listed.size() ? node(0) : 0);
    image.put(inventory_header + 28, packed.empty() ? 0 : pack);
    image.put(inventory_header + 32, static_cast<std::uint32_t>(packed.size()));
    image.text(pack, packed);
    std::uint32_t i = 0;
    for (const auto& item : listed) {
      image.block(node(i), 12);
      image.block(object(i), 0x338);
      image.put(node(i) + 4, object(i));
      image.put(node(i) + 8, i + 1 < listed.size() ? node(i + 1) : 0);
      image.put(object(i) + 0x32c, static_cast<std::uint32_t>(item.amount));
      image.put(object(i) + 0x330, static_cast<std::uint32_t>(item.symbol));
      ++i;
    }
  }
  void learned(unsigned element, std::int32_t value) {
    image.put(array_at(1464) + element * 4, static_cast<std::uint32_t>(value));
  }
};

// Persistent storage of our own. Never put the large reader on the callback stack.
ubc::RecipeCatalogueReader reader;
ubc::RecipeCatalogueCapture
capture(Fixture& fixture, c::Station station = c::Station::stove, bool stable = true,
        const std::array<bool, c::recipes.size()>& hands = verified_hands()) {
  return reader.capture(fixture.image.memory(), ubc::lhiver::game_parser, key(station), stable,
                        hands);
}

void baseline_and_owned_values() {
  static_assert(!std::is_copy_constructible_v<ubc::RecipeCatalogueReader>);
  static_assert(!std::is_move_constructible_v<ubc::RecipeCatalogueReader>);
  std::size_t capacity = 0;
  std::set<std::int32_t> symbols;
  for (const auto& descriptor : c::recipes) {
    capacity += descriptor.targets.count;
    for (const auto& target : descriptor.targets)
      symbols.insert(target.symbol);
  }
  check(ubc::recipe_catalogue_target_capacity == capacity && symbols.size() > 6,
        "union capacity derives from all descriptors, not the former six targets");
  Fixture f;
  f.stock({{16379, 3}, {90000, 19}}, "ITFOMUTTONRAW:7,ITFOMUTTON:2,ITMI_PAN:6");
  const auto value = capture(f);
  check(value.diagnostic.issue == ubc::RecipeCatalogueIssue::none &&
            value.diagnostic.inventory_known && value.diagnostic.target_count == symbols.size() &&
            value.diagnostic.listed_nodes == 2 && value.diagnostic.packed_entries == 3 &&
            value.diagnostic.hands_read_valid,
        "one deduplicated complete inventory and exact hands have separate diagnostics");
  check(f.image.inventory_reads == 2 && f.image.hand_reads == 1,
        "one inventory reader with its own verification and one hand reader serve all recipes");
  check(f.image.learning_reads == 0 && f.image.scalar_learning_reads > 0,
        "stove capture never reads alchemy learning values");
  check(ubc::valid_catalogue_snapshot(value.snapshot) && value.snapshot.key == key(),
        "all recipe states are initialized with valid metadata and bound context");
  const auto& meat = state(value, c::RecipeId::meat);
  check(meat.known && !meat.contract && meat.material_known && meat.hand_known && meat.maximum == 7,
        "known recipe and numeric maximum survive a deliberately rejected synthetic script body");
  check(row(meat, 16379).stored == 5 && row(meat, 16377).stored == 7 &&
            row(meat, 16607).stored == 6 && row(meat, 16608).stored == 0 &&
            row(meat, 16608).reserved == 0,
        "mixed output totals, six stored pans and generated hand prop are not conflated");
  for (std::size_t i = 0; i < c::recipes.size(); ++i) {
    const auto& descriptor = c::recipes[i];
    const auto& s = value.snapshot.recipes[i];
    const auto& diagnostic = value.diagnostic.recipes[i];
    if (descriptor.station == c::Station::stove) {
      check(diagnostic.applicable && diagnostic.evaluated &&
                diagnostic.profile.issue == rs::Issue::function_hash &&
                !diagnostic.profile.matches && diagnostic.knowledge.known &&
                diagnostic.knowledge.read_valid,
            "matching recipes expose knowledge separately from the immutable body contract");
    } else {
      check(!diagnostic.applicable && !diagnostic.evaluated && !s.known && !s.contract &&
                !s.material_known && !s.hand_known && !s.maximum,
            "foreign-station recipes are populated but not evaluated");
      for (std::size_t j = 0; j < s.target_count; ++j)
        check(s.materials[j].stored == -1 && s.materials[j].reserved == -1,
              "foreign-station numbers remain unknown, never accidental zero padding");
    }
  }
  const auto& molerat = state(value, c::RecipeId::molerat_dish);
  check(molerat.known && molerat.material_known && molerat.hand_known && !molerat.maximum &&
            ubc::recipe_amount_preview(row(molerat, 16363), 1).missing == 5,
        "learned molerat remains visible with known missing ingredient costs");
  const auto& mushroom = state(value, c::RecipeId::mushroom_dish);
  check(mushroom.known && mushroom.material_known && !mushroom.maximum &&
            ubc::recipe_amount_preview(row(mushroom, 16726), 1).missing == 50,
        "resource-gated recipe with no learning predicate is known even without fifty mushrooms");
  f.stock({}, "ITFOMUTTONRAW:2,ITMI_PAN:6");
  const auto later = capture(f);
  check(state(later, c::RecipeId::meat).maximum == 2 && meat.maximum == 7 &&
            row(meat, 16379).stored == 5,
        "returned snapshots own their numbers independently of subsequent workspace reuse");
}

std::string alchemy_stock(std::int32_t flasks = 2) {
  std::string result = entry(16604, flasks) + "," + entry(16736, 50);
  for (const auto id : {16714, 16716, 16718, 16708, 16710, 16712})
    result += "," + entry(id, 8);
  return result;
}
void alchemy_knowledge_reserve_and_limits() {
  Fixture empty(c::Station::alchemy);
  auto value = capture(empty, c::Station::alchemy);
  check(empty.image.inventory_reads == 2 && empty.image.hand_reads == 1 &&
            empty.image.scalar_learning_reads == 0,
        "six alchemy evaluations share one stock/hand capture and skip stove learning");
  for (const auto& descriptor : c::recipes)
    if (descriptor.station == c::Station::alchemy) {
      const auto& s = state(value, descriptor.id);
      check(
          s.known && s.material_known && s.hand_known && !s.maximum && row(s, 16604).stored == 0 &&
              row(s, 16604).reserved == 1 &&
              ubc::recipe_amount_preview(row(s, 16604), 1).available == 1 &&
              ubc::recipe_amount_preview(row(s, descriptor.costs[0].symbol), 1).missing == 1,
          "one real reserved flask and known missing herbs are displayed without requiring stock");
    }
  Fixture f(c::Station::alchemy);
  f.stock({}, alchemy_stock());
  f.learned(1, 0);
  f.learned(2, 2);
  f.learned(3, -1);
  value = capture(f, c::Station::alchemy);
  for (unsigned i = 0; i < 6; ++i) {
    const auto& s = value.snapshot.recipes[i + 1];
    check(s.known == (i == 0 || i == 4 || i == 5) && s.material_known && s.hand_known &&
              s.maximum == 3 && row(s, 16604).stored == 2 && row(s, 16604).reserved == 1,
          "per-recipe knowledge does not erase the independently known flask-limited maximum");
  }
  check(ubc::valid_catalogue_snapshot(value.snapshot),
        "reserved flask projections satisfy the session value contract");
  f.image.denied = array_at(1464) + 4 * 4;
  f.image.denied_size = 4;
  value = capture(f, c::Station::alchemy);
  check(!state(value, c::RecipeId::mana_02).known &&
            !value.diagnostic.recipes[index_of(c::RecipeId::mana_02)].knowledge.read_valid &&
            state(value, c::RecipeId::mana_03).known && state(value, c::RecipeId::health_01).known,
        "unreadable single learning array element hides only its recipe");
  f.image.denied_size = 0;
  auto proofs = verified_hands();
  proofs[index_of(c::RecipeId::mana_03)] = false;
  value = capture(f, c::Station::alchemy, true, proofs);
  check(
      !state(value, c::RecipeId::mana_03).hand_known &&
          !state(value, c::RecipeId::mana_03).maximum &&
          row(state(value, c::RecipeId::mana_03), 16604).reserved == -1 &&
          state(value, c::RecipeId::health_01).hand_known,
      "inferred station evidence never substitutes for the explicit per-recipe current hand proof");
  f.stock({}, alchemy_stock() + "," + entry(16775, INT32_MAX - 2));
  value = capture(f, c::Station::alchemy);
  check(
      state(value, c::RecipeId::health_01).maximum == 2 &&
          state(value, c::RecipeId::mana_03).maximum == 3,
      "actual output headroom limits only the affected recipe instead of rejecting the entire inventory");
  f.stock({},
          entry(16604, INT32_MAX) + "," + entry(16714, INT32_MAX) + "," + entry(16736, INT32_MAX));
  value = capture(f, c::Station::alchemy);
  check(state(value, c::RecipeId::health_01).maximum == static_cast<std::uint32_t>(INT32_MAX) &&
            ubc::recipe_amount_preview(row(state(value, c::RecipeId::health_01), 16604), 1)
                    .available == std::int64_t{INT32_MAX} + 1 &&
            ubc::valid_catalogue_snapshot(value.snapshot),
        "stored int32 maximum plus one held flask stays exact in preview arithmetic");
}

void ambiguity_and_hands() {
  for (const auto station : {c::Station::stove, c::Station::alchemy}) {
    Fixture fixture(station);
    const bool stove = station == c::Station::stove;
    fixture.stock({}, stove ? "ITFOMUTTONRAW:7E,ITMI_PAN:6" : entry(16714, 8) + "E");
    auto raw = capture(fixture, station);
    const auto index = index_of(stove ? c::RecipeId::meat : c::RecipeId::health_01);
    raw.snapshot.recipes[index].contract = raw.diagnostic.recipes[index].profile.matches = true;
    const auto snapshot = ubc::project_capture(raw);
    check(ubc::valid_crafting_snapshot(snapshot) &&
              equipped_test::localized(snapshot.recipes[index]),
          "the unique packed E marker precedes missing ingredients at the stove and alchemy table");
  }
  Fixture f(c::Station::alchemy);
  f.stock({{16714, 1}}, alchemy_stock());
  auto value = capture(f, c::Station::alchemy);
  check(
      state(value, c::RecipeId::health_01).material_known &&
          !state(value, c::RecipeId::health_01).maximum &&
          value.diagnostic.recipes[index_of(c::RecipeId::health_01)].material_issue ==
              ubc::RecipeMaterialIssue::ambiguous_ingredient &&
          state(value, c::RecipeId::mana_03).maximum == 3,
      "mixed ingredient is blocked locally while readable totals and unrelated recipe remain available");
  for (const auto& suffix : {std::string(",") + entry(16714, 1), std::string("E")}) {
    const auto packed = suffix == "E"
                            ? entry(16714, 8) + "E," + entry(16604, 2) + "," + entry(16736, 50)
                            : alchemy_stock() + suffix;
    f.stock({}, packed);
    value = capture(f, c::Station::alchemy);
    check(state(value, c::RecipeId::health_01).material_known &&
              !state(value, c::RecipeId::health_01).maximum &&
              value.diagnostic.recipes[index_of(c::RecipeId::health_01)].material_issue ==
                  ubc::RecipeMaterialIssue::ambiguous_ingredient,
          "duplicate or E-marked ingredient cannot grant production from known numeric totals");
    check(value.diagnostic.recipes[index_of(c::RecipeId::health_01)].equipped_ingredient ==
              (suffix == "E"),
          "only the single E-marked ingredient identifies equipment");
  }
  f.stock({{16775, 1}}, alchemy_stock() + "," + entry(16775, 2));
  value = capture(f, c::Station::alchemy);
  check(state(value, c::RecipeId::health_01).maximum == 3 &&
            row(state(value, c::RecipeId::health_01), 16775).stored == 3,
        "mixed output representation remains a valid output-capacity calculation");
  // Complete list including an unrelated first item. The right-hand
  // pointer deliberately appears only in the second node.
  f.stock({{90000, 1}, {16604, 1}}, entry(16714, 8) + "," + entry(16736, 8));
  f.image.put(node(1) + 4, right_object);
  value = capture(f, c::Station::alchemy);
  check(value.diagnostic.listed_nodes == 2 && state(value, c::RecipeId::health_01).material_known &&
            !state(value, c::RecipeId::health_01).hand_known &&
            value.diagnostic.recipes[index_of(c::RecipeId::health_01)].material_issue ==
                ubc::RecipeMaterialIssue::hand_alias,
        "complete copied list objects prevent double counting a hand flask after foreign nodes");
  f.stock({}, alchemy_stock());
  f.image.put(right_object, 0x123456);
  value = capture(f, c::Station::alchemy);
  check(value.hands.right.state == 2 && value.diagnostic.hands_read_valid &&
            state(value, c::RecipeId::health_01).material_known &&
            !state(value, c::RecipeId::health_01).hand_known,
        "readable unknown object type is occupied and never treated as the reserved flask");
  f.image.denied = hero + 0x9b0;
  f.image.denied_size = 12;
  value = capture(f, c::Station::alchemy);
  check(!value.diagnostic.hands_read_valid && value.hands.right.state == -1 &&
            state(value, c::RecipeId::health_01).material_known &&
            !state(value, c::RecipeId::health_01).hand_known,
        "unreadable hands preserve stock but grant no hand or manufacture permission");
  Fixture stove;
  stove.stock({{16607, 1}}, "ITMI_PAN:5,ITFOMUTTONRAW:7");
  value = capture(stove);
  check(state(value, c::RecipeId::meat).maximum == 7 &&
            row(state(value, c::RecipeId::meat), 16607).stored == 6,
        "mixed stored PAN tools are allowed in the proven PANFULL animation contract");
  for (const auto amount : {0, 1}) {
    stove.stock({{16608, amount}}, "ITMI_PAN:6,ITFOMUTTONRAW:7");
    value = capture(stove);
    for (const auto id : {c::RecipeId::meat, c::RecipeId::molerat_dish, c::RecipeId::mushroom_dish})
      check(
          state(value, id).material_known && !state(value, id).hand_known &&
              !state(value, id).maximum &&
              value.diagnostic.recipes[index_of(id)].material_issue ==
                  ubc::RecipeMaterialIssue::hand_prop_stock,
          "stored generated stove prop, including amount zero, rejects the shared destruction contract");
  }
  stove.stock({}, "ITMI_PAN:6,ITFOMUTTONRAW:7");
  value = capture(stove, c::Station::stove, true, {});
  check(state(value, c::RecipeId::meat).known && state(value, c::RecipeId::meat).material_known &&
            !state(value, c::RecipeId::meat).hand_known && !state(value, c::RecipeId::meat).maximum,
        "matching observed hand metadata alone never grants an omitted current hand contract");
}

void read_and_context_failures() {
  Fixture f;
  f.stock({}, "ITFOMUTTONRAW:7,ITMI_PAN:6");
  auto value = capture(f, c::Station::stove, false);
  check(value.diagnostic.issue == ubc::RecipeCatalogueIssue::boundary &&
            value.diagnostic.inventory_issue == ubc::PackedInventoryIssue::boundary_unverified &&
            !value.diagnostic.inventory_known && value.diagnostic.listed_known &&
            value.diagnostic.packed_known && value.diagnostic.snapshot_unchanged &&
            f.image.parser_reads == 0 && f.image.inventory_reads == 2 && f.image.hand_reads == 1,
        "unverified boundary provides diagnostics but no runtime or learning claims");
  for (const auto& s : value.snapshot.recipes)
    check(!s.known && !s.contract && !s.material_known && !s.hand_known && !s.maximum,
          "unconfirmed capture cannot leak stale knowledge or quantities");
  for (unsigned field = 0; field < 8; ++field) {
    auto invalid = key();
    switch (field) {
    case 0:
      invalid.generation = 0;
      break;
    case 1:
      invalid.thread = 0;
      break;
    case 2:
      invalid.world = 0;
      break;
    case 3:
      invalid.hero = 0;
      break;
    case 4:
      invalid.station = 0;
      break;
    case 5:
      invalid.manager = 0;
      break;
    case 6:
      invalid.kind = static_cast<c::Station>(123);
      break;
    default:
      invalid.hero = 0;
      break;
    }
    f.image.clear_counts();
    value =
        reader.capture(f.image.memory(), ubc::lhiver::game_parser, invalid, true, verified_hands());
    check(value.diagnostic.issue == ubc::RecipeCatalogueIssue::context && f.image.reads == 0,
          "incomplete or unsupported station identity is rejected before all memory reads");
  }
  value =
      reader.capture(f.image.memory(), ubc::lhiver::game_parser + 4, key(), true, verified_hands());
  check(
      !state(value, c::RecipeId::meat).known && !state(value, c::RecipeId::meat).contract &&
          state(value, c::RecipeId::meat).material_known &&
          state(value, c::RecipeId::meat).maximum == 7 &&
          value.diagnostic.recipes[index_of(c::RecipeId::meat)].profile.issue ==
              rs::Issue::wrong_parser,
      "wrong parser does not invent knowledge or script identity and does not erase numeric stock");
  f.image.put(inventory_header, hero + 4);
  value = capture(f);
  check(
      value.diagnostic.inventory_issue == ubc::PackedInventoryIssue::wrong_owner &&
          state(value, c::RecipeId::meat).known &&
          !state(value, c::RecipeId::meat).material_known &&
          !state(value, c::RecipeId::meat).hand_known &&
          row(state(value, c::RecipeId::meat), 16377).stored == -1,
      "wrong inventory owner cannot reuse previous workspace data while learning stays independent");
  f.stock({}, "ITFOMUTTONRAW:7,ITMI_PAN:6");
  f.image.mutate(pack, 2, pack, 0);
  value = capture(f);
  check(value.diagnostic.inventory_issue == ubc::PackedInventoryIssue::changed_during_read &&
            !state(value, c::RecipeId::meat).material_known &&
            state(value, c::RecipeId::meat).known,
        "changed packed bytes invalidate the stock capture even when learning is known");
  f.image.change_on_address = 0;
  f.stock({{16377, 7}}, "ITMI_PAN:6");
  f.image.mutate(object(0) + 0x32c, 2, object(0) + 0x32c, 8);
  value = capture(f);
  check(value.diagnostic.inventory_issue == ubc::PackedInventoryIssue::changed_during_read &&
            !state(value, c::RecipeId::meat).material_known,
        "changed listed item fields are not accepted through union projection");
  f.image.change_on_address = 0;
  f.stock({}, "ITFOMUTTONRAW:0");
  value = capture(f);
  check(value.diagnostic.inventory_issue == ubc::PackedInventoryIssue::invalid_amount &&
            !state(value, c::RecipeId::meat).material_known,
        "invalid packed grammar keeps material unknown rather than known missing");
  f.stock({}, "ITMI_PAN:6");
  value = capture(f);
  check(state(value, c::RecipeId::meat).material_known &&
            state(value, c::RecipeId::meat).hand_known &&
            !state(value, c::RecipeId::meat).maximum && value.diagnostic.listed_nodes == 0,
        "fresh valid empty list recovers without stale pointers after previous failures");
}

void learning_and_profile_failures() {
  Fixture f(c::Station::alchemy);
  f.stock({}, alchemy_stock());
  f.image.put(symbol(1464) + 0x20, metadata(15, 1, 0));
  auto value = capture(f, c::Station::alchemy);
  for (std::size_t i = 1; i <= 6; ++i)
    check(!value.snapshot.recipes[i].known && !value.diagnostic.recipes[i].knowledge.read_valid &&
              value.diagnostic.recipes[i].knowledge.issue == rs::Issue::symbol_signature &&
              value.snapshot.recipes[i].material_known,
          "wrong learned-array binding metadata is not interpreted as integer knowledge");
  f.image.put(symbol(1464) + 0x20, metadata(15, 2, 0));
  f.image.mutate(array_at(1464), 2, array_at(1464), 0);
  value = capture(f, c::Station::alchemy);
  check(!state(value, c::RecipeId::health_01).known &&
            value.diagnostic.recipes[index_of(c::RecipeId::health_01)].knowledge.issue ==
                rs::Issue::changed_during_read &&
            state(value, c::RecipeId::health_02).known,
        "learning change between its two reads fails only the affected predicate");
  Fixture stove;
  stove.stock({}, "ITFOMUTTONRAW:7,ITMI_PAN:6");
  const auto assembly = rr::assemble(*c::find(c::RecipeId::meat));
  const auto first = assembly.profile().functions[0];
  stove.image.put(symbol(first.symbol) + 0x20, 0);
  value = capture(stove);
  check(!state(value, c::RecipeId::meat).contract &&
            value.diagnostic.recipes[index_of(c::RecipeId::meat)].profile.issue ==
                rs::Issue::symbol_signature &&
            value.diagnostic.recipes[index_of(c::RecipeId::meat)].profile.failed_symbol ==
                first.symbol &&
            state(value, c::RecipeId::meat).known && state(value, c::RecipeId::meat).maximum == 7,
        "wrong native function metadata is diagnosed without treating it as recipe learning");
  stove.image.put(symbol(10535) + 0x18, 0);
  value = capture(stove);
  check(!state(value, c::RecipeId::molerat_dish).known &&
            value.diagnostic.recipes[index_of(c::RecipeId::molerat_dish)].knowledge.read_valid &&
            state(value, c::RecipeId::mushroom_dish).known,
        "scalar unlearned recipe stays hidden while resource-only mushroom recipe remains known");
}

bool inner_reentry_rejected = false;
void try_reentry(Image& image) noexcept {
  const auto reads_before = image.reads;
  const auto nested =
      reader.capture(image.memory(), ubc::lhiver::game_parser, key(), true, verified_hands());
  inner_reentry_rejected = nested.diagnostic.issue == ubc::RecipeCatalogueIssue::reentry &&
                           image.reads == reads_before && !state(nested, c::RecipeId::meat).known;
}
void reentry() {
  Fixture f;
  f.stock({}, "ITFOMUTTONRAW:7,ITMI_PAN:6");
  f.image.on_read = &try_reentry;
  const auto value = capture(f);
  check(inner_reentry_rejected && state(value, c::RecipeId::meat).maximum == 7 &&
            f.image.inventory_reads == 2,
        "same-thread recursive callback is rejected before touching the active union workspace");
  check(state(capture(f), c::RecipeId::meat).maximum == 7,
        "outer reader releases its reentry guard after every capture");
}

void all_descriptor_costs() {
  for (const auto& descriptor : c::recipes) {
    Fixture f(descriptor.station);
    std::string packed;
    const auto append = [&](std::int32_t item, std::int32_t amount) {
      if (!packed.empty())
        packed += ',';
      packed += entry(item, amount);
    };
    for (const auto& cost : descriptor.costs) {
      const auto reserved =
          descriptor.hand.from_inventory && cost.symbol == descriptor.hand.active_item_symbol ? 1u
                                                                                              : 0u;
      append(cost.symbol, static_cast<std::int32_t>(cost.units * 4 - reserved));
    }
    for (const auto& output : descriptor.outputs)
      append(output.symbol, 5);
    if (!descriptor.hand.from_inventory)
      append(descriptor.hand.start_item_symbol, 6);
    f.stock({}, packed);
    const auto value = capture(f, descriptor.station);
    const auto& s = state(value, descriptor.id);
    check(
        s.known && s.material_known && s.hand_known && s.maximum == 4 && !s.contract,
        "each actual descriptor maps its own exact stock, hand reserve and unit costs to maximum four");
    for (const auto& cost : descriptor.costs) {
      const auto preview = ubc::recipe_amount_preview(row(s, cost.symbol), 4);
      check(
          preview.available_known && preview.available == std::int64_t{cost.units} * 4 &&
              preview.required == std::int64_t{cost.units} * 4 && !preview.missing &&
              !preview.remaining,
          "all nine per-symbol ingredient forecasts account for every unit and reserved flask exactly once");
    }
    for (const auto& output : descriptor.outputs) {
      const auto preview = ubc::recipe_amount_preview(row(s, output.symbol), 4);
      check(preview.produced == std::int64_t{output.units} * 4 &&
                preview.remaining == 5 + std::int64_t{output.units} * 4,
            "the output symbol belongs to its own recipe and forecasts the complete scaled output");
    }
  }
}

void complete_pointer_bound_and_allocations() {
  Fixture f(c::Station::alchemy);
  f.stock({}, entry(16714, 8) + "," + entry(16736, 8));
  constexpr std::uint32_t nodes = 0x61000000, objects = 0x62000000;
  const auto count = ubc::max_packed_inventory_nodes;
  f.image.put(inventory_header + 16, nodes);
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto n = nodes + i * 16, o = objects + i * 0x400;
    f.image.block(n, 12);
    f.image.block(o, 0x338);
    f.image.put(n + 4, i + 1 == count ? right_object : o);
    f.image.put(n + 8, i + 1 < count ? n + 16 : 0);
    f.image.put(o + 0x32c, 1);
    f.image.put(o + 0x330, 90000);
  }
  const auto alias = capture(f, c::Station::alchemy);
  check(alias.diagnostic.inventory_known && alias.diagnostic.listed_nodes == count &&
            !state(alias, c::RecipeId::health_01).hand_known &&
            alias.diagnostic.recipes[index_of(c::RecipeId::health_01)].material_issue ==
                ubc::RecipeMaterialIssue::hand_alias,
        "hand alias at the final bounded node is found after 4095 unrelated item objects");
  f.image.put(nodes + (count - 1) * 16 + 4, objects + (count - 1) * 0x400);
  std::vector<std::uint8_t> allocation_control;
  const auto before = allocation_probe::requests;
  allocation_probe::observing = true;
  allocation_control.reserve(4096);
  allocation_probe::observing = false;
  check(allocation_probe::requests > before && allocation_control.capacity() >= 4096,
        "allocation observation detects ordinary C++ allocation in the control fixture");
  const auto capture_before = allocation_probe::requests;
  allocation_probe::observing = true;
  const auto value = capture(f, c::Station::alchemy);
  allocation_probe::observing = false;
  check(
      allocation_probe::requests == capture_before && value.diagnostic.listed_nodes == count &&
          state(value, c::RecipeId::health_01).maximum == 1,
      "full bounded capture including six profile/knowledge/material evaluations requests no C++ heap storage");
  // Profile assembly in the one-time constructor also uses our own
  // fixed arrays. The large test region itself has static lifetime.
  alignas(ubc::RecipeCatalogueReader) static std::array<unsigned char,
                                                        sizeof(ubc::RecipeCatalogueReader)>
      storage{};
  const auto init_before = allocation_probe::requests;
  allocation_probe::observing = true;
  auto* separate = new (storage.data()) ubc::RecipeCatalogueReader;
  const auto result = separate->capture(f.image.memory(), ubc::lhiver::game_parser,
                                        key(c::Station::alchemy), true, verified_hands());
  separate->~RecipeCatalogueReader();
  allocation_probe::observing = false;
  check(allocation_probe::requests == init_before &&
            state(result, c::RecipeId::health_01).maximum == 1,
        "initial profile assembly and separately owned capture also allocate no C++ heap");
}
} // namespace

int main() {
  baseline_and_owned_values();
  alchemy_knowledge_reserve_and_limits();
  ambiguity_and_hands();
  read_and_context_failures();
  learning_and_profile_failures();
  reentry();
  all_descriptor_costs();
  complete_pointer_bound_and_allocations();
  std::printf("recipe_snapshot_tests: %u checks, %u failures\n", checks, failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
