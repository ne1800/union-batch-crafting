// SPDX-License-Identifier: MIT
// Linux-Diagnose im Testkoordinator. Kein Code aus dieser Datei läuft in Gothic.
#include "ubc/stations/cool/cool_predebit_reader.hpp"
#include "ubc/stations/cool/cool_native_path.hpp"
#include "ubc/engine/crafting_hook_sites.hpp"
#include "ubc/stations/station_end_contract.hpp"
#include <cstring>

namespace {
using namespace ubc;
namespace p=cool_predebit::profile;
namespace dep=alchemy_extra_dependencies;
namespace rt=alchemy_flask_runtime;
namespace z=zbug_end_compat;
namespace cat=cool_recipe_catalog;
namespace abi=cool_destroy_abi;
namespace dispatch=cool_destroy_dispatch_profile;
namespace end=station_end_contract;
namespace bn=bow_item_native;
namespace call=destroy_remove_callsite;
using Digest=lhiver::Digest;
using Emit=void(*)(const char* space,unsigned key,unsigned length,const unsigned char* digest,
                   const char* source,const std::uint16_t* reloc,unsigned count);
Digest hash(const void* bytes,std::size_t n){
  Sha256 sha;sha.update(bytes,n);return sha.finish();
}
struct Catalog {
  Emit emit;
  void site(const char* space,unsigned key,unsigned length,const Digest& digest,const char* source) const {
    emit(space,key,length,digest.data(),source,nullptr,0);
  }
  void bytes(const char* space,unsigned key,const void* data,unsigned size,const char* source) const {
    site(space,key,size,hash(data,size),source);
  }
  void word(unsigned va,unsigned value,const char* source) const {
    std::array<std::uint8_t,4> data{};dep::detail::put(data.data(),value);
    bytes("engine",va,data.data(),4,source);
  }
  void code(const char* space,const z::CodeRange& r,const char* source) const {
    emit(space,r.rva,r.length,r.sha256.data(),source,r.relocations,r.relocation_count);
  }
  void script(const lhiver::Function& f,const char* source) const {
    site("script",f.byte_offset,f.byte_length,f.body_sha256,source);
    bytes("name",f.symbol,f.name,std::strlen(f.name)+1,source);
  }
  void binding(const lhiver::SymbolBinding& b,const char* source) const {
    bytes("name",b.symbol,b.name,std::strlen(b.name)+1,source);
  }
};
}

