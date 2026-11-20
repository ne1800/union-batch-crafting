// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/alchemy_recipe_catalog.hpp"
#include "ubc/core/recipe_navigation.hpp"

// Own metadata of the inspected DAT, no script bytes/dialog texts. The 13
// targets are direct INFO effects: 5 Special, 3 Booze, 1 Mana, 1 Health,
// 1 Root, and 2 Tobacco. In particular, no paid tobacco choice is opened.
namespace ubc::alchemy_extra_navigation {
namespace base = recipe_navigation;
namespace catalog = alchemy_recipe_catalog;
using base::classify_page;
using base::Invocation;
using base::Issue;
using base::Kind;
using base::Page;
using base::PageState;
using base::Step;
using base::supported_page;
inline constexpr bool runtime_ready = false;
inline constexpr std::size_t maximum_steps = 4;

inline constexpr std::array<lhiver::Function, 9> start_functions{{
    {"PC_BOOZE_START", 20281, 723314, 67,
     lhiver::digest_literal("d9b8a918a4d285bc0436186598a705667610908c88400c83847d78bc2dd55ed1"), 0,
     7, 1, 0, 1649},
    {"PC_BOOZE_START_CONDITION", 20282, 723381, 83,
     lhiver::digest_literal("6ab22611d530eebd13ea56fd1e1e4a08ae3db74fe012e2eace6dfc7444554eed"), 0,
     5, 3, 2, -1},
    {"PC_BOOZE_START_INFO", 20283, 723464, 12,
     lhiver::digest_literal("ba17c14523a1bf79e62694359b1aca8be1f39c69ef1438e36616fe48b82ab7f6"), 0,
     5, 1, 0, -1},
    {"PC_SPECIAL_START", 20362, 731671, 67,
     lhiver::digest_literal("5e722d5efdad43661c1a568a9fc760613cb43492131b2520508ce35a6f3479a8"), 0,
     7, 1, 0, 1649},
    {"PC_SPECIAL_START_CONDITION", 20363, 731738, 83,
     lhiver::digest_literal("8e6ca9978c6d3ff2f416b7cb33197e6041f722d8ab9d7d4fbb0b942f9e8942ac"), 0,
     5, 3, 2, -1},
    {"PC_SPECIAL_START_INFO", 20364, 731821, 12,
     lhiver::digest_literal("191485b94b966f123012679f197acd60eb9538785e64f8e72191b9a221fc1c69"), 0,
     5, 1, 0, -1},
    {"PC_TABAK_START", 20387, 733955, 67,
     lhiver::digest_literal("83a0a9e7050df942b4d8ca8ec4fa834343c4265437e0ed7272ce36c962b95a66"), 0,
     7, 1, 0, 1649},
    {"PC_TABAK_START_CONDITION", 20388, 734022, 83,
     lhiver::digest_literal("de82cea17ef5651b53963c664aa6daa822a483c46743e2d77252244fed80d328"), 0,
     5, 3, 2, -1},
    {"PC_TABAK_START_INFO", 20389, 734105, 12,
     lhiver::digest_literal("b8f6409f571552df6bcca24064bffda0ee83b5aea41e8deca416e61a24eae99e"), 0,
     5, 1, 0, -1},
}};
inline constexpr std::array<recipe_catalog::Owner, 3> start_owners{{
    {20281, 10, 22243, false, false, true, &start_functions[0], &start_functions[1],
     &start_functions[2], catalog::view(base::category_start_conditions)},
    {20362, 9, 22243, false, false, true, &start_functions[3], &start_functions[4],
     &start_functions[5], catalog::view(base::category_start_conditions)},
    {20387, 11, 22243, false, false, true, &start_functions[6], &start_functions[7],
     &start_functions[8], catalog::view(base::category_start_conditions)},
}};
inline constexpr std::array<base::Category, 5> categories{{
    base::categories[0],
    base::categories[1],
    {Page::booze_info, &start_owners[0], &base::category_owners[4], {20280, 1}, {20280, 0}},
    {Page::special_info, &start_owners[1], &base::category_owners[5], {20361, 1}, {20361, 0}},
    {Page::tobacco_info, &start_owners[2], &base::category_owners[6], {20386, 1}, {20386, 0}},
}};
inline constexpr Page target_page(catalog::Category category) noexcept {
  switch (category) {
  case catalog::Category::root:
    return Page::root;
  case catalog::Category::health:
    return Page::health_info;
  case catalog::Category::mana:
    return Page::mana_info;
  case catalog::Category::booze:
    return Page::booze_info;
  case catalog::Category::special:
    return Page::special_info;
  case catalog::Category::tobacco:
    return Page::tobacco_info;
  }
  return Page::unknown;
}
inline const base::Category* category_for(Page page) noexcept {
  for (const auto& category : categories)
    if (category.page == page)
      return &category;
  return nullptr;
}
inline const recipe_catalog::Owner* owner_for(std::int32_t instance) noexcept {
  for (const auto& owner : start_owners)
    if (owner.instance == instance)
      return &owner;
  for (const auto& recipe : catalog::recipes)
    if (recipe.owner.instance == instance)
      return &recipe.owner;
  return base::owner_for(instance);
}
inline const lhiver::Function* function_for(std::uint32_t symbol) noexcept {
  for (const auto& f : start_functions)
    if (f.symbol == symbol)
      return &f;
  if (const auto* f = catalog::function_for(symbol))
    return f;
  return symbol <= INT32_MAX ? base::function_for(static_cast<std::int32_t>(symbol)) : nullptr;
}
inline bool structurally_valid(const base::Category& category) noexcept {
  const auto* canonical = alchemy_extra_navigation::category_for(category.page);
  if (!canonical || !base::detail::same_owner_binding(category.start, canonical->start) ||
      !base::detail::same_owner_binding(category.stop, canonical->stop) ||
      !base::detail::valid_owner(category.start) || !base::detail::valid_owner(category.stop) ||
      category.on_start.symbol != canonical->on_start.symbol || category.on_start.value != 1 ||
      category.on_stop.symbol != canonical->on_stop.symbol || category.on_stop.value != 0 ||
      category.start->information->byte_length != 12 ||
      category.stop->information->byte_length != 12)
    return false;
  return true;
}
inline bool structurally_valid() noexcept {
  if (!base::structurally_valid())
    return false;
  for (const auto& category : categories)
    if (!alchemy_extra_navigation::structurally_valid(category))
      return false;
  for (const auto& recipe : catalog::recipes) {
    if (!catalog::structurally_valid(recipe) || recipe.callback.invocation != Invocation::info ||
        recipe.owner.information->symbol != recipe.callback.function->symbol ||
        (target_page(recipe.category) != Page::root &&
         !alchemy_extra_navigation::category_for(target_page(recipe.category))))
      return false;
  }
  return true;
}
struct Plan {
  Issue issue = Issue::contract;
  Page source = Page::unknown;
  catalog::RecipeId target = catalog::RecipeId::lou_spirit;
  std::uint32_t quantity = 0;
  std::array<Step, maximum_steps> steps{};
  std::size_t count = 0;
  bool valid() const noexcept { return issue == Issue::none && count && count <= steps.size(); }
  const Step* effect() const noexcept {
    return valid() && steps[count - 1].is_effect() ? &steps[count - 1] : nullptr;
  }
};
// Only a canonical sequence; every new collection/owner binding is
// freshly resolved by the native handoff. No stored rows or engine calls.
inline Plan make_plan(Page source, catalog::RecipeId target, std::uint32_t quantity) noexcept {
  Plan out;
  out.source = source;
  out.target = target;
  out.quantity = quantity;
  if (!supported_page(source)) {
    out.issue = Issue::unknown_page;
    return out;
  }
  const auto* recipe = catalog::find(target);
  if (!recipe) {
    out.issue = Issue::unsupported_recipe;
    return out;
  }
  if (!catalog::plan_quantity(*recipe, quantity).valid) {
    out.issue = Issue::quantity;
    return out;
  }
  if (!structurally_valid())
    return out;
  Page current = source;
  const auto destination = target_page(recipe->category);
  auto add = [&](Kind kind, Invocation mode, std::int32_t owner, std::uint32_t function,
                 Page after) noexcept {
    if (out.count == out.steps.size())
      return false;
    out.steps[out.count++] = {kind,    mode, owner, static_cast<std::int32_t>(function),
                              current, after};
    current = after;
    return true;
  };
  if (const auto* choices = base::route_for(current))
    if (!add(Kind::back, Invocation::choice, choices->owner->instance, choices->back->symbol,
             choices->category))
      return Plan{};
  if (current != destination) {
    if (const auto* from = alchemy_extra_navigation::category_for(current))
      if (!add(Kind::stop, Invocation::info, from->stop->instance, from->stop->information->symbol,
               Page::root))
        return Plan{};
    if (destination != Page::root) {
      const auto* to = alchemy_extra_navigation::category_for(destination);
      if (current != Page::root || !to ||
          !add(Kind::start, Invocation::info, to->start->instance, to->start->information->symbol,
               destination))
        return Plan{};
    }
  }
  if (current != destination || !add(Kind::effect, Invocation::info, recipe->owner.instance,
                                     recipe->callback.function->symbol, Page::unknown))
    return Plan{};
  out.issue = Issue::none;
  return out;
}
inline bool structurally_valid(const Plan& plan) noexcept {
  if (!plan.valid())
    return false;
  const auto canonical = make_plan(plan.source, plan.target, plan.quantity);
  if (!canonical.valid() || canonical.count != plan.count)
    return false;
  for (std::size_t i = 0; i < plan.count; ++i) {
    const auto& a = plan.steps[i];
    const auto& b = canonical.steps[i];
    if (a.kind != b.kind || a.invocation != b.invocation || a.owner_instance != b.owner_instance ||
        a.function_symbol != b.function_symbol || a.before != b.before || a.after != b.after)
      return false;
  }
  return true;
}
} // namespace ubc::alchemy_extra_navigation
