// SPDX-License-Identifier: MIT
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "ubc/ui/menu_model.hpp"
#include "../ui/native_ui_language.hpp"
#include "ubc/engine/native_menu_abi.hpp"
#include "ubc/engine/pe_guard.hpp"
#include "ubc/core/sha256.hpp"

#if !defined(_M_IX86)
#error Die Menueprobe benoetigt die Windows-x86-ABI.
#endif

namespace {
namespace abi = ubc::native_menu;

HMODULE own_module = nullptr;
HANDLE diagnostic = INVALID_HANDLE_VALUE;
volatile LONG hooks_installed = 0;
bool preview_attempted = false;
ubc::ui_text::Language preview_language = &ubc::ui_text::en::table;
bool preview_render_failed = false;
void* own_menu = nullptr;
ubc::MenuModel model;
std::uint32_t preview_frames = 0;

struct PreparedHook {
  void* trampoline = nullptr;
  std::array<std::uint8_t, 13> patch{};
  DWORD original_protection = 0;
  bool written = false;
};
std::array<PreparedHook, 3> hooks{};
abi::MenuRun original_run = nullptr;
abi::MenuHandleEvent original_handle_event = nullptr;
abi::ObjectFunction original_render = nullptr;

int __fastcall run_probe(void* self, void* unused_edx);
int __fastcall handle_probe(void* self, void* unused_edx, int key);
void __fastcall render_probe(void* self, void* unused_edx);

bool memory_readable(const void* pointer, std::size_t size, bool executable = false,
                     bool engine = false) noexcept {
  auto current = reinterpret_cast<std::uintptr_t>(pointer);
  if (!current || size > UINTPTR_MAX - current)
    return false;
  const auto end = current + size;
  if (engine && (current < abi::image_base || end > abi::image_base + abi::image_size))
    return false;
  while (current < end) {
    MEMORY_BASIC_INFORMATION memory{};
    if (VirtualQuery(reinterpret_cast<const void*>(current), &memory, sizeof(memory)) !=
            sizeof(memory) ||
        memory.State != MEM_COMMIT || (memory.Protect & PAGE_GUARD))
      return false;
    if (engine && (memory.AllocationBase != reinterpret_cast<void*>(abi::image_base) ||
                   memory.Type != MEM_IMAGE))
      return false;
    const DWORD access = memory.Protect & 0xff;
    const bool can_execute = access == PAGE_EXECUTE_READ || access == PAGE_EXECUTE_READWRITE ||
                             access == PAGE_EXECUTE_WRITECOPY;
    if (executable ? !can_execute
                   : !(can_execute || access == PAGE_READONLY || access == PAGE_READWRITE ||
                       access == PAGE_WRITECOPY))
      return false;
    const auto region = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    if (memory.RegionSize > UINTPTR_MAX - region)
      return false;
    const auto next = region + memory.RegionSize;
    if (next <= current)
      return false;
    current = next < end ? next : end;
  }
  return true;
}

bool target_identity_matches() noexcept {
  const auto* base = reinterpret_cast<const std::uint8_t*>(GetModuleHandleW(nullptr));
  if (reinterpret_cast<std::uintptr_t>(base) != abi::image_base ||
      !memory_readable(base, 4096, false, true) || !ubc::matches_gothic2_header(base, 4096))
    return false;

  wchar_t path[32768]{};
  const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
  if (!length || length >= 32768)
    return false;
  const HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE)
    return false;
  LARGE_INTEGER before{}, after{};
  bool valid = GetFileSizeEx(file, &before) && before.QuadPart > 0;
  ubc::Sha256 hash;
  std::array<std::uint8_t, 16384> buffer{};
  std::uint64_t total = 0;
  while (valid) {
    DWORD read = 0;
    if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr)) {
      valid = false;
      break;
    }
    if (!read)
      break;
    hash.update(buffer.data(), read);
    total += read;
  }
  valid = valid && GetFileSizeEx(file, &after) && before.QuadPart == after.QuadPart &&
          total == static_cast<std::uint64_t>(before.QuadPart);
  CloseHandle(file);
  return valid && hash.finish() == abi::executable_sha256;
}

