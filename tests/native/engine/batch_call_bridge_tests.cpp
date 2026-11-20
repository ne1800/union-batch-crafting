// SPDX-License-Identifier: MIT
#include <windows.h>
#include <intrin.h>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "ubc/engine/batch_call_bridge.hpp"

namespace b = ubc::batch_call;
namespace {
unsigned checks = 0, failures = 0;
void check(bool result, const char* label) {
  ++checks;
  if (!result) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", label);
  }
}
bool is_denied(void* pointer) {
  int value = 1;
  SIZE_T bytes = 0;
  return pointer &&
         ReadProcessMemory(GetCurrentProcess(), pointer, &value, sizeof(value), &bytes) &&
         bytes == sizeof(value) && value == 0;
}
int parser_storage = 0;
DWORD owner = 0;
volatile LONG originals = 0, writers = 0, signatures = 0, selections = 0;
volatile LONG request = 0, mode = 0, depth = 0, largest_readers = 0;
bool valid_signature = true, prove_restore = true;
HANDLE entered = nullptr, release_reader = nullptr;
void* nested_result = nullptr;
DWORD original_error = 0;
std::uintptr_t selected_caller = 0, writer_caller = 0;
constexpr DWORD test_exception_code = 0xe042b001;
constexpr std::uintptr_t result_value = 0x12345678;
using Invoke = void*(__cdecl*)(void*, int, ...);
Invoke invoke() { return reinterpret_cast<Invoke>(&UbcBatchCallBridge); }

bool __cdecl signature(const b::Call& call) noexcept {
  InterlockedIncrement(&signatures);
  SetLastError(7001);
  __asm { pxor xmm0,xmm0 }
  return valid_signature && call.parser == &parser_storage;
}
b::Decision __cdecl select(const b::Call& call) noexcept {
  InterlockedIncrement(&selections);
  SetLastError(7002);
  if (call.thread == owner)
    selected_caller = call.caller;
  return call.thread == owner && InterlockedCompareExchange(&request, 0, 0) ? b::Decision::writer
                                                                            : b::Decision::ordinary;
}
DWORD WINAPI foreign_call(void*) {
  nested_result = invoke()(&parser_storage, 11);
  return 0;
}
void* __cdecl original(void* parser, int symbol, ...) {
  original_error = GetLastError();
  InterlockedIncrement(&originals);
  const auto current = InterlockedIncrement(&depth);
  const auto status = b::diagnostics();
  if (status.readers > static_cast<unsigned>(largest_readers))
    InterlockedExchange(&largest_readers, static_cast<LONG>(status.readers));
  if (symbol == 999) {
    va_list args;
    va_start(args, symbol);
    const int one = va_arg(args, int), two = va_arg(args, int), three = va_arg(args, int);
    va_end(args);
    InterlockedDecrement(&depth);
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(one + two + three));
  }
  if (mode == 1 && current == 1)
    nested_result = invoke()(parser, symbol);
  if (mode == 2 && current == 1) {
    InterlockedExchange(&request, 1);
    nested_result = invoke()(parser, symbol);
    InterlockedExchange(&request, 0);
  }
  if (mode == 3) {
    SetEvent(entered);
    WaitForSingleObject(release_reader, 10000);
  }
  InterlockedDecrement(&depth);
  if (mode == 4)
    RaiseException(test_exception_code, 0, 0, nullptr);
  SetLastError(8001);
  return reinterpret_cast<void*>(result_value);
}
std::uint64_t last_writer_token = 0;
void* __cdecl writer(b::WriterScope& scope) {
  InterlockedIncrement(&writers);
  check(scope.token > last_writer_token,
        "actual exclusive writer issues a fresh monotone token, also after configure");
  last_writer_token = scope.token;
  check(b::writer_undisturbed(scope),
        "fresh exclusive token starts with its actual symbol rejection counter");
  writer_caller = scope.call.caller;
  void* result = nullptr;
  __try {
    if (mode == 5) {
      const LONG before = signatures;
      valid_signature = false;
      nested_result = invoke()(scope.call.parser, scope.call.symbol);
      check(signatures == before, "writer reentry blocked before signature callback");
      valid_signature = true;
    }
    if (mode == 6) {
      HANDLE thread = CreateThread(nullptr, 0, foreign_call, nullptr, 0, nullptr);
      check(thread != nullptr, "create foreign reader during writer");
      if (thread) {
        check(WaitForSingleObject(thread, 10000) == WAIT_OBJECT_0,
              "foreign writer-window call returns");
        CloseHandle(thread);
      }
    }
    check(b::writer_undisturbed(scope) == (mode != 5 && mode != 6),
          "nested or concurrent rejected target is visible to its actual writer");
    result = scope.original(scope.call.parser, scope.call.symbol);
  } __finally {
    // The synthetic callback writes no bytecode. Only this explicit evidence
    // authorizes the writer. Production integration must actually read the data.
    scope.restore_verified = prove_restore;
  }
  return result;
}
bool reset() {
  request = mode = depth = largest_readers = 0;
  originals = writers = signatures = selections = 0;
  valid_signature = prove_restore = true;
  nested_result = nullptr;
  owner = GetCurrentThreadId();
  selected_caller = writer_caller = 0;
  b::Config config{};
  config.parser = &parser_storage;
  config.symbol_count = 2;
  config.symbols[0] = 11;
  config.symbols[1] = 22;
  config.signature = signature;
  config.select = select;
  config.writer = writer;
  UbcBatchCallOriginal = reinterpret_cast<void*>(&original);
  return b::configure(config);
}
bool raises() {
  __try {
    invoke()(&parser_storage, 11);
  } __except (GetExceptionCode() == test_exception_code ? EXCEPTION_EXECUTE_HANDLER
                                                        : EXCEPTION_CONTINUE_SEARCH) {
    return true;
  }
  return false;
}
}

