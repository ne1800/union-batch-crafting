// SPDX-License-Identifier: MIT
#include "ubc/engine/runtime_script.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
namespace rs = ubc::runtime_script;
namespace profile = ubc::lhiver;
using rs::Issue;
unsigned checks = 0;
unsigned failures = 0;

void require(bool condition, const char* name) {
  ++checks;
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}

constexpr std::uint32_t parser = 0x1000;
constexpr std::uint32_t table = 0x2000;
constexpr std::uint32_t symbols = 0x3000;
constexpr std::uint32_t names = 0x5000;
constexpr std::uint32_t code = 0x10000;
constexpr std::uint32_t code_size = 5000;
constexpr std::uint32_t symbol_address(std::uint32_t index) { return symbols + index * 0x80u; }
constexpr std::uint32_t name_address(std::uint32_t index) { return names + index * 0x80u; }
constexpr std::uint32_t metadata(std::uint32_t elements, std::uint32_t type, std::uint32_t flags) {
  return elements | (type << 12u) | (flags << 16u);
}

// Synthetic pattern (i*37+11)%256, no game bytes. Expected hashes were
// computed independently with Python hashlib, without using the verifier.
constexpr std::array<profile::Function, 2> test_functions{
    {{"TEST_COOK", 2, 100, 65,
      profile::digest_literal("786bfd7089121972216bbea21539fa0bbbda5fa0d354a4d922a38ee4c5cc07ba")},
     {"TEST_END", 3, 500, 117,
      profile::digest_literal(
          "4b95905ebd055b9ebcfd0f6ae85899e0b0a4f44bcb5ea502d8ae201ebd08f462")}}};
constexpr std::array<profile::SymbolBinding, 6> test_bindings{
    {{"TEST_CLASS", 1, 4, 0, 32, -1},
     {"TEST_HERO", 4, 7, 1, 0, 1},
     {"TEST_PRODUCTION", 5, 2, 0, 1, -1},
     {"TEST_CONSTANT", 6, 2, 1, 1, -1, true, 4},
     {"TEST_AIVAR", 7, 2, 4, 100, 1, false, 0, true, 648},
     {"TEST_LEARNED", 8, 2, 0, 15, -1}}};
constexpr profile::Profile test_profile{
    parser,
    12,
    code_size,
    profile::digest_literal("30f3b7a1dd092dedd90df3905d0d95e538c70ddaa6ec8f4e9b4bb43ba2efa216"),
    test_functions.data(),
    test_functions.size(),
    test_bindings.data(),
    test_bindings.size()};

struct Image {
  std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(0x20000);
  std::uint32_t denied = 0;
  std::size_t denied_size = 0;
  std::size_t reads = 0;
  unsigned header_reads = 0;
  bool change_on_second_header = false;
  std::array<unsigned, 12> symbol_reads{};

  void put(std::uint32_t address, std::uint32_t value) noexcept {
    for (unsigned i = 0; i < 4; ++i)
      bytes[address + i] = static_cast<std::uint8_t>(value >> (8u * i));
  }
  static bool read(void* context, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& self = *static_cast<Image*>(context);
    ++self.reads;
    if (address > self.bytes.size() || size > self.bytes.size() - address)
      return false;
    if (self.denied_size && static_cast<std::uint64_t>(address) + size > self.denied &&
        static_cast<std::uint64_t>(self.denied) + self.denied_size > address)
      return false;
    if (address == parser && size == 0x58) {
      ++self.header_reads;
      if (self.change_on_second_header && self.header_reads == 2)
        self.put(parser + 0x20, 11);
    }
    if (address >= symbols && (address - symbols) % 0x80u == 0 && size == 0x3c) {
      const auto index = (address - symbols) / 0x80u;
      if (index < self.symbol_reads.size())
        ++self.symbol_reads[index];
    }
    std::memcpy(output, self.bytes.data() + address, size);
    return true;
  }
  ubc::ReadOnlyMemory memory() noexcept { return {this, &read}; }
};

