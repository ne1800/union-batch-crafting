// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/catalogs/lhiver_recipe_profile.hpp"
#include "ubc/engine/amount_patch.hpp"

// Static metadata plus pure quantity/progress calculation. No runtime admission,
// hand, item, end or writer evidence. No engine calls/writes. Installed
// DAT: 71a7875aa24029ea6367c69d9f8c2d6e8c18a92aae002cc292bd92e5c907fbb4.
namespace ubc::grind_recipe {
inline constexpr bool runtime_authorized=false;
inline constexpr std::uint32_t talent_symbol=10501,skill_symbol=10505,owner_symbol=20957;
inline constexpr std::int32_t lifecycle_item=16613;
struct Descriptor {lhiver::Function function;std::int32_t input,output;std::array<std::int32_t,2> guilds;std::uint32_t category;};
inline constexpr std::array<Descriptor,22> recipes{{
  {{"PC_GRIND_WEAPON_SCHWERT1_INFO",20965,816889,308,lhiver::digest_literal("10bb8e55aadf88bfb37face333e90399718d10bfbca4ecf0a0acb9e249509285")},16483,16484,{2,1},20961},
  {{"PC_GRIND_WEAPON_RUBINKLINGE_INFO",20966,817197,308,lhiver::digest_literal("6f79cfe4c016a7cf5e05e95475db6281100d6ee30409d98353c3c0f02dadf347")},16497,16498,{2,1},20961},
  {{"PC_GRIND_WEAPON_2H_PAL_SWORD_INFO",20967,817505,308,lhiver::digest_literal("daadb2a3675911e1f5376eef047ea430fd5df767b3ea8a5c1346bb37b4b4194e")},16457,16458,{2,1},20961},
  {{"PC_GRIND_WEAPON_1H_PAL_SWORD_INFO",20968,817813,308,lhiver::digest_literal("e579494c96a7966b7e08908e803d9a9af07a3e522f3178ff5ab7a5975516ad14")},16455,16456,{2,1},20961},
  {{"PC_GRIND_WEAPON_1H_BLESSED_01_INFO",20969,818121,308,lhiver::digest_literal("d8976407b21ad96b65d4b26e9bc745b0b3c0dd8d2bb7a9855ac8ce12b467c5b5")},16532,16533,{2,1},20961},
  {{"PC_GRIND_WEAPON_1H_BLESSED_02_INFO",20970,818429,308,lhiver::digest_literal("e780eca9992d70000df83efce68105ed3a2eba0d55bfd15554338a1a519e8fb6")},16534,16535,{2,1},20961},
  {{"PC_GRIND_WEAPON_1H_BLESSED_03_INFO",20971,818737,308,lhiver::digest_literal("1c911d2f2b7e981ab5062259786cfd34820722829ee59318e4ef8d732ad51b69")},16536,16537,{2,1},20961},
  {{"PC_GRIND_WEAPON_2H_BLESSED_01_INFO",20972,819045,308,lhiver::digest_literal("57bab3ce82f7862ebfa8539bba1645dc05f1c037a4fc05887aea5010e6a57f73")},16538,16539,{2,1},20961},
  {{"PC_GRIND_WEAPON_2H_BLESSED_02_INFO",20973,819353,308,lhiver::digest_literal("581a16408b26798e16e4ad28d24af28945d67f8481ecfe42bb4a9363f7d1ab87")},16540,16541,{2,1},20961},
  {{"PC_GRIND_WEAPON_2H_BLESSED_03_INFO",20974,819661,308,lhiver::digest_literal("967779151ef5e0427897b125cdaff606a3c3ecc1ff06754e9482e180a800098e")},16542,16543,{2,1},20961},
  {{"PC_GRIND_WEAPON_1H_SLD_SWORD_INFO",20975,819969,308,lhiver::digest_literal("be99d2726f2441aeacc88bd6c05ee3aa32ae0b051de684d2a4bf954b4397c0a9")},16450,16451,{7,6},20962},
  {{"PC_GRIND_WEAPON_2H_SLD_SWORD_INFO",20976,820277,308,lhiver::digest_literal("e7e2103e108f165a17c2030b7ce9a150e4cba2c4ecf1c7b07981d1d77beded7a")},16453,16454,{7,6},20962},
  {{"PC_GRIND_WEAPON_SCHWERT3_INFO",20977,820585,308,lhiver::digest_literal("0abbf1bced9f667e0d8945361e4f70e19927483f89d379556b78ccf25b5aa85a")},16489,16490,{7,6},20962},
  {{"PC_GRIND_WEAPON_ZWEIHAENDER2_INFO",20978,820893,308,lhiver::digest_literal("3ef172146186ff14ac7577820aad107a1178b8e5b2a6e9107b2d6959b532f881")},16500,16501,{7,6},20962},
  {{"PC_GRIND_WEAPON_1H_SPECIAL_01_INFO",20979,821201,308,lhiver::digest_literal("096e7e04c35df82fc154cc1c5b2d00d67ffc8cc217ed7c46595f045c6babc9fb")},16492,16493,{7,6},20962},
  {{"PC_GRIND_WEAPON_1H_SPECIAL_02_INFO",20980,821509,308,lhiver::digest_literal("5095db3edd7d8b4e71c39095384e276e075a094c1cfce06f90a0740d4043ee03")},16508,16509,{7,6},20962},
  {{"PC_GRIND_WEAPON_1H_SPECIAL_03_INFO",20981,821817,308,lhiver::digest_literal("d1a9be2fd3d1a98db0c728e136c0aa38eed639a8fc38a2b48062b3836f36bd4d")},16522,16523,{7,6},20962},
  {{"PC_GRIND_WEAPON_1H_SPECIAL_04_INFO",20982,822125,308,lhiver::digest_literal("5c436bbb87ce3085ce837a50281f2f6ed9fbbddc069c8850aada5d9d86e96f4e")},16528,16529,{7,6},20962},
  {{"PC_GRIND_WEAPON_2H_SPECIAL_01_INFO",20983,822433,308,lhiver::digest_literal("51c232770b38495e5535c2275a0f90e9c588c2a4399009b25d6c9f52986c3756")},16494,16495,{7,6},20962},
  {{"PC_GRIND_WEAPON_2H_SPECIAL_02_INFO",20984,822741,308,lhiver::digest_literal("5258dd4071c71bfd9f94f8bc6ae1013046bf689a807a3327b71ec9d60aac1022")},16510,16511,{7,6},20962},
  {{"PC_GRIND_WEAPON_2H_SPECIAL_03_INFO",20985,823049,308,lhiver::digest_literal("d9168305c4e632dce66948de404ede033e8f8dbc2d23ba873e2855039c9e35e3")},16524,16525,{7,6},20962},
  {{"PC_GRIND_WEAPON_2H_SPECIAL_04_INFO",20986,823357,308,lhiver::digest_literal("f03866d2935527bcb4423a92e47972a56764cb5088ef44e7553e3f15d5ec49f1")},16530,16531,{7,6},20962},
}};
inline const Descriptor* find(std::uint32_t callback) noexcept {
  for(const auto& r:recipes)if(r.function.symbol==callback)return &r;return nullptr;
}
// Own, already compiled metadata: name pointer remains canonical.
// The verifier reads no strings/descriptors supplied by the game.
inline bool structurally_valid(const Descriptor& r) noexcept {
  const auto* c=find(r.function.symbol);if(!c)return false;
  const auto& a=r.function;const auto& b=c->function;
  return a.name==b.name&&a.byte_offset==b.byte_offset&&a.byte_length==b.byte_length&&a.body_sha256==b.body_sha256&&
    a.argc==b.argc&&a.type==b.type&&a.flags==b.flags&&a.return_type==b.return_type&&a.parent_index==b.parent_index&&
    r.input==c->input&&r.output==c->output&&r.guilds==c->guilds&&r.category==c->category;
}
struct Stock {std::int32_t symbol=-1;std::int64_t amount=-1;bool known=false,unambiguous=false;};
struct Evidence {
  std::uint32_t callback=0;bool stable_boundary_verified=false;
  bool talent_known=false,guild_known=false,skill_known=false;
  std::int32_t talent=0,guild=0,skill=0;
  std::array<Stock,3> stocks{}; // Full inventory, without generated hand item.
};
enum class Issue {none,descriptor,observation,knowledge,guild,resources,lifecycle_room,quantity};
struct Plan {
  const Descriptor* recipe=nullptr;Issue issue=Issue::none;
  bool known=false,learned=false,guild_allowed=false,valid=false;
  std::uint32_t quantity=0,maximum=0,skill_increment=0;
  std::int32_t skill_before=0,skill_after=0;
  std::array<Stock,3> after_callback{},after_end{};
  std::array<AmountOperand,4> operands{};std::size_t count=0;
  // Use only directly with this live value copy. Copy/Move retains
  // no pointer into the operand list of another plan object.
  AmountBody body() const noexcept {
    return valid&&recipe&&count<=operands.size()?AmountBody{recipe->function.byte_length,recipe->function.body_sha256,operands.data(),count}:AmountBody{};
  }
};
inline Plan plan(const Descriptor& recipe,const Evidence& observed,std::uint32_t quantity) noexcept {
  Plan out;out.quantity=quantity;
  if(!structurally_valid(recipe)){out.issue=Issue::descriptor;return out;}
  out.recipe=find(recipe.function.symbol);
  if(observed.callback!=recipe.function.symbol||!observed.stable_boundary_verified||
     !observed.talent_known||!observed.guild_known||!observed.skill_known){out.issue=Issue::observation;return out;}
  out.learned=observed.talent==1;out.guild_allowed=observed.guild==recipe.guilds[0]||observed.guild==recipe.guilds[1];
  const std::array<std::int32_t,3> symbols{{recipe.input,recipe.output,lifecycle_item}};
  for(std::size_t i=0;i<symbols.size();++i){const Stock* selected=nullptr;
    for(const auto& s:observed.stocks)if(s.symbol==symbols[i]){if(selected){out.issue=Issue::resources;return out;}selected=&s;}
    if(!selected||!selected->known||selected->amount<0||selected->amount>INT32_MAX||(!i&&!selected->unambiguous)){
      out.issue=Issue::resources;return out;}
    out.after_callback[i]=out.after_end[i]=*selected;
  }
  out.known=true;out.skill_before=out.skill_after=observed.skill;
  const auto amount=static_cast<std::uint32_t>(out.after_end[0].amount);
  const auto room=static_cast<std::uint32_t>(INT32_MAX-out.after_end[1].amount);
  out.maximum=amount<room?amount:room;
  // The original callback ALWAYS outputs blade 16613 once. Only the
  // separately verified normal Destroy continuation debits 1 from
  // storage. No N-fold return or assumed inventory reserve on entry.
  if(out.after_end[2].amount==INT32_MAX){out.maximum=0;out.issue=Issue::lifecycle_room;return out;}
  if(!out.learned){out.issue=Issue::knowledge;return out;}
  if(!out.guild_allowed){out.issue=Issue::guild;return out;}
  if(!quantity)return out; // Known preview, no effective plan.
  if(quantity>out.maximum||quantity>INT32_MAX){out.issue=Issue::quantity;return out;}
  out.after_callback[0].amount-=quantity;out.after_end[0].amount-=quantity;
  out.after_callback[1].amount+=quantity;out.after_end[1].amount+=quantity;
  ++out.after_callback[2].amount;
  out.operands[0]={55,1,1,0,false};out.operands[1]={91,1,1,0,false};out.operands[2]={141,1,1,0,false};out.count=3;
  if(observed.skill<100){
    const auto remaining=static_cast<std::uint64_t>(std::int64_t{100}-observed.skill);
    out.skill_increment=static_cast<std::uint32_t>(remaining<quantity?remaining:quantity);
    out.skill_after=static_cast<std::int32_t>(std::int64_t{observed.skill}+out.skill_increment);
    // The unchanged comparison <100 is executed once. This positive
    // increment forms N original individual successes with Cap100.
    out.operands[3]={167,1,1,quantity-out.skill_increment,false};out.count=4;
  }
  // >=100: original +1 operand remains untouched/unreachable. The
  // comparison100(+151) and fixed lifecycle1(+293) are NEVER in the patch.
  out.valid=true;return out;
}
} // namespace ubc::grind_recipe
