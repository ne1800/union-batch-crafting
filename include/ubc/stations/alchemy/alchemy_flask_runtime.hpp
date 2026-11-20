// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/alchemy/alchemy_flask_debit.hpp"
#include "ubc/catalogs/alchemy_recipe_catalog.hpp"
#include "ubc/engine/runtime_script.hpp"
#include "ubc/engine/selection_abi.hpp"
#include "ubc/stations/station_recipe_snapshot.hpp"
#include "ubc/engine/zbug_end_compat.hpp"

// Our read-only x86 guards. No engine calls, VM writes or installation.
// Primary API: gothicuef/gothic-api@65fb327c76f58c751468c8e7df917a86a89b9ed1.
// EXE-SHA: 0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f.
namespace ubc::alchemy_flask_runtime {
namespace debit=alchemy_flask_debit;
namespace catalog=alchemy_recipe_catalog;
namespace script=runtime_script;
inline constexpr bool runtime_ready=false;
inline constexpr std::uint32_t current_parser_pointer=0xab628c;
inline constexpr std::uint32_t inventory_vtable=0x83c714;
inline constexpr std::size_t max_code_range=1024;
enum class RangeKind { complete_function, reviewed_site };
struct CodeRange { std::uint32_t address,length; lhiver::Digest digest; RangeKind kind; };
// Eleven previously separately checked areas plus the two item getters.
// Two areas are only review sites, NOT a complete dependency closure.
inline constexpr std::array<CodeRange,13> code_ranges{{
  {0x7495f0,69,lhiver::digest_literal("d7770dbf7cb8b3911f3e6b941302e558478f14d6f31cdead4a1844f93b140e2f"),RangeKind::complete_function},
  {0x70d080,234,lhiver::digest_literal("712b37bb44c304977cba62b30a881b28638b886361920c169009f2078072dd5a"),RangeKind::complete_function},
  {0x70ce20,600,lhiver::digest_literal("fddbca357f77d7b6c1094e77232fa64d057a7a7c01e842a323a69385f1000998"),RangeKind::complete_function},
  {0x7125a0,99,lhiver::digest_literal("dd0405c11373e734e96da95c78810e0f052a0d11abd2433f7383b074f6fc9c11"),RangeKind::complete_function},
  {0x7126d0,23,lhiver::digest_literal("c0d8880de7a9968d267f9159d822d4412c77d558b8ab5f7dfdffe54d0a3bc824"),RangeKind::complete_function},
  {0x792fa0,104,lhiver::digest_literal("5fafb4804012208d052a99a02f53587239cba062535417622184473396091886"),RangeKind::complete_function},
  {0x7a1550,284,lhiver::digest_literal("c0ff69e142b1b78e2fd66f6ac9039d3df4cb8995218e1cf0ad750ced0d971850"),RangeKind::complete_function},
  {0x7a4b10,6,lhiver::digest_literal("27689c1783bd22234fc3ae8bade5a0f532c2886a3f0a363f3d55f88c49169dc0"),RangeKind::complete_function},
  {0x7a4b20,12,lhiver::digest_literal("aaf316deb241c157c180874929a901c6f920e32281b9e2311ff9ce874e7a0708"),RangeKind::complete_function},
  {0x711c7c,36,lhiver::digest_literal("c8ee034d7bd35189ddb7d925c5b8267fda18fe737e5c5e51c9103077ea0bbf8b"),RangeKind::reviewed_site},
  {0x792517,100,lhiver::digest_literal("e2987bff1fad49204a844d80182f62f6b7d0d75bdf5d10b11b4b7a2eaa368e1b"),RangeKind::reviewed_site},
  {0x711410,6,lhiver::digest_literal("dabe4c98578a4aefbc31e932bbf5a908b08a7141b7f7383aee43de39292c658f"),RangeKind::complete_function},
  {0x711420,7,lhiver::digest_literal("edefe40dfb0c9a0883df7eb4dab972f5fb212583cb6da3a8a9e00bf4f112f448"),RangeKind::complete_function},
}};
struct VtableEntry {std::uint32_t address,target;};
inline constexpr std::array<VtableEntry,5> vtable_entries{{
  {0x83c714+0xc0,0x70d080},{0x83c714+0x48,0x70ce20},{0x83c714+0xa8,0x709660},
  {0x83c804,0x711410},{0x83c804+0x78,0x711420}
}};
inline constexpr std::array<lhiver::SymbolBinding,26> flask_bindings{{
  {"SELF",1679,7,1,0,1537,false,0,false,0},
  {"OTHER",1680,7,1,0,1537,false,0,false,0},
  {"VICTIM",1681,7,1,0,1537,false,0,false,0},
  {"HERO",1683,7,1,0,1537,false,0,false,0},
  {"C_NPC",1537,4,0,32,-1,false,0,false,0},
  {"C_ITEM",1584,4,0,44,-1,false,0,false,0},
  {"ITMI_FLASK",16604,7,1,0,1584,false,0,false,0},
  {"PLAYER_MOBSI_PRODUCTION",12330,2,0,1,-1,false,0,false,0},
  {"C_NPC.AIVAR",1563,2,4,100,1537,false,0,true,648},
  {"AIV_INVINCIBLE",11998,2,1,1,-1,true,4,false,0},
  {"C_ITEM.NAME",1586,3,4,1,1584,false,0,true,292},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"C_ITEM.VALUE",1593,2,4,1,1584,false,0,true,352},
  {"C_ITEM.VISUAL",1611,3,4,1,1584,false,0,true,524},
  {"C_ITEM.MATERIAL",1616,2,4,1,1584,false,0,true,608},
  {"C_ITEM.DESCRIPTION",1621,3,4,1,1584,false,0,true,628},
  {"C_ITEM.TEXT",1622,3,4,6,1584,false,0,true,648},
  {"C_ITEM.COUNT",1623,2,4,6,1584,false,0,true,768},
  {"ITEM_KAT_NONE",1031,2,1,1,-1,true,1,false,0},
  {"ITEM_MULTI",1056,2,1,1,-1,true,2097152,false,0},
  {"VALUE_FLASK",16549,2,1,1,-1,true,3,false,0},
  {"MAT_GLAS",1265,2,1,1,-1,true,5,false,0},
  // The loaded parser renames these anonymous literals. DAT names are
  // FF16030/31; for the evidenced runtime profile exactly FF16029/30 apply.
  // Index, metadata, full value/constructor hash remain unchanged mandatory.
  {"\xff""16029",46447,3,1,1,-1,false,0,false,0},
  {"\xff""16030",46448,3,1,1,-1,false,0,false,0},
  {"NAME_VALUE",14210,3,1,1,-1,false,0,false,0},
}};
struct StringContract {std::uint32_t symbol,length;lhiver::Digest digest;};
inline constexpr std::array<StringContract,3> flask_strings{{
  {46447,18,lhiver::digest_literal("37ec3fc4b34d0ac472d3e27e9680273e8906ed415b04c1c301803a53162034fa")},
  {46448,14,lhiver::digest_literal("726d231d24f7ae1c76da27bb5cced95dee24c2b5a4f34b470487639929bc4ada")},
  {14210,5,lhiver::digest_literal("1dcd75ced3627e99c880afe118bffedce818cc3f2a5cfd94b76b3f935a609130")},
}};
inline constexpr auto* flask_constructor=catalog::function_for(16604);
inline constexpr lhiver::Profile flask_profile{lhiver::game_parser,lhiver::symbol_count,lhiver::code_length,lhiver::code_sha256,flask_constructor,1,flask_bindings.data(),flask_bindings.size()};

