// SPDX-License-Identifier: MIT
#include "ubc/packed_inventory.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace {
unsigned checks = 0, failures = 0;
void check(bool condition, const char* message) {
  ++checks;
  if (!condition) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}

constexpr std::uint32_t hero = 0x2000;
constexpr std::uint32_t header = hero + 0x708;
constexpr std::uint32_t pack = 0x40000;
constexpr std::uint32_t node(unsigned i) { return 0x10000 + i * 0x10; }
constexpr std::uint32_t item(unsigned i) { return 0x90000 + i * 0x400; }
constexpr std::array<ubc::PackedInventoryTarget, 6> targets{{
    {"ITFOMUTTONRAW", 16377}, {"ITFOMUTTON", 16379}, {"ITMI_FLASK", 16604},
    {"ITPL_HEALTH_HERB_01", 16714}, {"ITPL_TEMP_HERB", 16736}, {"ITPO_HEALTH_01", 16775}}};
// Exactly one reusable region allocated off the callback stack.
ubc::PackedInventoryWorkspace workspace;

struct MemoryImage {
  struct Region { std::uint32_t base; std::vector<std::uint8_t> bytes; };
  struct Mutation {
    std::uint32_t trigger, address, value;
    unsigned occurrence = 2, width = 4, seen = 0;
  };
  std::vector<Region> regions{{0x1000, std::vector<std::uint8_t>(0x100000)}};
  std::vector<Mutation> mutations;
  std::vector<std::uint32_t> reads;
  std::uint64_t denied_begin = 0, denied_end = 0;
  std::uint32_t fail_address = 0;
  unsigned fail_occurrence = 0, fail_seen = 0, overflow_reads = 0;
  static bool read(void* context, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& image = *static_cast<MemoryImage*>(context);
    image.reads.push_back(address);
    const auto end = std::uint64_t{address} + size;
    if (end > (std::uint64_t{1} << 32)) { ++image.overflow_reads; return false; }
    for (auto& mutation : image.mutations)
      if (address == mutation.trigger && ++mutation.seen == mutation.occurrence)
        image.put(mutation.address, mutation.value, mutation.width);
    if (address == image.fail_address && ++image.fail_seen == image.fail_occurrence) return false;
    if (image.denied_begin && address < image.denied_end && end > image.denied_begin) return false;
    for (const auto& region : image.regions) {
      if (address >= region.base && end <= std::uint64_t{region.base} + region.bytes.size()) {
        std::memcpy(output, region.bytes.data() + address - region.base, size);
        return true;
      }
    }
    return false;
  }
  ubc::ReadOnlyMemory memory() { return {this, read}; }
  void add(std::uint32_t address, std::size_t size) {
    if (std::uint64_t{address} + size > (std::uint64_t{1} << 32)) std::abort();
    regions.push_back({address, std::vector<std::uint8_t>(size)});
  }
  void put(std::uint32_t address, std::uint32_t value, unsigned width = 4) {
    for (auto& region : regions) {
      if (address >= region.base && std::uint64_t{address} + width <=
          std::uint64_t{region.base} + region.bytes.size()) {
        for (unsigned i = 0; i < width; ++i)
          region.bytes[address - region.base + i] = static_cast<std::uint8_t>(value >> (8 * i));
        return;
      }
    }
    std::abort();
  }
  void text(std::uint32_t address, const std::string& value) {
    for (std::size_t i = 0; i <= value.size(); ++i)
      put(address + static_cast<std::uint32_t>(i), i == value.size() ? 0u :
          static_cast<unsigned char>(value[i]), 1);
  }
  void deny(std::uint32_t address) { denied_begin = address; denied_end = std::uint64_t{address} + 1; }
};

void populate(MemoryImage& image, const std::string& text = "", unsigned nodes = 0) {
  image = MemoryImage{};
  if (nodes) image.regions[0].bytes.resize(std::max<std::size_t>(image.regions[0].bytes.size(),
      static_cast<std::size_t>(item(nodes - 1)) + 0x338 - 0x1000));
  image.put(header, hero);
  image.put(header + 4, 1);
  image.put(header + 16, nodes ? node(0) : 0);
  image.put(header + 20, 0x82e6f0);
  image.put(header + 28, text.empty() ? 0 : pack);
  image.put(header + 32, static_cast<std::uint32_t>(text.size()));
  image.put(header + 36, 0); // reserved does not determine the read length.
  image.text(pack, text);
  for (unsigned i = 0; i < nodes; ++i) {
    image.put(node(i) + 4, item(i));
    image.put(node(i) + 8, i + 1 < nodes ? node(i + 1) : 0);
    image.put(item(i) + 0x32c, 1);
    image.put(item(i) + 0x330, static_cast<std::uint32_t>(targets[0].symbol));
  }
}

