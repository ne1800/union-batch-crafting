// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/bow_item_code.hpp"
#include "ubc/vob_locator_compat.hpp"

// Exclusively the concretely evidenced Vob-Locator-Insert wrapper. No
// queue, actor, material, or production release. The native caller
// binds its own held module leases and the actual resting call context.
namespace ubc::vob_inventory_insert_compat {
namespace vob=vob_locator_compat;
namespace zbug=zbug_end_compat;
using Module=zbug::Module;
struct Binding {Module locator{},union_core{};};
inline constexpr auto locator_profile=vob::module_profiles[0];
inline constexpr auto union_profile=vob::module_profiles[3];
inline constexpr std::uint32_t slot_address=0x83c758,original_target=0x70c730,
  wrapper_rva=0x50f50,record_rva=0x95dbc,resolver_import_rva=0x7b168,dirty_rva=0x960c0;
inline constexpr std::array<std::uint16_t,4> wrapper_relocations{{0x1a,0x2d,0x33,0x3d}};
inline constexpr zbug::CodeRange wrapper{wrapper_rva,91,
  lhiver::digest_literal("d8af2302e218df0d14a938a85bbff5a8f920693f1827749d8b22f314cbbcf7fc"),
  wrapper_relocations.data(),wrapper_relocations.size()};
inline constexpr auto resolver=zbug::union_code[0];
inline constexpr auto original=bow_item_code::ranges[9];
static_assert(original.address==original_target&&original.length==573,"konkreter originaler Insertkoerper");
enum class Route {none,original,vob_direct};
enum class Issue {none,binding,slot,range,unreadable,changed,header,code,imports,record};
struct Verification {
  bool valid=false;Issue issue=Issue::none;std::uint32_t address=0;
  Route route=Route::none;std::uint32_t slot_target=0,record=0;
  lhiver::Digest fingerprint{};
};
inline bool same_module(const Module& a,const Module& b) noexcept {
  return a.base==b.base&&a.image_size==b.image_size&&a.file_sha256==b.file_sha256&&
    a.mapping_verified==b.mapping_verified;
}
inline bool same(const Verification& a,const Verification& b) noexcept {
  return a.valid&&b.valid&&a.route==b.route&&a.slot_target==b.slot_target&&a.record==b.record&&
    a.fingerprint==b.fingerprint;
}
namespace detail {
inline Verification fail(Issue issue,std::uint32_t address) noexcept {Verification r;r.issue=issue;r.address=address;return r;}
inline bool binding_valid(const Binding& b) noexcept {
  const std::array<const Module*,2> images{{&b.locator,&b.union_core}};
  const std::array<const vob::ModuleProfile*,2> profiles{{&locator_profile,&union_profile}};
  for(std::size_t i=0;i<images.size();++i){const auto& m=*images[i];const auto& p=*profiles[i];
    if(!m.mapping_verified||m.base%0x10000u||!zbug::detail::range(m.base,m.image_size)||
      m.image_size!=p.image_size||m.file_sha256!=p.file_sha256||
      zbug::detail::overlaps(m.base,m.image_size,vob::engine_base,vob::engine_size))return false;
  }
  return !zbug::detail::overlaps(b.locator.base,b.locator.image_size,b.union_core.base,b.union_core.image_size);
}
inline bool record_address(const Binding& b,std::uint32_t p) noexcept {
  return p%4u==0&&zbug::detail::range(p,28)&&
    !zbug::detail::overlaps(p,28,vob::engine_base,vob::engine_size)&&
    !zbug::detail::overlaps(p,28,b.locator.base,b.locator.image_size)&&
    !zbug::detail::overlaps(p,28,b.union_core.base,b.union_core.image_size);
}
struct Snapshot {
  std::array<std::uint8_t,4> slot{},record_pointer{},resolver_import{};
  std::array<std::uint8_t,28> record_bytes{};
  std::array<std::uint8_t,original.length> original_code{};
  std::array<std::uint8_t,locator_profile.header_length> locator_header{};
  std::array<std::uint8_t,union_profile.header_length> union_header{};
  std::array<std::uint8_t,wrapper.length> wrapper_code{};
  std::array<std::uint8_t,resolver.length> resolver_code{};
  std::uint32_t locator_base=0,union_base=0,record=0;
  Route route=Route::none;
  // The existing check primitives read exclusively this own
  // copy; the two full intakes below come from the actual reader.
  static bool copy(void* raw,std::uint32_t address,void* out,std::size_t size) noexcept {
    const auto& s=*static_cast<const Snapshot*>(raw);
    auto from=[&](std::uint32_t at,const auto& bytes) noexcept {
      if(address<at||std::uint64_t{address}-at+size>bytes.size())return false;
      if(size)std::memcpy(out,bytes.data()+(address-at),size);
      return true;
    };
    if(from(slot_address,s.slot)||from(original.address,s.original_code))return true;
    return s.route==Route::vob_direct&&(from(s.locator_base,s.locator_header)||from(s.union_base,s.union_header)||
      from(s.locator_base+wrapper_rva,s.wrapper_code)||from(s.union_base+resolver.rva,s.resolver_code));
  }
  ReadOnlyMemory memory() noexcept{return {this,copy};}
};
inline Verification capture(const ReadOnlyMemory& m,const Binding& b,Snapshot& s) noexcept {
  if(!m.copy(slot_address,s.slot.data(),s.slot.size()))return fail(Issue::unreadable,slot_address);
  const auto target=vob::detail::u32(s.slot.data());
  if(target==original_target)s.route=Route::original;
  else {
    if(!binding_valid(b))return fail(Issue::binding,slot_address);
    if(target!=b.locator.base+wrapper_rva)return fail(Issue::slot,slot_address);
    s.route=Route::vob_direct;s.locator_base=b.locator.base;s.union_base=b.union_core.base;
  }
  if(!m.copy(original.address,s.original_code.data(),s.original_code.size()))return fail(Issue::unreadable,original.address);
  if(s.route==Route::vob_direct){
    if(!m.copy(s.locator_base,s.locator_header.data(),s.locator_header.size()))return fail(Issue::unreadable,s.locator_base);
    if(!m.copy(s.union_base,s.union_header.data(),s.union_header.size()))return fail(Issue::unreadable,s.union_base);
    if(!m.copy(s.locator_base+wrapper.rva,s.wrapper_code.data(),s.wrapper_code.size()))return fail(Issue::unreadable,s.locator_base+wrapper.rva);
    if(!m.copy(s.union_base+resolver.rva,s.resolver_code.data(),s.resolver_code.size()))return fail(Issue::unreadable,s.union_base+resolver.rva);
    if(!m.copy(s.locator_base+resolver_import_rva,s.resolver_import.data(),s.resolver_import.size()))return fail(Issue::unreadable,s.locator_base+resolver_import_rva);
    if(!m.copy(s.locator_base+record_rva,s.record_pointer.data(),s.record_pointer.size()))return fail(Issue::unreadable,s.locator_base+record_rva);
    s.record=vob::detail::u32(s.record_pointer.data());
    if(!record_address(b,s.record))return fail(Issue::range,s.locator_base+record_rva);
    if(!m.copy(s.record,s.record_bytes.data(),s.record_bytes.size()))return fail(Issue::unreadable,s.record);
  }
  Verification out;out.valid=true;return out;
}
inline Verification validate(Snapshot& s,const Binding& b) noexcept {
  const auto m=s.memory();const vob::Region engine{vob::Module::engine,original.address-vob::engine_base,
    original.length,original.digest};
  if(vob::detail::region(m,{},engine)!=vob::Issue::none)return fail(Issue::code,original.address);
  if(s.route==Route::vob_direct){
    if(vob::detail::header(m,s.locator_base,locator_profile)!=vob::Issue::none)return fail(Issue::header,s.locator_base);
    if(vob::detail::header(m,s.union_base,union_profile)!=vob::Issue::none)return fail(Issue::header,s.union_base);
    if(!zbug::detail::verify_code(m,b.locator,wrapper).valid)return fail(Issue::code,s.locator_base+wrapper.rva);
    if(!zbug::detail::verify_code(m,b.union_core,resolver).valid)return fail(Issue::code,s.union_base+resolver.rva);
    if(vob::detail::u32(s.resolver_import.data())!=s.union_base+resolver.rva)return fail(Issue::imports,s.locator_base+resolver_import_rva);
    const auto* p=s.record_bytes.data();
    if(vob::detail::u32(p+0xc)!=0)return fail(Issue::record,s.record+0xc);
    if(vob::detail::u32(p+0x14)!=original_target)return fail(Issue::record,s.record+0x14);
    if(vob::detail::u32(p+0x18)!=s.locator_base+wrapper.rva)return fail(Issue::record,s.record+0x18);
  }
  Verification out;out.valid=true;out.route=s.route;out.slot_target=vob::detail::u32(s.slot.data());out.record=s.record;
  Sha256 hash;
  auto add=[&](const auto& bytes) noexcept {hash.update(bytes.data(),bytes.size());};
  auto word=[&](std::uint32_t value) noexcept {std::array<std::uint8_t,4> bytes{};vob::detail::put32(bytes.data(),value);add(bytes);};
  word(static_cast<std::uint32_t>(s.route));word(s.locator_base);word(s.union_base);word(s.record);
  add(s.slot);add(s.original_code);
  if(s.route==Route::vob_direct){add(s.locator_header);add(s.union_header);add(s.wrapper_code);add(s.resolver_code);
    add(s.record_pointer);add(s.resolver_import);add(s.record_bytes);}
  out.fingerprint=hash.finish();return out;
}
} // namespace detail

// 91 complete wrapper bytes: when game&&hero and inventory+0xA0==hero,
// locator+960C0 is set to 1, otherwise the old value is kept. Then exactly
// one Forward call; ECX/Stack argument/EAX/RET4 remain original. next!=0
// is not a known chain. The null-record branch (CALL0) remains locked.
// A success value describes only this read route. Duplicate copies lock
// observed changes, but neither foreign threads nor reentry in the writer.
inline Verification verify(const ReadOnlyMemory& memory,const Binding* binding=nullptr) noexcept {
  const Binding held=binding?*binding:Binding{};Verification first{};
  for(unsigned pass=0;pass<2;++pass){
    detail::Snapshot snapshot;auto result=detail::capture(memory,held,snapshot);if(!result.valid)return result;
    result=detail::validate(snapshot,held);if(!result.valid)return result;
    if(pass&&!same(first,result))return detail::fail(Issue::changed,slot_address);
    first=result;
  }
  return first;
}
} // namespace ubc::vob_inventory_insert_compat
