// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/cool_destroy_abi.hpp"
#include "ubc/cool_destroy_dispatch.hpp"
#include "ubc/cool_set_interact_path.hpp"
#include "ubc/destroy_remove_callsite.hpp"
#include "ubc/alchemy_flask_runtime.hpp"
#include "ubc/sha256.hpp"
#include "ubc/station_abi.hpp"
#include <cstring>

namespace ubc::cool_native_path {
namespace abi=cool_destroy_abi;
namespace call=destroy_remove_callsite;
inline constexpr std::array<std::uint8_t,6> destroy_prefix{{0x64,0xa1,0,0,0,0}};
// Only the own successfully installed registry provides these
// data. A read E9/CALL is never an installation or ownership proof.
struct OwnedPatches {
  bool destroy_installed=false,remove_installed=false;
  std::uint32_t destination=0,trampoline=0;
  call::Plan remove{};
};
enum class Issue {none,range,unreadable,changed,ownership,entry,trampoline,callsite,code,native,dispatch};
struct Verification {
  bool valid=false;
  Issue issue=Issue::none;
  std::uint32_t address=0;
  // For a known ZBug-pack branch, the concrete current list path
  // must be separately positively verified. This codeguard does not.
  bool zbug_unpack_call=false;
  cool_destroy_dispatch::Evidence dispatch{};
  cool_set_interact_path::Verification set_interact{};
};
namespace detail {
inline bool range(std::uint32_t p,std::size_t n) noexcept {
  return p&&n&&n<=UINT64_C(0x100000000)-p;
}
inline bool overlap(std::uint32_t a,std::size_t an,std::uint32_t b,std::size_t bn) noexcept {
  return std::uint64_t{a}<std::uint64_t{b}+bn&&std::uint64_t{b}<std::uint64_t{a}+an;
}
inline bool external(std::uint32_t p,std::size_t n) noexcept {
  return range(p,n)&&!overlap(p,n,station_abi::image_base,station_abi::image_size);
}
inline void jump(std::uint8_t* out,std::uint32_t from,std::uint32_t to) noexcept {
  out[0]=0xe9;const auto d=to-(from+5u);
  for(unsigned i=0;i<4;++i)out[i+1]=static_cast<std::uint8_t>(d>>(8u*i));
}
inline lhiver::Digest digest(const void* p,std::size_t n) noexcept {
  Sha256 h;h.update(p,n);return h.finish();
}
template<std::size_t N> Verification stable(const ReadOnlyMemory& m,std::uint32_t address,
    std::array<std::uint8_t,N>& bytes,std::size_t size=N) noexcept {
  if(size>N||!range(address,size))return {false,Issue::range,address};
  std::array<std::uint8_t,N> second{};
  if(!m.copy(address,bytes.data(),size)||!m.copy(address,second.data(),size))return {false,Issue::unreadable,address};
  if(std::memcmp(bytes.data(),second.data(),size))return {false,Issue::changed,address};
  return {true,Issue::none,0};
}
inline Verification hooks(const ReadOnlyMemory& m,const OwnedPatches& own) noexcept {
  if(own.destroy_installed){
    if(!external(own.destination,1)||!external(own.trampoline,11)||
      overlap(own.destination,1,own.trampoline,11))return {false,Issue::ownership,abi::destroy_entry};
    std::array<std::uint8_t,6> actual{},expected{};expected.fill(0x90);
    jump(expected.data(),abi::destroy_entry,own.destination);
    auto v=stable(m,abi::destroy_entry,actual);if(!v.valid)return v;
    if(actual!=expected)return {false,Issue::entry,abi::destroy_entry};
    std::array<std::uint8_t,11> trampoline{},original{};
    std::memcpy(original.data(),destroy_prefix.data(),destroy_prefix.size());
    jump(original.data()+6,own.trampoline+6,abi::destroy_entry+6);
    v=stable(m,own.trampoline,trampoline);if(!v.valid)return v;
    if(trampoline!=original)return {false,Issue::trampoline,own.trampoline};
  }else if(own.destination||own.trampoline)return {false,Issue::ownership,abi::destroy_entry};
  if(own.remove_installed){
    if(!own.remove.valid||own.remove.call!=call::address||!external(own.remove.destination,1)||
      (own.destroy_installed&&(overlap(own.remove.destination,1,own.trampoline,11)||own.remove.destination==own.destination)))
      return {false,Issue::ownership,call::address};
    std::array<std::uint8_t,25> context{};
    auto v=stable(m,call::address-static_cast<std::uint32_t>(call::prefix_size),context);if(!v.valid)return v;
    if(!call::context_matches(context,own.remove,true))return {false,Issue::callsite,call::address};
  }else if(own.remove.valid||own.remove.call||own.remove.destination)return {false,Issue::ownership,call::address};
  return {true,Issue::none,0};
}
// area originates in the production path exclusively from identity_ranges.
// Testable subfunction: replaces only previously exactly verified own bytes.
inline Verification code(const ReadOnlyMemory& m,const abi::CodeRange& area,const OwnedPatches& own) noexcept {
  if(area.length>2048)return {false,Issue::range,area.address};
  auto v=hooks(m,own);if(!v.valid)return v;
  std::array<std::uint8_t,2048> bytes{};
  v=stable(m,area.address,bytes,area.length);if(!v.valid)return v;
  if(area.address==abi::destroy_entry){
    if(area.length!=460)return {false,Issue::range,area.address};
    if(own.destroy_installed){
      std::array<std::uint8_t,6> expected{};expected.fill(0x90);
      detail::jump(expected.data(),abi::destroy_entry,own.destination);
      if(std::memcmp(bytes.data(),expected.data(),expected.size()))return {false,Issue::entry,area.address};
      std::memcpy(bytes.data(),destroy_prefix.data(),destroy_prefix.size());
    }
    if(own.remove_installed){
      auto* site=bytes.data()+call::address-area.address;
      if(std::memcmp(site,own.remove.bytes.data(),call::patch_size))return {false,Issue::callsite,call::address};
      std::memcpy(site,call::original_context.data()+call::prefix_size,call::patch_size);
    }
  }
  if(digest(bytes.data(),area.length)!=area.sha256)return {false,Issue::code,area.address};
  return hooks(m,own);
}
}
inline bool installed_hooks_current(const ReadOnlyMemory& memory,const OwnedPatches& owned) noexcept {
  return owned.destroy_installed&&owned.remove_installed&&detail::hooks(memory,owned).valid;
}
// No engine call and no synchronization against any foreign threads.
// Native Caller binds file identity, own engine thread, and idle hook
// installation. Before installation own is empty; after it only own registry.
inline Verification verify(const ReadOnlyMemory& m,const OwnedPatches& own={},
    const alchemy_flask_runtime::NativeCodePolicy& policy={}) noexcept {
  bool zbug=false;
  for(unsigned pass=0;pass<2;++pass){
    for(const auto& area:abi::identity_ranges){const auto v=detail::code(m,area,own);if(!v.valid)return v;}
    const auto native=alchemy_flask_runtime::verify_native(m,policy);
    if(!native.reviewed_ranges_match||!native.vtable_slots_match||native.issue!=alchemy_flask_runtime::Issue::none)
      return {false,Issue::native,native.failed_address};
    if(pass&&zbug!=native.zbug_unpack_call)return {false,Issue::changed,call::target};
    zbug=native.zbug_unpack_call;
  }
  return {true,Issue::none,0,zbug};
}
// Tighter, positively read Manipulate9 path. The previous full-body
// guard remains unchanged the rule. Only this reader path replaces its
// four OnMessage blocks; Destroy/Create/SetInteract/GetInstance, own
// hooks, Manipulate table and the native Remove contract are further
// checked. before_writer knows no future message. The actual invocation
// is required separately before OriginalDestroy, never after its return.
inline Verification verify_dispatch(const ReadOnlyMemory& m,const OwnedPatches& own,
    const alchemy_flask_runtime::NativeCodePolicy& policy,cool_destroy_dispatch::Reader& reader,
    const cool_destroy_dispatch::Context& context,std::uint64_t frame,std::uint32_t thread,
    bool boundary,const cool_destroy_dispatch::Invocation* invocation=nullptr,
    const zbug_end_compat::Binding* set_interact_binding=nullptr) noexcept {
  namespace d=cool_destroy_dispatch;
  const auto scope=invocation?d::Scope::before_destroy:d::Scope::before_writer;
  const auto invocation_value=invocation?*invocation:d::Invocation{};
  const auto capture=[&]() noexcept {
    return invocation?reader.capture_before_destroy(m,context,frame,thread,boundary,invocation_value):
      reader.capture_before_writer(m,context,frame,thread,boundary);
  };
  auto evidence=capture();
  const auto rejected=[&](Issue issue) noexcept {
    Verification out{false,issue,evidence.failed_address?evidence.failed_address:0x74b020u};
    out.dispatch=evidence;return out;
  };
  if(!d::matches(evidence,context,frame,scope,invocation_value))return rejected(Issue::dispatch);
  const auto fingerprint=evidence.fingerprint;bool zbug=false;
  cool_set_interact_path::Verification interact{};
  for(unsigned pass=0;pass<2;++pass){
    for(std::size_t i=4;i<abi::identity_ranges.size();++i){
      if(i==6&&set_interact_binding){
        // Only this explicit native caller provides currently held
        // module leases. The default and verify() remain fully strict.
        const auto before=detail::hooks(m,own);if(!before.valid)return before;
        const auto current=cool_set_interact_path::verify(m,*set_interact_binding);
        if(!current.valid){Verification out{false,Issue::code,current.address};out.set_interact=current;return out;}
        if(pass&&(interact.route!=current.route||interact.fingerprint!=current.fingerprint))
          return {false,Issue::changed,cool_set_interact_path::body.address};
        interact=current;
        const auto after=detail::hooks(m,own);if(!after.valid)return after;
        continue;
      }
      const auto v=detail::code(m,abi::identity_ranges[i],own);if(!v.valid)return v;
    }
    const auto native=alchemy_flask_runtime::verify_native(m,policy);
    if(!native.reviewed_ranges_match||!native.vtable_slots_match||native.issue!=alchemy_flask_runtime::Issue::none)
      return {false,Issue::native,native.failed_address};
    if(pass&&zbug!=native.zbug_unpack_call)return {false,Issue::changed,call::target};
    zbug=native.zbug_unpack_call;evidence=capture();
    if(!d::matches(evidence,context,frame,scope,invocation_value))return rejected(Issue::dispatch);
    if(evidence.fingerprint!=fingerprint)return rejected(Issue::changed);
  }
  Verification out{true,Issue::none,0,zbug};out.dispatch=evidence;out.set_interact=interact;return out;
}
} // namespace ubc::cool_native_path
