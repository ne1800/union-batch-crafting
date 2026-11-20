// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/recipe_material.hpp"
#include "ubc/recipe_runtime.hpp"
#include "ubc/recipe_session.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ubc {

namespace recipe_snapshot_detail {
constexpr std::size_t target_capacity() noexcept {
  std::size_t result = 0;
  for (const auto& descriptor : recipe_catalog::recipes) result += descriptor.targets.count;
  return result;
}
} // namespace recipe_snapshot_detail

// Upper bound from exactly this catalog, no new global recipe bound.
// Repeated shared targets only occupy one active slot at runtime.
inline constexpr std::size_t recipe_catalogue_target_capacity = recipe_snapshot_detail::target_capacity();
static_assert(recipe_catalogue_target_capacity > 0, "Ein Katalog benoetigt Inventarziele");

enum class RecipeCatalogueIssue { none, catalogue, context, reentry, boundary };
struct RecipeReadDiagnostic {
  bool applicable = false, evaluated = false;
  recipe_runtime::AssemblyIssue assembly_issue = recipe_runtime::AssemblyIssue::none;
  runtime_script::RuntimeContract profile{};
  recipe_runtime::Knowledge knowledge{};
  RecipeMaterialIssue material_issue = RecipeMaterialIssue::none;
};
struct RecipeCatalogueDiagnostic {
  RecipeCatalogueIssue issue = RecipeCatalogueIssue::none;
  PackedInventoryIssue inventory_issue = PackedInventoryIssue::none;
  bool inventory_known = false, listed_known = false, packed_known = false;
  bool snapshot_unchanged = false, hands_read_valid = false;
  std::size_t target_count = 0;
  std::uint32_t listed_nodes = 0, packed_entries = 0;
  std::array<RecipeReadDiagnostic, recipe_catalog::recipes.size()> recipes{};
};
struct RecipeCatalogueCapture {
  CatalogueSnapshot snapshot{};
  HandsSnapshot hands{};
  RecipeCatalogueDiagnostic diagnostic{};
};

// Create once, preferably statically on the bound game thread. The
// large inventory workspace and the composed profiles belong to this
// object, not the callback stack. No engine calls/heap allocation/write
// accesses; ReadOnlyMemory is the only access to foreign data.
//
// The same game thread must verify the context identity before AND after
// capture and previously evidence the stable quiet bound and every allowed
// station hand contract. capturing_ merely prevents own synchronous
// re-entries; it is neither a thread lock nor evidence against foreign
// asynchronous writes. Returned snapshots/diagnostics contain only own value
// copies. Their hand object identities must never be dereferenced later.
class RecipeCatalogueReader {
public:
  RecipeCatalogueReader() noexcept {
    for (std::size_t i = 0; i < recipe_catalog::recipes.size(); ++i) {
      const auto& descriptor = recipe_catalog::recipes[i];
      if (!recipe_catalog::structurally_valid(descriptor) ||
          descriptor.targets.count > items_.size()) return;
      profiles_[i] = recipe_runtime::assemble(descriptor);
      for (const auto& target : descriptor.targets) {
        bool found = false;
        for (std::size_t j = 0; j < target_count_; ++j) {
          const bool same_symbol = target.symbol == targets_[j].symbol;
          const bool same_name = std::strcmp(target.name, targets_[j].name) == 0;
          if (same_symbol != same_name) return;
          if (same_symbol) { found = true; break; }
        }
        if (!found) {
          if (target_count_ == targets_.size()) return;
          targets_[target_count_++] = target;
        }
      }
    }
    std::array<std::size_t, recipe_catalogue_target_capacity> lengths{};
    catalogue_valid_ = target_count_ && packed_detail::validate_targets(targets_.data(), target_count_, lengths);
  }
  RecipeCatalogueReader(const RecipeCatalogueReader&) = delete;
  RecipeCatalogueReader& operator=(const RecipeCatalogueReader&) = delete;
  RecipeCatalogueReader(RecipeCatalogueReader&&) = delete;
  RecipeCatalogueReader& operator=(RecipeCatalogueReader&&) = delete;

