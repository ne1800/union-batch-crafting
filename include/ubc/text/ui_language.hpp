// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/ui_language_registry.hpp"

namespace ubc::ui_text {
namespace en = locale_en;
namespace de = locale_de;

constexpr const Table& table(Language language) noexcept {
  return *language;
}

constexpr bool option_is(std::wstring_view value, std::wstring_view lower) noexcept {
  if (value.size() != lower.size()) return false;
  for (std::size_t i = 0; i < lower.size(); ++i) {
    auto ch = value[i];
    if (ch >= L'A' && ch <= L'Z') ch += L'a' - L'A';
    if (ch != lower[i]) return false;
  }
  return true;
}

template<class DetectGerman>
Language choose(std::wstring_view value, DetectGerman german_game) noexcept {
  const auto start = value.find_first_not_of(L" \t");
  value = start == value.npos ? std::wstring_view{} :
      value.substr(start, value.find_last_not_of(L" \t") - start + 1);
  if (value.empty() || option_is(value, L"auto"))
    return german_game() ? &de::table : &en::table;
  for (const auto& locale : locales)
    if (option_is(value, locale.option)) return locale.texts;
  return &en::table;
}

constexpr Label unit(std::string_view catalog_unit) noexcept {
  if (catalog_unit == "Lose") return Label::lots;
  if (catalog_unit == "Klingen") return Label::blades;
  if (catalog_unit == "Waffen") return Label::weapons;
  return Label::batches;
}
} // namespace ubc::ui_text
