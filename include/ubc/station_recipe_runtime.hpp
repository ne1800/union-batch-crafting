// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/station_recipe_catalog.hpp"
#include "ubc/runtime_script.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace ubc::station_recipe_runtime {

inline constexpr std::size_t profile_capacity = 64; // Boundary of verify_local_contract.
enum class AssemblyIssue {
  none, descriptor, capacity, function_contract, function_conflict,
  binding_contract, binding_conflict, knowledge_contract
};

struct Knowledge {
  bool known = false;
  bool read_valid = false;
  runtime_script::Issue issue = runtime_script::Issue::none;
  std::uint32_t failed_symbol = 0;
  std::size_t checked = 0;
};
class ProfileAssembly;
inline ProfileAssembly assemble(const station_recipe_catalog::Descriptor& descriptor) noexcept;
inline Knowledge knowledge(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const ProfileAssembly& assembly) noexcept;

namespace detail {
inline bool supported_family(station_recipe_catalog::RecipeId id) noexcept {
  return station_recipe_catalog::find(id) != nullptr;
}
inline std::size_t name_length(const char* name) noexcept {
  if (!name) return 0;
  std::size_t count = 0;
  while (count < 64 && name[count]) ++count;
  return count;
}
inline bool same_function(const lhiver::Function& a, const lhiver::Function& b) noexcept {
  return std::strcmp(a.name, b.name) == 0 && a.symbol == b.symbol &&
    a.byte_offset == b.byte_offset && a.byte_length == b.byte_length && a.body_sha256 == b.body_sha256 &&
    a.argc == b.argc && a.type == b.type && a.flags == b.flags &&
    a.return_type == b.return_type && a.parent_index == b.parent_index;
}
inline bool same_binding(const lhiver::SymbolBinding& a, const lhiver::SymbolBinding& b) noexcept {
  return std::strcmp(a.name, b.name) == 0 && a.symbol == b.symbol && a.type == b.type &&
    a.flags == b.flags && a.elements == b.elements && a.parent_index == b.parent_index &&
    a.check_value == b.check_value && a.value == b.value &&
    a.check_offset == b.check_offset && a.offset == b.offset;
}
template<class T> inline bool bounded(station_recipe_catalog::View<T> values) noexcept {
  return values.count <= profile_capacity && (!values.count || values.data);
}
inline bool bounded_metadata(const station_recipe_catalog::Descriptor& descriptor) noexcept {
  return bounded(descriptor.targets) && bounded(descriptor.bindings) && bounded(descriptor.requirements) &&
    bounded(descriptor.checks) && bounded(descriptor.costs) && bounded(descriptor.outputs) &&
    bounded(descriptor.owner.conditions) && bounded(descriptor.callback.effects) &&
    bounded(descriptor.callback.failure_effects) && descriptor.station_profile &&
    bounded(descriptor.station_profile->end.conditions);
}
} // namespace detail

