// SPDX-License-Identifier: MIT
// Our own layout regressions. No game bytes or engine calls.
#include "ubc/ui/native_crafting_ui.hpp"
#ifdef _WIN32
#include "../../../src/ui/native_ui_language.hpp"
#endif
#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#define UBC_CRAFTING_FIXTURE_ONLY
#include "../catalogs/crafting_catalog_tests.cpp"
using namespace crafting_test;
namespace ui = ubc::crafting_ui;
namespace m = ubc::native_menu;

void test_menu_view() {
  constexpr std::uint32_t parent = 0x1000, inner = 0x2000;
  struct Memory {
    std::array<std::uint8_t, m::view_size> view{};
    std::uint32_t blit = m::view_blit;
  } memory;
  const auto read = [](void* context, std::uint32_t address, void* out, std::size_t size) noexcept {
    const auto& image = *static_cast<const Memory*>(context);
    if (address == inner && size == image.view.size())
      std::memcpy(out, image.view.data(), size);
    else if (address == m::menu_view_vtable + m::view_blit_slot * sizeof(image.blit) &&
             size == sizeof(image.blit))
      std::memcpy(out, &image.blit, size);
    else
      return false;
    return true;
  };
  m::MenuStorage menu{};
  m::write(menu.bytes.data(), m::menu_window, parent);
  m::write(menu.bytes.data(), m::menu_inner_window, inner);
  m::write(memory.view.data(), m::view_id, m::view_item);
  m::write(memory.view.data(), m::view_owner, parent);
  m::write<std::uint32_t>(memory.view.data(), m::vptr_offset, m::menu_view_vtable);
  ui::MenuView target{menu.bytes.data(), {&memory, read}};
  unsigned surface_calls = 0, text_calls = 0;
  const auto missing_texture = [&](void* view) {
    check(reinterpret_cast<std::uintptr_t>(view) == inner, "backdrop receives the checked view");
    ++surface_calls;
    return "ui_backdrop_texture_missing";
  };
  const auto draw = [&](void* view) {
    check(reinterpret_cast<std::uintptr_t>(view) == inner, "text receives the checked view");
    ++text_calls;
  };
  const auto backdrop = target.backdrop(missing_texture);
  check(backdrop.view_valid && !std::strcmp(backdrop.event, "ui_backdrop_texture_missing") &&
            target.submit(draw) && text_calls == 1 && surface_calls == 1,
        "a missing backdrop keeps text on the valid view");

  const auto rejected = [&](const char* reason, const char* message) {
    const auto result = target.backdrop(missing_texture);
    check(!result.view_valid && !std::strcmp(result.event, reason) && !target.submit(draw) &&
              text_calls == 1 && surface_calls == 1,
          message);
  };
  m::write(memory.view.data(), m::view_owner, parent + 1);
  rejected("ui_backdrop_view_mismatch", "the next frame rejects a changed owner before text");
  m::write(memory.view.data(), m::view_owner, parent);
  m::write<std::uint32_t>(memory.view.data(), m::vptr_offset, 0);
  rejected("ui_backdrop_view_mismatch", "readable item view with foreign vtable cannot draw");
  m::write<std::uint32_t>(memory.view.data(), m::vptr_offset, m::menu_view_vtable);
  memory.blit = 0;
  rejected("ui_backdrop_view_mismatch", "a changed blit slot rejects the view");
  memory.blit = m::view_blit;
  m::write(memory.view.data(), m::view_id, m::view_item + 1);
  rejected("ui_backdrop_view_mismatch", "a different item ID rejects the view");
  m::write(memory.view.data(), m::view_id, m::view_item);
  m::write(menu.bytes.data(), m::menu_window, 0u);
  rejected("ui_backdrop_view_mismatch", "missing parent rejects the view");
  m::write(menu.bytes.data(), m::menu_window, parent);
  m::write(menu.bytes.data(), m::menu_inner_window, inner + 1);
  rejected("ui_backdrop_view_mismatch", "an unreadable replacement view cannot draw");
  target.menu = nullptr;
  rejected("ui_backdrop_window_closed", "a closed window cannot draw");
}

