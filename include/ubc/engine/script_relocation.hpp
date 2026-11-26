// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/core/sha256.hpp"
#include "ubc/engine/read_only_memory.hpp"
#include "ubc/engine/runtime_script.hpp"
#include "ubc/engine/script_fingerprint.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

// One-time relocation of a reference script profile onto the loaded parser.
// Names are resolved through a sorted embedded list, data symbols are checked
// field by field and functions by their UBCF1 fingerprint. A matched function
// records its live index, live start and live raw digest; later writers bind
// exactly that digest. Recipes become active only when their whole group
// matched. No engine calls and no writes; the caller owns all storage.
namespace ubc::script_relocation {

using script_fingerprint::Digest;
using script_fingerprint::Issue;

inline constexpr std::uint32_t no_name = 0xffffffffu;

// Sorted ascending by unsigned bytes, unique, 1..63 bytes, not anonymous.
struct Name {
  const char* text;
  std::uint32_t reference_index;
};

// return_type is compared for FUNC (type 5) only, like check_function.
struct FunctionRecord {
  std::uint32_t name;
  std::uint32_t byte_length;
  Digest canonical_sha256;
  std::uint32_t reference_start;
  Digest reference_raw_sha256;
  std::uint32_t type = 5;
  std::uint32_t flags = 1;
  std::uint32_t argc = 0;
  std::uint32_t return_type = 0;
  std::uint32_t parent = no_name;
  const std::uint32_t* symbolic_ints = nullptr;
  std::size_t symbolic_int_count = 0;
  bool ignore_text = false;
};

struct DataRecord {
  std::uint32_t name;
  std::uint32_t type;
  std::uint32_t flags;
  std::uint32_t elements;
  std::uint32_t parent = no_name;
  bool check_offset = false;
  std::int32_t offset = 0;
  bool check_value = false;
  std::int32_t value = 0;
};

struct RecipeGroup {
  std::uint32_t recipe;
  const std::uint32_t* functions;
  std::size_t function_count;
  const std::uint32_t* data;
  std::size_t data_count;
};

struct ReferenceProfile {
  std::uint32_t parser_address;
  const Name* names;
  std::size_t name_count;
  const FunctionRecord* functions;
  std::size_t function_count;
  const DataRecord* data;
  std::size_t data_count;
  const RecipeGroup* groups;
  std::size_t group_count;
};

enum class NameState : std::uint8_t { missing, found, duplicate };

struct NameSlot {
  NameState state = NameState::missing;
  std::uint32_t live_index = 0;
  runtime_script::detail::Symbol symbol{};
};

struct Key {
  std::uint32_t key = 0;
  std::uint32_t name = no_name;
};

struct FunctionMatch {
  Issue issue = Issue::not_evaluated;
  std::uint32_t live_index = 0;
  std::uint32_t live_start = 0;
  Digest live_raw_sha256{};
  std::uint32_t failed_offset = 0;
};

struct DataMatch {
  Issue issue = Issue::not_evaluated;
  std::uint32_t live_index = 0;
};

enum class RecordKind : std::uint8_t { none, function, data };

struct RecipeState {
  std::uint32_t recipe = 0;
  bool active = false;
  Issue reason = Issue::not_evaluated;
  RecordKind kind = RecordKind::none;
  std::uint32_t record = 0;
};

// by_index and by_entry hold name_capacity keys each.
struct Workspace {
  NameSlot* names = nullptr;
  Key* by_index = nullptr;
  Key* by_entry = nullptr;
  std::size_t name_capacity = 0;
  FunctionMatch* functions = nullptr;
  std::size_t function_capacity = 0;
  DataMatch* data = nullptr;
  std::size_t data_capacity = 0;
  RecipeState* recipes = nullptr;
  std::size_t recipe_capacity = 0;
  script_fingerprint::Scratch* scratch = nullptr;
};

template <std::size_t Names, std::size_t Functions, std::size_t Data, std::size_t Recipes>
struct Storage {
  std::array<NameSlot, Names> names{};
  std::array<Key, Names> by_index{};
  std::array<Key, Names> by_entry{};
  std::array<FunctionMatch, Functions> functions{};
  std::array<DataMatch, Data> data{};
  std::array<RecipeState, Recipes> recipes{};
  script_fingerprint::Scratch scratch{};

