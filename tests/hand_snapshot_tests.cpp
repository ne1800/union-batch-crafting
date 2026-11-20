// SPDX-License-Identifier: MIT
#include "ubc/engine/hand_snapshot.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace {
unsigned checks = 0;
unsigned failures = 0;
void check(bool condition, const char* message) {
  ++checks;
  if (!condition) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}

struct MemoryImage {
  static constexpr std::uint32_t base = 0x1000;
  std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(0x850000);
  std::uint32_t denied_begin = 0;
  std::uint32_t denied_end = 0;
  std::uint32_t calls = 0;
  static bool read(void* context, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& image = *static_cast<MemoryImage*>(context);
    ++image.calls;
    const std::uint64_t end = static_cast<std::uint64_t>(address) + size;
    if (address < base || end > static_cast<std::uint64_t>(base) + image.bytes.size() ||
        (image.denied_begin && address < image.denied_end && end > image.denied_begin)) return false;
    std::memcpy(output, image.bytes.data() + address - base, size);
    return true;
  }
  ubc::ReadOnlyMemory memory() { return {this, read}; }
  void put(std::uint32_t address, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
      bytes.at(address - base + i) = static_cast<std::uint8_t>(value >> (8 * i));
  }
  void text(std::uint32_t address, const std::string& value) {
    std::copy(value.begin(), value.end(), bytes.begin() + (address - base));
    bytes.at(address - base + value.size()) = 0;
  }
};

constexpr std::uint32_t hero = 0x2000;
constexpr std::uint32_t array = 0x10000;
constexpr std::uint32_t slot(unsigned i) { return 0x20000 + i * 0x80; }
constexpr std::uint32_t name(unsigned i) { return 0x40000 + i * 0x50; }
constexpr std::uint32_t item(unsigned i) { return 0x60000 + i * 0x400; }

void make_slot(MemoryImage& image, unsigned i, const std::string& label, std::uint32_t vob) {
  image.put(array + 4 * i, slot(i));
  image.put(slot(i) + 8, name(i));
  image.put(slot(i) + 0xc, static_cast<std::uint32_t>(label.size()));
  image.text(name(i), label);
  image.put(slot(i) + 0x30, vob);
}

void populate(MemoryImage& image, unsigned count = 2) {
  image.denied_begin = image.denied_end = image.calls = 0;
  image.put(hero + 0x9b0, count ? array : 0);
  image.put(hero + 0x9b4, count);
  image.put(hero + 0x9b8, count);
  image.put(0x83c804, 0x711410);
  constexpr std::array<std::uint8_t, 6> getter{{0xb8, 0x68, 0x11, 0xab, 0x00, 0xc3}};
  std::copy(getter.begin(), getter.end(), image.bytes.begin() + (0x711410 - MemoryImage::base));
  for (unsigned i = 0; i < count; ++i) {
    make_slot(image, i, "SLOT_" + std::to_string(i), 0);
    image.put(item(i), 0x83c804);
    image.put(item(i) + 0x32c, i + 1);
    image.put(item(i) + 0x330, 1000 + i);
  }
  if (count) make_slot(image, 0, "ZS_LEFTHAND", item(0));
  if (count > 1) make_slot(image, 1, "ZS_RIGHTHAND", item(1));
}

bool unreadable(const ubc::HandObjectSnapshot& value) {
  return value.state == -1 && value.symbol == -1 && value.amount == -1;
}
void rejected(const ubc::HandsSnapshot& value, const char* message) {
  check(unreadable(value.left) && unreadable(value.right), message);
}

void ordinary_cases(MemoryImage& image) {
  populate(image, 0);
  auto result = ubc::read_hands(image.memory(), hero);
  check(result.left.state == 0 && result.right.state == 0, "no slots means both hands empty");
  check(result.left.symbol == -1 && result.left.amount == -1, "empty hands have no item metadata");
  populate(image);
  result = ubc::read_hands(image.memory(), hero);
  check(result.left.state == 1 && result.left.symbol == 1000 && result.left.amount == 1,
        "exact left-hand item");
  check(result.right.state == 1 && result.right.symbol == 1001 && result.right.amount == 2,
        "exact right-hand item");
  check(result.left.object == item(0) && result.right.object == item(1),
        "copied hand identities permit inventory alias checks without engine getters");
  image.put(slot(0) + 0x30, 0);
  check(ubc::read_hands(image.memory(), hero).left.state == 0, "null slot vob means empty hand");
  make_slot(image, 0, "OTHER_SLOT", item(0));
  check(ubc::read_hands(image.memory(), hero).left.state == 0, "missing hand name after full valid walk");
  populate(image);
  image.put(item(0), 0x87654321);
  result = ubc::read_hands(image.memory(), hero);
  check(result.left.state == 2 && result.left.symbol == -1 && result.left.amount == -1,
        "foreign vtable stays occupied and unknown");
  check(result.right.state == 1, "foreign type does not hide other hand");
  populate(image);
  image.put(item(0) + 0x32c, 0);
  result = ubc::read_hands(image.memory(), hero);
  check(result.left.state == 1 && result.left.amount == 0, "zero-count item is still held");
  image.put(item(0) + 0x32c, INT32_MAX);
  check(ubc::read_hands(image.memory(), hero).left.amount == INT32_MAX, "INT_MAX amount is representable");
}

