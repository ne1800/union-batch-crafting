// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/core/sha256.hpp"
#include "ubc/engine/read_only_memory.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

// UBCF1: position-independent canonical form of one Daedalus function.
// Symbol indices become names, call entries become callee names and jump
// targets become offsets relative to the function start. SHA-256 over the
// canonical stream is the fingerprint. tools/profile/fingerprint.py is the
// reference implementation and must produce identical digests.
//
// Stream (integers little-endian, str = u32 length followed by the bytes):
//   "UBCF1" 'H' str(name) u32(type) u32(flags) u32(argc) u32(return_type)
//   str(parent) u32(byte_length), then per instruction u8(opcode) and
//     PUSH_INT literal       'I' i32(value)
//     PUSH_INT symbolic      symbol operand
//     CALL                   'C' str(callee)
//     JUMP, JUMPF            'J' u32(target - start)
//     other symbol operands  symbol operand
//     PUSH_ARRAY_VAR         symbol operand u8(element)
//   symbol operand: 'S' str(name) | 'A' u32(text length) sha256(text) | 'T'
// return_type is the symbol offset for FUNC (type 5) and 0 otherwise. An
// empty parent means none. 'T' replaces a text constant when the reference
// ignores text content. Only the reference decides whether a PUSH_INT is a
// symbol index; the value alone never does.
//
// No engine calls and no writes. The body is read through ReadOnlyMemory.
// Any violation makes the result invalid; an invalid result never matches.
namespace ubc::script_fingerprint {

using Digest = std::array<std::uint8_t, 32>;

enum class Issue {
  none,
  invalid_input,
  function_length,
  code_unreadable,
  unknown_opcode,
  truncated_instruction,
  jump_target,
  missing_ret,
  symbolic_int,
  unknown_symbol,
  unknown_callee,
  text_unreadable,
  invalid_profile,
  capacity,
  wrong_parser,
  parser_unreadable,
  symbol_table_range,
  code_range,
  symbol_unreadable,
  name_unreadable,
  missing_name,
  duplicate_name,
  symbol_signature,
  symbol_parent,
  symbol_value,
  function_range,
  fingerprint_mismatch,
  changed_during_read,
  not_evaluated
};

inline const char* issue_name(Issue issue) noexcept {
  switch (issue) {
  case Issue::none:
    return "ok";
  case Issue::invalid_input:
    return "invalid_input";
  case Issue::function_length:
    return "function_length";
  case Issue::code_unreadable:
    return "code_unreadable";
  case Issue::unknown_opcode:
    return "unknown_opcode";
  case Issue::truncated_instruction:
    return "truncated_instruction";
  case Issue::jump_target:
    return "jump_target";
  case Issue::missing_ret:
    return "missing_ret";
  case Issue::symbolic_int:
    return "symbolic_int";
  case Issue::unknown_symbol:
    return "unknown_symbol";
  case Issue::unknown_callee:
    return "unknown_callee";
  case Issue::text_unreadable:
    return "text_unreadable";
  case Issue::invalid_profile:
    return "invalid_profile";
  case Issue::capacity:
    return "capacity";
  case Issue::wrong_parser:
    return "wrong_parser";
  case Issue::parser_unreadable:
    return "parser_unreadable";
  case Issue::symbol_table_range:
    return "symbol_table_range";
  case Issue::code_range:
    return "code_range";
  case Issue::symbol_unreadable:
    return "symbol_unreadable";
  case Issue::name_unreadable:
    return "name_unreadable";
  case Issue::missing_name:
    return "missing_name";
  case Issue::duplicate_name:
    return "duplicate_name";
  case Issue::symbol_signature:
    return "symbol_signature";
  case Issue::symbol_parent:
    return "symbol_parent";
  case Issue::symbol_value:
    return "symbol_value";
  case Issue::function_range:
    return "function_range";
  case Issue::fingerprint_mismatch:
    return "fingerprint_mismatch";
  case Issue::changed_during_read:
    return "changed_during_read";
  case Issue::not_evaluated:
    return "not_evaluated";
  }
  return "unknown_issue";
}

// Recipe functions are small; the limit bounds the boundary bitmaps.
inline constexpr std::uint32_t max_function_length = 64u * 1024u;
inline constexpr std::uint32_t max_text_length = 64u * 1024u;

enum class OperandKind : std::uint8_t { name, text };

struct Operand {
  OperandKind kind = OperandKind::name;
  std::string_view name{};
  std::uint32_t text_length = 0;
  Digest text_sha256{};
};

// Both callbacks receive raw operands from the live code. symbol maps a
// symbol index to its name or to the content of an anonymous text constant;
// entry maps a code offset to the one function whose entry is exactly there.
struct Resolver {
  void* context = nullptr;
  Issue (*symbol)(void*, std::uint32_t, Operand&) noexcept = nullptr;
  Issue (*entry)(void*, std::uint32_t, Operand&) noexcept = nullptr;
};

struct Header {
  std::string_view name{};
  std::uint32_t type = 5;
  std::uint32_t flags = 1;
  std::uint32_t argc = 0;
  std::uint32_t return_type = 0;
  std::string_view parent{};
};

// Reference facts for one function. symbolic_ints lists the relative
// offsets of every PUSH_INT whose operand is a symbol index, ascending.
struct Spec {
  std::uint32_t byte_length = 0;
  const std::uint32_t* symbolic_ints = nullptr;
  std::size_t symbolic_int_count = 0;
  bool ignore_text = false;
};

// Caller-owned bitmaps so the fingerprint needs neither heap nor a large
// stack frame. One scratch serves any number of sequential calls.
struct Scratch {
  std::array<std::uint8_t, max_function_length / 8> starts{};
  std::array<std::uint8_t, max_function_length / 8> targets{};
};

struct Result {
  Issue issue = Issue::not_evaluated;
  Digest canonical{};
  Digest raw{};
  std::uint32_t instructions = 0;
  // Relative offset of the offending instruction or jump target.
  std::uint32_t failed_offset = 0;
  bool valid() const noexcept { return issue == Issue::none; }
};

namespace detail {
inline constexpr std::uint8_t op_ret = 60;
inline constexpr std::uint8_t op_call = 61;
inline constexpr std::uint8_t op_push_int = 64;
inline constexpr std::uint8_t op_jump = 75;
inline constexpr std::uint8_t op_jumpf = 76;
inline constexpr std::uint8_t op_push_array = 245;

// Instruction size in bytes, 0 for an opcode outside the Gothic II table.
constexpr std::uint32_t instruction_size(std::uint8_t opcode) noexcept {
  switch (opcode) {
  case 61:
  case 62:
  case 64:
  case 65:
  case 66:
  case 67:
  case 68:
  case 75:
  case 76:
  case 80:
    return 5;
  case 245:
    return 6;
  default:
    break;
  }
  const bool simple = opcode <= 9 || (opcode >= 11 && opcode <= 22) ||
                      (opcode >= 30 && opcode <= 33) || opcode == 60 || opcode == 63 ||
                      (opcode >= 69 && opcode <= 74);
  return simple ? 1u : 0u;
}

inline std::uint32_t le32(const std::uint8_t* bytes) noexcept {
  return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8u) |
         (static_cast<std::uint32_t>(bytes[2]) << 16u) |
         (static_cast<std::uint32_t>(bytes[3]) << 24u);
}

class Stream {
public:
  void byte(std::uint8_t value) noexcept { hash_.update(&value, 1); }
  void word(std::uint32_t value) noexcept {
    const std::array<std::uint8_t, 4> bytes{
        {static_cast<std::uint8_t>(value), static_cast<std::uint8_t>(value >> 8u),
         static_cast<std::uint8_t>(value >> 16u), static_cast<std::uint8_t>(value >> 24u)}};
    hash_.update(bytes.data(), bytes.size());
  }
  void text(std::string_view value) noexcept {
    word(static_cast<std::uint32_t>(value.size()));
    hash_.update(value.data(), value.size());
  }
  void digest(const Digest& value) noexcept { hash_.update(value.data(), value.size()); }
  Digest finish() const noexcept { return hash_.finish(); }

private:
  Sha256 hash_;
};

inline void set_bit(std::uint8_t* bits, std::uint32_t index) noexcept {
  bits[index / 8u] = static_cast<std::uint8_t>(bits[index / 8u] | (1u << (index % 8u)));
}

inline Issue symbol_token(Stream& stream, const Resolver& resolver, std::uint32_t index,
                          bool ignore_text) noexcept {
  Operand operand{};
  const auto issue = resolver.symbol(resolver.context, index, operand);
  if (issue != Issue::none)
    return issue;
  if (operand.kind == OperandKind::text) {
    if (ignore_text) {
      stream.byte('T');
    } else {
      stream.byte('A');
      stream.word(operand.text_length);
      stream.digest(operand.text_sha256);
    }
    return Issue::none;
  }
  if (operand.name.empty())
    return Issue::unknown_symbol;
  stream.byte('S');
  stream.text(operand.name);
  return Issue::none;
}

inline bool valid_spec(const Spec& spec) noexcept {
  if (spec.symbolic_int_count && !spec.symbolic_ints)
    return false;
  for (std::size_t i = 0; i < spec.symbolic_int_count; ++i) {
    if (spec.symbolic_ints[i] >= spec.byte_length ||
        (i && spec.symbolic_ints[i] <= spec.symbolic_ints[i - 1]))
      return false;
  }
  return true;
}
} // namespace detail

