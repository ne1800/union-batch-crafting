// SPDX-License-Identifier: MIT
#include "ubc/text/ui_language.hpp"
#include <string_view>

int main() {
  using namespace ubc::ui_text;
  auto language = choose(L" QZ ", [] { return true; });
  return std::string_view(table(language)[Label::title]) == "Test translation" ? 0 : 1;
}
