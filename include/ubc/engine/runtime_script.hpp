// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/engine/read_only_memory.hpp"
#include "ubc/core/sha256.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

// Own read-only evaluation of the established x86 parser layout. No
// engine calls, no dereferencing of foreign pointers, no VM write method.
namespace ubc::runtime_script {

enum class Issue {
  none,
  invalid_profile,
  wrong_parser,
  parser_unreadable,
  symbol_table_range,
  symbol_count,
  code_range,
  code_length,
  code_unreadable,
  code_hash,
  unknown_function,
  symbol_index,
  symbol_unreadable,
  name_unreadable,
  symbol_name,
  symbol_signature,
  symbol_entry,
  symbol_parent,
  symbol_value,
  function_range,
  function_hash,
  changed_during_read,
  not_scalar_int
};

inline const char* issue_name(Issue issue) noexcept {
  switch (issue) {
  case Issue::none:
    return "ok";
  case Issue::invalid_profile:
    return "invalid_profile";
  case Issue::wrong_parser:
    return "wrong_parser";
  case Issue::parser_unreadable:
    return "parser_unreadable";
  case Issue::symbol_table_range:
    return "symbol_table_range";
  case Issue::symbol_count:
    return "symbol_count";
  case Issue::code_range:
    return "code_range";
  case Issue::code_length:
    return "code_length";
  case Issue::code_unreadable:
    return "code_unreadable";
  case Issue::code_hash:
    return "code_hash";
  case Issue::unknown_function:
    return "unknown_function";
  case Issue::symbol_index:
    return "symbol_index";
  case Issue::symbol_unreadable:
    return "symbol_unreadable";
  case Issue::name_unreadable:
    return "name_unreadable";
  case Issue::symbol_name:
    return "symbol_name";
  case Issue::symbol_signature:
    return "symbol_signature";
  case Issue::symbol_entry:
    return "symbol_entry";
  case Issue::symbol_parent:
    return "symbol_parent";
  case Issue::symbol_value:
    return "symbol_value";
  case Issue::function_range:
    return "function_range";
  case Issue::function_hash:
    return "function_hash";
  case Issue::changed_during_read:
    return "changed_during_read";
  case Issue::not_scalar_int:
    return "not_scalar_int";
  }
  return "unknown_issue";
}

struct FunctionIdentity {
  const lhiver::Function* candidate = nullptr;
  bool recognized = false;
  Issue issue = Issue::unknown_function;
};

struct RuntimeIdentity {
  bool matches = false;
  Issue issue = Issue::none;
  std::uint32_t symbol_count = 0;
  std::uint32_t symbol_table = 0;
  std::uint32_t code_base = 0;
  std::uint32_t code_length = 0;
  lhiver::Digest observed_code_hash{};
  bool code_hash_available = false;
  // On a symbol error the affected profile target; no foreign text.
  std::uint32_t failed_symbol = 0;
};

// Pure evidence capture, no release for script/inventory operations.
// local_contract_matches evaluates exclusively the fixed profile
// targets, never unknown additional symbols or the remaining script set.
struct RuntimeDiagnostic {
  bool exact_identity_matches = false;
  Issue exact_identity_issue = Issue::none;
  bool local_contract_matches = false;
  Issue local_contract_issue = Issue::none;
  bool functions_match = false;
  Issue functions_issue = Issue::none;
  std::uint32_t first_failed_function = 0;
  bool bindings_match = false;
  Issue bindings_issue = Issue::none;
  std::uint32_t first_failed_binding = 0;
  bool layout_stable = false;
  Issue layout_issue = Issue::none;
  std::uint32_t symbol_count = 0;
  std::uint32_t symbol_table = 0;
  std::uint32_t code_base = 0;
  std::uint32_t code_length = 0;
  lhiver::Digest observed_code_hash{};
  bool code_hash_available = false;
  lhiver::Digest observed_prefix_hash{};
  bool prefix_hash_available = false;
  // Positive comparison only at stable layout; hashes remain raw findings.
  bool original_prefix_matches = false;
  std::uint32_t failed_symbol = 0;
  std::uint32_t functions_checked = 0;
  std::uint32_t bindings_checked = 0;
};

struct SymbolAddress {
  bool valid = false;
  std::uint32_t address = 0; // zCPar_Symbol, not its instance address.
  Issue issue = Issue::none;
};

struct SymbolInt {
  bool valid = false;
  std::int32_t value = 0;
  Issue issue = Issue::none;
};

namespace detail {
inline constexpr std::uint32_t max_symbols = 1000000;
inline constexpr std::uint32_t max_code_capacity = 64u * 1024u * 1024u;
inline constexpr std::size_t max_name = 63;

inline std::uint32_t u32(const std::uint8_t* bytes) noexcept {
  return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8u) |
         (static_cast<std::uint32_t>(bytes[2]) << 16u) |
         (static_cast<std::uint32_t>(bytes[3]) << 24u);
}

inline bool range(std::uint64_t base, std::uint64_t size) noexcept {
  constexpr auto limit = std::numeric_limits<std::uint32_t>::max();
  return base != 0 && base <= limit && size != 0 && size - 1 <= limit - base;
}

inline bool valid_profile(const lhiver::Profile& profile) noexcept {
  if (!range(profile.parser_address, 0x58) || profile.symbol_count == 0 ||
      profile.symbol_count > max_symbols || profile.code_length == 0 ||
      profile.code_length > max_code_capacity || profile.function_count > 64 ||
      profile.binding_count > 64 || (profile.function_count && !profile.functions) ||
      (profile.binding_count && !profile.bindings))
    return false;
  for (std::size_t i = 0; i < profile.function_count; ++i) {
    const auto& function = profile.functions[i];
    if (!function.name || function.symbol >= profile.symbol_count || function.byte_length == 0 ||
        function.byte_offset > profile.code_length ||
        function.byte_length > profile.code_length - function.byte_offset)
      return false;
    for (std::size_t j = 0; j < i; ++j) {
      if (profile.functions[j].symbol == function.symbol ||
          profile.functions[j].byte_offset == function.byte_offset)
        return false;
    }
  }
  for (std::size_t i = 0; i < profile.binding_count; ++i) {
    const auto& binding = profile.bindings[i];
    if (!binding.name || binding.symbol >= profile.symbol_count || binding.parent_index < -1 ||
        (binding.parent_index >= 0 &&
         static_cast<std::uint32_t>(binding.parent_index) >= profile.symbol_count))
      return false;
  }
  return true;
}

struct Layout {
  std::uint32_t table = 0;
  std::uint32_t table_capacity = 0;
  std::uint32_t count = 0;
  std::uint32_t code = 0;
  std::uint32_t code_end = 0;
  std::uint32_t code_capacity = 0;
  std::uint32_t code_length = 0;
};

inline bool same_layout(const Layout& left, const Layout& right) noexcept {
  return left.table == right.table && left.table_capacity == right.table_capacity &&
         left.count == right.count && left.code == right.code && left.code_end == right.code_end &&
         left.code_capacity == right.code_capacity;
}

inline Issue read_layout(const ReadOnlyMemory& memory, std::uint32_t parser,
                         const lhiver::Profile& profile, Layout& layout) noexcept {
  if (parser != profile.parser_address)
    return Issue::wrong_parser;
  std::array<std::uint8_t, 0x58> bytes{};
  if (!memory.copy(parser, bytes.data(), bytes.size()))
    return Issue::parser_unreadable;
  layout.table = u32(bytes.data() + 0x18);
  layout.table_capacity = u32(bytes.data() + 0x1c);
  layout.count = u32(bytes.data() + 0x20);
  layout.code = u32(bytes.data() + 0x48);
  layout.code_end = u32(bytes.data() + 0x50);
  layout.code_capacity = u32(bytes.data() + 0x54);
  if (layout.table % 4u != 0 || layout.count == 0 || layout.count > layout.table_capacity ||
      layout.table_capacity > max_symbols ||
      !range(layout.table, static_cast<std::uint64_t>(layout.table_capacity) * 4u))
    return Issue::symbol_table_range;
  if (layout.code_end <= layout.code || layout.code_capacity == 0 ||
      layout.code_capacity > max_code_capacity || !range(layout.code, layout.code_capacity))
    return Issue::code_range;
  layout.code_length = layout.code_end - layout.code;
  if (layout.code_length > layout.code_capacity)
    return Issue::code_range;
  return Issue::none;
}

inline Issue stable_layout(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const lhiver::Profile& profile, const Layout& before) noexcept {
  Layout after{};
  if (read_layout(memory, parser, profile, after) != Issue::none || !same_layout(before, after))
    return Issue::changed_during_read;
  return Issue::none;
}

struct Symbol {
  std::uint32_t address = 0;
  std::uint32_t name_data = 0;
  std::uint32_t name_length = 0;
  std::uint32_t data = 0;
  std::uint32_t offset = 0;
  std::uint32_t elements = 0;
  std::uint32_t type = 0;
  std::uint32_t flags = 0;
  std::uint32_t parent = 0;
};

inline Issue lookup(const ReadOnlyMemory& memory, const Layout& layout, std::uint32_t index,
                    Symbol& symbol) noexcept {
  if (index >= layout.count)
    return Issue::symbol_index;
  const auto slot =
      static_cast<std::uint64_t>(layout.table) + static_cast<std::uint64_t>(index) * 4u;
  std::array<std::uint8_t, 4> pointer{};
  if (!range(slot, pointer.size()) ||
      !memory.copy(static_cast<std::uint32_t>(slot), pointer.data(), pointer.size()))
    return Issue::symbol_unreadable;
  symbol.address = u32(pointer.data());
  std::array<std::uint8_t, 0x3c> bytes{};
  if (symbol.address % 4u != 0 || !range(symbol.address, bytes.size()) ||
      !memory.copy(symbol.address, bytes.data(), bytes.size()))
    return Issue::symbol_unreadable;
  symbol.name_data = u32(bytes.data() + 8);
  symbol.name_length = u32(bytes.data() + 0xc);
  symbol.data = u32(bytes.data() + 0x18);
  symbol.offset = u32(bytes.data() + 0x1c);
  const auto metadata = u32(bytes.data() + 0x20);
  symbol.elements = metadata & 0xfffu;
  symbol.type = (metadata >> 12u) & 0xfu;
  symbol.flags = (metadata >> 16u) & 0x3fu;
  symbol.parent = u32(bytes.data() + 0x38);
  return Issue::none;
}

inline Issue name_matches(const ReadOnlyMemory& memory, const Symbol& symbol,
                          const char* expected) noexcept {
  std::size_t length = 0;
  while (length <= max_name && expected[length])
    ++length;
  if (length > max_name || symbol.name_length > max_name || symbol.name_length != length)
    return Issue::symbol_name;
  std::array<char, max_name + 1> bytes{};
  if (!range(symbol.name_data, length + 1) ||
      !memory.copy(symbol.name_data, bytes.data(), length + 1))
    return Issue::name_unreadable;
  if (bytes[length] != '\0' || std::memcmp(bytes.data(), expected, length) != 0)
    return Issue::symbol_name;
  return Issue::none;
}

inline Issue parent_matches(const ReadOnlyMemory& memory, const Layout& layout,
                            const Symbol& symbol, std::int32_t expected) noexcept {
  if (expected == -1)
    return symbol.parent == 0 ? Issue::none : Issue::symbol_parent;
  if (expected < 0)
    return Issue::invalid_profile;
  Symbol parent{};
  const auto issue = lookup(memory, layout, static_cast<std::uint32_t>(expected), parent);
  if (issue != Issue::none)
    return issue;
  return symbol.parent == parent.address ? Issue::none : Issue::symbol_parent;
}

inline bool hash_range(const ReadOnlyMemory& memory, std::uint32_t address, std::uint32_t length,
                       lhiver::Digest& result) noexcept {
  if (!range(address, length))
    return false;
  std::array<std::uint8_t, 4096> bytes{};
  Sha256 hash;
  std::uint32_t position = 0;
  while (position < length) {
    const auto remaining = length - position;
    const auto count =
        remaining < bytes.size() ? remaining : static_cast<std::uint32_t>(bytes.size());
    if (!memory.copy(address + position, bytes.data(), count))
      return false;
    hash.update(bytes.data(), count);
    position += count;
  }
  result = hash.finish();
  return true;
}

inline Issue check_function(const ReadOnlyMemory& memory, const Layout& layout,
                            const lhiver::Function& function) noexcept {
  Symbol symbol{};
  auto issue = lookup(memory, layout, function.symbol, symbol);
  if (issue != Issue::none)
    return issue;
  issue = name_matches(memory, symbol, function.name);
  if (issue != Issue::none)
    return issue;
  // zCPar_Symbol::offset is only the return type for FUNC. For INSTANCE
  // SetInstance(adr) occupies the same field with the running object address.
  // The constructor remains bound via data/Parent/Signature/Bodyhash; its
  // current C_INFO object is checked separately by the native selection adapter.
  const bool instance_constructor = function.type == 7 && function.flags == 1 &&
                                    function.argc == 0 && function.return_type == 0 &&
                                    function.parent_index >= 0;
  if (symbol.type != function.type || symbol.flags != function.flags ||
      symbol.elements != function.argc ||
      (!instance_constructor && symbol.offset != function.return_type))
    return Issue::symbol_signature;
  issue = parent_matches(memory, layout, symbol, function.parent_index);
  if (issue != Issue::none)
    return issue;
  if (symbol.data != function.byte_offset)
    return Issue::symbol_entry;
  if (function.byte_offset > layout.code_length ||
      function.byte_length > layout.code_length - function.byte_offset)
    return Issue::function_range;
  const auto address = static_cast<std::uint64_t>(layout.code) + function.byte_offset;
  lhiver::Digest hash{};
  if (!range(address, function.byte_length) ||
      !hash_range(memory, static_cast<std::uint32_t>(address), function.byte_length, hash))
    return Issue::code_unreadable;
  return hash == function.body_sha256 ? Issue::none : Issue::function_hash;
}

inline Issue check_binding(const ReadOnlyMemory& memory, const Layout& layout,
                           const lhiver::SymbolBinding& binding) noexcept {
  Symbol symbol{};
  auto issue = lookup(memory, layout, binding.symbol, symbol);
  if (issue != Issue::none)
    return issue;
  issue = name_matches(memory, symbol, binding.name);
  if (issue != Issue::none)
    return issue;
  if (symbol.type != binding.type || symbol.flags != binding.flags ||
      symbol.elements != binding.elements)
    return Issue::symbol_signature;
  issue = parent_matches(memory, layout, symbol, binding.parent_index);
  if (issue != Issue::none)
    return issue;
  if (binding.check_offset && symbol.offset != static_cast<std::uint32_t>(binding.offset))
    return Issue::symbol_signature;
  if (binding.check_value && (binding.type != 2 || binding.elements != 1 || binding.flags != 1 ||
                              symbol.data != static_cast<std::uint32_t>(binding.value)))
    return Issue::symbol_value;
  return Issue::none;
}
} // namespace detail