struct Fixture {
  Image image;
  profile::Profile expected = test_profile;
  Fixture() {
    image.put(parser + 0x18, table);
    image.put(parser + 0x1c, 16);
    image.put(parser + 0x20, 12);
    image.put(parser + 0x48, code);
    image.put(parser + 0x4c, code + 314); // The current PC is not a function filter.
    image.put(parser + 0x50, code + code_size);
    image.put(parser + 0x54, code_size + 64);
    for (std::uint32_t i = 0; i < code_size; ++i)
      image.bytes[code + i] = static_cast<std::uint8_t>((i * 37u + 11u) % 256u);
    for (const auto& function : test_functions) {
      make_symbol(function.symbol, function.name, function.argc, function.type, function.flags,
                  function.byte_offset, function.return_type, 0);
    }
    for (const auto& binding : test_bindings) {
      make_symbol(binding.symbol, binding.name, binding.elements, binding.type, binding.flags,
                  static_cast<std::uint32_t>(binding.value),
                  static_cast<std::uint32_t>(binding.offset),
                  binding.parent_index == -1
                      ? 0u
                      : symbol_address(static_cast<std::uint32_t>(binding.parent_index)));
    }
  }
  void make_symbol(std::uint32_t index, const char* name, std::uint32_t elements,
                   std::uint32_t type, std::uint32_t flags, std::uint32_t data,
                   std::uint32_t offset, std::uint32_t parent) {
    const auto symbol = symbol_address(index);
    image.put(table + 4u * index, symbol);
    image.put(symbol + 8, name_address(index));
    image.put(symbol + 0xc, static_cast<std::uint32_t>(std::strlen(name)));
    image.put(symbol + 0x10, static_cast<std::uint32_t>(std::strlen(name) + 1));
    image.put(symbol + 0x18, data);
    image.put(symbol + 0x1c, offset);
    image.put(symbol + 0x20, metadata(elements, type, flags));
    image.put(symbol + 0x38, parent);
    std::memcpy(image.bytes.data() + name_address(index), name, std::strlen(name) + 1);
  }
  rs::FunctionIdentity recognize(std::int32_t offset = 100) {
    return rs::recognize_function(image.memory(), parser, offset, expected);
  }
  rs::RuntimeIdentity verify() { return rs::verify_runtime(image.memory(), parser, expected); }
  rs::RuntimeDiagnostic diagnose() {
    return rs::diagnose_runtime(image.memory(), parser, expected);
  }
  void extend_code() {
    constexpr std::uint32_t extended_size = code_size + 68;
    image.put(parser + 0x50, code + extended_size);
    image.put(parser + 0x54, extended_size + 64);
    for (std::uint32_t i = code_size; i < extended_size; ++i)
      image.bytes[code + i] = static_cast<std::uint8_t>((i * 37u + 11u) % 256u);
  }
};

void test_profile_facts() {
  require(profile::profile.parser_address == 0x00ab40c0, "pinned game parser is an object address");
  require(profile::profile.symbol_count == 93005 && profile::profile.code_length == 2718059,
          "pinned table/code size");
  require(profile::functions.size() == 12, "twelve original functions");
  require(profile::item_symbols ==
              std::array<std::int32_t, 6>{{16377, 16379, 16604, 16714, 16736, 16775}},
          "inventory symbol order");
  for (const auto& function : profile::functions) {
    require(function.argc == 0 && function.type == 5 && function.flags == 1 &&
                function.return_type == 0 && function.parent_index == -1,
            "original function signatures");
    require(rs::known_function(static_cast<std::int32_t>(function.byte_offset)) == &function,
            "known offset selects exact profile entry");
  }
  require(rs::known_function(-1) == nullptr && rs::known_function(825105) == nullptr,
          "negative/interior offsets are not entries");
}

void test_valid_and_unknown() {
  Fixture f;
  const auto exact = f.verify();
  require(exact.matches && exact.issue == Issue::none && exact.code_hash_available,
          "valid synthetic runtime");
  require(exact.symbol_count == 12 && exact.code_length == code_size &&
              exact.symbol_table == table && exact.code_base == code,
          "observed identity fields");
  require(exact.observed_code_hash == test_profile.code_sha256, "independent whole-code hash");
  require(f.recognize().recognized && f.recognize(500).recognized,
          "both synthetic functions recognized");
  const auto reads = f.image.reads;
  const auto unknown = f.recognize(101);
  require(!unknown.candidate && !unknown.recognized && unknown.issue == Issue::unknown_function,
          "unknown offset stays unknown");
  require(f.image.reads == reads, "unknown offset does not inspect memory");
  const auto wrong = rs::recognize_function(f.image.memory(), parser + 4, 100, f.expected);
  require(wrong.candidate && !wrong.recognized && wrong.issue == Issue::wrong_parser,
          "known offset does not approve foreign parser");
  require(f.image.reads == reads, "foreign parser never dereferenced");
  require(!rs::verify_runtime(f.image.memory(), parser).matches,
          "test image cannot satisfy production profile");
  require(std::strcmp(rs::issue_name(Issue::function_hash), "function_hash") == 0,
          "ASCII diagnostic issue");
}

