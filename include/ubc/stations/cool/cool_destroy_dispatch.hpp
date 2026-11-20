// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_destroy_dispatch_profile.hpp"
#include "ubc/engine/read_only_memory.hpp"
#include "ubc/core/sha256.hpp"
#include <climits>
#include <cstring>

namespace ubc::cool_destroy_dispatch {
namespace profile=cool_destroy_dispatch_profile;
// Actual Root context values. Grinding uses no artificial Cool key.
struct Context {
  std::uint64_t generation=0;
  std::uint32_t thread=0,world=0,hero=0,station=0,manager=0;
  std::int32_t production_kind=0;
};
struct Invocation {std::uint32_t npc=0,message=0,caller=0,depth=0;};
enum class Scope {before_writer,before_destroy};
enum class Issue {none,binding,thread,reentry,boundary,unreadable,code,changed,npc,globals,class_chain,message,caller,refcount};
struct Values {
  std::uint32_t hero=0,npc_vtable=0,dispatch=0,visual=0,anictrl=0,anictrl_refs=0,skip=0,zero=0;
  std::array<std::uint32_t,4> parents{};
  std::uint32_t message_getter=0,message_delete=0;
  std::uint32_t cached_anictrl=0,message_vtable=0,message_refs=0,message_subtype=0,message_flags=0;
};
struct Evidence {
  bool path_identity_verified=false;
  Issue issue=Issue::binding;
  Scope scope=Scope::before_writer;
  Context context{};
  std::uint64_t frame=0;
  Invocation invocation{};
  Values values{};
  std::uint32_t failed_address=0;
  lhiver::Digest fingerprint{};
  // Separate tight dispatch evidence, no engine call/writer/end-ticket.
  static constexpr bool native_effect_authorized=false;
};
namespace detail {
inline bool valid(const Context& c) noexcept {
  return c.generation&&c.thread&&c.world&&c.hero&&c.station&&c.manager&&
    (c.production_kind==11||c.production_kind==13)&&!(c.hero&3u)&&c.hero<=UINT32_MAX-0x984u;
}
inline bool same(const Context& a,const Context& b) noexcept {
  return a.generation==b.generation&&a.thread==b.thread&&a.world==b.world&&a.hero==b.hero&&
    a.station==b.station&&a.manager==b.manager&&a.production_kind==b.production_kind;
}
inline bool same(const Invocation& a,const Invocation& b) noexcept {
  return a.npc==b.npc&&a.message==b.message&&a.caller==b.caller&&a.depth==b.depth;
}
inline bool read(const ReadOnlyMemory& m,std::uint32_t address,void* out,std::size_t size,Sha256& hash,Evidence& e) noexcept {
  if(!address||size>UINT64_C(0x100000000)-address||!m.copy(address,out,size)){
    e.issue=Issue::unreadable;e.failed_address=address;return false;
  }
  hash.update(&address,sizeof(address));hash.update(out,size);return true;
}
inline bool word(const ReadOnlyMemory& m,std::uint32_t address,std::uint32_t& out,Sha256& h,Evidence& e) noexcept {
  std::array<std::uint8_t,4> b{};if(!read(m,address,b.data(),b.size(),h,e))return false;
  out=std::uint32_t{b[0]}|(std::uint32_t{b[1]}<<8u)|(std::uint32_t{b[2]}<<16u)|(std::uint32_t{b[3]}<<24u);return true;
}
inline bool code(const ReadOnlyMemory& m,Sha256& h,Evidence& e) noexcept {
  std::array<std::uint8_t,2048> b{};
  for(const auto& r:profile::ranges){
    if(!r.length||r.length>b.size()||!read(m,r.address,b.data(),r.length,h,e))return false;
    Sha256 once;once.update(b.data(),r.length);
    if(once.finish()!=r.sha256){e.issue=Issue::code;e.failed_address=r.address;return false;}
  }
  return true;
}
inline bool values(const ReadOnlyMemory& m,Sha256& h,Evidence& e) noexcept {
  auto& v=e.values;const auto npc=e.context.hero;
  if(!word(m,0xab2684,v.hero,h,e)||!word(m,npc,v.npc_vtable,h,e)||
     !word(m,profile::npc_vtable+0x28,v.dispatch,h,e)||!word(m,npc+0xc8,v.visual,h,e)||
     !word(m,npc+0x97c,v.anictrl,h,e))return false;
  const auto reject=[&](Issue issue,std::uint32_t address) noexcept {
    e.issue=issue;e.failed_address=address;return false;
  };
  if(v.hero!=npc)return reject(Issue::npc,0xab2684);
  if(v.npc_vtable!=profile::npc_vtable)return reject(Issue::npc,npc);
  if(v.dispatch!=profile::on_message)return reject(Issue::npc,profile::npc_vtable+0x28);
  if(!v.visual)return reject(Issue::npc,npc+0xc8);
  if(!v.anictrl||(v.anictrl&3u)||v.anictrl>UINT32_MAX-4u)return reject(Issue::npc,npc+0x97c);
  if(!word(m,v.anictrl+4,v.anictrl_refs,h,e))return false;
  if(!v.anictrl_refs||v.anictrl_refs>INT32_MAX)return reject(Issue::npc,v.anictrl+4);
  // Exactly +0 excludes skip/delete, cache/init, and ASSERT side paths.
  // AB266C and AB26A0 are then unreachable and are not invented as evidence.
  if(!word(m,0xab2668,v.skip,h,e)||!word(m,0x82e8ac,v.zero,h,e))return false;
  if(v.skip)return reject(Issue::globals,0xab2668);
  if(v.zero)return reject(Issue::globals,0x82e8ac);
  auto node=profile::class_chain[0];
  for(std::size_t i=0;i<v.parents.size();++i){
    if(node!=profile::class_chain[i]||!word(m,node+0x3c,v.parents[i],h,e))return false;
    const auto expected=i+1<v.parents.size()?profile::class_chain[i+1]:0u;
    if(v.parents[i]!=expected){e.issue=Issue::class_chain;e.failed_address=node+0x3c;return false;}
    node=v.parents[i];
  }
  if(!word(m,profile::message_vtable,v.message_getter,h,e)||
     !word(m,profile::message_vtable+0x24,v.message_delete,h,e))return false;
  if(v.message_getter!=profile::class_getter)return reject(Issue::message,profile::message_vtable);
  if(v.message_delete!=profile::message_delete)return reject(Issue::message,profile::message_vtable+0x24);
  if(e.scope==Scope::before_writer)return true;
  const auto msg=e.invocation.message;
  if(!word(m,npc+0x980,v.cached_anictrl,h,e)||!word(m,msg,v.message_vtable,h,e)||
     !word(m,msg+4,v.message_refs,h,e)||!word(m,msg+0x40,v.message_flags,h,e))return false;
  std::array<std::uint8_t,2> subtype{};
  if(!read(m,msg+0x24,subtype.data(),subtype.size(),h,e))return false;
  v.message_subtype=std::uint32_t{subtype[0]}|(std::uint32_t{subtype[1]}<<8u);
  if(v.cached_anictrl!=v.anictrl)return reject(Issue::npc,npc+0x980);
  if(v.message_vtable!=profile::message_vtable)return reject(Issue::message,msg);
  if(v.message_subtype!=9)return reject(Issue::message,msg+0x24);
  // OnMessage captured exactly one reference before the handler. >=2 is a
  // current previous value for the normal DEC-JG return path, not evidence
  // for later reference consistency or an already-performed release.
  if(v.message_refs<2||v.message_refs>INT32_MAX)return reject(Issue::refcount,msg+4);
  return true;
}
}
class Reader {
public:
  explicit Reader(std::uint32_t owner_thread) noexcept:owner_(owner_thread){}
  Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
  Evidence capture_before_writer(const ReadOnlyMemory& m,const Context& c,std::uint64_t frame,
      std::uint32_t current_thread,bool stable_boundary) noexcept {
    return capture(m,c,frame,current_thread,stable_boundary,nullptr);
  }
  // Solely before original_destroy. Message not readable after.
  Evidence capture_before_destroy(const ReadOnlyMemory& m,const Context& c,std::uint64_t frame,
      std::uint32_t current_thread,bool stable_boundary,const Invocation& invocation) noexcept {
    return capture(m,c,frame,current_thread,stable_boundary,&invocation);
  }
private:
  Evidence capture(const ReadOnlyMemory& m,const Context& c,std::uint64_t frame,std::uint32_t thread,
      bool boundary,const Invocation* invocation) noexcept {
    Evidence e;e.context=c;e.frame=frame;e.scope=invocation?Scope::before_destroy:Scope::before_writer;
    if(invocation)e.invocation=*invocation;
    if(!detail::valid(c))return e;
    if(!owner_||owner_!=thread||owner_!=c.thread){e.issue=Issue::thread;return e;}
    if(reading_){e.issue=Issue::reentry;return e;}
    if(!boundary){e.issue=Issue::boundary;return e;}
    if(invocation&&(invocation->npc!=c.hero||invocation->caller!=cool_destroy_abi::destroy_return||invocation->depth!=1)){
      e.issue=Issue::caller;return e;
    }
    if(invocation&&(!invocation->message||(invocation->message&3u)||invocation->message>UINT32_MAX-0x44u)){
      e.issue=Issue::message;return e;
    }
    reading_=true;struct Lease{bool& reading;~Lease(){reading=false;}}lease{reading_};
    lhiver::Digest previous{};
    for(unsigned pass=0;pass<2;++pass){
      Sha256 h;if(!detail::code(m,h,e)||!detail::values(m,h,e))return e;
      const auto now=h.finish();if(pass&&now!=previous){e.issue=Issue::changed;return e;}previous=now;
    }
    e.fingerprint=previous;e.path_identity_verified=true;e.issue=Issue::none;return e;
  }
  std::uint32_t owner_=0;bool reading_=false;
};
inline bool matches(const Evidence& e,const Context& c,std::uint64_t frame,Scope scope,
    const Invocation& invocation={}) noexcept {
  if(!e.path_identity_verified||e.issue!=Issue::none||!detail::valid(c)||!detail::same(e.context,c)||
     e.frame!=frame||e.scope!=scope||e.values.hero!=c.hero||e.values.npc_vtable!=profile::npc_vtable||
     e.values.dispatch!=profile::on_message||!e.values.visual||!e.values.anictrl||!e.values.anictrl_refs||
     e.values.anictrl_refs>INT32_MAX||e.values.skip||e.values.zero||
     e.values.message_getter!=profile::class_getter||e.values.message_delete!=profile::message_delete)return false;
  for(std::size_t i=0;i<profile::class_chain.size();++i)
    if(e.values.parents[i]!=(i+1<profile::class_chain.size()?profile::class_chain[i+1]:0u))return false;
  if(scope==Scope::before_writer)return detail::same(e.invocation,Invocation{})&&detail::same(invocation,Invocation{});
  return detail::same(e.invocation,invocation)&&invocation.npc==c.hero&&invocation.caller==cool_destroy_abi::destroy_return&&
    invocation.depth==1&&invocation.message&&e.values.cached_anictrl==e.values.anictrl&&
    e.values.message_vtable==profile::message_vtable&&e.values.message_subtype==9&&
    e.values.message_refs>=2&&e.values.message_refs<=INT32_MAX;
}
} // namespace ubc::cool_destroy_dispatch
