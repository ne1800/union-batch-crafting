// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/catalogs/recipe_catalog.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Our own facts for L'Hiver DE 2.0.4. No original scripts or game bytes.
// DAT SHA-256 71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4.
// Route planning only. No engine calls, VM or flag writes, or authorization
// for knowledge, materials, hands or runtime execution. Bind each step afresh
// to the original collection. Row indices are never stored.
namespace ubc::recipe_navigation {
namespace catalog = recipe_catalog;
using catalog::Invocation;
using catalog::Owner;
using catalog::RecipeId;
using catalog::Requirement;
using catalog::RequirementKind;
using catalog::RequirementRole;
using lhiver::digest_literal;
using lhiver::Function;
using lhiver::SymbolBinding;

enum class Page {
  unknown,
  root,
  health_info,
  mana_info,
  health_01_choices,
  health_02_choices,
  health_03_choices,
  mana_01_choices,
  mana_02_choices,
  mana_03_choices,
  special_info,
  booze_info,
  tobacco_info
};
enum class Kind { back, stop, start, open_owner, effect };
enum class Issue { none, unknown_page, unsupported_recipe, quantity, contract };
inline constexpr bool runtime_ready = false;
inline constexpr std::size_t maximum_steps = 5;

// Dynamic values are not DAT initial value checks.
inline constexpr std::array<SymbolBinding, 5> flag_bindings{{{"HEALTHSTART", 20330, 2, 0, 1, -1},
                                                             {"MANASTART", 20299, 2, 0, 1, -1},
                                                             {"BOOZESTART", 20280, 2, 0, 1, -1},
                                                             {"TABAKSTART", 20386, 2, 0, 1, -1},
                                                             {"SPECIALSTART", 20361, 2, 0, 1, -1}}};

inline constexpr std::array<Function, 27> functions{{
    {"PC_MANA_START", 20300, 725368, 67,
     digest_literal("c101cabd27327ae2d9a077b93bc73f3d8a5ca38d5aa78d62f78f4259a08cc1cc"), 0, 7, 1, 0,
     1649},
    {"PC_MANA_START_CONDITION", 20301, 725435, 83,
     digest_literal("e1527415e2e63d47aa9e48a0182e9855308ef11a330d301639839c564712b795"), 0, 5, 3, 2,
     -1},
    {"PC_MANA_START_INFO", 20302, 725518, 12,
     digest_literal("219f9b0f68a3757ee0c503b628de351df21e09c5335b2d04ee91c84c51e269db"), 0, 5, 1, 0,
     -1},
    {"PC_MANA_STOP", 20303, 725530, 67,
     digest_literal("6906463c1091f289b8da29ad01bd1c1657173fbc20dbacf4c4c086b90bf03b0c"), 0, 7, 1, 0,
     1649},
    {"PC_MANA_STOP_CONDITION", 20304, 725597, 35,
     digest_literal("b8197ccd95b71a8bcf504357635555004e5c118dff2828ba4df8aa841282ec2c"), 0, 5, 3, 2,
     -1},
    {"PC_MANA_STOP_INFO", 20305, 725632, 12,
     digest_literal("89075d5afc8126f0827195c25c00b32e7a69fd57424625acea2829e70ff243e4"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_01_BACK", 20309, 725850, 11,
     digest_literal("888251b26dc61ac546c3181cf4956276babc19b09f475ed1667c72df514aaee1"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_02_BACK", 20316, 726704, 11,
     digest_literal("a85453afbd54f63c31a45a69b9bae7dc6c792b963fad3e4119b919a1172b8401"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_MANA_03_BACK", 20323, 727558, 11,
     digest_literal("9e9b3fc3eebf9a46afba30a72869b41f1c9fb92bd973fbaeb5c25c7581fdb0aa"), 0, 5, 1, 0,
     -1},
    {"PC_HEALTH_START", 20331, 728520, 67,
     digest_literal("7fe40c86c070a03f956fda99e4e6070d825a9cac825cb24efcf1e41aaf2f3458"), 0, 7, 1, 0,
     1649},
    {"PC_HEALTH_START_CONDITION", 20332, 728587, 83,
     digest_literal("c3397f78beb9d11fa2f6d804c387f90d9342bd0d13cfdfaf07206db938674ba6"), 0, 5, 3, 2,
     -1},
    {"PC_HEALTH_START_INFO", 20333, 728670, 12,
     digest_literal("563fd8c2d2542eef67bb2b36a769dc464de5c3db724b80eb22a9bce9935ad4da"), 0, 5, 1, 0,
     -1},
    {"PC_HEALTH_STOP", 20334, 728682, 67,
     digest_literal("2543dcf9f287c825cf8fbee8e2c38edba9ebd10b3c6ac169e93fa1fa59b55a6d"), 0, 7, 1, 0,
     1649},
    {"PC_HEALTH_STOP_CONDITION", 20335, 728749, 35,
     digest_literal("687c1ef2042e3648b80575b9592fa8c7c1694b9dcd00320ec365b62e2e631ce9"), 0, 5, 3, 2,
     -1},
    {"PC_HEALTH_STOP_INFO", 20336, 728784, 12,
     digest_literal("79fc500c43846ccb3b63cbe5d71958a68508e3ccf19c5ca7187316b776a6487e"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_01_BACK", 20340, 729001, 11,
     digest_literal("21f171097eb49e7320a880ba4dc72a941423f3f231df137ce76f73337f01ebee"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_02_BACK", 20347, 729855, 11,
     digest_literal("94576e6f19b2eb6083187a077fa02247d0bad76f8d2ab0c77125ff41883031b2"), 0, 5, 1, 0,
     -1},
    {"PC_ITPO_HEALTH_03_BACK", 20354, 730709, 11,
     digest_literal("eadb15e7ef22a335524a4b7d21cf5c01a83108db16139f263f9f7fc52bec3f78"), 0, 5, 1, 0,
     -1},
    {"PC_BOOZE_STOP", 20284, 723476, 67,
     digest_literal("3da9db3c8fcdc635c7f8592eaec445957aa93c293cc9b63626c840395733429a"), 0, 7, 1, 0,
     1649},
    {"PC_BOOZE_STOP_CONDITION", 20285, 723543, 35,
     digest_literal("cb670800344cefad00556f9a9e28d3a0b2b258cfecfbe162daca617cf23be8ad"), 0, 5, 3, 2,
     -1},
    {"PC_BOOZE_STOP_INFO", 20286, 723578, 12,
     digest_literal("d4aaea08660d4fef0e88f73e4f27381b05252febac11224964fc3d6ca2849cab"), 0, 5, 1, 0,
     -1},
    {"PC_SPECIAL_STOP", 20365, 731833, 67,
     digest_literal("cb6ccfc69ac3c88655f846d91458d4b12b04effd8ad44333690decab3ac350ce"), 0, 7, 1, 0,
     1649},
    {"PC_SPECIAL_STOP_CONDITION", 20366, 731900, 35,
     digest_literal("807c9b381a5c03138fe8ce2fe2c2f8c30c24094068fd8d8ae1431aad3449f7d0"), 0, 5, 3, 2,
     -1},
    {"PC_SPECIAL_STOP_INFO", 20367, 731935, 12,
     digest_literal("17b7a1e9b1b7e20860ccc13282ec31fb70af40f21a8c5f4ea137bf56cd632366"), 0, 5, 1, 0,
     -1},
    {"PC_TABAK_STOP", 20390, 734117, 67,
     digest_literal("6543ab81cc5e2b09663642bb0e9cfea2b8f024e88e9c5532381bbfe5fab4e096"), 0, 7, 1, 0,
     1649},
    {"PC_TABAK_STOP_CONDITION", 20391, 734184, 35,
     digest_literal("9193cc8bb3a8da56d99e0194c2ca56deb60c500c5536181907e0b451d4ae428d"), 0, 5, 3, 2,
     -1},
    {"PC_TABAK_STOP_INFO", 20392, 734219, 12,
     digest_literal("da1a8b7d02fc8d8084507a9dfcc4d1c52f4e25bef127d1de06d4cba70abf4493"), 0, 5, 1, 0,
     -1},
}};

constexpr const Function* navigation_function(std::int32_t symbol) noexcept {
  for (const auto& function : functions)
    if (function.symbol == static_cast<std::uint32_t>(symbol))
      return &function;
  return nullptr;
}
inline constexpr std::array<Requirement, 6> category_start_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::context, 20330, 0, 0},
     {RequirementKind::integer_equal, RequirementRole::context, 20299, 0, 0},
     {RequirementKind::integer_equal, RequirementRole::context, 20280, 0, 0},
     {RequirementKind::integer_equal, RequirementRole::context, 20386, 0, 0},
     {RequirementKind::integer_equal, RequirementRole::context, 20361, 0, 0}}};
inline constexpr std::array<Requirement, 2> health_stop_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::context, 20330, 0, 1}}};
inline constexpr std::array<Requirement, 2> mana_stop_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::context, 20299, 0, 1}}};
inline constexpr std::array<Requirement, 2> booze_stop_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::context, 20280, 0, 1}}};
inline constexpr std::array<Requirement, 2> special_stop_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::context, 20361, 0, 1}}};
inline constexpr std::array<Requirement, 2> tobacco_stop_conditions{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 4},
     {RequirementKind::integer_equal, RequirementRole::context, 20386, 0, 1}}};

