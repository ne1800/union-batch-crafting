// SPDX-License-Identifier: MIT
#include "ubc/ui/native_crafting_ui.hpp"
#include "ubc/stations/bow/bow_menu_trace.hpp"
#include "ubc/core/crafting_result.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace ubc::crafting_ui {
using L = ui_text::Label;
using ui_text::Table;
namespace {
bool dimensions(unsigned width, unsigned height) noexcept {
  return width >= 640 && height >= 480 && width <= 32768 && height <= 32768;
}
bool append(Frame& frame, int x, int y, const char* text, std::size_t limit) noexcept {
  if (!text || !limit || limit >= Line{}.text.size() || frame.count == frame.lines.size() ||
      x < 0 || x >= 8192 || y < 0 || y >= 8192)
    return false;
  auto& line = frame.lines[frame.count++];
  line = {};
  line.x = x;
  line.y = y;
  line.character_limit = limit;
  const auto length = std::strlen(text), count = length < limit ? length : limit;
  std::memcpy(line.text.data(), text, count);
  if (length > limit && count >= 3)
    std::memcpy(line.text.data() + count - 3, "...", 3);
  return true;
}
bool wrapped(Frame& frame, int x, int y, const char* text, std::size_t limit,
             int step = 260) noexcept {
  const auto length = std::strlen(text);
  if (length <= limit)
    return append(frame, x, y, text, limit);
  std::size_t split = limit;
  while (split && text[split] != ' ')
    --split;
  if (!split)
    split = limit;
  std::array<char, 192> first{};
  std::memcpy(first.data(), text, split);
  if (!append(frame, x, y, first.data(), limit))
    return false;
  while (text[split] == ' ')
    ++split;
  return append(frame, x, y + step, text + split, limit);
}
struct BlockText {
  const char* detail;
  const char* hint;
};
BlockText blocked(CraftingBlockReason reason, const Table& text) noexcept {
  switch (reason) {
  case CraftingBlockReason::none:
    return {"", ""};
  case CraftingBlockReason::no_known_recipes:
    return {text[L::no_recipes], text[L::hint_no_known_recipes]};
  case CraftingBlockReason::contract_unverified:
    return {text[L::unverified], text[L::hint_contract_unverified]};
  case CraftingBlockReason::actor_unknown:
    return {text[L::no_actor], text[L::hint_actor_unknown]};
  case CraftingBlockReason::actor_changed:
    return {text[L::wrong_actor], text[L::hint_actor_changed]};
  case CraftingBlockReason::hand_unknown:
    return {text[L::hand_not_ready], text[L::hint_hand_unknown]};
  case CraftingBlockReason::material_unknown:
    return {text[L::stock_unreadable], text[L::hint_material_unknown]};
  case CraftingBlockReason::material_unverified:
    return {text[L::stock_unverified], text[L::hint_material_unknown]};
  case CraftingBlockReason::missing_material:
    return {text[L::missing_material], text[L::hint_missing_material]};
  case CraftingBlockReason::output_capacity:
    return {text[L::output_full], text[L::hint_output_capacity]};
  case CraftingBlockReason::tool_return_capacity:
    return {text[L::tool_full], text[L::hint_output_capacity]};
  case CraftingBlockReason::unavailable:
    return {text[L::unavailable], text[L::hint_unavailable]};
  case CraftingBlockReason::quantity_unsupported:
    return {text[L::unsupported_amount], text[L::hint_quantity_unsupported]};
  }
  return {text[L::unavailable], text[L::hint_unavailable]};
}
bool relevant(const CraftingMaterialValue& row, const crafting_catalog::Descriptor& d) noexcept {
  return row.fixed_minimum || row.unit_cost || row.unit_output ||
         crafting_catalog::hand_amounts(d, row.symbol).tool;
}
// The native font is proportional. Draw the marker and the label at separate origins.
constexpr int marker_gap = 220;
bool choice(Frame& frame, int x, int y, const char* label, std::size_t limit,
            const char* marker = nullptr) noexcept {
  return (!marker || append(frame, x - marker_gap, y, marker, 3)) &&
         append(frame, x, y, label, limit);
}

struct KeyHint {
  L confirm, navigation;
};
KeyHint key_hint(CraftingMenuRow row) noexcept {
  switch (row) {
  case CraftingMenuRow::recipe:
    return {L::key_choose, L::browse_recipes};
  case CraftingMenuRow::quantity:
    return {L::key_continue, L::adjust_amount};
  case CraftingMenuRow::presets:
    return {L::key_preset, L::browse_buttons};
  case CraftingMenuRow::steps:
    return {L::key_step, L::browse_buttons};
  case CraftingMenuRow::manufacture:
    return {L::key_confirm, L::change_field};
  case CraftingMenuRow::original:
    return {L::key_original, L::change_field};
  }
  return {L::key_choose, L::browse_recipes};
}
} // namespace

