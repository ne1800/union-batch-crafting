// SPDX-License-Identifier: MIT
#include <windows.h>
#include <intrin.h>
#include <array>
#include <cstdio>
#include <cstring>
#include "ubc/engine/frame_bridge.hpp"
#include "ubc/engine/batch_call_bridge.hpp"
#include "ubc/core/batch_amount_call.hpp"
#include "ubc/ui/native_recipe_ui.hpp"
#include "ubc/core/recipe_snapshot.hpp"
#include "ubc/core/recipe_handoff.hpp"
#include "ubc/core/recipe_result.hpp"
#include "ubc/core/selection_snapshot.hpp"
#include "ubc/core/post_remove_choice.hpp"
#include "ubc/engine/windows_probe.hpp"

namespace {
namespace a=ubc::selection_abi;
namespace s=ubc::station_abi;
namespace m=ubc::native_menu;
namespace w=ubc::windows_probe;
namespace cat=ubc::recipe_catalog;
namespace nav=ubc::recipe_navigation;
namespace rt=ubc::runtime_script;
namespace bc=ubc::batch_call;
using Phase=ubc::SelectionPhase;
constexpr ubc::ReadOnlyMemory memory{nullptr,w::copy_memory};
constexpr ubc::lhiver::Profile layout_profile{ubc::lhiver::game_parser,ubc::lhiver::symbol_count,
  ubc::lhiver::code_length,ubc::lhiver::code_sha256,nullptr,0,nullptr,0};
HMODULE module=nullptr;
HANDLE log_file=INVALID_HANDLE_VALUE;
volatile LONG installed=0,engine_thread=0;
bool execution_enabled=false,disabled=false,native_update=false;
unsigned frame_depth=0,update_depth=0,info_depth=0,choice_depth=0,station_depth=0,collection_depth=0;
std::uint64_t frame=0,generation=0,end_sequence=0;
std::size_t log_bytes=0,log_events=0;
ubc::SelectionInfoRows rows;
bool choice_collection=false;
std::size_t choice_count=0;
std::array<std::int32_t,256> choice_functions{};
ubc::RecipeCatalogueReader catalogue_reader;
ubc::PackedInventoryWorkspace result_workspace;
ubc::RecipeSession session;
ubc::NativeRecipeUi ui;
ubc::RecipeHandoff handoff;
ubc::RecipeResult<> result_guard;
ubc::AmountPatch amount_patch;
ubc::PostRemoveChoice post_remove_choice;
ubc::HandoffTicket dispatched_ticket;
ubc::HandsSnapshot handed_hands;
std::array<ubc::RecipeResultItem,cat::catalog_target_capacity> expected_result{};
std::uint32_t borrowed_view=0,original_station=0;
bool private_enter=false,effect_authorized=false,effect_executed=false,recover_info=false;
bool failure_notified=false,failure_open_rejected_logged=false;
ULONGLONG handoff_started=0,result_started=0;
int last_frame_state=-1,last_handoff_state=-1,last_capture_issue=-1,last_failure_state=-1;
struct AmountDiagnostic {
  bool pending=false,entered=false,restored=false;
  int rejected=0,duration=0,symbol=0,quantity=0;
} amount_diagnostic;

using Start=void(__thiscall*)(void*,void*);
using End=void(__thiscall*)(void*,void*,int);
using Info=void(__thiscall*)(void*);
a::FastObject original_update=nullptr,original_choice=nullptr,original_clear=nullptr;
a::AddChoice original_add=nullptr;
a::ResolveInfo original_resolve=nullptr;
Info original_info=nullptr;
Start original_start=nullptr;
End original_end=nullptr;
m::MenuHandleEvent original_menu_event=nullptr;
a::HandleEvent original_choice_event=nullptr;
constexpr s::HookSite menu_event_hook{m::menu_handle_event,5,12,
  {{0x53,0x55,0x56,0x8b,0xf1,0x57,0x8b,0xbe,0x40,0x0c,0x00,0x00}}};
constexpr s::HookSite choice_event_hook{a::choice_handle_event,9,17,
  {{0x53,0x8b,0xd9,0x8b,0x83,0x14,0xff,0xff,0xff,0x56,0x8d,0xb3,0x14,0xff,0xff,0xff,0x57}}};
constexpr std::array<s::HookSite,12> sites{{a::frame_hook,a::add_hook,a::clear_hook,
  s::hook_sites[2],a::resolve_hook,s::hook_sites[3],s::hook_sites[4],
  s::hook_sites[0],s::hook_sites[1],menu_event_hook,
  {0x7929f0,7,27,{{0x6a,0xff,0x68,0x00,0xa9,0x82,0x00,0x64,0xa1,0x00,0x00,0x00,0x00,0x50,
    0x64,0x89,0x25,0x00,0x00,0x00,0x00,0x81,0xec,0x14,0x01,0x00,0x00}}},choice_event_hook}};
w::Hooks<s::HookSite,sites.size()> hooks(sites);

template<class T> bool read(std::uint32_t address,T& out) noexcept{return memory.copy(address,&out,sizeof(out));}
template<class T> bool field(std::uint32_t object,std::uint32_t offset,T& out) noexcept {
  return object&&offset<=UINT32_MAX-object&&read(object+offset,out);
}
std::int32_t integer(std::uint32_t object,std::uint32_t offset) noexcept {
  std::int32_t value=-1;field(object,offset,value);return value;
}
bool own_thread() noexcept {
  return InterlockedCompareExchange(&installed,0,0)&&
    InterlockedCompareExchange(&engine_thread,0,0)==static_cast<LONG>(GetCurrentThreadId());
}
void event(const char* name,int detail=0,int quantity=0) noexcept {
  if(log_file==INVALID_HANDLE_VALUE||log_events>=10000||log_bytes>=4u*1024u*1024u)return;
  char line[200]{};
  const int size=std::snprintf(line,sizeof(line),"{\"event\":\"%s\",\"frame\":%llu,\"detail\":%d,\"quantity\":%d}\r\n",
    name,static_cast<unsigned long long>(frame),detail,quantity);
  if(size<=0||static_cast<std::size_t>(size)>=sizeof(line))return;
  DWORD written=0;WriteFile(log_file,line,static_cast<DWORD>(size),&written,nullptr);
  log_bytes+=written;++log_events;
}
bool name_is(std::uint32_t object,std::uint32_t offset,const char* expected) noexcept {
  std::uint32_t data=0;std::int32_t length=-1;
  const auto wanted=std::strlen(expected);
  if(wanted>63||!field(object,offset+8,data)||!field(object,offset+12,length)||
    length<0||static_cast<std::size_t>(length)!=wanted)return false;
  if(!length)return true;
  std::array<char,64> text{};
  return memory.copy(data,text.data(),wanted+1)&&!std::memcmp(text.data(),expected,wanted+1);
}
bool station_owns(std::uint32_t mob,std::uint32_t hero) noexcept {
  std::uint32_t node=0;if(!field(mob,0x18c,node))return false;
  std::array<std::uint32_t,64> seen{};std::size_t count=0;unsigned matches=0;
  while(node){
    if(count==seen.size())return false;
    for(std::size_t i=0;i<count;++i)if(seen[i]==node)return false;
    seen[count++]=node;
    std::uint32_t position=0,next=0,npc=0;
    if(!field(node,0,position)||!field(node,4,next)||!field(position,0x44,npc))return false;
    if(npc==hero)++matches;node=next;
  }
  return matches==1;
}
struct Context {
  bool valid=false,identity_known=false,station_known=false,ownership_known=false;
  std::uint32_t game=0,world=0,hero=0,station=0;
  cat::Station kind=cat::Station::stove;
  ubc::SelectionDialog dialog{};
};
Context borrowed_context;
Context context() noexcept {
  Context c;
  if(!read(a::game_pointer,c.game)||!c.game||integer(c.game,0)!=static_cast<std::int32_t>(a::game_vtable)||
    !field(c.game,a::game_world,c.world)||!read(s::hero_pointer,c.hero))return c;
  c.identity_known=true;
  if(!c.world||!c.hero)return c;
  c.station_known=field(c.hero,s::npc_interact_mob,c.station);
  c.dialog=ubc::read_selection_dialog(memory,s::information_manager);
  if(!c.station_known||!c.station||!c.dialog.valid||c.dialog.npc!=c.hero||c.dialog.player!=c.hero||
    !station_owns(c.station,c.hero)||integer(c.station,0x1f4)!=1||
    integer(c.station,0x1f8)!=1||integer(c.station,0x1fc)!=1)return c;
  c.ownership_known=true;
  const auto hero=rt::read_symbol_address(memory,s::game_parser,ubc::lhiver::hero_symbol);
  std::uint32_t bound=0;
  if(!hero.valid||!field(hero.address,0x1c,bound)||bound!=c.hero)return c;
  const auto production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
  if(!production.valid)return c;
  if(production.value==14&&name_is(c.station,0x1e0,"FRYINGMEAT")&&
    name_is(c.station,0x1b8,"STOVE")&&name_is(c.station,0x1a4,"")&&name_is(c.station,0x1cc,"B_USESTOVE"))
    c.kind=cat::Station::stove;
  else if(production.value==4&&name_is(c.station,0x1e0,"POTIONALCHEMY")&&
    name_is(c.station,0x1a4,"ITMI_FLASK")&&name_is(c.station,0x1cc,""))c.kind=cat::Station::alchemy;
  else return c;
  c.valid=true;return c;
}
ubc::StationKey key(const Context& c) noexcept {
  return {generation,GetCurrentThreadId(),c.world,c.hero,c.station,s::information_manager,c.kind};
}
bool same_owner(const Context& left,const Context& right) noexcept {
  return left.ownership_known&&right.ownership_known&&left.game==right.game&&left.world==right.world&&
    left.hero==right.hero&&left.station==right.station&&left.dialog.view==right.dialog.view&&
    left.dialog.npc==right.dialog.npc&&left.dialog.player==right.dialog.player;
}
bool same_context(const Context& left,const Context& right) noexcept {
  return left.valid&&right.valid&&same_owner(left,right)&&left.kind==right.kind&&
    left.dialog.mode==right.dialog.mode&&left.dialog.info==right.dialog.info;
}
bool foreign_dialog(const Context& c) noexcept {
  return c.identity_known&&c.hero&&c.dialog.valid&&(c.dialog.npc!=c.hero||c.dialog.player!=c.hero);
}
const cat::Owner* owner_for(std::int32_t instance) noexcept {
  if(const auto* owner=nav::owner_for(instance))return owner;
  for(const auto& recipe:cat::recipes)for(const auto& owner:recipe.owners)
    if(owner.instance==instance)return &owner;
  return nullptr;
}
bool linked(const ubc::SelectionInfo& info) noexcept {
  const auto* owner=owner_for(info.instance);
  return info.valid&&owner&&info.number==owner->number&&info.npc==owner->npc&&
    info.important==static_cast<int>(owner->important)&&info.trade==static_cast<int>(owner->trade)&&
    info.permanent==static_cast<int>(owner->permanent)&&info.condition==static_cast<int>(owner->condition->symbol)&&
    info.function==static_cast<int>(owner->information->symbol);
}
bool linked_step_info(const ubc::SelectionInfo& info,const nav::Step* step) noexcept {
  if(!linked(info))return false;
  if(!info.first_choice)return true;
  // OnInfo calls the original information function even with choices
  // remaining. Only these six bound owner infos start by calling
  // ClearChoices on their own owner and then rebuild the entire selection.
  // Direct INFO effects and START/STOP still require an empty list.
  if(!step||step->kind!=nav::Kind::open_owner||step->invocation!=cat::Invocation::info||
    step->owner_instance!=info.instance||step->function_symbol!=info.function)return false;
  for(const auto& route:nav::routes)if(route.owner->instance==info.instance&&
    route.owner->information->symbol==static_cast<std::uint32_t>(info.function)&&
    step->before==route.category&&step->after==route.choices)return true;
  return false;
}
bool navigation_contract() noexcept {
  std::array<ubc::lhiver::SymbolBinding,cat::common_bindings.size()+nav::flag_bindings.size()> bindings{};
  std::size_t count=0;
  for(const auto& b:cat::common_bindings)bindings[count++]=b;
  for(const auto& b:nav::flag_bindings)bindings[count++]=b;
  const ubc::lhiver::Profile profile{layout_profile.parser_address,layout_profile.symbol_count,
    layout_profile.code_length,layout_profile.code_sha256,nav::functions.data(),nav::functions.size(),bindings.data(),count};
  return rt::verify_local_contract(memory,s::game_parser,profile).matches;
}
bool complete_collection(const Context& c,ubc::SelectionChoices* choice_out=nullptr) noexcept {
  if(!c.dialog.valid||!rows.generation||rows.view!=c.dialog.view)return false;
  if(c.dialog.mode==1)return rows.complete(c.dialog);
  if(c.dialog.mode!=2||!choice_collection)return false;
  const auto choices=ubc::read_selection_choices(memory,c.dialog);
  if(!choices.valid||!linked(choices.owner)||choices.count!=choice_count)return false;
  for(std::size_t i=0;i<choice_count;++i)if(choices.functions[i]!=choice_functions[i])return false;
  if(choice_out)*choice_out=choices;
  return true;
}
nav::Page page(const Context& c) noexcept {
  if(!c.valid||c.kind!=cat::Station::alchemy||!navigation_contract())return nav::Page::unknown;
  nav::PageState state;
  state.known=true;state.production=4;
  std::array<std::int32_t,5> values{};
  for(std::size_t i=0;i<values.size();++i){
    const auto v=rt::symbol_int(memory,s::game_parser,nav::flag_bindings[i].symbol);
    if(!v.valid)return nav::Page::unknown;
    values[i]=v.value;
  }
  state.health=values[0];state.mana=values[1];state.booze=values[2];state.tobacco=values[3];state.special=values[4];
  state.invocation=c.dialog.mode==1?cat::Invocation::info:cat::Invocation::choice;
  if(c.dialog.mode==2){
    const auto info=ubc::read_selection_info(memory,c.dialog.info);
    if(!linked(info))return nav::Page::unknown;
    state.choice_owner=info.instance;
  }
  for(std::size_t i=0;i<values.size();++i){
    const auto v=rt::symbol_int(memory,s::game_parser,nav::flag_bindings[i].symbol);
    if(!v.valid||v.value!=values[i])return nav::Page::unknown;
  }
  return nav::classify_page(state);
}
ubc::RecipeCatalogueCapture capture(const Context& c) noexcept {
  std::array<bool,cat::recipes.size()> hand_contract{};
  // Authorize only the exact station variants bound and verified
  // above. A static inferred_same_station value is never sufficient.
  for(std::size_t i=0;i<hand_contract.size();++i)hand_contract[i]=c.valid&&cat::recipes[i].station==c.kind;
  const bool outer=frame_depth==1&&!update_depth&&!info_depth&&!choice_depth;
  const bool callback=native_update&&update_depth==1&&info_depth+choice_depth<=1;
  const bool boundary=(outer||callback)&&!station_depth&&!collection_depth;
  auto captured=catalogue_reader.capture(memory,s::game_parser,key(c),c.valid&&boundary,hand_contract);
  if(!same_context(c,context()))captured.snapshot.key={};
  if(!result_guard.permits_next()||bc::diagnostics().writes_disabled)
    for(auto& recipe:captured.snapshot.recipes)recipe.contract=false;
  return captured;
}
const ubc::RecipeState* state_for(const ubc::CatalogueSnapshot& snapshot,cat::RecipeId id) noexcept {
  for(const auto& recipe:snapshot.recipes)if(recipe.id==id)return &recipe;
  return nullptr;
}
ubc::HandoffObservation observation(const Context& c,bool outer,bool materials=true) noexcept {
  ubc::HandoffObservation out;
  out.key=key(c);out.view=c.dialog.view;out.frame=frame;out.collection=rows.generation;
  out.context_known=c.identity_known&&c.station_known&&c.dialog.valid&&!foreign_dialog(c);
  out.mode=c.dialog.mode==1?cat::Invocation::info:cat::Invocation::choice;
  out.count=c.dialog.count;out.complete=complete_collection(c);
  out.ready=c.valid&&c.dialog.waiting&&c.dialog.open&&!c.dialog.done;
  out.outer_frame=outer;out.quiescent=outer&&frame_depth==1&&!update_depth&&!info_depth&&!choice_depth&&!station_depth&&!collection_depth;
  out.focused=ui.focused();out.page=page(c);
  out.page_known=c.valid&&(c.kind==cat::Station::stove?c.dialog.mode==1:nav::supported_page(out.page));
  if(c.dialog.mode==2)out.choice_owner=ubc::read_selection_info(memory,c.dialog.info).instance;
  if(materials&&c.valid){
    const auto current=capture(c);
    if(const auto* material=state_for(current.snapshot,handoff.request().id))out.materials=*material;
    out.hands=current.hands;
    if(!(current.snapshot.key==out.key))out.context_known=false;
  }
  return out;
}
ubc::HandoffOffer resolve_offer(const Context& c,const nav::Step* step) noexcept {
  if(!step||!complete_collection(c))return {};
  ubc::HandoffOffer offer;
  if(step->invocation==cat::Invocation::info&&c.dialog.mode==1){
    for(std::size_t i=0;i<rows.count;++i){
      const auto& row=rows.rows[i];
      if(linked_step_info(row,step)&&row.instance==step->owner_instance&&row.function==step->function_symbol){
        if(offer.valid)return {};
        offer={true,cat::Invocation::info,row.instance,row.function,static_cast<std::int32_t>(i),rows.generation};
      }
    }
  }else if(step->invocation==cat::Invocation::choice&&c.dialog.mode==2){
    ubc::SelectionChoices choices;
    if(!complete_collection(c,&choices)||choices.owner.instance!=step->owner_instance)return {};
    for(std::size_t i=0;i<choices.count;++i)if(choices.functions[i]==step->function_symbol){
      if(offer.valid)return {};
      offer={true,cat::Invocation::choice,choices.owner.instance,choices.functions[i],static_cast<std::int32_t>(i),rows.generation};
    }
  }
  return offer;
}
void restore_view(const Context& c) noexcept {
  if(!borrowed_view)return;
  if(same_owner(c,borrowed_context)&&c.dialog.mode==borrowed_context.dialog.mode&&
    c.dialog.info==borrowed_context.dialog.info&&c.dialog.view==borrowed_view&&c.dialog.waiting&&c.dialog.hidden){
    reinterpret_cast<a::FastObject>(a::choice_open_immediate)(reinterpret_cast<void*>(borrowed_view),nullptr);
    reinterpret_cast<a::Select>(a::choice_activate)(reinterpret_cast<void*>(borrowed_view),1);
  }
  borrowed_view=0;
}
void close_ui(const Context& c) noexcept {ui.close();restore_view(c);}
void borrow_view(const Context& c) noexcept {
  borrowed_view=c.dialog.view;borrowed_context=c;
  reinterpret_cast<a::Select>(a::choice_activate)(reinterpret_cast<void*>(borrowed_view),0);
  reinterpret_cast<a::FastObject>(a::choice_close_immediate)(reinterpret_cast<void*>(borrowed_view),nullptr);
}
void record_materials(const ubc::RecipeState& value,const char* name) noexcept {
  for(std::size_t i=0;i<value.target_count;++i)event(name,value.materials[i].symbol,value.materials[i].stored);
}
void drive_handoff(const Context& c) noexcept {
  if(!handoff.active())return;
  const auto current=observation(c,true);
  if(GetTickCount64()-handoff_started>30000)handoff.cancel(ubc::HandoffIssue::timeout);
  handoff.observe(current);
  if(foreign_dialog(c))handoff.cancel(ubc::HandoffIssue::context);
  if(handoff.state()==ubc::HandoffState::quarantined){
    bool finished=c.identity_known&&c.station_known&&integer(s::information_manager,0x2c)==1;
    for(const auto offset:{0x30u,0x34u,0x38u,0x3cu,0x40u,0x44u})
      if(integer(s::information_manager,offset)!=0)finished=false;
    if(finished)handoff.context_ended({handoff.request().key,dispatched_ticket.view,frame,true,true,true});
  }
  if(handoff.state()!=ubc::HandoffState::awaiting_step)return;
  if(!current.ready||!current.complete||!ubc::NativeSelectionUi::callback_is_top(c.dialog.view+a::view_callback)||
    integer(0x8d1e38,8)!=0){handoff.cancel(ubc::HandoffIssue::boundary);return;}
  const auto offer=resolve_offer(c,handoff.step());
  const auto ticket=handoff.ticket(current,offer);
  if(!ticket)return;
  const auto fresh=context();
  if(!handoff.dispatch(*ticket,observation(fresh,true),resolve_offer(fresh,handoff.step())))return;
  dispatched_ticket=*ticket;
  auto* view=reinterpret_cast<void*>(fresh.dialog.view);
  reinterpret_cast<a::Select>(a::choice_select)(view,offer.row);
  // Only this one synthetic input passes through the view hook. Physical
  // input remains guarded during the original category continuations.
  private_enter=true;
  reinterpret_cast<a::HandleEvent>(a::choice_handle_event)(static_cast<std::uint8_t*>(view)+a::view_callback,m::key_return);
  private_enter=false;
  const auto after=ubc::read_selection_dialog(memory,s::information_manager);
  if(!after.valid||!after.done||after.selected!=offer.row)handoff.cancel(ubc::HandoffIssue::offer);
  event("handoff_step",offer.function,static_cast<int>(ticket->step));
}
void observe_result(const Context& c) noexcept {
  if(result_guard.state()!=ubc::RecipeResultState::waiting)return;
  const auto& request=handoff.request();const auto* recipe=cat::find(request.id);
  ubc::RecipeResultObservation value;
  value.generation=request.key.generation;value.frame=frame;value.stable_boundary_verified=true;
  value.identity_known=c.identity_known;value.world=c.world;value.hero=c.hero;
  value.station_known=c.station_known;value.station=c.station;
  const auto production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
  value.production_known=production.valid;value.production=production.value;
  value.selection_known=c.dialog.valid;value.selection_open=c.dialog.open;value.selection_waiting=c.dialog.waiting;
  value.selection_done=c.dialog.done;
  value.collection_known=c.valid&&c.dialog.mode==1&&complete_collection(c);value.collection=rows.generation;
  std::array<ubc::RecipeResultItem,cat::catalog_target_capacity> items{};
  if(recipe&&c.identity_known&&c.world==request.key.world&&c.hero==request.key.hero){
    const auto inventory=ubc::read_packed_inventory<cat::catalog_target_capacity>(memory,c.hero,
      recipe->targets.data,recipe->targets.count,true,result_workspace);
    value.stock_known=inventory.storage_totals_known&&inventory.snapshot_unchanged&&
      inventory.stable_boundary_verified&&inventory.issue==ubc::PackedInventoryIssue::none;
    for(std::size_t i=0;i<recipe->targets.count;++i)items[i]={recipe->targets[i].symbol,inventory.total[i]};
    value.items={items.data(),recipe->targets.count};
    value.hands=ubc::read_hands(memory,c.hero);value.hands_known=value.hands.left.state>=0&&value.hands.right.state>=0;
    const auto fresh=context();
    if(fresh.world!=c.world||fresh.hero!=c.hero||fresh.station!=c.station)value.stock_known=false;
  }
  result_guard.observe(value);
  if(GetTickCount64()-result_started>30000)result_guard.timeout();
  if(result_guard.state()==ubc::RecipeResultState::verified){
    handoff.acknowledge_result(true);
    for(std::size_t i=0;i<value.items.count;++i)event("stock_verified",value.items[i].symbol,static_cast<int>(value.items[i].total));
    event("amount_result_verified",static_cast<int>(request.id),static_cast<int>(request.quantity));
  }else if(result_guard.state()==ubc::RecipeResultState::failed)
    event("amount_result_failed",static_cast<int>(result_guard.issue()));
}

bool open_failure_notice() noexcept {
  if(ui.open_failure()){event("failure_notice_opened");return true;}
  if(!failure_open_rejected_logged){failure_open_rejected_logged=true;event("failure_notice_open_rejected");}
  return false;
}

void __fastcall after_frame(void* game,void* manager) {
  const auto error=GetLastError();std::uint32_t current=0,owned=0;
  if(!InterlockedCompareExchange(&installed,0,0)||!read(a::game_pointer,current)||
    current!=reinterpret_cast<std::uintptr_t>(game)||!field(reinterpret_cast<std::uint32_t>(manager),0xc,owned)||owned!=current){SetLastError(error);return;}
  const auto thread=static_cast<LONG>(GetCurrentThreadId());const auto owner=InterlockedCompareExchange(&engine_thread,thread,0);
  if((owner&&owner!=thread)||disabled){SetLastError(error);return;}
  ++frame_depth;
  if(frame_depth==1&&!update_depth&&!info_depth&&!choice_depth&&!station_depth&&!collection_depth){
    if(frame!=UINT64_MAX)++frame;else disabled=true;
    const auto c=context();observe_result(c);
    const int state=(c.identity_known?1:0)|(c.world?2:0)|(c.hero?4:0)|(c.station?8:0)|
      (c.dialog.valid?16:0)|(c.ownership_known?32:0)|(c.valid?64:0)|
      (c.dialog.waiting?128:0)|(c.dialog.open?256:0)|(c.dialog.hidden?512:0)|(c.dialog.done?1024:0);
    if(state!=last_frame_state){last_frame_state=state;event("frame_state",state,c.dialog.mode);}
    if(result_guard.state()==ubc::RecipeResultState::failed||bc::diagnostics().writes_disabled){
      const int failed=(result_guard.state()==ubc::RecipeResultState::failed?1:0)|
        (bc::diagnostics().writes_disabled?2:0)|(failure_notified?4:0)|(ui.opened()?8:0)|
        (c.identity_known?16:0)|(c.world?32:0)|(c.hero?64:0)|(integer(0x8d1e38,8)==0?128:0);
      if(failed!=last_failure_state){last_failure_state=failed;event("failure_notice_state",failed,static_cast<int>(result_guard.issue()));}
    }
    if(original_station&&(!c.station_known||c.station!=original_station))original_station=0;
    if(ui.opened()){
      if(ui.failure_notice()){
        ui.observe_failure(frame,ui.buttons_held(),ui.focused());
        if(!c.identity_known||!c.world||!c.hero||!ui.owns_input())ui.dismiss_failure();
        if(ui.failure_return_ready()){original_station=c.station;close_ui(c);}else ui.render_failure();
      }else{
        session.observe(frame,ui.buttons_held(),ui.focused());
        if(!same_context(c,borrowed_context)||!ui.owns_input())session.original(ubc::SelectionIssue::context_changed);
        if(session.phase()!=Phase::original)session.refresh(capture(c).snapshot);
        if(session.phase()==Phase::original){
          if(session.return_ready()){original_station=c.station;close_ui(c);event("ui_original");}
          else ui.render(session,execution_enabled);
        }else if(session.phase()==Phase::ready){
          close_ui(c);
          const auto restored=context();const auto captured=capture(restored);
          const auto request=session.take(captured.snapshot,true);
          if(request){
            record_materials(request->material_snapshot,"stock_before");
            event("recipe_request",static_cast<int>(request->id),static_cast<int>(request->quantity));
            if(!execution_enabled){original_station=c.station;event("preview_confirmed");}
            else{
              // begin already needs the requested recipe. Until
              // then, observation() still uses the previous order.
              auto initial=observation(restored,true,false);
              initial.materials=request->material_snapshot;initial.hands=captured.hands;
              const auto plan=request->key.kind==cat::Station::alchemy?
                nav::make_plan(initial.page,request->id,request->quantity):ubc::make_direct_info_plan(*request);
              handed_hands=captured.hands;effect_executed=false;handoff_started=GetTickCount64();
              if(handoff.begin(*request,plan,initial,36000))drive_handoff(restored);
              else{original_station=c.station;event("handoff_rejected",static_cast<int>(handoff.issue()));}
            }
          }else{original_station=c.station;event("handoff_stale");}
        }else if(!ui.render(session,execution_enabled))session.original(ubc::SelectionIssue::invalid_snapshot);
      }
    }else if(!failure_notified&&c.identity_known&&c.world&&c.hero&&integer(0x8d1e38,8)==0&&
      (result_guard.state()==ubc::RecipeResultState::failed||bc::diagnostics().writes_disabled)){
      if(open_failure_notice()){
        failure_notified=true;
        if(c.valid&&c.dialog.waiting&&c.dialog.open)borrow_view(c);
        ui.render_failure();
      }
    }else if(handoff.active()){
      drive_handoff(c);
      if(handoff.state()==ubc::HandoffState::cancelled)original_station=c.station;
    }else if(!original_station&&c.valid&&c.dialog.waiting&&c.dialog.open&&complete_collection(c)&&
      ubc::NativeSelectionUi::callback_is_top(c.dialog.view+a::view_callback)&&integer(0x8d1e38,8)==0){
      const bool failure=result_guard.state()==ubc::RecipeResultState::failed||bc::diagnostics().writes_disabled;
      if(failure&&!failure_notified){
        if(open_failure_notice()){failure_notified=true;borrow_view(c);ui.render_failure();}
      }else if(!failure){
        const auto current_page=page(c);
        if(c.kind==cat::Station::stove||nav::supported_page(current_page)){
          if(generation!=UINT64_MAX)++generation;else disabled=true;
          const auto captured=capture(c);
          if(!disabled&&session.open(captured.snapshot)&&ui.open()){
            borrow_view(c);event("ui_open",static_cast<int>(c.kind),static_cast<int>(session.visible_count()));
            for(const auto& r:captured.snapshot.recipes)if(r.known)event("recipe_available",static_cast<int>(r.id),static_cast<int>(r.maximum));
            ui.render(session,execution_enabled);
          }else{
            const int issue=static_cast<int>(captured.diagnostic.issue);
            if(issue!=last_capture_issue){last_capture_issue=issue;event("catalogue_unavailable",issue);}
          }
        }
      }
    }
    const int handoff_state=static_cast<int>(handoff.state());
    if(handoff_state!=last_handoff_state){
      if(handoff.state()==ubc::HandoffState::cancelled)original_station=c.station;
      last_handoff_state=handoff_state;event("handoff_state",handoff_state,static_cast<int>(handoff.issue()));
    }
  }
  --frame_depth;SetLastError(error);
}

void __fastcall observe_clear(void* view,void* unused) {
  const auto error=GetLastError();const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());const bool own=own_thread();
  if(own){++collection_depth;rows.begin(reinterpret_cast<std::uint32_t>(view),caller==a::info_clear_return&&collection_depth==1);
    choice_collection=caller==a::choice_clear_return&&collection_depth==1;choice_count=0;}
  SetLastError(error);original_clear(view,unused);const auto after=GetLastError();
  if(own){if(integer(reinterpret_cast<std::uint32_t>(view),a::view_count)!=0){rows.valid=false;choice_collection=false;}--collection_depth;}
  SetLastError(after);
}
void __fastcall observe_add(void* view,void* text,int number) {
  const auto error=GetLastError();const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool own=own_thread();const auto address=reinterpret_cast<std::uint32_t>(view);
  const auto before=own?integer(address,a::view_count):-1;ubc::SelectionInfo info;ubc::ChoiceSnapshot choice;
  if(own){++collection_depth;const auto text_address=reinterpret_cast<std::uint32_t>(text);
    if(caller==a::info_add_return&&text_address>=0x30)info=ubc::read_selection_info(memory,text_address-0x30);
    if(caller==a::choice_add_return)choice=ubc::read_choice(memory,s::information_manager,text_address);}
  SetLastError(error);original_add(view,text,number);const auto after=GetLastError();
  if(own){const auto count=integer(address,a::view_count);
    if(caller==a::info_add_return)rows.append(address,info,before,count,collection_depth==1&&number==0);else rows.valid=false;
    if(choice_collection){
      if(caller!=a::choice_add_return||collection_depth!=1||number!=0||!choice.valid||before<0||
        static_cast<std::size_t>(before)!=choice_count||count!=before+1||choice.index!=before||choice_count==choice_functions.size())choice_collection=false;
      else choice_functions[choice_count++]=choice.function;
    }--collection_depth;
  }SetLastError(after);
}

