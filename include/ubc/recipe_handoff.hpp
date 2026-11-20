// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/hand_snapshot.hpp"
#include "ubc/recipe_navigation.hpp"
#include "ubc/recipe_session.hpp"

namespace ubc {

enum class HandoffState {
  idle, awaiting_step, ticket_ready, dispatched, executing, awaiting_collection,
  quarantined, cancelled, result_pending, finished
};
enum class HandoffIssue {
  none, request, plan, boundary, context, page, collection, resources, hands,
  focus, cancelled, timeout, ticket, offer, caller, reentry, counter
};
enum class HandoffDecision { ordinary, block, execute };

struct HandoffOffer {
  bool valid = false;
  recipe_catalog::Invocation mode = recipe_catalog::Invocation::info;
  std::int32_t owner = -1, function = -1, row = -1;
  std::uint64_t collection = 0;
};

// Only current, separately checked values. ready means original waiting,
// opened selection without IsDone/Animation, complete a full bound
// Collection cache. None of these statements follows from a hash.
struct HandoffObservation {
  StationKey key{};
  std::uint32_t view = 0;
  std::uint64_t frame = 0, collection = 0;
  bool context_known = false, page_known = false, complete = false, ready = false;
  bool outer_frame = false, quiescent = false, focused = false;
  recipe_navigation::Page page = recipe_navigation::Page::unknown;
  recipe_catalog::Invocation mode = recipe_catalog::Invocation::info;
  std::int32_t choice_owner = -1, count = 0;
  RecipeState materials{};
  HandsSnapshot hands{};
};

struct HandoffTicket {
  std::uint64_t id = 0, frame = 0;
  std::size_t step = 0;
  std::uint32_t view = 0;
  HandoffOffer offer{};
};
struct HandoffCallbackProof {
  bool own_thread = false, canonical_caller = false, native_update = false;
  unsigned update_depth = 0, callback_depth = 0, other_effect_depth = 0;
  unsigned collection_depth = 0, station_depth = 0;
};
struct HandoffCallback {
  HandoffDecision decision = HandoffDecision::ordinary;
  std::uint64_t ticket_id = 0, execution_id = 0;
};
struct HandoffEndWitness {
  StationKey key{}; // Identity of the positively finished OLD context.
  std::uint32_t view = 0;
  std::uint64_t frame = 0;
  bool confirmed = false, outer_frame = false, quiescent = false;
};

namespace recipe_handoff_detail {
namespace nav = recipe_navigation;
namespace cat = recipe_catalog;

inline bool same_offer(const HandoffOffer& a, const HandoffOffer& b) noexcept {
  return a.valid == b.valid && a.mode == b.mode && a.owner == b.owner &&
         a.function == b.function && a.row == b.row && a.collection == b.collection;
}
inline bool same_ticket(const HandoffTicket& a, const HandoffTicket& b) noexcept {
  return a.id && a.id == b.id && a.frame == b.frame && a.step == b.step && a.view == b.view && same_offer(a.offer, b.offer);
}
inline bool same_hand(const HandObjectSnapshot& a, const HandObjectSnapshot& b) noexcept {
  return a.state == b.state && a.symbol == b.symbol && a.amount == b.amount && a.object == b.object;
}
inline bool same_hands(const HandsSnapshot& a, const HandsSnapshot& b) noexcept {
  return same_hand(a.left,b.left) && same_hand(a.right,b.right);
}
inline bool valid_hands(const RecipeRequest& request, const HandsSnapshot& hands) noexcept {
  const auto* recipe = cat::find(request.id);
  return recipe && hands.left.state == 0 && !hands.left.object && hands.right.state == 1 && hands.right.object &&
         hands.right.symbol == recipe->hand.active_item_symbol &&
         hands.right.amount == static_cast<std::int32_t>(recipe->hand.amount);
}
inline bool same_resources(const RecipeState& a, const RecipeState& b) noexcept {
  if (!valid_recipe_state(a) || !valid_recipe_state(b) || a.id != b.id || a.known != b.known ||
      a.contract != b.contract || a.hand_known != b.hand_known || a.material_known != b.material_known ||
      a.maximum != b.maximum || a.target_count != b.target_count) return false;
  for (std::size_t i = 0; i < a.target_count; ++i) {
    const auto& row = a.materials[i];
    const auto* other = recipe_session_detail::material(b,row.symbol);
    if (!other || row.stored != other->stored || row.reserved != other->reserved ||
        row.unit_cost != other->unit_cost || row.unit_output != other->unit_output) return false;
  }
  return true;
}
inline bool valid_request(const RecipeRequest& request) noexcept {
  const auto* recipe = cat::find(request.id);
  const auto& material = request.material_snapshot;
  return recipe && recipe_session_detail::valid_key(request.key) && recipe->station == request.key.kind &&
         request.quantity && request.quantity <= material.maximum && material.id == request.id &&
         material.known && material.contract && material.material_known && material.hand_known &&
         valid_recipe_state(material) && cat::callback_for_quantity(*recipe,request.quantity);
}
inline bool same_step(const nav::Step& a, const nav::Step& b) noexcept {
  return a.kind == b.kind && a.invocation == b.invocation && a.owner_instance == b.owner_instance &&
         a.function_symbol == b.function_symbol && a.before == b.before && a.after == b.after;
}
} // namespace recipe_handoff_detail

// Herd info has no alchemy side. unknown is here explicitly only the
// separately evidenced STOVE info dialog; it is not output as an alchemy root.
inline recipe_navigation::Plan make_direct_info_plan(const RecipeRequest& request) noexcept {
  namespace nav = recipe_navigation;
  namespace cat = recipe_catalog;
  nav::Plan plan;
  plan.target = request.id; plan.quantity = request.quantity;
  const auto* recipe = cat::find(request.id);
  if (!recipe || recipe->station != cat::Station::stove || !cat::structurally_valid(*recipe)) return plan;
  const auto* callback = cat::callback_for_quantity(*recipe,request.quantity);
  if (!callback || callback->invocation != cat::Invocation::info) return plan;
  plan.steps[0] = {nav::Kind::effect,cat::Invocation::info,callback->owner_instance,
                  static_cast<std::int32_t>(callback->function->symbol),nav::Page::unknown,nav::Page::unknown};
  plan.count = 1; plan.issue = nav::Issue::none;
  return plan;
}

// Only used on the bound game thread. Proof fields replace no native
// adapter thread synchronization. This one resolves offers fresh,
// checks ABI/Body hashes and calls Select/HandleEvent or the original.
// The core itself performs no engine action and holds no pointers.
class RecipeHandoff {
public:
  bool begin(const RecipeRequest& request, const recipe_navigation::Plan& plan,
             const HandoffObservation& initial, std::uint64_t max_step_frames = 600) noexcept {
    if (active() || !request.key.generation || request.key.generation <= last_generation_) return false;
    last_generation_ = request.key.generation;
    request_ = request; plan_ = plan; view_ = initial.view; hands_ = initial.hands;
    step_ = 0; ticket_ = {}; callback_active_ = returned_ = cancelled_ = effect_started_ = false;
    execution_id_ = 0; callback_frame_ = 0;
    last_frame_ = step_frame_ = initial.frame; frame_limit_ = max_step_frames;
    state_ = HandoffState::awaiting_step; issue_ = HandoffIssue::none;
    if (!recipe_handoff_detail::valid_request(request)) { cancel(HandoffIssue::request); return false; }
    if (!valid_plan()) { cancel(HandoffIssue::plan); return false; }
    if (!frame_limit_ || !initial.frame || !initial.outer_frame || !initial.quiescent) {
      cancel(HandoffIssue::boundary); return false;
    }
    if (!check_common(initial) || !ready_page(initial,plan.steps[0].before)) {
      if (!cancelled_) cancel(HandoffIssue::page);
      return false;
    }
    return true;
  }

