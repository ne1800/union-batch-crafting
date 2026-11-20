// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/stations/cool/cool_predebit_admission.hpp"
#include "ubc/core/recipe_result.hpp"
#include "ubc/stations/station_recipe_result.hpp"

namespace ubc {
enum class CraftingResultState { idle, waiting, verified, failed };
enum class CraftingResultIssue { none, contract, context, stock, hands, anchor, timeout, restore, destroy, skill };
enum class CraftingResultTerminal { none, ready, ended };
struct CraftingResultBinding {
  // Directly before the single effect, after completed original navigation.
  std::uint64_t generation=0,start_frame=0,collection=0,end_sequence=0;
  std::uint32_t world=0,hero=0,station=0;
};
struct CraftingResultEndWitness {
  // Only a matching EndInteraction-AFTER after a normal exit. A callback
  // return, B_END or an interrupt alone supplies none of this evidence.
  bool after_original=false,normal_exit_verified=false;
  std::uint64_t generation=0,sequence=0,frame=0;
  std::uint32_t world=0,hero=0,station=0;
};
struct CraftingResultDestroyWitness {
  // Only from a successful station_destroy_contract on our own job. Require actual
  // handler and Remove AFTER events. Never infer them from Return1 or final balances.
  bool after_original=false,normal_remove_verified=false;
  CraftingKey key{};
  crafting_catalog::RecipeId recipe=UINT32_MAX;
  std::uint32_t quantity=0;
  std::uint64_t serial=0,frame=0;
  std::int32_t instance=-1;
  std::uint32_t amount=0;
};
struct CraftingResultSkillWitness {
  // Our own actual snapshots before and after the single original quantity
  // call. The native producer binds normal callback return, code, thread, job
  // and stable values. An expected number alone cannot produce this witness.
  bool after_original=false,original_progress_verified=false;
  CraftingKey key{};
  crafting_catalog::RecipeId recipe=UINT32_MAX;
  std::uint32_t quantity=0;
  std::uint64_t serial=0,frame=0;
  CraftingGrindValues before{},after{};
};
struct CraftingResultExtraRestore {
  // Only from the actually checked extra WriterResult and ITEM
  // restore. N1 requires its explicit not_required_for_one evidence.
  // Neither a quantity of 1 nor a successful AmountPatch supplies it.
  std::uint64_t generation=0;
  crafting_catalog::RecipeId recipe=UINT32_MAX;
  std::uint32_t quantity=0;
  bool writer_result_verified=false,item_restore_verified=false;
};
struct CraftingResultBranchWitness {
  bool known=false,writer_result_verified=false;
  std::uint64_t generation=0;
  crafting_catalog::RecipeId recipe=UINT32_MAX;
  std::uint32_t quantity=0;
  std::int32_t value=0;
};
struct CraftingResultObservation {
  std::uint64_t generation=0,frame=0;
  bool stable_boundary_verified=false,identity_known=false;
  std::uint32_t world=0,hero=0;
  bool station_known=false;
  std::uint32_t station=0;
  bool production_known=false;
  std::int32_t production=0;
  // selection_owner_verified binds our current manager, player and NPC to the
  // request context. A matching view address and row count are insufficient.
  bool selection_known=false,selection_owner_verified=false;
  bool selection_open=false,selection_waiting=false,selection_done=false;
  bool collection_known=false;
  std::uint64_t collection=0;
  // Full independent value copy from the actual stable point; no hand reserve
  // at all. An unreadable/unstable reader must deliver stock_known=false.
  bool stock_known=false;
  recipe_catalog::View<RecipeResultItem> items;
  bool hands_known=false;
  StationRecipeHands hands;
  CraftingResultBranchWitness branch{};
  // Fresh complete value report in the later end-stable frame.
  CraftingGrindValues grind{};
};
struct CraftingResultProduced { std::int32_t symbol=-1,amount=-1; };
struct CraftingResultStock { std::int32_t symbol=-1,stored=-1; };
struct CraftingResultDelta {
  std::int32_t symbol=-1;
  std::int64_t before=-1,after=-1,delta=0; // Inventory delta; flask-hand remains separate.
};
struct CraftingResultSuccess {
  bool result_verified=false,restore_verified=false;
  std::uint64_t receipt=0,frame=0;
  CraftingKey key{};
  crafting_catalog::RecipeId id=UINT32_MAX;
  std::uint32_t quantity=0;
  CraftingResultTerminal terminal=CraftingResultTerminal::none;
  std::size_t produced_count=0,stock_count=0;
  std::array<CraftingResultProduced,crafting_catalog::target_capacity> produced{};
  std::array<CraftingResultStock,crafting_catalog::target_capacity> stocks{};
  std::array<CraftingResultDelta,crafting_catalog::target_capacity> deltas{};
  bool branch_known=false;std::int32_t branch_value=0,branch_output=-1;
  bool quantity_receipt_required=false,quantity_receipt_verified=false;
  CraftingGrindValues grind_before{},grind_after{};
  std::uint32_t skill_increment=0;
};

namespace crafting_result_detail {
inline bool separate_ended_route(crafting_catalog::Route route) noexcept {
  using crafting_catalog::Route;
  return route==Route::tobacco||route==Route::alchemy_extra||route==Route::anvil||route==Route::ore||route==Route::spirit||route==Route::bow||route==Route::cool||route==Route::grind;
}
inline std::int32_t production(const crafting_catalog::Descriptor& d) noexcept {
  using crafting_catalog::Route;
  switch(d.route){
    case Route::base:return d.base?d.base->production_context:0;
    case Route::station:return d.station_descriptor?d.station_descriptor->production_context:0;
    case Route::alchemy_extra:return d.alchemy_extra?d.alchemy_extra->production_context:0;
    case Route::anvil:return d.anvil?d.anvil->production_context:0;
    case Route::ore:return d.ore?d.ore->production_context:0;
    case Route::tobacco:return d.tobacco?4:0;
    case Route::spirit:return d.spirit?4:0;
    case Route::bow:return d.bow?d.bow->station_profile->production_context:0;
    case Route::cool:return d.cool?d.cool->production_context:0;
    case Route::grind:return d.grind?11:0;
  }
  return 0;
}
inline bool canonical_ended(const crafting_catalog::Descriptor& d) noexcept {
  using crafting_catalog::Route;
  if(d.completion!=crafting_catalog::Completion::end_production)return false;
  switch(d.route){
    case Route::alchemy_extra:return d.alchemy_extra&&alchemy_recipe_catalog::structurally_valid(*d.alchemy_extra);
    case Route::anvil:return d.anvil&&anvil_recipe_catalog::structurally_valid(*d.anvil);
    case Route::ore:return d.ore&&ore_recipe_catalog::structurally_valid(*d.ore);
    case Route::tobacco:return d.tobacco&&tobacco::canonical(*d.tobacco);
    case Route::spirit:return d.spirit&&spirit_recipe_catalog::structurally_valid(*d.spirit);
    case Route::bow:return d.bow&&bow_recipe_catalog::structurally_valid(*d.bow);
    case Route::cool:return d.cool&&cool_recipe_catalog::structurally_valid(*d.cool);
    case Route::grind:return d.grind&&grind_recipe::structurally_valid(*d.grind);
    default:return false;
  }
}

// Own end watcher for the three new real catalog routes. No reinterpretation
// into a base/station descriptor and no ready replacement receipt.
// The expected values come from the above canonically reconstructed
// N-preview; after begin no Views or descriptor pointers are kept.
class EndedGuard {
public:
  bool begin(const crafting_catalog::Descriptor& d,recipe_catalog::View<RecipeResultItem> expected,
      const CraftingResultBinding& binding) noexcept {
    if(state_==CraftingResultState::waiting||state_==CraftingResultState::failed)return false;
    if(!canonical_ended(d)||!production(d)||!binding.generation||binding.generation<=binding_.generation||
      binding.start_frame<last_frame_||!binding.world||!binding.hero||!binding.station||
      !expected.data||!expected.count||expected.count>expected_.size()||expected.count!=d.targets.count)
      return fail(CraftingResultIssue::contract),false;
    for(std::size_t i=0;i<expected.count;++i){
      if(expected[i].symbol!=d.targets[i].symbol||expected[i].total<0||expected[i].total>INT32_MAX)
        return fail(CraftingResultIssue::contract),false;
      for(std::size_t j=0;j<i;++j)if(expected[i].symbol==expected[j].symbol)return fail(CraftingResultIssue::contract),false;
      expected_[i]=expected[i];
    }
    count_=expected.count;binding_=binding;production_=production(d);last_frame_=binding.start_frame;
    has_end_=false;end_frame_=0;issue_=CraftingResultIssue::none;state_=CraftingResultState::waiting;return true;
  }
  bool end_after(const CraftingResultEndWitness& v) noexcept {
    if(state_!=CraftingResultState::waiting||has_end_||!v.after_original||!v.normal_exit_verified||
      v.generation!=binding_.generation||v.sequence<=binding_.end_sequence||v.frame<last_frame_||
      v.world!=binding_.world||v.hero!=binding_.hero||v.station!=binding_.station)return false;
    has_end_=true;end_frame_=v.frame;return true;
  }
  CraftingResultState observe(const CraftingResultObservation& v) noexcept {
    if(state_!=CraftingResultState::waiting||v.generation<binding_.generation)return state_;
    if(v.generation!=binding_.generation)return fail(CraftingResultIssue::contract);
    if(!v.stable_boundary_verified||v.frame<=binding_.start_frame||v.frame<last_frame_)return state_;
    last_frame_=v.frame;
    if(v.identity_known&&(v.world!=binding_.world||v.hero!=binding_.hero))return fail(CraftingResultIssue::context);
    if(v.station_known&&v.station&&v.station!=binding_.station)return fail(CraftingResultIssue::context);
    if(v.production_known&&v.production&&v.production!=production_)return fail(CraftingResultIssue::context);
    if(!v.identity_known||!v.station_known||!v.production_known||!has_end_||
      v.frame<=end_frame_||v.station||v.production)return state_;
    if(v.stock_known&&!matching_stock(v.items))return fail(CraftingResultIssue::stock);
    bool empty=v.hands_known;
    if(v.hands_known){
      for(const auto* hand:{&v.hands.hands.left,&v.hands.hands.right}){
        if(hand->state==-1)empty=false;
        else if(hand->state!=0||hand->object)return fail(CraftingResultIssue::hands);
      }
      if(v.hands.interact_item_known&&v.hands.interact_item)return fail(CraftingResultIssue::anchor);
      empty=empty&&v.hands.interact_item_known;
    }
    if(v.stock_known&&empty)state_=CraftingResultState::verified;
    return state_;
  }
  CraftingResultState fail(CraftingResultIssue issue) noexcept {
    if(state_!=CraftingResultState::failed){state_=CraftingResultState::failed;issue_=issue;}
    return state_;
  }
  CraftingResultIssue issue() const noexcept{return issue_;}
private:
  bool matching_stock(recipe_catalog::View<RecipeResultItem> items) const noexcept {
    if(!items.data||items.count!=count_)return false;
    for(std::size_t i=0;i<items.count;++i){
      if(items[i].total<0||items[i].total>INT32_MAX)return false;
      for(std::size_t j=0;j<i;++j)if(items[j].symbol==items[i].symbol)return false;
      bool found=false;
      for(std::size_t j=0;j<count_;++j)if(items[i].symbol==expected_[j].symbol){
        if(items[i].total!=expected_[j].total)return false;found=true;break;
      }
      if(!found)return false;
    }
    return true;
  }
  CraftingResultState state_=CraftingResultState::idle;
  CraftingResultIssue issue_=CraftingResultIssue::none;
  CraftingResultBinding binding_{};
  std::array<RecipeResultItem,crafting_catalog::target_capacity> expected_{};
  std::size_t count_=0;
  std::uint64_t last_frame_=0,end_frame_=0;
  std::int32_t production_=0;
  bool has_end_=false;
};
} // namespace crafting_result_detail

// Shared pure adapter around the two unchanged result watchers and
// the separate end watch for Extra-Alchemy/Anvil/Ore. No engine
// call, memory reader, UI receipt, retry, or material rollback.
// The native adapter serializes on the bound thread and hands over
// the immediately re-confirmed request; its receipt flags are not
// re-proven by this quantity reconstruction. All values are copied.
// Views must only refer to own valid objects for the respective call.
//
// Error lock, generation, and frame boundary apply across ALL routes.
// N1 also requires restored(true). Success follows only a later
// full observation frame. receipt is the global job generation.
class CraftingResult {
public:
  bool begin(const CraftingRequest& request,const CraftingResultBinding& binding) noexcept {
    return begin_job(request,binding,nullptr);
  }
  bool begin_cool_predebit(const CraftingRequest& request,const CraftingResultBinding& binding,
      const cool_predebit::admission::Prepared& prepared,const cool_predebit::admission::InfoLease& lease,
      cool_predebit::check::Checks* diagnostic=nullptr) noexcept {
    cool_predebit::check::Checks local;auto& checks=diagnostic?*diagnostic:local;
    const bool idle=permits_next();const auto& k=request.key;const auto source=prepared.key();
    checks.flag("result.idle",idle);checks.flag("result.lease",prepared.current(lease));
    checks.equal("result.recipe",static_cast<int>(cool_recipe_catalog::RecipeId::blade),static_cast<int>(request.id));
    checks.equal("result.quantity",cool_predebit::quantity,request.quantity);
    checks.equal("result.kind",static_cast<int>(crafting_catalog::Station::cool),static_cast<int>(k.kind));
    checks.equal("result.job",k.generation,prepared.job().id);checks.equal("result.generation",k.generation,source.generation);
    checks.equal("result.thread",k.thread,source.thread);checks.equal("result.world",k.world,source.world);
    checks.equal("result.hero",k.hero,source.hero);checks.equal("result.station",k.station,source.station);
    checks.equal("result.manager",k.manager,source.manager);
    checks.equal("result.maximum",request.material_snapshot.maximum,prepared.amount().maximum);
    checks.equal("result.frame",binding.start_frame,prepared.frame());
    checks.flag("result.hands",same_crafting_hands(prepared.hands(),request.hands));
    for(const auto& stock:prepared.amount().before){
      const auto* material=crafting_detail::material(request.material_snapshot,stock.symbol);
      checks.equal(stock.symbol==16612?"result.hot":"result.cold",stock.amount,material?material->stored:0,material!=nullptr);
    }
    if(!idle)return false;
    if(!checks.ok()){if(diagnostic&&diagnostic->collecting()){
        auto probe=*this;checks.flag("result.request_binding",probe.begin_job(request,binding,&prepared));
      }
      return bad_contract();
    }
    const bool bound=begin_job(request,binding,&prepared);checks.flag("result.request_binding",bound);return bound;
  }
  const cool_predebit::admission::Prepared& cool_prepared()const noexcept{return predebit_;}
  const cool_predebit::Witness& cool_witness()const noexcept{return predebit_witness_;}
  bool cool_predebit_restored(const cool_predebit::Witness& witness,std::uint64_t frame) noexcept {
    if(state_!=CraftingResultState::waiting||!predebit_.valid()||restore_verified_||frame!=binding_.start_frame||
       !predebit_.matches_witness(witness)){fail(CraftingResultIssue::restore);return false;}
    predebit_witness_=witness;restore_verified_=true;return true;
  }
private:
  bool begin_job(const CraftingRequest& request,const CraftingResultBinding& binding,
                 const cool_predebit::admission::Prepared* prepared) noexcept {
    if(!permits_next())return false;
    const auto* d=crafting_catalog::find(request.id);const auto& s=request.material_snapshot;
    if(!d||!crafting_detail::valid_key(request.key)||request.key.kind!=d->station||
      !request.quantity||request.quantity>static_cast<std::uint32_t>(INT32_MAX)||
      request.quantity>s.maximum||s.id!=request.id||!s.evaluated||s.generation!=request.key.generation||
      !valid_crafting_state(s)||
      crafting_quantity_block_reason(s,request.quantity)!=CraftingBlockReason::none||
      (d->cool&&crafting_cool_plan(s,request.quantity).requires_extra_debit!=(prepared!=nullptr))||
      !crafting_detail::bound_hands(*d,request.hands)||binding.generation!=request.key.generation||
      binding.generation<=binding_.generation||binding.start_frame<last_frame_||
      binding.world!=request.key.world||binding.hero!=request.key.hero||binding.station!=request.key.station)
      return bad_contract();
    std::array<RecipeResultItem,crafting_catalog::target_capacity> expected{};
    std::array<StationRecipeMaterialRow,crafting_catalog::target_capacity> rows{};
    for(std::size_t i=0;i<d->targets.count;++i) {
      const auto* row=crafting_detail::material(s,d->targets[i].symbol);
      if(!row)return bad_contract();
      const auto preview=crafting_amount_preview(*row,request.quantity);
      if(!preview.available_known||!preview.ready_known||!preview.end_known||preview.missing||
        preview.output_shortfall||preview.end_overflow||!valid_total(preview.ready_storage)||!valid_total(preview.end_storage))
        return bad_contract();
      expected[i]={row->symbol,preview.end_storage};
      auto& out=rows[i];out.symbol=row->symbol;out.unit_cost=row->unit_cost;out.unit_output=row->unit_output;
      out.stored=row->stored;out.reserved_tool=row->tool_reserve;out.generated_hand=row->generated_prop;
      out.available=row->stored;out.available_known=true;
      out.required=preview.required;out.produced=preview.produced;out.missing=out.missing_for_one=0;
      out.output_space=INT32_MAX-row->stored;out.output_shortfall=out.output_shortfall_for_one=0;
      out.expected_ready_storage=preview.ready_storage;out.expected_end_storage=preview.end_storage;
      out.ready_storage_known=out.end_storage_known=true;out.end_overflow=0;
    }
    bool started=false;
    if(d->route==crafting_catalog::Route::base) {
      const RecipeResultBinding b{binding.generation,binding.start_frame,binding.collection,binding.end_sequence,
        binding.world,binding.hero,binding.station,request.hands.hands.right};
      started=base_.begin(*d->base,{expected.data(),d->targets.count},b);
    } else if(d->route==crafting_catalog::Route::station) {
      StationRecipeMaterial material;
      material.inventory_known=material.hand_known=material.craftable=true;
      material.maximum=s.maximum;material.quantity=request.quantity;material.count=d->targets.count;
      material.completion=d->station_descriptor->completion;
      const StationRecipeResultBinding b{binding.generation,binding.start_frame,binding.collection,binding.end_sequence,
        binding.world,binding.hero,binding.station,request.hands};
      started=station_.begin(*d->station_descriptor,material,{rows.data(),d->targets.count},b);
    } else if(crafting_result_detail::separate_ended_route(d->route))
      started=ended_.begin(*d,{expected.data(),d->targets.count},binding);
    if(!started)return bad_contract();
    predebit_=prepared?*prepared:cool_predebit::admission::Prepared{};predebit_witness_={};
    request_=request;binding_=binding;route_=d->route;
    production_=crafting_result_detail::production(*d);
    last_frame_=binding.start_frame;end_frame_=0;has_end_=restore_verified_=false;
    destroy_verified_=false;destroy_frame_=0;
    skill_verified_=false;skill_frame_=0;grind_expected_={};skill_increment_=0;
    if(d->grind){const auto numerical=crafting_grind_plan(s,request.quantity);
      if(!numerical.valid)return bad_contract();
      grind_expected_=s.grind;grind_expected_.skill=numerical.skill_after;skill_increment_=numerical.skill_increment;}
    success_={};issue_=CraftingResultIssue::none;state_=CraftingResultState::waiting;return true;
  }
public:
  bool restored(bool verified,const CraftingResultExtraRestore& extra={}) noexcept {
    if(state_==CraftingResultState::failed)return false;
    if(!verified||predebit_.valid()){fail(CraftingResultIssue::restore);return false;}
    if(state_!=CraftingResultState::waiting)return false;
    if((route_==crafting_catalog::Route::alchemy_extra||route_==crafting_catalog::Route::spirit)&&
      (extra.generation!=binding_.generation||extra.recipe!=request_.id||extra.quantity!=request_.quantity||
       !extra.writer_result_verified||!extra.item_restore_verified)){
      fail(CraftingResultIssue::restore);return false;
    }
    restore_verified_=true;return true;
  }
  bool end_after(const CraftingResultEndWitness& value) noexcept {
    if(state_!=CraftingResultState::waiting||!value.normal_exit_verified||has_end_||
      value.frame<last_frame_)return false;
    if(predebit_.valid()&&!destroy_verified_){fail(CraftingResultIssue::destroy);return false;}
    if((route_==crafting_catalog::Route::cool||route_==crafting_catalog::Route::grind)&&
      destroy_verified_&&value.frame<destroy_frame_){
      fail(CraftingResultIssue::destroy);return false;
    }
    if(route_==crafting_catalog::Route::grind&&skill_verified_&&value.frame<skill_frame_){
      fail(CraftingResultIssue::skill);return false;
    }
    bool accepted=false;
    if(route_==crafting_catalog::Route::base)
      accepted=base_.end_after({value.after_original,value.generation,value.sequence,value.frame,value.world,value.hero,value.station});
    else if(route_==crafting_catalog::Route::station)
      accepted=station_.end_after({value.after_original,value.normal_exit_verified,value.generation,value.sequence,value.frame,
        value.world,value.hero,value.station});
    else if(crafting_result_detail::separate_ended_route(route_))accepted=ended_.end_after(value);
    if(accepted){has_end_=true;end_frame_=value.frame;}
    return accepted;
  }
  bool destroy_after(const CraftingResultDestroyWitness& value) noexcept {
    if((route_!=crafting_catalog::Route::cool&&route_!=crafting_catalog::Route::grind)||
      state_==CraftingResultState::idle||state_==CraftingResultState::failed)return false;
    if(state_!=CraftingResultState::waiting||destroy_verified_||!value.after_original||!value.normal_remove_verified||
      !(value.key==request_.key)||value.recipe!=request_.id||value.quantity!=request_.quantity||
      (predebit_.valid()&&(!restore_verified_||value.serial<=predebit_witness_.original_serial))||
      !value.serial||value.frame<binding_.start_frame||(has_end_&&value.frame>end_frame_)||
      value.instance!=(route_==crafting_catalog::Route::cool?16612:grind_recipe::lifecycle_item)||value.amount!=1){
      fail(CraftingResultIssue::destroy);return false;
    }
    destroy_verified_=true;destroy_frame_=value.frame;return true;
  }
  bool skill_after(const CraftingResultSkillWitness& value) noexcept {
    if(route_!=crafting_catalog::Route::grind||state_==CraftingResultState::idle||state_==CraftingResultState::failed)return false;
    if(state_!=CraftingResultState::waiting||skill_verified_||!value.after_original||!value.original_progress_verified||
      !(value.key==request_.key)||value.recipe!=request_.id||value.quantity!=request_.quantity||!value.serial||
      value.frame<binding_.start_frame||(has_end_&&value.frame>end_frame_)||
      !(value.before==request_.material_snapshot.grind)||!(value.after==grind_expected_)){
      fail(CraftingResultIssue::skill);return false;
    }
    skill_verified_=true;skill_frame_=value.frame;return true;
  }
  CraftingResultState observe(const CraftingResultObservation& value) noexcept {
    if(state_!=CraftingResultState::waiting||value.generation<binding_.generation)return state_;
    if(value.generation!=binding_.generation)return fail(CraftingResultIssue::contract);
    if(!value.stable_boundary_verified||value.frame<=binding_.start_frame||value.frame<last_frame_)return state_;
    last_frame_=value.frame;
    if(value.identity_known&&(value.world!=binding_.world||value.hero!=binding_.hero))return fail(CraftingResultIssue::context);
    if(value.station_known&&value.station&&value.station!=binding_.station)return fail(CraftingResultIssue::context);
    if(value.production_known&&value.production&&value.production!=production_)return fail(CraftingResultIssue::context);
    if(route_==crafting_catalog::Route::spirit){
      const auto& b=value.branch;
      if(b.known&&b.value!=request_.material_snapshot.branch_value)return fail(CraftingResultIssue::context);
      if(b.writer_result_verified&&(!b.known||b.generation!=binding_.generation||
        b.recipe!=request_.id||b.quantity!=request_.quantity))return fail(CraftingResultIssue::contract);
    }
    if(route_==crafting_catalog::Route::grind&&value.grind.known&&!(value.grind==grind_expected_))
      return fail(CraftingResultIssue::skill);
    // Stronger than the old base watcher: normal AFTER is for each
    // route only in a later frame a finished animation/hand follow-up.
    if(has_end_&&value.frame<=end_frame_)return state_;
    bool verified=false;
    if(route_==crafting_catalog::Route::base) {
      RecipeResultObservation v;
      copy_observation(value,v);v.items=value.items;v.hands=value.hands.hands;
      auto next=base_;const auto state=next.observe(v);
      if(state==RecipeResultState::failed)return fail(convert(next.issue()));
      // The old base watcher knows only the two hand objects. The shared
      // normal end contract additionally requires the CURRENT zero anchor
      // here; an earlier end AFTER does not replace a later read receipt. Ready
      // at the hearth continues to master only its actually bound hand ABI.
      const bool ended=has_end_&&value.station_known&&!value.station&&value.production_known&&!value.production;
      const bool anchor_known=value.hands_known&&value.hands.interact_item_known;
      if(ended&&anchor_known&&value.hands.interact_item)return fail(CraftingResultIssue::anchor);
      const bool anchor_verified=!ended||anchor_known;
      if(state!=RecipeResultState::verified||(restore_verified_&&anchor_verified))base_=next;
      verified=state==RecipeResultState::verified&&anchor_verified;
    } else if(route_==crafting_catalog::Route::station) {
      StationRecipeResultObservation v;
      copy_observation(value,v);v.hands=value.hands;
      std::array<StationRecipeResultItem,crafting_catalog::target_capacity> items{};
      if(value.items.data&&value.items.count<=items.size()) {
        for(std::size_t i=0;i<value.items.count;++i)items[i]={value.items[i].symbol,value.items[i].total};
        v.items={items.data(),value.items.count};
      } else v.items={nullptr,value.items.count};
      auto next=station_;const auto state=next.observe(v);
      if(state==StationRecipeResultState::failed)return fail(convert(next.issue()));
      if(state!=StationRecipeResultState::verified||restore_verified_)station_=next;
      verified=state==StationRecipeResultState::verified;
    } else if(crafting_result_detail::separate_ended_route(route_)) {
      auto next=ended_;const auto state=next.observe(value);
      if(state==CraftingResultState::failed)return fail(next.issue());
      // A still-missing spirit end receipt must not prematurely
      // freeze the inner stock/hand watcher. The later frame is fully
      // re-checked, just as with a still-missing restore receipt.
      const bool spirit_end=route_!=crafting_catalog::Route::spirit||
        (value.branch.known&&value.branch.writer_result_verified);
      const bool cool_end=route_!=crafting_catalog::Route::cool||destroy_verified_;
      const bool grind_end=route_!=crafting_catalog::Route::grind||
        (destroy_verified_&&skill_verified_&&value.grind==grind_expected_);
      if(state!=CraftingResultState::verified||(restore_verified_&&spirit_end&&cool_end&&grind_end))ended_=next;
      verified=state==CraftingResultState::verified;
    } else return fail(CraftingResultIssue::contract);
    if(verified) {
      // Without restore the inner watcher also stays waiting. The
      // same or a later intake is fully re-checked after the restore.
      // Positive contradictions were still evaluated uncovered before.
      if(!restore_verified_)return state_;
      if(route_==crafting_catalog::Route::spirit&&(!value.branch.known||!value.branch.writer_result_verified))return state_;
      if(route_==crafting_catalog::Route::cool&&!destroy_verified_)return state_;
      if(route_==crafting_catalog::Route::grind&&
        (!destroy_verified_||!skill_verified_||!(value.grind==grind_expected_)))return state_;
      if(!copy_success(value))return fail(CraftingResultIssue::stock);
      state_=CraftingResultState::verified;
    }
    return state_;
  }
  CraftingResultState timeout() noexcept {
    return state_==CraftingResultState::waiting?fail(CraftingResultIssue::timeout):state_;
  }
  CraftingResultState fail(CraftingResultIssue issue) noexcept {
    if(state_!=CraftingResultState::failed) {
      issue_=issue==CraftingResultIssue::none?CraftingResultIssue::contract:issue;
      state_=CraftingResultState::failed;success_={};restore_verified_=false;
      base_.fail(RecipeResultIssue::contract);station_.fail(StationRecipeResultIssue::contract);
      ended_.fail(CraftingResultIssue::contract);
    }
    return state_;
  }
  CraftingResultState state() const noexcept {return state_;}
  CraftingResultIssue issue() const noexcept {return issue_;}
  bool permits_next() const noexcept {
    return state_==CraftingResultState::idle||(state_==CraftingResultState::verified&&
      (!success_.quantity_receipt_required||success_.quantity_receipt_verified));
  }
  // Material success remains readable for correct display. A mandatory
  // receipt is only bound from the real submitted_receipt of own UI;
  // success_notice_opened or an original print does not produce this value.
  bool acknowledge_quantity_receipt(std::uint64_t submitted_receipt) noexcept {
    if(state_!=CraftingResultState::verified||!success_.quantity_receipt_required||
      success_.quantity_receipt_verified||!submitted_receipt||submitted_receipt!=success_.receipt)return false;
    success_.quantity_receipt_verified=true;return true;
  }
  bool success(CraftingResultSuccess& out) const noexcept {
    out={};if(state_!=CraftingResultState::verified)return false;out=success_;return true;
  }
private:
  static bool valid_total(std::int64_t value) noexcept {return value>=0&&value<=INT32_MAX;}
  bool bad_contract() noexcept {fail(CraftingResultIssue::contract);return false;}
  template<class Observation>void copy_observation(const CraftingResultObservation& from,Observation& to) const noexcept {
    to.generation=from.generation;to.frame=from.frame;to.stable_boundary_verified=from.stable_boundary_verified;
    to.identity_known=from.identity_known;to.world=from.world;to.hero=from.hero;
    to.station_known=from.station_known;to.station=from.station;
    to.production_known=from.production_known;to.production=from.production;
    to.selection_known=from.selection_known&&from.selection_owner_verified;
    to.selection_open=from.selection_open;to.selection_waiting=from.selection_waiting;to.selection_done=from.selection_done;
    to.collection_known=from.collection_known;to.collection=from.collection;
    to.stock_known=from.stock_known;to.hands_known=from.hands_known;
  }
  static CraftingResultIssue convert(RecipeResultIssue issue) noexcept {
    switch(issue){case RecipeResultIssue::context:return CraftingResultIssue::context;
      case RecipeResultIssue::stock:return CraftingResultIssue::stock;
      case RecipeResultIssue::hands:return CraftingResultIssue::hands;
      case RecipeResultIssue::timeout:return CraftingResultIssue::timeout;
      case RecipeResultIssue::restore:return CraftingResultIssue::restore;
      default:return CraftingResultIssue::contract;}
  }
  static CraftingResultIssue convert(StationRecipeResultIssue issue) noexcept {
    switch(issue){case StationRecipeResultIssue::context:return CraftingResultIssue::context;
      case StationRecipeResultIssue::stock:return CraftingResultIssue::stock;
      case StationRecipeResultIssue::hands:return CraftingResultIssue::hands;
      case StationRecipeResultIssue::anchor:return CraftingResultIssue::anchor;
      case StationRecipeResultIssue::timeout:return CraftingResultIssue::timeout;
      case StationRecipeResultIssue::restore:return CraftingResultIssue::restore;
      default:return CraftingResultIssue::contract;}
  }
  bool copy_success(const CraftingResultObservation& value) noexcept {
    const auto* d=crafting_catalog::find(request_.id);
    if(!d||!value.stock_known||!value.items.data||value.items.count!=d->targets.count)return false;
    CraftingResultSuccess out;
    out.result_verified=out.restore_verified=true;out.receipt=binding_.generation;out.frame=value.frame;
    out.key=request_.key;out.id=request_.id;out.quantity=request_.quantity;
    out.quantity_receipt_required=crafting_quantity_receipt_required(request_.id,request_.quantity);
    out.terminal=has_end_?CraftingResultTerminal::ended:CraftingResultTerminal::ready;
    const auto& selected=request_.material_snapshot;
    out.branch_known=selected.branch_known;out.branch_value=selected.branch_value;out.branch_output=selected.branch_output;
    if(d->grind){out.grind_before=selected.grind;out.grind_after=value.grind;out.skill_increment=skill_increment_;}
    const auto outputs=crafting_catalog::outputs_for(*d,selected.branch_known,selected.branch_value);
    out.stock_count=d->targets.count;out.produced_count=outputs.count;
    for(std::size_t i=0;i<d->targets.count;++i) {
      const auto symbol=d->targets[i].symbol;const auto* before=crafting_detail::material(request_.material_snapshot,symbol);
      const RecipeResultItem* after=nullptr;
      for(std::size_t j=0;j<value.items.count;++j)if(value.items[j].symbol==symbol){if(after)return false;after=&value.items[j];}
      if(!before||!after||!valid_total(after->total))return false;
      out.stocks[i]={symbol,static_cast<std::int32_t>(after->total)};
      out.deltas[i]={symbol,before->stored,after->total,after->total-before->stored};
    }
    for(std::size_t i=0;i<outputs.count;++i) {
      const auto& output=outputs[i];const CraftingResultDelta* actual=nullptr;
      for(std::size_t j=0;j<out.stock_count;++j)if(out.deltas[j].symbol==output.symbol)actual=&out.deltas[j];
      const auto expected=std::int64_t{output.units}*request_.quantity;
      if(!actual||actual->delta<=0||actual->delta!=expected||!valid_total(actual->delta))return false;
      out.produced[i]={output.symbol,static_cast<std::int32_t>(actual->delta)};
    }
    success_=out;return true;
  }
  RecipeResult<crafting_catalog::target_capacity> base_;
  StationRecipeResult<crafting_catalog::target_capacity> station_;
  crafting_result_detail::EndedGuard ended_;
  CraftingResultState state_=CraftingResultState::idle;
  CraftingResultIssue issue_=CraftingResultIssue::none;
  crafting_catalog::Route route_=crafting_catalog::Route::base;
  CraftingRequest request_{};
  CraftingResultBinding binding_{};
  CraftingResultSuccess success_{};
  cool_predebit::admission::Prepared predebit_{};cool_predebit::Witness predebit_witness_{};
  std::uint64_t last_frame_=0,end_frame_=0,destroy_frame_=0,skill_frame_=0;
  std::int32_t production_=0;
  bool restore_verified_=false,has_end_=false,destroy_verified_=false,skill_verified_=false;
  CraftingGrindValues grind_expected_{};
  std::uint32_t skill_increment_=0;
};
} // namespace ubc
