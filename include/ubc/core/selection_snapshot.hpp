// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/engine/info_snapshot.hpp"
#include "ubc/stations/bow/bow_menu_trace.hpp"
#include "ubc/engine/selection_abi.hpp"
#include "ubc/core/selection_profile.hpp"
#include "ubc/core/selection_session.hpp"

namespace ubc {

struct SelectionInfo {
  bool valid = false;
  std::int32_t npc = -1, number = -1, important = -1, condition = -1;
  std::int32_t function = -1, trade = -1, permanent = -1, instance = -1;
  std::uint32_t first_choice = 0, address = 0;
};

inline SelectionInfo read_selection_info(const ReadOnlyMemory& memory,
                                         std::uint32_t info) noexcept {
  std::array<std::uint8_t, 0x5c> bytes{};
  if (!memory.copy(info, bytes.data(), bytes.size()) ||
      info_detail::u32(bytes.data()) != info_detail::info_vtable)
    return {};
  const auto field = [&](std::size_t offset) {
    return station_detail::little_i32(bytes.data() + offset);
  };
  SelectionInfo result{
      true,        field(0x1c), field(0x20), field(0x24), field(0x28),
      field(0x2c), field(0x44), field(0x48), field(0x50), info_detail::u32(bytes.data() + 0x58),
      info};
  if (result.function < 0 || result.instance < 0)
    return {};
  return result;
}

inline bool selection_info_link(const SelectionInfo& info,
                                const selection_profile::InfoLink& link) noexcept {
  return info.valid && info.instance == link.instance && info.condition == link.condition &&
         info.function == link.function && info.number == link.number && info.npc == 22243 &&
         info.important == 0 && info.trade == 0 && info.permanent == 1;
}

struct SelectionDialog {
  bool valid = false, waiting = false, open = false, hidden = false, done = false;
  bool rejected_info_transition = false;
  std::uint32_t view = 0, npc = 0, player = 0, info = 0;
  std::int32_t mode = 0, selected = -1, count = 0;
};

inline SelectionDialog read_selection_dialog(const ReadOnlyMemory& memory, std::uint32_t manager,
                                             BowMenuTrace* trace = nullptr,
                                             std::uint32_t hero = 0) noexcept {
  namespace abi = selection_abi;
  std::array<std::uint8_t, 0x58> header{};
  if (!memory.copy(manager, header.data(), header.size())) {
    if (trace)
      trace->check("dialog.manager", "lesbar", "unlesbar", false);
    return {};
  }
  const auto m = [&](std::size_t offset) { return info_detail::u32(header.data() + offset); };
  SelectionDialog result;
  result.view = m(0x1c);
  result.npc = m(0x20);
  result.player = m(0x24);
  result.info = m(0x28);
  result.mode = station_detail::little_i32(header.data() + 0x54);
  if (trace) {
    trace->equal("dialog.npc", hero, result.npc);
    trace->equal("dialog.player", hero, result.player);
    trace->equal("dialog.raw_mode", 1, result.mode);
    constexpr const char* flags[] = {
        "dialog.done_flag",  "dialog.end_flag",       "dialog.script_flag", "dialog.open_flag",
        "dialog.close_flag", "dialog.selection_flag", "dialog.must_open"};
    for (unsigned i = 0; i < 7; ++i)
      trace->equal(flags[i], i == 5 ? 1 : 0, m(0x2c + i * 4));
  }
  std::array<std::uint8_t, abi::view_size> view{};
  if (!memory.copy(result.view, view.data(), view.size())) {
    if (trace)
      trace->check("dialog.view_read", "lesbar", "unlesbar", false);
    return {};
  }
  const auto v = [&](std::size_t offset) { return info_detail::u32(view.data() + offset); };
  if (trace) {
    trace->equal("dialog.view_vtable", abi::view_vtable, v(0));
    trace->equal("dialog.base_vtable", 0x83bc10, v(0x24));
    trace->equal("dialog.callback_vtable", abi::view_callback_vtable, v(abi::view_callback));
    trace->equal("dialog.open_time", 0, v(abi::view_open_time));
    trace->equal("dialog.close_time", 0, v(abi::view_close_time));
    trace->equal("dialog.view_opened", 1, v(abi::view_opened));
    trace->equal("dialog.view_closed", 0, v(abi::view_closed));
    trace->equal("dialog.view_done", 0, v(abi::view_done));
    trace->equal("dialog.view_active", 1, v(abi::view_active));
    const int count = station_detail::little_i32(view.data() + abi::view_count);
    const int selected = station_detail::little_i32(view.data() + abi::view_selected);
    trace->value("dialog.raw_count", "0..256", count, count >= 0 && count <= 256);
    trace->value("dialog.selected", "-1 bis Zeilenzahl-1", selected,
                 selected >= -1 && (count ? selected < count : selected == -1));
  }
  if (v(0) != abi::view_vtable || v(0x24) != 0x83bc10 ||
      v(abi::view_callback) != abi::view_callback_vtable)
    return {};
  result.selected = station_detail::little_i32(view.data() + abi::view_selected);
  result.count = station_detail::little_i32(view.data() + abi::view_count);
  if (result.count < 0 || result.count > 256 || result.selected < -1 ||
      (result.count && result.selected >= result.count) || (!result.count && result.selected != -1))
    return {};
  result.waiting = m(0x2c) == 0 && m(0x30) == 0 && m(0x34) == 0 && m(0x38) == 0 && m(0x3c) == 0 &&
                   m(0x40) == 1 && m(0x44) == 0;
  result.done = v(abi::view_done) == 1 && v(abi::view_active) == 0;
  const bool no_animation = v(abi::view_open_time) == 0 && v(abi::view_close_time) == 0;
  result.open = no_animation && v(abi::view_opened) == 1 && v(abi::view_closed) == 0 &&
                v(abi::view_done) == 0 && v(abi::view_active) == 1;
  result.hidden = no_animation && v(abi::view_opened) == 0 && v(abi::view_closed) == 1 &&
                  v(abi::view_done) == 0 && v(abi::view_active) == 0;
  result.rejected_info_transition = result.mode == 1 && result.done && no_animation &&
                                    v(abi::view_opened) == 0 && v(abi::view_closed) == 1 &&
                                    m(0x2c) == 0 && m(0x30) == 0 && m(0x34) == 0 && m(0x38) == 0 &&
                                    m(0x3c) == 0 && m(0x40) == 0 && m(0x44) == 0;
  result.valid = result.npc && result.player && (result.mode == 1 || result.mode == 2);
  return result;
}

// Observe only the original collection process. Unknown RECIPES remain
// recorded native rows and never become offers of our own. Unknown CALLERS,
// special text rows or incomplete collections invalidate the cache.
struct SelectionInfoRows {
  std::uint64_t generation = 0;
  std::uint32_t view = 0;
  std::size_t count = 0;
  bool valid = false;
  std::array<SelectionInfo, 256> rows{};
  const char* rejected_field = nullptr;
  std::int64_t expected = 0, actual = 0;

