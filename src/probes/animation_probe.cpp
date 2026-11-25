// SPDX-License-Identifier: MIT
#include "../plugin/player_campaign_windows.hpp"
#include "ubc/engine/animation_observation.hpp"
#include "ubc/engine/packed_inventory.hpp"
#include "ubc/stations/station_abi.hpp"
#include "ubc/stations/station_recipe_snapshot.hpp"
#include <cstdio>

namespace {
namespace obs = ubc::animation_observation;
namespace abi = ubc::station_abi;
namespace win = ubc::windows_probe;
using obs::Address;
namespace slots = ubc::station_recipe_snapshot_detail;
constexpr ubc::ReadOnlyMemory memory{nullptr, win::copy_memory};
constexpr std::array<ubc::PackedInventoryTarget, 5> items{{{"ITFOMUTTONRAW", 6719},
                                                           {"ITFOMUTTON", 6721},
                                                           {"ITMI_PANFULL", 6910},
                                                           {"ITMISWORDRAW", 6912},
                                                           {"ITMISWORDRAWHOT", 6913}}};
constexpr std::array<abi::HookSite, 7> sites{
    {abi::hook_sites[0],
     abi::hook_sites[1],
     {0x742a20, 7, 14, {{0x6a, 0xff, 0x68, 0x38, 0x50, 0x82, 0x00, 0x64, 0xa1, 0, 0, 0, 0, 0x50}}},
     {0x74b020, 7, 14, {{0x6a, 0xff, 0x68, 0xda, 0x57, 0x82, 0x00, 0x64, 0xa1, 0, 0, 0, 0, 0x50}}},
     {0x720ce0, 5, 12, {{0x83, 0xec, 0x18, 0x53, 0x56, 0x8b, 0x74, 0x24, 0x24, 0x57, 0x8b, 0xf9}}},
     {0x721c20, 6, 14, {{0x64, 0xa1, 0, 0, 0, 0, 0x6a, 0xff, 0x68, 0xe4, 0x29, 0x82, 0, 0x50}}},
     {0x720f40, 7, 14, {{0x6a, 0xff, 0x68, 0x9e, 0x28, 0x82, 0, 0x64, 0xa1, 0, 0, 0, 0, 0x50}}}}};
win::Hooks<abi::HookSite, sites.size()> hooks(sites);
HMODULE module{};
HANDLE log_file = INVALID_HANDLE_VALUE;
DWORD owner{};
Address actor{}, pending_mob{};
bool was_linked{};
const char* station{};
unsigned seq{}, interaction{};
char last_animation[48]{};
int last_state{}, last_target{};
ULONGLONG tail_until{};
ubc::PackedInventoryWorkspace inventory_work;
slots::HandWorkspace hand_work;
using CanInteract = int(__thiscall*)(void*, void*);
using AniEvents = int(__thiscall*)(void*);
using Message = void(__thiscall*)(void*, void*, void*);

template <class T> T value(Address base, unsigned offset, T unknown = {}) noexcept {
  T v{};
  return obs::field(memory, base, offset, v) ? v : unknown;
}
void write(const char* line) noexcept {
  DWORD written{};
  const auto size = static_cast<DWORD>(std::strlen(line));
  if (log_file != INVALID_HANDLE_VALUE &&
      (!WriteFile(log_file, line, size, &written, nullptr) || written != size)) {
    CloseHandle(log_file);
    log_file = INVALID_HANDLE_VALUE;
  }
}
bool observing(void* npc) noexcept {
  return owner == GetCurrentThreadId() && station && actor == reinterpret_cast<Address>(npc) &&
         (!tail_until || GetTickCount64() <= tail_until) &&
         actor == value<Address>(abi::hero_pointer, 0);
}
void snapshot(const char* event, const char* phase, Address mob, const char* tag = "",
              int tag_frame = -1, int flag = -1) noexcept {
  if (seq >= 16000 || log_file == INVALID_HANDLE_VALUE)
    return;
  const auto ani = obs::current(memory, value<Address>(actor, 0xc8), station);
  const int state = value<int>(mob, 0x1f4, -2), target = value<int>(mob, 0x1fc, -2);
  if (!std::strcmp(event, "animation")) {
    if (!std::strcmp(ani.name, last_animation) && state == last_state && target == last_target)
      return;
    std::memcpy(last_animation, ani.name, sizeof(last_animation));
    last_state = state;
    last_target = target;
  }
  const bool verified =
      slots::begin_hands(memory, actor, hand_work) == ubc::StationHandReadIssue::none &&
      slots::verify_hands(memory, hand_work);
  const auto hands = verified ? hand_work.before : ubc::StationRecipeHands{};
  const auto& h = hands.hands;
  const auto a = hands.interact_item_known
                     ? ubc::hand_detail::read_object(memory, hands.interact_item)
                     : ubc::HandObjectSnapshot{};
  Address linked{};
  const int link_state = obs::field(memory, actor, 0x964, linked) ? int(linked != 0) : -1;
  const auto inv = ubc::read_packed_inventory(memory, actor, items, true, inventory_work);
  char line[1024]{};
  std::snprintf(
      line, sizeof(line),
      "{\"seq\":%u,\"tick\":%llu,\"interaction\":%u,\"station\":\"%s\",\"event\":\"%s\","
      "\"phase\":\"%s\",\"tag\":\"%s\",\"event_frame\":%d,\"animation\":\"%s\",\"ani_frame\":%d,"
      "\"state\":%d,\"target\":%d,\"linked\":%d,\"flag\":%d,"
      "\"left\":[%d,%d,%d],\"right\":[%d,%d,%d],\"anchor\":[%d,%d,%d],"
      "\"temporary_levels\":[%d,%d],\"anchor_slot\":\"%s\","
      "\"stock_issue\":\"%s\",\"stored\":[%d,%d,%d,%d,%d]}\r\n",
      ++seq, GetTickCount64(), interaction, station, event, phase, tag, tag_frame, ani.name,
      ani.frame, state, target, link_state, flag, h.left.state, h.left.symbol, h.left.amount,
      h.right.state, h.right.symbol, h.right.amount, a.state, a.symbol, a.amount,
      hands.left_tmp_level, hands.right_tmp_level, obs::anchor_slot(h, a),
      ubc::packed_inventory_issue_name(inv.issue), inv.total[0], inv.total[1], inv.total[2],
      inv.total[3], inv.total[4]);
  write(line);
  if (seq == 16000)
    write("{\"event\":\"limit\"}\r\n");
}
const char* watched_station(Address mob, void* npc) noexcept {
  if (owner != GetCurrentThreadId() || !npc ||
      reinterpret_cast<Address>(npc) != value<Address>(abi::hero_pointer, 0))
    return nullptr;
  char name[36]{};
  return obs::string(memory, mob, 0x1b8, name) ? obs::scheme(name) : nullptr;
}
void begin(Address hero, const char* scheme) noexcept {
  actor = hero;
  station = scheme;
  ++interaction;
  last_animation[0] = 0;
  was_linked = false;
  tail_until = 0;
}
int __fastcall can_interact(void* mob, void*, void* npc) {
  const auto error = GetLastError();
  const auto address = reinterpret_cast<Address>(mob);
  const auto hero = reinterpret_cast<Address>(npc);
  const auto* scheme = watched_station(address, npc);
  const bool watch = scheme && !value<Address>(hero, 0x964) && !value<Address>(hero, 0x968);
  if (watch) {
    begin(hero, scheme);
    pending_mob = address;
    snapshot("can_interact", "before", address);
  }
  SetLastError(error);
  const int result = reinterpret_cast<CanInteract>(hooks.original(6))(mob, npc);
  const auto after = GetLastError();
  if (watch && observing(npc)) {
    snapshot("can_interact", "after", address, "", -1, result);
    if (!result) {
      pending_mob = 0;
      tail_until = GetTickCount64() + 1500;
    }
  }
  SetLastError(after);
  return result;
}
void __fastcall start(void* mob, void*, void* npc) {
  const auto error = GetLastError();
  const auto address = reinterpret_cast<Address>(mob);
  const auto* scheme = watched_station(address, npc);
  if (scheme) {
    if (pending_mob != address || actor != reinterpret_cast<Address>(npc))
      begin(reinterpret_cast<Address>(npc), scheme);
    pending_mob = 0;
    snapshot("start", "before", address);
  } else if (owner == GetCurrentThreadId() && reinterpret_cast<Address>(npc) == actor) {
    station = nullptr;
    pending_mob = 0;
  }
  SetLastError(error);
  reinterpret_cast<abi::StartInteraction>(hooks.original(0))(mob, npc);
  const auto after = GetLastError();
  if (scheme && observing(npc)) {
    const auto current = value<Address>(actor, 0x964);
    was_linked = current != 0;
    snapshot("start", "after", current);
    if (!current)
      tail_until = GetTickCount64() + 1500;
  }
  SetLastError(after);
}
void __fastcall end(void* mob, void*, void* npc, int flag) {
  const auto error = GetLastError();
  const bool watch = observing(npc);
  if (watch)
    snapshot("end", "before", reinterpret_cast<Address>(mob), "", -1, flag);
  SetLastError(error);
  reinterpret_cast<abi::EndInteraction>(hooks.original(1))(mob, npc, flag);
  const auto after = GetLastError();
  if (watch && observing(npc)) {
    const auto current = value<Address>(actor, 0x964);
    snapshot("end", "after", current, "", -1, flag);
    if (!current)
      tail_until = GetTickCount64() + 1500;
  }
  SetLastError(after);
}
// These paths may run inside EndInteraction or independently on forced interruption.
template <unsigned Site> void __fastcall finish(void* mob, void*, void* npc) {
  const auto error = GetLastError();
  const auto event = Site == 4 ? "interrupt" : "stop";
  const bool watch = observing(npc);
  if (watch)
    snapshot(event, "before", reinterpret_cast<Address>(mob));
  SetLastError(error);
  reinterpret_cast<abi::StartInteraction>(hooks.original(Site))(mob, npc);
  const auto after = GetLastError();
  if (watch && observing(npc))
    snapshot(event, "after", value<Address>(actor, 0x964));
  SetLastError(after);
}
int __fastcall animate(void* npc, void*) {
  const auto error = GetLastError();
  if (observing(npc)) {
    const auto mob = value<Address>(actor, 0x964);
    if (was_linked && !mob) {
      snapshot("detached", "after", 0);
      tail_until = GetTickCount64() + 1500;
    }
    was_linked = mob != 0;
    snapshot("animation", "sample", mob);
    if (!obs::visit_tags(memory, value<Address>(actor, 0xc8), [mob](const char* tag, int frame) {
          snapshot("model_tag", "before", mob, tag, frame);
        }))
      snapshot("event_list_unreadable", "before", mob);
  }
  SetLastError(error);
  return reinterpret_cast<AniEvents>(hooks.original(2))(npc);
}
void __fastcall message(void* npc, void*, void* msg, void* sender) {
  const auto error = GetLastError();
  const char* tag =
      observing(npc) ? obs::message_tag(memory, reinterpret_cast<Address>(msg)) : nullptr;
  if (tag)
    snapshot("item_message", "before", value<Address>(actor, 0x964), tag);
  SetLastError(error);
  reinterpret_cast<Message>(hooks.original(3))(npc, msg, sender);
  const auto after = GetLastError();
  if (tag && observing(npc))
    snapshot("item_message", "after", value<Address>(actor, 0x964), tag);
  SetLastError(after);
}
} // namespace

