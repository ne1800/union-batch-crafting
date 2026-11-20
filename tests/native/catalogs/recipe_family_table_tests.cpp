// SPDX-License-Identifier: MIT
#include "ubc/catalogs/crafting_catalog.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
namespace cat = ubc::crafting_catalog;
using std::to_string;

const char* station_name(cat::Station station) {
  switch (station) {
  case cat::Station::stove:
    return "stove";
  case cat::Station::alchemy:
    return "alchemy";
  case cat::Station::leather:
    return "leather";
  case cat::Station::saw:
    return "saw";
  case cat::Station::forge:
    return "forge";
  case cat::Station::anvil:
    return "anvil";
  case cat::Station::bow:
    return "bow";
  case cat::Station::cool:
    return "cool";
  case cat::Station::grind:
    return "grind";
  }
  throw std::runtime_error("Unbekannte Station");
}

const char* completion_name(cat::Completion completion) {
  switch (completion) {
  case cat::Completion::ready_same_station:
    return "ready_same_station";
  case cat::Completion::end_production:
    return "end_production";
  }
  throw std::runtime_error("Unbekannter Stationsabschluss");
}

// Identify a static reference by its actual address in the source
// catalog. ID and position in the shared catalog are insufficient.
template <class T, std::size_t N>
std::string source_row(const T* bound, const std::array<T, N>& family, const char* header) {
  for (std::size_t i = 0; i < N; ++i) {
    if (bound == &family[i])
      return std::string(header) + "#recipes[" + to_string(i) + "]";
  }
  throw std::runtime_error(std::string("Fremder Profilverweis in ") + header);
}

struct Binding {
  const char* route;
  std::string source;
};

Binding binding(const cat::Descriptor& d) {
  const std::array<const void*, 10> links{d.base,          d.station_descriptor,
                                          d.alchemy_extra, d.anvil,
                                          d.ore,           d.spirit,
                                          d.bow,           d.cool,
                                          d.grind,         d.tobacco};
  if (std::count(links.begin(), links.end(), nullptr) != 9)
    throw std::runtime_error("Genau ein gebundenes Quellprofil erwartet");
  using namespace ubc;
  switch (d.route) {
  case cat::Route::base:
    return {"base", source_row(d.base, recipe_catalog::recipes, "recipe_catalog.hpp")};
  case cat::Route::station:
    return {"station", source_row(d.station_descriptor, station_recipe_catalog::recipes,
                                  "station_recipe_catalog.hpp")};
  case cat::Route::alchemy_extra:
    return {"alchemy_extra", source_row(d.alchemy_extra, alchemy_recipe_catalog::recipes,
                                        "alchemy_recipe_catalog.hpp")};
  case cat::Route::anvil:
    return {"anvil",
            source_row(d.anvil, anvil_recipe_catalog::recipes, "anvil_recipe_catalog.hpp")};
  case cat::Route::ore:
    return {"ore", source_row(d.ore, ore_recipe_catalog::recipes, "ore_recipe_catalog.hpp")};
  case cat::Route::spirit:
    return {"spirit",
            source_row(d.spirit, spirit_recipe_catalog::recipes, "spirit_recipe_catalog.hpp")};
  case cat::Route::bow:
    return {"bow", source_row(d.bow, bow_recipe_catalog::recipes, "bow_recipe_catalog.hpp")};
  case cat::Route::cool:
    if (d.cool == &cool_recipe_catalog::recipe)
      return {"cool", "cool_recipe_catalog.hpp#recipe"};
    throw std::runtime_error("Fremder Kuehlprofilverweis");
  case cat::Route::tobacco:
    return {"tobacco", source_row(d.tobacco, tobacco::recipes, "tobacco_recipe_catalog.hpp")};
  case cat::Route::grind:
    return {"grind", source_row(d.grind, grind_recipe::recipes, "grind_recipe_plan.hpp")};
  }
  throw std::runtime_error("Unbekannte Route");
}

template <class Rows, class Format> std::string list(const Rows& rows, Format format) {
  std::string result;
  for (const auto& row : rows) {
    const auto field = format(row);
    if (field.empty())
      continue;
    if (!result.empty())
      result += ',';
    result += field;
  }
  return result.empty() ? "-" : result;
}

std::string amounts(cat::View<cat::ItemAmount> items) {
  return list(
      items, [](const auto& item) { return to_string(item.symbol) + "*" + to_string(item.units); });
}

std::string branches(const cat::Descriptor& d) {
  if (!d.spirit)
    return "-";
  const auto equal = d.spirit->branch_equal;
  const auto symbol = to_string(d.spirit->branch_symbol);
  return symbol + "!=" + to_string(equal) + ">" +
         amounts(cat::outputs_for(d, true, equal == 0 ? 1 : 0)) + "," + symbol + "=" +
         to_string(equal) + ">" + amounts(cat::outputs_for(d, true, equal));
}

std::string catalogue() {
  std::ostringstream out;
  out << "# ubc-rezeptfamilien-v1\n"
         "id\tstation\troute\tquelle\tname\teinheit\tabschluss\tziele\tkosten\tausgaben"
         "\tbogenminima\thand\tausgabezweige\tfester_mindestbesitz\n";
  for (const auto& d : cat::recipes) {
    const auto profile = binding(d);
    const auto targets =
        list(d.targets, [](const auto& t) { return to_string(t.symbol) + "=" + t.name; });
    const auto hands = list(d.targets, [&](const auto& t) -> std::string {
      const auto h = cat::hand_amounts(d, t.symbol);
      if (!h.consumed && !h.tool && !h.generated && !h.stored_absent)
        return {};
      return to_string(t.symbol) + "=" + to_string(h.consumed) + "/" + to_string(h.tool) + "/" +
             to_string(h.generated) + "/" + (h.stored_absent ? "1" : "0");
    });
    out << "0x" << std::hex << std::setw(4) << std::setfill('0') << d.id << std::dec << '\t'
        << station_name(d.station) << '\t' << profile.route << '\t' << profile.source << '\t'
        << d.name << '\t' << d.quantity_unit << '\t' << completion_name(d.completion) << '\t'
        << targets << '\t' << amounts(d.costs) << '\t' << amounts(d.outputs) << '\t'
        << (d.bow ? amounts(d.bow->minima) : "-") << '\t' << hands << '\t' << branches(d) << '\t'
        << (d.tobacco ? to_string(d.tobacco->targets[2].symbol) + ">=1" : "-") << '\n';
  }
  return out.str();
}

int compare(std::istream& table, std::istream& compiled) {
  std::string expected, actual;
  for (std::size_t line = 1;; ++line) {
    const bool has_expected = bool(std::getline(table, expected));
    const bool has_actual = bool(std::getline(compiled, actual));
    if (!has_expected && !has_actual && table.eof() && compiled.eof())
      return 0;
    if (!has_expected || !has_actual || expected != actual) {
      std::cerr << "Abweichung in Zeile " << line
                << "\nTabelle: " << (has_expected ? expected : "<fehlt>")
                << "\nKatalog: " << (has_actual ? actual : "<fehlt>") << '\n';
      return 1;
    }
  }
}
} // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--print") {
      std::cout << catalogue();
      return !std::cout;
    }
    if (argc > 2)
      return 2;
    const char* path = argc == 2 ? argv[1] : "data/rezeptfamilien-v1.tsv";
    std::ifstream table(path);
    if (!table)
      throw std::runtime_error(std::string("Tabelle nicht lesbar: ") + path);
    std::istringstream compiled(catalogue());
    return compare(table, compiled);
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
