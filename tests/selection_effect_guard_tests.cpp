// SPDX-License-Identifier: MIT
// Dedicated Windows-x86 process with no game installation or
// original scripts. Include the actual wrappers. Original targets
// are our counters. Caller value and global data slots are explicit
// test seams. This tests control flow and actual memory readers,
// NOT native address layout or the ReturnAddress/trampoline
// ABI. Global addresses map only to our VirtualAlloc buffers,
// which ReadProcessMemory then reads. Never call native engine
// functions. Link with native_selection_ui.cpp, frame_bridge.cpp,
// kernel32 and user32. No hook installation or required image base.
#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <new>
#include "ubc/engine/selection_abi.hpp"
#include "ubc/engine/windows_probe.hpp"
namespace effect_guard_test {
std::uintptr_t synthetic_caller = 0;
std::uint8_t* global_slots = nullptr;
std::uint8_t* manager_slot = nullptr;
void* synthetic_return_address() noexcept { return reinterpret_cast<void*>(synthetic_caller); }
}
namespace ubc::windows_probe {
bool effect_guard_copy_memory(void* context, std::uint32_t value, void* out,
                              std::size_t size) noexcept {
  const auto translate = [&](std::uint32_t base, std::size_t length, const std::uint8_t* target) {
    if (value < base || value - base > length || size > length - (value - base))
      return false;
    value =
        target ? static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(target)) + value - base
               : 0;
    return true;
  };
  if (!translate(selection_abi::game_pointer, 4, effect_guard_test::global_slots) &&
      !translate(station_abi::hero_pointer, 4,
                 effect_guard_test::global_slots ? effect_guard_test::global_slots + 4 : nullptr))
    translate(station_abi::information_manager, 0x58, effect_guard_test::manager_slot);
  return copy_memory(context, value, out, size);
}
}
#define _ReturnAddress() effect_guard_test::synthetic_return_address()
#define copy_memory effect_guard_copy_memory
#include "../src/probes/native_selection_probe.cpp"
#undef copy_memory
#undef _ReturnAddress

#if !defined(_M_IX86)
#error Dieser Test benoetigt Windows x86.
#endif

namespace effect_guard_test {
unsigned checks = 0, failures = 0, info_calls = 0, choice_calls = 0;
void* seen_first = nullptr;
void* seen_second = nullptr;
DWORD seen_error = 0;
constexpr DWORD incoming_error = 0x1234, original_error = 0x5678;

void check(bool value, const char* name) {
  ++checks;
  if (!value) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}
std::uint32_t address(const void* value) noexcept {
  return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(value));
}
void put(void* object, std::size_t offset, std::uint32_t value) noexcept {
  std::memcpy(static_cast<std::uint8_t*>(object) + offset, &value, sizeof(value));
}

// Only our own pages freely assigned by the system. Never
// force a mapping at a potentially occupied engine address.
class PrivatePages {
public:
  static constexpr std::size_t size = 0x10000;
  bool open() noexcept {
    for (std::size_t i = 0; i < pages_.size(); ++i) {
      pages_[i] = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
      MEMORY_BASIC_INFORMATION region{};
      if (!pages_[i] || VirtualQuery(pages_[i], &region, sizeof(region)) != sizeof(region) ||
          region.AllocationBase != pages_[i] || region.Type != MEM_PRIVATE)
        return false;
      // The test seam must not mistake actual buffer addresses for global slots.
      if (address(pages_[i]) < 0xac0000u && address(pages_[i]) + size > 0xaa0000u)
        return false;
    }
    global_slots = static_cast<std::uint8_t*>(pages_[0]);
    manager_slot = static_cast<std::uint8_t*>(pages_[1]);
    return true;
  }
  bool protect(std::size_t index, DWORD protection) noexcept {
    DWORD old = 0;
    return index < pages_.size() && pages_[index] &&
           VirtualProtect(pages_[index], size, protection, &old);
  }
  ~PrivatePages() {
    for (auto* page : pages_)
      if (page)
        VirtualFree(page, 0, MEM_RELEASE);
    global_slots = manager_slot = nullptr;
  }
  void* fixture_space() noexcept { return static_cast<std::uint8_t*>(pages_[1]) + 0x1000; }

private:
  std::array<void*, 2> pages_{};
};

