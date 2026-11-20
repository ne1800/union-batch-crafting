// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/recipe_catalog.hpp"

// Small own C_INFO metadata from DAT 71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4.
// important/trade are not assigned and must currently be null.
namespace ubc::grind_menu_metadata {
inline constexpr std::array<lhiver::Function,18> functions{{
  {"PC_SMITHSHARP_END",20937,810805,67,lhiver::digest_literal("1956ec4c6992f92626c2519840da122ff155ebde0059dfda16bfe81ee3156b8b"),0,7,1,0,1649},
  {"PC_SMITHSHARP_END_CONDITION",20938,810872,23,lhiver::digest_literal("b8cc686f9c32b20b3b5d1d6e90bf12014a2f71013961d01a62eebbf534cc6795"),0,5,3,2,-1},
  {"PC_SMITHSHARP_END_INFO",20939,810895,62,lhiver::digest_literal("00697d80b8854208cc1b6247985f37e69fcc6ec99c45f67fb1b83455cfa90af4"),0,5,1,0,-1},
  {"PC_SHARPMISC_1",20945,814721,67,lhiver::digest_literal("118bdd88fb15a272f8b03d8d86b24775483af367a30ae4a905bd39afd28e1c38"),0,7,1,0,1649},
  {"PC_SHARPMISC_1_CONDITION",20946,814788,35,lhiver::digest_literal("a4414240007e4bef601cd6a531eeb1d24cf6281769bf60f912a0e044e08bc821"),0,5,3,2,-1},
  {"PC_SHARPMISC_1_INFO",20947,814823,230,lhiver::digest_literal("eb0bf473bf01376108bf18b49f0222ef0546f02c0f48daf829c433d88b1ded88"),0,5,1,0,-1},
  {"PC_SHARPMISC_2",20948,815053,67,lhiver::digest_literal("39af052b23c93b859b6bae1dd12effa5aa12ad9c5e34fccdc75161f0ccb8251b"),0,7,1,0,1649},
  {"PC_SHARPMISC_2_CONDITION",20949,815120,35,lhiver::digest_literal("bb341ad8c6f19a2e75260bc0d74c3f6ec49b80aa33ee377f0ee66c5e04eb5389"),0,5,3,2,-1},
  {"PC_SHARPMISC_2_INFO",20950,815155,230,lhiver::digest_literal("fa3f93e0817c51ba0d0c6437340308bc904abc6474130a722863ece37b80975c"),0,5,1,0,-1},
  {"PC_SHARPMISC_3",20951,815385,67,lhiver::digest_literal("44efce7653a1effd1e97948931a8d75124505c1702d70879ca321bc7ed5a67c5"),0,7,1,0,1649},
  {"PC_SHARPMISC_3_CONDITION",20952,815452,35,lhiver::digest_literal("5370718dc04389638875722858e19448abdceb83affc597e24c09e1e7e0da8c5"),0,5,3,2,-1},
  {"PC_SHARPMISC_3_INFO",20953,815487,230,lhiver::digest_literal("28e8a200179db0cb07b52c2f16a8befca999f0a494598396523674250f56c51c"),0,5,1,0,-1},
  {"PC_SHARPMISC_4",20954,815717,67,lhiver::digest_literal("eb5ea4f69373c6f417a1fee8206e97a7e51c3707affc389f835181205502f574"),0,7,1,0,1649},
  {"PC_SHARPMISC_4_CONDITION",20955,815784,35,lhiver::digest_literal("8682b073d75a8fe118bcf5999a07e671f223e9aa3fbd30e5c11edf05e541320f"),0,5,3,2,-1},
  {"PC_SHARPMISC_4_INFO",20956,815819,210,lhiver::digest_literal("7c9963e43d2d2ccba36fec342509ce26738fdb0adca98ff50b6d2e1fa064fe1d"),0,5,1,0,-1},
  {"PC_GRIND_WEAPON",20957,816029,67,lhiver::digest_literal("3d013271a05856f38b50c18f997a8fc2a015a66eb637f1ca16a612fa0177c0f5"),0,7,1,0,1649},
  {"PC_GRIND_WEAPON_CONDITION",20958,816096,95,lhiver::digest_literal("50517f6e8cf9bf26fa8611644f5b01c03ea1aa66e5392bd963274e430ff8060d"),0,5,3,2,-1},
  {"PC_GRIND_WEAPON_INFO",20959,816191,163,lhiver::digest_literal("15df02e9224927d6e1178f88e5a03a26d3a89cb1697e156bc260f9bf8fdd4a02"),0,5,1,0,-1},
}};
inline constexpr std::array<recipe_catalog::Owner,6> owners{{
  {20937,999,22243,false,false,true,&functions[0],&functions[1],&functions[2],{}},
  {20945,990,22243,false,false,true,&functions[3],&functions[4],&functions[5],{}},
  {20948,991,22243,false,false,true,&functions[6],&functions[7],&functions[8],{}},
  {20951,992,22243,false,false,true,&functions[9],&functions[10],&functions[11],{}},
  {20954,993,22243,false,false,true,&functions[12],&functions[13],&functions[14],{}},
  {20957,994,22243,false,false,true,&functions[15],&functions[16],&functions[17],{}},
}};
} // namespace ubc::grind_menu_metadata