// Canonicalizes code[start, start + byte_length). The code is read in
// bounded chunks; raw receives SHA-256 of exactly the same bytes.
inline Result fingerprint(const ReadOnlyMemory& memory, std::uint32_t code_base,
                          std::uint32_t start, const Header& header, const Spec& spec,
                          const Resolver& resolver, Scratch& scratch) noexcept {
  Result result{};
  const auto fail = [&result](Issue issue, std::uint32_t offset) noexcept {
    result.issue = issue;
    result.failed_offset = offset;
    return result;
  };
  if (header.name.empty() || !resolver.symbol || !resolver.entry || !detail::valid_spec(spec))
    return fail(Issue::invalid_input, 0);
  const auto length = spec.byte_length;
  if (length == 0 || length > max_function_length)
    return fail(Issue::function_length, 0);
  const auto address = static_cast<std::uint64_t>(code_base) + start;
  if (code_base == 0 || address + length - 1u > 0xffffffffu)
    return fail(Issue::code_unreadable, 0);
  const std::size_t bitmap = (length + 7u) / 8u;
  std::memset(scratch.starts.data(), 0, bitmap);
  std::memset(scratch.targets.data(), 0, bitmap);

  detail::Stream stream;
  Sha256 raw;
  stream.byte('U');
  stream.byte('B');
  stream.byte('C');
  stream.byte('F');
  stream.byte('1');
  stream.byte('H');
  stream.text(header.name);
  stream.word(header.type);
  stream.word(header.flags);
  stream.word(header.argc);
  stream.word(header.return_type);
  stream.text(header.parent);
  stream.word(length);

  std::array<std::uint8_t, 4096> window{};
  std::uint32_t window_begin = 0;
  std::uint32_t window_end = 0;
  std::size_t next_symbolic = 0;
  std::uint8_t last_opcode = 0;
  std::uint32_t position = 0;
  while (position < length) {
    if (position >= window_end || (window_end < length && window_end - position < 6u)) {
      const auto remaining = length - position;
      const auto count =
          remaining < window.size() ? remaining : static_cast<std::uint32_t>(window.size());
      if (!memory.copy(static_cast<std::uint32_t>(address + position), window.data(), count))
        return fail(Issue::code_unreadable, position);
      window_begin = position;
      window_end = position + count;
    }
    const auto* bytes = window.data() + (position - window_begin);
    const auto opcode = bytes[0];
    const auto size = detail::instruction_size(opcode);
    if (size == 0)
      return fail(Issue::unknown_opcode, position);
    if (size > length - position)
      return fail(Issue::truncated_instruction, position);
    if (next_symbolic < spec.symbolic_int_count && spec.symbolic_ints[next_symbolic] < position)
      return fail(Issue::symbolic_int, spec.symbolic_ints[next_symbolic]);
    const bool symbolic =
        next_symbolic < spec.symbolic_int_count && spec.symbolic_ints[next_symbolic] == position;
    if (symbolic && opcode != detail::op_push_int)
      return fail(Issue::symbolic_int, position);
    detail::set_bit(scratch.starts.data(), position);
    raw.update(bytes, size);
    stream.byte(opcode);
    const auto operand = size > 1 ? detail::le32(bytes + 1) : 0u;
    Issue issue = Issue::none;
    if (opcode == detail::op_push_int) {
      if (symbolic) {
        ++next_symbolic;
        issue = detail::symbol_token(stream, resolver, operand, spec.ignore_text);
      } else {
        stream.byte('I');
        stream.word(operand);
      }
    } else if (opcode == detail::op_call) {
      Operand callee{};
      issue = resolver.entry(resolver.context, operand, callee);
      if (issue == Issue::none && (callee.kind != OperandKind::name || callee.name.empty()))
        issue = Issue::unknown_callee;
      if (issue == Issue::none) {
        stream.byte('C');
        stream.text(callee.name);
      }
    } else if (opcode == detail::op_jump || opcode == detail::op_jumpf) {
      if (operand < start || operand - start >= length)
        return fail(Issue::jump_target, position);
      detail::set_bit(scratch.targets.data(), operand - start);
      stream.byte('J');
      stream.word(operand - start);
    } else if (size > 1) {
      issue = detail::symbol_token(stream, resolver, operand, spec.ignore_text);
      if (issue == Issue::none && opcode == detail::op_push_array)
        stream.byte(bytes[5]);
    }
    if (issue != Issue::none)
      return fail(issue, position);
    last_opcode = opcode;
    ++result.instructions;
    position += size;
  }
  if (next_symbolic != spec.symbolic_int_count)
    return fail(Issue::symbolic_int, spec.symbolic_ints[next_symbolic]);
  if (last_opcode != detail::op_ret)
    return fail(Issue::missing_ret, length);
  for (std::size_t i = 0; i < bitmap; ++i) {
    const auto stray = static_cast<std::uint8_t>(scratch.targets[i] & ~scratch.starts[i]);
    if (stray == 0)
      continue;
    std::uint32_t bit = 0;
    while (((stray >> bit) & 1u) == 0)
      ++bit;
    return fail(Issue::jump_target, static_cast<std::uint32_t>(i * 8u + bit));
  }
  result.canonical = stream.finish();
  result.raw = raw.finish();
  result.issue = Issue::none;
  return result;
}

} // namespace ubc::script_fingerprint
