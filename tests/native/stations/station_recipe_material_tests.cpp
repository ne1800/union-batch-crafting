// SPDX-License-Identifier: MIT
#include "ubc/stations/station_recipe_material.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {
namespace c = ubc::station_recipe_catalog;
unsigned checks = 0, failures = 0;
void check(bool condition, const char* name) {
  ++checks;
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}
const c::Descriptor& recipe(c::Station station, std::uint32_t output_units = 0) {
  for (const auto& value : c::recipes)
    if (value.station == station && (!output_units || value.outputs[0].units == output_units))
      return value;
  std::abort();
}
struct Fixture {
  c::Descriptor descriptor;
  std::vector<ubc::RecipeInventoryItem> items;
  std::vector<ubc::StationRecipeMaterialRow> rows;
  std::vector<std::uint32_t> objects;
  ubc::StationRecipeHands observed{};
  explicit Fixture(const c::Descriptor& value)
      : descriptor(value), items(value.targets.count), rows(value.targets.count) {
    for (std::size_t i = 0; i < items.size(); ++i) {
      items[i].symbol = value.targets[i].symbol;
      set(items[i].symbol, 0, 0);
    }
    for (const auto& cost : value.costs)
      set(cost.symbol, static_cast<std::int32_t>(cost.units * 7), 0);
    for (const auto& output : value.outputs)
      set(output.symbol, 3, 0);
    const auto hand = [&](const c::HandItem& expected, std::uint32_t pointer) {
      if (expected.source == c::HandSource::empty)
        return ubc::HandObjectSnapshot{0, -1, -1, 0};
      if (expected.source == c::HandSource::reserved_inventory_tool)
        set(expected.symbol, 6, 0);
      return ubc::HandObjectSnapshot{1, expected.symbol, static_cast<std::int32_t>(expected.amount),
                                     pointer};
    };
    observed.hands = {hand(value.hand.left, 0x8ff000), hand(value.hand.right, 0x8fe000)};
    observed.left_slot_known = observed.right_slot_known = true;
    observed.left_tmp_level = value.hand.left.source == c::HandSource::generated ? 1 : 0;
    observed.right_tmp_level = value.hand.right.source == c::HandSource::generated ? 2 : 0;
    observed.interact_item_known = true;
    observed.interact_item = value.hand.anchor == c::HandSide::left ? observed.hands.left.object
                                                                    : observed.hands.right.object;
  }
  ubc::RecipeInventoryItem& item(std::int32_t symbol) {
    for (auto& value : items)
      if (value.symbol == symbol)
        return value;
    std::abort();
  }
  ubc::StationRecipeMaterialRow& row(std::int32_t symbol) {
    for (auto& value : rows)
      if (value.symbol == symbol)
        return value;
    std::abort();
  }
  void set(std::int32_t symbol, std::int32_t listed, std::int32_t packed) {
    auto& value = item(symbol);
    value.listed = listed;
    value.packed = packed;
    value.total = static_cast<std::int32_t>(std::int64_t{listed} + packed);
    value.listed_occurrences = listed > 0 ? 1 : 0;
    value.packed_occurrences = packed > 0 ? 1 : 0;
    value.duplicate_packed_target = value.has_active_marker = false;
    value.mixed_representation = value.listed_occurrences && value.packed_occurrences;
  }
  ubc::RecipeInventory inventory() {
    ubc::RecipeInventory result;
    result.header_known = result.listed_known = result.packed_known = result.storage_totals_known =
        true;
    result.stable_boundary_verified = result.snapshot_unchanged = true;
    objects.clear();
    for (const auto& value : items) {
      result.packed_entries += value.packed_occurrences;
      for (std::uint32_t i = 0; i < value.listed_occurrences; ++i)
        objects.push_back(0x10000u + static_cast<std::uint32_t>(objects.size()) * 16);
    }
    result.listed_nodes = static_cast<std::uint32_t>(objects.size());
    result.items = {items.data(), items.size()};
    result.listed_objects = {objects.data(), objects.size()};
    return result;
  }
  ubc::StationRecipeMaterial evaluate(std::uint32_t quantity, bool verified = true) {
    return ubc::station_recipe_material(descriptor, inventory(), observed, quantity, rows.data(),
                                        rows.size(), verified);
  }
};

