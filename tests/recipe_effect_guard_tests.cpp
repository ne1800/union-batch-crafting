// SPDX-License-Identifier: MIT
// Dedicated Windows-x86 process with actual wrappers and readers. Use only
// our VirtualAlloc objects and original call counters. Global data slots and
// ReturnAddress are test seams. No engine calls, hook installation, game
// scripts or claims about the actual trampoline or address layout.
// The positive post-remove case injects only catalog and material evidence
// and focus for this headless host. The wrappers, selection reader, removal
// witness, registry, AmountPatch and restore remain real. The host's own
// synthetic 6-byte body is NOT game or engine evidence.
// Link: native_recipe_ui.cpp, native_selection_ui.cpp, frame_bridge.cpp,
// batch_call_bridge.cpp, kernel32 and user32.
#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <new>
#include "ubc/engine/selection_abi.hpp"
#include "ubc/engine/windows_probe.hpp"
#include "ubc/core/recipe_snapshot.hpp"
#include "ubc/core/recipe_handoff.hpp"
#include "ubc/ui/native_recipe_ui.hpp"

namespace recipe_guard_test {
std::uintptr_t caller = 0;
std::uint8_t* globals = nullptr;
std::uint8_t* manager_bytes = nullptr;
std::uint8_t* parser_bytes = nullptr;
bool inject_contracts = false;
ubc::CatalogueSnapshot injected_catalogue;
ubc::HandsSnapshot injected_hands;
std::uint32_t removed_node=0,removed_choice=0;
unsigned removed_reads=0;
void* return_address() noexcept { return reinterpret_cast<void*>(caller); }
}
namespace ubc {
class RecipeGuardCatalogueReader {
  RecipeCatalogueReader real_;
public:
  RecipeCatalogueCapture capture(const ReadOnlyMemory& memory,std::uint32_t parser,const StationKey& key,
      bool boundary,const std::array<bool,recipe_catalog::recipes.size()>& hands) noexcept {
    if(!recipe_guard_test::inject_contracts)return real_.capture(memory,parser,key,boundary,hands);
    RecipeCatalogueCapture out;out.snapshot=recipe_guard_test::injected_catalogue;out.hands=recipe_guard_test::injected_hands;
    out.snapshot.key=boundary?key:StationKey{};return out;
  }
};
class RecipeGuardUi:public NativeRecipeUi {
public:static bool focused() noexcept {return recipe_guard_test::inject_contracts||NativeRecipeUi::focused();}
};
namespace runtime_script {
inline RuntimeContract recipe_guard_verify_local_contract(const ReadOnlyMemory& memory,std::uint32_t parser,
    const lhiver::Profile& profile) noexcept {
  if(recipe_guard_test::inject_contracts)return {parser==lhiver::game_parser,Issue::none,0};
  return verify_local_contract(memory,parser,profile);
}
}
namespace recipe_catalog {
const Callback* recipe_guard_callback_for_quantity(const Descriptor&,std::uint32_t) noexcept;
}
}
namespace ubc::windows_probe {
bool recipe_guard_copy_memory(void* context, std::uint32_t value, void* out, std::size_t size) noexcept {
  if((recipe_guard_test::removed_node&&value==recipe_guard_test::removed_node)||
    (recipe_guard_test::removed_choice&&value==recipe_guard_test::removed_choice)){
    ++recipe_guard_test::removed_reads;return false;
  }
  const auto translate = [&](std::uint32_t base, std::size_t length, const std::uint8_t* target) {
    if (value < base || value - base > length || size > length - (value - base)) return false;
    value = target ? static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(target)) + value - base : 0;
    return true;
  };
  if (!translate(selection_abi::game_pointer, 4, recipe_guard_test::globals) &&
      !translate(station_abi::hero_pointer, 4, recipe_guard_test::globals ? recipe_guard_test::globals+4 : nullptr))
    if(!translate(station_abi::information_manager, 0x58, recipe_guard_test::manager_bytes))
      translate(station_abi::game_parser,0x58,recipe_guard_test::parser_bytes);
  return copy_memory(context,value,out,size);
}
}
#define _ReturnAddress() recipe_guard_test::return_address()
#define copy_memory recipe_guard_copy_memory
#define RecipeCatalogueReader RecipeGuardCatalogueReader
#define NativeRecipeUi RecipeGuardUi
#define verify_local_contract recipe_guard_verify_local_contract
#define callback_for_quantity recipe_guard_callback_for_quantity
#include "../src/probes/native_recipe_probe.cpp"
#undef callback_for_quantity
#undef verify_local_contract
#undef NativeRecipeUi
#undef RecipeCatalogueReader
#undef copy_memory
#undef _ReturnAddress

#if !defined(_M_IX86)
#error Dieser Test benoetigt Windows x86.
#endif

namespace ubc::recipe_catalog {
const Callback* recipe_guard_callback_for_quantity(const Descriptor& descriptor,std::uint32_t quantity) noexcept {
  const auto* original=callback_for_quantity(descriptor,quantity);
  if(!recipe_guard_test::inject_contracts||!original)return original;
  static Callback fixture;static lhiver::Function function;
  static constexpr std::array<std::uint8_t,6> code{{64,5,0,0,0,60}};
  static constexpr AmountOperand operand{1,5,1,0,false};
  Sha256 hash;hash.update(code.data(),code.size());
  fixture=*original;function=*original->function;function.byte_offset=0;function.byte_length=6;
  function.body_sha256=hash.finish();fixture.function=&function;
  fixture.amount={6,function.body_sha256,&operand,1};return &fixture;
}
}

