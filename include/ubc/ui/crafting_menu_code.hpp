// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/core/sha256.hpp"
#include "ubc/engine/native_menu_abi.hpp"
#include "ubc/engine/read_only_memory.hpp"
#include "ubc/engine/selection_abi.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace ubc::crafting_ui {

enum class CodeMatch { mismatch, original, alternative };

// Complete function bodies verified statically against Gothic II AddOn 2.6fix
// (SHA-256 0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f).
// Every engine routine called during menu open, language lookup, backdrop
// setup or zSTRING construction is checked in loaded memory before first use.
struct CodeSite {
  std::uintptr_t address;
  std::size_t length;
  lhiver::Digest digest;
  const char* mismatch;
  bool has_alternative = false;
  lhiver::Digest alternative{};

  CodeMatch match_digest(const lhiver::Digest& actual) const noexcept {
    if (actual == digest)
      return CodeMatch::original;
    if (has_alternative && actual == alternative)
      return CodeMatch::alternative;
    return CodeMatch::mismatch;
  }
};

inline constexpr auto string_ctor_digest =
    lhiver::digest_literal("7e3de453ee39a4b95d022477abd1dc90b6503d480bf58419836533e08f936d1c");
inline constexpr auto string_dtor_digest =
    lhiver::digest_literal("f9a8f22b98a5b3a5806030de83a907cd59cd9fc188f88a7c5ecbcc8e44132ace");
inline constexpr auto parser_get_index_digest =
    lhiver::digest_literal("036f2567c8ccc2ec2468c7660f4ded761faf8705cabcb071208eb16b3ec1bfbe");
inline constexpr auto menu_get_parser_digest =
    lhiver::digest_literal("d901f956609c7e87600d5dc0e9f73cb8970266357c5292e0f665827e6a9f9810");
inline constexpr auto view_set_alpha_digest =
    lhiver::digest_literal("b4f3c0e5997d991ad194c8fd1a9871303aefe560b0803af4a518d7cd41634389");
inline constexpr auto view_insert_back_digest =
    lhiver::digest_literal("a4ba95d10af5a714014d8f95036e6656369cf4485602bb3677f5e2d786ca63e1");
inline constexpr auto view_set_blend_digest =
    lhiver::digest_literal("8b1870c33b0980ec04fdb4f6ecc447d847f94bd7810e63593fe5a451ca96a946");
inline constexpr auto view_blit_digest =
    lhiver::digest_literal("da926dc025f49cbe9e20a4c62a0a401bbf71335c5541779f2923fbc649090c3c");
// Union 1.0m corrects four viewport edges and selects farZ 65534 in Blit.
// Both complete bodies draw the same tile before the same text call.
inline constexpr auto union_blit_digest =
    lhiver::digest_literal("996de8ea410b89b6298273c34c0b256ff38aa3275c07f1f6c6de1832b337f2f7");
// Observed in loaded ScreenInit code with the full L'Hiver Workshop stack.
// The plugin responsible for the patch still needs to be identified in a live run.
inline constexpr auto workshop_screen_init_digest =
    lhiver::digest_literal("866a9f319a041c92bd4f6cf4d59ea5b89c27c558c243a08da8aca7334e785de1");

inline constexpr std::array<CodeSite, 2> string_sites{{
    {native_menu::string_ctor, 127, string_ctor_digest, "ui_string_ctor_mismatch"},
    {native_menu::string_dtor, 67, string_dtor_digest, "ui_string_dtor_mismatch"},
}};

inline constexpr std::array<CodeSite, 3> language_sites{{
    {native_menu::string_ctor, 127, string_ctor_digest, "ui_string_ctor_mismatch"},
    {native_menu::string_dtor, 67, string_dtor_digest, "ui_string_dtor_mismatch"},
    {native_menu::parser_get_index, 177, parser_get_index_digest, "ui_parser_get_index_mismatch"},
}};

inline constexpr std::array<CodeSite, 1> menu_parser_sites{{
    {native_menu::menu_get_parser, 6, menu_get_parser_digest, "ui_menu_get_parser_mismatch"},
}};