void actual_descriptors() {
  for (const auto& descriptor : c::recipes) {
    Fixture f(descriptor);
    auto result = f.evaluate(3);
    check(result.known() && result.inventory_known && result.hand_known && result.craftable &&
              result.maximum == 7,
          "each actual station descriptor uses its exact independent cost and hand model");
    check(result.completion == descriptor.completion && result.count == descriptor.targets.count,
          "completion kind and full target count stay explicit, not inferred from callback return");
    for (const auto& cost : descriptor.costs) {
      const auto& row = f.row(cost.symbol);
      check(row.available_known && row.available == std::int64_t{cost.units} * 7 &&
                row.required == std::int64_t{cost.units} * 3 && !row.missing &&
                row.expected_ready_storage == std::int64_t{cost.units} * 4 &&
                row.expected_end_storage == row.expected_ready_storage && !row.reserved_tool &&
                !row.generated_hand,
            "ingredient counts never borrow generated hands or a reserved station tool");
    }
    for (const auto& output : descriptor.outputs) {
      const auto& row = f.row(output.symbol);
      check(row.produced == std::int64_t{output.units} * 3 &&
                row.expected_ready_storage == 3 + std::int64_t{output.units} * 3 &&
                row.expected_end_storage == row.expected_ready_storage,
            "scaled output preserves separate material-effect and normal-exit forecasts");
    }
    result = f.evaluate(0);
    check(result.known() && !result.craftable && result.maximum == 7,
          "known zero request is preview only and never permission to invoke a recipe");
    result = f.evaluate(8);
    check(result.known() && !result.craftable && result.maximum == 7,
          "over-maximum request remains explainable with no material permission");
    std::reverse(f.items.begin(), f.items.end());
    check(f.evaluate(7).craftable,
          "stock is joined by exact symbol identity rather than input order");
    result = f.evaluate(1, false);
    check(result.inventory_known && !result.hand_known && !result.maximum && !result.craftable &&
              result.hand_issue == ubc::StationRecipeHandIssue::unverified_contract,
          "static station evidence does not replace the explicit current native hand proof");
    f.descriptor.hand.observed_in_world = true;
    check(!f.evaluate(1, false).hand_known,
          "even a prior world observation does not replace the current explicit hand proof");
    f.descriptor.hand.static_lifecycle_verified = false;
    check(!f.evaluate(1, true).hand_known,
          "an unproven static lifecycle remains blocked despite a supplied current proof");
  }
}

