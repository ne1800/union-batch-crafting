// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/text/ui_language.hpp"
#include "ubc/engine/native_menu_abi.hpp"
#include "ubc/engine/runtime_script.hpp"
#include "ubc/engine/windows_probe.hpp"

namespace ubc::ui_text {
namespace language_detail {
namespace vm = runtime_script::detail;
inline constexpr ReadOnlyMemory memory{nullptr, windows_probe::copy_memory};

inline bool string_equals(const vm::Symbol& symbol, std::string_view expected) noexcept {
  if (symbol.type != 3 || symbol.elements != 1 || symbol.flags != 1 || symbol.parent)
    return false;
  std::array<std::uint8_t, 20> header{};
  std::array<char, 32> bytes{};
  return expected.size() < bytes.size() && memory.copy(symbol.data, header.data(), header.size()) &&
         vm::u32(header.data() + 12) == expected.size() &&
         memory.copy(vm::u32(header.data() + 8), bytes.data(), expected.size() + 1) &&
         bytes[expected.size()] == 0 && std::string_view(bytes.data(), expected.size()) == expected;
}

inline bool german_script(bool main_menu) noexcept {
  // The standalone control probe opens before the game scripts have loaded.
  // Its own menu parser already contains the localized load and save labels.
  using GetMenuParser = std::uint32_t(__cdecl*)();
  const auto parser =
      main_menu ? reinterpret_cast<GetMenuParser>(0x004DB130)() : lhiver::game_parser;
  lhiver::Profile profile{};
  profile.parser_address = parser;
  vm::Layout layout{};
  if (!parser || vm::read_layout(memory, parser, profile, layout) != runtime_script::Issue::none)
    return false;
  vm::Symbol symbol{};
  if (main_menu) {
    bool load = false, save = false;
    if (layout.count > 16384)
      return false;
    for (std::uint32_t i = 0; i < layout.count && !(load && save); ++i) {
      if (vm::lookup(memory, layout, i, symbol) != runtime_script::Issue::none)
        return false;
      load = load || string_equals(symbol, "Spiel laden");
      save = save || string_equals(symbol, "Spiel speichern");
    }
    return load && save;
  }

  // Gothic II AddOn zCParser::GetIndex resolves the named constant in the
  // loaded game, including mods which move it to a different symbol index.
  constexpr char name[] = "PRINT_NOTENOUGHGOLD";
  native_menu::StringStorage key{};
  reinterpret_cast<native_menu::StringConstructor>(native_menu::string_ctor)(key.bytes.data(),
                                                                             name);
  using GetIndex = int(__thiscall*)(void*, const void*);
  const auto index =
      reinterpret_cast<GetIndex>(0x00793470)(reinterpret_cast<void*>(parser), key.bytes.data());
  reinterpret_cast<native_menu::ObjectFunction>(native_menu::string_dtor)(key.bytes.data());
  return index >= 0 &&
         vm::lookup(memory, layout, static_cast<std::uint32_t>(index), symbol) ==
             runtime_script::Issue::none &&
         vm::name_matches(memory, symbol, name) == runtime_script::Issue::none &&
         string_equals(symbol, "Zu wenig Gold.");
}
} // namespace language_detail

template <class DetectGerman> Language configured_language(DetectGerman german_game) noexcept {
  wchar_t path[32768]{}, option[32]{};
  if (windows_probe::sibling_path(nullptr, path, L"Gothic.INI")) {
    const auto count =
        GetPrivateProfileStringW(L"UNIONBATCHCRAFTING", L"Language", L"auto", option, 32, path);
    if (count == 31)
      return &en::table;
  }
  return choose(option, german_game);
}

inline Language read_language(bool main_menu = false) noexcept {
  return configured_language([main_menu] { return language_detail::german_script(main_menu); });
}
} // namespace ubc::ui_text
