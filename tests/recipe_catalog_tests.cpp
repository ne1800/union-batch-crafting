// SPDX-License-Identifier: MIT
#include "ubc/recipe_catalog.hpp"
#include <array>
#include <cstdio>
#include <cstring>

namespace {
namespace catalog = ubc::recipe_catalog;
unsigned checks = 0, failures = 0;
void check(bool value, int line) {
  ++checks;
  if (!value) { ++failures; std::printf("FAIL line %d\n", line); }
}
#define CHECK(x) check((x), __LINE__)
const catalog::ItemAmount* item(catalog::View<catalog::ItemAmount> items, std::int32_t symbol) {
  for (const auto& value : items) if (value.symbol == symbol) return &value;
  return nullptr;
}
const catalog::Requirement* requirement(catalog::View<catalog::Requirement> requirements,
    catalog::RequirementKind kind, std::int32_t symbol, std::uint32_t element) {
  for (const auto& value : requirements)
    if (value.kind == kind && value.symbol == symbol && value.element == element) return &value;
  return nullptr;
}
const ubc::AmountOperand* operand(const catalog::Callback& callback, std::uint32_t offset) {
  for (std::size_t i = 0; i < callback.amount.count; ++i)
    if (callback.amount.operands[i].offset == offset) return &callback.amount.operands[i];
  return nullptr;
}
void check_potion(catalog::RecipeId id, std::uint32_t single_symbol, std::uint32_t learned,
                  std::int32_t herb, std::int32_t output, std::int32_t owner, int number) {
  const auto* recipe = catalog::find(id);
  CHECK(recipe != nullptr);
  if (!recipe) return;
  CHECK(catalog::structurally_valid(*recipe));
  CHECK(recipe->station == catalog::Station::alchemy && recipe->production_context == 4);
  CHECK(recipe->targets.count == 4 && recipe->costs.count == 3 && recipe->outputs.count == 1);
  CHECK(recipe->owners.count == 1 && recipe->callbacks.count == 3);
  const auto* input = item(recipe->costs, herb);
  CHECK(input && input->units == 1);
  const auto* temp = item(recipe->costs, 16736);
  CHECK(temp && temp->units == 1);
  const auto* flask = item(recipe->costs, 16604);
  CHECK(flask && flask->units == 1); // Total costs including the hand reserve.
  const auto* result = item(recipe->outputs, output);
  CHECK(result && result->units == 1);
  const auto* learn = requirement(recipe->requirements, catalog::RequirementKind::integer_equal, 1464, learned);
  CHECK(learn && learn->value == 1 && learn->role == catalog::RequirementRole::knowledge);
  const auto* menu = requirement(recipe->requirements, catalog::RequirementKind::integer_equal,
                                 learned < 3 ? 20330 : 20299, 0);
  CHECK(menu && menu->value == 1 && menu->role == catalog::RequirementRole::context);
  CHECK(recipe->owners[0].instance == owner && recipe->owners[0].number == number);
  CHECK(recipe->owners[0].npc == 22243 && recipe->owners[0].permanent);
  CHECK(!recipe->owners[0].important && !recipe->owners[0].trade);
  CHECK(recipe->owners[0].condition->return_type == 2 && recipe->owners[0].condition->flags == 3);
  CHECK(recipe->owners[0].information->symbol == single_symbol - 2);
  CHECK(recipe->station_entry->symbol == 20276 && recipe->station_end->symbol == 20279);
  CHECK(recipe->hand.start_item_symbol == 16604 && recipe->hand.active_item_symbol == 16604);
  CHECK(recipe->hand.amount == 1 && recipe->hand.from_inventory && recipe->hand.consumed_on_success);
  CHECK(recipe->hand.failure_refund == 1 && recipe->hand.left_must_be_empty);
  CHECK(recipe->hand.evidence == (learned == 0 ? catalog::HandEvidence::observed_original :
                                               catalog::HandEvidence::inferred_same_station));
  CHECK(catalog::callback_for_quantity(*recipe, 0) == nullptr);
  CHECK(catalog::callback_for_quantity(*recipe, UINT32_MAX) == nullptr);
  const auto* one = catalog::callback_for_quantity(*recipe, 1);
  CHECK(one && one->function->symbol == single_symbol && one->amount.count == 0);
  if (one) {
    CHECK(one->function->byte_length == 199 && one->maximum_quantity == 1);
    CHECK(one->effects.count == 3 && one->invocation == catalog::Invocation::choice);
    for (const auto& effect : one->effects) CHECK(effect.symbol != 16604);
  }
  CHECK(recipe->callbacks[1].function->symbol == single_symbol + 1);
  CHECK(recipe->callbacks[1].original_quantity == 3 && recipe->callbacks[1].minimum_quantity == 0);
  for (const auto n : {2u,3u,4u,5u,7u,23u,static_cast<unsigned>(INT32_MAX)}) {
    const auto* bulk = catalog::callback_for_quantity(*recipe, n);
    CHECK(bulk && bulk->function->symbol == single_symbol + 2);
    if (!bulk) continue;
    CHECK(bulk->function->byte_length == 219 && bulk->amount.count == 6);
    CHECK(bulk->minimum_quantity == 2 && bulk->original_quantity == 5);
    CHECK(bulk->body_checks_resources && bulk->ends_production && !bulk->clears_choices);
    CHECK(bulk->quantity_text_reviewed);
    CHECK(bulk->effects.count == 4 && bulk->failure_effects.count == 1);
    CHECK(bulk->failure_effects[0].action == catalog::MaterialAction::create);
    CHECK(bulk->failure_effects[0].symbol == 16604 && bulk->failure_effects[0].amount == 1);
    CHECK(bulk->failure_effects[0].actor == catalog::Actor::self);
    CHECK(operand(*bulk, 204) == nullptr); // The flask on the error path remains unchanged.
    for (const auto at : {1u,22u,59u,79u}) {
      const auto* op = operand(*bulk, at);
      CHECK(op && op->original == 5 && op->units == 1 && op->reserved == 0 && !op->allow_zero);
    }
    const bool mana_one = id == catalog::RecipeId::mana_01;
    const auto* flask_op = operand(*bulk, mana_one ? 99u : 119u);
    const auto* output_op = operand(*bulk, mana_one ? 119u : 99u);
    CHECK(flask_op && flask_op->original == 4 && flask_op->reserved == 1);
    CHECK(output_op && output_op->original == 5 && output_op->reserved == 0);
    const auto& flask_effect = bulk->effects[mana_one ? 2u : 3u];
    const auto& output_effect = bulk->effects[mana_one ? 3u : 2u];
    CHECK(flask_effect.action == catalog::MaterialAction::remove && flask_effect.symbol == 16604);
    CHECK(flask_effect.units == 1 && flask_effect.reserved == 1);
    CHECK(output_effect.action == catalog::MaterialAction::create && output_effect.symbol == output);
    CHECK(std::uint64_t{flask_effect.units} * n - flask_effect.reserved == n - 1);
    CHECK(std::uint64_t{flask_effect.units} * n - flask_effect.reserved + recipe->hand.amount == n);
  }
}
}
int main() {
  CHECK(!catalog::runtime_ready);
  CHECK(catalog::recipes.size() == 9);
  CHECK(catalog::find(static_cast<catalog::RecipeId>(99)) == nullptr);
  std::size_t maximum_targets = 0;
  for (const auto& recipe : catalog::recipes) {
    CHECK(catalog::structurally_valid(recipe));
    if (recipe.targets.count > maximum_targets) maximum_targets = recipe.targets.count;
    for (const auto& callback : recipe.callbacks) {
      CHECK(callback.function->argc == 0 && callback.function->return_type == 0);
      CHECK(callback.function->byte_offset < ubc::lhiver::code_length);
      CHECK(callback.function->byte_length <= ubc::lhiver::code_length - callback.function->byte_offset);
      CHECK(catalog::owner_for(recipe, callback.owner_instance) != nullptr);
    }
  }
  CHECK(catalog::catalog_target_capacity == maximum_targets);
  check_potion(catalog::RecipeId::health_01,20341,0,16714,16775,20337,2);
  check_potion(catalog::RecipeId::health_02,20348,1,16716,16778,20344,3);
  check_potion(catalog::RecipeId::health_03,20355,2,16718,16781,20351,4);
  check_potion(catalog::RecipeId::mana_01,20310,3,16708,16766,20306,2);
  check_potion(catalog::RecipeId::mana_02,20317,4,16710,16769,20313,3);
  check_potion(catalog::RecipeId::mana_03,20324,5,16712,16772,20320,4);

  const auto& meat = *catalog::find(catalog::RecipeId::meat);
  std::size_t campfire_bindings = 0;
  for (const auto& binding : catalog::common_bindings) {
    if (std::strcmp(binding.name, "MOBSI_CAMPFIRE") == 0) {
      ++campfire_bindings;
      CHECK(binding.symbol == 12324 && binding.check_value && binding.value == meat.production_context);
    }
  }
  CHECK(campfire_bindings == 1 && std::strcmp(meat.name, "Fleisch braten") == 0);
  CHECK(meat.callbacks.count == 4 && meat.owners.count == 4);
  CHECK(meat.station == catalog::Station::stove && meat.production_context == 14);
  CHECK(meat.hand.start_item_symbol == 16607 && meat.hand.active_item_symbol == 16608);
  CHECK(!meat.hand.from_inventory && !meat.hand.consumed_on_success && meat.hand.failure_refund == 0);
  CHECK(meat.hand.evidence == catalog::HandEvidence::observed_original);
  const auto* meat_n = catalog::callback_for_quantity(meat, 7);
  CHECK(meat_n && meat_n->function->symbol == 21010);
  CHECK(meat_n && meat_n->function->body_sha256 == ubc::lhiver::functions[1].body_sha256);
  CHECK(meat_n && meat_n->amount.count == 3 && meat_n->effects.count == 2);
  if (meat_n) {
    CHECK(meat_n->effects[0].action == catalog::MaterialAction::create && meat_n->effects[0].symbol == 16379);
    CHECK(meat_n->effects[1].action == catalog::MaterialAction::remove && meat_n->effects[1].symbol == 16377);
    CHECK(!meat_n->ends_production && meat_n->clears_choices);
  }
  CHECK(meat.callbacks[3].function->symbol == 21019 && meat.callbacks[3].original_quantity == 0);
  CHECK(meat.callbacks[3].minimum_quantity == 0); // Never use All as a quantity fallback.
  CHECK(catalog::callback_for_quantity(meat, INT32_MAX) == meat_n);
  CHECK(catalog::callback_for_quantity(meat, 0) == nullptr);

  const auto& dish = *catalog::find(catalog::RecipeId::molerat_dish);
  CHECK(dish.costs.count == 3 && dish.outputs.count == 1 && dish.owners.count == 2);
  for (const auto symbol : {16363,16401,16395}) {
    const auto* input = item(dish.costs, symbol);
    CHECK(input && input->units == 5);
  }
  CHECK(dish.outputs[0].symbol == 16365 && dish.outputs[0].units == 1);
  const auto* learned = requirement(dish.requirements, catalog::RequirementKind::integer_equal, 10535, 0);
  CHECK(learned && learned->value == 1 && learned->role == catalog::RequirementRole::knowledge);
  CHECK(dish.owners[0].instance == 21021 && dish.owners[0].number == 5);
  CHECK(dish.owners[1].instance == 21024 && dish.owners[1].number == 6);
  for (const auto& condition : dish.owners[1].conditions)
    if (condition.kind == catalog::RequirementKind::inventory_at_least) CHECK(condition.value == 10);
  const auto* dish_n = catalog::callback_for_quantity(dish, 2);
  CHECK(dish_n && dish_n->function->symbol == 21023 && dish_n->amount.count == 4);
  CHECK(dish_n && !dish_n->body_checks_resources && !dish_n->ends_production);
  CHECK(dish.hand.evidence == catalog::HandEvidence::inferred_same_station);
  CHECK(catalog::callback_for_quantity(dish, static_cast<unsigned>(INT32_MAX) / 5) == dish_n);
  CHECK(catalog::callback_for_quantity(dish, static_cast<unsigned>(INT32_MAX) / 5 + 1) == nullptr);

  const auto& mushroom = *catalog::find(catalog::RecipeId::mushroom_dish);
  CHECK(mushroom.costs.count == 1 && mushroom.costs[0].symbol == 16726 && mushroom.costs[0].units == 50);
  CHECK(mushroom.outputs.count == 1 && mushroom.outputs[0].symbol == 16361);
  CHECK(mushroom.owners[0].number == 7 && mushroom.requirements.count == 1);
  CHECK(mushroom.requirements[0].role == catalog::RequirementRole::context);
  const auto* mushroom_n = catalog::callback_for_quantity(mushroom, 7);
  CHECK(mushroom_n && mushroom_n->function->symbol == 21029 && mushroom_n->amount.count == 2);
  CHECK(mushroom_n && operand(*mushroom_n, 11)->units == 50 && operand(*mushroom_n, 31)->units == 1);
  CHECK(catalog::callback_for_quantity(mushroom, static_cast<unsigned>(INT32_MAX) / 50) == mushroom_n);
  CHECK(catalog::callback_for_quantity(mushroom, static_cast<unsigned>(INT32_MAX) / 50 + 1) == nullptr);

  // Structural validation does not need a global capacity of six. A custom
  // descriptor can still represent additional fully bound target items.
  std::array<ubc::PackedInventoryTarget,7> many_targets{};
  std::array<ubc::lhiver::SymbolBinding,32> many_bindings{};
  for (std::size_t i=0;i<meat.targets.count;++i) many_targets[i]=meat.targets[i];
  for (std::size_t i=0;i<meat.bindings.count;++i) many_bindings[i]=meat.bindings[i];
  const std::array<const char*,3> extra_names{{"TEST_SEVEN_A","TEST_SEVEN_B","TEST_SEVEN_C"}};
  for (std::size_t i=0;i<extra_names.size();++i) {
    many_targets[meat.targets.count+i]={extra_names[i],static_cast<std::int32_t>(91000+i)};
    many_bindings[meat.bindings.count+i]={extra_names[i],static_cast<std::uint32_t>(91000+i),7,1,0,1584};
  }
  auto wide=meat;
  wide.targets=catalog::view(many_targets);
  wide.bindings={many_bindings.data(),meat.bindings.count+3};
  CHECK(wide.targets.count>6 && catalog::structurally_valid(wide));

  auto broken=meat;
  broken.targets={nullptr,1}; CHECK(!catalog::structurally_valid(broken));
  broken=meat; broken.hand.evidence=static_cast<catalog::HandEvidence>(99);
  CHECK(!catalog::structurally_valid(broken));
  broken=meat; broken.hand.left_must_be_empty=false; CHECK(!catalog::structurally_valid(broken));
  broken=meat; broken.production_context=4; CHECK(!catalog::structurally_valid(broken));
  std::array<catalog::ItemAmount,2> duplicates{{meat.costs[0],meat.costs[0]}};
  broken=meat; broken.costs=catalog::view(duplicates); CHECK(!catalog::structurally_valid(broken));
  duplicates={{meat.outputs[0],meat.outputs[0]}};
  broken=meat; broken.outputs=catalog::view(duplicates); CHECK(!catalog::structurally_valid(broken));
  std::array<catalog::ItemAmount,1> invalid_cost{{{16379,1}}};
  broken=meat; broken.costs=catalog::view(invalid_cost); CHECK(!catalog::structurally_valid(broken));
  invalid_cost[0]={16377,0}; CHECK(!catalog::structurally_valid(broken));
  invalid_cost[0]={16377,static_cast<unsigned>(INT32_MAX)+1}; CHECK(!catalog::structurally_valid(broken));
  invalid_cost[0]={99999,1}; CHECK(!catalog::structurally_valid(broken));
  many_targets[6]=many_targets[0]; CHECK(!catalog::structurally_valid(wide));
  many_targets[6]={"TEST_SEVEN_C",91002};
  many_bindings[0].parent_index=-1; CHECK(!catalog::structurally_valid(wide));

  std::array<catalog::Callback,1> altered{{*meat_n}};
  broken=meat; broken.callbacks=catalog::view(altered);
  altered[0].amount.digest[0]^=1; CHECK(!catalog::structurally_valid(broken));
  altered[0]=*meat_n; altered[0].owner_instance=123; CHECK(!catalog::structurally_valid(broken));
  altered[0]=*meat_n; altered[0].maximum_quantity=0; CHECK(!catalog::structurally_valid(broken));
  altered[0]=*meat_n; altered[0].function=nullptr; CHECK(!catalog::structurally_valid(broken));
  std::array<ubc::AmountOperand,2> overlapping{{{1,1,1,0},{2,1,1,0}}};
  altered[0]=*meat_n; altered[0].amount.operands=overlapping.data();altered[0].amount.count=overlapping.size();
  CHECK(!catalog::structurally_valid(broken));
  overlapping[1]={37,1,1,0,true}; CHECK(!catalog::structurally_valid(broken));
  overlapping[1]={37,2,1,0}; CHECK(!catalog::structurally_valid(broken)); // Wrong old literal value.
  altered[0]=meat.callbacks[3]; altered[0].minimum_quantity=1; altered[0].maximum_quantity=1;
  CHECK(!catalog::structurally_valid(broken)); // The original All must never serve as a template for N=1.

  std::array<catalog::Requirement,1> invalid_requirement{{meat.requirements[0]}};
  broken=meat; broken.requirements=catalog::view(invalid_requirement);
  invalid_requirement[0].element=1; CHECK(!catalog::structurally_valid(broken));
  invalid_requirement[0]=meat.requirements[0]; invalid_requirement[0].symbol=99999;
  CHECK(!catalog::structurally_valid(broken));
  invalid_requirement[0]=meat.requirements[0]; invalid_requirement[0].role=static_cast<catalog::RequirementRole>(99);
  CHECK(!catalog::structurally_valid(broken));
  std::array<catalog::Owner,2> repeated_owner{{meat.owners[0],meat.owners[0]}};
  broken=meat; broken.owners=catalog::view(repeated_owner);
  CHECK(!catalog::structurally_valid(broken));

  const auto& healing=*catalog::find(catalog::RecipeId::health_01);
  broken=healing; broken.costs={healing.costs.data,2};
  CHECK(!catalog::structurally_valid(broken)); // The reserved flask must not disappear from the costs.
  CHECK(catalog::callback_for_quantity(broken,2)==nullptr);
  std::array<catalog::Callback,2> overlap{{healing.callbacks[0],healing.callbacks[2]}};
  broken=healing; broken.callbacks=catalog::view(overlap);overlap[1].minimum_quantity=1;
  CHECK(!catalog::structurally_valid(broken));
  overlap[0]=healing.callbacks[1]; overlap[0].minimum_quantity=1; overlap[0].maximum_quantity=1;
  overlap[1]=healing.callbacks[2];
  CHECK(!catalog::structurally_valid(broken)); // The unchanged triple is not a single-item path.
  std::printf("recipe_catalog: %u checks, %u failures\n",checks,failures);
  return failures ? 1 : 0;
}
