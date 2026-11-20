// SPDX-License-Identifier: MIT
#include "ubc/engine/pe_guard.hpp"
#include "ubc/core/quantity.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace {
std::uint64_t checks = 0;
std::uint64_t failures = 0;

void check(bool condition, const char* expression, int line) {
  ++checks;
  if (!condition) {
    ++failures;
    std::cerr << "FAIL line " << line << ": " << expression << '\n';
  }
}

#define CHECK(...) check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

void write16(std::vector<std::uint8_t>& bytes, std::size_t offset,
             std::uint16_t value) {
  bytes.at(offset) = static_cast<std::uint8_t>(value);
  bytes.at(offset + 1) = static_cast<std::uint8_t>(value >> 8);
}

void write32(std::vector<std::uint8_t>& bytes, std::size_t offset,
             std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) {
    bytes.at(offset + i) = static_cast<std::uint8_t>(value >> (8 * i));
  }
}

// Our own PE header data. No bytes from a game installation.
std::vector<std::uint8_t> target_header(std::size_t pe_offset = 128,
                                        std::uint16_t optional_size = 224) {
  std::vector<std::uint8_t> bytes(512, 0);
  bytes[0] = 'M';
  bytes[1] = 'Z';
  write32(bytes, 60, static_cast<std::uint32_t>(pe_offset));
  write32(bytes, pe_offset, 0x00004550);
  write16(bytes, pe_offset + 4, 0x014c);
  write32(bytes, pe_offset + 8, 0x3f966d87);
  write16(bytes, pe_offset + 20, optional_size);
  const auto optional_offset = pe_offset + 24;
  write16(bytes, optional_offset, 0x010b);
  write32(bytes, optional_offset + 28, 0x00400000);
  write32(bytes, optional_offset + 56, 0x00752000);
  return bytes;
}

bool matches(const std::vector<std::uint8_t>& bytes) {
  return ubc::matches_gothic2_header(bytes.data(), bytes.size());
}

void quantity_tests() {
  ubc::Quantity quantity;
  CHECK(quantity.maximum() == 0);
  CHECK(quantity.value() == 0);
  quantity.adjust(5);
  CHECK(quantity.value() == 0);
  CHECK(!quantity.preset(0));
  CHECK(!quantity.preset(1));

  quantity.reset(10);
  CHECK(quantity.maximum() == 10);
  CHECK(quantity.value() == 1);
  quantity.adjust(1);
  CHECK(quantity.value() == 2);
  quantity.adjust(5);
  CHECK(quantity.value() == 7);
  quantity.adjust(5);
  CHECK(quantity.value() == 10);
  quantity.adjust(-5);
  CHECK(quantity.value() == 5);
  quantity.adjust(-5);
  CHECK(quantity.value() == 1);
  quantity.adjust(0);
  CHECK(quantity.value() == 1);

  CHECK(quantity.preset(5));
  CHECK(quantity.value() == 5);
  CHECK(!quantity.preset(0));
  CHECK(!quantity.preset(11));
  CHECK(quantity.value() == 5);
  CHECK(quantity.preset(quantity.maximum()));
  CHECK(quantity.value() == 10);

  quantity.update_maximum(20);
  CHECK(quantity.maximum() == 20);
  CHECK(quantity.value() == 10);
  quantity.update_maximum(4);
  CHECK(quantity.value() == 4);
  CHECK(!quantity.preset(5));
  CHECK(quantity.value() == 4);
  quantity.update_maximum(0);
  CHECK(quantity.value() == 0);
  quantity.update_maximum(5);
  CHECK(quantity.value() == 1);
  CHECK(quantity.maximum() == 5);
  CHECK(quantity.preset(5));
  quantity.reset(5);
  CHECK(quantity.value() == 1);
  quantity.reset(0);
  CHECK(quantity.value() == 0);

  const auto largest = std::numeric_limits<std::uint32_t>::max();
  const auto step_min = std::numeric_limits<std::int32_t>::min();
  const auto step_max = std::numeric_limits<std::int32_t>::max();
  ubc::Quantity huge(largest);
  CHECK(huge.value() == 1);
  CHECK(huge.preset(largest));
  huge.adjust(step_max);
  CHECK(huge.value() == largest);
  huge.adjust(step_min);
  CHECK(huge.value() == 2147483647U);
  huge.adjust(step_min);
  CHECK(huge.value() == 1);
  huge.adjust(step_max);
  CHECK(huge.value() == 2147483648U);
  huge.adjust(step_max);
  CHECK(huge.value() == largest);
  huge.adjust(1);
  CHECK(huge.value() == largest);
  huge.reset(largest);
  CHECK(huge.value() == 1);

  ubc::Quantity single(1);
  single.adjust(step_max);
  CHECK(single.value() == 1);
  single.adjust(step_min);
  CHECK(single.value() == 1);

  for (const auto maximum : {0U, 1U, 4U, 5U, 9U, 10U, largest}) {
    ubc::Quantity q(maximum);
    CHECK(q.value() == (maximum == 0 ? 0U : 1U));
    for (const auto preset : {0U, 1U, 5U, 10U, largest}) {
      const auto previous = q.value();
      const bool expected = preset != 0 && preset <= maximum;
      CHECK(q.preset(preset) == expected);
      CHECK(q.value() == (expected ? preset : previous));
    }
    q.adjust(step_max);
    CHECK(q.value() <= maximum);
    q.adjust(step_min);
    CHECK(q.value() == (maximum == 0 ? 0U :
                        maximum == largest ? 2147483647U : 1U));
    q.adjust(step_min);
    CHECK(q.value() == (maximum == 0 ? 0U : 1U));
  }
}