void broken_slots(MemoryImage& image) {
  for (const auto offset : {0x9b4u, 0x9b8u}) {
    populate(image);
    image.put(hero + offset, UINT32_MAX);
    rejected(ubc::read_hands(image.memory(), hero), "negative array metadata rejected");
  }
  populate(image);
  image.put(hero + 0x9b4, 1);
  rejected(ubc::read_hands(image.memory(), hero), "count above capacity rejected");
  populate(image);
  image.put(hero + 0x9b0, UINT32_MAX - 3);
  rejected(ubc::read_hands(image.memory(), hero), "pointer array range overflow rejected");
  populate(image);
  image.put(hero + 0x9b0, 0);
  rejected(ubc::read_hands(image.memory(), hero), "nonempty array cannot have null data");
  populate(image);
  image.put(array, 0);
  rejected(ubc::read_hands(image.memory(), hero), "null slot pointer rejected");
  populate(image);
  image.put(array + 4, slot(0));
  rejected(ubc::read_hands(image.memory(), hero), "duplicate slot pointer rejected");
  populate(image);
  make_slot(image, 1, "ZS_LEFTHAND", item(1));
  rejected(ubc::read_hands(image.memory(), hero), "duplicate hand name rejected");
  populate(image);
  make_slot(image, 0, "OTHER", item(0));
  make_slot(image, 1, "OTHER", item(1));
  rejected(ubc::read_hands(image.memory(), hero), "duplicate other-slot name rejected");
  populate(image);
  image.put(slot(1) + 0x30, item(0));
  rejected(ubc::read_hands(image.memory(), hero), "same object cannot count as two unambiguous hands");
  populate(image);
  image.put(array, UINT32_MAX - 8);
  rejected(ubc::read_hands(image.memory(), hero), "slot range overflow rejected");
  populate(image);
  image.put(slot(0) + 8, UINT32_MAX - 2);
  rejected(ubc::read_hands(image.memory(), hero), "slot-name range overflow rejected");
  populate(image);
  image.put(slot(0) + 0xc, UINT32_MAX);
  rejected(ubc::read_hands(image.memory(), hero), "negative name length rejected");
  populate(image);
  image.put(slot(0) + 0xc, 0);
  rejected(ubc::read_hands(image.memory(), hero), "empty slot name rejected");
  populate(image);
  make_slot(image, 0, std::string(64, 'x'), 0);
  rejected(ubc::read_hands(image.memory(), hero), "name above explicit limit rejected");
  populate(image);
  make_slot(image, 0, std::string(63, 'x'), 0);
  check(ubc::read_hands(image.memory(), hero).right.state == 1, "name at explicit limit accepted");
  populate(image);
  image.bytes.at(name(0) - MemoryImage::base + 11) = 'x';
  rejected(ubc::read_hands(image.memory(), hero), "missing name terminator rejected");
  populate(image);
  image.bytes.at(name(0) - MemoryImage::base + 2) = 0;
  rejected(ubc::read_hands(image.memory(), hero), "embedded name NUL rejected");
  rejected(ubc::read_hands(image.memory(), 0), "null hero rejected");
  rejected(ubc::read_hands(image.memory(), UINT32_MAX - 0x9b0), "hero field range overflow rejected");
  rejected(ubc::read_hands({nullptr, nullptr}, hero), "absent reader rejected");

  populate(image, 256);
  auto result = ubc::read_hands(image.memory(), hero);
  check(result.left.state == 1 && result.right.state == 1, "exact slot limit accepted");
  check(image.calls < 1000, "slot reads remain bounded");
  populate(image, 257);
  rejected(ubc::read_hands(image.memory(), hero), "slot limit exceeded");
  check(image.calls <= 2, "oversized array rejected before walk");
}

void broken_items_and_guards(MemoryImage& image) {
  populate(image);
  image.put(0x83c804, 0x711400);
  auto result = ubc::read_hands(image.memory(), hero);
  check(result.left.state == 2 && result.right.state == 2, "changed class method cannot prove item type");
  populate(image);
  image.bytes.at(0x711410 - MemoryImage::base) = 0xe9;
  check(ubc::read_hands(image.memory(), hero).left.state == 2, "patched class method is never executed");
  for (const auto offset : {0x32cu, 0x330u}) {
    populate(image);
    image.put(item(0) + offset, UINT32_MAX);
    result = ubc::read_hands(image.memory(), hero);
    check(unreadable(result.left) && result.right.state == 1, "negative item fields are not usable metadata");
  }
  populate(image);
  image.put(slot(0) + 0x30, UINT32_MAX - 1);
  result = ubc::read_hands(image.memory(), hero);
  check(unreadable(result.left) && result.right.state == 1, "unreadable held object preserves other hand");
  for (std::uint32_t i = 0; i < 8; ++i) {
    populate(image);
    image.denied_begin = item(0) + 0x32c + i;
    image.denied_end = image.denied_begin + 1;
    result = ubc::read_hands(image.memory(), hero);
    check(unreadable(result.left) && result.right.state == 1, "every truncated item byte rejected");
  }
  for (std::uint32_t i = 0; i < 12; ++i) {
    populate(image);
    image.denied_begin = hero + 0x9b0 + i;
    image.denied_end = image.denied_begin + 1;
    rejected(ubc::read_hands(image.memory(), hero), "every truncated array-header byte rejected");
  }
  for (std::uint32_t i = 0; i < 12; ++i) {
    populate(image);
    image.denied_begin = name(0) + i;
    image.denied_end = image.denied_begin + 1;
    rejected(ubc::read_hands(image.memory(), hero), "every truncated hand-name byte rejected");
  }
}
}  // namespace

int main() {
  MemoryImage image;
  ordinary_cases(image);
  broken_slots(image);
  broken_items_and_guards(image);
  std::printf("HandSnapshot: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
