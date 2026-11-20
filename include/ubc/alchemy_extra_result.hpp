// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/alchemy_extra_snapshot.hpp"

namespace ubc {
enum class AlchemyExtraDebitProof { pending,not_required_for_one,verified };
enum class AlchemyExtraResultStage { idle,original_pending,extra_pending,closing,complete,blocked };
struct AlchemyExtraResultStatus {
  AlchemyExtraResultStage stage=AlchemyExtraResultStage::idle;
  bool original_success_verified=false,normal_station_end_verified=false;
  AlchemyExtraDebitProof extra=AlchemyExtraDebitProof::pending;
  alchemy_flask_debit::Issue issue=alchemy_flask_debit::Issue::none;
};
struct AlchemyExtraResultRow {
  std::uint32_t symbol=0;
  std::int64_t before=-1,expected=-1,actual=-1,consumed=0,produced=0,reserved_hand=0;
};
struct AlchemyExtraResultReceipt {
  bool verified=false;
  alchemy_recipe_catalog::RecipeId id=alchemy_recipe_catalog::RecipeId::lou_spirit;
  std::uint64_t job=0,writer_token=0,final_serial=0;
  std::uint32_t quantity=0;
  std::size_t count=0;
  std::array<AlchemyExtraResultRow,alchemy_recipe_catalog::catalog_target_capacity> rows{};
};

// Three separate checks. A callback return alone never establishes
// success: (1) exactly one successful scaled original call with operands
// restored, (2) confirmed N-1 debit and VM restore (explicitly no extra
// call for N1), (3) normal bound EndInteraction-AFTER followed by final
// stock at rest. No engine calls, retries, refunds or automatic rollback.
class AlchemyExtraResult {
public:
  AlchemyExtraResult() noexcept=default;
  AlchemyExtraResult(const AlchemyExtraResult&)=delete;
  AlchemyExtraResult& operator=(const AlchemyExtraResult&)=delete;

  AlchemyExtraResultStatus status() const noexcept {
    AlchemyExtraResultStatus result;
    result.original_success_verified=original_verified_;result.extra=extra_proof_;
    result.normal_station_end_verified=receipt_.verified;result.issue=tracker_.status().issue;
    if(tracker_.status().blocked)result.stage=AlchemyExtraResultStage::blocked;
    else if(receipt_.verified)result.stage=AlchemyExtraResultStage::complete;
    else if(!started_)result.stage=AlchemyExtraResultStage::idle;
    else if(!original_verified_)result.stage=AlchemyExtraResultStage::original_pending;
    else if(extra_proof_==AlchemyExtraDebitProof::pending)result.stage=AlchemyExtraResultStage::extra_pending;
    else result.stage=AlchemyExtraResultStage::closing;
    return result;
  }
  const AlchemyExtraResultReceipt& receipt() const noexcept {return receipt_;}

