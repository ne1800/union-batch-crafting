// SPDX-License-Identifier: MIT
// Table generated in the archived extraction workspace: python3 derive-grind-shapes-WRITES.py --apply
#pragma once

#include "ubc/catalogs/grind_recipe_profile.hpp"

// Our expected values from 45 fully hashed straight-line constructors.
// DAT: 71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4.
// Nativeflags = Scriptflags OR Mainflag (InitByScript 0x711CA0..0x711CBC,
// EXE 0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f).
// This describes the expected form. It does not establish current Create, Remove or end events.
namespace ubc::grind_item_shape {
struct Shape {
  std::uint32_t symbol;
  lhiver::Digest constructor_sha256;
  std::uint32_t mainflag, script_flags, native_flags;
};
inline constexpr std::array<Shape,45> items{{
  {16450,lhiver::digest_literal("2c04956b42bcc6e4af1b742d5aa3893d45ffb9d4d4233c7e2b44e7f78a39e52a"),0x2u,0x8000u,0x8002u},
  {16451,lhiver::digest_literal("c364c21d1a24e4ca6185011b538b0fc139825dde27938b9bd2bae3e7d61a189a"),0x2u,0x8000u,0x8002u},
  {16453,lhiver::digest_literal("40213d87b2fd254cdcaad4b9fba9237a26ff53d839713d49e623489f1fd46fd5"),0x2u,0x10000u,0x10002u},
  {16454,lhiver::digest_literal("632106ef978da7d2942df64b92dec32ef2b0576aba1a1445ec8b6937c9bcc50a"),0x2u,0x10000u,0x10002u},
  {16455,lhiver::digest_literal("1973f5c567d902d05e80179dba5b638b58baecbcd0af7f69ac5c12b5175bad86"),0x2u,0x4000u,0x4002u},
  {16456,lhiver::digest_literal("d038c7ada2acd69293053426049331127eb1eab0df82cace1e685fff38d16647"),0x2u,0x4000u,0x4002u},
  {16457,lhiver::digest_literal("b90651d848c680d4b62437d0514c0b8c91084ba72bab0f0a8b6c4cf0a090b6e9"),0x2u,0x10000u,0x10002u},
  {16458,lhiver::digest_literal("1301e6af88233de9f10f3da560f8d96b0a9dd81265b4b4f09c376cb65cd463b3"),0x2u,0x10000u,0x10002u},
  {16483,lhiver::digest_literal("143288508b8cde18573fd4c0154fee988224c47ee53f3a28c64d6a668c09cfaf"),0x2u,0x4000u,0x4002u},
  {16484,lhiver::digest_literal("2a67fb0c5bdf788d3a8c7ebcc1bfbc41582ef67f8f1c602e3d9f6530c4fbb31d"),0x2u,0x4000u,0x4002u},
  {16489,lhiver::digest_literal("242ba5955d46b5a42cb6bd44e729b1ad472c1033827171c71239fa600e8c1baa"),0x2u,0x4000u,0x4002u},
  {16490,lhiver::digest_literal("14a59d3fabe9e4227d1b008dbd488f6145f228f4076708756067fba8f0896354"),0x2u,0x4000u,0x4002u},
  {16492,lhiver::digest_literal("98410b523286be0e3628c4fb19d5d011379ed9c37b2b816c567ab10949131e4c"),0x2u,0x4000u,0x4002u},
  {16493,lhiver::digest_literal("79e00dd603473c78a6400c21a895207d5dc1a0b23ebd496a49949d67d97f842e"),0x2u,0x4000u,0x4002u},
  {16494,lhiver::digest_literal("f0621aff3cbe0cc71385d5ebd6ab7a487214a858b8aa899bf642cd1c2c249b54"),0x2u,0x10000u,0x10002u},
  {16495,lhiver::digest_literal("a14295619048de72e9005434766c40fbe6287c22020cf50bac69f26935ef6b1c"),0x2u,0x10000u,0x10002u},
  {16497,lhiver::digest_literal("c69c842dbe3cd3f0c75478a7475ef8f5f0d645a17d20c5dca1f8fe1eeb5909b6"),0x2u,0x4000u,0x4002u},
  {16498,lhiver::digest_literal("b20d8871db44d95ef94e86bba2b1b8564bbfe382918de9550de1d9d3c79a7052"),0x2u,0x4000u,0x4002u},
  {16500,lhiver::digest_literal("2f56c8246439b9fccebf8347842d5f1052fb3d8fd2b3ab3d47b79f1f82d97852"),0x2u,0x10000u,0x10002u},
  {16501,lhiver::digest_literal("d8de9d7596db14ba64007f83339103025c231ade729aed732402a5dbd6ceed79"),0x2u,0x10000u,0x10002u},
  {16508,lhiver::digest_literal("6026c6e8f0e9355b555cd38d6e69dacfad9c4be2cc9002f681e0a981a5ae3f6f"),0x2u,0x4000u,0x4002u},
  {16509,lhiver::digest_literal("dd15f6ca16e097062cb747f31ff8bb324729a979957fd01fa7c714a658e78488"),0x2u,0x4000u,0x4002u},
  {16510,lhiver::digest_literal("ca19b62d344e550cb745162cd5d3369e0704b8a3a11b5db7085363f5d383f1f8"),0x2u,0x10000u,0x10002u},
  {16511,lhiver::digest_literal("cc7e9c55f5ee1f9be4057e275a41999643d520e6ab568cdb508832aa5d980a89"),0x2u,0x10000u,0x10002u},
  {16522,lhiver::digest_literal("fc0b8b4034a209119e4e45b89643cb866258634a9a4cd5c8d2facdd09550312c"),0x2u,0x4000u,0x4002u},
  {16523,lhiver::digest_literal("95a252f7797fd5fa6406cb8c2bc816798ec679999489b962210c17a422b8c912"),0x2u,0x4000u,0x4002u},
  {16524,lhiver::digest_literal("b41f8eec3dd88aed1036ed2af3a26e2c437f2aca66524909be55ca53a401eb9c"),0x2u,0x10000u,0x10002u},
  {16525,lhiver::digest_literal("353409b83e2d7c9094dac1f60d263f9f64c6319268f9ad0dfa7aafb3a9d9f65c"),0x2u,0x10000u,0x10002u},
  {16528,lhiver::digest_literal("6811333493e039e7d32dfd300f99f4f6c593dbd92017fd024c8e6e3ee2c40963"),0x2u,0x4000u,0x4002u},
  {16529,lhiver::digest_literal("4e8e22c5b647a9775e198906df89baa3e09ebb88d7eeabea9fbdd7e10c9059f9"),0x2u,0x4000u,0x4002u},
  {16530,lhiver::digest_literal("0c39151d1336d3241754fbb89c3f6772e9bc30cc623a380078e6959d6b9d2517"),0x2u,0x10000u,0x10002u},
  {16531,lhiver::digest_literal("bb3e518b21d3d23880af627bce81feb907567add7ce8db381ae03a30889f9bf6"),0x2u,0x10000u,0x10002u},
  {16532,lhiver::digest_literal("821d807f4e72c5973d606da4a45fd794e92df33e351861953bfd683667d20a98"),0x2u,0x5000u,0x5002u},
  {16533,lhiver::digest_literal("239739ef5ec5d91f10137e732cdf862eca6700613104d13e006d392535109de8"),0x2u,0x5000u,0x5002u},
  {16534,lhiver::digest_literal("1ab48f5cfe9e90a8430bcb1e8fb10e470ac708153a70a7f13104afd0ffc3adb2"),0x2u,0x5000u,0x5002u},
  {16535,lhiver::digest_literal("ef82c453734e84dd1d5ced020ebf039916038f6241c7ff2beed3ed06584ec72d"),0x2u,0x5000u,0x5002u},
  {16536,lhiver::digest_literal("14ae1d03d41ca272236e08c6a1b2ee8d5e8b8c74cb2c946d748e5768c3383f78"),0x2u,0x5000u,0x5002u},
  {16537,lhiver::digest_literal("b6dcde1d33f9ecb9d42fbfbd319d2a10b02a3a8f81cbe981b7ebbec09d728e27"),0x2u,0x5000u,0x5002u},
  {16538,lhiver::digest_literal("88d69f3f6be6cfe5ad9f9e64a908692f30a2ace2161963e9a032d9f6aea9fa2f"),0x2u,0x10000u,0x10002u},
  {16539,lhiver::digest_literal("33309d70b92148b95f4e1e0686a4e49d6edb5ff23e277a78e03230c42f5e743f"),0x2u,0x10000u,0x10002u},
  {16540,lhiver::digest_literal("8797db057207fdf3a59ec6090041111a9ad6ca53a16b2d7ada4a10bb81c2d7d7"),0x2u,0x10000u,0x10002u},
  {16541,lhiver::digest_literal("26628b8716748d1fd78a952a7273cca3c46f5797bcd772713de23e54746f462d"),0x2u,0x10000u,0x10002u},
  {16542,lhiver::digest_literal("9acf5b00e517193554d456d2308140f259a99050e32ac8fceec8ad5b43c39ea3"),0x2u,0x10000u,0x10002u},
  {16543,lhiver::digest_literal("c01baeefabeab9433c362c4a8d12359f9d891b3ffdafa1bc0f69730a24a6b26a"),0x2u,0x10000u,0x10002u},
  {16613,lhiver::digest_literal("42533a3cfb2ce9b0b046ad0ed324dae34d0d190bad788369329b8a8dcda3a02e"),0x1u,0x200000u,0x200001u},
}};
inline const Shape* find(std::uint32_t symbol) noexcept {
  for (const auto& item : items) if (item.symbol == symbol) return &item;
  return nullptr;
}
// Do not pass on old form references when profiles are changed later.
inline bool bound_to_profile(const Shape& shape) noexcept {
  const auto* canonical = find(shape.symbol);
  if (!canonical || canonical != &shape) return false;
  const auto* profile = grind_recipe_profile::item_for(shape.symbol);
  return profile && profile->function && profile->function->body_sha256 == shape.constructor_sha256 &&
    (shape.script_flags | shape.mainflag) == shape.native_flags;
}
} // namespace ubc::grind_item_shape
