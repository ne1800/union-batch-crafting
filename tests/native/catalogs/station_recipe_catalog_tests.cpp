// SPDX-License-Identifier: MIT
#include "ubc/catalogs/station_recipe_catalog.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

namespace {
namespace c = ubc::station_recipe_catalog;
unsigned checks = 0, failures = 0;
void check(bool okay, int line) {
  ++checks;
  if (!okay) {
    ++failures;
    std::printf("FAIL line %d\n", line);
  }
}
#define CHECK(x) check((x), __LINE__)
const c::ItemAmount* item(c::View<c::ItemAmount> values, std::int32_t symbol) {
  for (const auto& value : values)
    if (value.symbol == symbol)
      return &value;
  return nullptr;
}
const c::RequirementCheck* requirement(const c::Descriptor& recipe, std::int32_t symbol) {
  for (const auto& value : recipe.checks)
    if (value.requirement.symbol == symbol)
      return &value;
  return nullptr;
}
template <class T> c::View<T> view(const std::vector<T>& values) {
  return {values.data(), values.size()};
}
template <class Edit> void rejects(const c::Descriptor& original, Edit edit) {
  auto copy = original;
  edit(copy);
  CHECK(!c::structurally_valid(copy));
  CHECK(c::callback_for_quantity(copy, 1) == nullptr);
}

void recipe_facts() {
  CHECK(!c::runtime_ready);
  CHECK(c::recipes.size() == 16 && c::stations.size() == 3 && c::functions.size() == 51);
  CHECK(c::catalog_target_capacity == 5);
  CHECK(c::find(static_cast<c::RecipeId>(0)) == nullptr);
  CHECK(c::find(static_cast<c::RecipeId>(UINT32_MAX)) == nullptr);
  CHECK(c::function_for(0) == nullptr &&
        c::function_for(21005) == nullptr); // Never use ALL as the template for N.
  struct Leather {
    c::RecipeId id;
    std::int32_t input;
    std::uint32_t callback, yield;
  };
  const std::array<Leather, 9> leather{{{c::RecipeId::leather_sheep, 16121, 20852, 1},
                                        {c::RecipeId::leather_wolf, 16122, 20853, 1},
                                        {c::RecipeId::leather_lurker, 16132, 20854, 1},
                                        {c::RecipeId::leather_boar, 16134, 20855, 1},
                                        {c::RecipeId::leather_warg, 16133, 20856, 1},
                                        {c::RecipeId::leather_shark, 16138, 20857, 2},
                                        {c::RecipeId::leather_shadowbeast, 16137, 20858, 3},
                                        {c::RecipeId::leather_troll, 16139, 20859, 4},
                                        {c::RecipeId::leather_black_troll, 16140, 20860, 6}}};
  for (const auto& expected : leather) {
    const auto* recipe = c::find(expected.id);
    CHECK(recipe != nullptr);
    if (!recipe)
      continue;
    CHECK(c::structurally_valid(*recipe));
    CHECK(recipe->station == c::Station::leatherwork && recipe->production_context == 12);
    CHECK(recipe->costs.count == 1 && recipe->outputs.count == 1 && recipe->targets.count == 3);
    const auto* cost = item(recipe->costs, expected.input);
    CHECK(cost && cost->units == 1);
    const auto* output = item(recipe->outputs, 16599);
    CHECK(output && output->units == expected.yield);
    CHECK(recipe->owner.instance == 20848 && recipe->owner.number == 2 &&
          recipe->back->symbol == 20851);
    CHECK(recipe->callback.function->symbol == expected.callback);
    CHECK(recipe->completion == c::Completion::end_production && recipe->callback.ends_production);
    CHECK(!recipe->callback.clears_choices && recipe->material_actor == c::Actor::hero);
    CHECK(recipe->callback.effects[0].action == c::MaterialAction::remove);
    CHECK(recipe->callback.effects[1].action == c::MaterialAction::create);
    CHECK(recipe->hand.right.symbol == 17342 &&
          recipe->hand.right.source == c::HandSource::generated);
    CHECK(recipe->hand.right.require_stored_absent && recipe->hand.anchor == c::HandSide::right);
    CHECK(recipe->hand.left.source == c::HandSource::empty && recipe->hand.left.symbol == -1);
    CHECK(recipe->station_profile->entry->symbol == 20844 &&
          recipe->station_profile->end.information->symbol == 20847);
    CHECK(recipe->station_profile->mob_condition->symbol == 20861);
    CHECK(c::detail::text(recipe->station_profile->on_state, "LEATHERWORK"));
    CHECK(c::detail::text(recipe->station_profile->use_with_item, ""));
    const auto* learn = requirement(*recipe, 10483);
    const auto* context = requirement(*recipe, 10536);
    CHECK(learn && learn->site == c::CheckSite::callback &&
          learn->function->symbol == expected.callback);
    CHECK(learn && learn->requirement.value == 1 &&
          learn->requirement.role == c::RequirementRole::knowledge);
    CHECK(context && context->site == c::CheckSite::mob_condition &&
          context->function->symbol == 20861);
    CHECK(context && context->requirement.value == 1 &&
          context->requirement.role == c::RequirementRole::context);
    CHECK(recipe->owner.conditions.count == 1 && recipe->owner.conditions[0].symbol == 12330);
  }
  struct Ammo {
    c::RecipeId id;
    std::uint32_t callback;
    std::int32_t owner, output, extra;
    std::uint32_t yield;
  };
  const std::array<Ammo, 5> ammo{{{c::RecipeId::arrows, 20091, 20087, 16804, 0, 30},
                                  {c::RecipeId::fire_arrows, 20094, 20087, 15946, 16654, 10},
                                  {c::RecipeId::magic_arrows, 20096, 20087, 15945, 16655, 10},
                                  {c::RecipeId::bolts, 20102, 20098, 16805, 0, 30},
                                  {c::RecipeId::magic_bolts, 20105, 20098, 15949, 16655, 10}}};
  for (const auto& expected : ammo) {
    const auto* recipe = c::find(expected.id);
    CHECK(recipe != nullptr);
    if (!recipe)
      continue;
    CHECK(c::structurally_valid(*recipe));
    CHECK(recipe->station == c::Station::saw && recipe->production_context == 17);
    CHECK(recipe->costs.count == (expected.extra ? 3u : 2u) && recipe->outputs.count == 1);
    for (const auto symbol : {16610, 16595}) {
      const auto* cost = item(recipe->costs, symbol);
      CHECK(cost && cost->units == 1);
    }
    if (expected.extra) {
      const auto* cost = item(recipe->costs, expected.extra);
      CHECK(cost && cost->units == 1);
    }
    const auto* output = item(recipe->outputs, expected.output);
    CHECK(output && output->units == expected.yield);
    CHECK(item(recipe->costs, 16609) == nullptr && item(recipe->outputs, 16609) == nullptr);
    CHECK(recipe->owner.instance == expected.owner && recipe->owner.number == 2);
    CHECK(recipe->back->symbol == static_cast<std::uint32_t>(expected.owner + 3));
    CHECK(recipe->callback.function->symbol == expected.callback);
    CHECK(recipe->completion == c::Completion::ready_same_station &&
          !recipe->callback.ends_production);
    CHECK(recipe->callback.clears_choices && recipe->material_actor == c::Actor::self);
    CHECK(c::detail::text(recipe->quantity_unit, "Lose"));
    CHECK(recipe->hand.right.symbol == 16609 &&
          recipe->hand.right.source == c::HandSource::reserved_inventory_tool);
    CHECK(!recipe->hand.right.require_stored_absent && recipe->hand.anchor == c::HandSide::right);
    CHECK(recipe->station_profile->entry->symbol == 20083 &&
          recipe->station_profile->end.information->symbol == 20086);
    CHECK(recipe->station_profile->mob_condition == nullptr);
    CHECK(c::detail::text(recipe->station_profile->scheme, "BAUMSAEGE"));
    CHECK(c::detail::text(recipe->station_profile->on_state, "WOODSAW"));
    CHECK(c::detail::text(recipe->station_profile->use_with_item, "ITMI_SAW"));
    const auto* learn = requirement(*recipe, 10491);
    CHECK(learn && learn->site == c::CheckSite::owner_information &&
          learn->function == recipe->owner.information);
    CHECK(learn && learn->requirement.value == 1 &&
          learn->requirement.role == c::RequirementRole::knowledge);
    CHECK(recipe->owner.conditions.count == 1 && recipe->owner.conditions[0].symbol == 12330);
    for (std::size_t i = 0; i < recipe->costs.count; ++i)
      CHECK(recipe->callback.effects[i].action == c::MaterialAction::remove);
    CHECK(recipe->callback.effects[recipe->costs.count].action == c::MaterialAction::create);
  }
  for (const auto id : {c::RecipeId::heat_raw_steel, c::RecipeId::form_hot_blade}) {
    const auto* recipe = c::find(id);
    CHECK(recipe != nullptr);
    if (!recipe)
      continue;
    const bool raw = id == c::RecipeId::heat_raw_steel;
    CHECK(c::structurally_valid(*recipe));
    CHECK(recipe->station == c::Station::forge && recipe->production_context == 10);
    CHECK(recipe->costs.count == 1 && recipe->costs[0].symbol == (raw ? 16610 : 16611) &&
          recipe->costs[0].units == 1);
    CHECK(recipe->outputs.count == 1 && recipe->outputs[0].symbol == (raw ? 16611 : 16612) &&
          recipe->outputs[0].units == 1);
    CHECK(recipe->owner.instance == (raw ? 20991 : 20999) && recipe->owner.number == (raw ? 1 : 4));
    CHECK(recipe->callback.function->symbol == (raw ? 20995u : 21003u));
    CHECK(recipe->back->symbol == (raw ? 20994u : 21002u));
    CHECK(recipe->hand.left.symbol == 16666 && recipe->hand.right.symbol == 16667 &&
          recipe->hand.anchor == c::HandSide::right);
    CHECK(recipe->hand.left.source == c::HandSource::generated &&
          recipe->hand.right.source == c::HandSource::generated);
    CHECK(!recipe->hand.left.require_stored_absent && recipe->hand.right.require_stored_absent);
    CHECK(recipe->requirements.count == 1 &&
          recipe->checks.count == 1); // Do not assume smithing knowledge.
    CHECK(recipe->completion == c::Completion::end_production &&
          recipe->material_actor == c::Actor::hero);
    CHECK(recipe->callback.effects[0].action == c::MaterialAction::create);
    CHECK(recipe->callback.effects[1].action == c::MaterialAction::remove);
    CHECK(recipe->station_profile->entry->symbol == 20987 &&
          recipe->station_profile->end.information->symbol == 20990);
    CHECK(c::detail::text(recipe->station_profile->scheme, "BSFIRE") &&
          recipe->station_profile->mob_condition == nullptr);
  }
}

void quantities_and_identity() {
  for (const auto& recipe : c::recipes) {
    CHECK(c::structurally_valid(recipe));
    CHECK(recipe.hand.static_lifecycle_verified && !recipe.hand.observed_in_world);
    CHECK(!recipe.callback.quantity_text_reviewed && recipe.callback.failure_effects.count == 0);
    CHECK(recipe.callback.invocation == c::Invocation::choice &&
          recipe.callback.original_quantity == 1);
    CHECK(recipe.callback.amount.count == recipe.costs.count * 2 + recipe.outputs.count);
    CHECK(recipe.back->byte_length == 11 && recipe.back->argc == 0 &&
          recipe.back->return_type == 0);
    CHECK(recipe.station_profile->end.number == 999 && recipe.station_profile->end.permanent);
    CHECK(c::binding_for(recipe, 12330) && !c::binding_for(recipe, 12330)->check_value);
    CHECK(c::binding_for(recipe, 11998) && c::binding_for(recipe, 11998)->check_value &&
          c::binding_for(recipe, 11998)->value == 4);
    CHECK(c::binding_for(recipe, 1563) && c::binding_for(recipe, 1563)->check_offset &&
          c::binding_for(recipe, 1563)->offset == 648);
    CHECK(c::binding_for(recipe, -1) == nullptr && !c::has_target(recipe, -1));
    for (const auto& requirement : recipe.requirements) {
      const auto* bound = c::binding_for(recipe, requirement.symbol);
      CHECK(bound && bound->type == 2 && !bound->check_value); // No dynamic DAT initial values.
    }
    std::uint32_t bound = INT32_MAX;
    for (const auto& output : recipe.outputs)
      bound = std::min(bound, static_cast<std::uint32_t>(INT32_MAX) / output.units);
    for (const auto n : {1u, 2u, 5u, 7u, 10u, 23u, bound}) {
      const auto* callback = c::callback_for_quantity(recipe, n);
      CHECK(callback == &recipe.callback);
      if (!callback)
        continue;
      for (std::size_t i = 0; i < callback->amount.count; ++i) {
        const auto& operand = callback->amount.operands[i];
        const auto& use = recipe.operand_uses[i];
        CHECK(!operand.reserved && !operand.allow_zero && operand.offset == use.offset);
        CHECK(std::uint64_t{operand.units} * n > 0 &&
              std::uint64_t{operand.units} * n <= INT32_MAX);
        const auto* amount = item(
            use.role == c::AmountRole::create_count ? recipe.outputs : recipe.costs, use.symbol);
        CHECK(amount && amount->units == operand.units);
      }
    }
    CHECK(c::callback_for_quantity(recipe, 0) == nullptr);
    CHECK(c::callback_for_quantity(recipe, bound + 1) == nullptr);
    CHECK(c::callback_for_quantity(recipe, UINT32_MAX) == nullptr);
    // Our synthetic pattern must never count as the verified game body.
    ubc::Sha256 hash;
    std::array<std::uint8_t, 4096> own{};
    hash.update(own.data(), recipe.callback.function->byte_length);
    CHECK(hash.finish() != recipe.callback.function->body_sha256);
  }
  for (std::size_t i = 0; i < c::functions.size(); ++i) {
    const auto& function = c::functions[i];
    CHECK(c::detail::known_function(&function));
    CHECK(c::function_for(function.symbol) == &function);
    for (std::size_t j = 0; j < i; ++j) {
      CHECK(function.symbol != c::functions[j].symbol);
      CHECK(function.byte_offset != c::functions[j].byte_offset);
    }
  }
  CHECK(c::function_for(20091)->body_sha256 ==
        c::digest_literal("3534c932091e25c90e0d59ac07f8850b03c072ef475475366df00e2166acbd40"));
  CHECK(c::function_for(20860)->body_sha256 ==
        c::digest_literal("76c2326582ce56fb274bffe088fa919413aa5cd2556589d0d289180840d4c418"));
  CHECK(c::function_for(21003)->body_sha256 ==
        c::digest_literal("71a855c0666bc3a279bf1d54d0d337b981cd424df8b0bc4c006c467f21a7e0bf"));
}

void malformed_descriptors() {
  for (const auto& recipe : c::recipes) {
    rejects(recipe, [](auto& r) { r.id = static_cast<c::RecipeId>(99); });
    rejects(recipe, [](auto& r) { r.name = nullptr; });
    rejects(recipe, [](auto& r) { r.name = ""; });
    rejects(recipe, [](auto& r) { r.quantity_unit = nullptr; });
    rejects(recipe, [](auto& r) { r.station = static_cast<c::Station>(999); });
    rejects(recipe, [](auto& r) { ++r.production_context; });
    rejects(recipe, [](auto& r) { r.material_actor = static_cast<c::Actor>(99); });
    rejects(recipe, [](auto& r) { r.completion = static_cast<c::Completion>(99); });
    rejects(recipe, [](auto& r) { r.targets.data = nullptr; });
    rejects(recipe, [](auto& r) { ++r.targets.count; });
    rejects(recipe, [](auto& r) { r.bindings.data = nullptr; });
    rejects(recipe, [](auto& r) { --r.bindings.count; });
    rejects(recipe, [](auto& r) { r.costs.data = nullptr; });
    rejects(recipe, [](auto& r) { r.outputs.count = 0; });
    rejects(recipe, [](auto& r) { r.requirements.data = nullptr; });
    rejects(recipe, [](auto& r) { r.checks.data = nullptr; });
    rejects(recipe, [](auto& r) { ++r.checks.count; });
    rejects(recipe, [](auto& r) { r.owner.instance = 20337; });
    rejects(recipe, [](auto& r) { ++r.owner.number; });
    rejects(recipe, [](auto& r) { r.owner.npc = 1679; });
    rejects(recipe, [](auto& r) { r.owner.important = true; });
    rejects(recipe, [](auto& r) { r.owner.trade = true; });
    rejects(recipe, [](auto& r) { r.owner.permanent = false; });
    rejects(recipe, [](auto& r) { r.owner.conditions = {}; });
    rejects(recipe, [](auto& r) { r.owner.information = r.back; });
    rejects(recipe, [](auto& r) { r.back = r.callback.function; });
    rejects(recipe, [](auto& r) { r.station_profile = nullptr; });
    rejects(recipe, [](auto& r) { ++r.hand.right.amount; });
    rejects(recipe, [](auto& r) { r.hand.right.symbol = 16604; });
    rejects(recipe, [](auto& r) { r.hand.right.source = c::HandSource::empty; });
    rejects(recipe, [](auto& r) { r.hand.left.symbol = 0; });
    rejects(recipe, [](auto& r) { r.hand.anchor = static_cast<c::HandSide>(99); });
    rejects(recipe, [](auto& r) {
      r.hand.right.require_stored_absent = !r.hand.right.require_stored_absent;
    });
    auto unknown_hand = recipe;
    unknown_hand.hand.static_lifecycle_verified = false;
    unknown_hand.hand.observed_in_world = true;
    CHECK(c::structurally_valid(
        unknown_hand)); // The material core must still require current hand authorization.
    rejects(recipe, [](auto& r) { r.callback.function = nullptr; });
    rejects(recipe, [](auto& r) { r.callback.owner_instance = 20337; });
    rejects(recipe, [](auto& r) { r.callback.invocation = c::Invocation::info; });
    rejects(recipe, [](auto& r) { r.callback.original_quantity = 5; });
    rejects(recipe, [](auto& r) { r.callback.minimum_quantity = 0; });
    rejects(recipe, [](auto& r) { r.callback.maximum_quantity = 0; });
    rejects(recipe, [](auto& r) { r.callback.body_checks_resources = false; });
    rejects(recipe, [](auto& r) { r.callback.ends_production = !r.callback.ends_production; });
    rejects(recipe, [](auto& r) { r.callback.clears_choices = !r.callback.clears_choices; });
    rejects(recipe, [](auto& r) { r.callback.quantity_text_reviewed = true; });
    rejects(recipe, [](auto& r) { r.callback.failure_effects.count = 1; });
    rejects(recipe, [](auto& r) { ++r.callback.amount.length; });
    rejects(recipe, [](auto& r) { r.callback.amount.digest[0] ^= 1; });
    rejects(recipe, [](auto& r) { r.callback.amount.operands = nullptr; });
    rejects(recipe, [](auto& r) { --r.callback.amount.count; });
    rejects(recipe, [](auto& r) { r.callback.effects.data = nullptr; });
    rejects(recipe, [](auto& r) { r.operand_uses.data = nullptr; });

    std::vector<ubc::PackedInventoryTarget> targets(recipe.targets.begin(), recipe.targets.end());
    rejects(recipe, [&](auto& r) {
      targets[0] = targets[1];
      r.targets = view(targets);
    });
    targets.assign(recipe.targets.begin(), recipe.targets.end());
    rejects(recipe, [&](auto& r) {
      targets[0].name = "WRONG";
      r.targets = view(targets);
    });
    std::vector<c::SymbolBinding> bindings(recipe.bindings.begin(), recipe.bindings.end());
    for (std::size_t i = 0; i < bindings.size(); ++i) {
      bindings.assign(recipe.bindings.begin(), recipe.bindings.end());
      rejects(recipe, [&](auto& r) {
        ++bindings[i].elements;
        r.bindings = view(bindings);
      });
    }
    bindings.assign(recipe.bindings.begin(), recipe.bindings.end());
    rejects(recipe, [&](auto& r) {
      bindings[0] = bindings[1];
      r.bindings = view(bindings);
    });
    bindings.assign(recipe.bindings.begin(), recipe.bindings.end());
    rejects(recipe, [&](auto& r) {
      bindings[0].name = nullptr;
      r.bindings = view(bindings);
    });
    std::vector<c::ItemAmount> costs(recipe.costs.begin(), recipe.costs.end());
    rejects(recipe, [&](auto& r) {
      costs[0].symbol = r.hand.right.symbol;
      r.costs = view(costs);
    });
    costs.assign(recipe.costs.begin(), recipe.costs.end());
    rejects(recipe, [&](auto& r) {
      costs[0].units = 0;
      r.costs = view(costs);
    });
    std::vector<c::ItemAmount> outputs(recipe.outputs.begin(), recipe.outputs.end());
    rejects(recipe, [&](auto& r) {
      outputs[0].symbol = r.costs[0].symbol;
      r.outputs = view(outputs);
    });
    outputs.assign(recipe.outputs.begin(), recipe.outputs.end());
    rejects(recipe, [&](auto& r) {
      outputs[0].symbol = r.hand.right.symbol;
      r.outputs = view(outputs);
    });
    std::vector<c::Requirement> requirements(recipe.requirements.begin(),
                                             recipe.requirements.end());
    for (std::size_t i = 0; i < requirements.size(); ++i) {
      requirements.assign(recipe.requirements.begin(), recipe.requirements.end());
      rejects(recipe, [&](auto& r) {
        ++requirements[i].value;
        r.requirements = view(requirements);
      });
    }
    std::vector<c::RequirementCheck> sites(recipe.checks.begin(), recipe.checks.end());
    for (std::size_t i = 0; i < sites.size(); ++i) {
      sites.assign(recipe.checks.begin(), recipe.checks.end());
      rejects(recipe, [&](auto& r) {
        sites[i].site = static_cast<c::CheckSite>(99);
        r.checks = view(sites);
      });
    }
    auto profile = *recipe.station_profile;
    rejects(recipe, [&](auto& r) {
      profile.use_with_item = "ITMI_FLASK";
      r.station_profile = &profile;
    });
    profile = *recipe.station_profile;
    rejects(recipe, [&](auto& r) {
      profile.mob_condition = r.callback.function;
      r.station_profile = &profile;
    });
    profile = *recipe.station_profile;
    rejects(recipe, [&](auto& r) {
      profile.end.number = 2;
      r.station_profile = &profile;
    });
    auto function = *recipe.callback.function;
    rejects(recipe, [&](auto& r) {
      function.argc = 1;
      r.callback.function = &function;
    });
    function = *recipe.callback.function;
    rejects(recipe, [&](auto& r) {
      function.body_sha256[31] ^= 0xff;
      r.callback.function = &function;
    });
    function = *recipe.callback.function;
    rejects(recipe, [&](auto& r) {
      function.byte_offset = UINT32_MAX;
      r.callback.function = &function;
    });
    std::vector<ubc::AmountOperand> operands(recipe.callback.amount.operands,
                                             recipe.callback.amount.operands +
                                                 recipe.callback.amount.count);
    for (std::size_t i = 0; i < operands.size(); ++i) {
      operands.assign(recipe.callback.amount.operands,
                      recipe.callback.amount.operands + recipe.callback.amount.count);
      rejects(recipe, [&](auto& r) {
        ++operands[i].offset;
        r.callback.amount.operands = operands.data();
      });
      operands.assign(recipe.callback.amount.operands,
                      recipe.callback.amount.operands + recipe.callback.amount.count);
      rejects(recipe, [&](auto& r) {
        operands[i].reserved = 1;
        r.callback.amount.operands = operands.data();
      });
      operands.assign(recipe.callback.amount.operands,
                      recipe.callback.amount.operands + recipe.callback.amount.count);
      rejects(recipe, [&](auto& r) {
        operands[i].allow_zero = true;
        r.callback.amount.operands = operands.data();
      });
    }
    std::vector<c::OperandUse> uses(recipe.operand_uses.begin(), recipe.operand_uses.end());
    rejects(recipe, [&](auto& r) {
      uses[0].symbol = r.outputs[0].symbol;
      r.operand_uses = view(uses);
    });
    std::vector<c::MaterialEffect> effects(recipe.callback.effects.begin(),
                                           recipe.callback.effects.end());
    rejects(recipe, [&](auto& r) {
      std::reverse(effects.begin(), effects.end());
      r.callback.effects = view(effects);
    });

    // Ordering within unordered mapping sets has no semantic significance.
    auto copy = recipe;
    targets.assign(recipe.targets.begin(), recipe.targets.end());
    std::reverse(targets.begin(), targets.end());
    copy.targets = view(targets);
    bindings.assign(recipe.bindings.begin(), recipe.bindings.end());
    std::reverse(bindings.begin(), bindings.end());
    copy.bindings = view(bindings);
    requirements.assign(recipe.requirements.begin(), recipe.requirements.end());
    std::reverse(requirements.begin(), requirements.end());
    copy.requirements = view(requirements);
    sites.assign(recipe.checks.begin(), recipe.checks.end());
    std::reverse(sites.begin(), sites.end());
    copy.checks = view(sites);
    CHECK(c::structurally_valid(copy));
  }
}
} // namespace

int main() {
  recipe_facts();
  quantities_and_identity();
  malformed_descriptors();
  std::printf("station_recipe_catalog: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