ubc::PackedInventorySnapshot sample(MemoryImage& image, bool boundary = true) {
  return ubc::read_packed_inventory(image.memory(), hero, targets, boundary, workspace);
}

bool unknown(const std::array<std::int32_t, 6>& values) {
  return std::all_of(values.begin(), values.end(), [](auto v) { return v == -1; });
}
void changed(const ubc::PackedInventorySnapshot& value, const char* message) {
  check(!value.listed_known && !value.packed_known && !value.storage_totals_known &&
        !value.snapshot_unchanged && unknown(value.listed) && unknown(value.packed) &&
        unknown(value.total) && value.issue == ubc::PackedInventoryIssue::changed_during_read, message);
}

void ordinary_cases() {
  MemoryImage image;
  for (const auto enabled : {0u, 1u, UINT32_MAX}) {
    for (const auto pointer : {0u, pack}) {
      populate(image);
      image.put(header + 4, enabled);
      image.put(header + 28, pointer);
      const auto value = sample(image);
      check(value.listed_known && value.packed_known && value.storage_totals_known &&
            value.snapshot_unchanged && value.stable_boundary_verified && value.total[0] == 0,
            "empty pack accepts both pointer forms and any native nonzero packAbility");
    }
  }
  populate(image, "", 2);
  image.put(header + 4, 0);
  image.put(item(0) + 0x32c, 4);
  auto value = sample(image);
  check(value.storage_totals_known && value.listed[0] == 5 && value.packed[0] == 0 &&
        value.total[0] == 5 && value.listed_occurrences[0] == 2 && value.listed_nodes == 2,
        "multiple distinct materialized items sum correctly");

  populate(image, "ITFOMUTTONRAW:37,ITFOMUTTON:2,ITMI_FLASK:3E,ITPL_HEALTH_HERB_01:4,ITPL_TEMP_HERB:5,ITPO_HEALTH_01:6");
  value = sample(image);
  check(value.storage_totals_known && value.packed == std::array<std::int32_t, 6>{{37, 2, 3, 4, 5, 6}} &&
        value.total == value.packed && value.packed_entries == 6,
        "all six exact names counted without cooked/raw prefix collision");
  check(value.has_active_marker[2] && !value.has_active_marker[0] && value.any_active_marker,
        "active suffix is reported separately from numeric quantity");
  populate(image, "OTHER:1,OTHER:2E,ITFOMUTTONRAW:2,ITFOMUTTONRAW:3E");
  value = sample(image);
  check(value.storage_totals_known && value.packed[0] == 5 && value.packed_occurrences[0] == 2 &&
        value.duplicate_packed_target[0] && value.has_active_marker[0] && value.packed_entries == 4,
        "foreign duplicates legal and target duplicates summed and marked");
  populate(image, "OTHER:1E,OTHER:1");
  value = sample(image);
  check(value.storage_totals_known && value.any_active_marker && !value.has_active_marker[0] &&
        !value.duplicate_packed_target[0], "foreign active marker does not taint target flags");

  populate(image, "ITFOMUTTONRAW:37", 1);
  value = sample(image);
  check(value.storage_totals_known && value.listed[0] == 1 && value.packed[0] == 37 &&
        value.total[0] == 38 && value.mixed_representation[0],
        "stored mixed total is L+P, never the native GetAmount mixed return 2L+P");
  value = sample(image, false);
  check(value.listed_known && value.packed_known && value.snapshot_unchanged &&
        !value.stable_boundary_verified && !value.storage_totals_known && unknown(value.total) &&
        value.issue == ubc::PackedInventoryIssue::boundary_unverified,
        "identical simulated unpack-intermediate copies do not prove a stable boundary");
  image.put(item(0) + 0x32c, 0);
  value = sample(image);
  check(value.total[0] == 37 && value.mixed_representation[0] && value.listed_occurrences[0] == 1,
        "zero-count materialized object still marks mixed representation");
  image.put(header + 4, 0);
  value = sample(image);
  check(value.listed_known && value.packed_known && value.snapshot_unchanged &&
        !value.storage_totals_known && unknown(value.total) &&
        value.issue == ubc::PackedInventoryIssue::pack_disabled_nonempty,
        "disabled packing with nonempty pack string is not a normal known total");

  populate(image, "ITFOMUTTONRAW:0002");
  check(sample(image).total[0] == 2, "positive decimal quantities may have leading zeroes");
}

