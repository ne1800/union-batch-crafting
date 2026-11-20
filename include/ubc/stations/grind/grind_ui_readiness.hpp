// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/ui/crafting_ui_diagnostics.hpp"

namespace ubc::grind_ui_readiness {
// Own value copies only. No engine access, no input or writer rights.
struct State {
  CraftingMenuRow row = CraftingMenuRow::recipe;
  unsigned column = 0;
  SelectionPhase phase = SelectionPhase::inactive;
  SelectionIssue issue = SelectionIssue::none;
  CraftingBlockReason reason = CraftingBlockReason::none;
  bool armed = false, can_confirm = false;
  std::uint32_t recipe = UINT32_MAX, quantity = 0, maximum = 0;
};
inline State state(const CraftingMenu& menu, const CraftingSession& session) noexcept {
  const auto* selected = session.selected();
  return {menu.row(),
          menu.column(),
          session.phase(),
          session.issue(),
          session.reason(),
          session.armed(),
          session.can_confirm(),
          selected ? selected->id : UINT32_MAX,
          session.quantity(),
          session.maximum()};
}
inline bool same(const State& a, const State& b) noexcept {
  return a.row == b.row && a.column == b.column && a.phase == b.phase && a.issue == b.issue &&
         a.reason == b.reason && a.armed == b.armed && a.can_confirm == b.can_confirm &&
         a.recipe == b.recipe && a.quantity == b.quantity && a.maximum == b.maximum;
}
inline bool valid(const State& s) noexcept {
  const auto* recipe = crafting_catalog::find(s.recipe);
  const auto row = static_cast<unsigned>(s.row), phase = static_cast<unsigned>(s.phase);
  const unsigned columns = s.row == CraftingMenuRow::presets ? 4
                           : s.row == CraftingMenuRow::steps ? 2
                                                             : 1;
  return recipe &&
         ((recipe->station == crafting_catalog::Station::grind && recipe->grind) ||
          (recipe->station == crafting_catalog::Station::bow && recipe->bow)) &&
         row <= 5 && s.column < columns && phase <= 5 && static_cast<unsigned>(s.issue) <= 5 &&
         static_cast<unsigned>(s.reason) <=
             static_cast<unsigned>(CraftingBlockReason::quantity_unsupported) &&
         s.maximum <= INT32_MAX && s.quantity <= s.maximum &&
         ((s.maximum == 0) == (s.quantity == 0)) &&
         (!s.can_confirm || (s.quantity && s.reason == CraftingBlockReason::none));
}
struct QuantityObservation {
  // From the actually traversed quantity checker; even available=false
  // is an evaluated observation. No invented positive stamp.
  bool observed = false;
  std::uint64_t epoch = 0, frame = 0;
  std::uint32_t recipe = UINT32_MAX, quantity = 0;
};
struct Render {
  std::uint64_t epoch = 0, frame = 0, input_serial_before = 0;
  State before{}, after{};
  QuantityObservation quantity{};
  bool outer_quiet = false, context_bound = false, rendered = false, owns_input = false;
  bool focused = false, held = false, poll_known = false;
};
enum class Issue {
  none,
  closed,
  epoch,
  frame,
  binding,
  render,
  state,
  focus,
  input,
  reentrant,
  quantity
};
using Record = crafting_ui_diagnostics::Record;
struct Batch {
  std::array<Record, 5> records{};
  std::size_t count = 0;
};
struct Input {
  std::uint32_t serial = 0;
  std::uint64_t frame = 0;
  int key = 0;
  CraftingMenuAction action = CraftingMenuAction::none;
  bool accepted = false;
};
// Bits: row0..2,column3..4,phase5..7,armed8,can_confirm9,issue10..14,
// reason15..19,focused20,held21,poll_known22,action23..26,accepted27,input28.
// Bits 29..31 are reserved. Each group belongs to the same outer frame.
inline int flags(const State& s, const Input& input, bool focused, bool held, bool known) noexcept {
  return static_cast<int>(
      static_cast<unsigned>(s.row) | (s.column << 3u) | (static_cast<unsigned>(s.phase) << 5u) |
      (unsigned{s.armed} << 8u) | (unsigned{s.can_confirm} << 9u) |
      (static_cast<unsigned>(s.issue) << 10u) | (static_cast<unsigned>(s.reason) << 15u) |
      (unsigned{focused} << 20u) | (unsigned{held} << 21u) | (unsigned{known} << 22u) |
      (static_cast<unsigned>(input.action) << 23u) | (unsigned{input.accepted} << 27u) |
      (unsigned{input.serial != 0} << 28u));
}
class Trace {
public:
  static constexpr unsigned max_inputs = 64, max_groups = 128, max_records = 2048;
  bool begin(std::uint64_t epoch, std::uint64_t frame) noexcept {
    if (!epoch || epoch <= epoch_ || !frame || frame <= last_frame_ || active_ || global_limited_) {
      fault(Issue::epoch);
      return false;
    }
    epoch_ = epoch;
    last_frame_ = frame;
    active_ = true;
    failed_ = false;
    pending_ = Issue::none;
    input_ = {};
    groups_ = 0;
    have_ = false;
    limit_ = 0;
    limited_reported_ = false;
    return true;
  }
  std::uint64_t epoch() const noexcept { return epoch_; }
  std::uint32_t input_serial() const noexcept { return input_.serial; }
  unsigned groups() const noexcept { return groups_; }
  unsigned records() const noexcept { return records_; }
  void observe_input(std::uint64_t epoch, std::uint64_t frame,
                     const crafting_ui_diagnostics::Action& a) noexcept {
    if (!active_ || failed_)
      return;
    if (epoch != epoch_) {
      fault(Issue::epoch);
      return;
    }
    if (!frame || frame < last_frame_ || frame < input_.frame) {
      fault(Issue::frame);
      return;
    }
    if (!a.observed || static_cast<unsigned>(a.action) > 10 ||
        (a.accepted && (!a.focused || a.action == CraftingMenuAction::none))) {
      fault(Issue::input);
      return;
    }
    if (input_.serial == max_inputs) {
      limit_ = 1;
      failed_ = true;
      return;
    }
    ++input_.serial;
    input_.frame = frame;
    input_.key = a.key;
    input_.action = a.action;
    input_.accepted = a.accepted;
  }
  Batch after_render(const Render& r) noexcept {
    if (!active_ || failed_)
      return pending();
    if (r.epoch != epoch_) {
      fault(Issue::epoch);
      return pending();
    }
    if (!r.frame || r.frame <= last_frame_) {
      fault(Issue::frame);
      return pending();
    }
    last_frame_ = r.frame;
    if (!r.outer_quiet) {
      fault(Issue::reentrant);
      return pending();
    }
    if (!r.context_bound || !r.owns_input) {
      fault(Issue::binding);
      return pending();
    }
    if (!r.rendered) {
      fault(Issue::render);
      return pending();
    }
    if (!r.poll_known || !r.focused) {
      fault(Issue::focus);
      return pending();
    }
    if (r.input_serial_before != input_.serial || !same(r.before, r.after)) {
      fault(Issue::reentrant);
      return pending();
    }
    if (!valid(r.after) || (r.held && r.after.armed)) {
      fault(Issue::state);
      return pending();
    }
    const auto& q = r.quantity;
    if (!q.observed || q.epoch != epoch_ || q.frame != r.frame || q.recipe != r.after.recipe ||
        q.quantity != r.after.quantity) {
      fault(Issue::quantity);
      return pending();
    }
    // In the same outer frame the old poll may precede the event.
    // No release from it, even if adjust()/tab() do not disarm.
    if (input_.serial && r.frame <= input_.frame)
      return {};
    const int bits = flags(r.after, input_, r.focused, r.held, r.poll_known);
    if (have_ && same(last_, r.after) && last_bits_ == bits && last_input_ == input_.serial)
      return {};
    if (groups_ == max_groups) {
      limit_ = 2;
      failed_ = true;
      return pending();
    }
    if (records_ + 5 >= max_records) {
      limit_ = 3;
      global_limited_ = failed_ = true;
      return pending();
    }
    using crafting_ui_diagnostics::word;
    Batch b;
    b.records = {{{"grind_ui_ready_epoch", word(static_cast<std::uint32_t>(epoch_)),
                   word(static_cast<std::uint32_t>(epoch_ >> 32u))},
                  {"grind_ui_ready_state", bits, static_cast<int>(r.after.recipe)},
                  {"grind_ui_ready_quantity", static_cast<int>(r.after.quantity),
                   static_cast<int>(r.after.maximum)},
                  {"grind_ui_ready_input", static_cast<int>(input_.serial), input_.key},
                  {"grind_ui_ready_input_frame", word(static_cast<std::uint32_t>(input_.frame)),
                   word(static_cast<std::uint32_t>(input_.frame >> 32u))}}};
    b.count = 5;
    records_ += 5;
    ++groups_;
    last_ = r.after;
    last_bits_ = bits;
    last_input_ = input_.serial;
    have_ = true;
    return b;
  }
  Batch close() noexcept {
    if (active_ && !failed_)
      fault(Issue::closed);
    active_ = false;
    return pending();
  }

private:
  void fault(Issue issue) noexcept {
    if (pending_ == Issue::none && !failed_) {
      pending_ = issue;
      failed_ = true;
    }
  }
  Batch pending() noexcept {
    Batch b;
    if (limit_ && !limited_reported_) {
      limited_reported_ = true;
      if (records_ < max_records) {
        b.records[0] = {"grind_ui_ready_limit", limit_,
                        static_cast<int>(limit_ == 1   ? input_.serial
                                         : limit_ == 2 ? groups_
                                                       : records_)};
        b.count = 1;
        ++records_;
      }
    } else if (pending_ != Issue::none) {
      if (records_ + 1 >= max_records && !global_limited_) {
        limit_ = 3;
        global_limited_ = true;
        return pending();
      }
      if (records_ < max_records) {
        b.records[0] = {"grind_ui_ready_revoked", static_cast<int>(pending_),
                        static_cast<int>(input_.serial)};
        b.count = 1;
        ++records_;
      }
      pending_ = Issue::none;
    }
    return b;
  }
  std::uint64_t epoch_ = 0, last_frame_ = 0;
  Input input_{};
  State last_{};
  std::uint32_t last_input_ = 0;
  unsigned groups_ = 0, records_ = 0;
  int last_bits_ = 0, limit_ = 0;
  Issue pending_ = Issue::none;
  bool active_ = false, failed_ = false, have_ = false, limited_reported_ = false,
       global_limited_ = false;
};
} // namespace ubc::grind_ui_readiness