void test_parser_ranges() {
  {
    Fixture f;
    f.image.denied = parser + 0x54;
    f.image.denied_size = 4;
    require(f.verify().issue == Issue::parser_unreadable, "truncated parser header");
  }
  const std::array<std::array<std::uint32_t, 2>, 7> bad_table{{{{0x18, 0}},
                                                               {{0x18, 0xfffffffcu}},
                                                               {{0x18, table + 1}},
                                                               {{0x1c, 0}},
                                                               {{0x1c, 11}},
                                                               {{0x1c, 1000001}},
                                                               {{0x20, 0xffffffffu}}}};
  for (const auto& field : bad_table) {
    Fixture f;
    f.image.put(parser + field[0], field[1]);
    require(f.verify().issue == Issue::symbol_table_range, "invalid table range/capacity rejected");
  }
  const std::array<std::array<std::uint32_t, 2>, 6> bad_code{{{{0x48, 0}},
                                                              {{0x48, 0xffffff00u}},
                                                              {{0x50, code - 1}},
                                                              {{0x54, 0}},
                                                              {{0x54, code_size - 1}},
                                                              {{0x54, 65u * 1024u * 1024u}}}};
  for (const auto& field : bad_code) {
    Fixture f;
    f.image.put(parser + field[0], field[1]);
    require(f.verify().issue == Issue::code_range, "invalid code range/capacity rejected");
  }
  {
    Fixture f;
    f.image.put(parser + 0x20, 13);
    require(f.recognize().recognized && f.verify().issue == Issue::symbol_count,
            "extended table is recognizable but not strict");
  }
  {
    Fixture f;
    f.image.put(parser + 0x50, code + code_size + 1);
    require(f.recognize().recognized && f.verify().issue == Issue::code_length,
            "changed code size is not strict identity");
  }
  {
    Fixture f;
    f.image.put(parser + 0x50, code + 120);
    require(f.recognize().issue == Issue::function_range, "function outside used code is rejected");
  }
  {
    Fixture f;
    f.image.change_on_second_header = true;
    require(f.recognize().issue == Issue::changed_during_read,
            "recognition rejects changed parser layout");
  }
  {
    Fixture f;
    f.image.change_on_second_header = true;
    require(f.verify().issue == Issue::changed_during_read,
            "whole identity rejects changed parser layout");
  }
}

void test_symbol_and_name_ranges() {
  for (const auto value : {0u, 0xfffffffcu, symbol_address(2) + 1u}) {
    Fixture f;
    f.image.put(table + 8, value);
    require(f.recognize().issue == Issue::symbol_unreadable, "invalid symbol pointers rejected");
  }
  {
    Fixture f;
    f.image.denied = table + 8;
    f.image.denied_size = 4;
    require(f.recognize().issue == Issue::symbol_unreadable, "unreadable table slot rejected");
  }
  {
    Fixture f;
    f.image.denied = symbol_address(2) + 0x38;
    f.image.denied_size = 4;
    require(f.recognize().issue == Issue::symbol_unreadable, "truncated symbol rejected");
  }
  {
    Fixture f;
    f.image.put(parser + 0x20, 2);
    require(f.recognize().issue == Issue::symbol_index, "symbol index outside live table rejected");
  }
  for (const auto length : {0u, 8u, 64u, 0xffffffffu}) {
    Fixture f;
    f.image.put(symbol_address(2) + 0xc, length);
    require(f.recognize().issue == Issue::symbol_name, "name length is exact and bounded");
  }
  for (const auto address : {0u, 0xfffffffcu}) {
    Fixture f;
    f.image.put(symbol_address(2) + 8, address);
    require(f.recognize().issue == Issue::name_unreadable, "invalid name address rejected");
  }
  {
    Fixture f;
    f.image.bytes[name_address(2) + 9] = 'X';
    require(f.recognize().issue == Issue::symbol_name, "missing terminator rejected");
  }
  {
    Fixture f;
    f.image.bytes[name_address(2) + 3] = 0;
    require(f.recognize().issue == Issue::symbol_name, "embedded terminator rejected");
  }
  {
    Fixture f;
    f.image.bytes[name_address(2)] = 'N';
    require(f.recognize().issue == Issue::symbol_name, "different symbol name rejected");
  }
}

