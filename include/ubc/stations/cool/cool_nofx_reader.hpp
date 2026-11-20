// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_nofx_profile.hpp"
#include "ubc/stations/cool/cool_hotblade_debit.hpp"
#include "ubc/stations/cool/cool_recipe_runtime.hpp"
#include "ubc/stations/alchemy/alchemy_extra_dependencies.hpp"

// Current NoFX partial proof. No engine calls, writes, hooks, EndFence or
// writer release. Even known==true proves neither Conversation17 nor the
// entire writer window. Double copies are not reservation/synchronization.
namespace ubc::cool_nofx {
namespace dep = alchemy_extra_dependencies;
namespace rs = runtime_script;
namespace cr = cool_recipe_runtime;
namespace hd = cool_hotblade_debit;
using Module = zbug_end_compat::Module;
inline constexpr bool runtime_ready = false;
inline constexpr std::uint32_t literal_index = 51135, external_index = 742, max_depth = 128;
enum class Issue {
  none,
  request,
  thread,
  reentry,
  serial,
  module,
  code,
  unreadable,
  changed,
  context,
  actor,
  script,
  literal,
  cow,
  alias,
  free_route,
  tree,
  capacity,
  duplicate,
  getter,
  fx_present,
  hero_missing
};
struct StackRange {
  // Supplier: real current x86 thread stack limits, no constants.
  bool verified = false;
  std::uint32_t thread = 0, low = 0, high = 0;
};
struct Request {
  hd::Job job{};
  std::uint64_t serial = 0;
  std::uint32_t frame = 0, current_thread = 0;
  Module engine{}, union_core{}, crt{};
  StackRange stack{};
};
struct Literal {
  rs::detail::Layout layout{};
  rs::detail::Symbol symbol{}, external{};
  cr::ActorEvidence actor{};
  std::array<std::uint8_t, 20> object{};
  std::array<std::uint8_t, 13> buffer{}; // Refbyte + eleven characters + NUL.
  std::uint32_t data = 0, init_flag = 0;
};
struct Capture {
  // known means only: the current, fully read narrow
  // subpath. No converter to cool_hotblade_debit::EndFence.
  static constexpr bool native_effect_authorized = false;
  bool known = false;
  Issue issue = Issue::none;
  Request request{};
  Literal literal{};
  std::uint32_t nodes = 0, vobs = 0, depth = 0, first_fx = 0, failed_address = 0;
  std::uint32_t free_entry = 0, free_forward = 0;
};
static_assert(sizeof(Capture) < 4096, "keinen Weltbaum auf dem Callbackstack zurueckgeben");
namespace detail {
inline bool range(std::uint32_t a, std::size_t n) noexcept { return rs::detail::range(a, n); }
inline bool overlap(std::uint32_t a, std::size_t n, std::uint32_t b, std::size_t m) noexcept {
  return zbug_end_compat::detail::overlaps(a, n, b, m);
}
inline bool word(const ReadOnlyMemory& m, std::uint32_t a, std::uint32_t offset,
                 std::uint32_t& out) noexcept {
  return dep::detail::word(m, a, offset, out);
}
inline bool same_symbol(const rs::detail::Symbol& a, const rs::detail::Symbol& b) noexcept {
  return station_recipe_runtime::detail::same_symbol(a, b);
}
inline bool same_literal(const Literal& a, const Literal& b) noexcept {
  return rs::detail::same_layout(a.layout, b.layout) && same_symbol(a.symbol, b.symbol) &&
         same_symbol(a.external, b.external) && cr::same_actor_evidence(a.actor, b.actor) &&
         a.object == b.object && a.buffer == b.buffer && a.data == b.data &&
         a.init_flag == b.init_flag;
}
inline bool request(const Request& q, std::uint32_t thread) noexcept {
  const auto& c = q.job.context;
  return hd::valid(q.job) && q.serial && q.frame && q.current_thread == thread &&
         c.thread == thread && q.stack.verified && q.stack.thread == thread && q.stack.low &&
         q.stack.high > q.stack.low && q.stack.low % 4 == 0 && q.stack.high % 4 == 0 &&
         c.game % 4 == 0 && range(c.game, 12) && c.world % 4 == 0 && range(c.world, 0x38) &&
         c.hero % 4 == 0 && range(c.hero, 0xbc) && c.mob % 4 == 0 && range(c.mob, 4) &&
         c.game != c.world && c.game != c.hero && c.game != c.mob && c.world != c.hero &&
         c.world != c.mob && c.hero != c.mob;
}
inline bool stack_alias(const Request& q, std::uint32_t a, std::size_t n) noexcept {
  return overlap(a, n, q.stack.low, q.stack.high - q.stack.low);
}
inline bool image_alias(const Request& q, std::uint32_t a, std::size_t n) noexcept {
  for (const auto* m : {&q.engine, &q.union_core, &q.crt})
    if (overlap(a, n, m->base, m->image_size))
      return true;
  return false;
}
inline bool modules(const ReadOnlyMemory& m, const Request& q) noexcept {
  if (!q.engine.mapping_verified || q.engine.base != 0x400000 || q.engine.image_size != 0x752000 ||
      q.engine.file_sha256 != profile::engine_sha256 ||
      !dep::detail::module(q.union_core, 0x438000, zbug_end_compat::union_file_sha256) ||
      !dep::detail::module(q.crt, 0xbe000, dep::crt_sha256) ||
      overlap(q.union_core.base, q.union_core.image_size, q.crt.base, q.crt.image_size))
    return false;
  return dep::detail::pe(m, q.engine, profile::engine_timestamp, 0x400000).valid &&
         dep::detail::pe(m, q.union_core, 0x62b8c69e, 0x10000000).valid &&
         dep::detail::pe(m, q.crt, 0x4ba1dbbe, 0x78aa0000).valid;
}
inline bool code(const ReadOnlyMemory& m, std::uint32_t& failed) noexcept {
  for (const auto& r : profile::engine_code)
    if (!dep::detail::plain(m, r).valid) {
      failed = r.address;
      return false;
    }
  return true;
}
inline constexpr std::array<std::uint16_t, 1> free_reloc{{6}};
const zbug_end_compat::CodeRange free_thunk{
    0x35460, 10,
    lhiver::digest_literal("fba8381af52b7bd2da7f71bd892b5c1aaae63b1c55a5d979c0af12d97f992ee2"),
    free_reloc.data(), free_reloc.size()};
inline constexpr std::array<std::uint8_t, 11> free_entry{
    {0x8b, 0xff, 0x55, 0x8b, 0xec, 0x83, 0x7d, 8, 0, 0x74, 0x19}};
inline constexpr std::array<std::uint8_t, 2> free_tail{{0x5d, 0xc3}};
inline bool free_route(const ReadOnlyMemory& m, const Request& q, Capture& out) noexcept {
  const auto u = q.union_core.base, c = q.crt.base;
  if (!dep::detail::export_ordinal(m, q.union_core, 259, 0x35460).valid ||
      !dep::detail::export_ordinal(m, q.crt, 1420, 0x1014e).valid)
    return false;
  if (!zbug_end_compat::detail::verify_code(m, q.union_core, free_thunk).valid)
    return false;
  std::array<std::uint8_t, 11> read_entry{};
  std::array<std::uint8_t, 2> read_tail{};
  std::uint32_t a = 0, b = 0;
  for (unsigned pass = 0; pass < 2; ++pass) {
    if (!m.copy(c + 0x1014e, read_entry.data(), read_entry.size()) || read_entry != free_entry ||
        !m.copy(c + 0x10172, read_tail.data(), read_tail.size()) || read_tail != free_tail ||
        !word(m, 0x82e308, 0, a) || a != u + 0x35460 || !word(m, u, 0x812ec, b) || b != c + 0x1014e)
      return false;
  }
  out.free_entry = a;
  out.free_forward = b;
  return true;
}
inline bool context(const ReadOnlyMemory& m, const Request& q,
                    std::array<std::uint32_t, 8>& v) noexcept {
  const auto& c = q.job.context;
  return word(m, selection_abi::game_pointer, 0, v[0]) && v[0] == c.game &&
         word(m, c.game, 8, v[1]) && v[1] == c.world &&
         word(m, station_abi::hero_pointer, 0, v[2]) && v[2] == c.hero &&
         word(m, c.hero, 0xb8, v[3]) && v[3] == c.world && word(m, c.world, 0, v[4]) &&
         v[4] == 0x83e174 && word(m, v[4], 0x54, v[5]) && v[5] == 0x624250 &&
         word(m, alchemy_flask_runtime::current_parser_pointer, 0, v[6]) && v[6] == c.parser &&
         word(m, 0xab642c, 0, v[7]) && (v[7] & 1u);
}
inline Issue literal(const ReadOnlyMemory& m, const Request& q, Literal& out) noexcept {
  const auto assembly = cr::assemble(cool_recipe_catalog::recipe);
  const auto p = assembly.profile();
  const auto parser = q.job.context.parser;
  if (rs::detail::read_layout(m, parser, p, out.layout) != rs::Issue::none)
    return Issue::script;
  out.actor = cr::actor(m, parser, assembly, q.job.context.hero);
  if (!out.actor.read_valid || !out.actor.matches)
    return Issue::actor;
  if (rs::detail::check_function(m, out.layout, cool_recipe_catalog::functions[14]) !=
          rs::Issue::none ||
      rs::detail::lookup(m, out.layout, external_index, out.external) != rs::Issue::none ||
      rs::detail::name_matches(m, out.external, "WLD_STOPEFFECT") != rs::Issue::none ||
      out.external.type != 5 || out.external.flags != 9 || out.external.elements != 1 ||
      out.external.parent || out.external.offset || out.external.data != 0x6e32b0)
    return Issue::script;
  if (rs::detail::lookup(m, out.layout, literal_index, out.symbol) != rs::Issue::none ||
      out.symbol.type != 3 || out.symbol.flags != 1 || out.symbol.elements != 1 ||
      out.symbol.parent || out.symbol.offset || out.symbol.data % 4 ||
      !range(out.symbol.data, out.object.size()) ||
      !m.copy(out.symbol.data, out.object.data(), out.object.size()))
    return Issue::literal;
  // Anonymous names are renamed at load: no invented DAT name.
  const auto u32 = rs::detail::u32;
  out.data = u32(out.object.data() + 8);
  if (u32(out.object.data()) != 0x82e6f0 || u32(out.object.data() + 12) != 11 ||
      u32(out.object.data() + 16) < 11 || out.data <= 1 ||
      !range(out.data - 1, out.buffer.size()) ||
      !m.copy(out.data - 1, out.buffer.data(), out.buffer.size()))
    return Issue::literal;
  constexpr char text[] = "DEMENTOR_FX";
  if (std::memcmp(out.buffer.data() + 1, text, sizeof(text)))
    return Issue::literal;
  if (out.buffer[0] >= 0xfe)
    return Issue::cow;
  if (!word(m, 0xab642c, 0, out.init_flag) || !(out.init_flag & 1u))
    return Issue::context;
  const auto begin = out.data - 1;
  const auto length = out.buffer.size();
  if (stack_alias(q, out.symbol.data, out.object.size()) || stack_alias(q, begin, length) ||
      image_alias(q, begin, length) || image_alias(q, out.symbol.data, out.object.size()) ||
      overlap(begin, length, out.symbol.data, out.object.size()) ||
      overlap(begin, length, out.symbol.address, 0x3c) ||
      overlap(begin, length, out.external.address, 0x3c) ||
      overlap(begin, length, out.layout.table, std::size_t{out.layout.table_capacity} * 4) ||
      overlap(begin, length, out.layout.code, out.layout.code_capacity) ||
      overlap(begin, length, q.job.context.hero + 0xb8, 4) ||
      overlap(begin, length, q.job.context.game, 12) ||
      overlap(begin, length, q.job.context.world, 4))
    return Issue::alias;
  if (rs::detail::stable_layout(m, parser, p, out.layout) != rs::Issue::none)
    return Issue::changed;
  return Issue::none;
}
inline const profile::ConstantReturn* constant_return(std::uint32_t entry) noexcept {
  std::size_t lo = 0, hi = profile::constant_returns.size();
  while (lo < hi) {
    const auto mid = lo + (hi - lo) / 2;
    const auto& p = profile::constant_returns[mid];
    if (p.entry < entry)
      lo = mid + 1;
    else
      hi = mid;
  }
  return lo < profile::constant_returns.size() && profile::constant_returns[lo].entry == entry
             ? &profile::constant_returns[lo]
             : nullptr;
}
inline bool constant_slot(std::uint32_t address, std::uint32_t entry) noexcept {
  std::size_t lo = 0, hi = profile::constant_slots.size();
  while (lo < hi) {
    const auto mid = lo + (hi - lo) / 2;
    if (profile::constant_slots[mid].address < address)
      lo = mid + 1;
    else
      hi = mid;
  }
  return lo < profile::constant_slots.size() && profile::constant_slots[lo].address == address &&
         profile::constant_slots[lo].entry == entry;
}
struct Node {
  std::uint32_t address = 0, parent = 0, previous = 0, depth = 0;
  std::array<std::uint32_t, 5> fields{};
  std::uint32_t vtable = 0, getter = 0, classdef = 0;
};
inline bool same(const Node& a, const Node& b) noexcept {
  return a.address == b.address && a.parent == b.parent && a.previous == b.previous &&
         a.depth == b.depth && a.fields == b.fields && a.vtable == b.vtable &&
         a.getter == b.getter && a.classdef == b.classdef;
}
inline Issue node(const ReadOnlyMemory& m, const Request& q, const Literal& literal,
                  Node& n) noexcept {
  std::array<std::uint8_t, 20> b{};
  if (!range(n.address, b.size()) || n.address % 4 || n.depth > max_depth)
    return Issue::tree;
  if (!m.copy(n.address, b.data(), b.size()))
    return Issue::unreadable;
  for (unsigned i = 0; i < 5; ++i)
    n.fields[i] = rs::detail::u32(b.data() + i * 4);
  if (n.fields[0] != n.parent || n.fields[3] != n.previous ||
      (!n.parent && (n.fields[2] || n.fields[3])))
    return Issue::tree;
  const auto start = literal.data - 1;
  const auto length = literal.buffer.size();
  if (overlap(start, length, n.address, 20) || stack_alias(q, n.address, 20) ||
      image_alias(q, n.address, 20))
    return Issue::alias;
  const auto vob = n.fields[4];
  if (!vob)
    return Issue::none;
  if (vob % 4 || !range(vob, 4) || !word(m, vob, 0, n.vtable) || !n.vtable || n.vtable % 4 ||
      !word(m, n.vtable, 0, n.getter))
    return Issue::unreadable;
  const auto* canonical = constant_return(n.getter);
  if (!canonical || !constant_slot(n.vtable, n.getter))
    return Issue::getter;
  std::array<std::uint8_t, 6> bytes{}, again{}, expected{{0xb8, 0, 0, 0, 0, 0xc3}};
  dep::detail::put(expected.data() + 1, canonical->value);
  if (!m.copy(n.getter, bytes.data(), bytes.size()) ||
      !m.copy(n.getter, again.data(), again.size()))
    return Issue::unreadable;
  if (bytes != again)
    return Issue::changed;
  if (bytes != expected)
    return Issue::getter;
  n.classdef = canonical->value;
  if (overlap(start, length, vob, 4) || overlap(start, length, n.vtable, 4) ||
      overlap(start, length, n.getter, 6) || stack_alias(q, vob, 4) || stack_alias(q, n.vtable, 4))
    return Issue::alias;
  return n.classdef == profile::fx_class ? Issue::fx_present : Issue::none;
}
} // namespace detail

// Caller owns this Workspace (when capacity is large in the heap/static).
// It is only locked within capture, never beyond a Callback.
template <std::size_t Capacity = 65536> struct Workspace {
  static_assert(Capacity >= 2 && Capacity <= 65536 && (Capacity & (Capacity - 1)) == 0,
                "begrenzte Zweierpotenz");
  Workspace() noexcept = default;
  Workspace(const Workspace&) = delete;
  Workspace& operator=(const Workspace&) = delete;
  std::array<detail::Node, Capacity> nodes{};
  std::array<std::uint32_t, Capacity * 2> node_set{}, vob_set{};
  bool busy = false, reentered = false;
};
template <std::size_t Capacity = 65536> class Reader {
public:
  Reader(std::uint32_t owner_thread, Workspace<Capacity>& workspace) noexcept
      : owner_(owner_thread), w_(workspace) {}
  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;
  Capture capture(const ReadOnlyMemory& m, const Request& supplied) noexcept {
    Capture out;
    out.request = supplied;
    const auto& q = out.request;
    if (!owner_ || q.current_thread != owner_ || q.job.context.thread != owner_) {
      out.issue = Issue::thread;
      return out;
    }
    if (w_.busy) {
      w_.reentered = true;
      out.issue = Issue::reentry;
      return out;
    }
    if (!detail::request(q, owner_)) {
      out.issue = Issue::request;
      return out;
    }
    if (q.serial <= last_serial_) {
      out.issue = Issue::serial;
      return out;
    }
    last_serial_ = q.serial;
    w_.busy = true;
    w_.reentered = false;
    struct Reset {
      Workspace<Capacity>& w;
      ~Reset() { w.busy = false; }
    } reset{w_};
    if (!detail::modules(m, q)) {
      out.issue = Issue::module;
      return out;
    }
    if (!detail::code(m, out.failed_address)) {
      out.issue = Issue::code;
      return out;
    }
    if (!detail::free_route(m, q, out)) {
      out.issue = Issue::free_route;
      return out;
    }
    std::array<std::uint32_t, 8> before{}, after{};
    if (!detail::context(m, q, before)) {
      out.issue = Issue::context;
      return out;
    }
    out.issue = detail::literal(m, q, out.literal);
    if (out.issue != Issue::none)
      return out;
    w_.node_set.fill(0);
    w_.vob_set.fill(0);
    std::size_t count = 0;
    const auto root = q.job.context.world + 0x24;
    if (!enqueue(root, 0, 0, 0, count, out))
      return out;
    std::uint32_t hero_count = 0;
    for (std::size_t i = 0; i < count; ++i) {
      auto& n = w_.nodes[i];
      out.issue = detail::node(m, q, out.literal, n);
      out.nodes = static_cast<std::uint32_t>(i + 1);
      if (out.issue != Issue::none) {
        out.failed_address = n.address;
        if (out.issue == Issue::fx_present)
          out.first_fx = n.fields[4];
        return out;
      }
      if (n.fields[4]) {
        if (!insert(w_.vob_set, n.fields[4])) {
          out.issue = Issue::duplicate;
          return out;
        }
        ++out.vobs;
        if (n.fields[4] == q.job.context.hero) {
          if (n.classdef != 0xab1e20) {
            out.issue = Issue::context;
            return out;
          }
          ++hero_count;
        }
      }
      if (n.depth > out.depth)
        out.depth = n.depth;
      if (n.fields[1] && !enqueue(n.fields[1], n.address, 0, n.depth + 1, count, out))
        return out;
      if (n.fields[2] && !enqueue(n.fields[2], n.parent, n.address, n.depth, count, out))
        return out;
    }
    if (hero_count != 1) {
      out.issue = Issue::hero_missing;
      return out;
    }
    // Fresh second full copy of all reachable nodes and getters.
    // Identical link fields guarantee the same reachability set
    // here, but explicitly NO immutability between the reads.
    for (std::size_t i = 0; i < count; ++i) {
      auto n = w_.nodes[i];
      out.issue = detail::node(m, q, out.literal, n);
      if (out.issue != Issue::none) {
        out.failed_address = n.address;
        return out;
      }
      if (!detail::same(n, w_.nodes[i])) {
        out.issue = Issue::changed;
        out.failed_address = n.address;
        return out;
      }
    }
    Literal next{};
    out.issue = detail::literal(m, q, next);
    if (out.issue != Issue::none)
      return out;
    if (!detail::same_literal(out.literal, next) || !detail::context(m, q, after) ||
        before != after) {
      out.issue = Issue::changed;
      return out;
    }
    if (!detail::modules(m, q)) {
      out.issue = Issue::module;
      return out;
    }
    if (!detail::code(m, out.failed_address)) {
      out.issue = Issue::code;
      return out;
    }
    if (!detail::free_route(m, q, out)) {
      out.issue = Issue::free_route;
      return out;
    }
    if (w_.reentered) {
      out.issue = Issue::reentry;
      return out;
    }
    out.known = true;
    out.issue = Issue::none;
    return out;
  }

private:
  std::uint32_t owner_ = 0;
  Workspace<Capacity>& w_;
  std::uint64_t last_serial_ = 0;
  static bool insert(std::array<std::uint32_t, Capacity * 2>& set, std::uint32_t value) noexcept {
    auto i = (std::size_t{value >> 2} * 2654435761u) & (set.size() - 1);
    for (std::size_t n = 0; n < set.size(); ++n, i = (i + 1) & (set.size() - 1)) {
      if (set[i] == value)
        return false;
      if (!set[i]) {
        set[i] = value;
        return true;
      }
    }
    return false;
  }
  bool enqueue(std::uint32_t a, std::uint32_t parent, std::uint32_t previous, std::uint32_t depth,
               std::size_t& count, Capture& out) noexcept {
    if (count == Capacity) {
      out.issue = Issue::capacity;
      return false;
    }
    if (a % 4 || !detail::range(a, 20) || depth > max_depth) {
      out.issue = Issue::tree;
      return false;
    }
    if (!insert(w_.node_set, a)) {
      out.issue = Issue::duplicate;
      return false;
    }
    w_.nodes[count++] = {a, parent, previous, depth, {}, {}, {}, {}};
    return true;
  }
};
} // namespace ubc::cool_nofx
