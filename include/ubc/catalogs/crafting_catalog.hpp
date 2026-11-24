// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/dndr_alchemy_profile.hpp"
#include "ubc/core/recipe_snapshot.hpp"
#include "ubc/stations/station_recipe_snapshot.hpp"
#include "ubc/stations/alchemy/alchemy_extra_snapshot.hpp"
#include "ubc/stations/anvil/anvil_recipe_snapshot.hpp"
#include "ubc/stations/ore/ore_recipe_snapshot.hpp"
#include "ubc/catalogs/spirit_recipe_catalog.hpp"
#include "ubc/stations/bow/bow_recipe_snapshot.hpp"
#include "ubc/stations/cool/cool_recipe_snapshot.hpp"
#include "ubc/catalogs/crafting_grind_names.hpp"
#include "ubc/catalogs/tobacco_recipe_catalog.hpp"

namespace ubc::crafting_catalog {
using RecipeId = std::uint32_t;
using recipe_catalog::ItemAmount;
using recipe_catalog::View;
enum class Station { stove, alchemy, leather, saw, forge, anvil, bow, cool, grind };
enum class Route {
  base,
  station,
  alchemy_extra,
  anvil,
  ore,
  spirit,
  bow,
  cool,
  grind,
  tobacco,
  dndr_alchemy
};
inline constexpr bool requires_context(Route route) noexcept {
  return route == Route::dndr_alchemy || route == Route::tobacco || route == Route::alchemy_extra ||
         route == Route::anvil || route == Route::ore || route == Route::spirit ||
         route == Route::bow || route == Route::cool || route == Route::grind;
}
enum class Completion { ready_same_station, end_production };
struct Descriptor {
  RecipeId id = UINT32_MAX;
  const char* name = nullptr;
  Station station = Station::stove;
  Route route = Route::base;
  View<PackedInventoryTarget> targets;
  View<ItemAmount> costs, outputs;
  const char* quantity_unit = nullptr;
  Completion completion = Completion::end_production;
  // Only static own descriptors, never engine/choice pointers.
  const recipe_catalog::Descriptor* base = nullptr;
  const station_recipe_catalog::Descriptor* station_descriptor = nullptr;
  const alchemy_recipe_catalog::Descriptor* alchemy_extra = nullptr;
  const anvil_recipe_catalog::Descriptor* anvil = nullptr;
  const ore_recipe_catalog::Descriptor* ore = nullptr;
  const spirit_recipe_catalog::Descriptor* spirit = nullptr;
  const bow_recipe_catalog::Descriptor* bow = nullptr;
  const cool_recipe_catalog::Descriptor* cool = nullptr;
  const grind_recipe::Descriptor* grind = nullptr;
  const tobacco::Descriptor* tobacco = nullptr;
  const dndr::AlchemyRecipe* dndr_alchemy = nullptr;
};
inline constexpr std::size_t base_begin = 0, station_begin = recipe_catalog::recipes.size();
inline constexpr std::size_t alchemy_extra_begin =
    station_begin + station_recipe_catalog::recipes.size();
inline constexpr std::size_t anvil_begin =
    alchemy_extra_begin + alchemy_recipe_catalog::recipes.size();
inline constexpr std::size_t ore_begin = anvil_begin + anvil_recipe_catalog::recipes.size();
inline constexpr std::size_t spirit_begin = ore_begin + ore_recipe_catalog::recipes.size();
inline constexpr std::size_t bow_begin = spirit_begin + spirit_recipe_catalog::recipes.size();
inline constexpr std::size_t cool_begin = bow_begin + bow_recipe_catalog::recipes.size();
inline constexpr std::size_t grind_begin = cool_begin + 1;
inline constexpr std::size_t tobacco_begin = grind_begin + grind_recipe::recipes.size();
inline constexpr std::size_t recipe_count = tobacco_begin + tobacco::recipes.size();
inline constexpr RecipeId grind_first_id = 0x9001;
inline constexpr auto grind_targets = [] {
  std::array<std::array<PackedInventoryTarget, 3>, grind_recipe::recipes.size()> result{};
  for (std::size_t i = 0; i < result.size(); ++i) {
    const auto& recipe = grind_recipe::recipes[i];
    const std::array<std::int32_t, 3> symbols{
        {recipe.input, recipe.output, grind_recipe::lifecycle_item}};
    for (std::size_t j = 0; j < 3; ++j)
      for (const auto& item : grind_recipe_profile::items)
        if (item.function->symbol == static_cast<std::uint32_t>(symbols[j]))
          result[i][j] = {item.function->name, symbols[j]};
  }
  return result;
}();
inline constexpr auto grind_costs = [] {
  std::array<std::array<ItemAmount, 1>, grind_recipe::recipes.size()> result{};
  for (std::size_t i = 0; i < result.size(); ++i)
    result[i][0] = {grind_recipe::recipes[i].input, 1};
  return result;
}();
inline constexpr auto grind_outputs = [] {
  std::array<std::array<ItemAmount, 1>, grind_recipe::recipes.size()> result{};
  for (std::size_t i = 0; i < result.size(); ++i)
    result[i][0] = {grind_recipe::recipes[i].output, 1};
  return result;
}();
inline constexpr std::array<ItemAmount, 1> cool_costs{{{16612, 1}}}, cool_outputs{{{16613, 1}}};
inline constexpr std::size_t target_capacity = []() constexpr {
  std::size_t maximum = 0;
  for (const auto n :
       {recipe_catalog::catalog_target_capacity, station_recipe_catalog::catalog_target_capacity,
        alchemy_recipe_catalog::catalog_target_capacity,
        anvil_recipe_catalog::catalog_target_capacity, ore_recipe_catalog::catalog_target_capacity,
        spirit_recipe_catalog::target_capacity})
    if (n > maximum)
      maximum = n;
  return maximum;
}();
inline constexpr auto recipes = [] {
  std::array<Descriptor, recipe_count> result{};
  std::size_t i = 0;
  for (const auto& d : recipe_catalog::recipes) {
    const bool stove = d.station == recipe_catalog::Station::stove;
    result[i++] = {static_cast<RecipeId>(d.id),
                   d.name,
                   stove ? Station::stove : Station::alchemy,
                   Route::base,
                   d.targets,
                   d.costs,
                   d.outputs,
                   "Durchgaenge",
                   stove ? Completion::ready_same_station : Completion::end_production,
                   &d,
                   nullptr};
  }
  for (const auto& d : station_recipe_catalog::recipes) {
    const auto kind = d.station == station_recipe_catalog::Station::leatherwork ? Station::leather
                      : d.station == station_recipe_catalog::Station::saw       ? Station::saw
                                                                                : Station::forge;
    result[i++] = {static_cast<RecipeId>(d.id),
                   d.name,
                   kind,
                   Route::station,
                   d.targets,
                   d.costs,
                   d.outputs,
                   d.quantity_unit,
                   d.completion == station_recipe_catalog::Completion::ready_same_station
                       ? Completion::ready_same_station
                       : Completion::end_production,
                   nullptr,
                   &d};
  }
  for (const auto& d : alchemy_recipe_catalog::recipes)
    result[i++] = {static_cast<RecipeId>(d.id),
                   d.name,
                   Station::alchemy,
                   Route::alchemy_extra,
                   d.targets,
                   d.costs,
                   d.outputs,
                   "Durchgaenge",
                   Completion::end_production,
                   nullptr,
                   nullptr,
                   &d,
                   nullptr,
                   nullptr};
  for (const auto& d : anvil_recipe_catalog::recipes)
    result[i++] = {static_cast<RecipeId>(d.id),
                   d.name,
                   Station::anvil,
                   Route::anvil,
                   d.targets,
                   d.costs,
                   d.outputs,
                   "Durchgaenge",
                   Completion::end_production,
                   nullptr,
                   nullptr,
                   nullptr,
                   &d,
                   nullptr};
  for (const auto& d : ore_recipe_catalog::recipes)
    result[i++] = {static_cast<RecipeId>(d.id),
                   d.name,
                   Station::anvil,
                   Route::ore,
                   d.targets,
                   d.costs,
                   d.outputs,
                   "Durchgaenge",
                   Completion::end_production,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   &d};
  for (const auto& d : spirit_recipe_catalog::recipes)
    result[i++] = {static_cast<RecipeId>(d.id),
                   d.name,
                   Station::alchemy,
                   Route::spirit,
                   d.targets,
                   d.costs,
                   {},
                   "Durchgaenge",
                   Completion::end_production,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   &d};
  for (const auto& d : bow_recipe_catalog::recipes)
    result[i++] = {static_cast<RecipeId>(d.id),
                   d.name,
                   Station::bow,
                   Route::bow,
                   d.targets,
                   d.costs,
                   d.outputs,
                   "Durchgaenge",
                   Completion::end_production,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   &d};
  const auto& cool = cool_recipe_catalog::recipe;
  result[i++] = {static_cast<RecipeId>(cool.id),
                 cool.name,
                 Station::cool,
                 Route::cool,
                 recipe_catalog::view(cool_recipe_catalog::targets),
                 recipe_catalog::view(cool_costs),
                 recipe_catalog::view(cool_outputs),
                 "Klingen",
                 Completion::end_production,
                 nullptr,
                 nullptr,
                 nullptr,
                 nullptr,
                 nullptr,
                 nullptr,
                 nullptr,
                 &cool};
  // IDs precede Cool; existing ARRAY positions remain unchanged.
  for (std::size_t n = 0; n < grind_recipe::recipes.size(); ++n)
    result[i++] = {grind_first_id + static_cast<RecipeId>(n),
                   crafting_grind_names::recipes[n],
                   Station::grind,
                   Route::grind,
                   recipe_catalog::view(grind_targets[n]),
                   recipe_catalog::view(grind_costs[n]),
                   recipe_catalog::view(grind_outputs[n]),
                   "Waffen",
                   Completion::end_production,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   &grind_recipe::recipes[n]};
  for (const auto& d : tobacco::recipes) {
    result[i++] = {d.id,
                   d.name,
                   Station::alchemy,
                   Route::tobacco,
                   recipe_catalog::view(d.targets),
                   recipe_catalog::view(tobacco::costs),
                   recipe_catalog::view(d.outputs),
                   "Durchgaenge",
                   Completion::end_production};
    result[i - 1].tobacco = &d;
  }
  return result;
}();
inline constexpr auto dndr_recipes = [] {
  std::array<Descriptor, dndr::recipes.size()> entries{};
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const auto& r = dndr::recipes[i];
    entries[i] = {r.id,    r.name,    Station::alchemy, Route::dndr_alchemy,       r.targets,
                  r.costs, r.outputs, "Durchgaenge",    Completion::end_production};
    entries[i].dndr_alchemy = &r;
  }
  return entries;
}();
inline constexpr const Descriptor* find(RecipeId id) noexcept {
  for (const auto& value : dndr_recipes)
    if (value.id == id)
      return &value;
  for (const auto& value : recipes)
    if (value.id == id)
      return &value;
  return nullptr;
}
inline std::uint32_t units(View<ItemAmount> entries, std::int32_t symbol) noexcept {
  for (const auto& item : entries)
    if (item.symbol == symbol)
      return item.units;
  return 0;
}
// Dynamic output is reconstructed exclusively from the exact bound
// branch. Without a branch there is no asserted exit for the spirit.
inline View<ItemAmount> outputs_for(const Descriptor& d, bool known = false,
                                    std::int32_t branch = 0) noexcept {
  if (d.route != Route::spirit)
    return d.outputs;
  if (!d.spirit || !known)
    return {};
  const auto* selected =
      spirit_recipe_catalog::variant(*d.spirit, spirit_recipe_catalog::branch_for(known, branch));
  return selected ? View<ItemAmount>{&selected->output, 1} : View<ItemAmount>{};
}
inline bool valid_branch(const Descriptor& d, bool known, std::int32_t value,
                         std::int32_t output) noexcept {
  if (d.route != Route::spirit)
    return !known && !value && output == -1;
  const auto expected = outputs_for(d, known, value);
  return known ? expected.count == 1 && output == expected[0].symbol : !value && output == -1;
}
struct HandAmounts {
  std::int32_t consumed = 0, tool = 0, generated = 0;
  bool stored_absent = false;
};
inline HandAmounts hand_amounts(const Descriptor& d, std::int32_t symbol) noexcept {
  HandAmounts result;
  if (d.route == Route::dndr_alchemy) {
    result.consumed = symbol == static_cast<int>(dndr::flask) ? 1 : 0;
    return result;
  }
  if (d.tobacco) {
    if (symbol == tobacco::flask)
      result.consumed = 1;
    return result;
  }
  // Generated blade and actual stored ingredient coexist.
  if (d.route == Route::cool && d.cool) {
    if (symbol == d.cool->hot_symbol)
      result.generated = 1;
    return result;
  }
  if (d.route == Route::grind && d.grind) {
    if (symbol == grind_recipe::lifecycle_item)
      result.generated = 1;
    return result;
  }
  if ((d.route == Route::base && d.base) || (d.route == Route::alchemy_extra && d.alchemy_extra) ||
      (d.route == Route::spirit && d.spirit)) {
    const auto& hand = d.route == Route::base            ? d.base->hand
                       : d.route == Route::alchemy_extra ? d.alchemy_extra->hand
                                                         : d.spirit->hand;
    if (hand.active_item_symbol != symbol)
      return result;
    if (hand.from_inventory && hand.consumed_on_success)
      result.consumed = static_cast<std::int32_t>(hand.amount);
    if (!hand.from_inventory) {
      result.generated = static_cast<std::int32_t>(hand.amount);
      result.stored_absent = true;
    }
  } else {
    const auto append = [&](const auto& contract) noexcept {
      for (const auto* hand : {&contract.left, &contract.right}) {
        if (hand->symbol != symbol)
          continue;
        if (hand->source == station_recipe_catalog::HandSource::reserved_inventory_tool)
          result.tool += static_cast<std::int32_t>(hand->amount);
        if (hand->source == station_recipe_catalog::HandSource::generated)
          result.generated += static_cast<std::int32_t>(hand->amount);
        result.stored_absent = result.stored_absent || hand->require_stored_absent;
      }
    };
    if (d.route == Route::station && d.station_descriptor)
      append(d.station_descriptor->hand);
    else if (d.route == Route::anvil && d.anvil)
      append(d.anvil->hand);
    else if (d.route == Route::ore && d.ore)
      append(d.ore->hand);
  }
  return result;
}
} // namespace ubc::crafting_catalog

