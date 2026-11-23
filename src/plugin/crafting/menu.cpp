// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
namespace {
// A rejected view also defers notices until the player leaves this station.
std::uint32_t rejected_view_station = 0;
ubc::CraftingPreviewCache preview_cache;

const char* menu_open_fields() noexcept {
  return ui.uses_workshop_screen_init()
             ? R"(,"field":"zCMenu::ScreenInit","status":"L'Hiver Workshop stack, patch source pending live identification")"
             : "";
}

ubc::CraftingPreviewKey preview_key(const Context& c) noexcept {
  const auto* selected = session.selected();
  return {key(c),
          world_epoch,
          rows.generation,
          selected ? selected->id : UINT32_MAX,
          session.quantity(),
          c.dialog,
          rows.count,
          rows.valid};
}

const char* preview_gate(const Context& c) noexcept {
  if (!ubc::CraftingPreviewCache::allows(session.phase()))
    return "phase";
  if (!same_context(c, borrowed_context))
    return "context";
  if (!(key(c) == session.preview().key))
    return "owner";
  if (!ui.owns_input())
    return "input_owner";
  if (!grind_frame_capture_quiet())
    return "frame_quiet";
  if (native_update)
    return "native_update";
  if (disabled)
    return "disabled";
  if (adapter_failed())
    return "adapter";
  if (grind_preview_input_depth)
    return "input_depth";
  if (grind_preview_overflow)
    return "overflow";
  if (grind_capture_leased)
    return "capture_lease";
  if (grind_preview_scope.active)
    return "active_scope";
  switch (c.kind) {
  case cat::Station::alchemy:
    return extra_catalogue_reader && spirit_catalogue_reader && tobacco_list &&
                   extra_serial != UINT64_MAX
               ? nullptr
               : "alchemy_reader";
  case cat::Station::bow:
    return bow_initialized && bow_item_reader ? nullptr : "bow_reader";
  case cat::Station::grind:
    if (!grind_preview_reader || !grind_item_reader || grind_preview_serial == UINT64_MAX)
      return "grind_reader";
    [[fallthrough]];
  case cat::Station::cool:
    return cool_ready && cool_destroy_installed && cool_remove_observer.owns_patch()
               ? nullptr
               : "remove_observer";
  case cat::Station::saw:
    return ammo::permits_next() ? nullptr : "ammo";
  default:
    return nullptr;
  }
}

void observe_cached_quantity(const Context& c) noexcept {
  const auto* selected = session.selected();
  if (!selected || (c.kind != cat::Station::grind && c.kind != cat::Station::bow &&
                    c.kind != cat::Station::cool))
    return;
  const ubc::CraftingRequest request{selected->id, session.quantity(), session.preview().key,
                                     *selected, session.preview().hands};
  session.observe_quantity_check(request, frame, preview_cache.quantity_available());
  if (c.kind != cat::Station::cool)
    grind_quantity_observation = {true, request.key.generation, frame, request.id,
                                  request.quantity};
}

ubc::CraftingPreviewCache::GrindUnits
verified_units(const Context& c, const GrindPreviewLease& lease,
               const ubc::CraftingSnapshot& snapshot) noexcept {
  ubc::CraftingPreviewCache::GrindUnits units{};
  const auto* published = lease.view(grind_key(c));
  if (published && ubc::valid_crafting_snapshot(snapshot)) {
    for (std::size_t i = 0; i < units.size(); ++i) {
      const auto& row = snapshot.recipes[cat::grind_begin + i];
      const ubc::CraftingRequest request{row.id, 1, snapshot.key, row, snapshot.hands};
      units[i] = ubc::crafting_handoff_detail::valid_request(request) &&
                 shpreview::quantity_matches(*published, request);
    }
  }
  return units;
}

using PreviewMiss = ubc::CraftingPreviewCache::Miss;

// Every cold frame points to its reason. Emit the address and classification
// once per run of identical misses, with a periodic refresh for long runs.
std::uint64_t report_grind_miss(const PreviewMiss& miss) noexcept {
  static PreviewMiss previous;
  static std::uint64_t first = 0;
  if (!first || frame - first >= 60 || std::strcmp(previous.reason, miss.reason) ||
      std::strcmp(previous.field, miss.field) || previous.address != miss.address ||
      previous.length != miss.length || previous.offset != miss.offset ||
      previous.subject != miss.subject) {
    char fields[224]{};
    std::snprintf(
        fields, sizeof(fields),
        R"(,"reason":"%s","field":"%s","address":%u,"length":%u,"offset":%u,"before":%llu,"after":%llu,"subject":%u)",
        miss.reason, miss.field, static_cast<unsigned>(miss.address), miss.length, miss.offset,
        static_cast<ULONGLONG>(miss.before), static_cast<ULONGLONG>(miss.after), miss.subject);
    event("grind_preview_cache_miss", 0, 0, fields);
    first = frame;
    previous = miss;
  }
  return first;
}

void report_grind_recompute(ULONGLONG started, std::uint32_t quantity, bool sealed,
                            const PreviewMiss& miss) noexcept {
  const auto elapsed = grind_microseconds(started, grind_counter());
  const auto miss_frame = report_grind_miss(miss);
  char fields[256]{};
  const auto prefix =
      std::snprintf(fields, sizeof(fields), R"(,"elapsed_us":%llu,"miss_frame":%llu)", elapsed,
                    static_cast<ULONGLONG>(miss_frame));
  if (sealed && grind_preview_reader) {
    const auto& timing = grind_preview_reader->read_set();
    const auto record = grind_microseconds(timing.start, timing.frozen);
    const auto replay = grind_microseconds(timing.frozen, timing.replayed);
    const auto seal = grind_microseconds(timing.replayed, timing.sealed);
    std::snprintf(
        fields + prefix, sizeof(fields) - prefix,
        R"(,"record_ms":%llu,"replay_ms":%llu,"seal_ms":%llu,"spans":%u,"calls":%llu,"record_us":%llu,"replay_us":%llu,"seal_us":%llu)",
        record / 1000, replay / 1000, seal / 1000, static_cast<unsigned>(timing.statistics().spans),
        timing.statistics().recording_calls, record, replay, seal);
  }
  event("grind_preview_recompute", static_cast<int>(elapsed / 1000), static_cast<int>(quantity),
        fields);
}

Capture capture_opening(const Context& c, ubc::BowMenuTrace* trace) noexcept {
  if (c.kind != cat::Station::grind)
    return capture(c, nullptr, trace);
  const auto started = grind_counter();
  auto inputs = preview_key(c);
  // A new generation has no selected recipe until session.open() chooses one.
  inputs.selection = UINT32_MAX;
  inputs.quantity = 0;
  const auto serial = grind_preview_input_serial;
  PreviewRecording recording(preview_cache, inputs);
  GrindPreviewLease opening(c, false);
  report_grind_preview_failure(opening);
  auto result = capture(c, &opening);
  const auto portions = verified_units(c, opening, result.snapshot);
  if (!recording.dialog_change && serial == grind_preview_input_serial && opening.ready)
    preview_cache.finish(inputs, false, portions);
  else if (serial != grind_preview_input_serial)
    preview_cache.invalidate({"input", "serial", 0, 0, 0, serial, grind_preview_input_serial});
  else
    preview_cache.invalidate(
        {"capture", recording.dialog_change ? recording.dialog_change : "publication"});
  report_grind_recompute(started, 1, opening.ready, {"key", "opening"});
  return result;
}

void refresh_menu_preview(const Context& c, uidiag::Frame& diagnostic) noexcept {
  const auto started = c.kind == cat::Station::grind ? grind_counter() : 0;
  const auto before = preview_key(c);
  const auto input = grind_preview_input_serial;
  const auto still_current = [&]() {
    const auto after = context();
    if (input != grind_preview_input_serial)
      preview_cache.invalidate({"input", "serial", 0, 0, 0, input, grind_preview_input_serial});
    else if (const auto* field = before.difference(preview_key(after)))
      preview_cache.invalidate({"key", field});
    else if (!same_context(c, after))
      preview_cache.invalidate({"gate", "context"});
    else if (const auto* gate = preview_gate(after))
      preview_cache.invalidate({"gate", gate});
    else
      return true;
    return false;
  };
  const auto* gate = preview_gate(c);
  const auto recapture = [&](const PreviewMiss& miss) {
    ubc::CraftingPreviewCache::GrindUnits units{};
    bool grind_sealed = false;
    std::optional<PreviewRecording> recording;
    if (!gate)
      recording.emplace(preview_cache, before);
    {
      std::optional<GrindPreviewLease> shared_grind;
      if (session.phase() != Phase::original && c.kind == cat::Station::grind) {
        shared_grind.emplace(c);
        report_grind_preview_failure(*shared_grind);
        grind_sealed = shared_grind->ready;
      }
      const auto* shared = shared_grind ? &*shared_grind : nullptr;
      if (session.phase() != Phase::original)
        session.refresh(capture(c, shared).snapshot);
      diagnostic.captured = GetTickCount64();
      if (session.phase() != Phase::original) {
        observe_bow_quantity(c);
        observe_cool_quantity(c);
        observe_grind_quantity(c, shared);
        if (recording && shared)
          units = verified_units(c, *shared, session.preview());
      }
    }
    // Neither recording nor a frame-bound grind publication survives into a
    // restored-view capture, the handoff, or an effect callback.
    const auto* dialog_change = recording ? recording->dialog_change : nullptr;
    recording.reset();
    if (!gate && !dialog_change && still_current())
      preview_cache.finish(before, session.can_confirm(), units);
    else if (dialog_change)
      preview_cache.invalidate({"capture", dialog_change});
    if (c.kind == cat::Station::grind && session.phase() != Phase::original)
      report_grind_recompute(started, session.quantity(), grind_sealed, miss);
  };
  if (preview_cache.reuse_or_capture(session.phase(), before, live_memory, gate, still_current,
                                     recapture)) {
    diagnostic.captured = GetTickCount64();
    observe_cached_quantity(c);
  }
}
}

