// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/cool_predebit_diagnostic.hpp"

namespace ubc::cool_predebit::admission {
struct InfoIdentity {
  value::Context context{};
  std::uint64_t frame=0;
  std::uint32_t actual_info=0,manager=0;
};
inline bool same(const InfoIdentity& a,const InfoIdentity& b)noexcept{
  return value::same(a.context,b.context)&&a.frame==b.frame&&a.actual_info==b.actual_info&&a.manager==b.manager;
}
// One long-lived owner on the game thread. Do not recreate it for every
// job. Nesting invalidates the old lease, and an old token cannot be
// restored. Only the actual observe_info owner may call open/close.
class InfoLease {
public:
  explicit InfoLease(std::uint64_t previous=0)noexcept:sequence_(previous){}
  InfoLease(const InfoLease&)=delete;InfoLease& operator=(const InfoLease&)=delete;
  std::uint64_t open(const InfoIdentity& identity)noexcept{
    if(active_){invalidate();return 0;}
    if(sequence_==UINT64_MAX||!value::valid(identity.context)||!identity.frame||
       !identity.actual_info||identity.actual_info%4||identity.actual_info>UINT32_MAX-0x5c||
       identity.manager!=station_abi::information_manager||
       (thread_&&thread_!=identity.context.thread))return 0;
    thread_=identity.context.thread;identity_=identity;active_=++sequence_;return active_;
  }
  void invalidate()noexcept{active_=0;identity_={};}
  void close(std::uint64_t token)noexcept{if(token&&token==active_)invalidate();}
  bool current(const InfoIdentity& identity,std::uint64_t token)const noexcept{
    return token&&token==active_&&same(identity_,identity);
  }
private:
  std::uint64_t sequence_=0,active_=0;std::uint32_t thread_=0;InfoIdentity identity_{};
};
// From the real WriterScope and its current Bridge diagnostics.
// No own hook, no boolean runtime release from a test fixture.
struct WriterEntry {
  std::uint64_t token=0;
  std::uint32_t parser=0,callback=0,caller=0,thread=0;
  bool exclusive=false,original_bound=false;
};
inline InfoIdentity identity(const Job& job,std::uint64_t frame)noexcept{
  return {job.context,frame,job.actual_info,job.information_manager};
}
class Prepared;
Prepared prepare(reader::Reader&,const ReadOnlyMemory&,const reader::Input&,
                 const InfoLease&,const WriterEntry&,Rejection* = nullptr,check::Report* = nullptr,reader::Capture* = nullptr)noexcept;
// Own value copy produced only by the full Reader. It confirms
// Admission before Patch/Clear, NOT Resultguard or execution.
class Prepared {
public:
  bool valid()const noexcept{return valid_;}
  reader::Issue issue()const noexcept{return issue_;}
  const Job& job()const noexcept{return before_.job;}
  std::uint64_t frame()const noexcept{return before_.frame;}
  std::uint64_t serial()const noexcept{return before_.serial;}
  std::uint32_t body_address()const noexcept{
    return before_.parser.code_base+cool_recipe_catalog::single.information->byte_offset;
  }
  const Plan& amount()const noexcept{return amount_;}
  const StationRecipeHands& hands()const noexcept{return hands_;}
  CoolRecipeKey key()const noexcept{
    const auto& c=job().context;return {c.epoch,c.thread,c.world,c.hero,c.mob,job().information_manager,13};
  }
  bool matches_witness(const Witness& w)const noexcept{
    return valid_&&w.valid&&same_job(job(),w.job)&&frame()==w.frame&&w.extra_serial>serial()&&
      w.original_serial>w.extra_serial&&w.instance==16612&&w.extra_debit==1&&w.original_calls==1&&same_plan(amount_,w.amount);
  }
  bool current(const InfoLease& lease)const noexcept{
    return valid_&&owner_==&lease&&lease.current(identity(job(),frame()),job().info_token);
  }
  bool matches(const Observation& now,check::Checks* report=nullptr)const noexcept{
    check::Checks local;auto& k=report?*report:local;
    k.flag("match.prepared",valid_);k.flag("match.job",same_job(job(),now.job));
    k.equal("match.frame",frame(),now.frame);k.test("match.serial","nach Zulassung",now.serial,now.serial>serial());
    k.flag("match.scope",scope_matches(job(),now.scope,0));k.flag("match.stocks",rows(now.stocks,before_.stocks.stocks));
    k.flag("match.parser",value::same(before_.parser,now.parser));k.flag("match.item",old::same_item(before_.item,now.item,before_.parser));
    k.flag("match.hand",old::same_hand(before_.hand,now.hand));k.equal("match.stack",before_.stack.object,now.stack.object);
    k.digest("match.inventory",before_.inventory_identity,now.inventory_identity);k.digest("match.slots",before_.hand_identity,now.hand_identity);
    k.equal("match.manager",before_.fence.event_manager,now.fence.event_manager);
    k.digest("match.queue",before_.fence.queue_fingerprint,now.fence.queue_fingerprint);
    return k.ok();
  }

private:
  friend Prepared prepare(reader::Reader&,const ReadOnlyMemory&,const reader::Input&,
                          const InfoLease&,const WriterEntry&,Rejection*,check::Report*,reader::Capture*)noexcept;
  bool valid_=false;reader::Issue issue_=reader::Issue::request;Observation before_{};
  Plan amount_{};StationRecipeHands hands_{};const InfoLease* owner_=nullptr;
};
inline Prepared prepare(reader::Reader& reader,const ReadOnlyMemory& memory,const reader::Input& input,
                        const InfoLease& lease,const WriterEntry& writer,Rejection* rejection,
                        check::Report* report,reader::Capture* observation)noexcept{
  Prepared out;const auto q=input;check::Checks k(report);
  const auto current=[&](){return lease.current(identity(q.job,q.frame),q.job.info_token);};
  k.equal("entry.phase",static_cast<int>(reader::Phase::admission),static_cast<int>(q.phase));
  k.equal("entry.call",0,q.call!=nullptr);k.flag("entry.amount_idle",q.amount_patch&&!q.amount_patch->active());
  k.equal("entry.writer_token",q.job.writer_token,writer.token);k.flag("entry.token_present",writer.token!=0);
  k.equal("entry.parser",q.job.context.parser,writer.parser);k.equal("entry.callback",callback,writer.callback);
  k.equal("entry.caller",0x703981,writer.caller);k.equal("entry.thread",q.current_thread,writer.thread);
  k.flag("entry.exclusive",writer.exclusive);k.flag("entry.original_bound",writer.original_bound);
  const auto failed=[&](Gate gate){if(rejection&&!rejection->event){rejection->failure=k.first;rejection->gate(q.phase,gate);}};
  if(!k.ok()){failed(Gate::admission_entry);if(!report)return out;}
  k.flag("entry.info_lease",current());
  if(!k.ok()){failed(Gate::info_lease);if(!report)return out;}
  const auto captured=reader.capture(memory,q,report);out.issue_=captured.issue;
  if(observation)*observation=captured;
  // Measurements remain readable even with locked admission for independent
  // follow-up checks. Only the final success branch releases the Lease.
  out.before_=captured.observation;out.amount_=plan(out.before_.stocks,quantity);out.hands_=captured.hands;
  if(!captured.known){if(rejection)rejection->reader_failed(q.phase,captured);if(!report)return out;}
  k.flag("entry.closing_lease",current());
  if(!k.ok()){failed(Gate::info_lease);out.issue_=reader::Issue::scope;if(!report)return out;}
  Tracker guard;const bool admitted=guard.admit(q.job,captured.observation);
  k.test("entry.tracker","zugelassen (Grund 0)",static_cast<int>(guard.status().issue),admitted);
  if(!admitted||!captured.known||!k.ok()){
    if(rejection)rejection->tracker_failed(q.phase,guard.status().issue);
    out.issue_=reader::Issue::request;return out;
  }
  out.owner_=&lease;out.valid_=true;return out;
}
} // namespace ubc::cool_predebit::admission
