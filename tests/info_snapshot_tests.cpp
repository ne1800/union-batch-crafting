// SPDX-License-Identifier: MIT
#include "ubc/info_snapshot.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
unsigned checks = 0;
unsigned failures = 0;
void check(bool condition, const char* message) {
  ++checks;
  if (!condition) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}

struct MemoryImage {
  struct Region { std::uint32_t base; std::vector<std::uint8_t> bytes; };
  std::vector<Region> regions{{0x1000, std::vector<std::uint8_t>(0xf000)}};
  std::uint64_t denied_begin = 0;
  std::uint64_t denied_end = 0;
  std::vector<std::uint32_t> reads;
  unsigned overflow_reads = 0;

  static bool read(void* context, std::uint32_t address, void* output,
                   std::size_t size) noexcept {
    auto& image = *static_cast<MemoryImage*>(context);
    image.reads.push_back(address);
    const auto end = std::uint64_t{address} + size;
    if (end > (std::uint64_t{1} << 32)) { ++image.overflow_reads; return false; }
    if (image.denied_begin && address < image.denied_end && end > image.denied_begin) return false;
    for (const auto& region : image.regions) {
      if (address >= region.base && end <= std::uint64_t{region.base} + region.bytes.size()) {
        std::memcpy(output, region.bytes.data() + (address - region.base), size);
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
  void put(std::uint32_t address, std::uint32_t value) {
    for (auto& region : regions) {
      if (address >= region.base && std::uint64_t{address} + 4 <=
          std::uint64_t{region.base} + region.bytes.size()) {
        for (unsigned i = 0; i < 4; ++i)
          region.bytes[address - region.base + i] = static_cast<std::uint8_t>(value >> (8 * i));
        return;
      }
    }
    std::abort();
  }
  void deny(std::uint32_t address) {
    denied_begin = address;
    denied_end = std::uint64_t{address} + 1;
  }
};

constexpr std::uint32_t manager = 0x2000;
constexpr std::uint32_t info = 0x3000;
constexpr std::uint32_t node(unsigned i) { return 0x4000 + i * 0x10; }
constexpr std::uint32_t choice(unsigned i) { return 0x8000 + i * 0x20; }

void make_info(MemoryImage& image, std::uint32_t address = info) {
  image.put(address, 0x83c44c);
  image.put(address + 0x2c, 20339);
  image.put(address + 0x4c, 1); // told is explicitly not an instance index.
  image.put(address + 0x50, 20337);
  image.put(address + 0x54, 0);
  image.put(address + 0x58, 0);
}

void populate(MemoryImage& image, unsigned count = 3) {
  image = MemoryImage{};
  make_info(image);
  image.put(manager + 0x28, info);
  image.put(info + 0x58, count ? node(0) : 0);
  for (unsigned i = 0; i < count; ++i) {
    image.put(node(i), choice(i));
    image.put(node(i) + 4, i + 1 < count ? node(i + 1) : 0);
    image.put(choice(i), 0x82e6f0);
    image.put(choice(i) + 8, 0xdeadbeef); // Never follow a text pointer.
    image.put(choice(i) + 0x14, 20341 + i);
  }
}

bool empty(const ubc::InfoSnapshot& value) {
  return !value.valid && value.instance == -1 && value.function == -1;
}
void rejected(const ubc::InfoSnapshot& value, const char* reason) {
  check(empty(value), reason);
}
void rejected(const ubc::ChoiceSnapshot& value, const char* reason) {
  check(!value.valid && empty(value.info) && value.function == -1 &&
        value.index == -1 && value.nodes == 0, reason);
}

void ordinary_cases() {
  MemoryImage image;
  populate(image);
  const auto owner = ubc::read_info(image.memory(), info);
  check(owner.valid && owner.function == 20339 && owner.instance == 20337,
        "info function and instance use distinct fields, not told");
  for (unsigned i = 0; i < 3; ++i) {
    const auto result = ubc::read_choice(image.memory(), manager, choice(i));
    check(result.valid && result.info.valid && result.info.instance == 20337 &&
          result.info.function == 20339 && result.function == static_cast<std::int32_t>(20341 + i) &&
          result.index == static_cast<std::int32_t>(i) && result.nodes == 3,
          "every list position resolves exact selected function and owner");
  }
  image.put(choice(2) + 0x14, 20341);
  check(ubc::read_choice(image.memory(), manager, choice(2)).valid,
        "distinct choices may legally name the same function");
  image.put(info + 0x2c, 0);
  image.put(info + 0x50, INT32_MAX);
  image.put(choice(0) + 0x14, 0);
  const auto unprofiled = ubc::read_choice(image.memory(), manager, choice(0));
  check(unprofiled.valid && unprofiled.info.instance == INT32_MAX &&
        unprofiled.info.function == 0 && unprofiled.function == 0,
        "representable metadata does not pretend to verify script-profile semantics");

  populate(image);
  const auto copied = ubc::read_choice(image.memory(), manager, choice(1));
  image.put(choice(1) + 0x14, 999);
  image.put(info + 0x50, 888);
  image.deny(choice(1));
  check(copied.function == 20342 && copied.info.instance == 20337 && copied.index == 1,
        "snapshot owns copied scalars and retains no borrowed choice data");

  populate(image);
  make_info(image, info + 1);
  check(ubc::read_info(image.memory(), info + 1).valid,
        "packed unaligned info is decoded without native pointer casts");
}

void malformed_metadata() {
  MemoryImage image;
  for (const auto value : {0u, 0x83c448u, UINT32_MAX}) {
    populate(image);
    image.put(info, value);
    rejected(ubc::read_info(image.memory(), info), "wrong native info vptr rejected");
    rejected(ubc::read_choice(image.memory(), manager, choice(0)), "wrong owner type rejected");
  }
  for (const auto value : {0x80000000u, UINT32_MAX}) {
    for (const auto offset : {0x2cu, 0x50u}) {
      populate(image);
      image.put(info + offset, value);
      rejected(ubc::read_info(image.memory(), info), "negative info symbol index rejected");
      rejected(ubc::read_choice(image.memory(), manager, choice(0)), "negative owner metadata rejected");
    }
    for (unsigned i = 0; i < 3; ++i) {
      populate(image);
      image.put(choice(i) + 0x14, value);
      rejected(ubc::read_choice(image.memory(), manager, choice(0)),
               "negative function index anywhere in list rejected");
    }
  }
  populate(image);
  rejected(ubc::read_info(image.memory(), 0), "null info rejected");
  rejected(ubc::read_info({nullptr, nullptr}, info), "absent info reader rejected");
  rejected(ubc::read_choice(image.memory(), 0, choice(0)), "null manager rejected");
  rejected(ubc::read_choice(image.memory(), manager, 0), "null selected choice rejected");
  rejected(ubc::read_choice({nullptr, nullptr}, manager, choice(0)), "absent choice reader rejected");
  image.put(manager + 0x28, 0);
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "null owner rejected");
  populate(image, 0);
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "empty list cannot own selection");
  populate(image);
  rejected(ubc::read_choice(image.memory(), manager, choice(3)), "selected pointer must be present");
}

void malformed_lists() {
  MemoryImage image;
  populate(image);
  image.put(node(0) + 4, node(0));
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "self cycle after selected entry rejected");
  populate(image);
  image.put(node(2) + 4, node(1));
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "tail cycle after selected entry rejected");
  populate(image);
  image.put(node(2), choice(0));
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "selected pointer appearing twice rejected");
  populate(image);
  image.put(node(2), choice(1));
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "duplicate unselected choice also rejected");
  populate(image);
  image.put(node(2), 0);
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "null choice after match rejected");
  populate(image);
  image.put(node(2) + 4, 0x40000000);
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "unreadable tail node after match rejected");
  populate(image);
  image.put(node(2), 0x40000000);
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "unreadable unselected choice rejected");

  static_assert(ubc::max_info_choices == 256, "Documented bounded menu-list contract");
  populate(image, ubc::max_info_choices);
  const auto result = ubc::read_choice(image.memory(), manager, choice(ubc::max_info_choices - 1));
  check(result.valid && result.nodes == ubc::max_info_choices && result.index == 255,
        "exact explicit list bound accepted");
  check(image.reads.size() <= 2 * ubc::max_info_choices + 8, "memory reads remain linearly bounded");
  populate(image, ubc::max_info_choices + 1);
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "overlong list after match rejected");
  bool read_extra_node = false;
  for (const auto address : image.reads) read_extra_node |= address == node(ubc::max_info_choices);
  check(!read_extra_node && image.reads.size() <= 2 * ubc::max_info_choices + 8,
        "reader never follows a node beyond its explicit bound");
}

