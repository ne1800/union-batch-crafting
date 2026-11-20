// SPDX-License-Identifier: MIT
#include <windows.h>
#include "ubc/frame_bridge.hpp"
#include "ubc/windows_probe.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
unsigned checks = 0, failures = 0, callbacks = 0;
void check(bool ok, const char* label) {
  ++checks;
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", label); }
}
struct State { void** vtable; unsigned blits; unsigned observed; DWORD error; };

__declspec(naked) void __thiscall blit(void*) {
  __asm {
    inc dword ptr [ecx + 4]
    mov eax, dword ptr [ecx + 8]
    inc eax
    mov dword ptr [ecx + 8], eax
    ret
  }
}

// Our own test program makes an object-bound virtual call via EDI, followed
// by code that uses registers and the stack. No copied engine function.
__declspec(naked) unsigned __cdecl loop(State*) {
  __asm {
    push edi
    mov edi, dword ptr [esp + 8]
    mov eax, dword ptr [edi]
    mov ecx, edi
    call dword ptr [eax + 14h]
    mov eax, dword ptr [edi + 4]
    pop edi
    ret
  }
}

void __fastcall after_frame(void* session, void*) {
  const auto saved = GetLastError();
  auto* state = static_cast<State*>(session);
  ++callbacks;
  state->error = saved;
  state->observed += 10;
  // The probe also includes actual Windows calls that modify registers.
  SetLastError(987);
  __asm { pxor xmm0, xmm0 }
  SetLastError(saved);
}

struct Site {
  std::uintptr_t address;
  std::size_t patch_size, guard_size;
  std::array<std::uint8_t, 32> original;
};
}

int main() {
  std::array<void*, 6> vtable{};
  vtable[5] = reinterpret_cast<void*>(&blit);
  State state{vtable.data(), 0, 0, 0};
  check(loop(&state) == 1 && state.observed == 1 && callbacks == 0, "original loop calls blit once");
  // Clang emits 89 F9 for MOV ECX,EDI. The runtime hook site has the semantically
  // equivalent opposite-direction encoding 8B CF. These are our own test bytes.
  const auto address = reinterpret_cast<std::uintptr_t>(&loop) + 5;
  const std::array<Site, 1> sites{{{address, 7, 10,
      {{0x8b, 0x07, 0x89, 0xf9, 0xff, 0x50, 0x14, 0x8b, 0x47, 0x04}}}}};
  ubc::windows_probe::Hooks<Site, 1> hooks(sites);
  const bool prepared = hooks.prepare({{reinterpret_cast<std::uintptr_t>(&UbcFrameBridge)}});
  check(prepared, "prepare complete mid-function instruction span");
  if (!prepared) return 1;
  UbcFrameContinuation = hooks.original(0);
  UbcAfterFrame = after_frame;
  check(hooks.install(reinterpret_cast<std::uintptr_t>(&UbcFrameBridge)), "install own-process frame bridge");
  for (unsigned i = 0; i < 100; ++i) {
    SetLastError(1234 + i);
    const auto result = loop(&state);
    const auto error = GetLastError();
    check(result == i + 2 && state.blits == i + 2, "original indirect blit and continuation exactly once");
    check(callbacks == i + 1 && state.observed == 1 + 11 * (i + 1), "observer once before original blit");
    check(state.error == 1234 + i && error == 1234 + i, "last error preserved across observer and continuation");
  }
  // The callback deliberately clobbers XMM0. The gate between instructions
  // must preserve it anyway. Normal C++ callee rules are insufficient.
  const unsigned pattern=0x12345678;unsigned preserved=0;
  State* object=&state;
  __asm {
    movd xmm0, pattern
    push object
    call loop
    add esp, 4
    movd preserved, xmm0
  }
  check(preserved==pattern,"SIMD state preserved across arbitrary callback work");
  std::printf("FrameBridge: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