void test_display_layout_fields() {
  Fixture f;
  auto memory = f.image.memory();
  memory.precise_parser_reads = []() noexcept { return true; };
  rs::detail::Layout initial{}, current{};
  require(rs::detail::read_layout(memory, parser, f.expected, initial) == rs::Issue::none,
          "read precise display layout");
  f.image.denied = parser + 0x4c;
  f.image.denied_size = 4;
  for (unsigned frame = 1; frame <= 8; ++frame) {
    f.image.put(parser + 0x4c, code + frame);
    require(rs::detail::read_layout(memory, parser, f.expected, current) == rs::Issue::none &&
                rs::detail::same_layout(initial, current),
            "moving instruction pointer is outside the display layout");
  }
  require(rs::detail::read_layout(f.image.memory(), parser, f.expected, current) ==
              rs::Issue::parser_unreadable,
          "full reader still requires the contiguous header");
  for (const unsigned offset : {0x18, 0x1c, 0x20, 0x48, 0x50, 0x54}) {
    f.image.denied = parser + offset;
    require(rs::detail::read_layout(memory, parser, f.expected, current) ==
                rs::Issue::parser_unreadable,
            "every consumed layout field must remain readable");
  }
}

void test_instance_constructor_identity() {
  for (const auto instance : {0u, 0x12345678u, 0xfedcba98u}) {
    Fixture f;
    auto functions = test_functions;
    functions[0].type = 7;
    functions[0].parent_index = 1;
    f.expected.functions = functions.data();
    f.make_symbol(2, "TEST_COOK", 0, 7, 1, 100, instance, symbol_address(1));
    require(f.recognize().recognized, "instance address is not a function return type");
    f.image.put(symbol_address(2) + 0x18, 101);
    require(f.recognize().issue == Issue::symbol_entry,
            "instance constructor still binds exact entry");
    f.image.put(symbol_address(2) + 0x18, 100);
    f.image.put(symbol_address(2) + 0x38, 0);
    require(f.recognize().issue == Issue::symbol_parent,
            "instance constructor still binds exact parent");
    f.image.put(symbol_address(2) + 0x38, symbol_address(1));
    f.image.bytes[code + 100] ^= 1;
    require(f.recognize().issue == Issue::function_hash,
            "instance constructor still binds body hash");
    f.image.bytes[code + 100] ^= 1;
    f.image.put(symbol_address(2) + 0x20, metadata(1, 7, 1));
    require(f.recognize().issue == Issue::symbol_signature,
            "instance constructor cannot gain parameters");
    f.image.put(symbol_address(2) + 0x20, metadata(0, 5, 1));
    require(f.recognize().issue == Issue::symbol_signature,
            "function cannot masquerade as instance constructor");
  }
}

void test_signatures_and_hashes() {
  for (const auto word : {metadata(1, 5, 1), metadata(0, 2, 1), metadata(0, 5, 9)}) {
    Fixture f;
    f.image.put(symbol_address(2) + 0x20, word);
    require(f.recognize().issue == Issue::symbol_signature,
            "parameter/type/flags mismatch rejected");
  }
  {
    Fixture f;
    f.image.put(symbol_address(2) + 0x1c, 2);
    require(f.recognize().issue == Issue::symbol_signature, "non-VOID function rejected");
  }
  {
    Fixture f;
    f.image.put(symbol_address(2) + 0x18, 101);
    require(f.recognize().issue == Issue::symbol_entry, "wrong script entry rejected");
  }
  {
    Fixture f;
    f.image.put(symbol_address(2) + 0x38, symbol_address(1));
    require(f.recognize().issue == Issue::symbol_parent, "unexpected function parent rejected");
  }
  {
    Fixture f;
    f.image.put(symbol_address(2) + 0x20, metadata(0, 5, 1) | 0xffc00000u);
    require(f.recognize().recognized, "space/debug bits are not parameter/type/flags");
  }
  {
    Fixture f;
    f.image.bytes[code + 100] ^= 1;
    require(f.recognize().issue == Issue::function_hash, "changed function body rejected");
    const auto runtime = f.verify();
    require(runtime.issue == Issue::code_hash && runtime.code_hash_available,
            "changed full hash is observable");
  }
  {
    Fixture f;
    f.image.bytes[code + 4000] ^= 1;
    require(f.recognize().recognized && f.verify().issue == Issue::code_hash,
            "known body does not approve changed remaining code");
  }
  {
    Fixture f;
    f.image.denied = code + 120;
    f.image.denied_size = 1;
    require(f.recognize().issue == Issue::code_unreadable, "unreadable function body rejected");
  }
  {
    Fixture f;
    f.image.denied = code + 4200;
    f.image.denied_size = 1;
    require(f.recognize().recognized && f.verify().issue == Issue::code_unreadable,
            "whole hash reads beyond first chunk");
  }
  {
    Fixture f;
    f.image.put(symbol_address(3) + 0x20, metadata(1, 5, 1));
    const auto runtime = f.verify();
    require(f.recognize().recognized && runtime.issue == Issue::symbol_signature &&
                runtime.failed_symbol == 3,
            "strict identity verifies every profile function");
  }
}

