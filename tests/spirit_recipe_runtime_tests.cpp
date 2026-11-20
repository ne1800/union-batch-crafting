// SPDX-License-Identifier: MIT
#include "ubc/stations/spirit/spirit_recipe_runtime.hpp"
#define UBC_STATION_RECIPE_FIXTURE_ONLY
#include "station_recipe_runtime_tests.cpp"

namespace spirit_test {
namespace cat = ubc::spirit_recipe_catalog;
namespace rt = ubc::spirit_recipe_runtime;
namespace rs = ubc::runtime_script;
using station_test::array_at;
using station_test::check;
using station_test::code;
using station_test::Image;
using station_test::name_at;
using station_test::symbol;
using station_test::table;
struct Fixture {
  rt::ProfileAssembly assembly;
  Image image;
  std::vector<ubc::lhiver::Function> synthetic;
  explicit Fixture(const cat::Descriptor& recipe) : assembly(rt::assemble(recipe)) {
    image.block(ubc::lhiver::game_parser, 0x58);
    image.block(table, static_cast<std::size_t>(ubc::lhiver::symbol_count + 2) * 4);
    image.put(ubc::lhiver::game_parser + 0x18, table);
    image.put(ubc::lhiver::game_parser + 0x1c, ubc::lhiver::symbol_count + 2);
    image.put(ubc::lhiver::game_parser + 0x20, ubc::lhiver::symbol_count);
    image.put(ubc::lhiver::game_parser + 0x48, code);
    image.put(ubc::lhiver::game_parser + 0x50, code + ubc::lhiver::code_length);
    image.put(ubc::lhiver::game_parser + 0x54, ubc::lhiver::code_length + 128);
    populate(recipe);
  }
  void make_symbol(std::uint32_t id, const char* name, std::uint32_t count, std::uint32_t type,
                   std::uint32_t flags, std::uint32_t data, std::uint32_t offset,
                   std::int32_t parent) {
    image.block(symbol(id), 0x3c);
    image.block(name_at(id), std::strlen(name) + 1);
    image.put(table + id * 4, symbol(id));
    image.put(symbol(id) + 8, name_at(id));
    image.put(symbol(id) + 12, static_cast<std::uint32_t>(std::strlen(name)));
    image.put(symbol(id) + 16, static_cast<std::uint32_t>(std::strlen(name) + 1));
    image.put(symbol(id) + 0x18, data);
    image.put(symbol(id) + 0x1c, offset);
    image.put(symbol(id) + 0x20, station_test::metadata(count, type, flags));
    image.put(symbol(id) + 0x38, parent < 0 ? 0 : symbol(static_cast<std::uint32_t>(parent)));
    std::memcpy(image.at(name_at(id), std::strlen(name) + 1), name, std::strlen(name) + 1);
    if (parent >= 0 && !image.at(symbol(static_cast<std::uint32_t>(parent)), 0x3c)) {
      image.block(symbol(static_cast<std::uint32_t>(parent)), 0x3c);
      image.put(table + static_cast<std::uint32_t>(parent) * 4,
                symbol(static_cast<std::uint32_t>(parent)));
    }
  }
  void populate(const cat::Descriptor& recipe) {
    const auto own = rt::assemble(recipe);
    const auto p = own.profile();
    for (std::size_t i = 0; i < p.binding_count; ++i) {
      const auto& b = p.bindings[i];
      make_symbol(b.symbol, b.name, b.elements, b.type, b.flags,
                  b.check_value ? static_cast<std::uint32_t>(b.value) : 0,
                  static_cast<std::uint32_t>(b.offset), b.parent_index);
      if (b.type == 2 && b.elements > 1 && b.parent_index == -1) {
        image.block(array_at(b.symbol), static_cast<std::size_t>(b.elements) * 4);
        image.put(symbol(b.symbol) + 0x18, array_at(b.symbol));
        for (std::uint32_t j = 0; j < b.elements; ++j)
          image.put(array_at(b.symbol) + j * 4, 1);
      }
    }
    for (std::size_t i = 0; i < p.function_count; ++i) {
      const auto& f = p.functions[i];
      make_symbol(f.symbol, f.name, f.argc, f.type, f.flags, f.byte_offset,
                  f.type == 7 ? 0x60000000 + f.symbol * 0x100 : f.return_type, f.parent_index);
      image.block(code + f.byte_offset, f.byte_length);
      auto* bytes = image.at(code + f.byte_offset, f.byte_length);
      for (std::uint32_t j = 0; j < f.byte_length; ++j)
        bytes[j] = static_cast<std::uint8_t>((f.symbol + j * 37u) % 256u);
    }
    image.put(symbol(1683) + 0x1c, 0x2000);
    image.put(symbol(1679) + 0x1c, 0x2000);
    image.put(symbol(12330) + 0x18, 4);
    image.put(symbol(20361) + 0x18, 1);
    image.put(symbol(11198) + 0x18, 1);
    image.put(symbol(11967) + 0x18, 0);
  }
  void learned(const cat::Descriptor& recipe, std::int32_t value) {
    for (const auto& r : recipe.requirements)
      if (r.role == cat::RequirementRole::knowledge)
        image.put(symbol(static_cast<std::uint32_t>(r.symbol)) + 0x18,
                  static_cast<std::uint32_t>(value));
  }
  rs::RuntimeContract synthetic_verify() {
    auto p = assembly.profile();
    synthetic.clear();
    for (std::size_t i = 0; i < p.function_count; ++i) {
      auto f = p.functions[i];
      ubc::Sha256 hash;
      hash.update(image.at(code + f.byte_offset, f.byte_length), f.byte_length);
      f.body_sha256 = hash.finish();
      synthetic.push_back(f);
    }
    p.functions = synthetic.data();
    return rs::verify_local_contract(image.memory(), ubc::lhiver::game_parser, p);
  }
};
}

