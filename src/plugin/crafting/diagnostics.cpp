// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
void event(const char* name,int detail,int quantity,const char* fields) noexcept {
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
void log_bow_check(const char* field, int expected, int actual, int subject, int quantity,
    std::uint32_t address) noexcept {
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
void log_tobacco_check(const ubc::GrindCheck& check,int subject) noexcept {
  if(!check.field)return;
  static ubc::GrindCheck last[3]{};
  const auto index=subject>=0xb001&&subject<=0xb003?subject-0xb001:0;
  if(last[index].same(check))return;
  last[index]=check;char fields[176]{};check.format(fields);
  event("tobacco_check_rejected",subject,check.symbol,fields);
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
void log_plugin_version(const char* name,const ubc::windows_probe::LoadedModuleIdentity& module) noexcept {
  if(!module.observed())return;
  const auto digest=module.digest();char hex[65]{};
  constexpr char digits[]="0123456789abcdef";
  for(std::size_t i=0;i<digest.size();++i){hex[2*i]=digits[digest[i]>>4];hex[2*i+1]=digits[digest[i]&15];}
  char fields[125]{};
  std::snprintf(fields,sizeof(fields),",\"sha256\":\"%s\",\"status\":\"%s\"",hex,module.current()?"getestet":"ungetestete Version");
  event(name,0,0,fields);
}
void flush_amount_diagnostic(std::uint64_t info_token) noexcept {
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
} // namespace ubc_native
