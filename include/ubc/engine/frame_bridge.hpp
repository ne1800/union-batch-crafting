// SPDX-License-Identifier: MIT
#pragma once

#if defined(_WIN32) && defined(_M_IX86)
// Only for the verified post-render hook site: EDI is gameSession
// and EBP is CGameManager. No additional render call or
// change to its original return address or the LeGo prologues.
extern "C" {
using UbcFrameCallback = void(__fastcall*)(void* session, void* manager);
extern void* UbcFrameContinuation;
extern UbcFrameCallback UbcAfterFrame;
void UbcFrameBridge();
}
#endif
