// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/lhiver_recipe_profile.hpp"
#include "ubc/read_only_memory.hpp"
#include "ubc/sha256.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ubc::zbug_end_compat {
// Tight read profile of the locally checked ZBug/Union version. It is
// NOT a general foreign hook bypass and NOT a normal end/material effect
// proof. The caller must continue to check end-before/after, null anchor,
// later resting end frame, and all result deltas. A null anchor before
// EndInteraction does not prove the value at the later StopInteraction load.
inline constexpr std::uint32_t preferred_base=0x10000000;
inline constexpr std::uint32_t zbug_image_size=0x67000,union_image_size=0x438000;
inline constexpr std::uint32_t call_address=0x721e53,original_put_in_inv=0x749350,wrapper_rva=0x11c80;
inline constexpr std::array<std::uint8_t,5> original_call{{0xe8,0xf8,0x74,0x02,0x00}};
inline constexpr auto zbug_file_sha256=lhiver::digest_literal("50c3dc669a451dc084c4691b6e6fb7a39838d40627629e59e884579873811963");
inline constexpr auto union_file_sha256=lhiver::digest_literal("78d9312f07ebdf978d301fde63841ddcbe265b59cb7c2753e01d2dbdc5742888");
inline constexpr std::size_t max_code_size=512;
struct Module {
  std::uint32_t base=0,image_size=0;
  lhiver::Digest file_sha256{};
  // Native adapter: exactly this loaded MEM_IMAGE is bound to
  // exactly this file; file fully hashed, module present until after
  // insertion. No proof solely from name, found CALL, or PE header.
  bool mapping_verified=false;
  bool (*executable_page)(std::uint32_t base,std::uint32_t target) noexcept=nullptr;
};
// The two-bit pre-debit trusts the loaded plugin from the first forward.
// All other order paths continue to use the fixed compatibility profile.
struct Binding {Module zbug{},union_core{};bool cooling_predebit=false;};
enum class Issue {none,binding,range,unreadable,changed,pe,call,imports,dispatch,code_hash};
struct Verification {bool valid=false;Issue issue=Issue::none;std::uint32_t call_target=0,original_target=0;};
struct CodeRange {
  std::uint32_t rva,length;lhiver::Digest sha256;
  const std::uint16_t* relocations=nullptr;std::size_t relocation_count=0;
};
inline constexpr std::array<std::uint16_t,14> wrapper_relocations{{6,21,57,66,153,158,215,220,349,358,367,377,399,407}};
inline constexpr std::array<std::uint16_t,6> getter_relocations{{2,11,20,30,39,47}};
inline constexpr std::array<std::uint16_t,1> compare_relocations{{42}};
inline constexpr std::array<CodeRange,5> zbug_code{{
  {wrapper_rva,428,lhiver::digest_literal("7193ed6f0a773773b50b46a45a9f92457d1daa1c2fb05b3b532eb313f2340c59"),wrapper_relocations.data(),wrapper_relocations.size()},
  {0x12920,62,lhiver::digest_literal("bd769bc2e9d057ecaa8e72346291150c6ad6ec8be9af94a4fd2f5ff58d5b47ba"),getter_relocations.data(),getter_relocations.size()},
  {0x92e0,21,lhiver::digest_literal("8ce027b17bf72f1ec1b956c646eceef6e383e43e3550fc6c3be8a46e562d8e52")},
  {0x9a60,21,lhiver::digest_literal("2bdc0331f187c520c6d69be13676d94abcecf8ab4c3e02b05527e9fe48ee5a51")},
  {0x9a80,21,lhiver::digest_literal("b8fa49dab7b03db4dec7accfc390fbcfa3f386863d829ad427ba5137d681d2c0")}
}};
inline constexpr std::array<CodeRange,2> union_code{{
  {0x31aa0,15,lhiver::digest_literal("00ad9d87537091652919476c1beb3470eab23b0a59ab9f9cf332434f3e53abc3")},
  {0x12c70,64,lhiver::digest_literal("875140693063a83cfab6791b6b6225551a654af70cf5c32d77cc7c5b37f3c9e8"),compare_relocations.data(),compare_relocations.size()}
}};
namespace detail {
inline bool range(std::uint32_t a,std::size_t n) noexcept {return a&&n&&n<=UINT64_C(0x100000000)-a;}
inline bool overlaps(std::uint32_t a,std::size_t an,std::uint32_t b,std::size_t bn) noexcept {
  return std::uint64_t{a}<std::uint64_t{b}+bn&&std::uint64_t{b}<std::uint64_t{a}+an;
}
inline std::uint32_t u32(const std::uint8_t* b) noexcept {
  return std::uint32_t{b[0]}|(std::uint32_t{b[1]}<<8u)|(std::uint32_t{b[2]}<<16u)|(std::uint32_t{b[3]}<<24u);
}
inline std::uint16_t u16(const std::uint8_t* b) noexcept {return static_cast<std::uint16_t>(unsigned{b[0]}|(unsigned{b[1]}<<8u));}
inline void word(std::uint8_t* out,std::uint32_t value) noexcept {
  for(unsigned i=0;i<4;++i)out[i]=static_cast<std::uint8_t>(value>>(i*8u));
}
inline lhiver::Digest digest(const void* data,std::size_t size) noexcept {Sha256 h;h.update(data,size);return h.finish();}
inline bool in_module(const Module& m,std::uint32_t offset,std::size_t n) noexcept {
  return range(m.base,m.image_size)&&n&&offset<m.image_size&&n<=m.image_size-offset;
}
inline bool mapped_plugin(const Module& m) noexcept {
  return m.mapping_verified&&range(m.base,m.image_size)&&!overlaps(m.base,m.image_size,0x400000,0x752000);
}
inline bool forwards_to(const Module& m,std::uint32_t target) noexcept {
  return mapped_plugin(m)&&target>=m.base&&in_module(m,target-m.base,1)&&
    m.executable_page&&m.executable_page(m.base,target);
}
inline bool cooling_forward(const Binding& b,std::uint32_t target) noexcept {
  return b.cooling_predebit&&(forwards_to(b.zbug,target)||forwards_to(b.union_core,target));
}
inline Verification fail(Issue issue) noexcept {return {false,issue,0,0};}
inline Verification okay() noexcept {return {true,Issue::none,0,0};}
inline bool binding_valid(const Binding& b) noexcept {
  for(const auto* m:{&b.zbug,&b.union_core})if(!m->mapping_verified||!range(m->base,m->image_size)||
      overlaps(m->base,m->image_size,0x400000,0x752000))return false;
  return b.zbug.image_size==zbug_image_size&&b.union_core.image_size==union_image_size&&
    b.zbug.file_sha256==zbug_file_sha256&&b.union_core.file_sha256==union_file_sha256&&
    !overlaps(b.zbug.base,b.zbug.image_size,b.union_core.base,b.union_core.image_size);
}
inline Verification verify_pe(const ReadOnlyMemory& mem,const Module& m,std::uint32_t timestamp) noexcept {
  std::array<std::uint8_t,64> first{},again{};
  if(!mem.copy(m.base,first.data(),first.size())||!mem.copy(m.base,again.data(),again.size()))return fail(Issue::unreadable);
  if(first!=again)return fail(Issue::changed);
  const auto offset=u32(first.data()+60);
  if(first[0]!='M'||first[1]!='Z'||offset<64||offset>0x1000-84||!in_module(m,offset,84))return fail(Issue::pe);
  std::array<std::uint8_t,84> nt{},nt2{};
  if(!mem.copy(m.base+offset,nt.data(),nt.size())||!mem.copy(m.base+offset,nt2.data(),nt2.size()))return fail(Issue::unreadable);
  if(nt!=nt2)return fail(Issue::changed);
  // Windows keeps the preferred base, Wine can register the real one.
  const auto image_base=u32(nt.data()+52);
  if(u32(nt.data())!=0x4550||u16(nt.data()+4)!=0x14c||u32(nt.data()+8)!=timestamp||
    u16(nt.data()+20)!=224||u16(nt.data()+24)!=0x10b||(image_base!=preferred_base&&image_base!=m.base)||
    u32(nt.data()+80)!=m.image_size)return fail(Issue::pe);
  return okay();
}
inline Verification verify_code(const ReadOnlyMemory& mem,const Module& m,const CodeRange& wanted) noexcept {
  if(!wanted.length||wanted.length>max_code_size||!in_module(m,wanted.rva,wanted.length)||
    (wanted.relocation_count&&!wanted.relocations)||wanted.relocation_count>wanted.length/4)return fail(Issue::range);
  std::array<std::uint8_t,max_code_size> a{},b{};
  if(!mem.copy(m.base+wanted.rva,a.data(),wanted.length)||!mem.copy(m.base+wanted.rva,b.data(),wanted.length))return fail(Issue::unreadable);
  if(std::memcmp(a.data(),b.data(),wanted.length))return fail(Issue::changed);
  const auto delta=m.base-preferred_base;
  for(std::size_t i=0;i<wanted.relocation_count;++i){const auto offset=wanted.relocations[i];
    if(offset>wanted.length-4||(i&&offset<wanted.relocations[i-1]+4))return fail(Issue::range);
    word(a.data()+offset,u32(a.data()+offset)-delta);
  }
  return digest(a.data(),wanted.length)==wanted.sha256?okay():fail(Issue::code_hash);
}
struct Dispatch {
  std::array<std::uint8_t,5> call{};
  std::array<std::uint8_t,24> globals{},object{};
  std::array<std::uint8_t,4> getter_import{},compare_import{};
  std::array<std::uint8_t,16> torch{};
  bool operator==(const Dispatch& b) const noexcept {
    return call==b.call&&globals==b.globals&&object==b.object&&getter_import==b.getter_import&&compare_import==b.compare_import&&torch==b.torch;
  }
};
inline Verification read_dispatch(const ReadOnlyMemory& mem,const Binding& b,Dispatch& out) noexcept {
  const auto base=b.zbug.base;
  if(!mem.copy(call_address,out.call.data(),out.call.size())||
    !mem.copy(base+0x5df70,out.globals.data(),out.globals.size())||
    !mem.copy(base+0x4515c,out.getter_import.data(),out.getter_import.size())||
    !mem.copy(base+0x451a0,out.compare_import.data(),out.compare_import.size())||
    !mem.copy(base+0x50e84,out.torch.data(),out.torch.size()))return fail(Issue::unreadable);
  const auto target=call_address+5u+u32(out.call.data()+1);
  if(out.call[0]!=0xe8||target!=base+wrapper_rva)return fail(Issue::call);
  if(u32(out.getter_import.data())!=b.union_core.base+0x31aa0||u32(out.compare_import.data())!=b.union_core.base+0x12c70)
    return fail(Issue::imports);
  // Only the observed ModulePatchCallInfo mode 2 and the direct original
  // path. Never guess or execute mode 1 or alternative hook chains.
  if(u32(out.globals.data()+8)!=2||u32(out.globals.data()+20)!=2||
    u32(out.globals.data()+12)!=original_put_in_inv||u32(out.globals.data()+16)!=target||
    std::memcmp(out.torch.data(),"ITLSTORCHBURNING",out.torch.size()))return fail(Issue::dispatch);
  const auto object=u32(out.globals.data());
  if(!range(object,out.object.size())||overlaps(object,out.object.size(),0x400000,0x752000)||
    overlaps(object,out.object.size(),b.zbug.base,b.zbug.image_size)||
    overlaps(object,out.object.size(),b.union_core.base,b.union_core.image_size))return fail(Issue::dispatch);
  if(!mem.copy(object,out.object.data(),out.object.size()))return fail(Issue::unreadable);
  // GetReturnAddress: mov eax,[ecx+0C]; test; if null mov eax,[ecx+14];ret.
  if(u32(out.object.data())!=b.union_core.base+0x81df8||u32(out.object.data()+12)||
    u32(out.object.data()+20)!=original_put_in_inv)return fail(Issue::dispatch);
  return {true,Issue::none,target,original_put_in_inv};
}
} // namespace detail

