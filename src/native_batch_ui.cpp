// SPDX-License-Identifier: MIT
#include <windows.h>
#include <cstdio>
#include "ubc/native_batch_ui.hpp"
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
void print(void* view,int y,const char* value) {
  Text text(value);reinterpret_cast<m::ViewPrint>(m::view_print)(view,450,y,text.get());
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
BatchMenuAction action(int key) noexcept {
  switch(key){
    case m::key_return:return BatchMenuAction::confirm;
    case m::key_escape:return BatchMenuAction::back;
    case m::key_left:return BatchMenuAction::left;
    case m::key_right:return BatchMenuAction::right;
    case m::key_up:return BatchMenuAction::up;
    case m::key_down:return BatchMenuAction::down;
    case 0x0f:return shift()?BatchMenuAction::previous:BatchMenuAction::next;
    case 0xc9:return BatchMenuAction::page_up;
    case 0xd1:return BatchMenuAction::page_down;
    default:break;
  }
  if(key<=0||key>0xffff)return BatchMenuAction::none;
  auto* input=pointer(m::input_pointer);if(!input)return BatchMenuAction::none;
  const auto logical=reinterpret_cast<m::LogicalKey>(m::input_logical_key)(input,static_cast<unsigned short>(key));
  switch(logical){
    case m::game_left:case m::game_strafe_left:return BatchMenuAction::left;
    case m::game_right:case m::game_strafe_right:return BatchMenuAction::right;
    case m::game_up:return BatchMenuAction::up;
    case m::game_down:return BatchMenuAction::down;
    default:return BatchMenuAction::none;
  }
}
}
int NativeBatchUi::handle(BatchSession& session,int key) noexcept {
  if(failure_){
    if(!focused()){failure_input_.observe(0,false,false);return 1;}
    if(focused()&&(action(key)==BatchMenuAction::back||action(key)==BatchMenuAction::confirm)&&failure_input_.press())dismiss_=true;
    return 1;
  }
  if(!focused()){session.observe(0,false,false);return 1;}
  menu_.on_action(action(key),session);
  return 1; // Consume blocked actions exclusively as well.
}
bool NativeBatchUi::render_failure() noexcept {
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
bool NativeBatchUi::render(const BatchSession& session,OriginalRecipe recipe,bool execution_enabled) noexcept {
  if(!opened())return false;
  const auto& text=window_.texts();
  auto* view=reinterpret_cast<void*>(m::read<std::uint32_t>(object(),m::menu_inner_window));
  if(!w::readable(view,m::view_size)||m::read<std::int32_t>(view,m::view_id)!=m::view_item)return false;
  reinterpret_cast<m::ObjectFunction>(m::view_clear_printwin)(view);
  Text font("FONT_OLD_10_WHITE.TGA");reinterpret_cast<m::ViewSetFont>(m::view_set_font)(view,font.get());
  const auto row=menu_.row();const auto column=menu_.column();
  const auto mark=[&](BatchMenuRow wanted){return row==wanted?">":" ";};
  char line[220]{};
  print(view,350,text[L::title]);
  std::snprintf(line,sizeof(line),"%s %s",mark(BatchMenuRow::recipe),recipe==OriginalRecipe::meat?text.recipe(0):text.item(16775));
  print(view,950,line);
  std::snprintf(line,sizeof(line),text[L::batch_amount],mark(BatchMenuRow::quantity),session.quantity(),session.maximum());
  print(view,1600,line);
  const char* labels[]={"1","5","10",text[L::maximum]};
  char presets[180]{};std::size_t used=0;
  for(unsigned i=0;i<4;++i){
    const bool enabled=BatchMenu::preset_enabled(i,session.maximum());
    const int written=std::snprintf(presets+used,sizeof(presets)-used,"%s%s%s%s    ",
      row==BatchMenuRow::presets&&column==i?"> ":"",enabled?"[":"(",labels[i],enabled?"]":text[L::disabled_preset]);
    if(written<0||static_cast<std::size_t>(written)>=sizeof(presets)-used)return false;
    used+=static_cast<std::size_t>(written);
  }
  print(view,2150,presets);
  std::snprintf(line,sizeof(line),"%s [-5]       %s [+5]",row==BatchMenuRow::steps&&column==0?">":" ",row==BatchMenuRow::steps&&column==1?">":" ");
  print(view,2650,line);
  const auto& snapshot=session.preview().selection;
  SelectionStock stock;stock.issue=StockIssue::none;stock.stored=snapshot.stored;stock.reserved=snapshot.reserved;stock.maximum=session.maximum();
  const auto preview=material_preview(recipe,stock,session.quantity());
  const char* names[]={text.item(16377),text.item(16379),text[L::flasks],text[L::healing_plants],text.item(16736),text[L::healing_potions]};
  if(session.quantity()&&!preview.valid)return false;
  int y=3400;
  if(preview.valid){
    for(std::size_t i=0;i<6;++i){
      if(!preview.consumed[i]&&!preview.produced[i])continue;
      if(preview.consumed[i])std::snprintf(line,sizeof(line),text[L::remaining],names[i],preview.available[i],preview.consumed[i],preview.remaining[i]);
      else std::snprintf(line,sizeof(line),text[L::after],names[i],preview.produced[i],preview.remaining[i]);
      print(view,y,line);y+=390;
    }
  }else print(view,y,text[L::not_enough]);
  std::snprintf(line,sizeof(line),"%s %s%s",mark(BatchMenuRow::manufacture),execution_enabled?text[L::craft]:text[L::test_only],session.maximum()?"":text[L::disabled_suffix]);
  print(view,5650,line);
  std::snprintf(line,sizeof(line),text[L::original_action],mark(BatchMenuRow::original));print(view,6200,line);
  const char* status=text[L::batch_controls];
  if(session.phase()==SelectionPhase::releasing||session.phase()==SelectionPhase::ready)
    status=execution_enabled?text[L::release_craft]:text[L::release_test];
  else if(session.phase()==SelectionPhase::original)status=text[L::release_back];
  else if(!session.armed())status=text[L::release_buttons];
  else if(session.issue()==SelectionIssue::stock_changed||session.issue()==SelectionIssue::offer_changed)
    status=text[L::choose_again];
  else if(session.issue()==SelectionIssue::focus_lost)status=text[L::reconfirm];
  print(view,7100,status);
  return true;
}
} // namespace ubc
