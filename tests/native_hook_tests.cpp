// SPDX-License-Identifier: MIT
// Standalone Windows-x86 test host. Changes only its own process
// memory. No game files, DLL installation or other processes.
// Link with /BASE:0x400000 /DYNAMICBASE:NO /FIXED /OPT:NOICF.
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined(_M_IX86)
#error Dieser Test prueft ausschliesslich Windows x86.
#endif

// One targeted failure exercises the otherwise difficult-to-trigger
// partial rollback. All other calls go directly to Kernel32.
BOOL WINAPI test_virtual_protect(LPVOID, SIZE_T, DWORD, PDWORD) noexcept;
#define VirtualProtect test_virtual_protect
#include "ubc/windows_probe.hpp"
#undef VirtualProtect

namespace {
void* fail_write_at = nullptr;
unsigned injected_failures = 0;
unsigned checks = 0;
unsigned failures = 0;

void check(bool condition, const char* message) {
  ++checks;
  if (!condition) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}

struct State {
  std::int32_t calls = 0;
  std::int32_t first = 0;
  std::int32_t second = 0;
  std::int32_t bias = 7;
};
static_assert(offsetof(State, bias) == 12, "Assemblerlayout");

// Deliberately use our own short instruction sequences. The patch spans of 7, 6 and
// 5 bytes each end on an instruction boundary, with a guard remaining afterward.
// ECX carries State*, and the explicit thiscall arguments are on the stack.
__declspec(naked) int __thiscall synthetic_one(void*, int) {
  __asm {
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    nop
    inc dword ptr [ecx]
    mov eax, dword ptr [ebp + 8]
    mov dword ptr [ecx + 4], eax
    add eax, dword ptr [ecx + 12]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret 4
  }
}

__declspec(naked) int __thiscall synthetic_two(void*, int, int) {
  __asm {
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    inc dword ptr [ecx]
    mov eax, dword ptr [ebp + 8]
    mov dword ptr [ecx + 4], eax
    mov edx, dword ptr [ebp + 12]
    mov dword ptr [ecx + 8], edx
    add eax, edx
    add eax, dword ptr [ecx + 12]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret 8
  }
}

__declspec(naked) int __fastcall synthetic_update(void*, void*) {
  __asm {
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    inc dword ptr [ecx]
    mov dword ptr [ecx + 4], edx
    mov eax, edx
    add eax, dword ptr [ecx + 12]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
  }
}

using One = int(__thiscall*)(void*, int);
using Two = int(__thiscall*)(void*, int, int);
using Update = int(__fastcall*)(void*, void*);
One original_one = nullptr;
Two original_two = nullptr;
Update original_update = nullptr;
std::array<unsigned, 3> callback_calls{};

int __fastcall callback_one(void* object, void*, int value) {
  ++callback_calls[0];
  return original_one(object, value) + 1000;
}
int __fastcall callback_two(void* object, void*, int first, int second) {
  ++callback_calls[1];
  return original_two(object, first, second) + 2000;
}
int __fastcall callback_update(void* object, void* second_register) {
  ++callback_calls[2];
  return original_update(object, second_register) + 3000;
}

struct Site {
  std::uintptr_t address;
  std::size_t patch_size;
  std::size_t guard_size;
  std::array<std::uint8_t, 32> original;
};
using Sites = std::array<Site, 3>;
using Hooks = ubc::windows_probe::Hooks<Site, 3>;

Sites sites() {
  constexpr std::array<std::uint8_t, 32> one{{0x55, 0x89, 0xe5, 0x53, 0x56, 0x57, 0x90, 0xff, 0x01}};
  constexpr std::array<std::uint8_t, 32> two{{0x55, 0x89, 0xe5, 0x53, 0x56, 0x57, 0xff, 0x01}};
  constexpr std::array<std::uint8_t, 32> update{{0x55, 0x89, 0xe5, 0x53, 0x56, 0x57}};
  return {{{reinterpret_cast<std::uintptr_t>(&synthetic_one), 7, 9, one},
           {reinterpret_cast<std::uintptr_t>(&synthetic_two), 6, 8, two},
           {reinterpret_cast<std::uintptr_t>(&synthetic_update), 5, 6, update}}};
}

std::array<std::uintptr_t, 3> destinations() {
  return {{reinterpret_cast<std::uintptr_t>(&callback_one),
           reinterpret_cast<std::uintptr_t>(&callback_two),
           reinterpret_cast<std::uintptr_t>(&callback_update)}};
}

bool original_heads(const Sites& values) {
  for (const auto& site : values)
    if (std::memcmp(reinterpret_cast<const void*>(site.address),
                    site.original.data(), site.guard_size)) return false;
  return true;
}

bool trampoline_valid(const Site& site, const void* trampoline) {
  MEMORY_BASIC_INFORMATION region{};
  if (!trampoline || VirtualQuery(trampoline, &region, sizeof(region)) != sizeof(region) ||
      region.State != MEM_COMMIT || region.Type != MEM_PRIVATE ||
      (region.Protect & 0xffu) != PAGE_EXECUTE_READ || (region.Protect & PAGE_GUARD)) return false;
  const auto* bytes = static_cast<const std::uint8_t*>(trampoline);
  if (std::memcmp(bytes, site.original.data(), site.patch_size) || bytes[site.patch_size] != 0xe9)
    return false;
  std::uint32_t relative = 0;
  std::memcpy(&relative, bytes + site.patch_size + 1, sizeof(relative));
  const auto target = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(bytes)) +
      static_cast<std::uint32_t>(site.patch_size) + 5u + relative;
  return target == site.address + site.patch_size;
}

