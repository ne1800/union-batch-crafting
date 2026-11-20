// SPDX-License-Identifier: MIT
#include "ubc/core/recipe_result.hpp"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <vector>

namespace {
namespace c = ubc::recipe_catalog;
using S = ubc::RecipeResultState;
using I = ubc::RecipeResultIssue;
unsigned checks = 0, failures = 0;
void check(bool value, const char* message) {
  ++checks;
  if (!value) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", message);
  }
}

struct Fixture {
  c::Descriptor descriptor;
  std::vector<ubc::RecipeResultItem> expected, observed;
  ubc::RecipeResultBinding binding;
  explicit Fixture(const c::Descriptor& recipe) : descriptor(recipe) {
    for (std::size_t i = 0; i < recipe.targets.count; ++i)
      expected.push_back({recipe.targets[i].symbol, static_cast<std::int64_t>(10 + i)});
    observed = expected;
    binding.generation = 7;
    binding.start_frame = 100;
    binding.collection = 20;
    binding.end_sequence = 4;
    binding.world = 1;
    binding.hero = 2;
    binding.station = 3;
    binding.ready_right = {1, recipe.hand.active_item_symbol,
                           static_cast<std::int32_t>(recipe.hand.amount), 0x88000};
  }
  c::View<ubc::RecipeResultItem> amounts() const { return {expected.data(), expected.size()}; }
  ubc::RecipeResultObservation ready() const {
    ubc::RecipeResultObservation value;
    value.generation = binding.generation;
    value.frame = binding.start_frame + 1;
    value.stable_boundary_verified = value.identity_known = value.station_known = true;
    value.world = binding.world;
    value.hero = binding.hero;
    value.station = binding.station;
    value.production_known = true;
    value.production = descriptor.production_context;
    value.selection_known = value.selection_open = value.selection_waiting = true;
    value.collection_known = true;
    value.collection = binding.collection + 1;
    value.stock_known = value.hands_known = true;
    value.items = {observed.data(), observed.size()};
    value.hands = {{0, -1, -1, 0}, binding.ready_right};
    return value;
  }
  ubc::RecipeResultObservation ended() const {
    auto value = ready();
    value.frame += 1;
    value.station = 0;
    value.production = 0;
    value.selection_open = value.selection_waiting = false;
    value.selection_done = true;
    value.hands.right = {0, -1, -1, 0};
    return value;
  }
  ubc::RecipeEndWitness witness() const {
    return {true,          binding.generation, binding.end_sequence + 1, binding.start_frame + 1,
            binding.world, binding.hero,       binding.station};
  }
};
}

