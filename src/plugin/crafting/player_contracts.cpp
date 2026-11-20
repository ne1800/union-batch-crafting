// SPDX-License-Identifier: MIT
#include "runtime.hpp"

namespace ubc_native {
#ifdef UBC_PLAYER_RELEASE
ubc::player_engine::Result player_image;
static_assert(static_cast<unsigned>(cat::Station::saw) == 3 &&
              static_cast<unsigned>(cat::Station::grind) == 8);
std::array<bool, cat::recipe_count> player_recipes{};
std::array<bool, 9> checked_stations{}, available_stations{};

bool player_recipe_matches(const cat::Descriptor& recipe) noexcept {
  const auto parser = s::game_parser;
  switch (recipe.route) {
    case cat::Route::base: return ubc::recipe_runtime::verify(memory, parser, *recipe.base).matches;
    case cat::Route::station: return ubc::station_recipe_runtime::verify(memory, parser, *recipe.station_descriptor).matches;
    case cat::Route::alchemy_extra: return ubc::alchemy_extra_execution::verify_recipe(memory, *recipe.alchemy_extra).valid;
    case cat::Route::anvil: return art::verify(memory, parser, *recipe.anvil).matches;
    case cat::Route::ore: return ort::verify(memory, parser, *recipe.ore).matches;
    case cat::Route::spirit: return grt::verify_recipe(memory, parser, grt::assemble(*recipe.spirit)).valid;
    case cat::Route::bow: return brt::verify_recipe(memory, parser, *recipe.bow, brt::assemble(*recipe.bow)).valid;
    case cat::Route::cool: return crt::verify_recipe(memory, parser, *recipe.cool).valid;
    case cat::Route::grind: return shrt::verify_recipe(memory, parser, *recipe.grind).valid;
    case cat::Route::tobacco: {
      ubc::GrindCheck check;
      return tobacco::verify_script(memory, check);
    }
  }
  return false;
}
bool player_scripts_match(cat::Station station) noexcept {
  const auto index = static_cast<unsigned>(station);
  if (!own_thread()) return false;
  if (!checked_stations[index]) {
    if (frame_depth != 1 || update_depth || info_depth || choice_depth ||
        station_depth || collection_depth || handoff.active() || amount_patch.active()) return false;
    for (std::size_t i = 0; i < cat::recipes.size(); ++i) {
      const auto& recipe = cat::recipes[i];
      if (recipe.station != station) continue;
      player_recipes[i] = player_recipe_matches(recipe);
      if (player_recipes[i]) available_stations[index] = true;
      else event("script_contract_rejected", static_cast<int>(recipe.id), index);
    }
    checked_stations[index] = true;
  }
  return available_stations[index];
}
#endif
} // namespace ubc_native
