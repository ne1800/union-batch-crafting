// SPDX-License-Identifier: MIT
#include "ubc/stations/bow/bow_item_code.hpp"
#define UBC_BOW_RUNTIME_FIXTURE_ONLY
#include "bow_recipe_runtime_tests.cpp"
#include <fstream>

namespace {
namespace bc = ubc::bow_item_code;
using namespace bow_test;
struct Externals : Fixture {
  Externals() : Fixture(cat::recipes.front()) {
    for (const auto& b : bc::bindings) {
      const auto pointer = b.symbol == 160   ? 0x6e72e0u
                           : b.symbol == 170 ? 0x6e7b60u
                           : b.symbol == 184 ? 0x6e8200u
                                             : 0u;
      make_symbol(b.symbol, b.name, b.elements, b.type, b.flags, pointer,
                  static_cast<std::uint32_t>(b.offset), b.parent_index);
    }
  }
  bc::Check verify() { return bc::verify_externals(image.memory(), ubc::lhiver::game_parser); }
};
struct PE {
  std::vector<std::uint8_t> data;
  std::uint32_t base = 0, sections = 0, count = 0;
  explicit PE(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
      std::abort();
    const auto n = f.tellg();
    if (n <= 0 || n > 16 * 1024 * 1024)
      std::abort();
    data.resize(static_cast<std::size_t>(n));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(data.data()), n);
    ubc::Sha256 hash;
    hash.update(data.data(), data.size());
    if (!f || hash.finish() != ubc::station_abi::executable_sha256)
      std::abort();
    const auto pe = number(0x3c);
    if (pe > data.size() - 248 || number(pe) != 0x4550)
      std::abort();
    const auto opt = pe + 24;
    count = data[pe + 6] | (std::uint32_t{data[pe + 7]} << 8u);
    sections = opt + data[pe + 20] + (std::uint32_t{data[pe + 21]} << 8u);
    base = number(opt + 28);
    if (!count || count > 32 || std::uint64_t{sections} + 40 * count > data.size())
      std::abort();
  }
  std::uint32_t number(std::size_t at) const {
    if (at > data.size() || 4 > data.size() - at)
      std::abort();
    return ubc::runtime_script::detail::u32(data.data() + at);
  }
  const std::uint8_t* at(std::uint32_t address, std::size_t length) const {
    if (address < base)
      std::abort();
    const auto rva = address - base;
    for (std::uint32_t i = 0; i < count; ++i) {
      const auto s = sections + i * 40;
      const auto start = number(s + 12), size = number(s + 16), raw = number(s + 20);
      if (rva >= start && rva - start <= size && length <= size - (rva - start) &&
          std::uint64_t{raw} + rva - start + length <= data.size())
        return data.data() + raw + rva - start;
    }
    std::abort();
  }
  void copy(Image& image, std::uint32_t address, std::size_t length) const {
    image.block(address, length);
    std::memcpy(image.at(address, length), at(address, length), length);
  }
};
void local_file(const char* path) {
  const PE exe(path);
  Image memory;
  for (const auto& r : bc::ranges)
    exe.copy(memory, r.address, r.length);
  // One complete block per vtable avoids overlapping fixture fragments.
  exe.copy(memory, 0x83c804, 0x80);
  exe.copy(memory, 0x83c714, 0xc4);
  check(bc::verify_original_parts(memory.memory()).verified,
        "fifteen complete original areas and exact vslots from pinned EXE");
  for (const auto& r : bc::ranges) {
    auto* bytes = memory.at(r.address, r.length);
    bytes[r.length - 1] ^= 1u;
    const auto out = bc::verify_original_parts(memory.memory());
    check(!out.verified && out.address == r.address,
          "last byte including jump table belongs to code contract");
    bytes[r.length - 1] ^= 1u;
  }
  for (const auto& slot : bc::slots) {
    memory.put(slot.address, slot.target + 1);
    check(!bc::verify_original_parts(memory.memory()).verified,
          "changed dynamic native destination rejects");
    memory.put(slot.address, slot.target);
  }
  for (const auto& r : bc::ranges) {
    memory.denied = r.address + 1;
    memory.denied_size = 1;
    check(!bc::verify_original_parts(memory.memory()).verified,
          "one unreadable original byte cannot become trusted");
  }
  memory.denied_size = 0;
  memory.change_address = bc::ranges[0].address;
  memory.change_target = bc::ranges[0].address;
  memory.change_on_read = 2;
  check(!bc::verify_original_parts(memory.memory()).verified, "change between code copies rejects");
}
}
int main(int argc, char** argv) {
  Externals good;
  check(good.verify().verified, "exact native External plus all eleven symbol records");
  for (const auto& b : bc::bindings)
    for (unsigned field : {0x20u, 0x38u, 0xcu}) {
      Externals f;
      const auto at = symbol(b.symbol) + field;
      if (field == 0x20) {
        const auto old = ubc::runtime_script::detail::u32(f.image.at(at, 4));
        f.image.put(at, old ^ 0x1000u);
      } else
        f.image.put(at, 1);
      check(!f.verify().verified, "signature parent and name-length changes reject every binding");
    }
  for (unsigned id : {160u, 170u, 184u}) {
    Externals f;
    f.image.put(symbol(id) + 0x18, 0x12345678);
    const auto out = f.verify();
    check(!out.verified && out.issue == bc::Issue::target && out.symbol == id,
          "a signature match cannot bless arbitrary native pointer");
    Externals ret;
    ret.image.put(symbol(id) + 0x1c, 9);
    check(!ret.verify().verified, "native return type remains fixed");
  }
  {
    Externals f;
    f.image.put(symbol(161) + 0x1c, 0x54321000);
    check(f.verify().verified, "parameter instance is not confused with external return type");
  }
  {
    Externals f;
    f.image.denied = symbol(187);
    f.image.denied_size = 4;
    check(!f.verify().verified, "unreadable final parameter rejects");
  }
  {
    Externals f;
    f.image.change_layout = true;
    check(!f.verify().verified, "unstable parser layout rejects");
  }
  {
    Externals f;
    f.image.change_address = symbol(160);
    f.image.change_target = symbol(160) + 0x18;
    f.image.change_on_read = 3;
    check(!f.verify().verified, "target changes across binding passes reject");
  }
  {
    Image blank;
    check(!bc::verify_original_parts(blank.memory()).verified, "missing code stays unknown");
    check(!bc::observe_modules(blank.memory(), {}, {}).observed,
          "absent module ownership never grants dispatch evidence");
  }
  if (argc == 3 && std::string(argv[1]) == "--local-exe")
    local_file(argv[2]);
  else if (argc != 1)
    return 2;
  std::printf("%u checks, %u failures\n", station_test::checks, station_test::failures);
  return station_test::failures ? 1 : 0;
}
