// SPDX-License-Identifier: MIT
#pragma once
#include <windows.h>
#include <intrin.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <new>
#include <memory>
#ifdef UBC_PLAYER_RELEASE
#include "../player_engine_windows.hpp"
#endif
#include "ubc/engine/frame_bridge.hpp"
#include "ubc/engine/crafting_hook_sites.hpp"
#include "ubc/engine/batch_call_bridge.hpp"
#include "ubc/core/batch_amount_call.hpp"
#include "ubc/ui/native_crafting_ui.hpp"
#include "ubc/engine/native_crafting_ammo.hpp"
#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/stations/cool/cool_listed_input.hpp"
#include "ubc/stations/saw/saw_actor_preview.hpp"
#include "ubc/core/crafting_handoff.hpp"
#include "ubc/core/crafting_info_effect.hpp"
#include "ubc/core/crafting_result.hpp"
#include "ubc/stations/station_end_contract.hpp"
#include "ubc/core/selection_snapshot.hpp"
#include "ubc/core/post_remove_choice.hpp"
#include "ubc/engine/windows_probe.hpp"
#include "ubc/engine/windows_module_identity.hpp"
#include "ubc/stations/alchemy/crafting_alchemy_extra_reader.hpp"
#include "../../writers/tobacco_writer.hpp"
#include "ubc/stations/alchemy/alchemy_extra_end_snapshot.hpp"
#include "../../writers/alchemy_extra_writer.hpp"
#include "ubc/stations/spirit/crafting_spirit_projection.hpp"
#include "ubc/stations/spirit/spirit_end_snapshot.hpp"
#include "../../writers/spirit_writer.hpp"
#include "ubc/stations/bow/crafting_bow_projection.hpp"
#include "ubc/stations/bow/bow_menu_checks.hpp"
#include "ubc/stations/bow/crafting_bow_execution.hpp"
#include "ubc/stations/cool/crafting_cool_projection.hpp"
#include "ubc/stations/cool/cool_direct_contract.hpp"
#include "../../bridges/cool_predebit_bridge.hpp"
#include "ubc/stations/cool/cool_predebit_journal.hpp"
#include "ubc/stations/cool/cool_predebit_projection.hpp"
#include "ubc/stations/cool/cool_native_path.hpp"
#include "ubc/stations/cool/cool_destroy_observation.hpp"
#include "ubc/stations/cool/cool_destroy_hand_diagnostic.hpp"
#include "ubc/stations/cool/generated_blade_hand_shape.hpp"
#include "ubc/engine/windows_destroy_remove_callsite.hpp"
#include "ubc/stations/grind/crafting_grind_projection.hpp"
#include "ubc/stations/grind/grind_destroy_observation.hpp"
#include "ubc/stations/grind/grind_menu_metadata.hpp"
#include "ubc/stations/grind/grind_preview_batch.hpp"
#include "ubc/stations/grind/grind_result_diagnostic.hpp"
#include "ubc/stations/grind/grind_preview_measurement.hpp"

