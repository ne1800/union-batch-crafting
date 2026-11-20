// SPDX-License-Identifier: MIT
#pragma once

#include "ubc/vob_locator_compat.hpp"

namespace ubc::ammo_compat_memory {
enum class Issue {none,raw_read,slot_shape,first_binding,forwarding,last_binding,binding_changed,slot_changed};
struct Diagnostic {
  Issue issue=Issue::none;
  vob_locator_compat::Issue compatibility=vob_locator_compat::Issue::none;
  std::uint32_t stage=0,address=0;
};
// Only the check copy of the known OnMessage vslot normalizes. No write
// and no cache of a foreign hook release. All payload/scope/ lifetime
// guards remain required in the unchanged ammo_runtime contract.
class Reader {
public:
  using Modules=vob_locator_compat::Modules;
  using Bind=bool (*)(void*,Modules&) noexcept;
  Reader(ReadOnlyMemory raw,void* user,Bind bind) noexcept:raw_(raw),user_(user),bind_(bind){}
  ReadOnlyMemory memory() noexcept {return {this,copy};}
  // Diagnosis only, never a release. The last error remains
  // readable, even if subsequent independent reads match again.
  Diagnostic diagnostic() const noexcept {return diagnostic_;}
private:
  ReadOnlyMemory raw_;void* user_;Bind bind_;
  Diagnostic diagnostic_{};
  bool fail(Issue issue,std::uint32_t address,
      vob_locator_compat::Issue compatibility=vob_locator_compat::Issue::none,std::uint32_t stage=0) noexcept {
    diagnostic_={issue,compatibility,stage,address};return false;
  }
  static bool copy(void* context,std::uint32_t address,void* output,std::size_t size) noexcept {
    namespace c=vob_locator_compat;
    auto& self=*static_cast<Reader*>(context);
    if(!c::detail::overlap(address,size,c::manager_slot,4))
      return self.raw_.copy(address,output,size)||self.fail(Issue::raw_read,address);
    // Existing readers query exactly this uint32. Partial
    // reads or larger unknown table areas receive no exception.
    if(address!=c::manager_slot||size!=4)return self.fail(Issue::slot_shape,address);
    std::uint32_t before=0,after=0;
    if(!self.raw_.copy(address,&before,4))return self.fail(Issue::raw_read,address);
    if(before!=c::original_on_message){
      Modules first{},last{};
      if(!self.bind_||!self.bind_(self.user_,first))return self.fail(Issue::first_binding,address);
      const auto proof=c::verify_print_forwarding(self.raw_,first);
      if(!proof.valid||proof.version!=c::profile_version||proof.on_message!=before)
        return self.fail(Issue::forwarding,address,proof.issue,proof.stage);
      if(!self.bind_(self.user_,last))return self.fail(Issue::last_binding,address);
      if(first.bases!=last.bases)return self.fail(Issue::binding_changed,address);
    }
    if(!self.raw_.copy(address,&after,4))return self.fail(Issue::raw_read,address);
    if(after!=before)return self.fail(Issue::slot_changed,address);
    const auto original=c::original_on_message;std::memcpy(output,&original,4);return true;
  }
};
} // namespace ubc::ammo_compat_memory
