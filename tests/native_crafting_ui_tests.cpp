// SPDX-License-Identifier: MIT
// Our own layout regressions. No game bytes or engine calls.
#include "ubc/ui/native_crafting_ui.hpp"
#ifdef _WIN32
#include "../src/ui/native_ui_language.hpp"
#endif
#include <string>
#include <vector>
#define UBC_CRAFTING_FIXTURE_ONLY
#include "crafting_catalog_tests.cpp"
using namespace crafting_test;
namespace ui = ubc::crafting_ui;
bool contains(const ui::Frame& f, const char* text) {
  for (std::size_t i = 0; i < f.count; ++i)
    if (std::strstr(f.lines[i].text.data(), text))
      return true;
  return false;
}
bool contains_wrapped(const ui::Frame& f, const char* text) {
  if (contains(f, text))
    return true;
  for (std::size_t i = 0; i < f.count; ++i)
    for (std::size_t j = 0; j < f.count; ++j)
      if (f.lines[i].x == f.lines[j].x && f.lines[j].y == f.lines[i].y + 260) {
        char joined[384]{};
        std::snprintf(joined, sizeof(joined), "%s %s", f.lines[i].text.data(),
                      f.lines[j].text.data());
        if (std::strstr(joined, text))
          return true;
      }
  return false;
}
void bounded(const ui::Frame& f) {
  check(f.valid && f.count && f.count <= f.lines.size(), "bounded frame contains submitted lines");
  for (std::size_t i = 0; i < f.count; ++i) {
    const auto& line = f.lines[i];
    check(line.x >= 0 && line.x < 8192 && line.y >= 0 && line.y < 8192 &&
              std::strlen(line.text.data()) <= line.character_limit,
          "all text stays within normalized coordinates and declared character budget");
    check(!std::strstr(line.text.data(), "..."),
          "ready menus and receipts show their complete labels");
    check(!std::strchr(line.text.data(), '*'),
          "no invisible asterisk is used as availability marker");
    bool ascii = true;
    for (const char ch : line.text)
      if (static_cast<unsigned char>(ch) >= 128)
        ascii = false;
    check(ascii, "native Gothic strings use ASCII transliteration");
    for (std::size_t j = 0; j < i; ++j) {
      const auto& earlier = f.lines[j];
      check(line.x != earlier.x || line.y != earlier.y,
            "same-column lines do not share a baseline");
      if (line.x == earlier.x)
        check(std::abs(line.y - earlier.y) >= 220,
              "same-column labels keep the existing native-font vertical clearance");
    }
  }
}
ubc::NativeCraftingSuccess success(const c::Descriptor& d) {
  ubc::NativeCraftingSuccess v;
  v.result_verified = v.restore_verified = true;
  v.receipt = 1;
  v.key = key(d.station);
  v.id = d.id;
  v.quantity = 3;
  const auto s = ready_state(d.id);
  v.stock_count = s.target_count;
  const auto outputs = c::outputs_for(d, s.branch_known, s.branch_value);
  v.produced_count = outputs.count;
  v.branch_known = s.branch_known;
  v.branch_value = s.branch_value;
  v.branch_output = s.branch_output;
  for (std::size_t i = 0; i < s.target_count; ++i) {
    const auto p = ubc::crafting_amount_preview(s.materials[i], v.quantity);
    v.stocks[i] = {s.materials[i].symbol,
                   static_cast<std::int32_t>(d.completion == c::Completion::ready_same_station
                                                 ? p.ready_storage
                                                 : p.end_storage)};
  }
  for (std::size_t i = 0; i < outputs.count; ++i)
    v.produced[i] = {outputs[i].symbol, static_cast<std::int32_t>(outputs[i].units * v.quantity)};
  return v;
}
using L = ubc::ui_text::Label;

std::vector<std::string> formats(std::string_view text) {
  std::vector<std::string> result;
  for (auto i = text.find('%'); i != text.npos; i = text.find('%', i + 1)) {
    const auto end = text.find_first_not_of("lz", i + 1);
    if (end == text.npos)
      return {"invalid"};
    result.emplace_back(text.substr(i, end - i + 1));
    i = end;
  }
  return result;
}