#ifndef UBC_SPIRIT_RUNTIME_FIXTURE_ONLY
int main() {
  using namespace spirit_test;
  const auto& recipe = cat::recipes[0];
  auto assembly = rt::assemble(recipe);
  check(assembly.valid() && assembly.function_count() <= 64 && assembly.binding_count() <= 64,
        "bounded spirit closure assembles");
  const auto profile = assembly.profile();
  for (const auto* required :
       {recipe.owner.constructor, recipe.owner.condition, recipe.callback, recipe.station_entry,
        recipe.station_end, recipe.say, recipe.end_production}) {
    bool found = false;
    for (std::size_t i = 0; i < profile.function_count; ++i)
      found |= profile.functions[i].symbol == required->symbol;
    check(found, "owner entry and original lifecycle bodies are included");
  }
  for (const auto* required : recipe.item_constructors) {
    bool found = false;
    for (std::size_t i = 0; i < profile.function_count; ++i)
      found |= profile.functions[i].symbol == required->symbol;
    check(found, "both possible output constructors and all input constructors are bound");
  }
  auto copy = assembly;
  assembly = {};
  check(copy.valid() && copy.profile().functions != profile.functions, "copy owns its tables");
  Fixture f(recipe);
  check(f.synthetic_verify().matches, "synthetic profile accepts only own positive bodies");
  check(!rt::verify(f.image.memory(), ubc::lhiver::game_parser, f.assembly).matches,
        "synthetic bytes never pass original canonical hashes");
  for (const auto v : {INT32_MIN, -1, 0, 1, 2, 3, INT32_MAX}) {
    f.image.put(symbol(11967) + 0x18, static_cast<std::uint32_t>(v));
    const auto b = rt::branch(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
    check(b.read_valid && b.value == v &&
              b.branch == (v == 1 ? cat::Branch::enriched : cat::Branch::ordinary),
          "only exactly one selects the enriched original branch");
    check(rt::knowledge(f.image.memory(), ubc::lhiver::game_parser, f.assembly).known,
          "branch value is not a learning predicate");
  }
  auto before = rt::branch(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
  f.image.put(symbol(11967) + 0x18, 2);
  auto after = rt::branch(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
  check(!rt::same_branch_evidence(before, after),
        "same output branch still binds exact observed integer");
  for (const auto v : {0, 2, -1}) {
    f.learned(recipe, v);
    const auto k = rt::knowledge(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
    check(k.read_valid && !k.known, "readable non-one learning does not grant recipe");
  }
  f.learned(recipe, 1);
  f.image.put(symbol(20361) + 0x18, 0);
  auto c = rt::context(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
  check(c.read_valid && c.station_matches() && !c.owner_matches(),
        "root page does not block a planned route to special");
  check(rt::knowledge(f.image.memory(), ubc::lhiver::game_parser, f.assembly).known,
        "category is not knowledge");
  f.image.put(symbol(12330) + 0x18, 0);
  c = rt::context(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
  check(c.read_valid && !c.station_matches() &&
            rt::knowledge(f.image.memory(), ubc::lhiver::game_parser, f.assembly).known,
        "production mismatch remains separate from learned state");
  for (const auto id : {11198u, 11967u, 12330u, 20361u})
    for (unsigned change = 0; change < 7; ++change) {
      Fixture x(recipe);
      switch (change) {
      case 0:
        x.image.put(symbol(id) + 0x20, station_test::metadata(2, 2, 0));
        break;
      case 1:
        x.image.put(symbol(id) + 0x20, station_test::metadata(1, 2, 1));
        break;
      case 2:
        x.image.put(symbol(id) + 0x38, symbol(1537));
        break;
      case 3:
        x.image.at(name_at(id), 1)[0] = 'X';
        break;
      case 4:
        x.image.denied = symbol(id) + 0x18;
        x.image.denied_size = 4;
        break;
      case 5:
        x.image.put(symbol(id) + 0x18, 1);
        x.image.change_address = symbol(id);
        x.image.change_target = symbol(id) + 0x18;
        x.image.change_on_read = 2;
        break;
      case 6:
        x.image.change_layout = true;
        break;
      }
      const bool valid =
          id == 11198
              ? rt::knowledge(x.image.memory(), ubc::lhiver::game_parser, x.assembly).read_valid
          : id == 11967
              ? rt::branch(x.image.memory(), ubc::lhiver::game_parser, x.assembly).read_valid
              : rt::context(x.image.memory(), ubc::lhiver::game_parser, x.assembly).read_valid;
      check(!valid, "wrong unstable or unreadable scalar never becomes known");
      if (id == 11967) {
        const auto k = rt::knowledge(x.image.memory(), ubc::lhiver::game_parser, x.assembly);
        if (change != 6)
          check(k.known, "unknown branch does not erase independently readable knowledge");
      }
    }
  for (unsigned change = 0; change < 6; ++change) {
    Fixture x(recipe);
    switch (change) {
    case 0:
      x.image.put(symbol(1679) + 0x1c, 0x3000);
      break;
    case 1:
      x.image.put(symbol(1683) + 0x1c, 0x3000);
      break;
    case 2:
      x.image.put(symbol(1679) + 0x20, station_test::metadata(1, 7, 1));
      break;
    case 3:
      x.image.denied = symbol(1679);
      x.image.denied_size = 4;
      break;
    case 4:
      x.image.change_address = symbol(1679);
      x.image.change_target = symbol(1679) + 0x1c;
      x.image.change_on_read = 2;
      break;
    case 5:
      x.image.change_layout = true;
      break;
    }
    const auto a = rt::actor(x.image.memory(), ubc::lhiver::game_parser, x.assembly, 0x2000);
    check(!a.matches, "failure refund SELF requires current stable SELF HERO identity too");
  }
  Fixture actor_ok(recipe);
  actor_ok.image.put(symbol(1679) + 0x18, 0xabcdef00);
  const auto a =
      rt::actor(actor_ok.image.memory(), ubc::lhiver::game_parser, actor_ok.assembly, 0x2000);
  check(a.read_valid && a.matches && a.requires_self, "instance value is offset1c not data18");
  for (unsigned change = 0; change < 6; ++change) {
    Fixture x(recipe);
    switch (change) {
    case 0:
      x.image.put(symbol(20370) + 0x18, recipe.callback->byte_offset + 1);
      break;
    case 1:
      x.image.put(symbol(20370) + 0x20, station_test::metadata(1, 5, 1));
      break;
    case 2:
      x.image.put(symbol(20370) + 0x1c, 2);
      break;
    case 3:
      x.image.put(symbol(15741) + 0x20, station_test::metadata(0, 7, 3));
      break;
    case 4:
      x.image.put(symbol(15741) + 0x38, symbol(1537));
      break;
    case 5:
      x.image.at(name_at(15739), 1)[0] = 'X';
      break;
    }
    check(!x.synthetic_verify().matches, "both branches require exact item and function metadata");
  }
  auto corrupt = recipe;
  corrupt.branch_equal = 2;
  check(!rt::assemble(corrupt).valid(), "altered branch cannot assemble canonical profile");
  Fixture wrong(recipe);
  check(!rt::branch(wrong.image.memory(), 1, wrong.assembly).read_valid &&
            !rt::knowledge(wrong.image.memory(), 1, wrong.assembly).read_valid &&
            !rt::actor(wrong.image.memory(), 1, wrong.assembly, 0x2000).read_valid,
        "all dynamic domains require the canonical parser");
  std::printf("spirit_recipe_runtime_tests: %u checks, %u failures\n", station_test::checks,
              station_test::failures);
  return station_test::failures ? 1 : 0;
}
#endif