extern "C" unsigned __cdecl UbcBatchPrepare(void*,int,std::uintptr_t);
extern "C" void* __cdecl UbcBatchWriter(void*,int);

namespace recipe_guard_test {
unsigned checks=0,failures=0,info_calls=0,choice_calls=0,input_calls=0,resolve_calls=0,clear_calls=0,add_calls=0;
void* seen_first=nullptr;
void* seen_second=nullptr;
void* resolve_result=nullptr;
DWORD seen_error=0;
constexpr DWORD incoming_error=0x1234, original_error=0x5678;
constexpr int original_input_result=73;
void check(bool value,const char* label) {
  ++checks;
  if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",label);}
}
std::uint32_t address(const void* value) noexcept {
  return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(value));
}
void put(void* object,std::size_t offset,std::uint32_t value) noexcept {
  std::memcpy(static_cast<std::uint8_t*>(object)+offset,&value,sizeof(value));
}
class PrivatePages {
public:
  static constexpr std::size_t size=0x40000;
  bool open() noexcept {
    for(auto& page:pages_){
      page=VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
      MEMORY_BASIC_INFORMATION region{};
      if(!page||VirtualQuery(page,&region,sizeof(region))!=sizeof(region)||
        region.AllocationBase!=page||region.Type!=MEM_PRIVATE)return false;
      if(address(page)<0xac0000u&&address(page)+size>0xaa0000u)return false;
    }
    globals=static_cast<std::uint8_t*>(pages_[0]);
    manager_bytes=static_cast<std::uint8_t*>(pages_[1]);return true;
  }
  bool protect_globals(DWORD protection) noexcept {
    DWORD old=0;return VirtualProtect(pages_[0],size,protection,&old)!=0;
  }
  void* fixture_space() noexcept {return static_cast<std::uint8_t*>(pages_[1])+0x1000;}
  ~PrivatePages(){
    for(auto* page:pages_)if(page)VirtualFree(page,0,MEM_RELEASE);
    globals=manager_bytes=parser_bytes=nullptr;
  }
private:
  std::array<void*,2> pages_{};
};
struct Fixture {
  alignas(4) std::array<std::uint8_t,0x5c> info{};
  alignas(4) std::array<std::uint8_t,0x18> choice{};
  alignas(4) std::array<std::uint8_t,8> node{};
  alignas(4) std::array<std::uint8_t,a::view_size> view{},second_view{};
  alignas(4) std::array<std::uint8_t,12> game{};
  alignas(4) std::array<std::uint8_t,0x968> hero{},other_hero{};
  alignas(4) std::array<std::uint8_t,0x204> station{};
  std::array<std::uint32_t,3> tokens{};
  alignas(4) std::array<std::uint8_t,0x58> parser{};
  alignas(4) std::array<std::uint32_t,24000> symbol_table{};
  alignas(4) std::array<std::array<std::uint8_t,0x3c>,8> symbols{};
  alignas(4) std::array<std::uint8_t,6> synthetic_body{{64,5,0,0,0,60}};
  alignas(4) std::array<std::uint8_t,8> station_node{};
  alignas(4) std::array<std::array<std::uint8_t,8>,3> extra_nodes{};
  alignas(4) std::array<std::array<std::uint8_t,0x18>,3> extra_choices{};
  alignas(4) std::array<std::uint8_t,0x48> position{};
  std::array<char,16> on_state{{'P','O','T','I','O','N','A','L','C','H','E','M','Y',0}};
  std::array<char,16> use_item{{'I','T','M','I','_','F','L','A','S','K',0}};
  ubc::HandoffObservation prepared;
  bool choice_mode=false;
  void* manager() const noexcept {return reinterpret_cast<void*>(s::information_manager);}
  void* callback() noexcept {return view.data()+a::view_callback;}
  bool reset(bool use_choice,bool pending=true) noexcept {
    inject_contracts=false;parser_bytes=nullptr;removed_node=removed_choice=0;removed_reads=0;
    post_remove_choice.clear();
    choice_mode=use_choice;
    info.fill(0);choice.fill(0);node.fill(0);view.fill(0);second_view.fill(0);
    game.fill(0);hero.fill(0);other_hero.fill(0);station.fill(0);
    std::memset(manager_bytes,0,0x58);
    const auto* descriptor=cat::find(use_choice?cat::RecipeId::health_01:cat::RecipeId::meat);
    const auto* target=cat::callback_for_quantity(*descriptor,1);
    const auto* owner=owner_for(target->owner_instance);
    put(info.data(),0,ubc::info_detail::info_vtable);put(info.data(),0x1c,static_cast<std::uint32_t>(owner->npc));
    put(info.data(),0x20,static_cast<std::uint32_t>(owner->number));
    put(info.data(),0x28,owner->condition->symbol);put(info.data(),0x2c,owner->information->symbol);
    put(info.data(),0x48,1);put(info.data(),0x50,static_cast<std::uint32_t>(owner->instance));
    if(use_choice){
      put(info.data(),0x58,address(node.data()));put(node.data(),0,address(choice.data()));
      put(choice.data(),0x14,target->function->symbol);
    }
    put(game.data(),0,a::game_vtable);put(game.data(),a::game_world,address(&tokens[0]));
    put(hero.data(),s::npc_interact_mob,address(station.data()));
    put(globals,0,address(game.data()));put(globals,4,address(hero.data()));
    put(manager_bytes,0x1c,address(view.data()));put(manager_bytes,0x20,address(hero.data()));
    put(manager_bytes,0x24,address(hero.data()));put(manager_bytes,0x28,address(info.data()));
    put(manager_bytes,0x40,1);put(manager_bytes,0x54,use_choice?2u:1u);
    put(view.data(),0,a::view_vtable);put(view.data(),0x24,0x83bc10);
    put(view.data(),a::view_callback,a::view_callback_vtable);
    put(view.data(),a::view_count,1);put(view.data(),a::view_selected,0);
    put(view.data(),a::view_opened,1);put(view.data(),a::view_active,1);second_view=view;
    InterlockedExchange(&installed,1);InterlockedExchange(&engine_thread,static_cast<LONG>(GetCurrentThreadId()));
    generation=1;frame=100;frame_depth=update_depth=info_depth=choice_depth=station_depth=collection_depth=0;
    native_update=false;disabled=false;private_enter=false;effect_authorized=effect_executed=recover_info=false;
    log_file=INVALID_HANDLE_VALUE;amount_diagnostic={};rows={};rows.generation=1;rows.view=address(view.data());
    choice_collection=false;choice_count=0;handoff={};dispatched_ticket={};result_guard={};
    info_calls=choice_calls=input_calls=resolve_calls=clear_calls=add_calls=0;seen_first=seen_second=nullptr;seen_error=0;
    resolve_result=info.data();
    if(!pending)return true;
    ubc::RecipeRequest request;
    request.id=descriptor->id;request.quantity=1;
    request.key={generation,GetCurrentThreadId(),address(&tokens[0]),address(hero.data()),address(station.data()),
      s::information_manager,descriptor->station};
    request.material_snapshot=ubc::make_recipe_state(request.id);
    auto& material=request.material_snapshot;
    material.known=material.contract=material.hand_known=material.material_known=true;material.maximum=4;
    for(std::size_t i=0;i<material.target_count;++i){
      auto& item=material.materials[i];item.stored=static_cast<std::int32_t>(item.unit_cost*4);item.reserved=0;
      if(descriptor->hand.from_inventory&&item.symbol==descriptor->hand.active_item_symbol){item.reserved=1;--item.stored;}
    }
    const auto plan=use_choice?nav::make_plan(nav::Page::health_01_choices,request.id,1):ubc::make_direct_info_plan(request);
    ubc::HandoffObservation initial;
    initial.key=request.key;initial.view=address(view.data());initial.frame=frame;initial.collection=1;
    initial.context_known=initial.page_known=initial.complete=initial.ready=true;
    initial.outer_frame=initial.quiescent=initial.focused=true;initial.page=plan.source;initial.mode=target->invocation;
    initial.choice_owner=use_choice?owner->instance:-1;initial.count=1;initial.materials=material;
    initial.hands.left={0,-1,-1,0};initial.hands.right={1,descriptor->hand.active_item_symbol,1,address(&tokens[1])};
    const ubc::HandoffOffer offer{true,target->invocation,target->owner_instance,static_cast<std::int32_t>(target->function->symbol),0,1};
    if(!handoff.begin(request,plan,initial))return false;
    const auto ticket=handoff.ticket(initial,offer);
    if(!ticket||!handoff.dispatch(*ticket,initial,offer))return false;
    dispatched_ticket=*ticket;prepared=initial;return true;
  }
  bool effect_returned() noexcept {
    auto actual=prepared;actual.outer_frame=actual.quiescent=actual.ready=false;
    const ubc::HandoffCallbackProof proof{true,true,true,1,1,0,0,0};
    const auto witness=handoff.enter_callback(dispatched_ticket,actual,dispatched_ticket.offer,proof);
    return witness.decision==ubc::HandoffDecision::execute&&handoff.callback_return(witness);
  }
  void foreign_dialogue() noexcept {
    put(manager_bytes,0x20,address(other_hero.data()));put(manager_bytes,0x24,address(hero.data()));
  }
  void foreign_target() noexcept {
    put(info.data(),0x50,90000);put(info.data(),0x2c,90001);
    if(choice_mode)put(choice.data(),0x14,90002);
  }
  void new_view() noexcept {put(manager_bytes,0x1c,address(second_view.data()));}
  bool quantity_two_with_neighbours() noexcept {
    auto request=handoff.request();request.quantity=2;
    const auto* descriptor=cat::find(request.id);const auto* target=cat::callback_for_quantity(*descriptor,2);
    if(!target)return false;
    put(choice.data(),0x14,target->function->symbol);
    for(std::size_t i=0;i<2;++i){extra_nodes[i].fill(0);extra_choices[i].fill(0);
      put(extra_nodes[i].data(),0,address(extra_choices[i].data()));put(extra_choices[i].data(),0x14,40001u+static_cast<std::uint32_t>(i));}
    put(info.data(),0x58,address(extra_nodes[0].data()));put(extra_nodes[0].data(),4,address(node.data()));
    put(node.data(),4,address(extra_nodes[1].data()));
    put(view.data(),a::view_count,3);put(view.data(),a::view_selected,1);second_view=view;
    prepared.count=3;
    handoff={};const auto plan=nav::make_plan(prepared.page,request.id,2);
    const ubc::HandoffOffer offer{true,cat::Invocation::choice,target->owner_instance,
      static_cast<std::int32_t>(target->function->symbol),1,1};
    if(!handoff.begin(request,plan,prepared))return false;
    const auto ticket=handoff.ticket(prepared,offer);
    if(!ticket||!handoff.dispatch(*ticket,prepared,offer))return false;
    dispatched_ticket=*ticket;return true;
  }
  bool native_phase(bool info_mode=false) noexcept {
    const auto request=handoff.request();
    inject_contracts=true;injected_catalogue={};injected_hands=prepared.hands;handed_hands=prepared.hands;
    injected_catalogue.recipes[0]=request.material_snapshot;
    parser_bytes=parser.data();parser.fill(0);symbol_table.fill(0);for(auto& symbol:symbols)symbol.fill(0);
    synthetic_body={{64,5,0,0,0,60}};
    const auto* target=cat::callback_for_quantity(*cat::find(request.id),request.quantity);
    put(parser.data(),0x18,address(symbol_table.data()));put(parser.data(),0x1c,static_cast<std::uint32_t>(symbol_table.size()));
    put(parser.data(),0x20,static_cast<std::uint32_t>(symbol_table.size()));put(parser.data(),0x48,address(synthetic_body.data()));
    put(parser.data(),0x50,address(synthetic_body.data())+6);put(parser.data(),0x54,6);
    const auto scalar=[&](std::uint32_t index,std::size_t slot,std::uint32_t value){
      symbol_table[index]=address(symbols[slot].data());put(symbols[slot].data(),0x20,1|(2u<<12u));
      put(symbols[slot].data(),0x18,value);
    };
    symbol_table[ubc::lhiver::hero_symbol]=address(symbols[0].data());put(symbols[0].data(),0x1c,address(hero.data()));
    scalar(ubc::lhiver::production_symbol,1,4);
    for(std::size_t i=0;i<nav::flag_bindings.size();++i)scalar(nav::flag_bindings[i].symbol,i+2,i==0?1u:0u);
    symbol_table[target->function->symbol]=address(symbols[7].data());
    put(symbols[7].data(),8,address(target->function->name));
    put(symbols[7].data(),0xc,static_cast<std::uint32_t>(std::strlen(target->function->name)));
    put(symbols[7].data(),0x20,(5u<<12u)|(1u<<16u));
    put(station.data(),0x18c,address(station_node.data()));put(station_node.data(),0,address(position.data()));
    put(position.data(),0x44,address(hero.data()));
    for(const auto offset:{0x1f4u,0x1f8u,0x1fcu})put(station.data(),offset,1);
    put(station.data(),0x1e8,address(on_state.data()));put(station.data(),0x1ec,13);
    put(station.data(),0x1ac,address(use_item.data()));put(station.data(),0x1b0,10);
    put(view.data(),a::view_done,1);put(view.data(),a::view_active,0);
    put(view.data(),a::view_opened,0);put(view.data(),a::view_closed,1);put(manager_bytes,0x40,0);
    choice_collection=true;choice_count=3;choice_functions[0]=40001;
    choice_functions[1]=dispatched_ticket.offer.function;choice_functions[2]=40002;
    if(info_mode){
      put(manager_bytes,0x54,1);choice_collection=false;choice_count=0;put(view.data(),a::view_count,1);
      rows.valid=true;rows.count=1;rows.rows[0]=ubc::read_selection_info(memory,address(info.data()));
    }
    native_update=true;update_depth=1;caller=a::choice_effect_return;
    return context().valid&&complete_collection(context());
  }
  bool owner_phase(const nav::RecipeRoute& route) noexcept {
    if(!reset(true))return false;
    const auto* descriptor=cat::find(route.recipe);const auto& owner=*route.owner;
    auto request=handoff.request();request.id=route.recipe;request.material_snapshot=ubc::make_recipe_state(request.id);
    auto& material=request.material_snapshot;material.known=material.contract=material.hand_known=material.material_known=true;material.maximum=4;
    for(std::size_t i=0;i<material.target_count;++i){
      auto& item=material.materials[i];item.stored=static_cast<std::int32_t>(item.unit_cost*4);item.reserved=0;
      if(descriptor->hand.from_inventory&&item.symbol==descriptor->hand.active_item_symbol){item.reserved=1;--item.stored;}
    }
    put(info.data(),0x1c,static_cast<std::uint32_t>(owner.npc));put(info.data(),0x20,static_cast<std::uint32_t>(owner.number));
    put(info.data(),0x28,owner.condition->symbol);put(info.data(),0x2c,owner.information->symbol);put(info.data(),0x50,static_cast<std::uint32_t>(owner.instance));
    // Our three remaining objects after an earlier quantity-5 choice: 1, 3, Zurueck.
    put(info.data(),0x58,address(node.data()));put(node.data(),0,address(choice.data()));put(node.data(),4,address(extra_nodes[0].data()));
    put(choice.data(),0x14,static_cast<std::uint32_t>(route.original_choice_symbols[3]));
    for(std::size_t i=0;i<2;++i){
      extra_nodes[i].fill(0);extra_choices[i].fill(0);put(extra_nodes[i].data(),0,address(extra_choices[i].data()));
      put(extra_nodes[i].data(),4,i?0:address(extra_nodes[1].data()));
      put(extra_choices[i].data(),0x14,static_cast<std::uint32_t>(route.original_choice_symbols[i?0:2]));
    }
    prepared.materials=material;prepared.page=route.category;prepared.mode=cat::Invocation::info;prepared.choice_owner=-1;prepared.count=1;
    handoff={};dispatched_ticket={};
    if(!handoff.begin(request,nav::make_plan(route.category,route.recipe,1),prepared)||!native_phase(true))return false;
    const auto category_flag=route.category==nav::Page::health_info?0u:1u;
    for(std::size_t i=0;i<nav::flag_bindings.size();++i)put(symbols[i+2].data(),0x18,i==category_flag?1u:0u);
    put(view.data(),a::view_done,0);put(view.data(),a::view_opened,1);put(view.data(),a::view_closed,0);put(view.data(),a::view_active,1);
    put(manager_bytes,0x40,1);frame_depth=1;update_depth=0;native_update=false;choice_mode=false;
    return context().valid&&page(context())==route.category&&complete_collection(context());
  }
};