void tool_and_generated_lifetimes() {
  Fixture saw(recipe(c::Station::saw, 30));
  check(saw.descriptor.hand.right.source == c::HandSource::reserved_inventory_tool &&
            saw.descriptor.completion == c::Completion::ready_same_station,
        "ammo has a reserved tool and ready terminal instead of an invented production end");
  for (const auto n : {0u, 1u, 3u, 7u}) {
    const auto r = saw.evaluate(n);
    const auto& tool = saw.row(16609);
    check(r.known() && tool.stored == 6 && tool.reserved_tool == 1 && tool.generated_hand == 0 &&
              tool.required == 0 && tool.produced == 0 && tool.expected_ready_storage == 6 &&
              tool.expected_end_storage == 7,
          "one saw remains held across any lot count and returns exactly once only on normal end");
  }
  saw.set(16609, 0, 0);
  check(saw.evaluate(7).craftable && saw.row(16609).expected_ready_storage == 0 &&
            saw.row(16609).expected_end_storage == 1,
        "the reserved saw suffices without an additional stored saw and is not consumed per lot");
  saw.set(16609, 2, 4);
  saw.item(16609).packed_occurrences = 2;
  saw.item(16609).duplicate_packed_target = true;
  saw.item(16609).has_active_marker = true;
  check(
      saw.evaluate(7).craftable && saw.row(16609).expected_end_storage == 7,
      "mixed extra stored tools are not ingredient ambiguity; the actual hand object still needs an alias proof");
  saw.set(16609, INT32_MAX, 0);
  auto r = saw.evaluate(0);
  check(
      r.inventory_known && r.hand_known && !r.maximum && !r.craftable &&
          r.issue == ubc::StationRecipeMaterialIssue::return_capacity &&
          saw.row(16609).end_overflow == 1,
      "unrepresentable normal tool return blocks manufacture even though the ready stock would fit");
  saw.set(16609, INT32_MAX - 1, 0);
  check(saw.evaluate(1).craftable && saw.row(16609).expected_end_storage == INT32_MAX,
        "largest safe stored tool count includes exactly one later inventory return");
  saw.observed.right_slot_known = false;
  r = saw.evaluate(3);
  check(r.inventory_known && !r.hand_known && saw.row(16609).stored == INT32_MAX - 1 &&
            saw.row(16609).reserved_tool == -1 && saw.row(16609).ready_storage_known &&
            !saw.row(16609).end_storage_known && saw.row(16609).expected_end_storage == -1,
        "unknown reserved tool keeps stored numbers but never invents a known end return");

  Fixture leather(recipe(c::Station::leatherwork));
  r = leather.evaluate(2);
  check(r.known() && leather.row(17342).generated_hand == 1 &&
            leather.row(17342).reserved_tool == 0 &&
            leather.row(17342).expected_ready_storage == 0 &&
            leather.row(17342).expected_end_storage == 0,
        "generated leather prop is never material stock or a refundable inventory tool");
  Fixture forge(recipe(c::Station::forge));
  check(forge.observed.hands.left.symbol == 16666 && forge.observed.hands.right.symbol == 16667 &&
            forge.observed.interact_item == forge.observed.hands.right.object,
        "forge observation binds two different generated hands and the right interact-item anchor");
  forge.set(16666, 2, 4);
  r = forge.evaluate(3);
  check(
      r.craftable && forge.row(16666).generated_hand == 1 && forge.row(16666).reserved_tool == 0 &&
          forge.row(16666).stored == 6 && forge.row(16666).expected_ready_storage == 6 &&
          forge.row(16666).expected_end_storage == 6,
      "normal forge cleanup preserves stored left tongs; only the right old interact-item instance is debited");
  forge.set(forge.descriptor.costs[0].symbol, 0, 0);
  r = forge.evaluate(0);
  check(r.known() && !r.maximum && forge.row(forge.descriptor.costs[0].symbol).missing_for_one == 1,
        "two tongs do not fabricate a raw or hot steel hand reserve");
}

