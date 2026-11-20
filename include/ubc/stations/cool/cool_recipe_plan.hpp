// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/cool_recipe_catalog.hpp"

namespace ubc::cool_recipe {
namespace cat=cool_recipe_catalog;
struct Stock {std::int32_t symbol=-1;std::int64_t amount=-1;bool known=false,unambiguous=false;};
struct Evidence {bool stable_boundary_verified=false;std::array<Stock,2> stocks{};};
enum class Issue {none,descriptor,observation,stock,quantity,lifecycle_room};
struct Plan {
  Issue issue=Issue::none;bool known=false,valid=false;
  std::uint32_t quantity=0,maximum=0,direct_maximum=0,callback=0,owner=0;
  std::uint32_t extra_debit=0,native_end_debit=0;
  bool requires_extra_debit=false,requires_quantity_receipt=false;
  std::array<Stock,2> after_callback{},before_end{},after_end{};
  std::array<AmountOperand,2> operands{};std::size_t count=0;
  AmountBody body() const noexcept {
    const auto* f=callback==20775?cat::single.information:callback==20778?cat::triple.information:nullptr;
    return valid&&f&&count<=operands.size()?AmountBody{f->byte_length,f->body_sha256,operands.data(),count}:AmountBody{};
  }
};
inline Plan inspect(const Evidence& observed) noexcept {
  Plan out;if(!observed.stable_boundary_verified){out.issue=Issue::observation;return out;}
  for(std::size_t i=0;i<cat::targets.size();++i){const Stock* match=nullptr;
    for(const auto& s:observed.stocks)if(s.symbol==cat::targets[i].symbol){if(match){out.issue=Issue::stock;return out;}match=&s;}
    if(!match||!match->known||match->amount<0||match->amount>INT32_MAX||(!i&&!match->unambiguous)){out.issue=Issue::stock;return out;}
    out.after_callback[i]=out.before_end[i]=out.after_end[i]=*match;
  }
  out.known=true;const auto hot=out.after_end[0].amount,room=INT32_MAX-out.after_end[1].amount;
  out.maximum=static_cast<std::uint32_t>(hot<room?hot:room);
  out.direct_maximum=hot<3&&out.maximum>1?1:out.maximum;return out;
}
// Pure number planning. No engine/hand/writer/presentation release.
// The generated hand object is never added to the inventory stock.
inline Plan plan(const cat::Descriptor& descriptor,const Evidence& observed,std::uint32_t quantity) noexcept {
  if(!cat::structurally_valid(descriptor)){Plan bad;bad.issue=Issue::descriptor;return bad;}
  auto out=inspect(observed);out.quantity=quantity;if(!out.known||!quantity)return out;
  if(quantity>out.maximum||quantity>INT32_MAX){out.issue=Issue::quantity;return out;}
  out.native_end_debit=1;
  if(quantity==1){out.callback=20775;out.owner=20773;}
  else if(out.after_end[0].amount>=3){
    // The original C_INFO condition >=3 is not patched/worked around.
    out.callback=20778;out.owner=20776;out.count=2;
    out.operands={{{62,2,1,1,false},{82,3,1,0,false}}};
    out.after_callback[0].amount-=quantity-1;
    out.requires_quantity_receipt=quantity!=3;
  }else{
    // Lager2/N2 requires a separate debit. cool_predebit::Plan binds
    // the runtime order and quantity receipt before the original call.
    out.callback=20775;out.owner=20773;out.count=1;
    out.operands[0]={62,1,1,0,false};out.extra_debit=quantity-1;out.requires_extra_debit=true;
  }
  out.after_callback[1].amount+=quantity;out.before_end=out.after_callback;
  out.before_end[0].amount-=out.extra_debit;out.after_end=out.before_end;--out.after_end[0].amount;
  out.valid=true;return out;
}
// ENDE is a separate abort path, not a production target. The original refund of 1
// occurs before the native inventory debit of 1. Account for the interval between them.
inline Plan cancel_plan(const Evidence& observed) noexcept {
  auto out=inspect(observed);if(!out.known)return out;
  if(out.after_end[0].amount==INT32_MAX){out.issue=Issue::lifecycle_room;return out;}
  out.callback=20772;out.owner=20770;out.native_end_debit=1;
  ++out.after_callback[0].amount;out.before_end=out.after_callback;out.valid=true;return out;
}
} // namespace ubc::cool_recipe
