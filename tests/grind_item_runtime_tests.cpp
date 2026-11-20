// SPDX-License-Identifier: MIT
// Use the same actual PE and DAT readers and module fixtures as
// the existing bow check, then only our specific grind values.
#define UBC_BOW_ITEM_RUNTIME_FIXTURE_MAIN bow_runtime_fixture_main
#include "bow_item_runtime_tests.cpp"
#include "ubc/grind_item_runtime.hpp"

namespace grind_runtime_test {
namespace g=ubc::grind_recipe;
namespace rt=ubc::grind_recipe_runtime;
namespace run=ubc::grind_item_runtime;
namespace profile=ubc::grind_recipe_profile;
using namespace bow_item_test;
using station_test::symbol;
constexpr std::uint32_t input_object=0x100000,output_object=0x100400,cold_object=0x100800,hand_object=0x150000;
ubc::GrindItemRuntimeReader reader{7};
ubc::GrindRecipeCatalogueReader catalogue_reader;
ubc::GrindRecipeKey key(){return {1,8,7,world,hero,0x8800,0x9000,11};}
struct GrindFixture:RuntimeFixture {
  GrindFixture(const PE& exe,const Dat& dat,const PE& zbug,const PE& core,const PE& crt,const PE& zpe,const PE& renderer,
      const g::Descriptor& recipe=g::recipes[0],std::int32_t amount=5,bool output=false,bool cold=false)
      :RuntimeFixture(exe,dat,zbug,core,crt,zpe,renderer){
    bow_test::Fixture values(cat::recipes.front());
    auto populate=[&](const ubc::lhiver::Profile& p){
      for(std::size_t i=0;i<p.binding_count;++i){const auto& b=p.bindings[i];
        values.make_symbol(b.symbol,b.name,b.elements,b.type,b.flags,b.check_value?static_cast<std::uint32_t>(b.value):0,
          static_cast<std::uint32_t>(b.offset),b.parent_index);}
      for(std::size_t i=0;i<p.function_count;++i){const auto& f=p.functions[i];
        values.make_symbol(f.symbol,f.name,f.argc,f.type,f.flags,f.byte_offset,f.type==7?0x61000000u:f.return_type,f.parent_index);
        values.image.block(station_test::code+f.byte_offset,f.byte_length);
        std::memcpy(values.image.at(station_test::code+f.byte_offset,f.byte_length),dat.code.data()+f.byte_offset,f.byte_length);}
    };
    for(const auto& d:g::recipes){const auto assembly=rt::assemble(d);populate(assembly.profile());}
    for(const auto& item:profile::items)populate(rt::item_profile(item));
    auto literal=[&](const profile::Text& t){
      const auto data=0x60000000u+t.symbol*0x200u;
      if(!values.image.at(symbol(t.symbol),0x3c))values.make_symbol(t.symbol,"LITERAL",1,3,1,data,0,-1);
      values.image.put(symbol(t.symbol)+0x18,data);values.image.block(data,20);
      values.image.put(data+8,data+32);values.image.put(data+12,t.length);values.image.put(data+16,t.length+1);
      text(values.image,data+32,dat.strings.at(t.symbol));
    };
    for(const auto& t:profile::common_texts)literal(t);
    for(const auto& texts:profile::recipe_texts)for(const auto& t:texts.texts)literal(t);
    for(const auto& item:profile::items)for(std::size_t i=0;i<item.text_count;++i)literal(item.texts[i]);
    values.make_symbol(1682,"ITEM",0,7,1,0,0,1584);
    for(const auto& b:ubc::bow_item_code::bindings){const auto address=b.symbol==160?0x6e72e0u:b.symbol==170?0x6e7b60u:b.symbol==184?0x6e8200u:0u;
      values.make_symbol(b.symbol,b.name,b.elements,b.type,b.flags,address,static_cast<std::uint32_t>(b.offset),b.parent_index);}
    values.make_symbol(0,"ZZZ",0,7,1,0,0,-1);
    values.image.put(symbol(1683)+0x1c,hero);values.image.put(symbol(1679)+0x1c,hero);
    values.image.put(symbol(12330)+0x18,11);values.image.put(symbol(10502)+0x18,1);
    values.image.put(symbol(10501)+0x18,1);values.image.put(symbol(10505)+0x18,98);
    for(const auto& block:values.image.blocks)image.blocks[block.first]=block.second;
    // Keep the same contiguous EXE test image. A short
    // new parser block must not mask later engine globals.
    auto& engine=image.blocks.at(0x400000);
    for(auto it=image.blocks.begin();it!=image.blocks.end();){
      if(it->first>0x400000&&std::uint64_t{it->first}+it->second.size()<=0x400000+engine.size()){
        std::memcpy(engine.data()+(it->first-0x400000),it->second.data(),it->second.size());it=image.blocks.erase(it);
      }else ++it;
    }
    image.put(hero+0x230,static_cast<std::uint32_t>(recipe.guilds[0]));
    item(input_object,recipe.input,amount);item(output_object,output?recipe.output:0,3);item(cold_object,16613,2);
    image.put(hero+0x718,0x11000);image.put(0x11004,input_object);image.put(0x11008,0x11010);
    image.put(0x11014,output_object);image.put(0x11018,cold?0x11020:0);image.put(0x11024,cold_object);image.put(0x11028,0);
    image.put(hero+0x724,0);image.put(hero+0x728,0);text(image,0x40000,"");
    image.put(hero+0x710,ubc::bow_item_path::sorter);image.put(hero+0x668,ubc::bow_item_path::inventory_vtable);
    image.put(hero+0x66c,hero+0x710);image.put(hero+0x68c,0);image.put(hero+0x700,0);
    item(hand_object,16613,1);image.put(hero+0x968,hand_object);
    image.put(0x31000+0x18,1);image.put(0x31000+0x30,hand_object);
    image.put(0x31100+0x18,0);image.put(0x31100+0x30,0);
    std::array<ubc::PackedInventoryTarget,3> targets{{
      {profile::item_for(static_cast<std::uint32_t>(recipe.input))->function->name,recipe.input},
      {profile::item_for(static_cast<std::uint32_t>(recipe.output))->function->name,recipe.output},
      {profile::item_for(16613)->function->name,16613}}};
    std::sort(targets.begin(),targets.end(),[](const auto& a,const auto& b){return std::strcmp(a.name,b.name)<0;});
    image.block(sorted,static_cast<std::size_t>(ubc::lhiver::symbol_count)*4);
    image.put(ubc::lhiver::game_parser+0x24,sorted);
    for(std::uint32_t i=0;i<targets.size();++i)image.put(sorted+i*4,static_cast<std::uint32_t>(targets[i].symbol));
  }
  void item(std::uint32_t object,std::int32_t id,std::int32_t amount){
    image.block(object,0x348);image.put(object,ubc::hand_detail::item_vtable);image.put(object+4,1);
    image.put(object+0x330,static_cast<std::uint32_t>(id));image.put(object+0x32c,static_cast<std::uint32_t>(amount));
    const auto* shape=ubc::grind_item_shape::find(static_cast<std::uint32_t>(id));
    image.put(object+0x154,shape?shape->mainflag:1);image.put(object+0x158,shape?shape->native_flags:0x200001);
  }
  run::Input input()const{return {{key(),7,true},modules,{renderer_base,renderer_code::image_size,renderer_code::file_sha256,true}};}
  ubc::GrindRecipeCatalogueCapture catalogue(){return catalogue_reader.capture(image.memory(),ubc::lhiver::game_parser,key(),true);}
  run::Evidence capture(const g::Descriptor& d=g::recipes[0],std::uint32_t amount=2,bool preview=false){
    const auto c=catalogue();
    if(preview){const auto guild=ubc::read_grind_guild_for_preview(image.memory(),ubc::lhiver::game_parser,key());
      return reader.capture_preview(image.memory(),c,guild,d.function.symbol,amount,input());}
    return reader.capture_writer(image.memory(),c,d.function.symbol,amount,input());
  }
  void free_pool(unsigned count){
    image.put(n::node_pool+12,count?0x50000:0);image.put(n::node_pool+16,count);
    for(unsigned i=0;i<count;++i){const auto node=0x50000+i*12;image.block(node,12);
      image.put(node,i+1<count?node+12:0);image.put(node+4,i?node-12:n::node_pool+12);}
  }
};
void debug(const run::Evidence& e){if(!e.item_path_verified)std::fprintf(stderr,
  "grind runtime issue=%u path=%u binding=%u guild=%u ctor=%u module=%u native=%u address=%x symbol=%u\n",
  static_cast<unsigned>(e.issue),static_cast<unsigned>(e.stock.issue),static_cast<unsigned>(e.stock.input.issue),static_cast<unsigned>(e.physical.issue),
  static_cast<unsigned>(e.constructors.issue),static_cast<unsigned>(e.modules.issue),static_cast<unsigned>(e.modules.code.issue),e.failed_address,e.failed_symbol);}
const ubc::GrindRecipeCatalogueCapture* nested_catalogue=nullptr;
run::Input nested_runtime_input{};
void nested_reader(Image& image) noexcept {
  const auto before=image.reads;
  check(reader.capture_writer(image.memory(),*nested_catalogue,g::recipes[0].function.symbol,2,nested_runtime_input).issue==run::Issue::reentry&&image.reads==before,
        "verschachtelter Runtimeaufruf nimmt keinen zweiten Speicherstand auf");
}
namespace vi=ubc::vob_inventory_insert_compat;
// The existing parser dispatch fixture already occupies 0x63000000.
constexpr std::uint32_t locator_base=0x6c000000,insert_record=0x64000000;
vi::Binding install_locator(GrindFixture& f,const PE& locator){
  locator.load(f.image,locator_base);
  f.image.put(vi::slot_address,locator_base+vi::wrapper_rva);
  f.image.put(locator_base+vi::record_rva,insert_record);
  f.image.put(locator_base+vi::resolver_import_rva,f.modules.union_core.base+vi::resolver.rva);
  f.image.block(insert_record,28);f.image.put(insert_record+0xc,0);
  f.image.put(insert_record+0x14,vi::original_target);f.image.put(insert_record+0x18,locator_base+vi::wrapper_rva);
  return {{locator_base,vi::locator_profile.image_size,vi::locator_profile.file_sha256,true},f.modules.union_core};
}
struct LateInsertChange {
  Image& image;unsigned reads=0;bool record_change=false;
  static bool copy(void* raw,std::uint32_t address,void* out,std::size_t count) noexcept {
    auto& s=*static_cast<LateInsertChange*>(raw);
    // Four slot copies belong to the early full code check. Only the
    // late check gets a DIFFERENT route, also valid in isolation.
    if(address==vi::slot_address&&++s.reads==5){
      if(s.record_change)s.image.put(locator_base+vi::record_rva,insert_record+64);
      else s.image.put(vi::slot_address,vi::original_target);
    }
    return Image::read(&s.image,address,out,count);
  }
};
struct LateUnpackChange {
  Image& image;std::uint32_t zbug;unsigned reads=0;bool route_change=false;
  static bool copy(void* raw,std::uint32_t address,void* out,std::size_t count) noexcept {
    auto& s=*static_cast<LateUnpackChange*>(raw);
    // Eight 9AA0 copies in the early code check. The first late code pass
    // consistently sees the other identity, which is valid in isolation.
    if(address==s.zbug+0x9aa0&&++s.reads==9){
      if(s.route_change)s.image.put(s.zbug+ubc::grind_zbug_unpack::thunk_rva+11,ubc::grind_zbug_unpack::original_target);
      else s.image.put(hero+0x724,0x40010);
    }
    return Image::read(&s.image,address,out,count);
  }
};
void files(const char* exe_file,const char* archive_file,const char* zbug_file,const char* core_file,const char* crt_file,const char* zpe_file,const char* renderer_file,const char* locator_file=nullptr){
  const PE exe(exe_file,ubc::station_abi::executable_sha256),zbug(zbug_file,ubc::zbug_end_compat::zbug_file_sha256),core(core_file,ubc::zbug_end_compat::union_file_sha256);
  const PE crt(crt_file,run::dep::crt_sha256),zpe(zpe_file,run::dep::parser_extender_sha256),renderer(renderer_file,renderer_code::file_sha256);const Dat dat(archive_file);
  namespace unpack=ubc::grind_zbug_unpack;
  for(const auto quantity:{1u,5u})for(const auto preview:{false,true})for(const auto pointer:{false,true}){
    GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);
    f.image.put(f.modules.zbug.base+unpack::thunk_rva+11,f.modules.zbug.base+unpack::wrapper_rva);
    f.image.put(hero+0x724,pointer?0x40000:0);
    const auto e=f.capture(g::recipes[0],quantity,preview);debug(e);
    check(e.item_path_verified&&e.unpack.valid&&e.unpack.route==unpack::Route::empty_zbug&&e.unpack.nul_read==pointer&&
      run::matches(e,key(),g::recipes[0].function.symbol,quantity,preview?run::path::Scope::preview:run::path::Scope::writer),
      "Grind bindet echte23B-Alternative und ganzen leeren HERO-Packstring fuer Preview/Writer/Split/Ganzstapel");
    check(!n::verify_zbug_hasitems(f.image.memory(),f.modules).verified&&
      !n::verify_routed_code(f.image.memory(),f.modules).verified&&
      !n::verify_routed_code(f.image.memory(),f.modules,{}).verified,
      "alle bisherigen Bow-Einstiege behalten den strikten Originalthunk");
    auto missing=e;missing.unpack={};check(!run::matches(missing,key(),g::recipes[0].function.symbol,quantity,e.scope),
      "fehlender Leerpfad-Teilbeleg kann nicht als kompletter Grindbefund auftreten");
  }
  for(const auto route_change:{false,true}){GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);
    f.image.put(f.modules.zbug.base+unpack::thunk_rva+11,f.modules.zbug.base+unpack::wrapper_rva);
    f.image.put(hero+0x724,0x40000);text(f.image,0x40010,"");const auto c=f.catalogue();
    LateUnpackChange changing{f.image,f.modules.zbug.base,0,route_change};
    const auto e=reader.capture_writer({&changing,LateUnpackChange::copy},c,g::recipes[0].function.symbol,2,f.input());
    check(changing.reads>=16&&!e.item_path_verified&&e.issue==run::Issue::changed,
      "verschiedene einzeln gueltige fruehe/spaete Leerpointer oder MOV-Routen werden abgewiesen");
  }
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);
    f.image.put(f.modules.zbug.base+unpack::thunk_rva+11,f.modules.zbug.base+unpack::wrapper_rva);
    const std::string foreign="ITMI_GOLD:1";text(f.image,0x40000,foreign);
    f.image.put(hero+0x724,0x40000);f.image.put(hero+0x728,static_cast<std::uint32_t>(foreign.size()));
    const auto e=f.capture();check(!e.item_path_verified,"auch nur fremder gepackter Bestand sperrt die alternative Unpackroute");}
  if(locator_file){const PE locator(locator_file,vi::locator_profile.file_sha256);
    for(const auto quantity:{1u,5u})for(const auto preview:{false,true})for(const auto patched_unpack:{false,true}){
      GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);auto in=f.input();in.inventory_insert=install_locator(f,locator);
      if(patched_unpack)f.image.put(f.modules.zbug.base+unpack::thunk_rva+11,f.modules.zbug.base+unpack::wrapper_rva);
      const auto c=f.catalogue();const auto guild=ubc::read_grind_guild_for_preview(f.image.memory(),ubc::lhiver::game_parser,key());
      const auto e=preview?reader.capture_preview(f.image.memory(),c,guild,g::recipes[0].function.symbol,quantity,in):
        reader.capture_writer(f.image.memory(),c,g::recipes[0].function.symbol,quantity,in);
      debug(e);
      check(e.item_path_verified&&e.inventory_insert.valid&&e.inventory_insert.route==vi::Route::vob_direct&&
        run::matches(e,key(),g::recipes[0].function.symbol,quantity,preview?run::path::Scope::preview:run::path::Scope::writer),
        "vollstaendiger Grindreader bindet Locator und beide23B-Routen fuer Preview/Writer sowie Split/Ganzstapel");
    }
    for(unsigned fault=0;fault<4;++fault){GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);
      auto in=f.input();in.inventory_insert=install_locator(f,locator);const auto c=f.catalogue();
      switch(fault){case 0:in.inventory_insert.locator.mapping_verified=false;break;
        case 1:in.inventory_insert.union_core.base+=0x10000;break;
        case 2:in.inventory_insert.locator.base=in.renderer.base;break;
        case 3:f.image.put(insert_record+0xc,insert_record+64);break;}
      check(!reader.capture_writer(f.image.memory(),c,g::recipes[0].function.symbol,2,in).item_path_verified,
        "fehlende Lease, fremde Unionbindung, Alias oder unbekannte Insertkette sperren Grind");
    }
    for(const auto record_change:{false,true}){GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);
      auto in=f.input();in.inventory_insert=install_locator(f,locator);const auto c=f.catalogue();
      f.image.block(insert_record+64,28);std::memcpy(f.image.at(insert_record+64,28),f.image.at(insert_record,28),28);
      LateInsertChange changing{f.image,0,record_change};
      const auto e=reader.capture_writer({&changing,LateInsertChange::copy},c,g::recipes[0].function.symbol,2,in);
      check(changing.reads>=8&&!e.item_path_verified&&e.issue==run::Issue::changed,
        "zwei einzeln gueltige aber verschiedene fruehe/spaete Insertrouten bilden keinen Grindbeleg");
    }
  }
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);const auto e=f.capture();debug(e);
    if(!e.item_path_verified)std::fprintf(stderr,"dispatch issue=%u stable=%d allocator=%d wrapper=%d version=%d item=%d\n",
      static_cast<unsigned>(e.modules.dispatch.issue),e.modules.dispatch.read_stable,e.modules.dispatch.allocator_boundary_verified,
      e.modules.dispatch.wrapper_dispatch_verified,e.modules.dispatch.engine_version_verified,e.modules.dispatch.item.known);
    check(e.item_path_verified,"echte Originalcode-/Module-/World-/Factorybelege ergeben positiven Schleifpfad");}
  for(std::size_t index=0;index<g::recipes.size();++index)for(const auto quantity:{1u,5u}){
    const auto& recipe=g::recipes[index];const bool output=(index%2)==0,cold=(index%3)==0;
    GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer,recipe,5,output,cold);
    f.free_pool(quantity==1?6:0);
    const auto e=f.capture(recipe,quantity);debug(e);
    check(e.item_path_verified&&e.modules.observed&&e.constructors.values_verified&&e.renderer.verified&&e.sort.stable&&e.world.stable&&e.pool.verified,
          "alle22 Originalprofile binden komplette vorhandene Native- und Worldroute fuer Split und Ganzstapel");
    check(e.expected_after_callback==std::array<std::int32_t,3>{{static_cast<std::int32_t>(5-quantity),static_cast<std::int32_t>((output?3:0)+quantity),cold?3:1}}&&
          e.new_output_node==std::array<bool,2>{{!output,!cold}}&&e.pool.required==2u+unsigned{!output}+unsigned{!cold},
          "genau N Eingangswaffen, N Ausgabewaffen und eine Kaltklinge; realer Poolbedarf unabhaengig von N");
    check(e.pool.one_growth==(quantity==5)&&run::matches(e,key(),recipe.function.symbol,quantity,ubc::grind_item_path::Scope::writer),
          "Wiederverwendung und gepruefte Union-CRT-Wachstumsroute sind getrennt positiv");
    check(!e.native_effect_authorized&&!g::runtime_authorized,"positiver Itempfad erfindet keinen Normal-End-/Writerticketbeleg");
    if(index==0&&quantity==1)for(unsigned field=0;field<11;++field){auto mixed=e;
      switch(field){case 0:mixed.modules.observed=false;break;case 1:mixed.constructors.values_verified=false;break;
        case 2:mixed.pool.verified=false;break;case 3:mixed.world.stable=false;break;case 4:mixed.sort.stable=false;break;
        case 5:mixed.renderer.verified=false;break;case 6:++mixed.expected_after_callback[2];break;
        case 7:++mixed.physical.key.frame;break;case 8:mixed.writer.issue=ubc::GrindGuildIssue::actor;break;
        case 9:++mixed.pool.required;break;case 10:mixed.inventory_insert.valid=false;break;}
      check(!run::matches(mixed,key(),recipe.function.symbol,quantity,ubc::grind_item_path::Scope::writer),
            "auseinandergerissene Modul-/World-/Delta-/Gildenbefunde sind kein Writerbeleg");}
    if(quantity==5)check(e.world.current_whole_refcount_safe[0]&&!e.stock.existing_whole_short_path,
                        "Ref1 wird durch wirkliche zwei neue Weltlistenreferenzen positiv geschlossen");
    if(index>=4&&index<=6)check((e.stock.input.item.flags&0x1000u)!=0,"ITEM_MISSION bleibt auch im kompletten Nativepfad erhalten");
  }
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);f.image.put(symbol(1679)+0x1c,0x5000);
    const auto c=f.catalogue();const auto guild=ubc::read_grind_guild_for_preview(f.image.memory(),ubc::lhiver::game_parser,key());
    const auto preview=reader.capture_preview(f.image.memory(),c,guild,g::recipes[0].function.symbol,2,f.input());debug(preview);
    const auto writer=reader.capture_writer(f.image.memory(),c,g::recipes[0].function.symbol,2,f.input());
    check(preview.item_path_verified&&preview.scope==ubc::grind_item_path::Scope::preview&&!preview.writer.known()&&
          !writer.item_path_verified&&writer.issue==run::Issue::guild,"physische Vorschau bleibt bei fremdem SELF positiv, Writer gesperrt");
    check(run::matches(preview,key(),g::recipes[0].function.symbol,2,ubc::grind_item_path::Scope::preview)&&
          !run::matches(preview,key(),g::recipes[0].function.symbol,2,ubc::grind_item_path::Scope::writer),
          "Previewwert kann nicht als Writer-SELF-Witness umgedeutet werden");
    auto changed=key();++changed.frame;
    check(!run::matches(preview,changed,g::recipes[0].function.symbol,2,ubc::grind_item_path::Scope::preview)&&
          !run::matches(preview,key(),g::recipes[1].function.symbol,2,ubc::grind_item_path::Scope::preview)&&
          !run::matches(preview,key(),g::recipes[0].function.symbol,1,ubc::grind_item_path::Scope::preview),
          "Frame Callback und Menge bleiben konkrete Bindungen");
    f.image.put(ubc::station_abi::hero_pointer,hero+4);
    check(!reader.capture_preview(f.image.memory(),c,guild,g::recipes[0].function.symbol,2,f.input()).item_path_verified,
          "alter physischer Gildenbefund ersetzt keinen aktuellen HERO");}
  // One catalog result may serve two consumers of the same frame snapshot.
  // The selected N reader still rereads EVERYTHING and compares against
  // this exact copy. Never relabel N1 evidence as quantity evidence.
  for(unsigned mode=0;mode<7;++mode){
    GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);const auto shared=f.catalogue();
    const auto physical=ubc::read_grind_guild_for_preview(f.image.memory(),ubc::lhiver::game_parser,key());
    const auto first=reader.capture_preview(f.image.memory(),shared,physical,g::recipes[0].function.symbol,1,f.input());
    check(first.item_path_verified,"shared catalogue begins with actual positive N1 preview");
    auto in=f.input();
    switch(mode){
      case 1:f.image.put(input_object+0x32c,4);break;
      case 2:*f.image.at(0x6e8200,1)^=1;break;
      case 3:f.image.put(0x31000+0x30,input_object);break;
      case 4:++in.boundary.key.frame;break;
      case 5:in.modules.zbug.mapping_verified=false;break;
      case 6:f.image.put(hero+0x728,1);break;
      default:break;
    }
    const auto reads=f.image.reads;
    const auto selected=reader.capture_preview(f.image.memory(),shared,physical,g::recipes[0].function.symbol,3,in);
    if(!mode)check(selected.item_path_verified&&f.image.reads>reads&&
      run::matches(selected,key(),g::recipes[0].function.symbol,3,ubc::grind_item_path::Scope::preview)&&
      !run::matches(first,key(),g::recipes[0].function.symbol,3,ubc::grind_item_path::Scope::preview),
      "chosen N3 receives a new complete proof even though the catalogue copy is shared");
    else check(!selected.item_path_verified,
      "changed stock native code hands frame module or pack state cannot hide behind shared catalogue");
  }
  for(unsigned mode=0;mode<28;++mode){
    GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer,g::recipes[0],5,true,true);
    switch(mode){
      case 0:*f.image.at(renderer_base+0x7ec10,1)^=1;break;
      case 1:f.image.put(renderer_device,renderer_base+0xcc7e0);break;
      case 2:f.image.put(hero+0x710,ubc::bow_item_path::sorter+1);break;
      case 3:f.image.put(symbol(170)+0x18,0x6e7b61);break;
      case 4:f.image.put(input_object+0x158,0x40004002);break;
      case 5:f.image.put(hero+0x968,input_object);break;
      case 6:f.image.put(0x8b7464,1585);break;
      case 7:f.image.put(runtime_game+8,world+4);break;
      case 8:f.image.put(0x83c804+0x78,0x711421);break;
      case 9:f.image.put(0x82e30c,f.modules.union_core.base+0x35451);break;
      case 10:f.image.put(runtime_game+0x28,1);break;
      case 11:f.image.put(0x83e174+0x34,0x7800c1);break;
      case 12:f.image.put(0x19000,0x82f108);break;
      case 13:f.image.put(0xab1168+0x3c,0);break;
      case 14:*f.image.at(0x6e8200,1)^=1;break;
      case 15:*f.image.at(0x730800,1)^=1;break;
      case 16:f.image.put(0x8a2558,1);break;
      case 17:f.image.put(0x8d8780,1);break;
      case 18:f.image.put(0x70000,0x77ef51);break;
      case 19:f.image.put(0x8b6be0+4,0);break;
      case 20:f.image.put(0xab11e4,0);break;
      case 21:f.image.put(output_object+0x158,0x40004002);break;
      case 22:f.image.put(cold_object+0x158,0x40000000u|0x200001u);break;
      case 23:f.image.put(output_object+0x32c,INT32_MAX);break;
      case 24:f.image.put(cold_object+0x32c,INT32_MAX);break;
      case 25:f.image.put(output_object+0x154,4);break;
      case 26:f.image.put(symbol(1605)+0x1c,0);break;
      case 27:f.image.put(symbol(1606)+0x1c,0);break;
    }
    const auto e=f.capture();if(e.item_path_verified)std::fprintf(stderr,"unexpected grind native positive mode=%u\n",mode);
    check(!e.item_path_verified,"veraenderte Native-/Module-/Factory-/Sort-/Constructor-/Ausgabefakten werden sichtbar gesperrt");
    if(mode==21||mode==22||mode==25)check(e.issue==run::Issue::outputs,"aktive oder falsche Ausgabeform ist derzeit ausdruecklich nicht abgedeckt");
    if(mode==23||mode==24)check(e.issue==run::Issue::overflow,"beide Ausgabemengen sind int32-begrenzt");
  }
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);const auto c=f.catalogue();auto in=f.input();in.modules.zbug.mapping_verified=false;
    check(!reader.capture_writer(f.image.memory(),c,g::recipes[0].function.symbol,2,in).item_path_verified,"fehlende echte Modulherkunft bleibt gesperrt");
    in=f.input();in.renderer.base=in.modules.zbug.base;
    check(!reader.capture_writer(f.image.memory(),c,g::recipes[0].function.symbol,2,in).item_path_verified,"ueberlappende Modulbereiche werden nicht normalisiert");
    in=f.input();in.boundary.current_thread=8;
    check(reader.capture_writer(f.image.memory(),c,g::recipes[0].function.symbol,2,in).issue==run::Issue::thread,"Reader bleibt an den Ownerthread gebunden");
    nested_catalogue=&c;nested_runtime_input=f.input();f.image.on_read=nested_reader;
    check(reader.capture_writer(f.image.memory(),c,g::recipes[0].function.symbol,2,f.input()).item_path_verified,"aeussere Aufnahme uebersteht abgewehrten Wiedereintritt");}
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer,g::recipes[0],5,true,true);
    f.image.change_address=renderer_base+0xea6a0;f.image.change_target=output_object+0x32c;f.image.change_on_read=1;
    check(!f.capture().item_path_verified,"Ausgabebestand wechselt zwischen erster Itemaufnahme und Nativebeleg nicht unbemerkt");}
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);f.image.put(hero+0x68c,1);
    check(f.capture(g::recipes[0],5).issue==run::Issue::whole_stack,"Ganzentnahme erfordert tatsaechlich gueltigen verbleibenden Auswahlindex");}
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);
    f.image.block(0x72000,12);f.image.block(0x72100,12);f.image.put(0x70008,0x72000);f.image.put(0x71008,0x72100);
    f.image.put(0x72004,input_object);f.image.put(0x72104,input_object);
    check(f.capture(g::recipes[0],5).issue==run::Issue::whole_stack,"Ref1 schon in beiden Weltlisten bekommt keine erfundenen Zusatzreferenzen");
    f.image.put(input_object+4,2);check(f.capture(g::recipes[0],5).item_path_verified,"realer Ref2 traegt denselben Ganzentzug ohne weitere Weltreferenz");}
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);f.free_pool(6);f.image.put(0x50004,0);
    check(f.capture().issue==run::Issue::pool,"ungueltiger freier Poolknoten ist keine Allokationszusage");}
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);const auto* item=profile::item_for(static_cast<std::uint32_t>(g::recipes[0].output));
    const std::string packed=std::string(item->function->name)+":3";text(f.image,0x40000,packed);f.image.put(hero+0x724,0x40000);f.image.put(hero+0x728,static_cast<std::uint32_t>(packed.size()));
    const auto e=f.capture();
    check(!e.item_path_verified&&(e.issue==run::Issue::outputs||e.issue==run::Issue::inventory),
          "gepackte Ausgabe oder ihre Eingangs-Suchnamenskollision bleibt sichtbar gesperrt");}
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);const auto* visual=ubc::grind_item_constructor::visual(static_cast<std::uint32_t>(g::recipes[0].output));
    const auto literal=0x60000000u+visual->symbol*0x200u;
    *f.image.at(literal+32,1)^=1;check(!f.capture().item_path_verified,"gehashtes tatsächliches .3DS-Visual wird nicht aus einem Namen geraten");}
}
}
#ifndef UBC_GRIND_ITEM_RUNTIME_FIXTURE_MAIN
#define UBC_GRIND_ITEM_RUNTIME_FIXTURE_MAIN main
#endif
int UBC_GRIND_ITEM_RUNTIME_FIXTURE_MAIN(int argc,char** argv){
  using namespace grind_runtime_test;
  Image none;run::Input input;ubc::GrindRecipeCatalogueCapture c;
  check(!reader.capture_writer(none.memory(),c,g::recipes[0].function.symbol,2,input).item_path_verified,
        "leere Laufzeitwerte erzeugen keine Itemfreigabe");
  check(!run::Evidence::native_effect_authorized&&!g::runtime_authorized,"positiver Teilbeleg ist kein CallFunc-/Endticket");
  if((argc==9||argc==10)&&std::string(argv[1])=="--local-files")grind_runtime_test::files(argv[2],argv[3],argv[4],argv[5],argv[6],argv[7],argv[8],argc==10?argv[9]:nullptr);
  else if(argc!=1)return 2;
  std::printf("grind_item_runtime_tests: %u checks, %u failures\n",station_test::checks,station_test::failures);
  return station_test::failures?1:0;
}
