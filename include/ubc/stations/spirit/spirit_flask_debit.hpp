// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/spirit_recipe_catalog.hpp"
#include "ubc/stations/alchemy/alchemy_flask_debit.hpp"

namespace ubc::spirit_debit {
namespace base=alchemy_flask_debit;
namespace cat=spirit_recipe_catalog;
using base::Context;using base::Scope;using base::Admission;using base::Row;
using base::FlaskStack;using base::Hand;using base::ParserState;using base::Call;
using base::RestoreObservation;using base::EndWitness;using base::Issue;
using base::Stage;using base::Status;using base::same;using base::valid;
using base::listed_flask;
inline constexpr std::uint32_t flask_symbol=16604;
inline constexpr std::size_t row_capacity=cat::target_capacity;
// Only the fully verified successful VOID-Callback20370 leaves four
// unused Npc_RemoveInvItems-INT-Returns. BEFORE remains empty per
// base::valid. Neither another callback nor any non-empty stack counts.
inline bool valid_after_original(const ParserState& s,const Context& c,std::uint32_t callback) noexcept {
  if(callback!=20370 || !base::valid_parser_bindings(s,c) || s.data_depth!=8 || !s.data_words_known)return false;
  for(std::size_t i=0;i<8;++i)if(s.data_words[i]!=(i%2?0x40u:1u))return false;
  return true;
}
inline bool restorable(const Call& call,const RestoreObservation& current) noexcept {
  return spirit_debit::valid_after_original(call.parser,call.context,call.callback) && base::restorable(call,current);
}
struct Job:base::Job {std::int32_t branch_value=0;};
struct Stocks {std::array<Row,row_capacity> rows{};std::size_t count=0;bool complete_unambiguous=false;};
struct Observation {
  Context context{};Scope scope{};std::uint64_t serial=0;
  Stocks stocks{};FlaskStack flask{};Hand hand{};ParserState parser{};
  std::int32_t production=-1,invincible=-1;
  bool native_contract=false,stable_read=false,branch_known=false;
  std::int32_t branch_value=0;
};
inline bool read_rows(const Stocks& stocks,std::array<std::int64_t,row_capacity>& result) noexcept {
  if(!stocks.complete_unambiguous||stocks.count!=row_capacity)return false;
  for(std::size_t i=0;i<row_capacity;++i){const Row* found=nullptr;
    for(std::size_t j=0;j<stocks.count;++j)if(stocks.rows[j].symbol==static_cast<std::uint32_t>(cat::targets[i].symbol)){
      if(found)return false;found=&stocks.rows[j];}
    if(!found||!found->known||found->stored<0||found->stored>INT32_MAX)return false;
    result[i]=found->stored;
  }return true;
}
inline bool same_stocks(const Stocks& a,const Stocks& b) noexcept {
  std::array<std::int64_t,row_capacity> x{},y{};return read_rows(a,x)&&read_rows(b,y)&&x==y;
}
inline std::size_t output_index(std::int32_t branch) noexcept {return branch==1?6u:5u;}
inline std::uint32_t maximum(const std::array<std::int64_t,row_capacity>& rows,std::int32_t branch) noexcept {
  std::uint64_t value=cat::maximum_quantity;
  for(std::size_t i=0;i<4;++i){const auto n=static_cast<std::uint64_t>(rows[i])/(i?1u:2u);if(n<value)value=n;}
  const auto flasks=static_cast<std::uint64_t>(rows[4])+1;
  if(flasks<value)value=flasks;
  const auto room=static_cast<std::uint64_t>(INT32_MAX-rows[output_index(branch)]);if(room<value)value=room;
  return static_cast<std::uint32_t>(value);
}

// Own seven-goal contract. No fake profile in the linear 13er tracker.
// Only an already authorized writer, an original call, at most one debit;
// Branch is the exact integer, not merely a derived output name from it.
class Tracker {
public:
  Tracker() noexcept=default;Tracker(const Tracker&)=delete;Tracker& operator=(const Tracker&)=delete;
  Status status() const noexcept{return status_;}
  const Job& job() const noexcept{return job_;}
  const std::array<std::int64_t,row_capacity>& expected() const noexcept{return expected_;}
  std::uint64_t serial() const noexcept{return last_serial_;}
  bool quarantine(Issue issue) noexcept {if(!status_.blocked){status_.blocked=true;status_.issue=issue;}return false;}
  bool arm(const Job& job,const Admission& proof,const Observation& before) noexcept {
    if(status_.blocked||(status_.stage!=Stage::idle&&status_.stage!=Stage::complete))return false;
    if(!valid(job.context)||job.callback!=20370||job.owner!=20368||!job.id||job.id<=last_job_||
       !job.writer_token||job.writer_token<=last_token_||!job.quantity||job.quantity>cat::maximum_quantity||
       !proof.local_script_contract||!proof.original_amount_contract||!proof.native_debit_and_restore_contract||
       !proof.flask_constructor||!proof.owner_and_knowledge||!proof.physical_lab_hand_contract)return false;
    if(!same(job.context,before.context)||!scope_matches(job,before.scope,0)||!base_read(before)||
       before.serial<=last_serial_||!before.branch_known||before.branch_value!=job.branch_value||
       before.production!=4||before.invincible!=1||!valid_hand(before.hand)||!valid(before.parser,job.context))return false;
    std::array<std::int64_t,row_capacity> rows{};
    if(!read_rows(before.stocks,rows)||job.quantity>maximum(rows,job.branch_value))return false;
    if(job.quantity>1&&(!listed_flask(before.flask,rows[4])||before.flask.object==before.hand.right))return false;
    job_=job;before_=before;expected_=rows;
    for(std::size_t i=0;i<4;++i)expected_[i]-=static_cast<std::int64_t>(job.quantity)*(i?1:2);
    expected_[output_index(job.branch_value)]+=job.quantity;
    status_={Stage::armed,Issue::none,false};last_job_=job.id;last_token_=job.writer_token;
    last_serial_=before.serial;post_parser_={};return true;
  }
  bool original_returned(const Observation& after,bool normal,bool restored) noexcept {
    if(status_.blocked)return false;
    if(status_.stage!=Stage::armed)return quarantine(Issue::sequence);
    if(!normal)return quarantine(Issue::original_exception);
    if(!restored)return quarantine(Issue::amount_restore);
    if(!common_after(after,0))return false;
    if(!rows_equal(after.stocks,expected_))return quarantine(Issue::original_result);
    if(!spirit_debit::valid_after_original(after.parser,job_.context,job_.callback))return quarantine(Issue::parser);
    if(job_.quantity>1&&!same_flask_before(after))return quarantine(Issue::flask);
    post_parser_=after.parser;last_serial_=after.serial;
    status_.stage=job_.quantity==1?Stage::debit_verified:Stage::awaiting_debit;return true;
  }
  Call begin_debit(const Observation& late) noexcept {
    if(status_.blocked)return {};
    if(status_.stage!=Stage::awaiting_debit){quarantine(Issue::sequence);return {};}
    if(!common_after(late,0))return {};
    if(!rows_equal(late.stocks,expected_)||!same_flask_before(late)){quarantine(Issue::stocks);return {};}
    if(!same(post_parser_,late.parser)){quarantine(Issue::parser);return {};}
    last_serial_=late.serial;status_.stage=Stage::calling;
    return {true,job_.id,job_.writer_token,job_.context,job_.callback,flask_symbol,job_.quantity-1,before_.flask.object,late.parser};
  }
  bool debit_returned(const Observation& after,bool normal,bool restored) noexcept {
    if(status_.blocked)return false;
    if(status_.stage!=Stage::calling)return quarantine(Issue::sequence);
    if(!normal)return quarantine(Issue::debit_exception);
    if(!restored)return quarantine(Issue::context_restore);
    if(!common_after(after,1))return false;
    if(!same(post_parser_,after.parser))return quarantine(Issue::parser);
    auto expected=expected_;expected[4]-=job_.quantity-1;
    if(!rows_equal(after.stocks,expected))return quarantine(Issue::debit_result);
    if(!listed_flask(after.flask,expected[4],before_.flask.object))return quarantine(Issue::flask);
    expected_=expected;last_serial_=after.serial;status_.stage=Stage::debit_verified;return true;
  }
  bool finish(const Observation& after,const EndWitness& witness) noexcept {
    if(status_.blocked)return false;
    if(status_.stage!=Stage::debit_verified)return quarantine(Issue::sequence);
    if(!base_read(after)||after.serial<=last_serial_||!same(job_.context,after.context,false)||after.context.mob||
       after.production||after.invincible||after.scope.writer_active||after.scope.writer_depth||after.scope.vm_depth||after.scope.debit_depth||
       !after.branch_known||after.branch_value!=job_.branch_value||!after.hand.known||!after.hand.left_empty||
       after.hand.right||after.hand.symbol||after.hand.amount||after.hand.interact_item||!witness.after||witness.job!=job_.id||
       witness.writer_token!=job_.writer_token||!same(witness.departed,job_.context)||witness.serial<=last_serial_||witness.serial>=after.serial)
      return quarantine(Issue::terminal);
    if(!rows_equal(after.stocks,expected_))return quarantine(Issue::terminal);
    last_serial_=after.serial;status_.stage=Stage::complete;return true;
  }
private:
  static bool base_read(const Observation& v) noexcept{return v.serial&&v.native_contract&&v.stable_read;}
  static bool valid_hand(const Hand& h) noexcept{return h.known&&h.left_empty&&h.right&&h.symbol==flask_symbol&&h.amount==1&&h.interact_item==h.right;}
  static bool scope_matches(const Job& j,const Scope& s,std::uint32_t depth) noexcept {
    return s.writer_active&&s.writer_depth==1&&!s.vm_depth&&s.debit_depth==depth&&s.job==j.id&&s.writer_token==j.writer_token&&s.callback==j.callback;
  }
  bool common_after(const Observation& v,std::uint32_t depth) noexcept {
    if(!same(job_.context,v.context)||!v.branch_known||v.branch_value!=job_.branch_value)return quarantine(Issue::context);
    if(!scope_matches(job_,v.scope,depth))return quarantine(Issue::scope);
    if(!base_read(v)||v.serial<=last_serial_)return quarantine(Issue::observation);
    if(v.production||v.invincible)return quarantine(Issue::original_result);
    if(!valid_hand(v.hand)||!same(before_.hand,v.hand))return quarantine(Issue::hand);return true;
  }
  static bool rows_equal(const Stocks& rows,const std::array<std::int64_t,row_capacity>& expected) noexcept {
    std::array<std::int64_t,row_capacity> actual{};return read_rows(rows,actual)&&actual==expected;
  }
  bool same_flask_before(const Observation& v) const noexcept {
    return listed_flask(v.flask,expected_[4],before_.flask.object)&&v.flask.flags==before_.flask.flags&&v.flask.object!=v.hand.right;
  }
  Status status_{};Job job_{};Observation before_{};ParserState post_parser_{};
  std::array<std::int64_t,row_capacity> expected_{};std::uint64_t last_serial_=0,last_job_=0,last_token_=0;
};
} // namespace ubc::spirit_debit
