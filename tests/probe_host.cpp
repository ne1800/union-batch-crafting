#include <windows.h>
#include <cstdio>
#include <cwchar>

int wmain(int argc, wchar_t** argv) {
  if (argc != 2 && !(argc == 3 && std::wcscmp(argv[2], L"1") == 0)) {
    std::fputs("Aufruf: ubc_probe_host.exe <DLL-Pfad> [1 fuer synthetischen Zielkopf]\n", stderr);
    return 2;
  }
  HMODULE module = LoadLibraryW(argv[1]);
  if (!module) {
    std::fprintf(stderr, "LoadLibrary fehlgeschlagen: %lu\n", GetLastError());
    return 3;
  }
  using Version = unsigned(__cdecl*)();
  using Matched = int(__cdecl*)();
  const auto version = reinterpret_cast<Version>(GetProcAddress(module, "UbcProbeVersion"));
  const auto matched = reinterpret_cast<Matched>(GetProcAddress(module, "UbcProbeTargetMatched"));
  if (!version || !matched) {
    std::fputs("Diagnoseexport fehlt.\n", stderr);
    FreeLibrary(module);
    return 4;
  }
  const unsigned value = version();
  const int target = matched();
  std::printf("probe_version=%u target_header_match=%d\n", value, target);
  const bool unloaded = FreeLibrary(module) != 0;
  const int expected_target = argc == 3 ? 1 : 0;
  return value == 1 && target == expected_target && unloaded ? 0 : 5;
}