Fixture* removal_fixture=nullptr;
unsigned removal_mode=0,script_calls=0,writer_routes=0;
void* __cdecl synthetic_script(void* parser,int symbol){
  ++script_calls;
  check(parser==reinterpret_cast<void*>(s::game_parser)&&symbol==dispatched_ticket.offer.function,
    "synthetic script receives exactly the selected parser/function pair");
  std::uint32_t quantity=0;std::memcpy(&quantity,removal_fixture->synthetic_body.data()+1,4);
  check(quantity==2&&bc::diagnostics().writer,"real amount patch is present only under actual exclusive registry writer");
  SetLastError(original_error);return nullptr;
}
void __fastcall removing_original_choice(void* manager,void* selected){
  ++choice_calls;auto& f=*removal_fixture;
  check(manager==f.manager()&&selected==f.choice.data(),"real wrapper forwards its bound original Choice arguments once");
  if(removal_mode!=1){
    if(removal_mode==2){put(f.info.data(),0x58,address(f.node.data()));removed_node=address(f.extra_nodes[0].data());removed_choice=address(f.extra_choices[0].data());}
    else{
      put(f.extra_nodes[0].data(),4,removal_mode==3?0:address(f.extra_nodes[1].data()));
      removed_node=address(f.node.data());removed_choice=address(f.choice.data());
    }
  }
  if(removal_mode==4)++rows.generation;
  if(removal_mode==5)put(f.info.data(),0x50,90000);
  if(removal_mode==6)f.new_view();
  check(complete_collection(context())==(removal_mode==1),
    "only missing removal retains ordinary complete-collection proof; real removal has old view count");
  const auto route=UbcBatchPrepare(reinterpret_cast<void*>(s::game_parser),dispatched_ticket.offer.function,bc::choice_call_return);
  if(route==2){++writer_routes;UbcBatchWriter(reinterpret_cast<void*>(s::game_parser),dispatched_ticket.offer.function);}
  else check(false,"canonical selected callback enters actual writer registry route");
}