bool render_selection(const Context* observed = nullptr, const uidiag::Frame* poll = nullptr,
                      bool poll_known = false) noexcept {
  const bool witness = grind_ui_trace_active && observed && poll;
  gready::Render readiness;
  if (witness) {
    readiness.epoch = session.preview().key.generation;
    readiness.frame = frame;
    readiness.input_serial_before = grind_ui_readiness.input_serial();
    readiness.before = ui.readiness_state(session);
    readiness.quantity = grind_quantity_observation;
    readiness.focused = poll->focused;
    readiness.held = poll->held;
    readiness.poll_known = poll_known;
  }
  const bool rendered = ui.render(session, execution_enabled);
  if (rendered && ui.owns_input())
    rendered_generation = session.preview().key.generation;
  else
    rendered_generation = 0;
  if (witness) {
    readiness.after = ui.readiness_state(session);
    readiness.rendered = rendered;
    readiness.focused = readiness.focused && ui.focused();
    readiness.owns_input = ui.owns_input();
    readiness.outer_quiet = grind_frame_capture_quiet();
    const auto* selected_recipe = cat::find(readiness.after.recipe);
    readiness.context_bound =
        observed->valid && selected_recipe && selected_recipe->station == observed->kind &&
        (observed->kind == cat::Station::grind || observed->kind == cat::Station::bow) &&
        key(*observed) == session.preview().key && same_context(*observed, borrowed_context) &&
        same_context(*observed, context());
    grind_readiness_events(grind_ui_readiness.after_render(readiness));
  }
  return rendered;
}

