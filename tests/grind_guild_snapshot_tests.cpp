// SPDX-License-Identifier: MIT
#include "ubc/grind_guild_snapshot.hpp"
#include "grind_runtime_fixture.hpp"
#include <iostream>

namespace guild_allocation { bool watching = false; unsigned count = 0; }
void* operator new(std::size_t size) {
  if (guild_allocation::watching) ++guild_allocation::count;
  if (auto* p = std::malloc(size ? size : 1)) return p;
  std::abort();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
using namespace grind_test;
using Issue = ubc::GrindGuildIssue;
namespace gd = ubc::grind_guild_snapshot_detail;
constexpr std::uint32_t hero = 0x2000;
ubc::GrindRecipeKey key() { return {1, 2, 7, 0x8000, hero, 0x8800, 0x9000, 11}; }
struct NativeFacts {
  std::array<std::array<std::uint8_t, 1024>, 2> bytes{};
  bool read() {
    for (std::size_t i = 0; i < gd::native_ranges.size(); ++i) {
      const auto& range = gd::native_ranges[i];
      if (!std::cin.read(reinterpret_cast<char*>(bytes[i].data()), static_cast<std::streamsize>(range.length))) return false;
      ubc::Sha256 hash; hash.update(bytes[i].data(), range.length);
      if (hash.finish() != range.digest) return false;
    }
    return std::cin.peek() == std::char_traits<char>::eof();
  }
  void load(Image& image) const {
    for (std::size_t i = 0; i < gd::native_ranges.size(); ++i) {
      const auto& range = gd::native_ranges[i]; image.block(range.address, range.length);
      std::memcpy(image.at(range.address, range.length), bytes[i].data(), range.length);
    }
  }
};
struct GuildFixture : Fixture {
  GuildFixture() : Fixture(g::recipes[0]) {
    image.block(hero, 0x234); image.put(hero + gd::guild_offset, 2);
    image.block(ubc::station_abi::hero_pointer, 4); image.put(ubc::station_abi::hero_pointer, hero);
  }
};
struct Probe {
  Image& image;
  unsigned guild_reads = 0, self_reads = 0, use_reads = 0;
  static bool read(void* raw, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& p = *static_cast<Probe*>(raw);
    const auto overlaps = [&](std::uint32_t target, std::size_t length) {
      return std::uint64_t{address} + size > target && std::uint64_t{target} + length > address;
    };
    if (overlaps(hero + gd::guild_offset, 4)) ++p.guild_reads;
    if (overlaps(symbol(1679), 0x3c)) ++p.self_reads;
    if (overlaps(ubc::station_abi::instance_symbol_pointer, 12)) ++p.use_reads;
    return Image::read(&p.image, address, output, size);
  }
  ubc::ReadOnlyMemory memory() { return {this, read}; }
};
ubc::GrindGuildSnapshot preview(GuildFixture& f, const ubc::GrindRecipeKey& k = key()) {
  return ubc::read_grind_guild_for_preview(f.image.memory(), ubc::lhiver::game_parser, k);
}
ubc::GrindWriterGuildSnapshot writer(GuildFixture& f, const ubc::GrindRecipeKey& k = key()) {
  return ubc::read_grind_guild_for_writer(f.image.memory(), ubc::lhiver::game_parser, k);
}
void portable() {
  check(!ubc::GrindGuildSnapshot{}.known() && !ubc::GrindWriterGuildSnapshot{}.known(), "leere Gildenbelege sind unbekannt");
  check(!ubc::GrindGuildSnapshot::native_effect_authorized && !ubc::GrindWriterGuildSnapshot::native_effect_authorized,
        "Gildenlesung ist keine Native-Herstellungsfreigabe");
  GuildFixture f;
  check(preview(f).issue == Issue::native_code && writer(f).issue == Issue::native_code,
        "fehlende gepinnte Nativekoerper sperren beide Reader");
  for (unsigned field = 0; field < 11; ++field) {
    auto k = key(); auto parser = ubc::lhiver::game_parser;
    switch (field) {
      case 0: k.generation = 0; break; case 1: k.frame = 0; break; case 2: k.thread = 0; break;
      case 3: k.world = 0; break; case 4: k.hero = 0; break; case 5: k.station = 0; break;
      case 6: k.manager = 0; break; case 7: k.production_kind = 13; break;
      case 8: parser = 0; break; case 9: k.hero = 0xfffffffc; break; case 10: ++k.hero; break;
    }
    const auto before = f.image.reads;
    const auto result = ubc::read_grind_guild_for_preview(f.image.memory(), parser, k);
    check(!result.known() && f.image.reads == before, "ungueltiger Kontext oder Adressueberlauf erzeugt keinerlei Lesung");
  }
}

void real_native(const NativeFacts& native) {
  for (const auto value : {INT32_MIN, -1, 0, 1, 2, 6, 7, INT32_MAX}) {
    GuildFixture f; native.load(f.image); f.image.put(hero + gd::guild_offset, static_cast<std::uint32_t>(value));
    Probe probe{f.image}; guild_allocation::watching = true;
    const auto observed = ubc::read_grind_guild_for_preview(probe.memory(), ubc::lhiver::game_parser, key());
    const auto active = ubc::read_grind_guild_for_writer(probe.memory(), ubc::lhiver::game_parser, key());
    guild_allocation::watching = false;
    check(observed.known() && observed.guild == value && observed.hero == hero && observed.guild_address == hero + 0x230 &&
          observed.hero_symbol.address == symbol(1683) && observed.hero_symbol.offset == hero,
          "wirklicher nativer INT32-Gildenwert; Symbolpointer bleibt von Objektadresse getrennt");
    check(active.known() && active.physical.guild == value && active.actor_before.requires_self && active.actor_after.requires_self,
          "Writer bindet zusaetzlich bestehendes SELF gleich HERO vor und nach dem Wert");
    check(probe.guild_reads == 4 && probe.self_reads >= 4 && !probe.use_reads && !guild_allocation::count,
          "doppelte Feldaufnahme ohne UseInstance-Getter oder Heap");
  }
  { GuildFixture f; native.load(f.image); f.image.put(symbol(1679) + 0x1c, hero + 4);
    Probe probe{f.image}; const auto observed = ubc::read_grind_guild_for_preview(probe.memory(), ubc::lhiver::game_parser, key());
    check(observed.known() && !probe.self_reads && !probe.use_reads, "physische Vorschau bleibt bei fremdem SELF gueltig ohne SELF-Lesung");
    check(writer(f).issue == Issue::actor, "dieselbe Aufnahme darf keinen aktiven Writer belegen"); }
  { GuildFixture f; native.load(f.image); f.image.denied = symbol(1679); f.image.denied_size = 0x3c;
    check(preview(f).known() && writer(f).issue == Issue::actor, "Vorschau benoetigt keinen lesbaren SELF-Slot"); }
  { GuildFixture f; native.load(f.image); f.image.denied = ubc::station_abi::instance_symbol_pointer; f.image.denied_size = 12;
    check(preview(f).known() && writer(f).known(), "ruhende UseInstance/Dummyglobals sind keine erfundene HERO-Voraussetzung"); }

  for (unsigned mode = 0; mode < 29; ++mode) {
    GuildFixture f; native.load(f.image); auto k = key();
    switch (mode) {
      case 0: f.image.put(ubc::station_abi::hero_pointer, 0); break;
      case 1: f.image.put(ubc::station_abi::hero_pointer, hero + 4); break;
      case 2: f.image.put(symbol(1683) + 0x1c, hero + 4); break;
      case 3: f.image.put(symbol(1683) + 0x1c, 0); break;
      case 4: f.image.put(symbol(1549) + 0x20, station_test::metadata(1, 2, 0)); break;
      case 5: f.image.put(symbol(1549) + 0x20, station_test::metadata(1, 3, 4)); break;
      case 6: f.image.put(symbol(1549) + 0x20, station_test::metadata(2, 2, 4)); break;
      case 7: f.image.put(symbol(1549) + 0x38, symbol(1584)); break;
      case 8: f.image.put(symbol(1549) + 0x1c, 0x234); break;
      case 9: f.image.put(symbol(1683) + 0x20, station_test::metadata(0, 6, 1)); break;
      case 10: f.image.put(symbol(1683) + 0x20, station_test::metadata(0, 7, 0)); break;
      case 11: f.image.put(symbol(1683) + 0x38, symbol(1584)); break;
      case 12: f.image.put(symbol(1537) + 0x20, station_test::metadata(32, 2, 0)); break;
      case 13: f.image.put(ubc::lhiver::game_parser + 0x20, ubc::lhiver::symbol_count + 3); break;
      case 14: f.image.denied = hero + 0x230; f.image.denied_size = 4; break;
      case 15: f.image.change_address = hero + 0x230; f.image.change_on_read = 2; break;
      case 16: f.image.change_address = ubc::station_abi::hero_pointer; f.image.change_on_read = 2; break;
      case 17: f.image.change_address = symbol(1683); f.image.change_target = symbol(1683) + 0x1c; f.image.change_on_read = 2; break;
      case 18: f.image.change_address = symbol(1549); f.image.change_target = symbol(1549) + 0x1c; f.image.change_on_read = 2; break;
      case 19: f.image.change_layout = true; break;
      case 20: f.image.change_address = table + 1549 * 4; f.image.change_on_read = 2; break;
      case 21: f.image.at(gd::native_ranges[0].address, 1)[0] ^= 1; break;
      case 22: f.image.at(gd::native_ranges[1].address, 1)[0] ^= 1; break;
      case 23: f.image.change_address = gd::native_ranges[0].address; f.image.change_on_read = 2; break;
      case 24: f.image.at(name_at(1549), 1)[0] ^= 1; break;
      case 25: f.image.at(name_at(1683), 1)[0] ^= 1; break;
      case 26: k.hero = gd::null_instance_int - gd::guild_offset;
        f.image.put(ubc::station_abi::hero_pointer, k.hero); f.image.put(symbol(1683) + 0x1c, k.hero); break;
      case 27: k.hero = symbol(1683); f.image.put(ubc::station_abi::hero_pointer, k.hero);
        f.image.put(symbol(1683) + 0x1c, k.hero); break;
      case 28: f.image.put(symbol(1683) + 0x18, hero); f.image.put(symbol(1683) + 0x1c, 0); break;
    }
    const auto observed = preview(f, k);
    if (observed.known()) std::fprintf(stderr, "guild negative mode=%u\n", mode);
    check(!observed.known(), "Null Dummy falsche Bindung Codeaenderung und wechselnde Werte bleiben unbekannt");
    if (mode >= 4 && mode <= 8) check(observed.failed_symbol == 1549, "defektes Guildmember wird konkret benannt");
    if (mode >= 9 && mode <= 11) check(observed.failed_symbol == 1683, "defekte HERO-Instanz wird konkret benannt");
    if (mode == 12) check(observed.failed_symbol == 1537, "defekte NPC-Klasse wird konkret benannt");
    if (mode == 15) check(observed.issue == Issue::changed, "spaeter geaenderter Gildenwert wird nicht als alte Aufnahme ausgegeben");
    if (mode == 26 || mode == 27) check(observed.issue == Issue::range || observed.issue == Issue::instance,
          "Dummyadresse und Symbol-als-Objekt werden vor Feldlesung abgewiesen");
  }
  { GuildFixture f; native.load(f.image); f.image.change_address = symbol(1679);
    f.image.change_target = symbol(1679) + 0x1c; f.image.change_on_read = 3;
    check(writer(f).issue == Issue::actor, "spaeterer SELF-Wechsel nach Gildenaufnahme sperrt Writer"); }
  { GuildFixture f; native.load(f.image); const auto first = preview(f);
    f.image.put(hero + 0x230, 7); const auto second = preview(f);
    check(first.known() && second.known() && first.guild == 2 && second.guild == 7 && !gd::same(first, second),
          "eigene Rueckgabekopien und expliziter Vergleich binden den konkreten Wert"); }
  { GuildFixture f; native.load(f.image);
    // Actual shared capture, state and actor readers. No manually invented
    // knowledge or contract booleans. Inventory and hand remain unknown.
    ubc::GrindRecipeCatalogueReader reader;
    const auto c = reader.capture(f.image.memory(), ubc::lhiver::game_parser, key(), true);
    const auto observed = preview(f); const auto active = writer(f);
    const auto p = ubc::grind_preview_knowledge(c, observed, key());
    const auto w = ubc::grind_writer_knowledge(c, active, key());
    check(p.guild_known && p.guild == 2 && p.talent_known && p.talent == 1 && p.skill_known && p.skill == 98 &&
          w.guild_known && w.guild == p.guild, "konkreter Gildenwert mit echten Capture-Statewerten verbunden");
    check(!c.recipes[0].material.maximum && !c.recipes[0].material.craftable && !c.recipes[0].material.ingredients_verified,
          "Wissensnaht gibt weder fehlende Inventar-/Handbelege noch Herstellung frei");
    auto changed = key(); ++changed.frame;
    check(!ubc::grind_preview_knowledge(c, observed, changed).guild_known, "Wissen bindet den aktuellen Frame");
    auto mixed = c; ++mixed.key.generation;
    check(!ubc::grind_writer_knowledge(mixed, active, key()).guild_known, "keine Vermischung verschiedener Generationen");
    mixed = c; mixed.diagnostic.state_after.skill = 1;
    check(!ubc::grind_preview_knowledge(mixed, observed, key()).guild_known, "abweichende Statekopie sperrt Wissen");
    f.image.put(symbol(1679) + 0x1c, hero + 4);
    const auto foreign_self = reader.capture(f.image.memory(), ubc::lhiver::game_parser, key(), true);
    check(ubc::grind_preview_knowledge(foreign_self, preview(f), key()).guild_known &&
          !ubc::grind_writer_knowledge(foreign_self, active, key()).guild_known,
          "Wissensnaht haelt physische Vorschau und aktiven Writer getrennt");
  }
}
}

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
int main(int argc, char** argv) {
  if (argc > 2 || (argc == 2 && std::strcmp(argv[1], "--native-stdin"))) return 2;
#ifdef _WIN32
  if (argc == 2 && _setmode(_fileno(stdin), _O_BINARY) == -1) return 2;
#endif
  NativeFacts native;
  if (argc == 2 && !native.read()) return 2;
  portable();
  if (argc == 2) real_native(native);
  std::printf("grind_guild_snapshot_tests: %u checks, %u failures\n", station_test::checks, station_test::failures);
  return station_test::failures ? 1 : 0;
}
