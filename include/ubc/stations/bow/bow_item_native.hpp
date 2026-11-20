// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/engine/vob_inventory_insert_compat.hpp"
#include "ubc/stations/bow/bow_item_code.hpp"
#include "ubc/stations/grind/grind_zbug_unpack.hpp"

// Pinned bow sub-paths. Every positive finding names only its own boundary;
// neither historical world observation nor a module name constitutes release.
namespace ubc::bow_item_native {
namespace path=bow_item_path; namespace cat=bow_recipe_catalog;
namespace dep=alchemy_extra_dependencies; namespace rs=runtime_script;
using CodeRange=bow_item_code::CodeRange;
enum class Issue {none,range,unreadable,changed,code,module,dispatch,constructor,
  string,pool,sorter,lookup,world,list,alias,boundary,thread,reentry};
struct Check {bool verified=false;Issue issue=Issue::none;std::uint32_t address=0;};
struct BowDispatch {
  zbug_end_compat::Module locator{};
  bool empty_pack=false;
  std::uint32_t unpack_target=0,insert_target=0;
};
inline constexpr std::array<CodeRange,17> engine_ranges{{
  {0x5664e0,394,lhiver::digest_literal("71f9aa5fe8639023681eefc0d849975a66511819eae94c3bb790b5c6a087825f")}, // PoolAlloc
  {0x565f70,5,lhiver::digest_literal("88e6263add3b00f73f6c287e32ebe715501dcc4d073efe648a420d28d64537f5")}, // PoolMalloc
  {0x565f80,5,lhiver::digest_literal("f13a21abdcbda7aac73fdc0715c57b950f1bb1ca94e91858faa23dac2b4c5434")}, // PoolFree
  {0x7b446f,5,lhiver::digest_literal("92e331aabd288d8b13ca7e9302c9368853b4405c90fa727e183a9602af18116a")}, // CFree
  {0x781480,119,lhiver::digest_literal("4f0f2c0b761f69f36497ea4ee28d7686b9258ae7911e8995d0a6353a2412dbee")}, // WorldListInsert
  {0x7110a0,119,lhiver::digest_literal("9dcc18c09aefc80e71e7ea0d7715671059bd6c8356c02e5cfc9aa08c7464c66e")}, // ItemListInsert
  {0x706070,584,lhiver::digest_literal("3ba98e8f009ef10512fc89b01d66e96ce85617147a22ac606876feba2f367b69")}, // CompareMisc
  {0x5fe1e0,598,lhiver::digest_literal("476c578d42cd0a3e56040b2a185c15e454949c4815dfd100b4876ae2a8b78f09")}, // BaseVobCtor
  {0x77b6a0,18,lhiver::digest_literal("9d7b0f5d244ac474db37d22a14d937f02f414709400b177e4426631de7c1e7b4")}, // GameVobCtor
  {0x793470,177,lhiver::digest_literal("036f2567c8ccc2ec2468c7660f4ded761faf8705cabcb071208eb16b3ec1bfbe")}, // ParserFind
  {0x7a3b60,227,lhiver::digest_literal("f6d73d392bc8a4a6c9c8140fc1b23f41dcd6e7473af45f1a49e84ed3520b40df")}, // SymtabFind
  {0x601c40,19,lhiver::digest_literal("ef653f8e422268bbe523288ad1dcca1384345a7d1e54392cef8a074c3e4fd333")}, // RemoveVobFromWorld: homeWorld +B8
  {0x77ef50,30,lhiver::digest_literal("266523792825be10b2e534b0751d000fba7818170af53be8a51b0784161c1534")}, // WorldSort
  {0x77ef90,32,lhiver::digest_literal("1854c9e026d1a48239697368b4c2ba82bf68e84729dbe1dfe429959920c18496")}, // ItemSort
  {0x712880,69,lhiver::digest_literal("979f5190f2e848817ee21a6c4886b13e69e4a34e35b07d553562b3938b889ca1")}, // ItemDescription
  {0x7800c0,616,lhiver::digest_literal("a4fb7b760995a075d19288fc96f659646018e6a99a04d4e67023148d3d0499d8")}, // WorldRemove
  {0x780990,572,lhiver::digest_literal("96b3280861015b9b6d3311925d3b212014db02f730e948075c0c636e4a80cb5e")}, // WorldRemoveLists
}};
inline constexpr std::array<lhiver::SymbolBinding,14> constructor_fields{{
  {"C_ITEM.NAME",1586,3,4,1,1584,false,0,true,292},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"C_ITEM.VALUE",1593,2,4,1,1584,false,0,true,352},
  {"C_ITEM.DAMAGETYPE",1594,2,4,1,1584,false,0,true,356},
  {"C_ITEM.DAMAGETOTAL",1595,2,4,1,1584,false,0,true,360},
  {"C_ITEM.COND_ATR",1600,2,4,3,1584,false,0,true,436},
  {"C_ITEM.COND_VALUE",1601,2,4,3,1584,false,0,true,448},
  {"C_ITEM.VISUAL",1611,3,4,1,1584,false,0,true,524},
  {"C_ITEM.MATERIAL",1616,2,4,1,1584,false,0,true,608},
  {"C_ITEM.MUNITION",1617,2,4,1,1584,false,0,true,612},
  {"C_ITEM.DESCRIPTION",1621,3,4,1,1584,false,0,true,628},
  {"C_ITEM.TEXT",1622,3,4,6,1584,false,0,true,648},
  {"C_ITEM.COUNT",1623,2,4,6,1584,false,0,true,768},
}};
struct Visual {std::uint32_t item,symbol,length;lhiver::Digest digest;};
inline constexpr std::array<Visual,42> visuals{{
  {16599,46434,19,lhiver::digest_literal("1db98ee54e81c6d7ddd5a7f14dd5afc288962a83c153fe5b9083db4846fbf7e4")},
  {16610,46460,16,lhiver::digest_literal("edce1961ca5185875b52a5a9c15ebf65a4752d155219b2e5bddd8c0f2e098e35")},
  {16806,46781,21,lhiver::digest_literal("ebc6ca6af9748de0c508e10e5a9d36bbd2ae30c856c1a59fe7c926e31f2ad033")},
  {16807,46783,21,lhiver::digest_literal("ebc6ca6af9748de0c508e10e5a9d36bbd2ae30c856c1a59fe7c926e31f2ad033")},
  {16808,46785,16,lhiver::digest_literal("adae071915875bf7d59b808b1fcbf5179791c17f5eda31c24ec982d15bc965b6")},
  {16809,46787,16,lhiver::digest_literal("adae071915875bf7d59b808b1fcbf5179791c17f5eda31c24ec982d15bc965b6")},
  {16810,46789,17,lhiver::digest_literal("0344a1bfe1a69e252c99d7b2e532aa1d67be16114730124d8a9344002fe27179")},
  {16811,46791,17,lhiver::digest_literal("0344a1bfe1a69e252c99d7b2e532aa1d67be16114730124d8a9344002fe27179")},
  {16812,46793,17,lhiver::digest_literal("f66dfa01eb4c1da01b28e086f7253fbec00e496f9f944338529e90ae56984fa6")},
  {16813,46795,17,lhiver::digest_literal("f66dfa01eb4c1da01b28e086f7253fbec00e496f9f944338529e90ae56984fa6")},
  {16814,46797,17,lhiver::digest_literal("7e3edaed3d5f37c8b170f54d6ddf0eebfa7f0cb5584145b84e84df63b166b51b")},
  {16815,46799,17,lhiver::digest_literal("7e3edaed3d5f37c8b170f54d6ddf0eebfa7f0cb5584145b84e84df63b166b51b")},
  {16816,46801,17,lhiver::digest_literal("4e5db032c464f118ef7e90e3ee8de86f849d12e6056f32b98387f36c22bcb202")},
  {16817,46803,17,lhiver::digest_literal("4e5db032c464f118ef7e90e3ee8de86f849d12e6056f32b98387f36c22bcb202")},
  {16818,46805,17,lhiver::digest_literal("7e3edaed3d5f37c8b170f54d6ddf0eebfa7f0cb5584145b84e84df63b166b51b")},
  {16819,46807,17,lhiver::digest_literal("7e3edaed3d5f37c8b170f54d6ddf0eebfa7f0cb5584145b84e84df63b166b51b")},
  {16820,46809,17,lhiver::digest_literal("4e5db032c464f118ef7e90e3ee8de86f849d12e6056f32b98387f36c22bcb202")},
  {16821,46811,17,lhiver::digest_literal("4e5db032c464f118ef7e90e3ee8de86f849d12e6056f32b98387f36c22bcb202")},
  {16822,46813,17,lhiver::digest_literal("6ef06811b56ebdd02832e93060d19c73ba4c7687bdd33d9dfa531a28ea743720")},
  {16823,46815,17,lhiver::digest_literal("6ef06811b56ebdd02832e93060d19c73ba4c7687bdd33d9dfa531a28ea743720")},
  {16824,46817,17,lhiver::digest_literal("975e164f435c8b80cb88d9a2c381f3ec3b596e0ede0644b4b76723ec1761fedb")},
  {16825,46819,17,lhiver::digest_literal("975e164f435c8b80cb88d9a2c381f3ec3b596e0ede0644b4b76723ec1761fedb")},
  {16826,46821,17,lhiver::digest_literal("529f89d9a6b13d6519cfdd34da21aae0923e519ce822a30c481df09ce0bd1e52")},
  {16827,46823,17,lhiver::digest_literal("529f89d9a6b13d6519cfdd34da21aae0923e519ce822a30c481df09ce0bd1e52")},
  {16828,46825,17,lhiver::digest_literal("9e7ae94f6e2f57d44b8170bc46b83943fcd156ddd8a233b25ff38ab91b9fd1ed")},
  {16829,46827,17,lhiver::digest_literal("9e7ae94f6e2f57d44b8170bc46b83943fcd156ddd8a233b25ff38ab91b9fd1ed")},
  {16830,46829,17,lhiver::digest_literal("72a1c2ee75e635ade0f8ffb22dda8abf2429ec03db34dd6a5c0cc4b22c6a10ae")},
  {16831,46831,17,lhiver::digest_literal("72a1c2ee75e635ade0f8ffb22dda8abf2429ec03db34dd6a5c0cc4b22c6a10ae")},
  {16832,46833,17,lhiver::digest_literal("9ef83c457030ebc5dde9c3cea7c33d1c11755af0cc5cde5e9bc3d579f5b4df0b")},
  {16833,46835,17,lhiver::digest_literal("9ef83c457030ebc5dde9c3cea7c33d1c11755af0cc5cde5e9bc3d579f5b4df0b")},
  {16834,46837,22,lhiver::digest_literal("d38fdbed4efbfc713e8b67ee0b7b01760220d752b015df36ef76e01406ec7b96")},
  {16835,46839,22,lhiver::digest_literal("d38fdbed4efbfc713e8b67ee0b7b01760220d752b015df36ef76e01406ec7b96")},
  {16836,46841,22,lhiver::digest_literal("ea5681b8583938aa42ba733f9396bec0ce88c29afe921d8038cb3d38b0ff792c")},
  {16837,46843,22,lhiver::digest_literal("ea5681b8583938aa42ba733f9396bec0ce88c29afe921d8038cb3d38b0ff792c")},
  {16838,46845,22,lhiver::digest_literal("a852b200120e8256eb182dee71a8a9fa9ed21f984192acb05348b6547eade899")},
  {16839,46847,22,lhiver::digest_literal("a852b200120e8256eb182dee71a8a9fa9ed21f984192acb05348b6547eade899")},
  {16840,46849,22,lhiver::digest_literal("515e3ea649d66a03347436c7d31c546f9a5ee862692eb4fa1f50e0779d1dfe9a")},
  {16841,46851,22,lhiver::digest_literal("515e3ea649d66a03347436c7d31c546f9a5ee862692eb4fa1f50e0779d1dfe9a")},
  {16842,46853,22,lhiver::digest_literal("e2246a05dd3ae5c0dc7de63789ee070c392a1337cedcc14b842d6aed43f9cb93")},
  {16843,46855,22,lhiver::digest_literal("e2246a05dd3ae5c0dc7de63789ee070c392a1337cedcc14b842d6aed43f9cb93")},
  {16844,46857,22,lhiver::digest_literal("dfab697c32bd8d3835c3ef5f1fbf492e556f8b97ae960b025b641845dc35aac0")},
  {16845,46859,22,lhiver::digest_literal("dfab697c32bd8d3835c3ef5f1fbf492e556f8b97ae960b025b641845dc35aac0")},
}};
inline constexpr std::array<std::uint16_t,6> hasitems_relocations{{7,38,50,106,115,147}};
inline constexpr std::array<std::uint16_t,1> cookie_relocations{{2}};
inline constexpr std::array<zbug_end_compat::CodeRange,6> zbug_ranges{{
  {0x12120,343,lhiver::digest_literal("c3fd08b4919fb0bb195c0f664b5d7cc1a200176e446bd88981945c59067b0629"),hasitems_relocations.data(),hasitems_relocations.size()},
  {0x9aa0,23,lhiver::digest_literal("c2c25d6f7b3f0dcb9e57d0104ff9c654fc6b26a6eeb720aa36aeb1f701be1384")},
  {0x9ae0,21,lhiver::digest_literal("d728a03f37acfd58ad47497288bfe32e07d047f17f527913a3e536625619555d")},
  {0x9b00,23,lhiver::digest_literal("a0a1a441b273d8aa4d104e47222b14840156b532ab684514a88adf917728dd77")},
  {0x92e0,21,lhiver::digest_literal("8ce027b17bf72f1ec1b956c646eceef6e383e43e3550fc6c3be8a46e562d8e52")},
  {0x2297a,14,lhiver::digest_literal("e859e10cdacddac39e8ddf08e5a76670f975d08594f792e660fef72a5b18e1e5"),cookie_relocations.data(),cookie_relocations.size()},
}};
namespace detail {
inline Check fail(Issue issue,std::uint32_t address=0) noexcept {return {false,issue,address};}
inline bool range(std::uint32_t a,std::size_t n) noexcept {return rs::detail::range(a,n);}
inline bool word(const ReadOnlyMemory& m,std::uint32_t a,std::uint32_t o,std::uint32_t& v) noexcept {return path::detail::word(m,a,o,v);}
inline Check plain(const ReadOnlyMemory& m,const CodeRange& c) noexcept {
  if(!c.length||c.length>dep::code_capacity)return fail(Issue::range,c.address);
  const auto r=dep::detail::plain(m,{c.address,c.length,c.digest,dep::RangeKind::complete_function});
  return r.valid?Check{true,Issue::none,0}:fail(r.issue==dep::Issue::changed?Issue::changed:Issue::code,c.address);
}
inline void word_hash(Sha256& h,std::uint32_t v) noexcept {path::detail::hash_word(h,v);}
inline bool read_hash(const ReadOnlyMemory& m,std::uint32_t a,void* p,std::size_t n,Sha256& hash) noexcept {
  if(!range(a,n)||!m.copy(a,p,n))return false;word_hash(hash,a);hash.update(p,n);return true;
}
} // namespace detail

