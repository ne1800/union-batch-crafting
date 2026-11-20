// SPDX-License-Identifier: MIT
#include <windows.h>
#include "ammo_presentation_bridge.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace b = ubc::ammo_presentation_bridge;
namespace p = ubc::ammo_presentation;
namespace {
unsigned checks=0,failures=0;
void check(bool value,const char* description) {
  ++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",description);}
}
constexpr DWORD host_exception=0xe042b410;
constexpr DWORD input_error=0x12345678,output_error=0x23456789;
constexpr std::uint32_t hero=0x3000,message=0x4000,event_manager=0x5000;
volatile LONG observer_calls[3]{},original_calls[3]{};
int observer_fault=-1,original_fault=-1;
bool own_print=false,invalid_decision=false,reenter=false,receipt_mode=false,immediate=false;
DWORD seen_error=0,callback_flags=0;
b::CaptureSite captured{};b::PrintSite printed{};b::DestructorSite destroyed{};
p::Tracker tracker;
p::Context context{1,0,ubc::lhiver::game_parser,0x1000,0x2000,hero};
p::Job job{};p::Scope scope{};
p::Payload payload{true,p::conversation_vtable,21,p::templates[0].text,p::font,-1,30,0x40000000};

void invoke(unsigned kind);
void damage(unsigned kind) {
  InterlockedIncrement(&observer_calls[kind]);SetLastError(7000+kind);
  unsigned flags=0;
  __asm {
    pushfd
    pop flags
  }
  callback_flags=flags;
  unsigned changed_mxcsr=0x5f80;
  __asm {
    fninit
    fldz
    pxor xmm0,xmm0
    pxor xmm7,xmm7
    ldmxcsr changed_mxcsr
  }
  if(static_cast<int>(kind)==observer_fault)RaiseException(host_exception,0,0,nullptr);
}
void __cdecl observe_capture(void*,const b::CaptureSite& site) {
  captured=site;damage(0);
  if(receipt_mode) {
    check(site.parser==context.parser&&site.message==message&&site.event_manager==event_manager,
      "native capture maps EDI parser, ESI message, EAX manager");
    p::Capture value{context,scope,site.message,site.event_manager,hero,p::templates[0].success_pc,
      p::external_dispatch_return,payload,true,true};value.context.thread=site.thread;
    check(tracker.capture(value),"capture callback creates own receipt before original queue");
  }
}
p::DisplayDecision __cdecl observe_print(void*,const b::PrintSite& site) {
  printed=site;damage(1);
  if(reenter)invoke(2);
  if(invalid_decision)return static_cast<p::DisplayDecision>(99);
  if(receipt_mode) {
    p::Display value{context,site.message,site.receiver,payload,true};value.context.thread=site.thread;
    return tracker.display(value);
  }
  return own_print?p::DisplayDecision::suppress_owned:p::DisplayDecision::original;
}
void __cdecl observe_destructor(void*,const b::DestructorSite& site) {
  destroyed=site;damage(2);
  if(receipt_mode) {
    p::Destruction value{context,site.message,payload,true};value.context.thread=site.thread;
    check(tracker.destroy(value),"destructor callback retires exactly captured message");
  }
}
}

