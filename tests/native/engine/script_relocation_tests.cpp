// SPDX-License-Identifier: MIT
#include "../helpers/parser_image.hpp"
#include "ubc/engine/script_relocation.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace {
namespace sr = ubc::script_relocation;
namespace t = ubc::test;
using sr::Issue;
unsigned checks = 0;
unsigned failures = 0;

void require(bool condition, const char* name) {
  ++checks;
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}

enum Recipe : std::uint32_t { apple = 11, potion = 12, flask = 13 };

struct Options {
  std::int32_t potion_amount = 1;
  std::int32_t constant = 3;
  std::string potion_text = "Trank fertig";
  bool flask_jump_out = false;
  bool flask_jump_inside = false;
  bool apple_unknown_callee = false;
  bool duplicate_flask = false;
  bool rename_learned = false;
  std::uint32_t flask_argc = 0;
  std::uint32_t value_offset = 0x10;
  std::string apple_parent = "C_ITEM";
};

// Synthetic recipe script. Each recipe owns a station function, an item
// constructor and shared helpers; every body references symbols or entries,
// so a shifted layout changes all raw digests.
t::Script script(const Options& o = {}) {
  t::Script s;
  s.data("C_ITEM", 4, 0, 2, {}, 0, 0x120);
  s.data("C_ITEM.VALUE", 2, 4, 1, "C_ITEM", 0, o.value_offset);
  s.data("C_OTHER", 4, 0, 1, {}, 0, 0x40);
  s.data("CREATEINVITEMS", 5, 8 | 1, 3);
  s.data("PRINT", 5, 8 | 1, 1);
  s.data("UBC_AMOUNT", 2, 1, 1, {}, o.constant);
  s.data(o.rename_learned ? "UBC_LEARNED_X" : "UBC_LEARNED", 2, 0, 5);
  const auto item = [](std::int32_t value) {
    t::Assembler a;
    a.push_int(value).push_var("C_ITEM.VALUE").op(9).ret();
    return a;
  };
  s.function("ITFO_APPLE", item(5), 7, 1, 0, o.apple_parent);
  s.function("ITPO_HEALTH", item(25), 7, 1, 0, "C_ITEM");
  s.function("ITMI_FLASK", item(10), 7, 1, 0, "C_ITEM");
  {
    t::Assembler a;
    a.push_text("Fertig").call_external("PRINT").ret();
    s.function("B_SAY", a);
  }
  {
    t::Assembler a;
    a.push_int(1).push_var("UBC_AMOUNT").op(16).ret();
    s.function("RECIPE_HELPER", a, 5, 1, 0, {}, 2);
  }
  {
    t::Assembler a;
    a.push_inst("ITFO_APPLE").push_symbol_int("ITFO_APPLE").push_int(2);
    a.call_external("CREATEINVITEMS").push_var("UBC_AMOUNT").jumpf("end");
    if (o.apple_unknown_callee)
      a.call_offset("B_SAY", 1);
    else
      a.call("B_SAY");
    a.label("end").ret();
    s.function("RECIPE_APPLE", a);
  }
  {
    t::Assembler a;
    // 1000 is a literal that happens to be a plausible index; it stays literal.
    a.push_symbol_int("ITPO_HEALTH").push_int(o.potion_amount).push_int(1000).op(0);
    a.call_external("CREATEINVITEMS").push_text(o.potion_text).call_external("PRINT");
    a.call("B_SAY").ret();
    s.function("RECIPE_POTION", a);
  }
  {
    t::Assembler a;
    a.push_symbol_int("ITMI_FLASK").push_int(4).call_external("CREATEINVITEMS");
    a.push_array(o.rename_learned ? "UBC_LEARNED_X" : "UBC_LEARNED", 2);
    if (o.flask_jump_out)
      a.jump_relative(76, 64);
    else if (o.flask_jump_inside)
      a.jump_relative(76, 1);
    else
      a.jumpf("end");
    a.call("RECIPE_HELPER").label("end").ret();
    s.function("RECIPE_FLASK", a, 5, 1, o.flask_argc);
  }
  if (o.duplicate_flask)
    s.data("ITMI_FLASK", 2, 0, 1);
  return s;
}

