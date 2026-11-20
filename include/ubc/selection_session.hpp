// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace ubc {

enum class SelectionPhase { inactive, selecting, releasing, ready, dispatched, original };
enum class SelectionIssue { none, invalid_snapshot, context_changed, stock_changed,
                            offer_changed, focus_lost };
enum class SelectionMode { info, choice };

// Native identities are comparison values only. No stored pointer is
// dereferenced by the core; choice pointers do not belong in this contract.
struct SelectionKey {
  std::uint64_t generation = 0;
  std::uint32_t thread = 0, world = 0, hero = 0, station = 0;
  std::uint32_t manager = 0, view = 0;
  SelectionMode mode = SelectionMode::info;
  std::int32_t owner_instance = -1;
  std::uint64_t collection = 0;

  bool operator==(const SelectionKey& rhs) const noexcept {
    return generation == rhs.generation && thread == rhs.thread && world == rhs.world &&
           hero == rhs.hero && station == rhs.station && manager == rhs.manager &&
           view == rhs.view && mode == rhs.mode && owner_instance == rhs.owner_instance &&
           collection == rhs.collection;
  }
};

struct OriginalOffer {
  std::int32_t info_instance = -1;
  std::int32_t function = -1;
  std::int32_t row = -1;
  std::uint32_t quantity = 0;
};

struct SelectionSnapshot {
  // Means only the explicit original-choice contract of the adapter,
  // never full bytecode identity or release of arbitrary amounts.
  bool selection_contract = false;
  bool stock_at_rest = false;
  SelectionKey key{};
  std::array<std::int32_t, 6> stored{};
  std::array<std::int32_t, 6> reserved{};
  std::array<OriginalOffer, 4> offers{};
  std::size_t count = 0;
};

inline bool valid_selection_snapshot(const SelectionSnapshot& s) noexcept {
  const auto& k = s.key;
  if (!s.selection_contract || !s.stock_at_rest || !k.generation || !k.thread ||
      !k.world || !k.hero || !k.station || !k.manager || !k.view ||
      (k.mode != SelectionMode::info && k.mode != SelectionMode::choice) ||
      (k.mode == SelectionMode::choice && k.owner_instance < 0) ||
      !s.count || s.count > s.offers.size()) return false;
  for (std::size_t i = 0; i < s.stored.size(); ++i)
    if (s.stored[i] < 0 || s.reserved[i] < 0 ||
        s.reserved[i] > INT32_MAX - s.stored[i]) return false;
  for (std::size_t i = 0; i < s.count; ++i) {
    const auto& offer = s.offers[i];
    if (offer.info_instance < 0 || offer.function < 0 || offer.row < 0 ||
        offer.row >= 256 || !offer.quantity || offer.quantity > INT32_MAX ||
        (k.mode == SelectionMode::choice && offer.info_instance != k.owner_instance)) return false;
    for (std::size_t j = 0; j < i; ++j)
      if (s.offers[j].function == offer.function || s.offers[j].row == offer.row) return false;
  }
  return true;
}

// A frame number comes from the attested outer engine frame, not from
// InfoManager::Update (which can run multiple times/recursively). held
// encompasses all confirm/dismiss bindings of the adapter. Events additionally
// count as pressed so that already released buffered events do not slip through.
class SelectionInput {
public:
  void reset() noexcept { *this = SelectionInput{}; }
  void disarm() noexcept { neutral_ = 0; latched_ = true; }

  void observe(std::uint64_t frame, bool held, bool focused) noexcept {
    if (!focused) { focused_ = false; disarm(); }
    // Polling can see the fresh press before the native event.
    // It immediately locks the handoff but does not consume its
    // one-time release from the previous two neutral frames.
    if (held) { neutral_ = 0; event_ = true; }
    if (!frame || (have_frame_ && frame < last_frame_)) { disarm(); return; }
    if (have_frame_ && frame == last_frame_) return;
    have_frame_ = true;
    last_frame_ = frame;
    focused_ = focused;
    if (!focused || held || event_) neutral_ = 0;
    else if (neutral_ < 2) ++neutral_;
    if (neutral_ == 2) latched_ = false;
    event_ = false;
  }

  bool press() noexcept {
    const bool accepted = focused_ && !latched_;
    event_ = true;
    disarm();
    return accepted;
  }
  bool armed() const noexcept { return focused_ && !latched_ && neutral_ == 2; }
  bool focused() const noexcept { return focused_; }

private:
  std::uint64_t last_frame_ = 0;
  bool have_frame_ = false, focused_ = false, latched_ = true, event_ = false;
  unsigned neutral_ = 0;
};

// One order per opening. The adapter must observe the full
// native completion after dispatched before it may open a new
// generation. An unclear success is never automatically retried.
class SelectionSession {
public:
  bool open(const SelectionSnapshot& snapshot) noexcept {
    if (!snapshot.key.generation || snapshot.key.generation <= last_generation_) {
      original(SelectionIssue::context_changed);
      return false;
    }
    const auto generation = snapshot.key.generation;
    *this = SelectionSession{};
    last_generation_ = generation;
    if (!valid_selection_snapshot(snapshot)) {
      original(SelectionIssue::invalid_snapshot);
      return false;
    }
    snapshot_ = snapshot;
    phase_ = SelectionPhase::selecting;
    choose_smallest();
    return true;
  }

