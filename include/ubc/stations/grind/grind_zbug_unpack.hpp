// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/grind/grind_recipe_snapshot.hpp"
#include "ubc/engine/zbug_end_compat.hpp"

namespace ubc::grind_zbug_unpack {
namespace z=zbug_end_compat;
using Binding=z::Binding;
enum class Route {none,original,empty_zbug};
enum class Issue {none,key,binding,range,unreadable,changed,hero,inventory,nonempty,terminator,code,cookie};
inline constexpr std::uint32_t thunk_rva=0x9b00,wrapper_rva=0x11e30,original_target=0x70f8f0;
inline constexpr std::uint32_t inventory_offset=0x668,inventory_vtable=0x83c714,cookie_rva=0x59014;
inline constexpr z::CodeRange thunk{thunk_rva,23,lhiver::digest_literal("a0a1a441b273d8aa4d104e47222b14840156b532ab684514a88adf917728dd77")};
inline constexpr std::array<std::uint16_t,2> wrapper_head_relocations{{6,21}},wrapper_tail_relocations{{16,205}};
inline constexpr std::array<std::uint16_t,1> cookie_relocations{{2}};
// Together exactly the whole 747B body; only split because of the
// existing 512B reader. No omitted instructions or non-empty branches.
inline constexpr std::array<z::CodeRange,3> code{{
  {wrapper_rva,512,lhiver::digest_literal("e1213bde500227d7d70513e93bfa76470fa9d27661cd3a7ab7612f0b24a32fe1"),wrapper_head_relocations.data(),wrapper_head_relocations.size()},
  {wrapper_rva+512,235,lhiver::digest_literal("0eb6b85c5acc692ac12b8a1059f1c1492939acd004a66e71d039f1c6c8e64a1a"),wrapper_tail_relocations.data(),wrapper_tail_relocations.size()},
  {0x2297a,14,lhiver::digest_literal("e859e10cdacddac39e8ddf08e5a76670f975d08594f792e660fef72a5b18e1e5"),cookie_relocations.data(),cookie_relocations.size()}
}};
struct Verification {
  bool valid=false;
  Issue issue=Issue::key;
  std::uint32_t address=0;
  Route route=Route::none;
  GrindRecipeKey key{};
  Binding binding{};
  std::uint32_t hero=0,vtable=0,cookie=0,thunk_target=0;
  packed_detail::Header header{};
  // Own values derived from the entire 23 bytes, no foreign code pointers.
  lhiver::Digest thunk_sha256{},normalized_thunk_sha256{};
  bool nul_read=false;
  std::uint8_t nul=0;
  lhiver::Digest fingerprint{};
  static constexpr bool native_effect_authorized=false;
};
namespace detail {
inline bool module_equal(const z::Module& a,const z::Module& b) noexcept {
  return a.base==b.base&&a.image_size==b.image_size&&a.file_sha256==b.file_sha256&&a.mapping_verified==b.mapping_verified;
}
inline bool binding_equal(const Binding& a,const Binding& b) noexcept {
  return module_equal(a.zbug,b.zbug)&&module_equal(a.union_core,b.union_core);
}
inline void number(Sha256& h,std::uint32_t v) noexcept {std::array<std::uint8_t,4> b{};z::detail::word(b.data(),v);h.update(b.data(),b.size());}
inline lhiver::Digest fingerprint(const Verification& e) noexcept {
  Sha256 h;for(const auto* m:{&e.binding.zbug,&e.binding.union_core}){
    number(h,m->base);number(h,m->image_size);h.update(m->file_sha256.data(),m->file_sha256.size());}
  for(const auto v:{e.hero,e.vtable,e.cookie,e.thunk_target,static_cast<std::uint32_t>(e.route),static_cast<std::uint32_t>(e.nul_read),std::uint32_t{e.nul}})number(h,v);
  h.update(e.header.data(),e.header.size());h.update(e.thunk_sha256.data(),e.thunk_sha256.size());
  h.update(e.normalized_thunk_sha256.data(),e.normalized_thunk_sha256.size());return h.finish();
}
inline Route route(const Verification& e) noexcept {
  if(e.normalized_thunk_sha256!=thunk.sha256||e.thunk_sha256==lhiver::Digest{})return Route::none;
  if(e.thunk_target==original_target)return e.thunk_sha256==thunk.sha256?Route::original:Route::none;
  return e.thunk_target==e.binding.zbug.base+wrapper_rva&&e.thunk_sha256!=thunk.sha256?Route::empty_zbug:Route::none;
}
inline bool read(const ReadOnlyMemory& m,std::uint32_t address,void* bytes,std::size_t length,Verification& out) noexcept {
  if(!z::detail::range(address,length)){out.issue=Issue::range;out.address=address;return false;}
  if(!m.copy(address,bytes,length)){out.issue=Issue::unreadable;out.address=address;return false;}return true;
}
inline bool word(const ReadOnlyMemory& m,std::uint32_t address,std::uint32_t& value,Verification& out) noexcept {
  std::array<std::uint8_t,4> bytes{};if(!read(m,address,bytes.data(),bytes.size(),out))return false;value=z::detail::u32(bytes.data());return true;
}
inline bool values(const ReadOnlyMemory& m,Verification& out) noexcept {
  if(!word(m,station_abi::hero_pointer,out.hero,out))return false;
  if(out.hero!=out.key.hero){out.issue=Issue::hero;out.address=station_abi::hero_pointer;return false;}
  if(!word(m,out.hero+inventory_offset,out.vtable,out)||
     !read(m,out.hero+packed_detail::header_offset,out.header.data(),out.header.size(),out))return false;
  if(out.vtable!=inventory_vtable||z::detail::u32(out.header.data())!=out.hero){out.issue=Issue::inventory;out.address=out.hero+inventory_offset;return false;}
  if(z::detail::u32(out.header.data()+32)){out.issue=Issue::nonempty;out.address=out.hero+packed_detail::header_offset+32;return false;}
  const auto pointer=z::detail::u32(out.header.data()+28);out.nul_read=pointer!=0;
  if(pointer&&!read(m,pointer,&out.nul,1,out))return false;
  if(out.nul){out.issue=Issue::terminator;out.address=pointer;return false;}
  if(!word(m,out.binding.zbug.base+cookie_rva,out.cookie,out))return false;
  if(!out.cookie){out.issue=Issue::cookie;out.address=out.binding.zbug.base+cookie_rva;return false;}
  std::array<std::uint8_t,23> bytes{};
  if(!read(m,out.binding.zbug.base+thunk.rva,bytes.data(),bytes.size(),out))return false;
  out.thunk_sha256=z::detail::digest(bytes.data(),bytes.size());
  out.thunk_target=z::detail::u32(bytes.data()+11);
  if(bytes[10]!=0xb8){out.issue=Issue::code;out.address=out.binding.zbug.base+thunk.rva+10;return false;}
  z::detail::word(bytes.data()+11,original_target);
  out.normalized_thunk_sha256=z::detail::digest(bytes.data(),bytes.size());
  out.route=route(out);if(out.route==Route::none){out.issue=Issue::code;out.address=out.binding.zbug.base+thunk.rva;return false;}
  return true;
}
}
inline bool matches(const Verification& e,const GrindRecipeKey& key,const Binding& binding) noexcept {
  return e.valid&&e.issue==Issue::none&&grind_recipe_snapshot_detail::valid_key(key)&&
    grind_recipe_snapshot_detail::same_key(e.key,key)&&z::detail::binding_valid(binding)&&detail::binding_equal(e.binding,binding)&&
    e.hero==key.hero&&e.vtable==inventory_vtable&&z::detail::u32(e.header.data())==key.hero&&
    !z::detail::u32(e.header.data()+32)&&e.nul_read==(z::detail::u32(e.header.data()+28)!=0)&&!e.nul&&e.cookie&&
    e.route!=Route::none&&e.route==detail::route(e)&&e.fingerprint==detail::fingerprint(e);
}
inline bool same(const Verification& a,const Verification& b) noexcept {
  return matches(a,b.key,b.binding)&&matches(b,a.key,a.binding)&&a.fingerprint==b.fingerprint;
}
// Only the fresh grind-HERO. The caller binds thread/rest window,
// physical preview-HERO or active writer-SELF as before. The two records
// lock no simultaneously writing plugins and no ABA. The alternative
// proves only the early empty path: +A4==0 or +C0==0 leads to cookie
// check and RET10 without tokenizer/allocator/stringwriter. Non-empty
// packstrings remain locked even if no recipe target is inside them.
inline Verification verify(const ReadOnlyMemory& memory,const Binding& supplied,const GrindRecipeKey& current) noexcept {
  Verification initial;initial.key=current;initial.binding=supplied;
  if(!grind_recipe_snapshot_detail::valid_key(initial.key))return initial;
  if(initial.key.hero%4||!z::detail::range(initial.key.hero,0x1000)){initial.issue=Issue::range;initial.address=initial.key.hero;return initial;}
  if(!z::detail::binding_valid(initial.binding)){initial.issue=Issue::binding;return initial;}
  for(unsigned pass=0;pass<2;++pass){
    Verification now;now.key=initial.key;now.binding=initial.binding;
    if(!detail::values(memory,now))return now;
    for(const auto* m:{&now.binding.zbug,&now.binding.union_core}){
      const auto c=z::detail::verify_pe(memory,*m,m==&now.binding.zbug?0x68bd012au:0x62b8c69eu);
      if(!c.valid){now.issue=c.issue==z::Issue::changed?Issue::changed:Issue::binding;now.address=m->base;return now;}
    }
    if(now.route==Route::empty_zbug)for(const auto& area:code){
      const auto c=z::detail::verify_code(memory,now.binding.zbug,area);
      if(!c.valid){now.issue=c.issue==z::Issue::changed?Issue::changed:Issue::code;now.address=now.binding.zbug.base+area.rva;return now;}
    }
    now.fingerprint=detail::fingerprint(now);now.issue=Issue::none;now.valid=true;
    if(pass&&!same(initial,now)){now.valid=false;now.issue=Issue::changed;now.address=now.hero+packed_detail::header_offset;return now;}
    initial=now;
  }
  return initial;
}
} // namespace ubc::grind_zbug_unpack
