// SPDX-License-Identifier: MIT
// Dedicated Windows-x86 host. Actual Root wrappers, selection reader,
// handoff, PostRemove, CallFunc registry and AmountPatch/restore, using only
// private RAM objects. Explicit test seams replace catalog, material, focus,
// ReturnAddress, global data slots and ammo adapter responses. No engine,
// original hash or world evidence. A separate host checks actual ammo lifetime.
#include <windows.h>
#include <intrin.h>
#include <cstdio>
#include <cstring>
#include <new>
#include "ubc/core/crafting_handoff.hpp"
#include "ubc/stations/saw/saw_actor_preview.hpp"
#include "ubc/ui/native_crafting_ui.hpp"
#include "ubc/engine/native_crafting_ammo.hpp"
#include "ubc/engine/windows_probe.hpp"
#include "ubc/engine/selection_abi.hpp"
#include "ubc/stations/station_end_contract.hpp"
#include "ubc/stations/alchemy/crafting_alchemy_extra_reader.hpp"
#include "ubc/stations/alchemy/alchemy_extra_end_snapshot.hpp"
#include "../src/writers/alchemy_extra_writer.hpp"
#include "ubc/stations/spirit/crafting_spirit_projection.hpp"
#include "ubc/stations/spirit/spirit_end_snapshot.hpp"
#include "../src/writers/spirit_writer.hpp"
#include "ubc/stations/bow/crafting_bow_projection.hpp"
#include "ubc/stations/bow/bow_item_runtime.hpp"
#include "ubc/stations/bow/crafting_bow_execution.hpp"
#include "ubc/stations/grind/crafting_grind_projection.hpp"
#include "ubc/stations/grind/grind_destroy_observation.hpp"
#include "ubc/stations/grind/grind_menu_metadata.hpp"
#include "ubc/stations/grind/grind_preview_batch.hpp"
#include "ubc/stations/cool/cool_predebit_admission.hpp"

namespace crafting_guard_test {
std::uintptr_t caller=0;
std::uint8_t* globals=nullptr,*manager_bytes=nullptr,*parser_bytes=nullptr;
bool inject_contracts=false,ui_created=false,ui_owned=false,ui_render_ok=false;
bool test_notice=false,lose_input_on_submit=false;
unsigned notice_renders=0,notice_receipt_reads=0;
bool ammo_available=true,ammo_armed=false,ammo_accept=true,damage_after_admit=false;
bool end_path_valid=true;
bool script_raises=false,script_changes_operand=false;
bool diagnostic_probe=false,script_changes_hero=false,ammo_close_accept=true,ammo_lose_health=false,early_diagnostic_flush=false;
// A complete check report contains up to 640 events of 352 bytes each.
char diagnostic_log[256 * 1024]{};
std::size_t diagnostic_size=0;
unsigned diagnostic_writes=0;
ubc::native_crafting_ammo::Status diagnostic_status{};
BOOL WINAPI diagnostic_write_file(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED) noexcept;
bool bind_choice_self=true;
bool bind_anvil_self=true;
unsigned script_exceptions=0;
unsigned ammo_admits=0,ammo_enters=0,ammo_closes=0;
unsigned ammo_installs=0,ammo_status_reads=0;
std::uint32_t ammo_install_thread=0;
bool ammo_install_accept=true,ammo_install_reenter=false;
ubc::ammo_presentation::Admission admitted_proof{};
ubc::ammo_presentation::Job admitted_job{};
ubc::CraftingSnapshot injected{};
std::uint32_t removed_node=0,removed_choice=0;
unsigned removed_reads=0;
unsigned grind_catalogue_captures=0;
unsigned preview_memory_reads=0;
void* return_address() noexcept{return reinterpret_cast<void*>(caller);}
}
namespace ubc {
class CraftingGuardGrindCatalogueReader:public GrindRecipeCatalogueReader {
public:
  GrindRecipeCatalogueCapture capture(const ReadOnlyMemory& memory,std::uint32_t parser,
      const GrindRecipeKey& key,bool boundary) noexcept {
    ++crafting_guard_test::grind_catalogue_captures;
    return GrindRecipeCatalogueReader::capture(memory,parser,key,boundary);
  }
};
struct CraftingGuardCapture {CraftingSnapshot snapshot{};struct {int issue=0;bool actor_unchanged=false;
  station_recipe_runtime::ActorEvidence actor_before{},actor_after{};} diagnostic;};
inline CraftingSnapshot project_capture(const CraftingGuardCapture& value,bool physical_hero_preview_verified=false) noexcept{
  auto out=value.snapshot;
  if(out.key.kind==crafting_catalog::Station::saw)for(auto& state:out.recipes)if(state.evaluated){
    state.actor_known=value.diagnostic.actor_unchanged;
    state.actor_matches=state.actor_known&&value.diagnostic.actor_before.matches&&value.diagnostic.actor_after.matches;
  }
  if(out.key.kind==crafting_catalog::Station::anvil)for(auto& state:out.recipes)if(state.evaluated){
    const auto& before=value.diagnostic.actor_before;const auto& after=value.diagnostic.actor_after;
    state.actor_known=state.actor_known&&value.diagnostic.actor_unchanged;
    state.actor_matches=state.actor_known&&before.requires_self&&after.requires_self&&before.hero==out.key.hero&&after.hero==out.key.hero&&
      (physical_hero_preview_verified||(state.actor_matches&&before.matches&&after.matches));
  }
  return out;
}
inline CraftingSnapshot project_capture(const CraftingGuardCapture& a,const CraftingGuardCapture& b,
    bool physical_hero_preview_verified=false) noexcept {
  return a.snapshot.key==b.snapshot.key?project_capture(a,physical_hero_preview_verified):CraftingSnapshot{};
}
inline CraftingSnapshot project_capture(const CraftingGuardCapture&,const AlchemyExtraCatalogueCapture&) noexcept {
  // This host does not authorize ExtraReader or ITEM. An Extra reader
  // initialized by mistake must not inherit the synthetic base test seam.
  return {};
}
inline CraftingSnapshot project_capture(const CraftingGuardCapture&,const CraftingBaseAlchemyEvidence&,
    const AlchemyExtraCatalogueCapture&,const SpiritCraftingCapture&) noexcept {
  // This material seam has no shared raw reader evidence.
  return {};
}
class CraftingGuardCatalogueReader {
public:
  template<class Key,std::size_t N>CraftingGuardCapture capture(const ReadOnlyMemory& memory,std::uint32_t parser,const Key& key,
    bool boundary,const std::array<bool,N>&) noexcept {
    CraftingGuardCapture out;
    if(crafting_guard_test::inject_contracts){
      out.snapshot=crafting_guard_test::injected;
      out.snapshot.key.generation=key.generation;out.snapshot.key.thread=key.thread;
      out.snapshot.key.world=key.world;out.snapshot.key.hero=key.hero;out.snapshot.key.station=key.station;out.snapshot.key.manager=key.manager;
      if(out.snapshot.key.kind==crafting_catalog::Station::saw){
        const auto& recipe=*station_recipe_catalog::find(station_recipe_catalog::RecipeId::arrows);
        out.diagnostic.actor_before=station_recipe_runtime::actor(memory,parser,recipe,key.hero);
        out.diagnostic.actor_after=station_recipe_runtime::actor(memory,parser,recipe,key.hero);
        out.diagnostic.actor_unchanged=station_recipe_runtime::same_actor_evidence(out.diagnostic.actor_before,out.diagnostic.actor_after);
      }else if(out.snapshot.key.kind==crafting_catalog::Station::anvil){
        for(const auto& state:out.snapshot.recipes)if(state.evaluated){
          const auto* recipe=crafting_catalog::find(state.id);if(!recipe)break;
          if(recipe->anvil){
            const auto profile=anvil_recipe_runtime::assemble(*recipe->anvil);
            out.diagnostic.actor_before=anvil_recipe_runtime::actor(memory,parser,profile,key.hero);
            out.diagnostic.actor_after=anvil_recipe_runtime::actor(memory,parser,profile,key.hero);
          }else if(recipe->ore){
            const auto profile=ore_recipe_runtime::assemble(*recipe->ore);
            out.diagnostic.actor_before=ore_recipe_runtime::actor(memory,parser,profile,key.hero);
            out.diagnostic.actor_after=ore_recipe_runtime::actor(memory,parser,profile,key.hero);
          }
          out.diagnostic.actor_unchanged=station_recipe_runtime::same_actor_evidence(out.diagnostic.actor_before,out.diagnostic.actor_after);
          break;
        }
      }
      if(!boundary)out.snapshot.key={};
    }
    return out;
  }
  CraftingGuardCapture capture(const ReadOnlyMemory& memory,std::uint32_t parser,const AnvilRecipeKey& key,
      AnvilCaptureProofs proof={}) noexcept {
    // Both new readers explicitly remain a material/profile seam. Context,
    // page, selection and the subsequent writer are actual Root functions.
    return capture(memory,parser,key,proof.stable_boundary_verified&&proof.station_hand_end_verified,std::array<bool,1>{{true}});
  }
};
class CraftingGuardUi:public NativeCraftingUi {
public:
  static bool focused() noexcept{return crafting_guard_test::inject_contracts;}
  bool opened() const noexcept{return crafting_guard_test::ui_created;}
  void* object() noexcept{return crafting_guard_test::ui_created?this:nullptr;}
  bool owns_input() const noexcept{return crafting_guard_test::ui_owned;}
  bool render(const CraftingSession&,bool) noexcept{return crafting_guard_test::ui_render_ok;}
  bool success_notice()const noexcept{return crafting_guard_test::test_notice||NativeCraftingUi::success_notice();}
  static bool buttons_held(bool* known=nullptr)noexcept{
    if(!crafting_guard_test::test_notice)return NativeCraftingUi::buttons_held(known);
    if(known)*known=true;return true;
  }
  bool render_success()noexcept{
    if(!crafting_guard_test::test_notice)return NativeCraftingUi::render_success();
    ++crafting_guard_test::notice_renders;
    if(crafting_guard_test::lose_input_on_submit)crafting_guard_test::ui_owned=false;
    return crafting_guard_test::ui_render_ok;
  }
  std::uint64_t success_receipt()const noexcept{
    if(!crafting_guard_test::test_notice)return NativeCraftingUi::success_receipt();
    ++crafting_guard_test::notice_receipt_reads;return 101;
  }
  void close() noexcept{crafting_guard_test::ui_created=crafting_guard_test::ui_owned=false;}
};
namespace runtime_script {
inline RuntimeContract crafting_guard_verify_local_contract(const ReadOnlyMemory& memory,std::uint32_t parser,const lhiver::Profile& profile) noexcept {
  if(crafting_guard_test::inject_contracts)return {parser==lhiver::game_parser,Issue::none,0};
  return verify_local_contract(memory,parser,profile);
}
}
namespace crafting_guard_station_runtime {
using station_recipe_runtime::assemble;
using station_recipe_runtime::actor;
using station_recipe_runtime::same_actor_evidence;
inline runtime_script::RuntimeContract verify(const ReadOnlyMemory& memory,std::uint32_t parser,const station_recipe_runtime::ProfileAssembly& profile) noexcept {
  if(crafting_guard_test::inject_contracts)return {parser==lhiver::game_parser,runtime_script::Issue::none,0};
  return station_recipe_runtime::verify(memory,parser,profile);
}
}
namespace crafting_guard_anvil_runtime {
using anvil_recipe_runtime::assemble;using anvil_recipe_runtime::context;using anvil_recipe_runtime::same_context_evidence;
using anvil_recipe_runtime::actor;
inline runtime_script::RuntimeContract verify(const ReadOnlyMemory& memory,std::uint32_t parser,const anvil_recipe_runtime::ProfileAssembly& profile) noexcept {
  if(crafting_guard_test::inject_contracts)return {parser==lhiver::game_parser,runtime_script::Issue::none,0};
  return anvil_recipe_runtime::verify(memory,parser,profile);
}
}
namespace crafting_guard_ore_runtime {
using ore_recipe_runtime::assemble;
using ore_recipe_runtime::actor;
inline runtime_script::RuntimeContract verify(const ReadOnlyMemory& memory,std::uint32_t parser,const ore_recipe_runtime::ProfileAssembly& profile) noexcept {
  if(crafting_guard_test::inject_contracts)return {parser==lhiver::game_parser,runtime_script::Issue::none,0};
  return ore_recipe_runtime::verify(memory,parser,profile);
}
}
namespace station_end_contract {
inline PathVerification crafting_guard_verify_path(const ReadOnlyMemory& memory,std::uint32_t vtable,const InstalledEndHook& hook,
    const zbug_end_compat::Binding* compatibility=nullptr) noexcept {
  if(crafting_guard_test::inject_contracts)return {crafting_guard_test::end_path_valid,
    crafting_guard_test::end_path_valid?PathIssue::none:PathIssue::code_hash};
  return verify_path(memory,vtable,hook,compatibility);
}
}
namespace recipe_catalog {const Callback* crafting_guard_callback_for_quantity(const Descriptor&,std::uint32_t) noexcept;}
namespace station_recipe_catalog {const Callback* crafting_guard_callback_for_quantity(const Descriptor&,std::uint32_t) noexcept;}
namespace alchemy_recipe_catalog {QuantityPlan crafting_guard_plan_quantity(const Descriptor&,std::uint32_t) noexcept;}
namespace anvil_recipe_catalog {QuantityPlan crafting_guard_plan_quantity(const Descriptor&,std::uint32_t) noexcept;}
namespace ore_recipe_catalog {QuantityPlan crafting_guard_plan_quantity(const Descriptor&,std::uint32_t) noexcept;}
// Do not give Bow a synthetic positive quantity or module contract.
namespace bow_recipe_catalog {inline QuantityPlan crafting_guard_plan_quantity(const Descriptor& d,std::uint32_t n) noexcept{return plan_quantity(d,n);}}
namespace crafting_guard_ammo {
using namespace native_crafting_ammo;
bool configure(const Readers&) noexcept;
bool install(std::uint32_t) noexcept;
Status status() noexcept;
bool healthy() noexcept;
bool permits_next() noexcept;
bool admit(const ammo_presentation::Job&,const ammo_presentation::Admission&) noexcept;
bool enter_effect(const ammo_presentation::Context&,const ammo_presentation::Scope&) noexcept;
bool close_effect(const ammo_presentation::Context&,const ammo_presentation::Scope&) noexcept;
bool observe_result(const ammo_presentation::Result&) noexcept;
ammo_presentation::Notice take_notice() noexcept;
bool ack_notice(std::uint64_t,std::uint64_t,bool) noexcept;
}
namespace windows_probe {
// Only this private host can admit its synthetic process to the test DLL entry.
bool activation_fixture = false;
bool crafting_guard_target_matches() noexcept {
  return activation_fixture || target_matches();
}
bool crafting_guard_copy_memory(void* user,std::uint32_t value,void* out,std::size_t size) noexcept {
  using namespace crafting_guard_test;
  ++preview_memory_reads;
  if((removed_node&&value==removed_node)||(removed_choice&&value==removed_choice)){++removed_reads;return false;}
  const auto translate=[&](std::uint32_t base,std::size_t length,const std::uint8_t* target){
    if(value<base||value-base>length||size>length-(value-base))return false;
    value=target?static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(target))+value-base:0;return true;
  };
  if(!translate(selection_abi::game_pointer,4,globals)&&!translate(station_abi::hero_pointer,4,globals?globals+4:nullptr))
    if(!translate(station_abi::information_manager,0x58,manager_bytes))translate(station_abi::game_parser,0x58,parser_bytes);
  const bool copied=copy_memory(user,value,out,size);SetLastError(0x99332211);return copied;
}
}
}
#define _ReturnAddress() crafting_guard_test::return_address()
#define target_matches crafting_guard_target_matches
#define copy_memory crafting_guard_copy_memory
#define RecipeCatalogueReader CraftingGuardCatalogueReader
#define StationRecipeCatalogueReader CraftingGuardCatalogueReader
#define AnvilRecipeCatalogueReader CraftingGuardCatalogueReader
#define OreRecipeCatalogueReader CraftingGuardCatalogueReader
#define GrindRecipeCatalogueReader CraftingGuardGrindCatalogueReader
#define NativeCraftingUi CraftingGuardUi
#define verify_local_contract crafting_guard_verify_local_contract
#define station_recipe_runtime crafting_guard_station_runtime
#define anvil_recipe_runtime crafting_guard_anvil_runtime
#define ore_recipe_runtime crafting_guard_ore_runtime
#define native_crafting_ammo crafting_guard_ammo
#define callback_for_quantity crafting_guard_callback_for_quantity
#define plan_quantity crafting_guard_plan_quantity
#define verify_path crafting_guard_verify_path
#define WriteFile crafting_guard_test::diagnostic_write_file
#include "crafting_native_fixture.hpp"
#undef target_matches
#undef WriteFile
#undef verify_path
#undef plan_quantity
#undef callback_for_quantity
#undef native_crafting_ammo
#undef station_recipe_runtime
#undef anvil_recipe_runtime
#undef ore_recipe_runtime
#undef verify_local_contract
#undef NativeCraftingUi
#undef StationRecipeCatalogueReader
#undef AnvilRecipeCatalogueReader
#undef OreRecipeCatalogueReader
#undef GrindRecipeCatalogueReader
#undef RecipeCatalogueReader
#undef copy_memory
#undef _ReturnAddress

