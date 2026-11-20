#include <windows.h>
#include <cstdio>

int wmain(int argc, wchar_t** argv) {
  if (argc != 2) {
    std::fputs("Aufruf: ubc_station_probe_host.exe <DLL-Pfad>\n", stderr);
    return 2;
  }
  const auto module = LoadLibraryW(argv[1]);
  if (!module) {
    std::fprintf(stderr, "LoadLibrary fehlgeschlagen: %lu\n", GetLastError());
    return 3;
  }
  using Version = unsigned(__cdecl*)();
  using Installed = int(__cdecl*)();
  const auto version = reinterpret_cast<Version>(GetProcAddress(module, "UbcStationProbeVersion"));
  const auto installed = reinterpret_cast<Installed>(GetProcAddress(module, "UbcStationProbeHooksInstalled"));
  const bool valid = version && installed && version() == 1 && installed() == 0;
  const bool unloaded = FreeLibrary(module) != 0;
  if (!valid || !unloaded) return 4;
  std::puts("station_probe_version=1 hooks_installed=0 unknown_target_rejected=true");
  return 0;
}