void output_limits_and_missing_costs() {
  for (const auto& descriptor : c::recipes) {
    Fixture f(descriptor);
    for (const auto& cost : descriptor.costs)
      f.set(cost.symbol, INT32_MAX, 0);
    const auto output = descriptor.outputs[0];
    f.set(output.symbol, INT32_MAX - static_cast<std::int32_t>(output.units) * 2, 0);
    auto r = f.evaluate(2);
    check(
        r.known() && r.craftable && r.maximum == 2 &&
            f.row(output.symbol).expected_end_storage == INT32_MAX,
        "actual per-lot output headroom limits valid N instead of rejecting abundant ingredients");
    r = f.evaluate(3);
    check(!r.craftable && f.row(output.symbol).output_shortfall == output.units,
          "first excessive lot reports its full scaled output shortage");
    f.set(output.symbol, INT32_MAX, 0);
    r = f.evaluate(0);
    check(r.known() && !r.maximum && f.row(output.symbol).output_shortfall_for_one == output.units,
          "full output inventory remains known with an exact one-lot shortage");
    f.set(output.symbol, 0, 0);
    for (const auto& cost : descriptor.costs) {
      f.set(cost.symbol, static_cast<std::int32_t>(cost.units) - 1, 0);
      r = f.evaluate(0);
      check(r.known() && !r.maximum && f.row(cost.symbol).missing_for_one == 1 &&
                f.row(cost.symbol).required == 0 && f.row(cost.symbol).produced == 0,
            "every actual ingredient has an independent known shortage at N0");
      f.set(cost.symbol, INT32_MAX, 0);
    }
    r = f.evaluate(UINT32_MAX);
    check(
        r.issue == ubc::StationRecipeMaterialIssue::quantity && !r.craftable &&
            r.maximum == static_cast<std::uint32_t>(INT32_MAX) / descriptor.outputs[0].units,
        "out-of-range request cannot wrap native quantities and retains independent numeric maximum");
  }
  for (const auto units : {10u, 30u}) {
    Fixture f(recipe(c::Station::saw, units));
    for (const auto& cost : f.descriptor.costs)
      f.set(cost.symbol, INT32_MAX, 0);
    f.set(f.descriptor.outputs[0].symbol, 0, 0);
    const auto n = static_cast<std::uint32_t>(INT32_MAX) / units;
    const auto r = f.evaluate(n);
    check(r.craftable && r.maximum == n &&
              f.row(f.descriptor.outputs[0].symbol).produced == std::int64_t{units} * n,
          "ten- and thirty-projectile lot limits preserve exact native int32 arithmetic");
  }
}

void observed_forge_contract() {
  // Regression test of values from the world observation. No engine call or runtime
  // evidence. Deliberately derive the hands independently of the catalog under test.
  for (const auto id : {c::RecipeId::heat_raw_steel, c::RecipeId::form_hot_blade}) {
    Fixture f(*c::find(id));
    f.observed.hands = {{1, 16666, 1, 0x8000}, {1, 16667, 1, 0x9000}};
    f.observed.left_tmp_level = f.observed.right_tmp_level = 1;
    f.observed.interact_item = 0x9000;
    const bool raw = id == c::RecipeId::heat_raw_steel;
    f.set(f.descriptor.costs[0].symbol, 0, raw ? 8 : 1);
    if (raw)
      f.set(16611, 0, 1);
    auto r = f.evaluate(1);
    check(r.hand_known && r.craftable && r.maximum == (raw ? 8u : 1u),
          "observed two tmp1 props with right anchor preserve actual packed steel maxima");
    f.observed.interact_item = 0x8000;
    r = f.evaluate(1);
    check(!r.hand_known && !r.craftable && !r.maximum &&
              r.hand_issue == ubc::StationRecipeHandIssue::anchor,
          "old left-anchor assumption is rejected rather than accepting either hand");
    f.observed.interact_item = 0x9000;
    for (const bool packed : {false, true}) {
      f.set(16667, packed ? 0 : 1, packed ? 1 : 0);
      r = f.evaluate(1);
      check(!r.hand_known && !r.craftable && !r.maximum &&
                r.hand_issue == ubc::StationRecipeHandIssue::stored_prop,
            "one stored right-anchor instance blocks before the later native Destroy debit");
    }
    f.set(16667, 0, 0);
    f.set(16666, 2, 4);
    r = f.evaluate(1);
    check(
        r.hand_known && r.craftable && f.row(16666).expected_ready_storage == 6 &&
            f.row(16666).expected_end_storage == 6 && f.row(16667).expected_end_storage == 0,
        "left stored prop remains independently conserved with no invented generated-tool refund");
    f.observed.interact_item_known = false;
    r = f.evaluate(1);
    check(!r.hand_known && !r.craftable && r.hand_issue == ubc::StationRecipeHandIssue::anchor,
          "a missing current anchor proof does not inherit the prior world observation");
  }
}

