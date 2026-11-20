// SPDX-License-Identifier: MIT
#include "ubc/frame_bridge.hpp"

#if !defined(_M_IX86)
#error Der Nach-Render-Adapter benoetigt Windows x86.
#endif

extern "C" {
void* UbcFrameContinuation = nullptr;
UbcFrameCallback UbcAfterFrame = nullptr;

// Set the callback before installation and leave it unchanged afterward. It
// must preserve LastError. Restore integer registers, EFLAGS, x87/MMX/XMM,
// MXCSR and the stack here. The trampoline then executes the original
// virtual RenderBlit call and jumps to the unchanged continuation.
__declspec(naked) void UbcFrameBridge() {
  __asm {
    pushfd
    pushad
    mov ebx, esp
    sub esp, 528
    and esp, -16
    fxsave [esp]
    mov ecx, edi
    mov edx, ebp
    call dword ptr [UbcAfterFrame]
    fxrstor [esp]
    mov esp, ebx
    popad
    popfd
    jmp dword ptr [UbcFrameContinuation]
  }
}
}
