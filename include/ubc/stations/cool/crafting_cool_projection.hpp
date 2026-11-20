// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/stations/cool/cool_actor_preview.hpp"
#include "ubc/stations/cool/cool_direct_contract.hpp"
#include "ubc/stations/cool/cool_predebit_contract.hpp"
#include "ubc/core/crafting_handoff.hpp"

namespace ubc {
inline cool_recipe_navigation::Page
cool_collection_page(const ReadOnlyMemory& memory, const SelectionInfoRows& rows,
                     const SelectionDialog& dialog, std::int32_t production, bool reread) noexcept {
  namespace cat = cool_recipe_catalog;
  std::array<cool_recipe_navigation::Offer, 3> offers{};
  if (!rows.generation || !rows.complete(dialog) || !rows.count || rows.count > offers.size())
    return {};
  for (std::size_t i = 0; i < rows.count; ++i) {
    const auto& saved = rows.rows[i];
    const auto row = reread ? read_selection_info(memory, saved.address) : saved;
    const auto& owner = saved.instance == cat::single.instance   ? cat::single
                        : saved.instance == cat::triple.instance ? cat::triple
                                                                 : cat::end_owner;
    if (row.instance != saved.instance || !cool_direct_contract::detail::matches(row, owner))
      return {};
    offers[i] = {static_cast<std::uint32_t>(row.instance),
                 static_cast<std::uint32_t>(row.condition),
                 static_cast<std::uint32_t>(row.function)};
  }
  return cool_recipe_navigation::identify_page(production, offers.data(), rows.count, true);
}

template <class WriterReady>
inline bool cool_quantity_matches(const CraftingRequest& request, const CraftingSnapshot& fresh,
                                  cool_recipe_navigation::Page page,
                                  WriterReady writer_ready) noexcept {
  const auto& state = fresh.recipes[crafting_catalog::cool_begin];
  if (request.key.kind != crafting_catalog::Station::cool ||
      !crafting_handoff_detail::valid_request(request) || !valid_crafting_snapshot(fresh) ||
      !(fresh.key == request.key) || !same_crafting_state(state, request.material_snapshot) ||
      !same_crafting_hands(fresh.hands, request.hands))
    return false;
  const auto plan = cool_recipe_navigation::make_plan(
      cool_recipe_catalog::recipe, crafting_cool_evidence(state), request.quantity, page);
  return plan.valid && (!plan.amount.requires_extra_debit || writer_ready());
}
template <class WriterReady>
inline bool cool_quantity_available(const CraftingRequest& request, const CraftingSnapshot& fresh,
                                    cool_recipe_navigation::Page page,
                                    WriterReady writer_ready) noexcept {
  return cool_quantity_matches(request, fresh, page,
                               [&]() { return cool_predebit::runtime_ready && writer_ready(); });
}

// Pure projection of the existing complete N0 snapshot. The physical
// preview never alters the raw SELF/HERO evidence for the writer.
inline CraftingSnapshot project_capture(const CoolRecipeCatalogueCapture& source,
                                        CoolActorPurpose purpose = CoolActorPurpose::original_entry,
                                        bool physical_hero_preview_verified = false) noexcept {
  namespace cat = crafting_catalog;
  namespace rt = cool_recipe_runtime;
  CraftingSnapshot out;
  const auto& k = source.key;
  const auto& d = source.diagnostic;
  const auto& s = source.recipe;
  const auto& m = s.material;
  auto& value = out.recipes[cat::cool_begin];
  out.key = {k.generation, k.thread, k.world, k.hero, k.station, k.manager, cat::Station::cool};
  out.hands = source.hands;
  if (!crafting_detail::valid_key(out.key) || k.production_kind != 13 ||
      d.issue != CoolCaptureIssue::none) {
    out.issue = CraftingProjectionIssue::context;
    return out;
  }
  if (s.id != cool_recipe_catalog::RecipeId::blade || m.eligible || m.numerical.quantity ||
      m.numerical.valid || m.numerical.callback || m.numerical.owner || m.numerical.count ||
      m.numerical.native_end_debit || m.numerical.extra_debit || m.numerical.requires_extra_debit ||
      m.numerical.requires_quantity_receipt ||
      (purpose != CoolActorPurpose::hero_preview && purpose != CoolActorPurpose::original_entry)) {
    out.issue = CraftingProjectionIssue::source;
    return out;
  }
  if (!s.evaluated)
    return out;
  value.evaluated = true;
  value.generation = k.generation;
  value.knowledge_read_valid = d.knowledge_unchanged && d.knowledge_before.read_valid &&
                               d.knowledge_after.read_valid &&
                               d.knowledge_before.issue == runtime_script::Issue::none &&
                               d.knowledge_after.issue == runtime_script::Issue::none &&
                               d.knowledge_before.known == d.knowledge_after.known;
  value.known = value.knowledge_read_valid && s.known && d.knowledge_before.known;
  value.contract = s.contract && d.profile.valid && d.profile.local.matches;
  const auto actor = cool_actor_binding(source, purpose, physical_hero_preview_verified);
  value.actor_known = actor.known;
  value.actor_matches = actor.matches;
  value.context_known = s.context_known && d.context_unchanged &&
                        d.context_before.issue == runtime_script::Issue::none &&
                        d.context_after.issue == runtime_script::Issue::none &&
                        rt::same_context_evidence(d.context_before, d.context_after);
  value.context_matches = value.context_known && s.context_matches &&
                          d.context_before.station_matches() && d.context_after.station_matches();
  value.material_known =
      m.inventory_known && d.inventory_unchanged && d.inventory_issue == PackedInventoryIssue::none;
  value.hand_known =
      m.hand_known && d.hands_unchanged && d.hand_issue == StationHandReadIssue::none;
  value.material_verified =
      value.material_known && value.hand_known && m.known() && m.ingredients_verified;
  for (std::size_t i = 0; i < value.target_count; ++i) {
    auto& row = value.materials[i];
    const auto& original = m.rows[i];
    if (original.symbol != row.symbol || original.unit_cost != row.unit_cost ||
        original.unit_output != row.unit_output || original.stored < -1 ||
        original.stored > INT32_MAX || original.reserved ||
        (value.material_known &&
         (!original.available_known || original.available != original.stored)) ||
        (value.hand_known && original.held != (i ? 0 : 1))) {
      out.issue = CraftingProjectionIssue::source;
      return out;
    }
    if (value.material_known)
      row.stored = static_cast<std::int32_t>(original.stored);
    if (value.hand_known)
      row.generated_prop = i ? 0 : 1;
  }
  value.maximum = value.material_verified ? m.maximum : 0;
  if (value.material_verified) {
    const auto numerical = crafting_cool_plan(value, 0);
    if (!numerical.known || numerical.maximum != m.maximum ||
        numerical.direct_maximum != m.direct_maximum || !m.numerical.known ||
        m.numerical.maximum != numerical.maximum ||
        m.numerical.direct_maximum != numerical.direct_maximum) {
      out.issue = CraftingProjectionIssue::source;
      return out;
    }
    if (!cool_predebit::runtime_ready)
      value.maximum = numerical.direct_maximum;
    for (std::size_t i = 0; i < 2; ++i)
      for (const auto* stage :
           {&m.numerical.after_callback, &m.numerical.before_end, &m.numerical.after_end}) {
        const auto& observed = (*stage)[i];
        const auto& expected = numerical.after_end[i];
        if (m.numerical.issue != cool_recipe::Issue::none || !observed.known ||
            (!i && !observed.unambiguous) || observed.symbol != expected.symbol ||
            observed.amount != expected.amount) {
          out.issue = CraftingProjectionIssue::source;
          return out;
        }
      }
  }
  if (!valid_crafting_snapshot(out) || !crafting_detail::stocks_agree(out))
    out.issue = CraftingProjectionIssue::source;
  return out;
}
} // namespace ubc
