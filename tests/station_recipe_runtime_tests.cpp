// SPDX-License-Identifier: MIT
#include "ubc/stations/station_recipe_runtime.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace station_test {
namespace c = ubc::station_recipe_catalog;
namespace rr = ubc::station_recipe_runtime;
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
  unsigned inventory_reads = 0, hand_reads = 0, anchor_reads = 0;
  void (*on_read)(Image&) noexcept = nullptr;
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
    if (address == 0x2708 && size == 40) ++self.inventory_reads;
    if (address == 0x29b0 && size == 12) ++self.hand_reads;
    if (address == 0x2968 && size == 4) ++self.anchor_reads;
    if (self.on_read) { const auto callback=self.on_read; self.on_read=nullptr; callback(self); }
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
    image.block(ubc::lhiver::game_parser, 0x58);
    image.block(table, static_cast<std::size_t>(ubc::lhiver::symbol_count + 2) * 4);
    image.put(ubc::lhiver::game_parser + 0x18, table);
    image.put(ubc::lhiver::game_parser + 0x1c, ubc::lhiver::symbol_count + 2);
    image.put(ubc::lhiver::game_parser + 0x20, ubc::lhiver::symbol_count);
    image.put(ubc::lhiver::game_parser + 0x48, code);
    image.put(ubc::lhiver::game_parser + 0x50, code + ubc::lhiver::code_length);
    image.put(ubc::lhiver::game_parser + 0x54, ubc::lhiver::code_length + 128);
    populate(descriptor);
  }
  void populate(const c::Descriptor& descriptor) {
    const auto assembled = rr::assemble(descriptor); const auto profile = assembled.profile();
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
      auto expected = function; expected.name = c::function_for(function.symbol)->name; ubc::Sha256 hash;
      hash.update(body, function.byte_length); expected.body_sha256 = hash.finish();
      synthetic_functions.push_back(expected);
    }
    for (const auto& requirement : descriptor.requirements)
      if (requirement.role == c::RequirementRole::knowledge)
        image.put(symbol(static_cast<std::uint32_t>(requirement.symbol)) + 0x18, static_cast<std::uint32_t>(requirement.value));
    image.put(symbol(1683) + 0x1c, 0x2000); image.put(symbol(1679) + 0x1c, 0x2000);
    for (std::size_t i = 0; i < profile.function_count; ++i)
      if (profile.functions[i].type == 7)
        image.put(symbol(profile.functions[i].symbol) + 0x1c, 0x60000000 + profile.functions[i].symbol * 0x100);

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


