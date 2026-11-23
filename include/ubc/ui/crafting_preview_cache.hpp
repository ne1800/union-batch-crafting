// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/catalogs/crafting_catalog.hpp"
#include "ubc/core/selection_session.hpp"
#include "ubc/core/selection_snapshot.hpp"
#include "ubc/engine/read_only_memory.hpp"
#include <algorithm>
#include <array>
#include <cstring>

namespace ubc {
struct CraftingPreviewKey {
  CraftingKey owner{};
  std::uint64_t world_epoch = 0, collection = 0;
  crafting_catalog::RecipeId selection = UINT32_MAX;
  std::uint32_t quantity = 0;
  SelectionDialog dialog{};
  std::size_t rows = 0;
  bool rows_valid = false;

  bool matches_dialog(const SelectionDialog& b) const noexcept {
    return dialog.view == b.view && dialog.npc == b.npc && dialog.player == b.player &&
           dialog.info == b.info && dialog.mode == b.mode && dialog.selected == b.selected &&
           dialog.count == b.count && dialog.valid == b.valid && dialog.waiting == b.waiting &&
           dialog.open == b.open && dialog.hidden == b.hidden && dialog.done == b.done &&
           dialog.rejected_info_transition == b.rejected_info_transition;
  }
  bool operator==(const CraftingPreviewKey& b) const noexcept {
    return owner == b.owner && world_epoch == b.world_epoch && collection == b.collection &&
           selection == b.selection && quantity == b.quantity && rows == b.rows &&
           rows_valid == b.rows_valid && matches_dialog(b.dialog);
  }
};

// The session already owns its display snapshot. Remember exactly which live
// bytes it depended on, including list links, stack amounts, script and code.
// Recording always passes through to the original reader, even on overflow.
// This object never supplies cached bytes to a reader or a crafting request.
class CraftingPreviewCache {
public:
  using GrindUnits = std::array<bool, grind_recipe::recipes.size()>;
  using Probe = bool (*)(std::uintptr_t, std::uintptr_t) noexcept;

  ReadOnlyMemory begin(const ReadOnlyMemory& source) noexcept {
    invalidate();
    source_ = source;
    recording_ = true;
    count_ = used_ = probe_count_ = 0;
    index_.fill(0);
    return {this, copy};
  }

  void finish(const CraftingPreviewKey& key, bool quantity_available,
              const GrindUnits& units = {}) noexcept {
    if (recording_ && count_) {
      std::sort(spans_.begin(), spans_.begin() + count_,
                [](const Span& a, const Span& b) { return a.address < b.address; });
      key_ = key;
      original_available_ = available_ = quantity_available;
      units_ = units;
      ready_ = true;
    }
    recording_ = false;
    source_ = {};
  }

  void invalidate() noexcept {
    ready_ = recording_ = false;
    source_ = {};
  }

  bool quantity_available() const noexcept { return ready_ && available_; }
  static bool allows(SelectionPhase phase) noexcept {
    return phase == SelectionPhase::selecting || phase == SelectionPhase::releasing;
  }

  // Borrowing the original dialog hides it after a successful first render.
  // Record just that expected presentation transition. All recorded bytes and
  // module probes must still pass reuse() before any quantity can be shown.
  bool bind_open_grind(const CraftingPreviewKey& opened) noexcept {
    auto borrowed = key_;
    if (!ready_ || !key_.dialog.open || key_.dialog.hidden || opened.dialog.open ||
        !opened.dialog.hidden || !unit_verified(opened))
      return false;
    borrowed.dialog.open = false;
    borrowed.dialog.hidden = true;
    borrowed.selection = opened.selection;
    borrowed.quantity = opened.quantity;
    if (!(borrowed == opened))
      return false;
    key_ = opened;
    original_available_ = available_ = true;
    return true;
  }

  // Module leases, executable-page protections and adapter state have inputs
  // outside ReadOnlyMemory. Recheck each distinct query once on a cache hit.
  void observe(Probe read, std::uintptr_t a, std::uintptr_t b, bool result) noexcept {
    if (!recording_)
      return;
    for (std::size_t i = 0; i < probe_count_; ++i) {
      const auto& p = probes_[i];
      if (p.read == read && p.a == a && p.b == b) {
        recording_ = p.result == result;
        return;
      }
    }
    if (probe_count_ == probes_.size())
      recording_ = false;
    else
      probes_[probe_count_++] = {read, a, b, result};
  }

