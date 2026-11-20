// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/grind/grind_item_constructor.hpp"
#include "ubc/stations/grind/grind_item_path.hpp"
#include "ubc/stations/bow/bow_item_runtime.hpp"

namespace ubc::grind_item_runtime {
namespace path=grind_item_path;
namespace native=bow_item_native;
namespace code=bow_item_code;
namespace renderer=bow_item_renderer;
namespace dep=alchemy_extra_dependencies;
namespace rs=runtime_script;
enum class Issue {boundary,thread,reentry,guild,inventory,outputs,overflow,modules,externals,
  constructors,native_code,renderer_path,pool,sorter,world,whole_stack,changed,none};
struct Input {
  path::Boundary boundary{};
  // Hold own genuine WindowsModuleIdentity leases over the entire intake.
  dep::Modules modules{};
  renderer::Module renderer{};
  // Empty means original slot. A known locator path requires
  // genuine, adapter-held module leases over the entire intake.
  vob_inventory_insert_compat::Binding inventory_insert{};
};
struct Evidence {
  bool item_path_verified=false;
  Issue issue=Issue::boundary;
  path::Scope scope=path::Scope::writer;
  GrindRecipeKey key{};
  std::uint32_t callback=0,quantity=0,failed_address=0,failed_symbol=0;
  path::Snapshot stock{};
  GrindGuildSnapshot physical{};
  GrindWriterGuildSnapshot writer{};
  code::ModuleEvidence modules{};
  native::ConstructorEvidence constructors{};
  native::PoolPath pool{};
  native::SortEvidence sort{};
  native::WorldListEvidence world{};
  renderer::ForwardEvidence renderer{};
  vob_inventory_insert_compat::Verification inventory_insert{};
  grind_zbug_unpack::Verification unpack{};
  std::array<std::int32_t,3> expected_after_callback{};
  std::array<bool,2> new_output_node{};
  // The finding may be bound by the matching root writer. It is neither
  // a ticket nor evidence for CallFunc/Normal-End/Result comparison.
  static constexpr bool native_effect_authorized=false;
};
namespace detail {
inline bool separate_modules(const Input& in) noexcept {
  for(const auto* m:{&in.modules.zbug,&in.modules.union_core,&in.modules.parser_extender,&in.modules.runtime_crt})
    if(zbug_end_compat::detail::overlaps(m->base,m->image_size,in.renderer.base,in.renderer.image_size))return false;
  if(in.inventory_insert.locator.base){
    const auto& locator=in.inventory_insert.locator;
    for(const auto* m:{&in.modules.zbug,&in.modules.union_core,&in.modules.parser_extender,&in.modules.runtime_crt})
      if(zbug_end_compat::detail::overlaps(m->base,m->image_size,locator.base,locator.image_size))return false;
    if(zbug_end_compat::detail::overlaps(in.renderer.base,in.renderer.image_size,locator.base,locator.image_size))return false;
  }
  return true;
}
inline native::ConstructorEvidence constructors(const ReadOnlyMemory& memory,const grind_recipe::Descriptor& recipe) noexcept {
  native::ConstructorEvidence out;
  const auto assembly=grind_recipe_runtime::assemble(recipe);
  if(!assembly.valid()){out.issue=native::Issue::constructor;return out;}
  const auto profile=assembly.profile();rs::detail::Layout layout{};
  if(rs::detail::read_layout(memory,lhiver::game_parser,profile,layout)!=rs::Issue::none){out.issue=native::Issue::constructor;return out;}
  lhiver::Digest first{};
  for(unsigned pass=0;pass<2;++pass){
    Sha256 hash;
    const auto verified=grind_recipe_runtime::verify_recipe(memory,lhiver::game_parser,recipe);
    if(!verified.valid){out.issue=native::Issue::constructor;out.symbol=verified.failed_symbol;return out;}
    for(const auto& field:grind_item_constructor::fields)
      if(rs::detail::check_binding(memory,layout,field)!=rs::Issue::none){out.issue=native::Issue::constructor;out.symbol=field.symbol;return out;}
    for(const auto id:{recipe.input,recipe.output,grind_recipe::lifecycle_item}){
      const auto symbol=static_cast<std::uint32_t>(id);
      const auto* shape=grind_item_shape::find(symbol);const auto* visual=grind_item_constructor::visual(symbol);
      const auto* item=grind_recipe_profile::item_for(symbol);
      if(!shape||!grind_item_shape::bound_to_profile(*shape)||!visual||!item||visual->constructor!=shape->constructor_sha256||
         visual->constructor!=item->function->body_sha256||!visual->length||visual->length>=64){out.issue=native::Issue::constructor;out.symbol=symbol;return out;}
      bool profiled=false;
      for(std::size_t i=0;i<item->text_count;++i)if(item->texts[i].symbol==visual->symbol)
        profiled=item->texts[i].length==visual->length&&item->texts[i].sha256==visual->digest;
      rs::detail::Symbol s{};std::array<std::uint8_t,20> h{};std::array<std::uint8_t,64> bytes{};
      if(!profiled||rs::detail::lookup(memory,layout,visual->symbol,s)!=rs::Issue::none||s.type!=3||s.flags!=1||s.elements!=1||s.parent||
         !native::detail::read_hash(memory,s.data,h.data(),h.size(),hash)||rs::detail::u32(h.data()+12)!=visual->length||
         !native::detail::read_hash(memory,rs::detail::u32(h.data()+8),bytes.data(),visual->length+1,hash)||bytes[visual->length]||
         std::memchr(bytes.data(),0,visual->length)||dep::detail::digest(bytes.data(),visual->length)!=visual->digest){
        out.issue=native::Issue::string;out.symbol=visual->symbol;return out;}
      for(const auto v:{symbol,s.address,s.data,s.offset})native::detail::word_hash(hash,v);
    }
    std::array<std::uint32_t,2> globals{};std::array<std::uint8_t,5> needle{};
    if(!native::detail::word(memory,0x8b7464,0,globals[0])||globals[0]!=1584||
       !native::detail::word(memory,0xab11e4,0,globals[1])||globals[1]!=1){out.issue=native::Issue::constructor;return out;}
    for(const auto v:globals)native::detail::word_hash(hash,v);
    if(!native::detail::read_hash(memory,0x8922a4,needle.data(),needle.size(),hash)||std::memcmp(needle.data(),".ZEN",5)){
      out.issue=native::Issue::string;return out;}
    const auto now=hash.finish();if(pass&&now!=first){out.issue=native::Issue::changed;return out;}first=now;
  }
  if(rs::detail::stable_layout(memory,lhiver::game_parser,profile,layout)!=rs::Issue::none){out.issue=native::Issue::changed;return out;}
  out.values_verified=true;out.fingerprint=first;return out;
}
inline bool outputs(Evidence& out) noexcept {
  const auto& stock=out.stock;
  if(!stock.captured()||!out.quantity||stock.quantity!=out.quantity)return false;
  for(std::size_t i=0;i<2;++i){
    const auto& row=stock.materials[i+1];const auto& object=stock.outputs[i];
    const auto* shape=grind_item_shape::find(static_cast<std::uint32_t>(row.symbol));
    if(!shape||!grind_item_shape::bound_to_profile(*shape)||row.listed<0||row.total!=row.listed||row.packed||row.packed_occurrences||
       row.duplicate_packed_target||row.has_active_marker||row.mixed_representation||row.listed_occurrences>1||stock.output_slot_alias[i])return false;
    if(!row.listed_occurrences){if(row.listed||object.object)return false;out.new_output_node[i]=true;}
    else if(!row.listed||!object.object||object.symbol!=static_cast<std::uint32_t>(row.symbol)||
      object.amount!=static_cast<std::uint32_t>(row.listed)||object.mainflag!=shape->mainflag||object.flags!=shape->native_flags||
      (object.flags&0x40000000u))return false;
    const std::int64_t amount=std::int64_t{row.total}+(i?1u:out.quantity);
    if(amount>INT32_MAX){out.issue=Issue::overflow;return false;}
    out.expected_after_callback[i+1]=static_cast<std::int32_t>(amount);
  }
  out.expected_after_callback[0]=static_cast<std::int32_t>(stock.amount_after);
  const auto extra=unsigned{out.new_output_node[0]}+unsigned{out.new_output_node[1]};
  if(extra>max_packed_inventory_nodes-stock.nodes_after){out.issue=Issue::overflow;return false;}
  return true;
}
inline bow_item_path::RemoveStep removal(const path::Snapshot& stock) noexcept {
  bow_item_path::RemoveStep out;out.operation.symbol=stock.input.stock.symbol;out.operation.amount=stock.quantity;
  out.source=bow_item_path::Source::listed;out.path=stock.removal;out.object=stock.input.item.object;out.refcount=stock.refcount;
  out.nodes_before=stock.listed_nodes;out.nodes_after=stock.nodes_after;return out;
}
inline bool whole(const Evidence& out) noexcept {
  if(out.stock.removal==bow_item_path::Removal::split)return true;
  return out.stock.removal==bow_item_path::Removal::whole&&out.world.stable&&out.world.current_whole_refcount_safe[0]&&
    out.stock.contents==out.key.hero+0x710u&&!out.stock.disabled&&!out.stock.pool_debug&&out.stock.selected<out.stock.nodes_after;
}
}

class Reader {
public:
  explicit Reader(std::uint32_t owner) noexcept:owner_(owner),inventory_(owner){}
  Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
  Evidence capture_writer(const ReadOnlyMemory& memory,const GrindRecipeCatalogueCapture& catalogue,
      std::uint32_t callback,std::uint32_t quantity,const Input& input) noexcept {
    return capture(memory,catalogue,callback,quantity,input,nullptr);
  }
  Evidence capture_preview(const ReadOnlyMemory& memory,const GrindRecipeCatalogueCapture& catalogue,
      const GrindGuildSnapshot& physical,std::uint32_t callback,std::uint32_t quantity,const Input& input) noexcept {
    return capture(memory,catalogue,callback,quantity,input,&physical);
  }
private:
  Evidence capture(const ReadOnlyMemory& memory,const GrindRecipeCatalogueCapture& catalogue,
      std::uint32_t callback,std::uint32_t quantity,const Input& in,const GrindGuildSnapshot* physical) noexcept {
    Evidence out;out.key=in.boundary.key;out.callback=callback;out.quantity=quantity;out.scope=physical?path::Scope::preview:path::Scope::writer;
    if(!owner_||in.boundary.current_thread!=owner_||out.key.thread!=owner_){out.issue=Issue::thread;return out;}
    if(reading_){out.issue=Issue::reentry;return out;}
    const auto* recipe=grind_recipe::find(callback);
    if(!recipe||!quantity||quantity>INT32_MAX||!grind_recipe_snapshot_detail::valid_key(out.key)||
       !grind_recipe_snapshot_detail::same_key(out.key,catalogue.key)||!in.boundary.stable_boundary_verified||!detail::separate_modules(in))return out;
    reading_=true;struct Lease{bool& value;~Lease(){value=false;}}lease{reading_};
    if(physical){
      out.physical=read_grind_guild_for_preview(memory,lhiver::game_parser,out.key);
      if(!out.physical.known()||!grind_guild_snapshot_detail::same(out.physical,*physical)||
         !grind_preview_knowledge(catalogue,out.physical,out.key).guild_known){out.issue=Issue::guild;return out;}
    }else{
      out.writer=read_grind_guild_for_writer(memory,lhiver::game_parser,out.key);out.physical=out.writer.physical;
      if(!grind_writer_knowledge(catalogue,out.writer,out.key).guild_known){out.issue=Issue::guild;return out;}
    }
    auto stock=[&]() noexcept {return physical?inventory_.capture_preview(memory,catalogue,out.physical,callback,quantity,in.boundary):
                                             inventory_.capture(memory,catalogue,callback,quantity,in.boundary);};
    out.stock=stock();if(!out.stock.captured()){out.issue=Issue::inventory;return out;}
    if(!detail::outputs(out)){if(out.issue!=Issue::overflow)out.issue=Issue::outputs;return out;}
    std::uint32_t game=0;
    if(!native::detail::word(memory,selection_abi::game_pointer,0,game)||!bow_item_runtime::detail::output_route(memory,game))return out;
    const alchemy_flask_debit::Context context{out.key.generation,out.key.thread,lhiver::game_parser,game,out.key.world,out.key.hero,out.key.station};
    out.modules=code::observe_modules(memory,in.modules,context);
    if(!out.modules.observed){out.issue=Issue::modules;out.failed_address=out.modules.code.address;return out;}
    const auto external=code::verify_externals(memory,lhiver::game_parser);
    if(!external.verified){out.issue=Issue::externals;out.failed_address=external.address;out.failed_symbol=external.symbol;return out;}
    out.constructors=detail::constructors(memory,*recipe);
    if(!out.constructors.values_verified){out.issue=Issue::constructors;out.failed_symbol=out.constructors.symbol;return out;}
    for(const auto checked:{native::verify_engine_parts(memory),
        native::verify_grind_routed_code(memory,in.modules,out.key,in.inventory_insert,&out.inventory_insert,&out.unpack)})
      if(!checked.verified){out.issue=Issue::native_code;out.failed_address=checked.address;return out;}
    out.renderer=renderer::verify_forwarding(memory,in.renderer);if(!out.renderer.verified){out.issue=Issue::renderer_path;return out;}
    // A debit may require two world-list nodes; each of the two real
    // CreateInvItems outputs at most one new inventory node. No N-factor.
    const std::uint32_t allocations=2u+unsigned{out.new_output_node[0]}+unsigned{out.new_output_node[1]};
    out.pool=native::verify_pool_allocations(memory,in.modules,allocations);if(!out.pool.verified){out.issue=Issue::pool;return out;}
    const auto free_nodes=native::read_pool_allocations(memory,allocations);
    if(!free_nodes.stable||free_nodes.fingerprint!=out.pool.fingerprint){out.issue=Issue::changed;return out;}
    const auto assembly=grind_recipe_runtime::assemble(*recipe);std::array<PackedInventoryTarget,3> targets{};
    for(std::size_t i=0;i<targets.size();++i){const auto id=out.stock.materials[i].symbol;
      targets[i]={grind_recipe_profile::item_for(static_cast<std::uint32_t>(id))->function->name,id};}
    out.sort=native::read_sort_values(memory,assembly.profile(),out.key.hero,out.stock.listed_nodes,targets.data(),targets.size());
    if(!out.sort.stable){out.issue=Issue::sorter;return out;}
    const auto remove=detail::removal(out.stock);
    const native::WorldListRequest world_request{out.key.hero,out.key.world,out.stock.listed_nodes,&remove,1};
    out.world=native::read_world_values(memory,world_request,free_nodes);if(!out.world.stable){out.issue=Issue::world;return out;}
    if(!detail::whole(out)){out.issue=Issue::whole_stack;return out;}
    const auto late=stock();const auto constructor_after=detail::constructors(memory,*recipe);
    const auto sort_after=native::read_sort_values(memory,assembly.profile(),out.key.hero,late.listed_nodes,targets.data(),targets.size());
    const auto pool_after=native::read_pool_allocations(memory,allocations);
    const auto world_after=native::read_world_values(memory,world_request,pool_after);
    const auto modules_after=code::observe_modules(memory,in.modules,context);
    const auto renderer_after=renderer::verify_forwarding(memory,in.renderer);
    const auto guild_after=read_grind_guild_for_preview(memory,lhiver::game_parser,out.key);
    vob_inventory_insert_compat::Verification insert_after;
    grind_zbug_unpack::Verification unpack_after;
    const auto routed_after=native::verify_grind_routed_code(memory,in.modules,out.key,in.inventory_insert,&insert_after,&unpack_after);
    if(!late.captured()||late.scope!=out.scope||late.fingerprint!=out.stock.fingerprint||
       !constructor_after.values_verified||constructor_after.fingerprint!=out.constructors.fingerprint||
       !sort_after.stable||sort_after.fingerprint!=out.sort.fingerprint||!pool_after.stable||pool_after.fingerprint!=out.pool.fingerprint||
       !world_after.stable||world_after.fingerprint!=out.world.fingerprint||!bow_item_runtime::detail::same_module_evidence(out.modules,modules_after)||
       !renderer::same(out.renderer,renderer_after)||!guild_after.known()||!grind_guild_snapshot_detail::same(out.physical,guild_after)||
       !bow_item_runtime::detail::output_route(memory,game)||!code::verify_externals(memory,lhiver::game_parser).verified||
       !native::verify_engine_parts(memory).verified||!routed_after.verified||
       !vob_inventory_insert_compat::same(out.inventory_insert,insert_after)||
       !grind_zbug_unpack::same(out.unpack,unpack_after)){out.issue=Issue::changed;return out;}
    if(!physical){const auto actor_after=read_grind_guild_for_writer(memory,lhiver::game_parser,out.key);
      if(!actor_after.known()||!station_recipe_runtime::same_actor_evidence(out.writer.actor_after,actor_after.actor_after)||
         !grind_guild_snapshot_detail::same(out.physical,actor_after.physical)){out.issue=Issue::changed;return out;}}
    out.item_path_verified=true;out.issue=Issue::none;return out;
  }
  std::uint32_t owner_=0;bool reading_=false;path::Reader inventory_;
};
// Compare own findings at the current root stable point. Preview
// findings must never be adopted as active SELF-checks into a Writer.
inline bool matches(const Evidence& evidence,const GrindRecipeKey& current,std::uint32_t callback,
    std::uint32_t quantity,path::Scope scope) noexcept {
  if(!evidence.item_path_verified||evidence.issue!=Issue::none||!evidence.stock.captured()||
     !grind_recipe_snapshot_detail::valid_key(current)||!grind_recipe_snapshot_detail::same_key(evidence.key,current)||
     !grind_recipe_snapshot_detail::same_key(evidence.stock.input.key,current)||evidence.callback!=callback||
     evidence.stock.input.callback!=callback||!quantity||evidence.quantity!=quantity||evidence.stock.quantity!=quantity||
     evidence.scope!=scope||evidence.stock.scope!=scope||!evidence.physical.known()||!evidence.stock.input.observed()||
     !grind_recipe_snapshot_detail::same_key(evidence.physical.key,current)||evidence.physical.hero!=current.hero||
     !evidence.modules.observed||!evidence.constructors.values_verified||!evidence.pool.verified||
     !evidence.sort.stable||!evidence.world.stable||!evidence.renderer.verified||!evidence.inventory_insert.valid||
     !grind_zbug_unpack::matches(evidence.unpack,current,evidence.unpack.binding))return false;
  if(scope==path::Scope::writer&&(!evidence.writer.known()||!grind_guild_snapshot_detail::same(evidence.writer.physical,evidence.physical)))return false;
  const auto* recipe=grind_recipe::find(callback);if(!recipe)return false;
  const std::array<std::int32_t,3> symbols{{recipe->input,recipe->output,grind_recipe::lifecycle_item}};
  for(std::size_t i=0;i<symbols.size();++i){
    const auto& row=evidence.stock.materials[i];if(row.symbol!=symbols[i]||row.total<0)return false;
    const auto delta=i==0?-static_cast<std::int64_t>(quantity):i==1?static_cast<std::int64_t>(quantity):1;
    const auto expected=std::int64_t{row.total}+delta;
    if(expected<0||expected>INT32_MAX||evidence.expected_after_callback[i]!=expected)return false;
  }
  if(evidence.pool.required!=2u+unsigned{evidence.new_output_node[0]}+unsigned{evidence.new_output_node[1]})return false;
  return true;
}
} // namespace ubc::grind_item_runtime
namespace ubc {using GrindItemRuntimeReader=grind_item_runtime::Reader;}