void test_translations() {
  namespace t = ubc::ui_text;
  for (bool german : {false, true}) {
    for (auto automatic : {L"", L"auto", L" AUTO "}) {
      check(t::choose(automatic, [=] { return german; }) ==
                (german ? &t::de::table : &t::en::table),
            "automatic selection uses the running game");
    }
    unsigned calls = 0;
    const auto detect = [&] {
      ++calls;
      return german;
    };
    check(t::choose(L" De ", detect) == &t::de::table &&
              t::choose(L"EN", detect) == &t::en::table &&
              t::choose(L"unknown", detect) == &t::en::table &&
              t::choose(L"de-extra", detect) == &t::en::table && calls == 0,
          "an explicit or unsupported INI value takes precedence");
  }
  const auto& en = t::en::table;
  const auto& de = t::de::table;
  for (const auto& locale : t::locales) {
    check(t::choose(locale.option, [] { return false; }) == locale.texts,
          "INI selects every registered table");
    for (std::size_t i = 0; i < en.labels.size(); ++i) {
      const char* source = en.labels[i];
      const char* translated = locale.texts->labels[i];
      const bool valid =
          source && *source && translated && *translated && formats(source) == formats(translated);
      if (!valid)
        std::fprintf(stderr, "%s: missing label or invalid format at key %zu (%s)\n", locale.code,
                     i, source ? source : "?");
      check(valid, "complete labels and matching printf arguments");
    }
    for (bool items : {false, true}) {
      auto expected = items ? en.items : en.recipes;
      auto names = items ? locale.texts->items : locale.texts->recipes;
      check(expected.count == names.count && names.count > 0, "matching name count");
      for (std::size_t i = 0; i < expected.count; ++i) {
        const auto id = expected.entries[i].id;
        const char* name = names.find(id, nullptr);
        if (!name || !*name)
          std::fprintf(stderr, "%s: missing %s %u\n", locale.code, items ? "item" : "recipe", id);
        check(name && *name && (i == 0 || expected.entries[i - 1].id < id),
              "each sorted key has a translation");
      }
    }
  }
  check(en.recipes.count == c::recipes.size(),
        "complete recipe inventory matches the current catalogue");
  for (const auto& recipe : c::recipes) {
    check(std::string_view(de.recipe(recipe.id)) == recipe.name &&
              std::string_view(de[t::unit(recipe.quantity_unit)]) == recipe.quantity_unit,
          "German names and units preserve the catalogue reference");
    for (const auto& target : recipe.targets) {
      const auto id = static_cast<std::uint32_t>(target.symbol);
      check(en.items.find(id, nullptr) && de.items.find(id, nullptr),
            "both languages name each recipe target");
    }
  }
  check(std::string_view(en.item(-1)) == "Unknown item" &&
            std::string_view(de.item(-1)) == "Unbekannter Gegenstand",
        "unknown item IDs use the selected language");
  for (const auto* text : {&en, &de}) {
    ui::Frame frame;
    for (const auto width : {640u, 1280u, 1920u}) {
      check(ui::compose_failure(frame, width, 720, *text) && contains(frame, (*text)[L::failure]),
            "localized failure title is shown");
      bounded(frame);
    }
  }
}

