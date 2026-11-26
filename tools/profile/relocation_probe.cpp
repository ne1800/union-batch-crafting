// SPDX-License-Identifier: MIT
// Local, read-only check of a generated script profile against a parser
// image (tools/profile/parser_image.py). Runs relocate() and prints the
// active recipes, the rejected ones with reason and the scan time as JSON.
// Usage: relocation_probe <marcello|dndr> <image> [repeat]
#include "ubc/catalogs/generated/dndr_26_fingerprints.hpp"
#include "ubc/catalogs/generated/marcello_204_fingerprints.hpp"
#include "ubc/engine/script_relocation.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace {
namespace sr = ubc::script_relocation;

struct Region {
  std::uint32_t address = 0;
  std::vector<std::uint8_t> bytes;
};
std::vector<Region> regions;

bool read(void*, std::uint32_t address, void* output, std::size_t size) noexcept {
  for (const auto& region : regions)
    if (address >= region.address &&
        std::uint64_t{address} + size <= std::uint64_t{region.address} + region.bytes.size()) {
      std::memcpy(output, region.bytes.data() + (address - region.address), size);
      return true;
    }
  return false;
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
    regions.push_back(std::move(region));
  }
  return true;
}

template <std::size_t Names, std::size_t Functions, std::size_t Data, std::size_t Recipes>
int run(const sr::ReferenceProfile& profile, int repeat) {
  using Storage = sr::Storage<Names, Functions, Data, Recipes>;
  auto storage = std::make_unique<Storage>();
  const ubc::ReadOnlyMemory memory{nullptr, &read};
  sr::Relocation result{};
  double best = 1e9;
  for (int i = 0; i < repeat; ++i) {
    const auto begin = std::chrono::steady_clock::now();
    result = sr::relocate(memory, profile.parser_address, profile, storage->workspace());
    const std::chrono::duration<double, std::milli> elapsed =
        std::chrono::steady_clock::now() - begin;
    best = elapsed.count() < best ? elapsed.count() : best;
  }
  std::uint32_t texts = 0;
  for (std::size_t i = 0; i < profile.text_count; ++i) {
    std::uint32_t live = 0;
    texts += sr::live_text(memory, result, profile, storage->workspace(),
                           profile.texts[i].reference_index, live);
  }
  std::printf("{\"valid\":%s,\"issue\":\"%s\",\"identity\":%s,\"active\":%u,\"total\":%u,"
              "\"names_found\":%u,\"names_missing\":%u,\"names_duplicate\":%u,"
              "\"texts\":%u,\"ms\":%.2f,\"rejected\":[",
              result.valid ? "true" : "false", ubc::script_fingerprint::issue_name(result.issue),
              result.identity ? "true" : "false", result.active, result.total, result.names_found,
              result.names_missing, result.names_duplicate, texts, best);
  bool first = true;
  for (std::size_t i = 0; i < profile.group_count; ++i) {
    const auto& state = storage->recipes[i];
    if (state.active)
      continue;
    const char* name = "";
    if (state.kind == sr::RecordKind::function)
      name = profile.names[profile.functions[state.record].name].text;
    else if (state.kind == sr::RecordKind::data)
      name = profile.names[profile.data[state.record].name].text;
    std::printf("%s{\"recipe\":%u,\"reason\":\"%s\",\"station\":%s,\"record\":\"%s\"}",
                first ? "" : ",", state.recipe, ubc::script_fingerprint::issue_name(state.reason),
                state.station ? "true" : "false", name[0] == '\xff' ? "" : name);
    first = false;
  }
  std::printf("]}\n");
  return result.valid ? 0 : 1;
}
} // namespace

int main(int argc, char** argv) {
  if (argc < 3 || !load(argv[2])) {
    std::fprintf(stderr, "usage: relocation_probe <marcello|dndr> <image> [repeat]\n");
    return 2;
  }
  const int repeat = argc > 3 ? std::max(1, std::atoi(argv[3])) : 1;
  const std::string campaign = argv[1];
  if (campaign == "marcello") {
    namespace p = ubc::script_profiles::marcello_204;
    return run<p::names.size(), p::functions.size(), p::data.size(), p::groups.size()>(p::profile,
                                                                                       repeat);
  }
  if (campaign == "dndr") {
    namespace p = ubc::script_profiles::dndr_26;
    return run<p::names.size(), p::functions.size(), p::data.size(), p::groups.size()>(p::profile,
                                                                                       repeat);
  }
  return 2;
}
