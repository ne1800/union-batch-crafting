// SPDX-License-Identifier: MIT
#include "ubc/recipe_runtime.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {
namespace c = ubc::recipe_catalog;
namespace rr = ubc::recipe_runtime;
namespace rs = ubc::runtime_script;
unsigned checks = 0, failures = 0;
void check(bool value, const char* name) {
  ++checks;
  if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", name); }
}
constexpr std::uint32_t table = 0x10000000, code = 0x20000000;
constexpr std::uint32_t symbol(std::uint32_t index) { return 0x30000000 + index * 0x80; }
constexpr std::uint32_t name_at(std::uint32_t index) { return 0x40000000 + index * 0x80; }
constexpr std::uint32_t array_at(std::uint32_t index) { return 0x50000000 + index * 0x100; }
std::uint32_t metadata(std::uint32_t count, std::uint32_t type, std::uint32_t flags) {
  return count | (type << 12u) | (flags << 16u);
}

struct Image {
  std::map<std::uint32_t, std::vector<std::uint8_t>> blocks;
  std::uint32_t denied = 0, change_address = 0, change_target = 0;
  std::size_t denied_size = 0, reads = 0, code_reads = 0;
  unsigned change_on_read = 0, watched_reads = 0, header_reads = 0;
  bool change_layout = false;
  void block(std::uint32_t address, std::size_t size) { blocks[address].resize(size); }
  std::uint8_t* at(std::uint32_t address, std::size_t size) {
    auto found = blocks.upper_bound(address);
    if (found == blocks.begin()) return nullptr;
    --found;
    const auto offset = static_cast<std::size_t>(address - found->first);
    if (offset > found->second.size() || size > found->second.size() - offset) return nullptr;
    return found->second.data() + offset;
  }
  void put(std::uint32_t address, std::uint32_t value) {
    auto* bytes = at(address, 4);
    if (!bytes) std::abort();
    for (unsigned i = 0; i < 4; ++i) bytes[i] = static_cast<std::uint8_t>(value >> (8u * i));
  }
  static bool read(void* context, std::uint32_t address, void* output, std::size_t size) noexcept {
    auto& self = *static_cast<Image*>(context); ++self.reads;
    if (address >= code && address < code + ubc::lhiver::code_length + 128) ++self.code_reads;
    if (address == ubc::lhiver::game_parser && size == 0x58) {
      ++self.header_reads;
      if (self.change_layout && self.header_reads == 2)
        self.put(ubc::lhiver::game_parser + 0x20, ubc::lhiver::symbol_count + 1);
    }
    if (address == self.change_address) {
      ++self.watched_reads;
      if (self.change_on_read && self.watched_reads == self.change_on_read)
        self.put(self.change_target ? self.change_target : address, 0);
    }
    if (self.denied_size && static_cast<std::uint64_t>(address) + size > self.denied &&
        static_cast<std::uint64_t>(self.denied) + self.denied_size > address) return false;
    const auto* bytes = self.at(address, size);
    if (!bytes) return false;
    std::memcpy(output, bytes, size); return true;
  }
  ubc::ReadOnlyMemory memory() { return {this, &read}; }
};

