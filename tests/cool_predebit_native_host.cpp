// SPDX-License-Identifier: MIT
#include "cool_predebit_native_host.hpp"
#include "ubc/ui/native_crafting_ui.hpp"
#include "ubc/engine/windows_destroy_remove_callsite.hpp"
#include "ubc/engine/windows_module_identity.hpp"

// The host supplies the environment (file images, private hook registry,
// focus). Selection, dispatch reader, INFO ownership, CallFunc and writer
// are plugin functions. _ReturnAddress remains the compiler intrinsic.
namespace ubc::cool_host_windows {
using namespace windows_probe;
ReadOnlyMemory image;
bool copy_memory(void*,std::uint32_t address,void* out,std::size_t size) noexcept {
  return image.copy(address,out,size);
}
class LoadedModuleIdentity {
public:
  alchemy_extra_dependencies::Module module{};
  bool tested=true;
  static bool executable_page(std::uint32_t base,std::uint32_t target) noexcept {return target-base>=0x1000;}
  bool observed() const noexcept {return module.mapping_verified;}
  bool current() const noexcept {return tested&&observed();}
  std::uint32_t base() const noexcept {return module.base;}
  std::uint32_t image_size() const noexcept {return module.image_size;}
  lhiver::Digest digest() const noexcept {return module.file_sha256;}
  bool acquire(const wchar_t*,std::uint32_t,const lhiver::Digest&,bool=false) noexcept {return false;}
};
template<class Site,std::size_t N> class Hooks:public windows_probe::Hooks<Site,N> {
public:
  using windows_probe::Hooks<Site,N>::Hooks;
  void* original(std::size_t index) const noexcept {
    if(N==1&&index==0)return reinterpret_cast<void*>(cool_host::destroy_trampoline);
    if(N!=1&&index==8)return reinterpret_cast<void*>(cool_host::end_trampoline);
    return nullptr;
  }
};
}
namespace ubc::cool_host_remove {
class Adapter:public windows_destroy_remove_callsite::Adapter {
public:
  bool owns_patch() const noexcept {return cool_host::remove_patch().valid;}
  destroy_remove_callsite::Plan patch_plan() const noexcept {return cool_host::remove_patch();}
};
}
namespace ubc {
class CoolHostUi:public NativeCraftingUi {
public:
  NativeCraftingSuccess notice{};
  std::uint64_t submitted=0;
  SelectionInput notice_input;
  inline static bool foreground=true;
  bool top=true,lose_focus_on_render=false;
  static bool focused() noexcept {return foreground;}
  static bool buttons_held(bool* known=nullptr) noexcept {if(known)*known=true;return false;}
  bool success_notice() const noexcept {return notice.receipt!=0;}
  bool owns_input() const noexcept {return top&&success_notice();}
  void observe_notice(std::uint64_t frame,bool held,bool focus) noexcept {notice_input.observe(frame,held,focus);}
  void dismiss_notice() noexcept {
    if(!notice_dismissed()){NativeCraftingUi::dismiss_notice();notice_input.disarm();}
  }
  bool notice_return_ready() const noexcept {return success_notice()&&notice_dismissed()&&notice_input.armed();}
  void close() noexcept {NativeCraftingUi::close();notice={};submitted=0;notice_input.reset();}
  bool render_success() noexcept {
    crafting_ui::Frame output;
    submitted=crafting_ui::submit_success(output,notice,1280,720,ubc::ui_text::de::table,[](const auto& frame){
      bool quantity=false,product=false;
      for(std::size_t i=0;i<frame.count;++i){
        quantity|=std::strstr(frame.lines[i].text.data(),"2 Klingen abgeschlossen")!=nullptr;
        product|=std::strstr(frame.lines[i].text.data(),"Klinge: +2")!=nullptr;
      }
      return quantity&&product;
    });
    if(lose_focus_on_render)foreground=false;
    return submitted!=0;
  }
  std::uint64_t success_receipt() const noexcept {return submitted;}
};
}
#define windows_probe cool_host_windows
#define windows_destroy_remove_callsite cool_host_remove
#define NativeCraftingUi CoolHostUi
#include "crafting_native_fixture.hpp"
#undef NativeCraftingUi
#undef windows_destroy_remove_callsite
#undef windows_probe

