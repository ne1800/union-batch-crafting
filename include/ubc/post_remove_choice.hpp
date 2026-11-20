// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/selection_snapshot.hpp"

namespace ubc {

// Own short-lived identities, no retained engine objects. OnChoice removes
// the chosen Choice at 0x662A53 before 0x662B6A CallFunc invokes. The
// View still has the old row count and collection. Only this single proven
// difference is permitted here; the normal Snapshot API remains strict.
// In particular, the removed Choice is never dereferenced afterwards.
class PostRemoveChoice {
public:
  static constexpr std::size_t capacity = 256;
  bool begin(const ReadOnlyMemory& memory, const SelectionDialog& dialog,
             std::uint64_t collection, std::uint64_t ticket,
             std::uint32_t selected, std::int32_t function) noexcept {
    if (active_) return false;
    used_ = false;
    if (!ticket || !collection || !dialog.valid || dialog.mode != 2 || !dialog.done ||
        !dialog.view || !dialog.info || !dialog.npc || !dialog.player || dialog.count <= 0 ||
        dialog.count > static_cast<std::int32_t>(capacity) || dialog.selected < 0 ||
        dialog.selected >= dialog.count || !selected || function < 0) return false;
    const auto owner = read_selection_info(memory, dialog.info);
    if (!owner.valid) return false;
    const auto count = static_cast<std::size_t>(dialog.count);
    if (!read_list(memory, owner.first_choice, entries_, count) || !stable(memory, dialog.info, owner, entries_, count))
      return false;
    const auto& chosen = entries_[static_cast<std::size_t>(dialog.selected)];
    if (chosen.choice != selected || chosen.function != function) return false;
    dialog_ = dialog; owner_ = owner; collection_ = collection; ticket_ = ticket;
    function_ = function; count_ = count; active_ = true;
    return true;
  }

  // Every attempt consumes the Witness, even a blocked attempt.
  // There is no re-release after an intervening contradiction.
  bool consume(const ReadOnlyMemory& memory, const SelectionDialog& dialog,
               std::uint64_t collection, std::uint64_t ticket, std::int32_t function) noexcept {
    if (!active_ || used_) return false;
    used_ = true;
    if (ticket != ticket_ || collection != collection_ || function != function_ || !same_dialog(dialog_, dialog)) return false;
    const auto owner = read_selection_info(memory, dialog.info);
    if (!same_owner(owner_, owner)) return false;
    std::array<Entry, capacity> after{};
    const auto& removed = entries_[static_cast<std::size_t>(dialog_.selected)];
    if (!read_list(memory, owner.first_choice, after, count_ - 1, removed.node, removed.choice) ||
        !stable(memory, dialog.info, owner, after, count_ - 1, removed.choice)) return false;
    std::size_t at = 0;
    for (std::size_t i = 0; i < count_; ++i) {
      if (i == static_cast<std::size_t>(dialog_.selected)) continue;
      const auto& old = entries_[i]; const auto& fresh = after[at++];
      if (old.node != fresh.node || old.choice != fresh.choice || old.function != fresh.function) return false;
    }
    return at == count_ - 1;
  }
  void clear() noexcept { active_ = false; used_ = false; }
  bool active() const noexcept { return active_; }

private:
  struct Entry { std::uint32_t node = 0, choice = 0, next = 0; std::int32_t function = -1; };
  static bool same_dialog(const SelectionDialog& a, const SelectionDialog& b) noexcept {
    return a.valid && b.valid && a.view == b.view && a.npc == b.npc && a.player == b.player && a.info == b.info &&
      a.mode == b.mode && a.selected == b.selected && a.count == b.count && a.done == b.done &&
      a.waiting == b.waiting && a.open == b.open && a.hidden == b.hidden;
  }
  static bool same_owner(const SelectionInfo& a, const SelectionInfo& b) noexcept {
    return a.valid && b.valid && a.npc == b.npc && a.number == b.number && a.important == b.important &&
      a.condition == b.condition && a.function == b.function && a.trade == b.trade &&
      a.permanent == b.permanent && a.instance == b.instance;
  }
  static bool read_entry(const ReadOnlyMemory& memory, std::uint32_t node, Entry& out,
                         std::uint32_t removed_choice = 0) noexcept {
    std::array<std::uint8_t, 8> link{}; std::array<std::uint8_t, 0x18> choice{};
    if (!node || !memory.copy(node, link.data(), link.size())) return false;
    out.node = node; out.choice = info_detail::u32(link.data()); out.next = info_detail::u32(link.data() + 4);
    if (!out.choice || out.choice == removed_choice || !memory.copy(out.choice, choice.data(), choice.size())) return false;
    out.function = station_detail::little_i32(choice.data() + 0x14);
    return out.function >= 0;
  }
  static bool read_list(const ReadOnlyMemory& memory, std::uint32_t head,
                        std::array<Entry, capacity>& out, std::size_t count,
                        std::uint32_t removed_node = 0, std::uint32_t removed_choice = 0) noexcept {
    if (count > capacity) return false;
    auto node = head;
    for (std::size_t i = 0; i < count; ++i) {
      if ((removed_node && node == removed_node) || !read_entry(memory, node, out[i], removed_choice)) return false;
      for (std::size_t j = 0; j < i; ++j)
        if (out[j].node == out[i].node || out[j].choice == out[i].choice) return false;
      node = out[i].next;
    }
    return node == 0;
  }
  static bool stable(const ReadOnlyMemory& memory, std::uint32_t address, const SelectionInfo& owner,
                      const std::array<Entry, capacity>& entries, std::size_t count,
                      std::uint32_t removed_choice = 0) noexcept {
    for (std::size_t i = 0; i < count; ++i) {
      Entry check;
      if (!read_entry(memory, entries[i].node, check, removed_choice) || check.choice != entries[i].choice ||
          check.next != entries[i].next || check.function != entries[i].function) return false;
    }
    const auto final_owner = read_selection_info(memory, address);
    return same_owner(owner, final_owner) && owner.first_choice == final_owner.first_choice;
  }
  SelectionDialog dialog_;
  SelectionInfo owner_;
  std::array<Entry, capacity> entries_{};
  std::size_t count_ = 0;
  std::uint64_t collection_ = 0, ticket_ = 0;
  std::int32_t function_ = -1;
  bool active_ = false, used_ = false;
};

} // namespace ubc
