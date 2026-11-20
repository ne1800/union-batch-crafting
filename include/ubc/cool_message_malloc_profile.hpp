// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/alchemy_extra_dependencies.hpp"

// Metadata only from pinned original files and exported Ghidra evidence.
// Entire malloc function with cold blocks; no original bodies in source.
namespace ubc::cool_message_malloc::profile {
inline constexpr bool runtime_ready=false;
inline constexpr std::uint32_t preferred_base=0x78aa0000,crt_size=0xbe000;
inline constexpr std::uint32_t heap_handle=0xb38b0,new_mode=0xb45f4,heap_alloc_iat=0x11f8;
inline constexpr std::uint32_t allocation_site=0x6dbc0b,call=0x6dbc10,return_address=0x6dbc15,size=0x98;
inline constexpr std::uint32_t malloc_entry=0x10233,maximum_size=0xffffffe0;
inline constexpr auto engine_sha256=lhiver::digest_literal("0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f");
inline constexpr std::array<alchemy_flask_runtime::CodeRange,4> engine_code{{
  {0x6dbb10,356,lhiver::digest_literal("95b486f8db8e92db577245637f6cd4c7b00a7e7a0b167c0fb480f9d2aef41d61"),alchemy_flask_runtime::RangeKind::complete_function},
  {0x565f50,5,lhiver::digest_literal("9eb6ea5f968745c77cd4e7624ae2878903391e3a0d16579297616a8080d0c9fa"),alchemy_flask_runtime::RangeKind::reviewed_site},
  {0x7b4460,5,lhiver::digest_literal("8123416c083ef1c0cb908278f350eed53af95b468ef4d39318e1ae15988e9687"),alchemy_flask_runtime::RangeKind::reviewed_site},
  {0x7b44c6,6,lhiver::digest_literal("46b1394bc3595afe5c054a75df7ed43b34658481ea69a602576af5937a31053d"),alchemy_flask_runtime::RangeKind::reviewed_site},
}};
inline constexpr std::array<std::uint16_t,3> reloc_0{{0x16,0x2c,0x32}};
inline constexpr std::array<std::uint16_t,0> reloc_1{{}};
inline constexpr std::array<std::uint16_t,1> reloc_2{{0x1f}};
inline constexpr std::array<zbug_end_compat::CodeRange,3> malloc_code{{
  {0x10233,71,lhiver::digest_literal("cad07997210b966c03fc47f54b0ecc0db0bc4fcccce466104662644cfa67d72d"),reloc_0.data(),reloc_0.size()},
  {0x102bc,5,lhiver::digest_literal("6323c89db8b78a88c2a1f8b07bf14e31178a0c76af74161354f1b030a3ec762d"),reloc_1.data(),reloc_1.size()},
  {0x3f22f,97,lhiver::digest_literal("3ba9a9419a0c5c8f388df0e7ee1f2562e6bdc4f4510375e5c457b94a68e092ae"),reloc_2.data(),reloc_2.size()},
}};
} // namespace ubc::cool_message_malloc::profile
