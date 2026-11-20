// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/engine/windows_probe.hpp"
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/engine/read_only_memory.hpp"
#include <cwchar>

namespace ubc::windows_probe {
// Only use after DllMain on the game thread. The loader provides the file
// path of the actually loaded module. A dedicated loader reference and
// a read-only shared file handle keep this mapping fixed until process
// end. No release is derived from a found CALL/JMP or a file name. The
// respective compatibility contract re-checks critical RAM regions.
class LoadedModuleIdentity {
public:
  LoadedModuleIdentity() noexcept = default;
  LoadedModuleIdentity(const LoadedModuleIdentity&) = delete;
  LoadedModuleIdentity& operator=(const LoadedModuleIdentity&) = delete;
  bool acquire(const wchar_t* name, std::uint32_t image_size, const lhiver::Digest& expected,
               bool retain_untested = false) noexcept {
    if (attempted_)
      return false;
    attempted_ = true;
    owner_ = GetCurrentThreadId();
    if (!name || !*name || !image_size || image_size > 64u * 1024u * 1024u)
      return false;
    HMODULE loaded = nullptr;
    if (!GetModuleHandleExW(0, name, &loaded))
      return false;
    module_ = loaded;
    const auto length = GetModuleFileNameW(module_, path_.data(), static_cast<DWORD>(path_.size()));
    bool good = length && length < path_.size();
    const wchar_t* base = path_.data();
    if (good) {
      for (const auto* p = base; *p; ++p)
        if (*p == L'\\' || *p == L'/')
          base = p + 1;
      good = _wcsicmp(base, name) == 0;
    }
    if (good)
      file_ = CreateFileW(path_.data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, nullptr);
    image_size_ = mapped_size();
    good = good && file_ != INVALID_HANDLE_VALUE && image_size_;
    LARGE_INTEGER before{}, after{};
    good = good && GetFileSizeEx(file_, &before) && before.QuadPart > 0 &&
           before.QuadPart <= 64 * 1024 * 1024;
    Sha256 hash;
    std::array<std::uint8_t, 16384> bytes{};
    std::uint64_t total = 0;
    while (good) {
      DWORD count = 0;
      if (!ReadFile(file_, bytes.data(), static_cast<DWORD>(bytes.size()), &count, nullptr)) {
        good = false;
        break;
      }
      if (!count)
        break;
      hash.update(bytes.data(), count);
      total += count;
      if (total > 64u * 1024u * 1024u) {
        good = false;
        break;
      }
    }
    good = good && GetFileSizeEx(file_, &after) && before.QuadPart == after.QuadPart &&
           total == static_cast<std::uint64_t>(before.QuadPart);
    if (good) {
      digest_ = hash.finish();
      tested_ = digest_ == expected && image_size_ == image_size;
      valid_ = tested_ || retain_untested;
      good = observed();
    }
    if (!good) {
      valid_ = false;
      if (file_ != INVALID_HANDLE_VALUE)
        CloseHandle(file_);
      file_ = INVALID_HANDLE_VALUE;
      FreeLibrary(module_);
      module_ = nullptr;
    }
    return good;
  }
  bool current() const noexcept { return tested_ && observed(); }
  static bool executable_page(std::uint32_t base, std::uint32_t target) noexcept {
    MEMORY_BASIC_INFORMATION page{};
    if (VirtualQuery(reinterpret_cast<void*>(target), &page, sizeof(page)) != sizeof(page) ||
        page.AllocationBase != reinterpret_cast<void*>(base) || page.Type != MEM_IMAGE ||
        page.State != MEM_COMMIT || (page.Protect & PAGE_GUARD))
      return false;
    const auto access = page.Protect & 0xff;
    return access == PAGE_EXECUTE || access == PAGE_EXECUTE_READ ||
           access == PAGE_EXECUTE_READWRITE || access == PAGE_EXECUTE_WRITECOPY;
  }
  bool observed() const noexcept {
    if (!valid_ || !module_ || file_ == INVALID_HANDLE_VALUE || owner_ != GetCurrentThreadId() ||
        mapped_size() != image_size_)
      return false;
    const auto length =
        GetModuleFileNameW(module_, scratch_.data(), static_cast<DWORD>(scratch_.size()));
    return length && length < scratch_.size() && std::wcscmp(scratch_.data(), path_.data()) == 0;
  }
  std::uint32_t base() const noexcept {
    return valid_ ? static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(module_)) : 0;
  }
  std::uint32_t image_size() const noexcept { return valid_ ? image_size_ : 0; }
  lhiver::Digest digest() const noexcept { return valid_ ? digest_ : lhiver::Digest{}; }

private:
  bool attempted_ = false, valid_ = false, tested_ = false;
  HMODULE module_ = nullptr;
  HANDLE file_ = INVALID_HANDLE_VALUE;
  DWORD owner_ = 0;
  std::uint32_t image_size_ = 0;
  lhiver::Digest digest_{};
  std::array<wchar_t, 32768> path_{};
  mutable std::array<wchar_t, 32768> scratch_{};
  std::uint32_t mapped_size() const noexcept {
    const auto start = reinterpret_cast<std::uintptr_t>(module_);
    if (!start || start > UINT32_MAX - 4096)
      return 0;
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(module_, &info, sizeof(info)) != sizeof(info) ||
        info.AllocationBase != module_ || info.State != MEM_COMMIT || info.Type != MEM_IMAGE)
      return false;
    std::array<std::uint8_t, 64> dos{};
    std::array<std::uint8_t, 84> nt{};
    const ReadOnlyMemory memory{nullptr, copy_memory};
    if (!memory.copy(static_cast<std::uint32_t>(start), dos.data(), dos.size()) || dos[0] != 'M' ||
        dos[1] != 'Z')
      return false;
    const auto word = [](const std::uint8_t* p) {
      std::uint32_t v = 0;
      std::memcpy(&v, p, 4);
      return v;
    };
    const auto offset = word(dos.data() + 60);
    if (offset < 64 || offset > 4096 - nt.size() ||
        !memory.copy(static_cast<std::uint32_t>(start) + offset, nt.data(), nt.size()) ||
        word(nt.data()) != 0x4550)
      return 0;
    const auto size = word(nt.data() + 80);
    return size >= offset + nt.size() && size <= UINT32_MAX - start ? size : 0;
  }
};
} // namespace ubc::windows_probe
