// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/engine/read_only_memory.hpp"
#include "ubc/stations/station_snapshot.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace ubc {

// Own observation limits, not engine constants.
inline constexpr std::uint32_t max_packed_inventory_bytes = 256u * 1024u;
inline constexpr std::uint32_t max_packed_inventory_entries = 8192;
inline constexpr std::uint32_t max_packed_inventory_nodes = 4096;

struct PackedInventoryTarget {
  // Trustworthy own profile string, max 63 characters plus NUL. The
  // native script name/index must be separately semantically validated.
  const char* name;
  std::int32_t symbol;
};

enum class PackedInventoryIssue {
  none, invalid_targets, header_unreadable, wrong_owner,
  list_node_limit, list_cycle, list_unreadable, invalid_item_pointer,
  duplicate_item, item_unreadable, invalid_item_fields, listed_overflow,
  invalid_pack_length, packed_byte_limit, string_pointer, string_unreadable,
  string_termination, packed_entry_limit, invalid_syntax, invalid_amount,
  amount_overflow, packed_overflow, verification_unreadable, changed_during_read,
  pack_disabled_nonempty, total_overflow, boundary_unverified
};

inline const char* packed_inventory_issue_name(PackedInventoryIssue issue) noexcept {
  switch (issue) {
    case PackedInventoryIssue::none: return "ok";
    case PackedInventoryIssue::invalid_targets: return "invalid_targets";
    case PackedInventoryIssue::header_unreadable: return "header_unreadable";
    case PackedInventoryIssue::wrong_owner: return "wrong_owner";
    case PackedInventoryIssue::list_node_limit: return "list_node_limit";
    case PackedInventoryIssue::list_cycle: return "list_cycle";
    case PackedInventoryIssue::list_unreadable: return "list_unreadable";
    case PackedInventoryIssue::invalid_item_pointer: return "invalid_item_pointer";
    case PackedInventoryIssue::duplicate_item: return "duplicate_item";
    case PackedInventoryIssue::item_unreadable: return "item_unreadable";
    case PackedInventoryIssue::invalid_item_fields: return "invalid_item_fields";
    case PackedInventoryIssue::listed_overflow: return "listed_overflow";
    case PackedInventoryIssue::invalid_pack_length: return "invalid_pack_length";
    case PackedInventoryIssue::packed_byte_limit: return "packed_byte_limit";
    case PackedInventoryIssue::string_pointer: return "string_pointer";
    case PackedInventoryIssue::string_unreadable: return "string_unreadable";
    case PackedInventoryIssue::string_termination: return "string_termination";
    case PackedInventoryIssue::packed_entry_limit: return "packed_entry_limit";
    case PackedInventoryIssue::invalid_syntax: return "invalid_syntax";
    case PackedInventoryIssue::invalid_amount: return "invalid_amount";
    case PackedInventoryIssue::amount_overflow: return "amount_overflow";
    case PackedInventoryIssue::packed_overflow: return "packed_overflow";
    case PackedInventoryIssue::verification_unreadable: return "verification_unreadable";
    case PackedInventoryIssue::changed_during_read: return "changed_during_read";
    case PackedInventoryIssue::pack_disabled_nonempty: return "pack_disabled_nonempty";
    case PackedInventoryIssue::total_overflow: return "total_overflow";
    case PackedInventoryIssue::boundary_unverified: return "boundary_unverified";
  }
  return "unknown_issue";
}

namespace packed_detail {
template<std::size_t N>
constexpr std::array<std::int32_t, N> unknown_amounts() noexcept {
  std::array<std::int32_t, N> result{};
  for (auto& amount : result) amount = -1;
  return result;
}
} // namespace packed_detail

