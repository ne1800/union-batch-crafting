// SPDX-License-Identifier: MIT
#pragma once

#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ubc/pe_guard.hpp"
#include "ubc/sha256.hpp"

namespace ubc::windows_probe {

inline bool readable(const void* pointer, std::size_t size,
                     bool executable = false, bool engine = false) noexcept {
  auto current = reinterpret_cast<std::uintptr_t>(pointer);
  if (!current || size > UINTPTR_MAX - current) return false;
  const auto end = current + size;
  if (engine && (current < 0x00400000u || end > 0x00b52000u)) return false;
  while (current < end) {
    MEMORY_BASIC_INFORMATION memory{};
    if (VirtualQuery(reinterpret_cast<const void*>(current), &memory, sizeof(memory)) !=
            sizeof(memory) || memory.State != MEM_COMMIT ||
        (memory.Protect & PAGE_GUARD)) return false;
    if (engine && (memory.AllocationBase != reinterpret_cast<void*>(0x00400000u) ||
                   memory.Type != MEM_IMAGE)) return false;
    const auto access = memory.Protect & 0xffu;
    const bool can_execute = access == PAGE_EXECUTE_READ ||
        access == PAGE_EXECUTE_READWRITE || access == PAGE_EXECUTE_WRITECOPY;
    if (executable ? !can_execute :
        !(can_execute || access == PAGE_READONLY || access == PAGE_READWRITE ||
          access == PAGE_WRITECOPY)) return false;
    const auto base = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    if (memory.RegionSize > UINTPTR_MAX - base) return false;
    const auto next = base + memory.RegionSize;
    if (next <= current) return false;
    current = next < end ? next : end;
  }
  return true;
}

// ReadProcessMemory also reports a region that has disappeared after VirtualQuery
// as an error. No unprotected dereferencing of foreign engine objects.
inline bool copy_memory(void*, std::uint32_t address, void* output,
                        std::size_t size) noexcept {
  if (!readable(reinterpret_cast<const void*>(address), size)) return false;
  SIZE_T copied = 0;
  return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address),
                           output, size, &copied) && copied == size;
}

inline bool target_matches() noexcept {
  constexpr char expected_hex[] =
      "0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f";
  const auto* base = reinterpret_cast<const std::uint8_t*>(GetModuleHandleW(nullptr));
  if (reinterpret_cast<std::uintptr_t>(base) != 0x00400000u ||
      !readable(base, 4096, false, true) || !matches_gothic2_header(base, 4096)) return false;
  wchar_t path[32768]{};
  const auto length = GetModuleFileNameW(nullptr, path, 32768);
  if (!length || length >= 32768) return false;
  const auto file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return false;
  LARGE_INTEGER before{}, after{};
  bool valid = GetFileSizeEx(file, &before) && before.QuadPart > 0;
  Sha256 hash;
  std::array<std::uint8_t, 16384> buffer{};
  std::uint64_t total = 0;
  while (valid) {
    DWORD count = 0;
    if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr)) {
      valid = false;
      break;
    }
    if (!count) break;
    hash.update(buffer.data(), count);
    total += count;
  }
  valid = valid && GetFileSizeEx(file, &after) && before.QuadPart == after.QuadPart &&
      total == static_cast<std::uint64_t>(before.QuadPart);
  CloseHandle(file);
  constexpr char digits[] = "0123456789abcdef";
  const auto result = hash.finish();
  for (std::size_t i = 0; i < result.size(); ++i)
    valid = valid && digits[result[i] >> 4] == expected_hex[2 * i] &&
        digits[result[i] & 15] == expected_hex[2 * i + 1];
  return valid;
}

template <std::size_t N>
inline bool sibling_path(HMODULE module, wchar_t (&path)[32768],
                         const wchar_t (&name)[N]) noexcept {
  const auto length = GetModuleFileNameW(module, path, 32768);
  if (!length || length >= 32768) return false;
  DWORD directory = length;
  while (directory && path[directory - 1] != L'\\' && path[directory - 1] != L'/')
    --directory;
  if (!directory || N > 32768 - directory) return false;
  std::memcpy(path + directory, name, sizeof(name));
  return true;
}

template <std::size_t N, std::size_t M>
inline bool consume_marker(HMODULE module, const wchar_t (&name)[N],
                           const char (&expected)[M]) noexcept {
  wchar_t path[32768]{};
  if (!sibling_path(module, path, name)) return false;
  const auto attributes = GetFileAttributesW(path);
  if (attributes == INVALID_FILE_ATTRIBUTES ||
      (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) return false;
  const auto file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
  if (file == INVALID_HANDLE_VALUE) return false;
  LARGE_INTEGER size{};
  std::array<char, M> bytes{};
  DWORD count = 0;
  const bool exact = GetFileSizeEx(file, &size) && size.QuadPart == M - 1 &&
      ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &count, nullptr) &&
      count == M - 1 && std::memcmp(bytes.data(), expected, M - 1) == 0;
  CloseHandle(file);
  return exact && DeleteFileW(path);
}

inline void jump(std::uint8_t* bytes, std::uintptr_t from, std::uintptr_t to) noexcept {
  bytes[0] = 0xe9;
  const auto offset = static_cast<std::uint32_t>(to) - static_cast<std::uint32_t>(from + 5);
  std::memcpy(bytes + 1, &offset, sizeof(offset));
}