inline const lhiver::Function*
known_function(std::int32_t byte_offset,
               const lhiver::Profile& profile = lhiver::profile) noexcept {
  if (byte_offset < 0 || !detail::valid_profile(profile))
    return nullptr;
  for (std::size_t i = 0; i < profile.function_count; ++i)
    if (profile.functions[i].byte_offset == static_cast<std::uint32_t>(byte_offset))
      return &profile.functions[i];
  return nullptr;
}

// Local function detection is intentionally not a substitute for
// verify_runtime. A known body may appear in an otherwise changed script set.
inline FunctionIdentity
recognize_function(const ReadOnlyMemory& memory, std::uint32_t parser, std::int32_t byte_offset,
                   const lhiver::Profile& profile = lhiver::profile) noexcept {
  FunctionIdentity result{};
  if (!detail::valid_profile(profile)) {
    result.issue = Issue::invalid_profile;
    return result;
  }
  result.candidate = known_function(byte_offset, profile);
  if (!result.candidate)
    return result;
  detail::Layout layout{};
  result.issue = detail::read_layout(memory, parser, profile, layout);
  if (result.issue == Issue::none)
    result.issue = detail::check_function(memory, layout, *result.candidate);
  if (result.issue == Issue::none)
    result.issue = detail::stable_layout(memory, parser, profile, layout);
  result.recognized = result.issue == Issue::none;
  return result;
}