void unreadable_bytes() {
  MemoryImage image;
  for (std::uint32_t i = 0; i < 0x5c; ++i) {
    populate(image);
    image.deny(info + i);
    rejected(ubc::read_info(image.memory(), info), "each truncated native info byte rejected");
    rejected(ubc::read_choice(image.memory(), manager, choice(0)), "each truncated owner byte rejected");
  }
  for (std::uint32_t i = 0; i < 4; ++i) {
    populate(image);
    image.deny(manager + 0x28 + i);
    rejected(ubc::read_choice(image.memory(), manager, choice(0)), "each truncated manager info-pointer byte rejected");
  }
  for (unsigned entry = 0; entry < 3; ++entry) {
    for (std::uint32_t i = 0; i < 8; ++i) {
      populate(image);
      image.deny(node(entry) + i);
      rejected(ubc::read_choice(image.memory(), manager, choice(0)), "each truncated list-node byte rejected");
    }
    for (std::uint32_t i = 0; i < 0x18; ++i) {
      populate(image);
      image.deny(choice(entry) + i);
      rejected(ubc::read_choice(image.memory(), manager, choice(0)), "each truncated choice byte rejected");
    }
  }
}

void address_boundaries() {
  MemoryImage image;
  populate(image);
  constexpr auto last_info = UINT32_MAX - 0x5b;
  image.add(last_info, 0x5c);
  make_info(image, last_info);
  check(ubc::read_info(image.memory(), last_info).valid, "info ending at last x86 address accepted");
  image.put(manager + 0x28, last_info);
  image.put(last_info + 0x58, node(0));
  check(ubc::read_choice(image.memory(), manager, choice(0)).valid,
        "choice owner ending at last x86 address accepted");
  for (const auto address : {last_info + 1, UINT32_MAX}) {
    rejected(ubc::read_info(image.memory(), address), "info extent overflow rejected");
    image.put(manager + 0x28, address);
    rejected(ubc::read_choice(image.memory(), manager, choice(0)), "owner extent overflow rejected");
  }
  check(image.overflow_reads == 0, "info overflow never reaches supplied reader");

  populate(image);
  constexpr auto last_manager = UINT32_MAX - 0x2b;
  image.add(last_manager, 0x2c);
  image.put(last_manager + 0x28, info);
  check(ubc::read_choice(image.memory(), last_manager, choice(0)).valid,
        "manager info-pointer ending at last x86 address accepted");
  for (const auto address : {last_manager + 1, UINT32_MAX - 0x27, UINT32_MAX})
    rejected(ubc::read_choice(image.memory(), address, choice(0)), "manager addition or word overflow rejected");
  check(image.overflow_reads == 0, "manager overflow never reaches supplied reader");

  populate(image, 1);
  constexpr auto last_node = UINT32_MAX - 7;
  image.add(last_node, 8);
  image.put(info + 0x58, last_node);
  image.put(last_node, choice(0));
  image.put(last_node + 4, 0);
  check(ubc::read_choice(image.memory(), manager, choice(0)).valid,
        "list node ending at last x86 address accepted");
  image.put(info + 0x58, last_node + 1);
  rejected(ubc::read_choice(image.memory(), manager, choice(0)), "list-node range overflow rejected");
  check(image.overflow_reads == 0, "list-node overflow never reaches supplied reader");

  populate(image, 1);
  constexpr auto last_choice = UINT32_MAX - 0x17;
  image.add(last_choice, 0x18);
  image.put(last_choice + 0x14, 20341);
  image.put(node(0), last_choice);
  check(ubc::read_choice(image.memory(), manager, last_choice).valid,
        "choice ending at last x86 address accepted");
  image.put(node(0), last_choice + 1);
  rejected(ubc::read_choice(image.memory(), manager, last_choice + 1), "choice extent overflow rejected");
  image.put(node(0), UINT32_MAX - 0x13);
  rejected(ubc::read_choice(image.memory(), manager, UINT32_MAX - 0x13), "choice function offset wrap rejected");
  check(image.overflow_reads == 0, "choice overflow never reaches supplied reader");
}
} // namespace

int main() {
  ordinary_cases();
  malformed_metadata();
  malformed_lists();
  unreadable_bytes();
  address_boundaries();
  std::printf("InfoSnapshot: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