template<std::size_t Capacity>
struct PackedInventorySnapshotT {
  bool header_known = false;
  bool listed_known = false;
  bool packed_known = false;
  bool storage_totals_known = false;
  bool stable_boundary_verified = false;
  bool snapshot_unchanged = false;
  std::int32_t pack_ability = 0; // Evaluate only with header_known; every !=0 is active.
  std::uint32_t listed_nodes = 0;
  std::uint32_t packed_entries = 0;
  std::array<std::int32_t, Capacity> listed = packed_detail::unknown_amounts<Capacity>();
  std::array<std::int32_t, Capacity> packed = packed_detail::unknown_amounts<Capacity>();
  std::array<std::int32_t, Capacity> total = packed_detail::unknown_amounts<Capacity>();
  std::array<std::uint32_t, Capacity> listed_occurrences{};
  std::array<std::uint32_t, Capacity> packed_occurrences{};
  // Target-oriented pack indicators; evaluate only together with packed_known.
  // E does not map all item flags; duplicates of foreign names are allowed.
  std::array<bool, Capacity> duplicate_packed_target{};
  std::array<bool, Capacity> has_active_marker{};
  std::array<bool, Capacity> mixed_representation{};
  bool any_active_marker = false; // Including syntactically valid foreign entries.
  PackedInventoryIssue issue = PackedInventoryIssue::none;
  // Only [0, target_count) are recipe targets. The rest remains unknown
  // (-1), even if all active targets are known. Invalid view size: 0.
  std::size_t target_count = Capacity;
};
using PackedInventorySnapshot = PackedInventorySnapshotT<6>;

namespace packed_detail {
struct ListRecord {
  std::uint32_t address;
  std::array<std::uint8_t, 12> node;
  std::array<std::uint8_t, 8> item_fields;
};
} // namespace packed_detail

// Held once by the caller, preferably statically on the bound game
// thread. Do not use simultaneously or recursively and do not allocate per
// callback on the stack. The reader itself reserves no heap. The copies
// allow real byte comparisons; mere sum or hash equality is not enough.
struct PackedInventoryWorkspace {
  std::array<std::uint8_t, max_packed_inventory_bytes + 1> packed_bytes;
  std::array<packed_detail::ListRecord, max_packed_inventory_nodes> records;
  station_detail::AddressSet node_addresses;
  station_detail::AddressSet item_addresses;
};
static_assert(sizeof(PackedInventoryWorkspace) <= 512u * 1024u, "Begrenzter wiederverwendeter Lesebereich");