void syntax_and_overflow() {
  MemoryImage image;
  const std::vector<std::string> invalid{
      ",", "A:1,", ",A:1", "A:1,,B:2", ":1", "A", "A:", "A:0", "A:000", "A:-1",
      "A:+1", "A:1.0", "A:1e2", "A:1EE", "A:1E2", "A:1:E", "A:1 ", " A:1",
      "a:1", "A-b:1", "1A:1", "A: 1", std::string("\xc4") + ":1",
      "A:2147483648", "A:999999999999999999999999", "ITFOMUTTONRAW:2147483647,ITFOMUTTONRAW:1"};
  for (const auto& text : invalid) {
    populate(image, text);
    const auto value = sample(image);
    check(value.listed_known && !value.packed_known && !value.storage_totals_known &&
          unknown(value.packed) && unknown(value.total) && value.issue != ubc::PackedInventoryIssue::none,
          "whole malformed pack rejected, including invalid unrelated entries");
  }
  populate(image, "ITFOMUTTONRAW:2147483647");
  check(sample(image).total[0] == INT32_MAX, "INT32_MAX pack quantity accepted");
  populate(image, "ITFOMUTTONRAW:1", 1);
  image.put(item(0) + 0x32c, INT32_MAX);
  auto value = sample(image);
  check(value.listed_known && value.packed_known && !value.storage_totals_known && unknown(value.total) &&
        value.issue == ubc::PackedInventoryIssue::total_overflow, "combined sum overflow preserves only known components");
  populate(image, "", 2);
  image.put(item(0) + 0x32c, INT32_MAX);
  value = sample(image);
  check(!value.listed_known && value.packed_known && !value.storage_totals_known && unknown(value.listed) &&
        value.issue == ubc::PackedInventoryIssue::listed_overflow, "materialized sum overflow rejected");
}

void malformed_storage() {
  MemoryImage image;
  for (const auto amount : {UINT32_MAX, 0x80000000u}) {
    populate(image, "ITMI_FLASK:2", 1);
    image.put(item(0) + 0x32c, amount);
    const auto value = sample(image);
    check(!value.listed_known && value.packed_known && value.packed[2] == 2 && unknown(value.listed),
          "negative listed amount does not erase separately valid packed component");
  }
  populate(image, "", 2);
  image.put(node(1) + 8, node(0));
  check(!sample(image).listed_known, "cyclic materialized list rejected");
  populate(image, "", 2);
  image.put(node(1) + 4, item(0));
  check(!sample(image).listed_known, "duplicate item pointer rejected");
  populate(image, "", 1);
  image.put(node(0) + 4, 0);
  check(!sample(image).listed_known, "null materialized item rejected");
  populate(image);
  image.put(header, hero + 1);
  auto value = sample(image);
  check(!value.listed_known && !value.packed_known && !value.storage_totals_known &&
        value.issue == ubc::PackedInventoryIssue::wrong_owner, "wrong inventory owner rejected");
  for (const auto length : {UINT32_MAX, 0x80000000u, ubc::max_packed_inventory_bytes + 1}) {
    populate(image);
    image.put(header + 32, length);
    value = sample(image);
    check(value.listed_known && !value.packed_known && !value.storage_totals_known,
          "negative or oversized pack length rejected without trusting reserved");
  }
  populate(image, "A:1");
  image.put(header + 28, 0);
  check(!sample(image).packed_known, "positive length needs a pointer");
  populate(image, "A:1");
  image.put(pack + 3, 'x', 1);
  check(!sample(image).packed_known, "missing final NUL rejected");
  populate(image, "A:1,B:2");
  image.put(pack + 3, 0, 1);
  check(!sample(image).packed_known, "embedded NUL rejected");
  populate(image);
  image.put(header + 28, pack);
  image.put(pack, 'x', 1);
  check(!sample(image).packed_known, "nonnull empty string pointer must still name NUL");
  for (std::uint32_t i = 0; i < 40; ++i) {
    populate(image);
    image.deny(header + i);
    value = sample(image);
    check(!value.listed_known && !value.packed_known && !value.storage_totals_known,
          "every missing inventory-header byte rejected");
  }
  for (std::uint32_t i = 0; i < 4; ++i) {
    populate(image, "A:1");
    image.deny(pack + i);
    check(!sample(image).packed_known, "every missing pack byte including NUL rejected");
  }
  for (std::uint32_t i = 0; i < 12; ++i) {
    populate(image, "A:1", 1);
    image.deny(node(0) + i);
    const auto part = sample(image);
    check(!part.listed_known && part.packed_known, "every missing list-node byte rejected independently");
  }
  for (std::uint32_t i = 0; i < 8; ++i) {
    populate(image, "A:1", 1);
    image.deny(item(0) + 0x32c + i);
    const auto part = sample(image);
    check(!part.listed_known && part.packed_known, "every missing item-field byte rejected independently");
  }
  populate(image, "A:1");
  image.put(header + 28, 0x40000000);
  check(!sample(image).packed_known, "in-range but unmapped pack pointer rejected");
  populate(image);
  check(!ubc::read_packed_inventory(image.memory(), 0, targets, true, workspace).storage_totals_known,
        "null hero rejected");
  check(!ubc::read_packed_inventory({nullptr, nullptr}, hero, targets, true, workspace).storage_totals_known,
        "missing reader rejected");
}