namespace crafting_guard_test {
unsigned checks=0,failures=0,info_calls=0,choice_calls=0,input_calls=0,resolve_calls=0,script_calls=0,writer_routes=0;
DWORD seen_error=0;
constexpr DWORD incoming_error=0x1234,original_error=0x5678;
constexpr DWORD synthetic_exception=0xe043b716;
void* resolve_result=nullptr;
void check(bool value,const char* name){++checks;if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",name);}}
BOOL WINAPI diagnostic_write_file(HANDLE,LPCVOID source,DWORD size,LPDWORD written,LPOVERLAPPED) noexcept {
  check(diagnostic_probe&&!amount_patch.active(),"diagnostic file write occurs only after restored amount bytes");
  check(diagnostic_size+size<sizeof(diagnostic_log),"bounded private diagnostic log has room");
  if(diagnostic_size+size>=sizeof(diagnostic_log)){if(written)*written=0;return FALSE;}
  std::memcpy(diagnostic_log+diagnostic_size,source,size);diagnostic_size+=size;diagnostic_log[diagnostic_size]=0;
  ++diagnostic_writes;if(written)*written=size;SetLastError(0x33445566);return TRUE;
}
bool diagnostic_event(const char* name,int detail,int quantity) noexcept {
  char line[200]{};std::snprintf(line,sizeof(line),"{\"event\":\"%s\",\"frame\":%llu,\"detail\":%d,\"quantity\":%d}\r\n",
    name,static_cast<unsigned long long>(frame),detail,quantity);
  return std::strstr(diagnostic_log,line)!=nullptr;
}
std::uint32_t address(const void* value){return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(value));}
void put(void* object,std::size_t offset,std::uint32_t value){std::memcpy(static_cast<std::uint8_t*>(object)+offset,&value,4);}
const basecat::Callback* synthetic(const basecat::Callback* value) noexcept {
  if(!inject_contracts||!value)return value;
  static basecat::Callback callback;static ubc::lhiver::Function function;
  static constexpr std::array<std::uint8_t,6> body{{64,5,0,0,0,60}};
  static constexpr ubc::AmountOperand operand{1,5,1,0,false};
  ubc::Sha256 hash;hash.update(body.data(),body.size());
  callback=*value;function=*value->function;function.byte_offset=0;function.byte_length=6;function.body_sha256=hash.finish();
  callback.function=&function;callback.amount={6,function.body_sha256,&operand,1};return &callback;
}
const basecat::Callback* fixture_callback(const cat::Descriptor& recipe,std::uint32_t quantity) noexcept {
  if(recipe.grind){
    // Metadata only for ordinary forwarding to the original.
    // Do not inject runtime, writer, skill or Destroy evidence.
    static basecat::Callback callback;
    callback={&recipe.grind->function,20957,basecat::Invocation::choice,1,1,INT32_MAX,
      {recipe.grind->function.byte_length,recipe.grind->function.body_sha256,nullptr,0},{},{},true,true,false,true};
    return &callback;
  }
  if(recipe.cool){
    // Metadata only for unchanged forwarding to the original. No
    // synthetic native or Destroy authorization for our cool writer.
    static const basecat::Callback callback{ccat::single.information,ccat::single.instance,basecat::Invocation::info,
      1,1,1,{ccat::single.information->byte_length,ccat::single.information->body_sha256,nullptr,0},{},{},true,true,false,true};
    return quantity==1?&callback:nullptr;
  }
  if(!recipe.spirit)return callback_for(recipe,quantity);
  // Only the private fixture needs this selection metadata. The native
  // dispatcher still uses the separate spirit writer, never this body.
  static const basecat::Callback callback{gcat::recipes[0].callback,gcat::owner.instance,basecat::Invocation::info,
    1,1,gcat::maximum_quantity,gcat::recipes[0].amount,{},{},true,true,false,true};
  return &callback;
}
class PrivatePages {
public:
  static constexpr std::size_t size=0x40000;
  bool open(){
    for(auto& page:pages){page=VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
      MEMORY_BASIC_INFORMATION region{};
      if(!page||VirtualQuery(page,&region,sizeof(region))!=sizeof(region)||region.AllocationBase!=page||region.Type!=MEM_PRIVATE)return false;
      if(address(page)<0xac0000u&&address(page)+size>0xaa0000u)return false;}
    globals=static_cast<std::uint8_t*>(pages[0]);manager_bytes=static_cast<std::uint8_t*>(pages[1]);return true;
  }
  void* space(){return static_cast<std::uint8_t*>(pages[1])+0x1000;}
  ~PrivatePages(){for(auto* page:pages)if(page)VirtualFree(page,0,MEM_RELEASE);globals=manager_bytes=parser_bytes=nullptr;}
private:std::array<void*,2> pages{};
};
struct Fixture {
  alignas(4) std::array<std::uint8_t,0x5c> info{};
  alignas(4) std::array<std::uint8_t,0x5c> stale_info{};
  alignas(4) std::array<std::uint8_t,0x18> choice{};
  alignas(4) std::array<std::uint8_t,8> node{},station_node{};
  alignas(4) std::array<std::array<std::uint8_t,8>,2> neighbours{};
  alignas(4) std::array<std::array<std::uint8_t,0x18>,2> neighbour_choices{};
  alignas(4) std::array<std::uint8_t,a::view_size> view{},second_view{};
  alignas(4) std::array<std::uint8_t,12> game{};
  alignas(4) std::array<std::uint8_t,0x9bc> hero{},other_hero{};
  alignas(4) std::array<std::uint8_t,0x204> station{};
  alignas(4) std::array<std::uint8_t,0x48> position{};
  alignas(4) std::array<std::uint8_t,0x58> parser{};
  alignas(4) std::array<std::uint32_t,24000> symbol_table{};
  alignas(4) std::array<std::array<std::uint8_t,0x3c>,10> symbols{};
  alignas(4) std::array<std::uint8_t,6> body{{64,5,0,0,0,60}};
  std::array<std::uint32_t,4> tokens{};
  ubc::CraftingHandoffObservation prepared{};
  const cat::Descriptor* descriptor=nullptr;
  bool choice_mode=false;
  void* manager()const{return reinterpret_cast<void*>(s::information_manager);}
  void* callback(){return view.data()+a::view_callback;}
  ubc::CraftingPage direct_page()const {
    if(descriptor->station==cat::Station::stove)return ubc::crafting_stove_page();
    if(descriptor->alchemy_extra)return ubc::crafting_page(xnav::target_page(descriptor->alchemy_extra->category));
    if(descriptor->spirit)return ubc::crafting_page(nav::Page::special_info);
    if(descriptor->route==cat::Route::base){
      const auto* route=nav::route_for(descriptor->base->id);return route?ubc::crafting_page(route->choices):ubc::CraftingPage{};
    }
    if(descriptor->station_descriptor)return ubc::crafting_page(snav::route_for(descriptor->station_descriptor->id)->choices);
    if(descriptor->anvil)return ubc::crafting_page(anav::page_for(descriptor->anvil->id));
    if(descriptor->ore)return ubc::crafting_page(anav::Page::ore);
    if(descriptor->bow)return ubc::crafting_page(bnav::Page::upgrades);
    return {};
  }
  bool reset(cat::RecipeId id,std::uint32_t quantity=1,bool pending=true) {
    inject_contracts=false;parser_bytes=nullptr;removed_node=removed_choice=0;removed_reads=0;post_remove_choice.clear();
    descriptor=cat::find(id);if(!descriptor)return false;
    const auto* target=fixture_callback(*descriptor,quantity);if(!target)return false;
    const auto* owner=owner_for(target->owner_instance);if(!owner)return false;
    choice_mode=target->invocation==basecat::Invocation::choice;
    info.fill(0);choice.fill(0);node.fill(0);view.fill(0);second_view.fill(0);game.fill(0);hero.fill(0);other_hero.fill(0);station.fill(0);
    std::memset(manager_bytes,0,0x58);
    put(info.data(),0,ubc::info_detail::info_vtable);put(info.data(),0x1c,static_cast<std::uint32_t>(owner->npc));
    put(info.data(),0x20,static_cast<std::uint32_t>(owner->number));put(info.data(),0x24,owner->important);
    put(info.data(),0x28,owner->condition->symbol);put(info.data(),0x2c,owner->information->symbol);
    put(info.data(),0x44,owner->trade);put(info.data(),0x48,owner->permanent);put(info.data(),0x50,static_cast<std::uint32_t>(owner->instance));
    if(choice_mode){put(info.data(),0x58,address(node.data()));put(node.data(),0,address(choice.data()));put(choice.data(),0x14,target->function->symbol);}
    put(game.data(),0,a::game_vtable);put(game.data(),a::game_world,address(&tokens[0]));
    put(hero.data(),s::npc_interact_mob,address(station.data()));put(globals,0,address(game.data()));put(globals,4,address(hero.data()));
    put(manager_bytes,0x1c,address(view.data()));put(manager_bytes,0x20,address(hero.data()));put(manager_bytes,0x24,address(hero.data()));
    put(manager_bytes,0x28,address(info.data()));put(manager_bytes,0x40,1);put(manager_bytes,0x54,choice_mode?2u:1u);
    put(view.data(),0,a::view_vtable);put(view.data(),0x24,0x83bc10);put(view.data(),a::view_callback,a::view_callback_vtable);
    put(view.data(),a::view_count,1);put(view.data(),a::view_selected,0);put(view.data(),a::view_opened,1);put(view.data(),a::view_active,1);second_view=view;
    InterlockedExchange(&installed,1);InterlockedExchange(&engine_thread,static_cast<LONG>(GetCurrentThreadId()));
    generation=1;frame=100;frame_depth=update_depth=info_depth=choice_depth=station_depth=collection_depth=0;
    native_update=disabled=private_enter=effect_authorized=effect_executed=recover_info=false;
    log_file=INVALID_HANDLE_VALUE;amount_diagnostic={};ammo_diagnostic={};log_events=log_bytes=0;
    grind_ui_readiness={};grind_quantity_observation={};grind_ui_trace_active=false;
    diagnostic_probe=script_changes_hero=ammo_lose_health=early_diagnostic_flush=false;ammo_close_accept=true;
    diagnostic_status={};diagnostic_size=diagnostic_writes=0;diagnostic_log[0]=0;
    tobacco_owner={};tobacco_owner_call=0;tobacco_choice_call=0;
    InterlockedExchange(&tobacco_unpaid_page,0);
    tobacco_writer_entered=tobacco_clean_rejection=tobacco_owner_opened=false;
    rows={};rows.generation=1;rows.view=address(view.data());
    choice_collection=false;choice_count=0;handoff={};dispatched_ticket={};result_guard={};amount_patch={};session={};
    ammo_job=extra_job=spirit_job=cool_job=grind_job=success_pending=false;extra_catalogue_reader.reset();extra_executor.reset();
    cool_active_entry={};cool_destroy_call={};cool_destroy_binding={};cool_destroy_depth=0;
    grind_prepared={};grind_destroy_call={};grind_destroy_binding={};grind_choice_lease={};
    grind_preview_reader.reset();grind_preview_scope={};grind_preview_publication={};
    grind_preview_serial=grind_preview_input_serial=0;grind_preview_input_depth=0;grind_preview_overflow=false;
    grind_preview_failure={};
    grind_item_reader.reset();grind_initialized=grind_original_returned=grind_skill_log_pending=false;
    grind_skill_after_values={};grind_serial=grind_job_floor=0;
    spirit_catalogue_reader.reset();spirit_executor.reset();spirit_writer_scope=nullptr;spirit_active_job={};
    spirit_normal_end=false;spirit_end={};spirit_end_frame=0;
    bow_item_reader.reset();bow_initialized=false;
    extra_writer_scope=nullptr;extra_active_job={};resolved_read_depth=0;info_effect_lease={};
    active_ammo_scope={};active_ammo_context={};world_epoch=1;
    ammo_install_attempted=false;ammo_installs=ammo_status_reads=0;ammo_install_thread=0;
    ammo_install_accept=true;ammo_install_reenter=false;
    observed_world=address(&tokens[0]);observed_hero=address(hero.data());borrowed_view=0;
    rendered_generation=request_ui_generation=0;request_ui_key={};
    ui_created=ui_owned=ui_render_ok=false;ammo_available=ammo_accept=end_path_valid=true;ammo_armed=damage_after_admit=false;
    ammo_admits=ammo_enters=ammo_closes=info_calls=choice_calls=input_calls=resolve_calls=script_calls=writer_routes=0;
    script_raises=script_changes_operand=false;script_exceptions=0;
    bind_choice_self=bind_anvil_self=true;
    seen_error=0;resolve_result=info.data();
    if(!pending)return true;
    ubc::CraftingRequest request;request.id=id;request.quantity=quantity;
    request.key={generation,GetCurrentThreadId(),address(&tokens[0]),address(hero.data()),address(station.data()),s::information_manager,descriptor->station};
    auto& material=request.material_snapshot;material=ubc::make_crafting_state(id);material.generation=1;
    material.evaluated=material.knowledge_read_valid=material.known=material.contract=true;
    material.material_known=material.material_verified=material.hand_known=material.actor_known=material.actor_matches=true;material.maximum=10;
    material.context_known=material.context_matches=cat::requires_context(descriptor->route);
    if(descriptor->spirit){material.branch_known=true;material.branch_value=0;material.branch_output=15739;
      for(auto& row:material.materials)row.unit_output=row.symbol==15739?1u:0u;}
    for(std::size_t i=0;i<material.target_count;++i){auto& row=material.materials[i];const auto hand=cat::hand_amounts(*descriptor,row.symbol);
      row.stored=static_cast<std::int32_t>((row.unit_cost>row.unit_minimum?row.unit_cost:row.unit_minimum)*material.maximum)-hand.consumed;
      row.consumed_reserve=hand.consumed;row.tool_reserve=hand.tool;row.generated_prop=hand.generated;}
    auto& hands=request.hands;
    if(descriptor->route==cat::Route::base){hands.hands.left={0,-1,-1,0};hands.hands.right={1,descriptor->base->hand.active_item_symbol,1,address(&tokens[1])};}
    else if(descriptor->alchemy_extra||descriptor->spirit){
      hands.hands={{0,-1,-1,0},{1,16604,1,address(&tokens[1])}};
      hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;
      hands.left_tmp_level=hands.right_tmp_level=0;hands.interact_item=hands.hands.right.object;
    }
    else if(descriptor->station_descriptor) {
      const auto& wanted=descriptor->station_descriptor->hand;
      const auto make=[](const scat::HandItem& hand,std::uint32_t object){return hand.source==scat::HandSource::empty?
        ubc::HandObjectSnapshot{0,-1,-1,0}:ubc::HandObjectSnapshot{1,hand.symbol,static_cast<std::int32_t>(hand.amount),object};};
      hands.hands={make(wanted.left,address(&tokens[1])),make(wanted.right,address(&tokens[2]))};
      hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;
      hands.left_tmp_level=wanted.left.source==scat::HandSource::generated?1:0;
      hands.right_tmp_level=wanted.right.source==scat::HandSource::generated?2:0;
      hands.interact_item=wanted.anchor==scat::HandSide::left?hands.hands.left.object:hands.hands.right.object;
    }else if(descriptor->bow){
      hands.hands={{0,-1,-1,0},{0,-1,-1,0}};hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;
    }else if(descriptor->anvil||descriptor->ore){
      const auto& wanted=descriptor->anvil?descriptor->anvil->hand:descriptor->ore->hand;
      hands.hands={{1,wanted.left.symbol,static_cast<std::int32_t>(wanted.left.amount),address(&tokens[1])},
        {1,wanted.right.symbol,static_cast<std::int32_t>(wanted.right.amount),address(&tokens[2])}};
      hands.left_slot_known=hands.right_slot_known=hands.interact_item_known=true;
      hands.left_tmp_level=wanted.left_tmp_level;hands.right_tmp_level=wanted.right_tmp_level;
      hands.interact_item=hands.hands.right.object;
    }else return false;
    const auto plan=ubc::make_crafting_plan(direct_page(),id,quantity,material.branch_known,material.branch_value);
    prepared={};prepared.key=request.key;prepared.view=address(view.data());prepared.frame=frame;prepared.collection=1;
    prepared.context_known=prepared.page_known=prepared.complete=prepared.ready=prepared.outer_frame=prepared.quiescent=prepared.focused=true;
    prepared.page=plan.source;prepared.mode=target->invocation;prepared.choice_owner=choice_mode?owner->instance:-1;
    prepared.count=1;prepared.materials=material;prepared.hands=hands;
    const ubc::CraftingHandoffOffer offer{true,target->invocation,target->owner_instance,static_cast<std::int32_t>(target->function->symbol),0,1};
    if(!handoff.begin(request,plan,prepared))return false;const auto ticket=handoff.ticket(prepared,offer);
    if(!ticket||!handoff.dispatch(*ticket,prepared,offer))return false;dispatched_ticket=*ticket;return true;
  }
  void string(void* object,std::size_t offset,const char* text){put(object,offset+8,address(text));put(object,offset+12,static_cast<std::uint32_t>(std::strlen(text)));}
  bool native_phase() {
    const auto& request=handoff.request();inject_contracts=true;injected={};injected.key=request.key;injected.hands=request.hands;handed_hands=request.hands;
    for(auto& state:injected.recipes)if(state.id==request.id)state=request.material_snapshot;
    parser_bytes=parser.data();parser.fill(0);symbol_table.fill(0);for(auto& symbol:symbols)symbol.fill(0);body={{64,5,0,0,0,60}};
    const auto* target=fixture_callback(*descriptor,request.quantity);
    put(parser.data(),0x18,address(symbol_table.data()));put(parser.data(),0x1c,static_cast<std::uint32_t>(symbol_table.size()));
    put(parser.data(),0x20,static_cast<std::uint32_t>(symbol_table.size()));put(parser.data(),0x48,address(body.data()));
    put(parser.data(),0x50,address(body.data())+6);put(parser.data(),0x54,6);
    const auto scalar=[&](std::uint32_t index,std::size_t slot,std::uint32_t value){symbol_table[index]=address(symbols[slot].data());
      put(symbols[slot].data(),0x20,1|(2u<<12u));put(symbols[slot].data(),0x18,value);};
    symbol_table[ubc::lhiver::hero_symbol]=address(symbols[0].data());put(symbols[0].data(),0x1c,address(hero.data()));
    symbol_table[1537]=address(symbols[8].data());symbol_table[1679]=address(symbols[9].data());
    string(symbols[0].data(),0,"HERO");string(symbols[9].data(),0,"SELF");
    for(const auto slot:{0u,9u}){put(symbols[slot].data(),0x20,(7u<<12u)|(1u<<16u));
      put(symbols[slot].data(),0x38,address(symbols[8].data()));}
    put(symbols[9].data(),0x1c,address(hero.data()));
    scalar(ubc::lhiver::production_symbol,1,static_cast<std::uint32_t>(ubc::crafting_result_detail::production(*descriptor)));
    const auto health=descriptor->base&&descriptor->station==cat::Station::alchemy&&
      nav::route_for(descriptor->base->id)->category==nav::Page::health_info;
    if(descriptor->station==cat::Station::anvil){
      scalar(10536,2,1);scalar(10552,3,descriptor->anvil?1u:0u);scalar(10551,4,descriptor->ore?1u:0u);
      string(symbols[1].data(),0,"PLAYER_MOBSI_PRODUCTION");string(symbols[2].data(),0,"STARTSMITHING");
      string(symbols[3].data(),0,"NORMALWAFFEN");string(symbols[4].data(),0,"ERZWAFFEN");
    }else if(descriptor->bow){
      scalar(20862,2,1);scalar(10484,3,1);string(symbols[1].data(),0,"PLAYER_MOBSI_PRODUCTION");
      string(symbols[2].data(),0,"M_BOWS");string(symbols[3].data(),0,"PLAYER_TALENT_BOW_CROSSBOW");
    }else for(std::size_t i=0;i<nav::flag_bindings.size();++i){
      const auto* category=descriptor->alchemy_extra?xnav::category_for(xnav::target_page(descriptor->alchemy_extra->category)):nullptr;
      const bool active=descriptor->spirit?nav::flag_bindings[i].symbol==20361:
        descriptor->alchemy_extra?category&&category->on_start.symbol==static_cast<std::int32_t>(nav::flag_bindings[i].symbol):i==(health?0u:1u);
      scalar(nav::flag_bindings[i].symbol,i+2,active?1u:0u);
    }
    symbol_table[target->function->symbol]=address(symbols[7].data());string(symbols[7].data(),0,target->function->name);
    put(symbols[7].data(),0x20,(5u<<12u)|(1u<<16u));
    put(station.data(),0x18c,address(station_node.data()));put(station_node.data(),0,address(position.data()));put(position.data(),0x44,address(hero.data()));
    for(const auto offset:{0x1f4u,0x1f8u,0x1fcu})put(station.data(),offset,1);
    const char* on="",*scheme="",*use="",*condition="";
    if(descriptor->station==cat::Station::stove){on="FRYINGMEAT";scheme="STOVE";condition="B_USESTOVE";}
    else if(descriptor->station==cat::Station::alchemy){on="POTIONALCHEMY";use="ITMI_FLASK";}
    else if(descriptor->station==cat::Station::anvil){const auto& profile=acat::station;
      on=profile.on_state;scheme=profile.scheme;use=profile.use_with_item;condition=profile.condition_name;}
    else if(descriptor->bow){const auto& profile=bcat::station;on=profile.on_state;scheme=profile.scheme;use=profile.use_with_item;condition=profile.condition_name;}
    else {const auto& profile=*descriptor->station_descriptor->station_profile;
      on=profile.on_state;scheme=profile.scheme;use=profile.use_with_item;condition=profile.condition_name;}
    string(station.data(),0x1e0,on);string(station.data(),0x1b8,scheme);string(station.data(),0x1a4,use);string(station.data(),0x1cc,condition);
    if(choice_mode){
      for(std::size_t i=0;i<2;++i){neighbours[i].fill(0);neighbour_choices[i].fill(0);
        put(neighbours[i].data(),0,address(neighbour_choices[i].data()));put(neighbour_choices[i].data(),0x14,40001u+static_cast<std::uint32_t>(i));}
      put(info.data(),0x58,address(neighbours[0].data()));put(neighbours[0].data(),4,address(node.data()));put(node.data(),4,address(neighbours[1].data()));
      put(view.data(),a::view_count,3);put(view.data(),a::view_selected,1);prepared.count=3;
      auto new_request=request;handoff={};const auto plan=ubc::make_crafting_plan(prepared.page,new_request.id,new_request.quantity);
      const ubc::CraftingHandoffOffer offer{true,basecat::Invocation::choice,target->owner_instance,static_cast<std::int32_t>(target->function->symbol),1,1};
      if(!handoff.begin(new_request,plan,prepared))return false;const auto ticket=handoff.ticket(prepared,offer);
      if(!ticket||!handoff.dispatch(*ticket,prepared,offer))return false;dispatched_ticket=*ticket;
      choice_collection=true;choice_count=3;choice_functions[0]=40001;choice_functions[1]=dispatched_ticket.offer.function;choice_functions[2]=40002;
    }else {rows.valid=true;rows.count=1;rows.rows[0]=ubc::read_selection_info(memory,address(info.data()));}
    put(view.data(),a::view_done,1);put(view.data(),a::view_active,0);put(view.data(),a::view_opened,0);put(view.data(),a::view_closed,1);put(manager_bytes,0x40,0);
    native_update=true;update_depth=1;caller=choice_mode?a::choice_effect_return:a::info_effect_return;
    return context().valid&&complete_collection(context())&&page(context())==prepared.page;
  }
  void foreign_dialogue(){put(manager_bytes,0x20,address(other_hero.data()));}
  void old_info_owner(){
    stale_info=info;put(stale_info.data(),0x28,20338);put(stale_info.data(),0x2c,20339);put(stale_info.data(),0x50,20337);
    put(stale_info.data(),0x58,0);put(manager_bytes,0x28,address(stale_info.data()));
  }
  void foreign_target(){put(info.data(),0x50,90000);put(info.data(),0x2c,90001);put(choice.data(),0x14,90002);}
  void new_view(){put(manager_bytes,0x1c,address(second_view.data()));}
};
Fixture* selected=nullptr;
unsigned removal_mode=0;
void __thiscall counted_info(void*){++info_calls;seen_error=GetLastError();SetLastError(original_error);}
void __fastcall counted_choice(void*,void*){++choice_calls;seen_error=GetLastError();SetLastError(original_error);}
int __thiscall counted_input(void*,int){++input_calls;return 73;}
void* __thiscall counted_resolve(void*,void*,void*,int){++resolve_calls;seen_error=GetLastError();SetLastError(original_error);return resolve_result;}
void* __cdecl synthetic_script(void* parser,int symbol){
  check(GetLastError()==incoming_error,"original receives input LastError despite reader and enter-scope side effects");
  ++script_calls;std::uint32_t quantity=0;std::memcpy(&quantity,selected->body.data()+1,4);
  check(parser==reinterpret_cast<void*>(s::game_parser)&&symbol==dispatched_ticket.offer.function&&quantity==handoff.request().quantity,
    "one synthetic original observes only the requested parser/function/quantity");
  check(bc::diagnostics().writer&&amount_patch.active(),"original runs inside actual exclusive writer and temporary patch");
  check(result_guard.state()==ubc::CraftingResultState::waiting,"real result guard starts before effect and does not claim success");
  if(script_changes_operand)put(selected->body.data(),1,301);
  if(script_changes_hero)put(selected->symbols[0].data(),0x1c,address(selected->other_hero.data()));
  SetLastError(original_error);if(script_raises)RaiseException(synthetic_exception,0,0,nullptr);return nullptr;
}
}
extern "C" unsigned __cdecl UbcBatchPrepare(void*,int,std::uintptr_t);
extern "C" void* __cdecl UbcBatchWriter(void*,int);
extern "C" void* __cdecl UbcBatchReader(void*,int);
namespace crafting_guard_test {
std::int32_t ordinary_symbol=0;
void* __cdecl ordinary_three_script(void* parser,int symbol){
  ++script_calls;
  check(parser==reinterpret_cast<void*>(s::game_parser)&&symbol==ordinary_symbol,
    "ordinary anvil three-choice keeps its original parser and symbol");
  check(!amount_patch.active()&&!bc::diagnostics().writer&&bc::diagnostics().readers==1,
    "ordinary anvil three-choice takes only the genuine bridge reader lease");
  check(GetLastError()==incoming_error,"ordinary original receives unchanged input LastError");
  SetLastError(original_error);return nullptr;
}
void __fastcall original_three_choice(void*,void*){
  ++choice_calls;
  const auto route=UbcBatchPrepare(reinterpret_cast<void*>(s::game_parser),ordinary_symbol,bc::choice_call_return);
  check(route==1,"registered ordinary three-choice uses typed bridge passthrough, never own writer");
  if(route==1)UbcBatchReader(reinterpret_cast<void*>(s::game_parser),ordinary_symbol);
}
void native_writer(){
  const auto caller_pc=selected->choice_mode?bc::choice_call_return:bc::info_call_return;
  const auto route=UbcBatchPrepare(reinterpret_cast<void*>(s::game_parser),dispatched_ticket.offer.function,caller_pc);
  check(route==2,"actual registry admits selected template to one writer lease");
  if(route==2){++writer_routes;
    __try {UbcBatchWriter(reinterpret_cast<void*>(s::game_parser),dispatched_ticket.offer.function);}
    __except(GetExceptionCode()==synthetic_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){++script_exceptions;}
  }
}
void __thiscall original_info_writer(void*){
  ++info_calls;
  if(bind_anvil_self&&selected->descriptor->ore)put(selected->symbols[9].data(),0x1c,address(selected->hero.data()));
  native_writer();
}
void __fastcall original_choice_writer(void*,void*){
  ++choice_calls;auto& f=*selected;
  // Reproduce only the verified original SELF binding before the inner
  // CallFunc. The effect and symbol metadata remain the host's own objects.
  if((bind_choice_self&&f.descriptor->station==cat::Station::saw)||(bind_anvil_self&&f.descriptor->anvil))
    put(f.symbols[9].data(),0x1c,address(f.hero.data()));
  if(removal_mode!=1){put(f.neighbours[0].data(),4,removal_mode==2?0:address(f.neighbours[1].data()));
    removed_node=address(f.node.data());removed_choice=address(f.choice.data());}
  if(removal_mode==3)++rows.generation;
  if(removal_mode==4)f.new_view();
  native_writer();
}
void render_proof(Fixture& f,bool valid=true){
  check(session.open(injected, f.descriptor->id),"own menu session receives same confirmed request snapshot");
  ui_created=ui_owned=true;ui_render_ok=valid;
  check(render_selection()==valid,"real render helper records only successful current UI submit");
  const auto shown=session.preview().key;const bool registered=ui.object()&&ui.owns_input()&&rendered_generation==shown.generation;
  close_ui(context());
  request_ui_generation=registered?shown.generation:0;request_ui_key=shown;
  check(!ui.object(),"effect starts after real close_ui, with no remaining native menu object");
}

bool installation_fixture(Fixture& f,cat::RecipeId recipe=0x2001,bool info_page=false) {
  if(!f.reset(recipe,2)||!f.native_phase())return false;
  handoff={};dispatched_ticket={};native_update=false;
  frame_depth=1;update_depth=info_depth=choice_depth=station_depth=collection_depth=0;
  put(manager_bytes,0x40,1);
  put(f.view.data(),a::view_done,0);put(f.view.data(),a::view_active,1);
  put(f.view.data(),a::view_opened,1);put(f.view.data(),a::view_closed,0);
  if(info_page){
    put(manager_bytes,0x54,1);put(f.view.data(),a::view_count,1);put(f.view.data(),a::view_selected,0);
    rows.valid=true;rows.count=1;rows.rows[0]=ubc::read_selection_info(memory,address(f.info.data()));
  }
  const auto c=context();return c.valid&&c.dialog.open&&c.dialog.waiting&&complete_collection(c);
}
void install_gate_cases(Fixture& f){
  // Actual context and collection readers and the actual Root installation
  // branch. Here the adapter only counts calls instead of writing an engine hook.
  for(const auto id:{0u,1u,0x1001u,0x3001u,0x5001u,0x6001u}){
    check(installation_fixture(f,id),"other station has complete original dialog fixture");
    maybe_install_ammo(context());
    check(!ammo_installs&&!ammo_install_attempted,"all non-saw stations including anvil and ore never install ammo hooks");
  }
  for(unsigned mode=0;mode<18;++mode){
    check(installation_fixture(f),"fresh ready saw starts each negative installation case");
    switch(mode){
      case 0:put(f.game.data(),a::game_world,0);break;
      case 1:put(globals,4,0);break;
      case 2:f.foreign_dialogue();break;
      case 3:put(manager_bytes,0x40,0);break;
      case 4:put(f.view.data(),a::view_open_time,1);break;
      case 5:put(f.view.data(),a::view_done,1);put(f.view.data(),a::view_active,0);break;
      case 6:choice_collection=false;break;
      case 7:++choice_count;break;
      case 8:++rows.generation;rows.view=address(f.second_view.data());break;
      case 9:ui_created=true;break;
      case 10:frame_depth=0;break;
      case 11:frame_depth=2;break;
      case 12:update_depth=1;break;
      case 13:info_depth=1;break;
      case 14:choice_depth=1;break;
      case 15:station_depth=1;break;
      case 16:collection_depth=1;break;
      default:InterlockedExchange(&engine_thread,static_cast<LONG>(GetCurrentThreadId()+1u));break;
    }
    maybe_install_ammo(context());
    check(!ammo_installs&&!ammo_install_attempted&&!ammo_status_reads,
      "early world incomplete or foreign saw nested frame and effect depths cannot consume install attempt");
  }
  check(installation_fixture(f),"active handoff installation fixture");
  const auto* cb=callback_for(*f.descriptor,2);const auto plan=ubc::make_crafting_plan(f.prepared.page,f.descriptor->id,2);
  const ubc::CraftingRequest request{f.descriptor->id,2,f.prepared.key,f.prepared.materials,f.prepared.hands};
  check(cb&&handoff.begin(request,plan,f.prepared),"own pending navigation starts without engine calls");
  maybe_install_ammo(context());check(!ammo_installs&&!ammo_install_attempted,"own pending handoff blocks installation");
  check(installation_fixture(f),"active amount installation fixture");
  ubc::Sha256 hash;hash.update(f.body.data(),f.body.size());
  const ubc::AmountBody body{static_cast<std::uint32_t>(f.body.size()),hash.finish(),nullptr,0};
  const ubc::AmountMemory own_memory{{nullptr,ubc::windows_probe::copy_memory},nullptr,
    [](void*,std::uint32_t,std::size_t)noexcept{return true;},
    [](void*,std::uint32_t,std::size_t)noexcept{return true;},
    [](void*,std::uint32_t,const void*,const void*,std::size_t)noexcept{return false;}};
  check(amount_patch.begin(own_memory,address(f.body.data()),body,1)&&amount_patch.active(),
    "real amount transaction is active on private unchanged host bytes");
  maybe_install_ammo(context());check(!ammo_installs&&!ammo_install_attempted,"own live amount patch blocks installation independently of depths");
  check(amount_patch.finish(),"private no-change amount transaction restores normally");
  for(const bool info_page:{false,true})for(const bool accept:{false,true}){
    check(installation_fixture(f,0x2001,info_page),"fully ready saw original collection before first own UI");
    ammo_install_accept=accept;ammo_install_reenter=true;
    maybe_install_ammo(context());
    check(ammo_installs==1&&ammo_install_attempted&&ammo_install_thread==GetCurrentThreadId(),
      "correct saw installs once on actual owner and nested install callback cannot retry");
    check(ammo_status_reads==(accept?0u:1u),"only failed attempt reads bounded compatibility diagnostic");
    for(unsigned repeat=0;repeat<3;++repeat){++frame;maybe_install_ammo(context());}
    check(ammo_installs==1&&ammo_status_reads==(accept?0u:1u),
      "later ready frames never repeat successful or rejected installation");
  }
}
void registry_tests(){
  bc::Config config;check(configure_batch_symbols(config),"production registry helper collects every allowed template");
  for(const auto& recipe:cat::recipes)for(const auto quantity:{1u,2u}){
    const auto* cb=callback_for(recipe,quantity);unsigned matches=0;
    const auto* function=recipe.cool?(quantity==1?ccat::single.information:ccat::triple.information):
      recipe.grind?&recipe.grind->function:recipe.spirit?recipe.spirit->callback:cb?cb->function:nullptr;
    check(function!=nullptr,"each implemented route resolves its own callback");
    for(std::size_t i=0;function&&i<config.symbol_count;++i)if(config.symbols[i]==static_cast<int>(function->symbol))++matches;
    check(matches==1,"all 95 route N1/N2 callbacks are present exactly once, including grind");
  }
  for(const auto& recipe:acat::recipes){unsigned matches=0;
    for(std::size_t i=0;i<config.symbol_count;++i)if(config.symbols[i]==static_cast<int>(recipe.original_three.function->symbol))++matches;
    check(matches==1,"original anvil three-choice signatures remain registered without becoming own amount templates");
    check(!amount_callback(static_cast<std::int32_t>(recipe.original_three.function->symbol)),"native original three-choice is not an own patchable body");
  }
  check(!configure_batch_symbols(config),"production registry helper refuses populated caller buffer");
}
void original_three_cases(Fixture& f){
  for(const auto& recipe:cat::recipes)if(recipe.anvil){
    check(f.reset(recipe.id,3)&&f.native_phase(),"original anvil three-choice has actual native station and private parser");
    const auto& function=*recipe.anvil->original_three.function;
    ordinary_symbol=static_cast<std::int32_t>(function.symbol);
    f.symbol_table[function.symbol]=address(f.symbols[7].data());f.string(f.symbols[7].data(),0,function.name);
    put(f.choice.data(),0x14,function.symbol);choice_functions[1]=ordinary_symbol;
    handoff={};dispatched_ticket={};UbcBatchCallOriginal=reinterpret_cast<void*>(ordinary_three_script);
    bc::Config config;config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;
    config.select=select_batch_call;config.writer=execute_amount;
    check(configure_batch_symbols(config)&&bc::configure(config),"real registry includes canonical original three-choice signature");
    original_choice=original_three_choice;SetLastError(incoming_error);
    observe_choice(f.manager(),f.choice.data());
    check(choice_calls==1&&script_calls==1&&!writer_routes&&GetLastError()==original_error,
      "unowned original three-choice forwards exactly once through actual native wrapper and bridge");
    const std::array<std::uint8_t,6> expected{{64,5,0,0,0,60}};
    check(f.body==expected&&!bc::diagnostics().readers&&!bc::diagnostics().writer&&!bc::diagnostics().writes_disabled,
      "ordinary original neither patches private bytes nor retains a bridge lease");
    put(f.symbols[7].data(),0x20,1|(5u<<12u)|(1u<<16u));
    check(UbcBatchPrepare(reinterpret_cast<void*>(s::game_parser),ordinary_symbol,bc::choice_call_return)==0&&
      bc::diagnostics().writes_disabled&&!bc::diagnostics().readers&&!bc::diagnostics().writer,
      "changed original argc preserves generic ABI forwarding while disabling own writers");
    check(bc::configure(config),"independent next host case starts without prior signature quarantine");
  }
}
// This family cannot start a direct effect from an unrelated choice that has already been paid for.
void* __cdecl interrupted_tobacco_payment(void* raw,int point){
  auto& stock=*static_cast<tobacco::Stock*>(raw);
  if(point>=1)--stock[0];
  if(point>=2)--stock[1];
  if(point==3)--stock[0];
  RaiseException(0xe0544142,0,0,nullptr);
  return nullptr;
}
void tobacco_guard_cases(Fixture& f){
  tobacco_list.emplace(GetCurrentThreadId());
  for(int point:{0,1,2,3}){
    const tobacco::Stock before{{3,2,1,4}};auto stock=before;stock[3]+=2;
    bc::WriterScope throwing{};throwing.original=interrupted_tobacco_payment;
    throwing.call.parser=&stock;throwing.call.symbol=point;tobacco::Checks checks;
    const auto result=tobacco::settle(before,2,[&](auto& out){out=stock;return true;},
      [&](unsigned i,unsigned count,const auto&){stock[i]+=(i==3?-1:1)*static_cast<int>(count);return true;},
      [&](const auto&,bool& entered){void* value=nullptr;entered=true;
        check(!tobacco_vm_call(throwing,value),"native SEH returns to the settlement boundary");return false;},checks);
    check(result==tobacco::Settlement::unpaid&&stock==before,"SEH after any partial debit restores exact starting stock");
  }
  for(const auto& recipe:tobacco::recipes)for(const auto quantity:{1u,3u,6u}){
    check(f.reset(recipe.id,quantity,false),"ordinary original tobacco choice available to native fixture");
    const auto foreign=ubc::CraftingPage{ubc::CraftingPageKind::tobacco_choices,tobacco::owner.instance};
    check(!ubc::make_crafting_plan(foreign,recipe.id,quantity).valid(),"paid foreign page cannot create a tobacco order");
    const auto plan=ubc::make_crafting_plan(ubc::crafting_page(nav::Page::tobacco_info),recipe.id,quantity);
    check(plan.valid()&&plan.count==2&&plan.steps[0].function_symbol==20401&&plan.effect()->is_effect(),
      "every own quantity requires one owned opening before its child");
    bc::WriterScope scope{};scope.call.symbol=recipe.callback.function->symbol;
    amount_diagnostic={};diagnostic_probe=true;log_file=reinterpret_cast<HANDLE>(1);
    check(!execute_tobacco_amount(scope,recipe)&&!amount_diagnostic.entered&&
      !effect_executed&&amount_diagnostic.rejected,"missing ticket declines before writing");
    for(const char* field:{"writer.ticket","parser.current","parser.cursor","parser.stack_depth","legacy.use_symbol",
        "gate.resources","script.contract","lab.production","inventory.read","hand.same","debit.apple_path",
        "stock.product","module.insert_target","script.create_items","script.child_body","call.executed"}){
      char expected[128]{};
      std::snprintf(expected,sizeof(expected),"\"field\":\"%s\",\"soll\":\"",field);
      check(std::strstr(diagnostic_log,expected)!=nullptr,"independent rejection values reach the actual log output");
    }
    check(!tobacco_report.overflow,"complete tobacco diagnostic fits the log buffer");
    for(std::size_t i=0;i<tobacco_report.count;++i){
      const auto& row=tobacco_report.rows[i];
      char expected[256]{};
      std::snprintf(expected,sizeof(expected),"\"field\":\"%s\",\"soll\":\"%s\",\"ist\":\"%s\"",
        row.field,row.expected,row.actual);
      check(std::strstr(diagnostic_log,expected)!=nullptr,"each reported check reaches WriteFile with its expected and actual value");
    }
    check(std::strstr(diagnostic_log,"\"field\":\"completion.returned\",\"soll\":\"geprueft\",\"ist\":\"nicht erreicht\"")!=nullptr,
      "rejected ticket logs the unentered final VM call");
    log_file=INVALID_HANDLE_VALUE;diagnostic_probe=false;
    tobacco_owner={key(context()),7,1,2,address(f.info.data()),address(f.view.data()),static_cast<int>(recipe.callback.function->symbol)};
    InterlockedExchange(&tobacco_unpaid_page,1);
    bc::Call call{reinterpret_cast<void*>(s::game_parser),tobacco_owner.function,bc::choice_call_return,GetCurrentThreadId()};
    check(reject_unexpected_cool_info(call),"manual unpaid effect stays blocked after handoff cancellation");
    tobacco_choice_call=20402;check(reject_unexpected_cool_info(call),"changed back symbol never reaches an unpaid effect");
    call.symbol=20402;check(!reject_unexpected_cool_info(call),"bound back exits the unpaid page");
    tobacco_choice_call=tobacco_owner.function;call.symbol=20402;
    check(reject_unexpected_cool_info(call),"effect cannot switch to a different CallFunc symbol");
    call.symbol=tobacco_owner.function;call.caller=0;
    check(reject_unexpected_cool_info(call),"direct CallFunc cannot spend the unpaid ticket");
    InterlockedExchange(&tobacco_unpaid_page,0);tobacco_owner={};tobacco_choice_call=0;
  }
  tobacco_list.reset();
}
void wrapper_cases(Fixture& f){
  original_info=counted_info;original_choice=counted_choice;
  for(const auto& recipe:cat::recipes){
    check(f.reset(recipe.id,1,false),"five-station fixture without pending job");
    caller=f.choice_mode?a::choice_effect_return:a::info_effect_return;SetLastError(incoming_error);
    if(f.choice_mode)observe_choice(f.manager(),f.choice.data());else observe_info(f.info.data(),nullptr);
    check(info_calls+choice_calls==1&&seen_error==incoming_error&&GetLastError()==original_error,
      "every unrelated original effect forwards exactly once with LastError preserved");
    if(recipe.alchemy_extra||recipe.tobacco||recipe.spirit||recipe.bow||recipe.cool||recipe.grind)continue; // Separate positive reader and writer checks.
    check(f.reset(recipe.id),"five-station dispatched pure-value fixture");
    caller=f.choice_mode?a::choice_effect_return:a::info_effect_return;
    if(f.choice_mode)observe_choice(f.manager(),f.choice.data());else observe_info(f.info.data(),nullptr);
    check(!info_calls&&!choice_calls&&handoff.cancelled()&&handoff.active(),"missing native Update cannot execute or retire own pending effect");
    check(f.reset(recipe.id),"foreign-dialog guarded fixture");f.foreign_dialogue();
    if(f.choice_mode)observe_choice(f.manager(),f.choice.data());else observe_info(f.info.data(),nullptr);
    check(!info_calls&&!choice_calls&&handoff.cancelled(),"late old recipe remains blocked after positive foreign-dialog replacement");
    f.foreign_target();
    if(f.choice_mode)observe_choice(f.manager(),f.choice.data());else observe_info(f.info.data(),nullptr);
    check(info_calls+choice_calls==1,"positively different original foreign target stays ordinary");
    check(f.reset(recipe.id)&&f.native_phase(),"scoped native input fixture");
    check(observe_choice_event(f.callback(),nullptr,m::key_return)==1&&!input_calls,"physical Enter cannot confirm pending owned view");
    check(observe_choice_event(f.callback(),nullptr,m::key_escape)==1&&!input_calls&&handoff.cancelled(),"B cancels own chain without ending native selection");
    private_enter=true;
    check(observe_choice_event(f.callback(),nullptr,m::key_return)==1&&!input_calls&&!private_enter,"private flag cannot retry cancelled work");
    check(observe_choice_event(f.second_view.data()+a::view_callback,nullptr,m::key_return)==73&&input_calls==1,
      "different input callback remains usable while own view quarantined");
    check(f.reset(recipe.id)&&f.native_phase(),"pending view before lost production-kind evidence");
    f.symbol_table[ubc::lhiver::production_symbol]=0;
    check(!context().valid&&context().dialog.valid&&!foreign_dialog(context()),"only station profile evidence is unknown, original view remains bound");
    check(observe_choice_event(f.callback(),nullptr,m::key_return)==1&&!input_calls,
      "unknown production/kind at old view is never positive permission for ordinary native Enter");
  }
}
void anvil_ore_boundary_cases(Fixture& f){
  for(const auto& recipe:cat::recipes)if(recipe.anvil||recipe.ore)for(unsigned bad=0;bad<12;++bad){
    check(f.reset(recipe.id,2)&&f.native_phase(),"new-route negative starts from actual native context and page");
    auto* state=const_cast<ubc::CraftingState*>(state_for(injected,recipe.id));
    check(state!=nullptr,"new route has its own copied material state");if(!state)continue;
    const auto before=observation(context(),false);
    check(ubc::crafting_handoff_detail::same_resources(before.materials,handoff.request().material_snapshot)&&
      ubc::same_crafting_hands(before.hands,handoff.request().hands)&&!adapter_failed(),
      "negative boundary has positive resources and no unrelated failure latch before mutation");
    switch(bad){
      case 0:put(f.symbols[2].data(),0x18,0);break; // STARTSMITHING
      case 1:put(f.symbols[3].data(),0x18,1);put(f.symbols[4].data(),0x18,1);break;
      case 2:put(f.symbols[3].data(),0x18,recipe.anvil?0u:1u);put(f.symbols[4].data(),0x18,recipe.anvil?1u:0u);break;
      case 3:f.string(f.symbols[2].data(),0,"DIFFERENT_GLOBAL");break;
      case 4:put(f.symbols[2].data(),0x20,1|(3u<<12u));break;
      case 5:state->context_known=false;break;
      case 6:state->context_matches=false;break;
      case 7:state->actor_known=false;break;
      case 8:f.string(f.symbols[9].data(),0,"OTHER");break;
      case 9:injected.hands.right_tmp_level=0;break;
      case 10:injected.hands.interact_item=injected.hands.hands.left.object;break;
      default:injected.hands.right_slot_known=false;break;
    }
    if(bad<5)check(page(context())!=f.prepared.page,"real named scalar/page proof rejects changed smithing context");
    selected=&f;original_info=original_info_writer;original_choice=original_choice_writer;
    if(f.choice_mode)observe_choice(f.manager(),f.choice.data());else observe_info(f.info.data(),nullptr);
    check(!info_calls&&!choice_calls&&!writer_routes&&!script_calls&&handoff.cancelled(),
      "changed smithing category actor or hand proof blocks before original effect");
    // Repairing data cannot implicitly retry an already cancelled ticket.
    injected.hands=handoff.request().hands;*state=handoff.request().material_snapshot;
    put(f.symbols[2].data(),0x18,1);put(f.symbols[2].data(),0x20,1|(2u<<12u));f.string(f.symbols[2].data(),0,"STARTSMITHING");
    put(f.symbols[3].data(),0x18,recipe.anvil?1u:0u);put(f.symbols[4].data(),0x18,recipe.ore?1u:0u);
    if(f.choice_mode)observe_choice(f.manager(),f.choice.data());else observe_info(f.info.data(),nullptr);
    check(!info_calls&&!choice_calls&&!writer_routes&&!script_calls,
      "later valid smithing data never retries cancelled native selection");
  }
}
unsigned extra_boundary_calls=0,extra_native_calls=0;
void* __thiscall forbidden_extra_remove(void*,int,int){++extra_native_calls;return nullptr;}
void __thiscall forbidden_extra_restore_pop(void*,int){++extra_native_calls;}
void __cdecl forbidden_extra_restore_instance(void*){++extra_native_calls;}
void __thiscall forbidden_extra_restore_item(void*,int){++extra_native_calls;}
bool __cdecl check_extra_before_original(void*,xwriter::Phase phase,const xdebit::Job& job,xwriter::Boundary&) {
  ++extra_boundary_calls;
  check(phase==xwriter::Phase::before_original,"extra regression stops at the first genuine Executor boundary");
  check(bc::diagnostics().writer&&extra_executor->scope_evidence().valid&&
    !extra_executor->scope_evidence().scope.vm_depth&&!extra_executor->scope_evidence().scope.debit_depth,
    "before-original proof comes from actual bridge and Executor scopes");
  check(result_guard.state()==ubc::CraftingResultState::waiting&&!result_guard.permits_next(),
    "already started result remains waiting while the same original is admitted");
  const auto current=context();
  check(active_extra_admission(current)&&late_gate(),
    "same executing Extra request passes the real late gate after result begin");
  xwriter::Boundary proof;
  check(extra_boundary(nullptr,phase,job,proof)&&proof.native_handoff_verified&&
    proof.actual_info==address(selected->info.data())&&proof.runtime.scope.job==job.id&&
    proof.runtime.scope.writer_token==job.writer_token,
    "Root boundary positively rechecks native handoff under the real Executor lease");

  // Manager.info may be null or the old choice owner. Only the
  // actual observe_info argument is bound to the active writer.
  check(current_info_effect(current)==address(selected->info.data()),
    "actual Info argument, never manager owner, survives real wrapper to writer");
  const auto saved_lease=info_effect_lease;
  for(unsigned bad=0;bad<25;++bad){
    const auto saved_generation=generation;const auto saved_collection=rows.generation;
    const auto saved_condition=integer(address(selected->info.data()),0x28);
    switch(bad){
      case 0:info_effect_lease.actual_info=0;break;
      case 1:info_effect_lease.actual_info=address(selected->stale_info.data());break;
      case 2:++info_effect_lease.thread;break;
      case 3:++info_effect_lease.depth;break;
      case 4:++info_effect_lease.game;break;
      case 5:++info_effect_lease.key.generation;break;
      case 6:++info_effect_lease.key.thread;break;
      case 7:++info_effect_lease.key.world;break;
      case 8:++info_effect_lease.key.hero;break;
      case 9:++info_effect_lease.key.station;break;
      case 10:++info_effect_lease.key.manager;break;
      case 11:info_effect_lease.key.kind=cat::Station::stove;break;
      case 12:++info_effect_lease.ticket.id;break;
      case 13:++info_effect_lease.ticket.frame;break;
      case 14:++info_effect_lease.ticket.step;break;
      case 15:++info_effect_lease.ticket.view;break;
      case 16:info_effect_lease.ticket.offer.mode=basecat::Invocation::choice;break;
      case 17:++info_effect_lease.ticket.offer.owner;break;
      case 18:++info_effect_lease.ticket.offer.function;break;
      case 19:++info_effect_lease.ticket.offer.collection;break;
      case 20:++generation;break;
      case 21:++rows.generation;break;
      case 22:++info_effect_lease.ticket.offer.row;break;
      case 23:info_effect_lease.ticket.offer.valid=false;break;
      default:put(selected->info.data(),0x28,static_cast<std::uint32_t>(saved_condition+1));break;
    }
    xwriter::Boundary rejected;
    check(!current_info_effect(context())&&!active_extra_admission(current)&&
      !extra_boundary(nullptr,phase,job,rejected),
      "wrong info pointer thread depth ticket collection key callback or fresh metadata fails closed");
    info_effect_lease=saved_lease;generation=saved_generation;rows.generation=saved_collection;
    put(selected->info.data(),0x28,static_cast<std::uint32_t>(saved_condition));
  }
  check(current_info_effect(context())==address(selected->info.data()),
    "restored private lease fixture is usable only inside the unchanged original scope");

  // Reversible changes only inside this private host fixture. No native
  // dependency, inventory or ITEM admission is supplied by this callback.
  for(unsigned bad=0;bad<21;++bad){
    const auto saved_job=extra_active_job;const auto saved_call=extra_writer_scope->call;
    auto* const saved_scope=extra_writer_scope;
    switch(bad){
      case 0:++extra_active_job.id;break;
      case 1:++extra_active_job.writer_token;break;
      case 2:++extra_active_job.callback;break;
      case 3:++extra_active_job.owner;break;
      case 4:++extra_active_job.quantity;break;
      case 5:++extra_active_job.context.world;break;
      case 6:++extra_active_job.context.hero;break;
      case 7:++extra_active_job.context.mob;break;
      case 8:++extra_writer_scope->call.caller;break;
      case 9:++extra_writer_scope->call.thread;break;
      case 10:++extra_writer_scope->call.symbol;break;
      case 11:++info_depth;break;
      case 12:++choice_depth;break;
      case 13:++station_depth;break;
      case 14:++collection_depth;break;
      case 15:++resolved_read_depth;break;
      case 16:native_update=false;break;
      case 17:effect_authorized=false;break;
      case 18:effect_executed=true;break;
      case 19:extra_job=false;break;
      default:extra_writer_scope=nullptr;break;
    }
    const auto captured=capture(context());const auto* state=state_for(captured.snapshot,handoff.request().id);
    check(!active_extra_admission(current)&&state&&!state->contract&&!late_gate(),
      "changed job caller depth or physical context cannot inherit waiting-result admission");
    extra_active_job=saved_job;extra_writer_scope=saved_scope;extra_writer_scope->call=saved_call;
    info_depth=1;choice_depth=station_depth=collection_depth=resolved_read_depth=0;
    native_update=effect_authorized=extra_job=true;effect_executed=false;
  }
  check(active_extra_admission(current)&&late_gate(),"restoring private test inputs restores only this still-active admission");
  auto* material=const_cast<ubc::CraftingState*>(state_for(injected,handoff.request().id));
  check(material!=nullptr,"Extra request owns one explicit synthetic material rowset");
  if(material){
    ++material->materials[0].stored;
    check(active_extra_admission(current)&&!late_gate(),"active admission never waives the fresh material comparison");
    --material->materials[0].stored;
    material->contract=false;
    check(!late_gate(),"missing recipe proof is not repaired by the waiting-result exception");
    material->contract=true;
  }
  ++rows.generation;
  check(!late_gate(),"changed collection cannot reuse an otherwise current Extra scope");
  --rows.generation;
  end_path_valid=false;
  check(!late_gate(),"missing station end contract still blocks the same Extra request");
  end_path_valid=true;
  check(late_gate(),"unchanged before-original handoff remains admissible before controlled stop");
  // Intentionally false BEFORE dependency reads, amount patch, original,
  // N-1 debit or ITEM restore. This is no complete execute_extra_amount test.
  return false;
}
void* __cdecl extra_admission_writer(bc::WriterScope& scope) {
  const auto error=GetLastError();const auto& request=handoff.request();
  const auto* recipe=cat::find(request.id);check(recipe&&recipe->alchemy_extra,"Extra writer test owns an exact original info recipe");
  if(!recipe||!recipe->alchemy_extra)return nullptr;
  // Same explicit ReturnAddress seam as the Root wrapper fixture. The
  // registry already admitted info_call_return; this host has no engine PC.
  scope.call.caller=bc::info_call_return;
  const auto* ticket=handoff.current_ticket();
  check(result_guard.begin(request,{request.key.generation,frame,ticket?ticket->offer.collection:0,end_sequence,
    request.key.world,request.key.hero,request.key.station}),"actual result begin precedes actual ExtraExecutor entry");
  extra_job=true;extra_active_job={extra_context(context()),request.key.generation,request.key.generation,
    recipe->alchemy_extra->callback.function->symbol,static_cast<std::uint32_t>(recipe->alchemy_extra->owner.instance),request.quantity};
  extra_writer_scope=&scope;
  xwriter::Config config;config.owner_thread=GetCurrentThreadId();config.memory=memory;config.boundary=check_extra_before_original;
  config.remove=forbidden_extra_remove;config.restore_pop=forbidden_extra_restore_pop;
  config.restore_instance=forbidden_extra_restore_instance;config.restore_item=forbidden_extra_restore_item;
  extra_executor.emplace(config);
  check(extra_executor->diagnostics().ready&&!extra_executor->scope_evidence().valid&&
    !active_extra_admission(context())&&!late_gate(),
    "configured but inactive ExtraExecutor cannot waive waiting-result gate");
  xwriter::Request run;run.snapshot.id=recipe->alchemy_extra->id;run.snapshot.quantity=request.quantity;
  run.snapshot.context=extra_active_job.context;run.job=extra_active_job;run.actual_info=current_info_effect(context());
  check(run.actual_info==address(selected->info.data()),"ExtraExecutor receives actual Info through the production lease reader");
  const ubc::AmountMemory writable{memory};
  SetLastError(error);extra_executor->run(scope,amount_patch,writable,run);
  check(!extra_executor->outcome().original_entered&&!extra_executor->outcome().original_returned&&
    extra_executor->outcome().blocked&&!extra_executor->scope_evidence().valid&&!scope.restore_verified,
    "controlled boundary rejection never claims execution or restore success");
  extra_writer_scope=nullptr;
  check(!active_extra_admission(context())&&!late_gate(),"completed rejected scope cannot revive the waiting request");
  SetLastError(error);return nullptr;
}
void extra_admission_cases(Fixture& f){
  for(const auto& recipe:cat::recipes)if(recipe.alchemy_extra)for(const auto quantity:{1u,2u})for(unsigned owner=0;owner<3;++owner){
    check(f.reset(recipe.id,quantity)&&f.native_phase(),"Extra N1/N2 fixture binds its original category owner and flask hand");
    f.old_info_owner();if(owner==0)put(manager_bytes,0x28,address(f.info.data()));
    if(owner==1)put(manager_bytes,0x28,0);
    selected=&f;extra_boundary_calls=extra_native_calls=0;
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);bc::Config config;
    config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;
    config.select=select_batch_call;config.writer=extra_admission_writer;
    check(configure_batch_symbols(config)&&bc::configure(config),"Extra regression enters actual registered CallFunc writer");
    original_info=original_info_writer;SetLastError(incoming_error);observe_info(f.info.data(),nullptr);
    check(info_calls==1&&writer_routes==1&&extra_boundary_calls==1&&!script_calls&&!extra_native_calls,
      "all thirteen Extra families reach first boundary once and perform zero native effects");
    check(!bc::diagnostics().writer&&bc::diagnostics().writes_disabled&&!amount_patch.active()&&
      !extra_writer_scope&&!info_depth&&!choice_depth&&!effect_authorized&&!info_effect_lease.actual_info,
      "controlled rejection balances all leases and leaves own writer quarantined");
    check(result_guard.state()==ubc::CraftingResultState::failed&&!result_guard.permits_next(),
      "boundary-only regression cannot fabricate completed crafting result");
    check(GetLastError()==incoming_error,"controlled Extra pre-effect stop preserves caller LastError");
  }
}
unsigned info_lifetime_mode=0;
std::uint64_t previous_info_token=0,previous_info_writer_token=0;
cadmission::InfoIdentity previous_info_identity;
bool same_info_lease(const InfoEffectLease& a,const InfoEffectLease& b){
  return a.actual_info==b.actual_info&&a.thread==b.thread&&a.game==b.game&&a.depth==b.depth&&a.key==b.key&&
    a.token==b.token&&ubc::crafting_handoff_detail::same_ticket(a.ticket,b.ticket);
}
void __thiscall original_info_lease_writer(void*);
void* __cdecl info_lease_lifetime_writer(bc::WriterScope& scope){
  ++writer_routes;
  check(bc::diagnostics().writer&&current_info_effect(context())==address(selected->info.data()),
    "lifetime case has genuine Info wrapper and exclusive bcWriter lease");
  const auto saved=info_effect_lease;
  previous_info_identity={extra_context(context()),frame,saved.actual_info,s::information_manager};
  check(saved.token>previous_info_token&&scope.token>previous_info_writer_token&&
    info_lease_owner.current(previous_info_identity,saved.token),
    "actual root Info and exclusive Writer issue monotone tokens even for identical job/frame/pointer");
  previous_info_token=saved.token;previous_info_writer_token=scope.token;
  ++info_effect_lease.token;
  check(!current_info_effect(context()),"copied metadata cannot invent a live Info token");
  info_effect_lease=saved;
  if(info_lifetime_mode==1){
    // The handoff already blocks a repeated bound callback.
    observe_info(selected->info.data(),nullptr);
    check(info_calls==1&&same_info_lease(info_effect_lease,saved)&&info_depth==1&&effect_authorized&&
      !current_info_effect(context())&&handoff.cancelled(),
      "nested bound callback restores outer lease but never revives cancelled permission");
  }else if(info_lifetime_mode==2){
    SetLastError(original_error);RaiseException(synthetic_exception,0,0,nullptr);
  }else if(info_lifetime_mode==3){
    // A confirmed unrelated dialog remains ordinary. Its SEH must neither
    // inherit nor leak the outer lease. No material or engine effect in the host.
    selected->foreign_dialogue();
    __try {observe_info(selected->stale_info.data(),nullptr);}
    __except(GetExceptionCode()==synthetic_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){++script_exceptions;}
    put(manager_bytes,0x20,address(selected->hero.data()));
    check(script_exceptions==1&&info_calls==2&&same_info_lease(info_effect_lease,saved)&&info_depth==1&&
      effect_authorized&&!current_info_effect(context()),
      "nested ordinary SEH restores exact prior lease while cancelled outer job stays blocked");
  }
  scope.restore_verified=true;SetLastError(original_error);return nullptr;
}
void __thiscall original_info_lease_writer(void*){
  ++info_calls;
  if(info_depth==2){
    check(!info_effect_lease.actual_info&&!effect_authorized&&!current_info_effect(context()),
      "ordinary nested Info sees no borrowed own effect lease");
    SetLastError(original_error);RaiseException(synthetic_exception,0,0,nullptr);return;
  }
  const auto route=UbcBatchPrepare(reinterpret_cast<void*>(s::game_parser),dispatched_ticket.offer.function,bc::info_call_return);
  check(route==2,"lifetime case enters actual CallFunc registry before abnormal or nested path");
  if(route==2)UbcBatchWriter(reinterpret_cast<void*>(s::game_parser),dispatched_ticket.offer.function);
}
unsigned spirit_boundaries=0;
bool __cdecl check_spirit_before_original(void*,gwriter::Phase phase,const gdebit::Job& job,gwriter::Boundary&) {
  ++spirit_boundaries;
  check(phase==gwriter::Phase::before_original,"spirit executor enters its actual pre-original scope");
  check(active_spirit_admission(context())&&late_gate(),"waiting result permits only current spirit pre-original material check");
  gwriter::Boundary actual;
  check(spirit_boundary(nullptr,phase,job,actual)&&actual.native_handoff_verified&&
    actual.actual_info==address(selected->info.data()),"root spirit boundary binds real selected Info lease and current material check");
  const auto input=extra_read_input(context(),true);
  check(read_spirit_actual_info(nullptr,input)==actual.actual_info,"reader accessor returns actual leased pointer");
  for(unsigned bad=0;bad<7;++bad){
    const auto old=info_effect_lease;const auto saved=spirit_active_job;
    if(bad==0)info_effect_lease.actual_info=0;
    if(bad==1)++info_effect_lease.ticket.offer.function;
    if(bad==2)++info_effect_lease.thread;
    if(bad==3)++info_effect_lease.key.generation;
    if(bad==4)++spirit_active_job.branch_value;
    if(bad==5)++spirit_active_job.context.world;
    if(bad==6)effect_executed=true;
    gwriter::Boundary rejected;
    check(!active_spirit_admission(context()),"changed spirit branch or lease cannot reuse waiting-result admission");
    check(!spirit_boundary(nullptr,phase,job,rejected)||!rejected.native_handoff_verified,
      "spirit branch job context lease or executed-state mismatch cannot admit original");
    info_effect_lease=old;spirit_active_job=saved;effect_executed=false;
  }
  // Stop before dependencies, original, debit and ITEM restore. No injected
  // native/module proof is used to manufacture a positive full writer run.
  return false;
}
void* __cdecl spirit_admission_writer(bc::WriterScope& scope) {
  const auto error=GetLastError();scope.call.caller=bc::info_call_return;
  const auto& request=handoff.request();const auto* ticket=handoff.current_ticket();
  check(result_guard.begin(request,{request.key.generation,frame,ticket?ticket->offer.collection:0,end_sequence,
    request.key.world,request.key.hero,request.key.station}),"spirit result begins before executor and keeps effect gated");
  spirit_job=true;spirit_active_job={{extra_context(context()),request.key.generation,request.key.generation,
    20370,20368,request.quantity},request.material_snapshot.branch_value};spirit_writer_scope=&scope;
  gwriter::Config config;config.owner_thread=GetCurrentThreadId();config.memory=memory;config.boundary=check_spirit_before_original;
  config.remove=forbidden_extra_remove;config.restore_pop=forbidden_extra_restore_pop;
  config.restore_instance=forbidden_extra_restore_instance;config.restore_item=forbidden_extra_restore_item;
  spirit_executor.emplace(config);
  check(spirit_executor->diagnostics().ready&&!active_spirit_admission(context()),"inactive executor cannot bypass waiting-result guard");
  gwriter::Request run;run.snapshot.state.id=gcat::RecipeId::spirit;run.snapshot_serial=next_extra_serial();
  run.quantity=request.quantity;run.job=spirit_active_job;run.actual_info=current_info_effect(context());
  const ubc::AmountMemory writable{memory};
  spirit_executor->run(scope,amount_patch,writable,run);
  check(!spirit_executor->outcome().original_entered&&spirit_executor->outcome().blocked&&!scope.restore_verified,
    "controlled pre-original refusal never calls original or claims restoration");
  spirit_writer_scope=nullptr;
  check(!active_spirit_admission(context())&&!spirit_executor->scope_evidence().valid,"ended executor lease cannot revive old spirit request");
  SetLastError(error);return nullptr;
}
void spirit_admission_cases(Fixture& f){
  for(const auto quantity:{1u,2u}){
    check(f.reset(0x7001,quantity)&&f.native_phase(),"spirit fixture has bound Info metadata and selected branch");
    selected=&f;spirit_boundaries=0;f.old_info_owner();put(manager_bytes,0x28,0);
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);bc::Config config;
    config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;
    config.select=select_batch_call;config.writer=spirit_admission_writer;
    check(configure_batch_symbols(config)&&bc::configure(config),"spirit is admitted through actual registered CallFunc writer");
    original_info=original_info_writer;observe_info(f.info.data(),nullptr);
    check(spirit_boundaries==1&&!script_calls&&!info_effect_lease.actual_info&&!info_depth&&!effect_authorized,
      "spirit guarded entry visited once and actual Info finally removes lease without original effect");
  }
}
void info_lease_lifetime_cases(Fixture& f){
  for(unsigned mode=1;mode<=3;++mode){
    check(f.reset(0x400bu,2)&&f.native_phase(),"lease lifetime starts from genuine Extra original Info selection");
    f.old_info_owner();put(manager_bytes,0x28,0);selected=&f;info_lifetime_mode=mode;
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);bc::Config config;
    config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;
    config.select=select_batch_call;config.writer=info_lease_lifetime_writer;
    check(configure_batch_symbols(config)&&bc::configure(config),"lease lifetime configures original bridge writer");
    original_info=original_info_lease_writer;SetLastError(incoming_error);
    __try {observe_info(f.info.data(),nullptr);}
    __except(GetExceptionCode()==synthetic_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){++script_exceptions;}
    const auto returned_error=GetLastError();
    check(!info_effect_lease.actual_info&&!info_depth&&!effect_authorized&&!bc::diagnostics().writer&&
      !current_info_effect(context())&&!script_calls&&writer_routes==1,
      "SEH or normal exit returns no Info pointer authorization depth or bc writer lease");
    check(!info_lease_owner.current(previous_info_identity,previous_info_token),
      "native finally retires token, restoring old metadata never resurrects it");
    check(returned_error==original_error,"actual exceptional or nested return LastError survives all lease cleanup");
    check(handoff.cancelled()&&result_guard.state()==ubc::CraftingResultState::failed,
      "unclear nested or exceptional own Info cannot claim callback completion or retry");
    if(mode==2)check(script_exceptions==1&&bc::diagnostics().writes_disabled,
      "uncaught writer SEH propagates through observe_info finally and quarantines own writes");
  }
}
bool accepts_test_result();
void saw_actor_cases(Fixture& f){
  for(const auto& recipe:cat::recipes)if(recipe.station==cat::Station::saw)for(const auto quantity:{1u,2u}){
    check(f.reset(recipe.id,quantity)&&f.native_phase(),"saw actor fixture uses genuine private metadata and station context");
    put(f.symbols[9].data(),0x1c,quantity==1?0u:address(f.other_hero.data()));
    const auto strict=ubc::station_recipe_runtime::actor(memory,s::game_parser,*recipe.station_descriptor,address(f.hero.data()));
    check(strict.read_valid&&strict.requires_self&&!strict.matches,"unchanged primitive actor reader rejects idle foreign or null SELF");
    const auto preview=capture(context());const auto* state=state_for(preview.snapshot,recipe.id);
    check(state&&state->actor_known&&state->actor_matches&&ubc::crafting_block_reason(*state)==ubc::CraftingBlockReason::none,
      "saw preview uses verified physical Hero while retaining complete actor metadata");
    selected=&f;removal_mode=0;render_proof(f);UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);
    bc::Config config;config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;
    config.select=select_batch_call;config.writer=execute_amount;
    check(configure_batch_symbols(config)&&bc::configure(config),"saw actor case uses real registered writer");
    original_choice=original_choice_writer;SetLastError(incoming_error);observe_choice(f.manager(),f.choice.data());
    check(choice_calls==1&&writer_routes==1&&script_calls==1,"original SELF binding between wrapper and writer permits exactly one saw effect");
    check(!amount_patch.active()&&!bc::diagnostics().writer&&GetLastError()==original_error,
      "saw actor transition keeps restore and LastError intact");
  }
  for(unsigned bad=0;bad<8;++bad){
    check(f.reset(0x2001u,2)&&f.native_phase(),"saw metadata negative fixture");
    if(bad==0)f.string(f.symbols[9].data(),0,"OTHER");
    if(bad==1)put(f.symbols[9].data(),0x20,(5u<<12u)|(1u<<16u));
    if(bad==2)put(f.symbols[9].data(),0x38,0);
    if(bad==3)f.symbol_table[1679]=0;
    if(bad==4)put(f.symbols[0].data(),0x1c,address(f.other_hero.data()));
    if(bad==5)f.foreign_dialogue();
    if(bad==6)put(manager_bytes,0x24,address(f.other_hero.data()));
    if(bad==7)injected.hands.hands.right.object=0;
    const auto preview=capture(context());const auto* state=state_for(preview.snapshot,0x2001u);
    check(!ubc::valid_crafting_snapshot(preview.snapshot)||!state||ubc::crafting_block_reason(*state)!=ubc::CraftingBlockReason::none,
      "unknown metadata, wrong Hero, foreign manager or missing hand cannot inherit saw preview admission");
  }
  for(const auto self:{0u,address(f.other_hero.data())}){
    check(f.reset(0x2001u,2)&&f.native_phase(),"late SELF rejection begins with correct physical station");
    put(f.symbols[9].data(),0x1c,self);selected=&f;removal_mode=0;bind_choice_self=false;render_proof(f);
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);bc::Config config;
    config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;config.select=select_batch_call;config.writer=execute_amount;
    check(configure_batch_symbols(config)&&bc::configure(config),"late SELF rejection uses real writer registry");
    original_choice=original_choice_writer;SetLastError(incoming_error);observe_choice(f.manager(),f.choice.data());
    check(choice_calls==1&&writer_routes==1&&!script_calls&&!ammo_admits,"fresh inner SELF mismatch rejects before amount mutation and ammo admission");
    check((f.body==std::array<std::uint8_t,6>{{64,5,0,0,0,60}})&&!amount_patch.active()&&!bc::diagnostics().writer,
      "rejected fresh actor leaves bytecode and writer ownership unchanged");
  }
}
void anvil_actor_cases(Fixture& f){
  for(const auto id:{0x5001u,0x6001u})for(const auto quantity:{1u,2u})for(const auto rebound:{false,true}){
    check(f.reset(id,quantity)&&f.native_phase(),"anvil actor fixture binds genuine ordinary Choice or ore Info context");
    put(f.symbols[9].data(),0x1c,quantity==1?0u:address(f.other_hero.data()));
    const auto strict=f.descriptor->anvil?
      ubc::anvil_recipe_runtime::actor(memory,s::game_parser,ubc::anvil_recipe_runtime::assemble(*f.descriptor->anvil),address(f.hero.data())):
      ubc::ore_recipe_runtime::actor(memory,s::game_parser,ubc::ore_recipe_runtime::assemble(*f.descriptor->ore),address(f.hero.data()));
    check(strict.read_valid&&strict.requires_self&&!strict.matches,"both primitive anvil actors reject idle null or foreign SELF");
    const auto preview=capture(context());const auto* state=state_for(preview.snapshot,id);
    check(state&&state->actor_known&&state->actor_matches&&ubc::crafting_block_reason(*state)==ubc::CraftingBlockReason::none,
      "bound physical Hero permits ordinary and ore preview before original SELF binding");
    selected=&f;removal_mode=0;bind_anvil_self=rebound;render_proof(f);
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);bc::Config config;
    config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;config.select=select_batch_call;config.writer=execute_amount;
    check(configure_batch_symbols(config)&&bc::configure(config),"anvil actor transition uses actual registered CallFunc writer");
    original_info=original_info_writer;original_choice=original_choice_writer;SetLastError(incoming_error);
    if(f.choice_mode)observe_choice(f.manager(),f.choice.data());else observe_info(f.info.data(),nullptr);
    check(info_calls+choice_calls==1&&writer_routes==1&&script_calls==(rebound?1u:0u),
      "only fresh actual SELF binding permits one ordinary or ore effect at N1 and N2");
    check(!rebound?handoff.cancelled():!handoff.cancelled(),"missing inner SELF binding cancels without an original script effect");
    check((f.body==std::array<std::uint8_t,6>{{64,5,0,0,0,60}})&&!amount_patch.active()&&!bc::diagnostics().writer&&!ammo_admits,
      "anvil actor admission and rejection preserve bytecode writer ownership and unrelated ammo state");
  }
  for(const auto id:{0x5001u,0x6001u})for(unsigned bad=0;bad<7;++bad){
    check(f.reset(id,2)&&f.native_phase(),"anvil physical-preview negative fixture");
    if(bad==0)f.string(f.symbols[9].data(),0,"OTHER");
    if(bad==1)f.foreign_dialogue();
    if(bad==2)put(manager_bytes,0x24,address(f.other_hero.data()));
    if(bad==3)put(f.position.data(),0x44,address(f.other_hero.data()));
    if(bad==4)put(f.symbols[0].data(),0x1c,address(f.other_hero.data()));
    if(bad==5)end_path_valid=false;
    if(bad==6)injected.hands.right_tmp_level=0;
    const auto preview=capture(context());const auto* state=state_for(preview.snapshot,id);
    check(!ubc::valid_crafting_snapshot(preview.snapshot)||!state||ubc::crafting_block_reason(*state)!=ubc::CraftingBlockReason::none,
      "missing actor metadata Hero manager station end or hand proof cannot inherit anvil preview admission");
  }
}
void writer_cases(Fixture& f){
  for(const auto& recipe:cat::recipes)for(const auto quantity:{1u,2u}){
    if(recipe.alchemy_extra||recipe.tobacco||recipe.spirit||recipe.bow||recipe.cool||recipe.grind)continue;
    check(f.reset(recipe.id,quantity)&&f.native_phase(),"base station anvil and ore N1/N2 snapshots use correct station/page/hand resources");
    selected=&f;removal_mode=0;
    render_proof(f);
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);
    bc::Config config;config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;
    config.select=select_batch_call;config.writer=execute_amount;
    check(configure_batch_symbols(config)&&bc::configure(config),"real production symbol set configures exclusive writer registry");
    original_info=original_info_writer;original_choice=original_choice_writer;
    SetLastError(incoming_error);
    if(f.choice_mode)observe_choice(f.manager(),f.choice.data());else observe_info(f.info.data(),nullptr);
    if(info_calls+choice_calls!=1||writer_routes!=1||script_calls!=1)
      std::fprintf(stderr,"DIAG recipe=%u N=%u info=%u choice=%u writer=%u script=%u handoff=%u issue=%u result=%u resultissue=%u\n",
        recipe.id,quantity,info_calls,choice_calls,writer_routes,script_calls,static_cast<unsigned>(handoff.state()),
        static_cast<unsigned>(handoff.issue()),static_cast<unsigned>(result_guard.state()),static_cast<unsigned>(result_guard.issue()));
    check(info_calls+choice_calls==1&&writer_routes==1&&script_calls==1,"each native selected effect executes one patched original call");
    check(GetLastError()==original_error,"original output LastError survives scope-close and patch-finally cleanup");
    check(!removed_reads&&!post_remove_choice.active()&&!info_depth&&!choice_depth&&!effect_authorized,
      "native callback restores scope and never rereads removed Choice/node");
    const std::array<std::uint8_t,6> expected{{64,5,0,0,0,60}};
    check(f.body==expected&&!amount_patch.active()&&!bc::diagnostics().writer&&!bc::diagnostics().writes_disabled,
      "complete original own bytes and writer lease return after one effect");
    check(result_guard.state()==ubc::CraftingResultState::waiting&&!result_guard.permits_next()&&
      handoff.state()==ubc::CraftingHandoffState::result_pending,
      "callback and byte restore still require separate real material/end result");
    check(accepts_test_result(),"only after root's positive restore can a later matching value fixture verify completion");
    if(recipe.station==cat::Station::saw&&quantity>1){
      check(ammo_admits==1&&ammo_enters==1&&ammo_closes==1&&admitted_proof.ui_available,
        "multi-lot ammo uses saved real UI proof despite already closed menu");
      check(!active_ammo_scope.active,"native finally clears active ammo scope only after close");
    }else check(!ammo_admits&&!ammo_enters&&!ammo_closes,"non-ammo and N1 leave the ammo receipt path untouched");
  }
}
bool accepts_test_result() {
  // Our value fixture provides NO native stock or end evidence. It checks
  // only whether Root released the separate result guard after a real restore.
  auto copy=result_guard;const auto& request=handoff.request();const auto* d=cat::find(request.id);
  const bool ended=d->completion==cat::Completion::end_production;
  if(ended)check(copy.end_after({true,true,request.key.generation,end_sequence+1,frame+1,
    request.key.world,request.key.hero,request.key.station}),"synthetic completed value witness is explicitly separate from native world proof");
  ubc::CraftingResultObservation v;v.generation=request.key.generation;v.frame=frame+2;
  v.stable_boundary_verified=v.identity_known=v.station_known=v.production_known=true;
  v.world=request.key.world;v.hero=request.key.hero;v.station=ended?0:request.key.station;
  v.production=ended?0:ubc::crafting_result_detail::production(*d);
  v.selection_known=v.selection_owner_verified=v.selection_open=v.selection_waiting=v.collection_known=true;
  v.collection=rows.generation+1;v.stock_known=v.hands_known=true;v.hands=request.hands;
  if(ended){v.hands={};v.hands.hands={{0,-1,-1,0},{0,-1,-1,0}};v.hands.interact_item_known=true;}
  std::array<ubc::RecipeResultItem,cat::target_capacity> stock{};
  for(std::size_t i=0;i<request.material_snapshot.target_count;++i){const auto& item=request.material_snapshot.materials[i];
    stock[i]={item.symbol,std::int64_t{item.stored}+item.consumed_reserve-std::int64_t{item.unit_cost}*request.quantity+
      std::int64_t{item.unit_output}*request.quantity+(ended?item.tool_reserve:0)};}
  v.items={stock.data(),request.material_snapshot.target_count};
  return copy.observe(v)==ubc::CraftingResultState::verified;
}
void rejection_cases(Fixture& f){
  const auto id=static_cast<cat::RecipeId>(scat::RecipeId::arrows);
  for(unsigned bad=0;bad<8;++bad){
    check(f.reset(id,2)&&f.native_phase(),"ammo-negative fixture has genuine native writer boundaries");
    selected=&f;removal_mode=bad<4?bad+1:0;render_proof(f,bad!=4);
    if(bad==5)++request_ui_generation;
    if(bad==6)++request_ui_key.station;
    if(bad==7)damage_after_admit=true;
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);bc::Config config;
    config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;config.select=select_batch_call;config.writer=execute_amount;
    check(configure_batch_symbols(config)&&bc::configure(config),"negative case uses real registry and writer");
    original_choice=original_choice_writer;observe_choice(f.manager(),f.choice.data());
    check(choice_calls==1&&writer_routes==1&&!script_calls,"changed removal/UI proof or failed patch never executes recipe");
    check(result_guard.state()==ubc::CraftingResultState::failed&&adapter_failed(),
      "failed admitted or selected attempt latches visible failure without implicit retry");
    check(!removed_reads&&!amount_patch.active()&&!bc::diagnostics().writer&&!bc::diagnostics().writes_disabled,
      "pre-effect rejection leaves native removal and writer lifetime balanced");
    if(bad==7)check(ammo_admits==1&&ammo_armed&&!ammo_enters&&!ammo_closes,
      "successful admission followed by begin rejection cannot silently leave healthy-looking future ammo");
  }
  for(const auto& recipe:cat::recipes)if(recipe.completion==cat::Completion::end_production){
    if(recipe.alchemy_extra||recipe.tobacco||recipe.spirit||recipe.bow||recipe.cool||recipe.grind)continue;
    check(f.reset(recipe.id,2)&&f.native_phase(),"EndProduction fixture starts with current bound native selection");
    selected=&f;removal_mode=0;end_path_valid=false;
    original_choice=original_choice_writer;original_info=original_info_writer;
    if(f.choice_mode)observe_choice(f.manager(),f.choice.data());else observe_info(f.info.data(),nullptr);
    check(!choice_calls&&!info_calls&&!writer_routes&&!script_calls&&handoff.cancelled(),
      "unproved native end path rejects the selected effect before any material mutation");
  }
}
void native_fault_cases(Fixture& f){
  for(unsigned fault=0;fault<2;++fault){
    check(f.reset(static_cast<cat::RecipeId>(scat::RecipeId::arrows),2)&&f.native_phase(),"native fault fixture has one allowed patched effect");
    selected=&f;removal_mode=0;render_proof(f);script_raises=fault==0;script_changes_operand=fault==1;
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);bc::Config config;
    config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;config.select=select_batch_call;config.writer=execute_amount;
    check(configure_batch_symbols(config)&&bc::configure(config),"fault case enters actual writer registry");
    original_choice=original_choice_writer;SetLastError(incoming_error);observe_choice(f.manager(),f.choice.data());
    check(script_calls==1&&ammo_enters==1&&ammo_closes==1&&!active_ammo_scope.active&&!bc::diagnostics().writer,
      "exception or restore failure still closes actual ammo scope and writer exactly once");
    check(GetLastError()==original_error,"abnormal native path also preserves original LastError");
    check(!accepts_test_result(),"restored bytes alone or failed restore cannot authorize an unreturned/invalid effect result");
    if(!fault){
      const std::array<std::uint8_t,6> original{{64,5,0,0,0,60}};
      check(script_exceptions==1&&f.body==original&&!amount_patch.active()&&!bc::diagnostics().writes_disabled,
        "real outer finally restores bytes even when synthetic original raises SEH");
    }else{
      std::uint32_t actual=0;std::memcpy(&actual,f.body.data()+1,4);
      check(actual==301&&bc::diagnostics().writes_disabled&&result_guard.state()==ubc::CraftingResultState::failed&&
        result_guard.issue()==ubc::CraftingResultIssue::restore,
        "foreign operand stays untouched while failed restore permanently quarantines later own writes");
    }
  }
}
unsigned destroy_forward_calls=0;
void cool_station_context_cases(Fixture& f){
  for(const auto target:{0,1}){
    check(f.reset(0x3001,1)&&f.native_phase(),"known forge context prepares only private station metadata");
    put(f.station.data(),0x1fc,static_cast<std::uint32_t>(target));
    check(context().valid==(target==1),"forge still requires its original target1 contract");
    put(f.symbols[1].data(),0x18,13);
    f.string(f.station.data(),0x1e0,"SMITHCOOL");f.string(f.station.data(),0x1b8,"BSCOOL");
    f.string(f.station.data(),0x1a4,"");f.string(f.station.data(),0x1cc,"B_USECOOL");
    put(f.station.data(),0x200,0);
    check(context().valid&&context().kind==cat::Station::cool,"exact cooling profile accepts observed S1 target0 and ordinary target1");
    for(const auto bad_target:{-1,2}){
      put(f.station.data(),0x1fc,static_cast<std::uint32_t>(bad_target));
      check(!context().valid,"unknown or foreign cooling target remains rejected");
    }
    put(f.station.data(),0x1fc,static_cast<std::uint32_t>(target));
    for(const auto offset:{0x1f4u,0x1f8u,0x200u}){
      const auto original=integer(address(f.station.data()),offset);
      put(f.station.data(),offset,static_cast<std::uint32_t>(original+1));
      check(!context().valid,"cooling still requires exact active state, state count and rewind");
      put(f.station.data(),offset,static_cast<std::uint32_t>(original));
    }
    f.string(f.station.data(),0x1cc,"B_USESTOVE");
    check(!context().valid,"target0 exception cannot admit another condition profile");
  }
}
void grind_station_context_cases(Fixture& f){
  check(f.reset(0x3001,1)&&f.native_phase(),"grind profile uses only private native station metadata");
  put(f.symbols[1].data(),0x18,11);
  f.string(f.station.data(),0x1e0,"SMITHSHARP");f.string(f.station.data(),0x1b8,"BSSHARP");
  f.string(f.station.data(),0x1a4,"");f.string(f.station.data(),0x1cc,"");put(f.station.data(),0x200,0);
  const auto mob=address(f.station.data());
  check(context().valid&&context().kind==cat::Station::grind&&matches_station_profile(mob,cat::Station::grind),
    "observed grind target1 and exact SMITHSHARP/BSSHARP profile are accepted");
  for(const auto target:{-1,0,1,2}){
    put(f.station.data(),0x1fc,static_cast<std::uint32_t>(target));
    check(context().valid==(target==1),"grind does not inherit the cooling target0 exception");
  }
  put(f.station.data(),0x1fc,1);
  for(const auto offset:{0x1f4u,0x1f8u,0x200u}){
    const auto original=integer(mob,offset);put(f.station.data(),offset,static_cast<std::uint32_t>(original+1));
    check(!context().valid,"changed grind state stateNum or rewind fails");put(f.station.data(),offset,static_cast<std::uint32_t>(original));
  }
  for(const auto offset:{0x1e0u,0x1b8u,0x1a4u,0x1ccu}){
    const auto saved=f.station;f.string(f.station.data(),offset,"FOREIGN");
    check(!context().valid&&!matches_station_profile(mob,cat::Station::grind),"all four grind station names remain exact guards");f.station=saved;
  }
  check(!grind_modules_current()&&!grind_station_proof(context(),{},true).station_hand_end_verified,
    "station metadata alone supplies neither loaded-module nor hand/end admission");
  for(const auto& recipe:cat::recipes)if(recipe.grind){
    const auto& function=recipe.grind->function;
    f.symbol_table[function.symbol]=address(f.symbols[7].data());f.string(f.symbols[7].data(),0,function.name);
    put(f.symbols[7].data(),0x20,(5u<<12u)|(1u<<16u));put(f.symbols[7].data(),0x1c,0);put(f.symbols[7].data(),0x38,0);
    bc::Call call{reinterpret_cast<void*>(s::game_parser),static_cast<int>(function.symbol),bc::choice_call_return,GetCurrentThreadId()};
    check(batch_signature(call),"each grind callback has its exact actual native symbol signature");
    put(f.symbols[7].data(),0x20,1|(5u<<12u)|(1u<<16u));
    check(!batch_signature(call),"changed grind argc rejects own native dispatch");
    put(f.symbols[7].data(),0x20,(5u<<12u)|(1u<<16u));
    bc::WriterScope scope;scope.call=call;
    check(!verify_grind_amount(scope,recipe)&&!effect_executed,
      "metadata and old synthetic catalogue cannot fabricate actual grind Writer Item Skill or Destroy proofs");
    f.symbol_table[function.symbol]=0;
  }
  const auto k=grind_key(context());
  // Default, handoff and writer each retain their own fresh snapshot. This narrow host
  // fixture tests lifetime only. It cannot establish a synthetic positive item path.
  handoff={};frame_depth=1;native_update=false;update_depth=0;result_guard={};grind_catalogue_captures=0;
  check(grind_frame_capture_quiet(),"own outer frame admits only explicit shared catalogue lifetime");
  {
    const GrindCaptureLease first{k,false};check(first.value&&grind_capture_leased,"persistent full capture has one scoped owner");
    check(grind_catalogue_captures==1,"own catalogue lease actually invokes the existing reader exactly once");
    if(first.value){const auto saved=first.value->key;
      const GrindCaptureLease nested{k,false};check(!nested.value&&ubc::grind_recipe_snapshot_detail::same_key(first.value->key,saved),
        "nested catalogue cannot replace the first owner's complete object copy");
      for(unsigned part=0;part<8;++part){auto changed=k;
        switch(part){case 0:++changed.generation;break;case 1:++changed.frame;break;case 2:++changed.thread;break;
          case 3:++changed.world;break;case 4:++changed.hero;break;case 5:++changed.station;break;
          case 6:++changed.manager;break;case 7:++changed.production_kind;break;}
        check(!first.view(changed)&&grind_catalogue_captures==1,
          "every complete GrindKey component still binds the direct capture");}
      ++frame;check(!first.view(k),"even identical stored key cannot survive a changed actual outer frame");--frame;
      check(ubc::grind_recipe_snapshot_detail::same_key(first.value->key,saved),"all rejected consumers preserve the original catalogue value");
    }
  }
  check(!grind_capture_leased,"full capture lease is released after projection");
  {const GrindCaptureLease restored{k,false};check(restored.value&&grind_catalogue_captures==2,
    "after outer block release restored-view capture performs its own new reader call");}
  {const GrindCaptureLease writer{k,false};check(writer.value&&grind_catalogue_captures==3,
    "later original lease constructor remains a separate new acquisition");}
  check(!grind_capture_leased,"all frame and subsequent own captures leave persistent storage released");
  const auto current=context();grind_preview_scope={current,17,true,false};
  shpreview::FrameObservation observed;const auto reads=preview_memory_reads;
  check(observe_grind_preview_scope(&grind_preview_scope,observed)&&shpreview::detail::valid_frame(observed,k.thread),
    "real local scope observer binds actual own thread generation frame and depth");
  const auto expected=observed;
  for(unsigned i=0;i<1024;++i)check(observe_grind_preview_scope(&grind_preview_scope,observed)&&
    shpreview::detail::same_frame(observed,expected),"unchanged local scope is repeatable");
  check(preview_memory_reads==reads,"per-read native scope probes make no process or Engine memory read");
  ++frame;observe_grind_preview_scope(&grind_preview_scope,observed);check(observed.key.frame==frame&&observed.key.frame!=expected.key.frame,
    "observer samples actual frame instead of echoing expected key");--frame;
  ++generation;observe_grind_preview_scope(&grind_preview_scope,observed);check(observed.key.generation==generation&&observed.key.generation!=expected.key.generation,
    "observer samples actual generation");--generation;
  for(auto* depth:{&update_depth,&info_depth,&choice_depth,&station_depth,&collection_depth,&resolved_read_depth,&cool_destroy_depth,&grind_preview_input_depth}){
    ++*depth;observe_grind_preview_scope(&grind_preview_scope,observed);
    check(!shpreview::detail::valid_frame(observed,k.thread),"every active own callback and input depth rejects copied evidence");--*depth;}
  frame_depth=2;observe_grind_preview_scope(&grind_preview_scope,observed);
  check(!shpreview::detail::valid_frame(observed,k.thread),"recursive frame is not outer preview scope");frame_depth=1;
  ui_created=ui_owned=true;grind_ui_trace_active=false;
  for(unsigned i=0;i<100;++i)observe_menu_event(ui.object(),nullptr,0);
  check(grind_preview_input_serial==100&&grind_preview_input_depth==0,"every actual no-op own input is counted beyond the readiness trace budget");
  observe_grind_preview_scope(&grind_preview_scope,observed);
  check(!shpreview::detail::same_frame(observed,expected),"completed no-op inputs still invalidate the prior scope identity");
  {const GrindCaptureLease forbidden{k,false};check(!forbidden.value&&grind_preview_scope.failed&&grind_catalogue_captures==3,
    "direct default or writer acquisition cannot reuse or replace active preview storage");}
  check(!observe_grind_preview_scope(&grind_preview_scope,observed),"detected direct reentry poisons current preview scope");
  grind_preview_scope={};grind_preview_input_serial=UINT64_MAX;
  observe_menu_event(ui.object(),nullptr,0);
  check(grind_preview_overflow&&grind_preview_input_serial==UINT64_MAX&&!grind_preview_input_depth,"input serial overflow stays closed without wrap or duplicate dispatch");
  grind_preview_input_serial=0;grind_preview_overflow=false;ui_created=ui_owned=false;
  {const GrindPreviewLease failed{current};check(!failed.view(k),"missing real native module setup cannot publish a preview");
    check(failed.failure_stage==1&&failed.read_issue==-1&&!failed.statistics.spans,
      "missing preview reader reports prerequisites without inventing a readset failure or stale counts");
    const auto before=grind_catalogue_captures;const auto invalid=capture(current,&failed);
    check(!ubc::valid_crafting_snapshot(invalid.snapshot)&&grind_catalogue_captures==before,"failed explicit preview has no silent direct-capture fallback");}
  check(!grind_preview_scope.active,"failed preview leaves no live callback context after lexical scope");
  {const GrindCaptureLease later{k,false};check(later.value&&grind_catalogue_captures==4,"later direct capture remains genuinely new after preview lifetime");}
  frame_depth=0;
  std::printf("GrindCatalogueCapture Windows-x86: %zu bytes, persistent workspace\n",sizeof(ubc::GrindRecipeCatalogueCapture));
}
void grind_owner_metadata_cases(){
  for(const auto& owner:ubc::grind_menu_metadata::owners){
    ubc::SelectionInfo info;info.valid=true;info.instance=owner.instance;info.number=owner.number;info.npc=owner.npc;
    info.important=owner.important;info.trade=owner.trade;info.permanent=owner.permanent;
    info.condition=static_cast<int>(owner.condition->symbol);info.function=static_cast<int>(owner.information->symbol);
    check(owner_for(info.instance)==&owner&&linked(info),"six derived grind INFO owners bind exact current fields");
    for(unsigned bad=0;bad<8;++bad){auto changed=info;
      switch(bad){case 0:changed.instance=-1;break;case 1:changed.number++;break;case 2:changed.npc++;break;
        case 3:changed.important=1;break;case 4:changed.trade=1;break;case 5:changed.permanent=0;break;
        case 6:changed.condition++;break;case 7:changed.function++;break;}
      check(!linked(changed),"changed grind INFO identity or action is not admitted by name alone");
    }
  }
}
struct GrindAmountHost {
  std::array<std::uint8_t,308> bytes{};
  shcat::Plan numerical{};
  unsigned calls=0;bool raises=false;
  std::int32_t observed_skill=0;
  static bool copy(void* raw,std::uint32_t at,void* out,std::size_t size) noexcept {
    auto& f=*static_cast<GrindAmountHost*>(raw);const auto start=address(f.bytes.data());
    if(at<start||at-start>f.bytes.size()||size>f.bytes.size()-(at-start))return false;
    std::memcpy(out,f.bytes.data()+at-start,size);return true;
  }
  static bool identity(void* raw,std::uint32_t at,std::size_t size) noexcept {
    const auto& f=*static_cast<GrindAmountHost*>(raw);return at==address(f.bytes.data())&&size==f.bytes.size();
  }
  static bool writable(void* raw,std::uint32_t at,std::size_t size) noexcept {
    const auto& f=*static_cast<GrindAmountHost*>(raw);const auto start=address(f.bytes.data());
    return at>=start&&at-start<=f.bytes.size()&&size<=f.bytes.size()-(at-start);
  }
  static bool replace(void* raw,std::uint32_t at,const void* old,const void* next,std::size_t size) noexcept {
    auto& f=*static_cast<GrindAmountHost*>(raw);if(!writable(raw,at,size))return false;
    auto* p=f.bytes.data()+at-address(f.bytes.data());if(std::memcmp(p,old,size))return false;
    std::memcpy(p,next,size);return true;
  }
};
void* __cdecl grind_amount_original(void* raw){
  auto& f=*static_cast<GrindAmountHost*>(raw);++f.calls;
  const auto word=[&](std::size_t at){return rt::detail::u32(f.bytes.data()+at);};
  check(GetLastError()==incoming_error,"grind amount scope preserves incoming LastError");
  for(std::size_t i=0;i<f.numerical.count;++i){const auto& op=f.numerical.operands[i];
    check(word(op.offset)==f.numerical.quantity*op.units-op.reserved,"actual patched grind operand comes from live Plan value copy");}
  check(word(151)==100&&word(293)==1,"original skill100 comparison and Cold16613 count1 are never scaled");
  const auto after=std::int64_t{f.numerical.skill_before}+(f.numerical.skill_before<100?word(167):0);
  check(after>=INT32_MIN&&after<=INT32_MAX,"actual progress operand stays in signed32 range");
  f.observed_skill=static_cast<std::int32_t>(after);SetLastError(original_error);
  if(f.raises)RaiseException(synthetic_exception,0,0,nullptr);
  return reinterpret_cast<void*>(73);
}
void grind_amount_host_cases(const char* path){
  // Optional private DAT bodies. No embedded game bytes. This checks Windows
  // AmountPatch/SEH and skill operands without authorizing a native writer.
  std::FILE* file=nullptr;const auto opened=fopen_s(&file,path,"rb");
  check(opened==0&&file!=nullptr,"private canonical grind bodies opened");if(!file)return;
  std::uint32_t count=0;check(std::fread(&count,4,1,file)==1&&count==shcat::recipes.size(),"private fixture contains exactly22 bodies");
  for(const auto& recipe:shcat::recipes){
    std::uint32_t callback=0;std::array<std::uint8_t,308> original{};
    if(std::fread(&callback,4,1,file)!=1||std::fread(original.data(),1,original.size(),file)!=original.size()){
      check(false,"private canonical grind body complete");break;}
    ubc::Sha256 hash;hash.update(original.data(),original.size());
    const bool canonical=callback==recipe.function.symbol&&hash.finish()==recipe.function.body_sha256;
    check(canonical,"each private grind body matches exact original callback hash");if(!canonical)continue;
    for(const auto skill:{INT32_MIN,-10,98,99,100,101,INT32_MAX})for(const auto n:{1u,2u,5u}){
      shcat::Evidence before;before.callback=callback;before.stable_boundary_verified=true;
      before.talent_known=before.guild_known=before.skill_known=true;before.talent=1;before.guild=recipe.guilds[0];before.skill=skill;
      before.stocks={shcat::Stock{recipe.input,5,true,true},shcat::Stock{recipe.output,0,true,true},shcat::Stock{16613,0,true,true}};
      GrindAmountHost f;f.bytes=original;f.numerical=shcat::plan(recipe,before,n);f.raises=n==2&&skill==99;
      check(f.numerical.valid,"actual dynamic grind plan is valid for private x86 amount run");
      const ubc::AmountMemory mem{{&f,GrindAmountHost::copy},&f,GrindAmountHost::identity,GrindAmountHost::writable,GrindAmountHost::replace};
      ubc::AmountPatch patch;ubc::AmountOutcome outcome;bool restored=false,caught=false;void* result=nullptr;SetLastError(incoming_error);
      __try {result=ubc::run_amount_call(patch,mem,address(f.bytes.data()),f.numerical.body(),n,grind_amount_original,&f,restored,outcome);}
      __except(GetExceptionCode()==synthetic_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){caught=true;}
      check(f.calls==1&&outcome.entered&&caught==f.raises&&(caught||result==reinterpret_cast<void*>(73)),
        "one actual x86 amount effect returns or forwards its original SEH exactly once");
      check(restored&&outcome.restored&&!patch.active()&&f.bytes==original&&GetLastError()==original_error,
        "all original grind bytes and LastError restored after normal or exceptional callback");
      check(f.observed_skill==f.numerical.skill_after,"actual capped operand yields original repeated-progress result");
    }
  }
  check(std::fgetc(file)==EOF,"private fixture has no trailing unbound body");std::fclose(file);
}
bool dispatch_scope_job(Fixture& f,bool grind,bool resolver,bool predebit=false){
  // Job and scope values only. No injected positive reader or native
  // crafting call. Physical state comes from private RAM objects.
  if(!f.reset(0x3001,1)||!f.native_phase())return false;
  put(f.symbols[1].data(),0x18,grind?11u:13u);
  f.string(f.station.data(),0x1e0,grind?"SMITHSHARP":"SMITHCOOL");
  f.string(f.station.data(),0x1b8,grind?"BSSHARP":"BSCOOL");
  f.string(f.station.data(),0x1a4,"");f.string(f.station.data(),0x1cc,grind?"":"B_USECOOL");put(f.station.data(),0x200,0);
  const auto c=context();if(!c.valid)return false;
  ubc::CraftingRequest request;request.id=grind?0x9001u:0xa001u;request.quantity=predebit?2u:1u;request.key=key(c);
  const auto& d=*cat::find(request.id);auto& value=request.material_snapshot;value=ubc::make_crafting_state(request.id);
  value.generation=generation;value.evaluated=value.knowledge_read_valid=value.known=value.contract=true;
  value.actor_known=value.actor_matches=value.context_known=value.context_matches=value.hand_known=value.material_known=value.material_verified=true;
  value.maximum=predebit?2u:7u;if(grind)value.grind={true,1,d.grind->guilds[0],98};
  for(std::size_t i=0;i<value.target_count;++i){auto& row=value.materials[i];row.stored=i?0:static_cast<int>(value.maximum);
    row.generated_prop=cat::hand_amounts(d,row.symbol).generated;}
  request.hands.hands={{1,grind?16613:16612,1,address(&f.tokens[1])},{0,-1,-1,0}};
  request.hands.left_slot_known=request.hands.right_slot_known=request.hands.interact_item_known=true;
  request.hands.left_tmp_level=1;request.hands.interact_item=request.hands.hands.left.object;
  const auto page=grind?ubc::crafting_page(shnav::PageEvidence{true,true,resolver?shnav::Page::info:shnav::Page::pal_mil,resolver?0u:20961u}):
    ubc::crafting_page(cnav::Page{true,true,!predebit});
  const auto plan=ubc::make_crafting_plan(page,request);if(!plan.valid())return false;
  ubc::CraftingHandoffObservation initial;initial.key=request.key;initial.view=c.dialog.view;initial.frame=frame;initial.collection=rows.generation;
  initial.context_known=initial.page_known=initial.complete=initial.ready=initial.outer_frame=initial.quiescent=initial.focused=true;
  initial.page=page;initial.mode=plan.steps[0].invocation;initial.choice_owner=plan.steps[0].owner_instance;
  initial.count=1;initial.materials=value;initial.hands=request.hands;
  put(manager_bytes,0x54,initial.mode==basecat::Invocation::info?1u:2u);
  handoff={};dispatched_ticket={};result_guard={};
  if(!handoff.begin(request,plan,initial))return false;
  const ubc::CraftingHandoffOffer offer{true,initial.mode,plan.steps[0].owner_instance,plan.steps[0].function_symbol,0,rows.generation};
  const auto ticket=handoff.ticket(initial,offer);if(!ticket||!handoff.dispatch(*ticket,initial,offer))return false;
  dispatched_ticket=*ticket;
  if(predebit){
    initial.outer_frame=initial.quiescent=initial.ready=false;
    return handoff.enter_callback(*ticket,initial,offer,{true,true,true,1,1,0,0,0}).decision==ubc::CraftingHandoffDecision::execute;
  }
  return result_guard.begin(request,{generation,frame,rows.generation,end_sequence,c.world,c.hero,c.station});
}
Context dispatch_scope_context{};ddispatch::Invocation dispatch_scope_invocation{};
DestroyReadPhase dispatch_scope_expected=DestroyReadPhase::closed;
bool dispatch_scope_message=false;bc::Decision dispatch_scope_decision=bc::Decision::writer;
bool __cdecl scope_signature(const bc::Call&) noexcept{return true;}
void cool_predebit_entry_cases(Fixture& f){
  static_assert(cpre::runtime_ready,"regulärer x86-Einstieg prüft den freigeschalteten Auslieferungsstand");
  for(const int symbol:{20775,20778,20772}){
    if(!dispatch_scope_job(f,false,false,true)){check(false,"Vorentzug hat ein ausführendes Einzel-INFO-Ticket");continue;}
    bc::Config config;config.parser=reinterpret_cast<void*>(s::game_parser);config.symbols[0]=20775;config.symbols[1]=20778;config.symbol_count=2;
    config.signature=scope_signature;config.select=select_batch_call;config.writer=execute_amount;config.reject_unexpected=reject_unexpected_cool_info;
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);
    check(bc::configure(config),"CallFunc-Brücke benutzt die nativen Eintrittsprüfungen");
    native_update=effect_authorized=true;update_depth=info_depth=1;choice_depth=0;
    check(UbcBatchPrepare(config.parser,20772,bc::choice_call_return)==0,"fremder Caller bleibt außerhalb der Kühl-INFO");
    if(symbol==20775){
      const auto route=UbcBatchPrepare(config.parser,symbol,bc::info_call_return);
      check(route==2,"passende Einzel-INFO erreicht den echten Writer");
      if(route==2){SetLastError(incoming_error);UbcBatchWriter(config.parser,symbol);}
      check(GetLastError()==incoming_error&&amount_diagnostic.rejected==1&&!amount_patch.active()&&!script_calls&&handoff.cancelled(),
        "auch ein gebundenes Zweierticket ohne frische INFO-Lease scheitert vor Mengenpatch und Original");
    }else{
      check(UbcBatchPrepare(config.parser,symbol,bc::info_call_return)==3&&handoff.cancelled()&&
        UbcBatchPrepare(config.parser,symbol,bc::info_call_return)==3&&!bc::diagnostics().readers&&!bc::diagnostics().writer&&!script_calls,
        "gewechselter registrierter oder fremder Callback fällt auch beim zweiten Aufruf nicht ins Original");
    }
  }
}
void cool_receipt_ownership_cases(Fixture& f){
  for(unsigned change=0;change<5;++change){
    check(f.reset(0x3001,1)&&f.native_phase(),"privater UI-Kontext für die Kühlquittung");
    static_cast<ubc::NativeCraftingUi&>(ui).close();
    const auto c=context();pending_success={};pending_success.key=key(c);pending_success.receipt=101;
    test_notice=ui_owned=ui_render_ok=cool_job=true;cool_receipt_acked=false;
    notice_renders=notice_receipt_reads=0;lose_input_on_submit=change==3;
    if(change==0)ui_owned=false;
    if(change==1)pending_success.key.world+=4;
    if(change==2)pending_success.key.hero+=4;
    if(change==4)ui_render_ok=false;
    observe_result_notice(c);
    check(notice_renders==(change>=3?1u:0u)&&!notice_receipt_reads&&!cool_receipt_acked,
      "Besitzverlust vor oder im Submit und fehlgeschlagene Ausgabe quittieren keinen alten Beleg");
    if(change<4){
      ui_owned=true;pending_success.key=key(c);observe_result_notice(c);
      check(ui.notice_dismissed()&&notice_renders==(change==3?1u:0u)&&!notice_receipt_reads,
        "bei gehaltener Taste bleibt die abgemeldete Quittung geschlossen");
    }else check(result_guard.state()==ubc::CraftingResultState::failed,"Renderfehler sperrt die Ergebniswache");
    test_notice=lose_input_on_submit=false;
  }
  static_cast<ubc::NativeCraftingUi&>(ui).close();
}
bc::Decision __cdecl scope_select(const bc::Call&) noexcept{return dispatch_scope_decision;}
void* __cdecl scope_writer(bc::WriterScope& scope){
  check(bc::diagnostics().writer&&!bc::diagnostics().readers,"scope test holds actual exclusive bridge writer lease");
  check(destroy_read_phase(dispatch_scope_context,dispatch_scope_message?&dispatch_scope_invocation:nullptr)==dispatch_scope_expected,
    "only the explicit callback read phase admits an actual bridge writer");
  scope.restore_verified=true;return nullptr;
}
void* __cdecl scope_reader(void*,int){
  check(bc::diagnostics().readers==1&&!bc::diagnostics().writer&&
    destroy_read_phase(dispatch_scope_context)==DestroyReadPhase::closed,"actual ordinary bridge reader closes native Dispatch snapshots");return nullptr;
}
void dispatch_scope_cases(Fixture& f){
  for(const bool grind:{false,true})for(unsigned phase=0;phase<4;++phase){
    check(dispatch_scope_job(f,grind,phase==1),"scope matrix has canonical separate Cool/Grind request and ticket");
    const auto reset=[&](){frame_depth=phase==0?1u:0u;cool_destroy_depth=phase==3?1u:0u;
      native_update=phase==1||phase==2;update_depth=native_update?1u:0u;resolved_read_depth=phase==1?1u:0u;
      info_depth=phase==2&&!grind?1u:0u;choice_depth=phase==2&&grind?1u:0u;station_depth=collection_depth=0;
      effect_executed=phase==3;cool_job=!grind;grind_job=grind;};
    reset();const auto c=context();const ddispatch::Invocation invocation{c.hero,address(f.choice.data()),cabi::destroy_return,1};
    const auto expected=phase==0?DestroyReadPhase::preview:phase==1?DestroyReadPhase::resolver:
      phase==2?DestroyReadPhase::callback:DestroyReadPhase::destroy;
    check(destroy_read_phase(c)==expected,"all four explicit read-only Native phases are reachable at their actual counters");
    check(destroy_read_phase(c,&invocation)==(phase==3?DestroyReadPhase::destroy:DestroyReadPhase::closed),
      "actual Message invocation only enters the bound Destroy phase");
    for(unsigned bad=0;bad<16;++bad){reset();auto altered=c;
      switch(bad){case 0:frame_depth=phase==0?0u:1u;break;case 1:cool_destroy_depth=phase==3?2u:1u;break;
        case 2:update_depth=2;break;case 3:info_depth=2;break;case 4:choice_depth=2;break;case 5:station_depth=1;break;
        case 6:collection_depth=1;break;case 7:resolved_read_depth=2;break;case 8:altered.identity_known=false;break;
        case 9:altered.station_known=false;break;case 10:altered.world=0;break;case 11:altered.hero=0;break;
        case 12:altered.station=0;break;case 13:altered.kind=cat::Station::forge;break;
        case 14:InterlockedExchange(&engine_thread,static_cast<LONG>(GetCurrentThreadId()+1));break;case 15:generation=0;break;}
      check(destroy_read_phase(altered)==DestroyReadPhase::closed,"crossed nested unknown or foreign Native read scope is closed");
      generation=1;InterlockedExchange(&engine_thread,static_cast<LONG>(GetCurrentThreadId()));
    }
    reset();
    if(phase){for(unsigned field=0;field<3;++field){auto changed=c;
      if(field==0)changed.world+=4;else if(field==1)changed.hero+=4;else changed.station+=4;
      check(destroy_read_phase(changed)==DestroyReadPhase::closed,"callback and Destroy require the same fresh physical request key");}}
    if(phase==3){auto closed=c;closed.valid=false;closed.kind={};
      check(destroy_read_phase(closed)==DestroyReadPhase::destroy,"closed dialog still permits message-free actual Destroy snapshots at bound physical identity");
      cool_job=grind;grind_job=!grind;check(destroy_read_phase(c)==DestroyReadPhase::closed,"wrong waiting job family is not a native Destroy scope");reset();
    }
    if(phase==1||phase==2){++dispatched_ticket.id;check(destroy_read_phase(c)==DestroyReadPhase::closed,"foreign callback ticket cannot admit read-only native evidence");--dispatched_ticket.id;}
    bc::Config config;config.parser=reinterpret_cast<void*>(s::game_parser);config.symbols[0]=20965;config.symbol_count=1;
    config.signature=scope_signature;config.select=scope_select;config.writer=scope_writer;
    check(bc::configure(config),"scope matrix configures actual isolated bridge");
    dispatch_scope_context=c;dispatch_scope_invocation=invocation;dispatch_scope_message=phase==3;
    dispatch_scope_expected=phase==2?DestroyReadPhase::callback:DestroyReadPhase::closed;
    dispatch_scope_decision=bc::Decision::writer;
    check(UbcBatchPrepare(config.parser,20965,bc::choice_call_return)==2,"scope matrix acquires a real writer lease");UbcBatchWriter(config.parser,20965);
    dispatch_scope_decision=bc::Decision::ordinary;UbcBatchCallOriginal=reinterpret_cast<void*>(scope_reader);
    check(UbcBatchPrepare(config.parser,20965,bc::choice_call_return)==1,"scope matrix acquires a real ordinary reader lease");UbcBatchReader(config.parser,20965);
    bc::disable_writes();check(destroy_read_phase(c)==DestroyReadPhase::closed,"quarantined bridge closes every Native read phase");check(bc::configure(config),"scope fixture releases bridge quarantine");
    GrindAmountHost amount;amount.bytes[54]=64;put(amount.bytes.data(),55,1);ubc::Sha256 hash;hash.update(amount.bytes.data(),amount.bytes.size());
    const ubc::AmountOperand operand{55,1,1,0,false};const ubc::AmountBody body{308,hash.finish(),&operand,1};
    const ubc::AmountMemory mem{{&amount,GrindAmountHost::copy},&amount,GrindAmountHost::identity,GrindAmountHost::writable,GrindAmountHost::replace};
    check(amount_patch.begin(mem,address(amount.bytes.data()),body,2)&&amount_patch.active(),"scope matrix opens a private synthetic amount window");
    check(destroy_read_phase(c)==DestroyReadPhase::closed,"active amount patch never supplies read-only native Dispatch admission");check(amount_patch.finish(),"scope fixture restores its private bytes");
    if(phase==3){result_guard.fail(ubc::CraftingResultIssue::destroy);check(destroy_read_phase(c)==DestroyReadPhase::closed,"failed job cannot be resurrected as Destroy evidence");}
  }
  check(dispatch_scope_job(f,true,false),"initial generation scope fixture");frame_depth=1;native_update=false;update_depth=choice_depth=info_depth=resolved_read_depth=cool_destroy_depth=0;
  generation=0;cool_install_attempted=false;initialize_cool(context());
  check(!cool_install_attempted,"missing first UI generation does not consume the only native installation attempt");
  generation=1;check(destroy_read_phase(context())==DestroyReadPhase::preview,"real first UI generation admits infrastructure read before capture and borrow");
}
void __fastcall throwing_choice(void*,void*){++choice_calls;SetLastError(original_error);RaiseException(synthetic_exception,0,0,nullptr);}
void choice_seh_cleanup_cases(Fixture& f){
  for(const auto id:{0x2001u,0x9001u}){
    check(f.reset(id,1,false),"ordinary Choice exception fixture is unowned");original_choice=throwing_choice;
    grind_choice_lease.owner=0x1234;const auto previous=grind_choice_lease;effect_authorized=true;bool caught=false;
    __try {observe_choice(f.manager(),f.choice.data());}
    __except(GetExceptionCode()==synthetic_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){caught=true;}
    check(caught&&choice_calls==1&&choice_depth==0&&effect_authorized&&grind_choice_lease.owner==previous.owner&&
      !post_remove_choice.active()&&GetLastError()==original_error,"Choice SEH forwards once and restores depth authorization lease and LastError");
  }
  original_choice=counted_choice;
}
bool destroy_forward_raises=false;
int __thiscall counted_destroy(void*,void*) {
  ++destroy_forward_calls;seen_error=GetLastError();SetLastError(original_error);
  if(destroy_forward_raises)RaiseException(synthetic_exception,0,0,nullptr);
  return 73;
}
void destroy_forwarding_cases(Fixture& f){
  for(const auto id:{0xa001u,0x9001u})for(bool raised:{false,true}){
    check(f.reset(id,1,false),"cool and grind original metadata gives no synthetic writer approval");
    original_destroy=counted_destroy;destroy_forward_calls=0;destroy_forward_raises=raised;
    caller=cabi::destroy_return;SetLastError(incoming_error);int returned=0;bool caught=false;
    __try {returned=observe_destroy(f.hero.data(),nullptr,nullptr);}
    __except(GetExceptionCode()==synthetic_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){caught=true;}
    check(destroy_forward_calls==1&&seen_error==incoming_error&&GetLastError()==original_error,
      "ordinary Destroy keeps exactly one original call and both LastError boundaries");
    check(caught==raised&&(raised||returned==73)&&cool_destroy_depth==0,
      "Destroy forwards actual x86 result and SEH while unwinding only its own depth");
    check(result_guard.state()!=ubc::CraftingResultState::verified&&!cool_destroy_call.destroy_entries&&!grind_destroy_call.destroy_entries,
      "unowned Destroy cannot fabricate cooling or grind result evidence");
  }
  destroy_forward_raises=false;
}
void ammo_diagnostic_cases(Fixture& f){
  for(unsigned mode=0;mode<4;++mode){
    check(f.reset(static_cast<cat::RecipeId>(scat::RecipeId::arrows),2)&&f.native_phase(),
      "diagnostic case has a real selected root writer boundary");
    selected=&f;removal_mode=0;render_proof(f);
    UbcBatchCallOriginal=reinterpret_cast<void*>(synthetic_script);bc::Config config;
    config.parser=reinterpret_cast<void*>(s::game_parser);config.signature=batch_signature;config.select=select_batch_call;config.writer=execute_amount;
    check(configure_batch_symbols(config)&&bc::configure(config),"diagnostic case uses actual amount restore finally");
    diagnostic_probe=true;log_file=reinterpret_cast<HANDLE>(1);
    script_changes_hero=mode==0;ammo_close_accept=mode==0||mode==2;early_diagnostic_flush=mode==1;ammo_lose_health=mode>=2;
    diagnostic_status.configured=diagnostic_status.install_attempted=diagnostic_status.installed=diagnostic_status.owner_thread=true;
    diagnostic_status.fault=ammo::Fault::tracker;diagnostic_status.runtime_issue=ubc::ammo_runtime::Issue::pc;
    diagnostic_status.tracker.stage=ap::Stage::armed;diagnostic_status.tracker.issue=ap::Issue::missing_capture;
    diagnostic_status.tracker.blocked=true;
    diagnostic_status.capture={handoff.request().key.generation,handoff.request().key.generation,20091,
      7,6,2,true,false,ubc::ammo_runtime::Issue::pc,692432,692433,0x792567,0x792568};
    original_choice=original_choice_writer;SetLastError(incoming_error);observe_choice(f.manager(),f.choice.data());
    check(GetLastError()==original_error,"deferred status and log writes preserve original callback LastError");
    check(script_calls==1&&!amount_patch.active()&&!active_ammo_scope.active,
      "failure diagnosis neither repeats effect nor changes patch and scope lifetime");
    check(ammo_closes==(mode==0?0u:1u),"context failure keeps original short circuit before close_effect");
    check(diagnostic_event("ammo_diag_phase",mode==0?1:mode==1?2:mode==2?4:6,20091),
      "close context close effect and postcall health remain distinct diagnostic phases");
    check(diagnostic_event("ammo_diag_job",1,0)&&diagnostic_event("ammo_diag_fault",12,12)&&
      diagnostic_event("ammo_diag_tracker",1,12)&&diagnostic_event("ammo_diag_flags",87,29),
      "deferred log retains job fault reader issue tracker state and explicit flags");
    check(diagnostic_event("ammo_diag_capture",7,6)&&diagnostic_event("ammo_diag_reader",2,12)&&
      diagnostic_event("ammo_diag_pc",692432,692433)&&diagnostic_event("ammo_diag_caller",0x792567,0x792568),
      "deferred log retains capture counters and observed versus expected PC and caller");
    const auto* restored=std::strstr(diagnostic_log,"\"event\":\"amount_restored\"");
    const auto* diagnosed=std::strstr(diagnostic_log,"\"event\":\"ammo_diag_phase\"");
    check(restored&&diagnosed&&restored<diagnosed,"diagnostic records follow the original positive restore record");
    check(result_guard.state()==(mode==2?ubc::CraftingResultState::waiting:ubc::CraftingResultState::failed),
      "diagnostic-only postcall observation leaves original failure timing unchanged");
    const auto writes=diagnostic_writes,reads=ammo_status_reads;
    SetLastError(incoming_error);flush_ammo_diagnostic(true);
    check(diagnostic_writes==writes&&ammo_status_reads==reads&&GetLastError()==incoming_error,
      "already reported phase does not repeat status or IO");
    if(mode==2){
      frame_depth=1;update_depth=info_depth=choice_depth=station_depth=collection_depth=0;
      SetLastError(incoming_error);observe_ammo_frame_health();
      check(diagnostic_event("ammo_diag_phase",8,20091)&&result_guard.state()==ubc::CraftingResultState::failed&&
        GetLastError()==incoming_error,"quiet frame records the later health failure with unchanged contract failure");
      const auto after_writes=diagnostic_writes;observe_ammo_frame_health();
      check(diagnostic_writes==after_writes,"repeated failing frames emit only one record set per phase");
    }
  }
  check(f.reset(static_cast<cat::RecipeId>(scat::RecipeId::arrows),2),"timeout diagnostic fixture is fresh");
  diagnostic_probe=true;log_file=reinterpret_cast<HANDLE>(1);ammo_job=success_pending=true;ammo_notice_ready=false;
  ammo_diagnostic={UINT64_C(0x12345678ffffffff),20091,0,0};
  diagnostic_status.capture.job=diagnostic_status.capture.scope_token=ammo_diagnostic.job;
  diagnostic_status.capture.callback=20091;diagnostic_status.capture.attempts=UINT32_MAX;
  frame_depth=1;result_started=GetTickCount64()-30001;
  SetLastError(incoming_error);observe_ammo_frame_health();
  check(diagnostic_event("ammo_diag_phase",16,20091)&&diagnostic_event("ammo_diag_job",-1,0x12345678)&&
    diagnostic_event("ammo_diag_capture",-1,0)&&result_guard.state()==ubc::CraftingResultState::failed&&
    GetLastError()==incoming_error,"healthy adapter timeout is distinct and unsigned diagnostic words stay lossless");
  ammo_diagnostic.reported=0;const auto writes=diagnostic_writes,reads=ammo_status_reads;
  update_depth=1;flush_ammo_diagnostic(false);
  check(diagnostic_writes==writes&&ammo_status_reads==reads,"frame flush rejects a nonquiescent frame without consuming its pending report");
  update_depth=0;flush_ammo_diagnostic(false);
  check(diagnostic_writes==writes+9&&ammo_status_reads==reads+1,"pending frame report survives until the real quiet boundary");
}
}
namespace ubc::recipe_catalog {
const Callback* crafting_guard_callback_for_quantity(const Descriptor& d,std::uint32_t n) noexcept{return crafting_guard_test::synthetic(callback_for_quantity(d,n));}
}
namespace ubc::station_recipe_catalog {
const Callback* crafting_guard_callback_for_quantity(const Descriptor& d,std::uint32_t n) noexcept{return crafting_guard_test::synthetic(callback_for_quantity(d,n));}
}
namespace ubc::alchemy_recipe_catalog {
QuantityPlan crafting_guard_plan_quantity(const Descriptor& d,std::uint32_t n) noexcept {
  auto plan=plan_quantity(d,n);plan.callback=crafting_guard_test::synthetic(plan.callback);return plan;
}
}
namespace ubc::anvil_recipe_catalog {
QuantityPlan crafting_guard_plan_quantity(const Descriptor& d,std::uint32_t n) noexcept {
  auto plan=plan_quantity(d,n);plan.callback=crafting_guard_test::synthetic(plan.callback);return plan;
}
}
namespace ubc::ore_recipe_catalog {
QuantityPlan crafting_guard_plan_quantity(const Descriptor& d,std::uint32_t n) noexcept {
  auto plan=plan_quantity(d,n);plan.callback=crafting_guard_test::synthetic(plan.callback);return plan;
}
}
namespace ubc::crafting_guard_ammo {
bool configure(const Readers&) noexcept{return true;}
bool install(std::uint32_t thread) noexcept{
  using namespace crafting_guard_test;++ammo_installs;ammo_install_thread=thread;
  if(ammo_install_reenter){ammo_install_reenter=false;maybe_install_ammo(context());}
  return ammo_install_accept;
}
Status status() noexcept{++crafting_guard_test::ammo_status_reads;
  if(crafting_guard_test::diagnostic_probe){
    crafting_guard_test::check(!amount_patch.active(),"deferred status copy never runs with an active amount patch");
    SetLastError(0x77885544);return crafting_guard_test::diagnostic_status;
  }
  Status out;
  out.fault=Fault::original_contract;out.runtime_issue=ammo_runtime::Issue::unreadable;
  out.compatibility.issue=ammo_compat_memory::Issue::first_binding;return out;}
bool healthy() noexcept{return crafting_guard_test::ammo_available;}
bool permits_next() noexcept{return healthy()&&!crafting_guard_test::ammo_armed;}
bool admit(const ammo_presentation::Job& value,const ammo_presentation::Admission& proof) noexcept {
  using namespace crafting_guard_test;++ammo_admits;admitted_job=value;admitted_proof=proof;
  const bool allowed=ammo_accept&&ammo_available&&proof.ui_available&&proof.original_body_and_amount_contract&&proof.self_equals_hero;
  if(allowed){ammo_armed=true;if(damage_after_admit)selected->body[0]=65;}return allowed;
}
bool enter_effect(const ammo_presentation::Context& c,const ammo_presentation::Scope& scope) noexcept {
  using namespace crafting_guard_test;++ammo_enters;ammo_presentation::Scope actual;
  check(read_ammo_scope(nullptr,actual)&&actual.active&&actual.patched_body_verified&&actual.depth==1&&
    actual.job==admitted_job.id&&actual.token==admitted_job.scope_token&&actual.callback==admitted_job.callback,
    "root scope reader binds actual active amount transaction and exact own callback");
  SetLastError(0x11224488);return ammo_presentation::same(c,admitted_job.context)&&scope.active&&amount_patch.active();
}
bool close_effect(const ammo_presentation::Context& c,const ammo_presentation::Scope& scope) noexcept {
  using namespace crafting_guard_test;++ammo_closes;ammo_presentation::Scope actual;
  check(read_ammo_scope(nullptr,actual)&&actual.active&&actual.patched_body_verified&&actual.job==admitted_job.id,
    "root closes receipt before clearing actual patch scope");
  if(early_diagnostic_flush){
    note_ammo_diagnostic(AmmoDiagnosticPhase::close_effect);
    const auto writes=diagnostic_writes,reads=ammo_status_reads;
    SetLastError(0x44223311);flush_ammo_diagnostic(true);
    check(diagnostic_writes==writes&&ammo_status_reads==reads&&GetLastError()==0x44223311,
      "attempted flush inside effect neither logs nor samples nor loses LastError");
  }
  if(ammo_lose_health)ammo_available=false;
  SetLastError(0x88774411);return ammo_close_accept&&ammo_presentation::same(c,admitted_job.context)&&scope.active&&amount_patch.active();
}
bool observe_result(const ammo_presentation::Result&) noexcept{return true;}
ammo_presentation::Notice take_notice() noexcept{return {};}
bool ack_notice(std::uint64_t,std::uint64_t,bool) noexcept{return true;}
}
int main(int argc,char** argv){
  using namespace crafting_guard_test;
  if (argc == 2 && (!std::strcmp(argv[1], "--marker-present") || !std::strcmp(argv[1], "--marker-absent"))) {
    ubc::windows_probe::activation_fixture = true;
    const auto instance = GetModuleHandleW(nullptr);
    DllMain(instance, DLL_PROCESS_ATTACH, nullptr);
    const bool ok = execution_enabled == (!std::strcmp(argv[1], "--marker-present")) && !installed;
    DllMain(instance, DLL_PROCESS_DETACH, nullptr);
    return ok ? 0 : 1;
  }
  registry_tests();PrivatePages pages;if(!pages.open()){check(false,"private test pages allocated");return 1;}
  static_assert(sizeof(Fixture)<=PrivatePages::size-0x1000,"fixture stays in private bounded memory");
  auto& f=*new(pages.space())Fixture;
  original_info=counted_info;original_choice=counted_choice;original_resolve=counted_resolve;original_choice_event=counted_input;
  install_gate_cases(f);original_three_cases(f);tobacco_guard_cases(f);wrapper_cases(f);anvil_ore_boundary_cases(f);cool_station_context_cases(f);
  grind_station_context_cases(f);grind_owner_metadata_cases();cool_predebit_entry_cases(f);cool_receipt_ownership_cases(f);
  dispatch_scope_cases(f);choice_seh_cleanup_cases(f);destroy_forwarding_cases(f);
  saw_actor_cases(f);anvil_actor_cases(f);writer_cases(f);rejection_cases(f);native_fault_cases(f);extra_admission_cases(f);spirit_admission_cases(f);info_lease_lifetime_cases(f);
  ammo_diagnostic_cases(f);f.~Fixture();
  if(argc==3&&std::strcmp(argv[1],"--grind-bodies")==0)grind_amount_host_cases(argv[2]);
  else if(argc!=1){check(false,"unsupported host arguments");}
  else std::printf("Grind original-body Windows restore cases: NOT_RUN (optional private fixture)\n");
  std::printf("CraftingEffectGuard: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
