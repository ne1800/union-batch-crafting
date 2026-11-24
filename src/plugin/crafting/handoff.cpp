// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
Context borrowed_context;

ubc::CraftingHandoffObservation observation(const Context& c, bool outer, bool materials) noexcept {
  ubc::CraftingHandoffObservation out;
  out.key = key(c);
  out.view = c.dialog.view;
  out.frame = frame;
  out.collection = rows.generation;
  // Missing production or profile information does not establish a station change.
  // If the kind is unknown, the handoff retains its previous input boundary.
  out.context_known = c.valid && !foreign_dialog(c);
  out.mode = c.dialog.mode == 1 ? basecat::Invocation::info : basecat::Invocation::choice;
  out.count = c.dialog.count;
  out.complete = complete_collection(c);
  out.ready = c.valid && c.dialog.waiting && c.dialog.open && !c.dialog.done;
  out.outer_frame = outer;
  out.quiescent = outer && frame_depth == 1 && !update_depth && !info_depth && !choice_depth &&
                  !station_depth && !collection_depth;
  out.focused = ui.focused();
  out.page = page(c);
  out.page_known = c.valid && ubc::valid_crafting_page(out.page);
  if (c.dialog.mode == 2)
    out.choice_owner = ubc::read_selection_info(memory, c.dialog.info).instance;
  if (materials && c.valid) {
    const auto& request = handoff.request();
    const auto* d = cat::find(request.id);
    if (d && d->tobacco && handoff.active()) {
      ubc::GrindCheck check;
      tobacco::Capture sample;
      const bool script = tobacco::verify_script(memory, check);
      bool fresh = read_tobacco_state(c, *d->tobacco, script, out.materials, sample, check);
      if (fresh && tobacco_owner.ticket && tobacco_owner.key == out.key) {
        fresh = tobacco::match_stock(*d->tobacco, tobacco::stocks(request.material_snapshot),
                                     sample.stock, check);
        out.materials.maximum = request.material_snapshot.maximum;
        if (fresh && out.complete && out.ready &&
            out.page.kind == ubc::CraftingPageKind::tobacco_choices)
          fresh = check.equal("owner.collection", true, tobacco_owner.bind(out.collection));
      }
      if (!fresh) {
        out.materials.contract = false;
        log_tobacco_check(check, d->id);
      }
      out.hands = sample.hands;
      return out;
    }
    const auto current = capture(c);
    if (const auto* material = state_for(current.snapshot, handoff.request().id))
      out.materials = *material;
    out.hands = current.hands;
    if (!(current.snapshot.key == out.key))
      out.context_known = false;
  }
  return out;
}
ubc::CraftingHandoffOffer resolve_offer(const Context& c, const ubc::CraftingStep* step) noexcept {
  if (!step || !complete_collection(c))
    return {};
  ubc::CraftingHandoffOffer offer;
  if (step->invocation == basecat::Invocation::info && c.dialog.mode == 1) {
    for (std::size_t i = 0; i < rows.count; ++i) {
      const auto& row = rows.rows[i];
      if (linked_step_info(row, step) && row.instance == step->owner_instance &&
          row.function == step->function_symbol) {
        if (offer.valid)
          return {};
        offer = {true,         basecat::Invocation::info,    row.instance,
                 row.function, static_cast<std::int32_t>(i), rows.generation};
      }
    }
  } else if (step->invocation == basecat::Invocation::choice && c.dialog.mode == 2) {
    ubc::SelectionChoices choices;
    if (!complete_collection(c, &choices) || choices.owner.instance != step->owner_instance)
      return {};
    for (std::size_t i = 0; i < choices.count; ++i)
      if (choices.functions[i] == step->function_symbol) {
        if (offer.valid)
          return {};
        offer = {true,
                 basecat::Invocation::choice,
                 choices.owner.instance,
                 choices.functions[i],
                 static_cast<std::int32_t>(i),
                 rows.generation};
      }
  }
  return offer;
}
void restore_view(const Context& c) noexcept {
  if (!borrowed_view)
    return;
  if (same_owner(c, borrowed_context) && c.dialog.mode == borrowed_context.dialog.mode &&
      c.dialog.info == borrowed_context.dialog.info && c.dialog.view == borrowed_view &&
      c.dialog.waiting && c.dialog.hidden) {
    reinterpret_cast<a::FastObject>(a::choice_open_immediate)(
        reinterpret_cast<void*>(borrowed_view), nullptr);
    reinterpret_cast<a::Select>(a::choice_activate)(reinterpret_cast<void*>(borrowed_view), 1);
  }
  borrowed_view = 0;
}
void close_ui(const Context& c) noexcept {
  close_grind_readiness();
  ui_diagnostic.end();
  ui.close();
  restore_view(c);
}
void borrow_view(const Context& c) noexcept {
  borrowed_view = c.dialog.view;
  borrowed_context = c;
  reinterpret_cast<a::Select>(a::choice_activate)(reinterpret_cast<void*>(borrowed_view), 0);
  reinterpret_cast<a::FastObject>(a::choice_close_immediate)(reinterpret_cast<void*>(borrowed_view),
                                                             nullptr);
}
void record_materials(const ubc::CraftingState& value, const char* name) noexcept {
  for (std::size_t i = 0; i < value.target_count; ++i)
    event(name, value.materials[i].symbol, value.materials[i].stored);
}
void drive_handoff(const Context& c) noexcept {
  if (!handoff.active())
    return;
  const auto current = observation(c, true);
  if (GetTickCount64() - handoff_started > 30000)
    handoff.cancel(ubc::CraftingHandoffIssue::timeout);
  handoff.observe(current);
  if (foreign_dialog(c))
    handoff.cancel(ubc::CraftingHandoffIssue::context);
  if (handoff.state() == ubc::CraftingHandoffState::quarantined) {
    bool finished =
        c.identity_known && c.station_known && integer(s::information_manager, 0x2c) == 1;
    for (const auto offset : {0x30u, 0x34u, 0x38u, 0x3cu, 0x40u, 0x44u})
      if (integer(s::information_manager, offset) != 0)
        finished = false;
    if (finished)
      handoff.context_ended(
          {handoff.request().key, dispatched_ticket.view, frame, true, true, true});
  }
  if (handoff.state() != ubc::CraftingHandoffState::awaiting_step)
    return;
  if (!current.ready || !current.complete ||
      !ubc::NativeSelectionUi::callback_is_top(c.dialog.view + a::view_callback) ||
      integer(0x8d1e38, 8) != 0) {
    handoff.cancel(ubc::CraftingHandoffIssue::boundary);
    return;
  }
  const auto offer = resolve_offer(c, handoff.step());
  const auto ticket = handoff.ticket(current, offer);
  if (!ticket)
    return;
  const auto fresh = context();
  if (!handoff.dispatch(*ticket, observation(fresh, true), resolve_offer(fresh, handoff.step())))
    return;
  dispatched_ticket = *ticket;
  auto* view = reinterpret_cast<void*>(fresh.dialog.view);
  reinterpret_cast<a::Select>(a::choice_select)(view, offer.row);
  // Only this one synthetic input passes through the view hook. Physical
  // input remains guarded during the original category continuations.
  private_enter = true;
  reinterpret_cast<a::HandleEvent>(a::choice_handle_event)(
      static_cast<std::uint8_t*>(view) + a::view_callback, m::key_return);
  private_enter = false;
  const auto after = ubc::read_selection_dialog(memory, s::information_manager);
  if (!after.valid || !after.done || after.selected != offer.row)
    handoff.cancel(ubc::CraftingHandoffIssue::offer);
  event("handoff_step", offer.function, static_cast<int>(ticket->step));
}

