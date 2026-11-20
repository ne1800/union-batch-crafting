// SPDX-License-Identifier: MIT
// Reuse our read-only fixture. Original file contents stay in RAM.
#define main dependencies_fixture_main
#include "alchemy_extra_dependencies_tests.cpp"
#undef main
#include "ubc/alchemy_extra_dependency_reader.hpp"

namespace dependency_reader_test {
namespace r=ubc::alchemy_extra_dependency_reader;
r::Reader reader{7};
r::Input nested_input{};
d::Issue nested=d::Issue::none;
void reenter(Memory& memory) noexcept {
  nested=reader.capture(memory.view(),ubc::alchemy_recipe_catalog::recipes[0],nested_input,r::Phase::before_original).guard.issue;
}
r::Input input(const Fixture& f) {
  r::Input v;v.boundary.context=f.context;
  v.boundary.scope={1,1,ubc::alchemy_recipe_catalog::recipes[0].callback.function->symbol,1,0,0,true};
  v.boundary.serial=1;v.boundary.current_thread=7;
  v.boundary.synchronous_writer_verified=v.boundary.physical_lab_hand_contract_verified=true;
  // This old bit alone must never authorize release.
  v.boundary.native_dependencies_and_configured_targets_verified=true;
  v.modules=f.modules;v.targets=d::expected_targets;v.restore_item=0x7a2f20;v.quantity=2;return v;
}
void cases() {
  const auto& recipe=ubc::alchemy_recipe_catalog::recipes[0];
  auto rejected_without_read=[&](auto mutate,r::Phase phase=r::Phase::before_original) {
    Fixture f;auto in=input(f);mutate(in);const auto before=f.memory.reads;
    const auto result=reader.capture(f.memory.view(),recipe,in,phase);
    CHECK(!result.valid && !result.runtime.observation.native_contract);
    CHECK(f.memory.reads==before);
  };
  rejected_without_read([](r::Input& i){i.boundary.current_thread++;});
  rejected_without_read([](r::Input& i){i.boundary.scope.writer_token=0;});
  rejected_without_read([](r::Input& i){i.boundary.scope.vm_depth=1;});
  rejected_without_read([](r::Input& i){i.quantity=0;});
  rejected_without_read([](r::Input& i){i.quantity=UINT32_MAX;});
  rejected_without_read([](r::Input& i){i.restore_item++;});
  rejected_without_read([](r::Input& i){i.targets.remove++;});
  rejected_without_read([](r::Input& i){i.modules.zbug.mapping_verified=false;});
  rejected_without_read([](r::Input&){},static_cast<r::Phase>(99));
  rejected_without_read([](r::Input&){},r::Phase::after_debit);
  rejected_without_read([](r::Input& i){i.boundary.scope.debit_depth=1;});
  Fixture f;auto in=input(f);
  const auto blocked=reader.capture(f.memory.view(),recipe,in,r::Phase::before_original);
  CHECK(!blocked.valid && !blocked.guard.valid); // Synthetic zero-filled bodies cannot establish engine identity.
  in.boundary.native_dependencies_and_configured_targets_verified=false;
  CHECK(!reader.capture(f.memory.view(),recipe,in,r::Phase::before_original).valid);
  nested_input=input(f);f.memory.callback=reenter;
  CHECK(!reader.capture(f.memory.view(),recipe,nested_input,r::Phase::before_original).valid);
  CHECK(nested==d::Issue::reentry);
  ubc::alchemy_flask_debit::Call call;call.quantity=1;in.boundary.scope.debit_depth=1;
  CHECK(!reader.restore_targets(f.memory.view(),call,in).valid);
  call.quantity=UINT32_MAX;CHECK(!reader.restore_targets(f.memory.view(),call,in).valid);
  const auto policy=r::code_policy(f.modules);
  CHECK(policy.mode==rt::NativeCodeMode::known_zbug_remove_site && policy.zbug.base==f.modules.zbug.base);
}
}
int main(int argc,char** argv) {
  const auto prior=dependencies_fixture_main(argc,argv);if(prior)return prior;
  dependency_reader_test::cases();
  std::printf("dependency reader: %u checks, %u failures; complete positive module dispatch is separate live evidence\n",checks,failures);
  return failures?1:0;
}
