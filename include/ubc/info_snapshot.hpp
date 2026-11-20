// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/read_only_memory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ubc {

// valid describes exclusively readable, unique native metadata. Names, symbol
// types, and recipe availability are checked separately by the script profile.
struct InfoSnapshot {
  bool valid = false;
  std::int32_t instance = -1;
  std::int32_t function = -1;
};

struct ChoiceSnapshot {
  bool valid = false;
  InfoSnapshot info{};
  std::int32_t function = -1;
  std::int32_t index = -1; // Zero-based position in the native choice list.
  std::uint32_t nodes = 0;
};

// Observation limit, not a claim of engine restriction. Larger lists
// remain unobserved; the caller still passes the original through.
inline constexpr std::uint32_t max_info_choices = 256;

namespace info_detail {

// Read-only verification against Gothic2.exe with SHA-256
// 0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f:
// Info constructor 0x703409 writes vptr 0x83C44C. Info() 0x703970
// reads +0x2C; GetInstance 0x7036E0 reads +0x50. +0x4C is instead told.
// OnChoice(int), 0x6627D9..0x66280A, reads manager+0x28 -> info+0x58
// and node {choice,next}. OnChoice(choice), 0x662A19, reads choice+0x14.
inline constexpr std::uint32_t info_vtable = 0x83c44c;
inline constexpr std::size_t info_size = 0x5c;
inline constexpr std::size_t choice_size = 0x18;

inline std::uint32_t u32(const std::uint8_t* bytes) noexcept {
  return std::uint32_t{bytes[0]} | (std::uint32_t{bytes[1]} << 8) |
         (std::uint32_t{bytes[2]} << 16) | (std::uint32_t{bytes[3]} << 24);
}

inline bool symbol_index(std::uint32_t value) noexcept {
  return value <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
}

struct InfoRecord {
  InfoSnapshot metadata{};
  std::uint32_t first_node = 0;
};

inline InfoRecord read_record(const ReadOnlyMemory& memory, std::uint32_t address) noexcept {
  std::array<std::uint8_t, info_size> bytes{};
  if (!memory.copy(address, bytes.data(), bytes.size()) || u32(bytes.data()) != info_vtable) return {};
  const auto function = u32(bytes.data() + 0x2c);
  const auto instance = u32(bytes.data() + 0x50);
  if (!symbol_index(function) || !symbol_index(instance)) return {};
  return {{true, static_cast<std::int32_t>(instance), static_cast<std::int32_t>(function)},
          u32(bytes.data() + 0x58)};
}

inline bool read_owner(const ReadOnlyMemory& memory, std::uint32_t manager,
                       std::uint32_t& owner) noexcept {
  constexpr std::uint32_t offset = 0x28;
  if (!manager || offset > std::numeric_limits<std::uint32_t>::max() - manager) return false;
  std::array<std::uint8_t, 4> bytes{};
  if (!memory.copy(manager + offset, bytes.data(), bytes.size())) return false;
  owner = u32(bytes.data());
  return owner != 0;
}

} // namespace info_detail

// Use only on the stable engine thread and after independent EXE
// verification. No engine calls, profile checks, or game state changes.
inline InfoSnapshot read_info(const ReadOnlyMemory& memory, std::uint32_t info) noexcept {
  return info_detail::read_record(memory, info).metadata;
}

// Execute before the original OnChoice and retain the result as a value.
// The original may delete Choices and list nodes (RemoveAllChoices,
// 0x703DB6/0x703DBC); do not read the old choice pointer again.
// The entire list must remain valid, even after the found entry.
inline ChoiceSnapshot read_choice(const ReadOnlyMemory& memory, std::uint32_t manager,
                                  std::uint32_t choice) noexcept {
  if (!choice) return {};
  std::uint32_t owner = 0;
  if (!info_detail::read_owner(memory, manager, owner)) return {};
  const auto info = info_detail::read_record(memory, owner);
  if (!info.metadata.valid) return {};

  std::array<std::uint32_t, max_info_choices> seen_nodes{};
  std::array<std::uint32_t, max_info_choices> seen_choices{};
  std::uint32_t count = 0;
  std::int32_t selected_index = -1;
  std::int32_t selected_function = -1;
  auto node = info.first_node;
  while (node) {
    if (count == max_info_choices) return {};
    for (std::uint32_t i = 0; i < count; ++i)
      if (seen_nodes[i] == node) return {};
    std::array<std::uint8_t, 8> links{};
    if (!memory.copy(node, links.data(), links.size())) return {};
    const auto data = info_detail::u32(links.data());
    if (!data) return {};
    for (std::uint32_t i = 0; i < count; ++i)
      if (seen_choices[i] == data) return {};
    std::array<std::uint8_t, info_detail::choice_size> fields{};
    if (!memory.copy(data, fields.data(), fields.size())) return {};
    const auto function = info_detail::u32(fields.data() + 0x14);
    if (!info_detail::symbol_index(function)) return {};
    if (data == choice) {
      selected_index = static_cast<std::int32_t>(count);
      selected_function = static_cast<std::int32_t>(function);
    }
    seen_nodes[count] = node;
    seen_choices[count] = data;
    ++count;
    node = info_detail::u32(links.data() + 4);
  }
  if (selected_index < 0) return {};
  return {true, info.metadata, selected_function, selected_index, count};
}

} // namespace ubc