const cat::Callback* amount_callback(std::int32_t symbol) noexcept {
  for(const auto& recipe:cat::recipes)for(const auto& callback:recipe.callbacks)
    if(callback.minimum_quantity&&callback.function->symbol==static_cast<std::uint32_t>(symbol))return &callback;
  return nullptr;
}
bool __cdecl batch_signature(const bc::Call& call) noexcept {
  const auto* entry=amount_callback(call.symbol);
  if(!entry||reinterpret_cast<std::uintptr_t>(call.parser)!=s::game_parser)return false;
  const auto& function=*entry->function;rt::detail::Layout layout{};rt::detail::Symbol symbol{};
  return rt::detail::read_layout(memory,s::game_parser,layout_profile,layout)==rt::Issue::none&&
    rt::detail::lookup(memory,layout,function.symbol,symbol)==rt::Issue::none&&
    rt::detail::name_matches(memory,symbol,function.name)==rt::Issue::none&&
    symbol.type==5&&symbol.flags==1&&symbol.elements==0&&symbol.offset==0&&
    rt::detail::parent_matches(memory,layout,symbol,-1)==rt::Issue::none&&
    rt::detail::stable_layout(memory,s::game_parser,layout_profile,layout)==rt::Issue::none;
}
bc::Decision __cdecl select_batch_call(const bc::Call& call) noexcept {
  if(!own_thread())return bc::Decision::ordinary;
  const auto* ticket=handoff.current_ticket();
  if(!handoff.active()||!ticket||call.symbol!=ticket->offer.function)return bc::Decision::ordinary;
  const auto expected=ticket->offer.mode==cat::Invocation::info?bc::info_call_return:bc::choice_call_return;
  if(!effect_authorized||effect_executed||handoff.cancelled()||call.caller!=expected||
    handoff.state()!=ubc::HandoffState::executing)return bc::Decision::reject;
  return bc::Decision::writer;
}
bool late_gate() noexcept {
  const auto* ticket=handoff.current_ticket();const auto* step=handoff.step();
  if(!ticket||!step||!step->is_effect()||!effect_authorized||effect_executed||handoff.cancelled()||
    handoff.state()!=ubc::HandoffState::executing||!native_update||!own_thread()||update_depth!=1||
    station_depth||collection_depth||info_depth+choice_depth!=1||!ui.focused())return false;
  const auto c=context();const auto current=observation(c,false);
  return c.valid&&current.context_known&&current.key==handoff.request().key&&current.view==ticket->view&&
    current.collection==ticket->offer.collection&&current.mode==ticket->offer.mode&&
    current.page_known&&current.page==step->before&&
    ubc::recipe_handoff_detail::same_resources(current.materials,handoff.request().material_snapshot)&&
    ubc::recipe_handoff_detail::same_hands(current.hands,handed_hands)&&same_context(c,context())&&
    (ticket->offer.mode==cat::Invocation::info?current.complete:
      post_remove_choice.consume(memory,c.dialog,rows.generation,ticket->id,ticket->offer.function));
}
struct NativeAmountContext {
  rt::detail::Layout layout{};
  const ubc::lhiver::Function* function=nullptr;
};
bool amount_identity(void* raw,std::uint32_t address,std::size_t length) noexcept {
  const auto& ctx=*static_cast<NativeAmountContext*>(raw);
  if(!own_thread()||!ctx.function||length!=ctx.function->byte_length||address!=ctx.layout.code+ctx.function->byte_offset)return false;
  rt::detail::Layout fresh{};rt::detail::Symbol symbol{};
  return rt::detail::read_layout(memory,s::game_parser,layout_profile,fresh)==rt::Issue::none&&
    rt::detail::same_layout(ctx.layout,fresh)&&rt::detail::lookup(memory,fresh,ctx.function->symbol,symbol)==rt::Issue::none&&
    symbol.data==ctx.function->byte_offset&&symbol.type==5&&symbol.flags==1&&symbol.elements==0&&symbol.offset==0&&
    rt::detail::name_matches(memory,symbol,ctx.function->name)==rt::Issue::none;
}
bool amount_writable(void*,std::uint32_t address,std::size_t size) noexcept {
  if(!address||!size||size>UINT32_MAX-address)return false;
  std::uintptr_t cursor=address;const auto end=cursor+size;
  while(cursor<end){
    MEMORY_BASIC_INFORMATION page_info{};
    if(VirtualQuery(reinterpret_cast<void*>(cursor),&page_info,sizeof(page_info))!=sizeof(page_info)||
      page_info.State!=MEM_COMMIT||page_info.Type!=MEM_PRIVATE||(page_info.Protect&PAGE_GUARD)||
      (page_info.Protect&0xffu)!=PAGE_READWRITE)return false;
    const auto base=reinterpret_cast<std::uintptr_t>(page_info.BaseAddress);
    if(base>cursor||page_info.RegionSize>UINT32_MAX-base||base+page_info.RegionSize<=cursor)return false;
    cursor=base+page_info.RegionSize;
  }
  return true;
}
bool amount_replace(void* raw,std::uint32_t address,const void* before,const void* after,std::size_t size) noexcept {
  auto* ctx=static_cast<NativeAmountContext*>(raw);
  if(!ctx->function||!amount_identity(raw,ctx->layout.code+ctx->function->byte_offset,ctx->function->byte_length)||
    !amount_writable(raw,address,size))return false;
  __try {
    auto* target=reinterpret_cast<void*>(address);if(std::memcmp(target,before,size))return false;
    std::memcpy(target,after,size);return std::memcmp(target,after,size)==0;
  } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void flush_amount_diagnostic() noexcept {
  if(!amount_diagnostic.pending)return;
  const auto saved=GetLastError();const auto report=amount_diagnostic;amount_diagnostic={};
  if(report.entered)event("native_effect_enter",report.symbol,report.quantity);
  if(report.rejected)event("amount_rejected",report.rejected);
  event(report.restored?"amount_restored":"amount_restore_failed",report.symbol,report.quantity);
  event("amount_duration_us",report.symbol,report.duration);SetLastError(saved);
}
void* __cdecl execute_original_amount(void* raw) {
  auto& scope=*static_cast<bc::WriterScope*>(raw);const auto& request=handoff.request();
  const auto* recipe=cat::find(request.id);const auto* ticket=handoff.current_ticket();
  const ubc::RecipeResultBinding binding{request.key.generation,frame,ticket?ticket->offer.collection:0,end_sequence,
    request.key.world,request.key.hero,request.key.station,handed_hands.right};
  if(!recipe||!result_guard.begin(*recipe,{expected_result.data(),recipe->targets.count},binding)){
    handoff.cancel(ubc::HandoffIssue::resources);return nullptr;
  }
  effect_executed=true;
  return scope.original(scope.call.parser,scope.call.symbol);
}
void* __cdecl execute_amount(bc::WriterScope& scope) {
  const auto incoming_error=GetLastError();scope.restore_verified=true;
  const auto& request=handoff.request();
  amount_diagnostic={true,false,true,0,0,scope.call.symbol,static_cast<int>(request.quantity)};
  const auto* recipe=cat::find(request.id);
  const auto* entry=recipe?cat::callback_for_quantity(*recipe,request.quantity):nullptr;
  if(!entry||entry->function->symbol!=static_cast<std::uint32_t>(scope.call.symbol)||!result_guard.permits_next()||!late_gate()){
    handoff.cancel(ubc::HandoffIssue::resources);amount_diagnostic.rejected=1;SetLastError(incoming_error);return nullptr;
  }
  NativeAmountContext ctx;ctx.function=entry->function;
  if(rt::detail::read_layout(memory,s::game_parser,layout_profile,ctx.layout)!=rt::Issue::none||
    ctx.function->byte_offset>ctx.layout.code_length||ctx.function->byte_length>ctx.layout.code_length-ctx.function->byte_offset){
    handoff.cancel(ubc::HandoffIssue::context);amount_diagnostic.rejected=2;SetLastError(incoming_error);return nullptr;
  }
  for(std::size_t i=0;i<request.material_snapshot.target_count;++i){
    const auto& row=request.material_snapshot.materials[i];const auto preview=ubc::recipe_amount_preview(row,request.quantity);
    if(!preview.available_known||preview.remaining<0||preview.remaining>INT32_MAX){
      handoff.cancel(ubc::HandoffIssue::resources);amount_diagnostic.rejected=3;SetLastError(incoming_error);return nullptr;
    }
    expected_result[i]={row.symbol,preview.remaining};
  }
  LARGE_INTEGER before_time{},after_time{},frequency{};
  QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&before_time);result_started=GetTickCount64();
  const ubc::AmountMemory writable{memory,&ctx,amount_identity,amount_writable,amount_replace};ubc::AmountOutcome outcome;
  SetLastError(incoming_error);
  void* result=ubc::run_amount_call(amount_patch,writable,ctx.layout.code+ctx.function->byte_offset,
    entry->amount,request.quantity,execute_original_amount,&scope,scope.restore_verified,outcome);
  const auto saved=GetLastError();amount_diagnostic.entered=effect_executed;amount_diagnostic.restored=scope.restore_verified;
  if(!outcome.entered){handoff.cancel(ubc::HandoffIssue::resources);amount_diagnostic.rejected=10+static_cast<int>(outcome.issue);}
  if(!scope.restore_verified){handoff.cancel(ubc::HandoffIssue::resources);result_guard.fail(ubc::RecipeResultIssue::restore);}
  QueryPerformanceCounter(&after_time);
  if(frequency.QuadPart>0&&after_time.QuadPart>=before_time.QuadPart){
    const auto elapsed=static_cast<double>(after_time.QuadPart-before_time.QuadPart)*1000000.0/static_cast<double>(frequency.QuadPart);
    amount_diagnostic.duration=elapsed>=INT32_MAX?INT32_MAX:static_cast<int>(elapsed);
  }
  SetLastError(saved);return result;
}