template <std::size_t N>
bool sibling_path(wchar_t (&path)[32768], const wchar_t (&filename)[N]) noexcept {
  const DWORD length = GetModuleFileNameW(own_module, path, 32768);
  if (!length || length >= 32768)
    return false;
  DWORD directory = length;
  while (directory && path[directory - 1] != L'\\' && path[directory - 1] != L'/')
    --directory;
  if (!directory || N > 32768 - directory)
    return false;
  std::memcpy(path + directory, filename, sizeof(filename));
  return true;
}

bool consume_activation_marker() noexcept {
  wchar_t path[32768]{};
  if (!sibling_path(path, L"UnionBatchCraftingMenuProbe.enable-once"))
    return false;
  const DWORD attributes = GetFileAttributesW(path);
  if (attributes == INVALID_FILE_ATTRIBUTES ||
      (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
    return false;
  const HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                  FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
  if (file == INVALID_HANDLE_VALUE)
    return false;
  constexpr char expected[] = "UI_PROBE_ONLY_V1\n";
  std::array<char, sizeof(expected)> bytes{};
  DWORD read = 0;
  LARGE_INTEGER size{};
  const bool exact =
      GetFileSizeEx(file, &size) && size.QuadPart == sizeof(expected) - 1 &&
      ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) &&
      read == sizeof(expected) - 1 &&
      std::memcmp(bytes.data(), expected, sizeof(expected) - 1) == 0;
  CloseHandle(file);
  return exact && DeleteFileW(path);
}

void log_line(const char* text) noexcept {
  if (diagnostic == INVALID_HANDLE_VALUE)
    return;
  DWORD written = 0;
  WriteFile(diagnostic, text, static_cast<DWORD>(std::strlen(text)), &written, nullptr);
}

void open_diagnostic() noexcept {
  wchar_t path[32768]{};
  if (!sibling_path(path, L"UnionBatchCraftingMenuProbe.log"))
    return;
  diagnostic = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
  log_line("probe_version=1\r\ntarget_identity_match=true\r\n"
           "activation_marker_consumed=true\r\nrecipe_calls=false\r\n");
}

bool hook_bytes_match(std::size_t index) noexcept {
  const auto& site = abi::hook_sites[index];
  const auto& hook = hooks[index];
  const auto* memory = reinterpret_cast<const std::uint8_t*>(site.address);
  if (!memory_readable(memory, site.guard_size, true, true))
    return false;
  if (!hook.written)
    return std::memcmp(memory, site.original.data(), site.guard_size) == 0;
  return std::memcmp(memory, hook.patch.data(), site.patch_size) == 0 &&
         std::memcmp(memory + site.patch_size, site.original.data() + site.patch_size,
                     site.guard_size - site.patch_size) == 0;
}

bool all_hook_bytes_match() noexcept {
  for (std::size_t i = 0; i < hooks.size(); ++i)
    if (!hook_bytes_match(i))
      return false;
  return true;
}

void relative_jump(std::uint8_t* bytes, std::uintptr_t from, std::uintptr_t to) noexcept {
  bytes[0] = 0xe9;
  // x86 EIP arithmetic is modulo 2^32. No signed overflow in C++.
  const std::uint32_t displacement =
      static_cast<std::uint32_t>(to) - static_cast<std::uint32_t>(from + 5);
  std::memcpy(bytes + 1, &displacement, sizeof(displacement));
}

void free_trampolines() noexcept {
  for (auto& hook : hooks) {
    if (hook.trampoline)
      VirtualFree(hook.trampoline, 0, MEM_RELEASE);
    hook.trampoline = nullptr;
  }
}

bool prepare_trampolines() noexcept {
  const std::array<std::uintptr_t, 3> destinations{
      {reinterpret_cast<std::uintptr_t>(&run_probe),
       reinterpret_cast<std::uintptr_t>(&handle_probe),
       reinterpret_cast<std::uintptr_t>(&render_probe)}};
  for (std::size_t i = 0; i < hooks.size(); ++i) {
    const auto& site = abi::hook_sites[i];
    auto& hook = hooks[i];
    const auto size = site.patch_size + 5;
    hook.trampoline = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!hook.trampoline) {
      free_trampolines();
      return false;
    }
    auto* code = static_cast<std::uint8_t*>(hook.trampoline);
    std::memcpy(code, site.original.data(), site.patch_size);
    relative_jump(code + site.patch_size, reinterpret_cast<std::uintptr_t>(code) + site.patch_size,
                  site.address + site.patch_size);
    DWORD previous = 0;
    if (!VirtualProtect(code, size, PAGE_EXECUTE_READ, &previous) ||
        !FlushInstructionCache(GetCurrentProcess(), code, size)) {
      free_trampolines();
      return false;
    }
    hook.patch.fill(0x90);
    relative_jump(hook.patch.data(), site.address, destinations[i]);
  }
  original_run = reinterpret_cast<abi::MenuRun>(hooks[0].trampoline);
  original_handle_event = reinterpret_cast<abi::MenuHandleEvent>(hooks[1].trampoline);
  original_render = reinterpret_cast<abi::ObjectFunction>(hooks[2].trampoline);
  return true;
}