void press_edge_tests() {
  ubc::PressEdge edge;
  CHECK(!edge.update(false));
  CHECK(edge.update(true));
  CHECK(!edge.update(true));
  CHECK(!edge.update(false));
  CHECK(edge.update(true));

  ubc::PressEdge opening_button(true);
  for (unsigned i = 0; i < 100; ++i) {
    CHECK(!opening_button.update(true));
  }
  CHECK(!opening_button.update(false));
  CHECK(opening_button.update(true));
  CHECK(!opening_button.update(true));
  opening_button.reset(true);
  CHECK(!opening_button.update(true));
  CHECK(!opening_button.update(false));
  opening_button.reset(false);
  CHECK(!opening_button.update(false));
  CHECK(opening_button.update(true));
  opening_button.reset(false);
  CHECK(opening_button.update(true));

  std::mt19937 random(0x2ba7c017U);
  bool previous = false;
  ubc::PressEdge random_edge;
  for (unsigned i = 0; i < 10000; ++i) {
    const bool down = (random() & 1U) != 0;
    CHECK(random_edge.update(down) == (down && !previous));
    previous = down;
  }
}

void pe_header_tests() {
  const auto valid = target_header();
  CHECK(matches(valid));
  CHECK(!ubc::matches_gothic2_header(nullptr, 0));
  CHECK(!ubc::matches_gothic2_header(nullptr, valid.size()));
  CHECK(!ubc::matches_gothic2_header(
      nullptr, std::numeric_limits<std::size_t>::max()));
  CHECK(!ubc::matches_gothic2_header(valid.data(), 0));

  // The declared optional header must be present in full, even
  // if every field required by this filter is already readable.
  for (std::size_t size = 0; size <= valid.size(); ++size) {
    // Exact-size allocations let ASan detect logical overreads
    // of a truncated file as well as access beyond 512 bytes.
    auto prefix = std::make_unique<std::uint8_t[]>(size);
    std::copy_n(valid.data(), size, prefix.get());
    CHECK(ubc::matches_gothic2_header(prefix.get(), size) == (size >= 376));
  }

  auto damaged = valid;
  damaged[0] = 'N';
  CHECK(!matches(damaged));
  damaged = valid;
  damaged[1] = 0;
  CHECK(!matches(damaged));
  damaged = valid;
  write32(damaged, 128, 0x50450000);
  CHECK(!matches(damaged));

  for (const auto offset : {0U, 1U, 63U, 489U, 512U, 513U, 0x7fffffffU,
                            0xffffffe8U, 0xfffffff0U, 0xffffffffU}) {
    damaged = valid;
    write32(damaged, 60, offset);
    CHECK(!matches(damaged));
  }
  for (const auto machine : {0U, 0x8664U, 0x01c4U}) {
    damaged = valid;
    write16(damaged, 132, static_cast<std::uint16_t>(machine));
    CHECK(!matches(damaged));
  }
  for (const auto magic : {0U, 0x0107U, 0x020bU}) {
    damaged = valid;
    write16(damaged, 152, static_cast<std::uint16_t>(magic));
    CHECK(!matches(damaged));
  }
  for (const auto optional_size : {0U, 1U, 59U, 361U, 65535U}) {
    damaged = valid;
    write16(damaged, 148, static_cast<std::uint16_t>(optional_size));
    CHECK(!matches(damaged));
  }
  for (const auto offset : {136U, 180U, 208U}) {
    damaged = valid;
    write32(damaged, offset, 0);
    CHECK(!matches(damaged));
  }

  auto minimal = target_header(128, 60);
  CHECK(ubc::matches_gothic2_header(minimal.data(), 212));
  CHECK(!ubc::matches_gothic2_header(minimal.data(), 211));
  damaged = target_header(128, 360);
  CHECK(matches(damaged));
  CHECK(!ubc::matches_gothic2_header(damaged.data(), 511));

  // Unaligned data and NT headers are valid. Read
  // bytes without assuming processor alignment.
  for (const auto pe_offset : {64U, 65U, 127U, 129U, 427U, 428U}) {
    const auto shifted = target_header(pe_offset, 60);
    CHECK(matches(shifted));
    const auto needed = static_cast<std::size_t>(pe_offset) + 24 + 60;
    CHECK(ubc::matches_gothic2_header(shifted.data(), needed));
    CHECK(!ubc::matches_gothic2_header(shifted.data(), needed - 1));
  }
  std::vector<std::uint8_t> unaligned(valid.size() + 1, 0);
  std::copy(valid.begin(), valid.end(), unaligned.begin() + 1);
  CHECK(ubc::matches_gothic2_header(unaligned.data() + 1, valid.size()));

  // Reject deterministic mutations of fixed identity fields.
  // Changes to unused bytes must not affect identity.
  const std::vector<std::size_t> fixed_bytes{
      0, 1, 128, 129, 130, 131, 132, 133, 136, 137, 138, 139,
      152, 153, 180, 181, 182, 183, 208, 209, 210, 211};
  std::mt19937 random(0x39510U);
  for (unsigned i = 0; i < 10000; ++i) {
    damaged = valid;
    const auto offset = fixed_bytes[random() % fixed_bytes.size()];
    damaged[offset] ^= static_cast<std::uint8_t>(1U + random() % 255U);
    CHECK(!matches(damaged));
    const auto size = static_cast<std::size_t>(random() % 513U);
    auto prefix = std::make_unique<std::uint8_t[]>(size);
    std::copy_n(damaged.data(), size, prefix.get());
    CHECK(!ubc::matches_gothic2_header(prefix.get(), size));

    damaged = valid;
    const auto declared_size = static_cast<std::uint16_t>(random());
    write16(damaged, 148, declared_size);
    CHECK(matches(damaged) == (declared_size >= 60 && declared_size <= 360));

    damaged = valid;
    const auto pe_offset = static_cast<std::uint32_t>(random());
    write32(damaged, 60, pe_offset);
    CHECK(matches(damaged) == (pe_offset == 128));

    damaged = valid;
    for (std::size_t j = 376; j < damaged.size(); ++j) {
      damaged[j] = static_cast<std::uint8_t>(random());
    }
    CHECK(matches(damaged));
  }

  // Even arbitrary unrelated short byte sequences must not
  // trigger bounds errors. Keep at least the DOS magic invalid.
  for (unsigned i = 0; i < 1000; ++i) {
    std::vector<std::uint8_t> noise(random() % 1025U);
    for (auto& byte : noise) {
      byte = static_cast<std::uint8_t>(random());
    }
    if (!noise.empty()) {
      noise[0] = 0;
    }
    CHECK(!matches(noise));
  }
}

void real_header_test(const char* path) {
  std::ifstream stream(path, std::ios::binary);
  CHECK(stream.is_open());
  if (!stream) {
    return;
  }
  std::vector<std::uint8_t> bytes(4096);
  stream.read(reinterpret_cast<char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
  bytes.resize(static_cast<std::size_t>(stream.gcount()));
  CHECK(matches(bytes));
  if (matches(bytes)) {
    std::cout << "Read-only executable header matched.\n";
  }
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 1 && !(argc == 3 && std::string(argv[1]) == "--exe")) {
    std::cerr << "Usage: core_tests [--exe path]\n";
    return 2;
  }
  quantity_tests();
  press_edge_tests();
  pe_header_tests();
  if (argc == 3) {
    real_header_test(argv[2]);
  }
  std::cout << checks << " checks, " << failures << " failures\n";
  return failures == 0 ? 0 : 1;
}