void post_remove_wrapper_cases(Fixture& f){
  for(removal_mode=0;removal_mode<7;++removal_mode){
    check(f.reset(true)&&f.quantity_two_with_neighbours()&&f.native_phase(),"own native wrapper fixture supplies a real bounded three-choice collection");
    removal_fixture=&f;script_calls=writer_routes=0;
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);
    bc::Config config;config.parser=reinterpret_cast<void*>(s::game_parser);config.symbol_count=1;
    config.symbols[0]=dispatched_ticket.offer.function;config.signature=batch_signature;
    config.select=select_batch_call;config.writer=execute_amount;
    check(bc::configure(config),"actual registry is configured only for the selected owned test symbol");
    original_choice=removing_original_choice;
    observe_choice(f.manager(),f.choice.data());
    check(choice_calls==1&&writer_routes==1,"one original wrapper produces one selected writer attempt");
    check(script_calls==(removal_mode==0?1u:0u),"only exact native post-removal state reaches one real amount-patch/original call");
    check(!post_remove_choice.active()&&!choice_depth&&update_depth==1&&!effect_authorized,
      "callback return retires witness and restores wrapper scope");
    check(!removed_reads,"native post-removal path never reads the removed node or Choice object");
    const std::array<std::uint8_t,6> original{{64,5,0,0,0,60}};
    check(f.synthetic_body==original&&!amount_patch.active()&&!bc::diagnostics().writer&&!bc::diagnostics().writes_disabled,
      "original own bytes and writer lease are restored after allowed or rejected attempt");
    check(result_guard.state()==(removal_mode==0?ubc::RecipeResultState::waiting:ubc::RecipeResultState::failed),
      "actual effect waits for result; pre-effect rejection latches failure instead of claiming material completion");
    check(handoff.state()==ubc::HandoffState::result_pending&&!result_guard.permits_next(),
      "native return cannot implicitly replay or approve another effect");
  }
  inject_contracts=false;removed_node=removed_choice=0;parser_bytes=nullptr;
}