  Workspace workspace() noexcept {
    return {names.data(),     by_index.data(), by_entry.data(), Names,
            functions.data(), Functions,       data.data(),     Data,
            recipes.data(),   Recipes,         &scratch};
  }
};

struct Relocation {
  bool valid = false;
  Issue issue = Issue::not_evaluated;
  // Every name, function start and raw body is exactly at its reference place.
  bool identity = false;
  std::uint32_t active = 0;
  std::uint32_t total = 0;
  std::uint32_t names_found = 0;
  std::uint32_t names_missing = 0;
  std::uint32_t names_duplicate = 0;
  std::uint32_t symbol_count = 0;
  std::uint32_t symbol_table = 0;
  std::uint32_t code_base = 0;
  std::uint32_t code_length = 0;
};

namespace detail {
namespace rs = runtime_script::detail;

inline constexpr std::uint32_t table_chunk = 1024;

inline std::string_view view(const char* text) noexcept {
  std::size_t length = 0;
  while (length <= rs::max_name && text[length])
    ++length;
  return {text, length};
}

inline bool valid_profile(const ReferenceProfile& profile) noexcept {
  if (!rs::range(profile.parser_address, 0x58) || profile.name_count == 0 || !profile.names ||
      profile.name_count >= no_name || (profile.function_count && !profile.functions) ||
      (profile.data_count && !profile.data) || (profile.group_count && !profile.groups))
    return false;
  for (std::size_t i = 0; i < profile.name_count; ++i) {
    if (!profile.names[i].text)
      return false;
    const auto name = view(profile.names[i].text);
    if (name.empty() || name.size() > rs::max_name ||
        static_cast<unsigned char>(name.front()) == 0xffu ||
        (i && view(profile.names[i - 1].text).compare(name) >= 0))
      return false;
  }
  const auto known = [&profile](std::uint32_t name, bool optional) noexcept {
    return name < profile.name_count || (optional && name == no_name);
  };
  for (std::size_t i = 0; i < profile.function_count; ++i) {
    const auto& record = profile.functions[i];
    const script_fingerprint::Spec spec{record.byte_length, record.symbolic_ints,
                                        record.symbolic_int_count, record.ignore_text};
    if (!known(record.name, false) || !known(record.parent, true) || record.byte_length == 0 ||
        record.byte_length > script_fingerprint::max_function_length ||
        !script_fingerprint::detail::valid_spec(spec))
      return false;
  }
  for (std::size_t i = 0; i < profile.data_count; ++i) {
    if (!known(profile.data[i].name, false) || !known(profile.data[i].parent, true))
      return false;
  }
  // A recipe is active only on proven code: every group needs a function.
  if (profile.group_count == 0)
    return false;
  for (std::size_t i = 0; i < profile.group_count; ++i) {
    const auto& group = profile.groups[i];
    if (group.function_count == 0 || !group.functions || (group.data_count && !group.data))
      return false;
    for (std::size_t j = 0; j < group.function_count; ++j)
      if (group.functions[j] >= profile.function_count)
        return false;
    for (std::size_t j = 0; j < group.data_count; ++j)
      if (group.data[j] >= profile.data_count)
        return false;
  }
  return true;
}

inline bool fits(const ReferenceProfile& profile, const Workspace& workspace) noexcept {
  return workspace.names && workspace.by_index && workspace.by_entry && workspace.scratch &&
         workspace.name_capacity >= profile.name_count &&
         (!profile.function_count || workspace.functions) &&
         workspace.function_capacity >= profile.function_count &&
         (!profile.data_count || workspace.data) && workspace.data_capacity >= profile.data_count &&
         (!profile.group_count || workspace.recipes) &&
         workspace.recipe_capacity >= profile.group_count;
}

inline std::uint32_t find_name(const ReferenceProfile& profile, std::string_view name) noexcept {
  const auto* begin = profile.names;
  const auto* end = profile.names + profile.name_count;
  const auto* found =
      std::lower_bound(begin, end, name, [](const Name& entry, std::string_view key) {
        return view(entry.text).compare(key) < 0;
      });
  if (found == end || view(found->text) != name)
    return no_name;
  return static_cast<std::uint32_t>(found - begin);
}

inline const Key* find_key(const Key* keys, std::size_t count, std::uint32_t key) noexcept {
  const auto* end = keys + count;
  const auto* found = std::lower_bound(
      keys, end, key, [](const Key& entry, std::uint32_t value) { return entry.key < value; });
  return found != end && found->key == key ? found : nullptr;
}

struct Context {
  const ReadOnlyMemory* memory;
  const rs::Layout* layout;
  const ReferenceProfile* profile;
  const Workspace* workspace;
  std::size_t indexed;
  std::size_t entries;
};

// Anonymous text constants are matched by content, never by their name:
// the loaded parser names them differently than the compiled DAT.
inline Issue anonymous_text(const Context& context, std::uint32_t index,
                            script_fingerprint::Operand& operand) noexcept {
  const auto& memory = *context.memory;
  rs::Symbol symbol{};
  if (rs::lookup(memory, *context.layout, index, symbol) != runtime_script::Issue::none)
    return Issue::symbol_unreadable;
  if (symbol.type != 3 || symbol.flags != 1 || symbol.elements != 1 || symbol.parent ||
      symbol.name_length == 0)
    return Issue::unknown_symbol;
  std::uint8_t first = 0;
  if (!memory.copy(symbol.name_data, &first, 1))
    return Issue::name_unreadable;
  if (first != 0xffu)
    return Issue::unknown_symbol;
  std::array<std::uint8_t, 20> object{};
  if (!rs::range(symbol.data, object.size()) ||
      !memory.copy(symbol.data, object.data(), object.size()))
    return Issue::text_unreadable;
  const auto data = rs::u32(object.data() + 8);
  const auto length = rs::u32(object.data() + 12);
  if (length > script_fingerprint::max_text_length)
    return Issue::text_unreadable;
  Sha256 hash;
  if (length) {
    if (!rs::range(data, static_cast<std::uint64_t>(length) + 1u))
      return Issue::text_unreadable;
    std::array<std::uint8_t, 1024> bytes{};
    std::uint32_t position = 0;
    while (position <= length) {
      const auto remaining = length + 1u - position;
      const auto count =
          remaining < bytes.size() ? remaining : static_cast<std::uint32_t>(bytes.size());
      if (!memory.copy(data + position, bytes.data(), count))
        return Issue::text_unreadable;
      const bool last = position + count == length + 1u;
      if (last && bytes[count - 1] != 0)
        return Issue::text_unreadable;
      hash.update(bytes.data(), last ? count - 1 : count);
      position += count;
    }
  }
  operand.kind = script_fingerprint::OperandKind::text;
  operand.text_length = length;
  operand.text_sha256 = hash.finish();
  return Issue::none;
}

inline Issue resolve_symbol(void* raw, std::uint32_t index,
                            script_fingerprint::Operand& operand) noexcept {
  const auto& context = *static_cast<const Context*>(raw);
  if (index >= context.layout->count)
    return Issue::unknown_symbol;
  const auto* key = find_key(context.workspace->by_index, context.indexed, index);
  if (!key)
    return anonymous_text(context, index, operand);
  if (context.workspace->names[key->name].state != NameState::found)
    return Issue::duplicate_name;
  operand.kind = script_fingerprint::OperandKind::name;
  operand.name = view(context.profile->names[key->name].text);
  return Issue::none;
}

inline Issue resolve_entry(void* raw, std::uint32_t offset,
                           script_fingerprint::Operand& operand) noexcept {
  const auto& context = *static_cast<const Context*>(raw);
  const auto* key = find_key(context.workspace->by_entry, context.entries, offset);
  if (!key || key->name == no_name || context.workspace->names[key->name].state != NameState::found)
    return Issue::unknown_callee;
  operand.kind = script_fingerprint::OperandKind::name;
  operand.name = view(context.profile->names[key->name].text);
  return Issue::none;
}

inline Issue named(const Workspace& workspace, std::uint32_t name) noexcept {
  switch (workspace.names[name].state) {
  case NameState::found:
    return Issue::none;
  case NameState::duplicate:
    return Issue::duplicate_name;
  case NameState::missing:
    break;
  }
  return Issue::missing_name;
}

inline Issue parent_matches(const Workspace& workspace, const rs::Symbol& symbol,
                            std::uint32_t parent) noexcept {
  if (parent == no_name)
    return symbol.parent == 0 ? Issue::none : Issue::symbol_parent;
  if (named(workspace, parent) != Issue::none ||
      workspace.names[parent].symbol.address != symbol.parent)
    return Issue::symbol_parent;
  return Issue::none;
}

// Reads the table in bounded chunks and each symbol's name header. A name
// is read only when its length occurs in the list. Unreadable symbols fail
// the whole scan: they could hide a duplicate name.
inline Issue scan(const ReadOnlyMemory& memory, const rs::Layout& layout,
                  const ReferenceProfile& profile, const Workspace& workspace,
                  std::size_t& indexed) noexcept {
  std::uint64_t lengths = 0;
  for (std::size_t i = 0; i < profile.name_count; ++i)
    lengths |= std::uint64_t{1} << view(profile.names[i].text).size();
  std::array<std::uint8_t, table_chunk * 4u> pointers{};
  std::array<char, rs::max_name> name{};
  for (std::uint32_t first = 0; first < layout.count; first += table_chunk) {
    const auto count = std::min(table_chunk, layout.count - first);
    if (!memory.copy(layout.table + first * 4u, pointers.data(), count * 4u))
      return Issue::symbol_unreadable;
    for (std::uint32_t i = 0; i < count; ++i) {
      const auto address = rs::u32(pointers.data() + i * 4u);
      std::array<std::uint8_t, 8> header{};
      if (address % 4u != 0 || !rs::range(address, 0x3c) ||
          !memory.copy(address + 8u, header.data(), header.size()))
        return Issue::symbol_unreadable;
      const auto data = rs::u32(header.data());
      const auto length = rs::u32(header.data() + 4);
      if (length == 0 || length > rs::max_name || ((lengths >> length) & 1u) == 0)
        continue;
      // zSTRING compares by length; a missing terminator must not hide a duplicate.
      if (!rs::range(data, length) || !memory.copy(data, name.data(), length))
        return Issue::name_unreadable;
      const auto id = find_name(profile, std::string_view(name.data(), length));
      if (id == no_name)
        continue;
      auto& slot = workspace.names[id];
      if (slot.state == NameState::missing) {
        slot.state = NameState::found;
        slot.live_index = first + i;
        slot.symbol.address = address;
        workspace.by_index[indexed++] = {first + i, id};
      } else {
        slot.state = NameState::duplicate;
      }
    }
  }
  return Issue::none;
}

// Constant FUNC and INSTANCE symbols and every PROTOTYPE own a code entry;
// compiled prototypes carry no constant flag. Class members (classvar),
// externals and variables such as SELF also carry type 5..7, but their data
// is no code offset; counting them would mark a real entry at the same offset
// as ambiguous.
inline bool code_symbol(const rs::Symbol& symbol) noexcept {
  if ((symbol.flags & (4u | 8u)) != 0)
    return false;
  return symbol.type == 6 || ((symbol.type == 5 || symbol.type == 7) && (symbol.flags & 1u) != 0);
}

inline Issue check_data(const Workspace& workspace, const DataRecord& record) noexcept {
  auto issue = named(workspace, record.name);
  if (issue != Issue::none)
    return issue;
  const auto& symbol = workspace.names[record.name].symbol;
  if (symbol.type != record.type || symbol.flags != record.flags ||
      symbol.elements != record.elements)
    return Issue::symbol_signature;
  issue = parent_matches(workspace, symbol, record.parent);
  if (issue != Issue::none)
    return issue;
  if (record.check_offset && symbol.offset != static_cast<std::uint32_t>(record.offset))
    return Issue::symbol_signature;
  if (record.check_value && (record.type != 2 || record.elements != 1 || record.flags != 1 ||
                             symbol.data != static_cast<std::uint32_t>(record.value)))
    return Issue::symbol_value;
  return Issue::none;
}

inline Issue check_function(const ReadOnlyMemory& memory, const rs::Layout& layout,
                            const Context& context, const FunctionRecord& record,
                            FunctionMatch& match) noexcept {
  const auto& workspace = *context.workspace;
  auto issue = named(workspace, record.name);
  if (issue != Issue::none)
    return issue;
  const auto& slot = workspace.names[record.name];
  const auto& symbol = slot.symbol;
  const auto return_type = symbol.type == 5 ? symbol.offset : 0u;
  if (symbol.type != record.type || symbol.flags != record.flags ||
      symbol.elements != record.argc || return_type != record.return_type)
    return Issue::symbol_signature;
  issue = parent_matches(workspace, symbol, record.parent);
  if (issue != Issue::none)
    return issue;
  if (symbol.data > layout.code_length || record.byte_length > layout.code_length - symbol.data)
    return Issue::function_range;
  script_fingerprint::Header header{};
  header.name = view(context.profile->names[record.name].text);
  header.type = symbol.type;
  header.flags = symbol.flags;
  header.argc = symbol.elements;
  header.return_type = return_type;
  if (record.parent != no_name)
    header.parent = view(context.profile->names[record.parent].text);
  const script_fingerprint::Spec spec{record.byte_length, record.symbolic_ints,
                                      record.symbolic_int_count, record.ignore_text};
  const script_fingerprint::Resolver resolver{const_cast<Context*>(&context), &resolve_symbol,
                                              &resolve_entry};
  const auto result = script_fingerprint::fingerprint(memory, layout.code, symbol.data, header,
                                                      spec, resolver, *workspace.scratch);
  match.failed_offset = result.failed_offset;
  if (!result.valid())
    return result.issue;
  if (result.canonical != record.canonical_sha256)
    return Issue::fingerprint_mismatch;
  match.live_index = slot.live_index;
  match.live_start = symbol.data;
  match.live_raw_sha256 = result.raw;
  return Issue::none;
}

inline void evaluate_groups(const ReferenceProfile& profile, const Workspace& workspace,
                            Relocation& result) noexcept {
  for (std::size_t i = 0; i < profile.group_count; ++i) {
    const auto& group = profile.groups[i];
    auto& state = workspace.recipes[i];
    state = {};
    state.recipe = group.recipe;
    state.reason = Issue::none;
    for (std::size_t j = 0; j < group.function_count && state.reason == Issue::none; ++j) {
      const auto issue = workspace.functions[group.functions[j]].issue;
      if (issue != Issue::none) {
        state.reason = issue;
        state.kind = RecordKind::function;
        state.record = group.functions[j];
      }
    }
    for (std::size_t j = 0; j < group.data_count && state.reason == Issue::none; ++j) {
      const auto issue = workspace.data[group.data[j]].issue;
      if (issue != Issue::none) {
        state.reason = issue;
        state.kind = RecordKind::data;
        state.record = group.data[j];
      }
    }
    state.active = state.reason == Issue::none;
    if (state.active)
      ++result.active;
  }
}

inline void reject_all(const ReferenceProfile& profile, const Workspace& workspace,
                       Relocation& result, Issue issue) noexcept {
  result.valid = false;
  result.identity = false;
  result.issue = issue;
  result.active = 0;
  for (std::size_t i = 0; i < profile.group_count; ++i) {
    workspace.recipes[i] = {};
    workspace.recipes[i].recipe = profile.groups[i].recipe;
    workspace.recipes[i].reason = issue;
  }
}

inline bool identity(const ReferenceProfile& profile, const Workspace& workspace,
                     const Relocation& result) noexcept {
  if (result.names_missing || result.names_duplicate || result.active != result.total)
    return false;
  for (std::size_t i = 0; i < profile.name_count; ++i)
    if (workspace.names[i].live_index != profile.names[i].reference_index)
      return false;
  for (std::size_t i = 0; i < profile.function_count; ++i) {
    const auto& match = workspace.functions[i];
    if (match.issue != Issue::none || match.live_start != profile.functions[i].reference_start ||
        match.live_raw_sha256 != profile.functions[i].reference_raw_sha256)
      return false;
  }
  for (std::size_t i = 0; i < profile.data_count; ++i)
    if (workspace.data[i].issue != Issue::none)
      return false;
  return true;
}
} // namespace detail

// Runs once after the parser loaded. The caller must observe on the game
// thread; the final layout check rejects a table or code move during the
// scan but does not make the individual reads atomic.
inline Relocation relocate(const ReadOnlyMemory& memory, std::uint32_t parser,
                           const ReferenceProfile& profile, const Workspace& workspace) noexcept {
  Relocation result{};
  if (!detail::valid_profile(profile)) {
    result.issue = Issue::invalid_profile;
    return result;
  }
  result.total = static_cast<std::uint32_t>(profile.group_count);
  if (!detail::fits(profile, workspace)) {
    result.issue = Issue::capacity;
    return result;
  }
  for (std::size_t i = 0; i < profile.name_count; ++i)
    workspace.names[i] = {};
  for (std::size_t i = 0; i < profile.function_count; ++i)
    workspace.functions[i] = {};
  for (std::size_t i = 0; i < profile.data_count; ++i)
    workspace.data[i] = {};
  const lhiver::Profile layout_profile{profile.parser_address, 0, 0, {}, nullptr, 0, nullptr, 0};
  runtime_script::detail::Layout layout{};
  switch (runtime_script::detail::read_layout(memory, parser, layout_profile, layout)) {
  case runtime_script::Issue::none:
    break;
  case runtime_script::Issue::wrong_parser:
    detail::reject_all(profile, workspace, result, Issue::wrong_parser);
    return result;
  case runtime_script::Issue::symbol_table_range:
    detail::reject_all(profile, workspace, result, Issue::symbol_table_range);
    return result;
  case runtime_script::Issue::code_range:
    detail::reject_all(profile, workspace, result, Issue::code_range);
    return result;
  default:
    detail::reject_all(profile, workspace, result, Issue::parser_unreadable);
    return result;
  }
  result.symbol_count = layout.count;
  result.symbol_table = layout.table;
  result.code_base = layout.code;
  result.code_length = layout.code_length;

  std::size_t indexed = 0;
  auto issue = detail::scan(memory, layout, profile, workspace, indexed);
  if (issue != Issue::none) {
    detail::reject_all(profile, workspace, result, issue);
    return result;
  }
  std::size_t entries = 0;
  for (std::size_t i = 0; i < profile.name_count; ++i) {
    auto& slot = workspace.names[i];
    if (slot.state == NameState::missing) {
      ++result.names_missing;
      continue;
    }
    if (slot.state == NameState::duplicate) {
      ++result.names_duplicate;
      continue;
    }
    ++result.names_found;
    const auto address = slot.symbol.address;
    if (runtime_script::detail::lookup(memory, layout, slot.live_index, slot.symbol) !=
            runtime_script::Issue::none ||
        slot.symbol.address != address) {
      detail::reject_all(profile, workspace, result, Issue::symbol_unreadable);
      return result;
    }
    if (detail::code_symbol(slot.symbol))
      workspace.by_entry[entries++] = {slot.symbol.data, static_cast<std::uint32_t>(i)};
  }
  std::sort(workspace.by_entry, workspace.by_entry + entries,
            [](const Key& left, const Key& right) { return left.key < right.key; });
  for (std::size_t i = 1; i < entries; ++i) {
    if (workspace.by_entry[i].key == workspace.by_entry[i - 1].key) {
      workspace.by_entry[i].name = no_name;
      workspace.by_entry[i - 1].name = no_name;
    }
  }

  const detail::Context context{&memory, &layout, &profile, &workspace, indexed, entries};
  for (std::size_t i = 0; i < profile.data_count; ++i)
    workspace.data[i].issue = detail::check_data(workspace, profile.data[i]);
  for (std::size_t i = 0; i < profile.data_count; ++i)
    if (workspace.data[i].issue == Issue::none)
      workspace.data[i].live_index = workspace.names[profile.data[i].name].live_index;
  for (std::size_t i = 0; i < profile.function_count; ++i)
    workspace.functions[i].issue = detail::check_function(
        memory, layout, context, profile.functions[i], workspace.functions[i]);
  if (runtime_script::detail::stable_layout(memory, parser, layout_profile, layout) !=
      runtime_script::Issue::none) {
    detail::reject_all(profile, workspace, result, Issue::changed_during_read);
    return result;
  }
  detail::evaluate_groups(profile, workspace, result);
  result.valid = true;
  result.issue = Issue::none;
  result.identity = detail::identity(profile, workspace, result);
  return result;
}

// Live index of a reference symbol index, or false when it was not
// resolved uniquely. Linear in the name count; the generated profile may
// add an index-sorted permutation when a hot path needs it.
inline bool live_symbol(const ReferenceProfile& profile, const Workspace& workspace,
                        std::uint32_t reference_index, std::uint32_t& live_index) noexcept {
  for (std::size_t i = 0; i < profile.name_count; ++i) {
    if (profile.names[i].reference_index != reference_index)
      continue;
    if (workspace.names[i].state != NameState::found)
      return false;
    live_index = workspace.names[i].live_index;
    return true;
  }
  return false;
}

} // namespace ubc::script_relocation
