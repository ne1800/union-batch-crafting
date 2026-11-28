// SPDX-License-Identifier: MIT
#pragma once
#include "state.hpp"

namespace ubc_native {
#ifdef UBC_ENABLE_DNDR
void install_forge() noexcept;
void forge_frame() noexcept;
bool forge_event(void* menu, int key) noexcept;
void forge_start(void* mob, void* npc) noexcept;
void forge_end(void* mob, void* npc, int flag, std::uint32_t caller) noexcept;
#endif
struct Context {
  bool valid = false, identity_known = false, station_known = false, ownership_known = false;
  std::uint32_t game = 0, world = 0, hero = 0, station = 0;
  cat::Station kind = cat::Station::stove;
  ubc::SelectionDialog dialog{};
};
// The observer context persists even after a failed batch. Engine addresses
// bind the scope. They do not represent fresh RAM reads at each probe.
// Global checks bracket the context and the modules actually held.
struct GrindPreviewScope {
  Context bound{};
  std::uint64_t serial = 0;
  bool active = false, failed = false;
};
enum class DestroyReadPhase { closed, preview, resolver, callback, destroy };
struct Capture {
  ubc::CraftingSnapshot snapshot{};
  ubc::StationRecipeHands hands{};
  int issue = 0;
};
struct NativeAmountContext {
  rt::detail::Layout layout{};
  const ubc::lhiver::Function* function = nullptr;
  bool predebit = false;
};

void capture_dndr(Capture&, const Context&, bool) noexcept;
void* run_dndr_amount(bc::WriterScope&, const ubc::AmountMemory&, const NativeAmountContext&,
                      const ubc::dndr::Recipe&, ubc::AmountOutcome&);
void* __cdecl execute_original_amount(void*);
// The full copy of 4096 objects persists. Exactly one reader holds it
// until its projection finishes. Reentry must not replace this storage.
struct GrindCaptureLease {
  ubc::GrindRecipeCatalogueCapture* value = nullptr;
  GrindCaptureLease(const ubc::GrindRecipeKey& k, bool boundary) noexcept;
  ~GrindCaptureLease();
  GrindCaptureLease(const GrindCaptureLease&) = delete;
  GrindCaptureLease& operator=(const GrindCaptureLease&) = delete;
  const ubc::GrindRecipeCatalogueCapture* view(const ubc::GrindRecipeKey& k) const noexcept;
};

// Only the outer refresh/quantity block holds this lease. The callback, handoff
// and writer never acquire it by default. There is no direct fallback on failure.
struct GrindPreviewLease {
  bool owner = false, ready = false;
  // 1 prerequisites, 2 fresh initial binding, 3 reader, 4 fresh
  // final binding, 5 publication. -1 is not a reader result.
  int failure_stage = 1, read_issue = -1;
  ubc::GrindCheck check{};
  shpreview::Statistics statistics{};
  explicit GrindPreviewLease(const Context& c, bool include_selection = true) noexcept;
  ~GrindPreviewLease();
  GrindPreviewLease(const GrindPreviewLease&) = delete;
  GrindPreviewLease& operator=(const GrindPreviewLease&) = delete;
  const shpreview::Publication* view(const ubc::GrindRecipeKey& k) const noexcept;
};

extern Context borrowed_context;
extern GrindPreviewScope grind_preview_scope;
extern std::uint64_t bow_measured_frame;
extern std::size_t bow_sample_count;
#ifdef UBC_PLAYER_RELEASE
extern ubc::player_engine::Result player_image;
extern std::array<bool, cat::recipe_count> player_recipes;
#endif

// Context
std::int32_t integer(std::uint32_t object, std::uint32_t offset) noexcept;
bool own_thread() noexcept;
bool name_is(std::uint32_t object, std::uint32_t offset, const char* expected) noexcept;
bool station_owns(std::uint32_t mob, std::uint32_t hero) noexcept;
Context context() noexcept;
bool __cdecl read_ammo_context(void*, ap::Context& out);
bool __cdecl read_ammo_scope(void*, ap::Scope& out);
bool adapter_failed() noexcept;
ubc::CraftingKey key(const Context& c) noexcept;
bool same_owner(const Context& left, const Context& right) noexcept;
bool same_context(const Context& left, const Context& right) noexcept;
bool foreign_dialog(const Context& c) noexcept;
const basecat::Owner* owner_for(std::int32_t instance) noexcept;
bool linked(const ubc::SelectionInfo& info) noexcept;
bool linked_step_info(const ubc::SelectionInfo& info, const ubc::CraftingStep* step) noexcept;
std::uint32_t current_info_effect(const Context& c) noexcept;
bool navigation_contract() noexcept;
bool complete_collection(const Context& c, ubc::SelectionChoices* choice_out = nullptr) noexcept;
void initialize_end_compatibility() noexcept;
const ubc::zbug_end_compat::Binding* end_compatibility(bool predebit = false) noexcept;
ubc::station_end_contract::PathVerification end_path(std::uint32_t mob,
                                                     bool predebit = false) noexcept;
std::uint64_t next_extra_serial() noexcept;
xdebit::Context extra_context(const Context& c) noexcept;
xdep::Modules extra_modules(bool predebit = false) noexcept;
bool initialize_item_modules(bool predebit = false) noexcept;
bool bow_modules_current() noexcept;
bool matches_station_profile(std::uint32_t mob, cat::Station kind) noexcept;
bool common_bindings_verified() noexcept;

// Player contracts
#ifdef UBC_PLAYER_RELEASE
bool player_scripts_match(cat::Station station) noexcept;
#endif

// Stations
bool bow_candidate(const Context& c) noexcept;
bool bow_entry(const Context& c, ubc::BowMenuTrace& trace) noexcept;
bitem::Input bow_read_input(const ubc::CraftingKey& k) noexcept;
void initialize_bow(const Context& c) noexcept;
void initialize_extra(const Context& c) noexcept;
ubc::CraftingAlchemyExtraInput extra_read_input(const Context& c, bool boundary) noexcept;
std::uint32_t read_spirit_actual_info(void*, const ubc::CraftingAlchemyExtraInput& in) noexcept;
scat::Station station_kind(cat::Station kind) noexcept;
cnav::Page cool_info_page(const Context& c, bool reread) noexcept;
cnav::Page cool_page(const Context& c) noexcept;
ubc::CraftingPage page(const Context& c, ubc::BowMenuTrace* trace = nullptr) noexcept;
void capture_bow(Capture& captured, const Context& c, std::uint64_t serial, bool boundary,
                 ubc::BowMenuTrace* trace = nullptr) noexcept;
Capture capture(const Context& c, const GrindPreviewLease* shared_grind = nullptr,
                ubc::BowMenuTrace* trace = nullptr) noexcept;
const ubc::CraftingState* state_for(const ubc::CraftingSnapshot& snapshot,
                                    cat::RecipeId id) noexcept;
void observe_bow_quantity(const Context& c) noexcept;
void observe_cool_quantity(const Context& c) noexcept;

// Cooling
cpath::Verification cool_runtime_path(bool predebit = false) noexcept;
bool cool_engine_current() noexcept;
void initialize_cool(const Context& c) noexcept;
ubc::CoolRecipeCatalogueCapture capture_cool(const Context& c, bool boundary) noexcept;
bool verify_cool_amount(const bc::WriterScope& scope, const cat::Descriptor& recipe) noexcept;
void* execute_cool_predebit(bc::WriterScope& scope, const ubc::AmountMemory& writable);

// Grinding
ubc::GrindRecipeKey grind_key(const Context& c) noexcept;
bool grind_frame_capture_quiet() noexcept;
std::uint64_t next_grind_serial() noexcept;
bool grind_modules_current() noexcept;
void initialize_grind(const Context& c) noexcept;
bool unpack_grind_inventory(const Context& c) noexcept;
bool report_grind_preview(const GrindPreviewLease& value) noexcept;
void measure_grind_preview(const Context& c) noexcept;
ubc::CraftingSnapshot capture_grind(const Context& c, bool boundary,
                                    const GrindPreviewLease* shared = nullptr) noexcept;
ubc::CraftingGrindValues read_grind_result_values(const ubc::CraftingRequest& request) noexcept;
bool current_grind_choice(const Context& c) noexcept;
shnav::PageEvidence grind_page(const Context& c) noexcept;
void observe_grind_quantity(const Context& c, const GrindPreviewLease* shared = nullptr) noexcept;
bool verify_grind_amount(const bc::WriterScope& scope, const cat::Descriptor& recipe) noexcept;
shdestroy::Snapshot grind_destroy_snapshot(bool initial) noexcept;

// Diagnostics
void event(const char* name, int detail = 0, int quantity = 0, const char* fields = "") noexcept;
void log_grind_check(const ubc::GrindCheck& check, int subject) noexcept;
void log_bow_check(const char* field, int expected, int actual, int subject = 0, int quantity = 0,
                   std::uint32_t address = 0) noexcept;
void bow_menu_window(const char* field, const char* wanted, const char* observed, int ok,
                     int subject, int symbol) noexcept;
void bow_menu_rejected(const char* field, const char* wanted, const char* observed, int ok,
                       int subject, int symbol) noexcept;
void ui_diagnostic_events(const uidiag::Batch& batch) noexcept;
void grind_readiness_events(const gready::Batch& batch) noexcept;
void close_grind_readiness() noexcept;
void note_ammo_diagnostic(AmmoDiagnosticPhase phase) noexcept;
void observe_ammo_frame_health() noexcept;
void log_tobacco_check(const ubc::GrindCheck& check, int subject) noexcept;
void measure_bow_menu(const Context& c, std::uint64_t serial, ubc::BowMenuTrace& trace) noexcept;
void buffer_bow_value(const char* field, const char* wanted, const char* observed, int ok,
                      int subject, int symbol) noexcept;
void observe_bow_entry(const Context& c, bool candidate) noexcept;
void log_plugin_version(const char* name,
                        const ubc::windows_probe::LoadedModuleIdentity& module) noexcept;
void flush_amount_diagnostic(std::uint64_t info_token = 0) noexcept;
void log_cool_destroy_hand_issue(cdestroy::Issue issue) noexcept;

// Handoff
ubc::CraftingHandoffObservation observation(const Context& c, bool outer,
                                            bool materials = true) noexcept;
ubc::CraftingHandoffOffer resolve_offer(const Context& c, const ubc::CraftingStep* step) noexcept;
void close_ui(const Context& c) noexcept;
void borrow_view(const Context& c) noexcept;
void record_materials(const ubc::CraftingState& value, const char* name) noexcept;
void drive_handoff(const Context& c) noexcept;
void observe_result(const Context& c) noexcept;

// Menu
void __fastcall after_frame(void* game, void* manager);
void __fastcall observe_clear(void* view, void* unused);
void __fastcall observe_add(void* view, void* text, int number);
int __fastcall observe_menu_event(void* menu, void*, int input);
int __fastcall observe_choice_event(void* callback, void*, int input);

// Amount
const basecat::Callback* callback_for(const cat::Descriptor& recipe,
                                      std::uint32_t quantity) noexcept;
bool __cdecl batch_signature(const bc::Call& call) noexcept;
bool __cdecl reject_unexpected_cool_info(const bc::Call& call) noexcept;
bc::Decision __cdecl select_batch_call(const bc::Call& call) noexcept;
bool late_gate(ubc::GrindCheck* diagnostic = nullptr, tobacco::Checks* trace = nullptr) noexcept;
bool __cdecl extra_boundary(void*, xwriter::Phase phase, const xdebit::Job& job,
                            xwriter::Boundary& out);
bool __cdecl spirit_boundary(void*, gwriter::Phase phase, const gdebit::Job& job,
                             gwriter::Boundary& out);
bool amount_identity(void* raw, std::uint32_t address, std::size_t length) noexcept;
bool amount_writable(void*, std::uint32_t address, std::size_t size) noexcept;
bool amount_replace(void* raw, std::uint32_t address, const void* before, const void* after,
                    std::size_t size) noexcept;
void* __cdecl execute_amount(bc::WriterScope& scope);

// Tobacco
bool read_tobacco_state(const Context& c, const tobacco::Descriptor& d, bool script,
                        ubc::CraftingState& state, tobacco::Capture& sample,
                        ubc::GrindCheck& check) noexcept;
void trace_tobacco_entry(const Context& c, const tobacco::Descriptor& d, int symbol,
                         const ubc::CraftingRequest* request, const char* field, int expected,
                         int actual) noexcept;
void* execute_tobacco_amount(bc::WriterScope& scope, const tobacco::Descriptor& d);

// Hooks
ubc::CraftingHandoffCallbackProof proof(std::uintptr_t caller, bool info) noexcept;
void __fastcall observe_update(void* manager, void* unused);
void* __fastcall observe_resolve(void* manager, void*, void* player, void* npc, int row);
void __fastcall observe_info(void* info, void*);
void __fastcall observe_choice(void* manager, void* choice);
void __fastcall observe_start(void* mob, void*, void* npc);
void __fastcall observe_end(void* mob, void*, void* npc, int flag);

} // namespace ubc_native
