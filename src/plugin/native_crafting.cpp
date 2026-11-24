// SPDX-License-Identifier: MIT
#include "crafting/runtime.hpp"
#ifdef UBC_PLAYER_RELEASE
#include "player_campaign_windows.hpp"
#endif

namespace ubc_native {
bool execution_enabled = false;
#ifndef UBC_PLAYER_RELEASE
bool grind_measurement_mode = false;
bool campfire_observation_mode = false;
bool bow_diagnosis = false, bow_comparison = false;
bool cool_context_mode = false;
#endif

bool configure_batch_symbols(bc::Config& config) noexcept {
  if (config.symbol_count)
    return false;
  for (const auto& recipe : basecat::recipes)
    for (const auto& callback : recipe.callbacks)
      if (callback.minimum_quantity) {
        if (config.symbol_count == config.symbols.size()) {
          return false;
        }
        config.symbols[config.symbol_count++] =
            static_cast<std::int32_t>(callback.function->symbol);
      }
  for (const auto& recipe : scat::recipes) {
    if (config.symbol_count == config.symbols.size()) {
      return false;
    }
    config.symbols[config.symbol_count++] =
        static_cast<std::int32_t>(recipe.callback.function->symbol);
  }
  const auto add = [&](const ubc::lhiver::Function* f) noexcept {
    if (!f || config.symbol_count == config.symbols.size())
      return false;
    config.symbols[config.symbol_count++] = static_cast<std::int32_t>(f->symbol);
    return true;
  };
  if (!add(tobacco::owner.information))
    return false;
  for (const auto& recipe : tobacco::recipes)
    if (!add(recipe.callback.function))
      return false;
  for (const auto& recipe : xcat::recipes)
    if (!add(recipe.callback.function))
      return false;
  for (const auto& recipe : acat::recipes)
    if (!add(recipe.callback.function) || !add(recipe.original_three.function))
      return false;
  for (const auto& recipe : ocat::recipes)
    if (!add(recipe.callback.function))
      return false;
  for (const auto& recipe : gcat::recipes)
    if (!add(recipe.callback))
      return false;
  for (const auto& recipe : bcat::recipes)
    if (!add(recipe.callback.function))
      return false;
  if (!add(ccat::single.information) || !add(ccat::triple.information))
    return false;
  for (const auto& recipe : shcat::recipes)
    if (!add(&recipe.function))
      return false;
  for (std::size_t i = 0; i < config.symbol_count; ++i)
    for (std::size_t j = 0; j < i; ++j)
      if (config.symbols[i] == config.symbols[j])
        return false;
  return true;
}
#ifdef UBC_PLAYER_RELEASE
static bool install_notice() noexcept {
  static constexpr std::array<s::HookSite, 2> notice_sites{
      {a::frame_hook, ubc::crafting_hook_sites::menu_event_hook}};
  static w::Hooks<s::HookSite, 2> notice_hooks(notice_sites);
  const auto bridge = reinterpret_cast<std::uintptr_t>(&UbcFrameBridge);
  if (!notice_hooks.prepare({bridge, reinterpret_cast<std::uintptr_t>(&observe_menu_event)}))
    return false;
  UbcFrameContinuation = notice_hooks.original(0);
  UbcAfterFrame = after_frame;
  original_menu_event = reinterpret_cast<m::MenuHandleEvent>(notice_hooks.original(1));
  return notice_hooks.install(bridge);
}
#endif
static void install(bool execute) noexcept {
  wchar_t path[32768]{};
  if (!w::sibling_path(module, path, L"UnionBatchCraftingExtended.log"))
    return;
#ifdef UBC_PLAYER_RELEASE
  constexpr DWORD disposition = CREATE_ALWAYS;
#else
  constexpr DWORD disposition = CREATE_NEW;
#endif
  log_file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, disposition,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (log_file == INVALID_HANDLE_VALUE)
    return;
#ifdef UBC_PLAYER_RELEASE
  const auto campaign = ubc::player_campaign::mode(ubc::player_campaign::active_dat_matches(),
                                                   ubc::player_campaign::marcello_selected());
  if (campaign == ubc::player_campaign::Mode::idle) {
    event("plugin_idle", 0, 0, ",\"reason\":\"unsupported_scripts_without_marcello_lhiver\"");
    return;
  }
#endif
  const char* header = execute ? "crafting_version=1\r\nmode=recipe_amounts\r\n"
                               : "crafting_version=1\r\nmode=ui_only\r\n";
  DWORD written = 0;
  WriteFile(log_file, header, static_cast<DWORD>(std::strlen(header)), &written, nullptr);
  log_bytes = written;
#ifdef UBC_PLAYER_RELEASE
  if (campaign == ubc::player_campaign::Mode::notice)
    event("script_profile_rejected");
  if (!w::player_image_header()) {
    event("engine_header_rejected");
    return;
  }
  player_image = ubc::player_engine::inspect_executable();
  for (unsigned i = 0; i < player_image.failure.size(); ++i)
    if (player_image.failure[i])
      event("engine_station_disabled", i, player_image.failure[i]);
  if (!player_image.any())
    return;
#endif
  std::array<std::uint8_t, a::frame_prefix_bytes.size()> prefix{};
  if (!memory.copy(a::frame_prefix, prefix.data(), prefix.size()) ||
      prefix != a::frame_prefix_bytes) {
    event("frame_prefix_rejected");
    return;
  }
#ifdef UBC_PLAYER_RELEASE
  if (campaign == ubc::player_campaign::Mode::notice) {
    script_notice_only = true;
    if (install_notice()) {
      InterlockedExchange(&installed, 1);
      event("notice_hooks_installed");
    } else
      event("hooks_rejected");
    return;
  }
#endif
  const std::array<std::uintptr_t, sites.size()> destinations{
      {reinterpret_cast<std::uintptr_t>(&UbcFrameBridge),
       reinterpret_cast<std::uintptr_t>(&observe_add),
       reinterpret_cast<std::uintptr_t>(&observe_clear),
       reinterpret_cast<std::uintptr_t>(&observe_update),
       reinterpret_cast<std::uintptr_t>(&observe_resolve),
       reinterpret_cast<std::uintptr_t>(&observe_info),
       reinterpret_cast<std::uintptr_t>(&observe_choice),
       reinterpret_cast<std::uintptr_t>(&observe_start),
       reinterpret_cast<std::uintptr_t>(&observe_end),
       reinterpret_cast<std::uintptr_t>(&observe_menu_event),
       reinterpret_cast<std::uintptr_t>(&UbcBatchCallBridge),
       reinterpret_cast<std::uintptr_t>(&observe_choice_event)}};
  if (!hooks.prepare(destinations)) {
    event("hooks_rejected");
    return;
  }
  UbcFrameContinuation = hooks.original(0);
  UbcAfterFrame = after_frame;
  original_add = reinterpret_cast<a::AddChoice>(hooks.original(1));
  original_clear = reinterpret_cast<a::FastObject>(hooks.original(2));
  original_update = reinterpret_cast<a::FastObject>(hooks.original(3));
  original_resolve = reinterpret_cast<a::ResolveInfo>(hooks.original(4));
  original_info = reinterpret_cast<Info>(hooks.original(5));
  original_choice = reinterpret_cast<a::FastObject>(hooks.original(6));
  original_start = reinterpret_cast<Start>(hooks.original(7));
  original_end = reinterpret_cast<End>(hooks.original(8));
  original_menu_event = reinterpret_cast<m::MenuHandleEvent>(hooks.original(9));
  UbcBatchCallOriginal = reinterpret_cast<void*>(hooks.original(10));
  original_choice_event = reinterpret_cast<a::HandleEvent>(hooks.original(11));
  bc::Config config;
  config.parser = reinterpret_cast<void*>(s::game_parser);
  config.signature = batch_signature;
  config.select = select_batch_call;
  config.writer = execute_amount;
  config.reject_unexpected = reject_unexpected_cool_info;
  if (!configure_batch_symbols(config)) {
    event("hooks_rejected");
    return;
  }
  if (!bc::configure(config) || !ammo::configure({nullptr, read_ammo_context, read_ammo_scope})) {
    event("hooks_rejected");
    return;
  }
  if (hooks.install(reinterpret_cast<std::uintptr_t>(&UbcFrameBridge))) {
    InterlockedExchange(&installed, 1);
    event("hooks_installed");
  } else
    event("hooks_rejected");
}
} // namespace ubc_native

