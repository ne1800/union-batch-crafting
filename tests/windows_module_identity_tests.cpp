// SPDX-License-Identifier: MIT
#include "ubc/windows_module_identity.hpp"
#include <cstdio>

namespace {
unsigned checks=0,failures=0;
void check(bool value,const char* label){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",label);}}
DWORD WINAPI foreign(void* context){return static_cast<ubc::windows_probe::LoadedModuleIdentity*>(context)->current()?1:0;}
}
int main(){
  static std::array<wchar_t,32768> path{};
  const auto module=GetModuleHandleW(nullptr);
  const auto count=GetModuleFileNameW(module,path.data(),static_cast<DWORD>(path.size()));
  if(!count||count>=path.size())return 2;
  const wchar_t* name=path.data();for(const auto* p=name;*p;++p)if(*p==L'\\'||*p==L'/')name=p+1;
  auto file=CreateFileW(path.data(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
  if(file==INVALID_HANDLE_VALUE)return 2;
  ubc::Sha256 hash;std::array<std::uint8_t,8192> bytes{};
  for(;;){DWORD n=0;if(!ReadFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&n,nullptr)){CloseHandle(file);return 2;}
    if(!n)break;hash.update(bytes.data(),n);}
  CloseHandle(file);const auto digest=hash.finish();
  const auto* base=reinterpret_cast<const std::uint8_t*>(module);
  const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
  const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
  const auto size=nt->OptionalHeader.SizeOfImage;
  static ubc::windows_probe::LoadedModuleIdentity good,wrong,missing,wrong_size;
  check(!good.current(),"unacquired module never current");
  auto bad=digest;bad[0]^=1;
  check(!wrong.acquire(name,size,bad)&&!wrong.current(),"whole-file digest mismatch rejects");
  check(!wrong_size.acquire(name,size+4096,digest),"loaded image extent must match");
  check(!missing.acquire(L"UBC_module_that_is_absent.dll",size,digest),"missing module does not get loaded");
  check(good.acquire(name,size,digest)&&good.current(),"existing exact loaded image is pinned and readable");
  check(good.base()==reinterpret_cast<std::uintptr_t>(module)&&good.image_size()==size&&good.digest()==digest,"binding describes actual loaded module");
  check(!good.acquire(name,size,digest)&&good.current(),"repeated enrollment is refused without losing the pinned image");
  const auto thread=CreateThread(nullptr,0,foreign,&good,0,nullptr);check(thread!=nullptr,"foreign-thread probe started");
  if(thread){const auto wait=WaitForSingleObject(thread,5000);DWORD code=1;
    check(wait==WAIT_OBJECT_0&&GetExitCodeThread(thread,&code)&&code==0,"foreign thread cannot use mutable path scratch or binding");CloseHandle(thread);}
  check(good.current(),"owner binding remains usable after foreign attempt");
  const auto write=CreateFileW(path.data(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
  check(write==INVALID_HANDLE_VALUE,"pinned file cannot be opened for writes");if(write!=INVALID_HANDLE_VALUE)CloseHandle(write);
  std::printf("ModuleIdentity: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
