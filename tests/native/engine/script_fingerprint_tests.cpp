// SPDX-License-Identifier: MIT
#include "../helpers/parser_image.hpp"
#include "ubc/engine/script_fingerprint.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

namespace {
namespace fp = ubc::script_fingerprint;
using fp::Issue;
unsigned checks = 0;
unsigned failures = 0;

void require(bool condition, const char* name) {
  ++checks;
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}

// Shared with tests/python/profile/test_fingerprint.py. Both implementations
// must reproduce these digests for the same synthetic golden function.
constexpr const char* golden = "e9d47319934443fa81ae089ec9508f35882ace20ec13b2f0ebfd23da95a77acc";
constexpr const char* golden_without_text =
    "c20427c8ec8fc151436a48aaf619dff4fd35d821af290ba83117da6c1046f030";
constexpr const char* golden_raw =
    "42e2a31f167712d60d0b951d28f411d97e239f47badcaab13d1e1e0255dbf9c6";
constexpr std::uint32_t code_base = 0x4000;
constexpr std::uint32_t start = 100;

std::string hex(const fp::Digest& digest) {
  std::string text;
  for (const auto byte : digest) {
    std::array<char, 3> pair{};
    std::snprintf(pair.data(), pair.size(), "%02x", byte);
    text += pair.data();
  }
  return text;
}

void word(std::vector<std::uint8_t>& code, std::uint8_t opcode, std::uint32_t value) {
  code.push_back(opcode);
  for (unsigned i = 0; i < 4; ++i)
    code.push_back(static_cast<std::uint8_t>(value >> (8u * i)));
}

std::vector<std::uint8_t> golden_body(std::uint32_t at = start) {
  std::vector<std::uint8_t> code;
  word(code, 64, 7);
  word(code, 64, 1234); // symbolic: ITFO_APPLE
  word(code, 62, 3);    // CREATEINVITEMS
  word(code, 65, 4);    // anonymous text "Hallo Welt"
  word(code, 61, 500);  // B_SAY
  word(code, 245, 5);   // UBC_ARRAY[2]
  code.push_back(2);
  word(code, 76, at + 42);
  code.push_back(9);
  word(code, 75, at + 43);
  code.push_back(60);
  code.push_back(60);
  return code;
}

struct Golden {
  ubc::test::Memory image;
  std::map<std::uint32_t, std::string> names{
      {1234, "ITFO_APPLE"}, {3, "CREATEINVITEMS"}, {5, "UBC_ARRAY"}};
  std::map<std::uint32_t, std::string> texts{{4, "Hallo Welt"}};
  std::map<std::uint32_t, std::string> entries{{500, "B_SAY"}};
  std::vector<std::uint32_t> symbolic{5};
  std::uint32_t length = 0;
  fp::Scratch scratch;