const std::vector<t::Recipe> recipes{
    {apple, {"RECIPE_APPLE", "B_SAY", "ITFO_APPLE"}, {"UBC_AMOUNT", "CREATEINVITEMS", "C_ITEM"}},
    {potion, {"RECIPE_POTION", "B_SAY", "ITPO_HEALTH"}, {"CREATEINVITEMS", "PRINT"}},
    {flask, {"RECIPE_FLASK", "RECIPE_HELPER", "ITMI_FLASK"}, {"UBC_LEARNED", "C_ITEM.VALUE"}}};

const t::Layout shifted{7, 1, 37, 0x30000, 0x200};

struct Run {
  sr::Storage<32, 16, 16, 8> storage;
  sr::Relocation result;
  const sr::RecipeState& recipe(std::uint32_t id) const {
    for (std::size_t i = 0; i < result.total; ++i)
      if (storage.recipes[i].recipe == id)
        return storage.recipes[i];
    return storage.recipes[0];
  }
  bool only_inactive(std::uint32_t id, Issue reason) const {
    if (!result.valid || result.active + 1 != result.total)
      return false;
    for (std::size_t i = 0; i < result.total; ++i) {
      const auto& state = storage.recipes[i];
      if (state.active != (state.recipe != id))
        return false;
    }
    return recipe(id).reason == reason;
  }
};

Run relocate(t::Built& live, const sr::ReferenceProfile& profile) {
  Run run;
  run.result = sr::relocate(live.memory(), t::Built::parser, profile, run.storage.workspace());
  return run;
}

struct World {
  t::Script reference_script = script();
  t::Built reference_build = t::build(reference_script);
  t::Reference reference{reference_build, reference_script, recipes};
};

void test_identity() {
  World w;
  const auto profile = w.reference.profile();
  auto run = relocate(w.reference_build, profile);
  require(run.result.valid && run.result.issue == Issue::none, "identity relocates");
  require(run.result.identity && run.result.active == 3 && run.result.total == 3,
          "identity activates every recipe");
  require(run.result.names_found == profile.name_count && run.result.names_missing == 0 &&
              run.result.names_duplicate == 0,
          "identity resolves every name once");
  bool same = true;
  for (std::size_t i = 0; i < profile.name_count; ++i)
    same = same && run.storage.names[i].live_index == profile.names[i].reference_index;
  for (std::size_t i = 0; i < profile.function_count; ++i) {
    const auto& match = run.storage.functions[i];
    same = same && match.issue == Issue::none &&
           match.live_start == profile.functions[i].reference_start &&
           match.live_raw_sha256 == profile.functions[i].reference_raw_sha256 &&
           match.live_index == profile.names[profile.functions[i].name].reference_index;
  }
  require(same, "identity keeps indices, starts and raw digests");
  std::uint32_t live = 0;
  const auto flask_index = w.reference_build.index.at("ITMI_FLASK");
  require(sr::live_symbol(profile, run.storage.workspace(), flask_index, live) &&
              live == flask_index,
          "identity translates reference indices to themselves");
}

void test_shifted() {
  World w;
  const auto profile = w.reference.profile();
  auto live = t::build(script(), shifted);
  auto run = relocate(live, profile);
  require(run.result.valid && !run.result.identity && run.result.active == 3,
          "shifted symbols and code keep every recipe active");
  require(run.result.code_base != w.reference_build.code &&
              run.result.symbol_table != w.reference_build.table &&
              run.result.symbol_count == w.reference_build.count + 7,
          "shifted image has another layout");
  bool moved = true;
  for (std::size_t i = 0; i < profile.function_count; ++i) {
    const auto& record = profile.functions[i];
    const auto& match = run.storage.functions[i];
    const std::string name = profile.names[record.name].text;
    moved = moved && match.issue == Issue::none && match.live_start == live.start.at(name) &&
            match.live_start != record.reference_start &&
            match.live_raw_sha256 != record.reference_raw_sha256 &&
            match.live_index == live.index.at(name);
  }
  require(moved, "shifted functions bind live index, start and another raw digest");
  std::uint32_t translated = 0;
  require(sr::live_symbol(profile, run.storage.workspace(),
                          w.reference_build.index.at("ITMI_FLASK"), translated) &&
              translated == live.index.at("ITMI_FLASK"),
          "reference index translates to the shifted index");
  run = relocate(live, profile);
  require(run.result.valid && run.result.active == 3, "relocation can run again");
}

