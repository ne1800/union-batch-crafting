// SPDX-License-Identifier: MIT
// Consistency of the generated reference script profiles without game data:
// the profiles are well-formed for relocate(), cover exactly the catalog
// recipes with their stations, and every catalog function the plugin hashes
// has a record with the same reference index, start, length and raw digest.
#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/catalogs/generated/dndr_26_fingerprints.hpp"
#include "ubc/catalogs/generated/marcello_204_fingerprints.hpp"
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/core/recipe_navigation.hpp"
#include "ubc/engine/script_relocation.hpp"
#include "ubc/stations/anvil/dndr_anvil.hpp"
#include <cstdio>
#include <cstring>
#include <set>
#include <string_view>

namespace {
namespace sr = ubc::script_relocation;
namespace cat = ubc::crafting_catalog;
unsigned checks = 0;
unsigned failures = 0;

void require(bool condition, const char* name) {
  ++checks;
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}

template <class Recipes>
void check_profile(const char* label, const sr::ReferenceProfile& profile, const Recipes& recipes,
                   std::uint32_t dat_size, std::uint32_t expected_size) {
  std::fprintf(stderr, "%s: %zu names, %zu functions, %zu data, %zu groups\n", label,
               profile.name_count, profile.function_count, profile.data_count, profile.group_count);
  require(sr::detail::valid_profile(profile), "generated profile is valid");
  require(dat_size == expected_size, "profile names its reference script");
  require(profile.parser_address == ubc::lhiver::game_parser, "profile uses the game parser");
  require(profile.by_reference && profile.stations && profile.station_count,
          "profile carries the permutation and station shares");
  require(profile.group_count == recipes.size(), "one group per catalog recipe");
  bool same = profile.group_count == recipes.size();
  for (std::size_t i = 0; same && i < recipes.size(); ++i) {
    const auto& group = profile.groups[i];
    same =
        group.recipe == static_cast<std::uint32_t>(recipes[i].id) &&
        group.station < profile.station_count &&
        profile.stations[group.station].station == static_cast<std::uint32_t>(recipes[i].station) &&
        group.function_count + profile.stations[group.station].function_count > 0;
  }
  require(same, "groups follow the catalog order, ids and stations");
  std::set<std::uint32_t> stations;
  for (std::size_t i = 0; i < profile.station_count; ++i)
    stations.insert(profile.stations[i].station);
  require(stations.size() == profile.station_count, "one share per station");
  bool texts = true;
  for (std::size_t i = 0; i < profile.text_count; ++i)
    texts = texts && profile.functions[profile.texts[i].function].ignore_text == false;
  require(texts, "functions holding adapter texts bind the text content");
}

const sr::FunctionRecord* record_of(const sr::ReferenceProfile& profile,
                                    const ubc::lhiver::Function& function) {
  for (std::size_t i = 0; i < profile.function_count; ++i) {
    const auto& record = profile.functions[i];
    const auto& name = profile.names[record.name];
    if (name.reference_index == function.symbol && std::strcmp(name.text, function.name) == 0)
      return &record;
  }
  return nullptr;
}

template <std::size_t N>
void check_functions(const char* label, const sr::ReferenceProfile& profile,
                     const std::array<ubc::lhiver::Function, N>& functions) {
  bool all = true;
  for (const auto& function : functions) {
    const auto* record = record_of(profile, function);
    const bool same = record && record->reference_start == function.byte_offset &&
                      record->byte_length == function.byte_length &&
                      record->reference_raw_sha256 == function.body_sha256 &&
                      record->type == function.type && record->flags == function.flags &&
                      record->argc == function.argc;
    if (!same)
      std::fprintf(stderr, "%s: no matching record for %s\n", label, function.name);
    all = all && same;
  }
  require(all, label);
}

void check_lookup(const sr::ReferenceProfile& profile) {
  // Without a scan every name is missing; the permutation must still find
  // each reference index exactly once and reject unknown ones.
  static sr::Storage<2048, 1, 1, 1> storage{};
  const auto workspace = storage.workspace();
  bool sorted = true;
  for (std::size_t i = 1; i < profile.name_count; ++i)
    sorted = sorted && profile.names[profile.by_reference[i - 1]].reference_index <
                           profile.names[profile.by_reference[i]].reference_index;
  require(sorted && profile.name_count <= 2048, "permutation orders reference indices");
  for (std::size_t i = 0; i < profile.name_count; ++i) {
    workspace.names[i].state = sr::NameState::found;
    workspace.names[i].live_index = static_cast<std::uint32_t>(i) + 7u;
  }
  bool found = true;
  for (std::size_t i = 0; i < profile.name_count; ++i) {
    std::uint32_t live = 0;
    found = found && sr::live_symbol(profile, workspace, profile.names[i].reference_index, live) &&
            live == i + 7u;
  }
  require(found, "every reference index maps through the permutation");
  std::uint32_t live = 0;
  require(!sr::live_symbol(profile, workspace, 0xfffffff0u, live), "unknown index is rejected");
}
} // namespace

int main() {
  namespace m = ubc::script_profiles::marcello_204;
  namespace d = ubc::script_profiles::dndr_26;
  check_profile("marcello", m::profile, cat::recipes, m::dat_size, 9327865u);
  check_profile("dndr", d::profile, cat::dndr_recipes, d::dat_size, 7587412u);
  require(m::dat_sha256 == ubc::lhiver::dat_sha256, "Marcello profile names the 2.0.4 script");
  require(m::symbol_count == ubc::lhiver::symbol_count, "Marcello symbol count matches");
  check_functions("Marcello base functions have records", m::profile, ubc::lhiver::functions);
  check_functions("navigation functions have records", m::profile,
                  ubc::recipe_navigation::functions);
  require(record_of(d::profile, ubc::dndr::anvil::spare) &&
              record_of(d::profile, ubc::dndr::anvil::sibling),
          "DNdR anvil code reserve has records");
  check_lookup(m::profile);
  check_lookup(d::profile);
  std::printf("script profiles: %u checks, %u failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
