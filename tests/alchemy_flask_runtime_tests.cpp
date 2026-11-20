// SPDX-License-Identifier: MIT
#include "ubc/alchemy_flask_runtime.hpp"
#include "ubc/tobacco_runtime.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace rt=ubc::alchemy_flask_runtime;
namespace cat=ubc::alchemy_recipe_catalog;
namespace debit=ubc::alchemy_flask_debit;
namespace {
unsigned checks=0,failures=0;
void check(bool value,int line) {++checks;if(!value){++failures;std::fprintf(stderr,"failure line %d\n",line);}}
#define CHECK(value) check((value),__LINE__)
constexpr std::uint32_t game=0x2000,world=0x4000,hero=0x6000,mob=0x8000,table=0x100000,
  code=0x200000,symbol_base=0x10000000,name_base=0x20000000,strings=0x70000,
  slot_array=0x90000,left_slot=0x91000,right_slot=0x92000,extra_slot=0x93000,hand_object=0x96000,
  interaction_node=0x98000,interaction_position=0x99000,pack=0x9a000;
constexpr std::uint32_t symbol(std::uint32_t n) {return symbol_base+n*0x80;}
constexpr std::uint32_t name(std::uint32_t n) {return name_base+n*0x80;}
constexpr std::uint32_t node(std::uint32_t n) {return 0xb0000+n*16;}
constexpr std::uint32_t object(std::uint32_t n) {return 0xc0000+n*0x400;}
std::uint32_t u32(const std::uint8_t* p) {return ubc::runtime_script::detail::u32(p);}
ubc::lhiver::Digest hash(const void* data,std::size_t size) {ubc::Sha256 h;h.update(data,size);return h.finish();}

struct Image {
  std::map<std::uint32_t,std::vector<std::uint8_t>> blocks;
  std::uint32_t denied=0,watch=0,change_target=0,change_value=0;
  unsigned read_count=0,watch_reads=0,change_on=0;
  void (*callback)(Image&) noexcept=nullptr;
  void block(std::uint32_t a,std::size_t n) {blocks[a].resize(n);}
  std::uint8_t* at(std::uint32_t a,std::size_t n) {
    auto found=blocks.upper_bound(a);if(found==blocks.begin())return nullptr;--found;
    const auto offset=static_cast<std::size_t>(a-found->first);
    return offset<=found->second.size() && n<=found->second.size()-offset?found->second.data()+offset:nullptr;
  }
  void put(std::uint32_t a,std::uint32_t v) {
    auto* p=at(a,4);if(!p)std::abort();for(unsigned i=0;i<4;++i)p[i]=static_cast<std::uint8_t>(v>>(i*8));
  }
  void text(std::uint32_t a,const std::string& s) {block(a,s.size()+1);std::memcpy(at(a,s.size()+1),s.c_str(),s.size()+1);}
  void zstring(std::uint32_t a,std::uint32_t data,const std::string& s) {text(data,s);put(a+8,data);put(a+12,static_cast<std::uint32_t>(s.size()));put(a+16,static_cast<std::uint32_t>(s.size()+1));}
  static bool read(void* p,std::uint32_t address,void* out,std::size_t n) noexcept {
    auto& self=*static_cast<Image*>(p);++self.read_count;
    if(self.callback) {auto callback=self.callback;self.callback=nullptr;callback(self);}
    if(address==self.watch && ++self.watch_reads==self.change_on)self.put(self.change_target,self.change_value);
    if(self.denied && address<=self.denied && std::uint64_t{self.denied}<std::uint64_t{address}+n)return false;
    auto* bytes=self.at(address,n);if(!bytes)return false;std::memcpy(out,bytes,n);return true;
  }
  ubc::ReadOnlyMemory memory() {return {this,&read};}
};

// Optional independent comparison. Open the original files explicitly
// read-only. Contents stay in RAM. Output neither bytes nor text.
struct LocalFacts {
  std::vector<std::uint8_t> exe,bytecode;
  std::array<std::string,3> flask_text{};
  std::uint32_t section_table=0,section_count=0;
  std::vector<std::uint8_t> read_file(const char* path,std::size_t maximum) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)std::abort();
    const auto n=file.tellg();if(n<=0 || static_cast<std::uint64_t>(n)>maximum)std::abort();
    std::vector<std::uint8_t> result(static_cast<std::size_t>(n));file.seekg(0);
    file.read(reinterpret_cast<char*>(result.data()),n);if(!file)std::abort();return result;
  }
  const std::uint8_t* native(std::uint32_t address,std::size_t n) const {
    if(address<0x400000)std::abort();const auto rva=address-0x400000;
    for(std::uint32_t i=0;i<section_count;++i) {
      const auto at=section_table+i*40;if(std::uint64_t{at}+40>exe.size())std::abort();
      const auto start=u32(exe.data()+at+12),length=u32(exe.data()+at+16),raw=u32(exe.data()+at+20);
      if(rva>=start && rva-start<=length && n<=length-(rva-start)) {
        const auto offset=std::uint64_t{raw}+rva-start;if(offset+n>exe.size())std::abort();return exe.data()+offset;
      }
    }
    std::abort();
  }
  LocalFacts(const char* exe_path,const char* archive_path) {
    exe=read_file(exe_path,16*1024*1024);
    if(hash(exe.data(),exe.size())!=ubc::station_abi::executable_sha256 || exe.size()<0x100)std::abort();
    const auto pe=u32(exe.data()+0x3c);if(std::uint64_t{pe}+24>exe.size() || u32(exe.data()+pe)!=0x4550)std::abort();
    section_count=exe[pe+6]|(std::uint32_t{exe[pe+7]}<<8);
    const auto opt=exe[pe+20]|(std::uint32_t{exe[pe+21]}<<8);section_table=pe+24+opt;
    if(!section_count || section_count>32 || std::uint64_t{section_table}+section_count*40>exe.size())std::abort();
    std::ifstream archive(archive_path,std::ios::binary|std::ios::ate);if(!archive)std::abort();const auto archive_size=archive.tellg();
    std::array<std::uint8_t,296> head{};archive.seekg(0);archive.read(reinterpret_cast<char*>(head.data()),head.size());if(!archive)std::abort();
    const auto count=u32(head.data()+272),catalogue=u32(head.data()+288),width=u32(head.data()+292);
    if(count>20000 || width!=80 || std::uint64_t{catalogue}+std::uint64_t{count}*width>static_cast<std::uint64_t>(archive_size))std::abort();
    std::vector<std::uint8_t> entries(static_cast<std::size_t>(count)*width);archive.seekg(catalogue);
    archive.read(reinterpret_cast<char*>(entries.data()),static_cast<std::streamsize>(entries.size()));if(!archive)std::abort();
    std::uint32_t data_offset=0,data_size=0;unsigned matches=0;
    for(std::uint32_t i=0;i<count;++i) {
      const auto* entry=entries.data()+i*80;std::size_t length=64;
      while(length && (entry[length-1]==0 || entry[length-1]==' '))--length;
      if(length==10 && std::memcmp(entry,"GOTHIC.DAT",10)==0) {++matches;data_offset=u32(entry+64);data_size=u32(entry+68);}
    }
    if(matches!=1 || data_size>16*1024*1024 || std::uint64_t{data_offset}+data_size>static_cast<std::uint64_t>(archive_size))std::abort();
    std::vector<std::uint8_t> dat(data_size);archive.seekg(data_offset);archive.read(reinterpret_cast<char*>(dat.data()),data_size);
    if(!archive || hash(dat.data(),dat.size())!=ubc::lhiver::dat_sha256)std::abort();
    std::size_t at=0;
    auto skip=[&](std::size_t n){if(at>dat.size() || n>dat.size()-at)std::abort();at+=n;};
    auto number=[&](){const auto before=at;skip(4);return u32(dat.data()+before);};
    auto line=[&](){const auto before=at;while(at<dat.size() && dat[at]!=10)++at;if(at==dat.size())std::abort();const auto end=at++;return std::string(reinterpret_cast<const char*>(dat.data()+before),end-before);};
    skip(1);const auto symbol_count=number();if(symbol_count!=ubc::lhiver::symbol_count)std::abort();skip(static_cast<std::size_t>(symbol_count)*4);
    for(std::uint32_t i=0;i<symbol_count;++i) {
      const auto named=number();if(named>1)std::abort();const auto symbol_name=named?line():std::string{};
      if(i==46447)CHECK(symbol_name==std::string("\xff""16030"));
      if(i==46448)CHECK(symbol_name==std::string("\xff""16031"));
      static_cast<void>(number());const auto props=number(),elements=props&0xfff,type=(props>>12)&15,flags=(props>>16)&63;skip(20);
      if(!(flags&4)) {
        if(type==1 || type==2)skip(elements*4);
        else if(type==3)for(std::uint32_t k=0;k<elements;++k) {auto value=line();for(std::size_t s=0;s<rt::flask_strings.size();++s)if(i==rt::flask_strings[s].symbol && !k)flask_text[s]=value;}
        else if(type>=4 && type<=7)skip(4);
        else if(type>7)std::abort();
      }
      skip(4);
    }
    const auto length=number();if(length!=ubc::lhiver::code_length || dat.size()-at!=length)std::abort();
    bytecode.assign(dat.begin()+static_cast<std::ptrdiff_t>(at),dat.end());
    for(std::size_t i=0;i<flask_text.size();++i)CHECK(hash(flask_text[i].data(),flask_text[i].size())==rt::flask_strings[i].digest);
  }
};

