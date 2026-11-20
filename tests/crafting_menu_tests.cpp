// SPDX-License-Identifier: MIT
#include "ubc/ui/crafting_menu.hpp"
#define UBC_CRAFTING_FIXTURE_ONLY
#include "crafting_catalog_tests.cpp"
int main() {
  using namespace crafting_test;using Row=ubc::CraftingMenuRow;using Action=ubc::CraftingMenuAction;
  for(const auto kind:{Kind::stove,Kind::alchemy,Kind::leather,Kind::saw,Kind::forge,Kind::anvil}) {
    auto snapshot=example(kind);ubc::CraftingSession session;ubc::CraftingMenu menu;std::uint64_t frame=0;
    session.open(snapshot);check(menu.row()==Row::recipe,"menu starts on recipe selection");
    check(!menu.on_action(Action::down,session)&&!menu.on_action(Action::confirm,session),"held opening input neither changes recipe nor activates detail panel");
    neutral(session,frame);const auto total=session.visible_count();
    for(std::size_t i=0;i<total;++i) {
      const auto viewport=menu.viewport(session,4);
      check(viewport.count==(total<4?total:4)&&viewport.has_selection&&viewport.selected<viewport.count&&
        viewport.first+viewport.selected==session.selected_index(),"viewport always includes current visible selection within caller's line bound");
      if(i+1<total) {
        check(menu.on_action(Action::down,session)&&session.selected_index()==i+1,"D-pad down reaches every known row without entering controls");
        check(!menu.on_action(Action::confirm,session),"selection change cannot share held confirm");neutral(session,frame);
      }
    }
    check(!menu.on_action(Action::down,session),"last recipe does not wrap into first or controls");
    for(std::size_t i=total-1;i>0;--i) {
      check(menu.on_action(Action::up,session)&&session.selected_index()==i-1,"D-pad up walks all recipes in reverse");neutral(session,frame);
      const auto viewport=menu.viewport(session,4);
      check(viewport.first+viewport.selected==i-1&&viewport.selected<viewport.count,"reverse viewport tracks list head without underflow");
    }
    check(!menu.on_action(Action::up,session),"top of list is stable");
    check(menu.on_action(Action::confirm,session)&&menu.row()==Row::quantity,"first confirm opens amount panel only");
    check(!menu.on_action(Action::confirm,session),"held first confirm cannot advance to manufacture");neutral(session,frame);
    check(menu.on_action(Action::right,session)&&session.quantity()==2,"right adjusts one cycle");
    check(menu.on_action(Action::page_up,session)&&session.quantity()==7,"page up adjusts five cycles and clamps to maximum");
    check(menu.on_action(Action::page_down,session)&&session.quantity()==2,"page down subtracts five cycles");
    check(menu.on_action(Action::confirm,session)&&menu.row()==Row::manufacture,"second fresh confirm selects manufacture button");neutral(session,frame);
    check(menu.on_action(Action::confirm,session)&&session.phase()==ubc::SelectionPhase::releasing,"third fresh confirm creates only pending user intent");
    check(menu.on_action(Action::back,session)&&session.phase()==ubc::SelectionPhase::original&&!session.take(snapshot,true),
      "Back wins before handoff even while manufacture-confirm is held");
  }
  {
    auto snapshot=example(Kind::leather);ubc::CraftingSession session;ubc::CraftingMenu menu;std::uint64_t frame=0;
    session.open(snapshot);neutral(session,frame);
    for(unsigned i=0;i<8;++i){menu.on_action(Action::down,session);neutral(session,frame);menu.viewport(session,4);}
    auto view=menu.viewport(session,4);
    check(view.first==5&&view.count==4&&view.selected==3,"ninth leather recipe has explicit scrolled window five through eight");
    state(snapshot,0x1001).known=false;
    check(!session.refresh(snapshot)&&session.selected_index()==7,"removing earlier learned row remaps selection by identity");
    view=menu.viewport(session,4);check(view.first==4&&view.selected==3&&view.count==4,"shrinking known list clamps previous viewport");
    check(menu.viewport(session,0).count==0&&!menu.viewport(session,0).has_selection,"zero visible lines produce empty bounded viewport");
    view=menu.viewport(session,SIZE_MAX);check(view.first==0&&view.count==8&&view.selected==7,"huge caller capacity never overflows or creates padded rows");
    ++snapshot.key.generation;for(auto& s:snapshot.recipes)if(s.evaluated)s.generation=snapshot.key.generation;
    check(session.open(snapshot),"explicit later session generation starts a new selection");
    view=menu.viewport(session,4);check(view.first==0&&view.selected==0,"new generation resets viewport instead of reusing old scroll offset");
    menu.reset();check(menu.row()==Row::recipe&&menu.column()==0,"menu reset restores navigation state");
  }
  {
    auto snapshot=example();ubc::CraftingSession session;ubc::CraftingMenu menu;std::uint64_t frame=0;
    session.open(snapshot);neutral(session,frame);
    const Row wanted[]{Row::quantity,Row::presets,Row::presets,Row::presets,Row::steps,Row::steps,Row::manufacture,Row::original,Row::recipe};
    const unsigned columns[]{0,0,1,3,0,1,0,0,0};
    for(std::size_t i=0;i<9;++i)check(menu.on_action(Action::next,session)&&menu.row()==wanted[i]&&menu.column()==columns[i],
      "tab navigation skips disabled ten-preset but retains exact maximum");
    check(menu.on_action(Action::previous,session)&&menu.row()==Row::original,"reverse tab reaches original return control");
    check(menu.on_action(Action::back,session)&&menu.row()==Row::recipe&&session.phase()==ubc::SelectionPhase::selecting,
      "Back from controls returns to recipe list before closing selection");neutral(session,frame);
    menu.on_action(Action::next,session);menu.on_action(Action::next,session);
    check(menu.row()==Row::presets&&menu.column()==0,"first enabled preset is one");
    check(menu.on_action(Action::right,session)&&menu.column()==1&&menu.on_action(Action::confirm,session)&&session.quantity()==5,
      "five-preset is exact and consumes activation");neutral(session,frame);
    check(menu.on_action(Action::right,session)&&menu.column()==3&&menu.on_action(Action::confirm,session)&&session.quantity()==7,
      "right skips unavailable ten and reaches proven maximum");
  }
  for(unsigned mode=0;mode<4;++mode) {
    auto snapshot=example(Kind::saw);auto& selected=state(snapshot,0x2001);
    if(mode==0)selected.maximum=0;
    if(mode==1)selected.contract=false;
    if(mode==2)selected.actor_matches=false;
    if(mode==3)selected.material_verified=false;
    ubc::CraftingSession session;ubc::CraftingMenu menu;std::uint64_t frame=0;
    check(session.open(snapshot)&&session.visible_count()==5,"blocked known recipe stays in controller list");neutral(session,frame);
    if(mode==0||mode==3)check(!menu.on_action(Action::confirm,session)&&menu.row()==Row::recipe,"unknown or zero amount cannot enter quantity controls");
    else {
      check(menu.on_action(Action::confirm,session)&&menu.row()==Row::quantity,"passive numerical preview is accessible even without runtime permission");neutral(session,frame);
      check(!menu.on_action(Action::confirm,session)&&menu.row()==Row::quantity,"preview cannot move to manufacture without profile or actor gate");
    }
    neutral(session,frame);check(!session.can_confirm(),"menu navigation never supplies missing evidence");
  }
  {
    auto snapshot=example();for(auto& s:snapshot.recipes)s.known=false;
    ubc::CraftingSession session;ubc::CraftingMenu menu;std::uint64_t frame=0;
    session.open(snapshot);neutral(session,frame);check(!menu.viewport(session,4).count,"empty learned list has empty viewport");
    check(menu.on_action(Action::next,session)&&menu.row()==Row::original,"empty menu skips all amount/manufacture controls");
    check(menu.on_action(Action::confirm,session)&&session.phase()==ubc::SelectionPhase::original,"empty menu still supports explicit original return");
  }
  std::printf("crafting_menu_tests: %u checks, %u failures\n",checks,failures);
  return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
