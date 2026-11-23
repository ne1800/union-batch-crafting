// SPDX-License-Identifier: MIT
#include "ubc/ui/crafting_preview_cache.hpp"
#include "ubc/core/crafting_session.hpp"
#define UBC_CRAFTING_FIXTURE_ONLY
#include "../catalogs/crafting_catalog_tests.cpp"
#include <memory>

namespace {
using namespace crafting_test;
using Phase = ubc::SelectionPhase;

struct Inputs {
  std::array<std::uint8_t, 8192> bytes{};
  unsigned reads = 0;
  bool readable = true, module_current = true, lose_module_on_read = false, sparse = false;

  ubc::ReadOnlyMemory memory() noexcept {
    return {this, [](void* ctx, std::uint32_t address, void* out, std::size_t size) noexcept {
              auto& self = *static_cast<Inputs*>(ctx);
              ++self.reads;
              if (self.lose_module_on_read)
                self.module_current = false;
              if (!self.readable || (self.sparse && size > 4) || address > self.bytes.size() ||
                  size > self.bytes.size() - address)
                return false;
              std::memcpy(out, self.bytes.data() + address, size);
              return true;
            }};
  }
  static bool module(std::uintptr_t address, std::uintptr_t) noexcept {
    return reinterpret_cast<Inputs*>(address)->module_current;
  }
};

struct Preview {
  Inputs inputs;
  std::unique_ptr<ubc::CraftingPreviewCache> cache = std::make_unique<ubc::CraftingPreviewCache>();
  ubc::CraftingPreviewKey key{crafting_test::key(Kind::grind), 3, 4, 0x9001, 2};