NativeCraftingSuccess success_notice(const CraftingResultSuccess& success) noexcept {
  NativeCraftingSuccess notice;
  if (success.produced_count > notice.produced.size() || success.stock_count > notice.stocks.size())
    return {};
  notice.result_verified = success.result_verified;
  notice.restore_verified = success.restore_verified;
  notice.receipt = success.receipt;
  notice.key = success.key;
  notice.id = success.id;
  notice.quantity = success.quantity;
  notice.produced_count = success.produced_count;
  notice.stock_count = success.stock_count;
  notice.branch_known = success.branch_known;
  notice.branch_value = success.branch_value;
  notice.branch_output = success.branch_output;
  for (std::size_t i = 0; i < success.produced_count; ++i)
    notice.produced[i] = {success.produced[i].symbol, success.produced[i].amount};
  for (std::size_t i = 0; i < success.stock_count; ++i)
    notice.stocks[i] = {success.stocks[i].symbol, success.stocks[i].stored};
  return notice;
}
bool valid_success(const NativeCraftingSuccess& value) noexcept {
  const auto* d = crafting_catalog::find(value.id);
  if (!d ||
      !crafting_catalog::valid_branch(*d, value.branch_known, value.branch_value,
                                      value.branch_output) ||
      (d->route == crafting_catalog::Route::spirit && !value.branch_known))
    return false;
  const auto outputs = crafting_catalog::outputs_for(*d, value.branch_known, value.branch_value);
  if (!value.result_verified || !value.restore_verified || !value.receipt ||
      !crafting_detail::valid_key(value.key) || value.key.kind != d->station || !value.quantity ||
      value.quantity > static_cast<std::uint32_t>(INT32_MAX) ||
      value.produced_count != outputs.count || value.produced_count > value.produced.size() ||
      value.stock_count != d->targets.count || value.stock_count > value.stocks.size())
    return false;
  for (std::size_t i = 0; i < value.produced_count; ++i) {
    const auto& output = value.produced[i];
    const auto units = crafting_catalog::units(outputs, output.symbol);
    if (!units || output.amount <= 0 ||
        std::int64_t{output.amount} != std::int64_t{units} * value.quantity)
      return false;
    for (std::size_t j = 0; j < i; ++j)
      if (value.produced[j].symbol == output.symbol)
        return false;
  }
  for (std::size_t i = 0; i < value.stock_count; ++i) {
    const auto& stock = value.stocks[i];
    bool found = false;
    for (const auto& target : d->targets)
      if (target.symbol == stock.symbol)
        found = true;
    if (!found || stock.stored < 0)
      return false;
    for (std::size_t j = 0; j < i; ++j)
      if (value.stocks[j].symbol == stock.symbol)
        return false;
  }
  return true;
}

