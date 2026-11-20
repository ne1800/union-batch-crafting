// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/crafting_handoff.hpp"
#include "ubc/cool_predebit_admission.hpp"

namespace ubc {
// The original Info()-parameter remains bound only during its own
// Callback. OnInfo sets Manager.info only after return, if Choices arose.
struct CraftingInfoEffectLease {
  std::uint32_t actual_info=0,thread=0,game=0;
  unsigned depth=0;
  CraftingKey key{};
  CraftingHandoffTicket ticket{};
  std::uint64_t token=0;

  std::uint32_t current(const ReadOnlyMemory& memory,const cool_predebit::admission::InfoLease& owner,
      const cool_predebit::admission::InfoIdentity& now,const CraftingHandoff& handoff,
      const CraftingHandoffTicket& dispatched,const SelectionInfoRows& rows,const SelectionDialog& dialog,
      const recipe_catalog::Owner* descriptor) const noexcept {
    const auto* step=handoff.step();
    const auto* active=handoff.current_ticket();
    const auto& c=now.context;
    if(depth!=1||actual_info!=now.actual_info||!owner.current(now,token)||!descriptor||
       handoff.cancelled()||handoff.state()!=CraftingHandoffState::executing||!step||!step->is_effect()||
       step->invocation!=recipe_catalog::Invocation::info||!active||
       !crafting_handoff_detail::same_ticket(ticket,*active)||!crafting_handoff_detail::same_ticket(ticket,dispatched)||
       !(key==handoff.request().key)||key.generation!=c.epoch||key.thread!=thread||thread!=c.thread||
       game!=c.game||key.world!=c.world||key.hero!=c.hero||key.station!=c.mob||key.manager!=now.manager||
       !ticket.offer.valid||ticket.offer.mode!=step->invocation||ticket.offer.collection!=rows.generation||
       ticket.offer.owner!=step->owner_instance||ticket.offer.function!=step->function_symbol||
       descriptor->instance!=ticket.offer.owner||!dialog.valid||dialog.mode!=1||dialog.view!=ticket.view||
       dialog.npc!=c.hero||dialog.player!=c.hero)return 0;
    if(key.kind==crafting_catalog::Station::cool&&handoff.request().quantity==cool_predebit::quantity&&
       step->function_symbol==cool_predebit::callback){
      const auto row=ticket.offer.row;
      if(!rows.valid||rows.view!=ticket.view||row<0||static_cast<std::size_t>(row)>=rows.count||rows.count>rows.rows.size()||
         rows.rows[static_cast<std::size_t>(row)].address!=actual_info)return 0;
    }
    const auto actual=read_selection_info(memory,actual_info);
    return cool_direct_contract::detail::matches(actual,*descriptor)&&actual.function==ticket.offer.function?actual_info:0;
  }
};
} // namespace ubc