inline constexpr std::array<Owner, 7> category_owners{{
    {20300, 6, 22243, false, false, true, navigation_function(20300), navigation_function(20301),
     navigation_function(20302), catalog::view(category_start_conditions)},
    {20303, 99, 22243, false, false, true, navigation_function(20303), navigation_function(20304),
     navigation_function(20305), catalog::view(mana_stop_conditions)},
    {20331, 7, 22243, false, false, true, navigation_function(20331), navigation_function(20332),
     navigation_function(20333), catalog::view(category_start_conditions)},
    {20334, 99, 22243, false, false, true, navigation_function(20334), navigation_function(20335),
     navigation_function(20336), catalog::view(health_stop_conditions)},
    {20284, 99, 22243, false, false, true, navigation_function(20284), navigation_function(20285),
     navigation_function(20286), catalog::view(booze_stop_conditions)},
    {20365, 99, 22243, false, false, true, navigation_function(20365), navigation_function(20366),
     navigation_function(20367), catalog::view(special_stop_conditions)},
    {20390, 99, 22243, false, false, true, navigation_function(20390), navigation_function(20391),
     navigation_function(20392), catalog::view(tobacco_stop_conditions)},
}};

struct FlagWrite {
  std::int32_t symbol, value;
};
struct Category {
  Page page;
  const Owner *start, *stop;
  FlagWrite on_start, on_stop; // Each time the only original write effect.
};
inline constexpr std::array<Category, 2> categories{
    {{Page::health_info, &category_owners[2], &category_owners[3], {20330, 1}, {20330, 0}},
     {Page::mana_info, &category_owners[0], &category_owners[1], {20299, 1}, {20299, 0}}}};
