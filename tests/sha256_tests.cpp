#include "ubc/core/sha256.hpp"
#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace {
unsigned checks = 0;
unsigned failures = 0;
std::string hex(const std::array<std::uint8_t, 32>& digest) {
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  for (const auto value : digest) {
    result += digits[value >> 4];
    result += digits[value & 15];
  }
  return result;
}
void require(bool condition, const char* name) {
  ++checks;
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}
void check(const void* data, std::size_t size, const char* expected) {
  ubc::Sha256 whole;
  whole.update(nullptr, 0);
  whole.update(data, size);
  require(hex(whole.finish()) == expected, "complete message");
  require(hex(whole.finish()) == expected, "non-mutating finish");
  ubc::Sha256 chunks;
  const auto* bytes = static_cast<const std::uint8_t*>(data);
  std::size_t pos = 0;
  while (pos < size) {
    const std::size_t requested = (pos % 71) + 1;
    const std::size_t amount = requested < size - pos ? requested : size - pos;
    chunks.update(bytes + pos, amount);
    pos += amount;
  }
  require(hex(chunks.finish()) == expected, "incremental message");
}
}
int main() {
  check(nullptr, 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  check("abc", 3, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  const std::string million(1000000, 'a');
  check(million.data(), million.size(),
        "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
  constexpr std::size_t sizes[] = {55, 56, 63, 64, 65, 127, 128, 129};
  constexpr const char* expected[] = {
      "90049a7259f123e69f1cea2403d496ffa7b647cd45ea9f4487dd778f9578b246",
      "40f60a3a579d89962bdffdbc476f9bd0d9a28231f4b65711c902127841d298e2",
      "bfb702913b0eabf76eb88475599bd4932114c08fce4e205593d6b7a34eb85a2e",
      "e7a402f6bfefa6ff48bc1fe1bb5b77e5c9c24ca9185937a8cf07a8fdaf2424f0",
      "d1c3eeb9bcf3d1593eaddd2664f0adb6f4a9c0dc048012780e4b098d4c0aaca8",
      "4c3c60813a50827cc68057ec8ae17d9806f6620a6c98d9be77d26b95b6972ffa",
      "a1e19a9775b673aaeb645655cad464ac5cfed26e721f9eb068194c70af4119e9",
      "5068a447ab2b324e59491e55149a9f597ca71fb2ead28bbb533d9549d388e35d"};
  for (std::size_t index = 0; index < 8; ++index) {
    std::vector<std::uint8_t> bytes(sizes[index]);
    for (std::size_t i = 0; i < bytes.size(); ++i)
      bytes[i] = static_cast<std::uint8_t>((i * 29 + 7) % 256);
    check(bytes.data(), bytes.size(), expected[index]);
  }
  ubc::Sha256 continued;
  continued.update("a", 1);
  (void)continued.finish();
  continued.update("bc", 2);
  require(hex(continued.finish()) ==
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "continue after finish");
  std::printf("SHA256: %u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