using namespace ubc_native;
#ifdef UBC_PLAYER_RELEASE
// Union calls this after the game has resolved its paths and loaded its scripts.
extern "C" __declspec(dllexport) void __cdecl Game_Init() {
  static volatile LONG attempted = 0;
  const auto error = GetLastError();
  if (execution_enabled && !InterlockedExchange(&attempted, 1))
    install(true);
  SetLastError(error);
}
#endif
extern "C" unsigned __cdecl UbcSelectionProbeVersion() { return 4; }
extern "C" int __cdecl UbcSelectionProbeHooksInstalled() {
  return static_cast<int>(InterlockedCompareExchange(&installed, 0, 0));
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    module = instance;
    const auto error = GetLastError();
#ifdef UBC_PLAYER_RELEASE
    wchar_t ini[32768]{};
    if (w::sibling_path(nullptr, ini, L"Gothic.INI") &&
        GetPrivateProfileIntW(L"UNIONBATCHCRAFTING", L"Enabled", 1, ini) != 0 &&
        ubc::player_engine::game_process()) {
      execution_enabled = true;
    }
#else
    if (w::target_matches()) {
      // The external cooling measurement reads only the bound
      // process. No hooks, UI, writers or engine calls in this mode.
      if (w::consume_marker(instance, L"UnionBatchCraftingExtended.enable-once",
                            "EXTENDED_COOL_MEASURE_V1\n")) {
        SetLastError(error);
        return TRUE;
      }
      grind_measurement_mode = w::consume_marker(
          instance, L"UnionBatchCraftingExtended.enable-once", "EXTENDED_GRIND_MEASURE_V1\n");
      campfire_observation_mode = w::consume_marker(
          instance, L"UnionBatchCraftingExtended.enable-once", "EXTENDED_CAMPFIRE_V1\n");
      bow_diagnosis = w::consume_marker(instance, L"UnionBatchCraftingExtended.enable-once",
                                        "EXTENDED_BOW_DIAG_V1\n");
      bow_comparison = w::consume_marker(instance, L"UnionBatchCraftingExtended.enable-once",
                                         "EXTENDED_BOW_COMPARE_V1\n");
      const bool preview = !campfire_observation_mode && !grind_measurement_mode &&
                           w::consume_marker(instance, L"UnionBatchCraftingExtended.enable-once",
                                             "EXTENDED_UI_V1\n");
      cool_context_mode = !campfire_observation_mode &&
                          w::consume_marker(instance, L"UnionBatchCraftingExtended.enable-once",
                                            "EXTENDED_COOL_CONTEXT_V1\n");
      execution_enabled = bow_comparison || cool_context_mode ||
                          (!campfire_observation_mode && !grind_measurement_mode && !preview &&
                           w::consume_marker(instance, L"UnionBatchCraftingExtended.enable-once",
                                             "EXTENDED_AMOUNTS_V1\n"));
      if (bow_diagnosis || campfire_observation_mode || grind_measurement_mode || preview ||
          execution_enabled)
        install(execution_enabled);
    }
#endif
    SetLastError(error);
  } else if (reason == DLL_PROCESS_DETACH && log_file != INVALID_HANDLE_VALUE) {
    CloseHandle(log_file);
    log_file = INVALID_HANDLE_VALUE;
  }
  return TRUE;
}
