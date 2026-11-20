// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/grind_recipe_snapshot.hpp"

namespace ubc {
enum class GrindGuildIssue { key, range, native_code, parser, binding, instance, unreadable, changed, actor, none };

// Concrete guild value of the physical hero, no release of a recipe. Own
// value copies; symbol address and native instance address remain separate.
struct GrindGuildSnapshot {
  GrindGuildIssue issue = GrindGuildIssue::key;
  GrindRecipeKey key{};
  std::uint32_t hero = 0, guild_address = 0, failed_address = 0, failed_symbol = 0;
  std::int32_t guild = 0;
  runtime_script::detail::Layout layout{};
  runtime_script::detail::Symbol npc_class{}, hero_symbol{}, guild_symbol{};
  bool known() const noexcept { return issue == GrindGuildIssue::none; }
  static constexpr bool native_effect_authorized = false;
};
struct GrindWriterGuildSnapshot {
  GrindGuildIssue issue = GrindGuildIssue::key;
  GrindGuildSnapshot physical{};
  grind_recipe_runtime::ActorEvidence actor_before{}, actor_after{};
  bool known() const noexcept { return issue == GrindGuildIssue::none && physical.known(); }
  static constexpr bool native_effect_authorized = false;
};

namespace grind_guild_snapshot_detail {
namespace rs = runtime_script;
namespace rt = grind_recipe_runtime;
namespace shared = station_recipe_runtime::detail;
namespace native = alchemy_flask_runtime;
inline constexpr std::uint32_t guild_symbol = 1549, guild_offset = 0x230, null_instance_int = 0xab6438;
// REA/Ghidra12.2, EXE0655b481...: GetDataAdr yields for Member-INT
// instance_adr + Symbol.offset +4*index. SetUseInstance takes exactly
// InstanceSymbol.offset(+0x1c); no additional NPC head correction.
inline constexpr std::array<native::CodeRange, 2> native_ranges{{
  {0x7a1a40,724,lhiver::digest_literal("346405004722d67e3462e048a978bfed3b9b2cf09f54a470ecb6631d01cbb013"),native::RangeKind::complete_function},
  native::code_ranges[6]
}};
static_assert(native_ranges[1].address == 0x7a1550 && native_ranges[1].length == 284, "bestehender SetUseInstance-Vertrag");
inline bool native_code(const ReadOnlyMemory& memory, GrindGuildSnapshot& out) noexcept {
  for (const auto& range : native_ranges) {
    const auto checked = native::detail::code_range(memory, range);
    if (!checked.valid) { out.issue = GrindGuildIssue::native_code; out.failed_address = range.address; return false; }
  }
  return true;
}
// Use the existing binding checker and compare its concrete symbol
// copies before/after. No second parser or generic member getter.
inline bool bound_symbol(const ReadOnlyMemory& memory, const rs::detail::Layout& layout,
    const lhiver::SymbolBinding& binding, rs::detail::Symbol& out) noexcept {
  rs::detail::Symbol after{};
  return rs::detail::lookup(memory, layout, binding.symbol, out) == rs::Issue::none &&
    rs::detail::check_binding(memory, layout, binding) == rs::Issue::none &&
    rs::detail::lookup(memory, layout, binding.symbol, after) == rs::Issue::none && shared::same_symbol(out, after);
}
inline GrindGuildSnapshot once(const ReadOnlyMemory& memory, std::uint32_t parser,
    const GrindRecipeKey& key, const rt::ProfileAssembly& assembly) noexcept {
  GrindGuildSnapshot out; out.key = key;
  const auto profile = assembly.profile();
  if (rs::detail::read_layout(memory, parser, profile, out.layout) != rs::Issue::none) {
    out.issue = GrindGuildIssue::parser; return out;
  }
  const auto* npc = assembly.binding(1537);
  const auto* guild = assembly.binding(guild_symbol);
  if (!npc || !bound_symbol(memory, out.layout, *npc, out.npc_class)) {
    out.issue = GrindGuildIssue::binding; out.failed_symbol = 1537; return out;
  }
  if (shared::instance(memory, out.layout, profile, 1683, out.hero_symbol) != rs::Issue::none) {
    out.issue = GrindGuildIssue::binding; out.failed_symbol = 1683; return out;
  }
  if (!guild || guild->type != 2 || guild->flags != 4 || guild->elements != 1 ||
      guild->parent_index != 1537 || !guild->check_offset || guild->offset != static_cast<std::int32_t>(guild_offset) ||
      !bound_symbol(memory, out.layout, *guild, out.guild_symbol)) {
    out.issue = GrindGuildIssue::binding; out.failed_symbol = guild_symbol; return out;
  }
  if (!native::detail::word(memory, station_abi::hero_pointer, 0, out.hero)) {
    out.issue = GrindGuildIssue::unreadable; out.failed_address = station_abi::hero_pointer; return out;
  }
  if (!out.hero || out.hero != key.hero || out.hero_symbol.offset != out.hero ||
      out.hero == out.hero_symbol.address || out.hero == out.guild_symbol.address || out.hero == null_instance_int) {
    out.issue = GrindGuildIssue::instance; return out;
  }
  const auto address = std::uint64_t{out.hero} + out.guild_symbol.offset;
  if (out.hero % 4 || !rs::detail::range(address, 4) || address == null_instance_int) {
    out.issue = GrindGuildIssue::range; return out;
  }
  out.guild_address = static_cast<std::uint32_t>(address);
  std::uint32_t value = 0;
  if (!native::detail::word(memory, out.guild_address, 0, value)) {
    out.issue = GrindGuildIssue::unreadable; out.failed_address = out.guild_address; return out;
  }
  out.guild = shared::signed_value(value);
  if (rs::detail::stable_layout(memory, parser, profile, out.layout) != rs::Issue::none) {
    out.issue = GrindGuildIssue::changed; return out;
  }
  out.issue = GrindGuildIssue::none;
  return out;
}
inline bool same(const GrindGuildSnapshot& a, const GrindGuildSnapshot& b) noexcept {
  return a.known() && b.known() && grind_recipe_snapshot_detail::same_key(a.key, b.key) &&
    a.hero == b.hero && a.guild_address == b.guild_address && a.guild == b.guild &&
    rs::detail::same_layout(a.layout, b.layout) && shared::same_symbol(a.npc_class, b.npc_class) &&
    shared::same_symbol(a.hero_symbol, b.hero_symbol) && shared::same_symbol(a.guild_symbol, b.guild_symbol);
}
}

// Preview: only the physical player and HERO. SELF, UseInstance and
// their current dummy are neither prerequisite nor source of this value.
// Read on the idle game thread; the double capture is not an ABA lock.
inline GrindGuildSnapshot read_grind_guild_for_preview(const ReadOnlyMemory& memory,
    std::uint32_t parser, const GrindRecipeKey& key) noexcept {
  namespace detail = grind_guild_snapshot_detail;
  GrindGuildSnapshot out; out.key = key;
  if (!grind_recipe_snapshot_detail::valid_key(key) || parser != lhiver::game_parser) return out;
  if (key.hero % 4 || !runtime_script::detail::range(std::uint64_t{key.hero} + detail::guild_offset, 4)) {
    out.issue = GrindGuildIssue::range; return out;
  }
  if (!detail::native_code(memory, out)) return out;
  const auto assembly = grind_recipe_runtime::assemble(grind_recipe::recipes.front());
  out = detail::once(memory, parser, key, assembly);
  if (!out.known()) return out;
  const auto after = detail::once(memory, parser, key, assembly);
  if (!after.known()) return after;
  if (!detail::same(out, after)) { out.issue = GrindGuildIssue::changed; return out; }
  if (!detail::native_code(memory, out)) return out;
  return out;
}

// Writer: the existing active SELF==HERO check encompasses the own
// Guild snapshot. Do not use for menu previews or loosen later.
inline GrindWriterGuildSnapshot read_grind_guild_for_writer(const ReadOnlyMemory& memory,
    std::uint32_t parser, const GrindRecipeKey& key) noexcept {
  namespace rt = grind_recipe_runtime;
  GrindWriterGuildSnapshot out;
  if (!grind_recipe_snapshot_detail::valid_key(key) || parser != lhiver::game_parser) return out;
  const auto assembly = rt::assemble(grind_recipe::recipes.front());
  out.actor_before = rt::actor(memory, parser, assembly, key.hero);
  if (!out.actor_before.read_valid || !out.actor_before.matches) { out.issue = GrindGuildIssue::actor; return out; }
  out.physical = read_grind_guild_for_preview(memory, parser, key);
  if (!out.physical.known()) { out.issue = out.physical.issue; return out; }
  out.actor_after = rt::actor(memory, parser, assembly, key.hero);
  if (!out.actor_after.read_valid || !out.actor_after.matches ||
      !station_recipe_runtime::same_actor_evidence(out.actor_before, out.actor_after) ||
      !runtime_script::detail::same_layout(out.actor_before.layout, out.physical.layout) ||
      !station_recipe_runtime::detail::same_symbol(out.actor_before.hero_symbol, out.physical.hero_symbol)) {
    out.issue = GrindGuildIssue::actor; return out;
  }
  out.issue = GrindGuildIssue::none;
  return out;
}

// Typed value seam to the existing material projection. Both
// captures must arise in the same idle window; different keys or
// parser layouts yield no known values. No item/hand/end proof.
inline GrindMaterialKnowledge grind_preview_knowledge(const GrindRecipeCatalogueCapture& capture,
    const GrindGuildSnapshot& guild, const GrindRecipeKey& current) noexcept {
  GrindMaterialKnowledge out;
  const auto& d = capture.diagnostic;
  if (!guild.known() || !grind_recipe_snapshot_detail::valid_key(current) ||
      !grind_recipe_snapshot_detail::same_key(capture.key, current) ||
      !grind_recipe_snapshot_detail::same_key(guild.key, current) || guild.hero != current.hero ||
      d.issue != GrindCaptureIssue::none || !d.state_unchanged ||
      !grind_recipe_runtime::same_state(d.state_before, d.state_after) ||
      !d.state_before.station_postcondition() || !d.state_after.station_postcondition() ||
      !runtime_script::detail::same_layout(guild.layout, d.state_before.layout)) return out;
  out.talent_known = out.guild_known = out.skill_known = true;
  out.talent = d.state_before.talent; out.guild = guild.guild; out.skill = d.state_before.skill;
  return out;
}
inline GrindMaterialKnowledge grind_writer_knowledge(const GrindRecipeCatalogueCapture& capture,
    const GrindWriterGuildSnapshot& guild, const GrindRecipeKey& current) noexcept {
  const auto& d = capture.diagnostic;
  if (!guild.known() || !d.actor_unchanged || !d.actor_before.matches || !d.actor_after.matches ||
      !station_recipe_runtime::same_actor_evidence(d.actor_before, guild.actor_before) ||
      !station_recipe_runtime::same_actor_evidence(d.actor_after, guild.actor_after)) return {};
  return grind_preview_knowledge(capture, guild.physical, current);
}
} // namespace ubc