// Own immutable value contract, including all names. Copies never
// appear in the memory of their source object or descriptor.
// profile() is a borrowed view: assembly during its use neither
// replaces nor destroys. A temporary assembly must not yield a view.
class ProfileAssembly {
public:
  ProfileAssembly() noexcept = default;
  ProfileAssembly(const ProfileAssembly& other) noexcept { *this = other; }
  ProfileAssembly& operator=(const ProfileAssembly& other) noexcept {
    if (this != &other) {
      issue_ = other.issue_; actor_ = other.actor_;
      functions_ = other.functions_; bindings_ = other.bindings_;
      function_names_ = other.function_names_; binding_names_ = other.binding_names_;
      knowledge_ = other.knowledge_;
      function_count_ = other.function_count_; binding_count_ = other.binding_count_;
      knowledge_count_ = other.knowledge_count_;
      for (std::size_t i = 0; i < function_count_; ++i) functions_[i].name = function_names_[i].data();
      for (std::size_t i = 0; i < binding_count_; ++i) bindings_[i].name = binding_names_[i].data();
    }
    return *this;
  }
  station_recipe_catalog::Actor material_actor() const noexcept { return actor_; }
  bool valid() const noexcept { return issue_ == AssemblyIssue::none; }
  AssemblyIssue issue() const noexcept { return issue_; }
  std::size_t function_count() const noexcept { return function_count_; }
  std::size_t binding_count() const noexcept { return binding_count_; }
  std::size_t knowledge_count() const noexcept { return knowledge_count_; }
  lhiver::Profile profile() const & noexcept {
    if (!valid()) return {}; // No seemingly valid empty local profile.
    return {lhiver::profile.parser_address, lhiver::profile.symbol_count,
      lhiver::profile.code_length, lhiver::profile.code_sha256,
      functions_.data(), function_count_, bindings_.data(), binding_count_};
  }
  lhiver::Profile profile() const && = delete;

private:
  friend ProfileAssembly assemble(const station_recipe_catalog::Descriptor&) noexcept;
  friend Knowledge knowledge(const ReadOnlyMemory&, std::uint32_t, const ProfileAssembly&) noexcept;
  bool reject(AssemblyIssue issue) noexcept { issue_ = issue; return false; }
  bool add_function(const lhiver::Function& function) noexcept {
    const auto length = detail::name_length(function.name);
    if (!length || length > 63 || !function.symbol || function.symbol >= lhiver::symbol_count ||
        !function.byte_length || function.byte_offset >= lhiver::code_length ||
        function.byte_length > lhiver::code_length - function.byte_offset ||
        function.argc > 0xfff || function.type > 0xf || function.flags > 0x3f ||
        function.parent_index < -1 || (function.parent_index >= 0 &&
        static_cast<std::uint32_t>(function.parent_index) >= lhiver::symbol_count))
      return reject(AssemblyIssue::function_contract);
    for (std::size_t i = 0; i < function_count_; ++i) {
      const auto& earlier = functions_[i];
      if (earlier.symbol == function.symbol) {
        if (detail::same_function(earlier, function)) return true;
        return reject(AssemblyIssue::function_conflict);
      }
      if (std::strcmp(earlier.name, function.name) == 0 ||
          (function.byte_offset < earlier.byte_offset + earlier.byte_length &&
           earlier.byte_offset < function.byte_offset + function.byte_length))
        return reject(AssemblyIssue::function_conflict);
    }
    if (function_count_ == profile_capacity) return reject(AssemblyIssue::capacity);
    functions_[function_count_] = function;
    std::memcpy(function_names_[function_count_].data(), function.name, length + 1);
    functions_[function_count_].name = function_names_[function_count_].data();
    ++function_count_;
    return true;
  }
  bool add_binding(const lhiver::SymbolBinding& binding) noexcept {
    const auto length = detail::name_length(binding.name);
    if (!length || length > 63 || !binding.symbol || binding.symbol >= lhiver::symbol_count ||
        binding.type > 0xf || binding.flags > 0x3f || binding.elements > 0xfff ||
        binding.parent_index < -1 || (binding.parent_index >= 0 &&
        static_cast<std::uint32_t>(binding.parent_index) >= lhiver::symbol_count) ||
        (binding.check_offset && binding.offset < 0) ||
        (binding.check_value && (binding.type != 2 || binding.flags != 1 ||
                                binding.elements != 1 || binding.parent_index != -1)))
      return reject(AssemblyIssue::binding_contract);
    for (std::size_t i = 0; i < binding_count_; ++i) {
      const auto& earlier = bindings_[i];
      if (earlier.symbol == binding.symbol) {
        if (detail::same_binding(earlier, binding)) return true;
        return reject(AssemblyIssue::binding_conflict);
      }
      if (std::strcmp(earlier.name, binding.name) == 0) return reject(AssemblyIssue::binding_conflict);
    }
    if (binding_count_ == profile_capacity) return reject(AssemblyIssue::capacity);
    bindings_[binding_count_] = binding;
    std::memcpy(binding_names_[binding_count_].data(), binding.name, length + 1);
    bindings_[binding_count_].name = binding_names_[binding_count_].data();
    ++binding_count_;
    return true;
  }
  const lhiver::SymbolBinding* binding_for(std::uint32_t symbol) const noexcept {
    for (std::size_t i = 0; i < binding_count_; ++i) if (bindings_[i].symbol == symbol) return &bindings_[i];
    return nullptr;
  }
  bool add_knowledge(const station_recipe_catalog::Requirement& requirement) noexcept {
    using namespace station_recipe_catalog;
    if (requirement.role != RequirementRole::knowledge) return true;
    const auto* binding = requirement.symbol > 0 ? binding_for(static_cast<std::uint32_t>(requirement.symbol)) : nullptr;
    if (requirement.kind != RequirementKind::integer_equal || !binding || binding->type != 2 ||
        binding->flags > 1 || binding->parent_index != -1 || !binding->elements ||
        requirement.element >= binding->elements)
      return reject(AssemblyIssue::knowledge_contract);
    for (std::size_t i = 0; i < knowledge_count_; ++i)
      if (knowledge_[i].symbol == requirement.symbol && knowledge_[i].element == requirement.element) {
        if (knowledge_[i].value == requirement.value) return true;
        return reject(AssemblyIssue::knowledge_contract);
      }
    if (knowledge_count_ == profile_capacity) return reject(AssemblyIssue::capacity);
    knowledge_[knowledge_count_++] = requirement;
    return true;
  }
  station_recipe_catalog::Actor actor_ = station_recipe_catalog::Actor::hero;
  AssemblyIssue issue_ = AssemblyIssue::descriptor;
  std::array<lhiver::Function, profile_capacity> functions_{};
  std::array<lhiver::SymbolBinding, profile_capacity> bindings_{};
  std::array<std::array<char, 64>, profile_capacity> function_names_{}, binding_names_{};
  std::array<station_recipe_catalog::Requirement, profile_capacity> knowledge_{};
  std::size_t function_count_ = 0, binding_count_ = 0, knowledge_count_ = 0;
};

