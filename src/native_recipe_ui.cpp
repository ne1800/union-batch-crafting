// SPDX-License-Identifier: MIT
#include <windows.h>
#include <cstdio>
#include "ubc/native_recipe_ui.hpp"
#include "ubc/windows_probe.hpp"

namespace ubc {
using L = ui_text::Label;
namespace {
namespace m = native_menu;
namespace w = windows_probe;
void* pointer(std::uint32_t address) noexcept {
  std::uint32_t result=0;
  return w::copy_memory(nullptr,address,&result,4)?reinterpret_cast<void*>(result):nullptr;
}
class Text {
public:
  explicit Text(const char* value) {reinterpret_cast<m::StringConstructor>(m::string_ctor)(data_.bytes.data(),value);}
  ~Text(){reinterpret_cast<m::ObjectFunction>(m::string_dtor)(data_.bytes.data());}
  const void* get()const noexcept{return data_.bytes.data();}
private:m::StringStorage data_{};
};
void print(void* view,int y,const char* value,int x=350) {
  Text text(value);reinterpret_cast<m::ViewPrint>(m::view_print)(view,x,y,text.get());
}
bool shift() noexcept {
  auto* input=pointer(m::input_pointer);
  if(!input||!w::readable(input,4))return false;
  const auto table=m::read<std::uint32_t>(input,0);
  auto* function=pointer(table+4*m::key_pressed_slot);
  if(!w::readable(function,1,true))return false;
  const auto pressed=reinterpret_cast<m::KeyPressed>(function);
  return pressed(input,0x2a)||pressed(input,0x36);
}
RecipeMenuAction action(int key) noexcept {
  switch(key){
    case m::key_return:return RecipeMenuAction::confirm;
    case m::key_escape:return RecipeMenuAction::back;
    case m::key_left:return RecipeMenuAction::left;
    case m::key_right:return RecipeMenuAction::right;
    case m::key_up:return RecipeMenuAction::up;
    case m::key_down:return RecipeMenuAction::down;
    case 0x0f:return shift()?RecipeMenuAction::previous:RecipeMenuAction::next;
    case 0xc9:return RecipeMenuAction::page_up;
    case 0xd1:return RecipeMenuAction::page_down;
    default:break;
  }
  if(key<=0||key>0xffff)return RecipeMenuAction::none;
  auto* input=pointer(m::input_pointer);if(!input)return RecipeMenuAction::none;
  const auto logical=reinterpret_cast<m::LogicalKey>(m::input_logical_key)(input,static_cast<unsigned short>(key));
  switch(logical){
    case m::game_left:case m::game_strafe_left:return RecipeMenuAction::left;
    case m::game_right:case m::game_strafe_right:return RecipeMenuAction::right;
    case m::game_up:return RecipeMenuAction::up;
    case m::game_down:return RecipeMenuAction::down;
    default:return RecipeMenuAction::none;
  }
}
}
int NativeRecipeUi::handle(RecipeSession& session,int key) noexcept {
  if(failure_){
    if(!focused()){failure_input_.observe(0,false,false);return 1;}
    if(focused()&&(action(key)==RecipeMenuAction::back||action(key)==RecipeMenuAction::confirm)&&failure_input_.press())dismiss_=true;
    return 1;
  }
  if(!focused()){session.observe(0,false,false);return 1;}
  menu_.on_action(action(key),session);
  return 1; // Consume blocked actions exclusively as well.
}
bool NativeRecipeUi::render_failure() noexcept {
  if(!opened())return false;
  const auto& text=window_.texts();
  auto* view=reinterpret_cast<void*>(m::read<std::uint32_t>(object(),m::menu_inner_window));
  if(!w::readable(view,m::view_size)||m::read<std::int32_t>(view,m::view_id)!=m::view_item)return false;
  reinterpret_cast<m::ObjectFunction>(m::view_clear_printwin)(view);
  Text font("FONT_OLD_10_WHITE.TGA");reinterpret_cast<m::ViewSetFont>(m::view_set_font)(view,font.get());
  print(view,800,text[L::failure_title]);
  print(view,2000,text[L::uncertain_recipe]);
  print(view,2800,text[L::locked_session]);
  print(view,3600,text[L::quit_unsaved]);
  print(view,4400,text[L::diagnostic_log]);
  print(view,6200,text[L::dismiss_probe]);
  return true;
}
namespace {
const char* item_name(std::int32_t symbol,const ui_text::Table& text) noexcept {
  switch(symbol){
    case 16377:return text.item(16377);case 16379:return text.item(16379);
    case 16604:return text[L::flasks];case 16736:return text.item(16736);
    case 16714:return text[L::healing_plants];case 16716:return text[L::healing_herbs];
    case 16718:return text[L::healing_roots];case 16708:return text[L::fire_nettles];
    case 16710:return text[L::fire_weeds];case 16712:return text[L::fire_roots];
    case 16775:return text[L::healing_essences];case 16778:return text[L::healing_extracts];
    case 16781:return text[L::healing_elixirs];case 16766:return text[L::mana_essences];
    case 16769:return text[L::mana_extracts];case 16772:return text[L::mana_elixirs];
    case 16363:return text[L::raw_molerat_legs];case 16401:return text.item(16401);
    case 16395:return text.item(16395);case 16365:return text[L::molerat_dishes];
    case 16726:return text[L::dark_mushrooms];case 16361:return text[L::mushroom_dishes];
    default:return text[L::material];
  }
}
const char* blocked(RecipeBlockReason reason,const ui_text::Table& text) noexcept {
  switch(reason){
    case RecipeBlockReason::none:return "";
    case RecipeBlockReason::no_known_recipes:return text[L::station_no_recipes];
    case RecipeBlockReason::contract_unverified:return text[L::station_unavailable];
    case RecipeBlockReason::hand_unknown:return text[L::station_not_ready];
    case RecipeBlockReason::material_unknown:return text[L::wait_stock];
    case RecipeBlockReason::missing_material:return text[L::missing_material];
    case RecipeBlockReason::output_capacity:return text[L::output_full];
    case RecipeBlockReason::unavailable:return text[L::unavailable];
  }
  return text[L::unavailable];
}
}
bool NativeRecipeUi::render(const RecipeSession& session,bool execution_enabled) noexcept {
  if(!opened())return false;
  const auto& text=window_.texts();
  auto* view=reinterpret_cast<void*>(m::read<std::uint32_t>(object(),m::menu_inner_window));
  if(!w::readable(view,m::view_size)||m::read<std::int32_t>(view,m::view_id)!=m::view_item)return false;
  reinterpret_cast<m::ObjectFunction>(m::view_clear_printwin)(view);
  Text font("FONT_OLD_10_WHITE.TGA");reinterpret_cast<m::ViewSetFont>(m::view_set_font)(view,font.get());
  const auto row=menu_.row();const auto column=menu_.column();
  const auto mark=[&](RecipeMenuRow wanted){return row==wanted?">":" ";};
  const auto* selected=session.selected();char line[240]{};
  print(view,300,text[L::title]);
  print(view,800,row==RecipeMenuRow::recipe?text[L::selected_recipes]:text[L::known_recipes]);
  for(std::size_t i=0;i<session.visible_count();++i){
    const auto* entry=session.visible(i);if(!entry)return false;
    const auto* desc=recipe_catalog::find(entry->id);if(!desc)return false;
    std::snprintf(line,sizeof(line),"%s %s%s",selected&&selected->id==entry->id?">":" ",
      text.recipe(static_cast<std::uint32_t>(desc->id)),recipe_block_reason(*entry)==RecipeBlockReason::none?"":" *");
    print(view,1250+static_cast<int>(i)*410,line);
  }
  if(!session.visible_count())print(view,1300,text[L::no_recipes]);
  print(view,4800,text[L::marked_unavailable]);
  constexpr int right=3400;
  std::snprintf(line,sizeof(line),text[L::recipe_amount],mark(RecipeMenuRow::quantity),session.quantity(),session.maximum());
  print(view,850,line,right);
  print(view,1250,text[L::adjust],right);
  const char* labels[]={"1","5","10",text[L::maximum]};
  char presets[160]{};std::size_t used=0;
  for(unsigned i=0;i<4;++i){
    const bool enabled=RecipeMenu::preset_enabled(i,session.maximum());
    const int n=std::snprintf(presets+used,sizeof(presets)-used,"%s%s%s%s  ",
      row==RecipeMenuRow::presets&&column==i?">":"",enabled?"[":"(",labels[i],enabled?"]":")");
    if(n<0||static_cast<std::size_t>(n)>=sizeof(presets)-used)return false;
    used+=static_cast<std::size_t>(n);
  }
  print(view,1800,presets,right);
  std::snprintf(line,sizeof(line),"%s [-5]        %s [+5]",row==RecipeMenuRow::steps&&column==0?">":" ",row==RecipeMenuRow::steps&&column==1?">":" ");
  print(view,2250,line,right);
  int y=2950;
  if(selected)for(std::size_t i=0;i<selected->target_count;++i){
    const auto& material=selected->materials[i];
    if(!material.unit_cost&&!material.unit_output)continue;
    const auto amount=recipe_amount_preview(material,session.quantity()?session.quantity():1);
    if(!amount.available_known)std::snprintf(line,sizeof(line),text[L::item_unknown],item_name(material.symbol,text));
    else if(material.unit_cost){
      if(amount.missing>0)std::snprintf(line,sizeof(line),text[L::item_missing],item_name(material.symbol,text),
        static_cast<long long>(amount.available),static_cast<long long>(amount.required),static_cast<long long>(amount.missing));
      else std::snprintf(line,sizeof(line),"%s: %lld - %lld = %lld",item_name(material.symbol,text),
        static_cast<long long>(amount.available),static_cast<long long>(amount.required),static_cast<long long>(amount.remaining));
    }else std::snprintf(line,sizeof(line),text[L::item_after],item_name(material.symbol,text),
      static_cast<long long>(amount.produced),static_cast<long long>(amount.remaining));
    print(view,y,line,right);y+=400;
  }
  print(view,4800,blocked(session.reason(),text),right);
  std::snprintf(line,sizeof(line),"%s %s%s",mark(RecipeMenuRow::manufacture),
    execution_enabled?text[L::craft]:text[L::test_amount],session.can_confirm()?"":text[L::disabled_suffix]);
  print(view,5700,line,right);
  std::snprintf(line,sizeof(line),text[L::original_action],mark(RecipeMenuRow::original));print(view,6200,line,right);
  const char* status=text[L::recipe_controls];
  if(session.phase()==SelectionPhase::releasing||session.phase()==SelectionPhase::ready)
    status=text[L::release_craft];
  else if(session.phase()==SelectionPhase::original)status=text[L::release_back];
  else if(!session.armed())status=text[L::release_buttons];
  else if(session.issue()==SelectionIssue::stock_changed||session.issue()==SelectionIssue::offer_changed)
    status=text[L::choose_again];
  else if(session.issue()==SelectionIssue::focus_lost)status=text[L::reconfirm];
  print(view,7100,status);
  return true;
}
} // namespace ubc