void changed_copies() {
  MemoryImage image;
  for (const auto offset : {0u, 4u, 8u, 16u, 20u, 28u, 32u, 36u}) {
    populate(image, "ITFOMUTTONRAW:2", 1);
    image.mutations.push_back({header, header + offset, 123});
    changed(sample(image), "whole inventory header compared again");
  }
  populate(image, "ITFOMUTTONRAW:2,OTHER:1", 1);
  image.mutations.push_back({pack, pack + 22, '2', 2, 1});
  // Change the final digit of the unrelated entry. Leave the target total unchanged.
  image.mutations[0].address = pack + static_cast<std::uint32_t>(std::strlen("ITFOMUTTONRAW:2,OTHER:1")) - 1;
  changed(sample(image), "same header and target totals do not hide changed foreign pack bytes");
  populate(image, std::string(5000, 'A') + ":1");
  image.mutations.push_back({pack + 4096, pack + 4096, 'B', 1, 1});
  changed(sample(image), "later verification chunk compared rather than only first block");
  populate(image, std::string(8190, 'A') + ":1");
  image.mutations.push_back({pack + 8192, pack + 8192, 'X', 1, 1});
  changed(sample(image), "final single-byte NUL chunk compared too");
  populate(image, "", 2);
  image.mutations.push_back({node(0), node(0), 7});
  changed(sample(image), "full copied list node compared again");
  populate(image, "", 2);
  image.mutations.push_back({item(0) + 0x32c, item(0) + 0x32c, 0});
  image.mutations.push_back({item(0) + 0x32c, item(1) + 0x32c, 2});
  changed(sample(image), "same materialized sum does not hide changed per-item quantities");
  for (const auto address : {header, pack, node(0), item(0) + 0x32c}) {
    populate(image, "A:1", 1);
    image.fail_address = address;
    image.fail_occurrence = 2;
    const auto value = sample(image);
    check(!value.listed_known && !value.packed_known && !value.storage_totals_known &&
          !value.snapshot_unchanged && value.issue == ubc::PackedInventoryIssue::verification_unreadable,
          "failed second copy discards every component");
  }
}

void limits_and_profile() {
  MemoryImage image;
  static_assert(ubc::max_packed_inventory_bytes == 256u * 1024u, "Documented byte bound");
  static_assert(ubc::max_packed_inventory_entries == 8192, "Documented entry bound");
  static_assert(ubc::max_packed_inventory_nodes == 4096, "Documented node bound");
  populate(image, std::string(ubc::max_packed_inventory_bytes - 2, 'A') + ":1");
  check(sample(image).storage_totals_known, "exact byte bound accepted without foreign-name allocation");
  std::string many;
  for (unsigned i = 0; i < ubc::max_packed_inventory_entries; ++i) many += i ? ",A:1" : "A:1";
  populate(image, many);
  auto value = sample(image);
  check(value.storage_totals_known && value.packed_entries == ubc::max_packed_inventory_entries,
        "exact entry bound with duplicate foreign names accepted");
  populate(image, many + ",A:1");
  check(!sample(image).packed_known, "entry bound overflow rejected");
  populate(image, "", ubc::max_packed_inventory_nodes);
  value = sample(image);
  check(value.storage_totals_known && value.listed_nodes == ubc::max_packed_inventory_nodes &&
        value.total[0] == static_cast<std::int32_t>(ubc::max_packed_inventory_nodes), "exact list bound accepted");
  check(image.reads.size() < 4 * ubc::max_packed_inventory_nodes + 10, "list copying and recheck remain linearly bounded");
  populate(image, "", ubc::max_packed_inventory_nodes + 1);
  check(!sample(image).listed_known, "list bound overflow rejected");

  const std::string too_long(64, 'A');
  for (const char* name : {static_cast<const char*>(nullptr), "", "lower", "1NAME", too_long.c_str()}) {
    populate(image);
    auto profile = targets;
    profile[0].name = name;
    value = ubc::read_packed_inventory(image.memory(), hero, profile, true, workspace);
    check(value.issue == ubc::PackedInventoryIssue::invalid_targets && image.reads.empty(),
          "invalid bounded target name rejected before memory access");
  }
  for (unsigned mode = 0; mode < 3; ++mode) {
    populate(image);
    auto profile = targets;
    if (mode == 0) profile[0].symbol = -1;
    if (mode == 1) profile[1].symbol = profile[0].symbol;
    if (mode == 2) profile[1].name = profile[0].name;
    check(ubc::read_packed_inventory(image.memory(), hero, profile, true, workspace).issue ==
          ubc::PackedInventoryIssue::invalid_targets, "negative or duplicate profile identity rejected");
  }
}

