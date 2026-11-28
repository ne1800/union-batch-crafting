// SPDX-License-Identifier: MIT
#include "ubc/engine/zbug_end_compat.hpp"
#include "ubc/stations/grind/grind_inventory_entry.hpp"
#include "ubc/stations/cool/cool_set_interact_path.hpp"
#include "ubc/stations/cool/cool_native_path.hpp"
#include "ubc/stations/station_end_contract.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <vector>

namespace {
namespace z = ubc::zbug_end_compat;
unsigned checks = 0, failures = 0;
void check(bool ok, const char* text) {
  ++checks;
  if (!ok) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", text);
  }
}
struct Region {
  std::uint32_t address;
  std::vector<std::uint8_t> bytes;
};
struct Memory {
  std::vector<Region> regions;
  std::function<void(Memory&, std::uint32_t)> before;
  std::uint32_t denied = 0;
  std::size_t reads = 0;
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
    auto* b = at(address, 4);
    if (!b)
      std::abort();
    for (unsigned i = 0; i < 4; ++i)
      b[i] = static_cast<std::uint8_t>(value >> (i * 8u));
  }
  static bool read(void* context, std::uint32_t address, void* out, std::size_t size) noexcept {
    auto& m = *static_cast<Memory*>(context);
    ++m.reads;
    if (m.before)
      m.before(m, address);
    if (m.denied && std::uint64_t{address} <= m.denied && std::uint64_t{address} + size > m.denied)
      return false;
    const auto* b = m.at(address, size);
    if (!b)
      return false;
    std::memcpy(out, b, size);
    return true;
  }
  ubc::ReadOnlyMemory reader() { return {this, read}; }
};
z::Binding binding() {
  return {{0x20000000, z::zbug_image_size, z::zbug_file_sha256, true},
          {0x30000000, z::union_image_size, z::union_file_sha256, true}};
}
void synthetic_tests() {
  Memory m;
  auto b = binding();
  const auto missing = ubc::cool_set_interact_path::verify(m.reader());
  check(!missing.valid && missing.issue == ubc::cool_set_interact_path::Issue::unreadable &&
            missing.address == ubc::cool_set_interact_path::body.address &&
            missing.route == ubc::cool_set_interact_path::Route::none,
        "SetInteract needs actual full-body reads");
  check(!z::verify(m.reader(), {}).valid, "no inferred module identity");
  for (unsigned i = 0; i < 12; ++i) {
    auto bad = b;
    switch (i) {
    case 0:
      bad.zbug.mapping_verified = false;
      break;
    case 1:
      bad.union_core.mapping_verified = false;
      break;
    case 2:
      bad.zbug.file_sha256[0] ^= 1;
      break;
    case 3:
      bad.union_core.file_sha256[0] ^= 1;
      break;
    case 4:
      --bad.zbug.image_size;
      break;
    case 5:
      ++bad.union_core.image_size;
      break;
    case 6:
      bad.zbug.base = 0;
      break;
    case 7:
      bad.union_core.base = UINT32_MAX - 64;
      break;
    case 8:
      bad.zbug.base = 0x400000;
      break;
    case 9:
      bad.union_core.base = bad.zbug.base;
      break;
    case 10:
      bad.zbug.base = UINT32_MAX - 64;
      break;
    case 11:
      bad.union_core.base = 0x400000;
      break;
    }
    const auto previous = m.reads;
    check(!z::verify(m.reader(), bad).valid, "wrong module identity/size/overlap cannot authorize");
    check(m.reads == previous, "invalid binding is rejected before memory reads");
  }
  constexpr std::array<std::uint16_t, 1> reloc{{4}};
  z::CodeRange area{0x100, 16, {}, reloc.data(), reloc.size()};
  m.add(b.zbug.base + area.rva, area.length);
  auto* code = m.at(b.zbug.base + area.rva, area.length);
  for (unsigned i = 0; i < area.length; ++i)
    code[i] = static_cast<std::uint8_t>(i + 1);
  m.word(b.zbug.base + area.rva + 4, z::preferred_base + 0x42);
  area.sha256 = z::detail::digest(code, area.length);
  m.word(b.zbug.base + area.rva + 4, b.zbug.base + 0x42);
  check(z::detail::verify_code(m.reader(), b.zbug, area).valid,
        "only exact listed HIGHLOW relocations normalize");
  m.word(b.zbug.base + area.rva + 4, b.zbug.base + 0x43);
  check(!z::detail::verify_code(m.reader(), b.zbug, area).valid,
        "changed relocated operand still fails hash");
  m.word(b.zbug.base + area.rva + 4, b.zbug.base + 0x42);
  *m.at(b.zbug.base + area.rva + 15) ^= 1;
  check(!z::detail::verify_code(m.reader(), b.zbug, area).valid, "last body byte is covered");
  *m.at(b.zbug.base + area.rva + 15) ^= 1;
  m.denied = b.zbug.base + area.rva + 15;
  check(!z::detail::verify_code(m.reader(), b.zbug, area).valid, "truncated code fails");
  m.denied = 0;
  unsigned copies = 0;
  m.before = [&](Memory& mm, std::uint32_t address) {
    if (address == b.zbug.base + area.rva && ++copies == 2)
      *mm.at(address) ^= 1;
  };
  check(!z::detail::verify_code(m.reader(), b.zbug, area).valid,
        "changed second copy fails before normalization");
  m.before = {};
  area.length = 0;
  check(!z::detail::verify_code(m.reader(), b.zbug, area).valid, "empty range fails");
  area.length = z::max_code_size + 1;
  check(!z::detail::verify_code(m.reader(), b.zbug, area).valid, "bounded code buffer enforced");
  area.length = 16;
  area.rva = b.zbug.image_size - 8;
  check(!z::detail::verify_code(m.reader(), b.zbug, area).valid, "cross-image code fails");
}
std::vector<std::uint8_t> read_file(const char* path) {
  std::FILE* file = nullptr;
#if defined(_MSC_VER)
  if (fopen_s(&file, path, "rb"))
    return {};
#else
  file = std::fopen(path, "rb");
#endif
  if (!file)
    return {};
  if (std::fseek(file, 0, SEEK_END)) {
    std::fclose(file);
    return {};
  }
  const auto length = std::ftell(file);
  if (length <= 0 || length > 16 * 1024 * 1024 || std::fseek(file, 0, SEEK_SET)) {
    std::fclose(file);
    return {};
  }
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
  const auto read = std::fread(bytes.data(), 1, bytes.size(), file);
  std::fclose(file);
  if (read != bytes.size())
    bytes.clear();
  return bytes;
}
// Optionally read local original files into RAM. The repository contains no
// game or plugin code blobs. All runtime objects and calls are synthetic. This
// test starts neither the DLL nor the game and does not establish live behavior.
bool load_pe(Memory& m, const std::vector<std::uint8_t>& file, std::uint32_t base,
             std::uint32_t expected_size) {
  const auto contains = [&](std::size_t at, std::size_t n) {
    return at <= file.size() && n <= file.size() - at;
  };
  if (!contains(0, 64) || file[0] != 'M' || file[1] != 'Z')
    return false;
  const auto pe = z::detail::u32(file.data() + 60);
  if (!contains(pe, 248) || z::detail::u32(file.data() + pe) != 0x4550)
    return false;
  const auto optional = std::size_t{pe} + 24;
  if (z::detail::u16(file.data() + optional) != 0x10b ||
      z::detail::u32(file.data() + optional + 56) != expected_size)
    return false;
  const auto section_count = z::detail::u16(file.data() + pe + 6);
  const auto section_table = optional + z::detail::u16(file.data() + pe + 20);
  const auto headers = z::detail::u32(file.data() + optional + 60);
  if (section_count > 32 || !contains(section_table, std::size_t{section_count} * 40) ||
      headers > expected_size || !contains(0, headers))
    return false;
  m.add(base, expected_size);
  std::memcpy(m.at(base, headers), file.data(), headers);
  for (unsigned i = 0; i < section_count; ++i) {
    const auto* s = file.data() + section_table + std::size_t{i} * 40;
    const auto rva = z::detail::u32(s + 12), n = z::detail::u32(s + 16),
               offset = z::detail::u32(s + 20);
    if (n && (!contains(offset, n) || rva > expected_size || n > expected_size - rva))
      return false;
    if (n)
      std::memcpy(m.at(base + rva, n), file.data() + offset, n);
  }
  const auto preferred = z::detail::u32(file.data() + optional + 28);
  if (base == preferred)
    return true;
  const auto reloc = z::detail::u32(file.data() + optional + 136),
             reloc_size = z::detail::u32(file.data() + optional + 140);
  if (!reloc || reloc > expected_size || reloc_size > expected_size - reloc)
    return false;
  std::uint32_t cursor = 0;
  while (cursor < reloc_size) {
    if (reloc_size - cursor < 8)
      return false;
    const auto* head = m.at(base + reloc + cursor, 8);
    if (!head)
      return false;
    const auto page = z::detail::u32(head), length = z::detail::u32(head + 4);
    if (length < 8 || length > reloc_size - cursor || (length & 1u))
      return false;
    for (std::uint32_t i = 8; i < length; i += 2) {
      const auto* field = m.at(base + reloc + cursor + i, 2);
      if (!field)
        return false;
      const auto entry = z::detail::u16(field);
      if (!(entry >> 12u))
        continue;
      if ((entry >> 12u) != 3)
        return false;
      const auto rva = std::uint64_t{page} + (entry & 0xfffu);
      if (rva + 4 > expected_size)
        return false;
      const auto address = base + static_cast<std::uint32_t>(rva);
      m.word(address, z::detail::u32(m.at(address, 4)) + base - preferred);
    }
    cursor += length;
  }
  return true;
}
void end_integration_tests(Memory& m, const z::Binding& b) {
  namespace e = ubc::station_end_contract;
  // Actual pinned file and RAM code profiles. Only our hook and
  // its registry values are synthetic. No loader or world evidence.
  const e::InstalledEndHook hook{true, 0x50000000, 0x50001000};
  m.add(hook.trampoline, e::end_prefix.size() + 5);
  auto* head = m.at(e::end_address, e::end_prefix.size());
  check(head && std::equal(e::end_prefix.begin(), e::end_prefix.end(), head),
        "integration starts with actual engine EndInteraction prefix");
  if (!head)
    return;
  std::fill_n(head, e::end_prefix.size(), 0x90);
  e::detail::jump(head, e::end_address, hook.destination);
  auto* trampoline = m.at(hook.trampoline, e::end_prefix.size() + 5);
  std::memcpy(trampoline, e::end_prefix.data(), e::end_prefix.size());
  e::detail::jump(trampoline + e::end_prefix.size(),
                  hook.trampoline + static_cast<std::uint32_t>(e::end_prefix.size()),
                  e::end_address + static_cast<std::uint32_t>(e::end_prefix.size()));
  const auto& stop = e::code_ranges[2];
  check(stop.address == 0x721c20 && z::call_address > stop.address &&
            z::call_address + 5 < stop.address + stop.length,
        "integration uses the actual complete StopInteraction range around its one permitted CALL");
  check(e::detail::verify_range(m.reader(), stop, hook, &b).valid,
        "actual relocated ZBug dispatch permits exactly its CALL in the complete engine range");
  for (const auto vtable : e::mob_vtables)
    check(
        e::verify_path(m.reader(), vtable, hook, &b).valid,
        "actual complete engine path and native vtables accept exact own End hook plus fresh ZBug proof");
  check(e::detail::verify_range(m.reader(), stop, hook).issue == e::PathIssue::compatibility &&
            e::verify_path(m.reader(), e::mob_vtables[0], hook).issue ==
                e::PathIssue::compatibility,
        "patched CALL cannot pass either integration layer without a module binding");
  for (unsigned mode = 0; mode < 5; ++mode) {
    auto bad = b;
    switch (mode) {
    case 0:
      bad.zbug.mapping_verified = false;
      break;
    case 1:
      bad.union_core.mapping_verified = false;
      break;
    case 2:
      bad.zbug.file_sha256[0] ^= 1;
      break;
    case 3:
      ++bad.union_core.base;
      break;
    default:
      --bad.zbug.image_size;
      break;
    }
    check(e::detail::verify_range(m.reader(), stop, hook, &bad).issue ==
                  e::PathIssue::compatibility &&
              e::verify_path(m.reader(), e::mob_vtables[0], hook, &bad).issue ==
                  e::PathIssue::compatibility,
          "binding identity, origin and mapping are rechecked before foreign CALL normalization");
  }
  for (const auto address : {z::call_address - 1, z::call_address + 5}) {
    *m.at(address) ^= 1;
    check(e::detail::verify_range(m.reader(), stop, hook, &b).issue == e::PathIssue::code_hash &&
              e::verify_path(m.reader(), e::mob_vtables[0], hook, &b).issue ==
                  e::PathIssue::code_hash,
          "neighboring bytes remain hashed on both sides of the exact five-byte CALL");
    *m.at(address) ^= 1;
  }
  *m.at(z::call_address + 1) ^= 1;
  check(!e::detail::verify_range(m.reader(), stop, hook, &b).valid &&
            !e::verify_path(m.reader(), e::mob_vtables[0], hook, &b).valid,
        "valid modules do not bless a different current CALL target");
  *m.at(z::call_address + 1) ^= 1;
  *m.at(b.zbug.base + z::wrapper_rva + 427) ^= 1;
  check(e::detail::verify_range(m.reader(), stop, hook, &b).issue == e::PathIssue::compatibility &&
            e::verify_path(m.reader(), e::mob_vtables[0], hook, &b).issue ==
                e::PathIssue::compatibility,
        "a previously valid module binding cannot cache permission over a changed wrapper tail");
  *m.at(b.zbug.base + z::wrapper_rva + 427) ^= 1;
  auto missing = hook;
  missing.installed = false;
  check(e::verify_path(m.reader(), e::mob_vtables[0], missing, &b).issue ==
            e::PathIssue::hook_proof,
        "foreign compatibility never replaces proof of the independently installed own End hook");
  bool changed = false;
  m.before = [&](Memory& mm, std::uint32_t address) {
    if (address == e::code_ranges.back().address && !changed) {
      changed = true;
      *mm.at(z::call_address + 5) ^= 1;
    }
  };
  check(
      !e::verify_path(m.reader(), e::mob_vtables[0], hook, &b).valid && changed,
      "second complete native-path pass rejects an earlier range changed after its first compatibility proof");
  m.before = {};
  if (changed)
    *m.at(z::call_address + 5) ^= 1;
  std::memcpy(m.at(z::call_address, 5), z::original_call.data(), z::original_call.size());
  check(e::detail::verify_range(m.reader(), stop, hook).valid &&
            e::verify_path(m.reader(), e::mob_vtables[0], hook).valid,
        "unmodified original engine CALL still works without optional compatibility modules");
  std::memcpy(head, e::end_prefix.data(), e::end_prefix.size());
  auto* call = m.at(z::call_address, 5);
  call[0] = 0xe8;
  m.word(z::call_address + 1, b.zbug.base + z::wrapper_rva - (z::call_address + 5u));
}
// The previously hashed and relocated PE fixture stays exclusively in RAM. Module
// identities and dispatch objects are test values. They do not authorize live use.
void set_interact_tests(Memory& m, const z::Binding& b) {
  namespace s = ubc::cool_set_interact_path;
  std::array<std::uint8_t, s::body.length> original{};
  std::memcpy(original.data(), m.at(s::body.address, original.size()), original.size());
  const auto restore = [&](bool patched) {
    m.before = {};
    m.denied = 0;
    std::memcpy(m.at(s::body.address, original.size()), original.data(), original.size());
    if (patched)
      m.word(s::call_address + 1, b.zbug.base + z::wrapper_rva - (s::call_address + 5u));
  };
  check(z::detail::digest(original.data(), original.size()) == s::body.sha256,
        "pinned original complete SetInteract body");
  const auto direct = s::verify(m.reader());
  check(direct.valid && direct.route == s::Route::original && direct.fingerprint == s::body.sha256,
        "original SetInteract remains valid without optional module evidence");
  // The route is visible in the result and can still be bound in later snapshots.
  restore(true);
  const auto routed = s::verify(m.reader(), b);
  check(routed.valid && routed.route == s::Route::zbug_put_in_inv &&
            routed.fingerprint != direct.fingerprint,
        "only exact ZBug PutInInv route passes after real compatibility reader");
  check(routed.fingerprint ==
                z::detail::digest(m.at(s::body.address, s::body.length), s::body.length) &&
            z::detail::u32(m.at(s::call_address + 1, 4)) ==
                b.zbug.base + z::wrapper_rva - (s::call_address + 5u),
        "normalization never changes input memory or loses actual CALL identity");
  check(!s::verify(m.reader()).valid, "patched call without binding fails");
  for (unsigned i = 0; i < 12; ++i) {
    auto bad = b;
    switch (i) {
    case 0:
      bad.zbug.mapping_verified = false;
      break;
    case 1:
      bad.union_core.mapping_verified = false;
      break;
    case 2:
      bad.zbug.file_sha256[0] ^= 1;
      break;
    case 3:
      bad.union_core.file_sha256[0] ^= 1;
      break;
    case 4:
      --bad.zbug.image_size;
      break;
    case 5:
      ++bad.union_core.image_size;
      break;
    case 6:
      bad.zbug.base = 0;
      break;
    case 7:
      bad.union_core.base = UINT32_MAX - 64;
      break;
    case 8:
      bad.zbug.base = 0x400000;
      break;
    case 9:
      bad.union_core.base = bad.zbug.base;
      break;
    case 10:
      bad.zbug.base = UINT32_MAX - 64;
      break;
    default:
      bad.union_core.base = 0x400000;
      break;
    }
    const auto failed = s::verify(m.reader(), bad);
    check(!failed.valid && failed.issue == s::Issue::call && failed.address == s::call_address &&
              failed.route == s::Route::none,
          "real mapping/file/range facts required for SetInteract alternative");
  }
  for (const auto patched : {false, true}) {
    restore(patched);
    for (std::uint32_t i = 0; i < s::body.length; ++i) {
      auto* byte = m.at(s::body.address + i);
      *byte ^= 1;
      check(!s::verify(m.reader(), b).valid,
            "each complete-body byte, also CALL opcode and displacement, remains guarded");
      *byte ^= 1;
    }
    for (const auto offset : {0u, 245u, 246u, 250u, 563u}) {
      m.denied = s::body.address + offset;
      check(!s::verify(m.reader(), b).valid, "partial body reads cannot establish either route");
      m.denied = 0;
    }
    unsigned copies = 0;
    m.before = [&](Memory& mm, std::uint32_t address) {
      if (address == s::body.address && ++copies == 2)
        *mm.at(address + 563) ^= 1;
    };
    const auto changed = s::verify(m.reader(), b);
    check(!changed.valid && changed.issue == s::Issue::changed && copies == 2,
          "second entire body copy detects late code change");
    restore(patched);
    copies = 0;
    m.before = [&](Memory& mm, std::uint32_t address) {
      if (address == s::body.address && ++copies == 2)
        mm.word(s::call_address + 1,
                (patched ? z::original_put_in_inv : b.zbug.base + z::wrapper_rva) -
                    (s::call_address + 5u));
    };
    check(!s::verify(m.reader(), b).valid,
          "switching even between both accepted CALL routes during capture fails");
    restore(patched);
    copies = 0;
    m.before = [&](Memory& mm, std::uint32_t address) {
      if (address == s::body.address && ++copies == 2)
        mm.denied = s::body.address + 563;
    };
    check(!s::verify(m.reader(), b).valid,
          "failed final full copy cannot reuse first successful read");
    restore(patched);
  }
  restore(true);
  for (const auto target : {0u, z::original_put_in_inv + 1, b.zbug.base + z::wrapper_rva + 1,
                            b.union_core.base + 0x31aa0, 0xffffffffu}) {
    m.word(s::call_address + 1, target - (s::call_address + 5u));
    const auto failed = s::verify(m.reader(), b);
    check(!failed.valid && failed.issue == s::Issue::call && failed.address == s::call_address,
          "unknown SetInteract target has no wildcard exception");
    restore(true);
  }
  // The same actual reader checks relevant third-party body, import, class
  // and original dispatch data. A test boolean cannot substitute for it.
  const auto object = z::detail::u32(m.at(b.zbug.base + 0x5df70, 4));
  for (const auto address :
       {b.zbug.base + z::wrapper_rva + 427, b.zbug.base + 0x12920 + 61, b.union_core.base + 0x31aae,
        b.union_core.base + 0x12c70 + 63, b.zbug.base + 0x4515c, b.zbug.base + 0x451a0,
        b.zbug.base + 0x5df78, b.zbug.base + 0x5df84, b.zbug.base + 0x5df7c, b.zbug.base + 0x5df80,
        object, object + 12, object + 20, z::call_address, z::original_put_in_inv + 356}) {
    auto* byte = m.at(address);
    *byte ^= 1;
    const auto failed = s::verify(m.reader(), b);
    check(!failed.valid && failed.issue == s::Issue::compatibility &&
              failed.compatibility_issue != z::Issue::none,
          "SetInteract propagates concrete compatibility failures");
    *byte ^= 1;
    m.denied = address;
    check(!s::verify(m.reader(), b).valid, "missing compatibility facts fail closed");
    m.denied = 0;
  }
  unsigned copies = 0;
  m.before = [&](Memory& mm, std::uint32_t address) {
    if (address == b.zbug.base + z::wrapper_rva && ++copies == 5)
      *mm.at(address + 427) ^= 1;
  };
  const auto late = s::verify(m.reader(), b);
  check(!late.valid && late.issue == s::Issue::compatibility && copies >= 5,
        "second complete compatibility pass detects later wrapper changes");
  m.before = {};
  *m.at(b.zbug.base + z::wrapper_rva + 427) ^= 1;
  check(s::verify(m.reader(), b).valid,
        "restored independent capture valid, failed state never cached");
  restore(false);
}
void set_interact_native_path_tests(Memory& m, const z::Binding& b) {
  namespace cp = ubc::cool_native_path;
  namespace d = ubc::cool_destroy_dispatch;
  namespace p = ubc::cool_destroy_dispatch_profile;
  namespace s = ubc::cool_set_interact_path;
  constexpr std::uint32_t hero = 0x15000000, ani = 0x15002000;
  m.add(hero, 0x1000);
  m.add(ani, 8);
  m.word(0xab2684, hero);
  m.word(hero, p::npc_vtable);
  m.word(p::npc_vtable + 0x28, p::on_message);
  m.word(hero + 0xc8, 0x15003000);
  m.word(hero + 0x97c, ani);
  m.word(ani + 4, 1);
  m.word(0xab2668, 0);
  m.word(0x82e8ac, 0);
  for (std::size_t i = 0; i < p::class_chain.size(); ++i)
    m.word(p::class_chain[i] + 0x3c, i + 1 < p::class_chain.size() ? p::class_chain[i + 1] : 0u);
  d::Reader reader(41);
  d::Context context{7, 41, 0x15004000, hero, 0x15005000, 0x15006000, 13};
  const auto capture = [&](const z::Binding* binding) {
    return cp::verify_dispatch(m.reader(), {}, {}, reader, context, 18, 41, true, nullptr, binding);
  };
  const auto original = capture(nullptr);
  check(original.valid, "full original native path still accepts default strict verifier");
  m.word(s::call_address + 1, b.zbug.base + z::wrapper_rva - (s::call_address + 5u));
  check(!capture(nullptr).valid, "full native default never permits optional SetInteract patch");
  for (const auto kind : {11, 13}) {
    context.production_kind = kind;
    const auto known = capture(&b);
    check(known.valid && known.set_interact.valid &&
              known.set_interact.route == s::Route::zbug_put_in_inv,
          "complete Grind/Cool path binds real optional SetInteract compatibility reader");
  }
  auto bad = b;
  bad.zbug.mapping_verified = false;
  check(!capture(&bad).valid, "complete path rejects missing actual module lease");
  unsigned bodies = 0;
  m.before = [&](Memory& mm, std::uint32_t address) {
    if (address == s::body.address && ++bodies == 3)
      mm.word(s::call_address + 1, z::original_put_in_inv - (s::call_address + 5u));
  };
  const auto changed = capture(&b);
  check(!changed.valid && changed.issue == cp::Issue::changed && bodies == 4,
        "even two individually valid SetInteract routes cannot change between outer passes");
  m.before = {};
  check(capture(nullptr).valid, "restored original complete path remains available");
}
void grind_entry_tests(Memory& memory, const z::Binding& binding) {
  namespace entry = ubc::grind_inventory_entry;
  const auto operand = entry::put_call + 1u;
  const auto after_call = entry::put_call + 5u;
  const auto forward = binding.zbug.base + z::wrapper_rva;
  check(entry::verify(memory.reader(), binding), "original grind inventory entry");
  memory.word(operand, forward - after_call);
  check(entry::verify(memory.reader(), binding),
        "saved inventory with verified zBugFixes PutInInv");
  check(!entry::verify(memory.reader(), {}), "forward requires the bound module pair");
  *memory.at(binding.zbug.base + z::wrapper_rva) ^= 1;
  check(!entry::verify(memory.reader(), binding), "changed forwarding routine");
  *memory.at(binding.zbug.base + z::wrapper_rva) ^= 1;
  *memory.at(entry::unpack.address + entry::unpack.length - 1) ^= 1;
  check(!entry::verify(memory.reader(), binding), "change outside the inventory call operand");
  *memory.at(entry::unpack.address + entry::unpack.length - 1) ^= 1;
  memory.word(operand, forward + 1u - after_call);
  check(!entry::verify(memory.reader(), binding), "different inventory call target");
  memory.word(operand, forward - after_call);
  memory.before = [&](Memory& current, std::uint32_t address) {
    if (address == forward)
      current.word(operand, z::original_put_in_inv - after_call);
  };
  check(!entry::verify(memory.reader(), binding),
        "entry route changed during dispatch verification");
  memory.before = {};
  memory.word(operand, z::original_put_in_inv - after_call);
}

