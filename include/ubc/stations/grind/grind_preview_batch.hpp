// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/grind/crafting_grind_projection.hpp"
#include "ubc/stations/grind/grind_destroy_observation.hpp"
#include <algorithm>

// Completed preview copy. No Writer entry and no ABA evidence.
namespace ubc::grind_preview_batch {
// Fixed contract based on run 0.0.34: 10967 ranges/205195 bytes in all
// five selection paths. Reserve: 5417 ranges/56949 bytes. Tested with 4096
// additional disjoint 12-byte world nodes (15063/254347 total). No heap
// growth within the frame. Coverage is not guaranteed for every world: two
// maximum-size lists alone require 1572864 bytes. Reaching either limit
// blocks publication of the entire result and falls back to the original
// menu.
// stored_bytes sums EXACT request sizes, including overlaps. On
// abort, spans/stored_bytes cover only a prefix of the required data.
inline constexpr std::size_t max_spans = 16384, byte_capacity = 256u * 1024u, max_span_bytes = 4096;
inline constexpr std::size_t max_requests = grind_recipe::recipes.size() + 1;
// Logical work per evaluation, no millisecond/frame guarantee.
// Additionally at most max_spans*(max_spans-1)/2 overlap checks and
// at seal max_spans real reads. Every frame starts without old cache.
inline constexpr std::uint64_t max_calls_per_pass = 1000000,
                               max_bytes_per_pass = 64u * 1024u * 1024u;
static_assert(max_span_bytes <= byte_capacity && byte_capacity <= UINT32_MAX,
              "jede gespeicherte Laenge und jeder Offset passen in Span");
struct FrameObservation {
  GrindRecipeKey key{};
  std::uint64_t scope_serial = 0;
  std::uint32_t current_thread = 0, outer_depth = 0;
  bool quiet = false, writer = false;
  std::uint64_t input_serial = 0;
  bool active_scope = false;
};
struct FrameSource {
  void* context = nullptr;
  bool (*observe)(void*, FrameObservation&) noexcept = nullptr;
  // Compare a validated observation in place on hot private-copy reads.
  bool (*matches)(void*, const FrameObservation&) noexcept = nullptr;
};
enum class Issue {
  idle,
  scope,
  reentry,
  bounds,
  span_limit,
  byte_limit,
  call_limit,
  unreadable,
  overlap,
  replay_miss,
  replay_changed,
  changed,
  closed,
  request,
  catalogue,
  runtime,
  none
};
struct Statistics {
  std::uint64_t recording_calls = 0, recording_bytes = 0, replay_calls = 0, replay_bytes = 0;
  std::uint64_t first_reads = 0, first_bytes = 0, final_reads = 0, final_bytes = 0;
  std::size_t spans = 0, stored_bytes = 0, largest_span = 0, overlaps = 0;
};
namespace detail {
inline bool same_frame(const FrameObservation& a, const FrameObservation& b) noexcept {
  return grind_recipe_snapshot_detail::same_key(a.key, b.key) && a.scope_serial == b.scope_serial &&
         a.current_thread == b.current_thread && a.outer_depth == b.outer_depth &&
         a.quiet == b.quiet && a.writer == b.writer && a.input_serial == b.input_serial &&
         a.active_scope == b.active_scope;
}
inline bool valid_frame(const FrameObservation& f, std::uint32_t owner) noexcept {
  return owner && grind_recipe_snapshot_detail::valid_key(f.key) && f.key.thread == owner &&
         f.current_thread == owner && f.scope_serial && f.outer_depth == 1 && f.quiet &&
         !f.writer && f.active_scope;
}
inline void word(Sha256& hash, std::uint32_t value) noexcept {
  std::array<std::uint8_t, 4> bytes{};
  for (unsigned i = 0; i < 4; ++i)
    bytes[i] = static_cast<std::uint8_t>(value >> (i * 8));
  hash.update(bytes.data(), bytes.size());
}
inline void wide(Sha256& hash, std::uint64_t value) noexcept {
  word(hash, static_cast<std::uint32_t>(value));
  word(hash, static_cast<std::uint32_t>(value >> 32));
}
inline void key(Sha256& hash, const GrindRecipeKey& k) noexcept {
  wide(hash, k.generation);
  wide(hash, k.frame);
  for (const auto v : {k.thread, k.world, k.hero, k.station, k.manager,
                       static_cast<std::uint32_t>(k.production_kind)})
    word(hash, v);
}

// Only exact spans. An overlapping, non-identical request reads
// truly anew and compares the intersection; no Widening/page cache.
class ReadSet {
public:
  static constexpr bool diagnostic_only = false;
  enum class Phase { idle, recording, replay, checking, published, failed };
  ReadSet() noexcept = default;
  ReadSet(const ReadSet&) = delete;
  ReadSet& operator=(const ReadSet&) = delete;
  bool begin(const ReadOnlyMemory& original, FrameSource frames, const FrameObservation& expected,
             std::uint32_t owner) noexcept {
    if (active_ || probing_)
      return reject(Issue::reentry);
    original_ = original;
    frames_ = frames;
    expected_ = expected;
    statistics_ = {};
    index_.fill(0);
    page_heads_.fill(0);
    issue_ = Issue::none;
    phase_ = Phase::recording;
    trace_ = Sha256{};
    trace_bytes_ = 0;
    first_trace_ = {};
    receipt_ = {};
    if (!original.read || !frames.observe || !valid_frame(expected, owner))
      return reject(Issue::scope);
    return current();
  }
  ReadOnlyMemory memory() noexcept { return {this, &copy, original_.precise_parser_reads}; }
  bool freeze() noexcept {
    if (active_ || phase_ != Phase::recording)
      return reject(active_ ? Issue::reentry : Issue::closed);
    if (!current() || !statistics_.spans)
      return reject(Issue::scope);
    first_trace_ = finish_trace();
    trace_ = Sha256{};
    phase_ = Phase::replay;
    return true;
  }
  bool seal() noexcept {
    if (active_ || phase_ != Phase::replay)
      return reject(active_ ? Issue::reentry : Issue::closed);
    if (statistics_.recording_calls != statistics_.replay_calls ||
        statistics_.recording_bytes != statistics_.replay_bytes || finish_trace() != first_trace_)
      return reject(Issue::replay_changed);
    active_ = true;
    struct Guard {
      bool& active;
      ~Guard() { active = false; }
    } guard{active_};
    if (!current())
      return false;
    phase_ = Phase::checking;
    Sha256 hash;
    key(hash, expected_.key);
    wide(hash, expected_.scope_serial);
    wide(hash, expected_.input_serial);
    hash.update(first_trace_.data(), first_trace_.size());
    for (std::size_t i = 0; i < statistics_.spans; ++i) {
      const auto& span = spans_[i];
      if (!current())
        return false;
      ++statistics_.final_reads;
      statistics_.final_bytes += span.length;
      if (!original_.copy(span.address, scratch_.data(), span.length))
        return reject(Issue::unreadable);
      if (phase_ != Phase::checking || !current())
        return false;
      if (std::memcmp(scratch_.data(), bytes_.data() + span.offset, span.length))
        return reject(Issue::changed);
      word(hash, span.address);
      word(hash, span.length);
      hash.update(scratch_.data(), span.length);
    }
    if (!current())
      return false;
    receipt_ = hash.finish();
    original_ = {};
    phase_ = Phase::published;
    return true;
  }
  bool current() noexcept {
    if (phase_ == Phase::failed || phase_ == Phase::idle)
      return false;
    if (probing_)
      return reject(Issue::reentry);
    probing_ = true;
    struct Guard {
      bool& active;
      ~Guard() { active = false; }
    } guard{probing_};
    if (frames_.matches)
      return frames_.matches(frames_.context, expected_) ? phase_ != Phase::failed
                                                         : reject(Issue::scope);
    // begin() validated every field of expected_; equality also proves validity.
    FrameObservation now{};
    if (!frames_.observe || !frames_.observe(frames_.context, now) || !same_frame(now, expected_))
      return reject(Issue::scope);
    return phase_ != Phase::failed;
  }
  bool published() noexcept { return phase_ == Phase::published && current(); }
  void close() noexcept {
    reject(active_ || probing_ ? Issue::reentry : Issue::closed);
    frames_ = {};
  }
  bool reject(Issue value) noexcept {
    if (phase_ != Phase::failed)
      issue_ = value;
    phase_ = Phase::failed;
    original_ = {};
    receipt_ = {};
    return false;
  }
  Issue issue() const noexcept { return issue_; }
  Phase phase() const noexcept { return phase_; }
  const Statistics& statistics() const noexcept { return statistics_; }
  const lhiver::Digest& receipt() const noexcept { return receipt_; }

private:
  struct Span {
    std::uint32_t address, offset;
    std::uint16_t length, next_on_page;
  };
  static constexpr std::size_t index_size = max_spans * 2;
  static_assert(max_spans < UINT16_MAX && (index_size & (index_size - 1)) == 0,
                "bounded hash table");
  static bool copy(void* context, std::uint32_t address, void* output, std::size_t size) noexcept {
    return static_cast<ReadSet*>(context)->read(address, output, size);
  }
  bool read(std::uint32_t address, void* output, std::size_t size) noexcept {
    if (active_)
      return reject(Issue::reentry);
    if (phase_ != Phase::recording && phase_ != Phase::replay)
      return reject(Issue::closed);
    active_ = true;
    struct Guard {
      bool& active;
      ~Guard() { active = false; }
    } guard{active_};
    if (!current())
      return false;
    if (!address || !output || !size || size > max_span_bytes ||
        std::uint64_t{address} + size > (std::uint64_t{1} << 32))
      return reject(Issue::bounds);
    auto& calls =
        phase_ == Phase::recording ? statistics_.recording_calls : statistics_.replay_calls;
    auto& count =
        phase_ == Phase::recording ? statistics_.recording_bytes : statistics_.replay_bytes;
    if (calls == max_calls_per_pass || size > max_bytes_per_pass - count)
      return reject(Issue::call_limit);
    ++calls;
    count += size;
    append_trace(address, static_cast<std::uint32_t>(size));
    std::uint32_t mixed = address ^ (static_cast<std::uint32_t>(size) * 0x9e3779b9u);
    mixed ^= mixed >> 16;
    mixed *= 0x7feb352du;
    mixed ^= mixed >> 15;
    auto bucket = static_cast<std::size_t>(mixed) & (index_size - 1);
    for (std::size_t probe = 0; probe < index_size;
         ++probe, bucket = (bucket + 1) & (index_size - 1)) {
      const auto entry = index_[bucket];
      if (entry) {
        const auto& span = spans_[entry - 1u];
        if (span.address != address || span.length != size)
          continue;
        std::memcpy(output, bytes_.data() + span.offset, size);
        return current();
      }
      if (phase_ == Phase::replay)
        return reject(Issue::replay_miss);
      if (statistics_.spans == max_spans)
        return reject(Issue::span_limit);
      if (size > byte_capacity - statistics_.stored_bytes)
        return reject(Issue::byte_limit);
      const auto offset = statistics_.stored_bytes;
      ++statistics_.first_reads;
      statistics_.first_bytes += size;
      if (!original_.copy(address, bytes_.data() + offset, size))
        return reject(Issue::unreadable);
      if (phase_ != Phase::recording || !current())
        return false;
      const Span span{address, static_cast<std::uint32_t>(offset), static_cast<std::uint16_t>(size),
                      page_heads_[page_bucket(address, span_width(size))]};
      if (!check_intersections(span))
        return reject(Issue::overlap);
      spans_[statistics_.spans] = span;
      page_heads_[page_bucket(address, span_width(size))] =
          static_cast<std::uint16_t>(statistics_.spans + 1);
      index_[bucket] = static_cast<std::uint16_t>(++statistics_.spans);
      statistics_.stored_bytes += size;
      statistics_.largest_span = std::max(statistics_.largest_span, size);
      std::memcpy(output, bytes_.data() + offset, size);
      return current();
    }
    return reject(Issue::span_limit);
  }
  // Short object fields and long records each use a start-address index.
  // An interval in a class starts at most width - 1 bytes before an intersection.
  static constexpr std::size_t small_span = 64, page_buckets = 8192;
  static_assert(max_span_bytes <= UINT16_MAX && (page_buckets & (page_buckets - 1)) == 0);
  static constexpr std::size_t span_width(std::size_t size) noexcept {
    return size <= small_span ? small_span : max_span_bytes;
  }
  static std::size_t page_bucket(std::uint32_t address, std::size_t width) noexcept {
    const auto page = address / width;
    return (width == small_span ? 0 : page_buckets / 2) +
           ((page ^ (page >> 12)) & (page_buckets / 2 - 1));
  }
  bool check_intersections(const Span& span) noexcept {
    const auto end = std::uint64_t{span.address} + span.length;
    for (const auto width : {small_span, max_span_bytes}) {
      const auto first = span.address < width ? 0 : (span.address - (width - 1)) / width;
      const auto last = static_cast<std::uint32_t>((end - 1) / width);
      for (auto page = first; page <= last; ++page) {
        for (auto entry = page_heads_[page_bucket(static_cast<std::uint32_t>(page * width), width)];
             entry;) {
          const auto& old = spans_[entry - 1u];
          entry = old.next_on_page;
          if (old.address / width != page)
            continue;
          const auto low = std::max(span.address, old.address);
          const auto high = std::min(end, std::uint64_t{old.address} + old.length);
          if (low < high) {
            ++statistics_.overlaps;
            if (std::memcmp(bytes_.data() + span.offset + (low - span.address),
                            bytes_.data() + old.offset + (low - old.address),
                            static_cast<std::size_t>(high - low)))
              return false;
          }
        }
      }
    }
    return true;
  }
  void append_trace(std::uint32_t address, std::uint32_t length) noexcept {
    // Canonical little-endian address/length pairs, identical to word().
    for (const auto value : {address, length}) {
      for (unsigned i = 0; i != 4; ++i)
        trace_buffer_[trace_bytes_ + i] = static_cast<std::uint8_t>(value >> (8 * i));
      trace_bytes_ += 4;
    }
    if (trace_bytes_ == trace_buffer_.size()) {
      trace_.update(trace_buffer_.data(), trace_bytes_);
      trace_bytes_ = 0;
    }
  }
  lhiver::Digest finish_trace() noexcept {
    trace_.update(trace_buffer_.data(), trace_bytes_);
    trace_bytes_ = 0;
    return trace_.finish();
  }
  std::array<std::uint8_t, 512> trace_buffer_{};
  unsigned trace_bytes_ = 0;
  std::array<std::uint16_t, page_buckets> page_heads_{};
  ReadOnlyMemory original_{};
  FrameSource frames_{};
  FrameObservation expected_{};
  bool active_ = false, probing_ = false;
  Issue issue_ = Issue::idle;
  Phase phase_ = Phase::idle;
  Statistics statistics_{};
  Sha256 trace_;
  lhiver::Digest first_trace_{}, receipt_{};
  std::array<Span, max_spans> spans_{};
  std::array<std::uint16_t, index_size> index_{};
  std::array<std::uint8_t, byte_capacity> bytes_{};
  std::array<std::uint8_t, max_span_bytes> scratch_{};
};

inline bool same_item_result(const grind_item_runtime::Evidence& a,
                             const grind_item_runtime::Evidence& b) noexcept {
  namespace run = grind_item_runtime;
  if (!run::matches(a, a.key, a.callback, a.quantity, grind_item_path::Scope::preview) ||
      !run::matches(b, a.key, a.callback, a.quantity, grind_item_path::Scope::preview) ||
      a.stock.fingerprint != b.stock.fingerprint ||
      !grind_guild_snapshot_detail::same(a.physical, b.physical) ||
      !bow_item_runtime::detail::same_module_evidence(a.modules, b.modules) ||
      a.constructors.fingerprint != b.constructors.fingerprint ||
      a.pool.fingerprint != b.pool.fingerprint || a.sort.fingerprint != b.sort.fingerprint ||
      a.world.fingerprint != b.world.fingerprint ||
      !bow_item_renderer::same(a.renderer, b.renderer) ||
      !vob_inventory_insert_compat::same(a.inventory_insert, b.inventory_insert) ||
      !grind_zbug_unpack::same(a.unpack, b.unpack) ||
      a.expected_after_callback != b.expected_after_callback ||
      a.new_output_node != b.new_output_node)
    return false;
  for (std::size_t i = 0; i < a.stock.materials.size(); ++i)
    if (!grind_item_path::detail::same_stock(a.stock.materials[i], b.stock.materials[i]))
      return false;
  return true;
}
} // namespace detail

struct Request {
  std::uint32_t callback = 0, quantity = 0;
};
// Use our existing selection policy without taking a second catalog snapshot beforehand.
// The adapter verifies the external station and dialog path both before AND after
// the batch. Each pass establishes its own current hand, catalog and guild binding.
struct Selection {
  Request current{};
  bool station_path_verified = false, hero_dialog_verified = false;
};
struct BlockedItem {
  bool blocked = false;
  grind_item_runtime::Issue phase = grind_item_runtime::Issue::boundary;
  grind_item_path::Issue inventory = grind_item_path::Issue::binding;
  GrindItemBindingIssue binding = GrindItemBindingIssue::capture;
  bow_item_code::Issue modules = bow_item_code::Issue::none;
  bow_item_native::Issue constructor = bow_item_native::Issue::none,
                         pool = bow_item_native::Issue::none, sort = bow_item_native::Issue::none,
                         world = bow_item_native::Issue::none;
  bow_item_renderer::Issue renderer = bow_item_renderer::Issue::none;
  vob_inventory_insert_compat::Issue insert = vob_inventory_insert_compat::Issue::none;
  grind_zbug_unpack::Issue unpack = grind_zbug_unpack::Issue::key;
  std::uint32_t address = 0, symbol = 0, inventory_address = 0, module_address = 0,
                constructor_symbol = 0, insert_address = 0, unpack_address = 0;
};
inline BlockedItem blocked_item(const grind_item_runtime::Evidence& item) noexcept {
  return {true,
          item.issue,
          item.stock.issue,
          item.stock.input.issue,
          item.modules.issue,
          item.constructors.issue,
          item.pool.issue,
          item.sort.issue,
          item.world.issue,
          item.renderer.issue,
          item.inventory_insert.issue,
          item.unpack.issue,
          item.failed_address,
          item.failed_symbol,
          item.stock.failed_address,
          item.modules.code.address,
          item.constructors.symbol,
          item.inventory_insert.address,
          item.unpack.address};
}
inline bool same_blocked(const BlockedItem& a, const BlockedItem& b) noexcept {
  return a.blocked && b.blocked && a.phase == b.phase && a.inventory == b.inventory &&
         a.binding == b.binding && a.modules == b.modules && a.constructor == b.constructor &&
         a.pool == b.pool && a.sort == b.sort && a.world == b.world && a.renderer == b.renderer &&
         a.insert == b.insert && a.unpack == b.unpack && a.address == b.address &&
         a.symbol == b.symbol && a.inventory_address == b.inventory_address &&
         a.module_address == b.module_address && a.constructor_symbol == b.constructor_symbol &&
         a.insert_address == b.insert_address && a.unpack_address == b.unpack_address;
}
// Exactly the previous capture_grind condition before the N1-item entry.
inline bool candidate(const CraftingState& row) noexcept {
  return row.known && row.contract && row.actor_matches && row.context_matches &&
         row.material_known && row.hand_known && row.materials[0].stored > 0 &&
         row.materials[1].stored != INT32_MAX && row.materials[2].stored != INT32_MAX;
}
struct Publication {
  GrindRecipeKey key{};
  FrameObservation scope{};
  GrindRecipeCatalogueCapture catalogue{};
  GrindGuildSnapshot physical{};
  GrindMaterialProofs station{};
  CraftingSnapshot snapshot{};
  std::array<Request, max_requests> requests{};
  std::array<grind_item_runtime::Evidence, max_requests> items{};
  std::array<BlockedItem, max_requests> blocked{};
  std::size_t count = 0;
  lhiver::Digest read_set{};
  static constexpr bool native_effect_authorized = false;
};
inline const grind_item_runtime::Evidence* find(const Publication& value, std::uint32_t callback,
                                                std::uint32_t quantity) noexcept {
  if (value.count > max_requests)
    return nullptr;
  for (std::size_t i = 0; i < value.count; ++i)
    if (value.requests[i].callback == callback && value.requests[i].quantity == quantity)
      return !value.blocked[i].blocked &&
                     grind_item_runtime::matches(value.items[i], value.key, callback, quantity,
                                                 grind_item_path::Scope::preview)
                 ? &value.items[i]
                 : nullptr;
  return nullptr;
}
// Only the completed preview can supply this pure quantity comparison. No
// engine context/module getter here; Native binds the entire scope fresh.
inline bool quantity_matches(const Publication& value, const CraftingRequest& request) noexcept {
  const auto* recipe = crafting_catalog::find(request.id);
  if (!recipe || !recipe->grind || request.key.kind != crafting_catalog::Station::grind ||
      !(request.key == value.snapshot.key) || !value.station.station_hand_end_verified ||
      !same_crafting_hands(request.hands, value.snapshot.hands))
    return false;
  const auto* item = find(value, recipe->grind->function.symbol, request.quantity);
  const CraftingState* row = nullptr;
  for (const auto& current : value.snapshot.recipes)
    if (current.id == request.id) {
      row = &current;
      break;
    }
  if (!item || !row || !same_crafting_state(*row, request.material_snapshot) ||
      !same_crafting_hands(item->stock.input.hands, request.hands))
    return false;
  const auto actual = grind_preview_knowledge(value.catalogue, value.physical, value.key);
  const auto& wanted = request.material_snapshot.grind;
  if (!wanted.known || !actual.talent_known || !actual.guild_known || !actual.skill_known ||
      actual.talent != wanted.talent || actual.guild != wanted.guild ||
      actual.skill != wanted.skill)
    return false;
  const auto plan = crafting_grind_plan(request.material_snapshot, request.quantity);
  if (!plan.valid)
    return false;
  for (std::size_t i = 0; i < 3; ++i)
    if (item->stock.materials[i].symbol != request.material_snapshot.materials[i].symbol ||
        item->stock.materials[i].total != request.material_snapshot.materials[i].stored ||
        item->expected_after_callback[i] != plan.after_callback[i].amount)
      return false;
  return true;
}

// Keep work area persistent outside the x86 stack. The two existing
// readers remain real readers; they are just supplied differently.
// No memory adapter and no unfinished result leaves this class.
template <class ReadSet = detail::ReadSet> class BasicReader {
public:
  BasicReader(std::uint32_t owner, GrindRecipeCatalogueReader& catalogue,
              GrindItemRuntimeReader& runtime) noexcept
      : owner_(owner), catalogue_reader_(catalogue), runtime_reader_(runtime) {}
  BasicReader(const BasicReader&) = delete;
  BasicReader& operator=(const BasicReader&) = delete;
  bool capture(const ReadOnlyMemory& original, FrameSource frames, const FrameObservation& expected,
               const grind_item_runtime::Input& input, const Request* requests,
               std::size_t count) noexcept {
    return capture_impl(original, frames, expected, input, requests, count, nullptr);
  }
  bool capture_selection(const ReadOnlyMemory& original, FrameSource frames,
                         const FrameObservation& expected, const grind_item_runtime::Input& input,
                         const Selection& selection) noexcept {
    return capture_impl(original, frames, expected, input, nullptr, 0, &selection);
  }
  bool publish(Publication& output) noexcept {
    output = {};
    if constexpr (ReadSet::diagnostic_only)
      return false;
    if (active_ || !ready_ || !reads_.published()) {
      ready_ = false;
      return false;
    }
    output = result_;
    return true;
  }
  bool diagnostic_result(Publication& output) noexcept {
    static_assert(ReadSet::diagnostic_only, "nur der getrennte Messreader");
    output = {};
    if (active_ || !ready_ || !reads_.published())
      return false;
    output = result_;
    return true;
  }
  const ReadSet& read_set() const noexcept { return reads_; }
  void close() noexcept {
    ready_ = false;
    reads_.close();
  }
  Issue issue() const noexcept { return reads_.issue(); }
  const Statistics& statistics() const noexcept { return reads_.statistics(); }

private:
  bool capture_impl(const ReadOnlyMemory& original, FrameSource frames,
                    const FrameObservation& expected, const grind_item_runtime::Input& input,
                    const Request* requests, std::size_t count,
                    const Selection* selection) noexcept {
    if (active_) {
      ready_ = false;
      return reads_.reject(Issue::reentry);
    }
    ready_ = false;
    active_ = true;
    struct Guard {
      bool& active;
      ~Guard() { active = false; }
    } guard{active_};
    if (!reads_.begin(original, frames, expected, owner_))
      return false;
    if ((count && !requests) || count > max_requests || !input.boundary.stable_boundary_verified ||
        input.boundary.current_thread != owner_ ||
        !grind_recipe_snapshot_detail::same_key(input.boundary.key, expected.key))
      return reads_.reject(Issue::request);
    if (selection &&
        ((selection->current.callback && !grind_recipe::find(selection->current.callback)) ||
         (!selection->current.callback && selection->current.quantity) ||
         selection->current.quantity > INT32_MAX))
      return reads_.reject(Issue::request);
    for (std::size_t i = 0; i < count; ++i) {
      if (!grind_recipe::find(requests[i].callback) || !requests[i].quantity ||
          requests[i].quantity > INT32_MAX)
        return reads_.reject(Issue::request);
      requests_[i] = requests[i];
    }
    const auto frozen_input = input;
    const auto frozen_selection = selection ? *selection : Selection{};
    const auto* policy = selection ? &frozen_selection : nullptr;
    if (!evaluate(first_, frozen_input, count, policy) || !reads_.freeze() ||
        !evaluate(result_, frozen_input, count, policy))
      return false;
    if (first_.count != result_.count ||
        !grind_guild_snapshot_detail::same(first_.physical, result_.physical))
      return reads_.reject(Issue::replay_changed);
    for (std::size_t i = 0; i < first_.count; ++i) {
      if (first_.requests[i].callback != result_.requests[i].callback ||
          first_.requests[i].quantity != result_.requests[i].quantity ||
          first_.blocked[i].blocked != result_.blocked[i].blocked)
        return reads_.reject(Issue::replay_changed);
      if (first_.blocked[i].blocked ? !same_blocked(first_.blocked[i], result_.blocked[i])
                                    : !detail::same_item_result(first_.items[i], result_.items[i]))
        return reads_.reject(Issue::replay_changed);
    }
    // Same complete read sequence + same private bytes, two real own
    // evaluations. Never use Structpadding as value comparison/hash.
    if (!reads_.seal())
      return false;
    result_.read_set = reads_.receipt();
    result_.scope = expected;
    ready_ = true;
    return true;
  }
  bool item(Publication& out, const grind_item_runtime::Input& input, Request request,
            grind_item_runtime::FrozenChecks* frozen = nullptr) noexcept {
    if (out.count == max_requests)
      return reads_.reject(Issue::request);
    const auto index = out.count++;
    out.requests[index] = request;
    if (frozen && request.quantity > 1) {
      for (std::size_t i = 0; i < index; ++i) {
        if (out.requests[i].callback == request.callback && out.items[i].item_path_verified) {
          frozen->selected_unit = &out.items[i];
          break;
        }
      }
    }
    const auto found = runtime_reader_.capture(reads_.memory(), out.catalogue, request.callback,
                                               request.quantity, input, &out.physical, frozen);
    if (!reads_.current())
      return false;
    if (grind_item_runtime::matches(found, out.key, request.callback, request.quantity,
                                    grind_item_path::Scope::preview)) {
      out.items[index] = found;
      if (frozen)
        frozen->common = &out.items[index];
    } else {
      if (found.item_path_verified || found.issue == grind_item_runtime::Issue::none ||
          found.issue == grind_item_runtime::Issue::thread ||
          found.issue == grind_item_runtime::Issue::reentry ||
          found.stock.issue == grind_item_path::Issue::thread ||
          found.stock.issue == grind_item_path::Issue::reentry)
        return reads_.reject(Issue::runtime);
      // No positive partial finding leaves the copy. Only explicit diagnostic values.
      out.blocked[index] = blocked_item(found);
    }
    return true;
  }
  bool evaluate(Publication& out, const grind_item_runtime::Input& input, std::size_t count,
                const Selection* selection) noexcept {
    out = {};
    out.key = input.boundary.key;
    const auto copy = reads_.memory();
    out.catalogue = catalogue_reader_.capture(copy, lhiver::game_parser, out.key, true);
    if (out.catalogue.diagnostic.issue != GrindCaptureIssue::none)
      return reads_.reject(Issue::catalogue);
    out.physical = read_grind_guild_for_preview(copy, lhiver::game_parser, out.key);
    if (!out.physical.known())
      return reads_.reject(Issue::runtime);
    if (!selection) {
      for (std::size_t i = 0; i < count; ++i)
        if (!item(out, input, requests_[i]))
          return false;
    } else {
      grind_item_runtime::FrozenChecks shared_checks{};
      const auto& raw = out.catalogue;
      const auto& d = raw.diagnostic;
      out.station.stable_boundary_verified =
          input.boundary.stable_boundary_verified && d.inventory_unchanged && d.hands_unchanged;
      if (out.station.stable_boundary_verified && selection->station_path_verified &&
          grind_destroy_observation::detail::active_grind_hand(raw.hands)) {
        out.station.station_hand_end_verified = true;
        out.station.verified_left_tmp_level = raw.hands.left_tmp_level;
      }
      GrindProjectionInput projection;
      projection.key = out.key;
      projection.scope = grind_item_path::Scope::preview;
      projection.physical = out.physical;
      projection.station = out.station;
      projection.physical_hero_preview_verified = selection->hero_dialog_verified;
      out.snapshot = project_capture(raw, projection);
      if (!valid_crafting_snapshot(out.snapshot))
        return reads_.reject(Issue::catalogue);
      for (std::size_t i = 0; i < grind_recipe::recipes.size(); ++i) {
        if (!candidate(out.snapshot.recipes[crafting_catalog::grind_begin + i]))
          continue;
        const auto index = out.count;
        if (!item(out, input, {grind_recipe::recipes[i].function.symbol, 1}, &shared_checks))
          return false;
        if (!out.blocked[index].blocked)
          projection.materials[i] = bind_grind_material(raw, out.items[index], out.station);
      }
      out.snapshot = project_capture(raw, projection);
      if (!valid_crafting_snapshot(out.snapshot))
        return reads_.reject(Issue::catalogue);
      const auto chosen = selection->current;
      if (chosen.quantity > 1)
        for (std::size_t i = 0; i < grind_recipe::recipes.size(); ++i) {
          if (grind_recipe::recipes[i].function.symbol != chosen.callback)
            continue;
          const auto& row = out.snapshot.recipes[crafting_catalog::grind_begin + i];
          if (row.material_verified && chosen.quantity <= row.maximum &&
              crafting_quantity_block_reason(row, chosen.quantity) == CraftingBlockReason::none &&
              !item(out, input, chosen, &shared_checks))
            return false;
        }
    }
    return reads_.current();
  }
  std::uint32_t owner_ = 0;
  bool active_ = false, ready_ = false;
  GrindRecipeCatalogueReader& catalogue_reader_;
  GrindItemRuntimeReader& runtime_reader_;
  ReadSet reads_;
  std::array<Request, max_requests> requests_{};
  Publication first_{}, result_{};
};
using Reader = BasicReader<>;
} // namespace ubc::grind_preview_batch