// Actual addresses of our own buffers. No invented info or
// choice pointers. Vtable values are read-only ABI metadata.
// Never call through them. The production readers check them.
struct Fixture {
  alignas(4) std::array<std::uint8_t, 0x5c> info{};
  alignas(4) std::array<std::uint8_t, 0x18> choice{};
  alignas(4) std::array<std::uint8_t, 8> node{};
  alignas(4) std::array<std::uint8_t, a::view_size> view{};
  alignas(4) std::array<std::uint8_t, 12> game{}, second_game{};
  alignas(4) std::array<std::uint8_t, 0x968> hero{}, second_hero{};
  std::array<std::uint32_t, 4> tokens{};
  bool is_choice = false;

  void* manager() const noexcept { return manager_slot; }
  void reset(bool choice_mode) noexcept {
    is_choice = choice_mode;
    info.fill(0);
    choice.fill(0);
    node.fill(0);
    view.fill(0);
    game.fill(0);
    second_game.fill(0);
    hero.fill(0);
    second_hero.fill(0);
    std::memset(manager(), 0, 0x58);
    const auto& link = p::info_links[is_choice ? 4u : 0u];
    put(info.data(), 0, ubc::info_detail::info_vtable);
    put(info.data(), 0x1c, 22243);
    put(info.data(), 0x20, static_cast<std::uint32_t>(link.number));
    put(info.data(), 0x28, static_cast<std::uint32_t>(link.condition));
    put(info.data(), 0x2c, static_cast<std::uint32_t>(link.function));
    put(info.data(), 0x48, 1);
    put(info.data(), 0x50, static_cast<std::uint32_t>(link.instance));
    if (is_choice) {
      put(info.data(), 0x58, address(node.data()));
      put(node.data(), 0, address(choice.data()));
      put(choice.data(), 0x14, 20341);
    }
    put(game.data(), 0, a::game_vtable);
    put(game.data(), a::game_world, address(&tokens[0]));
    second_game = game;
    put(hero.data(), s::npc_interact_mob, address(&tokens[1]));
    put(global_slots, 0, address(game.data()));
    put(global_slots, 4, address(hero.data()));
    put(manager(), 0x20, address(hero.data()));
    put(manager(), 0x24, address(hero.data()));
    put(manager(), 0x28, address(info.data()));

    InterlockedExchange(&installed, 1);
    InterlockedExchange(&engine_thread, static_cast<LONG>(GetCurrentThreadId()));
    frame_depth = update_depth = info_depth = choice_depth = station_depth = collection_depth = 0;
    native_update = false; // No test may reach actual engine calls.
    pending = true;
    executed = recover = recovering = handoff_cancelled = false;
    effect_calls = 0;
    original_station = 0;
    log_file = INVALID_HANDLE_VALUE;
    handed_context = {};
    handed_context.game = address(game.data());
    handed_snapshot = {};
    handed_snapshot.key.world = address(&tokens[0]);
    handed_snapshot.key.hero = address(hero.data());
    handed_snapshot.key.station = address(&tokens[1]);
    handed_snapshot.key.manager = address(manager());
    handed_snapshot.key.mode = is_choice ? ubc::SelectionMode::choice : ubc::SelectionMode::info;
    handed_snapshot.key.collection = rows.generation = 1;
    handed_offer = {link.instance, is_choice ? 20341 : link.function, 0, 1};
    info_calls = choice_calls = 0;
    seen_first = seen_second = nullptr;
    seen_error = 0;
  }
  bool reader_accepts() const noexcept {
    if (is_choice) {
      const auto value = ubc::read_choice(memory, address(manager()), address(choice.data()));
      return value.valid && value.info.instance == 20337 && value.function == 20341 &&
             value.index == 0;
    }
    const auto value = ubc::read_selection_info(memory, address(info.data()));
    return value.valid && ubc::selection_info_link(value, p::info_links[0]);
  }
  void make_foreign() noexcept {
    if (is_choice)
      put(choice.data(), 0x14, 20342);
    else {
      put(info.data(), 0x50, 21011);
      put(info.data(), 0x2c, 21013);
    }
  }
  void make_bound() noexcept {
    if (is_choice)
      put(choice.data(), 0x14, static_cast<std::uint32_t>(handed_offer.function));
    else {
      put(info.data(), 0x50, static_cast<std::uint32_t>(handed_offer.info_instance));
      put(info.data(), 0x2c, static_cast<std::uint32_t>(handed_offer.function));
    }
  }
  void open_dialog() noexcept {
    put(manager(), 0x1c, address(view.data()));
    put(manager(), 0x40, 1);
    put(manager(), 0x54, is_choice ? 2u : 1u);
    put(view.data(), 0, a::view_vtable);
    put(view.data(), 0x24, 0x83bc10);
    put(view.data(), a::view_callback, a::view_callback_vtable);
    put(view.data(), a::view_count, 1);
    put(view.data(), a::view_selected, 0);
    put(view.data(), a::view_opened, 1);
    put(view.data(), a::view_active, 1);
  }
};

