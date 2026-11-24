// SPDX-License-Identifier: MIT
#include <windows.h>
#include <cstdio>
#include <cwchar>
int wmain(int argc, wchar_t** argv) {
  if (argc != 2 && !(argc == 3 && std::wcscmp(argv[2], L"-game:Lhiver_marcello_de.ini") == 0))
    return 2;
  const auto module = LoadLibraryW(argv[1]);
  if (!module) {
    std::fprintf(stderr, "LoadLibrary: %lu\n", GetLastError());
    return 3;
  }
  using Init = void(__cdecl*)();
  if (const auto init = reinterpret_cast<Init>(GetProcAddress(module, "Game_Init"))) {
    init();
    init();
  }
  using Version = unsigned(__cdecl*)();
  using Installed = int(__cdecl*)();
  const auto version =
      reinterpret_cast<Version>(GetProcAddress(module, "UbcSelectionProbeVersion"));
  const auto installed =
      reinterpret_cast<Installed>(GetProcAddress(module, "UbcSelectionProbeHooksInstalled"));
  const bool valid = version && installed && version() == 4 && installed() == 0;
  if (!FreeLibrary(module) || !valid)
    return 4;
  std::puts("crafting_version=1 hooks_installed=0 unknown_target_rejected=true");
  return 0;
}