inline ProfileAssembly assemble(const station_recipe_catalog::Descriptor& descriptor) noexcept {
  namespace cat = station_recipe_catalog;
  ProfileAssembly result;
  if (!detail::bounded_metadata(descriptor)) { result.reject(AssemblyIssue::capacity); return result; }
  if (!detail::supported_family(descriptor.id) || !cat::structurally_valid(descriptor)) return result;
  result.issue_ = AssemblyIssue::none; result.actor_ = descriptor.material_actor;
  // The successor catalog contains Common and individual bindings already complete.
  for (const auto& binding : descriptor.bindings) if (!result.add_binding(binding)) return result;
  for (const auto* owner : {&descriptor.owner, &descriptor.station_profile->end})
    for (const auto* function : {owner->constructor, owner->condition, owner->information})
      if (!result.add_function(*function)) return result;
  for (const auto* function : {descriptor.back, descriptor.callback.function, descriptor.station_profile->entry})
    if (!result.add_function(*function)) return result;
  if (descriptor.station_profile->mob_condition && !result.add_function(*descriptor.station_profile->mob_condition)) return result;
  for (const auto& check : descriptor.checks) if (!result.add_function(*check.function)) return result;
  // Every examined recipe path reaches B_SAY. The normal station end path
  // reaches B_ENDPRODUCTIONDIALOG, even if ammunition itself remains Ready.
  if (!result.add_function(*cat::function_for(14646)) || !result.add_function(*cat::function_for(19965))) return result;
  for (std::size_t i = 0; i < result.function_count_; ++i)
    for (std::size_t j = 0; j < result.binding_count_; ++j) {
      const auto& function = result.functions_[i]; const auto& binding = result.bindings_[j];
      if (function.symbol != binding.symbol) {
        if (std::strcmp(function.name, binding.name) == 0) { result.reject(AssemblyIssue::binding_conflict); return result; }
        continue;
      }
      if (std::strcmp(function.name, binding.name) != 0 || function.argc != binding.elements ||
          function.type != binding.type || function.flags != binding.flags || function.parent_index != binding.parent_index ||
          binding.check_value || (binding.check_offset &&
          (function.type == 7 || function.return_type != static_cast<std::uint32_t>(binding.offset)))) {
        result.reject(AssemblyIssue::binding_conflict); return result;
      }
    }
  // The location of the learning check is recorded in the catalog: leather in the
  // callback, ammo in the owner info. No invented requirement in the C_INFO condition.
  for (const auto& requirement : descriptor.requirements) if (!result.add_knowledge(requirement)) return result;
  if (!runtime_script::detail::valid_profile(result.profile())) result.reject(AssemblyIssue::descriptor);
  return result;
}

