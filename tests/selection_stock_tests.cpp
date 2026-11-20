// SPDX-License-Identifier: MIT
#include "ubc/selection_stock.hpp"
#include <cstdio>

namespace {
unsigned checks = 0, failures = 0;
void check(bool ok, const char* name) {
  ++checks;
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", name); }
}
ubc::PackedInventorySnapshot inventory() {
  ubc::PackedInventorySnapshot s;
  s.header_known = s.listed_known = s.packed_known = s.storage_totals_known = true;
  s.stable_boundary_verified = s.snapshot_unchanged = true;
  s.listed = {41, 1, 22, 534, 60, 1};
  s.packed = {0, 0, 0, 0, 0, 37};
  s.total = {41, 1, 22, 534, 60, 38};
  s.listed_occurrences.fill(1);
  s.packed_occurrences[5] = 1;
  s.mixed_representation[5] = true;
  return s;
}
}

int main() {
  using ubc::OriginalRecipe;
  const ubc::HandsSnapshot pan{{0, -1, -1}, {1, 16608, 1}};
  const ubc::HandsSnapshot flask{{0, -1, -1}, {1, 16604, 1}};
  const auto base = inventory();
  auto stock = ubc::selection_stock(OriginalRecipe::meat, base, pan);
  check(stock.known() && stock.maximum == 41 && stock.reserved[0] == 0, "stove pan is tool, not meat reserve");
  auto p = ubc::material_preview(OriginalRecipe::meat, stock, 10);
  check(p.valid && p.consumed[0] == 10 && p.remaining[0] == 31 && p.remaining[1] == 11,
        "ten meat preview conserves ingredients and output");
  check(!ubc::material_preview(OriginalRecipe::meat, stock, 0).valid &&
        !ubc::material_preview(OriginalRecipe::meat, stock, 42).valid, "no zero or unavailable amount");
  stock = ubc::selection_stock(OriginalRecipe::healing_essence, base, flask);
  check(stock.known() && stock.maximum == 23 && stock.reserved[2] == 1,
        "alchemy total includes one reserved hand flask");
  for (const std::uint32_t n : {1u, 3u, 5u, 23u}) {
    p = ubc::material_preview(OriginalRecipe::healing_essence, stock, n);
    check(p.valid && p.available[2] == 23 && p.consumed[2] == static_cast<int>(n) &&
          p.remaining[2] == 23 - static_cast<int>(n), "flasks include hand exactly once");
    check(p.remaining[3] == 534 - static_cast<int>(n) && p.remaining[4] == 60 - static_cast<int>(n) &&
          p.remaining[5] == 38 + static_cast<int>(n), "herbs and mixed stored output accounted exactly");
  }
  for (unsigned variant = 0; variant < 13; ++variant) {
    auto s = base;
    switch (variant) {
      case 0: s.header_known = false; break;
      case 1: s.listed_known = false; break;
      case 2: s.packed_known = false; break;
      case 3: s.storage_totals_known = false; break;
      case 4: s.stable_boundary_verified = false; break;
      case 5: s.snapshot_unchanged = false; break;
      case 6: s.issue = ubc::PackedInventoryIssue::boundary_unverified; break;
      case 7: s.total[0] = -1; break;
      case 8: s.listed[0] = -1; break;
      case 9: s.packed[0] = -1; break;
      case 10: s.total[0] += 1; break;
      case 11: s.packed[0] = INT32_MAX; break;
      case 12: s.listed[1] = s.total[1] = INT32_MAX; break;
    }
    check(!ubc::selection_stock(OriginalRecipe::meat, s, pan).known(), "unknown/corrupt/overflow stock rejected");
  }
  for (unsigned ingredient : {2u, 3u, 4u}) {
    for (unsigned variant = 0; variant < 6; ++variant) {
      auto s = base;
      switch (variant) {
        case 0: s.duplicate_packed_target[ingredient] = true; break;
        case 1: s.has_active_marker[ingredient] = true; break;
        case 2: s.mixed_representation[ingredient] = true; break;
        case 3: s.packed_occurrences[ingredient] = 2; break;
        case 4: s.listed_occurrences[ingredient] = 2; break;
        case 5: ++s.packed[ingredient]; ++s.total[ingredient]; break;
      }
      check(!ubc::selection_stock(OriginalRecipe::healing_essence, s, flask).known(),
            "ambiguous native ingredient counting blocked");
    }
  }
  for (unsigned variant = 0; variant < 6; ++variant) {
    auto h = flask;
    switch (variant) {
      case 0: h.left.state = -1; break;
      case 1: h.left = {1, 16604, 1}; break;
      case 2: h.right.state = 0; break;
      case 3: h.right.state = 2; break;
      case 4: h.right.amount = 2; break;
      case 5: h.right.symbol = 16608; break;
    }
    check(!ubc::selection_stock(OriginalRecipe::healing_essence, base, h).known(), "exact alchemy hand contract required");
  }
  auto limited = base;
  limited.total[2] = limited.listed[2] = 0;
  stock = ubc::selection_stock(OriginalRecipe::healing_essence, limited, flask);
  check(stock.known() && stock.maximum == 1, "one hand flask suffices for one");
  check(ubc::material_preview(OriginalRecipe::healing_essence, stock, 1).valid &&
        !ubc::material_preview(OriginalRecipe::healing_essence, stock, 3).valid,
        "original three option cannot bypass missing extra flasks");
  limited.total[3] = limited.listed[3] = 0;
  stock = ubc::selection_stock(OriginalRecipe::healing_essence, limited, flask);
  check(stock.known() && stock.maximum == 0, "known empty stock remains distinct from unknown");
  check(!ubc::material_preview(OriginalRecipe::healing_essence, stock, 1).valid, "empty ingredient cannot produce");
  limited = base; limited.total[2] = limited.listed[2] = INT32_MAX;
  check(!ubc::selection_stock(OriginalRecipe::healing_essence, limited, flask).known(), "reserved flask sum overflow rejected");
  limited = base; limited.packed[5] = 0; limited.listed[5] = limited.total[5] = INT32_MAX;
  check(!ubc::selection_stock(OriginalRecipe::healing_essence, limited, flask).known(), "result stack overflow rejected");
  std::printf("SelectionStock: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
