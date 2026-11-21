// SPDX-License-Identifier: MIT
#include "ubc/text/ui_language.hpp"
#include <string_view>

int main() {
  using namespace ubc::ui_text;
  auto language = choose(L" QZ ", [] { return true; });
  return std::string_view(table(language)[Label::title]) == "\xa3\xf3"
                                                            "d\x9f"
             ? 0
             : 1;
}
