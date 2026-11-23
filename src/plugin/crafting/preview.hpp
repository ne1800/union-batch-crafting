// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/engine/windows_module_identity.hpp"
#include "ubc/ui/crafting_preview_cache.hpp"

namespace ubc_native {
inline constexpr ubc::ReadOnlyMemory live_memory{nullptr, ubc::windows_probe::copy_memory};

// Only reads made inside the menu's cold preview register dependencies. Even
// there, callbacks and readers receive the real bytes and real probe results.
struct PreviewRecording {
  ubc::CraftingPreviewCache& cache;
  ubc::ReadOnlyMemory reads;
  inline static thread_local PreviewRecording* active = nullptr;

  explicit PreviewRecording(ubc::CraftingPreviewCache& target) noexcept
      : cache(target), reads(cache.begin(live_memory)) {
    active = this;
  }
  ~PreviewRecording() { active = nullptr; }
  PreviewRecording(const PreviewRecording&) = delete;
  PreviewRecording& operator=(const PreviewRecording&) = delete;

  static bool copy(void*, std::uint32_t address, void* output, std::size_t size) noexcept {
    return (active ? active->reads : live_memory).copy(address, output, size);
  }
  static bool probe(ubc::CraftingPreviewCache::Probe read, std::uintptr_t a = 0,
                    std::uintptr_t b = 0) noexcept {
    const bool result = read(a, b);
    if (active)
      active->cache.observe(read, a, b, result);
    return result;
  }
};
inline constexpr ubc::ReadOnlyMemory memory{nullptr, PreviewRecording::copy};

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
        reinterpret_cast<std::uintptr_t>(static_cast<const Identity*>(this)));
  }
  bool observed() const noexcept {
    return PreviewRecording::probe(
        [](std::uintptr_t p, std::uintptr_t) noexcept {
          return reinterpret_cast<const Identity*>(p)->observed();
        },
        reinterpret_cast<std::uintptr_t>(static_cast<const Identity*>(this)));
  }
  static bool executable_page(std::uint32_t base, std::uint32_t address) noexcept {
    return PreviewRecording::probe(
        [](std::uintptr_t module_base, std::uintptr_t target) noexcept {
          return Identity::executable_page(static_cast<std::uint32_t>(module_base),
                                           static_cast<std::uint32_t>(target));
        },
        base, address);
  }
};
} // namespace ubc_native