void __thiscall counted_info(void* object) {
  ++info_calls;
  seen_first = object;
  seen_error = GetLastError();
  SetLastError(original_error);
}
void __fastcall counted_choice(void* object, void* selected) {
  ++choice_calls;
  seen_first = object;
  seen_second = selected;
  seen_error = GetLastError();
  SetLastError(original_error);
}
using Effect = void(__fastcall*)(void*, void*);
struct Call {
  Effect entry;
  std::uintptr_t caller;
};

void invoke(Fixture& fixture, Call call, bool forwards, const char* label) {
  info_calls = choice_calls = 0;
  seen_first = seen_second = nullptr;
  seen_error = 0;
  synthetic_caller = call.caller;
  SetLastError(incoming_error);
  call.entry(fixture.is_choice ? fixture.manager() : fixture.info.data(),
             fixture.is_choice ? fixture.choice.data() : nullptr);
  const auto after = GetLastError();
  check(info_calls + choice_calls == (forwards ? 1u : 0u), label);
  check(after == (forwards ? original_error : incoming_error), "original/block LastError contract");
  check(!effect_calls && !executed && !info_depth && !choice_depth,
        "rejection does not claim an effect or leak depth");
  if (forwards) {
    check(seen_error == incoming_error &&
              seen_first == (fixture.is_choice ? fixture.manager() : fixture.info.data()) &&
              (!fixture.is_choice || seen_second == fixture.choice.data()),
          "original receives exact arguments once");
  } else
    check(pending && handoff_cancelled, "rejected ticket remains guarded and cancelled");
}
} // namespace effect_guard_test

