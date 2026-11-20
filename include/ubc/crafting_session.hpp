// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/crafting_catalog.hpp"
#include "ubc/quantity.hpp"
#include "ubc/selection_session.hpp"

namespace ubc {

// A confirmed user order, no native selection yet. The
// adapter takes it once and runs its own bound side chain.
class CraftingSession {
public:
  bool accepts_generation(std::uint64_t generation) const noexcept {
    return generation && generation > last_generation_;
  }
  bool open(const CraftingSnapshot& snapshot, crafting_catalog::RecipeId preferred=UINT32_MAX) noexcept {
    if (!accepts_generation(snapshot.key.generation)) {
      original(SelectionIssue::context_changed); return false;
    }
    *this = CraftingSession{};
    last_generation_ = snapshot.key.generation;
    if (!valid_crafting_snapshot(snapshot)) { original(SelectionIssue::invalid_snapshot); return false; }
    snapshot_ = snapshot;
    phase_ = SelectionPhase::selecting;
    if (const auto* first = visible(0)) { selected_id_ = first->id; have_selection_ = true; }
    // Only start focus from a previously verified result. Visibility,
    // fresh material data, and the new input release remain mandatory.
    for(std::size_t i=0;i<visible_count();++i)if(visible(i)->id==preferred) {
      selected_id_=preferred;have_selection_=true;break;
    }
    quantity_.reset(maximum());
    return true;
  }

  void observe(std::uint64_t frame, bool held, bool focused) noexcept {
    quantity_checked_=false;
    observed_frame_valid_=frame>observed_frame_;
    if(observed_frame_valid_)observed_frame_=frame;
    input_.observe(frame, held, focused);
    if (!focused && (phase_ == SelectionPhase::releasing || phase_ == SelectionPhase::ready)) {
      pending_ = {}; phase_ = SelectionPhase::selecting; issue_ = SelectionIssue::focus_lost;
    }
    if (phase_ == SelectionPhase::releasing && input_.armed()) phase_ = SelectionPhase::ready;
  }

  const CraftingState* visible(std::size_t index) const noexcept {
    for (const auto& state : snapshot_.recipes) {
      const auto* descriptor = crafting_catalog::find(state.id);
      if (!state.known || !descriptor || descriptor->station != snapshot_.key.kind) continue;
      if (!index--) return &state;
    }
    return nullptr;
  }
  std::size_t visible_count() const noexcept {
    std::size_t count = 0;
    for (const auto& state : snapshot_.recipes) {
      const auto* descriptor = crafting_catalog::find(state.id);
      if (state.known && descriptor && descriptor->station == snapshot_.key.kind) ++count;
    }
    return count;
  }
  const CraftingState* selected() const noexcept {
    if (have_selection_) for (const auto& state : snapshot_.recipes) if (state.id == selected_id_) return &state;
    return nullptr;
  }
  std::size_t selected_index() const noexcept {
    for (std::size_t i = 0; i < visible_count(); ++i) if (visible(i)->id == selected_id_ && have_selection_) return i;
    return visible_count();
  }
  bool select_recipe(crafting_catalog::RecipeId id, bool activation = false) noexcept {
    const bool accepted = activation ? input_.press() : input_.armed();
    if (!accepted || phase_ != SelectionPhase::selecting || (have_selection_ && id == selected_id_)) return false;
    for (std::size_t i = 0; i < visible_count(); ++i) if (visible(i)->id == id) {
      selected_id_ = id; have_selection_ = true; pending_ = {};quantity_checked_=false;
      quantity_.reset(maximum()); input_.disarm(); issue_ = SelectionIssue::none; return true;
    }
    return false;
  }
  bool select_relative(int direction) noexcept {
    if (!direction || !have_selection_) return false;
    const auto current = selected_index();
    if ((direction < 0 && !current) || (direction > 0 && current + 1 >= visible_count())) return false;
    const auto* next = visible(direction < 0 ? current - 1 : current + 1);
    return next && select_recipe(next->id);
  }
  bool adjust(std::int32_t delta, bool activation = false) noexcept {
    const bool accepted = activation ? input_.press() : input_.armed();
    if (!accepted || phase_ != SelectionPhase::selecting || !maximum()) return false;
    quantity_.adjust(delta);quantity_checked_=false; return true;
  }
  bool preset(std::uint32_t amount) noexcept {
    const bool accepted = input_.press();
    const bool changed=accepted && phase_ == SelectionPhase::selecting && quantity_.preset(amount);
    if(changed)quantity_checked_=false;return changed;
  }
  bool acknowledge() noexcept {
    const bool accepted = input_.press();
    return accepted && phase_ == SelectionPhase::selecting;
  }
  bool can_confirm() const noexcept {
    const auto* state = selected();
    return state && crafting_quantity_block_reason(*state,quantity()) == CraftingBlockReason::none &&
           quantity() && quantity() <= maximum() && quantity_ready();
  }
  // Quantity-dependent inventory paths are not a blanket material receipt.
  // Root delivers the fresh chosen order after a pure read-only item
  // path check. Recipe, quantity, frame, or material change rejects it.
  bool observe_quantity_check(const CraftingRequest& checked,std::uint64_t frame,bool available) noexcept {
    quantity_checked_=false;const auto* state=selected();
    const bool valid=available&&state&&observed_frame_valid_&&frame&&frame==observed_frame_&&checked.key==snapshot_.key&&
      checked.id==selected_id_&&checked.quantity==quantity()&&checked.quantity&&
      same_crafting_state(checked.material_snapshot,*state)&&same_crafting_hands(checked.hands,snapshot_.hands)&&
      crafting_quantity_block_reason(*state,quantity())==CraftingBlockReason::none;
    if(valid){quantity_checked_=true;checked_id_=checked.id;checked_quantity_=checked.quantity;}
    else if(requires_quantity_check()&&(phase_==SelectionPhase::releasing||phase_==SelectionPhase::ready)){
      pending_={};phase_=SelectionPhase::selecting;issue_=SelectionIssue::none;input_.disarm();
    }
    return valid;
  }
  bool confirm() noexcept {
    const bool accepted = input_.press();
    if (!accepted || phase_ != SelectionPhase::selecting || !can_confirm()) return false;
    pending_ = {selected_id_, quantity(), snapshot_.key, *selected(), snapshot_.hands};
    phase_ = SelectionPhase::releasing; issue_ = SelectionIssue::none; return true;
  }
  bool back() noexcept {
    const bool accepted = input_.press();
    if (input_.focused() && (phase_ == SelectionPhase::releasing || phase_ == SelectionPhase::ready)) {
      original(SelectionIssue::none); return true;
    }
    if (!accepted || phase_ != SelectionPhase::selecting) return false;
    original(SelectionIssue::none); return true;
  }
  void original(SelectionIssue issue) noexcept {
    if (phase_ == SelectionPhase::dispatched || phase_ == SelectionPhase::original) return;
    pending_ = {}; phase_ = SelectionPhase::original; issue_ = issue; input_.disarm();
  }