struct Fixture {
  Image image;
  const cat::Descriptor& recipe;
  rt::Boundary boundary{{1,7,ubc::lhiver::game_parser,game,world,hero,mob},{1,1,0,1,0,0,true},1,7,true,true,true};
  explicit Fixture(const cat::Descriptor& r,const LocalFacts* facts=nullptr):recipe(r) {
    boundary.scope.callback=r.callback.function->symbol;
    image.block(ubc::lhiver::game_parser,0x2060);image.block(table,(ubc::lhiver::symbol_count+20)*4);
    image.put(ubc::lhiver::game_parser+0x18,table);image.put(ubc::lhiver::game_parser+0x1c,ubc::lhiver::symbol_count+20);image.put(ubc::lhiver::game_parser+0x20,ubc::lhiver::symbol_count+13);
    image.put(ubc::lhiver::game_parser+0x48,code);image.put(ubc::lhiver::game_parser+0x4c,code+100);
    image.put(ubc::lhiver::game_parser+0x50,code+ubc::lhiver::code_length+68);image.put(ubc::lhiver::game_parser+0x54,ubc::lhiver::code_length+128);
    for(const auto& b:rt::flask_bindings) {
      image.block(symbol(b.symbol),0x3c);image.put(table+b.symbol*4,symbol(b.symbol));
      image.zstring(symbol(b.symbol),name(b.symbol),b.name);
      image.put(symbol(b.symbol)+0x20,b.elements|(b.type<<12)|(b.flags<<16));image.put(symbol(b.symbol)+0x38,b.parent_index<0?0:symbol(static_cast<std::uint32_t>(b.parent_index)));
      if(b.check_value)image.put(symbol(b.symbol)+0x18,static_cast<std::uint32_t>(b.value));
      if(b.check_offset)image.put(symbol(b.symbol)+0x1c,static_cast<std::uint32_t>(b.offset));
    }
    const auto* f=rt::flask_constructor;image.put(symbol(16604)+0x18,f->byte_offset);image.put(symbol(16604)+0x1c,hand_object);
    image.block(code+f->byte_offset,f->byte_length);auto* body=image.at(code+f->byte_offset,f->byte_length);
    if(facts)std::memcpy(body,facts->bytecode.data()+f->byte_offset,f->byte_length);else std::fill(body,body+f->byte_length,0x31);
    for(std::size_t i=0;i<rt::flask_strings.size();++i) {
      const auto& s=rt::flask_strings[i];const auto address=strings+static_cast<std::uint32_t>(i)*0x100;
      image.block(address,20);image.put(symbol(s.symbol)+0x18,address);
      image.zstring(address,address+0x40,facts?facts->flask_text[i]:std::string(s.length,'X'));
    }
    image.put(symbol(1679)+0x1c,hero);image.put(symbol(1683)+0x1c,hero);
    image.put(symbol(1680)+0x1c,0x77000);image.put(symbol(1681)+0x1c,0);
    for(auto address:{rt::current_parser_pointer,ubc::station_abi::instance_symbol_pointer,ubc::station_abi::instance_address_pointer,
        ubc::station_abi::hero_pointer,ubc::selection_abi::game_pointer})image.block(address,4);
    image.put(rt::current_parser_pointer,ubc::lhiver::game_parser);image.put(ubc::station_abi::instance_symbol_pointer,symbol(1679));
    image.put(ubc::station_abi::instance_address_pointer,hero);image.put(ubc::station_abi::hero_pointer,hero);image.put(ubc::selection_abi::game_pointer,game);
    image.block(game,0x100);image.put(game,ubc::selection_abi::game_vtable);image.put(game+8,world);
    image.block(hero,0xa00);image.put(hero+0x964,mob);image.put(hero+0x668,rt::inventory_vtable);
    image.block(mob,0x220);image.put(mob+0x1f4,1);image.put(mob+0x1f8,1);image.put(mob+0x1fc,1);
    image.zstring(mob+0x1e0,0x990,"POTIONALCHEMY");image.zstring(mob+0x1a4,0x9b0,"ITMI_FLASK");image.zstring(mob+0x1cc,0x9d0,"");
    image.block(interaction_node,8);image.block(interaction_position,0x48);image.put(mob+0x18c,interaction_node);
    image.put(interaction_node,interaction_position);image.put(interaction_position+0x44,hero);
    image.block(slot_array,12);image.block(left_slot,0x38);image.block(right_slot,0x38);image.block(extra_slot,0x38);
    image.put(hero+0x9b0,slot_array);image.put(hero+0x9b4,3);image.put(hero+0x9b8,2);
    image.put(slot_array,left_slot);image.put(slot_array+4,right_slot);image.put(slot_array+8,extra_slot);
    image.zstring(left_slot,0x94000,"ZS_LEFTHAND");image.zstring(right_slot,0x94100,"ZS_RIGHTHAND");image.zstring(extra_slot,0x94200,"ZS_OTHER");
    image.block(hand_object,0x338);image.put(hand_object,ubc::hand_detail::item_vtable);image.put(hand_object+0x32c,1);image.put(hand_object+0x330,16604);
    image.put(right_slot+0x30,hand_object);image.put(hero+0x968,hand_object);
    for(const auto& range:rt::code_ranges) {
      image.block(range.address,range.length);auto* bytes=image.at(range.address,range.length);
      if(facts)std::memcpy(bytes,facts->native(range.address,range.length),range.length);else std::fill(bytes,bytes+range.length,0x31);
    }
    // The existing bounded hand reader checks its own getter prologue.
    std::memcpy(image.at(ubc::hand_detail::item_class_method,6),ubc::hand_detail::item_class_bytes.data(),6);
    for(const auto& entry:rt::vtable_entries) {image.block(entry.address,4);image.put(entry.address,entry.target);}
    std::vector<std::pair<std::int32_t,std::int32_t>> stock;
    for(const auto& cost:r.costs)stock.push_back({cost.symbol,cost.symbol==16604?5:static_cast<std::int32_t>(cost.units*20)});
    stock.push_back({r.outputs[0].symbol,2});storage(stock,"");set_production(4);
  }
  void set_production(std::int32_t value) {image.put(symbol(12330)+0x18,static_cast<std::uint32_t>(value));image.put(hero+0x298,value==4?1:0);}
  void storage(const std::vector<std::pair<std::int32_t,std::int32_t>>& values,const std::string& packed) {
    image.put(hero+0x708,hero);image.put(hero+0x70c,1);image.put(hero+0x718,values.empty()?0:node(0));
    image.put(hero+0x724,pack);image.put(hero+0x728,static_cast<std::uint32_t>(packed.size()));image.text(pack,packed);
    for(std::uint32_t i=0;i<values.size();++i) {
      image.block(node(i),12);image.block(object(i),0x338);image.put(node(i)+4,object(i));image.put(node(i)+8,i+1<values.size()?node(i+1):0);
      image.put(object(i),ubc::hand_detail::item_vtable);image.put(object(i)+0x158,0x200001);
      image.put(object(i)+0x32c,static_cast<std::uint32_t>(values[i].second));image.put(object(i)+0x330,static_cast<std::uint32_t>(values[i].first));
    }
  }
  void original_success(std::uint32_t quantity) {
    for(std::uint32_t i=0;i<recipe.costs.count;++i)if(recipe.costs[i].symbol!=16604)
      image.put(object(i)+0x32c,recipe.costs[i].units*(20-quantity));
    image.put(object(static_cast<std::uint32_t>(recipe.costs.count))+0x32c,2+quantity);set_production(0);
    const auto words=debit::original_return_words(recipe.callback.function->symbol);
    image.put(ubc::lhiver::game_parser+0x2058,words);
    for(std::uint32_t i=0;i<words;++i)image.put(ubc::lhiver::game_parser+0x58+i*4,i%2?0x40u:1u);
  }
  std::uint32_t flask_object() const {return object(static_cast<std::uint32_t>(recipe.costs.count-1));}
};
rt::Reader reader(7);
void reenter(Image& image) noexcept {
  Fixture other(cat::recipes[0]);const auto before=image.read_count;
  const auto v=reader.capture(image.memory(),other.recipe,other.boundary);
  CHECK(v.issue==rt::Issue::reentry && image.read_count==before);
}