// Local contract with the existing parser/table/code-area rules. No new
// full code identity: additional code/symbols retain exactly the same
// meaning as in verify_local_contract; a match is not a verify_runtime.
inline runtime_script::RuntimeContract verify(const ReadOnlyMemory& memory, std::uint32_t parser,
                                             const ProfileAssembly& assembly) noexcept {
  if (!assembly.valid()) return {false, runtime_script::Issue::invalid_profile, 0};
  return runtime_script::verify_local_contract(memory, parser, assembly.profile());
}
inline runtime_script::RuntimeContract verify(const ReadOnlyMemory& memory, std::uint32_t parser,
                                             const station_recipe_catalog::Descriptor& descriptor) noexcept {
  const auto assembly = assemble(descriptor);
  return verify(memory, parser, assembly);
}

namespace detail {
namespace rs = runtime_script;
struct IntegerRead {
  std::int32_t value = 0;
  rs::detail::Symbol symbol{};
  rs::Issue issue = rs::Issue::none;
};
inline std::int32_t signed_value(std::uint32_t value) noexcept {
  if (value <= static_cast<std::uint32_t>(INT32_MAX)) return static_cast<std::int32_t>(value);
  return -1 - static_cast<std::int32_t>(UINT32_MAX - value);
}
inline bool same_symbol(const rs::detail::Symbol& a, const rs::detail::Symbol& b) noexcept {
  return a.address == b.address && a.name_data == b.name_data && a.name_length == b.name_length &&
    a.data == b.data && a.offset == b.offset && a.elements == b.elements &&
    a.type == b.type && a.flags == b.flags && a.parent == b.parent;
}
inline IntegerRead read_integer(const ReadOnlyMemory& memory, const rs::detail::Layout& layout,
                               const lhiver::SymbolBinding& binding, std::uint32_t element) noexcept {
  IntegerRead result;
  result.issue = rs::detail::lookup(memory, layout, binding.symbol, result.symbol);
  if (result.issue != rs::Issue::none) return result;
  const auto& symbol = result.symbol;
  result.issue = rs::detail::name_matches(memory, symbol, binding.name);
  if (result.issue != rs::Issue::none) return result;
  if (symbol.type != binding.type || symbol.flags != binding.flags || symbol.elements != binding.elements ||
      symbol.type != 2 || symbol.flags > 1 ||
      !symbol.elements || element >= symbol.elements ||
      (binding.check_offset && symbol.offset != static_cast<std::uint32_t>(binding.offset))) {
    result.issue = rs::Issue::symbol_signature; return result;
  }
  if (binding.parent_index != -1 || symbol.parent) { result.issue = rs::Issue::symbol_parent; return result; }
  if (binding.check_value && symbol.data != static_cast<std::uint32_t>(binding.value)) {
    result.issue = rs::Issue::symbol_value; return result;
  }
  auto value = symbol.data;
  if (symbol.elements > 1) {
    // The entire declared array area must be representable;
    // only four bytes of the verified element are read anyway.
    if (symbol.data % 4 || !rs::detail::range(symbol.data, static_cast<std::uint64_t>(symbol.elements) * 4)) {
      result.issue = rs::Issue::symbol_unreadable; return result;
    }
    std::array<std::uint8_t, 4> bytes{};
    if (!memory.copy(symbol.data + element * 4, bytes.data(), bytes.size())) {
      result.issue = rs::Issue::symbol_unreadable; return result;
    }
    value = rs::detail::u32(bytes.data());
  }
  result.value = signed_value(value);
  return result;
}
} // namespace detail

