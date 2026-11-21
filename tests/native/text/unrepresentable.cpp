// SPDX-License-Identifier: MIT
#include "ubc/text/table.hpp"

inline constexpr ubc::ui_text::SourceRow<unsigned> words[] = {{1, U"Ж"}};
constexpr auto invalid = ubc::ui_text::encoded_rows<1252, words>;
