// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
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
} // namespace ubc_native