// WorldRemove and WorldRemoveLists have renderer detours in the verified
// installation. They belong to bow_item_renderer, not to the plain path.
inline Check verify_engine_parts(const ReadOnlyMemory& memory) noexcept {
  for(unsigned pass=0;pass<2;++pass)for(std::size_t i=0;i+2<engine_ranges.size();++i) {
    const auto c=detail::plain(memory,engine_ranges[i]);if(!c.verified)return c;
  }
  return {true,Issue::none,0};
}

// The concrete ZBug-HasItems replacement: counts each listed target once
// and materializes canonical pack entries via the ORIGINAL 70F8F0 thunk.
// No statement about GetAmount/IsIn/Remove-pack fallback or PutInInv here.
namespace detail {
// The first foreign entry belongs to the held module. Own source area
// and actually read target address remain bound across all checks.
inline Check forwarded(const zbug_end_compat::Module& module,std::uint32_t source,std::uint32_t target,
    std::uint32_t original,std::uint32_t& previous) noexcept {
  if(target!=original&&(module.file_sha256==lhiver::Digest{}||
     !zbug_end_compat::detail::forwards_to(module,target)))return fail(Issue::dispatch,source);
  if(previous&&previous!=target)return fail(Issue::changed,source);
  previous=target;
  return {true,Issue::none,0};
}
enum class PackedHasItems { original_only, observed_tobacco_wrapper };
inline Check bow_unpack(const ReadOnlyMemory& memory,const zbug_end_compat::Module& module,BowDispatch& bow,
    PackedHasItems packed=PackedHasItems::original_only) noexcept {
  const auto& thunk=grind_zbug_unpack::thunk;
  const auto address=module.base+thunk.rva;
  std::array<std::uint8_t,grind_zbug_unpack::thunk.length> bytes{};
  if(!dep::detail::twice(memory,address,bytes.data(),bytes.size()).valid)return fail(Issue::unreadable,address);
  const auto target=rs::detail::u32(bytes.data()+11);
  dep::detail::put(bytes.data()+11,grind_zbug_unpack::original_target);
  if(dep::detail::digest(bytes.data(),bytes.size())!=thunk.sha256)return fail(Issue::code,address);
  // In tobacco world run 1, +11E30 was verified before and after the choice, even
  // with a nonempty packed inventory. Authorization is bound to that specific file.
  const bool known_packed=packed==PackedHasItems::observed_tobacco_wrapper&&
    module.file_sha256==zbug_end_compat::zbug_file_sha256&&target==module.base+grind_zbug_unpack::wrapper_rva;
  if(!bow.empty_pack&&target!=grind_zbug_unpack::original_target&&!known_packed)return fail(Issue::boundary,address);
  return forwarded(module,address,target,grind_zbug_unpack::original_target,bow.unpack_target);
}
inline Check hasitems(const ReadOnlyMemory& memory,const dep::Modules& modules,
    const GrindRecipeKey* grind_key=nullptr,grind_zbug_unpack::Verification* unpack=nullptr,BowDispatch* bow=nullptr,
    PackedHasItems packed=PackedHasItems::original_only) noexcept {
  if(unpack)*unpack={};
  if(!dep::detail::modules(memory,modules).valid)return detail::fail(Issue::module);
  std::array<std::uint32_t,5> first{};
  grind_zbug_unpack::Verification initial_unpack{};
  for(unsigned pass=0;pass<2;++pass) {
    bool patched=false;const auto& area=bow_item_code::ranges[0];
    const auto call=dep::detail::patched_call(memory,{area.address,area.length,area.digest,dep::RangeKind::complete_function},
      0xd0,0x70c970,modules.zbug.base+0x12120,patched);
    if(!call.valid||!patched)return detail::fail(Issue::dispatch,0x6e73b0);
    for(const auto& r:zbug_ranges){
      if(bow&&r.rva==grind_zbug_unpack::thunk_rva){
        const auto checked=bow_unpack(memory,modules.zbug,*bow,packed);
        if(!checked.verified)return checked;
      }else if(grind_key&&r.rva==grind_zbug_unpack::thunk_rva){
        const auto current=grind_zbug_unpack::verify(memory,{modules.zbug,modules.union_core},*grind_key);
        if(!current.valid)return detail::fail(current.issue==grind_zbug_unpack::Issue::changed?Issue::changed:Issue::code,current.address);
        if(pass&&!grind_zbug_unpack::same(initial_unpack,current))return detail::fail(Issue::changed,modules.zbug.base+r.rva);
        initial_unpack=current;
      }else if(!zbug_end_compat::detail::verify_code(memory,modules.zbug,r).valid)
        return detail::fail(Issue::code,modules.zbug.base+r.rva);
    }
    std::array<std::uint32_t,5> values{};
    if(!detail::word(memory,modules.zbug.base,0x45130,values[0])||
       !detail::word(memory,modules.zbug.base,0x45164,values[1])||
       !detail::word(memory,modules.zbug.base,0x59840,values[2])||
       values[0]!=modules.union_core.base+0x3e96c8||values[1]!=modules.union_core.base+0x2d670||
       values[2]!=lhiver::game_parser||!detail::word(memory,values[0],8,values[3])||values[3]!=4||
       !detail::word(memory,modules.zbug.base,0x59014,values[4])||!values[4])
      return detail::fail(Issue::dispatch);
    const zbug_end_compat::CodeRange version{0x2d670,4,lhiver::digest_literal("916ae10aa3fd76a4d5b25b2fb66e9df0fd04957c7ab10930268d1ed7808be689")};
    if(!zbug_end_compat::detail::verify_code(memory,modules.union_core,version).valid)return detail::fail(Issue::code);
    if(pass&&values!=first)return detail::fail(Issue::changed);first=values;
  }
  if(unpack)*unpack=initial_unpack;
  return {true,Issue::none,0};
}
} // namespace detail
inline Check verify_zbug_hasitems(const ReadOnlyMemory& memory,const dep::Modules& modules) noexcept {
  return detail::hasitems(memory,modules);
}