namespace ubc_native {
namespace a = ubc::selection_abi;
namespace s = ubc::station_abi;
namespace m = ubc::native_menu;
namespace w = ubc::windows_probe;
namespace cat = ubc::crafting_catalog;
namespace basecat = ubc::recipe_catalog;
namespace scat = ubc::station_recipe_catalog;
namespace snav = ubc::station_recipe_navigation;
namespace acat = ubc::anvil_recipe_catalog;
namespace art = ubc::anvil_recipe_runtime;
namespace anav = ubc::anvil_recipe_navigation;
namespace ocat = ubc::ore_recipe_catalog;
namespace ort = ubc::ore_recipe_runtime;
namespace bcat = ubc::bow_recipe_catalog;
namespace brt = ubc::bow_recipe_runtime;
namespace bnav = ubc::bow_recipe_navigation;
namespace bitem = ubc::bow_item_runtime;
namespace ccat = ubc::cool_recipe_catalog;
namespace crt = ubc::cool_recipe_runtime;
namespace cnav = ubc::cool_recipe_navigation;
namespace cpath = ubc::cool_native_path;
namespace ddispatch = ubc::cool_destroy_dispatch;
namespace cdirect = ubc::cool_direct_contract;
namespace cpre = ubc::cool_predebit;
namespace cadmission = cpre::admission;
namespace cwriter = ubc::cool_predebit_bridge;
namespace cdestroy = ubc::cool_destroy_observation;
namespace chdiag = ubc::cool_destroy_hand_diagnostic;
namespace bhshape = ubc::generated_blade_hand_shape;
namespace cabi = ubc::cool_destroy_abi;
namespace rcall = ubc::destroy_remove_callsite;
namespace shcat = ubc::grind_recipe;
namespace shrt = ubc::grind_recipe_runtime;
namespace shnav = ubc::grind_recipe_navigation;
namespace shitem = ubc::grind_item_runtime;
namespace shdestroy = ubc::grind_destroy_observation;
namespace shpreview = ubc::grind_preview_batch;
namespace xcat = ubc::alchemy_recipe_catalog;
namespace tobacco = ubc::tobacco;
namespace xnav = ubc::alchemy_extra_navigation;
namespace xdep = ubc::alchemy_extra_dependencies;
namespace xwriter = ubc::alchemy_extra_writer;
namespace xdebit = ubc::alchemy_flask_debit;
namespace xbridge = ubc::alchemy_flask_debit_bridge;
namespace gcat = ubc::spirit_recipe_catalog;
namespace grt = ubc::spirit_recipe_runtime;
namespace gwriter = ubc::spirit_writer;
namespace gdebit = ubc::spirit_debit;
namespace ammo = ubc::native_crafting_ammo;
namespace uidiag = ubc::crafting_ui_diagnostics;
namespace gready = ubc::grind_ui_readiness;
namespace ap = ubc::ammo_presentation;
namespace nav = ubc::recipe_navigation;
namespace rt = ubc::runtime_script;
namespace bc = ubc::batch_call;
using Phase = ubc::SelectionPhase;
inline constexpr ubc::ReadOnlyMemory memory{nullptr, w::copy_memory};
inline constexpr ubc::lhiver::Profile layout_profile{ubc::lhiver::game_parser,
                                                     ubc::lhiver::symbol_count,
                                                     ubc::lhiver::code_length,
                                                     ubc::lhiver::code_sha256,
                                                     nullptr,
                                                     0,
                                                     nullptr,
                                                     0};
extern HMODULE module;
extern HANDLE log_file;
extern volatile LONG installed, engine_thread;
extern bool execution_enabled;
extern bool disabled, native_update;
#ifdef UBC_PLAYER_RELEASE
inline constexpr bool grind_measurement_mode = false;
inline constexpr bool campfire_observation_mode = false;
inline constexpr bool bow_diagnosis = false, bow_comparison = false;
inline constexpr bool cool_context_mode = false;
#else
extern bool grind_measurement_mode;
extern bool campfire_observation_mode;
extern bool bow_diagnosis, bow_comparison;
extern bool cool_context_mode;
#endif
extern cpre::inspection::Reports cool_check_reports;
extern unsigned grind_measurement_case;
extern std::uint32_t grind_measurement_whole;
extern unsigned frame_depth, update_depth, info_depth, choice_depth, station_depth,
    collection_depth;
extern unsigned resolved_read_depth;
extern std::uint64_t frame, generation, end_sequence;
extern std::size_t log_bytes, log_events;
extern ubc::SelectionInfoRows rows;
extern bool choice_collection;
extern std::size_t choice_count;
extern std::array<std::int32_t, 256> choice_functions;
extern ubc::RecipeCatalogueReader catalogue_reader;
extern ubc::StationRecipeCatalogueReader station_catalogue_reader;
extern ubc::AnvilRecipeCatalogueReader anvil_catalogue_reader;
extern ubc::OreRecipeCatalogueReader ore_catalogue_reader;
extern ubc::station_recipe_snapshot_detail::HandWorkspace result_hands, end_hands;
extern ubc::PackedInventoryWorkspace result_workspace;
extern ubc::CraftingSession session;
extern ubc::NativeCraftingUi ui;
extern uidiag::Trace ui_diagnostic;
extern gready::Trace grind_ui_readiness;
extern gready::QuantityObservation grind_quantity_observation;
extern bool grind_ui_trace_active;
extern ubc::CraftingHandoff handoff;
extern ubc::CraftingResult result_guard;
extern ubc::AmountPatch amount_patch;
extern ubc::PostRemoveChoice post_remove_choice;
extern ubc::CraftingHandoffTicket dispatched_ticket;
extern ubc::StationRecipeHands handed_hands;
extern ubc::NativeCraftingSuccess pending_success;
extern bool success_pending, ammo_job, ammo_install_attempted, ammo_notice_ready, ammo_notice_acked;
extern std::uint64_t world_epoch;
extern std::uint32_t observed_world, observed_hero;
extern ap::Scope active_ammo_scope;
extern ap::Context active_ammo_context;
extern std::uint32_t remembered_world, remembered_hero, remembered_station;
extern cat::RecipeId remembered_recipe;
extern std::uint64_t rendered_generation, request_ui_generation;
extern ubc::CraftingKey request_ui_key;
extern std::uint32_t borrowed_view, original_station;
extern bool private_enter, effect_authorized, effect_executed, recover_info;
extern bool failure_notified, failure_open_rejected_logged;
extern ULONGLONG handoff_started, result_started;
extern int last_frame_state, last_handoff_state, last_capture_issue, last_failure_state,
    last_end_path_issue;
extern ubc::windows_probe::LoadedModuleIdentity end_zbug_module, end_union_module;
extern bool end_compatibility_attempted;
extern ubc::zbug_end_compat::Binding end_compatibility_binding;
extern ubc::windows_probe::LoadedModuleIdentity extra_zpe_module, extra_crt_module;
extern ubc::windows_probe::LoadedModuleIdentity bow_renderer_module;
extern ubc::windows_probe::LoadedModuleIdentity bow_locator_module;
extern bool bow_locator_bound;
extern ubc::BowRecipeCatalogueReader bow_catalogue_reader;
extern std::optional<bitem::Reader> bow_item_reader;
extern bool bow_initialized;
extern std::optional<ubc::CraftingAlchemyExtraReader> extra_catalogue_reader;
extern std::optional<xwriter::Executor> extra_executor;
extern bool extra_initialized, extra_job, extra_normal_end;
extern std::uint64_t extra_serial, extra_end_frame;
extern xdebit::Job extra_active_job;
extern xdebit::EndWitness extra_end;
extern bc::WriterScope* extra_writer_scope;
extern tobacco::Reader tobacco_reader;
extern tobacco::OwnerTicket tobacco_owner;
extern std::uint32_t tobacco_owner_call;
extern int tobacco_choice_call;
extern volatile LONG tobacco_unpaid_page;
extern bool tobacco_owner_opened, tobacco_clean_rejection, tobacco_writer_entered;
extern ubc::windows_probe::LoadedModuleIdentity tobacco_locator;
extern ubc::bow_item_native::BowDispatch tobacco_targets;
extern ubc::cool_predebit::check::Report tobacco_report;
extern std::optional<xdep::ListReader> tobacco_list;

extern std::optional<ubc::SpiritCraftingReader> spirit_catalogue_reader;
extern std::optional<gwriter::Executor> spirit_executor;
extern bool spirit_job, spirit_normal_end;
extern std::uint64_t spirit_end_frame;
extern gdebit::Job spirit_active_job;
extern gdebit::EndWitness spirit_end;
extern bc::WriterScope* spirit_writer_scope;
extern ubc::CoolRecipeCatalogueReader cool_catalogue_reader;
extern std::optional<cpre::reader::Reader> cool_predebit_reader;
extern cpre::Tracker cool_predebit_tracker;
extern std::optional<cwriter::Executor> cool_predebit_executor;
extern cpre::DestroyJournal cool_predebit_journal;
extern const bc::WriterScope* cool_writer_scope;
extern std::uint64_t cool_fence_token;
#ifndef UBC_PLAYER_RELEASE
extern ubc::windows_probe::LoadedModuleIdentity cool_predebit_engine;
#endif
extern ubc::PackedInventoryWorkspace cool_inventory;
extern ubc::station_recipe_snapshot_detail::HandWorkspace cool_hands;
extern ubc::windows_destroy_remove_callsite::Adapter cool_remove_observer;
extern bool grind_probe_failed, cool_install_attempted, cool_destroy_installed, cool_ready,
    cool_job, cool_receipt_acked;
extern unsigned cool_destroy_depth;
extern std::uint64_t cool_serial, cool_job_floor;
extern cdirect::Entry cool_active_entry;
extern cdestroy::Observation cool_destroy_call;
extern cdestroy::Binding cool_destroy_binding;
extern chdiag::Record cool_destroy_hand_diagnostic;
extern ubc::GrindRecipeCatalogueReader grind_catalogue_reader;
alignas(ubc::GrindRecipeCatalogueCapture) extern std::array<
    std::uint8_t, sizeof(ubc::GrindRecipeCatalogueCapture)> grind_capture_storage;
extern bool grind_capture_leased;
struct GrindPreviewDiagnostic {
  std::uint64_t generation = 0;
  int issue = -1;
  std::uint32_t address = 0;
};
extern std::array<GrindPreviewDiagnostic, shcat::recipes.size()> grind_preview_diagnostic;
extern std::optional<shitem::Reader> grind_item_reader;
extern std::optional<shpreview::Reader> grind_preview_reader;
extern shpreview::Publication grind_preview_publication;
extern std::uint64_t grind_preview_serial, grind_preview_input_serial;
extern unsigned grind_preview_input_depth;
extern bool grind_preview_overflow;
struct GrindPreviewFailureDiagnostic {
  std::uint64_t generation = 0;
  int stage = 0, issue = -1;
  ubc::GrindCheck check{};
};
extern GrindPreviewFailureDiagnostic grind_preview_failure;
extern ubc::windows_probe::LoadedModuleIdentity grind_renderer_module;
extern ubc::windows_probe::LoadedModuleIdentity grind_locator_module;
extern bool grind_locator_bound;
extern bool grind_initialized, grind_job, grind_original_returned, grind_skill_log_pending;
extern std::uint64_t grind_serial, grind_job_floor;
extern shdestroy::Prepared grind_prepared;
extern shdestroy::Observation grind_destroy_call;
extern shdestroy::Binding grind_destroy_binding;
extern ubc::CraftingGrindValues grind_skill_after_values;
struct GrindChoiceLease {
  std::uint32_t owner = 0, thread = 0, game = 0;
  unsigned depth = 0;
  std::uint64_t frame_number = 0;
  ubc::CraftingKey key{};
  ubc::CraftingHandoffTicket ticket{};
  shnav::PageEvidence page{};
};
extern GrindChoiceLease grind_choice_lease;
extern std::optional<ddispatch::Reader> destroy_dispatch_reader;
using InfoEffectLease = ubc::CraftingInfoEffectLease;
extern InfoEffectLease info_effect_lease;
extern cadmission::InfoLease info_lease_owner;
struct AmountDiagnostic {
  bool pending = false, entered = false, restored = false;
  int rejected = 0, duration = 0, symbol = 0, quantity = 0;
  cpre::Rejection predebit{};
  std::uint64_t cool_info_token = 0;
  ubc::GrindCheck grind_check{};
};
extern AmountDiagnostic amount_diagnostic;
enum class AmmoDiagnosticPhase : unsigned {
  close_context = 1,
  close_effect = 2,
  postcall_health = 4,
  frame_health = 8,
  timeout = 16
};
struct AmmoDiagnostic {
  std::uint64_t job = 0;
  std::uint32_t callback = 0;
  unsigned phases = 0, reported = 0;
};
extern AmmoDiagnostic ammo_diagnostic;

using Start = void(__thiscall*)(void*, void*);
using End = void(__thiscall*)(void*, void*, int);
using Info = void(__thiscall*)(void*);
using Destroy = int(__thiscall*)(void*, void*);
extern Destroy original_destroy;
inline constexpr std::array<s::HookSite, 1> cool_sites{
    {{cabi::destroy_entry, 6, 6, {{0x64, 0xa1, 0, 0, 0, 0}}}}};
extern w::Hooks<s::HookSite, 1> cool_hooks;
extern a::FastObject original_update, original_choice, original_clear;
extern a::AddChoice original_add;
extern a::ResolveInfo original_resolve;
extern Info original_info;
extern Start original_start;
extern End original_end;
extern m::MenuHandleEvent original_menu_event;
extern a::HandleEvent original_choice_event;
using ubc::crafting_hook_sites::sites;
extern w::Hooks<s::HookSite, sites.size()> hooks;

template <class T> bool read(std::uint32_t address, T& out) noexcept {
  return memory.copy(address, &out, sizeof(out));
}
template <class T> bool field(std::uint32_t object, std::uint32_t offset, T& out) noexcept {
  return object && offset <= UINT32_MAX - object && read(object + offset, out);
}
} // namespace ubc_native