int main(int argc, [[maybe_unused]] char** argv) {
  if (argc != 1) {
#ifdef _WIN32
    if (argc == 4 && std::string_view(argv[1]) == "--check-game-ini") {
      const bool german_game = std::string_view(argv[2]) == "de";
      const auto language = ubc::ui_text::configured_language([=] { return german_game; });
      const char* selected = "";
      for (const auto& locale : ubc::ui_text::locales)
        if (locale.texts == language)
          selected = locale.code;
      std::printf("game_language=%s selected_language=%s\n", argv[2], selected);
      return std::string_view(argv[3]) == selected ? EXIT_SUCCESS : EXIT_FAILURE;
    }
#endif
    return EXIT_FAILURE;
  }
  test_translations();
  ui::Frame frame;
  std::set<std::int32_t> unknown_names;
  for (const auto& d : c::recipes) {
    for (const auto* language : {&ubc::ui_text::en::table, &ubc::ui_text::de::table}) {
      const auto& text = *language;
      for (const auto dimensions : std::array<std::array<unsigned, 2>, 4>{
               {{{1280, 720}}, {{1920, 1080}}, {{720, 720}}, {{640, 480}}}}) {
        auto snapshot = example(d.station, 7, d.grind ? d.grind->guilds[0] : 1);
        ubc::CraftingSession session;
        ubc::CraftingMenu menu;
        std::uint64_t f = 0;
        check(session.open(snapshot), "all station snapshots open for layout only");
        neutral(session, f);
        if (session.selected()->id != d.id) {
          session.select_recipe(d.id);
          neutral(session, f);
        }
        const auto composed =
            ui::compose(frame, session, menu, true, dimensions[0], dimensions[1], text);
        if (!composed)
          std::fprintf(stderr, "layout recipe=%u targets=%zu width=%u height=%u\n", d.id,
                       d.targets.count, dimensions[0], dimensions[1]);
        check(composed, "all canonical recipes compose at narrow and wide resolutions");
        bounded(frame);
        if (composed)
          for (const auto& target : d.targets)
            if (c::units(d.costs, target.symbol) || c::units(d.outputs, target.symbol) ||
                c::hand_amounts(d, target.symbol).tool)
              check(
                  contains_wrapped(frame, text.item(target.symbol)),
                  "every actual ingredient output and tool is visible without dropping a sixth row");
        check(contains_wrapped(frame, text.recipe(d.id)),
              "selected full recipe name remains visible outside truncated list rows");
        state(snapshot, d.id).contract = false;
        session.refresh(snapshot);
        neutral(session, f);
        check(
            ui::compose(frame, session, menu, false, dimensions[0], dimensions[1], text) &&
                contains(frame, language == &ubc::ui_text::en::table ? "[locked]" : "[gesperrt]") &&
                contains(frame, text[L::test_amount]),
            "known blocked entry uses readable marker and explicit UI-only action");
        const auto notice = success(d);
        check(ui::valid_success(notice) &&
                  ui::compose_success(frame, notice, dimensions[0], dimensions[1], text),
              "verified actual output and updated stocks compose a result notice");
        bounded(frame);
        check(contains(frame, text[L::success]) && contains(frame, text[L::updated_stock]),
              "success distinguishes actual result and updated storage");
      }
    }
    for (const auto& target : d.targets) {
      const bool known =
          std::strcmp(ubc::ui_text::de::table.item(target.symbol), "Unbekannter Gegenstand") != 0;
      if (!known && unknown_names.insert(target.symbol).second)
        std::fprintf(stderr, "unknown material symbol=%d\n", target.symbol);
      check(known, "all catalogue targets have verified name or explicit unnamed-prop label");
    }
    for (unsigned bad = 0; bad < 12; ++bad) {
      auto v = success(d);
      switch (bad) {
      case 0:
        v.result_verified = false;
        break;
      case 1:
        v.restore_verified = false;
        break;
      case 2:
        v.receipt = 0;
        break;
      case 3:
        v.key.hero = 0;
        break;
      case 4:
        v.id = UINT32_MAX;
        break;
      case 5:
        v.quantity = 0;
        break;
      case 6:
        ++v.produced[0].amount;
        break;
      case 7:
        v.produced[0].symbol = -1;
        break;
      case 8:
        v.produced_count = SIZE_MAX;
        break;
      case 9:
        --v.stock_count;
        break;
      case 10:
        v.stocks[0].stored = -1;
        break;
      default:
        v.stocks[1].symbol = v.stocks[0].symbol;
        break;
      }
      check(!ui::valid_success(v) &&
                !ui::compose_success(frame, v, 1280, 720, ubc::ui_text::de::table),
            "missing verification wrong amount or partial/ambiguous stock cannot claim success");
    }
  }
  {
    auto snapshot = example(Kind::leather);
    ubc::CraftingSession session;
    ubc::CraftingMenu menu;
    std::uint64_t f = 0;
    session.open(snapshot);
    neutral(session, f);
    session.select_recipe(0x1009);
    neutral(session, f);
    check(ui::compose(frame, session, menu, true, 1280, 720, ubc::ui_text::de::table) &&
              contains(frame, "6-9 / 9") && contains(frame, "schwarzem Troll"),
          "ninth leather row is reachable in four-line narrow viewport");
    check(ui::compose(frame, session, menu, true, 1920, 1080, ubc::ui_text::de::table) &&
              contains(frame, "4-9 / 9"),
          "wide viewport exposes six rows without losing selected item");
    check(!ui::compose(frame, session, menu, true, 0, 720, ubc::ui_text::de::table) &&
              !ui::compose(frame, session, menu, true, 1280, 0, ubc::ui_text::de::table),
          "invalid dimensions fail without native draw");
  }
  {
    auto snapshot = example(Kind::alchemy, 0);
    ubc::CraftingSession session;
    ubc::CraftingMenu menu;
    std::uint64_t f = 0;
    session.open(snapshot);
    neutral(session, f);
    check(ui::compose(frame, session, menu, true, 1280, 720, ubc::ui_text::de::table) &&
              contains(frame, "Vorschau: 1 Durchgang") && contains(frame, "Bedarf"),
          "known zero maximum labels one-cycle missing-material preview explicitly");
    snapshot = example(Kind::saw);
    ubc::CraftingSession saw;
    saw.open(snapshot);
    neutral(saw, f);
    check(ui::compose(frame, saw, menu, true, 1280, 720, ubc::ui_text::de::table) &&
              contains(frame, "Lose") && contains(frame, "Saege (Werkzeug") &&
              contains(frame, "Lager=bereit") && contains(frame, "Pfeil - Ausgabe"),
          "ammunition UI separates lots output and retained saw");
  }
  {
    auto snapshot = example(Kind::bow);
    ubc::CraftingSession session;
    ubc::CraftingMenu menu;
    std::uint64_t f = 0;
    session.open(snapshot, 32788);
    neutral(session, f);
    session.preset(2);
    neutral(session, f);
    check(
        ui::compose(frame, session, menu, true, 1280, 720, ubc::ui_text::de::table) &&
            contains(frame, "Vorrat fuer 7") && contains(frame, "Minimum 24") &&
            contains(frame, " - 20 = ") && contains(frame, " - 24 = "),
        "Bow UI separates stock bound, original steel minimum and actual steel/leather consumption");
    check(session.preset(7) &&
              ui::compose(frame, session, menu, true, 1280, 720, ubc::ui_text::de::table) &&
              contains(frame, "[gesperrt]"),
          "new Bow amount visibly remains blocked until its own current item-path proof");
  }
  for (const auto width : {640u, 720u, 1280u, 1920u})
    check(ui::compose_failure(frame, width, 720, ubc::ui_text::de::table) &&
              contains(frame, "nicht bestaetigt"),
          "failure notice remains separate from confirmed result at all supported widths");
  std::printf("native_crafting_ui portable layout: %u checks, %u failures\n", checks, failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
