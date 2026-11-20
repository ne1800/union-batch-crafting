// SPDX-License-Identifier: MIT
// Original file and DAT bodies in private RAM. Never execute the game.
#define UBC_BOW_ITEM_RUNTIME_FIXTURE_MAIN pre_bow_fixture_main
#include "../bow/bow_item_runtime_tests.cpp"
#include "ubc/stations/cool/cool_predebit_reader.hpp"
#include "ubc/stations/cool/cool_predebit_admission.hpp"
#include <memory>
namespace {
namespace pre = ubc::cool_predebit;
namespace pr = pre::reader;
constexpr std::uint32_t hot_object = 0x100000, cold_object = 0x100400, hand_object = 0x150000,
                        event_manager = 0x1a000, actual_info = 0x1b000;
struct PreFixture : RuntimeFixture {
  ubc::AmountPatch patch;
  std::uint64_t serial = 0;
  pre::Plan amount = pre::plan({true, {{{16612, 2, true, true}, {16613, 5, true, false}}}}, 2);
  PreFixture(const PE& exe, const Dat& dat, const PE& zbug, const PE& core, const PE& crt,
             const PE& zpe, const PE& renderer)
      : RuntimeFixture(exe, dat, zbug, core, crt, zpe, renderer) {
    // Synthetic plugin image: the PE header precedes RVA 0x1000.
    modules.zbug.executable_page = modules.union_core.executable_page =
        [](std::uint32_t base, std::uint32_t target) noexcept { return target - base >= 0x1000; };
    const auto assembly = ubc::cool_recipe_runtime::assemble(ubc::cool_recipe_catalog::recipe);
    const auto profile = assembly.profile();
    for (std::size_t i = 0; i < profile.binding_count; ++i)
      binding(profile.bindings[i]);
    for (const auto& b : pre::profile::bindings)
      binding(b);
    for (const auto& b : n::constructor_fields)
      binding(b);
    for (const auto& b : ubc::alchemy_flask_runtime::flask_bindings)
      if (b.symbol == 1680 || b.symbol == 1681 || b.symbol == 1682 || b.symbol == 1584 ||
          b.symbol == 1537)
        binding(b);
    for (std::size_t i = 0; i < profile.function_count; ++i) {
      const auto& f = profile.functions[i];
      symbol(f.symbol, f.name, f.argc, f.type, f.flags, f.byte_offset, f.return_type,
             f.parent_index);
      image.block(station_test::code + f.byte_offset, f.byte_length);
      std::memcpy(image.at(station_test::code + f.byte_offset, f.byte_length),
                  dat.code.data() + f.byte_offset, f.byte_length);
    }
    for (const auto& t : ubc::cool_recipe_catalog::texts)
      literal(dat, t.symbol, t.length);
    for (const auto& t : pre::profile::texts)
      literal(dat, t.symbol, t.length);
    image.put(station_test::symbol(1679) + 0x1c, hero);
    image.put(station_test::symbol(1683) + 0x1c, hero);
    image.put(station_test::symbol(12330) + 0x18, 13);
    image.put(station_test::symbol(10536) + 0x18, 1);
    image.put(station_test::symbol(1682) + 0x1c, 0);
    image.put(hero + 0x298, 1);
    image.put(hero + 0xb8, world);
    image.put(ubc::lhiver::game_parser + 0x4c, station_test::code + 1234);
    image.put(ubc::lhiver::game_parser + 0x2058, 0);
    image.put(ubc::lhiver::game_parser + 0x2198, 0);
    image.put(ubc::station_abi::instance_symbol_pointer, station_test::symbol(1679));
    image.put(ubc::station_abi::instance_address_pointer, hero);
    image.put(0xab642c, 1);
    image.put(0xab11e4, 1);
    image.put(0xab1168 + 0x48, 0);
    image.put(0x8b7464, 1584);
    *image.at(0x8cdcf0, 1) = 0;
    image.put(0x8800 + 0x1f4, 1);
    image.put(0x8800 + 0x1fc, 0);
    image.put(0x8800 + 0x1f8, 1);
    image.put(0x8800 + 0x200, 0);
    zstring(0x8800 + 0x1b8, 0x25500, "BSCOOL");
    zstring(0x8800 + 0x1e0, 0x25000, "SMITHCOOL");
    zstring(0x8800 + 0x1cc, 0x25100, "B_USECOOL");
    zstring(0x8800 + 0x1a4, 0x25200, "");
    image.block(0x25300, 8);
    image.block(0x25400, 0x48);
    image.put(0x8800 + 0x18c, 0x25300);
    image.put(0x25300, 0x25400);
    image.put(0x25444, hero);
    image.put(0x11000 + 4, hot_object);
    image.put(0x11000 + 8, 0x11010);
    image.put(0x11010 + 4, cold_object);
    image.put(0x11010 + 8, 0);
    object(hot_object, 16612, 2, 0x200001);
    object(cold_object, 16613, 5, 0x200001);
    object(hand_object, 16612, 1, 0xa00001);
    image.put(hero + 0x968, hand_object);
    image.put(0x31000 + 0x30, hand_object);
    image.put(0x31000 + 0x18, 1);
    image.block(event_manager, 0x40);
    image.put(hero + 0xfc, event_manager);
    image.put(event_manager, 0x83e23c);
    image.put(event_manager + 0x28, 1);
    image.put(event_manager + 0x3c, hero);
    image.block(actual_info, 0x5c);
    image.put(actual_info, ubc::info_detail::info_vtable);
    image.put(actual_info + 0x1c, 22243);
    image.put(actual_info + 0x20, 2);
    image.put(actual_info + 0x28, 20774);
    image.put(actual_info + 0x2c, 20775);
    image.put(actual_info + 0x48, 1);
    image.put(actual_info + 0x50, 20773);
    image.put(ubc::station_abi::information_manager + 0x20, hero);
    image.put(ubc::station_abi::information_manager + 0x24, hero);
    image.put(ubc::station_abi::information_manager + 0x54, 1);
    const auto u = modules.union_core.base, c = modules.runtime_crt.base;
    image.put(0x82e308, u + 0x35460);
    image.put(u + 0x812ec, c + 0x1014e);
    // Previously pinned unpacked Union export. This is our loader fixture. It
    // does not imply that the packed file contains these ten runtime bytes.
    const std::array<std::uint8_t, 6> thunk{{0x55, 0x8b, 0xec, 0x5d, 0xff, 0x25}};
    std::memcpy(image.at(u + 0x35460, 10), thunk.data(), 6);
    image.put(u + 0x35466, u + 0x812ec);
    const auto pe = u32(image.at(u + 60, 4)), exp = u32(image.at(u + pe + 120, 4)),
               base = u32(image.at(u + exp + 16, 4)), table = u32(image.at(u + exp + 28, 4));
    image.put(u + table + (259 - base) * 4, 0x35460);
    image.put(c + 0xb38b0, 0x777000);
    image.put(c + 0xb45f4, 0);
    image.put(c + 0x11f8, 0x77000100);
    image.put(c + 0x11fc, 0x77000200);
    check(patch.begin({image.memory(), this, &identity, &writable, &replace}, body_address(),
                      amount.body(), 2),
          "own exact77byte AmountPatch opens in fixture");
  }
  void symbol(std::uint32_t id, const char* name, std::uint32_t elements, std::uint32_t type,
              std::uint32_t flags, std::uint32_t data, std::uint32_t offset, std::int32_t parent) {
    image.block(station_test::symbol(id), 0x3c);
    image.put(station_test::symbol(id), 0x82e6f0);
    image.block(station_test::name_at(id) - 1, std::strlen(name) + 2);
    image.blocks.erase(station_test::name_at(id));
    std::memcpy(image.at(station_test::name_at(id), std::strlen(name) + 1), name,
                std::strlen(name) + 1);
    image.put(station_test::table + id * 4, station_test::symbol(id));
    image.put(station_test::symbol(id) + 8, station_test::name_at(id));
    image.put(station_test::symbol(id) + 12, static_cast<std::uint32_t>(std::strlen(name)));
    image.put(station_test::symbol(id) + 16, static_cast<std::uint32_t>(std::strlen(name) + 1));
    image.put(station_test::symbol(id) + 0x18, data);
    image.put(station_test::symbol(id) + 0x1c, offset);
    image.put(station_test::symbol(id) + 0x20, station_test::metadata(elements, type, flags));
    image.put(station_test::symbol(id) + 0x38,
              parent < 0 ? 0 : station_test::symbol(static_cast<std::uint32_t>(parent)));
  }
  void binding(const ubc::lhiver::SymbolBinding& b) {
    symbol(b.symbol, b.name, b.elements, b.type, b.flags,
           b.check_value ? static_cast<std::uint32_t>(b.value) : 0,
           b.check_offset ? static_cast<std::uint32_t>(b.offset) : 0, b.parent_index);
  }
  void literal(const Dat& dat, std::uint32_t id, std::uint32_t length) {
    const auto at = 0x65000000 + id * 0x100;
    symbol(id, id == 964 ? "FONT_SCREENSMALL" : "LITERAL", 1, 3, 1, at, 0, -1);
    image.block(at, 20);
    image.put(at, 0x82e6f0);
    image.put(at + 8, at + 32);
    image.put(at + 12, length);
    image.put(at + 16, length + 1);
    image.block(at + 31, length + 2);
    std::memcpy(image.at(at + 32, length + 1), dat.strings.at(id).c_str(), length + 1);
  }
  void zstring(std::uint32_t at, std::uint32_t data, const char* s) {
    image.put(at, 0x82e6f0);
    image.put(at + 8, data);
    image.put(at + 12, static_cast<std::uint32_t>(std::strlen(s)));
    text(image, data, s);
  }
  void object(std::uint32_t at, unsigned id, unsigned amount, unsigned flags) {
    if (!image.at(at, 0x348))
      image.block(at, 0x348);
    image.put(at, ubc::hand_detail::item_vtable);
    image.put(at + 4, 1);
    image.put(at + 0x154, 1);
    image.put(at + 0x158, flags);
    image.put(at + 0x32c, amount);
    image.put(at + 0x330, id);
  }
  static constexpr std::uint32_t body_address() { return station_test::code + 792600; }
  static bool identity(void*, std::uint32_t at, std::size_t n) noexcept {
    return at == body_address() && n == 77;
  }
  static bool writable(void*, std::uint32_t at, std::size_t n) noexcept {
    return at == body_address() + 62 && n == 4;
  }
  static bool replace(void* p, std::uint32_t at, const void* old, const void* value,
                      std::size_t n) noexcept {
    auto& f = *static_cast<PreFixture*>(p);
    auto* b = f.image.at(at, n);
    if (!b || !writable(p, at, n) || std::memcmp(b, old, n))
      return false;
    std::memcpy(b, value, n);
    return true;
  }
  pr::Input input(pr::Phase phase = pr::Phase::before_extra, const pre::Call* call = nullptr) {
    pr::Input q;
    q.job = {{1, 7, ubc::lhiver::game_parser, runtime_game, world, hero, 0x8800},
             10,
             20,
             30,
             40,
             actual_info,
             ubc::station_abi::information_manager};
    q.scope = {{10, 20, 20775, 1, 0,
                pr::reads_saved_call(phase) || phase == pr::Phase::after_extra ? 1u : 0u, true},
               40,
               actual_info,
               true};
    q.phase = phase;
    q.serial = ++serial;
    q.frame = 8;
    q.current_thread = 7;
    q.destroy_started = q.destroy_completed = 0;
    q.engine = {0x400000, 0x752000, ubc::station_abi::executable_sha256, true};
    q.modules = modules;
    q.amount_patch = &patch;
    q.call = call;
    q.stack = {true, 7, 0x3f000000, 0x3f100000};
    return q;
  }
  void original_values(unsigned printed) {
    const auto parser = ubc::lhiver::game_parser;
    image.put(parser + 0x4c, station_test::code + 2345);
    image.put(parser + 0x2058, 2);
    image.put(parser + ubc::station_abi::parser_data_stack, printed);
    image.put(parser + ubc::station_abi::parser_data_stack + 4, 0x40);
    image.put(cold_object + 0x32c, u32(image.at(cold_object + 0x32c, 4)) + 2);
    image.put(station_test::symbol(12330) + 0x18, 0);
    image.put(hero + 0x298, 0);
    // Unreadable successors: entering the original consumes the short split fence.
    image.put(hero + 0xfc, 0xdeadbeec);
    image.put(station_test::symbol(1682) + 0x1c, 0xdeadbaac);
  }
  void split_values() {
    image.put(hot_object + 0x32c, 1);
    image.put(ubc::lhiver::game_parser + 0x4c, station_test::code + 365905);
    image.put(station_test::symbol(16612) + 0x1c, 0x160000);
    image.put(station_test::symbol(1682) + 0x1c, 0x160000);
    image.put(ubc::station_abi::instance_symbol_pointer, station_test::symbol(16612));
    image.put(ubc::station_abi::instance_address_pointer, 0x160000);
  }
  void restore_values(const pre::Call& call) {
    image.put(ubc::lhiver::game_parser + 0x4c, call.parser.code_base + call.parser.pop_pos);
    image.put(ubc::station_abi::instance_symbol_pointer, call.parser.use_symbol);
    image.put(ubc::station_abi::instance_address_pointer, call.parser.use_address);
    image.put(station_test::symbol(1682) + 0x1c, call.item.value);
  }
  void pool_values(unsigned count) {
    // The contiguous engine image remains a single block.
    image.put(n::node_pool, 12);
    image.put(n::node_pool + 4, 4096);
    image.put(n::node_pool + 8, 0x40000);
    image.put(n::node_pool + 12, count ? 0x50000 : 0);
    image.put(n::node_pool + 16, count);
    image.put(0x8a2558, 0);
    image.put(0x8d8780, 0);
    for (unsigned i = 0; i < count; ++i) {
      const auto node = 0x50000 + i * 12;
      image.block(node, 12);
      image.put(node, i + 1 < count ? node + 12 : 0);
      image.put(node + 4, i ? node - 12 : n::node_pool + 12);
    }
  }
};
bool closed(const pr::Capture& e) {
  return !e.known && !e.observation.stable_read && !e.observation.native_split_restore &&
         !e.observation.fence.known && !e.observation.fence.path_excludes_dispatch &&
         !e.restore.targets.known && !e.restore.native_set_item;
}
pr::Reader* reentrant_reader = nullptr;
pr::Input reentrant_input{};
pr::Issue reentrant_issue{};
void reenter_pre(Image& image) noexcept {
  reentrant_issue = reentrant_reader->capture(image.memory(), reentrant_input).issue;
}
pr::Input* mutable_input = nullptr;
void mutate_input(Image&) noexcept {
  mutable_input->job.id++;
  mutable_input->current_thread++;
  mutable_input->frame++;
  mutable_input->amount_patch = nullptr;
}
pre::admission::InfoLease* mutable_info_lease = nullptr;
void retire_info_lease(Image&) noexcept { mutable_info_lease->invalidate(); }
struct CodeBaseChange {
  PreFixture& fixture;
  bool changed = false;
  static bool copy(void* p, std::uint32_t address, void* out, std::size_t n) noexcept {
    auto& self = *static_cast<CodeBaseChange*>(p);
    if (!self.changed && address == n::node_pool) {
      self.changed = true;
      for (const auto offset : {0x48u, 0x4cu, 0x50u}) {
        const auto at = ubc::lhiver::game_parser + offset;
        self.fixture.image.put(at, u32(self.fixture.image.at(at, 4)) + 0x1000000u);
      }
    }
    return self.fixture.image.memory().copy(address, out, n);
  }
};
void pre_files(const char* ex, const char* da, const char* zb, const char* un, const char* cr,
               const char* zp, const char* ren) {
  const PE exe(ex, ubc::station_abi::executable_sha256),
      zbug(zb, ubc::zbug_end_compat::zbug_file_sha256),
      core(un, ubc::zbug_end_compat::union_file_sha256), crt(cr, n::dep::crt_sha256),
      zpe(zp, n::dep::parser_extender_sha256), renderer(ren, renderer_code::file_sha256);
  const Dat dat(da);
  {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    for (const auto target : {0u, 1u, 2u}) {
      f.image.put(0x8800 + 0x1fc, target);
      check(reader.capture(f.image.memory(), f.input()).known == (target <= 1),
            "nur die beiden bekannten Zielzustaende am kompletten Kuehlprofil");
    }
    f.image.put(0x8800 + 0x1fc, 0);
    f.image.put(0x8800 + 0x200, 1);
    check(!reader.capture(f.image.memory(), f.input()).known, "Ziel0 mit fremdem rewind gesperrt");
    f.image.put(0x8800 + 0x200, 0);
    f.zstring(0x8800 + 0x1b8, 0x25500, "FALSCH");
    check(!reader.capture(f.image.memory(), f.input()).known,
          "Ziel0 mit fremdem Stationsschema gesperrt");
  }
  {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    const auto reader = std::make_unique<pr::Reader>(7);
    check(f.patch.finish(), "Logstufe vor ungepatchter Zulassung");
    for (const unsigned raw : {0u, 9u, 10u, 127u, 128u, 255u}) {
      *f.image.at(0x8cdcf0, 1) = static_cast<std::uint8_t>(raw);
      const auto q = f.input(pr::Phase::admission);
      const auto capture = reader->capture(f.image.memory(), q);
      const auto report = pre::context_check::diagnose(f.image.memory(), q.job.context);
      const auto& row = report.rows[report.count - 1];
      const int level = raw <= 127 ? static_cast<int>(raw) : static_cast<int>(raw) - 256;
      check(capture.known == (level < 10) && report.count == 31 && !report.overflow &&
                row.matches == capture.known && std::string(row.actual) == std::to_string(level) &&
                (capture.known || (capture.issue == pr::Issue::context &&
                                   std::string(capture.failure.field) == "engine.report")),
            "Vorzeichen, Diagnose und Ablehnungsfeld entsprechen dem JL der Engine");
    }
    f.image.put(station_test::symbol(12330) + 0x18, UINT32_MAX);
    f.image.put(station_test::symbol(10536) + 0x18, UINT32_MAX - 1);
    const auto script = pre::context_check::diagnose(f.image.memory(), f.input().job.context);
    unsigned negative = 0;
    for (std::size_t i = 0; i < script.count; ++i) {
      const auto& row = script.rows[i];
      if (!std::strcmp(row.field, "script.production") && !row.matches &&
          !std::strcmp(row.actual, "-1"))
        ++negative;
      if (!std::strcmp(row.field, "script.start_smithing") && !row.matches &&
          !std::strcmp(row.actual, "-2"))
        ++negative;
    }
    check(negative == 2, "Beide negativen Skriptwerte erscheinen mit Vorzeichen");
  }
  {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    constexpr auto data = 0x48020000u;
    f.image.block(data, 8);
    f.image.put(data, 0x610000u);
    f.image.put(data + 4, 0x620000u);
    f.image.put(event_manager + 0x30, data);
    f.image.put(event_manager + 0x34, 2);
    f.image.put(event_manager + 0x38, 2);
    f.image.put(event_manager + 0x2c, 0x330000);
    f.image.put(event_manager + 0x28, 0);
    pre::Tracker tracker;
    auto first = reader.capture(f.image.memory(), f.input());
    check(first.known && tracker.admit(first.observation.job, first.observation),
          "zwei wartende Nachrichten bei exklusivem Split erlaubt");
    const auto late = reader.capture(f.image.memory(), f.input());
    const auto call = tracker.enter_extra(late.observation);
    f.image.put(data + 4, 0x630000u); // Header, count and capacity stay the same.
    const auto changed = reader.capture(f.image.memory(), f.input(pr::Phase::before_remove, &call));
    check(!changed.known && changed.failure.field &&
              std::string(changed.failure.field) == "restore.queue",
          "ausgetauschter Nachrichteneintrag vor Remove gesperrt");
    f.image.put(event_manager + 0x3c, hero + 4);
    f.image.put(hot_object + 0x158, 7);
    f.image.put(0x8800 + 0x1f8, 3);
    auto report = std::make_unique<pre::check::Report>();
    const auto run = reader.capture(f.image.memory(), f.input(), report.get());
    const auto has = [&](const char* field, const char* actual) {
      for (std::size_t i = 0; i < report->count; ++i) {
        const auto& r = report->rows[i];
        if (!r.matches && !std::strcmp(r.field, field) && !std::strcmp(r.actual, actual))
          return true;
      }
      return false;
    };
    check(!run.known && !report->overflow && has("queue.owner", std::to_string(hero + 4).c_str()) &&
              has("inventory.hot_flags", "7") && has("station.count", "3"),
          "unabhängige Fehler im selben Read mit Istwert");
  }
  // Insert the 0.0.39 world observations individually. The first
  // mismatch must not mask the cross-check at another site.
  for (bool strcmp_entry : {true, false}) {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    const auto start = strcmp_entry ? 0x7d5940u : 0x7d0bbcu;
    const auto length = strcmp_entry ? 132u : 90u;
    const auto at = start + (strcmp_entry ? 0u : 45u);
    const auto target =
        strcmp_entry ? f.modules.union_core.base + 0x57a10u : f.modules.zbug.base + 0x9ba0u;
    auto* entry = f.image.at(at, 8);
    std::array<std::uint8_t, 8> original{};
    std::copy_n(entry, original.size(), original.begin());
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    const auto rejects = [&]() {
      const auto e = reader.capture(f.image.memory(), f.input());
      return closed(e) && e.issue == pr::Issue::engine && e.failed_address == start;
    };
    entry[0] = strcmp_entry ? 0xe9 : 0xe8;
    if (strcmp_entry)
      std::fill_n(entry + 5, 3, 0xcc);
    f.image.put(at + 1, target - (at + 5));
    check(reader.capture(f.image.memory(), f.input()).known,
          "Erstsprung in ein geladenes Plugin genügt");
    f.image.put(at + 1, target + 1 - (at + 5));
    check(reader.capture(f.image.memory(), f.input()).known,
          "neue Ziel-RVA im selben Modul zugelassen");
    f.image.put(at + 1, f.modules.zbug.base + 64 - (at + 5));
    check(rejects(), "PE-Kopf im richtigen Modul ist kein ausführbares Weiterleitungsziel");
    f.image.put(at + 1, 0x12345678 - (at + 5));
    check(rejects(), "Sprung ausserhalb der geladenen Plugins gesperrt");
    std::copy(original.begin(), original.end(), entry);
    *f.image.at(start + length - 1, 1) ^= 1;
    check(rejects(), "geändertes Funktionsende bei originalem Aufruf gesperrt");
  }
  {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    const auto mode = f.modules.runtime_crt.base + 0xb45f4;
    f.image.put(mode, 1);
    constexpr std::uint32_t encoded_null = 0x31415892;
    const auto handler = f.modules.runtime_crt.base + 0xb4460;
    f.image.put(handler, encoded_null);
    auto q = f.input();
    q.encoded_null_handler = encoded_null;
    check(reader.capture(f.image.memory(), q).known,
          "Newmode 1 mit kodiertem Nullhandler zugelassen");
    q = f.input();
    q.encoded_null_handler = encoded_null;
    f.image.put(handler, encoded_null + 1);
    check(reader.capture(f.image.memory(), q).issue == pr::Issue::crt,
          "registrierter Handler könnte im Split erneut Anwendungscode aufrufen");
    f.image.put(mode, 0);
    for (const auto cow : std::array<std::uint8_t, 2>{0xfe, 0xff}) {
      *f.image.at(station_test::name_at(16612) - 1, 1) = cow;
      check(reader.capture(f.image.memory(), f.input()).known,
            "Zuweisung kopiert den voll belegten oder eingefrorenen Quellnamen");
    }
    *f.image.at(station_test::name_at(16612), 1) = 'X';
    check(closed(reader.capture(f.image.memory(), f.input())),
          "Inhalt des Klingennamens bleibt gebunden");
  }
  {
    // Read the same parser header in two phases. Distinguish the fields in use from the remaining bytes.
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    const auto fingerprint = [&]() {
      pr::detail::Trace trace{f.image.memory()};
      std::array<std::uint8_t, 0x58> head{};
      check(trace.memory().copy(ubc::lhiver::game_parser, head.data(), head.size()),
            "Parserkopf lesbar");
      return trace.hash.finish();
    };
    const auto baseline = fingerprint();
    for (unsigned off = 0; off < 0x58; ++off) {
      auto& byte = *f.image.at(ubc::lhiver::game_parser + off, 1);
      byte ^= 1;
      const bool layout = (off >= 0x18 && off < 0x24) || (off >= 0x48 && off < 0x4c) || off >= 0x50;
      check((fingerprint() != baseline) == layout,
            "Nur Tabellen- und Codegrenzen zählen zum Layoutvergleich");
      byte ^= 1;
    }
  }
  // Admission must read the actual, still-empty stack BEFORE CallFunc.Clear
  // and the unchanged body. The later Clear must not repair anything.
  for (unsigned mode = 0; mode < 23; ++mode) {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    check(f.patch.finish(), "Admissionfixture restauriert Startpatch");
    pre::admission::InfoLease lease;
    auto q = f.input(pr::Phase::admission);
    q.job.info_token = lease.open(pre::admission::identity(q.job, q.frame));
    q.scope.info_token = q.job.info_token;
    pre::admission::WriterEntry writer{
        q.job.writer_token, q.job.context.parser, 20775, 0x703981, 7, true, true};
    switch (mode) {
    case 1:
      f.image.put(ubc::lhiver::game_parser + 0x2058, 2);
      break;
    case 2:
      f.image.put(PreFixture::body_address() + 62, 2);
      break;
    case 3:
      check(f.patch.begin({f.image.memory(), &f, &PreFixture::identity, &PreFixture::writable,
                           &PreFixture::replace},
                          PreFixture::body_address(), f.amount.body(), 2),
            "frueher Patch eingerichtet");
      break;
    case 4:
      lease.invalidate();
      break;
    case 5:
      writer.token++;
      break;
    case 6:
      writer.parser += 4;
      break;
    case 7:
      writer.callback = 20778;
      break;
    case 8:
      writer.caller = 0x662b6f;
      break;
    case 9:
      writer.thread++;
      break;
    case 10:
      writer.exclusive = false;
      break;
    case 11:
      writer.original_bound = false;
      break;
    case 12:
      q.scope.info_token++;
      break;
    case 13:
      q.frame++;
      break;
    case 14:
      q.job.actual_info += 4;
      break;
    case 15:
      q.destroy_started = 1;
      break;
    case 16:
      q.destroy_completed = 1;
      break;
    case 17:
      f.image.put(event_manager + 0x38, 1);
      break;
    case 18:
      f.image.put(hot_object + 0x32c, 1);
      break;
    case 19:
      q.phase = pr::Phase::before_extra;
      break;
    case 20:
      q.amount_patch = nullptr;
      break;
    case 21:
      q.scope.writer.vm_depth = 1;
      break;
    case 22:
      mutable_info_lease = &lease;
      f.image.on_read = &retire_info_lease;
      break;
    }
    const auto operand = u32(f.image.at(PreFixture::body_address() + 62, 4));
    const auto stack = u32(f.image.at(ubc::lhiver::game_parser + 0x2058, 4));
    pre::Rejection rejection;
    const auto prepared =
        pre::admission::prepare(reader, f.image.memory(), q, lease, writer, &rejection);
    check(prepared.valid() == (mode == 0),
          "nur aktuelle Writer-/INFO-Lease mit unberuehrtem Original und leerem Stack vorbereitet");
    check((rejection.event == nullptr) == prepared.valid(),
          "nur abgelehnte Zulassungen hinterlassen eine Diagnose");
    if (mode == 4 || mode == 5 || mode == 22) {
      const auto gate = mode == 5 ? pre::Gate::admission_entry : pre::Gate::info_lease;
      check(rejection.event && !std::strcmp(rejection.event, "cool_predebit_gate_rejected") &&
                rejection.phase == pr::Phase::admission &&
                rejection.reason == static_cast<int>(gate),
            "Writer- und INFO-Besitz werden als eigene Prüfstelle gemeldet");
    }
    check(u32(f.image.at(PreFixture::body_address() + 62, 4)) == operand &&
              u32(f.image.at(ubc::lhiver::game_parser + 0x2058, 4)) == stack,
          "Admission schreibt weder Operand noch Parserstack");
    if (mode == 0) {
      check(prepared.current(lease) && prepared.serial() == q.serial,
            "Prepared bindet wirklichen Besitzer und frische Aufnahme");
      pre::admission::InfoLease other;
      check(other.open(pre::admission::identity(q.job, q.frame)) == q.job.info_token &&
                !prepared.current(other),
            "gleiche Werte in fremdem Leasebesitzer gelten nicht");
      check(f.patch.begin({f.image.memory(), &f, &PreFixture::identity, &PreFixture::writable,
                           &PreFixture::replace},
                          PreFixture::body_address(), f.amount.body(), 2),
            "Patch erst nach erfolgreicher Admission");
      auto next = f.input();
      next.job = q.job;
      next.scope.info_token = q.job.info_token;
      const auto after = reader.capture(f.image.memory(), next);
      check(after.known && prepared.matches(after.observation),
            "aktives Patch2 schliesst exakt an vorbereitete Werte an");
      lease.close(q.job.info_token);
      check(!prepared.current(lease), "finally entwertet Prepared");
      check(lease.open(pre::admission::identity(q.job, q.frame)) > q.job.info_token &&
                !prepared.current(lease),
            "gleicher INFO-Pointer in neuer Lease kann altes Prepared nicht wiederbeleben");
    }
  }
  {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    pre::Tracker tracker;
    f.image.put(station_test::symbol(1682) + 0x1c,
                0x45000000); // Deliberately leave the old ITEM instance unreadable.
    const auto q = f.input();
    const auto first = reader.capture(f.image.memory(), q);
    if (!first.known)
      std::fprintf(stderr, "pre issue=%u address=%x\n", unsigned(first.issue),
                   first.failed_address);
    check(
        first.known && first.observation.fence.path_excludes_dispatch,
        "full original code/script/module and actual empty queue compose conditional pre-split path");
    check(first.observation.complete_inventory &&
              first.observation.inventory_identity != ubc::lhiver::Digest{} &&
              first.observation.hand_identity != ubc::lhiver::Digest{},
          "complete inventory and slot copies produce nonempty bindings");
    check(first.observation.stocks.stocks[0].amount == 2 &&
              first.observation.stocks.stocks[1].amount == 5,
          "canonical original stocks are copied");
    check(tracker.admit(q.job, first.observation),
          "real reader composition admits exact pre-debit plan");
    const auto late = reader.capture(f.image.memory(), f.input());
    const auto call = tracker.enter_extra(late.observation);
    check(pre::valid_call(call) && tracker.call().valid,
          "Tracker reserviert den Aufruf vor dem simulierten Entzug");
    f.split_values();
    for (const auto address : {0x7a4b20u, 0x7a1550u, 0x7a2f20u}) {
      auto* opcode = f.image.at(address, 1);
      *opcode ^= 1;
      const auto capture = reader.capture(f.image.memory(), f.input(pr::Phase::restore, &call));
      check(!capture.known && capture.failure.field &&
                !std::strcmp(capture.failure.field, "restore.setter_code") &&
                !pre::restore_plan(call, capture.restore).valid,
            "geänderter Parser-Setter bleibt nach dem Split gesperrt");
      *opcode ^= 1;
    }
    for (unsigned part = 0; part < 4; ++part) {
      const auto restore = reader.capture(f.image.memory(), f.input(pr::Phase::restore, &call));
      if (!restore.known)
        std::fprintf(stderr, "restore part=%u issue=%u address=%x\n", part, unsigned(restore.issue),
                     restore.failed_address);
      const auto plan = pre::restore_plan(call, restore.restore);
      check(restore.known && plan.valid,
            "fresh complete restore gate before each setter and after all setters");
      if (part == 0)
        f.image.put(ubc::lhiver::game_parser + 0x4c, call.parser.code_base + call.parser.pop_pos);
      if (part == 1) {
        f.image.put(ubc::station_abi::instance_symbol_pointer, call.parser.use_symbol);
        f.image.put(ubc::station_abi::instance_address_pointer, call.parser.use_address);
      }
      if (part == 2)
        f.image.put(station_test::symbol(1682) + 0x1c, call.item.value);
    }
    const auto after = reader.capture(f.image.memory(), f.input(pr::Phase::after_extra));
    check(after.known && tracker.extra_returned(after.observation, true, true),
          "restored split delta Hot1 Cold unchanged accepts with opaque old ITEM value");
    const auto ready = reader.capture(f.image.memory(), f.input(pr::Phase::before_original));
    check(ready.known && tracker.enter_original(ready.observation),
          "fresh INFO plus Hot1 permits original entry without repeating old Hot2 precondition");
    check(!tracker.witness().valid,
          "reader and pre-debit do not manufacture original or final success");
    f.original_values(0);
    check(!reader.capture(f.image.memory(), f.input(pr::Phase::after_original)).known,
          "AFTER bei aktivem Patch gesperrt");
    check(f.patch.finish(), "Amount2 vor AFTER exakt restauriert");
    const auto final = reader.capture(f.image.memory(), f.input(pr::Phase::after_original));
    check(final.known && !final.observation.fence.known && !final.restore.native_set_item &&
              tracker.original_returned(final.observation, true, true) && tracker.witness().valid,
          "frische AFTER-Bilanz, generierte Hand und aktiver Callback-Prefix bei Produktion0");
    check(final.observation.stocks.stocks[1].amount == 7 &&
              final.observation.parser.data_words[0] == 0 &&
              final.observation.parser.data_words[1] == 0x40,
          "Original-PrintReturn0 und echter Cold-Zuwachs2");
    check(reader.capture(f.image.memory(), q).issue == pr::Issue::serial,
          "old serial cannot be replayed");
  }
  {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    for (unsigned free : {0u, 1u, 2u, 3u}) {
      f.pool_values(free);
      check(reader.capture(f.image.memory(), f.input()).known,
            "actual finite pool reuse or one growth composes without renderer route");
    }
    f.pool_values(0);
    for (const auto& area : pre::profile::engine)
      for (const auto offset : {0u, area.length / 2, area.length - 1}) {
        auto* b = f.image.at(area.address + offset, 1);
        *b ^= 1;
        const auto e = reader.capture(f.image.memory(), f.input());
        check(closed(e) && e.issue == pr::Issue::engine && e.failed_address == area.address,
              "every full new engine range binds entry middle and final byte");
        *b ^= 1;
      }
    for (const auto& area : pre::profile::data)
      for (const auto offset : {0u, area.length - 1}) {
        auto* b = f.image.at(area.address + offset, 1);
        *b ^= 1;
        const auto e = reader.capture(f.image.memory(), f.input());
        check(closed(e) && e.issue == pr::Issue::engine,
              "RTTI graph and complete VM dispatch tables cannot drift");
        *b ^= 1;
      }
    for (const auto& c : pre::profile::free_code)
      for (std::uint32_t i = 0; i < c.length; ++i) {
        auto* b = f.image.at(f.modules.runtime_crt.base + c.rva + i, 1);
        *b ^= 1;
        check(closed(reader.capture(f.image.memory(), f.input())),
              "all CRT free bytes including relocation fields are bound");
        *b ^= 1;
      }
    for (const auto& c : ubc::cool_message_malloc::profile::malloc_code)
      for (const auto off : {0u, c.length - 1}) {
        auto* b = f.image.at(f.modules.runtime_crt.base + c.rva + off, 1);
        *b ^= 1;
        check(closed(reader.capture(f.image.memory(), f.input())),
              "full reused malloc range uses CRT preferred base");
        *b ^= 1;
      }
    for (const auto at :
         {0x82f104u + 0x3c, 0x83e174u + 0x6c, 0x83c804u + 0x78, 0x82fdb8u, 0x8922a4u}) {
      auto* b = f.image.at(at, 1);
      *b ^= 1;
      check(closed(reader.capture(f.image.memory(), f.input())),
            "actual virtual destinations and string lookup data remain bound");
      *b ^= 1;
    }
  }
  {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    struct Change {
      std::uint32_t address, value;
    };
    const Change changes[]{{event_manager, 0x83e240},
                           {event_manager + 0x38, 1},
                           {event_manager + 0x3c, hero + 4},
                           {hero + 0xfc, ubc::station_abi::information_manager},
                           {hero + 0x298, 0},
                           {0xab642c, 0},
                           {0xab11e4, 0},
                           {ubc::lhiver::game_parser + 0x2198, 1},
                           {0x8cdcf0, 10},
                           {0xab1168 + 0x48, 1},
                           {0x8b7464, 1585},
                           {hot_object + 0x32c, 3},
                           {hot_object + 0x158, 0xa00001},
                           {cold_object + 0x32c, INT32_MAX},
                           {hand_object + 0x158, 0x200001},
                           {0x31000 + 0x18, 0},
                           {actual_info + 0x2c, 20778},
                           {actual_info + 0x50, 20776},
                           {ubc::station_abi::information_manager + 0x20, hero + 4},
                           {station_test::symbol(12330) + 0x18, 0},
                           {f.modules.runtime_crt.base + 0xb38b0, 0},
                           {f.modules.runtime_crt.base + 0xb45f4, 1},
                           {f.modules.runtime_crt.base + 0x11f8, 0},
                           {f.modules.runtime_crt.base + 0x11fc, 0},
                           {0x82e308, 0x12345678},
                           {f.modules.parser_extender.base + 0xc0b1c, 0x1234},
                           {f.modules.parser_extender.base + 0xc44c0, 0},
                           {PreFixture::body_address() + 62, 3},
                           {station_test::symbol(1590) + 0x1c, 0x150}};
    for (const auto c : changes) {
      const auto saved = u32(f.image.at(c.address, 4));
      f.image.put(c.address, c.value);
      check(
          closed(reader.capture(f.image.memory(), f.input())),
          "concrete queue/context/constructor/material/INFO/CRT/own operand mutation rejects whole composition");
      f.image.put(c.address, saved);
    }
    for (const auto& s : pre::profile::texts) {
      const auto object = u32(f.image.at(station_test::symbol(s.symbol) + 0x18, 4));
      const auto data = u32(f.image.at(object + 8, 4));
      for (auto at : {object, object + 16}) {
        const auto saved = u32(f.image.at(at, 4));
        f.image.put(at, 0);
        check(closed(reader.capture(f.image.memory(), f.input())),
              "real constructor zSTRING header and capacity are checked");
        f.image.put(at, saved);
      }
      *f.image.at(data - 1, 1) = 0xfe;
      check(closed(reader.capture(f.image.memory(), f.input())),
            "unknown COW branch cannot borrow content-only hash");
      *f.image.at(data - 1, 1) = 0;
    }
    for (unsigned fault = 0; fault < 17; ++fault) {
      auto q = f.input();
      const auto reads = f.image.reads;
      switch (fault) {
      case 0:
        q.current_thread++;
        break;
      case 1:
        q.job.context.thread++;
        break;
      case 2:
        q.job.context.epoch = 0;
        break;
      case 3:
        q.job.id = 0;
        break;
      case 4:
        q.job.writer_token = 0;
        break;
      case 5:
        q.job.fence_token = 0;
        break;
      case 6:
        q.scope.info_lease_active = false;
        break;
      case 7:
        q.scope.writer.vm_depth = 1;
        break;
      case 8:
        q.scope.writer.debit_depth = 1;
        break;
      case 9:
        q.destroy_started = 1;
        break;
      case 10:
        q.frame = 0;
        break;
      case 11:
        q.stack.verified = false;
        break;
      case 12:
        q.stack.thread++;
        break;
      case 13:
        q.amount_patch = nullptr;
        break;
      case 14:
        q.phase = static_cast<pr::Phase>(99);
        break;
      case 15:
        q.scope.actual_info += 4;
        break;
      case 16:
        q.job.information_manager += 4;
        break;
      }
      check(closed(reader.capture(f.image.memory(), q)) && f.image.reads == reads,
            "foreign or incomplete ownership/scope closes before any read");
    }
    for (unsigned fault = 0; fault < 6; ++fault) {
      auto q = f.input();
      switch (fault) {
      case 0:
        q.engine.mapping_verified = false;
        break;
      case 1:
        q.modules.union_core.mapping_verified = false;
        break;
      case 2:
        q.modules.runtime_crt.file_sha256[0] ^= 1;
        break;
      case 3:
        q.modules.parser_extender.base = q.modules.union_core.base;
        break;
      case 4:
        q.modules.zbug.image_size = 0;
        break;
      case 5:
        q.engine.file_sha256[0] ^= 1;
        break;
      }
      check(closed(reader.capture(f.image.memory(), q)),
            "every actual module lease remains separate and current");
    }
    ubc::AmountPatch foreign;
    auto q = f.input();
    q.amount_patch = &foreign;
    check(reader.capture(f.image.memory(), q).issue == pr::Issue::amount,
          "equal memory operand alone cannot forge own active AmountPatch");
    f.image.denied = hand_object + 0x158;
    f.image.denied_size = 4;
    check(closed(reader.capture(f.image.memory(), f.input())), "unreadable hand is not zero-known");
    f.image.denied_size = 0;
    const auto saved = u32(f.image.at(event_manager + 0x28, 4));
    f.image.change_address = event_manager;
    f.image.change_target = event_manager + 0x28;
    f.image.change_on_read = 2;
    f.image.watched_reads = 0;
    f.image.put(event_manager + 0x28, 99);
    const auto changed = reader.capture(f.image.memory(), f.input());
    check(closed(changed) && changed.issue == pr::Issue::changed,
          "second whole-copy change clears every positive projection");
    f.image.change_on_read = 0;
    f.image.put(event_manager + 0x28, saved);
    reentrant_reader = &reader;
    reentrant_input = f.input();
    f.image.on_read = &reenter_pre;
    check(closed(reader.capture(f.image.memory(), f.input())) &&
              reentrant_issue == pr::Issue::reentry,
          "nested read poisons outer capture without sharing mutable workspace");
    auto mutable_q = f.input();
    mutable_input = &mutable_q;
    f.image.on_read = &mutate_input;
    const auto copied = reader.capture(f.image.memory(), mutable_q);
    check(copied.known && copied.observation.job.id == 10 && copied.observation.frame == 8,
          "entry copies caller request before invoking memory provider");
    check(f.patch.finish(), "fixture restores exactly own amount bytes");
    check(closed(reader.capture(f.image.memory(), f.input())),
          "closed AmountPatch cannot grant pre-debit");
  }
  for (unsigned fault = 0; fault < 7; ++fault) {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    pre::Tracker t;
    auto e = reader.capture(f.image.memory(), f.input());
    check(e.known && t.admit(e.observation.job, e.observation),
          "restore-negative case has genuine positive composite first");
    auto late = reader.capture(f.image.memory(), f.input());
    auto call = t.enter_extra(late.observation);
    check(pre::valid_call(call), "Rückführungsfall bindet den eigenen Aufruf");
    f.split_values();
    auto q = f.input(pr::Phase::restore, &call);
    switch (fault) {
    case 0:
      f.image.put(cold_object + 0x32c, 6);
      break;
    case 1:
      f.image.put(0x31000 + 0x18, 2);
      break;
    case 2:
      f.image.put(event_manager + 0x28, 99);
      break;
    case 3:
      f.image.put(station_test::symbol(1682) + 0x20, 0);
      break;
    case 4:
      q.frame++;
      break;
    case 5:
      q.destroy_completed = 1;
      break;
    case 6:
      call.inventory_identity[0] ^= 1;
      break;
    }
    const auto rejected = reader.capture(f.image.memory(), q);
    check(closed(rejected) && !pre::restore_plan(call, rejected.restore).valid,
          "fresh restore refuses changed foreign stock/hand/queue/ITEM/frame/journal/binding");
    t.fail(pre::Issue::restore);
    check(t.call().valid && t.status().blocked,
          "fehlerhafte Rückführung sperrt den gebundenen Auftrag");
  }
  {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    pre::Tracker t;
    constexpr auto unrelated = 0x170000u;
    f.object(unrelated, 999, 9, 0x200001);
    f.image.block(0x11020, 12);
    f.image.put(0x11010 + 8, 0x11020);
    f.image.put(0x11020 + 4, unrelated);
    const auto e = reader.capture(f.image.memory(), f.input());
    check(e.known && t.admit(e.observation.job, e.observation),
          "complete extra inventory row is read even though not a recipe material");
    f.image.put(unrelated + 0x32c, 10);
    const auto late = reader.capture(f.image.memory(), f.input());
    check(late.known && !t.enter_extra(late.observation).valid && !t.call().valid,
          "foreign nonmaterial delta cannot pass through two-material projection");
  }
  // Independently identified unrelated write targets, beyond a mismatch in pool
  // bytes. The old prefix-only baseline accepts all four up to enter_extra.
  for (unsigned alias = 0; alias < 4; ++alias) {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    pre::Tracker t;
    constexpr auto item = station_test::symbol(1682);
    if (alias < 2) {
      f.pool_values(1);
      const auto head = alias ? 0x3101cu : item + 0x18;
      f.image.put(n::node_pool + 12, head);
      f.image.put(head, 0);
      f.image.put(head + 4, n::node_pool + 12);
    } else {
      f.pool_values(3);
      const auto tail = alias == 2 ? event_manager + 0x2c : item + 0x18;
      f.image.put(0x5000c, tail);
      f.image.put(tail + 4, 0x5000c);
      if (alias == 2)
        f.image.put(event_manager + 0x34, 1);
    }
    const auto q = f.input();
    const auto e = reader.capture(f.image.memory(), q);
    const bool admitted = t.admit(q.job, e.observation);
    const auto late = reader.capture(f.image.memory(), f.input());
    const auto call = t.enter_extra(late.observation);
    check(closed(e) && !admitted && !call.valid && !t.call().valid,
          "pool payload/successor writes may not alias ITEM, generated slot or event queue");
  }
  for (const auto level : {0, 1, -1}) {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    constexpr auto other = 0x170000u;
    f.object(other, 999, 9, 0x200001);
    f.image.block(0x11020, 12);
    f.image.put(0x11010 + 8, 0x11020);
    f.image.put(0x11020 + 4, other);
    f.image.block(0x30000, 12);
    f.image.put(0x30008, 0x31200);
    f.image.block(0x31200, 0x34);
    f.image.put(hero + 0x9b4, 3);
    f.image.put(hero + 0x9b8, 3);
    f.zstring(0x31200, 0x32200, "ZS_SWORD");
    f.image.put(0x31218, static_cast<std::uint32_t>(level));
    f.image.put(0x31230, other);
    const auto e = reader.capture(f.image.memory(), f.input());
    check(level == 0 ? e.known : closed(e),
          "unchanged listed foreign equipment tmp0 remains valid; temporary aliases remain closed");
    if (level == 0) {
      pre::Tracker t;
      check(t.admit(e.observation.job, e.observation),
            "ordinary equipment binds actual inventory and slot identity");
      const auto late = reader.capture(f.image.memory(), f.input());
      const auto call = t.enter_extra(late.observation);
      check(pre::valid_call(call), "unmodified ordinary equipment does not block exact pre-debit");
      f.split_values();
      const auto restore = reader.capture(f.image.memory(), f.input(pr::Phase::restore, &call));
      check(restore.known && pre::restore_plan(call, restore.restore).valid,
            "ordinary equipment survives fresh restore admission");
      f.image.put(other + 0x32c, 10);
      const auto changed = reader.capture(f.image.memory(), f.input(pr::Phase::restore, &call));
      check(closed(changed),
            "foreign equipped amount change cannot hide behind same material totals");
      f.image.put(other + 0x32c, 9);
      for (const auto address : {other + 4, other + 0x158, other + 0x1c0, 0x31220u}) {
        const auto before = u32(f.image.at(address, 4));
        f.image.put(address, before + 1);
        check(closed(reader.capture(f.image.memory(), f.input(pr::Phase::restore, &call))),
              "equipped metadata and full slot bytes remain part of unchanged restore identity");
        f.image.put(address, before);
      }
    }
  }
  // Give the reserved record fields valid symbol metadata. The
  // unrelated pool write alone must still block admission entirely.
  for (const auto id : {1679u, 1683u, 1682u, 16612u, 1584u, 12330u, 20775u})
    for (unsigned kind = 0; kind < 2; ++kind) {
      PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
      auto reader_storage = std::make_unique<pr::Reader>(7);
      auto& reader = *reader_storage;
      const auto record = station_test::symbol(id);
      f.pool_values(kind ? 3 : 1);
      if (!kind) {
        f.image.put(n::node_pool + 12, record + 0x24);
        f.image.put(record + 0x24, 0);
        f.image.put(record + 0x28, n::node_pool + 12);
      } else {
        f.image.put(0x5000c, record + 0x24);
        f.image.put(record + 0x28, 0x5000c);
      }
      check(closed(reader.capture(f.image.memory(), f.input())),
            "all actually read parser symbol records reject payload and successor writes");
    }
  for (unsigned kind = 0; kind < 2; ++kind) {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    constexpr auto first = 0x1800000u, second = first + 0x100;
    f.pool_values(kind ? 3 : 2);
    f.image.block(first, 0x110);
    f.image.put(n::node_pool + 12, first);
    f.image.put(first, kind ? second : first + 8);
    f.image.put(first + 4, n::node_pool + 12);
    if (!kind) {
      f.image.put(first + 8, 0);
      f.image.put(first + 12, first);
    } else {
      f.image.put(second, first + 8);
      f.image.put(second + 4, first);
      f.image.put(first + 8, 0);
      f.image.put(first + 12, second);
    }
    check(closed(reader.capture(f.image.memory(), f.input())),
          "allocated payloads and remaining whole successor are internally disjoint");
  }
  for (unsigned target = 0; target < 3; ++target) {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    // The old validators accept these values. Only the unrelated successor.prev
    // write is wrong. World list nodes do not belong to the inventory.
    const auto address = target == 0 ? 0x11000u : target == 1 ? 0x70000u : 0x72000u;
    if (target == 2) {
      f.image.block(address, 12);
      f.image.put(address + 4, hot_object);
      f.image.put(0x70008, address);
    }
    f.pool_values(3);
    f.image.put(0x5000c, target == 1 ? address : address - 4);
    if (target == 1)
      f.image.put(address + 4, 0x5000c);
    else {
      f.image.blocks.erase(address);
      f.image.block(address - 4, 16);
      f.image.put(address, 0x5000c);
      f.image.put(address + 4, hot_object);
      f.image.put(address + 8, target == 0 ? 0x11010 : 0);
    }
    check(closed(reader.capture(f.image.memory(), f.input())),
          "successor write cannot corrupt existing inventory or world list metadata");
  }
  for (unsigned free = 0; free < 4; ++free) {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    f.pool_values(free);
    f.image.put(n::node_pool + 8, 0x45000000); // Opaque and deliberately unreadable.
    check(reader.capture(f.image.memory(), f.input()).known,
          "zero/one-free growth and two/three-free reuse preserve opaque old block pointer");
  }
  {
    PreFixture f(exe, dat, zbug, core, crt, zpe, renderer);
    auto reader_storage = std::make_unique<pr::Reader>(7);
    auto& reader = *reader_storage;
    pre::Tracker tracker;
    constexpr auto delta = 0x1000000u;
    std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> copies;
    for (const auto& block : f.image.blocks)
      if (block.first >= station_test::code &&
          block.first < station_test::code + ubc::lhiver::code_length)
        copies.push_back({block.first + delta, block.second});
    for (const auto& block : copies)
      f.image.blocks[block.first] = block.second;
    f.image.put(PreFixture::body_address() + delta + 62, 1);
    CodeBaseChange moved{f};
    const auto q = f.input();
    const auto e = reader.capture({&moved, &CodeBaseChange::copy}, q);
    check(
        moved.changed && f.patch.matches_active(PreFixture::body_address(), f.amount.body(), 2),
        "codebase race retains genuine old patch journal while current script uses original operand1");
    check(closed(e) && !tracker.admit(q.job, e.observation),
          "single capture cannot bind current original codebase to old patched amount journal");
    const auto late = reader.capture(f.image.memory(), f.input());
    check(closed(late) && !tracker.enter_extra(late.observation).valid && !tracker.call().valid,
          "fresh follow-up likewise rejects the foreign codebase before any mutation");
  }
}
}
// Same file-facts CLI as for bows. Load the renderer only for its fixture base.
// It is neither a dependency nor an authorization for the short pre-split reader.
#ifndef UBC_COOL_PREDEBIT_READER_MAIN
#define UBC_COOL_PREDEBIT_READER_MAIN main
#endif
int UBC_COOL_PREDEBIT_READER_MAIN(int argc, char** argv) {
  static_assert(!pr::Capture::native_effect_authorized);
  {
    pr::Reader r(7);
    const auto e = r.capture({}, {});
    check(closed(e), "empty request is never an execution grant");
  }
  if (argc == 9 && std::string(argv[1]) == "--local-files")
    pre_files(argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
  else if (argc != 1)
    return 2;
  std::printf("%u checks, %u failures\n", station_test::checks, station_test::failures);
  return station_test::failures ? 1 : 0;
}