extern "C" void cool_catalog(Emit emit){
  const Catalog c{emit};
  c.site("module",0,0x752000,station_abi::executable_sha256,"Gothic2.exe");
  c.site("module",1,z::zbug_image_size,z::zbug_file_sha256,"zBugFixes.dll");
  c.site("module",2,z::union_image_size,z::union_file_sha256,"Shw32.dll");
  c.site("module",3,dep::parser_extender_size,dep::parser_extender_sha256,"zParserExtender.dll");
  c.site("module",4,0xbe000,dep::crt_sha256,"msvcr100.dll");
  c.binding(dep::item_binding,"alchemy_extra_dependencies::read_item_binding");
  for(const auto& r:p::engine)c.site("engine",r.address,r.length,r.digest,"cool_predebit_profile::engine");
  for(const auto& r:p::data)c.site("engine",r.address,r.length,r.digest,"cool_predebit_profile::data");
  for(const auto i:p::split_engine_indices){
    const auto& r=bn::engine_ranges[i];c.site("engine",r.address,r.length,r.digest,"bow_item_native::engine_ranges (Split)");
  }
  for(const auto& r:rt::code_ranges)c.site("engine",r.address,r.length,r.digest,"alchemy_flask_runtime::code_ranges");
  for(const auto& r:dep::additional_ranges)c.site("engine",r.address,r.length,r.digest,"alchemy_extra_dependencies::additional_ranges");
  for(const auto& s:rt::vtable_entries)c.word(s.address,s.target,"alchemy_flask_runtime::vtable_entries");
  for(const auto& s:dep::additional_slots)c.word(s.address,s.target,"alchemy_extra_dependencies::additional_slots");
  c.site("engine",dep::factory.address,dep::factory.length,dep::factory.digest,"alchemy_extra_dependencies::factory");
  c.site("engine",0x791960,3536,dep::do_stack_hash,"alchemy_extra_dependencies::verify_known_code (DoStack)");
  c.site("engine",0x82fdb8,256,p::uppercase_sha256,"cool_predebit_reader::constructor (Upper)");
  c.bytes("engine",0x8922a4,".ZEN",5,"cool_predebit_reader::constructor (.ZEN)");
  for(const auto& r:dispatch::ranges)c.site("engine",r.address,r.length,r.sha256,"cool_destroy_dispatch_profile::ranges");
  for(std::size_t i=4;i<abi::identity_ranges.size();++i){
    const auto& r=abi::identity_ranges[i];c.site("engine",r.address,r.length,r.sha256,"cool_native_path::verify_dispatch");
  }
  for(std::size_t i=0;i<dispatch::class_chain.size();++i)
    c.word(dispatch::class_chain[i]+0x3c,i+1<dispatch::class_chain.size()?dispatch::class_chain[i+1]:0,"cool_destroy_dispatch::class_chain");
  c.word(dispatch::npc_vtable+0x28,dispatch::on_message,"cool_destroy_dispatch::OnMessage");
  c.word(dispatch::message_vtable,dispatch::class_getter,"cool_destroy_dispatch::class_getter");
  c.word(dispatch::message_vtable+0x24,dispatch::message_delete,"cool_destroy_dispatch::message_delete");
  c.word(0x82e8ac,0,"cool_destroy_dispatch::values (zero)");
  for(const auto& r:end::code_ranges)c.site("engine",r.address,r.length,r.sha256,"station_end_contract::code_ranges");
  for(const auto table:end::mob_vtables)for(const auto& slot:end::vtable_slots)
    c.word(table+slot.offset,slot.function,"station_end_contract::vtable_slots");
  for(const auto& r:crafting_hook_sites::sites)c.bytes("engine",r.address,r.original.data(),r.guard_size,"crafting_hook_sites::sites");
  c.bytes("engine",selection_abi::frame_prefix,selection_abi::frame_prefix_bytes.data(),selection_abi::frame_prefix_bytes.size(),"selection_abi::frame_prefix");
  c.bytes("engine",abi::destroy_entry,cool_native_path::destroy_prefix.data(),cool_native_path::destroy_prefix.size(),"cool_native_path::destroy_prefix");
  c.bytes("engine",call::address-call::prefix_size,call::original_context.data(),call::original_context.size(),"destroy_remove_callsite::original_context");
  for(const auto& r:z::zbug_code)c.code("zbug",r,"zbug_end_compat::zbug_code");
  for(const auto& r:z::union_code)c.code("union",r,"zbug_end_compat::union_code");
  const auto& put=bow_item_code::ranges[7];
  c.site("engine",put.address,put.length,put.digest,"zbug_end_compat::verify (original_put_in_inv)");
  c.bytes("zbug",0x50e84,"ITLSTORCHBURNING",16,"zbug_end_compat::read_dispatch");
  for(const auto& r:dep::zpe_ranges)c.code("zpe",r,"alchemy_extra_dependencies::zpe_ranges");
  for(const auto& r:{dep::thunk,dep::resolver,dep::version})c.code("union",r,"alchemy_extra_dependencies::read_pending_facts");
  c.code("union",cool_nofx::detail::free_thunk,"cool_nofx::free_route");
  const std::uint16_t malloc_reloc[]{22,44,50};
  c.code("crt",{0x10233,128,dep::crt_malloc_window_hash,malloc_reloc,3},"alchemy_extra_dependencies::read_pending_facts");
  for(const auto& r:p::free_code)c.code("crt",r,"cool_predebit_profile::free_code");
  for(const auto& r:cool_message_malloc::profile::malloc_code)c.code("crt",r,"cool_message_malloc_profile::malloc_code");
  c.bytes("crt",0x1014e,cool_nofx::detail::free_entry.data(),11,"cool_nofx::free_route (entry)");
  c.bytes("crt",0x10172,cool_nofx::detail::free_tail.data(),2,"cool_nofx::free_route (tail)");
  for(const auto& f:cat::functions)c.script(f,"cool_recipe_catalog::functions");
  for(const auto& f:dep::ikarus_functions)c.script(f,"alchemy_extra_dependencies::ikarus_functions");
  for(const auto& b:cat::bindings)c.binding(b,"cool_recipe_catalog::bindings");
  for(const auto& b:dep::ikarus_bindings)c.binding(b,"alchemy_extra_dependencies::ikarus_bindings");
  for(const auto& b:p::bindings)c.binding(b,"cool_predebit_profile::bindings");
  for(const auto i:p::constructor_field_indices)c.binding(bn::constructor_fields[i],"bow_item_native::constructor_fields (Split)");
  for(const auto& t:cat::texts)c.site("text",t.symbol,t.length,t.sha256,"cool_recipe_catalog::texts");
  for(const auto& t:p::texts)c.site("text",t.symbol,t.length,t.digest,"cool_predebit_profile::texts");
  c.word(0xab2668,0,"cool_destroy_dispatch::values (skip)");
  c.word(0x8b7464,1584,"cool_predebit_reader::constructor (C_ITEM)");
  std::array<std::uint8_t,4> original{{1,0,0,0}};
  c.bytes("script",cat::single.information->byte_offset+62,original.data(),4,"AmountPatch: Operand 62 vor Auftrag");
  c.site("alternative",p::object_created.address,p::object_created.length,p::union_object_created_sha256,"Union.patch G2A: ObjectCreated");
}