enum class Issue {
  none,boundary,thread,reentry,descriptor,range,unreadable,changed,native_code,
  vtable,script,string,parser,context,station,inventory,hands,alias,flask,
  dependency_evidence
};
struct Verification {bool valid=false;Issue issue=Issue::none;std::uint32_t target=0;};
struct NativeEvidence {
  bool reviewed_ranges_match=false,vtable_slots_match=false;
  Issue issue=Issue::none;std::uint32_t failed_address=0;
  // Only code identity. A checked list match must continue to
  // positively exclude the changed pack branch before any own remove.
  bool zbug_unpack_call=false;
  // The known external dependency closure is not invented here.
  // CreateVob/InsertVobInWorld/Allocator, used concrete sub-objects and the
  // configured bridge function pointers remain explicit adapter evidence.
};
enum class NativeCodeMode { original, known_zbug_remove_site, cooling_predebit };
struct NativeCodePolicy {
  NativeCodeMode mode=NativeCodeMode::original;
  zbug_end_compat::Module zbug{},union_core{};
};
struct ScriptEvidence {
  bool constructor_and_bindings_match=false,strings_match=false;
  script::Issue script_issue=script::Issue::none;
  Issue issue=Issue::none;std::uint32_t failed_symbol=0;
};
struct Boundary {
  debit::Context context{};
  debit::Scope scope{};
  std::uint64_t serial=0;
  std::uint32_t current_thread=0;
  bool synchronous_writer_verified=false;
  bool physical_lab_hand_contract_verified=false;
  bool native_dependencies_and_configured_targets_verified=false;
};
struct Capture {
  debit::Observation observation{};
  NativeEvidence native{};
  ScriptEvidence script{};
  PackedInventoryIssue inventory_issue=PackedInventoryIssue::none;
  StationHandReadIssue hand_issue=StationHandReadIssue::none;
  bool valid=false;Issue issue=Issue::none;
};
struct RestoreCapture {debit::RestoreObservation observation{};bool valid=false;Issue issue=Issue::none;};