void test_screen_init_digests() {
  // Check the production table against independently recorded hashes. The portable
  // test does not need a game executable or a copy of the patched function body.
  const auto screen_init =
      std::find_if(ui::open_sites.begin(), ui::open_sites.end(),
                   [](const ui::CodeSite& s) { return s.address == 0x004dcde0; });
  check(screen_init != ui::open_sites.end(), "the pre-open guard checks zCMenu::ScreenInit");
  if (screen_init == ui::open_sites.end())
    return;
  check(screen_init->length == 3066 &&
            !std::strcmp(screen_init->mismatch, "ui_screen_init_mismatch"),
        "ScreenInit keeps its full function length and mismatch key");
  constexpr auto original = ubc::lhiver::digest_literal(
      "a4a67a2b4b81506fe0d3e93a85a14abc5f58f082e2751802474730a23d85e86d");
  constexpr auto workshop = ubc::lhiver::digest_literal(
      "866a9f319a041c92bd4f6cf4d59ea5b89c27c558c243a08da8aca7334e785de1");
  check(screen_init->match_digest(original) == ui::CodeMatch::original,
        "ScreenInit accepts the original Gothic II 2.6fix digest");
  check(screen_init->match_digest(workshop) == ui::CodeMatch::alternative,
        "ScreenInit recognizes the observed L'Hiver Workshop digest");
  auto foreign = workshop;
  foreign.back() ^= 1u;
  check(screen_init->match_digest(foreign) == ui::CodeMatch::mismatch,
        "a one-bit difference from the Workshop digest is rejected");
  for (const auto& site : ui::open_sites)
    if (site.address != screen_init->address)
      check(!site.has_alternative && site.match_digest(workshop) == ui::CodeMatch::mismatch,
            "other pre-open functions still require their original digest");
}