// Learning knowledge only. Context/resource values, inventories,
// conditions and script functions are not executed or read. known is
// not a production release; verify and station/material gates remain
// separate. On the idle game thread: two runs and layout comparison
// make foreign concurrent writes neither atomic nor ABA-safe.
inline Knowledge knowledge(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const ProfileAssembly& assembly) noexcept {
  namespace rs = runtime_script;
  Knowledge result;
  if (!assembly.valid()) { result.issue = rs::Issue::invalid_profile; return result; }
  const auto profile = assembly.profile();
  if (parser != profile.parser_address) { result.issue = rs::Issue::wrong_parser; return result; }
  // Without a learn predicate, dynamic reading is not required. This
  // explicitly says nothing about validity or availability of the game world.
  if (!assembly.knowledge_count_) { result.known = result.read_valid = true; return result; }
  rs::detail::Layout layout{};
  result.issue = rs::detail::read_layout(memory, parser, profile, layout);
  if (result.issue != rs::Issue::none) return result;
  std::array<detail::IntegerRead, profile_capacity> before{};
  bool learned = true;
  for (std::size_t i = 0; i < assembly.knowledge_count_; ++i) {
    const auto& requirement = assembly.knowledge_[i];
    const auto* binding = assembly.binding_for(static_cast<std::uint32_t>(requirement.symbol));
    before[i] = detail::read_integer(memory, layout, *binding, requirement.element);
    if (before[i].issue != rs::Issue::none) {
      result.issue = before[i].issue; result.failed_symbol = binding->symbol; return result;
    }
    ++result.checked;
    if (before[i].value != requirement.value) learned = false;
  }
  for (std::size_t i = 0; i < assembly.knowledge_count_; ++i) {
    const auto& requirement = assembly.knowledge_[i];
    const auto* binding = assembly.binding_for(static_cast<std::uint32_t>(requirement.symbol));
    const auto after = detail::read_integer(memory, layout, *binding, requirement.element);
    if (after.issue != rs::Issue::none || after.value != before[i].value ||
        !detail::same_symbol(after.symbol, before[i].symbol)) {
      result.issue = after.issue != rs::Issue::none ? after.issue : rs::Issue::changed_during_read;
      result.failed_symbol = binding->symbol; return result;
    }
  }
  result.issue = rs::detail::stable_layout(memory, parser, profile, layout);
  if (result.issue != rs::Issue::none) return result;
  result.read_valid = true;
  result.known = learned;
  return result;
}
inline Knowledge knowledge(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const station_recipe_catalog::Descriptor& descriptor) noexcept {
  const auto assembly = assemble(descriptor);
  return knowledge(memory, parser, assembly);
}