int main() {
  using namespace effect_guard_test;
  PrivatePages pages;
  if (!pages.open()) {
    check(false, "exclusive private fixture pages available");
    return 1;
  }
  original_info = counted_info;
  original_choice = counted_choice;
  static_assert(sizeof(Fixture) <= PrivatePages::size - 0x1000, "Begrenzter eigener Fixturepuffer");
  auto& fixture = *new (pages.fixture_space()) Fixture;
  for (const bool choice_mode : {false, true}) {
    const auto entry = choice_mode ? &observe_choice : &observe_info;
    const Call normal{entry, choice_mode ? a::choice_effect_return : a::info_effect_return};
    const Call direct{entry, reinterpret_cast<std::uintptr_t>(&counted_info)};
    fixture.reset(choice_mode);
    check(fixture.reader_accepts(), "production reader accepts own complete fixture");
    check(!positively_detached(), "unchanged complete owner is not detached");
    handed_snapshot.key.mode = choice_mode ? ubc::SelectionMode::info : ubc::SelectionMode::choice;
    invoke(fixture, normal, false, "normal callsite rejects opposite pending mode");

    fixture.reset(choice_mode);
    put(choice_mode ? static_cast<void*>(fixture.choice.data()) : fixture.info.data(),
        choice_mode ? 0x14u : 0x2cu, UINT32_MAX);
    invoke(fixture, normal, false, "normal callsite rejects invalid semantic metadata");
    fixture.reset(choice_mode);
    handoff_cancelled = true;
    invoke(fixture, normal, false, "cancel latch prevents a later valid retry");
    fixture.reset(choice_mode);
    invoke(fixture, direct, false, "foreign caller cannot execute bound target");
    fixture.reset(choice_mode);
    fixture.make_foreign();
    invoke(fixture, direct, true, "unrelated foreign caller remains transparent");
    check(pending && !handoff_cancelled, "unrelated call does not consume pending ticket");
    fixture.reset(choice_mode);
    pending = false;
    invoke(fixture, normal, true, "no pending normal call forwards once");
    invoke(fixture, direct, true, "no pending direct call forwards once");

    for (unsigned change = 0; change < 5; ++change) {
      fixture.reset(choice_mode);
      if (change == 0)
        put(fixture.game.data(), a::game_world, address(&fixture.tokens[2]));
      if (change == 1)
        put(global_slots, 4, address(fixture.second_hero.data()));
      if (change == 2)
        put(global_slots, 0, address(fixture.second_game.data()));
      if (change == 3) {
        put(fixture.manager(), 0x20, address(fixture.second_hero.data()));
        put(fixture.manager(), 0x24, address(fixture.second_hero.data()));
      }
      if (change == 4)
        put(fixture.hero.data(), s::npc_interact_mob, address(&fixture.tokens[3]));
      check(positively_detached(), "positive owner/world/hero/game/station replacement detected");
      invoke(fixture, normal, false, "old callback remains blocked after owner replacement");
      fixture.make_foreign();
      invoke(fixture, normal, true, "new valid callback passes after positive replacement");
      check(pending && handoff_cancelled, "callbacks do not retire old ticket during execution");
      frame_depth = 1;
      retire_detached();
      frame_depth = 0;
      check(pending, "quiet stack alone does not retire an unfinished transition");
      fixture.make_bound();
      put(fixture.manager(), 0x3c, 1);
      frame_depth = 1;
      retire_detached();
      frame_depth = 0;
      check(pending, "closing animation keeps the old ticket guarded");
      invoke(fixture, normal, false, "old callback in a later closing frame stays blocked");
      put(fixture.manager(), 0x3c, 0);
      put(fixture.manager(), 0x2c, 1);
      frame_depth = 1;
      retire_detached();
      frame_depth = 0;
      check(!pending && handoff_cancelled,
            "quiet frame and completed manager retire detached ticket");
    }

    fixture.reset(choice_mode);
    fixture.open_dialog();
    put(fixture.manager(), 0x20, address(fixture.second_hero.data()));
    const auto dialog = ubc::read_selection_dialog(memory, s::information_manager);
    check(dialog.valid && dialog.waiting && dialog.open && !dialog.done,
          "production reader accepts own waiting dialog fixture");
    frame_depth = 1;
    retire_detached();
    frame_depth = 0;
    check(pending, "unchanged collection cannot retire detached ticket");
    rows.generation = 2;
    put(fixture.view.data(), a::view_done, 1);
    put(fixture.view.data(), a::view_active, 0);
    frame_depth = 1;
    retire_detached();
    frame_depth = 0;
    check(pending, "new collection with queued done event cannot retire ticket");
    put(fixture.view.data(), a::view_done, 0);
    put(fixture.view.data(), a::view_active, 1);
    frame_depth = 1;
    retire_detached();
    frame_depth = 0;
    check(!pending && handoff_cancelled,
          "new fully waiting open collection retires detached ticket");

    fixture.reset(choice_mode);
    if (!pages.protect(0, PAGE_NOACCESS)) {
      check(false, "make own global slot unreadable");
      return 1;
    }
    check(!positively_detached(), "unreadable game pointer is not proof of detachment");
    invoke(fixture, normal, false, "unreadable global slot retains effect guard");
    frame_depth = 1;
    retire_detached();
    frame_depth = 0;
    check(pending, "quiet frame cannot retire on read failure");
    if (!pages.protect(0, PAGE_READWRITE)) {
      check(false, "restore own fixture access");
      return 1;
    }

    fixture.reset(choice_mode);
    put(fixture.hero.data(), s::npc_interact_mob, 0);
    check(!positively_detached(), "empty mob alone is not completion");
    put(fixture.manager(), 0x2c, 1);
    put(fixture.manager(), 0x34, 1);
    check(!positively_detached(), "script wait still owns the old transition");
    put(fixture.manager(), 0x34, 0);
    check(positively_detached(), "empty mob plus completed nonwaiting manager proves end");
  }
  InterlockedExchange(&installed, 0);
  std::printf("SelectionEffectGuard: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
