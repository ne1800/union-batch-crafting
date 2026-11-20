// SPDX-License-Identifier: MIT
#include "ubc/stations/grind/grind_recipe_snapshot.hpp"
#include "grind_runtime_fixture.hpp"

namespace allocation_probe { bool watching = false; std::size_t count = 0; }
void* operator new(std::size_t size) {
  if (allocation_probe::watching) ++allocation_probe::count;
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
constexpr std::uint32_t hero = 0x2000, pack = 0x40000, slots = 0x110000;
constexpr std::uint32_t left_slot = 0x120000, right_slot = 0x120080, weapon_slot = 0x120100;
constexpr std::uint32_t left_name = 0x130000, right_name = 0x130050, weapon_name = 0x130100, hand_object = 0x150000;
constexpr std::uint32_t node(std::uint32_t i) { return 0x10000 + i * 16; }
constexpr std::uint32_t object(std::uint32_t i) { return 0x90000 + i * 0x400; }
struct Stored { std::int32_t symbol, amount; };
void text_at(Image& image, std::uint32_t address, const std::string& value) {
  image.block(address, value.size() + 1);
  std::memcpy(image.at(address, value.size() + 1), value.c_str(), value.size() + 1);
}
ubc::GrindRecipeKey key() { return {1, 2, 7, 0x8000, hero, 0x8800, 0x9000, 11}; }
struct CaptureFixture : Fixture {
  CaptureFixture() : Fixture(g::recipes[0]) {
    for (const auto& d : g::recipes) {
      const auto a = runtime::assemble(d); populate(a.profile());
    }
    for (const auto& i : facts::items) populate(runtime::item_profile(i));
    image.put(symbol(1683) + 0x1c, hero); image.put(symbol(1679) + 0x1c, hero);
    image.put(symbol(12330) + 0x18, 11); image.put(symbol(10502) + 0x18, 1);
    image.put(symbol(10501) + 0x18, 1); image.put(symbol(10505) + 0x18, 98);
    image.block(hero, 0xa00); image.put(hero + 0x230, 0xdeadbeef);
    image.block(slots, 12); image.block(left_slot, 0x38); image.block(right_slot, 0x38); image.block(weapon_slot, 0x38);
    text_at(image, left_name, "ZS_LEFTHAND"); text_at(image, right_name, "ZS_RIGHTHAND"); text_at(image, weapon_name, "ZS_SWORD");
    image.put(hero + 0x9b0, slots); image.put(hero + 0x9b4, 3); image.put(hero + 0x9b8, 2);
    image.put(slots, left_slot); image.put(slots + 4, right_slot); image.put(slots + 8, weapon_slot);
    image.put(left_slot + 8, left_name); image.put(left_slot + 12, 11);
    image.put(right_slot + 8, right_name); image.put(right_slot + 12, 12);
    image.put(weapon_slot + 8, weapon_name); image.put(weapon_slot + 12, 8);
    image.put(left_slot + 0x18, 1); image.put(left_slot + 0x30, hand_object); image.put(hero + 0x968, hand_object);
    item(hand_object, 16613, 1, 1, 0x200001);
    image.block(ubc::hand_detail::item_vtable, 4);
    image.put(ubc::hand_detail::item_vtable, ubc::hand_detail::item_class_method);
    image.block(ubc::hand_detail::item_class_method, ubc::hand_detail::item_class_bytes.size());
    std::memcpy(image.at(ubc::hand_detail::item_class_method, ubc::hand_detail::item_class_bytes.size()),
                ubc::hand_detail::item_class_bytes.data(), ubc::hand_detail::item_class_bytes.size());
    stock({{g::recipes[0].input, 5}}, "ITMISWORDBLADE:7");
  }
  void item(std::uint32_t p, std::int32_t id, std::int32_t amount, std::uint32_t mainflag = 2, std::uint32_t flags = 16386) {
    image.block(p, 0x338); image.put(p, ubc::hand_detail::item_vtable);
    image.put(p + 0x154, mainflag); image.put(p + 0x158, flags);
    image.put(p + 0x32c, static_cast<std::uint32_t>(amount)); image.put(p + 0x330, static_cast<std::uint32_t>(id));
  }
  void stock(const std::vector<Stored>& listed, const std::string& packed) {
    image.put(hero + 0x708, hero); image.put(hero + 0x70c, 1);
    image.put(hero + 0x718, listed.empty() ? 0 : node(0));
    image.put(hero + 0x724, packed.empty() ? 0 : pack); image.put(hero + 0x728, static_cast<std::uint32_t>(packed.size()));
    text_at(image, pack, packed);
    for (std::uint32_t i = 0; i < listed.size(); ++i) {
      image.block(node(i), 12); image.put(node(i) + 4, object(i));
      image.put(node(i) + 8, i + 1 < listed.size() ? node(i + 1) : 0);
      item(object(i), listed[i].symbol, listed[i].amount);
    }
  }
};
struct Probe {
  Image& image;
  unsigned guild_reads = 0;
  static bool read(void* raw, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& p = *static_cast<Probe*>(raw);
    if (std::uint64_t{address} + size > hero + 0x230 && address < hero + 0x234) ++p.guild_reads;
    return Image::read(&p.image, address, output, size);
  }
  ubc::ReadOnlyMemory memory() { return {this, read}; }
};
ubc::GrindRecipeCatalogueReader reader;
ubc::GrindRecipeCatalogueCapture capture(CaptureFixture& f) {
  return reader.capture(f.image.memory(), ubc::lhiver::game_parser, key(), true);
}
const ubc::RecipeInventoryItem& stock(const ubc::GrindRecipeCatalogueCapture& c, std::int32_t id) {
  for (const auto& s : c.stocks) if (s.symbol == id) return s;
  std::abort();
}
#ifndef UBC_GRIND_SNAPSHOT_FIXTURE_ONLY
void nested(Image& image) noexcept {
  const auto reads = image.reads;
  check(reader.capture(image.memory(), ubc::lhiver::game_parser, key(), true).diagnostic.issue == ubc::GrindCaptureIssue::reentry &&
        image.reads == reads, "Wiedereintritt fuehrt keinen zweiten Leser aus");
}
#endif
}

