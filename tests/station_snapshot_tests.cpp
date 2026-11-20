// SPDX-License-Identifier: MIT
#include "ubc/read_only_memory.hpp"
#include "ubc/station_snapshot.hpp"
#include "ubc/station_abi.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

namespace {
unsigned checks = 0;
unsigned failures = 0;
void check(bool condition, const char* message) {
  ++checks;
  if (!condition) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}

struct CountingReader {
  unsigned calls = 0;
  static bool read(void* context, std::uint32_t, void* output, std::size_t size) noexcept {
    ++static_cast<CountingReader*>(context)->calls;
    if (size) std::memset(output, 0x5a, size);
    return true;
  }
};

struct MemoryImage {
  static constexpr std::uint32_t base = 0x1000;
  std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(0x400000);
  std::uint32_t denied_begin = 0;
  std::uint32_t denied_end = 0;
  std::uint32_t calls = 0;

  static bool read(void* context, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& image = *static_cast<MemoryImage*>(context);
    ++image.calls;
    const std::uint64_t end = static_cast<std::uint64_t>(address) + size;
    if (address < base || end > static_cast<std::uint64_t>(base) + image.bytes.size() ||
        (image.denied_begin && address < image.denied_end && end > image.denied_begin)) return false;
    std::memcpy(output, image.bytes.data() + (address - base), size);
    return true;
  }
  ubc::ReadOnlyMemory memory() { return {this, read}; }
  void put(std::uint32_t address, std::uint32_t value) {
    const auto offset = static_cast<std::size_t>(address - base);
    for (unsigned i = 0; i < 4; ++i) bytes.at(offset + i) = static_cast<std::uint8_t>(value >> (8 * i));
  }
};

constexpr std::uint32_t hero = 0x2000;
constexpr std::uint32_t inventory = hero + 0x668;
constexpr std::array<std::int32_t, 6> symbols{{1001, 1002, 1003, 1004, 1005, 1006}};
constexpr std::uint32_t node(unsigned index) { return 0x10000 + index * 0x10; }
constexpr std::uint32_t item(unsigned index) { return 0x30000 + index * 0x350; }

void populate(MemoryImage& image, unsigned count, std::int32_t amount = 1) {
  image.put(inventory + 0xa0, hero);
  image.put(inventory + 0xa4, 0);
  image.put(inventory + 0xb0, count ? node(0) : 0);
  image.denied_begin = image.denied_end = image.calls = 0;
  for (unsigned i = 0; i < count; ++i) {
    image.put(node(i), 0);
    image.put(node(i) + 4, item(i));
    image.put(node(i) + 8, i + 1 < count ? node(i + 1) : 0);
    image.put(item(i) + 0x32c, static_cast<std::uint32_t>(amount));
    image.put(item(i) + 0x330, static_cast<std::uint32_t>(symbols[i % symbols.size()]));
  }
}

void rejected(const ubc::InventorySnapshot& result, const char* message) {
  check(!result.valid && !result.totals_known &&
        std::all_of(result.listed.begin(), result.listed.end(), [](auto n) { return n == 0; }), message);
}

void memory_boundaries() {
  CountingReader reader;
  const ubc::ReadOnlyMemory memory{&reader, CountingReader::read};
  std::uint8_t output = 0;
  check(!memory.copy(0, &output, 1), "null address rejected");
  check(!memory.copy(0, &output, 0), "zero-sized null address rejected");
  check(!memory.copy(1, nullptr, 1), "null destination rejected");
  check(!memory.copy(UINT32_MAX, &output, 2), "inclusive address range cannot wrap");
  check(!memory.copy(2, &output, std::numeric_limits<std::size_t>::max()), "huge size rejected");
  check(reader.calls == 0, "invalid ranges never reach reader");
  check(memory.copy(UINT32_MAX, &output, 1) && output == 0x5a, "last 32-bit byte is representable");
  const auto before = reader.calls;
  check(memory.copy(1, nullptr, 0) && reader.calls == before, "zero length is a no-op");
  const ubc::ReadOnlyMemory absent{nullptr, nullptr};
  check(!absent.copy(1, &output, 1), "missing reader rejected");
}

void inventory_cases() {
  MemoryImage image;
  populate(image, 0);
  auto result = ubc::read_inventory(image.memory(), hero, symbols);
  check(result.valid && result.totals_known && result.nodes == 0 &&
        result.listed == std::array<std::int32_t, 6>{}, "empty inventory is known");
  populate(image, 12, 3);
  result = ubc::read_inventory(image.memory(), hero, symbols);
  check(result.valid && result.totals_known && result.nodes == 12 &&
        result.listed == std::array<std::int32_t, 6>{{6, 6, 6, 6, 6, 6}}, "six ingredients and multiple stacks");
  image.put(item(0) + 0x330, 9999);
  result = ubc::read_inventory(image.memory(), hero, symbols);
  check(result.valid && result.listed[0] == 3 && result.nodes == 12, "unknown items are ignored in sums");
  image.put(inventory + 0xa4, 1);
  result = ubc::read_inventory(image.memory(), hero, symbols);
  check(result.valid && !result.totals_known && result.listed[0] == 3, "packed total remains unknown");
  image.put(inventory + 0xa4, UINT32_MAX);
  check(!ubc::read_inventory(image.memory(), hero, symbols).totals_known, "any nonzero pack flag is unknown");

  populate(image, 2);
  image.put(inventory + 0xa0, hero + 4);
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "wrong inventory owner rejected");
  populate(image, 2);
  image.put(node(1) + 8, node(0));
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "two-node cycle rejected");
  check(image.calls < 20, "cycle is detected promptly");
  populate(image, 2);
  image.put(node(1) + 4, item(0));
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "duplicate item pointer rejected");
  populate(image, 2);
  image.put(node(1) + 4, 0);
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "null item pointer rejected");
  populate(image, 2);
  image.put(item(1) + 0x32c, UINT32_MAX);
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "negative amount rejected after valid prefix");
  image.put(item(1) + 0x330, 9999);
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "negative unknown-item amount is also rejected");
  populate(image, 2, 0);
  result = ubc::read_inventory(image.memory(), hero, symbols);
  check(result.valid && result.totals_known && result.listed == std::array<std::int32_t, 6>{}, "zero amount is valid");

  populate(image, 2);
  image.put(item(0) + 0x32c, INT32_MAX);
  image.put(item(1) + 0x330, symbols[0]);
  image.put(item(1) + 0x32c, 0);
  result = ubc::read_inventory(image.memory(), hero, symbols);
  check(result.valid && result.listed[0] == INT32_MAX, "INT_MAX is representable");
  image.put(item(1) + 0x32c, 1);
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "sum above INT_MAX rejected");

  populate(image, 2);
  image.denied_begin = node(1) + 8;
  image.denied_end = node(1) + 12;
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "unreadable node suffix rejected");
  populate(image, 2);
  image.denied_begin = item(1) + 0x330;
  image.denied_end = item(1) + 0x334;
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "unreadable item symbol rejected");
  populate(image, 2);
  image.put(node(1) + 8, 0x70000000);
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "detached next node rejected");
  populate(image, 1);
  image.put(node(0) + 4, UINT32_MAX - 0x32b);
  rejected(ubc::read_inventory(image.memory(), hero, symbols), "item field address overflow rejected");
  rejected(ubc::read_inventory(image.memory(), UINT32_MAX - 0x667, symbols), "inventory address overflow rejected");
  rejected(ubc::read_inventory(image.memory(), 0, symbols), "null hero rejected");
  const ubc::ReadOnlyMemory absent{nullptr, nullptr};
  rejected(ubc::read_inventory(absent, hero, symbols), "missing inventory reader rejected");

  populate(image, 4096);
  result = ubc::read_inventory(image.memory(), hero, symbols);
  std::int32_t total = 0;
  for (const auto amount : result.listed) total += amount;
  check(result.valid && result.totals_known && result.nodes == 4096 && total == 4096, "exact node limit allowed");
  check(image.calls < 20000, "large inventory uses bounded linear reads");
  populate(image, 4097);
  result = ubc::read_inventory(image.memory(), hero, symbols);
  rejected(result, "node above limit rejected");
  check(result.nodes <= 4096 && image.calls < 20000, "limit prevents unbounded reading");

  // Deterministic links back into the list
  // must never appear as complete, known stock.
  populate(image, 32);
  std::uint32_t seed = 0x6c3a72d1;
  for (unsigned trial = 0; trial < 256; ++trial) {
    seed = seed * 1664525u + 1013904223u;
    const auto previous = seed % 32;
    image.put(node(31) + 8, node(previous));
    rejected(ubc::read_inventory(image.memory(), hero, symbols), "seeded cycle rejected");
  }
}

void abi_contracts() {
  namespace abi = ubc::station_abi;
  check(abi::hook_sites.size() == 5, "five outer passive hook sites");
  constexpr std::array<std::uintptr_t, 5> addresses{{0x721580, 0x721950, 0x660bb0, 0x703970, 0x6629a0}};
  constexpr std::array<std::size_t, 5> lengths{{6, 6, 5, 6, 7}};
  for (std::size_t i = 0; i < addresses.size(); ++i) {
    const auto& site = abi::hook_sites[i];
    check(site.address == addresses[i] && site.patch_size == lengths[i] &&
          site.guard_size > site.patch_size && site.guard_size <= site.original.size(), "complete guarded patch span");
    check(site.address + site.patch_size <= abi::do_stack_entry ||
          site.address >= abi::do_stack_entry + abi::do_stack_disk_entry.size(),
          "never patch Ikarus DoStack entry");
  }
}
}  // namespace

int main() {
  memory_boundaries();
  inventory_cases();
  abi_contracts();
  std::printf("StationSnapshot: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
