// SPDX-License-Identifier: MIT
#include "ubc/stations/cool/cool_actor_preview.hpp"
#include "ubc/stations/cool/cool_direct_contract.hpp"
#define UBC_COOL_SNAPSHOT_FIXTURE_ONLY
#include "cool_recipe_snapshot_tests.cpp"

int main(){
  using Purpose=ubc::CoolActorPurpose;
  for(const auto self:{0u,0x3000u,hero}){
    CaptureFixture f;f.image.put(symbol(1679)+0x1c,self);
    const auto source=capture(f);const auto reads=f.image.reads;
    const auto preview=ubc::cool_actor_binding(source,Purpose::hero_preview,true);
    const auto entry=ubc::cool_actor_binding(source,Purpose::original_entry,true);
    check(source.recipe.actor_known&&source.recipe.actor_matches==(self==hero),"reader retains strict SELF==HERO evidence");
    check(source.recipe.material.known()&&source.recipe.material.maximum==5&&source.recipe.material.rows[0].stored==5,
      "reader uses physical HERO stock even with foreign or null SELF");
    check(preview.known&&preview.matches&&preview.purpose==Purpose::hero_preview,"positive physical HERO preview tolerates stable foreign SELF");
    check(entry.known&&entry.matches==(self==hero)&&entry.purpose==Purpose::original_entry,"preview flag cannot free original entry");
    check(!ubc::cool_actor_binding(source,Purpose::hero_preview).matches,"physical preview proof is explicit and required even if SELF matches");
    check(ubc::cool_actor_binding(source,Purpose::original_entry).matches==(self==hero),"original proof does not depend on preview flag");
    check(!ubc::cool_actor_binding(source,Purpose::unknown,true).known&&
      !ubc::cool_actor_binding(source,static_cast<Purpose>(99),true).matches,"unknown actor purpose fails closed");
    check(f.image.reads==reads&&source.recipe.actor_matches==(self==hero)&&source.diagnostic.actor_before.self==self,
      "actor projection neither reads nor changes raw capture");
    // Synthetic-only code/profile flags isolate the actor gate. The fixture
    // intentionally has no real Gothic script bodies and proves no native code.
    auto synthetic=source;synthetic.recipe.contract=true;
    synthetic.diagnostic.profile.valid=synthetic.diagnostic.profile.local.matches=true;
    const auto prepared=ubc::cool_direct_contract::prepare(synthetic,{true,true,true},2);
    check(prepared.valid==(self==hero),"direct writer prepare still rejects foreign SELF after positive preview");
    check(!source.recipe.contract&&!source.diagnostic.profile.valid,"preview never replaces failed canonical body verification");
  }
  CaptureFixture baseline;const auto first=capture(baseline);
  for(unsigned mutation=0;mutation<25;++mutation){auto changed=first;
    auto& b=changed.diagnostic.actor_before;auto& a=changed.diagnostic.actor_after;
    switch(mutation){
      case 0:changed.key.generation=0;break;case 1:changed.key.thread=0;break;
      case 2:changed.key.world=0;break;case 3:changed.key.hero=0;break;
      case 4:changed.key.station=0;break;case 5:changed.key.manager=0;break;
      case 6:changed.key.production_kind=12;break;
      case 7:changed.recipe.id=static_cast<ubc::cool_recipe_catalog::RecipeId>(0);break;
      case 8:changed.recipe.evaluated=false;break;case 9:changed.diagnostic.issue=ubc::CoolCaptureIssue::context;break;
      case 10:changed.recipe.actor_known=false;break;case 11:changed.diagnostic.actor_unchanged=false;break;
      case 12:b.read_valid=false;break;case 13:a.read_valid=false;break;
      case 14:b.issue=ubc::runtime_script::Issue::changed_during_read;break;
      case 15:a.issue=ubc::runtime_script::Issue::changed_during_read;break;
      case 16:b.requires_self=false;break;case 17:a.requires_self=false;break;
      case 18:a.hero=0x3000;break;case 19:a.self=0x3000;break;
      case 20:a.hero_symbol.offset=0x3000;break;case 21:a.self_symbol.offset=0x3000;break;
      case 22:++a.layout.count;break;
      case 23:changed.key.hero=0x3000;break;
      case 24:b.hero=a.hero=0x3000;break;
    }
    check(!ubc::cool_actor_binding(changed,Purpose::hero_preview,true).matches,"bad key metadata drift or HERO mismatch cannot be physical preview");
    check(!ubc::cool_actor_binding(changed,Purpose::original_entry,true).matches,"bad evidence remains rejected for writer even with preview flag");
  }
  for(unsigned mutation=0;mutation<3;++mutation){auto changed=first;
    switch(mutation){case 0:changed.recipe.actor_matches=false;break;
      case 1:changed.diagnostic.actor_before.matches=false;break;
      case 2:changed.diagnostic.actor_after.matches=false;break;}
    check(!ubc::cool_actor_binding(changed,Purpose::original_entry,true).matches,"all strict original actor matches remain mandatory");
  }
  for(const auto id:{1679u,1683u})for(unsigned mutation=0;mutation<6;++mutation){
    CaptureFixture f;
    switch(mutation){
      case 0:f.image.put(symbol(id)+0x20,station_test::metadata(1,7,1));break;
      case 1:f.image.put(symbol(id)+0x38,symbol(1584));break;
      case 2:f.image.at(name_at(id),1)[0]='X';break;
      case 3:f.image.denied=symbol(id)+0x1c;f.image.denied_size=4;break;
      case 4:f.image.change_address=symbol(id);f.image.change_target=symbol(id)+0x1c;f.image.change_on_read=2;break;
      case 5:f.image.change_address=symbol(id);f.image.change_target=symbol(id)+0x1c;f.image.change_on_read=3;break;
    }
    const auto source=capture(f);
    check(!ubc::cool_actor_binding(source,Purpose::hero_preview,true).matches,"bad or changing SELF/HERO is not hidden by physical actor projection");
    check(!ubc::cool_actor_binding(source,Purpose::original_entry,true).matches,"bad or changing symbol cannot enter original callback");
  }
  {CaptureFixture f;f.image.on_read=nested;const auto source=capture(f);
    check(ubc::cool_actor_binding(source,Purpose::hero_preview,true).matches,"reentrant capture cannot corrupt outer physical preview");}
  {CaptureFixture f;const auto source=capture(f);const auto reads=f.image.reads;
    allocation_probe::watching=true;
    const auto preview=ubc::cool_actor_binding(source,Purpose::hero_preview,true);
    allocation_probe::watching=false;
    check(preview.matches&&!allocation_probe::count&&f.image.reads==reads,"pure actor projection performs no heap allocation and no reads");}
  std::printf("cool_actor_preview_tests: %u checks, %u failures\n",station_test::checks,station_test::failures);
  return station_test::failures?1:0;
}
