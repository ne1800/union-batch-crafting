// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/core/quantity.hpp"
#include "ubc/catalogs/recipe_catalog.hpp"
#include "ubc/core/selection_session.hpp"

namespace ubc {

// Identity values only: no dereferenceable engine objects in the
// value core. Mode, Owner and Collection belong to the separate native
// side contract; they may change within an explicit selection chain.
struct StationKey {
  std::uint64_t generation = 0;
  std::uint32_t thread = 0, world = 0, hero = 0, station = 0, manager = 0;
  recipe_catalog::Station kind = recipe_catalog::Station::stove;

  bool operator==(const StationKey& other) const noexcept {
    return generation == other.generation && thread == other.thread && world == other.world &&
           hero == other.hero && station == other.station && manager == other.manager && kind == other.kind;
  }
};

struct RecipeMaterialValue {
  std::int32_t symbol = -1, stored = -1, reserved = -1;
  std::uint32_t unit_cost = 0, unit_output = 0;
};

struct RecipeState {
  recipe_catalog::RecipeId id = recipe_catalog::RecipeId::meat;
  bool known = false, contract = false, hand_known = false, material_known = false;
  std::uint32_t maximum = 0;
  std::size_t target_count = 0;
  std::array<RecipeMaterialValue, recipe_catalog::catalog_target_capacity> materials{};
};

namespace recipe_session_detail {
inline std::uint32_t units(recipe_catalog::View<recipe_catalog::ItemAmount> entries,
                           std::int32_t symbol) noexcept {
  for (const auto& entry : entries) if (entry.symbol == symbol) return entry.units;
  return 0;
}
inline const RecipeMaterialValue* material(const RecipeState& state, std::int32_t symbol) noexcept {
  for (std::size_t i = 0; i < state.target_count; ++i)
    if (state.materials[i].symbol == symbol) return &state.materials[i];
  return nullptr;
}
inline bool valid_key(const StationKey& key) noexcept {
  return key.generation && key.thread && key.world && key.hero && key.station && key.manager &&
         (key.kind == recipe_catalog::Station::stove || key.kind == recipe_catalog::Station::alchemy);
}
} // namespace recipe_session_detail

// Own static metadata pre-set; dynamic values remain unknown.
inline RecipeState make_recipe_state(recipe_catalog::RecipeId id) noexcept {
  RecipeState state;
  state.id = id;
  const auto* descriptor = recipe_catalog::find(id);
  if (!descriptor || descriptor->targets.count > state.materials.size()) return state;
  state.target_count = descriptor->targets.count;
  for (std::size_t i = 0; i < state.target_count; ++i) {
    auto& row = state.materials[i];
    row.symbol = descriptor->targets[i].symbol;
    row.unit_cost = recipe_session_detail::units(descriptor->costs, row.symbol);
    row.unit_output = recipe_session_detail::units(descriptor->outputs, row.symbol);
  }
  return state;
}

inline auto make_catalogue_states() noexcept {
  std::array<RecipeState, recipe_catalog::recipes.size()> states{};
  for (std::size_t i = 0; i < states.size(); ++i) states[i] = make_recipe_state(recipe_catalog::recipes[i].id);
  return states;
}

struct CatalogueSnapshot {
  StationKey key{};
  std::array<RecipeState, recipe_catalog::recipes.size()> recipes = make_catalogue_states();
};

struct RecipeRequest {
  recipe_catalog::RecipeId id = recipe_catalog::RecipeId::meat;
  std::uint32_t quantity = 0;
  StationKey key{};
  RecipeState material_snapshot{};
};

enum class RecipeBlockReason {
  none, no_known_recipes, contract_unverified, hand_unknown, material_unknown,
  missing_material, output_capacity, unavailable
};

struct RecipeAmountPreview {
  bool available_known = false;
  std::int64_t available = -1, required = 0, produced = 0, missing = -1, remaining = -1;
  std::int64_t output_shortfall = -1;
};

// Even at quantity 0, the explicit preview for one pass via a call
// with quantity=1 remains possible. Unknown numbers are never zero.
inline RecipeAmountPreview recipe_amount_preview(const RecipeMaterialValue& row,
                                                 std::uint32_t quantity) noexcept {
  RecipeAmountPreview result;
  if (quantity > static_cast<std::uint32_t>(INT32_MAX)) return result;
  result.required = std::int64_t{row.unit_cost} * quantity;
  result.produced = std::int64_t{row.unit_output} * quantity;
  if (row.stored < 0 || row.reserved < 0) return result;
  result.available_known = true;
  result.available = std::int64_t{row.stored} + row.reserved;
  result.missing = result.required > result.available ? result.required - result.available : 0;
  // Catalog costs/products are per-symbol disjoint. Even untrusted
  // pure test values cannot push the calculation beyond int64.
  const auto rest = result.available - result.required;
  if (result.produced <= INT64_MAX - (rest > 0 ? rest : 0)) result.remaining = rest + result.produced;
  const auto space = std::int64_t{INT32_MAX} - row.stored;
  result.output_shortfall = result.produced > space ? result.produced - space : 0;
  return result;
}

inline bool valid_recipe_state(const RecipeState& state) noexcept {
  namespace detail = recipe_session_detail;
  const auto* descriptor = recipe_catalog::find(state.id);
  if (!descriptor || !state.target_count || state.target_count > state.materials.size() ||
      state.target_count != descriptor->targets.count || state.maximum > static_cast<std::uint32_t>(INT32_MAX))
    return false;
  std::uint64_t numeric_maximum = INT32_MAX;
  for (std::size_t i = 0; i < state.target_count; ++i) {
    const auto& row = state.materials[i];
    if (!recipe_catalog::has_target(*descriptor, row.symbol) || row.stored < -1 || row.reserved < -1 ||
        (state.material_known && row.stored < 0) || (state.hand_known && row.reserved < 0) ||
        row.unit_cost != detail::units(descriptor->costs, row.symbol) ||
        row.unit_output != detail::units(descriptor->outputs, row.symbol)) return false;
    for (std::size_t j = 0; j < i; ++j) if (state.materials[j].symbol == row.symbol) return false;
    if (state.material_known && state.hand_known) {
      const auto limit = [&](std::uint64_t value) { if (value < numeric_maximum) numeric_maximum = value; };
      if (row.unit_cost) {
        limit((std::uint64_t{static_cast<std::uint32_t>(row.stored)} + static_cast<std::uint32_t>(row.reserved)) / row.unit_cost);
        limit(static_cast<std::uint32_t>(INT32_MAX) / row.unit_cost);
      }
      if (row.unit_output) limit(static_cast<std::uint32_t>(INT32_MAX - row.stored) / row.unit_output);
    }
  }
  return !state.material_known || !state.hand_known || state.maximum <= numeric_maximum;
}

inline bool valid_catalogue_snapshot(const CatalogueSnapshot& snapshot) noexcept {
  if (!recipe_session_detail::valid_key(snapshot.key)) return false;
  for (std::size_t i = 0; i < snapshot.recipes.size(); ++i) {
    if (!valid_recipe_state(snapshot.recipes[i])) return false;
    for (std::size_t j = 0; j < i; ++j) if (snapshot.recipes[i].id == snapshot.recipes[j].id) return false;
  }
  return true;
}

inline RecipeBlockReason recipe_block_reason(const RecipeState& state) noexcept {
  if (!state.known) return RecipeBlockReason::no_known_recipes;
  if (!state.contract) return RecipeBlockReason::contract_unverified;
  if (!state.material_known) return RecipeBlockReason::material_unknown;
  if (!state.hand_known) return RecipeBlockReason::hand_unknown;
  if (state.maximum) return RecipeBlockReason::none;
  for (std::size_t i = 0; i < state.target_count && i < state.materials.size(); ++i)
    if (recipe_amount_preview(state.materials[i], 1).missing > 0) return RecipeBlockReason::missing_material;
  for (std::size_t i = 0; i < state.target_count && i < state.materials.size(); ++i)
    if (recipe_amount_preview(state.materials[i], 1).output_shortfall > 0) return RecipeBlockReason::output_capacity;
  return RecipeBlockReason::unavailable;
}

// A confirmed user order, not yet a native selection. The
// adapter takes it once and runs its own bound side chain.
class RecipeSession {
public:
  bool open(const CatalogueSnapshot& snapshot) noexcept {
    if (!snapshot.key.generation || snapshot.key.generation <= last_generation_) {
      original(SelectionIssue::context_changed); return false;
    }
    *this = RecipeSession{};
    last_generation_ = snapshot.key.generation;
    if (!valid_catalogue_snapshot(snapshot)) { original(SelectionIssue::invalid_snapshot); return false; }
    snapshot_ = snapshot;
    phase_ = SelectionPhase::selecting;
    if (const auto* first = visible(0)) { selected_id_ = first->id; have_selection_ = true; }
    quantity_.reset(maximum());
    return true;
  }

