// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/spirit/spirit_flask_debit.hpp"
#include "ubc/stations/spirit/spirit_recipe_snapshot.hpp"

namespace ubc {
enum class SpiritResultStage { idle, original_pending, debit_pending, closing, verified, blocked };
struct SpiritResultStatus {
  SpiritResultStage stage = SpiritResultStage::idle;
  spirit_debit::Issue issue = spirit_debit::Issue::none;
};
struct SpiritResultRow {
  std::uint32_t symbol = 0;
  std::int64_t before = -1, expected = -1, actual = -1, consumed = 0, produced = 0, reserved = 0;
};
struct SpiritResultReceipt {
  bool verified = false;
  std::uint64_t job = 0, writer_token = 0, final_serial = 0;
  std::uint32_t quantity = 0;
  std::int32_t branch_value = 0, output = -1;
  std::size_t count = 0;
  std::array<SpiritResultRow, spirit_debit::row_capacity> rows{};
};
class SpiritResult {
public:
  SpiritResultStatus status() const noexcept {
    const auto s = tracker_.status();
    if (s.blocked)
      return {SpiritResultStage::blocked, s.issue};
    if (receipt_.verified)
      return {SpiritResultStage::verified, s.issue};
    if (s.stage == spirit_debit::Stage::idle)
      return {};
    if (s.stage == spirit_debit::Stage::armed)
      return {SpiritResultStage::original_pending, s.issue};
    return {acknowledged_ ? SpiritResultStage::closing : SpiritResultStage::debit_pending, s.issue};
  }
  const SpiritResultReceipt& receipt() const noexcept { return receipt_; }
  spirit_debit::Tracker& tracker_for_bridge() noexcept { return tracker_; }
  void timeout() noexcept { tracker_.quarantine(spirit_debit::Issue::terminal); }
  bool begin(const SpiritRecipeCapture& snapshot, const spirit_debit::Job& job,
             const spirit_debit::Admission& admission,
             const spirit_debit::Observation& before) noexcept {
    namespace d = spirit_debit;
    namespace c = spirit_recipe_catalog;
    const auto& s = snapshot.state;
    const auto& k = snapshot.key;
    const auto& h = snapshot.hands;
    if (s.id != c::RecipeId::spirit || !s.evaluated || !s.known || !s.contract || !s.branch_known ||
        s.branch_value != job.branch_value || s.branch != c::branch_for(true, job.branch_value) ||
        s.output != (job.branch_value == 1 ? 15741 : 15739) ||
        // SELF/category may change between preview and original navigation.
        // The genuine current equality/owner condition is evidenced by
        // Admission, Entry, and before.parser in the writer, not an old UI bool.
        !s.actor_known || !s.context_known || !s.context_matches || !s.material.known() ||
        s.material.quantity || s.material.count != 7 || s.material.output != s.output ||
        s.material.additional_flasks || s.material.craftable || k.generation != job.context.epoch ||
        k.thread != job.context.thread || k.world != job.context.world ||
        k.hero != job.context.hero || k.station != job.context.mob || !k.manager ||
        k.kind != recipe_catalog::Station::alchemy || !h.left_slot_known || !h.right_slot_known ||
        !h.interact_item_known || h.hands.left.state != 0 || h.hands.left.object ||
        h.hands.right.state != 1 || h.hands.right.symbol != 16604 || h.hands.right.amount != 1 ||
        h.hands.right.object != before.hand.right || h.interact_item != before.hand.interact_item)
      return false;
    std::array<std::int64_t, 7> rows{};
    if (!d::read_rows(before.stocks, rows) ||
        s.material.maximum != d::maximum(rows, job.branch_value))
      return false;
    SpiritResultReceipt receipt;
    receipt.job = job.id;
    receipt.writer_token = job.writer_token;
    receipt.quantity = job.quantity;
    receipt.branch_value = job.branch_value;
    receipt.output = s.output;
    receipt.count = 7;
    for (std::size_t i = 0; i < 7; ++i) {
      const auto& r = s.rows[i];
      const auto cost = i < 4 ? (i ? 1u : 2u) : (i == 4 ? 1u : 0u);
      const auto output = i == d::output_index(job.branch_value) ? 1u : 0u;
      const auto reserve = i == 4 ? 1 : 0;
      if (r.symbol != c::targets[i].symbol || r.stored != rows[i] || r.unit_cost != cost ||
          r.unit_output != output || !r.available_known || r.available != r.stored + reserve ||
          r.held != reserve || r.reserved != reserve || r.required || r.produced ||
          r.remaining != r.available)
        return false;
      const auto consumed = std::int64_t{cost} * job.quantity,
                 produced = std::int64_t{output} * job.quantity;
      receipt.rows[i] = {static_cast<std::uint32_t>(r.symbol),
                         r.stored,
                         r.available - consumed + produced,
                         -1,
                         consumed,
                         produced,
                         reserve};
    }
    if (!tracker_.arm(job, admission, before))
      return false;
    receipt_ = receipt;
    acknowledged_ = false;
    original_serial_ = 0;
    original_after_ = {};
    return true;
  }
  bool original_returned(const spirit_debit::Observation& after, bool normal,
                         bool restored) noexcept {
    if (!tracker_.original_returned(after, normal, restored))
      return false;
    original_serial_ = after.serial;
    original_after_ = after;
    acknowledged_ = tracker_.job().quantity == 1;
    return true;
  }
  bool acknowledge_extra_debit(const spirit_debit::Observation& after) noexcept {
    namespace d = spirit_debit;
    std::array<std::int64_t, 7> rows{};
    const auto& j = tracker_.job();
    if (tracker_.status().blocked)
      return false;
    if (acknowledged_ || !original_serial_ || j.quantity <= 1 ||
        tracker_.status().stage != d::Stage::debit_verified || !after.native_contract ||
        !after.stable_read || after.serial != tracker_.serial() ||
        after.serial <= original_serial_ || !d::same(j.context, after.context) ||
        !after.branch_known || after.branch_value != j.branch_value || !after.scope.writer_active ||
        after.scope.writer_depth != 1 || after.scope.vm_depth || after.scope.debit_depth != 1 ||
        after.scope.job != j.id || after.scope.writer_token != j.writer_token ||
        after.scope.callback != j.callback || after.production || after.invincible ||
        !d::same(after.hand, original_after_.hand) ||
        !d::same(after.parser, original_after_.parser) ||
        !d::listed_flask(after.flask, receipt_.rows[4].expected, original_after_.flask.object) ||
        !d::read_rows(after.stocks, rows) || rows != tracker_.expected())
      return tracker_.quarantine(d::Issue::debit_result);
    acknowledged_ = true;
    return true;
  }
  SpiritResultStage observe_completion(const spirit_debit::Observation& after,
                                       const spirit_debit::EndWitness& witness,
                                       bool normal) noexcept {
    namespace d = spirit_debit;
    const auto& job = tracker_.job();
    if (tracker_.status().blocked || receipt_.verified || !original_serial_)
      return status().stage;
    if (!normal || !witness.after || witness.job != job.id ||
        witness.writer_token != job.writer_token || !d::same(witness.departed, job.context) ||
        witness.serial <= tracker_.serial() || after.serial <= witness.serial ||
        !after.native_contract || !after.stable_read)
      return status().stage;
    // Do not mask known contradictions with another unknown field.
    if (!d::same(job.context, after.context, false) || after.context.mob || after.production ||
        after.invincible || after.scope.writer_active || after.scope.writer_depth ||
        after.scope.vm_depth || after.scope.debit_depth ||
        (after.branch_known && after.branch_value != job.branch_value) ||
        (after.hand.known && (!after.hand.left_empty || after.hand.right || after.hand.symbol ||
                              after.hand.amount || after.hand.interact_item))) {
      tracker_.quarantine(d::Issue::terminal);
      return status().stage;
    }
    std::array<std::int64_t, 7> rows{};
    const bool stocks = d::read_rows(after.stocks, rows);
    if (stocks && rows != tracker_.expected()) {
      tracker_.quarantine(d::Issue::terminal);
      return status().stage;
    }
    if (!after.branch_known || !after.hand.known || !stocks)
      return status().stage;
    if (!acknowledged_) {
      tracker_.quarantine(d::Issue::terminal);
      return status().stage;
    }
    if (!tracker_.finish(after, witness))
      return status().stage;
    for (std::size_t i = 0; i < 7; ++i)
      receipt_.rows[i].actual = rows[i];
    receipt_.verified = true;
    receipt_.final_serial = after.serial;
    return status().stage;
  }

private:
  spirit_debit::Tracker tracker_{};
  SpiritResultReceipt receipt_{};
  spirit_debit::Observation original_after_{};
  bool acknowledged_ = false;
  std::uint64_t original_serial_ = 0;
};
} // namespace ubc
