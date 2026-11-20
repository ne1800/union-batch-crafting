// SPDX-License-Identifier: MIT
#include "ubc/stations/station_end_contract.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <vector>

namespace {
namespace e = ubc::station_end_contract;
unsigned checks = 0, failures = 0;
void check(bool ok, const char* message) {
  ++checks;
  if (!ok) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", message);
  }
}
struct Fixture {
  ubc::CraftingKey key{
      7, 11, 0x1000, 0x2000, 0x3000, 0x4000, ubc::crafting_catalog::Station::stove};
  ubc::StationEndCall call{};
  Fixture() {
    call.mob = key.station;
    call.npc = key.hero;
    call.return_address = e::completed_return;
    call.flag = 0;
    call.original_returned = call.original_path_verified = call.owner_thread_depth_verified = true;
    auto& b = call.before;
    b.read_stable = b.identity_known = b.station_known = b.station_owned =
        b.station_profile_verified = true;
    b.mob_fields_known = b.production_known = b.hands_known = true;
    b.thread = key.thread;
    b.world = key.world;
    b.hero = key.hero;
    b.station = key.station;
    b.kind = key.kind;
    b.state = b.target = b.rewind = b.production = 0;
    b.state_num = 1;
    b.hands.hands = {{0, -1, -1, 0}, {0, -1, -1, 0}};
    b.hands.interact_item_known = true;
    call.after = b;
    call.after.station = 0;
    call.after.station_owned = false;
    call.after.station_profile_verified = call.after.mob_fields_known = false;
    call.after.state = call.after.target = call.after.state_num = call.after.rewind = -1;
  }
};
void value_tests() {
  for (const auto station :
       {ubc::crafting_catalog::Station::stove, ubc::crafting_catalog::Station::alchemy,
        ubc::crafting_catalog::Station::leather, ubc::crafting_catalog::Station::saw,
        ubc::crafting_catalog::Station::forge, ubc::crafting_catalog::Station::anvil,
        ubc::crafting_catalog::Station::cool, ubc::crafting_catalog::Station::grind}) {
    Fixture f;
    f.key.kind = f.call.before.kind = station;
    check(ubc::verify_station_end(f.key, f.call).normal_exit_verified,
          "all five positively bound terminal station paths pass");
    for (unsigned bad = 0; bad < 35; ++bad) {
      auto b = f;
      switch (bad) {
      case 0:
        b.key.generation = 0;
        break;
      case 1:
        b.key.thread = 0;
        break;
      case 2:
        b.key.world = 0;
        break;
      case 3:
        b.key.hero = 0;
        break;
      case 4:
        b.key.station = 0;
        break;
      case 5:
        b.key.manager = 0;
        break;
      case 6:
        b.key.kind = static_cast<ubc::crafting_catalog::Station>(99);
        break;
      case 7:
        ++b.call.mob;
        break;
      case 8:
        ++b.call.npc;
        break;
      case 9:
        ++b.call.return_address;
        break;
      case 10:
        b.call.flag = 1;
        break;
      case 11:
        b.call.flag = -1;
        break;
      case 12:
        b.call.original_returned = false;
        break;
      case 13:
        b.call.original_path_verified = false;
        break;
      case 14:
        b.call.owner_thread_depth_verified = false;
        break;
      case 15:
        b.call.before.station_profile_verified = false;
        break;
      case 16:
        b.call.before.station_owned = false;
        break;
      case 17:
        b.call.before.mob_fields_known = false;
        break;
      case 18:
        b.call.before.state = 1;
        break;
      case 19:
        b.call.before.target = 1;
        break;
      case 20:
        b.call.before.state_num = 0;
        break;
      case 21:
        b.call.before.rewind = 1;
        break;
      case 22:
        b.call.before.station = 0;
        break;
      case 23:
        b.call.after.station = f.key.station;
        break;
      case 24:
        b.call.before.hands.interact_item_known = false;
        break;
      case 25:
        b.call.after.hands.interact_item_known = false;
        break;
      case 26:
        b.call.before.hands.interact_item = 0x8000;
        break;
      case 27:
        b.call.after.hands.interact_item = 0x8000;
        break;
      case 28:
        b.call.before.hands.hands.right = {1, 16604, 1, 0x8000};
        break;
      case 29:
        b.call.after.hands.hands.left = {2, -1, -1, 0x9000};
        break;
      case 30:
        b.call.before.hands.hands.left.state = -1;
        break;
      case 31:
        b.call.after.hands.hands.right.object = 0x8000;
        break;
      case 32:
        b.call.before.kind = static_cast<ubc::crafting_catalog::Station>(99);
        break;
      case 33:
        b.call.before.hands.hands.right.symbol = 16604;
        break;
      case 34:
        b.call.after.hands.hands.right.amount = 1;
        break;
      }
      check(!ubc::verify_station_end(b.key, b.call).normal_exit_verified,
            "missing or contradictory native fact never proves normal end");
    }
    for (bool after : {false, true})
      for (unsigned bad = 0; bad < 12; ++bad) {
        auto b = f;
        auto& p = after ? b.call.after : b.call.before;
        switch (bad) {
        case 0:
          p.read_stable = false;
          break;
        case 1:
          p.identity_known = false;
          break;
        case 2:
          p.station_known = false;
          break;
        case 3:
          p.production_known = false;
          break;
        case 4:
          p.hands_known = false;
          break;
        case 5:
          ++p.thread;
          break;
        case 6:
          ++p.world;
          break;
        case 7:
          ++p.hero;
          break;
        case 8:
          p.production = 1;
          break;
        case 9:
          p.hands.hands.left.state = 2;
          break;
        case 10:
          p.hands.hands.right.state = -1;
          break;
        case 11:
          p.hands.hands.left.object = 1;
          break;
        }
        check(!ubc::verify_station_end(b.key, b.call).normal_exit_verified,
              "both sides need stable current identity, empty hands, and production zero");
      }
    for (const auto caller :
         {0x71f3cbu, 0x71f4a5u, 0x71f52bu, 0x71f5a2u, 0x71d9c3u, 0x7220d7u, 0u}) {
      f.call.return_address = caller;
      check(!ubc::verify_station_end(f.key, f.call).normal_exit_verified,
            "request, Stop, AI and unknown return sites are not the checked completed caller");
    }
  }
  Fixture f;
  check(!ubc::verify_station_end({}, {}).normal_exit_verified,
        "zero initialized observations do not authorize");
  f.call.after.kind = static_cast<ubc::crafting_catalog::Station>(99);
  check(ubc::verify_station_end(f.key, f.call).normal_exit_verified,
        "after does not invent or dereference a detached station profile");
}
struct Region {
  std::uint32_t address;
  std::vector<std::uint8_t> bytes;
};
struct Memory {
  std::vector<Region> regions;
  std::function<void(Memory&, std::uint32_t)> before;
  std::uint32_t denied = 0;
  void add(std::uint32_t address, std::size_t size) {
    regions.push_back({address, std::vector<std::uint8_t>(size)});
  }
  std::uint8_t* at(std::uint32_t address, std::size_t size = 1) {
    for (auto& r : regions)
      if (address >= r.address && std::uint64_t{address} - r.address + size <= r.bytes.size())
        return r.bytes.data() + address - r.address;
    return nullptr;
  }
  void word(std::uint32_t address, std::uint32_t value) {
    auto* out = at(address, 4);
    if (!out)
      std::abort();
    for (unsigned i = 0; i < 4; ++i)
      out[i] = static_cast<std::uint8_t>(value >> (i * 8u));
  }
  static bool read(void* context, std::uint32_t address, void* out, std::size_t size) noexcept {
    auto& self = *static_cast<Memory*>(context);
    if (self.before)
      self.before(self, address);
    if (self.denied && e::detail::overlaps(address, size, self.denied, 1))
      return false;
    const auto* data = self.at(address, size);
    if (!data)
      return false;
    std::memcpy(out, data, size);
    return true;
  }
  ubc::ReadOnlyMemory reader() { return {this, read}; }
};
struct CodeFixture {
  Memory memory;
  e::InstalledEndHook hook{true, 0x10000000, 0x10001000};
  std::uint32_t vtable = 0x83d58c;
  CodeFixture() {
    memory.add(e::end_address, 717);
    memory.add(hook.trampoline, 11);
    for (const auto table : e::mob_vtables) {
      memory.add(table, 0x104);
      for (const auto slot : e::vtable_slots)
        memory.word(table + slot.offset, slot.function);
    }
    install();
  }
  void install() {
    auto* head = memory.at(e::end_address, 6);
    std::fill(head, head + 6, 0x90);
    e::detail::jump(head, e::end_address, hook.destination);
    auto* trampoline = memory.at(hook.trampoline, 11);
    std::memcpy(trampoline, e::end_prefix.data(), 6);
    e::detail::jump(trampoline + 6, hook.trampoline + 6, e::end_address + 6);
  }
};
void path_tests() {
  CodeFixture f;
  check(e::detail::verify_hook(f.memory.reader(), f.hook).valid,
        "own exactly installed E9/NOP and copied instruction plus return pass");
  for (unsigned bad = 0; bad < 11; ++bad) {
    auto b = f;
    switch (bad) {
    case 0:
      b.hook.installed = false;
      break;
    case 1:
      b.hook.destination = 0;
      break;
    case 2:
      b.hook.trampoline = 0;
      break;
    case 3:
      b.hook.trampoline = UINT32_MAX - 5;
      break;
    case 4:
      b.hook.destination = e::end_address;
      break;
    case 5:
      b.hook.destination = b.hook.trampoline + 6;
      break;
    case 6:
      b.hook.trampoline = 0x400000;
      break;
    case 7:
      *b.memory.at(e::end_address + 5) = 0xcc;
      break;
    case 8:
      *b.memory.at(e::end_address + 1) ^= 1;
      break;
    case 9:
      *b.memory.at(b.hook.trampoline + 1) ^= 1;
      break;
    case 10:
      *b.memory.at(b.hook.trampoline + 7) ^= 1;
      break;
    }
    check(!e::detail::verify_hook(b.memory.reader(), b.hook).valid,
          "foreign/overflowing/overlapping/wrong hook never normalizes code");
  }
  for (const auto table : e::mob_vtables)
    check(e::detail::verify_vtable(f.memory.reader(), table).valid,
          "supported native vtable retains each critical dispatch");
  for (const auto table : {0u, UINT32_MAX - 4, 0x83d58du, 0x10000000u})
    check(!e::detail::verify_vtable(f.memory.reader(), table).valid,
          "unknown vtables and integer wrap cannot authorize");
  for (const auto slot : e::vtable_slots) {
    auto b = f;
    b.memory.word(b.vtable + slot.offset, slot.function + 1);
    check(!e::detail::verify_vtable(b.memory.reader(), b.vtable).valid,
          "each changed virtual dispatch fails");
  }
  check(!e::verify_path(f.memory.reader(), f.vtable, f.hook).valid,
        "synthetic zeros are not an original native body");
  e::CodeRange synthetic{0x12000000, 16, {}};
  f.memory.add(synthetic.address, synthetic.length);
  for (unsigned i = 0; i < synthetic.length; ++i)
    *f.memory.at(synthetic.address + i) = static_cast<std::uint8_t>(i + 1);
  synthetic.sha256 =
      e::detail::digest(f.memory.at(synthetic.address, synthetic.length), synthetic.length);
  check(e::detail::verify_range(f.memory.reader(), synthetic, f.hook).valid,
        "bounded synthetic complete body matches its own hash");
  f.memory.denied = synthetic.address + synthetic.length - 1;
  check(!e::detail::verify_range(f.memory.reader(), synthetic, f.hook).valid,
        "last unreadable byte rejects whole body");
  f.memory.denied = 0;
  unsigned reads = 0;
  f.memory.before = [&](Memory& m, std::uint32_t a) {
    if (a == synthetic.address && ++reads == 2)
      *m.at(a + 1) ^= 1;
  };
  check(!e::detail::verify_range(f.memory.reader(), synthetic, f.hook).valid,
        "unstable repeated code read is rejected");
  f.memory.before = {};
  synthetic.length = 0;
  check(!e::detail::verify_range(f.memory.reader(), synthetic, f.hook).valid,
        "empty code range rejected");
  synthetic.length = e::max_code_range + 1;
  check(!e::detail::verify_range(f.memory.reader(), synthetic, f.hook).valid,
        "code bound is fixed");
  synthetic.address = UINT32_MAX - 1;
  synthetic.length = 4;
  check(!e::detail::verify_range(f.memory.reader(), synthetic, f.hook).valid,
        "range overflow rejected before reader");
}
bool load_engine(const char* path, std::vector<std::uint8_t>& bytes) {
  std::FILE* f = nullptr;
#if defined(_MSC_VER)
  if (fopen_s(&f, path, "rb"))
    return false;
#else
  f = std::fopen(path, "rb");
#endif
  if (!f)
    return false;
  bool good = std::fseek(f, 0, SEEK_END) == 0;
  const auto length = good ? std::ftell(f) : -1;
  good = length > 0 && length <= 16 * 1024 * 1024 && std::fseek(f, 0, SEEK_SET) == 0;
  if (good) {
    bytes.resize(static_cast<std::size_t>(length));
    good = std::fread(bytes.data(), 1, bytes.size(), f) == bytes.size();
  }
  std::fclose(f);
  return good &&
         e::detail::digest(bytes.data(), bytes.size()) == ubc::station_abi::executable_sha256;
}
bool copy_engine(const std::vector<std::uint8_t>& bytes, std::uint32_t address, void* out,
                 std::size_t length) {
  if (bytes.size() < 64 || address < 0x400000)
    return false;
  const auto pe = e::detail::u32(bytes.data() + 60);
  if (std::uint64_t{pe} + 24 > bytes.size())
    return false;
  const auto count = std::uint32_t{bytes[pe + 6]} | (std::uint32_t{bytes[pe + 7]} << 8u);
  const auto optional = std::uint32_t{bytes[pe + 20]} | (std::uint32_t{bytes[pe + 21]} << 8u);
  if (count > 32 || std::uint64_t{pe} + 24 + optional + count * 40 > bytes.size())
    return false;
  const auto rva = address - 0x400000;
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto* s = bytes.data() + pe + 24 + optional + i * 40;
    const auto va = e::detail::u32(s + 12), size = e::detail::u32(s + 16),
               raw = e::detail::u32(s + 20);
    if (rva >= va && std::uint64_t{rva} - va + length <= size &&
        std::uint64_t{raw} + rva - va + length <= bytes.size()) {
      std::memcpy(out, bytes.data() + raw + rva - va, length);
      return true;
    }
  }
  return false;
}
void engine_tests(const char* path) {
  std::vector<std::uint8_t> bytes;
  const bool loaded = load_engine(path, bytes);
  check(loaded, "optional EXE fixture requires full original SHA and read-only file access");
  if (!loaded)
    return;
  CodeFixture f;
  for (const auto& range : e::code_ranges) {
    if (!f.memory.at(range.address, range.length))
      f.memory.add(range.address, range.length);
    check(copy_engine(bytes, range.address, f.memory.at(range.address, range.length), range.length),
          "complete path range exists in pinned PE");
  }
  for (const auto table : e::mob_vtables)
    for (const auto slot : e::vtable_slots)
      check(copy_engine(bytes, table + slot.offset, f.memory.at(table + slot.offset, 4), 4),
            "actual virtual dispatch copied only into private host RAM");
  check(!e::verify_path(f.memory.reader(), f.vtable, f.hook).valid,
        "an uninstalled entry is not an owning-hook proof");
  f.install();
  for (const auto table : e::mob_vtables)
    check(e::verify_path(f.memory.reader(), table, f.hook).valid,
          "pinned complete native bodies with only exact own End hook pass");
  for (const auto& range : e::code_ranges) {
    auto* last = f.memory.at(range.address + range.length - 1);
    *last ^= 1;
    check(!e::verify_path(f.memory.reader(), f.vtable, f.hook).valid,
          "every changed critical function tail fails after install");
    *last ^= 1;
  }
  unsigned reads = 0;
  f.memory.before = [&](Memory& m, std::uint32_t a) {
    if (a == e::code_ranges.back().address && ++reads == 2)
      *m.at(e::code_ranges.front().address + 10) ^= 1;
  };
  check(!e::verify_path(f.memory.reader(), f.vtable, f.hook).valid,
        "second complete verification pass catches earlier code changed while reading later code");
}
}
int main(int argc, char** argv) {
  value_tests();
  path_tests();
  if (argc == 3 && !std::strcmp(argv[1], "--engine-READONLY"))
    engine_tests(argv[2]);
  else if (argc != 1) {
    std::fputs("usage: station_end_contract_tests [--engine-READONLY PATH]\n", stderr);
    return 2;
  }
  std::printf("StationEndContract: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