void test_recipe_changes() {
  World w;
  const auto profile = w.reference.profile();
  {
    Options o;
    o.potion_amount = 2;
    auto live = t::build(script(o), shifted);
    const auto run = relocate(live, profile);
    require(run.only_inactive(potion, Issue::fingerprint_mismatch),
            "changed amount rejects exactly that recipe");
    require(run.recipe(potion).kind == sr::RecordKind::function &&
                run.recipe(potion).record == w.reference.function_id("RECIPE_POTION"),
            "rejection names the changed function");
  }
  {
    Options o;
    o.constant = 4;
    auto live = t::build(script(o));
    const auto run = relocate(live, profile);
    require(run.only_inactive(apple, Issue::symbol_value),
            "changed constant value rejects its recipe");
    require(run.recipe(apple).kind == sr::RecordKind::data, "constant rejection is a data record");
  }
  {
    Options o;
    o.potion_text = "Potion ready";
    auto live = t::build(script(o), shifted);
    require(relocate(live, profile).only_inactive(potion, Issue::fingerprint_mismatch),
            "changed text rejects its recipe by default");
    t::Script reference_script = script();
    auto reference_build = t::build(reference_script);
    t::Reference tolerant{reference_build, reference_script, recipes, {"RECIPE_POTION"}};
    const auto run = relocate(live, tolerant.profile());
    require(run.result.valid && run.result.active == 3,
            "text-ignoring record accepts another text");
  }
  {
    Options o;
    o.flask_argc = 1;
    auto live = t::build(script(o));
    require(relocate(live, profile).only_inactive(flask, Issue::symbol_signature),
            "changed signature rejects its recipe");
  }
  {
    Options o;
    o.apple_parent = "C_OTHER";
    auto live = t::build(script(o));
    require(relocate(live, profile).only_inactive(apple, Issue::symbol_parent),
            "changed parent rejects its recipe");
  }
  {
    Options o;
    o.value_offset = 0x14;
    auto live = t::build(script(o), shifted);
    const auto run = relocate(live, profile);
    require(run.only_inactive(flask, Issue::symbol_signature) &&
                run.recipe(flask).kind == sr::RecordKind::data,
            "changed member offset rejects its recipe");
  }
}

void test_invalid_code() {
  World w;
  const auto profile = w.reference.profile();
  const auto flask_record = w.reference.function_id("RECIPE_FLASK");
  {
    Options o;
    o.flask_jump_out = true;
    auto live = t::build(script(o), shifted);
    const auto run = relocate(live, profile);
    require(run.only_inactive(flask, Issue::jump_target), "jump out of a function is rejected");
    require(run.storage.functions[flask_record].failed_offset == 21,
            "jump rejection names the instruction");
  }
  {
    Options o;
    o.flask_jump_inside = true;
    auto live = t::build(script(o), shifted);
    const auto run = relocate(live, profile);
    require(run.only_inactive(flask, Issue::jump_target), "jump into an instruction is rejected");
    require(run.storage.functions[flask_record].failed_offset == 1,
            "mid-instruction rejection names the target");
  }
  {
    Options o;
    o.apple_unknown_callee = true;
    auto live = t::build(script(o), shifted);
    require(relocate(live, profile).only_inactive(apple, Issue::unknown_callee),
            "call to an unknown entry is rejected");
  }
  {
    auto live = t::build(script(), shifted);
    const auto helper = live.symbol_address(live.index.at("RECIPE_HELPER"));
    live.image.put(helper + 0x18, live.code_length - 1u);
    const auto run = relocate(live, profile);
    require(run.result.valid && !run.recipe(flask).active &&
                run.storage.functions[w.reference.function_id("RECIPE_HELPER")].issue ==
                    Issue::function_range,
            "function past the code end is rejected");
  }
}

