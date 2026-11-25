// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/catalogs/dndr_alchemy_profile.hpp"
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/core/sha256.hpp"
#include <algorithm>
#include <string_view>

namespace ubc::player_campaign {
enum class Mode { idle, notice, crafting };

enum class Campaign { unknown, marcello, dndr };

template <class Char> constexpr Char folded(Char c) noexcept {
  if (c == '/')
    return static_cast<Char>('\\');
  return c >= 'A' && c <= 'Z' ? static_cast<Char>(c + ('a' - 'A')) : c;
}
inline bool equal_name(std::wstring_view a, std::wstring_view b) noexcept {
  return a.size() == b.size() &&
         std::equal(a.begin(), a.end(), b.begin(),
                    [](wchar_t x, wchar_t y) { return folded(x) == folded(y); });
}

template <std::size_t N>
bool dat_path(std::string_view root, std::string_view directory, std::string_view source,
              char (&result)[N]) noexcept {
  while (!root.empty() && folded(root.back()) == '\\')
    root.remove_suffix(1);
  if (root.empty() || directory.empty())
    return false;
  if (directory.size() > root.size() && folded(directory[root.size()]) == '\\' &&
      std::equal(root.begin(), root.end(), directory.begin(),
                 [](char a, char b) { return folded(a) == folded(b); })) {
    directory.remove_prefix(root.size() + 1);
  } else if (directory.size() > 1 && folded(directory[0]) == '\\' && folded(directory[1]) != '\\') {
    // GetDirString also returns directories rooted in the VDFS namespace.
    directory.remove_prefix(1);
  } else {
    return false;
  }
  while (!directory.empty() && folded(directory.back()) == '\\')
    directory.remove_suffix(1);
  source.remove_prefix(source.find_last_of("/\\") + 1);
  const auto stem = source.substr(0, source.find_last_of('.'));
  if (stem.empty() || directory.size() + stem.size() + 6 > N)
    return false;
  auto* pos = std::replace_copy(directory.begin(), directory.end(), result, '/', '\\');
  if (!directory.empty())
    *pos++ = '\\';
  pos = std::copy(stem.begin(), stem.end(), pos);
  std::copy_n(".DAT", 5, pos);
  return true;
}

inline std::wstring_view basename(std::wstring_view path) noexcept {
  return path.substr(path.find_last_of(L"/\\") + 1);
}

inline bool marcello_ini(std::wstring_view ini) noexcept {
  auto name = basename(ini);
  if (name.size() >= 4 && equal_name(name.substr(name.size() - 4), L".ini"))
    name.remove_suffix(4);
  return equal_name(name, L"Lhiver_marcello_de") ||
         equal_name(name, L"2847787799_Lhiver_marcello_de");
}

inline Mode mode(Campaign campaign, bool marcello_selected) noexcept {
  if (campaign == Campaign::marcello || campaign == Campaign::dndr)
    return Mode::crafting;
  return marcello_selected ? Mode::notice : Mode::idle;
}

// Hash the compiled DAT independently of changes made to the running VM.
template <class Read>
bool matches(long size, Read read, long expected_size, const lhiver::Digest& expected) noexcept {
  if (size != expected_size || size <= 0)
    return false;
  Sha256 hash;
  std::array<std::uint8_t, 16384> buffer{};
  while (size > 0) {
    const auto count = (std::min)(size, static_cast<long>(buffer.size()));
    if (read(buffer.data(), count) != count)
      return false;
    hash.update(buffer.data(), static_cast<std::size_t>(count));
    size -= count;
  }
  return hash.finish() == expected;
}
template <class Read> Campaign identify(long size, Read read) noexcept {
  if (size == 9327865 && matches(size, read, 9327865, lhiver::dat_sha256))
    return Campaign::marcello;
  if (size == dndr::dat_size && matches(size, read, dndr::dat_size, dndr::dat_digest))
    return Campaign::dndr;
  return Campaign::unknown;
}
} // namespace ubc::player_campaign
