// SPDX-License-Identifier: MIT
#pragma once
#include "alchemy_flask_debit_bridge.hpp"
#include "ubc/alchemy_extra_dependency_reader.hpp"
#include "ubc/tobacco_runtime.hpp"
#include "ubc/bow_item_native.hpp"

namespace ubc::tobacco_writer {
namespace dep=alchemy_extra_dependencies;
namespace deps=alchemy_extra_dependency_reader;
namespace rt=alchemy_flask_runtime;
namespace debit=alchemy_flask_debit;
namespace bridge=alchemy_flask_debit_bridge;

template<class Check>
inline bool debit_path(const ReadOnlyMemory& memory,dep::ListReader& reader,
    const debit::Context& context,const dep::Modules& modules,const tobacco::Descriptor& recipe,
    const tobacco::Capture& sample,std::size_t i,std::uint32_t quantity,Check& check) noexcept {
  if(quantity==1)return true;
  const auto guard=deps::verify_environment(memory,{context,modules,dep::expected_targets,bridge::restore_item_entry});
  const auto native=rt::verify_native(memory,deps::code_policy(modules));
  if(!check.equal("debit.environment",0,static_cast<int>(guard.issue))||
     !check.equal("debit.native",true,guard.valid&&native.reviewed_ranges_match&&native.vtable_slots_match)||
     !check.equal("debit.flask_constructor",true,rt::verify_flask_constructor(memory).constructor_and_bindings_match))return false;
  const auto symbol=static_cast<std::uint32_t>(recipe.targets[i].symbol);
  const auto count=i==0?quantity:quantity-1;
  const auto list=reader.capture(memory,context.thread,{context.hero,sample.objects[i],count,context.world,symbol});
  if(!check.equal("debit.list",0,static_cast<int>(list.issue))||
     !check.equal("debit.list_path",true,list.stable_complete&&list.unpack_unreachable&&
       (list.path==dep::RemovalPath::split||(list.path==dep::RemovalPath::whole&&list.whole_short_path)))||
     !check.equal("debit.stack",sample.stock[i],list.amount)){
    check.symbol=static_cast<int>(symbol);return false;
  }
  return true;
}
} // namespace ubc::tobacco_writer
