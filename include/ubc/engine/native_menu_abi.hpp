// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

// Small, specifically formulated ABI description for Gothic II AddOn
// 2.6fix. Addresses and layout were statically cross-checked against the
// released EXE. No engine class is emulated or provided with its own vtable.
namespace ubc::native_menu {

inline constexpr std::uintptr_t image_base = 0x00400000;
inline constexpr std::size_t image_size = 0x00752000;
inline constexpr std::array<std::uint8_t, 32> executable_sha256{
    {0x06, 0x55, 0xb4, 0x81, 0x1e, 0x00, 0x8a, 0x35, 0x3c, 0xe3, 0x72,
     0xfd, 0xe4, 0x63, 0xef, 0x00, 0xb6, 0x10, 0x1f, 0x3e, 0x89, 0xbe,
     0x85, 0x0e, 0x2f, 0x14, 0x98, 0x01, 0x40, 0x44, 0x7c, 0x7f}};

inline constexpr std::size_t menu_size = 0x0cc4;
inline constexpr std::size_t string_size = 0x14;
inline constexpr std::size_t string_data = 0x08;
inline constexpr std::size_t string_length = 0x0c;
inline constexpr std::size_t view_size = 0x100;
inline constexpr std::size_t vptr_offset = 0;
inline constexpr std::size_t menu_flags = 0x0c10;
inline constexpr std::size_t menu_default_out_game = 0x0c14;
inline constexpr std::size_t menu_default_in_game = 0x0c18;
inline constexpr std::size_t menu_window = 0x0c1c;
inline constexpr std::size_t menu_inner_window = 0x0c20;
inline constexpr std::size_t menu_exit_state = 0x0c24;
inline constexpr std::size_t menu_name = 0x0c28;
inline constexpr std::size_t menu_main_selection = 0x0c40;
inline constexpr std::size_t menu_screen_initialized = 0x0c44;
inline constexpr std::size_t menu_item_count = 0x0cc0;
inline constexpr std::size_t view_id = 0x10;
inline constexpr std::size_t view_owner = 0x3c;
inline constexpr std::size_t view_back_texture = 0x40;
inline constexpr std::int32_t view_item = 2;
inline constexpr std::int32_t exit_back = 1;

inline constexpr std::uintptr_t menu_vtable = 0x00830c3c;
inline constexpr std::uintptr_t main_menu_vtable = 0x008300b4;
inline constexpr char main_menu_name[] = "MENU_MAIN";
inline constexpr std::uintptr_t menu_ctor = 0x004da470;
inline constexpr std::uintptr_t menu_dtor = 0x004da750;
inline constexpr std::uintptr_t string_ctor = 0x004010c0;
inline constexpr std::uintptr_t string_dtor = 0x00401160;
inline constexpr std::uintptr_t menu_run = 0x004db9a0;
inline constexpr std::uintptr_t menu_handle_event = 0x004dbb70;
inline constexpr std::uintptr_t menu_render = 0x004ddc20;
// ScreenInit constructs its inner zCViewWindow at 0x4dcf41 and attaches it
// to menu_window at 0x4dcf64. Vtable slot 10 dispatches zCView::Blit.
inline constexpr std::uintptr_t menu_view_vtable = 0x0083140c;
inline constexpr std::size_t view_blit_slot = 10;
inline constexpr std::uintptr_t view_blit = 0x007a63c0;
// Blit passes backTex and color (alpha from +0x28) to DrawTile at
// 0x7a66df before BlitText at 0x7a6701. Glyphs use fontColor (+0x68).
inline constexpr std::uintptr_t view_insert_back = 0x007a6130; // zSTRING const&
inline constexpr std::uintptr_t view_set_blend = 0x007a6070;   // enum const& -> +0x20
inline constexpr std::uintptr_t view_set_alpha = 0x007a6060;   // int by value -> +0x28
inline constexpr int alpha_blend = 2;
inline constexpr int backdrop_opacity = 160;
inline constexpr std::uintptr_t view_clear_printwin = 0x007aa140;
inline constexpr std::uintptr_t view_set_font = 0x007a9920;
inline constexpr std::uintptr_t view_print = 0x007a9a40;
inline constexpr std::uintptr_t input_logical_key = 0x004cc5d0;
inline constexpr std::uintptr_t input_pointer = 0x008d1650;
inline constexpr std::uintptr_t in_game_menu = 0x008d1e78;
inline constexpr std::uintptr_t screen_pointer = 0x00ab6468;
inline constexpr std::size_t key_pressed_slot = 7;
inline constexpr std::size_t clear_key_buffer_slot = 14;

inline constexpr int key_escape = 0x01;
inline constexpr int key_return = 0x1c;
inline constexpr int key_up = 0xc8;
inline constexpr int key_left = 0xcb;
inline constexpr int key_right = 0xcd;
inline constexpr int key_down = 0xd0;
inline constexpr unsigned short game_left = 1;
inline constexpr unsigned short game_right = 2;
inline constexpr unsigned short game_up = 3;
inline constexpr unsigned short game_down = 4;
inline constexpr unsigned short game_strafe_left = 18;
inline constexpr unsigned short game_strafe_right = 19;

struct alignas(4) MenuStorage {
  std::array<std::uint8_t, menu_size> bytes{};
};
struct alignas(4) StringStorage {
  std::array<std::uint8_t, string_size> bytes{};
};
static_assert(sizeof(MenuStorage) == menu_size && alignof(MenuStorage) == 4);
static_assert(sizeof(StringStorage) == string_size && alignof(StringStorage) == 4);

template <typename T> T read(const void* object, std::size_t offset) noexcept {
  static_assert(std::is_trivially_copyable_v<T>);
  T value{};
  std::memcpy(&value, static_cast<const std::uint8_t*>(object) + offset, sizeof(T));
  return value;
}

template <typename T> void write(void* object, std::size_t offset, T value) noexcept {
  static_assert(std::is_trivially_copyable_v<T>);
  std::memcpy(static_cast<std::uint8_t*>(object) + offset, &value, sizeof(T));
}

struct HookSite {
  std::uintptr_t address;
  std::size_t patch_size;
  std::size_t guard_size;
  std::array<std::uint8_t, 18> original;
};

// Before the split point (patch_size) only whole instructions without relative
// operands. The following bytes additionally serve as a version guard.
inline constexpr std::array<HookSite, 3> hook_sites{
    {{menu_run,
      13,
      18,
      {{0x56, 0x8b, 0xf1, 0xc7, 0x86, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa1, 0x40,
        0x1e, 0x8d, 0x00}}},
     {menu_handle_event,
      5,
      12,
      {{0x53, 0x55, 0x56, 0x8b, 0xf1, 0x57, 0x8b, 0xbe, 0x40, 0x0c, 0x00, 0x00}}},
     {menu_render, 5, 11, {{0x51, 0x53, 0x57, 0x8b, 0xf9, 0x8b, 0x87, 0xc0, 0x0c, 0x00, 0x00}}}}};

#if defined(_WIN32) && defined(_M_IX86)
static_assert(sizeof(void*) == 4, "Die native ABI ist ausschliesslich x86.");
using ObjectFunction = void(__thiscall*)(void*);
using MenuRun = int(__thiscall*)(void*);
using MenuHandleEvent = int(__thiscall*)(void*, int);
using StringConstructor = void(__thiscall*)(void*, const char*);
using ViewSetFont = void(__thiscall*)(void*, const void*);
using ViewInsertBack = void(__thiscall*)(void*, const void*);
using ViewSetBlend = void(__thiscall*)(void*, const int&);
using ViewSetAlpha = void(__thiscall*)(void*, int);
using ViewPrint = void(__thiscall*)(void*, int, int, const void*);
using LogicalKey = unsigned short(__thiscall*)(void*, unsigned short);
using KeyPressed = int(__thiscall*)(void*, int);
#endif

} // namespace ubc::native_menu