namespace packed_detail {
using Issue = PackedInventoryIssue;
using station_detail::little_u32;
using station_detail::little_i32;
using station_detail::at_offset;
inline constexpr auto max_amount = std::numeric_limits<std::int32_t>::max();
inline constexpr std::uint32_t header_offset = 0x708;
using Header = std::array<std::uint8_t, 40>;

inline bool first_name_char(std::uint8_t ch) noexcept {
  return (ch >= 'A' && ch <= 'Z') || ch == '_';
}
inline bool name_char(std::uint8_t ch) noexcept {
  return first_name_char(ch) || (ch >= '0' && ch <= '9');
}
template<std::size_t Capacity>
inline bool validate_targets(const PackedInventoryTarget* targets, std::size_t target_count,
                             std::array<std::size_t, Capacity>& lengths) noexcept {
  for (std::size_t i = 0; i < target_count; ++i) {
    const auto& target = targets[i];
    if (!target.name || target.symbol < 0 || !first_name_char(static_cast<std::uint8_t>(target.name[0])))
      return false;
    std::size_t length = 0;
    while (length < 64 && target.name[length]) {
      if (!name_char(static_cast<std::uint8_t>(target.name[length]))) return false;
      ++length;
    }
    if (!length || length > 63) return false;
    lengths[i] = length;
    for (std::size_t j = 0; j < i; ++j)
      if (targets[j].symbol == target.symbol || (lengths[j] == length &&
          std::memcmp(targets[j].name, target.name, length) == 0)) return false;
  }
  return true;
}

template<std::size_t Capacity>
struct Part {
  bool known = false;
  Issue issue = Issue::none;
  std::array<std::int32_t, Capacity> sums{};
  std::array<std::uint32_t, Capacity> occurrences{};
  std::array<bool, Capacity> active{};
  bool any_active = false;
  std::uint32_t count = 0;
};

template<std::size_t Capacity>
inline Part<Capacity> read_list(const ReadOnlyMemory& memory, std::uint32_t current,
                      const PackedInventoryTarget* targets, std::size_t target_count,
                      PackedInventoryWorkspace& work) noexcept {
  Part<Capacity> result;
  work.node_addresses.slots.fill(0);
  work.item_addresses.slots.fill(0);
  while (current) {
    if (result.count == max_packed_inventory_nodes) { result.issue = Issue::list_node_limit; return result; }
    if (!work.node_addresses.insert(current)) { result.issue = Issue::list_cycle; return result; }
    auto& record = work.records[result.count];
    record.address = current;
    if (!memory.copy(current, record.node.data(), record.node.size())) {
      result.issue = Issue::list_unreadable; return result;
    }
    const auto item = little_u32(record.node.data() + 4);
    if (!item) { result.issue = Issue::invalid_item_pointer; return result; }
    if (!work.item_addresses.insert(item)) { result.issue = Issue::duplicate_item; return result; }
    if (!at_offset(memory, item, 0x32c, record.item_fields.data(), record.item_fields.size())) {
      result.issue = Issue::item_unreadable; return result;
    }
    const auto amount = little_i32(record.item_fields.data());
    const auto symbol = little_i32(record.item_fields.data() + 4);
    if (amount < 0 || symbol < 0) { result.issue = Issue::invalid_item_fields; return result; }
    for (std::size_t i = 0; i < target_count; ++i) {
      if (symbol != targets[i].symbol) continue;
      if (amount > max_amount - result.sums[i]) { result.issue = Issue::listed_overflow; return result; }
      result.sums[i] += amount;
      ++result.occurrences[i];
    }
    ++result.count;
    current = little_u32(record.node.data() + 8);
  }
  result.known = true;
  return result;
}

template<std::size_t Capacity>
inline Part<Capacity> read_pack(const ReadOnlyMemory& memory, std::uint32_t pointer, std::uint32_t length,
                      const PackedInventoryTarget* targets, std::size_t target_count,
                      const std::array<std::size_t, Capacity>& lengths,
                      PackedInventoryWorkspace& work) noexcept {
  Part<Capacity> result;
  if (length > static_cast<std::uint32_t>(max_amount)) { result.issue = Issue::invalid_pack_length; return result; }
  if (length > max_packed_inventory_bytes) { result.issue = Issue::packed_byte_limit; return result; }
  if (!pointer && length) { result.issue = Issue::string_pointer; return result; }
  auto& bytes = work.packed_bytes;
  if (pointer) {
    if (!memory.copy(pointer, bytes.data(), static_cast<std::size_t>(length) + 1)) {
      result.issue = Issue::string_unreadable; return result;
    }
  } else bytes[0] = 0;
  if (bytes[length] != 0 || std::memchr(bytes.data(), 0, length)) {
    result.issue = Issue::string_termination; return result;
  }
  std::uint32_t pos = 0;
  while (pos < length) {
    if (result.count == max_packed_inventory_entries) { result.issue = Issue::packed_entry_limit; return result; }
    const auto start = pos;
    if (!first_name_char(bytes[pos])) { result.issue = Issue::invalid_syntax; return result; }
    while (pos < length && name_char(bytes[pos])) ++pos;
    const auto name_length = pos - start;
    if (pos == length || bytes[pos++] != ':') { result.issue = Issue::invalid_syntax; return result; }
    if (pos == length || bytes[pos] < '0' || bytes[pos] > '9') {
      result.issue = Issue::invalid_amount; return result;
    }
    std::uint32_t amount = 0;
    while (pos < length && bytes[pos] >= '0' && bytes[pos] <= '9') {
      const auto digit = static_cast<std::uint32_t>(bytes[pos++] - '0');
      if (amount > (static_cast<std::uint32_t>(max_amount) - digit) / 10) {
        result.issue = Issue::amount_overflow; return result;
      }
      amount = amount * 10 + digit;
    }
    if (!amount) { result.issue = Issue::invalid_amount; return result; }
    const bool active = pos < length && bytes[pos] == 'E';
    if (active) ++pos;
    if (pos < length && bytes[pos] != ',') { result.issue = Issue::invalid_syntax; return result; }
    for (std::size_t i = 0; i < target_count; ++i) {
      if (lengths[i] != name_length || std::memcmp(bytes.data() + start, targets[i].name, name_length)) continue;
      if (amount > static_cast<std::uint32_t>(max_amount - result.sums[i])) {
        result.issue = Issue::packed_overflow; return result;
      }
      result.sums[i] += static_cast<std::int32_t>(amount);
      ++result.occurrences[i];
      result.active[i] = result.active[i] || active;
    }
    result.any_active = result.any_active || active;
    ++result.count;
    if (pos < length && ++pos == length) { result.issue = Issue::invalid_syntax; return result; }
  }
  result.known = true;
  return result;
}

inline Issue compare_list(const ReadOnlyMemory& memory, const PackedInventoryWorkspace& work,
                           std::uint32_t count) noexcept {
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto& record = work.records[i];
    std::array<std::uint8_t, 12> node{};
    std::array<std::uint8_t, 8> fields{};
    const auto item = little_u32(record.node.data() + 4);
    if (!memory.copy(record.address, node.data(), node.size()) ||
        !at_offset(memory, item, 0x32c, fields.data(), fields.size())) return Issue::verification_unreadable;
    if (node != record.node || fields != record.item_fields) return Issue::changed_during_read;
  }
  return Issue::none;
}