void file_tests(const char* zbug_path, const char* union_path, const char* exe_path) {
  auto b = binding();
  Memory m;
  const auto zb = read_file(zbug_path), un = read_file(union_path), engine = read_file(exe_path);
  check(z::detail::digest(zb.data(), zb.size()) == z::zbug_file_sha256, "exact ZBug file identity");
  check(z::detail::digest(un.data(), un.size()) == z::union_file_sha256,
        "exact Union file identity");
  check(z::detail::digest(engine.data(), engine.size()) ==
            ubc::lhiver::digest_literal(
                "0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f"),
        "exact engine file identity");
  if (failures)
    return;
  const bool loaded = load_pe(m, zb, b.zbug.base, b.zbug.image_size) &&
                      load_pe(m, un, b.union_core.base, b.union_core.image_size) &&
                      load_pe(m, engine, 0x400000, 0x752000);
  check(loaded, "all three verified PE images load into synthetic memory");
  if (!loaded)
    return;
  constexpr std::uint32_t object = 0x10000000;
  m.add(object, 24);
  const auto restore = [&] {
    auto* call = m.at(z::call_address, 5);
    call[0] = 0xe8;
    m.word(z::call_address + 1, b.zbug.base + z::wrapper_rva - (z::call_address + 5u));
    std::fill_n(m.at(b.zbug.base + 0x5df70, 24), 24, 0);
    m.word(b.zbug.base + 0x5df70, object);
    m.word(b.zbug.base + 0x5df78, 2);
    m.word(b.zbug.base + 0x5df7c, z::original_put_in_inv);
    m.word(b.zbug.base + 0x5df80, b.zbug.base + z::wrapper_rva);
    m.word(b.zbug.base + 0x5df84, 2);
    m.word(b.zbug.base + 0x4515c, b.union_core.base + 0x31aa0);
    m.word(b.zbug.base + 0x451a0, b.union_core.base + 0x12c70);
    std::fill_n(m.at(object, 24), 24, 0);
    m.word(object, b.union_core.base + 0x81df8);
    m.word(object + 20, z::original_put_in_inv);
  };
  restore();
  auto result = z::verify(m.reader(), b);
  check(result.valid && result.call_target == b.zbug.base + z::wrapper_rva &&
            result.original_target == z::original_put_in_inv,
        "actual complete relocated bodies plus synthetic direct original dispatch pass");
  if (!result.valid) {
    std::fprintf(stderr, "positive fixture issue=%u\n", static_cast<unsigned>(result.issue));
    return;
  }
  *m.at(b.union_core.base + 0x31aae) ^= 1;
  check(!z::verify(m.reader(), b).valid,
        "direct resolver branch final RET is part of its complete guarded body");
  *m.at(b.union_core.base + 0x31aae) ^= 1;
  for (const auto& module : {b.zbug, b.union_core}) {
    const auto pe = z::detail::u32(m.at(module.base + 60, 4));
    m.word(module.base + pe + 52, module.base);
    check(z::verify(m.reader(), b).valid,
          "Wine may expose the actual loaded image base in the PE header");
    m.word(module.base + pe + 52, z::preferred_base);
  }
  for (unsigned bad = 0; bad < 21; ++bad) {
    switch (bad) {
    case 0:
      *m.at(z::call_address) = 0xe9;
      break;
    case 1:
      *m.at(z::call_address + 1) ^= 1;
      break;
    case 2:
      m.word(b.zbug.base + 0x4515c, b.union_core.base + 0x31aa1);
      break;
    case 3:
      m.word(b.zbug.base + 0x451a0, b.union_core.base + 0x12c71);
      break;
    case 4:
      m.word(b.zbug.base + 0x5df70, 0);
      break;
    case 5:
      m.word(b.zbug.base + 0x5df70, UINT32_MAX - 8);
      break;
    case 6:
      m.word(b.zbug.base + 0x5df70, 0x400000);
      break;
    case 7:
      m.word(b.zbug.base + 0x5df70, b.zbug.base);
      break;
    case 8:
      m.word(b.zbug.base + 0x5df70, b.union_core.base);
      break;
    case 9:
      m.word(b.zbug.base + 0x5df78, 1);
      break;
    case 10:
      m.word(b.zbug.base + 0x5df84, 1);
      break;
    case 11:
      m.word(b.zbug.base + 0x5df7c, z::original_put_in_inv + 1);
      break;
    case 12:
      m.word(b.zbug.base + 0x5df80, b.zbug.base + z::wrapper_rva + 1);
      break;
    case 13:
      m.word(object, b.union_core.base + 0x81df9);
      break;
    case 14:
      m.word(object + 12, object);
      break;
    case 15:
      m.word(object + 20, z::original_put_in_inv + 1);
      break;
    case 16:
      m.denied = object + 23;
      break;
    case 17:
      m.denied = b.zbug.base + 0x11e2b;
      break;
    case 18:
      m.denied = b.union_core.base + 0x31aad;
      break;
    case 19:
      m.denied = z::original_put_in_inv + 356;
      break;
    case 20:
      m.denied = z::call_address + 4;
      break;
    }
    result = z::verify(m.reader(), b);
    check(!result.valid && result.call_target == 0 && result.original_target == 0,
          "unknown calls/dispatch/partial reads cannot provide a normalization token");
    restore();
    m.denied = 0;
  }
  for (const auto& module : {b.zbug, b.union_core}) {
    const auto areas = module.base == b.zbug.base ? z::zbug_code.data() : z::union_code.data();
    const auto count = module.base == b.zbug.base ? z::zbug_code.size() : z::union_code.size();
    for (std::size_t n = 0; n < count; ++n)
      for (std::uint32_t i = 0; i < areas[n].length; ++i) {
        auto* byte = m.at(module.base + areas[n].rva + i);
        *byte ^= 1;
        check(
            !z::verify(m.reader(), b).valid,
            "every wrapper/getter/thunk/compare byte remains guarded, including relocation operands");
        *byte ^= 1;
      }
    const auto pe = z::detail::u32(m.at(module.base + 60, 4));
    for (const auto offset : {0u, 60u, pe, pe + 4, pe + 8, pe + 20, pe + 24, pe + 52, pe + 80}) {
      auto* byte = m.at(module.base + offset);
      *byte ^= 1;
      check(!z::verify(m.reader(), b).valid, "each relevant loaded PE identity field is checked");
      *byte ^= 1;
    }
  }
  for (std::uint32_t i = 0; i < 16; ++i) {
    auto* byte = m.at(b.zbug.base + 0x50e84 + i);
    *byte ^= 1;
    check(!z::verify(m.reader(), b).valid,
          "entire burning-torch name including terminator is exact");
    *byte ^= 1;
  }
  for (std::uint32_t i = 0; i < 357; ++i) {
    auto* byte = m.at(z::original_put_in_inv + i);
    *byte ^= 1;
    check(!z::verify(m.reader(), b).valid,
          "the resolved original body must itself remain unmodified");
    *byte ^= 1;
  }
  unsigned calls = 0;
  m.before = [&](Memory& mm, std::uint32_t address) {
    if (address == b.zbug.base + 0x5df70 && ++calls == 2)
      mm.word(object + 20, 0);
  };
  check(!z::verify(m.reader(), b).valid, "dispatch changing after code verification fails");
  m.before = {};
  restore();
  calls = 0;
  m.before = [&](Memory& mm, std::uint32_t address) {
    if (address == object && ++calls == 2)
      mm.word(object + 4, 1);
  };
  check(!z::verify(m.reader(), b).valid,
        "even unused hook-object data changing in the bounded snapshot is unstable");
  m.before = {};
  restore();
  calls = 0;
  m.before = [&](Memory& mm, std::uint32_t address) {
    if (address == b.zbug.base + z::wrapper_rva && ++calls == 4)
      *mm.at(address + 427) ^= 1;
  };
  check(!z::verify(m.reader(), b).valid, "second full code pass observes late changes");
  m.before = {};
  *m.at(b.zbug.base + z::wrapper_rva + 427) ^= 1;
  check(z::verify(m.reader(), b).valid,
        "independent subsequent capture passes after synthetic fixture restoration");
  grind_entry_tests(m, b);
  end_integration_tests(m, b);
  set_interact_tests(m, b);
  set_interact_native_path_tests(m, b);
}
} // namespace
int main(int argc, char** argv) {
  synthetic_tests();
  if (argc == 4)
    file_tests(argv[1], argv[2], argv[3]);
  else if (argc != 1) {
    std::fprintf(stderr, "usage: zbug_end_compat_tests [ZBUGFIXES.DLL shw32.dll Gothic2.exe]\n");
    return 2;
  }
  std::printf("zbug_end_compat_tests: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
