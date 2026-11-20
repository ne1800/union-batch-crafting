// SPDX-License-Identifier: MIT
// Only our own Windows-x86 process and synthetic instructions.
#define UBC_DESTROY_REMOVE_HOST_TEST
#include <windows.h>
BOOL WINAPI host_virtual_protect(LPVOID,SIZE_T,DWORD,PDWORD);
BOOL WINAPI host_flush(HANDLE,LPCVOID,SIZE_T);
#define VirtualProtect host_virtual_protect
#define FlushInstructionCache host_flush
#include "ubc/engine/windows_destroy_remove_callsite.hpp"
#undef VirtualProtect
#undef FlushInstructionCache
#include <cstdio>
namespace w=ubc::windows_destroy_remove_callsite;
namespace c=ubc::destroy_remove_callsite;
unsigned checks=0,failures=0;
unsigned protect_calls=0,flush_calls=0,fail_protect_at=0,fail_flush_at=0;
BOOL WINAPI host_virtual_protect(LPVOID address,SIZE_T size,DWORD protect,PDWORD old) {
  if(++protect_calls==fail_protect_at){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
  return VirtualProtect(address,size,protect,old);
}
BOOL WINAPI host_flush(HANDLE process,LPCVOID address,SIZE_T size) {
  if(++flush_calls==fail_flush_at){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
  return FlushInstructionCache(process,address,size);
}
void check(bool value,const char* message){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL %s\n",message);}}
struct FakeNpc {std::uint8_t pad[0x668]{};std::uint32_t* table=nullptr;};
static_assert(offsetof(FakeNpc,table)==0x668);
std::uint32_t table[49]{},alternate[49]{};
FakeNpc npc{};
unsigned original_calls=0,alternate_calls=0,before_calls=0,after_calls=0;
void* seen_this=nullptr;
std::int32_t seen_instance=0,seen_amount=0;
DWORD seen_error=0;
std::uint32_t seen_return=0,outer_return=0,seen_result=0;
bool own_job=true,before_exception=false,after_exception=false,original_exception=false,replace_table=false;
void* returned=reinterpret_cast<void*>(0x12345678);
w::Adapter* test_adapter=nullptr;
bool attempt_busy_remove=false,busy_refused=false;

__declspec(noinline) void* __thiscall original(void* self,std::int32_t instance,std::int32_t amount) {
  ++original_calls;seen_this=self;seen_instance=instance;seen_amount=amount;
  seen_error=GetLastError();seen_return=reinterpret_cast<std::uint32_t>(_ReturnAddress());
  SetLastError(0x1234);
  if(original_exception)RaiseException(0xe0001234,0,0,nullptr);
  return returned;
}
__declspec(noinline) void* __thiscall alternate_original(void*,std::int32_t,std::int32_t) {
  ++alternate_calls;SetLastError(0x7654);return reinterpret_cast<void*>(0x87654321);
}

// Same calling context, otherwise entirely our own test code.
__declspec(naked) void* __fastcall synthetic_destroy(FakeNpc*,void*,std::int32_t) {
  __asm {
    push ebx
    push esi
    push edi
    mov edi,ecx
    mov esi,dword ptr [esp+16]
    xor ebx,ebx
    mov edx,dword ptr [edi+668h]
    lea ecx,[edi+668h]
    push 1
    push esi
    call dword ptr [edx+0c0h]
    _emit 0x8b
    _emit 0xf0
    _emit 0x3b
    _emit 0xf3
    mov eax,esi
    pop edi
    pop esi
    pop ebx
    ret 4
  }
}
std::uint32_t locate_site() {
  const auto start=reinterpret_cast<std::uint32_t>(&synthetic_destroy);
  std::uint32_t found=0;
  for(unsigned i=0;i<64;++i)if(!std::memcmp(reinterpret_cast<void*>(start+i),c::original_context.data(),25)) {
    if(found)return 0;found=start+i+15;
  }
  return found;
}
std::uint64_t __cdecl before(void*,const c::Invocation& input) {
  ++before_calls;outer_return=input.caller;
  check(input.inventory==reinterpret_cast<std::uint32_t>(&npc.table),"observer sees ECX inventory");
  if(replace_table)npc.table=alternate;
  if(attempt_busy_remove)busy_refused=!test_adapter->remove() && test_adapter->issue()==w::Issue::busy;
  SetLastError(0xdead);
  if(before_exception)RaiseException(0xe0002345,0,0,nullptr);
  return own_job?0x123456789abcdef0ull:0;
}
void __cdecl after(void*,const c::Invocation&,std::uint64_t token,std::uint32_t result) {
  ++after_calls;seen_result=result;
  check(token==0x123456789abcdef0ull,"full job token preserved");
  SetLastError(0xbeef);
  if(after_exception)RaiseException(0xe0003456,0,0,nullptr);
}
void* invoke(std::int32_t instance=16613) {
  std::uint32_t before_sp=0,after_sp=0;
  __asm mov before_sp,esp
  void* result=synthetic_destroy(&npc,nullptr,instance);
  __asm mov after_sp,esp
  check(before_sp==after_sp,"caller stack balance");
  return result;
}
void mutate(std::uint32_t address,std::uint8_t value) {
  DWORD old=0;check(VirtualProtect(reinterpret_cast<void*>(address),1,PAGE_EXECUTE_READWRITE,&old)!=FALSE,"fixture writable");
  *reinterpret_cast<std::uint8_t*>(address)=value;
  DWORD ignored=0;check(VirtualProtect(reinterpret_cast<void*>(address),1,old,&ignored)!=FALSE,"fixture protection restored");
  check(FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address),1)!=FALSE,"fixture cache flushed");
}
DWORD WINAPI worker(void*) {invoke();return 0;}
bool invoke_exception() {
  __try {invoke();return false;}
  __except(GetExceptionCode()==0xe0001234?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
}

int main() {
  table[48]=reinterpret_cast<std::uint32_t>(&original);
  alternate[48]=reinterpret_cast<std::uint32_t>(&alternate_original);npc.table=table;
  const auto site=locate_site();check(site!=0,"exact synthetic FF92 context located");if(!site)return 1;
  const w::Observer observer{nullptr,&before,&after};
  SetLastError(0xabc);check(invoke()==returned && original_calls==1,"unpatched baseline original exactly once");
  check(seen_return==site+6 && seen_error==0xabc && GetLastError()==0x1234,"baseline RA and LastError");
  {
    w::Adapter bad(site,reinterpret_cast<std::uint32_t>(table),reinterpret_cast<std::uint32_t>(&original));
    mutate(site+1,0x90);
    check(!bad.prepare(GetCurrentThreadId(),observer),"wrong FF90 rejected before patch");
    mutate(site+1,0x92);
    check(!bad.install() && !bad.retained(),"failed prepare cannot install");
  }
  {
    w::Adapter bad(site,reinterpret_cast<std::uint32_t>(table),reinterpret_cast<std::uint32_t>(&original));
    check(bad.prepare(GetCurrentThreadId(),observer),"prepare before race fixture");
    mutate(site+6,0x90);
    check(!bad.install() && !bad.retained(),"changed context after prepare rejected");
    mutate(site+6,c::original_context[21]);
  }
  for(unsigned failure=0;failure<3;++failure) {
    w::Adapter partial(site,reinterpret_cast<std::uint32_t>(table),reinterpret_cast<std::uint32_t>(&original));
    check(partial.prepare(GetCurrentThreadId(),observer),"prepare isolated failure injection");
    if(failure<2)fail_protect_at=protect_calls+1+failure;
    else fail_flush_at=flush_calls+1;
    SetLastError(0xabc);
    check(!partial.install() && GetLastError()==0xabc,"failed patch installation retains LastError");
    check(!partial.retained() && locate_site()==site,"failed install restores only its own patch");
    check(partial.issue()==(failure==2?w::Issue::flush:w::Issue::protect),"original install error retained after successful rollback");
    fail_protect_at=fail_flush_at=0;
  }
  w::Adapter adapter(site,reinterpret_cast<std::uint32_t>(table),reinterpret_cast<std::uint32_t>(&original));test_adapter=&adapter;
  SetLastError(0xabc);
  check(adapter.prepare(GetCurrentThreadId(),observer) && GetLastError()==0xabc,"prepare retains LastError");
  check(adapter.install() && GetLastError()==0xabc && adapter.owns_patch(),"install retains LastError and owns bytes");
  check(!adapter.install(),"no duplicate installation");
  const auto before_n=original_calls;
  SetLastError(0xabc);check(invoke()==returned,"EAX result unchanged");
  check(original_calls==before_n+1 && before_calls==1 && after_calls==1,"exactly one original and paired observers");
  check(seen_this==&npc.table && seen_instance==16613 && seen_amount==1,"thiscall ECX and both arguments unchanged");
  check(seen_error==0xabc && GetLastError()==0x1234,"observer LastError does not reach original or caller");
  check(outer_return==site+6 && seen_return!=site+6,"outer RA retained; original RA explicitly changed");
  check(seen_result==reinterpret_cast<std::uint32_t>(returned),"opague return copied without dereference");
  own_job=false;auto after_n=after_calls;SetLastError(0xabc);invoke();
  check(after_calls==after_n && original_calls==before_n+2,"unowned job still forwards once without after");own_job=true;
  returned=nullptr;invoke();check(seen_result==0,"null original result remains null");returned=reinterpret_cast<void*>(0x12345678);
  replace_table=true;auto alt_n=alternate_calls;SetLastError(0xabc);invoke();replace_table=false;
  check(alternate_calls==alt_n && seen_error==0xabc,"already loaded EDX target forwarded despite observer table change");npc.table=table;
  npc.table=alternate;auto obs_n=before_calls;SetLastError(0xabc);check(invoke()==reinterpret_cast<void*>(0x87654321),"foreign original is forwarded transparently");
  check(before_calls==obs_n && alternate_calls==alt_n+1 && GetLastError()==0x7654,"foreign vtable is never admitted");npc.table=table;
  obs_n=before_calls;invoke(-1);check(before_calls==obs_n && seen_instance==-1,"unexpected argument forwarded without observation");
  before_exception=true;after_n=after_calls;invoke();before_exception=false;
  check(after_calls==after_n && adapter.diagnostic().observer_faults==1,"before exception never suppresses original");
  after_exception=true;SetLastError(0xabc);invoke();after_exception=false;
  check(GetLastError()==0x1234 && adapter.diagnostic().observer_faults==2,"after exception preserves original LastError");
  original_exception=true;after_n=after_calls;check(invoke_exception(),"original SEH propagates unchanged");original_exception=false;
  check(after_calls==after_n && !adapter.in_flight(),"no after on original unwind and active count released");
  attempt_busy_remove=true;invoke();attempt_busy_remove=false;check(busy_refused && adapter.owns_patch(),"removal while active refused");
  obs_n=before_calls;HANDLE thread=CreateThread(nullptr,0,&worker,nullptr,0,nullptr);
  check(thread && WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"foreign thread invocation completed");if(thread)CloseHandle(thread);
  check(before_calls==obs_n,"foreign thread forwards without observation");
  const auto patch=adapter.patch_plan();mutate(site+2,static_cast<std::uint8_t>(patch.bytes[2]^1));
  check(!adapter.remove() && adapter.retained() && !adapter.owns_patch(),"foreign patch bytes are never restored");mutate(site+2,patch.bytes[2]);
  mutate(site+6,0x90);check(!adapter.remove() && adapter.retained(),"foreign suffix blocks restoration");mutate(site+6,c::original_context[21]);
  SetLastError(0xabc);check(adapter.remove() && GetLastError()==0xabc && !adapter.retained(),"owned patch restored with LastError");
  check(locate_site()==site,"exact original instruction context restored");
  obs_n=before_calls;invoke();check(before_calls==obs_n && seen_return==site+6,"unpatched original RA restored");
  std::printf("windows_destroy_remove_callsite_tests: %u checks, %u failures\n",checks,failures);
  return failures?1:0;
}
