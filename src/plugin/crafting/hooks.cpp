// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
bool bound_callback(const ubc::CraftingHandoffOffer& offer) noexcept {
  const auto* ticket = handoff.current_ticket();
  return ticket && offer.valid && offer.owner == ticket->offer.owner &&
         offer.function == ticket->offer.function;
}
bool callback_guarded(const Context& c, const ubc::CraftingHandoffOffer& offer) noexcept {
  if (!handoff.active())
    return false;
  if (bound_callback(offer))
    return true;
  // No further order goes to the engine after the effect. Only the
  // old callback remains protected against repetition. A normal end
  // path or a confirmed unrelated dialog must be able to continue.
  if (handoff.state() == ubc::CraftingHandoffState::result_pending)
    return !offer.valid;
  if (foreign_dialog(c) && offer.valid) {
    handoff.cancel(ubc::CraftingHandoffIssue::context);
    return false;
  }
  return true;
}
ubc::CraftingHandoffCallbackProof proof(std::uintptr_t caller, bool info) noexcept {
  return {own_thread(),
          caller == (info ? a::info_effect_return : a::choice_effect_return),
          native_update,
          update_depth,
          info ? info_depth : choice_depth,
          info ? choice_depth : info_depth,
          collection_depth,
          station_depth};
}
void __fastcall observe_update(void* manager, void* unused) {
  const auto error = GetLastError();
  const bool own = own_thread();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool previous_native = own ? native_update : false;
  if (own) {
    ++update_depth;
    native_update = update_depth == 1 &&
                    reinterpret_cast<std::uint32_t>(manager) == s::information_manager &&
                    (caller == 0x74c117 || caller == 0x75a727 || caller == 0x6c8a46);
    if (handoff.active() && !ui.focused())
      handoff.cancel(ubc::CraftingHandoffIssue::focus);
  }
  SetLastError(error);
  original_update(manager, unused);
  const auto after = GetLastError();
  if (own) {
    if (update_depth == 1 && recover_info) {
      recover_info = false;
      const auto c = context();
      const auto* ticket = handoff.current_ticket();
      if (native_update && c.valid && ticket && key(c) == handoff.request().key &&
          c.dialog.view == ticket->view && c.dialog.rejected_info_transition &&
          c.dialog.selected == ticket->offer.row && rows.generation == ticket->offer.collection) {
        // Only for a confirmed null resolve result. The original
        // InfoBegin creates a new collection without direct flag writes.
        reinterpret_cast<a::FastObject>(a::info_begin)(manager, nullptr);
        event("selection_reopened");
      } else
        event("selection_quarantined");
    }
    --update_depth;
    native_update = previous_native;
  }
  SetLastError(after);
}
void* __fastcall observe_resolve(void* manager, void*, void* player, void* npc, int row) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  SetLastError(error);
  void* result = original_resolve(manager, player, npc, row);
  const auto after = GetLastError();
  if (own_thread() && handoff.active() && caller == a::resolve_selection_return) {
    const auto info = ubc::read_selection_info(memory, reinterpret_cast<std::uint32_t>(result));
    const auto c = context();
    ubc::CraftingHandoffOffer actual{
        info.valid, basecat::Invocation::info, info.instance, info.function, row, rows.generation};
    ++resolved_read_depth;
    const auto current = observation(c, false);
    --resolved_read_depth;
    if (callback_guarded(c, actual)) {
      const bool different =
          current.context_known && current.view &&
          (!(current.key == handoff.request().key) || current.view != dispatched_ticket.view);
      if (different && actual.valid && !bound_callback(actual)) {
        handoff.cancel(ubc::CraftingHandoffIssue::context);
      } else if (!linked_step_info(info, handoff.step()) || !native_update || update_depth != 1 ||
                 info_depth || choice_depth || station_depth || collection_depth ||
                 reinterpret_cast<std::uint32_t>(player) != handoff.request().key.hero ||
                 reinterpret_cast<std::uint32_t>(npc) != handoff.request().key.hero ||
                 !handoff.matches_pending(dispatched_ticket, current, actual)) {
        result = nullptr;
        recover_info = true;
        handoff.cancel(ubc::CraftingHandoffIssue::offer);
        event("resolution_rejected");
      }
    }
  }
  SetLastError(after);
  return result;
}
void __fastcall observe_info(void* info, void*) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool own = own_thread();
  // The original INFO may change during the call. On normal
  // return, log only the identifiers read beforehand.
  const auto observed =
      campfire_observation_mode && own
          ? ubc::read_selection_info(memory, reinterpret_cast<std::uint32_t>(info))
          : ubc::SelectionInfo{};
  if (campfire_observation_mode && own) {
    const auto c = context();
    const auto production = rt::symbol_int(memory, s::game_parser, ubc::lhiver::production_symbol);
    if (caller != a::info_effect_return)
      event("campfire_caller", a::info_effect_return, static_cast<int>(caller));
    if (!c.ownership_known)
      event("campfire_owner", 1, 0);
    if (!name_is(c.station, 0x1b8, "CAMPFIRE"))
      event("campfire_scheme", 1, 0);
    if (!name_is(c.station, 0x1e0, "FRYINGMEAT"))
      event("campfire_on_state", 1, 0);
    if (!name_is(c.station, 0x1cc, "B_USECAMPFIRE"))
      event("campfire_condition", 1, 0);
    if (!name_is(c.station, 0x1a4, ""))
      event("campfire_use_empty", 1, 0);
    if (!production.valid || production.value != 14)
      event("campfire_production", 14, production.valid ? production.value : -1);
  }
  const auto prior_tobacco_owner_call = tobacco_owner_call;
  const auto previous_lease = own ? info_effect_lease : InfoEffectLease{};
  const bool previous_authorized = own ? effect_authorized : false;
  if (own) {
    info_effect_lease = {};
    info_lease_owner.invalidate();
  }
  if (own)
    ++info_depth;
  ubc::CraftingHandoffCallback witness;
  bool allow = true;
  bool final_effect = false;
  DWORD after = error;
  __try {
    if (own && handoff.active()) {
      const auto c = context();
      const auto actual_info =
          ubc::read_selection_info(memory, reinterpret_cast<std::uint32_t>(info));
      const ubc::CraftingHandoffOffer actual{actual_info.valid,    basecat::Invocation::info,
                                             actual_info.instance, actual_info.function,
                                             c.dialog.selected,    rows.generation};
      if (callback_guarded(c, actual)) {
        auto verified = proof(caller, true);
        verified.canonical_caller =
            verified.canonical_caller && linked_step_info(actual_info, handoff.step());
        witness =
            handoff.enter_callback(dispatched_ticket, observation(c, false), actual, verified);
        allow = witness.decision != ubc::CraftingHandoffDecision::block;
        final_effect = witness.decision == ubc::CraftingHandoffDecision::execute &&
                       handoff.step() && handoff.step()->is_effect();
        if (final_effect) {
          const auto* ticket = handoff.current_ticket();
          if (ticket && caller == a::info_effect_return && info_depth == 1 && !choice_depth) {
            info_effect_lease = {reinterpret_cast<std::uint32_t>(info),
                                 GetCurrentThreadId(),
                                 c.game,
                                 info_depth,
                                 handoff.request().key,
                                 *ticket};
            info_effect_lease.token = info_lease_owner.open({extra_context(c), frame,
                                                             reinterpret_cast<std::uint32_t>(info),
                                                             s::information_manager});
          }
        }
      }
    }
    const bool owned_tobacco = allow && witness.decision == ubc::CraftingHandoffDecision::execute &&
                               dispatched_ticket.offer.function == active_tobacco().info() &&
                               dispatched_ticket.offer.owner == active_tobacco().owner.instance;
    if (owned_tobacco) {
      tobacco_writer_entered = false;
      tobacco_owner_opened = false;
      tobacco_owner_call = reinterpret_cast<std::uint32_t>(info);
    }
    if (own)
      effect_authorized = final_effect;
    if (final_effect && !current_info_effect(context())) {
      allow = false;
      handoff.cancel(ubc::CraftingHandoffIssue::offer);
    }
    SetLastError(error);
    if (allow)
      original_info(info);
    after = GetLastError();
    if (owned_tobacco && !tobacco_owner_opened) {
      allow = false;
      handoff.cancel(ubc::CraftingHandoffIssue::resources);
      if (!tobacco_writer_entered && result_guard.permits_next() &&
          !bc::diagnostics().writes_disabled) {
        tobacco_clean_rejection = true;
        event("tobacco_order_unpaid", handoff.request().id, handoff.request().quantity);
      }
    }
    if (campfire_observation_mode && own && allow)
      event("campfire_info_returned", observed.instance, observed.function);
  } __finally {
    const bool abnormal = AbnormalTermination() != FALSE;
    if (abnormal)
      after = GetLastError();
    if (own) {
      tobacco_owner_call = prior_tobacco_owner_call;
      if (!abnormal || amount_diagnostic.cool_info_token)
        flush_amount_diagnostic(info_effect_lease.token);
      if (abnormal) {
        if (witness.decision == ubc::CraftingHandoffDecision::execute)
          handoff.cancel(ubc::CraftingHandoffIssue::offer);
        if (final_effect)
          result_guard.fail(ubc::CraftingResultIssue::contract);
      } else {
        if (witness.decision == ubc::CraftingHandoffDecision::execute) {
          if (allow && handoff.step() &&
              handoff.step()->function_symbol == active_tobacco().info() && !handoff.cancelled() &&
              dispatched_ticket.offer.function == active_tobacco().info() &&
              dispatched_ticket.offer.owner == active_tobacco().owner.instance) {
            event("tobacco_owner_returned", active_tobacco().info(), 1);
          }
          if (!tobacco_clean_rejection || !handoff.reject_unpaid_tobacco(witness))
            handoff.callback_return(witness);
          event(allow ? "native_callback_returned" : "effect_rejected",
                dispatched_ticket.offer.function);
          if (final_effect && !effect_executed)
            result_guard.fail(ubc::CraftingResultIssue::contract);
        } else if (!allow)
          event("effect_rejected");
      }
      info_lease_owner.close(info_effect_lease.token);
      info_effect_lease = previous_lease;
      effect_authorized = previous_authorized;
      --info_depth;
    }
    SetLastError(after);
  }
}
void __fastcall observe_choice(void* manager, void* choice) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
  const bool own = own_thread();
  const auto previous_grind_lease = grind_choice_lease;
  const int previous_tobacco_choice = tobacco_choice_call;
  const bool previous_writer_entered = tobacco_writer_entered;
  bool unpaid_effect = false;
  const bool previous_authorized = own ? effect_authorized : false;
  if (own) {
    ++choice_depth;
    grind_choice_lease = {};
    tobacco_choice_call = 0;
    tobacco_writer_entered = false;
  }
  ubc::CraftingHandoffCallback witness;
  bool allow = true;
  bool final_effect = false;
  bool removal_owned = false;
  DWORD after = error;
  __try {
    if (own && (handoff.active() || tobacco_owner.ticket)) {
      const auto c = context();
      const auto actual_choice = ubc::read_choice(memory, reinterpret_cast<std::uint32_t>(manager),
                                                  reinterpret_cast<std::uint32_t>(choice));
      const auto owner = ubc::read_selection_info(memory, c.dialog.info);
      const ubc::CraftingHandoffOffer actual{
          actual_choice.valid,    basecat::Invocation::choice, actual_choice.info.instance,
          actual_choice.function, actual_choice.index,         rows.generation};
      const bool unpaid = tobacco_owner.ticket != 0;
      const bool bound_page =
          unpaid && actual.valid && actual.owner == active_tobacco().owner.instance &&
          linked(owner) && tobacco_owner.owns(key(c), c.dialog.view, c.dialog.info) &&
          caller == a::choice_effect_return &&
          reinterpret_cast<std::uint32_t>(manager) == s::information_manager && choice_depth == 1;
      const bool back = bound_page && actual.function == active_tobacco().back;
      if (back) {
        handoff.cancel(ubc::CraftingHandoffIssue::cancelled);
        tobacco_choice_call = active_tobacco().back;
      } else if (callback_guarded(c, actual)) {
        auto verified = proof(caller, false);
        verified.canonical_caller =
            verified.canonical_caller && linked(owner) &&
            actual_choice.info.instance == owner.instance &&
            reinterpret_cast<std::uint32_t>(manager) == s::information_manager;
        witness =
            handoff.enter_callback(dispatched_ticket, observation(c, false), actual, verified);
        allow = witness.decision != ubc::CraftingHandoffDecision::block;
        final_effect = witness.decision == ubc::CraftingHandoffDecision::execute &&
                       handoff.step() && handoff.step()->is_effect();
        if (unpaid) {
          allow = allow && final_effect && bound_page &&
                  tobacco_owner.accepts(dispatched_ticket, actual.function);
          unpaid_effect = final_effect;
          if (allow) {
            tobacco_choice_call = actual.function;
            tobacco_writer_entered = false;
          }
        }
        if (allow && final_effect) {
          const auto current_grind_page =
              c.kind == cat::Station::grind ? grind_page(c) : shnav::PageEvidence{};
          removal_owned =
              post_remove_choice.begin(memory, c.dialog, rows.generation, dispatched_ticket.id,
                                       reinterpret_cast<std::uint32_t>(choice), actual.function);
          if (!removal_owned) {
            allow = false;
            handoff.cancel(ubc::CraftingHandoffIssue::offer);
          } else if (c.kind == cat::Station::grind) {
            if (!shnav::valid_page(current_grind_page) ||
                ubc::crafting_page(current_grind_page) != handoff.step()->before) {
              allow = false;
              handoff.cancel(ubc::CraftingHandoffIssue::offer);
            } else
              grind_choice_lease = {
                  c.dialog.info,     GetCurrentThreadId(), c.game, choice_depth, frame, key(c),
                  dispatched_ticket, current_grind_page};
          }
        }
      } else if (unpaid)
        allow = false;
    }
    if (own)
      effect_authorized = allow && final_effect;
    SetLastError(error);
    if (allow)
      original_choice(manager, choice);
    after = GetLastError();
  } __finally {
    const bool abnormal = AbnormalTermination() != FALSE;
    if (abnormal)
      after = GetLastError();
    if (removal_owned)
      post_remove_choice.clear();
    if (own) {
      if (abnormal) {
        if (witness.decision == ubc::CraftingHandoffDecision::execute)
          handoff.cancel(ubc::CraftingHandoffIssue::offer);
        if (final_effect)
          result_guard.fail(ubc::CraftingResultIssue::contract);
      } else {
        flush_amount_diagnostic();
        if (witness.decision == ubc::CraftingHandoffDecision::execute) {
          if (unpaid_effect && !tobacco_writer_entered && !effect_executed) {
            const auto& request = handoff.request();
            const auto* d = cat::find(request.id);
            if (d && d->tobacco)
              trace_tobacco_entry(context(), *d->tobacco, dispatched_ticket.offer.function,
                                  &request, "call.writer_entered", 1, 0);
          }
          if (unpaid_effect && !tobacco_writer_entered && !effect_executed &&
              result_guard.permits_next() && !bc::diagnostics().writes_disabled) {
            handoff.cancel(ubc::CraftingHandoffIssue::offer);
            tobacco_clean_rejection = true;
            event("tobacco_order_unpaid", handoff.request().id, handoff.request().quantity);
          }
          if (!tobacco_clean_rejection || !handoff.reject_unpaid_tobacco(witness))
            handoff.callback_return(witness);
          event(allow ? "native_callback_returned" : "effect_rejected",
                dispatched_ticket.offer.function);
          if (final_effect && !effect_executed && !tobacco_clean_rejection)
            result_guard.fail(ubc::CraftingResultIssue::contract);
        } else if (!allow)
          event("effect_rejected");
      }
      tobacco_writer_entered = previous_writer_entered;
      tobacco_choice_call = previous_tobacco_choice;
      grind_choice_lease = previous_grind_lease;
      effect_authorized = previous_authorized;
      --choice_depth;
    }
    SetLastError(after);
  }
}
void __fastcall observe_start(void* mob, void*, void* npc) {
  const auto error = GetLastError();
  const bool own = own_thread();
  if (own)
    ++station_depth;
  SetLastError(error);
#ifdef UBC_ENABLE_DNDR
  forge_start(mob, npc);
  SetLastError(error);
#endif
  original_start(mob, npc);
  const auto after = GetLastError();
  if (own)
    --station_depth;
  SetLastError(after);
}
ubc::StationEndPoint end_point_once(const ubc::CraftingKey& k, bool before) noexcept {
  ubc::StationEndPoint out;
  out.thread = GetCurrentThreadId();
  std::uint32_t game = 0;
  if (!common_bindings_verified())
    return out;
  if (!read(a::game_pointer, game) || !game ||
      integer(game, 0) != static_cast<std::int32_t>(a::game_vtable) ||
      !field(game, a::game_world, out.world) || !read(s::hero_pointer, out.hero) || !out.world ||
      !out.hero)
    return out;
  const auto hero = rt::read_symbol_address(memory, s::game_parser, hero_symbol());
  std::uint32_t bound = 0;
  if (!hero.valid || !field(hero.address, 0x1c, bound) || bound != out.hero)
    return out;
  out.identity_known = true;
  out.station_known = field(out.hero, s::npc_interact_mob, out.station);
  const auto production = rt::symbol_int(memory, s::game_parser, production_symbol());
  out.production_known = production.valid;
  out.production = production.value;
  out.hands_known = ubc::station_recipe_snapshot_detail::begin_hands(memory, out.hero, end_hands) ==
                        ubc::StationHandReadIssue::none &&
                    ubc::station_recipe_snapshot_detail::verify_hands(memory, end_hands);
  if (out.hands_known)
    out.hands = end_hands.before;
  if (before && out.station_known && out.station == k.station) {
    out.kind = k.kind;
    out.station_owned = station_owns(out.station, out.hero);
    out.station_profile_verified = matches_station_profile(out.station, k.kind);
    out.mob_fields_known =
        field(out.station, 0x1f4, out.state) && field(out.station, 0x1f8, out.state_num) &&
        field(out.station, 0x1fc, out.target) && field(out.station, 0x200, out.rewind);
  }
  return out;
}
bool same_end_point(const ubc::StationEndPoint& a, const ubc::StationEndPoint& b) noexcept {
  return a.identity_known && b.identity_known && a.station_known && b.station_known &&
         a.production_known && b.production_known && a.hands_known && b.hands_known &&
         a.thread == b.thread && a.world == b.world && a.hero == b.hero && a.station == b.station &&
         a.station_owned == b.station_owned &&
         a.station_profile_verified == b.station_profile_verified &&
         a.mob_fields_known == b.mob_fields_known && a.kind == b.kind && a.state == b.state &&
         a.target == b.target && a.state_num == b.state_num && a.rewind == b.rewind &&
         a.production == b.production && ubc::same_crafting_hands(a.hands, b.hands);
}
ubc::StationEndPoint end_point(const ubc::CraftingKey& k, bool before) noexcept {
  auto out = end_point_once(k, before);
  const auto second = end_point_once(k, before);
  out.read_stable = same_end_point(out, second);
  return out;
}
void __fastcall observe_end(void* mob, void*, void* npc, int flag) {
  const auto error = GetLastError();
  const auto caller = reinterpret_cast<std::uint32_t>(_ReturnAddress());
  const bool own = own_thread();
  if (own)
    ++station_depth;
  const auto& request = handoff.request();
  const bool candidate = own && result_guard.state() == ubc::CraftingResultState::waiting &&
                         reinterpret_cast<std::uint32_t>(mob) == request.key.station &&
                         reinterpret_cast<std::uint32_t>(npc) == request.key.hero && flag == 0 &&
                         caller == ubc::station_end_contract::completed_return;
  ubc::StationEndCall call;
  std::uint32_t table = 0;
  const ubc::station_end_contract::InstalledEndHook hook{
      InterlockedCompareExchange(&installed, 0, 0) != 0,
      reinterpret_cast<std::uint32_t>(&observe_end),
      reinterpret_cast<std::uint32_t>(hooks.original(8))};
  if (candidate) {
    call.mob = reinterpret_cast<std::uint32_t>(mob);
    call.npc = reinterpret_cast<std::uint32_t>(npc);
    call.return_address = caller;
    call.flag = flag;
    call.owner_thread_depth_verified = station_depth == 1 && !info_depth && !choice_depth &&
                                       !update_depth && !collection_depth && !amount_patch.active();
    call.before = end_point(request.key, true);
    call.original_path_verified =
        read(call.mob, table) &&
        ubc::station_end_contract::verify_path(
            memory, table, hook,
            end_compatibility(cool_job && result_guard.cool_prepared().valid()))
            .valid;
  }
  SetLastError(error);
  original_end(mob, npc, flag);
  const auto after = GetLastError();
#ifdef UBC_ENABLE_DNDR
  forge_end(mob, npc, flag, caller);
#endif
  if (own) {
    if (end_sequence != UINT64_MAX)
      ++end_sequence;
    else
      disabled = true;
    if (candidate) {
      call.original_returned = true;
      call.after = end_point(request.key, false);
      call.original_path_verified =
          call.original_path_verified &&
          ubc::station_end_contract::verify_path(
              memory, table, hook,
              end_compatibility(cool_job && result_guard.cool_prepared().valid()))
              .valid;
      const auto verified = ubc::verify_station_end(request.key, call);
      if (verified.normal_exit_verified) {
        if (extra_job && extra_executor && extra_active_job.id == request.key.generation &&
            extra_active_job.context.world == request.key.world &&
            extra_active_job.context.hero == request.key.hero &&
            extra_active_job.context.mob == request.key.station) {
          extra_end = {true, extra_active_job.id, extra_active_job.writer_token,
                       next_extra_serial(), extra_active_job.context};
          extra_normal_end = true;
          extra_end_frame = frame;
        }
        if (spirit_job && spirit_executor && spirit_active_job.id == request.key.generation &&
            spirit_active_job.context.world == request.key.world &&
            spirit_active_job.context.hero == request.key.hero &&
            spirit_active_job.context.mob == request.key.station) {
          spirit_end = {true, spirit_active_job.id, spirit_active_job.writer_token,
                        next_extra_serial(), spirit_active_job.context};
          spirit_normal_end = true;
          spirit_end_frame = frame;
        }
        result_guard.end_after({true, true, request.key.generation, end_sequence, frame,
                                request.key.world, request.key.hero, request.key.station});
        event("normal_end_verified");
      } else {
        if (grind_job)
          log_grind_check(ubc::diagnose_grind_end(verified.issue, request.key, call),
                          static_cast<int>(request.id));
        event("normal_end_rejected", static_cast<int>(verified.issue));
      }
    }
    --station_depth;
  }
  SetLastError(after);
}
} // namespace ubc_native