// Verified before NativeSelectionUi::open executes zCMenu::zCMenu, ScreenInit
// (which calls SetTransparency at 0x4dcfa5 and InsertBack at 0x4dd123),
// read_language (zSTRING ctor/dtor and zCParser::GetIndex), or teardown
// (ScreenDone and ~zCMenu).
inline constexpr std::array<CodeSite, 9> open_sites{{
    {native_menu::menu_ctor, 384,
     lhiver::digest_literal("0deaefd5cb334e29422656b4118831371bd0b5020b3eaa6f3b180d703240513d"),
     "ui_menu_ctor_mismatch"},
    {native_menu::menu_dtor, 508,
     lhiver::digest_literal("a1700d6b94a56ec95a7f93eda8f4d29e2a0822113300686a1c7fc6bcd7e570d5"),
     "ui_menu_dtor_mismatch"},
    {selection_abi::menu_screen_init, 3066,
     lhiver::digest_literal("a4a67a2b4b81506fe0d3e93a85a14abc5f58f082e2751802474730a23d85e86d"),
     "ui_screen_init_mismatch", true, workshop_screen_init_digest},
    {selection_abi::menu_screen_done, 573,
     lhiver::digest_literal("225dd5b3688330703b26e6f548a60a9845fa757863e52cc5cc05907afa69c0b8"),
     "ui_screen_done_mismatch"},
    {native_menu::view_set_alpha, 10, view_set_alpha_digest, "ui_screen_init_alpha_mismatch"},
    {native_menu::view_insert_back, 253, view_insert_back_digest, "ui_screen_init_insert_mismatch"},
    {native_menu::string_ctor, 127, string_ctor_digest, "ui_string_ctor_mismatch"},
    {native_menu::string_dtor, 67, string_dtor_digest, "ui_string_dtor_mismatch"},
    {native_menu::parser_get_index, 177, parser_get_index_digest, "ui_parser_get_index_mismatch"},
}};

inline constexpr std::array<CodeSite, 4> backdrop_sites{{
    {native_menu::view_insert_back, 253, view_insert_back_digest, "ui_backdrop_insert_mismatch"},
    {native_menu::view_set_blend, 12, view_set_blend_digest, "ui_backdrop_blend_mismatch"},
    {native_menu::view_set_alpha, 10, view_set_alpha_digest, "ui_backdrop_alpha_mismatch"},
    {native_menu::view_blit, 899, view_blit_digest, "ui_backdrop_blit_mismatch", true,
     union_blit_digest},
}};

inline const char* verify_code(ReadOnlyMemory memory, const CodeSite* sites, std::size_t count,
                               bool* workshop_screen_init = nullptr) noexcept {
  if (workshop_screen_init)
    *workshop_screen_init = false;
  bool workshop = false;
  if (!sites && count)
    return "ui_code_sites_invalid";
  for (std::size_t i = 0; i < count; ++i) {
    const auto& site = sites[i];
    if (!site.length || site.address > UINT32_MAX || site.length > UINT32_MAX - site.address)
      return site.mismatch;
    Sha256 hash;
    std::array<std::uint8_t, 256> chunk{};
    std::size_t remaining = site.length;
    auto current = static_cast<std::uint32_t>(site.address);
    while (remaining) {
      const auto step = remaining < chunk.size() ? remaining : chunk.size();
      if (!memory.copy(current, chunk.data(), step))
        return site.mismatch;
      hash.update(chunk.data(), step);
      current += static_cast<std::uint32_t>(step);
      remaining -= step;
    }
    const auto match = site.match_digest(hash.finish());
    if (match == CodeMatch::mismatch)
      return site.mismatch;
    if (site.address == selection_abi::menu_screen_init)
      workshop = match == CodeMatch::alternative;
  }
  if (workshop_screen_init)
    *workshop_screen_init = workshop;
  return nullptr;
}

template <std::size_t N>
inline const char* verify_code(ReadOnlyMemory memory,
                               const std::array<CodeSite, N>& sites) noexcept {
  return verify_code(memory, sites.data(), sites.size());
}

inline const char* verify_open(ReadOnlyMemory memory,
                               bool* workshop_screen_init = nullptr) noexcept {
  return verify_code(memory, open_sites.data(), open_sites.size(), workshop_screen_init);
}

inline const char* verify_backdrop(ReadOnlyMemory memory) noexcept {
  return verify_code(memory, backdrop_sites);
}

inline const char* verify_strings(ReadOnlyMemory memory) noexcept {
  return verify_code(memory, string_sites);
}

inline const char* verify_language(ReadOnlyMemory memory) noexcept {
  return verify_code(memory, language_sites);
}

inline const char* verify_menu_parser(ReadOnlyMemory memory) noexcept {
  return verify_code(memory, menu_parser_sites);
}

} // namespace ubc::crafting_ui
