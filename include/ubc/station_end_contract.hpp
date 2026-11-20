// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/crafting_catalog.hpp"
#include "ubc/sha256.hpp"
#include "ubc/station_abi.hpp"
#include "ubc/zbug_end_compat.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ubc::station_end_contract {
// Own narrow contract for the verified Gothic2.exe (SHA in station_abi).
// CheckStateChange calls after IsStanding()!=0 and state=state_target the
// virtual EndInteraction(NPC,0) at 0x7204B4; real return is 0x7204BA. The
// four Interact callers with Flag1 instead only start the exit animation.
inline constexpr std::uint32_t end_address=0x721950,completed_return=0x7204ba;
inline constexpr std::array<std::uint8_t,6> end_prefix{{0x64,0xa1,0,0,0,0}};
inline constexpr std::size_t max_code_range=2048;
struct InstalledEndHook {
  // From the own successfully installed, process-long-lived
  // hook registry. Never derive from a found JMP.
  bool installed=false;
  std::uint32_t destination=0,trampoline=0;
};
enum class PathIssue { none,range,unreadable,changed,hook_proof,hook_patch,trampoline,vtable,code_hash,compatibility };
struct PathVerification {bool valid=false;PathIssue issue=PathIssue::none;};
struct CodeRange {std::uint32_t address,length;lhiver::Digest sha256;};
inline constexpr std::array<CodeRange,8> code_ranges{{
  // CheckStateChange, EndInteraction, StopInteraction, OnMessage incl. jump
  // table, SetInteractMob, IsInteractingWith, IsStanding, SetDirection.
  {0x720440,1067,lhiver::digest_literal("2d7db06c049a7401ba949fcde5e861e025feaf1e2fedc76ae02327bdb813456c")},
  {0x721950,717,lhiver::digest_literal("b46a2907afcf4c1035e5237104ce43e2b1be94f787b010b92e4aef3f276b6750")},
  {0x721c20,731,lhiver::digest_literal("dca7a4072989d78d2a0b0101f4072d1ea225691f8ad9f79ca0ccb941a7618dba")},
  {0x71d950,176,lhiver::digest_literal("b6c34403c3de52d530304956ed25a2dc5c2be7dbccc1a95cfbb2dcd2a65edc28")},
  {0x74ab00,403,lhiver::digest_literal("dd9da50d362c9e7f7369ce1de3fb94dac8c255a5a932caf38d7119ba4311428e")},
  {0x721550,48,lhiver::digest_literal("d2537de2a181d49ec260e87d4702f0b5c8b7d46ee3ac1417a91f9d60e75cbe0e")},
  {0x6adee0,404,lhiver::digest_literal("71476a131c8417017665d7f706c2b65acc2cd7f3a9361d7f4892d04553af14f0")},
  {0x718cb0,16,lhiver::digest_literal("9be5cf66cc957d8f3ef3c0a52e329e62c5efb73eeb003c372f11765de1bddbbd")}
}};
// Original engine vtables with exactly these shared MobInter methods.
// This list alone identifies NO production profile. Derived/unknown
// classes remain excluded; strings/hand contract separately.
inline constexpr std::array<std::uint32_t,5> mob_vtables{{0x83c914,0x83ca3c,0x83d19c,0x83d2cc,0x83d58c}};
struct VtableSlot {std::uint32_t offset,function;};
inline constexpr std::array<VtableSlot,6> vtable_slots{{
  {0x28,0x71d950},{0xc0,0x718cb0},{0xd4,0x721950},
  {0xdc,0x721c20},{0xe4,0x721550},{0x100,0x720440}
}};
namespace detail {
inline bool range(std::uint32_t address,std::size_t size) noexcept {
  return address&&size&&size<=UINT64_C(0x100000000)-address;
}
inline bool overlaps(std::uint32_t a,std::size_t an,std::uint32_t b,std::size_t bn) noexcept {
  return std::uint64_t{a}<std::uint64_t{b}+bn&&std::uint64_t{b}<std::uint64_t{a}+an;
}
inline std::uint32_t u32(const std::uint8_t* b) noexcept {
  return std::uint32_t{b[0]}|(std::uint32_t{b[1]}<<8u)|(std::uint32_t{b[2]}<<16u)|(std::uint32_t{b[3]}<<24u);
}
inline void jump(std::uint8_t* out,std::uint32_t from,std::uint32_t to) noexcept {
  out[0]=0xe9;const auto relative=to-(from+5u);
  for(unsigned i=0;i<4;++i)out[i+1]=static_cast<std::uint8_t>(relative>>(i*8u));
}
inline lhiver::Digest digest(const void* data,std::size_t size) noexcept {
  Sha256 hash;hash.update(data,size);return hash.finish();
}
inline bool outside_engine(std::uint32_t address,std::size_t size) noexcept {
  return range(address,size)&&!overlaps(address,size,station_abi::image_base,station_abi::image_size);
}
inline PathVerification verify_hook(const ReadOnlyMemory& memory,const InstalledEndHook& hook) noexcept {
  constexpr auto size=end_prefix.size();
  if(!hook.installed||!outside_engine(hook.destination,1)||!outside_engine(hook.trampoline,size+5)||
    overlaps(hook.destination,1,hook.trampoline,size+5))return {false,PathIssue::hook_proof};
  std::array<std::uint8_t,size+5> actual{},expected{};
  expected.fill(0x90);jump(expected.data(),end_address,hook.destination);
  if(!memory.copy(end_address,actual.data(),size))return {false,PathIssue::unreadable};
  if(std::memcmp(actual.data(),expected.data(),size))return {false,PathIssue::hook_patch};
  std::memcpy(expected.data(),end_prefix.data(),size);
  jump(expected.data()+size,hook.trampoline+static_cast<std::uint32_t>(size),end_address+static_cast<std::uint32_t>(size));
  if(!memory.copy(hook.trampoline,actual.data(),actual.size()))return {false,PathIssue::unreadable};
  if(actual!=expected)return {false,PathIssue::trampoline};
  return {true,PathIssue::none};
}
inline PathVerification verify_vtable(const ReadOnlyMemory& memory,std::uint32_t table) noexcept {
  bool listed=false;for(const auto entry:mob_vtables)listed=listed||entry==table;
  if(!listed)return {false,PathIssue::vtable};
  for(const auto slot:vtable_slots){std::array<std::uint8_t,4> value{};
    if(!memory.copy(table+slot.offset,value.data(),value.size()))return {false,PathIssue::unreadable};
    if(u32(value.data())!=slot.function)return {false,PathIssue::vtable};}
  return {true,PathIssue::none};
}
inline PathVerification verify_range(const ReadOnlyMemory& memory,const CodeRange& wanted,const InstalledEndHook& hook,
    const zbug_end_compat::Binding* compatibility=nullptr) noexcept {
  if(!wanted.length||wanted.length>max_code_range||!range(wanted.address,wanted.length))return {false,PathIssue::range};
  std::array<std::uint8_t,max_code_range> first{},second{};
  if(!memory.copy(wanted.address,first.data(),wanted.length)||!memory.copy(wanted.address,second.data(),wanted.length))
    return {false,PathIssue::unreadable};
  if(std::memcmp(first.data(),second.data(),wanted.length))return {false,PathIssue::changed};
  if(overlaps(wanted.address,wanted.length,end_address,end_prefix.size())){
    if(end_address<wanted.address||std::uint64_t{end_address}+end_prefix.size()>std::uint64_t{wanted.address}+wanted.length)
      return {false,PathIssue::range};
    std::array<std::uint8_t,end_prefix.size()> expected{};expected.fill(0x90);jump(expected.data(),end_address,hook.destination);
    const auto offset=end_address-wanted.address;
    if(std::memcmp(first.data()+offset,expected.data(),expected.size()))return {false,PathIssue::hook_patch};
    std::memcpy(first.data()+offset,end_prefix.data(),end_prefix.size());
  }
  namespace z=zbug_end_compat;
  if(overlaps(wanted.address,wanted.length,z::call_address,z::original_call.size())){
    if(z::call_address<wanted.address||std::uint64_t{z::call_address}+z::original_call.size()>std::uint64_t{wanted.address}+wanted.length)
      return {false,PathIssue::range};
    const auto offset=z::call_address-wanted.address;
    if(std::memcmp(first.data()+offset,z::original_call.data(),z::original_call.size())){
      if(!compatibility||first[offset]!=0xe8)return {false,PathIssue::compatibility};
      const auto target=z::call_address+5u+u32(first.data()+offset+1);
      if(compatibility->cooling_predebit){
        if(!z::detail::cooling_forward(*compatibility,target))return {false,PathIssue::compatibility};
      }else{
        const auto proof=z::verify(memory,*compatibility);
        if(!proof.valid||target!=proof.call_target)return {false,PathIssue::compatibility};
      }
      std::memcpy(first.data()+offset,z::original_call.data(),z::original_call.size());
    }
  }
  return digest(first.data(),wanted.length)==wanted.sha256?PathVerification{true,PathIssue::none}:PathVerification{false,PathIssue::code_hash};
}
} // namespace detail