void guard_refusal(const Sites& values) {
  auto wrong = values;
  wrong[1].original[wrong[1].guard_size - 1] ^= 1;
  Hooks refused(wrong);
  check(!refused.prepare(destinations()), "changed suffix guard rejects preparation");
  check(refused.original(0) == nullptr && refused.original(1) == nullptr &&
        refused.original(2) == nullptr, "guard refusal owns no trampolines");
  check(original_heads(values), "guard refusal changes no source instruction");
}

void partial_rollback(const Sites& values) {
  Hooks rollback(values);
  const bool prepared = rollback.prepare(destinations());
  check(prepared, "rollback fixture prepares real trampolines");
  if (!prepared) return;
  fail_write_at = reinterpret_cast<void*>(values[1].address);
  const auto before = injected_failures;
  check(!rollback.install(reinterpret_cast<std::uintptr_t>(&callback_one)),
        "second patch protection failure rejects installation");
  check(injected_failures == before + 1 && !fail_write_at, "failure injected exactly once");
  check(original_heads(values), "partial rollback restores first patch and all guards");
  check(rollback.original(0) == nullptr && rollback.original(1) == nullptr &&
        rollback.original(2) == nullptr, "complete rollback releases all trampolines");
  State state;
  const auto one = &synthetic_one;
  check(one(&state, 10) == 17 && state.calls == 1 && callback_calls[0] == 0,
        "restored original remains callable without callback");
}

void exercise(const Sites& values) {
  Hooks installed(values);
  const bool prepared = installed.prepare(destinations());
  check(prepared, "positive fixture prepares real trampolines");
  if (!prepared) return;
  for (std::size_t i = 0; i < values.size(); ++i)
    check(trampoline_valid(values[i], installed.original(i)), "trampoline bytes, target and RX protection");
  original_one = reinterpret_cast<One>(installed.original(0));
  original_two = reinterpret_cast<Two>(installed.original(1));
  original_update = reinterpret_cast<Update>(installed.original(2));
  State direct;
  check(original_one(&direct, 11) == 18 && direct.calls == 1 && direct.first == 11,
        "uninstalled one-argument trampoline executes original");
  check(original_two(&direct, -9, 17) == 15 && direct.calls == 2 &&
        direct.first == -9 && direct.second == 17, "uninstalled two-argument trampoline executes original");
  check(original_update(&direct, reinterpret_cast<void*>(0x2030)) == 0x2037 &&
        direct.calls == 3 && direct.first == 0x2030, "fastcall trampoline receives EDX");
  const bool active = installed.install(reinterpret_cast<std::uintptr_t>(&callback_one));
  check(active, "real image hooks installed");
  if (!active) return;

  const auto one = &synthetic_one;
  const auto two = &synthetic_two;
  const auto update = &synthetic_update;
  State state;
  bool values_correct = true;
  for (int i = 0; i < 256; ++i) {
    values_correct = (one(&state, i - 128) == i - 128 + 7 + 1000) && values_correct;
    values_correct = state.first == i - 128 && values_correct;
    values_correct = (two(&state, i, -2 * i) == 7 - i + 2000) && values_correct;
    values_correct = state.first == i && state.second == -2 * i && values_correct;
    values_correct = (update(&state, reinterpret_cast<void*>(0x2030)) == 0x2037 + 3000) && values_correct;
    values_correct = state.first == 0x2030 && values_correct;
  }
  check(values_correct, "768 calls preserve arguments and return values through hooks");
  check(state.calls == 768 && callback_calls == std::array<unsigned, 3>{{256, 256, 256}},
        "every intercepted call executes exactly one callback and one original");
  for (std::size_t i = 0; i < values.size(); ++i)
    check(trampoline_valid(values[i], installed.original(i)), "installed trampoline remains unchanged and RX");
  check(!installed.prepare(destinations()), "second preparation is refused");
  check(!installed.install(reinterpret_cast<std::uintptr_t>(&callback_one)), "second installation is refused");
  check(one(&state, 3) == 1010 && state.calls == 769 && callback_calls[0] == 257,
        "one-shot refusal keeps existing hook operational");
  // Successful hooks last for the lifetime of the process. Never free
  // them while entry points are active. The OS cleans up at process exit.
}
}  // namespace

BOOL WINAPI test_virtual_protect(LPVOID address, SIZE_T size, DWORD protection,
                                PDWORD previous) noexcept {
  if (address == fail_write_at && protection == PAGE_EXECUTE_READWRITE) {
    fail_write_at = nullptr;
    ++injected_failures;
    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  return ::VirtualProtect(address, size, protection, previous);
}

int main() {
  const auto values = sites();
  const bool correct_image = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr)) == 0x400000;
  check(correct_image, "test host has required fixed image base");
  if (!correct_image) return 2;
  bool readable = true;
  for (const auto& site : values) {
    const bool valid = ubc::windows_probe::readable(reinterpret_cast<const void*>(site.address),
                                                   site.guard_size, true, true);
    check(valid, "synthetic entry is executable MEM_IMAGE inside host range");
    readable = readable && valid;
  }
  if (!readable) return 3;
  check(original_heads(values), "compiler emitted exact complete stub instructions");
  if (failures) return 4;
  guard_refusal(values);
  partial_rollback(values);
  exercise(values);
  std::printf("NativeHooks: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
