// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/alchemy/alchemy_flask_runtime.hpp"
#include "ubc/engine/zbug_end_compat.hpp"
#include <algorithm>

// Bounded READ-ONLY dependency observations. These do not authorize
// execution. The ranges were checked against local original files.
// A documented subset and the loaded allocator/ZPE dispatch were
// read on 01.11.2026. The last eight ranges added have only offline
// evidence here. Restoring ITEM on exception and integration remain
// separate. known_ranges_match alone must never set the existing
// native_dependencies_and_configured_targets_verified flag to true.
namespace ubc::alchemy_extra_dependencies {
namespace runtime=alchemy_flask_runtime;
namespace debit=alchemy_flask_debit;
namespace script=runtime_script;
using Module=zbug_end_compat::Module;
using CodeRange=runtime::CodeRange;
using RangeKind=runtime::RangeKind;
inline constexpr bool execution_authorized=false;
inline constexpr std::size_t code_capacity=4096,list_capacity=4096;
inline constexpr std::uint32_t parser_extender_size=0xdd000;
inline constexpr auto parser_extender_sha256=lhiver::digest_literal("4c3b9aa740a759de6da2f464564df9bbce8b528e1dd1a9bf9886294f57034045");
inline constexpr auto crt_sha256=lhiver::digest_literal("1221a09484964a6f38af5e34ee292b9afefccb3dc6e55435fd3aaf7c235d9067");
struct Modules {Module zbug{},union_core{},parser_extender{},runtime_crt{};};
struct Targets {std::uint32_t remove=0,restore_pop=0,restore_instance=0;};
inline constexpr Targets expected_targets{0x7495f0,0x7a4b20,0x7a1550};
enum class Issue {none,range,unreadable,changed,targets,module,code,vtable,patch,
  parser,item_binding,context,list,duplicate,item,flask,whole_stack,thread,reentry};
struct Check {bool valid=false;Issue issue=Issue::none;std::uint32_t address=0;};
inline constexpr std::array<CodeRange,21> additional_ranges{{
  {0x711bd0,1296,lhiver::digest_literal("e59773f0cbccb6bfa69ef177e414924de5c58a780eb92d7c5083790c939e66ae"),RangeKind::complete_function},
  {0x6c2c30,44,lhiver::digest_literal("ffe1a2ab43e98ed7e21c9193c39c0bbfba183b9dd235e5e3f24cdc2c0d0223e2"),RangeKind::complete_function},
  {0x5decf0,4,lhiver::digest_literal("916ae10aa3fd76a4d5b25b2fb66e9df0fd04957c7ab10930268d1ed7808be689"),RangeKind::complete_function},
  {0x77ef40,6,lhiver::digest_literal("926f69bc5892f22745b4e2130f3a3c718e5715a14b4b3e2e521c7d674eb31205"),RangeKind::complete_function},
  {0x780330,16,lhiver::digest_literal("8353eb7118ab2a8eca32dc79cd5be6f35c171d5c655bacf12e8e6cb118382516"),RangeKind::complete_function},
  {0x7808b0,216,lhiver::digest_literal("5a69df059e44636a345cccaa37964cfead95edf6a9df81aaca83682630d73911"),RangeKind::complete_function},
  {0x709660,216,lhiver::digest_literal("07044487ad5aa934aad18d417b8701767006082a44f4235906093a0ec1c4550c"),RangeKind::complete_function},
  {0x7092c0,42,lhiver::digest_literal("44c86e38b099aadcb0c8b9475fc36adb7bfdb07d473abf45810f7407f749f2d5"),RangeKind::complete_function},
  {0x711000,39,lhiver::digest_literal("5a2039a8824072c30fc1c91c331462bfab7afd147deb82e7e944d2708c2adfa1"),RangeKind::complete_function},
  {0x566680,59,lhiver::digest_literal("d2157ef684d81df47fafafa893736409692907549bd3e88e82d618f19074d259"),RangeKind::complete_function},
  {0x565f50,5,lhiver::digest_literal("9eb6ea5f968745c77cd4e7624ae2878903391e3a0d16579297616a8080d0c9fa"),RangeKind::reviewed_site},
  {0x7b4460,5,lhiver::digest_literal("8123416c083ef1c0cb908278f350eed53af95b468ef4d39318e1ae15988e9687"),RangeKind::reviewed_site},
  {0x7b44c6,6,lhiver::digest_literal("46b1394bc3595afe5c054a75df7ed43b34658481ea69a602576af5937a31053d"),RangeKind::reviewed_site},
  {0x77f3f0,535,lhiver::digest_literal("d52a63ee13a9b10294a31047180415e322df61d439ba85018bc73365a6b047b1"),RangeKind::complete_function},
  {0x711290,323,lhiver::digest_literal("9858d900b95483835823ffc79f02a8deb449668fa499480612f630228d1165e8"),RangeKind::complete_function},
  {0x711970,597,lhiver::digest_literal("c8b77815a48ccddfb252101e97f63c44db79845fc7dcca82c8a3bc041757e808"),RangeKind::complete_function},
  {0x794910,59,lhiver::digest_literal("c6eb35a66b9ea7e8eb4c71fe77f50d6f8b48ae2f720a92cc51676ec92998c76d"),RangeKind::complete_function},
  {0x7a3ee0,27,lhiver::digest_literal("4d58591c2bbb648afec3a4c5503e348a25a2de4823818e6a9c9d5f0844d2c637"),RangeKind::complete_function},
  {0x7a2f20,10,lhiver::digest_literal("eeccf6337909677489e16f4719bd017f6de247d05525e3dd9397e8acf32bd661"),RangeKind::complete_function},
  {0x7a2f30,4,lhiver::digest_literal("0b78473b3a56c63ff2a64431fb600078066cbe407d3dee7ea8fd7575bd7f82d5"),RangeKind::complete_function},
  {0x7938c0,8,lhiver::digest_literal("55206799d2de3269020df092efc4eca49b4d0bab13d2e70797bd21ce77f5699f"),RangeKind::complete_function}
}};
inline constexpr std::array<runtime::VtableEntry,10> additional_slots{{
  {0x83c28c+0x34,0x5decf0},{0x83c28c+0x8c,0x6c2c30},
  {0x82f104+0x3c,0x76fde0},{0x83e174,0x77ef40},
  {0x83e174+0x68,0x77f3f0},{0x83e174+0x6c,0x780330},
  {0x83c714+0x54,0x7092c0},{0x83c714+0x98,0x709740},
  {0x83c714+0xa0,0x709b00},{0x83c804+0x0c,0x711440}
}};
inline constexpr std::array<std::uint16_t,10> zpe_wrapper_relocations{{12,20,26,36,50,75,104,117,127,149}};
inline constexpr std::array<zbug_end_compat::CodeRange,5> zpe_ranges{{
  {0x7d560,169,lhiver::digest_literal("81cd366481c785a8f69102a445bb1f3db6f652dbfe4bc98488f3950146ed42aa"),zpe_wrapper_relocations.data(),zpe_wrapper_relocations.size()},
  {0x11fb0,17,lhiver::digest_literal("c8bdd0595a955bfe200718c58011174fe916c1a318ac8dafa4f07bc749fe577d")},
  {0x11ff0,17,lhiver::digest_literal("75c02e106e32cb16c8939015926178d174f3c224fa9b78bdddefbc6cccf9c63d")},
  {0x11c10,17,lhiver::digest_literal("aa93348e798ac170bf55ea8f0fdf8e03944dd9f400e1753793324c68b640fed9")},
  {0x12070,17,lhiver::digest_literal("7c537d670243af760660ffa64eec6ff9a0e424640749058e18e712e417a190c2")}
}};
inline constexpr lhiver::SymbolBinding item_binding{"ITEM",1682,7,1,0,1584,false,0,false,0};

namespace detail {
inline Check ok() noexcept {return {true,Issue::none,0};}
inline Check fail(Issue why,std::uint32_t address=0) noexcept {return {false,why,address};}
inline bool range(std::uint32_t a,std::size_t n) noexcept {return script::detail::range(a,n);}
inline bool word(const ReadOnlyMemory& m,std::uint32_t a,std::uint32_t o,std::uint32_t& out) noexcept {return runtime::detail::word(m,a,o,out);}
inline void put(std::uint8_t* p,std::uint32_t n) noexcept {for(unsigned i=0;i<4;++i)p[i]=static_cast<std::uint8_t>(n>>(i*8u));}
inline lhiver::Digest digest(const void* p,std::size_t n) noexcept {Sha256 h;h.update(p,n);return h.finish();}
inline Check twice(const ReadOnlyMemory& m,std::uint32_t a,std::uint8_t* out,std::size_t n) noexcept {
  if(!n||n>code_capacity||!range(a,n))return fail(Issue::range,a);
  std::array<std::uint8_t,code_capacity> second{};
  if(!m.copy(a,out,n)||!m.copy(a,second.data(),n))return fail(Issue::unreadable,a);
  return std::memcmp(out,second.data(),n)?fail(Issue::changed,a):ok();
}
inline Check plain(const ReadOnlyMemory& m,const CodeRange& c) noexcept {
  std::array<std::uint8_t,code_capacity> bytes{};const auto r=twice(m,c.address,bytes.data(),c.length);
  if(!r.valid)return r;
  return digest(bytes.data(),c.length)==c.digest?ok():fail(Issue::code,c.address);
}
inline Check patched_call(const ReadOnlyMemory& m,const CodeRange& c,std::uint32_t offset,
    std::uint32_t original,std::uint32_t replacement,bool& patched) noexcept {
  patched=false;
  if(c.length<5||offset>c.length-5)return fail(Issue::range,c.address);
  std::array<std::uint8_t,code_capacity> bytes{};auto r=twice(m,c.address,bytes.data(),c.length);if(!r.valid)return r;
  if(bytes[offset]!=0xe8)return fail(Issue::patch,c.address+offset);
  const auto target=c.address+offset+5u+script::detail::u32(bytes.data()+offset+1);
  if(target!=original&&target!=replacement)return fail(Issue::patch,c.address+offset);
  patched=target==replacement;
  // Exactly one previously verified CALL operand field. No general masking.
  put(bytes.data()+offset+1,original-(c.address+offset+5u));
  return digest(bytes.data(),c.length)==c.digest?ok():fail(Issue::code,c.address);
}
inline bool module(const Module& m,std::uint32_t size,const lhiver::Digest& hash) noexcept {
  return m.mapping_verified&&m.image_size==size&&m.file_sha256==hash&&range(m.base,size)&&
    !zbug_end_compat::detail::overlaps(m.base,size,0x400000,0x752000);
}
inline Check pe(const ReadOnlyMemory& m,const Module& module,std::uint32_t timestamp,std::uint32_t preferred, bool require_timestamp=true) noexcept {
  std::array<std::uint8_t,64> head{};auto r=twice(m,module.base,head.data(),head.size());if(!r.valid)return r;
  const auto at=script::detail::u32(head.data()+60);
  if(head[0]!='M'||head[1]!='Z'||at<64||at>4096-128)return fail(Issue::module,module.base);
  std::array<std::uint8_t,128> nt{};r=twice(m,module.base+at,nt.data(),nt.size());if(!r.valid)return r;
  const auto base=script::detail::u32(nt.data()+52);
  if(script::detail::u32(nt.data())!=0x4550||zbug_end_compat::detail::u16(nt.data()+4)!=0x14c||
    (require_timestamp && script::detail::u32(nt.data()+8)!=timestamp)||zbug_end_compat::detail::u16(nt.data()+20)!=224||
    zbug_end_compat::detail::u16(nt.data()+24)!=0x10b||(base!=preferred&&base!=module.base)||
    script::detail::u32(nt.data()+80)!=module.image_size)return fail(Issue::module,module.base);
  return ok();
}
inline Check modules(const ReadOnlyMemory& m,const Modules& b,bool cooling_predebit=false) noexcept {
  const bool plugins=cooling_predebit?
    zbug_end_compat::detail::mapped_plugin(b.zbug)&&zbug_end_compat::detail::mapped_plugin(b.union_core):
    module(b.zbug,zbug_end_compat::zbug_image_size,zbug_end_compat::zbug_file_sha256)&&
    module(b.union_core,zbug_end_compat::union_image_size,zbug_end_compat::union_file_sha256);
  if(!plugins||!module(b.parser_extender,parser_extender_size,parser_extender_sha256)||
    !module(b.runtime_crt,0xbe000,crt_sha256))return fail(Issue::module);
  const std::array<const Module*,4> all{{&b.zbug,&b.union_core,&b.parser_extender,&b.runtime_crt}};
  for(std::size_t i=0;i<all.size();++i)for(std::size_t j=0;j<i;++j)
    if(zbug_end_compat::detail::overlaps(all[i]->base,all[i]->image_size,all[j]->base,all[j]->image_size))return fail(Issue::module);
  const std::array<std::uint32_t,4> stamps{{0x68bd012a,0x62b8c69e,0x62b4dbf2,0x4ba1dbbe}};
  for(std::size_t i=cooling_predebit?2:0;i<all.size();++i)
    if(!pe(m,*all[i],stamps[i],i==3?0x78aa0000u:0x10000000u).valid)return fail(Issue::module,all[i]->base);
  return ok();
}
inline Check export_ordinal(const ReadOnlyMemory& m,const Module& module,std::uint32_t ordinal,std::uint32_t wanted_rva) noexcept {
  using zbug_end_compat::detail::in_module;
  std::array<std::uint32_t,6> initial{};
  for(unsigned pass=0;pass<2;++pass){
    std::uint32_t pe_at=0,exp_rva=0,exp_size=0;
    if(!word(m,module.base,60,pe_at)||pe_at<64||pe_at>4096-128||
      !word(m,module.base+pe_at,120,exp_rva)||!word(m,module.base+pe_at,124,exp_size)||
      exp_size<40||!in_module(module,exp_rva,exp_size))return fail(Issue::module,module.base);
    std::array<std::uint8_t,40> exp{};auto c=twice(m,module.base+exp_rva,exp.data(),exp.size());if(!c.valid)return c;
    const auto base=script::detail::u32(exp.data()+16),count=script::detail::u32(exp.data()+20),table=script::detail::u32(exp.data()+28);
    if(ordinal<base||!count||count>20000||ordinal-base>=count||!in_module(module,table,std::size_t{count}*4))return fail(Issue::module,module.base+exp_rva);
    std::uint32_t target=0;if(!word(m,module.base+table,(ordinal-base)*4u,target))return fail(Issue::unreadable,module.base+table);
    const std::array<std::uint32_t,6> current{{pe_at,exp_rva,exp_size,base,table,target}};
    if(pass&&current!=initial)return fail(Issue::changed,module.base);initial=current;
    if(target!=wanted_rva||!in_module(module,target,1)||(target>=exp_rva&&target-exp_rva<exp_size))return fail(Issue::targets,module.base+table);
  }
  return ok();
}
inline Check slot(const ReadOnlyMemory& m,const runtime::VtableEntry& s) noexcept {
  std::uint32_t a=0,b=0;
  if(!word(m,s.address,0,a)||!word(m,s.address,0,b))return fail(Issue::unreadable,s.address);
  if(a!=b)return fail(Issue::changed,s.address);
  return a==s.target?ok():fail(Issue::vtable,s.address);
}
} // namespace detail

inline constexpr CodeRange factory{0x76fde0,185,lhiver::digest_literal("9673ca01bd8ab68accdd5077e9f1dcfc25e66df515b2f103e9e841f7e0d2b3fe"),RangeKind::complete_function};
inline constexpr std::array<std::uint16_t,1> thunk_reloc{{6}};
inline const zbug_end_compat::CodeRange thunk{0x35450,10,lhiver::digest_literal("1efdd92c312a83b4876b9615f0f425850d3541fa2e63ca914efa60f30b000231"),thunk_reloc.data(),thunk_reloc.size()};
inline const zbug_end_compat::CodeRange resolver{0x31aa0,15,lhiver::digest_literal("00ad9d87537091652919476c1beb3470eab23b0a59ab9f9cf332434f3e53abc3")};
inline const zbug_end_compat::CodeRange version{0x2d670,4,lhiver::digest_literal("916ae10aa3fd76a4d5b25b2fb66e9df0fd04957c7ab10930268d1ed7808be689")};
inline constexpr auto do_stack_hash=lhiver::digest_literal("6c672bb4364d6b32fbc99bdd8062ec051249d740c9ef08c13b5f907fb036cb26");
inline constexpr auto handler_prefix_hash=lhiver::digest_literal("58177d55f905c4d4a8d287a102d12a912c4b1d452d9411b6f741742e8ca5f4e1");
inline constexpr auto crt_malloc_window_hash=lhiver::digest_literal("81b40a39b36bb761e1facb4d353fbfc59108dca3ac3aab7a5a73b60046513cb9");

struct KnownCode {
  bool known_ranges_match=false,zbug_unpack_call=false,zpe_init_call=false;
  bool ikarus_handler=false,zpe_error_call=false;
  Issue issue=Issue::none;std::uint32_t address=0;
};
inline constexpr std::array<lhiver::Function,2> ikarus_functions{{
  {"MEMINT_EXCEPTIONHANDLER",6490,29766,205,lhiver::digest_literal("f939a52aef54732cc55521efe5a3474cd5aa15fdf1eead6455ec87ee8ce29bf0"),0,5,1,0,-1},
  {"MEMINT_SETUPEXCEPTIONHANDLER",6494,29971,293,lhiver::digest_literal("8ad796993ce92a1ffe8ce2866aeaec745bde89329683d0e67f0302446a93bcc9"),0,5,1,0,-1}
}};
inline constexpr std::array<lhiver::SymbolBinding,5> ikarus_bindings{{
  {"MEMINT_TOPPOPPOS",6489,2,0,1,-1},
  {"MEMINT_EXCEPTIONHANDLERESP",6488,2,0,1,-1},
  {"MEMINT_SETUPEXCEPTIONHANDLER.HANDLEROFFSET",6496,2,0,1,-1},
  {"CONTENTPARSERADDRESS",1702,2,1,1,-1,true,static_cast<std::int32_t>(lhiver::game_parser)},
  {"MEMINT_SETUPEXCEPTIONHANDLER.CALL",6495,2,1,1,-1}
}};
inline constexpr lhiver::Profile ikarus_profile{lhiver::game_parser,lhiver::symbol_count,lhiver::code_length,lhiver::code_sha256,
  ikarus_functions.data(),ikarus_functions.size(),ikarus_bindings.data(),ikarus_bindings.size()};
namespace detail {
// Address binding alone does not verify code or execution. The calling
// handler guard also checks both script bodies and all 42 code bytes.
inline Check ikarus_targets(const ReadOnlyMemory& m,std::uint32_t handler,std::array<std::uint32_t,4>& targets) noexcept {
  if(!range(handler,42))return fail(Issue::range,handler);
  script::detail::Layout layout{};
  if(script::detail::read_layout(m,lhiver::game_parser,ikarus_profile,layout)!=script::Issue::none)return fail(Issue::parser);
  std::array<script::detail::Symbol,5> first{};
  for(unsigned pass=0;pass<2;++pass)for(std::size_t i=0;i<ikarus_bindings.size();++i){
    const auto& binding=ikarus_bindings[i];script::detail::Symbol symbol{};
    if(script::detail::check_binding(m,layout,binding)!=script::Issue::none||
       script::detail::lookup(m,layout,binding.symbol,symbol)!=script::Issue::none||!range(symbol.address,0x1c))return fail(Issue::parser,binding.symbol);
    if((i==2&&symbol.data!=ikarus_functions[0].byte_offset)||(i==4&&symbol.data!=handler))return fail(Issue::patch,binding.symbol);
    if(pass&&(symbol.address!=first[i].address||symbol.data!=first[i].data||symbol.offset!=first[i].offset))return fail(Issue::changed,binding.symbol);
    first[i]=symbol;
  }
  if(script::detail::stable_layout(m,lhiver::game_parser,ikarus_profile,layout)!=script::Issue::none)return fail(Issue::changed);
  for(std::size_t i=0;i<targets.size();++i)targets[i]=first[i].address+0x18u;
  return ok();
}
inline Check ikarus_handler(const ReadOnlyMemory& m,std::uint32_t handler) noexcept {
  if(!script::verify_local_contract(m,lhiver::game_parser,ikarus_profile).matches)return fail(Issue::parser,handler);
  std::array<std::uint32_t,4> before{},after{};auto checked=ikarus_targets(m,handler,before);if(!checked.valid)return checked;
  std::array<std::uint8_t,42> code{};checked=twice(m,handler,code.data(),code.size());if(!checked.valid)return checked;
  constexpr std::array<std::size_t,4> offsets{{7,15,20,27}};
  for(std::size_t i=0;i<offsets.size();++i){
    if(script::detail::u32(code.data()+offsets[i])!=before[i])return fail(Issue::patch,handler+static_cast<std::uint32_t>(offsets[i]));
    put(code.data()+offsets[i],0);
  }
  // Hash only AFTER binding each normalized field; no wildcards.
  if(digest(code.data(),31)!=handler_prefix_hash||
     code[31]!=0xe8||handler+36u+script::detail::u32(code.data()+32)!=0x791960||
     code[36]!=0xe9||handler+41u+script::detail::u32(code.data()+37)!=0x82a880||code[41]!=0xc3)return fail(Issue::patch,handler);
  checked=ikarus_targets(m,handler,after);if(!checked.valid)return checked;
  if(before!=after||!script::verify_local_contract(m,lhiver::game_parser,ikarus_profile).matches)return fail(Issue::changed,handler);
  return ok();
}
} // namespace detail
// Only the named sections. The ZBug unpack call may later be treated as
// unreachable solely through the complete positive list path. ZPE-Init
// remains reachable and additionally requires its Dispatch/Lifetime.
inline KnownCode verify_known_code(const ReadOnlyMemory& m,const Modules& b,const Targets& targets,
    bool cooling_predebit=false) noexcept {
  KnownCode out;using namespace detail;
  if(targets.remove!=expected_targets.remove||targets.restore_pop!=expected_targets.restore_pop||
    targets.restore_instance!=expected_targets.restore_instance){out.issue=Issue::targets;return out;}
  auto checked=modules(m,b,cooling_predebit);if(!checked.valid){out.issue=checked.issue;out.address=checked.address;return out;}
  KnownCode first;
  for(unsigned pass=0;pass<2;++pass){
    out.ikarus_handler=false;
    if(cooling_predebit){
      const auto native=runtime::verify_native(m,{runtime::NativeCodeMode::cooling_predebit,b.zbug,b.union_core});
      if(native.issue!=runtime::Issue::none){out.issue=Issue::code;out.address=native.failed_address;return out;}
      out.zbug_unpack_call=native.zbug_unpack_call;
    }else for(std::size_t i=0;i<runtime::code_ranges.size();++i){
      const auto& c=runtime::code_ranges[i];
      checked=i==1?patched_call(m,c,0x66,0x70f8f0,b.zbug.base+0x11e30,out.zbug_unpack_call):plain(m,c);
      if(!checked.valid){out.issue=checked.issue;out.address=checked.address;return out;}
    }
    for(const auto& c:additional_ranges){checked=plain(m,c);if(!checked.valid){out.issue=checked.issue;out.address=checked.address;return out;}}
    for(const auto& s:runtime::vtable_entries){checked=slot(m,s);if(!checked.valid){out.issue=checked.issue;out.address=checked.address;return out;}}
    for(const auto& s:additional_slots){checked=slot(m,s);if(!checked.valid){out.issue=checked.issue;out.address=checked.address;return out;}}
    checked=patched_call(m,factory,159,0x711bd0,b.parser_extender.base+0x7d560,out.zpe_init_call);
    if(!checked.valid){out.issue=checked.issue;out.address=checked.address;return out;}
    for(const auto& c:zpe_ranges)if(!zbug_end_compat::detail::verify_code(m,b.parser_extender,c).valid){out.issue=Issue::code;out.address=b.parser_extender.base+c.rva;return out;}
    // DoStack remains UNHOOKED. Its two existing mod fields are read
    // tightly; the error wrapper is not a claimed normal success path.
    std::array<std::uint8_t,code_capacity> bytes{};checked=twice(m,0x791960,bytes.data(),3536);
    if(!checked.valid){out.issue=checked.issue;out.address=checked.address;return out;}
    const auto handler=script::detail::u32(bytes.data()+3);
    if(handler!=0x82a880){
      checked=ikarus_handler(m,handler);
      if(!checked.valid){out.issue=checked.issue;out.address=checked.address;return out;}
      out.ikarus_handler=true;put(bytes.data()+3,0x82a880);
    }
    constexpr std::uint32_t error_offset=0x7926fc-0x791960;
    const auto error_target=0x792701u+script::detail::u32(bytes.data()+error_offset+1);
    if(bytes[error_offset]!=0xe8||(error_target!=0x78e270&&error_target!=b.parser_extender.base+0x71c70)){out.issue=Issue::patch;out.address=0x7926fc;return out;}
    out.zpe_error_call=error_target!=0x78e270;put(bytes.data()+error_offset+1,0x78e270u-0x792701u);
    if(digest(bytes.data(),3536)!=do_stack_hash){out.issue=Issue::code;out.address=0x791960;return out;}
    if(pass&&(out.zbug_unpack_call!=first.zbug_unpack_call||out.zpe_init_call!=first.zpe_init_call||
      out.ikarus_handler!=first.ikarus_handler||out.zpe_error_call!=first.zpe_error_call)){out.issue=Issue::changed;return out;}
    first=out;
  }
  out.known_ranges_match=true;return out;
}

struct ItemBinding {
  bool known=false;std::uint32_t parser=0,symbol=0,value=0,table=0,code=0;
  std::uint32_t code_size=0,symbol_count=0;
};
inline bool same_item_target(const ItemBinding& a,const ItemBinding& b) noexcept {
  return a.known&&b.known&&a.parser==lhiver::game_parser&&a.symbol&&a.symbol%4u==0&&a.table&&a.code&&
    a.code_size&&a.symbol_count>=lhiver::symbol_count&&a.parser==b.parser&&a.symbol==b.symbol&&a.table==b.table&&a.code==b.code&&
    a.code_size==b.code_size&&a.symbol_count==b.symbol_count;
}
inline bool same(const ItemBinding& a,const ItemBinding& b) noexcept {
  return same_item_target(a,b)&&a.value==b.value;
}
inline ItemBinding read_item_binding(const ReadOnlyMemory& m,std::uint32_t parser) noexcept {
  ItemBinding out;
  if(parser!=lhiver::game_parser)return out;
  for(unsigned pass=0;pass<2;++pass){
    script::detail::Layout layout{};script::detail::Symbol s{};std::uint32_t current=0;
    if(!detail::word(m,runtime::current_parser_pointer,0,current)||current!=parser||
      script::detail::read_layout(m,parser,runtime::flask_profile,layout)!=script::Issue::none||
      script::detail::check_binding(m,layout,runtime::flask_bindings[5])!=script::Issue::none||
      script::detail::check_binding(m,layout,item_binding)!=script::Issue::none||
      script::detail::lookup(m,layout,item_binding.symbol,s)!=script::Issue::none||
      script::detail::stable_layout(m,parser,runtime::flask_profile,layout)!=script::Issue::none)return {};
    ItemBinding current_value{true,parser,s.address,s.offset,layout.table,layout.code,layout.code_length,layout.count};
    if(pass&&!same(out,current_value))return {};out=current_value;
  }
  return out;
}
struct ItemRestorePlan {
  bool valid=false,requires_write=false;
  std::uint32_t native_entry=0,symbol_object=0,saved_value=0;
};
// Value-only plan. SetOffset(0x7A2F20) writes exactly Symbol+1C and makes no
// further calls. Immediately beforehand, the actual bridge must reread metadata,
// targets, thread and scope and positively configure native_entry. Afterward
// it must check same(saved,read_item_binding(...)). No finally block yet.
inline ItemRestorePlan item_restore_plan(const debit::Call& call,const ItemBinding& saved,
    const ItemBinding& current,const debit::Context& context,const debit::Scope& scope) noexcept {
  if(!call.valid||!call.job||!call.writer_token||!debit::find(call.callback)||!debit::valid(call.context)||!debit::valid_after_original(call.parser,call.context,call.callback)||
    call.instance!=16604||!call.quantity||call.quantity>INT32_MAX||!debit::same(call.context,context)||
    !scope.writer_active||scope.writer_depth!=1||scope.vm_depth||scope.debit_depth!=1||
    scope.job!=call.job||scope.writer_token!=call.writer_token||scope.callback!=call.callback||
    !same_item_target(saved,current)||saved.parser!=call.context.parser||saved.code!=call.parser.code_base||
    saved.code_size!=call.parser.code_size)return {};
  return {true,saved.value!=current.value,0x7a2f20,saved.symbol,saved.value};
}

enum class RemovalPath {unknown,split,whole};
// expected_world is exclusively the freshly bound Context.world. Default0
// allows no new Ref1 alternative; the previous Ref>=2 path remains.
struct ListRequest {std::uint32_t hero=0,flask_object=0,extra_quantity=0,expected_world=0,item_symbol=16604;};
inline constexpr std::size_t whole_world_node_capacity=65536;
inline constexpr std::array<CodeRange,17> whole_world_ranges{{
  runtime::code_ranges[2],runtime::code_ranges[11],runtime::code_ranges[12],
  additional_ranges[1],additional_ranges[2],additional_ranges[3],additional_ranges[4],additional_ranges[5],
  additional_ranges[6],additional_ranges[7],additional_ranges[8],additional_ranges[9],
  {0x781480,119,lhiver::digest_literal("4f0f2c0b761f69f36497ea4ee28d7686b9258ae7911e8995d0a6353a2412dbee"),RangeKind::complete_function},
  {0x7110a0,119,lhiver::digest_literal("9dcc18c09aefc80e71e7ea0d7715671059bd6c8356c02e5cfc9aa08c7464c66e"),RangeKind::complete_function},
  {0x5664e0,394,lhiver::digest_literal("71f9aa5fe8639023681eefc0d849975a66511819eae94c3bb790b5c6a087825f"),RangeKind::complete_function},
  {0x77ef50,30,lhiver::digest_literal("266523792825be10b2e534b0751d000fba7818170af53be8a51b0784161c1534"),RangeKind::complete_function},
  {0x77ef90,32,lhiver::digest_literal("1854c9e026d1a48239697368b4c2ba82bf68e84729dbe1dfe429959920c18496"),RangeKind::complete_function}
}};
inline constexpr std::array<runtime::VtableEntry,9> whole_world_slots{{
  {0x83c28c+0x34,0x5decf0},{0x83c28c+0x8c,0x6c2c30},{0x83e174,0x77ef40},{0x83e174+0x6c,0x780330},
  {0x83c714+0x54,0x7092c0},{0x83c714+0xa8,0x709660},{0x83c714+0x48,0x70ce20},
  {0x83c804,0x711410},{0x83c804+0x78,0x711420}
}};
struct WholeWorldEvidence {
  bool verified=false;
  std::uint32_t world=0,added_references=0,ref_before_release=0,pool_free_count=0;
  std::array<std::uint32_t,2> nodes{};
  std::array<bool,2> contains_target{};
  lhiver::Digest fingerprint{};
  Issue issue=Issue::none;
};
inline bool same(const WholeWorldEvidence& a,const WholeWorldEvidence& b) noexcept {
  return a.verified==b.verified&&a.world==b.world&&a.added_references==b.added_references&&
    a.ref_before_release==b.ref_before_release&&a.pool_free_count==b.pool_free_count&&a.nodes==b.nodes&&
    a.contains_target==b.contains_target&&a.fingerprint==b.fingerprint&&a.issue==b.issue;
}
struct ListEvidence {
  bool stable_complete=false,unpack_unreachable=false,whole_short_path=false;
  RemovalPath path=RemovalPath::unknown;
  std::size_t nodes=0,target_index=0;
  std::uint32_t amount=0,refcount=0;
  Issue issue=Issue::none;
  WholeWorldEvidence whole_world{};
};
inline bool same_list_evidence(const ListEvidence& a,const ListEvidence& b) noexcept {
  return a.stable_complete==b.stable_complete&&a.unpack_unreachable==b.unpack_unreachable&&
    a.whole_short_path==b.whole_short_path&&a.path==b.path&&a.nodes==b.nodes&&a.target_index==b.target_index&&
    a.amount==b.amount&&a.refcount==b.refcount&&a.issue==b.issue&&same(a.whole_world,b.whole_world);
}
// No pack-data assumption: the order must additionally bind the
// complete previous stock/slot inventory. This reader proves
// only the native linear list path, even through non-cost items.
class ListReader {
  struct Record {
    std::uint32_t node=0,object=0,next=0,amount=0,symbol=0,vptr=0,refcount=0,flags=0;
    bool operator==(const Record& b) const noexcept {return node==b.node&&object==b.object&&next==b.next&&amount==b.amount&&symbol==b.symbol&&vptr==b.vptr&&refcount==b.refcount&&flags==b.flags;}
  };
  std::array<Record,list_capacity> records_{};
  // Persistent limited workspace; capture allocates nothing. The
  // sorting also recognizes partially overlapping 12-byte nodes.
  std::array<std::uint32_t,list_capacity+2*whole_world_node_capacity+5> whole_nodes_{};
  std::uint32_t owner_=0;bool reading_=false;
  WholeWorldEvidence world_references(const ReadOnlyMemory& m,const ListRequest& q,const ListEvidence& list,
      std::uint32_t selected) noexcept {
    using namespace detail;WholeWorldEvidence out;
    if(!q.expected_world||list.refcount!=1||list.target_index>=list.nodes||list.nodes>records_.size()){
      out.issue=Issue::whole_stack;return out;}
    const auto code=[&]() noexcept {
      for(const auto& range:whole_world_ranges){const auto c=plain(m,range);if(!c.valid)return c;}
      for(const auto& entry:whole_world_slots){const auto c=slot(m,entry);if(!c.valid)return c;}
      return ok();
    };
    auto checked=code();if(!checked.valid){out.issue=checked.issue;return out;}
    constexpr std::uint32_t pool_address=0x8d83b4;
    const auto inv=q.hero+0x668;lhiver::Digest initial{};
    for(unsigned pass=0;pass<2;++pass){
      Sha256 hash;std::size_t used=0;
      const auto hash_word=[&](std::uint32_t value) noexcept {std::array<std::uint8_t,4> b{};put(b.data(),value);hash.update(b.data(),b.size());};
      const auto add_node=[&](std::uint32_t node) noexcept {
        if(!range(node,12)||node%4||used==whole_nodes_.size())return false;
        whole_nodes_[used++]=node;hash_word(node);return true;
      };
      std::array<std::uint32_t,16> context{};
      const auto read_context=[&](std::array<std::uint32_t,16>& v) noexcept {
        return word(m,station_abi::hero_pointer,0,v[0])&&v[0]==q.hero&&
          word(m,selection_abi::game_pointer,0,v[1])&&v[1]&&word(m,v[1],0,v[2])&&v[2]==0x83c28c&&
          word(m,v[1],8,v[3])&&v[3]==q.expected_world&&word(m,v[3],0,v[4])&&v[4]==0x83e174&&
          word(m,inv,0,v[5])&&v[5]==runtime::inventory_vtable&&word(m,inv,4,v[6])&&v[6]==inv+0xa8&&
          word(m,inv,0xb0,v[7])&&v[7]==records_[0].node&&word(m,inv,0x24,v[8])&&v[8]==selected&&
          word(m,inv,0x98,v[9])&&!v[9]&&word(m,0x8a2558,0,v[10])&&!v[10]&&
          word(m,0x8d8780,0,v[11])&&!v[11]&&word(m,0xab1168,0x3c,v[12])&&v[12]==0xab3510&&
          word(m,0xab3510,0x3c,v[13])&&v[13]==0x9a34d8&&
          word(m,0x9a34d8,0x3c,v[14])&&v[14]==0x8d8cd8&&word(m,0x8d8cd8,0x3c,v[15])&&!v[15];
      };
      if(!read_context(context)){out.issue=Issue::context;return out;}
      for(auto v:context)hash_word(v);
      // Binds the same complete inventory list, not just the target pointer.
      for(std::size_t i=0;i<list.nodes;++i){const auto& saved=records_[i];Record current;current.node=saved.node;
        if(!word(m,current.node,4,current.object)||!word(m,current.node,8,current.next)||
          !word(m,current.object,0,current.vptr)||!word(m,current.object,4,current.refcount)||
          !word(m,current.object,0x32c,current.amount)||!word(m,current.object,0x330,current.symbol)||
          !word(m,current.object,0x158,current.flags)){out.issue=Issue::unreadable;return out;}
        if(!(current==saved)){out.issue=Issue::changed;return out;}
        if(!add_node(current.node)){out.issue=Issue::range;return out;}
        for(auto v:{current.object,current.next,current.vptr,current.refcount,current.amount,current.symbol,current.flags})hash_word(v);
      }
      std::array<std::uint32_t,5> pool{};
      for(std::uint32_t i=0;i<pool.size();++i){if(!word(m,pool_address,i*4u,pool[i])){out.issue=Issue::unreadable;return out;}hash_word(pool[i]);}
      // Two already-existing free nodes suffice for both world lists.
      // The just-freed inventory node is conservatively NOT assumed
      // as additional reserve; no Allocator/Growth exception path.
      if(pool[0]!=12||pool[1]!=4096||pool[4]<2||pool[4]>INT32_MAX||!pool[3]){out.issue=Issue::whole_stack;return out;}
      auto free=pool[3],back=pool_address+12;
      for(unsigned i=0;i<2;++i){
        std::uint32_t next=0,previous=0;
        if(!add_node(free)||!word(m,free,0,next)||!word(m,free,4,previous)){out.issue=Issue::unreadable;return out;}
        if(previous!=back){out.issue=Issue::whole_stack;return out;}
        hash_word(next);hash_word(previous);back=free;free=next;
      }
      if(pool[4]==2&&free){out.issue=Issue::whole_stack;return out;}
      if(pool[4]>2&&!free){out.issue=Issue::whole_stack;return out;}
      if(free){std::uint32_t previous=0;
        if(!add_node(free)||!word(m,free,4,previous)){out.issue=Issue::unreadable;return out;}
        if(previous!=back){out.issue=Issue::whole_stack;return out;}hash_word(previous);
      }
      WholeWorldEvidence current;current.world=q.expected_world;current.pool_free_count=pool[4];
      for(unsigned index=0;index<2;++index){
        std::uint32_t header=0;std::array<std::uint8_t,12> fields{};
        if(!word(m,q.expected_world,index?0x6288u:0x6280u,header)||!add_node(header)||!m.copy(header,fields.data(),fields.size())){out.issue=Issue::unreadable;return out;}
        if(script::detail::u32(fields.data())!=(index?0x77ef90u:0x77ef50u)){out.issue=Issue::whole_stack;return out;}
        hash.update(fields.data(),fields.size());auto node=script::detail::u32(fields.data()+8);unsigned target_count=0;
        while(node){
          if(current.nodes[index]==whole_world_node_capacity){out.issue=Issue::list;return out;}
          if(!add_node(node)||!m.copy(node,fields.data(),fields.size())){out.issue=Issue::unreadable;return out;}
          const auto object=script::detail::u32(fields.data()+4);if(!object){out.issue=Issue::whole_stack;return out;}
          if(object==q.flask_object&&++target_count>1){out.issue=Issue::duplicate;return out;}
          hash.update(fields.data(),fields.size());node=script::detail::u32(fields.data()+8);++current.nodes[index];
        }
        current.contains_target[index]=target_count!=0;
        if(!target_count)++current.added_references;
      }
      std::sort(whole_nodes_.begin(),whole_nodes_.begin()+static_cast<std::ptrdiff_t>(used));
      for(std::size_t i=1;i<used;++i)if(std::uint64_t{whole_nodes_[i-1]}+12>whole_nodes_[i]){out.issue=Issue::duplicate;return out;}
      current.ref_before_release=list.refcount+current.added_references;
      if(current.ref_before_release<=1||current.ref_before_release>INT32_MAX){out.issue=Issue::whole_stack;return out;}
      std::array<std::uint32_t,16> final_context{};
      const auto& target=records_[list.target_index];std::array<std::uint32_t,5> final_target{};
      if(!read_context(final_context)||context!=final_context||!word(m,q.flask_object,0,final_target[0])||
        !word(m,q.flask_object,4,final_target[1])||!word(m,q.flask_object,0x158,final_target[2])||
        !word(m,q.flask_object,0x32c,final_target[3])||!word(m,q.flask_object,0x330,final_target[4])||
        final_target!=std::array<std::uint32_t,5>{{target.vptr,target.refcount,target.flags,target.amount,target.symbol}}){out.issue=Issue::changed;return out;}
      current.fingerprint=hash.finish();
      if(pass&&current.fingerprint!=initial){out.issue=Issue::changed;return out;}initial=current.fingerprint;out=current;
    }
    checked=code();if(!checked.valid){out.issue=checked.issue;return out;}
    out.verified=true;return out;
  }
public:
  explicit ListReader(std::uint32_t thread) noexcept:owner_(thread){}
  ListReader(const ListReader&)=delete;ListReader& operator=(const ListReader&)=delete;
  ListEvidence capture(const ReadOnlyMemory& m,std::uint32_t current_thread,const ListRequest& q) noexcept {
    ListEvidence out;using namespace detail;
    if(!owner_||current_thread!=owner_){out.issue=Issue::thread;return out;}
    if(reading_){out.issue=Issue::reentry;return out;}
    if((q.item_symbol!=16604&&q.item_symbol!=16660)||!q.hero||!q.flask_object||!q.extra_quantity||q.extra_quantity>INT32_MAX||q.hero>UINT32_MAX-0x800){out.issue=Issue::range;return out;}
    reading_=true;struct Guard {bool& b;~Guard(){b=false;}} guard{reading_};
    for(const auto& s:std::array<runtime::VtableEntry,3>{{{0x83c804+0x78,0x711420},{0x83c714+0xc0,0x70d080},{0x83c714+0x48,0x70ce20}}})
      if(!slot(m,s).valid){out.issue=Issue::vtable;return out;}
    const auto getter=plain(m,runtime::code_ranges[12]);if(!getter.valid){out.issue=getter.issue;return out;}
    std::uint32_t initial_head=0,initial_selected=0,initial_mode=0,initial_pool=0,initial_list=0;
    for(unsigned pass=0;pass<2;++pass){
      const auto inv=q.hero+0x668;std::uint32_t vptr=0,head=0,selected=0,mode=0,pool=0,list=0;
      if(!word(m,inv,0,vptr)||vptr!=runtime::inventory_vtable||!word(m,inv,4,list)||!word(m,inv,0xb0,head)||
        !word(m,inv,0x24,selected)||!word(m,inv,0x98,mode)||!word(m,0x8a2558,0,pool)){out.issue=Issue::unreadable;return out;}
      if(pass&&(head!=initial_head||selected!=initial_selected||mode!=initial_mode||pool!=initial_pool||list!=initial_list)){out.issue=Issue::changed;return out;}
      initial_head=head;initial_selected=selected;initial_mode=mode;initial_pool=pool;initial_list=list;
      std::size_t count=0,target_index=0;unsigned matches=0;std::uint32_t target_amount=0,target_ref=0;
      while(head){
        if(count==records_.size()){out.issue=Issue::list;return out;}
        Record r;r.node=head;
        if(!word(m,head,4,r.object)||!word(m,head,8,r.next)||!r.object||
          !word(m,r.object,0,r.vptr)||!word(m,r.object,4,r.refcount)||!word(m,r.object,0x32c,r.amount)||
          !word(m,r.object,0x330,r.symbol)||!word(m,r.object,0x158,r.flags)){out.issue=Issue::unreadable;return out;}
        if(r.vptr!=0x83c804||r.symbol>INT32_MAX||r.amount>INT32_MAX){out.issue=Issue::item;return out;}
        for(std::size_t i=0;i<count;++i)if(records_[i].node==r.node||records_[i].object==r.object){out.issue=Issue::duplicate;return out;}
        if(pass&&!(records_[count]==r)){out.issue=Issue::changed;return out;}records_[count]=r;
        if(r.symbol==q.item_symbol){
          ++matches;if(r.object!=q.flask_object||r.amount<q.extra_quantity||!r.refcount||r.refcount>INT32_MAX||
            !(r.flags&0x200000u)||(r.flags&0x40000000u)){out.issue=Issue::flask;return out;}
          target_index=count;target_amount=r.amount;target_ref=r.refcount;
        }
        ++count;head=r.next;
      }
      if(matches!=1){out.issue=Issue::flask;return out;}
      if(pass&&count!=out.nodes){out.issue=Issue::changed;return out;}
      out.nodes=count;out.target_index=target_index;out.amount=target_amount;out.refcount=target_ref;
      out.path=target_amount==q.extra_quantity?RemovalPath::whole:RemovalPath::split;
      if(out.path==RemovalPath::whole){
        // The index must still be valid AFTER node removal. Refcount>=2
        // rules out the later destructive release. Ref1 needs new
        // WorldAdd evidence only after BOTH inventory passes. No
        // atomicity if a third-party hook or engine error intervenes.
        if(list!=inv+0xa8||mode||pool||count<2||selected>=count-1||!target_ref||target_ref>INT32_MAX-2u||
          (target_ref==1&&!q.expected_world)){out.issue=Issue::whole_stack;return out;}
        out.whole_short_path=target_ref>=2;
      }
      std::uint32_t end_head=0,end_selected=0,end_mode=0,end_pool=0,end_type=0,end_list=0;
      if(!word(m,inv,0,end_type)||!word(m,inv,4,end_list)||!word(m,inv,0xb0,end_head)||!word(m,inv,0x24,end_selected)||
        !word(m,inv,0x98,end_mode)||!word(m,0x8a2558,0,end_pool)){out.issue=Issue::unreadable;return out;}
      if(end_type!=vptr||end_head!=initial_head||end_selected!=initial_selected||end_mode!=initial_mode||end_pool!=initial_pool||end_list!=initial_list){out.issue=Issue::changed;return out;}
    }
    if(out.path==RemovalPath::whole&&out.refcount==1){
      out.whole_world=world_references(m,q,out,initial_selected);
      if(!out.whole_world.verified){out.issue=out.whole_world.issue;return out;}
      out.whole_short_path=true;
    }
    out.stable_complete=true;out.unpack_unreachable=true;return out;
  }
};

struct ObjectEvidence {
  bool stable=false,flask_class_is_vob_item=false;
  std::uint32_t factory=0;Issue issue=Issue::none;
};
// +6C is InsertVobInWorld, not RemoveVob. The ClassDef chain excludes
// the NPC branch of the world-list entry; it proves no new refcount yet.
inline ObjectEvidence read_objects(const ReadOnlyMemory& m,const debit::Context& c) noexcept {
  ObjectEvidence out;using namespace detail;
  if(!debit::valid(c)){out.issue=Issue::context;return out;}
  std::array<std::uint32_t,18> first{};
  for(unsigned pass=0;pass<2;++pass){
    std::array<std::uint32_t,18> v{};
    if(!word(m,selection_abi::game_pointer,0,v[0])||v[0]!=c.game||!word(m,c.game,0,v[1])||v[1]!=0x83c28c||
      !word(m,c.game,8,v[2])||v[2]!=c.world||!word(m,c.world,0,v[3])||v[3]!=0x83e174||
      !word(m,station_abi::hero_pointer,0,v[4])||v[4]!=c.hero||!word(m,c.hero,0x964,v[5])||v[5]!=c.mob||
      !word(m,c.hero,0x668,v[6])||v[6]!=0x83c714||!word(m,0x8d8df0,0,v[7])||!v[7]||
      !word(m,v[7],0,v[8])||v[8]!=0x82f104){out.issue=Issue::context;return out;}
    std::uint32_t cls=0xab1168;bool item=false,vob=false;std::size_t n=0;
    while(cls){
      if(n==8||cls==0xab1e20){out.issue=Issue::context;return out;}
      for(std::size_t i=0;i<n;++i)if(v[10+i]==cls){out.issue=Issue::context;return out;}
      v[10+n++]=cls;item=item||cls==0xab1168;vob=vob||cls==0xab3510;
      if(!word(m,cls,0x3c,cls)){out.issue=Issue::unreadable;return out;}
    }
    v[9]=static_cast<std::uint32_t>(n);
    if(!item||!vob){out.issue=Issue::context;return out;}
    if(pass&&v!=first){out.issue=Issue::changed;return out;}first=v;out.factory=v[7];
  }
  out.stable=true;out.flask_class_is_vob_item=true;return out;
}

struct PendingFacts {
  bool read_stable=false,allocator_target_matches_export=false,wrapper_parser_matches=false;
  bool original_dispatch_fields_match=false,item_known=false;
  bool allocator_boundary_verified=false,wrapper_dispatch_verified=false,engine_version_verified=false;
  std::uint32_t allocator_entry=0,allocator_forward=0,wrapper_record=0,engine_version_entry=0;
  ItemBinding item{};Issue issue=Issue::none;
};
enum class ReadDepth {fields_only,reviewed_entry_points};
// Pinned loader file + loaded export/IAT binding + normalized entry
// are the explicit CRT trust boundary. No recursive CRT/heap audit.
// The 128B window is an identity check, not a full malloc body. Shw32
// is packed: the file stub +1A40 becomes Export +35450 after loading.
inline PendingFacts read_pending_facts(const ReadOnlyMemory& m,const Modules& b,ReadDepth depth=ReadDepth::reviewed_entry_points) noexcept {
  PendingFacts out;using namespace detail;auto checked=modules(m,b);
  if(!checked.valid){out.issue=checked.issue;return out;}
  std::array<std::uint32_t,12> before{};
  for(unsigned pass=0;pass<2;++pass){
    std::array<std::uint32_t,12> v{};const auto z=b.parser_extender.base,u=b.union_core.base;
    if(!word(m,0x82e30c,0,v[0])||!word(m,z,0xc0b1c,v[1])||!word(m,z,0xc44c0,v[2])||
      !word(m,z,0xa91fc,v[3])||!word(m,z,0xa9230,v[4])||!v[2]||
      !word(m,v[2],0,v[5])||!word(m,v[2],12,v[6])||!word(m,v[2],20,v[7])||
      !word(m,runtime::current_parser_pointer,0,v[8])||!word(m,u,0x812e8,v[9])||
      !word(m,z,0xa92e4,v[10])||v[10]!=u+0x3e96c8||!word(m,v[10],8,v[11])){out.issue=Issue::unreadable;return out;}
    if(pass&&v!=before){out.issue=Issue::changed;return out;}before=v;
    out.allocator_entry=v[0];out.allocator_forward=v[9];out.wrapper_record=v[2];out.engine_version_entry=v[4];
    out.allocator_target_matches_export=v[0]==u+0x35450;
    out.wrapper_parser_matches=v[1]==lhiver::game_parser&&v[8]==lhiver::game_parser;
    out.original_dispatch_fields_match=v[3]==u+0x31aa0&&v[5]==u+0x81df8&&!v[6]&&v[7]==0x711bd0;
    if(!out.allocator_target_matches_export||v[9]!=b.runtime_crt.base+0x10233||v[4]!=u+0x2d670||v[11]!=4){out.issue=Issue::targets;return out;}
    if(depth==ReadDepth::reviewed_entry_points){
    if(!export_ordinal(m,b.union_core,258,0x35450).valid||!export_ordinal(m,b.runtime_crt,1477,0x10233).valid){out.issue=Issue::module;return out;}
    for(const auto& code:{thunk,resolver,version})if(!zbug_end_compat::detail::verify_code(m,b.union_core,code).valid){out.issue=Issue::code;return out;}
    std::array<std::uint8_t,128> crt{};checked=twice(m,v[9],crt.data(),crt.size());if(!checked.valid){out.issue=checked.issue;return out;}
    for(auto off:{22u,44u,50u})put(crt.data()+off,script::detail::u32(crt.data()+off)-b.runtime_crt.base+0x78aa0000u);
    if(digest(crt.data(),crt.size())!=crt_malloc_window_hash){out.issue=Issue::code;return out;}
    out.allocator_boundary_verified=true;out.engine_version_verified=true;
    out.wrapper_dispatch_verified=out.original_dispatch_fields_match&&out.wrapper_parser_matches;
    }
    const auto item=read_item_binding(m,lhiver::game_parser);
    if(!item.known||(pass&&!same(out.item,item))){out.issue=Issue::item_binding;return out;}out.item=item;
  }
  out.item_known=true;out.read_stable=true;return out;
}
} // namespace ubc::alchemy_extra_dependencies
