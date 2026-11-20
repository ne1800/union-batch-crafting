// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/spirit/spirit_execution.hpp"
#include "ubc/stations/spirit/spirit_result.hpp"
#include "ubc/engine/batch_call_bridge.hpp"
#include "ubc/stations/spirit/spirit_flask_runtime.hpp"
#include "../bridges/spirit_flask_debit_bridge.hpp"
#if defined(_WIN32) && defined(_M_IX86)
#include <windows.h>

namespace ubc::spirit_writer {
namespace debit=spirit_debit;
namespace runtime=alchemy_flask_runtime;
enum class Phase { before_original,after_original,before_debit,after_debit,restore_targets,item_targets };
// On each call, Root supplies the current TLS, writer and physical context
// boundary and a monotonic serial. No engine calls, UI or allocation. Before
// the original call, also require that the currently selected actual_info
// is our exact bound effect from the full normal InfoManager handoff.
struct Boundary {
  runtime::Boundary runtime{};
  bool native_handoff_verified=false;
  std::uint32_t actual_info=0;
  alchemy_extra_dependencies::Modules modules{};
};
using ReadBoundary=bool(__cdecl*)(void*,Phase,const debit::Job&,Boundary&);
struct Config {
  std::uint32_t owner_thread=0;
  ReadOnlyMemory memory{};
  void* context=nullptr;
  ReadBoundary boundary=nullptr;
  spirit_flask_debit_bridge::Remove remove=nullptr;
  spirit_flask_debit_bridge::RestorePop restore_pop=nullptr;
  spirit_flask_debit_bridge::RestoreInstance restore_instance=nullptr;
  spirit_flask_debit_bridge::RestoreItem restore_item=nullptr;
#if defined(UBC_SPIRIT_HOST_TEST)
  // Isolated test binary only. Our own native pointers and synthetic objects verify
  // wrapper control flow. They do not establish actual loader or mod identity.
  bool(__cdecl* host_dependencies)(void*,Phase,const debit::Job&)=nullptr;
#endif
};
struct Request {
  SpiritRecipeCapture snapshot{};
  std::uint64_t snapshot_serial=0;std::uint32_t quantity=0;
  debit::Job job{};
  std::uint32_t actual_info=0;
};
enum class RestoreProof { pending,not_required_for_one,verified };
struct Outcome {
  bool original_entered=false,original_returned=false,operands_restored=false;
  bool closing=false,blocked=false; // closing explicitly does NOT indicate successful completion.
  AmountIssue amount_issue=AmountIssue::none;
  runtime::Issue read_issue=runtime::Issue::none;
  alchemy_extra_dependencies::Issue dependency_issue=alchemy_extra_dependencies::Issue::none;
  RestoreProof vm_restore=RestoreProof::pending,item_restore=RestoreProof::pending;
};
struct Diagnostics {bool ready=false,active=false,disabled=false;};
struct ScopeEvidence {bool valid=false;debit::Scope scope{};};

// No installer, hook or direct fixed engine pointers. Keep one executor
// with its result, reader and debit bridge for all orders. Never recreate
// it after an uncertain effect. run() MUST execute inside the existing
// exclusive batch_call writer. An independent writer is forbidden.
// Root supplies the shared AmountPatch/memory adapter. Its identity and
// write guards remain in place during the original call and restore.
class Executor {
public:
  explicit Executor(const Config&) noexcept;
  Executor(const Executor&)=delete;
  Executor& operator=(const Executor&)=delete;
  Diagnostics diagnostics() const noexcept;
  // Only the owner thread receives order fields. Native debit depth comes
  // from the bridge lease actually entered, VM depth from the original
  // call. The integrator must still bind the outer batch_call writer.
  ScopeEvidence scope_evidence() const noexcept;
  void disable() noexcept;
  void timeout() noexcept;
  const Outcome& outcome() const noexcept {return outcome_;} // Owner thread only.
  SpiritResultStatus status() const noexcept {return unavailable()?SpiritResultStatus{SpiritResultStage::blocked,debit::Issue::bridge}:result_.status();}
  const SpiritResultReceipt& receipt() const noexcept {return result_.receipt();}
  // Only forward the original return pointer. Never dereference or free it.
  // Preserve engine SEH and quarantine any uncertain effects of our own.
  void* run(batch_call::WriterScope&,AmountPatch&,const AmountMemory&,const Request&);
  SpiritResultStage observe_completion(const debit::Observation&,const debit::EndWitness&,bool normal_end_verified) noexcept;
private:
  bool boundary(Phase,Boundary&) noexcept;
  bool capture(Phase,debit::Observation&) noexcept;
  bool restore_targets(const debit::Call&,debit::RestoreObservation&) noexcept;
  bool item_targets(const debit::Call&,spirit_flask_debit_bridge::ItemObservation&) noexcept;
  alchemy_extra_dependency_reader::Input dependency_input(const Boundary&) const noexcept;
  bool entry(spirit_execution::Entry&) noexcept;
  bool local_restored() noexcept;
  static bool __cdecl read_debit(void*,spirit_flask_debit_bridge::ReadStage,debit::Observation&);
  static bool __cdecl read_restore(void*,const debit::Call&,debit::RestoreObservation&);
  static bool __cdecl read_item(void*,const debit::Call&,spirit_flask_debit_bridge::ItemObservation&);
  static void* __cdecl original(void*);
  bool unavailable() const noexcept;
  Config config_{};
  bool ready_=false;
  mutable volatile LONG active_=0,disabled_=0;
  spirit_flask_runtime::Reader reader_;
#if defined(UBC_SPIRIT_HOST_TEST)
  spirit_flask_runtime::CoreReader host_reader_;
#endif
  SpiritResult result_{};
  spirit_flask_debit_bridge::Executor debit_;
  Request request_{};
  const spirit_recipe_catalog::Descriptor* recipe_=nullptr;
  batch_call::WriterScope* scope_=nullptr;
  debit::Observation debit_after_{};
  bool debit_after_read_=false;
  alignas(16) unsigned char original_fx_[512]{};
  bool original_fx_captured_=false;
  bool original_active_=false;
  Outcome outcome_{};
};
} // namespace ubc::spirit_writer
#endif
