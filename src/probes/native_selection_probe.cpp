// SPDX-License-Identifier: MIT
#include <windows.h>
#include <intrin.h>
#include <array>
#include <cstdio>
#include <cstring>
#include "ubc/engine/frame_bridge.hpp"
#include "ubc/ui/native_selection_ui.hpp"
#include "ubc/engine/runtime_script.hpp"
#include "ubc/core/selection_snapshot.hpp"
#include "ubc/engine/windows_probe.hpp"
#ifdef UBC_FREEAMOUNTS
#include "ubc/core/amount_profile.hpp"
#include "ubc/core/batch_amount_call.hpp"
#include "ubc/core/batch_result.hpp"
#include "ubc/engine/batch_call_bridge.hpp"
#include "ubc/ui/native_batch_ui.hpp"
#endif

namespace {
namespace a = ubc::selection_abi;
namespace s = ubc::station_abi;
namespace m = ubc::native_menu;
namespace w = ubc::windows_probe;
namespace p = ubc::selection_profile;
using Phase = ubc::SelectionPhase;
constexpr ubc::ReadOnlyMemory memory{nullptr, w::copy_memory};
HMODULE module = nullptr;
HANDLE log_file = INVALID_HANDLE_VALUE;
volatile LONG installed = 0, engine_thread = 0;
bool execution_enabled = false, disabled = false;
unsigned frame_depth = 0, update_depth = 0, info_depth = 0, choice_depth = 0, station_depth = 0,
         collection_depth = 0;
std::uint64_t frame = 0, generation = 0;
std::size_t log_bytes = 0, log_events = 0;
ubc::SelectionInfoRows rows;
bool choice_collection = false;
std::size_t choice_count = 0;
std::array<std::int32_t, 256> choice_functions{};
ubc::PackedInventoryWorkspace inventory_workspace;
#ifdef UBC_FREEAMOUNTS
ubc::BatchSession session;
ubc::NativeBatchUi ui;
using SessionSnapshot = ubc::BatchSnapshot;
bool effect_authorized = false;
ubc::AmountPatch amount_patch;
ubc::BatchResult batch_result;
ubc::MaterialPreview expected_result;
ULONGLONG result_started = 0;
bool failure_notified = false, batch_end_witness = false;
struct AmountDiagnostic {
  bool pending = false, entered = false, restored = false;
  int rejected = 0, duration = 0, symbol = 0, quantity = 0;
} amount_diagnostic;

#else
ubc::SelectionSession session;
ubc::NativeSelectionUi ui;
using SessionSnapshot = ubc::SelectionSnapshot;
#endif
ubc::OriginalRecipe recipe = ubc::OriginalRecipe::meat;
std::uint32_t borrowed_view = 0, original_station = 0;
ubc::SelectionSnapshot handed_snapshot;
ubc::OriginalOffer handed_offer;
std::uint32_t handed_quantity = 0;
bool pending = false, executed = false, recover = false, recovering = false, native_update = false,
     handoff_cancelled = false;
unsigned effect_calls = 0;
int last_frame_state = -1, last_capture_issue = -1;
bool observe_stock_after = false;

using Start = void(__thiscall*)(void*, void*);
using End = void(__thiscall*)(void*, void*, int);
using Info = void(__thiscall*)(void*);
a::FastObject original_update = nullptr, original_choice = nullptr, original_clear = nullptr;
a::AddChoice original_add = nullptr;
a::ResolveInfo original_resolve = nullptr;
Info original_info = nullptr;
Start original_start = nullptr;
End original_end = nullptr;
m::MenuHandleEvent original_menu_event = nullptr;

constexpr s::HookSite menu_event_hook{
    m::menu_handle_event,
    5,
    12,
    {{0x53, 0x55, 0x56, 0x8b, 0xf1, 0x57, 0x8b, 0xbe, 0x40, 0x0c, 0x00, 0x00}}};
#ifdef UBC_FREEAMOUNTS
constexpr std::array<s::HookSite, 11> sites{
    {a::frame_hook,
     a::add_hook,
     a::clear_hook,
     s::hook_sites[2],
     a::resolve_hook,
     s::hook_sites[3],
     s::hook_sites[4],
     s::hook_sites[0],
     s::hook_sites[1],
     menu_event_hook,
     {0x7929f0, 7, 27, {{0x6a, 0xff, 0x68, 0x00, 0xa9, 0x82, 0x00, 0x64, 0xa1,
                         0x00, 0x00, 0x00, 0x00, 0x50, 0x64, 0x89, 0x25, 0x00,
                         0x00, 0x00, 0x00, 0x81, 0xec, 0x14, 0x01, 0x00, 0x00}}}}};
#else
constexpr std::array<s::HookSite, 10> sites{
    {a::frame_hook, a::add_hook, a::clear_hook, s::hook_sites[2], a::resolve_hook, s::hook_sites[3],
     s::hook_sites[4], s::hook_sites[0], s::hook_sites[1], menu_event_hook}};
#endif
w::Hooks<s::HookSite, sites.size()> hooks(sites);

template <class T> bool read(std::uint32_t address, T& out) noexcept {
  return memory.copy(address, &out, sizeof(out));
}
template <class T> bool field(std::uint32_t object, std::uint32_t offset, T& out) noexcept {
  return object && offset <= UINT32_MAX - object && read(object + offset, out);
}
std::int32_t integer(std::uint32_t object, std::uint32_t offset) noexcept {
  std::int32_t value = -1;
  field(object, offset, value);
  return value;
}
bool own_thread() noexcept {
  if (!InterlockedCompareExchange(&installed, 0, 0))
    return false;
  const auto current = static_cast<LONG>(GetCurrentThreadId());
  // Bind only from the verified outer frame, never from DLL/startup callbacks.
  return InterlockedCompareExchange(&engine_thread, 0, 0) == current;
}
void event(const char* name, int detail = 0, int quantity = 0) noexcept {
  if (log_file == INVALID_HANDLE_VALUE || log_events >= 10000 || log_bytes >= 4u * 1024u * 1024u)
    return;
  char line[200]{};
  const int size = std::snprintf(
      line, sizeof(line), "{\"event\":\"%s\",\"frame\":%llu,\"detail\":%d,\"quantity\":%d}\r\n",
      name, static_cast<unsigned long long>(frame), detail, quantity);
  if (size <= 0 || static_cast<std::size_t>(size) >= sizeof(line))
    return;
  DWORD written = 0;
  WriteFile(log_file, line, static_cast<DWORD>(size), &written, nullptr);
  log_bytes += written;
  ++log_events;
}
bool name_is(std::uint32_t object, std::uint32_t offset, const char* expected) noexcept {
  std::uint32_t data = 0;
  std::int32_t length = -1;
  const auto wanted = std::strlen(expected);
  if (wanted > 63 || !field(object, offset + 8, data) || !field(object, offset + 12, length) ||
      length < 0 || static_cast<std::size_t>(length) != wanted)
    return false;
  if (!length)
    return true;
  std::array<char, 64> text{};
  return memory.copy(data, text.data(), wanted + 1) &&
         !std::memcmp(text.data(), expected, wanted + 1);
}
bool station_owns(std::uint32_t mob, std::uint32_t hero) noexcept {
  std::uint32_t node = 0;
  if (!field(mob, 0x18c, node))
    return false;
  std::array<std::uint32_t, 64> seen{};
  std::size_t count = 0;
  unsigned matches = 0;
  while (node) {
    if (count == seen.size())
      return false;
    for (std::size_t i = 0; i < count; ++i)
      if (seen[i] == node)
        return false;
    seen[count++] = node;
    std::uint32_t position = 0, next = 0, npc = 0;
    if (!field(node, 0, position) || !field(node, 4, next) || !field(position, 0x44, npc))
      return false;
    if (npc == hero)
      ++matches;
    node = next;
  }
  return matches == 1;
}
struct Context {
  bool valid = false, ownership_known = false;
  std::uint32_t game = 0, world = 0, hero = 0, station = 0;
  ubc::OriginalRecipe recipe = ubc::OriginalRecipe::meat;
  ubc::SelectionDialog dialog{};
};
Context borrowed_context, handed_context;
Context context() noexcept {
  Context c;
  if (!read(a::game_pointer, c.game) || !c.game ||
      integer(c.game, 0) != static_cast<std::int32_t>(a::game_vtable) ||
      !field(c.game, a::game_world, c.world) || !c.world || !read(s::hero_pointer, c.hero) ||
      !c.hero || !field(c.hero, s::npc_interact_mob, c.station) || !c.station)
    return c;
  c.dialog = ubc::read_selection_dialog(memory, s::information_manager);
  if (!c.dialog.valid || c.dialog.npc != c.hero || c.dialog.player != c.hero ||
      !station_owns(c.station, c.hero) || integer(c.station, 0x1f4) != 1 ||
      integer(c.station, 0x1f8) != 1 || integer(c.station, 0x1fc) != 1)
    return c;
  c.ownership_known = true;
  const auto hero =
      ubc::runtime_script::read_symbol_address(memory, s::game_parser, ubc::lhiver::hero_symbol);
  std::uint32_t hero_bound = 0;
  if (!hero.valid || !field(hero.address, 0x1c, hero_bound) || hero_bound != c.hero)
    return c;
  const auto production =
      ubc::runtime_script::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
  if (!production.valid)
    return c;
  if (production.value == 14 && name_is(c.station, 0x1e0, "FRYINGMEAT") &&
      name_is(c.station, 0x1b8, "STOVE") && name_is(c.station, 0x1a4, "") &&
      name_is(c.station, 0x1cc, "B_USESTOVE"))
    c.recipe = ubc::OriginalRecipe::meat;
  else if (production.value == 4 && name_is(c.station, 0x1e0, "POTIONALCHEMY") &&
           name_is(c.station, 0x1a4, "ITMI_FLASK") && name_is(c.station, 0x1cc, ""))
    c.recipe = ubc::OriginalRecipe::healing_essence;
  else
    return c;
  if (c.recipe == ubc::OriginalRecipe::healing_essence) {
    const auto start = ubc::runtime_script::symbol_int(memory, s::game_parser, 20330);
    const auto learned = ubc::runtime_script::symbol_int_element(memory, s::game_parser, 1464, 0);
    if (!start.valid || start.value != 1 || !learned.valid || learned.value != 1)
      return c;
  }
  c.valid = true;
  return c;
}
bool same_context(const Context& a, const Context& b) noexcept {
  return a.valid && b.valid && a.game == b.game && a.world == b.world && a.hero == b.hero &&
         a.station == b.station && a.recipe == b.recipe && a.dialog.view == b.dialog.view &&
         a.dialog.npc == b.dialog.npc && a.dialog.player == b.dialog.player &&
         a.dialog.mode == b.dialog.mode && a.dialog.info == b.dialog.info;
}
bool same_owner(const Context& a, const Context& b) noexcept {
  return a.ownership_known && b.ownership_known && a.game == b.game && a.world == b.world &&
         a.hero == b.hero && a.station == b.station && a.dialog.view == b.dialog.view &&
         a.dialog.npc == b.dialog.npc && a.dialog.player == b.dialog.player;
}
bool positively_detached() noexcept {
  // A gap in reads does not establish the end. A confirmed new world
  // or hero instance, or a different current dialog owner, no longer
  // belongs to the consumed order. Its quarantine must not block them.
  std::uint32_t game = 0, world = 0, hero = 0, npc = 0, player = 0, mob = 0;
  if (!read(a::game_pointer, game) || !game ||
      integer(game, 0) != static_cast<std::int32_t>(a::game_vtable) ||
      !field(game, a::game_world, world) || !world || !read(s::hero_pointer, hero) || !hero)
    return false;
  if (game != handed_context.game || world != handed_snapshot.key.world ||
      hero != handed_snapshot.key.hero)
    return true;
  if (!field(s::information_manager, 0x20, npc) || !field(s::information_manager, 0x24, player))
    return false;
  if (npc && player && (npc != handed_snapshot.key.hero || player != handed_snapshot.key.hero))
    return true;
  if (!field(hero, s::npc_interact_mob, mob))
    return false;
  if (mob && mob != handed_snapshot.key.station)
    return true;
  // The station can be empty while cleanup is still in
  // progress. Only explicit manager completion confirms the end.
  if (!mob && integer(s::information_manager, 0x2c) == 1) {
    for (const auto offset : {0x30u, 0x34u, 0x38u, 0x3cu, 0x40u, 0x44u})
      if (integer(s::information_manager, offset) != 0)
        return false;
    return true;
  }
  return false;
}
void retire_detached() noexcept {
  // Only at the outer frame boundary with an observed empty update/effect
  // stack. Inside a callback the discarded order remains guarded. A
  // context change alone cannot remove an old effect already running.
  if (pending && !executed && positively_detached()) {
    handoff_cancelled = true;
    const auto dialog = ubc::read_selection_dialog(memory, s::information_manager);
    bool finished = integer(s::information_manager, 0x2c) == 1;
    for (const auto offset : {0x30u, 0x34u, 0x38u, 0x3cu, 0x40u, 0x44u})
      if (integer(s::information_manager, offset) != 0)
        finished = false;
    const bool replaced = dialog.valid && dialog.waiting && dialog.open && !dialog.done &&
                          rows.generation > handed_snapshot.key.collection;
    // A quiescent stack alone does not consume an IsDone event or a
    // closing animation. Until completion or a new ready collection,
    // block even an old effect that arrives only on the next frame.
    if (!finished && !replaced)
      return;
    pending = false;
    recover = false;
    handoff_cancelled = true;
    original_station = 0;
    event("selection_context_ended");
  }
}
bool profile_matches() noexcept {
  const auto diagnostic =
      ubc::runtime_script::verify_local_contract(memory, s::game_parser, p::profile);
  static auto last_issue = ubc::runtime_script::Issue::none;
  // A distinct limited selection contract; do not reclassify the full hash.
  if (!diagnostic.matches) {
    if (diagnostic.issue != last_issue)
      event("selection_contract_rejected", static_cast<int>(diagnostic.issue));
    last_issue = diagnostic.issue;
    return false;
  }
  last_issue = ubc::runtime_script::Issue::none;
  return true;
}
void capture_issue(int value) noexcept {
  if (last_capture_issue != value) {
    last_capture_issue = value;
    event("selection_state", value);
  }
}
bool record_stock(const Context& c, const char* stock_event, const char* hand_event) noexcept {
  // Call only at the outer frame boundary. Log numeric values only.
  // No object addresses, names or arbitrary game text in the log.
  if (!c.hero || !c.world)
    return false;
  const auto inventory =
      ubc::read_packed_inventory(memory, c.hero, p::targets, true, inventory_workspace);
  const auto hands = ubc::read_hands(memory, c.hero);
  if (!inventory.storage_totals_known || !inventory.snapshot_unchanged ||
      inventory.issue != ubc::PackedInventoryIssue::none || hands.left.state != 0 ||
      (hands.right.state != 0 && hands.right.state != 1))
    return false;
  for (std::size_t i = 0; i < inventory.total.size(); ++i)
    event(stock_event, static_cast<int>(i), inventory.total[i]);
  event(hand_event, hands.right.state ? hands.right.symbol : -1,
        hands.right.state ? hands.right.amount : 0);
  return true;
}
ubc::SelectionStock stock(const Context& c) noexcept {
  // Post-Render is the quiet outer boundary. The later synchronous effect
  // gates are inside the exact native Update path, after condition calls
  // returned; caller/depth gates exclude own reentry. No reservation is made.
  const auto inventory =
      ubc::read_packed_inventory(memory, c.hero, p::targets, true, inventory_workspace);
  const auto hands = ubc::read_hands(memory, c.hero);
  if (inventory.listed_known) {
    for (std::size_t n = 0; n < inventory.listed_nodes; ++n) {
      const auto item = ubc::info_detail::u32(inventory_workspace.records[n].node.data() + 4);
      if (item && (item == hands.left.object || item == hands.right.object))
        return {};
    }
  }
  return ubc::selection_stock(c.recipe, inventory, hands);
}
ubc::SelectionSnapshot capture(const Context& c, bool borrowed) noexcept {
  ubc::SelectionSnapshot out;
  if (!c.valid || !c.dialog.waiting || !(borrowed ? c.dialog.hidden : c.dialog.open) ||
      !rows.generation) {
    capture_issue(1);
    return out;
  }
  out.key = {generation,
             GetCurrentThreadId(),
             c.world,
             c.hero,
             c.station,
             s::information_manager,
             c.dialog.view,
             c.dialog.mode == 1 ? ubc::SelectionMode::info : ubc::SelectionMode::choice,
             -1,
             rows.generation};
  const auto inventory = stock(c);
  if (!inventory.known()) {
    capture_issue(10 + static_cast<int>(inventory.issue));
    return out;
  }
  out.stored = inventory.stored;
  out.reserved = inventory.reserved;
  const auto add = [&](std::int32_t instance, std::int32_t function, std::int32_t row,
                       std::uint32_t amount) {
#ifdef UBC_FREEAMOUNTS
    if (out.count == out.offers.size())
      return;
#else
    if (out.count == out.offers.size() || !ubc::material_preview(c.recipe, inventory, amount).valid)
      return;
#endif
    out.offers[out.count++] = {instance, function, row, amount};
  };
  if (c.recipe == ubc::OriginalRecipe::meat && rows.complete(c.dialog)) {
    for (std::size_t row = 0; row < rows.count; ++row)
      for (std::size_t j = 0; j <
#ifdef UBC_FREEAMOUNTS
                              1
#else
                              4
#endif
           ;
           ++j) {
        const auto& link = p::info_links[j];
        if (ubc::selection_info_link(rows.rows[row], link) && !rows.rows[row].first_choice) {
          constexpr std::array<std::uint32_t, 3> quantities{{1, 5, 10}};
          add(link.instance, link.function, static_cast<std::int32_t>(row),
              j < 3 ? quantities[j] : inventory.maximum);
        }
      }
  } else if (c.recipe == ubc::OriginalRecipe::healing_essence && c.dialog.mode == 2 &&
             choice_collection) {
    const auto choices = ubc::read_selection_choices(memory, c.dialog);
    if (!choices.valid || !ubc::selection_info_link(choices.owner, p::info_links[4]) ||
        choices.count != choice_count) {
      capture_issue(20);
      return {};
    }
    out.key.owner_instance = choices.owner.instance;
    for (std::size_t row = 0; row < choices.count; ++row) {
      if (choices.functions[row] != choice_functions[row]) {
        capture_issue(21);
        return {};
      }
      const auto function = choices.functions[row];
      if (function >= 20341 && function <= 20343) {
#ifdef UBC_FREEAMOUNTS
        if (function == 20342)
          continue;
#endif
        constexpr std::array<std::uint32_t, 3> quantities{{1, 3, 5}};
        add(choices.owner.instance, function, static_cast<std::int32_t>(row),
            quantities[static_cast<std::size_t>(function - 20341)]);
      }
    }
  }
  if (!out.count) {
    capture_issue(22);
    return out;
  }
  out.selection_contract = profile_matches();
  out.stock_at_rest = same_context(c, context());
  capture_issue(out.selection_contract && out.stock_at_rest ? 0 : 23);
  return out;
}

#ifdef UBC_FREEAMOUNTS
SessionSnapshot session_capture(const Context& c, bool borrowed) noexcept {
  SessionSnapshot snapshot;
  snapshot.selection = capture(c, borrowed);
  const auto material = stock(c);
  snapshot.maximum = material.maximum;
  // The separate quantity contract covers every available execution path.
  snapshot.amount_contract = material.known() && snapshot.selection.selection_contract &&
                             batch_result.permits_next() &&
                             !ubc::batch_call::diagnostics().writes_disabled;
  for (std::size_t i = 0; i < snapshot.selection.count; ++i)
    if (!ubc::amount_profile::find(
            static_cast<std::uint32_t>(snapshot.selection.offers[i].function)))
      snapshot.amount_contract = false;
  return snapshot;
}
const ubc::SelectionSnapshot& selection(const SessionSnapshot& value) noexcept {
  return value.selection;
}
const ubc::OriginalOffer& offer(const ubc::BatchRequest& value) noexcept { return value.original; }
std::uint32_t quantity(const ubc::BatchRequest& value) noexcept { return value.quantity; }
#else
SessionSnapshot session_capture(const Context& c, bool borrowed) noexcept {
  return capture(c, borrowed);
}
const ubc::SelectionSnapshot& selection(const SessionSnapshot& value) noexcept { return value; }
const ubc::OriginalOffer& offer(const ubc::OriginalOffer& value) noexcept { return value; }
std::uint32_t quantity(const ubc::OriginalOffer& value) noexcept { return value.quantity; }
#endif

void restore_view(const Context& c) noexcept {
  if (!borrowed_view)
    return;
  if (same_owner(c, borrowed_context) && c.dialog.mode == borrowed_context.dialog.mode &&
      c.dialog.info == borrowed_context.dialog.info && c.dialog.view == borrowed_view &&
      c.dialog.waiting && c.dialog.hidden) {
    reinterpret_cast<a::FastObject>(a::choice_open_immediate)(
        reinterpret_cast<void*>(borrowed_view), nullptr);
    reinterpret_cast<a::Select>(a::choice_activate)(reinterpret_cast<void*>(borrowed_view), 1);
  }
  borrowed_view = 0;
}
void close_ui(const Context& c) noexcept {
  ui.close();
  restore_view(c);
}
#ifdef UBC_FREEAMOUNTS
void observe_batch_result(const Context& c) noexcept {
  if (batch_result.state() != ubc::BatchResultState::waiting)
    return;
  ubc::BatchResultObservation observation;
  observation.identity_known = c.game && c.world && c.hero;
  observation.world = c.world;
  observation.hero = c.hero;
  observation.station_known = c.hero && field(c.hero, s::npc_interact_mob, observation.station);
  const auto production =
      ubc::runtime_script::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
  observation.ended = batch_end_witness && production.valid && production.value == 0;
  observation.reopened = c.valid && c.dialog.open && c.dialog.waiting && !c.dialog.done &&
                         c.recipe == ubc::OriginalRecipe::meat && rows.complete(c.dialog) &&
                         rows.generation > handed_snapshot.key.collection;
  if (observation.identity_known && c.world == handed_context.world &&
      c.hero == handed_context.hero && (observation.ended || observation.reopened)) {
    const auto inventory =
        ubc::read_packed_inventory(memory, c.hero, p::targets, true, inventory_workspace);
    observation.stock_known = inventory.storage_totals_known && inventory.snapshot_unchanged &&
                              inventory.stable_boundary_verified &&
                              inventory.issue == ubc::PackedInventoryIssue::none;
    observation.stored = inventory.total;
    observation.hands = ubc::read_hands(memory, c.hero);
    observation.hands_known =
        observation.hands.left.state >= 0 && observation.hands.right.state >= 0;
    const auto fresh = context();
    if (fresh.game != c.game || fresh.world != c.world || fresh.hero != c.hero ||
        fresh.station != c.station)
      observation.stock_known = false;
  }
  const auto state = batch_result.observe(observation);
  if (state == ubc::BatchResultState::waiting && GetTickCount64() - result_started > 30000)
    batch_result.fail(ubc::BatchResultIssue::timeout);
  if (batch_result.state() == ubc::BatchResultState::verified) {
    record_stock(c, "stock_verified", "hand_verified");
    event("amount_result_verified", handed_offer.function, static_cast<int>(handed_quantity));
  } else if (batch_result.state() == ubc::BatchResultState::failed) {
    record_stock(c, "stock_unconfirmed", "hand_unconfirmed");
    event("amount_result_failed", static_cast<int>(batch_result.issue()));
  }
}
#endif

void __fastcall after_frame(void* game, void* manager) {
  const auto error = GetLastError();
  std::uint32_t current = 0, owned = 0;
  if (!InterlockedCompareExchange(&installed, 0, 0) || !read(a::game_pointer, current) ||
      current != reinterpret_cast<std::uintptr_t>(game) ||
      !field(reinterpret_cast<std::uint32_t>(manager), 0xc, owned) || owned != current) {
    SetLastError(error);
    return;
  }
  const auto thread = static_cast<LONG>(GetCurrentThreadId());
  const auto owner = InterlockedCompareExchange(&engine_thread, thread, 0);
  if ((owner && owner != thread) || disabled) {
    SetLastError(error);
    return;
  }
  ++frame_depth;
  const bool boundary = frame_depth == 1 && !update_depth && !info_depth && !choice_depth &&
                        !station_depth && !collection_depth;
  if (boundary) {
    if (frame != UINT64_MAX)
      ++frame;
    else
      disabled = true;
    const auto c = context();
    retire_detached();
#ifdef UBC_FREEAMOUNTS
    observe_batch_result(c);
#endif
    const int state = (c.game ? 1 : 0) | (c.world ? 2 : 0) | (c.hero ? 4 : 0) |
                      (c.station ? 8 : 0) | (c.dialog.valid ? 16 : 0) |
                      (c.ownership_known ? 32 : 0) | (c.valid ? 64 : 0) |
                      (c.dialog.waiting ? 128 : 0) | (c.dialog.open ? 256 : 0) |
                      (c.dialog.hidden ? 512 : 0) | (c.dialog.done ? 1024 : 0);
    if (state != last_frame_state) {
      last_frame_state = state;
      event("frame_state", state, c.dialog.mode);
    }
    if (observe_stock_after && record_stock(c, "stock_after", "hand_after"))
      observe_stock_after = false;
    if (pending && !executed && (!ui.focused() || !same_context(c, handed_context)))
      handoff_cancelled = true;
    if (pending && handoff_cancelled && same_context(c, handed_context) &&
        rows.generation == handed_snapshot.key.collection && c.dialog.waiting && c.dialog.open &&
        !c.dialog.done) {
      pending = false;
      original_station = c.station;
      event("handoff_not_confirmed");
    }
    if (original_station && (!c.valid || c.station != original_station))
      original_station = 0;
    if (ui.opened()) {
#ifdef UBC_FREEAMOUNTS
      if (ui.failure_notice()) {
        ui.observe_failure(frame, ui.buttons_held(), ui.focused());
        if (!c.valid || c.dialog.view != borrowed_view || !ui.owns_input())
          ui.dismiss_failure();
        if (ui.failure_return_ready()) {
          original_station = c.station;
          close_ui(c);
        } else if (!ui.render_failure())
          ui.dismiss_failure();
        --frame_depth;
        SetLastError(error);
        return;
      }
#endif
      session.observe(frame, ui.buttons_held(), ui.focused());
      if (!c.valid || c.dialog.view != borrowed_view || !ui.owns_input())
        session.original(ubc::SelectionIssue::context_changed);
      if (session.phase() == Phase::original) {
        if (session.return_ready()) {
          original_station = c.station;
          close_ui(c);
          event("ui_original");
        } else
          ui.render(session, recipe, execution_enabled);
      } else {
        const auto fresh = session_capture(c, true);
        session.refresh(fresh);
        if (session.phase() == Phase::ready) {
          close_ui(c);
          // take() sees the original waiting view restored. Its snapshot was
          // copied before these UI-only calls; re-read all material/context now.
          const auto restored = context();
          const auto final_snapshot = session_capture(restored, false);
          const auto selected = session.take(final_snapshot, true);
          if (selected) {
            handed_snapshot = selection(final_snapshot);
            handed_context = restored;
            handed_offer = offer(*selected);
            handed_quantity = quantity(*selected);
            effect_calls = 0;
            executed = false;
            if (!execution_enabled) {
              original_station = c.station;
              observe_stock_after = true;
              event("preview_confirmed", handed_offer.function, static_cast<int>(handed_quantity));
            } else {
              pending = true;
              recover = false;
              handoff_cancelled = false;
              auto* view = reinterpret_cast<void*>(c.dialog.view);
              reinterpret_cast<a::Select>(a::choice_select)(view, handed_offer.row);
              reinterpret_cast<a::HandleEvent>(a::choice_handle_event)(
                  static_cast<std::uint8_t*>(view) + a::view_callback, m::key_return);
              const auto after = ubc::read_selection_dialog(memory, s::information_manager);
              if (!after.valid || !after.done || after.selected != handed_offer.row) {
                handoff_cancelled = true;
                event("handoff_uncertain");
              } else
                event("handoff", handed_offer.function, static_cast<int>(handed_quantity));
            }
          } else {
            event("handoff_stale");
            if (session.phase() == Phase::selecting && restored.valid && restored.dialog.open &&
                restored.dialog.waiting && ui.open()) {
              borrowed_view = restored.dialog.view;
              borrowed_context = restored;
              reinterpret_cast<a::Select>(a::choice_activate)(
                  reinterpret_cast<void*>(borrowed_view), 0);
              reinterpret_cast<a::FastObject>(a::choice_close_immediate)(
                  reinterpret_cast<void*>(borrowed_view), nullptr);
            } else
              original_station = c.station;
          }
        } else if (ui.opened() && !ui.render(session, recipe, execution_enabled))
          session.original(ubc::SelectionIssue::invalid_snapshot);
      }
    } else if (!pending && !original_station && c.valid && c.dialog.waiting && c.dialog.open &&
               ubc::NativeSelectionUi::callback_is_top(c.dialog.view + a::view_callback) &&
               integer(0x8d1e38, 8) == 0) {
      if (generation != UINT64_MAX)
        ++generation;
      else
        disabled = true;
#ifdef UBC_FREEAMOUNTS
      if (batch_result.state() == ubc::BatchResultState::failed && !failure_notified) {
        if (ui.open_failure()) {
          failure_notified = true;
          borrowed_view = c.dialog.view;
          borrowed_context = c;
          reinterpret_cast<a::Select>(a::choice_activate)(reinterpret_cast<void*>(borrowed_view),
                                                          0);
          reinterpret_cast<a::FastObject>(a::choice_close_immediate)(
              reinterpret_cast<void*>(borrowed_view), nullptr);
          ui.render_failure();
        }
        --frame_depth;
        SetLastError(error);
        return;
      }
#endif
      const auto snapshot = session_capture(c, false);
      if (!disabled && session.open(snapshot)) {
        recipe = c.recipe;
        if (ui.open()) {
          borrowed_view = c.dialog.view;
          borrowed_context = c;
          reinterpret_cast<a::Select>(a::choice_activate)(reinterpret_cast<void*>(borrowed_view),
                                                          0);
          reinterpret_cast<a::FastObject>(a::choice_close_immediate)(
              reinterpret_cast<void*>(borrowed_view), nullptr);
          event("ui_open", static_cast<int>(recipe), static_cast<int>(selection(snapshot).count));
          record_stock(c, "stock_before", "hand_before");
          if (!ui.render(session, recipe, execution_enabled))
            session.original(ubc::SelectionIssue::invalid_snapshot);
        } else {
          original_station = c.station;
          event("ui_failed");
        }
      }
    }
  }
  --frame_depth;
  SetLastError(error);
}

void __fastcall observe_clear(void* view, void* unused) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool own = own_thread();
  if (own) {
    ++collection_depth;
    rows.begin(reinterpret_cast<std::uint32_t>(view),
               caller == a::info_clear_return && collection_depth == 1);
    choice_collection = caller == a::choice_clear_return && collection_depth == 1;
    choice_count = 0;
  }
  SetLastError(error);
  original_clear(view, unused);
  const auto after = GetLastError();
  if (own) {
    if (integer(reinterpret_cast<std::uint32_t>(view), a::view_count) != 0) {
      rows.valid = false;
      choice_collection = false;
    }
    --collection_depth;
  }
  SetLastError(after);
}
void __fastcall observe_add(void* view, void* text, int number) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool own = own_thread();
  const auto address = reinterpret_cast<std::uint32_t>(view);
  const auto before = own ? integer(address, a::view_count) : -1;
  ubc::SelectionInfo info;
  ubc::ChoiceSnapshot choice;
  if (own) {
    ++collection_depth;
    const auto text_address = reinterpret_cast<std::uint32_t>(text);
    if (caller == a::info_add_return && text_address >= 0x30)
      info = ubc::read_selection_info(memory, text_address - 0x30);
    if (caller == a::choice_add_return)
      choice = ubc::read_choice(memory, s::information_manager, text_address);
  }
  SetLastError(error);
  original_add(view, text, number);
  const auto after = GetLastError();
  if (own) {
    const auto count = integer(address, a::view_count);
    if (caller == a::info_add_return)
      rows.append(address, info, before, count, collection_depth == 1 && number == 0);
    else
      rows.valid = false;
    if (choice_collection) {
      if (caller != a::choice_add_return || collection_depth != 1 || number != 0 || !choice.valid ||
          before < 0 || static_cast<std::size_t>(before) != choice_count || count != before + 1 ||
          choice.index != before || choice_count == choice_functions.size())
        choice_collection = false;
      else
        choice_functions[choice_count++] = choice.function;
    }
    --collection_depth;
  }
  SetLastError(after);
}