// Narrow current code alternative to verify_original_parts: mask exactly
// the locally verified ZBug CALLs and no other fields. The HasItems
// replacement bypasses GetAmount. The pack fallbacks in IsIn/Remove
// become unreachable only after three successful minimum checks that
// materialize items. snapshot is the freshly bound canonical initial
// stock. Only the previously verified shared code/module route is
// covered. Each station's caller must bind its own material, quantity and
// lookup paths. This partial result does not authorize a native effect.
namespace detail {
inline Check routed_code(const ReadOnlyMemory& memory,const dep::Modules& modules,
    const vob_inventory_insert_compat::Binding* insert_binding,
    vob_inventory_insert_compat::Verification* insert_evidence=nullptr,
    const GrindRecipeKey* grind_key=nullptr,grind_zbug_unpack::Verification* unpack_evidence=nullptr,
    BowDispatch* bow=nullptr) noexcept {
  if(insert_evidence)*insert_evidence={};
  if(unpack_evidence)*unpack_evidence={};
  const auto held_insert=insert_binding?*insert_binding:vob_inventory_insert_compat::Binding{};
  const auto held_key=grind_key?*grind_key:GrindRecipeKey{};
  const auto* key=grind_key?&held_key:nullptr;
  grind_zbug_unpack::Verification initial_unpack{};
  const auto has=detail::hasitems(memory,modules,key,&initial_unpack,bow);if(!has.verified)return has;
  // Already separated, fully verified ZBug original dispatch.
  // Its endcall evidence is not issued as a new bow end witness.
  if(!zbug_end_compat::verify(memory,{modules.zbug,modules.union_core}).valid)return detail::fail(Issue::dispatch);
  vob_inventory_insert_compat::Verification initial_insert{};
  for(unsigned pass=0;pass<2;++pass){
    for(std::size_t i=1;i<bow_item_code::ranges.size();++i){const auto& area=bow_item_code::ranges[i];
      if(i==3||i==4){bool patched=false;
        const auto checked=dep::detail::patched_call(memory,{area.address,area.length,area.digest,dep::RangeKind::complete_function},
          i==3?0x91u:0x76u,0x70f8f0,modules.zbug.base+0x11e30,patched);
        if(!checked.valid)return detail::fail(Issue::code,area.address);
      }else if(i==6){
        std::array<std::uint8_t,dep::code_capacity> bytes{};
        if(!dep::detail::twice(memory,area.address,bytes.data(),area.length).valid)return detail::fail(Issue::unreadable,area.address);
        for(const auto offset:{0x57u,0x7au,0xd0u}){
          if(bytes[offset]!=0xe8||area.address+offset+5u+rs::detail::u32(bytes.data()+offset+1)!=modules.zbug.base+0x11c80)return detail::fail(Issue::dispatch,area.address+offset);
          dep::detail::put(bytes.data()+offset+1,0x749350u-area.address-offset-5u);
        }
        if(dep::detail::digest(bytes.data(),area.length)!=area.digest)return detail::fail(Issue::code,area.address);
      }else{const auto checked=detail::plain(memory,area);if(!checked.verified)return checked;}
    }
    for(const auto& slot:bow_item_code::slots){
      if(bow&&slot.address==vob_inventory_insert_compat::slot_address){
        std::uint32_t target=0;
        if(!word(memory,slot.address,0,target))return fail(Issue::unreadable,slot.address);
        const auto checked=forwarded(bow->locator,slot.address,target,vob_inventory_insert_compat::original_target,bow->insert_target);
        if(!checked.verified)return checked;
      }else if(insert_binding&&slot.address==vob_inventory_insert_compat::slot_address){
        const auto current=vob_inventory_insert_compat::verify(memory,&held_insert);
        if(!current.valid)return detail::fail(Issue::dispatch,current.address);
        if(current.route==vob_inventory_insert_compat::Route::vob_direct&&
          !vob_inventory_insert_compat::same_module(held_insert.union_core,modules.union_core))
          return detail::fail(Issue::dispatch,slot.address);
        if(pass&&!vob_inventory_insert_compat::same(initial_insert,current))return detail::fail(Issue::changed,slot.address);
        initial_insert=current;
      }else if(!dep::detail::slot(memory,slot).valid)return detail::fail(Issue::dispatch,slot.address);
    }
  }
  grind_zbug_unpack::Verification final_unpack{};
  const auto final=detail::hasitems(memory,modules,key,&final_unpack,bow);
  if(final.verified&&key&&!grind_zbug_unpack::same(initial_unpack,final_unpack))return detail::fail(Issue::changed,modules.zbug.base+grind_zbug_unpack::thunk_rva);
  if(final.verified&&insert_evidence)*insert_evidence=initial_insert;
  if(final.verified&&unpack_evidence)*unpack_evidence=initial_unpack;
  return final;
}
} // namespace detail
// All existing callers remain on the strict original vslot.
inline Check verify_routed_code(const ReadOnlyMemory& memory,const dep::Modules& modules) noexcept {
  return detail::routed_code(memory,modules,nullptr);
}
// Explicit module values only; the reader is re-executed
// in both full code passes and its concrete route is bound.
inline Check verify_routed_code(const ReadOnlyMemory& memory,const dep::Modules& modules,
    const vob_inventory_insert_compat::Binding& insert_binding,
    vob_inventory_insert_compat::Verification* insert_evidence=nullptr) noexcept {
  return detail::routed_code(memory,modules,&insert_binding,insert_evidence);
}
// New alternative exclusively with fresh concrete Grind-HERO and
// empty entire pack string. Both prior Bow entries remain strict.
inline Check verify_grind_routed_code(const ReadOnlyMemory& memory,const dep::Modules& modules,
    const GrindRecipeKey& key,const vob_inventory_insert_compat::Binding& insert_binding,
    vob_inventory_insert_compat::Verification* insert_evidence,
    grind_zbug_unpack::Verification* unpack_evidence) noexcept {
  return detail::routed_code(memory,modules,&insert_binding,insert_evidence,&key,unpack_evidence);
}