bool compose(Frame& frame, const CraftingSession& session, CraftingMenu& menu, bool execution,
             unsigned width, unsigned height, const Table& text) noexcept {
  frame = {};
  if (!dimensions(width, height) || !valid_crafting_snapshot(session.preview()))
    return false;
  const bool narrow = width < 1600 || height < 900;
  constexpr int left = 300 + marker_gap;
  constexpr int wide_right = 4860;
  const int right = narrow ? left : wide_right;
  const std::size_t full = width < 1000 ? 40u : 68u, detail_limit = narrow ? full : 48u;
  // Leave a gutter before the right focus marker. Budget 26 pixels per glyph at
  // the two-column threshold and cap longer lines as the interface scales up.
  const auto list_pixels =
      static_cast<unsigned>(wide_right - marker_gap - left - 220) * width / 8192;
  const std::size_t list_limit = narrow ? full - 2 : std::min(40u, list_pixels / 26);
  const int first_y = narrow ? 1000 : 1250, footer_y = narrow ? 2200 : 7400;
  const int entry_step = narrow ? 300 : 400;
  const auto page_size = 1u + static_cast<unsigned>((footer_y - 220 - first_y) / entry_step);
  char line[192]{};
  if (!append(frame, 300, 280, text[L::title], full) ||
      !append(frame, left, narrow ? 650 : 800, text[L::known_recipes], list_limit))
    return false;
  auto view = menu.viewport(session, page_size);
  const auto* selected = session.selected();
  const bool recipe_focus = menu.row() == CraftingMenuRow::recipe;
  // Keep the selected recipe on screen even when a translated hint needs another line.
  const auto header_lines = frame.count;
  for (auto capacity = page_size; view.count;) {
    frame.count = header_lines;
    int y = first_y;
    std::size_t i = 0;
    for (; i < view.count; ++i) {
      const auto* entry = session.visible(view.first + i);
      if (!entry)
        return false;
      const char* hint = blocked(crafting_block_reason(*entry), text).hint;
      std::snprintf(line, sizeof(line), "%s%s%s", hint, *hint ? " " : "", text.recipe(entry->id));
      std::string_view rest(line);
      const char* marker = entry == selected ? (recipe_focus ? ">" : "[x]") : nullptr;
      while (y + 220 <= footer_y) {
        auto count = rest.size();
        if (count > list_limit) {
          const auto space = rest.rfind(' ', list_limit);
          count = space == std::string_view::npos ? list_limit : space;
        }
        char row[192];
        std::snprintf(row, sizeof(row), "%.*s", static_cast<int>(count), rest.data());
        if (!choice(frame, left, y, row, list_limit, marker))
          return false;
        rest.remove_prefix(count);
        if (rest.empty())
          break;
        if (rest.front() == ' ')
          rest.remove_prefix(1);
        marker = nullptr;
        y += 230;
      }
      if (!rest.empty())
        break;
      y += entry_step;
    }
    if (i == view.count)
      break;
    if (capacity <= 1)
      return false;
    view = menu.viewport(session, --capacity);
  }
  if (!view.count &&
      !choice(frame, left, first_y, text[L::no_recipes], list_limit, recipe_focus ? ">" : nullptr))
    return false;
  std::snprintf(line, sizeof(line), text[L::list_range], view.count ? view.first + 1 : 0,
                view.first + view.count, session.visible_count());
  if (!append(frame, left, footer_y, line, list_limit))
    return false;
  const auto* d = selected ? crafting_catalog::find(selected->id) : nullptr;
  if (!wrapped(frame, right, narrow ? 2500 : 700, d ? text.recipe(d->id) : text[L::select_recipe],
               detail_limit))
    return false;
  const auto mark = [&](CraftingMenuRow row, unsigned column = 0) -> const char* {
    return menu.row() == row && menu.column() == column ? ">" : nullptr;
  };
  if (!d || !session.quantity())
    std::snprintf(line, sizeof(line), "%s", text[d ? L::zero_quantity : L::no_quantity]);
  else if (d->bow)
    std::snprintf(line, sizeof(line), text[L::bow_quantity], session.quantity(), session.maximum());
  else
    std::snprintf(line, sizeof(line), text[L::quantity], session.quantity(), session.maximum(),
                  text[ui_text::unit(d->quantity_unit)]);
  if (!choice(frame, right, narrow ? 3050 : 1450, line, detail_limit,
              mark(CraftingMenuRow::quantity)))
    return false;
  const int cell_width = width < 1000 ? 850 : 570;
  const char* names[] = {"1", "5", "10", text[L::max_short]};
  for (unsigned i = 0; i < 4; ++i) {
    const int x = right + static_cast<int>(i) * cell_width;
    const bool enabled = CraftingMenu::preset_enabled(i, session.maximum());
    std::snprintf(line, sizeof(line), enabled ? "[%s]" : "(%s)", names[i]);
    if (!choice(frame, x, narrow ? 3370 : 1870, line, detail_limit,
                mark(CraftingMenuRow::presets, i)))
      return false;
    if (i < 2 && !choice(frame, x, narrow ? 3690 : 2290, i ? "[+5]" : "[-5]", detail_limit,
                         mark(CraftingMenuRow::steps, i)))
      return false;
  }
  int y = narrow ? 4070 : 2850;
  std::size_t material_count = 0;
  if (selected && d && d->grind && selected->grind.known) {
    const auto progress =
        crafting_grind_plan(*selected, session.quantity() ? session.quantity() : 1);
    if (progress.valid)
      std::snprintf(line, sizeof(line), text[L::grind_progress], progress.skill_before,
                    progress.skill_after);
    else
      std::snprintf(line, sizeof(line), text[L::grind_skill], selected->grind.skill);
    if (!append(frame, right, y, line, detail_limit))
      return false;
    y += 320;
  }
  if (selected && d)
    for (std::size_t i = 0; i < selected->target_count; ++i)
      if (relevant(selected->materials[i], *d))
        ++material_count;
  if (material_count > crafting_catalog::target_capacity)
    return false;
  int material_step = narrow ? 560 : 610, detail_step = 260;
  // All actual ingredients, outputs and tools remain visible.
  // Six-line recipes divide the same available detail area evenly.
  // Existing shorter previews keep their previous coordinates.
  const int material_bottom = narrow ? 6720 : 5900;
  if (material_count &&
      y + static_cast<int>(material_count - 1) * material_step + detail_step > material_bottom) {
    const int line_step = (material_bottom - y) / static_cast<int>(material_count * 2 - 1);
    if (line_step <= 0)
      return false;
    material_step = std::min(material_step, line_step * 2);
    detail_step = std::min(detail_step, line_step);
  }
  if (selected && d)
    for (std::size_t i = 0; i < selected->target_count; ++i) {
      const auto& row = selected->materials[i];
      if (!relevant(row, *d))
        continue;
      const auto hand = crafting_catalog::hand_amounts(*d, row.symbol);
      const auto p = crafting_amount_preview(row, session.quantity() ? session.quantity() : 1);
      const bool held_amount_unknown = hand.consumed && row.consumed_reserve < 0;
      if (hand.tool && p.end_known)
        std::snprintf(line, sizeof(line), text[L::tool_held], text.item(row.symbol));
      else if (held_amount_unknown)
        std::snprintf(line, sizeof(line), text[L::hand_needed], text.item(row.symbol),
                      static_cast<long long>(p.required));
      else if (hand.consumed)
        std::snprintf(line, sizeof(line), text[L::hand_cost], text.item(row.symbol), hand.consumed);
      else if (row.fixed_minimum)
        std::snprintf(line, sizeof(line), text[L::possession], text.item(row.symbol));
      else if (row.unit_minimum && row.unit_minimum != row.unit_cost)
        std::snprintf(line, sizeof(line), text[L::minimum], text.item(row.symbol),
                      static_cast<long long>(p.original_minimum));
      else
        std::snprintf(line, sizeof(line), "%s%s", text.item(row.symbol),
                      row.unit_output ? text[L::output_suffix] : "");
      const bool long_name = std::strlen(line) > detail_limit;
      if (long_name) {
        if (!wrapped(frame, right, y, line, detail_limit, 260))
          return false;
      } else if (!append(frame, right, y, line, detail_limit))
        return false;
      if (hand.tool) {
        if (row.stored < 0)
          std::snprintf(line, sizeof(line), "%s", text[L::tool_stock_unknown]);
        else if (!p.end_known)
          std::snprintf(line, sizeof(line), text[L::return_unknown], row.stored);
        else
          std::snprintf(
              line, sizeof(line), text[L::output_balance], static_cast<long long>(p.ready_storage),
              static_cast<long long>(row.tool_reserve), static_cast<long long>(p.end_storage));
      } else if (held_amount_unknown)
        std::snprintf(line, sizeof(line), "%s",
                      text[row.stored < 0 ? L::stock_and_hand_unknown : L::hand_unknown]);
      else if (!p.available_known) {
        const auto needed = row.fixed_minimum ? p.original_minimum : p.required;
        std::snprintf(line, sizeof(line), text[L::stock_unknown],
                      row.unit_output ? text[L::output] : text[L::required],
                      static_cast<long long>(row.unit_output ? p.produced : needed));
      } else if (row.unit_output)
        std::snprintf(line, sizeof(line), text[L::output_balance],
                      static_cast<long long>(p.available - p.required),
                      static_cast<long long>(p.produced), static_cast<long long>(p.ready_storage));
      else if (p.minimum_missing)
        std::snprintf(line, sizeof(line), text[L::missing_minimum],
                      static_cast<long long>(p.available),
                      static_cast<long long>(p.minimum_missing));
      else if (p.missing)
        std::snprintf(line, sizeof(line), text[L::shortfall], static_cast<long long>(p.required),
                      static_cast<long long>(p.missing));
      else
        std::snprintf(line, sizeof(line), "%lld - %lld = %lld", static_cast<long long>(p.available),
                      static_cast<long long>(p.required), static_cast<long long>(p.ready_storage));
      const int extra_name_line = long_name ? 260 : 0;
      if (!append(frame, right, y + detail_step + extra_name_line, line, detail_limit))
        return false;
      y += material_step + extra_name_line;
    }
  const auto action_reason = session.reason();
  const auto action_text = blocked(action_reason, text);
  const char* reason = action_text.detail;
  if (!session.quantity() && session.selected() && action_reason == CraftingBlockReason::none)
    reason = text[L::one_batch];
  if (!wrapped(frame, right, narrow ? 6950 : 6160, reason, detail_limit, 240))
    return false;
  const bool specific = action_reason != CraftingBlockReason::none &&
                        action_reason != CraftingBlockReason::unavailable;
  const char* suffix = session.can_confirm() ? ""
                       : specific            ? action_text.hint
                                             : text[L::locked_suffix];
  char action[96]{};
  std::snprintf(action, sizeof(action), "%s%s%s", execution ? text[L::craft] : text[L::test_amount],
                specific ? " " : "", suffix);
  if (!choice(frame, right, narrow ? 7410 : 6980, action, detail_limit,
              mark(CraftingMenuRow::manufacture)) ||
      !choice(frame, right, narrow ? 7640 : 7400, text[L::original_controls], detail_limit,
              mark(CraftingMenuRow::original)))
    return false;
  const auto keys = key_hint(menu.row());
  std::snprintf(line, sizeof(line), text[full == 40 ? L::controls_short : L::controls],
                text[keys.confirm], text[keys.navigation]);
  const char* status = line;
  if (session.phase() == SelectionPhase::releasing || session.phase() == SelectionPhase::ready)
    status = text[L::release_continue];
  else if (session.phase() == SelectionPhase::original)
    status = text[L::release_back];
  else if (!session.armed())
    status = text[L::release_buttons];
  else if (session.issue() == SelectionIssue::stock_changed ||
           session.issue() == SelectionIssue::offer_changed)
    status = text[L::stock_changed];
  else if (session.issue() == SelectionIssue::focus_lost)
    status = text[L::focus_lost];
  if (!append(frame, 300, 7860, status, full))
    return false;
  frame.valid = true;
  return true;
}

