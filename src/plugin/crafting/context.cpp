// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
std::int32_t integer(std::uint32_t object, std::uint32_t offset) noexcept {
  std::int32_t value = -1;
  field(object, offset, value);
  return value;
}
bool own_thread() noexcept {
  return InterlockedCompareExchange(&installed, 0, 0) &&
         InterlockedCompareExchange(&engine_thread, 0, 0) ==
             static_cast<LONG>(GetCurrentThreadId());
}
bool name_is(std::uint32_t object, std::uint32_t offset, const char* expected) noexcept {
  std::uint32_t data = 0;
  std::int32_t length = -1;
  const auto wanted = std::strlen(expected);
  if (wanted > 63 || !field(object, offset + 8, data) || !field(object, offset + 12, length) ||
      length < 0 || static_cast<std::size_t>(length) != wanted)
    return false;
  if (!length)
    return true;
  std::array<char, 64> text{};
  return memory.copy(data, text.data(), wanted + 1) &&
         !std::memcmp(text.data(), expected, wanted + 1);
}
bool station_owns(std::uint32_t mob, std::uint32_t hero) noexcept {
  std::uint32_t node = 0;
  if (!field(mob, 0x18c, node))
    return false;
  std::array<std::uint32_t, 64> seen{};
  std::size_t count = 0;
  unsigned matches = 0;
  while (node) {
    if (count == seen.size())
      return false;
    for (std::size_t i = 0; i < count; ++i)
      if (seen[i] == node)
        return false;
    seen[count++] = node;
    std::uint32_t position = 0, next = 0, npc = 0;
    if (!field(node, 0, position) || !field(node, 4, next) || !field(position, 0x44, npc))
      return false;
    if (npc == hero)
      ++matches;
    node = next;
  }
  return matches == 1;
}
Context context() noexcept {
  Context c;
  if (!read(a::game_pointer, c.game) || !c.game ||
      integer(c.game, 0) != static_cast<std::int32_t>(a::game_vtable) ||
      !field(c.game, a::game_world, c.world) || !read(s::hero_pointer, c.hero))
    return c;
  c.identity_known = true;
  if (!c.world || !c.hero)
    return c;
  c.station_known = field(c.hero, s::npc_interact_mob, c.station);
  // The display key compares the parsed dialog fields. Animation and layout
  // bytes elsewhere in the view do not affect the catalogue.
  c.dialog = ubc::read_selection_dialog(live_memory, s::information_manager);
  if (!c.station_known || !c.station || !c.dialog.valid || c.dialog.npc != c.hero ||
      c.dialog.player != c.hero || !station_owns(c.station, c.hero) ||
      integer(c.station, 0x1f4) != 1 || integer(c.station, 0x1f8) != 1)
    return c;
  c.ownership_known = true;
  const auto hero = rt::read_symbol_address(memory, s::game_parser, ubc::lhiver::hero_symbol);
  std::uint32_t bound = 0;
  if (!hero.valid || !field(hero.address, 0x1c, bound) || bound != c.hero)
    return c;
  const auto production = rt::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
  if (!production.valid)
    return c;
  if (production.value == 14 && name_is(c.station, 0x1e0, "FRYINGMEAT") &&
      name_is(c.station, 0x1b8, "STOVE") && name_is(c.station, 0x1a4, "") &&
      name_is(c.station, 0x1cc, "B_USESTOVE"))
    c.kind = cat::Station::stove;
  else if (production.value == 4 && name_is(c.station, 0x1e0, "POTIONALCHEMY") &&
           name_is(c.station, 0x1a4, "ITMI_FLASK") && name_is(c.station, 0x1cc, ""))
    c.kind = cat::Station::alchemy;
  else if (production.value == 1 && name_is(c.station, 0x1e0, acat::station.on_state) &&
           name_is(c.station, 0x1b8, acat::station.scheme) &&
           name_is(c.station, 0x1a4, acat::station.use_with_item) &&
           name_is(c.station, 0x1cc, acat::station.condition_name))
    c.kind = cat::Station::anvil;
  else if (production.value == 16 && name_is(c.station, 0x1e0, bcat::station.on_state) &&
           name_is(c.station, 0x1b8, bcat::station.scheme) &&
           name_is(c.station, 0x1a4, bcat::station.use_with_item) &&
           name_is(c.station, 0x1cc, bcat::station.condition_name) &&
           integer(c.station, 0x200) == bcat::station.rewind)
    c.kind = cat::Station::bow;
  else if (production.value == 13 && name_is(c.station, 0x1e0, ccat::station.on_state) &&
           name_is(c.station, 0x1b8, ccat::station.scheme) &&
           name_is(c.station, 0x1a4, ccat::station.use_with) &&
           name_is(c.station, 0x1cc, ccat::station.condition) &&
           integer(c.station, 0x200) == ccat::station.rewind)
    c.kind = cat::Station::cool;
  else if (production.value == 11 && name_is(c.station, 0x1e0, "SMITHSHARP") &&
           name_is(c.station, 0x1b8, "BSSHARP") && name_is(c.station, 0x1a4, "") &&
           name_is(c.station, 0x1cc, "") && integer(c.station, 0x200) == 0)
    c.kind = cat::Station::grind;
  else {
    bool found = false;
    for (const auto& profile : scat::stations)
      if (production.value == profile.production_context &&
          name_is(c.station, 0x1e0, profile.on_state) &&
          name_is(c.station, 0x1b8, profile.scheme) &&
          name_is(c.station, 0x1a4, profile.use_with_item) &&
          name_is(c.station, 0x1cc, profile.condition_name)) {
        c.kind = profile.station == scat::Station::leatherwork ? cat::Station::leather
                 : profile.station == scat::Station::saw       ? cat::Station::saw
                                                               : cat::Station::forge;
        found = true;
        break;
      }
    if (!found)
      return c;
  }
  // SMITHCOOL/rewind0 keeps the original INFO selection open at S1 with target
  // 0. The binding was observed live before and after a wait of >45s (0.0.20),
  // with the generated left blade still present. Only this fully recognized
  // profile may use target 0. All previous stations still require target 1.
  const auto target = integer(c.station, 0x1fc);
  if (target != 1 && !(c.kind == cat::Station::cool && target == 0))
    return c;
#ifdef UBC_PLAYER_RELEASE
  if (player_image.failure[static_cast<unsigned>(c.kind)] || !player_scripts_match(c.kind))
    return c;
#endif
  c.valid = true;
  return c;
}
bool __cdecl read_ammo_context(void*, ap::Context& out) {
  out = {};
  std::uint32_t game = 0, world = 0, hero = 0;
  if (!own_thread() || !world_epoch || !read(a::game_pointer, game) || !game ||
      integer(game, 0) != static_cast<std::int32_t>(a::game_vtable) ||
      !field(game, a::game_world, world) || !read(s::hero_pointer, hero) || !world || !hero ||
      world != observed_world || hero != observed_hero)
    return false;
  const auto symbol = rt::read_symbol_address(memory, s::game_parser, ubc::lhiver::hero_symbol);
  std::uint32_t bound = 0;
  if (!symbol.valid || !field(symbol.address, 0x1c, bound) || bound != hero)
    return false;
  out = {world_epoch, GetCurrentThreadId(), s::game_parser, game, world, hero};
  return true;
}
bool __cdecl read_ammo_scope(void*, ap::Scope& out) {
  out = {};
  if (!own_thread())
    return false;
  out = active_ammo_scope;
  if (out.active &&
      (!amount_patch.active() || !effect_authorized || info_depth + choice_depth != 1 ||
       !native_update || update_depth != 1 || station_depth || collection_depth))
    return false;
  return true;
}
bool adapter_failed() noexcept {
  return result_guard.state() == ubc::CraftingResultState::failed ||
         bc::diagnostics().writes_disabled || (ammo_job && !ammo::healthy()) ||
         (extra_job && extra_executor && extra_executor->diagnostics().disabled) ||
         (spirit_job && spirit_executor && spirit_executor->diagnostics().disabled);
}
ubc::CraftingKey key(const Context& c) noexcept {
  return {generation, GetCurrentThreadId(),   c.world, c.hero,
          c.station,  s::information_manager, c.kind};
}
bool same_owner(const Context& left, const Context& right) noexcept {
  return left.ownership_known && right.ownership_known && left.game == right.game &&
         left.world == right.world && left.hero == right.hero && left.station == right.station &&
         left.dialog.view == right.dialog.view && left.dialog.npc == right.dialog.npc &&
         left.dialog.player == right.dialog.player;
}
bool same_context(const Context& left, const Context& right) noexcept {
  return left.valid && right.valid && same_owner(left, right) && left.kind == right.kind &&
         left.dialog.mode == right.dialog.mode && left.dialog.info == right.dialog.info;
}
bool foreign_dialog(const Context& c) noexcept {
  return c.identity_known && c.hero && c.dialog.valid &&
         (c.dialog.npc != c.hero || c.dialog.player != c.hero);
}
const basecat::Owner* owner_for(std::int32_t instance) noexcept {
  if (instance == tobacco::owner.instance)
    return &tobacco::owner;
  if (const auto* owner = nav::owner_for(instance))
    return owner;
  for (const auto& recipe : basecat::recipes)
    for (const auto& owner : recipe.owners)
      if (owner.instance == instance)
        return &owner;
  if (const auto* owner = snav::owner_for(instance))
    return owner;
  if (const auto* owner = xnav::owner_for(instance))
    return owner;
  for (const auto& recipe : gcat::recipes)
    if (recipe.owner.instance == instance)
      return &recipe.owner;
  for (const auto& recipe : acat::recipes)
    if (recipe.owner.instance == instance)
      return &recipe.owner;
  for (const auto& recipe : ocat::recipes)
    if (recipe.owner.instance == instance)
      return &recipe.owner;
  for (const auto& recipe : bcat::recipes)
    if (recipe.owner.instance == instance)
      return &recipe.owner;
  if (bcat::station.category.instance == instance)
    return &bcat::station.category;
  if (bcat::station.end.instance == instance)
    return &bcat::station.end;
  for (const auto* owner : {&ccat::single, &ccat::triple, &ccat::end_owner})
    if (owner->instance == instance)
      return owner;
  for (const auto& owner : ubc::grind_menu_metadata::owners)
    if (owner.instance == instance)
      return &owner;
  for (const auto& item : acat::navigation)
    if (item.owner.instance == instance)
      return &item.owner;
  if (acat::station.end.instance == instance)
    return &acat::station.end;
  return nullptr;
}
bool linked(const ubc::SelectionInfo& info) noexcept {
  const auto* owner = owner_for(info.instance);
  return info.valid && owner && info.number == owner->number && info.npc == owner->npc &&
         info.important == static_cast<int>(owner->important) &&
         info.trade == static_cast<int>(owner->trade) &&
         info.permanent == static_cast<int>(owner->permanent) &&
         info.condition == static_cast<int>(owner->condition->symbol) &&
         info.function == static_cast<int>(owner->information->symbol);
}
bool linked_step_info(const ubc::SelectionInfo& info, const ubc::CraftingStep* step) noexcept {
  if (!linked(info))
    return false;
  if (!info.first_choice)
    return true;
  // OnInfo calls the original information function even with choices
  // remaining. Only these six bound owner infos start by calling
  // ClearChoices on their own owner and then rebuild the entire selection.
  // Direct INFO effects and START/STOP still require an empty list.
  if (!step || step->kind != ubc::CraftingStepKind::open_owner ||
      step->invocation != basecat::Invocation::info || step->owner_instance != info.instance ||
      step->function_symbol != info.function)
    return false;
  if (info.instance == 20399)
    return step->before == ubc::crafting_page(nav::Page::tobacco_info) &&
           step->after.kind == ubc::CraftingPageKind::tobacco_choices &&
           step->function_symbol == 20401;
  for (const auto& route : nav::routes)
    if (route.owner->instance == info.instance &&
        route.owner->information->symbol == static_cast<std::uint32_t>(info.function) &&
        step->before == ubc::crafting_page(route.category) &&
        step->after == ubc::crafting_page(route.choices))
      return true;
  for (const auto& route : snav::routes)
    if (route.owner->instance == info.instance &&
        route.owner->information->symbol == static_cast<std::uint32_t>(info.function) &&
        step->before == ubc::crafting_page(route.root) &&
        step->after == ubc::crafting_page(route.choices))
      return true;
  for (const auto& recipe : acat::recipes)
    if (recipe.owner.instance == info.instance &&
        recipe.owner.information->symbol == static_cast<std::uint32_t>(info.function) &&
        step->before == ubc::crafting_page(anav::Page::common) &&
        step->after == ubc::crafting_page(anav::page_for(recipe.id)))
      return true;
  if (info.instance == 20957 && info.function == 20959 &&
      ubc::crafting_grind_page(step->before).page == shnav::Page::info &&
      ubc::crafting_grind_page(step->after).page == shnav::Page::categories)
    return true;
  return false;
}
std::uint32_t current_info_effect(const Context& c) noexcept {
  // Never use a pointer from a previous owner or choice state, or grant access
  // from a nested call or a different thread. Production may already be 0 after
  // the original call. The physical job context and our callback remain bound.
  if (!own_thread() || !effect_authorized || !native_update || update_depth != 1 ||
      info_depth != 1 || choice_depth || station_depth || collection_depth || resolved_read_depth ||
      !c.identity_known || !c.station_known)
    return 0;
  const auto& lease = info_effect_lease;
  const cadmission::InfoIdentity identity{
      {generation, GetCurrentThreadId(), s::game_parser, c.game, c.world, c.hero, c.station},
      frame,
      lease.actual_info,
      s::information_manager};
  return lease.current(memory, info_lease_owner, identity, handoff, dispatched_ticket, rows,
                       c.dialog, owner_for(lease.ticket.offer.owner));
}
bool navigation_contract() noexcept {
  std::array<ubc::lhiver::SymbolBinding,
             basecat::common_bindings.size() + nav::flag_bindings.size()>
      bindings{};
  std::size_t count = 0;
  for (const auto& b : basecat::common_bindings)
    bindings[count++] = b;
  for (const auto& b : nav::flag_bindings)
    bindings[count++] = b;
  const ubc::lhiver::Profile profile{layout_profile.parser_address,
                                     layout_profile.symbol_count,
                                     layout_profile.code_length,
                                     layout_profile.code_sha256,
                                     nav::functions.data(),
                                     nav::functions.size(),
                                     bindings.data(),
                                     count};
  if (!rt::verify_local_contract(memory, s::game_parser, profile).matches)
    return false;
  const ubc::lhiver::Profile extra{layout_profile.parser_address,
                                   layout_profile.symbol_count,
                                   layout_profile.code_length,
                                   layout_profile.code_sha256,
                                   xnav::start_functions.data(),
                                   xnav::start_functions.size(),
                                   bindings.data(),
                                   count};
  return rt::verify_local_contract(memory, s::game_parser, extra).matches;
}
bool complete_collection(const Context& c, ubc::SelectionChoices* choice_out) noexcept {
  if (!c.dialog.valid || !rows.generation || rows.view != c.dialog.view)
    return false;
  if (c.dialog.mode == 1)
    return rows.complete(c.dialog);
  if (c.dialog.mode != 2 || !choice_collection)
    return false;
  const auto choices = ubc::read_selection_choices(memory, c.dialog);
  if (!choices.valid || !linked(choices.owner) || choices.count != choice_count)
    return false;
  for (std::size_t i = 0; i < choice_count; ++i)
    if (choices.functions[i] != choice_functions[i])
      return false;
  if (choice_out)
    *choice_out = choices;
  return true;
}