void close_rejected_view(const Context& c) noexcept {
  session.original(ubc::SelectionIssue::invalid_snapshot);
  rendered_generation = 0;
  original_station = rejected_view_station = c.station;
  close_ui(c);
}

bool open_selection_ui(const Context& c, ubc::BowMenuTrace* trace = nullptr,
                       ubc::BowMenuTrace* opened_trace = nullptr) noexcept {
  if (ui.open(trace, opened_trace))
    return true;
  if (const char* reason = ui.open_rejection()) {
    event(reason, static_cast<int>(c.kind));
    close_rejected_view(c);
  }
  return false;
}

template <class Borrow> bool show_selection_view(const Context& c, Borrow borrow) noexcept {
  const auto backdrop = ui.apply_backdrop();
  event(backdrop.event, static_cast<int>(c.kind));
  if (backdrop.view_valid) {
    if (render_selection()) {
      borrow(c);
      return true;
    }
    const char* reason = ui.text_rejection();
    if (!reason)
      reason = ui.text_view_rejected() ? "ui_text_view_mismatch" : "ui_initial_frame_rejected";
    event(reason, static_cast<int>(c.kind));
  }
  close_rejected_view(c);
  return false;
}

void reject_text_view(const Context& c) noexcept {
  const char* reason = ui.text_rejection();
  event(reason ? reason : "ui_text_view_mismatch", static_cast<int>(c.kind));
  if (ui.success_notice()) {
    success_pending = false;
    result_guard.fail(ubc::CraftingResultIssue::contract);
  }
  close_rejected_view(c);
}

bool notice_view_valid(const Context& c) noexcept {
  if (ui.has_valid_view())
    return true;
  reject_text_view(c);
  return false;
}

bool open_success_notice(const Context& c) noexcept {
  if (ui.open_success(pending_success)) {
    if (!notice_view_valid(c))
      return false;
    if (c.valid && c.dialog.waiting && c.dialog.open)
      borrow_view(c);
    event("success_notice_opened", static_cast<int>(pending_success.id),
          static_cast<int>(pending_success.quantity), menu_open_fields());
    return true;
  }
  if (const char* reason = ui.open_rejection()) {
    event(reason, static_cast<int>(c.kind));
    success_pending = false;
    result_guard.fail(ubc::CraftingResultIssue::contract);
    close_rejected_view(c);
  }
  return false;
}

