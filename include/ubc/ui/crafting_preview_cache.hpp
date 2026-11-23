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

  const char* dialog_difference(const SelectionDialog& b) const noexcept {
    if (dialog.view != b.view)
      return "dialog.view";
    if (dialog.npc != b.npc)
      return "dialog.npc";
    if (dialog.player != b.player)
      return "dialog.player";
    if (dialog.info != b.info)
      return "dialog.info";
    if (dialog.mode != b.mode)
      return "dialog.mode";
    if (dialog.selected != b.selected)
      return "dialog.selected";
    if (dialog.count != b.count)
      return "dialog.count";
    if (dialog.valid != b.valid)
      return "dialog.valid";
    if (dialog.waiting != b.waiting)
      return "dialog.waiting";
    if (dialog.open != b.open)
      return "dialog.open";
    if (dialog.hidden != b.hidden)
      return "dialog.hidden";
    if (dialog.done != b.done)
      return "dialog.done";
    if (dialog.rejected_info_transition != b.rejected_info_transition)
      return "dialog.rejected_info_transition";
    return nullptr;
  }
  bool matches_dialog(const SelectionDialog& b) const noexcept { return !dialog_difference(b); }
  const char* difference(const CraftingPreviewKey& b) const noexcept {
    if (owner.generation != b.owner.generation)
      return "owner.generation";
    if (owner.thread != b.owner.thread)
      return "owner.thread";
    if (owner.world != b.owner.world)
      return "owner.world";
    if (owner.hero != b.owner.hero)
      return "owner.hero";
    if (owner.station != b.owner.station)
      return "owner.station";
    if (owner.manager != b.owner.manager)
      return "owner.manager";
    if (owner.kind != b.owner.kind)
      return "owner.kind";
    if (world_epoch != b.world_epoch)
      return "world_epoch";
    if (collection != b.collection)
      return "collection";
    if (selection != b.selection)
      return "selection";
    if (quantity != b.quantity)
      return "quantity";
    if (rows != b.rows)
      return "rows";
    if (rows_valid != b.rows_valid)
      return "rows_valid";
    return dialog_difference(b.dialog);
  }
  bool operator==(const CraftingPreviewKey& b) const noexcept { return !difference(b); }
};

// The session already owns its display snapshot. Remember exactly which live
// bytes it depended on, including list links, stack amounts, script and code.
// Recording always passes through to the original reader, even on overflow.
// This object never supplies cached bytes to a reader or a crafting request.
class CraftingPreviewCache {
public:
  using GrindUnits = std::array<bool, grind_recipe::recipes.size()>;
  using Probe = bool (*)(std::uintptr_t, std::uintptr_t) noexcept;

  struct Miss {
    const char* reason = "empty";
    const char* field = "cache";
    std::uintptr_t address = 0;
    std::uint32_t length = 0, offset = 0;
    std::uint64_t before = 0, after = 0;
    std::uint32_t subject = 0;
  };

  const Miss& miss() const noexcept { return miss_; }
  void invalidate(Miss miss) noexcept {
    miss_ = miss;
    invalidate();
  }

  ReadOnlyMemory begin(const ReadOnlyMemory& source) noexcept {
    invalidate({"empty", "recording"});
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
    if (!ready_)
      return false;
    if (!key_.dialog.open || opened.dialog.open) {
      miss_ = {"key", "dialog.open"};
      return false;
    }
    if (key_.dialog.hidden || !opened.dialog.hidden) {
      miss_ = {"key", "dialog.hidden"};
      return false;
    }
    if (!unit_verified(opened)) {
      miss_ = {"capture", "opening_unit"};
      return false;
    }
    borrowed.dialog.open = false;
    borrowed.dialog.hidden = true;
    borrowed.selection = opened.selection;
    borrowed.quantity = opened.quantity;
    if (const auto* field = borrowed.difference(opened)) {
      miss_ = {"key", field};
      return false;
    }
    key_ = opened;
    original_available_ = available_ = true;
    return true;
  }