struct Fixture {
  rr::ProfileAssembly assembly;
  Image image;
  std::vector<ubc::lhiver::Function> synthetic_functions;
  explicit Fixture(const c::Descriptor& descriptor) : assembly(rr::assemble(descriptor)) {
    const auto profile = assembly.profile();
    image.block(ubc::lhiver::game_parser, 0x58);
    image.block(table, static_cast<std::size_t>(ubc::lhiver::symbol_count + 2) * 4);
    image.put(ubc::lhiver::game_parser + 0x18, table);
    image.put(ubc::lhiver::game_parser + 0x1c, ubc::lhiver::symbol_count + 2);
    image.put(ubc::lhiver::game_parser + 0x20, ubc::lhiver::symbol_count);
    image.put(ubc::lhiver::game_parser + 0x48, code);
    image.put(ubc::lhiver::game_parser + 0x50, code + ubc::lhiver::code_length);
    image.put(ubc::lhiver::game_parser + 0x54, ubc::lhiver::code_length + 128);
    for (std::size_t i = 0; i < profile.binding_count; ++i) {
      const auto& binding = profile.bindings[i];
      make_symbol(binding.symbol, binding.name, binding.elements, binding.type, binding.flags,
                  binding.check_value ? static_cast<std::uint32_t>(binding.value) : 0,
                  static_cast<std::uint32_t>(binding.offset), binding.parent_index);
      if (binding.type == 2 && binding.parent_index == -1 && binding.elements > 1) {
        image.block(array_at(binding.symbol), static_cast<std::size_t>(binding.elements) * 4);
        image.put(symbol(binding.symbol) + 0x18, array_at(binding.symbol));
        for (std::uint32_t at = 0; at < binding.elements; ++at) image.put(array_at(binding.symbol) + at * 4, 1);
      }
    }
    for (std::size_t i = 0; i < profile.function_count; ++i) {
      const auto& function = profile.functions[i];
      make_symbol(function.symbol, function.name, function.argc, function.type, function.flags,
                  function.byte_offset, function.return_type, function.parent_index);
      // Our deterministic test pattern only. No game bytes.
      image.block(code + function.byte_offset, function.byte_length);
      auto* body = image.at(code + function.byte_offset, function.byte_length);
      for (std::uint32_t at = 0; at < function.byte_length; ++at)
        body[at] = static_cast<std::uint8_t>((function.symbol + at * 37u) % 256u);
      auto expected = function; ubc::Sha256 hash;
      hash.update(body, function.byte_length); expected.body_sha256 = hash.finish();
      synthetic_functions.push_back(expected);
    }
    for (const auto& requirement : descriptor.requirements)
      if (requirement.role == c::RequirementRole::knowledge)
        learned(requirement.symbol, requirement.element, requirement.value);
  }
  void make_symbol(std::uint32_t index, const char* name, std::uint32_t elements,
                   std::uint32_t type, std::uint32_t flags, std::uint32_t data,
                   std::uint32_t offset, std::int32_t parent) {
    image.block(symbol(index), 0x3c); image.block(name_at(index), std::strlen(name) + 1);
    image.put(table + index * 4, symbol(index));
    image.put(symbol(index) + 8, name_at(index));
    image.put(symbol(index) + 0xc, static_cast<std::uint32_t>(std::strlen(name)));
    image.put(symbol(index) + 0x10, static_cast<std::uint32_t>(std::strlen(name) + 1));
    image.put(symbol(index) + 0x18, data); image.put(symbol(index) + 0x1c, offset);
    image.put(symbol(index) + 0x20, metadata(elements, type, flags));
    image.put(symbol(index) + 0x38, parent < 0 ? 0 : symbol(static_cast<std::uint32_t>(parent)));
    std::memcpy(image.at(name_at(index), std::strlen(name) + 1), name, std::strlen(name) + 1);
    if (parent >= 0 && !image.at(symbol(static_cast<std::uint32_t>(parent)), 0x3c)) {
      image.block(symbol(static_cast<std::uint32_t>(parent)), 0x3c);
      image.put(table + static_cast<std::uint32_t>(parent) * 4, symbol(static_cast<std::uint32_t>(parent)));
    }
  }
  void learned(std::int32_t index, std::uint32_t element, std::int32_t value) {
    const auto profile = assembly.profile();
    for (std::size_t i = 0; i < profile.binding_count; ++i) {
      const auto& binding = profile.bindings[i];
      if (binding.symbol == static_cast<std::uint32_t>(index)) {
        image.put(binding.elements > 1 ? array_at(binding.symbol) + element * 4 : symbol(binding.symbol) + 0x18,
                  static_cast<std::uint32_t>(value)); return;
      }
    }
    std::abort();
  }
  rs::RuntimeContract synthetic_verify() {
    auto profile = assembly.profile(); profile.functions = synthetic_functions.data();
    return rs::verify_local_contract(image.memory(), ubc::lhiver::game_parser, profile);
  }
  rr::Knowledge knowledge() { return rr::knowledge(image.memory(), ubc::lhiver::game_parser, assembly); }
};
}