void hand_failures() {
  for (const auto station : {c::Station::leatherwork, c::Station::saw, c::Station::forge}) {
    const auto& descriptor = recipe(station);
    for (unsigned mode = 0; mode < 10; ++mode) {
      Fixture f(descriptor);
      auto& active = descriptor.hand.anchor == c::HandSide::left ? f.observed.hands.left
                                                                 : f.observed.hands.right;
      switch (mode) {
      case 0:
        active.state = -1;
        break;
      case 1:
        active.state = 2;
        break;
      case 2:
        active.object = 0;
        break;
      case 3:
        active.symbol += 1;
        break;
      case 4:
        active.amount = 0;
        break;
      case 5:
        active.amount = 2;
        break;
      case 6:
        f.observed.interact_item_known = false;
        break;
      case 7:
        f.observed.interact_item = 0;
        break;
      case 8:
        f.observed.interact_item += 4;
        break;
      default:
        active.state = 0;
        break;
      }
      const auto r = f.evaluate(1);
      check(r.inventory_known && !r.hand_known && !r.maximum && !r.craftable,
            "unknown, wrong or detached active hand cannot grant production from known stock");
    }
    for (const bool left : {false, true}) {
      const auto& hand = left ? descriptor.hand.left : descriptor.hand.right;
      if (hand.source == c::HandSource::empty)
        continue;
      for (const auto level : {-1, 0, 1, 2, 3}) {
        Fixture f(descriptor);
        if (left)
          f.observed.left_tmp_level = level;
        else
          f.observed.right_tmp_level = level;
        const bool valid =
            hand.source == c::HandSource::generated ? level == 1 || level == 2 : level == 0;
        const auto r = f.evaluate(1);
        check(r.hand_known == valid && r.craftable == valid,
              "tmpLevel is actually read: generated accepts 1/2, reserved saw requires 0");
      }
      Fixture f(descriptor);
      if (left)
        f.observed.left_slot_known = false;
      else
        f.observed.right_slot_known = false;
      check(!f.evaluate(1).hand_known,
            "missing slot-lifetime evidence stays unknown even with exact item identity");
    }
    Fixture f(descriptor);
    auto inv = f.inventory();
    f.objects[0] = f.observed.hands.right.object;
    auto r = ubc::station_recipe_material(descriptor, inv, f.observed, 1, f.rows.data(),
                                          f.rows.size(), true);
    check(!r.hand_known && r.hand_issue == ubc::StationRecipeHandIssue::inventory_alias,
          "right hand cannot alias any item in the complete inventory list");
    if (station == c::Station::forge) {
      inv = f.inventory();
      f.objects.back() = f.observed.hands.left.object;
      r = ubc::station_recipe_material(descriptor, inv, f.observed, 1, f.rows.data(), f.rows.size(),
                                       true);
      check(!r.hand_known && r.hand_issue == ubc::StationRecipeHandIssue::inventory_alias,
            "forge left generated prop also cannot alias a listed ingredient or unrelated item");
      f.observed.hands.right.object = f.observed.hands.left.object;
      check(!f.evaluate(1).hand_known, "two named hands must own different actual objects");
    } else {
      f.observed.hands.left = {1, 99999, 1, 0x777700};
      check(!f.evaluate(1).hand_known, "leather and saw require a positively empty left hand");
    }
  }
}