// Only populated INFO outputs. In particular tobacco choices are not a
// pure navigation state: their owner has already consumed apple tobacco.
struct SourceCategory {
  Page page;
  const Owner* stop;
  FlagWrite on_stop;
};
inline constexpr std::array<SourceCategory, 3> source_categories{
    {{Page::special_info, &category_owners[5], {20361, 0}},
     {Page::booze_info, &category_owners[4], {20280, 0}},
     {Page::tobacco_info, &category_owners[6], {20386, 0}}}};

struct RecipeRoute {
  RecipeId recipe;
  Page choices, category;
  const Owner* owner;
  const Function* back; // ClearChoices(owner) only; no EndProduction.
  // Original AddChoice call order; explicitly NO UI row indices.
  std::array<std::int32_t, 4> original_choice_symbols;
};
inline constexpr std::array<RecipeRoute, 6> routes{{
    {RecipeId::health_01,
     Page::health_01_choices,
     Page::health_info,
     &catalog::health_01_owners[0],
     navigation_function(20340),
     {{20340, 20343, 20342, 20341}}},
    {RecipeId::health_02,
     Page::health_02_choices,
     Page::health_info,
     &catalog::health_02_owners[0],
     navigation_function(20347),
     {{20347, 20350, 20349, 20348}}},
    {RecipeId::health_03,
     Page::health_03_choices,
     Page::health_info,
     &catalog::health_03_owners[0],
     navigation_function(20354),
     {{20354, 20357, 20356, 20355}}},
    {RecipeId::mana_01,
     Page::mana_01_choices,
     Page::mana_info,
     &catalog::mana_01_owners[0],
     navigation_function(20309),
     {{20309, 20312, 20311, 20310}}},
    {RecipeId::mana_02,
     Page::mana_02_choices,
     Page::mana_info,
     &catalog::mana_02_owners[0],
     navigation_function(20316),
     {{20316, 20319, 20318, 20317}}},
    {RecipeId::mana_03,
     Page::mana_03_choices,
     Page::mana_info,
     &catalog::mana_03_owners[0],
     navigation_function(20323),
     {{20323, 20326, 20325, 20324}}},
}};