void test_bindings_and_dynamic_values() {
  {
    Fixture f;
    f.image.put(symbol_address(5) + 0x18, 0xffffff85u); // -123 as the current variable.
    f.image.put(symbol_address(4) + 0x1c, 0x12345678u); // Current HERO address.
    f.image.put(symbol_address(8) + 0x18, 0xdeadbeefu); // Do not dereference the array.
    require(f.verify().matches,
            "mutable scalar/instance/array values are not pinned to DAT defaults");
    const auto value = rs::symbol_int(f.image.memory(), parser, 5, f.expected);
    require(value.valid && value.value == -123, "inline mutable INT read safely");
  }
  {
    Fixture f;
    f.image.put(symbol_address(6) + 0x18, 5);
    const auto runtime = f.verify();
    require(runtime.issue == Issue::symbol_value && runtime.failed_symbol == 6,
            "constant values are checked");
  }
  {
    Fixture f;
    f.image.put(symbol_address(7) + 0x1c, 652);
    require(f.verify().issue == Issue::symbol_signature, "member offset is checked");
  }
  {
    Fixture f;
    f.image.put(symbol_address(4) + 0x38, 0);
    require(f.verify().issue == Issue::symbol_parent, "instance parent binding is checked");
  }
  {
    Fixture f;
    f.image.bytes[name_address(5)] = 'X';
    require(f.verify().issue == Issue::symbol_name, "dynamic variable still requires exact name");
  }
  {
    Fixture f;
    const auto address = rs::read_symbol_address(f.image.memory(), parser, 4, f.expected);
    require(address.valid && address.address == symbol_address(4),
            "symbol address helper returns symbol object");
    require(rs::read_symbol_address(f.image.memory(), parser, 12, f.expected).issue ==
                Issue::symbol_index,
            "symbol helper rejects out-of-range index");
    require(rs::symbol_int(f.image.memory(), parser, 7, f.expected).issue == Issue::not_scalar_int,
            "member INT is not treated as inline global");
    require(rs::symbol_int(f.image.memory(), parser, 8, f.expected).issue == Issue::not_scalar_int,
            "INT array is not treated as scalar");
    require(rs::symbol_int(f.image.memory(), parser, 4, f.expected).issue == Issue::not_scalar_int,
            "instance is not treated as scalar");
  }
  {
    Fixture f;
    f.image.put(table + 20, 0);
    require(!rs::symbol_int(f.image.memory(), parser, 5, f.expected).valid,
            "invalid scalar pointer has no fabricated zero value");
  }
}

void test_invalid_profile() {
  {
    Fixture f;
    f.expected.functions = nullptr;
    require(f.verify().issue == Issue::invalid_profile &&
                f.recognize().issue == Issue::invalid_profile,
            "missing profile array rejected");
  }
  {
    Fixture f;
    f.expected.code_length = 0;
    require(f.verify().issue == Issue::invalid_profile, "empty expected code rejected");
  }
  {
    Fixture f;
    f.expected.parser_address = 0xfffffff0u;
    require(f.verify().issue == Issue::invalid_profile, "overflowing expected parser rejected");
  }
  {
    Fixture f;
    const ubc::ReadOnlyMemory absent{nullptr, nullptr};
    require(rs::verify_runtime(absent, parser, f.expected).issue == Issue::parser_unreadable,
            "missing backend rejected");
  }
}

