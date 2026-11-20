// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/quantity.hpp"
#include "ubc/selection_session.hpp"

namespace ubc {

struct BatchSnapshot {
  // Only explicit adapter roles, not a list of all offers: a path
  // applies to all N; with two paths [0] applies to 1, [1] to
  // N>1. On refresh these roles are re-found by their identity.
  SelectionSnapshot selection;
  bool amount_contract = false;
  std::uint32_t maximum = 0;
};

struct BatchRequest {
  OriginalOffer original;
  std::uint32_t quantity = 0;
};

inline bool valid_batch_snapshot(const BatchSnapshot& snapshot) noexcept {
  return snapshot.amount_contract && snapshot.maximum <= static_cast<std::uint32_t>(INT32_MAX) &&
         snapshot.selection.count <= 2 && valid_selection_snapshot(snapshot.selection);
}

// Value-based contract. The model neither parametrizes scripts nor engine
// objects. amount_contract is set exclusively by the verified adapter.
// Maximum 0 is a known, currently unmanufacturable recipe preview.
class BatchSession {
public:
  bool open(const BatchSnapshot& snapshot) noexcept {
    const auto generation = snapshot.selection.key.generation;
    if (!generation || generation <= last_generation_) {
      original(SelectionIssue::context_changed);
      return false;
    }
    *this = BatchSession{};
    last_generation_ = generation;
    if (!valid_batch_snapshot(snapshot)) {
      original(SelectionIssue::invalid_snapshot);
      return false;
    }
    snapshot_ = snapshot;
    for (std::size_t i = 0; i < snapshot.selection.count; ++i) paths_[i] = snapshot.selection.offers[i];
    quantity_.reset(snapshot.maximum);
    phase_ = SelectionPhase::selecting;
    return true;
  }

  void observe(std::uint64_t frame, bool held, bool focused) noexcept {
    input_.observe(frame, held, focused);
    if (!focused && (phase_ == SelectionPhase::releasing || phase_ == SelectionPhase::ready)) {
      pending_ = {};
      phase_ = SelectionPhase::selecting;
      issue_ = SelectionIssue::focus_lost;
    }
    if (phase_ == SelectionPhase::releasing && input_.armed()) phase_ = SelectionPhase::ready;
  }

  // Directional inputs may be repeated; a step toggle activated
  // via A consumes exactly the same flank as a preset.
  bool adjust(std::int32_t delta, bool activation = false) noexcept {
    const bool accepted = activation ? input_.press() : input_.armed();
    if (!accepted || phase_ != SelectionPhase::selecting || !maximum()) return false;
    quantity_.adjust(delta);
    return true;
  }

  bool preset(std::uint32_t amount) noexcept {
    const bool accepted = input_.press();
    if (!accepted || phase_ != SelectionPhase::selecting) return false;
    return quantity_.preset(amount);
  }

  // Pure focus change via A/B: no manufacturing and no second
  // press() call for the same event in the surface adapter.
  bool acknowledge() noexcept {
    const bool accepted = input_.press();
    return accepted && phase_ == SelectionPhase::selecting;
  }

  bool confirm() noexcept {
    const bool accepted = input_.press();
    if (!accepted || phase_ != SelectionPhase::selecting || !quantity()) return false;
    pending_ = {path_for(quantity()), quantity()};
    phase_ = SelectionPhase::releasing;
    issue_ = SelectionIssue::none;
    return true;
  }

  bool back() noexcept {
    const bool accepted = input_.press();
    // An already confirmed contract may also be aborted during
    // release. The return path remains even at the release gate.
    if (input_.focused() && (phase_ == SelectionPhase::releasing || phase_ == SelectionPhase::ready)) {
      original(SelectionIssue::none);
      return true;
    }
    if (!accepted || phase_ != SelectionPhase::selecting) return false;
    original(SelectionIssue::none);
    return true;
  }

  void original(SelectionIssue issue) noexcept {
    if (phase_ == SelectionPhase::dispatched || phase_ == SelectionPhase::original) return;
    pending_ = {};
    phase_ = SelectionPhase::original;
    issue_ = issue;
    input_.disarm();
  }

  bool refresh(const BatchSnapshot& current) noexcept {
    if (phase_ == SelectionPhase::inactive || phase_ == SelectionPhase::original ||
        phase_ == SelectionPhase::dispatched) return false;
    if (!valid_batch_snapshot(current)) {
      original(SelectionIssue::invalid_snapshot);
      return false;
    }
    if (!(snapshot_.selection.key == current.selection.key)) {
      original(SelectionIssue::context_changed);
      return false;
    }
    std::array<OriginalOffer, 2> remapped{};
    if (current.selection.count != snapshot_.selection.count) {
      original(SelectionIssue::offer_changed);
      return false;
    }
    for (std::size_t role = 0; role < snapshot_.selection.count; ++role) {
      bool found = false;
      for (std::size_t i = 0; i < current.selection.count; ++i) {
        const auto& offer = current.selection.offers[i];
        const auto& expected = paths_[role];
        if (offer.info_instance == expected.info_instance && offer.function == expected.function &&
            offer.quantity == expected.quantity) {
          remapped[role] = offer;
          found = true;
          break;
        }
      }
      if (!found) {
        original(SelectionIssue::offer_changed);
        return false;
      }
    }
    const bool changed = snapshot_.selection.stored != current.selection.stored ||
                         snapshot_.selection.reserved != current.selection.reserved ||
                         snapshot_.maximum != current.maximum;
    snapshot_ = current;
    paths_ = remapped;
    if (changed) {
      pending_ = {};
      quantity_.reset(current.maximum);
      phase_ = SelectionPhase::selecting;
      issue_ = SelectionIssue::stock_changed;
      input_.disarm();
      return false;
    }
    return true;
  }

  std::optional<BatchRequest> take(const BatchSnapshot& current, bool ui_closed) noexcept {
    if (phase_ != SelectionPhase::ready || !input_.armed() || !ui_closed || !pending_.quantity)
      return std::nullopt;
    if (!refresh(current)) return std::nullopt;
    const BatchRequest result{path_for(pending_.quantity), pending_.quantity};
    // Before every engine effect, consume, even with unclear native follow-up.
    pending_ = {};
    phase_ = SelectionPhase::dispatched;
    input_.disarm();
    return result;
  }

  std::uint32_t quantity() const noexcept { return quantity_.value(); }
  std::uint32_t maximum() const noexcept { return quantity_.maximum(); }
  const BatchSnapshot& preview() const noexcept { return snapshot_; }
  SelectionPhase phase() const noexcept { return phase_; }
  SelectionIssue issue() const noexcept { return issue_; }
  bool armed() const noexcept { return input_.armed(); }
  bool return_ready() const noexcept { return phase_ == SelectionPhase::original && input_.armed(); }

private:
  const OriginalOffer& path_for(std::uint32_t amount) const noexcept {
    return paths_[snapshot_.selection.count == 2 && amount > 1 ? 1u : 0u];
  }
  BatchSnapshot snapshot_{};
  std::array<OriginalOffer, 2> paths_{};
  BatchRequest pending_{};
  Quantity quantity_{};
  SelectionInput input_{};
  std::uint64_t last_generation_ = 0;
  SelectionPhase phase_ = SelectionPhase::inactive;
  SelectionIssue issue_ = SelectionIssue::none;
};

} // namespace ubc