bool late_gate() noexcept {
  if (!pending || handoff_cancelled || recovering || !native_update || effect_calls ||
      !own_thread() || update_depth != 1 || station_depth || collection_depth || !ui.focused())
    return false;
  const auto c = context();
  const auto& k = handed_snapshot.key;
  if (!c.valid || c.world != k.world || c.hero != k.hero || c.station != k.station ||
      c.dialog.view != k.view || c.dialog.mode != (k.mode == ubc::SelectionMode::info ? 1 : 2) ||
      rows.generation != k.collection)
    return false;
  if (!profile_matches())
    return false;
  const auto material = stock(c);
  return material.known() && material.stored == handed_snapshot.stored &&
         material.reserved == handed_snapshot.reserved &&
         ubc::material_preview(c.recipe, material, handed_quantity).valid &&
         same_context(c, context());
}
#ifdef UBC_FREEAMOUNTS
namespace rt = ubc::runtime_script;
namespace bc = ubc::batch_call;
struct NativeAmountContext {
  rt::detail::Layout layout{};
  const ubc::lhiver::Function* function = nullptr;
};
bool __cdecl batch_signature(const bc::Call& call) noexcept {
  const auto* entry = ubc::amount_profile::find(static_cast<std::uint32_t>(call.symbol));
  if (!entry || reinterpret_cast<std::uintptr_t>(call.parser) != s::game_parser)
    return false;
  const auto& function = ubc::lhiver::functions[entry->function_index];
  rt::detail::Layout layout{};
  rt::detail::Symbol symbol{};
  return rt::detail::read_layout(memory, s::game_parser, p::profile, layout) == rt::Issue::none &&
         rt::detail::lookup(memory, layout, function.symbol, symbol) == rt::Issue::none &&
         rt::detail::name_matches(memory, symbol, function.name) == rt::Issue::none &&
         symbol.type == 5 && symbol.flags == 1 && symbol.elements == 0 && symbol.offset == 0 &&
         rt::detail::parent_matches(memory, layout, symbol, -1) == rt::Issue::none &&
         rt::detail::stable_layout(memory, s::game_parser, p::profile, layout) == rt::Issue::none;
}
bc::Decision __cdecl select_batch_call(const bc::Call& call) noexcept {
  // Other threads never read transient order fields. The bridge also
  // protects these calls atomically as readers and against an active writer.
  if (!own_thread())
    return bc::Decision::ordinary;
  if (!pending || call.symbol != handed_offer.function)
    return bc::Decision::ordinary;
  const auto expected = handed_snapshot.key.mode == ubc::SelectionMode::info
                            ? bc::info_call_return
                            : bc::choice_call_return;
  if (!effect_authorized || executed || effect_calls || handoff_cancelled ||
      call.caller != expected)
    return bc::Decision::reject;
  return bc::Decision::writer;
}
bool amount_identity(void* raw, std::uint32_t address, std::size_t length) noexcept {
  const auto& ctx = *static_cast<NativeAmountContext*>(raw);
  if (!own_thread() || !ctx.function || length != ctx.function->byte_length ||
      address != ctx.layout.code + ctx.function->byte_offset)
    return false;
  rt::detail::Layout fresh{};
  rt::detail::Symbol symbol{};
  return rt::detail::read_layout(memory, s::game_parser, p::profile, fresh) == rt::Issue::none &&
         rt::detail::same_layout(ctx.layout, fresh) &&
         rt::detail::lookup(memory, fresh, ctx.function->symbol, symbol) == rt::Issue::none &&
         symbol.data == ctx.function->byte_offset && symbol.type == 5 && symbol.flags == 1 &&
         symbol.elements == 0 && symbol.offset == 0 &&
         rt::detail::name_matches(memory, symbol, ctx.function->name) == rt::Issue::none;
}
bool amount_writable(void*, std::uint32_t address, std::size_t size) noexcept {
  if (!address || !size || size > UINT32_MAX - address)
    return false;
  std::uintptr_t cursor = address;
  const auto end = cursor + size;
  while (cursor < end) {
    MEMORY_BASIC_INFORMATION page{};
    if (VirtualQuery(reinterpret_cast<void*>(cursor), &page, sizeof(page)) != sizeof(page) ||
        page.State != MEM_COMMIT || page.Type != MEM_PRIVATE || (page.Protect & PAGE_GUARD) ||
        (page.Protect & 0xffu) != PAGE_READWRITE)
      return false;
    const auto base = reinterpret_cast<std::uintptr_t>(page.BaseAddress);
    if (base > cursor || page.RegionSize > UINT32_MAX - base || base + page.RegionSize <= cursor)
      return false;
    cursor = base + page.RegionSize;
  }
  return true;
}
bool amount_replace(void* raw, std::uint32_t address, const void* before, const void* after,
                    std::size_t size) noexcept {
  auto* ctx = static_cast<NativeAmountContext*>(raw);
  if (!ctx->function ||
      !amount_identity(raw, ctx->layout.code + ctx->function->byte_offset,
                       ctx->function->byte_length) ||
      !amount_writable(raw, address, size))
    return false;
  __try {
    auto* target = reinterpret_cast<void*>(address);
    if (std::memcmp(target, before, size))
      return false;
    std::memcpy(target, after, size);
    return std::memcmp(target, after, size) == 0;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}
void flush_amount_diagnostic() noexcept {
  if (!amount_diagnostic.pending)
    return;
  const auto saved = GetLastError();
  const auto report = amount_diagnostic;
  amount_diagnostic = {};
  if (report.entered)
    event("native_effect_enter", report.symbol, report.quantity);
  if (report.rejected)
    event("amount_rejected", report.rejected);
  event(report.restored ? "amount_restored" : "amount_restore_failed", report.symbol,
        report.quantity);
  event("amount_duration_us", report.symbol, report.duration);
  SetLastError(saved);
}
void* __cdecl execute_original_amount(void* raw) {
  auto& scope = *static_cast<bc::WriterScope*>(raw);
  if (!batch_result.begin(handed_context.recipe, expected_result, handed_context.world,
                          handed_context.hero, handed_context.station)) {
    handoff_cancelled = true;
    return nullptr;
  }
  batch_end_witness = false;
  ++effect_calls;
  executed = true;
  return scope.original(scope.call.parser, scope.call.symbol);
}
void* __cdecl execute_amount(bc::WriterScope& scope) {
  // The gate remains exclusive. Write diagnostics only after returning
  // from the native info/choice wrapper and releasing the gate.
  const auto incoming_error = GetLastError();
  scope.restore_verified = true;
  amount_diagnostic = {
      true, false, true, 0, 0, scope.call.symbol, static_cast<int>(handed_quantity)};
  const auto* entry = ubc::amount_profile::find(static_cast<std::uint32_t>(scope.call.symbol));
  if (!entry || !batch_result.permits_next() || !late_gate()) {
    handoff_cancelled = true;
    amount_diagnostic.rejected = 1;
    SetLastError(incoming_error);
    return nullptr;
  }
  NativeAmountContext ctx;
  ctx.function = &ubc::lhiver::functions[entry->function_index];
  if (rt::detail::read_layout(memory, s::game_parser, p::profile, ctx.layout) != rt::Issue::none ||
      ctx.function->byte_offset > ctx.layout.code_length ||
      ctx.function->byte_length > ctx.layout.code_length - ctx.function->byte_offset) {
    handoff_cancelled = true;
    amount_diagnostic.rejected = 2;
    SetLastError(incoming_error);
    return nullptr;
  }
  ubc::SelectionStock before_stock;
  before_stock.issue = ubc::StockIssue::none;
  before_stock.stored = handed_snapshot.stored;
  before_stock.reserved = handed_snapshot.reserved;
  before_stock.maximum = handed_quantity;
  expected_result = ubc::material_preview(handed_context.recipe, before_stock, handed_quantity);
  if (!expected_result.valid) {
    handoff_cancelled = true;
    amount_diagnostic.rejected = 3;
    SetLastError(incoming_error);
    return nullptr;
  }
  LARGE_INTEGER before_time{}, after_time{}, frequency{};
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&before_time);
  result_started = GetTickCount64();
  const ubc::AmountMemory writable{memory, &ctx, amount_identity, amount_writable, amount_replace};
  ubc::AmountOutcome outcome;
  SetLastError(incoming_error);
  void* result = ubc::run_amount_call(
      amount_patch, writable, ctx.layout.code + ctx.function->byte_offset, entry->body,
      handed_quantity, execute_original_amount, &scope, scope.restore_verified, outcome);
  const auto saved = GetLastError();
  amount_diagnostic.entered = executed;
  amount_diagnostic.restored = scope.restore_verified;
  if (!outcome.entered) {
    handoff_cancelled = true;
    amount_diagnostic.rejected = 10 + static_cast<int>(outcome.issue);
  }
  if (!scope.restore_verified) {
    handoff_cancelled = true;
    batch_result.fail(ubc::BatchResultIssue::restore);
  }
  QueryPerformanceCounter(&after_time);
  if (frequency.QuadPart > 0 && after_time.QuadPart >= before_time.QuadPart) {
    const auto elapsed = static_cast<double>(after_time.QuadPart - before_time.QuadPart) *
                         1000000.0 / static_cast<double>(frequency.QuadPart);
    amount_diagnostic.duration = elapsed >= INT32_MAX ? INT32_MAX : static_cast<int>(elapsed);
  }
  SetLastError(saved);
  return result;
}
#endif
void __fastcall observe_update(void* manager, void* unused) {
  const auto error = GetLastError();
  const bool own = own_thread();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool previous_native = own ? native_update : false;
  if (own) {
    ++update_depth;
    native_update = update_depth == 1 &&
                    reinterpret_cast<std::uint32_t>(manager) == s::information_manager &&
                    (caller == 0x74c117 || caller == 0x75a727 || caller == 0x6c8a46);
    if (pending && !ui.focused())
      handoff_cancelled = true;
  }
  SetLastError(error);
  original_update(manager, unused);
  const auto after = GetLastError();
  if (own) {
    if (update_depth == 1 && pending && recover) {
      recover = false;
      recovering = true;
      original_station = handed_snapshot.key.station;
      const auto current = context();
      if (native_update && same_context(current, handed_context) &&
          current.dialog.rejected_info_transition && current.dialog.selected == handed_offer.row &&
          rows.generation == handed_snapshot.key.collection &&
          reinterpret_cast<std::uint32_t>(manager) == s::information_manager) {
        reinterpret_cast<a::FastObject>(a::info_begin)(manager, nullptr);
        event("selection_reopened");
        pending = false;
      } else {
        // Unreadable rows or rows with a different layout do not establish
        // an abort. Continue guarding against effects of the consumed order.
        handoff_cancelled = true;
        event("selection_quarantined");
      }
      recovering = false;
      recover = false;
    } else if (update_depth == 1 && pending) {
      const auto d = ubc::read_selection_dialog(memory, s::information_manager);
      const auto current = context();
      const bool reopened = native_update && same_owner(current, handed_context) && d.valid &&
                            integer(s::information_manager, 0x44) == 1;
      const bool ended = native_update && same_owner(current, handed_context) && d.valid &&
                         integer(s::information_manager, 0x30) == 1;
      if (executed || reopened || ended) {
        observe_stock_after = executed;
        pending = false;
        event(executed ? "native_effect_returned" : "native_no_effect",
              static_cast<int>(effect_calls));
      } else if (!d.valid)
        handoff_cancelled = true;
    }
    --update_depth;
    native_update = previous_native;
  }
  SetLastError(after);
}
void* __fastcall observe_resolve(void* manager, void*, void* player, void* npc, int row) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  SetLastError(error);
  void* result = original_resolve(manager, player, npc, row);
  const auto after = GetLastError();
  if (own_thread() && pending && caller == a::resolve_selection_return) {
    const auto info = ubc::read_selection_info(memory, reinterpret_cast<std::uint32_t>(result));
    const bool detached = positively_detached();
    if (detached)
      handoff_cancelled = true;
    const bool bound = info.valid && info.instance == handed_offer.info_instance &&
                       info.function == handed_offer.function;
    // A confirmed new context may resolve its own dialog. Keep the
    // old bound recipe callback blocked until a quiescent frame.
    if (detached && ((info.valid && !bound) || !result)) {
      SetLastError(after);
      return result;
    }
    const auto& k = handed_snapshot.key;
    bool linked = false;
    for (std::size_t j = 0; j < 4; ++j)
      if (p::info_links[j].function == handed_offer.function)
        linked = ubc::selection_info_link(info, p::info_links[j]) && !info.first_choice;
    if (k.mode != ubc::SelectionMode::info || update_depth != 1 ||
        reinterpret_cast<std::uint32_t>(player) != k.hero ||
        reinterpret_cast<std::uint32_t>(npc) != k.hero || row != handed_offer.row || !linked ||
        info.instance != handed_offer.info_instance || info.function != handed_offer.function ||
        !late_gate()) {
      result = nullptr;
      recover = true;
      handoff_cancelled = true;
      event("resolution_rejected");
    }
  }
  SetLastError(after);
  return result;
}
void __fastcall observe_info(void* info, void*) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool own = own_thread();
  if (own)
    ++info_depth;
  bool allow = true;
  const auto actual = own && pending
                          ? ubc::read_selection_info(memory, reinterpret_cast<std::uint32_t>(info))
                          : ubc::SelectionInfo{};
  const bool bound_target = actual.valid && actual.instance == handed_offer.info_instance &&
                            actual.function == handed_offer.function;
  const bool detached = own && pending && positively_detached();
  if (detached)
    handoff_cancelled = true;
  const bool foreign = detached && actual.valid && !bound_target;
  if (own && pending && !foreign && (caller == a::info_effect_return || bound_target)) {
    bool linked = false;
    for (std::size_t j = 0; j < 4; ++j)
      if (p::info_links[j].function == handed_offer.function)
        linked = ubc::selection_info_link(actual, p::info_links[j]);
    allow = handed_snapshot.key.mode == ubc::SelectionMode::info &&
            caller == a::info_effect_return && info_depth == 1 && !choice_depth && actual.valid &&
            actual.instance == handed_offer.info_instance &&
            actual.function == handed_offer.function && linked && !actual.first_choice &&
            late_gate();
    if (allow) {
#ifndef UBC_FREEAMOUNTS
      ++effect_calls;
      executed = true;
      event("native_effect_enter", handed_offer.function, static_cast<int>(handed_offer.quantity));
#endif
    } else {
      handoff_cancelled = true;
      event("effect_rejected");
    }
  }