// Only fixed, previously disassembled prologues without relative operands.
// Site: address, patch_size, guard_size, original[32].
template <typename Site, std::size_t N>
class Hooks {
public:
  explicit Hooks(const std::array<Site, N>& sites) noexcept : sites_(sites) {}
  void* original(std::size_t index) const noexcept { return prepared_[index].trampoline; }

  bool prepare(const std::array<std::uintptr_t, N>& destinations) noexcept {
    if (prepare_attempted_) return false;
    prepare_attempted_ = true;
    if (!matches()) return false;
    for (std::size_t i = 0; i < N; ++i) {
      const auto& site = sites_[i];
      auto& hook = prepared_[i];
      if (site.patch_size < 5 || site.patch_size > hook.patch.size() ||
          site.guard_size < site.patch_size || site.guard_size > site.original.size()) {
        free_all(); return false;
      }
      hook.trampoline = VirtualAlloc(nullptr, site.patch_size + 5,
                                     MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      if (!hook.trampoline) { free_all(); return false; }
      auto* code = static_cast<std::uint8_t*>(hook.trampoline);
      std::memcpy(code, site.original.data(), site.patch_size);
      jump(code + site.patch_size, reinterpret_cast<std::uintptr_t>(code) + site.patch_size,
           site.address + site.patch_size);
      DWORD old = 0;
      if (!VirtualProtect(code, site.patch_size + 5, PAGE_EXECUTE_READ, &old) ||
          !FlushInstructionCache(GetCurrentProcess(), code, site.patch_size + 5)) {
        free_all(); return false;
      }
      hook.patch.fill(0x90);
      jump(hook.patch.data(), site.address, destinations[i]);
    }
    return true;
  }

  bool install(std::uintptr_t own_code) noexcept {
    if (install_attempted_ || !prepare_attempted_) return false;
    install_attempted_ = true;
    HMODULE pinned = nullptr;
    if (!matches() || !GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(own_code), &pinned)) { free_all(); return false; }
    for (std::size_t i = 0; i < N; ++i) {
      const auto& site = sites_[i];
      auto& hook = prepared_[i];
      auto* entry = reinterpret_cast<void*>(site.address);
      if (!hook.trampoline || !matches() ||
          !VirtualProtect(entry, site.patch_size, PAGE_EXECUTE_READWRITE, &hook.protection)) {
        rollback(); return false;
      }
      std::memcpy(entry, hook.patch.data(), site.patch_size);
      hook.written = true;
      DWORD ignored = 0;
      const bool protected_again = VirtualProtect(entry, site.patch_size, hook.protection, &ignored);
      const bool flushed = FlushInstructionCache(GetCurrentProcess(), entry, site.patch_size);
      if (!protected_again || !flushed) { rollback(); return false; }
    }
    if (!matches()) { rollback(); return false; }
    return true;
  }

private:
  struct Prepared {
    void* trampoline = nullptr;
    std::array<std::uint8_t, 32> patch{};
    DWORD protection = 0;
    bool written = false;
  };
  const std::array<Site, N>& sites_;
  std::array<Prepared, N> prepared_{};
  bool prepare_attempted_ = false;
  bool install_attempted_ = false;

  bool matches_one(std::size_t i) const noexcept {
    const auto& site = sites_[i];
    const auto& hook = prepared_[i];
    const auto* address = reinterpret_cast<const std::uint8_t*>(site.address);
    if (site.patch_size > site.guard_size || site.guard_size > site.original.size() ||
        !readable(address, site.guard_size, true, true)) return false;
    return hook.written ?
        std::memcmp(address, hook.patch.data(), site.patch_size) == 0 &&
        std::memcmp(address + site.patch_size, site.original.data() + site.patch_size,
                    site.guard_size - site.patch_size) == 0 :
        std::memcmp(address, site.original.data(), site.guard_size) == 0;
  }
  bool matches() const noexcept {
    for (std::size_t i = 0; i < N; ++i) if (!matches_one(i)) return false;
    return true;
  }
  void free_all() noexcept {
    for (auto& hook : prepared_) {
      if (hook.trampoline) VirtualFree(hook.trampoline, 0, MEM_RELEASE);
      hook.trampoline = nullptr;
    }
  }
  void rollback() noexcept {
    bool restored = true;
    for (std::size_t i = N; i-- > 0;) {
      auto& hook = prepared_[i];
      if (!hook.written) continue;
      const auto& site = sites_[i];
      auto* entry = reinterpret_cast<void*>(site.address);
      DWORD old = 0;
      if (!matches_one(i) || !VirtualProtect(entry, site.patch_size, PAGE_EXECUTE_READWRITE, &old)) {
        restored = false; continue;
      }
      std::memcpy(entry, site.original.data(), site.patch_size);
      hook.written = false;
      DWORD ignored = 0;
      const bool protected_again = VirtualProtect(entry, site.patch_size, hook.protection, &ignored);
      const bool flushed = FlushInstructionCache(GetCurrentProcess(), entry, site.patch_size);
      if (!protected_again || !flushed) restored = false;
    }
    // If teardown is incomplete, the pinned module and trampolines remain
    // valid. The wrappers forward only the original without release.
    if (restored) free_all();
  }
};

}  // namespace ubc::windows_probe