  RecipeCatalogueCapture capture(const ReadOnlyMemory& memory, std::uint32_t parser,
      const StationKey& key, bool stable_boundary_verified,
      const std::array<bool, recipe_catalog::recipes.size()>& hand_contract_verified = {}) noexcept {
    RecipeCatalogueCapture result;
    result.snapshot.key = key;
    auto& diagnostic = result.diagnostic;
    diagnostic.target_count = target_count_;
    for (std::size_t i = 0; i < recipe_catalog::recipes.size(); ++i) {
      diagnostic.recipes[i].applicable = recipe_catalog::recipes[i].station == key.kind;
      diagnostic.recipes[i].assembly_issue = profiles_[i].issue();
    }
    if (capturing_) { diagnostic.issue = RecipeCatalogueIssue::reentry; return result; }
    if (!catalogue_valid_) { diagnostic.issue = RecipeCatalogueIssue::catalogue; return result; }
    if (!recipe_session_detail::valid_key(key)) { diagnostic.issue = RecipeCatalogueIssue::context; return result; }
    capturing_ = true;
    struct CaptureScope {
      bool& active;
      ~CaptureScope() { active = false; }
    } scope{capturing_};

    // Exactly ONE complete inventory reader and ONE hand reader per
    // capture. The inventory reader has its own header/byte comparison runs.
    const auto inventory = read_packed_inventory<recipe_catalogue_target_capacity>(
        memory, key.hero, targets_.data(), target_count_, stable_boundary_verified, workspace_);
    result.hands = read_hands(memory, key.hero);
    diagnostic.inventory_issue = inventory.issue;
    diagnostic.inventory_known = inventory.storage_totals_known;
    diagnostic.listed_known = inventory.listed_known;
    diagnostic.packed_known = inventory.packed_known;
    diagnostic.snapshot_unchanged = inventory.snapshot_unchanged;
    diagnostic.hands_read_valid = result.hands.left.state >= 0 && result.hands.right.state >= 0;
    diagnostic.listed_nodes = inventory.listed_nodes;
    diagnostic.packed_entries = inventory.packed_entries;

    // An unmodified intermediate state alone confirms no quiet point.
    // Even knowledge and script profiles are only read at the explicitly
    // evidenced point here; old results are never returned as a substitute.
    if (!stable_boundary_verified) { diagnostic.issue = RecipeCatalogueIssue::boundary; return result; }

    std::size_t listed_count = 0;
    if (inventory.listed_known && inventory.snapshot_unchanged &&
        inventory.listed_nodes <= listed_objects_.size()) {
      listed_count = inventory.listed_nodes;
      for (std::size_t i = 0; i < listed_count; ++i)
        listed_objects_[i] = station_detail::little_u32(workspace_.records[i].node.data() + 4);
    }
    RecipeInventory projected;
    projected.header_known = inventory.header_known;
    projected.listed_known = inventory.listed_known;
    projected.packed_known = inventory.packed_known;
    projected.storage_totals_known = inventory.storage_totals_known;
    projected.stable_boundary_verified = inventory.stable_boundary_verified;
    projected.snapshot_unchanged = inventory.snapshot_unchanged;
    projected.issue = inventory.issue;
    projected.listed_nodes = inventory.listed_nodes;
    projected.packed_entries = inventory.packed_entries;
    projected.listed_objects = {listed_objects_.data(), listed_count};

    for (std::size_t i = 0; i < recipe_catalog::recipes.size(); ++i) {
      auto& detail = diagnostic.recipes[i];
      if (!detail.applicable) continue;
      const auto& descriptor = recipe_catalog::recipes[i];
      auto& state = result.snapshot.recipes[i];
      detail.evaluated = true;
      detail.profile = recipe_runtime::verify(memory, parser, profiles_[i]);
      detail.knowledge = recipe_runtime::knowledge(memory, parser, profiles_[i]);
      state.known = detail.knowledge.read_valid && detail.knowledge.known;
      state.contract = detail.profile.matches;
      bool mapped = true;
      for (std::size_t j = 0; j < descriptor.targets.count; ++j) {
        const auto index = target_index(descriptor.targets[j]);
        if (index == target_count_) { mapped = false; break; }
        auto& item = items_[j];
        item.symbol = descriptor.targets[j].symbol;
        item.listed = inventory.listed[index]; item.packed = inventory.packed[index]; item.total = inventory.total[index];
        item.listed_occurrences = inventory.listed_occurrences[index];
        item.packed_occurrences = inventory.packed_occurrences[index];
        item.duplicate_packed_target = inventory.duplicate_packed_target[index];
        item.has_active_marker = inventory.has_active_marker[index];
        item.mixed_representation = inventory.mixed_representation[index];
      }
      if (!mapped) { detail.material_issue = RecipeMaterialIssue::target_mapping; continue; }
      projected.items = {items_.data(), descriptor.targets.count};
      const auto material = recipe_material(descriptor, projected, result.hands, 0,
          rows_.data(), rows_.size(), hand_contract_verified[i]);
      detail.material_issue = material.issue;
      state.material_known = material.inventory_known;
      state.hand_known = material.hand_known;
      state.maximum = material.maximum;
      // The quantity 0 is exclusively a stock/preview capture. It starts
      // nothing. Even with missing ingredients known numbers remain
      // representable; an invalid profile is separately locked by contract.
      if (material.inventory_known) for (std::size_t j = 0; j < material.count; ++j) {
        state.materials[j].stored = static_cast<std::int32_t>(rows_[j].stored);
        // The hand contracts supported here deliver only -1, 0, or 1.
        state.materials[j].reserved = static_cast<std::int32_t>(rows_[j].reserved);
      }
    }
    return result;
  }

private:
  std::size_t target_index(const PackedInventoryTarget& target) const noexcept {
    for (std::size_t i = 0; i < target_count_; ++i)
      if (targets_[i].symbol == target.symbol && std::strcmp(targets_[i].name, target.name) == 0) return i;
    return target_count_;
  }

  PackedInventoryWorkspace workspace_{};
  std::array<recipe_runtime::ProfileAssembly, recipe_catalog::recipes.size()> profiles_{};
  std::array<PackedInventoryTarget, recipe_catalogue_target_capacity> targets_{};
  std::array<std::uint32_t, max_packed_inventory_nodes> listed_objects_{};
  std::array<RecipeInventoryItem, recipe_catalog::catalog_target_capacity> items_{};
  std::array<RecipeMaterialRow, recipe_catalog::catalog_target_capacity> rows_{};
  std::size_t target_count_ = 0;
  bool catalogue_valid_ = false, capturing_ = false;
};

} // namespace ubc