  void observe(std::uint64_t frame, bool held, bool focused) noexcept {
    input_.observe(frame, held, focused);
    if (!focused && (phase_ == SelectionPhase::releasing || phase_ == SelectionPhase::ready)) {
      pending_ = {}; phase_ = SelectionPhase::selecting; issue_ = SelectionIssue::focus_lost;
    }
    if (phase_ == SelectionPhase::releasing && input_.armed()) phase_ = SelectionPhase::ready;
  }

  const RecipeState* visible(std::size_t index) const noexcept {
    for (const auto& state : snapshot_.recipes) {
      const auto* descriptor = recipe_catalog::find(state.id);
      if (!state.known || !descriptor || descriptor->station != snapshot_.key.kind) continue;
      if (!index--) return &state;
    }
    return nullptr;
  }
  std::size_t visible_count() const noexcept {
    std::size_t count = 0;
    for (const auto& state : snapshot_.recipes) {
      const auto* descriptor = recipe_catalog::find(state.id);
      if (state.known && descriptor && descriptor->station == snapshot_.key.kind) ++count;
    }
    return count;
  }
  const RecipeState* selected() const noexcept {
    if (have_selection_) for (const auto& state : snapshot_.recipes) if (state.id == selected_id_) return &state;
    return nullptr;
  }
  std::size_t selected_index() const noexcept {
    for (std::size_t i = 0; i < visible_count(); ++i) if (visible(i)->id == selected_id_ && have_selection_) return i;
    return visible_count();
  }
  bool select_recipe(recipe_catalog::RecipeId id, bool activation = false) noexcept {
    const bool accepted = activation ? input_.press() : input_.armed();
    if (!accepted || phase_ != SelectionPhase::selecting || (have_selection_ && id == selected_id_)) return false;
    for (std::size_t i = 0; i < visible_count(); ++i) if (visible(i)->id == id) {
      selected_id_ = id; have_selection_ = true; pending_ = {};
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
    quantity_.adjust(delta); return true;
  }
  bool preset(std::uint32_t amount) noexcept {
    const bool accepted = input_.press();
    return accepted && phase_ == SelectionPhase::selecting && quantity_.preset(amount);
  }
  bool acknowledge() noexcept {
    const bool accepted = input_.press();
    return accepted && phase_ == SelectionPhase::selecting;
  }
  bool can_confirm() const noexcept {
    const auto* state = selected();
    return state && state->known && state->contract && state->hand_known && state->material_known &&
           quantity() && quantity() <= maximum();
  }
  bool confirm() noexcept {
    const bool accepted = input_.press();
    if (!accepted || phase_ != SelectionPhase::selecting || !can_confirm()) return false;
    pending_ = {selected_id_, quantity(), snapshot_.key, *selected()};
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

  bool refresh(const CatalogueSnapshot& current) noexcept {
    if (phase_ == SelectionPhase::inactive || phase_ == SelectionPhase::original || phase_ == SelectionPhase::dispatched)
      return false;
    if (!valid_catalogue_snapshot(current)) { original(SelectionIssue::invalid_snapshot); return false; }
    if (!(snapshot_.key == current.key)) { original(SelectionIssue::context_changed); return false; }
    const RecipeState* replacement = nullptr;
    bool knowledge_changed = false;
    for (const auto& old : snapshot_.recipes) for (const auto& next : current.recipes) if (old.id == next.id) {
      if (old.known != next.known) knowledge_changed = true;
      if (have_selection_ && next.id == selected_id_) replacement = &next;
    }
    if (have_selection_ && (!replacement || !replacement->known)) {
      original(SelectionIssue::offer_changed); return false;
    }
    const bool changed = knowledge_changed || (replacement && !same_state(*selected(), *replacement));
    snapshot_ = current;
    if (!have_selection_) if (const auto* first = visible(0)) { selected_id_ = first->id; have_selection_ = true; }
    if (changed) {
      pending_ = {}; quantity_.reset(maximum()); phase_ = SelectionPhase::selecting;
      issue_ = knowledge_changed ? SelectionIssue::offer_changed : SelectionIssue::stock_changed;
      input_.disarm(); return false;
    }
    return true;
  }

  // handoff_ready confirms the external handoff boundary, not UI closing:
  // for the later native chain the own menu may continue binding inputs.
  std::optional<RecipeRequest> take(const CatalogueSnapshot& current, bool handoff_ready) noexcept {
    if (phase_ != SelectionPhase::ready || !input_.armed() || !handoff_ready || !pending_.quantity)
      return std::nullopt;
    if (!refresh(current) || !can_confirm()) return std::nullopt;
    const RecipeRequest result{selected_id_, pending_.quantity, snapshot_.key, *selected()};
    pending_ = {}; phase_ = SelectionPhase::dispatched; input_.disarm(); return result;
  }

  std::uint32_t quantity() const noexcept { return quantity_.value(); }
  std::uint32_t maximum() const noexcept {
    const auto* state = selected();
    return state && state->material_known && state->hand_known ? state->maximum : 0;
  }
  RecipeBlockReason reason() const noexcept {
    const auto* state = selected();
    return state ? recipe_block_reason(*state) : RecipeBlockReason::no_known_recipes;
  }
  const CatalogueSnapshot& preview() const noexcept { return snapshot_; }
  SelectionPhase phase() const noexcept { return phase_; }
  SelectionIssue issue() const noexcept { return issue_; }
  bool armed() const noexcept { return input_.armed(); }
  bool return_ready() const noexcept { return phase_ == SelectionPhase::original && input_.armed(); }

private:
  static bool same_state(const RecipeState& left, const RecipeState& right) noexcept {
    if (left.id != right.id || left.known != right.known || left.contract != right.contract ||
        left.hand_known != right.hand_known || left.material_known != right.material_known ||
        left.maximum != right.maximum || left.target_count != right.target_count) return false;
    for (std::size_t i = 0; i < left.target_count; ++i) {
      const auto& value = left.materials[i];
      const auto* other = recipe_session_detail::material(right, value.symbol);
      if (!other || value.stored != other->stored || value.reserved != other->reserved ||
          value.unit_cost != other->unit_cost || value.unit_output != other->unit_output) return false;
    }
    return true;
  }
  CatalogueSnapshot snapshot_{};
  RecipeRequest pending_{};
  recipe_catalog::RecipeId selected_id_ = recipe_catalog::RecipeId::meat;
  bool have_selection_ = false;
  Quantity quantity_{};
  SelectionInput input_{};
  std::uint64_t last_generation_ = 0;
  SelectionPhase phase_ = SelectionPhase::inactive;
  SelectionIssue issue_ = SelectionIssue::none;
};

} // namespace ubc