bool rollback_hooks() noexcept {
  bool restored = true;
  for (std::size_t i = hooks.size(); i-- > 0;) {
    auto& hook = hooks[i];
    if (!hook.written)
      continue;
    const auto& site = abi::hook_sites[i];
    auto* entry = reinterpret_cast<void*>(site.address);
    DWORD previous = 0;
    if (!hook_bytes_match(i) ||
        !VirtualProtect(entry, site.patch_size, PAGE_EXECUTE_READWRITE, &previous)) {
      restored = false;
      continue;
    }
    std::memcpy(entry, site.original.data(), site.patch_size);
    hook.written = false;
    DWORD discarded = 0;
    const bool protection_restored =
        VirtualProtect(entry, site.patch_size, hook.original_protection, &discarded) != FALSE;
    const bool cache_flushed =
        FlushInstructionCache(GetCurrentProcess(), entry, site.patch_size) != FALSE;
    if (!protection_restored || !cache_flushed)
      restored = false;
  }
  return restored;
}

bool install_hooks() noexcept {
  if (!all_hook_bytes_match()) {
    log_line("hooks_installed=false\r\nrejection=hook_guard_mismatch\r\n");
    return false;
  }
  if (!prepare_trampolines()) {
    log_line("hooks_installed=false\r\nrejection=trampoline_setup_failed\r\n");
    return false;
  }
  HMODULE pinned = nullptr;
  if (!all_hook_bytes_match() ||
      !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          reinterpret_cast<LPCWSTR>(&run_probe), &pinned)) {
    free_trampolines();
    log_line("hooks_installed=false\r\nrejection=guard_or_module_pin_failed\r\n");
    return false;
  }
  bool complete = true;
  for (std::size_t i = 0; i < hooks.size(); ++i) {
    const auto& site = abi::hook_sites[i];
    auto& hook = hooks[i];
    auto* entry = reinterpret_cast<void*>(site.address);
    // Verify our existing patches too before each further write.
    if (!all_hook_bytes_match() || !VirtualProtect(entry, site.patch_size, PAGE_EXECUTE_READWRITE,
                                                   &hook.original_protection)) {
      complete = false;
      break;
    }
    std::memcpy(entry, hook.patch.data(), site.patch_size);
    hook.written = true;
    DWORD discarded = 0;
    const bool protection_restored =
        VirtualProtect(entry, site.patch_size, hook.original_protection, &discarded) != FALSE;
    const bool cache_flushed =
        FlushInstructionCache(GetCurrentProcess(), entry, site.patch_size) != FALSE;
    if (!protection_restored || !cache_flushed) {
      complete = false;
      break;
    }
  }
  if (!complete || !all_hook_bytes_match()) {
    const bool restored = rollback_hooks();
    if (restored)
      free_trampolines();
    // If rollback is incomplete, retain the module and trampolines.
    // Without the enable flag the run hook only forwards to the original.
    log_line(restored ? "hooks_installed=false\r\nrejection=install_rolled_back\r\n"
                      : "hooks_installed=false\r\nrejection=rollback_incomplete\r\n");
    return false;
  }
  InterlockedExchange(&hooks_installed, 1);
  log_line("hooks_installed=true\r\n");
  return true;
}