// Ergänzende, bedingte Prüfungen mit den echten lesenden Produktionsroutinen.
// Der Aufrufer protokolliert jede angeforderte Adresse mit zwei unabhängigen
// Kopien. Ein negatives Teilergebnis beendet keine andere Prüfung.
using Read=bool(*)(void*,std::uint32_t,void*,std::size_t) noexcept;
using Group=void(*)(const char*);
using Result=void(*)(bool);
extern "C" void cool_dynamic(Read read,Group group,Result result,unsigned engine,unsigned zbug,unsigned core,unsigned zpe,unsigned crt){
  const ReadOnlyMemory m{nullptr,read};
  // Der Koordinator übergibt hier ausschließlich an Inode und Sollhash
  // gebundene Modulbasen. Unbestätigte Kandidaten werden als 0 übergeben.
  const dep::Module exe{engine,0x752000,station_abi::executable_sha256,engine==0x400000};
  const dep::Modules mods{{zbug,z::zbug_image_size,z::zbug_file_sha256,zbug!=0},
    {core,z::union_image_size,z::union_file_sha256,core!=0},
    {zpe,dep::parser_extender_size,dep::parser_extender_sha256,zpe!=0},
    {crt,0xbe000,dep::crt_sha256,crt!=0}};
  group("modules.PE");result(dep::detail::modules(m,mods).valid);
  group("Gothic2.PE");result(exe.mapping_verified&&dep::detail::pe(m,exe,0x3f966d87,0x400000).valid);
  group("zbug_end_compat::verify");result(z::verify(m,{mods.zbug,mods.union_core}).valid);
  group("cool_set_interact_path::verify");result(cool_set_interact_path::verify(m,{mods.zbug,mods.union_core}).valid);
  group("alchemy_extra_dependencies::verify_known_code");result(dep::verify_known_code(m,mods,dep::expected_targets).known_ranges_match);
  group("alchemy_extra_dependencies::read_pending_facts");
  const auto facts=dep::read_pending_facts(m,mods);result(facts.read_stable&&facts.allocator_boundary_verified&&facts.wrapper_dispatch_verified);
  group("cool_predebit_reader::crt");cool_predebit::reader::Input q;q.modules=mods;
  cool_predebit::check::Checks crt_checks;cool_predebit::reader::detail::crt(m,q,crt_checks);result(crt_checks.ok());
  group("cool_recipe_runtime::verify_recipe");result(cool_recipe_runtime::verify_recipe(m,lhiver::game_parser,cat::recipe).valid);
  group("runtime_script::ikarus_profile");result(runtime_script::verify_local_contract(m,lhiver::game_parser,dep::ikarus_profile).matches);
  group("DoStack.Ikarus");unsigned handler=0;
  result(dep::detail::word(m,0x791960,3,handler)&&(handler==0x82a880||dep::detail::ikarus_handler(m,handler).valid));
  group("pool_writes");cool_predebit::check::Checks pool_checks;result(cool_predebit::reader::detail::pool_writes(m,pool_checks).known);
  group("inventory");unsigned hero=0;static PackedInventoryWorkspace inventory;
  const bool have_hero=dep::detail::word(m,station_abi::hero_pointer,0,hero)&&hero;
  const auto stock=read_packed_inventory(m,hero,cat::targets,true,inventory);
  result(have_hero&&stock.snapshot_unchanged&&stock.storage_totals_known);
  group("hands");station_recipe_snapshot_detail::HandWorkspace hands;
  const bool hands_verified=have_hero&&station_recipe_snapshot_detail::begin_hands(m,hero,hands)==StationHandReadIssue::none&&
                            station_recipe_snapshot_detail::verify_hands(m,hands);
  result(hands_verified);
  group("Split.Listeneintraege");
  const bool items_verified=stock.listed_known&&stock.listed_nodes<=inventory.records.size();
  bool listed=stock.snapshot_unchanged&&items_verified&&hands_verified;
  for(std::size_t i=0;items_verified&&i<stock.listed_nodes;++i){
    const auto object=runtime_script::detail::u32(inventory.records[i].node.data()+4);
    bn::path::detail::Fields fields;
    if(!bn::path::detail::fields(m,object,fields)){listed=false;continue;}
    if(!hands_verified)continue;
    for(std::size_t j=0;j<hands.count;++j){
      const auto& slot=hands.records[j].fields;
      if(runtime_script::detail::u32(slot.data()+0x30)!=object||runtime_script::detail::u32(slot.data()+0x18)||fields.symbol==16612)continue;
      std::array<std::uint8_t,0x348> equipped{};
      if(!m.copy(object,equipped.data(),equipped.size()))listed=false;
    }
  }
  result(listed);
  group("Split.Weltlisten");
  unsigned game=0,world=0;
  const bool have_world=dep::detail::word(m,selection_abi::game_pointer,0,game)&&game&&dep::detail::word(m,game,8,world)&&world;
  // Bei Split vergleicht der Produktionsreader nur vorhandene Listen, auch
  // ohne heißen Stapel im Ladeslot. Kein erfundenes Objekt, kein Entzug.
  bn::path::RemoveStep split;split.path=bn::path::Removal::split;
  result(have_world&&stock.snapshot_unchanged&&bn::read_world_values(m,{hero,world,stock.listed_nodes,&split,1},
         bn::read_pool_allocations(m,2)).stable);
  group("Split.Konstruktor");
  runtime_script::detail::Layout layout{};
  const auto profile=cool_recipe_runtime::assemble(cat::recipe).profile();
  cool_predebit::reader::detail::Literals literals;
  q.engine=exe;
  q.job.context.parser=lhiver::game_parser;q.job.context.game=game;q.job.context.world=world;q.job.context.hero=hero;
  const bool parser_ok=runtime_script::detail::read_layout(m,lhiver::game_parser,profile,layout)==runtime_script::Issue::none;
  cool_predebit::check::Checks constructor_checks;
  if(parser_ok)cool_predebit::reader::detail::constructor(m,q,layout,literals,constructor_checks);
  result(parser_ok&&constructor_checks.ok());
  group("Destroy.Dispatchwerte");
  cool_destroy_dispatch::Evidence evidence;evidence.context.hero=hero;
  evidence.scope=cool_destroy_dispatch::Scope::before_writer;Sha256 fingerprint;
  result(have_hero&&cool_destroy_dispatch::detail::values(m,fingerprint,evidence));

}

extern "C" void cool_station(Read read,Group group,Result result){
  const ReadOnlyMemory m{nullptr,read};
  group("SMITHCOOL.Fokus");
  unsigned hero=0,mob=0;
  const bool resolved=dep::detail::word(m,station_abi::hero_pointer,0,hero)&&hero&&
                      dep::detail::word(m,hero,0x9ac,mob)&&mob;
  result(resolved);
  const auto& station=cat::station;
  for(const auto& field:std::array<std::pair<unsigned,const char*>,4>{{
      {0x1e0,station.on_state},{0x1b8,station.scheme},{0x1cc,station.condition},{0x1a4,station.use_with}}}){
    group(field.second[0]?field.second:"use_with");
    result(resolved&&rt::detail::name_is(m,mob,field.first,field.second));
  }
  group("station_end_contract::verify_vtable");
  unsigned table=0;
  result(resolved&&dep::detail::word(m,mob,0,table)&&end::detail::verify_vtable(m,table).valid);
}