int main() {
  const auto& meat = *c::find(c::RecipeId::meat);
  const auto& health = *c::find(c::RecipeId::health_01);
  for (const auto& descriptor : c::recipes) {
    auto assembly = rr::assemble(descriptor);
    check(assembly.valid() && assembly.issue() == rr::AssemblyIssue::none, "all nine immutable profiles assemble");
    const auto profile = assembly.profile();
    check(profile.parser_address == ubc::lhiver::profile.parser_address &&
          profile.symbol_count == ubc::lhiver::profile.symbol_count &&
          profile.code_length == ubc::lhiver::profile.code_length &&
          profile.code_sha256 == ubc::lhiver::profile.code_sha256,
          "assembly retains existing parser/count/code identity facts without inventing a new whole hash");
    std::set<std::uint32_t> wanted_functions{14646, 19965, descriptor.station_entry->symbol, descriptor.station_end->symbol};
    for (const auto& owner : descriptor.owners)
      for (const auto* function : {owner.constructor, owner.condition, owner.information}) wanted_functions.insert(function->symbol);
    for (const auto& callback : descriptor.callbacks) wanted_functions.insert(callback.function->symbol);
    check(profile.function_count == wanted_functions.size(), "owner/callback/helper aliases deduplicate into the complete function set");
    for (std::size_t i = 0; i < profile.function_count; ++i)
      check(wanted_functions.erase(profile.functions[i].symbol) == 1, "every function occurs exactly once");
    std::set<std::uint32_t> wanted_bindings;
    for (const auto& binding : c::common_bindings) wanted_bindings.insert(binding.symbol);
    for (const auto& binding : descriptor.bindings) wanted_bindings.insert(binding.symbol);
    check(profile.binding_count == wanted_bindings.size(), "common and recipe bindings deduplicate by exact symbol contract");
    for (std::size_t i = 0; i < profile.binding_count; ++i)
      check(wanted_bindings.erase(profile.bindings[i].symbol) == 1, "every binding occurs exactly once");
    const auto expected_knowledge = descriptor.id == c::RecipeId::meat || descriptor.id == c::RecipeId::mushroom_dish ? 0u : 1u;
    check(assembly.knowledge_count() == expected_knowledge, "context and resources are not knowledge predicates");
    Fixture f(descriptor);
    check(f.synthetic_verify().matches, "assembled metadata verifies against independently owned synthetic bodies");
    const auto original = rr::verify(f.image.memory(), ubc::lhiver::game_parser, descriptor);
    check(!original.matches && original.issue == rs::Issue::function_hash,
          "synthetic bodies cannot satisfy the original pinned function hashes");
    const auto before = f.image.code_reads;
    auto learned = f.knowledge();
    check(learned.read_valid && learned.known && learned.checked == expected_knowledge,
          "learned arrays or no learning predicate are reported independently of production readiness");
    check(f.image.code_reads == before, "knowledge never hashes or executes recipe bodies");
    f.image.put(symbol(12330) + 0x18, 0);
    for (const auto& requirement : descriptor.requirements)
      if (requirement.role == c::RequirementRole::context && requirement.symbol != 12330)
        f.image.put(symbol(static_cast<std::uint32_t>(requirement.symbol)) + 0x18, 0);
    learned = f.knowledge();
    check(learned.read_valid && learned.known, "zero production/start flags do not unlearn a recipe");
    // The fixture has no inventory object. Owner resource
    // conditions must not trigger an implicit engine count either.
    for (const auto& requirement : descriptor.requirements) if (requirement.role == c::RequirementRole::knowledge) {
      for (const auto value : {0, 2, -1, INT32_MAX}) {
        f.learned(requirement.symbol, requirement.element, value); learned = f.knowledge();
        check(learned.read_valid && !learned.known && learned.issue == rs::Issue::none,
              "only exact learned predicate value grants knowledge, not truthiness");
      }
    }
  }
  {
    std::vector<ubc::lhiver::SymbolBinding> bindings(meat.bindings.begin(), meat.bindings.end());
    bindings.push_back(c::common_bindings[0]);
    auto descriptor = meat; descriptor.bindings = {bindings.data(), bindings.size()};
    auto assembly = rr::assemble(descriptor);
    check(assembly.valid() && assembly.profile().binding_count == rr::assemble(meat).binding_count(),
          "identical binding repetition does not consume an additional profile slot");
    bindings.back().elements = 1;
    check(!rr::assemble(descriptor).valid(), "conflicting duplicate binding is rejected instead of silently taking the first");
    bindings.back() = c::common_bindings[0]; bindings.back().symbol = 40001;
    check(!rr::assemble(descriptor).valid(), "one name cannot identify two symbols");
  }
  {
    std::vector<c::Owner> owners(meat.owners.begin(), meat.owners.end());
    auto changed = *owners[0].condition; changed.body_sha256[0] ^= 1;
    owners[1].condition = &changed;
    auto descriptor = meat; descriptor.owners = {owners.data(), owners.size()};
    check(!rr::assemble(descriptor).valid(), "same function symbol with conflicting body hash is rejected");
    changed = *owners[0].condition;
    check(rr::assemble(descriptor).valid(), "exact repeated function metadata may share one profile entry");
    changed.symbol = 40000;
    check(!rr::assemble(descriptor).valid(), "entry/name aliases under another symbol are not deduplicated as identical");
  }
  {
    std::vector<ubc::lhiver::SymbolBinding> bindings(meat.bindings.begin(), meat.bindings.end());
    std::vector<std::string> names; names.reserve(40);
    const auto baseline = rr::assemble(meat).binding_count();
    for (std::size_t i = baseline; i < 64; ++i) {
      names.push_back("SYNTHETIC_" + std::to_string(i));
      bindings.push_back({names.back().c_str(), static_cast<std::uint32_t>(40000 + i), 2, 0, 1, -1});
    }
    auto descriptor = meat; descriptor.bindings = {bindings.data(), bindings.size()};
    const auto exact = rr::assemble(descriptor);
    check(exact.valid() && exact.binding_count() == 64, "exact unique binding limit is supported after deduplication");
    bindings.push_back({"SYNTHETIC_OVERFLOW", 50000, 2, 0, 1, -1});
    descriptor.bindings = {bindings.data(), bindings.size()};
    const auto excess = rr::assemble(descriptor);
    check(!excess.valid() && excess.issue() == rr::AssemblyIssue::capacity,
          "sixty-fifth unique binding fails without a partial usable profile");
    check(!rs::detail::valid_profile(excess.profile()), "invalid assembly never exposes an empty permissive local profile");
  }
  {
    std::vector<c::Owner> owners(meat.owners.begin(), meat.owners.end());
    std::vector<ubc::lhiver::Function> functions; functions.reserve(51);
    std::vector<std::string> names; names.reserve(51);
    for (unsigned i = 0; i < 17; ++i) {
      const auto start = functions.size();
      for (unsigned part = 0; part < 3; ++part) {
        auto function = part == 0 ? *meat.owners[0].constructor : part == 1 ? *meat.owners[0].condition : *meat.owners[0].information;
        names.push_back("OWN_FUNCTION_" + std::to_string(i * 3 + part));
        function.name = names.back().c_str(); function.symbol = 50000 + i * 3 + part;
        function.byte_offset = 1000000 + (i * 3 + part) * 256;
        functions.push_back(function);
      }
      auto owner = meat.owners[0]; owner.instance = static_cast<std::int32_t>(functions[start].symbol);
      owner.constructor = &functions[start]; owner.condition = &functions[start + 1]; owner.information = &functions[start + 2];
      owners.push_back(owner);
      if (i == 15) {
        auto descriptor = meat; descriptor.owners = {owners.data(), owners.size()};
        const auto exact = rr::assemble(descriptor);
        check(exact.valid() && exact.function_count() == 64, "exact unique function limit is supported");
      }
    }
    auto descriptor = meat; descriptor.owners = {owners.data(), owners.size()};
    const auto excess = rr::assemble(descriptor);
    check(!excess.valid() && excess.issue() == rr::AssemblyIssue::capacity, "excess functions fail before a partial profile is used");
  }
  {
    auto descriptor = meat; descriptor.bindings.count = std::numeric_limits<std::size_t>::max();
    const auto invalid = rr::assemble(descriptor);
    check(!invalid.valid(), "bounded metadata counts are checked before traversing own descriptor views");
    Image memory;
    check(!rr::verify(memory.memory(), ubc::lhiver::game_parser, invalid).matches && memory.reads == 0,
          "invalid assembly causes no runtime memory reads");
    const auto missing = rr::knowledge(memory.memory(), ubc::lhiver::game_parser, invalid);
    check(!missing.read_valid && !missing.known && memory.reads == 0,
          "invalid metadata never becomes a vacuously known recipe");
    descriptor = meat; descriptor.id = static_cast<c::RecipeId>(999);
    check(!rr::assemble(descriptor).valid(), "new families need their own helper-dependency contract");
  }
  {
    auto descriptor = health;
    std::vector<c::Requirement> requirements(health.requirements.begin(), health.requirements.end());
    requirements.push_back({c::RequirementKind::integer_equal, c::RequirementRole::knowledge, 1464, 0, 0});
    descriptor.requirements = {requirements.data(), requirements.size()};
    check(!rr::assemble(descriptor).valid(), "conflicting knowledge predicates are not silently merged");
    requirements.back() = {c::RequirementKind::integer_equal, c::RequirementRole::knowledge, 1464, 1, 1};
    check(!rr::assemble(descriptor).valid(), "owner-specific alternatives need an explicit contract instead of an invented global AND");
  }
  {
    auto assembly = rr::assemble(health); auto copied = assembly; rr::ProfileAssembly assigned; assigned = copied;
    const auto a = assembly.profile(), b = copied.profile(), d = assigned.profile();
    check(a.functions != b.functions && b.functions != d.functions && a.bindings != b.bindings,
          "assembly copies own function and binding storage");
    check(a.functions[0].name != b.functions[0].name && std::strcmp(a.functions[0].name, b.functions[0].name) == 0,
          "copied profiles rebind their own immutable name storage");
    assembly = rr::assemble(meat);
    check(copied.function_count() == assigned.function_count() &&
          std::strcmp(copied.profile().functions[0].name, "PC_ITPO_HEALTH_01") == 0,
          "replacing the source assembly cannot invalidate a copy");
  }
  {
    std::vector<c::Owner> owners(meat.owners.begin(), meat.owners.end());
    char mutable_name[64]{};
    const auto name_size = std::strlen(owners[0].constructor->name) + 1;
    if (name_size > sizeof(mutable_name)) std::abort();
    std::memcpy(mutable_name, owners[0].constructor->name, name_size);
    auto constructor = *owners[0].constructor; constructor.name = mutable_name; owners[0].constructor = &constructor;
    auto descriptor = meat; descriptor.owners = {owners.data(), owners.size()};
    const auto assembly = rr::assemble(descriptor);
    const auto old_offset = constructor.byte_offset; mutable_name[0] = 'X'; constructor.byte_offset += 1;
    check(assembly.valid() && std::strcmp(assembly.profile().functions[0].name, "PC_FRYINGMEAT_ADDON") == 0 &&
          assembly.profile().functions[0].byte_offset == old_offset,
          "descriptor names and function records cannot mutate an already assembled contract");
  }
  {
    auto descriptor = health;
    std::vector<c::Requirement> requirements(health.requirements.begin(), health.requirements.end());
    std::vector<c::Owner> owners(health.owners.begin(), health.owners.end());
    std::vector<c::Requirement> conditions(owners[0].conditions.begin(), owners[0].conditions.end());
    for (auto& requirement : requirements) if (requirement.role == c::RequirementRole::knowledge) requirement.element = 14;
    for (auto& condition : conditions) if (condition.role == c::RequirementRole::knowledge) condition.element = 14;
    descriptor.requirements = {requirements.data(), requirements.size()};
    owners[0].conditions = {conditions.data(), conditions.size()}; descriptor.owners = {owners.data(), owners.size()};
    check(rr::assemble(descriptor).valid(), "last declared learned array element is supported");
    Fixture f(descriptor); check(f.knowledge().read_valid && f.knowledge().known, "array element fourteen is read at its exact offset");
    f.learned(1464, 0, 0);
    check(f.knowledge().known, "unrelated learned array elements do not decide this recipe");
    f.learned(1464, 14, 0);
    check(f.knowledge().read_valid && !f.knowledge().known, "last learned element is not confused with the first");
    for (auto& requirement : requirements) if (requirement.role == c::RequirementRole::knowledge) requirement.element = 15;
    for (auto& condition : conditions) if (condition.role == c::RequirementRole::knowledge) condition.element = 15;
    check(!rr::assemble(descriptor).valid(), "one-past-array requirement is rejected before a memory read");
  }
  {
    auto descriptor = health;
    std::vector<c::Requirement> requirements(health.requirements.begin(), health.requirements.end());
    std::vector<c::Owner> owners(health.owners.begin(), health.owners.end());
    requirements.push_back({c::RequirementKind::integer_equal, c::RequirementRole::knowledge, 1464, 1, 1});
    descriptor.requirements = {requirements.data(), requirements.size()};
    owners[0].conditions = descriptor.requirements; descriptor.owners = {owners.data(), owners.size()};
    Fixture f(descriptor);
    check(f.assembly.valid() && f.assembly.knowledge_count() == 2 && f.knowledge().known,
          "several verified knowledge predicates form one explicit common AND");
    f.learned(1464, 0, 0); f.image.denied = array_at(1464) + 4; f.image.denied_size = 4;
    const auto value = f.knowledge();
    check(!value.read_valid && !value.known, "an unlearned first predicate cannot hide an unreadable later predicate");
  }
  for (unsigned variant = 0; variant < 10; ++variant) {
    Fixture f(health);
    switch (variant) {
      case 0: f.image.put(symbol(1464) + 0x20, metadata(15, 3, 0)); break;
      case 1: f.image.put(symbol(1464) + 0x20, metadata(15, 2, 4)); break;
      case 2: f.image.put(symbol(1464) + 0x20, metadata(14, 2, 0)); break;
      case 3: f.image.put(symbol(1464) + 0x38, symbol(1537)); break;
      case 4: f.image.at(name_at(1464), 1)[0] = 'X'; break;
      case 5: f.image.put(symbol(1464) + 0x18, 0); break;
      case 6: f.image.put(symbol(1464) + 0x18, UINT32_MAX - 3); break;
      case 7: f.image.put(symbol(1464) + 0x18, array_at(1464) + 1); break;
      case 8: f.image.denied = array_at(1464); f.image.denied_size = 4; break;
      case 9: f.image.denied = symbol(1464); f.image.denied_size = 0x3c; break;
    }
    const auto value = f.knowledge();
    check(!value.read_valid && !value.known && value.issue != rs::Issue::none,
          "learned variable must retain exact binding identity and a bounded readable array");
  }
  {
    Fixture f(health); f.image.change_address = array_at(1464); f.image.change_on_read = 2;
    const auto value = f.knowledge();
    check(!value.read_valid && !value.known && value.issue == rs::Issue::changed_during_read,
          "changing learned value between passes invalidates the entire knowledge read");
  }
  {
    Fixture f(health); f.image.change_layout = true;
    const auto value = f.knowledge();
    check(!value.read_valid && !value.known && value.issue == rs::Issue::changed_during_read,
          "knowledge values never outlive a changed parser layout");
  }
  {
    const auto& recipe = *c::find(c::RecipeId::molerat_dish);
    Fixture f(recipe); f.image.change_address = symbol(10535); f.image.change_target = symbol(10535) + 0x18;
    f.image.change_on_read = 2;
    const auto value = f.knowledge();
    check(!value.read_valid && !value.known && value.issue == rs::Issue::changed_during_read,
          "scalar inline learning changes are detected as well as pointed array changes");
  }
  {
    Fixture f(health); const auto before = f.image.reads;
    const auto value = rr::knowledge(f.image.memory(), ubc::lhiver::game_parser + 4, f.assembly);
    check(!value.read_valid && !value.known && value.issue == rs::Issue::wrong_parser && f.image.reads == before,
          "foreign parser is rejected before dereferencing anything");
    check(!rr::verify(f.image.memory(), ubc::lhiver::game_parser + 4, health).matches && f.image.reads == before,
          "recipe verification preserves canonical parser semantics");
  }
  {
    Fixture f(health); f.image.denied = symbol(20330); f.image.denied_size = 0x3c;
    const auto value = f.knowledge();
    check(value.known && value.read_valid, "unreadable station-start flag is a separate context gate, not missing knowledge");
    f.image.denied = 0; f.image.denied_size = 0;
    f.image.put(ubc::lhiver::game_parser + 0x20, ubc::lhiver::symbol_count + 1);
    f.image.put(ubc::lhiver::game_parser + 0x50, code + ubc::lhiver::code_length + 32);
    check(f.synthetic_verify().matches && f.knowledge().known,
          "local contract retains existing extension semantics without claiming full-code identity");
  }
  {
    Fixture f(health); const auto first = f.assembly.profile().functions[0];
    f.image.at(code + first.byte_offset, 1)[0] ^= 1;
    const auto result = f.synthetic_verify();
    check(!result.matches && result.issue == rs::Issue::function_hash && result.failed_symbol == first.symbol,
          "changed recipe body rejects the assembled synthetic local contract");
  }
  {
    Fixture f(health); f.image.put(symbol(1464) + 0x20, metadata(15, 2, 1));
    const auto result = f.synthetic_verify();
    check(!result.matches && result.issue == rs::Issue::symbol_signature && result.failed_symbol == 1464,
          "assembled binding metadata is checked after valid synthetic function hashes");
  }
  {
    Fixture f(health); const auto first = f.assembly.profile().functions[0];
    f.image.put(symbol(first.symbol) + 0x20, metadata(first.argc, first.type, first.flags ^ 1u));
    const auto result = rr::verify(f.image.memory(), ubc::lhiver::game_parser, f.assembly);
    check(!result.matches && result.issue == rs::Issue::symbol_signature && result.failed_symbol == first.symbol,
          "public verify preserves exact function metadata before hashing the body");
  }
  {
    Fixture f(health);
    auto left = f.assembly.profile();
    auto right = left;
    left.functions = &f.synthetic_functions.at(0);
    right.functions = &f.synthetic_functions.at(1);
    left.function_count = right.function_count = 1;
    const auto verify = [&](const ubc::lhiver::Profile& profile) {
      return rs::verify_local_contract(f.image.memory(), ubc::lhiver::game_parser, profile).matches;
    };
    f.image.block(code + ubc::lhiver::code_length - 1, 1);
    *f.image.at(code + ubc::lhiver::code_length - 1, 1) = 0x73;
    check(verify(left) && verify(right), "unreferenced byte does not reject either script family");
    *f.image.at(code + left.functions->byte_offset, 1) ^= 1;
    check(!verify(left) && verify(right), "one mutated script body rejects only its consumer");
  }
  std::printf("RecipeRuntime: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
