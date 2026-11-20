// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/station_abi.hpp"

namespace ubc::selection_abi {
using station_abi::HookSite;
// Local EXE: SHA-256 0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f.
// Own facts after comparison with gothicuef/gothic-api commit
// 65fb327c76f58c751468c8e7df917a86a89b9ed1. No copied engine classes.
inline constexpr std::uint32_t game_pointer = 0xab0884;
inline constexpr std::uint32_t game_vtable = 0x83c28c;
inline constexpr std::uint32_t game_world = 8;
inline constexpr std::uint32_t frame_site = 0x425e6e;
inline constexpr std::uint32_t frame_prefix = 0x425e55;
inline constexpr std::array<std::uint8_t, 25> frame_prefix_bytes{{
  0x8b,0x7d,0x0c,0x3b,0xfe,0x74,0x19,0x8b,0x07,0x8b,0xcf,0xff,0x50,
  0x24,0x85,0xc0,0x74,0x0e,0x8b,0x17,0x8b,0xcf,0xff,0x52,0x10}};
inline constexpr HookSite frame_hook{frame_site, 7, 12,
  {{0x8b,0x07,0x8b,0xcf,0xff,0x50,0x14,0xbf,0xf0,0xe6,0x82,0x00}}};
inline constexpr HookSite add_hook{0x68f710, 7, 13,
  {{0x6a,0xff,0x68,0x28,0x69,0x81,0x00,0x64,0xa1,0x00,0x00,0x00,0x00}}};
inline constexpr HookSite clear_hook{0x68f9c0, 10, 17,
  {{0x33,0xc0,0x33,0xd2,0x89,0x81,0x04,0x01,0x00,0x00,
    0xc7,0x81,0x00,0x01,0x00,0x00,0xff}}};
inline constexpr HookSite resolve_hook{0x703030, 7, 13,
  {{0x6a,0xff,0x68,0x70,0x06,0x82,0x00,0x64,0xa1,0x00,0x00,0x00,0x00}}};

inline constexpr std::uint32_t info_add_return = 0x661b72;
inline constexpr std::uint32_t info_clear_return = 0x661b1c;
inline constexpr std::uint32_t choice_add_return = 0x661d8b;
inline constexpr std::uint32_t choice_clear_return = 0x661d62;
inline constexpr std::uint32_t resolve_selection_return = 0x66106f;
inline constexpr std::uint32_t info_effect_return = 0x6623f2;
inline constexpr std::uint32_t choice_effect_return = 0x660f5d;
inline constexpr std::uint32_t manager_choice = 0x1c;
inline constexpr std::uint32_t view_vtable = 0x83bc3c;
inline constexpr std::uint32_t view_callback_vtable = 0x83bc08;
inline constexpr std::uint32_t view_callback = 0xec;
inline constexpr std::uint32_t view_size = 0x10c;
inline constexpr std::uint32_t view_opened = 0x7c;
inline constexpr std::uint32_t view_closed = 0x80;
inline constexpr std::uint32_t view_open_time = 0x84;
inline constexpr std::uint32_t view_close_time = 0x88;
inline constexpr std::uint32_t view_done = 0xf0;
inline constexpr std::uint32_t view_active = 0xf4;
inline constexpr std::uint32_t view_selected = 0x100;
inline constexpr std::uint32_t view_count = 0x104;
inline constexpr std::uint32_t choice_select = 0x68f440;
inline constexpr std::uint32_t choice_handle_event = 0x68eba0;
inline constexpr std::uint32_t choice_activate = 0x6895b0;
inline constexpr std::uint32_t choice_open_immediate = 0x690c90;
inline constexpr std::uint32_t choice_close_immediate = 0x690d90;
inline constexpr std::uint32_t info_begin = 0x661ff0;
inline constexpr std::uint32_t menu_screen_init = 0x4dcde0;
inline constexpr std::uint32_t menu_screen_done = 0x4dd9e0;

#if defined(_WIN32) && defined(_M_IX86)
using FastObject = void(__fastcall*)(void*, void*);
using Select = void(__fastcall*)(void*, int);
using AddChoice = void(__fastcall*)(void*, void*, int);
using ResolveInfo = void*(__thiscall*)(void*, void*, void*, int);
using HandleEvent = int(__thiscall*)(void*, int);
#endif
} // namespace ubc::selection_abi
