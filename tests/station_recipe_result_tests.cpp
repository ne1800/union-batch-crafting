// SPDX-License-Identifier: MIT
#include "ubc/stations/station_recipe_result.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>

namespace {
namespace c = ubc::station_recipe_catalog;
using S = ubc::StationRecipeResultState;
using I = ubc::StationRecipeResultIssue;
unsigned checks = 0, failures = 0;
void check(bool condition, const char* message) {
  ++checks;
  if (!condition) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}
const c::Descriptor& recipe(c::Station station) {
  for (const auto& value : c::recipes) if (value.station == station) return value;
  std::abort();
}
struct Fixture {
  c::Descriptor descriptor;
  std::vector<ubc::RecipeInventoryItem> inventory_items;
  std::vector<std::uint32_t> inventory_objects;
  std::vector<ubc::StationRecipeMaterialRow> rows;
  std::vector<ubc::StationRecipeResultItem> ready_items, end_items;
  ubc::StationRecipeMaterial material;
  ubc::StationRecipeResultBinding binding;

  explicit Fixture(const c::Descriptor& value, std::uint32_t quantity = 3)
      : descriptor(value), inventory_items(value.targets.count), rows(value.targets.count) {
    for (std::size_t i = 0; i < inventory_items.size(); ++i) {
      auto& item = inventory_items[i]; item.symbol = value.targets[i].symbol;
      item.listed = item.packed = item.total = 0;
    }
    for (const auto& cost : value.costs) set(cost.symbol, static_cast<std::int32_t>(cost.units * 7));
    for (const auto& output : value.outputs) set(output.symbol, 3);
    const auto hand = [&](const c::HandItem& want, std::uint32_t object) {
      if (want.source == c::HandSource::empty) return ubc::HandObjectSnapshot{0, -1, -1, 0};
      if (want.source == c::HandSource::reserved_inventory_tool) set(want.symbol, 6);
      return ubc::HandObjectSnapshot{1, want.symbol, static_cast<std::int32_t>(want.amount), object};
    };
    binding.hands.hands = {hand(value.hand.left, 0x8000), hand(value.hand.right, 0x9000)};
    binding.hands.left_slot_known = binding.hands.right_slot_known = true;
    binding.hands.left_tmp_level = value.hand.left.source == c::HandSource::generated ? 1 : 0;
    binding.hands.right_tmp_level = value.hand.right.source == c::HandSource::generated ? 2 : 0;
    binding.hands.interact_item_known = true;
    binding.hands.interact_item = value.hand.anchor == c::HandSide::left ?
        binding.hands.hands.left.object : binding.hands.hands.right.object;
    binding.generation = 7; binding.start_frame = 100; binding.collection = 20; binding.end_sequence = 4;
    binding.world = 1; binding.hero = 2; binding.station = 3;
    evaluate(quantity);
  }
  void set(std::int32_t symbol, std::int32_t amount) {
    for (auto& item : inventory_items) if (item.symbol == symbol) {
      item.listed = item.total = amount; item.listed_occurrences = amount ? 1u : 0u; return;
    }
    std::abort();
  }
  void evaluate(std::uint32_t quantity) {
    ubc::RecipeInventory inventory;
    inventory.header_known = inventory.listed_known = inventory.packed_known = inventory.storage_totals_known = true;
    inventory.stable_boundary_verified = inventory.snapshot_unchanged = true;
    inventory_objects.clear();
    for (const auto& item : inventory_items) if (item.listed_occurrences)
      inventory_objects.push_back(0x1000u + static_cast<std::uint32_t>(inventory_objects.size()) * 16u);
    inventory.listed_nodes = static_cast<std::uint32_t>(inventory_objects.size());
    inventory.items = {inventory_items.data(), inventory_items.size()};
    inventory.listed_objects = {inventory_objects.data(), inventory_objects.size()};
    material = ubc::station_recipe_material(descriptor, inventory, binding.hands, quantity, rows.data(), rows.size(), true);
    ready_items.clear(); end_items.clear();
    for (const auto& row : rows) {
      ready_items.push_back({row.symbol, row.expected_ready_storage});
      end_items.push_back({row.symbol, row.expected_end_storage});
    }
  }
  c::View<ubc::StationRecipeMaterialRow> amounts() const { return {rows.data(), rows.size()}; }
  ubc::StationRecipeResultObservation ready() const {
    ubc::StationRecipeResultObservation value;
    value.generation = binding.generation; value.frame = binding.start_frame + 2;
    value.stable_boundary_verified = value.identity_known = value.station_known = value.production_known = true;
    value.world = binding.world; value.hero = binding.hero; value.station = binding.station;
    value.production = descriptor.production_context;
    value.selection_known = value.selection_open = value.selection_waiting = value.collection_known = true;
    value.collection = binding.collection + 1;
    value.stock_known = value.hands_known = true;
    value.items = {ready_items.data(), ready_items.size()}; value.hands = binding.hands;
    return value;
  }
  ubc::StationRecipeResultObservation ended() const {
    auto value = ready(); value.station = 0; value.production = 0;
    value.selection_open = value.selection_waiting = false; value.selection_done = true;
    value.items = {end_items.data(), end_items.size()};
    value.hands.hands = {{0, -1, -1, 0}, {0, -1, -1, 0}};
    value.hands.interact_item = 0;
    return value;
  }
  ubc::StationRecipeEndWitness witness() const {
    ubc::StationRecipeEndWitness value;
    value.after_original = value.normal_exit_verified = true;
    value.generation = binding.generation; value.sequence = binding.end_sequence + 1;
    value.frame = binding.start_frame + 1;
    value.world = binding.world; value.hero = binding.hero; value.station = binding.station;
    return value;
  }
  template<std::size_t N> bool begin(ubc::StationRecipeResult<N>& result) const {
    return result.begin(descriptor, material, amounts(), binding);
  }
};

void all_completions() {
  for (const auto& descriptor : c::recipes) for (const auto n : {1u, 3u, 7u}) {
    Fixture f(descriptor, n); ubc::StationRecipeResult<> result;
    check(f.material.craftable && f.material.maximum == 7, "actual catalog material contract is craftable");
    check(result.state() == S::idle && result.permits_next(), "idle permits a first job");
    check(f.begin(result) && !result.permits_next(), "all sixteen contracts start with copied material and hand identities");
    check(!f.begin(result), "waiting cannot be replaced or duplicated");
    auto before = f.ended(); before.frame = f.binding.start_frame + 1;
    check(result.observe(before) == S::waiting, "empty station and hands alone do not manufacture an end witness");
    check(result.end_after(f.witness()), "bound normal native after event is accepted exactly once");
    check(!result.end_after(f.witness()), "duplicate native after event cannot count a second tool return");
    check(result.observe(f.ended()) == S::verified && result.permits_next(), "normal end verifies exact end storage and both empty hands");
    check(result.timeout() == S::verified && !result.end_after(f.witness()), "completed job is inert to timeout and duplicate callbacks");
  }
  for (const auto& descriptor : c::recipes) {
    Fixture f(descriptor); ubc::StationRecipeResult<> result; check(f.begin(result), "ready test starts");
    const bool allowed = descriptor.completion == c::Completion::ready_same_station;
    check(result.observe(f.ready()) == (allowed ? S::verified : S::waiting), "only explicit ready completion accepts an open station");
    check(result.permits_next() == allowed, "leather and forge cannot retry before real production end");
  }
}

void invalid_contracts() {
  for (unsigned mode = 0; mode < 41; ++mode) {
    Fixture f(recipe(c::Station::saw)); auto rows = f.amounts();
    switch (mode) {
      case 0: rows.data = nullptr; break;
      case 1: rows.count = 0; break;
      case 2: --rows.count; break;
      case 3: rows.count = std::numeric_limits<std::size_t>::max(); break;
      case 4: std::swap(f.rows[0], f.rows[1]); break;
      case 5: f.rows[1].symbol = f.rows[0].symbol; break;
      case 6: f.rows[0].stored = -1; break;
      case 7: f.rows[0].stored = std::int64_t{INT32_MAX} + 1; break;
      case 8: f.rows[0].expected_ready_storage = -1; break;
      case 9: f.rows[0].expected_end_storage = std::int64_t{INT32_MAX} + 1; break;
      case 10: ++f.rows[0].expected_ready_storage; break;
      case 11: ++f.rows[0].expected_end_storage; break;
      case 12: ++f.rows[0].required; break;
      case 13: ++f.rows[0].produced; break;
      case 14: ++f.rows[0].unit_cost; break;
      case 15: ++f.rows[0].unit_output; break;
      case 16: ++f.rows[0].reserved_tool; break;
      case 17: ++f.rows[0].generated_hand; break;
      case 18: f.rows[0].ready_storage_known = false; break;
      case 19: f.rows[0].end_storage_known = false; break;
      case 20: f.material.quantity = 0; break;
      case 21: f.material.quantity = UINT32_MAX; break;
      case 22: f.material.maximum = 2; break;
      case 23: f.material.maximum = UINT32_MAX; break;
      case 24: f.material.inventory_known = false; break;
      case 25: f.material.hand_known = false; break;
      case 26: f.material.craftable = false; break;
      case 27: f.material.issue = ubc::StationRecipeMaterialIssue::return_capacity; break;
      case 28: f.material.hand_issue = ubc::StationRecipeHandIssue::anchor; break;
      case 29: --f.material.count; break;
      case 30: f.material.completion = c::Completion::end_production; break;
      case 31: f.binding.generation = 0; break;
      case 32: f.binding.world = 0; break;
      case 33: f.binding.hero = 0; break;
      case 34: f.binding.station = 0; break;
      case 35: f.binding.collection = 0; break;
      case 36: f.descriptor.production_context += 1; break;
      case 37: f.descriptor.hand.static_lifecycle_verified = false; break;
      case 38: f.descriptor.hand.right.amount = 2; break;
      case 39: f.descriptor.completion = c::Completion::end_production; break;
      case 40: f.descriptor.targets.data = nullptr; break;
    }
    ubc::StationRecipeResult<> result;
    check(!result.begin(f.descriptor, f.material, rows, f.binding), "malformed material or bound request is rejected without truncation");
    check(result.state() == S::failed && result.issue() == I::contract && !result.permits_next(), "bad contract latches a visible error");
    check(!f.begin(result), "bad contract cannot be retried implicitly with corrected inputs");
  }
  {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<0> result;
    check(!f.begin(result), "zero capacity never accepts a partial material list");
  }
  {
    Fixture f(recipe(c::Station::forge)); ubc::StationRecipeResult<1> result;
    check(!f.begin(result), "capacity smaller than this actual recipe does not hide extra hand targets");
  }
  for (const auto n : {0u, 8u, UINT32_MAX}) {
    Fixture f(recipe(c::Station::saw), n); ubc::StationRecipeResult<> result;
    check(!f.begin(result), "real noncraftable material previews never start an effect result");
  }
}

void binding_hands() {
  for (const auto station : {c::Station::leatherwork, c::Station::saw, c::Station::forge})
    for (unsigned mode = 0; mode < 15; ++mode) {
      Fixture f(recipe(station));
      auto& right = f.binding.hands.hands.right;
      switch (mode) {
        case 0: right.state = -1; break;
        case 1: right.state = 2; break;
        case 2: right.object = 0; break;
        case 3: ++right.symbol; break;
        case 4: ++right.amount; break;
        case 5: f.binding.hands.right_slot_known = false; break;
        case 6: f.binding.hands.right_tmp_level = 9; break;
        case 7: f.binding.hands.hands.left.state = -1; break;
        case 8: f.binding.hands.hands.left.state = 2; break;
        case 9: f.binding.hands.hands.left = right; break;
        case 10: f.binding.hands.interact_item_known = false; break;
        case 11: f.binding.hands.interact_item = 0; break;
        case 12: ++f.binding.hands.interact_item; break;
        case 13: f.binding.hands.hands.left.object = right.object; break;
        case 14: right.amount = 0; break;
      }
      ubc::StationRecipeResult<> result;
      check(!f.begin(result) && result.issue() == I::contract, "both source hands, lifetime and exact interact anchor are required at begin");
    }
  for (unsigned mode = 0; mode < 4; ++mode) {
    Fixture f(recipe(c::Station::forge));
    if (mode == 0) f.binding.hands.left_slot_known = false;
    if (mode == 1) f.binding.hands.left_tmp_level = 0;
    if (mode == 2) f.binding.hands.hands.left.object = 0;
    if (mode == 3) f.binding.hands.interact_item = f.binding.hands.hands.left.object;
    ubc::StationRecipeResult<> result;
    check(!f.begin(result), "forge requires the real left generated prop and exclusively the right anchor");
  }
}

void missing_ready_evidence() {
  for (unsigned mode = 0; mode < 22; ++mode) {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<> result; check(f.begin(result), "incomplete ready case starts");
    auto value = f.ready();
    switch (mode) {
      case 0: value.stable_boundary_verified = false; break;
      case 1: value.identity_known = false; break;
      case 2: value.station_known = false; break;
      case 3: value.production_known = false; break;
      case 4: value.selection_known = false; break;
      case 5: value.selection_open = false; break;
      case 6: value.selection_waiting = false; break;
      case 7: value.selection_done = true; break;
      case 8: value.collection_known = false; break;
      case 9: value.collection = f.binding.collection; break;
      case 10: value.collection = f.binding.collection - 1; break;
      case 11: value.frame = f.binding.start_frame; break;
      case 12: value.generation -= 1; break;
      case 13: value.stock_known = false; break;
      case 14: value.hands_known = false; break;
      case 15: value.hands.hands.left.state = -1; break;
      case 16: value.hands.hands.right.state = -1; break;
      case 17: value.hands.right_slot_known = false; break;
      case 18: value.hands.interact_item_known = false; break;
      case 19: value.station = 0; break;
      case 20: value.production = 0; break;
      case 21: value.station = 0; value.production = 0; break;
    }
    check(result.observe(value) == S::waiting && !result.permits_next(), "unknown or intermediate ready evidence never unlocks a second effect");
    value = f.ready(); ++value.frame;
    check(result.observe(value) == S::verified, "later complete ready evidence can confirm without replaying recipe");
  }
  for (unsigned mode = 0; mode < 9; ++mode) {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<> result; check(f.begin(result), "contradicting ready hand case starts");
    auto value = f.ready();
    switch (mode) {
      case 0: value.hands.hands.left = {1, 999, 1, 0xa000}; break;
      case 1: value.hands.hands.right.state = 0; value.hands.hands.right.object = 0; break;
      case 2: value.hands.hands.right.state = 2; break;
      case 3: ++value.hands.hands.right.symbol; break;
      case 4: ++value.hands.hands.right.amount; break;
      case 5: ++value.hands.hands.right.object; break;
      case 6: value.hands.right_tmp_level = 1; break;
      case 7: value.hands.interact_item = 0; break;
      case 8: ++value.hands.interact_item; break;
    }
    check(result.observe(value) == S::failed && result.issue() == (mode < 7 ? I::hands : I::anchor), "positive changed tool, slot or anchor is not merely an unknown ready state");
  }
}

void native_end_evidence() {
  for (unsigned mode = 0; mode < 10; ++mode) {
    Fixture f(recipe(c::Station::forge)); ubc::StationRecipeResult<> result; check(f.begin(result), "invalid witness case starts");
    auto witness = f.witness();
    switch (mode) {
      case 0: witness.after_original = false; break;
      case 1: witness.normal_exit_verified = false; break;
      case 2: --witness.generation; break;
      case 3: ++witness.generation; break;
      case 4: witness.sequence = f.binding.end_sequence; break;
      case 5: witness.sequence = 0; break;
      case 6: witness.frame = f.binding.start_frame - 1; break;
      case 7: ++witness.world; break;
      case 8: ++witness.hero; break;
      case 9: ++witness.station; break;
    }
    check(!result.end_after(witness) && result.state() == S::waiting, "foreign, old, pre-original or interrupted events cannot create a normal end");
    check(result.end_after(f.witness()) && result.observe(f.ended()) == S::verified, "subsequent exact normal native event and terminal frame can complete");
  }
  for (unsigned mode = 0; mode < 10; ++mode) {
    Fixture f(recipe(c::Station::forge)); ubc::StationRecipeResult<> result;
    check(f.begin(result) && result.end_after(f.witness()), "missing end data case starts");
    auto value = f.ended();
    switch (mode) {
      case 0: value.frame = f.witness().frame; break;
      case 1: value.hands_known = false; break;
      case 2: value.hands.hands.left.state = -1; break;
      case 3: value.hands.hands.right.state = -1; break;
      case 4: value.hands.interact_item_known = false; break;
      case 5: value.stock_known = false; break;
      case 6: value.station = f.binding.station; break;
      case 7: value.production = f.descriptor.production_context; break;
      case 8: value.identity_known = false; break;
      case 9: value.stable_boundary_verified = false; break;
    }
    check(result.observe(value) == S::waiting && !result.permits_next(), "end witness still needs a later stable fully cleared boundary");
    value = f.ended(); ++value.frame;
    check(result.observe(value) == S::verified, "complete final end snapshot eventually verifies once");
  }
  for (const auto station : {c::Station::leatherwork, c::Station::saw, c::Station::forge})
    for (unsigned mode = 0; mode < 6; ++mode) {
      Fixture f(recipe(station)); ubc::StationRecipeResult<> result;
      check(f.begin(result) && result.end_after(f.witness()), "nonempty terminal case starts");
      auto value = f.ended();
      switch (mode) {
        case 0: value.hands.hands.left = {1, 999, 1, 0xb000}; break;
        case 1: value.hands.hands.right = f.binding.hands.hands.right; break;
        case 2: value.hands.hands.left.state = 2; break;
        case 3: value.hands.hands.right.object = 0xb000; break;
        case 4: value.hands.interact_item = f.binding.hands.interact_item; break;
        case 5: value.hands.interact_item = 0xc000; break;
      }
      check(result.observe(value) == S::failed && result.issue() == (mode < 4 ? I::hands : I::anchor), "known leftover hand or interaction anchor blocks normal end completion");
    }
  {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<> result;
    check(f.begin(result) && result.end_after(f.witness()), "end already observed for ready-capable saw");
    check(result.observe(f.ready()) == S::waiting, "native end cannot be downgraded to old ready route to avoid checking tool return");
    auto value = f.ended(); ++value.frame;
    check(result.observe(value) == S::verified, "the saw completes only at its bound end once end was observed");
  }
}

void quantities_and_tools() {
  for (const auto& descriptor : c::recipes) {
    for (unsigned mode = 0; mode < 9; ++mode) {
      Fixture f(descriptor); ubc::StationRecipeResult<> result;
      check(f.begin(result) && result.end_after(f.witness()), "malformed terminal stock case starts");
      auto value = f.ended();
      switch (mode) {
        case 0: value.items.data = nullptr; break;
        case 1: value.items.count = 0; break;
        case 2: --value.items.count; break;
        case 3: value.items.count = std::numeric_limits<std::size_t>::max(); break;
        case 4: f.end_items[0].total = -1; break;
        case 5: f.end_items[0].total = std::int64_t{INT32_MAX} + 1; break;
        case 6: f.end_items[0].symbol = 999; break;
        case 7: f.end_items[1].symbol = f.end_items[0].symbol; break;
        case 8: ++f.end_items.back().total; break;
      }
      check(result.observe(value) == S::failed && result.issue() == I::stock, "no missing, duplicate, foreign or overflowing storage target is implicit success");
    }
    for (const auto& cost : descriptor.costs) for (unsigned mode = 0; mode < 2; ++mode) {
      Fixture f(descriptor); ubc::StationRecipeResult<> result; check(f.begin(result), "partial removal case starts");
      for (auto& item : f.end_items) if (item.symbol == cost.symbol) item.total += mode ? -1 : 1;
      check(result.end_after(f.witness()) && result.observe(f.ended()) == S::failed, "under- and over-consumed ingredients both block further manufacture");
    }
    for (const auto& output : descriptor.outputs) for (unsigned mode = 0; mode < 2; ++mode) {
      Fixture f(descriptor); ubc::StationRecipeResult<> result; check(f.begin(result), "wrong output case starts");
      for (auto& item : f.end_items) if (item.symbol == output.symbol) item.total += mode ? -1 : 1;
      check(result.end_after(f.witness()) && result.observe(f.ended()) == S::failed, "partial output and duplicate output both fail exact conservation");
    }
    Fixture f(descriptor); ubc::StationRecipeResult<> result; check(f.begin(result), "reordered observation starts");
    std::reverse(f.end_items.begin(), f.end_items.end());
    check(result.end_after(f.witness()) && result.observe(f.ended()) == S::verified, "observation target ordering is irrelevant when symbol set and amounts are exact");
  }
  for (const auto stored_tool : {0, 6, INT32_MAX - 1}) {
    Fixture f(recipe(c::Station::saw)); f.set(16609, stored_tool); f.evaluate(3);
    ubc::StationRecipeResult<> ready, ended;
    check(f.begin(ready) && ready.observe(f.ready()) == S::verified, "ready stock excludes the one held saw, including zero extra stored tools");
    check(f.begin(ended) && ended.end_after(f.witness()) && ended.observe(f.ended()) == S::verified, "normal exit returns exactly one saw even at int32 storage boundary");
  }
  for (unsigned mode = 0; mode < 3; ++mode) {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<> result; check(f.begin(result), "wrong tool return case starts");
    if (mode == 0) {
      auto value = f.ready(); value.items = {f.end_items.data(), f.end_items.size()};
      check(result.observe(value) == S::failed && result.issue() == I::stock, "ready plus premature inventory refund would duplicate held saw");
    } else {
      for (auto& item : f.end_items) if (item.symbol == 16609) item.total += mode == 1 ? -1 : 1;
      check(result.end_after(f.witness()) && result.observe(f.ended()) == S::failed, "missing or double saw return cannot match normal-end expectation");
    }
  }
  {
    Fixture f(recipe(c::Station::forge)); f.set(16666, 6); f.evaluate(3);
    ubc::StationRecipeResult<> result; check(f.begin(result), "unrelated stored left tongs are bound independently");
    check(result.end_after(f.witness()) && result.observe(f.ended()) == S::verified, "normal forge cleanup preserves left stored tongs while destroying both generated hands");
  }
  for (const auto& descriptor : c::recipes) {
    Fixture f(descriptor);
    for (const auto& cost : descriptor.costs) f.set(cost.symbol, INT32_MAX);
    for (const auto& output : descriptor.outputs) f.set(output.symbol, 0);
    f.evaluate(0); const auto maximum = f.material.maximum; f.evaluate(maximum);
    ubc::StationRecipeResult<> result;
    check(maximum > 7 && f.material.craftable && f.begin(result), "maximum representable lot count binds without truncating multiplied costs or output");
    check(result.end_after(f.witness()) && result.observe(f.ended()) == S::verified, "large exact output and normal hand cleanup verify without int32 wrap");
  }
  for (const auto station : {c::Station::leatherwork, c::Station::forge}) {
    Fixture f(recipe(station));
    const auto anchor_symbol = station == c::Station::forge ? 16667 : 17342;
    for (auto& row : f.rows) if (row.symbol == anchor_symbol)
      row.stored = row.expected_ready_storage = row.expected_end_storage = 1;
    ubc::StationRecipeResult<> result;
    check(!f.begin(result), "numerically consistent generated-anchor inventory is still outside the normal cleanup contract");
  }
}

void context_lifetime_and_failure() {
  for (unsigned mode = 0; mode < 6; ++mode) {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<> result; check(f.begin(result), "context mismatch case starts");
    auto value = f.ready();
    switch (mode) {
      case 0: ++value.world; break;
      case 1: ++value.hero; break;
      case 2: value.world = 0; break;
      case 3: value.hero = 0; break;
      case 4: ++value.station; break;
      case 5: ++value.production; break;
    }
    value.stock_known = false; value.hands_known = false;
    check(result.observe(value) == S::failed && result.issue() == I::context, "positive changed identity fails even when stock and hands are unreadable");
    check(!f.begin(result) && result.observe(f.ready()) == S::failed, "later matching context never retries a failed original effect");
  }
  for (unsigned mode = 0; mode < 2; ++mode) {
    Fixture f(recipe(c::Station::forge)); ubc::StationRecipeResult<> result;
    check(f.begin(result) && result.end_after(f.witness()), "mixed known and unknown terminal hands starts");
    auto value = f.ended();
    if (mode == 0) { value.hands.hands.left.state = -1; value.hands.hands.right = f.binding.hands.hands.right; }
    else { value.hands.hands.left = f.binding.hands.hands.left; value.hands.hands.right.state = -1; }
    check(result.observe(value) == S::failed && result.issue() == I::hands, "a known leftover hand is not masked by the other unreadable hand");
  }
  {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<> result; check(f.begin(result), "stale job and frame case starts");
    auto value = f.ready(); --value.generation; value.world = 999; value.frame += 100;
    check(result.observe(value) == S::waiting, "old generation cannot fail the current job or advance its clock");
    value = f.ready(); value.collection_known = false; value.frame += 1;
    check(result.observe(value) == S::waiting, "unknown new frame remains waiting");
    check(result.observe(f.ready()) == S::waiting && !result.end_after(f.witness()), "old frame and old callback after a later boundary cannot confirm");
    value = f.ready(); value.frame += 2;
    check(result.observe(value) == S::verified, "newer complete frame confirms without a second script effect");
  }
  {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<> result; check(f.begin(result), "future generation case starts");
    auto value = f.ready(); ++value.generation; value.stable_boundary_verified = false;
    check(result.observe(value) == S::failed && result.issue() == I::contract, "overlapping future job is invalid even outside a stable boundary");
  }
  {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<> result; check(f.begin(result), "owned snapshot case starts");
    auto value = f.ready();
    f.rows.clear(); f.material = {}; f.binding = {}; f.descriptor.production_context = 999;
    check(result.observe(value) == S::verified, "result retains only owned values, not descriptor, row or binding references");
  }
  for (unsigned mode = 0; mode < 3; ++mode) {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<> result;
    check(f.begin(result) && result.observe(f.ready()) == S::verified, "new job generation prerequisite verifies");
    f.binding.start_frame = f.ready().frame;
    if (mode == 0) ++f.binding.generation;
    if (mode == 1) --f.binding.generation;
    if (mode == 2) { ++f.binding.generation; --f.binding.start_frame; }
    check(f.begin(result) == (mode == 0), "only strictly new job generation with nondecreasing start frame may follow success");
  }
  for (unsigned mode = 0; mode < 3; ++mode) {
    Fixture f(recipe(c::Station::saw)); ubc::StationRecipeResult<> result; check(f.begin(result), "timeout or restore error case starts");
    if (mode == 0) check(result.timeout() == S::failed && result.issue() == I::timeout, "unproven completion times out instead of replaying original");
    if (mode == 1) check(result.fail(I::restore) == S::failed && result.issue() == I::restore, "byte restore error is independent of matching material output");
    if (mode == 2) check(result.fail(I::none) == S::failed && result.issue() == I::contract, "unclassified external failure is fail-closed");
    const auto issue = result.issue();
    check(!result.permits_next() && !f.begin(result) && !result.end_after(f.witness()) && result.observe(f.ready()) == S::failed,
          "failure cannot be cleared by retries, later callbacks or perfect counts");
    check(result.fail(I::hands) == S::failed && result.issue() == issue, "first failure remains available to user-facing diagnosis");
  }
}
} // namespace

int main() {
  all_completions();
  invalid_contracts();
  binding_hands();
  missing_ready_evidence();
  native_end_evidence();
  quantities_and_tools();
  context_lifetime_and_failure();
  std::printf("station recipe result: %u checks, %u failures\n", checks, failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
