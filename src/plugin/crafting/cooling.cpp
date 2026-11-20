// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
int __fastcall observe_destroy(void*, void*, void*);
std::uint64_t __cdecl cool_remove_before(void*, const rcall::Invocation&);
void __cdecl cool_remove_after(void*, const rcall::Invocation&, std::uint64_t, std::uint32_t);
ubc::alchemy_flask_runtime::NativeCodePolicy cool_native_policy(bool predebit = false) noexcept {
  ubc::alchemy_flask_runtime::NativeCodePolicy policy;
  if (predebit) {
    const auto* plugins = end_compatibility(true);
    if (plugins)
      return {ubc::alchemy_flask_runtime::NativeCodeMode::cooling_predebit, plugins->zbug,
              plugins->union_core};
  }
  if (end_zbug_module.current()) {
    policy.mode = ubc::alchemy_flask_runtime::NativeCodeMode::known_zbug_remove_site;
    policy.zbug = {end_zbug_module.base(), end_zbug_module.image_size(), end_zbug_module.digest(),
                   true};
  }
  return policy;
}
cpath::OwnedPatches cool_owned_patches() noexcept {
  cpath::OwnedPatches owned;
  if (cool_destroy_installed) {
    owned.destroy_installed = true;
    owned.destination = reinterpret_cast<std::uint32_t>(&observe_destroy);
    owned.trampoline = reinterpret_cast<std::uint32_t>(cool_hooks.original(0));
  }
  if (cool_remove_observer.owns_patch()) {
    owned.remove_installed = true;
    owned.remove = cool_remove_observer.patch_plan();
  }
  return owned;
}
ddispatch::Context destroy_dispatch_context(const Context& c) noexcept {
  if (!c.identity_known || !c.station_known || !c.world || !c.hero || !c.station)
    return {};
  auto kind = c.kind;
  if (!c.valid) {
    // When the original interaction ends, the dialog selection may already
    // be closed. In that case, take the route only from our pending order
    // with unchanged physical identities. Never infer it from a default kind.
    const auto& k = handoff.request().key;
    if (result_guard.state() != ubc::CraftingResultState::waiting || k.generation != generation ||
        k.thread != GetCurrentThreadId() || k.world != c.world || k.hero != c.hero ||
        k.station != c.station || k.manager != s::information_manager)
      return {};
    kind = k.kind;
  }
  const auto production = kind == cat::Station::cool ? 13 : kind == cat::Station::grind ? 11 : 0;
  return {generation, GetCurrentThreadId(),   c.world,   c.hero,
          c.station,  s::information_manager, production};
}
DestroyReadPhase destroy_read_phase(const Context& c,
                                    const ddispatch::Invocation* invocation = nullptr) noexcept {
  const auto bound = destroy_dispatch_context(c);
  const auto bridge = bc::diagnostics();
  if (!own_thread() || !generation || !frame || !c.identity_known || !c.station_known ||
      !bound.world || !bound.hero || !bound.station ||
      (bound.production_kind != 11 && bound.production_kind != 13) || station_depth ||
      collection_depth || amount_patch.active() || bridge.readers || bridge.writes_disabled)
    return DestroyReadPhase::closed;
  // Only our own quiescent post-render callback. An invocation
  // pointer does not authorize running the event manager phase early.
  if (frame_depth == 1 && !cool_destroy_depth && !native_update && !update_depth && !info_depth &&
      !choice_depth && !resolved_read_depth && !bridge.writer && c.valid && !invocation)
    return DestroyReadPhase::preview;
  if (frame_depth != 0)
    return DestroyReadPhase::closed;
  const auto& request = handoff.request();
  const auto& k = request.key;
  const auto* recipe = cat::find(request.id);
  if (!recipe || (recipe->route != cat::Route::cool && recipe->route != cat::Route::grind) ||
      k.generation != generation || k.thread != GetCurrentThreadId() || k.world != bound.world ||
      k.hero != bound.hero || k.station != bound.station || k.manager != bound.manager ||
      k.kind != recipe->station ||
      (k.kind == cat::Station::cool    ? 13
       : k.kind == cat::Station::grind ? 11
                                       : 0) != bound.production_kind)
    return DestroyReadPhase::closed;
  // This scope covers only our actual handler and its inner before/after
  // reads. Do not read the message again after the original call.
  if (cool_destroy_depth == 1 && !native_update && !update_depth && !info_depth && !choice_depth &&
      !resolved_read_depth && !bridge.writer && effect_executed &&
      result_guard.state() == ubc::CraftingResultState::waiting &&
      ((recipe->cool && cool_job && !grind_job) || (recipe->grind && grind_job && !cool_job))) {
    if (invocation && (invocation->npc != k.hero || invocation->caller != cabi::destroy_return ||
                       invocation->depth != 1))
      return DestroyReadPhase::closed;
    return DestroyReadPhase::destroy;
  }
  if (cool_destroy_depth || invocation || !c.valid || !native_update || update_depth != 1 ||
      !handoff.active() || handoff.cancelled() ||
      (handoff.state() != ubc::CraftingHandoffState::dispatched &&
       handoff.state() != ubc::CraftingHandoffState::executing))
    return DestroyReadPhase::closed;
  const auto* ticket = handoff.current_ticket();
  if (!ticket || !ubc::crafting_handoff_detail::same_ticket(*ticket, dispatched_ticket) ||
      ticket->view != c.dialog.view || ticket->offer.collection != rows.generation ||
      !(key(c) == k))
    return DestroyReadPhase::closed;
  // After the original GetInformation call there is no Info()/writer lease
  // yet. The existing observe_resolve scope verifies the exact caller.
  if (resolved_read_depth == 1 && !info_depth && !choice_depth && !bridge.writer &&
      handoff.state() == ubc::CraftingHandoffState::dispatched &&
      ticket->offer.mode == basecat::Invocation::info)
    return DestroyReadPhase::resolver;
  // Read-only observation on entry, followed later by the same inner writer. The native
  // effect still requires late_gate and the applicable info lease or PostRemoveChoice.
  if (!resolved_read_depth &&
      ((info_depth == 1 && !choice_depth && ticket->offer.mode == basecat::Invocation::info) ||
       (choice_depth == 1 && !info_depth && ticket->offer.mode == basecat::Invocation::choice)))
    return DestroyReadPhase::callback;
  return DestroyReadPhase::closed;
}
cpath::Verification destroy_verified_path(const cpath::OwnedPatches& patches,
                                          const ddispatch::Invocation* invocation = nullptr,
                                          bool predebit = false) noexcept {
  if (!destroy_dispatch_reader)
    return {false, cpath::Issue::dispatch, cabi::destroy_entry};
  const auto before = context();
  const auto bound = destroy_dispatch_context(before);
  const auto phase = destroy_read_phase(before, invocation);
  const auto* current_modules = end_compatibility(predebit);
  const auto modules = current_modules ? *current_modules : ubc::zbug_end_compat::Binding{};
  auto proof = cpath::verify_dispatch(memory, patches, cool_native_policy(predebit),
                                      *destroy_dispatch_reader, bound, frame, GetCurrentThreadId(),
                                      phase != DestroyReadPhase::closed, invocation,
                                      current_modules ? &modules : nullptr);
  if (current_modules && !end_compatibility(predebit))
    return {false, cpath::Issue::ownership, ubc::cool_set_interact_path::body.address};
  const auto after = context();
  if (!ddispatch::detail::same(bound, destroy_dispatch_context(after)) ||
      phase != destroy_read_phase(after, invocation))
    return {false, cpath::Issue::changed, cabi::destroy_entry};
  return proof;
}
cpath::Verification cool_runtime_path(bool predebit) noexcept {
  if (!cool_ready || !cool_destroy_installed || !cool_remove_observer.owns_patch())
    return {false, cpath::Issue::ownership, cabi::destroy_entry};
  return destroy_verified_path(cool_owned_patches(), nullptr, predebit);
}
bool __cdecl cool_predebit_boundary(void*, cpre::reader::Phase, cpre::reader::Input&) noexcept;
bool cool_engine_current() noexcept {
#ifdef UBC_PLAYER_RELEASE
  return player_image.failure[static_cast<unsigned>(cat::Station::cool)] == 0 &&
         w::player_image_header();
#else
  return cool_predebit_engine.current();
#endif
}
bool bind_cool_engine() noexcept {
#ifdef UBC_PLAYER_RELEASE
  return cool_engine_current();
#else
  return cool_predebit_engine.acquire(L"Gothic2.exe", 0x752000, s::executable_sha256);
#endif
}
void initialize_cool(const Context& c) noexcept {
  if (!generation || cool_install_attempted ||
      (c.kind == cat::Station::grind && grind_probe_failed) || !c.valid ||
      (c.kind != cat::Station::cool && c.kind != cat::Station::grind) || !own_thread() ||
      frame_depth != 1 || update_depth || info_depth || choice_depth || station_depth ||
      collection_depth || cool_destroy_depth || handoff.active() || amount_patch.active() ||
      !complete_collection(c) || !c.dialog.waiting || !c.dialog.open)
    return;
  const bool cooling = c.kind == cat::Station::cool;
  destroy_dispatch_reader.emplace(GetCurrentThreadId());
  // Both stations use the passive observers. Before execution,
  // each order rechecks the forwarding paths that apply to it.
  const auto native = destroy_verified_path({}, nullptr, cooling);
  if (!native.valid || !end_path(c.station, cooling).valid) {
    if (cooling)
      cool_install_attempted = true;
    else
      grind_probe_failed = true;
    event("cool_native_rejected", static_cast<int>(native.issue), static_cast<int>(native.address));
    return;
  }
  cool_install_attempted = true;
  const std::array<std::uintptr_t, 1> targets{{reinterpret_cast<std::uintptr_t>(&observe_destroy)}};
  if (!cool_hooks.prepare(targets) ||
      !cool_remove_observer.prepare(GetCurrentThreadId(),
                                    {nullptr, cool_remove_before, cool_remove_after},
                                    cool_native_policy(cooling))) {
    event("cool_hooks_rejected", static_cast<int>(cool_remove_observer.issue()));
    return;
  }
  original_destroy = reinterpret_cast<Destroy>(cool_hooks.original(0));
  cool_destroy_installed = cool_hooks.install(reinterpret_cast<std::uintptr_t>(&observe_destroy));
  if (!cool_destroy_installed || !cool_remove_observer.install()) {
    event("cool_hooks_rejected", static_cast<int>(cool_remove_observer.issue()));
    return;
  }
  cool_ready = true;
  const auto installed_path = cool_runtime_path(cooling);
  cool_ready = installed_path.valid;
  if (cool_ready && initialize_item_modules(true) && bind_cool_engine()) {
    cool_predebit_reader.emplace(GetCurrentThreadId());
    const cwriter::Config config{
        GetCurrentThreadId(),
        memory,
        nullptr,
        cool_predebit_boundary,
        reinterpret_cast<cwriter::Remove>(xdep::expected_targets.remove),
        reinterpret_cast<cwriter::RestorePop>(xdep::expected_targets.restore_pop),
        reinterpret_cast<cwriter::RestoreInstance>(xdep::expected_targets.restore_instance),
        reinterpret_cast<cwriter::RestoreItem>(0x7a2f20)};
    cool_predebit_executor.emplace(cool_predebit_tracker, config);
  }
  event(cool_ready ? "cool_hooks_installed" : "cool_native_rejected",
        static_cast<int>(installed_path.issue), static_cast<int>(installed_path.address));
}
ubc::CoolRecipeKey cool_key(const Context& c) noexcept {
  return {generation, GetCurrentThreadId(), c.world, c.hero, c.station, s::information_manager, 13};
}
// For now the direct native path is limited to the single verified
// list stack. No quantity query materializes a packed entry.
bool cool_listed_input(const ubc::CoolRecipeCatalogueCapture& source) noexcept {
  return ubc::verify_cool_listed_input(memory, source, cool_inventory, cool_hands);
}
ubc::CoolRecipeCatalogueCapture capture_cool(const Context& c, bool boundary) noexcept {
  ubc::CoolCaptureProofs proof;
  proof.stable_boundary_verified = c.valid && boundary;
  const auto native = cool_runtime_path(true);
  if (proof.stable_boundary_verified && native.valid && end_path(c.station, true).valid &&
      ubc::station_recipe_snapshot_detail::begin_hands(memory, c.hero, cool_hands) ==
          ubc::StationHandReadIssue::none &&
      ubc::station_recipe_snapshot_detail::verify_hands(memory, cool_hands)) {
    proof.verified_left_tmp_level = cool_hands.before.left_tmp_level;
    proof.station_hand_end_verified = matches_station_profile(c.station, cat::Station::cool);
  }
  auto source = cool_catalogue_reader.capture(memory, s::game_parser, cool_key(c), proof);
  const bool two = source.recipe.material.known() && source.recipe.material.rows[0].stored == 2;
  if (!cool_listed_input(source) || !same_context(c, context()) || !cool_runtime_path(two).valid ||
      !end_path(c.station, two).valid)
    source.recipe.contract = false;
  return source;
}
bool verify_cool_amount(const bc::WriterScope& scope, const cat::Descriptor& recipe) noexcept {
  const auto& request = handoff.request();
  const auto c = context();
  if (!recipe.cool || !c.valid || c.kind != cat::Station::cool || cool_destroy_depth ||
      scope.call.thread != GetCurrentThreadId() || scope.call.caller != bc::info_call_return ||
      !native_update || update_depth != 1 || info_depth != 1 || choice_depth || station_depth ||
      collection_depth || !cool_runtime_path().valid)
    return false;
  const auto actual = current_info_effect(c);
  if (!actual)
    return false;
  const auto current_page = cool_page(c);
  const auto before = capture_cool(c, true);
  const auto prepared = cdirect::prepare(before, current_page, request.quantity);
  if (!prepared.valid ||
      prepared.path.amount.callback != static_cast<std::uint32_t>(scope.call.symbol))
    return false;
  const auto late = capture_cool(c, true);
  const auto bound = cdirect::bind(memory, prepared, late, current_page, actual);
  if (!bound.valid || !cdirect::detail::same_key(bound.prepared.key, cool_key(c)) ||
      !ubc::same_crafting_hands(bound.prepared.hands, handed_hands) ||
      !same_context(c, context()) || !late_gate())
    return false;
  cool_active_entry = bound;
  cool_job_floor = cool_serial;
  cool_destroy_call = {};
  cool_destroy_binding = {};
  cool_destroy_hand_diagnostic = {};
  cool_receipt_acked = !bound.prepared.requirements().quantity_receipt;
  event("cool_direct_path_verified", static_cast<int>(recipe.id),
        static_cast<int>(request.quantity));
  return true;
}
std::uint64_t next_cool_serial() noexcept;
struct CoolProbePhase {
  const cadmission::Prepared& prepared;
  const ubc::AmountPatch& patch;
  const cpre::Tracker& tracker;
  bool waiting;
};
bool inspect_cool_boundary(cpre::reader::Phase phase, cpre::reader::Input& input,
                           cpre::check::Report* report,
                           const CoolProbePhase* model = nullptr) noexcept {
  using Phase = cpre::reader::Phase;
  using Gate = cpre::Gate;
  cpre::check::Checks k(report);
  auto& rejection = amount_diagnostic.predebit;
  const auto group = [&](Gate gate) { k.reason = static_cast<int>(gate); };
  const auto* scope = cool_writer_scope;
  const auto bridge = bc::diagnostics();
  group(Gate::writer_scope);
  k.flag("writer.game_thread", own_thread());
  k.flag("writer.original", scope && scope->original);
  k.equal("writer.thread", GetCurrentThreadId(), scope ? scope->call.thread : 0);
  k.equal("writer.parser", s::game_parser,
          scope ? reinterpret_cast<std::uint32_t>(scope->call.parser) : 0);
  k.equal("writer.caller", bc::info_call_return, scope ? scope->call.caller : 0);
  k.equal("writer.callback", cpre::callback, scope ? scope->call.symbol : 0);
  k.test("writer.token", ">0", scope ? scope->token : 0, scope && scope->token);
  k.flag("writer.active", bridge.writer);
  k.equal("writer.readers", 0, bridge.readers);
  k.equal("writer.disabled", 0, bridge.writes_disabled);
  k.flag("writer.executor", cool_predebit_executor.has_value());
  k.flag("writer.dispatch_reader", destroy_dispatch_reader.has_value());
  k.flag("writer.cool_ready", cool_ready);
  k.equal("writer.destroy_depth", 0, cool_destroy_depth);
  k.equal("writer.frame_depth", 0, frame_depth);
  if (cpre::reader::split_pending(phase))
    k.flag("writer.focus", ui.focused());
  k.flag("writer.cool_job", cool_job);
  k.equal("writer.grind_job", 0, grind_job);
  k.test("writer.fence", ">0", cool_fence_token, cool_fence_token != 0);
  const auto c = context();
  const auto actual = current_info_effect(c);
  const auto& request = handoff.request();
  const auto* ticket = handoff.current_ticket();
  const auto* step = handoff.step();
  group(Gate::handoff);
  k.test("ticket.actual_info", ">0", actual, actual != 0);
  k.flag("ticket.step", step != nullptr);
  k.equal("ticket.owner", cpre::owner, ticket ? ticket->offer.owner : 0);
  k.equal("ticket.callback", cpre::callback, ticket ? ticket->offer.function : 0);
  k.equal("ticket.station", static_cast<int>(cat::Station::cool),
          static_cast<int>(request.key.kind));
  k.equal("ticket.quantity", 2, request.quantity);
  k.equal("ticket.recipe", static_cast<int>(ccat::RecipeId::blade), static_cast<int>(request.id));
  const auto end = end_path(c.station, true);
  k.test("ticket.end_path", "gueltiger Endpfad", static_cast<int>(end.issue), end.valid);
  k.flag("ticket.station_profile", matches_station_profile(c.station, cat::Station::cool));
  const auto d =
      cool_predebit_executor ? cool_predebit_executor->diagnostics() : cwriter::Diagnostics{};
  const bool admission = phase == Phase::admission, after = phase == Phase::after_original;
  const auto& patch = model ? model->patch : amount_patch;
  const bool native = model ? (phase == Phase::before_remove || phase == Phase::restore ||
                               phase == Phase::after_extra)
                            : d.native_scope;
  group(Gate::executor_phase);
  k.flag("executor.configured", d.configured);
  k.equal("executor.disabled", 0, d.disabled);
  k.equal("executor.active", !admission, model ? true : d.active);
  k.flag("executor.guard", model       ? model->waiting
                           : admission ? result_guard.permits_next()
                                       : result_guard.state() == ubc::CraftingResultState::waiting);
  k.equal("executor.amount_active", !admission && !after, patch.active());
  if (!admission) {
    group(Gate::prepared);
    const auto& prepared = model ? model->prepared : result_guard.cool_prepared();
    const auto& job = prepared.job();
    k.flag("prepared.lease", prepared.current(info_lease_owner));
    k.equal("prepared.writer", scope ? scope->token : 0, job.writer_token);
    k.equal("prepared.fence", cool_fence_token, job.fence_token);
    k.equal("prepared.generation", request.key.generation, job.id);
    k.equal("prepared.info", actual, job.actual_info);
    k.equal("prepared.frame", frame, prepared.frame());
    k.flag("prepared.context", cpre::value::same(job.context, extra_context(c)));
    group(Gate::operand);
    if (!after)
      k.flag("prepared.operand", patch.matches_active(prepared.body_address(),
                                                      prepared.amount().body(), cpre::quantity));
  }
  if (!after) {
    group(Gate::context);
    k.flag("call.context", c.valid);
    k.flag("call.key", key(c) == request.key);
    k.equal("call.previous_effect", 0, effect_executed);
    if (phase == Phase::before_remove || phase == Phase::before_original || admission) {
      group(Gate::page);
      const auto page = cool_info_page(c, true);
      k.flag("page.valid", page.valid);
      k.flag("page.single", page.single);
      k.flag("page.ticket", step && ubc::crafting_page(page) == step->before);
      const auto row = ticket ? ticket->offer.row : -1;
      k.test("page.row", "in INFO-Liste", row,
             row >= 0 && static_cast<std::size_t>(row) < rows.count);
      k.equal("page.info", actual,
              row >= 0 && static_cast<std::size_t>(row) < rows.count
                  ? rows.rows[static_cast<std::size_t>(row)].address
                  : 0);
      group(Gate::actor);
      const auto actor = crt::actor(memory, s::game_parser, crt::assemble(ccat::recipe), c.hero);
      k.flag("actor.read", actor.read_valid);
      k.flag("actor.self", actor.requires_self);
      k.flag("actor.hero", actor.matches);
      group(Gate::dispatch);
      cpath::Verification path{};
      if (admission)
        path = cool_runtime_path(true);
      else {
        const bool before = phase == Phase::before_remove;
        const auto stage =
            model ? model->tracker.status().stage : cool_predebit_tracker.status().stage;
        const bool quiet = cool_predebit_journal.sample(cool_fence_token, input.destroy_started,
                                                        input.destroy_completed);
        const bool pending =
            stage == (before ? cpre::Stage::calling_extra : cpre::Stage::extra_verified) &&
            native == before &&
            (model || (cool_predebit_executor && !cool_predebit_executor->original_entered())) &&
            quiet;
        k.flag("dispatch.pending", pending);
        const auto* modules = end_compatibility(true);
        if (modules && destroy_dispatch_reader)
          path = cpath::verify_dispatch(memory, cool_owned_patches(), cool_native_policy(true),
                                        *destroy_dispatch_reader, destroy_dispatch_context(c),
                                        frame, GetCurrentThreadId(), model ? quiet : pending,
                                        nullptr, modules);
      }
      k.test("dispatch.issue", "gueltiger Destroy-Pfad", static_cast<int>(path.issue), path.valid);
      k.test("dispatch.address", "gepruefte Aufrufstelle", path.address, path.valid);
    }
  }
  const auto now = context();
  group(Gate::context);
  k.flag("call.closing_context", after ? (now.identity_known && now.station_known &&
                                          cpre::value::same(extra_context(c), extra_context(now)))
                                       : same_context(c, now));
  k.equal("call.closing_info", actual, current_info_effect(now));
  group(Gate::modules);
  k.flag("module.engine", cool_engine_current());
  k.flag("module.plugins", end_compatibility(true) != nullptr);
  k.flag("module.zpe", extra_zpe_module.current());
  k.flag("module.crt", extra_crt_module.current());
  group(Gate::hooks);
  k.flag("hooks.writer", scope && bc::writer_undisturbed(*scope));
  k.flag("hooks.installed", cpath::installed_hooks_current(memory, cool_owned_patches()));
  group(Gate::journal);
  k.flag("journal.quiet", cool_predebit_journal.sample(cool_fence_token, input.destroy_started,
                                                       input.destroy_completed));
  const auto& bound = info_effect_lease;
  const cpre::value::Context expected{bound.key.generation, bound.thread,    s::game_parser,
                                      bound.game,           bound.key.world, bound.key.hero,
                                      bound.key.station};
  // The diagnostic receiver may read only the bound ticket, even if the actual values differ.
  input.job = {report ? expected : extra_context(now),
               request.key.generation,
               scope ? scope->token : 0,
               cool_fence_token,
               bound.token,
               report ? bound.actual_info : actual,
               s::information_manager};
  input.scope = {
      {input.job.id, input.job.writer_token, cpre::callback, 1, 0, native ? 1u : 0u, true},
      bound.token,
      input.job.actual_info,
      true};
  input.phase = phase;
  input.serial = next_cool_serial();
  input.frame = frame;
  input.current_thread = GetCurrentThreadId();
#ifdef UBC_PLAYER_RELEASE
  input.engine_sites_verified = cool_engine_current();
  input.engine = {s::image_base, s::image_size, {}, input.engine_sites_verified};
#else
  input.engine = {cool_predebit_engine.base(), cool_predebit_engine.image_size(),
                  cool_predebit_engine.digest(), true};
#endif
  input.modules = extra_modules(true);
  input.amount_patch = &patch;
  input.encoded_null_handler = reinterpret_cast<std::uint32_t>(EncodePointer(nullptr));
  const auto* tib = reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
  input.stack = {true, input.current_thread, reinterpret_cast<std::uint32_t>(tib->StackLimit),
                 reinterpret_cast<std::uint32_t>(tib->StackBase)};
  group(Gate::serial);
  k.test("call.serial", ">0", input.serial, input.serial != 0);
  if (!k.ok() && !report && !rejection.event) {
    rejection.failure = k.first;
    rejection.gate(phase, static_cast<Gate>(k.first.reason));
  }
  return k.ok();
}
bool __cdecl cool_predebit_boundary(void*, cpre::reader::Phase phase,
                                    cpre::reader::Input& input) noexcept {
  return inspect_cool_boundary(phase, input, nullptr);
}
cadmission::Prepared prepare_cool_predebit(const bc::WriterScope& scope) noexcept {
  constexpr auto phase = cpre::reader::Phase::admission;
  auto& rejection = amount_diagnostic.predebit;
  cpre::reader::Input input;
  if (!cool_predebit_reader) {
    rejection.gate(phase, cpre::Gate::reader_missing);
    return {};
  }
  if (!late_gate()) {
    rejection.gate(phase, cpre::Gate::late_gate);
    return {};
  }
  if (!cool_predebit_boundary(nullptr, phase, input))
    return {};
  const cadmission::WriterEntry writer{scope.token,
                                       reinterpret_cast<std::uint32_t>(scope.call.parser),
                                       static_cast<std::uint32_t>(scope.call.symbol),
                                       static_cast<std::uint32_t>(scope.call.caller),
                                       scope.call.thread,
                                       true,
                                       true};
  const auto prepared = cadmission::prepare(*cool_predebit_reader, memory, input, info_lease_owner,
                                            writer, &rejection);
  if (!prepared.valid())
    return {};
  cpre::reader::Input closing;
  if (!prepared.current(info_lease_owner) || !late_gate() ||
      !cool_predebit_boundary(nullptr, phase, closing) ||
      !cpre::same_job(prepared.job(), closing.job) || prepared.frame() != closing.frame) {
    rejection.gate(phase, cpre::Gate::closing);
    return {};
  }
  return prepared;
}
// At the same writer boundary as prepare_cool_predebit. Subsequent phases use actual
// read channels and private copies of the operand, quantity, tracker and result guard.
void inspect_cool_transaction(const bc::WriterScope& scope,
                              const ubc::AmountMemory& live) noexcept {
  using Phase = cpre::reader::Phase;
  auto& entry = cool_check_reports.at(Phase::admission);
  cpre::check::Checks e(&entry);
  cpre::reader::Input input;
  e.flag("call.late_gate", late_gate());
  inspect_cool_boundary(Phase::admission, input, &entry);
  e.flag("call.reader", cool_predebit_reader.has_value());
  if (!cool_predebit_reader)
    return;
  const cadmission::WriterEntry writer{scope.token,
                                       reinterpret_cast<std::uint32_t>(scope.call.parser),
                                       static_cast<std::uint32_t>(scope.call.symbol),
                                       static_cast<std::uint32_t>(scope.call.caller),
                                       scope.call.thread,
                                       true,
                                       scope.original != nullptr};
  cpre::reader::Capture captured;
  const auto prepared = cadmission::prepare(*cool_predebit_reader, memory, input, info_lease_owner,
                                            writer, nullptr, &entry, &captured);
  const auto& baseline = captured.observation;
  const auto& f = *ccat::single.information;
  const auto address = baseline.parser.code_base + f.byte_offset;
  e.flag("call.prepared", prepared.current(info_lease_owner));
  e.flag("call.closing_gate", late_gate());
  cpre::reader::Input closing;
  inspect_cool_boundary(Phase::admission, closing, &entry);
  e.flag("call.closing_job", cpre::same_job(input.job, closing.job));
  e.equal("call.closing_frame", input.frame, closing.frame);
  e.flag("amount.identity", live.identity && live.identity(live.context, address, f.byte_length));
  e.flag("amount.writable", live.writable && live.writable(live.context, address + 62, 4));
  const std::uint32_t one = 1, two = 2;
  e.flag("amount.forward_owner",
         cwriter::permits_operand_replace(prepared, info_lease_owner, &scope, cool_predebit_journal,
                                          cool_fence_token, address + 62, &one, &two, 4,
                                          []() { return current_info_effect(context()); }));
  const auto& request = handoff.request();
  const auto* ticket = handoff.current_ticket();
  const ubc::CraftingResultBinding binding{request.key.generation,
                                           frame,
                                           ticket ? ticket->offer.collection : 0,
                                           end_sequence,
                                           request.key.world,
                                           request.key.hero,
                                           request.key.station};
  auto guard = result_guard;
  cpre::check::Checks result_checks(&entry);
  const bool waiting =
      guard.begin_cool_predebit(request, binding, prepared, info_lease_owner, &result_checks);
  e.flag("call.result_guard", waiting);
  cpre::inspection::SplitImage image(live, address, baseline.stack.object);
  ubc::AmountPatch patch;
  const std::array<ubc::AmountOperand, 1> operands{{{62, 1, 1, 0, false}}};
  const ubc::AmountBody body{f.byte_length, f.body_sha256, operands.data(), operands.size()};
  const bool patched = patch.begin(image.amount(), address, body, cpre::quantity);
  e.test("amount.begin", "0", static_cast<int>(patch.issue()), patched);
  cpre::Tracker tracker;
  cpre::Call call;
  CoolProbePhase model{prepared, patch, tracker, waiting};
  for (const auto phase : cpre::inspection::phases) {
    if (phase == Phase::admission)
      continue;
    if (phase == Phase::restore)
      image.after_split();
    auto& rows = cool_check_reports.at(phase);
    cpre::check::Checks k(&rows);
    cpre::reader::Input q;
    inspect_cool_boundary(phase, q, &rows, &model);
    if (cpre::reader::reads_saved_call(phase))
      q.call = &call;
    auto reading = cool_predebit_reader->capture(image.memory(), q, &rows);
    auto& v = reading.observation;
    const auto status = [&](const char* field, bool ok) {
      k.test(field, "0", static_cast<int>(tracker.status().issue), ok);
    };
    if (phase == Phase::before_extra) {
      cpre::check::Checks comparison(&rows);
      k.flag("call.prepared_matches", prepared.matches(v, &comparison));
      status("call.admit", tracker.admit(q.job, v));
      cpre::reader::Input late;
      inspect_cool_boundary(phase, late, &rows, &model);
      const auto last = cool_predebit_reader->capture(image.memory(), late, &rows);
      call = tracker.enter_extra(last.observation);
      status("call.enter_extra", call.valid);
      if (!call.valid) { // Measurement basis for independent restore fields only. Never execute.
        const auto& x = last.observation;
        call = {false,
                q.job,
                x.serial,
                16612,
                1,
                x.stack.object,
                x.hand.left,
                x.parser,
                x.item,
                x.fence,
                x.inventory_identity,
                x.hand_identity,
                x.frame};
      }
    } else if (phase == Phase::before_remove || phase == Phase::restore) {
      const auto plan = cpre::restore_plan(call, reading.restore);
      k.equal("call.pop_entry", 0x7a4b20, plan.pop_entry);
      k.equal("call.instance_entry", 0x7a1550, plan.instance_entry);
      k.equal("call.item_entry", 0x7a2f20, plan.item_entry);
      k.flag("call.parser_unchanged", cpre::value::same(call.parser, reading.restore.current));
      k.flag("call.item_unchanged",
             cpre::old::same_item(call.item, reading.restore.item, call.parser));
      if (phase == Phase::before_remove) {
        k.flag("call.stock_before", cpre::rows(v.stocks, baseline.stocks.stocks));
        k.flag("call.split_stack", cpre::old::listed_stack(v.stack, 2));
      }
    } else if (phase == Phase::after_extra)
      status("call.extra_returned", tracker.extra_returned(v, true, true));
    else
      status("call.enter_original", tracker.enter_original(v));
  }
  e.flag("amount.private_restore", !patch.active() || patch.finish());
}
void* execute_cool_predebit(bc::WriterScope& scope, const ubc::AmountMemory& writable) {
  DWORD error = GetLastError();
  void* result = nullptr;
  bool ran = false;
  alignas(16) unsigned char fx[512];
  auto& rejection = amount_diagnostic.predebit;
  if (cool_writer_scope) {
    rejection.gate(cpre::reader::Phase::admission, cpre::Gate::writer_scope);
    return nullptr;
  }
  if (!(cool_fence_token = cool_predebit_journal.open())) {
    rejection.gate(cpre::reader::Phase::admission, cpre::Gate::journal);
    return nullptr;
  }
  cool_writer_scope = &scope;
  __try {
    if (cool_context_mode) {
      inspect_cool_transaction(scope, writable);
      amount_diagnostic.rejected = 8;
      __leave;
    }
    const auto prepared = prepare_cool_predebit(scope);
    if (!prepared.valid()) {
      amount_diagnostic.rejected = 8;
      __leave;
    }
    const auto& request = handoff.request();
    const auto* ticket = handoff.current_ticket();
    const ubc::CraftingResultBinding binding{request.key.generation,
                                             frame,
                                             ticket ? ticket->offer.collection : 0,
                                             end_sequence,
                                             request.key.world,
                                             request.key.hero,
                                             request.key.station};
    cool_active_entry = {};
    cool_destroy_call = {};
    cool_destroy_binding = {};
    cool_destroy_hand_diagnostic = {};
    cool_job_floor = prepared.serial();
    cool_receipt_acked = false;
    const cwriter::Request run{prepared.job(),    frame,     &amount_patch, &prepared,
                               &info_lease_owner, &rejection};
    SetLastError(error);
    ran = true;
    result = cool_predebit_executor->run(scope, amount_patch, writable, run, result_guard, request,
                                         binding);
    error = GetLastError();
  } __finally {
    _fxsave(fx);
    if (AbnormalTermination())
      error = GetLastError();
    effect_executed = ran && cool_predebit_executor->extra_entered();
    amount_diagnostic.entered = ran && cool_predebit_executor->original_entered();
    amount_diagnostic.restored = !ran || cool_predebit_executor->operands_restored();
    if (result_guard.cool_witness().valid &&
        result_guard.cool_witness().job.fence_token == cool_fence_token)
      cool_job_floor = cool_serial;
    if (!cool_predebit_journal.close(cool_fence_token) || !bc::writer_undisturbed(scope)) {
      rejection.gate(rejection.phase, cpre::Gate::closing);
      result_guard.fail(ubc::CraftingResultIssue::destroy);
      if (effect_executed)
        scope.restore_verified = false;
      if (cool_predebit_executor)
        cool_predebit_executor->disable();
    }
    cool_fence_token = 0;
    cool_writer_scope = nullptr;
    _fxrstor(fx);
    SetLastError(error);
  }
  return result;
}
std::uint64_t next_cool_serial() noexcept {
  if (cool_serial == UINT64_MAX) {
    disabled = true;
    return 0;
  }
  return ++cool_serial;
}
cdestroy::Snapshot cool_destroy_snapshot(bool initial) noexcept {
  cdestroy::Snapshot out;
  auto& stamp = out.stamp;
  const auto& binding = cool_destroy_binding;
  const auto before = context();
  const auto bridge = bc::diagnostics();
  stamp.job = binding.job;
  stamp.frame = frame;
  stamp.serial = next_cool_serial();
  stamp.actual_thread = GetCurrentThreadId();
  if (!own_thread() || cool_destroy_depth != 1 || !cool_job ||
      (!cool_active_entry.valid && !result_guard.cool_witness().valid) ||
      binding.job != handoff.request().key.generation ||
      result_guard.state() != ubc::CraftingResultState::waiting || !before.identity_known ||
      !before.station_known || before.world != binding.key.world ||
      before.hero != binding.key.hero || before.station != binding.key.station || info_depth ||
      choice_depth || update_depth || station_depth || collection_depth || resolved_read_depth ||
      amount_patch.active() || bridge.writer || bridge.readers || bridge.writes_disabled)
    return out;
  stamp.key = {generation,  GetCurrentThreadId(), before.world,
               before.hero, before.station,       s::information_manager,
               13};
  const bool predebit = result_guard.cool_prepared().valid();
  const auto path = cool_runtime_path(predebit);
  stamp.native_code_verified = path.valid;
  stamp.hooks_owned = cool_destroy_installed && cool_remove_observer.owns_patch();
  const auto inventory =
      ubc::read_packed_inventory(memory, before.hero, ccat::targets, true, cool_inventory);
  const bool inventory_ok = inventory.storage_totals_known && inventory.snapshot_unchanged &&
                            inventory.stable_boundary_verified &&
                            inventory.issue == ubc::PackedInventoryIssue::none;
  const bool hands_ok = ubc::station_recipe_snapshot_detail::begin_hands(
                            memory, before.hero, cool_hands) == ubc::StationHandReadIssue::none &&
                        ubc::station_recipe_snapshot_detail::verify_hands(memory, cool_hands);
  if (hands_ok)
    out.hands = cool_hands.before;
  out.stocks.stable_boundary_verified = inventory_ok;
  for (std::size_t i = 0; i < 2; ++i) {
    const bool unambiguous = inventory.listed_occurrences[i] <= 1 &&
                             inventory.packed_occurrences[i] <= 1 &&
                             !inventory.duplicate_packed_target[i] &&
                             !inventory.has_active_marker[i] && !inventory.mixed_representation[i];
    out.stocks.stocks[i] = {ccat::targets[i].symbol, inventory.total[i], inventory_ok, unambiguous};
  }
  if (initial) {
    auto& evidence = cool_destroy_call;
    evidence.storage_alias_scan_known = inventory_ok && hands_ok;
    evidence.generated_storage_alias = false;
    for (std::size_t i = 0; inventory_ok && i < inventory.listed_nodes; ++i) {
      const auto object = rt::detail::u32(cool_inventory.records[i].node.data() + 4);
      if (object == out.hands.hands.left.object || object == out.hands.interact_item)
        evidence.generated_storage_alias = true;
    }
    const auto object = out.hands.hands.left.object;
    std::uint32_t type = 0, mainflag = 0, flags = 0;
    auto& diagnostic = cool_destroy_hand_diagnostic;
    diagnostic.prepared = result_guard.cool_prepared().valid()
                              ? result_guard.cool_prepared().hands()
                              : cool_active_entry.prepared.hands;
    diagnostic.observed = out.hands;
    diagnostic.hands_read_verified = hands_ok;
    const auto profile = [&]() noexcept {
      diagnostic.profile_checked = true;
      return diagnostic.profile_matches =
                 matches_station_profile(before.station, cat::Station::cool);
    };
    const auto first_shape = [&](unsigned index, std::uint32_t offset,
                                 std::uint32_t& value) noexcept {
      diagnostic.shape_attempted |= 1u << index;
      const bool known = field(object, offset, value);
      diagnostic.shape[index] = chdiag::signed_word(value);
      if (known)
        diagnostic.shape_known |= 1u << index;
      return known;
    };
    const auto second_shape = [&](unsigned index, std::uint32_t offset) noexcept {
      // Match integer() semantics exactly. Initialize to -1, call field() once and compare the
      // return value unchanged. Whether the read succeeded is diagnostic only and adds no new gate.
      std::int32_t value = -1;
      diagnostic.shape_attempted |= 1u << index;
      const bool known = field(object, offset, value);
      diagnostic.shape[index] = value;
      if (known)
        diagnostic.shape_known |= 1u << index;
      return value;
    };
    evidence.generated_left_verified =
        hands_ok && cdirect::detail::active_hand(out.hands) && profile() &&
        first_shape(0, 0, type) && type == ubc::hand_detail::item_vtable &&
        first_shape(1, 0x154, mainflag) && mainflag == 1 && first_shape(2, 0x158, flags) &&
        bhshape::matches(out.hands.hands.left.symbol, type, mainflag, flags) &&
        second_shape(3, 0) == static_cast<std::int32_t>(type) && second_shape(4, 0x154) == 1 &&
        second_shape(5, 0x158) == static_cast<std::int32_t>(bhshape::flags);
    diagnostic.generated_left_verified = evidence.generated_left_verified;
    // The known ZBug packing branch is never actually reached.
    if (inventory.packed_occurrences[0] || inventory.packed[0] ||
        inventory.listed_occurrences[0] != 1)
      out.stocks.stable_boundary_verified = false;
  }
  const auto after = context();
  stamp.stable_boundary_verified = inventory_ok && hands_ok && after.identity_known &&
                                   after.station_known && after.world == before.world &&
                                   after.hero == before.hero && after.station == before.station &&
                                   cool_runtime_path(predebit).valid && frame == binding.frame;
  if (initial) {
    cool_destroy_hand_diagnostic.stamp = stamp;
    cool_destroy_hand_diagnostic.captured = true;
  }
  return out;
}
std::uint64_t __cdecl cool_remove_before(void*, const rcall::Invocation& input) {
  if (grind_job) {
    if (!own_thread() || cool_destroy_depth != 1 || !grind_destroy_call.destroy_entries ||
        grind_destroy_call.destroy_exits ||
        result_guard.state() != ubc::CraftingResultState::waiting)
      return 0;
    auto& o = grind_destroy_call;
    ++o.remove_entries;
    if (o.remove_entries != 1) {
      o.reentry_seen = true;
      return 0;
    }
    o.remove = {input.caller,
                input.inventory,
                input.loaded_vtable,
                input.original,
                input.instance,
                input.amount,
                1,
                1,
                0,
                false,
                false};
    o.before_remove = grind_destroy_snapshot(false);
    o.set_interact_item_normal_return_verified =
        o.before_remove.stamp.native_code_verified && o.before_remove.stamp.hooks_owned &&
        o.destroy.caller == cabi::destroy_return && input.caller == cabi::inventory_remove_return &&
        input.inventory == o.destroy.npc + cabi::npc_inventory_offset;
    return o.before_remove.stamp.serial;
  }
  if (!own_thread() || cool_destroy_depth != 1 || !cool_job || !cool_destroy_call.destroy_entries ||
      cool_destroy_call.destroy_exits || result_guard.state() != ubc::CraftingResultState::waiting)
    return 0;
  auto& o = cool_destroy_call;
  ++o.remove_entries;
  if (o.remove_entries != 1) {
    o.reentry_seen = true;
    return 0;
  }
  o.remove = {input.caller,
              input.inventory,
              input.loaded_vtable,
              input.original,
              input.instance,
              input.amount,
              1,
              1,
              0,
              false,
              false};
  o.before_remove = cool_destroy_snapshot(false);
  o.set_interact_item_normal_return_verified =
      o.before_remove.stamp.native_code_verified && o.before_remove.stamp.hooks_owned &&
      o.destroy.caller == cabi::destroy_return && input.caller == cabi::inventory_remove_return &&
      input.inventory == o.destroy.npc + cabi::npc_inventory_offset;
  return o.before_remove.stamp.serial;
}
void __cdecl cool_remove_after(void*, const rcall::Invocation& input, std::uint64_t ticket,
                               std::uint32_t returned) {
  if (grind_job) {
    auto& o = grind_destroy_call;
    if (!own_thread() || cool_destroy_depth != 1 || !ticket ||
        ticket != o.before_remove.stamp.serial || o.remove.caller != input.caller ||
        o.remove.inventory != input.inventory || o.remove.target != input.original ||
        o.remove.loaded_vtable != input.loaded_vtable || o.remove.instance != input.instance ||
        o.remove.amount != input.amount) {
      o.reentry_seen = true;
      return;
    }
    ++o.remove_exits;
    o.remove.normal_return = true;
    o.remove.opaque_return = returned;
    o.after_remove = grind_destroy_snapshot(false);
    return;
  }
  auto& o = cool_destroy_call;
  if (!own_thread() || cool_destroy_depth != 1 || !ticket ||
      ticket != o.before_remove.stamp.serial || o.remove.caller != input.caller ||
      o.remove.inventory != input.inventory || o.remove.target != input.original ||
      o.remove.loaded_vtable != input.loaded_vtable || o.remove.instance != input.instance ||
      o.remove.amount != input.amount) {
    o.reentry_seen = true;
    return;
  }
  ++o.remove_exits;
  o.remove.normal_return = true;
  o.remove.opaque_return = returned;
  o.after_remove = cool_destroy_snapshot(false);
}
int __fastcall observe_destroy(void* npc, void*, void* message) {
  const DWORD incoming = GetLastError();
  const auto caller = reinterpret_cast<std::uint32_t>(_ReturnAddress());
  cool_predebit_journal.entered();
  const bool own = own_thread();
  if (own)
    ++cool_destroy_depth;
  const auto& request = handoff.request();
  const bool candidate = own && !cool_writer_scope && (cool_job || grind_job) &&
                         result_guard.state() == ubc::CraftingResultState::waiting &&
                         reinterpret_cast<std::uint32_t>(npc) == request.key.hero;
  auto& destroy = grind_job ? grind_destroy_call.destroy : cool_destroy_call.destroy;
  auto& entries =
      grind_job ? grind_destroy_call.destroy_entries : cool_destroy_call.destroy_entries;
  auto& reentry = grind_job ? grind_destroy_call.reentry_seen : cool_destroy_call.reentry_seen;
  if (own && cool_destroy_depth > 1)
    reentry = true;
  if (candidate) {
    ++entries;
    if (entries == 1) {
      if (grind_job)
        grind_destroy_binding = {
            grind_key(context()), request.key.generation,         frame,
            grind_job_floor,      grind_prepared.before.callback, request.quantity};
      else
        cool_destroy_binding = {result_guard.cool_prepared().valid()
                                    ? result_guard.cool_prepared().key()
                                    : cool_active_entry.prepared.key,
                                request.key.generation,
                                frame,
                                cool_job_floor,
                                request.id,
                                request.quantity};
      destroy.caller = caller;
      destroy.npc = reinterpret_cast<std::uint32_t>(npc);
      destroy.message = reinterpret_cast<std::uint32_t>(message);
      destroy.depth = cool_destroy_depth;
      const ddispatch::Invocation invocation{destroy.npc, destroy.message, caller,
                                             cool_destroy_depth};
      const auto dispatch = destroy_verified_path(cool_owned_patches(), &invocation,
                                                  cool_job && result_guard.cool_prepared().valid());
      destroy.message_class_verified = dispatch.valid;
      if (dispatch.valid) {
        destroy.classdef = cabi::manipulate_classdef;
        destroy.subtype = dispatch.dispatch.values.message_subtype;
      } else
        event("destroy_dispatch_rejected", static_cast<int>(dispatch.dispatch.issue),
              static_cast<int>(dispatch.address));
      if (grind_job)
        grind_destroy_call.before_destroy = grind_destroy_snapshot(true);
      else
        cool_destroy_call.before_destroy = cool_destroy_snapshot(true);
    } else
      reentry = true;
    ++destroy.original_calls;
  }
  int result = 0;
  DWORD error = incoming;
  SetLastError(incoming);
  __try {
    result = original_destroy(npc, message);
    error = GetLastError();
    if (candidate) {
      destroy.normal_return = true;
      destroy.return_value = result;
      cdestroy::Result proof;
      if (grind_job) {
        ++grind_destroy_call.destroy_exits;
        grind_destroy_call.after_destroy = grind_destroy_snapshot(false);
        ubc::GrindCheck check;
        proof = shdestroy::verify(grind_prepared, grind_destroy_binding, grind_destroy_call, check);
        if (!proof.valid)
          log_grind_check(check, static_cast<int>(request.id));
      } else {
        ++cool_destroy_call.destroy_exits;
        cool_destroy_call.after_destroy = cool_destroy_snapshot(false);
        proof = result_guard.cool_prepared().valid()
                    ? cdestroy::verify(result_guard.cool_prepared(), result_guard.cool_witness(),
                                       cool_destroy_binding, cool_destroy_call)
                    : cdestroy::verify(cool_active_entry.prepared, cool_destroy_binding,
                                       cool_destroy_call);
      }
      if (proof.valid) {
        if (result_guard.destroy_after({true, true, request.key, request.id, request.quantity,
                                        proof.serial, proof.frame, proof.instance,
                                        static_cast<std::uint32_t>(proof.amount)}))
          event(grind_job ? "grind_destroy_verified" : "cool_destroy_verified", proof.instance,
                proof.amount);
        else {
          if (grind_job) {
            ubc::GrindCheck check;
            check.equal("destroy.result_guard", true, false);
            log_grind_check(check, static_cast<int>(request.id));
          }
          result_guard.fail(ubc::CraftingResultIssue::destroy);
          event(grind_job ? "grind_destroy_rejected" : "cool_destroy_rejected", -1);
        }
      } else {
        result_guard.fail(ubc::CraftingResultIssue::destroy);
        event(grind_job ? "grind_destroy_rejected" : "cool_destroy_rejected",
              static_cast<int>(proof.issue));
        if (!grind_job)
          log_cool_destroy_hand_issue(proof.issue);
      }
    }
  } __finally {
    if (AbnormalTermination()) {
      error = GetLastError();
      if (candidate) {
        destroy.exception = true;
        result_guard.fail(ubc::CraftingResultIssue::destroy);
      }
    }
    if (own)
      --cool_destroy_depth;
    cool_predebit_journal.exited();
    SetLastError(error);
  }
  return result;
}
} // namespace ubc_native
