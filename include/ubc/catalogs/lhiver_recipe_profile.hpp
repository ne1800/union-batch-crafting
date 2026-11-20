// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// Own fact collection from L'Hiver DE 2.0.4, as of 2026-10-31. No
// original scripts or bytecode blocks. The hashes describe read
// areas; they do not allow claims about already executed recipes.
namespace ubc::lhiver {

using Digest = std::array<std::uint8_t, 32>;

constexpr unsigned hex_digit(char value) noexcept {
  return value >= '0' && value <= '9'   ? static_cast<unsigned>(value - '0')
         : value >= 'a' && value <= 'f' ? static_cast<unsigned>(value - 'a' + 10)
                                        : 256u;
}

template <std::size_t N> constexpr Digest digest_literal(const char (&text)[N]) {
  static_assert(N == 65, "SHA-256 braucht genau 64 Hexzeichen.");
  Digest result{};
  for (std::size_t i = 0; i < result.size(); ++i) {
    const auto high = hex_digit(text[2 * i]);
    const auto low = hex_digit(text[2 * i + 1]);
    if (high > 15 || low > 15)
      throw "Ungueltige SHA-256-Konstante";
    result[i] = static_cast<std::uint8_t>((high << 4u) | low);
  }
  return result;
}

struct Function {
  const char* name;
  std::uint32_t symbol;
  std::uint32_t byte_offset;
  std::uint32_t byte_length;
  Digest body_sha256;
  std::uint32_t argc = 0;
  std::uint32_t type = 5;
  std::uint32_t flags = 1;
  std::uint32_t return_type = 0;
  std::int32_t parent_index = -1;
};

struct SymbolBinding {
  const char* name;
  std::uint32_t symbol;
  std::uint32_t type;
  std::uint32_t flags;
  std::uint32_t elements;
  std::int32_t parent_index;
  bool check_value = false;
  std::int32_t value = 0;
  bool check_offset = false;
  std::int32_t offset = 0;
};

struct Profile {
  std::uint32_t parser_address;
  std::uint32_t symbol_count;
  std::uint32_t code_length;
  Digest code_sha256;
  const Function* functions;
  std::size_t function_count;
  const SymbolBinding* bindings;
  std::size_t binding_count;
};

inline constexpr auto archive_sha256 =
    digest_literal("b68c93dab7986b37aa3ea39a50b10e8e34400d4eb67bf46094e0f8994d157418");
inline constexpr auto dat_sha256 =
    digest_literal("71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4");
inline constexpr auto code_sha256 =
    digest_literal("3f134e4d314dc25c0f6407f8344a82747f64464d2e99a56aed2eba12400e6498");
inline constexpr std::uint32_t game_parser = 0x00ab40c0;
inline constexpr std::uint32_t symbol_count = 93005;
inline constexpr std::uint32_t code_length = 2718059;

inline constexpr std::array<Function, 12> functions{
    {{"FRYINGMEAT_S1", 21007, 825104, 65,
      digest_literal("755218d948763e83dd0668e19a1adcfb5669c737a40b537fe4d6058d8295b647")},
     {"PC_FRYINGMEAT_ADDON_INFO", 21010, 825281, 107,
      digest_literal("46429f2bf11e60c85e3012e1ba180e3c3eddbfbcc8001228e803c11186e088bc")},
     {"PC_FRYINGMEAT_ADDON_X5_INFO", 21013, 825500, 107,
      digest_literal("2082c66ed4d448c9ade3c8028a7431aa92608ab3097ec2b5d1d8f2502837737b")},
     {"PC_FRYINGMEAT_ADDON_X10_INFO", 21016, 825719, 107,
      digest_literal("faf66ccf2ccb0d4974f3fbe03ece78112117e15c6a36df690937cd59daac2b85")},
     {"PC_FRYINGMEAT_ADDON_ALL_INFO", 21019, 825938, 117,
      digest_literal("96f2ac3d3bfd65b50f8078ff216b7cbb15b735054f14e33b8ba6e5cb4477b221")},
     {"PC_FRYINGMEAT_END_INFO", 21032, 826916, 6,
      digest_literal("88e225fb843204b57529955367a14efd2314c9788ac29aff28032b3a2a8cd982")},
     {"POTIONALCHEMY_S1", 20276, 723073, 65,
      digest_literal("3ee4af5cf7c6c967d039c7585d9f4a085da7a638ee43ee02fe0c142c1f8d00ab")},
     {"PC_POTIONALCHEMY_END_INFO", 20279, 723288, 26,
      digest_literal("6a30e7f39ca34bd02e28e4e2be7c2932e1ddf2ba6c0eb69fa37542c3b74a0262")},
     {"PC_ITPO_HEALTH_01_1", 20341, 729012, 199,
      digest_literal("2c7e223e790509502e65b00f4781be32e48105aedf4e9bc4518ee5918e4f0625")},
     {"PC_ITPO_HEALTH_01_3", 20342, 729211, 219,
      digest_literal("45f0edd07cd827c4e0fc9ffe09c6f70b6a7ef64d06e976af787f3e60697055ef")},
     {"PC_ITPO_HEALTH_01_5", 20343, 729430, 219,
      digest_literal("37d19a0819ff7a81e56d58ac2512f04aefb6d74851bc439888eb430a49994394")},
     {"B_ENDPRODUCTIONDIALOG", 19965, 672156, 49,
      digest_literal("e9fcd2f7db76bab4ed7c5ceb6e5a98c3ab3b8166a3614a4dda2513092da34e64")}}};

// Order for inventory snapshots: raw, roasted, flask,
// healing herb, field knotweed, small healing potion.
inline constexpr std::array<std::int32_t, 6> item_symbols{
    {16377, 16379, 16604, 16714, 16736, 16775}};
inline constexpr std::uint32_t hero_symbol = 1683;
inline constexpr std::uint32_t self_symbol = 1679;
inline constexpr std::uint32_t production_symbol = 12330;
inline constexpr std::uint32_t aivar_symbol = 1563;
inline constexpr std::uint32_t invincible_index = 4;
inline constexpr std::int32_t cooking_context = 14;
inline constexpr std::int32_t alchemy_context = 4;

inline constexpr std::array<SymbolBinding, 19> bindings{
    {{"HERO", 1683, 7, 1, 0, 1537},
     {"SELF", 1679, 7, 1, 0, 1537},
     {"C_NPC", 1537, 4, 0, 32, -1},
     {"C_ITEM", 1584, 4, 0, 44, -1},
     {"C_NPC.AIVAR", 1563, 2, 4, 100, 1537, false, 0, true, 648},
     {"AIV_INVINCIBLE", 11998, 2, 1, 1, -1, true, 4},
     {"PLAYER_MOBSI_PRODUCTION", 12330, 2, 0, 1, -1},
     {"MOBSI_NONE", 12310, 2, 1, 1, -1, true, 0},
     {"MOBSI_CAMPFIRE", 12324, 2, 1, 1, -1, true, 14},
     {"MOBSI_POTIONALCHEMY", 12314, 2, 1, 1, -1, true, 4},
     {"ITFOMUTTONRAW", 16377, 7, 1, 0, 1584},
     {"ITFOMUTTON", 16379, 7, 1, 0, 1584},
     {"ITPL_HEALTH_HERB_01", 16714, 7, 1, 0, 1584},
     {"ITPL_TEMP_HERB", 16736, 7, 1, 0, 1584},
     {"ITMI_FLASK", 16604, 7, 1, 0, 1584},
     {"ITPO_HEALTH_01", 16775, 7, 1, 0, 1584},
     {"PLAYER_TALENT_ALCHEMY", 1464, 2, 0, 15, -1},
     {"POTION_HEALTH_01", 1448, 2, 1, 1, -1, true, 0},
     {"HEALTHSTART", 20330, 2, 0, 1, -1}}};

inline constexpr Profile profile{game_parser,     symbol_count,     code_length,
                                 code_sha256,     functions.data(), functions.size(),
                                 bindings.data(), bindings.size()};

} // namespace ubc::lhiver