bool extra_result_complete(const Context& c, const cat::Descriptor& recipe) noexcept {
  if (!extra_executor || !recipe.alchemy_extra) {
    result_guard.fail(ubc::CraftingResultIssue::contract);
    return false;
  }
  if (!extra_normal_end || frame <= extra_end_frame)
    return false;
  const auto quiet = []() noexcept {
    const auto bridge = bc::diagnostics();
    return own_thread() && frame_depth == 1 && !native_update && !update_depth && !info_depth &&
           !choice_depth && !station_depth && !collection_depth && !resolved_read_depth &&
           !amount_patch.active() && !extra_writer_scope && !bridge.writer && !bridge.readers &&
           !bridge.writes_disabled && extra_executor && !extra_executor->diagnostics().active &&
           !extra_executor->diagnostics().disabled;
  };
  if (!quiet() || !c.identity_known || !c.station_known)
    return false;
  ubc::AlchemyExtraEndFrame end;
  end.before = extra_context(c);
  end.current_thread = GetCurrentThreadId();
  if (!xdebit::same(end.before, extra_active_job.context, false)) {
    result_guard.fail(ubc::CraftingResultIssue::context);
    return false;
  }
  // Re-establish the semantic bindings for C_NPC/AIVAR/index and production.
  // The direct AIVAR read is meaningful only with these exact current bindings.
  namespace xr = ubc::alchemy_flask_runtime;
  rt::detail::Layout layout{};
  if (rt::detail::read_layout(memory, s::game_parser, xr::flask_profile, layout) != rt::Issue::none)
    return false;
  for (const auto index : {4u, 7u, 8u, 9u})
    if (rt::detail::check_binding(memory, layout, xr::flask_bindings[index]) != rt::Issue::none)
      return false;
  auto production = rt::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
  if (!production.valid || !field(c.hero, 0x298, end.invincible_before))
    return false;
  end.production_before = production.value;
  if (ubc::station_recipe_snapshot_detail::begin_hands(memory, c.hero, result_hands) !=
      ubc::StationHandReadIssue::none)
    return false;
  end.hands_before = result_hands.before;
  const auto inventory = ubc::read_packed_inventory<cat::target_capacity>(
      memory, c.hero, recipe.targets.data, recipe.targets.count, true, result_workspace);
  std::array<ubc::RecipeInventoryItem, cat::target_capacity> items{};
  for (std::size_t i = 0; i < recipe.targets.count; ++i)
    items[i] = {recipe.targets[i].symbol,
                inventory.listed[i],
                inventory.packed[i],
                inventory.total[i],
                inventory.listed_occurrences[i],
                inventory.packed_occurrences[i],
                inventory.duplicate_packed_target[i],
                inventory.has_active_marker[i],
                inventory.mixed_representation[i]};
  if (!ubc::station_recipe_snapshot_detail::verify_hands(memory, result_hands) ||
      rt::detail::stable_layout(memory, s::game_parser, xr::flask_profile, layout) !=
          rt::Issue::none)
    return false;
  end.hands_after = result_hands.before; // verify_hands has reread the same full snapshot.
  production = rt::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
  if (!production.valid || !field(c.hero, 0x298, end.invincible_after))
    return false;
  end.production_after = production.value;
  const auto after = context();
  if (!after.identity_known || !after.station_known || !quiet())
    return false;
  end.after = extra_context(after);
  end.serial = next_extra_serial();
  end.boundary_verified = true;
  // Both scopes are actually quiescent. Before and after the snapshot,
  // quiet() checked TLS, writer and executor state. No simulated phases.
  end.inventory = {inventory.header_known,
                   inventory.listed_known,
                   inventory.packed_known,
                   inventory.storage_totals_known,
                   inventory.stable_boundary_verified,
                   inventory.snapshot_unchanged,
                   inventory.issue,
                   inventory.listed_nodes,
                   inventory.packed_entries,
                   {items.data(), recipe.targets.count},
                   {}};
  if (!inventory.snapshot_unchanged || inventory.issue != ubc::PackedInventoryIssue::none)
    return false;
  const auto snapshot =
      ubc::alchemy_extra_end_snapshot(*recipe.alchemy_extra, extra_active_job, end);
  if (!snapshot.valid) {
    result_guard.fail(ubc::CraftingResultIssue::contract);
    event("extra_end_snapshot_rejected", static_cast<int>(snapshot.issue));
    return false;
  }
  const auto stage =
      extra_executor->observe_completion(snapshot.observation, extra_end, extra_normal_end);
  if (stage == ubc::AlchemyExtraResultStage::blocked) {
    result_guard.fail(ubc::CraftingResultIssue::stock);
    return false;
  }
  const auto& receipt = extra_executor->receipt();
  const bool complete =
      stage == ubc::AlchemyExtraResultStage::complete && receipt.verified &&
      receipt.job == extra_active_job.id && receipt.writer_token == extra_active_job.writer_token &&
      receipt.id == recipe.alchemy_extra->id && receipt.quantity == extra_active_job.quantity;
  if (complete)
    event("extra_result_verified", static_cast<int>(recipe.id), static_cast<int>(receipt.quantity));
  return complete;
}
bool spirit_result_complete(const Context& c, const cat::Descriptor& recipe,
                            ubc::CraftingResultObservation& value) noexcept {
  if (!spirit_executor || !recipe.spirit) {
    result_guard.fail(ubc::CraftingResultIssue::contract);
    return false;
  }
  if (!spirit_normal_end || frame <= spirit_end_frame)
    return false;
  const auto quiet = []() noexcept {
    const auto bridge = bc::diagnostics();
    return own_thread() && frame_depth == 1 && !native_update && !update_depth && !info_depth &&
           !choice_depth && !station_depth && !collection_depth && !resolved_read_depth &&
           !amount_patch.active() && !spirit_writer_scope && !bridge.writer && !bridge.readers &&
           !bridge.writes_disabled && spirit_executor && !spirit_executor->diagnostics().active &&
           !spirit_executor->diagnostics().disabled;
  };
  if (!quiet() || !c.identity_known || !c.station_known)
    return false;
  ubc::SpiritEndFrame end;
  end.before = extra_context(c);
  end.current_thread = GetCurrentThreadId();
  if (!gdebit::same(end.before, spirit_active_job.context, false)) {
    result_guard.fail(ubc::CraftingResultIssue::context);
    return false;
  }
  // Re-establish the semantic bindings for C_NPC/AIVAR/index and production.
  // The direct AIVAR read is meaningful only with these exact current bindings.
  namespace xr = ubc::alchemy_flask_runtime;
  rt::detail::Layout layout{};
  if (rt::detail::read_layout(memory, s::game_parser, xr::flask_profile, layout) != rt::Issue::none)
    return false;
  for (const auto index : {4u, 7u, 8u, 9u})
    if (rt::detail::check_binding(memory, layout, xr::flask_bindings[index]) != rt::Issue::none)
      return false;
  auto production = rt::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
  if (!production.valid || !field(c.hero, 0x298, end.invincible_before))
    return false;
  end.production_before = production.value;
  const auto profile = grt::assemble(*recipe.spirit);
  end.branch_before = grt::branch(memory, s::game_parser, profile);
  if (ubc::station_recipe_snapshot_detail::begin_hands(memory, c.hero, result_hands) !=
      ubc::StationHandReadIssue::none)
    return false;
  end.hands_before = result_hands.before;
  const auto inventory = ubc::read_packed_inventory<cat::target_capacity>(
      memory, c.hero, recipe.targets.data, recipe.targets.count, true, result_workspace);
  std::array<ubc::RecipeInventoryItem, cat::target_capacity> items{};
  for (std::size_t i = 0; i < recipe.targets.count; ++i)
    items[i] = {recipe.targets[i].symbol,
                inventory.listed[i],
                inventory.packed[i],
                inventory.total[i],
                inventory.listed_occurrences[i],
                inventory.packed_occurrences[i],
                inventory.duplicate_packed_target[i],
                inventory.has_active_marker[i],
                inventory.mixed_representation[i]};
  if (!ubc::station_recipe_snapshot_detail::verify_hands(memory, result_hands) ||
      rt::detail::stable_layout(memory, s::game_parser, xr::flask_profile, layout) !=
          rt::Issue::none)
    return false;
  end.hands_after = result_hands.before; // verify_hands has reread the same full snapshot.
  production = rt::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
  if (!production.valid || !field(c.hero, 0x298, end.invincible_after))
    return false;
  end.production_after = production.value;
  end.branch_after = grt::branch(memory, s::game_parser, profile);
  const auto after = context();
  if (!after.identity_known || !after.station_known || !quiet())
    return false;
  end.after = extra_context(after);
  end.serial = next_extra_serial();
  end.boundary_verified = true;
  // Both scopes are actually quiescent. Before and after the snapshot,
  // quiet() checked TLS, writer and executor state. No simulated phases.
  end.inventory = {inventory.header_known,
                   inventory.listed_known,
                   inventory.packed_known,
                   inventory.storage_totals_known,
                   inventory.stable_boundary_verified,
                   inventory.snapshot_unchanged,
                   inventory.issue,
                   inventory.listed_nodes,
                   inventory.packed_entries,
                   {items.data(), recipe.targets.count},
                   {}};
  if (!inventory.snapshot_unchanged || inventory.issue != ubc::PackedInventoryIssue::none)
    return false;
  const auto snapshot = ubc::spirit_end_snapshot(spirit_active_job, end);
  if (!snapshot.valid) {
    result_guard.fail(ubc::CraftingResultIssue::contract);
    event("spirit_end_snapshot_rejected", static_cast<int>(snapshot.issue));
    return false;
  }
  const auto stage =
      spirit_executor->observe_completion(snapshot.observation, spirit_end, spirit_normal_end);
  if (stage == ubc::SpiritResultStage::blocked) {
    result_guard.fail(ubc::CraftingResultIssue::stock);
    return false;
  }
  const auto& receipt = spirit_executor->receipt();
  const bool complete = stage == ubc::SpiritResultStage::verified && receipt.verified &&
                        receipt.job == spirit_active_job.id &&
                        receipt.writer_token == spirit_active_job.writer_token &&
                        receipt.quantity == spirit_active_job.quantity &&
                        receipt.branch_value == spirit_active_job.branch_value &&
                        receipt.output == handoff.request().material_snapshot.branch_output &&
                        receipt.count == recipe.targets.count;
  if (complete) {
    if (!value.stock_known || value.items.count != receipt.count) {
      result_guard.fail(ubc::CraftingResultIssue::stock);
      return false;
    }
    for (std::size_t i = 0; i < receipt.count; ++i) {
      const auto& row = receipt.rows[i];
      bool matched = false;
      for (const auto& stock : value.items)
        if (stock.symbol == static_cast<std::int32_t>(row.symbol) && stock.total == row.actual)
          matched = true;
      if (!matched) {
        result_guard.fail(ubc::CraftingResultIssue::stock);
        return false;
      }
    }
    value.branch = {true,
                    true,
                    spirit_active_job.id,
                    recipe.id,
                    spirit_active_job.quantity,
                    receipt.branch_value};
  }
  if (complete)
    event("spirit_result_verified", static_cast<int>(recipe.id),
          static_cast<int>(receipt.quantity));
  return complete;
}
void observe_result(const Context& c) noexcept {
  if (result_guard.state() != ubc::CraftingResultState::waiting)
    return;
  const auto& request = handoff.request();
  const auto* recipe = cat::find(request.id);
  ubc::CraftingResultObservation value;
  value.generation = request.key.generation;
  value.frame = frame;
  value.stable_boundary_verified = true;
  value.identity_known = c.identity_known;
  value.world = c.world;
  value.hero = c.hero;
  value.station_known = c.station_known;
  value.station = c.station;
  const auto production = rt::symbol_int(memory, s::game_parser, production_symbol());
  value.production_known = production.valid;
  value.production = production.value;
  value.selection_known = c.dialog.valid;
  value.selection_owner_verified =
      c.dialog.valid && c.dialog.npc == request.key.hero && c.dialog.player == request.key.hero;
  value.selection_open = c.dialog.open;
  value.selection_waiting = c.dialog.waiting;
  value.selection_done = c.dialog.done;
  value.collection_known = c.valid && complete_collection(c);
  value.collection = rows.generation;
  std::array<ubc::RecipeResultItem, cat::target_capacity> items{};
  if (recipe && c.identity_known && c.world == request.key.world && c.hero == request.key.hero) {
    const auto inventory = ubc::read_packed_inventory<cat::target_capacity>(
        memory, c.hero, recipe->targets.data, recipe->targets.count, true, result_workspace);
    value.stock_known = inventory.storage_totals_known && inventory.snapshot_unchanged &&
                        inventory.stable_boundary_verified &&
                        inventory.issue == ubc::PackedInventoryIssue::none;
    for (std::size_t i = 0; i < recipe->targets.count; ++i)
      items[i] = {recipe->targets[i].symbol, inventory.total[i]};
    value.items = {items.data(), recipe->targets.count};
    value.hands_known = ubc::station_recipe_snapshot_detail::begin_hands(
                            memory, c.hero, result_hands) == ubc::StationHandReadIssue::none &&
                        ubc::station_recipe_snapshot_detail::verify_hands(memory, result_hands);
    if (value.hands_known)
      value.hands = result_hands.before;
    if (recipe->dndr_recipe && recipe->dndr_recipe->branched()) {
      const auto branch =
          rt::symbol_int(memory, s::game_parser, ubc::dndr::bloodfly, ubc::dndr::layout_profile);
      if (!branch.valid || branch.value != request.material_snapshot.branch_value)
        value.stock_known = false;
      if (branch.valid && branch.value != request.material_snapshot.branch_value)
        result_guard.fail(ubc::CraftingResultIssue::context);
    }
    if (recipe->grind)
      value.grind = read_grind_result_values(request);
    const auto fresh = context();
    if (fresh.world != c.world || fresh.hero != c.hero || fresh.station != c.station)
      value.stock_known = false;
  }
  if (recipe && recipe->tobacco && value.identity_known && value.station_known && !c.station) {
    ubc::GrindCheck check;
    if (!tobacco::lab_flags(memory, c.hero, false, check)) {
      log_tobacco_check(check, recipe->id);
      result_guard.fail(ubc::CraftingResultIssue::context);
    }
  }
  if ((!extra_job || (recipe && extra_result_complete(c, *recipe))) &&
      (!spirit_job || (recipe && spirit_result_complete(c, *recipe, value))))
    result_guard.observe(value);
  const auto elapsed = GetTickCount64() - result_started;
  if (elapsed > 30000)
    result_guard.timeout();
  if (result_guard.state() == ubc::CraftingResultState::verified) {
    handoff.acknowledge_result(true);
    for (std::size_t i = 0; i < value.items.count; ++i)
      event("stock_verified", value.items[i].symbol, static_cast<int>(value.items[i].total));
    event("amount_result_verified", static_cast<int>(request.id),
          static_cast<int>(request.quantity));
    ubc::CraftingResultSuccess success;
    if (!result_guard.success(success)) {
      result_guard.fail(ubc::CraftingResultIssue::contract);
      return;
    }
    if (ammo_job) {
      const auto* cb = recipe ? callback_for(*recipe, request.quantity) : nullptr;
      const auto* profile = cb ? ap::find(cb->function->symbol) : nullptr;
      std::int64_t delta = -1;
      if (profile)
        for (std::size_t i = 0; i < success.stock_count; ++i)
          if (success.deltas[i].symbol == static_cast<std::int32_t>(profile->output))
            delta = success.deltas[i].delta;
      ap::Context current;
      if (!profile || !read_ammo_context(nullptr, current) ||
          !ammo::observe_result({current, success.receipt, profile->callback, request.quantity,
                                 profile->output, delta, ap::ResultState::matched_ready, true})) {
        result_guard.fail(ubc::CraftingResultIssue::contract);
        return;
      }
    }
    pending_success = ubc::crafting_ui::success_notice(success);
    success_pending = true;
    remembered_world = success.key.world;
    remembered_hero = success.key.hero;
    remembered_station = success.key.station;
    remembered_recipe = success.id;

  } else if (result_guard.state() == ubc::CraftingResultState::failed) {
    if (grind_job)
      log_grind_check(
          ubc::diagnose_grind_result(result_guard.issue(), grind_prepared, value, elapsed),
          static_cast<int>(request.id));
    event("amount_result_failed", static_cast<int>(result_guard.issue()));
  }
}

} // namespace ubc_native
