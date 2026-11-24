// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/engine/packed_inventory.hpp"

namespace ubc::dndr {
struct Item {
  PackedInventoryTarget target;
  int localized_symbol;
};
inline constexpr std::array<Item, 61> items{{
    {{"ITFO_ADDON_RUM", 6117}, 15673},
    {{"ITFO_ADDON_LOUSHAMMER", 6122}, 15678},
    {{"ITFO_ADDON_SCHLAFHAMMER", 6124}, 15680},
    {{"ITFO_ADDON_SCHNELLERHERING", 6126}, 15736},
    {{"ITFO_ADDON_PFEFFER_01", 6129}, 15682},
    {{"ITMI_ADDON_JOINT_01", 6149}, 15704},
    {{"ITPO_ADDON_GEIST_01", 6177}, 15739},
    {{"ITPO_ADDON_GEIST_02", 6179}, 15741},
    {{"ITPO_HEALTH_ADDON_04", 6181}, 15743},
    {{"ITPO_MANA_ADDON_04", 6183}, 15745},
    {{"ITAT_STING", 6546}, 16130},
    {{"ITAT_SHARKTEETH", 6559}, 16143},
    {{"ITFO_FISH", 6717}, 16375},
    {{"ITMI_FLASK", 6906}, 16604},
    {{"ITMI_JOINT", 6922}, 16620},
    {{"ITMI_SULFUR", 6948}, 16652},
    {{"ITMI_DARKPEARL", 6955}, 16659},
    {{"ITPL_BEET", 6997}, 16704},
    {{"ITPL_SWAMPHERB", 6999}, 16706},
    {{"ITPL_MANA_HERB_01", 7001}, 16708},
    {{"ITPL_MANA_HERB_02", 7003}, 16710},
    {{"ITPL_MANA_HERB_03", 7005}, 16712},
    {{"ITPL_HEALTH_HERB_01", 7007}, 16714},
    {{"ITPL_HEALTH_HERB_02", 7009}, 16716},
    {{"ITPL_HEALTH_HERB_03", 7011}, 16718},
    {{"ITPL_DEX_HERB_01", 7013}, 16720},
    {{"ITPL_STRENGTH_HERB_01", 7015}, 16722},
    {{"ITPL_SPEED_HERB_01", 7017}, 16724},
    {{"ITPL_TEMP_HERB", 7029}, 16736},
    {{"ITPL_PERM_HERB", 7031}, 16738},
    {{"ITPO_MANA_01", 7059}, 16766},
    {{"ITPO_MANA_02", 7061}, 16769},
    {{"ITPO_MANA_03", 7063}, 16772},
    {{"ITPO_HEALTH_01", 7065}, 16775},
    {{"ITPO_HEALTH_02", 7067}, 16778},
    {{"ITPO_HEALTH_03", 7069}, 16781},
    {{"ITPO_PERM_STR", 7071}, 16784},
    {{"ITPO_PERM_DEX", 7073}, 16787},
    {{"ITPO_PERM_HEALTH", 7075}, 16790},
    {{"ITPO_PERM_MANA", 7077}, 16794},
    {{"ITPO_SPEED", 7079}, 16798},
    {{"ITPO_MEGADRINK", 7081}, 16800},
    {{"ITAT_DRAGONEGG_MIS", 7676}, 17585},
    {{"ITAT_DRAGONBLOOD", 6572}, 16156},
    {{"ITMW_1H_MACE_L_04", 6767}, 16435},
    {{"ITMW_1H_COMMON_01", 6805}, 16477},
    {{"ITMW_SCHWERT1", 6811}, 16483},
    {{"ITMW_SCHWERT4", 6817}, 16491},
    {{"ITMW_1H_SPECIAL_01", 6818}, 16492},
    {{"ITMW_2H_SPECIAL_01", 6819}, 16494},
    {{"ITMW_RUBINKLINGE", 6821}, 16497},
    {{"ITMW_1H_SPECIAL_02", 6830}, 16508},
    {{"ITMW_2H_SPECIAL_02", 6831}, 16510},
    {{"ITMW_ELBASTARDO", 6832}, 16512},
    {{"ITMW_1H_SPECIAL_03", 6842}, 16522},
    {{"ITMW_2H_SPECIAL_03", 6843}, 16524},
    {{"ITMW_1H_SPECIAL_04", 6846}, 16528},
    {{"ITMW_2H_SPECIAL_04", 6847}, 16530},
    {{"ITMISWORDRAW", 6912}, 16610},
    {{"ITMISWORDRAWHOT", 6913}, 16611},
    {{"ITMI_NUGGET", 6928}, 16626},
}};
inline constexpr std::size_t item_index(int symbol) noexcept {
  for (std::size_t i = 0; i < items.size(); ++i)
    if (items[i].target.symbol == symbol)
      return i;
  return items.size();
}
} // namespace ubc::dndr
