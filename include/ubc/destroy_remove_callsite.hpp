// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace ubc::destroy_remove_callsite {
inline constexpr std::uint32_t address = 0x754b92, return_address = 0x754b98;
inline constexpr std::uint32_t vtable = 0x83c714, slot = 0xc0, target = 0x70d080;
inline constexpr std::size_t prefix_size = 15, patch_size = 6;
// EXE 0655b481...: only the narrowly bounded call context, no original bodies.
inline constexpr std::array<std::uint8_t,25> original_context{{
  0x8b,0x97,0x68,0x06,0,0, 0x8d,0x8f,0x68,0x06,0,0, 0x6a,0x01,0x56,
  0xff,0x92,0xc0,0,0,0, 0x8b,0xf0,0x3b,0xf3
}};
using Context = decltype(original_context);
using PatchBytes = std::array<std::uint8_t,patch_size>;

struct Plan {
  std::uint32_t call = 0, destination = 0;
  PatchBytes bytes{};
  bool valid = false;
};

// NOP; CALL rel32 preserves the outer return address call+6. The
// original Remove function instead sees the inner adapter CALL.
inline Plan plan(std::uint32_t call, std::uint32_t destination) noexcept {
  Plan out;
  if (call < prefix_size || call > UINT32_MAX - patch_size - 4 ||
      !destination || (destination >= call-prefix_size && destination < call+patch_size+4)) return out;
  out.call = call; out.destination = destination;
  const auto delta = static_cast<std::uint32_t>(destination - (call + patch_size));
  out.bytes = {{0x90,0xe8,static_cast<std::uint8_t>(delta),
    static_cast<std::uint8_t>(delta>>8),static_cast<std::uint8_t>(delta>>16),
    static_cast<std::uint8_t>(delta>>24)}};
  out.valid = true;
  return out;
}

inline bool context_matches(const std::array<std::uint8_t,25>& bytes,
    const Plan& p, bool owned) noexcept {
  const auto canonical = plan(p.call,p.destination);
  if (!p.valid || !canonical.valid || canonical.bytes != p.bytes) return false;
  for (std::size_t i=0;i<bytes.size();++i) {
    const auto expected = owned && i>=prefix_size && i<prefix_size+patch_size ?
      p.bytes[i-prefix_size] : original_context[i];
    if (bytes[i] != expected) return false;
  }
  return true;
}

struct Invocation {
  std::uint32_t inventory = 0, loaded_vtable = 0, original = 0, caller = 0, thread = 0;
  std::int32_t instance = 0, amount = 0;
};
struct Admission {
  std::uint32_t caller = return_address, loaded_vtable = vtable, original = target, thread = 0;
};
inline bool observable(const Invocation& value, const Admission& expected) noexcept {
  return expected.thread && expected.caller && expected.loaded_vtable && expected.original &&
    !(expected.loaded_vtable & 3u) && value.thread == expected.thread &&
    value.caller == expected.caller && value.loaded_vtable == expected.loaded_vtable &&
    value.original == expected.original && value.inventory && !(value.inventory & 3u) &&
    value.instance >= 0 && value.amount == 1;
}
// An admitted call is still not its own job and not a positive debit.
inline constexpr bool native_effect_authorized = false;
} // namespace ubc::destroy_remove_callsite
