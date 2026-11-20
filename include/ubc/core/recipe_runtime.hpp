// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/catalogs/recipe_catalog.hpp"
#include "ubc/engine/runtime_script.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace ubc::recipe_runtime {

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
inline ProfileAssembly assemble(const recipe_catalog::Descriptor& descriptor) noexcept;
inline Knowledge knowledge(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const ProfileAssembly& assembly) noexcept;

namespace detail {
inline bool supported_family(recipe_catalog::RecipeId id) noexcept {
  using recipe_catalog::RecipeId;
  switch (id) {
    case RecipeId::meat: case RecipeId::health_01: case RecipeId::health_02:
    case RecipeId::health_03: case RecipeId::mana_01: case RecipeId::mana_02:
    case RecipeId::mana_03: case RecipeId::molerat_dish: case RecipeId::mushroom_dish: return true;
  }
  return false;
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
template<class T> inline bool bounded(recipe_catalog::View<T> values) noexcept {
  return values.count <= profile_capacity && (!values.count || values.data);
}
inline bool bounded_metadata(const recipe_catalog::Descriptor& descriptor) noexcept {
  if (!bounded(descriptor.targets) || !bounded(descriptor.bindings) || !bounded(descriptor.requirements) ||
      !bounded(descriptor.owners) || !bounded(descriptor.callbacks) ||
      !bounded(descriptor.costs) || !bounded(descriptor.outputs)) return false;
  for (const auto& owner : descriptor.owners) if (!bounded(owner.conditions)) return false;
  for (const auto& callback : descriptor.callbacks)
    if (!bounded(callback.effects) || !bounded(callback.failure_effects)) return false;
  return true;
}
} // namespace detail

// Own immutable value contract, including all names. Copies never
// point into the memory of their source object or descriptor.
// profile() is a borrowed view: Assembly during its use neither
// replace nor destroy. A temporary Assembly must not yield a view.
class ProfileAssembly {
public:
  ProfileAssembly() noexcept = default;
  ProfileAssembly(const ProfileAssembly& other) noexcept { *this = other; }
  ProfileAssembly& operator=(const ProfileAssembly& other) noexcept {
    if (this != &other) {
      issue_ = other.issue_;
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
  bool valid() const noexcept { return issue_ == AssemblyIssue::none; }
  AssemblyIssue issue() const noexcept { return issue_; }
  std::size_t function_count() const noexcept { return function_count_; }
  std::size_t binding_count() const noexcept { return binding_count_; }
  std::size_t knowledge_count() const noexcept { return knowledge_count_; }
  lhiver::Profile profile() const & noexcept {
    if (!valid()) return {}; // No apparently valid empty local profile.
    return {lhiver::profile.parser_address, lhiver::profile.symbol_count,
      lhiver::profile.code_length, lhiver::profile.code_sha256,
      functions_.data(), function_count_, bindings_.data(), binding_count_};
  }
  lhiver::Profile profile() const && = delete;

private:
  friend ProfileAssembly assemble(const recipe_catalog::Descriptor&) noexcept;
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
  bool add_knowledge(const recipe_catalog::Requirement& requirement) noexcept {
    using namespace recipe_catalog;
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
  bool owner_knowledge_matches(const recipe_catalog::Owner& owner) noexcept {
    std::array<bool, profile_capacity> found{};
    for (const auto& requirement : owner.conditions) {
      if (requirement.role != recipe_catalog::RequirementRole::knowledge) continue;
      bool match = false;
      for (std::size_t i = 0; i < knowledge_count_; ++i)
        if (knowledge_[i].symbol == requirement.symbol && knowledge_[i].element == requirement.element &&
            knowledge_[i].value == requirement.value) { found[i] = true; match = true; break; }
      if (!match) return reject(AssemblyIssue::knowledge_contract);
    }
    for (std::size_t i = 0; i < knowledge_count_; ++i)
      if (!found[i]) return reject(AssemblyIssue::knowledge_contract);
    return true;
  }

  AssemblyIssue issue_ = AssemblyIssue::descriptor;
  std::array<lhiver::Function, profile_capacity> functions_{};
  std::array<lhiver::SymbolBinding, profile_capacity> bindings_{};
  std::array<std::array<char, 64>, profile_capacity> function_names_{}, binding_names_{};
  std::array<recipe_catalog::Requirement, profile_capacity> knowledge_{};
  std::size_t function_count_ = 0, binding_count_ = 0, knowledge_count_ = 0;
};

inline ProfileAssembly assemble(const recipe_catalog::Descriptor& descriptor) noexcept {
  ProfileAssembly result;
  if (!detail::bounded_metadata(descriptor)) { result.reject(AssemblyIssue::capacity); return result; }
  if (!detail::supported_family(descriptor.id) || !recipe_catalog::structurally_valid(descriptor)) return result;
  result.issue_ = AssemblyIssue::none;
  for (const auto& binding : recipe_catalog::common_bindings) if (!result.add_binding(binding)) return result;
  for (const auto& binding : descriptor.bindings) if (!result.add_binding(binding)) return result;
  for (const auto& owner : descriptor.owners)
    for (const auto* function : {owner.constructor, owner.condition, owner.information})
      if (!result.add_function(*function)) return result;
  for (const auto& callback : descriptor.callbacks) if (!result.add_function(*callback.function)) return result;
  if (!result.add_function(*descriptor.station_entry) || !result.add_function(*descriptor.station_end)) return result;
  // Statically verified shared BL closure for these nine stove/lab
  // families: every recipe path reaches B_SAY. The lab recipes
  // and both station end paths reach B_ENDPRODUCTIONDIALOG. No VM
  // execution or assumed dependency analysis for other stations.
  if (!result.add_function(recipe_catalog::functions[0]) ||
      !result.add_function(recipe_catalog::functions[1])) return result;
  for (std::size_t i = 0; i < result.function_count_; ++i)
    for (std::size_t j = 0; j < result.binding_count_; ++j) {
      const auto& function = result.functions_[i]; const auto& binding = result.bindings_[j];
      if (function.symbol != binding.symbol) {
        if (std::strcmp(function.name, binding.name) == 0) {
          result.reject(AssemblyIssue::binding_conflict); return result;
        }
        continue;
      }
      if (std::strcmp(function.name, binding.name) != 0 || function.argc != binding.elements ||
          function.type != binding.type || function.flags != binding.flags ||
          function.parent_index != binding.parent_index || binding.check_value ||
          (binding.check_offset && function.return_type != static_cast<std::uint32_t>(binding.offset))) {
        result.reject(AssemblyIssue::binding_conflict); return result;
      }
    }
  for (const auto& requirement : descriptor.requirements) if (!result.add_knowledge(requirement)) return result;
  // The shared requirements describe an AND for all owners. A
  // deviating owner knowledge set needs its own alternative model.
  for (const auto& owner : descriptor.owners) if (!result.owner_knowledge_matches(owner)) return result;
  if (!runtime_script::detail::valid_profile(result.profile())) result.reject(AssemblyIssue::descriptor);
  return result;
}

// Local contract with the existing parser/table/code-area rules. No new
// full-code identity: additional code/symbols retain exactly the same
// meaning as in verify_local_contract; a match is not a verify_runtime.
inline runtime_script::RuntimeContract verify(const ReadOnlyMemory& memory, std::uint32_t parser,
                                             const ProfileAssembly& assembly) noexcept {
  if (!assembly.valid()) return {false, runtime_script::Issue::invalid_profile, 0};
  return runtime_script::verify_local_contract(memory, parser, assembly.profile());
}
inline runtime_script::RuntimeContract verify(const ReadOnlyMemory& memory, std::uint32_t parser,
                                             const recipe_catalog::Descriptor& descriptor) noexcept {
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
// separate. On the idle game thread: two passes and layout comparison
// make foreign concurrent write accesses not atomic or ABA-safe.
inline Knowledge knowledge(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const ProfileAssembly& assembly) noexcept {
  namespace rs = runtime_script;
  Knowledge result;
  if (!assembly.valid()) { result.issue = rs::Issue::invalid_profile; return result; }
  const auto profile = assembly.profile();
  if (parser != profile.parser_address) { result.issue = rs::Issue::wrong_parser; return result; }
  // Without a learning predicate, no dynamic read is required. This says
  // explicitly nothing about validity or availability of the game world.
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
                           const recipe_catalog::Descriptor& descriptor) noexcept {
  const auto assembly = assemble(descriptor);
  return knowledge(memory, parser, assembly);
}

} // namespace ubc::recipe_runtime
