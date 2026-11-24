// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
const basecat::Callback* callback_for(const cat::Descriptor& recipe,
                                      std::uint32_t quantity) noexcept {
  if (recipe.tobacco && quantity && quantity <= INT32_MAX)
    return &recipe.tobacco->callback;
  if (recipe.base)
    return basecat::callback_for_quantity(*recipe.base, quantity);
  if (recipe.station_descriptor)
    return scat::callback_for_quantity(*recipe.station_descriptor, quantity);
  if (recipe.alchemy_extra) {
    const auto p = xcat::plan_quantity(*recipe.alchemy_extra, quantity);
    return p.valid ? p.callback : nullptr;
  }
  if (recipe.anvil) {
    const auto p = acat::plan_quantity(*recipe.anvil, quantity);
    return p.valid ? p.callback : nullptr;
  }
  if (recipe.ore) {
    const auto p = ocat::plan_quantity(*recipe.ore, quantity);
    return p.valid ? p.callback : nullptr;
  }
  if (recipe.bow) {
    const auto p = bcat::plan_quantity(*recipe.bow, quantity);
    return p.valid ? p.callback : nullptr;
  }
  return nullptr;
}
const basecat::Callback* amount_callback(std::int32_t symbol) noexcept {
  for (const auto& recipe : basecat::recipes)
    for (const auto& cb : recipe.callbacks)
      if (cb.minimum_quantity && cb.function->symbol == static_cast<std::uint32_t>(symbol))
        return &cb;
  for (const auto& recipe : scat::recipes)
    if (recipe.callback.function->symbol == static_cast<std::uint32_t>(symbol))
      return &recipe.callback;
  for (const auto& recipe : tobacco::recipes)
    if (recipe.callback.function->symbol == static_cast<std::uint32_t>(symbol))
      return &recipe.callback;
  for (const auto& recipe : xcat::recipes)
    if (recipe.callback.function->symbol == static_cast<std::uint32_t>(symbol))
      return &recipe.callback;
  for (const auto& recipe : acat::recipes)
    if (recipe.callback.function->symbol == static_cast<std::uint32_t>(symbol))
      return &recipe.callback;
  for (const auto& recipe : ocat::recipes)
    if (recipe.callback.function->symbol == static_cast<std::uint32_t>(symbol))
      return &recipe.callback;
  for (const auto& recipe : bcat::recipes)
    if (recipe.callback.function->symbol == static_cast<std::uint32_t>(symbol))
      return &recipe.callback;
  return nullptr;
}