// Full fixed observation profile: parser, table form, full code
// hash, twelve functions and relevant bindings. No check of the EXE
// or all mutable VM data; their current value remains runtime data.
inline RuntimeIdentity verify_runtime(const ReadOnlyMemory& memory, std::uint32_t parser,
                                      const lhiver::Profile& profile = lhiver::profile) noexcept {
  RuntimeIdentity result{};
  if (!detail::valid_profile(profile)) {
    result.issue = Issue::invalid_profile;
    return result;
  }
  detail::Layout layout{};
  result.issue = detail::read_layout(memory, parser, profile, layout);
  result.symbol_table = layout.table;
  result.symbol_count = layout.count;
  result.code_base = layout.code;
  result.code_length = layout.code_length;
  if (result.issue != Issue::none)
    return result;
  if (layout.count != profile.symbol_count) {
    result.issue = Issue::symbol_count;
    return result;
  }
  if (layout.code_length != profile.code_length) {
    result.issue = Issue::code_length;
    return result;
  }
  if (!detail::hash_range(memory, layout.code, layout.code_length, result.observed_code_hash)) {
    result.issue = Issue::code_unreadable;
    return result;
  }
  result.code_hash_available = true;
  if (result.observed_code_hash != profile.code_sha256) {
    result.issue = Issue::code_hash;
    return result;
  }
  for (std::size_t i = 0; i < profile.function_count; ++i) {
    result.issue = detail::check_function(memory, layout, profile.functions[i]);
    if (result.issue != Issue::none) {
      result.failed_symbol = profile.functions[i].symbol;
      return result;
    }
  }
  for (std::size_t i = 0; i < profile.binding_count; ++i) {
    result.issue = detail::check_binding(memory, layout, profile.bindings[i]);
    if (result.issue != Issue::none) {
      result.failed_symbol = profile.bindings[i].symbol;
      return result;
    }
  }
  result.issue = detail::stable_layout(memory, parser, profile, layout);
  result.matches = result.issue == Issue::none;
  return result;
}

