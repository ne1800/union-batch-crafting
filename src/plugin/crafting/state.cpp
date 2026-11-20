// SPDX-License-Identifier: MIT
#include "state.hpp"

namespace ubc_native {
HMODULE module = nullptr;
HANDLE log_file = INVALID_HANDLE_VALUE;
volatile LONG installed = 0, engine_thread = 0;
bool disabled = false, native_update = false;
cpre::inspection::Reports cool_check_reports;
unsigned grind_measurement_case = 0;
std::uint32_t grind_measurement_whole = 0;
unsigned frame_depth = 0, update_depth = 0, info_depth = 0, choice_depth = 0, station_depth = 0,
         collection_depth = 0;
unsigned resolved_read_depth = 0;
std::uint64_t frame = 0, generation = 0, end_sequence = 0;
std::size_t log_bytes = 0, log_events = 0;
ubc::SelectionInfoRows rows;
bool choice_collection = false;
std::size_t choice_count = 0;
std::array<std::int32_t, 256> choice_functions{};
ubc::RecipeCatalogueReader catalogue_reader;
ubc::StationRecipeCatalogueReader station_catalogue_reader;
ubc::AnvilRecipeCatalogueReader anvil_catalogue_reader;
ubc::OreRecipeCatalogueReader ore_catalogue_reader;
ubc::station_recipe_snapshot_detail::HandWorkspace result_hands, end_hands;
ubc::PackedInventoryWorkspace result_workspace;
ubc::CraftingSession session;
ubc::NativeCraftingUi ui;
uidiag::Trace ui_diagnostic;
gready::Trace grind_ui_readiness;
gready::QuantityObservation grind_quantity_observation;
bool grind_ui_trace_active = false;
ubc::CraftingHandoff handoff;
ubc::CraftingResult result_guard;
ubc::AmountPatch amount_patch;
ubc::PostRemoveChoice post_remove_choice;
ubc::CraftingHandoffTicket dispatched_ticket;
ubc::StationRecipeHands handed_hands;
ubc::NativeCraftingSuccess pending_success{};
bool success_pending = false, ammo_job = false, ammo_install_attempted = false,
     ammo_notice_ready = false, ammo_notice_acked = false;
std::uint64_t world_epoch = 0;
std::uint32_t observed_world = 0, observed_hero = 0;
ap::Scope active_ammo_scope{};
ap::Context active_ammo_context{};
std::uint32_t remembered_world = 0, remembered_hero = 0, remembered_station = 0;
cat::RecipeId remembered_recipe = UINT32_MAX;
std::uint64_t rendered_generation = 0, request_ui_generation = 0;
ubc::CraftingKey request_ui_key{};
std::uint32_t borrowed_view = 0, original_station = 0;
bool private_enter = false, effect_authorized = false, effect_executed = false,
     recover_info = false;
bool failure_notified = false, failure_open_rejected_logged = false;
ULONGLONG handoff_started = 0, result_started = 0;
int last_frame_state = -1, last_handoff_state = -1, last_capture_issue = -1,
    last_failure_state = -1, last_end_path_issue = -1;
ubc::windows_probe::LoadedModuleIdentity end_zbug_module, end_union_module;
bool end_compatibility_attempted = false;
ubc::zbug_end_compat::Binding end_compatibility_binding{};
ubc::windows_probe::LoadedModuleIdentity extra_zpe_module, extra_crt_module;
ubc::windows_probe::LoadedModuleIdentity bow_renderer_module;
ubc::windows_probe::LoadedModuleIdentity bow_locator_module;
bool bow_locator_bound = false;
ubc::BowRecipeCatalogueReader bow_catalogue_reader;
std::optional<bitem::Reader> bow_item_reader;
bool bow_initialized = false;
std::optional<ubc::CraftingAlchemyExtraReader> extra_catalogue_reader;
std::optional<xwriter::Executor> extra_executor;
bool extra_initialized = false, extra_job = false, extra_normal_end = false;
std::uint64_t extra_serial = 0, extra_end_frame = 0;
xdebit::Job extra_active_job{};
xdebit::EndWitness extra_end{};
bc::WriterScope* extra_writer_scope = nullptr;
tobacco::Reader tobacco_reader;
tobacco::OwnerTicket tobacco_owner;
std::uint32_t tobacco_owner_call = 0;
int tobacco_choice_call = 0;
volatile LONG tobacco_unpaid_page = 0;
bool tobacco_owner_opened = false, tobacco_clean_rejection = false, tobacco_writer_entered = false;
ubc::windows_probe::LoadedModuleIdentity tobacco_locator;
ubc::bow_item_native::BowDispatch tobacco_targets;
ubc::cool_predebit::check::Report tobacco_report;
std::optional<xdep::ListReader> tobacco_list;

std::optional<ubc::SpiritCraftingReader> spirit_catalogue_reader;
std::optional<gwriter::Executor> spirit_executor;
bool spirit_job = false, spirit_normal_end = false;
std::uint64_t spirit_end_frame = 0;
gdebit::Job spirit_active_job{};
gdebit::EndWitness spirit_end{};
bc::WriterScope* spirit_writer_scope = nullptr;
ubc::CoolRecipeCatalogueReader cool_catalogue_reader;
std::optional<cpre::reader::Reader> cool_predebit_reader;
cpre::Tracker cool_predebit_tracker;
std::optional<cwriter::Executor> cool_predebit_executor;
cpre::DestroyJournal cool_predebit_journal;
const bc::WriterScope* cool_writer_scope = nullptr;
std::uint64_t cool_fence_token = 0;
#ifndef UBC_PLAYER_RELEASE
ubc::windows_probe::LoadedModuleIdentity cool_predebit_engine;
#endif
ubc::PackedInventoryWorkspace cool_inventory;
ubc::station_recipe_snapshot_detail::HandWorkspace cool_hands;
ubc::windows_destroy_remove_callsite::Adapter cool_remove_observer;
bool grind_probe_failed = false, cool_install_attempted = false, cool_destroy_installed = false,
     cool_ready = false, cool_job = false, cool_receipt_acked = false;
unsigned cool_destroy_depth = 0;
std::uint64_t cool_serial = 0, cool_job_floor = 0;
cdirect::Entry cool_active_entry{};
cdestroy::Observation cool_destroy_call{};
cdestroy::Binding cool_destroy_binding{};
chdiag::Record cool_destroy_hand_diagnostic{};
ubc::GrindRecipeCatalogueReader grind_catalogue_reader;
alignas(ubc::GrindRecipeCatalogueCapture)
    std::array<std::uint8_t, sizeof(ubc::GrindRecipeCatalogueCapture)> grind_capture_storage{};
bool grind_capture_leased = false;
std::array<GrindPreviewDiagnostic, shcat::recipes.size()> grind_preview_diagnostic{};
std::optional<shitem::Reader> grind_item_reader;
std::optional<shpreview::Reader> grind_preview_reader;
shpreview::Publication grind_preview_publication;
std::uint64_t grind_preview_serial = 0, grind_preview_input_serial = 0;
unsigned grind_preview_input_depth = 0;
bool grind_preview_overflow = false;
GrindPreviewFailureDiagnostic grind_preview_failure;
ubc::windows_probe::LoadedModuleIdentity grind_renderer_module;
ubc::windows_probe::LoadedModuleIdentity grind_locator_module;
bool grind_locator_bound = false;
bool grind_initialized = false, grind_job = false, grind_original_returned = false,
     grind_skill_log_pending = false;
std::uint64_t grind_serial = 0, grind_job_floor = 0;
shdestroy::Prepared grind_prepared{};
shdestroy::Observation grind_destroy_call{};
shdestroy::Binding grind_destroy_binding{};
ubc::CraftingGrindValues grind_skill_after_values{};
GrindChoiceLease grind_choice_lease;
std::optional<ddispatch::Reader> destroy_dispatch_reader;
InfoEffectLease info_effect_lease;
cadmission::InfoLease info_lease_owner;
AmountDiagnostic amount_diagnostic;
AmmoDiagnostic ammo_diagnostic;

Destroy original_destroy = nullptr;
w::Hooks<s::HookSite, 1> cool_hooks(cool_sites);
a::FastObject original_update = nullptr, original_choice = nullptr, original_clear = nullptr;
a::AddChoice original_add = nullptr;
a::ResolveInfo original_resolve = nullptr;
Info original_info = nullptr;
Start original_start = nullptr;
End original_end = nullptr;
m::MenuHandleEvent original_menu_event = nullptr;
a::HandleEvent original_choice_event = nullptr;
w::Hooks<s::HookSite, sites.size()> hooks(sites);
} // namespace ubc_native
