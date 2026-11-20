// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/bow/bow_recipe_runtime.hpp"

namespace ubc::bow_recipe_navigation {
namespace cat = bow_recipe_catalog;
enum class Page { unknown, root, upgrades };
enum class Kind { open_category, effect };
enum class Issue { none, unknown_page, unsupported_recipe, quantity, contract };
struct PageState {
  bool known = false;
  std::int32_t production = -1, bows = -1;
  cat::Invocation invocation = cat::Invocation::info;
};
inline Page classify_page(const PageState& s) noexcept {
  if (!s.known || s.production != 16 || s.invocation != cat::Invocation::info)
    return Page::unknown;
  return s.bows == 0 ? Page::root : s.bows == 1 ? Page::upgrades : Page::unknown;
}
struct Step {
  Kind kind = Kind::effect;
  cat::Invocation invocation = cat::Invocation::info;
  std::int32_t owner_instance = -1;
  std::uint32_t function_symbol = 0;
  Page before = Page::unknown, after = Page::unknown;
};
struct Plan {
  Issue issue = Issue::contract;
  Page source = Page::unknown;
  cat::RecipeId target = static_cast<cat::RecipeId>(0);
  std::uint32_t quantity = 0;
  std::array<Step, 2> steps{};
  std::size_t count = 0;
  bool valid() const noexcept { return issue == Issue::none && count && count <= steps.size(); }
  const Step* effect() const noexcept {
    return valid() && steps[count - 1].kind == Kind::effect ? &steps[count - 1] : nullptr;
  }
};
// No paid intermediate path and no looping: category opening checks
// the original knowledge itself, then exactly one original direct
// INFO. Fresh idle collection, owner, knowledge and stock/hand binding
// remain required per level. M_BOWS=1 alone is not a knowledge proof.
inline Plan make_plan(Page source, cat::RecipeId target, std::uint32_t quantity) noexcept {
  Plan out;
  out.source = source;
  out.target = target;
  out.quantity = quantity;
  if (source != Page::root && source != Page::upgrades) {
    out.issue = Issue::unknown_page;
    return out;
  }
  const auto* r = cat::find(target);
  if (!r) {
    out.issue = Issue::unsupported_recipe;
    return out;
  }
  if (!cat::plan_quantity(*r, quantity).valid) {
    out.issue = Issue::quantity;
    return out;
  }
  if (source == Page::root)
    out.steps[out.count++] = {Kind::open_category,
                              cat::Invocation::info,
                              cat::station.category.instance,
                              cat::station.category.information->symbol,
                              Page::root,
                              Page::upgrades};
  out.steps[out.count++] = {Kind::effect,      cat::Invocation::info,
                            r->owner.instance, r->callback.function->symbol,
                            Page::upgrades,    Page::unknown};
  out.issue = Issue::none;
  return out;
}
inline bool structurally_valid(const Plan& p) noexcept {
  if (!p.valid())
    return false;
  const auto expected = make_plan(p.source, p.target, p.quantity);
  if (!expected.valid() || expected.count != p.count)
    return false;
  for (std::size_t i = 0; i < p.count; ++i) {
    const auto& a = p.steps[i];
    const auto& b = expected.steps[i];
    if (a.kind != b.kind || a.invocation != b.invocation || a.owner_instance != b.owner_instance ||
        a.function_symbol != b.function_symbol || a.before != b.before || a.after != b.after)
      return false;
  }
  return true;
}
inline bow_recipe_runtime::ProfileAssembly step_profile(const Plan& p, std::size_t index) noexcept {
  return structurally_valid(p) && index < p.count
             ? bow_recipe_runtime::assemble(*cat::find(p.target))
             : bow_recipe_runtime::ProfileAssembly{};
}
inline runtime_script::RuntimeContract verify_step(const ReadOnlyMemory& memory,
                                                   std::uint32_t parser, const Plan& p,
                                                   std::size_t index) noexcept {
  const auto profile = step_profile(p, index);
  return bow_recipe_runtime::verify(memory, parser, profile);
}
} // namespace ubc::bow_recipe_navigation
