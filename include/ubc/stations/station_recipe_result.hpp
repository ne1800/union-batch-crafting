// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/stations/station_recipe_material.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ubc {

enum class StationRecipeResultState { idle, waiting, verified, failed };
enum class StationRecipeResultIssue {
  none,
  contract,
  context,
  stock,
  hands,
  anchor,
  timeout,
  restore
};
struct StationRecipeResultItem {
  std::int32_t symbol = -1;
  std::int64_t total = -1; // Only store, neither reserved tool nor produced requisite.
};
struct StationRecipeResultBinding {
  // Bound immediately before the single effect, not before the owner navigation.
  std::uint64_t generation = 0, start_frame = 0, collection = 0, end_sequence = 0;
  std::uint32_t world = 0, hero = 0, station = 0;
  StationRecipeHands hands;
};
struct StationRecipeEndWitness {
  // AFTER an exactly matched, returned native EndInteraction.
  // normal_exit_verified requires the regular station exit; Interrupt, Stop,
  // Script EndProduction, or Recipe-Callback-Return alone are insufficient.
  bool after_original = false, normal_exit_verified = false;
  std::uint64_t generation = 0, sequence = 0, frame = 0;
  std::uint32_t world = 0, hero = 0, station = 0;
};
struct StationRecipeResultObservation {
  std::uint64_t generation = 0, frame = 0;
  bool stable_boundary_verified = false;
  bool identity_known = false;
  std::uint32_t world = 0, hero = 0;
  bool station_known = false;
  std::uint32_t station = 0;
  bool production_known = false;
  std::int32_t production = 0;
  // Exclusively the same own station dialog with a completely new
  // Collection. A foreign NPC Collection is not a Ready completion proof.
  bool selection_known = false, selection_open = false, selection_waiting = false,
       selection_done = false;
  bool collection_known = false;
  std::uint64_t collection = 0;
  // Complete, unaltered, and alias-checked record at this stable
  // point. Unreadability is unknown, never an invented zero stock.
  bool stock_known = false;
  station_recipe_catalog::View<StationRecipeResultItem> items;
  bool hands_known = false;
  StationRecipeHands hands;
};

// Pure value core, no engine action, compensation, or repetition. The
// adapter serializes all calls on a single thread and binds material,
// hands, SELF==HERO (if required), parser, and native station separately
// before the effect. A result proves neither these releases nor the
// byte-restore; fail(restore) remains therefore its own hard error path.
//
// begin copies all required values; no descriptor, view, or engine object
// is retained/dereferenced. Own views are only valid during the call.
// All counters are monotonic without wraparound. Unknown remains waiting
// until positive proof or explicit timeout(); failed has no reset.
template <std::size_t Capacity = station_recipe_catalog::catalog_target_capacity>
class StationRecipeResult {
public:
  bool begin(const station_recipe_catalog::Descriptor& recipe,
             const StationRecipeMaterial& material,
             station_recipe_catalog::View<StationRecipeMaterialRow> rows,
             const StationRecipeResultBinding& binding) noexcept {
    if (state_ == StationRecipeResultState::waiting || state_ == StationRecipeResultState::failed)
      return false;
    if (!binding.generation || binding.generation <= binding_.generation ||
        binding.start_frame < last_frame_ || !binding.world || !binding.hero || !binding.station ||
        !rows.data || !rows.count || rows.count > Capacity || rows.count != recipe.targets.count ||
        !station_recipe_catalog::structurally_valid(recipe) ||
        !recipe.hand.static_lifecycle_verified || !valid_binding_hands(recipe, binding.hands) ||
        (recipe.completion == station_recipe_catalog::Completion::ready_same_station &&
         !binding.collection) ||
        !material.known() || material.hand_issue != StationRecipeHandIssue::none ||
        !material.craftable || !material.quantity ||
        material.quantity > static_cast<std::uint32_t>(INT32_MAX) ||
        material.quantity > material.maximum ||
        material.maximum > static_cast<std::uint32_t>(INT32_MAX) || material.count != rows.count ||
        material.completion != recipe.completion)
      return bad_contract();
    for (std::size_t i = 0; i < rows.count; ++i) {
      const auto& row = rows[i];
      const auto cost = recipe_material_detail::units(recipe.costs, row.symbol);
      const auto output = recipe_material_detail::units(recipe.outputs, row.symbol);
      const auto reserve = hand_amount(recipe, row.symbol,
                                       station_recipe_catalog::HandSource::reserved_inventory_tool);
      const auto generated =
          hand_amount(recipe, row.symbol, station_recipe_catalog::HandSource::generated);
      if (row.symbol != recipe.targets[i].symbol || !valid_total(row.stored) ||
          row.unit_cost != cost || row.unit_output != output || row.reserved_tool != reserve ||
          row.generated_hand != generated || !row.ready_storage_known || !row.end_storage_known ||
          row.required != std::int64_t{cost} * material.quantity || row.required > row.stored ||
          row.produced != std::int64_t{output} * material.quantity ||
          row.produced > INT32_MAX - row.stored || !valid_total(row.expected_ready_storage) ||
          !valid_total(row.expected_end_storage) ||
          row.expected_ready_storage != row.stored - row.required + row.produced ||
          row.expected_end_storage != row.expected_ready_storage + reserve ||
          (requires_absence(recipe, row.symbol) && row.stored != 0))
        return bad_contract();
    }
    for (std::size_t i = 0; i < rows.count; ++i)
      expected_[i] = {rows[i].symbol, rows[i].expected_ready_storage, rows[i].expected_end_storage};
    count_ = rows.count;
    binding_ = binding;
    completion_ = recipe.completion;
    production_ = recipe.production_context;
    last_frame_ = binding.start_frame;
    has_end_ = false;
    end_frame_ = 0;
    issue_ = StationRecipeResultIssue::none;
    state_ = StationRecipeResultState::waiting;
    return true;
  }

