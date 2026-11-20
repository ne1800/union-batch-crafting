// SPDX-License-Identifier: MIT
#if !defined(UBC_SPIRIT_HOST_TEST)
#error Dieser synthetische Writerhost benoetigt den expliziten Testbuild, keinen Release-Override.
#endif
#define UBC_SPIRIT_EXECUTION_TEST_EMBEDDED
#include "spirit_execution_tests.cpp"
#include "writers/spirit_writer.hpp"
#include <intrin.h>
#include <memory>

namespace writer_test {
namespace writer=ubc::spirit_writer;
namespace debit=ubc::spirit_debit;namespace cat=ubc::spirit_recipe_catalog;
namespace bridge=ubc::spirit_flask_debit_bridge;
namespace bc=ubc::batch_call;
constexpr DWORD incoming_error=0x13572468,original_error=0x24681357,native_error=0x31415926,fault=0xe042ba13;
enum class Mode {normal,whole,one,reentry,observer_reentry,foreign,wrong_caller,wrong_owner,wrong_info,wrong_quantity,
  wrong_handoff,missing_dependencies,stale_snapshot,missing_knowledge,changed_body,
  before_fault,after_fault,restore_fault,original_fault_before,original_fault_after,
  original_partial,original_refund,debit_fault_before,debit_fault_after,debit_partial,
  context_after,restore_race,amount_restore_failure,wrong_thread_boundary,early_end,
  item_unreadable_before,item_target_after,item_restore_fault,item_restore_wrong,item_already_correct,missing_test_policy,ordinary,other_else,changed_branch_before,changed_branch_after,changed_untouched,late_branch,original_timeout,preview_navigation,
  before_nonempty,original_stack_empty,original_stack_wrong,late_stack,debit_stack_changed,debit_stack_fault,restore_prefix_race,after_restore_prefix};
struct Host;
Host* host=nullptr;
void observer_clobber() {
  SetLastError(1);const unsigned csr=0x5f80;__asm {fninit} __asm {fldz} __asm {ldmxcsr csr}
}
struct Host {
  spirit_execution_test::SpiritFixture fixture;
  std::unique_ptr<writer::Executor> executor;
  ubc::AmountPatch patch;
  ubc::AmountMemory writable;
  writer::Request request;
  bc::WriterScope scope;
  Mode mode;
  unsigned calls=0,debits=0,pop_restores=0,use_restores=0,writes=0;
  unsigned item_restores=0;
  bool restored_once=false;
  std::uint64_t serial=2;
  Host(const cat::Descriptor& recipe,const LocalFacts& facts,const std::array<std::string,4>& texts,Mode m);
  static bool __cdecl boundary(void* raw,writer::Phase phase,const debit::Job& job,writer::Boundary& out) {
    auto& h=*static_cast<Host*>(raw);observer_clobber();
    if(phase==writer::Phase::before_original && h.mode==Mode::observer_reentry) {
      bc::WriterScope nested=h.scope;
      CHECK(h.executor->run(nested,h.patch,h.writable,h.request)==nullptr && !nested.restore_verified);
    }
    if(phase==writer::Phase::before_original && h.mode==Mode::before_fault)RaiseException(fault,0,0,nullptr);
    if(phase==writer::Phase::after_original && h.mode==Mode::after_fault)RaiseException(fault,0,0,nullptr);
    if(phase==writer::Phase::restore_targets && h.mode==Mode::restore_fault)RaiseException(fault,0,0,nullptr);
    if(phase==writer::Phase::after_original && h.mode==Mode::context_after)h.fixture.image.put(game+8,world+4);
    if(phase==writer::Phase::before_debit && h.mode==Mode::late_branch)h.fixture.image.put(symbol(11967)+0x18,2);
    if((phase==writer::Phase::before_debit && h.mode==Mode::late_stack)||
       (phase==writer::Phase::after_debit && h.mode==Mode::after_restore_prefix))
      h.fixture.image.put(ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack+28,3);
    out.runtime=h.fixture.boundary;out.runtime.serial=++h.serial;
    out.runtime.context=job.context;out.runtime.current_thread=GetCurrentThreadId();
    const auto actual=h.executor->scope_evidence();CHECK(actual.valid);
    CHECK(actual.scope.job==job.id && actual.scope.writer_token==job.writer_token && actual.scope.callback==job.callback &&
      actual.scope.writer_depth==1 && !actual.scope.vm_depth);
    const bool in_debit=phase==writer::Phase::after_debit || phase==writer::Phase::restore_targets || phase==writer::Phase::item_targets;
    CHECK(actual.scope.debit_depth==(in_debit?1u:0u));out.runtime.scope=actual.scope;
    out.native_handoff_verified=h.mode!=Mode::wrong_handoff;out.actual_info=execution_test::actual_info;
    if(h.mode==Mode::missing_dependencies)out.runtime.native_dependencies_and_configured_targets_verified=false;
    if(h.mode==Mode::wrong_thread_boundary)out.runtime.current_thread++;
    if(h.mode==Mode::item_unreadable_before && phase==writer::Phase::item_targets)h.fixture.image.denied=symbol(1682);
    return true;
  }
  static bool __cdecl host_dependencies(void* raw,writer::Phase,const debit::Job&) {
    return static_cast<Host*>(raw)->mode!=Mode::missing_dependencies;
  }
  static bool identity(void* raw,std::uint32_t address,std::size_t length) noexcept {
    static_cast<void>(raw);const auto& f=*ubc::spirit_recipe_catalog::recipes[0].callback;
    return address==code+f.byte_offset && length==f.byte_length;
  }
  static bool writable_page(void* raw,std::uint32_t address,std::size_t size) noexcept {
    return static_cast<Host*>(raw)->fixture.image.at(address,size)!=nullptr;
  }
  static bool replace(void* raw,std::uint32_t address,const void* before,const void* after,std::size_t size) noexcept {
    auto& h=*static_cast<Host*>(raw);++h.writes;
    if(h.mode==Mode::amount_restore_failure && h.calls)return false;
    auto* target=h.fixture.image.at(address,size);
    if(!target || std::memcmp(target,before,size))return false;
    std::memcpy(target,after,size);return true;
  }
  static void* __cdecl original(void* parser,int callback) {
    auto& h=*host;++h.calls;
    const auto actual=h.executor->scope_evidence();CHECK(actual.valid && actual.scope.vm_depth==1 && !actual.scope.debit_depth);
    CHECK(reinterpret_cast<std::uintptr_t>(parser)==ubc::lhiver::game_parser && callback==static_cast<int>(h.request.job.callback));
    CHECK(GetLastError()==incoming_error);
    const auto& r=ubc::spirit_recipe_catalog::recipes[0];
    for(std::size_t i=0;i<r.amount.count;++i) {
      const auto& op=r.amount.operands[i];
      CHECK(u32(h.fixture.image.at(code+r.callback->byte_offset+op.offset,4))==op.units*h.request.job.quantity-op.reserved);
    }
    SetLastError(original_error);
    if(h.mode==Mode::original_fault_before)RaiseException(fault,0,0,nullptr);
    h.fixture.success(h.mode==Mode::original_partial?1:h.request.job.quantity);
    if(h.mode==Mode::original_stack_empty)h.fixture.parser_stack(0);
    if(h.mode==Mode::original_stack_wrong)h.fixture.image.put(ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack+28,3);
    if(h.mode==Mode::original_timeout)h.executor->timeout();
    if(h.mode==Mode::changed_branch_after)h.fixture.image.put(symbol(11967)+0x18,3);
    if(h.mode==Mode::changed_untouched)h.fixture.image.put(object(h.request.job.branch_value==1?5u:6u)+0x32c,99);
    if(h.mode==Mode::original_refund)h.fixture.image.put(h.fixture.flask_object()+0x32c,6);
    if(h.mode==Mode::reentry) {
      bc::WriterScope nested=h.scope;
      CHECK(h.executor->run(nested,h.patch,h.writable,h.request)==nullptr && !nested.restore_verified);
    }
    if(h.mode==Mode::original_fault_after)RaiseException(fault,0,0,nullptr);
    const unsigned csr=0x3f80;__asm {ldmxcsr csr}
    return reinterpret_cast<void*>(0xBAADF00D);
  }
  static void* __thiscall remove(void* actor,int instance,int quantity) {
    auto& h=*host;++h.debits;CHECK(reinterpret_cast<std::uintptr_t>(actor)==hero && instance==16604);
    const auto actual=h.executor->scope_evidence();CHECK(actual.valid && !actual.scope.vm_depth && actual.scope.debit_depth==1);
    CHECK(quantity==static_cast<int>(h.request.job.quantity-1) && GetLastError()==original_error);
    SetLastError(native_error);
    if(h.mode==Mode::debit_fault_before)RaiseException(fault,0,0,nullptr);
    const auto remaining=u32(h.fixture.image.at(object(4)+0x32c,4))-static_cast<std::uint32_t>(quantity)+(h.mode==Mode::debit_partial?1u:0u);
    h.fixture.image.put(h.fixture.flask_object()+0x32c,remaining);
    if(!remaining) {
      const auto index=static_cast<std::uint32_t>(ubc::spirit_recipe_catalog::recipes[0].costs.count-1);
      h.fixture.image.put(node(index-1)+8,node(index+1));
    }
    h.fixture.image.put(ubc::lhiver::game_parser+0x4c,code+900);
    h.fixture.image.put(ubc::station_abi::instance_symbol_pointer,symbol(16604));
    h.fixture.image.put(ubc::station_abi::instance_address_pointer,0xDEADBEEF);
    if(h.mode!=Mode::item_already_correct)h.fixture.image.put(symbol(1682)+0x1c,0xDEAD1234);
    if(h.mode==Mode::item_target_after)h.fixture.image.put(table+1682*4,0xDEADBEEF);
    if(h.mode==Mode::debit_stack_changed || h.mode==Mode::debit_stack_fault)
      h.fixture.image.put(ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack+28,3);
    if(h.mode==Mode::debit_stack_fault)RaiseException(fault,0,0,nullptr);
    if(h.mode==Mode::debit_fault_after)RaiseException(fault,0,0,nullptr);
    const unsigned csr=0x5f80;__asm {ldmxcsr csr}
    return reinterpret_cast<void*>(0xDEADC0DE);
  }
  static void __thiscall pop(void* stack,int position) {
    auto& h=*host;++h.pop_restores;
    CHECK(reinterpret_cast<std::uintptr_t>(stack)==ubc::lhiver::game_parser+0x48 && position==100);
    h.fixture.image.put(ubc::lhiver::game_parser+0x4c,code+static_cast<std::uint32_t>(position));
    if(h.mode==Mode::restore_race)h.fixture.image.put(game+8,world+4);
    if(h.mode==Mode::restore_prefix_race)h.fixture.image.put(ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack+28,3);
  }
  static void __cdecl use(void* target) {
    auto& h=*host;++h.use_restores;CHECK(reinterpret_cast<std::uintptr_t>(target)==symbol(1679));
    h.fixture.image.put(ubc::station_abi::instance_symbol_pointer,symbol(1679));
    h.fixture.image.put(ubc::station_abi::instance_address_pointer,hero);
  }
  static void __thiscall item(void* target,int value) {
    auto& h=*host;++h.item_restores;
    CHECK(reinterpret_cast<std::uintptr_t>(target)==symbol(1682) && static_cast<std::uint32_t>(value)==0xAA123400);
    observer_clobber();
    if(h.mode==Mode::item_restore_fault)RaiseException(fault,0,0,nullptr);
    h.fixture.image.put(symbol(1682)+0x1c,h.mode==Mode::item_restore_wrong?0xBAD00000:static_cast<std::uint32_t>(value));
  }
};
Host::Host(const cat::Descriptor& recipe,const LocalFacts& facts,const std::array<std::string,4>& texts,Mode m)
  :fixture(&facts,&texts,m==Mode::ordinary?0:m==Mode::other_else||m==Mode::changed_branch_after?2:1),mode(m) {
  fixture.boundary.context.thread=fixture.boundary.current_thread=GetCurrentThreadId();
  const auto& binding=ubc::alchemy_extra_dependencies::item_binding;
  fixture.image.block(symbol(1682),0x3c);fixture.image.put(table+1682*4,symbol(1682));
  fixture.image.zstring(symbol(1682),name(1682),binding.name);
  fixture.image.put(symbol(1682)+0x20,binding.elements|(binding.type<<12)|(binding.flags<<16));
  fixture.image.put(symbol(1682)+0x38,symbol(1584));fixture.image.put(symbol(1682)+0x1c,0xAA123400);
  writable={fixture.image.memory(),this,identity,writable_page,replace};
  executor=std::make_unique<writer::Executor>(writer::Config{GetCurrentThreadId(),fixture.image.memory(),this,boundary,remove,pop,use,item,
    mode==Mode::missing_test_policy?nullptr:host_dependencies});
  fixture.image.put(object(4)+0x32c,5);
  request.snapshot=fixture.snapshot();CHECK(request.snapshot.state.contract && request.snapshot.state.material.maximum==6);
  if(m==Mode::preview_navigation){request.snapshot.state.owner_conditions_match=false;request.snapshot.state.actor_matches=false;}
  const std::uint32_t quantity=m==Mode::one?1u:(m==Mode::whole?6u:2u);
  request.job=fixture.job(quantity);request.quantity=quantity;request.snapshot_serial=1;
  request.actual_info=execution_test::actual_info;
  scope.call={reinterpret_cast<void*>(ubc::lhiver::game_parser),static_cast<int>(recipe.callback->symbol),bc::info_call_return,GetCurrentThreadId()};scope.original=original;
  if(m==Mode::wrong_caller)scope.call.caller=bc::choice_call_return;
  if(m==Mode::wrong_owner)request.job.owner++;
  if(m==Mode::wrong_info)fixture.image.put(execution_test::actual_info+0x28,0);
  if(m==Mode::wrong_quantity)request.job.quantity++;
  if(m==Mode::stale_snapshot)request.snapshot_serial=10000;
  if(m==Mode::missing_knowledge)fixture.image.put(symbol(12330)+0x18,0);
  if(m==Mode::changed_branch_before)fixture.image.put(symbol(11967)+0x18,2);
  if(m==Mode::changed_body)fixture.image.at(code+recipe.callback->byte_offset,1)[0]^=1;
  if(m==Mode::before_nonempty)fixture.parser_stack(8);
}
bool invoke(Host& h,void*& out,DWORD& exception) {
  bool returned=false;
  __try {out=h.executor->run(h.scope,h.patch,h.writable,h.request);returned=true;}
  __except(GetExceptionCode()==fault?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){exception=GetExceptionCode();}
  return returned;
}
DWORD WINAPI foreign(void* raw) {
  auto& h=*static_cast<Host*>(raw);bc::WriterScope scope=h.scope;
  return !h.executor->scope_evidence().valid && h.executor->run(scope,h.patch,h.writable,h.request)==nullptr && !scope.restore_verified?0u:1u;
}
void one(const cat::Descriptor& recipe,const LocalFacts& facts,const std::array<std::string,4>& strings,Mode mode) {
  auto instance=std::make_unique<Host>(recipe,facts,strings,mode);auto& h=*instance;host=&h;
  CHECK(!h.executor->scope_evidence().valid);
  if(mode==Mode::foreign) {
    auto thread=CreateThread(nullptr,0,foreign,&h,0,nullptr);CHECK(thread!=nullptr);
    if(thread) {CHECK(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0);DWORD code_value=1;CHECK(GetExitCodeThread(thread,&code_value) && !code_value);CloseHandle(thread);}
  }
  unsigned esp_before=0,esp_after=0,seh_before=0,seh_after=0;
  __asm {mov eax,esp} __asm {mov esp_before,eax} __asm {mov eax,fs:[0]} __asm {mov seh_before,eax}
  alignas(16) unsigned char fx[512];_fxsave(fx);
  SetLastError(incoming_error);DWORD exception=0;void* value=nullptr;
  const bool returned=invoke(h,value,exception);const auto error=GetLastError();unsigned csr=0;__asm {stmxcsr csr} _fxrstor(fx);
  __asm {mov eax,esp} __asm {mov esp_after,eax} __asm {mov eax,fs:[0]} __asm {mov seh_after,eax}
  CHECK(esp_before==esp_after && seh_before==seh_after && !h.executor->diagnostics().active && !h.executor->scope_evidence().valid);
  const bool throws=mode==Mode::original_fault_before || mode==Mode::original_fault_after || mode==Mode::debit_fault_before || mode==Mode::debit_fault_after || mode==Mode::debit_stack_fault;
  CHECK(returned!=throws);if(throws)CHECK(exception==fault);
  if(mode==Mode::original_fault_before || mode==Mode::original_fault_after)CHECK(error==original_error);
  if(mode==Mode::debit_fault_before || mode==Mode::debit_fault_after || mode==Mode::debit_stack_fault)CHECK(error==native_error);
  CHECK(h.calls<=1 && h.debits<=1);
  const bool success=mode==Mode::normal || mode==Mode::whole || mode==Mode::one || mode==Mode::early_end || mode==Mode::item_already_correct || mode==Mode::ordinary || mode==Mode::other_else || mode==Mode::preview_navigation;
  if(success) {
    CHECK(value==reinterpret_cast<void*>(0xBAADF00D) && h.calls==1 && h.debits==(mode==Mode::one?0u:1u));
    CHECK(h.executor->outcome().closing && !h.executor->outcome().blocked && h.scope.restore_verified);
    CHECK(h.executor->outcome().vm_restore==(mode==Mode::one?writer::RestoreProof::not_required_for_one:writer::RestoreProof::verified));
    CHECK(h.executor->outcome().item_restore==h.executor->outcome().vm_restore);
    CHECK(h.executor->status().stage==ubc::SpiritResultStage::closing && !h.executor->receipt().verified);
    CHECK(error==original_error && csr==0x3f80); // The additional call does not change the original post-call state.
    debit::Observation end;end.context=h.request.job.context;end.context.mob=0;end.serial=++h.serial+2;
    end.native_contract=end.stable_read=end.branch_known=true;end.branch_value=h.request.job.branch_value;end.production=end.invincible=0;end.hand={true,true,0,0,0,0};
    end.stocks.count=recipe.targets.count;end.stocks.complete_unambiguous=true;
    for(std::size_t i=0;i<recipe.targets.count;++i)end.stocks.rows[i]={static_cast<std::uint32_t>(recipe.targets[i].symbol),h.executor->receipt().rows[i].expected,true};
    const debit::EndWitness witness{true,1,1,end.serial-1,h.request.job.context};
    CHECK(h.executor->observe_completion(end,witness,false)==ubc::SpiritResultStage::closing);
    CHECK(h.executor->observe_completion(end,witness,true)==ubc::SpiritResultStage::verified);
    CHECK(h.executor->receipt().verified && h.executor->receipt().quantity==h.request.job.quantity);
    CHECK(u32(h.fixture.image.at(symbol(1682)+0x1c,4))==0xAA123400);
    CHECK(h.item_restores==((mode==Mode::one || mode==Mode::item_already_correct)?0u:1u));
    CHECK(u32(h.fixture.image.at(ubc::lhiver::game_parser+0x2058,4))==8);
    for(std::uint32_t i=0;i<8;++i)CHECK(u32(h.fixture.image.at(ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack+i*4,4))==(i%2?64u:1u));
  } else {
    CHECK(h.executor->diagnostics().disabled && !h.scope.restore_verified && !h.executor->receipt().verified);
    const auto calls=h.calls,debits=h.debits;CHECK(h.executor->run(h.scope,h.patch,h.writable,h.request)==nullptr);
    CHECK(h.calls==calls && h.debits==debits); // No retry or refund after an uncertain outcome.
  }
  if(mode!=Mode::amount_restore_failure && mode!=Mode::changed_body) {
    const auto& f=*recipe.callback;
    CHECK(hash(h.fixture.image.at(code+f.byte_offset,f.byte_length),f.byte_length)==f.body_sha256);
  }
  if(mode==Mode::restore_race)CHECK(h.pop_restores==1 && !h.use_restores);
  if(mode==Mode::restore_prefix_race)CHECK(h.pop_restores==1 && !h.use_restores && !h.item_restores);
  if(mode==Mode::debit_stack_changed || mode==Mode::debit_stack_fault)CHECK(h.debits==1 && !h.pop_restores && !h.use_restores && !h.item_restores);
  if(mode==Mode::late_stack || mode==Mode::original_stack_wrong || mode==Mode::debit_stack_changed || mode==Mode::debit_stack_fault ||
     mode==Mode::restore_prefix_race || mode==Mode::after_restore_prefix)
    CHECK(u32(h.fixture.image.at(ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack+28,4))==3); // Do not invent a stack repair.
  if(mode==Mode::before_nonempty)CHECK(!h.calls && !h.debits);
  if(mode==Mode::original_stack_empty || mode==Mode::original_stack_wrong || mode==Mode::late_stack)CHECK(h.calls==1 && !h.debits);
  if(mode==Mode::restore_fault)CHECK(!h.pop_restores && !h.use_restores);
  if(mode==Mode::original_partial || mode==Mode::original_refund || mode==Mode::context_after ||
     mode==Mode::amount_restore_failure || mode==Mode::original_fault_before || mode==Mode::original_fault_after ||
     mode==Mode::late_branch||mode==Mode::original_timeout||mode==Mode::changed_branch_before||mode==Mode::changed_branch_after||mode==Mode::changed_untouched)CHECK(!h.debits);
}
void sequence(const LocalFacts& facts,const std::array<std::string,4>& strings){
  auto ptr=std::make_unique<Host>(cat::recipes[0],facts,strings,Mode::one);auto& h=*ptr;host=&h;
  for(unsigned step=0;step<3;++step){
    if(step){h.mode=step==2?Mode::normal:Mode::one;h.fixture.set_production(4);h.fixture.image.put(hero+0x298,1);
      // The next original CallFunc entry starts with an empty data stack.
      h.fixture.parser_stack(0);
      h.request.snapshot=h.fixture.snapshot();h.request.job.id++;h.request.job.writer_token++;
      h.request.quantity=h.request.job.quantity=step==2?2u:1u;h.request.snapshot_serial=++h.serial;}
    SetLastError(incoming_error);void* value=nullptr;DWORD exception=0;
    CHECK(invoke(h,value,exception)&&!exception&&h.executor->outcome().closing&&!h.patch.active());
    CHECK(h.calls==step+1&&h.debits==(step==2?1u:0u));
    debit::Observation end;end.context=h.request.job.context;end.context.mob=0;end.serial=++h.serial+2;h.serial=end.serial;
    end.native_contract=end.stable_read=end.branch_known=true;end.branch_value=h.request.job.branch_value;end.production=end.invincible=0;end.hand={true,true,0,0,0,0};
    end.stocks.count=7;end.stocks.complete_unambiguous=true;
    for(std::size_t i=0;i<7;++i)end.stocks.rows[i]={static_cast<std::uint32_t>(cat::targets[i].symbol),h.executor->receipt().rows[i].expected,true};
    const debit::EndWitness witness{true,h.request.job.id,h.request.job.writer_token,end.serial-1,h.request.job.context};
    CHECK(h.executor->observe_completion(end,witness,true)==ubc::SpiritResultStage::verified);
    CHECK(hash(h.fixture.image.at(code+732061,324),324)==cat::recipes[0].callback->body_sha256);
  }
  const auto calls=h.calls,debits=h.debits;h.fixture.set_production(4);h.fixture.image.put(hero+0x298,1);
  h.request.snapshot=h.fixture.snapshot();h.request.job.id++;h.request.job.writer_token++; // Deliberately stale snapshot_serial
  SetLastError(incoming_error);void* value=nullptr;DWORD exception=0;
  CHECK(invoke(h,value,exception)&&h.executor->diagnostics().disabled&&h.calls==calls&&h.debits==debits);
  auto timed=std::make_unique<Host>(cat::recipes[0],facts,strings,Mode::one);host=timed.get();SetLastError(incoming_error);
  CHECK(invoke(*timed,value,exception)&&timed->executor->outcome().closing);timed->executor->timeout();
  CHECK(timed->executor->status().stage==ubc::SpiritResultStage::blocked&&timed->executor->diagnostics().disabled);
}
}
int main(int argc,char** argv) {
  if(argc!=4 || std::strcmp(argv[1],"--local-facts"))return 2;
  LocalFacts facts(argv[2],argv[3]);const auto strings=execution_test::read_prints(argv[3]);
  spirit_execution_test::structural();spirit_execution_test::parser_phases();spirit_execution_test::local(facts,strings);spirit_execution_test::restore_prefix(facts,strings);
  for(const auto& recipe:ubc::spirit_recipe_catalog::recipes)for(const auto mode:{writer_test::Mode::normal,writer_test::Mode::whole,writer_test::Mode::one})writer_test::one(recipe,facts,strings,mode);
  for(const auto mode:{writer_test::Mode::reentry,writer_test::Mode::observer_reentry,writer_test::Mode::foreign,writer_test::Mode::wrong_caller,writer_test::Mode::wrong_owner,
      writer_test::Mode::wrong_info,writer_test::Mode::wrong_quantity,writer_test::Mode::wrong_handoff,writer_test::Mode::missing_dependencies,
      writer_test::Mode::stale_snapshot,writer_test::Mode::missing_knowledge,writer_test::Mode::changed_body,writer_test::Mode::before_fault,
      writer_test::Mode::after_fault,writer_test::Mode::restore_fault,writer_test::Mode::original_fault_before,writer_test::Mode::original_fault_after,
      writer_test::Mode::original_partial,writer_test::Mode::original_refund,writer_test::Mode::debit_fault_before,writer_test::Mode::debit_fault_after,
      writer_test::Mode::debit_partial,writer_test::Mode::context_after,writer_test::Mode::restore_race,writer_test::Mode::amount_restore_failure,
      writer_test::Mode::wrong_thread_boundary,writer_test::Mode::item_unreadable_before,writer_test::Mode::item_target_after,
      writer_test::Mode::item_restore_fault,writer_test::Mode::item_restore_wrong,writer_test::Mode::item_already_correct,
      writer_test::Mode::missing_test_policy,writer_test::Mode::ordinary,writer_test::Mode::other_else,
      writer_test::Mode::changed_branch_before,writer_test::Mode::changed_branch_after,writer_test::Mode::changed_untouched,
      writer_test::Mode::late_branch,writer_test::Mode::original_timeout,writer_test::Mode::preview_navigation,
      writer_test::Mode::before_nonempty,writer_test::Mode::original_stack_empty,writer_test::Mode::original_stack_wrong,
      writer_test::Mode::late_stack,writer_test::Mode::debit_stack_changed,writer_test::Mode::debit_stack_fault,
      writer_test::Mode::restore_prefix_race,writer_test::Mode::after_restore_prefix})writer_test::one(ubc::spirit_recipe_catalog::recipes[0],facts,strings,mode);
  writer_test::sequence(facts,strings);
  std::printf("spirit_writer_tests: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