void arm_animation_probe(HMODULE instance) noexcept { module = instance; }
void close_animation_probe() noexcept {
  if (log_file != INVALID_HANDLE_VALUE)
    CloseHandle(log_file);
  log_file = INVALID_HANDLE_VALUE;
}
extern "C" __declspec(dllexport) void __cdecl Game_Init() {
  if (!module)
    return;
  const auto saved = GetLastError();
  wchar_t path[32768]{};
  const auto instance = module;
  module = nullptr;
  if (win::sibling_path(instance, path, L"UnionBatchCraftingAnimation.log"))
    log_file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
  if (log_file != INVALID_HANDLE_VALUE) {
    owner = GetCurrentThreadId();
    if (ubc::player_campaign::active_dat() != ubc::player_campaign::Campaign::dndr) {
      write("{\"event\":\"unsupported_dat\"}\r\n");
    } else if (hooks.prepare(
                   {{reinterpret_cast<Address>(start), reinterpret_cast<Address>(end),
                     reinterpret_cast<Address>(animate), reinterpret_cast<Address>(message),
                     reinterpret_cast<Address>(finish<4>), reinterpret_cast<Address>(finish<5>),
                     reinterpret_cast<Address>(can_interact)}}) &&
               hooks.install(reinterpret_cast<Address>(start))) {
      write(
          "{\"event\":\"animation_probe_ready\",\"stored_order\":[6719,6721,6910,6912,6913]}\r\n");
    } else
      write("{\"event\":\"hooks_rejected\"}\r\n");
  }
  SetLastError(saved);
}