void test_names() {
  World w;
  const auto profile = w.reference.profile();
  {
    Options o;
    o.duplicate_flask = true;
    auto live = t::build(script(o), shifted);
    const auto run = relocate(live, profile);
    require(run.only_inactive(flask, Issue::duplicate_name),
            "duplicate name rejects only its recipe");
    require(run.result.names_duplicate == 1, "duplicate name is counted");
    const auto second = live.names.rbegin()->first;
    const auto name = live.image.get(live.symbol_address(second) + 8);
    require(live.names.at(second) == "ITMI_FLASK", "second flask symbol is the last name");
    live.image.bytes[name + 10] = 'X';
    require(relocate(live, profile).only_inactive(flask, Issue::duplicate_name),
            "unterminated duplicate name still rejects its recipe");
  }
  {
    Options o;
    o.rename_learned = true;
    auto live = t::build(script(o), shifted);
    const auto run = relocate(live, profile);
    require(run.only_inactive(flask, Issue::unknown_symbol),
            "missing name rejects only its recipe");
    require(run.result.names_missing == 1, "missing name is counted");
  }
  {
    auto live = t::build(script(), shifted);
    const auto text = live.texts.rbegin()->first; // the potion text is the last one
    live.image.bytes.at(live.image.get(live.symbol_address(text) + 8)) = 'X';
    require(relocate(live, profile).only_inactive(potion, Issue::unknown_symbol),
            "named string constant is no anonymous text");
  }
}

// Class members, externals and variables of type 5..7 carry data 0 like the
// function at code offset 0; only constant code symbols own an entry.
t::Script entry_script(bool alias) {
  t::Script s;
  s.data("C_INFO", 4, 0, 2, {}, 0, 0x20);
  s.data("C_NPC", 4, 0, 1, {}, 0, 0x40);
  s.data("C_INFO.INFORMATION", 5, 4, 1, "C_INFO", 0, 0x10); // var func member
  s.data("C_INFO.CONDITION", 5, 1 | 4, 1, "C_INFO", 0, 0x14);
  s.data("SELF", 7, 0, 0, "C_NPC");
  s.data("PRINT", 5, 8 | 1, 1);
  if (alias)
    s.data("B_ALIAS", 5, 1, 0); // a second constant entry at offset 0
  {
    t::Assembler a;
    a.push_text("Hilfe").call_external("PRINT").ret();
    s.function("B_HELPER", a);
  }
  {
    t::Assembler a;
    a.push_var("C_INFO.INFORMATION").push_var("SELF").call("B_HELPER").ret();
    s.function("RECIPE_X", a);
  }
  // Compiled prototypes carry no constant flag; an instance calls its
  // prototype first, so the prototype must still own its entry.
  {
    t::Assembler a;
    a.ret();
    s.function("PROTO_X", a, 6, 0, 0, "C_INFO");
  }
  {
    t::Assembler a;
    a.call("PROTO_X").ret();
    s.function("INST_X", a, 7, 1, 0, "PROTO_X");
  }
  return s;
}

void test_code_entries() {
  const std::vector<t::Recipe> entry_recipes{{apple,
                                              {"RECIPE_X", "B_HELPER", "INST_X"},
                                              {"C_INFO.INFORMATION", "C_INFO.CONDITION", "SELF"}}};
  const auto reference_script = entry_script(false);
  auto reference_build = t::build(reference_script);
  t::Reference reference{reference_build, reference_script, entry_recipes};
  const auto profile = reference.profile();
  require(reference_build.start.at("B_HELPER") == 0, "helper owns code offset 0");
  auto run = relocate(reference_build, profile);
  require(run.result.valid && run.result.identity && run.result.active == 1,
          "members, variables and externals leave the entry at offset 0 unique; "
          "the prototype keeps its entry");
  for (const auto& layout : {t::Layout{5, 1, 0, 0x30000, 0x80}, shifted}) {
    auto live = t::build(reference_script, layout);
    run = relocate(live, profile);
    require(run.result.valid && !run.result.identity && run.result.active == 1,
            "moved members and variables keep the helper call");
  }
  const auto alias_script = entry_script(true);
  auto alias_build = t::build(alias_script);
  t::Reference alias{alias_build, alias_script, entry_recipes};
  require(relocate(alias_build, alias.profile()).only_inactive(apple, Issue::unknown_callee),
          "two constant entries at one offset are ambiguous");
}

// The symbol table is read in blocks of 1024 pointers; named symbols past the
// first blocks must keep their own index.
void test_table_blocks() {
  World w;
  const auto profile = w.reference.profile();
  for (const std::uint32_t total : {1024u, 1025u, 2048u, 2049u, 3073u}) {
    auto live = t::build(script(), t::Layout{total - w.reference_build.count, 1, 0, 0x40000, 0x40});
    const auto run = relocate(live, profile);
    bool bound = live.count == total && run.result.valid && run.result.active == 3 &&
                 live.index.at("RECIPE_FLASK") + 1024u > total;
    for (std::size_t i = 0; i < profile.name_count; ++i)
      bound = bound && run.storage.names[i].live_index == live.index.at(profile.names[i].text);
    const auto message = "table of " + std::to_string(total) + " symbols binds every live index";
    require(bound, message.c_str());
  }
}