void address_boundaries() {
  MemoryImage image;
  populate(image);
  constexpr auto last_hero = UINT32_MAX - 0x72f;
  image.add(last_hero + 0x708, 40);
  image.put(last_hero + 0x708, last_hero);
  check(ubc::read_packed_inventory(image.memory(), last_hero, targets, true, workspace).storage_totals_known,
        "inventory header ending at UINT32_MAX accepted");
  check(!ubc::read_packed_inventory(image.memory(), last_hero + 1, targets, true, workspace).listed_known,
        "inventory header range overflow rejected");
  check(!ubc::read_packed_inventory(image.memory(), UINT32_MAX, targets, true, workspace).listed_known,
        "hero offset addition overflow rejected");
  populate(image, "A:1");
  constexpr auto last_pack = UINT32_MAX - 3;
  image.add(last_pack, 4);
  image.text(last_pack, "A:1");
  image.put(header + 28, last_pack);
  check(sample(image).storage_totals_known, "pack bytes ending at UINT32_MAX accepted");
  image.put(header + 28, last_pack + 1);
  check(!sample(image).packed_known, "pack length including NUL overflow rejected");
  populate(image, "", 1);
  constexpr auto last_node = UINT32_MAX - 11;
  image.add(last_node, 12);
  image.put(last_node + 4, item(0));
  image.put(header + 16, last_node);
  check(sample(image).storage_totals_known, "list node ending at UINT32_MAX accepted");
  image.put(header + 16, last_node + 1);
  check(!sample(image).listed_known, "list node extent overflow rejected");
  populate(image, "", 1);
  constexpr auto last_item = UINT32_MAX - 0x333;
  image.add(last_item + 0x32c, 8);
  image.put(last_item + 0x32c, 2);
  image.put(last_item + 0x330, 16377);
  image.put(node(0) + 4, last_item);
  check(sample(image).total[0] == 2, "item fields ending at UINT32_MAX accepted");
  image.put(node(0) + 4, last_item + 1);
  check(!sample(image).listed_known, "item field range overflow rejected");
  image.put(node(0) + 4, UINT32_MAX);
  check(!sample(image).listed_known, "item offset addition overflow rejected");
  check(image.overflow_reads == 0, "out-of-range reads never reach supplied reader");
}

constexpr std::array<ubc::PackedInventoryTarget, 9> extended_targets{{
    targets[0], targets[1], targets[2], targets[3], targets[4], targets[5],
    {"IT_EXTRA_SEVEN", 20007}, {"IT_EXTRA_EIGHT", 20008}, {"IT_EXTRA_NINE", 20009}}};

template<std::size_t N>
bool all_unknown(const std::array<std::int32_t, N>& values) {
  return std::all_of(values.begin(), values.end(), [](auto v) { return v == -1; });
}

template<std::size_t N>
bool inactive_unknown(const ubc::PackedInventorySnapshotT<N>& value) {
  if (value.target_count > N) return false;
  for (auto i = value.target_count; i < N; ++i) {
    if (value.listed[i] != -1 || value.packed[i] != -1 || value.total[i] != -1 ||
        value.listed_occurrences[i] || value.packed_occurrences[i] ||
        value.duplicate_packed_target[i] || value.has_active_marker[i] ||
        value.mixed_representation[i]) return false;
  }
  return true;
}