#ifdef UBC_FREEAMOUNTS
  const bool prior_authorized = own ? effect_authorized : false;
  if (own && pending)
    effect_authorized = allow && bound_target && info_depth == 1 && !choice_depth;
#endif
  SetLastError(error);
  if (allow)
    original_info(info);
  const auto after = GetLastError();
#ifdef UBC_FREEAMOUNTS
  if (own)
    flush_amount_diagnostic();
  if (own && pending && bound_target && !executed) {
    handoff_cancelled = true;
    recover = true;
    event("amount_call_rejected");
  }
  if (own)
    effect_authorized = prior_authorized;
#endif
  if (own)
    --info_depth;
  SetLastError(after);
}
void __fastcall observe_choice(void* manager, void* choice) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool own = own_thread();
  if (own)
    ++choice_depth;
  bool allow = true;
  const auto actual = own && pending
                          ? ubc::read_choice(memory, reinterpret_cast<std::uint32_t>(manager),
                                             reinterpret_cast<std::uint32_t>(choice))
                          : ubc::ChoiceSnapshot{};
  const bool bound_target = actual.valid && actual.info.instance == handed_offer.info_instance &&
                            actual.function == handed_offer.function &&
                            reinterpret_cast<std::uint32_t>(manager) == handed_snapshot.key.manager;
  const bool detached = own && pending && positively_detached();
  if (detached)
    handoff_cancelled = true;
  const bool foreign = detached && actual.valid && !bound_target;
  if (own && pending && !foreign && (caller == a::choice_effect_return || bound_target)) {
    std::uint32_t owner_info = 0;
    const bool linked =
        field(reinterpret_cast<std::uint32_t>(manager), 0x28, owner_info) &&
        ubc::selection_info_link(ubc::read_selection_info(memory, owner_info), p::info_links[4]);
    allow = handed_snapshot.key.mode == ubc::SelectionMode::choice &&
            caller == a::choice_effect_return && choice_depth == 1 && !info_depth && actual.valid &&
            actual.info.instance == handed_offer.info_instance && linked &&
            actual.function == handed_offer.function && actual.index == handed_offer.row &&
            late_gate();
    if (allow) {
#ifndef UBC_FREEAMOUNTS
      ++effect_calls;
      executed = true;
      event("native_effect_enter", handed_offer.function, static_cast<int>(handed_offer.quantity));
#endif
    } else {
      handoff_cancelled = true;
      event("effect_rejected");
    }
  }
