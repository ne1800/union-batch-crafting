// SPDX-License-Identifier: MIT
#include "ubc/core/post_remove_choice.hpp"
#include <cstdio>
#include <cstring>

namespace {
unsigned checks=0,failures=0;
void check(bool value,const char* name){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",name);}}
struct Fixture {
  static constexpr std::uint32_t base=0x10000,owner=base,view=base+0x100;
  std::array<std::uint8_t,0x9000> bytes{};
  ubc::SelectionDialog dialog;
  std::uint32_t forbidden_node=0,forbidden_choice=0;
  unsigned freed_reads=0;
  bool change_on_second_owner=false;
  unsigned owner_reads=0;
  bool replace_choice_on_second_node=false;
  unsigned first_node_reads=0;
  static std::uint32_t node(std::size_t i){return base+0x1000+static_cast<std::uint32_t>(i)*16;}
  static std::uint32_t choice(std::size_t i){return base+0x3000+static_cast<std::uint32_t>(i)*32;}
  void put(std::uint32_t address,std::uint32_t value){std::memcpy(bytes.data()+address-base,&value,4);}
  static bool copy(void* raw,std::uint32_t address,void* out,std::size_t size) noexcept {
    auto& f=*static_cast<Fixture*>(raw);
    if((f.forbidden_node&&address==f.forbidden_node)||(f.forbidden_choice&&address==f.forbidden_choice)){
      ++f.freed_reads;return false;
    }
    if(address<base||address-base>f.bytes.size()||size>f.bytes.size()-(address-base))return false;
    if(address==owner&&++f.owner_reads==2&&f.change_on_second_owner)f.put(owner+0x50,9999);
    if(address==node(0)&&f.replace_choice_on_second_node&&++f.first_node_reads==2)
      f.put(node(0),f.forbidden_choice);
    std::memcpy(out,f.bytes.data()+address-base,size);return true;
  }
  ubc::ReadOnlyMemory memory(){return {this,copy};}
  explicit Fixture(std::size_t count=3,std::int32_t row=1){
    put(owner,ubc::info_detail::info_vtable);put(owner+0x1c,22243);put(owner+0x20,2);
    put(owner+0x28,20345);put(owner+0x2c,20346);put(owner+0x48,1);put(owner+0x50,20344);
    put(owner+0x58,count?node(0):0);
    for(std::size_t i=0;i<count;++i){
      put(node(i),choice(i));put(node(i)+4,i+1<count?node(i+1):0);
      put(choice(i)+0x14,20349+static_cast<std::uint32_t>(i));
    }
    dialog.valid=true;dialog.done=true;dialog.view=view;dialog.info=owner;dialog.npc=11;dialog.player=11;
    dialog.mode=2;dialog.selected=row;dialog.count=static_cast<std::int32_t>(count);
  }
  std::int32_t selected_function() const{return 20349+dialog.selected;}
  bool begin(ubc::PostRemoveChoice& witness){return witness.begin(memory(),dialog,7,9,
    choice(static_cast<std::size_t>(dialog.selected)),selected_function());}
  void remove(std::size_t row){
    const auto next=row+1<static_cast<std::size_t>(dialog.count)?node(row+1):0;
    if(row)put(node(row-1)+4,next);else put(owner+0x58,next);
    forbidden_node=node(row);forbidden_choice=choice(row);
  }
  bool consume(ubc::PostRemoveChoice& witness){return witness.consume(memory(),dialog,7,9,selected_function());}
};
}
int main(){
  for(const auto count:{1u,2u,3u,256u})for(std::uint32_t row=0;row<count;++row){
    Fixture f(count,static_cast<std::int32_t>(row));ubc::PostRemoveChoice witness;
    check(f.begin(witness)&&witness.active(),"exact complete pre-Original list is bound");
    check(!f.begin(witness),"nested begin cannot replace a live removal witness");
    f.remove(row);
    check(f.consume(witness),"one selected row may disappear while old view count and collection remain");
    check(!f.freed_reads,"removed native node and choice are never dereferenced");
    check(!f.consume(witness),"writer witness is single-use");
    witness.clear();check(!witness.active()&&!f.consume(witness),"returned wrapper retires witness");
  }
  for(unsigned mode=0;mode<23;++mode){
    Fixture f;ubc::PostRemoveChoice witness;check(f.begin(witness),"negative post-remove case starts");
    if(mode!=0)f.remove(1);
    auto dialog=f.dialog;std::uint64_t collection=7,ticket=9;auto function=f.selected_function();
    switch(mode){
      case 0:break; // Nothing removed.
      case 1:f.put(Fixture::node(0)+4,0);break; // Two removed.
      case 2:f.put(Fixture::owner+0x58,Fixture::node(1));break; // Wrong or stale row.
      case 3:f.put(Fixture::node(0),Fixture::choice(2));break;
      case 4:f.put(Fixture::choice(0)+0x14,123);break;
      case 5:f.put(Fixture::node(2)+4,Fixture::node(0));break;
      case 6:f.put(Fixture::owner+0x50,123);break;
      case 7:f.put(Fixture::owner+0x28,123);break;
      case 8:f.put(Fixture::owner+0x2c,123);break;
      case 9:++dialog.view;break;
      case 10:++dialog.info;break;
      case 11:++dialog.npc;break;
      case 12:++dialog.player;break;
      case 13:dialog.mode=1;break;
      case 14:--dialog.count;break;
      case 15:--dialog.selected;break;
      case 16:dialog.done=false;break;
      case 17:dialog.valid=false;break;
      case 18:++collection;break;
      case 19:++ticket;break;
      case 20:++function;break;
      case 21:f.put(Fixture::node(0),Fixture::choice(1));break; // Freed choice pointer in an unrelated node.
      case 22:dialog.waiting=true;break;
    }
    check(!witness.consume(f.memory(),dialog,collection,ticket,function),"only exact native removal may authorize writer");
    check(!f.consume(witness),"failed postcondition cannot be retried with repaired state");
    check(!f.freed_reads,"malformed remaining list cannot cause reads of the removed object");
  }
  for(unsigned mode=0;mode<16;++mode){
    Fixture f;ubc::PostRemoveChoice witness;auto dialog=f.dialog;
    std::uint64_t collection=7,ticket=9;auto selected=Fixture::choice(1);auto function=f.selected_function();
    switch(mode){
      case 0:dialog.valid=false;break;case 1:dialog.done=false;break;case 2:dialog.mode=1;break;
      case 3:dialog.count=0;break;case 4:dialog.count=257;break;case 5:dialog.selected=-1;break;
      case 6:dialog.selected=3;break;case 7:collection=0;break;case 8:ticket=0;break;
      case 9:selected=Fixture::choice(2);break;case 10:++function;break;
      case 11:f.put(Fixture::node(2)+4,Fixture::node(0));break;
      case 12:f.put(Fixture::node(2),Fixture::choice(0));break;
      case 13:f.put(Fixture::choice(1)+0x14,UINT32_MAX);break;
      case 14:dialog.info=0;break;case 15:f.change_on_second_owner=true;break;
    }
    check(!witness.begin(f.memory(),dialog,collection,ticket,selected,function)&&!witness.active(),
      "unknown, unstable or mismatched precondition never arms a witness");
  }
  {
    Fixture f;ubc::PostRemoveChoice witness;
    f.put(Fixture::choice(0)+0x14,static_cast<std::uint32_t>(f.selected_function()));
    check(f.begin(witness),"equal functions in different nodes do not erase object identity");
    f.remove(0);
    check(!f.consume(witness),"removing an equal-function neighbour cannot impersonate removal of selected object");
  }
  {
    Fixture f;ubc::PostRemoveChoice witness;check(f.begin(witness),"late link mutation case starts");
    f.remove(1);f.replace_choice_on_second_node=true;f.first_node_reads=0;
    check(!f.consume(witness),"remaining node changed during comparison cannot confirm native removal");
    check(!f.freed_reads,"comparison pass rejects removed Choice identity before trying to read its memory");
  }
  std::printf("PostRemoveChoice: %u checks, %u failures\n",checks,failures);
  return failures?1:0;
}