bool bound_callback(const ubc::HandoffOffer& offer) noexcept {
  const auto* ticket=handoff.current_ticket();
  return ticket&&offer.valid&&offer.owner==ticket->offer.owner&&offer.function==ticket->offer.function;
}
bool callback_guarded(const Context& c,const ubc::HandoffOffer& offer) noexcept {
  if(!handoff.active())return false;
  if(bound_callback(offer))return true;
  // No further order goes to the engine after the effect. Only the
  // old callback remains protected against repetition. A normal end
  // path or a confirmed unrelated dialog must be able to continue.
  if(handoff.state()==ubc::HandoffState::result_pending)return !offer.valid;
  if(foreign_dialog(c)&&offer.valid){handoff.cancel(ubc::HandoffIssue::context);return false;}
  return true;
}
ubc::HandoffCallbackProof proof(std::uintptr_t caller,bool info) noexcept {
  return {own_thread(),caller==(info?a::info_effect_return:a::choice_effect_return),native_update,
    update_depth,info?info_depth:choice_depth,info?choice_depth:info_depth,collection_depth,station_depth};
}
void __fastcall observe_update(void* manager,void* unused) {
  const auto error=GetLastError();const bool own=own_thread();
  const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());const bool previous_native=own?native_update:false;
  if(own){++update_depth;native_update=update_depth==1&&reinterpret_cast<std::uint32_t>(manager)==s::information_manager&&
    (caller==0x74c117||caller==0x75a727||caller==0x6c8a46);
    if(handoff.active()&&!ui.focused())handoff.cancel(ubc::HandoffIssue::focus);}
  SetLastError(error);original_update(manager,unused);const auto after=GetLastError();
  if(own){
    if(update_depth==1&&recover_info){
      recover_info=false;const auto c=context();const auto* ticket=handoff.current_ticket();
      if(native_update&&c.valid&&ticket&&key(c)==handoff.request().key&&c.dialog.view==ticket->view&&
        c.dialog.rejected_info_transition&&c.dialog.selected==ticket->offer.row&&rows.generation==ticket->offer.collection){
        // Only for a confirmed null resolve result. The original
        // InfoBegin creates a new collection without direct flag writes.
        reinterpret_cast<a::FastObject>(a::info_begin)(manager,nullptr);event("selection_reopened");
      }else event("selection_quarantined");
    }
    --update_depth;native_update=previous_native;
  }SetLastError(after);
}
void* __fastcall observe_resolve(void* manager,void*,void* player,void* npc,int row) {
  const auto error=GetLastError();const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  SetLastError(error);void* result=original_resolve(manager,player,npc,row);const auto after=GetLastError();
  if(own_thread()&&handoff.active()&&caller==a::resolve_selection_return){
    const auto info=ubc::read_selection_info(memory,reinterpret_cast<std::uint32_t>(result));const auto c=context();
    ubc::HandoffOffer actual{info.valid,cat::Invocation::info,info.instance,info.function,row,rows.generation};
    const auto current=observation(c,false);
    if(callback_guarded(c,actual)){
      const bool different=current.context_known&&current.view&&
        (!(current.key==handoff.request().key)||current.view!=dispatched_ticket.view);
      if(different&&actual.valid&&!bound_callback(actual)){handoff.cancel(ubc::HandoffIssue::context);}
      else if(!linked_step_info(info,handoff.step())||!native_update||update_depth!=1||info_depth||choice_depth||station_depth||collection_depth||
        reinterpret_cast<std::uint32_t>(player)!=handoff.request().key.hero||
        reinterpret_cast<std::uint32_t>(npc)!=handoff.request().key.hero||
        !handoff.matches_pending(dispatched_ticket,current,actual)){
        result=nullptr;recover_info=true;handoff.cancel(ubc::HandoffIssue::offer);event("resolution_rejected");
      }
    }
  }SetLastError(after);return result;
}
void __fastcall observe_info(void* info,void*) {
  const auto error=GetLastError();const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());const bool own=own_thread();
  if(own)++info_depth;
  ubc::HandoffCallback witness;bool allow=true;bool final_effect=false;
  if(own&&handoff.active()){
    const auto c=context();const auto actual_info=ubc::read_selection_info(memory,reinterpret_cast<std::uint32_t>(info));
    const ubc::HandoffOffer actual{actual_info.valid,cat::Invocation::info,actual_info.instance,actual_info.function,c.dialog.selected,rows.generation};
    if(callback_guarded(c,actual)){
      auto verified=proof(caller,true);verified.canonical_caller=verified.canonical_caller&&linked_step_info(actual_info,handoff.step());
      witness=handoff.enter_callback(dispatched_ticket,observation(c,false),actual,verified);
      allow=witness.decision!=ubc::HandoffDecision::block;
      final_effect=witness.decision==ubc::HandoffDecision::execute&&handoff.step()&&handoff.step()->is_effect();
    }
  }
  const bool previous_authorized=own?effect_authorized:false;if(own)effect_authorized=final_effect;
  SetLastError(error);if(allow)original_info(info);const auto after=GetLastError();
  if(own){
    flush_amount_diagnostic();
    if(witness.decision==ubc::HandoffDecision::execute){
      handoff.callback_return(witness);event(allow?"native_callback_returned":"effect_rejected",dispatched_ticket.offer.function);
      if(final_effect&&!effect_executed)result_guard.fail(ubc::RecipeResultIssue::contract);
    }else if(!allow)event("effect_rejected");
    effect_authorized=previous_authorized;--info_depth;
  }SetLastError(after);
}
void __fastcall observe_choice(void* manager,void* choice) {
  const auto error=GetLastError();const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());const bool own=own_thread();
  if(own)++choice_depth;
  ubc::HandoffCallback witness;bool allow=true;bool final_effect=false;bool removal_owned=false;
  if(own&&handoff.active()){
    const auto c=context();const auto actual_choice=ubc::read_choice(memory,reinterpret_cast<std::uint32_t>(manager),reinterpret_cast<std::uint32_t>(choice));
    const auto owner=ubc::read_selection_info(memory,c.dialog.info);
    const ubc::HandoffOffer actual{actual_choice.valid,cat::Invocation::choice,
      actual_choice.info.instance,actual_choice.function,actual_choice.index,rows.generation};
    if(callback_guarded(c,actual)){
      auto verified=proof(caller,false);
      verified.canonical_caller=verified.canonical_caller&&linked(owner)&&actual_choice.info.instance==owner.instance&&
        reinterpret_cast<std::uint32_t>(manager)==s::information_manager;
      witness=handoff.enter_callback(dispatched_ticket,observation(c,false),actual,verified);
      allow=witness.decision!=ubc::HandoffDecision::block;
      final_effect=witness.decision==ubc::HandoffDecision::execute&&handoff.step()&&handoff.step()->is_effect();
      if(final_effect){
        removal_owned=post_remove_choice.begin(memory,c.dialog,rows.generation,dispatched_ticket.id,
          reinterpret_cast<std::uint32_t>(choice),actual.function);
        if(!removal_owned){allow=false;handoff.cancel(ubc::HandoffIssue::offer);}
      }
    }
  }
  const bool previous_authorized=own?effect_authorized:false;if(own)effect_authorized=final_effect;
  SetLastError(error);
  __try {if(allow)original_choice(manager,choice);}
  __finally {if(removal_owned)post_remove_choice.clear();}
  const auto after=GetLastError();
  if(own){
    flush_amount_diagnostic();
    if(witness.decision==ubc::HandoffDecision::execute){
      handoff.callback_return(witness);event(allow?"native_callback_returned":"effect_rejected",dispatched_ticket.offer.function);
      if(final_effect&&!effect_executed)result_guard.fail(ubc::RecipeResultIssue::contract);
    }else if(!allow)event("effect_rejected");
    effect_authorized=previous_authorized;--choice_depth;
  }SetLastError(after);
}
void __fastcall observe_start(void* mob,void*,void* npc){
  const auto error=GetLastError();const bool own=own_thread();if(own)++station_depth;
  SetLastError(error);original_start(mob,npc);const auto after=GetLastError();
  if(own)--station_depth;SetLastError(after);
}
void __fastcall observe_end(void* mob,void*,void* npc,int flag){
  const auto error=GetLastError();const bool own=own_thread();if(own)++station_depth;
  const auto& request=handoff.request();
  const bool own_result=own&&result_guard.state()==ubc::RecipeResultState::waiting&&
    reinterpret_cast<std::uint32_t>(mob)==request.key.station&&reinterpret_cast<std::uint32_t>(npc)==request.key.hero;
  SetLastError(error);original_end(mob,npc,flag);const auto after=GetLastError();
  if(own){
    if(end_sequence!=UINT64_MAX)++end_sequence;else disabled=true;
    if(own_result){
      const auto fresh=context();
      if(fresh.identity_known&&fresh.world==request.key.world&&fresh.hero==request.key.hero&&fresh.station_known&&!fresh.station)
        result_guard.end_after({true,request.key.generation,end_sequence,frame,request.key.world,request.key.hero,request.key.station});
    }--station_depth;
  }SetLastError(after);
}
int __fastcall observe_menu_event(void* menu,void*,int input){
  const auto error=GetLastError();
  if(own_thread()&&ui.opened()&&menu==ui.object()){
    const auto result=ui.handle(session,input);SetLastError(error);return result;
  }
  SetLastError(error);
  return original_menu_event(menu,input);
}
int __fastcall observe_choice_event(void* callback,void*,int input){
  const auto error=GetLastError();
  if(own_thread()&&handoff.active()){
    const auto c=context();
    if(private_enter){
      private_enter=false;
      const auto current=observation(c,false);
      if(input==m::key_return&&reinterpret_cast<std::uint32_t>(callback)==dispatched_ticket.view+a::view_callback&&
        c.dialog.selected==dispatched_ticket.offer.row&&current.ready&&
        handoff.matches_pending(dispatched_ticket,current,resolve_offer(c,handoff.step()))){
        SetLastError(error);return original_choice_event(callback,input);
      }
      handoff.cancel(ubc::HandoffIssue::offer);SetLastError(error);return 1;
    }
    auto actual=observation(c,false,false);
    if(dispatched_ticket.view&&reinterpret_cast<std::uint32_t>(callback)==dispatched_ticket.view+a::view_callback)
      actual.view=dispatched_ticket.view;
    if(!foreign_dialog(c)&&reinterpret_cast<std::uint32_t>(callback)==dispatched_ticket.view+a::view_callback&&
      handoff.guards_input(actual)){
      if(result_guard.state()!=ubc::RecipeResultState::failed){
        if(input==m::key_escape)handoff.cancel();
        SetLastError(error);return 1;
      }
    }
  }
  SetLastError(error);
  return original_choice_event(callback,input);
}
void install() noexcept {
  std::array<std::uint8_t,a::frame_prefix_bytes.size()> prefix{};
  if(!memory.copy(a::frame_prefix,prefix.data(),prefix.size())||prefix!=a::frame_prefix_bytes)return;
  wchar_t path[32768]{};if(!w::sibling_path(module,path,L"UnionBatchCraftingRecipes.log"))return;
  log_file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(log_file==INVALID_HANDLE_VALUE)return;
  const char* header=execution_enabled?"recipe_version=1\r\nmode=recipe_amounts\r\n":"recipe_version=1\r\nmode=ui_only\r\n";
  DWORD written=0;WriteFile(log_file,header,static_cast<DWORD>(std::strlen(header)),&written,nullptr);log_bytes=written;
  const std::array<std::uintptr_t,sites.size()> destinations{{reinterpret_cast<std::uintptr_t>(&UbcFrameBridge),
    reinterpret_cast<std::uintptr_t>(&observe_add),reinterpret_cast<std::uintptr_t>(&observe_clear),
    reinterpret_cast<std::uintptr_t>(&observe_update),reinterpret_cast<std::uintptr_t>(&observe_resolve),
    reinterpret_cast<std::uintptr_t>(&observe_info),reinterpret_cast<std::uintptr_t>(&observe_choice),
    reinterpret_cast<std::uintptr_t>(&observe_start),reinterpret_cast<std::uintptr_t>(&observe_end),
    reinterpret_cast<std::uintptr_t>(&observe_menu_event),reinterpret_cast<std::uintptr_t>(&UbcBatchCallBridge),
    reinterpret_cast<std::uintptr_t>(&observe_choice_event)}};
  if(!hooks.prepare(destinations)){event("hooks_rejected");return;}
  UbcFrameContinuation=hooks.original(0);UbcAfterFrame=after_frame;
  original_add=reinterpret_cast<a::AddChoice>(hooks.original(1));original_clear=reinterpret_cast<a::FastObject>(hooks.original(2));
  original_update=reinterpret_cast<a::FastObject>(hooks.original(3));original_resolve=reinterpret_cast<a::ResolveInfo>(hooks.original(4));
  original_info=reinterpret_cast<Info>(hooks.original(5));original_choice=reinterpret_cast<a::FastObject>(hooks.original(6));
  original_start=reinterpret_cast<Start>(hooks.original(7));original_end=reinterpret_cast<End>(hooks.original(8));
  original_menu_event=reinterpret_cast<m::MenuHandleEvent>(hooks.original(9));
  UbcBatchCallOriginal=reinterpret_cast<void*>(hooks.original(10));original_choice_event=reinterpret_cast<a::HandleEvent>(hooks.original(11));
  bc::Config config;config.parser=reinterpret_cast<void*>(s::game_parser);
  config.signature=batch_signature;config.select=select_batch_call;config.writer=execute_amount;
  for(const auto& recipe:cat::recipes)for(const auto& callback:recipe.callbacks)if(callback.minimum_quantity){
    if(config.symbol_count==config.symbols.size()){event("hooks_rejected");return;}
    config.symbols[config.symbol_count++]=static_cast<std::int32_t>(callback.function->symbol);
  }
  if(!bc::configure(config)){event("hooks_rejected");return;}
  if(hooks.install(reinterpret_cast<std::uintptr_t>(&UbcFrameBridge))){InterlockedExchange(&installed,1);event("hooks_installed");}
  else event("hooks_rejected");
}
}
extern "C" unsigned __cdecl UbcSelectionProbeVersion(){return 3;}
extern "C" int __cdecl UbcSelectionProbeHooksInstalled(){return static_cast<int>(InterlockedCompareExchange(&installed,0,0));}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){
  if(reason==DLL_PROCESS_ATTACH){
    module=instance;const auto error=GetLastError();
    if(w::target_matches()){
      const bool preview=w::consume_marker(instance,L"UnionBatchCraftingRecipes.enable-once","RECIPE_UI_V1\n");
      execution_enabled=!preview&&w::consume_marker(instance,L"UnionBatchCraftingRecipes.enable-once","RECIPE_AMOUNTS_V1\n");
      if(preview||execution_enabled)install();
    }SetLastError(error);
  }else if(reason==DLL_PROCESS_DETACH&&log_file!=INVALID_HANDLE_VALUE){CloseHandle(log_file);log_file=INVALID_HANDLE_VALUE;}
  return TRUE;
}
