// SPDX-License-Identifier: MIT
#include "ubc/grind_recipe_navigation.hpp"
#include <algorithm>
#include <cstdio>

namespace n = ubc::grind_recipe_navigation;
namespace g = ubc::grind_recipe;
namespace {
unsigned checks = 0, failures = 0;
void check(bool ok, const char* name) { ++checks; if (!ok) { ++failures; std::fprintf(stderr,"FAIL %s\n",name); } }
g::Evidence evidence(const g::Descriptor& d) {
  g::Evidence e; e.callback=d.function.symbol; e.stable_boundary_verified=true;
  e.talent_known=e.guild_known=e.skill_known=true; e.talent=1; e.guild=d.guilds[0]; e.skill=99;
  e.stocks={g::Stock{d.input,INT32_MAX,true,true},g::Stock{d.output,0,true,true},g::Stock{16613,0,true,true}};
  return e;
}
}
int main() {
  std::array<n::InfoOffer,6> info{{{20937,20938,20939},{20957,20958,20959},
    {20945,20946,20947},{20948,20949,20950},{20951,20952,20953},{20954,20955,20956}}};
  for (std::size_t rotation=0;rotation<info.size();++rotation) {
    const auto page=n::identify_info(11,info.data(),info.size(),true);
    check(page.known&&page.owner_present&&page.page==n::Page::info,"frische INFO-Collection ist reihenfolgeunabhaengig");
    for (std::size_t at=0;at<info.size();++at) for(unsigned field=0;field<3;++field) {
      auto bad=info; if(field==0)++bad[at].owner;else if(field==1)++bad[at].condition;else ++bad[at].function;
      check(!n::identify_info(11,bad.data(),bad.size(),true).known,"falsches Owner-/Condition-/Function-Tupel");
    }
    std::rotate(info.begin(),info.begin()+1,info.end());
  }
  const n::InfoOffer end{20937,20938,20939}, owner{20957,20958,20959};
  const auto missing=n::identify_info(11,&end,1,true);
  check(missing.known&&!missing.owner_present,"INFO ohne Gildenowner bleibt Originalseite");
  check(!n::identify_info(11,&owner,1,true).known,"Ende fehlt");
  check(!n::identify_info(13,info.data(),info.size(),true).known,"fremde Produktion");
  check(!n::identify_info(11,info.data(),info.size(),false).known,"unverifizierte Collection");
  check(!n::identify_info(11,nullptr,1,true).known&&!n::identify_info(11,info.data(),7,true).known,"Null oder zu grosse Collection");
  auto duplicate=info;duplicate[2]=duplicate[0];check(!n::identify_info(11,duplicate.data(),duplicate.size(),true).known,"doppeltes INFO");

  std::array<std::uint32_t,11> pal{{20963,20965,20966,20967,20968,20969,20970,20971,20972,20973,20974}};
  std::array<std::uint32_t,13> sld{{20964,20975,20976,20977,20978,20979,20980,20981,20982,20983,20984,20985,20986}};
  const std::array<std::uint32_t,2> pal_category{{20960,20961}},sld_category{{20960,20962}};
  const std::uint32_t back=20960;
  const auto root=n::identify_info(11,info.data(),info.size(),true);
  const auto categories_pal=n::identify_choices(11,20957,pal_category.data(),pal_category.size(),true);
  const auto categories_sld=n::identify_choices(11,20957,sld_category.data(),sld_category.size(),true);
  const auto categories_empty=n::identify_choices(11,20957,&back,1,true);
  const auto pal_page=n::identify_choices(11,20957,pal.data(),pal.size(),true);
  const auto sld_page=n::identify_choices(11,20957,sld.data(),sld.size(),true);
  check(categories_pal.page==n::Page::categories&&categories_pal.category==20961,"richtige Miliz-Paladin-Kategorie");
  check(categories_sld.page==n::Page::categories&&categories_sld.category==20962,"richtige Soeldner-Drachenjaeger-Kategorie");
  check(categories_empty.known&&categories_empty.category==0,"ungelerntes Original zeigt nur Zurueck");
  for(std::size_t rotation=0;rotation<pal.size();++rotation){
    check(n::identify_choices(11,20957,pal.data(),pal.size(),true).page==n::Page::pal_mil,"alle zehn Paladinwaffen");
    for(std::size_t i=0;i<pal.size();++i){auto bad=pal;bad[i]=20975;check(!n::identify_choices(11,20957,bad.data(),bad.size(),true).known,"fremde Gildenchoice");}
    std::rotate(pal.begin(),pal.begin()+1,pal.end());
  }
  for(std::size_t rotation=0;rotation<sld.size();++rotation){
    check(n::identify_choices(11,20957,sld.data(),sld.size(),true).page==n::Page::sld_djg,"alle zwoelf Soeldnerwaffen");
    for(std::size_t i=0;i<sld.size();++i){auto bad=sld;bad[i]=20965;check(!n::identify_choices(11,20957,bad.data(),bad.size(),true).known,"fremde Paladinchoice");}
    std::rotate(sld.begin(),sld.begin()+1,sld.end());
  }
  for(const auto args:{std::pair<std::int32_t,std::uint32_t>{13,20957},{11,20368},{0,0}})
    check(!n::identify_choices(args.first,args.second,pal.data(),pal.size(),true).known,"Station oder Choiceowner falsch");
  check(!n::identify_choices(11,20957,pal.data(),pal.size(),false).known,"frische Choicecollection fehlt");
  check(!n::identify_choices(11,20957,pal.data(),pal.size()-1,true).known,"fehlende Waffenchoice");
  auto duplicates=pal;duplicates[2]=duplicates[1];check(!n::identify_choices(11,20957,duplicates.data(),duplicates.size(),true).known,"duplizierte Waffenchoice");
  const std::array<std::uint32_t,3> both{{20960,20961,20962}};
  check(!n::identify_choices(11,20957,both.data(),both.size(),true).known,"beide Gildenkategorien sind kein Originalzustand");

  const std::array<n::PageEvidence,6> pages{{root,categories_pal,categories_sld,categories_empty,pal_page,sld_page}};
  for(const auto& d:g::recipes)for(const auto& page:pages)for(const auto quantity:{1u,7u,static_cast<std::uint32_t>(INT32_MAX)}){
    auto e=evidence(d);const auto plan=n::make_plan(page,d,e,quantity);
    check(plan.valid&&n::valid_plan(plan,page,d,e),"kanonischer Navigation-und-Mengenplan");
    check(plan.amount.skill_after==100&&plan.amount.skill_increment==1,"vorhandene Skillgrenze bleibt erhalten");
    const auto destination=d.category==20961?n::Page::pal_mil:n::Page::sld_djg;
    const bool same_page=page.page==destination;
    const bool own_category=page.page==n::Page::categories&&page.category==d.category;
    const auto expected=same_page?1u:own_category?2u:page.page==n::Page::info?3u:4u;
    check(plan.count==expected,"kuerzester belegter Originalweg");
    if(expected==4)check(plan.steps[0].kind==n::Kind::back&&plan.steps[0].after==n::Page::info&&
      plan.steps[1].kind==n::Kind::open_owner&&plan.steps[1].invocation==n::Invocation::info,
      "BACK2 und BACK3 gehen zur INFO-Wurzel");
    unsigned effects=0;
    for(std::size_t i=0;i<plan.count;++i){const auto& step=plan.steps[i];effects+=step.kind==n::Kind::effect?1u:0u;
      check(step.owner==20957,"ein echter gemeinsamer INFO-Owner");
      if(step.kind==n::Kind::effect)check(step.function==d.function.symbol&&step.invocation==n::Invocation::choice&&
        step.after==n::Page::unknown,"Effekt ist Choice und beweist kein Stationsende");
    }
    check(effects==1,"genau ein Originaleffekt");
    for(std::size_t i=0;i<plan.count;++i)for(unsigned mutation=0;mutation<6;++mutation){auto changed=plan;
      switch(mutation){case 0:changed.steps[i].owner++;break;case 1:changed.steps[i].function++;break;
        case 2:changed.steps[i].invocation=changed.steps[i].invocation==n::Invocation::info?n::Invocation::choice:n::Invocation::info;break;
        case 3:changed.steps[i].kind=static_cast<n::Kind>(99);break;case 4:changed.steps[i].before=n::Page::unknown;break;
        case 5:changed.steps[i].after=static_cast<n::Page>(99);break;}
      check(!n::valid_plan(changed,page,d,e),"gefalschter Navigationsschritt");
    }
    for(unsigned mutation=0;mutation<10;++mutation){auto changed=plan;
      switch(mutation){case 0:changed.count=5;break;case 1:changed.target++;break;case 2:changed.amount.maximum--;break;
        case 3:changed.amount.skill_before++;break;case 4:changed.amount.skill_after++;break;
        case 5:changed.amount.after_callback[2].amount++;break;case 6:changed.amount.after_end[2].amount++;break;
        case 7:changed.amount.operands[0].offset=151;break;case 8:changed.amount.operands[2].reserved++;break;
        case 9:changed.amount.recipe=nullptr;break;}
      check(!n::valid_plan(changed,page,d,e),"Mengen-Skill-Lifecycle-Ticket bleibt kanonisch");
    }
    e.skill=98;check(!n::valid_plan(plan,page,d,e),"Skillwechsel verwirft alten Plan");
  }
  for(const auto& d:g::recipes){auto e=evidence(d);
    check(!n::make_plan(missing,d,e,1).valid,"fehlender echter INFO-Owner");
    check(!n::make_plan({},d,e,1).valid,"unbekannte Seite");
    check(!n::make_plan(root,d,e,0).valid&&!n::make_plan(root,d,e,UINT32_MAX).valid,"0 und Ueberlauf sind kein Auftrag");
    for(unsigned mutation=0;mutation<7;++mutation){auto bad=e;
      switch(mutation){case 0:bad.talent=0;break;case 1:bad.talent=2;break;case 2:bad.guild=0;break;
        case 3:bad.skill_known=false;break;case 4:bad.stable_boundary_verified=false;break;
        case 5:bad.stocks[0].amount=0;break;case 6:bad.stocks[2].amount=INT32_MAX;break;}
      check(!n::make_plan(root,d,bad,1).valid,"urspruengliche Wissens-Gilden-Materialgrenzen");
    }
  }
  std::printf("grind_recipe_navigation_tests: %u checks, %u failures\n",checks,failures);
  return failures?1:0;
}
