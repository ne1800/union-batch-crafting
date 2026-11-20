// SPDX-License-Identifier: MIT
#pragma once
#if !defined(UBC_COOL_PREDEBIT_HOST_TEST)
#error Nur fuer den privaten x86-Host.
#endif
#include "cool_predebit_bridge.hpp"
#include "ubc/crafting_handoff.hpp"
#include "ubc/crafting_info_effect.hpp"
#include "ubc/cool_native_path.hpp"
#include "ubc/cool_destroy_observation.hpp"
#include "ubc/selection_abi.hpp"

namespace cool_host {
namespace debit=ubc::cool_predebit;
namespace bridge=ubc::cool_predebit_bridge;
// The same owners used by observe_info, execute_amount and the result guard.
struct State {
  ubc::CraftingHandoff& handoff;
  ubc::SelectionInfoRows& rows;
  ubc::CraftingResult& result;
  debit::admission::InfoLease& lease;
  debit::Tracker& tracker;
  std::optional<bridge::Executor>& executor;
  ubc::AmountPatch& patch;
};
State state();
struct Originals {
  void (__thiscall* info)(void*);
  ubc::selection_abi::ResolveInfo resolve;
  ubc::batch_call::Original script;
  int (__thiscall* destroy)(void*,void*);
  void (__thiscall* end)(void*,void*,int);
  void* (__thiscall* remove_item)(void*,int,int);
};
constexpr std::uint32_t destroy_trampoline=0x2f100100,end_trampoline=0x2f100300;
ubc::destroy_remove_callsite::Plan remove_patch();
using Info=void(__fastcall*)(void*,void*);
using Resolve=void*(__fastcall*)(void*,void*,void*,void*,int);
struct Entries {Info info;Resolve resolve;std::uint32_t destroy,end;};
Entries entries();
bool bind(const bridge::Config&,const ubc::alchemy_extra_dependencies::Modules&,
          const Originals&,ubc::batch_call::Writer writer);
ubc::cool_native_path::Verification preview_path();
bool dispatch(std::uint64_t frame);
void enter_update(bool active);
void set_event_log(HANDLE destination);
void set_context_diagnosis(bool enabled);
void set_focus(bool focused);
std::uint32_t current_info();
std::uint64_t info_token();
void* native_writer(ubc::batch_call::WriterScope&);
std::uint64_t last_serial();
const ubc::cool_destroy_observation::Observation& observed_destroy();
const ubc::cool_destroy_observation::Binding& destroy_binding();
void next_frame();
bool result_frame();
std::uint64_t submit_notice(bool focus=true,bool owns_input=true,bool lose_focus_on_render=false);
bool next_handoff();
}