// Like verify_runtime limited parser layout, but global
// deviations hide no local findings. All functions/bindings are
// read; the first local deviation is preserved. No Enroll, no
// engine calls. The caller must observe on the game thread: the
// final layout check does not make several memory reads atomic.
inline RuntimeDiagnostic
diagnose_runtime(const ReadOnlyMemory& memory, std::uint32_t parser,
                 const lhiver::Profile& profile = lhiver::profile) noexcept {
  RuntimeDiagnostic result{};
  detail::Layout layout{};
  result.layout_issue = detail::valid_profile(profile)
                            ? detail::read_layout(memory, parser, profile, layout)
                            : Issue::invalid_profile;
  result.symbol_table = layout.table;
  result.symbol_count = layout.count;
  result.code_base = layout.code;
  result.code_length = layout.code_length;
  if (result.layout_issue != Issue::none) {
    result.exact_identity_issue = result.local_contract_issue = result.layout_issue;
    result.functions_issue = result.bindings_issue = result.layout_issue;
    return result;
  }

  // A limited pass (max 64 MiB, 4096-byte buffer) for both hashes.
  // The prefix limit is also a read limit: an unreadable appended
  // region does not invalidate an already fully read hash.
  std::array<std::uint8_t, 4096> bytes{};
  Sha256 hash;
  std::uint32_t position = 0;
  while (position < layout.code_length) {
    const auto remaining = layout.code_length - position;
    auto count = remaining < bytes.size() ? remaining : static_cast<std::uint32_t>(bytes.size());
    if (layout.code_length >= profile.code_length && position < profile.code_length &&
        count > profile.code_length - position)
      count = profile.code_length - position;
    if (!memory.copy(layout.code + position, bytes.data(), count))
      break;
    hash.update(bytes.data(), count);
    position += count;
    if (position == profile.code_length) {
      result.observed_prefix_hash = hash.finish();
      result.prefix_hash_available = true;
    }
  }
  if (position == layout.code_length) {
    result.observed_code_hash = hash.finish();
    result.code_hash_available = true;
  }

  for (std::size_t i = 0; i < profile.function_count; ++i) {
    const auto issue = detail::check_function(memory, layout, profile.functions[i]);
    ++result.functions_checked;
    if (issue != Issue::none && result.functions_issue == Issue::none) {
      result.functions_issue = issue;
      result.first_failed_function = profile.functions[i].symbol;
    }
    if (issue != Issue::none && result.local_contract_issue == Issue::none) {
      result.local_contract_issue = issue;
      result.failed_symbol = profile.functions[i].symbol;
    }
  }
  for (std::size_t i = 0; i < profile.binding_count; ++i) {
    const auto issue = detail::check_binding(memory, layout, profile.bindings[i]);
    ++result.bindings_checked;
    if (issue != Issue::none && result.bindings_issue == Issue::none) {
      result.bindings_issue = issue;
      result.first_failed_binding = profile.bindings[i].symbol;
    }
    if (issue != Issue::none && result.local_contract_issue == Issue::none) {
      result.local_contract_issue = issue;
      result.failed_symbol = profile.bindings[i].symbol;
    }
  }
  result.layout_issue = detail::stable_layout(memory, parser, profile, layout);
  result.layout_stable = result.layout_issue == Issue::none;
  if (result.functions_issue == Issue::none)
    result.functions_issue = result.layout_issue;
  if (result.bindings_issue == Issue::none)
    result.bindings_issue = result.layout_issue;
  result.functions_match = result.layout_stable && result.functions_issue == Issue::none;
  result.bindings_match = result.layout_stable && result.bindings_issue == Issue::none;
  if (result.local_contract_issue == Issue::none)
    result.local_contract_issue = result.layout_issue;
  result.local_contract_matches = result.functions_match && result.bindings_match;
  result.original_prefix_matches = result.layout_stable && result.prefix_hash_available &&
                                   result.observed_prefix_hash == profile.code_sha256;

  // Same rejection order as the unchanged strict checker; additional
  // symbols and additional code are never accepted as exact identity.
  if (layout.count != profile.symbol_count)
    result.exact_identity_issue = Issue::symbol_count;
  else if (layout.code_length != profile.code_length)
    result.exact_identity_issue = Issue::code_length;
  else if (!result.code_hash_available)
    result.exact_identity_issue = Issue::code_unreadable;
  else if (result.observed_code_hash != profile.code_sha256)
    result.exact_identity_issue = Issue::code_hash;
  else
    result.exact_identity_issue = result.local_contract_issue;
  result.exact_identity_matches = result.exact_identity_issue == Issue::none;
  return result;
}