#ifdef UBC_FREEAMOUNTS
  const bool prior_authorized = own ? effect_authorized : false;
  if (own && pending)
    effect_authorized = allow && bound_target && choice_depth == 1 && !info_depth;
#endif
  SetLastError(error);
  if (allow)
    original_choice(manager, choice);
  const auto after = GetLastError();
#ifdef UBC_FREEAMOUNTS
  if (own)
    flush_amount_diagnostic();
  if (own && pending && bound_target && !executed) {
    handoff_cancelled = true;
    event("amount_call_rejected");
  }
  if (own)
    effect_authorized = prior_authorized;
#endif
  if (own)
    --choice_depth;
  SetLastError(after);
}
void __fastcall observe_start(void* mob, void*, void* npc) {
  const auto error = GetLastError();
  const bool own = own_thread();
  if (own)
    ++station_depth;
  SetLastError(error);
  original_start(mob, npc);
  const auto after = GetLastError();
  if (own)
    --station_depth;
  SetLastError(after);
}
void __fastcall observe_end(void* mob, void*, void* npc, int flag) {
  const auto error = GetLastError();
  const bool own = own_thread();
  if (own)
    ++station_depth;
#ifdef UBC_FREEAMOUNTS
  const bool own_result = own && batch_result.state() == ubc::BatchResultState::waiting &&
                          reinterpret_cast<std::uint32_t>(mob) == handed_context.station &&
                          reinterpret_cast<std::uint32_t>(npc) == handed_context.hero;
#endif
  SetLastError(error);
  original_end(mob, npc, flag);
  const auto after = GetLastError();
#ifdef UBC_FREEAMOUNTS
  if (own_result) {
    const auto fresh = context();
    std::uint32_t active_mob = UINT32_MAX;
    if (fresh.world == handed_context.world && fresh.hero == handed_context.hero &&
        field(fresh.hero, s::npc_interact_mob, active_mob) && !active_mob)
      batch_end_witness = true;
  }
#endif
  if (own)
    --station_depth;
  SetLastError(after);
}
int __fastcall observe_menu_event(void* menu, void*, int key) {
  if (own_thread() && ui.opened() && menu == ui.object())
    return ui.handle(session, key);
  return original_menu_event(menu, key);
}