void variable_target_counts() {
  static_assert(std::is_same<ubc::PackedInventorySnapshot,
                            ubc::PackedInventorySnapshotT<6>>::value,
                "Existing six-target snapshot name remains compatible");
  static_assert(std::is_same<decltype(sample(std::declval<MemoryImage&>())),
                            ubc::PackedInventorySnapshot>::value,
                "Existing array call retains the six-target result type");
  const ubc::PackedInventorySnapshotT<1> empty_single;
  const ubc::PackedInventorySnapshotT<9> empty_many;
  check(empty_single.target_count == 1 && empty_many.target_count == 9 &&
        all_unknown(empty_single.listed) && all_unknown(empty_single.packed) &&
        all_unknown(empty_single.total) && all_unknown(empty_many.listed) &&
        all_unknown(empty_many.packed) && all_unknown(empty_many.total),
        "default snapshots keep every capacity-specific amount unknown");

  MemoryImage image;
  constexpr std::array<ubc::PackedInventoryTarget, 1> single{{targets[0]}};
  populate(image, "ITFOMUTTONRAW:7,ITFOMUTTON:9E", 2);
  image.put(item(1) + 0x330, static_cast<std::uint32_t>(targets[1].symbol));
  const auto one = ubc::read_packed_inventory(image.memory(), hero, single, true, workspace);
  check(one.target_count == 1 && one.storage_totals_known && one.listed[0] == 1 &&
        one.packed[0] == 7 && one.total[0] == 8 && one.listed_nodes == 2 &&
        one.packed_entries == 2 && one.listed_occurrences[0] == 1 && one.mixed_representation[0],
        "single target counts exact names and symbols while retaining every foreign node");
  check(one.any_active_marker && !one.has_active_marker[0],
        "foreign active marker remains separate with a single target");
  const auto unproven = ubc::read_packed_inventory(image.memory(), hero, single, false, workspace);
  check(unproven.listed_known && unproven.packed_known && !unproven.storage_totals_known &&
        all_unknown(unproven.total) && unproven.target_count == 1 &&
        unproven.issue == ubc::PackedInventoryIssue::boundary_unverified,
        "single-target total still requires a separately verified stable boundary");
  populate(image, "ITFOMUTTONRAW:7,OTHER:0");
  const auto malformed = ubc::read_packed_inventory(image.memory(), hero, single, true, workspace);
  check(malformed.listed_known && !malformed.packed_known && !malformed.storage_totals_known &&
        all_unknown(malformed.packed) && all_unknown(malformed.total),
        "single-target query still rejects malformed unrelated packed entries");

  std::string packed;
  for (std::size_t i = 0; i < extended_targets.size(); ++i)
    packed += (i ? "," : "") + std::string(extended_targets[i].name) + ":" + std::to_string(2 * (i + 1));
  packed += ",IT_EXTRA_NINE:3E,IT_EXTRA_NINEX:99";
  populate(image, packed, 9);
  for (unsigned i = 0; i < extended_targets.size(); ++i) {
    image.put(item(i) + 0x32c, i + 1);
    image.put(item(i) + 0x330, static_cast<std::uint32_t>(extended_targets[i].symbol));
  }
  const auto many = ubc::read_packed_inventory(image.memory(), hero, extended_targets, true, workspace);
  check(many.target_count == 9 && many.storage_totals_known && many.listed_nodes == 9 &&
        many.packed_entries == 11 && many.snapshot_unchanged,
        "nine-target profile reports the complete stable inventory");
  for (std::size_t i = 0; i < extended_targets.size(); ++i) {
    const auto bonus = i == 8 ? 3 : 0;
    check(many.listed[i] == static_cast<std::int32_t>(i + 1) &&
          many.packed[i] == static_cast<std::int32_t>(2 * (i + 1)) + bonus &&
          many.total[i] == static_cast<std::int32_t>(3 * (i + 1)) + bonus &&
          many.listed_occurrences[i] == 1 && many.packed_occurrences[i] == (i == 8 ? 2u : 1u) &&
          many.duplicate_packed_target[i] == (i == 8) && many.has_active_marker[i] == (i == 8) &&
          many.mixed_representation[i],
          "all nine independent targets retain sums, occurrence counts and representation flags");
    check(ubc::station_detail::little_u32(workspace.records[i].node.data() + 4) == item(static_cast<unsigned>(i)),
          "reusable workspace retains all listed item identities independently of target capacity");
  }
  check(many.total[8] == 30, "ninth target is not confused with a longer packed name prefix");
  const auto many_unproven = ubc::read_packed_inventory(image.memory(), hero, extended_targets, false, workspace);
  check(many_unproven.listed_known && many_unproven.packed_known && all_unknown(many_unproven.total) &&
        !many_unproven.storage_totals_known &&
        many_unproven.issue == ubc::PackedInventoryIssue::boundary_unverified,
        "all nine totals remain unknown when the boundary has not been verified");
}

