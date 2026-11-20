// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/core/crafting_result.hpp"
#include "ubc/stations/grind/grind_destroy_observation.hpp"
#include "ubc/stations/station_end_contract.hpp"

namespace ubc {
// Reads only the value copy that the result watcher just discarded.
inline void diagnose_grind_skill(GrindCheck& check, const grind_destroy_observation::Prepared& prepared,
    const CraftingGrindValues& actual) noexcept {
  check.equal("skill.known",true,actual.known);
  if (!actual.known) return;
  check.equal("skill.talent",prepared.before.talent,actual.talent);
  check.equal("skill.guild",prepared.before.guild,actual.guild);
  check.equal("skill.value",prepared.amount.skill_after,actual.skill);
}

inline GrindCheck diagnose_grind_end(StationEndIssue issue, const CraftingKey& key, const StationEndCall& call) noexcept {
  GrindCheck check;
  using I=StationEndIssue;
  if (issue==I::state) {
    const auto& before=call.before;
    check.equal("end.mob_fields",true,before.mob_fields_known);
    check.equal("end.state",0,before.state);
    check.equal("end.target",0,before.target);
    check.equal("end.state_count",1,before.state_num);
    check.equal("end.rewind",0,before.rewind);
  } else if (issue==I::attachment) {
    check.equal("end.station_before",key.station,call.before.station);
    check.equal("end.station_after",0,call.after.station);
  } else if (issue==I::path) {
    check.equal("end.original_path",true,call.original_path_verified);
  } else if (issue==I::caller) {
    check.equal("end.flag",0,call.flag);
    check.equal("end.caller",station_end_contract::completed_return,call.return_address);
  } else if (issue==I::scope) {
    check.equal("end.returned",true,call.original_returned);
    check.equal("end.owner_scope",true,call.owner_thread_depth_verified);
  } else if (issue==I::profile) {
    check.equal("end.owned",true,call.before.station_owned);
    check.equal("end.profile",true,call.before.station_profile_verified);
    check.equal("end.kind",static_cast<int>(key.kind),static_cast<int>(call.before.kind));
  } else if (issue==I::binding) {
    check.equal("end.mob",key.station,call.mob);
    check.equal("end.hero",key.hero,call.npc);
  } else {
    for (const auto* p : {&call.before,&call.after}) {
      if (issue==I::read) {
        check.equal("end.stable",true,p->read_stable);
        check.equal("end.identity_known",true,p->identity_known);
        check.equal("end.station_known",true,p->station_known);
        check.equal("end.production_known",true,p->production_known);
        check.equal("end.hands_known",true,p->hands_known);
      } else if (issue==I::identity) {
        check.equal("end.thread",key.thread,p->thread);
        check.equal("end.world",key.world,p->world);
        check.equal("end.hero",key.hero,p->hero);
      } else if (issue==I::production) {
        check.equal("end.production",0,p->production);
      } else if (issue==I::hands) {
        check.equal("end.anchor_known",true,p->hands.interact_item_known);
        check.equal("end.anchor",0,p->hands.interact_item);
        for (const auto* hand : {&p->hands.hands.left,&p->hands.hands.right}) {
          check.equal("hand.state",0,hand->state);
          check.equal("hand.object",0,hand->object);
          check.equal("hand.symbol",-1,hand->symbol);
          check.equal("hand.amount",-1,hand->amount);
          if (check.field) break;
        }
      }
      if (check.field) {
        check.point=p==&call.before?"before_end":"after_end";
        break;
      }
    }
  }
  if (!check.field) check.equal("end.issue",static_cast<int>(I::none),static_cast<int>(issue));
  return check;
}

inline GrindCheck diagnose_grind_result(CraftingResultIssue issue, const grind_destroy_observation::Prepared& prepared,
    const CraftingResultObservation& value, std::uint64_t elapsed_ms) noexcept {
  GrindCheck check;
  if (issue == CraftingResultIssue::stock) {
    grind_destroy_observation::Stocks observed;
    if (check.equal("result.stock_known",true,value.stock_known) &&
        check.equal("result.stock_data",true,value.items.data!=nullptr) &&
        check.equal_unsigned("result.stock_count",observed.stocks.size(),value.items.count)) {
      observed.stable_boundary_verified=true;
      for (std::size_t i=0;i<observed.stocks.size();++i)
        observed.stocks[i]={value.items[i].symbol,value.items[i].total,true,false};
      grind_destroy_observation::detail::stock_matches(observed,prepared.amount.after_end,check,"result");
    }
  } else if (issue == CraftingResultIssue::skill) {
    diagnose_grind_skill(check,prepared,value.grind);
  } else if (issue == CraftingResultIssue::context) {
    if (value.identity_known) {
      check.equal("result.world",prepared.key.world,value.world);
      check.equal("result.hero",prepared.key.hero,value.hero);
    }
    if (value.station_known && value.station) check.equal("result.station",prepared.key.station,value.station);
    if (value.production_known && value.production) check.equal("result.production",prepared.key.production_kind,value.production);
  } else if (issue == CraftingResultIssue::hands) {
    const auto& hands=value.hands.hands;
    if (hands.left.state!=-1) {
      check.equal("result.left_state",0,hands.left.state);
      check.equal("result.left_object",0,hands.left.object);
    }
    if (hands.right.state!=-1) {
      check.equal("result.right_state",0,hands.right.state);
      check.equal("result.right_object",0,hands.right.object);
    }
  } else if (issue == CraftingResultIssue::anchor) {
    check.equal("result.anchor",0,value.hands.interact_item);
  } else if (issue == CraftingResultIssue::timeout) {
    check.equal_unsigned("result.maximum_ms",30000,elapsed_ms);
  } else if (issue == CraftingResultIssue::contract) {
    check.equal_unsigned("result.generation",prepared.job,value.generation);
  }
  if (!check.field) check.equal("result.issue",static_cast<int>(CraftingResultIssue::none),static_cast<int>(issue));
  return check;
}
}