void* global_object(std::uintptr_t address) noexcept {
  const auto* location = reinterpret_cast<const void*>(address);
  if (!memory_readable(location, sizeof(std::uint32_t), false, true))
    return nullptr;
  auto* object = reinterpret_cast<void*>(abi::read<std::uint32_t>(location, 0));
  return memory_readable(object, sizeof(std::uint32_t)) ? object : nullptr;
}

std::uintptr_t input_method(void* input, std::size_t slot) noexcept {
  if (!input || !memory_readable(input, sizeof(std::uint32_t)))
    return 0;
  const auto table = abi::read<std::uint32_t>(input, 0);
  const auto* entry = reinterpret_cast<const void*>(table + slot * sizeof(std::uint32_t));
  if (!memory_readable(entry, sizeof(std::uint32_t)))
    return 0;
  const auto method = abi::read<std::uint32_t>(entry, 0);
  return memory_readable(reinterpret_cast<const void*>(method), 1, true) ? method : 0;
}

bool key_down(void* input, int key) noexcept {
  const auto method = input_method(input, abi::key_pressed_slot);
  // A missing input object does not confirm that a held key was released.
  return !method || reinterpret_cast<abi::KeyPressed>(method)(input, key) != 0;
}

bool game_focused() noexcept {
  const HWND window = GetForegroundWindow();
  DWORD process = 0;
  return window && GetWindowThreadProcessId(window, &process) && process == GetCurrentProcessId();
}

bool engine_ready() noexcept {
  void* input = global_object(abi::input_pointer);
  if (!input_method(input, abi::key_pressed_slot) ||
      !input_method(input, abi::clear_key_buffer_slot) || !global_object(abi::screen_pointer))
    return false;
  constexpr std::array<std::uintptr_t, 8> functions{
      {abi::menu_ctor, abi::menu_dtor, abi::string_ctor, abi::string_dtor, abi::view_clear_printwin,
       abi::view_set_font, abi::view_print, abi::input_logical_key}};
  for (const auto function : functions)
    if (!memory_readable(reinterpret_cast<const void*>(function), 1, true, true))
      return false;
  return true;
}

class NativeString {
public:
  explicit NativeString(const char* text) {
    reinterpret_cast<abi::StringConstructor>(abi::string_ctor)(storage_.bytes.data(), text);
  }
  ~NativeString() {
    reinterpret_cast<abi::ObjectFunction>(abi::string_dtor)(storage_.bytes.data());
  }
  NativeString(const NativeString&) = delete;
  NativeString& operator=(const NativeString&) = delete;
  const void* get() const noexcept { return storage_.bytes.data(); }

private:
  abi::StringStorage storage_;
};

void print_line(void* view, int y, const char* text) {
  NativeString line(text);
  reinterpret_cast<abi::ViewPrint>(abi::view_print)(view, 700, y, line.get());
}

void log_model() noexcept {
  char line[160]{};
  std::snprintf(line, sizeof(line), "ui_state row=%u quantity=%u confirmations=%u closing=%u\r\n",
                model.row(), static_cast<unsigned>(model.quantity()),
                static_cast<unsigned>(model.confirmation_count()), model.closing() ? 1U : 0U);
  log_line(line);
}

