// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/engine/hand_snapshot.hpp"
#include "ubc/catalogs/recipe_catalog.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ubc {

enum class RecipeResultState { idle, waiting, verified, failed };
enum class RecipeResultIssue { none, contract, context, stock, hands, timeout, restore };

struct RecipeResultItem {
  std::int32_t symbol = -1;
  std::int64_t total = -1; // Only stored stock; no hand reserve.
};

struct RecipeResultBinding {
  std::uint64_t generation = 0, start_frame = 0, collection = 0, end_sequence = 0;
  std::uint32_t world = 0, hero = 0, station = 0;
  // For the stove, use the exact active tool hand bound when the order was
  // placed. Compare the object identity only. Never dereference it later.
  HandObjectSnapshot ready_right;
};

struct RecipeEndWitness {
  // Must come only from the matching EndInteraction AFTER hook, after
  // its original call returned. Callback return is not a substitute.
  bool after_original = false;
  std::uint64_t generation = 0, sequence = 0, frame = 0;
  std::uint32_t world = 0, hero = 0, station = 0;
};

struct RecipeResultObservation {
  std::uint64_t generation = 0, frame = 0;
  bool stable_boundary_verified = false;
  bool identity_known = false;
  std::uint32_t world = 0, hero = 0;
  bool station_known = false;
  std::uint32_t station = 0;
  bool production_known = false;
  std::int32_t production = 0;
  bool selection_known = false, selection_open = false;
  bool selection_waiting = false, selection_done = false;
  bool collection_known = false;
  std::uint64_t collection = 0;
  // true only for a complete, unaltered stock snapshot at the
  // confirmed stable point. Unreadable is false, never invented zero.
  bool stock_known = false;
  recipe_catalog::View<RecipeResultItem> items;
  bool hands_known = false;
  HandsSnapshot hands;
};