inline Check verify_routed_parts(const ReadOnlyMemory& memory,const dep::Modules& modules,
    const path::Snapshot& snapshot,BowDispatch* bow=nullptr) noexcept {
  const auto* descriptor=cat::find(snapshot.plan.id);
  if(!descriptor||!snapshot.captured||!snapshot.plan.valid||!path::detail::boundary(snapshot.boundary))return detail::fail(Issue::boundary);
  const auto expected=path::make_plan(*descriptor,snapshot.plan.quantity);
  auto operation=[](const path::Operation& a,const path::Operation& b) noexcept {return a.symbol==b.symbol&&a.amount==b.amount&&a.offset==b.offset;};
  if(!expected.valid||snapshot.plan.callback!=expected.callback||!operation(snapshot.plan.output,expected.output))return detail::fail(Issue::constructor);
  for(std::size_t i=0;i<3;++i){
    if(!operation(snapshot.plan.minima[i],expected.minima[i])||!operation(snapshot.plan.removals[i],expected.removals[i]))return detail::fail(Issue::constructor);
    const auto index=path::detail::target(*descriptor,expected.minima[i].symbol);
    if(index==descriptor->targets.count)return detail::fail(Issue::constructor);
    const auto& material=snapshot.materials[index];
    if(material.symbol!=expected.minima[i].symbol||material.listed<0||material.packed<0||
       std::int64_t{material.listed}+material.packed!=material.total||
       (material.listed&&material.packed)||material.total<static_cast<std::int32_t>(expected.minima[i].amount))return detail::fail(Issue::boundary);
    for(const auto& remove:expected.removals)if(remove.symbol==material.symbol&&material.total<static_cast<std::int32_t>(remove.amount))return detail::fail(Issue::boundary);
  }
  if(bow)bow->empty_pack=snapshot.packed_entries==0;
  return detail::routed_code(memory,modules,nullptr,nullptr,nullptr,nullptr,bow);
}

