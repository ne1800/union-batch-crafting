// SPDX-License-Identifier: MIT
#include "ubc/ui/native_crafting_ui.hpp"
#include "ubc/stations/bow/bow_menu_trace.hpp"
#include "ubc/core/crafting_result.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

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
const char* blocked(CraftingBlockReason reason, const Table& text) noexcept {
  switch (reason) {
  case CraftingBlockReason::none:
    return "";
  case CraftingBlockReason::no_known_recipes:
    return text[L::no_recipes];
  case CraftingBlockReason::contract_unverified:
    return text[L::unverified];
  case CraftingBlockReason::actor_unknown:
    return text[L::no_actor];
  case CraftingBlockReason::actor_changed:
    return text[L::wrong_actor];
  case CraftingBlockReason::hand_unknown:
    return text[L::hand_not_ready];
  case CraftingBlockReason::material_unknown:
    return text[L::stock_unreadable];
  case CraftingBlockReason::material_unverified:
    return text[L::stock_unverified];
  case CraftingBlockReason::missing_material:
    return text[L::missing_material];
  case CraftingBlockReason::output_capacity:
    return text[L::output_full];
  case CraftingBlockReason::tool_return_capacity:
    return text[L::tool_full];
  case CraftingBlockReason::unavailable:
    return text[L::unavailable];
  case CraftingBlockReason::quantity_unsupported:
    return text[L::unsupported_amount];
  }
  return text[L::unavailable];
}
bool relevant(const CraftingMaterialValue& row, const crafting_catalog::Descriptor& d) noexcept {
  return row.fixed_minimum || row.unit_cost || row.unit_output ||
         crafting_catalog::hand_amounts(d, row.symbol).tool;
}
bool presets(char* buffer, std::size_t capacity, const CraftingSession& session,
             const CraftingMenu& menu, const Table& text) noexcept {
  const char* names[] = {"1", "5", "10", text[L::max_short]};
  std::size_t used = 0;
  for (unsigned i = 0; i < 4; ++i) {
    const bool enabled = CraftingMenu::preset_enabled(i, session.maximum());
    const int n =
        std::snprintf(buffer + used, capacity - used, "%s%s%s%s ",
                      menu.row() == CraftingMenuRow::presets && menu.column() == i ? ">" : "",
                      enabled ? "[" : "(", names[i], enabled ? "]" : ")");
    if (n < 0 || static_cast<std::size_t>(n) >= capacity - used)
      return false;
    used += static_cast<std::size_t>(n);
  }
  return true;
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
  const std::size_t full = width < 1000 ? 40u : 68u, list_limit = narrow ? full : 28u,
                    detail_limit = narrow ? full : 48u;
  const int right = narrow ? 300 : 3350;
  char line[192]{};
  if (!append(frame, 300, 280, text[L::title], full))
    return false;
  const auto view = menu.viewport(session, narrow ? 4u : 6u);
  const auto* selected = session.selected();
  if (!append(frame, 300, narrow ? 650 : 800,
              menu.row() == CraftingMenuRow::recipe ? text[L::selected_recipes]
                                                    : text[L::known_recipes],
              list_limit))
    return false;
  for (std::size_t i = 0; i < view.count; ++i) {
    const auto* entry = session.visible(view.first + i);
    if (!entry)
      return false;
    const auto* d = crafting_catalog::find(entry->id);
    if (!d)
      return false;
    std::snprintf(
        line, sizeof(line), "%s%s%s", selected && selected->id == entry->id ? "> " : "  ",
        crafting_block_reason(*entry) == CraftingBlockReason::none ? "" : text[L::locked_prefix],
        text.recipe(d->id));
    const int y = (narrow ? 1000 : 1250) + static_cast<int>(i) * (narrow ? 300 : 500);
    if (narrow) {
      if (!append(frame, 300, y, line, list_limit))
        return false;
    } else if (!wrapped(frame, 300, y, line, list_limit, 230))
      return false;
  }
  if (!view.count && !append(frame, 300, narrow ? 1000 : 1250, text[L::no_recipes], list_limit))
    return false;
  std::snprintf(line, sizeof(line), text[L::list_range], view.count ? view.first + 1 : 0,
                view.first + view.count, session.visible_count());
  if (!append(frame, 300, narrow ? 2200 : 4550, line, list_limit))
    return false;
  const auto* d = selected ? crafting_catalog::find(selected->id) : nullptr;
  if (!wrapped(frame, right, narrow ? 2500 : 700, d ? text.recipe(d->id) : text[L::select_recipe],
               detail_limit))
    return false;
  const auto mark = [&](CraftingMenuRow row) { return menu.row() == row ? ">" : " "; };
  std::snprintf(line, sizeof(line), text[L::quantity], mark(CraftingMenuRow::quantity),
                session.quantity(), session.maximum(),
                d ? text[ui_text::unit(d->quantity_unit)] : "");
  if (d && d->bow)
    std::snprintf(line, sizeof(line), text[L::bow_quantity], mark(CraftingMenuRow::quantity),
                  session.quantity(), session.maximum());
  if (!session.quantity())
    std::snprintf(line, sizeof(line), text[L::zero_quantity], mark(CraftingMenuRow::quantity));
  if (!append(frame, right, narrow ? 3050 : 1450, line, detail_limit))
    return false;
  if (!presets(line, sizeof(line), session, menu, text) ||
      !append(frame, right, narrow ? 3370 : 1870, line, detail_limit))
    return false;
  std::snprintf(line, sizeof(line), text[L::steps],
                menu.row() == CraftingMenuRow::steps && menu.column() == 0 ? ">" : " ",
                menu.row() == CraftingMenuRow::steps && menu.column() == 1 ? ">" : " ");
  if (!append(frame, right, narrow ? 3690 : 2290, line, detail_limit))
    return false;
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
      if (hand.tool)
        std::snprintf(line, sizeof(line), text[L::tool_held], text.item(row.symbol));
      else if (hand.consumed)
        std::snprintf(line, sizeof(line), "%s (%s)", text.item(row.symbol),
                      row.consumed_reserve < 0 ? text[L::hand_unknown] : text[L::hand_cost]);
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
          std::snprintf(line, sizeof(line), text[L::return_unknown], row.stored, row.stored);
        else
          std::snprintf(line, sizeof(line), text[L::tool_balance],
                        static_cast<long long>(p.ready_storage),
                        static_cast<long long>(p.end_storage));
      } else if (!p.available_known)
        std::snprintf(line, sizeof(line), text[L::stock_unknown],
                      row.unit_output ? text[L::output] : text[L::required],
                      static_cast<long long>(row.unit_output ? p.produced : p.required));
      else if (row.unit_output)
        std::snprintf(line, sizeof(line), text[L::output_balance],
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
  const char* reason = blocked(session.reason(), text);
  if (!session.quantity() && session.selected() && session.reason() == CraftingBlockReason::none)
    reason = text[L::one_batch];
  if (!wrapped(frame, right, narrow ? 6990 : 6160, reason, detail_limit, 240))
    return false;
  if (narrow) {
    std::snprintf(line, sizeof(line), text[L::actions], mark(CraftingMenuRow::manufacture),
                  execution ? text[L::craft] : text[L::test_amount],
                  session.can_confirm() ? "" : text[L::locked_suffix],
                  mark(CraftingMenuRow::original));
    if (!append(frame, 300, 7470, line, full))
      return false;
  } else {
    std::snprintf(line, sizeof(line), "%s %s%s", mark(CraftingMenuRow::manufacture),
                  execution ? text[L::craft] : text[L::test_amount],
                  session.can_confirm() ? "" : text[L::locked_suffix]);
    if (!append(frame, right, 6980, line, detail_limit))
      return false;
    std::snprintf(line, sizeof(line), text[L::original_action], mark(CraftingMenuRow::original));
    if (!append(frame, right, 7400, line, detail_limit))
      return false;
  }
  const char* status = full == 40 ? text[L::controls_short] : text[L::controls];
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
  if (!opened() || !frame_.valid)
    return false;
  auto* view = reinterpret_cast<void*>(m::read<std::uint32_t>(object(), m::menu_inner_window));
  if (!w::readable(view, m::view_size) || m::read<std::int32_t>(view, m::view_id) != m::view_item)
    return false;
  reinterpret_cast<m::ObjectFunction>(m::view_clear_printwin)(view);
  Text font("FONT_OLD_10_WHITE.TGA");
  reinterpret_cast<m::ViewSetFont>(m::view_set_font)(view, font.get());
  for (std::size_t i = 0; i < frame_.count; ++i) {
    const auto& line = frame_.lines[i];
    Text text(line.text.data());
    reinterpret_cast<m::ViewPrint>(m::view_print)(view, line.x, line.y, text.get());
  }
  return true;
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