void install() noexcept {
  std::array<std::uint8_t, a::frame_prefix_bytes.size()> prefix{};
  if (!memory.copy(a::frame_prefix, prefix.data(), prefix.size()) ||
      prefix != a::frame_prefix_bytes)
    return;
  wchar_t path[32768]{};
  if (!w::sibling_path(module, path,
#ifdef UBC_FREEAMOUNTS
                       L"UnionBatchCrafting.log"
#else
                       L"UnionBatchCraftingSelectionProbe.log"
#endif
                       ))
    return;
  log_file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (log_file == INVALID_HANDLE_VALUE)
    return;
  const char* header = execution_enabled ?
#ifdef UBC_FREEAMOUNTS
                                         "batch_version=1\r\nmode=free_amounts\r\n"
                                         : "batch_version=1\r\nmode=ui_only\r\n";
#else
                                         "selection_probe_version=1\r\nmode=original_selection\r\n"
                                         : "selection_probe_version=1\r\nmode=ui_only\r\n";
#endif
  DWORD written = 0;
  WriteFile(log_file, header, static_cast<DWORD>(std::strlen(header)), &written, nullptr);
  log_bytes = written;
  const std::array<std::uintptr_t, sites.size()> destinations{
      {reinterpret_cast<std::uintptr_t>(&UbcFrameBridge),
       reinterpret_cast<std::uintptr_t>(&observe_add),
       reinterpret_cast<std::uintptr_t>(&observe_clear),
       reinterpret_cast<std::uintptr_t>(&observe_update),
       reinterpret_cast<std::uintptr_t>(&observe_resolve),
       reinterpret_cast<std::uintptr_t>(&observe_info),
       reinterpret_cast<std::uintptr_t>(&observe_choice),
       reinterpret_cast<std::uintptr_t>(&observe_start),
       reinterpret_cast<std::uintptr_t>(&observe_end),
       reinterpret_cast<std::uintptr_t>(&observe_menu_event)
#ifdef UBC_FREEAMOUNTS
           ,
       reinterpret_cast<std::uintptr_t>(&UbcBatchCallBridge)
#endif
      }};
  if (!hooks.prepare(destinations)) {
    event("hooks_rejected");
    return;
  }
  UbcFrameContinuation = hooks.original(0);
  UbcAfterFrame = after_frame;
  original_add = reinterpret_cast<a::AddChoice>(hooks.original(1));
  original_clear = reinterpret_cast<a::FastObject>(hooks.original(2));
  original_update = reinterpret_cast<a::FastObject>(hooks.original(3));
  original_resolve = reinterpret_cast<a::ResolveInfo>(hooks.original(4));
  original_info = reinterpret_cast<Info>(hooks.original(5));
  original_choice = reinterpret_cast<a::FastObject>(hooks.original(6));
  original_start = reinterpret_cast<Start>(hooks.original(7));
  original_end = reinterpret_cast<End>(hooks.original(8));
  original_menu_event = reinterpret_cast<m::MenuHandleEvent>(hooks.original(9));
#ifdef UBC_FREEAMOUNTS
  UbcBatchCallOriginal = reinterpret_cast<void*>(hooks.original(10));
  bc::Config config;
  config.parser = reinterpret_cast<void*>(s::game_parser);
  config.signature = batch_signature;
  config.select = select_batch_call;
  config.writer = execute_amount;
  for (const auto& entry : ubc::amount_profile::entries)
    config.symbols[config.symbol_count++] =
        static_cast<std::int32_t>(ubc::lhiver::functions[entry.function_index].symbol);
  if (!bc::configure(config)) {
    event("hooks_rejected");
    return;
  }
#endif
  if (hooks.install(reinterpret_cast<std::uintptr_t>(&UbcFrameBridge))) {
    InterlockedExchange(&installed, 1);
    event("hooks_installed");
  } else
    event("hooks_rejected");
}
}
extern "C" unsigned __cdecl UbcSelectionProbeVersion() {
  return
#ifdef UBC_FREEAMOUNTS
      2;
#else
      1;
#endif
}
extern "C" int __cdecl UbcSelectionProbeHooksInstalled() {
  return static_cast<int>(InterlockedCompareExchange(&installed, 0, 0));
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    module = instance;
    const auto error = GetLastError();
    if (w::target_matches()) {
#ifdef UBC_FREEAMOUNTS
      const bool preview =
          w::consume_marker(instance, L"UnionBatchCrafting.enable-once", "BATCH_UI_V1\n");
      execution_enabled = !preview && w::consume_marker(instance, L"UnionBatchCrafting.enable-once",
                                                        "FREE_AMOUNTS_V1\n");
#else
      const bool preview = w::consume_marker(
          instance, L"UnionBatchCraftingSelectionProbe.enable-once", "SELECTION_UI_V1\n");
      execution_enabled =
          !preview && w::consume_marker(instance, L"UnionBatchCraftingSelectionProbe.enable-once",
                                        "ORIGINAL_SELECTION_V1\n");
#endif
      if (preview || execution_enabled)
        install();
    }
    SetLastError(error);
  } else if (reason == DLL_PROCESS_DETACH && log_file != INVALID_HANDLE_VALUE) {
    CloseHandle(log_file);
    log_file = INVALID_HANDLE_VALUE;
  }
  return TRUE;
}