#ifndef UBC_STATION_RECIPE_FIXTURE_ONLY
int main() {
  using namespace station_test;
  for (const auto& descriptor : c::recipes) {
    auto assembly = rr::assemble(descriptor);
    check(assembly.valid() && assembly.function_count() <= 64 && assembly.binding_count() <= 64,
          "all sixteen exact profiles fit the bounded immutable assembly");
    check(assembly.knowledge_count() == (descriptor.station == c::Station::forge ? 0u : 1u),
          "only genuine callback/Info knowledge enters the learning set");
    const auto p = assembly.profile();
    check(p.parser_address == ubc::lhiver::game_parser && p.code_length == ubc::lhiver::code_length &&
          p.code_sha256 == ubc::lhiver::code_sha256 && p.symbol_count == ubc::lhiver::symbol_count,
          "local assembly preserves existing parser identity semantics without inventing whole-code verification");
    std::set<std::uint32_t> wanted{14646,19965,descriptor.back->symbol,descriptor.callback.function->symbol,
      descriptor.station_profile->entry->symbol};
    for (const auto* owner : {&descriptor.owner,&descriptor.station_profile->end})
      for (const auto* f : {owner->constructor,owner->condition,owner->information}) wanted.insert(f->symbol);
    if (descriptor.station_profile->mob_condition) wanted.insert(descriptor.station_profile->mob_condition->symbol);
    for (const auto& requirement : descriptor.checks) wanted.insert(requirement.function->symbol);
    check(wanted.size() == p.function_count, "full owner/back/callback/station/helper closure deduplicates exactly");
    for (std::size_t i = 0; i < p.function_count; ++i) check(wanted.erase(p.functions[i].symbol) == 1,"every function occurs once");
    Fixture f(descriptor);
    check(f.synthetic_verify().matches, "own synthetic hashes verify metadata including nonzero C_INFO instance offset");
    const auto original = rr::verify(f.image.memory(),ubc::lhiver::game_parser,assembly);
    check(!original.matches && original.issue == rs::Issue::function_hash, "own synthetic bodies never satisfy pinned game hashes");
    auto knowledge = f.knowledge();
    check(knowledge.read_valid && knowledge.known, "knowledge is independent from unavailable resources and synthetic body hashes");
    const auto actor = rr::actor(f.image.memory(),ubc::lhiver::game_parser,assembly,0x2000);
    check(actor.read_valid && actor.matches && actor.hero == 0x2000 &&
          actor.requires_self == (descriptor.material_actor == c::Actor::self), "actual instance offset proves the required material actor");
    f.image.put(symbol(1679)+0x1c,0x3000);
    const auto foreign = rr::actor(f.image.memory(),ubc::lhiver::game_parser,assembly,0x2000);
    check(foreign.read_valid && foreign.matches == (descriptor.material_actor != c::Actor::self),
          "foreign SELF blocks ammo while HERO-only recipes need no invented SELF equality");
    f.image.put(symbol(1683)+0x18,0x2000); f.image.put(symbol(1683)+0x1c,0);
    check(!rr::actor(f.image.memory(),ubc::lhiver::game_parser,assembly,0x2000).matches,
          "bytecode data field is never mistaken for the live HERO instance");
    for (const auto& requirement : descriptor.requirements) {
      if (requirement.role == c::RequirementRole::context) f.image.put(symbol(static_cast<std::uint32_t>(requirement.symbol))+0x18,0);
      if (requirement.role == c::RequirementRole::knowledge) {
        for (const auto value : {0,2,-1,INT32_MAX}) {
          f.learned(requirement.symbol,requirement.element,value); knowledge=f.knowledge();
          check(knowledge.read_valid && !knowledge.known,"only exact learned value one grants knowledge");
        }
        f.learned(requirement.symbol,requirement.element,1);
      }
    }
    check(f.knowledge().known,"context zeros do not erase learned knowledge");
    auto copied=assembly; assembly={};
    check(copied.valid() && rr::knowledge(f.image.memory(),ubc::lhiver::game_parser,copied).known,
          "profile copies own their strings and remain independent of source lifetime");
  }
  const auto& ammo=*c::find(c::RecipeId::arrows);
  for (unsigned mode=0;mode<8;++mode) {
    Fixture f(ammo);const auto* target=ammo.callback.function;
    switch(mode) {
      case 0:f.image.put(symbol(target->symbol)+0x20,metadata(1,5,1));break;
      case 1:f.image.put(symbol(target->symbol)+0x20,metadata(0,7,1));break;
      case 2:f.image.put(symbol(target->symbol)+0x1c,2);break;
      case 3:f.image.put(symbol(target->symbol)+0x18,target->byte_offset+1);break;
      case 4:f.image.put(symbol(target->symbol)+0x38,symbol(1537));break;
      case 5:f.image.put(name_at(target->symbol),0);break;
      case 6:f.image.put(code+target->byte_offset,0);break;
      default:f.image.denied=code+target->byte_offset;f.image.denied_size=target->byte_length;break;
    }
    check(!f.synthetic_verify().matches,"local runtime closure rejects wrong callback metadata/name/body or unreadable body");
  }
  for (unsigned mode=0;mode<7;++mode) {
    Fixture f(ammo);
    switch(mode) {
      case 0:f.image.put(symbol(1679)+0x20,metadata(1,7,1));break;
      case 1:f.image.put(symbol(1683)+0x38,0);break;
      case 2:f.image.put(symbol(1679)+8,0);break;
      case 3:f.image.denied=symbol(1679);f.image.denied_size=0x3c;break;
      case 4:f.image.change_address=symbol(1679);f.image.change_target=symbol(1679)+0x1c;f.image.change_on_read=2;break;
      case 5:f.image.put(ubc::lhiver::game_parser+0x48,UINT32_MAX);break;
      default:f.image.change_layout=true;break;
    }
    const auto actor=rr::actor(f.image.memory(),ubc::lhiver::game_parser,f.assembly,0x2000);
    check(!actor.read_valid&&!actor.matches,"wrong/unreadable/changing actor binding and layout fail closed");
  }
  {
    Fixture f(ammo);f.image.put(symbol(10491)+0x20,metadata(1,1,0));
    check(!f.knowledge().read_valid,"noninteger learned binding is rejected");
    f.image.put(symbol(10491)+0x20,metadata(1,2,0));
    f.image.change_address=symbol(10491);f.image.change_target=symbol(10491)+0x18;f.image.change_on_read=2;
    check(!f.knowledge().read_valid,"changed learned value is not accepted as stable knowledge");
  }
  {
    Fixture f(ammo);
    check(!rr::knowledge(f.image.memory(),ubc::lhiver::game_parser+4,f.assembly).read_valid,
          "foreign parser cannot provide learned knowledge");
    check(!rr::actor(f.image.memory(),ubc::lhiver::game_parser+4,f.assembly,0x2000).read_valid,
          "foreign parser cannot provide material actor identity");
    auto bad=ammo;bad.checks.count=65;
    check(!rr::assemble(bad).valid(),"oversized descriptor views are rejected before walking them");
    bad=ammo;bad.owner.condition=nullptr;
    check(!rr::assemble(bad).valid(),"broken owner function cannot enter profile closure");
  }
  std::printf("station_recipe_runtime_tests: %u checks, %u failures\n",checks,failures);
  return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
#endif
