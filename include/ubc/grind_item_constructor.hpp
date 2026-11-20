// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/grind_item_shape.hpp"

// Our metadata from 45 fully hashed straight-line constructors. All
// visual assignments are verified .3DS values. No original strings.
namespace ubc::grind_item_constructor {
inline constexpr std::array<lhiver::SymbolBinding,16> fields{{
  {"C_ITEM.NAME",1586,3,4,1,1584,false,0,true,292},
  {"C_ITEM.MAINFLAG",1590,2,4,1,1584,false,0,true,340},
  {"C_ITEM.FLAGS",1591,2,4,1,1584,false,0,true,344},
  {"C_ITEM.VALUE",1593,2,4,1,1584,false,0,true,352},
  {"C_ITEM.DAMAGETYPE",1594,2,4,1,1584,false,0,true,356},
  {"C_ITEM.DAMAGETOTAL",1595,2,4,1,1584,false,0,true,360},
  {"C_ITEM.COND_ATR",1600,2,4,3,1584,false,0,true,436},
  {"C_ITEM.COND_VALUE",1601,2,4,3,1584,false,0,true,448},
  {"C_ITEM.ON_EQUIP",1605,5,4,1,1584,false,0,true,488},
  {"C_ITEM.ON_UNEQUIP",1606,5,4,1,1584,false,0,true,492},
  {"C_ITEM.VISUAL",1611,3,4,1,1584,false,0,true,524},
  {"C_ITEM.MATERIAL",1616,2,4,1,1584,false,0,true,608},
  {"C_ITEM.RANGE",1619,2,4,1,1584,false,0,true,620},
  {"C_ITEM.DESCRIPTION",1621,3,4,1,1584,false,0,true,628},
  {"C_ITEM.TEXT",1622,3,4,6,1584,false,0,true,648},
  {"C_ITEM.COUNT",1623,2,4,6,1584,false,0,true,768},
}};
struct Visual {std::uint32_t item,symbol,length;lhiver::Digest digest,constructor;};
inline constexpr std::array<Visual,45> visuals{{
  {16450,46245,28,lhiver::digest_literal("092e46927396d6806c27920bc4ab9b67db5ef5830c3f08d62a1c1114aeeb9156"),lhiver::digest_literal("2c04956b42bcc6e4af1b742d5aa3893d45ffb9d4d4233c7e2b44e7f78a39e52a")},
  {16451,46247,28,lhiver::digest_literal("092e46927396d6806c27920bc4ab9b67db5ef5830c3f08d62a1c1114aeeb9156"),lhiver::digest_literal("c364c21d1a24e4ca6185011b538b0fc139825dde27938b9bd2bae3e7d61a189a")},
  {16453,46251,28,lhiver::digest_literal("a300806dacdd50ccbd8621b6adc8a49c7084a16fa93d408592b3ac91c3d2b543"),lhiver::digest_literal("40213d87b2fd254cdcaad4b9fba9237a26ff53d839713d49e623489f1fd46fd5")},
  {16454,46253,28,lhiver::digest_literal("a300806dacdd50ccbd8621b6adc8a49c7084a16fa93d408592b3ac91c3d2b543"),lhiver::digest_literal("632106ef978da7d2942df64b92dec32ef2b0576aba1a1445ec8b6937c9bcc50a")},
  {16455,46255,28,lhiver::digest_literal("4e6eabc1145cd54558431b7972bc64dcafb1b37def9f88ff227c1e29e42d7401"),lhiver::digest_literal("1973f5c567d902d05e80179dba5b638b58baecbcd0af7f69ac5c12b5175bad86")},
  {16456,46257,28,lhiver::digest_literal("4e6eabc1145cd54558431b7972bc64dcafb1b37def9f88ff227c1e29e42d7401"),lhiver::digest_literal("d038c7ada2acd69293053426049331127eb1eab0df82cace1e685fff38d16647")},
  {16457,46259,28,lhiver::digest_literal("4d0c833930a89d7225c3898e9098024ddf9d545ab5d2cc19aa639f951e91626c"),lhiver::digest_literal("b90651d848c680d4b62437d0514c0b8c91084ba72bab0f0a8b6c4cf0a090b6e9")},
  {16458,46261,28,lhiver::digest_literal("4d0c833930a89d7225c3898e9098024ddf9d545ab5d2cc19aa639f951e91626c"),lhiver::digest_literal("1301e6af88233de9f10f3da560f8d96b0a9dd81265b4b4f09c376cb65cd463b3")},
  {16483,46310,24,lhiver::digest_literal("120a9cdc1580727d88862cc059aaa6f809c14d48f799238159243601e7661135"),lhiver::digest_literal("143288508b8cde18573fd4c0154fee988224c47ee53f3a28c64d6a668c09cfaf")},
  {16484,46312,24,lhiver::digest_literal("120a9cdc1580727d88862cc059aaa6f809c14d48f799238159243601e7661135"),lhiver::digest_literal("2a67fb0c5bdf788d3a8c7ebcc1bfbc41582ef67f8f1c602e3d9f6530c4fbb31d")},
  {16489,46322,32,lhiver::digest_literal("061af86fb8e699fa0a49c10643701d1d0da5aac4fb4dcd45d1063a6ddf228044"),lhiver::digest_literal("242ba5955d46b5a42cb6bd44e729b1ad472c1033827171c71239fa600e8c1baa")},
  {16490,46324,32,lhiver::digest_literal("061af86fb8e699fa0a49c10643701d1d0da5aac4fb4dcd45d1063a6ddf228044"),lhiver::digest_literal("14a59d3fabe9e4227d1b008dbd488f6145f228f4076708756067fba8f0896354")},
  {16492,46327,30,lhiver::digest_literal("63d93c4367e838a662ab6ee2be54af34b8351da894d96418bbb7b128346b98b8"),lhiver::digest_literal("98410b523286be0e3628c4fb19d5d011379ed9c37b2b816c567ab10949131e4c")},
  {16493,46329,30,lhiver::digest_literal("63d93c4367e838a662ab6ee2be54af34b8351da894d96418bbb7b128346b98b8"),lhiver::digest_literal("79e00dd603473c78a6400c21a895207d5dc1a0b23ebd496a49949d67d97f842e")},
  {16494,46330,30,lhiver::digest_literal("d0fe0bcb5754d7324fc6da5b31b991e814db570471b7067c9b67bc4f0f01fbc1"),lhiver::digest_literal("f0621aff3cbe0cc71385d5ebd6ab7a487214a858b8aa899bf642cd1c2c249b54")},
  {16495,46332,30,lhiver::digest_literal("d0fe0bcb5754d7324fc6da5b31b991e814db570471b7067c9b67bc4f0f01fbc1"),lhiver::digest_literal("a14295619048de72e9005434766c40fbe6287c22020cf50bac69f26935ef6b1c")},
  {16497,46336,24,lhiver::digest_literal("a7cba29dd7e8e75d67a2f376517d5b1cc29a4e82ee79598da4ce86daccd9019e"),lhiver::digest_literal("c69c842dbe3cd3f0c75478a7475ef8f5f0d645a17d20c5dca1f8fe1eeb5909b6")},
  {16498,46338,24,lhiver::digest_literal("a7cba29dd7e8e75d67a2f376517d5b1cc29a4e82ee79598da4ce86daccd9019e"),lhiver::digest_literal("b20d8871db44d95ef94e86bba2b1b8564bbfe382918de9550de1d9d3c79a7052")},
  {16500,46342,30,lhiver::digest_literal("8d1613a24db59781e1678597561d9cce555cf6040a623a2c330a9f5e0664882a"),lhiver::digest_literal("2f56c8246439b9fccebf8347842d5f1052fb3d8fd2b3ab3d47b79f1f82d97852")},
  {16501,46344,30,lhiver::digest_literal("8d1613a24db59781e1678597561d9cce555cf6040a623a2c330a9f5e0664882a"),lhiver::digest_literal("d8de9d7596db14ba64007f83339103025c231ade729aed732402a5dbd6ceed79")},
  {16508,46357,30,lhiver::digest_literal("a33bf3a616c66c2be5b960f94775ae1d7335b93ddffd1eaf3e9fde35132bd224"),lhiver::digest_literal("6026c6e8f0e9355b555cd38d6e69dacfad9c4be2cc9002f681e0a981a5ae3f6f")},
  {16509,46359,30,lhiver::digest_literal("a33bf3a616c66c2be5b960f94775ae1d7335b93ddffd1eaf3e9fde35132bd224"),lhiver::digest_literal("dd15f6ca16e097062cb747f31ff8bb324729a979957fd01fa7c714a658e78488")},
  {16510,46360,30,lhiver::digest_literal("0ff043f7bb746efd49d4c944fa31d7a228ac34ac326522c941fac405577e5ff0"),lhiver::digest_literal("ca19b62d344e550cb745162cd5d3369e0704b8a3a11b5db7085363f5d383f1f8")},
  {16511,46362,30,lhiver::digest_literal("0ff043f7bb746efd49d4c944fa31d7a228ac34ac326522c941fac405577e5ff0"),lhiver::digest_literal("cc7e9c55f5ee1f9be4057e275a41999643d520e6ab568cdb508832aa5d980a89")},
  {16522,46383,30,lhiver::digest_literal("92ca340e8a844853e72e08bd8d8a58cdca4f2448201382a8d6b1221ff52162c8"),lhiver::digest_literal("fc0b8b4034a209119e4e45b89643cb866258634a9a4cd5c8d2facdd09550312c")},
  {16523,46385,30,lhiver::digest_literal("92ca340e8a844853e72e08bd8d8a58cdca4f2448201382a8d6b1221ff52162c8"),lhiver::digest_literal("95a252f7797fd5fa6406cb8c2bc816798ec679999489b962210c17a422b8c912")},
  {16524,46386,30,lhiver::digest_literal("530d3d455b7b85a39b9ae17199066b8b5351ca42ae554d5447a44ea45e42d3f4"),lhiver::digest_literal("b41f8eec3dd88aed1036ed2af3a26e2c437f2aca66524909be55ca53a401eb9c")},
  {16525,46388,30,lhiver::digest_literal("530d3d455b7b85a39b9ae17199066b8b5351ca42ae554d5447a44ea45e42d3f4"),lhiver::digest_literal("353409b83e2d7c9094dac1f60d263f9f64c6319268f9ad0dfa7aafb3a9d9f65c")},
  {16528,46393,30,lhiver::digest_literal("a57226d305670692652966aa3db1fedf040d8c9cb872fe5c8816c9ba9c88b128"),lhiver::digest_literal("6811333493e039e7d32dfd300f99f4f6c593dbd92017fd024c8e6e3ee2c40963")},
  {16529,46395,30,lhiver::digest_literal("a57226d305670692652966aa3db1fedf040d8c9cb872fe5c8816c9ba9c88b128"),lhiver::digest_literal("4e8e22c5b647a9775e198906df89baa3e09ebb88d7eeabea9fbdd7e10c9059f9")},
  {16530,46396,30,lhiver::digest_literal("0d31941b2a56c83c32846e7068bb97fe3c8cfd302d07bfb282ce46b32c6616df"),lhiver::digest_literal("0c39151d1336d3241754fbb89c3f6772e9bc30cc623a380078e6959d6b9d2517")},
  {16531,46398,30,lhiver::digest_literal("0d31941b2a56c83c32846e7068bb97fe3c8cfd302d07bfb282ce46b32c6616df"),lhiver::digest_literal("bb3e518b21d3d23880af627bce81feb907567add7ce8db381ae03a30889f9bf6")},
  {16532,46400,40,lhiver::digest_literal("c73d344a23d4aba207c915bacfbb317907ada2dfae623cc33dd313a6b22baa32"),lhiver::digest_literal("821d807f4e72c5973d606da4a45fd794e92df33e351861953bfd683667d20a98")},
  {16533,46402,40,lhiver::digest_literal("c73d344a23d4aba207c915bacfbb317907ada2dfae623cc33dd313a6b22baa32"),lhiver::digest_literal("239739ef5ec5d91f10137e732cdf862eca6700613104d13e006d392535109de8")},
  {16534,46404,40,lhiver::digest_literal("dec224ab911f6acc43d59a9bfc5ddbf6b3a25b54e9508a47a2edd6df986317a6"),lhiver::digest_literal("1ab48f5cfe9e90a8430bcb1e8fb10e470ac708153a70a7f13104afd0ffc3adb2")},
  {16535,46406,40,lhiver::digest_literal("dec224ab911f6acc43d59a9bfc5ddbf6b3a25b54e9508a47a2edd6df986317a6"),lhiver::digest_literal("ef82c453734e84dd1d5ced020ebf039916038f6241c7ff2beed3ed06584ec72d")},
  {16536,46408,40,lhiver::digest_literal("dec224ab911f6acc43d59a9bfc5ddbf6b3a25b54e9508a47a2edd6df986317a6"),lhiver::digest_literal("14ae1d03d41ca272236e08c6a1b2ee8d5e8b8c74cb2c946d748e5768c3383f78")},
  {16537,46410,40,lhiver::digest_literal("dec224ab911f6acc43d59a9bfc5ddbf6b3a25b54e9508a47a2edd6df986317a6"),lhiver::digest_literal("b6dcde1d33f9ecb9d42fbfbd319d2a10b02a3a8f81cbe981b7ebbec09d728e27")},
  {16538,46412,38,lhiver::digest_literal("6c2c53c27ad8b0e9f3e3ebf44dfeff0d7e61cdbde6fd59dd03a3f8717977f1e9"),lhiver::digest_literal("88d69f3f6be6cfe5ad9f9e64a908692f30a2ace2161963e9a032d9f6aea9fa2f")},
  {16539,46414,38,lhiver::digest_literal("6c2c53c27ad8b0e9f3e3ebf44dfeff0d7e61cdbde6fd59dd03a3f8717977f1e9"),lhiver::digest_literal("33309d70b92148b95f4e1e0686a4e49d6edb5ff23e277a78e03230c42f5e743f")},
  {16540,46416,38,lhiver::digest_literal("6c2c53c27ad8b0e9f3e3ebf44dfeff0d7e61cdbde6fd59dd03a3f8717977f1e9"),lhiver::digest_literal("8797db057207fdf3a59ec6090041111a9ad6ca53a16b2d7ada4a10bb81c2d7d7")},
  {16541,46418,38,lhiver::digest_literal("6c2c53c27ad8b0e9f3e3ebf44dfeff0d7e61cdbde6fd59dd03a3f8717977f1e9"),lhiver::digest_literal("26628b8716748d1fd78a952a7273cca3c46f5797bcd772713de23e54746f462d")},
  {16542,46420,38,lhiver::digest_literal("6c2c53c27ad8b0e9f3e3ebf44dfeff0d7e61cdbde6fd59dd03a3f8717977f1e9"),lhiver::digest_literal("9acf5b00e517193554d456d2308140f259a99050e32ac8fceec8ad5b43c39ea3")},
  {16543,46422,38,lhiver::digest_literal("6c2c53c27ad8b0e9f3e3ebf44dfeff0d7e61cdbde6fd59dd03a3f8717977f1e9"),lhiver::digest_literal("c01baeefabeab9433c362c4a8d12359f9d891b3ffdafa1bc0f69730a24a6b26a")},
  {16613,46466,18,lhiver::digest_literal("70680683e29ed5a528ab46ce99a8aa73c7cd2289a81627025a97f93902c27bc4"),lhiver::digest_literal("42533a3cfb2ce9b0b046ad0ed324dae34d0d190bad788369329b8a8dcda3a02e")},
}};
inline const Visual* visual(std::uint32_t symbol) noexcept {
  for(const auto& v:visuals)if(v.item==symbol)return &v;return nullptr;
}
} // namespace ubc::grind_item_constructor
