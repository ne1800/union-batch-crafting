// SPDX-License-Identifier: MIT
// Actual local binary contracts using only our own hosts and RAM objects.
#if !defined(UBC_COOL_PREDEBIT_HOST_TEST)
#error Nur der isolierte Host darf die synthetischen Nativeziele verwenden.
#endif
#define UBC_COOL_PREDEBIT_READER_MAIN predebit_reader_fixture_main
#include "cool_predebit_reader_tests.cpp"
#include "cool_predebit_bridge.hpp"
#include "ubc/cool_predebit_projection.hpp"
#include "ubc/cool_predebit_journal.hpp"
#include "ubc/cool_destroy_observation.hpp"
#include "ubc/cool_native_path.hpp"
#include "ubc/cool_listed_input.hpp"
#include "ubc/crafting_cool_projection.hpp"
#include "ubc/crafting_info_effect.hpp"
#include "ubc/native_crafting_ui.hpp"
#include <intrin.h>
#include <memory>
#include "cool_predebit_host_calls.hpp"
#include "ubc/station_end_contract.hpp"

namespace bridge_test {
namespace bridge=ubc::cool_predebit_bridge;
namespace batch=ubc::batch_call;
constexpr DWORD incoming=0x12345678,removed=0x23456789,original_error=0x3456789a;
constexpr DWORD native_fault=0xe042bb01,reader_fault=0xe042bb02,restore_fault=0xe042bb03;
enum class Mode {normal,null_return,item_noop,wrong_info,missing_prepared,expired_info,replaced_info,foreign_owner,
  changed_parser,changed_item,changed_cold,wrong_token,wrong_stack,wrong_frame,wrong_patch,
  wrong_job,wrong_scope,stale,queue,early_destroy,reader_seh,reader_reentry,patch_destroy,patch_info,patch_reentry,
  gate_parser,gate_hot1,gate_hot0,gate_hot3,gate_entry,gate_cold,
  remove_before_seh,remove_after_seh,combined_seh,no_debit,cold_delta,prefix_delta,
  pop_seh,instance_seh,item_seh,pop_noop,instance_noop,item_noop_fault,
  restore_reader_seh,restore_reentry,restore_target,restore_queue,after_pop_queue,after_pop_trampoline,after_instance_queue,after_pop_lease,
  after_scope,after_reader_seh,original_gate,original_gate_close,original_hook,original_lease,original_seh,amount_restore,
  after_prefix,after_cold,after_hand,after_production,after_invincible,after_smithing,after_lease,
  after_destroy,original_reentry,after_reader_seh_final,wrong_binding,foreign,missing_visual,missing_anictrl,zero_refs,
  foreign_info,foreign_outer,foreign_inner,swapped_callback,late_visual,late_anictrl,
  before_remove_visual,before_remove_anictrl,before_remove_focus,after_remove_anictrl,missing_message,
  stock_two_six,post_split_pool,post_split_focus,untested_plugins,bad_final_stock,union_object_created,foreign_jump_target,foreign_function_tail,
  admission_stack,admission_queue,patched_queue,tracker_and_operand_restore,remove_seh_and_queue,context_field,context_read_only,context_all_match,deferred_writer,deferred_preview};
Mode mode=Mode::normal;
void expect(bool condition,const char* label){
  if(!condition)std::fprintf(stderr,"mode=%u ",static_cast<unsigned>(mode));
  check(condition,label);
}
void clobber(){SetLastError(999);unsigned csr=0x5f80;__asm {fninit} __asm {fldz} __asm {ldmxcsr csr}}
struct Host;
Host* active=nullptr;
struct Host {
  PreFixture fixture;
  cool_host::State native=cool_host::state();
  pre::Tracker& tracker=native.tracker;
  std::optional<bridge::Executor>& executor=native.executor;
  pre::admission::InfoLease& info_lease=native.lease;
  pre::admission::InfoLease foreign_info_lease;
  ubc::CraftingHandoff& handoff=native.handoff;
  ubc::CraftingHandoffTicket ticket;
  ubc::SelectionInfoRows& rows=native.rows;
  pre::admission::Prepared prepared;
  bridge::Request request{};ubc::AmountMemory writable{};
  ubc::cool_native_path::OwnedPatches hooks{true,true,cool_host::entries().destroy,
    cool_host::destroy_trampoline,cool_host::remove_patch()};
  pre::DestroyJournal journal;ubc::CraftingResult& result=native.result;
  ubc::CraftingResult before_witness;
  batch::WriterScope scope{};ubc::CraftingRequest craft{};ubc::CraftingResultBinding binding{};
  unsigned boundaries=0,removes=0,pops=0,instances=0,items=0,originals=0,forward_stores=0,writer_calls=0,info_originals=0;
  std::uint64_t serial=0;
  bool changed_before_remove=false;
  void* script_value=nullptr;
  static constexpr std::uint32_t body=PreFixture::body_address(),alternate_info=0x26c00,message=0x26800;
  static constexpr auto body_size=ubc::cool_recipe_catalog::single.information->byte_length;
  bool production()const{return mode<=Mode::item_noop||mode>=Mode::missing_visual;}
  static bool read(void* raw,std::uint32_t at,void* dest,std::size_t size)noexcept{
    auto& h=*static_cast<Host*>(raw);
    if(mode==Mode::post_split_pool && h.removes && at==n::node_pool)return false;
    if(mode==Mode::patched_queue&&h.native.patch.active()&&at==n::node_pool)
      h.fixture.image.put(event_manager+0x38,1);
    if(mode==Mode::tracker_and_operand_restore&&h.native.patch.active()){
      if(at==n::node_pool)h.fixture.image.put(event_manager+0x28,99); // Fingerprint differs from the one at admission.
      if(h.tracker.status().blocked&&at==body+62&&size==4)return false; // Only when restoring the operand
    }
    // The pool read occurs during the last composite read between the
    // two dispatch checks. The quantity operand is patched at this point.
    if((mode==Mode::before_remove_visual||mode==Mode::before_remove_anictrl||mode==Mode::before_remove_focus)&&!h.changed_before_remove&&
       !h.removes&&at==n::node_pool&&h.tracker.status().stage==pre::Stage::calling_extra){
      h.changed_before_remove=true;
      if(mode==Mode::before_remove_focus)cool_host::set_focus(false);
      else h.fixture.image.put(mode==Mode::before_remove_visual?hero+0xc8:0x270004,0);
    }
    if(!h.fixture.image.memory().copy(at,dest,size))return false;
    if(h.production()){
      // An overlapping read also sees the same physical quantity operand.
      const auto first=std::max(at,body);
      const auto end=std::min<std::uint64_t>(std::uint64_t{at}+size,body+body_size);
      if(first<end)std::memcpy(static_cast<std::uint8_t*>(dest)+(first-at),
        reinterpret_cast<const void*>(first),static_cast<std::size_t>(end-first));
    }
    return true;
  }
  void install_fixture_hook(std::uint32_t entry,std::uint32_t destination,std::uint32_t trampoline){
    auto& image=fixture.image;image.block(trampoline,11);
    auto* original=image.at(entry,6);auto* copy=image.at(trampoline,11);
    std::memcpy(copy,original,6);
    ubc::cool_native_path::detail::jump(copy+6,trampoline+6,entry+6);
    std::fill_n(original,6,0x90);ubc::cool_native_path::detail::jump(original,entry,destination);
  }
  Host(const PE& exe,const Dat& dat,const PE& zbug,const PE& core,const PE& crt,const PE& zpe,const PE& renderer)
    :fixture(exe,dat,zbug,core,crt,zpe,renderer){
    namespace dispatch=ubc::cool_destroy_dispatch_profile;
    namespace end=ubc::station_end_contract;
    auto& image=fixture.image;
    if(mode>=Mode::stock_two_six){
      image.put(cold_object+0x32c,6);
      *image.at(0x8cdcf0,1)=0xff; // Actual report filter measured at 2/6
    }
    if(mode==Mode::untested_plugins||mode==Mode::bad_final_stock){
      auto& modules=fixture.modules;
      modules.union_core.file_sha256.fill(0x12);modules.zbug.file_sha256.fill(0xab);
      modules.union_core.image_size+=0x1000;modules.zbug.image_size+=0x1000;
      auto redirect=[&](unsigned at,unsigned op,unsigned to){
        *image.at(at,1)=static_cast<std::uint8_t>(op);image.put(at+1,to-at-5);
      };
      redirect(0x7d5940,0xe9,modules.union_core.base+0x1234);
      for(const auto at:{0x7d0be9u,0x70d0e6u,0x74adb6u,0x721e53u})
        redirect(at,0xe8,modules.zbug.base+0x2345);
      image.put(modules.runtime_crt.base+0xb45f4,1);
      image.put(modules.runtime_crt.base+0xb4460,reinterpret_cast<unsigned>(EncodePointer(nullptr)));
      *image.at(station_test::name_at(16612)-1,1)=0xff;
    }
    if(mode==Mode::union_object_created||mode==Mode::foreign_jump_target||mode==Mode::foreign_function_tail){
      // "Allow undeclarated instances" from the G2A section of the installed Union.patch.
      const std::uint8_t jump[]{0xe9,0x99,0,0,0,0x90};
      auto* site=image.at(0x5aaf1d,sizeof(jump));
      std::memcpy(site,jump,sizeof(jump));
      if(mode==Mode::foreign_jump_target)site[1]^=1;
      if(mode==Mode::foreign_function_tail)*image.at(0x5aafcc,1)^=1;
    }
    install_fixture_hook(ubc::cool_destroy_abi::destroy_entry,hooks.destination,hooks.trampoline);
    install_fixture_hook(end::end_address,cool_host::entries().end,cool_host::end_trampoline);
    std::memcpy(image.at(hooks.remove.call,6),hooks.remove.bytes.data(),6);
    image.put(0x8800,end::mob_vtables.front());
    image.put(hero,dispatch::npc_vtable);
    image.block(0x270000,8);image.put(hero+0xc8,0x260000);image.put(hero+0x97c,0x270000);image.put(0x270004,1);
    image.put(hero+0x980,0x270000);image.block(message,0x44);
    image.put(message,dispatch::message_vtable);image.put(message+4,2);image.put(message+0x24,9);
    for(std::size_t i=0;i<dispatch::class_chain.size();++i)
      image.put(dispatch::class_chain[i]+0x3c,i+1<dispatch::class_chain.size()?dispatch::class_chain[i+1]:0u);
    for(const auto& entry:ubc::recipe_catalog::common_bindings)
      if(!u32(image.at(station_test::table+4*entry.symbol,4)))fixture.binding(entry);
    expect(fixture.patch.finish(),"Fixture beendet ihr eigenes Startjournal");
    auto q=fixture.input();request={q.job,q.frame,&native.patch};request.job.context.thread=GetCurrentThreadId();
    request.job.id=request.job.context.epoch;request.job.actual_info=0;request.job.info_token=0;
    if(!production())request.job.fence_token=journal.open();
    // Only the native amount writer needs an actual private RW page.
    if(production())std::memcpy(reinterpret_cast<void*>(body),image.at(body,body_size),body_size);
    const ubc::ReadOnlyMemory memory{this,&read};
    writable={memory,this,&PreFixture::identity,&PreFixture::writable,&replace};
    const bridge::Config config{GetCurrentThreadId(),memory,this,production()?nullptr:&boundary,&remove,&pop,&instance,&item};
    expect(cool_host::bind(config,fixture.modules,{&info_original,&resolve_original,&original,&destroy_original,&end_original,&remove_last},&bridge_writer),
      "nativer Adapter mit privatem Speicher und gezählten Originalen eingerichtet");
  }
  static void* __thiscall resolve_original(void*,void*,void*,int row){
    return row==1?reinterpret_cast<void*>(mode==Mode::foreign_info?alternate_info:actual_info):nullptr;
  }
  static void __thiscall info_original(void* object){
    auto& h=*active;
    const auto address=reinterpret_cast<std::uint32_t>(object);
    ++h.info_originals;
    expect(cool_host::current_info()==address&&cool_host::info_token(),"observe_info besitzt sein unabhängig aufgelöstes Argument");
    if(mode==Mode::swapped_callback)h.fixture.image.put(address+0x2c,20778);
    if(mode==Mode::late_visual)h.fixture.image.put(hero+0xc8,0);
    if(mode==Mode::late_anictrl)h.fixture.image.put(0x270004,0);
    if(mode==Mode::admission_stack)h.fixture.image.put(ubc::lhiver::game_parser+0x2058,2);
    if(mode==Mode::context_field||mode==Mode::context_read_only){
      if(mode==Mode::context_read_only)h.fixture.image.put(0x8800+0x1f8,7);
      h.fixture.image.put(ubc::lhiver::game_parser+0x2198,3);
    }
    if(mode==Mode::admission_queue)h.fixture.image.put(event_manager+0x38,1);
    int symbol=0;if(!h.fixture.image.memory().copy(address+0x2c,&symbol,sizeof(symbol)))std::abort();
    h.script_value=mode==Mode::foreign_inner?h.run():CallSites::script(symbol);
  }
  static int __thiscall destroy_original(void* npc,void* msg){
    auto& h=*active;auto& image=h.fixture.image;
    expect(reinterpret_cast<std::uint32_t>(npc)==hero&&reinterpret_cast<std::uint32_t>(msg)==message,
      "Destroy erhält HERO und die geprüfte Manipulate9-Nachricht");
    image.put(hero+0x968,0);image.put(0x31000+0x30,0);
    const auto inventory=hero+ubc::cool_destroy_abi::npc_inventory_offset;
    auto* table=reinterpret_cast<void*>(u32(image.at(inventory,4)));
    return CallSites::remove(reinterpret_cast<void*>(inventory),table,16612,1)!=nullptr;
  }
  static void* __thiscall remove_last(void* inventory,int symbol,int amount){
    auto& image=active->fixture.image;
    expect(reinterpret_cast<std::uint32_t>(inventory)==hero+ubc::cool_destroy_abi::npc_inventory_offset&&
      symbol==16612&&amount==1&&u32(image.at(hot_object+0x32c,4))==1,"Destroy entfernt genau die letzte heiße Lagerklinge");
    image.put(hero+0x718,0x11010);return reinterpret_cast<void*>(hot_object);
  }
  static void __thiscall end_original(void* mob,void* npc,int flag){
    expect(reinterpret_cast<std::uint32_t>(mob)==0x8800&&reinterpret_cast<std::uint32_t>(npc)==hero&&!flag,
      "normales EndInteraction löst den Helden von der Kühlstation");
    active->fixture.image.put(hero+ubc::station_abi::npc_interact_mob,0);
  }
  void queue_destroy(){
    // The original leaves a valid successor manager on HERO.
    // +30/+34/+38 are the data field, capacity and occupancy.
    constexpr std::uint32_t manager=0x26d00,slots=0x26e00;
    auto& image=fixture.image;image.block(manager,0x40);image.block(slots,4);
    std::memcpy(image.at(manager,0x40),image.at(event_manager,0x40),0x40);
    image.put(hero+0xfc,manager);image.put(manager+0x30,slots);image.put(manager+0x34,1);
    if(mode!=Mode::missing_message){image.put(slots,message);image.put(manager+0x38,1);}
  }
  bool dispatch_message(){
    const auto m=fixture.image.memory();std::uint32_t manager=0,table=0,owner=0,count=0,slots=0,msg=0;
    if(!m.copy(hero+0xfc,&manager,4)||!m.copy(manager,&table,4)||table!=0x83e23c||
       !m.copy(manager+0x3c,&owner,4)||owner!=hero||!m.copy(manager+0x38,&count,4)||count!=1||
       !m.copy(manager+0x30,&slots,4)||!m.copy(slots,&msg,4)||!msg)return false;
    fixture.image.put(manager+0x38,0);fixture.image.put(slots,0);
    return CallSites::destroy(reinterpret_cast<void*>(owner),reinterpret_cast<void*>(msg))==1;
  }
  bool start_next_batch(){
    auto& image=fixture.image;
    image.put(hero+ubc::station_abi::npc_interact_mob,0x8800);image.put(0x8800+0x1f4,1);
    image.put(station_test::symbol(12330)+0x18,13);image.put(hero+0x298,1);
    image.put(hero+0x968,hand_object);image.put(0x31000+0x30,hand_object);
    image.put(hero+0x718,0x11000);image.put(hot_object+0x32c,2);
    collect();return cool_host::next_handoff();
  }
  void collect(){
    namespace a=ubc::selection_abi;
    constexpr std::uint32_t view=0x26100,end_info=0x26b00;
    auto& image=fixture.image;
    image.block(view,a::view_size);image.put(view,a::view_vtable);image.put(view+0x24,0x83bc10);
    image.put(view+a::view_callback,a::view_callback_vtable);image.put(view+a::view_count,2);
    image.put(view+a::view_selected,1);image.put(view+a::view_active,1);image.put(view+a::view_opened,1);
    const auto manager=request.job.information_manager;
    image.put(manager+0x1c,view);image.put(manager+0x28,end_info);image.put(manager+0x40,1);
    image.block(end_info,0x5c);image.put(end_info,ubc::info_detail::info_vtable);image.put(end_info+0x1c,22243);
    image.put(end_info+0x20,999);image.put(end_info+0x28,20771);image.put(end_info+0x2c,20772);
    image.put(end_info+0x48,1);image.put(end_info+0x50,20770);
    rows.begin(view,true);
    rows.append(view,ubc::read_selection_info(image.memory(),end_info),0,1,true);
    rows.append(view,ubc::read_selection_info(image.memory(),actual_info),1,2,true);
  }
  std::uint32_t current_info(){return cool_host::current_info();}
  bool open_handoff(){
    collect();
    if(mode==Mode::missing_visual)fixture.image.put(hero+0xc8,0);
    if(mode==Mode::missing_anictrl)fixture.image.put(hero+0x97c,0);
    if(mode==Mode::zero_refs)fixture.image.put(0x270004,0);
    const auto path=cool_host::preview_path();
    if(mode==Mode::normal&&!path.valid)std::fprintf(stderr,"Kühlpfad: Issue %u bei %08x, Dispatch %u\n",
      unsigned(path.issue),path.address,unsigned(path.dispatch.issue));
    if(mode>=Mode::missing_visual&&mode<=Mode::zero_refs){
      const auto field=mode==Mode::missing_visual?hero+0xc8:mode==Mode::missing_anictrl?hero+0x97c:0x270004;
      expect(!path.valid&&path.dispatch.failed_address==field,"Destroy-Reader nennt das fehlende HERO-/AniCtrl-Feld");
    }else expect(path.valid&&path.dispatch.values.visual==0x260000&&path.dispatch.values.anictrl==0x270000&&
      path.dispatch.values.anictrl_refs==1,"cool_runtime_path liest HERO-Visual und referenzierten AniCtrl");
    if(!cool_host::dispatch(request.frame))return false;
    ticket=*handoff.current_ticket();craft=handoff.request();
    binding={craft.key.generation,request.frame,ticket.offer.collection,0,craft.key.world,craft.key.hero,craft.key.station};
    if(mode==Mode::foreign_info){
      auto& image=fixture.image;image.block(alternate_info,0x5c);
      std::memcpy(image.at(alternate_info,0x5c),image.at(actual_info,0x5c),0x5c);
    }
    return !current_info()&&!cool_host::info_token();
  }
  void prepare(const batch::WriterScope& real){
    request.job.writer_token=real.token;
    if(!current_info())return;
    const auto entry=input(pr::Phase::admission);
    const pre::admission::WriterEntry writer{request.job.writer_token,request.job.context.parser,20775,
      static_cast<std::uint32_t>(real.call.caller),real.call.thread,batch::writer_undisturbed(real),real.original!=nullptr};
    auto reader=std::make_unique<pr::Reader>(GetCurrentThreadId());
    prepared=pre::admission::prepare(*reader,fixture.image.memory(),entry,info_lease,writer);
    expect(prepared.valid()&&!native.patch.active(),"echte vollständige Admission vor dem Amount-Patch");
    if(mode==Mode::wrong_binding)++binding.start_frame;
    request.prepared=&prepared;request.info_lease=&info_lease;
    if(mode==Mode::missing_prepared)request.prepared=nullptr;
    if(mode==Mode::expired_info)info_lease.invalidate();
    if(mode==Mode::replaced_info)CallSites::info(reinterpret_cast<void*>(request.job.actual_info));
    if(mode==Mode::foreign_owner)request.info_lease=&foreign_info_lease;
    if(mode==Mode::changed_parser)fixture.image.put(ubc::lhiver::game_parser+0x4c,station_test::code+1235);
    if(mode==Mode::changed_item)fixture.image.put(station_test::symbol(1682)+0x1c,0x45670000);
    if(mode==Mode::changed_cold)fixture.image.put(cold_object+0x32c,6);
  }
  pr::Input input(pr::Phase phase){
    auto q=fixture.input(phase);q.serial=++serial;q.job=request.job;q.current_thread=request.job.context.thread;
    q.scope.writer.writer_token=q.job.writer_token;q.scope.writer.job=q.job.id;
    q.scope.info_token=q.job.info_token;q.scope.actual_info=q.job.actual_info;q.stack.thread=q.current_thread;q.amount_patch=&native.patch;
    (void)journal.sample(request.job.fence_token,q.destroy_started,q.destroy_completed);
    return q;
  }
  static void* __cdecl bridge_writer(batch::WriterScope& real){
    auto& h=*active;const auto error=GetLastError();++h.writer_calls;
    expect(real.call.caller==batch::info_call_return&&batch::writer_undisturbed(real),"exklusive Lease vom ursprünglichen inneren CALL");
    h.request.job.actual_info=h.current_info();h.request.job.info_token=cool_host::info_token();
    if(mode==Mode::deferred_writer)pre::runtime_ready=false;
    h.scope=real;void* value=nullptr;
    __try {
      if(h.production()){
        SetLastError(error);value=cool_host::native_writer(real);
      }else{
        h.prepare(real);real.restore_verified=true;SetLastError(error);
        value=h.executor->run(real,h.native.patch,h.writable,h.request,h.result,h.craft,h.binding);
      }
    } __finally {h.scope=real;}
    return value;
  }
  std::uint32_t operand(){std::uint32_t value=0;if(!read(this,body+62,&value,4))std::abort();return value;}
  void* run(){
    using Invoke=void*(__cdecl*)(void*,int,...);
    return reinterpret_cast<Invoke>(&UbcBatchCallBridge)(reinterpret_cast<void*>(request.job.context.parser),ticket.offer.function);
  }
  static bool __cdecl boundary(void* raw,pr::Phase phase,pr::Input& q){
    auto& h=*static_cast<Host*>(raw);++h.boundaries;clobber();q=h.input(phase);
    const auto actual=h.executor->diagnostics();
    expect(actual.active&&actual.native_scope==(pr::reads_saved_call(phase)||phase==pr::Phase::after_extra),
      "Entzugstiefe und lebende INFO-Lease sind getrennte Besitzer");
    expect(h.result.state()==ubc::CraftingResultState::waiting,"Resultguard wartet bereits vor dem ersten Extra-Read");
    if(phase==pr::Phase::before_extra){
      if(h.boundaries==1)h.before_witness=h.result;
      if(mode==Mode::wrong_info)q.job.actual_info+=4;
      if(mode==Mode::wrong_token)++q.scope.info_token;
      if(mode==Mode::wrong_stack)h.fixture.image.put(ubc::lhiver::game_parser+0x2058,2);
      if(mode==Mode::wrong_frame)++q.frame;
      if(mode==Mode::wrong_patch)q.amount_patch=nullptr;
      if(mode==Mode::wrong_job)++q.job.id;
      if(mode==Mode::wrong_scope)q.scope.writer.vm_depth=1;
      if(mode==Mode::stale&&h.boundaries==2)--q.serial;
      if(mode==Mode::queue)h.fixture.image.put(event_manager+0x38,1);
      if(mode==Mode::early_destroy){h.journal.entered();h.journal.exited();}
      if(mode==Mode::reader_seh)RaiseException(reader_fault,0,0,nullptr);
      if(mode==Mode::reader_reentry)(void)h.run();
    }
    if(pr::reads_saved_call(phase)){
      if(!h.removes&&mode==Mode::gate_parser)h.fixture.image.put(ubc::lhiver::game_parser+0x4c,station_test::code+1235);
      if(!h.removes){
        if(mode==Mode::gate_entry)*h.fixture.image.at(ubc::cool_destroy_abi::destroy_entry,1)=0x64;
        if(mode==Mode::gate_hot1)h.fixture.image.put(hot_object+0x32c,1);
        if(mode==Mode::gate_hot0)h.fixture.image.put(hot_object+0x32c,0);
        if(mode==Mode::gate_hot3)h.fixture.image.put(hot_object+0x32c,3);
        if(mode==Mode::gate_cold)h.fixture.image.put(cold_object+0x32c,6);
      }
      if(h.removes){
        if(mode==Mode::after_pop_trampoline&&h.pops)*h.fixture.image.at(h.hooks.trampoline,1)=0;
        if(mode==Mode::after_pop_lease&&h.pops)h.info_lease.invalidate();
        if(mode==Mode::restore_reader_seh)RaiseException(reader_fault,0,0,nullptr);
        if(mode==Mode::restore_reentry)(void)h.run();
        if(mode==Mode::restore_target)h.fixture.image.put(station_test::table+1682*4,station_test::symbol(1683));
        if(mode==Mode::restore_queue||(mode==Mode::after_pop_queue&&h.pops)||(mode==Mode::after_instance_queue&&h.instances))
          h.fixture.image.put(event_manager+0x38,1);
      }
    }
    if(phase==pr::Phase::after_extra){
      if(mode==Mode::after_scope)++q.scope.info_token;
      if(mode==Mode::after_reader_seh)RaiseException(reader_fault,0,0,nullptr);
    }
    if(phase==pr::Phase::before_original){
      if(mode==Mode::original_gate)++q.scope.info_token;
      if(mode==Mode::original_gate_close){h.journal.entered();h.journal.exited();}
      if(mode==Mode::original_lease)h.info_lease.invalidate();
      if(mode==Mode::original_hook)*h.fixture.image.at(h.hooks.remove.call,1)=0xff;
    }
    if(phase==pr::Phase::after_original){
      expect(!h.native.patch.active(),"AFTER liest erst nach positivem Amount-finally");
      if(mode==Mode::after_reader_seh_final)RaiseException(reader_fault,0,0,nullptr);
    }
    return h.current_info()&&ubc::cool_native_path::installed_hooks_current(h.fixture.image.memory(),h.hooks);
  }
  static bool replace(void* raw,std::uint32_t at,const void* old,const void* value,std::size_t length)noexcept{
    auto& h=*static_cast<Host*>(raw);
    expect(h.result.state()==ubc::CraftingResultState::waiting,"Resultguard wartet bei jedem echten Operandentausch");
    if(length!=4)return false;
    const bool restoring=u32(static_cast<const std::uint8_t*>(value))==1;
    if(!restoring){
      if(mode==Mode::patch_destroy)h.journal.entered();
      if(mode==Mode::patch_info)h.info_lease.invalidate();
      if(mode==Mode::patch_reentry)(void)h.run();
    }
    if(!bridge::permits_operand_replace(h.result.cool_prepared(),h.info_lease,&h.scope,h.journal,h.request.job.fence_token,
        at,old,value,length,[&h](){return h.current_info();}))return false;
    if(restoring){
      clobber(); // Restoring bytes must not overwrite the FX output of the original.
      if(mode==Mode::amount_restore&&h.originals)return false;
    }else ++h.forward_stores;
    return PreFixture::replace(&h.fixture,at,old,value,length);
  }
  static void* __thiscall remove(void* npc,int symbol,int quantity){
    auto& h=*active;++h.removes;
    if(h.production())h.before_witness=h.result;
    expect(reinterpret_cast<std::uintptr_t>(npc)==h.request.job.context.hero&&symbol==16612&&quantity==1,"genau ein gebundener Extra1-Aufruf");
    expect(GetLastError()==incoming&&h.executor->diagnostics().native_scope,"LastError und Entzugstiefe am ABI-Eintritt");
    SetLastError(removed);
    if(mode==Mode::remove_seh_and_queue){
      h.fixture.image.put(event_manager+0x38,1);
      RaiseException(native_fault,0,0,nullptr);
    }
    if(mode==Mode::remove_before_seh)RaiseException(native_fault,0,0,nullptr);
    h.fixture.split_values();
    if(mode==Mode::post_split_focus)cool_host::set_focus(false);
    if(mode==Mode::after_remove_anictrl)h.fixture.image.put(0x270004,0);
    if(mode==Mode::no_debit)h.fixture.image.put(hot_object+0x32c,2);
    if(mode==Mode::cold_delta)h.fixture.image.put(cold_object+0x32c,6);
    if(mode==Mode::prefix_delta)h.fixture.image.put(ubc::lhiver::game_parser+0x2058,2);
    if(mode==Mode::item_noop)h.fixture.image.put(station_test::symbol(1682)+0x1c,h.tracker.call().item.value);
    unsigned csr=0x3f80;__asm {fninit} __asm {fld1} __asm {ldmxcsr csr}
    if(mode==Mode::remove_after_seh||mode==Mode::combined_seh)RaiseException(native_fault,0,0,nullptr);
    return mode==Mode::null_return?nullptr:reinterpret_cast<void*>(0xdeadc0de);
  }
  static void __thiscall pop(void* stack,int value){
    auto& h=*active;++h.pops;clobber();
    const auto& saved=h.tracker.call().parser;
    expect(reinterpret_cast<std::uintptr_t>(stack)==ubc::lhiver::game_parser+0x48&&static_cast<std::uint32_t>(value)==saved.pop_pos,"Popadresse und Cursor gebunden");
    if(mode==Mode::pop_seh)RaiseException(restore_fault,0,0,nullptr);
    if(mode!=Mode::pop_noop)h.fixture.image.put(ubc::lhiver::game_parser+0x4c,saved.code_base+saved.pop_pos);
  }
  static void __cdecl instance(void* symbol){
    auto& h=*active;++h.instances;clobber();const auto& saved=h.tracker.call().parser;
    expect(reinterpret_cast<std::uintptr_t>(symbol)==saved.use_symbol,"SetUseInstance auf frisch gebundenem Symbol");
    if(mode==Mode::instance_seh)RaiseException(restore_fault,0,0,nullptr);
    if(mode!=Mode::instance_noop){h.fixture.image.put(ubc::station_abi::instance_symbol_pointer,saved.use_symbol);
      h.fixture.image.put(ubc::station_abi::instance_address_pointer,saved.use_address);}
  }
  static void __thiscall item(void* symbol,int value){
    auto& h=*active;++h.items;clobber();const auto& saved=h.tracker.call().item;
    expect(reinterpret_cast<std::uintptr_t>(symbol)==saved.symbol_object&&static_cast<std::uint32_t>(value)==saved.value,"ITEM-Ziel vor Setter erneut geprüft");
    if(mode==Mode::item_seh||mode==Mode::combined_seh)RaiseException(restore_fault,0,0,nullptr);
    if(mode!=Mode::item_noop_fault)h.fixture.image.put(saved.symbol_object+0x1c,saved.value);
  }
  static void* __cdecl original(void* parser,int symbol){
    auto& h=*active;++h.originals;
    expect(parser==h.scope.call.parser&&symbol==20775&&GetLastError()==removed,"exakter CallFunc-ABI nach Extra");
    expect(h.tracker.status().stage==pre::Stage::calling_original&&h.result.state()==ubc::CraftingResultState::waiting,
      "frisches Originalgate und wartender Resultguard");
    expect(h.operand()==2,"Amount2 bleibt bis Original aktiv");
    SetLastError(original_error);
    if(mode==Mode::original_seh){
      __try {RaiseException(native_fault,0,0,nullptr);}
      __finally {unsigned csr=0x1f80;__asm {fninit} __asm {fld1} __asm {ldmxcsr csr}}
    }
    // Deliberately leave a different parser state afterward. The executor
    // must not destroy it by restoring the pre-original state after the fact.
    h.fixture.original_values(mode==Mode::null_return?0:1);
    if(h.production())h.queue_destroy();
    if(mode==Mode::after_prefix)h.fixture.image.put(ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack+4,0);
    if(mode==Mode::after_cold)h.fixture.image.put(cold_object+0x32c,6);
    if(mode==Mode::after_production)h.fixture.image.put(station_test::symbol(12330)+0x18,13);
    if(mode==Mode::after_smithing)h.fixture.image.put(station_test::symbol(10536)+0x18,0);
    if(mode==Mode::after_invincible)h.fixture.image.put(h.request.job.context.hero+0x298,1);
    if(mode==Mode::after_hand)h.fixture.image.put(hand_object+0x158,0x200001);
    if(mode==Mode::after_lease)h.info_lease.invalidate();
    if(mode==Mode::after_destroy){h.journal.entered();h.journal.exited();}
    if(mode==Mode::original_reentry)(void)h.run();
    unsigned csr=0x1f80;__asm {fninit} __asm {fld1} __asm {ldmxcsr csr}
    return reinterpret_cast<void*>(0xbaadf00d);
  }
};
DWORD WINAPI foreign(void* raw){
  auto& h=*static_cast<Host*>(raw);
  return h.executor->run(h.scope,h.native.patch,h.writable,h.request,h.result,h.craft,h.binding)?1u:0u;
}
DWORD WINAPI ordinary_call(void* raw){
  const auto value=static_cast<Host*>(raw)->run();
  return value&&value!=reinterpret_cast<void*>(0xbaadf00d)&&*static_cast<const int*>(value)==0?0u:1u;
}
bool invoke(Host& h,DWORD& exception,void*& value){
  bool normal=false;
  cool_host::enter_update(true);
  __try {
    const auto d=ubc::read_selection_dialog(h.fixture.image.memory(),h.request.job.information_manager);
    auto* resolved=CallSites::resolve(reinterpret_cast<void*>(h.request.job.information_manager),
      reinterpret_cast<void*>(h.request.job.context.hero),d.selected);
    if(mode<=Mode::item_noop)expect(resolved==reinterpret_cast<void*>(actual_info)&&d.info!=actual_info,
      "der Resolver liefert die gewählte Zeile unabhängig von der Manager-INFO");
    if(resolved){
      if(mode==Mode::foreign_outer)cool_host::entries().info(resolved,nullptr);
      else CallSites::info(resolved);
    }
    value=h.script_value;normal=true;
  }
  __except(EXCEPTION_EXECUTE_HANDLER){exception=GetExceptionCode();}
  const auto error=GetLastError();cool_host::enter_update(false);SetLastError(error);return normal;
}
void finish_result(Host& h){
  namespace destroy=ubc::cool_destroy_observation;
  using State=ubc::CraftingResultState;
  const auto& prepared=h.result.cool_prepared();const auto& witness=h.result.cool_witness();
  const auto& r=h.craft;const auto k=prepared.key();const auto frame=h.request.frame+1;
  cool_host::next_frame();
  const auto pending=h.result;
  expect(h.dispatch_message()&&!h.dispatch_message(),"die ausstehende Nachricht löst genau einen normalen Destroy aus");
  const auto& observed=cool_host::observed_destroy();const auto& binding=cool_host::destroy_binding();
  const auto proof=destroy::verify(prepared,witness,binding,observed);
  if(!proof.valid)std::fprintf(stderr,"Destroy-Vertrag: Issue %u, Dispatch %u, Hand %u, Lager %u\n",
    unsigned(proof.issue),observed.destroy.message_class_verified,observed.generated_left_verified,observed.before_destroy.stocks.stable_boundary_verified);
  expect(proof.valid&&proof.serial==cool_host::last_serial(),"vier native Lesegrenzen nach Original; Hot1 zu Hot0, Cold7");
  const auto& empty=observed.after_destroy.hands;
  const ubc::CraftingResultDestroyWitness removal{true,true,r.key,r.id,2,proof.serial,frame,16612,1};
  const ubc::CraftingResultEndWitness end{true,true,binding.job,1,frame,k.world,k.hero,k.station};
  const std::array<ubc::RecipeResultItem,2> totals{{{16612,0},{16613,7}}};
  ubc::CraftingResultObservation quiet;quiet.generation=binding.job;quiet.frame=frame+1;quiet.world=k.world;quiet.hero=k.hero;
  quiet.stable_boundary_verified=quiet.identity_known=quiet.station_known=quiet.production_known=true;
  quiet.stock_known=quiet.hands_known=true;quiet.hands=empty;quiet.items={totals.data(),totals.size()};
  for(unsigned fault=0;fault<9;++fault){
    auto copy=witness;auto actual=observed;auto bounds=binding;
    switch(fault){
      case 0:++copy.job.writer_token;break;case 1:++copy.frame;break;case 2:++copy.amount.before_original[0].amount;break;
      case 3:bounds.serial_floor=witness.original_serial-1;break;case 4:actual.before_remove.stocks.stocks[0].amount=2;break;
      case 5:++actual.after_destroy.stocks.stocks[1].amount;break;case 6:actual.before_destroy.hands.hands.left.object+=4;break;
      case 7:++copy.extra_debit;break;case 8:actual.remove.normal_return=false;break;
    }
    expect(!destroy::verify(prepared,copy,bounds,actual).valid,"fremder Zeuge oder falsche Destroy-/Remove-Grenze abgewiesen");
  }
  {
    auto missing=h.before_witness;
    expect(missing.state()==State::waiting&&!missing.destroy_after(removal)&&missing.state()==State::failed,
      "Destroy vor dem Vorentzugzeugen gesperrt");
    auto bypass=h.before_witness;
    expect(bypass.state()==State::waiting&&!bypass.restored(true),"allgemeines Restore darf H2/N2 nicht abschliessen");
    ubc::CraftingResult expired;
    expect(!expired.begin_cool_predebit(r,h.binding,prepared,h.info_lease),"nach dem Callback kein Neuerwerb des Resultguards");
    auto duplicate=pending;expect(!duplicate.cool_predebit_restored(witness,witness.frame),"doppelter Vorentzugzeuge gesperrt");
    auto early=pending;auto stale=removal;stale.serial=witness.original_serial;
    expect(!early.destroy_after(stale)&&early.state()==State::failed,"alte Destroyserial nicht uebertragbar");
    auto no_destroy=pending;expect(!no_destroy.end_after(end),"normales Ende vor Destroy abgewiesen");
    auto no_end=pending;expect(no_end.destroy_after(removal)&&no_end.observe(quiet)==State::waiting,"Bilanz ohne normales Ende wartet");
  }
  h.fixture.image.put(0x8800+0x1f4,0);
  CallSites::end(reinterpret_cast<void*>(k.station),reinterpret_cast<void*>(k.hero));
  expect(!cool_host::result_frame()&&h.result.state()==State::waiting,"im Endframe bleibt die Bilanz offen");
  cool_host::next_frame();
  if(mode==Mode::bad_final_stock){
    h.fixture.image.put(cold_object+0x32c,7);
    expect(!cool_host::result_frame()&&h.result.state()==State::failed&&!h.result.permits_next()&&
      !cool_host::submit_notice(),"Abweichung 0/7 nach normalem Ende sperrt Quittung und Folgeauftrag");
    cool_host::next_frame();h.fixture.image.put(cold_object+0x32c,8);
    expect(!cool_host::result_frame()&&!h.result.permits_next()&&h.removes==1&&h.originals==1,
      "ein später passender Bestand entsperrt keine unklare Bilanz");
    return;
  }
  expect(cool_host::result_frame()&&h.result.state()==State::verified,"späterer Ruheframe liest das private Inventar und schließt den Handoff");
  ubc::CraftingResultSuccess success;
  expect(h.result.success(success)&&success.produced_count==1&&success.produced[0].symbol==16613&&success.produced[0].amount==2&&
    success.quantity_receipt_required&&!h.result.permits_next(),"exakte Menge 2 bleibt bis zur eigenen Textquittung gesperrt");
  const auto notice=ubc::crafting_ui::success_notice(success);
  ubc::crafting_ui::Frame rendered;
  const auto failed=ubc::crafting_ui::submit_success(rendered,notice,1280,720,ubc::ui_text::de::table,[](const auto&){return false;});
  expect(!failed&&!h.result.acknowledge_quantity_receipt(failed)&&!h.result.acknowledge_quantity_receipt(success.receipt+1)&&
    !h.result.permits_next(),"fehlende oder fremde Ausgabe liefert keine Quittung");
  const bool lost_during_render=mode==Mode::null_return;
  expect(!cool_host::submit_notice(lost_during_render||mode==Mode::item_noop,mode!=Mode::item_noop,lost_during_render)&&
    !h.result.permits_next(),"vor oder während der Ausgabe wartet dieselbe Quittung bei Fokus- oder Eingabeverlust");
  cool_host::next_frame();
  const auto submitted=cool_host::submit_notice();
  expect(submitted&&submitted==success.receipt&&!h.result.acknowledge_quantity_receipt(submitted)&&
    h.result.permits_next(),"der native Adapter quittiert die Ausgabe für zwei Klingen genau einmal");
  expect(h.start_next_batch()&&h.handoff.current_ticket()->offer.function==20775&&h.originals==1,
    "nach Rückkehr und Quittung erhält der nächste Auftrag ein eigenes Ticket");
}
// The production path writes the actual JSON lines when leaving INFO.
void check_rejection_log(Host& h){
  char dir[MAX_PATH]{},file[MAX_PATH]{};
  if(!GetTempPathA(MAX_PATH,dir)||!GetTempFileNameA(dir,"ubc",0,file)){expect(false,"temporäres Hostlog");return;}
  HANDLE log_handle=CreateFileA(file,GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,
    FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
  if(log_handle==INVALID_HANDLE_VALUE){DeleteFileA(file);expect(false,"Hostlog geöffnet");return;}
  cool_host::set_context_diagnosis(mode==Mode::context_read_only||mode==Mode::context_all_match);
  cool_host::set_event_log(log_handle);
  DWORD exception=0;void* value=nullptr;SetLastError(incoming);
  const bool normal=invoke(h,exception,value);
  cool_host::set_event_log(INVALID_HANDLE_VALUE);cool_host::set_context_diagnosis(false);
  SetFilePointer(log_handle,0,nullptr,FILE_BEGIN);
  std::string log;char bytes[2048];DWORD count=0;
  while(ReadFile(log_handle,bytes,sizeof(bytes),&count,nullptr)&&count)log.append(bytes,count);
  CloseHandle(log_handle);
  const bool untested=mode==Mode::untested_plugins||mode==Mode::bad_final_stock;
  const bool post_split=mode==Mode::post_split_pool||mode==Mode::post_split_focus;
  const bool control=untested||post_split||mode==Mode::stock_two_six||mode==Mode::union_object_created,seh=mode==Mode::remove_seh_and_queue;
  const bool operand_failed=mode==Mode::tracker_and_operand_restore;
  const auto hot=u32(h.fixture.image.at(hot_object+0x32c,4)),cold=u32(h.fixture.image.at(cold_object+0x32c,4));
  const auto operand=u32(reinterpret_cast<const std::uint8_t*>(Host::body+62));
  expect(normal==!seh&&exception==(seh?native_fault:0u)&&h.writer_calls==1&&h.scope.call.symbol==20775&&
    h.removes==unsigned(control||seh)&&h.originals==unsigned(control)&&operand==(operand_failed?2u:1u)&&
    hot==(control?1u:2u)&&cold==(control?8u:6u)&&h.result.cool_witness().valid==control,
    "20775 bei 2/6: Aufrufe, Ausnahme, Operand und Inventar entsprechen dem eingespeisten Fehler");
  const char* label=mode==Mode::post_split_pool?"Pool nach Entzug":mode==Mode::post_split_focus?"Fokus nach Entzug":
    mode==Mode::stock_two_six?"Kontrolle":mode==Mode::union_object_created?"ObjectCreated/Union":
    mode==Mode::foreign_jump_target?"ObjectCreated/Sprungziel":mode==Mode::foreign_function_tail?"ObjectCreated/Funktionsende":
    mode==Mode::admission_stack?"Admission/Parser":
    mode==Mode::admission_queue?"Admission/Queue":operand_failed?"Tracker/Operand-Restore":
    seh?"Remove-SEH/Queue":"vor Entzug/Queue";
  std::printf("20775 %s, Hot=%u Cold=%u Remove=%u Original=%u\n%s",label,hot,cold,h.removes,h.originals,log.c_str());
  const std::string rejected="\"event\":\"amount_rejected\"";
  if(mode==Mode::deferred_writer){
    expect(!h.removes&&!h.originals&&log.find("\"event\":\"amount_rejected\",\"frame\":"+std::to_string(h.request.frame)+
      ",\"detail\":8,\"quantity\":0}")!=std::string::npos,"alter Auftrag erreicht bei abgeschaltetem Vorentzug keine Herstellung");
    return;
  }
  if(untested){
    const auto logged=[&](const char* name,const std::string& hash){
      const auto line=log.find(name);if(line==std::string::npos)return false;
      const auto plugin=log.substr(line,log.find('\n',line)-line);
      return plugin.find(hash)!=std::string::npos&&plugin.find("ungetestete Version")!=std::string::npos;
    };
    std::string union_hash,zbug_hash;
    for(unsigned i=0;i<32;++i){union_hash+="12";zbug_hash+="ab";}
    expect(logged("cool_union_version",union_hash)&&logged("cool_zbug_version",zbug_hash),
      "beide unbekannten DLLs mit vollständigem Dateihash und Versionsvermerk protokolliert");
  }
  if(untested||post_split)finish_result(h);
  if(control){expect(log.find(rejected)==std::string::npos,"Erfolgsfall ohne Ablehnungsdiagnose");return;}
  const auto frame=std::to_string(h.request.frame);
  const auto event=[&](const std::string& name,int detail,int quantity){
    return log.find("\"event\":\""+name+"\",\"frame\":"+frame+",\"detail\":"+std::to_string(detail)+
      ",\"quantity\":"+std::to_string(quantity)+"}")!=std::string::npos;
  };
  expect(event("amount_rejected",8,0)&&log.find(rejected)==log.rfind(rejected),"Vorentzugsfehler 8 bleibt einmalig im DLL-Log");
  if(mode==Mode::context_read_only||mode==Mode::context_all_match){
    const bool changed=mode==Mode::context_read_only;
    for(const auto phase:pre::inspection::phases){
      const auto token="\"phase\":"+std::to_string(static_cast<int>(phase))+",\"field\":\"queue.count\"";
      expect(log.find(token)!=std::string::npos,"Queue in jeder tatsächlichen oder projizierten Phase gelesen");
    }
    if(!changed){
      const auto bad=log.find("\"quantity\":0,\"phase\":");
      if(bad!=std::string::npos)std::fprintf(stderr,"Erste abweichende Hostprüfung: %.160s\n",log.c_str()+bad);
      expect(bad==std::string::npos,"vollständiger positiver Aufruf bis zur Originalgrenze lesend projiziert");
    }
    expect(log.find("\"event\":\"cool_checks_complete\"")!=std::string::npos&&
      log.find(changed?"\"field\":\"station.count\",\"soll\":\"1\",\"ist\":\"7\"":
                       "\"field\":\"station.count\",\"soll\":\"1\",\"ist\":\"1\"")!=std::string::npos&&
      log.find(changed?"\"field\":\"parser.debug\",\"soll\":\"0\",\"ist\":\"3\"":
                       "\"field\":\"parser.debug\",\"soll\":\"0\",\"ist\":\"0\"")!=std::string::npos&&
      !h.forward_stores&&!h.removes&&!h.originals,"Alle Kontextwerte bei passendem und abweichendem Kontext, null Schreibaufrufe");
  }else if(mode==Mode::context_field){
    expect(event("cool_predebit_reader_rejected",int(pr::Phase::admission),int(pr::Issue::context))&&
      log.find("\"field\":\"parser.debug\"")!=std::string::npos,"Grund 12 benennt die erste konkrete Prüfstelle");
  }else if(operand_failed||seh){
    const auto phase=seh?pr::Phase::before_remove:pr::Phase::before_extra;
    const auto issue=seh?pre::Issue::debit_exception:pre::Issue::observation;
    expect(event("cool_predebit_tracker_rejected",int(phase),int(issue))&&
      log.find("cool_predebit_reader_rejected")==std::string::npos,"Primärursache vor fehlerhafter Rückführung bleibt erhalten");
    expect(event(operand_failed?"amount_restore_failed":"cool_predebit_restore_failed",20775,2)&&
      batch::quarantined(20775),"fehlgeschlagene Rückführung separat im ursprünglichen INFO-Log und Callback gesperrt");
  }else if(mode==Mode::foreign_jump_target||mode==Mode::foreign_function_tail){
    const auto phase=int(pr::Phase::admission);
    expect(event("cool_predebit_reader_rejected",phase,int(pr::Issue::engine))&&
      event("cool_predebit_address",phase,0x5aaeb0),"fremde ObjectCreated-Fassung vor dem ersten Entzug erkannt");
  }else{
    const auto phase=mode==Mode::patched_queue?pr::Phase::before_extra:pr::Phase::admission;
    const auto issue=mode==Mode::admission_stack?pr::Issue::parser:pr::Issue::queue;
    expect(event("cool_predebit_reader_rejected",int(phase),int(issue)),"erste Readerprüfung mit Phase und Grund im DLL-Log");
    if(mode==Mode::admission_stack)
      expect(event("cool_predebit_address",int(phase),ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack_count),
        "die gemeldete Parseradresse bezeichnet die belegte Datenstacktiefe");
    expect(log.find("cool_predebit_tracker_rejected")==std::string::npos,"nachgelagerter Tracker verdeckt keinen Readerfehler");
  }
}
void one(const PE& exe,const Dat& dat,const PE& zbug,const PE& core,const PE& crt,const PE& zpe,const PE& renderer){
  // Never construct large reader workspaces on the x86 callback stack.
  auto storage=std::make_unique<Host>(exe,dat,zbug,core,crt,zpe,renderer);auto& h=*storage;active=&h;
  const bool opened=h.open_handoff();
  if(mode==Mode::deferred_preview){
    expect(!opened&&!h.writer_calls&&!h.removes&&!h.originals&&h.operand()==1,"Auslieferungszustand startet keinen Zweierauftrag");
    active=nullptr;return;
  }
  const bool bad_path=mode>=Mode::missing_visual&&mode<=Mode::zero_refs;
  expect(opened!=bad_path,"reguläre Vorschau und Handoff nur mit lesbarem Destroy-Pfad");
  if(bad_path)expect(!h.writer_calls&&!h.removes&&!h.originals,"fehlender Dispatch sperrt vor jeder Wirkung");
  if(!opened){active=nullptr;return;}
  if(mode>=Mode::stock_two_six){check_rejection_log(h);active=nullptr;return;}
  if(mode==Mode::foreign){
    const auto thread=CreateThread(nullptr,0,&foreign,&h,0,nullptr);expect(thread!=nullptr,"Fremdthread erzeugt");
    if(thread){expect(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"Fremdthread abgeschlossen");DWORD result=1;
      expect(GetExitCodeThread(thread,&result)&&!result,"Fremdthread vor Reads abgewiesen");CloseHandle(thread);}
  }
  unsigned esp_before=0,esp_after=0,seh_before=0,seh_after=0;alignas(16) unsigned char fx[512],result_fx[512];_fxsave(fx);
  __asm {mov eax,fs:[0]} __asm {mov seh_before,eax} __asm {mov esp_before,esp}
  DWORD exception=0;void* value=nullptr;SetLastError(incoming);const auto normal=invoke(h,exception,value);
  const auto error=GetLastError();_fxsave(result_fx);_fxrstor(fx);
  __asm {mov eax,fs:[0]} __asm {mov seh_after,eax} __asm {mov esp_after,esp}
  expect(!cool_host::info_token(),"observe_info schließt seine Lease im finally");
  expect(!h.current_info(),"Rückkehr aus Info schließt die Eigentümerbindung");
  if(mode==Mode::missing_message){
    cool_host::next_frame();
    expect(normal&&h.removes==1&&h.originals==1&&h.result.cool_witness().valid&&!h.dispatch_message()&&
      !cool_host::result_frame()&&!cool_host::submit_notice()&&h.result.state()==ubc::CraftingResultState::waiting&&
      !h.result.permits_next(),"ohne ausstehende Nachricht bleiben Destroy, Mengenquittung und Folgeauftrag aus");
    active=nullptr;return;
  }
  if(mode==Mode::before_remove_visual||mode==Mode::before_remove_anictrl||mode==Mode::before_remove_focus||mode==Mode::after_remove_anictrl){
    const bool removed_once=mode==Mode::after_remove_anictrl;
    expect(normal&&h.writer_calls==1&&h.changed_before_remove!=removed_once&&batch::quarantined(20775)==removed_once&&h.removes==(removed_once?1u:0u)&&
      h.pops==(removed_once?1u:0u)&&h.instances==h.pops&&h.items==h.pops&&!h.originals&&h.operand()==1&&
      h.result.state()==ubc::CraftingResultState::failed&&!h.result.cool_witness().valid,
      "später Dispatchverlust sperrt vor Remove oder erlaubt nur die Rückführung des bereits begonnenen Entzugs");
    active=nullptr;return;
  }
  if(mode>=Mode::foreign_info){
    const unsigned admitted_writer=mode==Mode::late_visual||mode==Mode::late_anictrl?1u:0u;
    expect(normal&&h.info_originals==(mode>=Mode::foreign_inner?1u:0u)&&h.writer_calls==admitted_writer&&
      !h.removes&&!h.originals&&h.operand()==1&&!h.result.cool_witness().valid,
      "Objekt, Aufrufstelle, Callback und spätes HERO-/AniCtrl-Gate sperren genau an ihrer Grenze");
    active=nullptr;return;
  }
  expect(esp_before==esp_after&&seh_before==seh_after,"x86 Stack und SEH-Kette unverändert");
  const bool fault=mode==Mode::remove_before_seh||mode==Mode::remove_after_seh||mode==Mode::combined_seh||mode==Mode::original_seh;
  expect(normal!=fault&&(!fault||exception==native_fault),"Engine-SEH bleibt trotz Restorefehler erhalten");
  const bool no_remove=(mode>=Mode::wrong_info&&mode<=Mode::gate_cold)||mode==Mode::foreign||mode==Mode::wrong_binding;
  const bool success=mode<=Mode::item_noop;
  if(mode==Mode::patch_destroy||mode==Mode::patch_info||mode==Mode::patch_reentry)
    expect(!h.forward_stores&&!h.removes&&!h.originals,"Besitzverlust am ersten Schreibzugriff lässt Originaloperand und Inventar unangetastet");
  const bool original=success||(mode>=Mode::original_seh&&mode<=Mode::after_reader_seh_final);
  expect(h.removes==(no_remove?0u:1u)&&h.originals==(original?1u:0u),"Extra/Original jeweils genau null oder einmal");
  expect(error==(original?original_error:no_remove?incoming:removed),"Reader und Restore verdecken keinen nativen LastError");
  expect(h.executor->operands_restored()==(mode!=Mode::amount_restore)&&!h.executor->diagnostics().active&&
    !h.executor->diagnostics().native_scope,"Amount-finally auf jedem Abbruchpfad und Lease geschlossen");
  expect(h.operand()==(mode==Mode::amount_restore?2u:1u),"exakter Operandrestore oder sichtbar fehlgeschlagen");
  expect(h.tracker.witness().valid==success&&h.result.cool_witness().valid==success,"nur positiver frischer AFTER erzeugt Vorentzugzeugen");
  expect(h.executor->original_entered()==original&&h.executor->original_returned()==(original&&mode!=Mode::original_seh),
    "Originaleintritt und normale Rückkehr getrennt belegt");
  if(success){
    expect(h.writer_calls==1&&h.info_originals==1&&!h.result.cool_prepared().current(h.info_lease),
      "ein nativer Writer und eine nach der INFO-Rückkehr verbrauchte Lease");
    expect(h.tracker.status().stage==pre::Stage::returned&&!h.executor->diagnostics().disabled,"positiver kurzer Vorentzug");
    expect(value==reinterpret_cast<void*>(0xbaadf00d)&&h.pops==1&&h.instances==1&&h.items==(mode==Mode::item_noop?0u:1u),"Rückgabewert und alle Parserteile");
    expect(u32(h.fixture.image.at(ubc::lhiver::game_parser+0x4c,4))==station_test::code+2345,"kein Restore über den Nachzustand von Original");

    expect(h.result.state()==ubc::CraftingResultState::waiting&&!h.result.permits_next(),"Original/AFTER allein erlauben keinen Folgejob");
    const auto removes=h.removes,originals=h.originals;
    (void)CallSites::script(h.ticket.offer.function);
    expect(h.removes==removes&&h.originals==originals,"das noch aktive Ticket erlaubt keinen zweiten Effekt");
    finish_result(h);
  }else{
    expect(h.executor->diagnostics().disabled&&h.tracker.status().blocked&&h.result.state()==ubc::CraftingResultState::failed,"Teileffekt oder Fehler bleibt gesperrt");
  }
  if(original){
    expect(*reinterpret_cast<const unsigned*>(result_fx+24)==0x1f80&&result_fx[4]==0x80,
      "FX des Originals ueber mutierenden Amount-Restore und normalen bzw. abnormalen Return erhalten");
  }
  const bool quarantined=h.removes!=0&&!success;
  expect(batch::quarantined(20775)==quarantined&&h.scope.restore_verified==!quarantined,
    "unklare Teilwirkung sperrt den echten CallFunc-Einstieg unabhaengig vom Operandennachweis");
  if(mode==Mode::pop_seh||mode==Mode::original_reentry){
    const auto before=h.originals;const auto thread=CreateThread(nullptr,0,&ordinary_call,&h,0,nullptr);DWORD answer=1;
    expect(thread&&WaitForSingleObject(thread,5000)==WAIT_OBJECT_0&&GetExitCodeThread(thread,&answer)&&!answer&&h.originals==before,
      "gewoehnlicher Fremdthreadaufruf nach unsicherem Parser-/Originalausgang bleibt vor dem Original gesperrt");
    if(thread)CloseHandle(thread);
  }
  if(mode==Mode::gate_entry||mode==Mode::gate_hot1)expect(!h.executor->extra_entered()&&!h.executor->original_entered()&&!h.originals,
    "eine Ablehnung am letzten Gate verbraucht das Ticket und lässt den Originalcallback frei");
  if(mode==Mode::after_pop_trampoline)expect(h.pops==1&&!h.instances&&!h.items&&!h.originals,"verlorener Trampolin sperrt jeden weiteren Setter");
  if(mode==Mode::after_pop_queue)expect(h.pops==1&&!h.instances&&!h.items,"Queuewechsel sperrt nächsten Setter");
  if(mode==Mode::after_instance_queue)expect(h.pops==1&&h.instances==1&&!h.items,"Queuewechsel sperrt ITEM-Setter");
  if(mode==Mode::after_pop_lease)expect(h.pops==1&&!h.instances&&!h.items,"abgelaufene INFO-Lease sperrt nächsten Setter");
  if(!success){
    const auto removes=h.removes,originals=h.originals;if(normal)(void)invoke(h,exception,value);
    expect(h.removes==removes&&h.originals==originals,"kein Retry nach Fehler");
  }
  active=nullptr;
}
}
extern "C" __declspec(dllexport) int __cdecl run_predebit_host(
    int argc,char** argv,unsigned char* storage,std::size_t size){
  if(argc!=9||std::string(argv[1])!="--local-files")return 2; // An empty host must not pass.
  bridge_test::CallSites calls;if(!calls.reserve(storage,size))return 3;
  const PE exe(argv[2],ubc::station_abi::executable_sha256),zbug(argv[4],ubc::zbug_end_compat::zbug_file_sha256),
    core(argv[5],ubc::zbug_end_compat::union_file_sha256),crt(argv[6],n::dep::crt_sha256),
    zpe(argv[7],n::dep::parser_extender_sha256),renderer(argv[8],renderer_code::file_sha256);const Dat dat(argv[3]);
  if(!calls.create(exe))return 3;
  bridge_test::mode=bridge_test::Mode::deferred_preview;
  bridge_test::one(exe,dat,zbug,core,crt,zpe,renderer);
  // Private host objects only. The game DLL has no switch.
  pre::runtime_ready=true;
  for(unsigned i=0;i<=static_cast<unsigned>(bridge_test::Mode::deferred_writer);++i){
    bridge_test::mode=static_cast<bridge_test::Mode>(i);bridge_test::one(exe,dat,zbug,core,crt,zpe,renderer);}
  pre::runtime_ready=false;
  std::printf("cool_predebit_bridge: Versandzustand und isolierter Writer, %u Fälle, %u Prüfungen, %u Fehler\n",
    static_cast<unsigned>(bridge_test::Mode::deferred_preview)+1,station_test::checks,station_test::failures);
  return station_test::failures?1:0;
}
