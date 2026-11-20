// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/bow/bow_item_native.hpp"

namespace ubc::bow_item_renderer {
namespace native=bow_item_native; namespace rs=runtime_script;
using Module=zbug_end_compat::Module;
using Issue=native::Issue;using Check=native::Check;
inline constexpr std::uint32_t image_size=0xfd000,preferred_base=0x10000000,timestamp=0x694df220;
inline constexpr auto file_sha256=lhiver::digest_literal("551862f85332321f6f4601feac555ca04f400bf816dd055bfe3230c45dcfd40a");
struct Range {std::uint32_t rva,length;lhiver::Digest digest;const std::uint16_t* relocations=nullptr;std::size_t count=0;};
inline constexpr std::array<std::uint16_t,1> reloc_0{{6}};
inline constexpr std::array<std::uint16_t,4> reloc_1{{6,11,59,116}};
inline constexpr std::array<std::uint16_t,3> reloc_2{{16,285,295}};
inline constexpr std::array<Range,5> ranges{{
  {0x7ec00,10,lhiver::digest_literal("a3d90c7a348e61a59b48669a10c072d476f7b268ee31254601249d55c8b16ff7"),reloc_0.data(),reloc_0.size()}, // WorldRemoveHook
  {0x7ec10,138,lhiver::digest_literal("cac7f1ebbee4f6c9110520f0698115129cedb9e207564d39665a5a1ad285caae"),reloc_1.data(),reloc_1.size()}, // WorldRemoveListsHook
  {0x66830,2073,lhiver::digest_literal("647210d51d67cac54c4496bf5206cd2783bfb98ecab15b87a8105f920a5d5caf"),reloc_2.data(),reloc_2.size()}, // OnVobRemoved
  {0x35e40,260,lhiver::digest_literal("8d50e9c63e1d119c90ae402252cd9e9951364baab173877d03ffc5fa9e1aabbd")}, // DeviceOnVobRemoved
  {0x386d0,99,lhiver::digest_literal("4690e3c5d058b5c9a294dd3ad318e383cc7aaece5dd9790c42e97c73d6915b2c")}, // HashFind
}};
namespace detail {
using native::detail::word;using native::detail::word_hash;using native::detail::read_hash;
using native::detail::range;using native::detail::fail;
inline Check code(const ReadOnlyMemory& m,const Module& module,const Range& r) noexcept {
  if(!r.length||r.length>4096||!zbug_end_compat::detail::in_module(module,r.rva,r.length))return fail(Issue::range);
  std::array<std::uint8_t,4096> bytes{};const auto checked=alchemy_extra_dependencies::detail::twice(m,module.base+r.rva,bytes.data(),r.length);
  if(!checked.valid)return fail(checked.issue==alchemy_extra_dependencies::Issue::changed?Issue::changed:Issue::unreadable,module.base+r.rva);
  for(std::size_t i=0;i<r.count;++i){const auto at=r.relocations[i];if(at>r.length-4||(i&&at<r.relocations[i-1]+4))return fail(Issue::range);
    alchemy_extra_dependencies::detail::put(bytes.data()+at,rs::detail::u32(bytes.data()+at)-module.base+preferred_base);}
  return alchemy_extra_dependencies::detail::digest(bytes.data(),r.length)==r.digest?Check{true,Issue::none,0}:fail(Issue::code,module.base+r.rva);
}
inline bool outside(const Module& m,std::uint32_t a,std::size_t n) noexcept {
  return range(a,n)&&!zbug_end_compat::detail::overlaps(a,n,m.base,m.image_size)&&!zbug_end_compat::detail::overlaps(a,n,0x400000,0x752000);
}
struct Hook {std::uint32_t entry,length,rva,slot;std::array<std::uint8_t,7> prologue;};
inline constexpr std::array<Hook,2> hooks{{
  {0x7800c0,7,0x7ec00,0xe2a3c,{{0x6a,0xff,0x68,0x13,0x92,0x82,0x00}}},
  {0x780990,6,0x7ec10,0xe2a40,{{0x64,0xa1,0x00,0x00,0x00,0x00,0x00}}}
}};
inline Check hook(const ReadOnlyMemory& m,const Module& module,const Hook& h,std::uint32_t& trampoline) noexcept {
  std::array<std::uint8_t,12> bytes{},copy{};std::array<std::uint8_t,4096> body{};
  if(!word(m,module.base,h.slot,trampoline)||!outside(module,trampoline,h.length+5))return fail(Issue::dispatch,h.entry);
  const auto& original=native::engine_ranges[h.entry==0x7800c0?15:16];
  if(!alchemy_extra_dependencies::detail::twice(m,h.entry,body.data(),original.length).valid||
     body[0]!=0xe9||h.entry+5u+rs::detail::u32(body.data()+1)!=module.base+h.rva)return fail(Issue::dispatch,h.entry);
  // The exact E9 transfer writes only five bytes. Behind this
  // unconditional jump either all original remaining bytes are
  // preserved or all are NOP or all are INT3. The entry jumps
  // past these bytes; the trampoline checked below returns only at
  // entry+length. Do not normalize arbitrary or mixed remaining bytes.
  const bool original_tail=std::memcmp(body.data()+5,h.prologue.data()+5,h.length-5)==0;
  bool nop_tail=true,int3_tail=true;
  for(std::uint32_t i=5;i<h.length;++i){nop_tail=nop_tail&&body[i]==0x90;int3_tail=int3_tail&&body[i]==0xcc;}
  if(!original_tail&&!nop_tail&&!int3_tail)return fail(Issue::dispatch,h.entry+5);
  std::memcpy(body.data(),h.prologue.data(),h.length);
  if(alchemy_extra_dependencies::detail::digest(body.data(),original.length)!=original.digest)return fail(Issue::code,h.entry);
  if(!m.copy(trampoline,bytes.data(),h.length+5)||!m.copy(trampoline,copy.data(),h.length+5)||bytes!=copy||
     std::memcmp(bytes.data(),h.prologue.data(),h.length)||bytes[h.length]!=0xe9||
     trampoline+h.length+5u+rs::detail::u32(bytes.data()+h.length+1)!=h.entry+h.length)return fail(Issue::dispatch,trampoline);
  if(zbug_end_compat::detail::overlaps(trampoline,h.length+5,module.base+h.rva,ranges[h.entry==0x7800c0?0:1].length))return fail(Issue::alias,trampoline);
  return {true,Issue::none,0};
}
} // namespace detail
struct CodeEvidence {bool verified=false;Issue issue=Issue::none;std::uint32_t address=0;std::array<std::uint32_t,2> trampolines{};};
// Full relevant bodies + exact relocs, both foreign hook targets, both
// original trampolines and full normalized original functions. No call.
inline CodeEvidence verify_code(const ReadOnlyMemory& memory,const Module& module) noexcept {
  CodeEvidence out;
  if(!alchemy_extra_dependencies::detail::module(module,image_size,file_sha256)||
     !alchemy_extra_dependencies::detail::pe(memory,module,timestamp,preferred_base).valid){out.issue=Issue::module;return out;}
  for(unsigned pass=0;pass<2;++pass){
    for(const auto& r:ranges){const auto c=detail::code(memory,module,r);if(!c.verified){out.issue=c.issue;out.address=c.address;return out;}}
    for(std::size_t i=0;i<detail::hooks.size();++i){std::uint32_t t=0;const auto c=detail::hook(memory,module,detail::hooks[i],t);
      if(!c.verified){out.issue=c.issue;out.address=c.address;return out;}
      if(pass&&t!=out.trampolines[i]){out.issue=Issue::changed;return out;}out.trampolines[i]=t;}
    if(zbug_end_compat::detail::overlaps(out.trampolines[0],12,out.trampolines[1],11)){out.issue=Issue::alias;return out;}
  }
  out.verified=true;return out;
}
struct ObjectRequest {
  bow_item_path::Boundary boundary{};
  std::uint32_t object=0;std::int32_t symbol=-1;
};
struct ForwardEvidence {
  bool verified=false;Issue issue=Issue::none;
  std::uint32_t state=0,device=0,vtable=0,callback=0;
  std::array<std::uint32_t,2> trampolines{};
};
inline bool same(const ForwardEvidence& a,const ForwardEvidence& b) noexcept {
  return a.verified&&b.verified&&a.state==b.state&&a.device==b.device&&a.vtable==b.vtable&&
    a.callback==b.callback&&a.trampolines==b.trampolines;
}
// Explicit compatibility boundary: the same pinned renderer may
// execute its normal cache/UI tail. The two wrappers then forward
// the same World-/Vob-arguments once to the original. This admission
// claims neither an empty cache nor side-effect-free render internals.
// The integrator holds the module lease, the original call, and the
// later material/end comparison; this reader suppresses no call.
inline ForwardEvidence verify_forwarding(const ReadOnlyMemory& memory,const Module& module) noexcept {
  ForwardEvidence out;const auto before=verify_code(memory,module);
  if(!before.verified){out.issue=before.issue;return out;}
  for(unsigned pass=0;pass<2;++pass){ForwardEvidence current;
    if(!detail::word(memory,module.base,0xea6ac,current.state)||!detail::outside(module,current.state,0x594)||
       !detail::word(memory,module.base,0xea6a0,current.device)||!detail::outside(module,current.device,0x208)||
       zbug_end_compat::detail::overlaps(current.state,0x594,current.device,0x208)||
       !detail::word(memory,current.device,0,current.vtable)||current.vtable!=module.base+0xcc7dc||
       !detail::word(memory,current.vtable,0xbc,current.callback)||current.callback!=module.base+0x35e40){out.verified=false;out.issue=Issue::dispatch;return out;}
    current.trampolines=before.trampolines;current.verified=true;
    if(pass&&!same(out,current)){out.verified=false;out.issue=Issue::changed;return out;}out=current;
  }
  const auto after=verify_code(memory,module);
  if(!after.verified||after.trampolines!=before.trampolines){out.verified=false;out.issue=Issue::changed;}
  return out;
}
// This positive check applies to EXACTLY the existing candidate at the
// serialized original WorldRemove entry. An earlier order whose factory
// return pointer is still unknown cannot establish it in advance. Optional
// diagnostics. Not required for verify_forwarding or ItemPath authorization.
struct ObjectEvidence {
  bool short_path_verified=false;Issue issue=Issue::none;
  std::uint32_t object=0,symbol=0,device_nodes=0,bucket_nodes=0;
  lhiver::Digest fingerprint{};
};
namespace detail {
inline std::uint32_t pointer_hash(std::uint32_t p) noexcept {
  std::uint32_t h=0x811c9dc5;for(unsigned i=0;i<4;++i){h^=(p>>(i*8u))&255u;h*=0x1000193;}return h;
}
inline bool distinct(const Module& module,std::uint32_t candidate,std::uint32_t at,std::size_t n) noexcept {
  return outside(module,at,n)&&!zbug_end_compat::detail::overlaps(candidate,0x348,at,n);
}
inline bool record(const ReadOnlyMemory& m,const Module& module,std::uint32_t candidate,std::uint32_t address,
    void* data,std::size_t size,Sha256& hash) noexcept {
  return distinct(module,candidate,address,size)&&read_hash(m,address,data,size,hash);
}
inline bool field(const ReadOnlyMemory& m,const Module& module,std::uint32_t candidate,std::uint32_t address,
    std::uint32_t offset,std::uint32_t& value,Sha256& hash) noexcept {
  std::array<std::uint8_t,4> bytes{};if(address>UINT32_MAX-offset||!record(m,module,candidate,address+offset,bytes.data(),4,hash))return false;
  value=rs::detail::u32(bytes.data());return true;
}
}
class Reader {
  std::uint32_t owner_=0;bool reading_=false;
public:
  explicit Reader(std::uint32_t owner) noexcept:owner_(owner){}
  Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
  ObjectEvidence capture(const ReadOnlyMemory& memory,const Module& module,const ObjectRequest& request) noexcept {
    ObjectEvidence out;out.object=request.object;out.symbol=static_cast<std::uint32_t>(request.symbol);
    if(!owner_||request.boundary.current_thread!=owner_||request.boundary.key.thread!=owner_){out.issue=Issue::thread;return out;}
    if(reading_){out.issue=Issue::reentry;return out;}
    if(!bow_item_path::detail::boundary(request.boundary)||!detail::outside(module,request.object,0x348)||request.symbol<0){out.issue=Issue::boundary;return out;}
    reading_=true;struct Lease{bool& value;~Lease(){value=false;}}lease{reading_};
    const auto before=verify_code(memory,module);if(!before.verified){out.issue=before.issue;return out;}
    lhiver::Digest first{};
    for(unsigned pass=0;pass<2;++pass){
      Sha256 hash;std::array<std::uint32_t,6> item{};
      std::array<std::uint32_t,6> context{};
      if(!detail::word(memory,selection_abi::game_pointer,0,context[0])||
         !detail::word(memory,context[0],0,context[1])||context[1]!=0x83c28c||
         !detail::word(memory,context[0],8,context[2])||context[2]!=request.boundary.key.world||
         !detail::word(memory,context[2],0,context[3])||context[3]!=0x83e174||
         !detail::word(memory,station_abi::hero_pointer,0,context[4])||context[4]!=request.boundary.key.hero||
         !detail::word(memory,context[4],0x964,context[5])||context[5]!=request.boundary.key.station){out.issue=Issue::boundary;return out;}
      for(const auto v:context)detail::word_hash(hash,v);
      // +B8 is homeWorld, +C8 the Visual pointer. Both are here separately
      // positive zero; an unknown field is never treated as empty.
      constexpr std::array<std::uint32_t,6> offsets{{0,4,0xb8,0xc8,0x330,0x340}};
      for(std::size_t i=0;i<offsets.size();++i)if(!detail::word(memory,request.object,offsets[i],item[i])){out.issue=Issue::unreadable;return out;}
      if(item[0]!=hand_detail::item_vtable||!item[1]||item[1]>INT32_MAX||item[2]||item[3]||item[4]!=static_cast<std::uint32_t>(request.symbol)||item[5]){
        out.issue=Issue::constructor;return out;}
      for(const auto v:item)detail::word_hash(hash,v);
      std::uint32_t world=0,device=0,vptr=0,slot=0;
      if(!detail::word(memory,module.base,0xea6ac,world)||!detail::word(memory,module.base,0xea6a0,device)||
         !detail::field(memory,module,request.object,device,0,vptr,hash)||vptr!=module.base+0xcc7dc||
         !detail::word(memory,vptr,0xbc,slot)||slot!=module.base+0x35e40||
         !detail::distinct(module,request.object,world,0x594)){out.issue=Issue::dispatch;return out;}
      for(const auto v:{world,device,vptr,slot})detail::word_hash(hash,v);
      // DeviceOnVobRemoved: references +100/+FC/+110/+10C may trigger UI calls.
      std::uint32_t ui=0;
      if(!detail::field(memory,module,request.object,device,0x204,ui,hash)){out.issue=Issue::unreadable;return out;}
      if(ui){std::uint32_t controls=0;if(!detail::field(memory,module,request.object,ui,0x38,controls,hash)){out.issue=Issue::unreadable;return out;}
        for(const auto offset:{0x100u,0xfcu,0x110u,0x10cu}){std::uint32_t ref=0,target=0;
          if(!detail::field(memory,module,request.object,controls,offset,ref,hash)||
             (ref&&!detail::field(memory,module,request.object,ref,8,target,hash))){out.issue=Issue::unreadable;return out;}
          if(target==request.object){out.issue=Issue::alias;return out;}}
      }
      // Device list: only the no-match path avoids its delete/free call.
      std::uint32_t head=0,count=0;
      if(!detail::field(memory,module,request.object,device,0x10c,head,hash)||
         !detail::field(memory,module,request.object,device,0x110,count,hash)||count>65536){out.issue=Issue::list;return out;}
      std::array<std::uint8_t,8> sentinel{};if(!detail::record(memory,module,request.object,head,sentinel.data(),sentinel.size(),hash)){out.issue=Issue::list;return out;}
      auto node=rs::detail::u32(sentinel.data()),previous=head;std::uint32_t nodes=0;
      while(node!=head){
        if(nodes>=count){out.issue=Issue::list;return out;}
        std::array<std::uint8_t,12> links{};std::uint32_t target=0;
        if(!detail::record(memory,module,request.object,node,links.data(),links.size(),hash)||rs::detail::u32(links.data()+4)!=previous||
           !detail::field(memory,module,request.object,rs::detail::u32(links.data()+8),0,target,hash)){out.issue=Issue::list;return out;}
        if(target==request.object){out.issue=Issue::alias;return out;}
        previous=node;node=rs::detail::u32(links.data());++nodes;
      }
      if(nodes!=count||previous!=rs::detail::u32(sentinel.data()+4)){out.issue=Issue::list;return out;}out.device_nodes=nodes;
      // Exact MSVC hash lookup for this pointer, not an invented empty map.
      std::array<std::uint8_t,32> map{};
      if(!detail::record(memory,module,request.object,world+0x574,map.data(),map.size(),hash)){out.issue=Issue::list;return out;}
      const auto end=rs::detail::u32(map.data()+4),size=rs::detail::u32(map.data()+8),buckets=rs::detail::u32(map.data()+12);
      const auto finish=rs::detail::u32(map.data()+16),capacity=rs::detail::u32(map.data()+20),mask=rs::detail::u32(map.data()+24),number=rs::detail::u32(map.data()+28);
      if(!detail::distinct(module,request.object,end,12)||size>65536||!number||number>131072||number!=mask+1u||(number&(number-1u))||
         !detail::distinct(module,request.object,buckets,std::size_t{number}*8)||finish!=buckets+number*8||capacity<finish){out.issue=Issue::list;return out;}
      const auto bucket=detail::pointer_hash(request.object)&mask;std::array<std::uint8_t,8> pair{};
      if(!detail::record(memory,module,request.object,buckets+bucket*8,pair.data(),pair.size(),hash)){out.issue=Issue::list;return out;}
      const auto first_node=rs::detail::u32(pair.data());node=rs::detail::u32(pair.data()+4);nodes=0;previous=0;
      if((first_node==end)!=(node==end)){out.issue=Issue::list;return out;}
      if(node!=end)for(;;){
        if(nodes>=size){out.issue=Issue::list;return out;}
        std::array<std::uint8_t,12> links{};
        if(!detail::record(memory,module,request.object,node,links.data(),links.size(),hash)||
           (previous&&rs::detail::u32(links.data())!=previous)){out.issue=Issue::list;return out;}
        const auto target=rs::detail::u32(links.data()+8);
        if(!target||(detail::pointer_hash(target)&mask)!=bucket){out.issue=Issue::list;return out;}
        if(target==request.object){out.issue=Issue::alias;return out;}
        ++nodes;if(node==first_node)break;previous=node;node=rs::detail::u32(links.data()+4);
      }
      out.bucket_nodes=nodes;
      const auto now=hash.finish();if(pass&&now!=first){out.issue=Issue::changed;return out;}first=now;
    }
    const auto after=verify_code(memory,module);if(!after.verified||after.trampolines!=before.trampolines){out.issue=Issue::changed;return out;}
    out.short_path_verified=true;out.fingerprint=first;return out;
  }
};
} // namespace ubc::bow_item_renderer