void active_target_prefix() {
  MemoryImage image;
  constexpr std::array<ubc::PackedInventoryTarget, 4> exact_prefix{{
      targets[0], targets[1], targets[2], targets[3]}};
  populate(image, "ITFOMUTTONRAW:2,ITFOMUTTON:3,ITMI_FLASK:4,ITPL_HEALTH_HERB_01:5,IT_EXTRA_NINE:90E", 2);
  image.put(item(1) + 0x330, static_cast<std::uint32_t>(extended_targets[8].symbol));
  auto value = ubc::read_packed_inventory<9>(image.memory(), hero, exact_prefix.data(),
                                           exact_prefix.size(), true, workspace);
  check(value.target_count == 4 && value.storage_totals_known && value.total[0] == 3 &&
        value.total[1] == 3 && value.total[2] == 4 && value.total[3] == 5 &&
        value.listed_nodes == 2 && value.any_active_marker && inactive_unknown(value),
        "pointer view consumes only its four real targets inside capacity nine");

  auto dirty = extended_targets;
  for (std::size_t i = exact_prefix.size(); i < dirty.size(); ++i) dirty[i] = {nullptr, -1};
  dirty[7] = dirty[0];
  dirty[8] = {"invalid target", 7};
  value = ubc::read_packed_inventory(image.memory(), hero, dirty, true, workspace, exact_prefix.size());
  check(value.target_count == 4 && value.storage_totals_known && value.total[0] == 3 &&
        inactive_unknown(value), "array prefix ignores invalid and duplicate unused target slots");
  populate(image, "ITFOMUTTONRAW:4,IT_EXTRA_NINE:0");
  value = ubc::read_packed_inventory(image.memory(), hero, dirty, true, workspace, exact_prefix.size());
  check(value.target_count == 4 && value.listed_known && !value.packed_known &&
        !value.storage_totals_known && inactive_unknown(value),
        "prefix selection does not weaken full foreign pack validation");

  for (const auto count : {std::size_t{0}, std::size_t{10}, std::numeric_limits<std::size_t>::max()}) {
    populate(image);
    value = ubc::read_packed_inventory<9>(image.memory(), hero, exact_prefix.data(), count, true, workspace);
    check(value.issue == ubc::PackedInventoryIssue::invalid_targets && value.target_count == 0 &&
          !value.header_known && !value.storage_totals_known && inactive_unknown(value) && image.reads.empty(),
          "zero or excessive active count is rejected before inventory or target access");
  }
  populate(image);
  value = ubc::read_packed_inventory<9>(image.memory(), hero, nullptr, 4, true, workspace);
  check(value.issue == ubc::PackedInventoryIssue::invalid_targets && value.target_count == 0 &&
        inactive_unknown(value) && image.reads.empty(),
        "null runtime target view is rejected before reading inventory");
  constexpr std::array<ubc::PackedInventoryTarget, 0> none{};
  const auto zero = ubc::read_packed_inventory(image.memory(), hero, none, true, workspace);
  check(zero.issue == ubc::PackedInventoryIssue::invalid_targets && zero.target_count == 0 &&
        !zero.storage_totals_known && image.reads.empty(), "zero-capacity array has no valid recipe target set");
  const auto excessive = ubc::read_packed_inventory(image.memory(), hero, dirty, true, workspace, 10);
  check(excessive.issue == ubc::PackedInventoryIssue::invalid_targets && excessive.target_count == 0 &&
        image.reads.empty(), "optional array count cannot exceed the physical array capacity");

  populate(image, "IT_EXTRA_NINE:8E", 1);
  image.put(item(0) + 0x330, static_cast<std::uint32_t>(extended_targets[8].symbol));
  const auto full = ubc::read_packed_inventory(image.memory(), hero, extended_targets, true, workspace);
  check(full.total[8] == 9 && full.mixed_representation[8] && full.has_active_marker[8],
        "workspace is first populated by a ninth-target mixed representation");
  const auto narrow = ubc::read_packed_inventory<9>(image.memory(), hero, exact_prefix.data(),
                                                 exact_prefix.size(), true, workspace);
  check(narrow.target_count == 4 && narrow.storage_totals_known && narrow.total[0] == 0 &&
        inactive_unknown(narrow), "reused workspace does not leak prior inactive target sums or flags");
}

