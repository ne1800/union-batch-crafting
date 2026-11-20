// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/bow/bow_item_renderer.hpp"

namespace ubc::bow_item_runtime {
namespace path = bow_item_path;
namespace native = bow_item_native;
namespace code = bow_item_code;
namespace renderer = bow_item_renderer;
namespace cat = bow_recipe_catalog;
namespace dep = alchemy_extra_dependencies;
inline constexpr unsigned profile_version = 1;
enum class Issue {
  none,
  boundary,
  thread,
  reentry,
  modules,
  inventory,
  externals,
  constructors,
  native_code,
  renderer_path,
  pool,
  sorter,
  world,
  whole_stack,
  changed
};
struct Input {
  path::Boundary boundary{};
  // Own file/module leases held throughout the entire call. Their
  // origin is WindowsModuleIdentity; no filename assumption.
  dep::Modules modules{};
  renderer::Module renderer{}, locator{};
};
struct Evidence {
  bool item_path_verified = false;
  Issue issue = Issue::none;
  std::uint32_t failed_address = 0, failed_symbol = 0;
  path::Snapshot stock{};
  code::ModuleEvidence modules{};
  native::ConstructorEvidence constructors{};
  native::PoolPath pool{};
  native::SortEvidence sort{};
  native::WorldListEvidence world{};
  renderer::ForwardEvidence renderer{};
  std::array<std::int32_t, cat::catalog_target_capacity> expected_totals{};
};
namespace detail {
inline bool output_route(const ReadOnlyMemory& memory, std::uint32_t game) noexcept {
  // CREATEINVITEMS 6E7C57 skips the entire output when game+28 != 0.
  // Insert reaches World::RemoveVob through the actual +34 vtable slot.
  std::uint32_t blocked = 0, target = 0;
  for (unsigned pass = 0; pass < 2; ++pass)
    if (!native::detail::word(memory, game, 0x28, blocked) || blocked ||
        !native::detail::word(memory, 0x83e174, 0x34, target) || target != 0x7800c0)
      return false;
  return true;
}
inline bool separate_modules(const Input& in) noexcept {
  using zbug_end_compat::detail::overlaps;
  if (in.locator.mapping_verified &&
      overlaps(in.locator.base, in.locator.image_size, in.renderer.base, in.renderer.image_size))
    return false;
  for (const auto* m : {&in.modules.zbug, &in.modules.union_core, &in.modules.parser_extender,
                        &in.modules.runtime_crt}) {
    if (overlaps(m->base, m->image_size, in.renderer.base, in.renderer.image_size) ||
        (in.locator.mapping_verified &&
         overlaps(m->base, m->image_size, in.locator.base, in.locator.image_size)))
      return false;
  }
  return true;
}
inline bool same_module_evidence(const code::ModuleEvidence& a,
                                 const code::ModuleEvidence& b) noexcept {
  return a.observed && b.observed && a.objects.factory == b.objects.factory &&
         a.code.zbug_unpack_call == b.code.zbug_unpack_call &&
         a.code.zpe_init_call == b.code.zpe_init_call &&
         a.code.zpe_error_call == b.code.zpe_error_call &&
         a.code.ikarus_handler == b.code.ikarus_handler &&
         dep::same(a.dispatch.item, b.dispatch.item) &&
         a.dispatch.allocator_entry == b.dispatch.allocator_entry &&
         a.dispatch.allocator_forward == b.dispatch.allocator_forward &&
         a.dispatch.wrapper_record == b.dispatch.wrapper_record &&
         a.dispatch.engine_version_entry == b.dispatch.engine_version_entry;
}
inline bool whole_paths(const path::Snapshot& s, const native::WorldListEvidence& world) noexcept {
  if (!world.stable)
    return false;
  for (std::size_t i = 0; i < s.removals.size(); ++i) {
    const auto& step = s.removals[i];
    if (step.path == path::Removal::whole) {
      if (s.contents != s.boundary.key.hero + 0x710u || s.disabled || s.pool_debug ||
          s.selected >= step.nodes_after)
        return false;
      if (step.source == path::Source::listed && !world.current_whole_refcount_safe[i])
        return false;
      // Pure pack: the pinned ZBug path creates a new, listed inactive
      // MultiSlot object. The two original world list entries keep
      // it alive at full removal. No future pointer is invented;
      // allocator/Factory/Insert paths are bound separately.
      if (step.source != path::Source::listed && step.source != path::Source::packed)
        return false;
    } else if (step.path != path::Removal::split)
      return false;
  }
  return true;
}
}
// Persistent workspace, do not construct on the engine stack. A positive
// finding applies only to this own serialized stable/late point. It
// replaces neither Recipe/Knowledge/Ticket nor Normal-end/Restore evidence.
// Known renderer/CRT paths remain original behavior; after execution all
// expected_totals and the normal hand/station termination must be re-read.
class Reader {
  std::uint32_t owner_ = 0;
  bool reading_ = false;
  path::Reader inventory_;

public:
  explicit Reader(std::uint32_t owner) noexcept : owner_(owner), inventory_(owner) {}
  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;
  Evidence capture(const ReadOnlyMemory& memory, const cat::Descriptor& descriptor,
                   std::uint32_t quantity, const Input& in) noexcept {
    Evidence out;
    if (!owner_ || in.boundary.current_thread != owner_ || in.boundary.key.thread != owner_) {
      out.issue = Issue::thread;
      return out;
    }
    if (reading_) {
      out.issue = Issue::reentry;
      return out;
    }
    if (!path::detail::boundary(in.boundary) || !cat::structurally_valid(descriptor) ||
        !detail::separate_modules(in)) {
      out.issue = Issue::boundary;
      return out;
    }
    reading_ = true;
    struct Lease {
      bool& value;
      ~Lease() { value = false; }
    } lease{reading_};
    std::uint32_t game = 0;
    if (!native::detail::word(memory, selection_abi::game_pointer, 0, game) ||
        !detail::output_route(memory, game)) {
      out.issue = Issue::boundary;
      return out;
    }
    const auto& key = in.boundary.key;
    const alchemy_flask_debit::Context context{
        key.generation, key.thread, lhiver::game_parser, game, key.world, key.hero, key.station};
    out.modules = code::observe_modules(memory, in.modules, context);
    if (!out.modules.observed) {
      out.issue = Issue::modules;
      out.failed_address = out.modules.code.address;
      return out;
    }
    out.stock = inventory_.capture(memory, descriptor, quantity, in.boundary);
    if (!out.stock.captured) {
      out.issue = Issue::inventory;
      return out;
    }
    const auto external = code::verify_externals(memory, lhiver::game_parser);
    if (!external.verified) {
      out.issue = Issue::externals;
      out.failed_address = external.address;
      out.failed_symbol = external.symbol;
      return out;
    }
    out.constructors = native::verify_constructor_values(memory, descriptor);
    if (!out.constructors.values_verified) {
      out.issue = Issue::constructors;
      out.failed_symbol = out.constructors.symbol;
      return out;
    }
    const auto engine = native::verify_engine_parts(memory);
    if (!engine.verified) {
      out.issue = Issue::native_code;
      out.failed_address = engine.address;
      return out;
    }
    native::BowDispatch dispatch{in.locator};
    const auto route = native::verify_routed_parts(memory, in.modules, out.stock, &dispatch);
    if (!route.verified) {
      out.issue = Issue::native_code;
      out.failed_address = route.address;
      return out;
    }
    out.renderer = renderer::verify_forwarding(memory, in.renderer);
    if (!out.renderer.verified) {
      out.issue = Issue::renderer_path;
      return out;
    }
    out.pool = native::verify_pool_path(memory, in.modules, out.stock);
    if (!out.pool.verified) {
      out.issue = Issue::pool;
      return out;
    }
    const auto free_nodes = native::read_pool(memory, out.stock);
    if (!free_nodes.stable || free_nodes.fingerprint != out.pool.fingerprint) {
      out.issue = Issue::changed;
      return out;
    }
    out.sort = native::read_sort_inputs(memory, descriptor, out.stock);
    if (!out.sort.stable) {
      out.issue = Issue::sorter;
      return out;
    }
    out.world = native::read_world_lists(memory, out.stock, free_nodes);
    if (!out.world.stable) {
      out.issue = Issue::world;
      return out;
    }
    if (!detail::whole_paths(out.stock, out.world)) {
      out.issue = Issue::whole_stack;
      return out;
    }
    // The native call comes only after return. An intermediate mutation
    // will not compensate or release masked by old material values.
    const auto late = inventory_.capture(memory, descriptor, quantity, in.boundary);
    const auto constructor_after = native::verify_constructor_values(memory, descriptor);
    const auto sort_after = native::read_sort_inputs(memory, descriptor, late);
    const auto pool_after = native::read_pool(memory, late);
    const auto world_after = native::read_world_lists(memory, late, pool_after);
    const auto modules_after = code::observe_modules(memory, in.modules, context);
    const auto renderer_after = renderer::verify_forwarding(memory, in.renderer);
    if (!path::same_capture(out.stock, late) || !constructor_after.values_verified ||
        constructor_after.fingerprint != out.constructors.fingerprint || !sort_after.stable ||
        sort_after.fingerprint != out.sort.fingerprint || !pool_after.stable ||
        pool_after.fingerprint != out.pool.fingerprint || !world_after.stable ||
        world_after.fingerprint != out.world.fingerprint ||
        !detail::same_module_evidence(out.modules, modules_after) ||
        !renderer::same(out.renderer, renderer_after) || !detail::output_route(memory, game) ||
        !code::verify_externals(memory, lhiver::game_parser).verified ||
        !native::verify_engine_parts(memory).verified) {
      out.issue = Issue::changed;
      return out;
    }
    const auto late_route = native::verify_routed_parts(memory, in.modules, late, &dispatch);
    if (!late_route.verified) {
      out.issue = Issue::changed;
      out.failed_address = late_route.address;
      return out;
    }
    for (std::size_t i = 0; i < descriptor.targets.count; ++i) {
      const auto& row = out.stock.materials[i];
      auto expected = std::int64_t{row.total};
      for (const auto& removal : out.stock.plan.removals)
        if (row.symbol == removal.symbol)
          expected -= removal.amount;
      if (row.symbol == out.stock.plan.output.symbol)
        expected += out.stock.plan.output.amount;
      if (expected < 0 || expected > INT32_MAX) {
        out.issue = Issue::inventory;
        return out;
      }
      out.expected_totals[i] = static_cast<std::int32_t>(expected);
    }
    out.item_path_verified = true;
    return out;
  }
};
} // namespace ubc::bow_item_runtime