int main() {
  const auto& meat = *c::find(c::RecipeId::meat);
  const auto& health = *c::find(c::RecipeId::health_01);
  for (const auto& recipe : c::recipes) {
    Fixture f(recipe);
    ubc::RecipeResult<> result;
    check(result.state() == S::idle && result.permits_next(), "idle permits one new job");
    check(result.begin(recipe, f.amounts(), f.binding),
          "all nine descriptors bind their exact item list");
    check(result.state() == S::waiting && !result.permits_next(),
          "started job blocks further production");
    check(!result.begin(recipe, f.amounts(), f.binding), "reentry cannot replace a waiting job");
    auto value = f.ready();
    value.collection = f.binding.collection;
    check(result.observe(value) == S::waiting, "old selection is not a completed production");
    value = f.ended();
    check(result.observe(value) == S::waiting,
          "Mob0 and empty hands without after-original witness do not prove completion");
    auto witness = f.witness();
    witness.frame = value.frame;
    check(result.end_after(witness), "matching fresh after-original event is recorded");
    check(result.observe(value) == S::verified && result.permits_next(),
          "matching terminal end verifies every descriptor");
    check(result.observe(f.ready()) == S::verified,
          "verified result does not become a new pending job");
    check(!result.end_after(witness), "completed job ignores later end events");
    check(result.timeout() == S::verified, "timeout cannot undo a verified job");
  }
  for (const auto& recipe : c::recipes) {
    Fixture f(recipe);
    ubc::RecipeResult<> result;
    check(result.begin(recipe, f.amounts(), f.binding), "ready variant starts");
    check(result.observe(f.ready()) ==
              (recipe.station == c::Station::stove ? S::verified : S::waiting),
          "only stove descriptors can complete at the fresh ready selection");
  }
  {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    check(result.begin(f.descriptor, f.amounts(), f.binding), "copy contract starts");
    auto value = f.ready();
    f.expected.assign(f.expected.size(), {999, 0});
    f.descriptor.production_context = 999;
    f.descriptor.station = c::Station::alchemy;
    f.binding.ready_right.object = 999;
    check(result.observe(value) == S::verified,
          "job owns all expected amounts and relevant descriptor/binding values");
  }
  for (unsigned variant = 0; variant < 17; ++variant) {
    Fixture f(meat);
    auto expected = f.amounts();
    auto binding = f.binding;
    switch (variant) {
    case 0:
      expected.data = nullptr;
      break;
    case 1:
      expected.count = 0;
      break;
    case 2:
      expected.count -= 1;
      break;
    case 3:
      expected.count = std::numeric_limits<std::size_t>::max();
      break;
    case 4:
      std::swap(f.expected[0], f.expected[1]);
      break;
    case 5:
      f.expected[1].symbol = f.expected[0].symbol;
      break;
    case 6:
      f.expected[0].total = -1;
      break;
    case 7:
      f.expected[0].total = static_cast<std::int64_t>(INT32_MAX) + 1;
      break;
    case 8:
      binding.generation = 0;
      break;
    case 9:
      binding.world = 0;
      break;
    case 10:
      binding.hero = 0;
      break;
    case 11:
      binding.station = 0;
      break;
    case 12:
      binding.collection = 0;
      break;
    case 13:
      binding.ready_right.state = 2;
      break;
    case 14:
      binding.ready_right.symbol = 16604;
      break;
    case 15:
      binding.ready_right.amount = 2;
      break;
    case 16:
      binding.ready_right.object = 0;
      break;
    }
    ubc::RecipeResult<> result;
    check(!result.begin(f.descriptor, expected, binding),
          "malformed job contract is rejected before being stored");
    check(result.state() == S::failed && result.issue() == I::contract && !result.permits_next(),
          "contract error is explicit and cannot auto-retry");
  }
  {
    Fixture f(meat);
    f.descriptor.hand.amount = 2;
    ubc::RecipeResult<> result;
    check(!result.begin(f.descriptor, f.amounts(), f.binding),
          "invalid descriptor cannot create a completion contract");
    ubc::RecipeResult<0> empty;
    check(!empty.begin(meat, f.amounts(), f.binding),
          "zero capacity never truncates an expected list");
  }
  for (unsigned variant = 0; variant < 12; ++variant) {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    check(result.begin(meat, f.amounts(), f.binding), "incomplete ready observation starts");
    auto value = f.ready();
    switch (variant) {
    case 0:
      value.stable_boundary_verified = false;
      break;
    case 1:
      value.identity_known = false;
      break;
    case 2:
      value.station_known = false;
      break;
    case 3:
      value.production_known = false;
      break;
    case 4:
      value.selection_known = false;
      break;
    case 5:
      value.selection_open = false;
      break;
    case 6:
      value.selection_waiting = false;
      break;
    case 7:
      value.selection_done = true;
      break;
    case 8:
      value.collection_known = false;
      break;
    case 9:
      value.collection = f.binding.collection;
      break;
    case 10:
      value.frame = f.binding.start_frame;
      break;
    case 11:
      value.production = 0;
      break;
    }
    check(result.observe(value) == S::waiting, "missing or transitional terminal evidence waits");
    check(!result.permits_next(), "waiting is not implicit success or retry authorization");
    check(result.observe(f.ready()) == S::verified, "later complete stable terminal can verify");
  }
  for (unsigned variant = 0; variant < 9; ++variant) {
    Fixture f(health);
    ubc::RecipeResult<> result;
    check(result.begin(health, f.amounts(), f.binding), "bad end witness case starts");
    auto witness = f.witness();
    switch (variant) {
    case 0:
      witness.after_original = false;
      break;
    case 1:
      witness.generation -= 1;
      break;
    case 2:
      witness.generation += 1;
      break;
    case 3:
      witness.sequence = f.binding.end_sequence;
      break;
    case 4:
      witness.frame = f.binding.start_frame - 1;
      break;
    case 5:
      witness.world += 1;
      break;
    case 6:
      witness.hero += 1;
      break;
    case 7:
      witness.station += 1;
      break;
    case 8:
      witness.station = 0;
      break;
    }
    check(!result.end_after(witness),
          "stale, foreign, or before-original event is not an own end witness");
    check(result.observe(f.ended()) == S::waiting,
          "callback return or unrelated event cannot confirm an old flask count");
  }
  {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    check(result.begin(meat, f.amounts(), f.binding) && result.end_after(f.witness()),
          "own end event starts terminal transition");
    check(result.observe(f.ready()) == S::waiting,
          "after end event a new ready selection cannot substitute actual interaction end");
    check(result.observe(f.ended()) == S::verified, "actual empty-hand terminal still completes");
  }
  {
    Fixture f(health);
    ubc::RecipeResult<> result;
    check(result.begin(health, f.amounts(), f.binding), "future end event starts");
    auto witness = f.witness();
    witness.frame = 110;
    check(result.end_after(witness), "after-original event carries its own frame");
    check(result.observe(f.ended()) == S::waiting, "snapshot preceding end event cannot complete");
    auto value = f.ended();
    value.frame = 110;
    check(result.observe(value) == S::verified,
          "same frame stable after-original snapshot can complete");
  }
  {
    Fixture f(health);
    ubc::RecipeResult<> result;
    check(result.begin(health, f.amounts(), f.binding), "dispatch-frame end event starts");
    auto witness = f.witness();
    witness.frame = f.binding.start_frame;
    check(result.end_after(witness), "synchronous end can occur in the original dispatch frame");
    auto value = f.ended();
    value.frame = f.binding.start_frame;
    check(result.observe(value) == S::waiting,
          "dispatch-frame snapshots still cannot stand in for the stable follow-up frame");
    check(result.observe(f.ended()) == S::verified,
          "the subsequent stable frame can consume the earlier own end witness");
  }
  for (unsigned variant = 0; variant < 7; ++variant) {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    check(result.begin(meat, f.amounts(), f.binding), "context change case starts");
    auto value = f.ready();
    value.stock_known = value.hands_known = value.selection_known = false;
    switch (variant) {
    case 0:
      value.world += 1;
      break;
    case 1:
      value.hero += 1;
      break;
    case 2:
      value.world = 0;
      break;
    case 3:
      value.hero = 0;
      break;
    case 4:
      value.station += 1;
      break;
    case 5:
      value.production = 4;
      break;
    case 6:
      value.production = -1;
      break;
    }
    check(result.observe(value) == S::failed && result.issue() == I::context,
          "positive context change fails even while other result fields are unknown");
    check(!result.begin(meat, f.amounts(), f.binding),
          "context failure cannot automatically restart");
    check(result.observe(f.ready()) == S::failed, "later matching values cannot erase a failure");
  }
  for (const bool ended : {false, true})
    for (unsigned variant = 0; variant < 9; ++variant) {
      Fixture f(ended ? health : meat);
      ubc::RecipeResult<> result;
      check(result.begin(f.descriptor, f.amounts(), f.binding), "stock check starts");
      if (ended)
        check(result.end_after(f.witness()), "stock check gets explicit end witness");
      auto value = ended ? f.ended() : f.ready();
      value.hands_known = false;
      switch (variant) {
      case 0:
        f.observed[0].total += 1;
        break;
      case 1:
        f.observed[0].total = -1;
        break;
      case 2:
        f.observed[0].total = static_cast<std::int64_t>(INT32_MAX) + 1;
        break;
      case 3:
        f.observed[0].symbol = 99999;
        break;
      case 4:
        f.observed[1].symbol = f.observed[0].symbol;
        break;
      case 5:
        value.items.count -= 1;
        break;
      case 6:
        value.items.count += 1;
        break;
      case 7:
        value.items.count = std::numeric_limits<std::size_t>::max();
        break;
      case 8:
        value.items.data = nullptr;
        break;
      }
      check(result.observe(value) == S::failed && result.issue() == I::stock,
            "known wrong stock or symbol mapping fails independently of unreadable hands");
    }
  {
    Fixture f(health);
    ubc::RecipeResult<> result;
    check(result.begin(health, f.amounts(), f.binding) && result.end_after(f.witness()),
          "symbol remapping case starts");
    std::reverse(f.observed.begin(), f.observed.end());
    check(result.observe(f.ended()) == S::verified,
          "observation order is irrelevant when every symbol and amount matches");
  }
  {
    Fixture f(health);
    ubc::RecipeResult<> result;
    check(result.begin(health, f.amounts(), f.binding) && result.end_after(f.witness()),
          "foreign recipe case starts");
    const auto& other = *c::find(c::RecipeId::mana_01);
    for (std::size_t i = 0; i < f.observed.size(); ++i)
      f.observed[i].symbol = other.targets[i].symbol;
    check(result.observe(f.ended()) == S::failed && result.issue() == I::stock,
          "another recipe with the same number of equal totals never completes this job");
  }
  for (unsigned variant = 0; variant < 8; ++variant) {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    check(result.begin(meat, f.amounts(), f.binding), "wrong ready hand case starts");
    auto value = f.ready();
    value.stock_known = false;
    switch (variant) {
    case 0:
      value.hands.left = {1, 16604, 1, 0x99000};
      break;
    case 1:
      value.hands.left = {2, -1, -1, 0x99000};
      break;
    case 2:
      value.hands.left.object = 0x99000;
      break;
    case 3:
      value.hands.right.state = 0;
      break;
    case 4:
      value.hands.right.state = 2;
      break;
    case 5:
      value.hands.right.symbol = 16604;
      break;
    case 6:
      value.hands.right.amount = 2;
      break;
    case 7:
      value.hands.right.object += 1;
      break;
    }
    check(result.observe(value) == S::failed && result.issue() == I::hands,
          "wrong ready hand fails even when stock is unknown");
  }
  {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    check(result.begin(meat, f.amounts(), f.binding), "partially unreadable hand case starts");
    auto value = f.ready();
    value.hands.left.state = -1;
    value.hands.right.amount = 2;
    check(result.observe(value) == S::failed && result.issue() == I::hands,
          "unknown left hand does not hide a positively wrong right hand");
  }
  for (unsigned variant = 0; variant < 5; ++variant) {
    Fixture f(health);
    ubc::RecipeResult<> result;
    check(result.begin(health, f.amounts(), f.binding) && result.end_after(f.witness()),
          "wrong ended hand case starts");
    auto value = f.ended();
    value.stock_known = false;
    switch (variant) {
    case 0:
      value.hands.right = f.binding.ready_right;
      break;
    case 1:
      value.hands.right = {2, -1, -1, 0x99000};
      break;
    case 2:
      value.hands.right.object = 0x99000;
      break;
    case 3:
      value.hands.left = {1, 16604, 1, 0x99000};
      break;
    case 4:
      value.hands.left.state = 2;
      break;
    }
    check(result.observe(value) == S::failed && result.issue() == I::hands,
          "old flask or occupied unknown hand is not a completed alchemy cycle");
  }
  for (const bool ended : {false, true})
    for (unsigned variant = 0; variant < 4; ++variant) {
      Fixture f(ended ? health : meat);
      ubc::RecipeResult<> result;
      check(result.begin(f.descriptor, f.amounts(), f.binding), "unknown result field case starts");
      if (ended)
        check(result.end_after(f.witness()), "unknown end case has lifecycle witness");
      auto value = ended ? f.ended() : f.ready();
      switch (variant) {
      case 0:
        value.stock_known = false;
        value.items = {};
        break;
      case 1:
        value.hands_known = false;
        break;
      case 2:
        value.hands.left.state = -1;
        break;
      case 3:
        value.hands.right.state = -1;
        break;
      }
      check(result.observe(value) == S::waiting,
            "unreadable result components wait without inventing zero or emptiness");
      check(result.timeout() == S::failed && result.issue() == I::timeout,
            "explicit timeout closes a still unknown result as failed");
      check(!result.permits_next(), "timeout never schedules a retry");
    }
  {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    check(result.begin(meat, f.amounts(), f.binding), "frame and generation case starts");
    auto value = f.ready();
    value.generation -= 1;
    value.world = 999;
    check(result.observe(value) == S::waiting, "old job observation cannot poison the new context");
    value = f.ready();
    value.frame = 110;
    value.stock_known = false;
    check(result.observe(value) == S::waiting, "incomplete fresh frame remains pending");
    check(result.observe(f.ready()) == S::waiting,
          "older complete frame cannot finish after a newer observation");
    check(!result.end_after(f.witness()),
          "delayed earlier end event cannot complete a newer observed state");
    value = f.ready();
    value.frame = 110;
    check(result.observe(value) == S::verified,
          "complete same-frame stable observation remains admissible");
    check(!result.begin(meat, f.amounts(), f.binding) && result.issue() == I::contract,
          "generation replay cannot create a second execution contract");
  }
  {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    check(result.begin(meat, f.amounts(), f.binding) && result.observe(f.ready()) == S::verified,
          "first independent job completes");
    f.binding.generation += 1;
    f.binding.start_frame += 1;
    f.binding.collection += 1;
    check(result.begin(meat, f.amounts(), f.binding),
          "later job requires a fresh explicit generation");
    check(result.observe(f.ready()) == S::verified, "fresh independent job can also complete once");
    result.fail(I::restore);
    check(result.state() == S::failed && result.issue() == I::restore && !result.permits_next(),
          "explicit restoration failure is permanent even after material observation");
    check(result.timeout() == S::failed && result.issue() == I::restore,
          "timeout preserves the original failure cause");
  }
  {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    check(result.begin(meat, f.amounts(), f.binding), "unexpected generation case starts");
    auto value = f.ready();
    value.generation += 1;
    check(result.observe(value) == S::failed && result.issue() == I::contract,
          "a future own generation cannot silently displace a pending result");
  }
  for (const bool station_change : {false, true}) {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    check(result.begin(meat, f.amounts(), f.binding), "partial context read case starts");
    auto value = f.ready();
    value.identity_known = false;
    if (station_change)
      value.station += 1;
    else
      value.production = 4;
    check(result.observe(value) == S::failed && result.issue() == I::context,
          "unreadable identity does not hide a separately proved station or production change");
  }
  {
    Fixture f(meat);
    ubc::RecipeResult<> result;
    for (std::size_t i = 0; i < f.expected.size(); ++i)
      f.expected[i].total = i % 2 ? INT32_MAX : 0;
    f.observed = f.expected;
    f.binding.generation = std::uint64_t{1} << 40;
    f.binding.collection = std::uint64_t{1} << 40;
    f.binding.start_frame = std::uint64_t{1} << 40;
    check(result.begin(meat, f.amounts(), f.binding),
          "zero/int32 maximum totals and wide sequence values bind exactly");
    check(result.observe(f.ready()) == S::verified,
          "no total or sequence truncation during completion");
  }
  {
    Fixture f(health);
    ubc::RecipeResult<> result;
    f.binding.end_sequence = std::numeric_limits<std::uint64_t>::max();
    check(result.begin(health, f.amounts(), f.binding), "exhausted end sequence remains explicit");
    auto witness = f.witness();
    witness.sequence = 0;
    check(!result.end_after(witness) && result.observe(f.ended()) == S::waiting,
          "wrapped sequence can never turn an old event into a fresh end witness");
    check(result.timeout() == S::failed,
          "sequence exhaustion fails by explicit timeout instead of wraparound success");
  }
  {
    std::vector<ubc::PackedInventoryTarget> targets(meat.targets.begin(), meat.targets.end());
    std::vector<ubc::lhiver::SymbolBinding> bindings(meat.bindings.begin(), meat.bindings.end());
    const char* names[] = {"SYNTHETIC_A", "SYNTHETIC_B", "SYNTHETIC_C", "SYNTHETIC_D"};
    for (unsigned i = 0; i < 4; ++i) {
      targets.push_back({names[i], static_cast<std::int32_t>(30000 + i)});
      bindings.push_back({names[i], 30000 + i, 7, 1, 0, 1584});
    }
    auto descriptor = meat;
    descriptor.targets = {targets.data(), targets.size()};
    descriptor.bindings = {bindings.data(), bindings.size()};
    Fixture f(descriptor);
    ubc::RecipeResult<8> result;
    check(c::structurally_valid(descriptor),
          "own synthetic eight-target descriptor is structurally valid");
    check(result.begin(descriptor, f.amounts(), f.binding),
          "capacity is explicit and does not inherit the old six-target contract");
    std::reverse(f.observed.begin(), f.observed.end());
    check(result.observe(f.ready()) == S::verified, "all eight symbols are independently checked");
    ubc::RecipeResult<7> too_short;
    check(!too_short.begin(descriptor, f.amounts(), f.binding),
          "insufficient capacity fails instead of dropping a target");
  }
  std::printf("RecipeResult: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