inline Issue compare_pack(const ReadOnlyMemory& memory, const PackedInventoryWorkspace& work,
                           std::uint32_t pointer, std::uint32_t length) noexcept {
  if (!pointer) return Issue::none; // Only the previously validated empty null string.
  std::array<std::uint8_t, 4096> bytes{};
  const auto size = static_cast<std::size_t>(length) + 1;
  for (std::size_t offset = 0; offset < size;) {
    const auto remaining = size - offset;
    const auto count = remaining < bytes.size() ? remaining : bytes.size();
    if (!at_offset(memory, pointer, static_cast<std::uint32_t>(offset), bytes.data(), count))
      return Issue::verification_unreadable;
    if (std::memcmp(bytes.data(), work.packed_bytes.data() + offset, count)) return Issue::changed_during_read;
    offset += count;
  }
  return Issue::none;
}

template<std::size_t Capacity>
inline PackedInventorySnapshotT<Capacity> failed(Issue issue, bool boundary,
                                                std::size_t target_count) noexcept {
  PackedInventorySnapshotT<Capacity> result;
  result.issue = issue;
  result.stable_boundary_verified = boundary;
  result.target_count = target_count;
  return result;
}
} // namespace packed_detail

// Layout for the previously, separately verified Gothic2.exe, SHA-256
// 0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f:
// I=H+0x668; I+A0 owner, +A4 packAbility, +B0 first node,
// +B4 zSTRING, +BC data, +C0 length. GetNumItemsInPackString 0x70F64B,
// GetAmount 0x70C970 and PackSingleItem 0x70FCB0 establish the fields and grammar.
// stable_boundary_verified requires independent evidence of a quiescent point
// on the game thread, outside pack/unpack and inventory mutations, with no reentry.
// Matching copies cannot replace this evidence or guarantee atomic consistency
// against asynchronous external mutations. No engine calls or writes.
// Known totals represent stored L+P. They neither guarantee the return value of
// Npc_HasItems/GetAmount nor authorize recipes for mixed or duplicate entries.
// Capacity describes only the caller's chosen output buffer. There is no fixed
// limit on the number of targets: only 1 <= target_count <= Capacity is valid.
// targets must contain target_count immutable profile entries of our own.
// Unused buffer slots are neither validated nor reported as known zeros.
template<std::size_t Capacity>
inline PackedInventorySnapshotT<Capacity> read_packed_inventory(
    const ReadOnlyMemory& memory, std::uint32_t hero,
    const PackedInventoryTarget* targets, std::size_t target_count, bool stable_boundary_verified,
    PackedInventoryWorkspace& work) noexcept {
  namespace detail = packed_detail;
  using Issue = PackedInventoryIssue;
  if (!targets || !target_count || target_count > Capacity)
    return detail::failed<Capacity>(Issue::invalid_targets, stable_boundary_verified, 0);
  std::array<std::size_t, Capacity> lengths{};
  if (!detail::validate_targets(targets, target_count, lengths))
    return detail::failed<Capacity>(Issue::invalid_targets, stable_boundary_verified, target_count);
  detail::Header header{};
  if (!detail::at_offset(memory, hero, detail::header_offset, header.data(), header.size()))
    return detail::failed<Capacity>(Issue::header_unreadable, stable_boundary_verified, target_count);
  if (detail::little_u32(header.data()) != hero)
    return detail::failed<Capacity>(Issue::wrong_owner, stable_boundary_verified, target_count);
  const auto pointer = detail::little_u32(header.data() + 28);
  const auto length = detail::little_u32(header.data() + 32);
  const auto listed = detail::read_list<Capacity>(memory, detail::little_u32(header.data() + 16),
                                                 targets, target_count, work);
  const auto packed = detail::read_pack(memory, pointer, length, targets, target_count, lengths, work);

  auto verification = listed.known ? detail::compare_list(memory, work, listed.count) : Issue::none;
  if (verification == Issue::none && packed.known) verification = detail::compare_pack(memory, work, pointer, length);
  if (verification != Issue::none)
    return detail::failed<Capacity>(verification, stable_boundary_verified, target_count);
  detail::Header after{};
  if (!detail::at_offset(memory, hero, detail::header_offset, after.data(), after.size()))
    return detail::failed<Capacity>(Issue::verification_unreadable, stable_boundary_verified, target_count);
  if (header != after)
    return detail::failed<Capacity>(Issue::changed_during_read, stable_boundary_verified, target_count);

  PackedInventorySnapshotT<Capacity> result;
  result.target_count = target_count;
  result.header_known = true;
  result.stable_boundary_verified = stable_boundary_verified;
  result.pack_ability = detail::little_i32(header.data() + 4);
  result.listed_nodes = listed.count;
  result.listed_known = listed.known;
  result.packed_known = packed.known;
  result.snapshot_unchanged = listed.known && packed.known;
  result.issue = listed.issue != Issue::none ? listed.issue : packed.issue;
  if (listed.known) {
    for (std::size_t i = 0; i < target_count; ++i) result.listed[i] = listed.sums[i];
    result.listed_occurrences = listed.occurrences;
  }
  if (packed.known) {
    for (std::size_t i = 0; i < target_count; ++i) result.packed[i] = packed.sums[i];
    result.packed_occurrences = packed.occurrences;
    result.has_active_marker = packed.active;
    result.any_active_marker = packed.any_active;
    result.packed_entries = packed.count;
    for (std::size_t i = 0; i < target_count; ++i)
      result.duplicate_packed_target[i] = packed.occurrences[i] > 1;
  }
  if (!result.snapshot_unchanged) return result;
  for (std::size_t i = 0; i < target_count; ++i)
    result.mixed_representation[i] = listed.occurrences[i] != 0 && packed.occurrences[i] != 0;
  if (!result.pack_ability && length) { result.issue = Issue::pack_disabled_nonempty; return result; }
  auto totals = detail::unknown_amounts<Capacity>();
  for (std::size_t i = 0; i < target_count; ++i) {
    if (packed.sums[i] > detail::max_amount - listed.sums[i]) { result.issue = Issue::total_overflow; return result; }
    totals[i] = listed.sums[i] + packed.sums[i];
  }
  if (!stable_boundary_verified) { result.issue = Issue::boundary_unverified; return result; }
  result.storage_totals_known = true;
  result.total = totals;
  return result;
}

// Previous call remains unchanged; the optional target count
// also allows a real prefix in the reused, catalog-derived array.
template<std::size_t Capacity>
inline PackedInventorySnapshotT<Capacity> read_packed_inventory(
    const ReadOnlyMemory& memory, std::uint32_t hero,
    const std::array<PackedInventoryTarget, Capacity>& targets, bool stable_boundary_verified,
    PackedInventoryWorkspace& work, std::size_t target_count = Capacity) noexcept {
  return read_packed_inventory<Capacity>(memory, hero, targets.data(), target_count,
                                         stable_boundary_verified, work);
}

} // namespace ubc