bool open_failure_notice(const Context& c) noexcept {
  if (ui.open_failure() && notice_view_valid(c)) {
    event("failure_notice_opened", 0, 0, menu_open_fields());
    return true;
  }
  if (const char* reason = ui.open_rejection()) {
    if (!failure_open_rejected_logged)
      event(reason, static_cast<int>(c.kind));
    close_rejected_view(c);
  }
  if (!failure_open_rejected_logged) {
    failure_open_rejected_logged = true;
    event("failure_notice_open_rejected");
  }
  return false;
}

void maybe_install_ammo(const Context& c) noexcept {
  if (!own_thread() || frame_depth != 1 || update_depth || info_depth || choice_depth ||
      station_depth || collection_depth)
    return;
  // Attempt once, only after the saw dialog is fully initialized.
  // The first world frame may still initialize third-party OnMessage
  // hooks. Never retry after rejection or weaken the guard.
  if (!ammo_install_attempted && c.valid && c.kind == cat::Station::saw && c.dialog.waiting &&
      c.dialog.open && !c.dialog.done && complete_collection(c) && !ui.opened() &&
      !handoff.active() && !amount_patch.active()) {
    ammo_install_attempted = true;
    if (!ammo::install(GetCurrentThreadId())) {
      const auto failure = ammo::status();
      event("ammo_hooks_rejected", static_cast<int>(failure.fault),
            static_cast<int>(failure.runtime_issue));
      event("ammo_compatibility_rejected", static_cast<int>(failure.compatibility.issue),
            static_cast<int>(failure.compatibility.compatibility));
      event("ammo_compatibility_site", static_cast<int>(failure.compatibility.stage),
            static_cast<int>(failure.compatibility.address));
    } else
      event("ammo_hooks_installed");
  }
}
void observe_result_notice(const Context& c) noexcept {
  const bool success = ui.success_notice();
  ui.observe_notice(frame, ui.buttons_held(), ui.focused());
  const auto can_present = [&](const Context& current) {
    const bool owner =
        current.identity_known && current.world && current.hero &&
        (!success || (!adapter_failed() && current.world == pending_success.key.world &&
                      current.hero == pending_success.key.hero));
    if (owner && ui.owns_input() && (!success || ui.focused()))
      return true;
    if (owner && success && success_pending && cool_job && !cool_receipt_acked &&
        result_guard.cool_witness().valid &&
        result_guard.state() == ubc::CraftingResultState::verified) {
      // The same order waits in its window for focus and input ownership.
      ui.observe_notice(frame, false, false);
    } else
      ui.dismiss_notice();
    return false;
  };
  const bool ready = can_present(c);
  if (ui.notice_return_ready()) {
    if (!success)
      original_station = c.station;
    close_ui(c);
    success_pending = false;
  } else if (ready && success && !ui.notice_dismissed()) {
    const bool rendered = ui.render_success();
    if (!can_present(context()))
      return;
    if (!rendered)
      result_guard.fail(ubc::CraftingResultIssue::contract);
    else if (ammo_job && !ammo_notice_acked && ui.success_receipt()) {
      ammo_notice_acked = ammo::ack_notice(pending_success.receipt, ui.success_receipt(), true);
      if (!ammo_notice_acked)
        result_guard.fail(ubc::CraftingResultIssue::contract);
    } else if (cool_job && !cool_receipt_acked && ui.success_receipt()) {
      cool_receipt_acked = result_guard.acknowledge_quantity_receipt(ui.success_receipt());
      if (!cool_receipt_acked)
        result_guard.fail(ubc::CraftingResultIssue::contract);
      else
        event("cool_quantity_receipt_verified", static_cast<int>(pending_success.id),
              static_cast<int>(pending_success.quantity));
    }
  } else if (!success)
    ui.render_failure();
}
void __fastcall after_frame(void* game, void* manager) {
  const auto error = GetLastError();
  std::uint32_t current = 0, owned = 0;
  if (!InterlockedCompareExchange(&installed, 0, 0) || !read(a::game_pointer, current) ||
      current != reinterpret_cast<std::uintptr_t>(game) ||
      !field(reinterpret_cast<std::uint32_t>(manager), 0xc, owned) || owned != current) {
    SetLastError(error);
    return;
  }
  const auto thread = static_cast<LONG>(GetCurrentThreadId());
  const auto owner = InterlockedCompareExchange(&engine_thread, thread, 0);
  if ((owner && owner != thread) || disabled) {
    SetLastError(error);
    return;
  }
  if (campfire_observation_mode) {
    if (frame != UINT64_MAX)
      ++frame;
    SetLastError(error);
    return;
  }
  ++frame_depth;
  if (frame_depth == 1 && !update_depth && !info_depth && !choice_depth && !station_depth &&
      !collection_depth) {
    if (frame != UINT64_MAX)
      ++frame;
    else
      disabled = true;
    const auto c = context();
    if (c.world != observed_world || c.hero != observed_hero) {
      observed_world = c.world;
      observed_hero = c.hero;
      if (world_epoch != UINT64_MAX)
        ++world_epoch;
      else
        disabled = true;
      remembered_world = remembered_hero = remembered_station = 0;
      remembered_recipe = UINT32_MAX;
    }
    if (tobacco_owner.ticket) {
      const bool departed =
          c.identity_known &&
          (c.world != tobacco_owner.key.world || c.hero != tobacco_owner.key.hero ||
           (c.station_known && c.station != tobacco_owner.key.station));
      const bool ready =
          c.valid && c.dialog.waiting && c.dialog.open && !c.dialog.done && complete_collection(c);
      tobacco_owner.observe(departed, ready, rows.generation, c.dialog.mode, c.dialog.info);
      if (!tobacco_owner.ticket)
        InterlockedExchange(&tobacco_unpaid_page, 0);
    }
    if (grind_measurement_mode) {
      initialize_end_compatibility();
      initialize_cool(c);
      initialize_grind(c);
      measure_grind_preview(c);
      --frame_depth;
      SetLastError(error);
      return;
    }
    initialize_end_compatibility();
    if (bow_diagnosis) {
      initialize_bow(c);
      observe_bow_entry(c, bow_candidate(c));
      --frame_depth;
      SetLastError(error);
      return;
    }
    initialize_extra(c);
    initialize_bow(c);
    initialize_cool(c);
    initialize_grind(c);
    maybe_install_ammo(c);
    observe_result(c);
    observe_ammo_frame_health();
    if (success_pending && ammo_job && !ammo_notice_ready && ammo::healthy()) {
      const auto notice = ammo::take_notice();
      if (notice.valid) {
        ammo_notice_ready =
            notice.job == pending_success.receipt && notice.lots == pending_success.quantity;
        if (!ammo_notice_ready)
          result_guard.fail(ubc::CraftingResultIssue::contract);
      }
    }
    const int state = (c.identity_known ? 1 : 0) | (c.world ? 2 : 0) | (c.hero ? 4 : 0) |
                      (c.station ? 8 : 0) | (c.dialog.valid ? 16 : 0) |
                      (c.ownership_known ? 32 : 0) | (c.valid ? 64 : 0) |
                      (c.dialog.waiting ? 128 : 0) | (c.dialog.open ? 256 : 0) |
                      (c.dialog.hidden ? 512 : 0) | (c.dialog.done ? 1024 : 0);
    if (state != last_frame_state) {
      last_frame_state = state;
      event("frame_state", state, c.dialog.mode);
    }
    if (adapter_failed()) {
      const int failed = (result_guard.state() == ubc::CraftingResultState::failed ? 1 : 0) |
                         (bc::diagnostics().writes_disabled ? 2 : 0) | (failure_notified ? 4 : 0) |
                         (ui.opened() ? 8 : 0) | (c.identity_known ? 16 : 0) | (c.world ? 32 : 0) |
                         (c.hero ? 64 : 0) | (integer(0x8d1e38, 8) == 0 ? 128 : 0);
      if (failed != last_failure_state) {
        last_failure_state = failed;
        event("failure_notice_state", failed, static_cast<int>(result_guard.issue()));
      }
    }
    if (original_station && (!c.station_known || c.station != original_station)) {
      original_station = 0;
      rejected_view_station = 0;
    }
    const bool allow_notice = !rejected_view_station;
    const bool bow_at_station = bow_candidate(c);
    observe_bow_entry(c, bow_at_station);
    ubc::BowMenuTrace rejection{bow_menu_rejected, false};
    auto* bow_trace = bow_at_station ? &rejection : nullptr;
    const bool bow_can_open = bow_at_station && !ui.opened() && bow_entry(c, rejection);
    if (ui.opened() && !ui.has_valid_view()) {
      reject_text_view(c);
    } else if (ui.opened()) {
      if (ui.failure_notice() || ui.success_notice()) {
        observe_result_notice(c);
      } else {
        uidiag::Frame diagnostic;
        diagnostic.started = GetTickCount64();
        diagnostic.before = ui.diagnostic_state(session);
        bool poll_known = false;
        diagnostic.held = ui.buttons_held(&poll_known);
        diagnostic.focused = ui.focused();
        session.observe(frame, diagnostic.held, diagnostic.focused);
        diagnostic.observed = ui.diagnostic_state(session);
        if (!same_context(c, borrowed_context) || !ui.owns_input())
          session.original(ubc::SelectionIssue::context_changed);
        diagnostic.capturing = GetTickCount64();
        refresh_menu_preview(c, diagnostic);
        diagnostic.checked = GetTickCount64();
        diagnostic.refreshed = ui.diagnostic_state(session);
        ui_diagnostic_events(ui_diagnostic.frame(session.preview().key.generation, diagnostic));
        if (session.phase() == Phase::original) {
          close_grind_readiness();
          if (session.return_ready()) {
            original_station = c.station;
            close_ui(c);
            event("ui_original");
          } else
            render_selection();
        } else if (session.phase() == Phase::ready) {
          const auto shown_key = session.preview().key;
          const bool shown = ui.object() != nullptr && ui.owns_input() &&
                             rendered_generation == shown_key.generation;
          close_ui(c);
          const auto restored = context();
          const auto captured = capture(restored);
          if (restored.kind == cat::Station::bow) {
            session.refresh(captured.snapshot);
            observe_bow_quantity(restored);
          }
          if (restored.kind == cat::Station::cool) {
            session.refresh(captured.snapshot);
            observe_cool_quantity(restored);
          }
          if (restored.kind == cat::Station::grind) {
            session.refresh(captured.snapshot);
            observe_grind_quantity(restored);
          }
          const auto request = session.take(captured.snapshot, true);
          if (request) {
            request_ui_generation =
                shown && request->key == shown_key ? request->key.generation : 0;
            request_ui_key = shown_key;
            record_materials(request->material_snapshot, "stock_before");
            event("recipe_request", static_cast<int>(request->id),
                  static_cast<int>(request->quantity));
            if (!execution_enabled) {
              original_station = c.station;
              event("preview_confirmed");
            } else {
              // begin already needs the requested recipe. Until
              // then, observation() still uses the previous order.
              tobacco_targets = {};
              tobacco_clean_rejection = false;
              auto initial = observation(restored, true, false);
              initial.materials = request->material_snapshot;
              initial.hands = captured.hands;
              const auto plan = ubc::make_crafting_plan(initial.page, *request);
              handed_hands = captured.hands;
              effect_executed = false;
              handoff_started = GetTickCount64();
              if (handoff.begin(*request, plan, initial, 36000))
                drive_handoff(restored);
              else {
                original_station = c.station;
                event("handoff_rejected", static_cast<int>(handoff.issue()));
              }
            }
          } else {
            original_station = c.station;
            event("handoff_stale");
          }
        } else if (!render_selection(&c, &diagnostic, poll_known))
          session.original(ubc::SelectionIssue::invalid_snapshot);
      }
    } else if (success_pending && !adapter_failed() && allow_notice) {
      if (c.identity_known && c.world == pending_success.key.world &&
          c.hero == pending_success.key.hero && integer(0x8d1e38, 8) == 0 &&
          (!ammo_job || ammo_notice_ready))
        open_success_notice(c);
    } else if (tobacco_clean_rejection && c.valid && allow_notice && open_failure_notice(c)) {
      tobacco_clean_rejection = false;
      original_station = c.station;
      borrow_view(c);
      ui.render_failure();
    } else if (!failure_notified && allow_notice && c.identity_known && c.world && c.hero &&
               integer(0x8d1e38, 8) == 0 && (adapter_failed())) {
      if (open_failure_notice(c)) {
        failure_notified = true;
        if (c.valid && c.dialog.waiting && c.dialog.open)
          borrow_view(c);
        ui.render_failure();
      }
    } else if (handoff.active()) {
      drive_handoff(c);
      if (handoff.state() == ubc::CraftingHandoffState::cancelled)
        original_station = c.station;
    } else if (bow_at_station
                   ? bow_can_open
                   : (!tobacco_owner.ticket && !original_station && c.valid && c.dialog.waiting &&
                      c.dialog.open && complete_collection(c) &&
                      ubc::NativeSelectionUi::callback_is_top(c.dialog.view + a::view_callback) &&
                      integer(0x8d1e38, 8) == 0)) {
      const bool failure = adapter_failed();
      if (failure && !failure_notified) {
        if (open_failure_notice(c)) {
          failure_notified = true;
          borrow_view(c);
          ui.render_failure();
        }
      } else if (!failure) {
        const auto current_page = page(c, bow_trace);
        if (ubc::valid_crafting_page(current_page)) {
          if (generation != UINT64_MAX)
            ++generation;
          else {
            disabled = true;
            if (bow_trace)
              bow_trace->equal("entry.generation_exhausted", 0, 1);
          }
          // At our first station, the actual new preview generation
          // now exists. The original dialog is still visible.
          if (!disabled)
            initialize_cool(c);
          const auto opening_serial = grind_preview_input_serial;
          const auto captured = capture_opening(c, bow_trace);
          if (bow_trace) {
            bow_trace->equal("session.generation", 1,
                             session.accepts_generation(captured.snapshot.key.generation));
            bow_trace->equal("session.snapshot", 1,
                             ubc::valid_crafting_snapshot(captured.snapshot));
          }
          const bool compare = bow_comparison && bow_at_station;
          bow_sample_count = 0;
          if (compare && bow_measured_frame != frame && !disabled &&
              session.accepts_generation(captured.snapshot.key.generation) &&
              ubc::valid_crafting_snapshot(captured.snapshot)) {
            ubc::BowMenuTrace buffered{buffer_bow_value, true};
            measure_bow_menu(c, generation, buffered);
          }
          const bool session_open =
              !disabled && session.open(captured.snapshot, c.world == remembered_world &&
                                                                   c.hero == remembered_hero &&
                                                                   c.station == remembered_station
                                                               ? remembered_recipe
                                                               : UINT32_MAX);
          ubc::BowMenuTrace window_trace{bow_menu_window, true};
          if (!session_open) {
            const int issue = captured.issue;
            if (issue != last_capture_issue) {
              last_capture_issue = issue;
              event("catalogue_unavailable", issue);
            }
          } else if (!open_selection_ui(c, bow_trace, compare ? &window_trace : nullptr)) {
            if (!rejected_view_station) {
              const int issue = captured.issue;
              if (issue != last_capture_issue) {
                last_capture_issue = issue;
                event("catalogue_unavailable", issue);
              }
            }
          } else if (show_selection_view(c, borrow_view)) {
            if (c.kind == cat::Station::grind) {
              const auto borrowed = context();
              if (grind_preview_input_serial != opening_serial)
                preview_cache.invalidate(
                    {"input", "serial", 0, 0, 0, opening_serial, grind_preview_input_serial});
              else if (const auto* gate = preview_gate(borrowed))
                preview_cache.invalidate({"gate", gate});
              else if (!preview_cache.bind_open_grind(preview_key(borrowed)))
                preview_cache.invalidate();
            }
            event("ui_open", static_cast<int>(c.kind), static_cast<int>(session.visible_count()),
                  menu_open_fields());
            ui_diagnostic_events(
                ui_diagnostic.begin(session.preview().key.generation, GetTickCount64()));
            if (c.kind == cat::Station::grind || c.kind == cat::Station::bow) {
              grind_quantity_observation = {};
              grind_ui_trace_active =
                  grind_ui_readiness.begin(session.preview().key.generation, frame);
            }
            for (const auto& r : captured.snapshot.recipes)
              if (r.known) {
                // Negative finding before this row. A reader must not serve
                // the last recipe_available before its contract block.
                if (!r.contract)
                  event("recipe_contract_unverified", static_cast<int>(r.id), 0);
                event("recipe_available", static_cast<int>(r.id), static_cast<int>(r.maximum));
              }
          }
        }
      }
    }
    // Capture and engine callbacks can change the view between the frame check and submission.
    if (ui.opened() && ui.text_view_rejected())
      reject_text_view(c);
    const int handoff_state = static_cast<int>(handoff.state());
    if (handoff_state != last_handoff_state) {
      if (handoff.state() == ubc::CraftingHandoffState::cancelled)
        original_station = c.station;
      last_handoff_state = handoff_state;
      event("handoff_state", handoff_state, static_cast<int>(handoff.issue()));
    }
  }
  --frame_depth;
  SetLastError(error);
}