// Structurally limited lookup. The meaning of any index
// must be verified by the caller beforehand via the profile.
inline SymbolAddress
read_symbol_address(const ReadOnlyMemory& memory, std::uint32_t parser, std::uint32_t index,
                    const lhiver::Profile& profile = lhiver::profile) noexcept {
  SymbolAddress result{};
  if (!detail::valid_profile(profile)) {
    result.issue = Issue::invalid_profile;
    return result;
  }
  detail::Layout layout{};
  detail::Symbol symbol{};
  result.issue = detail::read_layout(memory, parser, profile, layout);
  if (result.issue == Issue::none)
    result.issue = detail::lookup(memory, layout, index, symbol);
  if (result.issue == Issue::none)
    result.issue = detail::stable_layout(memory, parser, profile, layout);
  result.valid = result.issue == Issue::none;
  if (result.valid)
    result.address = symbol.address;
  return result;
}

inline SymbolInt symbol_int(const ReadOnlyMemory& memory, std::uint32_t parser, std::uint32_t index,
                            const lhiver::Profile& profile = lhiver::profile) noexcept {
  SymbolInt result{};
  if (!detail::valid_profile(profile)) {
    result.issue = Issue::invalid_profile;
    return result;
  }
  detail::Layout layout{};
  detail::Symbol symbol{};
  result.issue = detail::read_layout(memory, parser, profile, layout);
  if (result.issue == Issue::none)
    result.issue = detail::lookup(memory, layout, index, symbol);
  if (result.issue != Issue::none)
    return result;
  if (symbol.type != 2 || symbol.elements != 1 || (symbol.flags != 0 && symbol.flags != 1)) {
    result.issue = Issue::not_scalar_int;
    return result;
  }
  result.issue = detail::stable_layout(memory, parser, profile, layout);
  result.valid = result.issue == Issue::none;
  if (result.valid)
    result.value = static_cast<std::int32_t>(symbol.data);
  return result;
}

