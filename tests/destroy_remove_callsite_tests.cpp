// SPDX-License-Identifier: MIT
#include "ubc/destroy_remove_callsite.hpp"
#include <cstdio>
namespace c=ubc::destroy_remove_callsite;
unsigned checks=0,failures=0;
void check(bool value,const char* message) {++checks;if(!value){++failures;std::fprintf(stderr,"FAIL %s\n",message);}}
int main() {
  const auto p=c::plan(c::address,0x10002000);
  check(p.valid && p.bytes[0]==0x90 && p.bytes[1]==0xe8,"NOP CALL keeps six-byte outer return");
  for(const auto destination:{0x1000u,0x10002000u,0xf1000000u,0xffffffffu}) {
    const auto value=c::plan(c::address,destination);
    const auto d=std::uint32_t{value.bytes[2]}|(std::uint32_t{value.bytes[3]}<<8)|
      (std::uint32_t{value.bytes[4]}<<16)|(std::uint32_t{value.bytes[5]}<<24);
    check(value.valid && static_cast<std::uint32_t>(value.call+6+d)==destination,"x86 relative target including wrap");
  }
  for(const auto address:{0u,14u,0xfffffff6u,0xfffffffau,0xffffffffu})
    check(!c::plan(address,0x1000).valid,"invalid or crossing callsite refused");
  for(const auto dest:{0u,c::address-15,c::address,c::address+5,c::address+9})
    check(!c::plan(c::address,dest).valid,"null or context-overlapping observer refused");
  auto bytes=c::original_context;
  check(c::context_matches(bytes,p,false) && !c::context_matches(bytes,p,true),"original is not owned patch");
  for(unsigned i=0;i<bytes.size();++i) {
    auto altered=bytes;altered[i]^=1;
    check(!c::context_matches(altered,p,false),"every unknown original byte refused");
  }
  for(unsigned i=0;i<6;++i)bytes[15+i]=p.bytes[i];
  check(c::context_matches(bytes,p,true) && !c::context_matches(bytes,p,false),"exact owned bytes");
  for(unsigned i=0;i<bytes.size();++i) {
    auto altered=bytes;altered[i]^=1;
    check(!c::context_matches(altered,p,true),"every foreign patch or neighbor mutation refused");
  }
  auto forged=p;forged.bytes[2]^=1;
  check(!c::context_matches(bytes,forged,true),"corrupt plan cannot bless bytes");
  c::Plan forged_invalid;forged_invalid.valid=true;
  check(!c::context_matches(c::original_context,forged_invalid,false),"forged valid flag cannot bless invalid plan");
  auto wrong_opcode=c::original_context;wrong_opcode[16]=0x90;
  check(!c::context_matches(wrong_opcode,p,false),"FF90 EAX is not FF92 EDX");
  const c::Admission admission{c::return_address,c::vtable,c::target,123};
  const c::Invocation invocation{0x100000,c::vtable,c::target,c::return_address,123,16613,1};
  check(c::observable(invocation,admission),"exact call evidence");
  for(unsigned i=0;i<9;++i) {
    auto bad=invocation;
    switch(i){case 0:bad.inventory=0;break;case 1:bad.inventory+=1;break;case 2:++bad.loaded_vtable;break;
      case 3:++bad.original;break;case 4:--bad.caller;break;case 5:++bad.thread;break;
      case 6:bad.instance=-1;break;case 7:bad.amount=0;break;case 8:bad.amount=2;break;}
    check(!c::observable(bad,admission),"unknown call gets no observation admission");
  }
  auto no_thread=admission;no_thread.thread=0;
  check(!c::observable(invocation,no_thread) && !c::native_effect_authorized,"no thread and no effect authority");
  auto bad_admission=admission;bad_admission.original=0;
  auto zero_target=invocation;zero_target.original=0;
  check(!c::observable(zero_target,bad_admission),"null expected target cannot bless observation");
  std::printf("destroy_remove_callsite_tests: %u checks, %u failures\n",checks,failures);
  return failures?1:0;
}