  void observe(const HandoffObservation& value) noexcept {
    if (!active() || state_ == HandoffState::result_pending) return;
    if (!value.outer_frame || !value.quiescent || callback_active_) { cancel(HandoffIssue::boundary); return; }
    if (!value.frame || value.frame < last_frame_) { cancel(HandoffIssue::boundary); return; }
    last_frame_ = value.frame;
    if (state_ == HandoffState::quarantined) {
      // No semantic recipe release needed, only positive
      // evidence that the old IsDone/Close cannot be repeated.
      if (same_scope(value) && value.complete && value.ready && value.count >= 0 && value.count <= 256 &&
          value.collection > ticket_.offer.collection &&
          value.frame > ticket_.frame) {
        state_ = HandoffState::cancelled; ticket_ = {};
      }
      return;
    }
    if (!check_common(value)) return;
    if (state_ == HandoffState::awaiting_collection && returned_ &&
        value.frame > callback_frame_ && value.collection > ticket_.offer.collection && value.complete && value.ready) {
      if (!ready_page(value,plan_.steps[step_].after)) { cancel(HandoffIssue::page); return; }
      ++step_;
      if (step_ >= plan_.count) { cancel(HandoffIssue::plan); return; }
      ticket_ = {}; returned_ = false; step_frame_ = value.frame;
      state_ = HandoffState::awaiting_step;
    }
  }