ubc::MenuAction map_key(int key) noexcept {
  using Action = ubc::MenuAction;
  switch (key) {
  case abi::key_return:
    return Action::Confirm;
  case abi::key_escape:
    return Action::Back;
  case abi::key_left:
    return Action::Left;
  case abi::key_right:
    return Action::Right;
  case abi::key_up:
    return Action::Up;
  case abi::key_down:
    return Action::Down;
  default:
    break;
  }
  if (key <= 0 || key > 0xffff)
    return Action::None;
  void* input = global_object(abi::input_pointer);
  if (!input)
    return Action::None;
  const auto logical = reinterpret_cast<abi::LogicalKey>(abi::input_logical_key)(
      input, static_cast<unsigned short>(key));
  switch (logical) {
  case abi::game_left:
  case abi::game_strafe_left:
    return Action::Left;
  case abi::game_right:
  case abi::game_strafe_right:
    return Action::Right;
  case abi::game_up:
    return Action::Up;
  case abi::game_down:
    return Action::Down;
  default:
    return Action::None;
  }
}

bool is_exact_main_menu(const void* self) noexcept {
  if (!memory_readable(self, abi::menu_size))
    return false;
  const auto table = abi::read<std::uint32_t>(self, abi::vptr_offset);
  // Main agent, first main menu test on 2026-10-31: L'Hiver uses the
  // native base class zCMenu (00830C3C) for MENU_MAIN, not oCMenu_Main.
  // Check both known types, always requiring the exact instance name.
  if (table != abi::menu_vtable && table != abi::main_menu_vtable)
    return false;
  if (abi::read<std::int32_t>(self, abi::menu_name + abi::string_length) !=
      static_cast<std::int32_t>(sizeof(abi::main_menu_name) - 1))
    return false;
  const auto* name = reinterpret_cast<const char*>(
      abi::read<std::uint32_t>(self, abi::menu_name + abi::string_data));
  // Read only ten validated bytes including the terminating
  // NUL. No strlen or unbounded engine string call is needed.
  return memory_readable(name, sizeof(abi::main_menu_name)) &&
         std::memcmp(name, abi::main_menu_name, sizeof(abi::main_menu_name)) == 0;
}

int __fastcall run_probe(void* self, void*) {
  if (!InterlockedCompareExchange(&hooks_installed, 0, 0) || preview_attempted ||
      !is_exact_main_menu(self) ||
      !memory_readable(reinterpret_cast<const void*>(abi::in_game_menu), 4, false, true) ||
      abi::read<std::int32_t>(reinterpret_cast<const void*>(abi::in_game_menu), 0) != 0)
    return original_run(self);

  preview_attempted = true;
  if (!engine_ready()) {
    log_line("preview_opened=false\r\nrejection=engine_not_ready\r\n");
    return original_run(self);
  }
  preview_language = ubc::ui_text::read_language(true);
  abi::MenuStorage storage;
  void* menu = storage.bytes.data();
  reinterpret_cast<abi::ObjectFunction>(abi::menu_ctor)(menu);
  if (abi::read<std::uint32_t>(menu, 0) != abi::menu_vtable ||
      abi::read<std::int32_t>(menu, abi::menu_item_count) != 0) {
    reinterpret_cast<abi::ObjectFunction>(abi::menu_dtor)(menu);
    log_line("preview_opened=false\r\nrejection=native_menu_layout\r\n");
    return original_run(self);
  }
  abi::write<std::int32_t>(menu, abi::menu_default_out_game, -1);
  abi::write<std::int32_t>(menu, abi::menu_default_in_game, -1);
  abi::write<std::int32_t>(menu, abi::menu_main_selection, -1);
  model.reset();
  preview_frames = 0;
  preview_render_failed = false;
  own_menu = menu;
  log_line("preview_opened=true\r\n");
  original_run(menu);
  own_menu = nullptr;
  reinterpret_cast<abi::ObjectFunction>(abi::menu_dtor)(menu);
  char line[96]{};
  std::snprintf(line, sizeof(line), "preview_closed=true\r\npreview_frames=%u\r\n",
                static_cast<unsigned>(preview_frames));
  log_line(line);
  return original_run(self);
}

