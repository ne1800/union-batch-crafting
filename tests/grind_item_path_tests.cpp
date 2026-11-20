// SPDX-License-Identifier: MIT
#include "ubc/grind_item_path.hpp"
#define UBC_GRIND_SNAPSHOT_FIXTURE_ONLY
#include "grind_recipe_snapshot_tests.cpp"
#define UBC_GRIND_FACTS_FIXTURE_ONLY
#include "grind_recipe_facts_tests.cpp"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
namespace path = ubc::grind_item_path;
namespace shape = ubc::grind_item_shape;
using PathIssue = path::Issue;
path::Reader input_reader{7};
path::Boundary boundary() { return {key(), 7, true}; }
struct NativePredicates {
  std::array<std::array<std::uint8_t, 1024>, 3> data{};
  bool read() {
    for (std::size_t i = 0; i < path::detail::predicate_ranges.size(); ++i) {
      const auto& r = path::detail::predicate_ranges[i];
      if (!std::cin.read(reinterpret_cast<char*>(data[i].data()), static_cast<std::streamsize>(r.length))) return false;
      ubc::Sha256 hash; hash.update(data[i].data(), r.length);
      if (hash.finish() != r.digest) return false;
    }
    return true; // The existing facts reader that follows requires EOF.
  }
  void load(Image& image) const {
    for (std::size_t i = 0; i < path::detail::predicate_ranges.size(); ++i) {
      const auto& r = path::detail::predicate_ranges[i]; image.block(r.address, r.length);
      std::memcpy(image.at(r.address, r.length), data[i].data(), r.length);
    }
  }
};
struct PathFixture : CaptureFixture {
  explicit PathFixture(const g::Descriptor& recipe = g::recipes[0], std::int32_t amount = 5, bool foreign = true) {
    stock(foreign ? std::vector<Stored>{{recipe.input, amount}, {19000, 11}} : std::vector<Stored>{{recipe.input, amount}}, "ITMISWORDBLADE:7");
    const auto* expected = shape::find(static_cast<std::uint32_t>(recipe.input));
    image.put(object(0) + 0x154, expected->mainflag); image.put(object(0) + 0x158, expected->native_flags);
    image.put(object(0) + 4, 2);
    if (foreign) image.put(object(1) + 4, 2);
    image.put(hero + 0x668, ubc::bow_item_path::inventory_vtable); image.put(hero + 0x66c, hero + 0x710);
    image.put(hero + 0x68c, 0); image.put(hero + 0x700, 0);
    image.block(0x8a2558, 4); image.put(0x8a2558, 0);
    image.block(ubc::station_abi::hero_pointer, 4); image.put(ubc::station_abi::hero_pointer, hero);
    image.block(ubc::hand_detail::item_vtable, 0x7c);
    image.put(ubc::hand_detail::item_vtable, ubc::hand_detail::item_class_method);
    image.put(ubc::hand_detail::item_vtable + 0x78, 0x711420);
  }
  path::Snapshot path(const ubc::GrindRecipeCatalogueCapture& c, std::uint32_t quantity = 1, std::size_t index = 0) {
    return input_reader.capture(image.memory(), c, g::recipes[index].function.symbol, quantity, boundary());
  }
};
void closed(const path::Snapshot& p, const ubc::GrindRecipeCatalogueCapture& c) {
  check(!path::Snapshot::native_effect_authorized && !p.input.native_effect_authorized && !g::runtime_authorized &&
        !c.recipes[0].material.craftable && !c.recipes[0].material.ingredients_verified &&
        !c.recipes[0].material.hand_known && !c.recipes[0].material.maximum,
        "Wegeplan erteilt weder Objekt-/Hand-End- noch Herstellungserlaubnis");
}
void portable_path() {
  PathFixture f; const auto c = capture(f); const auto before = f.image.reads;
  check(f.path(c).issue == PathIssue::binding, "synthetischer Capture ist kein Originalprofil");
  check(f.image.reads == before, "falsche Bindung erreicht keinen zweiten Speicherleser");
  for (const auto quantity : {0u, UINT32_MAX, 0x80000000u})
    check(f.path(c, quantity).issue == PathIssue::quantity, "Menge bleibt positiv int32-begrenzt");
  auto b = boundary(); b.stable_boundary_verified = false;
  check(input_reader.capture(f.image.memory(), c, g::recipes[0].function.symbol, 1, b).issue == PathIssue::boundary,
        "externer oder ungebundener Ruhepunkt gesperrt");
  b = boundary(); b.current_thread = 8;
  check(input_reader.capture(f.image.memory(), c, g::recipes[0].function.symbol, 1, b).issue == PathIssue::thread,
        "falscher Thread fuehrt keine Aufnahme aus");
  closed(f.path(c), c);
}
const ubc::GrindRecipeCatalogueCapture* nested_catalogue = nullptr;
void nested_path(Image& image) noexcept {
  const auto reads = image.reads;
  check(input_reader.capture(image.memory(), *nested_catalogue, g::recipes[0].function.symbol, 1, boundary()).issue == PathIssue::reentry &&
        image.reads == reads, "Wiedereintritt vor weiteren Lesungen gesperrt");
}
struct TransientHeader {
  Image& image;
  bool first = true;
  static bool read(void* raw, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& self = *static_cast<TransientHeader*>(raw);
    if (!Image::read(&self.image, address, output, size)) return false;
    if (self.first && address == hero + 0x708 && size == 40) {
      self.first = false;
      auto* out = static_cast<std::uint8_t*>(output);
      for (unsigned i = 0; i < 4; ++i) out[32 + i] = 0xff;
    }
    return true;
  }
};
void real_path(const Facts& local, const NativePredicates& native) {
  for (std::size_t index = 0; index < g::recipes.size(); ++index) {
    PathFixture f{g::recipes[index]}; load_original(f, local); native.load(f.image);
    const auto c = capture(f); const auto original = f.image.blocks;
    allocation_probe::watching = true;
    const auto split = f.path(c, 1, index); const auto whole = f.path(c, 5, index);
    allocation_probe::watching = false;
    check(split.captured() && split.removal == ubc::bow_item_path::Removal::split && split.amount_after == 4 &&
          split.refcount == 2 && split.listed_nodes == 2 && split.nodes_after == 2 && !split.existing_whole_short_path,
          "alle22 echten Eingangsformen erlauben Kategorie2-MultiSlot mit Teilstapelbedarf");
    check(whole.captured() && whole.removal == ubc::bow_item_path::Removal::whole && whole.amount_after == 0 &&
          whole.nodes_after == 1 && whole.existing_whole_short_path && whole.selected == 0,
          "genau eine Ganzentnahme mit real gelesenen Select/Refcount-Daten");
    check(split.input.callback == g::recipes[index].function.symbol && split.input.item.object == object(0) &&
          split.input.stock.total == stock(c, g::recipes[index].input).total &&
          split.input.hands.hands.left.object == hand_object && split.input.hands.left_tmp_level == 1,
          "Waffe, Kaltklingenhand, Callback und Originalform getrennt gebunden");
    check(f.image.blocks == original && !allocation_probe::count, "portabler Reader schreibt nichts und allokiert keinen Heap");
    closed(split, c);
    if (index >= 4 && index <= 6) check((split.input.item.flags & 0x1000u) != 0, "gesegnete Eingangswaffen behalten ITEM_MISSION");
  }
  for (unsigned mode = 0; mode < 36; ++mode) {
    PathFixture f; load_original(f, local); native.load(f.image); const auto c = capture(f);
    switch (mode) {
      case 0: f.image.put(object(0) + 4, 0); break;
      case 1: f.image.put(object(0) + 4, 0x80000000); break;
      case 2: f.image.put(object(1), ubc::hand_detail::item_vtable + 4); break;
      case 3: f.image.put(object(1) + 4, 0); break;
      case 4: f.image.put(object(0) + 0x32c, 4); break;
      case 5: f.image.put(object(0) + 0x330, 19001); break;
      case 6: f.image.put(object(0) + 0x158, shape::find(16483)->native_flags | 0x40000000u); break;
      case 7: f.image.put(hero + 0x9b8, 3); f.image.put(weapon_slot + 0x30, object(0)); break;
      case 8: f.image.put(hero + 0x9b8, 3); f.image.put(weapon_slot + 0x30, object(0)); text_at(f.image, weapon_name, "CUSTOM"); f.image.put(weapon_slot + 12, 6); break;
      case 9: f.image.put(hero + 0x968, object(0)); break;
      case 10: f.image.put(left_slot + 0x30, object(0)); break;
      case 11: f.image.put(symbol(1679) + 0x1c, hero + 4); break;
      case 12: f.image.put(ubc::station_abi::hero_pointer, hero + 4); break;
      case 13: f.image.put(symbol(12330) + 0x18, 13); break;
      case 14: f.image.put(object(0) + 0x154, 4); break;
      case 15: f.image.put(hero + 0x668, ubc::bow_item_path::inventory_vtable + 4); break;
      case 16: f.image.put(node(1) + 8, node(0)); break;
      case 17: f.image.put(node(1) + 4, object(0)); break;
      case 18: f.image.put(ubc::hand_detail::item_vtable + 0x78, 0x711421); break;
      case 19: f.image.at(0x7125a0, 1)[0] ^= 1; break;
      case 20: f.image.at(0x7126d0, 1)[0] ^= 1; break;
      case 21: f.image.at(0x711420, 1)[0] ^= 1; break;
      case 22: f.image.denied = object(0) + 4; f.image.denied_size = 4; break;
      case 23: f.image.denied = object(1) + 0x16c; f.image.denied_size = 32; break;
      case 24: f.image.put(hero + 0x728, ubc::max_packed_inventory_bytes + 1); break;
      case 25: f.image.put(hero + 0x9b8, 4); break;
      case 26: f.image.put(object(1) + 0x32c, 0xffffffffu); break;
      case 27: f.image.put(object(0) + 0x158, shape::find(16483)->native_flags | 0x1000u); break;
      case 28: f.image.at(code + g::recipes[0].function.byte_offset, 1)[0] ^= 1; break;
      case 29: f.image.put(node(0) + 4, 0xfffffff0u); break;
      case 30: f.image.denied = 0x7125a0; f.image.denied_size = 99; break;
      case 31: f.image.put(symbol(1683) + 0x1c, hero + 4); break;
      case 32: f.image.put(object(0) + 0x32c, 0); break;
      case 33: f.image.put(ubc::hand_detail::item_vtable, 0x711414); break;
      case 34: f.image.put(slots + 4, left_slot); break;
      case 35: f.image.put(hero + 0x70c, 0); break;
    }
    const auto p = f.path(c);
    if (p.captured()) std::fprintf(stderr, "path negative mode=%u\n", mode);
    check(!p.captured(), "Aenderungen nach Capture an Actor Objekt Code Liste oder Slots sind keine aktuelle Eingangsbindung");
    closed(p, c);
  }
  for (unsigned mode = 0; mode < 10; ++mode) {
    PathFixture f; load_original(f, local); native.load(f.image); const auto c = capture(f);
    switch (mode) {
      case 0: f.image.change_address = object(0) + 4; f.image.change_on_read = 2; break;
      case 1: f.image.change_address = object(0) + 0x158; f.image.change_on_read = 2; break;
      case 2: f.image.put(object(1) + 0x16c, 1); f.image.change_address = object(1) + 0x16c; f.image.change_on_read = 2; break;
      case 3: f.image.change_address = 0x7125a0; f.image.change_on_read = 3; break;
      case 4: f.image.change_address = hero + 0x668; f.image.change_on_read = 2; break;
      case 5: f.image.change_address = ubc::station_abi::hero_pointer; f.image.change_on_read = 2; break;
      case 6: f.image.change_address = left_slot; f.image.change_target = left_slot + 0x18; f.image.change_on_read = 2; break;
      case 7: f.image.change_address = hero + 0x708; f.image.change_target = hero + 0x70c; f.image.change_on_read = 4; break;
      case 8: f.image.change_address = object(1) + 4; f.image.change_on_read = 2; break;
      case 9: f.image.change_address = symbol(1679); f.image.change_target = symbol(1679) + 0x1c; f.image.change_on_read = 3; break;
    }
    check(!f.path(c).captured(), "Aenderungen innerhalb der zweiten Aufnahme bleiben gesperrt");
  }
  for (unsigned mode = 0; mode < 6; ++mode) {
    PathFixture f; load_original(f, local); native.load(f.image);
    switch (mode) {
      case 0: f.image.put(hero + 0x68c, 1); break;
      case 1: f.image.put(hero + 0x700, 1); break;
      case 2: f.image.put(0x8a2558, 1); break;
      case 3: f.image.put(object(0) + 4, 1); break;
      case 4: f.image.put(object(0) + 4, INT32_MAX); break;
      case 5: f.image.put(hero + 0x66c, hero + 0x714); break;
    }
    const auto c = capture(f); const auto p = f.path(c, 5);
    check(p.captured() && p.removal == ubc::bow_item_path::Removal::whole && !p.existing_whole_short_path,
          "unpassende Select-/Refcount-/Poolbedingung wird als fehlender Ganzstapelvorbeleg ausgewiesen");
    closed(p, c);
  }
  for (const auto& suffix : {std::string("_EXTRA"), std::string("")}) {
    PathFixture f; load_original(f, local); native.load(f.image);
    const auto* item = facts::item_for(static_cast<std::uint32_t>(g::recipes[0].input));
    const auto collision = suffix.empty() ? std::string("PRE_") + item->function->name : std::string(item->function->name) + suffix;
    text_at(f.image, pack, collision + ":5"); f.image.put(hero + 0x728, static_cast<std::uint32_t>(collision.size() + 2));
    const auto c = capture(f);
    check(ubc::bind_grind_input_object(c, key(), g::recipes[0].function.symbol).observed() && !f.path(c).captured(),
          "konkrete native Suchnamenskollision sperrt trotz eigenem eindeutigem Eingangsobjekt");
  }
  { PathFixture f{g::recipes[0], 5, false}; load_original(f, local); native.load(f.image); const auto c = capture(f);
    const auto p = f.path(c, 5);
    check(p.captured() && p.nodes_after == 0 && !p.existing_whole_short_path, "letzter Listknoten hat keinen behaupteten Select-Kurzpfad"); }
  { PathFixture f{g::recipes[0], INT32_MAX}; load_original(f, local); native.load(f.image); const auto c = capture(f);
    check(f.path(c, INT32_MAX).captured() && f.path(c, 1).amount_after == INT32_MAX - 1u, "int32-Mengengrenze ohne Ueberlauf"); }
  { PathFixture f; load_original(f, local); native.load(f.image);
    f.image.put(object(1) + 0x158, 0x40004002); f.image.put(hero + 0x9b8, 3); f.image.put(weapon_slot + 0x30, object(1));
    const auto c = capture(f);
    check(f.path(c).captured(), "andere aktive ausgeruestete Waffe wird nicht pauschal mit dem inaktiven Eingang verwechselt"); }
  { PathFixture f; load_original(f, local); native.load(f.image); const auto c = capture(f);
    f.item(object(2), g::recipes[0].input, 5); f.image.put(object(2) + 4, 2); f.image.put(node(0) + 4, object(2));
    check(!f.path(c).captured(), "identischer Symbol-/Mengenwert an neuem Objekt ersetzt nicht die aktuelle Eingangsbindung"); }
  { PathFixture f; load_original(f, local); native.load(f.image); const auto c = capture(f);
    TransientHeader h{f.image};
    check(input_reader.capture({&h, TransientHeader::read}, c, g::recipes[0].function.symbol, 1, boundary()).issue == PathIssue::inventory,
          "einmaliger ungueltiger Vorheader wird vor jeder Packkopie oder Fingerabdrucklaenge abgewiesen"); }
  { PathFixture f; load_original(f, local); native.load(f.image); const auto c = capture(f);
    check(f.path(c, 6).issue == PathIssue::quantity, "kein Clamp von Ueberbestellung");
    nested_catalogue = &c; f.image.on_read = nested_path;
    check(f.path(c).captured(), "aeussere Aufnahme nach abgewehrtem Reentry unveraendert");
    const auto first = f.path(c); f.image.put(object(1) + 0x16c, 7); const auto second = f.path(c);
    check(first.captured() && second.captured() && first.fingerprint != second.fingerprint,
          "Fingerabdruck bindet auch fremde gelesene Objekte; kein Summenvergleich");
    f.image.put(object(0) + 4, 3); (void)f.path(c);
    check(first.refcount == 2 && first.input.stock.total == 5, "Ergebnis bleibt eigene Kopie statt Workspace-View"); }
}
}

int main(int argc, char** argv) {
  if (argc > 2 || (argc == 2 && std::strcmp(argv[1], "--facts-stdin"))) return 2;
#ifdef _WIN32
  if (argc == 2 && _setmode(_fileno(stdin), _O_BINARY) == -1) return 2;
#endif
  try {
    portable_path();
    if (argc == 2) { NativePredicates native; if (!native.read()) return 2; const Facts local; real_path(local, native); }
    std::printf("grind_item_path_tests: %u checks, %u failures\n", station_test::checks, station_test::failures);
    return station_test::failures ? 1 : 0;
  } catch (const std::exception&) { input_error(); }
}
