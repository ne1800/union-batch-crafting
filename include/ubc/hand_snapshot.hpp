// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/read_only_memory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace ubc {

struct HandObjectSnapshot {
  // -1: not reliably readable; 0: empty; 1: exact native item; 2:
  // occupied, type unknown. Only state 1 contains item metadata.
  std::int32_t state = -1;
  std::int32_t symbol = -1;
  std::int32_t amount = -1;
  // Use only as a transient identity for comparison with the
  // inventory list; don't log or dereference after a native call.
  std::uint32_t object = 0;
};

struct HandsSnapshot {
  HandObjectSnapshot left;
  HandObjectSnapshot right;
};

namespace hand_detail {

// Layout only for the previously verified complete Gothic2.exe:
// SHA-256 0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f.
// Pinned API: gothicuef/gothic-api@65fb327c76f58c751468c8e7df917a86a89b9ed1,
// Gothic_II_Addon/API/oNpc.h: TNpcSlot (0x38) and invSlot.
// GetRightHand 0x73AB50 / GetLeftHand 0x73ABE0 read the array at +0x9B0,
// count at +0x9B8, slot name at +0 and Vob at +0x30. The NPC constructor
// at 0x72DB7D initializes the array header, including capacity at +0x9B4.
inline constexpr std::uint32_t slot_array_offset = 0x9b0;
inline constexpr std::size_t slot_limit = 256;
inline constexpr std::size_t name_limit = 63;
inline constexpr std::uint32_t item_vtable = 0x83c804;
inline constexpr std::uint32_t item_class_method = 0x711410;
inline constexpr std::array<std::uint8_t, 6> item_class_bytes{{
    0xb8, 0x68, 0x11, 0xab, 0x00, 0xc3}};

inline std::uint32_t u32(const std::uint8_t* bytes) noexcept {
  return std::uint32_t{bytes[0]} | (std::uint32_t{bytes[1]} << 8) |
         (std::uint32_t{bytes[2]} << 16) | (std::uint32_t{bytes[3]} << 24);
}

inline std::int32_t i32(const std::uint8_t* bytes) noexcept {
  const auto value = u32(bytes);
  if (value <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
    return static_cast<std::int32_t>(value);
  return -1 - static_cast<std::int32_t>(std::numeric_limits<std::uint32_t>::max() - value);
}

inline bool at_offset(const ReadOnlyMemory& memory, std::uint32_t base,
                      std::uint32_t offset, void* output, std::size_t size) noexcept {
  if (!base || offset > std::numeric_limits<std::uint32_t>::max() - base) return false;
  return memory.copy(base + offset, output, size);
}

inline HandObjectSnapshot read_object(const ReadOnlyMemory& memory,
                                      std::uint32_t object) noexcept {
  if (!object) return {0, -1, -1};
  std::array<std::uint8_t, 4> word{};
  if (!memory.copy(object, word.data(), word.size())) return {};
  if (u32(word.data()) != item_vtable) return {2, -1, -1, object};

  // The native item constructor writes 0x83C804 at 0x7113B5.
  // Its vtable[0] points to 0x711410: MOV EAX,0xAB1168; RET
  // (_GetClassDef). Only this unchanged mapping proves an exact oCItem
  // here. No virtual function is executed; derived types remain 2.
  if (!memory.copy(item_vtable, word.data(), word.size())) return {};
  if (u32(word.data()) != item_class_method) return {2, -1, -1};
  std::array<std::uint8_t, item_class_bytes.size()> code{};
  if (!memory.copy(item_class_method, code.data(), code.size())) return {};
  if (code != item_class_bytes) return {2, -1, -1};

  // Item::GetInstance 0x711420 confirms +0x330; inventory
  // read path 0x70C9B2 confirms the signed quantity at +0x32C.
  std::array<std::uint8_t, 8> fields{};
  if (!at_offset(memory, object, 0x32c, fields.data(), fields.size())) return {};
  const auto amount = i32(fields.data());
  const auto symbol = i32(fields.data() + 4);
  if (amount < 0 || symbol < 0) return {};
  return {1, symbol, amount, object};
}

struct SlotName {
  std::uint32_t address = 0;
  std::uint32_t length = 0;
  std::array<char, name_limit + 1> text{};
};

}  // namespace hand_detail

// Read only existing slots and item fields. Execute on the
// stable engine thread; no getters, RTTI calls, or inventory
// methods. Only a fully verified slot loop can report a missing
// hand as empty. Ambiguous lists yield state -1 for both hands.
inline HandsSnapshot read_hands(const ReadOnlyMemory& memory, std::uint32_t hero) noexcept {
  using hand_detail::u32;
  std::array<std::uint8_t, 12> header{};
  if (!hand_detail::at_offset(memory, hero, hand_detail::slot_array_offset,
                             header.data(), header.size())) return {};
  const auto data = u32(header.data());
  const auto capacity = u32(header.data() + 4);
  const auto count = u32(header.data() + 8);
  if (count > capacity || capacity > hand_detail::slot_limit) return {};
  std::array<std::uint8_t, hand_detail::slot_limit * 4> pointers{};
  if (count && !memory.copy(data, pointers.data(), static_cast<std::size_t>(count) * 4)) return {};

  std::array<hand_detail::SlotName, hand_detail::slot_limit> seen{};
  std::uint32_t left = 0;
  std::uint32_t right = 0;
  for (std::size_t i = 0; i < count; ++i) {
    const auto slot = u32(pointers.data() + 4 * i);
    if (!slot) return {};
    for (std::size_t j = 0; j < i; ++j) if (seen[j].address == slot) return {};
    // Reads the name and vob pointer in a bounded slot prefix.
    std::array<std::uint8_t, 0x34> fields{};
    if (!memory.copy(slot, fields.data(), fields.size())) return {};
    auto& current = seen[i];
    current.address = slot;
    current.length = u32(fields.data() + 0xc);
    if (!current.length || current.length > hand_detail::name_limit ||
        !memory.copy(u32(fields.data() + 8), current.text.data(), current.length + 1) ||
        current.text[current.length] != '\0' ||
        std::memchr(current.text.data(), '\0', current.length)) return {};
    for (std::size_t j = 0; j < i; ++j)
      if (seen[j].length == current.length &&
          std::memcmp(seen[j].text.data(), current.text.data(), current.length) == 0) return {};

    // Constants also populated by EXE initialization 0x72BB20 / 0x72BB50.
    if (current.length == 11 && std::memcmp(current.text.data(), "ZS_LEFTHAND", 11) == 0)
      left = u32(fields.data() + 0x30);
    if (current.length == 12 && std::memcmp(current.text.data(), "ZS_RIGHTHAND", 12) == 0)
      right = u32(fields.data() + 0x30);
  }
  if (left && left == right) return {};
  return {hand_detail::read_object(memory, left), hand_detail::read_object(memory, right)};
}

}  // namespace ubc