struct ActorEvidence {
  bool read_valid = false, matches = false, requires_self = false;
  runtime_script::Issue issue = runtime_script::Issue::none;
  std::uint32_t failed_symbol = 0, hero = 0, self = 0;
  // Own copies for a before/after comparison, not borrowed pointers.
  runtime_script::detail::Layout layout{};
  runtime_script::detail::Symbol hero_symbol{}, self_symbol{};
};
namespace detail {
inline runtime_script::Issue instance(const ReadOnlyMemory& memory,
    const runtime_script::detail::Layout& layout, const lhiver::Profile& profile,
    std::uint32_t index, runtime_script::detail::Symbol& symbol) noexcept {
  namespace rs = runtime_script;
  const lhiver::SymbolBinding* binding = nullptr;
  for (std::size_t i = 0; i < profile.binding_count; ++i)
    if (profile.bindings[i].symbol == index) binding = &profile.bindings[i];
  // SELF/HERO are native C_NPC instances. Their dynamic value sits
  // in the offset field +0x1C, not in the data/bytecode field +0x18.
  if (!binding || binding->type != 7 || binding->flags != 1 || binding->elements ||
      binding->parent_index != 1537 || binding->check_value || binding->check_offset) return rs::Issue::invalid_profile;
  auto issue = rs::detail::lookup(memory, layout, index, symbol);
  if (issue != rs::Issue::none) return issue;
  issue = rs::detail::name_matches(memory, symbol, binding->name);
  if (issue != rs::Issue::none) return issue;
  if (symbol.type != binding->type || symbol.flags != binding->flags || symbol.elements != binding->elements)
    return rs::Issue::symbol_signature;
  return rs::detail::parent_matches(memory, layout, symbol, binding->parent_index);
}
inline ActorEvidence actor_once(const ReadOnlyMemory& memory, std::uint32_t parser,
                               const ProfileAssembly& assembly, std::uint32_t expected_hero) noexcept {
  namespace rs = runtime_script;
  ActorEvidence value;
  if (!assembly.valid() || !expected_hero) { value.issue = rs::Issue::invalid_profile; return value; }
  const auto profile = assembly.profile();
  value.requires_self = assembly.material_actor() == station_recipe_catalog::Actor::self;
  value.issue = rs::detail::read_layout(memory, parser, profile, value.layout);
  if (value.issue != rs::Issue::none) return value;
  value.issue = instance(memory, value.layout, profile, 1683, value.hero_symbol);
  if (value.issue != rs::Issue::none) { value.failed_symbol = 1683; return value; }
  value.hero = value.hero_symbol.offset;
  if (value.requires_self) {
    value.issue = instance(memory, value.layout, profile, 1679, value.self_symbol);
    if (value.issue != rs::Issue::none) { value.failed_symbol = 1679; return value; }
    value.self = value.self_symbol.offset;
  }
  value.issue = rs::detail::stable_layout(memory, parser, profile, value.layout);
  if (value.issue != rs::Issue::none) return value;
  value.read_valid = true;
  value.matches = value.hero == expected_hero && (!value.requires_self || value.self == expected_hero);
  return value;
}
} // namespace detail
inline bool same_actor_evidence(const ActorEvidence& a, const ActorEvidence& b) noexcept {
  return a.read_valid && b.read_valid && a.requires_self == b.requires_self &&
    a.hero == b.hero && a.self == b.self && runtime_script::detail::same_layout(a.layout, b.layout) &&
    detail::same_symbol(a.hero_symbol, b.hero_symbol) &&
    (!a.requires_self || detail::same_symbol(a.self_symbol, b.self_symbol));
}

// Two fully bound read passes; used on the same idle game thread.
// Snapshot can additionally compare evidence BEFORE/AFTER its
// capture. Neither this gate nor knowledge proves a runtime body hash.
inline ActorEvidence actor(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const ProfileAssembly& assembly, std::uint32_t expected_hero) noexcept {
  auto before = detail::actor_once(memory, parser, assembly, expected_hero);
  if (!before.read_valid) return before;
  const auto after = detail::actor_once(memory, parser, assembly, expected_hero);
  if (!after.read_valid) return after;
  if (!same_actor_evidence(before, after)) {
    before.read_valid = before.matches = false;
    before.issue = runtime_script::Issue::changed_during_read;
  }
  return before;
}
inline ActorEvidence actor(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const station_recipe_catalog::Descriptor& descriptor, std::uint32_t expected_hero) noexcept {
  const auto assembly = assemble(descriptor);
  return actor(memory, parser, assembly, expected_hero);
}

} // namespace ubc::station_recipe_runtime
