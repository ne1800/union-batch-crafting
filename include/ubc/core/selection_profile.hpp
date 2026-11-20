// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/engine/packed_inventory.hpp"

// Tight contract for offered original choices, no new release for
// VM patches or arbitrary amounts. Original conditions and effects
// continue to be executed by the engine. Library self-modification
// explicitly does not mean "full script identity confirmed".
namespace ubc::selection_profile {
using lhiver::Function;
using lhiver::SymbolBinding;
using lhiver::digest_literal;

inline constexpr std::array<Function, 6> additional_functions{{
  {"PC_FRYINGMEAT_ADDON_CONDITION", 21009, 825236, 45,
   digest_literal("2344beb9de819f9071cc97be8ceafed539b2f1771784bd8fd27f64c07b3515ec"), 0,5,3,2},
  {"PC_FRYINGMEAT_ADDON_X5_CONDITION", 21012, 825455, 45,
   digest_literal("d837c8b76e1c4f085ec2a7ceb8f23878a34f3e5538c86eb41e6dc47ca5604b11"), 0,5,3,2},
  {"PC_FRYINGMEAT_ADDON_X10_CONDITION", 21015, 825674, 45,
   digest_literal("ea66faf52228bb5433e751cd05a3011e18114acf9c5d42be04256ab622cb3990"), 0,5,3,2},
  {"PC_FRYINGMEAT_ADDON_ALL_CONDITION", 21018, 825893, 45,
   digest_literal("c800a423c37003ed0079b476d20f02168aba067c180a8ee2e8a06e8c9ba5f994"), 0,5,3,2},
  {"PC_ITPO_HEALTH_01_CONDITION", 20338, 728863, 47,
   digest_literal("7210a1a44b9deb74fed19bcdc698b5ce3e35cf1149091d769c48994a1955333c"), 0,5,3,2},
  {"PC_ITPO_HEALTH_01_INFO", 20339, 728910, 91,
   digest_literal("a1f2394a7cba7208cff17bfbac18a1c0d0208e97c78997b035b715c270cd2b1b")}
}};
inline constexpr std::array<SymbolBinding, 20> additional_bindings{{
  {"C_INFO",1649,4,0,8,-1},
  // Native field positions are checked at every info object. The DAT
  // member offsets are relative to the pd structure; no unchecked
  // runtime offset comparison against the absolute oCInfo offset.
  {"C_INFO.NPC",1650,2,4,1,1649},
  {"C_INFO.NR",1651,2,4,1,1649},
  {"C_INFO.IMPORTANT",1652,2,4,1,1649},
  {"C_INFO.CONDITION",1653,5,4,1,1649},
  {"C_INFO.INFORMATION",1654,5,4,1,1649},
  {"C_INFO.DESCRIPTION",1655,3,4,1,1649},
  {"C_INFO.TRADE",1656,2,4,1,1649},
  {"C_INFO.PERMANENT",1657,2,4,1,1649},
  {"OTHER",1680,7,1,0,1537},
  {"PC_HERO",22243,7,1,0,21074},
  {"ITMI_PAN",16607,7,1,0,1584},
  {"ITMI_PANFULL",16608,7,1,0,1584},
  {"TRUE",12297,2,1,1,-1,true,1},
  {"FALSE",12298,2,1,1,-1,true,0},
  {"PC_FRYINGMEAT_ADDON",21008,7,1,0,1649},
  {"PC_FRYINGMEAT_ADDON_X5",21011,7,1,0,1649},
  {"PC_FRYINGMEAT_ADDON_X10",21014,7,1,0,1649},
  {"PC_FRYINGMEAT_ADDON_ALL",21017,7,1,0,1649},
  {"PC_ITPO_HEALTH_01",20337,7,1,0,1649}
}};
template<class T, std::size_t N, std::size_t M>
constexpr std::array<T,N+M> append(const std::array<T,N>& a, const std::array<T,M>& b) {
  std::array<T,N+M> result{};
  for (std::size_t i=0;i<N;++i) result[i]=a[i];
  for (std::size_t i=0;i<M;++i) result[N+i]=b[i];
  return result;
}
inline constexpr auto functions = append(lhiver::functions, additional_functions);
inline constexpr auto bindings = append(lhiver::bindings, additional_bindings);
inline constexpr lhiver::Profile profile{lhiver::game_parser, lhiver::symbol_count,
  lhiver::code_length, lhiver::code_sha256, functions.data(), functions.size(),
  bindings.data(), bindings.size()};
inline constexpr std::array<PackedInventoryTarget,6> targets{{
  {"ITFOMUTTONRAW",16377}, {"ITFOMUTTON",16379}, {"ITMI_FLASK",16604},
  {"ITPL_HEALTH_HERB_01",16714}, {"ITPL_TEMP_HERB",16736}, {"ITPO_HEALTH_01",16775}
}};
struct InfoLink { std::int32_t instance, condition, function, number; };
inline constexpr std::array<InfoLink,5> info_links{{
  {21008,21009,21010,1},{21011,21012,21013,2},{21014,21015,21016,3},
  {21017,21018,21019,4},{20337,20338,20339,2}
}};
} // namespace ubc::selection_profile