inline const RecipeRoute* route_for(RecipeId id) noexcept {
  for (const auto& route : routes)
    if (route.recipe == id)
      return &route;
  return nullptr;
}
inline const RecipeRoute* route_for(Page page) noexcept {
  for (const auto& route : routes)
    if (route.choices == page)
      return &route;
  return nullptr;
}
inline const Category* category_for(Page page) noexcept {
  for (const auto& category : categories)
    if (category.page == page)
      return &category;
  return nullptr;
}
inline const SourceCategory* source_category_for(Page page) noexcept {
  for (const auto& category : source_categories)
    if (category.page == page)
      return &category;
  return nullptr;
}
inline bool supported_page(Page page) noexcept {
  return page == Page::root || category_for(page) || source_category_for(page) || route_for(page);
}
inline const Owner* owner_for(std::int32_t instance) noexcept {
  for (const auto& owner : category_owners)
    if (owner.instance == instance)
      return &owner;
  for (const auto& route : routes)
    if (route.owner->instance == instance)
      return route.owner;
  return nullptr;
}
// Resolve only these navigation targets and the six bound recipe families.
inline const Function* function_for(std::int32_t symbol) noexcept {
  if (const auto* own = navigation_function(symbol))
    return own;
  for (const auto& route : routes) {
    for (const auto* function :
         {route.owner->constructor, route.owner->condition, route.owner->information})
      if (function->symbol == static_cast<std::uint32_t>(symbol))
        return function;
    const auto* recipe = catalog::find(route.recipe);
    for (const auto& callback : recipe->callbacks)
      if (callback.function->symbol == static_cast<std::uint32_t>(symbol))
        return callback.function;
  }
  return nullptr;
}

namespace detail {
inline bool valid_function(const Function* f, std::uint32_t type, std::uint32_t flags,
                           std::uint32_t result, std::int32_t parent) noexcept {
  if (!f || !f->name || !*f->name || std::strlen(f->name) > 127 || !f->symbol ||
      f->symbol >= lhiver::symbol_count || f->argc || f->type != type || f->flags != flags ||
      f->return_type != result || f->parent_index != parent || !f->byte_length ||
      f->byte_offset >= lhiver::code_length ||
      f->byte_length > lhiver::code_length - f->byte_offset)
    return false;
  for (const auto byte : f->body_sha256)
    if (byte)
      return true;
  return false;
}
inline bool valid_owner(const Owner* owner) noexcept {
  return owner && owner->instance > 0 && owner->npc == 22243 && !owner->important &&
         !owner->trade && owner->permanent && valid_function(owner->constructor, 7, 1, 0, 1649) &&
         owner->constructor->symbol == static_cast<std::uint32_t>(owner->instance) &&
         valid_function(owner->condition, 5, 3, 2, -1) &&
         valid_function(owner->information, 5, 1, 0, -1) &&
         owner->condition->symbol != owner->information->symbol && owner->conditions.data &&
         owner->conditions.count;
}
inline bool condition(const Requirement& value, std::int32_t symbol,
                      std::int32_t expected) noexcept {
  return value.kind == RequirementKind::integer_equal && value.role == RequirementRole::context &&
         value.symbol == symbol && value.element == 0 && value.value == expected;
}
inline bool same_function(const Function* a, const Function* b) noexcept {
  return a && b && a->name && b->name && std::strcmp(a->name, b->name) == 0 &&
         a->symbol == b->symbol && a->byte_offset == b->byte_offset &&
         a->byte_length == b->byte_length && a->body_sha256 == b->body_sha256 &&
         a->argc == b->argc && a->type == b->type && a->flags == b->flags &&
         a->return_type == b->return_type && a->parent_index == b->parent_index;
}
inline bool same_owner_binding(const Owner* a, const Owner* b) noexcept {
  if (!(a && b && a->instance == b->instance && a->number == b->number && a->npc == b->npc &&
        a->important == b->important && a->trade == b->trade && a->permanent == b->permanent &&
        same_function(a->constructor, b->constructor) &&
        same_function(a->condition, b->condition) &&
        same_function(a->information, b->information) &&
        a->conditions.count == b->conditions.count && a->conditions.data && b->conditions.data))
    return false;
  for (std::size_t i = 0; i < a->conditions.count; ++i) {
    const auto& first = a->conditions[i];
    const auto& second = b->conditions[i];
    if (first.kind != second.kind || first.role != second.role || first.symbol != second.symbol ||
        first.element != second.element || first.value != second.value)
      return false;
  }
  return true;
}
} // namespace detail