// Native non-member INT arrays: GetDataAdr 0x7A1CD2..0x7A1CE6 reads
// the data pointer from +0x18 when elements>1, then +4*index.
// Scalars store their value directly at Symbol+0x18 instead.
inline SymbolInt symbol_int_element(const ReadOnlyMemory& memory, std::uint32_t parser,
                                    std::uint32_t symbol_index, std::uint32_t element,
                                    const lhiver::Profile& profile = lhiver::profile) noexcept {
  SymbolInt result{};
  if (!detail::valid_profile(profile)) {
    result.issue = Issue::invalid_profile;
    return result;
  }
  detail::Layout layout{};
  detail::Symbol symbol{};
  result.issue = detail::read_layout(memory, parser, profile, layout);
  if (result.issue == Issue::none)
    result.issue = detail::lookup(memory, layout, symbol_index, symbol);
  if (result.issue != Issue::none)
    return result;
  if (symbol.type != 2 || symbol.parent || symbol.flags > 1 || element >= symbol.elements) {
    result.issue = Issue::symbol_signature;
    return result;
  }
  std::uint32_t value = symbol.data;
  if (symbol.elements > 1) {
    const auto address =
        static_cast<std::uint64_t>(symbol.data) + 4u * static_cast<std::uint64_t>(element);
    if (!detail::range(address, 4) ||
        !memory.copy(static_cast<std::uint32_t>(address), &value, 4)) {
      result.issue = Issue::symbol_unreadable;
      return result;
    }
  }
  result.issue = detail::stable_layout(memory, parser, profile, layout);
  result.valid = result.issue == Issue::none;
  if (result.valid)
    result.value = static_cast<std::int32_t>(value);
  return result;
}

