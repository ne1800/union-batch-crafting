// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/engine/read_only_memory.hpp"
#include "ubc/stations/station_abi.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ubc {

struct InventorySnapshot {
  bool valid = false;
  bool totals_known = false;
  std::uint32_t nodes = 0;
  std::array<std::int32_t, 6> listed{};
};

namespace station_detail {

inline std::uint32_t little_u32(const std::uint8_t* bytes) noexcept {
  return std::uint32_t{bytes[0]} | (std::uint32_t{bytes[1]} << 8) |
         (std::uint32_t{bytes[2]} << 16) | (std::uint32_t{bytes[3]} << 24);
}

inline std::int32_t little_i32(const std::uint8_t* bytes) noexcept {
  const auto value = little_u32(bytes);
  if (value <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
    return static_cast<std::int32_t>(value);
  return -1 - static_cast<std::int32_t>(std::numeric_limits<std::uint32_t>::max() - value);
}

inline bool at_offset(const ReadOnlyMemory& memory, std::uint32_t base,
                      std::uint32_t offset, void* output, std::size_t size) noexcept {
  if (!base || offset > std::numeric_limits<std::uint32_t>::max() - base) return false;
  return memory.copy(base + offset, output, size);
}

// Two fixed, at most half-full tables avoid heap accesses as well as
// quadratic comparisons on long lists. Zero is not a valid pointer.
struct AddressSet {
  static constexpr std::size_t capacity = 8192;
  std::array<std::uint32_t, capacity> slots{};

  bool insert(std::uint32_t address) noexcept {
    if (!address) return false;
    auto mixed = address;
    mixed ^= mixed >> 16;
    mixed *= 0x7feb352du;
    mixed ^= mixed >> 15;
    mixed *= 0x846ca68bu;
    mixed ^= mixed >> 16;
    auto index = static_cast<std::size_t>(mixed) & (capacity - 1);
    for (std::size_t probes = 0; probes < capacity; ++probes) {
      if (slots[index] == address) return false;
      if (!slots[index]) { slots[index] = address; return true; }
      index = (index + 1) & (capacity - 1);
    }
    return false;
  }
};

}  // namespace station_detail

// Read only the already existing list. Packable inventories may contain
// further stocks outside the list; totals_known then remains false.
// On every error the sums are empty; nodes counts only fully checked
// nodes before the error. The reader must run on a stable engine thread.
inline InventorySnapshot read_inventory(
    const ReadOnlyMemory& memory, std::uint32_t hero,
    const std::array<std::int32_t, 6>& symbols) noexcept {
  namespace abi = station_abi;
  using station_detail::little_i32;
  using station_detail::little_u32;
  InventorySnapshot result;
  std::array<std::uint8_t, 20> inventory_header{};
  if (!station_detail::at_offset(memory, hero, abi::npc_inventory + abi::inventory_owner,
                                inventory_header.data(), inventory_header.size()) ||
      little_u32(inventory_header.data()) != hero) return result;

  const auto packed = little_u32(inventory_header.data() + 4);
  auto current = little_u32(inventory_header.data() + 16);
  std::array<std::int32_t, 6> sums{};
  station_detail::AddressSet nodes;
  station_detail::AddressSet items;
  while (current) {
    if (result.nodes == abi::inventory_node_limit || !nodes.insert(current)) return result;
    std::array<std::uint8_t, abi::inventory_node_size> node{};
    if (!memory.copy(current, node.data(), node.size())) return result;
    const auto item = little_u32(node.data() + abi::inventory_node_item);
    if (!items.insert(item)) return result;
    std::array<std::uint8_t, 8> item_fields{};
    if (!station_detail::at_offset(memory, item, abi::item_amount,
                                  item_fields.data(), item_fields.size())) return result;
    const auto amount = little_i32(item_fields.data());
    const auto symbol = little_i32(item_fields.data() + 4);
    if (amount < 0) return result;
    for (std::size_t i = 0; i < sums.size(); ++i) {
      if (symbol != symbols[i]) continue;
      if (amount > std::numeric_limits<std::int32_t>::max() - sums[i]) return result;
      sums[i] += amount;
    }
    ++result.nodes;
    current = little_u32(node.data() + abi::inventory_node_next);
  }
  result.valid = true;
  result.totals_known = packed == 0;
  result.listed = sums;
  return result;
}

}  // namespace ubc