void initialize_end_compatibility() noexcept {
  if (end_compatibility_attempted)
    return;
  end_compatibility_attempted = true;
  namespace z = ubc::zbug_end_compat;
  end_zbug_module.acquire(L"ZBUGFIXES.DLL", z::zbug_image_size, z::zbug_file_sha256, true);
  end_union_module.acquire(L"shw32.dll", z::union_image_size, z::union_file_sha256, true);
  if (end_zbug_module.current() && end_union_module.current())
    event("end_modules_verified");
}
const ubc::zbug_end_compat::Binding* end_compatibility(bool predebit) noexcept {
  if (predebit ? (!end_zbug_module.observed() || !end_union_module.observed())
               : (!end_zbug_module.current() || !end_union_module.current()))
    return nullptr;
  const auto page = CraftingModule::executable_page;
  end_compatibility_binding = {
      {end_zbug_module.base(), end_zbug_module.image_size(), end_zbug_module.digest(), true, page},
      {end_union_module.base(), end_union_module.image_size(), end_union_module.digest(), true,
       page},
      predebit};
  return &end_compatibility_binding;
}
ubc::station_end_contract::PathVerification end_path(std::uint32_t mob, bool predebit) noexcept {
  std::uint32_t table = 0;
  if (!read(mob, table))
    return {false, ubc::station_end_contract::PathIssue::unreadable};
  return ubc::station_end_contract::verify_path(
      memory, table,
      {InterlockedCompareExchange(&installed, 0, 0) != 0,
       reinterpret_cast<std::uint32_t>(&observe_end),
       reinterpret_cast<std::uint32_t>(hooks.original(8))},
      end_compatibility(predebit));
}

