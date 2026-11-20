// SPDX-License-Identifier: MIT
// Reuse our fixture values. No engine execution or boolean substitute
// for a code guard. PE and DAT bodies come from original files.
#define main bow_item_native_fixture_main
#include "bow_item_native_tests.cpp"
#undef main
#include "ubc/stations/bow/bow_item_runtime.hpp"
#include "ubc/stations/bow/crafting_bow_execution.hpp"

namespace {
namespace runtime=ubc::bow_item_runtime;namespace renderer_code=ubc::bow_item_renderer;
constexpr std::uint32_t runtime_game=0x18000,renderer_base=0x76030000;
constexpr std::uint32_t renderer_state=0x64000000,renderer_device=0x64100000;
runtime::Reader runtime_reader(7);
runtime::Input nested_input{};
struct RuntimeFixture:ModuleFixture {
  RuntimeFixture(const PE& exe,const Dat& dat,const PE& zbug,const PE& core,const PE& crt,const PE& zpe,const PE& renderer,
      const cat::Descriptor& d=cat::recipes.front(),unsigned amount=2,unsigned packed_mask=0,bool whole=false,bool output_merge=false):ModuleFixture(exe,zbug,core){
    growth(exe,crt);
    // Load true ZPE bodies, then actual imports/dispatch records as synthetic
    // loader values. No runtime file/identity is claimed by this host.
    zpe.load(image,modules.parser_extender.base);const auto z=modules.parser_extender.base,u=modules.union_core.base;
    for(const auto address:{0xc0b1cu,0xc44c0u,0xa91fcu,0xa9230u,0xa92e4u})image.blocks.erase(z+address);
    image.put(z+0xc0b1c,ubc::lhiver::game_parser);image.put(z+0xc44c0,0x63000000);image.put(z+0xa91fc,u+0x31aa0);
    image.put(z+0xa9230,u+0x2d670);image.put(z+0xa92e4,u+0x3e96c8);
    Fixture values(d);values.constructors(dat,d);
    values.make_symbol(1682,"ITEM",0,7,1,0,0,1584);
    for(const auto& b:ubc::bow_item_code::bindings){const auto entry=b.symbol==160?0x6e72e0u:b.symbol==170?0x6e7b60u:b.symbol==184?0x6e8200u:0u;
      values.make_symbol(b.symbol,b.name,b.elements,b.type,b.flags,entry,static_cast<std::uint32_t>(b.offset),b.parent_index);}
    values.image.put(hero+0x668,path::inventory_vtable);values.image.put(hero+0x66c,hero+0x710);
    values.image.put(hero+0x708,hero);values.image.put(hero+0x70c,1);values.image.put(hero+0x964,0x8800);
    constexpr std::uint32_t slots=0x30000,left=0x31000,right=0x31100;
    values.image.block(slots,8);values.image.block(left,0x34);values.image.block(right,0x34);
    text(values.image,0x32000,"ZS_LEFTHAND");text(values.image,0x32100,"ZS_RIGHTHAND");
    values.image.put(hero+0x9b0,slots);values.image.put(hero+0x9b4,2);values.image.put(hero+0x9b8,2);
    values.image.put(slots,left);values.image.put(slots+4,right);values.image.put(left+8,0x32000);values.image.put(left+12,11);
    values.image.put(right+8,0x32100);values.image.put(right+12,12);
    std::string packed;std::vector<std::pair<std::int32_t,std::uint32_t>> items;
    for(std::size_t i=0;i<d.costs.count;++i){const auto& cost=d.costs[i];const auto minimum=cat::units(d.minima,cost.symbol);
      const auto total=(minimum>cost.units?minimum:cost.units)*amount+(whole?0u:20u);
      if(packed_mask&(1u<<i)){if(!packed.empty())packed+=",";
        for(const auto& t:d.targets)if(t.symbol==cost.symbol)packed+=std::string(t.name)+":"+std::to_string(total);
      }else items.emplace_back(cost.symbol,total);}
    items.emplace_back(output_merge?d.outputs[0].symbol:0,3);
    for(std::size_t i=0;i<items.size();++i){const auto node=0x11000u+static_cast<std::uint32_t>(i)*16;
      const auto object=0x100000u+static_cast<std::uint32_t>(i)*0x400;
      values.image.block(node,12);values.image.put(node+4,object);values.image.put(node+8,i+1<items.size()?node+16:0);
      values.image.block(object,0x348);values.image.put(object,ubc::hand_detail::item_vtable);values.image.put(object+4,1);
      values.image.put(object+0x32c,items[i].second);values.image.put(object+0x330,static_cast<std::uint32_t>(items[i].first));
      std::uint32_t mainflag=1,flags=0x200001;for(const auto& shape:d.items)if(shape.symbol==items[i].first){mainflag=shape.mainflag;flags=shape.native_flags;}
      values.image.put(object+0x154,mainflag);values.image.put(object+0x158,flags);
    }
    values.image.put(hero+0x718,0x11000);text(values.image,0x40000,packed);
    values.image.put(hero+0x724,packed.empty()?0:0x40000);values.image.put(hero+0x728,static_cast<std::uint32_t>(packed.size()));
    for(const auto& b:values.image.blocks)image.blocks[b.first]=b.second;
    put(image,ubc::selection_abi::game_pointer,runtime_game);image.block(runtime_game,0x30);image.put(runtime_game,0x83c28c);image.put(runtime_game+8,world);
    image.put(world,0x83e174);put(image,ubc::station_abi::hero_pointer,hero);put(image,0x8d8df0,0x19000);put(image,0x19000,0x82f104);
    put(image,0xab1168+0x3c,0xab3510);put(image,0xab3510+0x3c,0);
    for(const auto& area:n::engine_ranges)exe.copy(image,area.address,area.length);
    for(const auto& area:ubc::alchemy_flask_runtime::code_ranges)exe.copy(image,area.address,area.length);
    for(const auto& area:n::dep::additional_ranges)exe.copy(image,area.address,area.length);
    for(const auto& slot:ubc::alchemy_flask_runtime::vtable_entries)exe.copy(image,slot.address,4);
    for(const auto& slot:n::dep::additional_slots)exe.copy(image,slot.address,4);
    exe.copy(image,0x76fde0,185);exe.copy(image,0x791960,3536);
    renderer.load(image,renderer_base);
    for(std::size_t i=0;i<renderer_code::detail::hooks.size();++i){const auto& h=renderer_code::detail::hooks[i];
      *image.at(h.entry,1)=0xe9;image.put(h.entry+1,renderer_base+h.rva-h.entry-5u);
      for(std::uint32_t j=5;j<h.length;++j)*image.at(h.entry+j,1)=0x90;
      const auto t=0x51000000u+static_cast<std::uint32_t>(i)*0x100u;image.block(t,h.length+5);std::memcpy(image.at(t,h.length),h.prologue.data(),h.length);
      *image.at(t+h.length,1)=0xe9;image.put(t+h.length+1,h.entry+h.length-t-h.length-5);image.put(renderer_base+h.slot,t);
    }
    image.block(renderer_state,0x600);image.block(renderer_device,0x208);image.put(renderer_base+0xea6ac,renderer_state);image.put(renderer_base+0xea6a0,renderer_device);
    image.put(renderer_device,renderer_base+0xcc7dc);image.put(renderer_base+0xcc7dc+0xbc,renderer_base+0x35e40);
    // A real nonempty renderer data area is allowed; its cache contents do not
    // determine whether the exact known wrapper will forward the original.
    image.put(renderer_state+0x57c,123);
    // One coherent EXE image; no overlapping short fixture block can mask a
    // changed byte in the corresponding full-function comparison.
    Image engine;exe.load(engine,0x400000);
    for(auto it=image.blocks.begin();it!=image.blocks.end();){
      if(it->first>=0x400000&&std::uint64_t{it->first}+it->second.size()<=0xb52000){
        std::memcpy(engine.at(it->first,it->second.size()),it->second.data(),it->second.size());it=image.blocks.erase(it);
      }else ++it;
    }
    image.blocks[0x400000]=std::move(engine.blocks[0x400000]);
    patch_call(image,0x70d0e6,modules.zbug.base+0x11e30);patch_call(image,0x76fe7f,z+0x7d560);patch_call(image,0x7926fc,z+0x71c70);
  }
  runtime::Input input()const{return {snapshot().boundary,modules,{renderer_base,renderer_code::image_size,renderer_code::file_sha256,true}};}
  runtime::Evidence capture(const cat::Descriptor& d=cat::recipes.front(),unsigned amount=2){return runtime_reader.capture(image.memory(),d,amount,input());}
};
void nested_runtime(Image& image) noexcept {check(runtime_reader.capture(image.memory(),cat::recipes.front(),2,nested_input).issue==runtime::Issue::reentry,"whole reader reentry is denied without shared workspace");}
struct LateBowTarget {
  Image& image;
  std::uint32_t operand,replacement;
  bool changed=false;
  static bool read(void* context,std::uint32_t address,void* output,std::size_t size) noexcept {
    auto& state=*static_cast<LateBowTarget*>(context);
    // The renderer check runs between the two full item path checks.
    if(!state.changed&&address==renderer_base+renderer_code::ranges[0].rva){
      state.image.put(state.operand,state.replacement);
      state.changed=true;
    }
    return Image::read(&state.image,address,output,size);
  }
};
ubc::CraftingRequest bound_request(const runtime::Evidence& e,const cat::Descriptor& d,unsigned amount){
  const auto& k=e.stock.boundary.key;
  ubc::CraftingRequest r;r.id=static_cast<std::uint32_t>(d.id);r.quantity=amount;
  r.key={k.generation,k.thread,k.world,k.hero,k.station,k.manager,ubc::crafting_catalog::Station::bow};
  r.hands=e.stock.hands;r.material_snapshot=ubc::make_crafting_state(r.id);auto& state=r.material_snapshot;
  state.generation=k.generation;state.maximum=amount;
  // The selection prerequisites are explicit values in our fixture. The
  // underlying inventory and code evidence comes from the actual reader.
  state.evaluated=state.knowledge_read_valid=state.known=state.contract=state.actor_known=state.actor_matches=true;
  state.hand_known=state.material_known=state.material_verified=state.context_known=state.context_matches=true;
  for(std::size_t i=0;i<state.target_count;++i)state.materials[i].stored=e.stock.materials[i].total;
  return r;
}
void runtime_files(const char* exe_file,const char* archive_file,const char* zbug_file,const char* union_file,const char* crt_file,const char* zpe_file,const char* renderer_file){
  const PE exe(exe_file,ubc::station_abi::executable_sha256),zbug(zbug_file,ubc::zbug_end_compat::zbug_file_sha256),core(union_file,ubc::zbug_end_compat::union_file_sha256);
  const PE crt(crt_file,n::dep::crt_sha256),zpe(zpe_file,n::dep::parser_extender_sha256),renderer(renderer_file,renderer_code::file_sha256);const Dat dat(archive_file);
  {RuntimeFixture f(exe,dat,zbug,core,crt,zpe,renderer);const auto result=f.capture();
    if(!result.item_path_verified)std::fprintf(stderr,"combined issue %u address %x symbol %u; module issue %u / %u\n",static_cast<unsigned>(result.issue),result.failed_address,result.failed_symbol,static_cast<unsigned>(result.modules.issue),static_cast<unsigned>(result.modules.code.issue));
    check(result.item_path_verified,"all real pinned bodies plus explicit synthetic RAM compose a positive item path");}
  // World run 5: both the zBug thunk at +0x9b00 and the Insert slot forward the call.
  for(unsigned fault=0;fault<9;++fault){
    const auto& d=cat::recipes[1];
    RuntimeFixture f(exe,dat,zbug,core,crt,zpe,renderer,d,1,fault==5?1:0);
    auto in=f.input();
    constexpr std::uint32_t locator_base=0x74e20000;
    const auto& profile=ubc::vob_locator_compat::module_profiles[0];
    const auto executable=[](std::uint32_t base,std::uint32_t target) noexcept {
      return target==base+0x11e30||target==base+0x50f50;
    };
    in.modules.zbug.executable_page=executable;
    in.locator={locator_base,profile.image_size,profile.file_sha256,true,executable};
    const auto thunk=in.modules.zbug.base+ubc::grind_zbug_unpack::thunk_rva;
    f.image.put(thunk+11,in.modules.zbug.base+0x11e30);
    f.image.put(ubc::vob_inventory_insert_compat::slot_address,locator_base+0x50f50);
    if(fault==0){
      namespace native=ubc::bow_item_native;
      native::BowDispatch tobacco; // Measured full packed stock before the mod's honey tobacco
      const auto inspect=[&]{return native::detail::bow_unpack(f.image.memory(),in.modules.zbug,tobacco,
        native::detail::PackedHasItems::observed_tobacco_wrapper);};
      check(inspect().verified&&tobacco.unpack_target==in.modules.zbug.base+0x11e30,"tabak binds exact observed nonempty-pack target");
      f.image.put(thunk+11,ubc::grind_zbug_unpack::original_target);
      check(!inspect().verified,"tabak rejects changed target before child");
      tobacco.unpack_target=0;f.image.put(thunk+11,in.modules.zbug.base+0x50f50);
      check(!inspect().verified,"tabak rejects another executable RVA for nonempty pack");
      f.image.put(thunk+11,in.modules.zbug.base+0x11e30);
    }
    if(fault==1)in.modules.zbug.executable_page=nullptr;
    if(fault==2)in.locator.executable_page=nullptr;
    if(fault==3)f.image.put(thunk+11,locator_base+0x50f50);
    if(fault==4)f.image.at(thunk,23)[10]^=1;
    if(fault==6)in.locator.mapping_verified=false;
    const auto slot=ubc::vob_inventory_insert_compat::slot_address;
    LateBowTarget change{f.image,fault==7?thunk+11:slot,
                         fault==7?ubc::grind_zbug_unpack::original_target:ubc::vob_inventory_insert_compat::original_target};
    auto memory=f.image.memory();
    if(fault>=7)memory={&change,&LateBowTarget::read};
    const auto result=runtime_reader.capture(memory,d,1,in);
    check(result.item_path_verified==(fault==0),"two measured forwards require intact source, mapped executable owners and empty pack");
    if(fault>=7)check(change.changed&&result.issue==runtime::Issue::changed&&result.failed_address==(fault==7?thunk:slot),
                     "late switch to another valid entry rejects and retains its source address");
  }
  for(const auto& d:cat::recipes)for(unsigned amount:{1u,7u}){
    RuntimeFixture f(exe,dat,zbug,core,crt,zpe,renderer,d,amount,amount==1?0u:7u,false,amount==1);
    const auto evidence=f.capture(d,amount);
    check(evidence.item_path_verified,"every canonical recipe admits exact real code with distinct quantities and pure pack/new-output path");
    const auto request=bound_request(evidence,d,amount);
    check(ubc::matching_bow_item_path(evidence,request,evidence.stock.boundary.frame),"actual positive code reader composes with canonical Root quantity and exact end deltas");
    auto other=request;other.quantity=amount==1?7u:1u;
    check(!ubc::matching_bow_item_path(evidence,other,evidence.stock.boundary.frame),"positive native item path is not transferable between N1 and N7");}
  for(unsigned mask=0;mask<8;++mask)for(bool whole:{false,true})for(bool merge:{false,true}){
    RuntimeFixture f(exe,dat,zbug,core,crt,zpe,renderer,cat::recipes.front(),2,mask,whole,merge);const auto e=f.capture();
    check(e.item_path_verified,"pure packed/listed, split/whole and new/merge output preserve exact original path");
    if(e.item_path_verified)for(std::size_t i=0;i<cat::recipes.front().targets.count;++i){const auto& row=e.stock.materials[i];
      const auto expected=std::int64_t{row.total}-std::int64_t{cat::units(cat::recipes.front().costs,row.symbol)}*2+
        std::int64_t{cat::units(cat::recipes.front().outputs,row.symbol)}*2;
      check(e.expected_totals[i]==expected,"every copied target has exact required final quantity");}
  }
  for(unsigned which=0;which<12;++which){RuntimeFixture f(exe,dat,zbug,core,crt,zpe,renderer);
    if(which==0)*f.image.at(renderer_base+0x7ec10,1)^=1;
    if(which==1)f.image.put(renderer_device,renderer_base+0xcc7e0);
    if(which==2)f.image.put(hero+0x710,path::sorter+1);
    if(which==3)f.image.put(station_test::symbol(170)+0x18,0x6e7b61);
    if(which==4)f.image.put(0x100000+0x158,0x40000000);
    if(which==5)f.image.put(hero+0x968,0x100000);
    if(which==6)f.image.put(0x8b7464,1585);
    if(which==7)f.image.put(runtime_game+8,world+4);
    if(which==8)f.image.put(0x83c804+0x78,0x711421);
    if(which==9)f.image.put(0x82e30c,f.modules.union_core.base+0x35451);
    if(which==10)f.image.put(runtime_game+0x28,1);
    if(which==11)f.image.put(0x83e174+0x34,0x7800c1);
    check(!f.capture().item_path_verified,"changed code, dispatch, material, constructor or actual context never grants item path");
  }
  {RuntimeFixture f(exe,dat,zbug,core,crt,zpe,renderer);auto in=f.input();in.modules.zbug.mapping_verified=false;
    check(!runtime_reader.capture(f.image.memory(),cat::recipes.front(),2,in).item_path_verified,"missing actual module lease remains blocked");
    in=f.input();in.renderer.base=in.modules.zbug.base;check(!runtime_reader.capture(f.image.memory(),cat::recipes.front(),2,in).item_path_verified,"overlapping module identities reject");
    in=f.input();in.boundary.current_thread=8;check(!runtime_reader.capture(f.image.memory(),cat::recipes.front(),2,in).item_path_verified,"foreign thread cannot enter reader");}
  {RuntimeFixture f(exe,dat,zbug,core,crt,zpe,renderer);nested_input=f.input();f.image.on_read=&nested_runtime;check(f.capture().item_path_verified,"outer composed reader survives nested attempt");}
  {RuntimeFixture f(exe,dat,zbug,core,crt,zpe,renderer);f.image.change_address=renderer_base+0xea6a0;f.image.change_target=0x100000+0x32c;f.image.change_on_read=1;
    check(!f.capture().item_path_verified,"material change between first inventory and renderer proof is found by final recapture");}
  {RuntimeFixture f(exe,dat,zbug,core,crt,zpe,renderer,cat::recipes.front(),2,0,true);f.image.put(hero+0x68c,1);
    check(!f.capture().item_path_verified,"last whole removal must leave original selected index valid");}
  {RuntimeFixture f(exe,dat,zbug,core,crt,zpe,renderer,cat::recipes.front(),2,0,true);
    f.image.block(0x72000,12);f.image.block(0x72100,12);f.image.put(0x70008,0x72000);f.image.put(0x71008,0x72100);
    f.image.put(0x72004,0x100000);f.image.put(0x72104,0x100000);
    check(!f.capture().item_path_verified,"Ref1 in both current world lists gains no invented release protection");}
}
}
// --local-files EXE LHIVER_MOD ZBUG UNION CRT ZPE RENDERER: the second argument is the original archive, not an extracted DAT.
#ifndef UBC_BOW_ITEM_RUNTIME_FIXTURE_MAIN
#define UBC_BOW_ITEM_RUNTIME_FIXTURE_MAIN main
#endif
int UBC_BOW_ITEM_RUNTIME_FIXTURE_MAIN(int argc,char** argv){Image none;runtime::Input empty;check(!runtime_reader.capture(none.memory(),cat::recipes.front(),2,empty).item_path_verified,"missing whole boundary is never positive");
  if(argc==9&&std::string(argv[1])=="--local-files")runtime_files(argv[2],argv[3],argv[4],argv[5],argv[6],argv[7],argv[8]);else if(argc!=1)return 2;
  std::printf("%u checks, %u failures\n",station_test::checks,station_test::failures);return station_test::failures?1:0;}