extern "C" {
alignas(16) unsigned char caller_fx[512]{}, expected_fx[512]{}, observed_fx[512]{};
alignas(16) unsigned char xmm_pattern[16]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
unsigned observed_gpr[7]{}, observed_args[5]{}, expected_flags = 0, observed_flags = 0;
unsigned expected_stack = 0, observed_stack = 0, returned_value = 0;
unsigned expected_caller = 0;
void* fixture_parser = nullptr;
__declspec(naked) void* __cdecl probe_caller() {
  __asm {
    lea eax,bridge_return
    mov dword ptr [expected_caller],eax
    push 11
    push dword ptr [fixture_parser]
    call UbcBatchCallBridge
  bridge_return:
    add esp,8
    ret
  }
}
__declspec(naked) void* __cdecl capture_original(void*, int, ...) {
  __asm {
    mov dword ptr [observed_gpr],eax
    mov dword ptr [observed_gpr+4],ecx
    mov dword ptr [observed_gpr+8],edx
    mov dword ptr [observed_gpr+12],ebx
    mov dword ptr [observed_gpr+16],esi
    mov dword ptr [observed_gpr+20],edi
    mov dword ptr [observed_gpr+24],ebp
    mov dword ptr [observed_stack],esp
    pushfd
    pop dword ptr [observed_flags]
    fxsave [observed_fx]
    mov eax,dword ptr [esp+4]
    mov dword ptr [observed_args],eax
    mov eax,dword ptr [esp+8]
    mov dword ptr [observed_args+4],eax
    mov eax,dword ptr [esp+12]
    mov dword ptr [observed_args+8],eax
    mov eax,dword ptr [esp+16]
    mov dword ptr [observed_args+12],eax
    mov eax,dword ptr [esp+20]
    mov dword ptr [observed_args+16],eax
    mov eax,5a005a00h
    ret
  }
}
__declspec(naked) void __cdecl probe_registers() {
  __asm {
    pushfd
    pushad
    fxsave [caller_fx]
    fninit
    fld1
    movdqu xmm0,xmmword ptr [xmm_pattern]
    fxsave [expected_fx]
    push 303
    push 202
    push 101
    push 11
    push dword ptr [fixture_parser]
    lea eax,[esp-4]
    mov dword ptr [expected_stack],eax
    push 247h
    popfd
    pushfd
    pop dword ptr [expected_flags]
    mov eax,11111111h
    mov ecx,22222222h
    mov edx,33333333h
    mov ebx,44444444h
    mov esi,55555555h
    mov edi,66666666h
    mov ebp,77777777h
    call UbcBatchCallBridge
    mov dword ptr [returned_value],eax
    add esp,20
    fxrstor [caller_fx]
    popad
    popfd
    ret
  }
}
}

