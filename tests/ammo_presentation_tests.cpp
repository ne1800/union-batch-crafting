// SPDX-License-Identifier: MIT
#include "ubc/ammo_presentation.hpp"
#include <cstdio>
#include <cstdlib>

namespace p = ubc::ammo_presentation;
namespace {
unsigned checks = 0, failures = 0;
void check(bool value, const char* description) {
  ++checks;
  if (!value) { ++failures; std::fprintf(stderr,"FAIL: %s\n",description); }
}
struct Fixture {
  const p::Template& profile;
  p::Tracker tracker;
  p::Context context{1,7,ubc::lhiver::game_parser,0x1000,0x2000,0x3000};
  p::Admission proof{true,true,true,true,true};
  p::Job job{};
  p::Scope scope{};
  p::Payload payload{};
  p::Capture capture{};
  p::Display display{};
  p::Destruction destruction{};
  p::Result result{};
  explicit Fixture(const p::Template& value = p::templates[0], std::uint32_t lots = 3) : profile(value) {
    job = {context,1,11,profile.callback,profile.owner,lots};
    scope = {job.id,job.scope_token,job.callback,1,true,true};
    payload = {true,p::conversation_vtable,21,profile.text,p::font,-1,30,0x40000000};
    capture = {context,scope,0x4000,0x5000,context.hero,profile.success_pc,p::external_dispatch_return,payload,true,true};
    display = {context,capture.message,context.hero,payload,true};
    destruction = {context,capture.message,payload,true};
    result = {context,job.id,job.callback,lots,profile.output,
      static_cast<std::int64_t>(lots)*profile.units_per_lot,p::ResultState::matched_ready,true};
  }
  bool arm() { return tracker.arm(job,proof); }
  bool cap() { return tracker.capture(capture); }
  bool close() { return tracker.close_effect(context,scope,true); }
  void begin() { check(arm(),"arm exact own amount job"); check(cap(),"capture exact success before queue"); }
};
void changed_context(p::Context& context, unsigned member) {
  switch(member) {
    case 0:++context.epoch;break;
    case 1:++context.thread;break;
    case 2:++context.parser;break;
    case 3:++context.game;break;
    case 4:++context.world;break;
    default:++context.hero;break;
  }
}
void changed_payload(p::Payload& value, unsigned member) {
  switch(member) {
    case 0:value.known=false;break;
    case 1:++value.vtable;break;
    case 2:++value.subtype;break;
    case 3:++value.text.length;break;
    case 4:value.text.sha256[0]^=1;break;
    case 5:++value.font.length;break;
    case 6:value.font.sha256[31]^=1;break;
    case 7:++value.x;break;
    case 8:++value.y;break;
    default:value.seconds_bits=0x7fc00000;break;
  }
}
}
int main() {
  for (const auto& profile : p::templates) {
    check(p::find(profile.callback)==&profile,"five exact native-success templates have stable identities");
    for (const auto lots : {2u,3u,static_cast<std::uint32_t>(INT32_MAX)/profile.units_per_lot}) {
      for (bool immediate : {false,true}) {
        Fixture f(profile,lots);f.begin();
        check(!f.tracker.take_notice(f.context,true).valid,"capture never claims manufacturing or UI success");
        if(!immediate)check(f.close(),"effect returns before deferred print");
        auto foreign=f.display;foreign.message++;
        check(f.tracker.display(foreign)==p::DisplayDecision::original,"foreign message during own pending receipt always original");
        check(f.tracker.display(f.display)==p::DisplayDecision::suppress_owned,"only exact live owned print is suppressed once");
        if(immediate)check(f.close(),"immediate dispatch before effect return is supported");
        auto unknown=f.result;unknown.state=p::ResultState::unknown;
        check(!f.tracker.confirm_result(unknown)&&!f.tracker.status().blocked,"unknown result waits without a success claim");
        check(!f.tracker.take_notice(f.context,true).valid,"suppression alone is not a completed recipe");
        check(f.tracker.confirm_result(f.result),"exact matched Ready delta confirms result");
        const auto notice=f.tracker.take_notice(f.context,true);
        check(notice.valid&&notice.job==f.job.id&&notice.lots==lots&&notice.output==profile.output&&
          notice.produced==lots*profile.units_per_lot,"own notice carries actual bounded output and lot count");
        check(!f.tracker.take_notice(f.context,true).valid,"notice is offered exactly once");
        check(!f.tracker.arm(f.job,f.proof),"outstanding live message prevents another amount job");
        check(f.tracker.acknowledge_notice(f.context,f.job.id,123,true,true),"genuine UI receipt is separately acknowledged");
        check(f.tracker.status().stage==p::Stage::suppressed,"UI registration does not retire a living object");
        check(f.tracker.destroy(f.destruction),"destructor retires receipt before address reuse");
        check(f.tracker.status().stage==p::Stage::idle&&!f.tracker.status().blocked,"all separate facts complete normal lifecycle");
        check(f.tracker.display(f.display)==p::DisplayDecision::original,"retired pointer cannot suppress a reused ordinary message");
        check(!f.tracker.arm(f.job,f.proof),"same job cannot be replayed after completion");
        ++f.job.id;
        check(!f.tracker.arm(f.job,f.proof),"same TLS token cannot be recycled as a new execution");
        ++f.job.scope_token;f.scope.job=f.job.id;f.scope.token=f.job.scope_token;f.capture.scope=f.scope;
        check(f.tracker.arm(f.job,f.proof)&&f.cap(),"same allocator address is safe only with retired lifetime and a new exact capture");
      }
    }
    for (const auto lots : {0u,1u,static_cast<std::uint32_t>(INT32_MAX)/profile.units_per_lot+1u,UINT32_MAX}) {
      Fixture f(profile,lots);
      check(!f.arm()&&!f.tracker.status().blocked,"zero, unchanged one-lot and overflowing quantities never request suppression");
      check(f.tracker.display(f.display)==p::DisplayDecision::original,"unarmed quantities preserve original text");
    }
  }
  check(!p::find(20343)&&!p::find(0),"potions and unknown callbacks are outside presentation scope");
  for(unsigned mode=0;mode<5;++mode) {
    Fixture f;
    switch(mode) {
      case 0:f.proof.native_contract=false;break;
      case 1:f.proof.lifetime_observers=false;break;
      case 2:f.proof.ui_available=false;break;
      case 3:f.proof.original_body_and_amount_contract=false;break;
      default:f.proof.self_equals_hero=false;break;
    }
    check(!f.arm(),"missing prerequisite blocks the amount job before production");
  }
  for(unsigned mode=0;mode<7;++mode) {
    Fixture f;
    switch(mode) {
      case 0:f.job.id=0;break;
      case 1:f.job.scope_token=0;break;
      case 2:f.job.owner++;break;
      case 3:f.job.callback++;break;
      case 4:f.job.context.hero=0;break;
      case 5:f.job.context.epoch=0;break;
      default:f.job.context.parser++;break;
    }
    check(!f.arm(),"invalid own job binding cannot arm");
  }
  for(unsigned field=0;field<6;++field) {
    Fixture cap;check(cap.arm(),"arm capture context test");changed_context(cap.capture.context,field);
    check(!cap.cap()&&cap.tracker.status().blocked,"wrong epoch/thread/parser/game/world/hero at exact PC blocks receipt");
    Fixture show;show.begin();changed_context(show.display.context,field);
    check(show.tracker.display(show.display)==p::DisplayDecision::original&&show.tracker.status().blocked,
      "wrong display context never suppresses a potentially foreign object");
    Fixture destroy;destroy.begin();changed_context(destroy.destruction.context,field);
    check(!destroy.tracker.destroy(destroy.destruction)&&destroy.tracker.status().blocked,"unknown destructor context cannot prove lifetime");
  }
  for(unsigned field=0;field<10;++field) {
    Fixture cap;check(cap.arm(),"arm payload test");changed_payload(cap.capture.payload,field);
    check(!cap.cap()&&cap.tracker.status().blocked,"every capture class/text/font/placement/duration identity is exact");
    Fixture show;show.begin();changed_payload(show.display.payload,field);
    check(show.tracker.display(show.display)==p::DisplayDecision::original&&show.tracker.status().blocked,
      "changed payload after capture never suppresses");
    Fixture destroy;destroy.begin();changed_payload(destroy.destruction.payload,field);
    check(!destroy.tracker.destroy(destroy.destruction)&&destroy.tracker.status().blocked,"changed destructor identity stays quarantined");
  }
  for(unsigned mode=0;mode<6;++mode) {
    Fixture f;check(f.arm(),"arm TLS test");
    switch(mode) {
      case 0:f.capture.scope.active=false;break;
      case 1:f.capture.scope.patched_body_verified=false;break;
      case 2:f.capture.scope.depth=2;break;
      case 3:f.capture.scope.job++;break;
      case 4:f.capture.scope.token++;break;
      default:f.capture.scope.callback++;break;
    }
    check(!f.cap()&&f.tracker.status().blocked,"nested or stale/non-TLS scope cannot capture own PC");
  }
  for(unsigned mode=0;mode<6;++mode) {
    Fixture f;check(f.arm(),"arm native capture binding test");
    switch(mode) {
      case 0:f.capture.message=0;break;
      case 1:f.capture.event_manager=0;break;
      case 2:f.capture.target++;break;
      case 3:f.capture.caller++;break;
      case 4:f.capture.native_contract=false;break;
      default:f.capture.manager_owner_verified=false;break;
    }
    check(!f.cap()&&f.tracker.status().blocked,"capture needs exact native return, message, manager owner and target");
  }
  {
    Fixture f;check(f.arm(),"arm ordinary/failure message test");auto failure=f.capture;failure.vm_pc+=66;
    check(!f.tracker.capture(failure)&&!f.tracker.status().blocked,"original error PC remains ordinary and does not create a receipt");
    check(f.tracker.display(f.display)==p::DisplayDecision::original,"matching text/pointer without capture does not suppress");
    check(!f.close()&&f.tracker.status().issue==p::Issue::missing_capture,"missing success capture after effect blocks further amounts without retry");
  }
  {
    Fixture f;f.begin();check(!f.cap()&&f.tracker.status().blocked,"duplicate own capture quarantines original pointer");
    check(f.tracker.display(f.display)==p::DisplayDecision::original,"quarantined duplicate never suppresses");
  }
  {
    Fixture f;f.begin();auto reused=f.capture;++reused.vm_pc;
    check(!f.tracker.capture(reused)&&f.tracker.status().blocked,"fresh foreign-PC capture at an unretired address proves ambiguous pointer reuse");
    check(f.tracker.display(f.display)==p::DisplayDecision::original,"even identical foreign payload after pointer reuse cannot be suppressed");
  }
  {
    Fixture f;f.begin();check(f.tracker.display(f.display)==p::DisplayDecision::suppress_owned,"first display allowed");
    check(f.tracker.display(f.display)==p::DisplayDecision::original&&f.tracker.status().blocked,"double handler is ambiguous, never a second suppression");
  }
  {
    Fixture f;f.begin();auto foreign=f.destruction;foreign.message++;
    check(!f.tracker.destroy(foreign)&&f.tracker.status().stage==p::Stage::captured,"unrelated destructor does not retire own object");
    check(f.tracker.destroy(f.destruction)&&f.tracker.status().stage==p::Stage::destroyed,"delete-before-display is explicit retired state");
    check(f.tracker.display(f.display)==p::DisplayDecision::original,"destroyed address never suppresses even identical contents");
    check(f.close()&&f.tracker.confirm_result(f.result),"destroyed unseen message may accompany a separately confirmed recipe result");
    check(f.tracker.take_notice(f.context,true).valid,"correct result notice allowed after own message was destroyed unseen");
    check(!f.tracker.arm(f.job,f.proof),"UI offer is not UI registration");
    check(f.tracker.acknowledge_notice(f.context,f.job.id,42,true,true)&&f.tracker.status().stage==p::Stage::idle,
      "registration completes already retired lifetime");
  }
  for(unsigned mode=0;mode<8;++mode) {
    Fixture f;f.begin();check(f.close(),"close result tests");
    switch(mode) {
      case 0:f.result.job++;break;
      case 1:f.result.callback++;break;
      case 2:f.result.lots++;break;
      case 3:f.result.output++;break;
      case 4:f.result.actual_output_delta++;break;
      case 5:f.result.state=p::ResultState::mismatch;break;
      case 6:f.result.bridge_healthy=false;break;
      default:f.result.context.epoch++;break;
    }
    check(!f.tracker.confirm_result(f.result)&&f.tracker.status().blocked,"wrong result identity/delta or unhealthy bridge never produces success");
    check(!f.tracker.take_notice(f.context,true).valid,"failed result cannot be turned into UI success");
  }
  for(unsigned mode=0;mode<4;++mode) {
    Fixture f;f.begin();check(f.close()&&f.tracker.confirm_result(f.result),"prepare UI acknowledgment test");
    check(f.tracker.display(f.display)==p::DisplayDecision::suppress_owned,"own print has completed");
    if(mode!=0)check(f.tracker.take_notice(f.context,true).valid,"offer notice before actual UI registration");
    check(!f.tracker.acknowledge_notice(f.context,mode==1?f.job.id+1:f.job.id,mode==2?0:7,mode!=3,true)&&
      f.tracker.status().blocked,"missing offer, wrong job, zero receipt or failed UI cannot claim presentation complete");
  }
  {
    Fixture f;f.begin();auto next=f.context;++next.epoch;
    check(!f.tracker.observe_context(next,true)&&f.tracker.status().blocked,"world epoch change keeps lifetime quarantined");
    check(f.tracker.display(f.display)==p::DisplayDecision::original&&!f.tracker.destroy(f.destruction),"returning to old context does not revive suppression");
  }
  {
    Fixture f;f.begin();check(!f.tracker.observe_context(f.context,false)&&f.tracker.status().blocked,"bridge observer/lifetime fault permanently blocks own receipt");
  }
  {
    Fixture f;check(!f.tracker.observe_context(f.context,false)&&!f.arm(),"a known bridge failure also blocks future initial admission");
  }
  std::printf("ammo_presentation_tests: %u checks, %u failures\n",checks,failures);
  return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
