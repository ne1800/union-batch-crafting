// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/runtime_script.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Tighter read contract for the 2026-11-01 evidenced message chain, supplemented
// by the 2026-11-14 statically evidenced zBugFixes type comparison. It
// applies ONLY to the already separately bound oCMsgConversation/Print21.
// No generic hook scanner, no call of foreign pointers, no writes. The
// integrator must re-check immediately before use on its own game thread
// and continue to bind Payload, Scope, Hero, and EventManager. Stable
// double reads are not proof of synchronization against foreign threads.
namespace ubc::vob_locator_compat {
inline constexpr std::uint32_t profile_version=2,engine_base=0x400000,
  engine_size=0x752000,preferred_base=0x10000000,manager_slot=0x83e264,
  original_on_message=0x786380,engine_dynamic_cast=0x7d0a3a;
inline constexpr std::size_t module_count=5,max_region=1024,max_relocations=32;
enum class Module : std::size_t {vob,bug,stats,union_core,torch,engine};
struct Modules {std::array<std::uint32_t,module_count> bases{};};
enum class Issue {none,modules,alias,unreadable,changed,header,code,transfer,chain,filter,type_contract};
struct Verification {
  bool valid=false;Issue issue=Issue::none;std::uint32_t stage=0,on_message=0,version=0;
};
struct ModuleProfile {
  const char* name;std::uint32_t image_size,header_length,image_base_offset;
  lhiver::Digest file_sha256,header_sha256;
};
struct Region {
  Module module;std::uint32_t rva,length;lhiver::Digest sha256;
  std::array<std::uint16_t,max_relocations> relocations{};std::uint32_t relocation_count=0;
};
struct CanonicalType {Module module;std::uint32_t rva,engine_rva,length;lhiver::Digest sha256;};
struct TypeCompareProfile {
  Module module;std::uint32_t entry,suffix_length,suffix_pointer,suffix;
};
// Two statically evidenced adapters of the same tight RTTI spot. The
// selection comes exclusively from the fresh CALL, never from load order.
inline constexpr std::array<TypeCompareProfile,2> type_compare_profiles{{
  {Module::torch,0x8e30,0x8d9ec,0x8da10,0x7c6b0},
  {Module::bug,0x9ba0,0x5e85c,0x5e8d4,0x506c0},
}};

inline constexpr std::array<ModuleProfile,module_count> module_profiles{{
  {"UNION_VOB_LOCATOR.DLL",0xc8000,1024,332,
    lhiver::digest_literal("72fc040ffe3d9ee6995493d4e5879336bbccf2f2301f5daa4578ff8f31b234fc"),lhiver::digest_literal("6408c04fdfbeacd99302c1f1a2986c92b77ab144a2579d2dfbdae9c2c1ff5ac7")},
  {"ZBUGFIXES.DLL",0x67000,1024,316,
    lhiver::digest_literal("50c3dc669a451dc084c4691b6e6fb7a39838d40627629e59e884579873811963"),lhiver::digest_literal("dc1d80b431ffc0928f8bb738af042d470b4812d1dcd36a1d9212c4531e465a8f")},
  {"UNION_EXT_STAT.DLL",0xb8000,1024,340,
    lhiver::digest_literal("13913f7e3f79c053a637eb1c193e85cbaa7eca8e8575e0b31fa54d3670589b1e"),lhiver::digest_literal("4da87220b6e4db74137df5ebe1571b9b550a6f70ebb48f99cb105bf8da5421c0")},
  {"shw32.dll",0x438000,1024,332,
    lhiver::digest_literal("78d9312f07ebdf978d301fde63841ddcbe265b59cb7c2753e01d2dbdc5742888"),lhiver::digest_literal("3162af3c0d1375092c2c0219c9f2506f61803ad5d8b203414404c82fbc473960")},
  {"ZTORCHCONTROL.DLL",0x98000,1024,324,
    lhiver::digest_literal("6d3d2ba90030a84a89e4996a049f4196531a3185f10545dc60041cc8b710a139"),lhiver::digest_literal("cf4481de22a3abbccce491bca27558b9bd05b2818e7406a70778beb9803628dd")},
}};
inline constexpr std::array<Region,37> regions{{
  {Module::vob,0x50fb0,408,lhiver::digest_literal("e7d74dc7ef27b91af2fdf5bf75117d52fe66f748ac60e4f275bac4ee50cb1d51"),
    {{6,21,72,130,135,214,219,254,298,320,342,354,364}},13}, // OnMessage
  {Module::vob,0x94d0,13,lhiver::digest_literal("8dd71ded5c308446b6853a8551409f3b59c8646b553efe75572d0b9b94cdffd4"),
    {{}},0}, // ClassDefThunk
  {Module::vob,0x55bb8,17,lhiver::digest_literal("bdf2fd9c3e17a4b3a18d8abf8638907fa3d3eb89b2d7b1b1dd05e45fe1b4cfce"),
    {{2}},1}, // StackCookie
  {Module::bug,0x12410,429,lhiver::digest_literal("a81284440af3cec03bb169ca0f82958efc88730f5897bb0e6aa7f5b063110dc7"),
    {{8,26,161,166,225,230,275,285,310,317,329,338,348,390,398}},15}, // OnMessage
  {Module::stats,0x1bc00,571,lhiver::digest_literal("6ad02b331082b95191b0d126b5fc2823ed569e1016ae6ed70861590124b670e2"),
    {{6,21,126,131,209,214,279,286,292,336,343,349,416,429,455,477,517,527}},18}, // OnMessage
  {Module::stats,0x5bf0,13,lhiver::digest_literal("8dd71ded5c308446b6853a8551409f3b59c8646b553efe75572d0b9b94cdffd4"),
    {{}},0}, // ClassDefThunk
  {Module::stats,0x3e15d,14,lhiver::digest_literal("57a04a79853d7cfed6f1f9d04290a1e2c1b54263b43cea473acd20ea7730d41c"),
    {{2}},1}, // StackCookie
  {Module::union_core,0x31aa0,15,lhiver::digest_literal("00ad9d87537091652919476c1beb3470eab23b0a59ab9f9cf332434f3e53abc3"),
    {{}},0}, // GetReturnAddress
  {Module::torch,0x8e30,355,lhiver::digest_literal("286a1d6cd43acccdd3785586741cdf5b718de19b953aca0072ee649e52e6062b"),
    {{7,40,59,128,150,168,186,255,277}},9}, // TypeCompare
  {Module::torch,0x4dfa5,14,lhiver::digest_literal("bfc60721a6cbcd7a434efbe0bf55d9850d2e55af1f6c406b4eae1b88b6ac9ac9"),
    {{2}},1}, // StackCookie
  {Module::torch,0x7c6b0,19,lhiver::digest_literal("11b9f78ab98881068ba350f2b772a2f0b1ed5bddccfea30b51276cfa6145d9fc"),
    {{}},0}, // NamespaceSuffix
  {Module::engine,0x76e30,36,lhiver::digest_literal("64ebe2f50b9a4cd8d54fdf030ea24d07fd0781064c7f4f31ec41b0fa44243760"),
    {{}},0}, // CheckInheritance
  {Module::engine,0x364ab0,6,lhiver::digest_literal("039a4767e38d0f38750c3e7095563f1907087dd7fb2898362f5bbb0f9501ef22"),
    {{1}},1}, // ConversationClass
  {Module::engine,0x386380,1024,lhiver::digest_literal("4e3cd4f9dacf930f8a0aaf28997d037b6e5d14d29e2be8a6aad3f633642cec6f"),
    {{3,45,77,167,189,229,239,261,347,370,463,484,560,679,764,783,822}},17}, // OnMessage0
  {Module::engine,0x386780,1024,lhiver::digest_literal("8f850eab6ecd0a6d2dce3c9f24bd160baaf4ce734195f28f725aa34bb34319ac"),
    {{486,516,597,609,623,658,679,717,736,759}},10}, // OnMessage1
  {Module::engine,0x386b80,223,lhiver::digest_literal("bdc7ea9d1d865fee35b4a27d8245e410ed50011104ffd2126cdff1e4e21f750f"),
    {{3,8,75,81,94,130,156}},7}, // OnMessage2
  {Module::engine,0x43de08,8,lhiver::digest_literal("cfe88646f1a06ef425f5dd170e70f0c954db12801d111ed50d37e0d599e3745c"),
    {{0,4}},2}, // ConversationCOLAndGetter
  {Module::engine,0x44b108,20,lhiver::digest_literal("f10588afb4f7c284eb890caca5792067d980b10210b0002e0d3117d6dd128dda"),
    {{12,16}},2}, // COL
  {Module::engine,0x44b0f8,16,lhiver::digest_literal("928bcf168b8759c7b6798802b4a9f421aa65b069cf7bbccceefacc896acf1f3c"),
    {{12}},1}, // Hierarchy
  {Module::engine,0x44b0e4,16,lhiver::digest_literal("5d2d58111ffb12682d9276c9d9f02ef4eefc5381b91d97c5927796ff0a2a0780"),
    {{0,4,8,12}},4}, // BaseArray
  {Module::engine,0x44b0cc,24,lhiver::digest_literal("b76d27c8c877dd99231492a85b7b8f1320dfbc4edf73f48ce086bbd608409d6c"),
    {{0}},1}, // ConversationBase
  {Module::engine,0x44ae50,24,lhiver::digest_literal("eea127b836f63a26db9751830ed4685e1b2b0c155ff5bac751961f7f04a2d197"),
    {{0}},1}, // NpcMessageBase
  {Module::engine,0x444da0,24,lhiver::digest_literal("1de73553ef7c2cae0a74fb3ad940d863f270bfc007fccda2ea6cab473d2a297a"),
    {{0}},1}, // EventMessageBase
  {Module::engine,0x444d88,24,lhiver::digest_literal("4bfe063710d92f90654928eac0bea5551672e3d3ca65d03cfedc6e08d8921ec4"),
    {{0}},1}, // ObjectBase
  {Module::engine,0x3d0a3a,279,lhiver::digest_literal("e55285fb75065b7f5c5dc28134367f76b51074e2f0c34cf81f56d1b8a5dd2b55"),
    {{6,11,199,216,253,266}},6}, // DynamicCast
  {Module::engine,0x3d0ba2,26,lhiver::digest_literal("b206e75e4cc36ca3fc0f2591db372bf78ed149fb28c4d7f7ebb054cf1f30af49"),
    {{}},0}, // CompleteObject
  {Module::engine,0x3d0bbc,45,lhiver::digest_literal("19f6ac73b8301fe3ac9a76d5fdd2cd23f19198df67cb89d4de0d486a3b1e3beb"),
    {{}},0}, // FindSITargetPrefix
  {Module::engine,0x3d0bee,40,lhiver::digest_literal("caca36955cfc7f861dadb9cceda4b8a729e3de3a340d54ea947e2c8facb77058"),
    {{}},0}, // FindSITargetSuffix
  {Module::engine,0x4af8fc,24,lhiver::digest_literal("0f943fac7f6a90e8cc08e9874b814cbdd74925b1f2e0448bf58b9cd244e281cd"),
    {{}},0}, // EngineType0
  {Module::engine,0x4b9dbc,19,lhiver::digest_literal("ab9341b308bdeb50ba9725dd5f0c7b1a1e0ad4e179f9ff80fb90f0f5d65865cc"),
    {{}},0}, // EngineType1
  {Module::engine,0x48de00,21,lhiver::digest_literal("a4dd289cd6ac1c0b80def2a62e6dc29cd9b2e774e0ac351781c4e72857dbd73b"),
    {{}},0}, // EngineType2
  {Module::engine,0x48dde8,15,lhiver::digest_literal("63d537470215ef2b5ea9406b63ebf79f560c7a821313f84fdc7551f2119fb7e6"),
    {{}},0}, // EngineType3
  {Module::engine,0x4b77e8,15,lhiver::digest_literal("af13d85e1717375a3a0e3e67581141ce5dfb84c9b8ba7d1445eaa680edaf96e6"),
    {{}},0}, // Canonicalvob
  {Module::engine,0x4b9ec4,22,lhiver::digest_literal("970803ba0f6bf5950fca774067eca3911975c446ed05baa761b9963034598c8a"),
    {{}},0}, // Canonicalstats
  {Module::bug,0x9ba0,355,lhiver::digest_literal("9897fe4b1eb823c2f3cc7bde0704e378599814f0acc60a3b80822c12d6e31f13"),
    {{7,40,59,128,150,168,186,255,277}},9}, // TypeCompare, REA 2026-11-14
  {Module::bug,0x2297a,14,lhiver::digest_literal("e859e10cdacddac39e8ddf08e5a76670f975d08594f792e660fef72a5b18e1e5"),
    {{2}},1}, // StackCookie
  {Module::bug,0x506c0,19,lhiver::digest_literal("11b9f78ab98881068ba350f2b772a2f0b1ed5bddccfea30b51276cfa6145d9fc"),
    {{}},0}, // NamespaceSuffix
}};
inline constexpr std::array<CanonicalType,2> canonical_types{{
  {Module::vob,0x9405c,0x4b77e8,15,lhiver::digest_literal("af13d85e1717375a3a0e3e67581141ce5dfb84c9b8ba7d1445eaa680edaf96e6")},
  {Module::stats,0x87f48,0x4b9ec4,22,lhiver::digest_literal("970803ba0f6bf5950fca774067eca3911975c446ed05baa761b9963034598c8a")},
}};
inline constexpr std::array<std::size_t,4> engine_type_regions{{28,29,30,31}};

namespace detail {
inline std::uint32_t u32(const std::uint8_t* data) noexcept {return runtime_script::detail::u32(data);}
inline void put32(std::uint8_t* out,std::uint32_t value) noexcept {
  for(unsigned i=0;i<4;++i)out[i]=static_cast<std::uint8_t>(value>>(8u*i));
}
inline bool range(std::uint64_t address,std::size_t size) noexcept {return runtime_script::detail::range(address,size);}
inline bool overlap(std::uint32_t a,std::size_t an,std::uint32_t b,std::size_t bn) noexcept {
  return std::uint64_t{a}<std::uint64_t{b}+bn&&std::uint64_t{b}<std::uint64_t{a}+an;
}
inline lhiver::Digest digest(const void* data,std::size_t size) noexcept {Sha256 hash;hash.update(data,size);return hash.finish();}
inline std::uint32_t base(const Modules& modules,Module module) noexcept {
  return module==Module::engine?engine_base:modules.bases[static_cast<std::size_t>(module)];
}
inline Issue modules_valid(const Modules& modules) noexcept {
  for(std::size_t i=0;i<module_count;++i){
    const auto address=modules.bases[i],size=module_profiles[i].image_size;
    if(!address||address%0x10000u||!range(address,size))return Issue::modules;
    if(overlap(address,size,engine_base,engine_size))return Issue::alias;
    for(std::size_t j=0;j<i;++j)if(overlap(address,size,modules.bases[j],module_profiles[j].image_size))return Issue::alias;
  }
  return Issue::none;
}
inline Issue stable(const ReadOnlyMemory& memory,std::uint32_t address,void* out,std::size_t size) noexcept {
  if(!range(address,size)||size>max_region)return Issue::unreadable;
  std::array<std::uint8_t,max_region> other{};
  if(!memory.copy(address,out,size)||!memory.copy(address,other.data(),size))return Issue::unreadable;
  return std::memcmp(out,other.data(),size)?Issue::changed:Issue::none;
}
inline Issue word(const ReadOnlyMemory& memory,std::uint32_t address,std::uint32_t& value) noexcept {
  std::array<std::uint8_t,4> bytes{};const auto issue=stable(memory,address,bytes.data(),bytes.size());
  if(issue==Issue::none)value=u32(bytes.data());return issue;
}
inline Issue byte(const ReadOnlyMemory& memory,std::uint32_t address,std::uint32_t& value) noexcept {
  std::uint8_t data=0;const auto issue=stable(memory,address,&data,1);value=data;return issue;
}
inline Issue header(const ReadOnlyMemory& memory,std::uint32_t address,const ModuleProfile& profile) noexcept {
  std::array<std::uint8_t,max_region> bytes{};
  if(profile.header_length>bytes.size()||profile.image_base_offset+4>profile.header_length)return Issue::header;
  auto issue=stable(memory,address,bytes.data(),profile.header_length);if(issue!=Issue::none)return issue;
  const auto actual=u32(bytes.data()+profile.image_base_offset);
  // Windows preserves the preferred base; Wine writes the
  // actual one. No other PE-header discrepancy is normalized.
  if(actual!=preferred_base&&actual!=address)return Issue::header;
  put32(bytes.data()+profile.image_base_offset,preferred_base);
  return digest(bytes.data(),profile.header_length)==profile.header_sha256?Issue::none:Issue::header;
}
inline Issue region(const ReadOnlyMemory& memory,const Modules& modules,const Region& wanted) noexcept {
  const auto module_index=static_cast<std::size_t>(wanted.module);
  if(module_index>module_count||!wanted.length||wanted.length>max_region||wanted.relocation_count>max_relocations)return Issue::code;
  const auto module_size=wanted.module==Module::engine?engine_size:module_profiles[module_index].image_size;
  if(std::uint64_t{wanted.rva}+wanted.length>module_size)return Issue::code;
  std::array<std::uint8_t,max_region> bytes{};const auto module_base=base(modules,wanted.module);
  auto issue=stable(memory,module_base+wanted.rva,bytes.data(),wanted.length);if(issue!=Issue::none)return issue;
  for(std::size_t i=0;i<wanted.relocation_count;++i){
    const auto offset=wanted.relocations[i];
    if(std::uint32_t{offset}+4>wanted.length||
      (i&&offset<wanted.relocations[i-1]+4u))return Issue::code;
    const auto delta=wanted.module==Module::engine?0u:module_base-preferred_base;
    put32(bytes.data()+offset,u32(bytes.data()+offset)-delta);
  }
  return digest(bytes.data(),wanted.length)==wanted.sha256?Issue::none:Issue::code;
}
inline Issue transfer(const ReadOnlyMemory& memory,std::uint32_t address,std::uint8_t opcode,
  std::uint32_t destination,std::size_t length) noexcept {
  if(length<5||length>7)return Issue::transfer;
  std::array<std::uint8_t,7> actual{},expected{};expected.fill(0xcc);expected[0]=opcode;
  put32(expected.data()+1,destination-(address+5u));
  auto issue=stable(memory,address,actual.data(),length);if(issue!=Issue::none)return issue;
  return std::memcmp(actual.data(),expected.data(),length)?Issue::transfer:Issue::none;
}
inline Issue type_compare(const ReadOnlyMemory& memory,const Modules& modules,
  const TypeCompareProfile*& selected,std::uint32_t& destination) noexcept {
  selected=nullptr;destination=0;
  constexpr std::uint32_t site=0x7d0be9;
  std::array<std::uint8_t,5> bytes{};
  const auto issue=stable(memory,site,bytes.data(),bytes.size());if(issue!=Issue::none)return issue;
  if(bytes[0]!=0xe8)return Issue::transfer;
  const auto target=site+5u+u32(bytes.data()+1);
  for(const auto& profile:type_compare_profiles){
    if(target==base(modules,profile.module)+profile.entry){selected=&profile;destination=target;return Issue::none;}
  }
  return Issue::transfer;
}
inline bool heap_record(const Modules& modules,std::uint32_t pointer) noexcept {
  if(pointer%4||!range(pointer,0x1c)||overlap(pointer,0x1c,engine_base,engine_size))return false;
  for(std::size_t i=0;i<module_count;++i)if(overlap(pointer,0x1c,modules.bases[i],module_profiles[i].image_size))return false;
  return true;
}
struct Chain {
  std::array<std::uint32_t,3> records{},next{},fallback{},callback{},resolver{};
  std::array<std::uint32_t,4> parents{};
  std::uint32_t root=0,bug_enabled=0,bug_framework=0,bug_hook_type=0,suffix_length=0,suffix=0,type_compare=0;
};
inline bool same(const Chain& a,const Chain& b) noexcept {
  return a.records==b.records&&a.next==b.next&&a.fallback==b.fallback&&a.callback==b.callback&&
    a.resolver==b.resolver&&a.parents==b.parents&&a.root==b.root&&a.bug_enabled==b.bug_enabled&&
    a.bug_framework==b.bug_framework&&a.bug_hook_type==b.bug_hook_type&&a.suffix_length==b.suffix_length&&a.suffix==b.suffix&&
    a.type_compare==b.type_compare;
}
inline Issue chain(const ReadOnlyMemory& memory,const Modules& modules,Chain& out) noexcept {
  const auto v=base(modules,Module::vob),b=base(modules,Module::bug),s=base(modules,Module::stats),
    u=base(modules,Module::union_core);
  auto issue=word(memory,manager_slot,out.root);if(issue!=Issue::none)return issue;
  if(out.root!=v+0x50fb0)return Issue::chain;
  const std::array<std::uint32_t,3> record_slots{{v+0x958e8,b+0x5eaf0,s+0x8a2e8}},
    resolver_slots{{v+0x7b168,b+0x4515c,s+0x7017c}},callbacks{{v+0x50fb0,b+0x12410,s+0x1bc00}};
  for(std::size_t i=0;i<3;++i){
    issue=word(memory,record_slots[i],out.records[i]);if(issue!=Issue::none)return issue;
    if(!heap_record(modules,out.records[i]))return Issue::alias;
    for(std::size_t j=0;j<i;++j)if(overlap(out.records[i],0x1c,out.records[j],0x1c))return Issue::alias;
    issue=word(memory,resolver_slots[i],out.resolver[i]);if(issue!=Issue::none)return issue;
    if(out.resolver[i]!=u+0x31aa0)return Issue::chain;
  }
  for(std::size_t i=0;i<3;++i){
    issue=word(memory,out.records[i]+0x0c,out.next[i]);if(issue!=Issue::none)return issue;
    issue=word(memory,out.records[i]+0x14,out.fallback[i]);if(issue!=Issue::none)return issue;
    issue=word(memory,out.records[i]+0x18,out.callback[i]);if(issue!=Issue::none)return issue;
    if(out.next[i]!=(i<2?out.records[i+1]:0)||out.fallback[i]!=original_on_message||out.callback[i]!=callbacks[i])return Issue::chain;
  }
  issue=byte(memory,b+0x5dbc0,out.bug_enabled);if(issue!=Issue::none)return issue;
  issue=word(memory,b+0x5eb04,out.bug_framework);if(issue!=Issue::none)return issue;
  issue=byte(memory,b+0x5eaf8,out.bug_hook_type);if(issue!=Issue::none)return issue;
  // With an active filter, delete/queue paths are possible: no fallback profile.
  if(out.bug_enabled||out.bug_framework!=2||out.bug_hook_type!=2)return Issue::filter;
  const TypeCompareProfile* selected=nullptr;
  issue=type_compare(memory,modules,selected,out.type_compare);if(issue!=Issue::none)return issue;
  const auto compare_base=base(modules,selected->module);
  issue=word(memory,compare_base+selected->suffix_length,out.suffix_length);if(issue!=Issue::none)return issue;
  issue=word(memory,compare_base+selected->suffix_pointer,out.suffix);if(issue!=Issue::none)return issue;
  if(out.suffix_length!=18||out.suffix!=compare_base+selected->suffix)return Issue::type_contract;
  constexpr std::array<std::uint32_t,4> parent_slots{{0xab29bc,0xab2b9c,0x9a36d4,0x8d8d14}},
    expected_parents{{0xab2b60,0x9a3698,0x8d8cd8,0}};
  for(std::size_t i=0;i<4;++i){issue=word(memory,parent_slots[i],out.parents[i]);if(issue!=Issue::none)return issue;}
  return out.parents==expected_parents?Issue::none:Issue::type_contract;
}
inline Issue safe_type_name(const ReadOnlyMemory& memory,std::uint32_t address,std::size_t length,
  const lhiver::Digest& expected,const std::array<std::uint8_t,19>& suffix) noexcept {
  if(!length||length>64)return Issue::type_contract;
  std::array<std::uint8_t,64> name{};auto issue=stable(memory,address,name.data(),length);if(issue!=Issue::none)return issue;
  if(digest(name.data(),length)!=expected||name[length-1]||std::memchr(name.data(),0,length-1))return Issue::type_contract;
  const auto chars=length-1;
  // Exactly the mutating normalization branch of both inspected adapters.
  if(chars>18&&!std::memcmp(name.data()+chars-18,suffix.data(),19))return Issue::type_contract;
  return Issue::none;
}
inline Issue types(const ReadOnlyMemory& memory,const Modules& modules,const TypeCompareProfile& selected) noexcept {
  std::array<std::uint8_t,19> suffix{};
  auto issue=stable(memory,base(modules,selected.module)+selected.suffix,suffix.data(),suffix.size());if(issue!=Issue::none)return issue;
  if(digest(suffix.data(),suffix.size())!=regions[10].sha256||suffix.back()||
    std::memchr(suffix.data(),0,suffix.size()-1))return Issue::type_contract;
  for(const auto& type:canonical_types){
    issue=safe_type_name(memory,base(modules,type.module)+type.rva,type.length,type.sha256,suffix);if(issue!=Issue::none)return issue;
  }
  for(const auto index:engine_type_regions){
    const auto& type=regions[index];
    issue=safe_type_name(memory,engine_base+type.rva,type.length,type.sha256,suffix);if(issue!=Issue::none)return issue;
  }
  return Issue::none;
}
inline Verification static_contract(const ReadOnlyMemory& memory,const Modules& modules,std::uint32_t& compare) noexcept {
  for(std::size_t i=0;i<module_count;++i){const auto issue=header(memory,modules.bases[i],module_profiles[i]);
    if(issue!=Issue::none)return {false,issue,static_cast<std::uint32_t>(i)};}
  for(std::size_t i=0;i<regions.size();++i){const auto issue=region(memory,modules,regions[i]);
    if(issue!=Issue::none)return {false,issue,100u+static_cast<std::uint32_t>(i)};}
  // Three explicit, fully resolved foreign patches. No foreign
  // jump is masked or rewritten from its pre-existing target.
  auto issue=transfer(memory,base(modules,Module::vob)+0x5757c,0xe9,engine_dynamic_cast,7);
  if(issue!=Issue::none)return {false,issue,200};
  issue=transfer(memory,base(modules,Module::stats)+0x3fc07,0xe9,engine_dynamic_cast,7);
  if(issue!=Issue::none)return {false,issue,201};
  const TypeCompareProfile* selected=nullptr;
  issue=type_compare(memory,modules,selected,compare);
  if(issue!=Issue::none)return {false,issue,202};
  issue=types(memory,modules,*selected);if(issue!=Issue::none)return {false,issue,203};
  return {true,Issue::none};
}
} // namespace detail

// The file hash in the profile documents the derivation. This API
// checks the loaded PE header and the complete required code/data
// closure, not arbitrary unused bytes of the entire DLL file.
inline Verification verify_print_forwarding(const ReadOnlyMemory& memory,const Modules& modules) noexcept {
  auto issue=detail::modules_valid(modules);if(issue!=Issue::none)return {false,issue};
  std::uint32_t compare=0;
  auto result=detail::static_contract(memory,modules,compare);if(!result.valid)return result;
  detail::Chain before{},after{};issue=detail::chain(memory,modules,before);
  if(issue!=Issue::none)return {false,issue,300};
  if(before.type_compare!=compare)return {false,Issue::changed,204};
  result=detail::static_contract(memory,modules,compare);if(!result.valid)return result;
  if(before.type_compare!=compare)return {false,Issue::changed,204};
  issue=detail::chain(memory,modules,after);if(issue!=Issue::none)return {false,issue,301};
  if(!detail::same(before,after))return {false,Issue::changed,302};
  return {true,Issue::none,0,before.root,profile_version};
}
} // namespace ubc::vob_locator_compat
