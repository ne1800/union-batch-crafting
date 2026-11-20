// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/anvil_recipe_catalog.hpp"
#include "ubc/station_recipe_snapshot.hpp"

// Exclusively the named individual templates and ITMISWORDRAWHOT. No
// general relaxation of mixed_representation, no engine calls. Physical
// stock L+P and original first GetAmount return 2L+P are different.
// Original file fingerprints are NOT current mod path evidence.
namespace ubc::mixed_hotsteel {
inline constexpr std::uint32_t policy_version=1;
inline constexpr std::int32_t symbol=16611;
inline constexpr PackedInventoryTarget target{"ITMISWORDRAWHOT",symbol};
inline constexpr std::uint32_t inventory_vtable=0x83c714;
inline constexpr std::uint32_t item_multi=0x00200000,item_active=0x40000000;

enum class Issue {
  none,thread,reentry,boundary,recipe,quantity,inventory,hands,item,alias,
  pack_token,physical_overflow,count_overflow,changed,writer,script,actor,
  materials,dependencies,native_unreadable,native_changed,native_code,vtable
};
struct Contract {
  std::uint32_t recipe=0,owner=0,callback=0;
  std::int32_t output=-1;
  bool require_self_is_hero=false;
  const recipe_catalog::Callback* definition=nullptr;
};
// Return exclusively from own canonical descriptors. In particular, neither
// original all-or-three templates nor later ore recipes are included.
inline Contract contract_for(std::uint32_t recipe,std::uint32_t callback) noexcept {
  namespace station=station_recipe_catalog;namespace anvil=anvil_recipe_catalog;
  if(recipe==static_cast<std::uint32_t>(station::RecipeId::form_hot_blade)) {
    const auto* r=station::find(station::RecipeId::form_hot_blade);
    if(r && station::structurally_valid(*r) && callback==21003 &&
       r->callback.function->symbol==callback)
      return {recipe,static_cast<std::uint32_t>(r->callback.owner_instance),callback,16612,false,&r->callback};
  }
  for(const auto& r:anvil::recipes) if(recipe==static_cast<std::uint32_t>(r.id) &&
      anvil::structurally_valid(r) && r.callback.function->symbol==callback)
    return {recipe,static_cast<std::uint32_t>(r.callback.owner_instance),callback,r.outputs[0].symbol,true,&r.callback};
  return {};
}

// Identity values, no ownership transfer or later-dereferenceable engine
// handles. The adapter must bind this relationship before/after the reader.
struct Identity {
  std::uint64_t generation=0;
  std::uint32_t thread=0,game=0,world=0,hero=0,station=0,manager=0,parser=0;
};
inline bool same(const Identity& a,const Identity& b) noexcept {
  return a.generation==b.generation && a.thread==b.thread && a.game==b.game &&
    a.world==b.world && a.hero==b.hero && a.station==b.station &&
    a.manager==b.manager && a.parser==b.parser;
}
inline bool valid(const Identity& k) noexcept {
  return k.generation && k.thread && k.game && k.world && k.hero && k.station &&
    k.manager && k.parser==lhiver::game_parser;
}
struct Boundary {
  Identity identity{};
  std::uint64_t frame=0,serial=0;
  std::uint32_t current_thread=0;
  bool quiescent_verified=false;
};
inline bool same(const Boundary& a,const Boundary& b) noexcept {
  return same(a.identity,b.identity) && a.frame==b.frame && a.serial==b.serial &&
    a.current_thread==b.current_thread && a.quiescent_verified==b.quiescent_verified;
}
inline bool valid(const Boundary& b) noexcept {
  return valid(b.identity) && b.frame && b.serial && b.current_thread==b.identity.thread &&
    b.quiescent_verified;
}
struct Capture {
  bool valid=false;Issue issue=Issue::none;
  Boundary boundary{};
  PackedInventoryIssue inventory_issue=PackedInventoryIssue::none;
  StationHandReadIssue hand_issue=StationHandReadIssue::none;
  std::int32_t listed=-1,packed=-1,total=-1;
  std::uint32_t object=0,mainflag=0,flags=0,listed_nodes=0,packed_entries=0;
  // Binds full list/pack string, all object fields, and all slots. Only
  // compares own value copies, no reservation against foreign threads.
  lhiver::Digest fingerprint{};
};
struct CodeRange {std::uint32_t address,length;lhiver::Digest digest;};
inline constexpr std::array<CodeRange,10> original_ranges{{
  {0x6e72e0,244,lhiver::digest_literal("481717adcba9af978541fdb48f8a7838bdb387f9157c1d1956aa7131ecc174c7")},
  {0x70c970,263,lhiver::digest_literal("10ae5c0974c06acb525b08289ae668dd07474e672de78699e43754eeb030cff4")},
  {0x70c730,573,lhiver::digest_literal("bc08d36bee92fcfca8ba05fb68d343e48bd92aa1cbde52cf2ed22b8eed4f2cc2")},
  {0x70f8f0,956,lhiver::digest_literal("b70ad1bf09d70f96c8031098f09ad21f5e2b665ac053bf90510d5210e200ed1a")},
  {0x7125a0,99,lhiver::digest_literal("dd0405c11373e734e96da95c78810e0f052a0d11abd2433f7383b074f6fc9c11")},
  {0x70d080,234,lhiver::digest_literal("712b37bb44c304977cba62b30a881b28638b886361920c169009f2078072dd5a")},
  {0x7495f0,69,lhiver::digest_literal("d7770dbf7cb8b3911f3e6b941302e558478f14d6f31cdead4a1844f93b140e2f")},
  {0x6e8200,602,lhiver::digest_literal("8ed9d0dd6fab11a114ded319d4b49b9c3d9715dc6e0ccb22165f59937db33422")},
  {0x711410,6,lhiver::digest_literal("dabe4c98578a4aefbc31e932bbf5a908b08a7141b7f7383aee43de39292c658f")},
  {0x711420,7,lhiver::digest_literal("edefe40dfb0c9a0883df7eb4dab972f5fb212583cb6da3a8a9e00bf4f112f448")}
}};
struct VtableEntry {std::uint32_t address,target;};
inline constexpr std::array<VtableEntry,4> original_vslots{{
  {inventory_vtable+0x44,0x70c730},{inventory_vtable+0xc0,0x70d080},
  {hand_detail::item_vtable,0x711410},{hand_detail::item_vtable+0x78,0x711420}
}};
struct NativeEvidence {
  bool original_ranges_match=false,original_vslots_match=false;
  Issue issue=Issue::none;std::uint32_t failed_address=0;
};
namespace detail {
inline bool word(const ReadOnlyMemory& m,std::uint32_t base,std::uint32_t offset,std::uint32_t& out) noexcept {
  std::array<std::uint8_t,4> bytes{};
  if(!hand_detail::at_offset(m,base,offset,bytes.data(),bytes.size()))return false;
  out=hand_detail::u32(bytes.data());return true;
}
inline void number(Sha256& hash,std::uint32_t value) noexcept {
  std::array<std::uint8_t,4> bytes{};
  for(unsigned i=0;i<4;++i)bytes[i]=static_cast<std::uint8_t>(value>>(i*8));
  hash.update(bytes.data(),bytes.size());
}
// The native search name must appear exactly once in the ENTIRE pack
// string. Also PRE_ITMISWORDRAWHOT or ITMISWORDRAWHOT_EXTRA are collisions.
inline bool canonical_pack(const std::uint8_t* bytes,std::uint32_t length,std::int32_t amount) noexcept {
  constexpr std::size_t name_size=sizeof("ITMISWORDRAWHOT")-1;
  if(!bytes || !length || length>max_packed_inventory_bytes || amount<=0)return false;
  std::size_t position=0,matches=0;
  for(std::size_t at=0;at+name_size<=length;++at)
    if(std::memcmp(bytes+at,target.name,name_size)==0){position=at;++matches;}
  if(matches!=1 || (position && bytes[position-1]!=',') ||
     position+name_size>=length || bytes[position+name_size]!=':')return false;
  auto at=position+name_size+1;
  if(at>=length || bytes[at]<'1' || bytes[at]>'9')return false;
  std::uint32_t value=0;
  while(at<length && bytes[at]>='0' && bytes[at]<='9') {
    const auto digit=static_cast<std::uint32_t>(bytes[at++]-'0');
    if(value>(static_cast<std::uint32_t>(INT32_MAX)-digit)/10)return false;
    value=value*10+digit;
  }
  return value==static_cast<std::uint32_t>(amount) && (at==length || bytes[at]==',');
}
} // namespace detail

// Only exact original bodies. No generic skipping/normalizing of
// CALL/JMP operands and no implicit trust in a ZBug registration.
// GetAmount->CreateVob(0x77F240)/Factory/Allocator and further
// dependencies are explicitly NOT fully covered by these ten areas.
inline NativeEvidence verify_original_code(const ReadOnlyMemory& memory) noexcept {
  NativeEvidence out;std::array<std::uint8_t,956> bytes{};
  for(unsigned pass=0;pass<2;++pass) {
    for(const auto& range:original_ranges) {
      if(!memory.copy(range.address,bytes.data(),range.length))return {false,false,Issue::native_unreadable,range.address};
      Sha256 hash;hash.update(bytes.data(),range.length);
      if(hash.finish()!=range.digest)return {false,false,pass?Issue::native_changed:Issue::native_code,range.address};
    }
    for(const auto& slot:original_vslots) {
      std::uint32_t value=0;
      if(!detail::word(memory,slot.address,0,value))return {false,false,Issue::native_unreadable,slot.address};
      if(value!=slot.target)return {false,false,Issue::vtable,slot.address};
    }
  }
  out.original_ranges_match=out.original_vslots_match=true;return out;
}

// Persistent workspace on exactly one owner thread. No stack megabuffer,
// no heap per capture. An additional comparison run also binds fields
// that the general pure quantity reader intentionally does not evaluate.
class Reader {
public:
  explicit Reader(std::uint32_t owner_thread) noexcept:owner_thread_(owner_thread){}
  Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
  Capture capture(const ReadOnlyMemory& memory,const Boundary& boundary) noexcept {
    Capture out;out.boundary=boundary;
    if(!owner_thread_ || boundary.current_thread!=owner_thread_ || boundary.identity.thread!=owner_thread_) {out.issue=Issue::thread;return out;}
    if(reading_){out.issue=Issue::reentry;return out;}
    if(!valid(boundary)){out.issue=Issue::boundary;return out;}
    reading_=true;struct Guard {bool& value;~Guard(){value=false;}} guard{reading_};
    const auto hero=boundary.identity.hero;
    packed_detail::Header header{};std::uint32_t type=0;
    if(!detail::word(memory,hero,0x668,type) || type!=inventory_vtable ||
       !hand_detail::at_offset(memory,hero,0x708,header.data(),header.size())) {out.issue=Issue::inventory;return out;}
    const auto stock=read_packed_inventory<1>(memory,hero,&target,1,true,inventory_);
    out.inventory_issue=stock.issue;
    if(!stock.storage_totals_known || !stock.snapshot_unchanged || stock.issue!=PackedInventoryIssue::none ||
       !stock.pack_ability || stock.listed_occurrences[0]!=1 || stock.packed_occurrences[0]!=1 ||
       stock.duplicate_packed_target[0] || stock.any_active_marker || stock.listed[0]<=0 || stock.packed[0]<=0) {
      out.issue=Issue::inventory;return out;
    }
    const auto length=hand_detail::u32(header.data()+32);
    if(!detail::canonical_pack(inventory_.packed_bytes.data(),length,stock.packed[0])) {out.issue=Issue::pack_token;return out;}
    out.hand_issue=station_recipe_snapshot_detail::begin_hands(memory,hero,hands_);
    if(out.hand_issue!=StationHandReadIssue::none || !hands_.before.left_slot_known || !hands_.before.right_slot_known) {out.issue=Issue::hands;return out;}
    Sha256 hash;hash.update(header.data(),header.size());detail::number(hash,type);
    for(std::size_t i=0;i<stock.listed_nodes;++i) {
      const auto& record=inventory_.records[i];const auto object=hand_detail::u32(record.node.data()+4);
      auto& fields=item_fields_[i];
      if(!memory.copy(object,fields.data(),4) || !hand_detail::at_offset(memory,object,0x154,fields.data()+4,8) ||
         hand_detail::u32(fields.data())!=hand_detail::item_vtable) {out.issue=Issue::item;return out;}
      // GetAmount visits all list elements via GetInstance. Even a
      // foreign object must not share a differing getter or slots.
      if(object==hands_.before.interact_item) {out.issue=Issue::alias;return out;}
      for(std::size_t slot=0;slot<hands_.count;++slot)
        if(object==hand_detail::u32(hands_.records[slot].fields.data()+0x30)) {out.issue=Issue::alias;return out;}
      if(hand_detail::i32(record.item_fields.data()+4)==symbol) {
        out.object=object;out.mainflag=hand_detail::u32(fields.data()+4);out.flags=hand_detail::u32(fields.data()+8);
        if(out.mainflag!=1 || !(out.flags&item_multi) || (out.flags&item_active)) {out.issue=Issue::item;return out;}
      }
      detail::number(hash,record.address);hash.update(record.node.data(),record.node.size());
      hash.update(record.item_fields.data(),record.item_fields.size());hash.update(fields.data(),fields.size());
    }
    hash.update(inventory_.packed_bytes.data(),static_cast<std::size_t>(length)+1);
    hash.update(hands_.header.data(),hands_.header.size());hash.update(hands_.anchor.data(),hands_.anchor.size());
    hash.update(hands_.pointers.data(),hands_.count*4);
    for(std::size_t i=0;i<hands_.count;++i) {
      const auto& r=hands_.records[i];detail::number(hash,r.address);
      hash.update(r.fields.data(),r.fields.size());hash.update(r.name.data(),r.length+1);
    }
    for(const auto* hand:{&hands_.before.hands.left,&hands_.before.hands.right}) {
      detail::number(hash,static_cast<std::uint32_t>(hand->state));detail::number(hash,hand->object);
      detail::number(hash,static_cast<std::uint32_t>(hand->symbol));detail::number(hash,static_cast<std::uint32_t>(hand->amount));
    }
    for(std::size_t i=0;i<stock.listed_nodes;++i) {
      std::array<std::uint8_t,12> fields{};const auto object=hand_detail::u32(inventory_.records[i].node.data()+4);
      if(!memory.copy(object,fields.data(),4) || !hand_detail::at_offset(memory,object,0x154,fields.data()+4,8) || fields!=item_fields_[i]) {out.issue=Issue::changed;return out;}
    }
    packed_detail::Header after{};std::uint32_t type_after=0;
    if(packed_detail::compare_list(memory,inventory_,stock.listed_nodes)!=PackedInventoryIssue::none ||
       packed_detail::compare_pack(memory,inventory_,hand_detail::u32(header.data()+28),length)!=PackedInventoryIssue::none ||
       !station_recipe_snapshot_detail::verify_hands(memory,hands_) ||
       !hand_detail::at_offset(memory,hero,0x708,after.data(),after.size()) || after!=header ||
       !detail::word(memory,hero,0x668,type_after) || type_after!=type) {out.issue=Issue::changed;return out;}
    out.listed=stock.listed[0];out.packed=stock.packed[0];out.total=stock.total[0];
    out.listed_nodes=stock.listed_nodes;out.packed_entries=stock.packed_entries;
    out.fingerprint=hash.finish();out.valid=out.object!=0;return out;
  }
private:
  std::uint32_t owner_thread_=0;bool reading_=false;
  PackedInventoryWorkspace inventory_{};
  station_recipe_snapshot_detail::HandWorkspace hands_{};
  std::array<std::array<std::uint8_t,12>,max_packed_inventory_nodes> item_fields_{};
};

struct Assessment {
  bool input_eligible=false;Issue issue=Issue::none;
  std::int32_t maximum=0,original_first_count=0,expected_remaining=0;
};
inline Assessment assess(std::uint32_t recipe,std::uint32_t callback,std::uint32_t quantity,const Capture& c) noexcept {
  if(!contract_for(recipe,callback).definition)return {false,Issue::recipe};
  if(!c.valid || !valid(c.boundary) || !c.object || c.mainflag!=1 || !(c.flags&item_multi) ||
     (c.flags&item_active) || c.listed<=0 || c.packed<=0)return {false,Issue::inventory};
  const auto physical=std::int64_t{c.listed}+c.packed;
  const auto counted=std::int64_t{2}*c.listed+c.packed;
  if(physical>INT32_MAX || c.total!=physical)return {false,Issue::physical_overflow};
  if(counted>INT32_MAX)return {false,Issue::count_overflow};
  if(!quantity || quantity>static_cast<std::uint32_t>(physical))return {false,Issue::quantity};
  return {true,Issue::none,static_cast<std::int32_t>(physical),static_cast<std::int32_t>(counted),
    static_cast<std::int32_t>(physical-quantity)};
}

// Mandatory external evidence for EXACTLY the late capture. None of these
// values is produced by double-read, file hash, or a historical world
// probe. The integrator must set every field on the same synchronous writer.
struct WriterProof {
  Boundary boundary{};
  std::uint64_t job=0,writer_token=0;
  std::uint32_t recipe=0,owner=0,callback=0,quantity=0,hero_binding=0,self_binding=0;
  bool own_context_before_after=false,canonical_callback_and_writer=false;
  bool full_recipe_and_item_constructors=false,amount_operands_equal_quantity=false,actors_verified=false;
  bool all_other_materials_and_output_room=false,station_hand_and_normal_end=false;
  // In particular current external registration including parameters, full
  // CreateVob->Factory->Allocator->InitByScript/Insert path, as well as
  // reachable Remove/Create follow-up. No such runtime evidence accompanies
  // this header yet. Five True literals are not production integration.
  bool external_bindings_and_parameters=false;
  bool getamount_pack_createvob_factory_allocator=false;
  bool insert_merge_and_release_dependencies=false;
  bool remove_dependencies=false,output_create_dependencies=false;
};
struct Admission {bool allowed=false;Issue issue=Issue::none;Assessment amount{};NativeEvidence native{};};
inline Admission admit(const ReadOnlyMemory& memory,const Capture& early,const Capture& late,const WriterProof& p) noexcept {
  Admission out;out.amount=assess(p.recipe,p.callback,p.quantity,late);
  if(!out.amount.input_eligible){out.issue=out.amount.issue;return out;}
  const auto contract=contract_for(p.recipe,p.callback);
  if(!assess(p.recipe,p.callback,p.quantity,early).input_eligible ||
     !same(early.boundary.identity,late.boundary.identity) || late.boundary.serial<=early.boundary.serial ||
     late.boundary.frame<early.boundary.frame || early.fingerprint!=late.fingerprint ||
     early.object!=late.object || early.listed!=late.listed || early.packed!=late.packed ||
     early.total!=late.total || early.mainflag!=late.mainflag || early.flags!=late.flags) {out.issue=Issue::changed;return out;}
  if(!same(p.boundary,late.boundary) || !p.job || !p.writer_token || p.owner!=contract.owner ||
     !p.own_context_before_after || !p.canonical_callback_and_writer) {out.issue=Issue::writer;return out;}
  if(!p.full_recipe_and_item_constructors || !p.amount_operands_equal_quantity) {out.issue=Issue::script;return out;}
  if(!p.actors_verified || p.hero_binding!=late.boundary.identity.hero ||
     (contract.require_self_is_hero && p.self_binding!=p.hero_binding)) {out.issue=Issue::actor;return out;}
  if(!p.all_other_materials_and_output_room || !p.station_hand_and_normal_end) {out.issue=Issue::materials;return out;}
  if(!p.external_bindings_and_parameters || !p.getamount_pack_createvob_factory_allocator ||
     !p.insert_merge_and_release_dependencies || !p.remove_dependencies || !p.output_create_dependencies) {out.issue=Issue::dependencies;return out;}
  out.native=verify_original_code(memory);
  if(!out.native.original_ranges_match || !out.native.original_vslots_match) {out.issue=out.native.issue;return out;}
  out.allowed=true;return out;
}
// admit is an input policy, not an executor and not an order store.
// The existing one-time writer/result receipt remains mandatory. After
// original, ONLY the expected total deltas are accepted; changed Pack/List
// representation is then allowed. Error/partial effect: lock, no retry/refund.
} // namespace ubc::mixed_hotsteel