// No engine calls and no automatic retry. The adapter must serialize
// this instance on a thread. Generation, Frame, Collection, and End
// event sequences are monotone counters without wraparound. Views show
// only during the respective call onto valid own C++ objects. begin
// copies the entire contract; observe retains no View/Choice pointer. A
// matching end quantity alone proves neither learning rules nor script
// restore: their own gates remain required; an error can fail() lock.
template <std::size_t Capacity = recipe_catalog::catalog_target_capacity> class RecipeResult {
public:
  bool begin(const recipe_catalog::Descriptor& recipe,
             recipe_catalog::View<RecipeResultItem> expected,
             const RecipeResultBinding& binding) noexcept {
    if (state_ == RecipeResultState::waiting || state_ == RecipeResultState::failed)
      return false;
    if (!binding.generation || binding.generation <= binding_.generation ||
        binding.start_frame < last_frame_ || !binding.world || !binding.hero || !binding.station ||
        !expected.data || !expected.count || expected.count > Capacity ||
        expected.count != recipe.targets.count || !recipe_catalog::structurally_valid(recipe))
      return bad_contract();
    if (recipe.station == recipe_catalog::Station::stove &&
        (!binding.collection || binding.ready_right.state != 1 ||
         binding.ready_right.symbol != recipe.hand.active_item_symbol ||
         binding.ready_right.amount != static_cast<std::int32_t>(recipe.hand.amount) ||
         !binding.ready_right.object))
      return bad_contract();
    // The own order has exactly the Descriptor order. Observations
    // are instead matched by symbol identity, not index.
    for (std::size_t i = 0; i < expected.count; ++i)
      if (expected[i].symbol != recipe.targets[i].symbol || !valid_total(expected[i].total))
        return bad_contract();
    for (std::size_t i = 0; i < expected.count; ++i)
      expected_[i] = expected[i];
    count_ = expected.count;
    binding_ = binding;
    station_type_ = recipe.station;
    production_ = recipe.production_context;
    last_frame_ = binding.start_frame;
    has_end_ = false;
    end_frame_ = 0;
    state_ = RecipeResultState::waiting;
    issue_ = RecipeResultIssue::none;
    return true;
  }

  bool end_after(const RecipeEndWitness& value) noexcept {
    if (state_ != RecipeResultState::waiting || has_end_ || !value.after_original ||
        value.generation != binding_.generation || value.sequence <= binding_.end_sequence ||
        value.frame < last_frame_ || value.world != binding_.world || value.hero != binding_.hero ||
        value.station != binding_.station)
      return false;
    has_end_ = true;
    end_frame_ = value.frame;
    return true;
  }

  RecipeResultState observe(const RecipeResultObservation& value) noexcept {
    if (state_ != RecipeResultState::waiting)
      return state_;
    // Stale jobs/frames must neither confirm nor contradict current
    // identity. A future own job during waiting is an error.
    if (value.generation < binding_.generation)
      return state_;
    if (value.generation != binding_.generation)
      return fail(RecipeResultIssue::contract);
    if (!value.stable_boundary_verified || value.frame <= binding_.start_frame ||
        value.frame < last_frame_)
      return state_;
    last_frame_ = value.frame;

    // Positive contradictions remain errors, even if another read part
    // is unclear. With identity_known, a null identity means a confirmed
    // end of this context; a read error must yield identity_known=false.
    if (value.identity_known && (value.world != binding_.world || value.hero != binding_.hero))
      return fail(RecipeResultIssue::context);
    if (value.station_known && value.station && value.station != binding_.station)
      return fail(RecipeResultIssue::context);
    if (value.production_known && value.production && value.production != production_)
      return fail(RecipeResultIssue::context);
    if (!value.identity_known || !value.station_known || !value.production_known)
      return state_;

    const bool ended = has_end_ && value.frame >= end_frame_ && !value.station && !value.production;
    const bool ready = !has_end_ && station_type_ == recipe_catalog::Station::stove &&
                       value.station == binding_.station && value.production == production_ &&
                       value.selection_known && value.selection_open && value.selection_waiting &&
                       !value.selection_done && value.collection_known &&
                       value.collection > binding_.collection;
    if (!ended && !ready)
      return state_;

    if (value.stock_known && !matching_stock(value.items))
      return fail(RecipeResultIssue::stock);
    const auto hands = value.hands_known ? matching_hands(value.hands, ended) : Match::unknown;
    if (hands == Match::different)
      return fail(RecipeResultIssue::hands);
    if (!value.stock_known || hands != Match::same)
      return state_;
    state_ = RecipeResultState::verified;
    return state_;
  }

  RecipeResultState timeout() noexcept {
    return state_ == RecipeResultState::waiting ? fail(RecipeResultIssue::timeout) : state_;
  }
  RecipeResultState fail(RecipeResultIssue issue) noexcept {
    if (state_ != RecipeResultState::failed) {
      issue_ = issue == RecipeResultIssue::none ? RecipeResultIssue::contract : issue;
      state_ = RecipeResultState::failed;
    }
    return state_;
  }
  RecipeResultState state() const noexcept { return state_; }
  RecipeResultIssue issue() const noexcept { return issue_; }
  bool permits_next() const noexcept {
    return state_ == RecipeResultState::idle || state_ == RecipeResultState::verified;
  }

private:
  enum class Match { unknown, same, different };
  static bool valid_total(std::int64_t total) noexcept {
    return total >= 0 && total <= std::numeric_limits<std::int32_t>::max();
  }
  bool bad_contract() noexcept {
    fail(RecipeResultIssue::contract);
    return false;
  }
  bool matching_stock(recipe_catalog::View<RecipeResultItem> values) const noexcept {
    if (!values.data || values.count != count_)
      return false;
    // count_ is checked against the own array capacity. Missing,
    // duplicate, and foreign symbols are never implicit zero stocks.
    for (std::size_t i = 0; i < count_; ++i) {
      if (!valid_total(values[i].total))
        return false;
      for (std::size_t j = 0; j < i; ++j)
        if (values[j].symbol == values[i].symbol)
          return false;
      bool found = false;
      for (std::size_t j = 0; j < count_; ++j)
        if (values[i].symbol == expected_[j].symbol) {
          if (values[i].total != expected_[j].total)
            return false;
          found = true;
          break;
        }
      if (!found)
        return false;
    }
    return true;
  }
  static Match empty_hand(const HandObjectSnapshot& hand) noexcept {
    if (hand.state == -1)
      return Match::unknown;
    return hand.state == 0 && !hand.object ? Match::same : Match::different;
  }
  Match matching_hands(const HandsSnapshot& hands, bool ended) const noexcept {
    const auto left = empty_hand(hands.left);
    Match right = Match::unknown;
    if (ended)
      right = empty_hand(hands.right);
    else if (hands.right.state != -1)
      right = hands.right.state == 1 && hands.right.symbol == binding_.ready_right.symbol &&
                      hands.right.amount == binding_.ready_right.amount &&
                      hands.right.object == binding_.ready_right.object
                  ? Match::same
                  : Match::different;
    if (left == Match::different || right == Match::different)
      return Match::different;
    return left == Match::same && right == Match::same ? Match::same : Match::unknown;
  }

  RecipeResultState state_ = RecipeResultState::idle;
  RecipeResultIssue issue_ = RecipeResultIssue::none;
  RecipeResultBinding binding_;
  recipe_catalog::Station station_type_ = recipe_catalog::Station::stove;
  std::int32_t production_ = 0;
  std::array<RecipeResultItem, Capacity> expected_{};
  std::size_t count_ = 0;
  std::uint64_t last_frame_ = 0, end_frame_ = 0;
  bool has_end_ = false;
};

} // namespace ubc
