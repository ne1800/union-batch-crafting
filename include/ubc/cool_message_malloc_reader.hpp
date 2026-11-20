// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/cool_message_malloc_profile.hpp"
#include "ubc/cool_hotblade_debit.hpp"
#include <utility>

namespace ubc::cool_message_malloc {
namespace dep=alchemy_extra_dependencies;
namespace hd=cool_hotblade_debit;
using Module=zbug_end_compat::Module;
inline constexpr bool runtime_ready=false;
enum class Issue {none,request,thread,reentry,serial,module,unreadable,changed,code,route,context,heap,new_mode,heap_alloc};
struct Request {
  hd::Job job{};
  std::uint64_t frame=0,serial=0;
  std::uint32_t current_thread=0;
  Module engine{},union_core{},crt{};
};
struct Values {
  std::uint32_t allocation_size=0,allocator_entry=0,allocator_forward=0;
  std::uint32_t heap_handle=0,new_mode=0,heap_alloc=0;
};
struct Capture {
  // Conditional partial evidence for the current state. No malloc call was observed.
  // The HeapAlloc/errno/PTD subpath and differences between windows remain unresolved.
  static constexpr bool native_effect_authorized=false,excludes_dispatch=false;
  bool known=false,direct_newhandler_edges_unreachable_current=false;
  Issue issue=Issue::none;
  std::uint32_t failed_address=0;
  Request request{};
  Values values{};
  lhiver::Digest fingerprint{};
};
namespace detail {
inline bool same_module(const Module& a,const Module& b) noexcept {
  return a.base==b.base&&a.image_size==b.image_size&&a.file_sha256==b.file_sha256&&a.mapping_verified==b.mapping_verified;
}
inline bool same_request(const Request& a,const Request& b) noexcept {
  const auto& x=a.job;const auto& y=b.job;
  return hd::values::same(x.context,y.context)&&x.id==y.id&&x.writer_token==y.writer_token&&x.fence_token==y.fence_token&&
    x.callback==y.callback&&x.owner==y.owner&&x.quantity==y.quantity&&a.frame==b.frame&&a.serial==b.serial&&
    a.current_thread==b.current_thread&&same_module(a.engine,b.engine)&&same_module(a.union_core,b.union_core)&&same_module(a.crt,b.crt);
}
inline bool request_valid(const Request& q) noexcept {
  if(!hd::valid(q.job)||!q.frame||!q.serial)return false;
  const auto& c=q.job.context;
  const std::array<std::uint32_t,4> objects{{c.game,c.world,c.hero,c.mob}};
  for(std::size_t i=0;i<objects.size();++i){
    if(objects[i]%4||!dep::detail::range(objects[i],i==2?0x968u:12u))return false;
    for(std::size_t j=0;j<i;++j)if(objects[i]==objects[j])return false;
  }
  return true;
}
inline void word_hash(Sha256& h,std::uint64_t value) noexcept {
  std::array<std::uint8_t,8> bytes{};for(unsigned i=0;i<8;++i)bytes[i]=static_cast<std::uint8_t>(value>>(i*8u));
  h.update(bytes.data(),bytes.size());
}
inline void request_hash(Sha256& h,const Request& q) noexcept {
  const auto& j=q.job;const auto& c=j.context;
  for(const std::uint64_t v:{c.epoch,std::uint64_t{c.thread},std::uint64_t{c.parser},std::uint64_t{c.game},
      std::uint64_t{c.world},std::uint64_t{c.hero},std::uint64_t{c.mob},j.id,j.writer_token,j.fence_token,
      std::uint64_t{j.callback},std::uint64_t{j.owner},std::uint64_t{j.quantity},q.frame,q.serial,std::uint64_t{q.current_thread}})word_hash(h,v);
  for(const auto* m:{&q.engine,&q.union_core,&q.crt}){
    word_hash(h,m->base);word_hash(h,m->image_size);word_hash(h,m->mapping_verified?1u:0u);
    h.update(m->file_sha256.data(),m->file_sha256.size());
  }
}
inline bool modules(const ReadOnlyMemory& m,const Request& q) noexcept {
  if(!q.engine.mapping_verified||q.engine.base!=0x400000||q.engine.image_size!=0x752000||q.engine.file_sha256!=profile::engine_sha256||
    !dep::detail::module(q.union_core,0x438000,zbug_end_compat::union_file_sha256)||
    !dep::detail::module(q.crt,profile::crt_size,dep::crt_sha256)||
    zbug_end_compat::detail::overlaps(q.union_core.base,q.union_core.image_size,q.crt.base,q.crt.image_size))return false;
  return dep::detail::pe(m,q.engine,0x3f966d87,0x400000).valid&&
    dep::detail::pe(m,q.union_core,0x62b8c69e,0x10000000).valid&&dep::detail::pe(m,q.crt,0x4ba1dbbe,profile::preferred_base).valid&&
    dep::detail::export_ordinal(m,q.union_core,258,0x35450).valid&&dep::detail::export_ordinal(m,q.crt,1477,profile::malloc_entry).valid;
}
struct Pass {Values values{};lhiver::Digest fingerprint{};};
struct ReadTrace {
  ReadOnlyMemory source;Sha256& hash;
  static bool copy(void* opaque,std::uint32_t address,void* target,std::size_t size) noexcept {
    auto& trace=*static_cast<ReadTrace*>(opaque);
    if(!trace.source.copy(address,target,size))return false;
    word_hash(trace.hash,address);word_hash(trace.hash,size);trace.hash.update(target,size);return true;
  }
};
inline Issue read_pass(const ReadOnlyMemory& source,const Request& q,Pass& out,std::uint32_t& failed) noexcept {
  Sha256 hash;request_hash(hash,q);
  ReadTrace trace{source,hash};const ReadOnlyMemory m{&trace,ReadTrace::copy};
  if(!modules(m,q))return Issue::module;
  std::array<std::uint8_t,512> code{};
  const auto read=[&](std::uint32_t address,std::size_t size) noexcept {
    failed=address;
    if(!size||size>code.size()||!dep::detail::range(address,size)||!m.copy(address,code.data(),size))return false;
    return true;
  };
  for(const auto& r:profile::engine_code){
    if(!read(r.address,r.length))return Issue::unreadable;
    if(dep::detail::digest(code.data(),r.length)!=r.digest)return Issue::code;
    if(r.address==0x6dbb10){
      const auto off=profile::allocation_site-r.address;
      out.values.allocation_size=runtime_script::detail::u32(code.data()+off+1);
      // The actual hashed callsite bytes deliver size/target;
      // no freely passed size bool and no claimed engine call.
      if(code[off]!=0x68||code[off+5]!=0xe8||out.values.allocation_size!=profile::size||
        out.values.allocation_size>profile::maximum_size||
        profile::return_address+runtime_script::detail::u32(code.data()+off+6)!=0x565f50)return Issue::route;
    }
  }
  const auto u=q.union_core.base,c=q.crt.base;
  if(!read(u+0x35450,10))return Issue::unreadable;
  dep::detail::put(code.data()+6,runtime_script::detail::u32(code.data()+6)-u+0x10000000u);
  if(dep::detail::digest(code.data(),10)!=lhiver::digest_literal("1efdd92c312a83b4876b9615f0f425850d3541fa2e63ca914efa60f30b000231"))return Issue::code;
  for(const auto& r:profile::malloc_code){
    if(!zbug_end_compat::detail::in_module(q.crt,r.rva,r.length)||!read(c+r.rva,r.length))return Issue::unreadable;
    for(std::size_t i=0;i<r.relocation_count;++i){
      const auto off=r.relocations[i];if(off>r.length-4||(i&&off<r.relocations[i-1]+4))return Issue::code;
      dep::detail::put(code.data()+off,runtime_script::detail::u32(code.data()+off)-c+profile::preferred_base);
    }
    if(dep::detail::digest(code.data(),r.length)!=r.sha256)return Issue::code;
  }
  const auto word=[&](std::uint32_t address,std::uint32_t& value) noexcept {
    if(!read(address,4))return false;value=runtime_script::detail::u32(code.data());return true;
  };
  auto& v=out.values;
  if(!word(0x82e30c,v.allocator_entry)||!word(u+0x812e8,v.allocator_forward))return Issue::unreadable;
  if(v.allocator_entry!=u+0x35450||v.allocator_forward!=c+profile::malloc_entry)return Issue::route;
  if(!word(c+profile::heap_handle,v.heap_handle)||!word(c+profile::new_mode,v.new_mode)||
    !word(c+profile::heap_alloc_iat,v.heap_alloc))return Issue::unreadable;
  if(!v.heap_handle){failed=c+profile::heap_handle;return Issue::heap;}
  if(v.new_mode){failed=c+profile::new_mode;return Issue::new_mode;}
  if(!v.heap_alloc){failed=c+profile::heap_alloc_iat;return Issue::heap_alloc;}
  const auto& context=q.job.context;
  const std::array<std::pair<std::uint32_t,std::uint32_t>,8> physical{{
    {selection_abi::game_pointer,context.game},{context.game,0x83c28c},{context.game+8,context.world},
    {context.world,0x83e174},{station_abi::hero_pointer,context.hero},{context.hero+0xb8,context.world},
    {context.hero+0x964,context.mob},{alchemy_flask_runtime::current_parser_pointer,context.parser}}};
  for(const auto& field:physical){std::uint32_t value=0;if(!word(field.first,value))return Issue::unreadable;if(value!=field.second)return Issue::context;}
  out.fingerprint=hash.finish();failed=0;return Issue::none;
}
} // namespace detail

// Own idle engine thread; held, truly file-bound modules. The memory
// copier must report errors as false. No engine/OS getters, allocation,
// I/O, or write access in capture(). Two complete copies prove neither
// ABA-freedom nor preservation of _newmode/Heap/Code in the later
// writer window. HeapAlloc is recorded only as the current IAT value;
// OS target binding and all errno/PTD subcalls remain explicitly open.
class Reader {
public:
  explicit Reader(std::uint32_t owner_thread) noexcept:owner_(owner_thread){}
  Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
  Capture capture(const ReadOnlyMemory& m,const Request& supplied) noexcept {
    Capture out;out.request=supplied;const auto& q=out.request;
    if(!owner_||q.current_thread!=owner_||q.job.context.thread!=owner_){out.issue=Issue::thread;return out;}
    if(busy_){reentered_=true;out.issue=Issue::reentry;return out;}
    if(!detail::request_valid(q)){out.issue=Issue::request;return out;}
    if(q.serial<=last_serial_){out.issue=Issue::serial;return out;}
    last_serial_=q.serial;busy_=true;reentered_=false;
    struct Reset {bool& busy;~Reset(){busy=false;}} reset{busy_};
    detail::Pass a{},b{};
    out.issue=detail::read_pass(m,q,a,out.failed_address);if(out.issue!=Issue::none)return out;
    out.issue=detail::read_pass(m,q,b,out.failed_address);if(out.issue!=Issue::none)return out;
    if(a.fingerprint!=b.fingerprint){out.issue=Issue::changed;return out;}
    if(reentered_){out.issue=Issue::reentry;return out;}
    out.values=a.values;out.fingerprint=a.fingerprint;out.known=out.direct_newhandler_edges_unreachable_current=true;return out;
  }
private:
  std::uint32_t owner_=0;std::uint64_t last_serial_=0;bool busy_=false,reentered_=false;
};
inline bool matches(const Capture& c,const Request& q) noexcept {
  return c.known&&c.direct_newhandler_edges_unreachable_current&&c.issue==Issue::none&&
    detail::request_valid(q)&&detail::same_request(c.request,q)&&c.values.allocation_size==profile::size&&
    c.values.heap_handle&&!c.values.new_mode&&c.values.heap_alloc&&
    c.values.allocator_entry==q.union_core.base+0x35450&&c.values.allocator_forward==q.crt.base+profile::malloc_entry;
}
} // namespace ubc::cool_message_malloc
