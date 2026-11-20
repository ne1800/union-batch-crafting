// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/grind/grind_recipe_runtime.hpp"
#define UBC_STATION_RECIPE_FIXTURE_ONLY
#include "station_recipe_runtime_tests.cpp"

namespace grind_test {
namespace runtime = ubc::grind_recipe_runtime;
namespace facts = ubc::grind_recipe_profile;
namespace g = ubc::grind_recipe;
namespace rs = ubc::runtime_script;
using station_test::check;
using station_test::Image;
using station_test::symbol;
using station_test::name_at;
using station_test::code;
using station_test::table;
struct Fixture {
  Image image;
  runtime::ProfileAssembly assembly;
  explicit Fixture(const g::Descriptor& descriptor) : assembly(runtime::assemble(descriptor)) {
    image.block(ubc::lhiver::game_parser, 0x58);
    image.block(table, static_cast<std::size_t>(ubc::lhiver::symbol_count + 2) * 4);
    image.put(ubc::lhiver::game_parser + 0x18, table);
    image.put(ubc::lhiver::game_parser + 0x1c, ubc::lhiver::symbol_count + 2);
    image.put(ubc::lhiver::game_parser + 0x20, ubc::lhiver::symbol_count);
    image.put(ubc::lhiver::game_parser + 0x48, code);
    image.put(ubc::lhiver::game_parser + 0x50, code + ubc::lhiver::code_length);
    image.put(ubc::lhiver::game_parser + 0x54, ubc::lhiver::code_length + 128);
    populate(assembly.profile());
    for (const auto id : {descriptor.input, descriptor.output, g::lifecycle_item})
      populate(runtime::item_profile(*facts::item_for(static_cast<std::uint32_t>(id))));
    image.put(symbol(1683) + 0x1c, 0x2000);
    image.put(symbol(1679) + 0x1c, 0x2000);
    image.put(symbol(12330) + 0x18, 11);
    image.put(symbol(10502) + 0x18, 1);
    image.put(symbol(10501) + 0x18, 1);
    image.put(symbol(10505) + 0x18, 98);
  }
  void make(std::uint32_t id, const char* name, std::uint32_t elements, std::uint32_t type,
            std::uint32_t flags, std::uint32_t data, std::uint32_t offset, std::int32_t parent) {
    image.block(symbol(id), 0x3c); image.block(name_at(id), std::strlen(name) + 1);
    image.put(table + id * 4, symbol(id)); image.put(symbol(id) + 8, name_at(id));
    image.put(symbol(id) + 12, static_cast<std::uint32_t>(std::strlen(name)));
    image.put(symbol(id) + 16, static_cast<std::uint32_t>(std::strlen(name) + 1));
    std::memcpy(image.at(name_at(id), std::strlen(name) + 1), name, std::strlen(name) + 1);
    image.put(symbol(id) + 0x18, data); image.put(symbol(id) + 0x1c, offset);
    image.put(symbol(id) + 0x20, station_test::metadata(elements, type, flags));
    image.put(symbol(id) + 0x38, parent < 0 ? 0 : symbol(static_cast<std::uint32_t>(parent)));
    if (parent >= 0 && !image.at(symbol(static_cast<std::uint32_t>(parent)), 0x3c)) {
      image.block(symbol(static_cast<std::uint32_t>(parent)), 0x3c);
      image.put(table + static_cast<std::uint32_t>(parent) * 4, symbol(static_cast<std::uint32_t>(parent)));
    }
  }
  void populate(const ubc::lhiver::Profile& profile) {
    for (std::size_t i = 0; i < profile.binding_count; ++i) {
      const auto& b = profile.bindings[i];
      make(b.symbol, b.name, b.elements, b.type, b.flags,
           b.check_value ? static_cast<std::uint32_t>(b.value) : 0,
           static_cast<std::uint32_t>(b.offset), b.parent_index);
    }
    for (std::size_t i = 0; i < profile.function_count; ++i) {
      const auto& f = profile.functions[i];
      make(f.symbol, f.name, f.argc, f.type, f.flags, f.byte_offset,
           f.type == 7 ? 0x61000000 : f.return_type, f.parent_index);
      image.block(code + f.byte_offset, f.byte_length);
      auto* bytes = image.at(code + f.byte_offset, f.byte_length);
      for (std::uint32_t at = 0; at < f.byte_length; ++at)
        bytes[at] = static_cast<std::uint8_t>((f.symbol + at * 37u) % 256u);
    }
  }
  rs::RuntimeContract synthetic_verify() {
    auto profile = assembly.profile();
    std::vector<ubc::lhiver::Function> copied(profile.functions, profile.functions + profile.function_count);
    for (auto& f : copied) {
      ubc::Sha256 hash; hash.update(image.at(code + f.byte_offset, f.byte_length), f.byte_length);
      f.body_sha256 = hash.finish();
    }
    profile.functions = copied.data();
    return rs::verify_local_contract(image.memory(), ubc::lhiver::game_parser, profile);
  }
};
} // namespace grind_test
