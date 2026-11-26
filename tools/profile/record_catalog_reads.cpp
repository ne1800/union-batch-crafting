// SPDX-License-Identifier: MIT
// Read-only recorder for tools/profile/generate-script-profile-WRITES.py.
// Runs the plugin's own per-recipe verifiers against a parser image (see
// tools/profile/parser_image.py) and prints, per recipe, every symbol slot
// and code range they read together with the catalog items. The image holds
// game data; this program only reads it and writes JSON to stdout.
// Usage: record_catalog_reads <marcello|dndr> <image>
#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/core/recipe_runtime.hpp"
#include "ubc/stations/alchemy/alchemy_extra_execution.hpp"
#include "ubc/stations/anvil/anvil_recipe_runtime.hpp"
#include "ubc/stations/anvil/dndr_anvil.hpp"
#include "ubc/stations/bow/bow_recipe_runtime.hpp"
#include "ubc/stations/cool/cool_recipe_runtime.hpp"
#include "ubc/stations/grind/grind_recipe_runtime.hpp"
#include "ubc/stations/ore/ore_recipe_runtime.hpp"
#include "ubc/stations/spirit/spirit_recipe_runtime.hpp"
#include "ubc/stations/station_recipe_runtime.hpp"
#include "ubc/stations/tobacco/tobacco_runtime.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {
namespace cat = ubc::crafting_catalog;
using ubc::lhiver::Function;

constexpr std::uint32_t table_address = 0x20000000;
constexpr std::uint32_t code_address = 0x25000000;

struct Region {
  std::uint32_t address = 0;
  std::vector<std::uint8_t> bytes;
};

struct Recorder {
  std::vector<Region> regions;
  std::set<std::uint32_t> slots;
  std::map<std::uint32_t, std::uint32_t> code; // begin -> farthest end
  unsigned outside = 0;
};

Recorder recorder;

bool read(void*, std::uint32_t address, void* output, std::size_t size) noexcept {
  for (const auto& region : recorder.regions) {
    if (address < region.address ||
        std::uint64_t{address} + size > std::uint64_t{region.address} + region.bytes.size())
      continue;
    std::memcpy(output, region.bytes.data() + (address - region.address), size);
    const auto offset = address - region.address;
    if (region.address == table_address && size)
      for (auto slot = offset / 4u; slot <= (offset + size - 1u) / 4u; ++slot)
        recorder.slots.insert(static_cast<std::uint32_t>(slot));
    if (region.address == code_address) {
      auto& end = recorder.code[offset];
      end = std::max<std::uint32_t>(end, static_cast<std::uint32_t>(offset + size));
    }
    return true;
  }
  ++recorder.outside;
  return false;
}

struct Check {
  int symbol = 0;
  template <class A, class B> bool equal(const char*, A expected, B value) noexcept {
    return static_cast<long long>(expected) == static_cast<long long>(value);
  }
};

std::string quoted(const char* text) {
  std::string out = "\"";
  for (const auto* p = text ? text : ""; *p; ++p) {
    const auto c = static_cast<unsigned char>(*p);
    if (c == '"' || c == '\\') {
      out += '\\';
      out += static_cast<char>(c);
    } else if (c < 0x20 || c >= 0x7f) {
      char escaped[8];
      std::snprintf(escaped, sizeof escaped, "\\u%04x", c);
      out += escaped;
    } else {
      out += static_cast<char>(c);
    }
  }
  return out + "\"";
}

struct Own {
  std::string text;
  void add(const char* role, const Function* function) {
    if (!function)
      return;
    char line[160];
    std::snprintf(line, sizeof line, "%s[%s,%u]", text.empty() ? "" : ",", quoted(role).c_str(),
                  function->symbol);
    text += line;
  }
  void owner(const ubc::recipe_catalog::Owner& owner) {
    add("owner", owner.constructor);
    add("owner", owner.condition);
    add("owner", owner.information);
  }
};

template <class View> void amounts(const char* key, const View& view) {
  std::printf(",%s:[", quoted(key).c_str());
  for (std::size_t i = 0; i < view.count; ++i)
    std::printf("%s%d", i ? "," : "", static_cast<int>(view[i].symbol));
  std::printf("]");
}

template <class Recipe> void begin(const Recipe& recipe, bool& first) {
  std::printf("%s\n{\"id\":%u,\"station\":%u,\"route\":%u", first ? "" : ",", recipe.id,
              static_cast<unsigned>(recipe.station), static_cast<unsigned>(recipe.route));
  first = false;
  std::printf(",\"targets\":[");
  for (std::size_t i = 0; i < recipe.targets.count; ++i)
    std::printf("%s%d", i ? "," : "", static_cast<int>(recipe.targets[i].symbol));
  std::printf("]");
  amounts("costs", recipe.costs);
  amounts("outputs", recipe.outputs);
}

void finish(bool matches, const Own& own, const std::string& extra) {
  std::printf(",\"own\":[%s]%s,\"matches\":%s,\"outside\":%u,\"slots\":[", own.text.c_str(),
              extra.c_str(), matches ? "true" : "false", recorder.outside);
  bool first = true;
  for (const auto slot : recorder.slots) {
    std::printf("%s%u", first ? "" : ",", slot);
    first = false;
  }
  std::printf("],\"code\":[");
  first = true;
  for (const auto& [offset, end] : recorder.code) {
    std::printf("%s[%u,%u]", first ? "" : ",", offset, end);
    first = false;
  }
  std::printf("]}");
}