extern "C" {
alignas(16) unsigned char caller_fx[512]{},expected_fx[512]{},observed_fx[512]{},returned_fx[512]{};
alignas(16) unsigned char xmm_pattern[16]{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
unsigned observed_gpr[8]{},returned_gpr[8]{};
unsigned expected_stack=0,observed_flags=0,expected_flags=0,returned_flags=0;
unsigned expected_seh=0,observed_seh=0,returned_seh=0,expected_caller=0,observed_caller=0;
unsigned kind_to_invoke=0;
void* bridge_to_invoke=nullptr;

void __cdecl original_body(unsigned kind) {
  seen_error=GetLastError();InterlockedIncrement(&original_calls[kind]);
  if(static_cast<int>(kind)==original_fault)RaiseException(host_exception,0,0,nullptr);
  if(receipt_mode&&kind==0&&immediate) { invoke(1);invoke(2); }
  SetLastError(output_error);
}
// Our own native test bodies, with no copied EXE bytes. They capture
// state at the tail transfer itself, before C++ may change it.
#define CAPTURE_NATIVE_STATE \
  __asm { mov dword ptr [observed_gpr],eax } \
  __asm { mov dword ptr [observed_gpr+4],ecx } \
  __asm { mov dword ptr [observed_gpr+8],edx } \
  __asm { mov dword ptr [observed_gpr+12],ebx } \
  __asm { mov dword ptr [observed_gpr+16],esp } \
  __asm { mov dword ptr [observed_gpr+20],ebp } \
  __asm { mov dword ptr [observed_gpr+24],esi } \
  __asm { mov dword ptr [observed_gpr+28],edi } \
  __asm { pushfd } \
  __asm { pop dword ptr [observed_flags] } \
  __asm { fxsave [observed_fx] } \
  __asm { mov eax,fs:[0] } \
  __asm { mov dword ptr [observed_seh],eax } \
  __asm { mov eax,dword ptr [esp] } \
  __asm { mov dword ptr [observed_caller],eax } \
  __asm { cld }
__declspec(naked) void original_capture() {
  CAPTURE_NATIVE_STATE
  __asm {
    push 0
    call original_body
    add esp,4
    mov eax,1234h
    ret
  }
}
__declspec(naked) int __thiscall original_print(void*,void*) {
  CAPTURE_NATIVE_STATE
  __asm {
    push 1
    call original_body
    add esp,4
    mov eax,5678h
    ret 4
  }
}
__declspec(naked) void __thiscall original_destructor(void*) {
  CAPTURE_NATIVE_STATE
  __asm {
    push 2
    call original_body
    add esp,4
    mov eax,9abch
    ret
  }
}
#undef CAPTURE_NATIVE_STATE

__declspec(naked) void probe_registers() {
  __asm {
    pushfd
    pushad
    fxsave [caller_fx]
    fninit
    fld1
    fldpi
    movdqu xmm0,xmmword ptr [xmm_pattern]
    movdqu xmm7,xmmword ptr [xmm_pattern]
    fxsave [expected_fx]
    mov eax,fs:[0]
    mov dword ptr [expected_seh],eax
    cmp dword ptr [kind_to_invoke],1
    jne no_message_argument
    push 4000h
  no_message_argument:
    lea eax,[esp-4]
    mov dword ptr [expected_stack],eax
    lea eax,bridge_return
    mov dword ptr [expected_caller],eax
    push 647h
    popfd
    pushfd
    pop dword ptr [expected_flags]
    mov eax,5000h
    mov ecx,3000h
    mov edx,33333333h
    mov ebx,44444444h
    mov ebp,77777777h
    mov esi,4000h
    mov edi,0ab40c0h
    call dword ptr [bridge_to_invoke]
  bridge_return:
    mov dword ptr [returned_gpr],eax
    mov dword ptr [returned_gpr+4],ecx
    mov dword ptr [returned_gpr+8],edx
    mov dword ptr [returned_gpr+12],ebx
    mov dword ptr [returned_gpr+16],esp
    mov dword ptr [returned_gpr+20],ebp
    mov dword ptr [returned_gpr+24],esi
    mov dword ptr [returned_gpr+28],edi
    pushfd
    pop dword ptr [returned_flags]
    fxsave [returned_fx]
    mov eax,fs:[0]
    mov dword ptr [returned_seh],eax
    fxrstor [caller_fx]
    popad
    popfd
    ret
  }
}
}

namespace {
void invoke(unsigned kind) {
  if(kind==0)UbcAmmoCaptureBridge();
  else if(kind==1)UbcAmmoPrintBridge(reinterpret_cast<void*>(hero),reinterpret_cast<void*>(message));
  else UbcAmmoDestructorBridge(reinterpret_cast<void*>(message));
}
bool raises(unsigned kind) {
  __try { invoke(kind); }
  __except(GetExceptionCode()==host_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) { return true; }
  return false;
}
void probe(unsigned kind) {
  kind_to_invoke=kind;
  bridge_to_invoke=kind==0?reinterpret_cast<void*>(&UbcAmmoCaptureBridge):
    kind==1?reinterpret_cast<void*>(&UbcAmmoPrintBridge):reinterpret_cast<void*>(&UbcAmmoDestructorBridge);
  SetLastError(input_error);probe_registers();
}
DWORD WINAPI foreign_entry(void*) { invoke(1);return 0; }
bool configure() {
  b::Config value{GetCurrentThreadId(),nullptr,reinterpret_cast<void*>(&original_capture),
    &original_print,&original_destructor,&observe_capture,&observe_print,&observe_destructor};
  auto bad=value;bad.capture=nullptr;
  check(!b::configure(bad),"missing observer rejected before configuration");
  check(b::configure(value),"all immutable native continuations/observers configured once");
  check(!b::configure(value),"configured bridge cannot be replaced or reset around a pending receipt");
  context.thread=GetCurrentThreadId();
  return b::diagnostics().configured;
}
void run_normal() {
  const std::array<unsigned,8> expected{{0x5000,0x3000,0x33333333,0x44444444,0,0x77777777,0x4000,0xab40c0}};
  for(unsigned kind=0;kind<3;++kind) {
    probe(kind);
    check(original_calls[kind]==1&&observer_calls[kind]==1,"ordinary original and observer each execute exactly once");
    for(std::size_t i=0;i<expected.size();++i)check(observed_gpr[i]==(i==4?expected_stack:expected[i]),"original sees every incoming register unchanged");
    check(observed_flags==expected_flags,"native original sees exact incoming EFLAGS including DF");
    check((callback_flags&0x400)==0,"C++ observer always has DF clear");
    check(std::memcmp(expected_fx,observed_fx,512)==0,"native original sees identical x87/XMM/MXCSR image");
    check(observed_caller==expected_caller&&observed_seh==expected_seh,"tail transfer preserves real caller address and SEH chain");
    check(returned_seh==expected_seh&&returned_gpr[4]==expected_stack+(kind==1?8u:4u),"original return has correct thiscall/cdecl cleanup and SEH");
    check(seen_error==input_error&&GetLastError()==output_error,"observer preserves input LastError and original output LastError");
    check(!b::diagnostics().disabled&&!b::diagnostics().observer_active,"normal observer lease is balanced");
    if(kind==0)check(captured.stack==expected_stack,"capture exposes the exact unmodified entry ESP for bounded caller reads");
    else if(kind==1)check(printed.caller==expected_caller,"print observer sees actual native caller");
    else check(destroyed.caller==expected_caller,"destructor observer sees actual native caller");
  }
  check(captured.parser==0xab40c0&&captured.message==message&&captured.event_manager==event_manager&&captured.stack,
    "capture exposes only its exact native fields");
  check(printed.receiver==hero&&printed.message==message&&printed.caller,"print exposes ECX receiver plus original stack message");
  check(destroyed.message==hero&&destroyed.caller,"destructor exposes native ECX object and actual return address");
  own_print=true;const auto originals=original_calls[1];probe(1);
  check(original_calls[1]==originals&&returned_gpr[0]==1,"positive own decision skips only print body and returns original success status one");
  for(std::size_t i=1;i<expected.size();++i)check(returned_gpr[i]==(i==4?expected_stack+8:expected[i]),"suppression preserves registers and exact callee stack cleanup");
  check(returned_flags==expected_flags&&std::memcmp(expected_fx,returned_fx,512)==0,"suppression preserves EFLAGS and complete floating-point state");
  check(GetLastError()==input_error&&returned_seh==expected_seh,"suppression preserves LastError and SEH without an original frame");
  own_print=false;
  for(unsigned kind=0;kind<3;++kind) {
    original_fault=static_cast<int>(kind);
    SetLastError(input_error);
    check(raises(kind),"original native SEH propagates unchanged to caller");
    check(seen_error==input_error,"original exception path receives unchanged LastError");
    check(!b::diagnostics().disabled&&!b::diagnostics().observer_active,"original exception is not caught as observer failure");
  }
  original_fault=-1;
}
void run_receipt() {
  receipt_mode=true;
  for(bool dispatch_immediately:{false,true}) {
    immediate=dispatch_immediately;
    job={context,immediate?2u:1u,immediate?12u:11u,p::templates[0].callback,p::templates[0].owner,3};
    scope={job.id,job.scope_token,job.callback,1,true,true};
    check(tracker.arm(job,{true,true,true,true,true}),"independent own result scope arms");
    const auto previous_queue=original_calls[0],previous_print=original_calls[1],previous_destroy=original_calls[2];
    probe(0); // Original capture continuation is our unchanged synthetic queue.
    check(original_calls[0]==previous_queue+1,"capture always preserves original enqueue/continuation");
    check(tracker.close_effect(context,scope,!b::diagnostics().disabled),"own effect closes after original queue path");
    if(!immediate){invoke(1);invoke(2);}
    check(original_calls[1]==previous_print&&original_calls[2]==previous_destroy+1,"only exact own display suppressed, original destructor still runs once");
    check(!tracker.take_notice(context,true).valid,"native suppression does not invent recipe result");
    const p::Result result{context,job.id,job.callback,job.lots,p::templates[0].output,90,p::ResultState::matched_ready,!b::diagnostics().disabled};
    check(tracker.confirm_result(result),"separate synthetic Ready witness confirms exact output");
    check(tracker.take_notice(context,true).valid,"correct own notice becomes available after result only");
    check(tracker.status().stage==p::Stage::destroyed,"no UI stub completes registration");
    check(tracker.acknowledge_notice(context,job.id,100+job.id,true,!b::diagnostics().disabled),"test supplies explicit synthetic UI receipt");
    check(tracker.status().stage==p::Stage::idle,"retired message and separate result/UI evidence complete scope");
    invoke(1);check(original_calls[1]==previous_print+1,"reused message pointer outside receipt remains ordinary");
  }
}
}

int main(int argc,char** argv) {
  static_assert(sizeof(void*)==4,"tests must execute x86 code");
  if(!configure())return 1;
  const char* mode=argc>1?argv[1]:"normal";
  if(std::strcmp(mode,"normal")==0)run_normal();
  else if(std::strcmp(mode,"receipt")==0)run_receipt();
  else if(std::strncmp(mode,"fault-",6)==0) {
    observer_fault=std::atoi(mode+6);check(observer_fault>=0&&observer_fault<3,"bounded observer fault scenario");
    if(observer_fault>=0&&observer_fault<3) {
      own_print=true;SetLastError(input_error);invoke(static_cast<unsigned>(observer_fault));
      check(original_calls[observer_fault]==1,"observer SEH fault still tailpasses exact original");
      check(seen_error==input_error&&GetLastError()==output_error,"observer exception cannot leak LastError into original or caller");
      check(b::diagnostics().disabled&&b::diagnostics().fault==b::Fault::observer_exception&&!b::diagnostics().observer_active,
        "observer SEH disables future suppression and releases own observer frame");
      const auto before=observer_calls[1];invoke(1);
      check(observer_calls[1]==before&&original_calls[1]>=1,"disabled bridge never issues another suppress decision");
    }
  } else if(std::strcmp(mode,"foreign")==0) {
    own_print=true;HANDLE thread=CreateThread(nullptr,0,foreign_entry,nullptr,0,nullptr);
    check(thread!=nullptr,"create isolated foreign host thread");
    if(thread){check(WaitForSingleObject(thread,10000)==WAIT_OBJECT_0,"foreign call finishes without being blocked");CloseHandle(thread);}
    check(original_calls[1]==1&&!observer_calls[1],"foreign native print remains original without touching owner TLS");
    check(b::diagnostics().disabled&&b::diagnostics().fault==b::Fault::foreign_thread,"foreign lifetime boundary disables own presentation");
  } else if(std::strcmp(mode,"reentry")==0) {
    own_print=true;reenter=true;invoke(1);
    check(original_calls[1]==1&&original_calls[2]==1&&!observer_calls[2],"nested original stays intact and outer stale suppression is discarded");
    check(b::diagnostics().disabled&&b::diagnostics().fault==b::Fault::reentry&&!b::diagnostics().observer_active,"observer reentry permanently quarantines own presentation");
  } else if(std::strcmp(mode,"decision")==0) {
    invalid_decision=true;invoke(1);
    check(original_calls[1]==1&&b::diagnostics().disabled&&b::diagnostics().fault==b::Fault::invalid_decision,
      "invalid observer enum never becomes a permissive Boolean suppression");
  } else if(std::strcmp(mode,"disable")==0) {
    own_print=true;b::disable();invoke(1);
    check(original_calls[1]==1&&!observer_calls[1]&&b::diagnostics().fault==b::Fault::disabled,"explicit native guard loss keeps original and disables suppression");
  } else check(false,"known host scenario");
  std::printf("ammo_presentation_bridge_tests(%s): %u checks, %u failures\n",mode,checks,failures);
  return failures?1:0;
}
