// SPDX-License-Identifier: MIT
#define UBC_ALCHEMY_EXTRA_FIXTURE_ONLY
#include "alchemy_extra_snapshot_tests.cpp"
#include "ubc/alchemy_extra_end_snapshot.hpp"

namespace end_test {
namespace d=ubc::alchemy_flask_debit;
using extra_test::Fixture;
struct EndFixture:Fixture {
  d::Job job{};
  ubc::AlchemyExtraEndFrame frame{};
  explicit EndFixture(const extra_test::cat::Descriptor& r):Fixture(r) {
    job={input.context,2,3,r.callback.function->symbol,static_cast<std::uint32_t>(r.owner.instance),2};
    frame.before=frame.after=input.context;frame.before.mob=frame.after.mob=0;
    frame.current_thread=job.context.thread;frame.serial=20;frame.boundary_verified=true;
    frame.production_before=frame.production_after=frame.invincible_before=frame.invincible_after=0;
    frame.hands_before.hands=frame.hands_after.hands={{0,-1,-1,0},{0,-1,-1,0}};
    frame.hands_before.interact_item_known=frame.hands_after.interact_item_known=true;frame.inventory=input.inventory;
  }
  ubc::AlchemyExtraEndSnapshot read() const {return ubc::alchemy_extra_end_snapshot(recipe,job,frame);}
};
void cases() {
  for(const auto& recipe:extra_test::cat::recipes) {
    EndFixture f(recipe);const auto read=f.read();CHECK(read.valid);
    CHECK(read.observation.context.mob==0 && read.observation.native_contract && read.observation.stable_read);
    CHECK(!read.observation.parser.known && !read.observation.flask.known);
    CHECK(read.observation.stocks.count==recipe.targets.count && read.observation.stocks.complete_unambiguous);
    CHECK(read.observation.hand.known && read.observation.hand.left_empty && !read.observation.hand.right);
    for(std::size_t i=0;i<recipe.targets.count;++i)CHECK(read.observation.stocks.rows[i].stored==f.items[i].total);
    // Finish still requires the separate normal end-AFTER witness.
    d::Tracker tracker;CHECK(!tracker.finish(read.observation,{}));
  }
  auto reject=[](auto mutate) {EndFixture f(extra_test::cat::recipes[0]);mutate(f);CHECK(!f.read().valid);};
  reject([](EndFixture& f){f.job.callback++;});reject([](EndFixture& f){f.job.owner++;});
  reject([](EndFixture& f){f.job.id=0;});reject([](EndFixture& f){f.job.quantity=0;});
  reject([](EndFixture& f){f.frame.before.mob=1;});reject([](EndFixture& f){f.frame.after.world++;});
  reject([](EndFixture& f){f.frame.current_thread++;});reject([](EndFixture& f){f.frame.serial=0;});
  reject([](EndFixture& f){f.frame.boundary_verified=false;});reject([](EndFixture& f){f.frame.scope_before.writer_active=true;});
  reject([](EndFixture& f){f.frame.scope_after.vm_depth=1;});reject([](EndFixture& f){f.frame.scope_before.debit_depth=1;});
  reject([](EndFixture& f){f.frame.scope_after.writer_depth=1;});reject([](EndFixture& f){f.frame.production_before=4;});
  reject([](EndFixture& f){f.frame.invincible_after=-1;});reject([](EndFixture& f){f.frame.hands_before.hands.left.state=-1;});
  reject([](EndFixture& f){f.frame.hands_after.hands.right.object=1;});reject([](EndFixture& f){f.frame.hands_after.interact_item_known=false;});
  reject([](EndFixture& f){f.frame.hands_before.interact_item=1;});reject([](EndFixture& f){f.frame.inventory.snapshot_unchanged=false;});
  reject([](EndFixture& f){f.frame.inventory.packed_known=false;});reject([](EndFixture& f){f.frame.inventory.items.count--;});
  reject([](EndFixture& f){f.items[1].symbol=f.items[0].symbol;});reject([](EndFixture& f){f.items[0].total++;});
  reject([](EndFixture& f){f.items[0].listed=-1;});reject([](EndFixture& f){f.items[0].listed_occurrences=0;});
  reject([](EndFixture& f){f.items[0].listed_occurrences=f.frame.inventory.listed_nodes;});
  reject([](EndFixture& f){f.frame.inventory.listed_nodes=4097;});
  EndFixture zero(extra_test::cat::recipes[0]);zero.set_total(16604,0);zero.frame.inventory=zero.input.inventory;
  CHECK(zero.read().valid); // A fully consumed flask has a known final stock.
}
}
int main() {end_test::cases();std::printf("%u checks, %u failures\n",extra_test::checks,extra_test::failures);return extra_test::failures?1:0;}
