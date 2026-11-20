// SPDX-License-Identifier: MIT
#pragma once
#include <cwchar>
#include "ubc/engine/player_engine.hpp"
#include "ubc/engine/windows_probe.hpp"

namespace ubc::player_engine {
inline bool game_process() noexcept {
  if (windows_probe::player_image_header())
    return true;
  wchar_t path[32768]{};
  const auto length = GetModuleFileNameW(nullptr, path, 32768);
  if (!length || length >= 32768)
    return false;
  const auto* name = std::wcsrchr(path, L'\\');
  return name && _wcsicmp(name + 1, L"Gothic2.exe") == 0;
}
inline Result inspect_executable() noexcept {
  wchar_t path[32768]{};
  Result failed;
  failed.failure.fill(1);
  const auto length = GetModuleFileNameW(nullptr, path, 32768);
  if (!length || length >= 32768)
    return failed;
  HANDLE file =
      CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
  if (file == INVALID_HANDLE_VALUE)
    return failed;
  LARGE_INTEGER size{};
  HANDLE mapping = nullptr;
  if (GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart <= 64 * 1024 * 1024)
    mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
  if (mapping) {
    const auto* view =
        static_cast<const std::uint8_t*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
    if (view) {
      failed = inspect(view, static_cast<std::size_t>(size.QuadPart), sites, std::size(sites));
      UnmapViewOfFile(view);
    }
    CloseHandle(mapping);
  }
  CloseHandle(file);
  return failed;
}
} // namespace ubc::player_engine
