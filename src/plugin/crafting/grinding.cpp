// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
GrindPreviewScope grind_preview_scope;

ubc::GrindRecipeKey grind_key(const Context& c) noexcept {
  return {generation, frame,     GetCurrentThreadId(),   c.world,
          c.hero,     c.station, s::information_manager, 11};
}
GrindCaptureLease::GrindCaptureLease(const ubc::GrindRecipeKey& k, bool boundary) noexcept {
  if (!own_thread() || k.thread != GetCurrentThreadId() || k.frame != frame || grind_capture_leased)
    return;
  if (grind_preview_scope.active) {
    grind_preview_scope.failed = true;
    return;
  }
  grind_capture_leased = true;
  value = ::new (grind_capture_storage.data()) ubc::GrindRecipeCatalogueCapture(
      grind_catalogue_reader.capture(memory, s::game_parser, k, boundary));
}

GrindCaptureLease::~GrindCaptureLease() {
  if (value) {
    value->~GrindRecipeCatalogueCapture();
    grind_capture_leased = false;
  }
}

const ubc::GrindRecipeCatalogueCapture*
GrindCaptureLease::view(const ubc::GrindRecipeKey& k) const noexcept {
  return value && grind_capture_leased &&
                 static_cast<const void*>(value) == grind_capture_storage.data() && own_thread() &&
                 k.thread == GetCurrentThreadId() && k.frame == frame &&
                 ubc::grind_recipe_snapshot_detail::same_key(value->key, k)
             ? value
             : nullptr;
}
bool grind_frame_capture_quiet() noexcept {
  const auto bridge = bc::diagnostics();
  return own_thread() && frame_depth == 1 && !update_depth && !info_depth && !choice_depth &&
         !station_depth && !collection_depth && !resolved_read_depth && !bridge.writer &&
         !bridge.readers && !bridge.writes_disabled && !amount_patch.active() &&
         !handoff.active() && result_guard.permits_next() && !cool_destroy_depth;
}
std::uint64_t next_grind_serial() noexcept {
  if (grind_serial == UINT64_MAX) {
    disabled = true;
    return 0;
  }
  return ++grind_serial;
}
bool grind_modules_current() noexcept {
  return grind_initialized && grind_item_reader && grind_renderer_module.current() &&
         extra_zpe_module.current() && extra_crt_module.current() && end_zbug_module.current() &&
         end_union_module.current() && (!grind_locator_bound || grind_locator_module.current());
}
shitem::Input grind_read_input(const ubc::GrindRecipeKey& k, bool boundary) noexcept {
  shitem::Input out{{k, GetCurrentThreadId(), boundary},
                    extra_modules(),
                    {grind_renderer_module.base(), grind_renderer_module.image_size(),
                     grind_renderer_module.digest(), true}};
  if (grind_locator_bound && grind_locator_module.current() && end_union_module.current())
    out.inventory_insert = {
        {grind_locator_module.base(), grind_locator_module.image_size(),
         grind_locator_module.digest(), true},
        {end_union_module.base(), end_union_module.image_size(), end_union_module.digest(), true}};
  return out;
}
void initialize_grind(const Context& c) noexcept {
  if (grind_initialized || !c.valid || c.kind != cat::Station::grind || !own_thread() ||
      frame_depth != 1 || update_depth || info_depth || choice_depth || station_depth ||
      collection_depth || handoff.active() || amount_patch.active())
    return;
  grind_initialized = true;
  grind_item_reader.emplace(GetCurrentThreadId());
  grind_preview_reader.emplace(GetCurrentThreadId(), grind_catalogue_reader, *grind_item_reader);
  const bool modules = initialize_item_modules();
  const bool renderer = grind_renderer_module.acquire(
      L"g2a_avx2.dll", ubc::bow_item_renderer::image_size, ubc::bow_item_renderer::file_sha256);
  const auto& locator = ubc::vob_locator_compat::module_profiles[0];
  // Without an actual module lease, only the original slot is
  // admissible. The code check still detects an unknown hook.
  grind_locator_bound = grind_locator_module.acquire(L"UNION_VOB_LOCATOR.DLL", locator.image_size,
                                                     locator.file_sha256);
  ubc::GrindCheck check;
  if (!check.equal("modules.item", true, modules) ||
      !check.equal("modules.renderer", true, renderer))
    log_grind_check(check, 0);
  event(modules && renderer ? "grind_modules_verified" : "grind_modules_rejected",
        grind_locator_bound ? 1 : 0);
}
ubc::GrindMaterialProofs grind_station_proof(const Context& c,
                                             const ubc::GrindRecipeCatalogueCapture& raw,
                                             bool boundary) noexcept {
  ubc::GrindMaterialProofs out;
  out.stable_boundary_verified =
      boundary && c.valid && c.kind == cat::Station::grind && own_thread() &&
      ubc::grind_recipe_snapshot_detail::same_key(raw.key, grind_key(c)) &&
      raw.diagnostic.inventory_unchanged && raw.diagnostic.hands_unchanged;
  if (out.stable_boundary_verified && cool_runtime_path().valid && end_path(c.station).valid &&
      matches_station_profile(c.station, cat::Station::grind) &&
      shdestroy::detail::active_grind_hand(raw.hands)) {
    out.station_hand_end_verified = true;
    out.verified_left_tmp_level = raw.hands.left_tmp_level;
  }
  return out;
}
// The thread ID is constant for the lifetime of this thread.
// TLS is initialized on scope entry. The millions of subsequent
// local scope checks perform no OS, engine or module reads.
std::uint32_t grind_preview_current_thread() noexcept {
  static thread_local const std::uint32_t actual = GetCurrentThreadId();
  return actual;
}
bool grind_scope_quiet(std::uint32_t thread, const bc::Diagnostics& bridge) noexcept {
  return ReadAcquire(&installed) && ReadAcquire(&engine_thread) == static_cast<LONG>(thread) &&
         frame_depth == 1 && !update_depth && !info_depth && !choice_depth && !station_depth &&
         !collection_depth && !resolved_read_depth && !grind_preview_input_depth &&
         !bridge.readers && !bridge.writes_disabled && !handoff.active() &&
         result_guard.permits_next() && !cool_destroy_depth && !grind_capture_leased;
}
bool observe_grind_preview_scope(void* object, shpreview::FrameObservation& out) noexcept {
  out = {};
  if (object != &grind_preview_scope || !grind_preview_scope.active || grind_preview_scope.failed ||
      grind_preview_overflow)
    return false;
  const auto thread = grind_preview_current_thread();
  const auto& bound = grind_preview_scope.bound;
  const auto bridge = bc::diagnostics();
  out.key = {
      generation, frame, thread, bound.world, bound.hero, bound.station, s::information_manager,
      11};
  out.scope_serial = grind_preview_scope.serial;
  out.current_thread = thread;
  out.outer_depth = frame_depth;
  out.input_serial = grind_preview_input_serial;
  out.active_scope = grind_preview_scope.active;
  out.writer = bridge.writer || amount_patch.active();
  out.quiet = grind_scope_quiet(thread, bridge);
  return true;
}
// The expected frame passed begin() with quiet=true, writer=false, depth=1
// and an active scope. Compare each live binding without copying a temporary.
bool matches_grind_preview_scope(void* object,
                                 const shpreview::FrameObservation& expected) noexcept {
  const auto& scope = grind_preview_scope;
  const auto& k = expected.key;
  const auto thread = grind_preview_current_thread();
  const auto bridge = bc::diagnostics();
  return object == &scope && scope.active && !scope.failed && !grind_preview_overflow &&
         scope.serial == expected.scope_serial &&
         grind_preview_input_serial == expected.input_serial && generation == k.generation &&
         frame == k.frame && thread == k.thread && scope.bound.world == k.world &&
         scope.bound.hero == k.hero && scope.bound.station == k.station &&
         k.manager == s::information_manager && k.production_kind == 11 && !bridge.writer &&
         !amount_patch.active() && grind_scope_quiet(thread, bridge);
}
bool grind_preview_station_path(const Context& c) noexcept {
  return cool_runtime_path().valid && end_path(c.station).valid &&
         matches_station_profile(c.station, cat::Station::grind);
}
bool same_grind_preview_input(const shitem::Input& a, const shitem::Input& b) noexcept {
  const auto same = [](const auto& x, const auto& y) noexcept {
    return x.base == y.base && x.image_size == y.image_size && x.file_sha256 == y.file_sha256 &&
           x.mapping_verified == y.mapping_verified;
  };
  return ubc::grind_recipe_snapshot_detail::same_key(a.boundary.key, b.boundary.key) &&
         a.boundary.current_thread == b.boundary.current_thread &&
         a.boundary.stable_boundary_verified == b.boundary.stable_boundary_verified &&
         same(a.modules.zbug, b.modules.zbug) && same(a.modules.union_core, b.modules.union_core) &&
         same(a.modules.parser_extender, b.modules.parser_extender) &&
         same(a.modules.runtime_crt, b.modules.runtime_crt) && same(a.renderer, b.renderer) &&
         same(a.inventory_insert.locator, b.inventory_insert.locator) &&
         same(a.inventory_insert.union_core, b.inventory_insert.union_core);
}
GrindPreviewLease::GrindPreviewLease(const Context& c, bool include_selection) noexcept {
  if (!check.equal("preview.reentry", false, grind_preview_scope.active)) {
    grind_preview_scope.failed = true;
    return;
  }
  if (!check.equal("preview.context", true, c.valid) ||
      !check.equal("preview.station", static_cast<int>(cat::Station::grind),
                   static_cast<int>(c.kind)) ||
      !check.equal("preview.quiet", true, grind_frame_capture_quiet()) ||
      !check.equal("preview.leased", false, grind_capture_leased) ||
      !check.equal("preview.input_depth", 0, grind_preview_input_depth) ||
      !check.equal("preview.overflow", false, grind_preview_overflow) ||
      !check.equal("preview.reader", true, grind_preview_reader.has_value()))
    return;
  if (!check.equal("preview.serial_exhausted", false, grind_preview_serial == UINT64_MAX)) {
    grind_preview_overflow = true;
    return;
  }
  owner = true;
  grind_preview_scope = {c, ++grind_preview_serial, true, false};
  failure_stage = 2;
  shpreview::FrameObservation expected;
  if (!check.equal("preview.scope", true,
                   observe_grind_preview_scope(&grind_preview_scope, expected)) ||
      !check.equal("preview.frame", true,
                   shpreview::detail::valid_frame(expected, GetCurrentThreadId())) ||
      !check.equal("preview.initial_context", true, same_context(c, context())) ||
      !check.equal("preview.initial_modules", true, grind_modules_current()))
    return;
  const auto before = grind_read_input(expected.key, true);
  shpreview::Selection choice;
  choice.station_path_verified = grind_preview_station_path(c);
  choice.hero_dialog_verified = c.dialog.npc == c.hero && c.dialog.player == c.hero;
  if (const auto* selected = include_selection ? session.selected() : nullptr)
    if (const auto* recipe = cat::find(selected->id))
      if (recipe->grind)
        choice.current = {recipe->grind->function.symbol, session.quantity()};
  failure_stage = 3;
  const bool captured = grind_preview_reader->capture_selection(
      memory, {&grind_preview_scope, observe_grind_preview_scope, matches_grind_preview_scope},
      expected, before, choice);
  statistics = grind_preview_reader->statistics();
  if (!captured) {
    read_issue = static_cast<int>(grind_preview_reader->issue());
    check.equal("preview.reader_issue", static_cast<int>(shpreview::Issue::none), read_issue);
    return;
  }
  // Actual engine and module boundaries, checked separately from the inexpensive local probes.
  failure_stage = 4;
  if (!check.equal("preview.final_context", true, same_context(c, context())) ||
      !check.equal("preview.final_modules", true, grind_modules_current()) ||
      !check.equal("preview.input", true,
                   same_grind_preview_input(before, grind_read_input(expected.key, true))) ||
      !check.equal("preview.station_path", choice.station_path_verified,
                   grind_preview_station_path(c)) ||
      !check.equal("preview.publish_context", true, same_context(c, context())))
    return;
  failure_stage = 5;
  ready = grind_preview_reader->publish(grind_preview_publication);
  if (ready)
    failure_stage = 0;
  else {
    read_issue = static_cast<int>(grind_preview_reader->issue());
    check.equal("preview.publication_issue", static_cast<int>(shpreview::Issue::none), read_issue);
  }
}