void primitive_readers() {
  Fixture f(cat::recipes[0]);auto p=rt::read_parser(f.image.memory(),f.boundary.context);
  CHECK(p.valid && p.value.self==hero && p.value.hero==hero && p.value.pop_pos==100 && p.value.data_depth==0);
  CHECK(p.value.self_symbol==symbol(1679) && p.value.hero_symbol==symbol(1683));
  {
    namespace t=ubc::tobacco;
    ubc::GrindCheck diagnostic;
    const auto mem=f.image.memory();const auto ctx=f.boundary.context;
    static ubc::cool_predebit::check::Report trace;
    auto entry=[&]{
      trace.count=0;trace.overflow=false;t::Checks gates(&trace),legacy(&trace);
      return t::parser_checks(mem,ctx,gates,legacy);
    };
    CHECK(t::lab_flags(mem,hero,true,diagnostic)&&entry()&&t::choice_entry(mem,ctx,diagnostic));
    f.image.put(ubc::lhiver::game_parser+0x2058,2);
    const auto words=ubc::lhiver::game_parser+ubc::station_abi::parser_data_stack;
    f.image.put(words,1);f.image.put(words+4,0x40);
    CHECK(entry()&&t::choice_entry(mem,ctx,diagnostic)&&!rt::read_parser(mem,ctx).valid);
    f.image.put(words,2);f.image.put(words+4,0);f.image.put(rt::current_parser_pointer,0);
    CHECK(!entry()&&!t::choice_entry(mem,ctx,diagnostic));
    unsigned failures=0;
    for(unsigned i=0;i<trace.count;++i)if(!trace.rows[i].matches&&
      (std::string_view(trace.rows[i].field)=="parser.current"||std::string_view(trace.rows[i].field)=="parser.stack_value"||
       std::string_view(trace.rows[i].field)=="parser.stack_type"))++failures;
    CHECK(failures==3);
    f.image.put(rt::current_parser_pointer,ubc::lhiver::game_parser);
    f.image.put(ubc::lhiver::game_parser+0x2058,0);
    f.image.put(ubc::station_abi::instance_symbol_pointer,symbol(16604));
    CHECK(entry()&&!t::choice_entry(mem,ctx,diagnostic));
    bool seen_use=false;
    for(unsigned i=0;i<trace.count;++i)
      if(std::string_view(trace.rows[i].field)=="legacy.use_symbol"&&!trace.rows[i].matches)seen_use=true;
    CHECK(seen_use);
    f.image.put(ubc::station_abi::instance_symbol_pointer,symbol(1679));
    f.set_production(0);diagnostic={};
    CHECK(t::lab_flags(mem,hero,false,diagnostic)&&rt::read_parser(mem,ctx).valid);
    f.image.put(hero+0x298,1);CHECK(!t::lab_flags(mem,hero,false,diagnostic));
    CHECK(diagnostic.expected==0&&diagnostic.actual==1);
    f.set_production(4);
  }
  CHECK(p.layout.count==ubc::lhiver::symbol_count+13 && p.layout.code_length==ubc::lhiver::code_length+68);
  f.image.put(ubc::lhiver::game_parser+0x2058,1);CHECK(!rt::read_parser(f.image.memory(),f.boundary.context).valid);
  f.image.put(ubc::lhiver::game_parser+0x2058,0);f.image.put(ubc::station_abi::instance_symbol_pointer,symbol(16604));CHECK(!rt::read_parser(f.image.memory(),f.boundary.context).valid);
  f.image.put(ubc::station_abi::instance_symbol_pointer,symbol(1679));f.image.put(symbol(1683)+0x1c,hero+4);CHECK(!rt::read_parser(f.image.memory(),f.boundary.context).valid);
  f.image.put(symbol(1683)+0x1c,hero);f.image.denied=symbol(1681)+0x1c;CHECK(!rt::read_parser(f.image.memory(),f.boundary.context).valid);
  f.image.denied=0;f.image.put(symbol(1681)+0x38,0);CHECK(!rt::read_parser(f.image.memory(),f.boundary.context).valid);
  f.image.put(symbol(1681)+0x38,symbol(1537));f.image.put(ubc::lhiver::game_parser+0x18,0xfffffff0);CHECK(!rt::read_parser(f.image.memory(),f.boundary.context).valid);
  Fixture g(cat::recipes[0]);g.image.watch=ubc::station_abi::instance_symbol_pointer;g.image.change_on=1;g.image.change_target=symbol(1680)+0x1c;g.image.change_value=0;
  CHECK(!rt::read_parser(g.image.memory(),g.boundary.context).valid);
  Fixture h(cat::recipes[0]);const auto& c=rt::flask_strings[0];
  auto expected=c;expected.digest=hash(std::string(c.length,'X').data(),c.length);
  ubc::runtime_script::detail::Layout layout{};CHECK(ubc::runtime_script::detail::read_layout(h.image.memory(),ubc::lhiver::game_parser,rt::flask_profile,layout)==ubc::runtime_script::Issue::none);
  CHECK(rt::detail::string_once(h.image.memory(),layout,expected).valid);CHECK(!rt::detail::string_once(h.image.memory(),layout,c).valid);
  h.image.put(strings+12,UINT32_MAX);CHECK(!rt::detail::string_once(h.image.memory(),layout,expected).valid);
  Image m;m.block(0x1000,32);std::fill(m.at(0x1000,32),m.at(0x1000,32)+32,0x71);
  rt::CodeRange range{0x1000,32,hash(m.at(0x1000,32),32),rt::RangeKind::complete_function};
  CHECK(rt::detail::code_range(m.memory(),range).valid);range.digest[0]^=1;CHECK(!rt::detail::code_range(m.memory(),range).valid);
  range.length=1025;CHECK(rt::detail::code_range(m.memory(),range).issue==rt::Issue::range);
  range={UINT32_MAX,4,{},rt::RangeKind::reviewed_site};CHECK(rt::detail::code_range(m.memory(),range).issue==rt::Issue::range);
  Fixture own(cat::recipes[0]);CHECK(!rt::verify_native(own.image.memory()).reviewed_ranges_match);
  CHECK(!rt::verify_flask_constructor(own.image.memory()).constructor_and_bindings_match);
  auto capture=reader.capture(own.image.memory(),own.recipe,own.boundary);
  CHECK(!capture.valid && !capture.observation.stable_read && !capture.observation.native_contract);
  CHECK(capture.observation.stocks.complete_unambiguous && capture.observation.stocks.rows[0].stored==20);
  own.boundary.current_thread=8;const auto before=own.image.read_count;
  CHECK(reader.capture(own.image.memory(),own.recipe,own.boundary).issue==rt::Issue::thread && own.image.read_count==before);
  own.boundary.current_thread=7;own.boundary.synchronous_writer_verified=false;
  CHECK(reader.capture(own.image.memory(),own.recipe,own.boundary).issue==rt::Issue::boundary && own.image.read_count==before);
  own.boundary.synchronous_writer_verified=true;own.image.callback=&reenter;static_cast<void>(reader.capture(own.image.memory(),own.recipe,own.boundary));
}

