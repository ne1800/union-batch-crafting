// SPDX-License-Identifier: MIT
// Same bounded readers and RAM fixtures as capture and facts. Original
// data can arrive only through an explicit private stdin pipe.
#include "ubc/stations/grind/grind_item_binding.hpp"
#define UBC_GRIND_SNAPSHOT_FIXTURE_ONLY
#include "grind_recipe_snapshot_tests.cpp"
#define UBC_GRIND_FACTS_FIXTURE_ONLY
#include "grind_recipe_facts_tests.cpp"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
using Issue = ubc::GrindItemBindingIssue;
namespace shape = ubc::grind_item_shape;

void static_shapes() {
  unsigned mission = 0, swords = 0, twohand = 0, axes = 0, cold = 0;
  check(shape::items.size() == facts::items.size(),
        "genau45 Formreferenzen fuer45 bereits vorhandene Profile");
  for (const auto& s : shape::items) {
    check(shape::bound_to_profile(s), "Form ist an kanonischen Konstruktorvollhash gebunden");
    auto copied = s;
    check(!shape::bound_to_profile(copied), "fremde Formdeskriptoren werden nicht zugelassen");
    check(shape::find(s.symbol) == &s && s.native_flags == (s.mainflag | s.script_flags),
          "eindeutige kanonische Sollform");
    if (s.symbol >= 16532 && s.symbol <= 16537) {
      ++mission;
      check(s.mainflag == 2 && s.script_flags == 0x5000 && s.native_flags == 0x5002,
            "alle sechs Segenswaffen behalten ITEM_MISSION");
    } else if (s.script_flags == 0x4000)
      ++swords;
    else if (s.script_flags == 0x10000)
      ++twohand;
    else if (s.script_flags == 0x8000)
      ++axes;
    else if (s.symbol == 16613 && s.mainflag == 1 && s.native_flags == 0x200001)
      ++cold;
  }
  check(mission == 6 && swords == 16 && twohand == 20 && axes == 2 && cold == 1,
        "belegte Formgruppen ohne Bogenuebertragung");
  check(!shape::find(0) && !shape::find(16599) && !shape::find(UINT32_MAX),
        "kein unbekanntes oder Bogen-Symbol");
  check(!ubc::GrindInputObjectBinding{}.observed() &&
            !ubc::GrindInputObjectBinding::native_effect_authorized && !g::runtime_authorized,
        "leerer Befund und native Herstellung bleiben geschlossen");
}

void put_stock(CaptureFixture& f, const g::Descriptor& d, std::int32_t amount = 5) {
  f.stock({{d.input, amount}}, "ITMISWORDBLADE:7");
  const auto* s = shape::find(static_cast<std::uint32_t>(d.input));
  f.image.put(object(0) + 0x154, s->mainflag);
  f.image.put(object(0) + 0x158, s->native_flags);
}

ubc::GrindInputObjectBinding bind(const ubc::GrindRecipeCatalogueCapture& c,
                                  std::size_t index = 0) {
  return ubc::bind_grind_input_object(c, key(), g::recipes[index].function.symbol);
}

void still_closed(const ubc::GrindRecipeCatalogueCapture& c, std::size_t index) {
  const auto& s = c.recipes[index];
  check(!s.material.ingredients_verified && !s.material.hand_known && !s.material.knowledge_known &&
            !s.material.evidence.guild_known && !s.material.craftable && !s.material.maximum &&
            !s.material.numerical.valid,
        "Objektbefund erfindet weder Gilde Handvertrag noch Native-/Herstellungsfreigabe");
}