void __thiscall counted_info(void* object){
  ++info_calls;seen_first=object;seen_error=GetLastError();SetLastError(original_error);
}
void __fastcall counted_choice(void* manager,void* selected){
  ++choice_calls;seen_first=manager;seen_second=selected;seen_error=GetLastError();SetLastError(original_error);
}
int __thiscall counted_input(void* callback,int input){
  ++input_calls;seen_first=callback;seen_second=reinterpret_cast<void*>(static_cast<std::uintptr_t>(input));
  return original_input_result;
}
void* __thiscall counted_resolve(void* manager,void* player,void*,int){
  ++resolve_calls;seen_first=manager;seen_second=player;seen_error=GetLastError();
  SetLastError(original_error);return resolve_result;
}
void __fastcall counted_clear(void* view,void*){
  ++clear_calls;put(view,a::view_count,0);put(view,a::view_selected,UINT32_MAX);
}
void __fastcall counted_add(void* view,void*,int){
  ++add_calls;const auto before=m::read<std::uint32_t>(view,a::view_count);
  put(view,a::view_count,before+1);if(!before)put(view,a::view_selected,0);
}
Fixture* owner_fixture=nullptr;
const nav::RecipeRoute* owner_route=nullptr;
void __thiscall rebuilding_owner_info(void* object){
  ++info_calls;seen_first=object;seen_error=GetLastError();auto& f=*owner_fixture;
  check(object==f.info.data()&&handoff.state()==ubc::HandoffState::executing&&handoff.step()->kind==nav::Kind::open_owner,
    "real Info wrapper enters exactly its selected rebuilding owner");
  check(!effect_authorized&&!effect_executed&&!amount_patch.active(),"owner navigation never authorizes amount mutation or claims a recipe effect");
  // Synthetic original call counter. Models only Clear/AddChoices in
  // our own objects. No script, engine call or claim about the world.
  put(f.info.data(),0x58,0);
  const std::array<void*,4> nodes{{f.node.data(),f.extra_nodes[0].data(),f.extra_nodes[1].data(),f.extra_nodes[2].data()}};
  const std::array<void*,4> choices{{f.choice.data(),f.extra_choices[0].data(),f.extra_choices[1].data(),f.extra_choices[2].data()}};
  for(std::size_t i=0;i<nodes.size();++i){
    put(nodes[i],0,address(choices[i]));put(nodes[i],4,i+1<nodes.size()?address(nodes[i+1]):0);
    put(choices[i],0x14,static_cast<std::uint32_t>(owner_route->original_choice_symbols[3-i]));
  }
  put(f.info.data(),0x58,address(nodes[0]));SetLastError(original_error);
}
bool dispatch_owner(Fixture& f) {
  const auto c=context();const auto current=observation(c,true);const auto offer=resolve_offer(c,handoff.step());
  check(current.complete&&current.ready&&current.page_known&&offer.valid&&offer.row==0,
    "actual native row resolver recognizes the repeated owner despite three residual choices");
  const auto ticket=handoff.ticket(current,offer);
  const bool sent=ticket&&handoff.dispatch(*ticket,current,resolve_offer(context(),handoff.step()));
  check(sent,"actual fresh resolver result produces exactly one native owner ticket");
  if(!sent)return false;dispatched_ticket=*ticket;
  put(f.view.data(),a::view_done,1);put(f.view.data(),a::view_active,0);put(f.view.data(),a::view_opened,0);put(f.view.data(),a::view_closed,1);
  put(manager_bytes,0x40,0);native_update=true;update_depth=1;frame_depth=0;return true;
}
void repeated_owner_wrapper_cases(Fixture& f){
  for(const auto& route:nav::routes){
    check(f.owner_phase(route),"repeated owner fixture creates a complete current native INFO page");
    owner_fixture=&f;owner_route=&route;original_info=rebuilding_owner_info;
    if(!dispatch_owner(f))continue;
    caller=a::resolve_selection_return;SetLastError(incoming_error);
    const auto resolved=observe_resolve(f.manager(),nullptr,f.hero.data(),f.hero.data(),0);
    check(resolved==f.info.data()&&resolve_calls==1&&!recover_info&&GetLastError()==original_error,
      "real resolver wrapper retains exact previously opened owner without synthetic NULL recovery");
    caller=a::info_effect_return;SetLastError(incoming_error);observe_info(resolved,nullptr);
    check(info_calls==1&&seen_first==f.info.data()&&seen_error==incoming_error&&GetLastError()==original_error,
      "real Info wrapper forwards the rebuilding original once and preserves LastError");
    check(handoff.state()==ubc::HandoffState::awaiting_collection&&!effect_executed&&!effect_authorized&&!info_depth,
      "owner return remains navigation pending and leaves no recipe or nested scope");
    frame_depth=1;update_depth=0;native_update=false;++frame;
    handoff.observe(observation(context(),true));
    check(handoff.state()==ubc::HandoffState::awaiting_collection&&handoff.step_index()==0,
      "old INFO collection cannot release an effect merely because owner returned");
    caller=a::choice_clear_return;observe_clear(f.view.data(),nullptr);
    const std::array<void*,4> choices{{f.choice.data(),f.extra_choices[0].data(),f.extra_choices[1].data(),f.extra_choices[2].data()}};
    caller=a::choice_add_return;for(auto* choice:choices)observe_add(f.view.data(),choice,0);
    put(manager_bytes,0x54,2);put(manager_bytes,0x40,1);put(f.view.data(),a::view_done,0);put(f.view.data(),a::view_opened,1);
    put(f.view.data(),a::view_closed,0);put(f.view.data(),a::view_active,1);
    const auto fresh=observation(context(),true);handoff.observe(fresh);
    const auto effect=resolve_offer(context(),handoff.step());
    check(clear_calls==1&&add_calls==4&&fresh.complete&&fresh.collection>dispatched_ticket.offer.collection,
      "real Clear/Add wrappers prove one newly finished four-choice collection");
    check(handoff.state()==ubc::HandoffState::awaiting_step&&handoff.step_index()==1&&effect.valid&&effect.row==0&&
      effect.function==static_cast<int>(cat::callback_for_quantity(*cat::find(route.recipe),1)->function->symbol),
      "only the later native target collection releases the freshly resolved original one-quantity effect");
    check(!choice_calls&&!script_calls&&!effect_executed,"successful repeated opening itself produces no item effect");
    handoff.cancel();
  }
  for(unsigned bad=0;bad<6;++bad){
    const auto& route=nav::routes[1];check(f.owner_phase(route),"negative repeated owner has valid native INFO context");
    owner_fixture=&f;owner_route=&route;original_info=rebuilding_owner_info;
    if(!dispatch_owner(f))continue;
    if(bad==0)put(f.info.data(),0x28,route.owner->condition->symbol+1);
    if(bad==1)++rows.generation;
    if(bad==2)f.new_view();
    if(bad==3)collection_depth=1;
    if(bad==4)native_update=false;
    if(bad==5)handoff.cancel();
    caller=a::resolve_selection_return;const auto resolved=observe_resolve(f.manager(),nullptr,f.hero.data(),f.hero.data(),0);
    check(!resolved&&recover_info&&resolve_calls==1,"changed metadata/page/collection, reentry or cancelled owner is rejected before native dispatch");
    caller=a::info_effect_return;observe_info(f.info.data(),nullptr);
    check(!info_calls&&!effect_executed&&handoff.cancelled(),"a late rebuilding callback cannot bypass the cancelled or mismatched ticket");
  }
  original_info=counted_info;inject_contracts=false;parser_bytes=nullptr;collection_depth=0;
}
void invoke_effect(Fixture& fixture,bool allow,const char* label){
  info_calls=choice_calls=0;seen_first=seen_second=nullptr;seen_error=0;
  caller=fixture.choice_mode?a::choice_effect_return:a::info_effect_return;
  SetLastError(incoming_error);
  if(fixture.choice_mode)observe_choice(fixture.manager(),fixture.choice.data());else observe_info(fixture.info.data(),nullptr);
  const auto error=GetLastError();
  check(info_calls+choice_calls==(allow?1u:0u),label);
  check(error==(allow?original_error:incoming_error),"original/block LastError remains exact");
  check(!info_depth&&!choice_depth&&!effect_authorized&&!effect_executed,"effect wrapper restores depth and never claims a recipe");
  if(allow)check(seen_error==incoming_error&&seen_first==(fixture.choice_mode?fixture.manager():fixture.info.data())&&
    (!fixture.choice_mode||seen_second==fixture.choice.data()),"ordinary original receives unchanged arguments exactly once");
  else check(handoff.cancelled()&&handoff.active(),"rejected old callback remains quarantined");
}
void invoke_resolve(Fixture& fixture,bool allow,const char* label){
  resolve_calls=0;seen_error=0;recover_info=false;caller=a::resolve_selection_return;
  SetLastError(incoming_error);
  const auto result=observe_resolve(fixture.manager(),nullptr,fixture.hero.data(),fixture.hero.data(),0);
  check(result==(allow?resolve_result:nullptr),label);
  check(resolve_calls==1&&seen_error==incoming_error&&GetLastError()==original_error,"resolver original runs exactly once with LastError preserved");
  check(recover_info!=allow,"only rejected resolver requests controlled native recovery");
}
} // namespace recipe_guard_test

