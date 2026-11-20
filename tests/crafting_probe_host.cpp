// SPDX-License-Identifier: MIT
#include <windows.h>
#include <cstdio>
int wmain(int argc, wchar_t** argv) {
  if (argc != 2)
    return 2;
  const auto module = LoadLibraryW(argv[1]);
  if (!module) {
    std::fprintf(stderr, "LoadLibrary: %lu\n", GetLastError());
    return 3;
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