void parser_failure_diagnostics() {
  constexpr auto parser=ubc::lhiver::game_parser;
  for(const auto depth:{0u,2u,8u,9u,2048u}) {
    Fixture f(cat::recipes[0]);f.image.put(parser+0x2058,depth);
    for(std::uint32_t i=0;i<8;++i)f.image.put(parser+0x58+i*4,i%2?0x40u:1u);
    const auto failed=rt::read_parser(f.image.memory(),f.boundary.context);
    CHECK(failed.valid==(depth==0));CHECK(failed.value.data_depth==depth);
    const auto d=rt::parser_failure_snapshot(f.image.memory(),f.boundary.context,failed.value);
    CHECK(d.words_stable && d.word_count==(depth<8?depth:8));
    CHECK(d.value.data_depth==depth && d.value.use_symbol==symbol(1679));
    for(std::uint32_t i=0;i<d.word_count;++i)CHECK(d.words[i]==(i%2?0x40u:1u));
    CHECK(rt::parser_failure_flags(d,f.boundary.context)==(1007u|(depth>8?1024u:0u)));
    CHECK(debit::valid(d.value,f.boundary.context)==(depth==0)); // Diagnostics do not extend any authorization.
    if(depth) {
      const auto captured=reader.capture(f.image.memory(),f.recipe,f.boundary);
      CHECK(!captured.valid && captured.issue==rt::Issue::parser && captured.observation.parser.data_depth==depth);
      CHECK(captured.observation.parser.known && !captured.observation.native_contract && !captured.observation.stable_read);
    }
  }
  for(unsigned mode=0;mode<7;++mode) {
    Fixture f(cat::recipes[0]);f.image.put(parser+0x2058,2);f.image.put(parser+0x58,1);f.image.put(parser+0x5c,0x40);
    const auto failed=rt::read_parser(f.image.memory(),f.boundary.context);CHECK(!failed.valid);
    auto saved=failed.value;
    switch(mode) {
      case 0:f.image.denied=parser+0x58;break;
      case 1:f.image.watch=parser+0x58;f.image.change_on=2;f.image.change_target=parser+0x58;f.image.change_value=2;break;
      case 2:f.image.put(parser+0x2058,3);break;
      case 3:f.image.put(ubc::station_abi::instance_symbol_pointer,symbol(1683));break;
      case 4:saved.known=false;break;
      case 5:saved.data_depth=2049;break;
      case 6:saved.current_parser=parser+4;break;
    }
    const auto d=rt::parser_failure_snapshot(f.image.memory(),f.boundary.context,saved);
    CHECK(!d.words_stable && !d.word_count && (d.words==std::array<std::uint32_t,8>{}));
    CHECK(d.value.data_depth==saved.data_depth); // Keep the failure visible. Never substitute zero.
  }
  Fixture invalid(cat::recipes[0]);invalid.image.put(ubc::station_abi::instance_symbol_pointer,symbol(16604));
  const auto captured=reader.capture(invalid.image.memory(),invalid.recipe,invalid.boundary);
  CHECK(!captured.valid && captured.issue==rt::Issue::parser && captured.observation.parser.use_symbol==symbol(16604));
  CHECK(!debit::valid(captured.observation.parser,invalid.boundary.context));
  for(const auto word:{0u,1u,0x40u,0x41u})CHECK(rt::parser_failure_log_word(word)==static_cast<int>(word));
  for(const auto word:{2u,0xdeadbeefu,parser,UINT32_MAX})CHECK(rt::parser_failure_log_word(word)==-1);
}

