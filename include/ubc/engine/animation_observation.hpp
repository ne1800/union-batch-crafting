// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/engine/hand_snapshot.hpp"
#include "ubc/stations/cool/cool_destroy_dispatch_profile.hpp"

#include <cmath>

namespace ubc::animation_observation {
using Address = std::uint32_t;

template <class T>
bool field(const ReadOnlyMemory& memory, Address base, unsigned offset, T& out) noexcept {
  return hand_detail::at_offset(memory, base, offset, &out, sizeof(out));
}

template <std::size_t N>
bool string(const ReadOnlyMemory& memory, Address base, unsigned offset, char (&out)[N]) noexcept {
  std::array<Address, 4> header{};
  if (!field(memory, base, offset, header) || header[3] >= N)
    return false;
  const auto length = header[3];
  if (!length) {
    out[0] = 0;
    return true;
  }
  return memory.copy(header[2], out, length + 1) && !out[length] && !std::memchr(out, 0, length);
}

inline constexpr std::array<const char*, 6> tags{"DEF_INSERT_ITEM", "DEF_REMOVE_ITEM",
                                                 "DEF_CREATE_ITEM", "DEF_DESTROY_ITEM",
                                                 "DEF_PLACE_ITEM",  "DEF_EXCHANGE_ITEM"};

inline const char* message_tag(const ReadOnlyMemory& m, Address message) noexcept {
  Address vtable{};
  std::uint16_t subtype{};
  return field(m, message, 0, vtable) && vtable == cool_destroy_dispatch_profile::message_vtable &&
                 field(m, message, cool_destroy_abi::message_subtype_offset, subtype) &&
                 subtype >= 6 && subtype <= 11
             ? tags[subtype - 6]
             : nullptr;
}

struct Animation {
  char name[48] = "unknown";
  int frame = -1; // Local MAN frame, independent of the source MSB frame range.
};

// zCModel::aniChannels has six slots. Fading animations may overlap the new one.
inline Animation current(const ReadOnlyMemory& m, Address model, const char* station) noexcept {
  std::array<Address, 6> channels{};
  Animation result;
  if (!field(m, model, 0x38, channels))
    return result;
  for (auto channel : channels) {
    Address prototype{};
    float frame{};
    int fading{};
    char name[48]{};
    if (!field(m, channel, 0, prototype) || !field(m, channel, 0xc, frame) ||
        !field(m, channel, 0x1c, fading) || fading || !std::isfinite(frame) || frame < 0 ||
        frame > 65535 || !string(m, prototype, 0x24, name))
      continue;
    const auto length = std::strlen(station);
    if ((name[0] != 'T' && name[0] != 'S') || name[1] != '_' ||
        std::strncmp(name + 2, station, length) || name[2 + length] != '_')
      continue;
    if (std::strspn(name, "ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789") != std::strlen(name))
      continue;
    std::memcpy(result.name, name, sizeof(name));
    result.frame = static_cast<int>(frame);
    break;
  }
  return result;
}

} // namespace ubc::animation_observation
