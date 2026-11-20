// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_hotblade_debit.hpp"

// Own pre-debit order. The existing post-original tracker remains
// unchanged. Pure values; no Nativecall and no second end machine.
namespace ubc::cool_predebit {
namespace value=alchemy_flask_debit;
namespace old=cool_hotblade_debit;
namespace prefix=cool_parser_prefix;
// World debit confirmed. The isolated x86 host also exercises the disabled state.
#ifdef UBC_COOL_PREDEBIT_HOST_TEST
inline bool runtime_ready=false;
#else
inline constexpr bool runtime_ready=true;
#endif
inline constexpr bool native_effect_authorized=false;
inline constexpr std::uint32_t callback=20775,owner=20773,quantity=2;
using Stocks=std::array<cool_recipe::Stock,2>;
struct Plan {
  bool valid=false,quantity_receipt=false;
  std::uint32_t maximum=0,direct_maximum=0,extra_debit=0,native_end_debit=0;
  Stocks before{},before_original{},after_callback{},after_end{};
  std::array<AmountOperand,1> operands{};
  AmountBody body()const & noexcept {
    const auto& f=*cool_recipe_catalog::single.information;
    return valid?AmountBody{f.byte_length,f.body_sha256,operands.data(),operands.size()}:AmountBody{};
  }
  AmountBody body()const &&=delete;
};
inline Plan plan(const cool_recipe::Evidence& before,std::uint32_t n) noexcept {
  const auto numeric=cool_recipe::plan(cool_recipe_catalog::recipe,before,n);
  if(!numeric.valid||n!=2||!numeric.requires_extra_debit||numeric.extra_debit!=1||
     numeric.native_end_debit!=1||numeric.callback!=callback||numeric.owner!=owner||
     numeric.after_callback[0].amount!=2)return {};
  Plan out;out.valid=out.quantity_receipt=true;out.maximum=numeric.maximum;
  out.direct_maximum=numeric.direct_maximum;out.extra_debit=out.native_end_debit=1;
  out.before=before.stocks;
  // Order of input values may differ; canonical rows are generated.
  out.before=numeric.after_callback;out.before[1].amount-=2;
  out.before_original=out.before;--out.before_original[0].amount;
  out.after_callback=out.before_original;out.after_callback[1].amount+=2;
  out.after_end=out.after_callback;--out.after_end[0].amount;
  out.operands[0]={62,1,1,0,false};return out;
}
inline bool rows(const cool_recipe::Evidence& observed,const Stocks& expected) noexcept {
  if(!observed.stable_boundary_verified)return false;
  for(const auto& wanted:expected){const cool_recipe::Stock* found=nullptr;
    for(const auto& row:observed.stocks)if(row.symbol==wanted.symbol){if(found)return false;found=&row;}
    if(!found||!found->known||found->amount!=wanted.amount||!wanted.known||
       (wanted.symbol==16612&&!found->unambiguous))return false;
  }return true;
}
inline bool same_plan(const Plan& a,const Plan& b) noexcept {
  if(!a.valid||!b.valid||!a.quantity_receipt||!b.quantity_receipt||a.maximum!=b.maximum||
     a.direct_maximum!=b.direct_maximum||a.extra_debit!=b.extra_debit||a.native_end_debit!=b.native_end_debit)return false;
  const auto& x=a.operands[0];const auto& y=b.operands[0];
  if(x.offset!=y.offset||x.original!=y.original||x.units!=y.units||x.reserved!=y.reserved||x.allow_zero!=y.allow_zero)return false;
  const auto same=[](const Stocks& left,const Stocks& right)noexcept{
    for(std::size_t i=0;i<left.size();++i){
      const auto& l=left[i];const auto& r=right[i];
      if(l.symbol!=r.symbol||l.amount!=r.amount||l.known!=r.known||l.unambiguous!=r.unambiguous)return false;
    }
    return true;
  };
  return same(a.before,b.before)&&same(a.before_original,b.before_original)&&
    same(a.after_callback,b.after_callback)&&same(a.after_end,b.after_end);
}
struct Job {
  value::Context context{};
  std::uint64_t id=0,writer_token=0,fence_token=0,info_token=0;
  std::uint32_t actual_info=0,information_manager=0;
};
inline bool valid(const Job& j) noexcept {
  return value::valid(j.context)&&j.id&&j.writer_token&&j.fence_token&&j.info_token&&
    j.actual_info&&j.information_manager&&j.actual_info%4u==0;
}
inline bool same_job(const Job& a,const Job& b) noexcept {
  return valid(a)&&valid(b)&&value::same(a.context,b.context)&&a.id==b.id&&a.writer_token==b.writer_token&&
    a.fence_token==b.fence_token&&a.info_token==b.info_token&&a.actual_info==b.actual_info&&
    a.information_manager==b.information_manager;
}
struct Scope {
  value::Scope writer{};
  std::uint64_t info_token=0;
  std::uint32_t actual_info=0;
  bool info_lease_active=false;
};
inline bool scope_matches(const Job& j,const Scope& s,std::uint32_t debit_depth) noexcept {
  return s.writer.writer_active&&s.writer.writer_depth==1&&!s.writer.vm_depth&&
    s.writer.debit_depth==debit_depth&&s.writer.job==j.id&&s.writer.writer_token==j.writer_token&&
    s.writer.callback==callback&&s.info_lease_active&&s.info_token==j.info_token&&s.actual_info==j.actual_info;
}
// Only the composed reader may supply these values in the native
// adapter. known includes genuine code/data binding; two identical reads
// or unchanged destroy counters do not produce the interval evidence.
struct SplitFence {
  bool known=false,path_excludes_dispatch=false;
  std::uint64_t token=0,serial=0;
  std::uint32_t event_manager=0,destroy_started=UINT32_MAX,destroy_completed=UINT32_MAX;
  lhiver::Digest queue_fingerprint{};
};
inline bool pending(const SplitFence& f,const Job& j,std::uint64_t serial) noexcept {
  return f.known&&f.path_excludes_dispatch&&f.token==j.fence_token&&f.serial==serial&&
    f.event_manager&&f.event_manager!=j.information_manager&&!f.destroy_started&&!f.destroy_completed;
}
struct Observation {
  Job job{};Scope scope{};std::uint64_t serial=0;
  cool_recipe::Evidence stocks{};old::Hand hand{};old::HotStack stack{};
  value::ParserState parser{};old::ItemBinding item{};SplitFence fence{};
  // Complete current list/package/slot copy; exactly the amount of the
  // bound hot stack is normalized in the inventory digest. No artificially
  // shortened material list as alias or foreign delta evidence.
  lhiver::Digest inventory_identity{},hand_identity{};
  bool complete_inventory=false;
  std::int32_t production=-1,invincible=-1,start_smithing=-1;
  bool stable_read=false,native_split_restore=false,generated_hand_profile=false;
  std::uint64_t frame=0;
};
enum class Stage {idle,admitted,calling_extra,extra_verified,calling_original,returned,failed};
enum class Issue {none,job,scope,sequence,observation,plan,hand,stack,parser,item,fence,
  debit_exception,restore,post_delta,info,original_exception,amount_restore,original_result};
struct Status {Stage stage=Stage::idle;Issue issue=Issue::none;bool blocked=false;};
struct Call {
  bool valid=false;Job job{};std::uint64_t serial=0;
  std::uint32_t instance=0,amount=0,stack_object=0,generated_hand=0;
  value::ParserState parser{};old::ItemBinding item{};SplitFence fence{};
  lhiver::Digest inventory_identity{},hand_identity{};
  std::uint64_t frame=0;
};
inline bool valid_call(const Call& c) noexcept {
  return c.valid&&valid(c.job)&&c.serial&&c.frame&&c.instance==16612&&c.amount==1&&c.stack_object&&
    c.generated_hand&&c.stack_object!=c.generated_hand&&pending(c.fence,c.job,c.serial)&&
    prefix::classify(c.parser,c.job.context,callback,prefix::Phase::before_original).recognized&&
    old::valid_item(c.item,c.parser);
}
struct RestoreObservation {
  Job job{};Scope scope{};std::uint64_t serial=0;
  value::RestoreObservation targets{};value::ParserState current{};old::ItemBinding item{};
  SplitFence fence{};bool native_set_item=false;
  lhiver::Digest inventory_identity{},hand_identity{};
  std::uint64_t frame=0;
};
inline old::RestorePlan restore_plan(const Call& call,const RestoreObservation& now) noexcept {
  if(!valid_call(call)||!same_job(call.job,now.job)||now.serial<=call.serial||
     !scope_matches(call.job,now.scope,1)||!pending(now.fence,call.job,now.serial)||
     now.fence.event_manager!=call.fence.event_manager||now.fence.queue_fingerprint!=call.fence.queue_fingerprint||
     !now.native_set_item||now.frame!=call.frame||now.inventory_identity!=call.inventory_identity||now.hand_identity!=call.hand_identity)return {};
  const value::Call neutral{true,call.job.id,call.job.writer_token,call.job.context,callback,
    call.instance,call.amount,call.stack_object,call.parser};
  if(!value::restorable(neutral,now.targets)||!old::same_parser_identity(call.parser,now.current)||
     !value::same_data_stack(call.parser,now.current)||!old::same_item_target(call.item,now.item,call.parser))return {};
  const auto& p=now.current;const auto& t=now.targets;
  if(p.current_parser!=t.current_parser||p.code_base!=t.code_base||p.code_size!=t.code_size||
     p.self_symbol!=t.self_symbol||p.hero_symbol!=t.hero_symbol||p.self!=t.self||p.hero!=t.hero||
     p.other!=t.other||p.victim!=t.victim)return {};
  return {true,call.item.value!=now.item.value,0x7a4b20,call.job.context.parser+0x48u,call.parser.pop_pos,
    0x7a1550,call.parser.use_symbol,0x7a2f20,call.item.symbol_object,call.item.value};
}
// No result or final success yet. The existing result guard requires an
// additional genuine destroy1, normalEnd, later balance, and receipt2-ACK.
struct Witness {
  bool valid=false;Job job{};std::uint64_t extra_serial=0,original_serial=0;
  std::uint32_t instance=0,extra_debit=0,original_calls=0;
  Plan amount{};std::uint64_t frame=0;
};
class Tracker {
public:
  Tracker()noexcept=default;Tracker(const Tracker&)=delete;Tracker& operator=(const Tracker&)=delete;
  Status status()const noexcept{return status_;}
  const Call& call()const noexcept{return call_;}
  bool fail(Issue why) noexcept {
    if(!status_.blocked){status_.issue=why;status_.blocked=true;status_.stage=Stage::failed;}return false;
  }
  bool admit(const Job& job,const Observation& before) noexcept {
    // Only a fully completed writer can accept a later order. The
    // result guard checks end and receipt of the predecessor first.
    if(status_.stage==Stage::returned&&job.id>job_.id&&job.writer_token>job_.writer_token&&
       job.info_token>job_.info_token&&before.serial>last_serial_)status_={};
    if(status_.stage!=Stage::idle||status_.blocked||!valid(job)||!same_job(job,before.job))return false;
    job_=job;call_={};
    if(!pre(before,0,2,nullptr))return false;
    amount_=plan(before.stocks,quantity);if(!amount_.valid)return fail(Issue::plan);
    before_=before;last_serial_=before.serial;status_.stage=Stage::admitted;return true;
  }
  // The reserved call binds the last read check. The executor
  // marks the actual remove entry only after this check.
  Call enter_extra(const Observation& late) noexcept {
    if(status_.blocked)return {};
    if(status_.stage!=Stage::admitted){fail(Issue::sequence);return {};}
    if(!pre(late,0,2,&before_)||!rows(late.stocks,amount_.before)){fail(Issue::post_delta);return {};}
    if(!value::same(before_.parser,late.parser)||!old::same_item(before_.item,late.item,before_.parser)){
      fail(Issue::parser);return {};}
    call_={true,job_,late.serial,16612,1,late.stack.object,late.hand.left,late.parser,late.item,late.fence};
    call_.inventory_identity=late.inventory_identity;call_.hand_identity=late.hand_identity;
    call_.frame=late.frame;
    last_serial_=late.serial;status_.stage=Stage::calling_extra;return call_;
  }
  bool extra_returned(const Observation& after,bool normal,bool restored) noexcept {
    if(status_.blocked)return false;
    if(status_.stage!=Stage::calling_extra||!valid_call(call_))return fail(Issue::sequence);
    if(!normal)return fail(Issue::debit_exception);if(!restored)return fail(Issue::restore);
    if(!pre(after,1,1,&before_)||!rows(after.stocks,amount_.before_original))return fail(Issue::post_delta);
    if(!value::same(call_.parser,after.parser)||!old::same_item(call_.item,after.item,call_.parser))return fail(Issue::restore);
    last_serial_=extra_serial_=after.serial;status_.stage=Stage::extra_verified;return true;
  }
  // Fresh PostDelta and INFO lease evidence. Do not require the Hot2 values to match again.
  bool enter_original(const Observation& late) noexcept {
    if(status_.blocked)return false;
    if(status_.stage!=Stage::extra_verified)return fail(Issue::sequence);
    if(!pre(late,0,1,&before_)||!rows(late.stocks,amount_.before_original))return fail(Issue::post_delta);
    if(!value::same(call_.parser,late.parser)||!old::same_item(call_.item,late.item,call_.parser))return fail(Issue::restore);
    last_serial_=late.serial;status_.stage=Stage::calling_original;return true;
  }
  // The short fence is already finished here. Queue/item pointers from
  // call cannot be dereferenced or used for restoration after original.
  bool original_returned(const Observation& after,bool normal,bool amount_restored) noexcept {
    if(status_.blocked)return false;
    if(status_.stage!=Stage::calling_original)return fail(Issue::sequence);
    if(!normal)return fail(Issue::original_exception);if(!amount_restored)return fail(Issue::amount_restore);
    if(!same_job(job_,after.job)||!scope_matches(job_,after.scope,0)||after.serial<=last_serial_||
       !after.stable_read||after.frame!=before_.frame||after.production||after.invincible||after.start_smithing!=1||
       !rows(after.stocks,amount_.after_callback)||!old::same_hand(before_.hand,after.hand)||
       !after.generated_hand_profile||after.hand_identity!=before_.hand_identity)return fail(Issue::original_result);
    if(!prefix::classify(after.parser,job_.context,callback,prefix::Phase::after_original).recognized||
       !old::same_parser_identity(before_.parser,after.parser))return fail(Issue::parser);
    last_serial_=after.serial;status_.stage=Stage::returned;return true;
  }
  Witness witness()const noexcept {
    if(status_.blocked||status_.stage!=Stage::returned||!call_.valid)return {};
    return {true,job_,extra_serial_,last_serial_,16612,1,1,amount_,before_.frame};
  }
private:
  bool pre(const Observation& now,std::uint32_t depth,std::uint32_t hot,const Observation* original) noexcept {
    if(!same_job(job_,now.job))return fail(Issue::job);
    if(!scope_matches(job_,now.scope,depth))return fail(Issue::scope);
    if(!now.serial||now.serial<=last_serial_||!now.frame||!now.stable_read||!now.native_split_restore||
       !now.complete_inventory||now.production!=13||now.invincible!=1||now.start_smithing!=1)return fail(Issue::observation);
    if(!old::active_hand(now.hand)||!now.generated_hand_profile)return fail(Issue::hand);
    if(!old::listed_stack(now.stack,hot)||now.stack.object==now.hand.left)return fail(Issue::stack);
    if(!prefix::classify(now.parser,job_.context,callback,prefix::Phase::before_original).recognized)return fail(Issue::parser);
    if(!old::valid_item(now.item,now.parser))return fail(Issue::item);
    if(!pending(now.fence,job_,now.serial))return fail(Issue::fence);
    if(original&&(original->frame!=now.frame||!old::same_hand(original->hand,now.hand)||original->stack.object!=now.stack.object||
       original->fence.event_manager!=now.fence.event_manager||original->fence.queue_fingerprint!=now.fence.queue_fingerprint||
       original->inventory_identity!=now.inventory_identity||original->hand_identity!=now.hand_identity))return fail(Issue::fence);
    return true;
  }
  Status status_{};Job job_{};Plan amount_{};Observation before_{};Call call_{};
  std::uint64_t last_serial_=0,extra_serial_=0;
};
} // namespace ubc::cool_predebit
