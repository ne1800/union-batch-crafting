// SPDX-License-Identifier: MIT
#include <windows.h>
#include <cstdio>
#include "ubc/ui/native_selection_ui.hpp"
#include "native_ui_language.hpp"
#include "ubc/stations/bow/bow_menu_trace.hpp"
#include "ubc/engine/selection_abi.hpp"
#include "ubc/engine/windows_probe.hpp"

namespace ubc {
using L = ui_text::Label;
namespace {
namespace m = native_menu;
namespace w = windows_probe;
void* pointer(std::uintptr_t address) noexcept {
  std::uint32_t result=0;
  return w::copy_memory(nullptr,static_cast<std::uint32_t>(address),&result,4) ?
    reinterpret_cast<void*>(result) : nullptr;
}
std::uintptr_t input_method(void* input,std::size_t slot) noexcept {
  if(!input||!w::readable(input,4))return 0;
  const auto vtable=m::read<std::uint32_t>(input,0);
  if(slot>32 || vtable>UINT32_MAX-slot*4)return 0;
  const auto function=pointer(vtable+slot*4);
  return w::readable(function,1,true) ? reinterpret_cast<std::uintptr_t>(function) : 0;
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
int navigation(int key) noexcept {
  switch(key){case m::key_left:return -1;case m::key_right:return 1;
    case m::key_up:return -2;case m::key_down:return 2;default:break;}
  if(key<=0||key>0xffff)return 0;
  auto* input=pointer(m::input_pointer);if(!input)return 0;
  const auto logical=reinterpret_cast<m::LogicalKey>(m::input_logical_key)(input,static_cast<unsigned short>(key));
  switch(logical){case m::game_left:case m::game_strafe_left:return -1;
    case m::game_right:case m::game_strafe_right:return 1;
    case m::game_up:return -2;case m::game_down:return 2;default:return 0;}
}
}

bool NativeSelectionUi::focused() noexcept {
  DWORD process=0;const auto window=GetForegroundWindow();
  return window&&GetWindowThreadProcessId(window,&process)&&process==GetCurrentProcessId();
}
bool NativeSelectionUi::buttons_held(bool* known) noexcept {
  if(known)*known=false;
  auto* input=pointer(m::input_pointer);
  const auto method=input_method(input,m::key_pressed_slot);
  if(!method)return true;
  if(known)*known=true;
  for(const auto key:{m::key_return,m::key_escape,0x1d})
    if(reinterpret_cast<m::KeyPressed>(method)(input,key))return true;
  return false;
}
bool NativeSelectionUi::owns_input() const noexcept {
  return initialized_&&callback_is_top(static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(storage_.bytes.data())));
}
bool NativeSelectionUi::callback_is_top(std::uint32_t callback) noexcept {
  return callback && top_callback()==callback;
}
std::uint32_t NativeSelectionUi::top_callback() noexcept {
  const auto first=pointer(0xab6464);
  return first&&w::readable(first,8)?m::read<std::uint32_t>(first,0):0;
}
bool NativeSelectionUi::can_open(BowMenuTrace* trace) const noexcept {
  const bool screen=pointer(m::screen_pointer)!=nullptr;
  const bool input=input_method(pointer(m::input_pointer),m::key_pressed_slot)!=0;
  if(trace){
    trace->equal("ui.created",0,created_);
    trace->equal("ui.screen",1,screen);
    trace->equal("ui.input_method",1,input);
  }
  return !created_&&screen&&input;
}
bool NativeSelectionUi::open(BowMenuTrace* trace,BowMenuTrace* opened_trace) noexcept {
  if(!can_open(trace))return false;
  reinterpret_cast<m::ObjectFunction>(m::menu_ctor)(storage_.bytes.data());
  created_=true;
  const auto table=m::read<std::uint32_t>(object(),0);
  const auto count=m::read<std::int32_t>(object(),m::menu_item_count);
  if(trace){trace->equal("ui.vtable",m::menu_vtable,table);trace->equal("ui.item_count",0,count);}
  if(table!=m::menu_vtable||count!=0){close();return false;}
  m::write<std::int32_t>(object(),m::menu_default_in_game,-1);
  m::write<std::int32_t>(object(),m::menu_default_out_game,-1);
  m::write<std::int32_t>(object(),m::menu_main_selection,-1);
  reinterpret_cast<m::ObjectFunction>(selection_abi::menu_screen_init)(object());
  const auto initialized=m::read<std::int32_t>(object(),m::menu_screen_initialized);
  initialized_=initialized==1;
  row_=0;
  language_=ui_text::read_language();
  const auto wanted=reinterpret_cast<std::uintptr_t>(object());
  const auto top=top_callback();
  if(trace){trace->equal("ui.initialized",1,initialized);trace->equal("ui.input_owner",wanted,top);}
  if(!initialized_||wanted!=top){close();return false;}
  if(opened_trace){
    opened_trace->equal("ui.vtable",m::menu_vtable,table);
    opened_trace->equal("ui.item_count",0,count);
    opened_trace->equal("ui.initialized",1,initialized);
    opened_trace->equal("ui.input_owner",wanted,top);
  }
  return true;
}
void NativeSelectionUi::close() noexcept {
  if(!created_)return;
  if(initialized_)reinterpret_cast<m::ObjectFunction>(selection_abi::menu_screen_done)(object());
  initialized_=false;
  reinterpret_cast<m::ObjectFunction>(m::menu_dtor)(object());
  created_=false;
}
int NativeSelectionUi::handle(SelectionSession& session,int key) noexcept {
  // Do not remove or reorder callback nodes inside DoEvents.
  if(!focused()){session.observe(0,false,false);return 1;}
  if(key==m::key_return){
    if(row_==1)session.confirm();
    else if(row_==2)session.back();
    else session.preset(session.selected());
    return 1;
  }
  if(key==m::key_escape){session.back();return 1;}
  if(!session.armed()||session.phase()!=SelectionPhase::selecting)return 1;
  const auto move=navigation(key);
  if(move==2)row_=(row_+1)%3;
  else if(move==-2)row_=(row_+2)%3;
  else if(move && session.preview().count){
    const auto n=session.preview().count;
    session.select((session.selected()+(move>0?1:n-1))%n);
  }
  return 1;
}
bool NativeSelectionUi::render(const SelectionSession& session,OriginalRecipe recipe,bool execution_enabled) noexcept {
  if(!initialized_)return false;
  const auto& text=texts();
  auto* view=reinterpret_cast<void*>(m::read<std::uint32_t>(object(),m::menu_inner_window));
  if(!w::readable(view,m::view_size)||m::read<std::int32_t>(view,m::view_id)!=m::view_item)return false;
  reinterpret_cast<m::ObjectFunction>(m::view_clear_printwin)(view);
  Text font("FONT_OLD_10_WHITE.TGA");reinterpret_cast<m::ViewSetFont>(m::view_set_font)(view,font.get());
  print(view,450,text[L::title]);
  print(view,1000,recipe==OriginalRecipe::meat?text.recipe(0):text.item(16775));
  const auto& s=session.preview();
  if(!s.count||session.selected()>=s.count)return false;
  const auto quantity=s.offers[session.selected()].quantity;
  SelectionStock stock;stock.issue=StockIssue::none;stock.stored=s.stored;stock.reserved=s.reserved;stock.maximum=quantity;
  const auto preview=material_preview(recipe,stock,quantity);
  if(!preview.valid)return false;
  char line[200]{};
  std::snprintf(line,sizeof(line),text[L::original_amounts],row_==0?">":" ",quantity);
  print(view,1700,line);
  const char* names[]={text.item(16377),text.item(16379),text[L::flasks],text[L::healing_plants],text.item(16736),text[L::healing_potions]};
  int y=2400;
  for(std::size_t i=0;i<6;++i){
    if(!preview.consumed[i]&&!preview.produced[i])continue;
    if(preview.consumed[i])std::snprintf(line,sizeof(line),text[L::available],names[i],preview.available[i],preview.consumed[i],preview.remaining[i]);
    else std::snprintf(line,sizeof(line),text[L::after],names[i],preview.produced[i],preview.remaining[i]);
    print(view,y,line);y+=420;
  }
  std::snprintf(line,sizeof(line),"%s %s",row_==1?">":" ",execution_enabled?text[L::craft]:text[L::test_selection]);
  print(view,4700,line);
  std::snprintf(line,sizeof(line),text[L::original_action],row_==2?">":" ");
  print(view,5200,line);
  const char* status=text[L::selection_controls];
  if(session.phase()==SelectionPhase::releasing||session.phase()==SelectionPhase::ready)
    status=execution_enabled?text[L::release_craft]:text[L::release_test];
  else if(session.phase()==SelectionPhase::original)status=text[L::release_back];
  else if(!session.armed())status=text[L::release_buttons];
  else if(session.issue()==SelectionIssue::stock_changed||session.issue()==SelectionIssue::offer_changed)
    status=text[L::selection_changed];
  print(view,6200,status);
  return true;
}
} // namespace ubc
