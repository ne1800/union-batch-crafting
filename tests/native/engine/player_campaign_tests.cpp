// SPDX-License-Identifier: MIT
#include "ubc/engine/player_campaign.hpp"
#include <cstring>

namespace pc = ubc::player_campaign;
#if defined(UBC_PLAYER_RELEASE) && !defined(UBC_ENABLE_DNDR)
#error The player package must support Night of the Raven
#endif
int main() {
  char path[128]{}, too_short[8]{};
  if (!pc::dat_path(R"(S:\GAME)", R"(\_WORK\DATA\SCRIPTS\_COMPILED\)", R"(CONTENT\GOTHIC.SRC)",
                    path) ||
      std::string_view(path) != R"(_WORK\DATA\SCRIPTS\_COMPILED\GOTHIC.DAT)" ||
      pc::dat_path(R"(S:\GAME)", R"(\\server\share\scripts)", "GOTHIC.SRC", path))
    return 6;
  if (!pc::dat_path("C:\\Gothic\\", "c:\\GOTHIC\\_Work\\Scripts\\", "Content\\Gothic.src", path) ||
      std::string_view(path) != "_Work\\Scripts\\Gothic.DAT" ||
      !pc::dat_path("D:\\Game", "D:\\Game\\Custom", "Other.src", path) ||
      std::string_view(path) != "Custom\\Other.DAT" ||
      pc::dat_path("C:\\Gothic", "C:\\Gothic2\\Scripts", "Other.src", path) ||
      pc::dat_path("C:\\Gothic", "C:\\Gothic\\Scripts", "Other.src", too_short))
    return 5;
  if (pc::mode(pc::Campaign::dndr, false) != pc::Mode::crafting ||
      pc::mode(pc::Campaign::dndr, true) != pc::Mode::crafting)
    return 7;
  const auto wrong_dat = pc::identify(ubc::dndr::dat_size, [](void* buffer, long size) {
    std::memset(buffer, 0, static_cast<std::size_t>(size));
    return size;
  });
  if (wrong_dat != pc::Campaign::unknown || pc::mode(wrong_dat, false) != pc::Mode::idle ||
      pc::mode(wrong_dat, true) != pc::Mode::notice)
    return 8;
  constexpr std::string_view dat = "abc";
  const auto expected = ubc::lhiver::digest_literal(
      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  const auto check = [&](std::string_view input, std::wstring_view ini) {
    const bool match = pc::matches(
        static_cast<long>(input.size()),
        [&](void* buffer, long size) {
          std::memcpy(buffer, input.data(), static_cast<std::size_t>(size));
          return size;
        },
        3, expected);
    return pc::mode(match ? pc::Campaign::marcello : pc::Campaign::unknown, pc::marcello_ini(ini));
  };
  if (check("abd", L"GothicGame.ini") != pc::Mode::idle ||
      check("abd", L"Other_Lhiver_marcello_de.ini") != pc::Mode::idle ||
      check("abd", L"Lhiver_marcello_de.ini.old") != pc::Mode::idle)
    return 1;
  if (check("abd", L"2847787799_LHIVER_MARCELLO_DE.INI") != pc::Mode::notice ||
      check("abd", L"Lhiver_marcello_de") != pc::Mode::notice ||
      check("", L"System\\Lhiver_marcello_de.ini") != pc::Mode::notice)
    return 2;
  if (check(dat, L"Lhiver_marcello_de.ini") != pc::Mode::crafting ||
      check(dat, L"GothicGame.ini") != pc::Mode::crafting)
    return 3;
  return pc::matches(3, [](void*, long size) { return size - 1; }, 3, expected) ? 4 : 0;
}
