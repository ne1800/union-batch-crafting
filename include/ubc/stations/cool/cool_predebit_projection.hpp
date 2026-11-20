// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_predebit_admission.hpp"

namespace ubc::cool_predebit::inspection {
// The only mutable memory of the diagnosis are these two local words. The
// read channel has neither a callback to game writing nor an engine call.
class SplitImage {
public:
  SplitImage(const AmountMemory& access,std::uint32_t body,std::uint32_t hot_object)noexcept:
    live_(access.reader),body_(body),hot_(hot_object){}
  ReadOnlyMemory memory()noexcept{return {this,&copy};}
  AmountMemory amount()noexcept{return {memory(),this,&identity,&writable,&replace};}
  void after_split()noexcept{split_=true;}
private:
  ReadOnlyMemory live_;std::uint32_t body_,hot_,operand_=1;bool patched_=false,split_=false;
  static bool copy(void* ctx,std::uint32_t address,void* target,std::size_t n)noexcept{
    auto& self=*static_cast<SplitImage*>(ctx);
    if(!self.live_.copy(address,target,n))return false;
    const auto overlay=[&](std::uint32_t at,std::uint32_t value){
      for(unsigned i=0;i<4;++i){const auto p=std::uint64_t{at}+i;
        if(p>=address&&p-address<n)static_cast<std::uint8_t*>(target)[p-address]=static_cast<std::uint8_t>(value>>(8*i));}
    };
    if(self.patched_)overlay(self.body_+62,self.operand_);
    if(self.split_&&self.hot_&&self.hot_<=UINT32_MAX-0x330)overlay(self.hot_+0x32c,1);
    return true;
  }
  static bool identity(void* ctx,std::uint32_t address,std::size_t n)noexcept{
    return address==static_cast<SplitImage*>(ctx)->body_&&n==cool_recipe_catalog::single.information->byte_length;
  }
  static bool writable(void* ctx,std::uint32_t address,std::size_t n)noexcept{
    return address==static_cast<SplitImage*>(ctx)->body_+62&&n==4;
  }
  static bool replace(void* ctx,std::uint32_t address,const void* expected,const void* next,std::size_t n)noexcept{
    auto& self=*static_cast<SplitImage*>(ctx);std::uint32_t current=0;
    if(!writable(ctx,address,n)||!copy(ctx,address,&current,4)||std::memcmp(expected,&current,4))return false;
    std::memcpy(&self.operand_,next,4);self.patched_=self.operand_!=1;return true;
  }
};

using Phase=reader::Phase;
inline constexpr std::array<Phase,6> phases{{Phase::admission,Phase::before_extra,Phase::before_remove,
  Phase::restore,Phase::after_extra,Phase::before_original}};
struct Reports {
  std::array<check::Report,7> phase{};
  check::Report& at(Phase p)noexcept{return phase[static_cast<unsigned>(p)];}
  void clear()noexcept{for(auto& r:phase){r.count=0;r.overflow=false;}}
};
} // namespace ubc::cool_predebit::inspection
