// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/stations/saw/ammo_presentation.hpp"
#include "ubc/engine/runtime_script.hpp"
#include "ubc/engine/selection_abi.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Only limited read-only x86 captures. No engine calls, hooks, strings
// outward, allocations, queue manipulation, or assumption of a live proof.
namespace ubc::ammo_runtime {
namespace presentation = ammo_presentation;
namespace script = runtime_script;
inline constexpr std::uint32_t external_entry=0x6e3070,external_symbol=22;
inline constexpr std::uint32_t hero_pointer=0xab2684;
inline constexpr std::uint32_t message_size=0x98,message_subtype=0x24,message_text=0x44,
  message_font=0x58,message_target=0x6c,message_seconds=0x88,message_x=0x90,message_y=0x94;
inline constexpr std::uint32_t manager_owner=0x3c,npc_manager=0xfc,manager_vtable=0x83e23c,
  manager_on_message_slot=0x28,manager_on_message=0x786380;
inline constexpr std::uint32_t deleting_destructor_slot=0x0c,deleting_destructor=0x769cd0;
inline constexpr std::uint32_t external_return_stack_offset=0x5c;
inline constexpr std::size_t hook_count=3,max_code_range=1024;

enum class Issue {
  none,context,scope,range,unreadable,changed,alias,message_class,string_bounds,
  string_terminator,owner,parser,pc,external_signature,external_name,
  external_registration,hook_proof,hook_patch,trampoline,code_hash,vtable
};
struct Verification { bool valid=false;Issue issue=Issue::none; };
template<class T> struct Read { T value{};bool valid=false;Issue issue=Issue::none; };
struct HookSite {
  std::uintptr_t address;
  std::size_t patch_size,guard_size;
  std::array<std::uint8_t,32> original;
};
// Whole instructions without relative operands. guard_size == patch_size:
// the additional residual contract are the full area hashes below.
inline constexpr std::array<HookSite,hook_count> hook_sites{{
  {0x6e320a,6,6,{{0x8b,0x0d,0x84,0x26,0xab,0x00}}},
  {0x759270,7,7,{{0x6a,0xff,0x68,0x00,0x65,0x82,0x00}}},
  {0x76a4a0,6,6,{{0x53,0x33,0xdb,0x56,0x8b,0xf1}}}
}};
struct InstalledHook {
  // Only from the own successfully installed, until process-end
  // living Registry. No scan, no foreign JMP and no RAM-derived
  // "probable" target address may produce this origin evidence.
  bool installed=false;
  std::uint32_t destination=0,trampoline=0;
};
struct InstalledHooks { std::array<InstalledHook,hook_count> hooks{}; };
struct CodeRange {std::uint32_t address,length;lhiver::Digest sha256;};
inline constexpr std::array<CodeRange,8> code_ranges{{
  {0x6e3070,565,lhiver::digest_literal("91e1e8c18046556061780fd0d7e7dbe6318cd4d78b38028d6da7d1a24230f97c")},
  {0x6d4a0e,56,lhiver::digest_literal("22cf63bf6b0fc6557d234f2835e3badb71f786665e43c3619c75d0f04ae8b145")},
  {0x759270,699,lhiver::digest_literal("659636a72c0e38fc4eda42a1ad8df65302130c03da1449764e764457655f083e")},
  {0x76a4a0,208,lhiver::digest_literal("e61a5b02e259607203c2632fd2fc5ab87c022967f13004e407ef17c6009e617e")},
  {0x76ab00,85,lhiver::digest_literal("5b875644c9fee9e481ae4be6c98dcb0bd212fdb7715b9a96195979c9032f6d6a")},
  {0x76af00,80,lhiver::digest_literal("efc4ea09a9a9b4dab6520c3e90d27c07379de73e5dd34db9b8e8dfaae0434b75")},
  {0x76af50,224,lhiver::digest_literal("f15d13858a3977db4469980d1f1890766f49f10caf419a3bef8b917849c2f6a3")},
  {0x769cd0,41,lhiver::digest_literal("fe693583ad7524403146aa023c668e2b803b7adbfc0de0a33ecaa5112d6e3ca0")}
}};
struct Payload { presentation::Payload payload{};std::uint32_t target=0; };
struct CaptureSite { std::uint32_t thread=0,parser=0,message=0,event_manager=0,stack=0; };
struct PrintSite { std::uint32_t thread=0,receiver=0,message=0,caller=0; };
struct DestructorSite { std::uint32_t thread=0,message=0,caller=0; };

namespace detail {
inline std::uint32_t u32(const std::uint8_t* data) noexcept {return script::detail::u32(data);}
inline std::uint32_t u16(const std::uint8_t* data) noexcept {
  return static_cast<std::uint32_t>(data[0])|(static_cast<std::uint32_t>(data[1])<<8u);
}
inline bool range(std::uint64_t address,std::size_t size) noexcept {return script::detail::range(address,size);}
inline bool overlaps(std::uint32_t a,std::size_t an,std::uint32_t b,std::size_t bn) noexcept {
  return std::uint64_t{a}<std::uint64_t{b}+bn&&std::uint64_t{b}<std::uint64_t{a}+an;
}
inline bool read_u32(const ReadOnlyMemory& memory,std::uint32_t base,std::uint32_t offset,std::uint32_t& value) noexcept {
  std::array<std::uint8_t,4> data{};const auto address=std::uint64_t{base}+offset;
  if(!range(address,data.size())||!memory.copy(static_cast<std::uint32_t>(address),data.data(),data.size()))return false;
  value=u32(data.data());return true;
}
inline lhiver::Digest digest(const void* data,std::size_t size) noexcept {Sha256 hash;hash.update(data,size);return hash.finish();}
inline bool live_context(const ReadOnlyMemory& memory,const presentation::Context& context) noexcept {
  if(!presentation::valid(context))return false;
  for(unsigned pass=0;pass<2;++pass){
    std::uint32_t game=0,type=0,world=0,hero=0;
    if(!read_u32(memory,selection_abi::game_pointer,0,game)||game!=context.game||
      !read_u32(memory,game,0,type)||type!=selection_abi::game_vtable||
      !read_u32(memory,game,selection_abi::game_world,world)||world!=context.world||
      !read_u32(memory,hero_pointer,0,hero)||hero!=context.hero)return false;
  }
  return true;
}
inline void jump(std::uint8_t* bytes,std::uint32_t from,std::uint32_t to) noexcept {
  bytes[0]=0xe9;const auto delta=to-(from+5u);
  for(unsigned i=0;i<4;++i)bytes[i+1]=static_cast<std::uint8_t>(delta>>(8u*i));
}
inline bool outside_engine(std::uint32_t address,std::size_t size) noexcept {
  return range(address,size)&&!overlaps(address,size,0x400000,0x752000);
}
inline Verification hook_proof(const InstalledHooks& proof) noexcept {
  for(std::size_t i=0;i<hook_count;++i){
    const auto& value=proof.hooks[i];const auto length=hook_sites[i].patch_size+5;
    if(!value.installed||!outside_engine(value.destination,1)||!outside_engine(value.trampoline,length))return {false,Issue::hook_proof};
    for(std::size_t j=0;j<hook_count;++j){
      const auto& other=proof.hooks[j];
      if(overlaps(value.destination,1,other.trampoline,hook_sites[j].patch_size+5))return {false,Issue::alias};
      if(i!=j&&(value.destination==other.destination||overlaps(value.trampoline,length,other.trampoline,hook_sites[j].patch_size+5)))
        return {false,Issue::alias};
    }
  }
  return {true,Issue::none};
}
inline Verification hooks_once(const ReadOnlyMemory& memory,const InstalledHooks& proof) noexcept {
  const auto provenance=hook_proof(proof);if(!provenance.valid)return provenance;
  for(std::size_t i=0;i<hook_count;++i){
    const auto& site=hook_sites[i];const auto address=static_cast<std::uint32_t>(site.address);const auto& own=proof.hooks[i];
    std::array<std::uint8_t,32> actual{},expected{};expected.fill(0x90);
    jump(expected.data(),address,own.destination);
    if(!memory.copy(address,actual.data(),site.patch_size))return {false,Issue::unreadable};
    if(std::memcmp(actual.data(),expected.data(),site.patch_size))return {false,Issue::hook_patch};
    std::memcpy(expected.data(),site.original.data(),site.patch_size);
    jump(expected.data()+site.patch_size,own.trampoline+static_cast<std::uint32_t>(site.patch_size),address+static_cast<std::uint32_t>(site.patch_size));
    if(!memory.copy(own.trampoline,actual.data(),site.patch_size+5))return {false,Issue::unreadable};
    if(std::memcmp(actual.data(),expected.data(),site.patch_size+5))return {false,Issue::trampoline};
  }
  return {true,Issue::none};
}
// Even in tests only a generic hash mechanism. The public
// Native verifiers below use exclusively fixed code_ranges.
inline Verification code_range(const ReadOnlyMemory& memory,const CodeRange& wanted,const InstalledHooks* proof) noexcept {
  if(!wanted.length||wanted.length>max_code_range||!range(wanted.address,wanted.length))return {false,Issue::range};
  std::array<std::uint8_t,max_code_range> first{},second{};
  if(!memory.copy(wanted.address,first.data(),wanted.length)||!memory.copy(wanted.address,second.data(),wanted.length))return {false,Issue::unreadable};
  if(std::memcmp(first.data(),second.data(),wanted.length))return {false,Issue::changed};
  if(proof){
    for(std::size_t i=0;i<hook_count;++i){
      const auto& site=hook_sites[i];const auto address=static_cast<std::uint32_t>(site.address);
      if(!overlaps(wanted.address,wanted.length,address,site.patch_size))continue;
      if(address<wanted.address||std::uint64_t{address}+site.patch_size>std::uint64_t{wanted.address}+wanted.length)return {false,Issue::range};
      std::array<std::uint8_t,32> expected{};expected.fill(0x90);jump(expected.data(),address,proof->hooks[i].destination);
      const auto offset=address-wanted.address;
      if(std::memcmp(first.data()+offset,expected.data(),site.patch_size))return {false,Issue::hook_patch};
      std::memcpy(first.data()+offset,site.original.data(),site.patch_size);
    }
  }
  return digest(first.data(),wanted.length)==wanted.sha256?Verification{true,Issue::none}:Verification{false,Issue::code_hash};
}
struct RawPayload {
  std::array<std::uint8_t,message_size> object{};
  std::array<std::uint8_t,presentation::string_read_bound> text{},font{};
  std::uint32_t text_data=0,font_data=0,text_length=0,font_length=0;
};
inline Issue payload_once(const ReadOnlyMemory& memory,std::uint32_t message,RawPayload& value) noexcept {
  if(message%4||!range(message,value.object.size()))return Issue::range;
  if(!memory.copy(message,value.object.data(),value.object.size()))return Issue::unreadable;
  if(u32(value.object.data())!=presentation::conversation_vtable||u16(value.object.data()+message_subtype)!=21)return Issue::message_class;
  value.text_data=u32(value.object.data()+message_text+8);value.text_length=u32(value.object.data()+message_text+12);
  value.font_data=u32(value.object.data()+message_font+8);value.font_length=u32(value.object.data()+message_font+12);
  if(!value.text_length||value.text_length>=value.text.size()||!value.font_length||value.font_length>=value.font.size())return Issue::string_bounds;
  const auto text_size=static_cast<std::size_t>(value.text_length)+1,font_size=static_cast<std::size_t>(value.font_length)+1;
  if(!range(value.text_data,text_size)||!range(value.font_data,font_size))return Issue::range;
  if(overlaps(value.text_data,text_size,message,value.object.size())||overlaps(value.font_data,font_size,message,value.object.size())||
    overlaps(value.text_data,text_size,value.font_data,font_size))return Issue::alias;
  if(!memory.copy(value.text_data,value.text.data(),text_size)||!memory.copy(value.font_data,value.font.data(),font_size))return Issue::unreadable;
  if(value.text[value.text_length]||value.font[value.font_length]||
    std::memchr(value.text.data(),0,value.text_length)||std::memchr(value.font.data(),0,value.font_length))return Issue::string_terminator;
  return Issue::none;
}
inline bool same_symbol(const script::detail::Symbol& a,const script::detail::Symbol& b) noexcept {
  return a.address==b.address&&a.name_data==b.name_data&&a.name_length==b.name_length&&a.data==b.data&&a.offset==b.offset&&
    a.elements==b.elements&&a.type==b.type&&a.flags==b.flags&&a.parent==b.parent;
}
inline Verification vtables(const ReadOnlyMemory& memory) noexcept {
  std::uint32_t first=0,second=0,owner_first=0,owner_second=0;
  if(!read_u32(memory,presentation::conversation_vtable,deleting_destructor_slot,first)||
    !read_u32(memory,manager_vtable,manager_on_message_slot,owner_first)||
    !read_u32(memory,presentation::conversation_vtable,deleting_destructor_slot,second)||
    !read_u32(memory,manager_vtable,manager_on_message_slot,owner_second))return {false,Issue::unreadable};
  if(first!=second||owner_first!=owner_second)return {false,Issue::changed};
  return first==deleting_destructor&&owner_first==manager_on_message?Verification{true,Issue::none}:Verification{false,Issue::vtable};
}
inline constexpr std::array<const char*,6> external_names{{"AI_PRINTSCREEN","AI_PRINTSCREEN.PAR0","AI_PRINTSCREEN.PAR1",
  "AI_PRINTSCREEN.PAR2","AI_PRINTSCREEN.PAR3","AI_PRINTSCREEN.PAR4"}};
inline constexpr std::array<std::uint32_t,6> external_types{{5,3,2,2,3,2}};
struct ExternalState {script::detail::Layout layout{};std::array<script::detail::Symbol,6> symbols{};};
inline Issue external_once(const ReadOnlyMemory& memory,std::uint32_t parser,ExternalState& value) noexcept {
  if(script::detail::read_layout(memory,parser,lhiver::profile,value.layout)!=script::Issue::none)return Issue::parser;
  for(std::size_t i=0;i<value.symbols.size();++i){
    auto& symbol=value.symbols[i];
    if(script::detail::lookup(memory,value.layout,external_symbol+static_cast<std::uint32_t>(i),symbol)!=script::Issue::none)return Issue::unreadable;
    if(script::detail::name_matches(memory,symbol,external_names[i])!=script::Issue::none)return Issue::external_name;
    if(symbol.type!=external_types[i]||symbol.flags!=(i?0u:11u)||symbol.elements!=(i?0u:5u)||symbol.parent)
      return Issue::external_signature;
    if(!i&&(symbol.data!=external_entry||symbol.offset!=2))return Issue::external_registration;
    for(std::size_t j=0;j<i;++j)if(overlaps(symbol.address,0x3c,value.symbols[j].address,0x3c)||
      overlaps(symbol.name_data,symbol.name_length+1,value.symbols[j].name_data,value.symbols[j].name_length+1))return Issue::alias;
  }
  return Issue::none;
}
inline bool same_external(const ExternalState& first,const ExternalState& second) noexcept {
  if(!script::detail::same_layout(first.layout,second.layout))return false;
  for(std::size_t i=0;i<first.symbols.size();++i)if(!same_symbol(first.symbols[i],second.symbols[i]))return false;
  return true;
}
struct CaptureFields {
  std::uint32_t target=0,manager_class=0,owner=0,reverse=0,on_message=0,pc=0,caller=0;
  script::detail::Layout layout{};
  std::array<std::uint8_t,5> instruction{};
};
inline Issue capture_fields_once(const ReadOnlyMemory& memory,const CaptureSite& site,const presentation::Context& context,CaptureFields& value) noexcept {
  if(!live_context(memory,context))return Issue::context;
  if(!range(site.event_manager,0x40)||!range(context.hero,0x100)||!range(site.stack,external_return_stack_offset+4)||
    !range(site.message,message_size))return Issue::range;
  if(overlaps(site.event_manager,0x40,context.hero,0x100)||overlaps(site.event_manager,0x40,site.message,message_size)||
    overlaps(context.hero,0x100,site.message,message_size))return Issue::alias;
  if(!read_u32(memory,hero_pointer,0,value.target)||!read_u32(memory,site.event_manager,0,value.manager_class)||
    !read_u32(memory,site.event_manager,manager_owner,value.owner)||!read_u32(memory,context.hero,npc_manager,value.reverse)||
    !read_u32(memory,manager_vtable,manager_on_message_slot,value.on_message)||
    !read_u32(memory,site.stack,external_return_stack_offset,value.caller))return Issue::unreadable;
  if(value.target!=context.hero||value.owner!=context.hero||value.reverse!=site.event_manager)return Issue::owner;
  if(value.manager_class!=manager_vtable||value.on_message!=manager_on_message)return Issue::vtable;
  if(value.caller!=presentation::external_dispatch_return)return Issue::external_registration;
  if(script::detail::read_layout(memory,site.parser,lhiver::profile,value.layout)!=script::Issue::none)return Issue::parser;
  std::uint32_t cursor=0;
  if(!read_u32(memory,site.parser,0x4c,cursor))return Issue::unreadable;
  if(cursor<value.layout.code||cursor>value.layout.code_end||cursor-value.layout.code<5)return Issue::pc;
  value.pc=cursor-value.layout.code;
  if(!memory.copy(cursor-5,value.instruction.data(),value.instruction.size()))return Issue::unreadable;
  // Daedalus BE is decimal 62 (0x3e), not the hex value 0xbe.
  if(value.instruction[0]!=0x3e||u32(value.instruction.data()+1)!=external_symbol)return Issue::pc;
  return Issue::none;
}
inline bool same_fields(const CaptureFields& a,const CaptureFields& b) noexcept {
  return a.target==b.target&&a.manager_class==b.manager_class&&a.owner==b.owner&&a.reverse==b.reverse&&a.on_message==b.on_message&&
    a.pc==b.pc&&a.caller==b.caller&&script::detail::same_layout(a.layout,b.layout)&&a.instruction==b.instruction;
}
} // namespace detail

inline Verification verify_original_code(const ReadOnlyMemory& memory) noexcept {
  auto result=detail::vtables(memory);if(!result.valid)return result;
  for(unsigned pass=0;pass<2;++pass)for(const auto& range:code_ranges){result=detail::code_range(memory,range,nullptr);if(!result.valid)return result;}
  return detail::vtables(memory);
}
inline Verification verify_installed_code(const ReadOnlyMemory& memory,const InstalledHooks& proof) noexcept {
  auto result=detail::hooks_once(memory,proof);if(!result.valid)return result;
  result=detail::vtables(memory);if(!result.valid)return result;
  for(unsigned pass=0;pass<2;++pass)for(const auto& range:code_ranges){result=detail::code_range(memory,range,&proof);if(!result.valid)return result;}
  result=detail::vtables(memory);if(!result.valid)return result;
  return detail::hooks_once(memory,proof);
}
inline Verification verify_external(const ReadOnlyMemory& memory,std::uint32_t parser) noexcept {
  detail::ExternalState first{},second{};
  auto issue=detail::external_once(memory,parser,first);if(issue!=Issue::none)return {false,issue};
  issue=detail::external_once(memory,parser,second);if(issue!=Issue::none)return {false,issue};
  if(!detail::same_external(first,second))return {false,Issue::changed};
  return {true,Issue::none};
}
inline Verification verify_contract(const ReadOnlyMemory& memory,std::uint32_t parser,const InstalledHooks& proof) noexcept {
  const auto code=verify_installed_code(memory,proof);if(!code.valid)return code;
  return verify_external(memory,parser);
}
inline Read<Payload> read_payload(const ReadOnlyMemory& memory,std::uint32_t message) noexcept {
  Read<Payload> out;detail::RawPayload first{},second{};
  out.issue=detail::payload_once(memory,message,first);if(out.issue!=Issue::none)return out;
  out.issue=detail::payload_once(memory,message,second);if(out.issue!=Issue::none)return out;
  if(first.object!=second.object||first.text!=second.text||first.font!=second.font){out.issue=Issue::changed;return out;}
  auto& value=out.value.payload;value.known=true;value.vtable=detail::u32(first.object.data());value.subtype=detail::u16(first.object.data()+message_subtype);
  value.text={first.text_length,detail::digest(first.text.data(),first.text_length)};
  value.font={first.font_length,detail::digest(first.font.data(),first.font_length)};
  value.x=static_cast<std::int32_t>(detail::u32(first.object.data()+message_x));value.y=static_cast<std::int32_t>(detail::u32(first.object.data()+message_y));
  value.seconds_bits=detail::u32(first.object.data()+message_seconds);out.value.target=detail::u32(first.object.data()+message_target);
  out.valid=true;return out;
}

// Pure data capture for targeted tests/diagnosis. native_contract remains
// ALWAYS false. Only read_capture below links it with the own Registry.
inline Read<presentation::Capture> read_capture_data(const ReadOnlyMemory& memory,const presentation::Context& context,
  const presentation::Scope& scope,const CaptureSite& site) noexcept {
  Read<presentation::Capture> out;out.value.context=context;out.value.scope=scope;out.value.message=site.message;out.value.event_manager=site.event_manager;
  if(!presentation::valid(context)||site.thread!=context.thread||site.parser!=context.parser){out.issue=Issue::context;return out;}
  const auto* profile=presentation::find(scope.callback);
  if(!profile||!scope.active||!scope.patched_body_verified||scope.depth!=1||!scope.job||!scope.token){out.issue=Issue::scope;return out;}
  detail::CaptureFields first{},second{};
  out.issue=detail::capture_fields_once(memory,site,context,first);out.value.vm_pc=first.pc;out.value.caller=first.caller;out.value.target=first.target;
  if(out.issue!=Issue::none)return out;
  const auto payload=read_payload(memory,site.message);out.value.payload=payload.value.payload;
  if(!payload.valid){out.issue=payload.issue;return out;}
  if(payload.value.target!=context.hero){out.issue=Issue::owner;return out;}
  out.issue=detail::capture_fields_once(memory,site,context,second);if(out.issue!=Issue::none)return out;
  if(!detail::same_fields(first,second)){out.issue=Issue::changed;return out;}
  if(first.pc!=profile->success_pc){out.issue=Issue::pc;return out;}
  out.value.manager_owner_verified=true;out.valid=true;return out;
}
inline Read<presentation::Capture> read_capture(const ReadOnlyMemory& memory,const presentation::Context& context,
  const presentation::Scope& scope,const CaptureSite& site,const InstalledHooks& proof) noexcept {
  auto out=read_capture_data(memory,context,scope,site);if(!out.valid)return out;
  detail::ExternalState before{},final{};out.issue=detail::external_once(memory,context.parser,before);
  if(out.issue!=Issue::none){out.valid=false;return out;}
  const auto code=verify_installed_code(memory,proof);
  if(!code.valid){out.valid=false;out.issue=code.issue;return out;}
  // The full contract must not turn the Snapshot into an old evidence.
  const auto after=read_capture_data(memory,context,scope,site);
  if(!after.valid||!presentation::same(out.value.payload,after.value.payload)||out.value.target!=after.value.target||
    out.value.vm_pc!=after.value.vm_pc||out.value.caller!=after.value.caller){out.valid=false;out.issue=Issue::changed;return out;}
  out.issue=detail::external_once(memory,context.parser,final);
  if(out.issue!=Issue::none||!detail::same_external(before,final)){out.valid=false;if(out.issue==Issue::none)out.issue=Issue::changed;return out;}
  out.value.native_contract=true;return out;
}
inline Read<presentation::Display> read_display(const ReadOnlyMemory& memory,const presentation::Context& context,
  const PrintSite& site,const InstalledHooks& proof) noexcept {
  Read<presentation::Display> out;out.value.context=context;out.value.message=site.message;out.value.receiver=site.receiver;
  if(!presentation::valid(context)||site.thread!=context.thread||site.receiver!=context.hero||!detail::live_context(memory,context)){out.issue=Issue::context;return out;}
  const auto first=read_payload(memory,site.message);out.value.payload=first.value.payload;if(!first.valid){out.issue=first.issue;return out;}
  std::uint32_t before=0,after=0;
  if(!detail::read_u32(memory,hero_pointer,0,before)||before!=context.hero||first.value.target!=context.hero){out.issue=Issue::owner;return out;}
  detail::ExternalState external_before{},external_after{};out.issue=detail::external_once(memory,context.parser,external_before);
  if(out.issue!=Issue::none)return out;
  const auto code=verify_installed_code(memory,proof);if(!code.valid){out.issue=code.issue;return out;}
  const auto second=read_payload(memory,site.message);
  if(!detail::live_context(memory,context)||!detail::read_u32(memory,hero_pointer,0,after)||before!=after||!second.valid||second.value.target!=first.value.target||
    !presentation::same(first.value.payload,second.value.payload)){out.issue=Issue::changed;return out;}
  out.issue=detail::external_once(memory,context.parser,external_after);
  if(out.issue!=Issue::none||!detail::same_external(external_before,external_after)){if(out.issue==Issue::none)out.issue=Issue::changed;return out;}
  out.value.native_contract=true;out.valid=true;return out;
}
inline Read<presentation::Destruction> read_destruction(const ReadOnlyMemory& memory,const presentation::Context& context,
  const DestructorSite& site,const InstalledHooks& proof) noexcept {
  // Entry before the first original instruction: the Destructor has neither
  // target nor strings deleted. Later captures are explicitly invalid.
  const auto display=read_display(memory,context,{site.thread,context.hero,site.message,site.caller},proof);
  return {{display.value.context,display.value.message,display.value.payload,display.value.native_contract},display.valid,display.issue};
}
} // namespace ubc::ammo_runtime
