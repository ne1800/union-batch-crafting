// SPDX-License-Identifier: MIT
#define UBC_GRIND_ITEM_RUNTIME_FIXTURE_MAIN grind_runtime_fixture_main
#include "grind_item_runtime_tests.cpp"
#include "ubc/stations/grind/grind_preview_batch.hpp"
#include "ubc/stations/grind/grind_preview_measurement.hpp"
#include "ubc/core/crafting_session.hpp"
#include "grind_preview_capacity_fixture.hpp"
#include <memory>
#include <optional>

namespace preview_test {
using namespace grind_runtime_test;
namespace batch=ubc::grind_preview_batch;
using Ledger=batch::detail::ReadSet;
struct Frames {
  batch::FrameObservation value{key(),91,7,1,true,false,0,true};
  bool readable=true;std::size_t observations=0;
  Ledger* nested=nullptr;
  static bool observe(void* context,batch::FrameObservation& out) noexcept {
    auto& f=*static_cast<Frames*>(context);++f.observations;
    if(f.nested){auto* nested=f.nested;f.nested=nullptr;std::uint8_t byte=0;
      check(!nested->memory().copy(0x1000,&byte,1),"scope-probe reentry fails closed");}
    out=f.value;return f.readable;
  }
  batch::FrameSource source(){return {this,observe};}
};
void fill(Image& image,std::uint32_t at,std::size_t size){
  image.block(at,size);for(std::size_t i=0;i<size;++i)*image.at(at+static_cast<std::uint32_t>(i),1)=static_cast<std::uint8_t>(i);
}
void measurement(){
  namespace measure=ubc::grind_preview_measurement;
  {Frames frame;frame.value.key.generation=0;
    check(!batch::detail::valid_frame(frame.value,7),"cold game with no UI generation cannot enter a reader");
    check(measure::start_generation(frame.value.key.generation)&&batch::detail::valid_frame(frame.value,7),
      "measurement opens its own first generation before the native station and frame readers");
    frame.value.key.generation=UINT64_MAX;
    check(!measure::start_generation(frame.value.key.generation)&&frame.value.key.generation==UINT64_MAX,
      "diagnostic generation never wraps into a reused scope");}
  for(unsigned mode=0;mode<3;++mode){
    Image image;constexpr std::uint32_t count=9000,width=16;fill(image,0x1000,count*width);
    Frames frame;auto ledger=std::make_unique<measure::ReadSet>();
    check(ledger->begin(image.memory(),frame.source(),frame.value,7),"separate read-only measurement starts");
    std::array<std::uint8_t,width> out{};const auto memory=ledger->memory();
    bool copied=true;for(std::uint32_t i=0;i<count;++i)copied=copied&&memory.copy(0x1000+i*width,out.data(),width);
    check(copied&&ledger->statistics().spans==count&&ledger->statistics().stored_bytes==count*width,
      "measurement continues beyond BOTH old span and byte budgets without truncating");
    const auto& rejected=ledger->rejected();
    check(rejected.issue==batch::Issue::span_limit&&rejected.address==0x21000&&rejected.length==width&&
      rejected.spans==8192&&rejected.bytes==131072,"first old rejection retains exact address length and prefix");
    check(ledger->union_size()==std::make_pair(std::size_t{1},std::uint64_t{count*width})&&ledger->ranges().size()==count,
      "complete unique range trace reproduces union without source contents");
    check(ledger->freeze(),"measurement freezes entire copy");
    for(std::uint32_t i=0;i<count;++i)copied=copied&&memory.copy(0x1000+i*width,out.data(),width);
    if(mode==1)*image.at(0x1000+count*width-1,1)^=1;
    if(mode==2)++frame.value.input_serial;
    check(copied&&image.reads==count,"measurement replay reads only its private copy");
    check(mode?(!ledger->seal()&&!ledger->published()):ledger->seal()&&ledger->published(),
      "measurement requires complete late reread and unchanged frame including last byte beyond old budget");
  }
  {Image image;fill(image,0x1000,140000);Frames frame;auto ledger=std::make_unique<measure::ReadSet>();
    ledger->begin(image.memory(),frame.source(),frame.value,7);std::array<std::uint8_t,4096> out{};
    for(unsigned i=0;i<33;++i)check(ledger->memory().copy(0x1000+i*4096,out.data(),out.size()),"measurement admits byte overflow");
    check(ledger->rejected().issue==batch::Issue::byte_limit&&ledger->rejected().length==4096&&
      ledger->rejected().bytes==131072&&ledger->statistics().stored_bytes==135168,"byte-only rejection remains a measured prefix");}
  {Image image;fill(image,0x1000,32);Frames frame;auto ledger=std::make_unique<measure::ReadSet>();
    ledger->begin(image.memory(),frame.source(),frame.value,7);std::array<std::uint8_t,12> out{};
    check(ledger->memory().copy(0x1004,out.data(),8)&&ledger->memory().copy(0x1000,out.data(),12),"measurement accepts contained agreeing reads");
    check(ledger->union_size()==std::make_pair(std::size_t{1},std::uint64_t{12}),"measurement union does not double-count overlaps");
    *image.at(0x1007,1)^=1;
    check(!ledger->memory().copy(0x1006,out.data(),8)&&ledger->issue()==batch::Issue::overlap,
      "measurement never hides conflicting overlapping data");}
}
void measured_capacity(){
  // Measured length distribution only. No game bytes or addresses.
  // Disjoint test ranges prevent savings from merging pages.
  namespace fixture=grind_preview_capacity_fixture;
  for(const unsigned extra:{0u,4096u})for(const bool changed:{false,true}){
    Image image;Frames frame;std::vector<std::pair<std::uint32_t,std::size_t>> ranges;
    std::uint32_t address=0x10000;std::size_t bytes=0;
    const auto append=[&](std::size_t length){ranges.emplace_back(address,length);bytes+=length;address+=static_cast<std::uint32_t>(length)+8;};
    for(const auto& entry:fixture::lengths)for(unsigned i=0;i<entry.second;++i)append(entry.first);
    for(unsigned i=0;i<extra;++i)append(12);
    fill(image,0x10000,address-0x10000);auto ledger=std::make_unique<Ledger>();
    check(ranges.size()==10967+extra&&bytes==205195+12*extra,"measured footprint plus explicit disjoint world-node reserve");
    check(ledger->begin(image.memory(),frame.source(),frame.value,7),"measured capacity frame begins");
    std::array<std::uint8_t,batch::max_span_bytes> value{};const auto memory=ledger->memory();bool copied=true;
    for(const auto& range:ranges)for(unsigned duplicate=0;duplicate<2;++duplicate)
      copied=memory.copy(range.first,value.data(),range.second)&&copied;
    check(copied&&image.reads==ranges.size()&&ledger->statistics().stored_bytes==bytes&&ledger->freeze(),
      "complete measured shape and reserve fit fixed storage without dropping or widening reads");
    for(const auto& range:ranges)for(unsigned duplicate=0;duplicate<2;++duplicate)
      copied=memory.copy(range.first,value.data(),range.second)&&copied;
    check(copied&&image.reads==ranges.size(),"measured shape and reserve replay exclusively from the private copy");
    if(changed)*image.at(ranges.back().first+static_cast<std::uint32_t>(ranges.back().second)-1,1)^=1;
    check(changed?!ledger->seal()&&!ledger->published()&&ledger->issue()==batch::Issue::changed:
      ledger->seal()&&ledger->published(),"last measured or reserve byte is mandatory at final verification");
    check(image.reads==ranges.size()*2,"all measured and reserve spans are read twice, including the last span");
  }
}
void low_level(){
  using batch::Issue;
  {Image image;fill(image,0x1000,32);Frames frame;auto ledger=std::make_unique<Ledger>();
    check(ledger->begin(image.memory(),frame.source(),frame.value,7),"explicit outer preview frame opens recorder");
    std::array<std::uint8_t,8> value{};const auto copy=ledger->memory();
    check(copy.copy(0x1000,value.data(),8)&&copy.copy(0x1004,value.data(),8)&&copy.copy(0x1000,value.data(),8),"exact duplicates and agreeing overlaps are copied");
    check(image.reads==2&&ledger->statistics().overlaps==1&&ledger->statistics().stored_bytes==16,"overlap stays a real distinct read; only exact duplicate is shared");
    check(ledger->freeze(),"recorded scope closes to own-copy evaluation");
    check(copy.copy(0x1000,value.data(),8)&&copy.copy(0x1004,value.data(),8)&&copy.copy(0x1000,value.data(),8)&&image.reads==2,"entire replay makes zero original reads");
    check(ledger->seal()&&ledger->published()&&image.reads==4,"seal rereads both complete exact spans once");
    check(ledger->statistics().first_bytes==16&&ledger->statistics().final_bytes==16,"two whole real copies have explicit equal byte counts");
    check(!copy.copy(0x1000,value.data(),8)&&ledger->issue()==Issue::closed&&image.reads==4,"even existing request after seal cannot reopen reads");
  }
  for(unsigned mode=0;mode<5;++mode){Image image;fill(image,0x1000,32);Frames frame;auto ledger=std::make_unique<Ledger>();
    check(ledger->begin(image.memory(),frame.source(),frame.value,7),"negative replay fixture begins");
    std::array<std::uint8_t,8> value{};const auto m=ledger->memory();check(m.copy(0x1000,value.data(),8)&&m.copy(0x1010,value.data(),8),"two exact source spans recorded");
    check(ledger->freeze(),"negative fixture freezes");
    if(mode==0){check(!m.copy(0x1001,value.data(),7)&&ledger->issue()==Issue::replay_miss,"contained but never-recorded request has no fallback");}
    if(mode==1){check(m.copy(0x1010,value.data(),8)&&m.copy(0x1000,value.data(),8)&&!ledger->seal()&&ledger->issue()==Issue::replay_changed,"same bytes in different replay order reject");}
    if(mode==2){check(m.copy(0x1000,value.data(),8)&&!ledger->seal()&&ledger->issue()==Issue::replay_changed,"omitted replay request rejects whole set");}
    if(mode>=3){check(m.copy(0x1000,value.data(),8)&&m.copy(0x1010,value.data(),8),"complete replay before late negative");
      if(mode==3)*image.at(0x1007,1)^=1;
      else{image.denied=0x1010;image.denied_size=8;}
      check(!ledger->seal()&&!ledger->published()&&ledger->issue()==(mode==3?Issue::changed:Issue::unreadable),"late changed or missing byte discards all spans");}
    check(image.reads==(mode==3?3u:mode==4?4u:2u),"no replay error falls back to original memory");
  }
  {Image image;fill(image,0x1000,32);Frames frame;auto ledger=std::make_unique<Ledger>();
    ledger->begin(image.memory(),frame.source(),frame.value,7);std::array<std::uint8_t,8> value{};
    check(ledger->memory().copy(0x1000,value.data(),8),"first overlap copy succeeds");
    *image.at(0x1005,1)^=1;
    check(!ledger->memory().copy(0x1004,value.data(),8)&&ledger->issue()==Issue::overlap&&!ledger->freeze(),"inconsistent overlapping first reads reject immediately");
  }
  for(unsigned mode=0;mode<16;++mode){Image image;fill(image,0x1000,8);Frames frame;auto ledger=std::make_unique<Ledger>();
    ledger->begin(image.memory(),frame.source(),frame.value,7);std::uint8_t value=0;check(ledger->memory().copy(0x1000,&value,1),"scope-change first byte recorded");
    switch(mode){case 0:++frame.value.key.generation;break;case 1:++frame.value.key.frame;break;case 2:++frame.value.key.thread;break;
      case 3:++frame.value.key.world;break;case 4:++frame.value.key.hero;break;case 5:++frame.value.key.station;break;
      case 6:++frame.value.key.manager;break;case 7:++frame.value.key.production_kind;break;case 8:++frame.value.scope_serial;break;
      case 9:++frame.value.current_thread;break;case 10:++frame.value.outer_depth;break;case 11:frame.value.quiet=false;break;
      case 12:frame.value.writer=true;break;case 13:frame.readable=false;break;
      case 14:++frame.value.input_serial;break;case 15:frame.value.active_scope=false;break;}
    check(!ledger->memory().copy(0x1000,&value,1)&&ledger->issue()==Issue::scope&&image.reads==1,"every observed key/owner/scope transition rejects even exact cached read");
  }
  {Image image;fill(image,0x1000,8);Frames frame;auto ledger=std::make_unique<Ledger>();frame.nested=ledger.get();
    check(!ledger->begin(image.memory(),frame.source(),frame.value,7)&&ledger->issue()==Issue::reentry&&image.reads==0,"observer recursion cannot reopen a scope");}
  for(unsigned mode=0;mode<5;++mode){Image image;fill(image,0x1000,8192);Frames frame;auto ledger=std::make_unique<Ledger>();
    ledger->begin(image.memory(),frame.source(),frame.value,7);std::array<std::uint8_t,8192> out{};const auto m=ledger->memory();
    const std::uint32_t address=mode==0?0u:mode==1?UINT32_MAX:0x1000;
    const auto size=mode==1?2u:mode==2?4097u:mode==4?0u:1u;
    check(!m.read(m.context,address,mode==3?nullptr:out.data(),size)&&ledger->issue()==Issue::bounds&&image.reads==0,"null/overflow/oversized/empty raw requests fail before source");}
  {Image image;fill(image,UINT32_MAX,1);Frames frame;auto ledger=std::make_unique<Ledger>();ledger->begin(image.memory(),frame.source(),frame.value,7);
    std::uint8_t v=0;check(ledger->memory().copy(UINT32_MAX,&v,1)&&ledger->freeze()&&ledger->memory().copy(UINT32_MAX,&v,1)&&ledger->seal(),"last single 32-bit byte is representable without overflow");}
  // World run 31: 430 listed objects exhausted 4096 spans. These 5000
  // synthetic reads establish only the limit, NOT the full world requirement.
  for(unsigned changed=0;changed<2;++changed){
    constexpr std::uint32_t count=5000,width=16;Image image;fill(image,0x1000,count*width);Frames frame;
    auto ledger=std::make_unique<Ledger>();ledger->begin(image.memory(),frame.source(),frame.value,7);
    std::array<std::uint8_t,width> value{};bool copied=true;
    for(std::uint32_t i=0;i<count;++i)if(!ledger->memory().copy(0x1000+i*width,value.data(),value.size())){copied=false;break;}
    check(copied,"valid 80KB preview with5000 exact spans fits the fixed workspace budget");
    if(!copied)continue;
    check(ledger->statistics().stored_bytes==80000&&ledger->statistics().spans==count&&ledger->freeze(),
      "large real-inventory-shaped recording retains exact spans without widening");
    for(std::uint32_t i=0;i<count;++i)if(!ledger->memory().copy(0x1000+i*width,value.data(),value.size())){copied=false;break;}
    check(copied&&image.reads==count,"large replay makes no original memory access");
    if(changed)*image.at(0x1000+4097*width+15,1)^=1;
    check(changed?!ledger->seal()&&!ledger->published()&&ledger->issue()==Issue::changed:
      ledger->seal()&&ledger->published()&&image.reads==count*2,
      "every full span beyond the old capacity is reread and late changes still reject publication");
  }
  {Image image;fill(image,0x1000,batch::max_spans+1);Frames frame;auto ledger=std::make_unique<Ledger>();ledger->begin(image.memory(),frame.source(),frame.value,7);std::uint8_t v=0;
    for(std::size_t i=0;i<batch::max_spans;++i)check(ledger->memory().copy(0x1000+static_cast<std::uint32_t>(i),&v,1),"exact span capacity remains bounded and readable");
    check(!ledger->memory().copy(0x1000+static_cast<std::uint32_t>(batch::max_spans),&v,1)&&ledger->issue()==Issue::span_limit&&image.reads==batch::max_spans,"one additional span never widens or falls back");}
  {Image image;fill(image,0x1000,batch::byte_capacity+batch::max_span_bytes);Frames frame;auto ledger=std::make_unique<Ledger>();ledger->begin(image.memory(),frame.source(),frame.value,7);
    std::array<std::uint8_t,batch::max_span_bytes> out{};
    for(std::size_t i=0;i<batch::byte_capacity/batch::max_span_bytes;++i)check(ledger->memory().copy(0x1000+static_cast<std::uint32_t>(i*out.size()),out.data(),out.size()),"byte-capacity complete spans fit");
    check(!ledger->memory().copy(0x1000+static_cast<std::uint32_t>(batch::byte_capacity),out.data(),out.size())&&ledger->issue()==Issue::byte_limit,"extra byte storage never allocates dynamically");}
  for(const bool changed:{false,true}){
    // Test the exact byte limit with unequal request lengths. Neither
    // rounding nor another full 4096 block may mask an off-by-one error.
    Image image;fill(image,0x1000,batch::byte_capacity+1);Frames frame;auto ledger=std::make_unique<Ledger>();
    ledger->begin(image.memory(),frame.source(),frame.value,7);
    std::array<std::uint8_t,batch::max_span_bytes> out{};
    const auto copy=ledger->memory();std::size_t offset=0;
    while(offset<batch::byte_capacity){const auto n=std::min(out.size()-1,batch::byte_capacity-offset);
      check(copy.copy(0x1000+static_cast<std::uint32_t>(offset),out.data(),n),"mixed exact spans fill every budget byte");offset+=n;}
    const auto stats=ledger->statistics();
    check(stats.stored_bytes==batch::byte_capacity&&stats.spans<batch::max_spans&&ledger->freeze(),"byte edge can freeze independently of span edge");
    offset=0;
    while(offset<batch::byte_capacity){const auto n=std::min(out.size()-1,batch::byte_capacity-offset);
      check(copy.copy(0x1000+static_cast<std::uint32_t>(offset),out.data(),n),"full byte budget replays without source reads");offset+=n;}
    if(changed)*image.at(0x1000+static_cast<std::uint32_t>(batch::byte_capacity)-1,1)^=1;
    check(changed?!ledger->seal()&&ledger->issue()==Issue::changed:ledger->seal()&&ledger->published(),"last budget byte participates in final real verification");
    check(image.reads==stats.spans*2,"complete byte budget is really read twice including last span");
    ledger->begin(image.memory(),frame.source(),frame.value,7);offset=0;
    while(offset<batch::byte_capacity){const auto n=std::min(out.size(),batch::byte_capacity-offset);
      check(copy.copy(0x1000+static_cast<std::uint32_t>(offset),out.data(),n),"next frame admits complete max_span_bytes requests");offset+=n;}
    const auto reads=image.reads;out[0]=0xa5;
    check(!copy.copy(0x1000+static_cast<std::uint32_t>(batch::byte_capacity),out.data(),1)&&ledger->issue()==Issue::byte_limit&&
      image.reads==reads&&out[0]==0xa5,"one extra byte rejects before touching source or caller output");
    check(!ledger->freeze()&&!ledger->seal()&&!ledger->published()&&ledger->issue()==Issue::byte_limit,
      "capacity failure cannot publish a partial copy or overwrite its first failure");
    check(ledger->begin(image.memory(),frame.source(),frame.value,7)&&ledger->statistics().spans==0&&
      ledger->statistics().stored_bytes==0&&copy.copy(0x1000,out.data(),1)&&image.reads==reads+1,
      "explicit new frame clears capacity failure and rereads old addresses");
  }
  {Image image;fill(image,0x1000,8);Frames frame;auto ledger=std::make_unique<Ledger>();ledger->begin(image.memory(),frame.source(),frame.value,7);std::uint8_t value=0;
    check(ledger->memory().copy(0x1000,&value,1),"ABA first observation");*image.at(0x1000,1)^=1;*image.at(0x1000,1)^=1;
    check(ledger->freeze()&&ledger->memory().copy(0x1000,&value,1)&&ledger->seal(),"documented limit: unobserved A-B-A is not a foreign-write fence");}
  {Image image;fill(image,0x1000,1);Frames frame;auto ledger=std::make_unique<Ledger>();ledger->begin(image.memory(),frame.source(),frame.value,7);std::uint8_t value=0;
    bool copied=true;for(std::uint64_t i=0;i<batch::max_calls_per_pass;++i)copied=copied&&ledger->memory().copy(0x1000,&value,1);
    check(copied&&image.reads==1,"logical request loop is bounded without redundant original reads");
    check(!ledger->memory().copy(0x1000,&value,1)&&ledger->issue()==Issue::call_limit,"one additional logical request cannot exceed fixed pass limit");}
  {Image image;fill(image,0x1000,batch::max_span_bytes);Frames frame;auto ledger=std::make_unique<Ledger>();ledger->begin(image.memory(),frame.source(),frame.value,7);
    std::array<std::uint8_t,batch::max_span_bytes> value{};bool copied=true;
    for(std::uint64_t i=0;i<batch::max_bytes_per_pass/value.size();++i)copied=copied&&ledger->memory().copy(0x1000,value.data(),value.size());
    check(copied&&image.reads==1,"logical byte budget counts repeated own-copy reads too");
    check(!ledger->memory().copy(0x1000,value.data(),value.size())&&ledger->issue()==Issue::call_limit,"logical byte overflow is bounded before another read");}
  for(unsigned wrong=0;wrong<3;++wrong){Image image;fill(image,0x1000,1);Frames frame;auto ledger=std::make_unique<Ledger>();
    auto source=image.memory();auto observed=frame.value;if(wrong==0)source={};if(wrong==1)observed.writer=true;
    check(!ledger->begin(source,frame.source(),observed,wrong==2?8u:7u)&&image.reads==0,"missing reader, Writer scope or wrong owner never opens recording");}
}

void all_inventory(GrindFixture& f){
  std::vector<ubc::PackedInventoryTarget> targets;
  for(std::size_t i=0;i<profile::items.size();++i){const auto& p=profile::items[i];const auto id=static_cast<std::int32_t>(p.function->symbol);
    const auto object=input_object+static_cast<std::uint32_t>(i)*0x400u,node=0x11000u+static_cast<std::uint32_t>(i)*16u;
    f.item(object,id,5);f.image.block(node,12);f.image.put(node+4,object);f.image.put(node+8,i+1<profile::items.size()?node+16:0);targets.push_back({p.function->name,id});}
  std::sort(targets.begin(),targets.end(),[](const auto& a,const auto& b){return std::strcmp(a.name,b.name)<0;});
  for(std::size_t i=0;i<targets.size();++i)f.image.put(sorted+static_cast<std::uint32_t>(i)*4,static_cast<std::uint32_t>(targets[i].symbol));
}
struct Count {
  Image& image;Frames& frame;batch::Reader* batch_reader=nullptr;
  std::size_t calls=0,bytes=0;
  std::map<std::pair<std::uint32_t,std::size_t>,unsigned> spans;
  unsigned mode=0;std::uint32_t target=0;bool triggered=false;
  const run::Input* input=nullptr;const batch::Request* requests=nullptr;std::size_t request_count=0;
  static bool read(void* raw,std::uint32_t address,void* output,std::size_t size) noexcept {
    auto& c=*static_cast<Count*>(raw);++c.calls;c.bytes+=size;const auto occurrence=++c.spans[{address,size}];
    if(!c.triggered&&c.mode==1&&occurrence==2&&address<=c.target&&std::uint64_t{address}+size>c.target){c.triggered=true;*c.image.at(c.target,1)^=1;}
    if(!c.triggered&&c.mode==2&&occurrence==2){c.triggered=true;++c.frame.value.key.frame;}
    if(!c.triggered&&c.mode==3&&occurrence==2){c.triggered=true;return false;}
    if(!c.triggered&&c.mode==4){c.triggered=true;batch::Publication out;out.count=17;
      check(!c.batch_reader->publish(out)&&out.count==0,"no draft or older publication escapes during source read");}
    if(!c.triggered&&c.mode==5){c.triggered=true;
      check(!c.batch_reader->capture(c.memory(),c.frame.source(),c.frame.value,*c.input,c.requests,c.request_count),"nested whole batch poisons current admission");}
    return Image::read(&c.image,address,output,size);
  }
  ubc::ReadOnlyMemory memory(){return {this,read};}
};
std::array<batch::Request,batch::max_requests> requests(bool all){
  std::array<batch::Request,batch::max_requests> out{};
  if(all){for(std::size_t i=0;i<g::recipes.size();++i)out[i]={g::recipes[i].function.symbol,1};out.back()={g::recipes[0].function.symbol,3};}
  else{out[0]={g::recipes[0].function.symbol,1};out[1]={g::recipes[0].function.symbol,3};}
  return out;
}
// Counterexample to “430 nodes = known total requirement”. Actual
// local code contracts, but explicitly synthetic object addresses,
// unrelated symbols and world lists. No replay of a save or world run.
void capacity_shapes(const PE& exe,const Dat& dat,const PE& zbug,const PE& core,
    const PE& crt,const PE& zpe,const PE& renderer,const PE& locator){
  constexpr std::array<unsigned,4> quantities{{1,2,5,3}},stocks{{11,10,8,3}},outputs{{2,3,5,10}},skills{{3,4,6,11}};
  for(std::size_t scenario=0;scenario<quantities.size();++scenario){
    std::size_t empty_spans=0,empty_bytes=0;
    for(const unsigned world_nodes:{0u,3835u,7931u,12000u}){
      GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer,g::recipes[0],static_cast<std::int32_t>(stocks[scenario]),true,true);
      f.image.put(output_object+0x32c,outputs[scenario]);f.image.put(station_test::symbol(10505)+0x18,skills[scenario]);
      for(unsigned i=3;i<430;++i){const auto node=0x11000u+i*16,object=0x200000u+i*0x400;
        f.item(object,0,1);f.image.block(node,12);f.image.put(node+4,object);f.image.put(node+8,i+1<430?node+16:0);}
      f.image.put(0x11028,0x11030);
      // Both lists: measured counts 2576/1259, then 4096 additional nodes in list 0.
      const auto second=world_nodes?1259u:0u,first=world_nodes-second;
      for(unsigned list=0;list<2;++list){const auto count=list?second:first,base=0x50000000u+list*0x100000u;
        for(unsigned i=0;i<count;++i){const auto node=base+i*12;
          f.image.block(node,12);f.image.put(node+4,0x58000000u+list*0x100000u+i*4);f.image.put(node+8,i+1<count?node+12:0);}
        f.image.put(list?0x71008:0x70008,count?base:0u);}
      auto input=f.input();input.inventory_insert=install_locator(f,locator);
      f.image.put(f.modules.zbug.base+ubc::grind_zbug_unpack::thunk_rva+11,f.modules.zbug.base+ubc::grind_zbug_unpack::wrapper_rva);
      f.image.put(hero+0x724,0x40000);
      Frames frame;Count source{f.image,frame,nullptr,0,0,{}};const auto memory=source.memory();
      const auto catalogue=catalogue_reader.capture(memory,ubc::lhiver::game_parser,key(),true);
      const auto physical=ubc::read_grind_guild_for_preview(memory,ubc::lhiver::game_parser,key());
      const auto one=reader.capture_preview(memory,catalogue,physical,g::recipes[0].function.symbol,1,input);
      const auto selected=quantities[scenario]==1?one:
        reader.capture_preview(memory,catalogue,physical,g::recipes[0].function.symbol,quantities[scenario],input);
      check(one.item_path_verified&&selected.item_path_verified&&selected.stock.listed_nodes==430&&
        selected.world.nodes[0]==first&&selected.world.nodes[1]==second,
        "complete existing readers accept all four synthetic quantity paths including whole stack");
      std::size_t bytes=0,largest=0;for(const auto& entry:source.spans){bytes+=entry.first.second;largest=std::max(largest,entry.first.second);}
      if(!world_nodes){empty_spans=source.spans.size();empty_bytes=bytes;}
      check(source.spans.size()==empty_spans+world_nodes&&bytes==empty_bytes+12*world_nodes,
        "each foreign world node adds one distinct 12-byte span independently of quantity");
      check(largest<=batch::max_span_bytes&&source.calls<batch::max_calls_per_pass&&source.bytes<batch::max_bytes_per_pass,
        "world-shaped fixture isolates storage capacity from per-read and logical-work limits");
      auto bounded=std::make_unique<batch::Reader>(7,catalogue_reader,reader);
      auto publication=std::make_unique<batch::Publication>();
      const batch::Selection selection{{g::recipes[0].function.symbol,quantities[scenario]},true,true};
      auto measured=std::make_unique<ubc::grind_preview_measurement::Reader>(7,catalogue_reader,reader);
      check(measured->capture_selection(f.image.memory(),frame.source(),frame.value,input,selection)&&
        measured->diagnostic_result(*publication)&&batch::find(*publication,selection.current.callback,selection.current.quantity),
        "diagnostic evaluates the identical complete selection including whole-stack branches beyond old byte capacity");
      check(measured->statistics().spans==source.spans.size()&&measured->statistics().stored_bytes==bytes,
        "diagnostic range demand equals independent complete source-reader trace");
      check(!measured->publish(*publication)&&!publication->count,"diagnostic cannot publish into the regular preview path");
      const bool captured=bounded->capture_selection(f.image.memory(),frame.source(),frame.value,input,selection);
      if(world_nodes<12000)check(captured&&bounded->publish(*publication)&&batch::find(*publication,selection.current.callback,selection.current.quantity),
        "both measured-size world lists plus4096 nodes fit and publish all four selected quantity paths");
      else check(bytes>batch::byte_capacity&&source.spans.size()>batch::max_spans&&!captured&&
        (bounded->issue()==batch::Issue::byte_limit||bounded->issue()==batch::Issue::span_limit)&&
        !bounded->publish(*publication)&&publication->count==0,
        "same inventory totals with another valid world list exceed byte budget and publish nothing");
      std::printf("capacity_shape=synthetic quantity=%u inventory=430 world_nodes=%u spans=%zu stored=%zu largest=%zu calls=%zu logical_bytes=%zu\n",
        quantities[scenario],world_nodes,source.spans.size(),bytes,largest,source.calls,source.bytes);
    }
  }
}
void files(char** argv){
  const PE exe(argv[2],ubc::station_abi::executable_sha256),zbug(argv[4],ubc::zbug_end_compat::zbug_file_sha256),core(argv[5],ubc::zbug_end_compat::union_file_sha256);
  const PE crt(argv[6],run::dep::crt_sha256),zpe(argv[7],run::dep::parser_extender_sha256),renderer(argv[8],renderer_code::file_sha256),locator(argv[9],vi::locator_profile.file_sha256);const Dat dat(argv[3]);
  capacity_shapes(exe,dat,zbug,core,crt,zpe,renderer,locator);
  // Actual catalog/guild decision. No external selection snapshot taken in
  // advance. On both passes the same reader sees all ranges consumed by its copy.
  for(unsigned mode=0;mode<8;++mode){
    GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);Frames frame;
    auto input=f.input();auto prototype=std::make_unique<batch::Reader>(7,catalogue_reader,reader);
    batch::Selection selected{{g::recipes[0].function.symbol,3},true,true};
    if(mode==1)selected.current.quantity=1;
    if(mode==2)selected.current.quantity=6; // Fresh maximum of 5: no N6 path
    if(mode==3)f.image.put(input_object+0x158,ubc::grind_item_shape::find(static_cast<std::uint32_t>(g::recipes[0].input))->native_flags|0x40000000u);
    if(mode==4)f.image.put(hero+0x718,0); // Actually empty listed inventory source
    if(mode==5)f.image.put(station_test::symbol(10501)+0x18,0); // Actual unknown recipe knowledge
    if(mode==6)selected.station_path_verified=false;
    if(mode==7)selected.hero_dialog_verified=false;
    Count source{f.image,frame,nullptr,0,0,{}};
    auto out=std::make_unique<batch::Publication>();
    check(prototype->capture_selection(source.memory(),frame.source(),frame.value,input,selected)&&prototype->publish(*out),
      "both private passes deterministically choose candidates from actual catalogue guild hand and inventory values");
    const auto& row=out->snapshot.recipes[ubc::crafting_catalog::grind_begin];
    check(ubc::valid_crafting_snapshot(out->snapshot)&&out->scope.input_serial==frame.value.input_serial,
      "sealed projection carries actual same-frame scope, including complete blocked catalogue");
    const auto& stats=prototype->statistics();
    check(source.calls==2*stats.spans&&stats.recording_calls==stats.replay_calls&&stats.first_reads==stats.final_reads,
      "selection policy makes no pre-capture and always rereads every consumed full source span at seal");
    if(mode<3){
      check(row.known&&row.maximum==5&&row.material_verified&&out->count==(mode?1u:2u),
        "known recipe keeps native maximum; N1 is not duplicated and impossible N6 is skipped");
      const auto n=mode==0?3u:1u;
      ubc::CraftingRequest request{row.id,n,out->snapshot.key,row,out->snapshot.hands};
      check(batch::quantity_matches(*out,request),"same closed N1 or selected-N evidence supplies genuine quantity comparison");
      request.quantity=mode==0?4u:2u;check(!batch::quantity_matches(*out,request),"unrecorded quantity cannot reuse neighboring amount proof");
      request.quantity=n;++request.key.hero;check(!batch::quantity_matches(*out,request),"changed physical actor cannot consume closed quantity");
    }else{
      check(!row.material_verified&&row.maximum==0&&!batch::find(*out,g::recipes[0].function.symbol,3),
        "missing ingredient knowledge or bound station never manufactures an item or amount proof");
      if(mode==3){
        check(out->count==1&&out->blocked[0].blocked&&!out->items[0].item_path_verified&&
          out->items[0].key.hero==0&&row.known,"same negative item phase publishes only blocked row, no partial evidence");
        const auto negative=out->blocked[0];auto wrong=negative;wrong.address++;
        check(batch::same_blocked(negative,negative)&&!batch::same_blocked(negative,wrong),"negative address is part of exact stable rejection");
        wrong=negative;wrong.phase=run::Issue::none;check(!batch::same_blocked(negative,wrong),"negative phase cannot be rewritten into success");
        wrong=negative;wrong.inventory=run::path::Issue::none;check(!batch::same_blocked(negative,wrong),"nested negative inventory phase also binds");
      }else check(out->count==0,"zero matching requests is valid and never starts an arbitrary item reader");
      if(mode!=5){ubc::CraftingSession session;check(session.open(out->snapshot,row.id)&&session.visible_count()>1&&
          session.selected()&&session.selected()->id==row.id&&session.quantity()==0&&!session.can_confirm(),"blocked known row remains visible and selectable");
        session.observe(20,false,true);session.observe(21,false,true);session.observe(22,false,true);
        check(session.select_relative(1)&&!session.can_confirm(),"user may select another blocked row without enabling an effect");}
    }
    prototype->close();out->count=23;
    check(!prototype->publish(*out)&&out->count==0,"ending lexical reader ownership retires stored callback and publication");
  }
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);all_inventory(f);Frames frame;auto input=f.input();
    const auto guild=g::recipes[0].guilds[0];std::size_t bad=g::recipes.size();
    for(std::size_t i=1;i<g::recipes.size();++i)if(g::recipes[i].guilds[0]==guild||g::recipes[i].guilds[1]==guild){bad=i;break;}
    check(bad<g::recipes.size(),"real guild has another eligible profile for independent blocked-row matrix");
    if(bad<g::recipes.size())for(std::size_t j=0;j<profile::items.size();++j)if(profile::items[j].function->symbol==static_cast<std::uint32_t>(g::recipes[bad].input))
      f.image.put(input_object+static_cast<std::uint32_t>(j)*0x400+0x158,profile::items[j].function->symbol?0x40000000u:0);
    auto prototype=std::make_unique<batch::Reader>(7,catalogue_reader,reader);auto out=std::make_unique<batch::Publication>();
    batch::Selection selected{{g::recipes[0].function.symbol,3},true,true};
    check(prototype->capture_selection(f.image.memory(),frame.source(),frame.value,input,selected)&&prototype->publish(*out),
      "one blocked input and other complete positive paths publish atomically in the same sealed catalogue");
    const auto& good=out->snapshot.recipes[ubc::crafting_catalog::grind_begin];
    check(good.material_verified&&good.maximum==5&&batch::find(*out,g::recipes[0].function.symbol,3),"other valid selected item remains available");
    if(bad<g::recipes.size())check(out->snapshot.recipes[ubc::crafting_catalog::grind_begin+bad].known&&
      !out->snapshot.recipes[ubc::crafting_catalog::grind_begin+bad].material_verified&&
      !batch::find(*out,g::recipes[bad].function.symbol,1),"bad item never borrows a neighboring row's positive proof");
    std::size_t expected=1;for(const auto& recipe:g::recipes)if(recipe.guilds[0]==guild||recipe.guilds[1]==guild)++expected;
    check(out->count==expected,"requests preserve actual guild-filtered policy, not every profile unconditionally");
  }
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);Frames frame;auto input=f.input();
    auto prototype=std::make_unique<batch::Reader>(7,catalogue_reader,reader);auto out=std::make_unique<batch::Publication>();
    batch::Selection selected{{g::recipes[0].function.symbol,5},true,true};
    check(prototype->capture_selection(f.image.memory(),frame.source(),frame.value,input,selected)&&prototype->publish(*out),"initial N5 frame captured");
    ubc::CraftingSession session;const auto id=out->snapshot.recipes[ubc::crafting_catalog::grind_begin].id;
    check(session.open(out->snapshot,id),"actual closed projection opens same existing session policy");
    session.observe(10,false,true);session.observe(11,false,true);session.observe(12,false,true);
    check(session.preset(5)&&session.quantity()==5,"real preset selects existing N5");
    f.image.put(input_object+0x32c,2);++frame.value.key.frame;++frame.value.scope_serial;++frame.value.input_serial;input.boundary.key=frame.value.key;
    check(prototype->capture_selection(f.image.memory(),frame.source(),frame.value,input,selected)&&prototype->publish(*out)&&out->count==1,
      "new lower stock skips stale N5 but retains freshly real-read N1");
    session.observe(13,false,true);
    check(!session.refresh(out->snapshot)&&session.quantity()==1&&session.maximum()==2,
      "real Session refresh resets changed stock to N1; no new quantity policy is invented");
    const ubc::CraftingRequest request{id,session.quantity(),session.preview().key,*session.selected(),session.preview().hands};
    check(batch::quantity_matches(*out,request)&&session.observe_quantity_check(request,13,true),
      "reset quantity consumes same jointly closed N1 proof without another real catalogue or item read");
    auto stale=request;stale.quantity=5;check(!batch::quantity_matches(*out,stale),"prior larger amount remains rejected after clamp/reset");
  }
  // The negative row itself must remain stable at the actual final comparison.
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);Frames frame;auto input=f.input();
    f.image.put(input_object+0x158,0x40000000u);
    auto prototype=std::make_unique<batch::Reader>(7,catalogue_reader,reader);
    Count source{f.image,frame,prototype.get(),0,0,{},1,input_object+0x158};
    const batch::Selection selected{{g::recipes[0].function.symbol,1},true,true};
    check(!prototype->capture_selection(source.memory(),frame.source(),frame.value,input,selected)&&source.triggered&&prototype->issue()==batch::Issue::changed,
      "even identical private negative outcomes cannot survive changed real end bytes");
    auto out=std::make_unique<batch::Publication>();check(!prototype->publish(*out),"changed negative row withholds entire catalogue");
  }
  for(unsigned variant=0;variant<3;++variant){const bool all=variant!=0;GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);if(all)all_inventory(f);
    auto input=f.input();if(variant==2){input.inventory_insert=install_locator(f,locator);f.image.put(f.modules.zbug.base+ubc::grind_zbug_unpack::thunk_rva+11,f.modules.zbug.base+ubc::grind_zbug_unpack::wrapper_rva);f.image.put(hero+0x724,0x40000);}
    Frames frame;const auto requested=requests(all);const auto count=all?batch::max_requests:2;
    Count baseline{f.image,frame,nullptr,0,0,{},0,0,false,nullptr,nullptr,0};const auto live=baseline.memory();
    const auto c=catalogue_reader.capture(live,ubc::lhiver::game_parser,key(),true);const auto guild=ubc::read_grind_guild_for_preview(live,ubc::lhiver::game_parser,key());
    std::array<run::Evidence,batch::max_requests> expected{};
    for(std::size_t i=0;i<count;++i){expected[i]=reader.capture_preview(live,c,guild,requested[i].callback,requested[i].quantity,input);
      check(run::matches(expected[i],key(),requested[i].callback,requested[i].quantity,run::path::Scope::preview),"baseline uses complete real pinned runtime readers");}
    auto prototype=std::make_unique<batch::Reader>(7,catalogue_reader,reader);
    Count captured{f.image,frame,prototype.get(),0,0,{},4,0,false,nullptr,nullptr,0};
    check(prototype->capture(captured.memory(),frame.source(),frame.value,input,requested.data(),count),"two own evaluations and complete second real copy admit entire batch");
    auto publication=std::make_unique<batch::Publication>();check(prototype->publish(*publication)&&publication->count==count,"only sealed complete batch is published as own values");
    for(std::size_t i=0;i<count;++i){check(batch::detail::same_item_result(expected[i],publication->items[i]),"published item proof equals complete baseline reader evidence");
      check(!run::matches(publication->items[i],key(),requested[i].callback,requested[i].quantity,run::path::Scope::writer),"published preview can never substitute writer SELF");}
    const auto& s=prototype->statistics();
    check(captured.calls==2*s.spans&&captured.bytes==2*s.stored_bytes&&s.first_reads==s.final_reads,"every exact source range is actually read twice, without source reads during replay");
    check(s.recording_calls==baseline.calls&&s.recording_bytes==baseline.bytes&&s.replay_calls==baseline.calls&&s.replay_bytes==baseline.bytes,"both complete evaluations consume identical baseline request sequence");
    check(captured.calls<baseline.calls&&captured.bytes<baseline.bytes,"real original-reader counts and byte totals are lower, no timing claim");
    std::printf("variant=%u baseline_reads=%zu baseline_bytes=%zu first=%llu/%llu final=%llu/%llu spans=%zu stored=%zu overlaps=%zu logical=%llu replay=%llu reader_workspace=%zu\n",variant,baseline.calls,baseline.bytes,
      static_cast<unsigned long long>(s.first_reads),static_cast<unsigned long long>(s.first_bytes),static_cast<unsigned long long>(s.final_reads),static_cast<unsigned long long>(s.final_bytes),s.spans,s.stored_bytes,s.overlaps,
      static_cast<unsigned long long>(s.recording_calls),static_cast<unsigned long long>(s.replay_calls),sizeof(batch::Reader));
    ++frame.value.key.frame;check(!prototype->publish(*publication)&&publication->count==0,"even a successful copy is not published in another current frame");
  }
  const std::array<std::uint32_t,9> changed{{input_object+0x32c,hero+0x728,hero+0x230,0x31030,0x6e8200,0x83c758,renderer_base+0x7ec00,station_test::symbol(10505)+0x18,0x8d8780}};
  for(const auto address:changed){GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);Frames frame;auto prototype=std::make_unique<batch::Reader>(7,catalogue_reader,reader);
    const auto wanted=requests(false);auto input=f.input();Count c{f.image,frame,prototype.get(),0,0,{},1,address,false,nullptr,nullptr,0};
    check(!prototype->capture(c.memory(),frame.source(),frame.value,input,wanted.data(),2)&&c.triggered&&prototype->issue()==batch::Issue::changed,"late stock/pack/guild/slot/native/IAT/renderer/skill/pool mutation discards ALL evidence");
    auto out=std::make_unique<batch::Publication>();out->count=23;check(!prototype->publish(*out)&&out->count==0,"failed batch publishes no partial prior successes");
  }
  for(const unsigned mode:{2u,3u,5u}){GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);Frames frame;auto prototype=std::make_unique<batch::Reader>(7,catalogue_reader,reader);
    const auto wanted=requests(false);auto input=f.input();Count c{f.image,frame,prototype.get(),0,0,{},mode,0,false,&input,wanted.data(),2};
    check(!prototype->capture(c.memory(),frame.source(),frame.value,input,wanted.data(),2)&&c.triggered,"late current-frame switch/read failure or recursive capture rejects whole batch");
    auto out=std::make_unique<batch::Publication>();check(!prototype->publish(*out),"failed boundary cannot issue publication");}
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);Frames frame;auto prototype=std::make_unique<batch::Reader>(7,catalogue_reader,reader);auto input=f.input();input.modules.zbug.mapping_verified=false;const auto wanted=requests(false);
    auto out=std::make_unique<batch::Publication>();
    check(prototype->capture(f.image.memory(),frame.source(),frame.value,input,wanted.data(),2)&&prototype->publish(*out)&&out->blocked[0].blocked&&
      !batch::find(*out,wanted[0].callback,1)&&!out->items[0].item_path_verified,"wrong genuine module binding can publish only identical explicit blocked outcomes");}
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);Frames frame;auto prototype=std::make_unique<batch::Reader>(7,catalogue_reader,reader);auto input=f.input();const auto wanted=requests(false);
    f.image.put(f.modules.zbug.base+ubc::grind_zbug_unpack::thunk_rva+11,f.modules.zbug.base+ubc::grind_zbug_unpack::wrapper_rva);f.image.put(hero+0x724,0x40000);
    Count c{f.image,frame,prototype.get(),0,0,{},1,0x40000,false,nullptr,nullptr,0};
    check(!prototype->capture(c.memory(),frame.source(),frame.value,input,wanted.data(),2)&&c.triggered&&prototype->issue()==batch::Issue::changed,"actual nonnull empty-Pack NUL is included in final whole-span verification");}
  {GrindFixture f(exe,dat,zbug,core,crt,zpe,renderer);Frames frame;auto prototype=std::make_unique<batch::Reader>(7,catalogue_reader,reader);auto input=f.input();const auto wanted=requests(false);
    auto previous=std::make_unique<batch::Publication>(),next=std::make_unique<batch::Publication>();
    check(prototype->capture(f.image.memory(),frame.source(),frame.value,input,wanted.data(),2)&&prototype->publish(*previous),"initial complete frame is publishable");
    f.image.put(input_object+0x32c,4);++frame.value.key.frame;++frame.value.scope_serial;input.boundary.key=frame.value.key;
    check(!prototype->publish(*next),"old seal cannot enter a new outer scope");
    const auto before=f.image.reads;
    check(prototype->capture(f.image.memory(),frame.source(),frame.value,input,wanted.data(),2)&&prototype->publish(*next)&&f.image.reads>before,"next explicit frame clears recorder and performs two new real copies");
    check(next->items[1].stock.materials[0].listed==4&&previous->items[1].stock.materials[0].listed==5&&
      !run::matches(previous->items[1],frame.value.key,wanted[1].callback,3,run::path::Scope::preview),"owned old copies remain historical values, never current-frame authority");
    for(unsigned mode=0;mode<6;++mode){auto wrong=wanted;auto bad_input=input;std::size_t count=2;
      if(mode==0)wrong[0].callback=1;if(mode==1)wrong[0].quantity=0;if(mode==2)count=batch::max_requests+1;
      if(mode==3)bad_input.boundary.stable_boundary_verified=false;if(mode==4)++bad_input.boundary.key.hero;if(mode==5)bad_input.boundary.current_thread=8;
      const auto reads=f.image.reads;
      check(!prototype->capture(f.image.memory(),frame.source(),frame.value,bad_input,wrong.data(),count)&&f.image.reads==reads&&!prototype->publish(*next)&&!next->count,"wrong request/context invalidates former result before any source access");}
  }
}
}
int main(int argc,char** argv){
  using namespace preview_test;low_level();measurement();measured_capacity();
  static_assert(sizeof(batch::Reader)<=768u*1024u,"fixed preview reader including both result copies stays below 768 KiB");
  static_assert(sizeof(std::optional<batch::Reader>)+sizeof(batch::Publication)<=896u*1024u,
    "reader AND separately published global copy remain below 896 KiB");
  std::printf("capacity_contract spans=%zu bytes=%zu max_span=%zu reader=%zu publication=%zu persistent_total=%zu calls_per_pass=%llu bytes_per_pass=%llu overlap_checks_max=%llu\n",
    batch::max_spans,batch::byte_capacity,batch::max_span_bytes,sizeof(batch::Reader),sizeof(batch::Publication),
    sizeof(std::optional<batch::Reader>)+sizeof(batch::Publication),static_cast<unsigned long long>(batch::max_calls_per_pass),
    static_cast<unsigned long long>(batch::max_bytes_per_pass),static_cast<unsigned long long>(batch::max_spans)*(batch::max_spans-1)/2);
  check(!batch::Publication::native_effect_authorized,"prototype publication is not effect admission");
  if(argc==10&&std::string(argv[1])=="--local-files")files(argv);
  else if(argc!=1)return 2;
  std::printf("grind_preview_batch_tests: %u checks, %u failures\n",station_test::checks,station_test::failures);
  return station_test::failures?1:0;
}
