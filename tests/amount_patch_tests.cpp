// SPDX-License-Identifier: MIT
#include "ubc/amount_patch.hpp"
#include <cstdio>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#include "ubc/batch_amount_call.hpp"
#endif

namespace {
unsigned checks = 0, failures = 0;
void check(bool value, int line) { ++checks; if (!value) { ++failures; std::printf("FAIL line %d\n", line); } }
#define CHECK(x) check((x), __LINE__)
struct Fixture {
  std::array<std::uint8_t, 48> bytes{};
  std::array<ubc::AmountOperand, 3> ops{{{1, 5, 1, 0}, {9, 4, 1, 1}, {25, 10, 2, 0}}};
  bool identity = true, writable = true;
  unsigned calls = 0, fail_call = 0, partial = 0;
  unsigned foreign_on_failure = 0, foreign_on_final_read = 0;
#if defined(_WIN32)
  bool raise_after_write = false;
  bool raise_in_effect = false;
  unsigned effects = 0;
  DWORD effect_input_error = 0;
#endif
  Fixture() {
    bytes.fill(0x7f);
    for (const auto& op : ops) {
      bytes[op.offset - 1] = 64;
      for (unsigned j = 0; j < 4; ++j) bytes[op.offset + j] = static_cast<std::uint8_t>(static_cast<std::uint32_t>(op.original) >> (8 * j));
    }
  }
  static bool read(void* p, std::uint32_t at, void* out, std::size_t n) noexcept {
    auto& f = *static_cast<Fixture*>(p);
#if defined(_WIN32)
    SetLastError(4321); // Like Win32 verification calls, outside the engine effect.
#endif
    if (at < 1000 || n > f.bytes.size() || at - 1000 > f.bytes.size() - n) return false;
    if (n == f.bytes.size() && f.calls == f.ops.size() && f.foreign_on_final_read) {
      f.foreign(f.foreign_on_final_read); f.foreign_on_final_read = 0;
    }
    std::memcpy(out, f.bytes.data() + at - 1000, n); return true;
  }
  static bool same(void* p, std::uint32_t at, std::size_t n) noexcept {
    return static_cast<Fixture*>(p)->identity && at == 1000 && n == 48;
  }
  static bool can_write(void* p, std::uint32_t, std::size_t) noexcept { return static_cast<Fixture*>(p)->writable; }
  static bool replace(void* p, std::uint32_t at, const void* old, const void* value, std::size_t n) noexcept {
    auto& f = *static_cast<Fixture*>(p);
    std::array<std::uint8_t, 4> current{};
    if (n > 4 || !read(p, at, current.data(), n) || std::memcmp(current.data(), old, n)) return false;
    ++f.calls;
    if (f.fail_call && f.calls == f.fail_call) {
      if (f.foreign_on_failure) f.foreign(f.foreign_on_failure);
      if (f.partial) std::memcpy(f.bytes.data() + at - 1000, value, f.partial < n ? f.partial : n);
#if defined(_WIN32)
      if(f.raise_after_write){SetLastError(9101);RaiseException(0xe042a001,0,0,nullptr);}
#endif
      return false;
    }
    std::memcpy(f.bytes.data() + at - 1000, value, n); return true;
  }
  ubc::AmountMemory memory() { return {{this, read}, this, same, can_write, replace}; }
  ubc::AmountBody body() { ubc::Sha256 sha; sha.update(bytes.data(), bytes.size()); return {48, sha.finish(), ops.data(), ops.size()}; }
  unsigned at(std::size_t i) { return bytes[ops[i].offset]; }
  void foreign(unsigned value) {
    for (unsigned j=0;j<4;++j) bytes[ops[0].offset+j]=static_cast<std::uint8_t>(value>>(8*j));
  }
  unsigned value(std::size_t i) const {
    unsigned result=0;
    for (unsigned j=0;j<4;++j) result |= static_cast<unsigned>(bytes[ops[i].offset+j])<<(8*j);
    return result;
  }
#if defined(_WIN32)
  static void* __cdecl effect(void* context) {
    auto& f=*static_cast<Fixture*>(context);
    f.effect_input_error=GetLastError();++f.effects;SetLastError(9003);
    if(f.raise_in_effect)RaiseException(0xe042a002,0,0,nullptr);
    return reinterpret_cast<void*>(0x12345678);
  }
#endif
};
#if defined(_WIN32)
bool interrupted_amount(Fixture& f,ubc::AmountPatch& patch,const ubc::AmountBody& body,
                        unsigned quantity,bool& restored,ubc::AmountOutcome& outcome,
                        DWORD& caught,DWORD& error) {
  __try {
    ubc::run_amount_call(patch,f.memory(),1000,body,quantity,Fixture::effect,&f,restored,outcome);
  } __except((GetExceptionCode()==0xe042a001||GetExceptionCode()==0xe042a002)?
      EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){
    caught=GetExceptionCode();error=GetLastError();return true;
  }
  return false;
}
#endif
}
int main() {
  // A composite reader may normalize only its own open N window for
  // hash comparison. The accessor itself performs no reads or writes.
  {
    Fixture f; const auto body=f.body(); ubc::AmountPatch patch;
    CHECK(!patch.matches_active(1000,body,7));CHECK(patch.begin(f.memory(),1000,body,7));
    const auto calls=f.calls;const auto bytes=f.bytes;
    CHECK(patch.matches_active(1000,body,7));CHECK(!patch.matches_active(1000,body,6));
    CHECK(!patch.matches_active(1001,body,7));
    for(unsigned field=0;field<8;++field){auto wrong=body;auto ops=f.ops;wrong.operands=ops.data();
      switch(field){case 0:wrong.digest[0]^=1;break;case 1:wrong.length--;break;case 2:wrong.count--;break;
      case 3:ops[0].offset++;break;case 4:ops[0].original++;break;case 5:ops[0].units++;break;
      case 6:ops[0].reserved++;break;case 7:ops[0].allow_zero=!ops[0].allow_zero;break;}
      CHECK(!patch.matches_active(1000,wrong,7));
    }
    CHECK(f.calls==calls&&f.bytes==bytes);
    // Cached identity expressly does not verify the current external bytes.
    f.foreign(99);CHECK(patch.matches_active(1000,body,7));
    CHECK(!patch.finish());CHECK(!patch.matches_active(1000,body,7));
  }
  for (const auto quantity : {2u, 3u, 5u, 7u, 10u, 23u, 100u}) {
    Fixture f; const auto old = f.bytes; auto body = f.body(); ubc::AmountPatch patch;
    CHECK(patch.begin(f.memory(), 1000, body, quantity)); CHECK(patch.active());
    CHECK(patch.matches_active(1000,body,quantity));
    CHECK(f.at(0) == quantity); CHECK(f.at(1) == quantity - 1); CHECK(f.at(2) == quantity * 2);
    CHECK(!patch.begin(f.memory(), 1000, body, quantity)); CHECK(patch.active());
    CHECK(patch.finish()); CHECK(!patch.active()); CHECK(f.bytes == old); CHECK(patch.finish());
    CHECK(!patch.matches_active(1000,body,quantity));
  }
  for (const auto quantity : {0u, 1u, UINT32_MAX, static_cast<unsigned>(INT32_MAX)}) {
    Fixture f; const auto old = f.bytes; ubc::AmountPatch patch;
    CHECK(!patch.begin(f.memory(), 1000, f.body(), quantity)); CHECK(f.bytes == old); CHECK(f.calls == 0);
  }
  for (unsigned mutation = 0; mutation < 9; ++mutation) {
    Fixture f; auto body = f.body(); ubc::AmountPatch patch;
    if (mutation == 0) body.digest[0] ^= 1;
    if (mutation == 1) f.ops[0].offset = 0;
    if (mutation == 2) f.ops[0].offset = 46;
    if (mutation == 3) f.ops[1].offset = f.ops[0].offset;
    if (mutation == 4) f.ops[0].original = 6;
    if (mutation == 5) f.ops[0].units = 0;
    if (mutation == 6) f.identity = false;
    if (mutation == 7) f.writable = false;
    if (mutation == 8) { f.bytes[0] = 65; body = f.body(); }
    const auto old = f.bytes;
    CHECK(!patch.begin(f.memory(), 1000, body, 7)); CHECK(f.bytes == old); CHECK(f.calls == 0);
  }
  // false does not establish a partial write of our own.
  // Complete values of our own can still be restored. Leave
  // a mixed integer unchanged and deny restoration approval.
  for (unsigned fail = 1; fail <= 3; ++fail) for (unsigned partial = 0; partial <= 4; ++partial) {
    Fixture f; const auto old = f.bytes; auto body = f.body(); f.fail_call = fail; f.partial = partial;
    ubc::AmountPatch patch;
    constexpr unsigned quantity=0x01020304;
    CHECK(!patch.begin(f.memory(), 1000, body, quantity)); CHECK(patch.mutation_attempted());
    if(partial==0||partial==4){
      CHECK(!patch.active()); CHECK(f.bytes==old); CHECK(patch.finish());
    }else{
      auto expected=old;
      const auto& op=f.ops[fail-1];
      const unsigned replacement=op.units*quantity-op.reserved;
      for(unsigned j=0;j<partial;++j)
        expected[op.offset+j]=static_cast<std::uint8_t>(replacement>>(8*j));
      CHECK(patch.active()); CHECK(f.bytes==expected); CHECK(!patch.finish());
      CHECK(f.bytes==expected); CHECK(patch.issue()==ubc::AmountIssue::restore_failed);
    }
  }
  {
    Fixture f; auto body = f.body(); ubc::AmountPatch patch;
    CHECK(patch.begin(f.memory(), 1000, body, 7)); f.bytes[1] = 99;
    CHECK(!patch.finish()); CHECK(f.bytes[1] == 99); CHECK(f.at(1) == 4); CHECK(f.at(2) == 10);
    CHECK(patch.active()); CHECK(patch.issue() == ubc::AmountIssue::restore_failed);
  }
  for (const unsigned foreign : {301u,261u}) {
    Fixture f; auto body = f.body(); ubc::AmountPatch patch;
    CHECK(patch.begin(f.memory(), 1000, body, 300));
    for (unsigned j=0;j<4;++j) f.bytes[1+j]=static_cast<std::uint8_t>(foreign>>(8*j));
    const auto altered=f.bytes;
    CHECK(!patch.finish());
    CHECK(std::memcmp(f.bytes.data()+1,altered.data()+1,4)==0);
    CHECK(f.at(1)==4); CHECK(f.at(2)==10);
  }
  // A later failed exchange does not turn earlier successful operands
  // into possible partial writes. Even 261 is an unrelated whole
  // integer, though each byte matches either the original 5 or our 300.
  for (const unsigned foreign : {301u,261u}) {
    Fixture f; auto body=f.body(); ubc::AmountPatch patch;
    f.fail_call=2; f.foreign_on_failure=foreign;
    CHECK(!patch.begin(f.memory(),1000,body,300));
    CHECK(f.value(0)==foreign); CHECK(f.value(1)==4); CHECK(f.value(2)==10);
    CHECK(patch.active()); CHECK(!patch.finish()); CHECK(f.value(0)==foreign);
  }
  // Likewise, a final readback mismatch does not establish a partial write.
  // Still restore the remaining values that exactly match our replacements.
  for (const unsigned foreign : {301u,261u}) {
    Fixture f; auto body=f.body(); ubc::AmountPatch patch;
    f.foreign_on_final_read=foreign;
    CHECK(!patch.begin(f.memory(),1000,body,300));
    CHECK(f.value(0)==foreign); CHECK(f.value(1)==4); CHECK(f.value(2)==10);
    CHECK(patch.active()); CHECK(!patch.finish()); CHECK(f.value(0)==foreign);
  }
  {
    Fixture f; auto body = f.body(); ubc::AmountPatch patch;
    CHECK(patch.begin(f.memory(), 1000, body, 7)); const auto changed = f.bytes; f.identity = false;
    CHECK(!patch.finish()); CHECK(f.bytes == changed); CHECK(patch.active());
    f.identity = true; CHECK(patch.finish());
  }
  {
    Fixture f; auto body = f.body(); body.count = 0; body.operands = nullptr; ubc::AmountPatch patch;
    CHECK(!patch.begin(f.memory(), 1000, body, 7));
    CHECK(patch.begin(f.memory(), 1000, body, 1)); CHECK(f.calls == 0);
    CHECK(patch.matches_active(1000,body,1));CHECK(!patch.matches_active(1000,body,2));CHECK(patch.finish());
  }
#if defined(_WIN32)
  // Actual SEH after mutation, before replace returns. The journal entry
  // must already be visible. Never assume ownership of partial bytes.
  for(unsigned fail=1;fail<=3;++fail)for(unsigned partial=0;partial<=4;++partial){
    Fixture f; const auto old=f.bytes; auto body=f.body(); ubc::AmountPatch patch;
    f.fail_call=fail;f.partial=partial;f.raise_after_write=true;bool restored=false;
    ubc::AmountOutcome outcome;DWORD caught=0,error=0;
    constexpr unsigned quantity=0x01020304;
    CHECK(interrupted_amount(f,patch,body,quantity,restored,outcome,caught,error));
    CHECK(caught==0xe042a001);CHECK(error==9101);CHECK(f.effects==0);CHECK(!outcome.entered);
    CHECK(outcome.restored==restored);
    CHECK(patch.mutation_attempted());
    if(partial==0||partial==4){
      CHECK(restored);CHECK(!patch.active());CHECK(f.bytes==old);
    }else{
      auto expected=old;const auto& op=f.ops[fail-1];
      const unsigned replacement=op.units*quantity-op.reserved;
      for(unsigned j=0;j<partial;++j)
        expected[op.offset+j]=static_cast<std::uint8_t>(replacement>>(8*j));
      CHECK(!restored);CHECK(patch.active());CHECK(f.bytes==expected);
      CHECK(!patch.finish());CHECK(f.bytes==expected);
    }
  }
  for(unsigned foreign:{301u,261u}){
    Fixture f;auto body=f.body();ubc::AmountPatch patch;
    f.fail_call=2;f.foreign_on_failure=foreign;f.raise_after_write=true;bool restored=false;
    ubc::AmountOutcome outcome;DWORD caught=0,error=0;
    CHECK(interrupted_amount(f,patch,body,300,restored,outcome,caught,error));CHECK(!restored);
    CHECK(caught==0xe042a001&&error==9101&&f.effects==0&&!outcome.entered&&!outcome.restored);
    CHECK(f.value(0)==foreign);CHECK(f.value(1)==4);CHECK(f.value(2)==10);
    CHECK(patch.active());CHECK(!patch.finish());CHECK(f.value(0)==foreign);
  }
  {
    Fixture f;const auto old=f.bytes;auto body=f.body();ubc::AmountPatch patch;
    f.raise_in_effect=true;bool restored=false;ubc::AmountOutcome outcome;DWORD caught=0,error=0;
    SetLastError(1234);
    CHECK(interrupted_amount(f,patch,body,7,restored,outcome,caught,error));
    CHECK(caught==0xe042a002&&error==9003);CHECK(f.effects==1);CHECK(f.effect_input_error==1234);
    CHECK(outcome.entered&&outcome.restored&&restored);CHECK(!patch.active());CHECK(f.bytes==old);
  }
  {
    Fixture f;const auto old=f.bytes;auto body=f.body();ubc::AmountPatch patch;
    bool restored=false;ubc::AmountOutcome outcome;SetLastError(1234);
    const auto result=ubc::run_amount_call(patch,f.memory(),1000,body,7,Fixture::effect,&f,restored,outcome);
    const auto error=GetLastError();
    CHECK(result==reinterpret_cast<void*>(0x12345678));CHECK(f.effects==1);CHECK(f.effect_input_error==1234);
    CHECK(error==9003);CHECK(outcome.entered&&outcome.restored&&restored);CHECK(f.bytes==old);
  }
  {
    // Even without operands, begin owns an active transaction.
    // Close it before the corresponding memory context ends.
    ubc::AmountPatch patch;
    for(unsigned turn=0;turn<3;++turn){
      Fixture f;const auto old=f.bytes;auto body=f.body();
      if(turn<2){body.count=0;body.operands=nullptr;}
      bool restored=false;ubc::AmountOutcome outcome;
      const auto result=ubc::run_amount_call(patch,f.memory(),1000,body,turn<2?1u:2u,
        Fixture::effect,&f,restored,outcome);
      CHECK(result==reinterpret_cast<void*>(0x12345678));CHECK(f.effects==1);
      CHECK(outcome.entered&&outcome.restored&&restored);CHECK(!patch.active());CHECK(f.bytes==old);
      CHECK(patch.finish());
    }
  }
#endif
  std::printf("AmountPatch: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