  std::optional<HandoffTicket> ticket(const HandoffObservation& value, const HandoffOffer& offer) noexcept {
    observe(value);
    if (state_ != HandoffState::awaiting_step) return std::nullopt;
    if (!ready_page(value,plan_.steps[step_].before)) { cancel(HandoffIssue::page); return std::nullopt; }
    if (!matches_offer(value,offer)) { cancel(HandoffIssue::offer); return std::nullopt; }
    if (ticket_serial_ == UINT64_MAX) { cancel(HandoffIssue::counter); return std::nullopt; }
    ticket_ = {++ticket_serial_,value.frame,step_,view_,offer};
    state_ = HandoffState::ticket_ready;
    return ticket_;
  }

  bool dispatch(const HandoffTicket& ticket, const HandoffObservation& value, const HandoffOffer& fresh) noexcept {
    if (state_ != HandoffState::ticket_ready) return false;
    if (!recipe_handoff_detail::same_ticket(ticket_,ticket) || value.frame != ticket_.frame) {
      cancel(HandoffIssue::ticket); return false;
    }
    if (!value.outer_frame || !value.quiescent || !check_common(value) ||
        !ready_page(value,plan_.steps[step_].before) || !matches_offer(value,fresh) ||
        !recipe_handoff_detail::same_offer(ticket_.offer,fresh)) {
      if (!cancelled_) cancel(HandoffIssue::offer);
      return false;
    }
    // Consumed before native Select/Enter. An ignored/unclear
    // native input is never repaired by a second attempt.
    state_ = HandoffState::dispatched;
    return true;
  }

  // Non-consuming pre-check for the normal info-result watcher.
  // OnInfo/OnChoice must still individually pass enter_callback later.
  bool matches_pending(const HandoffTicket& ticket, const HandoffObservation& actual,
                       const HandoffOffer& offer) const noexcept {
    return state_ == HandoffState::dispatched && !cancelled_ && actual.complete &&
           recipe_handoff_detail::same_ticket(ticket_,ticket) && same_scope(actual) && actual.focused &&
           page_matches(actual,plan_.steps[step_].before) && matches_offer(actual,offer) &&
           recipe_handoff_detail::same_offer(ticket_.offer,offer) &&
           recipe_handoff_detail::same_resources(request_.material_snapshot,actual.materials) &&
           recipe_handoff_detail::same_hands(hands_,actual.hands) && actual.frame >= last_frame_ &&
           actual.frame - step_frame_ <= frame_limit_;
  }