// Texts are hashed in blocks of 1024 bytes; every block starts at its own
// position and the terminator may fall into a block of its own.
void test_long_texts() {
  for (const std::uint32_t length : {1023u, 1024u, 1025u, 2047u, 2048u, 65536u, 65537u}) {
    Options o;
    o.potion_text.assign(length, 'a');
    for (std::uint32_t i = 0; i < length; ++i)
      o.potion_text[i] = static_cast<char>('a' + (i * 7u) % 26u);
    const auto reference_script = script(o);
    auto reference_build = t::build(reference_script);
    t::Reference reference{reference_build, reference_script, recipes};
    const auto profile = reference.profile();
    auto live = t::build(reference_script, shifted);
    const auto same = relocate(live, profile);
    o.potion_text.back() = o.potion_text.back() == 'q' ? 'r' : 'q';
    auto changed_build = t::build(script(o), shifted);
    const auto changed = relocate(changed_build, profile);
    const bool limit = length > ubc::script_fingerprint::max_text_length;
    const bool ok = limit ? same.only_inactive(potion, Issue::text_unreadable) &&
                                changed.only_inactive(potion, Issue::text_unreadable)
                          : same.result.valid && same.result.active == 3 &&
                                changed.only_inactive(potion, Issue::fingerprint_mismatch);
    const auto message = "text of " + std::to_string(length) + " bytes is hashed in place";
    require(ok, message.c_str());
  }
}

void test_reads_and_layout() {
  World w;
  const auto profile = w.reference.profile();
  {
    auto live = t::build(script(), shifted);
    live.image.deny(live.code_address("RECIPE_POTION") + 3, 2);
    require(relocate(live, profile).only_inactive(potion, Issue::code_unreadable),
            "denied code read rejects only its recipe");
  }
  {
    auto live = t::build(script(), shifted);
    const auto text = live.texts.rbegin()->first; // the potion text is the last one
    live.image.deny(live.image.get(live.image.get(live.symbol_address(text) + 0x18) + 8), 1);
    require(relocate(live, profile).only_inactive(potion, Issue::text_unreadable),
            "denied text read rejects only its recipe");
  }
  {
    auto live = t::build(script(), shifted);
    live.image.deny(live.table + 8u, 4);
    const auto run = relocate(live, profile);
    require(!run.result.valid && run.result.issue == Issue::symbol_unreadable &&
                run.result.active == 0 && !run.storage.recipes[0].active &&
                run.storage.recipes[0].reason == Issue::symbol_unreadable,
            "denied symbol table rejects every recipe");
  }
  {
    auto live = t::build(script(), shifted);
    live.image.deny(live.image.get(live.symbol_address(live.index.at("B_SAY")) + 8), 1);
    const auto run = relocate(live, profile);
    require(!run.result.valid && run.result.issue == Issue::name_unreadable,
            "denied name read rejects every recipe");
  }
  {
    auto live = t::build(script(), shifted);
    live.image.change_on_header = 2;
    const auto run = relocate(live, profile);
    require(!run.result.valid && run.result.issue == Issue::changed_during_read &&
                run.result.active == 0,
            "layout change during the scan rejects every recipe");
  }
  {
    auto live = t::build(script(), shifted);
    Run run;
    run.result =
        sr::relocate(live.memory(), t::Built::parser + 4, profile, run.storage.workspace());
    require(!run.result.valid && run.result.issue == Issue::wrong_parser,
            "foreign parser is rejected");
  }
}