void __fastcall observe_clear(void* view, void* unused) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool own = own_thread();
  if (own) {
    ++collection_depth;
    rows.begin(reinterpret_cast<std::uint32_t>(view),
               caller == a::info_clear_return && collection_depth == 1);
    if (caller != a::info_clear_return)
      rows.reject("info.clear_caller", a::info_clear_return, caller);
    else if (collection_depth != 1)
      rows.reject("info.clear_depth", 1, collection_depth);
    choice_collection = caller == a::choice_clear_return && collection_depth == 1;
    choice_count = 0;
  }
  SetLastError(error);
  original_clear(view, unused);
  const auto after = GetLastError();
  if (own) {
    const auto count = integer(reinterpret_cast<std::uint32_t>(view), a::view_count);
    if (count != 0) {
      rows.reject("info.clear_count", 0, count);
      choice_collection = false;
    }
    --collection_depth;
  }
  SetLastError(after);
}
void __fastcall observe_add(void* view, void* text, int number) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool own = own_thread();
  const auto address = reinterpret_cast<std::uint32_t>(view);
  const auto before = own ? integer(address, a::view_count) : -1;
  ubc::SelectionInfo info;
  ubc::ChoiceSnapshot choice;
  if (own) {
    ++collection_depth;
    const auto text_address = reinterpret_cast<std::uint32_t>(text);
    if (caller == a::info_add_return && text_address >= 0x30)
      info = ubc::read_selection_info(memory, text_address - 0x30);
    if (caller == a::choice_add_return)
      choice = ubc::read_choice(memory, s::information_manager, text_address);
  }
  SetLastError(error);
  original_add(view, text, number);
  const auto after = GetLastError();
  if (own) {
    const auto count = integer(address, a::view_count);
    if (caller == a::info_add_return) {
      if (collection_depth != 1)
        rows.reject("info.add_depth", 1, collection_depth);
      if (number != 0)
        rows.reject("info.add_number", 0, number);
      rows.append(address, info, before, count, true);
    } else
      rows.reject("info.add_caller", a::info_add_return, caller);
    if (choice_collection) {
      if (caller != a::choice_add_return || collection_depth != 1 || number != 0 || !choice.valid ||
          before < 0 || static_cast<std::size_t>(before) != choice_count || count != before + 1 ||
          choice.index != before || choice_count == choice_functions.size())
        choice_collection = false;
      else
        choice_functions[choice_count++] = choice.function;
    }
    --collection_depth;
  }
  SetLastError(after);
}