  void reject(const char* field, std::int64_t wanted, std::int64_t observed) noexcept {
    if (!rejected_field) {
      rejected_field = field;
      expected = wanted;
      actual = observed;
    }
    valid = false;
  }

  void begin(std::uint32_t new_view, bool info_collection) noexcept {
    valid = false;
    view = new_view;
    count = 0;
    rejected_field = nullptr;
    if (generation == UINT64_MAX) {
      reject("info.generation_exhausted", 0, 1);
      return;
    }
    ++generation;
    valid = info_collection && new_view;
  }
  void append(std::uint32_t current_view, const SelectionInfo& info, std::int32_t before,
              std::int32_t after, bool known_caller) noexcept {
    if (!valid)
      return;
    if (!known_caller)
      return reject("info.add_context", 1, 0);
    if (current_view != view)
      return reject("info.add_view", view, current_view);
    if (!info.valid)
      return reject("info.readable", 1, 0);
    if (count == rows.size())
      return reject("info.capacity", static_cast<std::int64_t>(rows.size() - 1),
                    static_cast<std::int64_t>(count));
    if (before < 0 || static_cast<std::size_t>(before) != count)
      return reject("info.count_before", static_cast<std::int64_t>(count), before);
    if (after != before + 1)
      return reject("info.count_after", before + 1, after);
    for (std::size_t i = 0; i < count; ++i)
      if (rows[i].instance == info.instance)
        return reject("info.duplicate_instance", 0, info.instance);
    rows[count++] = info;
  }
  bool complete(const SelectionDialog& dialog) const noexcept {
    return valid && dialog.valid && dialog.mode == 1 && dialog.view == view && dialog.count >= 0 &&
           count == static_cast<std::size_t>(dialog.count);
  }
};

struct SelectionChoices {
  bool valid = false;
  SelectionInfo owner{};
  std::size_t count = 0;
  std::array<std::int32_t, 256> functions{};
};

inline SelectionChoices read_selection_choices(const ReadOnlyMemory& memory,
                                               const SelectionDialog& dialog) noexcept {
  if (!dialog.valid || dialog.mode != 2 || !dialog.info)
    return {};
  SelectionChoices result;
  result.owner = read_selection_info(memory, dialog.info);
  if (!result.owner.valid)
    return {};
  std::array<std::uint32_t, 256> nodes{}, choices{};
  auto node = result.owner.first_choice;
  while (node) {
    if (result.count == nodes.size())
      return {};
    for (std::size_t i = 0; i < result.count; ++i)
      if (nodes[i] == node)
        return {};
    std::array<std::uint8_t, 8> links{};
    if (!memory.copy(node, links.data(), links.size()))
      return {};
    const auto choice = info_detail::u32(links.data());
    for (std::size_t i = 0; i < result.count; ++i)
      if (choices[i] == choice)
        return {};
    std::array<std::uint8_t, 0x18> record{};
    if (!memory.copy(choice, record.data(), record.size()))
      return {};
    const auto function = station_detail::little_i32(record.data() + 0x14);
    if (function < 0)
      return {};
    nodes[result.count] = node;
    choices[result.count] = choice;
    result.functions[result.count++] = function;
    node = info_detail::u32(links.data() + 4);
  }
  result.valid = result.count == static_cast<std::size_t>(dialog.count);
  return result.valid ? result : SelectionChoices{};
}
} // namespace ubc
