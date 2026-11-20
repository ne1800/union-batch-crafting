// SPDX-License-Identifier: MIT
#include "alchemy_flask_debit_bridge.hpp"
#include <intrin.h>
#include <cstdio>
#include <cstring>

namespace b=ubc::alchemy_flask_debit_bridge;
namespace d=ubc::alchemy_flask_debit;
namespace {
unsigned checks=0,failures=0;
void check(bool result,const char* label){++checks;if(!result){++failures;std::fprintf(stderr,"FAIL: %s\n",label);}}
constexpr DWORD input_error=0x10203040,output_error=0x50607080,host_exception=0xe042b411,restore_exception=0xe042b412;
enum class Mode { normal,whole,n1,read_before_fault,read_after_fault,read_restore_fault,
  restore_pop_fault,restore_instance_fault,native_before_fault,native_after_fault,
  partial,after_context,after_parser,restore_context,restore_race,reentry,observer_reentry,
  item_read_before_fault,item_read_after_fault,item_restore_fault,item_target_changed,item_table_changed,
  item_post_wrong,item_scope_changed,item_already_correct,item_observer_reentry,native_and_item_fault,
  prefix_changed,constructor_stack_fault,foreign };
struct Host {
  Mode mode=Mode::normal;
  d::Tracker tracker;
  b::Executor* executor=nullptr;
  d::Job job{};
  d::Observation base{},post{};
  d::ParserState original_parser{};
  unsigned removed=0,pop_restores=0,instance_restores=0,reads_before=0,reads_after=0,restore_reads=0;
  std::uint32_t native_hero=0,native_symbol=0,native_quantity=0,restore_stack=0,restore_symbol=0;
  bool world_changed=false;
  ubc::alchemy_extra_dependencies::ItemBinding item{};
  unsigned item_reads=0,item_restores=0;
  explicit Host(Mode m):mode(m){
    const std::uint32_t n=m==Mode::n1?1u:(m==Mode::whole?31u:7u);
    job={{1,GetCurrentThreadId(),ubc::lhiver::game_parser,0x1000,0x2000,0x3000,0x4000},2,3,20395,20393,n};
    base.context=job.context;base.scope={2,3,20395,1,0,0,true};base.serial=10;
    base.native_contract=true;base.stable_read=true;base.production=4;base.invincible=1;
    base.hand={true,true,0x5000,16604,1,0x5000};base.flask={true,true,true,0,1,0x6000,30,0x00200001};
    base.parser={true,true,ubc::lhiver::game_parser,0x1000000,3000000,400,0,
      0x7000,0x3000,0x7000,0x7100,0x3000,0x3000,0x8000,0x9000};
    item={true,ubc::lhiver::game_parser,0x7200,0x10204080,0x7300,0x1000000,3000000,ubc::lhiver::symbol_count};
    base.stocks.complete_unambiguous=true;base.stocks.count=3;
    base.stocks.rows[0]={16706,100,true};base.stocks.rows[1]={16620,0,true};base.stocks.rows[2]={16604,30,true};
    check(tracker.arm(job,{true,true,true,true,true,true},base),"host arms only reviewed linear recipe");
    post=base;post.serial=11;post.production=0;post.invincible=0;
    post.parser.data_depth=2;post.parser.data_words_known=true;post.parser.data_words={1,0x40};original_parser=post.parser;
    post.stocks.rows[0].stored-=n;post.stocks.rows[1].stored+=n;
    check(tracker.original_returned(post,true,true),"synthetic original success precedes native bridge");
  }
};
Host* active=nullptr;
void clobber_observer(){
  SetLastError(9999);unsigned csr=0x5f80;
  __asm { fninit }
  __asm { fldz }
  __asm { ldmxcsr csr }
  __asm { pxor xmm0,xmm0 }
}
bool __cdecl read(void* context,b::ReadStage stage,d::Observation& result){
  auto& h=*static_cast<Host*>(context);clobber_observer();
  check(h.executor->diagnostics().active && h.executor->diagnostics().native_scope==(stage==b::ReadStage::after_debit),
    "reader uses actual native lease depth, not callback phase as proof");
  if(stage==b::ReadStage::before_debit){
    ++h.reads_before;
    if(h.mode==Mode::read_before_fault)RaiseException(host_exception,0,0,nullptr);
    if(h.mode==Mode::observer_reentry)(void)h.executor->run();
    result=h.post;result.serial=12;return true;
  }
  ++h.reads_after;
  if(h.mode==Mode::read_after_fault)RaiseException(host_exception,0,0,nullptr);
  result=h.post;result.serial=13;result.scope.debit_depth=1;
  if(h.mode==Mode::after_context)result.context.mob++;
  if(h.mode==Mode::after_parser)result.parser.data_depth=1;
  return true;
}
bool __cdecl read_restore(void* context,const d::Call& call,d::RestoreObservation& out){
  auto& h=*static_cast<Host*>(context);++h.restore_reads;clobber_observer();
  check(h.executor->diagnostics().native_scope,"restore reader lies in admitted native lease");
  if(h.mode==Mode::read_restore_fault && h.removed)RaiseException(host_exception,0,0,nullptr);
  const auto& p=call.parser;
  out={call.context,{call.job,call.writer_token,call.callback,1,0,1,true},true,true,
    call.context.parser,p.code_base,p.code_size,p.use_symbol,p.use_address,
    p.self_symbol,p.hero_symbol,p.self,p.hero,p.other,p.victim};
  if((h.mode==Mode::restore_context && h.removed)||h.world_changed)out.context.world++;
  return d::same_data_stack(call.parser,h.post.parser);
}
bool __cdecl read_item(void* context,const d::Call& call,b::ItemObservation& out) {
  auto& h=*static_cast<Host*>(context);++h.item_reads;clobber_observer();
  check(h.executor->diagnostics().native_scope,"ITEM reader lies in admitted native lease");
  if((h.mode==Mode::item_read_before_fault && !h.removed) || (h.mode==Mode::item_read_after_fault && h.removed))
    RaiseException(host_exception,0,0,nullptr);
  if(h.mode==Mode::item_observer_reentry)(void)h.executor->run();
  out={h.item,call.context,{call.job,call.writer_token,call.callback,1,0,1,true},true};
  if(h.removed) {
    if(h.mode==Mode::item_target_changed)out.item.symbol+=4;
    if(h.mode==Mode::item_table_changed)out.item.table+=4;
    if(h.mode==Mode::item_scope_changed)out.scope.writer_token++;
    if(h.mode==Mode::item_post_wrong && h.item_restores)out.item.value++;
  }
  return true;
}
void* __thiscall remove_native(void* hero,int instance,int quantity){
  auto& h=*active;++h.removed;
  h.native_hero=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(hero));
  h.native_symbol=static_cast<std::uint32_t>(instance);h.native_quantity=static_cast<std::uint32_t>(quantity);
  check(GetLastError()==input_error,"native call receives caller LastError despite observers");
  SetLastError(output_error);
  if(h.mode==Mode::native_before_fault)RaiseException(host_exception,0,0,nullptr);
  const auto actual=static_cast<std::uint32_t>(quantity)-(h.mode==Mode::partial?1u:0u);
  h.post.stocks.rows[2].stored-=actual;h.post.flask.amount-=actual;
  if(!h.post.flask.amount){h.post.flask.object=0;h.post.flask.listed_stacks=0;}
  // Our simulation of the verified split constructor. No engine bytes.
  h.post.parser.pop_pos=900;h.post.parser.use_symbol=0xDEAD0000;h.post.parser.use_address=0xDEAD0004;
  if(h.mode==Mode::prefix_changed)h.post.parser.data_words[1]=0;
  if(h.mode==Mode::constructor_stack_fault)h.post.parser.data_depth+=4;
  if(h.mode!=Mode::item_already_correct)h.item.value=0xDEAD0008;
  unsigned csr=0x3f80;__asm { ldmxcsr csr }
  if(h.mode==Mode::native_after_fault || h.mode==Mode::native_and_item_fault || h.mode==Mode::constructor_stack_fault)RaiseException(host_exception,0,0,nullptr);
  if(h.mode==Mode::reentry)(void)h.executor->run();
  // Deliberately non-dereferenceable return value. It proves nothing.
  return reinterpret_cast<void*>(0xDEADC0DE);
}
void __thiscall restore_pop(void* stack,int pos){
  auto& h=*active;++h.pop_restores;h.restore_stack=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(stack));
  SetLastError(77);
  if(h.mode==Mode::restore_pop_fault)RaiseException(host_exception,0,0,nullptr);
  h.post.parser.pop_pos=static_cast<std::uint32_t>(pos);
  if(h.mode==Mode::restore_race)h.world_changed=true;
}
void __cdecl restore_instance(void* symbol){
  auto& h=*active;++h.instance_restores;h.restore_symbol=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(symbol));
  SetLastError(88);
  if(h.mode==Mode::restore_instance_fault)RaiseException(host_exception,0,0,nullptr);
  h.post.parser.use_symbol=h.restore_symbol;h.post.parser.use_address=h.job.context.hero;
}
void __thiscall restore_item(void* symbol,int value) {
  auto& h=*active;++h.item_restores;
  check(reinterpret_cast<std::uintptr_t>(symbol)==0x7200 && static_cast<std::uint32_t>(value)==0x10204080,
    "ITEM thiscall restores exact saved value to freshly bound symbol");
  clobber_observer();
  if(h.mode==Mode::item_restore_fault)RaiseException(host_exception,0,0,nullptr);
  if(h.mode==Mode::native_and_item_fault)RaiseException(restore_exception,0,0,nullptr);
  h.item.value=static_cast<std::uint32_t>(value);
}
DWORD WINAPI foreign(void* value){
  auto* executor=static_cast<b::Executor*>(value);
  return executor->run()==b::Outcome::rejected?0u:1u;
}
bool invoke(b::Executor& executor,b::Outcome& outcome,DWORD& code){
  bool normal=false;
  __try {outcome=executor.run();normal=true;}
  __except(GetExceptionCode()==host_exception || GetExceptionCode()==restore_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){code=GetExceptionCode();}
  return normal;
}
void one(Mode mode){
  Host h(mode);active=&h;
  b::Config config{GetCurrentThreadId(),&h,remove_native,restore_pop,restore_instance,read,read_restore,restore_item,read_item};
  b::Executor executor(h.tracker,config);h.executor=&executor;
  check(executor.diagnostics().ready,"explicit synthetic native configuration accepted");
  if(mode==Mode::foreign){
    HANDLE thread=CreateThread(nullptr,0,foreign,&executor,0,nullptr);
    check(thread!=nullptr,"own test thread created");
    if(thread){check(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"own test thread returns without native action");
      DWORD code=1;check(GetExitCodeThread(thread,&code)&&code==0,"foreign thread rejected");CloseHandle(thread);}
  }
  alignas(16) unsigned char fx_before[512],fx_after[512];_fxsave(fx_before);
  unsigned seh_before=0,seh_after=0,esp_before=0,esp_after=0;
  __asm { mov eax,fs:[0] }
  __asm { mov seh_before,eax }
  __asm { mov esp_before,esp }
  SetLastError(input_error);DWORD code=0;b::Outcome outcome=b::Outcome::rejected;
  const bool normal=invoke(executor,outcome,code);
  const DWORD error=GetLastError();_fxsave(fx_after);_fxrstor(fx_before);
  __asm { mov eax,fs:[0] }
  __asm { mov seh_after,eax }
  __asm { mov esp_after,esp }
  check(seh_before==seh_after&&esp_before==esp_after,"typed native calls preserve caller stack and SEH chain");
  const bool native_exception=mode==Mode::native_before_fault||mode==Mode::native_after_fault||mode==Mode::native_and_item_fault||mode==Mode::constructor_stack_fault;
  check(normal!=native_exception,"only original native exceptions propagate");
  if(native_exception)check(code==host_exception,"original exception identity is preserved after finally");
  if(mode==Mode::native_and_item_fault)check(h.item_restores==1,"failing ITEM restore was attempted but cannot replace original exception");
  const bool no_call=mode==Mode::n1||mode==Mode::read_before_fault||mode==Mode::observer_reentry||mode==Mode::foreign||
    mode==Mode::item_read_before_fault||mode==Mode::item_observer_reentry;
  check(h.removed==(no_call?0u:1u),"at most one native debit, none before positive gate");
  check(error==(no_call?input_error:output_error),"finally and read callbacks preserve authoritative LastError");
  check(!executor.diagnostics().active && !executor.diagnostics().native_scope,"active bridge and native leases always released");
  if(!no_call){
    check(h.native_hero==h.job.context.hero&&h.native_symbol==16604&&h.native_quantity==h.job.quantity-1,
      "thiscall receiver and stack arguments are exact bound flask debit");
    const bool no_restore=mode==Mode::read_restore_fault||mode==Mode::restore_context||mode==Mode::prefix_changed||mode==Mode::constructor_stack_fault;
    check(h.pop_restores==(no_restore?0u:1u),"PopPos restored only after positive target revalidation");
    check(h.instance_restores==((no_restore||mode==Mode::restore_race)?0u:1u),
      "UseInstance restored only after second target revalidation");
    if(h.pop_restores)check(h.restore_stack==ubc::lhiver::game_parser+0x48,"SetPopPos uses bytecodestack object");
    if(h.instance_restores)check(h.restore_symbol==h.base.parser.use_symbol,"SetUseInstance receives symbol not hero");
  }
  const bool successful=mode==Mode::normal||mode==Mode::whole||mode==Mode::n1||mode==Mode::item_already_correct;
  if(successful){
    check(outcome==(mode==Mode::n1?b::Outcome::no_extra_debit:b::Outcome::verified),"positive numeric comparison is only success route");
    check(!executor.diagnostics().disabled&&!h.tracker.status().blocked,"success leaves healthy bound job");
    if(mode!=Mode::n1){
      check(h.post.stocks.rows[2].stored==31-h.job.quantity,"post-debit storage is exact N-minus-one");
      const auto mxcsr=*reinterpret_cast<const std::uint32_t*>(fx_after+24);
      check(mxcsr==0x3f80,"own observers/restoration preserve native postcall FPU environment");
      check(d::same(h.original_parser,h.post.parser),"constructor cursor and instance restored with exact original return prefix");
      check(h.item.value==0x10204080,"ITEM postread equals saved value");
      check(h.item_restores==(mode==Mode::item_already_correct?0u:1u),"already correct ITEM needs no native write");
    }
  }else{
    check(executor.diagnostics().disabled&&h.tracker.status().blocked,"partial unknown foreign or reentrant work permanently blocks");
    check(outcome==b::Outcome::rejected,"failure never manufactures a verified receipt");
    const auto removed=h.removed;(void)executor.run();
    check(h.removed==removed,"no second native effect after ambiguous first result");
  }
  if(mode==Mode::item_target_changed||mode==Mode::item_table_changed||mode==Mode::item_scope_changed||
     mode==Mode::item_read_after_fault||mode==Mode::item_read_before_fault||mode==Mode::n1)
    check(h.item_restores==0,"unknown or foreign ITEM target is never written");
  if(mode==Mode::prefix_changed||mode==Mode::constructor_stack_fault){
    check(!h.item_restores,"corrupt or temporary constructor prefix forbids ITEM restore too");
    check(h.post.parser.data_depth==(mode==Mode::constructor_stack_fault?6u:2u),"no invented DataStack rollback after exception or changed prefix");
  }
  active=nullptr;
}
}
int main(){
  for(unsigned i=0;i<=static_cast<unsigned>(Mode::foreign);++i)one(static_cast<Mode>(i));
  Host invalid(Mode::normal);b::Executor missing(invalid.tracker,{});
  check(!missing.diagnostics().ready&&missing.run()==b::Outcome::rejected,"incomplete config has no native operation");
  std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