int __fastcall observe_menu_event(void* menu, void*, int input) {
  const auto error = GetLastError();
  if (own_thread() && ui.opened() && menu == ui.object()) {
    // Independently of the bounded diagnostics, every entry into our callback
    // revokes an active scope, including unknown, no-op or rejected input.
    bool counted = false;
    int result = 1;
    if (grind_preview_input_serial == UINT64_MAX || grind_preview_input_depth == UINT_MAX)
      grind_preview_overflow = true;
    else {
      ++grind_preview_input_serial;
      ++grind_preview_input_depth;
      counted = true;
    }
    __try {
      uidiag::Action diagnostic;
      result = ui.handle(session, input, &diagnostic);
      ui_diagnostic_events(ui_diagnostic.input(session.preview().key.generation, diagnostic));
      if (grind_ui_trace_active && diagnostic.observed)
        grind_ui_readiness.observe_input(session.preview().key.generation, frame, diagnostic);
    } __finally {
      if (counted)
        --grind_preview_input_depth;
      SetLastError(error);
    }
    return result;
  }
  SetLastError(error);
  return original_menu_event(menu, input);
}
int __fastcall observe_choice_event(void* callback, void*, int input) {
  const auto error = GetLastError();
  if (own_thread() && handoff.active()) {
    const auto c = context();
    if (private_enter) {
      private_enter = false;
      const auto current = observation(c, false);
      if (input == m::key_return &&
          reinterpret_cast<std::uint32_t>(callback) == dispatched_ticket.view + a::view_callback &&
          c.dialog.selected == dispatched_ticket.offer.row && current.ready &&
          handoff.matches_pending(dispatched_ticket, current, resolve_offer(c, handoff.step()))) {
        SetLastError(error);
        return original_choice_event(callback, input);
      }
      handoff.cancel(ubc::CraftingHandoffIssue::offer);
      SetLastError(error);
      return 1;
    }
    auto actual = observation(c, false, false);
    if (dispatched_ticket.view &&
        reinterpret_cast<std::uint32_t>(callback) == dispatched_ticket.view + a::view_callback)
      actual.view = dispatched_ticket.view;
    if (!foreign_dialog(c) &&
        reinterpret_cast<std::uint32_t>(callback) == dispatched_ticket.view + a::view_callback &&
        handoff.guards_input(actual) && !(tobacco_owner.ticket && handoff.cancelled())) {
      if (result_guard.state() != ubc::CraftingResultState::failed) {
        if (input == m::key_escape)
          handoff.cancel();
        SetLastError(error);
        return 1;
      }
    }
  }
  SetLastError(error);
  return original_choice_event(callback, input);
}
} // namespace ubc_native
