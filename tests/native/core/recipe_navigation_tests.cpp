// SPDX-License-Identifier: MIT
#include "ubc/core/recipe_navigation.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>

namespace {
namespace n = ubc::recipe_navigation;
namespace c = ubc::recipe_catalog;
using P = n::Page;
using K = n::Kind;
using I = c::Invocation;
unsigned checks = 0, failures = 0;
void check(bool value, const char* message) {
  ++checks;
  if (!value) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", message);
  }
}
struct Target {
  c::RecipeId id;
  P choice, category;
  std::int32_t owner, back, single, multiple;
};
constexpr std::array<Target, 6> targets{
    {{c::RecipeId::health_01, P::health_01_choices, P::health_info, 20337, 20340, 20341, 20343},
     {c::RecipeId::health_02, P::health_02_choices, P::health_info, 20344, 20347, 20348, 20350},
     {c::RecipeId::health_03, P::health_03_choices, P::health_info, 20351, 20354, 20355, 20357},
     {c::RecipeId::mana_01, P::mana_01_choices, P::mana_info, 20306, 20309, 20310, 20312},
     {c::RecipeId::mana_02, P::mana_02_choices, P::mana_info, 20313, 20316, 20317, 20319},
     {c::RecipeId::mana_03, P::mana_03_choices, P::mana_info, 20320, 20323, 20324, 20326}}};
constexpr std::array<P, 12> pages{{P::root, P::health_info, P::mana_info, P::health_01_choices,
                                   P::health_02_choices, P::health_03_choices, P::mana_01_choices,
                                   P::mana_02_choices, P::mana_03_choices, P::special_info,
                                   P::booze_info, P::tobacco_info}};
n::PageState page_state(P page) {
  n::PageState value;
  value.known = true;
  value.production = 4;
  value.health = value.mana = value.booze = value.tobacco = value.special = 0;
  if (page == P::health_info)
    value.health = 1;
  if (page == P::mana_info)
    value.mana = 1;
  if (page == P::special_info)
    value.special = 1;
  if (page == P::booze_info)
    value.booze = 1;
  if (page == P::tobacco_info)
    value.tobacco = 1;
  for (const auto& target : targets)
    if (page == target.choice) {
      value.invocation = I::choice;
      value.choice_owner = target.owner;
      if (target.category == P::health_info)
        value.health = 1;
      else
        value.mana = 1;
    }
  return value;
}
void simulate_navigation(const n::Plan& plan, const Target& target) {
  auto state = page_state(plan.source);
  unsigned effects = 0;
  for (std::size_t i = 0; i < plan.count; ++i) {
    const auto& step = plan.steps[i];
    check(n::classify_page(state) == step.before,
          "each step starts in its freshly classified original page");
    check(step.invocation == state.invocation, "Info and Choice are never interchangeable");
    const auto* owner = n::owner_for(step.owner_instance);
    const auto* function = n::function_for(step.function_symbol);
    check(owner && function, "every owner and callback has an explicit metadata identity");
    if (!owner || !function)
      continue;
    check(function->type == 5 && function->flags == 1 && !function->argc && !function->return_type,
          "planned invocation is always VOID without arguments");
    if (step.kind == K::effect) {
      ++effects;
      check(i + 1 == plan.count && step.is_effect() && step.after == P::unknown,
            "effect is last and claims no navigation-only result page");
      check(state.invocation == I::choice && state.choice_owner == target.owner &&
                step.owner_instance == target.owner &&
                step.function_symbol == (plan.quantity == 1 ? target.single : target.multiple),
            "quantity binds exactly original one or original five in the target owner");
      continue;
    }
    check(!step.is_effect(), "pure navigation is marked separately from production");
    if (step.kind == K::back) {
      bool exact = false;
      for (const auto& from : targets)
        if (from.owner == state.choice_owner && from.back == step.function_symbol)
          exact = true;
      check(exact && step.owner_instance == state.choice_owner,
            "BACK only clears the current exact owner");
      state.invocation = I::info;
      state.choice_owner = -1;
    } else if (step.kind == K::stop) {
      const auto owner = state.health    ? 20334
                         : state.mana    ? 20303
                         : state.special ? 20365
                         : state.booze   ? 20284
                         : state.tobacco ? 20390
                                         : -1;
      const auto function = state.health    ? 20336
                            : state.mana    ? 20305
                            : state.special ? 20367
                            : state.booze   ? 20286
                            : state.tobacco ? 20392
                                            : -1;
      check(step.owner_instance == owner && step.function_symbol == function,
            "STOP uses the original owner of the currently active category");
      if (state.health)
        state.health = 0;
      else if (state.mana)
        state.mana = 0;
      else if (state.special)
        state.special = 0;
      else if (state.booze)
        state.booze = 0;
      else if (state.tobacco)
        state.tobacco = 0;
    } else if (step.kind == K::start) {
      check(!state.health && !state.mana && !state.booze && !state.tobacco && !state.special,
            "START is reached only after all five category flags are zero");
      const bool health = target.category == P::health_info;
      check(step.owner_instance == (health ? 20331 : 20300) &&
                step.function_symbol == (health ? 20333 : 20302),
            "START binds the original target category info");
      if (health)
        state.health = 1;
      else
        state.mana = 1;
    } else {
      check(step.kind == K::open_owner && step.owner_instance == target.owner &&
                step.function_symbol == static_cast<std::int32_t>(owner->information->symbol),
            "recipe owner opening uses its original information function");
      state.invocation = I::choice;
      state.choice_owner = target.owner;
    }
    check(n::classify_page(state) == step.after,
          "original navigation effect yields precisely the next declared page");
  }
  check(effects == 1, "every plan contains exactly one separate production step");
}
}