void stored_props_and_ambiguity() {
  for (const auto& descriptor : c::recipes) {
    for (const auto& hand : {descriptor.hand.left, descriptor.hand.right}) {
      if (!hand.require_stored_absent)
        continue;
      for (unsigned variant = 0; variant < 4; ++variant) {
        Fixture f(descriptor);
        auto& item = f.item(hand.symbol);
        if (variant == 0)
          f.set(hand.symbol, 1, 0);
        if (variant == 1)
          f.set(hand.symbol, 0, 1);
        if (variant == 2)
          item.listed_occurrences = 1;
        if (variant == 3)
          item.has_active_marker = true;
        const auto r = f.evaluate(1);
        check(r.inventory_known && !r.hand_known && !r.maximum &&
                  r.hand_issue == ubc::StationRecipeHandIssue::stored_prop,
              "destroy-anchor prop must be absent even as a listed zero object or active marker");
      }
    }
    for (const auto& cost : descriptor.costs)
      for (unsigned mode = 0; mode < 5; ++mode) {
        Fixture f(descriptor);
        auto& item = f.item(cost.symbol);
        if (mode == 0)
          item.duplicate_packed_target = true;
        if (mode == 1)
          item.has_active_marker = true;
        if (mode == 2)
          item.mixed_representation = true;
        if (mode == 3)
          item.listed_occurrences = 2;
        if (mode == 4)
          f.set(cost.symbol, 1, 6);
        const auto r = f.evaluate(1);
        check(r.inventory_known && r.hand_known && !r.maximum && !r.craftable &&
                  r.issue == ubc::StationRecipeMaterialIssue::ambiguous_ingredient,
              "only counted ingredients reject mixed/duplicate/active representations");
      }
    Fixture f(descriptor);
    f.set(descriptor.outputs[0].symbol, 1, 2);
    f.item(descriptor.outputs[0].symbol).duplicate_packed_target = true;
    f.item(descriptor.outputs[0].symbol).has_active_marker = true;
    check(f.evaluate(3).craftable,
          "mixed/marked output stacks retain their exact numerical headroom");
  }
}

