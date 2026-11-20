// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/station_recipe_material.hpp"
#include "ubc/stations/station_recipe_runtime.hpp"

namespace ubc {

struct StationRecipeKey {
  std::uint64_t generation = 0;
  std::uint32_t thread = 0, world = 0, hero = 0, station = 0, manager = 0;
  station_recipe_catalog::Station kind = station_recipe_catalog::Station::leatherwork;
};
enum class StationRecipeCaptureIssue { none, context, catalogue, reentry, boundary };
enum class StationHandReadIssue { none, header, anchor, slots, name, duplicate, objects, changed };
struct StationRecipeState {
  station_recipe_catalog::RecipeId id = station_recipe_catalog::RecipeId::leather_sheep;
  bool applicable = false, evaluated = false, known = false, contract = false;
  bool actor_known = false, actor_matches = false;
  std::size_t target_count = 0;
  StationRecipeMaterial material{};
  std::array<StationRecipeMaterialRow, station_recipe_catalog::catalog_target_capacity> rows{};
};
struct StationRecipeReadDiagnostic {
  station_recipe_runtime::AssemblyIssue assembly =
      station_recipe_runtime::AssemblyIssue::descriptor;
  runtime_script::RuntimeContract profile{};
  station_recipe_runtime::Knowledge knowledge{};
};
struct StationRecipeCaptureDiagnostic {
  StationRecipeCaptureIssue issue = StationRecipeCaptureIssue::none;
  PackedInventoryIssue inventory_issue = PackedInventoryIssue::none;
  StationHandReadIssue hand_issue = StationHandReadIssue::none;
  bool inventory_known = false, hands_unchanged = false, actor_unchanged = false;
  std::size_t target_count = 0;
  std::uint32_t listed_nodes = 0, packed_entries = 0;
  station_recipe_runtime::ActorEvidence actor_before{}, actor_after{};
  std::array<StationRecipeReadDiagnostic, station_recipe_catalog::recipes.size()> recipes{};
};
struct StationRecipeCatalogueCapture {
  StationRecipeKey key{};
  std::array<StationRecipeState, station_recipe_catalog::recipes.size()> recipes{};
  StationRecipeHands hands{};
  StationRecipeCaptureDiagnostic diagnostic{};
};

namespace station_recipe_snapshot_detail {
constexpr std::size_t target_capacity() noexcept {
  std::size_t result = 0;
  for (const auto& recipe : station_recipe_catalog::recipes)
    result += recipe.targets.count;
  return result;
}
inline bool valid_key(const StationRecipeKey& key) noexcept {
  using station_recipe_catalog::Station;
  return key.generation && key.thread && key.world && key.hero && key.station && key.manager &&
         (key.kind == Station::leatherwork || key.kind == Station::saw ||
          key.kind == Station::forge);
}
struct SlotRecord {
  std::uint32_t address = 0;
  std::array<std::uint8_t, 0x34> fields{};
  std::array<char, hand_detail::name_limit + 1> name{};
  std::uint32_t length = 0;
};
struct HandWorkspace {
  std::array<std::uint8_t, 12> header{};
  std::array<std::uint8_t, 4> anchor{};
  std::array<std::uint8_t, hand_detail::slot_limit * 4> pointers{};
  std::array<SlotRecord, hand_detail::slot_limit> records{};
  StationRecipeHands before{};
  std::uint32_t hero = 0, count = 0;
  bool captured = false;
};
inline bool same_object(const HandObjectSnapshot& a, const HandObjectSnapshot& b) noexcept {
  return a.state == b.state && a.object == b.object && a.symbol == b.symbol && a.amount == b.amount;
}

// A shared handle phase followed by a byte-comparison stage.
// Verified EXE: NPC interactItem+0x968, Slotarray+0x9B0; TNpcSlot
// tmpLevel+0x18, Name-zSTRING+0, Object+0x30. Both named slots are
// positively bound to their object pointers. No getter/engine call.
inline StationHandReadIssue begin_hands(const ReadOnlyMemory& memory, std::uint32_t hero,
                                        HandWorkspace& work) noexcept {
  using hand_detail::u32;
  work.captured = false;
  work.before = {};
  work.hero = hero;
  work.count = 0;
  if (!hand_detail::at_offset(memory, hero, 0x968, work.anchor.data(), work.anchor.size()))
    return StationHandReadIssue::anchor;
  if (!hand_detail::at_offset(memory, hero, 0x9b0, work.header.data(), work.header.size()))
    return StationHandReadIssue::header;
  const auto capacity = u32(work.header.data() + 4);
  work.count = u32(work.header.data() + 8);
  if (work.count > capacity || capacity > hand_detail::slot_limit)
    return StationHandReadIssue::header;
  if (work.count && !memory.copy(u32(work.header.data()), work.pointers.data(), work.count * 4))
    return StationHandReadIssue::slots;
  std::uint32_t left = 0, right = 0;
  for (std::size_t i = 0; i < work.count; ++i) {
    auto& record = work.records[i];
    record = {};
    record.address = u32(work.pointers.data() + i * 4);
    if (!record.address)
      return StationHandReadIssue::slots;
    for (std::size_t j = 0; j < i; ++j)
      if (work.records[j].address == record.address)
        return StationHandReadIssue::duplicate;
    if (!memory.copy(record.address, record.fields.data(), record.fields.size()))
      return StationHandReadIssue::slots;
    record.length = u32(record.fields.data() + 0xc);
    if (!record.length || record.length > hand_detail::name_limit ||
        !memory.copy(u32(record.fields.data() + 8), record.name.data(), record.length + 1) ||
        record.name[record.length] || std::memchr(record.name.data(), 0, record.length))
      return StationHandReadIssue::name;
    for (std::size_t j = 0; j < i; ++j)
      if (record.length == work.records[j].length &&
          std::memcmp(record.name.data(), work.records[j].name.data(), record.length) == 0)
        return StationHandReadIssue::duplicate;
    const auto object = u32(record.fields.data() + 0x30);
    // A hand may also not share any further, differently named slot.
    // Unknown slot names are not evidence of separate object lifetimes.
    if (object)
      for (std::size_t j = 0; j < i; ++j)
        if (u32(work.records[j].fields.data() + 0x30) == object)
          return StationHandReadIssue::duplicate;
    const auto level = hand_detail::i32(record.fields.data() + 0x18);
    if (record.length == 11 && std::memcmp(record.name.data(), "ZS_LEFTHAND", 11) == 0) {
      left = object;
      work.before.left_slot_known = true;
      work.before.left_tmp_level = level;
    }
    if (record.length == 12 && std::memcmp(record.name.data(), "ZS_RIGHTHAND", 12) == 0) {
      right = object;
      work.before.right_slot_known = true;
      work.before.right_tmp_level = level;
    }
  }
  if (left && left == right)
    return StationHandReadIssue::duplicate;
  work.before.hands = {hand_detail::read_object(memory, left),
                       hand_detail::read_object(memory, right)};
  work.before.interact_item_known = true;
  work.before.interact_item = u32(work.anchor.data());
  work.captured = true;
  return work.before.hands.left.state < 0 || work.before.hands.right.state < 0
             ? StationHandReadIssue::objects
             : StationHandReadIssue::none;
}
inline bool verify_hands(const ReadOnlyMemory& memory, const HandWorkspace& work) noexcept {
  if (!work.captured)
    return false;
  std::array<std::uint8_t, 12> header{};
  std::array<std::uint8_t, 4> anchor{};
  if (!hand_detail::at_offset(memory, work.hero, 0x968, anchor.data(), anchor.size()) ||
      anchor != work.anchor ||
      !hand_detail::at_offset(memory, work.hero, 0x9b0, header.data(), header.size()) ||
      header != work.header)
    return false;
  std::array<std::uint8_t, hand_detail::slot_limit * 4> pointers{};
  if (work.count &&
      (!memory.copy(hand_detail::u32(header.data()), pointers.data(), work.count * 4) ||
       std::memcmp(pointers.data(), work.pointers.data(), work.count * 4) != 0))
    return false;
  for (std::size_t i = 0; i < work.count; ++i) {
    const auto& record = work.records[i];
    std::array<std::uint8_t, 0x34> fields{};
    std::array<char, hand_detail::name_limit + 1> name{};
    if (!memory.copy(record.address, fields.data(), fields.size()) || fields != record.fields ||
        !memory.copy(hand_detail::u32(fields.data() + 8), name.data(), record.length + 1) ||
        std::memcmp(name.data(), record.name.data(), record.length + 1) != 0)
      return false;
  }
  return same_object(work.before.hands.left,
                     hand_detail::read_object(memory, work.before.hands.left.object)) &&
         same_object(work.before.hands.right,
                     hand_detail::read_object(memory, work.before.hands.right.object));
}
} // namespace station_recipe_snapshot_detail

inline constexpr std::size_t station_recipe_target_capacity =
    station_recipe_snapshot_detail::target_capacity();

// Persistent own game-thread workspace. No concurrent use, no heap
// allocation, no engine calls. capture holds only value copies.
// Before/after capture the complete native world/station/thread identity
// remains the adapter's task. Two read passes do not replace this true
// idle boundary and assert no atomicity against foreign async writes.
class StationRecipeCatalogueReader {
public:
  StationRecipeCatalogueReader() noexcept {
    for (std::size_t i = 0; i < station_recipe_catalog::recipes.size(); ++i) {
      const auto& recipe = station_recipe_catalog::recipes[i];
      if (!station_recipe_catalog::structurally_valid(recipe))
        return;
      profiles_[i] = station_recipe_runtime::assemble(recipe);
      for (const auto& target : recipe.targets) {
        bool found = false;
        for (std::size_t j = 0; j < count_; ++j) {
          const bool symbol = target.symbol == targets_[j].symbol;
          const bool name = std::strcmp(target.name, targets_[j].name) == 0;
          if (symbol != name)
            return;
          if (symbol) {
            found = true;
            break;
          }
        }
        if (!found) {
          if (count_ == targets_.size())
            return;
          targets_[count_++] = target;
        }
      }
    }
    std::array<std::size_t, station_recipe_target_capacity> lengths{};
    valid_ = count_ && packed_detail::validate_targets(targets_.data(), count_, lengths);
  }
  StationRecipeCatalogueReader(const StationRecipeCatalogueReader&) = delete;
  StationRecipeCatalogueReader& operator=(const StationRecipeCatalogueReader&) = delete;
  StationRecipeCatalogueReader(StationRecipeCatalogueReader&&) = delete;
  StationRecipeCatalogueReader& operator=(StationRecipeCatalogueReader&&) = delete;

