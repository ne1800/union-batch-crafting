// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/cool_recipe_navigation.hpp"
#include "ubc/cool_recipe_snapshot.hpp"
#include "ubc/selection_snapshot.hpp"

namespace ubc::cool_direct_contract {
namespace cat=cool_recipe_catalog;
namespace nav=cool_recipe_navigation;
// Direct value and INFO binding only. The native adapter must still verify
// the actual thread, lease, hand, destroy and end path. No new writer,
// extra debit, end-state machine or self-generated EndFence evidence.
inline constexpr bool runtime_ready=false;
enum class Issue {none,key,capture,material,hand,quantity,extra_debit_unsupported,navigation,changed,info};
struct Requirements {
  bool positive_destroy_after=false,normal_end_after=false,quantity_receipt=false;
  std::uint32_t native_end_debit=0;
};
struct Prepared {
  bool valid=false,stock_known=false;
  Issue issue=Issue::none;
  CoolRecipeKey key{};
  StationRecipeHands hands{};
  cool_recipe::Evidence stocks{};
  nav::Plan path{};
  std::uint32_t maximum=0,direct_maximum=0;
  // These obligations are not fulfilled by valid=true. In particular,
  // PrintReturn1 and the fixed OriginalText3 are not quantity receipts.
  Requirements requirements() const noexcept {
    return valid?Requirements{true,true,path.amount.requires_quantity_receipt,1}:Requirements{};
  }
  AmountBody body() const & noexcept {return valid?path.amount.body():AmountBody{};}
  AmountBody body() const &&=delete;
};
namespace detail {
inline bool valid_key(const CoolRecipeKey& k) noexcept {
  return k.generation&&k.thread&&k.world&&k.hero&&k.station&&k.manager&&k.production_kind==13;
}
inline bool same_key(const CoolRecipeKey& a,const CoolRecipeKey& b) noexcept {
  return a.generation==b.generation&&a.thread==b.thread&&a.world==b.world&&a.hero==b.hero&&
    a.station==b.station&&a.manager==b.manager&&a.production_kind==b.production_kind;
}
inline bool active_hand(const StationRecipeHands& h) noexcept {
  const auto& l=h.hands.left;const auto& r=h.hands.right;
  return h.left_slot_known&&h.right_slot_known&&h.interact_item_known&&
    l.state==1&&l.object&&l.symbol==16612&&l.amount==1&&r.state==0&&!r.object&&
    r.symbol==-1&&r.amount==-1&&h.interact_item==l.object&&
    (h.left_tmp_level==1||h.left_tmp_level==2);
}
inline bool same_hand(const StationRecipeHands& a,const StationRecipeHands& b) noexcept {
  return active_hand(a)&&active_hand(b)&&a.hands.left.object==b.hands.left.object&&
    a.left_tmp_level==b.left_tmp_level&&a.right_tmp_level==b.right_tmp_level;
}
inline Issue captured(const CoolRecipeCatalogueCapture& c,cool_recipe::Evidence& stock) noexcept {
  if(!valid_key(c.key))return Issue::key;
  const auto& s=c.recipe;const auto& d=c.diagnostic;const auto& m=s.material;
  if(s.id!=cat::RecipeId::blade||!s.evaluated||!s.known||!s.contract||!s.actor_known||!s.actor_matches||
    !s.context_known||!s.context_matches||d.issue!=CoolCaptureIssue::none||
    d.inventory_issue!=PackedInventoryIssue::none||d.hand_issue!=StationHandReadIssue::none||
    !d.inventory_unchanged||!d.hands_unchanged||!d.actor_unchanged||!d.context_unchanged||
    !d.knowledge_unchanged||!d.profile.valid||!d.profile.local.matches)return Issue::capture;
  if(!m.known())return Issue::material;
  if(!active_hand(c.hands))return Issue::hand;
  stock.stable_boundary_verified=true;
  for(std::size_t i=0;i<2;++i){
    const auto& row=m.rows[i];
    if(row.symbol!=cat::targets[i].symbol||!row.available_known||row.stored<0||row.stored>INT32_MAX||
      row.available!=row.stored||row.reserved||row.held!=(i?0:1)||
      row.unit_cost!=(i?0u:1u)||row.unit_output!=(i?1u:0u))return Issue::material;
    stock.stocks[i]={row.symbol,row.stored,true,true};
  }
  const auto numerical=cool_recipe::inspect(stock);
  if(!numerical.known||m.maximum!=numerical.maximum||m.direct_maximum!=numerical.direct_maximum)
    return Issue::material;
  return Issue::none;
}
inline bool same_stocks(const cool_recipe::Evidence& a,const cool_recipe::Evidence& b) noexcept {
  if(!a.stable_boundary_verified||!b.stable_boundary_verified)return false;
  for(std::size_t i=0;i<2;++i){const auto& x=a.stocks[i];const auto& y=b.stocks[i];
    if(x.symbol!=y.symbol||x.amount!=y.amount||x.known!=y.known||x.unambiguous!=y.unambiguous)return false;}
  return true;
}
inline bool matches(const SelectionInfo& i,const recipe_catalog::Owner& o) noexcept {
  return i.valid&&!i.first_choice&&i.instance==o.instance&&i.npc==o.npc&&i.number==o.number&&
    i.important==static_cast<int>(o.important)&&i.trade==static_cast<int>(o.trade)&&
    i.permanent==static_cast<int>(o.permanent)&&i.condition==static_cast<std::int32_t>(o.condition->symbol)&&
    i.function==static_cast<std::int32_t>(o.information->symbol);
}
}
// The unchanged CoolRecipeCatalogueReader supplies the snapshot. maximum
// remains the mathematical quantity and direct_maximum identifies
// the available original paths. Reject Lager2/N2 with its own reason.
inline Prepared prepare(const CoolRecipeCatalogueCapture& capture,const nav::Page& page,std::uint32_t quantity) noexcept {
  Prepared p;p.key=capture.key;p.hands=capture.hands;
  p.issue=detail::captured(capture,p.stocks);if(p.issue!=Issue::none)return p;
  const auto numerical=cool_recipe::plan(cat::recipe,p.stocks,quantity);
  p.stock_known=true;p.maximum=numerical.maximum;p.direct_maximum=numerical.direct_maximum;
  if(!numerical.valid){p.issue=Issue::quantity;return p;}
  if(numerical.requires_extra_debit||numerical.extra_debit){p.issue=Issue::extra_debit_unsupported;return p;}
  p.path=nav::make_plan(cat::recipe,p.stocks,quantity,page);
  if(!p.path.valid||!nav::valid_plan(p.path,cat::recipe,p.stocks,page)){
    p.issue=Issue::navigation;return p;
  }
  p.valid=true;return p;
}
struct Entry {
  bool valid=false;
  Issue issue=Issue::none;
  Prepared prepared{};
  std::uint32_t actual_info=0;
  AmountBody body() const & noexcept {return valid?prepared.body():AmountBody{};}
  AmountBody body() const &&=delete;
};
// actual_info must come from the current own observe_info(info) lease,
// not from Manager.info or the UI line number. This pure value/read
// checker produces neither a lease nor a thread or code hook evidence.
// late is a newly bound catalogue intake of the same original boundary.
inline Entry bind(const ReadOnlyMemory& memory,const Prepared& requested,
    const CoolRecipeCatalogueCapture& late,const nav::Page& page,std::uint32_t actual_info) noexcept {
  Entry e;
  if(!requested.valid||!requested.stock_known||requested.issue!=Issue::none||!requested.path.valid){
    e.issue=Issue::capture;return e;
  }
  if(requested.path.amount.requires_extra_debit||requested.path.amount.extra_debit){
    e.issue=Issue::extra_debit_unsupported;return e;
  }
  const auto fresh=prepare(late,page,requested.path.quantity);
  if(!fresh.valid){e.issue=fresh.issue;return e;}
  if(!detail::same_key(requested.key,fresh.key)||!detail::same_hand(requested.hands,fresh.hands)||
    !detail::same_stocks(requested.stocks,fresh.stocks)||requested.maximum!=fresh.maximum||
    requested.direct_maximum!=fresh.direct_maximum||
    !nav::valid_plan(requested.path,cat::recipe,fresh.stocks,page)){
    e.issue=Issue::changed;return e;
  }
  if(!actual_info||actual_info%4u||actual_info>UINT32_MAX-0x5cu){e.issue=Issue::info;return e;}
  const auto* owner=fresh.path.amount.owner==20773?&cat::single:
    fresh.path.amount.owner==20776?&cat::triple:nullptr;
  if(!owner||!detail::matches(read_selection_info(memory,actual_info),*owner)||
    !detail::matches(read_selection_info(memory,actual_info),*owner)){
    e.issue=Issue::info;return e;
  }
  e.prepared=fresh;e.actual_info=actual_info;e.valid=true;return e;
}
} // namespace ubc::cool_direct_contract