// Check on the serialized engine thread before a own effect and at both
// end boundaries. No protection against arbitrary simultaneously writing
// plugins; two stable full passes are read proofs, not a lock. Additional
// behavior of the known wrapper concerns only ITLSTORCHBURNING: refcount
// protection, possibly world removal, then exactly the stored original call
// and its EAX/ret4. Other names/null take the direct original dispatch.
// CRT/engine/virtual world calls are thereby not generally certified.
inline Verification verify(const ReadOnlyMemory& memory,const Binding& binding) noexcept {
  using namespace detail;
  if(!binding_valid(binding))return fail(Issue::binding);
  Dispatch initial{};auto result=read_dispatch(memory,binding,initial);if(!result.valid)return result;
  const Module engine{0x400000,0x752000,{},true};
  constexpr CodeRange original{original_put_in_inv-0x400000,357,
    lhiver::digest_literal("047d8df6c814ae187d00d7d5186c124eb13e7e7bae6c27babae88cb809ad684c")};
  for(unsigned pass=0;pass<2;++pass){
    auto checked=verify_pe(memory,binding.zbug,0x68bd012a);if(!checked.valid)return checked;
    checked=verify_pe(memory,binding.union_core,0x62b8c69e);if(!checked.valid)return checked;
    for(const auto& code:zbug_code){checked=verify_code(memory,binding.zbug,code);if(!checked.valid)return checked;}
    for(const auto& code:union_code){checked=verify_code(memory,binding.union_core,code);if(!checked.valid)return checked;}
    checked=verify_code(memory,engine,original);if(!checked.valid)return checked;
    Dispatch current{};checked=read_dispatch(memory,binding,current);if(!checked.valid)return checked;
    if(!(initial==current))return fail(Issue::changed);
  }
  return result;
}
} // namespace ubc::zbug_end_compat
