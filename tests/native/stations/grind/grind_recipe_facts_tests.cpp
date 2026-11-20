// SPDX-License-Identifier: MIT
// Pinned original data arrives exclusively through an in-memory pipe.
#include "../../helpers/grind_runtime_fixture.hpp"
#include <cstdlib>
#include <exception>
#include <iostream>

namespace {
using namespace grind_test;
[[noreturn]] void input_error() {
  std::fputs("grind_recipe_facts_tests: fehlende oder ungueltige Faktenpipe\n", stderr);
  std::exit(2);
}
std::uint32_t number() {
  std::array<unsigned char, 4> bytes{};
  if (!std::cin.read(reinterpret_cast<char*>(bytes.data()), 4))
    input_error();
  return std::uint32_t{bytes[0]} | (std::uint32_t{bytes[1]} << 8) |
         (std::uint32_t{bytes[2]} << 16) | (std::uint32_t{bytes[3]} << 24);
}
std::vector<std::uint8_t> bytes(std::size_t count) {
  if (count > 100000)
    input_error();
  std::vector<std::uint8_t> out(count);
  if (count &&
      !std::cin.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(count)))
    input_error();
  return out;
}
struct Symbol {
  std::uint32_t type = 0, flags = 0, elements = 0;
  std::int32_t parent = -1, offset = 0, scalar = 0, entry = -1;
  std::string name;
};
struct Facts {
  std::map<std::uint32_t, Symbol> symbols;
  std::map<std::uint32_t, std::vector<std::uint8_t>> functions, texts;
  Facts() {
    const auto magic = bytes(8);
    if (std::memcmp(magic.data(), "UBCGFP2\n", 8))
      input_error();
    const auto count = number();
    if (!count || count > 2048)
      input_error();
    for (std::uint32_t i = 0; i < count; ++i) {
      const auto id = number();
      Symbol s;
      s.type = number();
      s.flags = number();
      s.elements = number();
      s.parent = static_cast<std::int32_t>(number());
      s.offset = static_cast<std::int32_t>(number());
      s.scalar = static_cast<std::int32_t>(number());
      const auto name = bytes(number());
      s.name.assign(name.begin(), name.end());
      s.entry = static_cast<std::int32_t>(number());
      if (!symbols.emplace(id, std::move(s)).second)
        input_error();
    }
    const auto bodies = number();
    if (bodies != 81)
      input_error();
    for (std::uint32_t i = 0; i < bodies; ++i) {
      const auto id = number();
      const auto value = bytes(number());
      if (!functions.emplace(id, value).second)
        input_error();
    }
    const auto strings = number();
    if (!strings || strings > 512)
      input_error();
    for (std::uint32_t i = 0; i < strings; ++i) {
      const auto id = number();
      const auto value = bytes(number());
      if (!texts.emplace(id, value).second)
        input_error();
    }
    if (std::cin.peek() != std::char_traits<char>::eof())
      input_error();
  }
};
#ifndef UBC_GRIND_FACTS_FIXTURE_ONLY
void static_profile(const ubc::lhiver::Profile& profile, const Facts& local) {
  check(rs::detail::valid_profile(profile), "jedes echte Teilprofil bleibt begrenzt");
  for (std::size_t i = 0; i < profile.function_count; ++i) {
    const auto& f = profile.functions[i];
    const auto& s = local.symbols.at(f.symbol);
    const auto& raw = local.functions.at(f.symbol);
    check(s.name == f.name && s.type == f.type && s.flags == f.flags && s.elements == f.argc &&
              s.parent == f.parent_index && s.offset == static_cast<std::int32_t>(f.return_type) &&
              s.entry == static_cast<std::int32_t>(f.byte_offset),
          "Funktionsmetadaten direkt aus dem DAT");
    ubc::Sha256 hash;
    hash.update(raw.data(), raw.size());
    check(raw.size() == f.byte_length && hash.finish() == f.body_sha256, "Originalkoerperhash");
  }
  for (std::size_t i = 0; i < profile.binding_count; ++i) {
    const auto& b = profile.bindings[i];
    const auto& s = local.symbols.at(b.symbol);
    check(s.name == b.name && s.type == b.type && s.flags == b.flags && s.elements == b.elements &&
              s.parent == b.parent_index && (!b.check_value || s.scalar == b.value) &&
              (!b.check_offset || s.offset == b.offset),
          "Bindingmetadaten unabhaengig im DAT");
  }
}
#endif
std::uint32_t text_base(std::uint32_t symbol_id) { return 0x60000000u + symbol_id * 0x200u; }
void load_original(Fixture& fixture, const Facts& local) {
  for (const auto& pair : local.functions) {
    const auto& raw = pair.second;
    const auto& s = local.symbols.at(pair.first);
    const auto address = code + static_cast<std::uint32_t>(s.entry);
    if (auto* dest = fixture.image.at(address, raw.size()))
      std::memcpy(dest, raw.data(), raw.size());
  }
  for (const auto& pair : local.texts) {
    const auto id = pair.first;
    const auto& raw = pair.second;
    const auto& s = local.symbols.at(id);
    check(s.type == 3 && s.flags == 1 && s.elements == 1 && s.parent == -1,
          "Originaltext ist konstante skalare STRING");
    const auto address = text_base(id);
    if (!fixture.image.at(symbol(id), 0x3c))
      fixture.make(id, s.name.c_str(), s.elements, s.type, s.flags, address, 0, s.parent);
    fixture.image.put(symbol(id) + 0x18, address);
    fixture.image.block(address, 20);
    fixture.image.block(address + 32, raw.size() + 1);
    fixture.image.put(address + 8, address + 32);
    fixture.image.put(address + 12, static_cast<std::uint32_t>(raw.size()));
    fixture.image.put(address + 16, static_cast<std::uint32_t>(raw.size() + 1));
    if (!raw.empty())
      std::memcpy(fixture.image.at(address + 32, raw.size()), raw.data(), raw.size());
  }
}
}
#ifndef UBC_GRIND_FACTS_FIXTURE_ONLY
int main(int argc, char** argv) {
  // Without explicit RAM facts mode, a case that runs zero checks must not pass.
  if (argc != 2 || std::strcmp(argv[1], "--facts-stdin"))
    return 2;
  try {
    const Facts local;
    for (const auto& descriptor : g::recipes) {
      Fixture fixture(descriptor);
      load_original(fixture, local);
      const auto profile = fixture.assembly.profile();
      static_profile(profile, local);
      std::vector<const ubc::lhiver::Function*> bodies;
      std::vector<const facts::Text*> texts;
      for (std::size_t i = 0; i < profile.function_count; ++i)
        bodies.push_back(&profile.functions[i]);
      for (const auto& text : facts::common_texts)
        texts.push_back(&text);
      for (const auto& text : facts::texts_for(descriptor.function.symbol)->texts)
        texts.push_back(&text);
      for (const auto id : {descriptor.input, descriptor.output, g::lifecycle_item}) {
        const auto* item = facts::item_for(static_cast<std::uint32_t>(id));
        static_profile(runtime::item_profile(*item), local);
        bodies.push_back(item->function);
        for (std::size_t i = 0; i < item->text_count; ++i)
          texts.push_back(&item->texts[i]);
      }
      const auto result =
          runtime::verify_recipe(fixture.image.memory(), ubc::lhiver::game_parser, descriptor);
      check(result.valid, "positives vollstaendiges Originalprofil im RAM");
      if (!result.valid) {
        std::fprintf(stderr, "DETAIL recipe=%u local=%u symbol=%u text=%u failed=%u\n",
                     descriptor.function.symbol, static_cast<unsigned>(result.local.issue),
                     result.local.failed_symbol, static_cast<unsigned>(result.string_issue),
                     result.failed_symbol);
        return 1;
      }
      for (const auto* f : bodies) {
        auto* b = fixture.image.at(code + f->byte_offset, 1);
        *b ^= 1;
        check(!runtime::verify_recipe(fixture.image.memory(), ubc::lhiver::game_parser, descriptor)
                   .valid,
              "jede Codekoerpermutation gesperrt");
        *b ^= 1;
      }
      for (const auto* text : texts) {
        ubc::Sha256 hash;
        const auto& raw = local.texts.at(text->symbol);
        hash.update(raw.data(), raw.size());
        check(raw.size() == text->length && hash.finish() == text->sha256,
              "konkrete Originaltextbindung");
        auto* b = fixture.image.at(text_base(text->symbol) + 32, 1);
        *b ^= 1;
        check(!runtime::verify_recipe(fixture.image.memory(), ubc::lhiver::game_parser, descriptor)
                   .valid,
              "jede Textmutation gesperrt");
        *b ^= 1;
      }
      fixture.image.change_address = symbol(1549);
      fixture.image.change_target = symbol(1549) + 0x1c;
      fixture.image.change_on_read = 2;
      check(!runtime::verify_recipe(fixture.image.memory(), ubc::lhiver::game_parser, descriptor)
                 .valid,
            "spaet wechselnder Gildenmembervertrag");
    }
    std::printf("grind_recipe_facts_tests: %u checks, %u failures\n", station_test::checks,
                station_test::failures);
    return station_test::failures ? 1 : 0;
  } catch (const std::exception&) {
    input_error();
  }
}
#endif
