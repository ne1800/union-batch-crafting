// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/engine/read_only_memory.hpp"
#include "ubc/core/sha256.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ubc {

struct AmountOperand {
  std::uint32_t offset = 0; // First byte of the PUSHINT operand, relative to the body.
  std::int32_t original = 0;
  std::uint32_t units = 0, reserved = 0;
  bool allow_zero = false; // Only evidenced comparisons; no zero-set Native effects.
};
struct AmountBody {
  std::uint32_t length = 0;
  std::array<std::uint8_t, 32> digest{};
  const AmountOperand* operands = nullptr;
  std::size_t count = 0;
};

// The native caller holds the exclusive CallFunc writer. identity must verify
// unchanged parser, code and function bindings, including before restoration.
// replace writes only if the expected bytes match exactly. false establishes
// neither a partial write of our own nor its length. No VirtualProtect.
struct AmountMemory {
  ReadOnlyMemory reader{};
  void* context = nullptr;
  bool (*identity)(void*, std::uint32_t, std::size_t) noexcept = nullptr;
  bool (*writable)(void*, std::uint32_t, std::size_t) noexcept = nullptr;
  bool (*replace)(void*, std::uint32_t, const void*, const void*, std::size_t) noexcept = nullptr;
};
enum class AmountIssue {
  none,
  busy,
  contract,
  quantity,
  identity,
  unreadable,
  body_hash,
  operand,
  overflow,
  not_writable,
  write_failed,
  changed,
  restore_failed
};

// Contains only short-lived copies of reads. Never persist them or include
// game bytecode in the package. Change only verified numeric operands.
class AmountPatch {
public:
  static constexpr std::size_t max_body = 4096, max_operands = 64;
  bool begin(const AmountMemory& memory, std::uint32_t address, const AmountBody& body,
             std::uint32_t quantity) noexcept {
    if (active_) {
      issue_ = AmountIssue::busy;
      return false;
    }
    *this = AmountPatch{};
    memory_ = memory;
    address_ = address;
    length_ = body.length;
    if (!address || !length_ || length_ > max_body || length_ - 1u > UINT32_MAX - address ||
        body.count > max_operands || (body.count && !body.operands) || !memory.identity ||
        !memory.writable || !memory.replace) {
      issue_ = AmountIssue::contract;
      return false;
    }
    if (!quantity || quantity > INT32_MAX || (!body.count && quantity != 1)) {
      issue_ = AmountIssue::quantity;
      return false;
    }
    if (!same_identity()) {
      issue_ = AmountIssue::identity;
      return false;
    }
    if (!memory_.reader.copy(address_, original_.data(), length_)) {
      issue_ = AmountIssue::unreadable;
      return false;
    }
    digest_ = body.digest;
    if (hash(original_.data(), length_) != digest_) {
      issue_ = AmountIssue::body_hash;
      return false;
    }
    original_verified_ = true;
    changed_ = original_;
    for (std::size_t i = 0; i < body.count; ++i) {
      const auto& op = body.operands[i];
      if (!op.offset || op.offset >= length_ || length_ - op.offset < 4 ||
          original_[op.offset - 1] != 64 || op.original < 0 || !op.units ||
          decode(original_.data() + op.offset) != static_cast<std::uint32_t>(op.original)) {
        issue_ = AmountIssue::operand;
        return false;
      }
      for (std::size_t j = 0; j < i; ++j)
        if (op.offset < operands_[j].offset + 4 && operands_[j].offset < op.offset + 4) {
          issue_ = AmountIssue::operand;
          return false;
        }
      const auto product = std::uint64_t{op.units} * quantity;
      if (product < op.reserved || product - op.reserved > INT32_MAX ||
          (!op.allow_zero && product == op.reserved)) {
        issue_ = AmountIssue::overflow;
        return false;
      }
      encode(changed_.data() + op.offset, static_cast<std::uint32_t>(product - op.reserved));
      operands_[i] = op;
    }
    count_ = body.count;
    if (!same_identity() || !unchanged_original()) {
      issue_ = AmountIssue::changed;
      return false;
    }
    for (std::size_t i = 0; i < count_; ++i)
      if (!memory_.writable(memory_.context, address_ + operands_[i].offset, 4)) {
        issue_ = AmountIssue::not_writable;
        return false;
      }
    active_ = true;
    for (std::size_t i = 0; i < count_; ++i) {
      const auto offset = operands_[i].offset;
      bool written = false;
      if (same_identity()) {
        // Publish before the call that may raise an SEH exception. The journal records
        // only the attempt. It never establishes ownership of individual bytes.
        attempted_ = i + 1;
        written = memory_.replace(memory_.context, address_ + offset, original_.data() + offset,
                                  changed_.data() + offset, 4);
      }
      if (!written) {
        issue_ = AmountIssue::write_failed;
        const auto prior = issue_;
        if (finish())
          issue_ = prior;
        return false;
      }
    }
    std::array<std::uint8_t, max_body> observed{};
    if (!same_identity() || !memory_.reader.copy(address_, observed.data(), length_) ||
        std::memcmp(observed.data(), changed_.data(), length_) != 0) {
      issue_ = AmountIssue::changed;
      const auto prior = issue_;
      if (finish())
        issue_ = prior;
      return false;
    }
    return true;
  }