struct ConstructorEvidence {bool values_verified=false;Issue issue=Issue::none;std::uint32_t symbol=0;lhiver::Digest fingerprint{};};
// Complete native field targets of ALL write commands of the 42 canonical
// constructors plus the actual .3DS/.MMS visual values. No .ZEN-,
// SetVisual, or Level-load branch can arise from these hashed values.
// The function also checks the four real constructor bytecode bodies.
inline ConstructorEvidence verify_constructor_values(const ReadOnlyMemory& memory,const cat::Descriptor& descriptor) noexcept {
  ConstructorEvidence out;if(!cat::structurally_valid(descriptor)){out.issue=Issue::constructor;return out;}
  const auto& d=*cat::find(descriptor.id);const auto assembly=bow_recipe_runtime::assemble(d);const auto profile=assembly.profile();
  rs::detail::Layout layout{};if(rs::detail::read_layout(memory,lhiver::game_parser,profile,layout)!=rs::Issue::none){out.issue=Issue::constructor;return out;}
  lhiver::Digest initial{};
  for(unsigned pass=0;pass<2;++pass) {
    Sha256 hash;
    for(const auto& b:constructor_fields)if(rs::detail::check_binding(memory,layout,b)!=rs::Issue::none){out.issue=Issue::constructor;out.symbol=b.symbol;return out;}
    for(const auto& item:d.items) {
      if(!item.constructor||item.constructor_calls||rs::detail::check_function(memory,layout,*item.constructor)!=rs::Issue::none){out.issue=Issue::constructor;out.symbol=static_cast<std::uint32_t>(item.symbol);return out;}
      const Visual* wanted=nullptr;for(const auto& v:visuals)if(v.item==static_cast<std::uint32_t>(item.symbol)){wanted=&v;break;}
      rs::detail::Symbol s{};std::array<std::uint8_t,20> header{};std::array<std::uint8_t,64> bytes{};
      if(!wanted||wanted->length>=bytes.size()||rs::detail::lookup(memory,layout,wanted->symbol,s)!=rs::Issue::none||
         s.type!=3||s.flags!=1||s.elements!=1||s.parent||!detail::read_hash(memory,s.address,header.data(),header.size(),hash)){
        out.issue=Issue::string;out.symbol=wanted?wanted->symbol:0;return out;}
      // Symbol.data is the zSTRING, not its Char buffer.
      if(!detail::read_hash(memory,s.data,header.data(),header.size(),hash)||rs::detail::u32(header.data()+12)!=wanted->length||
         !detail::read_hash(memory,rs::detail::u32(header.data()+8),bytes.data(),wanted->length+1,hash)||bytes[wanted->length]||
         std::memchr(bytes.data(),0,wanted->length)||dep::detail::digest(bytes.data(),wanted->length)!=wanted->digest){
        out.issue=Issue::string;out.symbol=wanted->symbol;return out;}
      detail::word_hash(hash,s.address);detail::word_hash(hash,s.data);detail::word_hash(hash,s.offset);
    }
    std::array<std::uint32_t,2> globals{};
    if(!detail::word(memory,0x8b7464,0,globals[0])||globals[0]!=1584||
       !detail::word(memory,0xab11e4,0,globals[1])||globals[1]!=1){out.issue=Issue::constructor;return out;}
    for(const auto v:globals)detail::word_hash(hash,v);
    std::array<std::uint8_t,5> needle{};
    if(!detail::read_hash(memory,0x8922a4,needle.data(),needle.size(),hash)||std::memcmp(needle.data(),".ZEN",5)){out.issue=Issue::string;return out;}
    const auto digest=hash.finish();if(pass&&digest!=initial){out.issue=Issue::changed;return out;}initial=digest;
  }
  if(rs::detail::stable_layout(memory,lhiver::game_parser,profile,layout)!=rs::Issue::none){out.issue=Issue::changed;return out;}
  out.values_verified=true;out.fingerprint=initial;return out;
}