void invalid_reader_and_buffers() {
  const auto& descriptor = recipe(c::Station::saw);
  for (unsigned mode = 0; mode < 8; ++mode) {
    Fixture f(descriptor);
    auto inv = f.inventory();
    switch (mode) {
    case 0:
      inv.header_known = false;
      break;
    case 1:
      inv.listed_known = false;
      break;
    case 2:
      inv.packed_known = false;
      break;
    case 3:
      inv.storage_totals_known = false;
      break;
    case 4:
      inv.stable_boundary_verified = false;
      break;
    case 5:
      inv.snapshot_unchanged = false;
      break;
    case 6:
      inv.issue = ubc::PackedInventoryIssue::changed_during_read;
      break;
    default:
      inv.issue = ubc::PackedInventoryIssue::invalid_amount;
      break;
    }
    const auto r = ubc::station_recipe_material(descriptor, inv, f.observed, 1, f.rows.data(),
                                                f.rows.size(), true);
    check(!r.inventory_known && !r.hand_known && !r.maximum && !r.craftable &&
              f.rows[0].stored == -1,
          "all complete stable packed/list reader gates must be positively supplied");
  }
  for (unsigned mode = 0; mode < 10; ++mode) {
    Fixture f(descriptor);
    auto& item = f.items[0];
    switch (mode) {
    case 0:
      item.total = -1;
      break;
    case 1:
      item.packed = -1;
      break;
    case 2:
      item.total += 1;
      break;
    case 3:
      item.listed_occurrences = 0;
      break;
    case 4:
      f.items.back().symbol = item.symbol;
      break;
    case 5:
      item.symbol = 99999;
      break;
    case 6:
      item.listed = INT32_MAX;
      item.packed = 1;
      item.total = INT32_MAX;
      break;
    case 7:
      item.packed = 1;
      item.total += 1;
      item.packed_occurrences = 0;
      break;
    case 8:
      item.listed = -1;
      break;
    default:
      item.total = INT32_MIN;
      break;
    }
    check(!f.evaluate(1).craftable,
          "corrupt sums, provenance or symbol mapping cannot become known manufacture");
  }
  Fixture f(descriptor);
  auto inv = f.inventory();
  check(ubc::station_recipe_material(descriptor, inv, f.observed, 1, nullptr, f.rows.size(), true)
                .issue == ubc::StationRecipeMaterialIssue::output_buffer,
        "null output buffer is rejected before writing");
  check(ubc::station_recipe_material(descriptor, inv, f.observed, 1, f.rows.data(),
                                     f.rows.size() - 1, true)
                .issue == ubc::StationRecipeMaterialIssue::output_buffer,
        "insufficient caller-owned capacity never truncates targets");
  auto broken = descriptor;
  broken.targets.data = nullptr;
  check(ubc::station_recipe_material(broken, inv, f.observed, 1, f.rows.data(), f.rows.size(), true)
                .issue == ubc::StationRecipeMaterialIssue::descriptor,
        "bad descriptor view is rejected without following its pointers");
  auto mapping = inv;
  mapping.items.data = nullptr;
  check(ubc::station_recipe_material(descriptor, mapping, f.observed, 1, f.rows.data(),
                                     f.rows.size(), true)
                .issue == ubc::StationRecipeMaterialIssue::target_mapping,
        "null projected item view is rejected before lookup");
  mapping = inv;
  mapping.items.count -= 1;
  check(!ubc::station_recipe_material(descriptor, mapping, f.observed, 1, f.rows.data(),
                                      f.rows.size(), true)
             .inventory_known,
        "partial target projection cannot silently treat the missing symbol as known zero");
  auto pointers = inv;
  pointers.listed_objects.data = nullptr;
  check(ubc::station_recipe_material(descriptor, pointers, f.observed, 1, f.rows.data(),
                                     f.rows.size(), true)
                .hand_issue == ubc::StationRecipeHandIssue::inventory_alias,
        "missing complete pointer list cannot prove absence of a hand alias");
  pointers = inv;
  pointers.listed_objects.count -= 1;
  check(
      !ubc::station_recipe_material(descriptor, pointers, f.observed, 1, f.rows.data(),
                                    f.rows.size(), true)
           .hand_known,
      "a truncated pointer list is insufficient even when all projected target amounts are known");
  std::vector<ubc::StationRecipeMaterialRow> roomy(9);
  roomy[descriptor.targets.count].symbol = 123456;
  auto result = ubc::station_recipe_material(descriptor, inv, f.observed, 1, roomy.data(),
                                             roomy.size(), true);
  check(
      result.craftable && result.count == descriptor.targets.count &&
          roomy[descriptor.targets.count].symbol == 123456,
      "caller capacity above six is supported without converting unused slots into recipe targets");
  f.objects.resize(ubc::max_packed_inventory_nodes);
  for (std::size_t i = 0; i < f.objects.size(); ++i)
    f.objects[i] = 0x500000 + static_cast<std::uint32_t>(i) * 16;
  inv.listed_nodes = static_cast<std::uint32_t>(f.objects.size());
  inv.listed_objects = {f.objects.data(), f.objects.size()};
  check(ubc::station_recipe_material(descriptor, inv, f.observed, 1, f.rows.data(), f.rows.size(),
                                     true)
            .craftable,
        "full bounded list includes unrelated items without limiting recipe target count");
  f.objects.back() = f.observed.hands.right.object;
  check(ubc::station_recipe_material(descriptor, inv, f.observed, 1, f.rows.data(), f.rows.size(),
                                     true)
                .hand_issue == ubc::StationRecipeHandIssue::inventory_alias,
        "alias in final bounded list node cannot evade the hand check");
  f.objects.back() = f.objects.front();
  check(!ubc::station_recipe_material(descriptor, inv, f.observed, 1, f.rows.data(), f.rows.size(),
                                      true)
             .hand_known,
        "duplicated unrelated inventory identity invalidates the claimed full-list proof");
  inv.listed_nodes = ubc::max_packed_inventory_nodes + 1;
  check(!ubc::station_recipe_material(descriptor, inv, f.observed, 1, f.rows.data(), f.rows.size(),
                                      true)
             .inventory_known,
        "reader list bound is checked before pointer enumeration");
  inv = f.inventory();
  inv.packed_entries = ubc::max_packed_inventory_entries + 1;
  check(!ubc::station_recipe_material(descriptor, inv, f.observed, 1, f.rows.data(), f.rows.size(),
                                      true)
             .inventory_known,
        "packed entry bound is also checked before inventory acceptance");
}
} // namespace

int main() {
  actual_descriptors();
  tool_and_generated_lifetimes();
  output_limits_and_missing_costs();
  observed_forge_contract();
  hand_failures();
  stored_props_and_ambiguity();
  invalid_reader_and_buffers();
  std::printf("station_recipe_material_tests: %u checks, %u failures\n", checks, failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
