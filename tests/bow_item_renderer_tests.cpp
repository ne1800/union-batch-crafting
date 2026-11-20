// SPDX-License-Identifier: MIT
#include "ubc/bow_item_renderer.hpp"
#include "bow_item_test_values.hpp"

namespace {
using namespace bow_item_test;namespace r=ubc::bow_item_renderer;
constexpr std::uint32_t base=0x76030000,object=0x100000,renderer=0x200000,device=0x300000;
constexpr std::uint32_t sentinel=0x14000000,buckets=0x14100000,head=0x14200000;
r::Module module(){return {base,r::image_size,r::file_sha256,true};}
r::ObjectRequest request(){return {{{1,7,0x8000,0x2000,0x8800,0x9000,16},8,7,true},object,16610};}
r::Reader reader(7);
struct Fixture {
  Image image;
  Fixture(const PE& exe,const PE& renderer_pe){
    renderer_pe.load(image,base);
    put(image,ubc::selection_abi::game_pointer,0x18000);put(image,ubc::station_abi::hero_pointer,0x2000);
    image.block(0x18000,16);image.put(0x18000,0x83c28c);image.put(0x18008,0x8000);put(image,0x8000,0x83e174);
    image.block(0x2000,0x1000);image.put(0x2964,0x8800);
    for(std::size_t i=0;i<r::detail::hooks.size();++i){const auto& h=r::detail::hooks[i];const auto& area=ubc::bow_item_native::engine_ranges[15+i];
      exe.copy(image,area.address,area.length);*image.at(h.entry,1)=0xe9;image.put(h.entry+1,base+h.rva-h.entry-5u);
      for(std::uint32_t j=5;j<h.length;++j)*image.at(h.entry+j,1)=0x90;
      const auto trampoline=0x5000000u+static_cast<std::uint32_t>(i)*0x100u;image.block(trampoline,h.length+5);
      std::memcpy(image.at(trampoline,h.length),h.prologue.data(),h.length);*image.at(trampoline+h.length,1)=0xe9;
      image.put(trampoline+h.length+1,h.entry+h.length-(trampoline+h.length+5));image.put(base+h.slot,trampoline);
    }
    image.block(object,0x348);image.put(object,ubc::hand_detail::item_vtable);image.put(object+4,2);image.put(object+0x330,16610);
    image.block(renderer,0x800);image.block(device,0x300);image.put(device,base+0xcc7dc);image.put(base+0xcc7dc+0xbc,base+0x35e40);
    image.put(base+0xea6ac,renderer);image.put(base+0xea6a0,device);image.put(device+0x10c,head);image.block(head,12);image.put(head,head);image.put(head+4,head);
    image.block(sentinel,12);image.block(buckets,64);image.put(renderer+0x578,sentinel);image.put(renderer+0x580,buckets);
    image.put(renderer+0x584,buckets+64);image.put(renderer+0x588,buckets+64);image.put(renderer+0x58c,7);image.put(renderer+0x590,8);
    for(unsigned i=0;i<8;++i){image.put(buckets+i*8,sentinel);image.put(buckets+i*8+4,sentinel);}
  }
  r::ObjectEvidence capture(){return reader.capture(image.memory(),module(),request());}
  void map_entry(std::uint32_t target){
    const auto slot=buckets+(r::detail::pointer_hash(object)&7u)*8u,node=0x14300000u;
    image.block(node,12);image.put(node,sentinel);image.put(node+4,sentinel);image.put(node+8,target);
    image.put(slot,node);image.put(slot+4,node);image.put(renderer+0x57c,1);
  }
  void device_entry(std::uint32_t target){
    image.block(0x14400000,12);image.block(0x14500000,4);image.put(0x14500000,target);image.put(0x14400000,head);image.put(0x14400004,head);image.put(0x14400008,0x14500000);
    image.put(head,0x14400000);image.put(head+4,0x14400000);image.put(device+0x110,1);
  }
};
void nested(Image& image) noexcept {check(reader.capture(image.memory(),module(),request()).issue==r::Issue::reentry,"nested reader cannot share state");}
void files(const char* exe_file,const char* renderer_file){
  const PE exe(exe_file,ubc::station_abi::executable_sha256),dll(renderer_file,r::file_sha256);
  {Fixture f(exe,dll);const auto code=r::verify_code(f.image.memory(),module());const auto e=f.capture();
    check(code.verified,"pinned renderer plus two exact synthetic MinHook/trampoline chains");
    check(e.short_path_verified&&e.object==object&&e.symbol==16610,"actual object without homeWorld, visual or cache/UI ownership has the short no-delete path");
    check(r::verify_forwarding(f.image.memory(),module()).verified,"exact forwarding chain is independently positive");}
  // E9 occupies exactly five bytes. For the 6- or 7-byte prologues
  // resumed in full in the trampoline, the remaining bytes may be entirely
  // original, NOP or INT3. Each site chooses its form independently.
  for(unsigned first=0;first<3;++first)for(unsigned second=0;second<3;++second){Fixture f(exe,dll);
    for(std::size_t i=0;i<r::detail::hooks.size();++i){
      const auto& h=r::detail::hooks[i];const auto form=i?second:first;
      if(form==0)std::memcpy(f.image.at(h.entry+5,h.length-5),h.prologue.data()+5,h.length-5);
      else std::memset(f.image.at(h.entry+5,h.length-5),form==1?0x90:0xcc,h.length-5);
    }
    check(r::verify_code(f.image.memory(),module()).verified,"all nine exact original/NOP/INT3 combinations verify");
    check(r::verify_forwarding(f.image.memory(),module()).verified,"three complete entry forms retain exact forwarding");
  }
  for(const auto first:{0x82u,0x90u,0xccu,0xcdu})for(const auto second:{0u,0x90u,0xccu,0xcdu}){
    if((first==0x82&&second==0)||(first==0x90&&second==0x90)||(first==0xcc&&second==0xcc))continue;
    Fixture f(exe,dll);const auto& h=r::detail::hooks[0];
    *f.image.at(h.entry+5,1)=static_cast<std::uint8_t>(first);*f.image.at(h.entry+6,1)=static_cast<std::uint8_t>(second);
    check(!r::verify_code(f.image.memory(),module()).verified,"mixed or unknown suffix is not a complete accepted form");
  }
  for(const auto& range:r::ranges){Fixture f(exe,dll);auto* last=f.image.at(base+range.rva+range.length-1,1);*last^=1;
    check(!r::verify_code(f.image.memory(),module()).verified,"last byte of full renderer body is included");}
  for(const auto& h:r::detail::hooks)for(unsigned which=0;which<6;++which){Fixture f(exe,dll);const auto t=u32(f.image.at(base+h.slot,4));
    std::memset(f.image.at(h.entry+5,h.length-5),0xcc,h.length-5);
    if(which==0)f.image.put(h.entry+1,base+h.rva-h.entry-4u);
    if(which==1)*f.image.at(h.entry+5,1)=0xcd;
    if(which==2)*f.image.at(t+h.length-1,1)^=1;
    if(which==3)f.image.put(t+h.length+1,h.entry+5-(t+h.length+5));
    if(which==4)*f.image.at(h.entry+h.length+1,1)^=1;
    if(which==5)*f.image.at(h.entry,1)=0xe8;
    check(!r::verify_code(f.image.memory(),module()).verified,"INT3 never waives target, padding, stolen bytes, return, body or E9 checks");}
  for(unsigned which=0;which<3;++which){Fixture f(exe,dll);auto m=module();if(which==0)m.mapping_verified=false;if(which==1)m.file_sha256[0]^=1;if(which==2)m.image_size-=4096;
    check(!r::verify_code(f.image.memory(),m).verified,"no filename-only module acceptance");}
  for(unsigned offset:{0xb8u,0xc8u,0x340u}){Fixture f(exe,dll);f.image.put(object+offset,0x600000);check(!f.capture().short_path_verified,"homeWorld, visual and item field +340 remain separate optional null gates");}
  {Fixture f(exe,dll);f.map_entry(object);check(!f.capture().short_path_verified,"actual renderer cache hit cannot claim no-cleanup path");
    check(r::verify_forwarding(f.image.memory(),module()).verified,"known renderer still forwards with actual cache hit");}
  {Fixture f(exe,dll);f.device_entry(object);check(!f.capture().short_path_verified,"device-list alias cannot claim no-delete path");}
  {Fixture f(exe,dll);std::uint32_t other=object+0x1000;while((r::detail::pointer_hash(other)&7u)!=(r::detail::pointer_hash(object)&7u))++other;
    f.map_entry(other);f.device_entry(other);const auto e=f.capture();check(e.short_path_verified&&e.bucket_nodes==1&&e.device_nodes==1,"nonempty actual containers permit proven absent candidate");}
  for(unsigned offset:{0x100u,0xfcu,0x110u,0x10cu}){Fixture f(exe,dll);f.image.block(0x16000000,0x40);f.image.block(0x16100000,0x120);f.image.block(0x16200000,12);
    f.image.put(device+0x204,0x16000000);f.image.put(0x16000038,0x16100000);f.image.put(0x16100000+offset,0x16200000);f.image.put(0x16200008,object);
    check(!f.capture().short_path_verified,"every device UI target is excluded independently");}
  {Fixture f(exe,dll);f.image.put(base+0xcc7dc+0xbc,base+0x35e41);check(!f.capture().short_path_verified,"dynamic device vslot must equal verified function");}
  {Fixture f(exe,dll);f.image.put(renderer+0x58c,6);check(!f.capture().short_path_verified,"invalid hash mask rejects before bucket read");}
  {Fixture f(exe,dll);f.device_entry(0x123400);f.image.put(0x14400000,0x14400000);check(!f.capture().short_path_verified,"device cycle is bounded");}
  for(unsigned offset:{0xb8u,0xc8u}){Fixture f(exe,dll);f.image.denied=object+offset;f.image.denied_size=4;check(!f.capture().short_path_verified,"unreadable homeWorld or visual is not zero");}
  {Fixture f(exe,dll);f.image.denied=renderer+0x574;f.image.denied_size=32;
    check(r::verify_forwarding(f.image.memory(),module()).verified,"compatibility does not require hypothetical future cache absence");}
  {Fixture f(exe,dll);f.image.put(base+0xcc7dc+0xbc,base+0x35e41);
    check(!r::verify_forwarding(f.image.memory(),module()).verified,"forwarding cannot accept a changed dynamic callback");}
  {Fixture f(exe,dll);f.image.change_address=base+0xea6a0;f.image.change_target=base+0xea6a0;f.image.change_on_read=2;
    check(!r::verify_forwarding(f.image.memory(),module()).verified,"device replacement across forwarding read rejects");}
  {Fixture f(exe,dll);f.image.change_address=renderer+0x574;f.image.change_target=renderer+0x580;f.image.change_on_read=2;
    check(!f.capture().short_path_verified,"hash container drift rejects whole observation");}
  {Fixture f(exe,dll);f.image.on_read=&nested;check(f.capture().short_path_verified,"outer observation survives denied reentry");}
  {Fixture f(exe,dll);auto q=request();q.object=0;check(!reader.capture(f.image.memory(),module(),q).short_path_verified,"future unknown factory pointer cannot be blessed");
    q=request();q.boundary.current_thread=9;check(!reader.capture(f.image.memory(),module(),q).short_path_verified,"foreign thread never owns observation");}
  {Fixture f(exe,dll);f.image.put(0x2964,0x9900);check(!f.capture().short_path_verified,"old request cannot claim current foreign station");}
}
}
int main(int argc,char** argv){Image none;check(!r::verify_code(none.memory(),{}).verified,"empty module ownership is unknown");
  if(argc==4&&std::string(argv[1])=="--local-files")files(argv[2],argv[3]);else if(argc!=1)return 2;
  std::printf("%u checks, %u failures\n",station_test::checks,station_test::failures);return station_test::failures?1:0;}