  bool reuse(SelectionPhase phase, const CraftingPreviewKey& key,
             const ReadOnlyMemory& live) noexcept {
    if (allows(phase) && ready_) {
      auto bound = key_;
      const bool other_unit = key.selection != key_.selection || key.quantity != key_.quantity;
      if (other_unit && unit_verified(key)) {
        bound.selection = key.selection;
        bound.quantity = 1;
      }
      if (key == bound && unchanged(live)) {
        available_ = other_unit || original_available_;
        return true;
      }
    }
    invalidate();
    return false;
  }

private:
  bool unit_verified(const CraftingPreviewKey& key) const noexcept {
    if (key.owner.kind != crafting_catalog::Station::grind || key.quantity != 1)
      return false;
    for (std::size_t i = 0; i < units_.size(); ++i)
      if (crafting_catalog::recipes[crafting_catalog::grind_begin + i].id == key.selection)
        return units_[i];
    return false;
  }
  struct Span {
    std::uint32_t address, length, offset;
  };
  struct ProbeResult {
    Probe read;
    std::uintptr_t a, b;
    bool result;
  };
  // Keep this optional display optimization bounded even with fragmented
  // inventories. A full table simply leaves the original frame path in use.
  static constexpr std::size_t page_size = 4096, max_spans = 32768;
  std::array<Span, max_spans> spans_{};
  std::array<std::uint16_t, max_spans * 2> index_{};
  std::array<std::uint8_t, 512 * 1024> bytes_{};
  std::array<ProbeResult, 64> probes_{};
  std::size_t count_ = 0, used_ = 0, probe_count_ = 0;
  ReadOnlyMemory source_{};
  CraftingPreviewKey key_{};
  GrindUnits units_{};
  bool ready_ = false, recording_ = false, available_ = false, original_available_ = false;

  static bool copy(void* raw, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& self = *static_cast<CraftingPreviewCache*>(raw);
    if (!self.source_.copy(address, output, size)) {
      self.recording_ = false;
      return false;
    }
    auto* data = static_cast<const std::uint8_t*>(output);
    while (self.recording_ && size) {
      const auto length = std::min(size, page_size - address % page_size);
      self.recording_ = self.remember(address, data, length);
      size -= length;
      address += static_cast<std::uint32_t>(length);
      data += length;
    }
    return true;
  }

  bool remember(std::uint32_t address, const std::uint8_t* data, std::size_t size) noexcept {
    auto hash = address ^ (static_cast<std::uint32_t>(size) * 0x9e3779b9u);
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    auto bucket = (hash ^ (hash >> 15)) % index_.size();
    while (index_[bucket]) {
      const auto& span = spans_[index_[bucket] - 1u];
      if (span.address == address && span.length == size)
        return std::memcmp(bytes_.data() + span.offset, data, size) == 0;
      bucket = (bucket + 1) % index_.size();
    }
    if (count_ == max_spans || size > bytes_.size() - used_)
      return false;
    spans_[count_] = {address, static_cast<std::uint32_t>(size), static_cast<std::uint32_t>(used_)};
    index_[bucket] = static_cast<std::uint16_t>(++count_);
    std::memcpy(bytes_.data() + used_, data, size);
    used_ += size;
    return true;
  }

  bool probes_current() const noexcept {
    for (std::size_t i = 0; i < probe_count_; ++i) {
      const auto& p = probes_[i];
      if (p.read(p.a, p.b) != p.result)
        return false;
    }
    return true;
  }
  bool unchanged(const ReadOnlyMemory& live) const noexcept {
    if (!probes_current())
      return false;
    std::array<std::uint8_t, page_size> scratch;
    for (std::size_t first = 0; first < count_;) {
      const auto start = spans_[first].address;
      const auto page = start / page_size;
      std::size_t next = first, length = 0;
      do {
        const auto& span = spans_[next++];
        length = std::max(length, std::size_t{span.address - start} + span.length);
      } while (next < count_ && spans_[next].address / page_size == page);
      // One protected process read per touched page. Unused bytes between
      // spans can change freely and never enter the comparison.
      const bool grouped = live.copy(start, scratch.data(), length);
      for (; first < next; ++first) {
        const auto& span = spans_[first];
        // A sparse reader may reject a gap. Every originally observed span
        // must still be readable and equal in its entirety.
        auto* actual = scratch.data() + (span.address - start);
        if ((!grouped && !live.copy(span.address, actual, span.length)) ||
            std::memcmp(actual, bytes_.data() + span.offset, span.length))
          return false;
      }
    }
    return probes_current();
  }
};
} // namespace ubc