void test_profile_checks() {
  World w;
  auto profile = w.reference.profile();
  {
    sr::Storage<4, 16, 16, 8> small;
    const auto result =
        sr::relocate(w.reference_build.memory(), t::Built::parser, profile, small.workspace());
    require(!result.valid && result.issue == Issue::capacity, "too small storage is rejected");
  }
  {
    auto& names = w.reference.names();
    std::swap(names[0], names[1]);
    const auto run = relocate(w.reference_build, w.reference.profile());
    require(!run.result.valid && run.result.issue == Issue::invalid_profile,
            "unsorted names are rejected");
    std::swap(names[0], names[1]);
  }
  {
    auto& functions = w.reference.functions();
    const auto saved = functions[0].byte_length;
    functions[0].byte_length = 0;
    const auto run = relocate(w.reference_build, w.reference.profile());
    require(!run.result.valid && run.result.issue == Issue::invalid_profile,
            "empty function record is rejected");
    functions[0].byte_length = saved;
  }
  {
    // A group without a function record proves nothing about the script.
    const std::vector<std::vector<t::Recipe>> empty{
        {}, {{apple, {}, {}}}, {{apple, {}, {"UBC_AMOUNT"}}}, {recipes[0], {potion, {}, {}}}};
    for (const auto& groups : empty) {
      const auto reference_script = script();
      auto reference_build = t::build(reference_script);
      t::Reference reference{reference_build, reference_script, groups};
      const auto run = relocate(reference_build, reference.profile());
      require(!run.result.valid && run.result.issue == Issue::invalid_profile &&
                  run.result.active == 0,
              "every profile group needs a function record");
    }
  }
  const auto run = relocate(w.reference_build, w.reference.profile());
  require(run.result.identity, "restored profile is the identity again");
  require(std::string(ubc::script_fingerprint::issue_name(Issue::duplicate_name)) ==
              "duplicate_name",
          "relocation issues have names");
}

