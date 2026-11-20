// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace ubc {
// A measurement value knows only its output. It cannot grant
// a menu release nor call an engine function or a writer.
struct BowMenuTrace {
  using Emit = void (*)(const char*, const char*, const char*, int, int, int);
  Emit emit;
  bool full;
  unsigned count = 0;

  bool check(const char* field, const char* expected, const char* actual,
             bool ok, int subject=0, int symbol=0) noexcept {
    if (full || !ok) {
      emit(field,expected,actual,ok,subject,symbol);
      ++count;
    }
    return ok;
  }
  bool equal(const char* field, std::int64_t expected, std::int64_t actual,
             int subject=0, int symbol=0) noexcept {
    if (!full && expected==actual) return true;
    char wanted[24], observed[24];
    std::snprintf(wanted,sizeof(wanted),"%lld",static_cast<long long>(expected));
    std::snprintf(observed,sizeof(observed),"%lld",static_cast<long long>(actual));
    return check(field,wanted,observed,expected==actual,subject,symbol);
  }
  bool value(const char* field, const char* expected, std::int64_t actual,
             bool ok, int subject=0, int symbol=0) noexcept {
    if (!full && ok) return true;
    char observed[24];
    std::snprintf(observed,sizeof(observed),"%lld",static_cast<long long>(actual));
    return check(field,expected,observed,ok,subject,symbol);
  }
  void pending(const char* field,const char* expected,const char* reason,int subject=0,int symbol=0) noexcept {
    if (full) { emit(field,expected,reason,-1,subject,symbol); ++count; }
  }
};
} // namespace ubc