// Only fixed hash ranges; only the proven own 6-byte end hook is
// normalized in the hash map. Optionally additionally applies exactly
// the freshly verified ZBug PutInInv CALL; no arbitrary foreign
// prefix is skipped. vtable comes from the previously alive-bound
// Mob. After original neither this Mob nor an old hand/slot/choice
// pointer is dereferenced. Two complete passes, additionally double
// range reads; no synchronization against any foreign threads or
// future code patches. Native caller must already positively bind
// engine file identity, own hook lifetime and serialized engine thread.
inline PathVerification verify_path(const ReadOnlyMemory& memory,std::uint32_t vtable,const InstalledEndHook& hook,
    const zbug_end_compat::Binding* compatibility=nullptr) noexcept {
  for(unsigned pass=0;pass<2;++pass){
    auto checked=detail::verify_hook(memory,hook);if(!checked.valid)return checked;
    checked=detail::verify_vtable(memory,vtable);if(!checked.valid)return checked;
    for(const auto& area:code_ranges){checked=detail::verify_range(memory,area,hook,compatibility);if(!checked.valid)return checked;}
    checked=detail::verify_vtable(memory,vtable);if(!checked.valid)return checked;
    checked=detail::verify_hook(memory,hook);if(!checked.valid)return checked;
  }
  return {true,PathIssue::none};
}
} // namespace ubc::station_end_contract