  // Module leases, executable-page protections and adapter state have inputs
  // outside ReadOnlyMemory. Recheck each distinct query once on a cache hit.
  void observe(Probe read, std::uintptr_t a, std::uintptr_t b, bool result,
               const char* name = "probe") noexcept {
    if (!recording_)
      return;
    for (std::size_t i = 0; i < probe_count_; ++i) {
      const auto& p = probes_[i];
      if (p.read == read && p.a == a && p.b == b) {
        if (p.result != result) {
          miss_ = {"probe", p.name, p.b ? p.b : p.a, 0, 0, p.result, result};
          recording_ = false;
        }
        return;
      }
    }
    if (probe_count_ == probes_.size()) {
      miss_ = {"capacity", "probes"};
      recording_ = false;
    } else
      probes_[probe_count_++] = {read, a, b, result, name};
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
      if (const auto* field = bound.difference(key))
        miss_ = {"key", field};
      else if (unchanged(live)) {
        available_ = other_unit || original_available_;
        return true;
      }
    } else if (!allows(phase))
      miss_ = {"gate", "phase"};
    invalidate();
    return false;
  }

  // A miss must refresh the session before this frame is drawn. Preserve its
  // reason across begin(), which starts recording the new dependencies.
  template <class Current, class Capture>
  bool reuse_or_capture(SelectionPhase phase, const CraftingPreviewKey& key,
                        const ReadOnlyMemory& live, const char* gate, Current current,
                        Capture capture) noexcept {
    if (gate)
      invalidate({"gate", gate});
    else if (reuse(phase, key, live) && current())
      return true;
    const auto miss = miss_;
    invalidate();
    capture(miss);
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
    const char* origin;
    std::uint32_t subject;
  };
  struct ProbeResult {
    Probe read;
    std::uintptr_t a, b;
    bool result;
    const char* name;
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
  Miss miss_{};
  bool ready_ = false, recording_ = false, available_ = false, original_available_ = false;

  static bool copy(void* raw, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& self = *static_cast<CraftingPreviewCache*>(raw);
    if (!self.source_.copy(address, output, size)) {
      if (self.recording_)
        self.miss_ = {"unreadable",
                      ReadOrigin::current,
                      address,
                      static_cast<std::uint32_t>(size),
                      0,
                      0,
                      0,
                      ReadOrigin::subject};
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
        return compare(span, data);
      bucket = (bucket + 1) % index_.size();
    }
    if (count_ == max_spans || size > bytes_.size() - used_) {
      miss_ = {"capacity", count_ == max_spans ? "spans" : "bytes", address,
               static_cast<std::uint32_t>(size)};
      return false;
    }
    spans_[count_] = {address, static_cast<std::uint32_t>(size), static_cast<std::uint32_t>(used_),
                      ReadOrigin::current, ReadOrigin::subject};
    index_[bucket] = static_cast<std::uint16_t>(++count_);
    std::memcpy(bytes_.data() + used_, data, size);
    used_ += size;
    return true;
  }

  bool compare(const Span& span, const std::uint8_t* actual) noexcept {
    const auto* expected = bytes_.data() + span.offset;
    if (std::memcmp(expected, actual, span.length) == 0)
      return true;
    const auto offset = static_cast<std::uint32_t>(
        std::mismatch(expected, expected + span.length, actual).first - expected);
    miss_ = {"bytes", span.origin,      span.address,   span.length,
             offset,  expected[offset], actual[offset], span.subject};
    return false;
  }
  bool probes_current() noexcept {
    for (std::size_t i = 0; i < probe_count_; ++i) {
      const auto& p = probes_[i];
      const bool now = p.read(p.a, p.b);
      if (now != p.result) {
        miss_ = {"probe", p.name, p.b ? p.b : p.a, 0, 0, p.result, now};
        return false;
      }
    }
    return true;
  }
  bool unchanged(const ReadOnlyMemory& live) noexcept {
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
        if (!grouped && !live.copy(span.address, actual, span.length)) {
          miss_ = {"unreadable", span.origin, span.address, span.length, 0, 0, 0, span.subject};
          return false;
        }
        if (!compare(span, actual))
          return false;
      }
    }
    return probes_current();
  }
};
} // namespace ubc