void test_menu_code_guards() {
  const auto hash_bytes = [](const std::vector<std::uint8_t>& bytes) {
    ubc::Sha256 hash;
    hash.update(bytes.data(), bytes.size());
    return hash.finish();
  };
  struct Memory {
    std::uint32_t base = 0;
    std::vector<std::uint8_t> bytes;
    std::uint32_t blocked_begin = 0, blocked_end = 0;
  };
  const auto read = [](void* context, std::uint32_t address, void* out, std::size_t size) noexcept {
    const auto& mem = *static_cast<const Memory*>(context);
    if (address < mem.base || size > mem.bytes.size() ||
        address - mem.base > mem.bytes.size() - size)
      return false;
    if (mem.blocked_begin < mem.blocked_end && address < mem.blocked_end &&
        address + size > mem.blocked_begin)
      return false;
    std::memcpy(out, mem.bytes.data() + (address - mem.base), size);
    return true;
  };
  // Multi-chunk synthetic bodies matching ScreenInit (3066 B) and Blit (899 B).
  Memory mem;
  mem.base = ubc::selection_abi::menu_screen_init;
  mem.bytes.resize(4096);
  for (std::size_t i = 0; i < mem.bytes.size(); ++i)
    mem.bytes[i] = static_cast<std::uint8_t>((i * 73u + 19u) & 0xffu);
  std::vector<std::uint8_t> long_body(mem.bytes.begin(), mem.bytes.begin() + 3066);
  const auto original_screen_digest = hash_bytes(long_body);
  long_body.back() ^= 2u;
  std::vector<std::uint8_t> alt_body(mem.bytes.begin() + 3100, mem.bytes.begin() + 3100 + 899);
  const auto orig_alt_digest = hash_bytes(alt_body);
  alt_body[10] ^= 0x55u;
  alt_body[898] ^= 0xaau;
  const auto patched_alt_digest = hash_bytes(alt_body);
  const std::array<ui::CodeSite, 2> synthetic{{
      {mem.base, long_body.size(), original_screen_digest, "ui_screen_init_mismatch", true,
       hash_bytes(long_body)},
      {mem.base + 3100, 899, orig_alt_digest, "synthetic_alt_mismatch", true, patched_alt_digest},
  }};
  const ubc::ReadOnlyMemory view{&mem, read};
  bool workshop = true;
  const auto verify = [&] {
    return ui::verify_code(view, synthetic.data(), synthetic.size(), &workshop);
  };
  check(verify() == nullptr && !workshop,
        "matching primary digests pass without identifying the Workshop variant");
  for (const std::size_t offset : {std::size_t{0}, std::size_t{255}, std::size_t{256},
                                   long_body.size() / 2, long_body.size() - 1}) {
    mem.bytes[offset] ^= 1u;
    const char* mismatch = verify();
    check(mismatch && !std::strcmp(mismatch, "ui_screen_init_mismatch") && !workshop,
          "any modified byte across chunk boundaries fails with that site's mismatch key");
    mem.bytes[offset] ^= 1u;
  }
  mem.bytes[3100 + 10] ^= 0x55u;
  mem.bytes[3100 + 898] ^= 0xaau;
  check(verify() == nullptr && !workshop,
        "an alternative Blit alone does not identify a Workshop ScreenInit");
  mem.bytes[long_body.size() - 1] ^= 2u;
  check(verify() == nullptr && workshop,
        "the accepted alternative ScreenInit is identified after all checks pass");
  mem.bytes[3100 + 400] ^= 1u;
  const char* alt_mismatch = verify();
  check(alt_mismatch && !std::strcmp(alt_mismatch, "synthetic_alt_mismatch") && !workshop,
        "a later mismatch rejects the guard and clears the previous Workshop match");
  mem.bytes[3100 + 400] ^= 1u;
  mem.blocked_begin = mem.base + 512;
  mem.blocked_end = mem.base + 516;
  const char* unreadable = verify();
  check(unreadable && !std::strcmp(unreadable, "ui_screen_init_mismatch") && !workshop,
        "an unreadable interior chunk fails with that site's mismatch key");
  mem.blocked_begin = mem.blocked_end = 0;

  const ui::CodeSite zero_site{mem.base, 0, orig_alt_digest, "zero_mismatch"};
  const ui::CodeSite overflow_site{0xfffffff0u, 32, orig_alt_digest, "overflow_mismatch"};
  check(ui::verify_code(view, nullptr, 1) &&
            !std::strcmp(ui::verify_code(view, nullptr, 1), "ui_code_sites_invalid"),
        "null site table is rejected");
  check(ui::verify_code(view, &zero_site, 1) &&
            !std::strcmp(ui::verify_code(view, &zero_site, 1), "zero_mismatch"),
        "zero-length site is rejected");
  check(ui::verify_code(view, &overflow_site, 1) &&
            !std::strcmp(ui::verify_code(view, &overflow_site, 1), "overflow_mismatch"),
        "address-overflow site is rejected");

  // Table invariants and unreadable-memory rejection for every real site table.
  const auto check_table = [](const ui::CodeSite* sites, std::size_t count) {
    check(count > 0, "code guard table is non-empty");
    for (std::size_t i = 0; i < count; ++i) {
      check(sites[i].address >= m::image_base && sites[i].length > 0 &&
                sites[i].address - m::image_base <= m::image_size - sites[i].length &&
                sites[i].mismatch && sites[i].mismatch[0] != '\0',
            "every guarded function lies inside the Gothic2.exe image with a named log key");
      for (std::size_t j = 0; j < i; ++j)
        check(std::strcmp(sites[i].mismatch, sites[j].mismatch) != 0,
              "every guarded function in a table has a distinct mismatch log key");
    }
  };
  check_table(ui::open_sites.data(), ui::open_sites.size());
  check_table(ui::backdrop_sites.data(), ui::backdrop_sites.size());
  check_table(ui::string_sites.data(), ui::string_sites.size());
  check_table(ui::language_sites.data(), ui::language_sites.size());
  check_table(ui::menu_parser_sites.data(), ui::menu_parser_sites.size());

  const ubc::ReadOnlyMemory empty{
      nullptr, [](void*, std::uint32_t, void*, std::size_t) noexcept { return false; }};
  check(!std::strcmp(ui::verify_open(empty), "ui_menu_ctor_mismatch"),
        "unreadable open sites report the first pre-open mismatch");
  check(!std::strcmp(ui::verify_backdrop(empty), "ui_backdrop_insert_mismatch"),
        "unreadable backdrop sites keep their existing mismatch key");
  check(!std::strcmp(ui::verify_strings(empty), "ui_string_ctor_mismatch"),
        "unreadable zSTRING sites report ctor mismatch");
  check(!std::strcmp(ui::verify_language(empty), "ui_string_ctor_mismatch"),
        "unreadable language sites report zSTRING ctor mismatch");
  check(!std::strcmp(ui::verify_menu_parser(empty), "ui_menu_get_parser_mismatch"),
        "unreadable menu parser site reports its mismatch key");
#ifdef _WIN32
  const ubc::ReadOnlyMemory host_code{nullptr, ubc::windows_probe::copy_engine_code};
  check(ui::verify_open(host_code) != nullptr && ui::verify_strings(host_code) != nullptr,
        "the foreign Windows test host is rejected by live engine code checks");
#endif
}