int main() {
  check(!n::runtime_ready, "static navigation never grants runtime authorization");
  check(n::structurally_valid(),
        "all category, owner, BACK and flag metadata are structurally valid");
  check(n::functions.size() == 27 && n::flag_bindings.size() == 5 && n::routes.size() == 6 &&
            n::category_owners.size() == 7 && n::source_categories.size() == 3 &&
            n::categories.size() == 2,
        "metadata adds only the three original exit infos and keeps six potion targets");
  for (const auto& binding : n::flag_bindings)
    check(binding.type == 2 && !binding.flags && binding.elements == 1 && !binding.check_value,
          "category flag bindings validate identity, never a DAT initial value");
  for (const auto page : pages) {
    check(n::supported_page(page) && n::classify_page(page_state(page)) == page,
          "all twelve supported original pages classify exactly");
    for (const auto& target : targets)
      for (const auto amount : {1u, 2u, 3u, 5u, 7u, 23u}) {
        const auto plan = n::make_plan(page, target.id, amount);
        check(plan.valid() && n::structurally_valid(plan),
              "all source/target/quantity combinations have a bounded valid plan");
        check(plan.count <= 5 && plan.count > 0 && plan.effect(),
              "five steps include the separate final effect");
        std::size_t expected = 0;
        if (page == P::root)
          expected = 3;
        else if (page == target.choice)
          expected = 1;
        else if (page == target.category)
          expected = 2;
        else if (page == P::health_info || page == P::mana_info || page == P::special_info ||
                 page == P::booze_info || page == P::tobacco_info)
          expected = 4;
        else {
          bool same_category = false;
          for (const auto& from : targets)
            if (from.choice == page)
              same_category = from.category == target.category;
          expected = same_category ? 3 : 5;
        }
        check(plan.count == expected, "route uses only the necessary original navigation steps");
        simulate_navigation(plan, target);
      }
  }
  for (const auto source : {P::special_info, P::booze_info, P::tobacco_info}) {
    const auto stop_owner = source == P::special_info ? 20365
                            : source == P::booze_info ? 20284
                                                      : 20390;
    const auto stop_function = source == P::special_info ? 20367
                               : source == P::booze_info ? 20286
                                                         : 20392;
    check(!n::category_for(source) && !n::route_for(source) && n::source_category_for(source),
          "additional categories provide exits, never a new recipe destination or choice");
    for (const auto& target : targets)
      for (const auto amount : {1u, 7u}) {
        const auto plan = n::make_plan(source, target.id, amount);
        check(
            plan.count == 4 && plan.steps[0].kind == K::stop &&
                plan.steps[0].invocation == I::info && plan.steps[0].owner_instance == stop_owner &&
                plan.steps[0].function_symbol == stop_function && plan.steps[0].after == P::root &&
                plan.steps[1].kind == K::start && plan.steps[2].kind == K::open_owner &&
                plan.steps[3].kind == K::effect,
            "source-only category takes exact STOP then original target navigation and one effect");
      }
    for (const auto owner : {20399, 20337, 20306, -1}) {
      auto state = page_state(source);
      state.invocation = I::choice;
      state.choice_owner = owner;
      check(n::classify_page(state) == P::unknown,
            "unknown choices, including already charged tobacco, never become a pure exit page");
      state.invocation = I::info;
      check(n::classify_page(state) == source,
            "positively known info mode ignores stale choice owner");
    }
  }
  for (unsigned flags = 0; flags < 32; ++flags) {
    auto state = page_state(P::root);
    state.health = (flags >> 0) & 1u;
    state.mana = (flags >> 1) & 1u;
    state.booze = (flags >> 2) & 1u;
    state.tobacco = (flags >> 3) & 1u;
    state.special = (flags >> 4) & 1u;
    const bool exclusive = flags == 0 || (flags & (flags - 1)) == 0;
    check((n::classify_page(state) != P::unknown) == exclusive,
          "all flag combinations reject simultaneous active categories");
  }
  {
    const auto plan = n::make_plan(P::health_02_choices, c::RecipeId::mana_03, 2);
    const std::array<K, 5> kinds{{K::back, K::stop, K::start, K::open_owner, K::effect}};
    const std::array<std::int32_t, 5> symbols{{20347, 20336, 20302, 20322, 20326}};
    const std::array<std::int32_t, 5> owners{{20344, 20334, 20300, 20320, 20320}};
    check(plan.count == 5, "cross-category choice requires the maximum five-step route");
    for (std::size_t i = 0; i < 5; ++i)
      check(plan.steps[i].kind == kinds[i] && plan.steps[i].function_symbol == symbols[i] &&
                plan.steps[i].owner_instance == owners[i],
            "cross-category symbols are verified facts, not inferred row offsets");
  }
  for (const auto page : {P::unknown, static_cast<P>(-1), static_cast<P>(100)}) {
    const auto plan = n::make_plan(page, c::RecipeId::health_01, 1);
    check(!n::supported_page(page) && !plan.valid() && plan.count == 0 &&
              plan.issue == n::Issue::unknown_page && !plan.effect(),
          "unknown page never falls back to an original recipe");
  }
  for (const auto id : {c::RecipeId::meat, c::RecipeId::molerat_dish, c::RecipeId::mushroom_dish,
                        static_cast<c::RecipeId>(-1)}) {
    const auto plan = n::make_plan(P::root, id, 1);
    check(!plan.valid() && plan.issue == n::Issue::unsupported_recipe && !plan.count,
          "stove and unknown recipes do not acquire an alchemy route");
  }
  for (const auto& target : targets) {
    for (const auto amount : {0u, static_cast<std::uint32_t>(INT32_MAX) + 1u,
                              std::numeric_limits<std::uint32_t>::max()}) {
      const auto plan = n::make_plan(P::root, target.id, amount);
      check(!plan.valid() && plan.issue == n::Issue::quantity && plan.count == 0,
            "invalid quantity never returns a partial navigation plan");
    }
    const auto* recipe = c::find(target.id);
    std::uint32_t max = INT32_MAX;
    for (const auto& input : recipe->costs)
      max = std::min(max, static_cast<std::uint32_t>(INT32_MAX) / input.units);
    for (const auto& output : recipe->outputs)
      max = std::min(max, static_cast<std::uint32_t>(INT32_MAX) / output.units);
    check(n::make_plan(P::root, target.id, max).valid(),
          "largest representable material quantity has a static route");
    if (max < INT32_MAX)
      check(!n::make_plan(P::root, target.id, max + 1).valid(),
            "material multiplier overflow is rejected by the descriptor contract");
  }
  for (unsigned variant = 0; variant < 15; ++variant) {
    auto state = page_state(P::health_01_choices);
    switch (variant) {
    case 0:
      state.known = false;
      break;
    case 1:
      state.production = -1;
      break;
    case 2:
      state.production = 14;
      break;
    case 3:
      state.health = -1;
      break;
    case 4:
      state.health = 2;
      break;
    case 5:
      state.mana = 1;
      break;
    case 6:
      state.booze = 1;
      break;
    case 7:
      state.tobacco = 1;
      break;
    case 8:
      state.special = 1;
      break;
    case 9:
      state.booze = -1;
      break;
    case 10:
      state.choice_owner = 20306;
      break;
    case 11:
      state.choice_owner = 20339;
      break;
    case 12:
      state.choice_owner = -1;
      break;
    case 13:
      state.invocation = static_cast<I>(88);
      break;
    case 14:
      state.health = 0;
      break;
    }
    check(n::classify_page(state) == P::unknown,
          "unknown, conflicting or foreign page evidence is never treated as supported");
  }
  {
    auto state = page_state(P::health_info);
    state.choice_owner = 20306;
    check(n::classify_page(state) == P::health_info,
          "info mode ignores a stale choice owner instead of following it");
    check(n::classify_page(n::PageState{}) == P::unknown, "default evidence is unknown, not root");
  }
  for (unsigned variant = 0; variant < 15; ++variant) {
    auto plan = n::make_plan(P::health_02_choices, c::RecipeId::mana_03, 2);
    switch (variant) {
    case 0:
      plan.count = 0;
      break;
    case 1:
      plan.count = 6;
      break;
    case 2:
      plan.issue = n::Issue::contract;
      break;
    case 3:
      plan.source = P::unknown;
      break;
    case 4:
      plan.target = c::RecipeId::health_01;
      break;
    case 5:
      plan.quantity = 1;
      break;
    case 6:
      plan.steps[0].kind = K::effect;
      break;
    case 7:
      plan.steps[0].invocation = I::info;
      break;
    case 8:
      plan.steps[0].owner_instance = 20337;
      break;
    case 9:
      plan.steps[0].function_symbol = 20340;
      break;
    case 10:
      plan.steps[0].before = P::mana_01_choices;
      break;
    case 11:
      plan.steps[0].after = P::root;
      break;
    case 12:
      std::swap(plan.steps[1], plan.steps[2]);
      break;
    case 13:
      plan.steps[4].function_symbol = 20325;
      break;
    case 14:
      plan.steps[4].after = P::mana_03_choices;
      break;
    }
    check(!n::structurally_valid(plan),
          "mutated or reordered handoff cannot change the static bound route");
  }
  for (unsigned variant = 0; variant < 16; ++variant) {
    auto category = n::categories[0];
    auto start = *category.start;
    auto stop = *category.stop;
    category.start = &start;
    category.stop = &stop;
    auto function = *start.information;
    start.information = &function;
    auto requirements = n::category_start_conditions;
    start.conditions = c::view(requirements);
    switch (variant) {
    case 0:
      category.page = P::root;
      break;
    case 1:
      category.on_start.symbol = 20299;
      break;
    case 2:
      category.on_start.value = 0;
      break;
    case 3:
      category.on_stop.value = 1;
      break;
    case 4:
      start.number = 6;
      break;
    case 5:
      start.instance = 20300;
      break;
    case 6:
      stop.number = 999;
      break;
    case 7:
      function.body_sha256[0] ^= 1;
      break;
    case 8:
      function.name = "OTHER";
      break;
    case 9:
      function.argc = 1;
      break;
    case 10:
      function.byte_offset = UINT32_MAX;
      break;
    case 11:
      requirements[1].value = 1;
      break;
    case 12:
      requirements[2].symbol = requirements[1].symbol;
      break;
    case 13:
      start.conditions.count = 5;
      break;
    case 14:
      start.conditions.data = nullptr;
      break;
    case 15:
      category.stop = nullptr;
      break;
    }
    check(!n::structurally_valid(category),
          "category contract rejects wrong links, hashes, flags and predicates");
  }
  for (const auto& original : n::source_categories) {
    check(n::structurally_valid(original),
          "each source-only exit matches its exact original metadata");
    for (unsigned variant = 0; variant < 11; ++variant) {
      auto category = original;
      auto stop = *category.stop;
      category.stop = &stop;
      auto info = *stop.information;
      stop.information = &info;
      std::array<c::Requirement, 2> conditions{{stop.conditions[0], stop.conditions[1]}};
      stop.conditions = c::view(conditions);
      switch (variant) {
      case 0:
        category.page = P::root;
        break;
      case 1:
        category.on_stop.symbol = 20330;
        break;
      case 2:
        category.on_stop.value = 1;
        break;
      case 3:
        stop.number = 998;
        break;
      case 4:
        info.body_sha256[0] ^= 1;
        break;
      case 5:
        info.byte_length = 11;
        break;
      case 6:
        conditions[1].value = 0;
        break;
      case 7:
        conditions[0].value = 14;
        break;
      case 8:
        stop.conditions.count = 1;
        break;
      case 9:
        stop.conditions.data = nullptr;
        break;
      case 10:
        category.stop = nullptr;
        break;
      }
      check(!n::structurally_valid(category),
            "source-only exit rejects altered flags, bindings and function identity");
    }
  }
  for (unsigned variant = 0; variant < 16; ++variant) {
    auto route = n::routes[0];
    auto owner = *route.owner;
    auto back = *route.back;
    route.owner = &owner;
    route.back = &back;
    std::array<c::Requirement, 3> requirements{};
    check(owner.conditions.count == requirements.size(),
          "potion owner has production, category and learning conditions");
    std::copy(owner.conditions.begin(), owner.conditions.end(), requirements.begin());
    owner.conditions = c::view(requirements);
    switch (variant) {
    case 0:
      route.recipe = c::RecipeId::meat;
      break;
    case 1:
      route.choices = P::mana_01_choices;
      break;
    case 2:
      route.category = P::mana_info;
      break;
    case 3:
      owner.number += 1;
      break;
    case 4:
      owner.npc = 1683;
      break;
    case 5:
      owner.permanent = false;
      break;
    case 6:
      back.body_sha256[0] ^= 1;
      break;
    case 7:
      back.type = 7;
      break;
    case 8:
      back.byte_length = 0;
      break;
    case 9:
      back.symbol = 20309;
      break;
    case 10:
      route.original_choice_symbols[0] = 20341;
      break;
    case 11:
      std::swap(route.original_choice_symbols[1], route.original_choice_symbols[2]);
      break;
    case 12:
      route.original_choice_symbols[3] = route.original_choice_symbols[2];
      break;
    case 13:
      route.back = nullptr;
      break;
    case 14:
      requirements[2].value = 0;
      break;
    case 15:
      requirements[1].symbol = 20330;
      break;
    }
    check(!n::structurally_valid(route),
          "recipe route rejects mismatched owner, BACK and original quantity choices");
  }
  {
    auto category = n::categories[0];
    auto start = *category.start;
    auto info = *start.information;
    start.information = &info;
    category.start = &start;
    check(n::structurally_valid(category),
          "identical copied own metadata does not depend on pointer identity");
    auto route = n::routes[0];
    auto back = *route.back;
    route.back = &back;
    check(n::structurally_valid(route),
          "identical copied BACK metadata retains the exact identity contract");
    check(!n::function_for(-1) && !n::function_for(21010) && !n::owner_for(-1),
          "metadata lookup does not silently select foreign or stove functions");
    check(!n::function_for(20401) && !n::function_for(20402) && !n::owner_for(20399) &&
              !n::owner_for(20281) && !n::owner_for(20362) && !n::owner_for(20387),
          "source-only exits authorize neither tobacco owner or BACK nor new category starts");
  }
  for (const auto issue : {n::Issue::none, n::Issue::unknown_page, n::Issue::unsupported_recipe,
                           n::Issue::quantity, n::Issue::contract})
    check(std::strcmp(n::issue_name(issue), "invalid_issue") != 0,
          "every defined planning issue has an ASCII log name");
  check(std::strcmp(n::issue_name(static_cast<n::Issue>(99)), "invalid_issue") == 0,
        "invalid issue value is explicit");
  std::printf("RecipeNavigation: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
