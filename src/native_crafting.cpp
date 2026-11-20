// SPDX-License-Identifier: MIT
#include <windows.h>
#include <intrin.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <new>
#include <memory>
#ifdef UBC_PLAYER_RELEASE
#include "player_engine_windows.hpp"
#endif
#include "ubc/frame_bridge.hpp"
#include "ubc/crafting_hook_sites.hpp"
#include "ubc/batch_call_bridge.hpp"
#include "ubc/batch_amount_call.hpp"
#include "ubc/native_crafting_ui.hpp"
#include "ubc/native_crafting_ammo.hpp"
#include "ubc/crafting_catalog.hpp"
#include "ubc/cool_listed_input.hpp"
#include "ubc/saw_actor_preview.hpp"
#include "ubc/crafting_handoff.hpp"
#include "ubc/crafting_info_effect.hpp"
#include "ubc/crafting_result.hpp"
#include "ubc/station_end_contract.hpp"
#include "ubc/selection_snapshot.hpp"
#include "ubc/post_remove_choice.hpp"
#include "ubc/windows_probe.hpp"
#include "ubc/windows_module_identity.hpp"
#include "ubc/crafting_alchemy_extra_reader.hpp"
#include "tobacco_writer.hpp"
#include "ubc/alchemy_extra_end_snapshot.hpp"
#include "alchemy_extra_writer.hpp"
#include "ubc/crafting_spirit_projection.hpp"
#include "ubc/spirit_end_snapshot.hpp"
#include "spirit_writer.hpp"
#include "ubc/crafting_bow_projection.hpp"
#include "ubc/bow_menu_checks.hpp"
#include "ubc/crafting_bow_execution.hpp"
#include "ubc/crafting_cool_projection.hpp"
#include "ubc/cool_direct_contract.hpp"
#include "cool_predebit_bridge.hpp"
#include "ubc/cool_predebit_journal.hpp"
#include "ubc/cool_predebit_projection.hpp"
#include "ubc/cool_native_path.hpp"
#include "ubc/cool_destroy_observation.hpp"
#include "ubc/cool_destroy_hand_diagnostic.hpp"
#include "ubc/generated_blade_hand_shape.hpp"
#include "ubc/windows_destroy_remove_callsite.hpp"
#include "ubc/crafting_grind_projection.hpp"
#include "ubc/grind_destroy_observation.hpp"
#include "ubc/grind_menu_metadata.hpp"
#include "ubc/grind_preview_batch.hpp"
#include "ubc/grind_result_diagnostic.hpp"
#include "ubc/grind_preview_measurement.hpp"

