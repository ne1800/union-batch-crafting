// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_destroy_abi.hpp"
#include "ubc/engine/zbug_end_compat.hpp"

namespace ubc::cool_set_interact_path {
// Full SetInteractItem, no exception for slots supposedly unreachable
// due to nullptr. Destroy passes nullptr as the new anchor; old
// slots can still reach RemoveFromSlot/PutInInv. Ghidra12.2,
// EXE-SHA0655..., CALL74ADB6: PUSH EDI (casted oCItem), ECX=NPC.
inline constexpr auto body = cool_destroy_abi::identity_ranges[6];
inline constexpr std::uint32_t call_address = 0x74adb6;
inline constexpr std::size_t call_offset = call_address - body.address;
inline constexpr std::array<std::uint8_t, 5> original_call{{0xe8, 0x95, 0xe5, 0xff, 0xff}};
static_assert(body.address == 0x74acc0 && body.length == 564 && call_offset + 5 <= body.length);
enum class Route { none, original, zbug_put_in_inv };
enum class Issue { none, unreadable, changed, call, code, compatibility };
struct Verification {
  bool valid = false;
  Issue issue = Issue::none;
  std::uint32_t address = 0;
  Route route = Route::none;
  zbug_end_compat::Issue compatibility_issue = zbug_end_compat::Issue::none;
  // Hash of the actually read body, including the specific CALL.
  lhiver::Digest fingerprint{};
};
// Code identity only. No engine call, slot/material/end-effect evidence,
// or general hook release. Native callers hold the real module leases
// and the resting engine thread boundary across the entire recording.
// A previously stored valid-bool does not replace this new recording.
inline Verification verify(const ReadOnlyMemory& memory,
                           const zbug_end_compat::Binding& binding = {}) noexcept {
  namespace z = zbug_end_compat;
  std::array<std::uint8_t, body.length> first{}, second{};
  if (!memory.copy(body.address, first.data(), first.size()))
    return {false, Issue::unreadable, body.address};
  const auto* call = first.data() + call_offset;
  const bool original = std::memcmp(call, original_call.data(), original_call.size()) == 0;
  const auto target = call_address + 5u + z::detail::u32(call + 1);
  const bool forwarded = z::detail::cooling_forward(binding, target);
  if (!original) {
    if (call[0] != 0xe8)
      return {false, Issue::call, call_address};
    if (binding.cooling_predebit
            ? !forwarded
            : (!z::detail::binding_valid(binding) || target != binding.zbug.base + z::wrapper_rva))
      return {false, Issue::call, call_address};
  }
  const auto compatibility = [&]() noexcept {
    const auto checked = z::verify(memory, binding);
    return checked.valid && checked.call_target == target &&
                   checked.original_target == z::original_put_in_inv
               ? Verification{true}
               : Verification{false, Issue::compatibility, call_address, Route::none,
                              checked.issue};
  };
  if (!original && !forwarded) {
    const auto v = compatibility();
    if (!v.valid)
      return v;
  }
  auto normalized = first;
  // Only this previously positively bound CALL is replaced in the
  // local copy. Every other byte remains in the prior full body hash.
  if (!original)
    std::memcpy(normalized.data() + call_offset, original_call.data(), original_call.size());
  if (z::detail::digest(normalized.data(), normalized.size()) != body.sha256)
    return {false, Issue::code, body.address};
  if (!original && !forwarded) {
    const auto v = compatibility();
    if (!v.valid)
      return v;
  }
  if (!memory.copy(body.address, second.data(), second.size()))
    return {false, Issue::unreadable, body.address};
  if (first != second)
    return {false, Issue::changed, body.address};
  return {true,
          Issue::none,
          0,
          original ? Route::original : Route::zbug_put_in_inv,
          z::Issue::none,
          z::detail::digest(first.data(), first.size())};
}
} // namespace ubc::cool_set_interact_path