namespace detail {
inline bool word(const ReadOnlyMemory& memory,std::uint32_t base,std::uint32_t offset,std::uint32_t& value) noexcept {
  if(!base || offset>UINT32_MAX-base) return false;
  std::array<std::uint8_t,4> data{};
  if(!memory.copy(base+offset,data.data(),data.size())) return false;
  value=script::detail::u32(data.data());return true;
}
inline Verification code_range(const ReadOnlyMemory& memory,const CodeRange& range) noexcept {
  if(!range.length || range.length>max_code_range || !script::detail::range(range.address,range.length)) return {false,Issue::range,range.address};
  std::array<std::uint8_t,max_code_range> a{},b{};
  if(!memory.copy(range.address,a.data(),range.length) || !memory.copy(range.address,b.data(),range.length)) return {false,Issue::unreadable,range.address};
  if(std::memcmp(a.data(),b.data(),range.length)) return {false,Issue::changed,range.address};
  Sha256 sha;sha.update(a.data(),range.length);
  return sha.finish()==range.digest?Verification{true,Issue::none,0}:Verification{false,Issue::native_code,range.address};
}
inline bool boundary(const Boundary& b,std::uint32_t thread) noexcept {
  return thread && b.current_thread==thread && b.context.thread==thread && debit::valid(b.context) && b.serial &&
    b.synchronous_writer_verified && b.physical_lab_hand_contract_verified &&
    b.scope.job && b.scope.writer_token && b.scope.callback && b.scope.writer_active &&
    b.scope.writer_depth==1 && b.scope.vm_depth==0 && b.scope.debit_depth<=1;
}
inline bool name_is(const ReadOnlyMemory& memory,std::uint32_t object,std::uint32_t offset,const char* wanted) noexcept {
  const auto length=std::strlen(wanted);if(length>63 || !object || offset>UINT32_MAX-object) return false;
  std::array<std::uint8_t,20> header{};std::array<char,64> text{};
  if(!memory.copy(object+offset,header.data(),header.size()) || script::detail::u32(header.data()+12)!=length) return false;
  if(!length) return true;
  return memory.copy(script::detail::u32(header.data()+8),text.data(),length+1) && std::memcmp(text.data(),wanted,length+1)==0;
}
inline const char* station_owners(const ReadOnlyMemory& memory,std::uint32_t mob,std::uint32_t hero,unsigned& matches) noexcept {
  matches=0;
  std::uint32_t node=0;if(!word(memory,mob,0x18c,node)) return "Liste unlesbar";
  std::array<std::uint32_t,64> seen{};std::size_t count=0;
  while(node) {
    if(count==seen.size()) return "mehr als 64 Positionen";
    for(std::size_t i=0;i<count;++i) if(seen[i]==node) return "Zyklus";
    seen[count++]=node;std::uint32_t position=0,next=0,npc=0;
    if(!word(memory,node,0,position)) return "Position unlesbar";
    if(!word(memory,node,4,next)) return "Nachfolger unlesbar";
    if(!word(memory,position,0x44,npc)) return "NPC unlesbar";
    if(npc==hero) ++matches;node=next;
  }
  return nullptr;
}
inline bool station_owns(const ReadOnlyMemory& memory,std::uint32_t mob,std::uint32_t hero) noexcept {
  unsigned matches=0;
  return !station_owners(memory,mob,hero,matches) && matches==1;
}
inline bool context(const ReadOnlyMemory& memory,const debit::Context& c) noexcept {
  if(!debit::valid(c)) return false;
  std::uint32_t game=0,type=0,world=0,hero=0,mob=0;
  return word(memory,selection_abi::game_pointer,0,game) && game==c.game &&
    word(memory,game,0,type) && type==selection_abi::game_vtable &&
    word(memory,game,selection_abi::game_world,world) && world==c.world &&
    word(memory,station_abi::hero_pointer,0,hero) && hero==c.hero &&
    word(memory,hero,station_abi::npc_interact_mob,mob) && mob==c.mob &&
    word(memory,hero,station_abi::npc_inventory,type) && type==inventory_vtable;
}
inline bool lab(const ReadOnlyMemory& memory,const debit::Context& c) noexcept {
  std::uint32_t state=0,target=0,count=0;
  return station_owns(memory,c.mob,c.hero) && word(memory,c.mob,0x1f4,state) && state==1 &&
    word(memory,c.mob,0x1fc,target) && target==1 && word(memory,c.mob,0x1f8,count) && count==1 &&
    name_is(memory,c.mob,0x1e0,"POTIONALCHEMY") && name_is(memory,c.mob,0x1a4,"ITMI_FLASK") && name_is(memory,c.mob,0x1cc,"");
}
struct ParserRead {
  debit::ParserState value{};script::detail::Layout layout{};
  bool valid=false;Issue issue=Issue::none;
};
inline ParserRead parser_once(const ReadOnlyMemory& memory,const debit::Context& c,bool require_current_use) noexcept {
  ParserRead result;auto& s=result.value;
  if(!debit::valid(c) || script::detail::read_layout(memory,c.parser,flask_profile,result.layout)!=script::Issue::none) {result.issue=Issue::parser;return result;}
  s.code_base=result.layout.code;s.code_size=result.layout.code_length;
  std::array<script::detail::Symbol,4> symbols{};
  // The parent class is positively checked; no merely coincidentally equal pointers.
  if(script::detail::check_binding(memory,result.layout,flask_bindings[4])!=script::Issue::none) {result.issue=Issue::script;return result;}
  for(std::size_t i=0;i<symbols.size();++i) {
    const auto& b=flask_bindings[i];
    if(script::detail::check_binding(memory,result.layout,b)!=script::Issue::none ||
       script::detail::lookup(memory,result.layout,b.symbol,symbols[i])!=script::Issue::none) {result.issue=Issue::script;return result;}
    for(std::size_t j=0;j<i;++j) if(symbols[j].address==symbols[i].address) {result.issue=Issue::alias;return result;}
  }
  s.self_symbol=symbols[0].address;s.hero_symbol=symbols[3].address;
  s.self=symbols[0].offset;s.other=symbols[1].offset;s.victim=symbols[2].offset;s.hero=symbols[3].offset;
  std::uint32_t cursor=0;
  if(!word(memory,current_parser_pointer,0,s.current_parser) || s.current_parser!=c.parser ||
     !word(memory,c.parser,0x4c,cursor) || cursor<s.code_base || cursor>result.layout.code_end ||
     !word(memory,c.parser,0x2058,s.data_depth) || s.data_depth>station_abi::parser_data_stack_capacity ||
     !word(memory,station_abi::instance_symbol_pointer,0,s.use_symbol) ||
     !word(memory,station_abi::instance_address_pointer,0,s.use_address)) {result.issue=Issue::parser;return result;}
  s.pop_pos=cursor-s.code_base;s.known=true;s.instance_bindings_verified=true;
  if(s.data_depth<=s.data_words.size()) {
    if(s.data_depth && !memory.copy(c.parser+station_abi::parser_data_stack,s.data_words.data(),
        s.data_depth*sizeof(std::uint32_t))) {result.issue=Issue::parser;return result;}
    s.data_words_known=true;
  }
  if(s.self!=c.hero || s.hero!=c.hero || (require_current_use && !debit::valid(s,c))) {result.issue=Issue::parser;return result;}
  if(script::detail::stable_layout(memory,c.parser,flask_profile,result.layout)!=script::Issue::none) {result.issue=Issue::changed;return result;}
  result.valid=true;return result;
}
inline bool same_parser(const ParserRead& a,const ParserRead& b) noexcept {
  return a.valid && b.valid && script::detail::same_layout(a.layout,b.layout) && debit::same(a.value,b.value);
}
inline Verification string_once(const ReadOnlyMemory& memory,const script::detail::Layout& layout,const StringContract& c) noexcept {
  script::detail::Symbol s{};std::array<std::uint8_t,20> header{};std::array<std::uint8_t,64> bytes{};
  if(c.length>=bytes.size() || script::detail::lookup(memory,layout,c.symbol,s)!=script::Issue::none ||
     s.type!=3 || s.flags!=1 || s.elements!=1 || s.parent || !memory.copy(s.data,header.data(),header.size()) ||
     script::detail::u32(header.data()+12)!=c.length ||
     !memory.copy(script::detail::u32(header.data()+8),bytes.data(),c.length+1)) return {false,Issue::string,c.symbol};
  if(bytes[c.length] || std::memchr(bytes.data(),0,c.length)) return {false,Issue::string,c.symbol};
  Sha256 sha;sha.update(bytes.data(),c.length);
  return sha.finish()==c.digest?Verification{true,Issue::none,0}:Verification{false,Issue::string,c.symbol};
}
inline bool same_stocks(const debit::Stocks& a,const debit::Stocks& b) noexcept {
  if(!a.complete_unambiguous || !b.complete_unambiguous || a.count!=b.count || a.count>a.rows.size()) return false;
  for(std::size_t i=0;i<a.count;++i) if(a.rows[i].symbol!=b.rows[i].symbol || a.rows[i].stored!=b.rows[i].stored || !a.rows[i].known || !b.rows[i].known) return false;
  return true;
}
inline bool same_flask(const debit::FlaskStack& a,const debit::FlaskStack& b) noexcept {
  return a.known && b.known && a.all_slot_aliases_excluded==b.all_slot_aliases_excluded && a.native_item_contract==b.native_item_contract &&
    a.packed==b.packed && a.listed_stacks==b.listed_stacks && a.object==b.object && a.amount==b.amount && a.flags==b.flags;
}
} // namespace detail