namespace {
namespace a=ubc::selection_abi;
namespace s=ubc::station_abi;
namespace m=ubc::native_menu;
namespace w=ubc::windows_probe;
namespace cat=ubc::crafting_catalog;
namespace basecat=ubc::recipe_catalog;
namespace scat=ubc::station_recipe_catalog;
namespace snav=ubc::station_recipe_navigation;
namespace acat=ubc::anvil_recipe_catalog;
namespace art=ubc::anvil_recipe_runtime;
namespace anav=ubc::anvil_recipe_navigation;
namespace ocat=ubc::ore_recipe_catalog;
namespace ort=ubc::ore_recipe_runtime;
namespace bcat=ubc::bow_recipe_catalog;
namespace brt=ubc::bow_recipe_runtime;
namespace bnav=ubc::bow_recipe_navigation;
namespace bitem=ubc::bow_item_runtime;
namespace ccat=ubc::cool_recipe_catalog;
namespace crt=ubc::cool_recipe_runtime;
namespace cnav=ubc::cool_recipe_navigation;
namespace cpath=ubc::cool_native_path;
namespace ddispatch=ubc::cool_destroy_dispatch;
namespace cdirect=ubc::cool_direct_contract;
namespace cpre=ubc::cool_predebit;
namespace cadmission=cpre::admission;
namespace cwriter=ubc::cool_predebit_bridge;
namespace cdestroy=ubc::cool_destroy_observation;
namespace chdiag=ubc::cool_destroy_hand_diagnostic;
namespace bhshape=ubc::generated_blade_hand_shape;
namespace cabi=ubc::cool_destroy_abi;
namespace rcall=ubc::destroy_remove_callsite;
namespace shcat=ubc::grind_recipe;
namespace shrt=ubc::grind_recipe_runtime;
namespace shnav=ubc::grind_recipe_navigation;
namespace shitem=ubc::grind_item_runtime;
namespace shdestroy=ubc::grind_destroy_observation;
namespace shpreview=ubc::grind_preview_batch;
namespace xcat=ubc::alchemy_recipe_catalog;
namespace tobacco=ubc::tobacco;
namespace xnav=ubc::alchemy_extra_navigation;
namespace xdep=ubc::alchemy_extra_dependencies;
namespace xwriter=ubc::alchemy_extra_writer;
namespace xdebit=ubc::alchemy_flask_debit;
namespace xbridge=ubc::alchemy_flask_debit_bridge;
namespace gcat=ubc::spirit_recipe_catalog;
namespace grt=ubc::spirit_recipe_runtime;
namespace gwriter=ubc::spirit_writer;
namespace gdebit=ubc::spirit_debit;
namespace ammo=ubc::native_crafting_ammo;
namespace uidiag=ubc::crafting_ui_diagnostics;
namespace gready=ubc::grind_ui_readiness;
namespace ap=ubc::ammo_presentation;
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
bool grind_measurement_mode=false;
bool campfire_observation_mode=false;
bool bow_diagnosis=false,bow_comparison=false;
bool cool_context_mode=false;
cpre::inspection::Reports cool_check_reports;
unsigned grind_measurement_case=0;
std::uint32_t grind_measurement_whole=0;
unsigned frame_depth=0,update_depth=0,info_depth=0,choice_depth=0,station_depth=0,collection_depth=0;
unsigned resolved_read_depth=0;
std::uint64_t frame=0,generation=0,end_sequence=0;
std::size_t log_bytes=0,log_events=0;
ubc::SelectionInfoRows rows;
bool choice_collection=false;
std::size_t choice_count=0;
std::array<std::int32_t,256> choice_functions{};
ubc::RecipeCatalogueReader catalogue_reader;
ubc::StationRecipeCatalogueReader station_catalogue_reader;
ubc::AnvilRecipeCatalogueReader anvil_catalogue_reader;
ubc::OreRecipeCatalogueReader ore_catalogue_reader;
ubc::station_recipe_snapshot_detail::HandWorkspace result_hands,end_hands;
ubc::PackedInventoryWorkspace result_workspace;
ubc::CraftingSession session;
ubc::NativeCraftingUi ui;
uidiag::Trace ui_diagnostic;
gready::Trace grind_ui_readiness;
gready::QuantityObservation grind_quantity_observation;
bool grind_ui_trace_active=false;
ubc::CraftingHandoff handoff;
ubc::CraftingResult result_guard;
ubc::AmountPatch amount_patch;
ubc::PostRemoveChoice post_remove_choice;
ubc::CraftingHandoffTicket dispatched_ticket;
ubc::StationRecipeHands handed_hands;
ubc::NativeCraftingSuccess pending_success{};
bool success_pending=false,ammo_job=false,ammo_install_attempted=false,ammo_notice_ready=false,ammo_notice_acked=false;
std::uint64_t world_epoch=0;
std::uint32_t observed_world=0,observed_hero=0;
ap::Scope active_ammo_scope{};
ap::Context active_ammo_context{};
std::uint32_t remembered_world=0,remembered_hero=0,remembered_station=0;
cat::RecipeId remembered_recipe=UINT32_MAX;
std::uint64_t rendered_generation=0,request_ui_generation=0;
ubc::CraftingKey request_ui_key{};
std::uint32_t borrowed_view=0,original_station=0;
bool private_enter=false,effect_authorized=false,effect_executed=false,recover_info=false;
bool failure_notified=false,failure_open_rejected_logged=false;
ULONGLONG handoff_started=0,result_started=0;
int last_frame_state=-1,last_handoff_state=-1,last_capture_issue=-1,last_failure_state=-1,last_end_path_issue=-1;
ubc::windows_probe::LoadedModuleIdentity end_zbug_module,end_union_module;
bool end_compatibility_attempted=false;
ubc::zbug_end_compat::Binding end_compatibility_binding{};
ubc::windows_probe::LoadedModuleIdentity extra_zpe_module,extra_crt_module;
ubc::windows_probe::LoadedModuleIdentity bow_renderer_module;
ubc::windows_probe::LoadedModuleIdentity bow_locator_module;
bool bow_locator_bound=false;
void log_plugin_version(const char* name,const ubc::windows_probe::LoadedModuleIdentity& module) noexcept;
ubc::BowRecipeCatalogueReader bow_catalogue_reader;
std::optional<bitem::Reader> bow_item_reader;
bool bow_initialized=false;
std::optional<ubc::CraftingAlchemyExtraReader> extra_catalogue_reader;
std::optional<xwriter::Executor> extra_executor;
bool extra_initialized=false,extra_job=false,extra_normal_end=false;
std::uint64_t extra_serial=0,extra_end_frame=0;
xdebit::Job extra_active_job{};
xdebit::EndWitness extra_end{};
bc::WriterScope* extra_writer_scope=nullptr;
tobacco::Reader tobacco_reader;
tobacco::OwnerTicket tobacco_owner;
std::uint32_t tobacco_owner_call=0;
int tobacco_choice_call=0;
volatile LONG tobacco_unpaid_page=0;
bool tobacco_owner_opened=false,tobacco_clean_rejection=false,tobacco_writer_entered=false;
ubc::windows_probe::LoadedModuleIdentity tobacco_locator;
ubc::bow_item_native::BowDispatch tobacco_targets;
ubc::cool_predebit::check::Report tobacco_report;
std::optional<xdep::ListReader> tobacco_list;

std::optional<ubc::SpiritCraftingReader> spirit_catalogue_reader;
std::optional<gwriter::Executor> spirit_executor;
bool spirit_job=false,spirit_normal_end=false;
std::uint64_t spirit_end_frame=0;
gdebit::Job spirit_active_job{};
gdebit::EndWitness spirit_end{};
bc::WriterScope* spirit_writer_scope=nullptr;
ubc::CoolRecipeCatalogueReader cool_catalogue_reader;
std::optional<cpre::reader::Reader> cool_predebit_reader;
cpre::Tracker cool_predebit_tracker;
std::optional<cwriter::Executor> cool_predebit_executor;
cpre::DestroyJournal cool_predebit_journal;
const bc::WriterScope* cool_writer_scope=nullptr;
std::uint64_t cool_fence_token=0;
#ifndef UBC_PLAYER_RELEASE
ubc::windows_probe::LoadedModuleIdentity cool_predebit_engine;
#endif
ubc::PackedInventoryWorkspace cool_inventory;
ubc::station_recipe_snapshot_detail::HandWorkspace cool_hands;
ubc::windows_destroy_remove_callsite::Adapter cool_remove_observer;
bool grind_probe_failed=false,cool_install_attempted=false,cool_destroy_installed=false,cool_ready=false,cool_job=false,cool_receipt_acked=false;
unsigned cool_destroy_depth=0;
std::uint64_t cool_serial=0,cool_job_floor=0;
cdirect::Entry cool_active_entry{};
cdestroy::Observation cool_destroy_call{};
cdestroy::Binding cool_destroy_binding{};
chdiag::Record cool_destroy_hand_diagnostic{};
ubc::GrindRecipeCatalogueReader grind_catalogue_reader;
alignas(ubc::GrindRecipeCatalogueCapture) std::array<std::uint8_t,sizeof(ubc::GrindRecipeCatalogueCapture)> grind_capture_storage{};
bool grind_capture_leased=false;
struct GrindPreviewDiagnostic {std::uint64_t generation=0;int issue=-1;std::uint32_t address=0;};
std::array<GrindPreviewDiagnostic,shcat::recipes.size()> grind_preview_diagnostic{};
std::optional<shitem::Reader> grind_item_reader;
std::optional<shpreview::Reader> grind_preview_reader;
shpreview::Publication grind_preview_publication;
std::uint64_t grind_preview_serial=0,grind_preview_input_serial=0;
unsigned grind_preview_input_depth=0;
bool grind_preview_overflow=false;
struct GrindPreviewFailureDiagnostic {std::uint64_t generation=0;int stage=0,issue=-1;ubc::GrindCheck check{};} grind_preview_failure;
ubc::windows_probe::LoadedModuleIdentity grind_renderer_module;
ubc::windows_probe::LoadedModuleIdentity grind_locator_module;
bool grind_locator_bound=false;
bool grind_initialized=false,grind_job=false,grind_original_returned=false,grind_skill_log_pending=false;
std::uint64_t grind_serial=0,grind_job_floor=0;
shdestroy::Prepared grind_prepared{};
shdestroy::Observation grind_destroy_call{};
shdestroy::Binding grind_destroy_binding{};
ubc::CraftingGrindValues grind_skill_after_values{};
struct GrindChoiceLease {
  std::uint32_t owner=0,thread=0,game=0;
  unsigned depth=0;
  std::uint64_t frame_number=0;
  ubc::CraftingKey key{};
  ubc::CraftingHandoffTicket ticket{};
  shnav::PageEvidence page{};
} grind_choice_lease;
std::optional<ddispatch::Reader> destroy_dispatch_reader;
using InfoEffectLease=ubc::CraftingInfoEffectLease;
InfoEffectLease info_effect_lease;
cadmission::InfoLease info_lease_owner;
struct AmountDiagnostic {
  bool pending=false,entered=false,restored=false;
  int rejected=0,duration=0,symbol=0,quantity=0;
  cpre::Rejection predebit{};
  std::uint64_t cool_info_token=0;
  ubc::GrindCheck grind_check{};
} amount_diagnostic;
enum class AmmoDiagnosticPhase : unsigned {
  close_context=1,close_effect=2,postcall_health=4,frame_health=8,timeout=16
};
struct AmmoDiagnostic {
  std::uint64_t job=0;
  std::uint32_t callback=0;
  unsigned phases=0,reported=0;
} ammo_diagnostic;

using Start=void(__thiscall*)(void*,void*);
using End=void(__thiscall*)(void*,void*,int);
using Info=void(__thiscall*)(void*);
using Destroy=int(__thiscall*)(void*,void*);
Destroy original_destroy=nullptr;
constexpr std::array<s::HookSite,1> cool_sites{{
  {cabi::destroy_entry,6,6,{{0x64,0xa1,0,0,0,0}}}
}};
w::Hooks<s::HookSite,1> cool_hooks(cool_sites);
a::FastObject original_update=nullptr,original_choice=nullptr,original_clear=nullptr;
a::AddChoice original_add=nullptr;
a::ResolveInfo original_resolve=nullptr;
Info original_info=nullptr;
Start original_start=nullptr;
End original_end=nullptr;
m::MenuHandleEvent original_menu_event=nullptr;
a::HandleEvent original_choice_event=nullptr;
using ubc::crafting_hook_sites::sites;
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
void event(const char* name,int detail=0,int quantity=0,const char* fields="") noexcept {
  if(log_file==INVALID_HANDLE_VALUE||log_events>=(grind_measurement_mode?180000u:10000u)||
     log_bytes>=(grind_measurement_mode?15u:4u)*1024u*1024u)return;
  char line[352]{};
  const int size=std::snprintf(line,sizeof(line),"{\"event\":\"%s\",\"frame\":%llu,\"detail\":%d,\"quantity\":%d%s}\r\n",
    name,static_cast<unsigned long long>(frame),detail,quantity,fields);
  if(size<=0||static_cast<std::size_t>(size)>=sizeof(line))return;
  DWORD written=0;WriteFile(log_file,line,static_cast<DWORD>(size),&written,nullptr);
  log_bytes+=written;++log_events;
}
void log_grind_check(const ubc::GrindCheck& check, int subject) noexcept {
  if(!check.field)return;
  char fields[176]{};
  check.format(fields);
  event("grind_check_rejected",subject,check.symbol,fields);
}
// Diagnostics use the existing check result. The preview runs every frame.
void log_bow_check(const char* field, int expected, int actual, int subject=0, int quantity=0,
    std::uint32_t address=0) noexcept {
  if(expected==actual)return;
  ubc::GrindCheck check;
  check.equal(field,expected,actual);
  check.symbol=static_cast<int>(address);
  static ubc::GrindCheck previous;
  static int last_subject=-1,last_quantity=-1;
  static std::uint64_t last_frame=0;
  const bool repeated=subject==last_subject&&quantity==last_quantity&&check.same(previous)&&frame-last_frame<=1;
  previous=check;last_subject=subject;last_quantity=quantity;last_frame=frame;
  if(repeated)return;
  char fields[176]{};
  check.format(fields);
  if(address){
    char detail[224]{};
    std::snprintf(detail,sizeof(detail),"%s,\"address\":%u",fields,address);
    event("bow_check_rejected",subject,quantity,detail);
  }else event("bow_check_rejected",subject,quantity,fields);
}
void bow_menu_event(const char* name,const char* field,const char* wanted,const char* observed,
                    int ok,int subject,int symbol) noexcept {
  char fields[264]{};
  std::snprintf(fields,sizeof(fields),",\"field\":\"%s\",\"soll\":\"%s\",\"ist\":\"%s\",\"ok\":%s",
                field,wanted,observed,ok<0?"null":ok?"true":"false");
  event(name,subject,symbol,fields);
}
void bow_menu_value(const char* field,const char* wanted,const char* observed,int ok,int subject,int symbol) noexcept {
  bow_menu_event("bow_menu_value",field,wanted,observed,ok,subject,symbol);
}
void publish_window_sample() noexcept;
void bow_menu_window(const char* field,const char* wanted,const char* observed,int ok,int subject,int symbol) noexcept {
  publish_window_sample();
  bow_menu_event("bow_menu_window",field,wanted,observed,ok,subject,symbol);
}
struct BowRejection {
  const char* field=nullptr;
  int subject=0,symbol=0;
  char wanted[80]{},observed[80]{};
  std::uint64_t last=0;
};
std::array<BowRejection,256> bow_rejections{};
std::size_t bow_rejection_count=0;
void bow_menu_rejected(const char* field,const char* wanted,const char* observed,int ok,int subject,int symbol) noexcept {
  for(std::size_t i=0;i<=bow_rejection_count&&i<bow_rejections.size();++i){
    auto& r=bow_rejections[i];
    if(i<bow_rejection_count&&(r.subject!=subject||r.symbol!=symbol||std::strcmp(field,r.field)))continue;
    const bool repeat=i<bow_rejection_count&&frame-r.last<=1&&
        !std::strcmp(wanted,r.wanted)&&!std::strcmp(observed,r.observed);
    r.last=frame;
    if(repeat)return;
    if(i==bow_rejection_count)++bow_rejection_count;
    r.field=field;r.subject=subject;r.symbol=symbol;
    std::snprintf(r.wanted,sizeof(r.wanted),"%s",wanted);
    std::snprintf(r.observed,sizeof(r.observed),"%s",observed);
    break;
  }
  bow_menu_event("bow_menu_rejected",field,wanted,observed,ok,subject,symbol);
}
void ui_diagnostic_events(const uidiag::Batch& batch) noexcept {
  for(std::size_t i=0;i<batch.count;++i){const auto& row=batch.records[i];event(row.name,row.detail,row.quantity);}
}
void grind_readiness_events(const gready::Batch& batch) noexcept {
  for(std::size_t i=0;i<batch.count;++i){const auto& row=batch.records[i];event(row.name,row.detail,row.quantity);}
}
void close_grind_readiness() noexcept {
  if(grind_ui_trace_active){grind_readiness_events(grind_ui_readiness.close());grind_ui_trace_active=false;}
}
void note_ammo_diagnostic(AmmoDiagnosticPhase phase) noexcept {
  // While the patch is active, record only our own POD values. No status reads or I/O.
  if(ammo_job&&ammo_diagnostic.job)ammo_diagnostic.phases|=static_cast<unsigned>(phase);
}
int diagnostic_word(std::uint32_t value) noexcept {
  // Numeric signed32 log format. The original uint32 can be reconstructed without loss.
  return value<=INT32_MAX?static_cast<int>(value):-1-static_cast<int>(UINT32_MAX-value);
}
void flush_ammo_diagnostic(bool after_restore) noexcept {
  if(!own_thread()||amount_patch.active()||(!after_restore&&
    (frame_depth!=1||update_depth||info_depth||choice_depth||station_depth||collection_depth)))return;
  const auto phases=ammo_diagnostic.phases&~ammo_diagnostic.reported;if(!phases)return;
  const auto saved=GetLastError();const auto status=ammo::status();
  const auto& tracker=status.tracker;const auto& capture=status.capture;
  const int flags=(status.configured?1:0)|(status.install_attempted?2:0)|(status.installed?4:0)|
    (status.healthy?8:0)|(status.owner_thread?16:0)|(status.effect_entered?32:0)|
    (tracker.blocked?64:0)|(tracker.effect_closed?128:0)|(tracker.result_confirmed?256:0)|
    (tracker.notice_offered?512:0)|(tracker.notice_registered?1024:0);
  const int capture_flags=(capture.reader_seen?1:0)|(capture.reader_valid?2:0)|
    (capture.job==ammo_diagnostic.job?4:0)|(capture.scope_token==ammo_diagnostic.job?8:0)|
    (capture.callback==ammo_diagnostic.callback?16:0);
  event("ammo_diag_phase",static_cast<int>(phases),diagnostic_word(ammo_diagnostic.callback));
  event("ammo_diag_job",diagnostic_word(static_cast<std::uint32_t>(ammo_diagnostic.job)),
    diagnostic_word(static_cast<std::uint32_t>(ammo_diagnostic.job>>32u)));
  event("ammo_diag_fault",static_cast<int>(status.fault),static_cast<int>(status.runtime_issue));
  event("ammo_diag_tracker",static_cast<int>(tracker.stage),static_cast<int>(tracker.issue));
  event("ammo_diag_flags",flags,capture_flags);
  event("ammo_diag_capture",diagnostic_word(capture.attempts),diagnostic_word(capture.reads));
  event("ammo_diag_reader",diagnostic_word(capture.accepted),static_cast<int>(capture.first_issue));
  event("ammo_diag_pc",diagnostic_word(capture.vm_pc),diagnostic_word(capture.expected_pc));
  event("ammo_diag_caller",diagnostic_word(capture.caller),diagnostic_word(capture.expected_caller));
  ammo_diagnostic.reported|=phases;SetLastError(saved);
}
void observe_ammo_frame_health() noexcept {
  if(ammo_job){
    if(!ammo::healthy()){
      note_ammo_diagnostic(AmmoDiagnosticPhase::frame_health);
      result_guard.fail(ubc::CraftingResultIssue::contract);
    }else if(success_pending&&!ammo_notice_ready&&GetTickCount64()-result_started>30000){
      note_ammo_diagnostic(AmmoDiagnosticPhase::timeout);
      result_guard.fail(ubc::CraftingResultIssue::contract);
    }
  }
  flush_ammo_diagnostic(false);
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
#ifdef UBC_PLAYER_RELEASE
ubc::player_engine::Result player_image;
static_assert(static_cast<unsigned>(cat::Station::saw) == 3 &&
              static_cast<unsigned>(cat::Station::grind) == 8);
std::array<bool, cat::recipe_count> player_recipes{};
std::array<bool, 9> checked_stations{}, available_stations{};

bool player_recipe_matches(const cat::Descriptor& recipe) noexcept {
  const auto parser = s::game_parser;
  switch (recipe.route) {
    case cat::Route::base: return ubc::recipe_runtime::verify(memory, parser, *recipe.base).matches;
    case cat::Route::station: return ubc::station_recipe_runtime::verify(memory, parser, *recipe.station_descriptor).matches;
    case cat::Route::alchemy_extra: return ubc::alchemy_extra_execution::verify_recipe(memory, *recipe.alchemy_extra).valid;
    case cat::Route::anvil: return art::verify(memory, parser, *recipe.anvil).matches;
    case cat::Route::ore: return ort::verify(memory, parser, *recipe.ore).matches;
    case cat::Route::spirit: return grt::verify_recipe(memory, parser, grt::assemble(*recipe.spirit)).valid;
    case cat::Route::bow: return brt::verify_recipe(memory, parser, *recipe.bow, brt::assemble(*recipe.bow)).valid;
    case cat::Route::cool: return crt::verify_recipe(memory, parser, *recipe.cool).valid;
    case cat::Route::grind: return shrt::verify_recipe(memory, parser, *recipe.grind).valid;
    case cat::Route::tobacco: {
      ubc::GrindCheck check;
      return tobacco::verify_script(memory, check);
    }
  }
  return false;
}
bool player_scripts_match(cat::Station station) noexcept {
  const auto index = static_cast<unsigned>(station);
  if (!own_thread()) return false;
  if (!checked_stations[index]) {
    if (frame_depth != 1 || update_depth || info_depth || choice_depth ||
        station_depth || collection_depth || handoff.active() || amount_patch.active()) return false;
    for (std::size_t i = 0; i < cat::recipes.size(); ++i) {
      const auto& recipe = cat::recipes[i];
      if (recipe.station != station) continue;
      player_recipes[i] = player_recipe_matches(recipe);
      if (player_recipes[i]) available_stations[index] = true;
      else event("script_contract_rejected", static_cast<int>(recipe.id), index);
    }
    checked_stations[index] = true;
  }
  return available_stations[index];
}
#endif
struct Context {
  bool valid=false,identity_known=false,station_known=false,ownership_known=false;
  std::uint32_t game=0,world=0,hero=0,station=0;
  cat::Station kind=cat::Station::stove;
  ubc::SelectionDialog dialog{};
};
Context borrowed_context;
// The observer context persists even after a failed batch. Engine addresses
// bind the scope. They do not represent fresh RAM reads at each probe.
// Global checks bracket the context and the modules actually held.
struct GrindPreviewScope {
  Context bound{};
  std::uint64_t serial=0;
  bool active=false,failed=false;
} grind_preview_scope;
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
    integer(c.station,0x1f8)!=1)return c;
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
  else if(production.value==1&&name_is(c.station,0x1e0,acat::station.on_state)&&
    name_is(c.station,0x1b8,acat::station.scheme)&&name_is(c.station,0x1a4,acat::station.use_with_item)&&
    name_is(c.station,0x1cc,acat::station.condition_name))c.kind=cat::Station::anvil;
  else if(production.value==16&&name_is(c.station,0x1e0,bcat::station.on_state)&&
    name_is(c.station,0x1b8,bcat::station.scheme)&&name_is(c.station,0x1a4,bcat::station.use_with_item)&&
    name_is(c.station,0x1cc,bcat::station.condition_name)&&integer(c.station,0x200)==bcat::station.rewind)
    c.kind=cat::Station::bow;
  else if(production.value==13&&name_is(c.station,0x1e0,ccat::station.on_state)&&
    name_is(c.station,0x1b8,ccat::station.scheme)&&name_is(c.station,0x1a4,ccat::station.use_with)&&
    name_is(c.station,0x1cc,ccat::station.condition)&&integer(c.station,0x200)==ccat::station.rewind)
    c.kind=cat::Station::cool;
  else if(production.value==11&&name_is(c.station,0x1e0,"SMITHSHARP")&&
    name_is(c.station,0x1b8,"BSSHARP")&&name_is(c.station,0x1a4,"")&&name_is(c.station,0x1cc,"")&&
    integer(c.station,0x200)==0)c.kind=cat::Station::grind;
  else {
    bool found=false;
    for(const auto& profile:scat::stations)if(production.value==profile.production_context&&
      name_is(c.station,0x1e0,profile.on_state)&&name_is(c.station,0x1b8,profile.scheme)&&
      name_is(c.station,0x1a4,profile.use_with_item)&&name_is(c.station,0x1cc,profile.condition_name)){
      c.kind=profile.station==scat::Station::leatherwork?cat::Station::leather:
        profile.station==scat::Station::saw?cat::Station::saw:cat::Station::forge;
      found=true;break;
    }
    if(!found)return c;
  }
  // SMITHCOOL/rewind0 keeps the original INFO selection open at S1 with target
  // 0. The binding was observed live before and after a wait of >45s (0.0.20),
  // with the generated left blade still present. Only this fully recognized
  // profile may use target 0. All previous stations still require target 1.
  const auto target=integer(c.station,0x1fc);
  if(target!=1&&!(c.kind==cat::Station::cool&&target==0))return c;
#ifdef UBC_PLAYER_RELEASE
  if (player_image.failure[static_cast<unsigned>(c.kind)] || !player_scripts_match(c.kind)) return c;
#endif
  c.valid=true;return c;
}
bool __cdecl read_ammo_context(void*,ap::Context& out) {
  out={};std::uint32_t game=0,world=0,hero=0;
  if(!own_thread()||!world_epoch||!read(a::game_pointer,game)||!game||
    integer(game,0)!=static_cast<std::int32_t>(a::game_vtable)||!field(game,a::game_world,world)||
    !read(s::hero_pointer,hero)||!world||!hero||world!=observed_world||hero!=observed_hero)return false;
  const auto symbol=rt::read_symbol_address(memory,s::game_parser,ubc::lhiver::hero_symbol);std::uint32_t bound=0;
  if(!symbol.valid||!field(symbol.address,0x1c,bound)||bound!=hero)return false;
  out={world_epoch,GetCurrentThreadId(),s::game_parser,game,world,hero};return true;
}
bool __cdecl read_ammo_scope(void*,ap::Scope& out) {
  out={};if(!own_thread())return false;
  out=active_ammo_scope;
  if(out.active&&(!amount_patch.active()||!effect_authorized||info_depth+choice_depth!=1||
    !native_update||update_depth!=1||station_depth||collection_depth))return false;
  return true;
}
bool adapter_failed() noexcept {
  return result_guard.state()==ubc::CraftingResultState::failed||bc::diagnostics().writes_disabled||
    (ammo_job&&!ammo::healthy())||(extra_job&&extra_executor&&extra_executor->diagnostics().disabled)||
    (spirit_job&&spirit_executor&&spirit_executor->diagnostics().disabled);
}
ubc::CraftingKey key(const Context& c) noexcept {
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
const basecat::Owner* owner_for(std::int32_t instance) noexcept {
  if(instance==tobacco::owner.instance)return &tobacco::owner;
  if(const auto* owner=nav::owner_for(instance))return owner;
  for(const auto& recipe:basecat::recipes)for(const auto& owner:recipe.owners)
    if(owner.instance==instance)return &owner;
  if(const auto* owner=snav::owner_for(instance))return owner;
  if(const auto* owner=xnav::owner_for(instance))return owner;
  for(const auto& recipe:gcat::recipes)if(recipe.owner.instance==instance)return &recipe.owner;
  for(const auto& recipe:acat::recipes)if(recipe.owner.instance==instance)return &recipe.owner;
  for(const auto& recipe:ocat::recipes)if(recipe.owner.instance==instance)return &recipe.owner;
  for(const auto& recipe:bcat::recipes)if(recipe.owner.instance==instance)return &recipe.owner;
  if(bcat::station.category.instance==instance)return &bcat::station.category;
  if(bcat::station.end.instance==instance)return &bcat::station.end;
  for(const auto* owner:{&ccat::single,&ccat::triple,&ccat::end_owner})if(owner->instance==instance)return owner;
  for(const auto& owner:ubc::grind_menu_metadata::owners)if(owner.instance==instance)return &owner;
  for(const auto& item:acat::navigation)if(item.owner.instance==instance)return &item.owner;
  if(acat::station.end.instance==instance)return &acat::station.end;
  return nullptr;
}
bool linked(const ubc::SelectionInfo& info) noexcept {
  const auto* owner=owner_for(info.instance);
  return info.valid&&owner&&info.number==owner->number&&info.npc==owner->npc&&
    info.important==static_cast<int>(owner->important)&&info.trade==static_cast<int>(owner->trade)&&
    info.permanent==static_cast<int>(owner->permanent)&&info.condition==static_cast<int>(owner->condition->symbol)&&
    info.function==static_cast<int>(owner->information->symbol);
}
bool linked_step_info(const ubc::SelectionInfo& info,const ubc::CraftingStep* step) noexcept {
  if(!linked(info))return false;
  if(!info.first_choice)return true;
  // OnInfo calls the original information function even with choices
  // remaining. Only these six bound owner infos start by calling
  // ClearChoices on their own owner and then rebuild the entire selection.
  // Direct INFO effects and START/STOP still require an empty list.
  if(!step||step->kind!=ubc::CraftingStepKind::open_owner||step->invocation!=basecat::Invocation::info||
    step->owner_instance!=info.instance||step->function_symbol!=info.function)return false;
  if(info.instance==20399)return step->before==ubc::crafting_page(nav::Page::tobacco_info)&&
    step->after.kind==ubc::CraftingPageKind::tobacco_choices&&step->function_symbol==20401;
  for(const auto& route:nav::routes)if(route.owner->instance==info.instance&&
    route.owner->information->symbol==static_cast<std::uint32_t>(info.function)&&
    step->before==ubc::crafting_page(route.category)&&step->after==ubc::crafting_page(route.choices))return true;
  for(const auto& route:snav::routes)if(route.owner->instance==info.instance&&
    route.owner->information->symbol==static_cast<std::uint32_t>(info.function)&&
    step->before==ubc::crafting_page(route.root)&&step->after==ubc::crafting_page(route.choices))return true;
  for(const auto& recipe:acat::recipes)if(recipe.owner.instance==info.instance&&
    recipe.owner.information->symbol==static_cast<std::uint32_t>(info.function)&&
    step->before==ubc::crafting_page(anav::Page::common)&&step->after==ubc::crafting_page(anav::page_for(recipe.id)))return true;
  if(info.instance==20957&&info.function==20959&&
    ubc::crafting_grind_page(step->before).page==shnav::Page::info&&
    ubc::crafting_grind_page(step->after).page==shnav::Page::categories)return true;
  return false;
}
std::uint32_t current_info_effect(const Context& c) noexcept {
  // Never use a pointer from a previous owner or choice state, or grant access
  // from a nested call or a different thread. Production may already be 0 after
  // the original call. The physical job context and our callback remain bound.
  if(!own_thread()||!effect_authorized||!native_update||update_depth!=1||info_depth!=1||
    choice_depth||station_depth||collection_depth||resolved_read_depth||!c.identity_known||!c.station_known)return 0;
  const auto& lease=info_effect_lease;
  const cadmission::InfoIdentity identity{{generation,GetCurrentThreadId(),s::game_parser,c.game,c.world,c.hero,c.station},
    frame,lease.actual_info,s::information_manager};
  return lease.current(memory,info_lease_owner,identity,handoff,dispatched_ticket,rows,c.dialog,owner_for(lease.ticket.offer.owner));
}
bool navigation_contract() noexcept {
  std::array<ubc::lhiver::SymbolBinding,basecat::common_bindings.size()+nav::flag_bindings.size()> bindings{};
  std::size_t count=0;
  for(const auto& b:basecat::common_bindings)bindings[count++]=b;
  for(const auto& b:nav::flag_bindings)bindings[count++]=b;
  const ubc::lhiver::Profile profile{layout_profile.parser_address,layout_profile.symbol_count,
    layout_profile.code_length,layout_profile.code_sha256,nav::functions.data(),nav::functions.size(),bindings.data(),count};
  if(!rt::verify_local_contract(memory,s::game_parser,profile).matches)return false;
  const ubc::lhiver::Profile extra{layout_profile.parser_address,layout_profile.symbol_count,
    layout_profile.code_length,layout_profile.code_sha256,xnav::start_functions.data(),xnav::start_functions.size(),bindings.data(),count};
  return rt::verify_local_contract(memory,s::game_parser,extra).matches;
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
bool bow_candidate(const Context& c) noexcept {
#ifdef UBC_PLAYER_RELEASE
  if (!c.valid) return false;
#endif
  return c.station&&(c.valid?c.kind==cat::Station::bow:
    name_is(c.station,0x1e0,bcat::station.on_state)||name_is(c.station,0x1b8,bcat::station.scheme)||
    name_is(c.station,0x120,"MOBNAME_BOW_WBENCH"));
}
void bow_station_name(ubc::BowMenuTrace& trace,const Context& c,const char* field_name,
                      unsigned offset,const char* expected) noexcept {
  std::uint32_t data=0;int length=-1;std::array<char,64> value{};
  if(!field(c.station,offset+8,data)||!field(c.station,offset+12,length)||length<0||length>=64||
     (length&&(!memory.copy(data,value.data(),length+1)||value[length]))){
    trace.check(field_name,expected,"unlesbar",false);return;
  }
  const bool matches=std::strlen(expected)==static_cast<unsigned>(length)&&!std::memcmp(value.data(),expected,length);
  for(int i=0;i<length;++i)if(value[i]<32||value[i]>126||value[i]=='"'||value[i]=='\\')value[i]='?';
  trace.check(field_name,expected,value.data(),matches);
}
void bow_station_checks(const Context& c,ubc::BowMenuTrace& trace) noexcept {
  trace.equal("station.game_world_hero",1,c.identity_known&&c.world&&c.hero);
  trace.equal("station.interact_mob_read",1,c.station_known);
  trace.equal("station.dialog_valid",1,c.dialog.valid);
  trace.equal("station.owns_hero",1,station_owns(c.station,c.hero));
  trace.equal("station.state",1,integer(c.station,0x1f4));
  trace.equal("station.state_count",1,integer(c.station,0x1f8));
  trace.equal("station.target",1,integer(c.station,0x1fc));
  trace.equal("station.rewind",bcat::station.rewind,integer(c.station,0x200));
  const auto hero=rt::read_symbol_address(memory,s::game_parser,ubc::lhiver::hero_symbol);
  std::uint32_t bound=0;
  if(hero.valid&&field(hero.address,0x1c,bound))trace.equal("station.hero_binding",c.hero,bound);
  else trace.check("station.hero_binding","Held","unlesbar",false);
  const auto p=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
  if(p.valid)trace.equal("station.production",16,p.value);
  else trace.check("station.production","16",rt::issue_name(p.issue),false);
  bow_station_name(trace,c,"station.on_state",0x1e0,bcat::station.on_state);
  bow_station_name(trace,c,"station.scheme",0x1b8,bcat::station.scheme);
  bow_station_name(trace,c,"station.use_with_item",0x1a4,bcat::station.use_with_item);
  bow_station_name(trace,c,"station.condition",0x1cc,bcat::station.condition_name);
}
bool bow_entry(const Context& c,ubc::BowMenuTrace& trace) noexcept {
  if(trace.full||!c.valid)bow_station_checks(c,trace);
  if(trace.full||!c.valid||!c.dialog.open||!c.dialog.waiting)
    ubc::read_selection_dialog(memory,s::information_manager,&trace,c.hero);
  const bool collection=complete_collection(c);
  trace.equal("entry.original_station",0,original_station);
  trace.equal("entry.handoff",0,handoff.active());
  trace.equal("entry.success_pending",0,success_pending);
  trace.equal("entry.adapter_failed",0,adapter_failed());
  trace.equal("entry.context",1,c.valid);
  trace.equal("dialog.waiting",1,c.dialog.waiting);
  trace.equal("dialog.open",1,c.dialog.open);
  trace.equal("info.complete",1,collection);
  if(trace.full||!collection){
    trace.value("info.generation",">0",rows.generation,rows.generation!=0);
    trace.equal("info.view",c.dialog.view,rows.view);
    trace.equal("info.valid",1,rows.valid);
    trace.equal("info.count",c.dialog.count,rows.count);
    if(rows.rejected_field)trace.equal(rows.rejected_field,rows.expected,rows.actual);
  }
  if(trace.full)for(std::size_t i=0;i<rows.count;++i){
    const auto& row=rows.rows[i];
    const auto* owner=owner_for(row.instance);
    const int index=static_cast<int>(i);
    trace.value("info.instance","bekannter Owner",row.instance,owner!=nullptr,index);
    if(owner){
      trace.equal("info.condition",owner->condition->symbol,row.condition,index,row.instance);
      trace.equal("info.function",owner->information->symbol,row.function,index,row.instance);
      trace.equal("info.number",owner->number,row.number,index,row.instance);
      trace.equal("info.npc",owner->npc,row.npc,index,row.instance);
      trace.equal("info.important",owner->important,row.important,index,row.instance);
      trace.equal("info.trade",owner->trade,row.trade,index,row.instance);
      trace.equal("info.permanent",owner->permanent,row.permanent,index,row.instance);
    }
  }
  std::uint32_t view=c.dialog.view;
  if(!c.dialog.valid)field(s::information_manager,0x1c,view);
  const auto expected=view&&view<=UINT32_MAX-a::view_callback?view+a::view_callback:0;
  const auto top=ubc::NativeSelectionUi::top_callback();
  const int global_menu=integer(0x8d1e38,8);
  if(expected)trace.equal("dialog.input_owner",expected,top);
  else trace.check("dialog.input_owner","Dialogcallback","View unlesbar",false);
  trace.equal("entry.global_menu",0,global_menu);
  return !original_station&&c.valid&&c.dialog.waiting&&c.dialog.open&&collection&&expected&&top==expected&&global_menu==0;
}
nav::Page alchemy_page(const Context& c) noexcept {
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
  state.invocation=c.dialog.mode==1?basecat::Invocation::info:basecat::Invocation::choice;
  if(c.dialog.mode==2){
    const auto info=ubc::read_selection_info(memory,c.dialog.info);
    if(!linked(info))return nav::Page::unknown;
    state.choice_owner=info.instance;
  }
  for(std::size_t i=0;i<values.size();++i){
    const auto v=rt::symbol_int(memory,s::game_parser,nav::flag_bindings[i].symbol);
    if(!v.valid||v.value!=values[i])return nav::Page::unknown;
  }
  // A paid page is accessible only within this specific handoff.
  if(state.choice_owner==20399&&c.dialog.mode==2&&handoff.active()&&!handoff.cancelled()&&
     tobacco_owner.owns(key(c),c.dialog.view,c.dialog.info)){
    state.invocation=basecat::Invocation::info;
    if(nav::classify_page(state)==nav::Page::tobacco_info)return nav::Page::tobacco_info;
    return nav::Page::unknown;
  }
  return nav::classify_page(state);
}
void __fastcall observe_end(void*,void*,void*,int);
void initialize_end_compatibility() noexcept {
  if(end_compatibility_attempted)return;
  end_compatibility_attempted=true;
  namespace z=ubc::zbug_end_compat;
  end_zbug_module.acquire(L"ZBUGFIXES.DLL",z::zbug_image_size,z::zbug_file_sha256,true);
  end_union_module.acquire(L"shw32.dll",z::union_image_size,z::union_file_sha256,true);
  if(end_zbug_module.current()&&end_union_module.current())event("end_modules_verified");
}
const ubc::zbug_end_compat::Binding* end_compatibility(bool predebit=false) noexcept {
  if(predebit?(!end_zbug_module.observed()||!end_union_module.observed()):
      (!end_zbug_module.current()||!end_union_module.current()))return nullptr;
  const auto page=ubc::windows_probe::LoadedModuleIdentity::executable_page;
  end_compatibility_binding={{end_zbug_module.base(),end_zbug_module.image_size(),end_zbug_module.digest(),true,page},
    {end_union_module.base(),end_union_module.image_size(),end_union_module.digest(),true,page},predebit};
  return &end_compatibility_binding;
}
ubc::station_end_contract::PathVerification end_path(std::uint32_t mob,bool predebit=false) noexcept {
  std::uint32_t table=0;if(!read(mob,table))return {false,ubc::station_end_contract::PathIssue::unreadable};
  return ubc::station_end_contract::verify_path(memory,table,{InterlockedCompareExchange(&installed,0,0)!=0,
    reinterpret_cast<std::uint32_t>(&observe_end),reinterpret_cast<std::uint32_t>(hooks.original(8))},end_compatibility(predebit));
}
bool matches_station_profile(std::uint32_t mob,cat::Station kind) noexcept;
bool __cdecl extra_boundary(void*,xwriter::Phase,const xdebit::Job&,xwriter::Boundary&);
bool __cdecl spirit_boundary(void*,gwriter::Phase,const gdebit::Job&,gwriter::Boundary&);
std::uint64_t next_extra_serial() noexcept {
  if(extra_serial==UINT64_MAX){disabled=true;return 0;}return ++extra_serial;
}
xdebit::Context extra_context(const Context& c) noexcept {
  return {generation,GetCurrentThreadId(),s::game_parser,c.game,c.world,c.hero,c.station};
}
xdep::Modules extra_modules(bool predebit=false) noexcept {
  const auto* plugins=end_compatibility(predebit);
  if(!plugins||!extra_zpe_module.current()||!extra_crt_module.current())return {};
  const auto value=[](const auto& m) noexcept {return xdep::Module{m.base(),m.image_size(),m.digest(),true};};
  return {plugins->zbug,plugins->union_core,value(extra_zpe_module),value(extra_crt_module)};
}
bool initialize_item_modules(bool predebit=false) noexcept {
  const bool zpe=extra_zpe_module.current()||extra_zpe_module.acquire(L"zParserExtender.dll",xdep::parser_extender_size,xdep::parser_extender_sha256);
  const bool crt=extra_crt_module.current()||extra_crt_module.acquire(L"msvcr100.dll",0xbe000,xdep::crt_sha256);
  return zpe&&crt&&end_compatibility(predebit);
}
bool bow_modules_current() noexcept {
  return bow_initialized&&bow_item_reader&&bow_renderer_module.current()&&extra_zpe_module.current()&&
    extra_crt_module.current()&&end_zbug_module.current()&&end_union_module.current()&&
    (!bow_locator_bound||bow_locator_module.observed());
}
bitem::Input bow_read_input(const ubc::CraftingKey& k) noexcept {
  bitem::Input input;
  input.boundary={{k.generation,k.thread,k.world,k.hero,k.station,k.manager,16},frame,GetCurrentThreadId(),true};
  input.modules=extra_modules();input.renderer={bow_renderer_module.base(),bow_renderer_module.image_size(),bow_renderer_module.digest(),true};
  if(bow_locator_bound&&bow_locator_module.observed())
    input.locator={bow_locator_module.base(),bow_locator_module.image_size(),bow_locator_module.digest(),true,
      ubc::windows_probe::LoadedModuleIdentity::executable_page};
  return input;
}
void initialize_bow(const Context& c) noexcept {
  if(bow_initialized||!c.valid||c.kind!=cat::Station::bow||!own_thread()||frame_depth!=1||update_depth||
    info_depth||choice_depth||station_depth||collection_depth||handoff.active()||amount_patch.active())return;
  bow_initialized=true;bow_item_reader.emplace(GetCurrentThreadId());
  const bool modules=initialize_item_modules();
  const bool renderer=bow_renderer_module.acquire(L"g2a_avx2.dll",ubc::bow_item_renderer::image_size,ubc::bow_item_renderer::file_sha256);
  const auto& locator=ubc::vob_locator_compat::module_profiles[0];
  bow_locator_bound=bow_locator_module.acquire(L"UNION_VOB_LOCATOR.DLL",locator.image_size,locator.file_sha256,true);
  log_plugin_version("bow_locator_version",bow_locator_module);
  log_plugin_version("bow_zbug_version",end_zbug_module);
  log_plugin_version("bow_union_version",end_union_module);
  log_bow_check("module.dependencies",1,modules);
  log_bow_check("module.renderer",1,renderer);
  event(modules&&renderer?"bow_modules_verified":"bow_modules_rejected");
}
int __fastcall observe_destroy(void*,void*,void*);
std::uint64_t __cdecl cool_remove_before(void*,const rcall::Invocation&);
void __cdecl cool_remove_after(void*,const rcall::Invocation&,std::uint64_t,std::uint32_t);
ubc::alchemy_flask_runtime::NativeCodePolicy cool_native_policy(bool predebit=false) noexcept {
  ubc::alchemy_flask_runtime::NativeCodePolicy policy;
  if(predebit){
    const auto* plugins=end_compatibility(true);
    if(plugins)return {ubc::alchemy_flask_runtime::NativeCodeMode::cooling_predebit,plugins->zbug,plugins->union_core};
  }
  if(end_zbug_module.current()){
    policy.mode=ubc::alchemy_flask_runtime::NativeCodeMode::known_zbug_remove_site;
    policy.zbug={end_zbug_module.base(),end_zbug_module.image_size(),end_zbug_module.digest(),true};
  }
  return policy;
}
cpath::OwnedPatches cool_owned_patches() noexcept {
  cpath::OwnedPatches owned;
  if(cool_destroy_installed){owned.destroy_installed=true;
    owned.destination=reinterpret_cast<std::uint32_t>(&observe_destroy);
    owned.trampoline=reinterpret_cast<std::uint32_t>(cool_hooks.original(0));}
  if(cool_remove_observer.owns_patch()){
    owned.remove_installed=true;owned.remove=cool_remove_observer.patch_plan();
  }
  return owned;
}
ddispatch::Context destroy_dispatch_context(const Context& c) noexcept {
  if(!c.identity_known||!c.station_known||!c.world||!c.hero||!c.station)return {};
  auto kind=c.kind;
  if(!c.valid){
    // When the original interaction ends, the dialog selection may already
    // be closed. In that case, take the route only from our pending order
    // with unchanged physical identities. Never infer it from a default kind.
    const auto& k=handoff.request().key;
    if(result_guard.state()!=ubc::CraftingResultState::waiting||k.generation!=generation||
      k.thread!=GetCurrentThreadId()||k.world!=c.world||k.hero!=c.hero||k.station!=c.station||
      k.manager!=s::information_manager)return {};
    kind=k.kind;
  }
  const auto production=kind==cat::Station::cool?13:kind==cat::Station::grind?11:0;
  return {generation,GetCurrentThreadId(),c.world,c.hero,c.station,s::information_manager,production};
}
enum class DestroyReadPhase {closed,preview,resolver,callback,destroy};
DestroyReadPhase destroy_read_phase(const Context& c,const ddispatch::Invocation* invocation=nullptr) noexcept {
  const auto bound=destroy_dispatch_context(c);const auto bridge=bc::diagnostics();
  if(!own_thread()||!generation||!frame||!c.identity_known||!c.station_known||
    !bound.world||!bound.hero||!bound.station||(bound.production_kind!=11&&bound.production_kind!=13)||
    station_depth||collection_depth||amount_patch.active()||bridge.readers||bridge.writes_disabled)return DestroyReadPhase::closed;
  // Only our own quiescent post-render callback. An invocation
  // pointer does not authorize running the event manager phase early.
  if(frame_depth==1&&!cool_destroy_depth&&!native_update&&!update_depth&&!info_depth&&!choice_depth&&!resolved_read_depth&&
    !bridge.writer&&c.valid&&!invocation)return DestroyReadPhase::preview;
  if(frame_depth!=0)return DestroyReadPhase::closed;
  const auto& request=handoff.request();const auto& k=request.key;const auto* recipe=cat::find(request.id);
  if(!recipe||(recipe->route!=cat::Route::cool&&recipe->route!=cat::Route::grind)||
    k.generation!=generation||k.thread!=GetCurrentThreadId()||k.world!=bound.world||k.hero!=bound.hero||
    k.station!=bound.station||k.manager!=bound.manager||k.kind!=recipe->station||
    (k.kind==cat::Station::cool?13:k.kind==cat::Station::grind?11:0)!=bound.production_kind)return DestroyReadPhase::closed;
  // This scope covers only our actual handler and its inner before/after
  // reads. Do not read the message again after the original call.
  if(cool_destroy_depth==1&&!native_update&&!update_depth&&!info_depth&&!choice_depth&&!resolved_read_depth&&!bridge.writer&&
    effect_executed&&result_guard.state()==ubc::CraftingResultState::waiting&&
    ((recipe->cool&&cool_job&&!grind_job)||(recipe->grind&&grind_job&&!cool_job))){
    if(invocation&&(invocation->npc!=k.hero||invocation->caller!=cabi::destroy_return||invocation->depth!=1))return DestroyReadPhase::closed;
    return DestroyReadPhase::destroy;
  }
  if(cool_destroy_depth||invocation||!c.valid||!native_update||update_depth!=1||!handoff.active()||handoff.cancelled()||
    (handoff.state()!=ubc::CraftingHandoffState::dispatched&&handoff.state()!=ubc::CraftingHandoffState::executing))return DestroyReadPhase::closed;
  const auto* ticket=handoff.current_ticket();
  if(!ticket||!ubc::crafting_handoff_detail::same_ticket(*ticket,dispatched_ticket)||
    ticket->view!=c.dialog.view||ticket->offer.collection!=rows.generation||!(key(c)==k))return DestroyReadPhase::closed;
  // After the original GetInformation call there is no Info()/writer lease
  // yet. The existing observe_resolve scope verifies the exact caller.
  if(resolved_read_depth==1&&!info_depth&&!choice_depth&&!bridge.writer&&
    handoff.state()==ubc::CraftingHandoffState::dispatched&&ticket->offer.mode==basecat::Invocation::info)
    return DestroyReadPhase::resolver;
  // Read-only observation on entry, followed later by the same inner writer. The native
  // effect still requires late_gate and the applicable info lease or PostRemoveChoice.
  if(!resolved_read_depth&&((info_depth==1&&!choice_depth&&ticket->offer.mode==basecat::Invocation::info)||
    (choice_depth==1&&!info_depth&&ticket->offer.mode==basecat::Invocation::choice)))return DestroyReadPhase::callback;
  return DestroyReadPhase::closed;
}
cpath::Verification destroy_verified_path(const cpath::OwnedPatches& patches,
    const ddispatch::Invocation* invocation=nullptr,bool predebit=false) noexcept {
  if(!destroy_dispatch_reader)return {false,cpath::Issue::dispatch,cabi::destroy_entry};
  const auto before=context();const auto bound=destroy_dispatch_context(before);
  const auto phase=destroy_read_phase(before,invocation);
  const auto* current_modules=end_compatibility(predebit);
  const auto modules=current_modules?*current_modules:ubc::zbug_end_compat::Binding{};
  auto proof=cpath::verify_dispatch(memory,patches,cool_native_policy(predebit),*destroy_dispatch_reader,
    bound,frame,GetCurrentThreadId(),phase!=DestroyReadPhase::closed,invocation,current_modules?&modules:nullptr);
  if(current_modules&&!end_compatibility(predebit))
    return {false,cpath::Issue::ownership,ubc::cool_set_interact_path::body.address};
  const auto after=context();
  if(!ddispatch::detail::same(bound,destroy_dispatch_context(after))||phase!=destroy_read_phase(after,invocation))
    return {false,cpath::Issue::changed,cabi::destroy_entry};
  return proof;
}
cpath::Verification cool_runtime_path(bool predebit=false) noexcept {
  if(!cool_ready||!cool_destroy_installed||!cool_remove_observer.owns_patch())
    return {false,cpath::Issue::ownership,cabi::destroy_entry};
  return destroy_verified_path(cool_owned_patches(),nullptr,predebit);
}
bool __cdecl cool_predebit_boundary(void*,cpre::reader::Phase,cpre::reader::Input&) noexcept;
bool cool_engine_current() noexcept {
#ifdef UBC_PLAYER_RELEASE
  return player_image.failure[static_cast<unsigned>(cat::Station::cool)] == 0 && w::player_image_header();
#else
  return cool_predebit_engine.current();
#endif
}
bool bind_cool_engine() noexcept {
#ifdef UBC_PLAYER_RELEASE
  return cool_engine_current();
#else
  return cool_predebit_engine.acquire(L"Gothic2.exe",0x752000,s::executable_sha256);
#endif
}
void initialize_cool(const Context& c) noexcept {
  if(!generation||cool_install_attempted||(c.kind==cat::Station::grind&&grind_probe_failed)||!c.valid||(c.kind!=cat::Station::cool&&c.kind!=cat::Station::grind)||!own_thread()||frame_depth!=1||
    update_depth||info_depth||choice_depth||station_depth||collection_depth||cool_destroy_depth||
    handoff.active()||amount_patch.active()||!complete_collection(c)||!c.dialog.waiting||!c.dialog.open)return;
  const bool cooling=c.kind==cat::Station::cool;
  destroy_dispatch_reader.emplace(GetCurrentThreadId());
  // Both stations use the passive observers. Before execution,
  // each order rechecks the forwarding paths that apply to it.
  const auto native=destroy_verified_path({},nullptr,cooling);
  if(!native.valid||!end_path(c.station,cooling).valid){
    if(cooling)cool_install_attempted=true;else grind_probe_failed=true;
    event("cool_native_rejected",static_cast<int>(native.issue),static_cast<int>(native.address));return;
  }
  cool_install_attempted=true;
  const std::array<std::uintptr_t,1> targets{{reinterpret_cast<std::uintptr_t>(&observe_destroy)}};
  if(!cool_hooks.prepare(targets)||!cool_remove_observer.prepare(GetCurrentThreadId(),
      {nullptr,cool_remove_before,cool_remove_after},cool_native_policy(cooling))){
    event("cool_hooks_rejected",static_cast<int>(cool_remove_observer.issue()));return;
  }
  original_destroy=reinterpret_cast<Destroy>(cool_hooks.original(0));
  cool_destroy_installed=cool_hooks.install(reinterpret_cast<std::uintptr_t>(&observe_destroy));
  if(!cool_destroy_installed||!cool_remove_observer.install()){
    event("cool_hooks_rejected",static_cast<int>(cool_remove_observer.issue()));return;
  }
  cool_ready=true;
  const auto installed_path=cool_runtime_path(cooling);cool_ready=installed_path.valid;
  if(cool_ready&&initialize_item_modules(true)&&
     bind_cool_engine()){
    cool_predebit_reader.emplace(GetCurrentThreadId());
    const cwriter::Config config{GetCurrentThreadId(),memory,nullptr,cool_predebit_boundary,
      reinterpret_cast<cwriter::Remove>(xdep::expected_targets.remove),
      reinterpret_cast<cwriter::RestorePop>(xdep::expected_targets.restore_pop),
      reinterpret_cast<cwriter::RestoreInstance>(xdep::expected_targets.restore_instance),
      reinterpret_cast<cwriter::RestoreItem>(0x7a2f20)};
    cool_predebit_executor.emplace(cool_predebit_tracker,config);
  }
  event(cool_ready?"cool_hooks_installed":"cool_native_rejected",
    static_cast<int>(installed_path.issue),static_cast<int>(installed_path.address));
}
ubc::CoolRecipeKey cool_key(const Context& c) noexcept {
  return {generation,GetCurrentThreadId(),c.world,c.hero,c.station,s::information_manager,13};
}
// For now the direct native path is limited to the single verified
// list stack. No quantity query materializes a packed entry.
bool cool_listed_input(const ubc::CoolRecipeCatalogueCapture& source) noexcept {
  return ubc::verify_cool_listed_input(memory,source,cool_inventory,cool_hands);
}
ubc::CoolRecipeCatalogueCapture capture_cool(const Context& c,bool boundary) noexcept {
  ubc::CoolCaptureProofs proof;proof.stable_boundary_verified=c.valid&&boundary;
  const auto native=cool_runtime_path(true);
  if(proof.stable_boundary_verified&&native.valid&&end_path(c.station,true).valid&&
    ubc::station_recipe_snapshot_detail::begin_hands(memory,c.hero,cool_hands)==ubc::StationHandReadIssue::none&&
    ubc::station_recipe_snapshot_detail::verify_hands(memory,cool_hands)){
    proof.verified_left_tmp_level=cool_hands.before.left_tmp_level;
    proof.station_hand_end_verified=matches_station_profile(c.station,cat::Station::cool);
  }
  auto source=cool_catalogue_reader.capture(memory,s::game_parser,cool_key(c),proof);
  const bool two=source.recipe.material.known()&&source.recipe.material.rows[0].stored==2;
  if(!cool_listed_input(source)||!same_context(c,context())||!cool_runtime_path(two).valid||!end_path(c.station,two).valid)
    source.recipe.contract=false;
  return source;
}
ubc::GrindRecipeKey grind_key(const Context& c) noexcept {
  return {generation,frame,GetCurrentThreadId(),c.world,c.hero,c.station,s::information_manager,11};
}
// The full copy of 4096 objects persists. Exactly one reader holds it
// until its projection finishes. Reentry must not replace this storage.
struct GrindCaptureLease {
  ubc::GrindRecipeCatalogueCapture* value=nullptr;
  GrindCaptureLease(const ubc::GrindRecipeKey& k,bool boundary) noexcept {
    if(!own_thread()||k.thread!=GetCurrentThreadId()||k.frame!=frame||grind_capture_leased)return;
    if(grind_preview_scope.active){grind_preview_scope.failed=true;return;}
    grind_capture_leased=true;
    value=::new (grind_capture_storage.data()) ubc::GrindRecipeCatalogueCapture(
      grind_catalogue_reader.capture(memory,s::game_parser,k,boundary));
  }
  ~GrindCaptureLease(){if(value){value->~GrindRecipeCatalogueCapture();grind_capture_leased=false;}}
  GrindCaptureLease(const GrindCaptureLease&)=delete;
  GrindCaptureLease& operator=(const GrindCaptureLease&)=delete;
  const ubc::GrindRecipeCatalogueCapture* view(const ubc::GrindRecipeKey& k) const noexcept {
    return value&&grind_capture_leased&&static_cast<const void*>(value)==grind_capture_storage.data()&&
      own_thread()&&k.thread==GetCurrentThreadId()&&k.frame==frame&&
      ubc::grind_recipe_snapshot_detail::same_key(value->key,k)?value:nullptr;
  }
};
bool grind_frame_capture_quiet() noexcept {
  const auto bridge=bc::diagnostics();
  return own_thread()&&frame_depth==1&&!update_depth&&!info_depth&&!choice_depth&&!station_depth&&!collection_depth&&
    !resolved_read_depth&&!bridge.writer&&!bridge.readers&&!bridge.writes_disabled&&!amount_patch.active()&&
    !handoff.active()&&result_guard.permits_next()&&!cool_destroy_depth;
}
std::uint64_t next_grind_serial() noexcept {
  if(grind_serial==UINT64_MAX){disabled=true;return 0;}return ++grind_serial;
}
bool grind_modules_current() noexcept {
  return grind_initialized&&grind_item_reader&&grind_renderer_module.current()&&extra_zpe_module.current()&&
    extra_crt_module.current()&&end_zbug_module.current()&&end_union_module.current()&&
    (!grind_locator_bound||grind_locator_module.current());
}
shitem::Input grind_read_input(const ubc::GrindRecipeKey& k,bool boundary) noexcept {
  shitem::Input out{{k,GetCurrentThreadId(),boundary},extra_modules(),
    {grind_renderer_module.base(),grind_renderer_module.image_size(),grind_renderer_module.digest(),true}};
  if(grind_locator_bound&&grind_locator_module.current()&&end_union_module.current())
    out.inventory_insert={
      {grind_locator_module.base(),grind_locator_module.image_size(),grind_locator_module.digest(),true},
      {end_union_module.base(),end_union_module.image_size(),end_union_module.digest(),true}};
  return out;
}
void initialize_grind(const Context& c) noexcept {
  if(grind_initialized||!c.valid||c.kind!=cat::Station::grind||!own_thread()||frame_depth!=1||update_depth||
    info_depth||choice_depth||station_depth||collection_depth||handoff.active()||amount_patch.active())return;
  grind_initialized=true;grind_item_reader.emplace(GetCurrentThreadId());
  grind_preview_reader.emplace(GetCurrentThreadId(),grind_catalogue_reader,*grind_item_reader);
  const bool modules=initialize_item_modules();
  const bool renderer=grind_renderer_module.acquire(L"g2a_avx2.dll",ubc::bow_item_renderer::image_size,ubc::bow_item_renderer::file_sha256);
  const auto& locator=ubc::vob_locator_compat::module_profiles[0];
  // Without an actual module lease, only the original slot is
  // admissible. The code check still detects an unknown hook.
  grind_locator_bound=grind_locator_module.acquire(L"UNION_VOB_LOCATOR.DLL",locator.image_size,locator.file_sha256);
  ubc::GrindCheck check;
  if(!check.equal("modules.item",true,modules)||!check.equal("modules.renderer",true,renderer))
    log_grind_check(check,0);
  event(modules&&renderer?"grind_modules_verified":"grind_modules_rejected",grind_locator_bound?1:0);
}
ubc::GrindMaterialProofs grind_station_proof(const Context& c,const ubc::GrindRecipeCatalogueCapture& raw,bool boundary) noexcept {
  ubc::GrindMaterialProofs out;out.stable_boundary_verified=boundary&&c.valid&&c.kind==cat::Station::grind&&
    own_thread()&&ubc::grind_recipe_snapshot_detail::same_key(raw.key,grind_key(c))&&
    raw.diagnostic.inventory_unchanged&&raw.diagnostic.hands_unchanged;
  if(out.stable_boundary_verified&&cool_runtime_path().valid&&end_path(c.station).valid&&
    matches_station_profile(c.station,cat::Station::grind)&&shdestroy::detail::active_grind_hand(raw.hands)){
    out.station_hand_end_verified=true;out.verified_left_tmp_level=raw.hands.left_tmp_level;
  }
  return out;
}
// The thread ID is constant for the lifetime of this thread.
// TLS is initialized on scope entry. The millions of subsequent
// local scope checks perform no OS, engine or module reads.
std::uint32_t grind_preview_current_thread() noexcept {
  static thread_local const std::uint32_t actual=GetCurrentThreadId();return actual;
}
bool observe_grind_preview_scope(void* object,shpreview::FrameObservation& out) noexcept {
  out={};
  if(object!=&grind_preview_scope||!grind_preview_scope.active||grind_preview_scope.failed||grind_preview_overflow)return false;
  const auto thread=grind_preview_current_thread();const auto& bound=grind_preview_scope.bound;
  const auto bridge=bc::diagnostics();
  out.key={generation,frame,thread,bound.world,bound.hero,bound.station,s::information_manager,11};
  out.scope_serial=grind_preview_scope.serial;out.current_thread=thread;out.outer_depth=frame_depth;
  out.input_serial=grind_preview_input_serial;out.active_scope=grind_preview_scope.active;
  out.writer=bridge.writer||amount_patch.active();
  out.quiet=InterlockedCompareExchange(&installed,0,0)&&
    InterlockedCompareExchange(&engine_thread,0,0)==static_cast<LONG>(thread)&&frame_depth==1&&
    !update_depth&&!info_depth&&!choice_depth&&!station_depth&&!collection_depth&&!resolved_read_depth&&
    !grind_preview_input_depth&&!bridge.readers&&!bridge.writes_disabled&&!handoff.active()&&
    result_guard.permits_next()&&!cool_destroy_depth&&!grind_capture_leased;
  return true;
}
bool grind_preview_station_path(const Context& c) noexcept {
  return cool_runtime_path().valid&&end_path(c.station).valid&&matches_station_profile(c.station,cat::Station::grind);
}
bool same_grind_preview_input(const shitem::Input& a,const shitem::Input& b) noexcept {
  const auto same=[](const auto& x,const auto& y) noexcept {
    return x.base==y.base&&x.image_size==y.image_size&&x.file_sha256==y.file_sha256&&x.mapping_verified==y.mapping_verified;
  };
  return ubc::grind_recipe_snapshot_detail::same_key(a.boundary.key,b.boundary.key)&&
    a.boundary.current_thread==b.boundary.current_thread&&a.boundary.stable_boundary_verified==b.boundary.stable_boundary_verified&&
    same(a.modules.zbug,b.modules.zbug)&&same(a.modules.union_core,b.modules.union_core)&&
    same(a.modules.parser_extender,b.modules.parser_extender)&&same(a.modules.runtime_crt,b.modules.runtime_crt)&&
    same(a.renderer,b.renderer)&&same(a.inventory_insert.locator,b.inventory_insert.locator)&&
    same(a.inventory_insert.union_core,b.inventory_insert.union_core);
}
// Only the outer refresh/quantity block holds this lease. The callback, handoff
// and writer never acquire it by default. There is no direct fallback on failure.
struct GrindPreviewLease {
  bool owner=false,ready=false;
  // 1 prerequisites, 2 fresh initial binding, 3 reader, 4 fresh
  // final binding, 5 publication. -1 is not a reader result.
  int failure_stage=1,read_issue=-1;
  ubc::GrindCheck check{};
  shpreview::Statistics statistics{};
  explicit GrindPreviewLease(const Context& c) noexcept {
    if(!check.equal("preview.reentry",false,grind_preview_scope.active)){grind_preview_scope.failed=true;return;}
    if(!check.equal("preview.context",true,c.valid)||
       !check.equal("preview.station",static_cast<int>(cat::Station::grind),static_cast<int>(c.kind))||
       !check.equal("preview.quiet",true,grind_frame_capture_quiet())||
       !check.equal("preview.leased",false,grind_capture_leased)||
       !check.equal("preview.input_depth",0,grind_preview_input_depth)||
       !check.equal("preview.overflow",false,grind_preview_overflow)||
       !check.equal("preview.reader",true,grind_preview_reader.has_value()))return;
    if(!check.equal("preview.serial_exhausted",false,grind_preview_serial==UINT64_MAX)){grind_preview_overflow=true;return;}
    owner=true;grind_preview_scope={c,++grind_preview_serial,true,false};
    failure_stage=2;
    shpreview::FrameObservation expected;
    if(!check.equal("preview.scope",true,observe_grind_preview_scope(&grind_preview_scope,expected))||
       !check.equal("preview.frame",true,shpreview::detail::valid_frame(expected,GetCurrentThreadId()))||
       !check.equal("preview.initial_context",true,same_context(c,context()))||
       !check.equal("preview.initial_modules",true,grind_modules_current()))return;
    const auto before=grind_read_input(expected.key,true);
    shpreview::Selection choice;
    choice.station_path_verified=grind_preview_station_path(c);
    choice.hero_dialog_verified=c.dialog.npc==c.hero&&c.dialog.player==c.hero;
    if(const auto* selected=session.selected())if(const auto* recipe=cat::find(selected->id))if(recipe->grind)
      choice.current={recipe->grind->function.symbol,session.quantity()};
    failure_stage=3;
    const bool captured=grind_preview_reader->capture_selection(memory,{&grind_preview_scope,observe_grind_preview_scope},expected,before,choice);
    statistics=grind_preview_reader->statistics();
    if(!captured){
      read_issue=static_cast<int>(grind_preview_reader->issue());
      check.equal("preview.reader_issue",static_cast<int>(shpreview::Issue::none),read_issue);
      return;
    }
    // Actual engine and module boundaries, checked separately from the inexpensive local probes.
    failure_stage=4;
    if(!check.equal("preview.final_context",true,same_context(c,context()))||
       !check.equal("preview.final_modules",true,grind_modules_current())||
       !check.equal("preview.input",true,same_grind_preview_input(before,grind_read_input(expected.key,true)))||
       !check.equal("preview.station_path",choice.station_path_verified,grind_preview_station_path(c))||
       !check.equal("preview.publish_context",true,same_context(c,context())))return;
    failure_stage=5;ready=grind_preview_reader->publish(grind_preview_publication);
    if(ready)failure_stage=0;
    else{
      read_issue=static_cast<int>(grind_preview_reader->issue());
      check.equal("preview.publication_issue",static_cast<int>(shpreview::Issue::none),read_issue);
    }
  }
  ~GrindPreviewLease(){if(owner){grind_preview_scope.active=false;ready=false;if(grind_preview_reader)grind_preview_reader->close();}}
  GrindPreviewLease(const GrindPreviewLease&)=delete;
  GrindPreviewLease& operator=(const GrindPreviewLease&)=delete;
  const shpreview::Publication* view(const ubc::GrindRecipeKey& k) const noexcept {
    shpreview::FrameObservation current;
    return owner&&ready&&ubc::grind_recipe_snapshot_detail::same_key(k,grind_preview_publication.key)&&
      observe_grind_preview_scope(&grind_preview_scope,current)&&
      shpreview::detail::same_frame(current,grind_preview_publication.scope)?&grind_preview_publication:nullptr;
  }
};
void report_grind_preview_failure(const GrindPreviewLease& value) noexcept {
  if(value.ready||(grind_preview_failure.generation==generation&&
    grind_preview_failure.stage==value.failure_stage&&grind_preview_failure.issue==value.read_issue&&
    grind_preview_failure.check.same(value.check)))return;
  grind_preview_failure={generation,value.failure_stage,value.read_issue,value.check};
  log_grind_check(value.check,value.failure_stage);
  event("grind_preview_batch_rejected",value.failure_stage,value.read_issue);
  event("grind_preview_batch_bounds",static_cast<int>(value.statistics.spans),static_cast<int>(value.statistics.stored_bytes));
}
// Dedicated one-shot measurement mode. No session refresh, menu order or
// handoff. Take five fresh, fully sealed snapshots. Snapshot 5 observes
// the actual whole-stack path without crafting to produce a remainder.
void measure_grind_preview(const Context& c) noexcept {
  if(execution_enabled||grind_measurement_case>=5||!c.valid||c.kind!=cat::Station::grind||
     !c.dialog.waiting||!c.dialog.open||!complete_collection(c)||!grind_frame_capture_quiet()||
     grind_preview_scope.active||grind_capture_leased||grind_preview_input_depth||grind_preview_overflow||
     !grind_item_reader||!grind_modules_current())return;
  if(grind_preview_serial==UINT64_MAX){grind_preview_overflow=true;return;}
  if(!grind_measurement_case){
    if(!ubc::grind_preview_measurement::start_generation(generation)){grind_preview_overflow=true;return;}
    initialize_cool(c);
  }
  const unsigned index=++grind_measurement_case;
  const std::array<std::uint32_t,5> quantities{{1,2,5,3,grind_measurement_whole}};
  const auto quantity=quantities[index-1];
  event("grind_measure_case",index,quantity);
  bool complete=false;int stage=1;
  grind_preview_scope={c,++grind_preview_serial,true,false};
  try {
    shpreview::FrameObservation expected;
    if(observe_grind_preview_scope(&grind_preview_scope,expected)&&
       shpreview::detail::valid_frame(expected,GetCurrentThreadId())&&same_context(c,context())&&quantity){
      stage=2;
      const auto input=grind_read_input(expected.key,true);
      const shpreview::Selection choice{{20975,quantity},grind_preview_station_path(c),
        c.dialog.npc==c.hero&&c.dialog.player==c.hero};
      auto reader=std::make_unique<ubc::grind_preview_measurement::Reader>(GetCurrentThreadId(),grind_catalogue_reader,*grind_item_reader);
      auto result=std::make_unique<shpreview::Publication>();const auto started=GetTickCount64();
      const bool captured=reader->capture_selection(memory,{&grind_preview_scope,observe_grind_preview_scope},expected,input,choice);
      const auto elapsed=GetTickCount64()-started;
      complete=captured&&reader->diagnostic_result(*result)&&same_context(c,context())&&grind_modules_current()&&
        same_grind_preview_input(input,grind_read_input(expected.key,true))&&choice.station_path_verified==grind_preview_station_path(c);
      if(complete)stage=3;
      const auto& reads=reader->read_set();const auto& stats=reads.statistics();const auto& rejected=reads.rejected();
      event("grind_measure_issue",static_cast<int>(reader->issue()),static_cast<int>(elapsed));
      event("grind_measure_bounds",static_cast<int>(stats.spans),static_cast<int>(stats.stored_bytes));
      const auto united=reads.union_size();event("grind_measure_union",static_cast<int>(united.first),static_cast<int>(united.second));
      event("grind_measure_calls",static_cast<int>(stats.recording_calls),static_cast<int>(stats.recording_bytes));
      event("grind_measure_replay",static_cast<int>(stats.replay_calls),static_cast<int>(stats.replay_bytes));
      event("grind_measure_largest",static_cast<int>(stats.largest_span),static_cast<int>(stats.overlaps));
      event("grind_measure_old_issue",static_cast<int>(rejected.issue),0);
      event("grind_measure_old_bounds",static_cast<int>(rejected.spans),static_cast<int>(rejected.bytes));
      event("grind_measure_old_request",chdiag::signed_word(rejected.address),static_cast<int>(rejected.length));
      for(const auto& range:reads.ranges())event("grind_measure_span",chdiag::signed_word(range.first),static_cast<int>(range.second));
      if(complete){
        const auto* selected=shpreview::find(*result,20975,quantity);
        complete=selected&&selected->world.stable;
        if(selected){
          event("grind_measure_world",selected->world.nodes[0],selected->world.nodes[1]);
          event("grind_measure_inventory",result->catalogue.diagnostic.listed_nodes,result->catalogue.diagnostic.packed_entries);
          event("grind_measure_stock",selected->stock.materials[0].total,result->catalogue.diagnostic.state_before.skill);
          if(index==1)grind_measurement_whole=selected->stock.materials[0].total;
        }
      }
    }
  }catch(...){complete=false;}
  grind_preview_scope.active=false;
  event("grind_measure_stage",complete?0:stage,static_cast<int>(generation));
  event("grind_measure_end",index,complete?1:0);
  if(!complete)grind_measurement_case=5; // Never retry incomplete snapshots.
}
void grind_preview_item_diagnostic(std::size_t index,const ubc::GrindRecipeKey& k,int issue,std::uint32_t address) noexcept {
  auto& last=grind_preview_diagnostic[index];
  if(issue>=0&&(last.generation!=k.generation||last.issue!=issue||last.address!=address)){
    const auto id=cat::recipes[cat::grind_begin+index].id;
    ubc::GrindCheck check;
    check.equal("preview.item_issue",static_cast<int>(shitem::Issue::none),issue);
    log_grind_check(check,static_cast<int>(id));
    event("grind_preview_item_issue",static_cast<int>(id),issue);
    if(address)event("grind_preview_item_address",static_cast<int>(id),chdiag::signed_word(address));
  }
  last={k.generation,issue,address};
}
ubc::CraftingSnapshot capture_grind(const Context& c,bool boundary,const GrindPreviewLease* shared=nullptr) noexcept {
  const auto k=grind_key(c);
  if(shared){
    const auto* closed=c.valid&&boundary?shared->view(k):nullptr;if(!closed)return {};
    for(std::size_t i=0;i<closed->count;++i)if(closed->requests[i].quantity==1)
      for(std::size_t j=0;j<shcat::recipes.size();++j)if(closed->requests[i].callback==shcat::recipes[j].function.symbol)
        grind_preview_item_diagnostic(j,k,closed->blocked[i].blocked?static_cast<int>(closed->blocked[i].phase):-1,closed->blocked[i].address);
    return closed->snapshot;
  }
  const GrindCaptureLease lease{k,c.valid&&boundary};if(!lease.value)return {};
  const auto& raw=*lease.value;
  ubc::GrindProjectionInput input;input.key=k;input.scope=ubc::grind_item_path::Scope::preview;
  input.physical=ubc::read_grind_guild_for_preview(memory,s::game_parser,k);
  input.station=grind_station_proof(c,raw,boundary);
  input.physical_hero_preview_verified=c.valid&&boundary&&c.dialog.npc==c.hero&&c.dialog.player==c.hero;
  const auto candidates=ubc::project_capture(raw,input);
  if(grind_modules_current()&&ubc::valid_crafting_snapshot(candidates))
    for(std::size_t i=0;i<shcat::recipes.size();++i){const auto& row=candidates.recipes[cat::grind_begin+i];
      if(!shpreview::candidate(row))continue;
      const auto item=grind_item_reader->capture_preview(memory,raw,input.physical,shcat::recipes[i].function.symbol,1,
        grind_read_input(k,input.station.stable_boundary_verified));
      grind_preview_item_diagnostic(i,k,item.item_path_verified?-1:static_cast<int>(item.issue),item.failed_address);
      input.materials[i]=ubc::bind_grind_material(raw,item,input.station);
    }
  auto out=ubc::project_capture(raw,input);
  const auto guild_after=ubc::read_grind_guild_for_preview(memory,s::game_parser,k);
  if(!same_context(c,context())||!ubc::grind_guild_snapshot_detail::same(input.physical,guild_after)||
    !shrt::same_state(raw.diagnostic.state_before,shrt::state(memory,s::game_parser,shrt::assemble(shcat::recipes[0]))))out.key={};
  return out;
}
bool matching_grind_item(const shitem::Evidence& item,const ubc::CraftingRequest& request,
    ubc::grind_item_path::Scope purpose) noexcept {
  const auto* recipe=cat::find(request.id);const auto c=context();
  if(!recipe||!recipe->grind||request.key.kind!=cat::Station::grind||!c.valid||!(key(c)==request.key)||
    !shitem::matches(item,grind_key(c),recipe->grind->function.symbol,request.quantity,purpose)||
    !ubc::same_crafting_hands(item.stock.input.hands,request.hands)||!grind_modules_current())return false;
  const auto wanted=ubc::crafting_grind_plan(request.material_snapshot,request.quantity);
  if(!wanted.valid)return false;
  for(std::size_t i=0;i<3;++i)if(item.stock.materials[i].symbol!=request.material_snapshot.materials[i].symbol||
    item.stock.materials[i].total!=request.material_snapshot.materials[i].stored||
    item.expected_after_callback[i]!=wanted.after_callback[i].amount)return false;
  return true;
}
bool matching_grind_knowledge(const ubc::GrindMaterialKnowledge& actual,const ubc::CraftingRequest& request) noexcept {
  const auto& wanted=request.material_snapshot.grind;
  return wanted.known&&actual.talent_known&&actual.guild_known&&actual.skill_known&&
    actual.talent==wanted.talent&&actual.guild==wanted.guild&&actual.skill==wanted.skill;
}
// Result and restore read the actual globals and the bound physical guild.
// Production may already be 0 after the original call. This does not create a new
// station entry, and the old SELF preview does not qualify as writer evidence.
ubc::CraftingGrindValues read_grind_result_values(const ubc::CraftingRequest& request) noexcept {
  ubc::CraftingGrindValues out;const auto* recipe=cat::find(request.id);const auto before=context();
  if(!recipe||!recipe->grind||!own_thread()||!before.identity_known||!before.station_known||
    before.world!=request.key.world||before.hero!=request.key.hero||
    request.key.thread!=GetCurrentThreadId()||request.key.generation!=generation||amount_patch.active())return out;
  const auto assembly=shrt::assemble(*recipe->grind);
  const auto state=shrt::state(memory,s::game_parser,assembly);
  auto k=grind_key(before);k.station=request.key.station;
  const auto guild=ubc::read_grind_guild_for_preview(memory,s::game_parser,k);
  const auto after_state=shrt::state(memory,s::game_parser,assembly);const auto after=context();
  if(!state.read_valid||!shrt::same_state(state,after_state)||!guild.known()||
    !rt::detail::same_layout(state.layout,guild.layout)||!after.identity_known||!after.station_known||
    after.world!=before.world||after.hero!=before.hero||after.station!=before.station||
    !shrt::verify_recipe(memory,s::game_parser,*recipe->grind).valid)return out;
  return {true,state.talent,guild.guild,state.skill};
}
void initialize_extra(const Context& c) noexcept {
  if(extra_initialized||!c.valid||c.kind!=cat::Station::alchemy||!own_thread()||frame_depth!=1||update_depth||
    info_depth||choice_depth||station_depth||collection_depth||handoff.active()||amount_patch.active())return;
  extra_initialized=true;
  extra_catalogue_reader.emplace(GetCurrentThreadId());
  tobacco_list.emplace(GetCurrentThreadId());
  const auto& locator=ubc::vob_locator_compat::module_profiles[0];
  tobacco_locator.acquire(L"UNION_VOB_LOCATOR.DLL",locator.image_size,locator.file_sha256,true);
  spirit_catalogue_reader.emplace(GetCurrentThreadId());
  const bool modules=initialize_item_modules();
  xwriter::Config config;config.owner_thread=GetCurrentThreadId();config.memory=memory;config.boundary=extra_boundary;
  config.remove=reinterpret_cast<xbridge::Remove>(xdep::expected_targets.remove);
  config.restore_pop=reinterpret_cast<xbridge::RestorePop>(xdep::expected_targets.restore_pop);
  config.restore_instance=reinterpret_cast<xbridge::RestoreInstance>(xdep::expected_targets.restore_instance);
  config.restore_item=reinterpret_cast<xbridge::RestoreItem>(0x7a2f20);
  extra_executor.emplace(config);
  gwriter::Config spirit_config;spirit_config.owner_thread=GetCurrentThreadId();spirit_config.memory=memory;
  spirit_config.boundary=spirit_boundary;
  spirit_config.remove=config.remove;spirit_config.restore_pop=config.restore_pop;
  spirit_config.restore_instance=config.restore_instance;spirit_config.restore_item=config.restore_item;
  spirit_executor.emplace(spirit_config);
  event(modules?"extra_modules_verified":"extra_modules_rejected");
}
ubc::CraftingAlchemyExtraInput extra_read_input(const Context& c,bool boundary) noexcept {
  ubc::CraftingAlchemyExtraInput in;in.key=key(c);in.context=extra_context(c);in.serial=next_extra_serial();
  in.current_thread=GetCurrentThreadId();in.modules=extra_modules();in.targets=xdep::expected_targets;in.restore_item=0x7a2f20;
  const auto bridge=bc::diagnostics();
  in.boundary.point=resolved_read_depth?ubc::AlchemyExtraReadPoint::selection_resolved:
    info_depth+choice_depth?ubc::AlchemyExtraReadPoint::before_original:ubc::AlchemyExtraReadPoint::post_render;
  in.boundary.verified=boundary&&own_thread()&&c.valid&&!station_depth&&!collection_depth&&
    (!resolved_read_depth||(resolved_read_depth==1&&native_update&&update_depth==1&&!info_depth&&!choice_depth));
  in.boundary.physical_lab_hand_contract_verified=c.valid&&c.kind==cat::Station::alchemy&&end_path(c.station).valid;
  in.boundary.callback_depth=info_depth+choice_depth;in.boundary.writer_depth=bridge.writer?1u:0u;
  in.boundary.resolve_depth=resolved_read_depth;
  return in;
}
std::uint32_t read_spirit_actual_info(void*,const ubc::CraftingAlchemyExtraInput& in) noexcept {
  const auto c=context();
  if(!(key(c)==in.key)||!xdebit::same(extra_context(c),in.context))return 0;
  return current_info_effect(c);
}
scat::Station station_kind(cat::Station kind) noexcept {
  return kind==cat::Station::leather?scat::Station::leatherwork:
    kind==cat::Station::saw?scat::Station::saw:scat::Station::forge;
}
cnav::Page cool_info_page(const Context& c,bool reread) noexcept {
  if(!c.valid||c.kind!=cat::Station::cool)return {};
  const auto state=crt::context(memory,s::game_parser,crt::assemble(ccat::recipe));
  if(!state.station_matches())return {};
  const auto page=ubc::cool_collection_page(memory,rows,c.dialog,state.production,reread);
  return crt::same_context_evidence(state,crt::context(memory,s::game_parser,crt::assemble(ccat::recipe)))?page:cnav::Page{};
}
cnav::Page cool_page(const Context& c) noexcept {
  if(!c.valid||c.kind!=cat::Station::cool||!crt::verify_recipe(memory,s::game_parser,ccat::recipe).valid)return {};
  return cool_info_page(c,false);
}
bool current_grind_choice(const Context& c) noexcept {
  const auto& lease=grind_choice_lease;const auto* ticket=handoff.current_ticket();const auto* step=handoff.step();
  if(!lease.owner||!own_thread()||lease.thread!=GetCurrentThreadId()||lease.depth!=1||lease.frame_number!=frame||
    !effect_authorized||!native_update||update_depth!=1||choice_depth!=1||info_depth||station_depth||collection_depth||
    resolved_read_depth||!post_remove_choice.active()||handoff.cancelled()||handoff.state()!=ubc::CraftingHandoffState::executing||
    !ticket||!step||!step->is_effect()||step->invocation!=basecat::Invocation::choice||
    !ubc::crafting_handoff_detail::same_ticket(lease.ticket,*ticket)||
    !ubc::crafting_handoff_detail::same_ticket(lease.ticket,dispatched_ticket)||!(lease.key==handoff.request().key)||
    !c.valid||c.kind!=cat::Station::grind||!(key(c)==lease.key)||c.game!=lease.game||c.dialog.mode!=2||
    c.dialog.info!=lease.owner||c.dialog.view!=ticket->view||ticket->offer.owner!=20957||
    rows.generation!=ticket->offer.collection||!shnav::valid_page(lease.page)||ubc::crafting_page(lease.page)!=step->before)return false;
  const auto owner=ubc::read_selection_info(memory,lease.owner);
  return linked(owner)&&owner.instance==20957;
}
shnav::PageEvidence grind_page(const Context& c) noexcept {
  if(!c.valid||c.kind!=cat::Station::grind)return {};
  const auto& functions=ubc::grind_menu_metadata::functions;
  const auto& bindings=ubc::grind_recipe_profile::common_bindings;
  const ubc::lhiver::Profile menu_profile{layout_profile.parser_address,layout_profile.symbol_count,
    layout_profile.code_length,layout_profile.code_sha256,functions.data(),functions.size(),bindings.data(),bindings.size()};
  const auto assembly=shrt::assemble(shcat::recipes[0]);
  if(!rt::verify_local_contract(memory,s::game_parser,menu_profile).matches||!shrt::verify(memory,s::game_parser,assembly).matches)return {};
  const auto before=shrt::state(memory,s::game_parser,assembly);if(!before.station_postcondition())return {};
  shnav::PageEvidence out;
  if(current_grind_choice(c))out=grind_choice_lease.page;
  else if(c.dialog.mode==1){
    if(!complete_collection(c)||!rows.count||rows.count>6)return {};
    std::array<shnav::InfoOffer,6> offers{};
    for(std::size_t i=0;i<rows.count;++i){const auto& row=rows.rows[i];if(!linked(row))return {};
      offers[i]={static_cast<std::uint32_t>(row.instance),static_cast<std::uint32_t>(row.condition),static_cast<std::uint32_t>(row.function)};}
    out=shnav::identify_info(before.production,offers.data(),rows.count,true);
  }else if(c.dialog.mode==2){
    ubc::SelectionChoices choices;if(!complete_collection(c,&choices)||choices.owner.instance!=20957)return {};
    std::array<std::uint32_t,13> callbacks{};if(choices.count>callbacks.size())return {};
    for(std::size_t i=0;i<choices.count;++i){if(choices.functions[i]<0)return {};callbacks[i]=static_cast<std::uint32_t>(choices.functions[i]);}
    out=shnav::identify_choices(before.production,20957,callbacks.data(),choices.count,true);
  }
  return shrt::same_state(before,shrt::state(memory,s::game_parser,assembly))?out:shnav::PageEvidence{};
}
ubc::CraftingPage page(const Context& c,ubc::BowMenuTrace* trace=nullptr) noexcept {
  if(!c.valid)return {};
  if(c.kind==cat::Station::stove)return c.dialog.mode==1?ubc::crafting_stove_page():ubc::CraftingPage{};
  if(c.kind==cat::Station::alchemy){
    const auto current=alchemy_page(c);
    if(c.dialog.mode==2&&current==nav::Page::tobacco_info)
      return {ubc::CraftingPageKind::tobacco_choices,tobacco::owner.instance};
    return ubc::crafting_page(current);
  }
  if(c.kind==cat::Station::cool)return ubc::crafting_page(cool_page(c));
  if(c.kind==cat::Station::grind)return ubc::crafting_page(grind_page(c));
  if(c.kind==cat::Station::bow)
    return ubc::crafting_page(ubc::bow_menu::page(memory,s::game_parser,c.dialog.mode,trace));
  if(c.kind==cat::Station::anvil){
    // Use the entire source graph, including other sources' own BACK owners,
    // beyond the target recipe. Rebind every original category continuation.
    const auto primary=art::assemble(acat::recipes[0]);
    for(const auto& recipe:acat::recipes)if(!art::verify(memory,s::game_parser,art::assemble(recipe)).matches)return {};
    for(const auto& recipe:ocat::recipes)if(!ort::verify(memory,s::game_parser,ort::assemble(recipe)).matches)return {};
    const auto before=art::context(memory,s::game_parser,primary);if(!before.station_matches())return {};
    anav::PageState state{true,before.production,before.start_smithing,before.normal_weapons,before.ore_weapons,
      c.dialog.mode==1?basecat::Invocation::info:basecat::Invocation::choice,-1};
    if(c.dialog.mode==2){const auto owner=ubc::read_selection_info(memory,c.dialog.info);
      if(!linked(owner))return {};state.choice_owner=owner.instance;}
    if(!art::same_context_evidence(before,art::context(memory,s::game_parser,primary)))return {};
    return ubc::crafting_page(anav::classify_page(state));
  }
  const auto kind=station_kind(c.kind);
  // All possible owner and BACK functions for the current station
  // remain bound to their original bodies, even when the source changes.
  for(const auto& recipe:scat::recipes)if(recipe.station==kind){
    const auto profile=ubc::station_recipe_runtime::assemble(recipe);
    if(!ubc::station_recipe_runtime::verify(memory,s::game_parser,profile).matches)return {};
  }
  snav::PageState state;state.known=true;state.station=kind;
  state.production=static_cast<std::int32_t>(kind);
  state.invocation=c.dialog.mode==1?basecat::Invocation::info:basecat::Invocation::choice;
  if(c.dialog.mode==2){const auto owner=ubc::read_selection_info(memory,c.dialog.info);
    if(!linked(owner))return {};state.choice_owner=owner.instance;}
  return ubc::crafting_page(snav::classify_page(state));
}
struct Capture {ubc::CraftingSnapshot snapshot{};ubc::StationRecipeHands hands{};int issue=0;};
void capture_bow(Capture& captured,const Context& c,std::uint64_t serial,bool boundary,
                 ubc::BowMenuTrace* trace=nullptr) noexcept {
  const ubc::BowRecipeKey k{serial,GetCurrentThreadId(),c.world,c.hero,c.station,s::information_manager,16};
  // Only the private measurement may read the independently identified hero
  // inventory when the dialog binding is rejected. Never pass it to session.
  const bool measuring=trace&&trace->full;
  const bool read_boundary=boundary&&(c.valid||(measuring&&c.identity_known&&c.world&&c.hero&&c.station));
  const auto end=read_boundary?end_path(c.station):ubc::station_end_contract::PathVerification{};
  const auto source=bow_catalogue_reader.capture(memory,s::game_parser,k,{read_boundary,c.valid&&boundary&&end.valid});
  captured.snapshot=ubc::project_capture(source,c.valid&&boundary&&c.dialog.npc==c.hero&&c.dialog.player==c.hero);
  const bool modules=bow_modules_current();
  ubc::gate_bow_modules(captured.snapshot,modules);
  if(trace){
    trace->equal("catalogue.boundary",1,read_boundary);
    if(read_boundary)trace->equal("catalogue.end_path",0,static_cast<int>(end.issue));
    else trace->pending("catalogue.end_path","0","kein Lesepunkt");
    trace->equal("catalogue.modules",1,modules);
    ubc::bow_menu::catalogue(*trace,source,captured.snapshot);
  }else{
    log_bow_check("catalogue.capture",0,static_cast<int>(source.diagnostic.issue));
    log_bow_check("catalogue.inventory",0,static_cast<int>(source.diagnostic.inventory_issue));
    log_bow_check("catalogue.hands",0,static_cast<int>(source.diagnostic.hand_issue));
  }
  captured.issue=static_cast<int>(source.diagnostic.issue);
}
bool active_extra_admission(const Context& c) noexcept {
  // Reread only before this specific, already bound original call. A pending
  // result continues to block the menu and unrelated subsequent orders.
  if(!extra_job||!extra_executor||!extra_writer_scope||!effect_authorized||effect_executed||
    extra_executor->outcome().original_entered||result_guard.state()!=ubc::CraftingResultState::waiting||
    !native_update||update_depth!=1||info_depth!=1||choice_depth||station_depth||collection_depth||
    resolved_read_depth||amount_patch.active()||!own_thread()||handoff.cancelled()||
    handoff.state()!=ubc::CraftingHandoffState::executing)return false;
  const auto bridge=bc::diagnostics();const auto actual=extra_executor->scope_evidence();
  const auto& request=handoff.request();const auto* recipe=cat::find(request.id);
  return bridge.writer&&!bridge.readers&&!bridge.writes_disabled&&actual.valid&&
    actual.scope.writer_active&&actual.scope.writer_depth==1&&!actual.scope.vm_depth&&!actual.scope.debit_depth&&
    actual.scope.job==extra_active_job.id&&actual.scope.writer_token==extra_active_job.writer_token&&
    actual.scope.callback==extra_active_job.callback&&extra_active_job.id==request.key.generation&&
    extra_active_job.writer_token==request.key.generation&&extra_active_job.quantity==request.quantity&&
    recipe&&recipe->alchemy_extra&&recipe->alchemy_extra->callback.function->symbol==extra_active_job.callback&&
    static_cast<std::uint32_t>(recipe->alchemy_extra->owner.instance)==extra_active_job.owner&&key(c)==request.key&&
    xdebit::same(extra_context(c),extra_active_job.context)&&
    extra_writer_scope->call.thread==GetCurrentThreadId()&&
    extra_writer_scope->call.symbol==static_cast<int>(extra_active_job.callback)&&
    extra_writer_scope->call.caller==bc::info_call_return&&current_info_effect(c);
}
bool active_spirit_admission(const Context& c) noexcept {
  // Reread only before this specific, already bound original call. A pending
  // result continues to block the menu and unrelated subsequent orders.
  if(!spirit_job||!spirit_executor||!spirit_writer_scope||!effect_authorized||effect_executed||
    spirit_executor->outcome().original_entered||result_guard.state()!=ubc::CraftingResultState::waiting||
    !native_update||update_depth!=1||info_depth!=1||choice_depth||station_depth||collection_depth||
    resolved_read_depth||amount_patch.active()||!own_thread()||handoff.cancelled()||
    handoff.state()!=ubc::CraftingHandoffState::executing)return false;
  const auto bridge=bc::diagnostics();const auto actual=spirit_executor->scope_evidence();
  const auto& request=handoff.request();const auto* recipe=cat::find(request.id);
  return bridge.writer&&!bridge.readers&&!bridge.writes_disabled&&actual.valid&&
    actual.scope.writer_active&&actual.scope.writer_depth==1&&!actual.scope.vm_depth&&!actual.scope.debit_depth&&
    actual.scope.job==spirit_active_job.id&&actual.scope.writer_token==spirit_active_job.writer_token&&
    actual.scope.callback==spirit_active_job.callback&&spirit_active_job.id==request.key.generation&&
    spirit_active_job.writer_token==request.key.generation&&spirit_active_job.quantity==request.quantity&&
    request.material_snapshot.branch_known&&spirit_active_job.branch_value==request.material_snapshot.branch_value&&
    recipe&&recipe->spirit&&recipe->spirit->callback->symbol==spirit_active_job.callback&&
    static_cast<std::uint32_t>(recipe->spirit->owner.instance)==spirit_active_job.owner&&key(c)==request.key&&
    gdebit::same(extra_context(c),spirit_active_job.context)&&
    spirit_writer_scope->call.thread==GetCurrentThreadId()&&
    spirit_writer_scope->call.symbol==static_cast<int>(spirit_active_job.callback)&&
    spirit_writer_scope->call.caller==bc::info_call_return&&current_info_effect(c);
}
void log_tobacco_check(const ubc::GrindCheck& check,int subject) noexcept {
  if(!check.field)return;
  static ubc::GrindCheck last[3]{};
  const auto index=subject>=0xb001&&subject<=0xb003?subject-0xb001:0;
  if(last[index].same(check))return;
  last[index]=check;char fields[176]{};check.format(fields);
  event("tobacco_check_rejected",subject,check.symbol,fields);
}
bool tobacco_quantity_path(const Context& c,const tobacco::Descriptor& recipe,const tobacco::Capture& sample,
    std::uint32_t quantity,ubc::GrindCheck& check) noexcept {
  if(quantity==1)return true;
  if(!check.equal("debit.initialized",true,tobacco_list.has_value()))return false;
  const auto ctx=extra_context(c);const auto modules=extra_modules();
  for(std::size_t i=0;i<2;++i)
    if(!ubc::tobacco_writer::debit_path(memory,*tobacco_list,ctx,modules,recipe,sample,i,quantity,check))return false;
  return check.equal("debit.context_stable",true,same_context(c,context()));
}
bool read_tobacco_state(const Context& c,const tobacco::Descriptor& d,bool script,
    ubc::CraftingState& state,tobacco::Capture& sample,ubc::GrindCheck& check) noexcept {
  state=ubc::make_crafting_state(d.id);state.evaluated=true;state.generation=generation;
  // Neither the owner nor the child reads a learning variable. The possession check belongs to the material check.
  state.knowledge_read_valid=state.known=script;
  if(!script||!check.equal("station.physical",true,c.valid&&c.kind==cat::Station::alchemy&&c.dialog.npc==c.hero&&c.dialog.player==c.hero)||
     !tobacco_reader.capture(memory,d,extra_context(c),sample,check))return false;
  if(!tobacco::lab_flags(memory,c.hero,true,check)||
     !check.equal("station.end_path",0,static_cast<int>(end_path(c.station).issue)))return false;
  const auto actor=grt::actor(memory,s::game_parser,grt::assemble(gcat::recipes[0]),c.hero);
  if(!check.equal("actor.hero",c.hero,actor.read_valid?actor.hero:0)||
     !check.equal("station.same",true,same_context(c,context())))return false;
  state.contract=state.actor_known=state.actor_matches=state.context_known=state.context_matches=true;
  state.material_known=state.material_verified=state.hand_known=true;
  for(std::size_t i=0;i<sample.stock.size();++i)state.materials[i].stored=sample.stock[i];
  state.materials[1].consumed_reserve=1;
  state.maximum=tobacco::maximum(sample.stock);
  if(!tobacco_owner.ticket||!handoff.active()){
    state.maximum=tobacco::permitted_maximum(sample.stock,[&](std::uint32_t quantity){
      ubc::GrindCheck path;
      if(tobacco_quantity_path(c,d,sample,quantity,path))return true;
      log_tobacco_check(path,d.id);return false;
    });
  }
  return true;
}
Capture capture(const Context& c,const GrindPreviewLease* shared_grind=nullptr,ubc::BowMenuTrace* trace=nullptr) noexcept {
  const bool outer=frame_depth==1&&!update_depth&&!info_depth&&!choice_depth;
  const bool callback=native_update&&update_depth==1&&info_depth+choice_depth<=1;
  const bool boundary=(outer||callback)&&!station_depth&&!collection_depth;
  Capture captured;
  if(c.kind==cat::Station::stove||c.kind==cat::Station::alchemy){
    const auto kind=c.kind==cat::Station::stove?basecat::Station::stove:basecat::Station::alchemy;
    std::array<bool,basecat::recipes.size()> contracts{};
    for(std::size_t i=0;i<contracts.size();++i)contracts[i]=c.valid&&basecat::recipes[i].station==kind;
    const ubc::StationKey k{generation,GetCurrentThreadId(),c.world,c.hero,c.station,s::information_manager,kind};
    const bool shared=c.kind==cat::Station::alchemy&&extra_catalogue_reader&&spirit_catalogue_reader;
    ubc::CraftingBaseAlchemyEvidence base_evidence;
    ubc::CraftingAlchemyExtraInput shared_input;
    const auto spirit_profile=grt::assemble(gcat::recipes[0]);
    Context base_before;
    if(shared){
      shared_input=extra_read_input(c,boundary);base_before=context();
      base_evidence.serial=shared_input.serial;base_evidence.context_before=extra_context(base_before);
      base_evidence.manager_before=base_before.dialog.valid?s::information_manager:0;
      const auto production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
      if(production.valid)base_evidence.production_before=production.value;
      base_evidence.actor_before=grt::actor(memory,s::game_parser,spirit_profile,c.hero);
    }
    const auto source=catalogue_reader.capture(memory,s::game_parser,k,c.valid&&boundary,contracts);
    captured.snapshot=ubc::project_capture(source);captured.issue=static_cast<int>(source.diagnostic.issue);
    if(c.kind==cat::Station::alchemy&&extra_catalogue_reader){
      const auto extra=extra_catalogue_reader->capture(memory,shared?shared_input:extra_read_input(c,boundary));
      if(shared){
        const auto spirit=spirit_catalogue_reader->capture(memory,{shared_input,nullptr,read_spirit_actual_info});
        base_evidence.actor_after=grt::actor(memory,s::game_parser,spirit_profile,c.hero);
        const auto production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
        if(production.valid)base_evidence.production_after=production.value;
        const auto after=context();base_evidence.context_after=extra_context(after);
        base_evidence.manager_after=after.dialog.valid?s::information_manager:0;
        base_evidence.context_unchanged=same_context(c,base_before)&&same_context(base_before,after);
        base_evidence.actor_unchanged=grt::same_actor_evidence(base_evidence.actor_before,base_evidence.actor_after);
        base_evidence.boundary_verified=shared_input.boundary.verified&&base_evidence.context_unchanged;
        captured.snapshot=ubc::project_capture(source,base_evidence,extra.capture,spirit);
      }else captured.snapshot=ubc::project_capture(source,extra.capture);
    }
    if(c.kind==cat::Station::alchemy&&boundary&&c.valid&&ubc::valid_crafting_snapshot(captured.snapshot)){
      ubc::GrindCheck script_check;
      const bool script=tobacco::verify_script(memory,script_check);
      for(std::size_t i=0;i<tobacco::recipes.size();++i){
        auto& state=captured.snapshot.recipes[cat::tobacco_begin+i];
        ubc::GrindCheck check=script_check;
        tobacco::Capture sample;
        if(!read_tobacco_state(c,tobacco::recipes[i],script,state,sample,check)||
           !check.equal("preview.hands",true,ubc::same_crafting_hands(captured.snapshot.hands,sample.hands)))state.contract=false;
        if(check.field)log_tobacco_check(check,state.id);
      }
      if(!ubc::crafting_detail::stocks_agree(captured.snapshot))captured.snapshot.issue=ubc::CraftingProjectionIssue::source;
    }
  }else if(c.kind==cat::Station::grind){
    captured.snapshot=capture_grind(c,boundary,shared_grind);captured.issue=static_cast<int>(captured.snapshot.issue);
  }else if(c.kind==cat::Station::cool){
    const auto source=capture_cool(c,boundary);
    captured.snapshot=ubc::project_capture(source,ubc::CoolActorPurpose::hero_preview,
      c.valid&&boundary&&c.dialog.npc==c.hero&&c.dialog.player==c.hero);
    captured.issue=static_cast<int>(source.diagnostic.issue);
  }else if(c.kind==cat::Station::bow){
    capture_bow(captured,c,generation,boundary,trace);
  }else if(c.kind==cat::Station::anvil){
    const bool proof=c.valid&&boundary&&end_path(c.station).valid;
    const ubc::AnvilRecipeKey k{generation,GetCurrentThreadId(),c.world,c.hero,c.station,s::information_manager,acat::Station::anvil};
    const auto ordinary=anvil_catalogue_reader.capture(memory,s::game_parser,k,{c.valid&&boundary,proof});
    const ubc::OreRecipeKey ore_key{generation,GetCurrentThreadId(),c.world,c.hero,c.station,s::information_manager,ocat::Station::anvil};
    const auto ore=ore_catalogue_reader.capture(memory,s::game_parser,ore_key,{c.valid&&boundary,proof});
    captured.snapshot=ubc::project_capture(ordinary,ore,
      c.valid&&boundary&&c.dialog.npc==c.hero&&c.dialog.player==c.hero);
    captured.issue=static_cast<int>(ordinary.diagnostic.issue)|static_cast<int>(ore.diagnostic.issue);
  }else{
    const auto kind=station_kind(c.kind);std::array<bool,scat::recipes.size()> contracts{};
    for(std::size_t i=0;i<contracts.size();++i)contracts[i]=c.valid&&scat::recipes[i].station==kind;
    const ubc::StationRecipeKey k{generation,GetCurrentThreadId(),c.world,c.hero,c.station,s::information_manager,kind};
    const auto source=station_catalogue_reader.capture(memory,s::game_parser,k,c.valid&&boundary,contracts);
    captured.snapshot=ubc::project_capture(source);captured.issue=static_cast<int>(source.diagnostic.issue);
    if(c.kind==cat::Station::saw)ubc::project_saw_actor_preview(captured.snapshot,
      source.diagnostic.actor_before,source.diagnostic.actor_after,source.diagnostic.actor_unchanged,
      c.valid&&boundary&&c.dialog.npc==c.hero&&c.dialog.player==c.hero);
  }
  if(c.valid&&(c.kind==cat::Station::alchemy||c.kind==cat::Station::leather||c.kind==cat::Station::forge||c.kind==cat::Station::anvil||c.kind==cat::Station::bow||c.kind==cat::Station::cool||c.kind==cat::Station::grind)){
    const auto& cool=captured.snapshot.recipes[cat::cool_begin];
    const auto path=end_path(c.station,cpre::runtime_ready&&c.kind==cat::Station::cool&&cool.material_known&&cool.maximum==2);
    const auto issue=static_cast<int>(path.issue);
    if(issue!=last_end_path_issue){last_end_path_issue=issue;event("end_path_issue",issue);}
    if(!path.valid)for(auto& state:captured.snapshot.recipes)state.contract=false;
  }
#ifdef UBC_PLAYER_RELEASE
  for (std::size_t i = 0; i < player_recipes.size(); ++i)
    if (!player_recipes[i]) {
      captured.snapshot.recipes[i].contract = false;
      captured.snapshot.recipes[i].known = false;
    }
#endif
  captured.hands=captured.snapshot.hands;
  const bool current=same_context(c,context());
  if(trace)trace->equal("catalogue.same_context",1,current);
  if(!current)captured.snapshot.key={};
  if((!result_guard.permits_next()&&!active_extra_admission(c)&&!active_spirit_admission(c))||adapter_failed())
    for(auto& recipe:captured.snapshot.recipes)recipe.contract=false;
  if(c.kind==cat::Station::saw&&!ammo::permits_next())
    for(auto& recipe:captured.snapshot.recipes)recipe.contract=false;
  return captured;
}
const ubc::CraftingState* state_for(const ubc::CraftingSnapshot& snapshot,cat::RecipeId id) noexcept {
  for(const auto& recipe:snapshot.recipes)if(recipe.id==id)return &recipe;
  return nullptr;
}
void observe_bow_quantity(const Context& c) noexcept {
  const auto* selected=session.selected();if(!selected||session.preview().key.kind!=cat::Station::bow)return;
  const ubc::CraftingRequest request{selected->id,session.quantity(),session.preview().key,*selected,session.preview().hands};
  const auto* recipe=cat::find(request.id);const auto bridge=bc::diagnostics();bool available=false;
  const bool quiet=c.valid&&key(c)==request.key&&own_thread()&&frame_depth==1&&!update_depth&&!info_depth&&!choice_depth&&
    !station_depth&&!collection_depth&&!resolved_read_depth&&!bridge.writer&&!bridge.readers&&!bridge.writes_disabled&&
    !amount_patch.active()&&!handoff.active()&&result_guard.permits_next();
  if(quiet&&recipe&&recipe->bow&&bow_modules_current()&&ubc::crafting_handoff_detail::valid_request(request)&&end_path(c.station).valid){
    const auto proof=bow_item_reader->capture(memory,*recipe->bow,request.quantity,bow_read_input(request.key));
    log_bow_check("preview.item_path",0,static_cast<int>(proof.issue),static_cast<int>(request.id),request.quantity,proof.failed_address);
    available=ubc::matching_bow_item_path(proof,request,frame)&&bow_modules_current()&&same_context(c,context());
  }
  session.observe_quantity_check(request,frame,available);
  grind_quantity_observation={true,request.key.generation,frame,request.id,request.quantity};
}
void observe_cool_quantity(const Context& c) noexcept {
  const auto* selected=session.selected();if(!selected||session.preview().key.kind!=cat::Station::cool)return;
  const ubc::CraftingRequest request{selected->id,session.quantity(),session.preview().key,*selected,session.preview().hands};
  const auto bridge=bc::diagnostics();bool available=false;
  const bool quiet=c.valid&&key(c)==request.key&&own_thread()&&frame_depth==1&&!update_depth&&!info_depth&&!choice_depth&&
    !station_depth&&!collection_depth&&!resolved_read_depth&&!bridge.writer&&!bridge.readers&&!bridge.writes_disabled&&
    !amount_patch.active()&&!handoff.active()&&result_guard.permits_next()&&!cool_destroy_depth;
  const auto plan=ubc::crafting_cool_plan(request.material_snapshot,request.quantity);
  const bool predebit=plan.valid&&plan.requires_extra_debit;
  if(quiet&&cool_runtime_path(predebit).valid&&end_path(c.station,predebit).valid){
    const auto collection=rows.generation;
    const auto fresh=capture(c);
    available=ubc::cool_quantity_available(request,fresh.snapshot,cool_info_page(c,true),[]() noexcept {
      if(!cool_predebit_executor||!cool_predebit_reader||cool_predebit_tracker.status().blocked||
         bc::quarantined(cpre::callback)||!cool_engine_current()||!end_compatibility(true)||!extra_zpe_module.current()||!extra_crt_module.current())return false;
      const auto d=cool_predebit_executor->diagnostics();
      return d.configured&&!d.disabled&&!d.active&&!d.native_scope;
    })&&collection==rows.generation&&same_context(c,context());
  }
  session.observe_quantity_check(request,frame,available);
}
void observe_grind_quantity(const Context& c,const GrindPreviewLease* shared=nullptr) noexcept {
  const auto* selected=session.selected();if(!selected||session.preview().key.kind!=cat::Station::grind)return;
  grind_quantity_observation={};
  const ubc::CraftingRequest request{selected->id,session.quantity(),session.preview().key,*selected,session.preview().hands};
  const auto* recipe=cat::find(request.id);const auto bridge=bc::diagnostics();bool available=false;
  const bool quiet=c.valid&&key(c)==request.key&&own_thread()&&frame_depth==1&&!update_depth&&!info_depth&&!choice_depth&&
    !station_depth&&!collection_depth&&!resolved_read_depth&&!bridge.writer&&!bridge.readers&&!bridge.writes_disabled&&
    !amount_patch.active()&&!handoff.active()&&result_guard.permits_next()&&!cool_destroy_depth;
  if(shared){
    const auto* closed=quiet?shared->view(grind_key(c)):nullptr;
    available=closed&&ubc::crafting_handoff_detail::valid_request(request)&&shpreview::quantity_matches(*closed,request);
    session.observe_quantity_check(request,frame,available);
    grind_quantity_observation={true,request.key.generation,frame,request.id,request.quantity};return;
  }
  if(quiet&&recipe&&recipe->grind&&grind_modules_current()&&ubc::crafting_handoff_detail::valid_request(request)){
    const auto k=grind_key(c);const GrindCaptureLease lease{k,true};
    if(!lease.value){
      session.observe_quantity_check(request,frame,false);
      grind_quantity_observation={true,request.key.generation,frame,request.id,request.quantity};return;
    }
    const auto& raw=*lease.value;
    const auto physical=ubc::read_grind_guild_for_preview(memory,s::game_parser,k);
    const auto station=grind_station_proof(c,raw,true);
    if(station.station_hand_end_verified&&matching_grind_knowledge(ubc::grind_preview_knowledge(raw,physical,k),request)){
      const auto item=grind_item_reader->capture_preview(memory,raw,physical,recipe->grind->function.symbol,request.quantity,
        grind_read_input(k,station.stable_boundary_verified));
      available=matching_grind_item(item,request,ubc::grind_item_path::Scope::preview)&&same_context(c,context());
    }
  }
  session.observe_quantity_check(request,frame,available);
  grind_quantity_observation={true,request.key.generation,frame,request.id,request.quantity};
}
ubc::CraftingHandoffObservation observation(const Context& c,bool outer,bool materials=true) noexcept {
  ubc::CraftingHandoffObservation out;
  out.key=key(c);out.view=c.dialog.view;out.frame=frame;out.collection=rows.generation;
  // Missing production or profile information does not establish a station change.
  // If the kind is unknown, the handoff retains its previous input boundary.
  out.context_known=c.valid&&!foreign_dialog(c);
  out.mode=c.dialog.mode==1?basecat::Invocation::info:basecat::Invocation::choice;
  out.count=c.dialog.count;out.complete=complete_collection(c);
  out.ready=c.valid&&c.dialog.waiting&&c.dialog.open&&!c.dialog.done;
  out.outer_frame=outer;out.quiescent=outer&&frame_depth==1&&!update_depth&&!info_depth&&!choice_depth&&!station_depth&&!collection_depth;
  out.focused=ui.focused();out.page=page(c);
  out.page_known=c.valid&&ubc::valid_crafting_page(out.page);
  if(c.dialog.mode==2)out.choice_owner=ubc::read_selection_info(memory,c.dialog.info).instance;
  if(materials&&c.valid){
    const auto& request=handoff.request();
    const auto* d=cat::find(request.id);
    if(d&&d->tobacco&&handoff.active()){
      ubc::GrindCheck check;tobacco::Capture sample;
      const bool script=tobacco::verify_script(memory,check);
      bool fresh=read_tobacco_state(c,*d->tobacco,script,out.materials,sample,check);
      if(fresh&&tobacco_owner.ticket&&tobacco_owner.key==out.key){
        fresh=tobacco::match_stock(*d->tobacco,tobacco::stocks(request.material_snapshot),sample.stock,check);
        out.materials.maximum=request.material_snapshot.maximum;
        if(fresh&&out.complete&&out.ready&&out.page.kind==ubc::CraftingPageKind::tobacco_choices)
          fresh=check.equal("owner.collection",true,tobacco_owner.bind(out.collection));
      }
      if(!fresh){out.materials.contract=false;log_tobacco_check(check,d->id);}
      out.hands=sample.hands;
      return out;
    }
    const auto current=capture(c);
    if(const auto* material=state_for(current.snapshot,handoff.request().id))out.materials=*material;
    out.hands=current.hands;
    if(!(current.snapshot.key==out.key))out.context_known=false;
  }
  return out;
}
ubc::CraftingHandoffOffer resolve_offer(const Context& c,const ubc::CraftingStep* step) noexcept {
  if(!step||!complete_collection(c))return {};
  ubc::CraftingHandoffOffer offer;
  if(step->invocation==basecat::Invocation::info&&c.dialog.mode==1){
    for(std::size_t i=0;i<rows.count;++i){
      const auto& row=rows.rows[i];
      if(linked_step_info(row,step)&&row.instance==step->owner_instance&&row.function==step->function_symbol){
        if(offer.valid)return {};
        offer={true,basecat::Invocation::info,row.instance,row.function,static_cast<std::int32_t>(i),rows.generation};
      }
    }
  }else if(step->invocation==basecat::Invocation::choice&&c.dialog.mode==2){
    ubc::SelectionChoices choices;
    if(!complete_collection(c,&choices)||choices.owner.instance!=step->owner_instance)return {};
    for(std::size_t i=0;i<choices.count;++i)if(choices.functions[i]==step->function_symbol){
      if(offer.valid)return {};
      offer={true,basecat::Invocation::choice,choices.owner.instance,choices.functions[i],static_cast<std::int32_t>(i),rows.generation};
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
void close_ui(const Context& c) noexcept {close_grind_readiness();ui_diagnostic.end();ui.close();restore_view(c);}
void borrow_view(const Context& c) noexcept {
  borrowed_view=c.dialog.view;borrowed_context=c;
  reinterpret_cast<a::Select>(a::choice_activate)(reinterpret_cast<void*>(borrowed_view),0);
  reinterpret_cast<a::FastObject>(a::choice_close_immediate)(reinterpret_cast<void*>(borrowed_view),nullptr);
}
void record_materials(const ubc::CraftingState& value,const char* name) noexcept {
  for(std::size_t i=0;i<value.target_count;++i)event(name,value.materials[i].symbol,value.materials[i].stored);
}
void drive_handoff(const Context& c) noexcept {
  if(!handoff.active())return;
  const auto current=observation(c,true);
  if(GetTickCount64()-handoff_started>30000)handoff.cancel(ubc::CraftingHandoffIssue::timeout);
  handoff.observe(current);
  if(foreign_dialog(c))handoff.cancel(ubc::CraftingHandoffIssue::context);
  if(handoff.state()==ubc::CraftingHandoffState::quarantined){
    bool finished=c.identity_known&&c.station_known&&integer(s::information_manager,0x2c)==1;
    for(const auto offset:{0x30u,0x34u,0x38u,0x3cu,0x40u,0x44u})
      if(integer(s::information_manager,offset)!=0)finished=false;
    if(finished)handoff.context_ended({handoff.request().key,dispatched_ticket.view,frame,true,true,true});
  }
  if(handoff.state()!=ubc::CraftingHandoffState::awaiting_step)return;
  if(!current.ready||!current.complete||!ubc::NativeSelectionUi::callback_is_top(c.dialog.view+a::view_callback)||
    integer(0x8d1e38,8)!=0){handoff.cancel(ubc::CraftingHandoffIssue::boundary);return;}
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
  if(!after.valid||!after.done||after.selected!=offer.row)handoff.cancel(ubc::CraftingHandoffIssue::offer);
  event("handoff_step",offer.function,static_cast<int>(ticket->step));
}
const basecat::Callback* callback_for(const cat::Descriptor&,std::uint32_t) noexcept;
bool extra_result_complete(const Context& c,const cat::Descriptor& recipe) noexcept {
  if(!extra_executor||!recipe.alchemy_extra){result_guard.fail(ubc::CraftingResultIssue::contract);return false;}
  if(!extra_normal_end||frame<=extra_end_frame)return false;
  const auto quiet=[]() noexcept {
    const auto bridge=bc::diagnostics();
    return own_thread()&&frame_depth==1&&!native_update&&!update_depth&&!info_depth&&!choice_depth&&
      !station_depth&&!collection_depth&&!resolved_read_depth&&!amount_patch.active()&&!extra_writer_scope&&
      !bridge.writer&&!bridge.readers&&!bridge.writes_disabled&&extra_executor&&
      !extra_executor->diagnostics().active&&!extra_executor->diagnostics().disabled;
  };
  if(!quiet()||!c.identity_known||!c.station_known)return false;
  ubc::AlchemyExtraEndFrame end;
  end.before=extra_context(c);end.current_thread=GetCurrentThreadId();
  if(!xdebit::same(end.before,extra_active_job.context,false)){
    result_guard.fail(ubc::CraftingResultIssue::context);return false;
  }
  // Re-establish the semantic bindings for C_NPC/AIVAR/index and production.
  // The direct AIVAR read is meaningful only with these exact current bindings.
  namespace xr=ubc::alchemy_flask_runtime;
  rt::detail::Layout layout{};
  if(rt::detail::read_layout(memory,s::game_parser,xr::flask_profile,layout)!=rt::Issue::none)return false;
  for(const auto index:{4u,7u,8u,9u})
    if(rt::detail::check_binding(memory,layout,xr::flask_bindings[index])!=rt::Issue::none)return false;
  auto production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
  if(!production.valid||!field(c.hero,0x298,end.invincible_before))return false;
  end.production_before=production.value;
  if(ubc::station_recipe_snapshot_detail::begin_hands(memory,c.hero,result_hands)!=ubc::StationHandReadIssue::none)return false;
  end.hands_before=result_hands.before;
  const auto inventory=ubc::read_packed_inventory<cat::target_capacity>(memory,c.hero,
    recipe.targets.data,recipe.targets.count,true,result_workspace);
  std::array<ubc::RecipeInventoryItem,cat::target_capacity> items{};
  for(std::size_t i=0;i<recipe.targets.count;++i)items[i]={recipe.targets[i].symbol,
    inventory.listed[i],inventory.packed[i],inventory.total[i],inventory.listed_occurrences[i],
    inventory.packed_occurrences[i],inventory.duplicate_packed_target[i],inventory.has_active_marker[i],inventory.mixed_representation[i]};
  if(!ubc::station_recipe_snapshot_detail::verify_hands(memory,result_hands)||
    rt::detail::stable_layout(memory,s::game_parser,xr::flask_profile,layout)!=rt::Issue::none)return false;
  end.hands_after=result_hands.before; // verify_hands has reread the same full snapshot.
  production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
  if(!production.valid||!field(c.hero,0x298,end.invincible_after))return false;
  end.production_after=production.value;
  const auto after=context();if(!after.identity_known||!after.station_known||!quiet())return false;
  end.after=extra_context(after);end.serial=next_extra_serial();end.boundary_verified=true;
  // Both scopes are actually quiescent. Before and after the snapshot,
  // quiet() checked TLS, writer and executor state. No simulated phases.
  end.inventory={inventory.header_known,inventory.listed_known,inventory.packed_known,
    inventory.storage_totals_known,inventory.stable_boundary_verified,inventory.snapshot_unchanged,
    inventory.issue,inventory.listed_nodes,inventory.packed_entries,{items.data(),recipe.targets.count},{}};
  if(!inventory.snapshot_unchanged||inventory.issue!=ubc::PackedInventoryIssue::none)return false;
  const auto snapshot=ubc::alchemy_extra_end_snapshot(*recipe.alchemy_extra,extra_active_job,end);
  if(!snapshot.valid){result_guard.fail(ubc::CraftingResultIssue::contract);event("extra_end_snapshot_rejected",static_cast<int>(snapshot.issue));return false;}
  const auto stage=extra_executor->observe_completion(snapshot.observation,extra_end,extra_normal_end);
  if(stage==ubc::AlchemyExtraResultStage::blocked){result_guard.fail(ubc::CraftingResultIssue::stock);return false;}
  const auto& receipt=extra_executor->receipt();
  const bool complete=stage==ubc::AlchemyExtraResultStage::complete&&receipt.verified&&
    receipt.job==extra_active_job.id&&receipt.writer_token==extra_active_job.writer_token&&
    receipt.id==recipe.alchemy_extra->id&&receipt.quantity==extra_active_job.quantity;
  if(complete)event("extra_result_verified",static_cast<int>(recipe.id),static_cast<int>(receipt.quantity));
  return complete;
}
bool spirit_result_complete(const Context& c,const cat::Descriptor& recipe,ubc::CraftingResultObservation& value) noexcept {
  if(!spirit_executor||!recipe.spirit){result_guard.fail(ubc::CraftingResultIssue::contract);return false;}
  if(!spirit_normal_end||frame<=spirit_end_frame)return false;
  const auto quiet=[]() noexcept {
    const auto bridge=bc::diagnostics();
    return own_thread()&&frame_depth==1&&!native_update&&!update_depth&&!info_depth&&!choice_depth&&
      !station_depth&&!collection_depth&&!resolved_read_depth&&!amount_patch.active()&&!spirit_writer_scope&&
      !bridge.writer&&!bridge.readers&&!bridge.writes_disabled&&spirit_executor&&
      !spirit_executor->diagnostics().active&&!spirit_executor->diagnostics().disabled;
  };
  if(!quiet()||!c.identity_known||!c.station_known)return false;
  ubc::SpiritEndFrame end;
  end.before=extra_context(c);end.current_thread=GetCurrentThreadId();
  if(!gdebit::same(end.before,spirit_active_job.context,false)){
    result_guard.fail(ubc::CraftingResultIssue::context);return false;
  }
  // Re-establish the semantic bindings for C_NPC/AIVAR/index and production.
  // The direct AIVAR read is meaningful only with these exact current bindings.
  namespace xr=ubc::alchemy_flask_runtime;
  rt::detail::Layout layout{};
  if(rt::detail::read_layout(memory,s::game_parser,xr::flask_profile,layout)!=rt::Issue::none)return false;
  for(const auto index:{4u,7u,8u,9u})
    if(rt::detail::check_binding(memory,layout,xr::flask_bindings[index])!=rt::Issue::none)return false;
  auto production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
  if(!production.valid||!field(c.hero,0x298,end.invincible_before))return false;
  end.production_before=production.value;
  const auto profile=grt::assemble(*recipe.spirit);
  end.branch_before=grt::branch(memory,s::game_parser,profile);
  if(ubc::station_recipe_snapshot_detail::begin_hands(memory,c.hero,result_hands)!=ubc::StationHandReadIssue::none)return false;
  end.hands_before=result_hands.before;
  const auto inventory=ubc::read_packed_inventory<cat::target_capacity>(memory,c.hero,
    recipe.targets.data,recipe.targets.count,true,result_workspace);
  std::array<ubc::RecipeInventoryItem,cat::target_capacity> items{};
  for(std::size_t i=0;i<recipe.targets.count;++i)items[i]={recipe.targets[i].symbol,
    inventory.listed[i],inventory.packed[i],inventory.total[i],inventory.listed_occurrences[i],
    inventory.packed_occurrences[i],inventory.duplicate_packed_target[i],inventory.has_active_marker[i],inventory.mixed_representation[i]};
  if(!ubc::station_recipe_snapshot_detail::verify_hands(memory,result_hands)||
    rt::detail::stable_layout(memory,s::game_parser,xr::flask_profile,layout)!=rt::Issue::none)return false;
  end.hands_after=result_hands.before; // verify_hands has reread the same full snapshot.
  production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
  if(!production.valid||!field(c.hero,0x298,end.invincible_after))return false;
  end.production_after=production.value;
  end.branch_after=grt::branch(memory,s::game_parser,profile);
  const auto after=context();if(!after.identity_known||!after.station_known||!quiet())return false;
  end.after=extra_context(after);end.serial=next_extra_serial();end.boundary_verified=true;
  // Both scopes are actually quiescent. Before and after the snapshot,
  // quiet() checked TLS, writer and executor state. No simulated phases.
  end.inventory={inventory.header_known,inventory.listed_known,inventory.packed_known,
    inventory.storage_totals_known,inventory.stable_boundary_verified,inventory.snapshot_unchanged,
    inventory.issue,inventory.listed_nodes,inventory.packed_entries,{items.data(),recipe.targets.count},{}};
  if(!inventory.snapshot_unchanged||inventory.issue!=ubc::PackedInventoryIssue::none)return false;
  const auto snapshot=ubc::spirit_end_snapshot(spirit_active_job,end);
  if(!snapshot.valid){result_guard.fail(ubc::CraftingResultIssue::contract);event("spirit_end_snapshot_rejected",static_cast<int>(snapshot.issue));return false;}
  const auto stage=spirit_executor->observe_completion(snapshot.observation,spirit_end,spirit_normal_end);
  if(stage==ubc::SpiritResultStage::blocked){result_guard.fail(ubc::CraftingResultIssue::stock);return false;}
  const auto& receipt=spirit_executor->receipt();
  const bool complete=stage==ubc::SpiritResultStage::verified&&receipt.verified&&
    receipt.job==spirit_active_job.id&&receipt.writer_token==spirit_active_job.writer_token&&
    receipt.quantity==spirit_active_job.quantity&&receipt.branch_value==spirit_active_job.branch_value&&
    receipt.output==handoff.request().material_snapshot.branch_output&&receipt.count==recipe.targets.count;
  if(complete){
    if(!value.stock_known||value.items.count!=receipt.count){result_guard.fail(ubc::CraftingResultIssue::stock);return false;}
    for(std::size_t i=0;i<receipt.count;++i){
      const auto& row=receipt.rows[i];bool matched=false;
      for(const auto& stock:value.items)if(stock.symbol==static_cast<std::int32_t>(row.symbol)&&stock.total==row.actual)matched=true;
      if(!matched){result_guard.fail(ubc::CraftingResultIssue::stock);return false;}
    }
    value.branch={true,true,spirit_active_job.id,recipe.id,spirit_active_job.quantity,receipt.branch_value};
  }
  if(complete)event("spirit_result_verified",static_cast<int>(recipe.id),static_cast<int>(receipt.quantity));
  return complete;
}
void observe_result(const Context& c) noexcept {
  if(result_guard.state()!=ubc::CraftingResultState::waiting)return;
  const auto& request=handoff.request();const auto* recipe=cat::find(request.id);
  ubc::CraftingResultObservation value;
  value.generation=request.key.generation;value.frame=frame;value.stable_boundary_verified=true;
  value.identity_known=c.identity_known;value.world=c.world;value.hero=c.hero;
  value.station_known=c.station_known;value.station=c.station;
  const auto production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
  value.production_known=production.valid;value.production=production.value;
  value.selection_known=c.dialog.valid;
  value.selection_owner_verified=c.dialog.valid&&c.dialog.npc==request.key.hero&&c.dialog.player==request.key.hero;value.selection_open=c.dialog.open;value.selection_waiting=c.dialog.waiting;
  value.selection_done=c.dialog.done;
  value.collection_known=c.valid&&complete_collection(c);value.collection=rows.generation;
  std::array<ubc::RecipeResultItem,cat::target_capacity> items{};
  if(recipe&&c.identity_known&&c.world==request.key.world&&c.hero==request.key.hero){
    const auto inventory=ubc::read_packed_inventory<cat::target_capacity>(memory,c.hero,
      recipe->targets.data,recipe->targets.count,true,result_workspace);
    value.stock_known=inventory.storage_totals_known&&inventory.snapshot_unchanged&&
      inventory.stable_boundary_verified&&inventory.issue==ubc::PackedInventoryIssue::none;
    for(std::size_t i=0;i<recipe->targets.count;++i)items[i]={recipe->targets[i].symbol,inventory.total[i]};
    value.items={items.data(),recipe->targets.count};
    value.hands_known=ubc::station_recipe_snapshot_detail::begin_hands(memory,c.hero,result_hands)==ubc::StationHandReadIssue::none&&
      ubc::station_recipe_snapshot_detail::verify_hands(memory,result_hands);
    if(value.hands_known)value.hands=result_hands.before;
    if(recipe->grind)value.grind=read_grind_result_values(request);
    const auto fresh=context();
    if(fresh.world!=c.world||fresh.hero!=c.hero||fresh.station!=c.station)value.stock_known=false;
  }
  if(recipe&&recipe->tobacco&&value.identity_known&&value.station_known&&!c.station){
    ubc::GrindCheck check;
    if(!tobacco::lab_flags(memory,c.hero,false,check)){
      log_tobacco_check(check,recipe->id);result_guard.fail(ubc::CraftingResultIssue::context);
    }
  }
  if((!extra_job||(recipe&&extra_result_complete(c,*recipe)))&&
    (!spirit_job||(recipe&&spirit_result_complete(c,*recipe,value))))result_guard.observe(value);
  const auto elapsed=GetTickCount64()-result_started;
  if(elapsed>30000)result_guard.timeout();
  if(result_guard.state()==ubc::CraftingResultState::verified){
    handoff.acknowledge_result(true);
    for(std::size_t i=0;i<value.items.count;++i)event("stock_verified",value.items[i].symbol,static_cast<int>(value.items[i].total));
    event("amount_result_verified",static_cast<int>(request.id),static_cast<int>(request.quantity));
    ubc::CraftingResultSuccess success;
    if(!result_guard.success(success)){result_guard.fail(ubc::CraftingResultIssue::contract);return;}
    if(ammo_job){
      const auto* cb=recipe?callback_for(*recipe,request.quantity):nullptr;
      const auto* profile=cb?ap::find(cb->function->symbol):nullptr;
      std::int64_t delta=-1;
      if(profile)for(std::size_t i=0;i<success.stock_count;++i)
        if(success.deltas[i].symbol==static_cast<std::int32_t>(profile->output))delta=success.deltas[i].delta;
      ap::Context current;
      if(!profile||!read_ammo_context(nullptr,current)||!ammo::observe_result({current,success.receipt,
        profile->callback,request.quantity,profile->output,delta,ap::ResultState::matched_ready,true})){
        result_guard.fail(ubc::CraftingResultIssue::contract);return;
      }
    }
    pending_success=ubc::crafting_ui::success_notice(success);
    success_pending=true;
    remembered_world=success.key.world;remembered_hero=success.key.hero;
    remembered_station=success.key.station;remembered_recipe=success.id;

  }else if(result_guard.state()==ubc::CraftingResultState::failed){
    if(grind_job)log_grind_check(ubc::diagnose_grind_result(result_guard.issue(),grind_prepared,value,elapsed),
                                static_cast<int>(request.id));
    event("amount_result_failed",static_cast<int>(result_guard.issue()));
  }
}

bool render_selection(const Context* observed=nullptr,const uidiag::Frame* poll=nullptr,bool poll_known=false) noexcept {
  const bool witness=grind_ui_trace_active&&observed&&poll;
  gready::Render readiness;
  if(witness){
    readiness.epoch=session.preview().key.generation;readiness.frame=frame;
    readiness.input_serial_before=grind_ui_readiness.input_serial();
    readiness.before=ui.readiness_state(session);readiness.quantity=grind_quantity_observation;
    readiness.focused=poll->focused;readiness.held=poll->held;readiness.poll_known=poll_known;
  }
  const bool rendered=ui.render(session,execution_enabled);
  if(rendered&&ui.owns_input())rendered_generation=session.preview().key.generation;
  else rendered_generation=0;
  if(witness){
    readiness.after=ui.readiness_state(session);readiness.rendered=rendered;
    readiness.focused=readiness.focused&&ui.focused();
    readiness.owns_input=ui.owns_input();readiness.outer_quiet=grind_frame_capture_quiet();
    const auto* selected_recipe=cat::find(readiness.after.recipe);
    readiness.context_bound=observed->valid&&selected_recipe&&selected_recipe->station==observed->kind&&
      (observed->kind==cat::Station::grind||observed->kind==cat::Station::bow)&&
      key(*observed)==session.preview().key&&same_context(*observed,borrowed_context)&&
      same_context(*observed,context());
    grind_readiness_events(grind_ui_readiness.after_render(readiness));
  }
  return rendered;
}

bool open_failure_notice() noexcept {
  if(ui.open_failure()){event("failure_notice_opened");return true;}
  if(!failure_open_rejected_logged){failure_open_rejected_logged=true;event("failure_notice_open_rejected");}
  return false;
}

void maybe_install_ammo(const Context& c) noexcept {
  if(!own_thread()||frame_depth!=1||update_depth||info_depth||choice_depth||station_depth||collection_depth)return;
  // Attempt once, only after the saw dialog is fully initialized.
  // The first world frame may still initialize third-party OnMessage
  // hooks. Never retry after rejection or weaken the guard.
  if(!ammo_install_attempted&&c.valid&&c.kind==cat::Station::saw&&
    c.dialog.waiting&&c.dialog.open&&!c.dialog.done&&complete_collection(c)&&
    !ui.opened()&&!handoff.active()&&!amount_patch.active()){
    ammo_install_attempted=true;
    if(!ammo::install(GetCurrentThreadId())){
      const auto failure=ammo::status();
      event("ammo_hooks_rejected",static_cast<int>(failure.fault),static_cast<int>(failure.runtime_issue));
      event("ammo_compatibility_rejected",static_cast<int>(failure.compatibility.issue),static_cast<int>(failure.compatibility.compatibility));
      event("ammo_compatibility_site",static_cast<int>(failure.compatibility.stage),static_cast<int>(failure.compatibility.address));
    }else event("ammo_hooks_installed");
  }
}
void observe_result_notice(const Context& c) noexcept {
  const bool success=ui.success_notice();
  ui.observe_notice(frame,ui.buttons_held(),ui.focused());
  const auto can_present=[&](const Context& current){
    const bool owner=current.identity_known&&current.world&&current.hero&&
      (!success||(!adapter_failed()&&current.world==pending_success.key.world&&current.hero==pending_success.key.hero));
    if(owner&&ui.owns_input()&&(!success||ui.focused()))return true;
    if(owner&&success&&success_pending&&cool_job&&!cool_receipt_acked&&result_guard.cool_witness().valid&&
       result_guard.state()==ubc::CraftingResultState::verified){
      // The same order waits in its window for focus and input ownership.
      ui.observe_notice(frame,false,false);
    }else ui.dismiss_notice();
    return false;
  };
  const bool ready=can_present(c);
  if(ui.notice_return_ready()){
    if(!success)original_station=c.station;
    close_ui(c);success_pending=false;
  }else if(ready&&success&&!ui.notice_dismissed()){
    const bool rendered=ui.render_success();
    if(!can_present(context()))return;
    if(!rendered)result_guard.fail(ubc::CraftingResultIssue::contract);
    else if(ammo_job&&!ammo_notice_acked&&ui.success_receipt()){
      ammo_notice_acked=ammo::ack_notice(pending_success.receipt,ui.success_receipt(),true);
      if(!ammo_notice_acked)result_guard.fail(ubc::CraftingResultIssue::contract);
    }
    else if(cool_job&&!cool_receipt_acked&&ui.success_receipt()){
      cool_receipt_acked=result_guard.acknowledge_quantity_receipt(ui.success_receipt());
      if(!cool_receipt_acked)result_guard.fail(ubc::CraftingResultIssue::contract);
      else event("cool_quantity_receipt_verified",static_cast<int>(pending_success.id),static_cast<int>(pending_success.quantity));
    }
  }else if(!success)ui.render_failure();
}
// A measurement pass reads at the outer frame even beyond checks that
// block progress. Generation, session, input, window, inventory and
// order status belong exclusively to the subsequent normal menu path.
void measure_bow_menu(const Context& c,std::uint64_t serial,ubc::BowMenuTrace& trace) noexcept {
  bow_entry(c,trace);
  const int mode=c.dialog.valid?c.dialog.mode:integer(s::information_manager,0x54);
  const auto page=ubc::bow_menu::page(memory,s::game_parser,mode,&trace);
  trace.value("entry.page","root/upgrades",static_cast<int>(page),page!=bnav::Page::unknown);
  const auto talent=rt::symbol_int(memory,s::game_parser,10484);
  if(talent.valid)trace.equal("catalogue.talent",1,talent.value,0,10484);
  else trace.check("catalogue.talent","1",rt::issue_name(talent.issue),false,0,10484);
  Capture captured;
  capture_bow(captured,c,serial,true,&trace);
  const bool current=same_context(c,context());
  trace.equal("catalogue.same_context",1,current);
  if(!current)captured.snapshot.key={};
  trace.equal("session.generation",1,session.accepts_generation(serial));
  trace.equal("session.snapshot",1,ubc::valid_crafting_snapshot(captured.snapshot));
  ui.inspect_open(trace);
}
std::uint64_t bow_measured_frame=0;
void publish_bow_sample(const Context& c) noexcept {
  ubc::BowMenuTrace trace{bow_menu_value,true};
  event("bow_menu_begin",static_cast<int>(cat::Station::bow));
  measure_bow_menu(c,generation==UINT64_MAX?0:generation+1,trace);
  event("bow_menu_end",static_cast<int>(cat::Station::bow),trace.count);
  bow_measured_frame=frame;
}
// If input ownership changes later, for example, the frame in which the window
// actually opens needs its own initial snapshot. Failed attempts to open
// the window must not fill the bounded log with identical full snapshots.
struct BowSampleRow {const char* field;char wanted[80],observed[80];int ok,subject,symbol;};
std::array<BowSampleRow,3072> bow_window_sample; // 256 INFO rows with 8 fields each, plus 20 profiles
std::size_t bow_sample_count=0;
void buffer_bow_value(const char* field,const char* wanted,const char* observed,int ok,int subject,int symbol) noexcept {
  if(bow_sample_count<bow_window_sample.size()){
    auto& r=bow_window_sample[bow_sample_count];
    r.field=field;r.ok=ok;r.subject=subject;r.symbol=symbol;
    std::snprintf(r.wanted,sizeof(r.wanted),"%s",wanted);
    std::snprintf(r.observed,sizeof(r.observed),"%s",observed);
  }
  ++bow_sample_count;
}
void publish_window_sample() noexcept {
  if(!bow_sample_count)return;
  event("bow_menu_begin",static_cast<int>(cat::Station::bow));
  for(std::size_t i=0;i<bow_sample_count&&i<bow_window_sample.size();++i){
    const auto& r=bow_window_sample[i];bow_menu_value(r.field,r.wanted,r.observed,r.ok,r.subject,r.symbol);
  }
  // On overflow the evaluator detects missing rows from the expected count.
  event("bow_menu_end",static_cast<int>(cat::Station::bow),static_cast<int>(bow_sample_count));
  bow_sample_count=0;
}
void observe_bow_entry(const Context& c,bool candidate) noexcept {
  static std::uint32_t station=0,world=0,hero=0;
  static ULONGLONG started=0;
  static bool early=false,ready=false;
  if(station!=c.station||world!=c.world||hero!=c.hero){
    station=c.station;world=c.world;hero=c.hero;early=ready=false;
    started=GetTickCount64();bow_rejection_count=0;
  }
  if(!candidate||(!bow_diagnosis&&!bow_comparison)||
     ui.opened()||handoff.active()||amount_patch.active())return;
  if(c.dialog.open&&c.dialog.waiting){
    if(!ready){publish_bow_sample(c);ready=true;}
  }else if(!early&&!ready&&GetTickCount64()-started>=1500){
    publish_bow_sample(c);early=true;
  }
}
void __fastcall after_frame(void* game,void* manager) {
  const auto error=GetLastError();std::uint32_t current=0,owned=0;
  if(!InterlockedCompareExchange(&installed,0,0)||!read(a::game_pointer,current)||
    current!=reinterpret_cast<std::uintptr_t>(game)||!field(reinterpret_cast<std::uint32_t>(manager),0xc,owned)||owned!=current){SetLastError(error);return;}
  const auto thread=static_cast<LONG>(GetCurrentThreadId());const auto owner=InterlockedCompareExchange(&engine_thread,thread,0);
  if((owner&&owner!=thread)||disabled){SetLastError(error);return;}
  if(campfire_observation_mode){
    if(frame!=UINT64_MAX)++frame;
    SetLastError(error);return;
  }
  ++frame_depth;
  if(frame_depth==1&&!update_depth&&!info_depth&&!choice_depth&&!station_depth&&!collection_depth){
    if(frame!=UINT64_MAX)++frame;else disabled=true;
    const auto c=context();
    if(c.world!=observed_world||c.hero!=observed_hero){
      observed_world=c.world;observed_hero=c.hero;
      if(world_epoch!=UINT64_MAX)++world_epoch;else disabled=true;
      remembered_world=remembered_hero=remembered_station=0;remembered_recipe=UINT32_MAX;
    }
    if(tobacco_owner.ticket){
      const bool departed=c.identity_known&&(c.world!=tobacco_owner.key.world||c.hero!=tobacco_owner.key.hero||
        (c.station_known&&c.station!=tobacco_owner.key.station));
      const bool ready=c.valid&&c.dialog.waiting&&c.dialog.open&&!c.dialog.done&&complete_collection(c);
      tobacco_owner.observe(departed,ready,rows.generation,c.dialog.mode,c.dialog.info);
      if(!tobacco_owner.ticket)InterlockedExchange(&tobacco_unpaid_page,0);
    }
    if(grind_measurement_mode){
      initialize_end_compatibility();initialize_cool(c);initialize_grind(c);measure_grind_preview(c);
      --frame_depth;SetLastError(error);return;
    }
    initialize_end_compatibility();
    if(bow_diagnosis){
      initialize_bow(c);observe_bow_entry(c,bow_candidate(c));
      --frame_depth;SetLastError(error);return;
    }
    initialize_extra(c);
    initialize_bow(c);
    initialize_cool(c);
    initialize_grind(c);
    maybe_install_ammo(c);
    observe_result(c);
    observe_ammo_frame_health();
    if(success_pending&&ammo_job&&!ammo_notice_ready&&ammo::healthy()){
      const auto notice=ammo::take_notice();
      if(notice.valid){
        ammo_notice_ready=notice.job==pending_success.receipt&&notice.lots==pending_success.quantity;
        if(!ammo_notice_ready)result_guard.fail(ubc::CraftingResultIssue::contract);
      }
    }
    const int state=(c.identity_known?1:0)|(c.world?2:0)|(c.hero?4:0)|(c.station?8:0)|
      (c.dialog.valid?16:0)|(c.ownership_known?32:0)|(c.valid?64:0)|
      (c.dialog.waiting?128:0)|(c.dialog.open?256:0)|(c.dialog.hidden?512:0)|(c.dialog.done?1024:0);
    if(state!=last_frame_state){last_frame_state=state;event("frame_state",state,c.dialog.mode);}
    if(adapter_failed()){
      const int failed=(result_guard.state()==ubc::CraftingResultState::failed?1:0)|
        (bc::diagnostics().writes_disabled?2:0)|(failure_notified?4:0)|(ui.opened()?8:0)|
        (c.identity_known?16:0)|(c.world?32:0)|(c.hero?64:0)|(integer(0x8d1e38,8)==0?128:0);
      if(failed!=last_failure_state){last_failure_state=failed;event("failure_notice_state",failed,static_cast<int>(result_guard.issue()));}
    }
    if(original_station&&(!c.station_known||c.station!=original_station))original_station=0;
    const bool bow_at_station=bow_candidate(c);
    observe_bow_entry(c,bow_at_station);
    ubc::BowMenuTrace rejection{bow_menu_rejected,false};
    auto* bow_trace=bow_at_station?&rejection:nullptr;
    const bool bow_can_open=bow_at_station&&!ui.opened()&&bow_entry(c,rejection);
    if(ui.opened()){
      if(ui.failure_notice()||ui.success_notice()){
        observe_result_notice(c);
      }else{
        uidiag::Frame diagnostic;diagnostic.started=GetTickCount64();
        diagnostic.before=ui.diagnostic_state(session);
        bool poll_known=false;
        diagnostic.held=ui.buttons_held(&poll_known);diagnostic.focused=ui.focused();
        session.observe(frame,diagnostic.held,diagnostic.focused);
        diagnostic.observed=ui.diagnostic_state(session);
        if(!same_context(c,borrowed_context)||!ui.owns_input())session.original(ubc::SelectionIssue::context_changed);
        diagnostic.capturing=GetTickCount64();
        {
          std::optional<GrindPreviewLease> shared_grind;
          if(session.phase()!=Phase::original&&c.kind==cat::Station::grind)
            shared_grind.emplace(c);
          if(shared_grind)report_grind_preview_failure(*shared_grind);
          const auto* shared=shared_grind?&*shared_grind:nullptr;
          if(session.phase()!=Phase::original)session.refresh(capture(c,shared).snapshot);
          diagnostic.captured=GetTickCount64();
          if(session.phase()!=Phase::original)observe_bow_quantity(c);
          if(session.phase()!=Phase::original)observe_cool_quantity(c);
          if(session.phase()!=Phase::original)observe_grind_quantity(c,shared);
        } // Always release before restoring the UI, taking a new snapshot or performing any handoff.
        diagnostic.checked=GetTickCount64();diagnostic.refreshed=ui.diagnostic_state(session);
        ui_diagnostic_events(ui_diagnostic.frame(session.preview().key.generation,diagnostic));
        if(session.phase()==Phase::original){
          close_grind_readiness();
          if(session.return_ready()){original_station=c.station;close_ui(c);event("ui_original");}
          else render_selection();
        }else if(session.phase()==Phase::ready){
          const auto shown_key=session.preview().key;
          const bool shown=ui.object()!=nullptr&&ui.owns_input()&&rendered_generation==shown_key.generation;
          close_ui(c);
          const auto restored=context();const auto captured=capture(restored);
          if(restored.kind==cat::Station::bow){session.refresh(captured.snapshot);observe_bow_quantity(restored);}
          if(restored.kind==cat::Station::cool){session.refresh(captured.snapshot);observe_cool_quantity(restored);}
          if(restored.kind==cat::Station::grind){session.refresh(captured.snapshot);observe_grind_quantity(restored);}
          const auto request=session.take(captured.snapshot,true);
          if(request){
            request_ui_generation=shown&&request->key==shown_key?request->key.generation:0;
            request_ui_key=shown_key;
            record_materials(request->material_snapshot,"stock_before");
            event("recipe_request",static_cast<int>(request->id),static_cast<int>(request->quantity));
            if(!execution_enabled){original_station=c.station;event("preview_confirmed");}
            else{
              // begin already needs the requested recipe. Until
              // then, observation() still uses the previous order.
              tobacco_targets={};tobacco_clean_rejection=false;
              auto initial=observation(restored,true,false);
              initial.materials=request->material_snapshot;initial.hands=captured.hands;
              const auto plan=ubc::make_crafting_plan(initial.page,*request);
              handed_hands=captured.hands;effect_executed=false;handoff_started=GetTickCount64();
              if(handoff.begin(*request,plan,initial,36000))drive_handoff(restored);
              else{original_station=c.station;event("handoff_rejected",static_cast<int>(handoff.issue()));}
            }
          }else{original_station=c.station;event("handoff_stale");}
        }else if(!render_selection(&c,&diagnostic,poll_known))session.original(ubc::SelectionIssue::invalid_snapshot);
      }
    }else if(success_pending&&!adapter_failed()){
      if(c.identity_known&&c.world==pending_success.key.world&&c.hero==pending_success.key.hero&&
        integer(0x8d1e38,8)==0&&(!ammo_job||ammo_notice_ready)&&ui.open_success(pending_success)){
        if(c.valid&&c.dialog.waiting&&c.dialog.open)borrow_view(c);
        event("success_notice_opened",static_cast<int>(pending_success.id),static_cast<int>(pending_success.quantity));
      }
    }else if(tobacco_clean_rejection&&c.valid&&!ui.opened()&&open_failure_notice()){
      tobacco_clean_rejection=false;original_station=c.station;borrow_view(c);ui.render_failure();
    }else if(!failure_notified&&c.identity_known&&c.world&&c.hero&&integer(0x8d1e38,8)==0&&
      (adapter_failed())){
      if(open_failure_notice()){
        failure_notified=true;
        if(c.valid&&c.dialog.waiting&&c.dialog.open)borrow_view(c);
        ui.render_failure();
      }
    }else if(handoff.active()){
      drive_handoff(c);
      if(handoff.state()==ubc::CraftingHandoffState::cancelled)original_station=c.station;
    }else if(bow_at_station?bow_can_open:
      (!tobacco_owner.ticket&&!original_station&&c.valid&&c.dialog.waiting&&c.dialog.open&&complete_collection(c)&&
       ubc::NativeSelectionUi::callback_is_top(c.dialog.view+a::view_callback)&&integer(0x8d1e38,8)==0)){
      const bool failure=adapter_failed();
      if(failure&&!failure_notified){
        if(open_failure_notice()){failure_notified=true;borrow_view(c);ui.render_failure();}
      }else if(!failure){
        const auto current_page=page(c,bow_trace);
        if(ubc::valid_crafting_page(current_page)){
          if(generation!=UINT64_MAX)++generation;else {disabled=true;if(bow_trace)bow_trace->equal("entry.generation_exhausted",0,1);}
          // At our first station, the actual new preview generation
          // now exists. The original dialog is still visible.
          if(!disabled)initialize_cool(c);
          const auto captured=capture(c,nullptr,bow_trace);
          if(bow_trace){
            bow_trace->equal("session.generation",1,session.accepts_generation(captured.snapshot.key.generation));
            bow_trace->equal("session.snapshot",1,ubc::valid_crafting_snapshot(captured.snapshot));
          }
          const bool compare=bow_comparison&&bow_at_station;
          bow_sample_count=0;
          if(compare&&bow_measured_frame!=frame&&!disabled&&session.accepts_generation(captured.snapshot.key.generation)&&
             ubc::valid_crafting_snapshot(captured.snapshot)){
            ubc::BowMenuTrace buffered{buffer_bow_value,true};
            measure_bow_menu(c,generation,buffered);
          }
          const bool session_open=!disabled&&session.open(captured.snapshot,
            c.world==remembered_world&&c.hero==remembered_hero&&c.station==remembered_station?remembered_recipe:UINT32_MAX);
          ubc::BowMenuTrace window_trace{bow_menu_window,true};
          if(session_open&&ui.open(bow_trace,compare?&window_trace:nullptr)){
            borrow_view(c);event("ui_open",static_cast<int>(c.kind),static_cast<int>(session.visible_count()));
            ui_diagnostic_events(ui_diagnostic.begin(session.preview().key.generation,GetTickCount64()));
            if(c.kind==cat::Station::grind||c.kind==cat::Station::bow){
              grind_quantity_observation={};
              grind_ui_trace_active=grind_ui_readiness.begin(session.preview().key.generation,frame);
            }
            for(const auto& r:captured.snapshot.recipes)if(r.known){
              // Negative finding before this row. A reader must not serve
              // the last recipe_available before its contract block.
              if(!r.contract)event("recipe_contract_unverified",static_cast<int>(r.id),0);
              event("recipe_available",static_cast<int>(r.id),static_cast<int>(r.maximum));
            }
            render_selection();
          }else{
            const int issue=captured.issue;
            if(issue!=last_capture_issue){last_capture_issue=issue;event("catalogue_unavailable",issue);}
          }
        }
      }
    }
    const int handoff_state=static_cast<int>(handoff.state());
    if(handoff_state!=last_handoff_state){
      if(handoff.state()==ubc::CraftingHandoffState::cancelled)original_station=c.station;
      last_handoff_state=handoff_state;event("handoff_state",handoff_state,static_cast<int>(handoff.issue()));
    }
  }
  --frame_depth;SetLastError(error);
}

void __fastcall observe_clear(void* view,void* unused) {
  const auto error=GetLastError();const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());const bool own=own_thread();
  if(own){++collection_depth;rows.begin(reinterpret_cast<std::uint32_t>(view),caller==a::info_clear_return&&collection_depth==1);
    if(caller!=a::info_clear_return)rows.reject("info.clear_caller",a::info_clear_return,caller);
    else if(collection_depth!=1)rows.reject("info.clear_depth",1,collection_depth);
    choice_collection=caller==a::choice_clear_return&&collection_depth==1;choice_count=0;}
  SetLastError(error);original_clear(view,unused);const auto after=GetLastError();
  if(own){const auto count=integer(reinterpret_cast<std::uint32_t>(view),a::view_count);
    if(count!=0){rows.reject("info.clear_count",0,count);choice_collection=false;}--collection_depth;}
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
    if(caller==a::info_add_return){
      if(collection_depth!=1)rows.reject("info.add_depth",1,collection_depth);
      if(number!=0)rows.reject("info.add_number",0,number);
      rows.append(address,info,before,count,true);
    }else rows.reject("info.add_caller",a::info_add_return,caller);
    if(choice_collection){
      if(caller!=a::choice_add_return||collection_depth!=1||number!=0||!choice.valid||before<0||
        static_cast<std::size_t>(before)!=choice_count||count!=before+1||choice.index!=before||choice_count==choice_functions.size())choice_collection=false;
      else choice_functions[choice_count++]=choice.function;
    }--collection_depth;
  }SetLastError(after);
}

const basecat::Callback* callback_for(const cat::Descriptor& recipe,std::uint32_t quantity) noexcept {
  if(recipe.tobacco&&quantity&&quantity<=INT32_MAX)return &recipe.tobacco->callback;
  if(recipe.base)return basecat::callback_for_quantity(*recipe.base,quantity);
  if(recipe.station_descriptor)return scat::callback_for_quantity(*recipe.station_descriptor,quantity);
  if(recipe.alchemy_extra){const auto p=xcat::plan_quantity(*recipe.alchemy_extra,quantity);return p.valid?p.callback:nullptr;}
  if(recipe.anvil){const auto p=acat::plan_quantity(*recipe.anvil,quantity);return p.valid?p.callback:nullptr;}
  if(recipe.ore){const auto p=ocat::plan_quantity(*recipe.ore,quantity);return p.valid?p.callback:nullptr;}
  if(recipe.bow){const auto p=bcat::plan_quantity(*recipe.bow,quantity);return p.valid?p.callback:nullptr;}
  return nullptr;
}
const basecat::Callback* amount_callback(std::int32_t symbol) noexcept {
  for(const auto& recipe:basecat::recipes)for(const auto& cb:recipe.callbacks)
    if(cb.minimum_quantity&&cb.function->symbol==static_cast<std::uint32_t>(symbol))return &cb;
  for(const auto& recipe:scat::recipes)
    if(recipe.callback.function->symbol==static_cast<std::uint32_t>(symbol))return &recipe.callback;
  for(const auto& recipe:tobacco::recipes)
    if(recipe.callback.function->symbol==static_cast<std::uint32_t>(symbol))return &recipe.callback;
  for(const auto& recipe:xcat::recipes)
    if(recipe.callback.function->symbol==static_cast<std::uint32_t>(symbol))return &recipe.callback;
  for(const auto& recipe:acat::recipes)
    if(recipe.callback.function->symbol==static_cast<std::uint32_t>(symbol))return &recipe.callback;
  for(const auto& recipe:ocat::recipes)
    if(recipe.callback.function->symbol==static_cast<std::uint32_t>(symbol))return &recipe.callback;
  for(const auto& recipe:bcat::recipes)
    if(recipe.callback.function->symbol==static_cast<std::uint32_t>(symbol))return &recipe.callback;
  return nullptr;
}
void trace_tobacco_entry(const Context&,const tobacco::Descriptor&,int,const ubc::CraftingRequest*,
    const char*,int,int) noexcept;
bool __cdecl batch_signature(const bc::Call& call) noexcept {
  const auto* entry=amount_callback(call.symbol);
  const ubc::lhiver::Function* registered=call.symbol==20401?tobacco::owner.information:entry?entry->function:nullptr;
  for(const auto& recipe:acat::recipes)if(recipe.original_three.function->symbol==static_cast<std::uint32_t>(call.symbol))
    registered=recipe.original_three.function;
  for(const auto& recipe:gcat::recipes)if(recipe.callback->symbol==static_cast<std::uint32_t>(call.symbol))
    registered=recipe.callback;
  for(const auto* f:{ccat::single.information,ccat::triple.information})
    if(f->symbol==static_cast<std::uint32_t>(call.symbol))registered=f;
  for(const auto& recipe:shcat::recipes)
    if(recipe.function.symbol==static_cast<std::uint32_t>(call.symbol))registered=&recipe.function;
  if(!registered||reinterpret_cast<std::uintptr_t>(call.parser)!=s::game_parser)return false;
  const auto& function=*registered;rt::detail::Layout layout{};rt::detail::Symbol symbol{};
  return rt::detail::read_layout(memory,s::game_parser,layout_profile,layout)==rt::Issue::none&&
    rt::detail::lookup(memory,layout,function.symbol,symbol)==rt::Issue::none&&
    rt::detail::name_matches(memory,symbol,function.name)==rt::Issue::none&&
    symbol.type==5&&symbol.flags==1&&symbol.elements==0&&symbol.offset==0&&
    rt::detail::parent_matches(memory,layout,symbol,-1)==rt::Issue::none&&
    rt::detail::stable_layout(memory,s::game_parser,layout_profile,layout)==rt::Issue::none;
}
bool __cdecl reject_unexpected_cool_info(const bc::Call& call) noexcept {
  const bool protected_effect=(call.symbol==20401||(call.symbol>=20403&&call.symbol<=20405))&&
    InterlockedCompareExchange(&tobacco_unpaid_page,0,0);
  if(protected_effect&&(!own_thread()||call.caller!=bc::choice_call_return))return true;
  if(own_thread()){
    const bool wrong_owner=tobacco_owner_call&&call.caller==bc::info_call_return&&call.symbol!=20401;
    const bool wrong_choice=tobacco_owner.ticket&&call.caller==bc::choice_call_return&&
      (!tobacco_choice_call||call.symbol!=tobacco_choice_call);
    if(wrong_owner||wrong_choice){
      char fields[128]{};
      std::snprintf(fields,sizeof(fields),",\"field\":\"call.bound_symbol\",\"soll\":\"%d\",\"ist\":\"%d\"",
        wrong_owner?20401:tobacco_choice_call,call.symbol);
      event("tobacco_check_value",wrong_owner?20401:tobacco_owner.function,0,fields);
      return true;
    }
  }
  if(!own_thread()||call.caller!=bc::info_call_return||!effect_authorized||!native_update||
     update_depth!=1||info_depth!=1||choice_depth)return false;
  const auto* ticket=handoff.current_ticket();
  if(!ticket||handoff.request().key.kind!=cat::Station::cool||handoff.request().quantity!=cpre::quantity||
     ticket->offer.owner!=cpre::owner||ticket->offer.function!=cpre::callback||call.symbol==ticket->offer.function)return false;
  handoff.cancel(ubc::CraftingHandoffIssue::offer);
  return true;
}
bc::Decision __cdecl select_batch_call(const bc::Call& call) noexcept {
  if(!own_thread())return bc::Decision::ordinary;
  if(!tobacco_owner.ticket&&call.caller==bc::choice_call_return&&choice_depth==1&&call.symbol>=20403&&call.symbol<=20405)
    trace_tobacco_entry(context(),tobacco::recipes[call.symbol-20403],call.symbol,nullptr,"call.original",1,1);
  if(tobacco_owner_call){
    return call.symbol==20401&&call.caller==bc::info_call_return&&!handoff.cancelled()
      ?bc::Decision::writer:bc::Decision::reject;
  }
  const auto* ticket=handoff.current_ticket();
  if(tobacco_owner.ticket&&call.caller==bc::choice_call_return&&call.symbol!=20402&&
     (!ticket||!tobacco_owner.accepts(*ticket,call.symbol)||tobacco_choice_call!=call.symbol))
    return bc::Decision::reject;
  if(!handoff.active()||!ticket||call.symbol!=ticket->offer.function)return bc::Decision::ordinary;
  const auto expected=ticket->offer.mode==basecat::Invocation::info?bc::info_call_return:bc::choice_call_return;
  if(!effect_authorized||effect_executed||handoff.cancelled()||call.caller!=expected||
    handoff.state()!=ubc::CraftingHandoffState::executing)return bc::Decision::reject;
  return bc::Decision::writer;
}
bool late_gate(ubc::GrindCheck* diagnostic=nullptr,tobacco::Checks* trace=nullptr) noexcept {
  const auto matches=[diagnostic,trace](const char* name,std::int64_t expected,std::int64_t actual){
    if(trace)return trace->equal(name,expected,actual);
    return diagnostic?diagnostic->equal(name,expected,actual):expected==actual;
  };
  const auto* ticket=handoff.current_ticket();const auto* step=handoff.step();
  if(!matches("gate.ticket",true,ticket!=nullptr)||!matches("gate.step",true,step!=nullptr)||
     !matches("gate.effect_step",true,step&&step->is_effect())||!matches("gate.authorized",true,effect_authorized)||
     !matches("gate.executed",false,effect_executed)||!matches("gate.cancelled",false,handoff.cancelled())||
     !matches("gate.handoff_state",static_cast<int>(ubc::CraftingHandoffState::executing),static_cast<int>(handoff.state()))||
     !matches("gate.native_update",true,native_update)||!matches("gate.owner_thread",true,own_thread())||
     !matches("gate.update_depth",1,update_depth)||!matches("gate.station_depth",0,station_depth)||
     !matches("gate.collection_depth",0,collection_depth)||!matches("gate.callback_depth",1,info_depth+choice_depth)||
     !matches("gate.focus",true,ui.focused()))return false;
  const auto c=context();const auto current=observation(c,false);
  if(c.kind==cat::Station::saw){
    const auto* recipe=cat::find(handoff.request().id);
    if(!c.valid||!recipe||recipe->route!=cat::Route::station||recipe->station!=cat::Station::saw||
      !recipe->station_descriptor||recipe->station_descriptor->material_actor!=basecat::Actor::self)return false;
    // The original OnChoice binds SELF from Manager.NPC at 0x662AA5 before
    // CallFunc reaches our writer at 0x662B6A. The physical preview actor
    // cannot authorize this call. The freshly read script actor must match here.
    const auto actor=ubc::station_recipe_runtime::actor(memory,s::game_parser,*recipe->station_descriptor,c.hero);
    if(!actor.read_valid||!actor.requires_self||!actor.matches)return false;
  }
  if(c.kind==cat::Station::bow){
    const auto* recipe=cat::find(handoff.request().id);
    if(!c.valid||!recipe||!recipe->bow||!current_info_effect(c))return false;
    const auto profile=brt::assemble(*recipe->bow);const auto actor=brt::actor(memory,s::game_parser,profile,c.hero);
    const auto state=brt::context(memory,s::game_parser,profile);const auto known=brt::knowledge(memory,s::game_parser,profile);
    if(!actor.read_valid||!actor.requires_self||!actor.matches||!state.owner_matches()||!known.read_valid||!known.known)return false;
  }
  if(c.kind==cat::Station::cool){
    const auto* recipe=cat::find(handoff.request().id);
    const auto plan=ubc::crafting_cool_plan(handoff.request().material_snapshot,handoff.request().quantity);
    if(!c.valid||!recipe||!recipe->cool||!current_info_effect(c)||!plan.valid||
       !cool_runtime_path(plan.requires_extra_debit).valid||!end_path(c.station,plan.requires_extra_debit).valid)return false;
    const auto actor=crt::actor(memory,s::game_parser,crt::assemble(ccat::recipe),c.hero);
    if(!actor.read_valid||!actor.requires_self||!actor.matches)return false;
  }
  if(c.kind==cat::Station::grind){
    const auto* recipe=cat::find(handoff.request().id);
    if(!matches("gate.context",true,c.valid)||!matches("gate.recipe",true,recipe!=nullptr)||
       !matches("gate.descriptor",true,recipe->grind!=nullptr)||!matches("gate.choice",true,current_grind_choice(c))||
       !matches("gate.destroy_path",true,cool_runtime_path().valid))return false;
    if(!recipe||!recipe->grind)return false;
    const auto actor=shrt::actor(memory,s::game_parser,shrt::assemble(*recipe->grind),c.hero);
    if(!matches("gate.actor_read",true,actor.read_valid)||!matches("gate.requires_self",true,actor.requires_self)||
       !matches("gate.actor_matches",true,actor.matches))return false;
  }
  if(c.kind==cat::Station::anvil){
    const auto* recipe=cat::find(handoff.request().id);
    if(!c.valid||!recipe||recipe->station!=cat::Station::anvil)return false;
    // The physical preview is not a script actor. Only at the inner
    // CallFunc, after the original SELF binding, may the SELF minimum check
    // and the HERO material effect be admitted together for this order.
    if(recipe->route==cat::Route::anvil&&recipe->anvil){
      const auto actor=art::actor(memory,s::game_parser,art::assemble(*recipe->anvil),c.hero);
      if(!actor.read_valid||!actor.requires_self||!actor.matches)return false;
    }else if(recipe->route==cat::Route::ore&&recipe->ore){
      const auto actor=ort::actor(memory,s::game_parser,ort::assemble(*recipe->ore),c.hero);
      if(!actor.read_valid||!actor.requires_self||!actor.matches)return false;
    }else return false;
  }
  if((!ticket||!step)&&!trace)return false;
  const ubc::CraftingHandoffTicket missing_ticket{};const ubc::CraftingStep missing_step{};
  if(!ticket)ticket=&missing_ticket;
  if(!step)step=&missing_step;
  return matches("gate.final_context",true,c.valid)&&matches("gate.context_known",true,current.context_known)&&
    matches("gate.key",true,current.key==handoff.request().key)&&matches("gate.view",ticket->view,current.view)&&
    matches("gate.collection",true,current.collection==ticket->offer.collection)&&
    matches("gate.mode",static_cast<int>(ticket->offer.mode),static_cast<int>(current.mode))&&
    matches("gate.page_known",true,current.page_known)&&matches("gate.page_kind",static_cast<int>(step->before.kind),static_cast<int>(current.page.kind))&&
    matches("gate.page",step->before.value,current.page.value)&&
    matches("gate.resources",true,ubc::crafting_handoff_detail::same_resources(current.materials,handoff.request().material_snapshot))&&
    matches("gate.hands",true,ubc::same_crafting_hands(current.hands,handed_hands))&&
    matches("gate.stable",true,same_context(c,context()))&&
    (ticket->offer.mode==basecat::Invocation::info?matches("gate.info_complete",true,current.complete):
      matches("gate.post_remove_choice",true,post_remove_choice.consume(memory,c.dialog,rows.generation,ticket->id,ticket->offer.function)))&&(!trace||trace->ok());
}
bool __cdecl extra_boundary(void*,xwriter::Phase phase,const xdebit::Job& job,xwriter::Boundary& out) {
  out={};const auto bridge=bc::diagnostics();
  if(!own_thread()||!extra_executor||!extra_writer_scope||!extra_job||!bridge.writer||bridge.readers||
    bridge.writes_disabled||!native_update||update_depth!=1||info_depth!=1||choice_depth||station_depth||collection_depth||
    extra_writer_scope->call.thread!=GetCurrentThreadId()||extra_writer_scope->call.symbol!=static_cast<int>(job.callback)||
    extra_writer_scope->call.caller!=bc::info_call_return||job.id!=extra_active_job.id||job.writer_token!=extra_active_job.writer_token||
    job.quantity!=extra_active_job.quantity||job.callback!=extra_active_job.callback||job.owner!=extra_active_job.owner||
    !xdebit::same(job.context,extra_active_job.context))return false;
  const auto scope=extra_executor->scope_evidence();
  if(!scope.valid||!scope.scope.writer_active||scope.scope.writer_depth!=1||scope.scope.vm_depth)return false;
  const auto c=context();
  if(!c.identity_known||!c.station_known||!xdebit::same(extra_context(c),job.context)||
    !matches_station_profile(c.station,cat::Station::alchemy)||!end_path(c.station).valid)return false;
  out.runtime.context=extra_context(c);out.runtime.scope=scope.scope;
  out.runtime.serial=next_extra_serial();out.runtime.current_thread=GetCurrentThreadId();
  out.runtime.synchronous_writer_verified=true;out.runtime.physical_lab_hand_contract_verified=true;
  out.actual_info=current_info_effect(c);if(!out.actual_info)return false;
  out.modules=extra_modules();
  if(phase==xwriter::Phase::before_original)out.native_handoff_verified=late_gate();
  return out.runtime.serial&&xdebit::same(extra_context(context()),job.context);
}
bool __cdecl spirit_boundary(void*,gwriter::Phase phase,const gdebit::Job& job,gwriter::Boundary& out) {
  out={};const auto bridge=bc::diagnostics();
  if(!own_thread()||!spirit_executor||!spirit_writer_scope||!spirit_job||!bridge.writer||bridge.readers||
    bridge.writes_disabled||!native_update||update_depth!=1||info_depth!=1||choice_depth||station_depth||collection_depth||
    spirit_writer_scope->call.thread!=GetCurrentThreadId()||spirit_writer_scope->call.symbol!=static_cast<int>(job.callback)||
    spirit_writer_scope->call.caller!=bc::info_call_return||job.id!=spirit_active_job.id||job.writer_token!=spirit_active_job.writer_token||
    job.quantity!=spirit_active_job.quantity||job.branch_value!=spirit_active_job.branch_value||job.callback!=spirit_active_job.callback||job.owner!=spirit_active_job.owner||
    !gdebit::same(job.context,spirit_active_job.context))return false;
  const auto scope=spirit_executor->scope_evidence();
  if(!scope.valid||!scope.scope.writer_active||scope.scope.writer_depth!=1||scope.scope.vm_depth)return false;
  const auto c=context();
  if(!c.identity_known||!c.station_known||!gdebit::same(extra_context(c),job.context)||
    !matches_station_profile(c.station,cat::Station::alchemy)||!end_path(c.station).valid)return false;
  out.runtime.context=extra_context(c);out.runtime.scope=scope.scope;
  out.runtime.serial=next_extra_serial();out.runtime.current_thread=GetCurrentThreadId();
  out.runtime.synchronous_writer_verified=true;out.runtime.physical_lab_hand_contract_verified=true;
  out.actual_info=current_info_effect(c);if(!out.actual_info)return false;
  out.modules=extra_modules();
  if(phase==gwriter::Phase::before_original)out.native_handoff_verified=late_gate();
  return out.runtime.serial&&gdebit::same(extra_context(context()),job.context);
}
struct NativeAmountContext {
  rt::detail::Layout layout{};
  const ubc::lhiver::Function* function=nullptr;
  bool predebit=false;
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
    if(ctx->predebit&&!cwriter::permits_operand_replace(result_guard.cool_prepared(),info_lease_owner,cool_writer_scope,
       cool_predebit_journal,cool_fence_token,address,before,after,size,[](){return current_info_effect(context());}))return false;
    std::memcpy(target,after,size);return std::memcmp(target,after,size)==0;
  } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void log_plugin_version(const char* name,const ubc::windows_probe::LoadedModuleIdentity& module) noexcept {
  if(!module.observed())return;
  const auto digest=module.digest();char hex[65]{};
  constexpr char digits[]="0123456789abcdef";
  for(std::size_t i=0;i<digest.size();++i){hex[2*i]=digits[digest[i]>>4];hex[2*i+1]=digits[digest[i]&15];}
  char fields[125]{};
  std::snprintf(fields,sizeof(fields),",\"sha256\":\"%s\",\"status\":\"%s\"",hex,module.current()?"getestet":"ungetestete Version");
  event(name,0,0,fields);
}
void flush_amount_diagnostic(std::uint64_t info_token=0) noexcept {
  if(!amount_diagnostic.pending||(amount_diagnostic.cool_info_token&&amount_diagnostic.cool_info_token!=info_token))return;
  alignas(16) unsigned char fx[512];
  if(amount_diagnostic.cool_info_token)_fxsave(fx);
  const auto saved=GetLastError();const auto report=amount_diagnostic;amount_diagnostic={};
  if(report.cool_info_token){
    log_plugin_version("cool_union_version",end_union_module);
    log_plugin_version("cool_zbug_version",end_zbug_module);
    const auto witness=result_guard.cool_witness();
    if(witness.valid&&witness.job.info_token==info_token)
      event("cool_predebit_verified",witness.extra_debit,witness.original_calls);
  }
  if(report.cool_info_token && cool_context_mode){
    unsigned count=0;bool complete=true;
    for(const auto phase:cpre::inspection::phases){
      const auto& rows=cool_check_reports.at(phase);complete&=rows.count!=0&&!rows.overflow;
      for(std::size_t i=0;i<rows.count;++i){
        const auto& row=rows.rows[i];char fields[248]{};
        std::snprintf(fields,sizeof(fields),",\"phase\":%d,\"field\":\"%s\",\"soll\":\"%s\",\"ist\":\"%s\"",
          static_cast<int>(phase),row.field,row.expected,row.actual);
        event("cool_check_value",static_cast<int>(count++),row.matches?1:0,fields);
      }
    }
    if(complete)event("cool_checks_complete",static_cast<int>(count),report.quantity);
    cool_check_reports.clear();
  }
  if(report.entered)event("native_effect_enter",report.symbol,report.quantity);
  if(report.predebit.event){
    const int phase=static_cast<int>(report.predebit.phase);
    event(report.predebit.event,phase,report.predebit.reason);
    if(report.predebit.failure.field){
      const auto& row=report.predebit.failure;char fields[240]{};
      std::snprintf(fields,sizeof(fields),",\"field\":\"%s\",\"soll\":\"%s\",\"ist\":\"%s\"",row.field,row.expected,row.actual);
      event("cool_check_rejected",phase,report.predebit.reason,fields);
    }
    if(report.predebit.address)event("cool_predebit_address",phase,diagnostic_word(report.predebit.address));
  }
  if(report.predebit.native_restore_failed)event("cool_predebit_restore_failed",report.symbol,report.quantity);
  if(report.symbol>=20403&&report.symbol<=20405)log_tobacco_check(report.grind_check,handoff.request().id);
  else log_grind_check(report.grind_check,report.symbol);
  const int rejected=report.predebit.event?8:report.rejected;
  if(rejected)event("amount_rejected",rejected);
  event(report.restored?"amount_restored":"amount_restore_failed",report.symbol,report.quantity);
  if(grind_skill_log_pending){
    const auto& request=handoff.request();grind_skill_log_pending=false;
    event("grind_skill_before",static_cast<int>(request.id),grind_prepared.before.skill);
    event("grind_skill_after",static_cast<int>(request.id),grind_skill_after_values.skill);
    event("grind_skill_verified",static_cast<int>(request.id),static_cast<int>(request.quantity));
  }
  event("amount_duration_us",report.symbol,report.duration);
  flush_ammo_diagnostic(true);
  if(report.cool_info_token)_fxrstor(fx);
  SetLastError(saved);
}
void* __cdecl execute_original_amount(void* raw) {
  const auto incoming_error=GetLastError();
  auto& scope=*static_cast<bc::WriterScope*>(raw);const auto& request=handoff.request();
  const auto* recipe=cat::find(request.id);const auto* ticket=handoff.current_ticket();
  const ubc::CraftingResultBinding binding{request.key.generation,frame,ticket?ticket->offer.collection:0,end_sequence,
    request.key.world,request.key.hero,request.key.station};
  if(!recipe||!result_guard.begin(request,binding)){
    handoff.cancel(ubc::CraftingHandoffIssue::resources);SetLastError(incoming_error);return nullptr;
  }
  if(ammo_job){
    active_ammo_scope={request.key.generation,request.key.generation,static_cast<std::uint32_t>(scope.call.symbol),1,true,amount_patch.active()};
    if(!read_ammo_context(nullptr,active_ammo_context)||!ammo::enter_effect(active_ammo_context,active_ammo_scope)){
      active_ammo_scope={};result_guard.fail(ubc::CraftingResultIssue::contract);SetLastError(incoming_error);return nullptr;
    }
  }
  void* value=nullptr;effect_executed=true;DWORD original_error=incoming_error;
  SetLastError(incoming_error);
  __try {value=scope.original(scope.call.parser,scope.call.symbol);original_error=GetLastError();
    if(grind_job)grind_original_returned=true;}
  __finally {
    if(AbnormalTermination())original_error=GetLastError();
    if(ammo_job){
      ap::Context current;
      if(!read_ammo_context(nullptr,current)){
        note_ammo_diagnostic(AmmoDiagnosticPhase::close_context);
        result_guard.fail(ubc::CraftingResultIssue::contract);
      }else if(!ammo::close_effect(current,active_ammo_scope)){
        note_ammo_diagnostic(AmmoDiagnosticPhase::close_effect);
        result_guard.fail(ubc::CraftingResultIssue::contract);
      }
      active_ammo_scope={};
    }
    SetLastError(original_error);
  }
  return value;
}
void* execute_extra_amount(bc::WriterScope& scope,const ubc::AmountMemory& writable,const cat::Descriptor& recipe) {
  const auto incoming_error=GetLastError();const auto& request=handoff.request();
  if(!recipe.alchemy_extra||!extra_catalogue_reader||!extra_executor){SetLastError(incoming_error);return nullptr;}
  const auto c=context();
  const auto actual_info=current_info_effect(c);
  if(!actual_info){event("extra_info_rejected",1);SetLastError(incoming_error);return nullptr;}
  const auto input=extra_read_input(c,native_update&&update_depth==1&&info_depth==1&&!choice_depth&&!station_depth&&!collection_depth);
  const auto fresh=extra_catalogue_reader->capture_recipe(memory,input,recipe.alchemy_extra->id,request.quantity);
  if(!fresh.effect_ready||fresh.quantity!=request.quantity||!xdebit::same(fresh.context,extra_context(c))||
    !same_context(c,context())){
    event("extra_snapshot_rejected",static_cast<int>(fresh.issue),static_cast<int>(request.quantity));
    SetLastError(incoming_error);return nullptr;
  }
  const auto* ticket=handoff.current_ticket();
  const ubc::CraftingResultBinding binding{request.key.generation,frame,ticket?ticket->offer.collection:0,end_sequence,
    request.key.world,request.key.hero,request.key.station};
  if(!result_guard.begin(request,binding)){SetLastError(incoming_error);return nullptr;}
  extra_active_job={fresh.context,request.key.generation,request.key.generation,
    recipe.alchemy_extra->callback.function->symbol,static_cast<std::uint32_t>(recipe.alchemy_extra->owner.instance),request.quantity};
  extra_end={};extra_normal_end=false;extra_end_frame=0;extra_writer_scope=&scope;
  const xwriter::Request run{fresh,extra_active_job,actual_info};
  void* returned=nullptr;DWORD error=incoming_error;
  SetLastError(incoming_error);
  __try {returned=extra_executor->run(scope,amount_patch,writable,run);error=GetLastError();}
  __finally {
    if(AbnormalTermination())error=GetLastError();
    effect_executed=extra_executor->outcome().original_entered;extra_writer_scope=nullptr;
    if(AbnormalTermination())result_guard.fail(ubc::CraftingResultIssue::restore);
    SetLastError(error);
  }
  const auto& outcome=extra_executor->outcome();
  const auto expected=request.quantity==1?xwriter::RestoreProof::not_required_for_one:xwriter::RestoreProof::verified;
  const bool restored=outcome.original_entered&&outcome.original_returned&&outcome.operands_restored&&
    outcome.closing&&!outcome.blocked&&scope.restore_verified&&outcome.vm_restore==expected&&outcome.item_restore==expected;
  result_guard.restored(restored,{request.key.generation,request.id,request.quantity,
    outcome.closing&&!outcome.blocked&&outcome.original_returned,outcome.item_restore==expected});
  if(!restored){
    event("extra_writer_rejected",static_cast<int>(outcome.dependency_issue),static_cast<int>(outcome.read_issue));
    const auto& failure=outcome.parser_failure;
    if(failure.captured){
      const auto& diagnostic=failure.snapshot;
      event("extra_parser_failure",static_cast<int>(failure.phase),
        diagnostic.value.data_depth<=INT32_MAX?static_cast<int>(diagnostic.value.data_depth):-1);
      event("extra_parser_flags",static_cast<int>(ubc::alchemy_flask_runtime::parser_failure_flags(diagnostic,run.job.context)),
        static_cast<int>(diagnostic.word_count));
      if(diagnostic.words_stable)for(std::uint32_t i=0;i<diagnostic.word_count&&i<diagnostic.words.size();++i)
        event("extra_parser_word",static_cast<int>(i),ubc::alchemy_flask_runtime::parser_failure_log_word(diagnostic.words[i]));
    }
  }
  SetLastError(error);return returned;
}
void* execute_spirit_amount(bc::WriterScope& scope,const ubc::AmountMemory& writable,const cat::Descriptor& recipe) {
  const auto incoming_error=GetLastError();const auto& request=handoff.request();
  if(!recipe.spirit||!spirit_catalogue_reader||!spirit_executor){SetLastError(incoming_error);return nullptr;}
  const auto c=context();const auto actual_info=current_info_effect(c);
  if(!actual_info){event("spirit_info_rejected",1);SetLastError(incoming_error);return nullptr;}
  const auto input=extra_read_input(c,native_update&&update_depth==1&&info_depth==1&&!choice_depth&&!station_depth&&!collection_depth);
  const auto fresh=spirit_catalogue_reader->capture(memory,{input,nullptr,read_spirit_actual_info});
  const auto quantity=ubc::spirit_crafting_quantity(fresh,request.quantity);
  const auto projected=ubc::project_capture(fresh);
  if(!quantity.effect_ready||fresh.actual_info!=actual_info||!(projected.key==request.key)||
    !ubc::valid_crafting_snapshot(projected)||
    !ubc::crafting_handoff_detail::same_resources(projected.recipes[cat::spirit_begin],request.material_snapshot)||
    !ubc::same_crafting_hands(projected.hands,handed_hands)||!same_context(c,context())){
    event("spirit_snapshot_rejected",static_cast<int>(fresh.diagnostic.issue),static_cast<int>(request.quantity));
    SetLastError(incoming_error);return nullptr;
  }
  const auto* ticket=handoff.current_ticket();
  const ubc::CraftingResultBinding binding{request.key.generation,frame,ticket?ticket->offer.collection:0,end_sequence,
    request.key.world,request.key.hero,request.key.station};
  if(!result_guard.begin(request,binding)){SetLastError(incoming_error);return nullptr;}
  spirit_active_job={{extra_context(c),request.key.generation,request.key.generation,recipe.spirit->callback->symbol,
    static_cast<std::uint32_t>(recipe.spirit->owner.instance),request.quantity},request.material_snapshot.branch_value};
  spirit_end={};spirit_normal_end=false;spirit_end_frame=0;spirit_writer_scope=&scope;
  const gwriter::Request run{fresh.recipe,fresh.serial,request.quantity,spirit_active_job,actual_info};
  void* returned=nullptr;DWORD error=incoming_error;
  SetLastError(incoming_error);
  __try {returned=spirit_executor->run(scope,amount_patch,writable,run);error=GetLastError();}
  __finally {
    if(AbnormalTermination())error=GetLastError();
    effect_executed=spirit_executor->outcome().original_entered;spirit_writer_scope=nullptr;
    if(AbnormalTermination())result_guard.fail(ubc::CraftingResultIssue::restore);
    SetLastError(error);
  }
  const auto& outcome=spirit_executor->outcome();
  const auto expected=request.quantity==1?gwriter::RestoreProof::not_required_for_one:gwriter::RestoreProof::verified;
  const bool restored=outcome.original_entered&&outcome.original_returned&&outcome.operands_restored&&
    outcome.closing&&!outcome.blocked&&scope.restore_verified&&outcome.vm_restore==expected&&outcome.item_restore==expected;
  result_guard.restored(restored,{request.key.generation,request.id,request.quantity,
    outcome.closing&&!outcome.blocked&&outcome.original_returned,outcome.item_restore==expected});
  if(!restored)event("spirit_writer_rejected",static_cast<int>(outcome.dependency_issue),static_cast<int>(outcome.read_issue));
  SetLastError(error);return returned;
}
bool verify_bow_amount(const bc::WriterScope& scope,const cat::Descriptor& recipe) noexcept {
  const auto& request=handoff.request();const auto c=context();const auto bridge=bc::diagnostics();
  if(!recipe.bow||recipe.route!=cat::Route::bow||!bow_modules_current()||
    !c.valid||c.kind!=cat::Station::bow||!(key(c)==request.key)||!bridge.writer||bridge.readers||bridge.writes_disabled||
    !scope.original||scope.call.thread!=GetCurrentThreadId()||scope.call.caller!=bc::info_call_return||
    scope.call.symbol!=static_cast<int>(recipe.bow->callback.function->symbol)||!current_info_effect(c)||
    !end_path(c.station).valid||!native_update||update_depth!=1||info_depth!=1||choice_depth||
    station_depth||collection_depth||resolved_read_depth||amount_patch.active())return false;
  const auto proof=bow_item_reader->capture(memory,*recipe.bow,request.quantity,bow_read_input(request.key));
  if(!proof.item_path_verified){
    log_bow_check("writer.item_path",0,static_cast<int>(proof.issue),static_cast<int>(request.id),request.quantity,proof.failed_address);
    event("bow_item_path_rejected",static_cast<int>(proof.issue),static_cast<int>(request.quantity));return false;
  }
  if(!ubc::matching_bow_item_path(proof,request,frame)||!ubc::same_crafting_hands(proof.stock.hands,handed_hands))return false;
  // Reread all material, knowledge, actor and INFO evidence after the full
  // closure, before the first operand patch. N1 does not establish Nmax.
  if(!bow_modules_current()||!same_context(c,context())||!late_gate())return false;
  event("bow_item_path_verified",static_cast<int>(recipe.id),static_cast<int>(request.quantity));return true;
}
bool verify_cool_amount(const bc::WriterScope& scope,const cat::Descriptor& recipe) noexcept {
  const auto& request=handoff.request();const auto c=context();
  if(!recipe.cool||!c.valid||c.kind!=cat::Station::cool||cool_destroy_depth||
    scope.call.thread!=GetCurrentThreadId()||scope.call.caller!=bc::info_call_return||
    !native_update||update_depth!=1||info_depth!=1||choice_depth||station_depth||collection_depth||
    !cool_runtime_path().valid)return false;
  const auto actual=current_info_effect(c);if(!actual)return false;
  const auto current_page=cool_page(c);
  const auto before=capture_cool(c,true);
  const auto prepared=cdirect::prepare(before,current_page,request.quantity);
  if(!prepared.valid||prepared.path.amount.callback!=static_cast<std::uint32_t>(scope.call.symbol))return false;
  const auto late=capture_cool(c,true);
  const auto bound=cdirect::bind(memory,prepared,late,current_page,actual);
  if(!bound.valid||!cdirect::detail::same_key(bound.prepared.key,cool_key(c))||
    !ubc::same_crafting_hands(bound.prepared.hands,handed_hands)||
    !same_context(c,context())||!late_gate())return false;
  cool_active_entry=bound;cool_job_floor=cool_serial;
  cool_destroy_call={};cool_destroy_binding={};
  cool_destroy_hand_diagnostic={};
  cool_receipt_acked=!bound.prepared.requirements().quantity_receipt;
  event("cool_direct_path_verified",static_cast<int>(recipe.id),static_cast<int>(request.quantity));return true;
}
std::uint64_t next_cool_serial() noexcept;
struct CoolProbePhase {
  const cadmission::Prepared& prepared;
  const ubc::AmountPatch& patch;
  const cpre::Tracker& tracker;
  bool waiting;
};
bool inspect_cool_boundary(cpre::reader::Phase phase,cpre::reader::Input& input,
                           cpre::check::Report* report,const CoolProbePhase* model=nullptr) noexcept {
  using Phase=cpre::reader::Phase;using Gate=cpre::Gate;
  cpre::check::Checks k(report);auto& rejection=amount_diagnostic.predebit;
  const auto group=[&](Gate gate){k.reason=static_cast<int>(gate);};
  const auto* scope=cool_writer_scope;const auto bridge=bc::diagnostics();
  group(Gate::writer_scope);
  k.flag("writer.game_thread",own_thread());k.flag("writer.original",scope&&scope->original);
  k.equal("writer.thread",GetCurrentThreadId(),scope?scope->call.thread:0);
  k.equal("writer.parser",s::game_parser,scope?reinterpret_cast<std::uint32_t>(scope->call.parser):0);
  k.equal("writer.caller",bc::info_call_return,scope?scope->call.caller:0);
  k.equal("writer.callback",cpre::callback,scope?scope->call.symbol:0);
  k.test("writer.token",">0",scope?scope->token:0,scope&&scope->token);
  k.flag("writer.active",bridge.writer);k.equal("writer.readers",0,bridge.readers);
  k.equal("writer.disabled",0,bridge.writes_disabled);
  k.flag("writer.executor",cool_predebit_executor.has_value());k.flag("writer.dispatch_reader",destroy_dispatch_reader.has_value());
  k.flag("writer.cool_ready",cool_ready);k.equal("writer.destroy_depth",0,cool_destroy_depth);k.equal("writer.frame_depth",0,frame_depth);
  if(cpre::reader::split_pending(phase))k.flag("writer.focus",ui.focused());
  k.flag("writer.cool_job",cool_job);k.equal("writer.grind_job",0,grind_job);
  k.test("writer.fence",">0",cool_fence_token,cool_fence_token!=0);
  const auto c=context();const auto actual=current_info_effect(c);const auto& request=handoff.request();
  const auto* ticket=handoff.current_ticket();const auto* step=handoff.step();
  group(Gate::handoff);
  k.test("ticket.actual_info",">0",actual,actual!=0);k.flag("ticket.step",step!=nullptr);
  k.equal("ticket.owner",cpre::owner,ticket?ticket->offer.owner:0);k.equal("ticket.callback",cpre::callback,ticket?ticket->offer.function:0);
  k.equal("ticket.station",static_cast<int>(cat::Station::cool),static_cast<int>(request.key.kind));
  k.equal("ticket.quantity",2,request.quantity);k.equal("ticket.recipe",static_cast<int>(ccat::RecipeId::blade),static_cast<int>(request.id));
  const auto end=end_path(c.station,true);
  k.test("ticket.end_path","gueltiger Endpfad",static_cast<int>(end.issue),end.valid);
  k.flag("ticket.station_profile",matches_station_profile(c.station,cat::Station::cool));
  const auto d=cool_predebit_executor?cool_predebit_executor->diagnostics():cwriter::Diagnostics{};
  const bool admission=phase==Phase::admission,after=phase==Phase::after_original;
  const auto& patch=model?model->patch:amount_patch;
  const bool native=model?(phase==Phase::before_remove||phase==Phase::restore||phase==Phase::after_extra):d.native_scope;
  group(Gate::executor_phase);k.flag("executor.configured",d.configured);k.equal("executor.disabled",0,d.disabled);
  k.equal("executor.active",!admission,model?true:d.active);
  k.flag("executor.guard",model?model->waiting:admission?result_guard.permits_next():result_guard.state()==ubc::CraftingResultState::waiting);
  k.equal("executor.amount_active",!admission&&!after,patch.active());
  if(!admission){
    group(Gate::prepared);
    const auto& prepared=model?model->prepared:result_guard.cool_prepared();const auto& job=prepared.job();
    k.flag("prepared.lease",prepared.current(info_lease_owner));k.equal("prepared.writer",scope?scope->token:0,job.writer_token);
    k.equal("prepared.fence",cool_fence_token,job.fence_token);k.equal("prepared.generation",request.key.generation,job.id);
    k.equal("prepared.info",actual,job.actual_info);k.equal("prepared.frame",frame,prepared.frame());
    k.flag("prepared.context",cpre::value::same(job.context,extra_context(c)));
    group(Gate::operand);
    if(!after)k.flag("prepared.operand",patch.matches_active(prepared.body_address(),prepared.amount().body(),cpre::quantity));
  }
  if(!after){
    group(Gate::context);k.flag("call.context",c.valid);k.flag("call.key",key(c)==request.key);k.equal("call.previous_effect",0,effect_executed);
    if(phase==Phase::before_remove||phase==Phase::before_original||admission){
      group(Gate::page);const auto page=cool_info_page(c,true);k.flag("page.valid",page.valid);k.flag("page.single",page.single);
      k.flag("page.ticket",step&&ubc::crafting_page(page)==step->before);
      const auto row=ticket?ticket->offer.row:-1;
      k.test("page.row","in INFO-Liste",row,row>=0&&static_cast<std::size_t>(row)<rows.count);
      k.equal("page.info",actual,row>=0&&static_cast<std::size_t>(row)<rows.count?rows.rows[static_cast<std::size_t>(row)].address:0);
      group(Gate::actor);const auto actor=crt::actor(memory,s::game_parser,crt::assemble(ccat::recipe),c.hero);
      k.flag("actor.read",actor.read_valid);k.flag("actor.self",actor.requires_self);k.flag("actor.hero",actor.matches);
      group(Gate::dispatch);cpath::Verification path{};
      if(admission)path=cool_runtime_path(true);
      else{
        const bool before=phase==Phase::before_remove;
        const auto stage=model?model->tracker.status().stage:cool_predebit_tracker.status().stage;
        const bool quiet=cool_predebit_journal.sample(cool_fence_token,input.destroy_started,input.destroy_completed);
        const bool pending=stage==(before?cpre::Stage::calling_extra:cpre::Stage::extra_verified)&&native==before&&
          (model||(cool_predebit_executor&&!cool_predebit_executor->original_entered()))&&
          quiet;
        k.flag("dispatch.pending",pending);
        const auto* modules=end_compatibility(true);
        if(modules&&destroy_dispatch_reader)path=cpath::verify_dispatch(memory,cool_owned_patches(),cool_native_policy(true),*destroy_dispatch_reader,
          destroy_dispatch_context(c),frame,GetCurrentThreadId(),model?quiet:pending,nullptr,modules);
      }
      k.test("dispatch.issue","gueltiger Destroy-Pfad",static_cast<int>(path.issue),path.valid);
      k.test("dispatch.address","gepruefte Aufrufstelle",path.address,path.valid);
    }
  }
  const auto now=context();group(Gate::context);
  k.flag("call.closing_context",after?(now.identity_known&&now.station_known&&cpre::value::same(extra_context(c),extra_context(now))):same_context(c,now));
  k.equal("call.closing_info",actual,current_info_effect(now));
  group(Gate::modules);k.flag("module.engine",cool_engine_current());k.flag("module.plugins",end_compatibility(true)!=nullptr);
  k.flag("module.zpe",extra_zpe_module.current());k.flag("module.crt",extra_crt_module.current());
  group(Gate::hooks);k.flag("hooks.writer",scope&&bc::writer_undisturbed(*scope));k.flag("hooks.installed",cpath::installed_hooks_current(memory,cool_owned_patches()));
  group(Gate::journal);k.flag("journal.quiet",cool_predebit_journal.sample(cool_fence_token,input.destroy_started,input.destroy_completed));
  const auto& bound=info_effect_lease;
  const cpre::value::Context expected{bound.key.generation,bound.thread,s::game_parser,bound.game,bound.key.world,bound.key.hero,bound.key.station};
  // The diagnostic receiver may read only the bound ticket, even if the actual values differ.
  input.job={report?expected:extra_context(now),request.key.generation,scope?scope->token:0,cool_fence_token,bound.token,report?bound.actual_info:actual,s::information_manager};
  input.scope={{input.job.id,input.job.writer_token,cpre::callback,1,0,native?1u:0u,true},bound.token,input.job.actual_info,true};
  input.phase=phase;input.serial=next_cool_serial();input.frame=frame;input.current_thread=GetCurrentThreadId();
#ifdef UBC_PLAYER_RELEASE
  input.engine_sites_verified = cool_engine_current();
  input.engine = {s::image_base, s::image_size, {}, input.engine_sites_verified};
#else
  input.engine={cool_predebit_engine.base(),cool_predebit_engine.image_size(),cool_predebit_engine.digest(),true};
#endif
  input.modules=extra_modules(true);input.amount_patch=&patch;
  input.encoded_null_handler=reinterpret_cast<std::uint32_t>(EncodePointer(nullptr));
  const auto* tib=reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
  input.stack={true,input.current_thread,reinterpret_cast<std::uint32_t>(tib->StackLimit),reinterpret_cast<std::uint32_t>(tib->StackBase)};
  group(Gate::serial);k.test("call.serial",">0",input.serial,input.serial!=0);
  if(!k.ok()&&!report&&!rejection.event){rejection.failure=k.first;rejection.gate(phase,static_cast<Gate>(k.first.reason));}
  return k.ok();
}
bool __cdecl cool_predebit_boundary(void*,cpre::reader::Phase phase,cpre::reader::Input& input) noexcept {
  return inspect_cool_boundary(phase,input,nullptr);
}
cadmission::Prepared prepare_cool_predebit(const bc::WriterScope& scope) noexcept {
  constexpr auto phase=cpre::reader::Phase::admission;
  auto& rejection=amount_diagnostic.predebit;
  cpre::reader::Input input;
  if(!cool_predebit_reader){rejection.gate(phase,cpre::Gate::reader_missing);return {};}
  if(!late_gate()){rejection.gate(phase,cpre::Gate::late_gate);return {};}
  if(!cool_predebit_boundary(nullptr,phase,input))return {};
  const cadmission::WriterEntry writer{scope.token,reinterpret_cast<std::uint32_t>(scope.call.parser),
    static_cast<std::uint32_t>(scope.call.symbol),static_cast<std::uint32_t>(scope.call.caller),scope.call.thread,true,true};
  const auto prepared=cadmission::prepare(*cool_predebit_reader,memory,input,info_lease_owner,writer,&rejection);
  if(!prepared.valid())return {};
  cpre::reader::Input closing;
  if(!prepared.current(info_lease_owner)||!late_gate()||!cool_predebit_boundary(nullptr,phase,closing)||
     !cpre::same_job(prepared.job(),closing.job)||prepared.frame()!=closing.frame){rejection.gate(phase,cpre::Gate::closing);return {};}
  return prepared;
}
// At the same writer boundary as prepare_cool_predebit. Subsequent phases use actual
// read channels and private copies of the operand, quantity, tracker and result guard.
void inspect_cool_transaction(const bc::WriterScope& scope,const ubc::AmountMemory& live)noexcept{
  using Phase=cpre::reader::Phase;
  auto& entry=cool_check_reports.at(Phase::admission);cpre::check::Checks e(&entry);
  cpre::reader::Input input;
  e.flag("call.late_gate",late_gate());
  inspect_cool_boundary(Phase::admission,input,&entry);
  e.flag("call.reader",cool_predebit_reader.has_value());
  if(!cool_predebit_reader)return;
  const cadmission::WriterEntry writer{scope.token,reinterpret_cast<std::uint32_t>(scope.call.parser),
    static_cast<std::uint32_t>(scope.call.symbol),static_cast<std::uint32_t>(scope.call.caller),scope.call.thread,true,scope.original!=nullptr};
  cpre::reader::Capture captured;
  const auto prepared=cadmission::prepare(*cool_predebit_reader,memory,input,info_lease_owner,writer,nullptr,&entry,&captured);
  const auto& baseline=captured.observation;const auto& f=*ccat::single.information;
  const auto address=baseline.parser.code_base+f.byte_offset;
  e.flag("call.prepared",prepared.current(info_lease_owner));
  e.flag("call.closing_gate",late_gate());
  cpre::reader::Input closing;inspect_cool_boundary(Phase::admission,closing,&entry);
  e.flag("call.closing_job",cpre::same_job(input.job,closing.job));e.equal("call.closing_frame",input.frame,closing.frame);
  e.flag("amount.identity",live.identity&&live.identity(live.context,address,f.byte_length));
  e.flag("amount.writable",live.writable&&live.writable(live.context,address+62,4));
  const std::uint32_t one=1,two=2;
  e.flag("amount.forward_owner",cwriter::permits_operand_replace(prepared,info_lease_owner,&scope,cool_predebit_journal,
    cool_fence_token,address+62,&one,&two,4,[](){return current_info_effect(context());}));
  const auto& request=handoff.request();const auto* ticket=handoff.current_ticket();
  const ubc::CraftingResultBinding binding{request.key.generation,frame,ticket?ticket->offer.collection:0,end_sequence,
    request.key.world,request.key.hero,request.key.station};
  auto guard=result_guard;
  cpre::check::Checks result_checks(&entry);
  const bool waiting=guard.begin_cool_predebit(request,binding,prepared,info_lease_owner,&result_checks);
  e.flag("call.result_guard",waiting);
  cpre::inspection::SplitImage image(live,address,baseline.stack.object);
  ubc::AmountPatch patch;const std::array<ubc::AmountOperand,1> operands{{{62,1,1,0,false}}};
  const ubc::AmountBody body{f.byte_length,f.body_sha256,operands.data(),operands.size()};
  const bool patched=patch.begin(image.amount(),address,body,cpre::quantity);
  e.test("amount.begin","0",static_cast<int>(patch.issue()),patched);
  cpre::Tracker tracker;cpre::Call call;CoolProbePhase model{prepared,patch,tracker,waiting};
  for(const auto phase:cpre::inspection::phases){
    if(phase==Phase::admission)continue;
    if(phase==Phase::restore)image.after_split();
    auto& rows=cool_check_reports.at(phase);cpre::check::Checks k(&rows);
    cpre::reader::Input q;inspect_cool_boundary(phase,q,&rows,&model);
    if(cpre::reader::reads_saved_call(phase))q.call=&call;
    auto reading=cool_predebit_reader->capture(image.memory(),q,&rows);auto& v=reading.observation;
    const auto status=[&](const char* field,bool ok){k.test(field,"0",static_cast<int>(tracker.status().issue),ok);};
    if(phase==Phase::before_extra){
      cpre::check::Checks comparison(&rows);k.flag("call.prepared_matches",prepared.matches(v,&comparison));status("call.admit",tracker.admit(q.job,v));
      cpre::reader::Input late;inspect_cool_boundary(phase,late,&rows,&model);
      const auto last=cool_predebit_reader->capture(image.memory(),late,&rows);
      call=tracker.enter_extra(last.observation);status("call.enter_extra",call.valid);
      if(!call.valid){ // Measurement basis for independent restore fields only. Never execute.
        const auto& x=last.observation;
        call={false,q.job,x.serial,16612,1,x.stack.object,x.hand.left,x.parser,x.item,x.fence,x.inventory_identity,x.hand_identity,x.frame};
      }
    }else if(phase==Phase::before_remove||phase==Phase::restore){
      const auto plan=cpre::restore_plan(call,reading.restore);
      k.equal("call.pop_entry",0x7a4b20,plan.pop_entry);k.equal("call.instance_entry",0x7a1550,plan.instance_entry);
      k.equal("call.item_entry",0x7a2f20,plan.item_entry);
      k.flag("call.parser_unchanged",cpre::value::same(call.parser,reading.restore.current));
      k.flag("call.item_unchanged",cpre::old::same_item(call.item,reading.restore.item,call.parser));
      if(phase==Phase::before_remove){k.flag("call.stock_before",cpre::rows(v.stocks,baseline.stocks.stocks));
        k.flag("call.split_stack",cpre::old::listed_stack(v.stack,2));}
    }else if(phase==Phase::after_extra)status("call.extra_returned",tracker.extra_returned(v,true,true));
    else status("call.enter_original",tracker.enter_original(v));
  }
  e.flag("amount.private_restore",!patch.active()||patch.finish());
}
void* execute_cool_predebit(bc::WriterScope& scope,const ubc::AmountMemory& writable){
  DWORD error=GetLastError();void* result=nullptr;bool ran=false;alignas(16) unsigned char fx[512];
  auto& rejection=amount_diagnostic.predebit;
  if(cool_writer_scope){rejection.gate(cpre::reader::Phase::admission,cpre::Gate::writer_scope);return nullptr;}
  if(!(cool_fence_token=cool_predebit_journal.open())){rejection.gate(cpre::reader::Phase::admission,cpre::Gate::journal);return nullptr;}
  cool_writer_scope=&scope;
  __try {
    if(cool_context_mode){inspect_cool_transaction(scope,writable);amount_diagnostic.rejected=8;__leave;}
    const auto prepared=prepare_cool_predebit(scope);
    if(!prepared.valid()){amount_diagnostic.rejected=8;__leave;}
    const auto& request=handoff.request();const auto* ticket=handoff.current_ticket();
    const ubc::CraftingResultBinding binding{request.key.generation,frame,ticket?ticket->offer.collection:0,end_sequence,
      request.key.world,request.key.hero,request.key.station};
    cool_active_entry={};cool_destroy_call={};cool_destroy_binding={};cool_destroy_hand_diagnostic={};
    cool_job_floor=prepared.serial();cool_receipt_acked=false;
    const cwriter::Request run{prepared.job(),frame,&amount_patch,&prepared,&info_lease_owner,&rejection};
    SetLastError(error);
    ran=true;result=cool_predebit_executor->run(scope,amount_patch,writable,run,result_guard,request,binding);error=GetLastError();
  } __finally {
    _fxsave(fx);if(AbnormalTermination())error=GetLastError();
    effect_executed=ran&&cool_predebit_executor->extra_entered();
    amount_diagnostic.entered=ran&&cool_predebit_executor->original_entered();
    amount_diagnostic.restored=!ran||cool_predebit_executor->operands_restored();
    if(result_guard.cool_witness().valid&&result_guard.cool_witness().job.fence_token==cool_fence_token)cool_job_floor=cool_serial;
    if(!cool_predebit_journal.close(cool_fence_token)||!bc::writer_undisturbed(scope)){
      rejection.gate(rejection.phase,cpre::Gate::closing);
      result_guard.fail(ubc::CraftingResultIssue::destroy);
      if(effect_executed)scope.restore_verified=false;
      if(cool_predebit_executor)cool_predebit_executor->disable();
    }
    cool_fence_token=0;cool_writer_scope=nullptr;
    _fxrstor(fx);SetLastError(error);
  }
  return result;
}
bool verify_grind_amount(const bc::WriterScope& scope,const cat::Descriptor& recipe) noexcept {
  const auto& request=handoff.request();const auto c=context();const auto bridge=bc::diagnostics();
  auto& check=amount_diagnostic.grind_check;
  if(!check.equal("writer.descriptor",true,recipe.grind!=nullptr)||
     !check.equal("writer.route",static_cast<int>(cat::Route::grind),static_cast<int>(recipe.route))||
     !check.equal("writer.modules",true,grind_modules_current())||
     !check.equal("writer.context",true,c.valid)||
     !check.equal("writer.station",static_cast<int>(cat::Station::grind),static_cast<int>(c.kind))||
     !check.equal("writer.request_key",true,key(c)==request.key)||
     !check.equal("writer.exclusive",true,bridge.writer)||!check.equal("writer.readers",0,bridge.readers)||
     !check.equal("writer.disabled",false,bridge.writes_disabled)||!check.equal("writer.original",true,scope.original!=nullptr)||
     !check.equal("writer.thread",GetCurrentThreadId(),scope.call.thread)||
     !check.equal("writer.caller",bc::choice_call_return,scope.call.caller)||
     !check.equal("writer.callback",recipe.grind->function.symbol,scope.call.symbol)||
     !check.equal("writer.choice",true,current_grind_choice(c))||
     !check.equal("writer.native_update",true,native_update)||
     !check.equal("writer.update_depth",1,update_depth)||!check.equal("writer.choice_depth",1,choice_depth)||
     !check.equal("writer.info_depth",0,info_depth)||!check.equal("writer.station_depth",0,station_depth)||
     !check.equal("writer.collection_depth",0,collection_depth)||!check.equal("writer.read_depth",0,resolved_read_depth)||
     !check.equal("writer.active_patch",false,amount_patch.active())||!check.equal("writer.destroy_depth",0,cool_destroy_depth))return false;
  const auto k=grind_key(c);const GrindCaptureLease lease{k,true};
  if(!check.equal("writer.catalogue",true,lease.value!=nullptr))return false;
  const auto& raw=*lease.value;
  const auto station=grind_station_proof(c,raw,true);
  if(!check.equal("writer.station_hand_end",true,station.station_hand_end_verified))return false;
  const auto item=grind_item_reader->capture_writer(memory,raw,recipe.grind->function.symbol,request.quantity,
    grind_read_input(k,station.stable_boundary_verified));
  if(!item.item_path_verified){
    check.equal("writer.item_issue",static_cast<int>(shitem::Issue::none),static_cast<int>(item.issue));
    event("grind_item_path_rejected",static_cast<int>(item.issue),static_cast<int>(request.quantity));return false;
  }
  if(!check.equal("writer.item_binding",true,matching_grind_item(item,request,ubc::grind_item_path::Scope::writer))||
     !check.equal("writer.knowledge",true,matching_grind_knowledge(ubc::grind_writer_knowledge(raw,item.writer,k),request))||
     !check.equal("writer.handed_hands",true,ubc::same_crafting_hands(item.stock.input.hands,handed_hands)))return false;
  const auto after=ubc::read_grind_guild_for_writer(memory,s::game_parser,k);
  const auto assembly=shrt::assemble(*recipe.grind);
  if(!check.equal("writer.guild_read",true,after.known())||
     !check.equal("writer.guild_stable",true,ubc::grind_guild_snapshot_detail::same(item.writer.physical,after.physical))||
     !check.equal("writer.actor",true,ubc::station_recipe_runtime::same_actor_evidence(item.writer.actor_after,after.actor_after))||
     !check.equal("writer.parser",true,shrt::same_state(raw.diagnostic.state_before,shrt::state(memory,s::game_parser,assembly)))||
     !check.equal("writer.final_choice",true,current_grind_choice(c))||
     !check.equal("writer.final_modules",true,grind_modules_current())||
     !check.equal("writer.final_context",true,same_context(c,context())))return false;
  // late_gate has already consumed the PostRemoveChoice evidence exactly once. This
  // snapshot refreshes material, knowledge and SELF without consuming it again.
  const auto before=ubc::crafting_grind_evidence(request.material_snapshot);
  const auto numerical=shcat::plan(*recipe.grind,before,request.quantity);
  if(!numerical.valid){
    check.equal("writer.plan_issue",static_cast<int>(shcat::Issue::none),static_cast<int>(numerical.issue));
    return false;
  }
  grind_prepared={k,request.key.generation,before,numerical,item.stock.input.hands};grind_job_floor=grind_serial;
  grind_destroy_call={};grind_destroy_binding={};
  event("grind_item_path_verified",static_cast<int>(recipe.id),static_cast<int>(request.quantity));return true;
}
void tobacco_rows(int symbol) noexcept {
  for(std::size_t i=0;i<tobacco_report.count;++i){
    const auto& row=tobacco_report.rows[i];char fields[250]{};
    std::snprintf(fields,sizeof(fields),",\"field\":\"%s\",\"soll\":\"%s\",\"ist\":\"%s\"",
      row.field,row.expected,row.actual);
    event("tobacco_check_value",symbol,row.matches?1:0,fields);
  }
}
void tobacco_module_checks(ubc::bow_item_native::BowDispatch& targets,bool empty_pack,tobacco::Checks& checks) noexcept {
  const auto modules=extra_modules();
  const auto externals=ubc::bow_item_code::verify_externals(memory,s::game_parser);
  checks.equal("script.externals",0,static_cast<int>(externals.issue));
  for(unsigned i:{1u,2u}){
    const auto body=ubc::bow_item_native::detail::plain(memory,ubc::bow_item_code::ranges[i]);
    checks.equal(i==1?"script.create_items":"script.remove_items",0,static_cast<int>(body.issue));
  }
  // The game executes HasItems before the debit and Insert for the product.
  // Both target addresses belong to the same order until the child is selected.
  targets.empty_pack=empty_pack;
  if(tobacco_locator.observed())targets.locator={tobacco_locator.base(),tobacco_locator.image_size(),tobacco_locator.digest(),
    true,ubc::windows_probe::LoadedModuleIdentity::executable_page};
  const auto unpack=ubc::bow_item_native::detail::hasitems(memory,modules,nullptr,nullptr,&targets,
    ubc::bow_item_native::detail::PackedHasItems::observed_tobacco_wrapper);
  checks.equal("module.hasitems",0,static_cast<int>(unpack.issue));
  checks.test("module.unpack_target","gebunden",targets.unpack_target,unpack.verified);
  namespace insert=ubc::vob_inventory_insert_compat;
  std::uint32_t target=0;const bool read_target=read(insert::slot_address,target);
  const auto bound=ubc::bow_item_native::detail::forwarded(targets.locator,insert::slot_address,target,
    insert::original_target,targets.insert_target);
  checks.test("module.insert_target","Original oder gebundene Modulseite",target,bound.verified,read_target);
}

void tobacco_preconditions(const Context& c,const tobacco::Descriptor& d,const ubc::CraftingRequest* request,
    ubc::bow_item_native::BowDispatch& targets,tobacco::Capture& sample,tobacco::Checks& checks) noexcept {
  const auto ctx=extra_context(c);
  tobacco::AdmissionChecks admission(&tobacco_report);
  tobacco::verify_script(memory,admission);
  tobacco::lab_flags(memory,c.hero,true,admission);
  tobacco_reader.capture(memory,d,ctx,sample,admission);
  if(request)checks.flag("hand.same",ubc::same_crafting_hands(request->hands,sample.hands));
  constexpr const char* stock_fields[]={"stock.apple","stock.flask","stock.additive","stock.product"};
  const auto initial=request?tobacco::stocks(request->material_snapshot):sample.stock;
  for(unsigned i=0;i<initial.size();++i)checks.equal(stock_fields[i],initial[i],sample.stock[i],sample.stock[i]>=0);
  const unsigned quantity=request?request->quantity:1;
  checks.test("stock.quantity","1 bis Materialmaximum",quantity,quantity&&quantity<=tobacco::maximum(sample.stock));
  checks.flag("debit.reader",tobacco_list.has_value());
  const auto modules=extra_modules();
  for(unsigned i=0;i<2;++i){
    tobacco::AdmissionChecks path(&tobacco_report);
    if(tobacco_list)ubc::tobacco_writer::debit_path(memory,*tobacco_list,ctx,modules,d,sample,i,quantity,path);
    checks.flag(i?"debit.flask_path":"debit.apple_path",tobacco_list&&path.ok());
  }
  checks.flag("admission.complete",admission.ok());
  tobacco_module_checks(targets,sample.empty_pack,checks);
  const auto end=end_path(c.station);
  checks.equal("station.end",0,static_cast<int>(end.issue));
}
// Also records original calls and rejections before the writer is acquired.
void trace_tobacco_entry(const Context& c,const tobacco::Descriptor& d,int symbol,const ubc::CraftingRequest* request,
    const char* field,int expected,int actual) noexcept {
  tobacco_report.count=0;tobacco_report.overflow=false;
  tobacco::Checks checks(&tobacco_report),legacy(&tobacco_report);
  tobacco::parser_checks(memory,extra_context(c),checks,legacy);
  tobacco::Capture sample;ubc::bow_item_native::BowDispatch targets{};
  tobacco_preconditions(c,d,request,targets,sample,checks);
  checks.equal(field,expected,actual);
  tobacco_rows(symbol);
}
bool tobacco_vm_call(const bc::WriterScope& scope,void*& result) noexcept {
  __try {result=scope.original(scope.call.parser,scope.call.symbol);return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void* execute_tobacco_amount(bc::WriterScope& scope,const tobacco::Descriptor& d) {
  tobacco_writer_entered=true;
  const auto error=GetLastError();const auto& request=handoff.request();
  const bool child=scope.call.symbol!=20401;
  const auto c=context();const auto ctx=extra_context(c);const auto* ticket=handoff.current_ticket();
  tobacco_report.count=0;tobacco_report.overflow=false;
  tobacco::Checks checks(&tobacco_report),legacy(&tobacco_report);
  checks.flag("writer.thread",own_thread()&&scope.call.thread==ctx.thread);
  checks.flag("writer.exclusive",bc::writer_undisturbed(scope));
  checks.flag("writer.ticket",ticket&&ubc::crafting_handoff_detail::same_ticket(*ticket,dispatched_ticket));
  checks.flag("writer.executing",handoff.state()==ubc::CraftingHandoffState::executing&&!handoff.cancelled());
  checks.flag("writer.context",c.valid&&key(c)==request.key&&c.kind==cat::Station::alchemy);
  checks.flag("writer.depth",native_update&&update_depth==1&&!station_depth&&!collection_depth&&!amount_patch.active());
  checks.flag("writer.callback",child?(choice_depth==1&&!info_depth&&effect_authorized&&post_remove_choice.active()):
    (info_depth==1&&!choice_depth&&tobacco_owner_call&&!tobacco_owner_opened));
  checks.equal("writer.caller",child?bc::choice_call_return:bc::info_call_return,scope.call.caller);
  checks.equal("writer.symbol",child?d.callback.function->symbol:20401u,scope.call.symbol);
  checks.flag("writer.offer",ticket&&ticket->offer.owner==20399&&ticket->offer.function==scope.call.symbol);
  checks.flag("writer.result_idle",result_guard.permits_next());
  tobacco::parser_checks(memory,ctx,checks,legacy);
  if(child){
    checks.flag("owner.ticket",ticket&&tobacco_owner.owns(request.key,ticket->view,c.dialog.info)&&
      tobacco_owner.accepts(*ticket,scope.call.symbol)&&tobacco_choice_call==scope.call.symbol);
    checks.flag("gate.allowed",late_gate(nullptr,&checks));
  }else checks.flag("owner.available",!tobacco_owner.ticket);
  const auto initial=tobacco::stocks(request.material_snapshot);
  tobacco::Capture sample;
  tobacco_preconditions(c,d,&request,tobacco_targets,sample,checks);
  NativeAmountContext owner,choice;owner.function=tobacco::owner.information;choice.function=d.callback.function;
  const auto layout=rt::detail::read_layout(memory,s::game_parser,layout_profile,owner.layout);
  choice.layout=owner.layout;checks.equal("call.layout",0,static_cast<int>(layout));
  tobacco::Transaction tx;
  tobacco::Transaction::PaymentCode pay;
  const auto capture=[&](auto& window,NativeAmountContext& binding){
    const auto& f=*binding.function;
    return layout==rt::Issue::none&&f.byte_offset<=binding.layout.code_length&&
      f.byte_length<=binding.layout.code_length-f.byte_offset&&
      window.capture({memory,&binding,amount_identity,amount_writable,amount_replace},binding.layout.code+f.byte_offset,f);
  };
  const bool owner_captured=capture(tx.owner_body,owner),child_captured=capture(tx.child_body,choice);
  checks.flag("script.owner_body",owner_captured);checks.flag("script.child_body",child_captured);
  if(child)checks.flag("script.quantity",tx.prepare_child(d,request.quantity,sample.stock,pay));
  else tx.defer_owner();
  checks.flag("script.owner_writable",owner_captured&&tx.owner_body.ready());
  if(child)checks.flag("script.child_writable",child_captured&&tx.child_body.ready());
  checks.flag("writer.stable",same_context(c,context())&&bc::writer_undisturbed(scope));
  checks.flag("trace.complete",!tobacco_report.overflow);
  if(child){
    ammo_job=extra_job=spirit_job=cool_job=grind_job=false;
    result_started=GetTickCount64();
    amount_diagnostic={true,false,true,0,0,scope.call.symbol,static_cast<int>(request.quantity)};
  }
  ubc::CraftingResult prepared=result_guard;
  if(child){
    const ubc::CraftingResultBinding binding{request.key.generation,frame,ticket?ticket->offer.collection:0,
      end_sequence,request.key.world,request.key.hero,request.key.station};
    checks.flag("result.binding",prepared.begin(request,binding));
  }
  const auto live=[&]{
    const auto* current=handoff.current_ticket();
    return current&&ubc::crafting_handoff_detail::same_ticket(*current,dispatched_ticket)&&
      scope.original&&bc::writer_undisturbed(scope)&&same_context(c,context());
  };
  const auto ready_to_call=[&]{
    const bool stable=live();
    checks.flag("call.same_writer",stable);
    const bool windows=tx.at_call(checks);
    return stable&&windows;
  };
  void* result=nullptr;bool opened=false;
  auto settlement=tobacco::Settlement::unpaid;
  DWORD saved=error;
  const auto binding_end=end_sequence;
  bool output_returned=false,may_refund=true,vm_exception=false;
  const auto read_stock=[&](tobacco::Stock& stock){
    ubc::GrindCheck issue;tobacco::Capture fresh;
    const bool read=may_refund&&live()&&tobacco::lab_flags(memory,c.hero,true,issue)&&
      tobacco_reader.capture(memory,d,ctx,fresh,issue)&&ubc::same_crafting_hands(request.hands,fresh.hands)&&live();
    checks.flag("settle.read",read);
    if(issue.field)checks.equal(issue.field,issue.expected,issue.actual);
    if(read)stock=fresh.stock;
    return read;
  };
  const auto adjust=[&](unsigned i,unsigned count,const tobacco::Stock& observed){
    tobacco::Checks recovery(&tobacco_report),legacy_recovery(&tobacco_report);
    tobacco::parser_checks(memory,ctx,recovery,legacy_recovery);
    tobacco_module_checks(tobacco_targets,sample.empty_pack,recovery);
    const auto code=tx.adjustment(d,i,count,observed);
    if(!recovery.ok()||!live()||!tx.payment_program(code,checks)||!ready_to_call())return false;
    void* ignored=nullptr;
    const bool returned=tobacco_vm_call(scope,ignored);
    if(!returned)vm_exception=true;
    checks.flag("refund.returned",returned);return returned;
  };
  const auto authorized=[&]{
    tobacco::Checks lease(&tobacco_report);
    lease.flag("lease.ticket",ticket&&live());lease.flag("lease.cancelled",!handoff.cancelled());
    lease.equal("lease.handoff",static_cast<int>(ubc::CraftingHandoffState::executing),static_cast<int>(handoff.state()));
    lease.flag("lease.native_update",native_update);lease.equal("lease.update_depth",1,update_depth);
    lease.equal("lease.collect_depth",0,collection_depth);lease.equal("lease.station_depth",0,station_depth);
    lease.flag("lease.focus",ui.focused());lease.flag("lease.result_idle",result_guard.permits_next());
    lease.equal("lease.collection",ticket?ticket->offer.collection:0,rows.generation);
    lease.equal("lease.view",ticket?ticket->view:0,c.dialog.view);
    lease.equal("lease.choice_depth",child?1:0,choice_depth);lease.equal("lease.info_depth",child?0:1,info_depth);
    if(child){
      lease.flag("lease.authorized",effect_authorized);lease.equal("lease.symbol",scope.call.symbol,tobacco_choice_call);
      lease.flag("lease.owner",ticket&&tobacco_owner.owns(request.key,ticket->view,c.dialog.info)&&
        tobacco_owner.bound_child(*ticket,scope.call.symbol,rows.generation));
    }else lease.flag("lease.owner",tobacco_owner_call!=0);
    return lease.ok();
  };
  const auto finish=[&](const tobacco::Stock& paid,bool& entered){
    tobacco::Checks final_checks(&tobacco_report),old_checks(&tobacco_report);
    tobacco::parser_checks(memory,ctx,final_checks,old_checks);
    tobacco_module_checks(tobacco_targets,sample.empty_pack,final_checks);
    const bool permitted=output_returned&&authorized();
    checks.equal("payment.collection",ticket?ticket->offer.collection:0,rows.generation);
    checks.flag("payment.authorized",permitted);
    if(!permitted||!final_checks.ok())return false;
    const bool installed=tx.payment_program(pay,checks);
    checks.flag("payment.installed",installed);
    if(!installed||!authorized()||!ready_to_call())return false;
    entered=true;
    void* payment=nullptr;
    const bool returned=tobacco_vm_call(scope,payment);
    if(!returned)vm_exception=true;
    checks.flag("payment.returned",returned);
    // These two VM phases contain neither PRINT nor ENDE. Even an exception
    // after the last removal therefore leaves an unambiguous reconciliation.
    tobacco::Stock after{};
    const bool known=read_stock(after);
    constexpr const char* fields[]={"payment.apple","payment.flask","payment.additive","payment.product"};
    for(unsigned i=0;i<paid.size();++i)checks.equal(fields[i],paid[i],after[i],known);
    if(!returned||!known||after!=paid||!authorized()||end_sequence!=binding_end)return false;
    const auto done=tx.completion();
    const bool complete=tx.payment_program(done,checks);
    checks.flag("completion.installed",complete);
    if(!complete||!authorized()||!ready_to_call())return false;
    may_refund=false;result_guard=prepared;effect_executed=true;
    const bool completed=tobacco_vm_call(scope,result);
    if(!completed)vm_exception=true;
    saved=GetLastError();checks.flag("completion.returned",completed);
    return completed;
  };
  __try {
    if(checks.ok()&&tx.install(child,checks)&&authorized()&&ready_to_call()){
      SetLastError(error);
      if(child){
        tobacco_owner.spent=true;
        settlement=tobacco::Settlement::uncertain;
        void* staged=nullptr;
        output_returned=tobacco_vm_call(scope,staged);
        vm_exception=!output_returned;checks.flag("output.returned",output_returned);
        settlement=tobacco::settle(initial,request.quantity,read_stock,adjust,finish,checks);
        if(vm_exception&&settlement==tobacco::Settlement::unpaid){
          tobacco::Checks recovery(&tobacco_report),old_checks(&tobacco_report);
          tobacco::parser_checks(memory,ctx,recovery,old_checks);
          if(!recovery.ok()||!live())settlement=tobacco::Settlement::uncertain;
        }
      }else{
        tobacco_owner={request.key,ticket->id,ticket->offer.collection,0,tobacco_owner_call,ticket->view,
          static_cast<int>(d.callback.function->symbol)};
        InterlockedExchange(&tobacco_unpaid_page,1);
        result=scope.original(scope.call.parser,scope.call.symbol);
        saved=GetLastError();opened=true;
      }
    }
  } __finally {
    const bool abnormal=AbnormalTermination()!=FALSE;
    if(abnormal)saved=GetLastError();
    const bool restored=tx.restore();
    scope.restore_verified=restored&&!abnormal&&settlement!=tobacco::Settlement::uncertain;
    if(!scope.restore_verified){
      bc::quarantine(20401);bc::quarantine(d.callback.function->symbol);
      result_guard.fail(ubc::CraftingResultIssue::restore);
    }
    checks.flag("script.restored",restored);
    checks.flag("inventory.settled",settlement!=tobacco::Settlement::uncertain);
    checks.flag("call.executed",child?effect_executed:opened);
    if(child&&settlement!=tobacco::Settlement::completed){
      for(const char* field:{"call.owner_window","call.child_window","output.returned","payment.authorized",
          "payment.installed","payment.returned","payment.apple","payment.flask","payment.additive","payment.product",
          "completion.installed","completion.returned"}){
        bool found=false;
        for(std::size_t i=0;i<tobacco_report.count;++i)if(!std::strcmp(tobacco_report.rows[i].field,field)){found=true;break;}
        if(!found)checks.record(field,"geprueft","nicht erreicht",false);
      }
    }
    tobacco_rows(scope.call.symbol);
    if(child){
      amount_diagnostic.entered=effect_executed;amount_diagnostic.restored=scope.restore_verified;
      if(effect_executed)result_guard.restored(scope.restore_verified);
      if(settlement!=tobacco::Settlement::completed){
        amount_diagnostic.rejected=10;
        handoff.cancel(ubc::CraftingHandoffIssue::resources);
      }
    }else tobacco_owner_opened=opened&&scope.restore_verified;
    if((child?settlement==tobacco::Settlement::unpaid:!opened)&&scope.restore_verified&&result_guard.permits_next()&&
       !bc::diagnostics().writes_disabled){
      tobacco_clean_rejection=true;event("tobacco_order_unpaid",d.id,request.quantity);
    }
    SetLastError(saved);
  }
  return result;
}
void* __cdecl execute_amount(bc::WriterScope& scope) {
  const auto incoming_error=GetLastError();scope.restore_verified=true;
  const auto& request=handoff.request();
  const auto* recipe=cat::find(request.id);
  if(recipe&&recipe->tobacco)return execute_tobacco_amount(scope,*recipe->tobacco);
  amount_diagnostic={true,false,true,0,0,scope.call.symbol,static_cast<int>(request.quantity)};
  const auto* entry=recipe?callback_for(*recipe,request.quantity):nullptr;
  const auto cool_plan=recipe&&recipe->cool?ubc::crafting_cool_plan(request.material_snapshot,request.quantity):ubc::cool_recipe::Plan{};
  const bool predebit=cool_plan.valid&&cool_plan.requires_extra_debit;
  const bool inspect=predebit&&cool_context_mode;
  if(inspect)cool_check_reports.clear();
  if(predebit)amount_diagnostic.cool_info_token=info_effect_lease.token;
  if(cool_context_mode&&recipe&&recipe->cool&&!inspect){
    handoff.cancel(ubc::CraftingHandoffIssue::resources);amount_diagnostic.rejected=8;SetLastError(incoming_error);return nullptr;
  }
  if(predebit&&!cpre::runtime_ready){
    handoff.cancel(ubc::CraftingHandoffIssue::resources);amount_diagnostic.rejected=8;SetLastError(incoming_error);return nullptr;
  }
  const auto* function=recipe&&recipe->cool?(cool_plan.valid?
    (cool_plan.callback==20775?ccat::single.information:cool_plan.callback==20778?ccat::triple.information:nullptr):nullptr):
    recipe&&recipe->grind?&recipe->grind->function:recipe&&recipe->spirit?recipe->spirit->callback:entry?entry->function:nullptr;
  const bool matching=function&&function->symbol==static_cast<std::uint32_t>(scope.call.symbol);
  if(inspect){
    cpre::check::Checks rows(&cool_check_reports.at(cpre::reader::Phase::admission));
    rows.flag("call.function",matching);rows.flag("call.guard_idle",result_guard.permits_next());
  }
  auto* grind_check=(request.key.kind==cat::Station::grind||(recipe&&recipe->tobacco))?&amount_diagnostic.grind_check:nullptr;
  const bool idle=inspect||!matching||result_guard.permits_next();
  if(!inspect&&(!matching||!idle||!late_gate(grind_check))){
    if(grind_check){
      if(!function)grind_check->equal("call.function",true,false);
      else if(!matching)grind_check->equal("call.symbol",function->symbol,scope.call.symbol);
      else if(!idle)grind_check->equal("call.guard_idle",true,false);
    }
    handoff.cancel(ubc::CraftingHandoffIssue::resources);amount_diagnostic.rejected=1;SetLastError(incoming_error);return nullptr;
  }
  NativeAmountContext ctx;ctx.function=function;ctx.predebit=predebit;
  const auto layout=rt::detail::read_layout(memory,s::game_parser,layout_profile,ctx.layout);
  const bool fits=function&&function->byte_offset<=ctx.layout.code_length&&function->byte_length<=ctx.layout.code_length-function->byte_offset;
  if(inspect){cpre::check::Checks rows(&cool_check_reports.at(cpre::reader::Phase::admission));
    rows.equal("call.layout",0,static_cast<int>(layout));rows.flag("call.body_range",fits);}
  if(!inspect&&(layout!=rt::Issue::none||!fits)){
    if(grind_check){
      if(layout!=rt::Issue::none)grind_check->equal("call.layout",static_cast<int>(rt::Issue::none),static_cast<int>(layout));
      else grind_check->equal_unsigned("call.minimum_code_length",std::uint64_t{function->byte_offset}+function->byte_length,ctx.layout.code_length);
    }
    handoff.cancel(ubc::CraftingHandoffIssue::context);amount_diagnostic.rejected=2;SetLastError(incoming_error);return nullptr;
  }
  ammo_job=recipe->station==cat::Station::saw&&request.quantity>1;ammo_notice_ready=ammo_notice_acked=false;
  extra_job=recipe->route==cat::Route::alchemy_extra;
  spirit_job=recipe->route==cat::Route::spirit;
  cool_job=recipe->route==cat::Route::cool;
  grind_job=recipe->route==cat::Route::grind;grind_original_returned=grind_skill_log_pending=false;
  grind_skill_after_values={};
  if(recipe->bow&&!verify_bow_amount(scope,*recipe)){
    handoff.cancel(ubc::CraftingHandoffIssue::resources);amount_diagnostic.rejected=5;SetLastError(incoming_error);return nullptr;
  }
  if(cool_job&&!predebit&&!verify_cool_amount(scope,*recipe)){
    handoff.cancel(ubc::CraftingHandoffIssue::resources);amount_diagnostic.rejected=6;SetLastError(incoming_error);return nullptr;
  }
  if(grind_job&&!verify_grind_amount(scope,*recipe)){
    handoff.cancel(ubc::CraftingHandoffIssue::resources);amount_diagnostic.rejected=7;SetLastError(incoming_error);return nullptr;
  }
  if(ammo_job){
    ap::Context current;
    const ap::Admission proof{false,false,request_ui_generation==request.key.generation&&request_ui_key==request.key,
      true,request.material_snapshot.actor_known&&request.material_snapshot.actor_matches};
    if(!read_ammo_context(nullptr,current)||!ammo::admit({current,request.key.generation,request.key.generation,
      entry->function->symbol,static_cast<std::uint32_t>(entry->owner_instance),request.quantity},proof)){
      handoff.cancel(ubc::CraftingHandoffIssue::resources);amount_diagnostic.rejected=4;SetLastError(incoming_error);return nullptr;
    }
    ammo_diagnostic={request.key.generation,entry->function->symbol,0,0};
  }
  LARGE_INTEGER before_time{},after_time{},frequency{};
  QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&before_time);result_started=GetTickCount64();
  const ubc::AmountMemory writable{memory,&ctx,amount_identity,amount_writable,amount_replace};ubc::AmountOutcome outcome;
  SetLastError(incoming_error);
  void* result=nullptr;alignas(16) unsigned char predebit_fx[512];
  if(extra_job){
    result=execute_extra_amount(scope,writable,*recipe);outcome.entered=effect_executed;
    outcome.issue=extra_executor?extra_executor->outcome().amount_issue:ubc::AmountIssue::contract;
  }else if(spirit_job){
    result=execute_spirit_amount(scope,writable,*recipe);outcome.entered=effect_executed;
    outcome.issue=spirit_executor?spirit_executor->outcome().amount_issue:ubc::AmountIssue::contract;
  }else if(predebit){
    result=execute_cool_predebit(scope,writable);_fxsave(predebit_fx);
    outcome.entered=result_guard.cool_witness().valid&&result_guard.state()==ubc::CraftingResultState::waiting;
    outcome.issue=amount_patch.issue();
  }else if(cool_job)result=ubc::run_amount_call(amount_patch,writable,ctx.layout.code+ctx.function->byte_offset,
    cool_active_entry.body(),request.quantity,execute_original_amount,&scope,scope.restore_verified,outcome);
  else if(grind_job)result=ubc::run_amount_call(amount_patch,writable,ctx.layout.code+ctx.function->byte_offset,
    grind_prepared.amount.body(),request.quantity,execute_original_amount,&scope,scope.restore_verified,outcome);
  else result=ubc::run_amount_call(amount_patch,writable,ctx.layout.code+ctx.function->byte_offset,
    entry->amount,request.quantity,execute_original_amount,&scope,scope.restore_verified,outcome);
  const auto saved=GetLastError();
  if(!predebit){amount_diagnostic.entered=effect_executed;amount_diagnostic.restored=scope.restore_verified;}
  if(ammo_job&&!ammo::healthy())note_ammo_diagnostic(AmmoDiagnosticPhase::postcall_health);
  if(!outcome.entered){
    handoff.cancel(ubc::CraftingHandoffIssue::resources);
    if(!amount_diagnostic.rejected&&!amount_diagnostic.predebit.event)
      amount_diagnostic.rejected=10+static_cast<int>(outcome.issue);
    if(ammo_job)result_guard.fail(scope.restore_verified?ubc::CraftingResultIssue::contract:ubc::CraftingResultIssue::restore);
  }
  if(effect_executed&&!extra_job&&!spirit_job&&!predebit)result_guard.restored(scope.restore_verified);
  if(grind_job&&effect_executed&&scope.restore_verified){
    const auto bridge=bc::diagnostics();
    grind_skill_after_values=read_grind_result_values(request);
    const bool original_bound=grind_original_returned&&outcome.entered&&own_thread()&&bridge.writer&&!bridge.readers&&
      !bridge.writes_disabled&&scope.call.thread==GetCurrentThreadId()&&scope.call.caller==bc::choice_call_return&&
      scope.call.symbol==static_cast<int>(grind_prepared.before.callback)&&grind_prepared.job==request.key.generation&&
      grind_prepared.key.frame==frame&&grind_modules_current()&&grind_skill_after_values.known;
    grind_skill_log_pending=result_guard.skill_after({grind_original_returned,original_bound,request.key,request.id,
      request.quantity,next_grind_serial(),frame,request.material_snapshot.grind,grind_skill_after_values});
    if(!grind_skill_log_pending){
      auto& check=amount_diagnostic.grind_check;
      check.equal("skill.original_return",true,grind_original_returned);
      check.equal("skill.original_enter",true,outcome.entered);
      ubc::diagnose_grind_skill(check,grind_prepared,grind_skill_after_values);
      check.equal("skill.original_binding",true,original_bound);
      check.equal("skill.result_guard",true,false);
    }
  }
  if(!scope.restore_verified){handoff.cancel(ubc::CraftingHandoffIssue::resources);result_guard.fail(ubc::CraftingResultIssue::restore);}
  QueryPerformanceCounter(&after_time);
  if(frequency.QuadPart>0&&after_time.QuadPart>=before_time.QuadPart){
    const auto elapsed=static_cast<double>(after_time.QuadPart-before_time.QuadPart)*1000000.0/static_cast<double>(frequency.QuadPart);
    amount_diagnostic.duration=elapsed>=INT32_MAX?INT32_MAX:static_cast<int>(elapsed);
  }
  if(predebit)_fxrstor(predebit_fx);
  SetLastError(saved);return result;
}

bool bound_callback(const ubc::CraftingHandoffOffer& offer) noexcept {
  const auto* ticket=handoff.current_ticket();
  return ticket&&offer.valid&&offer.owner==ticket->offer.owner&&offer.function==ticket->offer.function;
}
bool callback_guarded(const Context& c,const ubc::CraftingHandoffOffer& offer) noexcept {
  if(!handoff.active())return false;
  if(bound_callback(offer))return true;
  // No further order goes to the engine after the effect. Only the
  // old callback remains protected against repetition. A normal end
  // path or a confirmed unrelated dialog must be able to continue.
  if(handoff.state()==ubc::CraftingHandoffState::result_pending)return !offer.valid;
  if(foreign_dialog(c)&&offer.valid){handoff.cancel(ubc::CraftingHandoffIssue::context);return false;}
  return true;
}
ubc::CraftingHandoffCallbackProof proof(std::uintptr_t caller,bool info) noexcept {
  return {own_thread(),caller==(info?a::info_effect_return:a::choice_effect_return),native_update,
    update_depth,info?info_depth:choice_depth,info?choice_depth:info_depth,collection_depth,station_depth};
}
void __fastcall observe_update(void* manager,void* unused) {
  const auto error=GetLastError();const bool own=own_thread();
  const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());const bool previous_native=own?native_update:false;
  if(own){++update_depth;native_update=update_depth==1&&reinterpret_cast<std::uint32_t>(manager)==s::information_manager&&
    (caller==0x74c117||caller==0x75a727||caller==0x6c8a46);
    if(handoff.active()&&!ui.focused())handoff.cancel(ubc::CraftingHandoffIssue::focus);}
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
    ubc::CraftingHandoffOffer actual{info.valid,basecat::Invocation::info,info.instance,info.function,row,rows.generation};
    ++resolved_read_depth;const auto current=observation(c,false);--resolved_read_depth;
    if(callback_guarded(c,actual)){
      const bool different=current.context_known&&current.view&&
        (!(current.key==handoff.request().key)||current.view!=dispatched_ticket.view);
      if(different&&actual.valid&&!bound_callback(actual)){handoff.cancel(ubc::CraftingHandoffIssue::context);}
      else if(!linked_step_info(info,handoff.step())||!native_update||update_depth!=1||info_depth||choice_depth||station_depth||collection_depth||
        reinterpret_cast<std::uint32_t>(player)!=handoff.request().key.hero||
        reinterpret_cast<std::uint32_t>(npc)!=handoff.request().key.hero||
        !handoff.matches_pending(dispatched_ticket,current,actual)){
        result=nullptr;recover_info=true;handoff.cancel(ubc::CraftingHandoffIssue::offer);event("resolution_rejected");
      }
    }
  }SetLastError(after);return result;
}
void __fastcall observe_info(void* info,void*) {
  const auto error=GetLastError();const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());const bool own=own_thread();
  // The original INFO may change during the call. On normal
  // return, log only the identifiers read beforehand.
  const auto observed=campfire_observation_mode&&own
    ?ubc::read_selection_info(memory,reinterpret_cast<std::uint32_t>(info)):ubc::SelectionInfo{};
  if(campfire_observation_mode&&own){
    const auto c=context();
    const auto production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
    if(caller!=a::info_effect_return)event("campfire_caller",a::info_effect_return,static_cast<int>(caller));
    if(!c.ownership_known)event("campfire_owner",1,0);
    if(!name_is(c.station,0x1b8,"CAMPFIRE"))event("campfire_scheme",1,0);
    if(!name_is(c.station,0x1e0,"FRYINGMEAT"))event("campfire_on_state",1,0);
    if(!name_is(c.station,0x1cc,"B_USECAMPFIRE"))event("campfire_condition",1,0);
    if(!name_is(c.station,0x1a4,""))event("campfire_use_empty",1,0);
    if(!production.valid||production.value!=14)event("campfire_production",14,production.valid?production.value:-1);
  }
  const auto prior_tobacco_owner_call=tobacco_owner_call;
  const auto previous_lease=own?info_effect_lease:InfoEffectLease{};
  const bool previous_authorized=own?effect_authorized:false;
  if(own){info_effect_lease={};info_lease_owner.invalidate();}
  if(own)++info_depth;
  ubc::CraftingHandoffCallback witness;bool allow=true;bool final_effect=false;
  DWORD after=error;
  __try {
    if(own&&handoff.active()){
      const auto c=context();const auto actual_info=ubc::read_selection_info(memory,reinterpret_cast<std::uint32_t>(info));
      const ubc::CraftingHandoffOffer actual{actual_info.valid,basecat::Invocation::info,actual_info.instance,actual_info.function,c.dialog.selected,rows.generation};
      if(callback_guarded(c,actual)){
        auto verified=proof(caller,true);verified.canonical_caller=verified.canonical_caller&&linked_step_info(actual_info,handoff.step());
        witness=handoff.enter_callback(dispatched_ticket,observation(c,false),actual,verified);
        allow=witness.decision!=ubc::CraftingHandoffDecision::block;
        final_effect=witness.decision==ubc::CraftingHandoffDecision::execute&&handoff.step()&&handoff.step()->is_effect();
        if(final_effect){
          const auto* ticket=handoff.current_ticket();
          if(ticket&&caller==a::info_effect_return&&info_depth==1&&!choice_depth){
            info_effect_lease={reinterpret_cast<std::uint32_t>(info),GetCurrentThreadId(),c.game,
              info_depth,handoff.request().key,*ticket};
            info_effect_lease.token=info_lease_owner.open({extra_context(c),frame,
              reinterpret_cast<std::uint32_t>(info),s::information_manager});
          }
        }
      }
    }
    const bool owned_tobacco=allow&&witness.decision==ubc::CraftingHandoffDecision::execute&&
      dispatched_ticket.offer.function==20401&&dispatched_ticket.offer.owner==20399;
    if(owned_tobacco){tobacco_writer_entered=false;tobacco_owner_opened=false;tobacco_owner_call=reinterpret_cast<std::uint32_t>(info);}
    if(own)effect_authorized=final_effect;
    if(final_effect&&!current_info_effect(context())){
      allow=false;handoff.cancel(ubc::CraftingHandoffIssue::offer);
    }
    SetLastError(error);if(allow)original_info(info);after=GetLastError();
    if(owned_tobacco&&!tobacco_owner_opened){
      allow=false;handoff.cancel(ubc::CraftingHandoffIssue::resources);
      if(!tobacco_writer_entered&&result_guard.permits_next()&&!bc::diagnostics().writes_disabled){
        tobacco_clean_rejection=true;event("tobacco_order_unpaid",handoff.request().id,handoff.request().quantity);
      }
    }
    if(campfire_observation_mode&&own&&allow)event("campfire_info_returned",observed.instance,observed.function);
  } __finally {
    const bool abnormal=AbnormalTermination()!=FALSE;
    if(abnormal)after=GetLastError();
    if(own){
      tobacco_owner_call=prior_tobacco_owner_call;
      if(!abnormal||amount_diagnostic.cool_info_token)flush_amount_diagnostic(info_effect_lease.token);
      if(abnormal){
        if(witness.decision==ubc::CraftingHandoffDecision::execute)handoff.cancel(ubc::CraftingHandoffIssue::offer);
        if(final_effect)result_guard.fail(ubc::CraftingResultIssue::contract);
      }else{
        if(witness.decision==ubc::CraftingHandoffDecision::execute){
          if(allow&&handoff.step()&&handoff.step()->function_symbol==20401&&
             !handoff.cancelled()&&dispatched_ticket.offer.function==20401&&dispatched_ticket.offer.owner==20399){
            event("tobacco_owner_returned",20401,1);
          }
          if(!tobacco_clean_rejection||!handoff.reject_unpaid_tobacco(witness))handoff.callback_return(witness);
          event(allow?"native_callback_returned":"effect_rejected",dispatched_ticket.offer.function);
          if(final_effect&&!effect_executed)result_guard.fail(ubc::CraftingResultIssue::contract);
        }else if(!allow)event("effect_rejected");
      }
      info_lease_owner.close(info_effect_lease.token);
      info_effect_lease=previous_lease;effect_authorized=previous_authorized;--info_depth;
    }
    SetLastError(after);
  }
}
void __fastcall observe_choice(void* manager,void* choice) {
  const auto error=GetLastError();const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());const bool own=own_thread();
  const auto previous_grind_lease=grind_choice_lease;
  const int previous_tobacco_choice=tobacco_choice_call;
  const bool previous_writer_entered=tobacco_writer_entered;
  bool unpaid_effect=false;
  const bool previous_authorized=own?effect_authorized:false;
  if(own){++choice_depth;grind_choice_lease={};tobacco_choice_call=0;tobacco_writer_entered=false;}
  ubc::CraftingHandoffCallback witness;bool allow=true;bool final_effect=false;bool removal_owned=false;
  DWORD after=error;
  __try {
  if(own&&(handoff.active()||tobacco_owner.ticket)){
    const auto c=context();const auto actual_choice=ubc::read_choice(memory,reinterpret_cast<std::uint32_t>(manager),reinterpret_cast<std::uint32_t>(choice));
    const auto owner=ubc::read_selection_info(memory,c.dialog.info);
    const ubc::CraftingHandoffOffer actual{actual_choice.valid,basecat::Invocation::choice,
      actual_choice.info.instance,actual_choice.function,actual_choice.index,rows.generation};
    const bool unpaid=tobacco_owner.ticket!=0;
    const bool bound_page=unpaid&&actual.valid&&actual.owner==20399&&linked(owner)&&
      tobacco_owner.owns(key(c),c.dialog.view,c.dialog.info)&&caller==a::choice_effect_return&&
      reinterpret_cast<std::uint32_t>(manager)==s::information_manager&&choice_depth==1;
    const bool back=bound_page&&actual.function==20402;
    if(back){
      handoff.cancel(ubc::CraftingHandoffIssue::cancelled);
      tobacco_choice_call=20402;
    }else if(callback_guarded(c,actual)){
      auto verified=proof(caller,false);
      verified.canonical_caller=verified.canonical_caller&&linked(owner)&&actual_choice.info.instance==owner.instance&&
        reinterpret_cast<std::uint32_t>(manager)==s::information_manager;
      witness=handoff.enter_callback(dispatched_ticket,observation(c,false),actual,verified);
      allow=witness.decision!=ubc::CraftingHandoffDecision::block;
      final_effect=witness.decision==ubc::CraftingHandoffDecision::execute&&handoff.step()&&handoff.step()->is_effect();
      if(unpaid){
        allow=allow&&final_effect&&bound_page&&tobacco_owner.accepts(dispatched_ticket,actual.function);
        unpaid_effect=final_effect;
        if(allow){tobacco_choice_call=actual.function;tobacco_writer_entered=false;}
      }
      if(allow&&final_effect){
        const auto current_grind_page=c.kind==cat::Station::grind?grind_page(c):shnav::PageEvidence{};
        removal_owned=post_remove_choice.begin(memory,c.dialog,rows.generation,dispatched_ticket.id,
          reinterpret_cast<std::uint32_t>(choice),actual.function);
        if(!removal_owned){allow=false;handoff.cancel(ubc::CraftingHandoffIssue::offer);}
        else if(c.kind==cat::Station::grind){
          if(!shnav::valid_page(current_grind_page)||ubc::crafting_page(current_grind_page)!=handoff.step()->before){
            allow=false;handoff.cancel(ubc::CraftingHandoffIssue::offer);
          }else grind_choice_lease={c.dialog.info,GetCurrentThreadId(),c.game,choice_depth,frame,key(c),dispatched_ticket,current_grind_page};
        }
      }
    }else if(unpaid)allow=false;
  }
  if(own)effect_authorized=allow&&final_effect;
  SetLastError(error);
  if(allow)original_choice(manager,choice);after=GetLastError();
  } __finally {
    const bool abnormal=AbnormalTermination()!=FALSE;if(abnormal)after=GetLastError();
    if(removal_owned)post_remove_choice.clear();
    if(own){
      if(abnormal){
        if(witness.decision==ubc::CraftingHandoffDecision::execute)handoff.cancel(ubc::CraftingHandoffIssue::offer);
        if(final_effect)result_guard.fail(ubc::CraftingResultIssue::contract);
      }else{
        flush_amount_diagnostic();
        if(witness.decision==ubc::CraftingHandoffDecision::execute){
          if(unpaid_effect&&!tobacco_writer_entered&&!effect_executed){
            const auto& request=handoff.request();const auto* d=cat::find(request.id);
            if(d&&d->tobacco)trace_tobacco_entry(context(),*d->tobacco,dispatched_ticket.offer.function,&request,
              "call.writer_entered",1,0);
          }
          if(unpaid_effect&&!tobacco_writer_entered&&!effect_executed&&result_guard.permits_next()&&!bc::diagnostics().writes_disabled){
            handoff.cancel(ubc::CraftingHandoffIssue::offer);tobacco_clean_rejection=true;
            event("tobacco_order_unpaid",handoff.request().id,handoff.request().quantity);
          }
          if(!tobacco_clean_rejection||!handoff.reject_unpaid_tobacco(witness))handoff.callback_return(witness);
          event(allow?"native_callback_returned":"effect_rejected",dispatched_ticket.offer.function);
          if(final_effect&&!effect_executed&&!tobacco_clean_rejection)result_guard.fail(ubc::CraftingResultIssue::contract);
        }else if(!allow)event("effect_rejected");
      }
      tobacco_writer_entered=previous_writer_entered;tobacco_choice_call=previous_tobacco_choice;grind_choice_lease=previous_grind_lease;effect_authorized=previous_authorized;--choice_depth;
    }
    SetLastError(after);
  }
}
void __fastcall observe_start(void* mob,void*,void* npc){
  const auto error=GetLastError();const bool own=own_thread();if(own)++station_depth;
  SetLastError(error);original_start(mob,npc);const auto after=GetLastError();
  if(own)--station_depth;SetLastError(after);
}
bool matches_station_profile(std::uint32_t mob,cat::Station kind) noexcept {
  if(kind==cat::Station::stove)return name_is(mob,0x1e0,"FRYINGMEAT")&&name_is(mob,0x1b8,"STOVE")&&
    name_is(mob,0x1a4,"")&&name_is(mob,0x1cc,"B_USESTOVE");
  if(kind==cat::Station::alchemy)return name_is(mob,0x1e0,"POTIONALCHEMY")&&
    name_is(mob,0x1a4,"ITMI_FLASK")&&name_is(mob,0x1cc,"");
  if(kind==cat::Station::anvil)return name_is(mob,0x1e0,acat::station.on_state)&&name_is(mob,0x1b8,acat::station.scheme)&&
    name_is(mob,0x1a4,acat::station.use_with_item)&&name_is(mob,0x1cc,acat::station.condition_name);
  if(kind==cat::Station::bow)return name_is(mob,0x1e0,bcat::station.on_state)&&name_is(mob,0x1b8,bcat::station.scheme)&&
    name_is(mob,0x1a4,bcat::station.use_with_item)&&name_is(mob,0x1cc,bcat::station.condition_name);
  if(kind==cat::Station::cool)return name_is(mob,0x1e0,ccat::station.on_state)&&name_is(mob,0x1b8,ccat::station.scheme)&&
    name_is(mob,0x1a4,ccat::station.use_with)&&name_is(mob,0x1cc,ccat::station.condition)&&
    integer(mob,0x200)==ccat::station.rewind;
  if(kind==cat::Station::grind)return name_is(mob,0x1e0,"SMITHSHARP")&&name_is(mob,0x1b8,"BSSHARP")&&
    name_is(mob,0x1a4,"")&&name_is(mob,0x1cc,"")&&integer(mob,0x200)==0;
  for(const auto& profile:scat::stations)if(profile.station==station_kind(kind))return
    name_is(mob,0x1e0,profile.on_state)&&name_is(mob,0x1b8,profile.scheme)&&
    name_is(mob,0x1a4,profile.use_with_item)&&name_is(mob,0x1cc,profile.condition_name);
  return false;
}
bool common_bindings_verified() noexcept {
  const ubc::lhiver::Profile profile{layout_profile.parser_address,layout_profile.symbol_count,
    layout_profile.code_length,layout_profile.code_sha256,nullptr,0,basecat::common_bindings.data(),basecat::common_bindings.size()};
  return rt::verify_local_contract(memory,s::game_parser,profile).matches;
}
std::uint64_t next_cool_serial() noexcept {
  if(cool_serial==UINT64_MAX){disabled=true;return 0;}return ++cool_serial;
}
void log_cool_destroy_hand_issue(cdestroy::Issue issue) noexcept {
  auto& diagnostic=cool_destroy_hand_diagnostic;
  if(!chdiag::ready(diagnostic,issue,cool_destroy_call.before_destroy.stamp))return;
  diagnostic.emitted=true;
  event("cool_destroy_hand_diag",static_cast<int>(chdiag::flags(diagnostic)),
    static_cast<int>(cool_destroy_binding.quantity));
  const auto values=chdiag::values(diagnostic);
  for(std::size_t i=0;i<values.size();++i)
    event("cool_destroy_hand_value",static_cast<int>(i+1),values[i]);
}
cdestroy::Snapshot cool_destroy_snapshot(bool initial) noexcept {
  cdestroy::Snapshot out;auto& stamp=out.stamp;const auto& binding=cool_destroy_binding;
  const auto before=context();const auto bridge=bc::diagnostics();
  stamp.job=binding.job;stamp.frame=frame;stamp.serial=next_cool_serial();stamp.actual_thread=GetCurrentThreadId();
  if(!own_thread()||cool_destroy_depth!=1||!cool_job||(!cool_active_entry.valid&&!result_guard.cool_witness().valid)||
    binding.job!=handoff.request().key.generation||result_guard.state()!=ubc::CraftingResultState::waiting||
    !before.identity_known||!before.station_known||before.world!=binding.key.world||
    before.hero!=binding.key.hero||before.station!=binding.key.station||
    info_depth||choice_depth||update_depth||station_depth||collection_depth||resolved_read_depth||
    amount_patch.active()||bridge.writer||bridge.readers||bridge.writes_disabled)return out;
  stamp.key={generation,GetCurrentThreadId(),before.world,before.hero,before.station,s::information_manager,13};
  const bool predebit=result_guard.cool_prepared().valid();
  const auto path=cool_runtime_path(predebit);stamp.native_code_verified=path.valid;
  stamp.hooks_owned=cool_destroy_installed&&cool_remove_observer.owns_patch();
  const auto inventory=ubc::read_packed_inventory(memory,before.hero,ccat::targets,true,cool_inventory);
  const bool inventory_ok=inventory.storage_totals_known&&inventory.snapshot_unchanged&&
    inventory.stable_boundary_verified&&inventory.issue==ubc::PackedInventoryIssue::none;
  const bool hands_ok=ubc::station_recipe_snapshot_detail::begin_hands(memory,before.hero,cool_hands)==ubc::StationHandReadIssue::none&&
    ubc::station_recipe_snapshot_detail::verify_hands(memory,cool_hands);
  if(hands_ok)out.hands=cool_hands.before;
  out.stocks.stable_boundary_verified=inventory_ok;
  for(std::size_t i=0;i<2;++i){
    const bool unambiguous=inventory.listed_occurrences[i]<=1&&inventory.packed_occurrences[i]<=1&&
      !inventory.duplicate_packed_target[i]&&!inventory.has_active_marker[i]&&!inventory.mixed_representation[i];
    out.stocks.stocks[i]={ccat::targets[i].symbol,inventory.total[i],inventory_ok,unambiguous};
  }
  if(initial){
    auto& evidence=cool_destroy_call;evidence.storage_alias_scan_known=inventory_ok&&hands_ok;
    evidence.generated_storage_alias=false;
    for(std::size_t i=0;inventory_ok&&i<inventory.listed_nodes;++i){
      const auto object=rt::detail::u32(cool_inventory.records[i].node.data()+4);
      if(object==out.hands.hands.left.object||object==out.hands.interact_item)evidence.generated_storage_alias=true;
    }
    const auto object=out.hands.hands.left.object;std::uint32_t type=0,mainflag=0,flags=0;
    auto& diagnostic=cool_destroy_hand_diagnostic;
    diagnostic.prepared=result_guard.cool_prepared().valid()?result_guard.cool_prepared().hands():cool_active_entry.prepared.hands;diagnostic.observed=out.hands;
    diagnostic.hands_read_verified=hands_ok;
    const auto profile=[&]() noexcept {
      diagnostic.profile_checked=true;
      return diagnostic.profile_matches=matches_station_profile(before.station,cat::Station::cool);
    };
    const auto first_shape=[&](unsigned index,std::uint32_t offset,std::uint32_t& value) noexcept {
      diagnostic.shape_attempted|=1u<<index;
      const bool known=field(object,offset,value);
      diagnostic.shape[index]=chdiag::signed_word(value);
      if(known)diagnostic.shape_known|=1u<<index;
      return known;
    };
    const auto second_shape=[&](unsigned index,std::uint32_t offset) noexcept {
      // Match integer() semantics exactly. Initialize to -1, call field() once and compare the
      // return value unchanged. Whether the read succeeded is diagnostic only and adds no new gate.
      std::int32_t value=-1;diagnostic.shape_attempted|=1u<<index;
      const bool known=field(object,offset,value);diagnostic.shape[index]=value;
      if(known)diagnostic.shape_known|=1u<<index;
      return value;
    };
    evidence.generated_left_verified=hands_ok&&cdirect::detail::active_hand(out.hands)&&
      profile()&&
      first_shape(0,0,type)&&type==ubc::hand_detail::item_vtable&&first_shape(1,0x154,mainflag)&&mainflag==1&&
      first_shape(2,0x158,flags)&&bhshape::matches(out.hands.hands.left.symbol,type,mainflag,flags)&&
      second_shape(3,0)==static_cast<std::int32_t>(type)&&second_shape(4,0x154)==1&&
      second_shape(5,0x158)==static_cast<std::int32_t>(bhshape::flags);
    diagnostic.generated_left_verified=evidence.generated_left_verified;
    // The known ZBug packing branch is never actually reached.
    if(inventory.packed_occurrences[0]||inventory.packed[0]||inventory.listed_occurrences[0]!=1)
      out.stocks.stable_boundary_verified=false;
  }
  const auto after=context();
  stamp.stable_boundary_verified=inventory_ok&&hands_ok&&after.identity_known&&after.station_known&&
    after.world==before.world&&after.hero==before.hero&&after.station==before.station&&
    cool_runtime_path(predebit).valid&&frame==binding.frame;
  if(initial){cool_destroy_hand_diagnostic.stamp=stamp;cool_destroy_hand_diagnostic.captured=true;}
  return out;
}
shdestroy::Snapshot grind_destroy_snapshot(bool initial) noexcept {
  shdestroy::Snapshot out;auto& stamp=out.stamp;const auto& binding=grind_destroy_binding;
  const auto before=context();const auto bridge=bc::diagnostics();const auto* recipe=cat::find(handoff.request().id);
  stamp.job=binding.job;stamp.frame=frame;stamp.serial=next_grind_serial();stamp.actual_thread=GetCurrentThreadId();
  if(!own_thread()||cool_destroy_depth!=1||!grind_job||!recipe||!recipe->grind||recipe->targets.count!=3||
    binding.job!=handoff.request().key.generation||result_guard.state()!=ubc::CraftingResultState::waiting||
    !before.identity_known||!before.station_known||before.world!=binding.key.world||
    before.hero!=binding.key.hero||before.station!=binding.key.station||
    info_depth||choice_depth||update_depth||station_depth||collection_depth||resolved_read_depth||
    amount_patch.active()||bridge.writer||bridge.readers||bridge.writes_disabled)return out;
  stamp.key=grind_key(before);
  stamp.native_code_verified=cool_runtime_path().valid&&shrt::verify_recipe(memory,s::game_parser,*recipe->grind).valid;
  stamp.hooks_owned=cool_destroy_installed&&cool_remove_observer.owns_patch();
  const auto inventory=ubc::read_packed_inventory<3>(memory,before.hero,recipe->targets.data,recipe->targets.count,true,cool_inventory);
  const bool inventory_ok=inventory.storage_totals_known&&inventory.snapshot_unchanged&&
    inventory.stable_boundary_verified&&inventory.issue==ubc::PackedInventoryIssue::none;
  const bool hands_ok=ubc::station_recipe_snapshot_detail::begin_hands(memory,before.hero,cool_hands)==ubc::StationHandReadIssue::none&&
    ubc::station_recipe_snapshot_detail::verify_hands(memory,cool_hands);
  if(hands_ok)out.hands=cool_hands.before;
  out.stocks.stable_boundary_verified=inventory_ok;
  for(std::size_t i=0;i<3;++i){
    const bool unambiguous=inventory.listed_occurrences[i]<=1&&inventory.packed_occurrences[i]<=1&&
      !inventory.duplicate_packed_target[i]&&!inventory.has_active_marker[i]&&!inventory.mixed_representation[i];
    out.stocks.stocks[i]={recipe->targets[i].symbol,inventory.total[i],inventory_ok,unambiguous};
  }
  if(initial){
    auto& evidence=grind_destroy_call;evidence.storage_alias_scan_known=inventory_ok&&hands_ok;
    for(std::size_t i=0;inventory_ok&&i<inventory.listed_nodes;++i){
      const auto object=rt::detail::u32(cool_inventory.records[i].node.data()+4);
      if(object==out.hands.hands.left.object||object==out.hands.interact_item)evidence.generated_storage_alias=true;
    }
    const auto* shape=ubc::grind_item_shape::find(shcat::lifecycle_item);
    const auto object=out.hands.hands.left.object;std::uint32_t type=0,mainflag=0,flags=0;
    evidence.generated_left_verified=hands_ok&&shdestroy::detail::active_grind_hand(out.hands)&&
      matches_station_profile(before.station,cat::Station::grind)&&shape&&ubc::grind_item_shape::bound_to_profile(*shape)&&
      field(object,0,type)&&type==ubc::hand_detail::item_vtable&&field(object,0x154,mainflag)&&mainflag==shape->mainflag&&
      shape->native_flags==bhshape::stored_flags&&field(object,0x158,flags)&&
      bhshape::matches(out.hands.hands.left.symbol,type,mainflag,flags)&&
      integer(object,0)==static_cast<std::int32_t>(type)&&integer(object,0x154)==static_cast<std::int32_t>(mainflag)&&
      integer(object,0x158)==static_cast<std::int32_t>(flags);
    // At the actual original debit, Cold16613 must be a single listed stack.
    // Packed or active forms do not qualify for the admitted ZBug branch.
    if(recipe->targets[2].symbol!=shcat::lifecycle_item||inventory.packed_occurrences[2]||inventory.packed[2]||
      inventory.listed_occurrences[2]!=1||inventory.has_active_marker[2]||inventory.mixed_representation[2])
      out.stocks.stable_boundary_verified=false;
  }
  const auto after=context();
  stamp.stable_boundary_verified=inventory_ok&&hands_ok&&after.identity_known&&after.station_known&&
    after.world==before.world&&after.hero==before.hero&&after.station==before.station&&
    cool_runtime_path().valid&&frame==binding.frame;
  return out;
}
std::uint64_t __cdecl cool_remove_before(void*,const rcall::Invocation& input) {
  if(grind_job){
    if(!own_thread()||cool_destroy_depth!=1||!grind_destroy_call.destroy_entries||
      grind_destroy_call.destroy_exits||result_guard.state()!=ubc::CraftingResultState::waiting)return 0;
    auto& o=grind_destroy_call;++o.remove_entries;
    if(o.remove_entries!=1){o.reentry_seen=true;return 0;}
    o.remove={input.caller,input.inventory,input.loaded_vtable,input.original,input.instance,input.amount,1,1,0,false,false};
    o.before_remove=grind_destroy_snapshot(false);
    o.set_interact_item_normal_return_verified=o.before_remove.stamp.native_code_verified&&
      o.before_remove.stamp.hooks_owned&&o.destroy.caller==cabi::destroy_return&&
      input.caller==cabi::inventory_remove_return&&input.inventory==o.destroy.npc+cabi::npc_inventory_offset;
    return o.before_remove.stamp.serial;
  }
  if(!own_thread()||cool_destroy_depth!=1||!cool_job||!cool_destroy_call.destroy_entries||
    cool_destroy_call.destroy_exits||result_guard.state()!=ubc::CraftingResultState::waiting)return 0;
  auto& o=cool_destroy_call;++o.remove_entries;
  if(o.remove_entries!=1){o.reentry_seen=true;return 0;}
  o.remove={input.caller,input.inventory,input.loaded_vtable,input.original,input.instance,input.amount,1,1,0,false,false};
  o.before_remove=cool_destroy_snapshot(false);
  o.set_interact_item_normal_return_verified=o.before_remove.stamp.native_code_verified&&
    o.before_remove.stamp.hooks_owned&&o.destroy.caller==cabi::destroy_return&&
    input.caller==cabi::inventory_remove_return&&input.inventory==o.destroy.npc+cabi::npc_inventory_offset;
  return o.before_remove.stamp.serial;
}
void __cdecl cool_remove_after(void*,const rcall::Invocation& input,std::uint64_t ticket,std::uint32_t returned) {
  if(grind_job){
    auto& o=grind_destroy_call;
    if(!own_thread()||cool_destroy_depth!=1||!ticket||ticket!=o.before_remove.stamp.serial||
      o.remove.caller!=input.caller||o.remove.inventory!=input.inventory||o.remove.target!=input.original||
      o.remove.loaded_vtable!=input.loaded_vtable||o.remove.instance!=input.instance||o.remove.amount!=input.amount){
      o.reentry_seen=true;return;
    }
    ++o.remove_exits;o.remove.normal_return=true;o.remove.opaque_return=returned;
    o.after_remove=grind_destroy_snapshot(false);return;
  }
  auto& o=cool_destroy_call;
  if(!own_thread()||cool_destroy_depth!=1||!ticket||ticket!=o.before_remove.stamp.serial||
    o.remove.caller!=input.caller||o.remove.inventory!=input.inventory||o.remove.target!=input.original||
    o.remove.loaded_vtable!=input.loaded_vtable||o.remove.instance!=input.instance||o.remove.amount!=input.amount){
    o.reentry_seen=true;return;
  }
  ++o.remove_exits;o.remove.normal_return=true;o.remove.opaque_return=returned;
  o.after_remove=cool_destroy_snapshot(false);
}
int __fastcall observe_destroy(void* npc,void*,void* message) {
  const DWORD incoming=GetLastError();const auto caller=reinterpret_cast<std::uint32_t>(_ReturnAddress());
  cool_predebit_journal.entered();
  const bool own=own_thread();if(own)++cool_destroy_depth;
  const auto& request=handoff.request();
  const bool candidate=own&&!cool_writer_scope&&(cool_job||grind_job)&&result_guard.state()==ubc::CraftingResultState::waiting&&
    reinterpret_cast<std::uint32_t>(npc)==request.key.hero;
  auto& destroy=grind_job?grind_destroy_call.destroy:cool_destroy_call.destroy;
  auto& entries=grind_job?grind_destroy_call.destroy_entries:cool_destroy_call.destroy_entries;
  auto& reentry=grind_job?grind_destroy_call.reentry_seen:cool_destroy_call.reentry_seen;
  if(own&&cool_destroy_depth>1)reentry=true;
  if(candidate){
    ++entries;
    if(entries==1){
      if(grind_job)grind_destroy_binding={grind_key(context()),request.key.generation,frame,grind_job_floor,
        grind_prepared.before.callback,request.quantity};
      else cool_destroy_binding={result_guard.cool_prepared().valid()?result_guard.cool_prepared().key():cool_active_entry.prepared.key,request.key.generation,frame,cool_job_floor,request.id,request.quantity};
      destroy.caller=caller;destroy.npc=reinterpret_cast<std::uint32_t>(npc);
      destroy.message=reinterpret_cast<std::uint32_t>(message);destroy.depth=cool_destroy_depth;
      const ddispatch::Invocation invocation{destroy.npc,destroy.message,caller,cool_destroy_depth};
      const auto dispatch=destroy_verified_path(cool_owned_patches(),&invocation,cool_job&&result_guard.cool_prepared().valid());
      destroy.message_class_verified=dispatch.valid;
      if(dispatch.valid){destroy.classdef=cabi::manipulate_classdef;destroy.subtype=dispatch.dispatch.values.message_subtype;}
      else event("destroy_dispatch_rejected",static_cast<int>(dispatch.dispatch.issue),static_cast<int>(dispatch.address));
      if(grind_job)grind_destroy_call.before_destroy=grind_destroy_snapshot(true);
      else cool_destroy_call.before_destroy=cool_destroy_snapshot(true);
    }else reentry=true;
    ++destroy.original_calls;
  }
  int result=0;DWORD error=incoming;
  SetLastError(incoming);
  __try {
    result=original_destroy(npc,message);error=GetLastError();
    if(candidate){
      destroy.normal_return=true;destroy.return_value=result;cdestroy::Result proof;
      if(grind_job){++grind_destroy_call.destroy_exits;grind_destroy_call.after_destroy=grind_destroy_snapshot(false);
        ubc::GrindCheck check;
        proof=shdestroy::verify(grind_prepared,grind_destroy_binding,grind_destroy_call,check);
        if(!proof.valid)log_grind_check(check,static_cast<int>(request.id));
      }else{++cool_destroy_call.destroy_exits;cool_destroy_call.after_destroy=cool_destroy_snapshot(false);
        proof=result_guard.cool_prepared().valid()?
          cdestroy::verify(result_guard.cool_prepared(),result_guard.cool_witness(),cool_destroy_binding,cool_destroy_call):
          cdestroy::verify(cool_active_entry.prepared,cool_destroy_binding,cool_destroy_call);}
      if(proof.valid){
        if(result_guard.destroy_after({true,true,request.key,request.id,request.quantity,proof.serial,proof.frame,proof.instance,static_cast<std::uint32_t>(proof.amount)}))
          event(grind_job?"grind_destroy_verified":"cool_destroy_verified",proof.instance,proof.amount);
        else{
          if(grind_job){
            ubc::GrindCheck check;
            check.equal("destroy.result_guard",true,false);
            log_grind_check(check,static_cast<int>(request.id));
          }
          result_guard.fail(ubc::CraftingResultIssue::destroy);event(grind_job?"grind_destroy_rejected":"cool_destroy_rejected",-1);
        }
      }else{
        result_guard.fail(ubc::CraftingResultIssue::destroy);event(grind_job?"grind_destroy_rejected":"cool_destroy_rejected",static_cast<int>(proof.issue));
        if(!grind_job)log_cool_destroy_hand_issue(proof.issue);
      }
    }
  } __finally {
    if(AbnormalTermination()){
      error=GetLastError();
      if(candidate){destroy.exception=true;result_guard.fail(ubc::CraftingResultIssue::destroy);}
    }
    if(own)--cool_destroy_depth;
    cool_predebit_journal.exited();
    SetLastError(error);
  }
  return result;
}
ubc::StationEndPoint end_point_once(const ubc::CraftingKey& k,bool before) noexcept {
  ubc::StationEndPoint out;out.thread=GetCurrentThreadId();std::uint32_t game=0;
  if(!common_bindings_verified())return out;
  if(!read(a::game_pointer,game)||!game||integer(game,0)!=static_cast<std::int32_t>(a::game_vtable)||
    !field(game,a::game_world,out.world)||!read(s::hero_pointer,out.hero)||!out.world||!out.hero)return out;
  const auto hero=rt::read_symbol_address(memory,s::game_parser,ubc::lhiver::hero_symbol);std::uint32_t bound=0;
  if(!hero.valid||!field(hero.address,0x1c,bound)||bound!=out.hero)return out;
  out.identity_known=true;out.station_known=field(out.hero,s::npc_interact_mob,out.station);
  const auto production=rt::symbol_int(memory,s::game_parser,ubc::lhiver::production_symbol);
  out.production_known=production.valid;out.production=production.value;
  out.hands_known=ubc::station_recipe_snapshot_detail::begin_hands(memory,out.hero,end_hands)==ubc::StationHandReadIssue::none&&
    ubc::station_recipe_snapshot_detail::verify_hands(memory,end_hands);
  if(out.hands_known)out.hands=end_hands.before;
  if(before&&out.station_known&&out.station==k.station){
    out.kind=k.kind;out.station_owned=station_owns(out.station,out.hero);
    out.station_profile_verified=matches_station_profile(out.station,k.kind);
    out.mob_fields_known=field(out.station,0x1f4,out.state)&&field(out.station,0x1f8,out.state_num)&&
      field(out.station,0x1fc,out.target)&&field(out.station,0x200,out.rewind);
  }
  return out;
}
bool same_end_point(const ubc::StationEndPoint& a,const ubc::StationEndPoint& b) noexcept {
  return a.identity_known&&b.identity_known&&a.station_known&&b.station_known&&
    a.production_known&&b.production_known&&a.hands_known&&b.hands_known&&
    a.thread==b.thread&&a.world==b.world&&a.hero==b.hero&&a.station==b.station&&
    a.station_owned==b.station_owned&&a.station_profile_verified==b.station_profile_verified&&
    a.mob_fields_known==b.mob_fields_known&&a.kind==b.kind&&a.state==b.state&&a.target==b.target&&
    a.state_num==b.state_num&&a.rewind==b.rewind&&a.production==b.production&&ubc::same_crafting_hands(a.hands,b.hands);
}
ubc::StationEndPoint end_point(const ubc::CraftingKey& k,bool before) noexcept {
  auto out=end_point_once(k,before);const auto second=end_point_once(k,before);
  out.read_stable=same_end_point(out,second);return out;
}
void __fastcall observe_end(void* mob,void*,void* npc,int flag){
  const auto error=GetLastError();const auto caller=reinterpret_cast<std::uint32_t>(_ReturnAddress());
  const bool own=own_thread();if(own)++station_depth;
  const auto& request=handoff.request();
  const bool candidate=own&&result_guard.state()==ubc::CraftingResultState::waiting&&
    reinterpret_cast<std::uint32_t>(mob)==request.key.station&&reinterpret_cast<std::uint32_t>(npc)==request.key.hero&&
    flag==0&&caller==ubc::station_end_contract::completed_return;
  ubc::StationEndCall call;std::uint32_t table=0;
  const ubc::station_end_contract::InstalledEndHook hook{InterlockedCompareExchange(&installed,0,0)!=0,
    reinterpret_cast<std::uint32_t>(&observe_end),reinterpret_cast<std::uint32_t>(hooks.original(8))};
  if(candidate){
    call.mob=reinterpret_cast<std::uint32_t>(mob);call.npc=reinterpret_cast<std::uint32_t>(npc);
    call.return_address=caller;call.flag=flag;
    call.owner_thread_depth_verified=station_depth==1&&!info_depth&&!choice_depth&&!update_depth&&!collection_depth&&!amount_patch.active();
    call.before=end_point(request.key,true);
    call.original_path_verified=read(call.mob,table)&&ubc::station_end_contract::verify_path(memory,table,hook,end_compatibility(cool_job&&result_guard.cool_prepared().valid())).valid;
  }
  SetLastError(error);original_end(mob,npc,flag);const auto after=GetLastError();
  if(own){
    if(end_sequence!=UINT64_MAX)++end_sequence;else disabled=true;
    if(candidate){
      call.original_returned=true;call.after=end_point(request.key,false);
      call.original_path_verified=call.original_path_verified&&ubc::station_end_contract::verify_path(memory,table,hook,end_compatibility(cool_job&&result_guard.cool_prepared().valid())).valid;
      const auto verified=ubc::verify_station_end(request.key,call);
      if(verified.normal_exit_verified){
        if(extra_job&&extra_executor&&extra_active_job.id==request.key.generation&&
          extra_active_job.context.world==request.key.world&&extra_active_job.context.hero==request.key.hero&&
          extra_active_job.context.mob==request.key.station){
          extra_end={true,extra_active_job.id,extra_active_job.writer_token,next_extra_serial(),extra_active_job.context};
          extra_normal_end=true;extra_end_frame=frame;
        }
        if(spirit_job&&spirit_executor&&spirit_active_job.id==request.key.generation&&
          spirit_active_job.context.world==request.key.world&&spirit_active_job.context.hero==request.key.hero&&
          spirit_active_job.context.mob==request.key.station){
          spirit_end={true,spirit_active_job.id,spirit_active_job.writer_token,next_extra_serial(),spirit_active_job.context};
          spirit_normal_end=true;spirit_end_frame=frame;
        }
        result_guard.end_after({true,true,request.key.generation,end_sequence,frame,
          request.key.world,request.key.hero,request.key.station});event("normal_end_verified");
      }else{
        if(grind_job)log_grind_check(ubc::diagnose_grind_end(verified.issue,request.key,call),static_cast<int>(request.id));
        event("normal_end_rejected",static_cast<int>(verified.issue));
      }
    }--station_depth;
  }SetLastError(after);
}
int __fastcall observe_menu_event(void* menu,void*,int input){
  const auto error=GetLastError();
  if(own_thread()&&ui.opened()&&menu==ui.object()){
    // Independently of the bounded diagnostics, every entry into our callback
    // revokes an active scope, including unknown, no-op or rejected input.
    bool counted=false;int result=1;
    if(grind_preview_input_serial==UINT64_MAX||grind_preview_input_depth==UINT_MAX)grind_preview_overflow=true;
    else{++grind_preview_input_serial;++grind_preview_input_depth;counted=true;}
    __try{
      uidiag::Action diagnostic;result=ui.handle(session,input,&diagnostic);
      ui_diagnostic_events(ui_diagnostic.input(session.preview().key.generation,diagnostic));
      if(grind_ui_trace_active&&diagnostic.observed)
        grind_ui_readiness.observe_input(session.preview().key.generation,frame,diagnostic);
    }__finally{if(counted)--grind_preview_input_depth;SetLastError(error);}
    return result;
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
      handoff.cancel(ubc::CraftingHandoffIssue::offer);SetLastError(error);return 1;
    }
    auto actual=observation(c,false,false);
    if(dispatched_ticket.view&&reinterpret_cast<std::uint32_t>(callback)==dispatched_ticket.view+a::view_callback)
      actual.view=dispatched_ticket.view;
    if(!foreign_dialog(c)&&reinterpret_cast<std::uint32_t>(callback)==dispatched_ticket.view+a::view_callback&&
      handoff.guards_input(actual)&&!(tobacco_owner.ticket&&handoff.cancelled())){
      if(result_guard.state()!=ubc::CraftingResultState::failed){
        if(input==m::key_escape)handoff.cancel();
        SetLastError(error);return 1;
      }
    }
  }
  SetLastError(error);
  return original_choice_event(callback,input);
}
bool configure_batch_symbols(bc::Config& config) noexcept {
  if(config.symbol_count)return false;
  for(const auto& recipe:basecat::recipes)for(const auto& callback:recipe.callbacks)if(callback.minimum_quantity){
    if(config.symbol_count==config.symbols.size()){return false;}
    config.symbols[config.symbol_count++]=static_cast<std::int32_t>(callback.function->symbol);
  }
  for(const auto& recipe:scat::recipes){
    if(config.symbol_count==config.symbols.size()){return false;}
    config.symbols[config.symbol_count++]=static_cast<std::int32_t>(recipe.callback.function->symbol);
  }
  const auto add=[&](const ubc::lhiver::Function* f) noexcept {
    if(!f||config.symbol_count==config.symbols.size())return false;
    config.symbols[config.symbol_count++]=static_cast<std::int32_t>(f->symbol);return true;
  };
  if(!add(tobacco::owner.information))return false;
  for(const auto& recipe:tobacco::recipes)if(!add(recipe.callback.function))return false;
  for(const auto& recipe:xcat::recipes)if(!add(recipe.callback.function))return false;
  for(const auto& recipe:acat::recipes)if(!add(recipe.callback.function)||!add(recipe.original_three.function))return false;
  for(const auto& recipe:ocat::recipes)if(!add(recipe.callback.function))return false;
  for(const auto& recipe:gcat::recipes)if(!add(recipe.callback))return false;
  for(const auto& recipe:bcat::recipes)if(!add(recipe.callback.function))return false;
  if(!add(ccat::single.information)||!add(ccat::triple.information))return false;
  for(const auto& recipe:shcat::recipes)if(!add(&recipe.function))return false;
  for(std::size_t i=0;i<config.symbol_count;++i)for(std::size_t j=0;j<i;++j)
    if(config.symbols[i]==config.symbols[j])return false;
  return true;
}
void install() noexcept {
  wchar_t path[32768]{};if(!w::sibling_path(module,path,L"UnionBatchCraftingExtended.log"))return;
#ifdef UBC_PLAYER_RELEASE
  constexpr DWORD disposition = CREATE_ALWAYS;
#else
  constexpr DWORD disposition = CREATE_NEW;
#endif
  log_file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,disposition,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(log_file==INVALID_HANDLE_VALUE)return;
  const char* header=execution_enabled?"crafting_version=1\r\nmode=recipe_amounts\r\n":"crafting_version=1\r\nmode=ui_only\r\n";
  DWORD written=0;WriteFile(log_file,header,static_cast<DWORD>(std::strlen(header)),&written,nullptr);log_bytes=written;
#ifdef UBC_PLAYER_RELEASE
  if (!w::player_image_header()) { event("engine_header_rejected"); return; }
  player_image = ubc::player_engine::inspect_executable();
  for (unsigned i = 0; i < player_image.failure.size(); ++i)
    if (player_image.failure[i]) event("engine_station_disabled", i, player_image.failure[i]);
  if (!player_image.any()) return;
#endif
  std::array<std::uint8_t,a::frame_prefix_bytes.size()> prefix{};
  if(!memory.copy(a::frame_prefix,prefix.data(),prefix.size())||prefix!=a::frame_prefix_bytes){
    event("frame_prefix_rejected");return;
  }
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
  config.reject_unexpected=reject_unexpected_cool_info;
  if(!configure_batch_symbols(config)){event("hooks_rejected");return;}
  if(!bc::configure(config)||!ammo::configure({nullptr,read_ammo_context,read_ammo_scope})){event("hooks_rejected");return;}
  if(hooks.install(reinterpret_cast<std::uintptr_t>(&UbcFrameBridge))){InterlockedExchange(&installed,1);event("hooks_installed");}
  else event("hooks_rejected");
}
}
extern "C" unsigned __cdecl UbcSelectionProbeVersion(){return 4;}
extern "C" int __cdecl UbcSelectionProbeHooksInstalled(){return static_cast<int>(InterlockedCompareExchange(&installed,0,0));}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){
  if(reason==DLL_PROCESS_ATTACH){
    module=instance;const auto error=GetLastError();
#ifdef UBC_PLAYER_RELEASE
    wchar_t ini[32768]{};
    if (w::sibling_path(nullptr, ini, L"Gothic.INI") &&
        GetPrivateProfileIntW(L"UNIONBATCHCRAFTING", L"Enabled", 1, ini) != 0 &&
        ubc::player_engine::game_process()) {
      execution_enabled = true;
      install();
    }
#else
    if(w::target_matches()){
      // The external cooling measurement reads only the bound
      // process. No hooks, UI, writers or engine calls in this mode.
      if(w::consume_marker(instance,L"UnionBatchCraftingExtended.enable-once","EXTENDED_COOL_MEASURE_V1\n")){
        SetLastError(error);return TRUE;
      }
      grind_measurement_mode=w::consume_marker(instance,L"UnionBatchCraftingExtended.enable-once","EXTENDED_GRIND_MEASURE_V1\n");
      campfire_observation_mode=w::consume_marker(instance,L"UnionBatchCraftingExtended.enable-once","EXTENDED_CAMPFIRE_V1\n");
      bow_diagnosis=w::consume_marker(instance,L"UnionBatchCraftingExtended.enable-once","EXTENDED_BOW_DIAG_V1\n");
      bow_comparison=w::consume_marker(instance,L"UnionBatchCraftingExtended.enable-once","EXTENDED_BOW_COMPARE_V1\n");
      const bool preview=!campfire_observation_mode&&!grind_measurement_mode&&w::consume_marker(instance,L"UnionBatchCraftingExtended.enable-once","EXTENDED_UI_V1\n");
      cool_context_mode=!campfire_observation_mode&&w::consume_marker(instance,L"UnionBatchCraftingExtended.enable-once","EXTENDED_COOL_CONTEXT_V1\n");
      execution_enabled=bow_comparison||cool_context_mode||(!campfire_observation_mode&&!grind_measurement_mode&&!preview&&w::consume_marker(instance,L"UnionBatchCraftingExtended.enable-once","EXTENDED_AMOUNTS_V1\n"));
      if(bow_diagnosis||campfire_observation_mode||grind_measurement_mode||preview||execution_enabled)install();
    }
#endif
    SetLastError(error);
  }else if(reason==DLL_PROCESS_DETACH&&log_file!=INVALID_HANDLE_VALUE){CloseHandle(log_file);log_file=INVALID_HANDLE_VALUE;}
  return TRUE;
}