void portable() {
  CaptureFixture f;
  put_stock(f, g::recipes[0]);
  const auto c = capture(f);
  check(c.diagnostic.inventory_unchanged && c.diagnostic.hands_unchanged &&
            stock(c, g::recipes[0].input).total == 5,
        "echte portable Capture-Reader lesen synthetische Objekte und Slots");
  check(bind(c).issue == Issue::profile && !bind(c).observed(),
        "synthetische Koerper erfinden keinen kanonischen Vertrag");
  for (unsigned field = 0; field < 8; ++field) {
    auto current = key();
    switch (field) {
    case 0:
      ++current.generation;
      break;
    case 1:
      ++current.frame;
      break;
    case 2:
      ++current.thread;
      break;
    case 3:
      ++current.world;
      break;
    case 4:
      ++current.hero;
      break;
    case 5:
      ++current.station;
      break;
    case 6:
      ++current.manager;
      break;
    case 7:
      current.production_kind = 13;
      break;
    }
    check(ubc::bind_grind_input_object(c, current, g::recipes[0].function.symbol).issue ==
              Issue::key,
          "jedes Feld des aktuellen Schluessels muss exakt uebereinstimmen");
  }
  check(ubc::bind_grind_input_object(c, key(), 20987).issue == Issue::recipe,
        "fremder Callback gesperrt");
  check(!bind(ubc::GrindRecipeCatalogueCapture{}).observed(), "leere Aufnahme gesperrt");
  still_closed(c, 0);
}