  HandoffCallback enter_callback(const HandoffTicket& ticket, const HandoffObservation& actual,
                                const HandoffOffer& offer, const HandoffCallbackProof& proof) noexcept {
    if (!active()) return {};
    const HandoffCallback blocked{HandoffDecision::block,0,0};
    // A positive other owner rejects the remainder. A delayed old
    // callback remains bound, even with a different row/Collection.
    // Only a positively identified other callback is foreign; a
    // read gap is not such evidence. Before dispatch there was
    // no order to the engine and thus no old callback to protect.
    if (actual.context_known && actual.view && !same_scope(actual)) {
      const bool native_pending = ticket_.id && state_ != HandoffState::ticket_ready;
      const bool identified = offer.valid && offer.owner >= 0 && offer.function >= 0 &&
          (offer.mode == recipe_catalog::Invocation::info || offer.mode == recipe_catalog::Invocation::choice);
      const bool old_callback = offer.owner == ticket_.offer.owner && offer.function == ticket_.offer.function;
      cancel(HandoffIssue::context);
      return !native_pending || (identified && !old_callback) ? HandoffCallback{} : blocked;
    }
    if (!proof.own_thread || !proof.canonical_caller || !proof.native_update || proof.update_depth != 1 ||
        proof.callback_depth != 1 || proof.other_effect_depth || proof.collection_depth || proof.station_depth) {
      cancel(HandoffIssue::caller); return blocked;
    }
    if (callback_active_ || state_ == HandoffState::executing) { cancel(HandoffIssue::reentry); return blocked; }
    if (!matches_pending(ticket,actual,offer)) { cancel(HandoffIssue::ticket); return blocked; }
    if (execution_serial_ == UINT64_MAX) { cancel(HandoffIssue::counter); return blocked; }
    execution_id_ = ++execution_serial_;
    callback_frame_ = actual.frame; callback_active_ = true;
    effect_started_ = plan_.steps[step_].is_effect();
    state_ = HandoffState::executing;
    return {HandoffDecision::execute,ticket_.id,execution_id_};
  }

  bool callback_return(const HandoffCallback& witness) noexcept {
    if (witness.decision != HandoffDecision::execute) return false;
    if (!callback_active_ || witness.ticket_id != ticket_.id || witness.execution_id != execution_id_) return false;
    callback_active_ = false; returned_ = true;
    // Here neither check the next ticket nor the page: the original
    // update must have fully left its Collect-/Open follow-up.
    state_ = effect_started_ ? HandoffState::result_pending :
             cancelled_ ? HandoffState::quarantined : HandoffState::awaiting_collection;
    return true;
  }

  void cancel(HandoffIssue issue = HandoffIssue::cancelled) noexcept {
    if (state_ == HandoffState::idle || state_ == HandoffState::finished || state_ == HandoffState::cancelled) return;
    cancelled_ = true;
    if (issue_ == HandoffIssue::none) issue_ = issue;
    if (state_ == HandoffState::result_pending) return; // Already begun effect needs result verification.
    if (state_ == HandoffState::dispatched || state_ == HandoffState::executing ||
        state_ == HandoffState::awaiting_collection || state_ == HandoffState::quarantined || callback_active_)
      state_ = HandoffState::quarantined;
    else { state_ = HandoffState::cancelled; ticket_ = {}; }
  }

  bool context_ended(const HandoffEndWitness& witness) noexcept {
    if (!active() || state_ == HandoffState::result_pending || callback_active_ ||
        !witness.confirmed || !witness.outer_frame || !witness.quiescent || !witness.frame ||
        witness.frame < last_frame_ || !(witness.key == request_.key) || witness.view != view_) return false;
    cancelled_ = true;
    if (issue_ == HandoffIssue::none) issue_ = HandoffIssue::context;
    state_ = HandoffState::cancelled; ticket_ = {}; last_frame_ = witness.frame;
    return true;
  }

  // Only after the SEPARATE positive material/station completion check.
  // false and a mere CallbackReturn give no further production free.
  bool acknowledge_result(bool verified) noexcept {
    if (!verified || state_ != HandoffState::result_pending || callback_active_) return false;
    state_ = HandoffState::finished; ticket_ = {}; return true;
  }

