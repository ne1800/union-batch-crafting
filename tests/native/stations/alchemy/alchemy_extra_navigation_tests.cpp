// SPDX-License-Identifier: MIT
#include "ubc/stations/alchemy/alchemy_extra_navigation.hpp"
#include <cstdio>
namespace n = ubc::alchemy_extra_navigation;
namespace c = ubc::alchemy_recipe_catalog;
namespace {
unsigned checks = 0, failures = 0;
void check(bool b, int line) {
  ++checks;
  if (!b) {
    ++failures;
    std::fprintf(stderr, "failure line %d\n", line);
  }
}
#define CHECK(v) check((v), __LINE__)
void graph() {
  CHECK(n::structurally_valid());
  unsigned routes = 0;
  std::size_t longest = 0;
  for (const auto source :
       {n::Page::root, n::Page::health_info, n::Page::mana_info, n::Page::booze_info,
        n::Page::special_info, n::Page::tobacco_info, n::Page::health_01_choices,
        n::Page::health_02_choices, n::Page::health_03_choices, n::Page::mana_01_choices,
        n::Page::mana_02_choices, n::Page::mana_03_choices})
    for (const auto& recipe : c::recipes)
      for (const auto amount : {1u, 2u, 7u}) {
        const auto p = n::make_plan(source, recipe.id, amount);
        ++routes;
        CHECK(p.valid() && n::structurally_valid(p));
        if (p.count > longest)
          longest = p.count;
        CHECK(p.effect() && p.effect()->invocation == n::Invocation::info &&
              p.effect()->owner_instance == recipe.owner.instance &&
              p.effect()->function_symbol ==
                  static_cast<std::int32_t>(recipe.callback.function->symbol));
        CHECK(p.effect()->before == n::target_page(recipe.category) &&
              p.effect()->after == n::Page::unknown);
        unsigned effects = 0;
        auto page = source;
        for (std::size_t i = 0; i < p.count; ++i) {
          const auto& step = p.steps[i];
          effects += step.is_effect() ? 1u : 0u;
          CHECK(step.before == page && n::owner_for(step.owner_instance) &&
                n::function_for(static_cast<std::uint32_t>(step.function_symbol)));
          CHECK(step.kind != n::Kind::open_owner);
          page = step.after;
        }
        CHECK(effects == 1 && page == n::Page::unknown);
        auto corrupt = p;
        corrupt.steps[p.count - 1].function_symbol++;
        CHECK(!n::structurally_valid(corrupt));
        corrupt = p;
        corrupt.steps[0].owner_instance++;
        CHECK(!n::structurally_valid(corrupt));
        corrupt = p;
        corrupt.steps[0].after = n::Page::unknown;
        CHECK(!n::structurally_valid(corrupt) || p.count == 1);
        if (source == n::target_page(recipe.category))
          CHECK(p.count == 1);
      }
  CHECK(routes == 468 && longest == 4);
  for (const auto& recipe : c::recipes) {
    CHECK(!n::make_plan(n::Page::unknown, recipe.id, 1).valid());
    CHECK(!n::make_plan(n::Page::root, recipe.id, 0).valid());
    CHECK(!n::make_plan(n::Page::root, recipe.id, UINT32_MAX).valid());
  }
  CHECK(!n::make_plan(n::Page::root, static_cast<c::RecipeId>(1), 1).valid());
  // Unrelated tobacco choices and those opened through payment remain unknown. INFO is pure.
  n::PageState state{true, 4, 0, 0, 0, 1, 0, n::Invocation::choice, 20400};
  CHECK(n::classify_page(state) == n::Page::unknown);
  state.invocation = n::Invocation::info;
  CHECK(n::classify_page(state) == n::Page::tobacco_info);
  state.special = 1;
  CHECK(n::classify_page(state) == n::Page::unknown);
  for (const auto& category : n::categories) {
    CHECK(n::structurally_valid(category));
    auto bad = category;
    bad.on_start.value = 0;
    CHECK(!n::structurally_valid(bad));
    bad = category;
    auto owner = *bad.start;
    owner.number++;
    bad.start = &owner;
    CHECK(!n::structurally_valid(bad));
  }
  CHECK(n::make_plan(n::Page::tobacco_info, c::RecipeId::swamp_roll, 2).count == 1);
  CHECK(n::make_plan(n::Page::tobacco_info, c::RecipeId::dragon_egg_potion, 2).count == 2);
}
}
int main() {
  graph();
  std::printf("%u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
