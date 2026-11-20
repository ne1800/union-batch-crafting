// SPDX-License-Identifier: MIT
#include "ubc/stations/cool/cool_native_path.hpp"
#include <algorithm>
#include <cstdio>
#include <functional>
#include <vector>
namespace {
namespace p=ubc::cool_native_path;
namespace a=ubc::cool_destroy_abi;
namespace c=ubc::destroy_remove_callsite;
unsigned checks=0,failures=0;
void check(bool ok,const char* name){++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",name);}}
struct Fixture {
  std::array<std::uint8_t,460> body{};
  std::array<std::uint8_t,11> trampoline{};
  p::OwnedPatches own{true,true,0x10001000,0x10002000,c::plan(c::address,0x10003000)};
  ubc::cool_destroy_abi::CodeRange area{a::destroy_entry,460,{}};
  std::uint32_t denied=0;
  bool transient_entry=false,transient_call=false,changes=false;
  unsigned whole_reads=0;
  Fixture(){
    body.fill(0xcc);
    std::copy(p::destroy_prefix.begin(),p::destroy_prefix.end(),body.begin());
    std::copy(c::original_context.begin(),c::original_context.end(),body.begin()+c::address-a::destroy_entry-c::prefix_size);
    area.sha256=p::detail::digest(body.data(),body.size());
    install();
  }
  void install(){
    std::copy(p::destroy_prefix.begin(),p::destroy_prefix.end(),trampoline.begin());
    p::detail::jump(trampoline.data()+6,own.trampoline+6,a::destroy_entry+6);
    std::fill_n(body.begin(),6,0x90);p::detail::jump(body.data(),a::destroy_entry,own.destination);
    std::copy(own.remove.bytes.begin(),own.remove.bytes.end(),body.begin()+c::address-a::destroy_entry);
  }
  void original(){
    std::copy(p::destroy_prefix.begin(),p::destroy_prefix.end(),body.begin());
    std::copy(c::original_context.begin()+c::prefix_size,c::original_context.begin()+c::prefix_size+c::patch_size,
      body.begin()+c::address-a::destroy_entry);own={};
  }
  static bool read(void* raw,std::uint32_t address,void* out,std::size_t size) noexcept {
    auto& f=*static_cast<Fixture*>(raw);
    if(f.denied&&p::detail::overlap(address,size,f.denied,1))return false;
    const auto copy=[&](std::uint32_t base,const auto& data) noexcept {
      if(address<base||std::uint64_t{address}-base+size>data.size())return false;
      std::memcpy(out,data.data()+address-base,size);return true;
    };
    bool ok=copy(a::destroy_entry,f.body)||copy(0x10002000,f.trampoline);
    if(ok&&address==a::destroy_entry&&size==460){
      ++f.whole_reads;
      auto* b=static_cast<std::uint8_t*>(out);
      if(f.transient_entry)b[1]^=1;
      if(f.transient_call)b[c::address-a::destroy_entry+2]^=1;
      if(f.changes&&f.whole_reads%2)b[130]^=1;
    }
    return ok;
  }
  ubc::ReadOnlyMemory memory(){return {this,read};}
  p::Verification verify(){return p::detail::code(memory(),area,own);}
};
void tests(){
  Fixture f;check(f.verify().valid,"exact owned entry and call restore canonical digest in value copy only");
  check(p::installed_hooks_current(f.memory(),f.own),"both owned hooks are available to the writer");
  f.original();check(f.verify().valid,"unpatched original accepted before installation");
  check(!p::installed_hooks_current(f.memory(),f.own),"original bytes without owned hooks cannot guard a writer");
  check(!p::verify(f.memory()).valid,"synthetic body cannot satisfy production identities");
  for(unsigned i=0;i<460;++i){Fixture x;x.body[i]^=1;check(!x.verify().valid,"every changed body byte rejected");}
  for(unsigned i=0;i<11;++i){Fixture x;x.trampoline[i]^=1;check(!x.verify().valid,"every changed trampoline byte rejected");}
  for(unsigned i=0;i<23;++i){Fixture x;switch(i){
    case 0:x.own.destroy_installed=false;break;
    case 1:x.own.remove_installed=false;break;
    case 2:x.own.destination=0;break;
    case 3:x.own.destination=a::destroy_entry;break;
    case 4:x.own.destination=x.own.trampoline;break;
    case 5:x.own.trampoline=0;break;
    case 6:x.own.trampoline=UINT32_MAX-3;break;
    case 7:x.own.trampoline=a::destroy_entry;break;
    case 8:x.own.remove.call++;break;
    case 9:x.own.remove.valid=false;break;
    case 10:x.own.remove.destination=0;break;
    case 11:x.own.remove.destination=x.own.destination;break;
    case 12:x.own.remove.destination=x.own.trampoline;break;
    case 13:x.own.remove.bytes[2]^=1;break;
    case 14:x.area.length=2049;break;
    case 15:x.area.length=459;break;
    case 16:x.area.address=UINT32_MAX;break;
    case 17:x.denied=a::destroy_entry;break;
    case 18:x.denied=x.own.trampoline;break;
    case 19:x.transient_entry=true;break;
    case 20:x.transient_call=true;break;
    case 21:x.changes=true;break;
    case 22:x.area.sha256[0]^=1;break;
  }check(!x.verify().valid,"missing ownership, changed snapshots, bad hash or inaccessible memory rejected");}
  Fixture missing;missing.own={};check(!missing.verify().valid,"patch bytes without own registry never normalized");
  Fixture entry_only;entry_only.own.remove_installed=false;entry_only.own.remove={};
  std::copy(c::original_context.begin()+c::prefix_size,c::original_context.begin()+c::prefix_size+c::patch_size,
    entry_only.body.begin()+c::address-a::destroy_entry);
  check(entry_only.verify().valid,"partly installed own observer remains verifiable without authorizing work");
  check(!p::installed_hooks_current(entry_only.memory(),entry_only.own),"an owned entry alone cannot guard a writer");
}
}
int main(){tests();std::printf("cool_native_path_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;}
