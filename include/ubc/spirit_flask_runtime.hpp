// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/spirit_flask_debit.hpp"
#include "ubc/spirit_recipe_snapshot.hpp"
#include "ubc/alchemy_extra_dependency_reader.hpp"

namespace ubc::spirit_flask_runtime {
namespace debit=spirit_debit;namespace catalog=spirit_recipe_catalog;
namespace rt=alchemy_flask_runtime;namespace script=runtime_script;namespace detail=rt::detail;
using rt::Issue;using rt::Boundary;using rt::NativeCodePolicy;using rt::NativeEvidence;using rt::ScriptEvidence;
using rt::verify_native;using rt::verify_flask_constructor;using rt::RestoreCapture;
enum class ParserStackMode {empty,original_returns};
inline detail::ParserRead read_parser(const ReadOnlyMemory& memory,const debit::Context& context,
    ParserStackMode mode=ParserStackMode::empty) noexcept {
  const auto accepted=[&](const debit::ParserState& value) noexcept {
    return mode==ParserStackMode::empty?debit::valid(value,context):
      mode==ParserStackMode::original_returns&&debit::valid_after_original(value,context,20370);
  };
  auto first=detail::parser_once(memory,context,false);if(!first.valid)return first;
  if(!accepted(first.value)){first.valid=false;first.issue=Issue::parser;return first;}
  auto second=detail::parser_once(memory,context,false);if(!second.valid)return second;
  if(!accepted(second.value)){second.valid=false;second.issue=Issue::parser;return second;}
  if(!detail::same_parser(first,second)){second.valid=false;second.issue=Issue::changed;return second;}
  return second;
}
struct Capture {debit::Observation observation{};NativeEvidence native{};ScriptEvidence script{};
  PackedInventoryIssue inventory_issue=PackedInventoryIssue::none;StationHandReadIssue hand_issue=StationHandReadIssue::none;
  bool valid=false;Issue issue=Issue::none;};
// Same bounded native/parser/flask primitives; own genuine seven-target
// read. CoreReader alone does NOT prove external dependency closure.
class CoreReader {
public:
  explicit CoreReader(std::uint32_t owner_thread) noexcept : owner_thread_(owner_thread) {}
  CoreReader(const CoreReader&)=delete;CoreReader& operator=(const CoreReader&)=delete;
  Capture capture(const ReadOnlyMemory& memory,const Boundary& boundary,std::int32_t expected_branch,
      const NativeCodePolicy& policy={},ParserStackMode stack_mode=ParserStackMode::empty) noexcept {
    const auto& recipe=catalog::recipes[0];
    Capture out;out.observation.context=boundary.context;out.observation.scope=boundary.scope;out.observation.serial=boundary.serial;
    if(!owner_thread_ || boundary.current_thread!=owner_thread_ || boundary.context.thread!=owner_thread_) {out.issue=Issue::thread;return out;}
    if(reading_) {out.issue=Issue::reentry;return out;}
    if(!detail::boundary(boundary,owner_thread_)) {out.issue=Issue::boundary;return out;}
    if(!catalog::structurally_valid(recipe) || recipe.callback->symbol!=boundary.scope.callback) {out.issue=Issue::descriptor;return out;}
    reading_=true;struct Scope {bool& value;~Scope(){value=false;}} guard{reading_};
    out.native=verify_native(memory,policy);out.script=verify_flask_constructor(memory);
    auto first=read_once(memory,recipe,boundary,expected_branch,stack_mode);if(!first.valid) {first.native=out.native;first.script=out.script;return first;}
    auto second=read_once(memory,recipe,boundary,expected_branch,stack_mode);if(!second.valid) {second.native=out.native;second.script=out.script;return second;}
    out.observation=second.observation;
    if(!debit::same_stocks(first.observation.stocks,second.observation.stocks) ||
       !detail::same_flask(first.observation.flask,second.observation.flask) ||
       !debit::same(first.observation.hand,second.observation.hand) ||
       !debit::same(first.observation.parser,second.observation.parser) ||
       first.observation.branch_value!=second.observation.branch_value || first.observation.production!=second.observation.production || first.observation.invincible!=second.observation.invincible) {out.issue=Issue::changed;return out;}
    const auto native_after=verify_native(memory,policy);const auto script_after=verify_flask_constructor(memory);
    if(!out.native.reviewed_ranges_match || !out.native.vtable_slots_match) {out.issue=out.native.issue;return out;}
    if(!native_after.reviewed_ranges_match || !native_after.vtable_slots_match) {out.native=native_after;out.issue=native_after.issue;return out;}
    if(!out.script.constructor_and_bindings_match || !out.script.strings_match) {out.issue=out.script.issue;return out;}
    if(!script_after.constructor_and_bindings_match || !script_after.strings_match) {out.script=script_after;out.issue=script_after.issue;return out;}
    if(!detail::context(memory,boundary.context) || !detail::lab(memory,boundary.context)) {out.issue=Issue::context;return out;}
    if(!boundary.native_dependencies_and_configured_targets_verified) {out.issue=Issue::dependency_evidence;return out;}
    out.observation.native_contract=true;out.observation.stable_read=true;out.valid=true;return out;
  }
  RestoreCapture restore_targets(const ReadOnlyMemory& memory,const debit::Call& call,const Boundary& boundary,
      const NativeCodePolicy& policy={}) noexcept {
    RestoreCapture out;
    if(!owner_thread_ || boundary.current_thread!=owner_thread_ || boundary.context.thread!=owner_thread_) {out.issue=Issue::thread;return out;}
    if(reading_) {out.issue=Issue::reentry;return out;}
    if(!detail::boundary(boundary,owner_thread_) || boundary.scope.debit_depth!=1 || !call.valid ||
       !debit::valid_after_original(call.parser,call.context,call.callback) || !debit::same(call.context,boundary.context)) {out.issue=Issue::boundary;return out;}
    reading_=true;struct Scope {bool& value;~Scope(){value=false;}} guard{reading_};
    detail::ParserRead first;
    for(unsigned pass=0;pass<2;++pass) {
      if(!detail::context(memory,boundary.context) || !detail::lab(memory,boundary.context)) {out.issue=Issue::context;return out;}
      // Cursor/UseInstance may diverge after the internal split. The
      // stored write targets are instead re-bound via symbol indices.
      const auto p=detail::parser_once(memory,boundary.context,false);
      if(!p.valid) {out.issue=p.issue;return out;}
      // The direct native split does not invoke CallFunc.Clear. A
      // diverging prefix (even SEH residuals) is not a repair order.
      if(!debit::base::same_data_stack(call.parser,p.value)) {out.issue=Issue::parser;return out;}
      if(pass&&!detail::same_parser(first,p)) {out.issue=Issue::changed;return out;}
      first=p;
      const auto& value=p.value;debit::RestoreObservation current;
      current.context=boundary.context;current.scope=boundary.scope;
      current.current_parser=value.current_parser;current.code_base=value.code_base;current.code_size=value.code_size;
      current.self_symbol=value.self_symbol;current.hero_symbol=value.hero_symbol;
      current.self=value.self;current.hero=value.hero;current.other=value.other;current.victim=value.victim;
      if(call.parser.use_symbol==value.self_symbol) {current.saved_use_symbol=value.self_symbol;current.saved_use_address=value.self;}
      else if(call.parser.use_symbol==value.hero_symbol) {current.saved_use_symbol=value.hero_symbol;current.saved_use_address=value.hero;}
      else {out.issue=Issue::parser;return out;}
      const auto native=verify_native(memory,policy);
      current.native_restore_contract=native.reviewed_ranges_match && native.vtable_slots_match && boundary.native_dependencies_and_configured_targets_verified;
      current.known=true;
      if(!debit::restorable(call,current)) {out.issue=current.native_restore_contract?Issue::parser:Issue::dependency_evidence;return out;}
      out.observation=current;
    }
    out.valid=true;return out;
  }
private:
  Capture read_once(const ReadOnlyMemory& memory,const catalog::Descriptor& recipe,const Boundary& boundary,std::int32_t expected_branch,ParserStackMode stack_mode) noexcept {
    Capture out;auto& v=out.observation;v.context=boundary.context;v.scope=boundary.scope;v.serial=boundary.serial;
    const auto profile=spirit_recipe_runtime::assemble(recipe);
    const auto branch=spirit_recipe_runtime::branch(memory,boundary.context.parser,profile);
    if(!branch.read_valid||branch.value!=expected_branch){out.issue=Issue::script;return out;}
    v.branch_known=true;v.branch_value=branch.value;
    if(!detail::context(memory,boundary.context)) {out.issue=Issue::context;return out;}
    if(!detail::lab(memory,boundary.context)) {out.issue=Issue::station;return out;}
    const auto p=read_parser(memory,boundary.context,stack_mode);if(!p.valid) {out.issue=p.issue;return out;}v.parser=p.value;
    const auto production=script::symbol_int(memory,boundary.context.parser,12330);
    std::uint32_t invincible=0;
    if(!production.valid || !detail::word(memory,boundary.context.hero,0x298,invincible) || invincible>1) {out.issue=Issue::script;return out;}
    v.production=production.value;v.invincible=static_cast<std::int32_t>(invincible);
    const auto stock=read_packed_inventory<catalog::target_capacity>(memory,boundary.context.hero,recipe.targets.data,recipe.targets.count,true,inventory_);
    out.inventory_issue=stock.issue;
    if(!stock.storage_totals_known || !stock.snapshot_unchanged || stock.issue!=PackedInventoryIssue::none) {out.issue=Issue::inventory;return out;}
    out.hand_issue=station_recipe_snapshot_detail::begin_hands(memory,boundary.context.hero,hands_);
    const auto& hand=hands_.before;
    if(out.hand_issue!=StationHandReadIssue::none || !hand.left_slot_known || !hand.right_slot_known ||
       hand.hands.left.state!=0 || hand.hands.right.state!=1 || hand.hands.right.symbol!=16604 || hand.hands.right.amount!=1 ||
       !hand.interact_item_known || hand.interact_item!=hand.hands.right.object) {out.issue=Issue::hands;return out;}
    v.hand={true,true,hand.hands.right.object,16604,1,hand.interact_item};
    v.stocks.count=recipe.targets.count;std::size_t flask_index=recipe.targets.count;
    for(std::size_t i=0;i<recipe.targets.count;++i) {
      const auto symbol=recipe.targets[i].symbol;
      if(recipe_material_detail::units(recipe.costs,symbol) && (stock.listed_occurrences[i]>1 || stock.packed_occurrences[i]>1 ||
         stock.duplicate_packed_target[i] || stock.has_active_marker[i] || stock.mixed_representation[i] ||
         (stock.listed_occurrences[i] && stock.packed_occurrences[i]))) {out.issue=Issue::inventory;return out;}
      v.stocks.rows[i]={static_cast<std::uint32_t>(symbol),stock.total[i],true};
      if(symbol==16604) flask_index=i;
    }
    if(flask_index==recipe.targets.count) {out.issue=Issue::descriptor;return out;}
    v.flask.known=true;v.flask.packed=stock.packed[flask_index];v.flask.listed_stacks=stock.listed_occurrences[flask_index];
    for(std::size_t i=0;i<stock.listed_nodes;++i) {
      const auto& node=inventory_.records[i];const auto object=script::detail::u32(node.node.data()+4);
      const auto symbol=station_detail::little_i32(node.item_fields.data()+4);
      if(object==v.hand.right) {out.issue=Issue::alias;return out;}
      if(!recipe_material_detail::units(recipe.costs,symbol)) continue;
      std::uint32_t type=0,flags=0;
      if(!detail::word(memory,object,0,type) || type!=hand_detail::item_vtable || !detail::word(memory,object,0x158,flags) ||
         !(flags&0x00200000u) || (flags&0x40000000u)) {out.issue=Issue::flask;return out;}
      for(std::size_t slot=0;slot<hands_.count;++slot)
        if(script::detail::u32(hands_.records[slot].fields.data()+0x30)==object) {out.issue=Issue::alias;return out;}
      if(symbol==16604) {v.flask.object=object;v.flask.amount=script::detail::u32(node.item_fields.data());v.flask.flags=flags;}
    }
    v.flask.all_slot_aliases_excluded=true;v.flask.native_item_contract=true;
    v.stocks.complete_unambiguous=true;
    if(!station_recipe_snapshot_detail::verify_hands(memory,hands_)) {out.issue=Issue::changed;return out;}
    if(!detail::context(memory,boundary.context) || !detail::lab(memory,boundary.context)) {out.issue=Issue::context;return out;}
    const auto after=read_parser(memory,boundary.context,stack_mode);
    if(!after.valid || !detail::same_parser(p,after)) {out.issue=Issue::changed;return out;}
    const auto branch_after=spirit_recipe_runtime::branch(memory,boundary.context.parser,profile);
    if(!spirit_recipe_runtime::same_branch_evidence(branch,branch_after)){out.issue=Issue::changed;return out;}
    out.valid=true;return out; // Partial capture only; public capture sets the release bits only at the very end.
  }
  const std::uint32_t owner_thread_;
  bool reading_=false;
  PackedInventoryWorkspace inventory_{};
  station_recipe_snapshot_detail::HandWorkspace hands_{};
};
namespace dependencies=alchemy_extra_dependencies;
namespace common=alchemy_extra_dependency_reader;
using Input=common::Input;using Phase=common::Phase;
struct GuardedCapture {Capture runtime{};common::Guard guard{};dependencies::ListEvidence list{};bool valid=false;};
class Reader {
public:
  explicit Reader(std::uint32_t owner) noexcept:owner_(owner),core_(owner),list_(owner){}
  Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
  GuardedCapture capture(const ReadOnlyMemory& memory,const Input& in,Phase phase,std::int32_t branch) noexcept {
    GuardedCapture out;
    if(reading_){out.guard.issue=dependencies::Issue::reentry;return out;}
    if((phase!=Phase::before_original&&phase!=Phase::after_original&&phase!=Phase::before_debit&&phase!=Phase::after_debit)||
       in.boundary.scope.callback!=20370||in.boundary.scope.debit_depth!=(phase==Phase::after_debit?1u:0u)||
       in.quantity>catalog::maximum_quantity){out.guard.issue=dependencies::Issue::context;return out;}
    reading_=true;struct Lease{bool& b;~Lease(){b=false;}}lease{reading_};
    out.guard=common::verify_guard(memory,in,owner_);if(!out.guard.valid)return out;
    auto boundary=in.boundary;boundary.native_dependencies_and_configured_targets_verified=true;
    out.runtime=core_.capture(memory,boundary,branch,common::code_policy(in.modules),
      phase==Phase::before_original?ParserStackMode::empty:ParserStackMode::original_returns);if(!out.runtime.valid)return out;
    if(in.quantity>1&&phase!=Phase::after_debit){const auto& flask=out.runtime.observation.flask;
      if(!flask.known||flask.packed||flask.listed_stacks!=1||!flask.object){out.guard.issue=dependencies::Issue::flask;return out;}
      out.list=list_.capture(memory,in.boundary.current_thread,{in.boundary.context.hero,flask.object,in.quantity-1,in.boundary.context.world});
      if(!out.list.stable_complete||!out.list.unpack_unreachable||(out.list.path==dependencies::RemovalPath::whole&&!out.list.whole_short_path))return out;
      if(out.list.amount!=flask.amount){out.guard.issue=dependencies::Issue::changed;return out;}
    }
    const auto after=common::verify_guard(memory,in,owner_);
    if(!after.valid||!dependencies::same(out.guard.item,after.item)||out.guard.code.zbug_unpack_call!=after.code.zbug_unpack_call||
       out.guard.code.zpe_init_call!=after.code.zpe_init_call){out.guard=after;out.guard.valid=false;out.guard.issue=dependencies::Issue::changed;return out;}
    out.guard=after;out.valid=true;return out;
  }
  common::RestoreCapture restore_targets(const ReadOnlyMemory& memory,const debit::Call& call,const Input& in) noexcept {
    common::RestoreCapture out;
    if(reading_){out.guard.issue=dependencies::Issue::reentry;return out;}
    reading_=true;struct Lease{bool& b;~Lease(){b=false;}}lease{reading_};
    if(call.callback!=20370||in.quantity!=call.quantity+1u||in.boundary.scope.debit_depth!=1){out.guard.issue=dependencies::Issue::context;return out;}
    out.guard=common::verify_guard(memory,in,owner_);if(!out.guard.valid)return out;
    auto boundary=in.boundary;boundary.native_dependencies_and_configured_targets_verified=true;
    out.runtime=core_.restore_targets(memory,call,boundary,common::code_policy(in.modules));if(!out.runtime.valid)return out;
    const auto after=common::verify_guard(memory,in,owner_);
    if(!after.valid||!dependencies::same(out.guard.item,after.item)){out.guard=after;out.guard.valid=false;out.guard.issue=dependencies::Issue::changed;return out;}
    out.guard=after;out.valid=true;return out;
  }
private:
  std::uint32_t owner_=0;bool reading_=false;CoreReader core_;dependencies::ListReader list_;
};
} // namespace ubc::spirit_flask_runtime