void exact_prefix_reader() {
  constexpr auto parser=ubc::lhiver::game_parser;
  constexpr std::array<std::uint32_t,13> counts{{8,8,6,4,4,4,4,4,4,4,2,4,6}};
  for(std::size_t index=0;index<cat::recipes.size();++index) {
    Fixture f(cat::recipes[index]);const auto callback=f.recipe.callback.function->symbol;
    CHECK(rt::read_parser(f.image.memory(),f.boundary.context).valid);
    CHECK(!rt::read_parser(f.image.memory(),f.boundary.context,rt::ParserStackMode::original_returns,callback).valid);
    f.original_success(2);auto good=rt::read_parser(f.image.memory(),f.boundary.context,rt::ParserStackMode::original_returns,callback);
    CHECK(good.valid && good.value.data_words_known && good.value.data_depth==counts[index]);
    CHECK(!rt::read_parser(f.image.memory(),f.boundary.context).valid);
    CHECK(!rt::read_parser(f.image.memory(),f.boundary.context,rt::ParserStackMode::original_returns,20370).valid);
    CHECK(!rt::read_parser(f.image.memory(),f.boundary.context,static_cast<rt::ParserStackMode>(99),callback).valid);
    for(std::uint32_t i=0;i<counts[index];++i) {
      const auto address=parser+0x58+i*4,value=i%2?0x40u:1u;
      f.image.put(address,value^1);CHECK(!rt::read_parser(f.image.memory(),f.boundary.context,rt::ParserStackMode::original_returns,callback).valid);
      f.image.put(address,value);f.image.denied=address;
      CHECK(!rt::read_parser(f.image.memory(),f.boundary.context,rt::ParserStackMode::original_returns,callback).valid);f.image.denied=0;
    }
    f.image.watch=parser+0x58;f.image.watch_reads=0;f.image.change_on=2;
    f.image.change_target=parser+0x58;f.image.change_value=0;
    CHECK(!rt::read_parser(f.image.memory(),f.boundary.context,rt::ParserStackMode::original_returns,callback).valid);
  }
}

