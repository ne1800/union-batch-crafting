// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/engine/player_campaign.hpp"
#include "ubc/engine/windows_probe.hpp"
#include <shellapi.h>

namespace ubc::player_campaign {
inline bool marcello_selected() noexcept {
  int argc = 0;
  auto** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (!argv)
    return false;
  bool selected = false;
  for (int i = 1; i < argc; ++i) {
    const std::wstring_view arg(argv[i]);
    if (equal_name(arg.substr(0, 6), L"-game:")) {
      selected = marcello_ini(arg.substr(6));
      break;
    }
  }
  LocalFree(argv);
  return selected;
}

template <class T> bool read_value(std::uint32_t address, T& value) noexcept {
  return windows_probe::copy_memory(nullptr, address, &value, sizeof(value));
}
template <std::size_t N> bool read_string(std::uint32_t address, char (&out)[N]) noexcept {
  std::array<std::uint32_t, 2> text{};
  if (!read_value(address + 8, text) || !text[0] || !text[1] || text[1] >= N ||
      !windows_probe::copy_memory(nullptr, text[0], out, text[1] + 1))
    return false;
  return out[text[1]] == '\0' && std::strlen(out) == text[1];
}

inline bool active_dat_matches() noexcept {
  // Game_Init runs after oCGame::Init loaded the game parser. Read its actual
  // source and resolved directory, including Paths.d and forced mod options.
  constexpr auto parser = lhiver::game_parser;
  std::uint32_t options = 0, loaded_dat = 0;
  int reparse = 0;
  std::uint8_t physical_first = 0;
  if (!windows_probe::player_image_header() || !read_value(0x8cd988, options) || !options ||
      !read_value(parser + 0x2088, loaded_dat) || loaded_dat != 1 ||
      !read_value(0xab6284, reparse) || reparse || !read_value(0x8c34c5, physical_first))
    return false;
  char source[MAX_PATH]{}, directory[MAX_PATH]{}, root[MAX_PATH]{}, dat[MAX_PATH]{};
  constexpr std::uint32_t dir_strings = 0x7c, compiled_scripts = 19, root_dir = 24;
  if (!read_string(parser + 0x2074, source) ||
      !read_string(options + dir_strings + compiled_scripts * 20, directory) ||
      !read_string(options + dir_strings + root_dir * 20, root) ||
      !dat_path(root, directory, source, dat))
    return false;

  const auto vdfs = GetModuleHandleW(L"vdfs32g.dll");
  if (!vdfs)
    return false;
  const auto open =
      reinterpret_cast<long(__cdecl*)(const char*, long)>(GetProcAddress(vdfs, "vdf_fopen"));
  const auto read =
      reinterpret_cast<long(__cdecl*)(long, void*, long)>(GetProcAddress(vdfs, "vdf_fread"));
  const auto size = reinterpret_cast<long(__cdecl*)(long)>(GetProcAddress(vdfs, "vdf_ffilesize"));
  const auto close = reinterpret_cast<long(__cdecl*)(long)>(GetProcAddress(vdfs, "vdf_fclose"));
  if (!open || !read || !size || !close)
    return false;
  constexpr long virtual_and_physical = 1 | 2, physical_first_flag = 4;
  const long file = open(dat, virtual_and_physical | (physical_first ? physical_first_flag : 0));
  if (file == -1)
    return false;
  const bool match = matches(
      size(file), [&](void* out, long count) { return read(file, out, count); }, 9327865,
      lhiver::dat_sha256);
  close(file);
  return match;
}
} // namespace ubc::player_campaign