  bool begin(const AlchemyExtraSnapshot& snapshot,const alchemy_flask_debit::Job& job,
      const alchemy_flask_debit::Admission& admission,const alchemy_flask_debit::Observation& before) noexcept {
    namespace cat=alchemy_recipe_catalog;namespace debit=alchemy_flask_debit;
    if(tracker_.status().blocked || (started_ && !receipt_.verified))return false;
    const auto* recipe=cat::find(snapshot.id);
    if(!recipe || !cat::structurally_valid(*recipe) || !snapshot.effect_ready || !snapshot.visible ||
       !snapshot.profile_verified || !snapshot.learned_known || !snapshot.learned || !snapshot.owner_conditions_match ||
       !snapshot.native_debit_verified || !snapshot.material.known() || !snapshot.material.craftable ||
       !snapshot.quantity || snapshot.quantity!=job.quantity || snapshot.material.quantity!=job.quantity ||
       snapshot.material.count!=recipe->targets.count || job.quantity>snapshot.maximum_executable ||
       job.callback!=recipe->callback.function->symbol || job.owner!=static_cast<std::uint32_t>(recipe->owner.instance) ||
       !debit::same(job.context,snapshot.context) || !debit::same(job.context,before.context) ||
       !snapshot.serial || before.serial<=snapshot.serial || !debit::same(snapshot.hand,before.hand))return false;
    std::array<AlchemyExtraResultRow,cat::catalog_target_capacity> planned{};
    for(std::size_t i=0;i<recipe->targets.count;++i) {
      const auto& row=snapshot.rows[i];const auto symbol=recipe->targets[i].symbol;
      const auto cost=cat::units(recipe->costs,symbol),output=cat::units(recipe->outputs,symbol);
      const auto reserved=symbol==16604?1:0;
      const auto required=std::int64_t{cost}*job.quantity,produced=std::int64_t{output}*job.quantity;
      const auto* observed=stock(before.stocks,static_cast<std::uint32_t>(symbol));
      if(row.symbol!=symbol || row.unit_cost!=cost || row.unit_output!=output || row.stored<0 || row.stored>INT32_MAX ||
         !row.available_known || row.available!=row.stored+reserved || row.reserved!=reserved || row.held!=reserved ||
         row.required!=required || row.produced!=produced || row.remaining!=row.available-required+produced ||
         row.remaining<0 || row.remaining>INT32_MAX || !observed || !observed->known || observed->stored!=row.stored)return false;
      planned[i]={static_cast<std::uint32_t>(symbol),row.stored,row.remaining,-1,required,produced,reserved};
    }
    const auto* flask=stock(before.stocks,16604);
    if(job.quantity>1 && (!snapshot.extra_flask_stack_verified || !flask ||
       !debit::listed_flask(snapshot.flask,flask->stored) || !same_flask(snapshot.flask,before.flask)))return false;
    if(!tracker_.arm(job,admission,before))return false;
    started_=true;job_=job;original_verified_=false;extra_proof_=AlchemyExtraDebitProof::pending;
    original_serial_=extra_serial_=0;original_after_={};
    receipt_={};receipt_.id=snapshot.id;receipt_.job=job.id;receipt_.writer_token=job.writer_token;
    receipt_.quantity=job.quantity;receipt_.count=recipe->targets.count;receipt_.rows=planned;
    return true;
  }

  bool original_returned(const alchemy_flask_debit::Observation& after,bool returned_normally,bool amount_restored) noexcept {
    if(!same_job() || !tracker_.original_returned(after,returned_normally,amount_restored))return false;
    original_verified_=true;original_serial_=after.serial;original_after_=after;
    if(job_.quantity==1) {extra_proof_=AlchemyExtraDebitProof::not_required_for_one;extra_serial_=after.serial;}
    return true;
  }

  // Exclusively for the already isolated native executor instance.
  // Its positive debit_returned stage is necessary, but the associated
  // AFTER capture must be acknowledged to the result order below again.
  alchemy_flask_debit::Tracker& tracker_for_bridge() noexcept {return tracker_;}

  bool acknowledge_extra_debit(const alchemy_flask_debit::Observation& after) noexcept {
    namespace debit=alchemy_flask_debit;
    if(!same_job())return false;
    if(!original_verified_ || job_.quantity<=1 || extra_proof_!=AlchemyExtraDebitProof::pending ||
       tracker_.status().stage!=debit::Stage::debit_verified || !after.native_contract || !after.stable_read ||
       after.serial<=original_serial_ || !debit::same(after.context,job_.context) ||
       !after.scope.writer_active || after.scope.writer_depth!=1 || after.scope.vm_depth || after.scope.debit_depth!=1 ||
       after.scope.job!=job_.id || after.scope.writer_token!=job_.writer_token || after.scope.callback!=job_.callback ||
       after.production!=0 || after.invincible!=0 || !debit::same(after.hand,original_after_.hand) ||
       !debit::same(after.parser,original_after_.parser) || !expected_stocks(after.stocks))return tracker_.quarantine(debit::Issue::debit_result);
    const auto* flask=stock(after.stocks,16604);
    if(!flask || !debit::listed_flask(after.flask,flask->stored,original_after_.flask.object))return tracker_.quarantine(debit::Issue::flask);
    extra_proof_=AlchemyExtraDebitProof::verified;extra_serial_=after.serial;return true;
  }