// Station shares gate every recipe of a station, the permutation replaces
// the linear lookup and anonymous texts resolve by position.
void test_station_shares_and_lookups() {
  World w;
  const auto base = w.reference.profile();
  const std::uint32_t say = w.reference.function_id("B_SAY");
  const std::uint32_t potion_body = w.reference.function_id("RECIPE_POTION");
  const std::vector<std::uint32_t> share_functions{say};
  const std::vector<sr::StationShare> stations{{4, share_functions.data(), 1, nullptr, 0}};
  std::vector<sr::RecipeGroup> groups(base.groups, base.groups + base.group_count);
  groups[0].station = 0; // apple
  groups[1].station = 0; // potion
  std::vector<std::uint32_t> by_reference(base.name_count);
  for (std::uint32_t i = 0; i < base.name_count; ++i)
    by_reference[i] = i;
  std::sort(by_reference.begin(), by_reference.end(), [&base](std::uint32_t l, std::uint32_t r) {
    return base.names[l].reference_index < base.names[r].reference_index;
  });
  // RECIPE_POTION pushes its text after three PUSH_INT, ADD and CALL_EXTERNAL.
  const std::uint32_t text_offset = 3u * 5u + 1u + 5u;
  const auto text_index = [&](const t::Built& built) {
    for (const auto& text : built.texts)
      if (text.second == "Trank fertig")
        return text.first;
    return 0u;
  };
  const std::vector<sr::AnonymousText> texts{
      {text_index(w.reference_build), potion_body, text_offset}};
  auto profile = base;
  profile.groups = groups.data();
  profile.by_reference = by_reference.data();
  profile.stations = stations.data();
  profile.station_count = stations.size();
  profile.texts = texts.data();
  profile.text_count = texts.size();
  require(sr::detail::valid_profile(profile), "profile with station shares is valid");

  auto run = relocate(w.reference_build, profile);
  require(run.result.identity && run.result.active == 3, "station shares keep the identity");
  std::uint32_t live = 0;
  require(sr::live_text(w.reference_build.memory(), run.result, profile, run.storage.workspace(),
                        texts[0].reference_index, live) &&
              live == texts[0].reference_index,
          "identity text resolves to itself");

  auto moved = t::build(script(), shifted);
  run = relocate(moved, profile);
  require(run.result.valid && run.result.active == 3, "shifted image keeps station recipes");
  require(sr::live_symbol(profile, run.storage.workspace(),
                          w.reference_build.index.at("ITMI_FLASK"), live) &&
              live == moved.index.at("ITMI_FLASK"),
          "permutation translates a reference index");
  require(!sr::live_symbol(profile, run.storage.workspace(), 0xfffffff0u, live),
          "permutation rejects an unknown index");
  require(sr::live_text(moved.memory(), run.result, profile, run.storage.workspace(),
                        texts[0].reference_index, live) &&
              live == text_index(moved) && live != texts[0].reference_index,
          "text resolves by position in the shifted image");
  require(!sr::live_text(moved.memory(), run.result, profile, run.storage.workspace(),
                         texts[0].reference_index + 1u, live),
          "unlisted text is rejected");

  {
    Options o;
    o.apple_unknown_callee = true;
    auto live_build = t::build(script(o), shifted);
    const auto changed = relocate(live_build, profile);
    require(changed.result.valid && changed.result.active == 2 && !changed.recipe(apple).active &&
                changed.recipe(potion).active,
            "recipe share failure stays in its recipe");
    require(!changed.recipe(apple).station, "recipe share failure is not a station failure");
  }
  {
    Options o;
    o.potion_text = "Potion ready";
    auto live_build = t::build(script(o), shifted);
    auto changed = relocate(live_build, profile);
    require(!changed.recipe(potion).active &&
                !sr::live_text(live_build.memory(), changed.result, profile,
                               changed.storage.workspace(), texts[0].reference_index, live),
            "changed text blocks its recipe and its lookup");
  }
  {
    auto changed_share = share_functions;
    changed_share[0] = w.reference.function_id("RECIPE_HELPER");
    const std::vector<sr::StationShare> helper{{4, changed_share.data(), 1, nullptr, 0}};
    auto with_helper = profile;
    with_helper.stations = helper.data();
    Options o;
    o.constant = 4; // RECIPE_HELPER is unchanged; UBC_AMOUNT is apple data.
    auto live_build = t::build(script(o));
    const auto changed = relocate(live_build, with_helper);
    require(changed.only_inactive(apple, Issue::symbol_value), "share is checked with recipes");
  }
  {
    Options o;
    o.flask_jump_inside = true;
    std::vector<std::uint32_t> flask_share{w.reference.function_id("RECIPE_FLASK")};
    const std::vector<sr::StationShare> flask_station{{4, flask_share.data(), 1, nullptr, 0}};
    auto with_flask = profile;
    with_flask.stations = flask_station.data();
    auto live_build = t::build(script(o));
    const auto changed = relocate(live_build, with_flask);
    require(changed.result.valid && changed.result.active == 0 && changed.recipe(apple).station &&
                changed.recipe(potion).station && !changed.recipe(flask).station,
            "failing station share blocks the whole station");
  }
  {
    auto broken = profile;
    std::swap(by_reference[0], by_reference[1]);
    require(!sr::detail::valid_profile(broken), "unsorted permutation is rejected");
    std::swap(by_reference[0], by_reference[1]);
    groups[2].station = 1;
    require(!sr::detail::valid_profile(broken), "unknown station share is rejected");
    groups[2].station = sr::no_name;
    auto far = texts;
    far[0].offset = 0xffffu;
    broken.texts = far.data();
    require(!sr::detail::valid_profile(broken), "text outside its function is rejected");
    far[0].offset = profile.functions[potion_body].byte_length - 4u;
    require(!sr::detail::valid_profile(broken),
            "text instruction past the function end is rejected");
    far[0].offset = profile.functions[potion_body].byte_length - 5u;
    require(sr::detail::valid_profile(broken), "last five bytes of a function are allowed");
    broken.texts = texts.data();
    w.reference.functions()[potion_body].ignore_text = true;
    require(!sr::detail::valid_profile(broken), "text carrier must bind its texts");
    w.reference.functions()[potion_body].ignore_text = false;
    require(sr::detail::valid_profile(broken), "restored text carrier is valid");
  }
}

// Named string constants whose text a verifier reads are compared by length
// and SHA-256 of their zSTRING text, bounded like anonymous texts.
t::Script text_script(const std::string& done) {
  t::Script s;
  s.data("PRINT", 5, 8 | 1, 1);
  s.text("UBC_DONE", done);
  s.data("UBC_COUNT", 2, 1, 1, {}, 2);
  {
    t::Assembler a;
    a.push_var("UBC_DONE").call_external("PRINT").ret();
    s.function("RECIPE_DONE", a);
  }
  {
    t::Assembler a;
    a.push_var("UBC_COUNT").ret();
    s.function("RECIPE_COUNT", a);
  }
  return s;
}

const std::vector<t::Recipe> text_recipes{{apple, {"RECIPE_DONE"}, {"UBC_DONE", "PRINT"}},
                                          {potion, {"RECIPE_COUNT"}, {"UBC_COUNT"}}};