// Only well-formed own metadata; no validation of arbitrary storage pointers
// and no substitute for a runtime comparison of names/signatures/body hashes.
inline bool structurally_valid(const Category& category) noexcept {
  if (category.page != Page::health_info && category.page != Page::mana_info)
    return false;
  const auto* canonical = category_for(category.page);
  if (!canonical || !detail::same_owner_binding(category.start, canonical->start) ||
      !detail::same_owner_binding(category.stop, canonical->stop))
    return false;
  const auto flag = category.page == Page::health_info ? 20330 : 20299;
  if (!detail::valid_owner(category.start) || !detail::valid_owner(category.stop) ||
      category.start->instance == category.stop->instance || category.stop->number != 99 ||
      category.start->number != (category.page == Page::health_info ? 7 : 6) ||
      category.on_start.symbol != flag || category.on_start.value != 1 ||
      category.on_stop.symbol != flag || category.on_stop.value != 0 ||
      category.start->conditions.count != 6 || category.stop->conditions.count != 2)
    return false;
  if (!detail::condition(category.start->conditions[0], 12330, 4) ||
      !detail::condition(category.stop->conditions[0], 12330, 4) ||
      !detail::condition(category.stop->conditions[1], flag, 1))
    return false;
  for (std::size_t i = 0; i < flag_bindings.size(); ++i)
    if (!detail::condition(category.start->conditions[i + 1],
                           static_cast<std::int32_t>(flag_bindings[i].symbol), 0))
      return false;
  return true;
}
inline bool structurally_valid(const SourceCategory& category) noexcept {
  const auto* canonical = source_category_for(category.page);
  if (!canonical || !detail::same_owner_binding(category.stop, canonical->stop) ||
      !detail::valid_owner(category.stop) || category.stop->number != 99 ||
      category.stop->information->byte_length != 12 || category.stop->conditions.count != 2 ||
      category.on_stop.symbol != canonical->on_stop.symbol || category.on_stop.value != 0)
    return false;
  return detail::condition(category.stop->conditions[0], 12330, 4) &&
         detail::condition(category.stop->conditions[1], category.on_stop.symbol, 1);
}
inline bool structurally_valid(const RecipeRoute& route) noexcept {
  const auto* recipe = catalog::find(route.recipe);
  if (!recipe || recipe->station != catalog::Station::alchemy || recipe->production_context != 4 ||
      !catalog::structurally_valid(*recipe) || !detail::valid_owner(route.owner) ||
      !category_for(route.category) || route.choices == Page::unknown ||
      !detail::valid_function(route.back, 5, 1, 0, -1) || route.back->byte_length != 11 ||
      route.original_choice_symbols[0] != static_cast<std::int32_t>(route.back->symbol) ||
      recipe->owners.count != 1 || route.owner->instance != recipe->owners[0].instance)
    return false;
  const auto* canonical = route_for(route.recipe);
  if (!canonical || canonical->choices != route.choices || canonical->category != route.category ||
      !detail::same_owner_binding(route.owner, canonical->owner) ||
      !detail::same_function(route.back, canonical->back))
    return false;
  for (std::size_t i = 0; i < route.original_choice_symbols.size(); ++i) {
    if (route.original_choice_symbols[i] <= 0)
      return false;
    for (std::size_t j = 0; j < i; ++j)
      if (route.original_choice_symbols[i] == route.original_choice_symbols[j])
        return false;
  }
  if (recipe->callbacks.count != 3)
    return false;
  for (std::size_t i = 1; i < route.original_choice_symbols.size(); ++i) {
    const std::uint32_t original_quantity = i == 1 ? 5 : i == 2 ? 3 : 1;
    bool found = false;
    for (const auto& callback : recipe->callbacks)
      if (callback.original_quantity == original_quantity &&
          callback.owner_instance == route.owner->instance &&
          callback.invocation == Invocation::choice &&
          callback.function->symbol == static_cast<std::uint32_t>(route.original_choice_symbols[i]))
        found = true;
    if (!found)
      return false;
  }
  return true;
}
inline bool structurally_valid() noexcept {
  for (std::size_t i = 0; i < flag_bindings.size(); ++i) {
    const auto& flag = flag_bindings[i];
    if (!flag.name || !*flag.name || flag.type != 2 || flag.flags || flag.elements != 1 ||
        flag.parent_index != -1 || flag.check_value || flag.check_offset)
      return false;
    for (std::size_t j = 0; j < i; ++j)
      if (flag.symbol == flag_bindings[j].symbol ||
          std::strcmp(flag.name, flag_bindings[j].name) == 0)
        return false;
  }
  for (std::size_t i = 0; i < functions.size(); ++i) {
    const auto& f = functions[i];
    if (!detail::valid_function(&f, f.type, f.flags, f.return_type, f.parent_index))
      return false;
    for (std::size_t j = 0; j < i; ++j) {
      const auto& other = functions[j];
      if (f.symbol == other.symbol || std::strcmp(f.name, other.name) == 0 ||
          (f.byte_offset < other.byte_offset + other.byte_length &&
           other.byte_offset < f.byte_offset + f.byte_length))
        return false;
    }
  }
  for (const auto& category : categories)
    if (!structurally_valid(category))
      return false;
  for (const auto& category : source_categories)
    if (!structurally_valid(category))
      return false;
  for (std::size_t i = 0; i < routes.size(); ++i) {
    if (!structurally_valid(routes[i]))
      return false;
    for (std::size_t j = 0; j < i; ++j)
      if (routes[i].recipe == routes[j].recipe || routes[i].choices == routes[j].choices ||
          routes[i].owner->instance == routes[j].owner->instance ||
          routes[i].back->symbol == routes[j].back->symbol)
        return false;
  }
  return true;
}