void repeated_owner_cases(){
  using namespace recipe_guard_test;
  for(const auto& route:nav::routes){
    const auto& owner=*route.owner;
    ubc::SelectionInfo info{true,owner.npc,owner.number,0,static_cast<int>(owner.condition->symbol),
      static_cast<int>(owner.information->symbol),0,1,owner.instance,0x123400};
    const nav::Step canonical{nav::Kind::open_owner,cat::Invocation::info,owner.instance,
      static_cast<int>(owner.information->symbol),route.category,route.choices};
    check(linked_step_info(info,&canonical),"residual choices permit only the bound original rebuilding owner");
    check(!linked_step_info(info,nullptr),"residual choices need a concrete pending navigation step");
    for(const auto kind:{nav::Kind::back,nav::Kind::stop,nav::Kind::start,nav::Kind::effect}){
      auto step=canonical;step.kind=kind;
      check(!linked_step_info(info,&step),"residual choice exception never applies to another step kind");
    }
    auto step=canonical;step.invocation=cat::Invocation::choice;
    check(!linked_step_info(info,&step),"residual owner must be reached through INFO");
    step=canonical;++step.owner_instance;
    check(!linked_step_info(info,&step),"different owner cannot reuse residual choice exception");
    step=canonical;++step.function_symbol;
    check(!linked_step_info(info,&step),"different function cannot reuse residual choice exception");
    step=canonical;step.before=nav::Page::root;
    check(!linked_step_info(info,&step),"owner must start in its canonical category");
    step=canonical;step.after=nav::Page::root;
    check(!linked_step_info(info,&step),"owner must end in its canonical choice page");
    auto changed=info;++changed.condition;
    check(!linked_step_info(changed,&canonical),"changed info metadata is rejected before owner execution");
    changed=info;changed.first_choice=0;
    check(linked_step_info(changed,nullptr),"original empty info list remains eligible for the other gates");
  }
  const auto& owner=cat::meat_owners[0];
  ubc::SelectionInfo effect{true,owner.npc,owner.number,0,static_cast<int>(owner.condition->symbol),
    static_cast<int>(owner.information->symbol),0,1,owner.instance,0x123400};
  const nav::Step fabricated{nav::Kind::open_owner,cat::Invocation::info,owner.instance,
    static_cast<int>(owner.information->symbol),nav::Page::root,nav::Page::health_01_choices};
  check(!linked_step_info(effect,&fabricated),"direct cooking info effect cannot be relabelled as rebuilding owner");
}

