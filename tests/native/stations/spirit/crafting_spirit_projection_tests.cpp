// SPDX-License-Identifier: MIT
#include "ubc/stations/spirit/crafting_spirit_projection.hpp"
#define UBC_SPIRIT_EXECUTION_TEST_EMBEDDED
#include "spirit_execution_tests.cpp"
#define UBC_CRAFTING_FIXTURE_ONLY
#include "../../catalogs/crafting_catalog_tests.cpp"
namespace spirit_crafting_test {
namespace sc = ubc::spirit_recipe_catalog;
namespace detail = ubc::spirit_crafting_detail;
using Point = ubc::AlchemyExtraReadPoint;
using Issue = ubc::SpiritCraftingIssue;
[[maybe_unused]] auto* const old_cases = &spirit_execution_test::local;
struct Fixture : spirit_execution_test::SpiritFixture {
  ubc::SpiritCraftingInput input{};
  unsigned lease_calls = 0;
  std::uint32_t lease_value = execution_test::actual_info;
  bool expire = false, change_branch = false, replace_info = false;
  std::uint32_t changed_branch = 2;
  explicit Fixture(const LocalFacts* facts = nullptr,
                   const std::array<std::string, 4>* texts = nullptr, std::int32_t branch = 1)
      : SpiritFixture(facts, texts, branch) {
    image.block(ubc::station_abi::information_manager, 0x58);
    image.put(ubc::station_abi::information_manager + 0x20, hero);
    image.put(ubc::station_abi::information_manager + 0x24, hero);
    input.common.context = boundary.context;
    input.common.key = {1,
                        7,
                        world,
                        hero,
                        mob,
                        ubc::station_abi::information_manager,
                        ubc::crafting_catalog::Station::alchemy};
    input.common.serial = 20;
    input.common.current_thread = 7;
    input.common.boundary = {Point::post_render, true, true, 0, 0, 0, 0};
    input.common.targets = ubc::alchemy_extra_dependencies::expected_targets;
    input.common.restore_item = 0x7a2f20;
    input.info_context = this;
    input.read_actual_info = lease;
  }
  static std::uint32_t lease(void* p, const ubc::CraftingAlchemyExtraInput& in) noexcept {
    auto& self = *static_cast<Fixture*>(p);
    ++self.lease_calls;
    CHECK(in.context.hero == hero && in.key.station == mob && in.serial == 20);
    if (self.lease_calls == 2 && self.change_branch)
      self.image.put(symbol(11967) + 0x18, self.changed_branch);
    if (self.lease_calls == 2 && self.replace_info)
      return self.lease_value + 4;
    return self.expire && self.lease_calls == 2 ? 0 : self.lease_value;
  }
  void entry() { input.common.boundary = {Point::before_original, true, true, 1, 0, 1, 0}; }
};
ubc::SpiritCraftingReader reader{7};
}
namespace projection_test {
namespace sc = ubc::spirit_recipe_catalog;
ubc::SpiritCraftingCapture capture(std::int32_t branch = 1) {
  spirit_crafting_test::Fixture f(nullptr, nullptr, branch);
  return spirit_crafting_test::reader.capture(f.image.memory(), f.input);
}
void basic() {
  for (const auto branch : {0, 1, 2, 3, INT32_MIN, INT32_MAX}) {
    const auto c = capture(branch);
    const auto p = ubc::project_capture(c);
    const auto& s = p.recipes[ubc::crafting_catalog::spirit_begin];
    CHECK(ubc::valid_crafting_snapshot(p) && s.known && s.branch_known &&
          s.branch_value == branch && s.branch_output == (branch == 1 ? 15741 : 15739));
    CHECK(s.material_known && s.material_verified && s.hand_known && s.actor_matches &&
          s.context_matches && !s.contract && !s.maximum);
    CHECK(ubc::same_crafting_hands(p.hands, c.recipe.hands));
    auto next = c;
    next.recipe.state.branch_value = branch == 2 ? 3 : 2;
    CHECK(!ubc::valid_crafting_snapshot(ubc::project_capture(next)));
  }
  auto c = capture();
  for (unsigned change = 0; change < 27; ++change) {
    auto b = c;
    switch (change) {
    case 0:
      b.serial = 0;
      break;
    case 1:
      b.context_after.world++;
      break;
    case 2:
      b.recipe.key.manager = 0;
      break;
    case 3:
      b.context_before.epoch++;
      break;
    case 4:
      b.recipe.state.material.quantity = 1;
      break;
    case 5:
      b.recipe.state.material.count = 6;
      break;
    case 6:
      b.recipe.state.material.craftable = true;
      break;
    case 7:
      b.recipe.state.material.additional_flasks = 1;
      break;
    case 8:
      b.recipe.state.rows[0].symbol++;
      break;
    case 9:
      b.recipe.state.rows[4].reserved = 0;
      break;
    case 10:
      b.recipe.state.rows[4].available--;
      break;
    case 11:
      b.recipe.state.rows[2].required = 1;
      break;
    case 12:
      b.recipe.state.rows[3].produced = 1;
      break;
    case 13:
      b.recipe.state.rows[5].unit_output = 1;
      break;
    case 14:
      b.recipe.state.material.maximum++;
      break;
    case 15:
      b.maximum_executable = 51;
      break;
    case 16:
      b.recipe.state.branch_known = false;
      break;
    case 17:
      b.recipe.state.output = 15739;
      break;
    case 18:
      b.recipe.diagnostic.branch_after.value = 3;
      break;
    case 19:
      b.recipe.diagnostic.knowledge_after.readings[0].value = 0;
      break;
    case 20:
      b.recipe.state.known = false;
      break;
    case 21:
      b.recipe.hands.interact_item++;
      break;
    case 22:
      b.recipe.hands.right_tmp_level = -1;
      break;
    case 23:
      b.actor_verified = false;
      break;
    case 24:
      b.actor_purpose = ubc::AlchemyExtraActorPurpose::unknown;
      break;
    case 25:
      b.recipe.state.rows[1].remaining++;
      break;
    case 26:
      b.recipe.state.rows[1].missing_for_one++;
      break;
    }
    if (ubc::valid_crafting_snapshot(ubc::project_capture(b)))
      std::fprintf(stderr, "mutation %u accepted\n", change);
    CHECK(!ubc::valid_crafting_snapshot(ubc::project_capture(b)));
  }
  spirit_crafting_test::Fixture f;
  f.image.put(symbol(1679) + 0x1c, hero + 4);
  auto preview = spirit_crafting_test::reader.capture(f.image.memory(), f.input);
  CHECK(ubc::project_capture(preview).recipes[ubc::crafting_catalog::spirit_begin].actor_matches);
  preview.actor_purpose = ubc::AlchemyExtraActorPurpose::original_entry;
  CHECK(!ubc::valid_crafting_snapshot(ubc::project_capture(preview)));
  f.image.put(symbol(11198) + 0x18, 0);
  const auto unlearned = spirit_crafting_test::reader.capture(f.image.memory(), f.input);
  const auto u = ubc::project_capture(unlearned);
  CHECK(ubc::valid_crafting_snapshot(u) && !u.recipes[ubc::crafting_catalog::spirit_begin].known &&
        u.recipes[ubc::crafting_catalog::spirit_begin].material_known);
  f.image.denied = symbol(11967);
  const auto unknown = spirit_crafting_test::reader.capture(f.image.memory(), f.input);
  const auto p = ubc::project_capture(unknown);
  CHECK(ubc::valid_crafting_snapshot(p) &&
        !p.recipes[ubc::crafting_catalog::spirit_begin].branch_known &&
        !p.recipes[ubc::crafting_catalog::spirit_begin].maximum);
}
}
namespace projection_test {
// These positive native fields are explicit value fixtures for the projection. They
// do not establish that plugin modules are loaded or that in-game crafting occurred.
void material_contract(ubc::SpiritCraftingCapture& c) {
  auto& d = c.diagnostic;
  auto& rd = c.recipe.diagnostic;
  c.recipe.state.contract = true;
  rd.profile.valid = rd.profile.local.matches = true;
  rd.profile.local.issue = ubc::runtime_script::Issue::none;
  c.native_debit_verified = true;
  d.dependencies_before.valid = d.dependencies_after.valid = true;
  d.dependencies_before.item = {true,
                                ubc::lhiver::game_parser,
                                symbol(1684),
                                0,
                                table,
                                code,
                                ubc::lhiver::code_length,
                                ubc::lhiver::symbol_count};
  d.dependencies_after.item = d.dependencies_before.item;
  d.native_before.reviewed_ranges_match = d.native_before.vtable_slots_match = true;
  d.native_after = d.native_before;
  d.flask_before.constructor_and_bindings_match = d.flask_before.strings_match = true;
  d.flask_after = d.flask_before;
  d.split.stable_complete = d.split.unpack_unreachable = true;
  d.split.path = ubc::alchemy_extra_dependencies::RemovalPath::split;
  d.split.amount = d.flask.amount;
  c.maximum_executable = ubc::spirit_crafting_detail::material_maximum(c);
}
void effect_entry(ubc::SpiritCraftingCapture& c) {
  c.actual_info_verified = true;
  c.actual_info = execution_test::actual_info;
  auto& e = c.diagnostic.entry;
  const auto& d = c.recipe.diagnostic;
  e.valid = true;
  e.contract = d.profile;
  e.knowledge = d.knowledge_after;
  e.branch = d.branch_after;
  e.context = d.context_after;
  e.code_address = e.branch.layout.code + sc::recipes[0].callback->byte_offset;
}
void material_and_entry() {
  spirit_crafting_test::Fixture f;
  auto preview = spirit_crafting_test::reader.capture(f.image.memory(), f.input);
  material_contract(preview);
  CHECK(preview.maximum_executable == 50);
  const auto p = ubc::project_capture(preview);
  CHECK(ubc::valid_crafting_snapshot(p) &&
        p.recipes[ubc::crafting_catalog::spirit_begin].maximum == 50);
  CHECK(ubc::spirit_crafting_quantity(preview, 2).selectable &&
        !ubc::spirit_crafting_quantity(preview, 2).effect_ready);
  f.entry();
  f.input.read_actual_info = nullptr;
  auto entry = spirit_crafting_test::reader.capture(f.image.memory(), f.input);
  material_contract(entry);
  CHECK(entry.actor_verified && !entry.actual_info_verified && !entry.actual_info &&
        entry.maximum_executable == 50);
  const auto no_lease = ubc::project_capture(entry);
  CHECK(ubc::valid_crafting_snapshot(no_lease) &&
        no_lease.recipes[ubc::crafting_catalog::spirit_begin].actor_matches &&
        no_lease.recipes[ubc::crafting_catalog::spirit_begin].maximum == 50);
  CHECK(ubc::spirit_crafting_quantity(entry, 2).selectable &&
        !ubc::spirit_crafting_quantity(entry, 2).effect_ready);
  effect_entry(entry);
  const auto effect = ubc::project_capture(entry);
  CHECK(ubc::valid_crafting_snapshot(effect) &&
        effect.recipes[ubc::crafting_catalog::spirit_begin].maximum == 50);
  CHECK(ubc::spirit_crafting_quantity(entry, 2).effect_ready);
  for (unsigned change = 0; change < 10; ++change) {
    auto b = entry;
    switch (change) {
    case 0:
      b.actual_info = 0;
      break;
    case 1:
      b.actual_info_verified = false;
      break;
    case 2:
      b.diagnostic.entry.valid = false;
      break;
    case 3:
      b.diagnostic.entry.branch.value = 3;
      break;
    case 4:
      b.diagnostic.entry.code_address++;
      break;
    case 5:
      b.diagnostic.entry.context.special = 0;
      break;
    case 6:
      b.diagnostic.entry.knowledge.known = false;
      break;
    case 7:
      b.diagnostic.entry.contract.valid = false;
      break;
    case 8:
      b.diagnostic.entry.contract.local.matches = false;
      break;
    case 9:
      b.diagnostic.entry.branch.readings[0].value++;
      break;
    }
    CHECK(!ubc::valid_crafting_snapshot(ubc::project_capture(b)));
  }
  for (unsigned change = 0; change < 9; ++change) {
    auto b = preview;
    switch (change) {
    case 0:
      b.boundary_verified = false;
      break;
    case 1:
      b.context_unchanged = false;
      break;
    case 2:
      b.actor_verified = false;
      break;
    case 3:
      b.native_debit_verified = false;
      break;
    case 4:
      b.recipe.state.material.inventory_known = false;
      break;
    case 5:
      b.recipe.state.known = false;
      break;
    case 6:
      b.recipe.state.contract = false;
      break;
    case 7:
      b.recipe.state.context_matches = false;
      break;
    case 8:
      b.recipe.state.material.branch_known = false;
      break;
    }
    CHECK(!ubc::spirit_crafting_detail::material_maximum(b));
  }
  // Before the effect, SELF==HERO is still required even without a lease.
  f.image.put(symbol(1679) + 0x1c, hero + 4);
  auto foreign = spirit_crafting_test::reader.capture(f.image.memory(), f.input);
  material_contract(foreign);
  CHECK(!foreign.actor_verified && !foreign.maximum_executable &&
        !ubc::project_capture(foreign).recipes[ubc::crafting_catalog::spirit_begin].actor_matches);
}
ubc::RecipeCatalogueReader base_reader;
ubc::CraftingAlchemyExtraReader extra_reader{7};
struct MergeCapture {
  ubc::RecipeCatalogueCapture base{};
  ubc::CraftingBaseAlchemyEvidence evidence{};
  ubc::AlchemyExtraCatalogueCapture extra{};
  ubc::SpiritCraftingCapture spirit{};
  ubc::CraftingSnapshot project() const {
    return ubc::project_capture(base, evidence, extra, spirit);
  }
};
MergeCapture merged(bool entry = false, bool foreign_self = false, std::int32_t branch = 1,
                    bool learned = true, bool branch_readable = true) {
  spirit_crafting_test::Fixture f(nullptr, nullptr, branch);
  if (entry) {
    f.entry();
    f.input.read_actual_info = nullptr;
  }
  if (foreign_self)
    f.image.put(symbol(1679) + 0x1c, hero + 4);
  if (!learned)
    f.image.put(symbol(11198) + 0x18, 0);
  if (!branch_readable)
    f.image.denied = symbol(11967);
  const auto& in = f.input.common;
  MergeCapture c;
  auto& e = c.evidence;
  e.serial = in.serial;
  e.boundary_verified = in.boundary.verified;
  const auto before = ubc::crafting_alchemy_extra_detail::context(f.image.memory(), in);
  e.context_before = in.context;
  e.manager_before = in.key.manager;
  e.production_before = before.production;
  e.actor_before = ubc::crafting_alchemy_extra_detail::actor(f.image.memory(), in.context);
  const ubc::StationKey key{in.key.generation,
                            in.key.thread,
                            in.key.world,
                            in.key.hero,
                            in.key.station,
                            in.key.manager,
                            ubc::recipe_catalog::Station::alchemy};
  std::array<bool, ubc::recipe_catalog::recipes.size()> proof{};
  proof.fill(true);
  c.base = base_reader.capture(f.image.memory(), in.context.parser, key, true, proof);
  const auto after = ubc::crafting_alchemy_extra_detail::context(f.image.memory(), in);
  e.context_after = in.context;
  e.manager_after = in.key.manager;
  e.production_after = after.production;
  e.actor_after = ubc::crafting_alchemy_extra_detail::actor(f.image.memory(), in.context);
  e.context_unchanged = ubc::crafting_alchemy_extra_detail::same(before, after);
  e.actor_unchanged =
      ubc::station_recipe_runtime::same_actor_evidence(e.actor_before, e.actor_after);
  c.extra = extra_reader.capture(f.image.memory(), in).capture;
  c.spirit = spirit_crafting_test::reader.capture(f.image.memory(), f.input);
  return c;
}
void merge() {
  for (bool entry : {false, true}) {
    const auto c = merged(entry);
    const auto value = c.project();
    if (!ubc::valid_crafting_snapshot(value))
      std::fprintf(stderr, "merge initial issue %d, base %d extra %d spirit %d\n", int(value.issue),
                   int(ubc::project_capture(c.base).issue),
                   int(ubc::project_capture(c.extra).issue),
                   int(ubc::project_capture(c.spirit).issue));
    CHECK(ubc::valid_crafting_snapshot(value));
    CHECK(value.recipes[ubc::crafting_catalog::spirit_begin].evaluated &&
          value.recipes[ubc::crafting_catalog::spirit_begin].known);
    CHECK(ubc::same_crafting_hands(value.hands, c.spirit.recipe.hands));
  }
  for (bool entry : {false, true}) {
    auto c = merged(entry);
    material_contract(c.spirit);
    const auto p = c.project();
    CHECK(ubc::valid_crafting_snapshot(p) &&
          p.recipes[ubc::crafting_catalog::spirit_begin].maximum == 50);
    CHECK(!ubc::spirit_crafting_quantity(c.spirit, 2).effect_ready);
    if (entry) {
      effect_entry(c.spirit);
      CHECK(ubc::valid_crafting_snapshot(c.project()) &&
            ubc::spirit_crafting_quantity(c.spirit, 2).effect_ready);
    }
    auto foreign = merged(entry, true);
    const auto q = foreign.project();
    CHECK(ubc::valid_crafting_snapshot(q) &&
          q.recipes[ubc::crafting_catalog::spirit_begin].actor_matches == !entry);
  }
  for (const auto branch : {0, 1, 2, 3, INT32_MIN, INT32_MAX}) {
    const auto c = merged(false, false, branch);
    const auto p = c.project();
    CHECK(ubc::valid_crafting_snapshot(p) &&
          p.recipes[ubc::crafting_catalog::spirit_begin].branch_value == branch);
  }
  {
    const auto c = merged(false, false, 1, false);
    const auto p = c.project();
    CHECK(ubc::valid_crafting_snapshot(p) && !p.recipes[ubc::crafting_catalog::spirit_begin].known);
  }
  {
    const auto c = merged(false, false, 1, true, false);
    const auto p = c.project();
    CHECK(ubc::valid_crafting_snapshot(p) &&
          !p.recipes[ubc::crafting_catalog::spirit_begin].branch_known);
  }
  const auto good = merged();
  for (unsigned change = 0; change < 36; ++change) {
    auto c = good;
    switch (change) {
    case 0:
      c.evidence.serial++;
      break;
    case 1:
      c.extra.serial++;
      break;
    case 2:
      c.spirit.serial++;
      break;
    case 3:
      c.evidence.context_after.world++;
      break;
    case 4:
      c.extra.context_before.epoch++;
      break;
    case 5:
      c.spirit.context_after.mob++;
      break;
    case 6:
      c.evidence.manager_before++;
      break;
    case 7:
      c.evidence.manager_after++;
      break;
    case 8:
      c.evidence.production_before = 3;
      break;
    case 9:
      c.evidence.production_after = 3;
      break;
    case 10:
      c.evidence.actor_before.self_symbol.flags++;
      break;
    case 11:
      c.evidence.actor_after.hero_symbol.elements++;
      break;
    case 12:
      c.extra.actor_after.layout.code++;
      break;
    case 13:
      c.spirit.recipe.diagnostic.actor_before.hero_symbol.offset++;
      break;
    case 14:
      c.extra.actor_purpose = ubc::AlchemyExtraActorPurpose::original_entry;
      break;
    case 15:
      c.extra.physical_hero_preview_verified = false;
      break;
    case 16:
      c.evidence.boundary_verified = false;
      break;
    case 17:
      c.evidence.actor_unchanged = false;
      break;
    case 18:
      c.evidence.context_unchanged = false;
      break;
    case 19:
      c.base.hands.right.object++;
      break;
    case 20:
      c.extra.hands_after.right_tmp_level++;
      break;
    case 21:
      c.spirit.recipe.hands.left_tmp_level++;
      break;
    case 22:
      c.spirit.recipe.hands.interact_item++;
      break;
    case 23:
      c.base.snapshot.key.generation++;
      break;
    case 24:
      c.extra.key.manager++;
      break;
    case 25:
      c.evidence.actor_after.matches = false;
      break;
    case 26:
      c.evidence.actor_before.read_valid = false;
      break;
    case 27:
      c.evidence.actor_after.requires_self = false;
      break;
    case 28:
      c.evidence.context_before.game++;
      break;
    case 29:
      c.evidence.context_after.parser++;
      break;
    case 30:
      c.extra.production_before++;
      break;
    case 31:
      c.extra.production_after++;
      break;
    case 32:
      c.extra.actor_before.hero_symbol.type++;
      break;
    case 33:
      c.spirit.recipe.diagnostic.actor_after.self_symbol.parent++;
      break;
    case 34:
      c.extra.hands_before.left_slot_known = false;
      break;
    case 35:
      c.spirit.recipe.hands.right_slot_known = false;
      break;
    }
    CHECK(!ubc::valid_crafting_snapshot(c.project()));
  }
  // Other raw actor values observed at the same time remain unrelated evidence even if internally consistent.
  {
    auto c = good;
    c.extra.actor_before.self = c.extra.actor_after.self = hero + 4;
    c.extra.actor_before.self_symbol.offset = c.extra.actor_after.self_symbol.offset = hero + 4;
    c.extra.actor_before.matches = c.extra.actor_after.matches = false;
    CHECK(ubc::valid_crafting_snapshot(ubc::project_capture(c.extra)));
    CHECK(!ubc::valid_crafting_snapshot(c.project()));
  }
  // Manipulate the shared stock count, including an otherwise correct row balance.
  {
    auto c = good;
    auto& r = c.spirit.recipe.state.rows[4];
    ++r.stored;
    ++r.available;
    ++r.remaining;
    --r.output_space;
    ++c.spirit.diagnostic.flask.amount;
    CHECK(ubc::valid_crafting_snapshot(ubc::project_capture(c.spirit)));
    CHECK(!ubc::valid_crafting_snapshot(c.project()));
  }
}
}
int main() {
  projection_test::basic();
  projection_test::material_and_entry();
  projection_test::merge();
  std::printf("crafting_spirit_projection_tests: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
