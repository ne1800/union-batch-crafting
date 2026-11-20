// SPDX-License-Identifier: MIT
#include "ubc/stations/bow/bow_item_native.hpp"
#include "bow_item_test_values.hpp"

namespace {
namespace n = ubc::bow_item_native;
namespace path = ubc::bow_item_path;
namespace cat = ubc::bow_recipe_catalog;
using namespace bow_item_test;
constexpr std::uint32_t hero = 0x2000, world = 0x8000, sorted = 0x60000000;
path::Snapshot snapshot(const cat::Descriptor& d = cat::recipes.front()) {
  path::Snapshot s;
  s.captured = true;
  s.boundary = {{1, 7, world, hero, 0x8800, 0x9000, 16}, 8, 7, true};
  s.plan = path::make_plan(d, 2);
  s.original_sorter_bound = true;
  s.requirements.new_output_node = true;
  s.listed_nodes = 3;
  for (std::size_t i = 0; i < d.targets.count; ++i)
    s.materials[i] = {d.targets[i].symbol, 100, 0, 100, 0, 0};
  for (std::size_t i = 0; i < 3; ++i)
    s.removals[i] = {s.plan.removals[i],
                     path::Source::listed,
                     path::Removal::whole,
                     0x100000u + static_cast<std::uint32_t>(i) * 0x400u,
                     1,
                     4u - static_cast<std::uint32_t>(i),
                     3u - static_cast<std::uint32_t>(i),
                     false};
  return s;
}
void pool(Image& image, unsigned count = 20) {
  image.block(n::node_pool, 20);
  image.put(n::node_pool, 12);
  image.put(n::node_pool + 4, 4096);
  image.put(n::node_pool + 8, 0x40000);
  image.put(n::node_pool + 12, count ? 0x50000 : 0);
  image.put(n::node_pool + 16, count);
  put(image, 0x8a2558, 0);
  put(image, 0x8d8780, 0);
  for (unsigned i = 0; i < count; ++i) {
    const auto node = 0x50000 + i * 12;
    image.block(node, 12);
    image.put(node, i + 1 < count ? node + 12 : 0);
    image.put(node + 4, i ? node - 12 : n::node_pool + 12);
  }
}
void world_lists(Image& image) {
  image.block(world, 0x6300);
  put(image, world + 0x6280, 0x70000);
  put(image, world + 0x6288, 0x71000);
  image.block(0x70000, 12);
  image.block(0x71000, 12);
  image.put(0x70000, 0x77ef50);
  image.put(0x71000, 0x77ef90);
  const auto s = snapshot();
  for (const auto& step : s.removals) {
    image.block(step.object, 0x348);
    image.put(step.object + 4, 1);
    image.put(step.object + 0x330, static_cast<std::uint32_t>(step.operation.symbol));
  }
  if (!image.at(hero + 0x718, 4)) {
    image.block(hero, 0x1000);
    image.put(hero + 0x718, 0x11000);
    for (unsigned i = 0; i < 3; ++i) {
      image.block(0x11000 + i * 16, 12);
      image.put(0x11008 + i * 16, i < 2 ? 0x11000 + (i + 1) * 16 : 0);
    }
  }
}
struct Fixture : bow_test::Fixture {
  explicit Fixture(const cat::Descriptor& d = cat::recipes.front()) : bow_test::Fixture(d) {
    image.block(hero, 0x1000);
    image.put(hero + 0x710, path::sorter);
    image.put(hero + 0x718, 0x11000);
    image.block(0x8b6be0, 36);
    for (unsigned i = 0; i < 9; ++i)
      image.put(0x8b6be0 + i * 4, i);
    image.block(sorted, static_cast<std::size_t>(ubc::lhiver::symbol_count) * 4);
    image.put(ubc::lhiver::game_parser + 0x24, sorted);
    make_symbol(0, "ZZZ", 0, 7, 1, 0, 0, -1);
    std::vector<ubc::PackedInventoryTarget> targets;
    for (const auto& target : d.targets)
      targets.push_back(target);
    std::sort(targets.begin(), targets.end(),
              [](const auto& a, const auto& b) { return std::strcmp(a.name, b.name) < 0; });
    for (std::size_t i = 0; i < targets.size(); ++i)
      image.put(sorted + static_cast<std::uint32_t>(i) * 4,
                static_cast<std::uint32_t>(targets[i].symbol));
    for (std::size_t i = 0; i < 3; ++i) {
      const auto node = 0x11000u + static_cast<std::uint32_t>(i) * 16u,
                 obj = 0x100000u + static_cast<std::uint32_t>(i) * 0x400u;
      image.block(node, 12);
      image.put(node + 4, obj);
      image.put(node + 8, i < 2 ? node + 16 : 0);
      image.block(obj, 0x348);
      image.put(obj, ubc::hand_detail::item_vtable);
      image.put(obj + 4, 1);
      image.put(obj + 0x330, static_cast<std::uint32_t>(d.targets[i].symbol));
    }
    pool(image);
    world_lists(image);
    // world_lists constructs the same three current objects; restore sort fields.
    for (unsigned i = 0; i < 3; ++i) {
      image.put(0x100000 + i * 0x400, ubc::hand_detail::item_vtable);
      image.put(0x100330 + i * 0x400, static_cast<std::uint32_t>(d.targets[i].symbol));
    }
  }
  void constructors(const Dat& dat, const cat::Descriptor& d) {
    for (const auto& b : n::constructor_fields)
      make_symbol(b.symbol, b.name, b.elements, b.type, b.flags, 0,
                  static_cast<std::uint32_t>(b.offset), b.parent_index);
    for (const auto& item : d.items) {
      const auto& f = *item.constructor;
      image.block(station_test::code + f.byte_offset, f.byte_length);
      std::memcpy(image.at(station_test::code + f.byte_offset, f.byte_length),
                  dat.code.data() + f.byte_offset, f.byte_length);
      for (const auto& v : n::visuals)
        if (v.item == static_cast<std::uint32_t>(item.symbol)) {
          const auto data = 0x61000000 + v.symbol * 0x100;
          make_symbol(v.symbol, "LITERAL", 1, 3, 1, data, 0, -1);
          image.block(data, 20);
          image.put(data + 8, data + 32);
          image.put(data + 12, v.length);
          text(image, data + 32, dat.strings.at(v.symbol));
        }
    }
    put(image, 0x8b7464, 1584);
    put(image, 0xab11e4, 1);
    text(image, 0x8922a4, ".ZEN");
  }
};
void data_cases() {
  for (unsigned count : {0u, 1u, 6u, 7u, 10u, 20u}) {
    Image m;
    pool(m, count);
    const auto r = n::read_pool(m.memory(), snapshot());
    check(r.stable && r.required == 7 && r.growth_required == (count < 7) &&
              r.reuse_path == (count >= 7),
          "pool reuse versus real growth need is explicit");
  }
  {
    auto s = snapshot();
    for (auto& step : s.removals)
      step.source = path::Source::packed;
    Image m;
    pool(m);
    const auto r = n::read_pool(m.memory(), s);
    check(r.stable && r.required == 10 && r.reuse_path,
          "three packed ingredients have finite ten-node allocation upper bound independent of N");
  }
  for (unsigned which = 0; which < 5; ++which) {
    Image m;
    pool(m);
    if (which == 0)
      m.put(n::node_pool, 16);
    if (which == 1)
      m.put(n::node_pool + 4, 0);
    if (which == 2)
      m.put(0x50000 + 4, 0);
    if (which == 3)
      m.put(0x50000, 0x50000);
    if (which == 4)
      m.put(0x8a2558, 1);
    check(!n::read_pool(m.memory(), snapshot()).stable,
          "bad native pool shape, links, cycle or debug path rejects");
  }
  {
    Image m;
    pool(m);
    m.change_address = n::node_pool;
    m.change_target = n::node_pool + 16;
    m.change_on_read = 2;
    check(!n::read_pool(m.memory(), snapshot()).stable, "pool change across passes rejects");
  }
  {
    Image m;
    pool(m);
    world_lists(m);
    const auto s = snapshot();
    const auto p = n::read_pool(m.memory(), s);
    const auto r = n::read_world_lists(m.memory(), s, p);
    check(r.stable && r.current_whole_refcount_safe[0] && r.current_whole_refcount_safe[2],
          "real Ref1 gains two world list references before original release");
    m.block(0x72000, 12);
    m.block(0x72100, 12);
    m.put(0x70008, 0x72000);
    m.put(0x71008, 0x72100);
    m.put(0x72004, s.removals[0].object);
    m.put(0x72104, s.removals[0].object);
    const auto both = n::read_world_lists(m.memory(), s, p);
    check(both.stable && !both.current_whole_refcount_safe[0],
          "Ref1 already present in both world lists cannot invent extra references");
    m.put(0x71008, 0);
    check(n::read_world_lists(m.memory(), s, p).current_whole_refcount_safe[0],
          "one real new reference suffices for strict greater-than-one check");
    m.put(0x70008, 0x50000);
    check(!n::read_world_lists(m.memory(), s, p).stable,
          "world node cannot alias currently free pool node");
  }
  for (const auto& d : cat::recipes) {
    Fixture f(d);
    auto s = snapshot(d);
    check(n::read_sort_inputs(f.image.memory(), d, s).stable,
          "all twenty canonical target searches and comparator inputs resolve");
  }
  for (unsigned which = 0; which < 5; ++which) {
    Fixture f;
    auto s = snapshot();
    if (which == 0)
      f.image.put(sorted, 0);
    if (which == 1)
      f.image.put(0x8b6be0 + 4, 0);
    if (which == 2)
      f.image.put(hero + 0x710, path::sorter + 1);
    if (which == 3)
      f.image.put(0x100330, ubc::lhiver::symbol_count);
    if (which == 4)
      f.image.put(station_test::symbol(16610) + 12, 128);
    check(!n::read_sort_inputs(f.image.memory(), cat::recipes.front(), s).stable,
          "wrong index, duplicate category, foreign comparator or missing name rejects");
  }
  {
    Fixture f;
    f.image.change_address = 0x8b6be0;
    f.image.change_target = 0x8b6be0 + 4;
    f.image.change_on_read = 2;
    check(!n::read_sort_inputs(f.image.memory(), cat::recipes.front(), snapshot()).stable,
          "sort order changes cannot survive comparison pass");
  }
  {
    Fixture f;
    check(!n::verify_constructor_values(f.image.memory(), cat::recipes.front()).values_verified,
          "synthetic bodies cannot become constructor evidence");
  }
}
void pe_header(Image& m, const n::dep::Module& module, std::uint32_t stamp,
               std::uint32_t preferred) {
  m.block(module.base, 0x1000);
  auto* b = m.at(module.base, 0x1000);
  b[0] = 'M';
  b[1] = 'Z';
  m.put(module.base + 60, 0x80);
  m.put(module.base + 0x80, 0x4550);
  m.put(module.base + 0x84, 0x14c);
  m.put(module.base + 0x88, stamp);
  m.put(module.base + 0x94, 224);
  m.put(module.base + 0x98, 0x10b);
  m.put(module.base + 0xb4, preferred);
  m.put(module.base + 0xd0, module.image_size);
}
struct ModuleFixture {
  Image image;
  n::dep::Modules modules;
  ModuleFixture(const PE& exe, const PE& zbug, const PE& core) {
    modules = {{0x68000000, ubc::zbug_end_compat::zbug_image_size,
                ubc::zbug_end_compat::zbug_file_sha256, true},
               {0x69000000, ubc::zbug_end_compat::union_image_size,
                ubc::zbug_end_compat::union_file_sha256, true},
               {0x6a000000, n::dep::parser_extender_size, n::dep::parser_extender_sha256, true},
               {0x6b000000, 0xbe000, n::dep::crt_sha256, true}};
    zbug.load(image, modules.zbug.base);
    core.load(image, modules.union_core.base);
    // These two metadata fixtures test only the API dependency
    // boundary. They do not establish positive ZPE or CRT code identity.
    pe_header(image, modules.parser_extender, 0x62b4dbf2, 0x10000000);
    pe_header(image, modules.runtime_crt, 0x4ba1dbbe, 0x78aa0000);
    const auto z = modules.zbug.base, u = modules.union_core.base;
    for (const auto& area : ubc::bow_item_code::ranges)
      exe.copy(image, area.address, area.length);
    exe.copy(image, 0x83c804, 0x80);
    exe.copy(image, 0x83c714, 0xc4);
    patch_call(image, 0x6e73b0, z + 0x12120);
    patch_call(image, 0x70ca01, z + 0x11e30);
    patch_call(image, 0x70cb66, z + 0x11e30);
    for (const auto call : {0x730857u, 0x73087au, 0x7308d0u})
      patch_call(image, call, z + 0x11c80);
    image.put(z + 0x45130, u + 0x3e96c8);
    image.put(z + 0x45164, u + 0x2d670);
    image.put(z + 0x59840, ubc::lhiver::game_parser);
    image.put(u + 0x3e96c8 + 8, 4);
    // The packed Union file stub differs from the loaded export. Only
    // this separately pinned four-byte getter is synthetic runtime code.
    const std::array<std::uint8_t, 4> version{{0x8b, 0x41, 0x08, 0xc3}};
    std::memcpy(image.at(u + 0x2d670, 4), version.data(), 4);
    image.put(z + 0x59014, 0xa17439b5);
    exe.copy(image, 0x721e53, 5);
    patch_call(image, 0x721e53, z + 0x11c80);
    image.put(z + 0x4515c, u + 0x31aa0);
    image.put(z + 0x451a0, u + 0x12c70);
    image.put(z + 0x5df70, 0x62000000);
    image.put(z + 0x5df78, 2);
    image.put(z + 0x5df7c, 0x749350);
    image.put(z + 0x5df80, z + 0x11c80);
    image.put(z + 0x5df84, 2);
    image.block(0x62000000, 24);
    image.put(0x62000000, u + 0x81df8);
    image.put(0x62000000 + 20, 0x749350);
  }
  void growth(const PE& exe, const PE& crt) {
    Fixture symbols;
    symbols.make_symbol(1682, "ITEM", 0, 7, 1, 0, 0x100000, 1584);
    const auto& cls = ubc::alchemy_flask_runtime::flask_bindings[5];
    symbols.make_symbol(cls.symbol, cls.name, cls.elements, cls.type, cls.flags, 0, 0,
                        cls.parent_index);
    for (const auto& b : symbols.image.blocks)
      image.blocks[b.first] = b.second;
    const auto z = modules.parser_extender.base, u = modules.union_core.base,
               c = modules.runtime_crt.base;
    image.blocks[z].resize(modules.parser_extender.image_size);
    crt.load(image, c);
    image.put(z + 0xc0b1c, ubc::lhiver::game_parser);
    image.put(z + 0xc44c0, 0x63000000);
    image.put(z + 0xa91fc, u + 0x31aa0);
    image.put(z + 0xa9230, u + 0x2d670);
    image.put(z + 0xa92e4, u + 0x3e96c8);
    image.block(0x63000000, 24);
    image.put(0x63000000, u + 0x81df8);
    image.put(0x63000000 + 20, 0x711bd0);
    put(image, ubc::alchemy_flask_runtime::current_parser_pointer, ubc::lhiver::game_parser);
    put(image, 0x82e30c, u + 0x35450);
    image.put(u + 0x812e8, c + 0x10233);
    const std::array<std::uint8_t, 6> thunk{{0x55, 0x8b, 0xec, 0x5d, 0xff, 0x25}};
    std::memcpy(image.at(u + 0x35450, 10), thunk.data(), 6);
    image.put(u + 0x35456, u + 0x812e8);
    const auto pe = u32(image.at(u + 60, 4)), exp = u32(image.at(u + pe + 120, 4));
    const auto ordinal = u32(image.at(u + exp + 16, 4)), table = u32(image.at(u + exp + 28, 4));
    if (ordinal > 258)
      std::abort();
    image.put(u + table + (258 - ordinal) * 4, 0x35450); // actual observed unpacked export RVA
    for (const auto index : {0u, 1u}) {
      const auto& r = n::engine_ranges[index];
      exe.copy(image, r.address, r.length);
    }
    for (const auto index : {11u, 12u}) {
      const auto& r = n::dep::additional_ranges[index];
      exe.copy(image, r.address, r.length);
    }
    pool(image, 0);
  }
};
void files(const char* exe_path, const char* dat_path, const char* zbug_path,
           const char* union_path, const char* crt_path) {
  const PE exe(exe_path, ubc::station_abi::executable_sha256);
  const Dat dat(dat_path);
  Image code;
  for (const auto& r : n::engine_ranges)
    exe.copy(code, r.address, r.length);
  check(n::verify_engine_parts(code.memory()).verified,
        "all fifteen extra complete original native functions from exact EXE");
  pool(code);
  check(n::verify_pool_path(code.memory(), {}, snapshot()).verified,
        "original nonempty pool uses no guessed allocator evidence");
  pool(code, 0);
  check(!n::verify_pool_path(code.memory(), {}, snapshot()).verified,
        "growth cannot borrow missing Union/CRT boundary");
  for (std::size_t i = 0; i + 2 < n::engine_ranges.size(); ++i) {
    const auto& r = n::engine_ranges[i];
    auto* b = code.at(r.address, r.length);
    b[r.length - 1] ^= 1;
    check(!n::verify_engine_parts(code.memory()).verified,
          "last byte of every extra function is guarded");
    b[r.length - 1] ^= 1;
  }
  for (const auto& d : cat::recipes) {
    Fixture f(d);
    f.constructors(dat, d);
    const auto ok = n::verify_constructor_values(f.image.memory(), d);
    check(ok.values_verified,
          "all twenty real constructor combinations and exact visual bytes are bound");
    for (const auto& b : n::constructor_fields) {
      const auto at = station_test::symbol(b.symbol) + 0x1c;
      f.image.put(at, static_cast<std::uint32_t>(b.offset) + 4);
      check(!n::verify_constructor_values(f.image.memory(), d).values_verified,
            "each actual constructor field offset remains fixed");
      f.image.put(at, static_cast<std::uint32_t>(b.offset));
    }
    const auto* v = &n::visuals[0];
    for (const auto& candidate : n::visuals)
      if (candidate.item == static_cast<std::uint32_t>(d.items[0].symbol))
        v = &candidate;
    const auto at = 0x61000000 + v->symbol * 0x100 + 32;
    f.image.at(at, 1)[0] ^= 1;
    check(!n::verify_constructor_values(f.image.memory(), d).values_verified,
          "changed visual cannot bypass .ZEN exclusion");
  }
  const PE zbug(zbug_path, ubc::zbug_end_compat::zbug_file_sha256),
      core(union_path, ubc::zbug_end_compat::union_file_sha256);
  {
    ModuleFixture f(exe, zbug, core);
    check(
        n::verify_zbug_hasitems(f.image.memory(), f.modules).verified,
        "real ZBug code, relocated exact imports and engine version compose narrow HasItems path");
    check(n::verify_routed_parts(f.image.memory(), f.modules, snapshot()).verified,
          "exact known CALL set and positive direct PutInInv chain");
  }
  for (const auto& r : n::zbug_ranges) {
    ModuleFixture f(exe, zbug, core);
    *f.image.at(f.modules.zbug.base + r.rva + r.length - 1, 1) ^= 1;
    check(!n::verify_zbug_hasitems(f.image.memory(), f.modules).verified,
          "all reachable ZBug thunks including cookie and release have complete guards");
  }
  for (unsigned which = 0; which < 8; ++which) {
    ModuleFixture f(exe, zbug, core);
    const auto z = f.modules.zbug.base;
    if (which == 0)
      f.image.put(z + 0x45164, f.modules.union_core.base + 0x2d671);
    if (which == 1)
      f.image.put(z + 0x59840, 0);
    if (which == 2)
      f.image.put(f.modules.union_core.base + 0x3e96c8 + 8, 3);
    if (which == 3)
      *f.image.at(0x6e73af, 1) ^= 1;
    if (which == 4)
      patch_call(f.image, 0x70ca01, z + 0x11e31);
    if (which == 5)
      patch_call(f.image, 0x70cb66, z + 0x11e31);
    if (which == 6)
      patch_call(f.image, 0x73087a, z + 0x11c81);
    if (which == 7)
      f.image.put(0x62000000 + 12, 1);
    check(!n::verify_routed_parts(f.image.memory(), f.modules, snapshot()).verified,
          "wrong import/version/caller/fallback or non-direct dispatch rejects");
  }
  {
    ModuleFixture f(exe, zbug, core);
    auto s = snapshot();
    s.materials[0].packed = 1;
    check(!n::verify_routed_parts(f.image.memory(), f.modules, s).verified,
          "mixed input cannot borrow unreachable-pack lemma");
    s = snapshot();
    s.plan.minima[0].amount = 1;
    check(!n::verify_routed_parts(f.image.memory(), f.modules, s).verified,
          "copied or changed plan does not match canonical ordering");
  }
  const PE crt(crt_path, n::dep::crt_sha256);
  {
    ModuleFixture f(exe, zbug, core);
    f.growth(exe, crt);
    const auto p = n::verify_pool_path(f.image.memory(), f.modules, snapshot());
    check(p.verified && p.one_growth && p.required == 7,
          "actual original pool code and exact Union/CRT export chain allow one growth path");
    f.image.put(f.modules.union_core.base + 0x812e8, f.modules.runtime_crt.base + 0x10234);
    check(!n::verify_pool_path(f.image.memory(), f.modules, snapshot()).verified,
          "changed allocator destination cannot authorize growth");
  }
}
}
#ifndef UBC_BOW_ITEM_NATIVE_FIXTURE_MAIN
#define UBC_BOW_ITEM_NATIVE_FIXTURE_MAIN main
#endif
int UBC_BOW_ITEM_NATIVE_FIXTURE_MAIN(int argc, char** argv) {
  data_cases();
  if (argc == 7 && std::string(argv[1]) == "--local-files")
    files(argv[2], argv[3], argv[4], argv[5], argv[6]);
  else if (argc != 1)
    return 2;
  std::printf("%u checks, %u failures\n", station_test::checks, station_test::failures);
  return station_test::failures ? 1 : 0;
}
