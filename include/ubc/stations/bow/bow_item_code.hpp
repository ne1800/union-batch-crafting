// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/bow/bow_item_path.hpp"
#include "ubc/stations/alchemy/alchemy_extra_dependencies.hpp"

namespace ubc::bow_item_code {
namespace rs=runtime_script;
namespace dep=alchemy_extra_dependencies;
// Additional fixed original bodies for bow. No CALL masking or acceptance
// of a foreign entry JMP. The old known module fields are checked
// exclusively by dep::verify_known_code with its tight origin binding.
struct CodeRange {std::uint32_t address,length;lhiver::Digest digest;};
inline constexpr std::array<CodeRange,15> ranges{{
  {0x6e72e0,244,lhiver::digest_literal("481717adcba9af978541fdb48f8a7838bdb387f9157c1d1956aa7131ecc174c7")}, // NpcHasItems
  {0x6e7b60,546,lhiver::digest_literal("4a7c43779cc9104f2a23ad030249dc20e3e5e1ab112d7b236fa1f83abf075737")}, // CreateInvItems
  {0x6e8200,602,lhiver::digest_literal("8ed9d0dd6fab11a114ded319d4b49b9c3d9715dc6e0ccb22165f59937db33422")}, // NpcRemoveInvItems
  {0x70c970,263,lhiver::digest_literal("10ae5c0974c06acb525b08289ae668dd07474e672de78699e43754eeb030cff4")}, // GetAmount
  {0x70caf0,238,lhiver::digest_literal("97b0b4b6ff76d352dc6700f2938a7984cd1503a609ec616a4a9c1f82aff8e55a")}, // IsIn_int_int
  {0x70f8f0,956,lhiver::digest_literal("b70ad1bf09d70f96c8031098f09ad21f5e2b665ac053bf90510d5210e200ed1a")}, // GetPackedItemInfo
  {0x730800,445,lhiver::digest_literal("29308fddee492fe52ddcbf5f382e9e98a4d5043c7d74ff0df07b14a707d38256")}, // NpcCreateItems
  {0x749350,357,lhiver::digest_literal("047d8df6c814ae187d00d7d5186c124eb13e7e7bae6c27babae88cb809ad684c")}, // NpcPutInInv
  {0x77f240,419,lhiver::digest_literal("19f97586909ee8baa422a2a1b3aa128025c3884b5638f692695dc992d9d0e17a")}, // WorldCreateVobString
  {0x70c730,573,lhiver::digest_literal("bc08d36bee92fcfca8ba05fb68d343e48bd92aa1cbde52cf2ed22b8eed4f2cc2")}, // InventoryInsert
  {0x705b80,804,lhiver::digest_literal("b722aa31e892036f30fbff7a95e78bdd2dd810b09e5473ecf9cd1534a4151c73")}, // CompareItemsWithJumpTable
  {0x705eb0,263,lhiver::digest_literal("5b9b1075bf1efe9e359d9ebf1640cfffd528d0260ab519a2e1707013cb1c4ab8")}, // CompareInstanceNames
  {0x712500,25,lhiver::digest_literal("a9086a5d17561c445228719d6fb32b424ace2d1ba38bd1b46629568bd6a97928")}, // GetDamageTotal
  {0x713d30,49,lhiver::digest_literal("70664dab34ebad0a3bb193ce5ab840dad80ff3ee4b4ad1656b09c609f2ab9503")}, // GetItemInstanceName
  {0x7937f0,185,lhiver::digest_literal("4d931fba9f02bb327b69a8c657922988c96465ea3b03235b30f83fcf82d3821b")}, // ParserGetSymbolInfo
}};
inline constexpr std::array<alchemy_flask_runtime::VtableEntry,7> slots{{
  {0x83c804,0x711410},{0x83c804+0x78,0x711420},{0x83c804+0x7c,0x713d30},
  {0x83c714+0x44,0x70c730},{0x83c714+0x48,0x70ce20},
  {0x83c714+0xa8,0x709660},{0x83c714+0xc0,0x70d080}
}};
inline constexpr std::array<lhiver::SymbolBinding,11> bindings{{
  {"NPC_HASITEMS",160,5,11,2,-1,false,0,true,2},
  {"NPC_HASITEMS.PAR0",161,7,0,0,-1,false,0,false,0},
  {"NPC_HASITEMS.PAR1",162,2,0,0,-1,false,0,false,0},
  {"CREATEINVITEMS",170,5,9,3,-1,false,0,true,0},
  {"CREATEINVITEMS.PAR0",171,7,0,0,-1,false,0,false,0},
  {"CREATEINVITEMS.PAR1",172,2,0,0,-1,false,0,false,0},
  {"CREATEINVITEMS.PAR2",173,2,0,0,-1,false,0,false,0},
  {"NPC_REMOVEINVITEMS",184,5,11,3,-1,false,0,true,2},
  {"NPC_REMOVEINVITEMS.PAR0",185,7,0,0,-1,false,0,false,0},
  {"NPC_REMOVEINVITEMS.PAR1",186,2,0,0,-1,false,0,false,0},
  {"NPC_REMOVEINVITEMS.PAR2",187,2,0,0,-1,false,0,false,0}
}};
enum class Issue {none,range,read,changed,hash,slot,parser,binding,target,module,dispatch,objects};
struct Check {bool verified=false;Issue issue=Issue::none;std::uint32_t address=0,symbol=0;};
inline Check verify_original_parts(const ReadOnlyMemory& memory) noexcept {
  for(unsigned pass=0;pass<2;++pass){
    for(const auto& r:ranges){
      if(!r.length||r.length>dep::code_capacity||!rs::detail::range(r.address,r.length))return {false,Issue::range,r.address,0};
      const auto result=dep::detail::plain(memory,{r.address,r.length,r.digest,alchemy_flask_runtime::RangeKind::complete_function});
      if(!result.valid)return {false,result.issue==dep::Issue::changed?Issue::changed:
        result.issue==dep::Issue::code?Issue::hash:Issue::read,r.address,0};
    }
    for(const auto& slot:slots)if(!dep::detail::slot(memory,slot).valid)return {false,Issue::slot,slot.address,0};
  }return {true,Issue::none,0,0};
}
inline Check verify_externals(const ReadOnlyMemory& memory,std::uint32_t parser) noexcept {
  rs::detail::Layout layout{};
  if(rs::detail::read_layout(memory,parser,lhiver::profile,layout)!=rs::Issue::none)return {false,Issue::parser,parser,0};
  std::array<rs::detail::Symbol,bindings.size()> first{};
  for(unsigned pass=0;pass<2;++pass){
    for(std::size_t i=0;i<bindings.size();++i){
      const auto& b=bindings[i];rs::detail::Symbol symbol{};
      if(rs::detail::check_binding(memory,layout,b)!=rs::Issue::none||
         rs::detail::lookup(memory,layout,b.symbol,symbol)!=rs::Issue::none||
         rs::detail::name_matches(memory,symbol,b.name)!=rs::Issue::none)return {false,Issue::binding,0,b.symbol};
      if(symbol.type!=b.type||symbol.flags!=b.flags||symbol.elements!=b.elements||symbol.parent||
        (b.check_offset&&symbol.offset!=static_cast<std::uint32_t>(b.offset)))return {false,Issue::binding,0,b.symbol};
      const auto target=b.symbol==160?0x6e72e0u:b.symbol==170?0x6e7b60u:b.symbol==184?0x6e8200u:0u;
      if(target&&symbol.data!=target)return {false,Issue::target,symbol.data,b.symbol};
      if(pass&&!station_recipe_runtime::detail::same_symbol(first[i],symbol))return {false,Issue::changed,symbol.address,b.symbol};
      first[i]=symbol;
    }
    if(rs::detail::stable_layout(memory,parser,lhiver::profile,layout)!=rs::Issue::none)return {false,Issue::changed,parser,0};
  }return {true,Issue::none,0,0};
}
struct ModuleEvidence {
  bool observed=false;Issue issue=Issue::none;
  dep::KnownCode code{};dep::PendingFacts dispatch{};dep::ObjectEvidence objects{};
};
// Real code/PE/export/IAT/object reads. The module values must originate
// from held own WindowsModuleIdentity leases. No name lookup, no
// 'historically once loaded', and no synthetic true in the adapter.
// Also observed is only this partial closure; it excludes the new
// sort/pool/visual paths and therefore produces no execution ticket.
inline ModuleEvidence observe_modules(const ReadOnlyMemory& memory,const dep::Modules& modules,
    const alchemy_flask_debit::Context& context) noexcept {
  ModuleEvidence out;out.code=dep::verify_known_code(memory,modules,dep::expected_targets);
  if(!out.code.known_ranges_match){out.issue=Issue::module;return out;}
  out.dispatch=dep::read_pending_facts(memory,modules);
  if(!out.dispatch.read_stable||!out.dispatch.allocator_boundary_verified||!out.dispatch.wrapper_dispatch_verified||
    !out.dispatch.engine_version_verified||!out.dispatch.item_known){out.issue=Issue::dispatch;return out;}
  // read_objects checks generic Game/World/Hero/Mob/Factory and
  // oCItem ClassDef bindings. No lab/flask profile release is used.
  out.objects=dep::read_objects(memory,context);
  if(!out.objects.stable||!out.objects.flask_class_is_vob_item){out.issue=Issue::objects;return out;}
  const auto again=dep::read_pending_facts(memory,modules);
  if(!again.read_stable||!again.allocator_boundary_verified||!again.wrapper_dispatch_verified||
    !again.engine_version_verified||!dep::same(again.item,out.dispatch.item)||
    again.allocator_entry!=out.dispatch.allocator_entry||again.allocator_forward!=out.dispatch.allocator_forward||
    again.wrapper_record!=out.dispatch.wrapper_record||again.engine_version_entry!=out.dispatch.engine_version_entry){out.issue=Issue::changed;return out;}
  const auto code_after=dep::verify_known_code(memory,modules,dep::expected_targets);
  const auto objects_after=dep::read_objects(memory,context);
  if(!code_after.known_ranges_match||code_after.zbug_unpack_call!=out.code.zbug_unpack_call||code_after.zpe_init_call!=out.code.zpe_init_call||
    code_after.zpe_error_call!=out.code.zpe_error_call||code_after.ikarus_handler!=out.code.ikarus_handler||
    !objects_after.stable||!objects_after.flask_class_is_vob_item||objects_after.factory!=out.objects.factory){out.issue=Issue::changed;return out;}
  out.observed=true;return out;
}
} // namespace ubc::bow_item_code
