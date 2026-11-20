// SPDX-License-Identifier: MIT
#include "ubc/engine/vob_locator_compat.hpp"
#include "ubc/stations/saw/ammo_compat_memory.hpp"
#include "ubc/stations/station_abi.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace {
namespace c = ubc::vob_locator_compat;
unsigned checks = 0, failures = 0;
void check(bool value, const char* message) {
  ++checks;
  if (!value) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", message);
  }
}
struct Segment {
  std::uint32_t address;
  std::vector<std::uint8_t> data;
};
struct Memory {
  std::vector<Segment> segments;
  std::function<void(Memory&, std::uint32_t, std::size_t)> before;
  std::uint32_t denied = 0;
  std::size_t calls = 0, largest = 0;
  std::uint8_t* at(std::uint32_t address, std::size_t size = 1) {
    for (auto& item : segments)
      if (address >= item.address &&
          std::uint64_t{address} - item.address + size <= item.data.size())
        return item.data.data() + (address - item.address);
    return nullptr;
  }
  void add(std::uint32_t address, std::size_t size) {
    if (!at(address, size))
      segments.push_back({address, std::vector<std::uint8_t>(size)});
  }
  void put(std::uint32_t address, std::uint32_t value) {
    add(address, 4);
    c::detail::put32(at(address, 4), value);
  }
  void byte(std::uint32_t address, std::uint8_t value) {
    add(address, 1);
    *at(address) = value;
  }
  static bool read(void* raw, std::uint32_t address, void* out, std::size_t size) noexcept {
    auto& self = *static_cast<Memory*>(raw);
    ++self.calls;
    self.largest = std::max(self.largest, size);
    if (self.before)
      self.before(self, address, size);
    if (self.denied && c::detail::overlap(address, size, self.denied, 1))
      return false;
    const auto* bytes = self.at(address, size);
    if (!bytes)
      return false;
    std::memcpy(out, bytes, size);
    return true;
  }
  ubc::ReadOnlyMemory reader() { return {this, read}; }
};
constexpr c::Modules modules{{{0x31000000, 0x32000000, 0x33000000, 0x34000000, 0x35000000}}};
constexpr std::array<std::uint32_t, 3> records{{0x20000, 0x21000, 0x22000}};
struct Binding {
  unsigned calls = 0, deny_call = 0, change_call = 0;
  static bool get(void* user, c::Modules& out) noexcept {
    auto& b = *static_cast<Binding*>(user);
    ++b.calls;
    if (b.calls == b.deny_call)
      return false;
    out = modules;
    if (b.calls == b.change_call)
      out.bases[0] += 0x10000;
    return true;
  }
};
std::uint32_t address(c::Module module, std::uint32_t rva) {
  return c::detail::base(modules, module) + rva;
}
void transfer(Memory& memory, std::uint32_t from, std::uint8_t opcode, std::uint32_t to,
              std::size_t length) {
  memory.add(from, length);
  auto* data = memory.at(from, length);
  std::memset(data, 0xcc, length);
  data[0] = opcode;
  c::detail::put32(data + 1, to - (from + 5u));
}
void choose_type_compare(Memory& memory, const c::TypeCompareProfile& profile) {
  const auto start = c::detail::base(modules, profile.module);
  transfer(memory, 0x7d0be9, 0xe8, start + profile.entry, 5);
  memory.put(start + profile.suffix_length, 18);
  memory.put(start + profile.suffix_pointer, start + profile.suffix);
}
void chain_fixture(Memory& memory) {
  const auto v = modules.bases[0], b = modules.bases[1], s = modules.bases[2], u = modules.bases[3],
             t = modules.bases[4];
  memory.put(c::manager_slot, v + 0x50fb0);
  const std::array<std::uint32_t, 3> slots{{v + 0x958e8, b + 0x5eaf0, s + 0x8a2e8}},
      iats{{v + 0x7b168, b + 0x4515c, s + 0x7017c}},
      callbacks{{v + 0x50fb0, b + 0x12410, s + 0x1bc00}};
  for (std::size_t i = 0; i < 3; ++i) {
    memory.put(slots[i], records[i]);
    memory.put(iats[i], u + 0x31aa0);
    memory.put(records[i] + 0xc, i < 2 ? records[i + 1] : 0);
    memory.put(records[i] + 0x14, c::original_on_message);
    memory.put(records[i] + 0x18, callbacks[i]);
  }
  memory.byte(b + 0x5dbc0, 0);
  memory.put(b + 0x5eb04, 2);
  memory.byte(b + 0x5eaf8, 2);
  memory.put(t + 0x8d9ec, 18);
  memory.put(t + 0x8da10, t + 0x7c6b0);
  choose_type_compare(memory, c::type_compare_profiles[0]);
  memory.put(0xab29bc, 0xab2b60);
  memory.put(0xab2b9c, 0x9a3698);
  memory.put(0x9a36d4, 0x8d8cd8);
  memory.put(0x8d8d14, 0);
}
void portable_tests() {
  {
    Memory m;
    m.put(c::manager_slot, c::original_on_message);
    m.put(0x16000, 0x87654321);
    Binding b;
    ubc::ammo_compat_memory::Reader adapter(m.reader(), &b, Binding::get);
    auto reader = adapter.memory();
    std::uint32_t value = 0;
    check(reader.copy(c::manager_slot, &value, 4) && value == c::original_on_message &&
              b.calls == 0,
          "original slot requires no foreign module enrollment");
    check(reader.copy(0x16000, &value, 4) && value == 0x87654321,
          "unrelated data is copied unchanged");
    for (const auto size : {1u, 2u, 3u, 5u})
      check(!reader.copy(c::manager_slot, &value, size),
            "partial or oversized slot read never normalizes");
    check(adapter.diagnostic().issue == ubc::ammo_compat_memory::Issue::slot_shape &&
              adapter.diagnostic().address == c::manager_slot,
          "diagnostic identifies rejected slot shape");
    m.put(c::manager_slot, modules.bases[0] + 0x50fb0);
    check(!reader.copy(c::manager_slot, &value, 4),
          "known destination without full code closure rejects");
    m.put(c::manager_slot, c::original_on_message);
    unsigned hits = 0;
    m.before = [&](Memory& x, std::uint32_t at, std::size_t) {
      if (at == c::manager_slot && ++hits == 2)
        x.put(at, 0xdeadbeef);
    };
    check(!reader.copy(c::manager_slot, &value, 4),
          "even original slot must remain stable across the reader");
    check(adapter.diagnostic().issue == ubc::ammo_compat_memory::Issue::slot_changed,
          "diagnostic preserves exact changed-slot reason");
  }
  check(c::detail::modules_valid(modules) == c::Issue::none, "separate aligned module images");
  for (std::size_t i = 0; i < c::module_count; ++i) {
    for (const auto bad :
         {0u, 1u, 0xffff0000u, c::engine_base, modules.bases[(i + 1) % c::module_count]}) {
      auto x = modules;
      x.bases[i] = bad;
      Memory m;
      const auto result = c::verify_print_forwarding(m.reader(), x);
      check(!result.valid && m.calls == 0, "bad module addresses rejected before reading");
    }
  }
  {
    Memory m;
    check(!c::verify_print_forwarding(m.reader(), modules).valid,
          "synthetic empty memory never proves native code");
  }
  {
    Memory m;
    chain_fixture(m);
    c::detail::Chain out{};
    check(c::detail::chain(m.reader(), modules, out) == c::Issue::none,
          "three distinct records resolve to original engine entry");
    check(out.records == records && out.next[2] == 0 && out.fallback[2] == c::original_on_message,
          "terminal record uses original fallback");
    check(!c::verify_print_forwarding(m.reader(), modules).valid,
          "pure chain proof alone is not complete native approval");
  }
  for (const auto& profile : c::type_compare_profiles) {
    Memory original;
    chain_fixture(original);
    choose_type_compare(original, profile);
    c::detail::Chain out{};
    const auto start = c::detail::base(modules, profile.module);
    check(c::detail::chain(original.reader(), modules, out) == c::Issue::none &&
              out.type_compare == start + profile.entry,
          "active exact type adapter owns the suffix globals");
    for (const auto field : {start + profile.suffix_length, start + profile.suffix_pointer}) {
      for (const auto value : {0u, 1u, 0xdeadbeefu}) {
        auto m = original;
        m.put(field, value);
        check(c::detail::chain(m.reader(), modules, out) == c::Issue::type_contract,
              "active adapter globals cannot borrow another provider's proof");
      }
      auto m = original;
      m.denied = field;
      check(c::detail::chain(m.reader(), modules, out) == c::Issue::unreadable,
            "missing active adapter globals reject");
    }
    for (const auto target : {start + profile.entry + 1, start + profile.suffix, 0u}) {
      auto m = original;
      transfer(m, 0x7d0be9, 0xe8, target, 5);
      check(c::detail::chain(m.reader(), modules, out) == c::Issue::transfer,
            "unknown type helper is not supported by module identity alone");
    }
    auto m = original;
    transfer(m, 0x7d0be9, 0xe9, start + profile.entry, 5);
    check(c::detail::chain(m.reader(), modules, out) == c::Issue::transfer,
          "type helper must be a CALL with the evidenced ABI");
    m = original;
    m.denied = 0x7d0be9;
    check(c::detail::chain(m.reader(), modules, out) == c::Issue::unreadable,
          "unreadable adapter selection rejects");
  }
  const std::array<std::uint32_t, 24> changed_words{{c::manager_slot,
                                                     modules.bases[0] + 0x958e8,
                                                     modules.bases[1] + 0x5eaf0,
                                                     modules.bases[2] + 0x8a2e8,
                                                     modules.bases[0] + 0x7b168,
                                                     modules.bases[1] + 0x4515c,
                                                     modules.bases[2] + 0x7017c,
                                                     records[0] + 0xc,
                                                     records[1] + 0xc,
                                                     records[2] + 0xc,
                                                     records[0] + 0x14,
                                                     records[1] + 0x14,
                                                     records[2] + 0x14,
                                                     records[0] + 0x18,
                                                     records[1] + 0x18,
                                                     records[2] + 0x18,
                                                     modules.bases[1] + 0x5eb04,
                                                     modules.bases[4] + 0x8d9ec,
                                                     modules.bases[4] + 0x8da10,
                                                     0xab29bc,
                                                     0xab2b9c,
                                                     0x9a36d4,
                                                     0x8d8d14,
                                                     modules.bases[0] + 0x958e8}};
  for (const auto field : changed_words) {
    for (const auto value : {0u, 1u, 0xdeadbeefu}) {
      Memory m;
      chain_fixture(m);
      const auto old = c::detail::u32(m.at(field, 4));
      if (value == old)
        continue;
      m.put(field, value);
      c::detail::Chain out{};
      check(c::detail::chain(m.reader(), modules, out) != c::Issue::none,
            "unknown or altered chain field rejects");
    }
    Memory m;
    chain_fixture(m);
    m.denied = field;
    c::detail::Chain out{};
    check(c::detail::chain(m.reader(), modules, out) != c::Issue::none,
          "unreadable chain field is not zero");
  }
  for (const auto field : {modules.bases[1] + 0x5dbc0, modules.bases[1] + 0x5eaf8}) {
    for (const auto value : {std::uint8_t{1}, std::uint8_t{3}, std::uint8_t{255}}) {
      Memory m;
      chain_fixture(m);
      m.byte(field, value);
      c::detail::Chain out{};
      check(c::detail::chain(m.reader(), modules, out) == c::Issue::filter,
            "active filter or unsupported framework hook rejects");
    }
  }
  for (const auto record :
       {records[0], records[0] + 4, c::engine_base, modules.bases[0], 0xfffffffcU}) {
    Memory m;
    chain_fixture(m);
    m.put(modules.bases[1] + 0x5eaf0, record);
    c::detail::Chain out{};
    check(c::detail::chain(m.reader(), modules, out) == c::Issue::alias,
          "overlapping, image-owned or overflowing record rejected before dereference");
  }
  {
    Memory m;
    chain_fixture(m);
    unsigned hits = 0;
    m.before = [&](Memory& x, std::uint32_t at, std::size_t) {
      if (at == records[0] + 0xc && ++hits == 2)
        x.put(at, records[2]);
    };
    c::detail::Chain out{};
    check(c::detail::chain(m.reader(), modules, out) == c::Issue::changed,
          "second linked-record read cannot hide mutation");
  }
  for (const std::size_t length : {5u, 7u}) {
    Memory m;
    transfer(m, 0x11000, 0xe9, 0x17000, length);
    check(c::detail::transfer(m.reader(), 0x11000, 0xe9, 0x17000, length) == c::Issue::none,
          "exact explicit transfer");
    for (std::size_t i = 0; i < length; ++i) {
      auto x = m;
      *x.at(0x11000 + static_cast<std::uint32_t>(i)) ^= 1;
      check(c::detail::transfer(x.reader(), 0x11000, 0xe9, 0x17000, length) == c::Issue::transfer,
            "every transfer byte belongs to fixed destination and padding");
    }
    check(c::detail::transfer(m.reader(), 0x11000, 0xe8, 0x17000, length) == c::Issue::transfer,
          "CALL never accepted as JMP");
    check(c::detail::transfer(m.reader(), 0x11000, 0xe9, 0x17001, length) == c::Issue::transfer,
          "unknown foreign target not enrolled");
  }
  {
    Memory m;
    constexpr std::uint32_t where = 0x35002000;
    std::array<std::uint8_t, 64> own{};
    for (std::size_t i = 0; i < own.size(); ++i)
      own[i] = static_cast<std::uint8_t>(i * 3);
    c::detail::put32(own.data() + 4, c::preferred_base + 0x1234);
    c::detail::put32(own.data() + 20, c::preferred_base + 0x4321);
    c::Region region{c::Module::torch, 0x2000, 64, c::detail::digest(own.data(), own.size()),
                     {{4, 20}},        2};
    m.add(where, own.size());
    std::memcpy(m.at(where, own.size()), own.data(), own.size());
    for (const auto offset : {4u, 20u})
      c::detail::put32(m.at(where + offset, 4), c::detail::u32(own.data() + offset) +
                                                    (modules.bases[4] - c::preferred_base));
    check(c::detail::region(m.reader(), modules, region) == c::Issue::none,
          "only declared PE HIGHLOW positions are normalized");
    for (std::uint32_t i = 0; i < 64; ++i) {
      auto x = m;
      *x.at(where + i) ^= 1;
      check(c::detail::region(x.reader(), modules, region) == c::Issue::code,
            "changed relocation or code byte rejects");
    }
    for (unsigned scenario = 0; scenario < 7; ++scenario) {
      auto r = region;
      switch (scenario) {
      case 0:
        r.length = 0;
        break;
      case 1:
        r.length = 1025;
        break;
      case 2:
        r.relocation_count = 33;
        break;
      case 3:
        r.relocations[1] = 63;
        break;
      case 4:
        r.relocations[1] = 6;
        break;
      case 5:
        r.rva = 0xfffffff0;
        break;
      case 6:
        r.module = static_cast<c::Module>(6);
        break;
      }
      check(c::detail::region(m.reader(), modules, r) != c::Issue::none,
            "malformed generic range metadata rejected");
    }
    auto x = m;
    unsigned hits = 0;
    x.before = [&](Memory& y, std::uint32_t at, std::size_t) {
      if (at == where && ++hits == 2)
        *y.at(at) ^= 1;
    };
    check(c::detail::region(x.reader(), modules, region) == c::Issue::changed,
          "code changes between reads reject");
  }
  {
    Memory m;
    std::array<std::uint8_t, 19> suffix{};
    suffix.fill('q');
    suffix.back() = 0;
    constexpr std::uint32_t name = 0x18000;
    std::array<std::uint8_t, 32> text{};
    text.fill('a');
    text.back() = 0;
    m.add(name, text.size());
    std::memcpy(m.at(name, text.size()), text.data(), text.size());
    check(c::detail::safe_type_name(m.reader(), name, text.size(),
                                    c::detail::digest(text.data(), text.size()),
                                    suffix) == c::Issue::none,
          "canonical names require no mutation");
    std::memcpy(text.data() + text.size() - suffix.size(), suffix.data(), suffix.size());
    std::memcpy(m.at(name, text.size()), text.data(), text.size());
    check(c::detail::safe_type_name(m.reader(), name, text.size(),
                                    c::detail::digest(text.data(), text.size()),
                                    suffix) == c::Issue::type_contract,
          "even hash-known name needing normalization is rejected");
    text[1] = 0;
    std::memcpy(m.at(name, text.size()), text.data(), text.size());
    check(c::detail::safe_type_name(m.reader(), name, text.size(),
                                    c::detail::digest(text.data(), text.size()),
                                    suffix) == c::Issue::type_contract,
          "embedded terminator cannot alter cast branch");
  }
}

