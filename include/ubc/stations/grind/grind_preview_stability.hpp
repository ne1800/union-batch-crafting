// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/ui/crafting_preview_cache.hpp"

namespace ubc {
// This gate can only postpone a fresh full evaluation. Its samples can never
// supply display data or authorize a quantity. Read both the changing span and
// the allocator head because one can settle while the other is still moving.
class GrindPreviewStability {
public:
  using Miss = CraftingPreviewCache::Miss;
  static constexpr unsigned quiet_frames = 6;

  bool active() const noexcept { return active_; }
  void reset() noexcept { active_ = false; }
  const Miss& reason() const noexcept { return reason_; }

  bool start(const Miss& miss, const CraftingKey& owner, const ReadOnlyMemory& live,
             std::uint32_t pool_head, std::uint64_t frame, std::uint64_t serial) noexcept {
    reset();
    if (std::strcmp(miss.reason, "bytes") || !miss.address || !miss.length ||
        miss.length > bytes_.size())
      return false;
    reason_ = miss;
    owner_ = owner;
    last_change_ = frame;
    serial_ = serial;
    active_ = live.copy(static_cast<std::uint32_t>(miss.address), bytes_.data(), miss.length) &&
              live.copy(pool_head, &pool_, sizeof(pool_));
    return active_;
  }

  bool waiting(const CraftingKey& owner, const ReadOnlyMemory& live, std::uint32_t pool_head,
               std::uint64_t frame, std::uint64_t serial) noexcept {
    if (!active_)
      return false;
    std::array<std::uint8_t, 4096> current;
    std::uint32_t pool = 0;
    if (!(owner == owner_) || frame < last_change_ ||
        !live.copy(static_cast<std::uint32_t>(reason_.address), current.data(), reason_.length) ||
        !live.copy(pool_head, &pool, sizeof(pool))) {
      reset();
      return false;
    }
    if (serial != serial_ || pool != pool_ ||
        std::memcmp(current.data(), bytes_.data(), reason_.length)) {
      std::memcpy(bytes_.data(), current.data(), reason_.length);
      pool_ = pool;
      serial_ = serial;
      last_change_ = frame;
    }
    active_ = frame - last_change_ < quiet_frames;
    return active_;
  }

private:
  std::array<std::uint8_t, 4096> bytes_{};
  CraftingKey owner_{};
  Miss reason_{};
  std::uint32_t pool_ = 0;
  std::uint64_t last_change_ = 0, serial_ = 0;
  bool active_ = false;
};
} // namespace ubc