int test_local_exe_code_guards(const char* path) {
  std::ifstream stream(path, std::ios::binary);
  std::vector<std::uint8_t> exe{std::istreambuf_iterator<char>(stream), {}};
  ubc::Sha256 file_hash;
  file_hash.update(exe.data(), exe.size());
  check(exe.size() >= 0x4c2000 && file_hash.finish() == m::executable_sha256,
        "local executable matches the verified Gothic II 2.6fix SHA-256");
  struct Image {
    std::vector<std::uint8_t>* bytes;
    std::uint32_t blocked_begin = 0, blocked_end = 0;
  } image{&exe};
  const auto read = [](void* context, std::uint32_t address, void* out, std::size_t size) noexcept {
    const auto& img = *static_cast<const Image*>(context);
    if (address < m::image_base)
      return false;
    const auto offset = static_cast<std::size_t>(address - m::image_base);
    if (size > img.bytes->size() || offset > img.bytes->size() - size)
      return false;
    if (img.blocked_begin < img.blocked_end && address < img.blocked_end &&
        address + size > img.blocked_begin)
      return false;
    std::memcpy(out, img.bytes->data() + offset, size);
    return true;
  };
  const ubc::ReadOnlyMemory mem{&image, read};
  check(ui::verify_open(mem) == nullptr, "all pre-open functions match the real Gothic2.exe");
  check(ui::verify_backdrop(mem) == nullptr, "all backdrop functions match the real Gothic2.exe");
  check(ui::verify_strings(mem) == nullptr, "both zSTRING functions match the real Gothic2.exe");
  check(ui::verify_language(mem) == nullptr, "all language functions match the real Gothic2.exe");
  check(ui::verify_menu_parser(mem) == nullptr, "zCMenu::GetParser matches the real Gothic2.exe");

  // Apply Union 1.0m's six documented Blit patch bytes and verify acceptance + rejection.
  const std::array<std::pair<std::uint32_t, std::uint8_t>, 6> union_patches{{
      {0x007a6457, 0x00},
      {0x007a646a, 0x00},
      {0x007a64d6, 0x41},
      {0x007a6512, 0x42},
      {0x007a668e, 0x90},
      {0x007a668f, 0x90},
  }};
  std::array<std::uint8_t, 6> saved{};
  for (std::size_t i = 0; i < union_patches.size(); ++i) {
    const auto off = static_cast<std::size_t>(union_patches[i].first - m::image_base);
    saved[i] = exe[off];
    exe[off] = union_patches[i].second;
  }
  check(ui::verify_backdrop(mem) == nullptr,
        "Union 1.0m patched zCView::Blit passes backdrop verification");
  exe[0x007a63c0 - m::image_base] ^= 1u;
  check(ui::verify_backdrop(mem) &&
            !std::strcmp(ui::verify_backdrop(mem), "ui_backdrop_blit_mismatch"),
        "unexpected mutation on Union-patched Blit is rejected");
  exe[0x007a63c0 - m::image_base] ^= 1u;
  for (std::size_t i = 0; i < union_patches.size(); ++i)
    exe[static_cast<std::size_t>(union_patches[i].first - m::image_base)] = saved[i];

  const auto mutate_table = [&](const ui::CodeSite* sites, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
      const auto& site = sites[i];
      const auto base = static_cast<std::size_t>(site.address - m::image_base);
      for (const std::size_t delta : {std::size_t{0}, site.length / 2, site.length - 1}) {
        exe[base + delta] ^= 1u;
        const char* actual = ui::verify_code(mem, sites, count);
        check(actual && !std::strcmp(actual, site.mismatch),
              "1-bit mutation in real Gothic2.exe reports that exact function mismatch");
        exe[base + delta] ^= 1u;
      }
      image.blocked_begin = static_cast<std::uint32_t>(site.address);
      image.blocked_end = static_cast<std::uint32_t>(site.address + 1);
      const char* unreadable = ui::verify_code(mem, sites, count);
      check(unreadable && !std::strcmp(unreadable, site.mismatch),
            "unreadable entry byte reports that exact function mismatch");
      image.blocked_begin = image.blocked_end = 0;
    }
    check(ui::verify_code(mem, sites, count) == nullptr,
          "restored Gothic2.exe passes verification again");
  };
  mutate_table(ui::open_sites.data(), ui::open_sites.size());
  mutate_table(ui::backdrop_sites.data(), ui::backdrop_sites.size());
  mutate_table(ui::string_sites.data(), ui::string_sites.size());
  mutate_table(ui::language_sites.data(), ui::language_sites.size());
  mutate_table(ui::menu_parser_sites.data(), ui::menu_parser_sites.size());
  std::printf("native_crafting_ui local exe code guards: %u checks, %u failures\n", checks,
              failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}

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

// The engine contract catalogue keeps its original ASCII spellings.
std::string catalogue_spelling(std::string_view bytes) {
  std::string name;
  for (char byte : bytes) {
    const auto ch = static_cast<unsigned char>(byte);
    switch (ch) {
    case 0xe4:
      name += "ae";
      break;
    case 0xf6:
      name += "oe";
      break;
    case 0xfc:
      name += "ue";
      break;
    case 0xdf:
      name += "ss";
      break;
    default:
      name += static_cast<char>(ch);
    }
  }
  return name;
}

