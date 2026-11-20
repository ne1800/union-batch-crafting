// SPDX-License-Identifier: MIT
#include "ubc/selection_snapshot.hpp"
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
unsigned checks=0,failures=0;
void check(bool ok,const char* name) {
  ++checks; if(!ok) {++failures;std::fprintf(stderr,"FAIL: %s\n",name);}
}
struct Image {
  std::vector<std::uint8_t> bytes=std::vector<std::uint8_t>(0x10000);
  std::uint32_t denied=0;
  void put(std::uint32_t address,std::uint32_t value) {
    for(unsigned i=0;i<4;++i)bytes[address+i]=static_cast<std::uint8_t>(value>>(i*8));
  }
  static bool read(void* ctx,std::uint32_t address,void* out,std::size_t size) noexcept {
    auto& self=*static_cast<Image*>(ctx);
    if(address>self.bytes.size()||size>self.bytes.size()-address ||
       (self.denied && address<=self.denied && size>self.denied-address))return false;
    std::memcpy(out,self.bytes.data()+address,size);return true;
  }
  ubc::ReadOnlyMemory memory(){return {this,read};}
};
constexpr std::uint32_t manager=0x1000,view=0x2000,info=0x3000;
void base(Image& m) {
  m.put(manager+0x1c,view);m.put(manager+0x20,0x5000);m.put(manager+0x24,0x5000);
  m.put(manager+0x40,1);m.put(manager+0x54,1);
  m.put(view,0x83bc3c);m.put(view+0x24,0x83bc10);m.put(view+0xec,0x83bc08);
  m.put(view+0x7c,1);m.put(view+0xf4,1);m.put(view+0x104,4);
  m.put(info,0x83c44c);m.put(info+0x1c,22243);m.put(info+0x20,1);
  m.put(info+0x28,21009);m.put(info+0x2c,21010);m.put(info+0x48,1);m.put(info+0x50,21008);
}
}
int main() {
  Image image;base(image);
  auto d=ubc::read_selection_dialog(image.memory(),manager);
  check(d.valid&&d.waiting&&d.open&&!d.done&&!d.hidden&&d.count==4,"native ready selection decoded");
  auto i=ubc::read_selection_info(image.memory(),info);
  check(ubc::selection_info_link(i,ubc::selection_profile::info_links[0]),"native C_INFO links exact known condition and effect");
  for(const unsigned offset:{0u,0x1cu,0x20u,0x24u,0x28u,0x2cu,0x44u,0x48u,0x50u}) {
    Image m;base(m);m.bytes[info+offset]^=1;
    check(!ubc::selection_info_link(ubc::read_selection_info(m.memory(),info),
                                  ubc::selection_profile::info_links[0]),"changed link field cannot gain recipe permission");
  }
  for(const unsigned offset:{0u,0x24u,0xecu}) {
    Image m;base(m);m.put(view+offset,0);
    check(!ubc::read_selection_dialog(m.memory(),manager).valid,"wrong native subobject type rejected");
  }
  for(const unsigned offset:{0x2cu,0x30u,0x34u,0x38u,0x3cu,0x44u}) {
    Image m;base(m);m.put(manager+offset,1);
    check(!ubc::read_selection_dialog(m.memory(),manager).waiting,"busy native manager not borrowed");
  }
  for(const unsigned offset:{0x84u,0x88u}) {
    Image m;base(m);m.put(view+offset,1);
    check(!ubc::read_selection_dialog(m.memory(),manager).open,"even tiny unfinished animation not ready");
  }
  for(const auto count:{0u,257u,0xffffffffu}) {
    Image m;base(m);m.put(view+0x104,count);
    check(!ubc::read_selection_dialog(m.memory(),manager).valid,"invalid row count/index rejected");
  }
  for(const auto selection:{4u,0xfffffffeu}) {
    Image m;base(m);m.put(view+0x100,selection);
    check(!ubc::read_selection_dialog(m.memory(),manager).valid,"invalid selected index rejected");
  }
  image.put(view+0x7c,0);image.put(view+0x80,1);image.put(view+0xf4,0);
  d=ubc::read_selection_dialog(image.memory(),manager);
  check(d.valid&&d.waiting&&d.hidden&&!d.open&&!d.done,"own hidden view remains native waiting selection");
  image.put(view+0xf0,1);
  d=ubc::read_selection_dialog(image.memory(),manager);
  check(d.done&&!d.hidden,"native confirmation is distinct from hiding");
  check(!d.rejected_info_transition,"still-waiting confirmation is not the recovery boundary");
  image.put(manager+0x40,0);
  check(ubc::read_selection_dialog(image.memory(),manager).rejected_info_transition,
        "closed completed selection with all manager waits clear permits guarded recovery");
  for(const unsigned offset:{0x2cu,0x30u,0x34u,0x38u,0x3cu,0x40u,0x44u}) {
    for(const auto value:{1u,0xffffffffu}) {
      image.put(manager+offset,value);
      check(!ubc::read_selection_dialog(image.memory(),manager).rejected_info_transition,
            "set or unreadable-style manager flag cannot permit recovery");
      image.put(manager+offset,0);
    }
  }
  image.put(manager+0x54,2);
  check(!ubc::read_selection_dialog(image.memory(),manager).rejected_info_transition,
        "choice mode must use its own native recovery path");
  for(const auto denied:{manager,view,info}) {
    Image m;base(m);m.denied=denied;
    if(denied==info)check(!ubc::read_selection_info(m.memory(),info).valid,"unreadable info rejected");
    else check(!ubc::read_selection_dialog(m.memory(),manager).valid,"unreadable dialog rejected");
  }
  {
    Image m;base(m);auto dialog=ubc::read_selection_dialog(m.memory(),manager);
    ubc::SelectionInfoRows rows;
    rows.begin(view,true);const auto generation=rows.generation;
    for(int n=0;n<4;++n){auto item=i;item.instance+=n*3;item.function+=n*3;rows.append(view,item,n,n+1,true);}
    check(rows.complete(dialog),"observed native add sequence maps visible indices");
    check(rows.rows[2].instance==21014&&rows.rows[2].function==21016,"third original row maps independently of selected recipe");
    rows.begin(view,false);
    check(rows.generation>generation&&!rows.complete(dialog),"any clear invalidates earlier mapping");
    rows.append(view,i,0,1,true);
    check(!rows.valid,"unknown clear cannot be repaired by one later add");
    for(unsigned variant=0;variant<6;++variant){
      rows.begin(view,true);
      rows.append(view,i,0,1,true);
      auto other=i;other.instance+=3;
      switch(variant){
        case 0:rows.append(view+4,other,1,2,true);break;
        case 1:rows.append(view,other,2,3,true);break;
        case 2:rows.append(view,other,1,3,true);break;
        case 3:rows.append(view,other,1,2,false);break;
        case 4:rows.append(view,i,1,2,true);break;
        case 5:rows.append(view,{},1,2,true);break;
      }
      check(!rows.valid&&!rows.complete(dialog),"invalid collection never completes");
    }
    rows.generation=UINT64_MAX;rows.begin(view,true);
    check(!rows.valid&&rows.generation==UINT64_MAX,"generation exhaustion never wraps");
  }
  for(unsigned bad=0;bad<7;++bad){
    Image m;base(m);m.put(manager+0x54,2);m.put(manager+0x28,info);
    m.put(view+0x104,3);m.put(info+0x58,0x4000);
    for(unsigned n=0;n<3;++n){
      const auto node=0x4000+n*8;const auto choice=0x6000+n*0x20;
      m.put(node,choice);m.put(node+4,n==2?0:node+8);m.put(choice+0x14,20341+n);
    }
    switch(bad){
      case 1:m.put(0x4014,0x4000);break;
      case 2:m.put(0x4008,0x6000);break;
      case 3:m.put(0x6014,0xffffffff);break;
      case 4:m.put(view+0x104,4);break;
      case 5:m.denied=0x6014;break;
      case 6:m.put(info,0);break;
    }
    const auto dialog=ubc::read_selection_dialog(m.memory(),manager);
    const auto choices=ubc::read_selection_choices(m.memory(),dialog);
    if(!bad)check(choices.valid&&choices.count==3&&choices.functions[2]==20343,
                  "fresh complete choice list matches native order");
    else check(!choices.valid,"corrupt or incomplete choice list rejected");
  }
  std::printf("SelectionSnapshot: %u checks, %u failures\n",checks,failures);
  return failures?1:0;
}