  bool end_after(const StationRecipeEndWitness& value) noexcept {
    if (state_ != StationRecipeResultState::waiting || has_end_ || !value.after_original ||
        !value.normal_exit_verified || value.generation != binding_.generation ||
        value.sequence <= binding_.end_sequence || value.frame < last_frame_ ||
        value.world != binding_.world || value.hero != binding_.hero ||
        value.station != binding_.station)
      return false;
    has_end_ = true;
    end_frame_ = value.frame;
    return true;
  }

  StationRecipeResultState observe(const StationRecipeResultObservation& value) noexcept {
    if (state_ != StationRecipeResultState::waiting || value.generation < binding_.generation)
      return state_;
    if (value.generation != binding_.generation)
      return fail(StationRecipeResultIssue::contract);
    if (!value.stable_boundary_verified || value.frame <= binding_.start_frame ||
        value.frame < last_frame_)
      return state_;
    last_frame_ = value.frame;
    if (value.identity_known && (value.world != binding_.world || value.hero != binding_.hero))
      return fail(StationRecipeResultIssue::context);
    if (value.station_known && value.station && value.station != binding_.station)
      return fail(StationRecipeResultIssue::context);
    if (value.production_known && value.production && value.production != production_)
      return fail(StationRecipeResultIssue::context);
    if (!value.identity_known || !value.station_known || !value.production_known)
      return state_;

    const bool ended = has_end_ && value.frame > end_frame_ && !value.station && !value.production;
    const bool ready =
        !has_end_ && completion_ == station_recipe_catalog::Completion::ready_same_station &&
        value.station == binding_.station && value.production == production_ &&
        value.selection_known && value.selection_open && value.selection_waiting &&
        !value.selection_done && value.collection_known && value.collection > binding_.collection;
    if (!ended && !ready)
      return state_;

    if (value.stock_known && !matching_stock(value.items, ended))
      return fail(StationRecipeResultIssue::stock);
    const auto hands = value.hands_known ? matching_hands(value.hands, ended) : Match::unknown;
    const auto anchor = value.hands_known ? matching_anchor(value.hands, ended) : Match::unknown;
    if (hands == Match::different)
      return fail(StationRecipeResultIssue::hands);
    if (anchor == Match::different)
      return fail(StationRecipeResultIssue::anchor);
    if (!value.stock_known || hands != Match::same || anchor != Match::same)
      return state_;
    state_ = StationRecipeResultState::verified;
    return state_;
  }