  bool refresh(const CraftingSnapshot& current) noexcept {
    if (phase_ == SelectionPhase::inactive || phase_ == SelectionPhase::original || phase_ == SelectionPhase::dispatched)
      return false;
    if (!valid_crafting_snapshot(current)) { original(SelectionIssue::invalid_snapshot); return false; }
    if (!(snapshot_.key == current.key)) { original(SelectionIssue::context_changed); return false; }
    const CraftingState* replacement = nullptr;
    bool knowledge_changed = false;
    for (const auto& old : snapshot_.recipes) for (const auto& next : current.recipes) if (old.id == next.id) {
      if (old.known != next.known) knowledge_changed = true;
      if (have_selection_ && next.id == selected_id_) replacement = &next;
    }
    if (have_selection_ && (!replacement || !replacement->known)) {
      original(SelectionIssue::offer_changed); return false;
    }
    const bool changed = knowledge_changed || !same_crafting_hands(snapshot_.hands,current.hands) ||
      (replacement && !same_crafting_state(*selected(), *replacement));
    snapshot_ = current;
    if (!have_selection_) if (const auto* first = visible(0)) { selected_id_ = first->id; have_selection_ = true; }
    if (changed) {
      quantity_checked_=false;
      pending_ = {}; quantity_.reset(maximum()); phase_ = SelectionPhase::selecting;
      issue_ = knowledge_changed ? SelectionIssue::offer_changed : SelectionIssue::stock_changed;
      input_.disarm(); return false;
    }
    return true;
  }

  // handoff_ready confirms the external handoff boundary, not UI closing:
  // for the later native chain the own menu may continue binding inputs.
  std::optional<CraftingRequest> take(const CraftingSnapshot& current, bool handoff_ready) noexcept {
    if (phase_ != SelectionPhase::ready || !input_.armed() || !handoff_ready || !pending_.quantity)
      return std::nullopt;
    if (!refresh(current) || !can_confirm()) return std::nullopt;
    const CraftingRequest result{selected_id_, pending_.quantity, snapshot_.key, *selected(), snapshot_.hands};
    pending_ = {}; phase_ = SelectionPhase::dispatched; input_.disarm(); return result;
  }

  std::uint32_t quantity() const noexcept { return quantity_.value(); }
  std::uint32_t maximum() const noexcept {
    const auto* state = selected();
    return state && state->material_known && state->hand_known && state->material_verified ? state->maximum : 0;
  }
  CraftingBlockReason reason() const noexcept {
    const auto* state = selected();
    if(!state)return CraftingBlockReason::no_known_recipes;
    const auto reason=crafting_quantity_block_reason(*state,quantity());
    return reason==CraftingBlockReason::none&&!quantity_ready()?CraftingBlockReason::unavailable:reason;
  }
  const CraftingSnapshot& preview() const noexcept { return snapshot_; }
  SelectionPhase phase() const noexcept { return phase_; }
  SelectionIssue issue() const noexcept { return issue_; }
  bool armed() const noexcept { return input_.armed(); }
  bool return_ready() const noexcept { return phase_ == SelectionPhase::original && input_.armed(); }

private:
  bool requires_quantity_check() const noexcept {
    const auto* d=have_selection_?crafting_catalog::find(selected_id_):nullptr;
    return d&&(d->route==crafting_catalog::Route::bow||d->route==crafting_catalog::Route::cool||d->route==crafting_catalog::Route::grind);
  }
  bool quantity_ready() const noexcept {
    return !requires_quantity_check()||(quantity_checked_&&observed_frame_valid_&&observed_frame_&&checked_id_==selected_id_&&checked_quantity_==quantity());
  }
  CraftingSnapshot snapshot_{};
  CraftingRequest pending_{};
  crafting_catalog::RecipeId selected_id_ = 0;
  bool have_selection_ = false;
  bool quantity_checked_=false;
  bool observed_frame_valid_=false;
  crafting_catalog::RecipeId checked_id_=UINT32_MAX;
  std::uint32_t checked_quantity_=0;
  std::uint64_t observed_frame_=0;
  Quantity quantity_{};
  SelectionInput input_{};
  std::uint64_t last_generation_ = 0;
  SelectionPhase phase_ = SelectionPhase::inactive;
  SelectionIssue issue_ = SelectionIssue::none;
};

} // namespace ubc
