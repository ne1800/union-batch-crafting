// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/alchemy/alchemy_extra_dependencies.hpp"

namespace ubc::alchemy_extra_dependency_reader {
namespace dep=alchemy_extra_dependencies;
namespace rt=alchemy_flask_runtime;
namespace debit=alchemy_flask_debit;
enum class Phase { before_original,after_original,before_debit,after_debit };
struct Input {
  rt::Boundary boundary{};
  dep::Modules modules{};
  dep::Targets targets{};
  std::uint32_t restore_item=0,quantity=0;
};
struct Environment {
  debit::Context context{};
  dep::Modules modules{};
  dep::Targets targets{};
  std::uint32_t restore_item=0;
};
struct Guard {
  bool valid=false;
  dep::KnownCode code{};
  dep::PendingFacts dispatch{};
  dep::ObjectEvidence objects{};
  dep::ItemBinding item{};
  dep::Issue issue=dep::Issue::none;
};
struct Capture {
  rt::Capture runtime{};
  Guard guard{};
  dep::ListEvidence list{};
  bool valid=false;
};
struct RestoreCapture {
  rt::RestoreCapture runtime{};
  Guard guard{};
  bool valid=false;
};
inline rt::NativeCodePolicy code_policy(const dep::Modules& modules) noexcept {
  return {rt::NativeCodeMode::known_zbug_remove_site,modules.zbug};
}
// This checks actual memory, not a caller-supplied release bit.
// The loader/file assignment of the modules remains its concrete
// lease-bound native task; their PE/code/export targets are read here.
inline Guard verify_environment(const ReadOnlyMemory& memory,const Environment& in) noexcept {
  Guard out;
  if(!debit::valid(in.context) || in.restore_item!=0x7a2f20) {
    out.issue=dep::Issue::context;return out;
  }
  out.code=dep::verify_known_code(memory,in.modules,in.targets);
  if(!out.code.known_ranges_match) {out.issue=out.code.issue;return out;}
  out.dispatch=dep::read_pending_facts(memory,in.modules);
  if(!out.dispatch.read_stable || !out.dispatch.allocator_boundary_verified ||
     !out.dispatch.wrapper_dispatch_verified || !out.dispatch.engine_version_verified || !out.dispatch.item_known) {
    out.issue=out.dispatch.issue==dep::Issue::none?dep::Issue::targets:out.dispatch.issue;return out;
  }
  out.objects=dep::read_objects(memory,in.context);
  if(!out.objects.stable || !out.objects.flask_class_is_vob_item) {out.issue=out.objects.issue;return out;}
  out.item=dep::read_item_binding(memory,in.context.parser);
  if(!dep::same(out.item,out.dispatch.item)) {out.issue=dep::Issue::item_binding;return out;}
  if(!rt::detail::context(memory,in.context) || !rt::detail::lab(memory,in.context)) {
    out.issue=dep::Issue::context;return out;
  }
  out.valid=true;return out;
}
inline Guard verify_guard(const ReadOnlyMemory& memory,const Input& in,std::uint32_t owner) noexcept {
  if(!rt::detail::boundary(in.boundary,owner) || !in.quantity || in.quantity>INT32_MAX) {
    Guard out;out.issue=dep::Issue::context;return out;
  }
  return verify_environment(memory,{in.boundary.context,in.modules,in.targets,in.restore_item});
}
// Persistent, tightly bound capture. The bare runtime reader
// remains available for original code; only this adapter binds known
// mod fields to the full positive list/dispatcher/ITEM contract.
class Reader {
public:
  explicit Reader(std::uint32_t owner) noexcept:owner_(owner),runtime_(owner),list_(owner){}
  Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
  Capture capture(const ReadOnlyMemory& memory,const alchemy_recipe_catalog::Descriptor& recipe,
      const Input& in,Phase phase) noexcept {
    Capture out;
    if(reading_) {out.guard.issue=dep::Issue::reentry;return out;}
    if((phase!=Phase::before_original && phase!=Phase::after_original && phase!=Phase::before_debit && phase!=Phase::after_debit) ||
       in.boundary.scope.debit_depth!=(phase==Phase::after_debit?1u:0u)) {out.guard.issue=dep::Issue::context;return out;}
    reading_=true;struct Lease {bool& b;~Lease(){b=false;}} lease{reading_};
    out.guard=verify_guard(memory,in,owner_);if(!out.guard.valid)return out;
    auto boundary=in.boundary;
    // The old adapter evidence is set only AFTER the concrete
    // full guard. A bit already set by the caller is ignored.
    boundary.native_dependencies_and_configured_targets_verified=true;
    out.runtime=runtime_.capture(memory,recipe,boundary,code_policy(in.modules),
      phase==Phase::before_original?rt::ParserStackMode::empty:rt::ParserStackMode::original_returns);
    if(!out.runtime.valid)return out;
    if(in.quantity>1 && phase!=Phase::after_debit) {
      const auto& flask=out.runtime.observation.flask;
      if(!flask.known || flask.packed || flask.listed_stacks!=1 || !flask.object) {out.guard.issue=dep::Issue::flask;return out;}
      out.list=list_.capture(memory,in.boundary.current_thread,{in.boundary.context.hero,flask.object,in.quantity-1,in.boundary.context.world});
      if(!out.list.stable_complete || !out.list.unpack_unreachable ||
         (out.list.path==dep::RemovalPath::whole && !out.list.whole_short_path))return out;
      if(out.list.amount!=flask.amount) {out.guard.issue=dep::Issue::changed;return out;}
    }
    const auto after=verify_guard(memory,in,owner_);
    if(!after.valid || !dep::same(out.guard.item,after.item) ||
       out.guard.code.zbug_unpack_call!=after.code.zbug_unpack_call || out.guard.code.zpe_init_call!=after.code.zpe_init_call) {
      out.guard=after;out.guard.valid=false;out.guard.issue=dep::Issue::changed;return out;
    }
    out.guard=after;out.valid=true;return out;
  }
  RestoreCapture restore_targets(const ReadOnlyMemory& memory,const debit::Call& call,const Input& in) noexcept {
    RestoreCapture out;
    if(reading_) {out.guard.issue=dep::Issue::reentry;return out;}
    reading_=true;struct Lease {bool& b;~Lease(){b=false;}} lease{reading_};
    if(in.quantity!=call.quantity+1u || in.boundary.scope.debit_depth!=1) {out.guard.issue=dep::Issue::context;return out;}
    out.guard=verify_guard(memory,in,owner_);if(!out.guard.valid)return out;
    auto boundary=in.boundary;boundary.native_dependencies_and_configured_targets_verified=true;
    out.runtime=runtime_.restore_targets(memory,call,boundary,code_policy(in.modules));
    if(!out.runtime.valid)return out;
    const auto after=verify_guard(memory,in,owner_);
    if(!after.valid || !dep::same(out.guard.item,after.item)) {out.guard=after;out.guard.valid=false;out.guard.issue=dep::Issue::changed;return out;}
    out.guard=after;out.valid=true;return out;
  }
private:
  std::uint32_t owner_=0;bool reading_=false;
  rt::Reader runtime_;
  dep::ListReader list_;
};
} // namespace ubc::alchemy_extra_dependency_reader