GrindPreviewLease::~GrindPreviewLease() {
  if (owner) {
    grind_preview_scope.active = false;
    ready = false;
    if (grind_preview_reader)
      grind_preview_reader->close();
  }
}

const shpreview::Publication* GrindPreviewLease::view(const ubc::GrindRecipeKey& k) const noexcept {
  shpreview::FrameObservation current;
  return owner && ready &&
                 ubc::grind_recipe_snapshot_detail::same_key(k, grind_preview_publication.key) &&
                 observe_grind_preview_scope(&grind_preview_scope, current) &&
                 shpreview::detail::same_frame(current, grind_preview_publication.scope)
             ? &grind_preview_publication
             : nullptr;
}
bool report_grind_preview(const GrindPreviewLease& value) noexcept {
  // An unsuccessful menu opening receives a new generation on every frame.
  if (value.ready) {
    grind_preview_failure = {};
    return true;
  }
  if (!grind_preview_failure.first(frame, value.failure_stage, value.read_issue, value.check))
    return false;
  log_grind_check(value.check, value.failure_stage);
  event("grind_preview_batch_rejected", value.failure_stage, value.read_issue);
  event("grind_preview_batch_bounds", static_cast<int>(value.statistics.spans),
        static_cast<int>(value.statistics.stored_bytes));
  return true;
}
// Dedicated one-shot measurement mode. No session refresh, menu order or
// handoff. Take five fresh, fully sealed snapshots. Snapshot 5 observes
// the actual whole-stack path without crafting to produce a remainder.
void measure_grind_preview(const Context& c) noexcept {
  if (execution_enabled || grind_measurement_case >= 5 || !c.valid ||
      c.kind != cat::Station::grind || !c.dialog.waiting || !c.dialog.open ||
      !complete_collection(c) || !grind_frame_capture_quiet() || grind_preview_scope.active ||
      grind_capture_leased || grind_preview_input_depth || grind_preview_overflow ||
      !grind_item_reader || !grind_modules_current())
    return;
  if (grind_preview_serial == UINT64_MAX) {
    grind_preview_overflow = true;
    return;
  }
  if (!grind_measurement_case) {
    if (!ubc::grind_preview_measurement::start_generation(generation)) {
      grind_preview_overflow = true;
      return;
    }
    initialize_cool(c);
  }
  const unsigned index = ++grind_measurement_case;
  const std::array<std::uint32_t, 5> quantities{{1, 2, 5, 3, grind_measurement_whole}};
  const auto quantity = quantities[index - 1];
  event("grind_measure_case", index, quantity);
  bool complete = false;
  int stage = 1;
  grind_preview_scope = {c, ++grind_preview_serial, true, false};
  try {
    shpreview::FrameObservation expected;
    if (observe_grind_preview_scope(&grind_preview_scope, expected) &&
        shpreview::detail::valid_frame(expected, GetCurrentThreadId()) &&
        same_context(c, context()) && quantity) {
      stage = 2;
      const auto input = grind_read_input(expected.key, true);
      const shpreview::Selection choice{{20975, quantity},
                                        grind_preview_station_path(c),
                                        c.dialog.npc == c.hero && c.dialog.player == c.hero};
      auto reader = std::make_unique<ubc::grind_preview_measurement::Reader>(
          GetCurrentThreadId(), grind_catalogue_reader, *grind_item_reader);
      auto result = std::make_unique<shpreview::Publication>();
      const auto started = GetTickCount64();
      const bool captured = reader->capture_selection(
          memory, {&grind_preview_scope, observe_grind_preview_scope}, expected, input, choice);
      const auto elapsed = GetTickCount64() - started;
      complete = captured && reader->diagnostic_result(*result) && same_context(c, context()) &&
                 grind_modules_current() &&
                 same_grind_preview_input(input, grind_read_input(expected.key, true)) &&
                 choice.station_path_verified == grind_preview_station_path(c);
      if (complete)
        stage = 3;
      const auto& reads = reader->read_set();
      const auto& stats = reads.statistics();
      const auto& rejected = reads.rejected();
      event("grind_measure_issue", static_cast<int>(reader->issue()), static_cast<int>(elapsed));
      event("grind_measure_bounds", static_cast<int>(stats.spans),
            static_cast<int>(stats.stored_bytes));
      const auto united = reads.union_size();
      event("grind_measure_union", static_cast<int>(united.first), static_cast<int>(united.second));
      event("grind_measure_calls", static_cast<int>(stats.recording_calls),
            static_cast<int>(stats.recording_bytes));
      event("grind_measure_replay", static_cast<int>(stats.replay_calls),
            static_cast<int>(stats.replay_bytes));
      event("grind_measure_largest", static_cast<int>(stats.largest_span),
            static_cast<int>(stats.overlaps));
      event("grind_measure_old_issue", static_cast<int>(rejected.issue), 0);
      event("grind_measure_old_bounds", static_cast<int>(rejected.spans),
            static_cast<int>(rejected.bytes));
      event("grind_measure_old_request", chdiag::signed_word(rejected.address),
            static_cast<int>(rejected.length));
      for (const auto& range : reads.ranges())
        event("grind_measure_span", chdiag::signed_word(range.first),
              static_cast<int>(range.second));
      if (complete) {
        const auto* selected = shpreview::find(*result, 20975, quantity);
        complete = selected && selected->world.stable;
        if (selected) {
          event("grind_measure_world", selected->world.nodes[0], selected->world.nodes[1]);
          event("grind_measure_inventory", result->catalogue.diagnostic.listed_nodes,
                result->catalogue.diagnostic.packed_entries);
          event("grind_measure_stock", selected->stock.materials[0].total,
                result->catalogue.diagnostic.state_before.skill);
          if (index == 1)
            grind_measurement_whole = selected->stock.materials[0].total;
        }
      }
    }
  } catch (...) {
    complete = false;
  }
  grind_preview_scope.active = false;
  event("grind_measure_stage", complete ? 0 : stage, static_cast<int>(generation));
  event("grind_measure_end", index, complete ? 1 : 0);
  if (!complete)
    grind_measurement_case = 5; // Never retry incomplete snapshots.
}
void grind_preview_item_diagnostic(std::size_t index, const ubc::GrindRecipeKey& k, int issue,
                                   std::uint32_t address) noexcept {
  auto& last = grind_preview_diagnostic[index];
  if (issue >= 0 &&
      (last.generation != k.generation || last.issue != issue || last.address != address)) {
    const auto id = cat::recipes[cat::grind_begin + index].id;
    ubc::GrindCheck check;
    check.equal("preview.item_issue", static_cast<int>(shitem::Issue::none), issue);
    log_grind_check(check, static_cast<int>(id));
    event("grind_preview_item_issue", static_cast<int>(id), issue);
    if (address)
      event("grind_preview_item_address", static_cast<int>(id), chdiag::signed_word(address));
  }
  last = {k.generation, issue, address};
}
ubc::CraftingSnapshot capture_grind(const Context& c, bool boundary,
                                    const GrindPreviewLease* shared) noexcept {
  const auto k = grind_key(c);
  if (shared) {
    const auto* closed = c.valid && boundary ? shared->view(k) : nullptr;
    if (!closed)
      return {};
    for (std::size_t i = 0; i < closed->count; ++i)
      if (closed->requests[i].quantity == 1)
        for (std::size_t j = 0; j < shcat::recipes.size(); ++j)
          if (closed->requests[i].callback == shcat::recipes[j].function.symbol)
            grind_preview_item_diagnostic(
                j, k, closed->blocked[i].blocked ? static_cast<int>(closed->blocked[i].phase) : -1,
                closed->blocked[i].address);
    return closed->snapshot;
  }
  const GrindCaptureLease lease{k, c.valid && boundary};
  if (!lease.value)
    return {};
  const auto& raw = *lease.value;
  ubc::GrindProjectionInput input;
  input.key = k;
  input.scope = ubc::grind_item_path::Scope::preview;
  input.physical = ubc::read_grind_guild_for_preview(memory, s::game_parser, k);
  input.station = grind_station_proof(c, raw, boundary);
  input.physical_hero_preview_verified =
      c.valid && boundary && c.dialog.npc == c.hero && c.dialog.player == c.hero;
  const auto candidates = ubc::project_capture(raw, input);
  if (grind_modules_current() && ubc::valid_crafting_snapshot(candidates))
    for (std::size_t i = 0; i < shcat::recipes.size(); ++i) {
      const auto& row = candidates.recipes[cat::grind_begin + i];
      if (!shpreview::candidate(row))
        continue;
      const auto item = grind_item_reader->capture_preview(
          memory, raw, input.physical, shcat::recipes[i].function.symbol, 1,
          grind_read_input(k, input.station.stable_boundary_verified));
      grind_preview_item_diagnostic(
          i, k, item.item_path_verified ? -1 : static_cast<int>(item.issue), item.failed_address);
      input.materials[i] = ubc::bind_grind_material(raw, item, input.station);
    }
  auto out = ubc::project_capture(raw, input);
  const auto guild_after = ubc::read_grind_guild_for_preview(memory, s::game_parser, k);
  if (!same_context(c, context()) ||
      !ubc::grind_guild_snapshot_detail::same(input.physical, guild_after) ||
      !shrt::same_state(raw.diagnostic.state_before,
                        shrt::state(memory, s::game_parser, shrt::assemble(shcat::recipes[0]))))
    out.key = {};
  return out;
}
bool matching_grind_item(const shitem::Evidence& item, const ubc::CraftingRequest& request,
                         ubc::grind_item_path::Scope purpose) noexcept {
  const auto* recipe = cat::find(request.id);
  const auto c = context();
  if (!recipe || !recipe->grind || request.key.kind != cat::Station::grind || !c.valid ||
      !(key(c) == request.key) ||
      !shitem::matches(item, grind_key(c), recipe->grind->function.symbol, request.quantity,
                       purpose) ||
      !ubc::same_crafting_hands(item.stock.input.hands, request.hands) || !grind_modules_current())
    return false;
  const auto wanted = ubc::crafting_grind_plan(request.material_snapshot, request.quantity);
  if (!wanted.valid)
    return false;
  for (std::size_t i = 0; i < 3; ++i)
    if (item.stock.materials[i].symbol != request.material_snapshot.materials[i].symbol ||
        item.stock.materials[i].total != request.material_snapshot.materials[i].stored ||
        item.expected_after_callback[i] != wanted.after_callback[i].amount)
      return false;
  return true;
}
bool matching_grind_knowledge(const ubc::GrindMaterialKnowledge& actual,
                              const ubc::CraftingRequest& request) noexcept {
  const auto& wanted = request.material_snapshot.grind;
  return wanted.known && actual.talent_known && actual.guild_known && actual.skill_known &&
         actual.talent == wanted.talent && actual.guild == wanted.guild &&
         actual.skill == wanted.skill;
}
// Result and restore read the actual globals and the bound physical guild.
// Production may already be 0 after the original call. This does not create a new
// station entry, and the old SELF preview does not qualify as writer evidence.
ubc::CraftingGrindValues read_grind_result_values(const ubc::CraftingRequest& request) noexcept {
  ubc::CraftingGrindValues out;
  const auto* recipe = cat::find(request.id);
  const auto before = context();
  if (!recipe || !recipe->grind || !own_thread() || !before.identity_known ||
      !before.station_known || before.world != request.key.world ||
      before.hero != request.key.hero || request.key.thread != GetCurrentThreadId() ||
      request.key.generation != generation || amount_patch.active())
    return out;
  const auto assembly = shrt::assemble(*recipe->grind);
  const auto state = shrt::state(memory, s::game_parser, assembly);
  auto k = grind_key(before);
  k.station = request.key.station;
  const auto guild = ubc::read_grind_guild_for_preview(memory, s::game_parser, k);
  const auto after_state = shrt::state(memory, s::game_parser, assembly);
  const auto after = context();
  if (!state.read_valid || !shrt::same_state(state, after_state) || !guild.known() ||
      !rt::detail::same_layout(state.layout, guild.layout) || !after.identity_known ||
      !after.station_known || after.world != before.world || after.hero != before.hero ||
      after.station != before.station ||
      !shrt::verify_recipe(memory, s::game_parser, *recipe->grind).valid)
    return out;
  return {true, state.talent, guild.guild, state.skill};
}
bool current_grind_choice(const Context& c) noexcept {
  const auto& lease = grind_choice_lease;
  const auto* ticket = handoff.current_ticket();
  const auto* step = handoff.step();
  if (!lease.owner || !own_thread() || lease.thread != GetCurrentThreadId() || lease.depth != 1 ||
      lease.frame_number != frame || !effect_authorized || !native_update || update_depth != 1 ||
      choice_depth != 1 || info_depth || station_depth || collection_depth || resolved_read_depth ||
      !post_remove_choice.active() || handoff.cancelled() ||
      handoff.state() != ubc::CraftingHandoffState::executing || !ticket || !step ||
      !step->is_effect() || step->invocation != basecat::Invocation::choice ||
      !ubc::crafting_handoff_detail::same_ticket(lease.ticket, *ticket) ||
      !ubc::crafting_handoff_detail::same_ticket(lease.ticket, dispatched_ticket) ||
      !(lease.key == handoff.request().key) || !c.valid || c.kind != cat::Station::grind ||
      !(key(c) == lease.key) || c.game != lease.game || c.dialog.mode != 2 ||
      c.dialog.info != lease.owner || c.dialog.view != ticket->view ||
      ticket->offer.owner != 20957 || rows.generation != ticket->offer.collection ||
      !shnav::valid_page(lease.page) || ubc::crafting_page(lease.page) != step->before)
    return false;
  const auto owner = ubc::read_selection_info(memory, lease.owner);
  return linked(owner) && owner.instance == 20957;
}
shnav::PageEvidence grind_page(const Context& c) noexcept {
  if (!c.valid || c.kind != cat::Station::grind)
    return {};
  const auto& functions = ubc::grind_menu_metadata::functions;
  const auto& bindings = ubc::grind_recipe_profile::common_bindings;
  const ubc::lhiver::Profile menu_profile{layout_profile.parser_address,
                                          layout_profile.symbol_count,
                                          layout_profile.code_length,
                                          layout_profile.code_sha256,
                                          functions.data(),
                                          functions.size(),
                                          bindings.data(),
                                          bindings.size()};
  const auto assembly = shrt::assemble(shcat::recipes[0]);
  if (!rt::verify_local_contract(memory, s::game_parser, menu_profile).matches ||
      !shrt::verify(memory, s::game_parser, assembly).matches)
    return {};
  const auto before = shrt::state(memory, s::game_parser, assembly);
  if (!before.station_postcondition())
    return {};
  shnav::PageEvidence out;
  if (current_grind_choice(c))
    out = grind_choice_lease.page;
  else if (c.dialog.mode == 1) {
    if (!complete_collection(c) || !rows.count || rows.count > 6)
      return {};
    std::array<shnav::InfoOffer, 6> offers{};
    for (std::size_t i = 0; i < rows.count; ++i) {
      const auto& row = rows.rows[i];
      if (!linked(row))
        return {};
      offers[i] = {static_cast<std::uint32_t>(row.instance),
                   static_cast<std::uint32_t>(row.condition),
                   static_cast<std::uint32_t>(row.function)};
    }
    out = shnav::identify_info(before.production, offers.data(), rows.count, true);
  } else if (c.dialog.mode == 2) {
    ubc::SelectionChoices choices;
    if (!complete_collection(c, &choices) || choices.owner.instance != 20957)
      return {};
    std::array<std::uint32_t, 13> callbacks{};
    if (choices.count > callbacks.size())
      return {};
    for (std::size_t i = 0; i < choices.count; ++i) {
      if (choices.functions[i] < 0)
        return {};
      callbacks[i] = static_cast<std::uint32_t>(choices.functions[i]);
    }
    out = shnav::identify_choices(before.production, 20957, callbacks.data(), choices.count, true);
  }
  return shrt::same_state(before, shrt::state(memory, s::game_parser, assembly))
             ? out
             : shnav::PageEvidence{};
}
void observe_grind_quantity(const Context& c, const GrindPreviewLease* shared) noexcept {
  const auto* selected = session.selected();
  if (!selected || session.preview().key.kind != cat::Station::grind)
    return;
  grind_quantity_observation = {};
  const ubc::CraftingRequest request{selected->id, session.quantity(), session.preview().key,
                                     *selected, session.preview().hands};
  const auto* recipe = cat::find(request.id);
  const auto bridge = bc::diagnostics();
  bool available = false;
  const bool quiet = c.valid && key(c) == request.key && own_thread() && frame_depth == 1 &&
                     !update_depth && !info_depth && !choice_depth && !station_depth &&
                     !collection_depth && !resolved_read_depth && !bridge.writer &&
                     !bridge.readers && !bridge.writes_disabled && !amount_patch.active() &&
                     !handoff.active() && result_guard.permits_next() && !cool_destroy_depth;
  if (shared) {
    const auto* closed = quiet ? shared->view(grind_key(c)) : nullptr;
    available = closed && ubc::crafting_handoff_detail::valid_request(request) &&
                shpreview::quantity_matches(*closed, request);
    session.observe_quantity_check(request, frame, available);
    grind_quantity_observation = {true, request.key.generation, frame, request.id,
                                  request.quantity};
    return;
  }
  if (quiet && recipe && recipe->grind && grind_modules_current() &&
      ubc::crafting_handoff_detail::valid_request(request)) {
    const auto k = grind_key(c);
    const GrindCaptureLease lease{k, true};
    if (!lease.value) {
      session.observe_quantity_check(request, frame, false);
      grind_quantity_observation = {true, request.key.generation, frame, request.id,
                                    request.quantity};
      return;
    }
    const auto& raw = *lease.value;
    const auto physical = ubc::read_grind_guild_for_preview(memory, s::game_parser, k);
    const auto station = grind_station_proof(c, raw, true);
    if (station.station_hand_end_verified &&
        matching_grind_knowledge(ubc::grind_preview_knowledge(raw, physical, k), request)) {
      const auto item = grind_item_reader->capture_preview(
          memory, raw, physical, recipe->grind->function.symbol, request.quantity,
          grind_read_input(k, station.stable_boundary_verified));
      available = matching_grind_item(item, request, ubc::grind_item_path::Scope::preview) &&
                  same_context(c, context());
    }
  }
  session.observe_quantity_check(request, frame, available);
  grind_quantity_observation = {true, request.key.generation, frame, request.id, request.quantity};
}
bool verify_grind_amount(const bc::WriterScope& scope, const cat::Descriptor& recipe) noexcept {
  const auto& request = handoff.request();
  const auto c = context();
  const auto bridge = bc::diagnostics();
  auto& check = amount_diagnostic.grind_check;
  if (!check.equal("writer.descriptor", true, recipe.grind != nullptr) ||
      !check.equal("writer.route", static_cast<int>(cat::Route::grind),
                   static_cast<int>(recipe.route)) ||
      !check.equal("writer.modules", true, grind_modules_current()) ||
      !check.equal("writer.context", true, c.valid) ||
      !check.equal("writer.station", static_cast<int>(cat::Station::grind),
                   static_cast<int>(c.kind)) ||
      !check.equal("writer.request_key", true, key(c) == request.key) ||
      !check.equal("writer.exclusive", true, bridge.writer) ||
      !check.equal("writer.readers", 0, bridge.readers) ||
      !check.equal("writer.disabled", false, bridge.writes_disabled) ||
      !check.equal("writer.original", true, scope.original != nullptr) ||
      !check.equal("writer.thread", GetCurrentThreadId(), scope.call.thread) ||
      !check.equal("writer.caller", bc::choice_call_return, scope.call.caller) ||
      !check.equal("writer.callback", recipe.grind->function.symbol, scope.call.symbol) ||
      !check.equal("writer.choice", true, current_grind_choice(c)) ||
      !check.equal("writer.native_update", true, native_update) ||
      !check.equal("writer.update_depth", 1, update_depth) ||
      !check.equal("writer.choice_depth", 1, choice_depth) ||
      !check.equal("writer.info_depth", 0, info_depth) ||
      !check.equal("writer.station_depth", 0, station_depth) ||
      !check.equal("writer.collection_depth", 0, collection_depth) ||
      !check.equal("writer.read_depth", 0, resolved_read_depth) ||
      !check.equal("writer.active_patch", false, amount_patch.active()) ||
      !check.equal("writer.destroy_depth", 0, cool_destroy_depth))
    return false;
  const auto k = grind_key(c);
  const GrindCaptureLease lease{k, true};
  if (!check.equal("writer.catalogue", true, lease.value != nullptr))
    return false;
  const auto& raw = *lease.value;
  const auto station = grind_station_proof(c, raw, true);
  if (!check.equal("writer.station_hand_end", true, station.station_hand_end_verified))
    return false;
  const auto item = grind_item_reader->capture_writer(
      memory, raw, recipe.grind->function.symbol, request.quantity,
      grind_read_input(k, station.stable_boundary_verified));
  if (!item.item_path_verified) {
    check.equal("writer.item_issue", static_cast<int>(shitem::Issue::none),
                static_cast<int>(item.issue));
    event("grind_item_path_rejected", static_cast<int>(item.issue),
          static_cast<int>(request.quantity));
    return false;
  }
  if (!check.equal("writer.item_binding", true,
                   matching_grind_item(item, request, ubc::grind_item_path::Scope::writer)) ||
      !check.equal(
          "writer.knowledge", true,
          matching_grind_knowledge(ubc::grind_writer_knowledge(raw, item.writer, k), request)) ||
      !check.equal("writer.handed_hands", true,
                   ubc::same_crafting_hands(item.stock.input.hands, handed_hands)))
    return false;
  const auto after = ubc::read_grind_guild_for_writer(memory, s::game_parser, k);
  const auto assembly = shrt::assemble(*recipe.grind);
  if (!check.equal("writer.guild_read", true, after.known()) ||
      !check.equal("writer.guild_stable", true,
                   ubc::grind_guild_snapshot_detail::same(item.writer.physical, after.physical)) ||
      !check.equal("writer.actor", true,
                   ubc::station_recipe_runtime::same_actor_evidence(item.writer.actor_after,
                                                                    after.actor_after)) ||
      !check.equal("writer.parser", true,
                   shrt::same_state(raw.diagnostic.state_before,
                                    shrt::state(memory, s::game_parser, assembly))) ||
      !check.equal("writer.final_choice", true, current_grind_choice(c)) ||
      !check.equal("writer.final_modules", true, grind_modules_current()) ||
      !check.equal("writer.final_context", true, same_context(c, context())))
    return false;
  // late_gate has already consumed the PostRemoveChoice evidence exactly once. This
  // snapshot refreshes material, knowledge and SELF without consuming it again.
  const auto before = ubc::crafting_grind_evidence(request.material_snapshot);
  const auto numerical = shcat::plan(*recipe.grind, before, request.quantity);
  if (!numerical.valid) {
    check.equal("writer.plan_issue", static_cast<int>(shcat::Issue::none),
                static_cast<int>(numerical.issue));
    return false;
  }
  grind_prepared = {k, request.key.generation, before, numerical, item.stock.input.hands};
  grind_job_floor = grind_serial;
  grind_destroy_call = {};
  grind_destroy_binding = {};
  event("grind_item_path_verified", static_cast<int>(recipe.id),
        static_cast<int>(request.quantity));
  return true;
}
shdestroy::Snapshot grind_destroy_snapshot(bool initial) noexcept {
  shdestroy::Snapshot out;
  auto& stamp = out.stamp;
  const auto& binding = grind_destroy_binding;
  const auto before = context();
  const auto bridge = bc::diagnostics();
  const auto* recipe = cat::find(handoff.request().id);
  stamp.job = binding.job;
  stamp.frame = frame;
  stamp.serial = next_grind_serial();
  stamp.actual_thread = GetCurrentThreadId();
  if (!own_thread() || cool_destroy_depth != 1 || !grind_job || !recipe || !recipe->grind ||
      recipe->targets.count != 3 || binding.job != handoff.request().key.generation ||
      result_guard.state() != ubc::CraftingResultState::waiting || !before.identity_known ||
      !before.station_known || before.world != binding.key.world ||
      before.hero != binding.key.hero || before.station != binding.key.station || info_depth ||
      choice_depth || update_depth || station_depth || collection_depth || resolved_read_depth ||
      amount_patch.active() || bridge.writer || bridge.readers || bridge.writes_disabled)
    return out;
  stamp.key = grind_key(before);
  stamp.native_code_verified = cool_runtime_path().valid &&
                               shrt::verify_recipe(memory, s::game_parser, *recipe->grind).valid;
  stamp.hooks_owned = cool_destroy_installed && cool_remove_observer.owns_patch();
  const auto inventory = ubc::read_packed_inventory<3>(memory, before.hero, recipe->targets.data,
                                                       recipe->targets.count, true, cool_inventory);
  const bool inventory_ok = inventory.storage_totals_known && inventory.snapshot_unchanged &&
                            inventory.stable_boundary_verified &&
                            inventory.issue == ubc::PackedInventoryIssue::none;
  const bool hands_ok = ubc::station_recipe_snapshot_detail::begin_hands(
                            memory, before.hero, cool_hands) == ubc::StationHandReadIssue::none &&
                        ubc::station_recipe_snapshot_detail::verify_hands(memory, cool_hands);
  if (hands_ok)
    out.hands = cool_hands.before;
  out.stocks.stable_boundary_verified = inventory_ok;
  for (std::size_t i = 0; i < 3; ++i) {
    const bool unambiguous = inventory.listed_occurrences[i] <= 1 &&
                             inventory.packed_occurrences[i] <= 1 &&
                             !inventory.duplicate_packed_target[i] &&
                             !inventory.has_active_marker[i] && !inventory.mixed_representation[i];
    out.stocks.stocks[i] = {recipe->targets[i].symbol, inventory.total[i], inventory_ok,
                            unambiguous};
  }
  if (initial) {
    auto& evidence = grind_destroy_call;
    evidence.storage_alias_scan_known = inventory_ok && hands_ok;
    for (std::size_t i = 0; inventory_ok && i < inventory.listed_nodes; ++i) {
      const auto object = rt::detail::u32(cool_inventory.records[i].node.data() + 4);
      if (object == out.hands.hands.left.object || object == out.hands.interact_item)
        evidence.generated_storage_alias = true;
    }
    const auto* shape = ubc::grind_item_shape::find(shcat::lifecycle_item);
    const auto object = out.hands.hands.left.object;
    std::uint32_t type = 0, mainflag = 0, flags = 0;
    evidence.generated_left_verified =
        hands_ok && shdestroy::detail::active_grind_hand(out.hands) &&
        matches_station_profile(before.station, cat::Station::grind) && shape &&
        ubc::grind_item_shape::bound_to_profile(*shape) && field(object, 0, type) &&
        type == ubc::hand_detail::item_vtable && field(object, 0x154, mainflag) &&
        mainflag == shape->mainflag && shape->native_flags == bhshape::stored_flags &&
        field(object, 0x158, flags) &&
        bhshape::matches(out.hands.hands.left.symbol, type, mainflag, flags) &&
        integer(object, 0) == static_cast<std::int32_t>(type) &&
        integer(object, 0x154) == static_cast<std::int32_t>(mainflag) &&
        integer(object, 0x158) == static_cast<std::int32_t>(flags);
    // At the actual original debit, Cold16613 must be a single listed stack.
    // Packed or active forms do not qualify for the admitted ZBug branch.
    if (recipe->targets[2].symbol != shcat::lifecycle_item || inventory.packed_occurrences[2] ||
        inventory.packed[2] || inventory.listed_occurrences[2] != 1 ||
        inventory.has_active_marker[2] || inventory.mixed_representation[2])
      out.stocks.stable_boundary_verified = false;
  }
  const auto after = context();
  stamp.stable_boundary_verified = inventory_ok && hands_ok && after.identity_known &&
                                   after.station_known && after.world == before.world &&
                                   after.hero == before.hero && after.station == before.station &&
                                   cool_runtime_path().valid && frame == binding.frame;
  return out;
}
} // namespace ubc_native