inline constexpr std::uint32_t node_pool=0x8d83b4;
inline constexpr std::size_t max_allocations=10; // 3 Unpack + 2*3 world lists + 1 output, NOT recipe count.
struct PoolEvidence {
  bool stable=false,reuse_path=false,growth_required=false;
  Issue issue=Issue::none;std::uint32_t required=0,free_count=0;
  std::array<std::uint32_t,max_allocations> prefix{};
  lhiver::Digest fingerprint{};
};
inline std::uint32_t maximum_allocations(const path::Snapshot& snapshot) noexcept {
  if(!snapshot.captured||!snapshot.plan.valid)return 0;
  std::uint32_t n=snapshot.requirements.new_output_node?1u:0u;
  for(const auto& step:snapshot.removals){n+=2;if(step.source==path::Source::packed)++n;}
  return n;
}
// Positive reuse of existing pool nodes; Growth is reported
// separately. An unknown malloc/heap limit is not positively guessed.
inline PoolEvidence read_pool_allocations(const ReadOnlyMemory& memory,std::uint32_t required) noexcept {
  PoolEvidence out;out.required=required;
  if(!out.required||out.required>max_allocations){out.issue=Issue::boundary;return out;}
  lhiver::Digest first{};
  for(unsigned pass=0;pass<2;++pass) {
    Sha256 hash;std::array<std::uint8_t,20> pool{};
    std::uint32_t debug=0,site=0;
    if(!detail::read_hash(memory,node_pool,pool.data(),pool.size(),hash)||!detail::word(memory,0x8a2558,0,debug)||
       !detail::word(memory,0x8d8780,0,site)){out.issue=Issue::unreadable;return out;}
    if(debug||site||rs::detail::u32(pool.data())!=12||rs::detail::u32(pool.data()+4)!=4096){out.issue=Issue::pool;return out;}
    detail::word_hash(hash,debug);detail::word_hash(hash,site);
    const auto free=rs::detail::u32(pool.data()+16);auto node=rs::detail::u32(pool.data()+12);
    if(free>INT32_MAX||(!node)!=(!free)){out.issue=Issue::pool;return out;}
    out.free_count=free;out.growth_required=free<out.required;
    const auto count=free<out.required?free:out.required;auto backlink=node_pool+12;
    for(std::uint32_t i=0;i<count;++i) {
      std::array<std::uint8_t,8> links{};if(!node||node%4||!detail::read_hash(memory,node,links.data(),links.size(),hash)){out.issue=Issue::pool;return out;}
      for(std::uint32_t j=0;j<i;++j)if(out.prefix[j]==node){out.issue=Issue::alias;return out;}
      if(rs::detail::u32(links.data()+4)!=backlink){out.issue=Issue::pool;return out;}
      if(pass&&out.prefix[i]!=node){out.issue=Issue::changed;return out;}out.prefix[i]=node;
      backlink=node;node=rs::detail::u32(links.data());
    }
    if(free==count&&node){out.issue=Issue::pool;return out;}
    // Alloc reads the next node to implement its prev-link.
    if(node){std::uint32_t next_back=0;if(!detail::word(memory,node,4,next_back)||next_back!=backlink){out.issue=Issue::pool;return out;}
      detail::word_hash(hash,node);detail::word_hash(hash,next_back);}
    const auto current=hash.finish();if(pass&&current!=first){out.issue=Issue::changed;return out;}first=current;
  }
  out.stable=true;out.reuse_path=!out.growth_required;out.fingerprint=first;return out;
}
inline PoolEvidence read_pool(const ReadOnlyMemory& memory,const path::Snapshot& snapshot) noexcept {
  if(!path::detail::boundary(snapshot.boundary)){PoolEvidence out;out.issue=Issue::boundary;return out;}
  return read_pool_allocations(memory,maximum_allocations(snapshot));
}
struct PoolPath {bool verified=false,one_growth=false;Issue issue=Issue::none;std::uint32_t required=0;lhiver::Digest fingerprint{};};
// Growth needs exactly one 8-byte block header and 4096*12 data bytes. The
// current Union->CRT export/IAT binding must be positive. This is the same
// explicit CRT trust limit as dep, NO promise of successful allocation
// and no own reservation. Normal engine errors remain result errors.
inline PoolPath verify_pool_allocations(const ReadOnlyMemory& memory,const dep::Modules& modules,std::uint32_t required) noexcept {
  PoolPath out;const auto before=read_pool_allocations(memory,required);if(!before.stable){out.issue=before.issue;return out;}
  for(const auto index:{0u,1u}){const auto c=detail::plain(memory,engine_ranges[index]);if(!c.verified){out.issue=c.issue;return out;}}
  dep::PendingFacts allocator{};
  if(before.growth_required){
    for(const auto index:{11u,12u})if(!dep::detail::plain(memory,dep::additional_ranges[index]).valid){out.issue=Issue::code;return out;}
    allocator=dep::read_pending_facts(memory,modules);
    if(!allocator.read_stable||!allocator.allocator_boundary_verified){out.issue=Issue::dispatch;return out;}
  }
  const auto after=read_pool_allocations(memory,required);
  if(!after.stable||after.fingerprint!=before.fingerprint||after.required!=before.required){out.issue=Issue::changed;return out;}
  if(before.growth_required){const auto again=dep::read_pending_facts(memory,modules);
    if(!again.read_stable||!again.allocator_boundary_verified||again.allocator_entry!=allocator.allocator_entry||
       again.allocator_forward!=allocator.allocator_forward||!dep::same(again.item,allocator.item)){out.issue=Issue::changed;return out;}}
  out.verified=true;out.one_growth=before.growth_required;out.required=before.required;out.fingerprint=before.fingerprint;return out;
}
inline PoolPath verify_pool_path(const ReadOnlyMemory& memory,const dep::Modules& modules,const path::Snapshot& snapshot) noexcept {
  if(!path::detail::boundary(snapshot.boundary)){PoolPath out;out.issue=Issue::boundary;return out;}
  return verify_pool_allocations(memory,modules,maximum_allocations(snapshot));
}
struct SortEvidence {bool stable=false;Issue issue=Issue::none;std::uint32_t nodes=0;lhiver::Digest fingerprint{};};
namespace detail {
inline bool symbol_name(const ReadOnlyMemory& m,const rs::detail::Layout& layout,std::uint32_t index,
    std::array<std::uint8_t,128>& bytes,std::uint32_t& length,Sha256& hash) noexcept {
  rs::detail::Symbol s{};if(rs::detail::lookup(m,layout,index,s)!=rs::Issue::none||!s.name_length||s.name_length>=bytes.size())return false;
  length=s.name_length;if(!read_hash(m,s.name_data,bytes.data(),length+1,hash)||bytes[length]||std::memchr(bytes.data(),0,length))return false;
  // No output of original names. The native comparison needs only a
  // positive, finite buffer; also non-recipe-related instances are read.
  word_hash(hash,index);word_hash(hash,s.address);word_hash(hash,s.name_data);word_hash(hash,length);return true;
}
inline bool resolves(const ReadOnlyMemory& m,const rs::detail::Layout& layout,std::uint32_t sorted,
    const PackedInventoryTarget& wanted,Sha256& hash) noexcept {
  std::size_t length=0;while(length<64&&wanted.name[length])++length;if(!length||length==64)return false;
  std::int64_t low=0,high=static_cast<std::int64_t>(layout.count)-1;
  for(unsigned step=0;step<32&&low<=high;++step){
    const auto middle=static_cast<std::uint32_t>(low+(high-low)/2);std::uint32_t index=0;
    if(!word(m,sorted,middle*4u,index)||index>=layout.count)return false;word_hash(hash,middle);word_hash(hash,index);
    std::array<std::uint8_t,128> name{};std::uint32_t size=0;if(!symbol_name(m,layout,index,name,size,hash))return false;
    const auto common=length<size?length:size;int comparison=std::memcmp(wanted.name,name.data(),common);
    if(!comparison)comparison=length<size?-1:length>size?1:0;
    if(!comparison)return index==static_cast<std::uint32_t>(wanted.symbol);
    if(comparison<0)high=static_cast<std::int64_t>(middle)-1;else low=static_cast<std::int64_t>(middle)+1;
  }
  return false;
}
}
// Canonical comparator, its current category order, all actually
// listed GetInstanceName buffers plus exactly the four native
// FindSymbol search paths. No engine HasItems/FindSymbol is executed.
inline SortEvidence read_sort_values(const ReadOnlyMemory& memory,const lhiver::Profile& profile,
    std::uint32_t hero,std::uint32_t listed_nodes,const PackedInventoryTarget* targets,std::size_t target_count) noexcept {
  SortEvidence out;
  if(!hero||hero>UINT32_MAX-0x1000u||listed_nodes>max_packed_inventory_nodes||!targets||!target_count||target_count>45||
     !rs::detail::valid_profile(profile)){out.issue=Issue::boundary;return out;}
  rs::detail::Layout layout{};if(rs::detail::read_layout(memory,lhiver::game_parser,profile,layout)!=rs::Issue::none){out.issue=Issue::lookup;return out;}
  lhiver::Digest first{};
  for(unsigned pass=0;pass<2;++pass){
    Sha256 hash;std::array<std::uint8_t,36> order{};std::uint32_t seen=0,sorted=0,compare=0,head=0;
    if(!detail::read_hash(memory,0x8b6be0,order.data(),order.size(),hash)||
       !detail::word(memory,lhiver::game_parser,0x24,sorted)||!detail::range(sorted,std::size_t{layout.count}*4)||
       !detail::word(memory,hero,0x710,compare)||compare!=path::sorter||
       !detail::word(memory,hero,0x718,head)){out.issue=Issue::sorter;return out;}
    for(unsigned i=0;i<9;++i){const auto value=rs::detail::u32(order.data()+i*4);if(value>8||(seen&(1u<<value))){out.issue=Issue::sorter;return out;}seen|=1u<<value;}
    detail::word_hash(hash,sorted);detail::word_hash(hash,compare);detail::word_hash(hash,head);
    for(std::size_t i=0;i<target_count;++i)if(!targets[i].name||targets[i].symbol<0||
      !detail::resolves(memory,layout,sorted,targets[i],hash)){out.issue=Issue::lookup;return out;}
    std::uint32_t count=0;
    while(head){
      if(count==max_packed_inventory_nodes){out.issue=Issue::list;return out;}
      std::array<std::uint8_t,12> node{};if(!detail::read_hash(memory,head,node.data(),node.size(),hash)){out.issue=Issue::unreadable;return out;}
      const auto object=rs::detail::u32(node.data()+4);path::detail::Fields fields{};
      if(!path::detail::fields(memory,object,fields)||fields.vptr!=hand_detail::item_vtable||fields.symbol>=layout.count){out.issue=Issue::sorter;return out;}
      for(const auto v:{fields.object,fields.vptr,fields.refcount,fields.mainflag,fields.flags,fields.amount,fields.symbol})detail::word_hash(hash,v);
      hash.update(fields.damages.data(),fields.damages.size());
      std::array<std::uint8_t,128> name{};std::uint32_t length=0;if(!detail::symbol_name(memory,layout,fields.symbol,name,length,hash)){out.issue=Issue::lookup;return out;}
      head=rs::detail::u32(node.data()+8);++count;
    }
    if(count!=listed_nodes){out.issue=Issue::changed;return out;}out.nodes=count;
    const auto now=hash.finish();if(pass&&now!=first){out.issue=Issue::changed;return out;}first=now;
  }
  if(rs::detail::stable_layout(memory,lhiver::game_parser,profile,layout)!=rs::Issue::none){out.issue=Issue::changed;return out;}
  out.stable=true;out.fingerprint=first;return out;
}
inline SortEvidence read_sort_inputs(const ReadOnlyMemory& memory,const cat::Descriptor& descriptor,
    const path::Snapshot& snapshot) noexcept {
  if(!cat::structurally_valid(descriptor)||!snapshot.captured||snapshot.plan.id!=descriptor.id||
     !path::detail::boundary(snapshot.boundary)||!snapshot.original_sorter_bound){SortEvidence out;out.issue=Issue::boundary;return out;}
  const auto& d=*cat::find(descriptor.id);const auto assembly=bow_recipe_runtime::assemble(d);
  return read_sort_values(memory,assembly.profile(),snapshot.boundary.key.hero,snapshot.listed_nodes,d.targets.data,d.targets.count);
}