bool compose_success(Frame& frame, const NativeCraftingSuccess& value, unsigned width,
                     unsigned height, const Table& text) noexcept {
  frame = {};
  if (!dimensions(width, height) || !valid_success(value))
    return false;
  const auto& d = *crafting_catalog::find(value.id);
  const std::size_t limit = width < 1000 ? 40u : 68u;
  char line[192]{};
  if (!append(frame, 300, 450, text[L::success], limit) ||
      !wrapped(frame, 300, 1050, text.recipe(d.id), limit))
    return false;
  std::snprintf(line, sizeof(line), text[L::completed], value.quantity,
                text[ui_text::unit(d.quantity_unit)]);
  if (!append(frame, 300, 1800, line, limit))
    return false;
  int y = 2380;
  for (std::size_t i = 0; i < value.produced_count; ++i) {
    const auto& output = value.produced[i];
    std::snprintf(line, sizeof(line), "%s: +%d", text.item(output.symbol), output.amount);
    if (!wrapped(frame, 300, y, line, limit))
      return false;
    y += std::strlen(line) > limit ? 620 : 360;
  }
  if (!append(frame, 300, 3250, text[L::updated_stock], limit))
    return false;
  y = 3730;
  for (std::size_t i = 0; i < value.stock_count; ++i) {
    const auto& stock = value.stocks[i];
    const auto hand = crafting_catalog::hand_amounts(d, stock.symbol);
    if (!crafting_catalog::units(d.costs, stock.symbol) &&
        !crafting_catalog::units(
            crafting_catalog::outputs_for(d, value.branch_known, value.branch_value),
            stock.symbol) &&
        !hand.tool)
      continue;
    std::snprintf(line, sizeof(line), "%s: %d%s", text.item(stock.symbol), stock.stored,
                  hand.tool ? text[L::tool_suffix] : "");
    if (!wrapped(frame, 300, y, line, limit))
      return false;
    y += std::strlen(line) > limit ? 690 : 430;
  }
  if (!wrapped(frame, 300, 6800, text[L::back_to_recipes], limit) ||
      !append(frame, 300, 7590, text[L::no_repeat], limit))
    return false;
  frame.valid = true;
  return true;
}
bool compose_failure(Frame& frame, unsigned width, unsigned height, const Table& text) noexcept {
  frame = {};
  if (!dimensions(width, height))
    return false;
  const std::size_t limit = width < 1000 ? 40u : 68u;
  if (!wrapped(frame, 300, 800, text[L::failure], limit) ||
      !wrapped(frame, 300, 2000, text[L::uncertain], limit) ||
      !wrapped(frame, 300, 3100, text[L::locked_session], limit) ||
      !wrapped(frame, 300, 4350, text[L::quit_unsaved], limit) ||
      !wrapped(frame, 300, 6600, text[L::dismiss], limit))
    return false;
  frame.valid = true;
  return true;
}
} // namespace ubc::crafting_ui

