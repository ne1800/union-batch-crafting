// SPDX-License-Identifier: MIT
#pragma once
#define UBC_BOW_RUNTIME_FIXTURE_ONLY
#include "bow_recipe_runtime_tests.cpp"
#include <fstream>

// Test RAM only. A synthetically bound PE here does NOT establish a
// loaded module or game world. Never write or output the original files.
namespace bow_item_test {
using Image=station_test::Image;using station_test::check;
inline std::uint32_t u32(const std::uint8_t* b){return ubc::runtime_script::detail::u32(b);}
inline ubc::lhiver::Digest hash(const void* data,std::size_t n){ubc::Sha256 s;s.update(data,n);return s.finish();}
inline std::vector<std::uint8_t> file(const char* name,std::size_t maximum){
  std::ifstream in(name,std::ios::binary|std::ios::ate);if(!in)std::abort();const auto size=in.tellg();
  if(size<=0||static_cast<std::uint64_t>(size)>maximum)std::abort();std::vector<std::uint8_t> b(static_cast<std::size_t>(size));
  in.seekg(0);in.read(reinterpret_cast<char*>(b.data()),size);if(!in)std::abort();return b;
}
struct PE {
  std::vector<std::uint8_t> bytes;std::uint32_t base=0,size=0,nt=0,sections=0,count=0;
  PE(const char* path,const ubc::lhiver::Digest& expected):bytes(file(path,16*1024*1024)){
    if(hash(bytes.data(),bytes.size())!=expected||bytes.size()<256)std::abort();nt=get(60);if(get(nt)!=0x4550)std::abort();
    base=get(nt+52);size=get(nt+80);count=half(nt+6);sections=nt+24+half(nt+20);
    if(!count||count>32||size>16*1024*1024||static_cast<std::uint64_t>(sections)+40*count>bytes.size())std::abort();
  }
  std::uint32_t get(std::size_t at)const{if(at>bytes.size()||bytes.size()-at<4)std::abort();return u32(bytes.data()+at);}
  std::uint16_t half(std::size_t at)const{if(at>bytes.size()||bytes.size()-at<2)std::abort();return static_cast<std::uint16_t>(bytes[at]|(unsigned{bytes[at+1]}<<8));}
  const std::uint8_t* at(std::uint32_t address,std::size_t n)const{
    if(address<base)std::abort();const auto rva=address-base;
    if(rva<0x1000&&rva+n<=bytes.size())return bytes.data()+rva;
    for(std::uint32_t i=0;i<count;++i){const auto p=sections+i*40;const auto start=get(p+12),length=get(p+16),raw=get(p+20);
      if(rva>=start&&rva-start<=length&&n<=length-(rva-start)&&static_cast<std::uint64_t>(raw)+rva-start+n<=bytes.size())return bytes.data()+raw+rva-start;}
    std::abort();
  }
  void copy(Image& image,std::uint32_t address,std::size_t n)const{image.block(address,n);std::memcpy(image.at(address,n),at(address,n),n);}
  void load(Image& image,std::uint32_t relocated)const{
    image.block(relocated,size);std::memcpy(image.at(relocated,0x1000),bytes.data(),0x1000);
    for(std::uint32_t i=0;i<count;++i){const auto p=sections+i*40;const auto start=get(p+12),length=get(p+16),raw=get(p+20);
      if(static_cast<std::uint64_t>(raw)+length>bytes.size()||static_cast<std::uint64_t>(start)+length>size)std::abort();
      std::memcpy(image.at(relocated+start,length),bytes.data()+raw,length);}
    const auto reloc=get(nt+24+96+5*8),length=get(nt+24+100+5*8);std::uint32_t pos=0;
    while(pos<length){const auto* h=at(base+reloc+pos,8);const auto page=u32(h),block=u32(h+4);if(block<8||block>length-pos)std::abort();
      for(std::uint32_t i=8;i<block;i+=2){const auto* e=at(base+reloc+pos+i,2);const auto value=unsigned{e[0]}|(unsigned{e[1]}<<8);
        if((value>>12)==3){const auto a=relocated+page+(value&4095);image.put(a,u32(image.at(a,4))+relocated-base);}else if(value>>12)std::abort();}
      pos+=block;
    }
  }
};
struct Dat {
  std::vector<std::uint8_t> code;std::map<std::uint32_t,std::string> strings;
  explicit Dat(const char* path){
    std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)std::abort();const auto size=in.tellg();std::array<std::uint8_t,296> h{};
    in.seekg(0);in.read(reinterpret_cast<char*>(h.data()),h.size());if(!in)std::abort();const auto count=u32(h.data()+272),start=u32(h.data()+288);
    if(count>20000||u32(h.data()+292)!=80||static_cast<std::uint64_t>(start)+count*80>static_cast<std::uint64_t>(size))std::abort();
    std::vector<std::uint8_t> entries(count*80);in.seekg(start);in.read(reinterpret_cast<char*>(entries.data()),static_cast<std::streamsize>(entries.size()));
    unsigned found=0;std::uint32_t offset=0,length=0;
    for(std::uint32_t i=0;i<count;++i){const auto* p=entries.data()+80*i;std::size_t n=64;while(n&&(p[n-1]==0||p[n-1]==' '))--n;
      if(n==10&&!std::memcmp(p,"GOTHIC.DAT",10)){++found;offset=u32(p+64);length=u32(p+68);}}
    if(!in||found!=1||length>16*1024*1024||static_cast<std::uint64_t>(offset)+length>static_cast<std::uint64_t>(size))std::abort();
    std::vector<std::uint8_t> data(length);in.seekg(offset);in.read(reinterpret_cast<char*>(data.data()),length);
    if(!in||hash(data.data(),data.size())!=ubc::lhiver::dat_sha256)std::abort();std::size_t p=0;
    auto skip=[&](std::size_t n){if(p>data.size()||n>data.size()-p)std::abort();p+=n;};
    auto number=[&](){const auto old=p;skip(4);return u32(data.data()+old);};
    auto line=[&](){const auto old=p;while(p<data.size()&&data[p]!=10)++p;if(p==data.size())std::abort();const auto end=p++;return std::string(reinterpret_cast<const char*>(data.data()+old),end-old);};
    skip(1);const auto symbols=number();if(symbols!=ubc::lhiver::symbol_count)std::abort();skip(static_cast<std::size_t>(symbols)*4);
    for(std::uint32_t i=0;i<symbols;++i){const auto named=number();if(named>1)std::abort();if(named)static_cast<void>(line());
      static_cast<void>(number());const auto props=number(),elements=props&4095,type=(props>>12)&15,flags=(props>>16)&63;skip(20);
      if(!(flags&4)){if(type==1||type==2)skip(elements*4);else if(type==3)for(std::uint32_t k=0;k<elements;++k){const auto text=line();if(!k)strings[i]=text;}
        else if(type>=4&&type<=7)skip(4);else if(type>7)std::abort();}skip(4);
    }
    const auto n=number();if(n!=ubc::lhiver::code_length||data.size()-p!=n)std::abort();code.assign(data.begin()+static_cast<std::ptrdiff_t>(p),data.end());
  }
};
inline void put(Image& image,std::uint32_t address,std::uint32_t value){if(!image.at(address,4))image.block(address,4);image.put(address,value);}
inline void text(Image& image,std::uint32_t address,const std::string& value){image.block(address,value.size()+1);std::memcpy(image.at(address,value.size()+1),value.c_str(),value.size()+1);}
inline void patch_call(Image& image,std::uint32_t address,std::uint32_t target){*image.at(address,1)=0xe8;image.put(address+1,target-address-5u);}
}