  StationRecipeResultState timeout() noexcept {
    return state_ == StationRecipeResultState::waiting ? fail(StationRecipeResultIssue::timeout)
                                                       : state_;
  }
  StationRecipeResultState fail(StationRecipeResultIssue issue) noexcept {
    if (state_ != StationRecipeResultState::failed) {
      issue_ = issue == StationRecipeResultIssue::none ? StationRecipeResultIssue::contract : issue;
      state_ = StationRecipeResultState::failed;
    }
    return state_;
  }
  StationRecipeResultState state() const noexcept { return state_; }
  StationRecipeResultIssue issue() const noexcept { return issue_; }
  bool permits_next() const noexcept {
    return state_ == StationRecipeResultState::idle || state_ == StationRecipeResultState::verified;
  }

private:
  enum class Match { unknown, same, different };
  struct Expected {
    std::int32_t symbol = -1;
    std::int64_t ready = -1, end = -1;
  };
  static bool valid_total(std::int64_t total) noexcept { return total >= 0 && total <= INT32_MAX; }
  bool bad_contract() noexcept {
    fail(StationRecipeResultIssue::contract);
    return false;
  }
  static bool valid_binding_hands(const station_recipe_catalog::Descriptor& recipe,
                                  const StationRecipeHands& hands) noexcept {
    namespace detail = station_recipe_material_detail;
    if (detail::check_hand(recipe.hand.left, hands.hands.left, hands.left_slot_known,
                           hands.left_tmp_level) != StationRecipeHandIssue::none ||
        detail::check_hand(recipe.hand.right, hands.hands.right, hands.right_slot_known,
                           hands.right_tmp_level) != StationRecipeHandIssue::none ||
        (hands.hands.left.object && hands.hands.left.object == hands.hands.right.object))
      return false;
    const auto anchor = recipe.hand.anchor == station_recipe_catalog::HandSide::left
                            ? hands.hands.left.object
                            : hands.hands.right.object;
    return hands.interact_item_known && anchor && hands.interact_item == anchor;
  }
  static std::int64_t hand_amount(const station_recipe_catalog::Descriptor& recipe,
                                  std::int32_t symbol,
                                  station_recipe_catalog::HandSource source) noexcept {
    std::int64_t amount = 0;
    for (const auto* hand : {&recipe.hand.left, &recipe.hand.right})
      if (hand->source == source && hand->symbol == symbol)
        amount += hand->amount;
    return amount;
  }
  static bool requires_absence(const station_recipe_catalog::Descriptor& recipe,
                               std::int32_t symbol) noexcept {
    return (recipe.hand.left.symbol == symbol && recipe.hand.left.require_stored_absent) ||
           (recipe.hand.right.symbol == symbol && recipe.hand.right.require_stored_absent);
  }
  bool matching_stock(station_recipe_catalog::View<StationRecipeResultItem> values,
                      bool ended) const noexcept {
    if (!values.data || values.count != count_)
      return false;
    for (std::size_t i = 0; i < count_; ++i) {
      if (!valid_total(values[i].total))
        return false;
      for (std::size_t j = 0; j < i; ++j)
        if (values[j].symbol == values[i].symbol)
          return false;
      bool found = false;
      for (std::size_t j = 0; j < count_; ++j)
        if (values[i].symbol == expected_[j].symbol) {
          if (values[i].total != (ended ? expected_[j].end : expected_[j].ready))
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
  static Match ready_hand(const HandObjectSnapshot& value, const HandObjectSnapshot& expected,
                          bool slot_known, std::int32_t tmp_level,
                          std::int32_t expected_tmp) noexcept {
    if (expected.state == 0)
      return empty_hand(value);
    if (value.state == -1)
      return Match::unknown;
    if (value.state != 1 || value.object != expected.object || value.symbol != expected.symbol ||
        value.amount != expected.amount)
      return Match::different;
    if (!slot_known)
      return Match::unknown;
    return tmp_level == expected_tmp ? Match::same : Match::different;
  }
  Match matching_hands(const StationRecipeHands& value, bool ended) const noexcept {
    const auto left =
        ended ? empty_hand(value.hands.left)
              : ready_hand(value.hands.left, binding_.hands.hands.left, value.left_slot_known,
                           value.left_tmp_level, binding_.hands.left_tmp_level);
    const auto right =
        ended ? empty_hand(value.hands.right)
              : ready_hand(value.hands.right, binding_.hands.hands.right, value.right_slot_known,
                           value.right_tmp_level, binding_.hands.right_tmp_level);
    if (left == Match::different || right == Match::different)
      return Match::different;
    return left == Match::same && right == Match::same ? Match::same : Match::unknown;
  }
  Match matching_anchor(const StationRecipeHands& value, bool ended) const noexcept {
    if (!value.interact_item_known)
      return Match::unknown;
    // Normal SetInteractItem(nullptr): Store NPC+0x968 at 0x74AED5/0x74AEE1.
    // The EndInteraction witness may precede it; only this later empty
    // anchor combined with empty hands completes the finished tail.
    return value.interact_item == (ended ? 0u : binding_.hands.interact_item) ? Match::same
                                                                              : Match::different;
  }

  StationRecipeResultState state_ = StationRecipeResultState::idle;
  StationRecipeResultIssue issue_ = StationRecipeResultIssue::none;
  StationRecipeResultBinding binding_;
  station_recipe_catalog::Completion completion_ =
      station_recipe_catalog::Completion::end_production;
  std::int32_t production_ = 0;
  std::array<Expected, Capacity> expected_{};
  std::size_t count_ = 0;
  std::uint64_t last_frame_ = 0, end_frame_ = 0;
  bool has_end_ = false;
};

} // namespace ubc