namespace ubc {
struct StationEndPoint {
  // Own limited copy at the respective synchronous hook point. read_stable
  // requires the same positive identity/read proof before and after
  // this capture. Production comes from exactly verified script symbol.
  bool read_stable=false,identity_known=false,station_known=false;
  std::uint32_t thread=0,world=0,hero=0,station=0;
  bool station_owned=false,station_profile_verified=false,mob_fields_known=false;
  crafting_catalog::Station kind=crafting_catalog::Station::stove;
  std::int32_t state=-1,target=-1,state_num=-1,rewind=-1;
  bool production_known=false,hands_known=false;
  std::int32_t production=-1;
  StationRecipeHands hands{};
};
struct StationEndCall {
  std::uint32_t mob=0,npc=0,return_address=0;
  std::int32_t flag=-1;
  bool original_returned=false,original_path_verified=false,owner_thread_depth_verified=false;
  StationEndPoint before{},after{};
};
enum class StationEndIssue { none,binding,path,caller,scope,read,identity,profile,state,production,hands,attachment };
struct StationEndVerification {bool normal_exit_verified=false;StationEndIssue issue=StationEndIssue::none;};

// Narrow positive evidence of a normal EndInteraction-AFTER. This does NOT yet
// establish a recipe result. Covers only the five statically bound stateNum1/rewind0
// profiles. A known production contract from the request must again match the
// current mob profile strings. c.valid in S1 is NOT required. original_path_verified
// requires verify_path at BOTH hook boundaries. owner_thread_depth_verified
// requires our exact engine thread and the outer end wrapper, with no
// active effect, callback or start mutation. Never derive it from the UI.
//
// Normal hand cleanup must already have left the hand empty BEFORE
// EndInteraction(NPC,0). Otherwise StopInteraction at +0x721E53 could return
// a remaining anchor to inventory. This abort or emergency path must not claim
// a normal return. InterruptInteraction is not an admitted caller either.
//
// Only then require a fresh quiescent result frame, exact quantities, world and hero,
// production 0 and empty hands and anchor. CraftingResult checks sequence, generation,
// timeout, once-only execution and expected final stock. This value-only check does not.
inline StationEndVerification verify_station_end(const CraftingKey& key,const StationEndCall& call) noexcept {
  using Issue=StationEndIssue;
  if(!key.generation||!key.thread||!key.world||!key.hero||!key.station||!key.manager)return {false,Issue::binding};
  switch(key.kind){case crafting_catalog::Station::stove:case crafting_catalog::Station::alchemy:
    case crafting_catalog::Station::leather:case crafting_catalog::Station::saw:case crafting_catalog::Station::forge:
    case crafting_catalog::Station::anvil:case crafting_catalog::Station::bow:case crafting_catalog::Station::cool:
    case crafting_catalog::Station::grind:break;
    default:return {false,Issue::binding};}
  if(call.mob!=key.station||call.npc!=key.hero)return {false,Issue::binding};
  if(!call.original_path_verified)return {false,Issue::path};
  if(call.flag||call.return_address!=station_end_contract::completed_return)return {false,Issue::caller};
  if(!call.original_returned||!call.owner_thread_depth_verified)return {false,Issue::scope};
  for(const auto* p:{&call.before,&call.after}){
    if(!p->read_stable||!p->identity_known||!p->station_known||!p->production_known||!p->hands_known)
      return {false,Issue::read};
    if(p->thread!=key.thread||p->world!=key.world||p->hero!=key.hero)return {false,Issue::identity};
    if(p->production)return {false,Issue::production};
    if(!p->hands.interact_item_known||p->hands.interact_item)return {false,Issue::hands};
    for(const auto* hand:{&p->hands.hands.left,&p->hands.hands.right})
      if(hand->state!=0||hand->object||hand->symbol!=-1||hand->amount!=-1)return {false,Issue::hands};
  }
  const auto& before=call.before;
  if(!before.station_owned||!before.station_profile_verified||before.kind!=key.kind)return {false,Issue::profile};
  if(!before.mob_fields_known||before.state||before.target||before.state_num!=1||before.rewind)return {false,Issue::state};
  if(before.station!=key.station||call.after.station)return {false,Issue::attachment};
  return {true,Issue::none};
}
} // namespace ubc
