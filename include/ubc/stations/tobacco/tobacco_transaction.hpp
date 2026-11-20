// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/tobacco_recipe_catalog.hpp"
#include "ubc/stations/tobacco/tobacco_parser.hpp"

namespace ubc::tobacco {
// Both windows reside exclusively in the hashed script functions of the
// bound ticket. The CallFunc bridge protects both Owner and child together.
template <std::size_t Size> struct ScriptWindow {
  AmountMemory memory{};
  std::uint32_t address = 0;
  std::array<std::uint8_t, Size> original{}, prepared{};
  std::size_t prefix = 0;
  bool attempted = false;

  bool capture(const AmountMemory& adapter, std::uint32_t start,
               const lhiver::Function& f) noexcept {
    memory = adapter;
    address = start;
    if (f.byte_length != Size || !memory.identity || !memory.writable || !memory.replace ||
        !memory.identity(memory.context, address, Size) ||
        !memory.reader.copy(address, original.data(), Size))
      return false;
    Sha256 hash;
    hash.update(original.data(), Size);
    prepared = original;
    return hash.finish() == f.body_sha256;
  }
  bool matches(const std::array<std::uint8_t, Size>& expected) const noexcept {
    std::array<std::uint8_t, Size> found{};
    return memory.identity && memory.identity(memory.context, address, Size) &&
           memory.reader.copy(address, found.data(), Size) && found == expected;
  }
  bool ready() const noexcept {
    return prefix && prefix <= Size && matches(original) &&
           memory.writable(memory.context, address, prefix);
  }
  bool install() noexcept {
    if (!ready())
      return false;
    attempted = true;
    return memory.replace(memory.context, address, original.data(), prepared.data(), prefix) &&
           matches(prepared);
  }
  bool restore() noexcept {
    if (!attempted || matches(original))
      return true;
    return matches(prepared) &&
           memory.replace(memory.context, address, prepared.data(), original.data(), prefix) &&
           matches(original);
  }
};

struct Transaction {
  ScriptWindow<189> owner_body;
  ScriptWindow<36> child_body;
  static constexpr auto owner_offset = functions[2].byte_offset;
  static void operand(std::uint8_t* bytes, std::uint32_t n) noexcept {
    for (unsigned i = 0; i < 4; ++i)
      bytes[i] = static_cast<std::uint8_t>(n >> (i * 8));
  }
  static void jump(std::uint8_t* bytes, std::uint8_t opcode, std::uint32_t target) noexcept {
    bytes[0] = opcode;
    operand(bytes + 1, target);
  }
  void defer_owner() noexcept {
    // Continue at INFO_ClearChoices. The first four checked commands
    // are exclusively NPC_RemoveInvItems(HERO, AppleTobacco, 1).
    jump(owner_body.prepared.data(), 75, owner_offset + 20);
    owner_body.prefix = 5;
  }
  // All VM entries use the same two bound functions. The inventory
  // script also returns before PRINT and station end even on success.
  struct PaymentCode {
    std::array<std::uint8_t, 189> owner{};
    std::array<std::uint8_t, 36> child{};
  };
  bool prepare_child(const Descriptor& recipe, std::uint32_t n, const Stock& before,
                     PaymentCode& pay) noexcept {
    if (!canonical(recipe) || !n || n > maximum(before))
      return false;
    owner_body.prepared[0] = 60;
    owner_body.prefix = 1;
    operand(child_body.prepared.data() + 11, n);
    child_body.prepared[20] = 60;
    child_body.prefix = 21;
    pay.owner = owner_body.prepared;
    pay.child = child_body.original;
    jump(pay.child.data(), 75, owner_offset + 1);
    const auto stop = recipe.callback.function->byte_offset + 35; // checked RETURN
    unsigned at = 1;
    const auto charge = [&](unsigned item, unsigned quantity, unsigned initial) {
      auto* out = pay.owner.data() + at;
      std::copy_n(owner_body.original.data(), 20, out);
      operand(out + 6, item);
      operand(out + 11, quantity);
      jump(out + 20, 76, owner_offset + at + 25); // Reject INT-return, regardless of its value
      std::copy_n(owner_body.original.data() + 50, 20,
                  out + 25); // Wanted, HERO, Instance, NPC_HasItems
      operand(out + 26, initial - quantity);
      operand(out + 36, item);
      out[45] = 16;
      jump(out + 46, 76, stop);
      at += 51;
    };
    charge(apple, n, static_cast<unsigned>(before[0]));
    if (n > 1)
      charge(flask, n - 1, static_cast<unsigned>(before[1]));
    pay.owner[at] = 60;
    static_assert(1 + 2 * 51 + 1 <= std::tuple_size<decltype(pay.owner)>::value);
    return true;
  }
  bool install(bool child, Checks& checks) noexcept {
    const bool owner = owner_body.ready();
    const bool choice = child ? child_body.ready() : child_body.matches(child_body.original);
    checks.flag("install.owner_ready", owner);
    checks.flag("install.child_ready", choice);
    if (!owner || !choice)
      return false;
    const bool installed = owner_body.install();
    checks.flag("install.owner", installed);
    const bool installed_child = !child || (installed && child_body.install());
    checks.equal("install.child", 1, installed_child, installed);
    return installed && installed_child;
  }
  bool at_call(Checks& checks) const noexcept {
    const bool owner = owner_body.matches(owner_body.prepared);
    const bool choice = child_body.matches(child_body.prepared);
    checks.flag("call.owner_window", owner);
    checks.flag("call.child_window", choice);
    return owner && choice;
  }
  template <std::size_t Size>
  static bool change(ScriptWindow<Size>& window,
                     const std::array<std::uint8_t, Size>& next) noexcept {
    if (!window.matches(window.prepared) ||
        !window.memory.writable(window.memory.context, window.address, Size))
      return false;
    const auto previous = window.prepared;
    const bool written = window.memory.replace(window.memory.context, window.address,
                                               previous.data(), next.data(), Size);
    if (written || !window.matches(previous)) {
      window.prepared = next;
      window.prefix = Size;
    }
    return written;
  }
  bool payment_program(const PaymentCode& pay, Checks& checks) noexcept {
    return at_call(checks) && change(owner_body, pay.owner) && change(child_body, pay.child);
  }
  PaymentCode completion() const noexcept {
    PaymentCode code{owner_body.original, child_body.original};
    code.owner[0] = 60;
    std::copy(child_body.original.begin() + 20, child_body.original.end(), code.child.begin());
    return code;
  }
  PaymentCode adjustment(const Descriptor& d, unsigned i, unsigned count,
                         const Stock& observed) const noexcept {
    PaymentCode code{owner_body.original, child_body.original};
    code.owner[0] = 60;
    // Immediately before the single fixed settlement amount, the VM must see
    // exactly the stored inventory from which settle() derived that amount.
    const auto item = static_cast<unsigned>(d.targets[i].symbol);
    jump(code.child.data(), 64, static_cast<unsigned>(observed[i]));
    jump(code.child.data() + 5, 67, 1683);
    jump(code.child.data() + 10, 64, item);
    jump(code.child.data() + 15, 62, 160);
    code.child[20] = 16;
    jump(code.child.data() + 21, 76, d.callback.function->byte_offset + 35);
    jump(code.child.data() + 26, 75, owner_offset + 1);
    auto* body = code.owner.data() + 1;
    std::copy_n(child_body.original.data(), 20, body);
    operand(body + 6, item);
    operand(body + 11, count);
    if (i == 3) {
      std::copy_n(owner_body.original.data() + 15, 5, body + 15);
      jump(body + 20, 76, owner_offset + 26);
      body[25] = 60;
    } else
      body[20] = 60;
    return code;
  }
  bool restore() noexcept {
    const bool child = child_body.restore();
    const bool owner = owner_body.restore();
    return child && owner;
  }
};
enum class Settlement { completed, unpaid, uncertain };

template <class Read, class Adjust, class Finish>
Settlement settle(const Stock& before, std::uint32_t n, Read read, Adjust adjust, Finish finish,
                  Checks& checks) {
  // Call after attempted product output. All four counters come
  // from the same stable intake; the hand flask remains bound.
  if (!n || n > maximum(before))
    return Settlement::uncertain;
  const auto count = static_cast<int>(n);
  const Stock produced{{before[0], before[1], before[2], before[3] + count}};
  const Stock paid{{before[0] - count, before[1] - (count - 1), before[2], before[3] + count}};
  Stock now{};
  const bool known = read(now);
  constexpr const char* fields[] = {"settle.apple", "settle.flask", "settle.additive",
                                    "settle.product"};
  for (unsigned i = 0; i < now.size(); ++i)
    checks.equal(fields[i], produced[i], now[i], known);
  bool payment_entered = false;
  if (known && now == produced && finish(paid, payment_entered))
    return Settlement::completed;

  // Re-read fresh even after rejected final check. Only reconcile
  // differences of this order, never repeat an unclear write attempt.
  const bool refund_read = read(now);
  constexpr const char* limits[] = {"refund.apple", "refund.flask", "refund.additive",
                                    "refund.product"};
  bool attributable = refund_read;
  for (unsigned i = 0; i < now.size(); ++i) {
    const auto low = payment_entered && i < 2 ? paid[i] : before[i];
    const auto high = i == 3 ? produced[i] : before[i];
    char range[32]{};
    std::snprintf(range, sizeof(range), "%d..%d", low, high);
    const bool within = now[i] >= low && now[i] <= high;
    checks.test(limits[i], range, now[i], within, refund_read);
    if (!within)
      attributable = false;
  }
  if (!attributable)
    return Settlement::uncertain;
  for (unsigned i : {0u, 1u, 3u}) {
    while (now[i] != before[i]) {
      const auto previous = now;
      const int difference = before[i] - now[i];
      if (!adjust(i, static_cast<unsigned>(difference > 0 ? difference : -difference), previous) ||
          !read(now))
        return Settlement::uncertain;
      auto same = previous;
      same[i] = now[i];
      if (now != same || (i == 3 ? (now[i] >= previous[i] || now[i] < before[i])
                                 : (now[i] <= previous[i] || now[i] > before[i])))
        return Settlement::uncertain;
    }
  }
  return now == before ? Settlement::unpaid : Settlement::uncertain;
}
} // namespace ubc::tobacco