inline NativeEvidence verify_native(const ReadOnlyMemory& memory,const NativeCodePolicy& policy={}) noexcept {
  NativeEvidence result;
  for(unsigned pass=0;pass<2;++pass) {
    for(const auto& range:code_ranges) {
      Verification read;
      const bool cooling=policy.mode==NativeCodeMode::cooling_predebit;
      if((cooling||policy.mode==NativeCodeMode::known_zbug_remove_site) && range.address==0x70d080) {
        const auto& module=policy.zbug;
        if(!cooling&&(!module.mapping_verified || module.image_size!=zbug_end_compat::zbug_image_size ||
           module.file_sha256!=zbug_end_compat::zbug_file_sha256 ||
           !script::detail::range(module.base,module.image_size) ||
           zbug_end_compat::detail::overlaps(module.base,module.image_size,0x400000,0x752000) ||
           !zbug_end_compat::detail::verify_pe(memory,module,0x68bd012a).valid)) {
          result.issue=Issue::dependency_evidence;result.failed_address=module.base;return result;
        }
        std::array<std::uint8_t,234> a{},b{};
        if(!memory.copy(range.address,a.data(),a.size()) || !memory.copy(range.address,b.data(),b.size()))
          read={false,Issue::unreadable,range.address};
        else if(a!=b)read={false,Issue::changed,range.address};
        else {
          constexpr std::size_t offset=0x66;
          const auto target=0x70d0ebu+script::detail::u32(a.data()+offset+1);
          const bool patched=cooling?zbug_end_compat::detail::cooling_forward({module,policy.union_core,true},target):
            target==module.base+0x11e30;
          if(a[offset]!=0xe8 || (!patched && target!=0x70f8f0))read={false,Issue::native_code,0x70d0e6};
          else {
            if(pass && patched!=result.zbug_unpack_call) {result.issue=Issue::changed;return result;}
            result.zbug_unpack_call=patched;
            const std::uint32_t original=0x70f8f0u-0x70d0ebu;
            for(unsigned i=0;i<4;++i)a[offset+1+i]=static_cast<std::uint8_t>(original>>(8u*i));
            Sha256 hash;hash.update(a.data(),a.size());
            read=hash.finish()==range.digest?Verification{true,Issue::none,0}:Verification{false,Issue::native_code,range.address};
          }
        }
      } else if(cooling||policy.mode==NativeCodeMode::original || policy.mode==NativeCodeMode::known_zbug_remove_site)
        read=detail::code_range(memory,range);
      else read={false,Issue::dependency_evidence,0};
      if(!read.valid) {result.issue=read.issue;result.failed_address=read.target;return result;}
    }
    for(const auto& entry:vtable_entries) {
      std::uint32_t value=0;
      if(!detail::word(memory,entry.address,0,value) || value!=entry.target) {result.issue=Issue::vtable;result.failed_address=entry.address;return result;}
    }
  }
  result.reviewed_ranges_match=true;result.vtable_slots_match=true;return result;
}
inline ScriptEvidence verify_flask_constructor(const ReadOnlyMemory& memory) noexcept {
  ScriptEvidence result;
  for(unsigned pass=0;pass<2;++pass) {
    const auto checked=script::verify_local_contract(memory,lhiver::game_parser,flask_profile);
    if(!checked.matches) {result.issue=Issue::script;result.script_issue=checked.issue;result.failed_symbol=checked.failed_symbol;return result;}
    script::detail::Layout layout{};
    if(script::detail::read_layout(memory,lhiver::game_parser,flask_profile,layout)!=script::Issue::none) {result.issue=Issue::parser;return result;}
    for(const auto& c:flask_strings) {
      const auto read=detail::string_once(memory,layout,c);
      if(!read.valid) {result.issue=read.issue;result.failed_symbol=read.target;return result;}
    }
    if(script::detail::stable_layout(memory,lhiver::game_parser,flask_profile,layout)!=script::Issue::none) {result.issue=Issue::changed;return result;}
  }
  result.constructor_and_bindings_match=true;result.strings_match=true;return result;
}
enum class ParserStackMode { empty,original_returns };
inline detail::ParserRead read_parser(const ReadOnlyMemory& memory,const debit::Context& context,
    ParserStackMode mode=ParserStackMode::empty,std::uint32_t callback=0) noexcept {
  const auto accepted=[&](const debit::ParserState& value) noexcept {
    return mode==ParserStackMode::empty?debit::valid(value,context):
      mode==ParserStackMode::original_returns&&debit::valid_after_original(value,context,callback);
  };
  auto first=detail::parser_once(memory,context,false);if(!first.valid)return first;
  if(!accepted(first.value)) {first.valid=false;first.issue=Issue::parser;return first;}
  auto second=detail::parser_once(memory,context,false);if(!second.valid)return second;
  if(!accepted(second.value)) {second.valid=false;second.issue=Issue::parser;return second;}
  if(!detail::same_parser(first,second)) {auto failed=second;failed.valid=false;failed.issue=Issue::changed;return failed;}
  return second;
}