void real_profiles(const Facts& local) {
  for (std::size_t index = 0; index < g::recipes.size(); ++index) {
    const auto& d = g::recipes[index];
    CaptureFixture f;
    load_original(f, local);
    put_stock(f, d);
    Probe probe{f.image};
    const auto c = reader.capture(probe.memory(), ubc::lhiver::game_parser, key(), true);
    const auto before = f.image.reads;
    allocation_probe::watching = true;
    const auto result = bind(c, index);
    allocation_probe::watching = false;
    check(result.observed() && result.issue == Issue::none,
          "alle22 Originalprofile geben nur eindeutigen Objektbefund");
    check(result.stock.symbol == d.input && result.stock.listed == 5 &&
              result.item.object == object(0) &&
              result.item.flags == shape::find(static_cast<std::uint32_t>(d.input))->native_flags &&
              result.constructor_sha256 ==
                  facts::item_for(static_cast<std::uint32_t>(d.input))->function->body_sha256,
          "Ergebnis besitzt das wirkliche Eingangsobjekt und seinen Konstruktorhash");
    check(result.key.frame == key().frame && result.callback == d.function.symbol &&
              result.hands.hands.left.symbol == 16613 && result.hands.left_tmp_level == 1,
          "Schluessel Callback und beobachtete Handwerte bleiben gebunden");
    check(
        !allocation_probe::count && f.image.reads == before && probe.guild_reads == 0,
        "Binder liest keinen Speicher und allokiert nicht; Capture liest kein unbelegtes Gildenfeld");
    still_closed(c, index);
    f.stock({}, "");
    (void)capture(f);
    check(result.observed() && result.stock.listed == 5 && result.item.object == object(0),
          "spaetere Readerverwendung veraendert keine Bindungskopie");
    // The actual recipe verifier covers input, output and
    // 16613. No table comparison can replace this full hash.
    for (const auto item_id : {d.input, d.output, g::lifecycle_item}) {
      const auto* item = facts::item_for(static_cast<std::uint32_t>(item_id));
      const auto* s = shape::find(static_cast<std::uint32_t>(item_id));
      const auto& raw = local.functions.at(static_cast<std::uint32_t>(item_id));
      ubc::Sha256 hash;
      hash.update(raw.data(), raw.size());
      check(s && s->constructor_sha256 == hash.finish() &&
                s->constructor_sha256 == item->function->body_sha256,
            "jede referenzierte Form gegen Originalkoerper in RAM");
    }
  }

  // Negative memory cases always go through the same existing readers.
  for (unsigned mode = 0; mode < 31; ++mode) {
    CaptureFixture f;
    load_original(f, local);
    put_stock(f, g::recipes[0]);
    const auto* expected = shape::find(static_cast<std::uint32_t>(g::recipes[0].input));
    switch (mode) {
    case 0:
      f.image.put(object(0) + 0x158, expected->native_flags | 0x40000000u);
      break;
    case 1:
      f.image.put(hero + 0x9b8, 3);
      f.image.put(weapon_slot + 0x30, object(0));
      break;
    case 2:
      f.image.put(object(0), ubc::hand_detail::item_vtable + 4);
      break;
    case 3:
      f.image.put(object(0) + 0x154, 4);
      break;
    case 4:
      f.image.put(object(0) + 0x158, expected->native_flags | 0x1000);
      break;
    case 5:
      f.stock(
          {},
          std::string(
              facts::item_for(static_cast<std::uint32_t>(g::recipes[0].input))->function->name) +
              ":5");
      break;
    case 6:
      f.stock({{g::recipes[0].input, 2}, {g::recipes[0].input, 3}}, "");
      break;
    case 7:
      f.stock(
          {{g::recipes[0].input, 5}},
          std::string(
              facts::item_for(static_cast<std::uint32_t>(g::recipes[0].input))->function->name) +
              ":1");
      break;
    case 8:
      f.image.put(hero + 0x9b8, 4);
      break;
    case 9:
      f.image.put(slots + 4, left_slot);
      break;
    case 10:
      f.image.put(hero + 0x708, 0);
      break;
    case 11:
      f.image.put(node(0) + 8, node(0));
      break;
    case 12:
      f.image.put(hero + 0x728, ubc::max_packed_inventory_bytes + 1);
      break;
    case 13:
      f.image.denied = object(0) + 0x154;
      f.image.denied_size = 4;
      break;
    case 14:
      f.image.change_address = object(0) + 0x158;
      f.image.change_on_read = 2;
      break;
    case 15:
      f.image.change_address = left_slot;
      f.image.change_target = left_slot + 0x18;
      f.image.change_on_read = 2;
      break;
    case 16:
      f.image.change_address = hero + 0x708;
      f.image.change_target = hero + 0x70c;
      f.image.change_on_read = 4;
      break;
    case 17:
      f.image.change_address = symbol(10505);
      f.image.change_target = symbol(10505) + 0x18;
      f.image.change_on_read = 3;
      break;
    case 18:
      f.image.change_address = symbol(1679);
      f.image.change_target = symbol(1679) + 0x1c;
      f.image.change_on_read = 3;
      break;
    case 19:
      f.image.put(symbol(12330) + 0x18, 13);
      break;
    case 20:
      f.image.put(symbol(1683) + 0x1c, hero + 4);
      break;
    case 21:
      f.image.put(hero + 0x968, object(0));
      break;
    case 22:
      f.image.put(object(0) + 0x32c, 0);
      break;
    case 23:
      f.image.at(code + facts::item_for(static_cast<std::uint32_t>(g::recipes[0].input))
                            ->function->byte_offset,
                 1)[0] ^= 1;
      break;
    case 24:
      f.image.at(code + facts::item_for(static_cast<std::uint32_t>(g::recipes[0].output))
                            ->function->byte_offset,
                 1)[0] ^= 1;
      break;
    case 25:
      f.image.put(symbol(1591) + 0x1c, 0x154);
      break;
    case 26:
      f.image.at(ubc::hand_detail::item_class_method, 1)[0] ^= 1;
      break;
    case 27:
      f.image.put(hero + 0x718, 0);
      break;
    case 28:
      f.image.put(hero + 0x9b8, 1);
      break;
    case 29:
      f.image.put(left_slot + 0x30, object(0));
      break;
    case 30:
      f.image.put(symbol(1679) + 0x1c, hero + 4);
      break;
    }
    const auto c = capture(f);
    const auto result = bind(c);
    if (result.observed())
      std::fprintf(stderr, "negative mode=%u\n", mode);
    check(!result.observed(),
          "instabile aktive ausgeruestete mehrdeutige oder fremde Objekte gesperrt");
    if (mode == 0)
      check(result.issue == Issue::active, "Activebit hat eigenen negativen Befund");
    if (mode == 1)
      check(result.issue == Issue::slot_alias, "dritter Ausruestungsslot wird wirklich verglichen");
    if (mode == 3 || mode == 4)
      check(result.issue == Issue::shape, "falsche Form wird nicht normalisiert");
    if (mode == 5 || mode == 6 || mode == 7)
      check(result.issue == Issue::representation,
            "Pack und Mehrdeutigkeit liefern keinen Objektbeleg");
    if (mode == 30)
      check(result.issue == Issue::context,
            "Writer-Vorbefund erfordert frisches SELF gleich HERO; kein Vorschauvertrag");
    still_closed(c, 0);
  }
  for (std::size_t index = 4; index <= 6; ++index) {
    CaptureFixture f;
    load_original(f, local);
    put_stock(f, g::recipes[index]);
    f.image.put(object(0) + 0x158, 0x4002);
    check(bind(capture(f), index).issue == Issue::shape,
          "fehlendes ITEM_MISSION sperrt jede Segens-Eingangswaffe");
  }
  {
    CaptureFixture f;
    load_original(f, local);
    put_stock(f, g::recipes[0]);
    f.image.put(left_slot + 0x18, 2);
    const auto c = capture(f);
    const auto result = bind(c);
    check(result.observed() && result.hands.left_tmp_level == 2, "tmp2 wird lediglich kopiert");
    still_closed(c, 0);
  }
  {
    CaptureFixture f;
    load_original(f, local);
    put_stock(f, g::recipes[0], INT32_MAX);
    const auto c = capture(f);
    check(bind(c).observed() && bind(c).stock.listed == INT32_MAX,
          "INT32_MAX bleibt lesender Bestand ohne arithmetischen Ueberlauf");
    still_closed(c, 0);
  }

  // Copies mixed together after capture cannot establish a binding.
  CaptureFixture f;
  load_original(f, local);
  put_stock(f, g::recipes[0]);
  const auto original = capture(f);
  for (unsigned mode = 0; mode < 15; ++mode) {
    auto c = original;
    auto& state = c.recipes[0];
    auto& item = *std::find_if(c.items.begin(), c.items.end(),
                               [](const auto& s) { return s.symbol == g::recipes[0].input; });
    auto& input_stock = *std::find_if(c.stocks.begin(), c.stocks.end(), [](const auto& s) {
      return s.symbol == g::recipes[0].input;
    });
    switch (mode) {
    case 0:
      ++state.binding_request.key.frame;
      break;
    case 1:
      ++state.binding_request.callback;
      break;
    case 2:
      ++state.binding_request.input.object;
      break;
    case 3:
      ++state.binding_request.hands.left_tmp_level;
      break;
    case 4:
      ++state.material.rows[0].stored;
      break;
    case 5:
      c.items.back() = item;
      break;
    case 6:
      c.stocks.back() = input_stock;
      break;
    case 7:
      input_stock.total = INT32_MIN;
      break;
    case 8:
      input_stock.has_active_marker = true;
      break;
    case 9:
      c.diagnostic.item_fields_unchanged = false;
      break;
    case 10:
      c.diagnostic.hands_unchanged = false;
      break;
    case 11:
      c.diagnostic.profiles[0].valid = false;
      break;
    case 12:
      c.diagnostic.actor_after.self += 4;
      break;
    case 13:
      c.diagnostic.state_after.production = 13;
      break;
    case 14:
      c.diagnostic.listed_nodes = ubc::max_packed_inventory_nodes + 1;
      break;
    }
    check(!bind(c).observed(), "fremde Request-/Material-/Kontextkopien werden nicht akzeptiert");
  }
}
}

int main(int argc, char** argv) {
  if (argc > 2 || (argc == 2 && std::strcmp(argv[1], "--facts-stdin")))
    return 2;
#ifdef _WIN32
  if (argc == 2 && _setmode(_fileno(stdin), _O_BINARY) == -1)
    return 2;
#endif
  try {
    static_shapes();
    portable();
    if (argc == 2) {
      const Facts local;
      real_profiles(local);
    }
    std::printf("grind_item_binding_tests: %u checks, %u failures\n", station_test::checks,
                station_test::failures);
    return station_test::failures ? 1 : 0;
  } catch (const std::exception&) {
    input_error();
  }
}