void test_named_texts() {
  const std::string done = "Gut gebraut";
  const auto reference_script = text_script(done);
  auto reference_build = t::build(reference_script);
  t::Reference reference{reference_build, reference_script, text_recipes};
  const auto profile = reference.profile();
  const auto& record = reference.data()[reference.data_id("UBC_DONE")];
  require(sr::detail::valid_profile(profile) && record.check_text &&
              record.text_length == done.size() && !record.check_value,
          "named text record carries length and digest");
  require(relocate(reference_build, profile).result.identity, "named text keeps the identity");
  {
    auto live = t::build(text_script(done), shifted);
    const auto run = relocate(live, profile);
    require(run.result.valid && run.result.active == 2 && !run.result.identity,
            "named text matches in a shifted image");
  }
  {
    auto live = t::build(text_script("Gut gebrauT"));
    require(relocate(live, profile).only_inactive(apple, Issue::symbol_value),
            "changed named text rejects its recipe");
  }
  {
    auto live = t::build(text_script("Gut gebraut!"));
    require(relocate(live, profile).only_inactive(apple, Issue::symbol_value),
            "longer named text rejects its recipe");
  }
  {
    auto live = t::build(text_script(done));
    const auto object = live.image.get(live.symbol_address(live.index.at("UBC_DONE")) + 0x18);
    const auto text = live.image.get(object + 8);
    live.image.bytes.at(text + done.size()) = '!';
    require(relocate(live, profile).only_inactive(apple, Issue::text_unreadable),
            "unterminated named text rejects its recipe");
  }
  {
    auto live = t::build(text_script(done));
    const auto object = live.image.get(live.symbol_address(live.index.at("UBC_DONE")) + 0x18);
    live.image.deny(live.image.get(object + 8) + 3u, 1);
    require(relocate(live, profile).only_inactive(apple, Issue::text_unreadable),
            "unreadable named text rejects its recipe");
    auto oversized = live.image.get(object + 0xc);
    live.image.denied.clear();
    live.image.put(object + 0xc, ubc::script_fingerprint::max_text_length + 1u);
    require(relocate(live, profile).only_inactive(apple, Issue::text_unreadable),
            "named text above the limit is not read");
    live.image.put(object + 0xc, oversized);
    require(relocate(live, profile).result.active == 2, "restored named text matches");
  }
  {
    const std::string long_text(2500, 'x');
    const auto long_script = text_script(long_text);
    auto long_build = t::build(long_script);
    t::Reference long_reference{long_build, long_script, text_recipes};
    auto live = t::build(text_script(long_text), shifted);
    require(relocate(live, long_reference.profile()).result.active == 2,
            "named text across read blocks matches");
    auto changed = t::build(text_script(long_text.substr(0, 2499) + "y"), shifted);
    require(relocate(changed, long_reference.profile()).only_inactive(apple, Issue::symbol_value),
            "last byte of a long named text counts");
  }
  {
    auto unchecked = reference.data();
    unchecked[reference.data_id("UBC_DONE")].check_text = false;
    auto without = profile;
    without.data = unchecked.data();
    auto live = t::build(text_script("Gut gebrauT"));
    require(relocate(live, without).result.active == 2,
            "without check_text only the signature is compared");
  }
  {
    auto broken = reference.data();
    auto invalid = profile;
    invalid.data = broken.data();
    broken[reference.data_id("UBC_COUNT")].check_text = true;
    require(!sr::detail::valid_profile(invalid), "text check on an integer is rejected");
    broken[reference.data_id("UBC_COUNT")].check_text = false;
    broken[reference.data_id("UBC_DONE")].check_value = true;
    require(!sr::detail::valid_profile(invalid), "text check with a value check is rejected");
    broken[reference.data_id("UBC_DONE")].check_value = false;
    broken[reference.data_id("UBC_DONE")].text_length =
        ubc::script_fingerprint::max_text_length + 1u;
    require(!sr::detail::valid_profile(invalid), "text check above the limit is rejected");
  }
}
} // namespace

int main() {
  test_identity();
  test_shifted();
  test_recipe_changes();
  test_invalid_code();
  test_names();
  test_code_entries();
  test_table_blocks();
  test_long_texts();
  test_reads_and_layout();
  test_profile_checks();
  test_station_shares_and_lookups();
  test_named_texts();
  std::printf("script relocation: %u checks, %u failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