void test_translations() {
  namespace t = ubc::ui_text;
  static_assert(static_cast<unsigned char>(t::game_byte<1252>(U'ä')) == 0xe4);
  static_assert(static_cast<unsigned char>(t::game_byte<1252>(U'ß')) == 0xdf);
  static_assert(static_cast<unsigned char>(t::game_byte<1252>(U'€')) == 0x80);
  static_assert(static_cast<unsigned char>(t::game_byte<1250>(U'Ł')) == 0xa3);
  static_assert(static_cast<unsigned char>(t::game_byte<1251>(U'Я')) == 0xdf);
  static_assert(std::string_view(t::de::table[L::batches]) == "Durchg\xe4nge");
  static_assert(std::string_view(t::de::table[L::batches]).size() == 10);
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
    check(catalogue_spelling(de.recipe(recipe.id)) == recipe.name &&
              catalogue_spelling(de[t::unit(recipe.quantity_unit)]) == recipe.quantity_unit,
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

void test_equipped_ingredient_layout() {
  auto snapshot = example(Kind::bow);
  auto& oak = state(snapshot, 0x800b);
  oak.equipped_ingredient = true;
  oak.material_verified = false;
  oak.maximum = 0;
  ubc::CraftingSession session;
  ubc::CraftingMenu menu;
  check(session.open(snapshot, oak.id) && !session.can_confirm(),
        "the equipped recipe stays locked");
  for (const auto* text : {&ubc::ui_text::de::table, &ubc::ui_text::en::table}) {
    const bool german = text == &ubc::ui_text::de::table;
    const std::string_view hint = german ? "[ausger\xfcstet]" : "[equipped]";
    const std::string_view detail = german ? "Lege die ausger\xfcstete Zutat zuerst ab."
                                           : "Unequip the ingredient before crafting.";
    const auto action = std::string((*text)[L::craft]) + " " + std::string(hint);
    for (const auto size :
         {std::array{640u, 480u}, std::array{1600u, 900u}, std::array{1920u, 1080u}}) {
      ui::Frame frame;
      check(ui::compose(frame, session, menu, true, size[0], size[1], *text),
            "equipped recipe layout");
      bool list = false, instruction = false, craft = false;
      for (std::size_t i = 0; i < frame.count; ++i) {
        const std::string_view line = frame.lines[i].text.data();
        list |= line.substr(0, hint.size()) == hint;
        instruction |= line == detail;
        craft |= line == action;
      }
      check(list && instruction && craft,
            "list, detail and craft action fit with the exact game bytes");
      bounded(frame);
    }
  }
}

// Only the second list entry changes. Keep the range footer and all detail rows byte-identical.
void test_list_boundaries() {
  auto snapshot = example(Kind::alchemy);
  for (const auto& d : c::recipes)
    if (d.station == Kind::alchemy && d.id != 1 && d.id != 0x4001) {
      state(snapshot, d.id).known = false;
      state(snapshot, d.id).maximum = 0;
    }
  state(snapshot, 0x4001).contract = false;
  ubc::CraftingSession session;
  ubc::CraftingMenu menu;
  check(session.open(snapshot), "two alchemy recipes open for layout comparison");
  for (const auto* text : {&ubc::ui_text::de::table, &ubc::ui_text::en::table}) {
    for (const unsigned width : {640u, 1920u}) {
      auto labels = *text;
      ui::Frame compact, wrapped;
      labels.labels[static_cast<unsigned>(L::hint_contract_unverified)] = "[x]";
      check(ui::compose(compact, session, menu, true, width, 1080, labels), "short hint fits");
      labels.labels[static_cast<unsigned>(L::hint_contract_unverified)] =
          "[a longer translated hint for this recipe]";
      check(ui::compose(wrapped, session, menu, true, width, 1080, labels) &&
                wrapped.count > compact.count,
            "long hint wraps the unselected entry");
      const int footer = width == 640 ? 2200 : 7400;
      std::size_t i = 0, j = 0;
      while (i < compact.count && compact.lines[i].y != footer)
        ++i;
      while (j < wrapped.count && wrapped.lines[j].y != footer)
        ++j;
      check(i < compact.count && j < wrapped.count && compact.count - i == wrapped.count - j,
            "same footer and number of detail rows");
      for (; i < compact.count && j < wrapped.count; ++i, ++j) {
        const auto& a = compact.lines[i];
        const auto& b = wrapped.lines[j];
        check(a.x == b.x && a.y == b.y && a.character_limit == b.character_limit &&
                  a.text == b.text,
              "wrapping preserves every footer and detail row");
      }
    }
  }
}

void test_focus_and_controls() {
  using Row = ubc::CraftingMenuRow;
  for (const auto size :
       {std::array{640u, 480u}, std::array{1600u, 900u}, std::array{1920u, 1080u}}) {
    for (const auto* text : {&ubc::ui_text::de::table, &ubc::ui_text::en::table}) {
      ubc::CraftingSession session;
      ubc::CraftingMenu menu;
      ui::Frame frame;
      auto snapshot = example(Kind::alchemy, 12);
      auto& recipe = state(snapshot, 1);
      for (auto& material : recipe.materials)
        if (material.unit_output)
          material.stored = 64;
      check(session.open(snapshot) &&
                ui::compose(frame, session, menu, true, size[0], size[1], *text) &&
                contains(frame, (*text)[L::release_buttons]),
            "opening keeps the release-keys hint");
      std::uint64_t tick = 0;
      neutral(session, tick);
      const auto label = [&](Row row) {
        switch (row) {
        case Row::recipe:
          return text->recipe(1);
        case Row::quantity:
          return (text == &ubc::ui_text::de::table) ? "Menge:" : "Amount:";
        case Row::presets: {
          const char* buttons[] = {"[1]", "[5]", "[10]", "[Max]"};
          return buttons[menu.column()];
        }
        case Row::steps:
          return menu.column() ? "[+5]" : "[-5]";
        case Row::manufacture:
          return (*text)[L::craft];
        case Row::original:
          return (*text)[L::original_controls];
        }
        return "";
      };
      const L actions[] = {L::key_choose, L::key_continue, L::key_preset,
                           L::key_step,   L::key_confirm,  L::key_original};
      std::vector<std::string> initial_labels;
      for (int pos = 0; pos < 10; ++pos) {
        check(ui::compose(frame, session, menu, true, size[0], size[1], *text),
              "every focus position composes");
        const auto row = menu.row();
        unsigned arrows = 0;
        std::vector<std::string> labels;
        for (std::size_t i = 0; i + 1 < frame.count; ++i) {
          const auto& line = frame.lines[i];
          const std::string bytes(line.text.data());
          if (bytes == ">") {
            ++arrows;
            const auto& next = frame.lines[i + 1];
            check(next.y == line.y && next.x - line.x == 220 &&
                      std::string_view(next.text.data()).find(label(row)) == 0,
                  "the single focus marker identifies the active control at a fixed distance");
          } else if (bytes != "[x]") {
            labels.push_back(std::to_string(line.x) + "/" + std::to_string(line.y) + " " + bytes);
          }
        }
        if (!pos)
          initial_labels = labels;
        check(arrows == 1 && contains(frame, "[x]") == (row != Row::recipe) &&
                  labels == initial_labels,
              "moving focus preserves every label coordinate and marks the selected recipe");
        check(contains(frame, (*text)[actions[static_cast<unsigned>(row)]]) &&
                  contains(frame, "B/Esc") &&
                  (size[0] == 640 || row != Row::quantity ||
                   contains(frame, (*text)[L::adjust_amount])),
              "the footer identifies confirm and back for the focused control");
        check(contains(frame, "64 + 1 = 65"), "the output equation includes starting stock");
        check(menu.on_action(ubc::CraftingMenuAction::next, session),
              "Tab advances to the next control");
      }
      recipe.material_known = recipe.material_verified = recipe.hand_known = false;
      recipe.maximum = 0;
      for (auto& material : recipe.materials) {
        if (material.consumed_reserve)
          material.consumed_reserve = -1;
        else
          material.stored = -1;
      }
      session.refresh(snapshot);
      const bool de = text == &ubc::ui_text::de::table;
      check(ui::compose(frame, session, menu, true, size[0], size[1], *text) &&
                contains(frame, de ? "Vorrat unklar. Du erh\xe4ltst 1."
                                   : "Stock is unknown. You get 1.") &&
                contains(frame, de ? "Laborwasserflasche (1 ben\xf6tigt)"
                                   : "Lab water flask (1 needed)") &&
                contains(frame, de ? "Unklar, wie viele du in der Hand hast."
                                   : "The number in your hand is unknown.") &&
                contains(frame,
                         de ? "Vorrat unklar. Du brauchst 1." : "Stock is unknown. You need 1.") &&
                contains(frame, de ? "Menge: 0. Vorschau f\xfcr einen Durchgang."
                                   : "Amount: 0. Preview for one batch."),
            "unknown stock and held amounts retain the one-batch cost and yield");
      bounded(frame);
    }
  }
}

void test_tool_return() {
  for (const bool german : {false, true}) {
    const auto& text = german ? ubc::ui_text::de::table : ubc::ui_text::en::table;
    auto snapshot = example(Kind::saw);
    auto& recipe = state(snapshot, 0x2001);
    const auto saw = std::find_if(recipe.materials.begin(), recipe.materials.end(),
                                  [](const auto& row) { return row.symbol == 16609; });
    check(saw != recipe.materials.end(), "the arrow recipe includes its saw");
    if (saw == recipe.materials.end())
      continue;
    saw->stored = 17;
    ui::Frame frame;
    ubc::CraftingSession session;
    ubc::CraftingMenu menu;
    const char* returned =
        german ? "S\xe4ge (beim Verlassen zur\374ck)" : "Saw (returned when you leave)";
    const auto draw = [&] { return ui::compose(frame, session, menu, true, 640, 480, text); };
    check(session.open(snapshot, recipe.id) && draw() && contains(frame, returned) &&
              contains(frame, "17 + 1 = 18") && contains(frame, german ? "Lose" : "lots") &&
              contains(frame, german ? "Pfeil - Ausgabe" : "Arrow - output"),
          "the held saw returns to inventory once and stays separate from arrow output");
    bounded(frame);
    recipe.hand_known = recipe.material_verified = false;
    recipe.maximum = 0;
    saw->tool_reserve = -1;
    session.refresh(snapshot);
    check(draw() && !contains(frame, returned) &&
              contains(frame, german ? "17 im Inventar. R\374ckgabe offen."
                                     : "17 in pack. Return is uncertain.") &&
              !contains(frame, "17 +"),
          "an unknown return keeps the known stock without promising a tool or a total");
    bounded(frame);
    recipe.material_known = false;
    saw->stored = -1;
    session.refresh(snapshot);
    check(draw() &&
              contains(frame, german ? "Unklar, wie viele im Inventar sind."
                                     : "Your inventory count is unknown.") &&
              !contains(frame, returned) && !contains(frame, "-1"),
          "unknown tool stock has its own explanation without a fabricated count");
    bounded(frame);
  }
}

int main(int argc, [[maybe_unused]] char** argv) {
  if (argc != 1) {
    if (argc == 3 && std::string_view(argv[1]) == "--local-exe")
      return test_local_exe_code_guards(argv[2]);
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
  test_menu_view();
  test_screen_init_digests();
  test_menu_code_guards();
  test_translations();
  test_equipped_ingredient_layout();
  test_list_boundaries();
  test_focus_and_controls();
  test_tool_return();
  ui::Frame frame;
  std::set<std::int32_t> unknown_names;
  for (const auto& d : c::recipes) {
    for (const auto* language : {&ubc::ui_text::en::table, &ubc::ui_text::de::table}) {
      const auto& text = *language;
      for (const auto dimensions : std::array<std::array<unsigned, 2>, 6>{{{{1280, 720}},
                                                                           {{1600, 899}},
                                                                           {{1600, 900}},
                                                                           {{1920, 1080}},
                                                                           {{720, 720}},
                                                                           {{640, 480}}}}) {
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
        if (d.grind)
          check(contains(frame, language == &ubc::ui_text::de::table ? "Schleifkunst 98 auf 99"
                                                                     : "Honing skill 98 to 99"),
                "grinding labels the current and projected skill");
        if (d.bow)
          check(contains(frame, language == &ubc::ui_text::de::table ? "Menge: 1, Teile f\xfcr 7"
                                                                     : "Amount: 1, parts for 7"),
                "bow upgrades label the selected quantity and the material limit");
        if (d.id == 0xb002 && language == &ubc::ui_text::de::table) {
          char expected[64];
          std::snprintf(expected, sizeof(expected), "Menge: %u / %u Durchg\xe4nge",
                        session.quantity(), session.maximum());
          check(contains(frame, expected), "swamp tobacco quantity shows CP1252 batch units");
        }
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
        check(ui::compose(frame, session, menu, false, dimensions[0], dimensions[1], text) &&
                  contains(frame, text[L::hint_contract_unverified]) &&
                  contains(frame, text[L::test_amount]),
              "known blocked entry uses readable marker and explicit UI-only action");
        // Stress every name with the longest encoded hint in this language.
        auto worst = text;
        const char* longest = text[L::hint_contract_unverified];
        for (auto label = static_cast<unsigned>(L::hint_no_known_recipes);
             label <= static_cast<unsigned>(L::hint_quantity_unsupported); ++label) {
          const auto* candidate = text[static_cast<L>(label)];
          if (std::strlen(candidate) > std::strlen(longest))
            longest = candidate;
        }
        worst.labels[static_cast<unsigned>(L::hint_contract_unverified)] = longest;
        check(ui::compose(frame, session, menu, false, dimensions[0], dimensions[1], worst),
              "longest hint composes with every recipe");
        bounded(frame);
        const bool wide = dimensions[0] >= 1600 && dimensions[1] >= 900;
        const int footer = wide ? 7400 : 2200;
        std::size_t index = 2;
        int previous_y = 0;
        std::string reconstructed;
        bool selected = false;
        for (; index < frame.count && frame.lines[index].y < footer; ++index) {
          const auto& line = frame.lines[index];
          const std::string_view bytes(line.text.data());
          if (bytes == ">") {
            check(line.x == 300 && index + 1 < frame.count && frame.lines[index + 1].y == line.y,
                  "focus has its own gutter on the selected recipe baseline");
            selected = true;
            continue;
          }
          const int gap = line.y - previous_y;
          check(line.x == 520 && (!previous_y || gap == 230 || gap == (wide ? 400 : 300)),
                "recipe and continuation text share a fixed origin and keep their spacing");
          if (selected && (gap == 230 || reconstructed.empty())) {
            if (!reconstructed.empty())
              reconstructed += ' ';
            reconstructed += bytes;
          } else {
            selected = false;
          }
          previous_y = line.y;
          if (wide) {
            // Conservative two-times old-font advances including the space between glyphs.
            // Measure the submitted text independently of the composer's character budget.
            int pixels = 0;
            for (char ch : bytes)
              pixels += std::strchr("ilfjrt ", ch) ? 20 : std::strchr("mwMW", ch) ? 30 : 28;
            const int width = static_cast<int>(dimensions[0]);
            check(line.x * width / 8192 + pixels < 4640 * width / 8192,
                  "recipe text ends before the detail focus gutter in screen pixels");
          }
        }
        check(reconstructed == std::string(longest) + " " + text.recipe(d.id),
              "the selected recipe and its full block reason survive wrapping and scrolling");
        check(index + 1 < frame.count && frame.lines[index].y == footer &&
                  frame.lines[index + 1].x == (wide ? 4860 : 520),
              "the list ends before the footer and the details start at their own origin");
        check(contains(frame, (std::string(text[L::test_amount]) + " " + longest).c_str()),
              "confirmation includes the specific reason");
        auto& unreadable = state(snapshot, d.id);
        unreadable.contract = true;
        unreadable.material_known = unreadable.material_verified = unreadable.hand_known = false;
        unreadable.maximum = 0;
        for (auto& material : unreadable.materials) {
          material.stored = -1;
          if (material.consumed_reserve)
            material.consumed_reserve = -1;
          if (material.tool_reserve)
            material.tool_reserve = -1;
        }
        session.refresh(snapshot);
        check(ui::compose(frame, session, menu, true, dimensions[0], dimensions[1], text),
              "the detail column also fits recipes with unknown inventory and held counts");
        bounded(frame);
        if (d.tobacco) {
          const bool de = language == &ubc::ui_text::de::table;
          check(contains(frame, de ? "Vorrat und Menge in der Hand unbekannt."
                                   : "Stock and amount in hand are unknown.") &&
                    !contains(frame, de ? "Du brauchst 0." : "You need 0."),
                "an unreadable flask and a possession-only ingredient retain their requirements");
        }
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
              contains(frame, "1-9 / 9"),
          "wide viewport fits all nine recipes without losing the selection");
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
              contains(frame, "Vorschau f\xfcr einen Durchgang.") && contains(frame, "Bedarf"),
          "known zero maximum labels one-cycle missing-material preview explicitly");
    for (auto& recipe : snapshot.recipes)
      recipe.known = false;
    ubc::CraftingSession empty;
    check(empty.open(snapshot) &&
              ui::compose(frame, empty, menu, true, 640, 480, ubc::ui_text::de::table) &&
              contains(frame, "Menge: 0") && !contains(frame, "Durchgang"),
          "an empty recipe list has no one-batch preview");
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
            contains(frame, "Menge: 2, Teile f\xfcr 7") && contains(frame, "Minimum 24") &&
            contains(frame, " - 20 = ") && contains(frame, " - 24 = "),
        "Bow UI separates stock bound, original steel minimum and actual steel/leather consumption");
    check(session.preset(7) &&
              ui::compose(frame, session, menu, true, 1280, 720, ubc::ui_text::de::table) &&
              contains(frame, "[gesperrt]"),
          "new Bow amount visibly remains blocked until its own current item-path proof");
  }
  for (const auto width : {640u, 720u, 1280u, 1920u})
    check(ui::compose_failure(frame, width, 720, ubc::ui_text::de::table) &&
              contains(frame, "nicht best\xe4tigt"),
          "failure notice remains separate from confirmed result at all supported widths");
  std::printf("native_crafting_ui portable layout: %u checks, %u failures\n", checks, failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
