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

  bool operator==(const CraftingPreviewKey& b) const noexcept {
    return owner == b.owner && world_epoch == b.world_epoch && collection == b.collection &&
           selection == b.selection && quantity == b.quantity && rows == b.rows &&
           rows_valid == b.rows_valid && dialog.view == b.dialog.view &&
           dialog.npc == b.dialog.npc && dialog.player == b.dialog.player &&
           dialog.info == b.dialog.info && dialog.mode == b.dialog.mode &&
           dialog.selected == b.dialog.selected && dialog.count == b.dialog.count &&
           dialog.valid == b.dialog.valid && dialog.waiting == b.dialog.waiting &&
           dialog.open == b.dialog.open && dialog.hidden == b.dialog.hidden &&
           dialog.done == b.dialog.done &&
           dialog.rejected_info_transition == b.dialog.rejected_info_transition;
  }
};

// The session already owns its display snapshot. Remember exactly which live
// bytes it depended on, including list links, stack amounts, script and code.
// Recording always passes through to the original reader, even on overflow.
// This object never supplies cached bytes to a reader or a crafting request.
class CraftingPreviewCache {
public:
  using Probe = bool (*)(std::uintptr_t, std::uintptr_t) noexcept;

  ReadOnlyMemory begin(const ReadOnlyMemory& source) noexcept {
    invalidate();
    source_ = source;
    recording_ = true;
    count_ = used_ = probe_count_ = 0;
    index_.fill(0);
    return {this, copy};
  }

  void finish(const CraftingPreviewKey& key, bool quantity_available) noexcept {
    if (recording_ && count_) {
      std::sort(spans_.begin(), spans_.begin() + count_,
                [](const Span& a, const Span& b) { return a.address < b.address; });
      key_ = key;
      available_ = quantity_available;
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
    if (phase == SelectionPhase::selecting && ready_ && key == key_ && unchanged(live))
      return true;
    invalidate();
    return false;
  }

private:
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
  bool ready_ = false, recording_ = false, available_ = false;

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

  bool unchanged(const ReadOnlyMemory& live) const noexcept {
    for (std::size_t i = 0; i < probe_count_; ++i) {
      const auto& p = probes_[i];
      if (p.read(p.a, p.b) != p.result)
        return false;
    }
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
      if (!live.copy(start, scratch.data(), length))
        return false;
      for (; first < next; ++first) {
        const auto& span = spans_[first];
        if (std::memcmp(scratch.data() + (span.address - start), bytes_.data() + span.offset,
                        span.length))
          return false;
      }
    }
    return true;
  }
};
} // namespace ubc
