// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/alchemy_recipe_catalog.hpp"
#include "ubc/alchemy_flask_debit.hpp"
#include "ubc/recipe_material.hpp"
#include "ubc/runtime_script.hpp"

namespace ubc {
struct AlchemyExtraRequirementValue {
  std::int32_t symbol=-1;
  std::uint32_t element=0;
  std::int32_t value=0;
};
constexpr std::size_t alchemy_extra_requirement_capacity() noexcept {
  std::size_t count=0;
  for(const auto& r:alchemy_recipe_catalog::recipes)if(r.requirements.count>count)count=r.requirements.count;
  return count;
}
struct AlchemyExtraKnowledge {
  bool bindings_verified=false,unchanged=false;
  std::array<AlchemyExtraRequirementValue,alchemy_extra_requirement_capacity()> values{};
  std::size_t count=0;
  runtime_script::Issue issue=runtime_script::Issue::none;
  std::uint32_t failed_symbol=0;
};

// Only the exact original predicate values are read. A learned value alone does
// not confirm the callback/C_INFO contract; this remains separate evidence.
inline AlchemyExtraKnowledge read_alchemy_extra_knowledge(const ReadOnlyMemory& memory,
    std::uint32_t parser,const alchemy_recipe_catalog::Descriptor& recipe) noexcept {
  namespace cat=alchemy_recipe_catalog;namespace rt=runtime_script;
  AlchemyExtraKnowledge result;
  if(!cat::structurally_valid(recipe)) {result.issue=rt::Issue::invalid_profile;return result;}
  rt::detail::Layout before{};result.issue=rt::detail::read_layout(memory,parser,lhiver::profile,before);
  if(result.issue!=rt::Issue::none)return result;
  result.count=recipe.requirements.count;
  for(unsigned pass=0;pass<2;++pass)for(std::size_t i=0;i<result.count;++i) {
    const auto& q=recipe.requirements[i];const auto symbol=static_cast<std::uint32_t>(q.symbol);
    const auto* binding=cat::binding_for(recipe,symbol);
    result.failed_symbol=symbol;
    if(!binding) {result.issue=rt::Issue::invalid_profile;return result;}
    result.issue=rt::detail::check_binding(memory,before,*binding);if(result.issue!=rt::Issue::none)return result;
    const auto value=rt::symbol_int_element(memory,parser,symbol,q.element);
    if(!value.valid) {result.issue=value.issue;return result;}
    if(pass && result.values[i].value!=value.value) {result.issue=rt::Issue::changed_during_read;return result;}
    result.values[i]={q.symbol,q.element,value.value};
  }
  result.issue=rt::detail::stable_layout(memory,parser,lhiver::profile,before);
  result.bindings_verified=result.unchanged=result.issue==rt::Issue::none;
  if(result.unchanged)result.failed_symbol=0;
  return result;
}

struct AlchemyExtraSnapshotInput {
  alchemy_flask_debit::Context context{}; // Bound active physical LAB station.
  std::uint64_t serial=0; // Shared monotone observation sequence, no callback counter.
  RecipeInventory inventory{};
  HandsSnapshot hands{};
  AlchemyExtraKnowledge knowledge{};
  alchemy_flask_debit::FlaskStack flask{};
  bool boundary_verified=false,local_recipe_contract_verified=false;
  bool hand_contract_verified=false,interact_item_known=false;
  std::uint32_t interact_item=0;
  // Includes the local native/restore/configuration closure, not just
  // a matching byte range. Packed flasks remain separately visible.
  bool native_debit_contract_verified=false;
};
enum class AlchemyExtraSnapshotIssue {
  none,descriptor,context,unknown_profile,unknown_knowledge,not_learned,
  material,native_contract,extra_flask_stack,quantity
};
struct AlchemyExtraSnapshot {
  alchemy_recipe_catalog::RecipeId id=alchemy_recipe_catalog::RecipeId::lou_spirit;
  alchemy_flask_debit::Context context{};
  std::uint64_t serial=0;
  std::uint32_t quantity=0,maximum_executable=0;
  bool profile_verified=false,learned_known=false,learned=false,visible=false;
  bool owner_conditions_match=false,native_debit_verified=false,extra_flask_stack_verified=false;
  bool can_select=false,effect_ready=false;
  RecipeMaterial material{};
  std::array<RecipeMaterialRow,alchemy_recipe_catalog::catalog_target_capacity> rows{};
  alchemy_flask_debit::Hand hand{};
  alchemy_flask_debit::FlaskStack flask{};
  AlchemyExtraSnapshotIssue issue=AlchemyExtraSnapshotIssue::none;
};

namespace alchemy_extra_detail {
inline bool knowledge(const alchemy_recipe_catalog::Descriptor& recipe,const AlchemyExtraKnowledge& input,
    bool& learned,bool& conditions,bool& active_lab) noexcept {
  if(!input.bindings_verified || !input.unchanged || input.issue!=runtime_script::Issue::none ||
     input.count!=recipe.requirements.count || input.count>input.values.size()) return false;
  learned=true;conditions=true;active_lab=false;
  for(const auto& q:recipe.requirements) {
    const AlchemyExtraRequirementValue* found=nullptr;
    for(std::size_t i=0;i<input.count;++i)if(input.values[i].symbol==q.symbol && input.values[i].element==q.element) {
      if(found)return false;found=&input.values[i];
    }
    if(!found)return false;
    const bool matches=found->value==q.value;conditions&=matches;
    if(q.role==recipe_catalog::RequirementRole::knowledge)learned&=matches;
    if(q.symbol==12330 && q.element==0 && q.value==4)active_lab=matches;
  }
  return true;
}
inline RecipeMaterial material(const alchemy_recipe_catalog::Descriptor& recipe,const AlchemyExtraSnapshotInput& input,
    std::uint32_t quantity,RecipeMaterialRow* rows,std::size_t capacity) noexcept {
  namespace detail=recipe_material_detail;
  RecipeMaterial result;result.quantity=quantity;
  if(!rows || capacity<recipe.targets.count) {result.issue=RecipeMaterialIssue::output_buffer;return result;}
  result.count=recipe.targets.count;
  for(std::size_t i=0;i<result.count;++i) {
    rows[i]={};rows[i].symbol=recipe.targets[i].symbol;rows[i].unit_cost=alchemy_recipe_catalog::units(recipe.costs,rows[i].symbol);
    rows[i].unit_output=alchemy_recipe_catalog::units(recipe.outputs,rows[i].symbol);
  }
  const auto& inventory=input.inventory;
  if(!inventory.header_known || !inventory.listed_known || !inventory.packed_known || !inventory.storage_totals_known ||
     !inventory.stable_boundary_verified || !inventory.snapshot_unchanged || inventory.issue!=PackedInventoryIssue::none) {result.issue=RecipeMaterialIssue::unknown_inventory;return result;}
  if(inventory.listed_nodes>max_packed_inventory_nodes || inventory.packed_entries>max_packed_inventory_entries ||
     inventory.items.count!=result.count || !inventory.items.data) {result.issue=RecipeMaterialIssue::invalid_inventory;return result;}
  std::uint64_t listed=0,packed=0;
  for(std::size_t i=0;i<result.count;++i) {
    auto& row=rows[i];const auto* item=detail::item(inventory,row.symbol);
    if(!item) {result.issue=RecipeMaterialIssue::target_mapping;return result;}
    if(item->listed<0 || item->packed<0 || item->total<0 || std::int64_t{item->listed}+item->packed!=item->total ||
       (item->listed>0 && !item->listed_occurrences) || ((item->packed>0)!=(item->packed_occurrences>0)) ||
       item->listed_occurrences>inventory.listed_nodes || item->packed_occurrences>inventory.packed_entries) {result.issue=RecipeMaterialIssue::invalid_inventory;return result;}
    listed+=item->listed_occurrences;packed+=item->packed_occurrences;
    row.stored=row.available=item->total;row.available_known=true;
    if(row.unit_cost && (item->listed_occurrences>1 || item->packed_occurrences>1 || item->duplicate_packed_target ||
       item->has_active_marker || item->mixed_representation || (item->listed_occurrences && item->packed_occurrences)))result.issue=RecipeMaterialIssue::ambiguous_ingredient;
  }
  if(listed>inventory.listed_nodes || packed>inventory.packed_entries) {result.issue=RecipeMaterialIssue::invalid_inventory;return result;}
  result.inventory_known=true;
  auto hand_issue=RecipeMaterialIssue::none;
  if(!input.hand_contract_verified || !input.interact_item_known)hand_issue=RecipeMaterialIssue::hand_contract_unverified;
  else if(input.hands.left.state!=0 || input.hands.left.object || input.hands.right.state!=1 || !input.hands.right.object ||
    input.hands.right.symbol!=16604 || input.hands.right.amount!=1 || input.interact_item!=input.hands.right.object)hand_issue=RecipeMaterialIssue::hand_object;
  else if(!detail::no_hand_alias(inventory,input.hands))hand_issue=RecipeMaterialIssue::hand_alias;
  else result.hand_known=true;
  if(result.issue==RecipeMaterialIssue::none)result.issue=hand_issue;
  std::uint64_t maximum=recipe.callback.maximum_quantity;
  for(std::size_t i=0;i<result.count;++i) {
    auto& row=rows[i];
    if(row.symbol==16604) {
      row.held=row.reserved=result.hand_known?1:-1;row.available_known=result.hand_known;
      row.available=result.hand_known?row.stored+1:-1;
    }
    row.output_space=INT32_MAX-row.stored;
    row.output_shortfall_for_one=detail::positive(std::int64_t{row.unit_output}-row.output_space);
    row.missing_for_one=row.available_known?detail::positive(std::int64_t{row.unit_cost}-row.available):-1;
    if(row.unit_cost && row.available_known)detail::limit(maximum,static_cast<std::uint64_t>(row.available)/row.unit_cost);
    if(row.unit_output)detail::limit(maximum,static_cast<std::uint64_t>(row.output_space)/row.unit_output);
    if(quantity<=INT32_MAX) {
      row.required=std::int64_t{row.unit_cost}*quantity;row.produced=std::int64_t{row.unit_output}*quantity;
      row.missing=row.available_known?detail::positive(row.required-row.available):-1;
      row.remaining=row.available_known?row.available-row.required+row.produced:-1;
      row.output_shortfall=detail::positive(row.produced-row.output_space);
    }
  }
  if(result.known())result.maximum=static_cast<std::uint32_t>(maximum);
  if(quantity>INT32_MAX && result.issue==RecipeMaterialIssue::none)result.issue=RecipeMaterialIssue::quantity;
  result.craftable=result.known() && quantity>0 && quantity<=result.maximum;
  return result;
}
} // namespace alchemy_extra_detail

// Quantity 0 is a preview only and shows known shortages. Hide recipes
// whose learned status is unknown. Keep known material shortages visible
// for learned recipes. Other categories' flags require navigation and are
// no reason to remove an otherwise learned recipe from the shared list.
inline AlchemyExtraSnapshot alchemy_extra_snapshot(const alchemy_recipe_catalog::Descriptor& recipe,
    const AlchemyExtraSnapshotInput& input,std::uint32_t quantity) noexcept {
  namespace cat=alchemy_recipe_catalog;using Issue=AlchemyExtraSnapshotIssue;
  AlchemyExtraSnapshot out;out.id=recipe.id;out.context=input.context;out.serial=input.serial;out.quantity=quantity;
  if(!cat::structurally_valid(recipe)) {out.issue=Issue::descriptor;return out;}
  if(!alchemy_flask_debit::valid(input.context) || !input.serial || !input.boundary_verified) {out.issue=Issue::context;return out;}
  out.profile_verified=input.local_recipe_contract_verified;out.native_debit_verified=input.native_debit_contract_verified;
  bool active_lab=false;
  out.learned_known=alchemy_extra_detail::knowledge(recipe,input.knowledge,out.learned,out.owner_conditions_match,active_lab);
  out.visible=out.profile_verified && out.learned_known && out.learned && active_lab;
  out.material=alchemy_extra_detail::material(recipe,input,quantity,out.rows.data(),out.rows.size());
  out.flask=input.flask;
  if(out.material.hand_known)out.hand={true,true,input.hands.right.object,16604,1,input.interact_item};
  const auto* flask=out.material.known()?recipe_material_detail::item(input.inventory,16604):nullptr;
  if(out.material.known() && flask && alchemy_flask_debit::listed_flask(input.flask,flask->total) &&
     input.flask.packed==flask->packed && input.flask.listed_stacks==flask->listed_occurrences) {
    std::size_t found=0;for(const auto object:input.inventory.listed_objects)if(object==input.flask.object)++found;
    out.extra_flask_stack_verified=flask->total==0?(!input.flask.object && !found):found==1;
  }
  if(out.material.known() && out.native_debit_verified) {
    out.maximum_executable=out.material.maximum;
    if(!out.extra_flask_stack_verified && out.maximum_executable>1)out.maximum_executable=1;
  }
  out.can_select=out.visible && out.material.craftable && quantity<=out.maximum_executable;
  out.effect_ready=out.can_select && out.owner_conditions_match;
  if(!out.profile_verified)out.issue=Issue::unknown_profile;
  else if(!out.learned_known)out.issue=Issue::unknown_knowledge;
  else if(!out.learned)out.issue=Issue::not_learned;
  else if(!active_lab)out.issue=Issue::context;
  else if(!out.material.known())out.issue=Issue::material;
  else if(!out.native_debit_verified)out.issue=Issue::native_contract;
  else if(quantity>1 && !out.extra_flask_stack_verified)out.issue=Issue::extra_flask_stack;
  else if(quantity && !out.material.craftable)out.issue=Issue::quantity;
  return out;
}
} // namespace ubc