#if defined(_WIN32) && defined(_M_IX86)
#include <windows.h>
#include "ubc/core/sha256.hpp"
#include "ubc/engine/windows_probe.hpp"
namespace ubc {
namespace {
namespace m = native_menu;
namespace w = windows_probe;
void* pointer(std::uintptr_t address) noexcept {
  std::uint32_t result = 0;
  return address <= UINT32_MAX &&
                 w::copy_memory(nullptr, static_cast<std::uint32_t>(address), &result, 4)
             ? reinterpret_cast<void*>(result)
             : nullptr;
}
class Text {
public:
  explicit Text(const char* value) {
    reinterpret_cast<m::StringConstructor>(m::string_ctor)(data_.bytes.data(), value);
  }
  ~Text() { reinterpret_cast<m::ObjectFunction>(m::string_dtor)(data_.bytes.data()); }
  const void* get() const noexcept { return data_.bytes.data(); }

private:
  m::StringStorage data_{};
};
struct BackdropFunction {
  std::uintptr_t address;
  std::size_t length;
  lhiver::Digest digest;
  const char* mismatch;
};
// Union 1.0m corrects four viewport edges and selects farZ 65534 in Blit.
// Both complete bodies draw the same tile before the same text call.
constexpr auto union_blit =
    lhiver::digest_literal("996de8ea410b89b6298273c34c0b256ff38aa3275c07f1f6c6de1832b337f2f7");
constexpr BackdropFunction backdrop_functions[] = {
    {m::view_insert_back, 253,
     lhiver::digest_literal("a4ba95d10af5a714014d8f95036e6656369cf4485602bb3677f5e2d786ca63e1"),
     "ui_backdrop_insert_mismatch"},
    {m::view_set_blend, 12,
     lhiver::digest_literal("8b1870c33b0980ec04fdb4f6ecc447d847f94bd7810e63593fe5a451ca96a946"),
     "ui_backdrop_blend_mismatch"},
    {m::view_set_alpha, 10,
     lhiver::digest_literal("b4f3c0e5997d991ad194c8fd1a9871303aefe560b0803af4a518d7cd41634389"),
     "ui_backdrop_alpha_mismatch"},
    {m::view_blit, 899,
     lhiver::digest_literal("da926dc025f49cbe9e20a4c62a0a401bbf71335c5541779f2923fbc649090c3c"),
     "ui_backdrop_blit_mismatch"},
};
bool shift() noexcept {
  auto* input = pointer(m::input_pointer);
  if (!input || !w::readable(input, 4))
    return false;
  const auto table = m::read<std::uint32_t>(input, 0);
  if (table > UINT32_MAX - 4 * m::key_pressed_slot)
    return false;
  auto* function = pointer(table + 4 * m::key_pressed_slot);
  if (!w::readable(function, 1, true))
    return false;
  const auto pressed = reinterpret_cast<m::KeyPressed>(function);
  return pressed(input, 0x2a) || pressed(input, 0x36);
}
CraftingMenuAction action(int key) noexcept {
  using A = CraftingMenuAction;
  switch (key) {
  case m::key_return:
    return A::confirm;
  case m::key_escape:
    return A::back;
  case m::key_left:
    return A::left;
  case m::key_right:
    return A::right;
  case m::key_up:
    return A::up;
  case m::key_down:
    return A::down;
  case 0x0f:
    return shift() ? A::previous : A::next;
  case 0xc9:
    return A::page_up;
  case 0xd1:
    return A::page_down;
  default:
    break;
  }
  if (key <= 0 || key > 0xffff)
    return A::none;
  auto* input = pointer(m::input_pointer);
  if (!input || !w::readable(input, 4))
    return A::none;
  const auto logical = reinterpret_cast<m::LogicalKey>(m::input_logical_key)(
      input, static_cast<unsigned short>(key));
  switch (logical) {
  case m::game_left:
  case m::game_strafe_left:
    return A::left;
  case m::game_right:
  case m::game_strafe_right:
    return A::right;
  case m::game_up:
    return A::up;
  case m::game_down:
    return A::down;
  default:
    return A::none;
  }
}
bool size(unsigned& width, unsigned& height) noexcept {
  DWORD process = 0;
  const auto window = GetForegroundWindow();
  RECT area{};
  if (!window || !GetWindowThreadProcessId(window, &process) || process != GetCurrentProcessId() ||
      !GetClientRect(window, &area) || area.right <= area.left || area.bottom <= area.top)
    return false;
  const auto wide = std::int64_t{area.right} - area.left,
             high = std::int64_t{area.bottom} - area.top;
  if (wide > 32768 || high > 32768)
    return false;
  width = static_cast<unsigned>(wide);
  height = static_cast<unsigned>(high);
  return true;
}
} // namespace
void NativeCraftingUi::inspect_open(BowMenuTrace& trace) const noexcept {
  unsigned width = width_, height = height_;
  size(width, height);
  trace.equal("ui.opened", 0, opened());
  trace.value("ui.width", ">=640", width, width >= 640);
  trace.value("ui.height", ">=480", height, height >= 480);
  window_.can_open(&trace);
  // The constructor results can only be read during the actual attempt to open the window.
  trace.pending("ui.vtable", "Menue-VTable", "vor Konstruktor");
  trace.pending("ui.item_count", "0", "vor Konstruktor");
  trace.pending("ui.initialized", "1", "vor ScreenInit");
  trace.pending("ui.input_owner", "eigenes Menue", "vor ScreenInit");
}
bool NativeCraftingUi::open(BowMenuTrace* trace, BowMenuTrace* opened_trace) noexcept {
  if (opened()) {
    if (trace)
      trace->equal("ui.opened", 0, 1);
    return false;
  }
  if (!update_size()) {
    if (trace) {
      trace->value("ui.width", ">=640", width_, width_ >= 640);
      trace->value("ui.height", ">=480", height_, height_ >= 480);
    }
    return false;
  }
  if (!window_.open(trace, opened_trace))
    return false;
  mode_ = Mode::selection;
  menu_.reset();
  dismiss_ = false;
  notice_input_ = {};
  success_ = {};
  submitted_receipt_ = 0;
  return true;
}
crafting_ui::MenuView NativeCraftingUi::menu_view() noexcept {
  return {opened() ? object() : nullptr, {nullptr, w::copy_memory}};
}
namespace {
const char* apply_view_backdrop(void* view) noexcept {
  if (m::read<std::uintptr_t>(view, m::view_back_texture))
    return "ui_backdrop_texture_in_use";
  for (const auto& function : backdrop_functions) {
    const auto* code = reinterpret_cast<const void*>(function.address);
    if (!w::readable(code, function.length, true, true))
      return function.mismatch;
    Sha256 hash;
    hash.update(code, function.length);
    const auto actual = hash.finish();
    if (actual != function.digest && !(function.address == m::view_blit && actual == union_blit))
      return function.mismatch;
  }
  // The window owns the texture reference until ScreenDone destroys it.
  Text black("BLACK.TGA");
  reinterpret_cast<m::ViewInsertBack>(m::view_insert_back)(view, black.get());
  if (!m::read<std::uintptr_t>(view, m::view_back_texture))
    return "ui_backdrop_texture_missing";
  reinterpret_cast<m::ViewSetBlend>(m::view_set_blend)(view, m::alpha_blend);
  reinterpret_cast<m::ViewSetAlpha>(m::view_set_alpha)(view, m::backdrop_opacity);
  return "ui_backdrop_ready";
}
} // namespace
crafting_ui::BackdropResult NativeCraftingUi::apply_backdrop() noexcept {
  return menu_view().backdrop(apply_view_backdrop);
}
bool NativeCraftingUi::update_size() noexcept {
  unsigned width = 0, height = 0;
  if (size(width, height)) {
    width_ = width;
    height_ = height;
  }
  // Losing focus is not a rendering error. The last verified format of
  // our window remains valid while the input gate blocks input separately.
  return width_ >= 640 && height_ >= 480;
}
bool NativeCraftingUi::notice_window() noexcept {
  return update_size() && (opened() ? owns_input() : window_.open());
}
bool NativeCraftingUi::open_failure() noexcept {
  if (mode_ == Mode::failure && opened())
    return owns_input();
  if (!notice_window())
    return false;
  mode_ = Mode::failure;
  dismiss_ = false;
  notice_input_ = {};
  submitted_receipt_ = 0;
  return true;
}
bool NativeCraftingUi::open_success(const NativeCraftingSuccess& value) noexcept {
  if (mode_ == Mode::failure || mode_ == Mode::success || value.receipt <= last_receipt_ ||
      !crafting_ui::valid_success(value) || !notice_window())
    return false;
  success_ = value;
  last_receipt_ = value.receipt;
  submitted_receipt_ = 0;
  mode_ = Mode::success;
  dismiss_ = false;
  notice_input_ = {};
  return true;
}
void NativeCraftingUi::close() noexcept {
  window_.close();
  mode_ = Mode::selection;
  success_ = {};
  submitted_receipt_ = 0;
  dismiss_ = false;
  notice_input_ = {};
  menu_.reset();
}
int NativeCraftingUi::handle(CraftingSession& session, int key,
                             crafting_ui_diagnostics::Action* diagnostic) noexcept {
  if (diagnostic)
    *diagnostic = {};
  if (mode_ != Mode::selection) {
    if (!focused()) {
      notice_input_.observe(0, false, false);
      return 1;
    }
    const auto a = action(key);
    if ((a == CraftingMenuAction::confirm || a == CraftingMenuAction::back) &&
        notice_input_.press())
      dismiss_ = true;
    return 1;
  }
  const bool has_focus = focused();
  const auto observed = crafting_ui_diagnostics::dispatch(
      menu_, session, key, has_focus ? action(key) : CraftingMenuAction::none, has_focus);
  if (diagnostic)
    *diagnostic = observed;
  return 1;
}
bool NativeCraftingUi::submit() noexcept {
  return frame_.valid && menu_view().submit([this](void* view) {
    reinterpret_cast<m::ObjectFunction>(m::view_clear_printwin)(view);
    Text font("FONT_OLD_10_WHITE.TGA");
    reinterpret_cast<m::ViewSetFont>(m::view_set_font)(view, font.get());
    for (std::size_t i = 0; i < frame_.count; ++i) {
      const auto& line = frame_.lines[i];
      Text text(line.text.data());
      reinterpret_cast<m::ViewPrint>(m::view_print)(view, line.x, line.y, text.get());
    }
  });
}
bool NativeCraftingUi::render(const CraftingSession& session, bool execution_enabled) noexcept {
  if (mode_ == Mode::failure)
    return render_failure();
  if (mode_ == Mode::success)
    return render_success();
  return update_size() &&
         crafting_ui::compose(frame_, session, menu_, execution_enabled, width_, height_,
                              window_.texts()) &&
         submit();
}
bool NativeCraftingUi::render_failure() noexcept {
  return mode_ == Mode::failure && update_size() &&
         crafting_ui::compose_failure(frame_, width_, height_, window_.texts()) && submit();
}
bool NativeCraftingUi::render_success() noexcept {
  if (mode_ != Mode::success || !update_size())
    return false;
  submitted_receipt_ = crafting_ui::submit_success(
      frame_, success_, width_, height_, window_.texts(), [this](const auto&) { return submit(); });
  return submitted_receipt_ != 0;
}
} // namespace ubc
#endif