namespace cool_host {
namespace {
ubc::windows_destroy_remove_callsite::Remove host_remove=nullptr;
// Physical engine call in the host. Both native observers independently
// read their stocks and hands at the four actual callback boundaries.
__declspec(noinline) void* __fastcall remove_call(void* inventory,void* table,int symbol,int amount){
  const auto loaded=reinterpret_cast<std::uint32_t>(table);
  std::uint32_t target=0;if(!read(loaded+rcall::slot,target))return nullptr;
  const rcall::Invocation input{reinterpret_cast<std::uint32_t>(inventory),loaded,target,
    reinterpret_cast<std::uint32_t>(_ReturnAddress()),GetCurrentThreadId(),symbol,amount};
  if(!rcall::observable(input,{rcall::return_address,rcall::vtable,rcall::target,GetCurrentThreadId()}))return nullptr;
  const auto token=cool_remove_before(nullptr,input);auto* item=host_remove(inventory,symbol,amount);
  cool_remove_after(nullptr,input,token,reinterpret_cast<std::uint32_t>(item));return item;
}
}
rcall::Plan remove_patch(){return rcall::plan(rcall::address,reinterpret_cast<std::uint32_t>(&remove_call));}
State state(){return {handoff,rows,result_guard,info_lease_owner,cool_predebit_tracker,cool_predebit_executor,amount_patch};}
Entries entries(){return {observe_info,observe_resolve,reinterpret_cast<std::uint32_t>(&observe_destroy),reinterpret_cast<std::uint32_t>(&observe_end)};}
bool bind(const bridge::Config& io,const xdep::Modules& modules,const Originals& originals,bc::Writer writer){
  installed=1;engine_thread=static_cast<LONG>(GetCurrentThreadId());
  generation=1;frame=1;frame_depth=1;native_update=false;update_depth=0;
  cool_predebit_executor.reset();
  cool_predebit_tracker.~Tracker();new(&cool_predebit_tracker) debit::Tracker;
  amount_patch.~AmountPatch();new(&amount_patch) ubc::AmountPatch;
  session={};handoff={};rows={};result_guard={};dispatched_ticket={};
  cool_job=effect_executed=disabled=recover_info=false;cool_ready=cool_destroy_installed=true;
  cool_serial=cool_job_floor=end_sequence=0;
  ubc::cool_host_windows::image=io.memory;
  cool_predebit_engine.module={0x400000,0x752000,s::executable_sha256,true};
  end_zbug_module.module=modules.zbug;end_union_module.module=modules.union_core;
  end_zbug_module.tested=modules.zbug.file_sha256==ubc::zbug_end_compat::zbug_file_sha256;
  end_union_module.tested=modules.union_core.file_sha256==ubc::zbug_end_compat::union_file_sha256;
  extra_zpe_module.module=modules.parser_extender;extra_crt_module.module=modules.runtime_crt;
  destroy_dispatch_reader.emplace(GetCurrentThreadId());cool_predebit_reader.emplace(GetCurrentThreadId());
  auto config=io;
  if(!config.boundary)config.boundary=cool_predebit_boundary;
  cool_predebit_executor.emplace(cool_predebit_tracker,config);
  original_info=originals.info;original_resolve=originals.resolve;
  original_destroy=originals.destroy;original_end=originals.end;host_remove=originals.remove_item;
  success_pending=false;pending_success={};ui.close();ui.foreground=ui.top=true;ui.lose_focus_on_render=false;
  UbcBatchCallOriginal=reinterpret_cast<void*>(originals.script);
  bc::Config callbacks;
  callbacks.parser=reinterpret_cast<void*>(s::game_parser);callbacks.symbols[0]=20775;callbacks.symbols[1]=20778;
  callbacks.symbol_count=2;callbacks.signature=batch_signature;callbacks.select=select_batch_call;
  callbacks.reject_unexpected=reject_unexpected_cool_info;callbacks.writer=writer;
  return bc::configure(callbacks)&&cool_predebit_executor->diagnostics().configured;
}
cpath::Verification preview_path(){return cool_runtime_path(true);}
bool dispatch(std::uint64_t dispatch_frame){
  const auto c=context();
  if(!c.valid||!session.open(capture(c).snapshot))return false;
  session.observe(frame,false,true);session.observe(++frame,false,true);
  if(!session.adjust(1)||session.quantity()!=2)return false;
  observe_cool_quantity(c);
  if(!session.can_confirm()||!session.confirm())return false;
  for(unsigned i=0;i<3;++i){session.observe(++frame,false,true);observe_cool_quantity(c);}
  const auto request=session.take(capture(c).snapshot,true);if(!request)return false;
  frame=dispatch_frame;
  const auto plan=ubc::make_crafting_plan(page(c),*request);auto now=observation(c,true,false);
  now.materials=request->material_snapshot;now.hands=request->hands;
  if(!handoff.begin(*request,plan,now))return false;
  handed_hands=request->hands;
  const auto offer=resolve_offer(c,handoff.step());now=observation(c,true);
  const auto ticket=handoff.ticket(now,offer);
  if(!ticket||!handoff.dispatch(*ticket,now,offer))return false;
  dispatched_ticket=*ticket;
  return ticket->offer.owner==20773&&ticket->offer.function==20775;
}
void enter_update(bool active){frame_depth=0;native_update=active;update_depth=active?1u:0u;}
void set_focus(bool focused){ui.foreground=focused;}
void set_context_diagnosis(bool enabled){cool_context_mode=enabled;}
void set_event_log(HANDLE destination){log_file=destination;}
std::uint32_t current_info(){return current_info_effect(context());}
std::uint64_t info_token(){return info_effect_lease.token;}
void* native_writer(bc::WriterScope& scope){return execute_amount(scope);}
std::uint64_t last_serial(){return cool_serial;}
const cdestroy::Observation& observed_destroy(){return cool_destroy_call;}
const cdestroy::Binding& destroy_binding(){return cool_destroy_binding;}
void next_frame(){++frame;}
bool result_frame(){frame_depth=1;observe_result(context());frame_depth=0;return success_pending;}
std::uint64_t submit_notice(bool focus,bool owns_input,bool lose_focus_on_render){
  if(!success_pending)return 0;
  ui.foreground=focus;ui.top=owns_input;ui.lose_focus_on_render=lose_focus_on_render;
  if(!ui.success_notice())ui.notice=pending_success;
  observe_result_notice(context());
  return cool_receipt_acked?ui.success_receipt():0;
}
bool next_handoff(){
  ui.dismiss_notice();
  for(unsigned i=0;i<2;++i){++frame;observe_result_notice(context());}
  if(success_pending||ui.success_notice()||!result_guard.permits_next())return false;
  ++generation;frame_depth=1;
  return dispatch(frame+7);
}
}