bool __cdecl batch_signature(const bc::Call& call) noexcept {
  if (dndr_active) {
    const auto* recipe = ubc::dndr::callback(static_cast<unsigned>(call.symbol));
    if (!recipe || reinterpret_cast<std::uintptr_t>(call.parser) != s::game_parser)
      return false;
    rt::detail::Layout layout{};
    return rt::detail::read_layout(memory, s::game_parser, ubc::dndr::profile, layout) ==
               rt::Issue::none &&
           rt::detail::check_function(memory, layout, *recipe->effect->information) ==
               rt::Issue::none;
  }
  const auto* entry = amount_callback(call.symbol);
  const ubc::lhiver::Function* registered = call.symbol == 20401 ? tobacco::owner.information
                                            : entry              ? entry->function
                                                                 : nullptr;
  for (const auto& recipe : acat::recipes)
    if (recipe.original_three.function->symbol == static_cast<std::uint32_t>(call.symbol))
      registered = recipe.original_three.function;
  for (const auto& recipe : gcat::recipes)
    if (recipe.callback->symbol == static_cast<std::uint32_t>(call.symbol))
      registered = recipe.callback;
  for (const auto* f : {ccat::single.information, ccat::triple.information})
    if (f->symbol == static_cast<std::uint32_t>(call.symbol))
      registered = f;
  for (const auto& recipe : shcat::recipes)
    if (recipe.function.symbol == static_cast<std::uint32_t>(call.symbol))
      registered = &recipe.function;
  if (!registered || reinterpret_cast<std::uintptr_t>(call.parser) != s::game_parser)
    return false;
  const auto& function = *registered;
  rt::detail::Layout layout{};
  rt::detail::Symbol symbol{};
  return rt::detail::read_layout(memory, s::game_parser, layout_profile, layout) ==
             rt::Issue::none &&
         rt::detail::lookup(memory, layout, function.symbol, symbol) == rt::Issue::none &&
         rt::detail::name_matches(memory, symbol, function.name) == rt::Issue::none &&
         symbol.type == 5 && symbol.flags == 1 && symbol.elements == 0 && symbol.offset == 0 &&
         rt::detail::parent_matches(memory, layout, symbol, -1) == rt::Issue::none &&
         rt::detail::stable_layout(memory, s::game_parser, layout_profile, layout) ==
             rt::Issue::none;
}
bool __cdecl reject_unexpected_cool_info(const bc::Call& call) noexcept {
  const bool protected_effect =
      (call.symbol == 20401 || (call.symbol >= 20403 && call.symbol <= 20405)) &&
      InterlockedCompareExchange(&tobacco_unpaid_page, 0, 0);
  if (protected_effect && (!own_thread() || call.caller != bc::choice_call_return))
    return true;
  if (own_thread()) {
    const bool wrong_owner =
        tobacco_owner_call && call.caller == bc::info_call_return && call.symbol != 20401;
    const bool wrong_choice = tobacco_owner.ticket && call.caller == bc::choice_call_return &&
                              (!tobacco_choice_call || call.symbol != tobacco_choice_call);
    if (wrong_owner || wrong_choice) {
      char fields[128]{};
      std::snprintf(fields, sizeof(fields),
                    ",\"field\":\"call.bound_symbol\",\"soll\":\"%d\",\"ist\":\"%d\"",
                    wrong_owner ? 20401 : tobacco_choice_call, call.symbol);
      event("tobacco_check_value", wrong_owner ? 20401 : tobacco_owner.function, 0, fields);
      return true;
    }
  }
  if (!own_thread() || call.caller != bc::info_call_return || !effect_authorized ||
      !native_update || update_depth != 1 || info_depth != 1 || choice_depth)
    return false;
  const auto* ticket = handoff.current_ticket();
  if (!ticket || handoff.request().key.kind != cat::Station::cool ||
      handoff.request().quantity != cpre::quantity || ticket->offer.owner != cpre::owner ||
      ticket->offer.function != cpre::callback || call.symbol == ticket->offer.function)
    return false;
  handoff.cancel(ubc::CraftingHandoffIssue::offer);
  return true;
}
bc::Decision __cdecl select_batch_call(const bc::Call& call) noexcept {
  if (!own_thread())
    return bc::Decision::ordinary;
  if (!tobacco_owner.ticket && call.caller == bc::choice_call_return && choice_depth == 1 &&
      call.symbol >= 20403 && call.symbol <= 20405)
    trace_tobacco_entry(context(), tobacco::recipes[call.symbol - 20403], call.symbol, nullptr,
                        "call.original", 1, 1);
  if (tobacco_owner_call) {
    return call.symbol == 20401 && call.caller == bc::info_call_return && !handoff.cancelled()
               ? bc::Decision::writer
               : bc::Decision::reject;
  }
  const auto* ticket = handoff.current_ticket();
  if (tobacco_owner.ticket && call.caller == bc::choice_call_return && call.symbol != 20402 &&
      (!ticket || !tobacco_owner.accepts(*ticket, call.symbol) ||
       tobacco_choice_call != call.symbol))
    return bc::Decision::reject;
  if (!handoff.active() || !ticket || call.symbol != ticket->offer.function)
    return bc::Decision::ordinary;
  const auto expected = ticket->offer.mode == basecat::Invocation::info ? bc::info_call_return
                                                                        : bc::choice_call_return;
  if (!effect_authorized || effect_executed || handoff.cancelled() || call.caller != expected ||
      handoff.state() != ubc::CraftingHandoffState::executing)
    return bc::Decision::reject;
  return bc::Decision::writer;
}
bool late_gate(ubc::GrindCheck* diagnostic, tobacco::Checks* trace) noexcept {
  const auto matches = [diagnostic, trace](const char* name, std::int64_t expected,
                                           std::int64_t actual) {
    if (trace)
      return trace->equal(name, expected, actual);
    return diagnostic ? diagnostic->equal(name, expected, actual) : expected == actual;
  };
  const auto* ticket = handoff.current_ticket();
  const auto* step = handoff.step();
  if (!matches("gate.ticket", true, ticket != nullptr) ||
      !matches("gate.step", true, step != nullptr) ||
      !matches("gate.effect_step", true, step && step->is_effect()) ||
      !matches("gate.authorized", true, effect_authorized) ||
      !matches("gate.executed", false, effect_executed) ||
      !matches("gate.cancelled", false, handoff.cancelled()) ||
      !matches("gate.handoff_state", static_cast<int>(ubc::CraftingHandoffState::executing),
               static_cast<int>(handoff.state())) ||
      !matches("gate.native_update", true, native_update) ||
      !matches("gate.owner_thread", true, own_thread()) ||
      !matches("gate.update_depth", 1, update_depth) ||
      !matches("gate.station_depth", 0, station_depth) ||
      !matches("gate.collection_depth", 0, collection_depth) ||
      !matches("gate.callback_depth", 1, info_depth + choice_depth) ||
      !matches("gate.focus", true, ui.focused()))
    return false;
  const auto c = context();
  const auto current = observation(c, false);
  if (dndr_active) {
    const auto actor = rt::read_symbol_address(memory, s::game_parser, ubc::dndr::self);
    std::uint32_t bound = 0;
    if (!actor.valid || !field(actor.address, 0x1c, bound) || bound != c.hero ||
        !current_info_effect(c) ||
        !ubc::bow_item_code::verify_externals(memory, s::game_parser).verified ||
        !rt::verify_local_contract(memory, s::game_parser,
                                   c.kind == cat::Station::anvil ? ubc::dndr::anvil::profile
                                                                 : ubc::dndr::profile)
             .matches)
      return false;
  }
  if (c.kind == cat::Station::saw) {
    const auto* recipe = cat::find(handoff.request().id);
    if (!c.valid || !recipe || recipe->route != cat::Route::station ||
        recipe->station != cat::Station::saw || !recipe->station_descriptor ||
        recipe->station_descriptor->material_actor != basecat::Actor::self)
      return false;
    // The original OnChoice binds SELF from Manager.NPC at 0x662AA5 before
    // CallFunc reaches our writer at 0x662B6A. The physical preview actor
    // cannot authorize this call. The freshly read script actor must match here.
    const auto actor = ubc::station_recipe_runtime::actor(memory, s::game_parser,
                                                          *recipe->station_descriptor, c.hero);
    if (!actor.read_valid || !actor.requires_self || !actor.matches)
      return false;
  }
  if (c.kind == cat::Station::bow) {
    const auto* recipe = cat::find(handoff.request().id);
    if (!c.valid || !recipe || !recipe->bow || !current_info_effect(c))
      return false;
    const auto profile = brt::assemble(*recipe->bow);
    const auto actor = brt::actor(memory, s::game_parser, profile, c.hero);
    const auto state = brt::context(memory, s::game_parser, profile);
    const auto known = brt::knowledge(memory, s::game_parser, profile);
    if (!actor.read_valid || !actor.requires_self || !actor.matches || !state.owner_matches() ||
        !known.read_valid || !known.known)
      return false;
  }
  if (c.kind == cat::Station::cool) {
    const auto* recipe = cat::find(handoff.request().id);
    const auto plan =
        ubc::crafting_cool_plan(handoff.request().material_snapshot, handoff.request().quantity);
    if (!c.valid || !recipe || !recipe->cool || !current_info_effect(c) || !plan.valid ||
        !cool_runtime_path(plan.requires_extra_debit).valid ||
        !end_path(c.station, plan.requires_extra_debit).valid)
      return false;
    const auto actor = crt::actor(memory, s::game_parser, crt::assemble(ccat::recipe), c.hero);
    if (!actor.read_valid || !actor.requires_self || !actor.matches)
      return false;
  }
  if (c.kind == cat::Station::grind) {
    if (!ubc::grind_recipe_gate(
            handoff.request().id, c.valid, matches, [&] { return current_grind_choice(c); },
            [&] { return cool_runtime_path().valid; },
            [&](const auto& descriptor) {
              return shrt::actor(memory, s::game_parser, shrt::assemble(descriptor), c.hero);
            }) &&
        !(trace && trace->collecting()))
      return false;
  }
  if (!dndr_active && c.kind == cat::Station::anvil) {
    const auto* recipe = cat::find(handoff.request().id);
    if (!c.valid || !recipe || recipe->station != cat::Station::anvil)
      return false;
    // The physical preview is not a script actor. Only at the inner
    // CallFunc, after the original SELF binding, may the SELF minimum check
    // and the HERO material effect be admitted together for this order.
    if (recipe->route == cat::Route::anvil && recipe->anvil) {
      const auto actor = art::actor(memory, s::game_parser, art::assemble(*recipe->anvil), c.hero);
      if (!actor.read_valid || !actor.requires_self || !actor.matches)
        return false;
    } else if (recipe->route == cat::Route::ore && recipe->ore) {
      const auto actor = ort::actor(memory, s::game_parser, ort::assemble(*recipe->ore), c.hero);
      if (!actor.read_valid || !actor.requires_self || !actor.matches)
        return false;
    } else
      return false;
  }
  if ((!ticket || !step) && !trace)
    return false;
  const ubc::CraftingHandoffTicket missing_ticket{};
  const ubc::CraftingStep missing_step{};
  if (!ticket)
    ticket = &missing_ticket;
  if (!step)
    step = &missing_step;
  return matches("gate.final_context", true, c.valid) &&
         matches("gate.context_known", true, current.context_known) &&
         matches("gate.key", true, current.key == handoff.request().key) &&
         matches("gate.view", ticket->view, current.view) &&
         matches("gate.collection", true, current.collection == ticket->offer.collection) &&
         matches("gate.mode", static_cast<int>(ticket->offer.mode),
                 static_cast<int>(current.mode)) &&
         matches("gate.page_known", true, current.page_known) &&
         matches("gate.page_kind", static_cast<int>(step->before.kind),
                 static_cast<int>(current.page.kind)) &&
         matches("gate.page", step->before.value, current.page.value) &&
         matches("gate.resources", true,
                 ubc::crafting_handoff_detail::same_resources(
                     current.materials, handoff.request().material_snapshot)) &&
         matches("gate.hands", true, ubc::same_crafting_hands(current.hands, handed_hands)) &&
         matches("gate.stable", true, same_context(c, context())) &&
         (ticket->offer.mode == basecat::Invocation::info
              ? matches("gate.info_complete", true, current.complete)
              : matches("gate.post_remove_choice", true,
                        post_remove_choice.consume(memory, c.dialog, rows.generation, ticket->id,
                                                   ticket->offer.function))) &&
         (!trace || trace->ok());
}
bool __cdecl extra_boundary(void*, xwriter::Phase phase, const xdebit::Job& job,
                            xwriter::Boundary& out) {
  out = {};
  const auto bridge = bc::diagnostics();
  if (!own_thread() || !extra_executor || !extra_writer_scope || !extra_job || !bridge.writer ||
      bridge.readers || bridge.writes_disabled || !native_update || update_depth != 1 ||
      info_depth != 1 || choice_depth || station_depth || collection_depth ||
      extra_writer_scope->call.thread != GetCurrentThreadId() ||
      extra_writer_scope->call.symbol != static_cast<int>(job.callback) ||
      extra_writer_scope->call.caller != bc::info_call_return || job.id != extra_active_job.id ||
      job.writer_token != extra_active_job.writer_token ||
      job.quantity != extra_active_job.quantity || job.callback != extra_active_job.callback ||
      job.owner != extra_active_job.owner || !xdebit::same(job.context, extra_active_job.context))
    return false;
  const auto scope = extra_executor->scope_evidence();
  if (!scope.valid || !scope.scope.writer_active || scope.scope.writer_depth != 1 ||
      scope.scope.vm_depth)
    return false;
  const auto c = context();
  if (!c.identity_known || !c.station_known || !xdebit::same(extra_context(c), job.context) ||
      !matches_station_profile(c.station, cat::Station::alchemy) || !end_path(c.station).valid)
    return false;
  out.runtime.context = extra_context(c);
  out.runtime.scope = scope.scope;
  out.runtime.serial = next_extra_serial();
  out.runtime.current_thread = GetCurrentThreadId();
  out.runtime.synchronous_writer_verified = true;
  out.runtime.physical_lab_hand_contract_verified = true;
  out.actual_info = current_info_effect(c);
  if (!out.actual_info)
    return false;
  out.modules = extra_modules();
  if (phase == xwriter::Phase::before_original)
    out.native_handoff_verified = late_gate();
  return out.runtime.serial && xdebit::same(extra_context(context()), job.context);
}
bool __cdecl spirit_boundary(void*, gwriter::Phase phase, const gdebit::Job& job,
                             gwriter::Boundary& out) {
  out = {};
  const auto bridge = bc::diagnostics();
  if (!own_thread() || !spirit_executor || !spirit_writer_scope || !spirit_job || !bridge.writer ||
      bridge.readers || bridge.writes_disabled || !native_update || update_depth != 1 ||
      info_depth != 1 || choice_depth || station_depth || collection_depth ||
      spirit_writer_scope->call.thread != GetCurrentThreadId() ||
      spirit_writer_scope->call.symbol != static_cast<int>(job.callback) ||
      spirit_writer_scope->call.caller != bc::info_call_return || job.id != spirit_active_job.id ||
      job.writer_token != spirit_active_job.writer_token ||
      job.quantity != spirit_active_job.quantity ||
      job.branch_value != spirit_active_job.branch_value ||
      job.callback != spirit_active_job.callback || job.owner != spirit_active_job.owner ||
      !gdebit::same(job.context, spirit_active_job.context))
    return false;
  const auto scope = spirit_executor->scope_evidence();
  if (!scope.valid || !scope.scope.writer_active || scope.scope.writer_depth != 1 ||
      scope.scope.vm_depth)
    return false;
  const auto c = context();
  if (!c.identity_known || !c.station_known || !gdebit::same(extra_context(c), job.context) ||
      !matches_station_profile(c.station, cat::Station::alchemy) || !end_path(c.station).valid)
    return false;
  out.runtime.context = extra_context(c);
  out.runtime.scope = scope.scope;
  out.runtime.serial = next_extra_serial();
  out.runtime.current_thread = GetCurrentThreadId();
  out.runtime.synchronous_writer_verified = true;
  out.runtime.physical_lab_hand_contract_verified = true;
  out.actual_info = current_info_effect(c);
  if (!out.actual_info)
    return false;
  out.modules = extra_modules();
  if (phase == gwriter::Phase::before_original)
    out.native_handoff_verified = late_gate();
  return out.runtime.serial && gdebit::same(extra_context(context()), job.context);
}
bool amount_identity(void* raw, std::uint32_t address, std::size_t length) noexcept {
  const auto& ctx = *static_cast<NativeAmountContext*>(raw);
  if (!own_thread() || !ctx.function || length != ctx.function->byte_length ||
      address != ctx.layout.code + ctx.function->byte_offset)
    return false;
  rt::detail::Layout fresh{};
  rt::detail::Symbol symbol{};
  return rt::detail::read_layout(memory, s::game_parser, layout_profile, fresh) ==
             rt::Issue::none &&
         rt::detail::same_layout(ctx.layout, fresh) &&
         rt::detail::lookup(memory, fresh, ctx.function->symbol, symbol) == rt::Issue::none &&
         symbol.data == ctx.function->byte_offset && symbol.type == 5 && symbol.flags == 1 &&
         symbol.elements == 0 && symbol.offset == 0 &&
         rt::detail::name_matches(memory, symbol, ctx.function->name) == rt::Issue::none;
}
bool amount_writable(void*, std::uint32_t address, std::size_t size) noexcept {
  if (!address || !size || size > UINT32_MAX - address)
    return false;
  std::uintptr_t cursor = address;
  const auto end = cursor + size;
  while (cursor < end) {
    MEMORY_BASIC_INFORMATION page_info{};
    if (VirtualQuery(reinterpret_cast<void*>(cursor), &page_info, sizeof(page_info)) !=
            sizeof(page_info) ||
        page_info.State != MEM_COMMIT || page_info.Type != MEM_PRIVATE ||
        (page_info.Protect & PAGE_GUARD) || (page_info.Protect & 0xffu) != PAGE_READWRITE)
      return false;
    const auto base = reinterpret_cast<std::uintptr_t>(page_info.BaseAddress);
    if (base > cursor || page_info.RegionSize > UINT32_MAX - base ||
        base + page_info.RegionSize <= cursor)
      return false;
    cursor = base + page_info.RegionSize;
  }
  return true;
}
bool amount_replace(void* raw, std::uint32_t address, const void* before, const void* after,
                    std::size_t size) noexcept {
  auto* ctx = static_cast<NativeAmountContext*>(raw);
  if (!ctx->function ||
      !amount_identity(raw, ctx->layout.code + ctx->function->byte_offset,
                       ctx->function->byte_length) ||
      !amount_writable(raw, address, size))
    return false;
  __try {
    auto* target = reinterpret_cast<void*>(address);
    if (std::memcmp(target, before, size))
      return false;
    if (ctx->predebit && !cwriter::permits_operand_replace(
                             result_guard.cool_prepared(), info_lease_owner, cool_writer_scope,
                             cool_predebit_journal, cool_fence_token, address, before, after, size,
                             []() { return current_info_effect(context()); }))
      return false;
    std::memcpy(target, after, size);
    return std::memcmp(target, after, size) == 0;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}
void* __cdecl execute_original_amount(void* raw) {
  const auto incoming_error = GetLastError();
  auto& scope = *static_cast<bc::WriterScope*>(raw);
  const auto& request = handoff.request();
  const auto* recipe = cat::find(request.id);
  const auto* ticket = handoff.current_ticket();
  const ubc::CraftingResultBinding binding{request.key.generation,
                                           frame,
                                           ticket ? ticket->offer.collection : 0,
                                           end_sequence,
                                           request.key.world,
                                           request.key.hero,
                                           request.key.station};
  if (!recipe || !result_guard.begin(request, binding)) {
    handoff.cancel(ubc::CraftingHandoffIssue::resources);
    SetLastError(incoming_error);
    return nullptr;
  }
  if (ammo_job) {
    active_ammo_scope = {request.key.generation,
                         request.key.generation,
                         static_cast<std::uint32_t>(scope.call.symbol),
                         1,
                         true,
                         amount_patch.active()};
    if (!read_ammo_context(nullptr, active_ammo_context) ||
        !ammo::enter_effect(active_ammo_context, active_ammo_scope)) {
      active_ammo_scope = {};
      result_guard.fail(ubc::CraftingResultIssue::contract);
      SetLastError(incoming_error);
      return nullptr;
    }
  }
  void* value = nullptr;
  effect_executed = true;
  DWORD original_error = incoming_error;
  SetLastError(incoming_error);
  __try {
    value = scope.original(scope.call.parser, scope.call.symbol);
    original_error = GetLastError();
    if (grind_job)
      grind_original_returned = true;
  } __finally {
    if (AbnormalTermination())
      original_error = GetLastError();
    if (ammo_job) {
      ap::Context current;
      if (!read_ammo_context(nullptr, current)) {
        note_ammo_diagnostic(AmmoDiagnosticPhase::close_context);
        result_guard.fail(ubc::CraftingResultIssue::contract);
      } else if (!ammo::close_effect(current, active_ammo_scope)) {
        note_ammo_diagnostic(AmmoDiagnosticPhase::close_effect);
        result_guard.fail(ubc::CraftingResultIssue::contract);
      }
      active_ammo_scope = {};
    }
    SetLastError(original_error);
  }
  return value;
}
void* execute_extra_amount(bc::WriterScope& scope, const ubc::AmountMemory& writable,
                           const cat::Descriptor& recipe) {
  const auto incoming_error = GetLastError();
  const auto& request = handoff.request();
  if (!recipe.alchemy_extra || !extra_catalogue_reader || !extra_executor) {
    SetLastError(incoming_error);
    return nullptr;
  }
  const auto c = context();
  const auto actual_info = current_info_effect(c);
  if (!actual_info) {
    event("extra_info_rejected", 1);
    SetLastError(incoming_error);
    return nullptr;
  }
  const auto input = extra_read_input(c, native_update && update_depth == 1 && info_depth == 1 &&
                                             !choice_depth && !station_depth && !collection_depth);
  const auto fresh = extra_catalogue_reader->capture_recipe(memory, input, recipe.alchemy_extra->id,
                                                            request.quantity);
  if (!fresh.effect_ready || fresh.quantity != request.quantity ||
      !xdebit::same(fresh.context, extra_context(c)) || !same_context(c, context())) {
    event("extra_snapshot_rejected", static_cast<int>(fresh.issue),
          static_cast<int>(request.quantity));
    SetLastError(incoming_error);
    return nullptr;
  }
  const auto* ticket = handoff.current_ticket();
  const ubc::CraftingResultBinding binding{request.key.generation,
                                           frame,
                                           ticket ? ticket->offer.collection : 0,
                                           end_sequence,
                                           request.key.world,
                                           request.key.hero,
                                           request.key.station};
  if (!result_guard.begin(request, binding)) {
    SetLastError(incoming_error);
    return nullptr;
  }
  extra_active_job = {fresh.context,
                      request.key.generation,
                      request.key.generation,
                      recipe.alchemy_extra->callback.function->symbol,
                      static_cast<std::uint32_t>(recipe.alchemy_extra->owner.instance),
                      request.quantity};
  extra_end = {};
  extra_normal_end = false;
  extra_end_frame = 0;
  extra_writer_scope = &scope;
  const xwriter::Request run{fresh, extra_active_job, actual_info};
  void* returned = nullptr;
  DWORD error = incoming_error;
  SetLastError(incoming_error);
  __try {
    returned = extra_executor->run(scope, amount_patch, writable, run);
    error = GetLastError();
  } __finally {
    if (AbnormalTermination())
      error = GetLastError();
    effect_executed = extra_executor->outcome().original_entered;
    extra_writer_scope = nullptr;
    if (AbnormalTermination())
      result_guard.fail(ubc::CraftingResultIssue::restore);
    SetLastError(error);
  }
  const auto& outcome = extra_executor->outcome();
  const auto expected = request.quantity == 1 ? xwriter::RestoreProof::not_required_for_one
                                              : xwriter::RestoreProof::verified;
  const bool restored = outcome.original_entered && outcome.original_returned &&
                        outcome.operands_restored && outcome.closing && !outcome.blocked &&
                        scope.restore_verified && outcome.vm_restore == expected &&
                        outcome.item_restore == expected;
  result_guard.restored(restored, {request.key.generation, request.id, request.quantity,
                                   outcome.closing && !outcome.blocked && outcome.original_returned,
                                   outcome.item_restore == expected});
  if (!restored) {
    event("extra_writer_rejected", static_cast<int>(outcome.dependency_issue),
          static_cast<int>(outcome.read_issue));
    const auto& failure = outcome.parser_failure;
    if (failure.captured) {
      const auto& diagnostic = failure.snapshot;
      event("extra_parser_failure", static_cast<int>(failure.phase),
            diagnostic.value.data_depth <= INT32_MAX ? static_cast<int>(diagnostic.value.data_depth)
                                                     : -1);
      event("extra_parser_flags",
            static_cast<int>(
                ubc::alchemy_flask_runtime::parser_failure_flags(diagnostic, run.job.context)),
            static_cast<int>(diagnostic.word_count));
      if (diagnostic.words_stable)
        for (std::uint32_t i = 0; i < diagnostic.word_count && i < diagnostic.words.size(); ++i)
          event("extra_parser_word", static_cast<int>(i),
                ubc::alchemy_flask_runtime::parser_failure_log_word(diagnostic.words[i]));
    }
  }
  SetLastError(error);
  return returned;
}
void* execute_spirit_amount(bc::WriterScope& scope, const ubc::AmountMemory& writable,
                            const cat::Descriptor& recipe) {
  const auto incoming_error = GetLastError();
  const auto& request = handoff.request();
  if (!recipe.spirit || !spirit_catalogue_reader || !spirit_executor) {
    SetLastError(incoming_error);
    return nullptr;
  }
  const auto c = context();
  const auto actual_info = current_info_effect(c);
  if (!actual_info) {
    event("spirit_info_rejected", 1);
    SetLastError(incoming_error);
    return nullptr;
  }
  const auto input = extra_read_input(c, native_update && update_depth == 1 && info_depth == 1 &&
                                             !choice_depth && !station_depth && !collection_depth);
  const auto fresh =
      spirit_catalogue_reader->capture(memory, {input, nullptr, read_spirit_actual_info});
  const auto quantity = ubc::spirit_crafting_quantity(fresh, request.quantity);
  const auto projected = ubc::project_capture(fresh);
  if (!quantity.effect_ready || fresh.actual_info != actual_info ||
      !(projected.key == request.key) || !ubc::valid_crafting_snapshot(projected) ||
      !ubc::crafting_handoff_detail::same_resources(projected.recipes[cat::spirit_begin],
                                                    request.material_snapshot) ||
      !ubc::same_crafting_hands(projected.hands, handed_hands) || !same_context(c, context())) {
    event("spirit_snapshot_rejected", static_cast<int>(fresh.diagnostic.issue),
          static_cast<int>(request.quantity));
    SetLastError(incoming_error);
    return nullptr;
  }
  const auto* ticket = handoff.current_ticket();
  const ubc::CraftingResultBinding binding{request.key.generation,
                                           frame,
                                           ticket ? ticket->offer.collection : 0,
                                           end_sequence,
                                           request.key.world,
                                           request.key.hero,
                                           request.key.station};
  if (!result_guard.begin(request, binding)) {
    SetLastError(incoming_error);
    return nullptr;
  }
  spirit_active_job = {{extra_context(c), request.key.generation, request.key.generation,
                        recipe.spirit->callback->symbol,
                        static_cast<std::uint32_t>(recipe.spirit->owner.instance),
                        request.quantity},
                       request.material_snapshot.branch_value};
  spirit_end = {};
  spirit_normal_end = false;
  spirit_end_frame = 0;
  spirit_writer_scope = &scope;
  const gwriter::Request run{fresh.recipe, fresh.serial, request.quantity, spirit_active_job,
                             actual_info};
  void* returned = nullptr;
  DWORD error = incoming_error;
  SetLastError(incoming_error);
  __try {
    returned = spirit_executor->run(scope, amount_patch, writable, run);
    error = GetLastError();
  } __finally {
    if (AbnormalTermination())
      error = GetLastError();
    effect_executed = spirit_executor->outcome().original_entered;
    spirit_writer_scope = nullptr;
    if (AbnormalTermination())
      result_guard.fail(ubc::CraftingResultIssue::restore);
    SetLastError(error);
  }
  const auto& outcome = spirit_executor->outcome();
  const auto expected = request.quantity == 1 ? gwriter::RestoreProof::not_required_for_one
                                              : gwriter::RestoreProof::verified;
  const bool restored = outcome.original_entered && outcome.original_returned &&
                        outcome.operands_restored && outcome.closing && !outcome.blocked &&
                        scope.restore_verified && outcome.vm_restore == expected &&
                        outcome.item_restore == expected;
  result_guard.restored(restored, {request.key.generation, request.id, request.quantity,
                                   outcome.closing && !outcome.blocked && outcome.original_returned,
                                   outcome.item_restore == expected});
  if (!restored)
    event("spirit_writer_rejected", static_cast<int>(outcome.dependency_issue),
          static_cast<int>(outcome.read_issue));
  SetLastError(error);
  return returned;
}
bool verify_bow_amount(const bc::WriterScope& scope, const cat::Descriptor& recipe) noexcept {
  const auto& request = handoff.request();
  const auto c = context();
  const auto bridge = bc::diagnostics();
  if (!recipe.bow || recipe.route != cat::Route::bow || !bow_modules_current() || !c.valid ||
      c.kind != cat::Station::bow || !(key(c) == request.key) || !bridge.writer || bridge.readers ||
      bridge.writes_disabled || !scope.original || scope.call.thread != GetCurrentThreadId() ||
      scope.call.caller != bc::info_call_return ||
      scope.call.symbol != static_cast<int>(recipe.bow->callback.function->symbol) ||
      !current_info_effect(c) || !end_path(c.station).valid || !native_update ||
      update_depth != 1 || info_depth != 1 || choice_depth || station_depth || collection_depth ||
      resolved_read_depth || amount_patch.active())
    return false;
  const auto proof =
      bow_item_reader->capture(memory, *recipe.bow, request.quantity, bow_read_input(request.key));
  if (!proof.item_path_verified) {
    log_bow_check("writer.item_path", 0, static_cast<int>(proof.issue),
                  static_cast<int>(request.id), request.quantity, proof.failed_address);
    event("bow_item_path_rejected", static_cast<int>(proof.issue),
          static_cast<int>(request.quantity));
    return false;
  }
  if (!ubc::matching_bow_item_path(proof, request, frame) ||
      !ubc::same_crafting_hands(proof.stock.hands, handed_hands))
    return false;
  // Reread all material, knowledge, actor and INFO evidence after the full
  // closure, before the first operand patch. N1 does not establish Nmax.
  if (!bow_modules_current() || !same_context(c, context()) || !late_gate())
    return false;
  event("bow_item_path_verified", static_cast<int>(recipe.id), static_cast<int>(request.quantity));
  return true;
}
void* __cdecl execute_amount(bc::WriterScope& scope) {
  const auto incoming_error = GetLastError();
  scope.restore_verified = true;
  const auto& request = handoff.request();
  const auto* recipe = cat::find(request.id);
  if (recipe && recipe->tobacco)
    return execute_tobacco_amount(scope, *recipe->tobacco);
  amount_diagnostic = {
      true, false, true, 0, 0, scope.call.symbol, static_cast<int>(request.quantity)};
  const auto* entry = recipe ? callback_for(*recipe, request.quantity) : nullptr;
  const auto cool_plan = recipe && recipe->cool
                             ? ubc::crafting_cool_plan(request.material_snapshot, request.quantity)
                             : ubc::cool_recipe::Plan{};
  const bool predebit = cool_plan.valid && cool_plan.requires_extra_debit;
  const bool inspect = predebit && cool_context_mode;
  if (inspect)
    cool_check_reports.clear();
  if (predebit)
    amount_diagnostic.cool_info_token = info_effect_lease.token;
  if (cool_context_mode && recipe && recipe->cool && !inspect) {
    handoff.cancel(ubc::CraftingHandoffIssue::resources);
    amount_diagnostic.rejected = 8;
    SetLastError(incoming_error);
    return nullptr;
  }
  if (predebit && !cpre::runtime_ready) {
    handoff.cancel(ubc::CraftingHandoffIssue::resources);
    amount_diagnostic.rejected = 8;
    SetLastError(incoming_error);
    return nullptr;
  }
  const auto* dndr_recipe =
      dndr_active ? ubc::dndr::callback(static_cast<unsigned>(scope.call.symbol)) : nullptr;
  const auto* function =
      dndr_recipe && dndr_recipe->id == request.id ? dndr_recipe->effect->information
      : recipe && recipe->cool
          ? (cool_plan.valid ? (cool_plan.callback == 20775   ? ccat::single.information
                                : cool_plan.callback == 20778 ? ccat::triple.information
                                                              : nullptr)
                             : nullptr)
      : recipe && recipe->grind  ? &recipe->grind->function
      : recipe && recipe->spirit ? recipe->spirit->callback
      : entry                    ? entry->function
                                 : nullptr;
  const bool matching =
      function && function->symbol == static_cast<std::uint32_t>(scope.call.symbol);
  if (inspect) {
    cpre::check::Checks rows(&cool_check_reports.at(cpre::reader::Phase::admission));
    rows.flag("call.function", matching);
    rows.flag("call.guard_idle", result_guard.permits_next());
  }
  auto* grind_check = (request.key.kind == cat::Station::grind || (recipe && recipe->tobacco))
                          ? &amount_diagnostic.grind_check
                          : nullptr;
  const bool idle = inspect || !matching || result_guard.permits_next();
  if (!inspect && (!matching || !idle || !late_gate(grind_check))) {
    if (grind_check) {
      if (!function)
        grind_check->equal("call.function", true, false);
      else if (!matching)
        grind_check->equal("call.symbol", function->symbol, scope.call.symbol);
      else if (!idle)
        grind_check->equal("call.guard_idle", true, false);
    }
    handoff.cancel(ubc::CraftingHandoffIssue::resources);
    amount_diagnostic.rejected = 1;
    SetLastError(incoming_error);
    return nullptr;
  }
  NativeAmountContext ctx;
  ctx.function = function;
  ctx.predebit = predebit;
  const auto layout = rt::detail::read_layout(memory, s::game_parser, layout_profile, ctx.layout);
  const bool fits = function && function->byte_offset <= ctx.layout.code_length &&
                    function->byte_length <= ctx.layout.code_length - function->byte_offset;
  if (inspect) {
    cpre::check::Checks rows(&cool_check_reports.at(cpre::reader::Phase::admission));
    rows.equal("call.layout", 0, static_cast<int>(layout));
    rows.flag("call.body_range", fits);
  }
  if (!inspect && (layout != rt::Issue::none || !fits)) {
    if (grind_check) {
      if (layout != rt::Issue::none)
        grind_check->equal("call.layout", static_cast<int>(rt::Issue::none),
                           static_cast<int>(layout));
      else
        grind_check->equal_unsigned("call.minimum_code_length",
                                    std::uint64_t{function->byte_offset} + function->byte_length,
                                    ctx.layout.code_length);
    }
    handoff.cancel(ubc::CraftingHandoffIssue::context);
    amount_diagnostic.rejected = 2;
    SetLastError(incoming_error);
    return nullptr;
  }
  ammo_job = recipe->station == cat::Station::saw && request.quantity > 1;
  ammo_notice_ready = ammo_notice_acked = false;
  extra_job = recipe->route == cat::Route::alchemy_extra;
  spirit_job = recipe->route == cat::Route::spirit;
  cool_job = recipe->route == cat::Route::cool;
  grind_job = recipe->route == cat::Route::grind;
  grind_original_returned = grind_skill_log_pending = false;
  grind_skill_after_values = {};
  if (recipe->bow && !verify_bow_amount(scope, *recipe)) {
    handoff.cancel(ubc::CraftingHandoffIssue::resources);
    amount_diagnostic.rejected = 5;
    SetLastError(incoming_error);
    return nullptr;
  }
  if (cool_job && !predebit && !verify_cool_amount(scope, *recipe)) {
    handoff.cancel(ubc::CraftingHandoffIssue::resources);
    amount_diagnostic.rejected = 6;
    SetLastError(incoming_error);
    return nullptr;
  }
  if (grind_job && !verify_grind_amount(scope, *recipe)) {
    handoff.cancel(ubc::CraftingHandoffIssue::resources);
    amount_diagnostic.rejected = 7;
    SetLastError(incoming_error);
    return nullptr;
  }
  if (ammo_job) {
    ap::Context current;
    const ap::Admission proof{
        false, false,
        request_ui_generation == request.key.generation && request_ui_key == request.key, true,
        request.material_snapshot.actor_known && request.material_snapshot.actor_matches};
    if (!read_ammo_context(nullptr, current) ||
        !ammo::admit({current, request.key.generation, request.key.generation,
                      entry->function->symbol, static_cast<std::uint32_t>(entry->owner_instance),
                      request.quantity},
                     proof)) {
      handoff.cancel(ubc::CraftingHandoffIssue::resources);
      amount_diagnostic.rejected = 4;
      SetLastError(incoming_error);
      return nullptr;
    }
    ammo_diagnostic = {request.key.generation, entry->function->symbol, 0, 0};
  }
  LARGE_INTEGER before_time{}, after_time{}, frequency{};
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&before_time);
  result_started = GetTickCount64();
  const ubc::AmountMemory writable{memory, &ctx, amount_identity, amount_writable, amount_replace};
  ubc::AmountOutcome outcome;
  SetLastError(incoming_error);
  void* result = nullptr;
  alignas(16) unsigned char predebit_fx[512];
  if (extra_job) {
    result = execute_extra_amount(scope, writable, *recipe);
    outcome.entered = effect_executed;
    outcome.issue =
        extra_executor ? extra_executor->outcome().amount_issue : ubc::AmountIssue::contract;
  } else if (spirit_job) {
    result = execute_spirit_amount(scope, writable, *recipe);
    outcome.entered = effect_executed;
    outcome.issue =
        spirit_executor ? spirit_executor->outcome().amount_issue : ubc::AmountIssue::contract;
  } else if (predebit) {
    result = execute_cool_predebit(scope, writable);
    _fxsave(predebit_fx);
    outcome.entered = result_guard.cool_witness().valid &&
                      result_guard.state() == ubc::CraftingResultState::waiting;
    outcome.issue = amount_patch.issue();
  } else if (dndr_recipe) {
    result = run_dndr_amount(scope, writable, ctx, *dndr_recipe, outcome);
  } else if (cool_job)
    result =
        ubc::run_amount_call(amount_patch, writable, ctx.layout.code + ctx.function->byte_offset,
                             cool_active_entry.body(), request.quantity, execute_original_amount,
                             &scope, scope.restore_verified, outcome);
  else if (grind_job)
    result =
        ubc::run_amount_call(amount_patch, writable, ctx.layout.code + ctx.function->byte_offset,
                             grind_prepared.amount.body(), request.quantity,
                             execute_original_amount, &scope, scope.restore_verified, outcome);
  else
    result = ubc::run_amount_call(
        amount_patch, writable, ctx.layout.code + ctx.function->byte_offset, entry->amount,
        request.quantity, execute_original_amount, &scope, scope.restore_verified, outcome);
  const auto saved = GetLastError();
  if (!predebit) {
    amount_diagnostic.entered = effect_executed;
    amount_diagnostic.restored = scope.restore_verified;
  }
  if (ammo_job && !ammo::healthy())
    note_ammo_diagnostic(AmmoDiagnosticPhase::postcall_health);
  if (!outcome.entered) {
    handoff.cancel(ubc::CraftingHandoffIssue::resources);
    if (!amount_diagnostic.rejected && !amount_diagnostic.predebit.event)
      amount_diagnostic.rejected = 10 + static_cast<int>(outcome.issue);
    if (ammo_job)
      result_guard.fail(scope.restore_verified ? ubc::CraftingResultIssue::contract
                                               : ubc::CraftingResultIssue::restore);
  }
  if (effect_executed && !extra_job && !spirit_job && !predebit)
    result_guard.restored(scope.restore_verified);
  if (grind_job && effect_executed && scope.restore_verified) {
    const auto bridge = bc::diagnostics();
    grind_skill_after_values = read_grind_result_values(request);
    const bool original_bound =
        grind_original_returned && outcome.entered && own_thread() && bridge.writer &&
        !bridge.readers && !bridge.writes_disabled && scope.call.thread == GetCurrentThreadId() &&
        scope.call.caller == bc::choice_call_return &&
        scope.call.symbol == static_cast<int>(grind_prepared.before.callback) &&
        grind_prepared.job == request.key.generation && grind_prepared.key.frame == frame &&
        grind_modules_current() && grind_skill_after_values.known;
    grind_skill_log_pending = result_guard.skill_after(
        {grind_original_returned, original_bound, request.key, request.id, request.quantity,
         next_grind_serial(), frame, request.material_snapshot.grind, grind_skill_after_values});
    if (!grind_skill_log_pending) {
      auto& check = amount_diagnostic.grind_check;
      check.equal("skill.original_return", true, grind_original_returned);
      check.equal("skill.original_enter", true, outcome.entered);
      ubc::diagnose_grind_skill(check, grind_prepared, grind_skill_after_values);
      check.equal("skill.original_binding", true, original_bound);
      check.equal("skill.result_guard", true, false);
    }
  }
  if (!scope.restore_verified) {
    handoff.cancel(ubc::CraftingHandoffIssue::resources);
    result_guard.fail(ubc::CraftingResultIssue::restore);
  }
  QueryPerformanceCounter(&after_time);
  if (frequency.QuadPart > 0 && after_time.QuadPart >= before_time.QuadPart) {
    const auto elapsed = static_cast<double>(after_time.QuadPart - before_time.QuadPart) *
                         1000000.0 / static_cast<double>(frequency.QuadPart);
    amount_diagnostic.duration = elapsed >= INT32_MAX ? INT32_MAX : static_cast<int>(elapsed);
  }
  if (predebit)
    _fxrstor(predebit_fx);
  SetLastError(saved);
  return result;
}

} // namespace ubc_native
