// SPDX-License-Identifier: MIT
#include "ubc/stations/anvil/anvil_recipe_runtime.hpp"
#define UBC_STATION_RECIPE_FIXTURE_ONLY
#include "../station_recipe_runtime_tests.cpp"

namespace anvil_test {
namespace cat = ubc::anvil_recipe_catalog;
namespace rt = ubc::anvil_recipe_runtime;
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
    image.put(symbol(12330) + 0x18, 1);
    image.put(symbol(10536) + 0x18, 1);
    image.put(symbol(10552) + 0x18, 0);
    image.put(symbol(10551) + 0x18, 0);
  }
  void learned(const cat::Descriptor& recipe, std::int32_t value) {
    for (const auto& r : recipe.requirements)
      if (r.role == cat::RequirementRole::knowledge)
        image.put(array_at(static_cast<std::uint32_t>(r.symbol)) + r.element * 4,
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

#ifndef UBC_ANVIL_RUNTIME_FIXTURE_ONLY
int main() {
  using namespace anvil_test;
  for (const auto& recipe : cat::recipes) {
    auto a = rt::assemble(recipe);
    check(a.valid() && a.function_count() <= 64 && a.binding_count() <= 64,
          "bounded anvil closure assembles");
    check(a.knowledge_count() == 1, "only the exact smith learning array element is knowledge");
    auto p = a.profile();
    const auto contains = [&](std::uint32_t id) {
      for (std::size_t i = 0; i < p.function_count; ++i)
        if (p.functions[i].symbol == id)
          return true;
      return false;
    };
    for (const auto& nav : cat::navigation)
      for (const auto* f : {nav.owner.constructor, nav.owner.condition, nav.owner.information})
        check(contains(f->symbol), "all reversible category routes are included before a handoff");
    for (const auto* f : {recipe.back, recipe.callback.function, recipe.original_three.function,
                          cat::station.entry, cat::station.mob_condition})
      check(contains(f->symbol), "owner choices entry and station condition are bound");
    for (const auto* f : recipe.item_constructors)
      check(contains(f->symbol), "relevant item constructors are in local closure");
    rt::ProfileAssembly copy = a;
    const auto q = copy.profile();
    check(q.functions != p.functions && q.bindings != p.bindings, "copied profile owns tables");
    check(q.functions[0].name != p.functions[0].name &&
              std::strcmp(q.functions[0].name, p.functions[0].name) == 0,
          "copied profile owns names");
    a = {};
    check(copy.valid() && q.function_count == copy.function_count(),
          "resetting source does not invalidate copied view");
    Fixture fixture(recipe);
    check(fixture.synthetic_verify().matches,
          "synthetic memory validates against its own hashes, never against original hashes");
    check(!rt::verify(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly).matches,
          "synthetic body cannot pass canonical original hashes");
    check(rt::knowledge(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly).known,
          "learning array element one is known");
    for (const auto value : {0, 2, -1, INT32_MAX}) {
      fixture.learned(recipe, value);
      const auto k =
          rt::knowledge(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly);
      check(k.read_valid && !k.known, "readable non-one learning value is not learned");
    }
    fixture.learned(recipe, 1);
    for (const auto id : {12330u, 10536u, 10552u, 10551u})
      fixture.image.put(symbol(id) + 0x18, 0);
    check(rt::knowledge(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly).known,
          "context flags do not hide learned recipes");
    auto context = rt::context(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly);
    check(context.read_valid && !context.station_matches(),
          "readable exited production remains a separate context failure");
    fixture.image.put(symbol(12330) + 0x18, 1);
    fixture.image.put(symbol(10536) + 0x18, 1);
    context = rt::context(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly);
    check(context.read_valid && context.station_matches() && context.normal_weapons == 0 &&
              context.ore_weapons == 0,
          "root page is known without requiring target category already open");
    fixture.image.put(symbol(10552) + 0x18, 1);
    fixture.image.put(symbol(10551) + 0x18, 1);
    check(!rt::context(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly)
               .station_matches(),
          "contradictory categories cannot open a handoff");
    check(rt::knowledge(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly).known,
          "bad context still does not erase knowledge");
    auto actor =
        rt::actor(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly, 0x2000);
    check(actor.read_valid && actor.matches && actor.requires_self,
          "SELF minimum and HERO effect both bind actual hero");
    fixture.image.put(symbol(1679) + 0x1c, 0x3000);
    actor = rt::actor(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly, 0x2000);
    check(actor.read_valid && !actor.matches,
          "HERO material actor never bypasses distinct SELF minimum");
    fixture.image.put(symbol(1679) + 0x1c, 0x2000);
    fixture.image.put(symbol(1679) + 0x18, 0xabcdef00);
    check(rt::actor(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly, 0x2000)
              .matches,
          "instance address comes from offset field not code/data field");
    fixture.image.put(symbol(1496) + 0x20, station_test::metadata(29, 2, 0));
    check(!rt::knowledge(fixture.image.memory(), ubc::lhiver::game_parser, fixture.assembly)
               .read_valid,
          "wrong learned-array shape is unknown");
  }
  for (unsigned change = 0; change < 8; ++change) {
    Fixture f(cat::recipes[0]);
    switch (change) {
    case 0:
      f.image.put(symbol(1496) + 0x18, 0xfffffffc);
      break;
    case 1:
      f.image.put(symbol(1496) + 0x18, array_at(1496) + 1);
      break;
    case 2:
      f.image.put(symbol(1496) + 0x38, symbol(1537));
      break;
    case 3:
      f.image.put(symbol(1496) + 0x20, station_test::metadata(30, 2, 1));
      break;
    case 4:
      f.image.denied = array_at(1496);
      f.image.denied_size = 4;
      break;
    case 5:
      f.image.change_address = array_at(1496);
      f.image.change_on_read = 2;
      break;
    case 6:
      f.image.change_layout = true;
      break;
    case 7:
      f.image.put(symbol(1496) + 8, 0xfffffff0);
      break;
    }
    const auto k = rt::knowledge(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
    check(!k.read_valid && !k.known, "unreadable changed or misbound knowledge fails closed");
  }
  Fixture actor_change(cat::recipes[0]);
  actor_change.image.change_address = symbol(1679);
  actor_change.image.change_target = symbol(1679) + 0x1c;
  actor_change.image.change_on_read = 2;
  check(!rt::actor(actor_change.image.memory(), ubc::lhiver::game_parser, actor_change.assembly,
                   0x2000)
             .read_valid,
        "actor changes between complete passes invalidate proof");
  Fixture context_change(cat::recipes[0]);
  context_change.image.change_address = symbol(10536);
  context_change.image.change_target = symbol(10536) + 0x18;
  context_change.image.change_on_read = 2;
  check(
      !rt::context(context_change.image.memory(), ubc::lhiver::game_parser, context_change.assembly)
           .read_valid,
      "context changes between complete passes invalidate proof");
  for (unsigned change = 0; change < 6; ++change) {
    Fixture f(cat::recipes[0]);
    const auto id = cat::recipes[0].callback.function->symbol;
    switch (change) {
    case 0:
      f.image.put(symbol(id) + 0x18, cat::recipes[0].callback.function->byte_offset + 1);
      break;
    case 1:
      f.image.put(symbol(id) + 0x20, station_test::metadata(1, 5, 1));
      break;
    case 2:
      f.image.put(symbol(id) + 0x1c, 2);
      break;
    case 3:
      f.image.put(symbol(16611) + 0x20, station_test::metadata(0, 7, 3));
      break;
    case 4:
      f.image.put(symbol(16611) + 0x38, symbol(1537));
      break;
    case 5:
      f.image.at(name_at(16611), 1)[0] = 'X';
      break;
    }
    check(!f.synthetic_verify().matches,
          "actual metadata or item binding cannot match even the positively hashed synthetic code");
  }
  Fixture wrong_parser(cat::recipes[0]);
  check(!rt::knowledge(wrong_parser.image.memory(), 1, wrong_parser.assembly).read_valid &&
            !rt::actor(wrong_parser.image.memory(), 1, wrong_parser.assembly, 0x2000).read_valid &&
            !rt::context(wrong_parser.image.memory(), 1, wrong_parser.assembly).read_valid,
        "all read domains require the canonical parser");
  auto corrupt = cat::recipes[0];
  corrupt.require_self_is_hero = false;
  check(!rt::assemble(corrupt).valid(), "altered descriptor cannot weaken actor contract");
  std::printf("anvil_recipe_runtime_tests: %u checks, %u failures\n", station_test::checks,
              station_test::failures);
  return station_test::failures ? 1 : 0;
}
#endif
