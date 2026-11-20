// SPDX-License-Identifier: MIT
#include "ubc/grind_recipe_material.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdio>

namespace {
namespace g = ubc::grind_recipe;
using Issue = ubc::GrindMaterialIssue;
unsigned checks = 0, failures = 0;
void check(bool value, const char* why) {
  ++checks;
  if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", why); }
}
struct Fixture {
  const g::Descriptor& recipe;
  std::array<ubc::RecipeInventoryItem, 3> items{};
  std::array<std::uint32_t, 4> objects{{0x1000, 0x2000, 0x3000, 0x4000}};
  ubc::RecipeInventory inventory{};
  ubc::StationRecipeHands hands{};
  ubc::GrindMaterialKnowledge knowledge{};
  ubc::GrindMaterialProofs proofs{};
  explicit Fixture(const g::Descriptor& d = g::recipes[0]) : recipe(d) {
    items = {{{d.input, 5, 0, 5, 1, 0}, {d.output, 3, 0, 3, 1, 0},
              {g::lifecycle_item, 7, 0, 7, 1, 0}}};
    inventory.header_known = inventory.listed_known = inventory.packed_known = true;
    inventory.storage_totals_known = inventory.stable_boundary_verified = inventory.snapshot_unchanged = true;
    inventory.listed_nodes = 4;
    inventory.items = {items.data(), items.size()};
    inventory.listed_objects = {objects.data(), objects.size()};
    hands.hands.left = {1, g::lifecycle_item, 1, 0x5000};
    hands.hands.right = {0, -1, -1, 0};
    hands.left_slot_known = hands.right_slot_known = hands.interact_item_known = true;
    hands.left_tmp_level = 1;
    hands.interact_item = 0x5000;
    knowledge.talent_known = knowledge.guild_known = knowledge.skill_known = true;
    knowledge.talent = 1; knowledge.guild = d.guilds[0]; knowledge.skill = 98;
    proofs.stable_boundary_verified = proofs.station_hand_end_verified = proofs.input_objects_verified = true;
    proofs.verified_left_tmp_level = 1;
  }
  Fixture(const Fixture&) = delete;
  Fixture& operator=(const Fixture&) = delete;
  ubc::GrindMaterial get(std::uint32_t quantity = 2) const {
    return ubc::grind_recipe_material(recipe, inventory, hands, knowledge, quantity, proofs);
  }
  void packed_input(std::int32_t amount) {
    items[0] = {recipe.input, 0, amount, amount, 0, amount ? 1u : 0u};
    inventory.packed_entries = amount ? 1u : 0u;
  }
};

void all_recipes() {
  check(g::recipes.size() == 22 && !g::runtime_authorized, "keine neue Laufzeitfreigabe");
  for (const auto& descriptor : g::recipes) {
    for (const auto guild : descriptor.guilds) {
      for (const auto quantity : {0u, 1u, 2u, 5u}) {
        Fixture f(descriptor); f.knowledge.guild = guild;
        const auto value = f.get(quantity);
        check(value.known() && value.issue == Issue::none && value.maximum == 5, "alle 22 Materialprofile");
        check(value.quantity == quantity && value.numerical.valid == (quantity != 0) &&
              value.craftable == (quantity != 0), "N0 ist nur Vorschau");
        check(value.learned && value.guild_allowed && value.knowledge_known, "beide Originalgilden");
        check(value.evidence.callback == descriptor.function.symbol && value.evidence.stable_boundary_verified,
              "Evidence nur nach vollstaendiger Projektion");
        check(value.rows[0].symbol == descriptor.input && value.rows[1].symbol == descriptor.output &&
              value.rows[2].symbol == g::lifecycle_item, "symbolgebundene kanonische Reihen");
        check(value.rows[0].available == 5 && value.rows[0].required == quantity &&
              value.rows[0].remaining == 5 - static_cast<std::int64_t>(quantity), "nur gelagerte Waffen werden verbraucht");
        check(value.rows[1].produced == quantity && value.rows[1].remaining == 3 + quantity, "Waffentausch");
        check(value.rows[2].held == 1 && value.rows[2].stored == 7 && value.rows[2].available == 7 &&
              value.rows[2].reserved == 0 && value.rows[2].required == 0 && value.rows[2].produced == 0 &&
              value.rows[2].remaining == 7, "Handklinge wird weder addiert noch als Lagerreserve abgezogen");
        if (quantity) {
          check(value.numerical.after_callback[2].amount == 8 && value.numerical.after_end[2].amount == 7,
                "feste Einzelkompensation und Endabzug bleiben im bestehenden Plan");
          check(value.numerical.skill_after == (quantity == 1 ? 99 : 100), "vorhandener Skillplan unveraendert");
        }
        const auto saved = value;
        f.items[0].listed = f.items[0].total = 0;
        f.hands.hands.left = {};
        check(saved.evidence.stocks[0].amount == 5 && saved.rows[2].held == 1,
              "Ergebnis enthaelt eigene Werte statt Inventarviews");
        if (quantity) check(saved.numerical.body().operands == saved.numerical.operands.data(),
                            "kopierter Mengenplan behaelt eigene Operanden");
      }
    }
    Fixture packed(descriptor); packed.packed_input(5);
    check(packed.get().craftable && packed.get().maximum == 5, "gepackte eindeutige Waffen erlaubt");
    std::swap(packed.items[0], packed.items[2]);
    check(packed.get().craftable && packed.get().rows[0].stored == 5, "keine Abhaengigkeit von Targetreihenfolge");
    Fixture no_cold(descriptor); no_cold.items[2].listed = no_cold.items[2].total = 0;
    check(no_cold.get().craftable && no_cold.get().rows[2].available == 0 &&
          no_cold.get().numerical.after_callback[2].amount == 1 && no_cold.get().numerical.after_end[2].amount == 0,
          "kein erfundener Kaltklingen-Mindestbestand");
  }
}

void inventory_failures() {
  for (unsigned mode = 0; mode < 26; ++mode) {
    Fixture f;
    switch (mode) {
      case 0: f.inventory.header_known = false; break;
      case 1: f.inventory.listed_known = false; break;
      case 2: f.inventory.packed_known = false; break;
      case 3: f.inventory.storage_totals_known = false; break;
      case 4: f.inventory.stable_boundary_verified = false; break;
      case 5: f.inventory.snapshot_unchanged = false; break;
      case 6: f.inventory.issue = ubc::PackedInventoryIssue::changed_during_read; break;
      case 7: f.inventory.listed_nodes = ubc::max_packed_inventory_nodes + 1; break;
      case 8: f.inventory.packed_entries = ubc::max_packed_inventory_entries + 1; break;
      case 9: f.inventory.items.data = nullptr; break;
      case 10: f.inventory.items.count = 2; break;
      case 11: f.inventory.items.count = 4; break;
      case 12: f.items[1].symbol = f.recipe.input; break;
      case 13: f.items[2].symbol = 16612; break;
      case 14: f.items[0].listed = -1; break;
      case 15: f.items[0].packed = -1; break;
      case 16: f.items[0].total = -1; break;
      case 17: f.items[0].total = 4; break;
      case 18: f.items[0].listed_occurrences = 0; break;
      case 19: f.items[0].packed_occurrences = 1; f.inventory.packed_entries = 1; break;
      case 20: f.items[0].listed_occurrences = 5; break;
      case 21: f.items[0].packed = 1; f.items[0].listed = 4; break;
      case 22: f.inventory.listed_nodes = 2; break;
      case 23: f.items[1].listed = INT32_MAX; f.items[1].packed = 1; f.items[1].total = INT32_MAX; break;
      case 24: f.inventory.listed_objects.count = 3; break;
      case 25: f.inventory.listed_objects.data = nullptr; break;
    }
    const auto value = f.get();
    check(!value.known() && !value.craftable && !value.maximum && !value.evidence.stable_boundary_verified &&
          !g::plan(f.recipe, value.evidence, 2).valid, "defekte Aufnahme bleibt auch fuer nachgelagerten Plan gesperrt");
  }
  for (unsigned mode = 0; mode < 6; ++mode) {
    Fixture f;
    switch (mode) {
      case 0: f.items[0].listed_occurrences = 2; break;
      case 1: f.packed_input(5); f.items[0].packed_occurrences = 2; f.inventory.packed_entries = 2; break;
      case 2: f.items[0].duplicate_packed_target = true; break;
      case 3: f.items[0].has_active_marker = true; break;
      case 4: f.items[0].mixed_representation = true; break;
      case 5: f.items[0].listed = 4; f.items[0].packed = 1; f.items[0].packed_occurrences = 1;
              f.inventory.packed_entries = 1; break;
    }
    const auto value = f.get();
    check(value.issue == Issue::ambiguous_ingredient && !value.craftable && !value.maximum &&
          !value.evidence.stable_boundary_verified, "mehrdeutiger oder aktiver Input gesperrt");
  }
  // The existing quantity plan requires an unambiguous consumed input.
  // Fragmented target or cold stock does not introduce a new cost rule.
  { Fixture f; f.items[1].listed_occurrences = 2; f.items[2].listed_occurrences = 2;
    f.inventory.listed_nodes = 5;
    std::array<std::uint32_t, 5> objects{{1, 2, 3, 4, 5}};
    f.inventory.listed_objects = {objects.data(), objects.size()};
    check(f.get().craftable, "Ausgabe- und Kaltbestand duerfen aggregiert sein"); }
}

void hand_and_proofs() {
  for (const auto tmp : {1, 2}) {
    Fixture f; f.hands.left_tmp_level = f.proofs.verified_left_tmp_level = tmp;
    check(f.get().craftable, "nur aktuell bewiesene generierte tmp-Stufe");
  }
  for (unsigned mode = 0; mode < 23; ++mode) {
    Fixture f;
    switch (mode) {
      case 0: f.proofs.stable_boundary_verified = false; break;
      case 1: f.proofs.station_hand_end_verified = false; break;
      case 2: f.proofs.input_objects_verified = false; break;
      case 3: f.proofs.verified_left_tmp_level = 0; break;
      case 4: f.proofs.verified_left_tmp_level = 3; break;
      case 5: f.hands.left_slot_known = false; break;
      case 6: f.hands.right_slot_known = false; break;
      case 7: f.hands.interact_item_known = false; break;
      case 8: f.hands.hands.left.state = 2; break;
      case 9: f.hands.hands.left.object = 0; break;
      case 10: f.hands.hands.left.symbol = f.recipe.input; break;
      case 11: f.hands.hands.left.symbol = 16612; break;
      case 12: f.hands.hands.left.amount = 0; break;
      case 13: f.hands.hands.left.amount = 2; break;
      case 14: f.hands.hands.right.state = -1; break;
      case 15: f.hands.hands.right.object = 0x6000; break;
      case 16: f.hands.interact_item = 0; break;
      case 17: f.hands.interact_item = 0x6000; break;
      case 18: f.hands.left_tmp_level = 2; break;
      case 19: f.objects[3] = f.hands.hands.left.object; break;
      case 20: f.objects[3] = f.objects[0]; break;
      case 21: f.objects[3] = 0; break;
      case 22: f.hands.hands.left.state = 0; break;
    }
    const auto value = f.get();
    check(!value.known() && !value.craftable && !value.maximum && !value.evidence.stable_boundary_verified,
          "falsche Hand, fremder Alias oder fehlender Anschlussbeleg gesperrt");
  }
  Fixture f;
  const auto value = ubc::grind_recipe_material(f.recipe, f.inventory, f.hands, f.knowledge, 2);
  check(!value.craftable && !value.evidence.stable_boundary_verified, "Defaultbeweise sind geschlossen");
}

void knowledge_quantity_and_edges() {
  for (unsigned mode = 0; mode < 3; ++mode) {
    Fixture f;
    if (!mode) f.knowledge.talent_known = false;
    if (mode == 1) f.knowledge.guild_known = false;
    if (mode == 2) f.knowledge.skill_known = false;
    const auto value = f.get();
    check(value.issue == Issue::knowledge && !value.known() && !value.evidence.stable_boundary_verified,
          "fehlende Wissen/Gilde/Skill-Aufnahme gesperrt");
  }
  for (const auto talent : {0, 2, -1, INT32_MAX}) {
    Fixture f; f.knowledge.talent = talent;
    const auto value = f.get();
    check(value.known() && !value.learned && !value.craftable && !value.maximum &&
          value.numerical.issue == g::Issue::knowledge, "Talent exakt1; gelesen ist nicht gelernt");
  }
  for (const auto guild : {0, 6, 7, -1, INT32_MAX}) {
    Fixture f; f.knowledge.guild = guild;
    const auto value = f.get();
    check(value.known() && !value.guild_allowed && !value.craftable && !value.maximum &&
          value.numerical.issue == g::Issue::guild, "unpassende Gilde bleibt Originalbedingung");
  }
  { Fixture f; const auto value = f.get(6);
    check(value.known() && value.maximum == 5 && !value.craftable && value.rows[0].missing == 1 &&
          value.rows[0].remaining == -1, "Fehlbestand mit bekannter Vorschau ohne Effekt"); }
  { Fixture f; f.items[0].listed = f.items[0].total = 0;
    check(f.get().known() && !f.get().maximum && !f.get().craftable && f.get().rows[0].missing == 2,
          "Nullbestand ist bekannt, keine kuenstliche Eins"); }
  { Fixture f; const auto value = f.get(UINT32_MAX);
    check(value.issue == Issue::quantity && !value.craftable && !value.maximum &&
          !value.evidence.stable_boundary_verified && value.rows[0].required == -1,
          "nicht darstellbare Menge erzeugt keine Rechnung oder offene Evidence"); }
  { Fixture f; f.items[1].listed = f.items[1].total = INT32_MAX - 2;
    check(f.get().craftable && f.get().maximum == 2 && f.get().numerical.after_end[1].amount == INT32_MAX,
          "Waffenausgaberaum begrenzt Menge");
    check(!f.get(3).craftable && f.get(3).rows[1].output_shortfall == 1, "Waffenueberlauf sichtbar"); }
  { Fixture f; f.items[2].listed = f.items[2].total = INT32_MAX;
    const auto value = f.get();
    check(value.known() && !value.craftable && !value.maximum && value.numerical.issue == g::Issue::lifecycle_room &&
          value.rows[2].output_shortfall == 1, "einzelne Kaltkompensation braucht Zwischenraum"); }
  { Fixture f; f.items[2].listed = f.items[2].total = INT32_MAX - 1;
    check(f.get(5).craftable && f.get(5).numerical.after_callback[2].amount == INT32_MAX,
          "ein Platz reicht auch fuer Menge5"); }
  { Fixture f; f.packed_input(INT32_MAX); f.items[1].listed = f.items[1].total = 0;
    f.items[2].listed = f.items[2].total = 0; f.knowledge.skill = INT32_MIN;
    const auto value = f.get(INT32_MAX);
    check(value.craftable && value.maximum == INT32_MAX && value.numerical.skill_after == -1 &&
          value.rows[0].remaining == 0 && value.rows[1].remaining == INT32_MAX &&
          value.numerical.after_callback[2].amount == 1, "64Bit-Zwischenrechnung ohne Mengenobergrenze"); }
  { Fixture f; auto descriptor = f.recipe; descriptor.output = descriptor.input;
    const auto value = ubc::grind_recipe_material(descriptor, f.inventory, f.hands, f.knowledge, 2, f.proofs);
    check(value.issue == Issue::descriptor && !value.craftable && !value.evidence.stable_boundary_verified,
          "keine fremden Deskriptoren"); }
}
} // namespace

int main() {
  all_recipes(); inventory_failures(); hand_and_proofs(); knowledge_quantity_and_edges();
  std::printf("grind_recipe_material_tests: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