  StationRecipeCatalogueCapture
  capture(const ReadOnlyMemory& memory, std::uint32_t parser, const StationRecipeKey& key,
          bool stable_boundary_verified,
          const std::array<bool, station_recipe_catalog::recipes.size()>& hand_contract_verified =
              {}) noexcept {
    namespace cat = station_recipe_catalog;
    namespace rt = station_recipe_runtime;
    namespace detail = station_recipe_snapshot_detail;
    StationRecipeCatalogueCapture result;
    result.key = key;
    result.diagnostic.target_count = count_;
    auto& diagnostic = result.diagnostic;
    std::size_t first = cat::recipes.size();
    for (std::size_t i = 0; i < cat::recipes.size(); ++i) {
      auto& state = result.recipes[i];
      const auto& recipe = cat::recipes[i];
      state.id = recipe.id;
      state.target_count = recipe.targets.count;
      state.material.completion = recipe.completion;
      diagnostic.recipes[i].assembly = profiles_[i].issue();
      state.applicable = recipe.station == key.kind;
      if (state.applicable && first == cat::recipes.size())
        first = i;
      for (std::size_t j = 0; j < recipe.targets.count; ++j) {
        state.rows[j].symbol = recipe.targets[j].symbol;
        state.rows[j].unit_cost =
            recipe_material_detail::units(recipe.costs, recipe.targets[j].symbol);
        state.rows[j].unit_output =
            recipe_material_detail::units(recipe.outputs, recipe.targets[j].symbol);
      }
    }
    if (capturing_) {
      diagnostic.issue = StationRecipeCaptureIssue::reentry;
      return result;
    }
    if (!valid_) {
      diagnostic.issue = StationRecipeCaptureIssue::catalogue;
      return result;
    }
    if (!detail::valid_key(key) || first == cat::recipes.size()) {
      diagnostic.issue = StationRecipeCaptureIssue::context;
      return result;
    }
    capturing_ = true;
    struct Scope {
      bool& flag;
      ~Scope() { flag = false; }
    } scope{capturing_};
    if (stable_boundary_verified)
      diagnostic.actor_before = rt::actor(memory, parser, profiles_[first], key.hero);
    const auto inventory = read_packed_inventory<station_recipe_target_capacity>(
        memory, key.hero, targets_.data(), count_, stable_boundary_verified, inventory_work_);
    diagnostic.inventory_known = inventory.storage_totals_known;
    diagnostic.inventory_issue = inventory.issue;
    diagnostic.listed_nodes = inventory.listed_nodes;
    diagnostic.packed_entries = inventory.packed_entries;
    diagnostic.hand_issue = detail::begin_hands(memory, key.hero, hand_work_);
    if (!stable_boundary_verified) {
      diagnostic.issue = StationRecipeCaptureIssue::boundary;
      return result;
    }
    for (std::size_t i = 0; i < cat::recipes.size(); ++i)
      if (result.recipes[i].applicable) {
        auto& state = result.recipes[i];
        auto& d = diagnostic.recipes[i];
        state.evaluated = true;
        d.profile = rt::verify(memory, parser, profiles_[i]);
        d.knowledge = rt::knowledge(memory, parser, profiles_[i]);
        state.known = d.knowledge.read_valid && d.knowledge.known;
        state.contract = d.profile.matches;
      }
    diagnostic.actor_after = rt::actor(memory, parser, profiles_[first], key.hero);
    diagnostic.actor_unchanged =
        rt::same_actor_evidence(diagnostic.actor_before, diagnostic.actor_after);
    diagnostic.hands_unchanged = detail::verify_hands(memory, hand_work_);
    if (hand_work_.captured && !diagnostic.hands_unchanged)
      diagnostic.hand_issue = StationHandReadIssue::changed;
    if (diagnostic.hands_unchanged)
      result.hands = hand_work_.before;

    std::size_t objects = 0;
    if (inventory.listed_known && inventory.snapshot_unchanged &&
        inventory.listed_nodes <= listed_objects_.size()) {
      objects = inventory.listed_nodes;
      for (std::size_t i = 0; i < objects; ++i)
        listed_objects_[i] = station_detail::little_u32(inventory_work_.records[i].node.data() + 4);
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
    projected.listed_objects = {listed_objects_.data(), objects};
    for (std::size_t i = 0; i < cat::recipes.size(); ++i)
      if (result.recipes[i].applicable) {
        const auto& recipe = cat::recipes[i];
        auto& state = result.recipes[i];
        state.actor_known = diagnostic.actor_unchanged &&
                            profiles_[i].material_actor() == profiles_[first].material_actor();
        state.actor_matches =
            state.actor_known && diagnostic.actor_before.matches && diagnostic.actor_after.matches;
        bool mapped = true;
        for (std::size_t j = 0; j < recipe.targets.count; ++j) {
          std::size_t index = count_;
          for (std::size_t k = 0; k < count_; ++k)
            if (targets_[k].symbol == recipe.targets[j].symbol) {
              index = k;
              break;
            }
          if (index == count_) {
            mapped = false;
            break;
          }
          items_[j] = {recipe.targets[j].symbol,
                       inventory.listed[index],
                       inventory.packed[index],
                       inventory.total[index],
                       inventory.listed_occurrences[index],
                       inventory.packed_occurrences[index],
                       inventory.duplicate_packed_target[index],
                       inventory.has_active_marker[index],
                       inventory.mixed_representation[index]};
        }
        if (!mapped) {
          state.material.issue = StationRecipeMaterialIssue::target_mapping;
          continue;
        }
        projected.items = {items_.data(), recipe.targets.count};
        state.material = station_recipe_material(
            recipe, projected, result.hands, 0, state.rows.data(), state.rows.size(),
            hand_contract_verified[i] && diagnostic.hands_unchanged);
      }
    return result;
  }

private:
  PackedInventoryWorkspace inventory_work_{};
  station_recipe_snapshot_detail::HandWorkspace hand_work_{};
  std::array<station_recipe_runtime::ProfileAssembly, station_recipe_catalog::recipes.size()>
      profiles_{};
  std::array<PackedInventoryTarget, station_recipe_target_capacity> targets_{};
  std::array<std::uint32_t, max_packed_inventory_nodes> listed_objects_{};
  std::array<RecipeInventoryItem, station_recipe_catalog::catalog_target_capacity> items_{};
  std::size_t count_ = 0;
  bool valid_ = false, capturing_ = false;
};
} // namespace ubc