// Error diagnosis only. The read values do not grant parser, material,
// or restore permission. In particular, valid(...).data_depth==0 remains.
struct ParserFailureSnapshot {
  debit::ParserState value{};
  std::array<std::uint32_t,8> words{};
  std::uint32_t word_count=0;
  bool words_stable=false;
};
inline ParserFailureSnapshot parser_failure_snapshot(const ReadOnlyMemory& memory,
    const debit::Context& context,const debit::ParserState& failed) noexcept {
  ParserFailureSnapshot out;out.value=failed;
  if(!debit::valid(context) || !failed.known || !failed.instance_bindings_verified ||
     failed.current_parser!=context.parser || failed.data_depth>station_abi::parser_data_stack_capacity)return out;
  // With a larger unknown stack, only diagnosis of the first eight
  // words is possible. This is not a same_data_stack/restore proof.
  const auto same_diagnostic_state=[&](const debit::ParserState& current) noexcept {
    if(failed.data_depth<=out.words.size())return debit::same(failed,current);
    if(failed.data_depth!=current.data_depth)return false;
    auto a=failed,b=current;a.data_depth=0;b.data_depth=0;return debit::same(a,b);
  };
  const auto first=detail::parser_once(memory,context,false);
  if(!first.valid || !same_diagnostic_state(first.value))return out;
  const auto count=failed.data_depth<out.words.size()?failed.data_depth:static_cast<std::uint32_t>(out.words.size());
  std::array<std::uint32_t,8> a{},b{};
  if(count && (!memory.copy(context.parser+station_abi::parser_data_stack,a.data(),count*sizeof(std::uint32_t)) ||
               !memory.copy(context.parser+station_abi::parser_data_stack,b.data(),count*sizeof(std::uint32_t))))return out;
  const auto after=detail::parser_once(memory,context,false);
  if(!after.valid || !same_diagnostic_state(after.value) || a!=b)return out;
  out.words=a;out.word_count=count;out.words_stable=true;return out;
}
inline std::uint32_t parser_failure_flags(const ParserFailureSnapshot& value,const debit::Context& context) noexcept {
  const auto& s=value.value;
  const bool cursor=s.known && s.code_base && s.code_size && s.code_size<=0x1000000u &&
    std::uint64_t{s.code_base}+s.code_size<=UINT32_MAX && s.pop_pos<=s.code_size;
  return (s.current_parser && s.current_parser==context.parser?1u:0u) |
    (context.hero && s.self==context.hero?2u:0u) | (context.hero && s.hero==context.hero?4u:0u) |
    (s.use_symbol && s.use_symbol==s.self_symbol?8u:0u) | (s.use_symbol && s.use_symbol==s.hero_symbol?16u:0u) |
    (context.hero && s.use_address==context.hero?32u:0u) | (cursor?64u:0u) |
    (s.instance_bindings_verified?128u:0u) | (s.known?256u:0u) | (value.words_stable?512u:0u) |
    (value.words_stable && s.data_depth>value.word_count?1024u:0u);
}
// No arbitrary data/pointer values in the native log. These four literals
// suffice for the suspected integer return [1,0x40]; otherwise redigited=-1.
inline int parser_failure_log_word(std::uint32_t word) noexcept {
  return word==0 || word==1 || word==0x40 || word==0x41?static_cast<int>(word):-1;
}