struct PageState {
  bool known = false; // Mode, production value, and ALL five flags read.
  std::int32_t production = -1;
  std::int32_t health = -1, mana = -1, booze = -1, tobacco = -1, special = -1;
  Invocation invocation = Invocation::info;
  std::int32_t choice_owner = -1; // Only when choice: currently positively bound owner.
};
// The adapter must separately record Station, current Collection,
// Mode, and Context. This function classifies only already-read values.
inline Page classify_page(const PageState& value) noexcept {
  if (!value.known || value.production != 4)
    return Page::unknown;
  unsigned active = 0;
  for (const auto flag : {value.health, value.mana, value.booze, value.tobacco, value.special}) {
    if (flag != 0 && flag != 1)
      return Page::unknown;
    active += static_cast<unsigned>(flag);
  }
  if (active > 1)
    return Page::unknown;
  const auto category = value.health    ? Page::health_info
                        : value.mana    ? Page::mana_info
                        : value.booze   ? Page::booze_info
                        : value.tobacco ? Page::tobacco_info
                        : value.special ? Page::special_info
                                        : Page::root;
  if (value.invocation == Invocation::info)
    return category;
  if (value.invocation != Invocation::choice || category == Page::root)
    return Page::unknown;
  for (const auto& route : routes)
    if (route.owner->instance == value.choice_owner && route.category == category)
      return route.choices;
  return Page::unknown;
}

