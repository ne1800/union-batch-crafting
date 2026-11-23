#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if (defined(__i386__) || defined(__x86_64__)) && (defined(__clang__) || defined(__GNUC__))
#include <immintrin.h>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

namespace ubc {
// SHA-256 for comparing a complete engine file. No crypto provider, no
// allocation and no hidden DLL initialization in the loader context.
class Sha256 {
public:
  void update(const void* data, std::size_t size) noexcept {
    if (size == 0)
      return;
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    length_ += static_cast<std::uint64_t>(size);
    if (size < block_.size() - used_) {
      std::memcpy(block_.data() + used_, bytes, size);
      used_ += size;
      return;
    }
    while (size != 0) {
      const auto space = block_.size() - used_;
      const auto amount = size < space ? size : space;
      std::memcpy(block_.data() + used_, bytes, amount);
      used_ += amount;
      bytes += amount;
      size -= amount;
      if (used_ == block_.size()) {
        compress();
        used_ = 0;
      }
    }
  }

  std::array<std::uint8_t, 32> finish() const noexcept {
    Sha256 copy = *this;
    const std::uint64_t bits = copy.length_ * 8;
    copy.block_[copy.used_++] = 0x80;
    if (copy.used_ > 56) {
      while (copy.used_ < 64)
        copy.block_[copy.used_++] = 0;
      copy.compress();
      copy.used_ = 0;
    }
    while (copy.used_ < 56)
      copy.block_[copy.used_++] = 0;
    for (unsigned i = 0; i < 8; ++i)
      copy.block_[56 + i] = static_cast<std::uint8_t>(bits >> (56 - 8 * i));
    copy.compress();
    std::array<std::uint8_t, 32> result{};
    for (std::size_t i = 0; i < 8; ++i) {
      for (unsigned j = 0; j < 4; ++j)
        result[4 * i + j] = static_cast<std::uint8_t>(copy.state_[i] >> (24 - 8 * j));
    }
    return result;
  }

private:
  static std::uint32_t rotr(std::uint32_t value, unsigned amount) noexcept {
    return (value >> amount) | (value << (32 - amount));
  }
  inline static constexpr std::uint32_t k[64] = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
      0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
      0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
      0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
      0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
      0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
      0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
      0xc67178f2};

#if (defined(__i386__) || defined(__x86_64__)) && (defined(__clang__) || defined(__GNUC__))
  static bool has_sha_instructions() noexcept {
#ifdef _MSC_VER
    int info[4]{};
    __cpuid(info, 0);
    if (info[0] < 7)
      return false;
    __cpuid(info, 1);
    const auto simd = info[2];
    __cpuidex(info, 7, 0);
    const auto extended = info[1];
#else
    unsigned a, b, simd, d, extended;
    if (!__get_cpuid(1, &a, &b, &simd, &d) || !__get_cpuid_count(7, 0, &a, &extended, &b, &d))
      return false;
#endif
    return (simd & (1u << 9)) && (simd & (1u << 19)) && (extended & (1u << 29));
  }

  __attribute__((target("sha,ssse3,sse4.1"))) void compress_x86() noexcept {
    // SHA256RNDS2 uses ABEF and CDGH in separate vectors.
    const auto abcd =
        _mm_shuffle_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(state_.data())), 0xb1);
    const auto efgh = _mm_shuffle_epi32(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(state_.data() + 4)), 0x1b);
    auto abef = _mm_alignr_epi8(abcd, efgh, 8);
    auto cdgh = _mm_blend_epi16(efgh, abcd, 0xf0);
    const auto initial_abef = abef, initial_cdgh = cdgh;
    const auto reverse_word = _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
    __m128i words[4];
    for (unsigned group = 0; group < 16; ++group) {
      const auto j = group % 4;
      if (group < 4) {
        words[j] = _mm_shuffle_epi8(
            _mm_loadu_si128(reinterpret_cast<const __m128i*>(block_.data() + 16 * j)),
            reverse_word);
      } else {
        const auto last = words[(j + 3) % 4];
        const auto shifted = _mm_alignr_epi8(last, words[(j + 2) % 4], 4);
        words[j] = _mm_sha256msg2_epu32(
            _mm_add_epi32(_mm_sha256msg1_epu32(words[j], words[(j + 1) % 4]), shifted), last);
      }
      const auto rounds =
          _mm_add_epi32(words[j], _mm_loadu_si128(reinterpret_cast<const __m128i*>(k + 4 * group)));
      cdgh = _mm_sha256rnds2_epu32(cdgh, abef, rounds);
      abef = _mm_sha256rnds2_epu32(abef, cdgh, _mm_shuffle_epi32(rounds, 0x0e));
    }
    abef = _mm_shuffle_epi32(_mm_add_epi32(abef, initial_abef), 0x1b);
    cdgh = _mm_shuffle_epi32(_mm_add_epi32(cdgh, initial_cdgh), 0xb1);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(state_.data()), _mm_blend_epi16(abef, cdgh, 0xf0));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(state_.data() + 4), _mm_alignr_epi8(cdgh, abef, 8));
  }
#endif

  void compress() noexcept {
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__clang__) || defined(__GNUC__))
    static const bool accelerated = has_sha_instructions();
    if (accelerated) {
      compress_x86();
      return;
    }
#endif
    std::uint32_t w[64]{};
    for (std::size_t i = 0; i < 16; ++i) {
      w[i] = (static_cast<std::uint32_t>(block_[4 * i]) << 24) |
             (static_cast<std::uint32_t>(block_[4 * i + 1]) << 16) |
             (static_cast<std::uint32_t>(block_[4 * i + 2]) << 8) |
             static_cast<std::uint32_t>(block_[4 * i + 3]);
    }
    for (std::size_t i = 16; i < 64; ++i) {
      const auto s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
      const auto s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    auto a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    auto e = state_[4], f = state_[5], g = state_[6], h = state_[7];
    for (std::size_t i = 0; i < 64; ++i) {
      const auto s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      const auto choice = (e & f) ^ (~e & g);
      const auto t1 = h + s1 + choice + k[i] + w[i];
      const auto s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      const auto majority = (a & b) ^ (a & c) ^ (b & c);
      const auto t2 = s0 + majority;
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
  }
  std::array<std::uint32_t, 8> state_ = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                         0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
  std::array<std::uint8_t, 64> block_{};
  std::uint64_t length_ = 0;
  std::size_t used_ = 0;
};
} // namespace ubc
