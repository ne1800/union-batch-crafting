// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_destroy_observation.hpp"

namespace ubc::cool_destroy_hand_diagnostic {
// Only own values of the before_destroy boundary. No memory reader, writer
// or substitute for the unchanged cool_destroy_observation::verify contract.
inline constexpr bool execution_authorized=false;
struct Record {
  cool_destroy_observation::Stamp stamp{};
  StationRecipeHands prepared{},observed{};
  bool captured=false,emitted=false,hands_read_verified=false;
  bool profile_checked=false,profile_matches=false,generated_left_verified=false;
  std::uint32_t shape_attempted=0,shape_known=0;
  // type/mainflag/flags, then the same three original two-way branches.
  std::array<std::int32_t,6> shape{{INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN}};
};
inline std::int32_t signed_word(std::uint32_t value) noexcept {
  return value<=INT32_MAX?static_cast<std::int32_t>(value):
    -1-static_cast<std::int32_t>(UINT32_MAX-value);
}
inline bool ready(const Record& d,cool_destroy_observation::Issue issue,
    const cool_destroy_observation::Stamp& current) noexcept {
  const auto& a=d.stamp;
  return d.captured&&!d.emitted&&issue==cool_destroy_observation::Issue::generated_hand&&
    a.stable_boundary_verified&&a.native_code_verified&&a.hooks_owned&&
    current.stable_boundary_verified&&current.native_code_verified&&current.hooks_owned&&
    a.job&&a.frame&&a.serial&&a.actual_thread&&
    cool_direct_contract::detail::valid_key(a.key)&&a.actual_thread==a.key.thread&&
    cool_direct_contract::detail::same_key(a.key,current.key)&&a.job==current.job&&
    a.frame==current.frame&&a.serial==current.serial&&a.actual_thread==current.actual_thread;
}
inline std::uint32_t flags(const Record& d) noexcept {
  namespace direct=cool_direct_contract::detail;
  const auto& a=d.prepared;const auto& b=d.observed;
  return (d.captured?1u:0u)|(d.hands_read_verified?2u:0u)|
    (direct::active_hand(b)?4u:0u)|(direct::active_hand(a)?8u:0u)|
    (d.profile_checked?16u:0u)|(d.profile_matches?32u:0u)|
    (d.generated_left_verified?64u:0u)|(direct::same_hand(a,b)?128u:0u)|
    (a.hands.left.object==b.hands.left.object?256u:0u)|
    (a.left_tmp_level==b.left_tmp_level?512u:0u)|
    (a.right_tmp_level==b.right_tmp_level?1024u:0u)|
    (b.hands.left.object&&b.hands.left.object%4u==0?2048u:0u)|
    (a.interact_item==b.interact_item?4096u:0u)|
    ((d.shape_attempted&63u)<<16)|((d.shape_known&63u)<<24);
}
// Fixed selectors 1..16. Hand-object addresses are never emitted; only their
// equality is reported in flags(). Unattempted/unreadable Shape values are
// considered interpretable only together with shape_attempted/shape_known.
inline std::array<std::int32_t,16> values(const Record& d) noexcept {
  const auto& a=d.prepared;const auto& b=d.observed;
  return {{b.hands.left.state,b.hands.left.symbol,b.hands.left.amount,b.left_tmp_level,
    b.hands.right.state,b.right_tmp_level,a.hands.left.symbol,a.hands.left.amount,
    a.left_tmp_level,a.right_tmp_level,
    d.shape[0],d.shape[1],d.shape[2],d.shape[3],d.shape[4],d.shape[5]}};
}
} // namespace ubc::cool_destroy_hand_diagnostic