std::uint64_t next_extra_serial() noexcept {
  if (extra_serial == UINT64_MAX) {
    disabled = true;
    return 0;
  }
  return ++extra_serial;
}
xdebit::Context extra_context(const Context& c) noexcept {
  return {generation, GetCurrentThreadId(), s::game_parser, c.game, c.world, c.hero, c.station};
}
xdep::Modules extra_modules(bool predebit) noexcept {
  const auto* plugins = end_compatibility(predebit);
  if (!plugins || !extra_zpe_module.current() || !extra_crt_module.current())
    return {};
  const auto value = [](const auto& m) noexcept {
    return xdep::Module{m.base(), m.image_size(), m.digest(), true};
  };
  return {plugins->zbug, plugins->union_core, value(extra_zpe_module), value(extra_crt_module)};
}
bool initialize_item_modules(bool predebit) noexcept {
  const bool zpe = extra_zpe_module.current() ||
                   extra_zpe_module.acquire(L"zParserExtender.dll", xdep::parser_extender_size,
                                            xdep::parser_extender_sha256);
  const bool crt = extra_crt_module.current() ||
                   extra_crt_module.acquire(L"msvcr100.dll", 0xbe000, xdep::crt_sha256);
  return zpe && crt && end_compatibility(predebit);
}
bool matches_station_profile(std::uint32_t mob, cat::Station kind) noexcept {
  if (kind == cat::Station::stove)
    return name_is(mob, 0x1e0, "FRYINGMEAT") && name_is(mob, 0x1b8, "STOVE") &&
           name_is(mob, 0x1a4, "") && name_is(mob, 0x1cc, "B_USESTOVE");
  if (kind == cat::Station::alchemy)
    return name_is(mob, 0x1e0, "POTIONALCHEMY") && name_is(mob, 0x1a4, "ITMI_FLASK") &&
           name_is(mob, 0x1cc, "");
  if (kind == cat::Station::anvil)
    return name_is(mob, 0x1e0, acat::station.on_state) &&
           name_is(mob, 0x1b8, acat::station.scheme) &&
           name_is(mob, 0x1a4, acat::station.use_with_item) &&
           name_is(mob, 0x1cc, acat::station.condition_name);
  if (kind == cat::Station::bow)
    return name_is(mob, 0x1e0, bcat::station.on_state) &&
           name_is(mob, 0x1b8, bcat::station.scheme) &&
           name_is(mob, 0x1a4, bcat::station.use_with_item) &&
           name_is(mob, 0x1cc, bcat::station.condition_name);
  if (kind == cat::Station::cool)
    return name_is(mob, 0x1e0, ccat::station.on_state) &&
           name_is(mob, 0x1b8, ccat::station.scheme) &&
           name_is(mob, 0x1a4, ccat::station.use_with) &&
           name_is(mob, 0x1cc, ccat::station.condition) &&
           integer(mob, 0x200) == ccat::station.rewind;
  if (kind == cat::Station::grind)
    return name_is(mob, 0x1e0, "SMITHSHARP") && name_is(mob, 0x1b8, "BSSHARP") &&
           name_is(mob, 0x1a4, "") && name_is(mob, 0x1cc, "") && integer(mob, 0x200) == 0;
  for (const auto& profile : scat::stations)
    if (profile.station == station_kind(kind))
      return name_is(mob, 0x1e0, profile.on_state) && name_is(mob, 0x1b8, profile.scheme) &&
             name_is(mob, 0x1a4, profile.use_with_item) &&
             name_is(mob, 0x1cc, profile.condition_name);
  return false;
}
bool common_bindings_verified() noexcept {
  const ubc::lhiver::Profile profile{layout_profile.parser_address,
                                     layout_profile.symbol_count,
                                     layout_profile.code_length,
                                     layout_profile.code_sha256,
                                     nullptr,
                                     0,
                                     basecat::common_bindings.data(),
                                     basecat::common_bindings.size()};
  return rt::verify_local_contract(memory, s::game_parser, profile).matches;
}
} // namespace ubc_native