void positive_local_contract(const LocalFacts& facts) {
  for(const auto& r:cat::recipes) {
    Fixture f(r,&facts);auto before=reader.capture(f.image.memory(),r,f.boundary);
    CHECK(before.valid && before.observation.native_contract && before.observation.stable_read);
    CHECK(before.observation.flask.known && before.observation.flask.packed==0 && before.observation.flask.listed_stacks==1);
    CHECK(before.observation.flask.object==f.flask_object() && before.observation.flask.amount==5);
    CHECK(before.observation.hand.right==hand_object && before.observation.hand.interact_item==hand_object);
    debit::Tracker tracker;debit::Admission a{true,true,true,true,true,true};
    debit::Job job{f.boundary.context,1,1,r.callback.function->symbol,static_cast<std::uint32_t>(r.owner.instance),3};
    CHECK(tracker.arm(job,a,before.observation));
    f.original_success(3);f.boundary.serial=2;auto original=reader.capture(f.image.memory(),r,f.boundary,{},rt::ParserStackMode::original_returns);
    CHECK(original.valid && tracker.original_returned(original.observation,true,true));
    f.boundary.serial=3;auto late=reader.capture(f.image.memory(),r,f.boundary,{},rt::ParserStackMode::original_returns);const auto call=tracker.begin_debit(late.observation);
    CHECK(call.valid && call.quantity==2 && call.stack_object==f.flask_object());
    f.boundary.scope.debit_depth=1;
    f.image.put(ubc::lhiver::game_parser+0x4c,code+rt::flask_constructor->byte_offset+rt::flask_constructor->byte_length);
    f.image.put(ubc::station_abi::instance_symbol_pointer,symbol(16604));f.image.put(ubc::station_abi::instance_address_pointer,0x70000000);
    const auto restore=reader.restore_targets(f.image.memory(),call,f.boundary);
    CHECK(restore.valid && debit::restorable(call,restore.observation));
    f.image.put(ubc::lhiver::game_parser+0x58,0);
    CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid);
    f.image.put(ubc::lhiver::game_parser+0x58,1);
    f.image.put(ubc::lhiver::game_parser+0x2058,call.parser.data_depth+4);
    CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid); // SEH at the constructor's peak stack depth.
    f.image.put(ubc::lhiver::game_parser+0x2058,call.parser.data_depth);
    f.boundary.native_dependencies_and_configured_targets_verified=false;
    CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid);
    f.boundary.native_dependencies_and_configured_targets_verified=true;
    ++f.boundary.scope.writer_token;CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid);--f.boundary.scope.writer_token;
    f.boundary.current_thread=8;const auto reads=f.image.read_count;
    CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid && f.image.read_count==reads);f.boundary.current_thread=7;
    auto wrong_call=call;wrong_call.parser.use_symbol=symbol(16604);CHECK(!reader.restore_targets(f.image.memory(),wrong_call,f.boundary).valid);
    f.image.put(ubc::lhiver::game_parser+0x48,code+4);CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid);f.image.put(ubc::lhiver::game_parser+0x48,code);
    f.image.put(hero+0x964,0);CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid);f.image.put(hero+0x964,mob);
    f.image.put(symbol(1680)+0x1c,0);CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid);f.image.put(symbol(1680)+0x1c,0x77000);
    f.image.put(symbol(1679)+0x1c,hero+4);CHECK(!reader.restore_targets(f.image.memory(),call,f.boundary).valid);f.image.put(symbol(1679)+0x1c,hero);
    f.image.put(ubc::lhiver::game_parser+0x4c,code+call.parser.pop_pos);f.image.put(ubc::station_abi::instance_symbol_pointer,call.parser.use_symbol);
    f.image.put(ubc::station_abi::instance_address_pointer,call.parser.use_address);f.image.put(f.flask_object()+0x32c,3);
    f.boundary.serial=4;auto after=reader.capture(f.image.memory(),r,f.boundary,{},rt::ParserStackMode::original_returns);
    CHECK(after.valid && tracker.debit_returned(after.observation,true,true));
    CHECK(tracker.status().stage==debit::Stage::debit_verified && !tracker.status().blocked);
  }
}

