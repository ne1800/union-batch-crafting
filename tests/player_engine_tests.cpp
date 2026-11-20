// SPDX-License-Identifier: MIT
#include "ubc/player_engine.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace pe = ubc::player_engine;
using Bytes = std::vector<std::uint8_t>;
void word(Bytes& bytes, unsigned offset, unsigned value) {
  for (unsigned i = 0; i < 4; ++i) bytes[offset+i] = static_cast<std::uint8_t>(value >> (8*i));
}
bool all_active(const pe::Result& r) { return r.failure == pe::Result{}.failure; }
bool only_saw_blocked(pe::Result r) {
  if (!r.failure[3]) return false;
  r.failure[3] = 0;
  return all_active(r);
}
int main(int argc, char** argv) {
  if (argc == 2) {
    std::ifstream file(argv[1], std::ios::binary);
    Bytes image{std::istreambuf_iterator<char>(file), {}};
    auto inspect = [&] {return pe::inspect(image.data(), image.size(), pe::sites, std::size(pe::sites));};
    if (!all_active(inspect()) || image.size() < 0x4c3000) return 1;
    image[0x759270-0x400000] ^= 1;
    if (!only_saw_blocked(inspect())) return 2;
    image[0x759270-0x400000] ^= 1;
    image[ubc::detail::read_le32_unchecked(image.data()+60)+8] ^= 1; // Link timestamp.
    image[0x1000] ^= 1; // Unused engine code outside the recorded functions.
    image[0x4c2800] ^= 1; // An icon/resource byte outside the engine contract.
    return all_active(inspect()) ? 0 : 3;
  }
  Bytes image(1024);
  word(image, 0, 0x5a4d); word(image, 60, 128); word(image, 128, 0x4550);
  word(image, 132, 0x14c); word(image, 136, 0x3f966d87); word(image, 148, 224);
  word(image, 152, 0x10b); word(image, 180, 0x400000); word(image, 208, 0x752000);
  constexpr char zeros[] = "374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb";
  const pe::Site checks[]{{400, 16, pe::saw, zeros}, {480, 16, pe::all, zeros}};
  auto inspect = [&] {return pe::inspect(image.data(), image.size(), checks, 2);};
  image[450] ^= 1;
  if (!all_active(inspect())) return 4;
  image[415] ^= 1;
  if (!only_saw_blocked(inspect())) return 5;
  image[480] ^= 1;
  if (inspect().any()) return 6;
  image.resize(42);
  return inspect().any() ? 7 : 0;
}
