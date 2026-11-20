// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/core/selection_stock.hpp"

namespace ubc {
enum class BatchResultState { idle, waiting, verified, failed };
enum class BatchResultIssue { none, contract, context, stock, hands, timeout, restore };
struct BatchResultObservation {
  bool identity_known = false, stock_known = false, hands_known = false;
  std::uint32_t world = 0, hero = 0, station = 0;
  bool station_known = false, ended = false, reopened = false;
  std::array<std::int32_t, 6> stored{};
  HandsSnapshot hands{};
};

// A callback return is not a result evidence. Only the idle
// follow frame with completed lab run or re-ready hearth may
// confirm. An unclear order stays locked; no automatic retry.
class BatchResult {
public:
  bool begin(OriginalRecipe recipe, const MaterialPreview& expected,
      std::uint32_t world, std::uint32_t hero, std::uint32_t station) noexcept {
    if (state_ == BatchResultState::waiting || state_ == BatchResultState::failed ||
        !expected.valid || !world || !hero || !station) return false;
    recipe_ = recipe; expected_ = expected.remaining;
    world_ = world; hero_ = hero; station_ = station;
    state_ = BatchResultState::waiting; issue_ = BatchResultIssue::none;
    return true;
  }
  BatchResultState observe(const BatchResultObservation& value) noexcept {
    if (state_ != BatchResultState::waiting) return state_;
    if (!value.identity_known) return state_;
    if (value.world != world_ || value.hero != hero_) return fail(BatchResultIssue::context);
    if (value.station_known && value.station && value.station != station_)
      return fail(BatchResultIssue::context);
    const bool done = value.ended && value.station_known && !value.station;
    const bool ready = recipe_ == OriginalRecipe::meat && value.reopened &&
        value.station_known && value.station == station_;
    if (!done && !ready) return state_;
    if (!value.stock_known || !value.hands_known) return state_;
    if (value.hands.left.state != 0) return fail(BatchResultIssue::hands);
    const bool hands_ok = done ? value.hands.right.state == 0 :
        value.hands.right.state == 1 && value.hands.right.symbol == 16608 &&
        value.hands.right.amount == 1;
    if (!hands_ok) return fail(BatchResultIssue::hands);
    if (value.stored != expected_) return fail(BatchResultIssue::stock);
    state_ = BatchResultState::verified;
    return state_;
  }
  BatchResultState fail(BatchResultIssue issue) noexcept {
    issue_ = issue; state_ = BatchResultState::failed; return state_;
  }
  BatchResultState state() const noexcept { return state_; }
  BatchResultIssue issue() const noexcept { return issue_; }
  bool permits_next() const noexcept { return state_ == BatchResultState::idle || state_ == BatchResultState::verified; }
private:
  BatchResultState state_ = BatchResultState::idle;
  BatchResultIssue issue_ = BatchResultIssue::none;
  OriginalRecipe recipe_ = OriginalRecipe::meat;
  std::uint32_t world_ = 0, hero_ = 0, station_ = 0;
  std::array<std::int32_t, 6> expected_{};
};
} // namespace ubc