void reset() {
  recorder.slots.clear();
  recorder.code.clear();
  recorder.outside = 0;
}

bool load(const char* path) {
  std::ifstream input(path, std::ios::binary);
  char magic[4] = {};
  std::uint32_t count = 0;
  input.read(magic, 4);
  input.read(reinterpret_cast<char*>(&count), 4);
  if (!input || std::memcmp(magic, "UBCI", 4) != 0 || count > 64)
    return false;
  for (std::uint32_t i = 0; i < count; ++i) {
    Region region;
    std::uint32_t length = 0;
    input.read(reinterpret_cast<char*>(&region.address), 4);
    input.read(reinterpret_cast<char*>(&length), 4);
    region.bytes.resize(length);
    input.read(reinterpret_cast<char*>(region.bytes.data()), length);
    if (!input)
      return false;
    recorder.regions.push_back(std::move(region));
  }
  return true;
}

bool verify(const ubc::ReadOnlyMemory& memory, const cat::Descriptor& recipe) {
  const auto parser = ubc::lhiver::game_parser;
  switch (recipe.route) {
  case cat::Route::base:
    return ubc::recipe_runtime::verify(memory, parser, *recipe.base).matches;
  case cat::Route::station:
    return ubc::station_recipe_runtime::verify(memory, parser, *recipe.station_descriptor).matches;
  case cat::Route::alchemy_extra:
    return ubc::alchemy_extra_execution::verify_recipe(memory, *recipe.alchemy_extra).valid;
  case cat::Route::anvil:
    return ubc::anvil_recipe_runtime::verify(memory, parser, *recipe.anvil).matches;
  case cat::Route::ore:
    return ubc::ore_recipe_runtime::verify(memory, parser, *recipe.ore).matches;
  case cat::Route::spirit:
    return ubc::spirit_recipe_runtime::verify_recipe(
               memory, parser, ubc::spirit_recipe_runtime::assemble(*recipe.spirit))
        .valid;
  case cat::Route::bow:
    return ubc::bow_recipe_runtime::verify_recipe(memory, parser, *recipe.bow,
                                                  ubc::bow_recipe_runtime::assemble(*recipe.bow))
        .valid;
  case cat::Route::cool:
    return ubc::cool_recipe_runtime::verify_recipe(memory, parser, *recipe.cool).valid;
  case cat::Route::grind:
    return ubc::grind_recipe_runtime::verify_recipe(memory, parser, *recipe.grind).valid;
  case cat::Route::tobacco: {
    Check check;
    return ubc::tobacco::verify_script(memory, check);
  }
  default:
    return false;
  }
}
} // namespace

int main(int argc, char** argv) {
  if (argc != 3 || !load(argv[2])) {
    std::fprintf(stderr, "usage: record_catalog_reads <marcello|dndr> <image>\n");
    return 2;
  }
  const std::string campaign = argv[1];
  const ubc::ReadOnlyMemory memory{nullptr, &read};
  std::printf("{\"campaign\":%s,\"recipes\":[", quoted(campaign.c_str()).c_str());
  bool first = true;
  if (campaign == "marcello") {
    for (const auto& recipe : cat::recipes) {
      reset();
      begin(recipe, first);
      Own own;
      if (recipe.tobacco) {
        own.add("choice", recipe.tobacco->callback.function);
        own.owner(recipe.tobacco->profile.owner);
      }
      if (recipe.base) {
        for (const auto& owner : recipe.base->owners)
          own.owner(owner);
        for (const auto& callback : recipe.base->callbacks)
          own.add("callback", callback.function);
        own.add("station_entry", recipe.base->station_entry);
        own.add("station_end", recipe.base->station_end);
      }
      const bool matches = verify(memory, recipe);
      finish(matches, own, {});
    }
  } else if (campaign == "dndr") {
    for (const auto& recipe : cat::dndr_recipes) {
      reset();
      begin(recipe, first);
      Own own;
      std::string extra;
      bool matches = false;
      if (recipe.route == cat::Route::dndr) {
        const auto& profile =
            recipe.station == cat::Station::anvil ? ubc::dndr::anvil::profile : ubc::dndr::profile;
        matches =
            ubc::runtime_script::verify_local_contract(memory, ubc::lhiver::game_parser, profile)
                .matches;
      } else {
        Check check;
        matches = ubc::tobacco::verify_script(memory, check, true);
      }
      if (const auto* formula = recipe.dndr_recipe) {
        own.owner(*formula->effect);
        if (formula->at_anvil()) {
          own.add("spare", &ubc::dndr::anvil::spare);
          own.add("sibling", &ubc::dndr::anvil::sibling);
        }
        extra = ",\"ingredients\":[";
        bool next = false;
        for (const auto& ingredient : formula->ingredients) {
          if (!ingredient.symbol)
            continue;
          extra += (next ? "," : "") + std::to_string(ingredient.symbol);
          next = true;
        }
        extra += "],\"products\":[" + std::to_string(formula->products[0]) + "," +
                 std::to_string(formula->products[1]) + "]";
      } else if (recipe.tobacco) {
        own.add("choice", recipe.tobacco->callback.function);
        own.owner(recipe.tobacco->profile.owner);
      }
      finish(matches, own, extra);
    }
  } else {
    std::fprintf(stderr, "unknown campaign %s\n", campaign.c_str());
    return 2;
  }
  std::printf("\n]}\n");
  return 0;
}
