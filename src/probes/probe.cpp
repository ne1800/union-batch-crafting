#include <windows.h>
#include <cstddef>
#include <cstdint>
#include "ubc/engine/pe_guard.hpp"

namespace {
int target_matched = 0;

bool inspect_main_module() noexcept {
  const auto* base = reinterpret_cast<const std::uint8_t*>(GetModuleHandleW(nullptr));
  if (!base)
    return false;
  MEMORY_BASIC_INFORMATION memory{};
  if (VirtualQuery(base, &memory, sizeof(memory)) != sizeof(memory))
    return false;
  if (memory.State != MEM_COMMIT || memory.BaseAddress != base)
    return false;
  if ((memory.Protect & PAGE_GUARD) != 0)
    return false;
  const DWORD protection = memory.Protect & 0xff;
  if (protection != PAGE_READONLY && protection != PAGE_READWRITE && protection != PAGE_WRITECOPY &&
      protection != PAGE_EXECUTE_READ && protection != PAGE_EXECUTE_READWRITE &&
      protection != PAGE_EXECUTE_WRITECOPY)
    return false;
  // A PE header filter. It does not authorize the use of arbitrary engine addresses.
  const auto size = memory.RegionSize < 4096 ? memory.RegionSize : 4096;
  return ubc::matches_gothic2_header(base, size);
}

void write_diagnostic(HMODULE own_module) noexcept {
  wchar_t path[32768]{};
  const DWORD count = GetModuleFileNameW(own_module, path, 32768);
  if (count == 0 || count >= 32768)
    return;
  DWORD directory_end = count;
  while (directory_end > 0 && path[directory_end - 1] != L'\\' && path[directory_end - 1] != L'/') {
    --directory_end;
  }
  if (directory_end == 0)
    return;
  constexpr wchar_t name[] = L"UnionBatchCraftingProbe.log";
  constexpr std::size_t length = sizeof(name) / sizeof(wchar_t);
  if (directory_end > 32768 - length)
    return;
  for (std::size_t i = 0; i < length; ++i)
    path[directory_end + i] = name[i];
  const HANDLE file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE)
    return;
  constexpr char message[] = "probe_version=1\r\n"
                             "target_header_match=true\r\n"
                             "gameplay_hooks=false\r\n"
                             "script_calls=false\r\n";
  DWORD written = 0;
  WriteFile(file, message, static_cast<DWORD>(sizeof(message) - 1), &written, nullptr);
  CloseHandle(file);
}
} // namespace

extern "C" unsigned __cdecl UbcProbeVersion() { return 1; }
extern "C" int __cdecl UbcProbeTargetMatched() { return target_matched; }

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    target_matched = inspect_main_module() ? 1 : 0;
    if (target_matched)
      write_diagnostic(instance);
  }
  return TRUE;
}
