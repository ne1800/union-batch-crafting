// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/station_recipe_catalog.hpp"

// Own metadata from the pinned L'Hiver-DAT, no script bytes.
namespace ubc::cool_recipe_catalog {
using lhiver::digest_literal;
using lhiver::Function;
using lhiver::SymbolBinding;
using recipe_catalog::Owner;
using recipe_catalog::Requirement;
using recipe_catalog::RequirementKind;
using recipe_catalog::RequirementRole;
using recipe_catalog::view;
enum class RecipeId : std::uint32_t { blade = 0xa001 };
inline constexpr bool runtime_ready = false;
inline constexpr std::array<Function, 15> functions{{
    {"SMITHCOOL_S1", 20769, 792307, 65,
     digest_literal("e4d75a6c5d55df44327c882c10d40f51458a173903865ff068b87bd7fc6ccaee"), 0, 5, 1, 0,
     -1},
    {"PC_SMITHCOOL_END", 20770, 792372, 67,
     digest_literal("c702fad65101ab9b5ca6f4de29276614d1b848e3c29b9be9a27af77d91e7d662"), 0, 7, 1, 0,
     1649},
    {"PC_SMITHCOOL_END_CONDITION", 20771, 792439, 23,
     digest_literal("68b7e200e07539dd654af50a5511d41da3c95e48c65a53df46d07581108788b9"), 0, 5, 3, 2,
     -1},
    {"PC_SMITHCOOL_END_INFO", 20772, 792462, 26,
     digest_literal("4fff593be944917f948e681327afbfd8cf10231ca9b249204ecbdf0768ec2f88"), 0, 5, 1, 0,
     -1},
    {"PC_SMITHCOOL_ADDON", 20773, 792488, 67,
     digest_literal("cda3d36ab6509c1e7821102a59fe35f8804c12a65ee37bd9b67f559c9715f1cd"), 0, 7, 1, 0,
     1649},
    {"PC_SMITHCOOL_ADDON_CONDITION", 20774, 792555, 45,
     digest_literal("d28a841e303e9c8fc0daf8cd2fbe063f5ee0fbb06feab5c641d993c82990f1c7"), 0, 5, 3, 2,
     -1},
    {"PC_SMITHCOOL_ADDON_INFO", 20775, 792600, 77,
     digest_literal("282be9c8faf87de0727b76a426c7c21b898548691585ee68200ec5ead44762e7"), 0, 5, 1, 0,
     -1},
    {"PC_SMITHCOOL_ADDON3", 20776, 792677, 67,
     digest_literal("7a8f0a0b9fb6930b4f603d000d06c2bf760b8930f05950e55f96cab982278ec4"), 0, 7, 1, 0,
     1649},
    {"PC_SMITHCOOL_ADDON3_CONDITION", 20777, 792744, 45,
     digest_literal("9baa6a53f7cc56779aa42b9a00c982080d87ee9fcbe88d47baa8023c060ee990"), 0, 5, 3, 2,
     -1},
    {"PC_SMITHCOOL_ADDON3_INFO", 20778, 792789, 97,
     digest_literal("13a78c83eb90d08c4e7e279323be9bbbbe53c2197c74f0ff8275cbd2e5cd0ce7"), 0, 5, 1, 0,
     -1},
    {"B_USECOOL", 20779, 792886, 23,
     digest_literal("20219a2f1fad34ba3347b23fa6843720f6d9a02d5cbf924e7f6a107ba35be8fc"), 0, 5, 3, 2,
     -1},
    {"ITMISWORDBLADEHOT", 16612, 365803, 102,
     digest_literal("0c88c359c41f30272923c22b8c104af6b7e161c2ac7847614b7fb3f8b434e2f4"), 0, 7, 1, 0,
     1584},
    {"ITMISWORDBLADE", 16613, 365905, 102,
     digest_literal("42533a3cfb2ce9b0b046ad0ed324dae34d0d190bad788369329b8a8dcda3a02e"), 0, 7, 1, 0,
     1584},
    {"B_SAY", 14646, 218446, 39,
     digest_literal("b8b93bf2920074b9958b398d5d642935220402bd20ff5044c51f9a416675d6ad"), 3, 5, 1, 0,
     -1},
    {"B_ENDPRODUCTIONDIALOG", 19965, 672156, 49,
     digest_literal("e9fcd2f7db76bab4ed7c5ceb6e5a98c3ab3b8166a3614a4dda2513092da34e64"), 0, 5, 1, 0,
     -1},
}};
inline constexpr std::array<PackedInventoryTarget, 2> targets{
    {{"ITMISWORDBLADEHOT", 16612}, {"ITMISWORDBLADE", 16613}}};
inline constexpr std::array<Requirement, 1> end_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 13}}};
inline constexpr std::array<Requirement, 2> single_conditions{
    {end_conditions[0],
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16612, 0, 1}}};
inline constexpr std::array<Requirement, 2> triple_conditions{
    {end_conditions[0],
     {RequirementKind::inventory_at_least, RequirementRole::resource, 16612, 0, 3}}};