int main(){
  repeated_owner_cases();
  using namespace recipe_guard_test;
  PrivatePages pages;if(!pages.open()){check(false,"private pages available");return 1;}
  static_assert(sizeof(Fixture)<=PrivatePages::size-0x1000,"bounded own fixture");
  auto& fixture=*new(pages.fixture_space()) Fixture;
  original_info=counted_info;original_choice=counted_choice;original_choice_event=counted_input;original_resolve=counted_resolve;
  original_clear=counted_clear;original_add=counted_add;
  for(const bool choice_mode:{false,true}){
    check(fixture.reset(choice_mode,false),"fixture without pending request");
    invoke_effect(fixture,true,"without own request every original callback passes once");
    check(fixture.reset(choice_mode),"valid dispatched fixture");
    invoke_effect(fixture,false,"unproven native Update cannot execute an owned recipe");
    for(const bool alter_metadata:{false,true}){
      check(fixture.reset(choice_mode),"foreign-dialog fixture starts");fixture.foreign_dialogue();
      if(alter_metadata)put(fixture.info.data(),0x28,90003);
      invoke_effect(fixture,false,"late bound recipe stays blocked in foreign dialogue even with altered condition");
      fixture.foreign_target();
      invoke_effect(fixture,true,"readable foreign noncatalogue Info or Choice remains ordinary");
    }
    check(fixture.reset(choice_mode),"unknown foreign callback fixture");fixture.foreign_dialogue();
    put(fixture.info.data(),0x2c,UINT32_MAX);
    invoke_effect(fixture,false,"unreadable foreign callback is not permission for old pending work");
    check(fixture.reset(choice_mode),"foreign view callback fixture");fixture.new_view();fixture.foreign_target();
    invoke_effect(fixture,true,"positive different view permits its unrelated callback");
    check(fixture.reset(choice_mode)&&fixture.effect_returned(),"pure handoff fixture reaches result pending without running an engine effect");
    put(fixture.info.data(),0x2c,UINT32_MAX);
    invoke_effect(fixture,false,"unknown callback stays guarded while effect result is still pending");
    fixture.foreign_target();invoke_effect(fixture,true,"valid unrelated original end path remains ordinary during result verification");
  }
  check(fixture.reset(false),"resolver old target fixture");fixture.foreign_dialogue();
  invoke_resolve(fixture,false,"old Info is rejected before native effect after dialog replacement");
  fixture.foreign_target();invoke_resolve(fixture,true,"foreign noncatalogue Info is resolved without synthetic NULL");
  check(fixture.reset(false),"new-view resolver fixture");fixture.new_view();fixture.foreign_target();
  invoke_resolve(fixture,true,"other valid Info on a positive new view is not forced into broken NULL recovery");

  check(fixture.reset(false),"input ticket fixture");
  check(observe_choice_event(fixture.callback(),nullptr,m::key_return)==1&&!input_calls,
    "physical confirm cannot reach the original bound view while ticket is pending");
  check(observe_choice_event(fixture.callback(),nullptr,m::key_escape)==1&&!input_calls&&handoff.cancelled(),
    "physical B cancels own chain without confirming or ending original selection");
  private_enter=true;
  check(observe_choice_event(fixture.callback(),nullptr,m::key_return)==1&&!input_calls,
    "synthetic flag cannot reopen an already cancelled ticket");
  check(fixture.reset(false),"synthetic input without engine contract fixture");private_enter=true;
  check(observe_choice_event(fixture.callback(),nullptr,m::key_return)==1&&!input_calls&&!private_enter&&handoff.cancelled(),
    "private Enter consumes its flag but cannot bypass the missing fresh engine contract");
  check(observe_choice_event(fixture.callback(),nullptr,m::key_return)==1&&!input_calls,
    "second Enter cannot reuse private permission or cancelled intent");
  check(fixture.reset(false),"synthetic foreign input fixture");fixture.foreign_dialogue();private_enter=true;
  check(observe_choice_event(fixture.callback(),nullptr,m::key_return)==1&&!input_calls&&!private_enter&&handoff.cancelled(),
    "private synthetic Enter never leaks into a new foreign dialogue");
  check(fixture.reset(false),"input read failure fixture");
  if(!pages.protect_globals(PAGE_NOACCESS)){check(false,"own global page unreadable");return 1;}
  check(observe_choice_event(fixture.callback(),nullptr,m::key_return)==1&&!input_calls,
    "unknown current context still consumes physical input at stored pending view");
  if(!pages.protect_globals(PAGE_READWRITE)){check(false,"restore own global page");return 1;}
  check(fixture.reset(false),"foreign input fixture");fixture.foreign_dialogue();
  check(observe_choice_event(fixture.callback(),nullptr,m::key_return)==original_input_result&&input_calls==1,
    "positive foreign dialogue can use the same native view address");
  check(fixture.reset(false),"unrelated callback fixture");
  check(observe_choice_event(fixture.second_view.data()+a::view_callback,nullptr,m::key_return)==original_input_result&&input_calls==1,
    "input guard is scoped to precisely the bound callback");

  for(const bool choice_mode:{false,true}){
    check(fixture.reset(choice_mode,false),"collection fixture");
    const auto before=rows.generation;caller=choice_mode?a::choice_clear_return:a::info_clear_return;
    observe_clear(fixture.view.data(),nullptr);
    check(clear_calls==1&&rows.generation==before+1&&!collection_depth,
      "one original clear creates one new collection generation and restores wrapper depth");
    caller=choice_mode?a::choice_add_return:a::info_add_return;
    observe_add(fixture.view.data(),choice_mode?static_cast<void*>(fixture.choice.data()):fixture.info.data()+0x30,0);
    check(add_calls==1&&!collection_depth&&complete_collection(context()),
      "canonical original adds form a complete Info or Choice collection without script calls");
    caller=0x123456;
    observe_add(fixture.view.data(),choice_mode?static_cast<void*>(fixture.choice.data()):fixture.info.data()+0x30,0);
    check(add_calls==2&&!complete_collection(context()),
      "unknown AddChoice still forwards exactly once and invalidates either collection kind");
    caller=choice_mode?a::choice_clear_return:a::info_clear_return;collection_depth=1;
    observe_clear(fixture.view.data(),nullptr);
    check(clear_calls==2&&collection_depth==1&&!rows.valid&&!choice_collection,
      "reentrant clear cannot claim a finished selection collection");
    collection_depth=0;
  }
  repeated_owner_wrapper_cases(fixture);
  post_remove_wrapper_cases(fixture);
  fixture.~Fixture();
  std::printf("RecipeEffectGuard: %u checks, %u failures\n",checks,failures);
  return failures?1:0;
}
