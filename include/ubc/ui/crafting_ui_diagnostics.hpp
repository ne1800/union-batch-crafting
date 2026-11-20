// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/ui/crafting_menu.hpp"
#include <array>
#include <cstdint>
#include <climits>

namespace ubc::crafting_ui_diagnostics {
// Only own UI model values. No inventory-/engine access and
// neither execution release nor proof of physical key delivery.
struct State {
  CraftingMenuRow row=CraftingMenuRow::recipe;
  SelectionPhase phase=SelectionPhase::inactive;
  SelectionIssue issue=SelectionIssue::none;
  CraftingBlockReason reason=CraftingBlockReason::none;
  bool armed=false,can_confirm=false;
  std::uint32_t quantity=0,recipe=UINT32_MAX;
};
inline State state(const CraftingMenu& menu,const CraftingSession& session) noexcept {
  const auto* selected=session.selected();
  return {menu.row(),session.phase(),session.issue(),session.reason(),session.armed(),
    session.can_confirm(),session.quantity(),selected?selected->id:UINT32_MAX};
}
struct Action {
  bool observed=false,focused=false,accepted=false;
  int key=0;
  CraftingMenuAction action=CraftingMenuAction::none;
  State before{},after{};
};
// Existing UI decision, exactly once. focused comes from the already
// necessary focus read; action is not natively released on focus loss.
inline Action dispatch(CraftingMenu& menu,CraftingSession& session,int key,
    CraftingMenuAction action,bool focused) noexcept {
  Action out;out.observed=true;out.focused=focused;out.key=key;out.action=action;
  out.before=state(menu,session);
  if(!focused)session.observe(0,false,false);
  else out.accepted=menu.on_action(action,session);
  out.after=state(menu,session);return out;
}
// Bits: row[0:2], phase[3:5], armed6, can_confirm7, reason[8:12],
// issue[13:17], focused18, held19, poll_known20. held is set/known only when
// poll_known is set. The action does not reread key state.
inline int flags(const State& s,bool focused,bool held=false,bool poll_known=false) noexcept {
  return static_cast<int>((static_cast<unsigned>(s.row)&7u)|((static_cast<unsigned>(s.phase)&7u)<<3u)|
    (unsigned{s.armed}<<6u)|(unsigned{s.can_confirm}<<7u)|((static_cast<unsigned>(s.reason)&31u)<<8u)|
    ((static_cast<unsigned>(s.issue)&31u)<<13u)|(unsigned{focused}<<18u)|
    (unsigned{held&&poll_known}<<19u)|(unsigned{poll_known}<<20u));
}
inline int word(std::uint32_t value) noexcept {
  return value<=INT32_MAX?static_cast<int>(value):-1-static_cast<int>(UINT32_MAX-value);
}
inline int milliseconds(std::uint64_t begin,std::uint64_t end) noexcept {
  if(end<begin)return -1;const auto value=end-begin;
  return value>INT32_MAX?INT32_MAX:static_cast<int>(value);
}
struct Record {const char* name=nullptr;int detail=0,quantity=0;};
struct Batch {std::array<Record,4> records{};std::size_t count=0;};
struct Frame {
  State before{},observed{},refreshed{};
  bool focused=false,held=false;
  std::uint64_t started=0,capturing=0,captured=0,checked=0;
};
// Times are monotonic OS ticks in ms; Frames are supplied solely by the
// existing outer Native caller in event(). No time-/frame values drive the UI.
class Trace {
public:
  static constexpr unsigned max_actions=64,max_frames=96,max_records=2048;
  Batch begin(std::uint64_t epoch,std::uint64_t now) noexcept {
    if(!epoch||epoch<=epoch_)return {};
    epoch_=epoch;active_=true;actions_=frames_=0;action_limit_=frame_limit_=false;previous_=now;
    Batch out;if(!reserve(1,out))return out;
    out.records[0]={"ui_diag_epoch",word(static_cast<std::uint32_t>(epoch)),word(static_cast<std::uint32_t>(epoch>>32u))};
    out.count=1;return out;
  }
  void end() noexcept {active_=false;}
  Batch input(std::uint64_t epoch,const Action& value) noexcept {
    Batch out;if(!active_||epoch!=epoch_||!value.observed)return out;
    if(!admit(actions_,max_actions,action_limit_,1,out))return out;
    out.records={{{"ui_diag_input",value.key,static_cast<int>(value.action)|(value.accepted?256:0)},
      {"ui_diag_input_state",flags(value.before,value.focused),flags(value.after,value.focused)},
      {"ui_diag_input_quantity",word(value.before.quantity),word(value.after.quantity)},
      {"ui_diag_input_recipe",word(value.before.recipe),word(value.after.recipe)}}};out.count=4;return out;
  }
  Batch frame(std::uint64_t epoch,const Frame& value) noexcept {
    Batch out;if(!active_||epoch!=epoch_)return out;
    const auto gap=milliseconds(previous_,value.started);previous_=value.started;
    if(!admit(frames_,max_frames,frame_limit_,2,out))return out;
    out.records={{{"ui_diag_poll",flags(value.before,value.focused,value.held,true),flags(value.observed,value.focused,value.held,true)},
      {"ui_diag_refresh",flags(value.refreshed,value.focused,value.held,true),word(value.refreshed.quantity)},
      {"ui_diag_time",milliseconds(value.capturing,value.captured),milliseconds(value.captured,value.checked)},
      {"ui_diag_frame_gap",gap,0}}};out.count=4;return out;
  }
  unsigned records() const noexcept {return records_;}
private:
  bool reserve(unsigned count,Batch& out) noexcept {
    if(global_limit_)return false;
    // Get the last record for the explicit global limit message.
    if(records_+count>=max_records){global_limit_=true;
      out.records[0]={"ui_diag_limit",3,static_cast<int>(records_)};out.count=1;++records_;return false;}
    records_+=count;return true;
  }
  bool admit(unsigned& used,unsigned limit,bool& reported,int kind,Batch& out) noexcept {
    if(used==limit){if(!reported){reported=true;if(reserve(1,out)){
        out.records[0]={"ui_diag_limit",kind,static_cast<int>(limit)};out.count=1;}}
      return false;}
    if(!reserve(4,out))return false;++used;return true;
  }
  std::uint64_t epoch_=0,previous_=0;
  unsigned actions_=0,frames_=0,records_=0;
  bool active_=false,action_limit_=false,frame_limit_=false,global_limit_=false;
};
} // namespace ubc::crafting_ui_diagnostics
