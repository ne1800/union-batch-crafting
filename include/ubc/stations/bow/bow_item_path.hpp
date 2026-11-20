// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/bow/bow_recipe_snapshot.hpp"
#include "ubc/core/sha256.hpp"

namespace ubc::bow_item_path {
namespace cat=bow_recipe_catalog;
namespace hd=station_recipe_snapshot_detail;

// Own admission and native route plan, NO execution ticket. In
// particular new list nodes/sorting, Unpack, visual follow-up and
// loaded modules must be closed by a later concrete code/dispatch guard.
inline constexpr bool complete_execution_contract=false;
inline constexpr std::uint32_t inventory_vtable=0x83c714,sorter=0x705b80;
enum class Issue {none,descriptor,quantity,boundary,thread,reentry,inventory,hands,
  fields,item,alias,ambiguous,pack_search,resources,overflow,changed};
enum class Source {unknown,listed,packed};
enum class Removal {unknown,split,whole};
enum class Insertion {unknown,new_stack,merge};
struct Operation {
  std::int32_t symbol=-1;std::uint32_t amount=0,offset=0;
};
struct Plan {
  bool valid=false;cat::RecipeId id=static_cast<cat::RecipeId>(0);
  std::uint32_t quantity=0,callback=0;
  std::array<Operation,3> minima{},removals{};Operation output{};
};
inline Plan make_plan(const cat::Descriptor& descriptor,std::uint32_t quantity) noexcept {
  Plan out;if(!cat::plan_quantity(descriptor,quantity).valid)return out;
  const auto& d=*cat::find(descriptor.id);std::size_t minima=0,removals=0,creates=0;
  for(const auto& event:d.success){
    const auto amount=std::uint64_t{static_cast<std::uint32_t>(event.value)}*quantity;
    if(event.kind!=cat::EffectKind::minimum&&event.kind!=cat::EffectKind::remove&&event.kind!=cat::EffectKind::create)continue;
    if(event.value<=0||amount>INT32_MAX||event.actor!=cat::Actor::hero)return {};
    Operation op{event.symbol,static_cast<std::uint32_t>(amount),event.offset};
    if(event.kind==cat::EffectKind::minimum){if(minima==3||removals||creates||event.callee!=160)return {};out.minima[minima++]=op;}
    else if(event.kind==cat::EffectKind::remove){if(minima!=3||removals==3||creates||event.callee!=184)return {};out.removals[removals++]=op;}
    else {if(minima!=3||removals!=3||creates++||event.callee!=170)return {};out.output=op;}
  }
  if(minima!=3||removals!=3||creates!=1)return {};
  out.valid=true;out.id=d.id;out.quantity=quantity;out.callback=d.callback.function->symbol;return out;
}
struct Boundary {
  BowRecipeKey key{};std::uint64_t frame=0;std::uint32_t current_thread=0;
  // A real outer own stable point or serialized Late-Writer
  // point; no external /proc double-read as substitute.
  bool stable_boundary_verified=false;
};
struct Material {
  std::int32_t symbol=-1,listed=-1,packed=-1,total=-1;
  std::uint32_t object=0,refcount=0;
};
struct RemoveStep {
  Operation operation{};Source source=Source::unknown;Removal path=Removal::unknown;
  std::uint32_t object=0,refcount=0,nodes_before=0,nodes_after=0;
  // Only objects that have already been read can establish this short whole-stack path.
  // Never assume a reference count for an object that will only be unpacked later.
  bool existing_whole_short_path=false;
};
struct Requirements {
  bool unpack_create=false,split_create=false,whole_remove=false;
  bool new_output_node=false,merge_output=false;
  bool materialized_object_evidence=false;
};
struct Snapshot {
  bool captured=false;Issue issue=Issue::none;
  Boundary boundary{};Plan plan{};
  PackedInventoryIssue inventory_issue=PackedInventoryIssue::none;
  StationHandReadIssue hand_issue=StationHandReadIssue::none;
  std::array<Material,cat::catalog_target_capacity> materials{};
  std::array<RemoveStep,3> removals{};
  StationRecipeHands hands{};Requirements requirements{};
  std::uint32_t listed_nodes=0,packed_entries=0,selected=0,contents=0,compare=0,disabled=0,pool_debug=0;
  std::uint32_t output_merge_object=0;
  std::int32_t pack_ability=0;
  Insertion insertion=Insertion::unknown;
  bool original_sorter_bound=false;
  lhiver::Digest fingerprint{};
};
namespace detail {
inline bool key(const BowRecipeKey& a,const BowRecipeKey& b) noexcept {
  return a.generation==b.generation&&a.thread==b.thread&&a.world==b.world&&a.hero==b.hero&&
    a.station==b.station&&a.manager==b.manager&&a.production_kind==b.production_kind;
}
inline bool boundary(const Boundary& b) noexcept {
  const auto& k=b.key;return b.stable_boundary_verified&&b.frame&&k.generation&&k.thread&&k.world&&k.hero&&
    k.station&&k.manager&&k.production_kind==16&&b.current_thread==k.thread;
}
inline bool word(const ReadOnlyMemory& m,std::uint32_t base,std::uint32_t offset,std::uint32_t& out) noexcept {
  std::array<std::uint8_t,4> b{};if(!hand_detail::at_offset(m,base,offset,b.data(),b.size()))return false;
  out=hand_detail::u32(b.data());return true;
}
inline std::size_t target(const cat::Descriptor& d,std::int32_t symbol) noexcept {
  for(std::size_t i=0;i<d.targets.count;++i)if(d.targets[i].symbol==symbol)return i;return d.targets.count;
}
inline const cat::ItemShape* shape(const cat::Descriptor& d,std::int32_t symbol) noexcept {
  for(const auto& item:d.items)if(item.symbol==symbol)return &item;return nullptr;
}
// Native GetPackedItemInfo searches first for NAME+':'. For existing
// packed targets this first search path must yield exactly one canonical
// token. For absent targets prefix/suffix collisions are also rejected
// so the subsequent native fallback does not materialize a foreign token.
inline bool packed_search(const std::uint8_t* bytes,std::size_t length,const char* name,std::int32_t amount) noexcept {
  if(!bytes||!name||amount<0||length>max_packed_inventory_bytes)return false;
  std::size_t size=0;while(size<64&&name[size])++size;if(!size||size==64)return false;
  std::size_t raw=0,exact=0,position=0;
  for(std::size_t i=0;i+size<=length;++i)if(!std::memcmp(bytes+i,name,size)){
    ++raw;if(i+size<length&&bytes[i+size]==':'){++exact;position=i;}}
  if(!amount)return raw==0;
  if(exact!=1||(position&&bytes[position-1]!=','))return false;
  std::size_t p=position+size+1;std::uint64_t found=0;bool digit=false;
  while(p<length&&bytes[p]>='0'&&bytes[p]<='9'){
    digit=true;found=found*10u+static_cast<unsigned>(bytes[p++]-'0');if(found>INT32_MAX)return false;}
  return digit&&found==static_cast<std::uint32_t>(amount)&&(p==length||bytes[p]==',');
}
struct Fields {
  std::uint32_t object=0,vptr=0,refcount=0,mainflag=0,flags=0,amount=0,symbol=0;
  std::array<std::uint8_t,32> damages{};
  bool operator==(const Fields& b) const noexcept {
    return object==b.object&&vptr==b.vptr&&refcount==b.refcount&&mainflag==b.mainflag&&flags==b.flags&&
      amount==b.amount&&symbol==b.symbol&&damages==b.damages;
  }
};
inline bool fields(const ReadOnlyMemory& m,std::uint32_t object,Fields& f) noexcept {
  f={};f.object=object;return object&&word(m,object,0,f.vptr)&&word(m,object,4,f.refcount)&&
    word(m,object,0x154,f.mainflag)&&word(m,object,0x158,f.flags)&&
    word(m,object,0x32c,f.amount)&&word(m,object,0x330,f.symbol)&&
    hand_detail::at_offset(m,object,0x16c,f.damages.data(),f.damages.size());
}
inline void hash_word(Sha256& hash,std::uint32_t value) noexcept {
  std::array<std::uint8_t,4> b{};for(unsigned i=0;i<4;++i)b[i]=static_cast<std::uint8_t>(value>>(i*8u));hash.update(b.data(),b.size());
}
} // namespace detail

// Persistent own thread workspace. No engine calls, heap allocation,
// back-reference or preparatory HasItems query. The result binds
// values/objects/slots/pack data; it confirms NO native code closure.
class Reader {
public:
  explicit Reader(std::uint32_t owner_thread) noexcept:owner_(owner_thread){}
  Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
  Snapshot capture(const ReadOnlyMemory& memory,const cat::Descriptor& descriptor,std::uint32_t quantity,const Boundary& boundary) noexcept {
    Snapshot out;out.boundary=boundary;out.plan=make_plan(descriptor,quantity);
    if(!out.plan.valid){out.issue=cat::structurally_valid(descriptor)?Issue::quantity:Issue::descriptor;return out;}
    if(!owner_||boundary.current_thread!=owner_||boundary.key.thread!=owner_){out.issue=Issue::thread;return out;}
    if(!detail::boundary(boundary)){out.issue=Issue::boundary;return out;}
    if(reading_){out.issue=Issue::reentry;return out;}
    reading_=true;struct Lease{bool& v;~Lease(){v=false;}}lease{reading_};
    const auto& d=*cat::find(descriptor.id);const auto hero=boundary.key.hero;
    if(hero>UINT32_MAX-0x1000u){out.issue=Issue::fields;return out;}
    const auto inv=hero+0x668;
    std::array<std::uint32_t,6> initial{},final{};
    auto controls=[&](auto& values) noexcept {return detail::word(memory,inv,0,values[0])&&detail::word(memory,inv,4,values[1])&&
      detail::word(memory,inv,0x24,values[2])&&detail::word(memory,inv,0xa8,values[3])&&
      detail::word(memory,inv,0x98,values[4])&&detail::word(memory,0x8a2558,0,values[5]);};
    packed_detail::Header initial_header{},final_header{};
    if(!controls(initial)||initial[0]!=inventory_vtable||!hand_detail::at_offset(memory,hero,packed_detail::header_offset,initial_header.data(),initial_header.size())){
      out.issue=Issue::fields;return out;}
    out.contents=initial[1];out.selected=initial[2];out.compare=initial[3];out.disabled=initial[4];out.pool_debug=initial[5];
    out.original_sorter_bound=out.compare==sorter;
    const auto stock=read_packed_inventory<cat::catalog_target_capacity>(memory,hero,d.targets.data,d.targets.count,true,inventory_);
    out.inventory_issue=stock.issue;
    if(!stock.storage_totals_known||!stock.snapshot_unchanged||stock.issue!=PackedInventoryIssue::none){out.issue=Issue::inventory;return out;}
    out.listed_nodes=stock.listed_nodes;out.packed_entries=stock.packed_entries;out.pack_ability=stock.pack_ability;
    out.hand_issue=hd::begin_hands(memory,hero,hands_);
    if(out.hand_issue!=StationHandReadIssue::none){out.issue=Issue::hands;return out;}
    const auto& hands=hands_.before;
    if(!hands.left_slot_known||!hands.right_slot_known||!hands.interact_item_known||hands.interact_item||
      hands.hands.left.state!=0||hands.hands.right.state!=0||hands.hands.left.object||hands.hands.right.object){out.issue=Issue::hands;return out;}
    const auto pack_length=hand_detail::u32(initial_header.data()+32);
    Sha256 hash;hash.update(initial_header.data(),initial_header.size());for(const auto v:initial)detail::hash_word(hash,v);
    for(std::size_t i=0;i<d.targets.count;++i){auto& row=out.materials[i];row.symbol=d.targets[i].symbol;
      row.listed=stock.listed[i];row.packed=stock.packed[i];row.total=stock.total[i];
      if(cat::units(d.costs,row.symbol)||cat::units(d.minima,row.symbol)){
        if(stock.listed_occurrences[i]>1||stock.packed_occurrences[i]>1||stock.duplicate_packed_target[i]||stock.has_active_marker[i]||
          stock.mixed_representation[i]||(stock.listed_occurrences[i]&&stock.packed_occurrences[i])){out.issue=Issue::ambiguous;return out;}
        if(stock.pack_ability&&!detail::packed_search(inventory_.packed_bytes.data(),pack_length,d.targets[i].name,row.packed)){
          out.issue=Issue::pack_search;return out;}
      }
    }
    bool output_merge=false;
    for(std::size_t i=0;i<stock.listed_nodes;++i){const auto& record=inventory_.records[i];const auto object=hand_detail::u32(record.node.data()+4);
      auto& f=fields_[i];if(!detail::fields(memory,object,f)){out.issue=Issue::fields;return out;}
      // GetAmount/IsIn/Remove also see all foreign listed items.
      if(f.vptr!=hand_detail::item_vtable||!f.refcount||f.refcount>INT32_MAX||f.amount>INT32_MAX||f.symbol>INT32_MAX){out.issue=Issue::item;return out;}
      if(f.amount!=hand_detail::u32(record.item_fields.data())||f.symbol!=hand_detail::u32(record.item_fields.data()+4)){out.issue=Issue::changed;return out;}
      const auto index=detail::target(d,static_cast<std::int32_t>(f.symbol));
      if(index<d.targets.count){auto& row=out.materials[index];
        if(stock.listed_occurrences[index]==1){row.object=f.object;row.refcount=f.refcount;}
        const auto* shape=detail::shape(d,row.symbol);const bool exact=shape&&f.mainflag==shape->mainflag&&f.flags==shape->native_flags;
        bool alias=false;for(std::size_t j=0;j<hands_.count;++j)alias=alias||hand_detail::u32(hands_.records[j].fields.data()+0x30)==object;
        if(cat::units(d.costs,row.symbol)||cat::units(d.minima,row.symbol)){
          if(alias){out.issue=Issue::alias;return out;}if(!exact){out.issue=Issue::item;return out;}
        }
        if(row.symbol==out.plan.output.symbol){
          const bool active=shape&&f.mainflag==shape->mainflag&&f.flags==(shape->native_flags|0x40000000u);
          if(!exact&&!active){out.issue=Issue::item;return out;}
          if(exact&&alias){out.issue=Issue::alias;return out;}
          if(exact){output_merge=true;if(!out.output_merge_object)out.output_merge_object=f.object;}
        }
      }
      detail::hash_word(hash,record.address);hash.update(record.node.data(),record.node.size());hash.update(record.item_fields.data(),record.item_fields.size());
      for(const auto v:{f.object,f.vptr,f.refcount,f.mainflag,f.flags,f.amount,f.symbol})detail::hash_word(hash,v);
      hash.update(f.damages.data(),f.damages.size());
    }
    std::uint32_t materialized=0;
    for(const auto op:out.plan.minima){const auto index=detail::target(d,op.symbol);
      if(index==d.targets.count||out.materials[index].total<static_cast<std::int32_t>(op.amount)){out.issue=Issue::resources;return out;}
      if(out.materials[index].packed>0)++materialized;
    }
    if(materialized>max_packed_inventory_nodes-stock.listed_nodes){out.issue=Issue::overflow;return out;}
    out.requirements.unpack_create=materialized!=0;out.requirements.materialized_object_evidence=materialized!=0;
    auto nodes=stock.listed_nodes+materialized;
    for(std::size_t i=0;i<out.removals.size();++i){auto& step=out.removals[i];step.operation=out.plan.removals[i];
      const auto index=detail::target(d,step.operation.symbol);if(index==d.targets.count){out.issue=Issue::descriptor;return out;}
      const auto& row=out.materials[index];if(row.total<static_cast<std::int32_t>(step.operation.amount)){out.issue=Issue::resources;return out;}
      step.source=row.packed>0?Source::packed:Source::listed;step.object=row.object;step.refcount=row.refcount;step.nodes_before=nodes;
      step.path=row.total==static_cast<std::int32_t>(step.operation.amount)?Removal::whole:Removal::split;
      if(step.path==Removal::whole){if(!nodes){out.issue=Issue::overflow;return out;}--nodes;out.requirements.whole_remove=true;
        step.existing_whole_short_path=step.source==Source::listed&&out.contents==inv+0xa8&&!out.disabled&&!out.pool_debug&&
          out.selected<nodes&&row.refcount>=2&&row.refcount<=INT32_MAX-2u;
      }else out.requirements.split_create=true;
      step.nodes_after=nodes;
    }
    const auto output_index=detail::target(d,out.plan.output.symbol);
    if(output_index==d.targets.count||std::int64_t{out.materials[output_index].total}+out.plan.output.amount>INT32_MAX){out.issue=Issue::overflow;return out;}
    out.insertion=output_merge?Insertion::merge:Insertion::new_stack;
    out.requirements.merge_output=output_merge;out.requirements.new_output_node=!output_merge;
    if(!output_merge&&nodes==max_packed_inventory_nodes){out.issue=Issue::overflow;return out;}
    hash.update(inventory_.packed_bytes.data(),static_cast<std::size_t>(pack_length)+1);
    hash.update(hands_.header.data(),hands_.header.size());hash.update(hands_.anchor.data(),hands_.anchor.size());
    hash.update(hands_.pointers.data(),hands_.count*4);
    for(std::size_t i=0;i<hands_.count;++i){const auto& r=hands_.records[i];detail::hash_word(hash,r.address);
      hash.update(r.fields.data(),r.fields.size());hash.update(r.name.data(),r.length+1);}
    for(std::size_t i=0;i<stock.listed_nodes;++i){detail::Fields f;if(!detail::fields(memory,fields_[i].object,f)||!(f==fields_[i])){out.issue=Issue::changed;return out;}}
    if(!hd::verify_hands(memory,hands_)||packed_detail::compare_list(memory,inventory_,stock.listed_nodes)!=PackedInventoryIssue::none||
      packed_detail::compare_pack(memory,inventory_,hand_detail::u32(initial_header.data()+28),pack_length)!=PackedInventoryIssue::none||
      !hand_detail::at_offset(memory,hero,packed_detail::header_offset,final_header.data(),final_header.size())||initial_header!=final_header||
      !controls(final)||initial!=final){out.issue=Issue::changed;return out;}
    out.hands=hands;out.fingerprint=hash.finish();out.captured=true;return out;
  }
private:
  std::uint32_t owner_=0;bool reading_=false;
  PackedInventoryWorkspace inventory_{};hd::HandWorkspace hands_{};
  std::array<detail::Fields,max_packed_inventory_nodes> fields_{};
};
// Recognition only of own observations; no release from digest equality.
inline bool same_capture(const Snapshot& early,const Snapshot& late) noexcept {
  if(!(early.captured&&late.captured&&early.issue==Issue::none&&late.issue==Issue::none&&early.plan.valid&&late.plan.valid&&
    detail::boundary(early.boundary)&&detail::boundary(late.boundary)&&detail::key(early.boundary.key,late.boundary.key)&&
    late.boundary.frame>=early.boundary.frame&&early.plan.id==late.plan.id&&early.plan.quantity==late.plan.quantity&&
    early.plan.callback==late.plan.callback&&early.fingerprint==late.fingerprint&&early.insertion==late.insertion&&
    early.output_merge_object==late.output_merge_object&&
    early.listed_nodes==late.listed_nodes&&early.packed_entries==late.packed_entries&&early.pack_ability==late.pack_ability&&
    early.selected==late.selected&&early.contents==late.contents&&early.compare==late.compare&&early.disabled==late.disabled&&
    early.pool_debug==late.pool_debug&&early.original_sorter_bound==late.original_sorter_bound))return false;
  auto op=[](const Operation& a,const Operation& b){return a.symbol==b.symbol&&a.amount==b.amount&&a.offset==b.offset;};
  if(!op(early.plan.output,late.plan.output))return false;
  for(std::size_t i=0;i<early.materials.size();++i){const auto& a=early.materials[i];const auto& b=late.materials[i];
    if(a.symbol!=b.symbol||a.listed!=b.listed||a.packed!=b.packed||a.total!=b.total||a.object!=b.object||a.refcount!=b.refcount)return false;}
  for(std::size_t i=0;i<early.removals.size();++i){const auto& a=early.removals[i];const auto& b=late.removals[i];
    if(!op(early.plan.minima[i],late.plan.minima[i])||!op(early.plan.removals[i],late.plan.removals[i])||
      !op(a.operation,b.operation)||a.source!=b.source||a.path!=b.path||a.object!=b.object||a.refcount!=b.refcount||
      a.nodes_before!=b.nodes_before||a.nodes_after!=b.nodes_after||a.existing_whole_short_path!=b.existing_whole_short_path)return false;}
  const auto& a=early.requirements;const auto& b=late.requirements;
  const auto& h=early.hands;const auto& j=late.hands;
  if(!hd::same_object(h.hands.left,j.hands.left)||!hd::same_object(h.hands.right,j.hands.right)||
    h.left_slot_known!=j.left_slot_known||h.right_slot_known!=j.right_slot_known||h.left_tmp_level!=j.left_tmp_level||
    h.right_tmp_level!=j.right_tmp_level||h.interact_item_known!=j.interact_item_known||h.interact_item!=j.interact_item)return false;
  return a.unpack_create==b.unpack_create&&a.split_create==b.split_create&&a.whole_remove==b.whole_remove&&
    a.new_output_node==b.new_output_node&&a.merge_output==b.merge_output&&a.materialized_object_evidence==b.materialized_object_evidence;
}
} // namespace ubc::bow_item_path