struct WorldListEvidence {
  bool stable=false;Issue issue=Issue::none;std::array<std::uint32_t,2> nodes{};
  std::array<bool,3> current_whole_refcount_safe{};lhiver::Digest fingerprint{};
};
struct WorldListRequest {
  std::uint32_t hero=0,world=0,listed_nodes=0;
  const path::RemoveStep* removals=nullptr;std::size_t removal_count=0;
};
// Positive refcount evidence for ALREADY existing withdrawal objects, even Ref1,
// provided the original admission into the two world lists demonstrably increased
// it before release. No imaginary pointer for items generated only later.
inline WorldListEvidence read_world_values(const ReadOnlyMemory& memory,const WorldListRequest& request,
    const PoolEvidence& pool) noexcept {
  WorldListEvidence out;if(!request.hero||!request.world||request.hero>UINT32_MAX-0x1000u||request.world>UINT32_MAX-0x628cu||
    !request.removals||!request.removal_count||request.removal_count>3||request.listed_nodes>max_packed_inventory_nodes||!pool.stable){out.issue=Issue::boundary;return out;}
  lhiver::Digest first{};
  for(unsigned pass=0;pass<2;++pass){
    Sha256 hash;std::array<std::array<bool,3>,2> present{};
    std::uint32_t inventory=0;if(!detail::word(memory,request.hero,0x718,inventory)){out.issue=Issue::unreadable;return out;}
    std::uint32_t inventory_count=0;
    while(inventory){if(inventory_count++==max_packed_inventory_nodes){out.issue=Issue::list;return out;}
      for(const auto free:pool.prefix)if(free&&zbug_end_compat::detail::overlaps(free,12,inventory,12)){out.issue=Issue::alias;return out;}
      std::array<std::uint8_t,12> node{};if(!detail::read_hash(memory,inventory,node.data(),node.size(),hash)){out.issue=Issue::unreadable;return out;}
      inventory=rs::detail::u32(node.data()+8);
    }
    if(inventory_count!=request.listed_nodes){out.issue=Issue::changed;return out;}
    for(std::size_t list=0;list<2;++list){
      std::uint32_t header=0;if(!detail::word(memory,request.world,list?0x6288:0x6280,header)){out.issue=Issue::unreadable;return out;}
      std::array<std::uint8_t,12> h{};if(!detail::read_hash(memory,header,h.data(),h.size(),hash)||
        rs::detail::u32(h.data())!=(list?0x77ef90u:0x77ef50u)){out.issue=Issue::world;return out;}
      auto node=rs::detail::u32(h.data()+8);std::uint32_t count=0;
      while(node){
        if(count==65536){out.issue=Issue::list;return out;}
        for(const auto free:pool.prefix)if(free&&zbug_end_compat::detail::overlaps(free,12,node,12)){out.issue=Issue::alias;return out;}
        std::array<std::uint8_t,12> bytes{};
        if(!detail::read_hash(memory,node,bytes.data(),bytes.size(),hash)){out.issue=Issue::unreadable;return out;}
        const auto object=rs::detail::u32(bytes.data()+4);if(!object){out.issue=Issue::world;return out;}
        for(std::size_t i=0;i<request.removal_count;++i)if(request.removals[i].object==object)present[list][i]=true;
        node=rs::detail::u32(bytes.data()+8);++count;
      }
      if(pass&&count!=out.nodes[list]){out.issue=Issue::changed;return out;}out.nodes[list]=count;
    }
    for(std::size_t i=0;i<request.removal_count;++i){const auto& step=request.removals[i];bool safe=false;
      if(step.object&&step.source==path::Source::listed&&step.path==path::Removal::whole){
        std::uint32_t ref=0,instance=0;
        if(!detail::word(memory,step.object,4,ref)||!detail::word(memory,step.object,0x330,instance)||
           ref!=step.refcount||instance!=static_cast<std::uint32_t>(step.operation.symbol)){out.issue=Issue::changed;return out;}
        const auto added=unsigned{!present[0][i]}+unsigned{!present[1][i]};
        safe=ref&&ref<=INT32_MAX-added&&ref+added>1;detail::word_hash(hash,ref);detail::word_hash(hash,instance);
      }
      if(pass&&safe!=out.current_whole_refcount_safe[i]){out.issue=Issue::changed;return out;}out.current_whole_refcount_safe[i]=safe;
    }
    const auto now=hash.finish();if(pass&&now!=first){out.issue=Issue::changed;return out;}first=now;
  }
  out.stable=true;out.fingerprint=first;return out;
}
inline WorldListEvidence read_world_lists(const ReadOnlyMemory& memory,const path::Snapshot& snapshot,
    const PoolEvidence& pool) noexcept {
  if(!snapshot.captured||!path::detail::boundary(snapshot.boundary)){WorldListEvidence out;out.issue=Issue::boundary;return out;}
  return read_world_values(memory,{snapshot.boundary.key.hero,snapshot.boundary.key.world,snapshot.listed_nodes,
    snapshot.removals.data(),snapshot.removals.size()},pool);
}
} // namespace ubc::bow_item_native