  bool active() const noexcept {
    return state_ != HandoffState::idle && state_ != HandoffState::cancelled && state_ != HandoffState::finished;
  }
  // Even the native view can be reused in a later world/station.
  // Positive other ownership remains free even at the same
  // address; unknown ownership of the bound views remains guarded.
  bool guards_input(const HandoffObservation& actual) const noexcept {
    return active() && actual.view && actual.view == view_ &&
           (!actual.context_known || actual.key == request_.key);
  }
  HandoffState state() const noexcept { return state_; }
  HandoffIssue issue() const noexcept { return issue_; }
  bool cancelled() const noexcept { return cancelled_; }
  std::size_t step_index() const noexcept { return step_; }
  const recipe_navigation::Step* step() const noexcept { return active() && step_ < plan_.count ? &plan_.steps[step_] : nullptr; }
  const HandoffTicket* current_ticket() const noexcept { return ticket_.id ? &ticket_ : nullptr; }
  const RecipeRequest& request() const noexcept { return request_; }

private:
  bool valid_plan() const noexcept {
    namespace nav = recipe_navigation;
    if (plan_.target != request_.id || plan_.quantity != request_.quantity || !plan_.valid()) return false;
    if (request_.key.kind == recipe_catalog::Station::alchemy) return nav::structurally_valid(plan_);
    const auto direct = make_direct_info_plan(request_);
    return direct.valid() && plan_.source == direct.source && plan_.count == 1 &&
           recipe_handoff_detail::same_step(plan_.steps[0],direct.steps[0]);
  }
  bool same_scope(const HandoffObservation& value) const noexcept {
    return value.context_known && view_ && value.view == view_ && value.key == request_.key;
  }
  bool page_matches(const HandoffObservation& value, recipe_navigation::Page page) const noexcept {
    namespace nav = recipe_navigation;
    if (!value.page_known || value.page != page) return false;
    if (request_.key.kind == recipe_catalog::Station::stove)
      return page == nav::Page::unknown && value.mode == recipe_catalog::Invocation::info;
    if (const auto* route = nav::route_for(page))
      return value.mode == recipe_catalog::Invocation::choice && value.choice_owner == route->owner->instance;
    return nav::supported_page(page) && value.mode == recipe_catalog::Invocation::info;
  }
  bool ready_page(const HandoffObservation& value, recipe_navigation::Page page) const noexcept {
    return value.complete && value.ready && value.collection && value.count > 0 && value.count <= 256 && page_matches(value,page);
  }
  bool matches_offer(const HandoffObservation& value, const HandoffOffer& offer) const noexcept {
    if (step_ >= plan_.count) return false;
    const auto& step = plan_.steps[step_];
    return offer.valid && value.count > 0 && value.count <= 256 && offer.row >= 0 && offer.row < value.count && offer.collection &&
           offer.collection == value.collection && offer.mode == value.mode && offer.mode == step.invocation &&
           offer.owner == step.owner_instance && offer.function == step.function_symbol;
  }
  bool check_common(const HandoffObservation& value) noexcept {
    if (!same_scope(value)) { cancel(HandoffIssue::context); return false; }
    if (!value.focused) { cancel(HandoffIssue::focus); return false; }
    if (!value.frame || value.frame < last_frame_ || value.frame < step_frame_) { cancel(HandoffIssue::boundary); return false; }
    if (value.frame - step_frame_ > frame_limit_) { cancel(HandoffIssue::timeout); return false; }
    if (!recipe_handoff_detail::same_resources(request_.material_snapshot,value.materials)) {
      cancel(HandoffIssue::resources); return false;
    }
    if (!recipe_handoff_detail::valid_hands(request_,value.hands) || !recipe_handoff_detail::same_hands(hands_,value.hands)) {
      cancel(HandoffIssue::hands); return false;
    }
    return true;
  }

  RecipeRequest request_{};
  recipe_navigation::Plan plan_{};
  HandsSnapshot hands_{};
  HandoffTicket ticket_{};
  std::uint32_t view_ = 0;
  std::size_t step_ = 0;
  std::uint64_t last_generation_ = 0, last_frame_ = 0, step_frame_ = 0, frame_limit_ = 0;
  std::uint64_t ticket_serial_ = 0, execution_serial_ = 0, execution_id_ = 0, callback_frame_ = 0;
  bool callback_active_ = false, returned_ = false, cancelled_ = false, effect_started_ = false;
  HandoffState state_ = HandoffState::idle;
  HandoffIssue issue_ = HandoffIssue::none;
};

} // namespace ubc
