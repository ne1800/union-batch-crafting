// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
bool bow_candidate(const Context& c) noexcept {
#ifdef UBC_PLAYER_RELEASE
  if (!c.valid)
    return false;
#endif
  return c.station && (c.valid ? c.kind == cat::Station::bow
                               : name_is(c.station, 0x1e0, bcat::station.on_state) ||
                                     name_is(c.station, 0x1b8, bcat::station.scheme) ||
                                     name_is(c.station, 0x120, "MOBNAME_BOW_WBENCH"));
}
void bow_station_name(ubc::BowMenuTrace& trace, const Context& c, const char* field_name,
                      unsigned offset, const char* expected) noexcept {
  std::uint32_t data = 0;
  int length = -1;
  std::array<char, 64> value{};
  if (!field(c.station, offset + 8, data) || !field(c.station, offset + 12, length) || length < 0 ||
      length >= 64 || (length && (!memory.copy(data, value.data(), length + 1) || value[length]))) {
    trace.check(field_name, expected, "unlesbar", false);
    return;
  }
  const bool matches = std::strlen(expected) == static_cast<unsigned>(length) &&
                       !std::memcmp(value.data(), expected, length);
  for (int i = 0; i < length; ++i)
    if (value[i] < 32 || value[i] > 126 || value[i] == '"' || value[i] == '\\')
      value[i] = '?';
  trace.check(field_name, expected, value.data(), matches);
}
void bow_station_checks(const Context& c, ubc::BowMenuTrace& trace) noexcept {
  trace.equal("station.game_world_hero", 1, c.identity_known && c.world && c.hero);
  trace.equal("station.interact_mob_read", 1, c.station_known);
  trace.equal("station.dialog_valid", 1, c.dialog.valid);
  trace.equal("station.owns_hero", 1, station_owns(c.station, c.hero));
  trace.equal("station.state", 1, integer(c.station, 0x1f4));
  trace.equal("station.state_count", 1, integer(c.station, 0x1f8));
  trace.equal("station.target", 1, integer(c.station, 0x1fc));
  trace.equal("station.rewind", bcat::station.rewind, integer(c.station, 0x200));
  const auto hero = rt::read_symbol_address(memory, s::game_parser, ubc::lhiver::hero_symbol);
  std::uint32_t bound = 0;
  if (hero.valid && field(hero.address, 0x1c, bound))
    trace.equal("station.hero_binding", c.hero, bound);
  else
    trace.check("station.hero_binding", "Held", "unlesbar", false);
  const auto p = rt::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
  if (p.valid)
    trace.equal("station.production", 16, p.value);
  else
    trace.check("station.production", "16", rt::issue_name(p.issue), false);
  bow_station_name(trace, c, "station.on_state", 0x1e0, bcat::station.on_state);
  bow_station_name(trace, c, "station.scheme", 0x1b8, bcat::station.scheme);
  bow_station_name(trace, c, "station.use_with_item", 0x1a4, bcat::station.use_with_item);
  bow_station_name(trace, c, "station.condition", 0x1cc, bcat::station.condition_name);
}
bool bow_entry(const Context& c, ubc::BowMenuTrace& trace) noexcept {
  if (trace.full || !c.valid)
    bow_station_checks(c, trace);
  if (trace.full || !c.valid || !c.dialog.open || !c.dialog.waiting)
    ubc::read_selection_dialog(memory, s::information_manager, &trace, c.hero);
  const bool collection = complete_collection(c);
  trace.equal("entry.original_station", 0, original_station);
  trace.equal("entry.handoff", 0, handoff.active());
  trace.equal("entry.success_pending", 0, success_pending);
  trace.equal("entry.adapter_failed", 0, adapter_failed());
  trace.equal("entry.context", 1, c.valid);
  trace.equal("dialog.waiting", 1, c.dialog.waiting);
  trace.equal("dialog.open", 1, c.dialog.open);
  trace.equal("info.complete", 1, collection);
  if (trace.full || !collection) {
    trace.value("info.generation", ">0", rows.generation, rows.generation != 0);
    trace.equal("info.view", c.dialog.view, rows.view);
    trace.equal("info.valid", 1, rows.valid);
    trace.equal("info.count", c.dialog.count, rows.count);
    if (rows.rejected_field)
      trace.equal(rows.rejected_field, rows.expected, rows.actual);
  }
  if (trace.full)
    for (std::size_t i = 0; i < rows.count; ++i) {
      const auto& row = rows.rows[i];
      const auto* owner = owner_for(row.instance);
      const int index = static_cast<int>(i);
      trace.value("info.instance", "bekannter Owner", row.instance, owner != nullptr, index);
      if (owner) {
        trace.equal("info.condition", owner->condition->symbol, row.condition, index, row.instance);
        trace.equal("info.function", owner->information->symbol, row.function, index, row.instance);
        trace.equal("info.number", owner->number, row.number, index, row.instance);
        trace.equal("info.npc", owner->npc, row.npc, index, row.instance);
        trace.equal("info.important", owner->important, row.important, index, row.instance);
        trace.equal("info.trade", owner->trade, row.trade, index, row.instance);
        trace.equal("info.permanent", owner->permanent, row.permanent, index, row.instance);
      }
    }
  std::uint32_t view = c.dialog.view;
  if (!c.dialog.valid)
    field(s::information_manager, 0x1c, view);
  const auto expected = view && view <= UINT32_MAX - a::view_callback ? view + a::view_callback : 0;
  const auto top = ubc::NativeSelectionUi::top_callback();
  const int global_menu = integer(0x8d1e38, 8);
  if (expected)
    trace.equal("dialog.input_owner", expected, top);
  else
    trace.check("dialog.input_owner", "Dialogcallback", "View unlesbar", false);
  trace.equal("entry.global_menu", 0, global_menu);
  return !original_station && c.valid && c.dialog.waiting && c.dialog.open && collection &&
         expected && top == expected && global_menu == 0;
}
nav::Page alchemy_page(const Context& c) noexcept {
  if (!c.valid || c.kind != cat::Station::alchemy || !navigation_contract())
    return nav::Page::unknown;
  nav::PageState state;
  state.known = true;
  state.production = 4;
  std::array<std::int32_t, 5> values{};
  for (std::size_t i = 0; i < values.size(); ++i) {
    const auto v = rt::symbol_int(memory, s::game_parser, nav::flag_bindings[i].symbol);
    if (!v.valid)
      return nav::Page::unknown;
    values[i] = v.value;
  }
  state.health = values[0];
  state.mana = values[1];
  state.booze = values[2];
  state.tobacco = values[3];
  state.special = values[4];
  state.invocation = c.dialog.mode == 1 ? basecat::Invocation::info : basecat::Invocation::choice;
  if (c.dialog.mode == 2) {
    const auto info = ubc::read_selection_info(memory, c.dialog.info);
    if (!linked(info))
      return nav::Page::unknown;
    state.choice_owner = info.instance;
  }
  for (std::size_t i = 0; i < values.size(); ++i) {
    const auto v = rt::symbol_int(memory, s::game_parser, nav::flag_bindings[i].symbol);
    if (!v.valid || v.value != values[i])
      return nav::Page::unknown;
  }
  // A paid page is accessible only within this specific handoff.
  if (state.choice_owner == 20399 && c.dialog.mode == 2 && handoff.active() &&
      !handoff.cancelled() && tobacco_owner.owns(key(c), c.dialog.view, c.dialog.info)) {
    state.invocation = basecat::Invocation::info;
    if (nav::classify_page(state) == nav::Page::tobacco_info)
      return nav::Page::tobacco_info;
    return nav::Page::unknown;
  }
  return nav::classify_page(state);
}
bool bow_modules_current() noexcept {
  return bow_initialized && bow_item_reader && bow_renderer_module.current() &&
         extra_zpe_module.current() && extra_crt_module.current() && end_zbug_module.current() &&
         end_union_module.current() && (!bow_locator_bound || bow_locator_module.observed());
}
bitem::Input bow_read_input(const ubc::CraftingKey& k) noexcept {
  bitem::Input input;
  input.boundary = {{k.generation, k.thread, k.world, k.hero, k.station, k.manager, 16},
                    frame,
                    GetCurrentThreadId(),
                    true};
  input.modules = extra_modules();
  input.renderer = {bow_renderer_module.base(), bow_renderer_module.image_size(),
                    bow_renderer_module.digest(), true};
  if (bow_locator_bound && bow_locator_module.observed())
    input.locator = {bow_locator_module.base(), bow_locator_module.image_size(),
                     bow_locator_module.digest(), true, CraftingModule::executable_page};
  return input;
}
void initialize_bow(const Context& c) noexcept {
  if (bow_initialized || !c.valid || c.kind != cat::Station::bow || !own_thread() ||
      frame_depth != 1 || update_depth || info_depth || choice_depth || station_depth ||
      collection_depth || handoff.active() || amount_patch.active())
    return;
  bow_initialized = true;
  bow_item_reader.emplace(GetCurrentThreadId());
  const bool modules = initialize_item_modules();
  const bool renderer = bow_renderer_module.acquire(
      L"g2a_avx2.dll", ubc::bow_item_renderer::image_size, ubc::bow_item_renderer::file_sha256);
  const auto& locator = ubc::vob_locator_compat::module_profiles[0];
  bow_locator_bound = bow_locator_module.acquire(L"UNION_VOB_LOCATOR.DLL", locator.image_size,
                                                 locator.file_sha256, true);
  log_plugin_version("bow_locator_version", bow_locator_module);
  log_plugin_version("bow_zbug_version", end_zbug_module);
  log_plugin_version("bow_union_version", end_union_module);
  log_bow_check("module.dependencies", 1, modules);
  log_bow_check("module.renderer", 1, renderer);
  event(modules && renderer ? "bow_modules_verified" : "bow_modules_rejected");
}
void initialize_extra(const Context& c) noexcept {
  if (extra_initialized || !c.valid || c.kind != cat::Station::alchemy || !own_thread() ||
      frame_depth != 1 || update_depth || info_depth || choice_depth || station_depth ||
      collection_depth || handoff.active() || amount_patch.active())
    return;
  extra_initialized = true;
  const auto& locator = ubc::vob_locator_compat::module_profiles[0];
  tobacco_locator.acquire(L"UNION_VOB_LOCATOR.DLL", locator.image_size, locator.file_sha256, true);
  const bool modules = initialize_item_modules();
  if (dndr_active)
    return;
  extra_catalogue_reader.emplace(GetCurrentThreadId());
  tobacco_list.emplace(GetCurrentThreadId());
  spirit_catalogue_reader.emplace(GetCurrentThreadId());
  xwriter::Config config;
  config.owner_thread = GetCurrentThreadId();
  config.memory = memory;
  config.boundary = extra_boundary;
  config.remove = reinterpret_cast<xbridge::Remove>(xdep::expected_targets.remove);
  config.restore_pop = reinterpret_cast<xbridge::RestorePop>(xdep::expected_targets.restore_pop);
  config.restore_instance =
      reinterpret_cast<xbridge::RestoreInstance>(xdep::expected_targets.restore_instance);
  config.restore_item = reinterpret_cast<xbridge::RestoreItem>(0x7a2f20);
  extra_executor.emplace(config);
  gwriter::Config spirit_config;
  spirit_config.owner_thread = GetCurrentThreadId();
  spirit_config.memory = memory;
  spirit_config.boundary = spirit_boundary;
  spirit_config.remove = config.remove;
  spirit_config.restore_pop = config.restore_pop;
  spirit_config.restore_instance = config.restore_instance;
  spirit_config.restore_item = config.restore_item;
  spirit_executor.emplace(spirit_config);
  event(modules ? "extra_modules_verified" : "extra_modules_rejected");
}
ubc::CraftingAlchemyExtraInput extra_read_input(const Context& c, bool boundary) noexcept {
  ubc::CraftingAlchemyExtraInput in;
  in.key = key(c);
  in.context = extra_context(c);
  in.serial = next_extra_serial();
  in.current_thread = GetCurrentThreadId();
  in.modules = extra_modules();
  in.targets = xdep::expected_targets;
  in.restore_item = 0x7a2f20;
  const auto bridge = bc::diagnostics();
  in.boundary.point = resolved_read_depth         ? ubc::AlchemyExtraReadPoint::selection_resolved
                      : info_depth + choice_depth ? ubc::AlchemyExtraReadPoint::before_original
                                                  : ubc::AlchemyExtraReadPoint::post_render;
  in.boundary.verified =
      boundary && own_thread() && c.valid && !station_depth && !collection_depth &&
      (!resolved_read_depth || (resolved_read_depth == 1 && native_update && update_depth == 1 &&
                                !info_depth && !choice_depth));
  in.boundary.physical_lab_hand_contract_verified =
      c.valid && c.kind == cat::Station::alchemy && end_path(c.station).valid;
  in.boundary.callback_depth = info_depth + choice_depth;
  in.boundary.writer_depth = bridge.writer ? 1u : 0u;
  in.boundary.resolve_depth = resolved_read_depth;
  return in;
}
std::uint32_t read_spirit_actual_info(void*, const ubc::CraftingAlchemyExtraInput& in) noexcept {
  const auto c = context();
  if (!(key(c) == in.key) || !xdebit::same(extra_context(c), in.context))
    return 0;
  return current_info_effect(c);
}
scat::Station station_kind(cat::Station kind) noexcept {
  return kind == cat::Station::leather ? scat::Station::leatherwork
         : kind == cat::Station::saw   ? scat::Station::saw
                                       : scat::Station::forge;
}
cnav::Page cool_info_page(const Context& c, bool reread) noexcept {
  if (!c.valid || c.kind != cat::Station::cool)
    return {};
  const auto state = crt::context(memory, s::game_parser, crt::assemble(ccat::recipe));
  if (!state.station_matches())
    return {};
  const auto page = ubc::cool_collection_page(memory, rows, c.dialog, state.production, reread);
  return crt::same_context_evidence(
             state, crt::context(memory, s::game_parser, crt::assemble(ccat::recipe)))
             ? page
             : cnav::Page{};
}
cnav::Page cool_page(const Context& c) noexcept {
  if (!c.valid || c.kind != cat::Station::cool ||
      !crt::verify_recipe(memory, s::game_parser, ccat::recipe).valid)
    return {};
  return cool_info_page(c, false);
}
ubc::CraftingPage page(const Context& c, ubc::BowMenuTrace* trace) noexcept {
  if (!c.valid)
    return {};
  if (c.kind == cat::Station::stove)
    return c.dialog.mode == 1 ? ubc::crafting_stove_page() : ubc::CraftingPage{};
  if (dndr_active && (c.kind == cat::Station::alchemy || c.kind == cat::Station::anvil)) {
    const bool at_anvil = c.kind == cat::Station::anvil;
    if (c.dialog.mode != 1 && (at_anvil || c.dialog.mode != 2))
      return {};
    unsigned selected = 0;
    for (const auto& category : ubc::dndr::station_categories(at_anvil)) {
      const auto flag = category.flag;
      const auto value = rt::symbol_int(memory, s::game_parser, flag, ubc::dndr::layout_profile);
      if (!value.valid || (value.value != 0 && value.value != 1))
        return {};
      if (value.value) {
        if (selected)
          return {};
        selected = flag;
      }
    }
    if (c.dialog.mode == 2) {
      const auto owner = ubc::read_selection_info(memory, c.dialog.info);
      if (selected == 10120 && linked(owner) && owner.instance == active_tobacco().owner.instance &&
          tobacco_owner.owns(key(c), c.dialog.view, c.dialog.info))
        return {ubc::CraftingPageKind::tobacco_choices, static_cast<unsigned>(owner.instance)};
      return {};
    }
    return {at_anvil ? ubc::CraftingPageKind::dndr_anvil : ubc::CraftingPageKind::dndr_alchemy,
            selected};
  }
  if (c.kind == cat::Station::alchemy) {
    const auto current = alchemy_page(c);
    if (c.dialog.mode == 2 && current == nav::Page::tobacco_info)
      return {ubc::CraftingPageKind::tobacco_choices, tobacco::owner.instance};
    return ubc::crafting_page(current);
  }
  if (c.kind == cat::Station::cool)
    return ubc::crafting_page(cool_page(c));
  if (c.kind == cat::Station::grind)
    return ubc::crafting_page(grind_page(c));
  if (c.kind == cat::Station::bow)
    return ubc::crafting_page(ubc::bow_menu::page(memory, s::game_parser, c.dialog.mode, trace));
  if (c.kind == cat::Station::anvil) {
    // Use the entire source graph, including other sources' own BACK owners,
    // beyond the target recipe. Rebind every original category continuation.
    const auto primary = art::assemble(acat::recipes[0]);
    for (const auto& recipe : acat::recipes)
      if (!art::verify(memory, s::game_parser, art::assemble(recipe)).matches)
        return {};
    for (const auto& recipe : ocat::recipes)
      if (!ort::verify(memory, s::game_parser, ort::assemble(recipe)).matches)
        return {};
    const auto before = art::context(memory, s::game_parser, primary);
    if (!before.station_matches())
      return {};
    anav::PageState state{true,
                          before.production,
                          before.start_smithing,
                          before.normal_weapons,
                          before.ore_weapons,
                          c.dialog.mode == 1 ? basecat::Invocation::info
                                             : basecat::Invocation::choice,
                          -1};
    if (c.dialog.mode == 2) {
      const auto owner = ubc::read_selection_info(memory, c.dialog.info);
      if (!linked(owner))
        return {};
      state.choice_owner = owner.instance;
    }
    if (!art::same_context_evidence(before, art::context(memory, s::game_parser, primary)))
      return {};
    return ubc::crafting_page(anav::classify_page(state));
  }
  const auto kind = station_kind(c.kind);
  // All possible owner and BACK functions for the current station
  // remain bound to their original bodies, even when the source changes.
  for (const auto& recipe : scat::recipes)
    if (recipe.station == kind) {
      const auto profile = ubc::station_recipe_runtime::assemble(recipe);
      if (!ubc::station_recipe_runtime::verify(memory, s::game_parser, profile).matches)
        return {};
    }
  snav::PageState state;
  state.known = true;
  state.station = kind;
  state.production = static_cast<std::int32_t>(kind);
  state.invocation = c.dialog.mode == 1 ? basecat::Invocation::info : basecat::Invocation::choice;
  if (c.dialog.mode == 2) {
    const auto owner = ubc::read_selection_info(memory, c.dialog.info);
    if (!linked(owner))
      return {};
    state.choice_owner = owner.instance;
  }
  return ubc::crafting_page(snav::classify_page(state));
}
void capture_bow(Capture& captured, const Context& c, std::uint64_t serial, bool boundary,
                 ubc::BowMenuTrace* trace) noexcept {
  const ubc::BowRecipeKey k{serial,    GetCurrentThreadId(),   c.world, c.hero,
                            c.station, s::information_manager, 16};
  // Only the private measurement may read the independently identified hero
  // inventory when the dialog binding is rejected. Never pass it to session.
  const bool measuring = trace && trace->full;
  const bool read_boundary =
      boundary && (c.valid || (measuring && c.identity_known && c.world && c.hero && c.station));
  const auto end =
      read_boundary ? end_path(c.station) : ubc::station_end_contract::PathVerification{};
  const auto source = bow_catalogue_reader.capture(
      memory, s::game_parser, k, {read_boundary, c.valid && boundary && end.valid});
  captured.snapshot = ubc::project_capture(source, c.valid && boundary && c.dialog.npc == c.hero &&
                                                       c.dialog.player == c.hero);
  const bool modules = bow_modules_current();
  ubc::gate_bow_modules(captured.snapshot, modules);
  if (trace) {
    trace->equal("catalogue.boundary", 1, read_boundary);
    if (read_boundary)
      trace->equal("catalogue.end_path", 0, static_cast<int>(end.issue));
    else
      trace->pending("catalogue.end_path", "0", "kein Lesepunkt");
    trace->equal("catalogue.modules", 1, modules);
    ubc::bow_menu::catalogue(*trace, source, captured.snapshot);
  } else {
    log_bow_check("catalogue.capture", 0, static_cast<int>(source.diagnostic.issue));
    log_bow_check("catalogue.inventory", 0, static_cast<int>(source.diagnostic.inventory_issue));
    log_bow_check("catalogue.hands", 0, static_cast<int>(source.diagnostic.hand_issue));
  }
  captured.issue = static_cast<int>(source.diagnostic.issue);
}
bool active_extra_admission(const Context& c) noexcept {
  // Reread only before this specific, already bound original call. A pending
  // result continues to block the menu and unrelated subsequent orders.
  if (!extra_job || !extra_executor || !extra_writer_scope || !effect_authorized ||
      effect_executed || extra_executor->outcome().original_entered ||
      result_guard.state() != ubc::CraftingResultState::waiting || !native_update ||
      update_depth != 1 || info_depth != 1 || choice_depth || station_depth || collection_depth ||
      resolved_read_depth || amount_patch.active() || !own_thread() || handoff.cancelled() ||
      handoff.state() != ubc::CraftingHandoffState::executing)
    return false;
  const auto bridge = bc::diagnostics();
  const auto actual = extra_executor->scope_evidence();
  const auto& request = handoff.request();
  const auto* recipe = cat::find(request.id);
  return bridge.writer && !bridge.readers && !bridge.writes_disabled && actual.valid &&
         actual.scope.writer_active && actual.scope.writer_depth == 1 && !actual.scope.vm_depth &&
         !actual.scope.debit_depth && actual.scope.job == extra_active_job.id &&
         actual.scope.writer_token == extra_active_job.writer_token &&
         actual.scope.callback == extra_active_job.callback &&
         extra_active_job.id == request.key.generation &&
         extra_active_job.writer_token == request.key.generation &&
         extra_active_job.quantity == request.quantity && recipe && recipe->alchemy_extra &&
         recipe->alchemy_extra->callback.function->symbol == extra_active_job.callback &&
         static_cast<std::uint32_t>(recipe->alchemy_extra->owner.instance) ==
             extra_active_job.owner &&
         key(c) == request.key && xdebit::same(extra_context(c), extra_active_job.context) &&
         extra_writer_scope->call.thread == GetCurrentThreadId() &&
         extra_writer_scope->call.symbol == static_cast<int>(extra_active_job.callback) &&
         extra_writer_scope->call.caller == bc::info_call_return && current_info_effect(c);
}
bool active_spirit_admission(const Context& c) noexcept {
  // Reread only before this specific, already bound original call. A pending
  // result continues to block the menu and unrelated subsequent orders.
  if (!spirit_job || !spirit_executor || !spirit_writer_scope || !effect_authorized ||
      effect_executed || spirit_executor->outcome().original_entered ||
      result_guard.state() != ubc::CraftingResultState::waiting || !native_update ||
      update_depth != 1 || info_depth != 1 || choice_depth || station_depth || collection_depth ||
      resolved_read_depth || amount_patch.active() || !own_thread() || handoff.cancelled() ||
      handoff.state() != ubc::CraftingHandoffState::executing)
    return false;
  const auto bridge = bc::diagnostics();
  const auto actual = spirit_executor->scope_evidence();
  const auto& request = handoff.request();
  const auto* recipe = cat::find(request.id);
  return bridge.writer && !bridge.readers && !bridge.writes_disabled && actual.valid &&
         actual.scope.writer_active && actual.scope.writer_depth == 1 && !actual.scope.vm_depth &&
         !actual.scope.debit_depth && actual.scope.job == spirit_active_job.id &&
         actual.scope.writer_token == spirit_active_job.writer_token &&
         actual.scope.callback == spirit_active_job.callback &&
         spirit_active_job.id == request.key.generation &&
         spirit_active_job.writer_token == request.key.generation &&
         spirit_active_job.quantity == request.quantity && request.material_snapshot.branch_known &&
         spirit_active_job.branch_value == request.material_snapshot.branch_value && recipe &&
         recipe->spirit && recipe->spirit->callback->symbol == spirit_active_job.callback &&
         static_cast<std::uint32_t>(recipe->spirit->owner.instance) == spirit_active_job.owner &&
         key(c) == request.key && gdebit::same(extra_context(c), spirit_active_job.context) &&
         spirit_writer_scope->call.thread == GetCurrentThreadId() &&
         spirit_writer_scope->call.symbol == static_cast<int>(spirit_active_job.callback) &&
         spirit_writer_scope->call.caller == bc::info_call_return && current_info_effect(c);
}
Capture capture(const Context& c, const GrindPreviewLease* shared_grind,
                ubc::BowMenuTrace* trace) noexcept {
  const bool outer = frame_depth == 1 && !update_depth && !info_depth && !choice_depth;
  const bool callback = native_update && update_depth == 1 && info_depth + choice_depth <= 1;
  const bool boundary = (outer || callback) && !station_depth && !collection_depth;
  Capture captured;
  if (dndr_active) {
    capture_dndr(captured, c, boundary);
  } else if (c.kind == cat::Station::stove || c.kind == cat::Station::alchemy) {
    const auto kind =
        c.kind == cat::Station::stove ? basecat::Station::stove : basecat::Station::alchemy;
    std::array<bool, basecat::recipes.size()> contracts{};
    for (std::size_t i = 0; i < contracts.size(); ++i)
      contracts[i] = c.valid && basecat::recipes[i].station == kind;
    const ubc::StationKey k{generation, GetCurrentThreadId(),   c.world, c.hero,
                            c.station,  s::information_manager, kind};
    const bool shared =
        c.kind == cat::Station::alchemy && extra_catalogue_reader && spirit_catalogue_reader;
    ubc::CraftingBaseAlchemyEvidence base_evidence;
    ubc::CraftingAlchemyExtraInput shared_input;
    const auto spirit_profile = grt::assemble(gcat::recipes[0]);
    Context base_before;
    if (shared) {
      shared_input = extra_read_input(c, boundary);
      base_before = context();
      base_evidence.serial = shared_input.serial;
      base_evidence.context_before = extra_context(base_before);
      base_evidence.manager_before = base_before.dialog.valid ? s::information_manager : 0;
      const auto production =
          rt::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
      if (production.valid)
        base_evidence.production_before = production.value;
      base_evidence.actor_before = grt::actor(memory, s::game_parser, spirit_profile, c.hero);
    }
    const auto source =
        catalogue_reader.capture(memory, s::game_parser, k, c.valid && boundary, contracts);
    captured.snapshot = ubc::project_capture(source);
    captured.issue = static_cast<int>(source.diagnostic.issue);
    if (c.kind == cat::Station::alchemy && extra_catalogue_reader) {
      const auto extra = extra_catalogue_reader->capture(
          memory, shared ? shared_input : extra_read_input(c, boundary));
      if (shared) {
        const auto spirit = spirit_catalogue_reader->capture(
            memory, {shared_input, nullptr, read_spirit_actual_info});
        base_evidence.actor_after = grt::actor(memory, s::game_parser, spirit_profile, c.hero);
        const auto production =
            rt::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
        if (production.valid)
          base_evidence.production_after = production.value;
        const auto after = context();
        base_evidence.context_after = extra_context(after);
        base_evidence.manager_after = after.dialog.valid ? s::information_manager : 0;
        base_evidence.context_unchanged =
            same_context(c, base_before) && same_context(base_before, after);
        base_evidence.actor_unchanged =
            grt::same_actor_evidence(base_evidence.actor_before, base_evidence.actor_after);
        base_evidence.boundary_verified =
            shared_input.boundary.verified && base_evidence.context_unchanged;
        captured.snapshot = ubc::project_capture(source, base_evidence, extra.capture, spirit);
      } else
        captured.snapshot = ubc::project_capture(source, extra.capture);
    }
    if (c.kind == cat::Station::alchemy && boundary && c.valid &&
        ubc::valid_crafting_snapshot(captured.snapshot)) {
      ubc::GrindCheck script_check;
      const bool script = tobacco::verify_script(memory, script_check);
      for (std::size_t i = 0; i < tobacco::recipes.size(); ++i) {
        auto& state = captured.snapshot.recipes[cat::tobacco_begin + i];
        ubc::GrindCheck check = script_check;
        tobacco::Capture sample;
        if (!read_tobacco_state(c, tobacco::recipes[i], script, state, sample, check) ||
            !check.equal("preview.hands", true,
                         ubc::same_crafting_hands(captured.snapshot.hands, sample.hands)))
          state.contract = false;
        if (check.field)
          log_tobacco_check(check, state.id);
      }
      if (!ubc::crafting_detail::stocks_agree(captured.snapshot))
        captured.snapshot.issue = ubc::CraftingProjectionIssue::source;
    }
  } else if (c.kind == cat::Station::grind) {
    captured.snapshot = capture_grind(c, boundary, shared_grind);
    captured.issue = static_cast<int>(captured.snapshot.issue);
  } else if (c.kind == cat::Station::cool) {
    const auto source = capture_cool(c, boundary);
    captured.snapshot = ubc::project_capture(source, ubc::CoolActorPurpose::hero_preview,
                                             c.valid && boundary && c.dialog.npc == c.hero &&
                                                 c.dialog.player == c.hero);
    captured.issue = static_cast<int>(source.diagnostic.issue);
  } else if (c.kind == cat::Station::bow) {
    capture_bow(captured, c, generation, boundary, trace);
  } else if (c.kind == cat::Station::anvil) {
    const bool proof = c.valid && boundary && end_path(c.station).valid;
    const ubc::AnvilRecipeKey k{generation,
                                GetCurrentThreadId(),
                                c.world,
                                c.hero,
                                c.station,
                                s::information_manager,
                                acat::Station::anvil};
    const auto ordinary =
        anvil_catalogue_reader.capture(memory, s::game_parser, k, {c.valid && boundary, proof});
    const ubc::OreRecipeKey ore_key{generation,
                                    GetCurrentThreadId(),
                                    c.world,
                                    c.hero,
                                    c.station,
                                    s::information_manager,
                                    ocat::Station::anvil};
    const auto ore =
        ore_catalogue_reader.capture(memory, s::game_parser, ore_key, {c.valid && boundary, proof});
    captured.snapshot = ubc::project_capture(
        ordinary, ore, c.valid && boundary && c.dialog.npc == c.hero && c.dialog.player == c.hero);
    captured.issue =
        static_cast<int>(ordinary.diagnostic.issue) | static_cast<int>(ore.diagnostic.issue);
  } else {
    const auto kind = station_kind(c.kind);
    std::array<bool, scat::recipes.size()> contracts{};
    for (std::size_t i = 0; i < contracts.size(); ++i)
      contracts[i] = c.valid && scat::recipes[i].station == kind;
    const ubc::StationRecipeKey k{generation, GetCurrentThreadId(),   c.world, c.hero,
                                  c.station,  s::information_manager, kind};
    const auto source =
        station_catalogue_reader.capture(memory, s::game_parser, k, c.valid && boundary, contracts);
    captured.snapshot = ubc::project_capture(source);
    captured.issue = static_cast<int>(source.diagnostic.issue);
    if (c.kind == cat::Station::saw)
      ubc::project_saw_actor_preview(
          captured.snapshot, source.diagnostic.actor_before, source.diagnostic.actor_after,
          source.diagnostic.actor_unchanged,
          c.valid && boundary && c.dialog.npc == c.hero && c.dialog.player == c.hero);
  }
  if (c.valid && (c.kind == cat::Station::alchemy || c.kind == cat::Station::leather ||
                  c.kind == cat::Station::forge || c.kind == cat::Station::anvil ||
                  c.kind == cat::Station::bow || c.kind == cat::Station::cool ||
                  c.kind == cat::Station::grind)) {
    const auto& cool = captured.snapshot.recipes[cat::cool_begin];
    const auto path = end_path(c.station, cpre::runtime_ready && c.kind == cat::Station::cool &&
                                              cool.material_known && cool.maximum == 2);
    const auto issue = static_cast<int>(path.issue);
    if (issue != last_end_path_issue) {
      last_end_path_issue = issue;
      event("end_path_issue", issue);
    }
    if (!path.valid)
      for (auto& state : captured.snapshot.recipes)
        state.contract = false;
  }
#ifdef UBC_PLAYER_RELEASE
  for (std::size_t i = 0; i < player_recipes.size(); ++i)
    if (!dndr_active && !player_recipes[i]) {
      captured.snapshot.recipes[i].contract = false;
      captured.snapshot.recipes[i].known = false;
    }
#endif
  captured.hands = captured.snapshot.hands;
  const bool current = same_context(c, context());
  if (trace)
    trace->equal("catalogue.same_context", 1, current);
  if (!current)
    captured.snapshot.key = {};
  if ((!result_guard.permits_next() && !active_extra_admission(c) && !active_spirit_admission(c)) ||
      adapter_failed())
    for (auto& recipe : captured.snapshot.recipes)
      recipe.contract = false;
  if (c.kind == cat::Station::saw && !ammo::permits_next())
    for (auto& recipe : captured.snapshot.recipes)
      recipe.contract = false;
  return captured;
}
const ubc::CraftingState* state_for(const ubc::CraftingSnapshot& snapshot,
                                    cat::RecipeId id) noexcept {
  for (const auto& recipe : snapshot.recipes)
    if (recipe.id == id)
      return &recipe;
  return nullptr;
}
void observe_bow_quantity(const Context& c) noexcept {
  const auto* selected = session.selected();
  if (!selected || session.preview().key.kind != cat::Station::bow)
    return;
  const ubc::CraftingRequest request{selected->id, session.quantity(), session.preview().key,
                                     *selected, session.preview().hands};
  const auto* recipe = cat::find(request.id);
  const auto bridge = bc::diagnostics();
  bool available = false;
  const bool quiet = c.valid && key(c) == request.key && own_thread() && frame_depth == 1 &&
                     !update_depth && !info_depth && !choice_depth && !station_depth &&
                     !collection_depth && !resolved_read_depth && !bridge.writer &&
                     !bridge.readers && !bridge.writes_disabled && !amount_patch.active() &&
                     !handoff.active() && result_guard.permits_next();
  if (quiet && recipe && recipe->bow && bow_modules_current() &&
      ubc::crafting_handoff_detail::valid_request(request) && end_path(c.station).valid) {
    const auto proof = bow_item_reader->capture(memory, *recipe->bow, request.quantity,
                                                bow_read_input(request.key));
    log_bow_check("preview.item_path", 0, static_cast<int>(proof.issue),
                  static_cast<int>(request.id), request.quantity, proof.failed_address);
    available = ubc::matching_bow_item_path(proof, request, frame) && bow_modules_current() &&
                same_context(c, context());
  }
  session.observe_quantity_check(request, frame, available);
  grind_quantity_observation = {true, request.key.generation, frame, request.id, request.quantity};
}
bool cool_predebit_available(std::uintptr_t, std::uintptr_t) noexcept {
  if (!cool_predebit_executor || !cool_predebit_reader || cool_predebit_tracker.status().blocked ||
      bc::quarantined(cpre::callback) || !cool_engine_current() || !end_compatibility(true) ||
      !extra_zpe_module.current() || !extra_crt_module.current())
    return false;
  const auto d = cool_predebit_executor->diagnostics();
  return d.configured && !d.disabled && !d.active && !d.native_scope;
}
void observe_cool_quantity(const Context& c) noexcept {
  const auto* selected = session.selected();
  if (!selected || session.preview().key.kind != cat::Station::cool)
    return;
  const ubc::CraftingRequest request{selected->id, session.quantity(), session.preview().key,
                                     *selected, session.preview().hands};
  const auto bridge = bc::diagnostics();
  bool available = false;
  const bool quiet = c.valid && key(c) == request.key && own_thread() && frame_depth == 1 &&
                     !update_depth && !info_depth && !choice_depth && !station_depth &&
                     !collection_depth && !resolved_read_depth && !bridge.writer &&
                     !bridge.readers && !bridge.writes_disabled && !amount_patch.active() &&
                     !handoff.active() && result_guard.permits_next() && !cool_destroy_depth;
  const auto plan = ubc::crafting_cool_plan(request.material_snapshot, request.quantity);
  const bool predebit = plan.valid && plan.requires_extra_debit;
  if (quiet && cool_runtime_path(predebit).valid && end_path(c.station, predebit).valid) {
    const auto collection = rows.generation;
    const auto fresh = capture(c);
    available = ubc::cool_quantity_available(
                    request, fresh.snapshot, cool_info_page(c, true),
                    []() noexcept { return PreviewRecording::probe(cool_predebit_available); }) &&
                collection == rows.generation && same_context(c, context());
  }
  session.observe_quantity_check(request, frame, available);
}
} // namespace ubc_native