void test_diagnostic_exact_and_extensions() {
  {
    Fixture f;
    const auto d = f.diagnose();
    require(d.exact_identity_matches && d.exact_identity_issue == Issue::none &&
                d.local_contract_matches && d.local_contract_issue == Issue::none,
            "diagnostic reports exact and local identity independently");
    require(d.layout_stable && d.layout_issue == Issue::none && f.image.header_reads == 2,
            "diagnostic checks layout after all reads");
    require(d.symbol_count == 12 && d.code_length == code_size && d.symbol_table == table &&
                d.code_base == code,
            "diagnostic observed layout fields");
    require(d.code_hash_available && d.prefix_hash_available && d.original_prefix_matches &&
                d.observed_code_hash == test_profile.code_sha256 &&
                d.observed_prefix_hash == test_profile.code_sha256,
            "exact image has two independently expected digests");
    require(d.functions_checked == test_functions.size() &&
                d.bindings_checked == test_bindings.size() && d.failed_symbol == 0,
            "diagnostic checks complete fixed contract");
    require(d.functions_match && d.bindings_match && d.functions_issue == Issue::none &&
                d.bindings_issue == Issue::none && d.first_failed_function == 0 &&
                d.first_failed_binding == 0,
            "valid diagnostic reports both fixed contract groups separately");
  }
  {
    Fixture f;
    f.image.put(parser + 0x20, 13);
    f.image.put(table + 12u * 4u, 0xffffffffu); // An unrelated extra symbol remains unknown.
    f.extend_code();
    const auto d = f.diagnose();
    require(!d.exact_identity_matches && d.exact_identity_issue == Issue::symbol_count &&
                d.symbol_count == 13 && d.code_length == code_size + 68,
            "extensions remain rejected by exact identity");
    require(d.local_contract_matches && d.local_contract_issue == Issue::none && d.layout_stable,
            "extensions do not prevent read-only checks of fixed local contract");
    require(d.prefix_hash_available && d.original_prefix_matches &&
                d.observed_prefix_hash == test_profile.code_sha256,
            "original prefix is hashed at exact length");
    require(d.code_hash_available &&
                d.observed_code_hash ==
                    profile::digest_literal(
                        "794fc412076f3a6ff6c8d41473814cc17a5eb94b34cb2e763449bc9d668529f4"),
            "appended code has independently calculated full digest");
    require(f.verify().issue == Issue::symbol_count, "diagnosis never enrolls an extended image");
  }
  {
    Fixture f;
    f.extend_code();
    const auto d = f.diagnose();
    require(!d.exact_identity_matches && d.exact_identity_issue == Issue::code_length &&
                d.local_contract_matches && d.original_prefix_matches,
            "code-only extension stays diagnostically separate");
  }
  {
    Fixture f;
    f.image.put(parser + 0x20, 13);
    const auto d = f.diagnose();
    require(!d.exact_identity_matches && d.exact_identity_issue == Issue::symbol_count &&
                d.local_contract_matches && d.code_hash_available &&
                d.observed_code_hash == test_profile.code_sha256,
            "symbol-only extension does not bypass identity gate");
  }
}

void test_diagnostic_local_failures() {
  {
    Fixture f;
    f.image.bytes[code + 4000] ^= 1;
    const auto d = f.diagnose();
    require(!d.exact_identity_matches && d.exact_identity_issue == Issue::code_hash &&
                !d.original_prefix_matches && d.local_contract_matches,
            "changed unprofiled code is not confused with fixed local function failure");
  }
  {
    Fixture f;
    f.image.put(parser + 0x20, 13);
    f.image.bytes[code + 100] ^= 1;
    f.image.bytes[name_address(5)] = 'X';
    const auto d = f.diagnose();
    require(d.exact_identity_issue == Issue::symbol_count && !d.local_contract_matches &&
                d.local_contract_issue == Issue::function_hash && d.failed_symbol == 2,
            "global mismatch does not hide first local failure");
    require(d.functions_checked == test_functions.size() &&
                d.bindings_checked == test_bindings.size(),
            "diagnostic completes checks despite earlier failures");
    bool all_read = true;
    for (const auto& function : test_functions)
      all_read = all_read && f.image.symbol_reads[function.symbol] != 0;
    for (const auto& binding : test_bindings)
      all_read = all_read && f.image.symbol_reads[binding.symbol] != 0;
    require(all_read, "each fixed function and binding is actually read after first failure");
  }
  {
    Fixture f;
    f.extend_code();
    f.image.put(symbol_address(6) + 0x18, 5);
    const auto d = f.diagnose();
    require(d.exact_identity_issue == Issue::code_length && !d.local_contract_matches &&
                d.local_contract_issue == Issue::symbol_value && d.failed_symbol == 6,
            "binding failure is reported even when code length differs");
  }
  {
    Fixture f;
    f.image.put(symbol_address(3) + 0x18, 501);
    const auto d = f.diagnose();
    require(!d.exact_identity_matches && !d.local_contract_matches &&
                d.exact_identity_issue == Issue::symbol_entry &&
                d.local_contract_issue == Issue::symbol_entry && d.failed_symbol == 3,
            "identical whole-code digest does not approve changed function entry");
  }
}

