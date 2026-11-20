// SPDX-License-Identifier: MIT
#include "ubc/stations/cool/cool_parser_prefix.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>

namespace prefix = ubc::cool_parser_prefix;
namespace base = ubc::alchemy_flask_debit;
namespace {
std::uint32_t checks = 0, failures = 0;
void check(bool condition, const char* label) {
  ++checks;
  if (!condition) { ++failures; std::cerr << "FAIL " << label << '\n'; }
}
prefix::Context context() { return {1, 2, ubc::lhiver::game_parser, 3, 4, 5, 6}; }
prefix::ParserState state() {
  prefix::ParserState value;
  value.known = value.instance_bindings_verified = value.data_words_known = true;
  value.current_parser = ubc::lhiver::game_parser;
  value.code_base = 0x01000000;
  value.code_size = 1000;
  value.pop_pos = 500;
  value.self_symbol = 100;
  value.hero_symbol = 200;
  value.use_symbol = value.self_symbol;
  value.use_address = value.self = value.hero = context().hero;
  value.other = 300;
  value.victim = 400;
  return value;
}
prefix::ParserState after(std::uint32_t callback, std::uint32_t print) {
  auto value = state();
  if (callback != prefix::cancel_callback) {
    value.data_depth = callback == prefix::single_callback ? 2u : 4u;
    value.data_words = {print, prefix::integer_tag, 1, prefix::integer_tag, 0, 0, 0, 0};
  }
  return value;
}
bool accepted(const prefix::ParserState& value, std::uint32_t callback) {
  return prefix::classify(value, context(), callback, prefix::Phase::after_original).recognized;
}
}
int main() {
  constexpr auto maximum = std::numeric_limits<std::uint32_t>::max();
  constexpr std::array<std::uint32_t, 3> callbacks{
      prefix::single_callback, prefix::triple_callback, prefix::cancel_callback};
  check(!prefix::runtime_ready, "kein Laufzeitfreigabebaustein");
  check(prefix::printscreen_declared_arguments == 5 && prefix::printscreen_native_reads == 6,
        "separater belegter Print-ABI-Sonderfall");
  for (const auto callback : callbacks) {
    check(prefix::classify(state(), context(), callback, prefix::Phase::before_original).recognized,
          "BEFORE leer");
    auto empty = state(); empty.data_words_known = false;
    check(prefix::classify(empty, context(), callback, prefix::Phase::before_original).recognized,
          "leerer Stack benoetigt keine aktiven Woerter");
    for (std::uint32_t depth = 1; depth <= 9; ++depth) {
      auto dirty = state(); dirty.data_depth = depth;
      check(!prefix::classify(dirty, context(), callback, prefix::Phase::before_original).recognized,
            "BEFORE nichtleerer Prefix gesperrt");
    }
    for (const auto print : {0u, 1u}) {
      const auto good = after(callback, print);
      const auto result = prefix::classify(good, context(), callback, prefix::Phase::after_original);
      check(result.recognized, "exakte Originalform erkannt");
      check(result.print == (callback == prefix::cancel_callback ? prefix::PrintResult::not_called :
                            print == 0 ? prefix::PrintResult::returned_zero : prefix::PrintResult::returned_one),
            "Print0 und Print1 bleiben getrennt");
      check(prefix::unchanged(good, good, context(), callback, prefix::Phase::after_original),
            "identische Aufnahmen");
      check(!base::valid_after_original(good, context(), callback), "Alchemiewhitelist bleibt geschlossen");
      for (std::uint32_t depth = 0; depth <= 10; ++depth) {
        auto changed = good; changed.data_depth = depth;
        check(accepted(changed, callback) == (depth == good.data_depth), "exakte Stacktiefe");
      }
      for (const auto depth : {2048u, 2049u, maximum}) {
        auto changed = good; changed.data_depth = depth;
        check(!accepted(changed, callback), "uebergrosser Stack");
      }
      for (std::uint32_t word = 0; word < good.data_depth; ++word) {
        for (std::uint32_t bit = 0; bit < 32; ++bit) {
          auto changed = good; changed.data_words[word] ^= std::uint32_t{1} << bit;
          const bool alternate_print = word == 0 && bit == 0;
          check(accepted(changed, callback) == alternate_print, "einzelne Prefixbitmutation");
          check(!prefix::unchanged(good, changed, context(), callback, prefix::Phase::after_original),
                "aktiver Prefix muss bytegleich bleiben");
        }
      }
      for (std::uint32_t word = good.data_depth; word < good.data_words.size(); ++word) {
        auto changed = good; changed.data_words[word] = maximum;
        check(prefix::unchanged(good, changed, context(), callback, prefix::Phase::after_original),
              "ungenutzte Zellen sind kein aktiver Prefix");
      }
      auto unknown = good; unknown.data_words_known = false;
      check(accepted(unknown, callback) == (callback == prefix::cancel_callback), "aktive Woerter brauchen Lesebeleg");
      auto unknown_state = good; unknown_state.known = false;
      check(!accepted(unknown_state, callback), "unbekannter Parser");
      auto unknown_bindings = good; unknown_bindings.instance_bindings_verified = false;
      check(!accepted(unknown_bindings, callback), "Symbolbindung fehlt");
      for (const auto field : {&prefix::ParserState::current_parser, &prefix::ParserState::code_base,
                              &prefix::ParserState::code_size, &prefix::ParserState::use_symbol,
                              &prefix::ParserState::use_address, &prefix::ParserState::self_symbol,
                              &prefix::ParserState::hero_symbol, &prefix::ParserState::self,
                              &prefix::ParserState::hero}) {
        auto changed = good; changed.*field = 0;
        check(!accepted(changed, callback), "fehlende Parseridentitaet");
      }
      auto mixed = good; mixed.self_symbol = mixed.hero_symbol;
      check(!accepted(mixed, callback), "SELF und HERO Metadaten nicht aliasieren");
      auto other_actor = good; other_actor.self = good.hero + 1;
      check(!accepted(other_actor, callback), "fremder SELF");
      auto alias = good; alias.use_symbol = alias.hero_symbol;
      check(accepted(alias, callback), "positiv gebundener HERO-UseInstance");
      check(!prefix::unchanged(good, alias, context(), callback, prefix::Phase::after_original),
            "UseInstance-Wechsel ist nicht unveraendert");
      for (const auto field : {&prefix::ParserState::pop_pos, &prefix::ParserState::other, &prefix::ParserState::victim}) {
        auto changed = good; ++(changed.*field);
        check(accepted(changed, callback), "separat gueltiger Zustand");
        check(!prefix::unchanged(good, changed, context(), callback, prefix::Phase::after_original),
              "Cursor oder rohe Nebenakteure geaendert");
      }
      auto cursor = good; cursor.pop_pos = cursor.code_size + 1;
      check(!accepted(cursor, callback), "Cursor ausserhalb Code");
      auto range = good; range.code_base = maximum;
      check(!accepted(range, callback), "Codeadressueberlauf");
      auto code_size = good; code_size.code_size = 0x1000001;
      check(!accepted(code_size, callback), "unplausible Codegroesse");
      auto zero_epoch = context(); zero_epoch.epoch = 0;
      check(!prefix::classify(good, zero_epoch, callback, prefix::Phase::after_original).recognized,
            "Epoch0 gesperrt");
      for (const auto field : {&prefix::Context::thread, &prefix::Context::parser, &prefix::Context::game,
                              &prefix::Context::world, &prefix::Context::hero, &prefix::Context::mob}) {
        auto bad = context(); bad.*field = 0;
        check(!prefix::classify(good, bad, callback, prefix::Phase::after_original).recognized,
              "fehlende Contextidentitaet");
      }
      check(!prefix::classify(good, context(), callback, static_cast<prefix::Phase>(2)).recognized,
            "unbekannte Phase");
    }
  }
  for (const auto callback : {0u, 20289u, 20370u, 20395u, 20774u, 20777u, maximum}) {
    check(!prefix::classify(state(), context(), callback, prefix::Phase::before_original).recognized,
          "fremder Callback BEFORE");
    for (const auto source : callbacks)
      check(!accepted(after(source, 1), callback), "fremder Callback AFTER");
  }
  auto foreign_stack = after(prefix::triple_callback, 1);
  foreign_stack.data_depth = 8; foreign_stack.data_words = {1,64,1,64,1,64,1,64};
  for (const auto callback : callbacks)
    check(!accepted(foreign_stack, callback), "Geistprefix ist keine Kuehlung");
  std::cout << "{\"checks\":" << checks << ",\"failures\":" << failures << "}\n";
  return failures ? 1 : 0;
}
