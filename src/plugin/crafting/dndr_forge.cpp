// SPDX-License-Identifier: MIT
#include "runtime.hpp"
#include "forge_profile.hpp"
#include "ubc/engine/animation_observation.hpp"
#include "ubc/stations/forge/forge_stock.hpp"
#include "ubc/ui/station_quantity.hpp"

namespace ubc_native {
namespace {
namespace f = ubc::dndr::forge;
namespace obs = ubc::animation_observation;
using Address = std::uint32_t;
using WithNpc = int(__thiscall*)(void*, void*);
using Message = void(__thiscall*)(void*, void*, void*);
using Interact = void(__thiscall*)(void*, void*, int, int, int, int, int);
using Enter = int(__thiscall*)(void*, void*, int);
using Notify = void(__thiscall*)(void*, void*);
constexpr Address enter = 0x721f00;
constexpr Address input_state = 0x4d4d30, input_toggled = 0x4d5020;
constexpr unsigned short use_key = 5;
constexpr std::array<s::HookSite, 7> forge_sites{
    {{0x6990b2, 6, 14, {{0xff, 0x90, 0xe4, 0, 0, 0, 0x85, 0xc0, 0x0f, 0x85, 0x31, 4, 0, 0}}},
     {0x74b020, 7, 14, {{0x6a, 0xff, 0x68, 0xda, 0x57, 0x82, 0, 0x64, 0xa1, 0, 0, 0, 0, 0x50}}},
     {0x720ce0, 5, 12, {{0x83, 0xec, 0x18, 0x53, 0x56, 0x8b, 0x74, 0x24, 0x24, 0x57, 0x8b, 0xf9}}},
     {0x71f210, 8, 12, {{0x53, 0x55, 0x56, 0x57, 0x8b, 0x7c, 0x24, 0x14, 0x8b, 0xf1, 0x8b, 0x06}}},
     {0x6993be, 6, 14, {{0xff, 0x90, 0xe4, 0, 0, 0, 0x85, 0xc0, 0x75, 0x78, 0x8b, 0x85, 0x2c, 1}}},
     {input_state,
      5,
      13,
      {{0xa1, 0x64, 0x1d, 0x8d, 0, 0x85, 0xc0, 0x53, 0x55, 0x56, 0x57, 0x8b, 0xd9}}},
     {input_toggled,
      5,
      13,
      {{0xa1, 0x64, 0x1d, 0x8d, 0, 0x85, 0xc0, 0x53, 0x55, 0x56, 0x57, 0x8b, 0xf9}}}}};
w::Hooks<s::HookSite, forge_sites.size()> forge_hooks(forge_sites);
ubc::NativeCraftingUi window;
ubc::StationQuantity chooser;
ubc::SelectionInput release;
f::Order order;
const f::StationProfile* station = &f::smithy;
f::ReturnFocus return_focus;
f::Stock opening_stock;
ubc::PackedInventoryWorkspace inventory;
ubc::station_recipe_snapshot_detail::HandWorkspace hands;
Address hero{}, mob{}, world{}, callback{};
int health{};
bool installed_forge{}, pending{}, locked{}, forward_sent{}, closing{}, passthrough{};
ULONGLONG deadline{}, last_tick{};
std::uint64_t ended_frame{};
unsigned message_depth{};

template <typename T> Address addr(T* p) noexcept { return reinterpret_cast<Address>(p); }
bool active() noexcept { return pending || window.opened() || order.active(); }
bool belongs(void* object, void* npc) noexcept {
  return own_thread() && mob == addr(object) && hero == addr(npc) && hero && order.active();
}
void station_event(const char* suffix, int detail = 0, int quantity = 0,
                   const char* fields = "") noexcept {
  char name[64]{};
  std::snprintf(name, sizeof(name), "%s_%s", station->kind == f::Station::pan ? "pan" : "forge",
                suffix);
  event(name, detail, quantity, fields);
}
void stop(const char* reason) noexcept {
  if (active() && !closing)
    station_event(reason, static_cast<int>(order.completed()), static_cast<int>(order.selected()));
  order.stop();
  pending = false;
  closing = true;
  locked = true;
  release.disarm();
}
f::Stock sample() noexcept {
  namespace h = ubc::station_recipe_snapshot_detail;
  const auto stock = ubc::read_packed_inventory(memory, hero, station->items, true, inventory);
  if (h::begin_hands(memory, hero, hands) != ubc::StationHandReadIssue::none ||
      !h::verify_hands(memory, hands))
    return {};
  const auto& v = hands.before;
  const auto anchor = ubc::hand_detail::read_object(memory, v.interact_item);
  return f::stock_from(stock, inventory, f::hand_state(v, anchor, *station), *station);
}
bool world_ok() noexcept {
  Address game{}, current{}, actor{}, hw{}, mw{};
  int menus = -1, done = -1;
  return read(a::game_pointer, game) && integer(game, 0) == static_cast<int>(a::game_vtable) &&
         field(game, a::game_world, current) && current == world && world &&
         read(s::hero_pointer, actor) && actor == hero && field(hero, 0xb8, hw) && hw == world &&
         field(mob, 0xb8, mw) && mw == world && integer(hero, 0x1b8) == health && health > 0 &&
         read(0x8d1e40, menus) && menus == 0 && field(s::information_manager, 0x2c, done) &&
         done == 1 && window.focused();
}
bool ready_place(bool require_focus = true) noexcept {
  Address node{}, pos{}, next{}, occupant{}, focus{}, linked{};
  std::array<float, 16> actor{}, place{};
  if ((require_focus && (!field(hero, 0x9ac, focus) || focus != mob)) ||
      !field(hero, 0x964, linked) || linked || !field(mob, 0x18c, node) || !field(node, 0, pos) ||
      !field(node, 4, next) || next || !field(pos, 0x44, occupant) || occupant ||
      !field(hero, 0x3c, actor) || !field(pos, 0, place))
    return false;
  float sum = 0;
  for (unsigned i : {3u, 7u, 11u}) {
    const float d = place[i] - actor[i];
    sum += d * d;
  }
  return sum <= 150.0f * 150.0f && integer(mob, 0x1f8) == 1 && integer(mob, 0x200) == 0 &&
         integer(mob, 0x1f4) == 0 && integer(mob, 0x1fc) == 0 && name_is(mob, 0x1cc, "") &&
         name_is(mob, 0x1e0, "");
}
const f::StationProfile* station_profile(Address target) noexcept {
  char name[48]{};
  for (const auto* profile : {&f::smithy, &f::pan})
    if (name_is(target, 0x1b8, profile->scheme) && obs::string(memory, target, 0x1a4, name) &&
        _stricmp(name, profile->items[0].name) == 0)
      return profile;
  return nullptr;
}
// The forge fix relocates the original RemoveFromSlot CALL into a small Union stub.
Address branch(Address where, std::uint8_t op) noexcept {
  std::array<std::uint8_t, 5> bytes{};
  if (!memory.copy(where, bytes.data(), bytes.size()) || bytes[0] != op)
    return 0;
  return where + 5 + ubc::packed_detail::little_u32(bytes.data() + 1);
}
const char* profile_issue() noexcept {
  Address table{}, method{};
  Address device{};
  if (!read(m::input_pointer, device) || !read(device, table) || !field(table, 4, method) ||
      method != input_state || !field(table, 12, method) || method != input_toggled)
    return "forge_input_device_rejected";
  if (!read(mob, table) || !field(table, 0xec, method) || method != enter ||
      !field(table, 0xd0, method) || method != forge_sites[3].address ||
      !field(table, 0xe0, method) || method != 0x720f40)
    return "forge_vtable_rejected";
  if (const auto* bad = ubc::crafting_ui::verify_code(memory, forge_profile::routes))
    return bad;
  if (!end_path(mob).valid)
    return "forge_end_path_rejected";
  const auto* b = end_compatibility();
  // The direct inventory route is valid with the pinned Union build and no ZBugFixes.
  // A loaded, unrecognised DLL must never fall through to this profile.
  if (!end_union_module.current() || (!b && GetModuleHandleW(L"ZBUGFIXES.DLL")))
    return "forge_modules_rejected";
  const auto put = b ? b->zbug.base + ubc::zbug_end_compat::wrapper_rva
                     : ubc::zbug_end_compat::original_put_in_inv;
  for (Address where : {0x721e53u, 0x754805u, 0x74adb6u})
    if (branch(where, 0xe8) != put)
      return "forge_storage_path_rejected";
  std::uint64_t nops{};
  const auto stub = branch(0x75479b, 0xe9);
  std::array<std::uint8_t, 7> reset{};
  constexpr std::array<std::uint8_t, 7> expected{{0xc7, 0x43, 0x18, 0, 0, 0, 0}};
  if (!read(0x720d85, nops) || nops != UINT64_C(0x9090909090909090) || !stub ||
      !read(stub, reset) || reset != expected || branch(stub + 7, 0xe8) != 0x74a340 ||
      branch(stub + 12, 0xe9) != 0x7547a0)
    return "forge_item_patch_rejected";
  return nullptr;
}
bool held(bool directions = false) noexcept {
  if (window.buttons_held())
    return true;
  Address input{}, table{}, method{};
  if (!read(m::input_pointer, input) || !read(input, table) || !field(table, 4, method) ||
      !w::readable(reinterpret_cast<void*>(method), 1, true))
    return true;
  using KeyState = float(__thiscall*)(void*, unsigned short);
  auto state = reinterpret_cast<KeyState>(method);
  auto* device = reinterpret_cast<void*>(input);
  if (state(device, use_key) != 0)
    return true;
  if (directions)
    for (unsigned short key : {m::game_left, m::game_right, m::game_up, m::game_down,
                               m::game_strafe_left, m::game_strafe_right})
      if (state(device, key) != 0)
        return true;
  return false;
}
// Gameplay also polls movement independently of zCMenu::HandleEvent. Keep raw
// key events and the plugin's own release checks available to the menu.
bool menu_direction(Address caller, unsigned short key) noexcept {
  // oCAIHuman's player-control methods, from Pressed through DoAI.
  if (caller < 0x695540 || caller > 0x69c38e || !own_thread() || (!pending && !window.opened()))
    return false;
  return key == use_key || key == m::game_left || key == m::game_right || key == m::game_up ||
         key == m::game_down || key == m::game_strafe_left || key == m::game_strafe_right;
}
float __fastcall world_key_state(void* device, void*, unsigned short key) {
  if (menu_direction(addr(_ReturnAddress()), key))
    return 0.0f;
  using GetState = float(__thiscall*)(void*, unsigned short);
  return reinterpret_cast<GetState>(forge_hooks.original(5))(device, key);
}
int __fastcall world_key_toggled(void* device, void*, unsigned short key) {
  if (menu_direction(addr(_ReturnAddress()), key))
    return 0;
  using GetToggled = int(__thiscall*)(void*, unsigned short);
  return reinterpret_cast<GetToggled>(forge_hooks.original(6))(device, key);
}
bool witness(f::Witness what) noexcept {
  const auto stock = sample();
  obs::Animation animation;
  if (station->kind == f::Station::pan &&
      (what == f::Witness::insert || what == f::Witness::exchange || what == f::Witness::remove)) {
    Address model{};
    if (field(hero, 0xc8, model))
      animation = obs::current(memory, model, station->scheme);
  }
  const bool ok = order.observe(what, stock, frame, animation.name);
  char fields[112]{};
  const char* format = station->kind == f::Station::pan ? ",\"raw\":%d,\"cooked\":%d,\"hand\":%d"
                                                        : ",\"cold\":%d,\"hot\":%d,\"hand\":%d";
  std::snprintf(fields, sizeof(fields), format, stock.input_count, stock.output_count,
                static_cast<int>(stock.hand));
  station_event(ok ? "witness" : "balance_rejected", static_cast<int>(what),
                static_cast<int>(order.completed()), fields);
  if (!ok)
    stop("order_stopped");
  return ok;
}
int __fastcall occupancy(void* object, void*, void* npc) {
  // The displaced virtual call uses this object's own vtable, including overrides.
  const auto table = *static_cast<Address*>(object);
  const int actual =
      reinterpret_cast<WithNpc>(*reinterpret_cast<Address*>(table + 0xe4))(object, npc);
  const auto error = GetLastError();
  // Only these two keyboard/gamepad action branches go on to reserve the item.
  // AI, reachability checks, NPC decisions and the original entry keep their answer.
  const auto* profile = !actual && own_thread() && addr(npc) == observed_hero
                            ? station_profile(addr(object))
                            : nullptr;
  if (!profile) {
    SetLastError(error);
    return actual;
  }
  bool consume = locked || active();
  if (!consume && passthrough) {
    passthrough = false;
    if (mob == addr(object) && hero == addr(npc)) {
      SetLastError(error);
      return actual;
    }
  }
  if (!consume && !ui.opened() && !handoff.active() && !disabled) {
    station = profile;
    mob = addr(object);
    hero = addr(npc);
    world = observed_world;
    health = integer(hero, 0x1b8);
    callback = ubc::NativeSelectionUi::top_callback();
    if (callback && world_ok()) {
      const auto* issue = profile_issue();
      if (issue) {
        event(issue);
      } else {
        // The original initializes an untouched station's position list before testing its range.
        // ScanIdealPositions only builds the model positions. CanInteractWith would reserve material.
        Address node{};
        if (field(mob, 0x18c, node) && !node) {
          using ScanPositions = void(__thiscall*)(void*);
          reinterpret_cast<ScanPositions>(0x71dc30)(object);
        }
        if (!ready_place()) {
          station_event("place_unavailable");
          SetLastError(error);
          return actual;
        }
        opening_stock = sample();
        const auto max = f::maximum(opening_stock);
        if (max) {
          pending = locked = consume = true;
          closing = false;
          chooser.open(max);
          last_tick = GetTickCount64();
          station_event("use_captured", opening_stock.input_count, static_cast<int>(max));
        } else
          station_event("stock_unavailable", opening_stock.input_count,
                        static_cast<int>(opening_stock.hand));
      }
    } else {
      station_event("use_unavailable", world_ok() ? 1 : 0, ready_place() ? 1 : 0);
    }
  }
  SetLastError(error);
  return consume ? 1 : actual;
}
// Each site replaces one six-byte indirect CALL. ECX and the single stacked NPC
// already carry the original thiscall arguments; occupancy returns with RET 4.
__declspec(naked) void primary_input() {
  __asm { call occupancy }
  __asm {push 0x6990b8} __asm { ret }
}
__declspec(naked) void secondary_input() {
  __asm { call occupancy }
  __asm {push 0x6993c4} __asm { ret }
}
void __fastcall item_message(void* npc, void*, void* msg, void* sender) {
  const auto saved = GetLastError();
  const bool watch = own_thread() && hero == addr(npc) && order.active();
  if (own_thread() && active() && hero == addr(npc) && integer(addr(msg), 0) == 0x83dafc)
    stop("hit"); // oCMsgDamage, including hits absorbed by armour.
  const auto* tag = watch ? obs::message_tag(memory, addr(msg)) : nullptr;
  if (tag && (++message_depth != 1 || integer(hero, 0x964) != static_cast<int>(mob)))
    stop("message_context");
  SetLastError(saved);
  reinterpret_cast<Message>(forge_hooks.original(1))(npc, msg, sender);
  const auto error = GetLastError();
  if (tag) {
    --message_depth;
    if (order.active()) {
      if (integer(hero, 0x964) != static_cast<int>(mob))
        stop("message_context");
      else if (tag == obs::tags[0])
        witness(f::Witness::insert);
      else if (tag == obs::tags[5])
        witness(f::Witness::exchange);
      else if (tag == obs::tags[1])
        witness(f::Witness::remove);
      else
        stop("unexpected_item");
    }
  }
  SetLastError(error);
}
void __fastcall interrupted(void* object, void*, void* npc) {
  const auto error = GetLastError();
  if (belongs(object, npc))
    stop("interrupted");
  SetLastError(error);
  reinterpret_cast<Notify>(forge_hooks.original(2))(object, npc);
}
void __fastcall interact(void* object, void*, void* npc, int action, int up, int down, int left,
                         int right) {
  const auto saved = GetLastError();
  if (belongs(object, npc) && order.selected() > 1 && addr(_ReturnAddress()) == 0x698643) {
    if (action || up || down || left || right || !world_ok() ||
        !ubc::NativeSelectionUi::callback_is_top(callback))
      stop("input_interrupted");
    else if (!forward_sent && order.awaiting_forward() && integer(mob, 0x1f4) == 1 &&
             integer(mob, 0x1fc) == 1 && station_owns(mob, hero)) {
      Address model{};
      if (field(hero, 0xc8, model) &&
          !std::strcmp(obs::current(memory, model, station->scheme).name,
                       station->working_animation)) {
        if (station->kind == f::Station::pan && !order.pan_workpiece(sample())) {
          stop("workpiece_changed");
        } else {
          forward_sent = true;
          up = 1;
          station_event("original_forward");
        }
      }
    }
  }
  SetLastError(saved);
  reinterpret_cast<Interact>(forge_hooks.original(3))(object, npc, action, up, down, left, right);
}
} // namespace

void install_forge() noexcept {
  if (!w::target_matches()) {
    event("forge_image_rejected");
    return;
  }
  installed_forge =
      forge_hooks.prepare({addr(&primary_input), addr(&item_message), addr(&interrupted),
                           addr(&interact), addr(&secondary_input), addr(&world_key_state),
                           addr(&world_key_toggled)}) &&
      forge_hooks.install(addr(&primary_input));
  event(installed_forge ? "forge_hooks_installed" : "forge_hooks_rejected");
}
void forge_start(void* object, void* npc) noexcept {
  if (belongs(object, npc))
    witness(f::Witness::reservation);
}
void forge_end(void* object, void* npc, int flag, Address caller) noexcept {
  if (!flag && belongs(object, npc)) {
    if (caller != ubc::station_end_contract::completed_return || integer(hero, 0x964) != 0)
      stop("end_rejected");
    else if (witness(f::Witness::end)) {
      ended_frame = frame;
      return_focus.reset(GetTickCount64());
    }
  }
}
bool forge_event(void* menu, int key) noexcept {
  if (!own_thread() || !installed_forge || !active())
    return false;
  if (menu != window.object()) {
    stop("external_menu");
    return false;
  }
  if (window.owns_input() && window.focused() && !closing)
    chooser.handle(window.station_action(key));
  else
    stop("focus_lost");
  return true;
}
void forge_frame() noexcept {
  if (!installed_forge || (!active() && !locked))
    return;
  const auto tick = GetTickCount64();
  const bool down = held(!window.opened() && !pending);
  release.observe(frame, down, window.focused());
  if (active() && (!world_ok() || tick - last_tick > 1500))
    stop("context_lost");
  last_tick = tick;
  if (closing) {
    if (window.opened())
      window.close();
    closing = false;
  }
  if (!active()) {
    if (release.armed())
      locked = false;
    return;
  }
  if (pending) {
    pending = false;
    if (!ready_place()) {
      stop("place_changed");
      return;
    }
    if (!window.open() ||
        !window.render_station_quantity(chooser, station->kind == f::Station::pan)) {
      passthrough = true;
      stop("window_rejected");
      return;
    }
    window.apply_backdrop();
    station_event("menu_opened", opening_stock.input_count, static_cast<int>(chooser.maximum()));
  }
  if (window.opened()) {
    if (!window.owns_input() || !ready_place()) {
      stop("menu_focus_lost");
      return;
    }
    chooser.observe(frame, down, true);
    if (chooser.decision() < 0) {
      if (!window.render_station_quantity(chooser, station->kind == f::Station::pan))
        stop("render_failed");
      return;
    }
    if (!chooser.released())
      return;
    if (!chooser.decision()) {
      stop("menu_cancelled");
      window.close();
      return;
    }
    window.close();
    if (!order.open(opening_stock, static_cast<unsigned>(chooser.decision()), station->kind)) {
      stop("order_rejected");
      return;
    }
    station_event("order_selected", static_cast<int>(order.selected()));
  }
  if (order.phase() == f::Phase::ended && frame > ended_frame) {
    Address focus{};
    if (!field(hero, 0x9ac, focus) || !ready_place(false) || held(true) || !release.armed() ||
        !ubc::NativeSelectionUi::callback_is_top(callback) || !order.neutral_stock(sample())) {
      stop("neutral_rejected");
      return;
    }
    const auto target = !focus         ? f::Focus::missing
                        : focus == mob ? f::Focus::forge
                                       : f::Focus::other;
    const auto result = return_focus.observe(tick, target);
    if (result == f::NeutralResult::rejected) {
      stop("return_focus_rejected");
      return;
    }
    if (result == f::NeutralResult::waiting)
      return;
    if (!witness(f::Witness::neutral))
      return;
    if (order.selected() > 1)
      window.show_station_progress(order.completed(), order.selected(),
                                   station->kind == f::Station::pan);
    if (!order.active())
      station_event("order_complete", static_cast<int>(order.completed()));
    return;
  }
  if (order.phase() != f::Phase::ready) {
    if (tick > deadline)
      stop("timeout");
    return;
  }
  if (held(true) || !release.armed() || !ubc::NativeSelectionUi::callback_is_top(callback) ||
      !ready_place() || station_profile(mob) != station || profile_issue() ||
      !order.start(sample())) {
    stop("restart_rejected");
    return;
  }
  forward_sent = false;
  deadline = tick + 15000;
  station_event("original_entry", static_cast<int>(order.completed() + 1),
                static_cast<int>(order.selected()));
  // This original request reserves once and posts oCMobMsg(STARTINTERACTION, hero).
  // It can return before StartInteraction. Never retry a dispatched request.
  reinterpret_cast<Enter>(enter)(reinterpret_cast<void*>(mob), reinterpret_cast<void*>(hero), 1);
}
} // namespace ubc_native
