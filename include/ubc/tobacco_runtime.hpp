// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/alchemy_extra_execution.hpp"
#include "ubc/crafting_handoff.hpp"
#include "ubc/grind_check.hpp"
#include "ubc/tobacco_parser.hpp"
#include "ubc/tobacco_transaction.hpp"

namespace ubc::tobacco {
namespace rt=alchemy_flask_runtime;
namespace script=runtime_script;
inline constexpr rt::StringContract success_text{14375,25,
  lhiver::digest_literal("ad3c708821fe40c2d054d9899796d83042f22c31c18da808c423ddd1b6ba74c9")};
inline alchemy_extra_execution::Profile script_profile() noexcept {
  alchemy_extra_execution::Profile assembly;
  alchemy_extra_execution::common(assembly);
  for(const auto& f:functions)assembly.add(&f);
  for(auto id:{20387u,20388u,20389u})assembly.add(alchemy_extra_navigation::function_for(id));
  for(auto id:{16604u,16706u,20276u,20279u,19965u})assembly.add(alchemy_recipe_catalog::function_for(id));
  assembly.add(lhiver::SymbolBinding{"ITPL_MUSHROOM_01",16726,7,1,0,1584});
  assembly.add(lhiver::SymbolBinding{"PRINT_TABAKSUCCESS",14375,3,1,1,-1});
  return assembly;
}
template<class Check>
inline bool verify_script(const ReadOnlyMemory& memory,Check& check) noexcept {
  const auto assembly=script_profile();
  const auto profile=assembly.view();
  const auto result=script::verify_local_contract(memory,lhiver::game_parser,profile);
  if(!check.equal("script.contract",0,static_cast<int>(result.issue))||!check.equal("script.failed_symbol",0,result.failed_symbol)||!check.equal("script.matches",1,result.matches)){
    check.symbol=static_cast<int>(result.failed_symbol);return false;
  }
  script::detail::Layout layout{};
  if(!check.equal("script.layout",0,static_cast<int>(script::detail::read_layout(memory,lhiver::game_parser,profile,layout))))return false;
  const auto msg=rt::detail::string_once(memory,layout,success_text);
  return check.equal("script.success_text",0,static_cast<int>(msg.issue))&&
    check.equal("script.stable",0,static_cast<int>(script::detail::stable_layout(memory,lhiver::game_parser,profile,layout)));
}
// The last unused INT return value comes from the original owner. On the
// next CallFunc Gothic empties the stack before the child's execution.
inline bool owner_prefix(const alchemy_flask_debit::ParserState& p) noexcept {
  return p.data_words_known&&p.data_depth==2&&p.data_words[0]==1&&p.data_words[1]==0x40;
}
inline bool choice_entry(const ReadOnlyMemory& memory,const alchemy_flask_debit::Context& c,GrindCheck& check) noexcept {
  auto first=rt::detail::parser_once(memory,c,false);
  if(!check.equal("parser.entry",true,first.valid&&alchemy_flask_debit::valid_parser_bindings(first.value,c)))return false;
  // An empty stack is also occupied when the engine has already
  // cleaned up between the CallFuncs. The caller binds the paid ticket.
  if(first.value.data_depth&&!check.equal("parser.owner_prefix",true,owner_prefix(first.value)))return false;
  const auto second=rt::detail::parser_once(memory,c,false);
  return check.equal("parser.entry_stable",true,second.valid&&rt::detail::same_parser(first,second));
}
template<class Check>
inline bool lab_flags(const ReadOnlyMemory& memory,std::uint32_t hero,bool active,Check& check) noexcept {
  script::detail::Layout layout{};
  if(!check.equal("lab.layout",0,static_cast<int>(script::detail::read_layout(memory,lhiver::game_parser,rt::flask_profile,layout))))return false;
  constexpr const char* names[]={"lab.npc_class","lab.production_binding","lab.aivar","lab.invincible_binding"};
  unsigned name=0;
  for(const unsigned i:{4u,7u,8u,9u})
    if(!check.equal(names[name++],0,static_cast<int>(script::detail::check_binding(memory,layout,rt::flask_bindings[i])))){
      check.symbol=static_cast<int>(rt::flask_bindings[i].symbol);return false;
    }
  const auto prod=script::symbol_int(memory,lhiver::game_parser,lhiver::production_symbol);
  std::uint32_t invincible=0;
  return check.equal("lab.production",active?4:0,prod.valid?prod.value:-1)&&
    check.equal("lab.invincible_read",true,hero&&rt::detail::word(memory,hero,0x298,invincible))&&
    check.equal("lab.invincible",active?1:0,invincible)&&
    check.equal("lab.stable",0,static_cast<int>(script::detail::stable_layout(memory,lhiver::game_parser,rt::flask_profile,layout)));
}
template<class Probe>
std::uint32_t permitted_maximum(const Stock& stock,Probe probe) {
  const auto maximum_by_stock=maximum(stock);
  if(maximum_by_stock<=1||probe(maximum_by_stock))return maximum_by_stock;
  // Only the full withdrawal has a different native list path.
  if(maximum_by_stock>2&&probe(maximum_by_stock-1))return maximum_by_stock-1;
  return 1;
}
struct Capture {
  Stock stock{{-1,-1,-1,-1}};
  std::array<std::uint32_t,2> objects{};
  StationRecipeHands hands{};
  bool empty_pack=false;
};
class Reader {
public:
  template<class Check>
  bool capture(const ReadOnlyMemory& memory,const Descriptor& recipe,
               const alchemy_flask_debit::Context& context,Capture& out,Check& check) noexcept {
    if(!check.equal("station.hero",true,rt::detail::context(memory,context))||
       !check.equal("station.lab",true,rt::detail::lab(memory,context)))return false;
    const auto issue=station_recipe_snapshot_detail::begin_hands(memory,context.hero,hands_);
    if(!check.equal("hand.read",0,static_cast<int>(issue)))return false;
    out.hands=hands_.before;
    const auto* entry=crafting_catalog::find(recipe.id);
    if(!entry||!check.equal("hand.flask",true,crafting_detail::bound_hands(*entry,out.hands)))return false;
    const auto inv=read_packed_inventory<4>(memory,context.hero,recipe.targets.data(),4,true,inventory_);
    if(!check.equal("inventory.read",0,static_cast<int>(inv.issue))||
       !check.equal("inventory.stable",true,inv.storage_totals_known&&inv.snapshot_unchanged))return false;
    if(inv.storage_totals_known&&inv.snapshot_unchanged)out.stock=inv.total;
    out.empty_pack=inv.packed_entries==0;
    for(std::size_t i=0;i<2;++i){
      if(!check.equal("inventory.active",0,inv.has_active_marker[i])||
         !check.equal("inventory.mixed",0,inv.mixed_representation[i]||inv.duplicate_packed_target[i]||
          inv.listed_occurrences[i]>1||inv.packed_occurrences[i]>1))return false;
    }
    out.objects={};
    for(std::size_t i=0;i<inv.listed_nodes;++i){
      const auto& record=inventory_.records[i];const auto object=script::detail::u32(record.node.data()+4);
      if(!check.equal("hand.in_inventory",false,object==out.hands.interact_item))return false;
      const auto item=script::detail::u32(record.item_fields.data()+4);
      if(item!=apple&&item!=flask)continue;
      for(std::size_t slot=0;slot<hands_.count;++slot)
        if(!check.equal("stock.in_slot",false,script::detail::u32(hands_.records[slot].fields.data()+0x30)==object))return false;
      out.objects[item==apple?0:1]=object;
    }
    return check.equal("hand.stable",true,issue==StationHandReadIssue::none&&station_recipe_snapshot_detail::verify_hands(memory,hands_))&&
      check.equal("station.stable",true,rt::detail::context(memory,context));
  }
private:
  PackedInventoryWorkspace inventory_{};
  station_recipe_snapshot_detail::HandWorkspace hands_{};
};
inline Stock stocks(const CraftingState& state) noexcept {
  Stock out{};
  for(std::size_t i=0;i<out.size();++i)out[i]=state.materials[i].stored;
  return out;
}
inline bool match_stock(const Descriptor& recipe,const Stock& expected,const Stock& actual,GrindCheck& check) noexcept {
  for(std::size_t i=0;i<expected.size();++i)
    if(!check.equal("stock.units",expected[i],actual[i])){check.symbol=recipe.targets[i].symbol;return false;}
  return true;
}
// The own Owner opened the Choices without debit. A
// foreign, already-paid Choice must not trigger a new order.
struct OwnerTicket {
  CraftingKey key{};
  std::uint64_t ticket=0,source_collection=0,choice_collection=0;
  std::uint32_t info=0,view=0;
  int function=0;
  bool spent=false;
  // A new Collect of the same child side, focus loss, or a read gap
  // does not clear the unpaid Owner. BACK remains open as an exit.
  void observe(bool departed,bool ready,std::uint64_t collection,int mode,std::uint32_t current_info) noexcept {
    if(!ticket)return;
    if(departed){*this={};return;}
    if(!ready||collection<=source_collection)return;
    if(choice_collection&&(mode==1||(mode==2&&current_info&&current_info!=info))){*this={};return;}
    if(mode==2&&current_info==info)bind(collection);
  }
  bool bound_child(const CraftingHandoffTicket& child,int symbol,std::uint64_t current_collection) const noexcept {
    return ticket&&function==symbol&&child.id>ticket&&child.view==view&&choice_collection&&
      current_collection==choice_collection&&
      child.offer.valid&&child.offer.mode==recipe_catalog::Invocation::choice&&child.offer.owner==20399&&
      child.offer.function==function&&child.offer.collection==choice_collection;
  }
  bool accepts(const CraftingHandoffTicket& child,int symbol) const noexcept {
    return !spent&&bound_child(child,symbol,child.offer.collection);
  }
  bool owns(const CraftingKey& current,std::uint32_t current_view,std::uint32_t owner) const noexcept {
    return ticket&&key==current&&view&&view==current_view&&info&&info==owner;
  }
  bool bind(std::uint64_t collection) noexcept {
    if(!ticket||collection<=source_collection||(choice_collection&&choice_collection!=collection))return false;
    choice_collection=collection;return true;
  }
};
} // namespace ubc::tobacco