struct RuntimeContract {
  bool matches = false;
  Issue issue = Issue::none;
  std::uint32_t failed_symbol = 0;
};

// Explicit local contract without full code hash. Covers only the
// listed functions/bindings; no claim of complete identity. For a
// frequent UI gate, do not hash several MiB of library code per frame.
inline RuntimeContract verify_local_contract(const ReadOnlyMemory& memory, std::uint32_t parser,
                                             const lhiver::Profile& profile) noexcept {
  RuntimeContract result;
  if (!detail::valid_profile(profile)) {
    result.issue = Issue::invalid_profile;
    return result;
  }
  detail::Layout layout{};
  result.issue = detail::read_layout(memory, parser, profile, layout);
  if (result.issue != Issue::none)
    return result;
  for (std::size_t i = 0; i < profile.function_count; ++i) {
    result.issue = detail::check_function(memory, layout, profile.functions[i]);
    if (result.issue != Issue::none) {
      result.failed_symbol = profile.functions[i].symbol;
      return result;
    }
  }
  for (std::size_t i = 0; i < profile.binding_count; ++i) {
    result.issue = detail::check_binding(memory, layout, profile.bindings[i]);
    if (result.issue != Issue::none) {
      result.failed_symbol = profile.bindings[i].symbol;
      return result;
    }
  }
  result.issue = detail::stable_layout(memory, parser, profile, layout);
  result.matches = result.issue == Issue::none;
  return result;
}

} // namespace ubc::runtime_script
