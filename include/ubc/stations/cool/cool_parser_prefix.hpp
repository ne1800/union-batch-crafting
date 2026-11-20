// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/alchemy/alchemy_flask_debit.hpp"

namespace ubc::cool_parser_prefix {
namespace values = alchemy_flask_debit;
using Context = values::Context;
using ParserState = values::ParserState;

// Pure offline classifier. This is not a release for a Nativecall,
// an item/parser restore, station close, or a visible receipt.
inline constexpr bool runtime_ready = false;
inline constexpr std::uint32_t single_callback = 20775;
inline constexpr std::uint32_t triple_callback = 20778;
inline constexpr std::uint32_t cancel_callback = 20772;
inline constexpr std::uint32_t printscreen_entry = 0x006e3070;
inline constexpr std::uint32_t printscreen_declared_arguments = 5;
inline constexpr std::uint32_t printscreen_native_reads = 6;
inline constexpr std::uint32_t integer_tag = 0x40;

enum class Phase { before_original, after_original };
enum class PrintResult { not_called, returned_zero, returned_one, unknown };
struct Classification {
  bool recognized = false;
  PrintResult print = PrintResult::unknown;
};

inline constexpr bool known_callback(std::uint32_t callback) noexcept {
  return callback == single_callback || callback == triple_callback || callback == cancel_callback;
}

// The three VOID original callbacks begin after CallFunc.Clear with an
// empty DataStack. AI_PRINTSCREEN consumes the five DAT arguments and reads
// natively a sixth INT from the empty stack: Pop yields 0 without depth
// change. Then [0|1,0x40] remain, and for the trio additionally [1,0x40].
// PrintReturn1 alone does not prove a displayed text object. In particular,
// neither the extra alchemy nor the spirit whitelist may be extended here.
inline Classification classify(const ParserState& state, const Context& context,
                               std::uint32_t callback, Phase phase) noexcept {
  if (!values::valid(context) || !known_callback(callback) ||
      !values::valid_parser_bindings(state, context))
    return {};
  if (phase == Phase::before_original)
    return state.data_depth == 0 ? Classification{true, PrintResult::not_called} : Classification{};
  if (phase != Phase::after_original)
    return {};
  if (callback == cancel_callback)
    return state.data_depth == 0 ? Classification{true, PrintResult::not_called} : Classification{};
  const auto expected = callback == single_callback ? 2u : 4u;
  if (!state.data_words_known || state.data_depth != expected || state.data_words[0] > 1 ||
      state.data_words[1] != integer_tag)
    return {};
  if (callback == triple_callback &&
      (state.data_words[2] != 1 || state.data_words[3] != integer_tag))
    return {};
  return {true, state.data_words[0] == 0 ? PrintResult::returned_zero : PrintResult::returned_one};
}

// Two individually recognized prefixes are not enough: a change from
// PrintReturn0 to 1 or from Cursor/UseInstance/Actors is also a change.
// Only active data words compare; unused stack cells are irrelevant.
inline bool unchanged(const ParserState& before, const ParserState& after, const Context& context,
                      std::uint32_t callback, Phase phase) noexcept {
  return classify(before, context, callback, phase).recognized &&
         classify(after, context, callback, phase).recognized && values::same(before, after);
}
} // namespace ubc::cool_parser_prefix