int main() {
  UbcBatchCallOriginal = reinterpret_cast<void*>(&original);
  check(invoke()(&parser_storage, 999, 1, 2, 3) == reinterpret_cast<void*>(6),
        "unconfigured bridge tailpasses complete arguments");
  check(reset(), "configure bounded callback contract");
  SetLastError(1234);
  check(invoke()(&parser_storage, 11) == reinterpret_cast<void*>(result_value),
        "ordinary target keeps return value");
  check(originals == 1 && writers == 0 && largest_readers == 1,
        "ordinary target exactly once under reader");
  check(original_error == 1234 && GetLastError() == 8001,
        "callback inspection and reader cleanup preserve LastError");
  check(b::diagnostics().readers == 0 && !b::diagnostics().writer,
        "ordinary return releases reader");
  check(invoke()(&parser_storage, 999, 100, 20, 3) == reinterpret_cast<void*>(123),
        "unregistered varargs survive tail jump");
  check(signatures == 1 && selections == 1, "unregistered function skips callbacks");
  check(invoke()(reinterpret_cast<void*>(0x1234), 999, 10, 20, 30) == reinterpret_cast<void*>(60),
        "other parser retains varargs");

  check(reset(), "reset reentry fixture");
  mode = 1;
  check(invoke()(&parser_storage, 11) == reinterpret_cast<void*>(result_value),
        "reader reentry returns outer result");
  check(originals == 2 && largest_readers == 2 && b::diagnostics().readers == 0,
        "reader reentry counted and released twice");
  check(reset(), "reset writer-from-reader fixture");
  mode = 2;
  invoke()(&parser_storage, 11);
  check(originals == 1 && writers == 0 && is_denied(nested_result),
        "writer request inside reader rejected without fixed-quantity fallback");
  check(b::diagnostics().readers == 0, "reader remains balanced after writer refusal");

  check(reset(), "reset foreign reader fixture");
  mode = 3;
  entered = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  release_reader = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  check(entered && release_reader, "create bounded synchronization events");
  HANDLE thread = entered && release_reader
                      ? CreateThread(nullptr, 0, foreign_call, nullptr, 0, nullptr)
                      : nullptr;
  check(thread != nullptr, "create foreign reader");
  if (thread) {
    check(WaitForSingleObject(entered, 10000) == WAIT_OBJECT_0,
          "foreign original reached under reader lease");
    request = 1;
    const auto denied = invoke()(&parser_storage, 11);
    check(is_denied(denied) && originals == 1 && writers == 0,
          "running foreign reader prevents writer");
    check(b::diagnostics().readers == 1, "denial leaves foreign reader lease intact");
    SetEvent(release_reader);
    check(WaitForSingleObject(thread, 10000) == WAIT_OBJECT_0, "foreign reader finishes");
    CloseHandle(thread);
  }
  if (entered)
    CloseHandle(entered);
  if (release_reader)
    CloseHandle(release_reader);
  check(b::diagnostics().readers == 0, "foreign reader cleanup");

  for (int scenario : {0, 5, 6}) {
    check(reset(), "reset exclusive writer fixture");
    request = 1;
    mode = scenario;
    SetLastError(1234);
    check(invoke()(&parser_storage, 11) == reinterpret_cast<void*>(result_value),
          "writer preserves original pointer return");
    check(writers == 1 && originals == 1, "writer original exactly once");
    check(!b::diagnostics().writer && !b::diagnostics().writes_disabled && !b::quarantined(11),
          "verified writer restores and releases");
    check(GetLastError() == 8001, "writer cleanup preserves original LastError");
    if (scenario)
      check(is_denied(nested_result), "writer excludes nested or foreign target");
  }
  check(reset(), "reset exact caller fixture");
  request = 1;
  fixture_parser = &parser_storage;
  check(probe_caller() == reinterpret_cast<void*>(result_value),
        "known caller thunk returns original result");
  check(selected_caller == expected_caller && writer_caller == expected_caller,
        "select and writer see original return address, not bridge helper");

  check(reset(), "reset reader exception fixture");
  mode = 4;
  check(raises(), "reader exception propagates to outer handler");
  check(!b::diagnostics().readers && !b::diagnostics().writer, "reader SEH unwind releases lease");
  check(reset(), "reset writer exception fixture");
  mode = 4;
  request = 1;
  check(raises(), "writer exception propagates to outer handler");
  check(!b::diagnostics().writer && !b::quarantined(11),
        "writer proven cleanup survives SEH unwind");
  check(reset(), "reset unproven exception fixture");
  mode = 4;
  request = 1;
  prove_restore = false;
  check(raises(), "unproven writer exception still propagates");
  check(!b::diagnostics().writer && b::quarantined(11) && b::diagnostics().writes_disabled,
        "unproven unwind quarantines target permanently");
  mode = 0;
  request = 0;
  const auto before = originals;
  const auto quarantined = invoke()(&parser_storage, 11);
  check(is_denied(quarantined) && originals == before, "quarantine never falls through original");
  check(invoke()(&parser_storage, 22) == reinterpret_cast<void*>(result_value),
        "different unmodified target remains callable");
  request = 1;
  const auto disabled = invoke()(&parser_storage, 22);
  check(is_denied(disabled) && writers == 1, "unproven restore disables future writers");

  check(reset(), "reset no-proof normal return fixture");
  request = 1;
  prove_restore = false;
  invoke()(&parser_storage, 11);
  check(b::quarantined(11) && b::diagnostics().writes_disabled,
        "missing proof on normal return also quarantines");
  check(reset(), "reset signature tail fixture");
  valid_signature = false;
  UbcBatchCallOriginal = reinterpret_cast<void*>(&capture_original);
  fixture_parser = &parser_storage;
  probe_registers();
  const std::array<unsigned, 7> expected{
      {0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555, 0x66666666, 0x77777777}};
  for (std::size_t i = 0; i < expected.size(); ++i)
    check(observed_gpr[i] == expected[i],
          "signature-failure tail preserves incoming integer register");
  check(observed_flags == expected_flags && observed_stack == expected_stack,
        "signature-failure tail preserves flags and exact native stack");
  check(observed_args[0] == reinterpret_cast<unsigned>(&parser_storage) && observed_args[1] == 11 &&
            observed_args[2] == 101 && observed_args[3] == 202 && observed_args[4] == 303,
        "signature-failure tail retains all varargs");
  check(std::memcmp(expected_fx, observed_fx, sizeof(expected_fx)) == 0,
        "signature-failure tail restores x87 XMM and MXCSR state");
  check(returned_value == 0x5a005a00 && b::diagnostics().writes_disabled,
        "tail return intact and unknown signature disables writing");
  check(!b::diagnostics().readers, "signature-failure tail releases preparatory reader");
  check(reset(), "reset owned invalid signature fixture");
  request = 1;
  valid_signature = false;
  const auto rejected = invoke()(&parser_storage, 11);
  check(is_denied(rejected) && originals == 0 && writers == 0,
        "owned invalid signature rejected rather than executing original fallback");
  check(!b::diagnostics().writer && b::diagnostics().writes_disabled,
        "invalid writer signature releases lease and disables writes");
  check(reset(), "reset explicit quarantine fixture");
  check(b::quarantine(11) && b::quarantined(11) && !b::quarantine(999),
        "quarantine limited to configured targets");
  const auto count = signatures;
  invoke()(&parser_storage, 11);
  check(originals == 0 && signatures == count, "quarantine checked before all signature fallbacks");
  check(reset(), "reset registry boundary fixture");
  b::Config registry{};
  registry.parser = &parser_storage;
  registry.signature = signature;
  registry.select = select;
  registry.writer = writer;
  registry.symbol_count = b::max_symbols;
  for (std::size_t i = 0; i < b::max_symbols; ++i)
    registry.symbols[i] = static_cast<int>(i + 10);
  check(b::configure(registry), "full 256-target registry accepted");
  invoke()(&parser_storage, static_cast<int>(b::max_symbols + 9));
  check(signatures == 1 && originals == 1, "last bounded registry entry uses reader path");
  registry.symbol_count = b::max_symbols + 1;
  check(!b::configure(registry), "oversized registry rejected without indexing beyond bound");
  registry.symbol_count = b::max_symbols;
  registry.symbols[255] = registry.symbols[0];
  check(!b::configure(registry), "duplicate target rejected");
  registry.symbol_count = 1;
  registry.symbols[0] = -1;
  check(!b::configure(registry), "negative registered symbol rejected");
  check(!b::diagnostics().readers && !b::diagnostics().writer,
        "registry rejection leaves no lease");
  std::printf("BatchCallBridge: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