struct PE {
  std::vector<std::uint8_t> bytes;
  std::uint32_t nt = 0, optional = 0;
  std::uint16_t sections = 0, optional_size = 0;
  bool load(const char* path, const ubc::lhiver::Digest& expected) {
    std::FILE* file = nullptr;
#ifdef _MSC_VER
    if (fopen_s(&file, path, "rb"))
      return false;
#else
    file = std::fopen(path, "rb");
#endif
    if (!file)
      return false;
    const bool seek = std::fseek(file, 0, SEEK_END) == 0;
    const auto length = seek ? std::ftell(file) : -1;
    if (length <= 64 || length > 16 * 1024 * 1024 || std::fseek(file, 0, SEEK_SET)) {
      std::fclose(file);
      return false;
    }
    bytes.resize(static_cast<std::size_t>(length));
    const auto got = std::fread(bytes.data(), 1, bytes.size(), file);
    std::fclose(file);
    if (got != bytes.size() || c::detail::digest(bytes.data(), bytes.size()) != expected)
      return false;
    nt = c::detail::u32(bytes.data() + 60);
    if (std::uint64_t{nt} + 24 > bytes.size())
      return false;
    optional = nt + 24;
    sections = static_cast<std::uint16_t>(bytes[nt + 6] | (std::uint16_t{bytes[nt + 7]} << 8u));
    optional_size =
        static_cast<std::uint16_t>(bytes[nt + 20] | (std::uint16_t{bytes[nt + 21]} << 8u));
    return sections <= 32 &&
           std::uint64_t{optional} + optional_size + 40u * sections <= bytes.size();
  }
  const std::uint8_t* at(std::uint32_t rva, std::size_t length) const {
    if (rva < 1024 && std::uint64_t{rva} + length <= 1024)
      return bytes.data() + rva;
    for (std::size_t i = 0; i < sections; ++i) {
      const auto* section = bytes.data() + optional + optional_size + 40 * i;
      const auto va = c::detail::u32(section + 12), size = c::detail::u32(section + 16),
                 offset = c::detail::u32(section + 20);
      if (rva >= va && std::uint64_t{rva} - va + length <= size &&
          std::uint64_t{offset} + rva - va + length <= bytes.size())
        return bytes.data() + offset + rva - va;
    }
    return nullptr;
  }
};
bool populate(Memory& memory, const std::array<PE, 6>& files) {
  for (std::size_t i = 0; i < c::module_count; ++i) {
    const auto& p = c::module_profiles[i];
    memory.add(modules.bases[i], p.header_length);
    std::memcpy(memory.at(modules.bases[i], p.header_length), files[i + 1].bytes.data(),
                p.header_length);
    memory.put(modules.bases[i] + p.image_base_offset, modules.bases[i]);
  }
  for (const auto& r : c::regions) {
    const auto index = r.module == c::Module::engine ? 0 : static_cast<std::size_t>(r.module) + 1;
    const auto* source = files[index].at(r.rva, r.length);
    if (!source)
      return false;
    const auto start = address(r.module, r.rva);
    memory.add(start, r.length);
    std::memcpy(memory.at(start, r.length), source, r.length);
    const auto delta =
        r.module == c::Module::engine ? 0u : c::detail::base(modules, r.module) - c::preferred_base;
    for (std::size_t i = 0; i < r.relocation_count; ++i) {
      const auto at = start + r.relocations[i];
      memory.put(at, c::detail::u32(memory.at(at, 4)) + delta);
    }
  }
  for (const auto& type : c::canonical_types) {
    const auto* source = files[0].at(type.engine_rva, type.length);
    if (!source)
      return false;
    const auto at = address(type.module, type.rva);
    memory.add(at, type.length);
    std::memcpy(memory.at(at, type.length), source, type.length);
  }
  transfer(memory, address(c::Module::vob, 0x5757c), 0xe9, c::engine_dynamic_cast, 7);
  transfer(memory, address(c::Module::stats, 0x3fc07), 0xe9, c::engine_dynamic_cast, 7);
  transfer(memory, 0x7d0be9, 0xe8, address(c::Module::torch, 0x8e30), 5);
  chain_fixture(memory);
  return true;
}
void file_tests(char** argv) {
  std::array<PE, 6> files;
  if (!files[0].load(argv[2], ubc::station_abi::executable_sha256)) {
    check(false, "exact local engine fixture");
    return;
  }
  for (std::size_t i = 0; i < c::module_count; ++i)
    if (!files[i + 1].load(argv[i + 3], c::module_profiles[i].file_sha256)) {
      check(false, "exact local module fixture");
      return;
    }
  Memory original;
  if (!populate(original, files)) {
    check(false, "all bounded fixture sections");
    return;
  }
  const auto good = c::verify_print_forwarding(original.reader(), modules);
  check(good.valid && good.issue == c::Issue::none && good.version == 2 &&
            good.on_message == modules.bases[0] + 0x50fb0,
        "full production verifier accepts exact local files plus authored runtime records");
  check(original.largest <= 1024 && original.calls < 512,
        "full proof stays within fixed read bounds");
  for (const auto& profile : c::type_compare_profiles) {
    auto m = original;
    choose_type_compare(m, profile);
    m.calls = 0;
    const auto result = c::verify_print_forwarding(m.reader(), modules);
    check(result.valid && result.version == 2 && m.largest <= 1024 && m.calls < 512,
          "both fully hash-bound RTTI providers accept within the unchanged read budget");
    Binding binding;
    ubc::ammo_compat_memory::Reader adapter(m.reader(), &binding, Binding::get);
    std::uint32_t value = 0;
    check(
        adapter.memory().copy(c::manager_slot, &value, 4) && value == c::original_on_message,
        "production adapter permits the selected exact provider without rewriting the native slot");
    const auto at = address(profile.module, profile.suffix_pointer);
    const auto& other = c::type_compare_profiles[profile.module == c::Module::torch ? 1 : 0];
    m.put(at, address(other.module, other.suffix));
    check(
        !c::verify_print_forwarding(m.reader(), modules).valid,
        "even identical suffix bytes in the other module cannot replace the owned global binding");
  }
  for (unsigned trigger : {3u, 5u, 7u}) {
    auto m = original;
    unsigned hits = 0;
    m.before = [&](Memory& x, std::uint32_t at, std::size_t) {
      if (at == 0x7d0be9 && ++hits == trigger)
        choose_type_compare(x, c::type_compare_profiles[1]);
    };
    const auto result = c::verify_print_forwarding(m.reader(), modules);
    check(!result.valid && result.issue == c::Issue::changed,
          "two valid providers cannot be exchanged between closure and chain observations");
  }
  for (unsigned scenario = 0; scenario < 7; ++scenario) {
    auto m = original;
    Binding binding;
    if (scenario == 1)
      binding.deny_call = 1;
    if (scenario == 2)
      binding.deny_call = 2;
    if (scenario == 3)
      binding.change_call = 2;
    if (scenario == 4)
      m.byte(modules.bases[1] + 0x5dbc0, 1);
    if (scenario == 5)
      m.put(records[2] + 0x14, 0xdeadbeef);
    if (scenario == 6)
      *m.at(address(c::regions[0].module, c::regions[0].rva)) ^= 1;
    ubc::ammo_compat_memory::Reader adapter(m.reader(), &binding, Binding::get);
    std::uint32_t value = 0;
    const auto accepted = adapter.memory().copy(c::manager_slot, &value, 4);
    check(accepted == (scenario == 0),
          "read adapter requires both module bindings and complete fresh closure");
    using AdapterIssue = ubc::ammo_compat_memory::Issue;
    const auto expected = scenario == 0   ? AdapterIssue::none
                          : scenario == 1 ? AdapterIssue::first_binding
                          : scenario == 2 ? AdapterIssue::last_binding
                          : scenario == 3 ? AdapterIssue::binding_changed
                                          : AdapterIssue::forwarding;
    check(adapter.diagnostic().issue == expected,
          "diagnostic distinguishes original-contract failure stage");
    if (scenario >= 4)
      check(adapter.diagnostic().compatibility != c::Issue::none && adapter.diagnostic().stage,
            "nested forwarding diagnostic includes exact failed native stage");
    if (accepted) {
      check(value == c::original_on_message, "only proof copy contains original dispatch");
      check(c::detail::u32(m.at(c::manager_slot, 4)) == modules.bases[0] + 0x50fb0,
            "live foreign slot remains unchanged");
      m.byte(modules.bases[1] + 0x5dbc0, 1);
      check(!adapter.memory().copy(c::manager_slot, &value, 4),
            "later changed filter cannot reuse earlier positive proof");
    }
  }
  for (std::size_t i = 0; i < c::module_count; ++i) {
    for (const auto bad : {std::uint32_t{0}, std::uint32_t{1}, c::preferred_base + 1}) {
      auto m = original;
      m.put(modules.bases[i] + c::module_profiles[i].image_base_offset, bad);
      check(!c::verify_print_forwarding(m.reader(), modules).valid,
            "unknown loaded image base rejects");
    }
    auto m = original;
    m.put(modules.bases[i] + c::module_profiles[i].image_base_offset, c::preferred_base);
    check(c::verify_print_forwarding(m.reader(), modules).valid,
          "unmodified Windows preferred-base header accepted");
    m = original;
    *m.at(modules.bases[i] + 8) ^= 1;
    check(!c::verify_print_forwarding(m.reader(), modules).valid,
          "other PE header byte is never normalized");
  }
  for (const auto& r : c::regions) {
    const auto at = address(r.module, r.rva);
    for (const auto offset : {0u, r.length - 1}) {
      auto m = original;
      *m.at(at + offset) ^= 1;
      check(!c::verify_print_forwarding(m.reader(), modules).valid,
            "every native region is required including its last byte");
    }
    for (std::size_t i = 0; i < r.relocation_count; ++i) {
      auto m = original;
      *m.at(at + r.relocations[i]) ^= 1;
      check(!c::verify_print_forwarding(m.reader(), modules).valid,
            "a changed PE-relocated operand is not hidden");
    }
    auto m = original;
    m.denied = at;
    check(!c::verify_print_forwarding(m.reader(), modules).valid, "unknown native region rejects");
  }
  for (const auto& type : c::canonical_types) {
    auto m = original;
    *m.at(address(type.module, type.rva)) ^= 1;
    check(!c::verify_print_forwarding(m.reader(), modules).valid,
          "different canonical target type rejects");
  }
  for (const auto site :
       {address(c::Module::vob, 0x5757c), address(c::Module::stats, 0x3fc07), 0x7d0be9u}) {
    for (std::uint32_t offset = 0; offset < (site == 0x7d0be9u ? 5u : 7u); ++offset) {
      auto m = original;
      *m.at(site + offset) ^= 1;
      check(!c::verify_print_forwarding(m.reader(), modules).valid,
            "every explicit foreign patch byte is required");
    }
  }
  {
    auto m = original;
    unsigned hits = 0;
    const auto field = records[0] + 0xc;
    m.before = [&](Memory& x, std::uint32_t at, std::size_t) {
      if (at == field && ++hits == 3)
        x.put(at, records[2]);
    };
    check(!c::verify_print_forwarding(m.reader(), modules).valid,
          "late record rewrite between complete passes rejects");
  }
  {
    auto m = original;
    unsigned hits = 0;
    const auto field = modules.bases[1] + 0x5dbc0;
    m.before = [&](Memory& x, std::uint32_t at, std::size_t) {
      if (at == field && ++hits == 3)
        x.byte(at, 1);
    };
    check(!c::verify_print_forwarding(m.reader(), modules).valid,
          "late activated message filter rejects");
  }
  {
    auto m = original;
    unsigned hits = 0;
    const auto& r = c::regions[0];
    const auto field = address(r.module, r.rva);
    m.before = [&](Memory& x, std::uint32_t at, std::size_t) {
      if (at == field && ++hits == 3)
        *x.at(at) ^= 1;
    };
    check(!c::verify_print_forwarding(m.reader(), modules).valid,
          "late changed foreign callback rejects");
  }
  {
    auto m = original;
    unsigned hits = 0;
    m.before = [&](Memory& x, std::uint32_t at, std::size_t) {
      if (at != modules.bases[0] || ++hits != 3)
        return;
      const std::array<std::uint32_t, 3> slots{
          {modules.bases[0] + 0x958e8, modules.bases[1] + 0x5eaf0, modules.bases[2] + 0x8a2e8}},
          callbacks{
              {modules.bases[0] + 0x50fb0, modules.bases[1] + 0x12410, modules.bases[2] + 0x1bc00}};
      for (std::size_t i = 0; i < 3; ++i) {
        const auto replacement = records[i] + 0x100000;
        x.put(slots[i], replacement);
        x.put(replacement + 0xc, i < 2 ? records[i + 1] + 0x100000 : 0);
        x.put(replacement + 0x14, c::original_on_message);
        x.put(replacement + 0x18, callbacks[i]);
      }
    };
    const auto result = c::verify_print_forwarding(m.reader(), modules);
    check(!result.valid && result.issue == c::Issue::changed && result.stage == 302,
          "two individually valid but differently owned chains are not one stable proof");
  }
  // Full code and file hashes come from read-only local PE files. The process
  // base and records are our own values. This is not world-run evidence.
}
}
int main(int argc, char** argv) {
  portable_tests();
  if (argc == 8 && std::string(argv[1]) == "--files")
    file_tests(argv);
  else if (argc != 1) {
    std::fprintf(stderr, "usage: test [--files engine vob bug stats union torch]\n");
    return 2;
  }
  std::printf("%u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
