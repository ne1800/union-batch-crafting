// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/grind/grind_recipe_runtime.hpp"
#include <utility>

namespace ubc::grind_recipe_navigation {
namespace g = grind_recipe;
using Invocation = recipe_catalog::Invocation;
enum class Page { unknown, info, categories, pal_mil, sld_djg };
struct PageEvidence {
  bool known = false, owner_present = false;
  Page page = Page::unknown;
  std::uint32_t category = 0;
};
struct InfoOffer { std::uint32_t owner = 0, condition = 0, function = 0; };
// IDs come from a fresh verified original collection. These values
// produce no observe_info/Choice lease and no engine pointers.
inline PageEvidence identify_info(std::int32_t production, const InfoOffer* offers,
                                 std::size_t count, bool collection_verified) noexcept {
  if (!collection_verified || production != 11 || !offers || !count || count > 6) return {};
  bool end = false, owner = false;
  std::array<bool, 4> trials{};
  for (std::size_t i = 0; i < count; ++i) {
    const auto& row = offers[i];
    if (row.owner == 20937 && row.condition == 20938 && row.function == 20939) {
      if (end) return {}; end = true;
    } else if (row.owner == 20957 && row.condition == 20958 && row.function == 20959) {
      if (owner) return {}; owner = true;
    } else {
      bool found = false;
      for (std::size_t trial = 0; trial < trials.size(); ++trial) {
        const auto id = 20945u + 3u * static_cast<std::uint32_t>(trial);
        if (row.owner == id && row.condition == id + 1 && row.function == id + 2) {
          if (trials[trial]) return {}; trials[trial] = found = true; break;
        }
      }
      if (!found) return {};
    }
  }
  return end ? PageEvidence{true, owner, Page::info, 0} : PageEvidence{};
}
inline PageEvidence identify_choices(std::int32_t production, std::uint32_t owner,
                                    const std::uint32_t* functions, std::size_t count,
                                    bool collection_verified) noexcept {
  if (!collection_verified || production != 11 || owner != 20957 || !functions || !count || count > 13) return {};
  for (std::size_t i = 0; i < count; ++i)
    for (std::size_t j = 0; j < i; ++j) if (functions[i] == functions[j]) return {};
  const auto contains = [&](std::uint32_t symbol) noexcept {
    for (std::size_t i = 0; i < count; ++i) if (functions[i] == symbol) return true;
    return false;
  };
  if (contains(20960) && count <= 2) {
    if (count == 1) return {true, true, Page::categories, 0};
    if (contains(20961)) return {true, true, Page::categories, 20961};
    if (contains(20962)) return {true, true, Page::categories, 20962};
    return {};
  }
  if (count == 11 && contains(20963)) {
    for (std::uint32_t callback = 20965; callback <= 20974; ++callback)
      if (!contains(callback)) return {};
    return {true, true, Page::pal_mil, 20961};
  }
  if (count == 13 && contains(20964)) {
    for (std::uint32_t callback = 20975; callback <= 20986; ++callback)
      if (!contains(callback)) return {};
    return {true, true, Page::sld_djg, 20962};
  }
  return {};
}
inline bool valid_page(const PageEvidence& p) noexcept {
  if (!p.known) return false;
  if (p.page == Page::info) return p.category == 0;
  if (!p.owner_present) return false;
  if (p.page == Page::categories) return p.category == 0 || p.category == 20961 || p.category == 20962;
  return (p.page == Page::pal_mil && p.category == 20961) || (p.page == Page::sld_djg && p.category == 20962);
}
enum class Kind { back, open_owner, open_category, effect };
enum class Issue { none, page, owner, amount, contract };
struct Step {
  Kind kind = Kind::effect;
  Invocation invocation = Invocation::info;
  std::uint32_t owner = 0, function = 0;
  Page before = Page::unknown, after = Page::unknown;
};
struct Plan {
  bool valid = false;
  Issue issue = Issue::contract;
  std::uint32_t target = 0, quantity = 0;
  Page source = Page::unknown;
  std::array<Step, 4> steps{};
  std::size_t count = 0;
  g::Plan amount{};
};
// Uses the finished quantity/skill calculation unchanged. All three
// BACKs only delete choices and go to the INFO root; BACK2/3 invoke
// NO supercategory. Each step needs a new original collection,
// current context and real native lease at the later connection.
inline Plan make_plan(const PageEvidence& page, const g::Descriptor& descriptor,
                      const g::Evidence& evidence, std::uint32_t quantity) noexcept {
  Plan out; out.source = page.page; out.quantity = quantity; out.target = descriptor.function.symbol;
  if (!valid_page(page)) { out.issue = Issue::page; return out; }
  if (page.page == Page::info && !page.owner_present) { out.issue = Issue::owner; return out; }
  out.amount = g::plan(descriptor, evidence, quantity);
  if (!out.amount.valid) { out.issue = Issue::amount; return out; }
  const auto destination = descriptor.category == 20961 ? Page::pal_mil : Page::sld_djg;
  Page current = page.page;
  const auto add = [&](Kind kind, Invocation invocation, std::uint32_t function, Page after) noexcept {
    if (out.count == out.steps.size()) return false;
    out.steps[out.count++] = {kind, invocation, 20957, function, current, after}; current = after; return true;
  };
  if (current != Page::info && current != destination &&
      !(current == Page::categories && page.category == descriptor.category)) {
    const auto back = current == Page::categories ? 20960u : current == Page::pal_mil ? 20963u : 20964u;
    if (!add(Kind::back, Invocation::choice, back, Page::info)) return Plan{};
  }
  if (current == Page::info && !add(Kind::open_owner, Invocation::info, 20959, Page::categories)) return Plan{};
  if (current == Page::categories && !add(Kind::open_category, Invocation::choice, descriptor.category, destination)) return Plan{};
  if (current != destination || !add(Kind::effect, Invocation::choice, descriptor.function.symbol, Page::unknown)) return Plan{};
  out.valid = true; out.issue = Issue::none; return out;
}
inline bool same_amount(const g::Plan& a, const g::Plan& b) noexcept {
  if (a.recipe != b.recipe || a.issue != b.issue || a.known != b.known || a.learned != b.learned ||
      a.guild_allowed != b.guild_allowed || a.valid != b.valid || a.quantity != b.quantity ||
      a.maximum != b.maximum || a.skill_increment != b.skill_increment || a.skill_before != b.skill_before ||
      a.skill_after != b.skill_after || a.count != b.count || a.count > a.operands.size()) return false;
  for (std::size_t i = 0; i < a.count; ++i) {
    const auto& x = a.operands[i]; const auto& y = b.operands[i];
    if (x.offset != y.offset || x.original != y.original || x.units != y.units || x.reserved != y.reserved || x.allow_zero != y.allow_zero) return false;
  }
  for (std::size_t i = 0; i < 3; ++i)
    for (const auto pair : {std::pair<const g::Stock*, const g::Stock*>{&a.after_callback[i], &b.after_callback[i]},
                           {&a.after_end[i], &b.after_end[i]}})
      if (pair.first->symbol != pair.second->symbol || pair.first->amount != pair.second->amount ||
          pair.first->known != pair.second->known || pair.first->unambiguous != pair.second->unambiguous) return false;
  return true;
}
inline bool valid_plan(const Plan& plan, const PageEvidence& page, const g::Descriptor& descriptor,
                       const g::Evidence& evidence) noexcept {
  const auto expected = make_plan(page, descriptor, evidence, plan.quantity);
  if (!plan.valid || !expected.valid || plan.issue != expected.issue || plan.source != expected.source ||
      plan.target != expected.target || plan.count != expected.count || plan.count > plan.steps.size() ||
      !same_amount(plan.amount, expected.amount)) return false;
  for (std::size_t i = 0; i < plan.count; ++i) {
    const auto& a = plan.steps[i]; const auto& b = expected.steps[i];
    if (a.kind != b.kind || a.invocation != b.invocation || a.owner != b.owner ||
        a.function != b.function || a.before != b.before || a.after != b.after) return false;
  }
  return true;
}
} // namespace ubc::grind_recipe_navigation
