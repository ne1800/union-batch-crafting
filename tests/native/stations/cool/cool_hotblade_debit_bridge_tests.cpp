// SPDX-License-Identifier: MIT
#include "bridges/cool_hotblade_debit_bridge.hpp"
#include <intrin.h>
#include <cstdio>
namespace b = ubc::cool_hotblade_debit_bridge;
namespace d = ubc::cool_hotblade_debit;
namespace {
unsigned checks = 0, failures = 0;
#define CHECK(x)                                                                                   \
  do {                                                                                             \
    ++checks;                                                                                      \
    if (!(x)) {                                                                                    \
      ++failures;                                                                                  \
      std::fprintf(stderr, "line %d mode %u: %s\n", __LINE__, current_mode, #x);                   \
    }                                                                                              \
  } while (false)
unsigned current_mode = 0;
constexpr DWORD input_error = 0x10203040, output_error = 0x50607080;
constexpr DWORD native_exception = 0xe042b431, reader_exception = 0xe042b432,
                restore_exception = 0xe042b433;
enum class Mode {
  normal,
  print0,
  tmp2,
  item_noop,
  null_return,
  before_fence,
  before_no_exclusion,
  before_alias,
  before_stock,
  before_fault,
  gate_fence,
  gate_parser,
  gate_item,
  gate_fault,
  before_reentry,
  restore_reentry,
  native_before_fault,
  native_after_fault,
  combined_fault,
  changed_prefix,
  native_reentry,
  no_debit,
  changed_cold,
  destroy_started,
  pop_fault,
  instance_fault,
  item_fault,
  pop_noop,
  instance_noop,
  item_failed_write,
  changed_item_target,
  changed_actor,
  fence_after_pop,
  fence_after_instance,
  restore_fault,
  stale_restore,
  stale_after,
  after_hand,
  after_scope,
  after_fault,
  foreign
};
struct Host {
  Mode mode;
  d::Tracker tracker;
  b::Executor* executor = nullptr;
  d::Job job{};
  d::Admission admission{};
  d::Observation before{}, now{};
  d::ParserState saved{};
  d::ItemBinding saved_item{};
  std::uint64_t serial = 2;
  unsigned removes = 0, pops = 0, instances = 0, items = 0, restore_reads = 0;
  explicit Host(Mode m) : mode(m) {
    job = {
        {1, GetCurrentThreadId(), ubc::lhiver::game_parser, 0x100000, 0x200000, 0x300000, 0x400000},
        10,
        20,
        30,
        d::callback,
        d::owner,
        2};
    admission = {job.context,
                 job.id,
                 job.writer_token,
                 job.fence_token,
                 1,
                 true,
                 true,
                 true,
                 true,
                 true,
                 true,
                 true,
                 m == Mode::tmp2 ? 2 : 1};
    before.context = job.context;
    before.scope = {10, 20, d::callback, 1, 0, 0, true};
    before.serial = 1;
    before.stocks = {true, {{{16612, 2, true, true}, {16613, 5, true, false}}}};
    before.hand = {true, true, 0x600000, 0, d::hot, 1, 0x600000, admission.verified_left_tmp_level};
    before.stack = {true, true, true, true, 0, 1, 0x500000, d::hot, 2, 1, 0x83c804, 1, 0x200001};
    auto& p = before.parser;
    p.known = p.instance_bindings_verified = true;
    p.current_parser = job.context.parser;
    p.code_base = 0x10000000;
    p.code_size = ubc::lhiver::code_length;
    p.pop_pos = 0x1234;
    p.self_symbol = 0x700000;
    p.hero_symbol = 0x700100;
    p.self = p.hero = p.use_address = job.context.hero;
    p.use_symbol = p.self_symbol;
    p.other = 0x800000;
    before.item = {true,
                   true,
                   job.context.parser,
                   0x700200,
                   0x900000,
                   p.code_base,
                   p.code_size,
                   ubc::lhiver::symbol_count,
                   1682,
                   7,
                   1,
                   0,
                   0x999000,
                   1584};
    before.fence = {true, true, 30, 1, 0, 0};
    before.production = 13;
    before.invincible = before.start_smithing = 1;
    before.native_contract = before.stable_read = true;
    CHECK(tracker.arm(job, admission, before));
    now = before;
    now.serial = now.fence.serial = 2;
    now.production = now.invincible = 0;
    now.stocks.stocks[1].amount = 7;
    now.parser.data_depth = 2;
    now.parser.data_words_known = true;
    now.parser.data_words = {m == Mode::print0 ? 0u : 1u, 0x40};
    now.parser.pop_pos = 0x2345;
    now.item.value = 0x888000;
    saved = now.parser;
    saved_item = now.item;
    CHECK(tracker.original_returned(now, true, true));
  }
};
Host* active = nullptr;
void clobber() {
  SetLastError(999);
  unsigned csr = 0x5f80;
  __asm { fninit }
  __asm { fldz }
  __asm { ldmxcsr csr }
}
bool __cdecl read(void* context, b::ReadStage stage, d::Observation& out) {
  auto& h = *static_cast<Host*>(context);
  clobber();
  const bool after = stage == b::ReadStage::after_debit;
  CHECK(h.executor->diagnostics().active);
  CHECK(h.executor->diagnostics().native_scope == after);
  if ((!after && h.mode == Mode::before_fault) || (after && h.mode == Mode::after_fault))
    RaiseException(reader_exception, 0, 0, nullptr);
  if (!after && h.mode == Mode::before_reentry)
    (void)h.executor->run();
  out = h.now;
  out.serial = out.fence.serial = ++h.serial;
  out.scope.debit_depth = after ? 1 : 0;
  if (!after) {
    if (h.mode == Mode::before_fence)
      out.fence.known = false;
    if (h.mode == Mode::before_no_exclusion)
      out.fence.excludes_dispatch = false;
    if (h.mode == Mode::before_alias)
      out.stack.object = out.hand.left;
    if (h.mode == Mode::before_stock)
      out.stocks.stocks[0].amount = 3;
  } else {
    if (h.mode == Mode::stale_after)
      --out.serial;
    if (h.mode == Mode::after_hand)
      ++out.hand.left;
    if (h.mode == Mode::after_scope)
      ++out.scope.writer_token;
  }
  return true;
}
bool __cdecl read_restore(void* context, const d::Call& call, d::RestoreObservation& out) {
  auto& h = *static_cast<Host*>(context);
  ++h.restore_reads;
  clobber();
  CHECK(h.executor->diagnostics().active && h.executor->diagnostics().native_scope);
  if ((!h.removes && h.mode == Mode::gate_fault) || (h.removes && h.mode == Mode::restore_fault))
    RaiseException(reader_exception, 0, 0, nullptr);
  if (h.mode == Mode::restore_reentry)
    (void)h.executor->run();
  const auto& s = call.parser;
  out.serial = ++h.serial;
  out.fence = {true, true, 30, out.serial, 0, 0};
  out.native_item_set_offset_contract = true;
  out.targets = {call.job.context,
                 {10, 20, d::callback, 1, 0, 1, true},
                 true,
                 true,
                 s.current_parser,
                 s.code_base,
                 s.code_size,
                 s.use_symbol,
                 s.use_address,
                 s.self_symbol,
                 s.hero_symbol,
                 s.self,
                 s.hero,
                 s.other,
                 s.victim};
  out.current = h.now.parser;
  out.item = h.now.item;
  if (!h.removes) {
    if (h.mode == Mode::gate_fence)
      out.fence.excludes_dispatch = false;
    if (h.mode == Mode::gate_parser)
      ++out.current.pop_pos;
    if (h.mode == Mode::gate_item)
      ++out.item.value;
  } else {
    if (h.mode == Mode::changed_item_target)
      out.item.symbol_object += 4;
    if (h.mode == Mode::changed_actor)
      ++out.current.self;
    if (h.mode == Mode::destroy_started)
      out.fence.destroy_started = 1;
    if ((h.mode == Mode::fence_after_pop && h.pops) ||
        (h.mode == Mode::fence_after_instance && h.instances))
      out.fence.excludes_dispatch = false;
    if (h.mode == Mode::stale_restore && h.pops) {
      --out.serial;
      out.fence.serial = out.serial;
    }
  }
  return true;
}
void* __thiscall remove_native(void* hero, int symbol, int quantity) {
  auto& h = *active;
  ++h.removes;
  CHECK(reinterpret_cast<std::uintptr_t>(hero) == h.job.context.hero && symbol == 16612 &&
        quantity == 1);
  CHECK(GetLastError() == input_error);
  SetLastError(output_error);
  if (h.mode == Mode::native_before_fault)
    RaiseException(native_exception, 0, 0, nullptr);
  if (h.mode != Mode::no_debit) {
    --h.now.stocks.stocks[0].amount;
    --h.now.stack.amount;
  }
  if (h.mode == Mode::changed_cold)
    ++h.now.stocks.stocks[1].amount;
  h.now.parser.pop_pos = 0x3456;
  h.now.parser.use_symbol = 0xdeaddea0;
  h.now.parser.use_address = 0xdeaddea4;
  if (h.mode == Mode::changed_prefix)
    h.now.parser.data_words[0] ^= 1;
  if (h.mode != Mode::item_noop)
    h.now.item.value = 0xdeaddea8;
  unsigned csr = 0x3f80;
  __asm { fninit }
  __asm { fld1 }
  __asm { ldmxcsr csr }
  if (h.mode == Mode::native_reentry)(void) h.executor->run();
  if (h.mode == Mode::native_after_fault || h.mode == Mode::combined_fault)
    RaiseException(native_exception, 0, 0, nullptr);
  return h.mode == Mode::null_return ? nullptr : reinterpret_cast<void*>(0xdeadc0de);
}
void __thiscall restore_pop(void* stack, int pos) {
  auto& h = *active;
  ++h.pops;
  clobber();
  CHECK(reinterpret_cast<std::uintptr_t>(stack) == ubc::lhiver::game_parser + 0x48u &&
        static_cast<std::uint32_t>(pos) == h.saved.pop_pos);
  if (h.mode == Mode::pop_fault)
    RaiseException(restore_exception, 0, 0, nullptr);
  if (h.mode != Mode::pop_noop)
    h.now.parser.pop_pos = static_cast<std::uint32_t>(pos);
}
void __cdecl restore_instance(void* symbol) {
  auto& h = *active;
  ++h.instances;
  clobber();
  CHECK(reinterpret_cast<std::uintptr_t>(symbol) == h.saved.use_symbol);
  if (h.mode == Mode::instance_fault)
    RaiseException(restore_exception, 0, 0, nullptr);
  if (h.mode != Mode::instance_noop) {
    h.now.parser.use_symbol = h.saved.use_symbol;
    h.now.parser.use_address = h.saved.use_address;
  }
}
void __thiscall restore_item(void* symbol, int value) {
  auto& h = *active;
  ++h.items;
  clobber();
  CHECK(reinterpret_cast<std::uintptr_t>(symbol) == h.saved_item.symbol_object &&
        static_cast<std::uint32_t>(value) == h.saved_item.value);
  if (h.mode == Mode::item_fault || h.mode == Mode::combined_fault)
    RaiseException(restore_exception, 0, 0, nullptr);
  if (h.mode != Mode::item_failed_write)
    h.now.item.value = static_cast<std::uint32_t>(value);
}
DWORD WINAPI foreign(void* executor) {
  return static_cast<b::Executor*>(executor)->run() == b::Outcome::rejected ? 0u : 1u;
}
bool invoke(b::Executor& executor, b::Outcome& out, DWORD& exception) {
  bool normal = false;
  __try {
    out = executor.run();
    normal = true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    exception = GetExceptionCode();
  }
  return normal;
}
void one(Mode mode) {
  current_mode = static_cast<unsigned>(mode);
  Host h(mode);
  active = &h;
  const b::Config config{GetCurrentThreadId(), &h,           remove_native, restore_pop,
                         restore_instance,     restore_item, read,          read_restore};
  b::Executor executor(h.tracker, config);
  h.executor = &executor;
  CHECK(executor.diagnostics().configured);
  if (mode == Mode::foreign) {
    HANDLE thread = CreateThread(nullptr, 0, foreign, &executor, 0, nullptr);
    CHECK(thread != nullptr);
    if (thread) {
      CHECK(WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0);
      DWORD code = 1;
      CHECK(GetExitCodeThread(thread, &code) && code == 0);
      CloseHandle(thread);
    }
  }
  unsigned esp_before = 0, esp_after = 0, seh_before = 0, seh_after = 0;
  alignas(16) unsigned char fx[512], after_fx[512];
  _fxsave(fx);
  __asm { mov eax,fs:[0] }
  __asm { mov seh_before, eax }
  __asm { mov esp_before,esp }
  SetLastError(input_error);
  b::Outcome result = b::Outcome::rejected;
  DWORD exception = 0;
  const bool normal = invoke(executor, result, exception);
  const DWORD error = GetLastError();
  _fxsave(after_fx);
  _fxrstor(fx);
  __asm { mov eax,fs:[0] }
  __asm { mov seh_after, eax }
  __asm { mov esp_after,esp }
  CHECK(esp_before == esp_after && seh_before == seh_after);
  const bool native_fault = mode == Mode::native_before_fault || mode == Mode::native_after_fault ||
                            mode == Mode::combined_fault;
  CHECK(normal != native_fault);
  if (native_fault)
    CHECK(exception == native_exception);
  const bool no_call =
      (mode >= Mode::before_fence && mode <= Mode::restore_reentry) || mode == Mode::foreign;
  CHECK(h.removes == (no_call ? 0u : 1u));
  CHECK(error == (no_call ? input_error : output_error));
  CHECK(!executor.diagnostics().active && !executor.diagnostics().native_scope);
  const bool success = mode <= Mode::null_return;
  if (success) {
    CHECK(result == b::Outcome::debit_verified && !executor.diagnostics().disabled &&
          !h.tracker.status().blocked);
    CHECK(h.tracker.status().stage ==
          d::Stage::debit_verified); // No invented Destroy or end evidence.
    CHECK(h.now.stocks.stocks[0].amount == 1 && h.now.stocks.stocks[1].amount == 7);
    CHECK(d::values::same(h.saved, h.now.parser) &&
          d::same_item(h.saved_item, h.now.item, h.saved));
    CHECK(h.pops == 1 && h.instances == 1 && h.items == (mode == Mode::item_noop ? 0u : 1u));
    CHECK(*reinterpret_cast<const unsigned*>(after_fx + 24) ==
          0x3f80); // Preserve the native MXCSR.
    CHECK(after_fx[4] ==
          0x80); // Native x87 usage: exactly one occupied register (no full value comparison).
  } else {
    CHECK(result == b::Outcome::rejected && executor.diagnostics().disabled &&
          h.tracker.status().blocked);
    CHECK(h.tracker.status().stage != d::Stage::debit_verified);
  }
  if (no_call || mode == Mode::changed_prefix || mode == Mode::changed_item_target ||
      mode == Mode::changed_actor || mode == Mode::destroy_started || mode == Mode::restore_fault ||
      mode == Mode::native_reentry)
    CHECK(h.pops == 0 && h.instances == 0 && h.items == 0);
  if (mode == Mode::fence_after_pop || mode == Mode::stale_restore)
    CHECK(h.pops == 1 && !h.instances && !h.items);
  if (mode == Mode::fence_after_instance)
    CHECK(h.pops == 1 && h.instances == 1 && !h.items);
  if (mode == Mode::pop_fault || mode == Mode::instance_fault || mode == Mode::combined_fault)
    CHECK(h.pops == 1 && h.instances == 1 &&
          h.items == 1); // Still attempt parts that can be independently verified afresh.
  const auto count = h.removes;
  (void)executor.run();
  CHECK(h.removes == count); // Do not run even a successful bridge twice.
  active = nullptr;
}
}
int main() {
  static_assert(!d::runtime_ready);
  for (unsigned i = 0; i <= static_cast<unsigned>(Mode::foreign); ++i)
    one(static_cast<Mode>(i));
  Host h(Mode::normal);
  b::Executor missing(h.tracker, {});
  CHECK(!missing.diagnostics().configured && missing.run() == b::Outcome::rejected);
  std::printf("cool_hotblade_debit_bridge: %u cases, %u checks, %u failures\n",
              static_cast<unsigned>(Mode::foreign) + 2, checks, failures);
  return failures ? 1 : 0;
}
