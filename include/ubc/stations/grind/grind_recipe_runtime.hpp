// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/grind_recipe_profile.hpp"
#include "ubc/stations/station_recipe_runtime.hpp"
#include "ubc/stations/alchemy/alchemy_flask_runtime.hpp"

// Read-only script profiles and global values. No weapon inventory,
// guild object, hand, native, writer, or station end contract.
namespace ubc::grind_recipe_runtime {
namespace g = grind_recipe;
namespace facts = grind_recipe_profile;
namespace rs = runtime_script;
namespace shared = station_recipe_runtime::detail;
using ActorEvidence = station_recipe_runtime::ActorEvidence;

class ProfileAssembly;
inline ProfileAssembly assemble(const g::Descriptor&) noexcept;
class ProfileAssembly {
public:
  bool valid() const noexcept { return recipe_ != nullptr; }
  const g::Descriptor* recipe() const noexcept { return recipe_; }
  lhiver::Profile profile() const& noexcept {
    return valid() ? lhiver::Profile{lhiver::game_parser,
                                     lhiver::symbol_count,
                                     lhiver::code_length,
                                     lhiver::code_sha256,
                                     functions_.data(),
                                     functions_.size(),
                                     facts::common_bindings.data(),
                                     facts::common_bindings.size()}
                   : lhiver::Profile{};
  }
  lhiver::Profile profile() const&& = delete;
  const lhiver::SymbolBinding* binding(std::uint32_t symbol) const noexcept {
    if (valid())
      for (const auto& b : facts::common_bindings)
        if (b.symbol == symbol)
          return &b;
    return nullptr;
  }

private:
  friend ProfileAssembly assemble(const g::Descriptor&) noexcept;
  const g::Descriptor* recipe_ = nullptr;
  std::array<lhiver::Function, facts::common_functions.size() + 1> functions_{};
};
inline ProfileAssembly assemble(const g::Descriptor& descriptor) noexcept {
  ProfileAssembly out;
  if (!g::structurally_valid(descriptor))
    return out;
  out.recipe_ = g::find(descriptor.function.symbol);
  for (std::size_t i = 0; i < facts::common_functions.size(); ++i)
    out.functions_[i] = facts::common_functions[i];
  out.functions_.back() = out.recipe_->function;
  if (!rs::detail::valid_profile(out.profile()) || !facts::texts_for(descriptor.function.symbol) ||
      !facts::item_for(static_cast<std::uint32_t>(descriptor.input)) ||
      !facts::item_for(static_cast<std::uint32_t>(descriptor.output)) || !facts::item_for(16613))
    out.recipe_ = nullptr;
  return out;
}
inline lhiver::Profile item_profile(const facts::Item& item) noexcept {
  // Accepts only static canonical entries; no foreign pointers are read.
  for (const auto& known : facts::items)
    if (&item == &known)
      return {lhiver::game_parser, lhiver::symbol_count, lhiver::code_length,
              lhiver::code_sha256, item.function,        1,
              item.bindings,       item.binding_count};
  return {};
}
inline rs::RuntimeContract verify(const ReadOnlyMemory& memory, std::uint32_t parser,
                                  const ProfileAssembly& assembly) noexcept {
  return assembly.valid() ? rs::verify_local_contract(memory, parser, assembly.profile())
                          : rs::RuntimeContract{false, rs::Issue::invalid_profile, 0};
}
struct Verification {
  bool valid = false;
  rs::RuntimeContract local{};
  alchemy_flask_runtime::Issue string_issue = alchemy_flask_runtime::Issue::none;
  std::uint32_t failed_symbol = 0;
};
inline Verification verify_recipe(const ReadOnlyMemory& memory, std::uint32_t parser,
                                  const g::Descriptor& descriptor) noexcept {
  Verification out;
  const auto assembly = assemble(descriptor);
  if (!assembly.valid()) {
    out.local.issue = rs::Issue::invalid_profile;
    return out;
  }
  const auto profile = assembly.profile();
  rs::detail::Layout initial{};
  out.local.issue = rs::detail::read_layout(memory, parser, profile, initial);
  if (out.local.issue != rs::Issue::none)
    return out;
  const auto strings = [&](const facts::Text* values, std::size_t count) noexcept {
    for (std::size_t i = 0; i < count; ++i) {
      const auto& text = values[i];
      const auto read = alchemy_flask_runtime::detail::string_once(
          memory, initial, {text.symbol, text.length, text.sha256});
      if (!read.valid) {
        out.string_issue = read.issue;
        out.failed_symbol = text.symbol;
        return false;
      }
    }
    return true;
  };
  const auto* own_texts = facts::texts_for(descriptor.function.symbol);
  for (unsigned pass = 0; pass < 2; ++pass) {
    out.local = verify(memory, parser, assembly);
    if (!out.local.matches)
      return out;
    if (!strings(facts::common_texts.data(), facts::common_texts.size()) ||
        !strings(own_texts->texts.data(), own_texts->texts.size()))
      return out;
    for (const auto symbol : {descriptor.input, descriptor.output, g::lifecycle_item}) {
      const auto* item = facts::item_for(static_cast<std::uint32_t>(symbol));
      out.local = rs::verify_local_contract(memory, parser, item_profile(*item));
      if (!out.local.matches || !strings(item->texts, item->text_count))
        return out;
    }
    out.local.issue = rs::detail::stable_layout(memory, parser, profile, initial);
    if (out.local.issue != rs::Issue::none) {
      out.local.matches = false;
      return out;
    }
  }
  out.valid = true;
  return out;
}

struct StateEvidence {
  bool read_valid = false;
  std::int32_t production = -1, first_values = 0, talent = 0, skill = 0;
  rs::Issue issue = rs::Issue::none;
  std::uint32_t failed_symbol = 0;
  rs::detail::Layout layout{};
  std::array<shared::IntegerRead, 4> readings{};
  bool learned() const noexcept { return read_valid && talent == 1; }
  // The original entry initializes only when FIRSTVALUESHARP==0. Leave
  // other nonzero values as they are. Do not introduce a rule requiring
  // 1. This script postcondition does not establish entry or hand state.
  bool station_postcondition() const noexcept {
    return read_valid && production == 11 && first_values != 0;
  }
};
inline bool same_state(const StateEvidence& a, const StateEvidence& b) noexcept {
  if (!a.read_valid || !b.read_valid || !rs::detail::same_layout(a.layout, b.layout) ||
      a.production != b.production || a.first_values != b.first_values || a.talent != b.talent ||
      a.skill != b.skill)
    return false;
  for (std::size_t i = 0; i < a.readings.size(); ++i)
    if (a.readings[i].value != b.readings[i].value ||
        !shared::same_symbol(a.readings[i].symbol, b.readings[i].symbol))
      return false;
  return true;
}
namespace detail {
inline StateEvidence state_once(const ReadOnlyMemory& memory, std::uint32_t parser,
                                const ProfileAssembly& assembly) noexcept {
  StateEvidence out;
  if (!assembly.valid()) {
    out.issue = rs::Issue::invalid_profile;
    return out;
  }
  const auto profile = assembly.profile();
  out.issue = rs::detail::read_layout(memory, parser, profile, out.layout);
  if (out.issue != rs::Issue::none)
    return out;
  constexpr std::array<std::uint32_t, 4> symbols{{12330, 10502, 10501, 10505}};
  for (std::size_t i = 0; i < symbols.size(); ++i) {
    const auto* binding = assembly.binding(symbols[i]);
    if (!binding) {
      out.issue = rs::Issue::invalid_profile;
      return out;
    }
    out.readings[i] = shared::read_integer(memory, out.layout, *binding, 0);
    if (out.readings[i].issue != rs::Issue::none) {
      out.issue = out.readings[i].issue;
      out.failed_symbol = symbols[i];
      return out;
    }
  }
  out.issue = rs::detail::stable_layout(memory, parser, profile, out.layout);
  if (out.issue != rs::Issue::none)
    return out;
  out.production = out.readings[0].value;
  out.first_values = out.readings[1].value;
  out.talent = out.readings[2].value;
  out.skill = out.readings[3].value;
  out.read_valid = true;
  return out;
}
inline ActorEvidence actor_once(const ReadOnlyMemory& memory, std::uint32_t parser,
                                const ProfileAssembly& assembly, std::uint32_t hero) noexcept {
  ActorEvidence out;
  out.requires_self = true;
  if (!assembly.valid() || !hero) {
    out.issue = rs::Issue::invalid_profile;
    return out;
  }
  const auto profile = assembly.profile();
  out.issue = rs::detail::read_layout(memory, parser, profile, out.layout);
  if (out.issue != rs::Issue::none)
    return out;
  out.issue = shared::instance(memory, out.layout, profile, 1683, out.hero_symbol);
  if (out.issue != rs::Issue::none) {
    out.failed_symbol = 1683;
    return out;
  }
  out.issue = shared::instance(memory, out.layout, profile, 1679, out.self_symbol);
  if (out.issue != rs::Issue::none) {
    out.failed_symbol = 1679;
    return out;
  }
  out.hero = out.hero_symbol.offset;
  out.self = out.self_symbol.offset;
  out.issue = rs::detail::stable_layout(memory, parser, profile, out.layout);
  out.read_valid = out.issue == rs::Issue::none;
  out.matches = out.read_valid && out.hero == hero && out.self == hero;
  return out;
}
} // namespace detail
inline StateEvidence state(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const ProfileAssembly& assembly) noexcept {
  auto before = detail::state_once(memory, parser, assembly);
  if (!before.read_valid)
    return before;
  const auto after = detail::state_once(memory, parser, assembly);
  if (!after.read_valid)
    return after;
  if (!same_state(before, after)) {
    before.read_valid = false;
    before.issue = rs::Issue::changed_during_read;
  }
  return before;
}
inline ActorEvidence actor(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const ProfileAssembly& assembly, std::uint32_t hero) noexcept {
  auto before = detail::actor_once(memory, parser, assembly, hero);
  if (!before.read_valid)
    return before;
  const auto after = detail::actor_once(memory, parser, assembly, hero);
  if (!after.read_valid)
    return after;
  if (!station_recipe_runtime::same_actor_evidence(before, after)) {
    before.read_valid = before.matches = false;
    before.issue = rs::Issue::changed_during_read;
  }
  return before;
}
} // namespace ubc::grind_recipe_runtime
