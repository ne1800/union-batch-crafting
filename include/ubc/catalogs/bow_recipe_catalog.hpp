// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/anvil_recipe_catalog.hpp"

// Our canonical bow and crossbow profiles. No original script bytes.
// DAT SHA256 71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4.
namespace ubc::bow_recipe_catalog {
using anvil_recipe_catalog::Effect;
using anvil_recipe_catalog::EffectKind;
using anvil_recipe_catalog::PrintFingerprint;
using lhiver::digest_literal;
using lhiver::Function;
using lhiver::SymbolBinding;
using recipe_catalog::Actor;
using recipe_catalog::Callback;
using recipe_catalog::Invocation;
using recipe_catalog::ItemAmount;
using recipe_catalog::MaterialAction;
using recipe_catalog::MaterialEffect;
using recipe_catalog::Owner;
using recipe_catalog::Requirement;
using recipe_catalog::RequirementKind;
using recipe_catalog::RequirementRole;
using recipe_catalog::View;
using recipe_catalog::view;
using station_recipe_catalog::AmountRole;
using station_recipe_catalog::CheckSite;
using station_recipe_catalog::OperandUse;
using station_recipe_catalog::RequirementCheck;
enum class RecipeId : std::uint32_t {
  bow_01 = 0x8001,
  bow_02,
  bow_03,
  bow_04,
  bow_05,
  bow_06,
  bow_07,
  bow_08,
  bow_09,
  bow_10,
  bow_11,
  bow_12,
  bow_13,
  bow_14,
  bow_15,
  bow_16,
  bow_17,
  bow_18,
  bow_19,
  bow_20
};
inline constexpr bool runtime_ready = false;
struct HandContract {
  bool left_empty, right_empty, anchor_zero, static_animation_no_items, observed_in_world;
};
struct ItemShape {
  std::int32_t symbol;
  const Function* constructor;
  std::uint32_t mainflag, script_flags, native_flags;
  bool constructor_calls;
};
struct StationProfile {
  std::int32_t production_context;
  const char *on_state, *scheme, *visual, *use_with_item, *condition_name;
  std::int32_t state_num, rewind;
  const Function* entry;
  Owner end, category;
};
struct Descriptor {
  RecipeId id;
  const char* name;
  View<PackedInventoryTarget> targets;
  View<SymbolBinding> bindings;
  View<Requirement> requirements;
  View<RequirementCheck> checks;
  Owner owner;
  Callback callback;
  View<ItemAmount> minima, costs, outputs;
  HandContract hand;
  const StationProfile* station_profile;
  View<OperandUse> operand_uses;
  bool require_self_is_hero, quantity_text_reviewed;
  View<Effect> success, failure;
  View<PrintFingerprint> prints;
  View<ItemShape> items;
};
inline constexpr std::size_t catalog_target_capacity = 4;
inline constexpr HandContract hand{true, true, true, true, false};
// The six BOWSDW animation bodies contain no item events; both ZEN
// stations have empty useWithItem/conditionFunc. This replaces
// no currently bound hand/end path and no observed crafting.
inline constexpr auto common_bindings = station_recipe_catalog::common_bindings;
using anvil_recipe_catalog::with_common;
inline constexpr std::array<Function, 111> functions{{
    {"B_SAY", 14646, 218446, 39,
     digest_literal("b8b93bf2920074b9958b398d5d642935220402bd20ff5044c51f9a416675d6ad"), 3, 5, 1, 0,
     -1},
    {"ITMI_LEATHER", 16599, 364413, 102,
     digest_literal("b814406a4cfb88fb1ea760f840aaae3bc253ec6b36f3021026e8dda520a97843"), 0, 7, 1, 0,
     1584},
    {"ITMISWORDRAW", 16610, 365599, 102,
     digest_literal("2e4a6bcaef760946ad1d3efd1985575de17bc435235c574b24b6977959888769"), 0, 7, 1, 0,
     1584},
    {"ITRW_MIL_CROSSBOW", 16806, 380678, 208,
     digest_literal("a9a7dda77bad8169867f42ef28c83afd2d3813b3e5862c1e80c19dae87da4776"), 0, 7, 1, 0,
     1584},
    {"ITRW_MIL_CROSSBOW_V1", 16807, 380886, 208,
     digest_literal("ebddebc121658771800c67d1dd153a0e85b0c05dc05ad352d0dbc5a461a1f353"), 0, 7, 1, 0,
     1584},
    {"ITRW_SLD_BOW", 16808, 381094, 208,
     digest_literal("ee42de70509374063ca77449a237192a37de4ef6b3314f21d2fb18f21a07222f"), 0, 7, 1, 0,
     1584},
    {"ITRW_SLD_BOW_V1", 16809, 381302, 208,
     digest_literal("5da8e0df9ced6ef0076a29ac9b7e7e91e00d2f44c45600930a6357ac75621f04"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_L_01", 16810, 381510, 208,
     digest_literal("c927c7c16d954f5b33514519287c6ca0b1684645e3bccf8d36bb0160ec8cc5b3"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_L_01_V1", 16811, 381718, 208,
     digest_literal("fdce30fd6a5056947ef55a0c6417b7eac86b7a9ffd0307a3355ad6aefa6ed812"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_L_02", 16812, 381926, 208,
     digest_literal("71cf2d18cfbec0936c5892cdbd9c9dcbb8d36494a200727dbaa6b1e34ffbb459"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_L_02_V1", 16813, 382134, 208,
     digest_literal("edd889b1a09afe03e0829c4c9d787826cecfee2ae5361efb1c929b92e206a03b"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_L_03", 16814, 382342, 208,
     digest_literal("c59391e94c015efdbbf06a6268546b36df30cf0cd4ad65960d0adb5987686622"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_L_03_V1", 16815, 382550, 208,
     digest_literal("c1d6684b2a13f3be7dc16b3c61001fee3e7882d6931cd7157d10db2adb10dec8"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_L_04", 16816, 382758, 208,
     digest_literal("c9bab582cc2b196d5a3d44de24a0593bb307fc7d9d36883bd81573298edfd3da"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_L_04_V1", 16817, 382966, 208,
     digest_literal("afae2987eb2f0b61934c0746ea7fd3a16f5fecf289db553e33801bf6697a2e9d"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_M_01", 16818, 383174, 208,
     digest_literal("95d31107c915c4c789592c50a451f9c7a816df6a0195803d7e0caf2326dfd62a"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_M_01_V1", 16819, 383382, 208,
     digest_literal("b29616d80fd5d845f6ff059c1fa74437e86a6ec8439c3b17c3e31eaf91c7afc3"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_M_02", 16820, 383590, 208,
     digest_literal("fb939e4deb7d64520a32a98aa5e9c4161f5fa7d71f1cad2ee1263da99d987bd8"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_M_02_V1", 16821, 383798, 208,
     digest_literal("896a331ff2c420f65886b8a597d9aaccf42a17c3a5ca1b7ef378d50cf88eebc5"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_M_03", 16822, 384006, 208,
     digest_literal("a2ab2cc98acbc262575009c2328d502f9c994a40d27e5cccd20c06c5777aefd2"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_M_03_V1", 16823, 384214, 208,
     digest_literal("d9d9fd3330d6d7b9b9bc5e3fb3975e5481285d12f6fb0177dd8f0c0ce2e83b0f"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_M_04", 16824, 384422, 208,
     digest_literal("b91226f8922a2960ca4b9cc957793878a1a10804b12ebf45bd9559bdd3ecded4"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_M_04_V1", 16825, 384630, 208,
     digest_literal("67985c03bfde8950d33aec026fb2433f54fcaa2a7744db8ba15e03a577bf8b70"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_H_01", 16826, 384838, 208,
     digest_literal("00850781991bba4b20e9a38802c3c8aee9a62800d4f76a35d487b9f7e9e2dcfc"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_H_01_V1", 16827, 385046, 208,
     digest_literal("05abd02e14ffee8c7f27f0ae76546a7eb1e61a65844c9df70615cc77db6877f9"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_H_02", 16828, 385254, 208,
     digest_literal("827c74630bb259a4ea75c79057cbd66698b95f1f8cc8d950838b13eb38c2521b"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_H_02_V1", 16829, 385462, 208,
     digest_literal("1d300a129f0e93984c0da17ee993807823cafbece6013f2c22d5e15aef02775a"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_H_03", 16830, 385670, 208,
     digest_literal("476a3e07635b12609d39fdd24526629408fe152b9dbc80eb89b5f875a63ac215"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_H_03_V1", 16831, 385878, 208,
     digest_literal("8f4b01c2b690e866caf1015d2d364526a121660437b721ed752d271bf33031c0"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_H_04", 16832, 386086, 208,
     digest_literal("4b3a2673dc1585d950bae2e55834a426b5ecccba8b9b3553ebb9832405d36b03"), 0, 7, 1, 0,
     1584},
    {"ITRW_BOW_H_04_V1", 16833, 386294, 208,
     digest_literal("aecfb7643389ee60512c54cd493b060a509923b965b9ea0e768c043a078ea978"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_L_01", 16834, 386502, 257,
     digest_literal("4a3c40c1cf152946139a5dfaafb6cd2af17a55984bb094fa6ba606339622b303"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_L_01_V1", 16835, 386759, 257,
     digest_literal("33a665eb97c8df15ab34dfa7771dcce166aed496073d64a297f8a4e596b4d3ca"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_L_02", 16836, 387016, 257,
     digest_literal("b101c48171893d4cb526534e89b195e70b77863f7ab75df08867705808e0fc48"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_L_02_V1", 16837, 387273, 257,
     digest_literal("7550ce8c5bf0253852ffa3b8b0136a352b8e4ae3fa7d7ea9ecd7e2f25f8e9ce2"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_M_01", 16838, 387530, 257,
     digest_literal("070b6402ebce2febd7707a8c731bff5736f3f91c99305c9c88fa06596650e3fc"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_M_01_V1", 16839, 387787, 257,
     digest_literal("0cd430550ae44589f693310d61729a9028439fd8f44c1dfec8482db1b59e1011"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_M_02", 16840, 388044, 257,
     digest_literal("2bd43de01bdc447fb2ac1bfd6a7a63dde83c585fa6e9320b2d91a92be7da6ae5"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_M_02_V1", 16841, 388301, 257,
     digest_literal("5974faa9ffdf96ba28d3d5cd21e8efd2620ae45f6bfd6d9156ae92d1a15d6a50"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_H_01", 16842, 388558, 257,
     digest_literal("b5e13d30d4ee49f45e6d12e64b00aa1b0f0811656877e87daf64abc3a918129b"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_H_01_V1", 16843, 388815, 257,
     digest_literal("35ecea43c4b9485e2c2445d497927be48454ece7d4c54fd08721652e32b71a7a"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_H_02", 16844, 389072, 257,
     digest_literal("54088a43c83acb5de9ae2ed939bf56995424ddaa555986d1b5406f613820573d"), 0, 7, 1, 0,
     1584},
    {"ITRW_CROSSBOW_H_02_V1", 16845, 389329, 257,
     digest_literal("496972966c69621755ff75322cf6349f96669f0065b0cbe193c8fd981b4c9ed8"), 0, 7, 1, 0,
     1584},
    {"B_ENDPRODUCTIONDIALOG", 19965, 672156, 49,
     digest_literal("e9fcd2f7db76bab4ed7c5ceb6e5a98c3ab3b8166a3614a4dda2513092da34e64"), 0, 5, 1, 0,
     -1},
    {"MAKEBOWS_S1", 20864, 803591, 65,
     digest_literal("9d49dd0d608be0046501fcfe67c67587983bfb6ee0824e005211f0047d384df8"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_END", 20865, 803656, 67,
     digest_literal("810e3c6bb6b64a3a085aaae2c22b90510d335bc4fb3acd4f04b17322ac276901"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_END_CONDITION", 20866, 803723, 23,
     digest_literal("fb23e91859508f6cd7543751c34f6f577af7e791bb7353d202af792536ce44e3"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_END_INFO", 20867, 803746, 17,
     digest_literal("395c155750a9f4d160b7d14bf7bb96448aa284ff09dd3dd5e5bab7b92877ee47"), 0, 5, 1, 0,
     -1},
    {"PC_BOWS", 20868, 803763, 56,
     digest_literal("79d736c1a8862561b10c222ea46b077149ad7a487aeb3ca0fb938cbb87778c9b"), 0, 7, 1, 0,
     1649},
    {"PC_BOWS_CONDITION", 20869, 803819, 35,
     digest_literal("dd90fc748e5e42f1b71c28e5846a9da595e54a65b24dee042020bfd3f45f68de"), 0, 5, 3, 2,
     -1},
    {"PC_BOWS_INFO", 20870, 803854, 63,
     digest_literal("18913dfc6a9a11a516afff3a1ed808342006b1e2f43ea235632d4338d8a989e4"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_1", 20871, 803917, 67,
     digest_literal("5e2c81f613bec555865e00bb984d011a17407e811817511d358fc92c85faa9b3"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_1_CONDITION", 20872, 803984, 35,
     digest_literal("e2da1abfa4c9b0faa99838913e2a98970032aa4aa83a687b093b7530eb671965"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_1_INFO", 20873, 804019, 232,
     digest_literal("7dfedea3a5fb97a33edd9a711853f178a355cd9cddda745352dc7e4e37d7b496"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_2", 20874, 804251, 67,
     digest_literal("434e7ef4779082882ad981e3b0c8ec698901373b7f7a93703255a57e57c91896"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_2_CONDITION", 20875, 804318, 35,
     digest_literal("65d65d7606f6a82f93b7deefa69b23fc58da686363f4b6e8f64ae42a4018d5a4"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_2_INFO", 20876, 804353, 232,
     digest_literal("0e6adbf706bcd4a95090dc78c0b34cb86bd09a8e946117e59c207715bedb642e"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_3", 20877, 804585, 67,
     digest_literal("cbcb2dc84f45794c992d17e9fdd1602d18be726c6df1868917cc89e466081f28"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_3_CONDITION", 20878, 804652, 35,
     digest_literal("3a0f2e0437b606887edc0fca5881729c0fb62f824d6cbe361bc5bcb0abab0e1e"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_3_INFO", 20879, 804687, 232,
     digest_literal("e4e7a213cbc15e8b1702aecb0f5574d6b284362d27feb8b224c25fd304e4c124"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_4", 20880, 804919, 67,
     digest_literal("49c380340af83bf21aabfcf0c55bd436ce0eebabddc9872b37f6fddcaba4a73c"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_4_CONDITION", 20881, 804986, 35,
     digest_literal("45d2dcd8f0a2f1a1fb975778dfc5a008d90e32be604ac0d27559ec5d57eb2de1"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_4_INFO", 20882, 805021, 232,
     digest_literal("993160c5528e064eee8398e49cf602a4cefa2025ad1d70aa24f073036d056c49"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_5", 20883, 805253, 67,
     digest_literal("826ed62435d6ec735e30d0f976da4b2c44599d51abaf929cb1d704e82d9c9788"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_5_CONDITION", 20884, 805320, 35,
     digest_literal("2c6f4bfc88e5ace1b7c88a0a9e35bddf966eae675fd896f0061512b20676c68e"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_5_INFO", 20885, 805355, 232,
     digest_literal("aa175237cf622302f21c5ada1885210b42594b72d0188c5ca2cc159de67febca"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_6", 20886, 805587, 67,
     digest_literal("363869693207010c60ec5d41a5cd8614fa3eb843f263f54229291ba26abc9439"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_6_CONDITION", 20887, 805654, 35,
     digest_literal("3fb915ce67cf31be00f0b82849248ddf02620acb29c6a738e1f6f2f63015d599"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_6_INFO", 20888, 805689, 232,
     digest_literal("0d0c1e06670ec6180dc76e4182418eb10ac2193ab9aaa690dbdf671874a65e88"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_7", 20889, 805921, 67,
     digest_literal("4fc176d160f45adfdd580154b40013255d1e7fb21074edbe8e15f67166a65297"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_7_CONDITION", 20890, 805988, 35,
     digest_literal("68dcb66909e78d2a91883289ad22ebf07862cfee7b49f91c7cb67dc8ae6879c4"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_7_INFO", 20891, 806023, 232,
     digest_literal("ce1fd7321b9ed1a3767ece7fbf7f1b17281696c8e3f546fa9a5db40977175530"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_8", 20892, 806255, 67,
     digest_literal("afb45ba1db132e74ca5143700749d122c89e916757fb96d37013f334193ff602"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_8_CONDITION", 20893, 806322, 35,
     digest_literal("4dae92026086b52f50a9a1b38ff768824c4c4a31ffdd619c120c2d2ab295debe"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_8_INFO", 20894, 806357, 232,
     digest_literal("0471633d44a088aade9742487d4b0852503592d7f04975a98574b4788dda6fda"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_9", 20895, 806589, 67,
     digest_literal("0f2ceea71692dc0566134c5f906b76c1bc072e97cfcdb94b0cf331da8f08588a"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_9_CONDITION", 20896, 806656, 35,
     digest_literal("4084702cd18af8c0195724e14d02f3f282b8bc8140b09aed2f27e5666764f131"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_9_INFO", 20897, 806691, 232,
     digest_literal("047c56765787ae7c1a71a12e284cd6c073a34d4696d9706c31a226fbee6867be"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_10", 20898, 806923, 67,
     digest_literal("6f5507af21108a282e5adfa28b8e11adea6d771bb381f7ae4d9ce59224bb7c3c"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_10_CONDITION", 20899, 806990, 35,
     digest_literal("43213b8d23e6ddb245f5bdbf090ca6850c9c835f39a6fac49f5c610e6025860f"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_10_INFO", 20900, 807025, 232,
     digest_literal("900a0747b43501d44ce18a7547a00c966f0d4a1f68f51460e12e6a73cf970a0b"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_11", 20901, 807257, 67,
     digest_literal("62969fbf646a6d6fb19d23144bd72d94112f9be133c11c9f2c4935c51e0432d0"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_11_CONDITION", 20902, 807324, 35,
     digest_literal("a847541051748fea96ae5ec3000e1d6674c3b2025d08dfbd07e909aeb840fe62"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_11_INFO", 20903, 807359, 232,
     digest_literal("b815524c2e966278dade52c665406f76a3aa19d42746ddde917dcc4ec83179bf"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_12", 20904, 807591, 67,
     digest_literal("ac3f9ea79dea8e3d5eca73b7dfc484733ab30a5dedfc751139f7aa9fafa591c4"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_12_CONDITION", 20905, 807658, 35,
     digest_literal("9909b6f37bd7326a543a240436db496ee17f28d53cc7c32f8f4e302f3fd8836d"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_12_INFO", 20906, 807693, 232,
     digest_literal("1831f9a82744095425440dfab857fa4f1354ca3181eeaed5467f6b25ae320b1e"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_13", 20907, 807925, 67,
     digest_literal("b903762659939336569327ef52840fe00946ec55b645fa85b25ec29db1861e5b"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_13_CONDITION", 20908, 807992, 35,
     digest_literal("7f9897ba30215fc4b5f43ed35a0efa345077ed7bea17a7db8cad392fd6d08d67"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_13_INFO", 20909, 808027, 232,
     digest_literal("3fb42c4d00ec77c788d1d27bee5eb2579a9ca517d2a02f87a771a5260c957a90"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_14", 20910, 808259, 67,
     digest_literal("f0551ba6de2f05179111bc3d9f204f5d26d7c78426c41b5b876889396e2dca1f"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_14_CONDITION", 20911, 808326, 35,
     digest_literal("e74eebdb243bbd1b146cff71bfeb597c642a24b2a64835ac46fde0d97080ae3e"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_14_INFO", 20912, 808361, 232,
     digest_literal("a94f1a2b5d44fb151ad424bc681a85c7f3a0fdd778f39de42eea40966287ca95"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_15", 20913, 808593, 67,
     digest_literal("d8ee196a0720cf2ce2be8a7b3b40acbf48f2e8e7abf9381f77eea312e6579a35"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_15_CONDITION", 20914, 808660, 35,
     digest_literal("0c5e348a9cb6a0807460efdf968e432690a9dfa5b11f9f5802e101df860d994b"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_15_INFO", 20915, 808695, 232,
     digest_literal("e6f70502dc71ab6de5bd252e1341e3794f528443c44674380fe529936b2e2ab9"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_16", 20916, 808927, 67,
     digest_literal("b6c25fae06d7a089da1a1d27d5655902dbdf24dcbc562347f78440ab995fa584"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_16_CONDITION", 20917, 808994, 35,
     digest_literal("81f64b4762659393c149d0bad359f48b8dc5aff87f8bc7014e17dccc0384c97a"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_16_INFO", 20918, 809029, 232,
     digest_literal("11ea47af75dac8dee36273901af961aaf019df820a9eae9b8c2da9e6918329da"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_17", 20919, 809261, 67,
     digest_literal("b40e81cdcbb40872d36858fcd7cb19c0bcc451e081df189cc75fbd8329fa03d3"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_17_CONDITION", 20920, 809328, 35,
     digest_literal("0e2022b72d08d37195035306e6ec1944ac54dc110aaf1271bfd46bab17f6b744"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_17_INFO", 20921, 809363, 232,
     digest_literal("082308489a1258dad2767a6a63a170cf4b9ee40d32ed9daffb65e0ffe16c32f0"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_18", 20922, 809595, 67,
     digest_literal("24f32e34970a0087b63f3097c0d71b9c1d42f616c3b6eda7fe3fd5a9332ed9e4"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_18_CONDITION", 20923, 809662, 35,
     digest_literal("d5055de446c0ca335ff81b088668014714963d25e166152b9be46d0219504582"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_18_INFO", 20924, 809697, 232,
     digest_literal("a4bb92a63071e652eff058e77803ac591ec00bd5c5fccab3b27782e7ac32b6a9"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_19", 20925, 809929, 67,
     digest_literal("d238292d0349f05fa91125ff3a6933518e03619e9c64eea5380f0abf89323812"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_19_CONDITION", 20926, 809996, 35,
     digest_literal("62258feefc24613e0a161003a69328765ed332bc893a0431e41a1619e30d6b78"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_19_INFO", 20927, 810031, 232,
     digest_literal("db63378901f3bd4e5053256d3555cb5f4788a23e50932e3c6121345e7104c516"), 0, 5, 1, 0,
     -1},
    {"PC_MAKEBOWS_BSPR_20", 20928, 810263, 67,
     digest_literal("c6105ff711a32f9621c3ae7bdac95432cc785c9959701de6c38ec5ba069f2a24"), 0, 7, 1, 0,
     1649},
    {"PC_MAKEBOWS_BSPR_20_CONDITION", 20929, 810330, 35,
     digest_literal("6e95da6badf996756dd884f1cb9c2d9d25a3717a8fa89c4a11bc4010f8483cf3"), 0, 5, 3, 2,
     -1},
    {"PC_MAKEBOWS_BSPR_20_INFO", 20930, 810365, 232,
     digest_literal("f8e4736bc32a952c043234cae8b9b85f5feb2dfe022dc648b1bd5c6fd8587e94"), 0, 5, 1, 0,
     -1},
}};
inline constexpr const Function* function_for(std::uint32_t symbol) noexcept {
  for (const auto& f : functions)
    if (f.symbol == symbol)
      return &f;
  return nullptr;
}
inline constexpr std::array<Requirement, 1> end_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
}};
inline constexpr std::array<Requirement, 2> category_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 0},
}};
inline constexpr Owner end_owner{20865,
                                 999,
                                 22243,
                                 false,
                                 false,
                                 true,
                                 function_for(20865),
                                 function_for(20866),
                                 function_for(20867),
                                 view(end_conditions)};
inline constexpr Owner category_owner{20868,
                                      -1,
                                      22243,
                                      false,
                                      false,
                                      true,
                                      function_for(20868),
                                      function_for(20869),
                                      function_for(20870),
                                      view(category_conditions)};
inline constexpr StationProfile station{
    16, "MAKEBOWS",          "BOWSDW",  "BOWSDW_WBENCH.MDS", "", "", 1,
    0,  function_for(20864), end_owner, category_owner};
inline constexpr std::array<PackedInventoryTarget, 4> bow_0_targets{{
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_L_01", 16810},
    {"ITRW_BOW_L_01_V1", 16811},
}};
// The anonymous success texts have different names in the loaded parser than in
// GOTHIC.DAT. The 20 names below were measured at the respective symbol index in
// WorldRun 3. Length and SHA-256 of the printed text come from the original archive.
inline constexpr std::array<SymbolBinding, 19> bow_0_own_bindings{{
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_L_01", 16810, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_L_01_V1", 16811, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_1", 20871, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23400",
     53818, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_0_bindings = with_common(bow_0_own_bindings);
inline constexpr std::array<Requirement, 2> bow_0_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_0_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16810, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_0_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20872)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20872)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 1},
     CheckSite::callback,
     function_for(20873)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 1},
     CheckSite::callback,
     function_for(20873)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16810, 0, 1},
     CheckSite::callback,
     function_for(20873)},
}};
inline constexpr Owner bow_0_owner{20871,
                                   1,
                                   22243,
                                   false,
                                   false,
                                   true,
                                   function_for(20871),
                                   function_for(20872),
                                   function_for(20873),
                                   view(bow_0_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_0_costs{{
    {16610, 1},
    {16599, 1},
    {16810, 1},
}};
inline constexpr std::array<ItemAmount, 1> bow_0_outputs{{
    {16811, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_0_minima{{
    {16610, 1},
    {16599, 1},
    {16810, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_0_material{{
    {MaterialAction::remove, 16610, 1, 0},
    {MaterialAction::remove, 16599, 1, 0},
    {MaterialAction::remove, 16810, 1, 0},
    {MaterialAction::create, 16811, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_0_operands{{
    {1, 1, 1, 0, false},
    {22, 1, 1, 0, false},
    {43, 1, 1, 0, false},
    {81, 1, 1, 0, false},
    {101, 1, 1, 0, false},
    {121, 1, 1, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_0_uses{{
    {1, AmountRole::minimum_inventory, 16810},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16610},
    {101, AmountRole::remove_count, 16599},
    {121, AmountRole::remove_count, 16810},
    {141, AmountRole::create_count, 16811},
}};
inline constexpr std::array<ItemShape, 4> bow_0_items{{
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16810, function_for(16810), 4, 524288, 524292, false},
    {16811, function_for(16811), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_0_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53818, 45, digest_literal("9cf8d8f3615a2fa26917972b1624671ede8f46d65722f9325c17eba86bf8cff1")},
}};
inline constexpr std::array<Effect, 11> bow_0_success{{
    {EffectKind::minimum, 15, 160, 16810, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 1, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 1, Actor::hero},
    {EffectKind::remove, 85, 184, 16610, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16599, 1, Actor::hero},
    {EffectKind::remove, 125, 184, 16810, 1, Actor::hero},
    {EffectKind::create, 145, 170, 16811, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53818, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_0_failure{{
    {EffectKind::minimum, 15, 160, 16810, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 1, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 1, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_1_targets{{
    {"ITRW_BOW_L_02", 16812},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_L_02_V1", 16813},
}};
inline constexpr std::array<SymbolBinding, 19> bow_1_own_bindings{{
    {"ITRW_BOW_L_02", 16812, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_L_02_V1", 16813, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_2", 20874, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23404",
     53822, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_1_bindings = with_common(bow_1_own_bindings);
inline constexpr std::array<Requirement, 2> bow_1_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_1_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 2},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 2},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16812, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_1_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20875)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20875)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 2},
     CheckSite::callback,
     function_for(20876)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 2},
     CheckSite::callback,
     function_for(20876)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16812, 0, 1},
     CheckSite::callback,
     function_for(20876)},
}};
inline constexpr Owner bow_1_owner{20874,
                                   2,
                                   22243,
                                   false,
                                   false,
                                   true,
                                   function_for(20874),
                                   function_for(20875),
                                   function_for(20876),
                                   view(bow_1_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_1_costs{{
    {16812, 1},
    {16610, 2},
    {16599, 2},
}};
inline constexpr std::array<ItemAmount, 1> bow_1_outputs{{
    {16813, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_1_minima{{
    {16610, 2},
    {16599, 2},
    {16812, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_1_material{{
    {MaterialAction::remove, 16812, 1, 0},
    {MaterialAction::remove, 16610, 2, 0},
    {MaterialAction::remove, 16599, 2, 0},
    {MaterialAction::create, 16813, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_1_operands{{
    {1, 1, 1, 0, false},
    {22, 2, 2, 0, false},
    {43, 2, 2, 0, false},
    {81, 1, 1, 0, false},
    {101, 2, 2, 0, false},
    {121, 2, 2, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_1_uses{{
    {1, AmountRole::minimum_inventory, 16812},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16812},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16813},
}};
inline constexpr std::array<ItemShape, 4> bow_1_items{{
    {16812, function_for(16812), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16813, function_for(16813), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_1_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53822, 47, digest_literal("e0d5e3f5eadca7fd2e1a9f2758aae3d1d27cb5bcf7ed1dd18836cd37f1992ad2")},
}};
inline constexpr std::array<Effect, 11> bow_1_success{{
    {EffectKind::minimum, 15, 160, 16812, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 2, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 2, Actor::hero},
    {EffectKind::remove, 85, 184, 16812, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 2, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 2, Actor::hero},
    {EffectKind::create, 145, 170, 16813, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53822, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_1_failure{{
    {EffectKind::minimum, 15, 160, 16812, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 2, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 2, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_2_targets{{
    {"ITRW_SLD_BOW", 16808},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_SLD_BOW_V1", 16809},
}};
inline constexpr std::array<SymbolBinding, 19> bow_2_own_bindings{{
    {"ITRW_SLD_BOW", 16808, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_SLD_BOW_V1", 16809, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_3", 20877, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23408",
     53826, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_2_bindings = with_common(bow_2_own_bindings);
inline constexpr std::array<Requirement, 2> bow_2_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_2_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16808, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_2_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20878)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20878)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
     CheckSite::callback,
     function_for(20879)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 3},
     CheckSite::callback,
     function_for(20879)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16808, 0, 1},
     CheckSite::callback,
     function_for(20879)},
}};
inline constexpr Owner bow_2_owner{20877,
                                   3,
                                   22243,
                                   false,
                                   false,
                                   true,
                                   function_for(20877),
                                   function_for(20878),
                                   function_for(20879),
                                   view(bow_2_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_2_costs{{
    {16808, 1},
    {16610, 3},
    {16599, 3},
}};
inline constexpr std::array<ItemAmount, 1> bow_2_outputs{{
    {16809, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_2_minima{{
    {16610, 3},
    {16599, 3},
    {16808, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_2_material{{
    {MaterialAction::remove, 16808, 1, 0},
    {MaterialAction::remove, 16610, 3, 0},
    {MaterialAction::remove, 16599, 3, 0},
    {MaterialAction::create, 16809, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_2_operands{{
    {1, 1, 1, 0, false},
    {22, 3, 3, 0, false},
    {43, 3, 3, 0, false},
    {81, 1, 1, 0, false},
    {101, 3, 3, 0, false},
    {121, 3, 3, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_2_uses{{
    {1, AmountRole::minimum_inventory, 16808},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16808},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16809},
}};
inline constexpr std::array<ItemShape, 4> bow_2_items{{
    {16808, function_for(16808), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16809, function_for(16809), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_2_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53826, 47, digest_literal("50eb8a4a509a463d7e1f1fbfc961d000ce07e883c142ce3afb342705c2863004")},
}};
inline constexpr std::array<Effect, 11> bow_2_success{{
    {EffectKind::minimum, 15, 160, 16808, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 3, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::remove, 85, 184, 16808, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 3, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 3, Actor::hero},
    {EffectKind::create, 145, 170, 16809, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53826, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_2_failure{{
    {EffectKind::minimum, 15, 160, 16808, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 3, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_3_targets{{
    {"ITRW_BOW_L_03", 16814},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_L_03_V1", 16815},
}};
inline constexpr std::array<SymbolBinding, 19> bow_3_own_bindings{{
    {"ITRW_BOW_L_03", 16814, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_L_03_V1", 16815, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_4", 20880, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23412",
     53830, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_3_bindings = with_common(bow_3_own_bindings);
inline constexpr std::array<Requirement, 2> bow_3_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_3_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 4},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16814, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_3_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20881)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20881)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
     CheckSite::callback,
     function_for(20882)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 4},
     CheckSite::callback,
     function_for(20882)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16814, 0, 1},
     CheckSite::callback,
     function_for(20882)},
}};
inline constexpr Owner bow_3_owner{20880,
                                   4,
                                   22243,
                                   false,
                                   false,
                                   true,
                                   function_for(20880),
                                   function_for(20881),
                                   function_for(20882),
                                   view(bow_3_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_3_costs{{
    {16814, 1},
    {16610, 3},
    {16599, 4},
}};
inline constexpr std::array<ItemAmount, 1> bow_3_outputs{{
    {16815, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_3_minima{{
    {16610, 3},
    {16599, 4},
    {16814, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_3_material{{
    {MaterialAction::remove, 16814, 1, 0},
    {MaterialAction::remove, 16610, 3, 0},
    {MaterialAction::remove, 16599, 4, 0},
    {MaterialAction::create, 16815, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_3_operands{{
    {1, 1, 1, 0, false},
    {22, 4, 4, 0, false},
    {43, 3, 3, 0, false},
    {81, 1, 1, 0, false},
    {101, 3, 3, 0, false},
    {121, 4, 4, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_3_uses{{
    {1, AmountRole::minimum_inventory, 16814},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16814},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16815},
}};
inline constexpr std::array<ItemShape, 4> bow_3_items{{
    {16814, function_for(16814), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16815, function_for(16815), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_3_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53830, 45, digest_literal("44b76e96a8ebf486d904894a38794f5f9ddb380b350285d5f55b4c79b76b430d")},
}};
inline constexpr std::array<Effect, 11> bow_3_success{{
    {EffectKind::minimum, 15, 160, 16814, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 4, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::remove, 85, 184, 16814, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 3, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 4, Actor::hero},
    {EffectKind::create, 145, 170, 16815, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53830, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_3_failure{{
    {EffectKind::minimum, 15, 160, 16814, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 4, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_4_targets{{
    {"ITRW_BOW_L_04", 16816},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_L_04_V1", 16817},
}};
inline constexpr std::array<SymbolBinding, 19> bow_4_own_bindings{{
    {"ITRW_BOW_L_04", 16816, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_L_04_V1", 16817, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_5", 20883, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23416",
     53834, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_4_bindings = with_common(bow_4_own_bindings);
inline constexpr std::array<Requirement, 2> bow_4_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_4_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 5},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16816, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_4_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20884)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20884)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
     CheckSite::callback,
     function_for(20885)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 5},
     CheckSite::callback,
     function_for(20885)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16816, 0, 1},
     CheckSite::callback,
     function_for(20885)},
}};
inline constexpr Owner bow_4_owner{20883,
                                   5,
                                   22243,
                                   false,
                                   false,
                                   true,
                                   function_for(20883),
                                   function_for(20884),
                                   function_for(20885),
                                   view(bow_4_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_4_costs{{
    {16816, 1},
    {16610, 3},
    {16599, 5},
}};
inline constexpr std::array<ItemAmount, 1> bow_4_outputs{{
    {16817, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_4_minima{{
    {16610, 3},
    {16599, 5},
    {16816, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_4_material{{
    {MaterialAction::remove, 16816, 1, 0},
    {MaterialAction::remove, 16610, 3, 0},
    {MaterialAction::remove, 16599, 5, 0},
    {MaterialAction::create, 16817, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_4_operands{{
    {1, 1, 1, 0, false},
    {22, 5, 5, 0, false},
    {43, 3, 3, 0, false},
    {81, 1, 1, 0, false},
    {101, 3, 3, 0, false},
    {121, 5, 5, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_4_uses{{
    {1, AmountRole::minimum_inventory, 16816},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16816},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16817},
}};
inline constexpr std::array<ItemShape, 4> bow_4_items{{
    {16816, function_for(16816), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16817, function_for(16817), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_4_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53834, 46, digest_literal("f6e319a97dd55b97ca969c7fad87bc3749789a4c9a5e6e643574e7f3562bffe5")},
}};
inline constexpr std::array<Effect, 11> bow_4_success{{
    {EffectKind::minimum, 15, 160, 16816, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 5, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::remove, 85, 184, 16816, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 3, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 5, Actor::hero},
    {EffectKind::create, 145, 170, 16817, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53834, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_4_failure{{
    {EffectKind::minimum, 15, 160, 16816, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 5, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_5_targets{{
    {"ITRW_BOW_M_01", 16818},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_M_01_V1", 16819},
}};
inline constexpr std::array<SymbolBinding, 19> bow_5_own_bindings{{
    {"ITRW_BOW_M_01", 16818, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_M_01_V1", 16819, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_6", 20886, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23420",
     53838, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_5_bindings = with_common(bow_5_own_bindings);
inline constexpr std::array<Requirement, 2> bow_5_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_5_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 6},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16818, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_5_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20887)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20887)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
     CheckSite::callback,
     function_for(20888)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 6},
     CheckSite::callback,
     function_for(20888)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16818, 0, 1},
     CheckSite::callback,
     function_for(20888)},
}};
inline constexpr Owner bow_5_owner{20886,
                                   6,
                                   22243,
                                   false,
                                   false,
                                   true,
                                   function_for(20886),
                                   function_for(20887),
                                   function_for(20888),
                                   view(bow_5_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_5_costs{{
    {16818, 1},
    {16610, 3},
    {16599, 6},
}};
inline constexpr std::array<ItemAmount, 1> bow_5_outputs{{
    {16819, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_5_minima{{
    {16610, 3},
    {16599, 6},
    {16818, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_5_material{{
    {MaterialAction::remove, 16818, 1, 0},
    {MaterialAction::remove, 16610, 3, 0},
    {MaterialAction::remove, 16599, 6, 0},
    {MaterialAction::create, 16819, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_5_operands{{
    {1, 1, 1, 0, false},
    {22, 6, 6, 0, false},
    {43, 3, 3, 0, false},
    {81, 1, 1, 0, false},
    {101, 3, 3, 0, false},
    {121, 6, 6, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_5_uses{{
    {1, AmountRole::minimum_inventory, 16818},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16818},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16819},
}};
inline constexpr std::array<ItemShape, 4> bow_5_items{{
    {16818, function_for(16818), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16819, function_for(16819), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_5_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53838, 49, digest_literal("2437d81b1b7cec166326e4c61e676f419ab84f6af3294aac841b98adedf0210e")},
}};
inline constexpr std::array<Effect, 11> bow_5_success{{
    {EffectKind::minimum, 15, 160, 16818, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 6, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::remove, 85, 184, 16818, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 3, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 6, Actor::hero},
    {EffectKind::create, 145, 170, 16819, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53838, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_5_failure{{
    {EffectKind::minimum, 15, 160, 16818, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 6, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_6_targets{{
    {"ITRW_BOW_M_02", 16820},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_M_02_V1", 16821},
}};
inline constexpr std::array<SymbolBinding, 19> bow_6_own_bindings{{
    {"ITRW_BOW_M_02", 16820, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_M_02_V1", 16821, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_7", 20889, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23424",
     53842, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_6_bindings = with_common(bow_6_own_bindings);
inline constexpr std::array<Requirement, 2> bow_6_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_6_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 7},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16820, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_6_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20890)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20890)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
     CheckSite::callback,
     function_for(20891)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 7},
     CheckSite::callback,
     function_for(20891)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16820, 0, 1},
     CheckSite::callback,
     function_for(20891)},
}};
inline constexpr Owner bow_6_owner{20889,
                                   7,
                                   22243,
                                   false,
                                   false,
                                   true,
                                   function_for(20889),
                                   function_for(20890),
                                   function_for(20891),
                                   view(bow_6_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_6_costs{{
    {16820, 1},
    {16610, 3},
    {16599, 7},
}};
inline constexpr std::array<ItemAmount, 1> bow_6_outputs{{
    {16821, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_6_minima{{
    {16610, 3},
    {16599, 7},
    {16820, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_6_material{{
    {MaterialAction::remove, 16820, 1, 0},
    {MaterialAction::remove, 16610, 3, 0},
    {MaterialAction::remove, 16599, 7, 0},
    {MaterialAction::create, 16821, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_6_operands{{
    {1, 1, 1, 0, false},
    {22, 7, 7, 0, false},
    {43, 3, 3, 0, false},
    {81, 1, 1, 0, false},
    {101, 3, 3, 0, false},
    {121, 7, 7, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_6_uses{{
    {1, AmountRole::minimum_inventory, 16820},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16820},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16821},
}};
inline constexpr std::array<ItemShape, 4> bow_6_items{{
    {16820, function_for(16820), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16821, function_for(16821), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_6_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53842, 47, digest_literal("bf3737f6c25afc48825751ea2ff97cf253be4c33a11b15f6cfde1237dc5418a3")},
}};
inline constexpr std::array<Effect, 11> bow_6_success{{
    {EffectKind::minimum, 15, 160, 16820, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 7, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::remove, 85, 184, 16820, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 3, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 7, Actor::hero},
    {EffectKind::create, 145, 170, 16821, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53842, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_6_failure{{
    {EffectKind::minimum, 15, 160, 16820, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 7, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_7_targets{{
    {"ITRW_BOW_M_03", 16822},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_M_03_V1", 16823},
}};
inline constexpr std::array<SymbolBinding, 19> bow_7_own_bindings{{
    {"ITRW_BOW_M_03", 16822, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_M_03_V1", 16823, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_8", 20892, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23428",
     53846, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_7_bindings = with_common(bow_7_own_bindings);
inline constexpr std::array<Requirement, 2> bow_7_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_7_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 8},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16822, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_7_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20893)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20893)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
     CheckSite::callback,
     function_for(20894)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 8},
     CheckSite::callback,
     function_for(20894)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16822, 0, 1},
     CheckSite::callback,
     function_for(20894)},
}};
inline constexpr Owner bow_7_owner{20892,
                                   8,
                                   22243,
                                   false,
                                   false,
                                   true,
                                   function_for(20892),
                                   function_for(20893),
                                   function_for(20894),
                                   view(bow_7_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_7_costs{{
    {16822, 1},
    {16610, 3},
    {16599, 8},
}};
inline constexpr std::array<ItemAmount, 1> bow_7_outputs{{
    {16823, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_7_minima{{
    {16610, 3},
    {16599, 8},
    {16822, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_7_material{{
    {MaterialAction::remove, 16822, 1, 0},
    {MaterialAction::remove, 16610, 3, 0},
    {MaterialAction::remove, 16599, 8, 0},
    {MaterialAction::create, 16823, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_7_operands{{
    {1, 1, 1, 0, false},
    {22, 8, 8, 0, false},
    {43, 3, 3, 0, false},
    {81, 1, 1, 0, false},
    {101, 3, 3, 0, false},
    {121, 8, 8, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_7_uses{{
    {1, AmountRole::minimum_inventory, 16822},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16822},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16823},
}};
inline constexpr std::array<ItemShape, 4> bow_7_items{{
    {16822, function_for(16822), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16823, function_for(16823), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_7_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53846, 45, digest_literal("cbad68dd993ca324fa94e5b7e00c68405d7e3b617ba284858aa3c44605c04f7d")},
}};
inline constexpr std::array<Effect, 11> bow_7_success{{
    {EffectKind::minimum, 15, 160, 16822, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 8, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::remove, 85, 184, 16822, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 3, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 8, Actor::hero},
    {EffectKind::create, 145, 170, 16823, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53846, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_7_failure{{
    {EffectKind::minimum, 15, 160, 16822, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 8, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_8_targets{{
    {"ITRW_BOW_M_04", 16824},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_M_04_V1", 16825},
}};
inline constexpr std::array<SymbolBinding, 19> bow_8_own_bindings{{
    {"ITRW_BOW_M_04", 16824, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_M_04_V1", 16825, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_9", 20895, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23432",
     53850, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_8_bindings = with_common(bow_8_own_bindings);
inline constexpr std::array<Requirement, 2> bow_8_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_8_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 9},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16824, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_8_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20896)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20896)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
     CheckSite::callback,
     function_for(20897)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 9},
     CheckSite::callback,
     function_for(20897)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16824, 0, 1},
     CheckSite::callback,
     function_for(20897)},
}};
inline constexpr Owner bow_8_owner{20895,
                                   9,
                                   22243,
                                   false,
                                   false,
                                   true,
                                   function_for(20895),
                                   function_for(20896),
                                   function_for(20897),
                                   view(bow_8_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_8_costs{{
    {16824, 1},
    {16610, 3},
    {16599, 9},
}};
inline constexpr std::array<ItemAmount, 1> bow_8_outputs{{
    {16825, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_8_minima{{
    {16610, 3},
    {16599, 9},
    {16824, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_8_material{{
    {MaterialAction::remove, 16824, 1, 0},
    {MaterialAction::remove, 16610, 3, 0},
    {MaterialAction::remove, 16599, 9, 0},
    {MaterialAction::create, 16825, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_8_operands{{
    {1, 1, 1, 0, false},
    {22, 9, 9, 0, false},
    {43, 3, 3, 0, false},
    {81, 1, 1, 0, false},
    {101, 3, 3, 0, false},
    {121, 9, 9, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_8_uses{{
    {1, AmountRole::minimum_inventory, 16824},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16824},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16825},
}};
inline constexpr std::array<ItemShape, 4> bow_8_items{{
    {16824, function_for(16824), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16825, function_for(16825), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_8_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53850, 47, digest_literal("509f6ffdbaffe9972416a189133f58ebcb6b5128320d4bb07f22ceca596c3bc1")},
}};
inline constexpr std::array<Effect, 11> bow_8_success{{
    {EffectKind::minimum, 15, 160, 16824, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 9, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::remove, 85, 184, 16824, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 3, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 9, Actor::hero},
    {EffectKind::create, 145, 170, 16825, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53850, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_8_failure{{
    {EffectKind::minimum, 15, 160, 16824, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 9, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_9_targets{{
    {"ITRW_BOW_H_01", 16826},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_H_01_V1", 16827},
}};
inline constexpr std::array<SymbolBinding, 19> bow_9_own_bindings{{
    {"ITRW_BOW_H_01", 16826, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_H_01_V1", 16827, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_10", 20898, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23436",
     53854, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_9_bindings = with_common(bow_9_own_bindings);
inline constexpr std::array<Requirement, 2> bow_9_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_9_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 10},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16826, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_9_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20899)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20899)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
     CheckSite::callback,
     function_for(20900)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 10},
     CheckSite::callback,
     function_for(20900)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16826, 0, 1},
     CheckSite::callback,
     function_for(20900)},
}};
inline constexpr Owner bow_9_owner{20898,
                                   10,
                                   22243,
                                   false,
                                   false,
                                   true,
                                   function_for(20898),
                                   function_for(20899),
                                   function_for(20900),
                                   view(bow_9_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_9_costs{{
    {16826, 1},
    {16610, 3},
    {16599, 10},
}};
inline constexpr std::array<ItemAmount, 1> bow_9_outputs{{
    {16827, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_9_minima{{
    {16610, 3},
    {16599, 10},
    {16826, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_9_material{{
    {MaterialAction::remove, 16826, 1, 0},
    {MaterialAction::remove, 16610, 3, 0},
    {MaterialAction::remove, 16599, 10, 0},
    {MaterialAction::create, 16827, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_9_operands{{
    {1, 1, 1, 0, false},
    {22, 10, 10, 0, false},
    {43, 3, 3, 0, false},
    {81, 1, 1, 0, false},
    {101, 3, 3, 0, false},
    {121, 10, 10, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_9_uses{{
    {1, AmountRole::minimum_inventory, 16826},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16826},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16827},
}};
inline constexpr std::array<ItemShape, 4> bow_9_items{{
    {16826, function_for(16826), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16827, function_for(16827), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_9_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53854, 48, digest_literal("e8a1cf196bc1d19ec1165135cc1c755adb81bd98bcd0ac9a4de9f3e476b3a2e2")},
}};
inline constexpr std::array<Effect, 11> bow_9_success{{
    {EffectKind::minimum, 15, 160, 16826, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 10, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::remove, 85, 184, 16826, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 3, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 10, Actor::hero},
    {EffectKind::create, 145, 170, 16827, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53854, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_9_failure{{
    {EffectKind::minimum, 15, 160, 16826, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 10, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_10_targets{{
    {"ITRW_BOW_H_02", 16828},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_H_02_V1", 16829},
}};
inline constexpr std::array<SymbolBinding, 19> bow_10_own_bindings{{
    {"ITRW_BOW_H_02", 16828, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_H_02_V1", 16829, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_11", 20901, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23440",
     53858, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_10_bindings = with_common(bow_10_own_bindings);
inline constexpr std::array<Requirement, 2> bow_10_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_10_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 11},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16828, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_10_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20902)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20902)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
     CheckSite::callback,
     function_for(20903)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 11},
     CheckSite::callback,
     function_for(20903)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16828, 0, 1},
     CheckSite::callback,
     function_for(20903)},
}};
inline constexpr Owner bow_10_owner{20901,
                                    11,
                                    22243,
                                    false,
                                    false,
                                    true,
                                    function_for(20901),
                                    function_for(20902),
                                    function_for(20903),
                                    view(bow_10_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_10_costs{{
    {16828, 1},
    {16610, 3},
    {16599, 11},
}};
inline constexpr std::array<ItemAmount, 1> bow_10_outputs{{
    {16829, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_10_minima{{
    {16610, 3},
    {16599, 11},
    {16828, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_10_material{{
    {MaterialAction::remove, 16828, 1, 0},
    {MaterialAction::remove, 16610, 3, 0},
    {MaterialAction::remove, 16599, 11, 0},
    {MaterialAction::create, 16829, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_10_operands{{
    {1, 1, 1, 0, false},
    {22, 11, 11, 0, false},
    {43, 3, 3, 0, false},
    {81, 1, 1, 0, false},
    {101, 3, 3, 0, false},
    {121, 11, 11, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_10_uses{{
    {1, AmountRole::minimum_inventory, 16828},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16828},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16829},
}};
inline constexpr std::array<ItemShape, 4> bow_10_items{{
    {16828, function_for(16828), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16829, function_for(16829), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_10_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53858, 47, digest_literal("f550aec118c9b32ddbd84d534d450a9d2ef6c98ab1517dee75966a26a911b8bc")},
}};
inline constexpr std::array<Effect, 11> bow_10_success{{
    {EffectKind::minimum, 15, 160, 16828, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 11, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::remove, 85, 184, 16828, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 3, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 11, Actor::hero},
    {EffectKind::create, 145, 170, 16829, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53858, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_10_failure{{
    {EffectKind::minimum, 15, 160, 16828, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 11, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_11_targets{{
    {"ITRW_BOW_H_03", 16830},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_H_03_V1", 16831},
}};
inline constexpr std::array<SymbolBinding, 19> bow_11_own_bindings{{
    {"ITRW_BOW_H_03", 16830, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_H_03_V1", 16831, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_12", 20904, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23444",
     53862, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_11_bindings = with_common(bow_11_own_bindings);
inline constexpr std::array<Requirement, 2> bow_11_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_11_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 5},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 12},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16830, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_11_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20905)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20905)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 5},
     CheckSite::callback,
     function_for(20906)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 12},
     CheckSite::callback,
     function_for(20906)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16830, 0, 1},
     CheckSite::callback,
     function_for(20906)},
}};
inline constexpr Owner bow_11_owner{20904,
                                    12,
                                    22243,
                                    false,
                                    false,
                                    true,
                                    function_for(20904),
                                    function_for(20905),
                                    function_for(20906),
                                    view(bow_11_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_11_costs{{
    {16830, 1},
    {16610, 5},
    {16599, 12},
}};
inline constexpr std::array<ItemAmount, 1> bow_11_outputs{{
    {16831, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_11_minima{{
    {16610, 5},
    {16599, 12},
    {16830, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_11_material{{
    {MaterialAction::remove, 16830, 1, 0},
    {MaterialAction::remove, 16610, 5, 0},
    {MaterialAction::remove, 16599, 12, 0},
    {MaterialAction::create, 16831, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_11_operands{{
    {1, 1, 1, 0, false},
    {22, 12, 12, 0, false},
    {43, 5, 5, 0, false},
    {81, 1, 1, 0, false},
    {101, 5, 5, 0, false},
    {121, 12, 12, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_11_uses{{
    {1, AmountRole::minimum_inventory, 16830},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16830},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16831},
}};
inline constexpr std::array<ItemShape, 4> bow_11_items{{
    {16830, function_for(16830), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16831, function_for(16831), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_11_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53862, 47, digest_literal("d8fb23c23766a72cab368485ae845f65cb5c673c4bf2e6bc68754f64ce86a2eb")},
}};
inline constexpr std::array<Effect, 11> bow_11_success{{
    {EffectKind::minimum, 15, 160, 16830, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 12, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 5, Actor::hero},
    {EffectKind::remove, 85, 184, 16830, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 5, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 12, Actor::hero},
    {EffectKind::create, 145, 170, 16831, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53862, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_11_failure{{
    {EffectKind::minimum, 15, 160, 16830, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 12, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 5, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_12_targets{{
    {"ITRW_BOW_H_04", 16832},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_BOW_H_04_V1", 16833},
}};
inline constexpr std::array<SymbolBinding, 19> bow_12_own_bindings{{
    {"ITRW_BOW_H_04", 16832, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_BOW_H_04_V1", 16833, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_13", 20907, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_BOW", 1049, 2, 1, 1, -1, true, 524288, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23448",
     53866, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_12_bindings = with_common(bow_12_own_bindings);
inline constexpr std::array<Requirement, 2> bow_12_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_12_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 5},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 13},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16832, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_12_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20908)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20908)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 5},
     CheckSite::callback,
     function_for(20909)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 13},
     CheckSite::callback,
     function_for(20909)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16832, 0, 1},
     CheckSite::callback,
     function_for(20909)},
}};
inline constexpr Owner bow_12_owner{20907,
                                    13,
                                    22243,
                                    false,
                                    false,
                                    true,
                                    function_for(20907),
                                    function_for(20908),
                                    function_for(20909),
                                    view(bow_12_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_12_costs{{
    {16832, 1},
    {16610, 5},
    {16599, 13},
}};
inline constexpr std::array<ItemAmount, 1> bow_12_outputs{{
    {16833, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_12_minima{{
    {16610, 5},
    {16599, 13},
    {16832, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_12_material{{
    {MaterialAction::remove, 16832, 1, 0},
    {MaterialAction::remove, 16610, 5, 0},
    {MaterialAction::remove, 16599, 13, 0},
    {MaterialAction::create, 16833, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_12_operands{{
    {1, 1, 1, 0, false},
    {22, 13, 13, 0, false},
    {43, 5, 5, 0, false},
    {81, 1, 1, 0, false},
    {101, 5, 5, 0, false},
    {121, 13, 13, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_12_uses{{
    {1, AmountRole::minimum_inventory, 16832},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16832},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16833},
}};
inline constexpr std::array<ItemShape, 4> bow_12_items{{
    {16832, function_for(16832), 4, 524288, 524292, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16833, function_for(16833), 4, 524288, 524292, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_12_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53866, 48, digest_literal("012b617f5d264a6d987cfbca6f1e3522caddbc603ffd8dd6e9d329cf1ea08488")},
}};
inline constexpr std::array<Effect, 11> bow_12_success{{
    {EffectKind::minimum, 15, 160, 16832, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 13, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 5, Actor::hero},
    {EffectKind::remove, 85, 184, 16832, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 5, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 13, Actor::hero},
    {EffectKind::create, 145, 170, 16833, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53866, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_12_failure{{
    {EffectKind::minimum, 15, 160, 16832, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 13, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 5, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_13_targets{{
    {"ITRW_MIL_CROSSBOW", 16806},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_MIL_CROSSBOW_V1", 16807},
}};
inline constexpr std::array<SymbolBinding, 19> bow_13_own_bindings{{
    {"ITRW_MIL_CROSSBOW", 16806, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_MIL_CROSSBOW_V1", 16807, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_14", 20910, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_CROSSBOW", 1050, 2, 1, 1, -1, true, 1048576, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23452",
     53870, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_13_bindings = with_common(bow_13_own_bindings);
inline constexpr std::array<Requirement, 2> bow_13_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_13_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 2},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 2},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16806, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_13_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20911)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20911)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 2},
     CheckSite::callback,
     function_for(20912)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 2},
     CheckSite::callback,
     function_for(20912)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16806, 0, 1},
     CheckSite::callback,
     function_for(20912)},
}};
inline constexpr Owner bow_13_owner{20910,
                                    14,
                                    22243,
                                    false,
                                    false,
                                    true,
                                    function_for(20910),
                                    function_for(20911),
                                    function_for(20912),
                                    view(bow_13_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_13_costs{{
    {16806, 1},
    {16610, 2},
    {16599, 2},
}};
inline constexpr std::array<ItemAmount, 1> bow_13_outputs{{
    {16807, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_13_minima{{
    {16610, 2},
    {16599, 2},
    {16806, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_13_material{{
    {MaterialAction::remove, 16806, 1, 0},
    {MaterialAction::remove, 16610, 2, 0},
    {MaterialAction::remove, 16599, 2, 0},
    {MaterialAction::create, 16807, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_13_operands{{
    {1, 1, 1, 0, false},
    {22, 2, 2, 0, false},
    {43, 2, 2, 0, false},
    {81, 1, 1, 0, false},
    {101, 2, 2, 0, false},
    {121, 2, 2, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_13_uses{{
    {1, AmountRole::minimum_inventory, 16806},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16806},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16807},
}};
inline constexpr std::array<ItemShape, 4> bow_13_items{{
    {16806, function_for(16806), 4, 1048576, 1048580, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16807, function_for(16807), 4, 1048576, 1048580, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_13_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53870, 52, digest_literal("27b9676fa4ce4ec78af057b2aa80d11116aad13356dfc092c84c4c960ca73d9a")},
}};
inline constexpr std::array<Effect, 11> bow_13_success{{
    {EffectKind::minimum, 15, 160, 16806, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 2, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 2, Actor::hero},
    {EffectKind::remove, 85, 184, 16806, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 2, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 2, Actor::hero},
    {EffectKind::create, 145, 170, 16807, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53870, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_13_failure{{
    {EffectKind::minimum, 15, 160, 16806, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 2, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 2, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_14_targets{{
    {"ITRW_CROSSBOW_L_01", 16834},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_CROSSBOW_L_01_V1", 16835},
}};
inline constexpr std::array<SymbolBinding, 19> bow_14_own_bindings{{
    {"ITRW_CROSSBOW_L_01", 16834, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_CROSSBOW_L_01_V1", 16835, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_15", 20913, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_CROSSBOW", 1050, 2, 1, 1, -1, true, 1048576, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23456",
     53874, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_14_bindings = with_common(bow_14_own_bindings);
inline constexpr std::array<Requirement, 2> bow_14_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_14_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16834, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_14_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20914)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20914)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 1},
     CheckSite::callback,
     function_for(20915)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 1},
     CheckSite::callback,
     function_for(20915)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16834, 0, 1},
     CheckSite::callback,
     function_for(20915)},
}};
inline constexpr Owner bow_14_owner{20913,
                                    15,
                                    22243,
                                    false,
                                    false,
                                    true,
                                    function_for(20913),
                                    function_for(20914),
                                    function_for(20915),
                                    view(bow_14_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_14_costs{{
    {16834, 1},
    {16610, 1},
    {16599, 1},
}};
inline constexpr std::array<ItemAmount, 1> bow_14_outputs{{
    {16835, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_14_minima{{
    {16610, 1},
    {16599, 1},
    {16834, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_14_material{{
    {MaterialAction::remove, 16834, 1, 0},
    {MaterialAction::remove, 16610, 1, 0},
    {MaterialAction::remove, 16599, 1, 0},
    {MaterialAction::create, 16835, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_14_operands{{
    {1, 1, 1, 0, false},
    {22, 1, 1, 0, false},
    {43, 1, 1, 0, false},
    {81, 1, 1, 0, false},
    {101, 1, 1, 0, false},
    {121, 1, 1, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_14_uses{{
    {1, AmountRole::minimum_inventory, 16834},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16834},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16835},
}};
inline constexpr std::array<ItemShape, 4> bow_14_items{{
    {16834, function_for(16834), 4, 1048576, 1048580, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16835, function_for(16835), 4, 1048576, 1048580, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_14_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53874, 48, digest_literal("ed55c162ea96fe5572bd49f8934eb2d46c9b4903f9573e4cc2c7675f547ae633")},
}};
inline constexpr std::array<Effect, 11> bow_14_success{{
    {EffectKind::minimum, 15, 160, 16834, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 1, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 1, Actor::hero},
    {EffectKind::remove, 85, 184, 16834, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 1, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 1, Actor::hero},
    {EffectKind::create, 145, 170, 16835, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53874, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_14_failure{{
    {EffectKind::minimum, 15, 160, 16834, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 1, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 1, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_15_targets{{
    {"ITRW_CROSSBOW_L_02", 16836},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_CROSSBOW_L_02_V1", 16837},
}};
inline constexpr std::array<SymbolBinding, 19> bow_15_own_bindings{{
    {"ITRW_CROSSBOW_L_02", 16836, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_CROSSBOW_L_02_V1", 16837, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_16", 20916, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_CROSSBOW", 1050, 2, 1, 1, -1, true, 1048576, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23460",
     53878, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_15_bindings = with_common(bow_15_own_bindings);
inline constexpr std::array<Requirement, 2> bow_15_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_15_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 3},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16836, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_15_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20917)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20917)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 3},
     CheckSite::callback,
     function_for(20918)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 3},
     CheckSite::callback,
     function_for(20918)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16836, 0, 1},
     CheckSite::callback,
     function_for(20918)},
}};
inline constexpr Owner bow_15_owner{20916,
                                    16,
                                    22243,
                                    false,
                                    false,
                                    true,
                                    function_for(20916),
                                    function_for(20917),
                                    function_for(20918),
                                    view(bow_15_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_15_costs{{
    {16836, 1},
    {16610, 3},
    {16599, 3},
}};
inline constexpr std::array<ItemAmount, 1> bow_15_outputs{{
    {16837, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_15_minima{{
    {16610, 3},
    {16599, 3},
    {16836, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_15_material{{
    {MaterialAction::remove, 16836, 1, 0},
    {MaterialAction::remove, 16610, 3, 0},
    {MaterialAction::remove, 16599, 3, 0},
    {MaterialAction::create, 16837, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_15_operands{{
    {1, 1, 1, 0, false},
    {22, 3, 3, 0, false},
    {43, 3, 3, 0, false},
    {81, 1, 1, 0, false},
    {101, 3, 3, 0, false},
    {121, 3, 3, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_15_uses{{
    {1, AmountRole::minimum_inventory, 16836},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16836},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16837},
}};
inline constexpr std::array<ItemShape, 4> bow_15_items{{
    {16836, function_for(16836), 4, 1048576, 1048580, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16837, function_for(16837), 4, 1048576, 1048580, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_15_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53878, 44, digest_literal("8943fee2b671b557d96930ddd33ab2cc9d6d5c3aecb2052d34d97963ca903958")},
}};
inline constexpr std::array<Effect, 11> bow_15_success{{
    {EffectKind::minimum, 15, 160, 16836, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 3, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::remove, 85, 184, 16836, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 3, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 3, Actor::hero},
    {EffectKind::create, 145, 170, 16837, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53878, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_15_failure{{
    {EffectKind::minimum, 15, 160, 16836, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 3, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 3, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_16_targets{{
    {"ITRW_CROSSBOW_M_01", 16838},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_CROSSBOW_M_01_V1", 16839},
}};
inline constexpr std::array<SymbolBinding, 19> bow_16_own_bindings{{
    {"ITRW_CROSSBOW_M_01", 16838, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_CROSSBOW_M_01_V1", 16839, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_17", 20919, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_CROSSBOW", 1050, 2, 1, 1, -1, true, 1048576, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23464",
     53882, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_16_bindings = with_common(bow_16_own_bindings);
inline constexpr std::array<Requirement, 2> bow_16_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_16_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 4},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 4},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16838, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_16_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20920)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20920)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 4},
     CheckSite::callback,
     function_for(20921)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 4},
     CheckSite::callback,
     function_for(20921)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16838, 0, 1},
     CheckSite::callback,
     function_for(20921)},
}};
inline constexpr Owner bow_16_owner{20919,
                                    17,
                                    22243,
                                    false,
                                    false,
                                    true,
                                    function_for(20919),
                                    function_for(20920),
                                    function_for(20921),
                                    view(bow_16_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_16_costs{{
    {16838, 1},
    {16610, 4},
    {16599, 4},
}};
inline constexpr std::array<ItemAmount, 1> bow_16_outputs{{
    {16839, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_16_minima{{
    {16610, 4},
    {16599, 4},
    {16838, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_16_material{{
    {MaterialAction::remove, 16838, 1, 0},
    {MaterialAction::remove, 16610, 4, 0},
    {MaterialAction::remove, 16599, 4, 0},
    {MaterialAction::create, 16839, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_16_operands{{
    {1, 1, 1, 0, false},
    {22, 4, 4, 0, false},
    {43, 4, 4, 0, false},
    {81, 1, 1, 0, false},
    {101, 4, 4, 0, false},
    {121, 4, 4, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_16_uses{{
    {1, AmountRole::minimum_inventory, 16838},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16838},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16839},
}};
inline constexpr std::array<ItemShape, 4> bow_16_items{{
    {16838, function_for(16838), 4, 1048576, 1048580, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16839, function_for(16839), 4, 1048576, 1048580, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_16_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53882, 49, digest_literal("ef5a7f4760e8000e4f7a975c0f0580f633285baa1f3527de77c62c135382ec8c")},
}};
inline constexpr std::array<Effect, 11> bow_16_success{{
    {EffectKind::minimum, 15, 160, 16838, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 4, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 4, Actor::hero},
    {EffectKind::remove, 85, 184, 16838, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 4, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 4, Actor::hero},
    {EffectKind::create, 145, 170, 16839, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53882, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_16_failure{{
    {EffectKind::minimum, 15, 160, 16838, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 4, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 4, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_17_targets{{
    {"ITRW_CROSSBOW_M_02", 16840},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_CROSSBOW_M_02_V1", 16841},
}};
inline constexpr std::array<SymbolBinding, 19> bow_17_own_bindings{{
    {"ITRW_CROSSBOW_M_02", 16840, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_CROSSBOW_M_02_V1", 16841, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_18", 20922, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_CROSSBOW", 1050, 2, 1, 1, -1, true, 1048576, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23468",
     53886, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_17_bindings = with_common(bow_17_own_bindings);
inline constexpr std::array<Requirement, 2> bow_17_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_17_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 6},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 6},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16840, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_17_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20923)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20923)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 6},
     CheckSite::callback,
     function_for(20924)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 6},
     CheckSite::callback,
     function_for(20924)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16840, 0, 1},
     CheckSite::callback,
     function_for(20924)},
}};
inline constexpr Owner bow_17_owner{20922,
                                    18,
                                    22243,
                                    false,
                                    false,
                                    true,
                                    function_for(20922),
                                    function_for(20923),
                                    function_for(20924),
                                    view(bow_17_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_17_costs{{
    {16840, 1},
    {16610, 6},
    {16599, 6},
}};
inline constexpr std::array<ItemAmount, 1> bow_17_outputs{{
    {16841, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_17_minima{{
    {16610, 6},
    {16599, 6},
    {16840, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_17_material{{
    {MaterialAction::remove, 16840, 1, 0},
    {MaterialAction::remove, 16610, 6, 0},
    {MaterialAction::remove, 16599, 6, 0},
    {MaterialAction::create, 16841, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_17_operands{{
    {1, 1, 1, 0, false},
    {22, 6, 6, 0, false},
    {43, 6, 6, 0, false},
    {81, 1, 1, 0, false},
    {101, 6, 6, 0, false},
    {121, 6, 6, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_17_uses{{
    {1, AmountRole::minimum_inventory, 16840},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16840},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16841},
}};
inline constexpr std::array<ItemShape, 4> bow_17_items{{
    {16840, function_for(16840), 4, 1048576, 1048580, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16841, function_for(16841), 4, 1048576, 1048580, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_17_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53886, 50, digest_literal("5fea70595e0cee73f121aa7c21f0b41aa9912c9efc1312c646f33ab4a30e3d69")},
}};
inline constexpr std::array<Effect, 11> bow_17_success{{
    {EffectKind::minimum, 15, 160, 16840, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 6, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 6, Actor::hero},
    {EffectKind::remove, 85, 184, 16840, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 6, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 6, Actor::hero},
    {EffectKind::create, 145, 170, 16841, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53886, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_17_failure{{
    {EffectKind::minimum, 15, 160, 16840, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 6, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 6, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_18_targets{{
    {"ITRW_CROSSBOW_H_01", 16842},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_CROSSBOW_H_01_V1", 16843},
}};
inline constexpr std::array<SymbolBinding, 19> bow_18_own_bindings{{
    {"ITRW_CROSSBOW_H_01", 16842, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_CROSSBOW_H_01_V1", 16843, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_19", 20925, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_CROSSBOW", 1050, 2, 1, 1, -1, true, 1048576, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23472",
     53890, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_18_bindings = with_common(bow_18_own_bindings);
inline constexpr std::array<Requirement, 2> bow_18_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_18_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 8},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 8},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16842, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_18_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20926)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20926)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 8},
     CheckSite::callback,
     function_for(20927)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 8},
     CheckSite::callback,
     function_for(20927)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16842, 0, 1},
     CheckSite::callback,
     function_for(20927)},
}};
inline constexpr Owner bow_18_owner{20925,
                                    19,
                                    22243,
                                    false,
                                    false,
                                    true,
                                    function_for(20925),
                                    function_for(20926),
                                    function_for(20927),
                                    view(bow_18_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_18_costs{{
    {16842, 1},
    {16610, 8},
    {16599, 8},
}};
inline constexpr std::array<ItemAmount, 1> bow_18_outputs{{
    {16843, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_18_minima{{
    {16610, 8},
    {16599, 8},
    {16842, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_18_material{{
    {MaterialAction::remove, 16842, 1, 0},
    {MaterialAction::remove, 16610, 8, 0},
    {MaterialAction::remove, 16599, 8, 0},
    {MaterialAction::create, 16843, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_18_operands{{
    {1, 1, 1, 0, false},
    {22, 8, 8, 0, false},
    {43, 8, 8, 0, false},
    {81, 1, 1, 0, false},
    {101, 8, 8, 0, false},
    {121, 8, 8, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_18_uses{{
    {1, AmountRole::minimum_inventory, 16842},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16842},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16843},
}};
inline constexpr std::array<ItemShape, 4> bow_18_items{{
    {16842, function_for(16842), 4, 1048576, 1048580, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16843, function_for(16843), 4, 1048576, 1048580, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_18_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53890, 52, digest_literal("dca52057c2b2d63bf22a1164fc4a2746b4e1cd34612166c0bc70bb892e937d20")},
}};
inline constexpr std::array<Effect, 11> bow_18_success{{
    {EffectKind::minimum, 15, 160, 16842, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 8, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 8, Actor::hero},
    {EffectKind::remove, 85, 184, 16842, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 8, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 8, Actor::hero},
    {EffectKind::create, 145, 170, 16843, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53890, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_18_failure{{
    {EffectKind::minimum, 15, 160, 16842, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 8, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 8, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<PackedInventoryTarget, 4> bow_19_targets{{
    {"ITRW_CROSSBOW_H_02", 16844},
    {"ITMISWORDRAW", 16610},
    {"ITMI_LEATHER", 16599},
    {"ITRW_CROSSBOW_H_02_V1", 16845},
}};
inline constexpr std::array<SymbolBinding, 19> bow_19_own_bindings{{
    {"ITRW_CROSSBOW_H_02", 16844, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMISWORDRAW", 16610, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITMI_LEATHER", 16599, 7, 1, 0, 1584, false, 0, false, 0},
    {"ITRW_CROSSBOW_H_02_V1", 16845, 7, 1, 0, 1584, false, 0, false, 0},
    {"PC_MAKEBOWS_BSPR_20", 20928, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_MAKEBOWS_END", 20865, 7, 1, 0, 1649, false, 0, false, 0},
    {"PC_BOWS", 20868, 7, 1, 0, 1649, false, 0, false, 0},
    {"PLAYER_TALENT_BOW_CROSSBOW", 10484, 2, 0, 1, -1, false, 0, false, 0},
    {"M_BOWS", 20862, 2, 0, 1, -1, false, 0, false, 0},
    {"MOBSI_MAKEBOWS", 12326, 2, 1, 1, -1, true, 16, false, 0},
    {"C_ITEM.MAINFLAG", 1590, 2, 4, 1, 1584, false, 0, true, 340},
    {"C_ITEM.FLAGS", 1591, 2, 4, 1, 1584, false, 0, true, 344},
    {"ITEM_KAT_FF", 1033, 2, 1, 1, -1, true, 4, false, 0},
    {"ITEM_CROSSBOW", 1050, 2, 1, 1, -1, true, 1048576, false, 0},
    {"ITEM_KAT_NONE", 1031, 2, 1, 1, -1, true, 1, false, 0},
    {"ITEM_MULTI", 1056, 2, 1, 1, -1, true, 2097152, false, 0},
    {"PRINT_PRODITEMSMISSING", 14374, 3, 1, 1, -1, false, 0, false, 0},
    {"PRINT_NOPICKLOCKTALENT", 14389, 3, 1, 1, -1, false, 0, false, 0},
    {"\xff"
     "23476",
     53894, 3, 1, 1, -1, false, 0, false, 0},
}};
inline constexpr auto bow_19_bindings = with_common(bow_19_own_bindings);
inline constexpr std::array<Requirement, 2> bow_19_conditions{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
}};
inline constexpr std::array<Requirement, 6> bow_19_requirements{{
    {RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
    {RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
    {RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 12},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 10},
    {RequirementKind::inventory_at_least, RequirementRole::resource, 16844, 0, 1},
}};
inline constexpr std::array<RequirementCheck, 6> bow_19_checks{{
    {{RequirementKind::integer_equal, RequirementRole::context, 12330, 0, 16},
     CheckSite::owner_condition,
     function_for(20929)},
    {{RequirementKind::integer_equal, RequirementRole::context, 20862, 0, 1},
     CheckSite::owner_condition,
     function_for(20929)},
    {{RequirementKind::integer_equal, RequirementRole::knowledge, 10484, 0, 1},
     CheckSite::owner_information,
     function_for(20870)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16610, 0, 12},
     CheckSite::callback,
     function_for(20930)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16599, 0, 10},
     CheckSite::callback,
     function_for(20930)},
    {{RequirementKind::inventory_at_least, RequirementRole::resource, 16844, 0, 1},
     CheckSite::callback,
     function_for(20930)},
}};
inline constexpr Owner bow_19_owner{20928,
                                    20,
                                    22243,
                                    false,
                                    false,
                                    true,
                                    function_for(20928),
                                    function_for(20929),
                                    function_for(20930),
                                    view(bow_19_conditions)};
inline constexpr std::array<ItemAmount, 3> bow_19_costs{{
    {16844, 1},
    {16610, 10},
    {16599, 12},
}};
inline constexpr std::array<ItemAmount, 1> bow_19_outputs{{
    {16845, 1},
}};
inline constexpr std::array<ItemAmount, 3> bow_19_minima{{
    {16610, 12},
    {16599, 10},
    {16844, 1},
}};
inline constexpr std::array<MaterialEffect, 4> bow_19_material{{
    {MaterialAction::remove, 16844, 1, 0},
    {MaterialAction::remove, 16610, 10, 0},
    {MaterialAction::remove, 16599, 12, 0},
    {MaterialAction::create, 16845, 1, 0},
}};
inline constexpr std::array<AmountOperand, 7> bow_19_operands{{
    {1, 1, 1, 0, false},
    {22, 10, 10, 0, false},
    {43, 12, 12, 0, false},
    {81, 1, 1, 0, false},
    {101, 10, 10, 0, false},
    {121, 12, 12, 0, false},
    {141, 1, 1, 0, false},
}};
inline constexpr std::array<OperandUse, 7> bow_19_uses{{
    {1, AmountRole::minimum_inventory, 16844},
    {22, AmountRole::minimum_inventory, 16599},
    {43, AmountRole::minimum_inventory, 16610},
    {81, AmountRole::remove_count, 16844},
    {101, AmountRole::remove_count, 16610},
    {121, AmountRole::remove_count, 16599},
    {141, AmountRole::create_count, 16845},
}};
inline constexpr std::array<ItemShape, 4> bow_19_items{{
    {16844, function_for(16844), 4, 1048576, 1048580, false},
    {16610, function_for(16610), 1, 2097152, 2097153, false},
    {16599, function_for(16599), 1, 2097152, 2097153, false},
    {16845, function_for(16845), 4, 1048576, 1048580, false},
}};
inline constexpr std::array<PrintFingerprint, 3> bow_19_prints{{
    {14374, 18, digest_literal("0e9aa2a1052f2804ac5c364d5cc64cba55b074bad9f0b2672e5dac7124ce033d")},
    {14389, 26, digest_literal("20d16aa1cdaa3f82c8fa45f0ca02dbddf0a308cafc99dc70f1857e9b78ccec02")},
    {53894, 56, digest_literal("7e0c63582b05c3807421324f5fb4b1413ed99d04f9513a515d8a3a0b2666866c")},
}};
inline constexpr std::array<Effect, 11> bow_19_success{{
    {EffectKind::minimum, 15, 160, 16844, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 10, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 12, Actor::hero},
    {EffectKind::remove, 85, 184, 16844, 1, Actor::hero},
    {EffectKind::remove, 105, 184, 16610, 10, Actor::hero},
    {EffectKind::remove, 125, 184, 16599, 12, Actor::hero},
    {EffectKind::create, 145, 170, 16845, 1, Actor::hero},
    {EffectKind::print, 155, 12, 53894, 0, Actor::hero},
    {EffectKind::say, 175, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Effect, 7> bow_19_failure{{
    {EffectKind::minimum, 15, 160, 16844, 1, Actor::hero},
    {EffectKind::minimum, 36, 160, 16599, 10, Actor::hero},
    {EffectKind::minimum, 57, 160, 16610, 12, Actor::hero},
    {EffectKind::print, 190, 12, 14374, 0, Actor::hero},
    {EffectKind::say, 210, 14646, -1, 0, Actor::self},
    {EffectKind::end_production, 215, 19965, -1, 0, Actor::hero},
    {EffectKind::integer_store, 230, 0, 20862, 0, Actor::hero},
}};
inline constexpr std::array<Descriptor, 20> recipes{{
    {RecipeId::bow_01,
     "Kurzbogen (verbessert)",
     view(bow_0_targets),
     view(bow_0_bindings),
     view(bow_0_requirements),
     view(bow_0_checks),
     bow_0_owner,
     {function_for(20873),
      20871,
      Invocation::info,
      1,
      1,
      2147483647,
      {232, function_for(20873)->body_sha256, bow_0_operands.data(), 7},
      view(bow_0_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_0_minima),
     view(bow_0_costs),
     view(bow_0_outputs),
     hand,
     &station,
     view(bow_0_uses),
     true,
     true,
     view(bow_0_success),
     view(bow_0_failure),
     view(bow_0_prints),
     view(bow_0_items)},
    {RecipeId::bow_02,
     "Weidenbogen (verbessert)",
     view(bow_1_targets),
     view(bow_1_bindings),
     view(bow_1_requirements),
     view(bow_1_checks),
     bow_1_owner,
     {function_for(20876),
      20874,
      Invocation::info,
      1,
      1,
      1073741823,
      {232, function_for(20876)->body_sha256, bow_1_operands.data(), 7},
      view(bow_1_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_1_minima),
     view(bow_1_costs),
     view(bow_1_outputs),
     hand,
     &station,
     view(bow_1_uses),
     true,
     true,
     view(bow_1_success),
     view(bow_1_failure),
     view(bow_1_prints),
     view(bow_1_items)},
    {RecipeId::bow_03,
     "Guter Bogen (verbessert)",
     view(bow_2_targets),
     view(bow_2_bindings),
     view(bow_2_requirements),
     view(bow_2_checks),
     bow_2_owner,
     {function_for(20879),
      20877,
      Invocation::info,
      1,
      1,
      715827882,
      {232, function_for(20879)->body_sha256, bow_2_operands.data(), 7},
      view(bow_2_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_2_minima),
     view(bow_2_costs),
     view(bow_2_outputs),
     hand,
     &station,
     view(bow_2_uses),
     true,
     true,
     view(bow_2_success),
     view(bow_2_failure),
     view(bow_2_prints),
     view(bow_2_items)},
    {RecipeId::bow_04,
     "Jagdbogen (verbessert)",
     view(bow_3_targets),
     view(bow_3_bindings),
     view(bow_3_requirements),
     view(bow_3_checks),
     bow_3_owner,
     {function_for(20882),
      20880,
      Invocation::info,
      1,
      1,
      536870911,
      {232, function_for(20882)->body_sha256, bow_3_operands.data(), 7},
      view(bow_3_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_3_minima),
     view(bow_3_costs),
     view(bow_3_outputs),
     hand,
     &station,
     view(bow_3_uses),
     true,
     true,
     view(bow_3_success),
     view(bow_3_failure),
     view(bow_3_prints),
     view(bow_3_items)},
    {RecipeId::bow_05,
     "Ulmenbogen (verbessert)",
     view(bow_4_targets),
     view(bow_4_bindings),
     view(bow_4_requirements),
     view(bow_4_checks),
     bow_4_owner,
     {function_for(20885),
      20883,
      Invocation::info,
      1,
      1,
      429496729,
      {232, function_for(20885)->body_sha256, bow_4_operands.data(), 7},
      view(bow_4_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_4_minima),
     view(bow_4_costs),
     view(bow_4_outputs),
     hand,
     &station,
     view(bow_4_uses),
     true,
     true,
     view(bow_4_success),
     view(bow_4_failure),
     view(bow_4_prints),
     view(bow_4_items)},
    {RecipeId::bow_06,
     "Kompositbogen (verbessert)",
     view(bow_5_targets),
     view(bow_5_bindings),
     view(bow_5_requirements),
     view(bow_5_checks),
     bow_5_owner,
     {function_for(20888),
      20886,
      Invocation::info,
      1,
      1,
      357913941,
      {232, function_for(20888)->body_sha256, bow_5_operands.data(), 7},
      view(bow_5_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_5_minima),
     view(bow_5_costs),
     view(bow_5_outputs),
     hand,
     &station,
     view(bow_5_uses),
     true,
     true,
     view(bow_5_success),
     view(bow_5_failure),
     view(bow_5_prints),
     view(bow_5_items)},
    {RecipeId::bow_07,
     "Eschenbogen (verbessert)",
     view(bow_6_targets),
     view(bow_6_bindings),
     view(bow_6_requirements),
     view(bow_6_checks),
     bow_6_owner,
     {function_for(20891),
      20889,
      Invocation::info,
      1,
      1,
      306783378,
      {232, function_for(20891)->body_sha256, bow_6_operands.data(), 7},
      view(bow_6_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_6_minima),
     view(bow_6_costs),
     view(bow_6_outputs),
     hand,
     &station,
     view(bow_6_uses),
     true,
     true,
     view(bow_6_success),
     view(bow_6_failure),
     view(bow_6_prints),
     view(bow_6_items)},
    {RecipeId::bow_08,
     "Langbogen (verbessert)",
     view(bow_7_targets),
     view(bow_7_bindings),
     view(bow_7_requirements),
     view(bow_7_checks),
     bow_7_owner,
     {function_for(20894),
      20892,
      Invocation::info,
      1,
      1,
      268435455,
      {232, function_for(20894)->body_sha256, bow_7_operands.data(), 7},
      view(bow_7_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_7_minima),
     view(bow_7_costs),
     view(bow_7_outputs),
     hand,
     &station,
     view(bow_7_uses),
     true,
     true,
     view(bow_7_success),
     view(bow_7_failure),
     view(bow_7_prints),
     view(bow_7_items)},
    {RecipeId::bow_09,
     "Buchenbogen (verbessert)",
     view(bow_8_targets),
     view(bow_8_bindings),
     view(bow_8_requirements),
     view(bow_8_checks),
     bow_8_owner,
     {function_for(20897),
      20895,
      Invocation::info,
      1,
      1,
      238609294,
      {232, function_for(20897)->body_sha256, bow_8_operands.data(), 7},
      view(bow_8_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_8_minima),
     view(bow_8_costs),
     view(bow_8_outputs),
     hand,
     &station,
     view(bow_8_uses),
     true,
     true,
     view(bow_8_success),
     view(bow_8_failure),
     view(bow_8_prints),
     view(bow_8_items)},
    {RecipeId::bow_10,
     "Knochenbogen (verbessert)",
     view(bow_9_targets),
     view(bow_9_bindings),
     view(bow_9_requirements),
     view(bow_9_checks),
     bow_9_owner,
     {function_for(20900),
      20898,
      Invocation::info,
      1,
      1,
      214748364,
      {232, function_for(20900)->body_sha256, bow_9_operands.data(), 7},
      view(bow_9_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_9_minima),
     view(bow_9_costs),
     view(bow_9_outputs),
     hand,
     &station,
     view(bow_9_uses),
     true,
     true,
     view(bow_9_success),
     view(bow_9_failure),
     view(bow_9_prints),
     view(bow_9_items)},
    {RecipeId::bow_11,
     "Eichenbogen (verbessert)",
     view(bow_10_targets),
     view(bow_10_bindings),
     view(bow_10_requirements),
     view(bow_10_checks),
     bow_10_owner,
     {function_for(20903),
      20901,
      Invocation::info,
      1,
      1,
      195225786,
      {232, function_for(20903)->body_sha256, bow_10_operands.data(), 7},
      view(bow_10_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_10_minima),
     view(bow_10_costs),
     view(bow_10_outputs),
     hand,
     &station,
     view(bow_10_uses),
     true,
     true,
     view(bow_10_success),
     view(bow_10_failure),
     view(bow_10_prints),
     view(bow_10_items)},
    {RecipeId::bow_12,
     "Kriegsbogen (verbessert)",
     view(bow_11_targets),
     view(bow_11_bindings),
     view(bow_11_requirements),
     view(bow_11_checks),
     bow_11_owner,
     {function_for(20906),
      20904,
      Invocation::info,
      1,
      1,
      178956970,
      {232, function_for(20906)->body_sha256, bow_11_operands.data(), 7},
      view(bow_11_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_11_minima),
     view(bow_11_costs),
     view(bow_11_outputs),
     hand,
     &station,
     view(bow_11_uses),
     true,
     true,
     view(bow_11_success),
     view(bow_11_failure),
     view(bow_11_prints),
     view(bow_11_items)},
    {RecipeId::bow_13,
     "Drachenbogen (verbessert)",
     view(bow_12_targets),
     view(bow_12_bindings),
     view(bow_12_requirements),
     view(bow_12_checks),
     bow_12_owner,
     {function_for(20909),
      20907,
      Invocation::info,
      1,
      1,
      165191049,
      {232, function_for(20909)->body_sha256, bow_12_operands.data(), 7},
      view(bow_12_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_12_minima),
     view(bow_12_costs),
     view(bow_12_outputs),
     hand,
     &station,
     view(bow_12_uses),
     true,
     true,
     view(bow_12_success),
     view(bow_12_failure),
     view(bow_12_prints),
     view(bow_12_items)},
    {RecipeId::bow_14,
     "Leichte Armbrust (verbessert)",
     view(bow_13_targets),
     view(bow_13_bindings),
     view(bow_13_requirements),
     view(bow_13_checks),
     bow_13_owner,
     {function_for(20912),
      20910,
      Invocation::info,
      1,
      1,
      1073741823,
      {232, function_for(20912)->body_sha256, bow_13_operands.data(), 7},
      view(bow_13_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_13_minima),
     view(bow_13_costs),
     view(bow_13_outputs),
     hand,
     &station,
     view(bow_13_uses),
     true,
     true,
     view(bow_13_success),
     view(bow_13_failure),
     view(bow_13_prints),
     view(bow_13_items)},
    {RecipeId::bow_15,
     "Jagdarmbrust (verbessert)",
     view(bow_14_targets),
     view(bow_14_bindings),
     view(bow_14_requirements),
     view(bow_14_checks),
     bow_14_owner,
     {function_for(20915),
      20913,
      Invocation::info,
      1,
      1,
      2147483647,
      {232, function_for(20915)->body_sha256, bow_14_operands.data(), 7},
      view(bow_14_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_14_minima),
     view(bow_14_costs),
     view(bow_14_outputs),
     hand,
     &station,
     view(bow_14_uses),
     true,
     true,
     view(bow_14_success),
     view(bow_14_failure),
     view(bow_14_prints),
     view(bow_14_items)},
    {RecipeId::bow_16,
     "Armbrust (verbessert)",
     view(bow_15_targets),
     view(bow_15_bindings),
     view(bow_15_requirements),
     view(bow_15_checks),
     bow_15_owner,
     {function_for(20918),
      20916,
      Invocation::info,
      1,
      1,
      715827882,
      {232, function_for(20918)->body_sha256, bow_15_operands.data(), 7},
      view(bow_15_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_15_minima),
     view(bow_15_costs),
     view(bow_15_outputs),
     hand,
     &station,
     view(bow_15_uses),
     true,
     true,
     view(bow_15_success),
     view(bow_15_failure),
     view(bow_15_prints),
     view(bow_15_items)},
    {RecipeId::bow_17,
     "Gute Armbrust (verbessert)",
     view(bow_16_targets),
     view(bow_16_bindings),
     view(bow_16_requirements),
     view(bow_16_checks),
     bow_16_owner,
     {function_for(20921),
      20919,
      Invocation::info,
      1,
      1,
      536870911,
      {232, function_for(20921)->body_sha256, bow_16_operands.data(), 7},
      view(bow_16_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_16_minima),
     view(bow_16_costs),
     view(bow_16_outputs),
     hand,
     &station,
     view(bow_16_uses),
     true,
     true,
     view(bow_16_success),
     view(bow_16_failure),
     view(bow_16_prints),
     view(bow_16_items)},
    {RecipeId::bow_18,
     "Kriegsarmbrust (verbessert)",
     view(bow_17_targets),
     view(bow_17_bindings),
     view(bow_17_requirements),
     view(bow_17_checks),
     bow_17_owner,
     {function_for(20924),
      20922,
      Invocation::info,
      1,
      1,
      357913941,
      {232, function_for(20924)->body_sha256, bow_17_operands.data(), 7},
      view(bow_17_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_17_minima),
     view(bow_17_costs),
     view(bow_17_outputs),
     hand,
     &station,
     view(bow_17_uses),
     true,
     true,
     view(bow_17_success),
     view(bow_17_failure),
     view(bow_17_prints),
     view(bow_17_items)},
    {RecipeId::bow_19,
     "Schwere Armbrust (verbessert)",
     view(bow_18_targets),
     view(bow_18_bindings),
     view(bow_18_requirements),
     view(bow_18_checks),
     bow_18_owner,
     {function_for(20927),
      20925,
      Invocation::info,
      1,
      1,
      268435455,
      {232, function_for(20927)->body_sha256, bow_18_operands.data(), 7},
      view(bow_18_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_18_minima),
     view(bow_18_costs),
     view(bow_18_outputs),
     hand,
     &station,
     view(bow_18_uses),
     true,
     true,
     view(bow_18_success),
     view(bow_18_failure),
     view(bow_18_prints),
     view(bow_18_items)},
    {RecipeId::bow_20,
     "Drachenjaegerarmbrust (verbessert)",
     view(bow_19_targets),
     view(bow_19_bindings),
     view(bow_19_requirements),
     view(bow_19_checks),
     bow_19_owner,
     {function_for(20930),
      20928,
      Invocation::info,
      1,
      1,
      178956970,
      {232, function_for(20930)->body_sha256, bow_19_operands.data(), 7},
      view(bow_19_material),
      {},
      true,
      true,
      false,
      true},
     view(bow_19_minima),
     view(bow_19_costs),
     view(bow_19_outputs),
     hand,
     &station,
     view(bow_19_uses),
     true,
     true,
     view(bow_19_success),
     view(bow_19_failure),
     view(bow_19_prints),
     view(bow_19_items)},
}};
inline const Descriptor* find(RecipeId id) noexcept {
  for (const auto& r : recipes)
    if (r.id == id)
      return &r;
  return nullptr;
}
inline std::uint32_t units(View<ItemAmount> values, std::int32_t symbol) noexcept {
  for (const auto& v : values)
    if (v.symbol == symbol)
      return v.units;
  return 0;
}
inline const SymbolBinding* binding_for(const Descriptor& d, std::uint32_t symbol) noexcept {
  if (!d.bindings.data || d.bindings.count > 64)
    return nullptr;
  for (const auto& b : d.bindings)
    if (b.symbol == symbol)
      return &b;
  return nullptr;
}
namespace detail {
namespace base = anvil_recipe_catalog::detail;
inline bool hand(const HandContract& a, const HandContract& b) noexcept {
  return a.left_empty == b.left_empty && a.right_empty == b.right_empty &&
         a.anchor_zero == b.anchor_zero &&
         a.static_animation_no_items == b.static_animation_no_items &&
         a.observed_in_world == b.observed_in_world;
}
inline bool station(const StationProfile* a, const StationProfile* b) noexcept {
  return a && b && a->production_context == b->production_context &&
         base::text(a->on_state, b->on_state) && base::text(a->scheme, b->scheme) &&
         base::text(a->visual, b->visual) && base::text(a->use_with_item, b->use_with_item) &&
         base::text(a->condition_name, b->condition_name) && a->state_num == b->state_num &&
         a->rewind == b->rewind && base::same_function(a->entry, b->entry) &&
         base::owner(a->end, b->end) && base::owner(a->category, b->category);
}
inline bool item(const ItemAmount& a, const ItemAmount& b) noexcept {
  return a.symbol == b.symbol && a.units == b.units;
}
}
// Valid own views; each copy remains bound to the entire same-indexed
// template. Minimum and consumption are explicitly NOT equated.
inline bool structurally_valid(const Descriptor& r) noexcept {
  namespace d = anvil_recipe_catalog::detail;
  const auto* e = find(r.id);
  if (!e)
    return false;
  if (!d::text(r.name, e->name) || !d::owner(r.owner, e->owner) ||
      !d::callback(r.callback, e->callback) || !detail::hand(r.hand, e->hand) ||
      !detail::station(r.station_profile, e->station_profile) ||
      r.require_self_is_hero != e->require_self_is_hero ||
      r.quantity_text_reviewed != e->quantity_text_reviewed)
    return false;
  return d::same(r.targets, e->targets,
                 [](const auto& a, const auto& b) {
                   return a.symbol == b.symbol && d::text(a.name, b.name);
                 }) &&
         d::same(r.bindings, e->bindings, d::same_binding) &&
         d::same(r.requirements, e->requirements, d::same_requirement) &&
         d::same(r.checks, e->checks,
                 [](const auto& a, const auto& b) {
                   return d::same_requirement(a.requirement, b.requirement) && a.site == b.site &&
                          d::same_function(a.function, b.function);
                 }) &&
         d::same(r.minima, e->minima, detail::item) && d::same(r.costs, e->costs, detail::item) &&
         d::same(r.outputs, e->outputs, detail::item) &&
         d::same(r.operand_uses, e->operand_uses,
                 [](const auto& a, const auto& b) {
                   return a.offset == b.offset && a.role == b.role && a.symbol == b.symbol;
                 }) &&
         d::same(r.success, e->success, d::effect) && d::same(r.failure, e->failure, d::effect) &&
         d::same(r.prints, e->prints,
                 [](const auto& a, const auto& b) {
                   return a.symbol == b.symbol && a.length == b.length && a.sha256 == b.sha256;
                 }) &&
         d::same(r.items, e->items, [](const auto& a, const auto& b) {
           return a.symbol == b.symbol && d::same_function(a.constructor, b.constructor) &&
                  a.mainflag == b.mainflag && a.script_flags == b.script_flags &&
                  a.native_flags == b.native_flags && a.constructor_calls == b.constructor_calls;
         });
}
struct QuantityPlan {
  const Callback* callback = nullptr;
  std::uint32_t quantity = 0;
  bool valid = false, patch_original = false;
};
inline QuantityPlan plan_quantity(const Descriptor& r, std::uint32_t quantity) noexcept {
  if (!quantity || !structurally_valid(r) || quantity > r.callback.maximum_quantity)
    return {};
  return {&r.callback, quantity, true, quantity > 1};
}
} // namespace ubc::bow_recipe_catalog
