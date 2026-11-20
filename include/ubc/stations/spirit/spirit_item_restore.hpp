// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/spirit/spirit_flask_debit.hpp"
#include "ubc/stations/alchemy/alchemy_extra_dependencies.hpp"

namespace ubc::spirit_item_restore {
using alchemy_extra_dependencies::ItemBinding;using alchemy_extra_dependencies::ItemRestorePlan;
// Same positively read SetOffset/ITEM contract, but own genuine
// Callback20370. No renaming into any of the 13 linear callback indices.
inline ItemRestorePlan plan(const spirit_debit::Call& call,const ItemBinding& saved,const ItemBinding& current,
    const spirit_debit::Context& context,const spirit_debit::Scope& scope) noexcept {
  namespace d=spirit_debit;namespace dep=alchemy_extra_dependencies;
  if(!call.valid||!call.job||!call.writer_token||call.callback!=20370||!d::valid(call.context)||!d::valid_after_original(call.parser,call.context,call.callback)||
     call.instance!=16604||!call.quantity||call.quantity>=spirit_recipe_catalog::maximum_quantity||!d::same(call.context,context)||
     !scope.writer_active||scope.writer_depth!=1||scope.vm_depth||scope.debit_depth!=1||
     scope.job!=call.job||scope.writer_token!=call.writer_token||scope.callback!=call.callback||
     !dep::same_item_target(saved,current)||saved.parser!=call.context.parser||saved.code!=call.parser.code_base||
     saved.code_size!=call.parser.code_size)return {};
  return {true,saved.value!=current.value,0x7a2f20,saved.symbol,saved.value};
}
} // namespace ubc::spirit_item_restore