int __fastcall handle_probe(void* self, void*, int key) {
  if (!own_menu || self != own_menu)
    return original_handle_event(self, key);
  if (!game_focused())
    model.observe(false, false, false);
  else if (model.on_action(map_key(key)))
    log_model();
  // Unbound events must not operate the underlying menu either.
  return 1;
}

void __fastcall render_probe(void* self, void*) {
  if (!own_menu || self != own_menu) {
    original_render(self);
    return;
  }
  if (preview_frames != UINT32_MAX)
    ++preview_frames;
  void* input = global_object(abi::input_pointer);
  model.observe(key_down(input, abi::key_return), key_down(input, abi::key_escape), game_focused());
  auto* view = reinterpret_cast<void*>(abi::read<std::uint32_t>(self, abi::menu_inner_window));
  if (!memory_readable(view, abi::view_size) ||
      abi::read<std::int32_t>(view, abi::view_id) != abi::view_item) {
    if (!preview_render_failed)
      log_line("rejection=inner_view_unavailable\r\n");
    preview_render_failed = true;
  }
  // The error persists across focus loss. When focus returns,
  // request closure again, but never without a release.
  if (preview_render_failed && model.focused() && !model.closing() && !model.close_ready())
    model.request_close();
  if (model.close_ready()) {
    const auto clear = input_method(input, abi::clear_key_buffer_slot);
    if (clear)
      reinterpret_cast<abi::ObjectFunction>(clear)(input);
    abi::write<std::int32_t>(self, abi::menu_exit_state, abi::exit_back);
  }
  if (preview_render_failed) {
    original_render(self);
    return;
  }
  reinterpret_cast<abi::ObjectFunction>(abi::view_clear_printwin)(view);
  NativeString font("FONT_OLD_10_WHITE.TGA");
  reinterpret_cast<abi::ViewSetFont>(abi::view_set_font)(view, font.get());
  using L = ubc::ui_text::Label;
  const auto& text = ubc::ui_text::table(preview_language);
  print_line(view, 600, text[L::probe_title]);
  print_line(view, 1050, text[L::probe_scope]);
  print_line(view, 1450, text[L::probe_demo]);
  char line[160]{};
  std::snprintf(line, sizeof(line), text[L::probe_amount], model.row() == 0 ? ">" : " ",
                static_cast<unsigned>(model.quantity()));
  print_line(view, 2100, line);
  const char* labels[] = {text[L::preset_one], text[L::preset_five],   text[L::preset_ten],
                          text[L::preset_max], text[L::probe_confirm], text[L::probe_main]};
  for (unsigned row = 1; row < 7; ++row) {
    std::snprintf(line, sizeof(line), "%s %s", model.row() == row ? ">" : " ", labels[row - 1]);
    print_line(view, 2100 + static_cast<int>(row) * 480, line);
  }
  if (model.closing()) {
    print_line(view, 5900, text[L::probe_release_close]);
  } else if (!model.armed()) {
    print_line(view, 5900, text[L::probe_release]);
  } else if (model.confirmation_count()) {
    std::snprintf(line, sizeof(line), text[L::probe_result],
                  static_cast<unsigned>(model.confirmed_quantity()));
    print_line(view, 5900, line);
  }
  print_line(view, 6700, text[L::probe_controls]);
  original_render(self);
}

} // namespace

extern "C" unsigned __cdecl UbcMenuProbeVersion() { return 1; }
extern "C" int __cdecl UbcMenuProbeHooksInstalled() {
  return static_cast<int>(InterlockedCompareExchange(&hooks_installed, 0, 0));
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    own_module = instance;
    if (target_identity_matches() && consume_activation_marker()) {
      open_diagnostic();
      install_hooks();
    }
  } else if (reason == DLL_PROCESS_DETACH && diagnostic != INVALID_HANDLE_VALUE) {
    CloseHandle(diagnostic);
    diagnostic = INVALID_HANDLE_VALUE;
  }
  return TRUE;
}