  void capture(bool available = true) {
    const auto source = cache->begin(inputs.memory());
    std::uint32_t value = 0;
    // A list link, amount at the same item address, overlapping hand fields,
    // script knowledge and the last byte of a cross-page code window.
    for (const auto address : {32u, 4094u, 64u, 66u, 256u, 64u})
      check(source.copy(address, &value, sizeof(value)), "each original read passes through");
    cache->observe(Inputs::module, reinterpret_cast<std::uintptr_t>(&inputs), 0,
                   inputs.module_current);
    cache->finish(key, available);
  }
};

void dependencies() {
  Preview p;
  p.capture(false);
  ++p.inputs.bytes[128];
  p.inputs.reads = 0;
  check(p.cache->reuse(Phase::selecting, p.key, p.inputs.memory()) &&
            !p.cache->quantity_available() && p.inputs.reads == 2,
        "reuse checks both pages, ignores gaps and preserves a blocked quantity");
  for (const auto address : {32u, 64u, 69u, 256u, 4097u}) {
    p.capture();
    ++p.inputs.bytes[address];
    check(!p.cache->reuse(Phase::selecting, p.key, p.inputs.memory()),
          "a change at the same address invalidates the display");
  }
  p.capture();
  p.inputs.module_current = false;
  check(!p.cache->reuse(Phase::selecting, p.key, p.inputs.memory()),
        "a changed module binding invalidates identical memory");
  p.inputs.module_current = true;
  p.capture();
  p.inputs.lose_module_on_read = true;
  check(!p.cache->reuse(Phase::selecting, p.key, p.inputs.memory()),
        "a module change during the byte comparison also invalidates the preview");
  p.inputs.lose_module_on_read = false;
  p.inputs.sparse = true;
  p.capture();
  check(p.cache->reuse(Phase::selecting, p.key, p.inputs.memory()),
        "unobserved page gaps do not prevent comparing every original span");
  p.capture();
  p.inputs.readable = false;
  check(!p.cache->reuse(Phase::selecting, p.key, p.inputs.memory()),
        "an inaccessible page cannot leave a positive quantity");
  auto source = p.cache->begin(p.inputs.memory());
  std::uint32_t value = 0;
  check(!source.copy(32, &value, 4), "failed source reads remain failures");
  p.inputs.readable = true;
  check(source.copy(64, &value, 4), "later source reads still pass through");
  p.cache->finish(p.key, true);
  check(!p.cache->reuse(Phase::selecting, p.key, p.inputs.memory()),
        "partially readable evaluation cannot seed a cache");
  source = p.cache->begin(p.inputs.memory());
  source.copy(64, &value, 4);
  ++p.inputs.bytes[64];
  source.copy(64, &value, 4);
  p.cache->finish(p.key, true);
  check(!p.cache->reuse(Phase::selecting, p.key, p.inputs.memory()),
        "contradictory reads cannot seed a cache");
}

void capacity() {
  auto cache = std::make_unique<ubc::CraftingPreviewCache>();
  const ubc::ReadOnlyMemory zeros{nullptr,
                                  [](void*, std::uint32_t, void* out, std::size_t size) noexcept {
                                    std::memset(out, 0, size);
                                    return true;
                                  }};
  const ubc::CraftingPreviewKey key{crafting_test::key(Kind::alchemy)};
  const auto source = cache->begin(zeros);
  std::uint8_t value = 1;
  bool readable = true;
  for (std::uint32_t i = 1; i <= 65536; ++i)
    readable = source.copy(i, &value, 1) && readable;
  cache->finish(key, true);
  check(readable && !value && !cache->reuse(Phase::selecting, key, zeros),
        "exhausting optional display storage preserves every full read and disables reuse");
}

void changed_keys() {
  Preview p;
  const auto misses = [&](auto change) {
    auto other = p.key;
    change(other);
    p.capture();
    check(!p.cache->reuse(Phase::selecting, other, p.inputs.memory()),
          "changing an input invalidates the preview");
  };
  misses([](auto& k) { ++k.selection; });
  misses([](auto& k) { ++k.quantity; });
  misses([](auto& k) { ++k.world_epoch; });
  misses([](auto& k) { ++k.collection; });
  misses([](auto& k) { ++k.owner.generation; });
  misses([](auto& k) { ++k.owner.thread; });
  misses([](auto& k) { ++k.owner.world; });
  misses([](auto& k) { ++k.owner.hero; });
  misses([](auto& k) { ++k.owner.station; });
  misses([](auto& k) { ++k.owner.manager; });
  misses([](auto& k) { k.owner.kind = Kind::bow; });
  misses([](auto& k) { ++k.dialog.view; });
  misses([](auto& k) { ++k.dialog.info; });
  misses([](auto& k) { ++k.dialog.mode; });
  misses([](auto& k) { ++k.dialog.selected; });
  misses([](auto& k) { k.dialog.hidden = !k.dialog.hidden; });
  misses([](auto& k) { ++k.rows; });
  misses([](auto& k) { k.rows_valid = !k.rows_valid; });
}

void confirmation() {
  for (const auto kind : {Kind::alchemy, Kind::bow, Kind::cool, Kind::grind}) {
    for (const bool changed_after_close : {false, true}) {
      Preview p;
      ubc::CraftingSession session;
      auto live = example(kind);
      std::uint64_t frame = 0;
      check(session.open(live), "fresh menu opens");
      neutral(session, frame);
      p.key.owner = live.key;
      p.key.selection = session.selected()->id;
      p.key.quantity = session.quantity();
      p.capture();
      check(p.cache->reuse(session.phase(), p.key, p.inputs.memory()) && session.confirm(),
            "unchanged selection can request confirmation");
      check(
          p.cache->reuse(session.phase(), p.key, p.inputs.memory()),
          "a held confirm button can keep showing the verified preview while waiting for release");
      neutral(session, frame);
      p.capture(); // Even an identical, populated cache must miss at the final boundary.
      check(session.phase() == Phase::ready &&
                !p.cache->reuse(session.phase(), p.key, p.inputs.memory()),
            "ready never takes the display shortcut");
      if (changed_after_close)
        state(live, p.key.selection).contract = false;
      check(session.take(live, true).has_value() == !changed_after_close,
            "a change in the restored-view capture rejects a previously confirmable order");
    }
  }
}
} // namespace

int main() {
  dependencies();
  capacity();
  changed_keys();
  confirmation();
  std::printf("crafting_preview_cache_tests: %u checks, %u failures\n", crafting_test::checks,
              crafting_test::failures);
  return crafting_test::failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
