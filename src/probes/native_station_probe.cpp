// SPDX-License-Identifier: MIT
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/engine/hand_snapshot.hpp"
#include "ubc/engine/info_snapshot.hpp"
#include "ubc/engine/read_only_memory.hpp"
#include "ubc/engine/runtime_script.hpp"
#include "ubc/stations/station_abi.hpp"
#include "ubc/stations/station_snapshot.hpp"
#include "ubc/engine/windows_probe.hpp"

#if !defined(_M_IX86)
#error Die Stationsprobe benoetigt die Windows-x86-ABI.
#endif

namespace {
namespace win = ubc::windows_probe;
namespace script = ubc::runtime_script;
namespace abi = ubc::station_abi;

constexpr std::uint32_t parser_address = 0x00ab40c0u;
constexpr std::uint32_t hero_slot = 0x00ab2684u;
constexpr std::size_t max_events = 9997; // Reserve three further error diagnostics.
constexpr std::size_t max_log_bytes = 4u * 1024u * 1024u;
constexpr ubc::ReadOnlyMemory memory{nullptr, win::copy_memory};

HMODULE own_module = nullptr;
HANDLE diagnostic = INVALID_HANDLE_VALUE;
volatile LONG hooks_installed = 0;
volatile LONG observer_thread = 0;
bool observer_enabled = false;
std::size_t events = 0;
std::size_t log_bytes = 0;
volatile LONG log_busy = 0;
volatile LONG fault_count = 0;
volatile LONG fault_busy = 0;
PVOID exception_observer = nullptr;
std::uint32_t own_image_size = 0;
bool first_before[6]{};
bool first_after[6]{};
// Only the observation thread bound once uses this counter. No
// static DLL TLS. Exclude other threads before every access,
// even during script initialization while loading the world.
std::uint32_t callback_depth = 0;
ULONGLONG last_sample = 0;

using StartInteraction = void(__thiscall*)(void*, void*);
using EndInteraction = void(__thiscall*)(void*, void*, int);
using Update = void(__fastcall*)(void*, void*);
using Info = void(__thiscall*)(void*);
using Choice = void(__fastcall*)(void*, void*);
StartInteraction original_start = nullptr;
EndInteraction original_end = nullptr;
Update original_update = nullptr;
Info original_info = nullptr;
Choice original_choice = nullptr;
constexpr auto& observed_sites = abi::hook_sites;
win::Hooks<abi::HookSite, observed_sites.size()> hooks(observed_sites);

void __fastcall observe_start(void*, void*, void*);
void __fastcall observe_end(void*, void*, void*, int);
void __fastcall observe_update(void*, void*);
void __fastcall observe_info(void*, void*);
void __fastcall observe_choice(void*, void*);

template <typename T> bool read(std::uint32_t address, T& value) noexcept {
  return memory.copy(address, &value, sizeof(value));
}

template <typename T> bool field(std::uint32_t base, std::uint32_t offset, T& value) noexcept {
  return base && offset <= UINT32_MAX - base && read(base + offset, value);
}

std::int32_t integer(std::uint32_t base, std::uint32_t offset) noexcept {
  std::int32_t result = -1;
  return field(base, offset, result) ? result : -1;
}

bool can_observe() noexcept {
  if (!InterlockedCompareExchange(&hooks_installed, 0, 0))
    return false;
  const auto thread = static_cast<LONG>(GetCurrentThreadId());
  const auto owner = InterlockedCompareExchange(&observer_thread, thread, 0);
  return (!owner || owner == thread) && observer_enabled;
}

bool write_data(const char* text, std::size_t size) noexcept {
  if (diagnostic == INVALID_HANDLE_VALUE)
    return false;
  if (InterlockedCompareExchange(&log_busy, 1, 0))
    return false;
  if (size > max_log_bytes - log_bytes) {
    InterlockedExchange(&log_busy, 0);
    return false;
  }
  DWORD written = 0;
  const bool valid =
      WriteFile(diagnostic, text, static_cast<DWORD>(size), &written, nullptr) && written == size;
  log_bytes += written;
  InterlockedExchange(&log_busy, 0);
  return valid;
}

bool write_raw(const char* text) noexcept { return write_data(text, std::strlen(text)); }

void emit(const char* line) noexcept {
  if (!observer_enabled)
    return;
  if (events >= max_events) {
    observer_enabled = false;
    return;
  }
  if (write_raw(line))
    ++events;
  else
    observer_enabled = false;
}

void trace_callback(unsigned hook, bool after) noexcept {
  auto& seen = after ? first_after[hook] : first_before[hook];
  if (seen)
    return;
  seen = true;
  char line[128]{};
  std::snprintf(line, sizeof(line),
                "{\"event\":\"first_callback\",\"hook\":%u,\"phase\":\"%s\"}\r\n", hook,
                after ? "after" : "before");
  emit(line);
  if (hook == 3 && after) {
    std::array<std::uint8_t, 7> entry{};
    const auto& expected = abi::do_stack_disk_entry;
    const bool readable = memory.copy(abi::do_stack_entry, entry.data(), entry.size());
    std::snprintf(line, sizeof(line),
                  "{\"event\":\"script_entry_state\",\"readable\":%s,"
                  "\"opcode_unchanged\":%s,\"seh_operand_changed\":%s}\r\n",
                  readable ? "true" : "false",
                  readable && !std::memcmp(entry.data(), expected.data(), 3) ? "true" : "false",
                  readable && std::memcmp(entry.data() + 3, expected.data() + 3, 4) ? "true"
                                                                                    : "false");
    emit(line);
  }
}

// Bounded error locations only. No absolute pointers or memory contents. A first-chance
// exception does not necessarily mean a crash. The engine continues handling it.
LONG CALLBACK observe_exception(EXCEPTION_POINTERS* error) noexcept {
  const auto saved_error = GetLastError();
  if (InterlockedCompareExchange(&fault_busy, 1, 0)) {
    SetLastError(saved_error);
    return EXCEPTION_CONTINUE_SEARCH;
  }
  if (error && error->ExceptionRecord && error->ContextRecord &&
      (error->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
       error->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION ||
       error->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)) {
    const auto ip = static_cast<std::uint32_t>(error->ContextRecord->Eip);
    unsigned location = 0, offset = 0;
    const auto own = reinterpret_cast<std::uintptr_t>(own_module);
    if (ip >= 0x400000u && ip < 0xb52000u) {
      location = 1;
      offset = ip - 0x400000u;
    } else if (ip >= own && ip - own < own_image_size) {
      location = 2;
      offset = ip - own;
    } else
      for (std::size_t i = 0; i < observed_sites.size(); ++i) {
        const auto code = reinterpret_cast<std::uintptr_t>(hooks.original(i));
        if (code && ip >= code && ip - code < observed_sites[i].patch_size + 5u) {
          location = static_cast<unsigned>(4 + i);
          offset = ip - code;
          break;
        }
      }
    // Third-party libraries can deliberately catch first-chance exceptions during
    // startup. They do not consume the budget for our fixed code locations.
    if ((!location && ip != 0) || InterlockedIncrement(&fault_count) > 3) {
      InterlockedExchange(&fault_busy, 0);
      SetLastError(saved_error);
      return EXCEPTION_CONTINUE_SEARCH;
    }
    unsigned access = UINT32_MAX, target_zero = 0, target_state = 0, target_protect = 0;
    if (error->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        error->ExceptionRecord->NumberParameters >= 2) {
      access = static_cast<unsigned>(error->ExceptionRecord->ExceptionInformation[0]);
      const auto target = error->ExceptionRecord->ExceptionInformation[1];
      target_zero = target == 0;
      MEMORY_BASIC_INFORMATION region{};
      if (VirtualQuery(reinterpret_cast<const void*>(target), &region, sizeof(region))) {
        target_state = region.State;
        target_protect = region.Protect;
      }
    }
    char line[384];
    std::size_t used = 0;
    const auto append = [&](const char* text) noexcept {
      while (*text && used < sizeof(line))
        line[used++] = *text++;
    };
    const auto number = [&](std::uint32_t value) noexcept {
      char digits[10];
      unsigned count = 0;
      do {
        digits[count++] = static_cast<char>('0' + value % 10);
        value /= 10;
      } while (value);
      while (count && used < sizeof(line))
        line[used++] = digits[--count];
    };
    append("{\"event\":\"first_chance_fault\",\"exception_code\":");
    number(error->ExceptionRecord->ExceptionCode);
    append(",\"location\":");
    number(location);
    append(",\"offset\":");
    number(offset);
    append(",\"instruction_zero\":");
    append(ip == 0 ? "true" : "false");
    append(",\"access\":");
    number(access);
    append(",\"target_zero\":");
    number(target_zero);
    append(",\"target_state\":");
    number(target_state);
    append(",\"target_protect\":");
    number(target_protect);
    append("}\r\n");
    if (used < sizeof(line))
      write_data(line, used);
  }
  InterlockedExchange(&fault_busy, 0);
  SetLastError(saved_error);
  return EXCEPTION_CONTINUE_SEARCH;
}

struct VerifiedRuntime {
  bool exact = false;
  bool local_contract = false;
  bool bindings = false;
  std::uint32_t code_base = 0;
  std::uint32_t code_length = 0;
  std::uint32_t symbols = 0;
  std::uint32_t table = 0;
} verified;

void hash_text(const ubc::lhiver::Digest& hash, char (&text)[65]) noexcept {
  constexpr char hex[] = "0123456789abcdef";
  for (std::size_t i = 0; i < hash.size(); ++i) {
    text[2 * i] = hex[hash[i] >> 4];
    text[2 * i + 1] = hex[hash[i] & 15];
  }
  text[64] = 0;
}

__declspec(noinline) void verify_profile() noexcept {
  const auto result = script::diagnose_runtime(memory, parser_address);
  verified = {result.exact_identity_matches,
              result.local_contract_matches,
              result.bindings_match,
              result.code_base,
              result.code_length,
              result.symbol_count,
              result.symbol_table};
  char digest[65]{}, prefix[65]{};
  hash_text(result.observed_code_hash, digest);
  hash_text(result.observed_prefix_hash, prefix);
  char line[768]{};
  std::snprintf(line, sizeof(line),
                "{\"event\":\"runtime_profile\",\"seq\":%u,\"identity\":\"%s\","
                "\"reason\":\"%s\",\"symbols\":%u,\"code_length\":%u,"
                "\"layout_stable\":%s,\"code_hash_available\":%s%s%s%s}\r\n",
                static_cast<unsigned>(events + 1), verified.exact ? "exact" : "rejected",
                script::issue_name(result.exact_identity_issue), result.symbol_count,
                result.code_length, result.layout_stable ? "true" : "false",
                result.code_hash_available ? "true" : "false",
                result.code_hash_available ? ",\"code_sha256\":\"" : "",
                result.code_hash_available ? digest : "", result.code_hash_available ? "\"" : "");
  emit(line);
  std::snprintf(
      line, sizeof(line),
      "{\"event\":\"runtime_contract\",\"seq\":%u,\"identity\":\"%s\","
      "\"reason\":\"%s\",\"failed_symbol\":%u,\"functions_checked\":%u,"
      "\"bindings_checked\":%u,\"prefix_matches\":%s,\"prefix_hash_available\":%s%s%s%s}\r\n",
      static_cast<unsigned>(events + 1), verified.local_contract ? "local_only" : "rejected",
      script::issue_name(result.local_contract_issue), result.failed_symbol,
      static_cast<unsigned>(result.functions_checked),
      static_cast<unsigned>(result.bindings_checked),
      result.original_prefix_matches ? "true" : "false",
      result.prefix_hash_available ? "true" : "false",
      result.prefix_hash_available ? ",\"prefix_sha256\":\"" : "",
      result.prefix_hash_available ? prefix : "", result.prefix_hash_available ? "\"" : "");
  emit(line);
  std::snprintf(line, sizeof(line),
                "{\"event\":\"runtime_bindings\",\"seq\":%u,\"identity\":\"%s\","
                "\"reason\":\"%s\",\"failed_symbol\":%u}\r\n",
                static_cast<unsigned>(events + 1), verified.bindings ? "bindings_only" : "rejected",
                script::issue_name(result.bindings_issue), result.first_failed_binding);
  emit(line);
}

// A state event explicitly marks this identity as cached. The
// infrequent start, end and script calls each get a full hash check.
bool runtime_still_bound(unsigned kind = 0) noexcept {
  std::uint32_t table = 0, count = 0, base = 0, end = 0;
  return (kind == 2 ? verified.bindings : (kind == 1 ? verified.local_contract : verified.exact)) &&
         field(parser_address, 0x18, table) && table == verified.table &&
         field(parser_address, 0x20, count) && count == verified.symbols &&
         field(parser_address, 0x48, base) && base == verified.code_base &&
         field(parser_address, 0x50, end) && end >= base && end - base == verified.code_length;
}

// Map only fixed names to numbers. Unrelated text and pointer addresses
// never leave the process. 0=empty, -1=unreadable, -2=another valid name.
int name_code(std::uint32_t object, std::uint32_t offset, const char* const* allowed,
              std::size_t count) noexcept {
  std::uint32_t data = 0;
  std::int32_t length = 0;
  if (offset > UINT32_MAX - 12 || !field(object, offset + 8, data) ||
      !field(object, offset + 12, length) || length < 0 || length > 127)
    return -1;
  if (!length)
    return 0;
  std::array<char, 128> text{};
  if (!memory.copy(data, text.data(), static_cast<std::size_t>(length) + 1) ||
      text[static_cast<std::size_t>(length)] ||
      std::memchr(text.data(), 0, static_cast<std::size_t>(length)))
    return -1;
  for (std::size_t i = 0; i < count; ++i)
    if (std::strcmp(text.data(), allowed[i]) == 0)
      return static_cast<int>(i + 1);
  return -2;
}

int station_binding(std::uint32_t mob, std::uint32_t hero) noexcept {
  std::uint32_t node = 0;
  if (!field(mob, 0x18c, node))
    return -1;
  std::array<std::uint32_t, 64> visited{};
  std::size_t count = 0;
  bool found = false;
  while (node) {
    if (count == visited.size())
      return -1;
    for (std::size_t i = 0; i < count; ++i)
      if (visited[i] == node)
        return -1;
    visited[count++] = node;
    std::uint32_t position = 0, next = 0, occupant = 0;
    if (!field(node, 0, position) || !field(node, 4, next) || !field(position, 0x44, occupant))
      return -1;
    found = found || occupant == hero;
    node = next;
  }
  return found ? 1 : 0;
}

enum Field : std::size_t {
  hero_present,
  hero_binding,
  production,
  invincible,
  inventory_valid,
  totals_known,
  inventory_nodes,
  station_present,
  station_hero_link,
  station_npc_link,
  scheme,
  on_state,
  use_item,
  condition,
  state,
  state_target,
  state_num,
  update_done,
  waiting_selection,
  waiting_script,
  waiting_open,
  waiting_close,
  waiting_end,
  must_open,
  left_item,
  right_item,
  left_state,
  right_state,
  left_amount,
  right_amount,
  field_count
};
constexpr std::array<const char*, field_count> field_names{
    {"hero_present", "hero_binding",    "production",      "invincible",        "inventory_valid",
     "totals_known", "inventory_nodes", "station_present", "station_binding",   "station_npc_link",
     "scheme",       "on_state",        "use_item",        "condition",         "state",
     "state_target", "state_num",       "update_done",     "waiting_selection", "waiting_script",
     "waiting_open", "waiting_close",   "waiting_end",     "must_open",         "left_item",
     "right_item",   "left_state",      "right_state",     "left_amount",       "right_amount"}};

struct Snapshot {
  bool identity = false;
  bool local_contract = false;
  bool bindings = false;
  std::array<std::int32_t, field_count> fields{};
  std::array<std::int32_t, 6> listed{};
  bool operator==(const Snapshot& other) const noexcept {
    return identity == other.identity && local_contract == other.local_contract &&
           bindings == other.bindings && fields == other.fields && listed == other.listed;
  }
};
Snapshot previous{};
bool have_previous = false;

// The inventory reader needs fixed work tables. Its temporary read
// frame ends before each original call, even in release builds.
__declspec(noinline) Snapshot capture(std::uint32_t passed_station = 0,
                                      std::uint32_t info = 0) noexcept {
  Snapshot result;
  result.fields.fill(-1);
  result.listed.fill(-1);
  auto& f = result.fields;
  result.identity = runtime_still_bound();
  result.local_contract = runtime_still_bound(1);
  result.bindings = runtime_still_bound(2);
  std::uint32_t hero = 0;
  if (!read(hero_slot, hero))
    return result;
  f[hero_present] = hero ? 1 : 0;
  if (!hero)
    return result;
  const auto hands = ubc::read_hands(memory, hero);
  f[left_state] = hands.left.state;
  f[right_state] = hands.right.state;
  f[left_item] = hands.left.symbol;
  f[right_item] = hands.right.symbol;
  f[left_amount] = hands.left.amount;
  f[right_amount] = hands.right.amount;
  if (result.bindings) {
    const auto symbol =
        script::read_symbol_address(memory, parser_address, ubc::lhiver::hero_symbol);
    std::uint32_t bound_hero = 0;
    if (symbol.valid && field(symbol.address, 0x1c, bound_hero))
      f[hero_binding] = bound_hero == hero;
    const auto value = script::symbol_int(memory, parser_address, ubc::lhiver::production_symbol);
    if (value.valid)
      f[production] = value.value;
    f[invincible] = integer(hero, 0x298);
    const auto inventory = ubc::read_inventory(memory, hero, ubc::lhiver::item_symbols);
    f[inventory_valid] = inventory.valid;
    f[totals_known] = inventory.totals_known;
    f[inventory_nodes] = static_cast<std::int32_t>(inventory.nodes);
    if (inventory.valid)
      result.listed = inventory.listed;
  }
  std::uint32_t current_station = 0;
  if (!field(hero, 0x964, current_station))
    return result;
  const auto mob = passed_station ? passed_station : current_station;
  f[station_present] = mob ? 1 : 0;
  f[station_hero_link] = mob ? (mob == current_station ? 1 : 0) : 0;
  if (mob) {
    f[station_npc_link] = station_binding(mob, hero);
    constexpr const char* schemes[] = {"CAMPFIRE", "STOVE", "POTION_ALCHEMY", "ALCHEMY"};
    constexpr const char* states[] = {"FRYINGMEAT", "POTIONALCHEMY"};
    constexpr const char* items[] = {"ITFOMUTTONRAW", "ITMI_FLASK"};
    constexpr const char* conditions[] = {"B_USECAMPFIRE", "B_USESTOVE", "B_USEPOTIONLAB"};
    f[scheme] = name_code(mob, 0x1b8, schemes, std::size(schemes));
    f[on_state] = name_code(mob, 0x1e0, states, std::size(states));
    f[use_item] = name_code(mob, 0x1a4, items, std::size(items));
    f[condition] = name_code(mob, 0x1cc, conditions, std::size(conditions));
    f[state] = integer(mob, 0x1f4);
    f[state_num] = integer(mob, 0x1f8);
    f[state_target] = integer(mob, 0x1fc);
  }
  if (info) {
    f[update_done] = integer(info, 0x2c);
    f[waiting_end] = integer(info, 0x30);
    f[waiting_script] = integer(info, 0x34);
    f[waiting_open] = integer(info, 0x38);
    f[waiting_close] = integer(info, 0x3c);
    f[waiting_selection] = integer(info, 0x40);
    f[must_open] = integer(info, 0x44);
  }
  return result;
}

void log_snapshot(const char* event, const char* phase, const Snapshot& snapshot, int function = -1,
                  int entry = -1) noexcept {
  if (!observer_enabled)
    return;
  std::uint32_t pc = 0, base = 0;
  std::int32_t offset = -1;
  if (field(parser_address, 0x48, base) && field(parser_address, 0x4c, pc) && pc >= base &&
      pc - base <= INT32_MAX)
    offset = static_cast<std::int32_t>(pc - base);
  char line[2048]{};
  const int head = std::snprintf(
      line, sizeof(line),
      "{\"event\":\"%s\",\"phase\":\"%s\",\"seq\":%u,\"callback_depth\":%u,"
      "\"identity\":\"%s\",\"local_contract\":%s,\"binding_contract_cached\":%s,\"function_id\":%d,\"entry\":%d,\"pc\":%d,\"curfunc\":%d",
      event, phase, static_cast<unsigned>(events + 1), callback_depth,
      snapshot.identity ? "verified_cached" : "unverified",
      snapshot.local_contract ? "true" : "false", snapshot.bindings ? "true" : "false", function,
      entry, offset, integer(parser_address, 0x219c));
  if (head < 0 || static_cast<std::size_t>(head) >= sizeof(line))
    return;
  auto used = static_cast<std::size_t>(head);
  for (std::size_t i = 0; i < field_count; ++i) {
    const int count = std::snprintf(line + used, sizeof(line) - used, ",\"%s\":%d", field_names[i],
                                    snapshot.fields[i]);
    if (count < 0 || static_cast<std::size_t>(count) >= sizeof(line) - used)
      return;
    used += static_cast<std::size_t>(count);
  }
  const auto& values = snapshot.listed;
  const int tail =
      std::snprintf(line + used, sizeof(line) - used, ",\"listed\":[%d,%d,%d,%d,%d,%d]}\r\n",
                    values[0], values[1], values[2], values[3], values[4], values[5]);
  if (tail < 0 || static_cast<std::size_t>(tail) >= sizeof(line) - used)
    return;
  emit(line);
}

const ubc::lhiver::Function* candidate_for_symbol(std::int32_t symbol) noexcept {
  if (symbol < 0)
    return nullptr;
  for (const auto& candidate : ubc::lhiver::functions)
    if (candidate.symbol == static_cast<std::uint32_t>(symbol))
      return &candidate;
  return nullptr;
}

bool is_production_station() noexcept {
  std::uint32_t hero = 0, mob = 0;
  if (!read(hero_slot, hero) || !hero || !field(hero, 0x964, mob) || !mob)
    return false;
  constexpr const char* names[] = {"FRYINGMEAT", "POTIONALCHEMY"};
  return name_code(mob, 0x1e0, names, std::size(names)) > 0;
}

void information_snapshot(const char* event, const char* phase, const ubc::InfoSnapshot& info,
                          bool valid, std::int32_t function, std::int32_t choice_index,
                          std::uint32_t choice_nodes) noexcept {
  // The structure metadata consists of copied values. It does not authorize a recipe.
  char line[512]{};
  std::snprintf(line, sizeof(line),
                "{\"event\":\"information_metadata\",\"phase\":\"%s\",\"seq\":%u,"
                "\"valid\":%s,\"info_instance\":%d,\"owner_function\":%d,"
                "\"function_id\":%d,\"choice_index\":%d,\"choice_nodes\":%u}\r\n",
                phase, static_cast<unsigned>(events + 1), valid ? "true" : "false", info.instance,
                info.function, function, choice_index, choice_nodes);
  emit(line);
  verify_profile();
  const auto* candidate = candidate_for_symbol(function);
  if (candidate) {
    const auto recognized = script::recognize_function(
        memory, parser_address, static_cast<std::int32_t>(candidate->byte_offset));
    std::snprintf(line, sizeof(line),
                  "{\"event\":\"function_identity\",\"phase\":\"%s\",\"seq\":%u,"
                  "\"function_id\":%u,\"identity\":\"%s\",\"reason\":\"%s\"}\r\n",
                  phase, static_cast<unsigned>(events + 1), candidate->symbol,
                  recognized.recognized ? "recognized" : "rejected",
                  script::issue_name(recognized.issue));
    emit(line);
  }
  log_snapshot(event, phase, capture(0, abi::information_manager), function,
               candidate ? static_cast<int>(candidate->byte_offset) : -1);
}

void __fastcall observe_info(void* info, void*) {
  const auto before_error = GetLastError();
  const bool own_thread = can_observe();
  if (own_thread)
    trace_callback(4, false);
  const auto metadata = own_thread ? ubc::read_info(memory, reinterpret_cast<std::uint32_t>(info))
                                   : ubc::InfoSnapshot{};
  const bool watch =
      own_thread && (candidate_for_symbol(metadata.function) || is_production_station());
  const bool counted = watch && callback_depth < UINT32_MAX;
  if (counted)
    ++callback_depth;
  if (watch)
    information_snapshot("information", "before", metadata, metadata.valid, metadata.function, -1,
                         0);
  SetLastError(before_error);
  original_info(info);
  const auto after_error = GetLastError();
  if (own_thread)
    trace_callback(4, true);
  if (watch && observer_enabled)
    information_snapshot("information", "after", metadata, metadata.valid, metadata.function, -1,
                         0);
  if (counted)
    --callback_depth;
  SetLastError(after_error);
}

void __fastcall observe_choice(void* manager, void* choice) {
  const auto before_error = GetLastError();
  const bool own_thread = can_observe();
  if (own_thread)
    trace_callback(5, false);
  const auto metadata = own_thread
                            ? ubc::read_choice(memory, reinterpret_cast<std::uint32_t>(manager),
                                               reinterpret_cast<std::uint32_t>(choice))
                            : ubc::ChoiceSnapshot{};
  const bool watch =
      own_thread && (candidate_for_symbol(metadata.function) || is_production_station());
  const bool counted = watch && callback_depth < UINT32_MAX;
  if (counted)
    ++callback_depth;
  if (watch)
    information_snapshot("choice", "before", metadata.info, metadata.valid, metadata.function,
                         metadata.index, metadata.nodes);
  SetLastError(before_error);
  original_choice(manager, choice);
  const auto after_error = GetLastError();
  if (own_thread)
    trace_callback(5, true);
  // Use only the metadata copied beforehand. The original
  // may already have freed choice and the entire choice list.
  if (watch && observer_enabled)
    information_snapshot("choice", "after", metadata.info, metadata.valid, metadata.function,
                         metadata.index, metadata.nodes);
  if (counted)
    --callback_depth;
  SetLastError(after_error);
}

bool is_hero(void* npc) noexcept {
  std::uint32_t hero = 0;
  return read(hero_slot, hero) && hero && hero == reinterpret_cast<std::uintptr_t>(npc);
}

void __fastcall observe_start(void* mob, void*, void* npc) {
  const auto before_error = GetLastError();
  const bool own_thread = can_observe();
  if (own_thread)
    trace_callback(1, false);
  const bool watch = own_thread && is_hero(npc);
  if (watch) {
    verify_profile();
    log_snapshot("interaction_start", "before", capture(reinterpret_cast<std::uint32_t>(mob)));
  }
  SetLastError(before_error);
  original_start(mob, npc);
  const auto after_error = GetLastError();
  if (own_thread)
    trace_callback(1, true);
  if (watch && observer_enabled) {
    verify_profile();
    log_snapshot("interaction_start", "after", capture());
  }
  SetLastError(after_error);
}

void __fastcall observe_end(void* mob, void*, void* npc, int flag) {
  const auto before_error = GetLastError();
  const bool own_thread = can_observe();
  if (own_thread)
    trace_callback(2, false);
  const bool watch = own_thread && is_hero(npc);
  if (watch) {
    verify_profile();
    log_snapshot("interaction_end", "before", capture(reinterpret_cast<std::uint32_t>(mob)));
  }
  SetLastError(before_error);
  original_end(mob, npc, flag);
  const auto after_error = GetLastError();
  if (own_thread)
    trace_callback(2, true);
  if (watch && observer_enabled) {
    verify_profile();
    log_snapshot("interaction_end", "after", capture());
  }
  SetLastError(after_error);
}

void __fastcall observe_update(void* info, void* unused_edx) {
  // Every original update runs even during throttling or rejection.
  const auto before_error = GetLastError();
  const bool own_thread = can_observe();
  if (own_thread)
    trace_callback(3, false);
  SetLastError(before_error);
  original_update(info, unused_edx);
  const auto after_error = GetLastError();
  if (own_thread) {
    trace_callback(3, true);
    const auto now = GetTickCount64();
    if (now - last_sample >= 250) {
      last_sample = now;
      const auto snapshot = capture(0, reinterpret_cast<std::uint32_t>(info));
      const bool relevant = snapshot.fields[station_present] == 1 ||
                            (have_previous && previous.fields[station_present] == 1);
      if (relevant && (!have_previous || !(snapshot == previous))) {
        log_snapshot("station_state", "after", snapshot);
        previous = snapshot;
        have_previous = true;
      } else if (!relevant) {
        have_previous = false;
      }
    }
  }
  SetLastError(after_error);
}

void install() noexcept {
  wchar_t path[32768]{};
  if (!win::sibling_path(own_module, path, L"UnionBatchCraftingStationProbe.log"))
    return;
  diagnostic = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
  if (diagnostic == INVALID_HANDLE_VALUE)
    return;
  const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(own_module);
  const auto* pe = reinterpret_cast<const IMAGE_NT_HEADERS*>(
      reinterpret_cast<const std::uint8_t*>(own_module) + dos->e_lfanew);
  own_image_size = pe->OptionalHeader.SizeOfImage;
  if (!write_raw("station_probe_version=1\r\ntarget_identity_match=true\r\n"
                 "activation_marker_consumed=true\r\nrecipe_calls=false\r\n"
                 "inventory_calls=false\r\n"))
    return;
  const std::array<std::uintptr_t, 5> destinations{
      {reinterpret_cast<std::uintptr_t>(&observe_start),
       reinterpret_cast<std::uintptr_t>(&observe_end),
       reinterpret_cast<std::uintptr_t>(&observe_update),
       reinterpret_cast<std::uintptr_t>(&observe_info),
       reinterpret_cast<std::uintptr_t>(&observe_choice)}};
  if (!hooks.prepare(destinations)) {
    write_raw("hooks_installed=false\r\nrejection=hook_preparation\r\n");
    return;
  }
  original_start = reinterpret_cast<StartInteraction>(hooks.original(0));
  original_end = reinterpret_cast<EndInteraction>(hooks.original(1));
  original_update = reinterpret_cast<Update>(hooks.original(2));
  original_info = reinterpret_cast<Info>(hooks.original(3));
  original_choice = reinterpret_cast<Choice>(hooks.original(4));
  if (!hooks.install(reinterpret_cast<std::uintptr_t>(&observe_start))) {
    write_raw("hooks_installed=false\r\nrejection=hook_installation\r\n");
    return;
  }
  observer_enabled = write_raw("hooks_installed=true\r\n");
  observer_enabled = write_raw("script_observer=false\r\n") && observer_enabled;
  observer_enabled = write_raw("information_observer=true\r\n") && observer_enabled;
  // The header is in place and all trampoline addresses are immutable from this point on.
  exception_observer = AddVectoredExceptionHandler(1, observe_exception);
  observer_enabled = write_raw(exception_observer ? "exception_observer=true\r\n"
                                                  : "exception_observer=false\r\n") &&
                     observer_enabled;
  // Publish for game callbacks only after full initialization.
  InterlockedExchange(&hooks_installed, 1);
}
} // namespace

extern "C" unsigned __cdecl UbcStationProbeVersion() { return 1; }
extern "C" int __cdecl UbcStationProbeHooksInstalled() {
  return static_cast<int>(InterlockedCompareExchange(&hooks_installed, 0, 0));
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    own_module = instance;
    const auto error = GetLastError();
    if (win::target_matches() &&
        win::consume_marker(instance, L"UnionBatchCraftingStationProbe.enable-once",
                            "STATION_OBSERVER_V1\n"))
      install();
    SetLastError(error);
  } else if (reason == DLL_PROCESS_DETACH && diagnostic != INVALID_HANDLE_VALUE) {
    if (exception_observer)
      RemoveVectoredExceptionHandler(exception_observer);
    CloseHandle(diagnostic);
    diagnostic = INVALID_HANDLE_VALUE;
  }
  return TRUE;
}