namespace ubc {
struct CraftingKey {
  std::uint64_t generation = 0;
  std::uint32_t thread = 0, world = 0, hero = 0, station = 0, manager = 0;
  crafting_catalog::Station kind = crafting_catalog::Station::stove;
  bool operator==(const CraftingKey& b) const noexcept {
    return generation == b.generation && thread == b.thread && world == b.world && hero == b.hero &&
           station == b.station && manager == b.manager && kind == b.kind;
  }
};
struct CraftingMaterialValue {
  std::int32_t symbol = -1, stored = -1;
  // Flask: total consumption includes this hand reserve. Saw: tool, neither
  // available ingredient nor consumption; return only at normal end.
  std::int32_t consumed_reserve = 0, tool_reserve = 0, generated_prop = 0;
  std::uint32_t unit_cost = 0, unit_output = 0;
  // The original minimum check can differ from actual consumption (bow callback 20930).
  std::uint32_t unit_minimum = 0;
  std::uint32_t fixed_minimum = 0;
};
struct CraftingGrindValues {
  bool known = false;
  std::int32_t talent = 0, guild = 0, skill = 0;
  bool operator==(const CraftingGrindValues& other) const noexcept {
    return known == other.known && talent == other.talent && guild == other.guild &&
           skill == other.skill;
  }
};
struct CraftingState {
  crafting_catalog::RecipeId id = UINT32_MAX;
  std::uint64_t generation = 0;
  bool evaluated = false, knowledge_read_valid = false, known = false, contract = false;
  bool actor_known = false, actor_matches = false, hand_known = false, material_known = false,
       material_verified = false;
  // Display evidence only. Admission, diagnostic codes and quantity receipts use the original gates.
  bool equipped_ingredient = false;
  // Additional routes bind physical station/production/start. Categories
  // and owner conditions remain mutable under original navigation.
  bool context_known = false, context_matches = false;
  std::uint32_t maximum = 0;
  std::size_t target_count = 0;
  std::array<CraftingMaterialValue, crafting_catalog::target_capacity> materials{};
  // Exact integer remains bound: even Else2->Else3 rejects the order.
  bool branch_known = false;
  std::int32_t branch_value = 0, branch_output = -1;
  // Exact current original values, no learning/guild replacement and no writer evidence.
  CraftingGrindValues grind{};
};
inline CraftingState make_crafting_state(crafting_catalog::RecipeId id) noexcept {
  CraftingState value;
  value.id = id;
  const auto* d = crafting_catalog::find(id);
  if (!d)
    return value;
  value.target_count = d->targets.count;
  for (std::size_t i = 0; i < value.target_count; ++i) {
    auto& row = value.materials[i];
    row.symbol = d->targets[i].symbol;
    row.unit_cost = crafting_catalog::units(d->costs, row.symbol);
    row.unit_output = crafting_catalog::units(d->outputs, row.symbol);
    if (d->tobacco && row.symbol == d->tobacco->targets[2].symbol)
      row.fixed_minimum = 1;
    if (d->bow)
      row.unit_minimum = crafting_catalog::units(d->bow->minima, row.symbol);
    const auto hand = crafting_catalog::hand_amounts(*d, row.symbol);
    if (hand.consumed)
      row.consumed_reserve = -1;
    if (hand.tool)
      row.tool_reserve = -1;
    if (hand.generated)
      row.generated_prop = -1;
  }
  return value;
}
inline auto make_crafting_states() noexcept {
  std::array<CraftingState, crafting_catalog::recipe_count> result{};
  for (std::size_t i = 0; i < result.size(); ++i)
    result[i] = make_crafting_state(crafting_catalog::recipes[i].id);
  return result;
}
enum class CraftingProjectionIssue { none, context, source };
struct CraftingSnapshot {
  CraftingKey key{};
  CraftingProjectionIssue issue = CraftingProjectionIssue::none;
  std::array<CraftingState, crafting_catalog::recipe_count> recipes = make_crafting_states();
  // The base catalog has only the two object observations. Leave unknown slot
  // or anchor metadata unset. The station route requires the full contract.
  StationRecipeHands hands{};
};
// Own N0 records, no views onto short-lived inventory buffers. All values
// belong to the same native record/generation; this wrapper does not
// re-read. The native adapter must actually evidence the positive flags.
enum class AlchemyExtraActorPurpose { unknown, hero_preview, original_entry };
struct AlchemyExtraCatalogueCapture {
  CraftingKey key{};
  std::uint64_t serial = 0;
  AlchemyExtraActorPurpose actor_purpose = AlchemyExtraActorPurpose::unknown;
  bool physical_hero_preview_verified = false;
  bool boundary_verified = false, context_unchanged = false, hands_unchanged = false,
       actor_unchanged = false;
  alchemy_flask_debit::Context context_before{}, context_after{};
  std::uint32_t manager_before = 0, manager_after = 0;
  std::int32_t production_before = -1, production_after = -1;
  StationRecipeHands hands_before{}, hands_after{};
  station_recipe_runtime::ActorEvidence actor_before{}, actor_after{};
  std::array<AlchemyExtraSnapshot, alchemy_recipe_catalog::recipes.size()> recipes{};
  std::array<bool, alchemy_recipe_catalog::recipes.size()> evaluated{};
};
struct CraftingRequest {
  crafting_catalog::RecipeId id = UINT32_MAX;
  std::uint32_t quantity = 0;
  CraftingKey key{};
  CraftingState material_snapshot{};
  StationRecipeHands hands{};
};
enum class CraftingBlockReason {
  none,
  no_known_recipes,
  contract_unverified,
  actor_unknown,
  actor_changed,
  hand_unknown,
  material_unknown,
  material_unverified,
  missing_material,
  output_capacity,
  tool_return_capacity,
  unavailable,
  quantity_unsupported,
  equipped_ingredient
};
inline CraftingBlockReason crafting_display_reason(CraftingBlockReason reason,
                                                   const CraftingState* state) noexcept {
  return reason == CraftingBlockReason::material_unverified && state && state->equipped_ingredient
             ? CraftingBlockReason::equipped_ingredient
             : reason;
}
struct CraftingAmountPreview {
  bool available_known = false, ready_known = false, end_known = false;
  std::int64_t available = -1, required = 0, produced = 0, missing = -1, ready_storage = -1,
               end_storage = -1;
  std::int64_t output_shortfall = -1, end_overflow = -1;
  std::int64_t original_minimum = 0, minimum_missing = -1;
};
inline CraftingAmountPreview crafting_amount_preview(const CraftingMaterialValue& row,
                                                     std::uint32_t quantity) noexcept {
  CraftingAmountPreview value;
  if (quantity > static_cast<std::uint32_t>(INT32_MAX))
    return value;
  value.required = std::int64_t{row.unit_cost} * quantity;
  value.produced = std::int64_t{row.unit_output} * quantity;
  value.original_minimum =
      std::max<std::int64_t>(row.fixed_minimum, std::int64_t{row.unit_minimum} * quantity);
  if (row.stored < 0 || row.consumed_reserve < 0)
    return value;
  value.available_known = true;
  value.available = std::int64_t{row.stored} + row.consumed_reserve;
  value.missing = value.required > value.available ? value.required - value.available : 0;
  value.minimum_missing =
      value.original_minimum > value.available ? value.original_minimum - value.available : 0;
  if (value.minimum_missing > value.missing)
    value.missing = value.minimum_missing;
  const auto rest = value.available - value.required;
  if (value.produced <= INT64_MAX - (rest > 0 ? rest : 0)) {
    value.ready_known = true;
    value.ready_storage = rest + value.produced;
    if (row.tool_reserve >= 0 && value.ready_storage <= INT64_MAX - row.tool_reserve) {
      value.end_known = true;
      value.end_storage = value.ready_storage + row.tool_reserve;
      value.end_overflow = value.end_storage > INT32_MAX ? value.end_storage - INT32_MAX : 0;
    }
  }
  const auto space = std::int64_t{INT32_MAX} - row.stored;
  value.output_shortfall = value.produced > space ? value.produced - space : 0;
  return value;
}
namespace crafting_detail {
inline bool valid_key(const CraftingKey& key) noexcept {
  using crafting_catalog::Station;
  return key.generation && key.thread && key.world && key.hero && key.station && key.manager &&
         (key.kind == Station::stove || key.kind == Station::alchemy ||
          key.kind == Station::leather || key.kind == Station::saw || key.kind == Station::forge ||
          key.kind == Station::anvil || key.kind == Station::bow || key.kind == Station::cool ||
          key.kind == Station::grind);
}
inline const CraftingMaterialValue* material(const CraftingState& state,
                                             std::int32_t symbol) noexcept {
  if (state.target_count > state.materials.size())
    return nullptr;
  for (std::size_t i = 0; i < state.target_count; ++i)
    if (state.materials[i].symbol == symbol)
      return &state.materials[i];
  return nullptr;
}
inline bool hand_value(std::int32_t actual, std::int32_t expected, bool known) noexcept {
  return known ? actual == expected : actual == expected || (expected && actual == -1);
}
inline bool bound_hands(const crafting_catalog::Descriptor& d,
                        const StationRecipeHands& hands) noexcept {
  if (d.route == crafting_catalog::Route::grind) {
    const auto& l = hands.hands.left;
    const auto& r = hands.hands.right;
    return d.grind && grind_recipe::structurally_valid(*d.grind) && hands.left_slot_known &&
           hands.right_slot_known && hands.interact_item_known && l.state == 1 && l.object &&
           l.symbol == grind_recipe::lifecycle_item && l.amount == 1 && r.state == 0 && !r.object &&
           r.symbol == -1 && r.amount == -1 && hands.interact_item == l.object &&
           (hands.left_tmp_level == 1 || hands.left_tmp_level == 2);
  }
  if (d.route == crafting_catalog::Route::cool) {
    const auto& l = hands.hands.left;
    const auto& r = hands.hands.right;
    return d.cool && cool_recipe_catalog::structurally_valid(*d.cool) && hands.left_slot_known &&
           hands.right_slot_known && hands.interact_item_known && l.state == 1 && l.object &&
           l.symbol == 16612 && l.amount == 1 && r.state == 0 && !r.object && r.symbol == -1 &&
           r.amount == -1 && hands.interact_item == l.object &&
           (hands.left_tmp_level == 1 || hands.left_tmp_level == 2);
  }
  if (d.route == crafting_catalog::Route::bow) {
    const auto empty = [](const HandObjectSnapshot& h) noexcept {
      return h.state == 0 && !h.object && h.symbol == -1 && h.amount == -1;
    };
    return d.bow && hands.left_slot_known && hands.right_slot_known && hands.interact_item_known &&
           !hands.interact_item && empty(hands.hands.left) && empty(hands.hands.right);
  }
  if (d.route == crafting_catalog::Route::base) {
    const auto& want = d.base->hand;
    const auto& left = hands.hands.left;
    const auto& right = hands.hands.right;
    return left.state == 0 && !left.object && right.state == 1 && right.object &&
           right.symbol == want.active_item_symbol &&
           right.amount == static_cast<std::int32_t>(want.amount);
  }
  if (d.route == crafting_catalog::Route::dndr_alchemy || d.tobacco ||
      d.route == crafting_catalog::Route::alchemy_extra ||
      d.route == crafting_catalog::Route::spirit) {
    const auto& left = hands.hands.left;
    const auto& right = hands.hands.right;
    const auto flask =
        d.route == crafting_catalog::Route::dndr_alchemy ? static_cast<int>(dndr::flask) : 16604;
    return (d.route == crafting_catalog::Route::dndr_alchemy || d.tobacco || d.alchemy_extra ||
            d.spirit) &&
           hands.left_slot_known && hands.right_slot_known && hands.interact_item_known &&
           left.state == 0 && !left.object && right.state == 1 && right.object &&
           right.symbol == flask && right.amount == 1 && hands.interact_item == right.object;
  }
  if (d.route == crafting_catalog::Route::anvil || d.route == crafting_catalog::Route::ore) {
    const auto* want = d.route == crafting_catalog::Route::anvil
                           ? (d.anvil ? &d.anvil->hand : nullptr)
                           : (d.ore ? &d.ore->hand : nullptr);
    if (!want)
      return false;
    return station_recipe_material_detail::check_hand(
               want->left, hands.hands.left, hands.left_slot_known, hands.left_tmp_level) ==
               StationRecipeHandIssue::none &&
           station_recipe_material_detail::check_hand(
               want->right, hands.hands.right, hands.right_slot_known, hands.right_tmp_level) ==
               StationRecipeHandIssue::none &&
           hands.left_tmp_level == want->left_tmp_level &&
           hands.right_tmp_level == want->right_tmp_level &&
           hands.hands.left.object != hands.hands.right.object && hands.interact_item_known &&
           hands.interact_item == hands.hands.right.object;
  }
  if (d.route != crafting_catalog::Route::station || !d.station_descriptor)
    return false;
  const auto& want = d.station_descriptor->hand;
  const auto anchor = want.anchor == station_recipe_catalog::HandSide::left
                          ? hands.hands.left.object
                          : hands.hands.right.object;
  return station_recipe_material_detail::check_hand(want.left, hands.hands.left,
                                                    hands.left_slot_known, hands.left_tmp_level) ==
             StationRecipeHandIssue::none &&
         station_recipe_material_detail::check_hand(
             want.right, hands.hands.right, hands.right_slot_known, hands.right_tmp_level) ==
             StationRecipeHandIssue::none &&
         (!hands.hands.left.object || hands.hands.left.object != hands.hands.right.object) &&
         anchor && hands.interact_item_known && hands.interact_item == anchor;
}
} // namespace crafting_detail
// Compare only our previously validated values for equality. This does not authorize anything
// independently. Two unknown values comparing equal cannot replace knowledge, hand or actor evidence.
inline bool same_crafting_hands(const StationRecipeHands& a, const StationRecipeHands& b) noexcept {
  return station_recipe_snapshot_detail::same_object(a.hands.left, b.hands.left) &&
         station_recipe_snapshot_detail::same_object(a.hands.right, b.hands.right) &&
         a.left_slot_known == b.left_slot_known && a.right_slot_known == b.right_slot_known &&
         a.left_tmp_level == b.left_tmp_level && a.right_tmp_level == b.right_tmp_level &&
         a.interact_item_known == b.interact_item_known && a.interact_item == b.interact_item;
}
inline bool same_crafting_state(const CraftingState& a, const CraftingState& b) noexcept {
  if (a.id != b.id || a.generation != b.generation || a.evaluated != b.evaluated ||
      a.knowledge_read_valid != b.knowledge_read_valid || a.known != b.known ||
      a.contract != b.contract || a.actor_known != b.actor_known ||
      a.actor_matches != b.actor_matches || a.material_verified != b.material_verified ||
      a.hand_known != b.hand_known || a.material_known != b.material_known ||
      a.context_known != b.context_known || a.context_matches != b.context_matches ||
      a.branch_known != b.branch_known || a.branch_value != b.branch_value ||
      a.branch_output != b.branch_output || !(a.grind == b.grind) || a.maximum != b.maximum ||
      a.target_count != b.target_count || a.target_count > a.materials.size() ||
      b.target_count > b.materials.size())
    return false;
  for (std::size_t i = 0; i < a.target_count; ++i) {
    for (std::size_t j = 0; j < i; ++j)
      if (a.materials[j].symbol == a.materials[i].symbol ||
          b.materials[j].symbol == b.materials[i].symbol)
        return false;
    const auto& value = a.materials[i];
    const auto* other = crafting_detail::material(b, value.symbol);
    if (!other || value.stored != other->stored ||
        value.consumed_reserve != other->consumed_reserve ||
        value.tool_reserve != other->tool_reserve ||
        value.generated_prop != other->generated_prop || value.unit_cost != other->unit_cost ||
        value.unit_output != other->unit_output || value.fixed_minimum != other->fixed_minimum ||
        value.unit_minimum != other->unit_minimum)
      return false;
  }
  return true;
}
inline bool valid_crafting_state(const CraftingState& state) noexcept {
  namespace cat = crafting_catalog;
  const auto* d = cat::find(state.id);
  if (!d || !state.target_count || state.target_count != d->targets.count ||
      state.target_count > state.materials.size() ||
      state.maximum > static_cast<std::uint32_t>(INT32_MAX) ||
      (state.known && !state.knowledge_read_valid) || (state.actor_matches && !state.actor_known) ||
      (state.context_matches && !state.context_known))
    return false;
  if (!cat::valid_branch(*d, state.branch_known, state.branch_value, state.branch_output) ||
      (d->route == cat::Route::spirit && !state.branch_known &&
       (state.material_known || state.material_verified || state.maximum)))
    return false;
  if (d->grind) {
    if (state.grind.known != state.knowledge_read_valid ||
        state.known != (state.grind.known && state.grind.talent == 1 &&
                        (state.grind.guild == d->grind->guilds[0] ||
                         state.grind.guild == d->grind->guilds[1])))
      return false;
  } else if (!(state.grind == CraftingGrindValues{}))
    return false;
  if (!state.grind.known && (state.grind.talent || state.grind.guild || state.grind.skill))
    return false;
  const auto outputs = cat::outputs_for(*d, state.branch_known, state.branch_value);
  std::uint64_t numeric = INT32_MAX;
  for (std::size_t i = 0; i < state.target_count; ++i) {
    const auto& row = state.materials[i];
    bool found = false;
    for (const auto& target : d->targets)
      if (target.symbol == row.symbol)
        found = true;
    const auto hand = cat::hand_amounts(*d, row.symbol);
    if (!found || row.stored < -1 || (state.material_known && row.stored < 0) ||
        row.unit_cost != cat::units(d->costs, row.symbol) ||
        row.unit_output != cat::units(outputs, row.symbol) ||
        row.fixed_minimum !=
            (d->tobacco && row.symbol == d->tobacco->targets[2].symbol ? 1u : 0u) ||
        row.unit_minimum != (d->bow ? cat::units(d->bow->minima, row.symbol) : 0u) ||
        !crafting_detail::hand_value(row.consumed_reserve, hand.consumed, state.hand_known) ||
        !crafting_detail::hand_value(row.tool_reserve, hand.tool, state.hand_known) ||
        !crafting_detail::hand_value(row.generated_prop, hand.generated, state.hand_known))
      return false;
    for (std::size_t j = 0; j < i; ++j)
      if (state.materials[j].symbol == row.symbol)
        return false;
    if (state.material_known && state.hand_known) {
      const auto limit = [&](std::uint64_t n) {
        if (n < numeric)
          numeric = n;
      };
      if (row.unit_cost) {
        limit((std::uint64_t{static_cast<std::uint32_t>(row.stored)} +
               static_cast<std::uint32_t>(row.consumed_reserve)) /
              row.unit_cost);
        limit(static_cast<std::uint32_t>(INT32_MAX) / row.unit_cost);
      }
      if (row.unit_output)
        limit(static_cast<std::uint32_t>(INT32_MAX - row.stored) / row.unit_output);
      if (row.stored < static_cast<int>(row.fixed_minimum))
        limit(0);
      if (row.unit_minimum)
        limit(static_cast<std::uint32_t>(row.stored) / row.unit_minimum);
      if (std::int64_t{row.stored} + row.tool_reserve > INT32_MAX)
        limit(0);
      if (hand.stored_absent && row.stored)
        limit(0);
      // Original yields the cold blade once between callback and end.
      if (d->grind && row.symbol == grind_recipe::lifecycle_item && row.stored == INT32_MAX)
        limit(0);
    }
  }
  return !state.material_known || !state.hand_known || state.maximum <= numeric;
}
inline bool valid_crafting_snapshot(const CraftingSnapshot& snapshot) noexcept {
  if (snapshot.issue != CraftingProjectionIssue::none || !crafting_detail::valid_key(snapshot.key))
    return false;
  const CraftingGrindValues* grind_values = nullptr;
  for (std::size_t i = 0; i < snapshot.recipes.size(); ++i) {
    const auto& s = snapshot.recipes[i];
    if (!valid_crafting_state(s))
      return false;
    if (s.grind.known) {
      if (grind_values && !(*grind_values == s.grind))
        return false;
      grind_values = &s.grind;
    }
    for (std::size_t j = 0; j < i; ++j)
      if (snapshot.recipes[j].id == s.id)
        return false;
    const bool evidence = s.evaluated || s.known || s.knowledge_read_valid || s.contract ||
                          s.actor_known || s.actor_matches || s.material_known ||
                          s.material_verified || s.hand_known || s.maximum || s.context_known ||
                          s.context_matches || s.branch_known || s.grind.known;
    if (evidence && (!s.evaluated || s.generation != snapshot.key.generation ||
                     crafting_catalog::find(s.id)->station != snapshot.key.kind))
      return false;
    if (s.hand_known &&
        !crafting_detail::bound_hands(*crafting_catalog::find(s.id), snapshot.hands))
      return false;
  }
  return true;
}
inline CraftingBlockReason crafting_block_reason(const CraftingState& state) noexcept {
  using Reason = CraftingBlockReason;
  const auto* d = crafting_catalog::find(state.id);
  if (!d || !state.known || !state.knowledge_read_valid)
    return Reason::no_known_recipes;
  if (!state.contract)
    return Reason::contract_unverified;
  if (d->route != crafting_catalog::Route::base && !state.actor_known)
    return Reason::actor_unknown;
  if (d->route != crafting_catalog::Route::base && !state.actor_matches)
    return Reason::actor_changed;
  if (crafting_catalog::requires_context(d->route) && !state.context_known)
    return Reason::actor_unknown;
  if (crafting_catalog::requires_context(d->route) && !state.context_matches)
    return Reason::unavailable;
  if (!state.material_known)
    return Reason::material_unknown;
  if (!state.hand_known)
    return Reason::hand_unknown;
  if (!state.material_verified)
    return Reason::material_unverified;
  if (state.maximum)
    return Reason::none;
  if (d->grind) {
    const auto* cold = crafting_detail::material(state, grind_recipe::lifecycle_item);
    if (cold && cold->stored == INT32_MAX)
      return Reason::output_capacity;
  }
  for (std::size_t i = 0; i < state.target_count && i < state.materials.size(); ++i)
    if (crafting_amount_preview(state.materials[i], 1).missing > 0)
      return Reason::missing_material;
  for (std::size_t i = 0; i < state.target_count && i < state.materials.size(); ++i) {
    const auto p = crafting_amount_preview(state.materials[i], 1);
    if (p.output_shortfall > 0)
      return Reason::output_capacity;
    if (p.end_overflow > 0)
      return Reason::tool_return_capacity;
  }
  return Reason::unavailable;
}

// Only numbers from already bound own material values. The current INFO
// lease as well as hand/destroy/end/display witnesses are not created here.
inline cool_recipe::Evidence crafting_cool_evidence(const CraftingState& state) noexcept {
  const auto* d = crafting_catalog::find(state.id);
  if (!d || d->route != crafting_catalog::Route::cool || !d->cool || !valid_crafting_state(state) ||
      !state.material_known || !state.material_verified || !state.hand_known)
    return {};
  cool_recipe::Evidence evidence;
  evidence.stable_boundary_verified = true;
  for (std::size_t i = 0; i < cool_recipe_catalog::targets.size(); ++i) {
    const auto symbol = cool_recipe_catalog::targets[i].symbol;
    const auto* row = crafting_detail::material(state, symbol);
    if (!row)
      return {};
    evidence.stocks[i] = {symbol, row->stored, true, true};
  }
  return evidence;
}
inline cool_recipe::Plan crafting_cool_plan(const CraftingState& state,
                                            std::uint32_t quantity) noexcept {
  return cool_recipe::plan(cool_recipe_catalog::recipe, crafting_cool_evidence(state), quantity);
}
inline grind_recipe::Evidence crafting_grind_evidence(const CraftingState& state) noexcept {
  grind_recipe::Evidence out;
  const auto* d = crafting_catalog::find(state.id);
  if (!d || !d->grind || !valid_crafting_state(state) || !state.material_known ||
      !state.hand_known || !state.material_verified || !state.grind.known)
    return out;
  out.callback = d->grind->function.symbol;
  out.stable_boundary_verified = true;
  out.talent_known = out.guild_known = out.skill_known = true;
  out.talent = state.grind.talent;
  out.guild = state.grind.guild;
  out.skill = state.grind.skill;
  for (std::size_t i = 0; i < 3; ++i) {
    const auto* row = crafting_detail::material(state, d->targets[i].symbol);
    if (!row)
      return {};
    out.stocks[i] = {row->symbol, row->stored, true, true};
  }
  return out;
}
inline grind_recipe::Plan crafting_grind_plan(const CraftingState& state,
                                              std::uint32_t quantity) noexcept {
  const auto* d = crafting_catalog::find(state.id);
  return d && d->grind ? grind_recipe::plan(*d->grind, crafting_grind_evidence(state), quantity)
                       : grind_recipe::Plan{};
}
inline CraftingBlockReason crafting_quantity_block_reason(const CraftingState& state,
                                                          std::uint32_t quantity) noexcept {
  const auto base = crafting_block_reason(state);
  if (base != CraftingBlockReason::none)
    return base;
  if (!quantity || quantity > state.maximum || !valid_crafting_state(state))
    return CraftingBlockReason::unavailable;
  const auto* d = crafting_catalog::find(state.id);
  if (d && d->cool && !crafting_cool_plan(state, quantity).valid)
    return CraftingBlockReason::unavailable;
  if (d && d->grind && !crafting_grind_plan(state, quantity).valid)
    return CraftingBlockReason::unavailable;
  return CraftingBlockReason::none;
}
inline bool crafting_quantity_receipt_required(crafting_catalog::RecipeId id,
                                               std::uint32_t quantity) noexcept {
  const auto* d = crafting_catalog::find(id);
  return d && d->route == crafting_catalog::Route::cool && d->cool && quantity > 1 && quantity != 3;
}

// Projections process exclusively own capture values. No second storage
// record, no engine calls and no invented release from metadata. The
// native adapter remains responsible for context before/after capture.
inline CraftingSnapshot project_capture(const RecipeCatalogueCapture& source) noexcept {
  namespace cat = crafting_catalog;
  CraftingSnapshot result;
  const auto& k = source.snapshot.key;
  result.hands.hands = source.hands;
  result.key = {k.generation,
                k.thread,
                k.world,
                k.hero,
                k.station,
                k.manager,
                k.kind == recipe_catalog::Station::stove ? cat::Station::stove
                                                         : cat::Station::alchemy};
  if (!recipe_session_detail::valid_key(k) ||
      source.diagnostic.issue != RecipeCatalogueIssue::none) {
    result.issue = CraftingProjectionIssue::context;
    return result;
  }
  for (std::size_t i = 0; i < recipe_catalog::recipes.size(); ++i) {
    const auto& d = cat::recipes[i];
    if (d.station != result.key.kind)
      continue;
    const auto& s = source.snapshot.recipes[i];
    const auto& diag = source.diagnostic.recipes[i];
    auto& out = result.recipes[i];
    if (static_cast<cat::RecipeId>(s.id) != d.id || !valid_recipe_state(s)) {
      result.issue = CraftingProjectionIssue::source;
      return result;
    }
    if (!diag.applicable || !diag.evaluated)
      continue;
    out.generation = k.generation;
    out.evaluated = true;
    out.knowledge_read_valid = diag.knowledge.read_valid;
    out.known = s.known && out.knowledge_read_valid && diag.knowledge.known;
    out.contract = s.contract && diag.profile.matches &&
                   diag.assembly_issue == recipe_runtime::AssemblyIssue::none;
    out.material_known = s.material_known &&
                         source.diagnostic.inventory_issue == PackedInventoryIssue::none &&
                         source.diagnostic.inventory_known && source.diagnostic.listed_known &&
                         source.diagnostic.packed_known && source.diagnostic.snapshot_unchanged;
    out.hand_known = s.hand_known && out.material_known && source.diagnostic.hands_read_valid;
    out.material_verified =
        out.material_known && out.hand_known && diag.material_issue == RecipeMaterialIssue::none;
    out.equipped_ingredient = out.material_known && diag.equipped_ingredient;
    out.maximum = s.maximum;
    for (std::size_t j = 0; j < out.target_count; ++j) {
      auto& row = out.materials[j];
      const auto* input = recipe_session_detail::material(s, row.symbol);
      if (!input) {
        result.issue = CraftingProjectionIssue::source;
        return result;
      }
      const auto hand = cat::hand_amounts(d, row.symbol);
      if (out.material_known)
        row.stored = input->stored;
      if (hand.consumed)
        row.consumed_reserve = out.hand_known ? input->reserved : -1;
      else if (out.material_known && input->reserved != 0) {
        result.issue = CraftingProjectionIssue::source;
        return result;
      }
      if (hand.generated)
        row.generated_prop = out.hand_known ? hand.generated : -1;
    }
  }
  if (!valid_crafting_snapshot(result))
    result.issue = CraftingProjectionIssue::source;
  return result;
}
inline CraftingSnapshot project_capture(const StationRecipeCatalogueCapture& source) noexcept {
  namespace cat = crafting_catalog;
  CraftingSnapshot result;
  const auto& k = source.key;
  result.hands = source.hands;
  const auto kind = k.kind == station_recipe_catalog::Station::leatherwork ? cat::Station::leather
                    : k.kind == station_recipe_catalog::Station::saw       ? cat::Station::saw
                                                                           : cat::Station::forge;
  result.key = {k.generation, k.thread, k.world, k.hero, k.station, k.manager, kind};
  if (!station_recipe_snapshot_detail::valid_key(k) ||
      source.diagnostic.issue != StationRecipeCaptureIssue::none) {
    result.issue = CraftingProjectionIssue::context;
    return result;
  }
  for (std::size_t i = 0; i < station_recipe_catalog::recipes.size(); ++i) {
    const auto index = recipe_catalog::recipes.size() + i;
    const auto& d = cat::recipes[index];
    if (d.station != kind)
      continue;
    const auto& s = source.recipes[i];
    const auto& diag = source.diagnostic.recipes[i];
    auto& out = result.recipes[index];
    if (static_cast<cat::RecipeId>(s.id) != d.id || s.target_count != d.targets.count ||
        s.material.quantity || s.material.craftable ||
        (s.evaluated && (s.material.count != d.targets.count ||
                         s.material.completion != d.station_descriptor->completion))) {
      result.issue = CraftingProjectionIssue::source;
      return result;
    }
    if (!s.applicable || !s.evaluated)
      continue;
    out.generation = k.generation;
    out.evaluated = true;
    out.knowledge_read_valid = diag.knowledge.read_valid;
    out.known = s.known && out.knowledge_read_valid && diag.knowledge.known;
    out.contract = s.contract && diag.profile.matches &&
                   diag.assembly == station_recipe_runtime::AssemblyIssue::none;
    const auto& before = source.diagnostic.actor_before;
    const auto& after = source.diagnostic.actor_after;
    out.actor_known = s.actor_known && source.diagnostic.actor_unchanged &&
                      station_recipe_runtime::same_actor_evidence(before, after);
    const bool self = d.station_descriptor->material_actor == recipe_catalog::Actor::self;
    out.actor_matches = out.actor_known && s.actor_matches && before.matches && after.matches &&
                        before.requires_self == self && before.hero == k.hero &&
                        after.hero == k.hero &&
                        (!self || (before.self == k.hero && after.self == k.hero));
    out.material_known = s.material.inventory_known &&
                         source.diagnostic.inventory_issue == PackedInventoryIssue::none &&
                         source.diagnostic.inventory_known;
    out.hand_known =
        s.material.hand_known && source.diagnostic.hands_unchanged && out.material_known;
    out.material_verified = out.material_known && out.hand_known &&
                            s.material.issue == StationRecipeMaterialIssue::none &&
                            s.material.hand_issue == StationRecipeHandIssue::none;
    out.equipped_ingredient = out.material_known && s.material.equipped_ingredient;
    out.maximum = s.material.maximum;
    for (std::size_t j = 0; j < out.target_count; ++j) {
      auto& row = out.materials[j];
      const StationRecipeMaterialRow* input = nullptr;
      for (std::size_t n = 0; n < s.target_count; ++n)
        if (s.rows[n].symbol == row.symbol) {
          if (input) {
            result.issue = CraftingProjectionIssue::source;
            return result;
          }
          input = &s.rows[n];
        }
      if (!input || input->unit_cost != row.unit_cost || input->unit_output != row.unit_output ||
          input->stored < -1 || input->stored > INT32_MAX || input->reserved_tool < -1 ||
          input->reserved_tool > INT32_MAX || input->generated_hand < -1 ||
          input->generated_hand > INT32_MAX) {
        result.issue = CraftingProjectionIssue::source;
        return result;
      }
      if (out.material_known)
        row.stored = static_cast<std::int32_t>(input->stored);
      const auto hand = cat::hand_amounts(d, row.symbol);
      if (hand.tool)
        row.tool_reserve = out.hand_known ? static_cast<std::int32_t>(input->reserved_tool) : -1;
      else if (input->reserved_tool) {
        result.issue = CraftingProjectionIssue::source;
        return result;
      }
      if (hand.generated)
        row.generated_prop = out.hand_known ? static_cast<std::int32_t>(input->generated_hand) : -1;
      else if (input->generated_hand) {
        result.issue = CraftingProjectionIssue::source;
        return result;
      }
    }
  }
  if (!valid_crafting_snapshot(result))
    result.issue = CraftingProjectionIssue::source;
  return result;
}

namespace crafting_detail {
inline bool stocks_agree(const CraftingSnapshot& value) noexcept {
  for (std::size_t i = 0; i < value.recipes.size(); ++i) {
    const auto& a = value.recipes[i];
    if (!a.material_known)
      continue;
    for (std::size_t j = i + 1; j < value.recipes.size(); ++j) {
      const auto& b = value.recipes[j];
      if (!b.material_known)
        continue;
      for (std::size_t n = 0; n < a.target_count; ++n) {
        const auto* other = material(b, a.materials[n].symbol);
        if (other && other->stored != a.materials[n].stored)
          return false;
      }
    }
  }
  return true;
}
template <class Rows>
inline bool project_station_rows(const crafting_catalog::Descriptor& d, const Rows& rows,
                                 std::size_t count, CraftingState& out) noexcept {
  namespace cat = crafting_catalog;
  if (count != out.target_count || count > rows.size())
    return false;
  for (std::size_t j = 0; j < out.target_count; ++j) {
    auto& row = out.materials[j];
    const StationRecipeMaterialRow* input = nullptr;
    for (std::size_t n = 0; n < count; ++n)
      if (rows[n].symbol == row.symbol) {
        if (input)
          return false;
        input = &rows[n];
      }
    if (!input || input->unit_cost != row.unit_cost || input->unit_output != row.unit_output ||
        input->stored < -1 || input->stored > INT32_MAX || input->reserved_tool < -1 ||
        input->reserved_tool > INT32_MAX || input->generated_hand < -1 ||
        input->generated_hand > INT32_MAX)
      return false;
    if (out.material_known)
      row.stored = static_cast<std::int32_t>(input->stored);
    const auto hand = cat::hand_amounts(d, row.symbol);
    if (hand.tool)
      row.tool_reserve = out.hand_known ? static_cast<std::int32_t>(input->reserved_tool) : -1;
    else if (input->reserved_tool)
      return false;
    if (hand.generated)
      row.generated_prop = out.hand_known ? static_cast<std::int32_t>(input->generated_hand) : -1;
    else if (input->generated_hand)
      return false;
  }
  return true;
}
template <class Source, class Recipes, class SameContext>
inline CraftingSnapshot project_anvil(const Source& source, const Recipes& recipes,
                                      std::size_t begin, SameContext same_context,
                                      bool physical_hero_preview_verified) noexcept {
  namespace cat = crafting_catalog;
  CraftingSnapshot result;
  const auto& k = source.key;
  const auto& diag = source.diagnostic;
  result.key = {k.generation, k.thread, k.world, k.hero, k.station, k.manager, cat::Station::anvil};
  result.hands = source.hands;
  if (!anvil_recipe_snapshot_detail::valid_key(k) || diag.issue != decltype(diag.issue)::none) {
    result.issue = CraftingProjectionIssue::context;
    return result;
  }
  for (std::size_t i = 0; i < recipes.size(); ++i) {
    const auto index = begin + i;
    const auto& d = cat::recipes[index];
    const auto& s = source.recipes[i];
    const auto& rd = diag.recipes[i];
    auto& out = result.recipes[index];
    if (static_cast<cat::RecipeId>(s.id) != d.id || s.target_count != d.targets.count ||
        s.material.quantity || s.material.craftable ||
        (s.evaluated &&
         (s.material.count != d.targets.count ||
          s.material.completion != station_recipe_catalog::Completion::end_production))) {
      result.issue = CraftingProjectionIssue::source;
      return result;
    }
    if (!s.evaluated)
      continue;
    out.generation = k.generation;
    out.evaluated = true;
    out.knowledge_read_valid = rd.knowledge.read_valid;
    out.known = s.known && out.knowledge_read_valid && rd.knowledge.known;
    out.contract = s.contract && rd.profile.matches &&
                   rd.assembly == station_recipe_runtime::AssemblyIssue::none;
    const auto& before = diag.actor_before;
    const auto& after = diag.actor_after;
    out.actor_known = s.actor_known && diag.actor_unchanged &&
                      before.issue == runtime_script::Issue::none &&
                      after.issue == runtime_script::Issue::none &&
                      station_recipe_runtime::same_actor_evidence(before, after);
    // Preview and navigation may use the currently bound physical hero.
    // At the inner CallFunc, the writer must check actor().matches again.
    // The minimum check on SELF and the material effect on HERO remain
    // separate script accesses. The raw actor evidence is unchanged.
    out.actor_matches =
        out.actor_known && before.requires_self && after.requires_self && before.hero == k.hero &&
        after.hero == k.hero &&
        (physical_hero_preview_verified || (s.actor_matches && before.matches && after.matches &&
                                            before.self == k.hero && after.self == k.hero));
    const auto& cb = diag.context_before;
    const auto& ca = diag.context_after;
    out.context_known = s.context_known && diag.context_unchanged &&
                        cb.issue == runtime_script::Issue::none &&
                        ca.issue == runtime_script::Issue::none && same_context(cb, ca);
    // Do not persist category/owner values in CraftingState: an
    // original navigation may switch COMMON/ORE. Only the physical
    // production/start contract remains identical across all stages.
    out.context_matches = out.context_known && cb.production == 1 && ca.production == 1 &&
                          cb.start_smithing == 1 && ca.start_smithing == 1;
    out.material_known = s.material.inventory_known && diag.inventory_known &&
                         diag.inventory_issue == PackedInventoryIssue::none;
    out.hand_known = s.material.hand_known && out.material_known && diag.hands_unchanged &&
                     diag.hand_issue == StationHandReadIssue::none;
    out.material_verified = out.material_known && out.hand_known &&
                            s.material.issue == StationRecipeMaterialIssue::none &&
                            s.material.hand_issue == StationRecipeHandIssue::none;
    out.equipped_ingredient = out.material_known && s.material.equipped_ingredient;
    out.maximum = s.material.maximum;
    if (!project_station_rows(d, s.rows, s.target_count, out)) {
      result.issue = CraftingProjectionIssue::source;
      return result;
    }
  }
  if (!valid_crafting_snapshot(result) || !stocks_agree(result))
    result.issue = CraftingProjectionIssue::source;
  return result;
}
inline bool extra_context(const AlchemyExtraCatalogueCapture& source) noexcept {
  const auto& k = source.key;
  const auto& a = source.context_before;
  const auto& b = source.context_after;
  return valid_key(k) && k.kind == crafting_catalog::Station::alchemy && source.serial &&
         source.boundary_verified && source.context_unchanged && alchemy_flask_debit::valid(a) &&
         alchemy_flask_debit::valid(b) && alchemy_flask_debit::same(a, b) &&
         a.epoch == k.generation && a.thread == k.thread && a.world == k.world &&
         a.hero == k.hero && a.mob == k.station && source.manager_before == k.manager &&
         source.manager_after == k.manager;
}
inline bool extra_hand(const alchemy_flask_debit::Hand& recorded,
                       const StationRecipeHands& actual) noexcept {
  return recorded.known && recorded.left_empty && recorded.right == actual.hands.right.object &&
         recorded.symbol == 16604 && recorded.amount == 1 &&
         recorded.interact_item == actual.interact_item;
}
inline bool project_extra_rows(const crafting_catalog::Descriptor& d,
                               const AlchemyExtraSnapshot& source, CraftingState& out) noexcept {
  if (source.material.count != out.target_count || source.material.count > source.rows.size())
    return false;
  for (std::size_t j = 0; j < out.target_count; ++j) {
    auto& row = out.materials[j];
    const RecipeMaterialRow* input = nullptr;
    for (std::size_t n = 0; n < source.material.count; ++n)
      if (source.rows[n].symbol == row.symbol) {
        if (input)
          return false;
        input = &source.rows[n];
      }
    if (!input || input->unit_cost != row.unit_cost || input->unit_output != row.unit_output ||
        input->stored < -1 || input->stored > INT32_MAX || input->reserved < -1 ||
        input->reserved > INT32_MAX)
      return false;
    if (out.material_known)
      row.stored = static_cast<std::int32_t>(input->stored);
    const auto hand = crafting_catalog::hand_amounts(d, row.symbol);
    if (hand.consumed)
      row.consumed_reserve = out.hand_known ? static_cast<std::int32_t>(input->reserved) : -1;
    else if (input->reserved)
      return false;
  }
  return true;
}
} // namespace crafting_detail

inline CraftingSnapshot project_capture(const AnvilRecipeCatalogueCapture& source,
                                        bool physical_hero_preview_verified = false) noexcept {
  return crafting_detail::project_anvil(
      source, anvil_recipe_catalog::recipes, crafting_catalog::anvil_begin,
      anvil_recipe_runtime::same_context_evidence, physical_hero_preview_verified);
}
inline CraftingSnapshot project_capture(const OreRecipeCatalogueCapture& source,
                                        bool physical_hero_preview_verified = false) noexcept {
  return crafting_detail::project_anvil(
      source, ore_recipe_catalog::recipes, crafting_catalog::ore_begin,
      ore_recipe_runtime::same_context_evidence, physical_hero_preview_verified);
}
inline CraftingSnapshot project_capture(const AlchemyExtraCatalogueCapture& source) noexcept {
  namespace cat = crafting_catalog;
  CraftingSnapshot result;
  result.key = source.key;
  if (!crafting_detail::extra_context(source)) {
    result.issue = CraftingProjectionIssue::context;
    return result;
  }
  const auto& k = source.key;
  const bool hand_stable =
      source.hands_unchanged && same_crafting_hands(source.hands_before, source.hands_after);
  if (hand_stable)
    result.hands = source.hands_before;
  const auto& before = source.actor_before;
  const auto& after = source.actor_after;
  const bool actor_stable = source.actor_unchanged && before.issue == runtime_script::Issue::none &&
                            after.issue == runtime_script::Issue::none &&
                            station_recipe_runtime::same_actor_evidence(before, after);
  for (std::size_t i = 0; i < alchemy_recipe_catalog::recipes.size(); ++i) {
    if (!source.evaluated[i])
      continue;
    const auto index = cat::alchemy_extra_begin + i;
    const auto& d = cat::recipes[index];
    const auto& s = source.recipes[i];
    auto& out = result.recipes[index];
    if (static_cast<cat::RecipeId>(s.id) != d.id || s.serial != source.serial ||
        !alchemy_flask_debit::same(s.context, source.context_before) || s.quantity ||
        s.material.quantity || s.material.craftable || s.can_select || s.effect_ready ||
        s.maximum_executable > s.material.maximum ||
        s.maximum_executable > d.alchemy_extra->callback.maximum_quantity ||
        (s.maximum_executable && (!s.native_debit_verified || !s.material.known())) ||
        (s.maximum_executable > 1 && !s.extra_flask_stack_verified)) {
      result.issue = CraftingProjectionIssue::source;
      return result;
    }
    out.generation = k.generation;
    out.evaluated = true;
    out.knowledge_read_valid = s.learned_known;
    out.known = s.learned_known && s.learned;
    out.contract = s.profile_verified && s.native_debit_verified;
    out.actor_known = actor_stable;
    // The UI plans for the physically bound hero, without emitting the
    // last global SELF value as a new callback context. Before original,
    // HERO recipes must freshly bind SELF for their deficit refund.
    out.actor_matches = out.actor_known && before.read_valid && after.read_valid &&
                        before.requires_self && before.hero == k.hero &&
                        ((source.actor_purpose == AlchemyExtraActorPurpose::hero_preview &&
                          source.physical_hero_preview_verified) ||
                         (source.actor_purpose == AlchemyExtraActorPurpose::original_entry &&
                          before.matches && after.matches && before.self == k.hero));
    out.context_known =
        source.production_before >= 0 && source.production_before == source.production_after;
    out.context_matches = out.context_known && source.production_before == 4;
    out.material_known = s.material.inventory_known;
    out.hand_known = s.material.hand_known && out.material_known && hand_stable &&
                     crafting_detail::extra_hand(s.hand, result.hands) &&
                     crafting_detail::bound_hands(d, result.hands);
    out.material_verified =
        out.material_known && out.hand_known && s.material.issue == RecipeMaterialIssue::none;
    out.equipped_ingredient = out.material_known && s.material.equipped_ingredient;
    out.maximum = s.maximum_executable;
    if (!crafting_detail::project_extra_rows(d, s, out)) {
      result.issue = CraftingProjectionIssue::source;
      return result;
    }
    if (s.extra_flask_stack_verified) {
      const auto* flask = crafting_detail::material(out, 16604);
      if (!flask || !out.material_known ||
          !alchemy_flask_debit::listed_flask(s.flask, flask->stored) ||
          (s.flask.object && s.flask.object == result.hands.hands.right.object)) {
        result.issue = CraftingProjectionIssue::source;
        return result;
      }
      for (std::size_t j = 0; j < i; ++j)
        if (source.evaluated[j] && source.recipes[j].extra_flask_stack_verified) {
          const auto& other = source.recipes[j].flask;
          if (s.flask.object != other.object || s.flask.amount != other.amount ||
              s.flask.flags != other.flags) {
            result.issue = CraftingProjectionIssue::source;
            return result;
          }
        }
    }
  }
  if (!valid_crafting_snapshot(result) || !crafting_detail::stocks_agree(result))
    result.issue = CraftingProjectionIssue::source;
  return result;
}

namespace crafting_detail {
inline CraftingSnapshot merge(CraftingSnapshot first, const CraftingSnapshot& second,
                              crafting_catalog::Route route, bool base_objects_only) noexcept {
  if (!valid_crafting_snapshot(first) || !valid_crafting_snapshot(second) ||
      !(first.key == second.key)) {
    first.issue = CraftingProjectionIssue::context;
    return first;
  }
  const bool same = base_objects_only
                        ? station_recipe_snapshot_detail::same_object(first.hands.hands.left,
                                                                      second.hands.hands.left) &&
                              station_recipe_snapshot_detail::same_object(first.hands.hands.right,
                                                                          second.hands.hands.right)
                        : same_crafting_hands(first.hands, second.hands);
  if (!same) {
    first.issue = CraftingProjectionIssue::source;
    return first;
  }
  // Only the second genuine capture yields new slot/anchor evidence. The
  // first base capture possesses exclusively the just-compared hand objects.
  if (base_objects_only)
    first.hands = second.hands;
  for (std::size_t i = 0; i < first.recipes.size(); ++i)
    if (crafting_catalog::recipes[i].route == route)
      first.recipes[i] = second.recipes[i];
  if (!valid_crafting_snapshot(first) || !stocks_agree(first))
    first.issue = CraftingProjectionIssue::source;
  return first;
}
}
inline CraftingSnapshot project_capture(const RecipeCatalogueCapture& base,
                                        const AlchemyExtraCatalogueCapture& extra) noexcept {
  auto first = project_capture(base);
  if (first.key.kind != crafting_catalog::Station::alchemy) {
    first.issue = CraftingProjectionIssue::context;
    return first;
  }
  return crafting_detail::merge(first, project_capture(extra),
                                crafting_catalog::Route::alchemy_extra, true);
}
inline CraftingSnapshot project_capture(const AnvilRecipeCatalogueCapture& anvil,
                                        const OreRecipeCatalogueCapture& ore,
                                        bool physical_hero_preview_verified = false) noexcept {
  auto result = crafting_detail::merge(project_capture(anvil, physical_hero_preview_verified),
                                       project_capture(ore, physical_hero_preview_verified),
                                       crafting_catalog::Route::ore, false);
  // Even between the two partial snapshots, a physical
  // preview must not obscure an observed SELF/HERO switch.
  if (physical_hero_preview_verified &&
      !station_recipe_runtime::same_actor_evidence(anvil.diagnostic.actor_after,
                                                   ore.diagnostic.actor_before))
    result.issue = CraftingProjectionIssue::context;
  return result;
}
} // namespace ubc