void rejection_cases(const LocalFacts& facts) {
  const auto& r=cat::recipes[0];
  auto reject=[&](auto mutate,rt::Issue expected) {Fixture f(r,&facts);mutate(f);const auto result=reader.capture(f.image.memory(),r,f.boundary);CHECK(!result.valid && !result.observation.stable_read && !result.observation.native_contract);CHECK(result.issue==expected);};
  reject([](Fixture& f){f.image.put(rt::code_ranges[0].address,0);},rt::Issue::native_code);
  reject([](Fixture& f){f.image.put(rt::vtable_entries[0].address,0);},rt::Issue::vtable);
  reject([](Fixture& f){f.image.put(code+rt::flask_constructor->byte_offset,0);},rt::Issue::script);
  reject([](Fixture& f){f.image.put(symbol(1591)+0x1c,0);},rt::Issue::script);
  reject([](Fixture& f){f.image.put(symbol(1056)+0x18,1);},rt::Issue::script);
  reject([](Fixture& f){f.image.zstring(symbol(46447),name(46447),"\xff""16030");},rt::Issue::script);
  reject([](Fixture& f){f.image.zstring(symbol(46448),name(46448),"\xff""16031");},rt::Issue::script);
  reject([](Fixture& f){f.image.put(strings+12,2);},rt::Issue::string);
  reject([](Fixture& f){f.image.put(ubc::selection_abi::game_pointer,0);},rt::Issue::context);
  reject([](Fixture& f){f.image.put(game+8,world+4);},rt::Issue::context);
  reject([](Fixture& f){f.image.put(hero+0x964,mob+4);},rt::Issue::context);
  reject([](Fixture& f){f.image.put(hero+0x668,0);},rt::Issue::context);
  reject([](Fixture& f){f.image.put(mob+0x1f4,0);},rt::Issue::station);
  reject([](Fixture& f){f.image.put(interaction_node+4,interaction_node);},rt::Issue::station);
  reject([](Fixture& f){f.image.put(interaction_position+0x44,0);},rt::Issue::station);
  reject([](Fixture& f){f.image.put(mob+0x1a4+12,0);},rt::Issue::station);
  reject([](Fixture& f){f.image.put(hero+0x708,0);},rt::Issue::inventory);
  reject([](Fixture& f){f.image.put(node(0)+8,node(0));},rt::Issue::inventory);
  reject([](Fixture& f){f.image.put(f.flask_object()+0x158,0x40200001);},rt::Issue::flask);
  reject([](Fixture& f){f.image.put(f.flask_object()+0x158,1);},rt::Issue::flask);
  reject([](Fixture& f){f.image.put(f.flask_object(),0);},rt::Issue::flask);
  reject([](Fixture& f){f.image.put(hero+0x9b8,3);f.image.put(extra_slot+0x30,f.flask_object());},rt::Issue::alias);
  reject([](Fixture& f){f.image.put(left_slot+0x30,hand_object);},rt::Issue::hands);
  reject([](Fixture& f){f.image.put(hero+0x968,0);},rt::Issue::hands);
  reject([](Fixture& f){f.image.put(hero+0x9b4,257);},rt::Issue::hands);
  reject([](Fixture& f){f.image.put(right_slot+12,64);},rt::Issue::hands);
  reject([](Fixture& f){f.image.put(hand_object+0x32c,2);},rt::Issue::hands);
  reject([](Fixture& f){f.image.put(node(0)+4,hand_object);},rt::Issue::inventory);
  reject([](Fixture& f){f.image.denied=pack;},rt::Issue::inventory);
  reject([](Fixture& f){f.image.put(hero+0x728,ubc::max_packed_inventory_bytes+1);},rt::Issue::inventory);
  reject([](Fixture& f){f.boundary.native_dependencies_and_configured_targets_verified=false;},rt::Issue::dependency_evidence);
  reject([](Fixture& f){f.image.watch=ubc::station_abi::instance_symbol_pointer;f.image.change_on=1;f.image.change_target=symbol(1680)+0x1c;f.image.change_value=0;},rt::Issue::changed);
  Fixture mixed(r,&facts);mixed.image.text(pack,"ITMI_FLASK:2");mixed.image.put(hero+0x728,12);
  CHECK(!reader.capture(mixed.image.memory(),r,mixed.boundary).valid);
  // Packed stock is known stock. Do not assume zero. The existing
  // tracker still forbids N>1 even if the read-only snapshot succeeds.
  Fixture packed_only(r,&facts);std::vector<std::pair<std::int32_t,std::int32_t>> values;
  for(const auto& c:r.costs)if(c.symbol!=16604)values.push_back({c.symbol,static_cast<std::int32_t>(c.units*20)});
  values.push_back({r.outputs[0].symbol,2});packed_only.storage(values,"ITMI_FLASK:5");
  auto v=reader.capture(packed_only.image.memory(),r,packed_only.boundary);CHECK(v.valid && v.observation.flask.packed==5 && v.observation.flask.listed_stacks==0);
  debit::Tracker tracker;debit::Job job{packed_only.boundary.context,1,1,r.callback.function->symbol,static_cast<std::uint32_t>(r.owner.instance),2};
  CHECK(!tracker.arm(job,{true,true,true,true,true,true},v.observation));
  job.quantity=1;CHECK(tracker.arm(job,{true,true,true,true,true,true},v.observation));
  Fixture empty(r,&facts);values.clear();for(const auto& c:r.costs)if(c.symbol!=16604)values.push_back({c.symbol,static_cast<std::int32_t>(c.units*20)});
  values.push_back({r.outputs[0].symbol,2});empty.storage(values,"");v=reader.capture(empty.image.memory(),r,empty.boundary);
  CHECK(v.valid && debit::listed_flask(v.observation.flask,0));
  Fixture changed(r,&facts);changed.image.watch=hero+0x708;changed.image.change_on=3;changed.image.change_target=changed.flask_object()+0x32c;changed.image.change_value=4;
  const auto changed_result=reader.capture(changed.image.memory(),r,changed.boundary);CHECK(!changed_result.valid && changed_result.issue==rt::Issue::changed);
}
void native_policy_cases(const LocalFacts& facts) {
  Fixture f(cat::recipes[0],&facts);
  constexpr std::uint32_t base=0x50000000;
  f.image.block(base,4096);f.image.put(base,0x5a4d);f.image.put(base+60,128);
  f.image.put(base+128,0x4550);f.image.put(base+132,0x14c);f.image.put(base+136,0x68bd012a);
  f.image.put(base+148,224);f.image.put(base+152,0x10b);f.image.put(base+180,0x10000000);
  f.image.put(base+208,ubc::zbug_end_compat::zbug_image_size);
  rt::NativeCodePolicy policy{rt::NativeCodeMode::known_zbug_remove_site,
    {base,ubc::zbug_end_compat::zbug_image_size,ubc::zbug_end_compat::zbug_file_sha256,true}};
  CHECK(rt::verify_native(f.image.memory(),policy).reviewed_ranges_match);
  CHECK(!rt::verify_native(f.image.memory(),policy).zbug_unpack_call);
  f.image.put(0x70d0e7,base+0x11e30u-0x70d0ebu);
  CHECK(!rt::verify_native(f.image.memory()).reviewed_ranges_match);
  auto matched=rt::verify_native(f.image.memory(),policy);
  CHECK(matched.reviewed_ranges_match && matched.vtable_slots_match && matched.zbug_unpack_call);
  auto bad=policy;bad.zbug.mapping_verified=false;CHECK(!rt::verify_native(f.image.memory(),bad).reviewed_ranges_match);
  bad=policy;bad.zbug.file_sha256[0]^=1;CHECK(!rt::verify_native(f.image.memory(),bad).reviewed_ranges_match);
  bad=policy;bad.zbug.image_size++;CHECK(!rt::verify_native(f.image.memory(),bad).reviewed_ranges_match);
  bad=policy;bad.mode=static_cast<rt::NativeCodeMode>(99);CHECK(!rt::verify_native(f.image.memory(),bad).reviewed_ranges_match);
  f.image.put(base+136,0);CHECK(!rt::verify_native(f.image.memory(),policy).reviewed_ranges_match);f.image.put(base+136,0x68bd012a);
  f.image.put(0x70d0e7,base+0x11e31u-0x70d0ebu);CHECK(!rt::verify_native(f.image.memory(),policy).reviewed_ranges_match);
  f.image.put(0x70d0e7,base+0x11e30u-0x70d0ebu);
  const auto saved=*f.image.at(0x70d080,1);*f.image.at(0x70d080,1)^=1;
  CHECK(!rt::verify_native(f.image.memory(),policy).reviewed_ranges_match);*f.image.at(0x70d080,1)=saved;
  f.image.watch=0x70d080;f.image.change_on=3;f.image.change_target=0x70d0e7;f.image.change_value=0x70f8f0u-0x70d0ebu;
  matched=rt::verify_native(f.image.memory(),policy);CHECK(!matched.reviewed_ranges_match && matched.issue==rt::Issue::changed);
}
}
int main(int argc,char** argv) {
  static_assert(!std::is_copy_constructible<rt::Reader>::value,"persistent workspace is noncopyable");
  primitive_readers();
  parser_failure_diagnostics();
  exact_prefix_reader();
  if(argc==4 && std::strcmp(argv[1],"--local-facts")==0) {LocalFacts facts(argv[2],argv[3]);positive_local_contract(facts);rejection_cases(facts);native_policy_cases(facts);}
  else if(argc!=1) {std::fprintf(stderr,"Optional read-only: --local-facts EXE ARCHIVE. No process access, writes or enginecalls.\n");return 2;}
  std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