  explicit Golden(const std::vector<std::uint8_t>& body = golden_body()) { load(body); }
  void load(const std::vector<std::uint8_t>& body) {
    image.bytes.assign(code_base + start + body.size() + 64u, 60);
    std::copy(body.begin(), body.end(), image.bytes.begin() + code_base + start);
    length = static_cast<std::uint32_t>(body.size());
  }
  static Issue symbol(void* context, std::uint32_t index, fp::Operand& operand) noexcept {
    const auto& self = *static_cast<const Golden*>(context);
    const auto text = self.texts.find(index);
    if (text != self.texts.end()) {
      ubc::Sha256 hash;
      hash.update(text->second.data(), text->second.size());
      operand.kind = fp::OperandKind::text;
      operand.text_length = static_cast<std::uint32_t>(text->second.size());
      operand.text_sha256 = hash.finish();
      return Issue::none;
    }
    const auto name = self.names.find(index);
    if (name == self.names.end())
      return Issue::unknown_symbol;
    operand.name = name->second;
    return Issue::none;
  }
  static Issue entry(void* context, std::uint32_t offset, fp::Operand& operand) noexcept {
    const auto& self = *static_cast<const Golden*>(context);
    const auto found = self.entries.find(offset);
    if (found == self.entries.end())
      return Issue::unknown_callee;
    operand.name = found->second;
    return Issue::none;
  }
  fp::Result run(bool ignore_text = false, std::uint32_t at = start) {
    fp::Header header{};
    header.name = "UBC_GOLDEN";
    const fp::Spec spec{length, symbolic.data(), symbolic.size(), ignore_text};
    const fp::Resolver resolver{this, &symbol, &entry};
    return fp::fingerprint(image.memory(), code_base, at, header, spec, resolver, scratch);
  }
};

void test_golden() {
  Golden g;
  const auto result = g.run();
  require(result.valid() && hex(result.canonical) == golden,
          "C++ matches the Python golden digest");
  require(hex(result.raw) == golden_raw && result.instructions == 11, "golden raw digest and size");
  require(hex(g.run(true).canonical) == golden_without_text, "golden digest without text");
  g.texts[4] = "Hallo Welt!";
  require(hex(g.run().canonical) != golden, "text content enters the fingerprint");
  require(hex(g.run(true).canonical) == golden_without_text, "ignored text keeps the fingerprint");
  require(std::strcmp(fp::issue_name(Issue::jump_target), "jump_target") == 0, "ASCII issue names");
}

void test_moved_function() {
  constexpr std::uint32_t moved = 900;
  Golden g(golden_body(moved));
  g.image.bytes.assign(code_base + moved + g.length + 64u, 60);
  const auto body = golden_body(moved);
  std::copy(body.begin(), body.end(), g.image.bytes.begin() + code_base + moved);
  const auto result = g.run(false, moved);
  require(result.valid() && hex(result.canonical) == golden, "moved function keeps the digest");
  require(hex(result.raw) != golden_raw, "moved function changes the raw digest");
}

void expect(Golden& g, Issue issue, std::uint32_t offset, const char* name) {
  const auto result = g.run();
  require(!result.valid() && result.issue == issue && result.failed_offset == offset, name);
}

void test_rejections() {
  const auto body = golden_body();
  {
    auto changed = body;
    changed[32] = static_cast<std::uint8_t>(start + 44);
    Golden g(changed);
    expect(g, Issue::jump_target, 31, "jump to the function end is rejected");
  }
  {
    auto changed = body;
    changed[32] = static_cast<std::uint8_t>(start + 38);
    Golden g(changed);
    expect(g, Issue::jump_target, 38, "jump into an instruction is rejected");
  }
  {
    std::vector<std::uint8_t> backwards;
    word(backwards, 75, start - 1);
    backwards.push_back(60);
    Golden g(backwards);
    g.symbolic.clear();
    expect(g, Issue::jump_target, 0, "jump before the function is rejected");
  }
  {
    auto changed = body;
    changed[36] = 10;
    Golden g(changed);
    expect(g, Issue::unknown_opcode, 36, "unknown opcode is rejected");
  }
  {
    Golden g(std::vector<std::uint8_t>(body.begin(), body.begin() + 3));
    g.symbolic.clear();
    expect(g, Issue::truncated_instruction, 0, "instruction across the end is rejected");
  }
  {
    std::vector<std::uint8_t> open;
    word(open, 64, 1);
    open.push_back(60);
    open.push_back(9);
    Golden g(open);
    g.symbolic.clear();
    expect(g, Issue::missing_ret, 7, "function without final RET is rejected");
  }
  {
    Golden g;
    g.symbolic = {5, 10};
    expect(g, Issue::symbolic_int, 10, "symbolic offset must be a PUSH_INT");
    g.symbolic = {5, 6};
    expect(g, Issue::symbolic_int, 6, "symbolic offset must be an instruction start");
    g.symbolic = {10, 5};
    expect(g, Issue::invalid_input, 0, "symbolic offsets must ascend");
  }
  {
    Golden g;
    g.entries = {{501, "B_SAY"}};
    expect(g, Issue::unknown_callee, 20, "unknown callee is rejected");
  }
  {
    Golden g;
    g.names.erase(5);
    expect(g, Issue::unknown_symbol, 25, "unknown symbol is rejected");
  }
  {
    Golden g;
    g.image.deny(code_base + start + 20, 1);
    expect(g, Issue::code_unreadable, 0, "denied code read is rejected");
  }
  {
    Golden g;
    g.length = 0;
    g.symbolic.clear();
    expect(g, Issue::function_length, 0, "empty function is rejected");
    g.length = fp::max_function_length + 1;
    expect(g, Issue::function_length, 0, "oversized function is rejected");
  }
}

void test_chunked_read() {
  // A body longer than the read window must decode across chunk borders.
  std::vector<std::uint8_t> body;
  for (unsigned i = 0; i < 1700; ++i)
    word(body, 64, i);
  body.push_back(60);
  Golden g(body);
  g.symbolic.clear();
  const auto first = g.run();
  require(first.valid() && first.instructions == 1701, "long body decodes across chunks");
  body[5 * 1000 + 1] = 0xff;
  g.load(body);
  require(g.run().valid() && g.run().canonical != first.canonical,
          "literal change in a later chunk changes the digest");
}
} // namespace

int main() {
  test_golden();
  test_moved_function();
  test_rejections();
  test_chunked_read();
  std::printf("script fingerprint: %u checks, %u failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