inline constexpr Owner end_owner{
    20770, 999,           22243,         false,         false,
    true,  &functions[1], &functions[2], &functions[3], view(end_conditions)};
inline constexpr Owner single{20773,         2,
                              22243,         false,
                              false,         true,
                              &functions[4], &functions[5],
                              &functions[6], view(single_conditions)};
inline constexpr Owner triple{20776,         2,
                              22243,         false,
                              false,         true,
                              &functions[7], &functions[8],
                              &functions[9], view(triple_conditions)};
inline constexpr auto bindings = station_recipe_catalog::with_common(std::array<SymbolBinding, 6>{{
    {"STARTSMITHING", 10536, 2, 0, 1, -1},
    {"MOBSI_SMITHCOOL", 12323, 2, 1, 1, -1, true, 13},
    {"YPOS_LEVELUP", 976, 2, 1, 1, -1, true, 65},
    {"FONT_SCREENSMALL", 964, 3, 1, 1, -1},
    {"ITMISWORDBLADEHOT", 16612, 7, 1, 0, 1584},
    {"ITMISWORDBLADE", 16613, 7, 1, 0, 1584},
}});
struct Text {
  std::uint32_t symbol, length;
  lhiver::Digest sha256;
};
inline constexpr std::array<Text, 5> texts{{
    {53663, 10, digest_literal("ca98fb72b6e135282883db6fa2ec344f4495064b1d18d365945b876413ff12db")},
    {53664, 17, digest_literal("f682f5ab94ba8a0cd81d3ba1dd83cb4ac2f69719cce5ec9e14253f2b45d5c0b5")},
    {53666, 10, digest_literal("ca98fb72b6e135282883db6fa2ec344f4495064b1d18d365945b876413ff12db")},
    {53667, 20, digest_literal("908def7cf0c2bb61f2ba6f9e96e569773388098c40d957138a31f3c6256aef64")},
    {964, 21, digest_literal("b0ee6624c33f4f973067ef10217f6a33f0df22be9cb7974647680b34f065a20f")},
}};
struct StationProfile {
  const char *on_state, *scheme, *visual, *condition, *use_with;
  std::int32_t state_num, rewind;
  const Function *entry, *mob_condition;
};
inline constexpr StationProfile station{"SMITHCOOL", "BSCOOL", "BSCOOL_OC.MDS", "B_USECOOL",   "",
                                        1,           0,        &functions[0],   &functions[10]};
struct Descriptor {
  RecipeId id;
  const char* name;
  std::int32_t production_context, hot_symbol, cold_symbol;
  const Owner *single, *triple, *end;
  const StationProfile* station_profile;
  bool generated_left;
  std::uint32_t native_end_debit;
};
inline constexpr Descriptor recipe{
    RecipeId::blade, "Klinge abkuehlen", 13,       16612, 16613, &single,
    &triple,         &end_owner,         &station, true,  1};
inline bool structurally_valid(const Descriptor& d) noexcept {
  return d.id == recipe.id && d.name == recipe.name && d.production_context == 13 &&
         d.hot_symbol == 16612 && d.cold_symbol == 16613 && d.single == &single &&
         d.triple == &triple && d.end == &end_owner && d.station_profile == &station &&
         d.generated_left && d.native_end_debit == 1;
}
inline const Descriptor* find(RecipeId id) noexcept { return id == recipe.id ? &recipe : nullptr; }
// S0->S1 creates exactly one blade in the left hand. S1->S0 DESTROY releases the
// actual InteractItem anchor and then removes one stored item of that instance.
// No inventory reserve! The current left anchor/tmpLevel, station entry,
// native hook forwarding and normal end-AFTER each require separate evidence.
} // namespace ubc::cool_recipe_catalog
