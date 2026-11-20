// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/anvil_recipe_catalog.hpp"

// Original categories/choices of the bounded five-profile. A plan allows
// no script execution: each step requires fresh owner, learning, context,
// material, hand, thread, and collection evidence in the native adapter.
namespace ubc::anvil_recipe_navigation {
namespace cat = anvil_recipe_catalog;
using cat::Invocation;
using cat::RecipeId;
enum class Page {
  unknown,
  root,
  common,
  ore,
  common_sword,
  harad_01,
  harad_02,
  harad_03,
  harad_04
};
enum class Kind { back, open_owner, effect };
enum class Issue { none, unknown_page, unsupported_recipe, quantity, contract };
inline constexpr bool runtime_ready = false;
inline constexpr std::size_t maximum_steps = 4;
inline constexpr std::array<Page, 8> pages{{Page::root, Page::common, Page::ore, Page::common_sword,
                                            Page::harad_01, Page::harad_02, Page::harad_03,
                                            Page::harad_04}};
inline Page page_for(RecipeId id) noexcept {
  for (std::size_t i = 0; i < cat::recipes.size(); ++i)
    if (cat::recipes[i].id == id)
      return pages[i + 3];
  return Page::unknown;
}
inline const cat::Descriptor* recipe_for(Page page) noexcept {
  for (const auto& r : cat::recipes)
    if (page_for(r.id) == page)
      return &r;
  return nullptr;
}
inline bool supported_page(Page page) noexcept {
  for (const auto p : pages)
    if (p == page)
      return true;
  return false;
}
inline bool structurally_valid() noexcept {
  for (const auto& r : cat::recipes)
    if (!cat::structurally_valid(r) || !r.back || r.back->byte_length != 11 ||
        r.callback.invocation != Invocation::choice ||
        r.callback.owner_instance != r.owner.instance)
      return false;
  const auto& open = cat::navigation[0];
  const auto& back = cat::navigation[3];
  return open.before == cat::Page::root && open.after == cat::Page::common && open.flag == 10552 &&
         open.before_value == 0 && open.after_value == 1 && open.owner.instance == 20701 &&
         open.owner.information->symbol == 20703 && back.before == cat::Page::ore &&
         back.after == cat::Page::root && back.flag == 10551 && back.before_value == 1 &&
         back.after_value == 0 && back.owner.instance == 20710 &&
         back.owner.information->symbol == 20712;
}
struct PageState {
  bool known = false;
  std::int32_t production = -1, start_smithing = -1, normal_weapons = -1, ore_weapons = -1;
  Invocation invocation = Invocation::info;
  std::int32_t choice_owner = -1;
};
inline Page classify_page(const PageState& value) noexcept {
  if (!value.known || value.production != 1 || value.start_smithing != 1)
    return Page::unknown;
  Page base = Page::unknown;
  if (value.normal_weapons == 0 && value.ore_weapons == 0)
    base = Page::root;
  if (value.normal_weapons == 1 && value.ore_weapons == 0)
    base = Page::common;
  if (value.normal_weapons == 0 && value.ore_weapons == 1)
    base = Page::ore;
  if (value.invocation == Invocation::info)
    return base;
  if (value.invocation != Invocation::choice || base != Page::common)
    return Page::unknown;
  for (const auto& r : cat::recipes)
    if (r.owner.instance == value.choice_owner)
      return page_for(r.id);
  // Ore weapon choices have their own back callbacks. Do NOT invent them here.
  return Page::unknown;
}
struct Step {
  Kind kind = Kind::back;
  Invocation invocation = Invocation::info;
  std::int32_t owner_instance = -1, function_symbol = -1;
  Page before = Page::unknown, after = Page::unknown;
  bool is_effect() const noexcept { return kind == Kind::effect; }
};
struct Plan {
  Issue issue = Issue::contract;
  Page source = Page::unknown;
  RecipeId target = static_cast<RecipeId>(0);
  std::uint32_t quantity = 0;
  std::array<Step, maximum_steps> steps{};
  std::size_t count = 0;
  bool valid() const noexcept { return issue == Issue::none && count && count <= maximum_steps; }
  const Step* effect() const noexcept {
    return valid() && steps[count - 1].is_effect() ? &steps[count - 1] : nullptr;
  }
};
inline Plan make_plan(Page source, RecipeId target, std::uint32_t quantity) noexcept {
  Plan out;
  out.source = source;
  out.target = target;
  out.quantity = quantity;
  if (!supported_page(source)) {
    out.issue = Issue::unknown_page;
    return out;
  }
  const auto* recipe = cat::find(target);
  if (!recipe) {
    out.issue = Issue::unsupported_recipe;
    return out;
  }
  const auto amount = cat::plan_quantity(*recipe, quantity);
  if (!amount.valid) {
    out.issue = Issue::quantity;
    return out;
  }
  if (!structurally_valid())
    return out;
  auto current = source;
  const auto add = [&](Kind kind, Invocation invocation, std::int32_t owner, std::uint32_t function,
                       Page after) noexcept {
    if (out.count == maximum_steps)
      return false;
    out.steps[out.count++] = {kind,    invocation, owner, static_cast<std::int32_t>(function),
                              current, after};
    current = after;
    return true;
  };
  if (const auto* from = recipe_for(current))
    if (from->id != target)
      if (!add(Kind::back, Invocation::choice, from->owner.instance, from->back->symbol,
               Page::common))
        return Plan{};
  if (current == Page::ore) {
    const auto& back = cat::navigation[3];
    if (!add(Kind::back, Invocation::info, back.owner.instance, back.owner.information->symbol,
             Page::root))
      return Plan{};
  }
  if (current == Page::root) {
    const auto& open = cat::navigation[0];
    if (!add(Kind::open_owner, Invocation::info, open.owner.instance,
             open.owner.information->symbol, Page::common))
      return Plan{};
  }
  if (current == Page::common)
    if (!add(Kind::open_owner, Invocation::info, recipe->owner.instance,
             recipe->owner.information->symbol, page_for(target)))
      return Plan{};
  if (current != page_for(target) || !add(Kind::effect, Invocation::choice, recipe->owner.instance,
                                          amount.callback->function->symbol, Page::unknown))
    return Plan{};
  out.issue = Issue::none;
  return out;
}
inline bool structurally_valid(const Plan& plan) noexcept {
  if (!plan.valid())
    return false;
  const auto expected = make_plan(plan.source, plan.target, plan.quantity);
  if (!expected.valid() || expected.count != plan.count)
    return false;
  for (std::size_t i = 0; i < plan.count; ++i) {
    const auto& a = plan.steps[i];
    const auto& b = expected.steps[i];
    if (a.kind != b.kind || a.invocation != b.invocation || a.owner_instance != b.owner_instance ||
        a.function_symbol != b.function_symbol || a.before != b.before || a.after != b.after)
      return false;
  }
  return true;
}
} // namespace ubc::anvil_recipe_navigation