  bool finish() noexcept {
    if (!active_)
      return original_verified_ && same_identity() && unchanged_original();
    // A new layout is not permission to describe old addresses.
    if (!same_identity()) {
      issue_ = AmountIssue::identity;
      return false;
    }
    bool restored = true;
    for (std::size_t i = attempted_; i > 0; --i) {
      const auto offset = operands_[i - 1].offset;
      std::array<std::uint8_t, 4> value{};
      if (!same_identity() || !memory_.reader.copy(address_ + offset, value.data(), value.size())) {
        restored = false;
        continue;
      }
      if (std::memcmp(value.data(), original_.data() + offset, value.size()) == 0)
        continue;
      const bool owned = std::memcmp(value.data(), changed_.data() + offset, value.size()) == 0;
      // Receive an external integer as a whole, not restore individual
      // matching bytes from it. Even false or SEH during writing
      // proves no own mix of old/new bytes. Such values remain standing
      // and prevent restore permission; exact own values withdraw
      // regardless. This also applies after final readback mismatch.
      if (!owned || !memory_.writable(memory_.context, address_ + offset, value.size()) ||
          !memory_.replace(memory_.context, address_ + offset, value.data(),
                           original_.data() + offset, value.size()))
        restored = false;
    }
    restored = restored && same_identity() && unchanged_original();
    if (restored) {
      active_ = false;
      issue_ = AmountIssue::none;
    } else
      issue_ = AmountIssue::restore_failed;
    return restored;
  }
  bool active() const noexcept { return active_; }
  bool mutation_attempted() const noexcept { return attempted_ != 0; }
  AmountIssue issue() const noexcept { return issue_; }

  // Read-only identity of our OWN successfully opened window. No
  // memory comparison or writer authorization. Each reader must
  // check the current bytes itself. active() alone is insufficient
  // to normalize an operand. The owner still holds the actual lease.
  bool matches_active(std::uint32_t address, const AmountBody& body,
                      std::uint32_t quantity) const noexcept {
    if (!active_ || issue_ != AmountIssue::none || !original_verified_ || !quantity ||
        quantity > INT32_MAX || address != address_ || body.length != length_ ||
        body.digest != digest_ || body.count != count_ || attempted_ != count_ ||
        (count_ && !body.operands))
      return false;
    for (std::size_t i = 0; i < count_; ++i) {
      const auto& a = operands_[i];
      const auto& b = body.operands[i];
      if (a.offset != b.offset || a.original != b.original || a.units != b.units ||
          a.reserved != b.reserved || a.allow_zero != b.allow_zero)
        return false;
      const auto product = std::uint64_t{a.units} * quantity;
      if (product < a.reserved || product - a.reserved > INT32_MAX ||
          decode(changed_.data() + a.offset) != product - a.reserved)
        return false;
    }
    return count_ || quantity == 1;
  }

private:
  static std::uint32_t decode(const std::uint8_t* p) noexcept {
    return std::uint32_t{p[0]} | (std::uint32_t{p[1]} << 8u) | (std::uint32_t{p[2]} << 16u) |
           (std::uint32_t{p[3]} << 24u);
  }
  static void encode(std::uint8_t* p, std::uint32_t n) noexcept {
    for (unsigned i = 0; i < 4; ++i)
      p[i] = static_cast<std::uint8_t>(n >> (8u * i));
  }
  static std::array<std::uint8_t, 32> hash(const void* bytes, std::size_t length) noexcept {
    Sha256 sha;
    sha.update(bytes, length);
    return sha.finish();
  }
  bool same_identity() const noexcept {
    return memory_.identity && memory_.identity(memory_.context, address_, length_);
  }
  bool unchanged_original() const noexcept {
    std::array<std::uint8_t, max_body> current{};
    return memory_.reader.copy(address_, current.data(), length_) &&
           std::memcmp(original_.data(), current.data(), length_) == 0 &&
           hash(current.data(), length_) == digest_;
  }
  AmountMemory memory_{};
  std::uint32_t address_ = 0, length_ = 0;
  std::size_t count_ = 0;
  // Exclusive writer, no thread synchronization. volatile preserves the
  // journal stores from external calls even for their SEH unwind boundary.
  volatile std::size_t attempted_ = 0;
  std::array<std::uint8_t, max_body> original_{}, changed_{};
  std::array<AmountOperand, max_operands> operands_{};
  std::array<std::uint8_t, 32> digest_{};
  volatile bool active_ = false;
  bool original_verified_ = false;
  AmountIssue issue_ = AmountIssue::none;
};
} // namespace ubc
