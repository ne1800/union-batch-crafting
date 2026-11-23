// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/engine/windows_module_identity.hpp"
#include "ubc/ui/crafting_preview_cache.hpp"

namespace ubc_native {
inline constexpr ubc::ReadOnlyMemory live_memory{nullptr, ubc::windows_probe::copy_memory};

// The menu reads the current process on the bound game thread. Check the
// entire original range, including guard pages, on each real read. SEH
// covers a mapping that disappears between VirtualQuery and the exact copy.
inline bool copy_grind_display(void*, std::uint32_t address, void* out, std::size_t size) noexcept {
  const auto* source = reinterpret_cast<const void*>(address);
  if (!ubc::windows_probe::readable(source, size))
    return false;
  __try {
    std::memcpy(out, source, size);
    return true;
  } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
                      GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ||
                      GetExceptionCode() == EXCEPTION_GUARD_PAGE
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH) {
    return false;
  }
}
inline constexpr ubc::ReadOnlyMemory grind_display_memory{nullptr, copy_grind_display};

// Only reads made inside the menu's cold preview register dependencies. Even
// there, callbacks and readers receive the real bytes and real probe results.
struct PreviewRecording {
  ubc::CraftingPreviewCache& cache;
  const ubc::CraftingPreviewKey& key;
  ubc::ReadOnlyMemory reads;
  const char* dialog_change = nullptr;
  inline static thread_local PreviewRecording* active = nullptr;

  PreviewRecording(ubc::CraftingPreviewCache& target,
                   const ubc::CraftingPreviewKey& inputs) noexcept
      : cache(target), key(inputs),
        reads(cache.begin(inputs.owner.kind == ubc::crafting_catalog::Station::grind
                              ? grind_display_memory
                              : live_memory)) {
    active = this;
  }
  ~PreviewRecording() { active = nullptr; }
  PreviewRecording(const PreviewRecording&) = delete;
  PreviewRecording& operator=(const PreviewRecording&) = delete;

  static bool copy(void*, std::uint32_t address, void* output, std::size_t size) noexcept {
    return (active ? active->reads : live_memory).copy(address, output, size);
  }
  static void observe_dialog(const ubc::SelectionDialog& dialog) noexcept {
    if (active && !active->dialog_change)
      active->dialog_change = active->key.dialog_difference(dialog);
  }
  static bool probe(ubc::CraftingPreviewCache::Probe read, std::uintptr_t a = 0,
                    std::uintptr_t b = 0, const char* name = "adapter") noexcept {
    const bool result = read(a, b);
    if (active)
      active->cache.observe(read, a, b, result, name);
    return result;
  }
};
inline constexpr ubc::ReadOnlyMemory memory{
    nullptr, PreviewRecording::copy, []() noexcept {
      const auto* recording = PreviewRecording::active;
      return recording && recording->key.owner.kind == ubc::crafting_catalog::Station::grind;
    }};

// All crafting module identities live for the lifetime of the adapter. Their
// file hashes and base addresses are pinned by acquire(). The observed mapping
// and the executable protection of a particular target can still change.
class CraftingModule : public ubc::windows_probe::LoadedModuleIdentity {
  using Identity = ubc::windows_probe::LoadedModuleIdentity;

public:
  bool current() const noexcept {
    return PreviewRecording::probe(
        [](std::uintptr_t p, std::uintptr_t) noexcept {
          return reinterpret_cast<const Identity*>(p)->current();
        },
        reinterpret_cast<std::uintptr_t>(static_cast<const Identity*>(this)), 0, "module.current");
  }
  bool observed() const noexcept {
    return PreviewRecording::probe(
        [](std::uintptr_t p, std::uintptr_t) noexcept {
          return reinterpret_cast<const Identity*>(p)->observed();
        },
        reinterpret_cast<std::uintptr_t>(static_cast<const Identity*>(this)), 0, "module.observed");
  }
  static bool executable_page(std::uint32_t base, std::uint32_t address) noexcept {
    return PreviewRecording::probe(
        [](std::uintptr_t module_base, std::uintptr_t target) noexcept {
          return Identity::executable_page(static_cast<std::uint32_t>(module_base),
                                           static_cast<std::uint32_t>(target));
        },
        base, address, "module.executable_page");
  }
};
} // namespace ubc_native