// Persistent workspace only for the configured engine thread. The caller
// provides current_thread from the native thread API and a genuine
// synchronous write proof. Identical read results are not a reservation
// or synchronization against any foreign plugins. No heap allocation.
class Reader {
public:
  explicit Reader(std::uint32_t owner_thread) noexcept : owner_thread_(owner_thread) {}
  Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
  Capture capture(const ReadOnlyMemory& memory,const catalog::Descriptor& recipe,const Boundary& boundary,
      const NativeCodePolicy& policy={},ParserStackMode stack_mode=ParserStackMode::empty) noexcept {
    Capture out;out.observation.context=boundary.context;out.observation.scope=boundary.scope;out.observation.serial=boundary.serial;
    if(!owner_thread_ || boundary.current_thread!=owner_thread_ || boundary.context.thread!=owner_thread_) {out.issue=Issue::thread;return out;}
    if(reading_) {out.issue=Issue::reentry;return out;}
    if(!detail::boundary(boundary,owner_thread_)) {out.issue=Issue::boundary;return out;}
    if(!catalog::structurally_valid(recipe) || recipe.callback.function->symbol!=boundary.scope.callback) {out.issue=Issue::descriptor;return out;}
    reading_=true;struct Scope {bool& value;~Scope(){value=false;}} guard{reading_};
    out.native=verify_native(memory,policy);out.script=verify_flask_constructor(memory);
    auto first=read_once(memory,recipe,boundary,stack_mode);if(!first.valid) {first.native=out.native;first.script=out.script;return first;}
    auto second=read_once(memory,recipe,boundary,stack_mode);if(!second.valid) {second.native=out.native;second.script=out.script;return second;}
    out.observation=second.observation;
    if(!detail::same_stocks(first.observation.stocks,second.observation.stocks) ||
       !detail::same_flask(first.observation.flask,second.observation.flask) ||
       !debit::same(first.observation.hand,second.observation.hand) ||
       !debit::same(first.observation.parser,second.observation.parser) ||
       first.observation.production!=second.observation.production || first.observation.invincible!=second.observation.invincible) {out.issue=Issue::changed;return out;}
    const auto native_after=verify_native(memory,policy);const auto script_after=verify_flask_constructor(memory);
    if(!out.native.reviewed_ranges_match || !out.native.vtable_slots_match) {out.issue=out.native.issue;return out;}
    if(!native_after.reviewed_ranges_match || !native_after.vtable_slots_match) {out.native=native_after;out.issue=native_after.issue;return out;}
    if(!out.script.constructor_and_bindings_match || !out.script.strings_match) {out.issue=out.script.issue;return out;}
    if(!script_after.constructor_and_bindings_match || !script_after.strings_match) {out.script=script_after;out.issue=script_after.issue;return out;}
    if(!detail::context(memory,boundary.context) || !detail::lab(memory,boundary.context)) {out.issue=Issue::context;return out;}
    if(!boundary.native_dependencies_and_configured_targets_verified) {out.issue=Issue::dependency_evidence;return out;}
    out.observation.native_contract=true;out.observation.stable_read=true;out.valid=true;return out;
  }
  RestoreCapture restore_targets(const ReadOnlyMemory& memory,const debit::Call& call,const Boundary& boundary,
      const NativeCodePolicy& policy={}) noexcept {
    RestoreCapture out;
    if(!owner_thread_ || boundary.current_thread!=owner_thread_ || boundary.context.thread!=owner_thread_) {out.issue=Issue::thread;return out;}
    if(reading_) {out.issue=Issue::reentry;return out;}
    if(!detail::boundary(boundary,owner_thread_) || boundary.scope.debit_depth!=1 || !call.valid ||
       !debit::valid_after_original(call.parser,call.context,call.callback) || !debit::same(call.context,boundary.context)) {out.issue=Issue::boundary;return out;}
    reading_=true;struct Scope {bool& value;~Scope(){value=false;}} guard{reading_};
    for(unsigned pass=0;pass<2;++pass) {
      if(!detail::context(memory,boundary.context) || !detail::lab(memory,boundary.context)) {out.issue=Issue::context;return out;}
      // Cursor/UseInstance may diverge after the internal split. The
      // stored write targets are instead re-bound via symbol indices.
      const auto p=detail::parser_once(memory,boundary.context,false);
      if(!p.valid) {out.issue=p.issue;return out;}
      // CreateInstance runs directly/balanced on this active prefix. In
      // SEH/partial construction, additional words are not a restorable
      // normal case; no clear/pop/data-stack repair is invented.
      if(!debit::same_data_stack(call.parser,p.value)) {out.issue=Issue::parser;return out;}
      const auto& value=p.value;debit::RestoreObservation current;
      current.context=boundary.context;current.scope=boundary.scope;
      current.current_parser=value.current_parser;current.code_base=value.code_base;current.code_size=value.code_size;
      current.self_symbol=value.self_symbol;current.hero_symbol=value.hero_symbol;
      current.self=value.self;current.hero=value.hero;current.other=value.other;current.victim=value.victim;
      if(call.parser.use_symbol==value.self_symbol) {current.saved_use_symbol=value.self_symbol;current.saved_use_address=value.self;}
      else if(call.parser.use_symbol==value.hero_symbol) {current.saved_use_symbol=value.hero_symbol;current.saved_use_address=value.hero;}
      else {out.issue=Issue::parser;return out;}
      const auto native=verify_native(memory,policy);
      current.native_restore_contract=native.reviewed_ranges_match && native.vtable_slots_match && boundary.native_dependencies_and_configured_targets_verified;
      current.known=true;
      if(!debit::restorable(call,current)) {out.issue=current.native_restore_contract?Issue::parser:Issue::dependency_evidence;return out;}
      out.observation=current;
    }
    out.valid=true;return out;
  }
private:
  Capture read_once(const ReadOnlyMemory& memory,const catalog::Descriptor& recipe,const Boundary& boundary,ParserStackMode stack_mode) noexcept {
    Capture out;auto& v=out.observation;v.context=boundary.context;v.scope=boundary.scope;v.serial=boundary.serial;
    if(!detail::context(memory,boundary.context)) {out.issue=Issue::context;return out;}
    if(!detail::lab(memory,boundary.context)) {out.issue=Issue::station;return out;}
    const auto p=read_parser(memory,boundary.context,stack_mode,recipe.callback.function->symbol);v.parser=p.value;
    if(!p.valid) {out.issue=p.issue;return out;}
    const auto production=script::symbol_int(memory,boundary.context.parser,12330);
    std::uint32_t invincible=0;
    if(!production.valid || !detail::word(memory,boundary.context.hero,0x298,invincible) || invincible>1) {out.issue=Issue::script;return out;}
    v.production=production.value;v.invincible=static_cast<std::int32_t>(invincible);
    const auto stock=read_packed_inventory<catalog::catalog_target_capacity>(memory,boundary.context.hero,recipe.targets.data,recipe.targets.count,true,inventory_);
    out.inventory_issue=stock.issue;
    if(!stock.storage_totals_known || !stock.snapshot_unchanged || stock.issue!=PackedInventoryIssue::none) {out.issue=Issue::inventory;return out;}
    out.hand_issue=station_recipe_snapshot_detail::begin_hands(memory,boundary.context.hero,hands_);
    const auto& hand=hands_.before;
    if(out.hand_issue!=StationHandReadIssue::none || !hand.left_slot_known || !hand.right_slot_known ||
       hand.hands.left.state!=0 || hand.hands.right.state!=1 || hand.hands.right.symbol!=16604 || hand.hands.right.amount!=1 ||
       !hand.interact_item_known || hand.interact_item!=hand.hands.right.object) {out.issue=Issue::hands;return out;}
    v.hand={true,true,hand.hands.right.object,16604,1,hand.interact_item};
    v.stocks.count=recipe.targets.count;std::size_t flask_index=recipe.targets.count;
    for(std::size_t i=0;i<recipe.targets.count;++i) {
      const auto symbol=recipe.targets[i].symbol;
      if(catalog::units(recipe.costs,symbol) && (stock.listed_occurrences[i]>1 || stock.packed_occurrences[i]>1 ||
         stock.duplicate_packed_target[i] || stock.has_active_marker[i] || stock.mixed_representation[i] ||
         (stock.listed_occurrences[i] && stock.packed_occurrences[i]))) {out.issue=Issue::inventory;return out;}
      v.stocks.rows[i]={static_cast<std::uint32_t>(symbol),stock.total[i],true};
      if(symbol==16604) flask_index=i;
    }
    if(flask_index==recipe.targets.count) {out.issue=Issue::descriptor;return out;}
    v.flask.known=true;v.flask.packed=stock.packed[flask_index];v.flask.listed_stacks=stock.listed_occurrences[flask_index];
    for(std::size_t i=0;i<stock.listed_nodes;++i) {
      const auto& node=inventory_.records[i];const auto object=script::detail::u32(node.node.data()+4);
      const auto symbol=station_detail::little_i32(node.item_fields.data()+4);
      if(object==v.hand.right) {out.issue=Issue::alias;return out;}
      if(!catalog::units(recipe.costs,symbol)) continue;
      std::uint32_t type=0,flags=0;
      if(!detail::word(memory,object,0,type) || type!=hand_detail::item_vtable || !detail::word(memory,object,0x158,flags) ||
         !(flags&0x00200000u) || (flags&0x40000000u)) {out.issue=Issue::flask;return out;}
      for(std::size_t slot=0;slot<hands_.count;++slot)
        if(script::detail::u32(hands_.records[slot].fields.data()+0x30)==object) {out.issue=Issue::alias;return out;}
      if(symbol==16604) {v.flask.object=object;v.flask.amount=script::detail::u32(node.item_fields.data());v.flask.flags=flags;}
    }
    v.flask.all_slot_aliases_excluded=true;v.flask.native_item_contract=true;
    v.stocks.complete_unambiguous=true;
    if(!station_recipe_snapshot_detail::verify_hands(memory,hands_)) {out.issue=Issue::changed;return out;}
    if(!detail::context(memory,boundary.context) || !detail::lab(memory,boundary.context)) {out.issue=Issue::context;return out;}
    const auto after=read_parser(memory,boundary.context,stack_mode,recipe.callback.function->symbol);
    if(!after.valid) {v.parser=after.value;out.issue=after.issue;return out;}
    if(!detail::same_parser(p,after)) {out.issue=Issue::changed;return out;}
    out.valid=true;return out; // Partial capture only; public capture sets the permission bits only at the very end.
  }
  const std::uint32_t owner_thread_;
  bool reading_=false;
  PackedInventoryWorkspace inventory_{};
  station_recipe_snapshot_detail::HandWorkspace hands_{};
};
} // namespace ubc::alchemy_flask_runtime
