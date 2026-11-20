// SPDX-License-Identifier: MIT
#include "ubc/stations/saw/ammo_runtime.hpp"
#include <algorithm>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace {
namespace a=ubc::ammo_runtime;namespace p=ubc::ammo_presentation;
unsigned checks=0,failures=0;
void check(bool value,const char* name){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",name);}}
struct Region {std::uint32_t address;std::vector<std::uint8_t> bytes;};
struct Memory {
  std::vector<Region> regions;
  std::function<void(Memory&,std::uint32_t,std::size_t)> before;
  std::uint32_t denied=0;unsigned denied_reads=0;
  void add(std::uint32_t address,std::size_t size){regions.push_back({address,std::vector<std::uint8_t>(size)});}
  std::uint8_t* at(std::uint32_t address,std::size_t size=1){
    for(auto& r:regions)if(address>=r.address&&std::uint64_t{address}-r.address+size<=r.bytes.size())return r.bytes.data()+(address-r.address);
    return nullptr;
  }
  void put(std::uint32_t address,std::uint32_t value){auto* out=at(address,4);if(!out)std::abort();for(unsigned i=0;i<4;++i)out[i]=static_cast<std::uint8_t>(value>>(8u*i));}
  void text(std::uint32_t address,const char* value){const auto size=std::strlen(value)+1;auto* out=at(address,size);if(!out)std::abort();std::memcpy(out,value,size);}
  static bool read(void* context,std::uint32_t address,void* out,std::size_t size) noexcept {
    auto& self=*static_cast<Memory*>(context);if(self.before)self.before(self,address,size);
    if(self.denied&&a::detail::overlaps(address,size,self.denied,1)){++self.denied_reads;return false;}
    const auto* found=self.at(address,size);if(!found)return false;std::memcpy(out,found,size);return true;
  }
  ubc::ReadOnlyMemory reader(){return {this,read};}
};
struct Example {
  static constexpr std::uint32_t message=0x100000,text=0x101000,font=0x102000,hero=0x103000,manager=0x104000,
    stack=0x105000,table=0x106000,symbols=0x107000,names=0x108000,code=0x200000,code_size=800000;
  Memory memory;
  p::Context context{1,7,ubc::lhiver::game_parser,0x40000,0x50000,hero};
  p::Scope scope{1,2,p::templates[0].callback,1,true,true};
  a::CaptureSite capture{7,ubc::lhiver::game_parser,message,manager,stack};
  a::InstalledHooks hooks{};
  Example(){
    memory.add(message,a::message_size);memory.add(text,p::string_read_bound);memory.add(font,p::string_read_bound);
    memory.add(hero,0x100);memory.add(manager,0x40);memory.add(stack,0x60);memory.add(table,128);
    memory.add(symbols,6*0x40);memory.add(names,6*0x40);memory.add(code,code_size);
    memory.add(ubc::lhiver::game_parser,0x58);memory.add(a::hero_pointer,4);
    memory.add(ubc::selection_abi::game_pointer,4);memory.add(context.game,12);
    memory.put(ubc::selection_abi::game_pointer,context.game);memory.put(context.game,ubc::selection_abi::game_vtable);
    memory.put(context.game+ubc::selection_abi::game_world,context.world);
    memory.add(a::manager_vtable,0x2c);memory.add(p::conversation_vtable,0x10);
    memory.put(a::manager_vtable+a::manager_on_message_slot,a::manager_on_message);
    memory.put(p::conversation_vtable+a::deleting_destructor_slot,a::deleting_destructor);
    memory.put(message,p::conversation_vtable);memory.put(message+a::message_subtype,21);
    memory.put(message+a::message_text+8,text);memory.put(message+a::message_text+12,18);
    memory.put(message+a::message_font+8,font);memory.put(message+a::message_font+12,13);
    memory.text(text,"owned test message");memory.text(font,"test_font.fnt");
    memory.put(message+a::message_target,hero);memory.put(message+a::message_x,UINT32_MAX);
    memory.put(message+a::message_y,30);memory.put(message+a::message_seconds,0x40000000);
    memory.put(a::hero_pointer,hero);memory.put(manager,a::manager_vtable);
    memory.put(manager+a::manager_owner,hero);memory.put(hero+a::npc_manager,manager);
    memory.put(stack+a::external_return_stack_offset,p::external_dispatch_return);
    memory.put(context.parser+0x18,table);memory.put(context.parser+0x1c,32);memory.put(context.parser+0x20,28);
    memory.put(context.parser+0x48,code);memory.put(context.parser+0x50,code+code_size);memory.put(context.parser+0x54,code_size);
    set_callback(0);
    for(unsigned i=0;i<6;++i){
      const auto symbol=symbols+0x40*i,name=names+0x40*i;
      memory.put(table+4*(22+i),symbol);memory.put(symbol+8,name);
      memory.put(symbol+12,static_cast<std::uint32_t>(std::strlen(a::detail::external_names[i])));
      memory.text(name,a::detail::external_names[i]);
      memory.put(symbol+0x20,(i?0u:5u)|(a::detail::external_types[i]<<12u)|((i?0u:11u)<<16u));
      if(!i){memory.put(symbol+0x18,a::external_entry);memory.put(symbol+0x1c,2);}
    }
    for(std::size_t i=0;i<a::hook_count;++i){
      const auto& site=a::hook_sites[i];hooks.hooks[i]={true,0x3100000+static_cast<std::uint32_t>(i)*0x100,0x3000000+static_cast<std::uint32_t>(i)*0x100};
      memory.add(hooks.hooks[i].trampoline,32);
      // Synthetic code ranges of our own. No original function in the test.
      (void)site;
    }
    for(const auto& range:a::code_ranges)memory.add(range.address,range.length);
  }
  void set_callback(std::size_t index){
    scope.callback=p::templates[index].callback;const auto cursor=code+p::templates[index].success_pc;
    memory.put(context.parser+0x4c,cursor);*memory.at(cursor-5)=0x3e;memory.put(cursor-4,22);
  }
  void install(){
    for(std::size_t i=0;i<a::hook_count;++i){
      const auto& site=a::hook_sites[i];const auto address=static_cast<std::uint32_t>(site.address);const auto& own=hooks.hooks[i];
      auto* patch=memory.at(address,site.patch_size);std::memset(patch,0x90,site.patch_size);a::detail::jump(patch,address,own.destination);
      auto* trampoline=memory.at(own.trampoline,site.patch_size+5);std::memcpy(trampoline,site.original.data(),site.patch_size);
      a::detail::jump(trampoline+site.patch_size,own.trampoline+static_cast<std::uint32_t>(site.patch_size),address+static_cast<std::uint32_t>(site.patch_size));
    }
  }
};
void payload_tests(){
  Example e;const auto value=a::read_payload(e.memory.reader(),e.message);
  check(value.valid&&value.value.payload.known&&value.value.target==e.hero,"stable bounded own text fixture is copied as fingerprints");
  check(value.value.payload.text.length==18&&value.value.payload.font.length==13,"lengths exclude terminating NUL");
  check(value.value.payload.text.sha256==a::detail::digest("owned test message",18)&&
    value.value.payload.font.sha256==a::detail::digest("test_font.fnt",13),"only exact input bytes contribute to SHA");
  check(value.value.payload.x==-1&&value.value.payload.y==30&&value.value.payload.seconds_bits==0x40000000,"integer and float bit patterns remain exact");
  for(unsigned bad=0;bad<17;++bad){
    Example x;auto address=x.message;
    switch(bad){
      case 0:address=0;break;case 1:address=UINT32_MAX-3;break;case 2:++address;break;
      case 3:x.memory.denied=x.message;break;case 4:x.memory.put(x.message,7);break;
      case 5:x.memory.put(x.message+a::message_subtype,20);break;
      case 6:x.memory.put(x.message+a::message_text+12,0);break;
      case 7:x.memory.put(x.message+a::message_text+12,128);break;
      case 8:x.memory.put(x.message+a::message_font+12,UINT32_MAX);break;
      case 9:*x.memory.at(x.text+18)=1;break;case 10:*x.memory.at(x.font+2)=0;break;
      case 11:x.memory.put(x.message+a::message_font+8,x.text);break;
      case 12:x.memory.put(x.message+a::message_font+8,x.text+10);break;
      case 13:x.memory.put(x.message+a::message_text+8,x.message+60);break;
      case 14:x.memory.put(x.message+a::message_text+8,UINT32_MAX-5);break;
      case 15:x.memory.denied=x.font;break;case 16:x.memory.denied=x.text;break;
    }
    const auto result=a::read_payload(x.memory.reader(),address);
    check(!result.valid&&!result.value.payload.known,"invalid, unknown, unterminated, overlapping or unreadable payload stays unknown");
  }
  {
    Example x;x.memory.put(x.message+a::message_text+12,127);std::memset(x.memory.at(x.text,127),'x',127);*x.memory.at(x.text+127)=0;
    check(a::read_payload(x.memory.reader(),x.message).valid,"largest accepted string fits including NUL in the fixed 128-byte bound");
  }
  for(unsigned which=0;which<4;++which){
    Example x;unsigned reads=0;
    x.memory.before=[&](Memory& m,std::uint32_t address,std::size_t){
      if(address==(which<2?x.message:x.text)&&++reads==2){
        if(which==0)m.put(x.message+a::message_y,31);
        if(which==1)m.put(x.message+a::message_target,x.hero+4);
        if(which==2)*m.at(x.text)='X';
        if(which==3)*m.at(x.font)='F';
      }
    };
    check(!a::read_payload(x.memory.reader(),x.message).valid,"second object/text/font pass rejects changed immutable payload");
  }
  {
    Example x;x.memory.denied=x.message+60;x.memory.put(x.message+a::message_text+8,x.message+60);
    // Header read itself includes denied byte, so use a read spy after it.
    unsigned pointer_reads=0;x.memory.denied=0;
    x.memory.before=[&](Memory&,std::uint32_t address,std::size_t){if(address==x.message+60)++pointer_reads;};
    check(!a::read_payload(x.memory.reader(),x.message).valid&&!pointer_reads,"aliased string pointer is rejected before following it");
  }
}
void external_tests(){
  Example e;check(a::verify_external(e.memory.reader(),e.context.parser).valid,"exact current External22 and five loaded parameter symbols match");
  for(unsigned bad=0;bad<18;++bad){
    Example x;auto parser=x.context.parser;
    switch(bad){
      case 0:parser+=4;break;case 1:x.memory.put(parser+0x20,27);break;
      case 2:x.memory.put(parser+0x1c,1000001);break;case 3:x.memory.put(parser+0x18,0);break;
      case 4:x.memory.put(x.symbols+0x18,a::external_entry+1);break;
      case 5:x.memory.put(x.symbols+0x1c,0);break;
      case 6:x.memory.put(x.symbols+0x20,5|(5u<<12u)|(3u<<16u));break;
      case 7:x.memory.put(x.symbols+0x38,x.symbols);break;
      case 8:*x.memory.at(x.names)='X';break;case 9:x.memory.put(x.symbols+12,64);break;
      case 10:x.memory.put(x.symbols+0x40+0x20,2u<<12u);break;
      case 11:x.memory.put(x.symbols+0x40+0x20,(3u<<12u)|1);break;
      case 12:x.memory.put(x.symbols+0x40+0x38,x.symbols);break;
      case 13:x.memory.put(x.table+4*23,x.symbols);break;
      case 14:x.memory.put(x.symbols+0x40+8,x.names);break;
      case 15:x.memory.denied=x.symbols+0x80;break;
      case 16:x.memory.put(parser+0x50,x.code);break;
      case 17:x.memory.put(parser+0x54,64u*1024u*1024u+1);break;
    }
    check(!a::verify_external(x.memory.reader(),parser).valid,"external registration, signature, name, parent, aliases and layout are all strict");
  }
  for(unsigned bad=0;bad<3;++bad){
    Example x;unsigned reads=0;
    x.memory.before=[&](Memory& m,std::uint32_t address,std::size_t){if(address==x.context.parser&&++reads==2){
      if(bad==0)m.put(x.context.parser+0x20,29);
      if(bad==1)m.put(x.symbols+0x40+0x18,12);
      if(bad==2)m.put(x.context.parser+0x50,x.code+x.code_size-1);
    }};
    check(!a::verify_external(x.memory.reader(),x.context.parser).valid,"second external pass binds layout and live symbol values");
  }
}
void capture_tests(){
  for(std::size_t i=0;i<p::templates.size();++i){
    Example e;e.set_callback(i);const auto result=a::read_capture_data(e.memory.reader(),e.context,e.scope,e.capture);
    check(result.valid&&result.value.manager_owner_verified&&result.value.vm_pc==p::templates[i].success_pc&&result.value.target==e.hero,
      "all five current success PCs bind message target and both manager owner directions");
    check(!result.value.native_contract,"data-only capture never claims installed native code");
    check(!a::read_capture(e.memory.reader(),e.context,e.scope,e.capture,e.hooks).valid,"synthetic data plus arbitrary code cannot manufacture full native contract");
  }
  for(std::size_t i=0;i<p::templates.size();++i){
    for(unsigned bad=0;bad<3;++bad){
      Example e;e.set_callback(i);const auto cursor=e.code+p::templates[i].success_pc;
      if(bad==0)*e.memory.at(cursor-5)=0xbe;
      else e.memory.put(cursor-4,bad==1?23u:a::external_entry);
      const auto value=a::read_capture_data(e.memory.reader(),e.context,e.scope,e.capture);
      check(!value.valid&&value.issue==a::Issue::pc&&value.value.vm_pc==p::templates[i].success_pc&&
        value.value.caller==p::external_dispatch_return&&!value.value.native_contract,
        "all five profiles reject hexadecimal BE wrong symbol and native address despite exact PC and caller");
    }
    for(unsigned change=0;change<3;++change){
      Example e;e.set_callback(i);const auto cursor=e.code+p::templates[i].success_pc;unsigned reads=0;
      if(change==2){
        *e.memory.at(cursor+5)=0x3e;e.memory.put(cursor+6,22);
        e.memory.before=[&](Memory& memory,std::uint32_t address,std::size_t){
          if(address==e.context.parser+0x4c&&++reads==2)memory.put(address,cursor+10);
        };
      }else e.memory.before=[&](Memory& memory,std::uint32_t address,std::size_t){
        if(address==cursor-5&&++reads==2){
          if(change==0)*memory.at(cursor-5)=0xbe;else memory.put(cursor-4,23);
        }
      };
      const auto value=a::read_capture_data(e.memory.reader(),e.context,e.scope,e.capture);
      check(reads==2&&!value.valid&&value.issue==(change==2?a::Issue::changed:a::Issue::pc)&&
        value.value.vm_pc==p::templates[i].success_pc&&!value.value.native_contract,
        "all five profiles reject second-pass opcode operand or PC changes while retaining first observed PC");
    }
  }
  for(unsigned bad=0;bad<28;++bad){
    Example e;
    switch(bad){
      case 0:e.context.epoch=0;break;case 1:++e.capture.thread;break;case 2:++e.capture.parser;break;
      case 3:e.scope.callback=99;break;case 4:e.scope.depth=2;break;case 5:e.scope.active=false;break;
      case 6:e.scope.patched_body_verified=false;break;case 7:e.scope.token=0;break;
      case 8:e.memory.put(a::hero_pointer,e.hero+4);break;case 9:e.memory.put(e.manager+a::manager_owner,e.hero+4);break;
      case 10:e.memory.put(e.hero+a::npc_manager,e.manager+4);break;case 11:e.memory.put(e.message+a::message_target,e.hero+4);break;
      case 12:e.memory.put(e.manager,a::manager_vtable+4);break;
      case 13:e.memory.put(a::manager_vtable+a::manager_on_message_slot,a::manager_on_message+1);break;
      case 14:e.memory.put(e.stack+a::external_return_stack_offset,p::external_dispatch_return+1);break;
      case 15:e.capture.event_manager=e.hero;break;case 16:e.capture.event_manager=e.message;break;
      case 17:e.capture.stack=UINT32_MAX-8;break;
      case 18:e.memory.put(e.context.parser+0x4c,e.code+4);break;
      case 19:e.memory.put(e.context.parser+0x4c,e.code+e.code_size+1);break;
      case 20:e.memory.put(e.code+p::templates[0].success_pc-4,23);break;
      case 21:*e.memory.at(e.code+p::templates[0].success_pc-5)=0xbd;break;
      case 22:e.set_callback(1);e.scope.callback=p::templates[0].callback;break;
      case 23:e.memory.denied=e.hero+a::npc_manager;break;
      case 24:e.memory.put(ubc::selection_abi::game_pointer,e.context.game+4);break;
      case 25:e.memory.put(e.context.game+ubc::selection_abi::game_world,e.context.world+4);break;
      case 26:e.memory.put(e.context.game,ubc::selection_abi::game_vtable+4);break;
      case 27:e.memory.denied=ubc::selection_abi::game_pointer;break;
    }
    const auto result=a::read_capture_data(e.memory.reader(),e.context,e.scope,e.capture);
    check(!result.valid&&!result.value.native_contract,"wrong context, scope, owner, actual PC, caller or unknown field cannot capture");
  }
  for(unsigned bad=0;bad<3;++bad){
    Example e;unsigned reads=0;
    e.memory.before=[&](Memory& m,std::uint32_t address,std::size_t){if(address==e.manager&&++reads==2){
      if(bad==0)m.put(e.context.parser+0x20,29);
      if(bad==1)m.put(e.manager+a::manager_owner,e.hero+4);
      if(bad==2)m.put(e.stack+a::external_return_stack_offset,0);
    }};
    check(!a::read_capture_data(e.memory.reader(),e.context,e.scope,e.capture).valid,"owner/PC layout/return address are re-read after payload");
  }
}
void hook_tests(){
  Example e;e.install();check(a::detail::hooks_once(e.memory.reader(),e.hooks).valid,"all three exact own JMPs and full return trampolines agree");
  for(std::size_t i=0;i<a::hook_count;++i)for(unsigned bad=0;bad<11;++bad){
    Example x;x.install();const auto& site=a::hook_sites[i];const auto address=static_cast<std::uint32_t>(site.address);auto& own=x.hooks.hooks[i];
    switch(bad){
      case 0:own.installed=false;break;case 1:own.destination=0;break;case 2:own.trampoline=0;break;
      case 3:own.destination=own.trampoline+3;break;
      case 4:own.trampoline=x.hooks.hooks[(i+1)%3].trampoline+4;break;
      case 5:own.destination=x.hooks.hooks[(i+1)%3].destination;break;
      case 6:own.destination=0x759280;break;
      case 7:*x.memory.at(address)=0x90;break;case 8:*x.memory.at(address+5)=0xcc;break;
      case 9:*x.memory.at(own.trampoline)=0xcc;break;
      case 10:*x.memory.at(own.trampoline+static_cast<std::uint32_t>(site.patch_size)+2)^=1;break;
    }
    check(!a::detail::hooks_once(x.memory.reader(),x.hooks).valid,"missing provenance, aliases, changed patch bytes and wrong trampoline returns fail");
  }
  for(std::size_t i=0;i<a::hook_count;++i){
    Example x;const auto& site=a::hook_sites[i];const auto address=static_cast<std::uint32_t>(site.address);const auto length=static_cast<std::uint32_t>(site.patch_size+8);
    auto* bytes=x.memory.at(address,length);std::memset(bytes,0x47,length);std::memcpy(bytes,site.original.data(),site.patch_size);
    const a::CodeRange wanted{address,length,a::detail::digest(bytes,length)};
    check(a::detail::code_range(x.memory.reader(),wanted,nullptr).valid,"generic verifier accepts exact own synthetic original range");
    x.install();check(a::detail::code_range(x.memory.reader(),wanted,&x.hooks).valid,"only exact own installed prefix is normalized before whole-range hash");
    check(!a::detail::code_range(x.memory.reader(),wanted,nullptr).valid,"installed bytes are never mistaken for original bytes");
    *x.memory.at(address+static_cast<std::uint32_t>(site.patch_size)+2)^=1;
    check(!a::detail::code_range(x.memory.reader(),wanted,&x.hooks).valid,"unpatched tail alteration still rejects an otherwise own hook");
  }
  {
    Example x;const auto address=a::code_ranges[1].address;const a::CodeRange range{address,32,a::detail::digest(x.memory.at(address,32),32)};unsigned reads=0;
    x.memory.before=[&](Memory& m,std::uint32_t at,std::size_t){if(at==address&&++reads==2)*m.at(address+1)=1;};
    check(!a::detail::code_range(x.memory.reader(),range,nullptr).valid,"code hash rejects changes between its bounded copies");
    check(!a::detail::code_range(x.memory.reader(),{address,1025,{}},nullptr).valid,"code reader retains fixed hard range bound");
  }
  {
    Example x;check(a::detail::vtables(x.memory.reader()).valid,"both native dispatch slots match the independently identified methods");
    x.memory.put(p::conversation_vtable+a::deleting_destructor_slot,7);
    check(!a::detail::vtables(x.memory.reader()).valid,"same conversation vptr with replaced deleting destructor is not a lifetime proof");
  }
}

// Optional read-only comparison with the existing local EXE. No byte
// or string export, installation or access to a running process.
bool load_engine(const char* path,std::vector<std::uint8_t>& bytes){
  std::FILE* file=nullptr;
#if defined(_MSC_VER)
  if(fopen_s(&file,path,"rb")!=0)return false;
#else
  file=std::fopen(path,"rb");
#endif
  if(!file)return false;
  bool good=std::fseek(file,0,SEEK_END)==0;const long length=good?std::ftell(file):-1;
  good=length>0&&length<=16*1024*1024&&std::fseek(file,0,SEEK_SET)==0;
  if(good){bytes.resize(static_cast<std::size_t>(length));good=std::fread(bytes.data(),1,bytes.size(),file)==bytes.size();}
  std::fclose(file);return good&&a::detail::digest(bytes.data(),bytes.size())==ubc::lhiver::digest_literal("0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f");
}
bool engine_copy(const std::vector<std::uint8_t>& bytes,std::uint32_t address,void* output,std::size_t size){
  if(bytes.size()<0x40||bytes[0]!='M'||bytes[1]!='Z'||address<0x400000)return false;
  const auto pe=a::detail::u32(bytes.data()+0x3c);if(std::uint64_t{pe}+24>bytes.size())return false;
  const auto sections=a::detail::u16(bytes.data()+pe+6),optional=a::detail::u16(bytes.data()+pe+20);
  if(sections>32||std::uint64_t{pe}+24+optional+sections*40>bytes.size())return false;
  const auto rva=address-0x400000;
  for(std::uint32_t i=0;i<sections;++i){
    const auto* section=bytes.data()+pe+24+optional+i*40;
    const auto va=a::detail::u32(section+12),raw_size=a::detail::u32(section+16),raw=a::detail::u32(section+20);
    if(rva>=va&&std::uint64_t{rva}-va+size<=raw_size&&std::uint64_t{raw}+rva-va+size<=bytes.size()){
      std::memcpy(output,bytes.data()+raw+rva-va,size);return true;
    }
  }
  return false;
}
void engine_tests(const char* path){
  std::vector<std::uint8_t> bytes;const bool loaded=load_engine(path,bytes);
  check(loaded,"optional engine fixture is read-only and has exact complete file SHA");if(!loaded)return;
  Example e;
  for(const auto& range:a::code_ranges)check(engine_copy(bytes,range.address,e.memory.at(range.address,range.length),range.length),"bounded native range exists in verified PE");
  check(engine_copy(bytes,p::conversation_vtable+a::deleting_destructor_slot,e.memory.at(p::conversation_vtable+a::deleting_destructor_slot,4),4),"actual PE deleting destructor slot copied only to test RAM");
  check(engine_copy(bytes,a::manager_vtable+a::manager_on_message_slot,e.memory.at(a::manager_vtable+a::manager_on_message_slot,4),4),"actual PE OnMessage slot copied only to test RAM");
  check(a::verify_original_code(e.memory.reader()).valid,"fixed original whole-range and vtable contracts match pinned on-disk EXE");
  e.install();check(!a::verify_original_code(e.memory.reader()).valid,"own RAM test installation no longer claims original entry bytes");
  check(a::verify_installed_code(e.memory.reader(),e.hooks).valid,"exact own hooks plus unchanged rest of all native routines verify");
  for(std::size_t i=0;i<p::templates.size();++i){
    e.set_callback(i);const auto capture=a::read_capture(e.memory.reader(),e.context,e.scope,e.capture,e.hooks);
    check(capture.valid&&capture.value.native_contract&&capture.value.manager_owner_verified,"full capture combines real code contract with synthetic data and current success PC");
    const auto display=a::read_display(e.memory.reader(),e.context,{e.context.thread,e.hero,e.message,0},e.hooks);
    check(display.valid&&display.value.native_contract,"queued display does not require a synchronous effect Scope");
    const auto destroy=a::read_destruction(e.memory.reader(),e.context,{e.context.thread,e.message,0},e.hooks);
    check(destroy.valid&&destroy.value.native_contract,"destructor entry validates payload before the original frees it");
  }
  for(unsigned bad=0;bad<12;++bad){
    auto copy=e;auto site=a::PrintSite{copy.context.thread,copy.hero,copy.message,0};
    switch(bad){
      case 0:copy.context.epoch=0;break;case 1:++site.thread;break;case 2:++site.receiver;break;
      case 3:site.message=0;break;
      case 4:copy.memory.put(a::hero_pointer,copy.hero+4);break;
      case 5:copy.memory.put(copy.message+a::message_target,copy.hero+4);break;
      case 6:copy.memory.put(copy.context.game+ubc::selection_abi::game_world,copy.context.world+4);break;
      case 7:copy.memory.put(copy.symbols+0x18,a::external_entry+1);break;
      case 8:copy.hooks.hooks[2].installed=false;break;
      case 9:copy.memory.denied=copy.font;break;
      case 10:copy.memory.put(p::conversation_vtable+a::deleting_destructor_slot,0);break;
      case 11:copy.memory.put(copy.message,0);break;
    }
    const auto display=a::read_display(copy.memory.reader(),copy.context,site,copy.hooks);
    check(!display.valid&&!display.value.native_contract,"display requires live world, receiver, target, payload, registration and all lifetime hooks");
    if(bad!=2){
      const auto destruction=a::read_destruction(copy.memory.reader(),copy.context,{site.thread,site.message,0},copy.hooks);
      check(!destruction.valid&&!destruction.value.native_contract,"destructor entry never retires a message on unknown or changed native context");
    }
  }
  for(unsigned bad=0;bad<5;++bad){
    auto copy=e;unsigned reads=0;
    if(bad==4){copy.memory.add(0x109000,128);std::memcpy(copy.memory.at(0x109000,128),copy.memory.at(copy.table,128),128);}
    copy.memory.before=[&](Memory& m,std::uint32_t address,std::size_t){if(address==a::code_ranges[0].address&&++reads==1){
      if(bad==0)m.put(copy.symbols+0x40+0x18,17); // Signature still valid, different snapshot.
      if(bad==1)m.put(copy.context.game+ubc::selection_abi::game_world,copy.context.world+4);
      if(bad==2)*m.at(copy.text)='X';
      if(bad==3)m.put(copy.context.parser+0x4c,copy.code+p::templates[0].success_pc);
      if(bad==4)m.put(copy.context.parser+0x18,0x109000);
    }};
    const auto capture=a::read_capture(copy.memory.reader(),copy.context,copy.scope,copy.capture,copy.hooks);
    check(!capture.valid&&!capture.value.native_contract,"full capture binds external table/values, current world, payload and PC across code verification");
  }
  for(unsigned bad=0;bad<4;++bad){
    auto copy=e;unsigned reads=0;
    copy.memory.before=[&](Memory& m,std::uint32_t address,std::size_t){if(address==a::code_ranges[0].address&&++reads==1){
      if(bad==0)m.put(copy.symbols+0x40+0x18,17);
      if(bad==1)m.put(copy.context.game+ubc::selection_abi::game_world,copy.context.world+4);
      if(bad==2)*m.at(copy.font)='X';
      if(bad==3)m.put(copy.message+a::message_target,copy.hero+4);
    }};
    const auto display=a::read_display(copy.memory.reader(),copy.context,{copy.context.thread,copy.hero,copy.message,0},copy.hooks);
    check(!display.valid&&!display.value.native_contract,"queued display verifies stable native contract and payload without borrowing a former effect scope");
  }
  for(const auto& range:a::code_ranges){
    auto* byte=e.memory.at(range.address+range.length-1);*byte^=1;
    check(!a::verify_installed_code(e.memory.reader(),e.hooks).valid,"each native routine remains hash-bound after installation");*byte^=1;
  }
  unsigned reads=0;const auto early=a::code_ranges[0].address;
  e.memory.before=[&](Memory& m,std::uint32_t address,std::size_t){if(address==a::code_ranges.back().address&&++reads==2)*m.at(early+10)^=1;};
  check(!a::verify_installed_code(e.memory.reader(),e.hooks).valid,"second complete range pass catches an earlier region changed while a later region was read");
}
} // namespace
int main(int argc,char** argv){
  payload_tests();external_tests();capture_tests();hook_tests();
  if(argc==3&&std::strcmp(argv[1],"--engine-READONLY")==0)engine_tests(argv[2]);
  else if(argc!=1){std::fputs("usage: ammo_runtime_tests [--engine-READONLY PATH]\n",stderr);return 2;}
  std::printf("AmmoRuntime: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