void test_diagnostic_short_and_unreadable_code() {
  {
    Fixture f;
    f.image.put(parser + 0x50, code + 1000);
    f.image.denied = code + 1000;
    f.image.denied_size = code_size;
    const auto d = f.diagnose();
    require(d.code_hash_available &&
                d.observed_code_hash ==
                    profile::digest_literal(
                        "57799de80e3dd6e2ac4d40c41a150d1662f7f87d0d994776a2fdc37c39b0ea4e") &&
                !d.prefix_hash_available && !d.original_prefix_matches,
            "short code is hashed without reading nonexistent original prefix");
    require(d.local_contract_matches && d.exact_identity_issue == Issue::code_length,
            "short image may retain local contract but never exact identity");
  }
  {
    Fixture f;
    f.image.put(parser + 0x50, code + 120);
    const auto d = f.diagnose();
    require(d.code_hash_available && !d.prefix_hash_available && !d.local_contract_matches &&
                d.local_contract_issue == Issue::function_range && d.failed_symbol == 2,
            "function outside shortened live code fails local contract");
  }
  {
    Fixture f;
    f.extend_code();
    f.image.denied = code + code_size + 5;
    f.image.denied_size = 1;
    const auto d = f.diagnose();
    require(!d.code_hash_available && d.prefix_hash_available && d.original_prefix_matches &&
                d.observed_prefix_hash == test_profile.code_sha256 && d.local_contract_matches,
            "unreadable appended tail retains completed prefix and independent local evidence");
    require(!d.exact_identity_matches, "unreadable appended code cannot yield exact identity");
  }
  {
    Fixture f;
    f.image.denied = code + 4200;
    f.image.denied_size = 1;
    const auto d = f.diagnose();
    require(!d.code_hash_available && !d.prefix_hash_available &&
                d.exact_identity_issue == Issue::code_unreadable && d.local_contract_matches,
            "unreadable unprofiled area is separate from independently readable local contract");
  }
  {
    Fixture f;
    f.image.denied = code + 120;
    f.image.denied_size = 1;
    const auto d = f.diagnose();
    require(!d.code_hash_available && !d.local_contract_matches &&
                d.local_contract_issue == Issue::code_unreadable && d.failed_symbol == 2,
            "unreadable known function has explicit local failure");
  }
}

void test_diagnostic_contract_groups() {
  {
    Fixture f;
    f.image.bytes[code + 100] ^= 1;
    const auto d = f.diagnose();
    require(!d.functions_match && d.functions_issue == Issue::function_hash &&
                d.first_failed_function == 2,
            "changed function has independent function failure");
    require(d.bindings_match && d.bindings_issue == Issue::none && d.first_failed_binding == 0,
            "changed function does not hide unchanged bindings");
    require(!d.local_contract_matches && !d.exact_identity_matches &&
                d.local_contract_matches == (d.functions_match && d.bindings_match),
            "matching bindings alone do not approve local or exact identity");
  }
  {
    Fixture f;
    f.image.put(symbol_address(6) + 0x18, 5);
    const auto d = f.diagnose();
    require(d.functions_match && d.functions_issue == Issue::none && d.first_failed_function == 0,
            "changed binding does not hide unchanged functions");
    require(!d.bindings_match && d.bindings_issue == Issue::symbol_value &&
                d.first_failed_binding == 6,
            "changed binding has independent binding failure");
    require(!d.local_contract_matches && !d.exact_identity_matches &&
                d.local_contract_matches == (d.functions_match && d.bindings_match),
            "matching functions alone do not approve local or exact identity");
  }
  {
    Fixture f;
    f.image.bytes[code + 100] ^= 1;
    f.image.put(symbol_address(3) + 0x18, 501);
    f.image.bytes[name_address(5)] = 'X';
    f.image.put(symbol_address(6) + 0x18, 5);
    const auto d = f.diagnose();
    require(!d.functions_match && !d.bindings_match && d.first_failed_function == 2 &&
                d.functions_issue == Issue::function_hash && d.first_failed_binding == 5 &&
                d.bindings_issue == Issue::symbol_name,
            "first failure is preserved independently in each group");
    require(d.failed_symbol == 2 && d.local_contract_issue == Issue::function_hash,
            "combined contract keeps original first-error ordering");
  }
  {
    Fixture f;
    f.image.bytes[code + 100] ^= 1;
    f.image.change_on_second_header = true;
    const auto d = f.diagnose();
    require(!d.functions_match && !d.bindings_match && !d.local_contract_matches &&
                !d.layout_stable,
            "unstable layout invalidates otherwise matching binding group");
    require(d.functions_issue == Issue::function_hash && d.first_failed_function == 2 &&
                d.bindings_issue == Issue::changed_during_read && d.first_failed_binding == 0,
            "layout failure does not erase an earlier concrete function failure");
  }
}

