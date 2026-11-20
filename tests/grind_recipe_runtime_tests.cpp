// SPDX-License-Identifier: MIT
#include "grind_runtime_fixture.hpp"

int main() {
  using namespace grind_test;
  for (const auto& descriptor : g::recipes) {
    Fixture f(descriptor);
    check(f.assembly.valid() && f.assembly.profile().function_count == 15 &&
          f.assembly.profile().binding_count == 51, "eng gebundenes Rezeptprofil");
    auto copied = f.assembly;
    check(copied.profile().functions != f.assembly.profile().functions &&
          copied.profile().functions[14].symbol == descriptor.function.symbol, "Kopie besitzt eigene Funktionsliste");
    f.assembly = {}; f.assembly = copied;
    check(f.synthetic_verify().matches, "synthetische Layout-Metadaten positiv");
    check(!runtime::verify(f.image.memory(), ubc::lhiver::game_parser, f.assembly).matches,
          "Testmuster erhaelt keinen Originalhashbeleg");
    for (const auto id : {descriptor.input, descriptor.output, g::lifecycle_item}) {
      const auto* item = facts::item_for(static_cast<std::uint32_t>(id));
      check(item && ubc::runtime_script::detail::valid_profile(runtime::item_profile(*item)),
            "separate begrenzte Waffen- und Klingenprofile");
      const auto fake = *item;
      check(!ubc::runtime_script::detail::valid_profile(runtime::item_profile(fake)), "keine fremde Itemprofilkopie");
    }
    auto value = runtime::state(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
    check(value.read_valid && value.learned() && value.station_postcondition() && value.skill == 98,
          "Wissen Skill und Eintrittspostbedingung getrennt");
    for (const auto talent : {0, 2, -1, INT32_MAX}) {
      f.image.put(symbol(10501) + 0x18, static_cast<std::uint32_t>(talent));
      const auto unknown = runtime::state(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
      check(unknown.read_valid && !unknown.learned() && unknown.station_postcondition(), "nur Talent gleich1 gelernt");
    }
    f.image.put(symbol(10501) + 0x18, 1);
    for (const auto first : {0, 1, 2, -1, INT32_MAX}) {
      f.image.put(symbol(10502) + 0x18, static_cast<std::uint32_t>(first));
      const auto original = runtime::state(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
      check(original.read_valid && original.learned() && original.station_postcondition() == (first != 0),
            "keine erfundene Erstinitialisierung oder muss1-Regel");
    }
    f.image.put(symbol(10502) + 0x18, 1);
    for (const auto skill : {INT32_MIN, -10, 0, 99, 100, 101, INT32_MAX}) {
      f.image.put(symbol(10505) + 0x18, static_cast<std::uint32_t>(skill));
      const auto original = runtime::state(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
      check(original.read_valid && original.skill == skill && original.learned(), "Skill nicht als Lernen oder Initialisierung umdeuten");
    }
    f.image.put(symbol(12330) + 0x18, 13);
    value = runtime::state(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
    check(value.learned() && !value.station_postcondition(), "Kuehlung ist keine Schleifstation");
    check(!runtime::state(f.image.memory(), 1, f.assembly).read_valid, "fremder Parser gesperrt");
    for (const auto id : {12330u, 10502u, 10501u, 10505u}) for (unsigned mutation = 0; mutation < 7; ++mutation) {
      Fixture bad(descriptor);
      switch (mutation) {
        case 0: bad.image.put(symbol(id) + 0x20, station_test::metadata(1,2,1)); break;
        case 1: bad.image.put(symbol(id) + 0x20, station_test::metadata(2,2,0)); break;
        case 2: bad.image.put(symbol(id) + 0x38, symbol(1537)); break;
        case 3: bad.image.at(name_at(id),1)[0] = 'X'; break;
        case 4: bad.image.denied = symbol(id) + 0x18; bad.image.denied_size = 4; break;
        case 5: bad.image.change_address = symbol(id); bad.image.change_target = symbol(id) + 0x18; bad.image.change_on_read = 2; break;
        case 6: bad.image.change_layout = true; break;
      }
      check(!runtime::state(bad.image.memory(), ubc::lhiver::game_parser, bad.assembly).read_valid,
            "falsche oder wechselnde globale Metadaten gesperrt");
    }
    Fixture actor(descriptor);
    check(runtime::actor(actor.image.memory(), ubc::lhiver::game_parser, actor.assembly,0x2000).matches,
          "gebundene SELF-HERO-Rohinstanzen");
    actor.image.put(symbol(1679) + 0x1c, 0x3000);
    check(!runtime::actor(actor.image.memory(), ubc::lhiver::game_parser, actor.assembly,0x2000).matches,
          "fremder SELF");
    Fixture changed(descriptor);
    changed.image.change_address = symbol(1683); changed.image.change_target = symbol(1683) + 0x1c; changed.image.change_on_read = 2;
    check(!runtime::actor(changed.image.memory(), ubc::lhiver::game_parser, changed.assembly,0x2000).read_valid,
          "Actorwechsel zwischen Lesungen");
    auto forged = descriptor; forged.input = 16604;
    check(!runtime::assemble(forged).valid(), "keine Flasche statt Waffeninput");
    check(!runtime::verify_recipe(f.image.memory(), ubc::lhiver::game_parser, forged).valid,
          "fremder Deskriptor kann nicht verifizieren");
    check(f.image.inventory_reads == 0 && f.image.hand_reads == 0, "kein erfundener Inventar- oder Handreader");
  }
  check(!g::runtime_authorized && !facts::item_for(16604), "keine pauschale Native- oder Flaschenfreigabe");
  std::printf("grind_recipe_runtime_tests: %u checks, %u failures\n", station_test::checks, station_test::failures);
  return station_test::failures ? 1 : 0;
}