  // Wait if the subsequent frame is still unknown or stale. A confirmed
  // contradictory terminal state permanently blocks execution. Unrelated end
  // events do not end this order. normal_end_verified comes from the separate
  // native normal-end contract. An arbitrary EndInteraction return is insufficient.
  AlchemyExtraResultStage observe_completion(const alchemy_flask_debit::Observation& after,
      const alchemy_flask_debit::EndWitness& witness,bool normal_end_verified) noexcept {
    namespace debit=alchemy_flask_debit;
    if(!same_job() || tracker_.status().blocked || receipt_.verified)return status().stage;
    if(!normal_end_verified || !witness.after || witness.job!=job_.id || witness.writer_token!=job_.writer_token ||
       !debit::same(witness.departed,job_.context))return status().stage;
    if(!after.native_contract || !after.stable_read || !after.stocks.complete_unambiguous || !after.hand.known ||
       after.serial<=original_serial_ || after.serial<=extra_serial_ ||
       witness.serial<=original_serial_ || witness.serial<=extra_serial_ || witness.serial>=after.serial)return status().stage;
    if(!original_verified_ || extra_proof_==AlchemyExtraDebitProof::pending) {tracker_.quarantine(debit::Issue::terminal);return status().stage;}
    if(!tracker_.finish(after,witness))return status().stage;
    for(std::size_t i=0;i<receipt_.count;++i) {
      const auto* actual=stock(after.stocks,receipt_.rows[i].symbol);
      if(!actual || !actual->known || actual->stored!=receipt_.rows[i].expected) {
        tracker_.quarantine(debit::Issue::terminal);return status().stage;
      }
      receipt_.rows[i].actual=actual->stored;
    }
    receipt_.verified=true;receipt_.final_serial=after.serial;return status().stage;
  }
private:
  static const alchemy_flask_debit::Row* stock(const alchemy_flask_debit::Stocks& values,std::uint32_t symbol) noexcept {
    if(!values.complete_unambiguous || values.count>values.rows.size())return nullptr;
    const alchemy_flask_debit::Row* found=nullptr;
    for(std::size_t i=0;i<values.count;++i)if(values.rows[i].symbol==symbol) {if(found)return nullptr;found=&values.rows[i];}
    return found;
  }
  static bool same_flask(const alchemy_flask_debit::FlaskStack& a,const alchemy_flask_debit::FlaskStack& b) noexcept {
    return a.known && b.known && a.all_slot_aliases_excluded==b.all_slot_aliases_excluded &&
      a.native_item_contract==b.native_item_contract && a.packed==b.packed && a.listed_stacks==b.listed_stacks &&
      a.object==b.object && a.amount==b.amount && a.flags==b.flags;
  }
  bool expected_stocks(const alchemy_flask_debit::Stocks& values) const noexcept {
    if(!values.complete_unambiguous || values.count!=receipt_.count)return false;
    for(std::size_t i=0;i<receipt_.count;++i) {
      const auto* value=stock(values,receipt_.rows[i].symbol);
      if(!value || !value->known || value->stored!=receipt_.rows[i].expected)return false;
    }
    return true;
  }
  bool same_job() noexcept {
    namespace debit=alchemy_flask_debit;
    if(!started_)return false;
    const auto& actual=tracker_.job();
    if(actual.id!=job_.id || actual.writer_token!=job_.writer_token || actual.callback!=job_.callback ||
       actual.owner!=job_.owner || actual.quantity!=job_.quantity || !debit::same(actual.context,job_.context))return tracker_.quarantine(debit::Issue::context);
    return !tracker_.status().blocked;
  }
  alchemy_flask_debit::Tracker tracker_{};
  alchemy_flask_debit::Job job_{};
  alchemy_flask_debit::Observation original_after_{};
  AlchemyExtraResultReceipt receipt_{};
  bool started_=false,original_verified_=false;
  AlchemyExtraDebitProof extra_proof_=AlchemyExtraDebitProof::pending;
  std::uint64_t original_serial_=0,extra_serial_=0;
};
} // namespace ubc