#ifndef UBC_GRIND_SNAPSHOT_FIXTURE_ONLY
int main() {
  for (std::size_t i = 0; i < g::recipes.size(); ++i) {
    const auto& descriptor = g::recipes[i];
    CaptureFixture f;
    const auto* output = facts::item_for(static_cast<std::uint32_t>(descriptor.output));
    f.stock({{descriptor.input, 5}, {19000, 11}}, std::string(output->function->name) + ":3,ITMISWORDBLADE:7");
    Probe probe{f.image};
    const auto c = reader.capture(probe.memory(), ubc::lhiver::game_parser, key(), true);
    const auto& d = c.diagnostic; const auto& s = c.recipes[i];
    check(d.issue == ubc::GrindCaptureIssue::none && d.target_count == 45 && d.inventory_unchanged &&
          d.item_fields_unchanged && d.item_class_verified && d.hands_unchanged && !d.hand_inventory_alias,
          "tatsaechliche begrenzte Listen-Pack-Slotleser stabil");
    check(d.actor_unchanged && d.state_unchanged && s.actor_matches && s.context_matches && s.learned,
          "bestehende State- und Actorleser verbunden");
    check(s.callback == descriptor.function.symbol && s.material.inventory_known &&
          s.material.rows[0].stored == 5 && s.material.rows[1].stored == 3 && s.material.rows[2].stored == 7,
          "alle22 Rezeptziele korrekt aus45 gemeinsam gelesenen Bestandszielen projiziert");
    check(s.binding_request.input.object == object(0) && s.binding_request.input.fields_known &&
          s.binding_request.input.exact_item_type && !s.binding_request.input.active_bit_seen &&
          !s.binding_request.input.slot_alias_seen && s.binding_request.input.symbol == descriptor.input,
          "einzige Eingangswaffe mit echten beobachteten Itemfeldern");
    check(s.binding_request.key.frame == 2 && s.binding_request.key.hero == hero &&
          s.binding_request.hands.hands.left.symbol == 16613 && s.binding_request.hands.left_tmp_level == 1,
          "konkreter Key Frame und Handwerte fuer spaeteren Binder");
    check(!s.material.maximum && !s.material.craftable && !s.material.evidence.stable_boundary_verified &&
          !s.contract && !d.profiles[i].valid && s.material.issue == ubc::GrindMaterialIssue::hand_contract,
          "beobachtete Felder und synthetische Koerper erfinden keinen Native- oder Originalhashbeleg");
    check(probe.guild_reads == 0 && f.image.inventory_reads >= 4 && f.image.hand_reads >= 2,
          "native Gilde nicht aus Parseroffset gelesen; echte Reader laufen");
    check(stock(c, descriptor.input).total == 5 && stock(c, 16613).total == 7, "Bestandskopien getrennt von Handklinge");
    check(c.listed_objects_known&&c.listed_object_count==d.listed_nodes&&c.listed_object_count==2&&
      c.listed_objects[0]==object(0)&&c.listed_objects[1]==object(1)&&
      ubc::grind_recipe_snapshot_detail::same_key(c.listed_objects_key,key()),
      "vollstaendige eigene Objektkopie bindet auch fremdes Item an aktuellen Key und Frame");
  }
  { CaptureFixture f; const auto* input = facts::item_for(static_cast<std::uint32_t>(g::recipes[0].input));
    f.stock({}, std::string(input->function->name) + ":5"); const auto c = capture(f);
    check(c.diagnostic.inventory_unchanged && c.recipes[0].material.rows[0].stored == 5 &&
          !c.recipes[0].binding_request.input.fields_known && !c.recipes[0].binding_request.input.exact_item_type &&
          !c.recipes[0].material.maximum, "Packbestand erzeugt keinen fiktiven nativen Waffenbeleg"); }
  { CaptureFixture f; f.image.put(object(0) + 0x158, 0x40004002); const auto c = capture(f);
    check(c.recipes[0].binding_request.input.active_bit_seen && !c.recipes[0].material.maximum,
          "aktive Waffenbits werden beobachtet, nicht freigegeben"); }
  { CaptureFixture f; f.image.put(hero + 0x9b8, 3); f.image.put(weapon_slot + 0x30, object(0));
    const auto c = capture(f); check(c.diagnostic.hands_unchanged && c.recipes[0].binding_request.input.slot_alias_seen,
          "Ausruestungsalias auch in drittem anders benannten Slot erkannt"); }
  { CaptureFixture f; f.stock({{g::recipes[0].input, 2}, {g::recipes[0].input, 3}}, ""); const auto c = capture(f);
    check(c.recipes[0].binding_request.input.listed_count == 2 && !c.recipes[0].binding_request.input.object &&
          !c.recipes[0].binding_request.input.mainflag && !c.recipes[0].binding_request.input.flags &&
          c.recipes[0].material.issue == ubc::GrindMaterialIssue::ambiguous_ingredient,
          "mehrere Waffenobjekte bleiben mehrdeutig"); }
  for (unsigned mutation = 0; mutation < 13; ++mutation) {
    CaptureFixture f;
    switch (mutation) {
      case 0: f.image.put(hero + 0x708, 0); break;
      case 1: f.image.put(node(0) + 8, node(0)); break;
      case 2: f.image.put(hero + 0x728, ubc::max_packed_inventory_bytes + 1); break;
      case 3: f.image.put(hero + 0x9b8, 4); break;
      case 4: f.image.put(slots + 4, left_slot); break;
      case 5: f.image.put(hero + 0x9b8, 3); f.image.put(weapon_slot + 0x30, hand_object); break;
      case 6: f.image.denied = object(0) + 0x154; f.image.denied_size = 4; break;
      case 7: f.image.change_address = object(0) + 0x158; f.image.change_on_read = 2; break;
      case 8: f.image.change_address = left_slot; f.image.change_target = left_slot + 0x18; f.image.change_on_read = 2; break;
      case 9: f.image.change_address = hero + 0x708; f.image.change_target = hero + 0x70c; f.image.change_on_read = 4; break;
      case 10: f.image.change_address = symbol(10505); f.image.change_target = symbol(10505) + 0x18; f.image.change_on_read = 3; break;
      case 11: f.image.change_address = symbol(1679); f.image.change_target = symbol(1679) + 0x1c; f.image.change_on_read = 3; break;
      case 12: f.image.change_layout = true; break;
    }
    const auto c = capture(f); const auto& d = c.diagnostic;
    check(!d.inventory_unchanged || !d.hands_unchanged || !d.state_unchanged || !d.actor_unchanged,
          "defekte oder waehrend Aufnahme veraenderte Speicherfelder bleiben sichtbar gesperrt");
    for (const auto& s : c.recipes) check(!s.material.maximum && !s.material.evidence.stable_boundary_verified,
          "kein Capturefehler eroeffnet Materialfreigabe");
  }
  { CaptureFixture f; f.image.put(node(0) + 4, hand_object); const auto c = capture(f);
    check(c.diagnostic.hand_inventory_alias && !c.recipes[0].material.inventory_known,
          "generierte Handklinge in Lagerliste verhindert getrennte Bestandsprojektion"); }
  { CaptureFixture f; f.image.put(object(0), ubc::hand_detail::item_vtable + 4); const auto c = capture(f);
    check(c.diagnostic.inventory_unchanged && !c.recipes[0].binding_request.input.exact_item_type,
          "lesbarer Fremdtyp wird nicht zu exaktem oCItem"); }
  { CaptureFixture f; f.image.at(ubc::hand_detail::item_class_method, 1)[0] ^= 1;
    check(!capture(f).diagnostic.item_class_verified, "veraenderte Klassenzuordnung bleibt unbelegt"); }
  { CaptureFixture f; f.image.put(left_slot + 0x18, 2); const auto c = capture(f);
    check(c.hands.left_tmp_level == 2 && !c.recipes[0].material.hand_known && !c.recipes[0].material.maximum,
          "beobachtetes tmp2 erzeugt keinen verifizierten Endvertrag"); }
  { CaptureFixture f; f.image.denied = hero + 0x230; f.image.denied_size = 4; const auto c = capture(f);
    check(c.diagnostic.inventory_unchanged && c.diagnostic.hands_unchanged && c.diagnostic.state_unchanged,
          "nicht belegtes natives Gildenfeld wird auch bei unlesbarem Wert nicht angefasst"); }
  for (unsigned mode = 0; mode < 11; ++mode) {
    CaptureFixture f; auto k = key(); auto parser = ubc::lhiver::game_parser;
    switch (mode) {
      case 0: k.generation = 0; break; case 1: k.frame = 0; break; case 2: k.thread = 0; break;
      case 3: k.world = 0; break; case 4: k.hero = 0; break; case 5: k.station = 0; break;
      case 6: k.manager = 0; break; case 7: k.production_kind = 13; break;
      case 8: parser = 0; break; case 9: parser += 4; break; case 10: break;
    }
    const auto reads = f.image.reads;
    const auto c = reader.capture(f.image.memory(), parser, k, mode != 10);
    check(c.diagnostic.issue == (mode == 10 ? ubc::GrindCaptureIssue::boundary : ubc::GrindCaptureIssue::context) &&
          f.image.reads == reads, "ohne richtigen Key Parser Ruhegrenze keinerlei Lesung");
    check(!c.listed_objects_known&&!c.listed_object_count&&!c.listed_objects_key.frame,
      "abgelehnte Aufnahme erfindet keine bekannte Objektliste");
  }
  { CaptureFixture f; f.image.put(symbol(10502) + 0x18, 0); const auto c = capture(f);
    check(c.recipes[0].state_known && !c.recipes[0].context_matches && c.recipes[0].learned,
          "Initialisierung und Wissen bleiben getrennt"); }
  { CaptureFixture f; f.image.put(symbol(12330) + 0x18, 13);
    check(!capture(f).recipes[0].context_matches, "Produktionsglobal widerspricht Schluessel"); }
  { CaptureFixture f; const auto first = capture(f); f.stock({}, ""); const auto second = capture(f);
    check(first.recipes[0].material.rows[0].stored == 5 && second.recipes[0].material.rows[0].stored == 0 &&
          first.recipes[0].binding_request.input.object == object(0), "spaetere Readerverwendung veraendert keine Rueckgabewerte"); }
  { CaptureFixture f;const auto first=capture(f);auto changed_key=key();++changed_key.frame;
    f.stock({},"");const auto second=reader.capture(f.image.memory(),ubc::lhiver::game_parser,changed_key,true);
    check(first.listed_objects_known&&first.listed_object_count==1&&first.listed_objects[0]==object(0)&&
      first.listed_objects_key.frame==2&&second.listed_objects_known&&!second.listed_object_count&&
      second.listed_objects_key.frame==3,"spaetere Inventar- und Frameaufnahme veraendert alte Objektkopie nicht");}
  { CaptureFixture f; f.image.on_read = nested; check(capture(f).diagnostic.inventory_unchanged, "aeussere Aufnahme nach Wiedereintritt intakt"); }
  { CaptureFixture f; allocation_probe::watching = true; const auto c = capture(f); allocation_probe::watching = false;
    check(c.diagnostic.inventory_unchanged && !allocation_probe::count, "Capture allokiert keinen Heap"); }
  std::printf("grind_recipe_snapshot_tests: %u checks, %u failures\n", station_test::checks, station_test::failures);
  return station_test::failures ? 1 : 0;
}
#endif