void test_diagnostic_layout_and_limits() {
  {
    Fixture f;
    f.image.change_on_second_header = true;
    const auto d = f.diagnose();
    require(!d.layout_stable && d.layout_issue == Issue::changed_during_read &&
                !d.exact_identity_matches && !d.local_contract_matches &&
                !d.original_prefix_matches &&
                d.exact_identity_issue == Issue::changed_during_read &&
                d.local_contract_issue == Issue::changed_during_read,
            "changed layout invalidates every positive diagnostic classification");
    require(d.code_hash_available && d.prefix_hash_available,
            "digests remain raw observations when layout changes");
    require(!d.functions_match && !d.bindings_match &&
                d.functions_issue == Issue::changed_during_read &&
                d.bindings_issue == Issue::changed_during_read && d.first_failed_function == 0 &&
                d.first_failed_binding == 0,
            "unstable layout rejects both otherwise valid contract groups");
  }
  {
    Fixture f;
    f.image.put(parser + 0x54, 65u * 1024u * 1024u);
    const auto d = f.diagnose();
    require(d.layout_issue == Issue::code_range && !d.layout_stable && !d.code_hash_available &&
                !d.prefix_hash_available && d.functions_checked == 0 && d.bindings_checked == 0 &&
                f.image.reads == 1,
            "oversized code rejected before hashing or symbol traversal");
  }
  {
    Fixture f;
    const auto d = rs::diagnose_runtime(f.image.memory(), parser + 4, f.expected);
    require(d.layout_issue == Issue::wrong_parser &&
                d.exact_identity_issue == Issue::wrong_parser &&
                d.local_contract_issue == Issue::wrong_parser && f.image.reads == 0,
            "diagnostic does not dereference foreign parser");
    require(!d.functions_match && !d.bindings_match && d.functions_issue == Issue::wrong_parser &&
                d.bindings_issue == Issue::wrong_parser,
            "early layout failure is explicit for both groups");
  }
  {
    Fixture f;
    f.expected.functions = nullptr;
    const auto d = f.diagnose();
    require(d.layout_issue == Issue::invalid_profile && !d.exact_identity_matches &&
                !d.local_contract_matches && f.image.reads == 0,
            "invalid diagnostic profile is rejected before reads");
  }
}

void test_array_values_and_local_contract() {
  Fixture f;
  constexpr std::uint32_t values = 0x18000;
  f.image.put(symbol_address(8) + 0x18, values);
  for (std::uint32_t i = 0; i < 15; ++i)
    f.image.put(values + 4 * i, i + 1);
  for (std::uint32_t i = 0; i < 15; ++i) {
    const auto v = rs::symbol_int_element(f.image.memory(), parser, 8, i, f.expected);
    require(v.valid && v.value == static_cast<std::int32_t>(i + 1),
            "array element reads pointed storage");
  }
  require(!rs::symbol_int_element(f.image.memory(), parser, 8, 15, f.expected).valid,
          "out of range array index rejected");
  f.image.put(values, 0);
  const auto unlearned = rs::symbol_int_element(f.image.memory(), parser, 8, 0, f.expected);
  require(unlearned.valid && unlearned.value == 0,
          "unlearned zero remains a real zero, not a missing value");
  f.image.put(symbol_address(8) + 0x18, UINT32_MAX - 1);
  require(!rs::symbol_int_element(f.image.memory(), parser, 8, 0, f.expected).valid,
          "array pointer overflow rejected");
  require(!rs::symbol_int_element(f.image.memory(), parser, 7, 0, f.expected).valid,
          "member array cannot be read without native instance binding");
  const auto scalar = rs::symbol_int_element(f.image.memory(), parser, 6, 0, f.expected);
  require(scalar.valid && scalar.value == 4, "scalar value stays inline");
  {
    Fixture g;
    g.image.denied = code + 3000;
    g.image.denied_size = 1;
    require(rs::verify_local_contract(g.image.memory(), parser, g.expected).matches,
            "local gate never reads unrelated code pages");
    require(!g.verify().matches, "local match does not bypass full identity check");
  }
  {
    Fixture g;
    g.image.bytes[code + 101] ^= 1;
    const auto result = rs::verify_local_contract(g.image.memory(), parser, g.expected);
    require(!result.matches && result.failed_symbol == 2,
            "changed recipe still rejects local gate");
  }
  {
    Fixture g;
    g.image.put(symbol_address(6) + 0x18, 5);
    require(!rs::verify_local_contract(g.image.memory(), parser, g.expected).matches,
            "constant binding change rejects local gate");
  }
  {
    Fixture g;
    g.image.change_on_second_header = true;
    require(!rs::verify_local_contract(g.image.memory(), parser, g.expected).matches,
            "local gate rejects layout changes");
  }
}
} // namespace

int main() {
  test_profile_facts();
  test_valid_and_unknown();
  test_parser_ranges();
  test_symbol_and_name_ranges();
  test_signatures_and_hashes();
  test_display_layout_fields();
  test_instance_constructor_identity();
  test_bindings_and_dynamic_values();
  test_invalid_profile();
  test_diagnostic_exact_and_extensions();
  test_diagnostic_local_failures();
  test_diagnostic_short_and_unreadable_code();
  test_diagnostic_contract_groups();
  test_diagnostic_layout_and_limits();
  test_array_values_and_local_contract();
  std::printf("Runtime-Skript: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