  void observe(std::uint64_t frame, bool held, bool focused) noexcept {
    input_.observe(frame, held, focused);
    if (!focused && (phase_ == SelectionPhase::releasing || phase_ == SelectionPhase::ready)) {
      // Focus loss permanently invalidates an already confirmed craft.
      pending_ = {};
      phase_ = SelectionPhase::selecting;
      issue_ = SelectionIssue::focus_lost;
    }
    if (phase_ == SelectionPhase::releasing && input_.armed()) phase_ = SelectionPhase::ready;
  }

  bool select(std::size_t index) noexcept {
    if (phase_ != SelectionPhase::selecting || !input_.armed() || index >= snapshot_.count) return false;
    selected_ = index;
    return true;
  }

  bool confirm() noexcept {
    const bool accepted = input_.press();
    if (!accepted || phase_ != SelectionPhase::selecting) return false;
    pending_ = snapshot_.offers[selected_];
    phase_ = SelectionPhase::releasing;
    issue_ = SelectionIssue::none;
    return true;
  }

  bool preset(std::size_t index) noexcept {
    const bool accepted = input_.press();
    if (!accepted || phase_ != SelectionPhase::selecting || index >= snapshot_.count) return false;
    selected_ = index;
    return true;
  }

  bool back() noexcept {
    const bool accepted = input_.press();
    if (input_.focused() && (phase_ == SelectionPhase::releasing || phase_ == SelectionPhase::ready)) {
      original(SelectionIssue::none);
      return true;
    }
    if (!accepted || (phase_ != SelectionPhase::selecting && phase_ != SelectionPhase::ready)) return false;
    original(SelectionIssue::none);
    return true;
  }

  // Even an error/original return must only forward
  // held keys to the underlying dialog after release.
  void original(SelectionIssue issue) noexcept {
    // An unclear native tail is not a return path to the original
    // dialog. In particular, a late error/reentry event must not
    // restore the consumed crafting order to an input-ready state.
    if (phase_ == SelectionPhase::dispatched || phase_ == SelectionPhase::original) return;
    phase_ = SelectionPhase::original;
    issue_ = issue;
    pending_ = {};
    input_.disarm();
  }

  bool refresh(const SelectionSnapshot& current) noexcept {
    if (phase_ == SelectionPhase::inactive || phase_ == SelectionPhase::original ||
        phase_ == SelectionPhase::dispatched) return false;
    if (!valid_selection_snapshot(current)) {
      original(SelectionIssue::invalid_snapshot);
      return false;
    }
    if (!(snapshot_.key == current.key)) {
      original(SelectionIssue::context_changed);
      return false;
    }
    const bool stock_changed = snapshot_.stored != current.stored || snapshot_.reserved != current.reserved;
    const auto target = phase_ == SelectionPhase::selecting ? snapshot_.offers[selected_] : pending_;
    std::size_t replacement = current.count;
    for (std::size_t i = 0; i < current.count; ++i) {
      const auto& offer = current.offers[i];
      if (offer.info_instance == target.info_instance && offer.function == target.function &&
          offer.quantity == target.quantity) replacement = i;
    }
    snapshot_ = current;
    if (stock_changed || replacement == current.count) {
      pending_ = {};
      phase_ = SelectionPhase::selecting;
      issue_ = stock_changed ? SelectionIssue::stock_changed : SelectionIssue::offer_changed;
      choose_smallest();
      input_.disarm();
      return false;
    }
    selected_ = replacement;
    return true;
  }

  std::optional<OriginalOffer> take(const SelectionSnapshot& current, bool ui_closed) noexcept {
    // Consume before every native call. Only the caller performs
    // Select and HandleEvent; recursive calls are already dispatched.
    if (phase_ != SelectionPhase::ready || !input_.armed() || !ui_closed) return std::nullopt;
    if (!refresh(current)) return std::nullopt;
    const auto result = snapshot_.offers[selected_];
    phase_ = SelectionPhase::dispatched;
    pending_ = {};
    input_.disarm();
    return result;
  }

  SelectionPhase phase() const noexcept { return phase_; }
  SelectionIssue issue() const noexcept { return issue_; }
  bool armed() const noexcept { return input_.armed(); }
  bool return_ready() const noexcept { return phase_ == SelectionPhase::original && input_.armed(); }
  std::size_t selected() const noexcept { return selected_; }
  const SelectionSnapshot& preview() const noexcept { return snapshot_; }

private:
  void choose_smallest() noexcept {
    selected_ = 0;
    for (std::size_t i = 1; i < snapshot_.count; ++i)
      if (snapshot_.offers[i].quantity < snapshot_.offers[selected_].quantity) selected_ = i;
  }
  SelectionSnapshot snapshot_{};
  std::uint64_t last_generation_ = 0;
  OriginalOffer pending_{};
  SelectionInput input_{};
  SelectionPhase phase_ = SelectionPhase::inactive;
  SelectionIssue issue_ = SelectionIssue::none;
  std::size_t selected_ = 0;
};

} // namespace ubc
