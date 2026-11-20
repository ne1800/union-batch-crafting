// SPDX-License-Identifier: MIT
#include "ubc/stations/bow/bow_menu_checks.hpp"
#include "ubc/core/selection_snapshot.hpp"
#include "bow_item_test_values.hpp"
#include <cstdio>
namespace n = ubc::bow_recipe_navigation;
namespace c = ubc::bow_recipe_catalog;
namespace {
unsigned checks = 0, failures = 0;
void check(bool b, int line) {
  ++checks;
  if (!b) {
    ++failures;
    std::fprintf(stderr, "failure line %d\n", line);
  }
}
#define CHECK(v) check((v), __LINE__)

// The 20 pairs come from the measurement block in frame 11491 of world
// run 3. Construct test names independently of recipe catalog names.
void check_world3_print_names(const bow_item_test::Dat* archive) {
  namespace rt = ubc::bow_recipe_runtime;
  bow_test::Fixture fixture(c::recipes.front());
  auto& image = fixture.image;
  for (unsigned i = 0; i < c::recipes.size(); ++i) {
    const auto& recipe = c::recipes[i];
    const auto assembly = rt::assemble(recipe);
    const auto symbol = 53818u + 4 * i;
    const auto observed = "\xff" + std::to_string(23400 + 4 * i);
    const auto* binding = assembly.binding(symbol);
    CHECK(binding && binding->name == observed);
    if (!archive)
      continue;

    fixture.populate(recipe);
    bow_item_test::text(image, bow_test::name_at(symbol), observed);
    const auto profile = assembly.profile();
    for (std::size_t j = 0; j < profile.function_count; ++j) {
      const auto& f = profile.functions[j];
      if (f.byte_offset > archive->code.size() ||
          f.byte_length > archive->code.size() - f.byte_offset) {
        CHECK(false);
        return;
      }
      std::memcpy(image.at(bow_test::code + f.byte_offset, f.byte_length),
                  archive->code.data() + f.byte_offset, f.byte_length);
    }
    for (const auto& print : recipe.prints) {
      const auto& value = archive->strings.at(print.symbol);
      const auto object = bow_test::array_at(print.symbol);
      image.put(bow_test::symbol(print.symbol) + 0x18, object);
      image.block(object, 20);
      image.put(object + 8, object + 32);
      image.put(object + 12, static_cast<std::uint32_t>(value.size()));
      bow_item_test::text(image, object + 32, value);
    }
  }
  if (!archive)
    return;

  const auto parser = ubc::lhiver::game_parser;
  const auto memory = image.memory();
  for (const auto& recipe : c::recipes)
    CHECK(rt::verify_recipe(memory, parser, recipe, rt::assemble(recipe)).valid);
  CHECK(ubc::bow_menu::page(memory, parser, 1, nullptr) == n::Page::root);
  image.put(bow_test::symbol(20862) + 0x18, 1);
  CHECK(ubc::bow_menu::page(memory, parser, 1, nullptr) == n::Page::upgrades);

  const auto& first = c::recipes.front();
  const auto assembly = rt::assemble(first);
  bow_item_test::text(image, bow_test::name_at(53818),
                      "\xff"
                      "23401");
  const auto stale = rt::verify(memory, parser, assembly);
  CHECK(!stale.matches && stale.issue == ubc::runtime_script::Issue::symbol_name &&
        stale.failed_symbol == 53818);
  CHECK(ubc::bow_menu::page(memory, parser, 1, nullptr) == n::Page::unknown);
  bow_item_test::text(image, bow_test::name_at(53818),
                      "\xff"
                      "23400");
  *image.at(bow_test::array_at(53818) + 32, 1) ^= 1;
  const auto damaged = rt::verify_recipe(memory, parser, first, assembly);
  CHECK(!damaged.valid && damaged.local.matches && damaged.failed_symbol == 53818 &&
        damaged.string_issue == ubc::alchemy_flask_runtime::Issue::string);
}

unsigned contracts = 0, category_reads = 0, stocks = 0, skipped = 0, bad_text = 0, wrong_view = 0,
         raw_npc = 0;
void record(const char* field, const char* wanted, const char* actual, int ok, int recipe,
            int symbol) {
  if (!std::strcmp(field, "page.recipe_contract")) {
    CHECK(recipe == static_cast<int>(c::recipes[contracts].id));
    CHECK(!std::strcmp(wanted, "ok") && !std::strcmp(actual, "parser_unreadable") && ok == 0);
    ++contracts;
  } else if (!std::strcmp(field, "page.m_bows")) {
    CHECK(symbol == 20862 && ok == 0 && !std::strcmp(actual, "parser_unreadable"));
    ++category_reads;
  } else if (!std::strcmp(field, "recipe.stock")) {
    CHECK(recipe >= 0x8001 && symbol > 0 && ok == -1 && !std::strcmp(actual, "ungelesen"));
    ++stocks;
  } else if (!std::strcmp(field, "catalogue.inventory") || !std::strcmp(field, "catalogue.hands")) {
    if (ok == -1 && !std::strcmp(actual, "ungelesen"))
      ++skipped;
  } else if (!std::strcmp(field, "recipe.contract") && symbol == 14374) {
    CHECK(ok == 0 && !std::strcmp(actual, "Drucktext"));
    ++bad_text;
  } else if (!std::strcmp(field, "dialog.view_vtable")) {
    CHECK(ok == 0 && !std::strcmp(actual, "4660"));
    ++wrong_view;
  } else if (!std::strcmp(field, "dialog.npc")) {
    CHECK(ok == 1 && !std::strcmp(actual, "4096"));
    ++raw_npc;
  }
}
}
int main(int argc, char** argv) {
  if (argc == 3 && std::string(argv[1]) == "--local-archive") {
    const bow_item_test::Dat archive(argv[2]);
    check_world3_print_names(&archive);
  } else if (argc == 1) {
    check_world3_print_names(nullptr);
  } else
    return 2;
  const ubc::ReadOnlyMemory unreadable{};
  ubc::BowMenuTrace trace{record, true};
  CHECK(ubc::bow_menu::page(unreadable, ubc::lhiver::game_parser, 2, &trace) == n::Page::unknown);
  CHECK(contracts == 20 && category_reads == 1);
  ubc::bow_menu::catalogue(trace, ubc::BowRecipeCatalogueCapture{}, ubc::CraftingSnapshot{});
  CHECK(stocks == 80 && skipped == 2);
  ubc::BowRecipeCatalogueCapture text_failure;
  text_failure.recipes[0].evaluated = true;
  auto& profile = text_failure.diagnostic.profiles[0];
  profile.local.matches = true;
  profile.failed_symbol = 14374;
  profile.string_issue = ubc::alchemy_flask_runtime::Issue::string;
  ubc::bow_menu::catalogue(trace, text_failure, ubc::CraftingSnapshot{});
  CHECK(bad_text == 1);
  std::array<std::uint32_t, 256> words{};
  words[(0x100 + 0x1c) / 4] = 0x200;
  words[(0x100 + 0x20) / 4] = words[(0x100 + 0x24) / 4] = 0x1000;
  words[0x200 / 4] = 0x1234;
  ubc::ReadOnlyMemory dialog{
      words.data(), [](void* data, std::uint32_t at, void* out, std::size_t size) noexcept {
        if (at > 1024 || size > 1024 - at)
          return false;
        std::memcpy(out, static_cast<const char*>(data) + at, size);
        return true;
      }};
  CHECK(!ubc::read_selection_dialog(dialog, 0x100, &trace, 0x1000).valid);
  CHECK(wrong_view == 1 && raw_npc == 1);
  contracts = category_reads = 0;
  trace.full = false;
  CHECK(ubc::bow_menu::page(unreadable, ubc::lhiver::game_parser, 1, &trace) == n::Page::unknown);
  CHECK(contracts == 1 && category_reads == 0);
  ubc::SelectionInfoRows collection;
  collection.begin(42, true);
  ubc::SelectionInfo info;
  info.valid = true;
  info.instance = 20865;
  collection.append(42, info, 0, 1, true);
  collection.append(42, info, 1, 2, true);
  CHECK(!collection.valid && collection.count == 1 && collection.actual == 20865);
  CHECK(!std::strcmp(collection.rejected_field, "info.duplicate_instance"));
  collection.begin(42, true);
  CHECK(collection.valid && !collection.rejected_field);
  for (const auto& r : c::recipes)
    for (const auto source : {n::Page::root, n::Page::upgrades})
      for (const auto amount : {1u, 2u, r.callback.maximum_quantity}) {
        const auto p = n::make_plan(source, r.id, amount);
        CHECK(p.valid() && n::structurally_valid(p));
        CHECK(p.count == (source == n::Page::root ? 2u : 1u));
        CHECK(p.effect() && p.effect()->kind == n::Kind::effect &&
              p.effect()->owner_instance == r.owner.instance &&
              p.effect()->function_symbol == r.callback.function->symbol);
        CHECK(p.effect()->before == n::Page::upgrades && p.effect()->after == n::Page::unknown &&
              p.effect()->invocation == c::Invocation::info);
        if (source == n::Page::root)
          CHECK(p.steps[0].function_symbol == 20870 && p.steps[0].owner_instance == 20868 &&
                p.steps[0].kind == n::Kind::open_category);
        const auto profile = n::step_profile(p, 0);
        CHECK(profile.valid());
        CHECK(!n::step_profile(p, p.count).valid());
        unsigned effects = 0;
        auto page = source;
        for (std::size_t i = 0; i < p.count; ++i) {
          CHECK(p.steps[i].before == page);
          effects += p.steps[i].kind == n::Kind::effect ? 1u : 0u;
          page = p.steps[i].after;
        }
        CHECK(effects == 1 && page == n::Page::unknown);
        for (unsigned mutation = 0; mutation < 8; ++mutation) {
          auto bad = p;
          switch (mutation) {
          case 0:
            bad.count = 3;
            break;
          case 1:
            bad.quantity = 0;
            break;
          case 2:
            bad.steps[0].owner_instance++;
            break;
          case 3:
            bad.steps[0].function_symbol++;
            break;
          case 4:
            bad.steps[0].invocation = c::Invocation::choice;
            break;
          case 5:
            bad.steps[0].before = n::Page::unknown;
            break;
          case 6:
            bad.target = static_cast<c::RecipeId>(1);
            break;
          case 7:
            bad.steps[bad.count - 1].kind = n::Kind::open_category;
            break;
          }
          CHECK(!n::structurally_valid(bad) && !n::step_profile(bad, 0).valid());
        }
      }
  CHECK(n::classify_page({true, 16, 0, c::Invocation::info}) == n::Page::root);
  CHECK(n::classify_page({true, 16, 1, c::Invocation::info}) == n::Page::upgrades);
  for (const auto category : {-1, 2, INT32_MAX})
    CHECK(n::classify_page({true, 16, category, c::Invocation::info}) == n::Page::unknown);
  CHECK(n::classify_page({false, 16, 1, c::Invocation::info}) == n::Page::unknown);
  CHECK(n::classify_page({true, 17, 1, c::Invocation::info}) == n::Page::unknown);
  CHECK(n::classify_page({true, 16, 1, c::Invocation::choice}) == n::Page::unknown);
  CHECK(!n::make_plan(n::Page::unknown, c::recipes[0].id, 1).valid());
  CHECK(!n::make_plan(n::Page::root, c::recipes[0].id, UINT32_MAX).valid());
  std::printf("bow_recipe_navigation_tests: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
