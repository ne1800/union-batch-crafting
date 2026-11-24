// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/core/recipe_navigation.hpp"
#include "ubc/stations/station_recipe_navigation.hpp"
#include "ubc/stations/alchemy/alchemy_extra_navigation.hpp"
#include "ubc/stations/anvil/anvil_recipe_navigation.hpp"
#include "ubc/stations/ore/ore_recipe_navigation.hpp"
#include "ubc/stations/spirit/spirit_recipe_navigation.hpp"
#include "ubc/stations/bow/bow_recipe_navigation.hpp"
#include "ubc/stations/cool/cool_recipe_navigation.hpp"
#include "ubc/stations/grind/grind_recipe_navigation.hpp"
#include <optional>

namespace ubc {

enum class CraftingPageKind {
  unknown,
  stove_info,
  alchemy,
  station,
  anvil,
  bow,
  cool,
  grind,
  tobacco_choices,
  dndr_alchemy,
  dndr_anvil
};
struct CraftingPage {
  CraftingPageKind kind = CraftingPageKind::unknown;
  std::uint32_t value = 0;
  bool operator==(const CraftingPage& other) const noexcept {
    return kind == other.kind && value == other.value;
  }
  bool operator!=(const CraftingPage& other) const noexcept { return !(*this == other); }
};
inline CraftingPage crafting_stove_page() noexcept { return {CraftingPageKind::stove_info, 0}; }
inline CraftingPage crafting_page(recipe_navigation::Page page) noexcept {
  return recipe_navigation::supported_page(page)
             ? CraftingPage{CraftingPageKind::alchemy, static_cast<std::uint32_t>(page)}
             : CraftingPage{};
}
inline CraftingPage crafting_page(station_recipe_navigation::Page page) noexcept {
  return station_recipe_navigation::supported_page(page)
             ? CraftingPage{CraftingPageKind::station, static_cast<std::uint32_t>(page)}
             : CraftingPage{};
}
inline CraftingPage crafting_page(anvil_recipe_navigation::Page page) noexcept {
  return anvil_recipe_navigation::supported_page(page)
             ? CraftingPage{CraftingPageKind::anvil, static_cast<std::uint32_t>(page)}
             : CraftingPage{};
}
inline CraftingPage crafting_page(bow_recipe_navigation::Page page) noexcept {
  return page == bow_recipe_navigation::Page::root || page == bow_recipe_navigation::Page::upgrades
             ? CraftingPage{CraftingPageKind::bow, static_cast<std::uint32_t>(page)}
             : CraftingPage{};
}
inline CraftingPage crafting_page(cool_recipe_navigation::Page page) noexcept {
  return page.valid && (!page.triple || page.single)
             ? CraftingPage{CraftingPageKind::cool,
                            (page.single ? 1u : 0u) | (page.triple ? 2u : 0u)}
             : CraftingPage{};
}
inline CraftingPage crafting_page(const grind_recipe_navigation::PageEvidence& page) noexcept {
  if (!grind_recipe_navigation::valid_page(page))
    return {};
  return {CraftingPageKind::grind, static_cast<std::uint32_t>(page.page) |
                                       (page.owner_present ? 0x100u : 0u) |
                                       (page.category == 20961   ? 0x200u
                                        : page.category == 20962 ? 0x400u
                                                                 : 0u)};
}
inline grind_recipe_navigation::PageEvidence crafting_grind_page(CraftingPage page) noexcept {
  using Page = grind_recipe_navigation::Page;
  if (page.kind != CraftingPageKind::grind || (page.value & ~0x707u) ||
      (page.value & 0x600u) == 0x600u)
    return {};
  const grind_recipe_navigation::PageEvidence out{true, (page.value & 0x100u) != 0,
                                                  static_cast<Page>(page.value & 7u),
                                                  (page.value & 0x200u)   ? 20961u
                                                  : (page.value & 0x400u) ? 20962u
                                                                          : 0u};
  return grind_recipe_navigation::valid_page(out) ? out : grind_recipe_navigation::PageEvidence{};
}
inline bool valid_crafting_page(CraftingPage page) noexcept {
  if (page.kind == CraftingPageKind::tobacco_choices)
    return page.value == 20399;
  if (page.kind == CraftingPageKind::stove_info)
    return !page.value;
  if (page.kind == CraftingPageKind::alchemy)
    return recipe_navigation::supported_page(static_cast<recipe_navigation::Page>(page.value));
  if (page.kind == CraftingPageKind::station)
    return station_recipe_navigation::supported_page(
        static_cast<station_recipe_navigation::Page>(page.value));
  if (page.kind == CraftingPageKind::anvil)
    return anvil_recipe_navigation::supported_page(
        static_cast<anvil_recipe_navigation::Page>(page.value));
  if (page.kind == CraftingPageKind::dndr_anvil)
    return dndr::supported_category(page.value, true);
  if (page.kind == CraftingPageKind::dndr_alchemy)
    return dndr::supported_category(page.value);
  if (page.kind == CraftingPageKind::bow)
    return crafting_page(static_cast<bow_recipe_navigation::Page>(page.value)) == page;
  if (page.kind == CraftingPageKind::cool)
    return page.value == 0 || page.value == 1 || page.value == 3;
  if (page.kind == CraftingPageKind::grind)
    return crafting_grind_page(page).known;
  return false;
}
enum class CraftingStepKind { back, stop, start, open_owner, effect };
enum class CraftingPlanIssue {
  none,
  unknown_page,
  unsupported_recipe,
  station,
  quantity,
  contract
};
struct CraftingStep {
  CraftingStepKind kind = CraftingStepKind::back;
  recipe_catalog::Invocation invocation = recipe_catalog::Invocation::info;
  std::int32_t owner_instance = -1, function_symbol = -1;
  CraftingPage before{}, after{};
  bool is_effect() const noexcept { return kind == CraftingStepKind::effect; }
};
struct CraftingPlan {
  static constexpr std::size_t maximum_steps = recipe_navigation::maximum_steps;
  CraftingPlanIssue issue = CraftingPlanIssue::contract;
  CraftingPage source{};
  crafting_catalog::RecipeId target = UINT32_MAX;
  std::uint32_t quantity = 0;
  std::array<CraftingStep, maximum_steps> steps{};
  std::size_t count = 0;
  bool branch_known = false;
  std::int32_t branch_value = 0;
  // During cooling and grinding, the original selection depends on the material.
  CraftingState material_snapshot{};
  bool valid() const noexcept {
    return issue == CraftingPlanIssue::none && count && count <= steps.size();
  }
  const CraftingStep* effect() const noexcept {
    return valid() && steps[count - 1].is_effect() ? &steps[count - 1] : nullptr;
  }
};

// Only normalized own metadata. The existing graphs remain unchanged;
// neither new native calls nor learning grants are created.
inline CraftingPlan make_crafting_plan(CraftingPage source, crafting_catalog::RecipeId target,
                                       std::uint32_t quantity, bool branch_known = false,
                                       std::int32_t branch_value = 0,
                                       const CraftingState* material = nullptr) noexcept {
  namespace base = recipe_navigation;
  namespace station = station_recipe_navigation;
  CraftingPlan out;
  out.source = source;
  out.target = target;
  out.quantity = quantity;
  out.branch_known = branch_known;
  out.branch_value = branch_value;
  if (!valid_crafting_page(source)) {
    out.issue = CraftingPlanIssue::unknown_page;
    return out;
  }
  const auto* recipe = crafting_catalog::find(target);
  if (!recipe) {
    out.issue = CraftingPlanIssue::unsupported_recipe;
    return out;
  }
  if (!crafting_catalog::has_output_branch(*recipe) && (branch_known || branch_value))
    return out;
  if (recipe->route == crafting_catalog::Route::dndr) {
    const auto* r = recipe->dndr_recipe;
    if (!r || r->id != target || !quantity || quantity > INT32_MAX ||
        (r->branched() && !branch_known))
      return out;
    const auto kind = r->at_anvil() ? CraftingPageKind::dndr_anvil : CraftingPageKind::dndr_alchemy;
    if (source.kind != kind)
      return out;
    const CraftingPage root{kind, 0};
    const CraftingPage desired{kind, r->category_flag};
    auto current = source;
    auto step = [&](CraftingStepKind kind, const recipe_catalog::Owner& owner, CraftingPage next) {
      out.steps[out.count++] = {kind,           recipe_catalog::Invocation::info,
                                owner.instance, static_cast<int>(owner.information->symbol),
                                current,        next};
      current = next;
    };
    if (!(current == desired)) {
      if (const auto* c = dndr::category(current.value, r->at_anvil()))
        step(CraftingStepKind::stop, *c->stop, root);
      if (const auto* c = dndr::category(desired.value, r->at_anvil()))
        step(CraftingStepKind::start, *c->start, desired);
    }
    step(CraftingStepKind::effect, *r->effect, {});
  } else if (recipe->route == crafting_catalog::Route::base) {
    const bool stove = recipe->station == crafting_catalog::Station::stove;
    if (source.kind != (stove ? CraftingPageKind::stove_info : CraftingPageKind::alchemy)) {
      out.issue = CraftingPlanIssue::station;
      return out;
    }
    if (!recipe_catalog::structurally_valid(*recipe->base))
      return out;
    const auto* callback = recipe_catalog::callback_for_quantity(*recipe->base, quantity);
    if (!callback) {
      out.issue = CraftingPlanIssue::quantity;
      return out;
    }
    if (stove) {
      if (callback->invocation != recipe_catalog::Invocation::info)
        return out;
      out.steps[0] = {CraftingStepKind::effect,
                      callback->invocation,
                      callback->owner_instance,
                      static_cast<std::int32_t>(callback->function->symbol),
                      source,
                      {}};
      out.count = 1;
    } else {
      const auto plan =
          base::make_plan(static_cast<base::Page>(source.value), recipe->base->id, quantity);
      if (!base::structurally_valid(plan))
        return out;
      out.count = plan.count;
      for (std::size_t i = 0; i < plan.count; ++i) {
        const auto& step = plan.steps[i];
        CraftingStepKind kind = CraftingStepKind::back;
        switch (step.kind) {
        case base::Kind::back:
          kind = CraftingStepKind::back;
          break;
        case base::Kind::stop:
          kind = CraftingStepKind::stop;
          break;
        case base::Kind::start:
          kind = CraftingStepKind::start;
          break;
        case base::Kind::open_owner:
          kind = CraftingStepKind::open_owner;
          break;
        case base::Kind::effect:
          kind = CraftingStepKind::effect;
          break;
        }
        out.steps[i] = {kind,
                        step.invocation,
                        step.owner_instance,
                        step.function_symbol,
                        crafting_page(step.before),
                        crafting_page(step.after)};
      }
    }
  } else if (recipe->route == crafting_catalog::Route::station) {
    if (source.kind != CraftingPageKind::station) {
      out.issue = CraftingPlanIssue::station;
      return out;
    }
    const auto plan = station::make_plan(static_cast<station::Page>(source.value),
                                         recipe->station_descriptor->id, quantity);
    if (!station::structurally_valid(plan)) {
      if (plan.issue == station::Issue::station)
        out.issue = CraftingPlanIssue::station;
      if (plan.issue == station::Issue::quantity)
        out.issue = CraftingPlanIssue::quantity;
      return out;
    }
    out.count = plan.count;
    for (std::size_t i = 0; i < plan.count; ++i) {
      const auto& step = plan.steps[i];
      const auto kind = step.kind == station::Kind::back         ? CraftingStepKind::back
                        : step.kind == station::Kind::open_owner ? CraftingStepKind::open_owner
                                                                 : CraftingStepKind::effect;
      out.steps[i] = {kind,
                      step.invocation,
                      step.owner_instance,
                      step.function_symbol,
                      crafting_page(step.before),
                      crafting_page(step.after)};
    }
  } else if (recipe->tobacco) {
    if (source.kind != CraftingPageKind::alchemy || !tobacco::canonical(*recipe->tobacco))
      return out;
    if (!quantity || quantity > INT32_MAX) {
      out.issue = CraftingPlanIssue::quantity;
      return out;
    }
    // Up to the tobacco page, the path is the same as to the marsh herb roller.
    const auto nav = alchemy_extra_navigation::make_plan(
        static_cast<base::Page>(source.value), alchemy_recipe_catalog::RecipeId::swamp_roll, 1);
    if (!nav.valid() || nav.count + 1 > out.steps.size())
      return out;
    for (std::size_t i = 0; i + 1 < nav.count; ++i) {
      const auto& step = nav.steps[i];
      const auto kind = step.kind == base::Kind::back   ? CraftingStepKind::back
                        : step.kind == base::Kind::stop ? CraftingStepKind::stop
                                                        : CraftingStepKind::start;
      out.steps[out.count++] = {kind,
                                step.invocation,
                                step.owner_instance,
                                step.function_symbol,
                                crafting_page(step.before),
                                crafting_page(step.after)};
    }
    const CraftingPage choices{CraftingPageKind::tobacco_choices, tobacco::owner.instance};
    out.steps[out.count++] = {CraftingStepKind::open_owner,
                              base::Invocation::info,
                              20399,
                              20401,
                              crafting_page(base::Page::tobacco_info),
                              choices};
    out.steps[out.count++] = {CraftingStepKind::effect,
                              base::Invocation::choice,
                              20399,
                              static_cast<int>(recipe->tobacco->callback.function->symbol),
                              choices,
                              {}};
  } else if (recipe->route == crafting_catalog::Route::alchemy_extra) {
    if (source.kind != CraftingPageKind::alchemy) {
      out.issue = CraftingPlanIssue::station;
      return out;
    }
    const auto plan = alchemy_extra_navigation::make_plan(static_cast<base::Page>(source.value),
                                                          recipe->alchemy_extra->id, quantity);
    if (!alchemy_extra_navigation::structurally_valid(plan)) {
      if (plan.issue == base::Issue::quantity)
        out.issue = CraftingPlanIssue::quantity;
      return out;
    }
    out.count = plan.count;
    for (std::size_t i = 0; i < plan.count; ++i) {
      const auto& step = plan.steps[i];
      CraftingStepKind kind = CraftingStepKind::back;
      switch (step.kind) {
      case base::Kind::back:
        kind = CraftingStepKind::back;
        break;
      case base::Kind::stop:
        kind = CraftingStepKind::stop;
        break;
      case base::Kind::start:
        kind = CraftingStepKind::start;
        break;
      case base::Kind::open_owner:
        kind = CraftingStepKind::open_owner;
        break;
      case base::Kind::effect:
        kind = CraftingStepKind::effect;
        break;
      }
      out.steps[i] = {kind,
                      step.invocation,
                      step.owner_instance,
                      step.function_symbol,
                      crafting_page(step.before),
                      crafting_page(step.after)};
    }
  } else if (recipe->route == crafting_catalog::Route::spirit) {
    if (source.kind != CraftingPageKind::alchemy) {
      out.issue = CraftingPlanIssue::station;
      return out;
    }
    const auto plan = spirit_recipe_navigation::make_plan(
        static_cast<base::Page>(source.value), recipe->spirit->id, quantity,
        spirit_recipe_catalog::branch_for(branch_known, branch_value));
    if (!spirit_recipe_navigation::structurally_valid(plan)) {
      if (plan.issue == base::Issue::quantity)
        out.issue = CraftingPlanIssue::quantity;
      return out;
    }
    out.count = plan.count;
    for (std::size_t i = 0; i < plan.count; ++i) {
      const auto& step = plan.steps[i];
      CraftingStepKind kind = CraftingStepKind::back;
      switch (step.kind) {
      case base::Kind::back:
        kind = CraftingStepKind::back;
        break;
      case base::Kind::stop:
        kind = CraftingStepKind::stop;
        break;
      case base::Kind::start:
        kind = CraftingStepKind::start;
        break;
      case base::Kind::open_owner:
        kind = CraftingStepKind::open_owner;
        break;
      case base::Kind::effect:
        kind = CraftingStepKind::effect;
        break;
      }
      out.steps[i] = {kind,
                      step.invocation,
                      step.owner_instance,
                      step.function_symbol,
                      crafting_page(step.before),
                      crafting_page(step.after)};
    }
  } else if (recipe->route == crafting_catalog::Route::bow) {
    if (source.kind != CraftingPageKind::bow) {
      out.issue = CraftingPlanIssue::station;
      return out;
    }
    const auto plan = bow_recipe_navigation::make_plan(
        static_cast<bow_recipe_navigation::Page>(source.value), recipe->bow->id, quantity);
    if (!bow_recipe_navigation::structurally_valid(plan)) {
      if (plan.issue == bow_recipe_navigation::Issue::quantity)
        out.issue = CraftingPlanIssue::quantity;
      return out;
    }
    out.count = plan.count;
    for (std::size_t i = 0; i < plan.count; ++i) {
      const auto& step = plan.steps[i];
      out.steps[i] = {step.kind == bow_recipe_navigation::Kind::effect
                          ? CraftingStepKind::effect
                          : CraftingStepKind::open_owner,
                      step.invocation,
                      step.owner_instance,
                      static_cast<std::int32_t>(step.function_symbol),
                      crafting_page(step.before),
                      crafting_page(step.after)};
    }
  } else if (recipe->route == crafting_catalog::Route::cool) {
    if (source.kind != CraftingPageKind::cool) {
      out.issue = CraftingPlanIssue::station;
      return out;
    }
    if (!material || material->id != target || !recipe->cool)
      return out;
    const cool_recipe_navigation::Page page{true, (source.value & 1u) != 0,
                                            (source.value & 2u) != 0};
    const auto plan = cool_recipe_navigation::make_plan(
        *recipe->cool, crafting_cool_evidence(*material), quantity, page);
    if (!plan.valid) {
      out.issue = CraftingPlanIssue::quantity;
      return out;
    }
    out.material_snapshot = *material;
    out.steps[0] = {CraftingStepKind::effect,
                    plan.step.invocation,
                    static_cast<std::int32_t>(plan.step.owner),
                    static_cast<std::int32_t>(plan.step.function),
                    source,
                    {}};
    out.count = 1;
  } else if (recipe->route == crafting_catalog::Route::grind) {
    namespace nav = grind_recipe_navigation;
    if (source.kind != CraftingPageKind::grind) {
      out.issue = CraftingPlanIssue::station;
      return out;
    }
    if (!material || material->id != target || !recipe->grind ||
        crafting_quantity_block_reason(*material, quantity) != CraftingBlockReason::none)
      return out;
    const auto page = crafting_grind_page(source);
    const auto evidence = crafting_grind_evidence(*material);
    const auto plan = nav::make_plan(page, *recipe->grind, evidence, quantity);
    if (!nav::valid_plan(plan, page, *recipe->grind, evidence) || plan.count > out.steps.size())
      return out;
    out.material_snapshot = *material;
    out.count = plan.count;
    auto current = source;
    for (std::size_t i = 0; i < plan.count; ++i) {
      const auto& step = plan.steps[i];
      CraftingPage after{};
      if (step.after != nav::Page::unknown)
        after = crafting_page(nav::PageEvidence{
            true, true, step.after, step.after == nav::Page::info ? 0u : recipe->grind->category});
      const auto kind = step.kind == nav::Kind::effect ? CraftingStepKind::effect
                        : step.kind == nav::Kind::back ? CraftingStepKind::back
                                                       : CraftingStepKind::open_owner;
      out.steps[i] = {kind,
                      step.invocation,
                      static_cast<std::int32_t>(step.owner),
                      static_cast<std::int32_t>(step.function),
                      current,
                      after};
      current = after;
    }
  } else if (recipe->route == crafting_catalog::Route::anvil ||
             recipe->route == crafting_catalog::Route::ore) {
    if (source.kind != CraftingPageKind::anvil) {
      out.issue = CraftingPlanIssue::station;
      return out;
    }
    const auto copy = [&](const auto& plan) noexcept {
      out.count = plan.count;
      for (std::size_t i = 0; i < plan.count; ++i) {
        const auto& step = plan.steps[i];
        const auto kind = step.kind == anvil_recipe_navigation::Kind::back ? CraftingStepKind::back
                          : step.kind == anvil_recipe_navigation::Kind::open_owner
                              ? CraftingStepKind::open_owner
                              : CraftingStepKind::effect;
        out.steps[i] = {kind,
                        step.invocation,
                        step.owner_instance,
                        step.function_symbol,
                        crafting_page(step.before),
                        crafting_page(step.after)};
      }
    };
    if (recipe->route == crafting_catalog::Route::anvil) {
      const auto plan = anvil_recipe_navigation::make_plan(
          static_cast<anvil_recipe_navigation::Page>(source.value), recipe->anvil->id, quantity);
      if (!anvil_recipe_navigation::structurally_valid(plan)) {
        if (plan.issue == anvil_recipe_navigation::Issue::quantity)
          out.issue = CraftingPlanIssue::quantity;
        return out;
      }
      copy(plan);
    } else {
      const auto plan = ore_recipe_navigation::make_plan(
          static_cast<anvil_recipe_navigation::Page>(source.value), recipe->ore->id, quantity);
      if (!ore_recipe_navigation::structurally_valid(plan)) {
        if (plan.issue == ore_recipe_navigation::Issue::quantity)
          out.issue = CraftingPlanIssue::quantity;
        return out;
      }
      copy(plan);
    }
  } else {
    return out;
  }
  out.issue = CraftingPlanIssue::none;
  return out;
}
inline CraftingPlan make_crafting_plan(CraftingPage source,
                                       const CraftingRequest& request) noexcept {
  return make_crafting_plan(source, request.id, request.quantity,
                            request.material_snapshot.branch_known,
                            request.material_snapshot.branch_value, &request.material_snapshot);
}
inline bool valid_crafting_plan(const CraftingPlan& plan) noexcept {
  if (!plan.valid())
    return false;
  const auto expected =
      make_crafting_plan(plan.source, plan.target, plan.quantity, plan.branch_known,
                         plan.branch_value, &plan.material_snapshot);
  if (!expected.valid() || expected.count != plan.count ||
      !same_crafting_state(plan.material_snapshot, expected.material_snapshot))
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

enum class CraftingHandoffState {
  idle,
  awaiting_step,
  ticket_ready,
  dispatched,
  executing,
  awaiting_collection,
  quarantined,
  cancelled,
  result_pending,
  finished
};
enum class CraftingHandoffIssue {
  none,
  request,
  plan,
  boundary,
  context,
  page,
  collection,
  resources,
  hands,
  focus,
  cancelled,
  timeout,
  ticket,
  offer,
  caller,
  reentry,
  counter
};
enum class CraftingHandoffDecision { ordinary, block, execute };
struct CraftingHandoffOffer {
  bool valid = false;
  recipe_catalog::Invocation mode = recipe_catalog::Invocation::info;
  std::int32_t owner = -1, function = -1, row = -1;
  std::uint64_t collection = 0;
};
struct CraftingHandoffObservation {
  CraftingKey key{};
  std::uint32_t view = 0;
  std::uint64_t frame = 0, collection = 0;
  // complete/ready: full own collection, pending opened original
  // dialog, no animation/IsDone. Every truth value is a current
  // native evidence; neither static plan nor SHA replaces it.
  bool context_known = false, page_known = false, complete = false, ready = false;
  bool outer_frame = false, quiescent = false, focused = false;
  CraftingPage page{};
  recipe_catalog::Invocation mode = recipe_catalog::Invocation::info;
  std::int32_t choice_owner = -1, count = 0;
  CraftingState materials{};
  StationRecipeHands hands{};
};
struct CraftingHandoffTicket {
  std::uint64_t id = 0, frame = 0;
  std::size_t step = 0;
  std::uint32_t view = 0;
  CraftingHandoffOffer offer{};
};
struct CraftingHandoffCallbackProof {
  bool own_thread = false, canonical_caller = false, native_update = false;
  unsigned update_depth = 0, callback_depth = 0, other_effect_depth = 0, collection_depth = 0,
           station_depth = 0;
};
struct CraftingHandoffCallback {
  CraftingHandoffDecision decision = CraftingHandoffDecision::ordinary;
  std::uint64_t ticket_id = 0, execution_id = 0;
};
struct CraftingHandoffEndWitness {
  CraftingKey key{};
  std::uint32_t view = 0;
  std::uint64_t frame = 0;
  bool confirmed = false, outer_frame = false, quiescent = false;
};
namespace crafting_handoff_detail {
inline bool same_offer(const CraftingHandoffOffer& a, const CraftingHandoffOffer& b) noexcept {
  return a.valid == b.valid && a.mode == b.mode && a.owner == b.owner && a.function == b.function &&
         a.row == b.row && a.collection == b.collection;
}
inline bool same_ticket(const CraftingHandoffTicket& a, const CraftingHandoffTicket& b) noexcept {
  return a.id && a.id == b.id && a.frame == b.frame && a.step == b.step && a.view == b.view &&
         same_offer(a.offer, b.offer);
}
inline bool same_resources(const CraftingState& a, const CraftingState& b) noexcept {
  return valid_crafting_state(a) && valid_crafting_state(b) && same_crafting_state(a, b);
}
inline bool valid_hands(const CraftingRequest& request, const StationRecipeHands& hands) noexcept {
  const auto* recipe = crafting_catalog::find(request.id);
  return recipe && crafting_detail::bound_hands(*recipe, hands);
}
inline bool valid_request(const CraftingRequest& request) noexcept {
  const auto* recipe = crafting_catalog::find(request.id);
  const auto& state = request.material_snapshot;
  return recipe && crafting_detail::valid_key(request.key) && recipe->station == request.key.kind &&
         request.quantity && request.quantity <= state.maximum && state.id == request.id &&
         state.generation == request.key.generation && state.evaluated &&
         state.knowledge_read_valid && state.known && state.contract && state.hand_known &&
         state.material_known && state.material_verified && valid_crafting_state(state) &&
         crafting_quantity_block_reason(state, request.quantity) == CraftingBlockReason::none &&
         valid_hands(request, request.hands);
}
inline bool page_mode(CraftingPage page, recipe_catalog::Invocation mode,
                      std::int32_t owner) noexcept {
  if (!valid_crafting_page(page))
    return false;
  if (page.kind == CraftingPageKind::tobacco_choices)
    return mode == recipe_catalog::Invocation::choice && owner == 20399;
  if (page.kind == CraftingPageKind::stove_info || page.kind == CraftingPageKind::dndr_alchemy ||
      page.kind == CraftingPageKind::dndr_anvil)
    return mode == recipe_catalog::Invocation::info;
  if (page.kind == CraftingPageKind::bow || page.kind == CraftingPageKind::cool)
    return mode == recipe_catalog::Invocation::info;
  if (page.kind == CraftingPageKind::grind) {
    const auto decoded = crafting_grind_page(page);
    return decoded.page == grind_recipe_navigation::Page::info
               ? mode == recipe_catalog::Invocation::info
               : mode == recipe_catalog::Invocation::choice && owner == 20957;
  }
  if (page.kind == CraftingPageKind::alchemy) {
    const auto* route =
        recipe_navigation::route_for(static_cast<recipe_navigation::Page>(page.value));
    return route ? mode == recipe_catalog::Invocation::choice && owner == route->owner->instance
                 : mode == recipe_catalog::Invocation::info;
  }
  if (page.kind == CraftingPageKind::anvil) {
    const auto* recipe =
        anvil_recipe_navigation::recipe_for(static_cast<anvil_recipe_navigation::Page>(page.value));
    return recipe ? mode == recipe_catalog::Invocation::choice && owner == recipe->owner.instance
                  : mode == recipe_catalog::Invocation::info;
  }
  const auto* route = station_recipe_navigation::route_for(
      static_cast<station_recipe_navigation::Page>(page.value));
  return route ? mode == recipe_catalog::Invocation::choice && owner == route->owner->instance
               : mode == recipe_catalog::Invocation::info;
}
} // namespace crafting_handoff_detail

// The adapter serializes on the bound game thread. Tickets allow exactly
// one original selection; no own VM execution. enter_callback lies
// BEFORE original OnChoice. For the inner CallFunc additionally applies
// the native PostRemoveChoice witness, never this full Pre-snapshot.
// After the effect, the separate result check remains mandatory.
class CraftingHandoff {
public:
  bool begin(const CraftingRequest& request, const CraftingPlan& plan,
             const CraftingHandoffObservation& initial,
             std::uint64_t max_step_frames = 600) noexcept {
    if (active() || !request.key.generation || request.key.generation <= last_generation_)
      return false;
    const auto previous_frame = last_frame_;
    last_generation_ = request.key.generation;
    request_ = request;
    plan_ = plan;
    view_ = initial.view;
    step_ = 0;
    ticket_ = {};
    callback_active_ = returned_ = cancelled_ = effect_started_ = false;
    execution_id_ = 0;
    callback_frame_ = 0;
    last_frame_ = initial.frame < previous_frame ? previous_frame : initial.frame;
    step_frame_ = initial.frame;
    frame_limit_ = max_step_frames;
    state_ = CraftingHandoffState::awaiting_step;
    issue_ = CraftingHandoffIssue::none;
    if (!crafting_handoff_detail::valid_request(request)) {
      cancel(CraftingHandoffIssue::request);
      return false;
    }
    if (!valid_plan()) {
      cancel(CraftingHandoffIssue::plan);
      return false;
    }
    if (!frame_limit_ || !initial.frame || initial.frame < previous_frame || !initial.outer_frame ||
        !initial.quiescent) {
      cancel(CraftingHandoffIssue::boundary);
      return false;
    }
    if (!check_common(initial) || !ready_page(initial, plan.steps[0].before)) {
      if (!cancelled_)
        cancel(CraftingHandoffIssue::page);
      return false;
    }
    return true;
  }

  void observe(const CraftingHandoffObservation& value) noexcept {
    if (!active() || state_ == CraftingHandoffState::result_pending)
      return;
    if (!value.outer_frame || !value.quiescent || callback_active_) {
      cancel(CraftingHandoffIssue::boundary);
      return;
    }
    if (!value.frame || value.frame < last_frame_) {
      cancel(CraftingHandoffIssue::boundary);
      return;
    }
    last_frame_ = value.frame;
    if (state_ == CraftingHandoffState::quarantined) {
      // No semantic recipe grant necessary, only positive evidence
      // that the old IsDone/Close is no longer repeatable.
      if (same_scope(value) && value.complete && value.ready && value.count >= 0 &&
          value.count <= 256 && value.collection > ticket_.offer.collection &&
          value.frame > ticket_.frame) {
        state_ = CraftingHandoffState::cancelled;
        ticket_ = {};
      }
      return;
    }
    if (!check_common(value))
      return;
    if (state_ == CraftingHandoffState::awaiting_collection && returned_ &&
        value.frame > callback_frame_ && value.collection > ticket_.offer.collection &&
        value.complete && value.ready) {
      if (!ready_page(value, plan_.steps[step_].after)) {
        cancel(CraftingHandoffIssue::page);
        return;
      }
      ++step_;
      if (step_ >= plan_.count) {
        cancel(CraftingHandoffIssue::plan);
        return;
      }
      ticket_ = {};
      returned_ = false;
      step_frame_ = value.frame;
      state_ = CraftingHandoffState::awaiting_step;
    }
  }

  std::optional<CraftingHandoffTicket> ticket(const CraftingHandoffObservation& value,
                                              const CraftingHandoffOffer& offer) noexcept {
    observe(value);
    if (state_ != CraftingHandoffState::awaiting_step)
      return std::nullopt;
    if (!ready_page(value, plan_.steps[step_].before)) {
      cancel(CraftingHandoffIssue::page);
      return std::nullopt;
    }
    if (!matches_offer(value, offer)) {
      cancel(CraftingHandoffIssue::offer);
      return std::nullopt;
    }
    if (ticket_serial_ == UINT64_MAX) {
      cancel(CraftingHandoffIssue::counter);
      return std::nullopt;
    }
    ticket_ = {++ticket_serial_, value.frame, step_, view_, offer};
    state_ = CraftingHandoffState::ticket_ready;
    return ticket_;
  }

  bool dispatch(const CraftingHandoffTicket& ticket, const CraftingHandoffObservation& value,
                const CraftingHandoffOffer& fresh) noexcept {
    if (state_ != CraftingHandoffState::ticket_ready)
      return false;
    if (!crafting_handoff_detail::same_ticket(ticket_, ticket) || value.frame != ticket_.frame) {
      cancel(CraftingHandoffIssue::ticket);
      return false;
    }
    if (!value.outer_frame || !value.quiescent || !check_common(value) ||
        !ready_page(value, plan_.steps[step_].before) || !matches_offer(value, fresh) ||
        !crafting_handoff_detail::same_offer(ticket_.offer, fresh)) {
      if (!cancelled_)
        cancel(CraftingHandoffIssue::offer);
      return false;
    }
    // Consume before the native Select/Enter. An ignored/unclear
    // native input is never repaired by a second attempt.
    state_ = CraftingHandoffState::dispatched;
    return true;
  }

  // Non-consuming pre-check for the normal info-result watcher.
  // OnInfo/OnChoice must still individually pass enter_callback later.
  bool matches_pending(const CraftingHandoffTicket& ticket,
                       const CraftingHandoffObservation& actual,
                       const CraftingHandoffOffer& offer) const noexcept {
    return state_ == CraftingHandoffState::dispatched && !cancelled_ && actual.complete &&
           crafting_handoff_detail::same_ticket(ticket_, ticket) && same_scope(actual) &&
           actual.focused && page_matches(actual, plan_.steps[step_].before) &&
           matches_offer(actual, offer) &&
           crafting_handoff_detail::same_offer(ticket_.offer, offer) &&
           crafting_handoff_detail::same_resources(request_.material_snapshot, actual.materials) &&
           same_crafting_hands(request_.hands, actual.hands) && actual.frame >= last_frame_ &&
           actual.frame - step_frame_ <= frame_limit_;
  }

  CraftingHandoffCallback enter_callback(const CraftingHandoffTicket& ticket,
                                         const CraftingHandoffObservation& actual,
                                         const CraftingHandoffOffer& offer,
                                         const CraftingHandoffCallbackProof& proof) noexcept {
    if (!active())
      return {};
    const CraftingHandoffCallback blocked{CraftingHandoffDecision::block, 0, 0};
    // A positively different owner rejects the remainder. A delayed
    // old Callback remains bound, even with a different row/Collection.
    // Only a positively identified different Callback is foreign;
    // a read gap is not such evidence. Before Dispatch there was
    // no order to the engine and thus no old Callback to protect.
    if (actual.context_known && actual.view && !same_scope(actual)) {
      const bool native_pending = ticket_.id && state_ != CraftingHandoffState::ticket_ready;
      const bool identified = offer.valid && offer.owner >= 0 && offer.function >= 0 &&
                              (offer.mode == recipe_catalog::Invocation::info ||
                               offer.mode == recipe_catalog::Invocation::choice);
      const bool old_callback =
          offer.owner == ticket_.offer.owner && offer.function == ticket_.offer.function;
      cancel(CraftingHandoffIssue::context);
      return !native_pending || (identified && !old_callback) ? CraftingHandoffCallback{} : blocked;
    }
    if (!proof.own_thread || !proof.canonical_caller || !proof.native_update ||
        proof.update_depth != 1 || proof.callback_depth != 1 || proof.other_effect_depth ||
        proof.collection_depth || proof.station_depth) {
      cancel(CraftingHandoffIssue::caller);
      return blocked;
    }
    if (callback_active_ || state_ == CraftingHandoffState::executing) {
      cancel(CraftingHandoffIssue::reentry);
      return blocked;
    }
    if (!matches_pending(ticket, actual, offer)) {
      cancel(CraftingHandoffIssue::ticket);
      return blocked;
    }
    if (execution_serial_ == UINT64_MAX) {
      cancel(CraftingHandoffIssue::counter);
      return blocked;
    }
    execution_id_ = ++execution_serial_;
    last_frame_ = callback_frame_ = actual.frame;
    callback_active_ = true;
    effect_started_ = plan_.steps[step_].is_effect();
    state_ = CraftingHandoffState::executing;
    return {CraftingHandoffDecision::execute, ticket_.id, execution_id_};
  }

  bool callback_return(const CraftingHandoffCallback& witness) noexcept {
    if (witness.decision != CraftingHandoffDecision::execute)
      return false;
    if (!callback_active_ || witness.ticket_id != ticket_.id ||
        witness.execution_id != execution_id_)
      return false;
    callback_active_ = false;
    returned_ = true;
    // Do not check next ticket or page here: the original
    // update must have fully left its Collect-/Open follow-up.
    state_ = effect_started_ ? CraftingHandoffState::result_pending
             : cancelled_    ? CraftingHandoffState::quarantined
                             : CraftingHandoffState::awaiting_collection;
    return true;
  }

  bool reject_unpaid_tobacco(const CraftingHandoffCallback& witness) noexcept {
    const auto* recipe = crafting_catalog::find(request_.id);
    if (!recipe || !recipe->tobacco || !cancelled_ || !callback_active_ ||
        witness.decision != CraftingHandoffDecision::execute || witness.ticket_id != ticket_.id ||
        witness.execution_id != execution_id_ ||
        ticket_.offer.function != static_cast<int>(effect_started_
                                                       ? recipe->tobacco->callback.function->symbol
                                                       : tobacco::owner.information->symbol))
      return false;
    // The unpaid owner guards the child page independently of the handoff.
    callback_active_ = effect_started_ = false;
    returned_ = true;
    state_ = CraftingHandoffState::cancelled;
    ticket_ = {};
    return true;
  }
  void cancel(CraftingHandoffIssue issue = CraftingHandoffIssue::cancelled) noexcept {
    if (state_ == CraftingHandoffState::idle || state_ == CraftingHandoffState::finished ||
        state_ == CraftingHandoffState::cancelled)
      return;
    cancelled_ = true;
    if (issue_ == CraftingHandoffIssue::none)
      issue_ = issue;
    if (state_ == CraftingHandoffState::result_pending)
      return; // Already started effect needs result check.
    if (state_ == CraftingHandoffState::dispatched || state_ == CraftingHandoffState::executing ||
        state_ == CraftingHandoffState::awaiting_collection ||
        state_ == CraftingHandoffState::quarantined || callback_active_)
      state_ = CraftingHandoffState::quarantined;
    else {
      state_ = CraftingHandoffState::cancelled;
      ticket_ = {};
    }
  }

  bool context_ended(const CraftingHandoffEndWitness& witness) noexcept {
    if (!active() || state_ == CraftingHandoffState::result_pending || callback_active_ ||
        !witness.confirmed || !witness.outer_frame || !witness.quiescent || !witness.frame ||
        witness.frame < last_frame_ || !(witness.key == request_.key) || witness.view != view_)
      return false;
    cancelled_ = true;
    if (issue_ == CraftingHandoffIssue::none)
      issue_ = CraftingHandoffIssue::context;
    state_ = CraftingHandoffState::cancelled;
    ticket_ = {};
    last_frame_ = witness.frame;
    return true;
  }

  // Only after the SEPARATE positive material/station completion check.
  // false and a mere CallbackReturn do not grant further production.
  bool acknowledge_result(bool verified) noexcept {
    if (!verified || state_ != CraftingHandoffState::result_pending || callback_active_)
      return false;
    state_ = CraftingHandoffState::finished;
    ticket_ = {};
    return true;
  }

  bool active() const noexcept {
    return state_ != CraftingHandoffState::idle && state_ != CraftingHandoffState::cancelled &&
           state_ != CraftingHandoffState::finished;
  }
  // Even the native View can be reused in a later world/station.
  // Positively different ownership remains free even at the same
  // address; unknown ownership of the bound Views remains guarded.
  bool guards_input(const CraftingHandoffObservation& actual) const noexcept {
    return active() && actual.view && actual.view == view_ &&
           (!actual.context_known || actual.key == request_.key);
  }
  CraftingHandoffState state() const noexcept { return state_; }
  CraftingHandoffIssue issue() const noexcept { return issue_; }
  bool cancelled() const noexcept { return cancelled_; }
  std::size_t step_index() const noexcept { return step_; }
  const CraftingStep* step() const noexcept {
    return active() && step_ < plan_.count ? &plan_.steps[step_] : nullptr;
  }
  const CraftingHandoffTicket* current_ticket() const noexcept {
    return ticket_.id ? &ticket_ : nullptr;
  }
  const CraftingRequest& request() const noexcept { return request_; }

private:
  bool valid_plan() const noexcept {
    const auto* recipe = crafting_catalog::find(request_.id);
    return plan_.target == request_.id && plan_.quantity == request_.quantity &&
           plan_.branch_known == request_.material_snapshot.branch_known &&
           plan_.branch_value == request_.material_snapshot.branch_value &&
           (recipe && (!(recipe->grind || recipe->cool) ||
                       crafting_handoff_detail::same_resources(plan_.material_snapshot,
                                                               request_.material_snapshot))) &&
           valid_crafting_plan(plan_);
  }
  bool same_scope(const CraftingHandoffObservation& value) const noexcept {
    return value.context_known && view_ && value.view == view_ && value.key == request_.key;
  }
  bool page_matches(const CraftingHandoffObservation& value, CraftingPage page) const noexcept {
    return value.page_known && value.page == page &&
           crafting_handoff_detail::page_mode(page, value.mode, value.choice_owner);
  }
  bool ready_page(const CraftingHandoffObservation& value, CraftingPage page) const noexcept {
    return value.complete && value.ready && value.collection && value.count > 0 &&
           value.count <= 256 && page_matches(value, page);
  }
  bool matches_offer(const CraftingHandoffObservation& value,
                     const CraftingHandoffOffer& offer) const noexcept {
    if (step_ >= plan_.count)
      return false;
    const auto& step = plan_.steps[step_];
    return offer.valid && value.count > 0 && value.count <= 256 && offer.row >= 0 &&
           offer.row < value.count && offer.collection && offer.collection == value.collection &&
           offer.mode == value.mode && offer.mode == step.invocation &&
           offer.owner == step.owner_instance && offer.function == step.function_symbol;
  }
  bool check_common(const CraftingHandoffObservation& value) noexcept {
    if (!same_scope(value)) {
      cancel(CraftingHandoffIssue::context);
      return false;
    }
    if (!value.focused) {
      cancel(CraftingHandoffIssue::focus);
      return false;
    }
    if (!value.frame || value.frame < last_frame_ || value.frame < step_frame_) {
      cancel(CraftingHandoffIssue::boundary);
      return false;
    }
    if (value.frame - step_frame_ > frame_limit_) {
      cancel(CraftingHandoffIssue::timeout);
      return false;
    }
    if (!crafting_handoff_detail::same_resources(request_.material_snapshot, value.materials)) {
      cancel(CraftingHandoffIssue::resources);
      return false;
    }
    if (!crafting_handoff_detail::valid_hands(request_, value.hands) ||
        !same_crafting_hands(request_.hands, value.hands)) {
      cancel(CraftingHandoffIssue::hands);
      return false;
    }
    return true;
  }

  CraftingRequest request_{};
  CraftingPlan plan_{};
  CraftingHandoffTicket ticket_{};
  std::uint32_t view_ = 0;
  std::size_t step_ = 0;
  std::uint64_t last_generation_ = 0, last_frame_ = 0, step_frame_ = 0, frame_limit_ = 0;
  std::uint64_t ticket_serial_ = 0, execution_serial_ = 0, execution_id_ = 0, callback_frame_ = 0;
  bool callback_active_ = false, returned_ = false, cancelled_ = false, effect_started_ = false;
  CraftingHandoffState state_ = CraftingHandoffState::idle;
  CraftingHandoffIssue issue_ = CraftingHandoffIssue::none;
};

} // namespace ubc