struct Step {
  Kind kind = Kind::back;
  Invocation invocation = Invocation::info;
  std::int32_t owner_instance = -1, function_symbol = -1;
  Page before = Page::unknown, after = Page::unknown;
  bool is_effect() const noexcept { return kind == Kind::effect; }
};
struct Plan {
  Issue issue = Issue::contract;
  Page source = Page::unknown;
  RecipeId target = RecipeId::meat;
  std::uint32_t quantity = 0;
  std::array<Step, maximum_steps> steps{};
  std::size_t count = 0;
  bool valid() const noexcept {
    return issue == Issue::none && count > 0 && count <= maximum_steps;
  }
  const Step* effect() const noexcept {
    return valid() && steps[count - 1].is_effect() ? &steps[count - 1] : nullptr;
  }
};
// The result is not a consumable order and does nothing. The
// native Handoff rebinds every step and rejects on context switch.
// effect.after=unknown: its station/material follow-up is checked separately.
inline Plan make_plan(Page source, RecipeId target, std::uint32_t quantity) noexcept {
  Plan out;
  out.source = source;
  out.target = target;
  out.quantity = quantity;
  if (!supported_page(source)) {
    out.issue = Issue::unknown_page;
    return out;
  }
  const auto* destination = route_for(target);
  if (!destination) {
    out.issue = Issue::unsupported_recipe;
    return out;
  }
  if (!structurally_valid())
    return out;
  const auto* recipe = catalog::find(target);
  const auto* callback = catalog::callback_for_quantity(*recipe, quantity);
  if (!callback) {
    out.issue = Issue::quantity;
    return out;
  }
  if (callback->invocation != Invocation::choice ||
      callback->owner_instance != destination->owner->instance ||
      callback->original_quantity != (quantity == 1 ? 1u : 5u))
    return out;
  Page current = source;
  auto add = [&](Kind kind, Invocation invocation, std::int32_t owner, std::uint32_t function,
                 Page after) noexcept {
    if (out.count >= out.steps.size())
      return false;
    out.steps[out.count++] = {kind,    invocation, owner, static_cast<std::int32_t>(function),
                              current, after};
    current = after;
    return true;
  };
  if (const auto* from = route_for(current)) {
    if (from->recipe != target && !add(Kind::back, Invocation::choice, from->owner->instance,
                                       from->back->symbol, from->category))
      return Plan{};
  }
  if (current != destination->choices) {
    if (current != destination->category) {
      if (const auto* category = category_for(current))
        if (!add(Kind::stop, Invocation::info, category->stop->instance,
                 category->stop->information->symbol, Page::root))
          return Plan{};
      if (const auto* category = source_category_for(current))
        if (!add(Kind::stop, Invocation::info, category->stop->instance,
                 category->stop->information->symbol, Page::root))
          return Plan{};
      const auto* category = category_for(destination->category);
      if (!category || current != Page::root ||
          !add(Kind::start, Invocation::info, category->start->instance,
               category->start->information->symbol, category->page))
        return Plan{};
    }
    if (!add(Kind::open_owner, Invocation::info, destination->owner->instance,
             destination->owner->information->symbol, destination->choices))
      return Plan{};
  }
  if (!add(Kind::effect, Invocation::choice, destination->owner->instance,
           callback->function->symbol, Page::unknown))
    return Plan{};
  out.issue = Issue::none;
  return out;
}
inline bool structurally_valid(const Plan& plan) noexcept {
  if (!plan.valid())
    return false;
  const auto expected = make_plan(plan.source, plan.target, plan.quantity);
  if (!expected.valid() || expected.count != plan.count)
    return false;
  for (std::size_t i = 0; i < plan.count; ++i) {
    const auto& a = plan.steps[i];
    const auto& b = expected.steps[i];
    if (a.kind != b.kind || a.invocation != b.invocation || a.owner_instance != b.owner_instance ||
        a.function_symbol != b.function_symbol || a.before != b.before || a.after != b.after)
      return false;
  }
  return true;
}
inline const char* issue_name(Issue issue) noexcept {
  switch (issue) {
  case Issue::none:
    return "none";
  case Issue::unknown_page:
    return "unknown_page";
  case Issue::unsupported_recipe:
    return "unsupported_recipe";
  case Issue::quantity:
    return "quantity";
  case Issue::contract:
    return "contract";
  }
  return "invalid_issue";
}
} // namespace ubc::recipe_navigation
