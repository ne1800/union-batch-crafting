// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_recipe_plan.hpp"
#include <utility>
namespace ubc::cool_recipe_navigation {
namespace cat=cool_recipe_catalog;
struct Offer {std::uint32_t owner=0,condition=0,function=0;};
struct Page {bool valid=false,single=false,triple=false;};
// Own copied IDs from a fresh, verified INFO-Collection. Manager.info
// may be null/old and is explicitly NOT used here. The later
// effect must re-bind the real observe_info(info) argument via
// the native Lease; this value plan produces no such Lease.
inline Page identify_page(std::int32_t production,const Offer* offers,std::size_t count,bool collection_verified) noexcept {
  Page p;if(!collection_verified||production!=13||!offers||!count||count>3)return p;bool end=false;
  for(std::size_t i=0;i<count;++i){const auto& o=offers[i];
    if(o.owner==20770&&o.condition==20771&&o.function==20772){if(end)return {};end=true;}
    else if(o.owner==20773&&o.condition==20774&&o.function==20775){if(p.single)return {};p.single=true;}
    else if(o.owner==20776&&o.condition==20777&&o.function==20778){if(p.triple)return {};p.triple=true;}
    else return {};
  }
  p.valid=end&&(!p.triple||p.single);return p.valid?p:Page{};
}
struct Step {recipe_catalog::Invocation invocation=recipe_catalog::Invocation::info;std::uint32_t owner=0,function=0;};
struct Plan {bool valid=false;std::uint32_t quantity=0;std::size_t count=0;Step step{};cool_recipe::Plan amount{};};
inline Plan make_plan(const cat::Descriptor& d,const cool_recipe::Evidence& stock,std::uint32_t n,const Page& page) noexcept {
  Plan p;p.quantity=n;if(!page.valid||(page.triple&&!page.single))return p;p.amount=cool_recipe::plan(d,stock,n);if(!p.amount.valid)return p;
  if(p.amount.requires_extra_debit&&page.triple)return p; // Triplet condition contradicts Hot2.
  if((p.amount.owner==20773&&!page.single)||(p.amount.owner==20776&&!page.triple))return p;
  p.step={recipe_catalog::Invocation::info,p.amount.owner,p.amount.callback};p.count=1;p.valid=true;return p;
}
inline bool valid_plan(const Plan& p,const cat::Descriptor& d,const cool_recipe::Evidence& stock,const Page& page) noexcept {
  const auto expected=make_plan(d,stock,p.quantity,page);if(!p.valid||!expected.valid||p.count!=1||p.step.invocation!=expected.step.invocation||p.step.owner!=expected.step.owner||p.step.function!=expected.step.function)return false;
  const auto& a=p.amount;const auto& b=expected.amount;
  if(!a.valid||!a.known||a.issue!=b.issue||a.quantity!=b.quantity||a.maximum!=b.maximum||a.direct_maximum!=b.direct_maximum||a.callback!=b.callback||a.owner!=b.owner||a.count!=b.count||a.extra_debit!=b.extra_debit||a.native_end_debit!=b.native_end_debit||a.requires_extra_debit!=b.requires_extra_debit||a.requires_quantity_receipt!=b.requires_quantity_receipt)return false;
  for(std::size_t i=0;i<a.count;++i){const auto& x=a.operands[i];const auto& y=b.operands[i];if(x.offset!=y.offset||x.original!=y.original||x.units!=y.units||x.reserved!=y.reserved||x.allow_zero!=y.allow_zero)return false;}
  for(std::size_t i=0;i<2;++i)for(const auto pair:{std::pair<const cool_recipe::Stock*,const cool_recipe::Stock*>{&a.after_callback[i],&b.after_callback[i]},{&a.before_end[i],&b.before_end[i]},{&a.after_end[i],&b.after_end[i]}})
    if(pair.first->symbol!=pair.second->symbol||pair.first->amount!=pair.second->amount||pair.first->known!=pair.second->known||pair.first->unambiguous!=pair.second->unambiguous)return false;
  return true;
}
} // namespace ubc::cool_recipe_navigation
