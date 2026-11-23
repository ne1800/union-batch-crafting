// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/stations/grind/grind_item_binding.hpp"
#include "ubc/stations/bow/bow_item_path.hpp"

namespace ubc::grind_item_path {
namespace rt = grind_recipe_runtime;
namespace shared = bow_item_path::detail;
namespace hands = station_recipe_snapshot_detail;
namespace native = alchemy_flask_runtime;
enum class Issue {
  binding,
  boundary,
  thread,
  reentry,
  quantity,
  context,
  profile,
  native_code,
  fields,
  inventory,
  hands,
  object,
  alias,
  changed,
  none
};
enum class Scope { writer, preview };
struct Boundary {
  GrindRecipeKey key{};
  std::uint32_t current_thread = 0;
  bool stable_boundary_verified = false;
};
struct Snapshot {
  Issue issue = Issue::binding;
  Scope scope = Scope::writer;
  GrindInputObjectBinding input{};
  std::array<RecipeInventoryItem, 3> materials{}; // Input, weapon output, 16613
  std::array<shared::Fields, 2> outputs{};
  std::array<bool, 2> output_slot_alias{};
  std::uint32_t quantity = 0, refcount = 0, amount_after = 0, listed_nodes = 0, nodes_after = 0,
                selected = 0, contents = 0, disabled = 0, pool_debug = 0, failed_address = 0;
  bow_item_path::Removal removal = bow_item_path::Removal::unknown;
  // The existing conservative full-stack pre-evidence from bow_item_path. No
  // native release: Factory/World/Module and post-observation are absent here.
  bool existing_whole_short_path = false;
  lhiver::Digest fingerprint{};
  bool captured() const noexcept { return issue == Issue::none; }
  static constexpr bool native_effect_authorized = false;
};

namespace detail {
inline bool same_stock(const RecipeInventoryItem& a, const RecipeInventoryItem& b) noexcept {
  return a.symbol == b.symbol && a.listed == b.listed && a.packed == b.packed &&
         a.total == b.total && a.listed_occurrences == b.listed_occurrences &&
         a.packed_occurrences == b.packed_occurrences &&
         a.duplicate_packed_target == b.duplicate_packed_target &&
         a.has_active_marker == b.has_active_marker &&
         a.mixed_representation == b.mixed_representation;
}
// Reuse the original full-body. No generic weapon Boolean and no
// category change: all 22 canonical input forms are category 2.
inline constexpr std::array<native::CodeRange, 3> predicate_ranges{
    {native::code_ranges[3], native::code_ranges[4], native::code_ranges[12]}};
static_assert(predicate_ranges[0].address == 0x7125a0 && predicate_ranges[0].length == 99 &&
                  predicate_ranges[1].address == 0x7126d0 &&
                  predicate_ranges[2].address == 0x711420,
              "vorhandene MultiSlot/HasFlag/GetInstance-Vertraege");
inline bool predicates(const ReadOnlyMemory& memory, Snapshot& out) noexcept {
  for (const auto& range : predicate_ranges)
    if (!native::detail::code_range(memory, range).valid) {
      out.issue = Issue::native_code;
      out.failed_address = range.address;
      return false;
    }
  std::uint32_t instance = 0;
  if (!shared::word(memory, hand_detail::item_vtable, 0x78, instance) || instance != 0x711420 ||
      !grind_recipe_snapshot_detail::item_class(memory)) {
    out.issue = Issue::native_code;
    return false;
  }
  return true;
}
}

// Tight writer pre-finding for already-listed input weapons. No
// new inventory/slot implementation and no hidden HasItems/Engine
// call. SELF is freshly checked; this class is NOT the menu
// preview. Return only own values; do not reuse after an effect.
class Reader {
public:
  explicit Reader(std::uint32_t owner_thread) noexcept : owner_(owner_thread) {}
  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;
  Snapshot capture(const ReadOnlyMemory& memory, const GrindRecipeCatalogueCapture& catalogue,
                   std::uint32_t callback, std::uint32_t quantity,
                   const Boundary& boundary) noexcept {
    return capture_bound(memory, catalogue, callback, quantity, boundary, nullptr);
  }
  Snapshot capture_preview(const ReadOnlyMemory& memory,
                           const GrindRecipeCatalogueCapture& catalogue,
                           const GrindGuildSnapshot& physical, std::uint32_t callback,
                           std::uint32_t quantity, const Boundary& boundary) noexcept {
    return capture_bound(memory, catalogue, callback, quantity, boundary, &physical);
  }

private:
  Snapshot capture_bound(const ReadOnlyMemory& memory, const GrindRecipeCatalogueCapture& catalogue,
                         std::uint32_t callback, std::uint32_t quantity, const Boundary& boundary,
                         const GrindGuildSnapshot* physical) noexcept {
    ReadOrigin origin("inventory");
    Snapshot out;
    out.quantity = quantity;
    out.scope = physical ? Scope::preview : Scope::writer;
    if (!owner_ || boundary.current_thread != owner_ || boundary.key.thread != owner_) {
      out.issue = Issue::thread;
      return out;
    }
    if (!boundary.stable_boundary_verified) {
      out.issue = Issue::boundary;
      return out;
    }
    if (reading_) {
      out.issue = Issue::reentry;
      return out;
    }
    if (!quantity || quantity > INT32_MAX) {
      out.issue = Issue::quantity;
      return out;
    }
    out.input =
        physical ? bind_grind_input_for_preview(catalogue, *physical, boundary.key, callback).values
                 : bind_grind_input_object(catalogue, boundary.key, callback);
    if (!out.input.observed())
      return out;
    if (quantity > static_cast<std::uint32_t>(out.input.stock.listed)) {
      out.issue = Issue::quantity;
      return out;
    }
    const auto hero = boundary.key.hero;
    if (hero > UINT32_MAX - 0x1000u) {
      out.issue = Issue::fields;
      return out;
    }
    reading_ = true;
    struct Lease {
      bool& active;
      ~Lease() { active = false; }
    } lease{reading_};
    const auto* recipe = grind_recipe::find(callback); // Already checked by Binder
    const auto assembly = rt::assemble(*recipe);
    auto context = [&]() noexcept {
      const auto state = rt::state(memory, lhiver::game_parser, assembly);
      if (!rt::same_state(state, catalogue.diagnostic.state_after) ||
          !state.station_postcondition())
        return false;
      if (physical) {
        const auto now = read_grind_guild_for_preview(memory, lhiver::game_parser, boundary.key);
        return now.known() && grind_guild_snapshot_detail::same(now, *physical);
      }
      std::uint32_t current_hero = 0;
      const auto actor = rt::actor(memory, lhiver::game_parser, assembly, hero);
      return actor.matches && actor.requires_self && actor.hero == hero && actor.self == hero &&
             shared::word(memory, station_abi::hero_pointer, 0, current_hero) &&
             current_hero == hero &&
             station_recipe_runtime::same_actor_evidence(actor, catalogue.diagnostic.actor_after);
    };
    if (!context()) {
      out.issue = Issue::context;
      return out;
    }
    if (!rt::verify_recipe(memory, lhiver::game_parser, *recipe).valid) {
      out.issue = Issue::profile;
      return out;
    }
    if (!detail::predicates(memory, out))
      return out;
    const auto inv = hero + 0x668;
    std::array<std::uint32_t, 5> initial{}, final{};
    auto controls = [&](auto& values) noexcept {
      return shared::word(memory, inv, 0, values[0]) && shared::word(memory, inv, 4, values[1]) &&
             shared::word(memory, inv, 0x24, values[2]) &&
             shared::word(memory, inv, 0x98, values[3]) &&
             shared::word(memory, 0x8a2558, 0, values[4]);
    };
    packed_detail::Header before{}, after{};
    if (!controls(initial) || initial[0] != bow_item_path::inventory_vtable ||
        !hand_detail::at_offset(memory, hero, packed_detail::header_offset, before.data(),
                                before.size())) {
      out.issue = Issue::fields;
      return out;
    }
    const auto pack_length = hand_detail::u32(before.data() + 32);
    if (pack_length > max_packed_inventory_bytes) {
      out.issue = Issue::inventory;
      return out;
    }
    const std::array<std::int32_t, 3> symbols{
        {recipe->input, recipe->output, grind_recipe::lifecycle_item}};
    std::array<PackedInventoryTarget, 3> targets{};
    for (std::size_t i = 0; i < targets.size(); ++i)
      targets[i] = {
          grind_recipe_profile::item_for(static_cast<std::uint32_t>(symbols[i]))->function->name,
          symbols[i]};
    const auto stock =
        read_packed_inventory<3>(memory, hero, targets.data(), targets.size(), true, inventory_);
    if (!stock.storage_totals_known || !stock.snapshot_unchanged ||
        stock.issue != PackedInventoryIssue::none ||
        stock.listed_nodes != catalogue.diagnostic.listed_nodes ||
        stock.packed_entries != catalogue.diagnostic.packed_entries ||
        stock.listed_occurrences[0] != 1 || stock.listed[0] != out.input.stock.listed ||
        stock.packed[0] != 0 || stock.total[0] != stock.listed[0] || stock.packed_occurrences[0] ||
        stock.has_active_marker[0] || stock.duplicate_packed_target[0] ||
        stock.mixed_representation[0]) {
      out.issue = Issue::inventory;
      return out;
    }
    for (std::size_t i = 0; i < out.materials.size(); ++i) {
      out.materials[i] = {symbols[i],
                          stock.listed[i],
                          stock.packed[i],
                          stock.total[i],
                          stock.listed_occurrences[i],
                          stock.packed_occurrences[i],
                          stock.duplicate_packed_target[i],
                          stock.has_active_marker[i],
                          stock.mixed_representation[i]};
      unsigned matches = 0;
      for (const auto& old : catalogue.stocks)
        if (old.symbol == symbols[i]) {
          ++matches;
          if (!detail::same_stock(old, out.materials[i])) {
            out.issue = Issue::changed;
            return out;
          }
        }
      if (matches != 1) {
        out.issue = Issue::binding;
        return out;
      }
    }
    if (!hand_detail::at_offset(memory, hero, packed_detail::header_offset, after.data(),
                                after.size()) ||
        before != after) {
      out.issue = Issue::changed;
      return out;
    }
    // GetAmount can also search and materialize on a list hit by NAME. No
    // prefix/suffix collision of the input name in the entire pack string.
    if (stock.pack_ability &&
        !shared::packed_search(inventory_.packed_bytes.data(), pack_length, targets[0].name, 0)) {
      out.issue = Issue::inventory;
      return out;
    }
    if (hands::begin_hands(memory, hero, hands_) != StationHandReadIssue::none ||
        !grind_item_binding_detail::same_hands(hands_.before, out.input.hands)) {
      out.issue = Issue::hands;
      return out;
    }
    Sha256 hash;
    hash.update(before.data(), before.size());
    for (const auto v : initial)
      shared::hash_word(hash, v);
    bool found = false;
    for (std::size_t i = 0; i < stock.listed_nodes; ++i) {
      const auto& row = inventory_.records[i];
      const auto object = hand_detail::u32(row.node.data() + 4);
      auto& f = fields_[i];
      if (!shared::fields(memory, object, f)) {
        out.issue = Issue::fields;
        return out;
      }
      // Native list search also reaches foreign objects/GetInstance slots.
      if (f.vptr != hand_detail::item_vtable || !f.refcount || f.refcount > INT32_MAX ||
          f.amount > INT32_MAX || f.symbol > INT32_MAX) {
        out.issue = Issue::object;
        return out;
      }
      if (f.amount != hand_detail::u32(row.item_fields.data()) ||
          f.symbol != hand_detail::u32(row.item_fields.data() + 4)) {
        out.issue = Issue::changed;
        return out;
      }
      if (object == out.input.item.object) {
        if (found || f.symbol != static_cast<std::uint32_t>(recipe->input) ||
            f.amount != static_cast<std::uint32_t>(stock.listed[0]) ||
            f.mainflag != out.input.item.mainflag || f.flags != out.input.item.flags ||
            f.mainflag != 2 || !(f.flags & 2u) || (f.flags & (0x40000000u | 0x04000000u))) {
          out.issue = Issue::object;
          return out;
        }
        // 7125DA..7125FD: category 2 and inactive yield MultiSlot==1;
        // ITEM_MISSION remains part of the exactly bound form.
        for (std::size_t j = 0; j < hands_.count; ++j)
          if (hand_detail::u32(hands_.records[j].fields.data() + 0x30) == object) {
            out.issue = Issue::alias;
            return out;
          }
        if (object == hands_.before.interact_item) {
          out.issue = Issue::alias;
          return out;
        }
        out.refcount = f.refcount;
        found = true;
      }
      for (std::size_t j = 0; j < out.outputs.size(); ++j)
        if (f.symbol == static_cast<std::uint32_t>(symbols[j + 1])) {
          if (stock.listed_occurrences[j + 1] == 1)
            out.outputs[j] = f;
          for (std::size_t k = 0; k < hands_.count; ++k)
            out.output_slot_alias[j] =
                out.output_slot_alias[j] ||
                hand_detail::u32(hands_.records[k].fields.data() + 0x30) == object;
          out.output_slot_alias[j] =
              out.output_slot_alias[j] || hands_.before.interact_item == object;
        }
      shared::hash_word(hash, row.address);
      hash.update(row.node.data(), row.node.size());
      for (const auto value :
           {f.object, f.vptr, f.refcount, f.mainflag, f.flags, f.amount, f.symbol})
        shared::hash_word(hash, value);
      hash.update(f.damages.data(), f.damages.size());
    }
    if (!found) {
      out.issue = Issue::changed;
      return out;
    }
    hash.update(inventory_.packed_bytes.data(), static_cast<std::size_t>(pack_length) + 1);
    hash.update(hands_.header.data(), hands_.header.size());
    hash.update(hands_.anchor.data(), hands_.anchor.size());
    hash.update(hands_.pointers.data(), hands_.count * 4);
    for (std::size_t i = 0; i < hands_.count; ++i) {
      const auto& slot = hands_.records[i];
      shared::hash_word(hash, slot.address);
      hash.update(slot.fields.data(), slot.fields.size());
      hash.update(slot.name.data(), slot.length + 1);
    }
    for (std::size_t i = 0; i < stock.listed_nodes; ++i) {
      shared::Fields fresh;
      if (!shared::fields(memory, fields_[i].object, fresh) || !(fresh == fields_[i])) {
        out.issue = Issue::changed;
        return out;
      }
    }
    if (!hands::verify_hands(memory, hands_) ||
        packed_detail::compare_list(memory, inventory_, stock.listed_nodes) !=
            PackedInventoryIssue::none ||
        packed_detail::compare_pack(memory, inventory_, hand_detail::u32(before.data() + 28),
                                    pack_length) != PackedInventoryIssue::none ||
        !hand_detail::at_offset(memory, hero, packed_detail::header_offset, after.data(),
                                after.size()) ||
        before != after || !controls(final) || initial != final) {
      out.issue = Issue::changed;
      return out;
    }
    if (!context()) {
      out.issue = Issue::context;
      return out;
    }
    if (!rt::verify_recipe(memory, lhiver::game_parser, *recipe).valid) {
      out.issue = Issue::profile;
      return out;
    }
    if (!detail::predicates(memory, out))
      return out;
    out.contents = initial[1];
    out.selected = initial[2];
    out.disabled = initial[3];
    out.pool_debug = initial[4];
    out.listed_nodes = out.nodes_after = stock.listed_nodes;
    out.amount_after = static_cast<std::uint32_t>(stock.listed[0]) - quantity;
    out.removal = out.amount_after ? bow_item_path::Removal::split : bow_item_path::Removal::whole;
    if (!out.amount_after) {
      --out.nodes_after;
      out.existing_whole_short_path = out.contents == inv + 0xa8 && !out.disabled &&
                                      !out.pool_debug && out.selected < out.nodes_after &&
                                      out.refcount >= 2 && out.refcount <= INT32_MAX - 2u;
    }
    out.fingerprint = hash.finish();
    out.issue = Issue::none;
    return out;
  }

private:
  std::uint32_t owner_ = 0;
  bool reading_ = false;
  PackedInventoryWorkspace inventory_{};
  hands::HandWorkspace hands_{};
  std::array<shared::Fields, max_packed_inventory_nodes> fields_{};
};
} // namespace ubc::grind_item_path