void extended_target_failures() {
  MemoryImage image;
  for (unsigned mode = 0; mode < 4; ++mode) {
    populate(image);
    auto profile = extended_targets;
    if (mode == 0) profile[8].name = "bad";
    if (mode == 1) profile[8].symbol = -1;
    if (mode == 2) profile[8].symbol = profile[0].symbol;
    if (mode == 3) profile[8].name = profile[0].name;
    const auto invalid = ubc::read_packed_inventory(image.memory(), hero, profile, true, workspace);
    check(invalid.issue == ubc::PackedInventoryIssue::invalid_targets && !invalid.storage_totals_known &&
          all_unknown(invalid.total) && image.reads.empty(),
          "late targets retain name, symbol and duplicate validation before any memory read");
  }
  populate(image, "", 2);
  for (unsigned i = 0; i < 2; ++i)
    image.put(item(i) + 0x330, static_cast<std::uint32_t>(extended_targets[8].symbol));
  image.put(item(0) + 0x32c, INT32_MAX);
  auto value = ubc::read_packed_inventory(image.memory(), hero, extended_targets, true, workspace);
  check(value.issue == ubc::PackedInventoryIssue::listed_overflow && !value.listed_known &&
        value.packed_known && all_unknown(value.listed) && all_unknown(value.total),
        "ninth-target listed overflow discards the entire listed component");
  populate(image, "IT_EXTRA_NINE:2147483647,IT_EXTRA_NINE:1");
  value = ubc::read_packed_inventory(image.memory(), hero, extended_targets, true, workspace);
  check(value.issue == ubc::PackedInventoryIssue::packed_overflow && value.listed_known &&
        !value.packed_known && all_unknown(value.packed) && all_unknown(value.total),
        "ninth-target packed overflow discards the entire packed component");
  populate(image, "IT_EXTRA_NINE:2147483647", 1);
  image.put(item(0) + 0x330, static_cast<std::uint32_t>(extended_targets[8].symbol));
  value = ubc::read_packed_inventory(image.memory(), hero, extended_targets, true, workspace);
  check(value.issue == ubc::PackedInventoryIssue::total_overflow && value.listed_known &&
        value.packed_known && !value.storage_totals_known && all_unknown(value.total),
        "ninth-target combined overflow does not expose partially computed totals");

  for (const auto address : {header, pack, node(8), item(8) + 0x32c}) {
    populate(image, "IT_EXTRA_NINE:2,OTHER:1", 9);
    image.put(item(8) + 0x330, static_cast<std::uint32_t>(extended_targets[8].symbol));
    if (address == pack)
      image.mutations.push_back({pack, pack + static_cast<std::uint32_t>(std::strlen("IT_EXTRA_NINE:2,OTHER:1")) - 1, '2', 2, 1});
    else image.mutations.push_back({address, address, 123});
    value = ubc::read_packed_inventory(image.memory(), hero, extended_targets, true, workspace);
    check(value.issue == ubc::PackedInventoryIssue::changed_during_read && value.target_count == 9 &&
          !value.listed_known && !value.packed_known && !value.storage_totals_known &&
          !value.snapshot_unchanged && all_unknown(value.listed) && all_unknown(value.packed) &&
          all_unknown(value.total), "nine-target snapshot discards every component after any copied-byte change");
  }
  constexpr std::array<ubc::PackedInventoryTarget, 1> single{{targets[0]}};
  populate(image, "OTHER:2", 2);
  image.put(item(1) + 0x330, static_cast<std::uint32_t>(extended_targets[8].symbol));
  image.mutations.push_back({item(1) + 0x32c, item(1) + 0x32c, 2});
  value = ubc::read_packed_inventory<9>(image.memory(), hero, single.data(), single.size(), true, workspace);
  check(value.issue == ubc::PackedInventoryIssue::changed_during_read && value.target_count == 1 &&
        !value.storage_totals_known && all_unknown(value.listed) && all_unknown(value.packed) &&
        all_unknown(value.total) && inactive_unknown(value),
        "runtime prefix still detects changes in unrelated materialized items");
}
} // namespace

int main() {
  ordinary_cases(); syntax_and_overflow(); malformed_storage(); changed_copies();
  limits_and_profile(); address_boundaries();
  variable_target_counts(); active_target_prefix(); extended_target_failures();
  std::printf("PackedInventory: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
