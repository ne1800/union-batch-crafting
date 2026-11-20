// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/stations/cool/cool_predebit_context.hpp"
#include "ubc/stations/cool/cool_predebit_profile.hpp"
#include "ubc/stations/cool/cool_direct_contract.hpp"
#include "ubc/stations/cool/cool_recipe_snapshot.hpp"
#include "ubc/stations/cool/cool_nofx_reader.hpp"
#include "ubc/stations/cool/cool_message_malloc_profile.hpp"
#include "ubc/stations/bow/bow_item_native.hpp"
#include "ubc/stations/cool/generated_blade_hand_shape.hpp"
#include <optional>
#include <cstdlib>

// ONE composite reader for the short pre-debit, split and restore. No
// execution, hooks or queue changes. The native owner supplies actual writer,
// INFO and module leases and monotonic fence counters. Double reads cannot
// replace them. The handoff states the limits of the CRT/allocator assumption.
// This header establishes neither an OS audit nor a successful allocation.
namespace ubc::cool_predebit::reader {
namespace dep=alchemy_extra_dependencies;namespace rs=runtime_script;
namespace rt=alchemy_flask_runtime;namespace cr=cool_recipe_runtime;
namespace bn=bow_item_native;namespace bc=bow_item_code;
enum class Phase {before_extra,restore,after_extra,before_original,admission,after_original,before_remove};
inline constexpr bool split_pending(Phase phase) noexcept {
  return phase==Phase::admission || phase==Phase::before_extra || phase==Phase::before_remove;
}
inline bool reads_saved_call(Phase phase)noexcept{return phase==Phase::before_remove||phase==Phase::restore;}
enum class Issue {none,request,scope,thread,reentry,serial,amount,engine,module,code,
  script,constructor,context,parser,info,queue,inventory,hand,alias,pool,world,
  crt,changed,restore};
struct Input {
  Job job{};Scope scope{};Phase phase=Phase::before_extra;
  std::uint64_t serial=0,frame=0;std::uint32_t current_thread=0;
  // Own journal of the not-yet-entered destroy hook for this lease.
  std::uint32_t destroy_started=UINT32_MAX,destroy_completed=UINT32_MAX;
  zbug_end_compat::Module engine{};dep::Modules modules{};
  cool_nofx::StackRange stack{}; // Actual current x86 thread stack limits.
  std::optional<std::uint32_t> encoded_null_handler{}; // EncodePointer(nullptr) in the same process.
  // Only the own already-validated AmountPatch instance; the Reader
  // normalizes exactly Operand62 from 2 to 1 for the original hash.
  const AmountPatch* amount_patch=nullptr;
  bool engine_sites_verified=false;
  const Call* call=nullptr; // Restore only; never use after original.
};
struct Capture {
  static constexpr bool native_effect_authorized=false;
  bool known=false;Issue issue=Issue::none;std::uint32_t failed_address=0;
  check::Sample failure{};
  Observation observation{};RestoreObservation restore{};StationRecipeHands hands{};
  lhiver::Digest read_fingerprint{};
};
static_assert(sizeof(Capture)<4096,"Nur Wertkopien auf dem Callbackstack");
namespace detail {
inline bool word(const ReadOnlyMemory& m,std::uint32_t a,std::uint32_t o,std::uint32_t& v)noexcept{return dep::detail::word(m,a,o,v);}
inline void hw(Sha256& h,std::uint32_t v)noexcept{std::array<std::uint8_t,4>b{};dep::detail::put(b.data(),v);h.update(b.data(),b.size());}
inline bool overlaps(std::uint32_t a,std::size_t n,std::uint32_t b,std::size_t s)noexcept{return zbug_end_compat::detail::overlaps(a,n,b,s);}
// Every read address and every byte, before the own hash normalization.
struct Trace {
  ReadOnlyMemory source{};Sha256 hash{};std::uint32_t operand=0;bool ok=true;
  static bool copy(void* p,std::uint32_t a,void* out,std::size_t n)noexcept{
    auto& t=*static_cast<Trace*>(p);
    if(n>UINT32_MAX||!rs::detail::range(a,n)||!t.source.copy(a,out,n)){t.ok=false;return false;}
    hw(t.hash,a);hw(t.hash,static_cast<std::uint32_t>(n));
    if(a==lhiver::game_parser&&n==0x58){
      // read_layout uses only these six fields. Cursor (+4c), Stack and
      // Instance are read separately in their respective Writer/Restore states.
      const auto* bytes=static_cast<const std::uint8_t*>(out);
      t.hash.update(bytes+0x18,12);t.hash.update(bytes+0x48,4);t.hash.update(bytes+0x50,8);
    }else t.hash.update(out,n);
    if(t.operand&&overlaps(a,n,t.operand,4)){
      auto* bytes=static_cast<std::uint8_t*>(out);
      for(unsigned k=0;k<4;++k){const auto at=std::uint64_t{t.operand}+k;
        if(at<a||at>=std::uint64_t{a}+n)continue;
        const auto i=static_cast<std::size_t>(at-a);if(bytes[i]!=(k?0:2)){t.ok=false;return false;}
        bytes[i]=k?0:1;
      }
    }return true;
  }
  ReadOnlyMemory memory()noexcept{return {this,&copy};}
};
// Concrete stock write targets from exactly two bound PoolAlloc calls.
// Memory freshly delivered from the CRT retains its explicit boundary.
struct PoolWrite {std::uint32_t address=0,length=0;bool control=false;};
struct PoolWrites {
  bool known=false;bn::PoolEvidence pool{};
  std::array<PoolWrite,4> ranges{};std::size_t count=0;
  bool add(std::uint32_t a,std::uint32_t n,bool control=false)noexcept{
    if(!a||a%4||!rs::detail::range(a,n)||count==ranges.size())return false;
    for(std::size_t i=0;i<count;++i)if(overlaps(a,n,ranges[i].address,ranges[i].length))return false;
    ranges[count++]={a,n,control};return true;
  }
};
inline PoolWrites pool_writes(const ReadOnlyMemory& m,check::Checks& checks)noexcept{
  PoolWrites out;auto& pool=out.pool;pool.required=2;std::uint32_t size=0,block=0,debug=0,site=0,head=0;
  const auto read=[&](const char* field,std::uint32_t address,std::uint32_t expected,std::uint32_t& actual){
    const bool known=word(m,address,0,actual);checks.equal(field,expected,actual,known);
  };
  read("pool.node_size",bn::node_pool,12,size);read("pool.block_size",bn::node_pool+4,4096,block);
  read("pool.debug",0x8a2558,0,debug);read("pool.site",0x8d8780,0,site);
  const bool free_read=word(m,bn::node_pool,16,pool.free_count),head_read=word(m,bn::node_pool,12,head);
  checks.test("pool.free_count","0..INT_MAX",pool.free_count,pool.free_count<=INT32_MAX,free_read);
  checks.test("pool.head","null genau bei 0 freien Knoten",head,(!head)==(!pool.free_count),head_read);
  pool.growth_required=pool.free_count<2;pool.reuse_path=!pool.growth_required;
  checks.flag("pool.controls",out.add(bn::node_pool+(pool.growth_required?8:12),pool.growth_required?12:8,true));
  const auto count=std::min(pool.free_count,2u);auto previous=bn::node_pool+12;
  if(!count){checks.record("pool.node","wenn vorhanden","keiner",true);checks.record("pool.previous","wenn vorhanden","keiner",true);}
  for(unsigned i=0;i<count;++i){
    pool.prefix[i]=head;checks.test("pool.node","getrennte 12 Bytes",head,out.add(head,12));
    std::uint32_t back=0,next=0;const bool link=head&&word(m,head,0,next)&&word(m,head,4,back);
    checks.equal("pool.previous",previous,back,link);previous=head;head=next;
  }
  checks.test("pool.next","null genau am Listenende",head,(!head)==(pool.free_count==count));
  if(head){
    bool apart=head%4==0&&rs::detail::range(head,12);
    for(std::size_t i=0;i<out.count;++i)apart&=!overlaps(head,12,out.ranges[i].address,out.ranges[i].length);
    checks.test("pool.successor","getrennte 12 Bytes",head,apart);
    std::array<std::uint8_t,12> tail{};const bool known=m.copy(head,tail.data(),tail.size());
    checks.equal("pool.backlink",previous,rs::detail::u32(tail.data()+4),known);
    checks.flag("pool.successor_write",apart&&out.add(head+4,4));
  }else for(const auto field:{"pool.successor","pool.backlink","pool.successor_write"})
    checks.record(field,"wenn vorhanden","kein Nachfolger",true);
  pool.stable=out.known=checks.ok();return out;
}
// Every downstream code/symbol/slot/inventory/world reader receives only
// this shell. Even individual symbol records and successor.prev are thus
// protected, without an incomplete list of suspected parser targets.
// Pool validation itself happens beforehand on the same complete trace.
struct ProtectedReads {
  ReadOnlyMemory source{};const PoolWrites& writes;std::uint32_t conflict=0;
  static bool copy(void* p,std::uint32_t a,void* bytes,std::size_t n)noexcept{
    auto& self=*static_cast<ProtectedReads*>(p);
    for(std::size_t i=0;i<self.writes.count;++i){const auto& w=self.writes.ranges[i];
      if(overlaps(a,n,w.address,w.length)){self.conflict=w.address;return false;}}
    return self.source.copy(a,bytes,n);
  }
  ReadOnlyMemory memory()noexcept{return {this,&copy};}
};
inline void request(const Input& q,std::uint32_t thread,check::Checks& k)noexcept{
  const auto& c=q.job.context;const auto& s=q.scope;const auto& w=s.writer;
  const bool saved=reads_saved_call(q.phase);
  k.test("request.phase","Vorentzugphase",static_cast<int>(q.phase),static_cast<unsigned>(q.phase)<=static_cast<unsigned>(Phase::before_remove));
  k.flag("request.context",value::valid(c));
  k.test("request.job",">0",q.job.id,q.job.id!=0);
  k.test("request.writer_token",">0",q.job.writer_token,q.job.writer_token!=0);
  k.test("request.fence_token",">0",q.job.fence_token,q.job.fence_token!=0);
  k.test("request.info_token",">0",q.job.info_token,q.job.info_token!=0);
  k.test("request.actual_info","ausgerichtete INFO",q.job.actual_info,q.job.actual_info&&!(q.job.actual_info%4));
  k.test("request.frame",">0",q.frame,q.frame!=0);
  k.flag("request.stack_verified",q.stack.verified);
  k.equal("request.stack_thread",thread,q.stack.thread);
  k.test("request.stack_low","ausgerichtet, >0",q.stack.low,q.stack.low&&!(q.stack.low%4));
  k.test("request.stack_high","ausgerichtet, >StackLow",q.stack.high,q.stack.high>q.stack.low&&!(q.stack.high%4));
  k.equal("request.manager",station_abi::information_manager,q.job.information_manager);
  k.equal("request.parser",lhiver::game_parser,c.parser);
  k.flag("scope.active",w.writer_active);k.equal("scope.writer_depth",1,w.writer_depth);k.equal("scope.vm_depth",0,w.vm_depth);
  k.equal("scope.debit_depth",saved||q.phase==Phase::after_extra?1:0,w.debit_depth);
  k.equal("scope.job",q.job.id,w.job);k.equal("scope.writer_token",q.job.writer_token,w.writer_token);
  k.equal("scope.callback",callback,w.callback);k.flag("scope.info_active",s.info_lease_active);
  k.equal("scope.info_token",q.job.info_token,s.info_token);k.equal("scope.actual_info",q.job.actual_info,s.actual_info);
  k.equal("scope.destroy_started",0,q.destroy_started);k.equal("scope.destroy_completed",0,q.destroy_completed);
  k.equal("request.saved_call",saved,q.call!=nullptr);k.flag("request.amount_patch",q.amount_patch!=nullptr);
  if(saved&&q.call){k.flag("request.call_valid",valid_call(*q.call));k.flag("request.same_job",same_job(q.job,q.call->job));
    k.equal("request.call_frame",q.frame,q.call->frame);k.test("request.call_serial","vor aktueller Serie",q.call->serial,q.serial>q.call->serial);}
}
inline old::ItemBinding item(const dep::ItemBinding& b)noexcept{
  return {b.known,b.known,b.parser,b.symbol,b.table,b.code,b.code_size,b.symbol_count,
    1682,7,1,0,b.value,1584};
}
// The parameter INFO is bound by the ticket. Manager.currentInfo is
// not a substitute for the actually passed dataset during OnInfo.
inline void info(const ReadOnlyMemory& m,const Input& q,check::Checks& checks) noexcept {
  const auto& single=cool_recipe_catalog::single;
  const auto at=q.job.actual_info;
  const auto compare=[&](const char* field,std::uint32_t base,unsigned offset,std::uint32_t want){
    std::uint32_t value=0;const bool known=word(m,base,offset,value);
    checks.equal(field,want,value,known);
  };
  compare("info.vtable",at,0,info_detail::info_vtable);
  compare("info.instance",at,0x50,single.instance);
  compare("info.condition",at,0x28,single.condition->symbol);
  compare("info.callback",at,0x2c,single.information->symbol);
  compare("info.choice",at,0x58,0);
  compare("info.owner_npc",at,0x1c,single.npc);
  compare("info.number",at,0x20,single.number);
  compare("info.important",at,0x24,single.important);
  compare("info.trade",at,0x44,single.trade);
  compare("info.permanent",at,0x48,single.permanent);
  compare("info.npc",q.job.information_manager,0x20,q.job.context.hero);
  compare("info.player",q.job.information_manager,0x24,q.job.context.hero);
  compare("info.mode",q.job.information_manager,0x54,1);
}
inline void queue(const ReadOnlyMemory& m,const Input& q,SplitFence& out,check::Checks& checks) noexcept {
  std::uint32_t manager=0;
  const bool resolved=word(m,q.job.context.hero,0xfc,manager);
  const bool valid=manager && !(manager%4) && manager!=q.job.information_manager;
  checks.test("queue.manager","gueltiger Eventmanager",manager,valid,resolved);
  std::array<std::uint8_t,0x40> b{};
  const bool known=resolved&&valid&&m.copy(manager,b.data(),b.size());
  const auto u=[&](unsigned o){return rs::detail::u32(b.data()+o);};
  checks.equal("queue.vtable",0x83e23c,u(0),known);
  checks.equal("queue.owner",q.job.context.hero,u(0x3c),known);
  const auto data=u(0x30),capacity=u(0x34),used=u(0x38);
  const bool span=used<=capacity && used<=4096 && (!used||(data && !(data%4) &&
                                     rs::detail::range(data,std::size_t{used}*4)));
  checks.test("queue.data","lesbare aktive Eintraege",data,!used||(data&&!(data%4)),known);
  checks.test("queue.capacity",">= Anzahl",capacity,capacity>=used,known);
  checks.test("queue.count","0..4096",used,used<=4096,known);
  for(const auto& field:std::array<std::pair<const char*,unsigned>,3>{{
      {"queue.cleared",0x24},{"queue.active",0x28},{"queue.cutscene",0x2c}}})
    checks.test(field.first,"beobachtet",u(field.second),true,known);
  Sha256 h;hw(h,manager);h.update(b.data()+0x24,0x1c);
  bool readable=known&&span;std::array<std::uint32_t,64> entries{};
  for(std::uint32_t i=0;readable&&i<used;){
    const auto n=std::min<std::uint32_t>(used-i,64);
    readable=m.copy(data+4*i,entries.data(),4*n);
    if(readable)h.update(entries.data(),4*n);
    i+=n;
  }
  checks.test("queue.entries","lesbar",used,readable,readable);
  out={false,false,q.job.fence_token,q.serial,manager,q.destroy_started,q.destroy_completed,h.finish()};
}
inline void crt(const ReadOnlyMemory& m,const Input& q,check::Checks& checks)noexcept{
  // CRT prefers 78AA0000, not the Union/ZBug base 10000000. Therefore
  // do not use their specialized relocation function for CRT code.
  const auto bound=[&](const zbug_end_compat::CodeRange& code)noexcept{
    std::array<std::uint8_t,512> bytes{},again{};
    if(code.length<4||code.length>bytes.size()||!zbug_end_compat::detail::in_module(q.modules.runtime_crt,code.rva,code.length)||
      (code.relocation_count&&!code.relocations)||code.relocation_count>code.length/4||
      !m.copy(q.modules.runtime_crt.base+code.rva,bytes.data(),code.length)||
      !m.copy(q.modules.runtime_crt.base+code.rva,again.data(),code.length)||std::memcmp(bytes.data(),again.data(),code.length))return false;
    for(std::size_t i=0;i<code.relocation_count;++i){const auto at=code.relocations[i];
      if(at>code.length-4||(i&&at<code.relocations[i-1]+4))return false;
      dep::detail::put(bytes.data()+at,rs::detail::u32(bytes.data()+at)-q.modules.runtime_crt.base+0x78aa0000u);
    }
    return dep::detail::digest(bytes.data(),code.length)==code.sha256;
  };
  for(const auto& code:profile::free_code)checks.test("crt.free_code","Profil an RVA",code.rva,bound(code));
  for(const auto& code:cool_message_malloc::profile::malloc_code)checks.test("crt.malloc_code","Profil an RVA",code.rva,bound(code));
  const auto c=q.modules.runtime_crt.base;
  const auto read=[&](const char* field,unsigned offset){
    std::uint32_t value=0;const bool known=word(m,c,offset,value);
    checks.test(field,">0",value,value!=0,known);
  };
  read("crt.heap",0xb38b0);
  std::uint32_t mode=0,handler=0;
  const bool known=word(m,c,0xb45f4,mode);
  checks.test("crt.newmode","0..1",mode,mode<=1,known);
  if(known&&mode==1){
    const bool read_handler=word(m,c,0xb4460,handler);
    checks.equal("crt.new_handler",q.encoded_null_handler.value_or(0),handler,read_handler&&q.encoded_null_handler.has_value());
  }else checks.record("crt.new_handler","nur bei Newmode 1","inaktiv",known);
  read("crt.heap_alloc",0x11f8);
  read("crt.heap_free",0x11fc);
}
struct Literal {std::uint32_t object=0,data=0,length=0;};
using Literals=std::array<Literal,profile::texts.size()+1>; // Plus symbol name 16612 for GetSymbolInfo/SetVobName.
inline bool literal_disjoint(const Literals& strings,std::uint32_t address,std::size_t n)noexcept{
  for(const auto& s:strings)if(overlaps(s.object,20,address,n)||overlaps(s.data-1,s.length+2,address,n))return false;
  return true;
}
inline bool protected_range(const Input& q,const rs::detail::Layout& l,std::uint32_t a,std::size_t n)noexcept{
  for(const auto* module:{&q.engine,&q.modules.zbug,&q.modules.union_core,&q.modules.parser_extender,&q.modules.runtime_crt})
    if(overlaps(a,n,module->base,module->image_size))return true;
  const auto& c=q.job.context;
  return overlaps(a,n,q.stack.low,q.stack.high-q.stack.low)||overlaps(a,n,c.parser,0x219c)||
    overlaps(a,n,l.table,std::size_t{l.table_capacity}*4)||overlaps(a,n,l.code,l.code_capacity)||
    overlaps(a,n,c.hero,0xa04)||overlaps(a,n,c.world,0x628c)||overlaps(a,n,c.mob,0x240)||
    overlaps(a,n,c.game,0x30)||overlaps(a,n,q.job.actual_info,0x5c);
}
inline void constructor(const ReadOnlyMemory& m,const Input& q,const rs::detail::Layout& layout,
                        Literals& strings,check::Checks& checks)noexcept{
  const auto binding=[&](const lhiver::SymbolBinding& b){
    checks.test("constructor.binding","Metadaten laut Profil",b.symbol,rs::detail::check_binding(m,layout,b)==rs::Issue::none);
  };
  for(const auto& b:profile::bindings)binding(b);
  for(const auto i:profile::constructor_field_indices)binding(bn::constructor_fields[i]);
  bool aliases=false;
  for(std::size_t i=0;i<strings.size();++i){
    constexpr char hot_name[]="ITMISWORDBLADEHOT";
    const bool item_name=i==profile::texts.size();
    const auto id=item_name?16612:profile::texts[i].symbol;
    const auto length=item_name?static_cast<std::uint32_t>(sizeof(hot_name)-1):profile::texts[i].length;
    rs::detail::Symbol symbol;const auto lookup=rs::detail::lookup(m,layout,id,symbol);
    checks.test("constructor.symbol","Symbol lesbar",id,lookup==rs::Issue::none);
    const auto object=item_name?symbol.address:symbol.data;
    std::array<std::uint8_t,20> header{};std::array<std::uint8_t,65> buffer{};
    const bool known=lookup==rs::Issue::none && object && !(object%4) && m.copy(object,header.data(),header.size());
    auto& str=strings[i];str={object,rs::detail::u32(header.data()+8),length};
    checks.equal("constructor.vtable",0x82e6f0,rs::detail::u32(header.data()),known);
    checks.equal("constructor.length",length,rs::detail::u32(header.data()+12),known);
    const auto capacity=rs::detail::u32(header.data()+16);
    checks.test("constructor.capacity","Laenge..1048576",capacity,capacity>=length&&capacity<=0x100000,known);
    const bool bytes=known&&str.data>1&&length+2<=buffer.size()&&m.copy(str.data-1,buffer.data(),length+2);
    checks.test("constructor.data","lesbar",str.data,bytes);
    checks.test("constructor.cow",item_name?"teilen oder kopieren":"<254",buffer[0],item_name||buffer[0]<0xfe,bytes);
    const bool text=item_name?!std::memcmp(buffer.data()+1,hot_name,sizeof(hot_name)):
                        rt::detail::string_once(m,layout,profile::texts[i]).valid;
    checks.test("constructor.text","Originaltext des Symbols",id,text&&bytes);
    if(!bytes)continue;
    bool conflict=protected_range(q,layout,str.object,20)||protected_range(q,layout,str.data-1,length+2)||
      overlaps(str.object,20,str.data-1,length+2)||overlaps(str.data-1,length+2,symbol.address,0x3c);
    for(std::size_t j=0;j<i;++j){const auto& other=strings[j];
      conflict|=overlaps(str.object,20,other.object,20)||overlaps(str.object,20,other.data-1,other.length+2)||
        overlaps(str.data-1,length+2,other.object,20)||overlaps(str.data-1,length+2,other.data-1,other.length+2);}
    if(conflict){aliases=true;checks.test("alias.constructor","getrennte Textobjekte",id,false);}
  }
  if(!aliases)checks.equal("alias.constructor",0,0);
  const auto symbol_safe=[&](std::uint32_t id){
    rs::detail::Symbol s;const bool known=rs::detail::lookup(m,layout,id,s)==rs::Issue::none;
    bool separate=known;
    for(const auto& text:strings)if((text.object!=s.address&&overlaps(text.object,20,s.address,0x3c))||
      overlaps(text.data-1,text.length+2,s.address,0x3c))separate=false;
    checks.test("alias.symbol","getrenntes Symbol",id,separate);
  };
  for(const auto& f:cool_recipe_catalog::functions)symbol_safe(f.symbol);
  for(const auto& b:cool_recipe_catalog::bindings)symbol_safe(b.symbol);
  for(const auto& b:profile::bindings)symbol_safe(b.symbol);
  for(const auto& b:bn::constructor_fields)symbol_safe(b.symbol);
  std::uint32_t flags=0,cached=0;
  const bool have_flags=word(m,0xab1168,0x48,flags),have_class=word(m,0x8b7464,0,cached);
  checks.test("constructor.flags","Bit 0 frei",flags,!(flags&1u),have_flags);
  checks.equal("constructor.item_class",1584,cached,have_class);
  std::array<std::uint8_t,256> upper{};Sha256 h;
  if(m.copy(0x82fdb8,upper.data(),upper.size())){h.update(upper.data(),upper.size());checks.digest("constructor.uppercase",profile::uppercase_sha256,h.finish());}
  else checks.record("constructor.uppercase","Profilhash","unlesbar",false);
  std::array<char,5> needle{};const bool have_needle=m.copy(0x8922a4,needle.data(),needle.size());
  checks.test("constructor.extension",".ZEN mit NUL",rs::detail::u32(reinterpret_cast<const std::uint8_t*>(needle.data())),
              !std::memcmp(needle.data(),".ZEN",5),have_needle);
}
inline bool object_created_code(const ReadOnlyMemory& m)noexcept{
  constexpr auto& fn=profile::object_created;
  std::array<std::uint8_t,fn.length> bytes{};
  if(!dep::detail::twice(m,fn.address,bytes.data(),bytes.size()).valid)return false;
  const auto hash=dep::detail::digest(bytes.data(),bytes.size());
  return hash==fn.digest||hash==profile::union_object_created_sha256;
}
inline void engine(const ReadOnlyMemory& m,const Input& q,check::Checks& checks)noexcept{
  checks.flag("engine.mapping",q.engine.mapping_verified);
  checks.equal("engine.base",0x400000,q.engine.base);
  checks.equal("engine.size",0x752000,q.engine.image_size);
  if (q.engine_sites_verified) checks.flag("engine.sites", q.engine.mapping_verified);
  else checks.digest("engine.file",station_abi::executable_sha256,q.engine.file_sha256);
  const auto pe=dep::detail::pe(m,q.engine,0x3f966d87,0x400000,!q.engine_sites_verified);
  checks.equal("engine.pe",0,static_cast<int>(pe.issue));
  for(const auto& range:profile::engine){
    const zbug_end_compat::Binding plugins{q.modules.zbug,q.modules.union_core,true};
    bool valid=range.address==profile::object_created.address
      ?object_created_code(m):dep::detail::plain(m,range).valid;
    if(!valid&&range.address==0x7d5940){
      // An Entry-JMP leaves _strcmp entirely.
      std::array<std::uint8_t,5> entry{};
      valid=m.copy(range.address,entry.data(),entry.size())&&entry[0]==0xe9&&
        zbug_end_compat::detail::cooling_forward(plugins,range.address+5+rs::detail::u32(entry.data()+1));
    }else if(!valid&&range.address==0x7d0bbc){
      // In the RTTI comparison only the call at +45 is replaced.
      std::uint32_t displacement=0;bool patched=false;
      constexpr auto next=0x7d0beeu;
      valid=word(m,next-4,0,displacement)&&zbug_end_compat::detail::cooling_forward(plugins,next+displacement)&&
        dep::detail::patched_call(m,range,45,0x7d5940,next+displacement,patched).valid;
    }
    checks.test("engine.code","Original oder gebundener Einstieg",range.address,valid);
  }
  for(const auto& range:profile::data){const auto c=dep::detail::plain(m,range);checks.test("engine.data","Profildaten an Adresse",range.address,c.valid);}
  // Entire engine bodies needed only for Split/WorldInsert.
  // WorldRemove/Renderer lie outside this short window.
  for(const auto index:profile::split_engine_indices){
    const auto c=bn::detail::plain(m,bn::engine_ranges[index]);checks.test("engine.split","Profilcode an Adresse",bn::engine_ranges[index].address,c.verified);}
}
}

class Reader {
public:
  explicit Reader(std::uint32_t owner_thread)noexcept:owner_thread_(owner_thread){}
  Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
  Capture capture(const ReadOnlyMemory& raw,const Input& supplied,check::Report* report=nullptr)noexcept{
    Input q=supplied;Call saved;Capture out;
    check::Checks checks(report);
    const auto stop=[&](Issue issue){
      out.issue=issue;out.failure=checks.first;out.known=false;return out;
    };
    checks.flag("reader.exclusive",!reading_);
    checks.test("reader.owner_thread",">0",owner_thread_,owner_thread_!=0);
    if(reading_){reentered_=true;return stop(Issue::reentry);}
    checks.equal("reader.current_thread",owner_thread_,q.current_thread);
    checks.equal("reader.job_thread",owner_thread_,q.job.context.thread);
    if(!checks.ok()&&!report)return stop(Issue::thread);
    if(reads_saved_call(q.phase)&&q.call){saved=*q.call;q.call=&saved;}
    detail::request(q,owner_thread_,checks);
    checks.test("reader.serial","neuer als letzter Read",q.serial,q.serial>serial_);
    if(!checks.ok()&&!report)return stop(std::strcmp(checks.first.field,"reader.serial")?Issue::request:Issue::serial);
    serial_=q.serial;reading_=true;reentered_=false;
    struct Lease{bool& b;~Lease(){b=false;}}lease{reading_};
    const auto assembly=cr::assemble(cool_recipe_catalog::recipe);const auto p=assembly.profile();
    rs::detail::Layout layout{};
    const auto layout_issue=rs::detail::read_layout(raw,q.job.context.parser,p,layout);
    checks.equal("parser.layout",0,static_cast<int>(layout_issue));
    if(!checks.ok()&&!report)return stop(Issue::parser);
    const auto& f=*cool_recipe_catalog::single.information;
    const std::array<AmountOperand,1> ops{{{62,1,1,0,false}}};
    const AmountBody body{f.byte_length,f.body_sha256,ops.data(),ops.size()};
    const auto address=layout.code+f.byte_offset;
    const bool original=q.phase==Phase::admission||q.phase==Phase::after_original;
    const auto amount_matches=[&](){
      return q.amount_patch&&(original?!q.amount_patch->active():q.amount_patch->matches_active(address,body,2));
    };
    checks.flag("amount.journal",amount_matches());
    if(!checks.ok()&&!report)return stop(Issue::amount);
    lhiver::Digest first{};
    for(unsigned pass=0;pass<2;++pass){
      detail::Trace trace{raw,{},original?0u:address+62,true};
      auto current=sample(trace.memory(),q,assembly,report);
      if(!current.known&&!report)return current;
      const auto digest=trace.hash.finish();
      const auto current_address=std::uint64_t{current.observation.parser.code_base}+f.byte_offset;
      checks.flag("changed.readable",trace.ok);
      checks.flag("reader.reentry",!reentered_);
      if(pass)checks.digest("changed.double_read",first,digest);
      checks.equal("amount.body_address",address,current_address);
      checks.flag("amount.closing_journal",amount_matches());
      if(!checks.ok()&&!report){
        out=current;out.observation.stable_read=out.observation.native_split_restore=false;
        out.observation.fence.known=out.observation.fence.path_excludes_dispatch=false;
        out.restore.targets.known=out.restore.native_set_item=false;
        out.failed_address=current_address<=UINT32_MAX?static_cast<std::uint32_t>(current_address):0;
        return stop(reentered_?Issue::reentry:current_address!=address||!amount_matches()?Issue::amount:Issue::changed);
      }
      first=digest;out=current;
    }
    if(!checks.ok())return stop(Issue::changed);
    out.read_fingerprint=first;return out;
  }
private:
  void read_hand(const ReadOnlyMemory& m,const Input& q,Capture& out,check::Checks& checks)noexcept{
    auto& v=out.observation;
    const auto issue=station_recipe_snapshot_detail::begin_hands(m,q.job.context.hero,hands_);
    checks.equal("hand.read",0,static_cast<int>(issue));
    const bool read=issue==StationHandReadIssue::none;
    const auto& h=hands_.before;const auto& left=h.hands.left;
    v.hand={h.left_slot_known&&h.right_slot_known&&h.interact_item_known,read,left.object,h.hands.right.object,
      static_cast<std::uint32_t>(left.symbol),static_cast<std::uint32_t>(left.amount),h.interact_item,h.left_tmp_level};
    checks.flag("hand.slots",v.hand.known);
    checks.test("hand.left_object",">0",left.object,left.object!=0,read);
    checks.equal("hand.right_object",0,v.hand.right,read);
    checks.equal("hand.symbol",16612,left.symbol,read);
    checks.equal("hand.amount",1,left.amount,read);
    checks.equal("hand.anchor",left.object,h.interact_item,read);
    checks.test("hand.tmp_level","1 oder 2",h.left_tmp_level,h.left_tmp_level==1||h.left_tmp_level==2,read);
    checks.equal("hand.left_state",1,left.state,read);
    checks.equal("hand.right_state",0,h.hands.right.state,read);
    std::uint32_t hv=0,hm=0,hf=0;
    const bool type=read&&detail::word(m,left.object,0,hv);
    const bool main=read&&detail::word(m,left.object,0x154,hm);
    const bool flags=read&&detail::word(m,left.object,0x158,hf);
    checks.equal("hand.vtable",hand_detail::item_vtable,hv,type);
    checks.equal("hand.mainflag",1,hm,main);
    checks.equal("hand.flags",generated_blade_hand_shape::flags,hf,flags);
    if(!read)return;
    Sha256 hand;hand.update(hands_.header.data(),hands_.header.size());hand.update(hands_.anchor.data(),hands_.anchor.size());
    hand.update(hands_.pointers.data(),hands_.count*4);detail::hw(hand,hv);detail::hw(hand,hm);detail::hw(hand,hf);
    for(std::size_t i=0;i<hands_.count;++i){const auto& s=hands_.records[i];detail::hw(hand,s.address);
      hand.update(s.fields.data(),s.fields.size());hand.update(s.name.data(),s.length+1);}
    v.hand_identity=hand.finish();
    out.hands=h;
  }
  Capture sample(const ReadOnlyMemory& traced,const Input& q,const cr::ProfileAssembly& assembly,check::Report* report)noexcept{
    Capture out;auto& v=out.observation;const auto& c=q.job.context;
    v.job=q.job;v.scope=q.scope;v.serial=q.serial;v.frame=q.frame;
    const bool after=q.phase==Phase::after_original;
    // AFTER re-resolves its objects. Old event managers, Split pool and ITEM are then taboo.
    check::Checks pool_checks(report);pool_checks.reason=static_cast<int>(Issue::pool);
    const bool split=split_pending(q.phase);
    const auto writes=split?detail::pool_writes(traced,pool_checks):detail::PoolWrites{};
    detail::ProtectedReads protected_reads{traced,writes};const auto m=protected_reads.memory();
    context_check::Checks checks(m,report);checks.first=pool_checks.first;
    const auto group=[&](Issue issue){checks.reason=static_cast<int>(issue);};
    const auto finish=[&](){
      if(protected_reads.conflict){
        check::Checks alias(report);alias.reason=static_cast<int>(Issue::alias);
        alias.equal("alias.pool_read",0,protected_reads.conflict);checks.first=alias.first;
        out.failed_address=protected_reads.conflict;
      }
      out.failure=checks.first;out.issue=static_cast<Issue>(out.failure.reason);
      out.known=checks.ok();
      if(!out.known){v.stable_read=v.native_split_restore=v.fence.known=v.fence.path_excludes_dispatch=false;
        out.restore.targets.known=out.restore.native_set_item=false;}
      return out;
    };
    if(split){
      group(Issue::pool);
      checks.equal("pool.read",0,static_cast<int>(writes.pool.issue));
      checks.flag("pool.write_ranges",writes.known);
      if(!report&&!checks.ok())return finish();
      group(Issue::engine);detail::engine(m,q,checks);
      if(!checks.ok()&&checks.first.field&&(!std::strcmp(checks.first.field,"engine.code")||
         !std::strcmp(checks.first.field,"engine.data")||!std::strcmp(checks.first.field,"engine.split")))
        out.failed_address=static_cast<std::uint32_t>(std::strtoul(checks.first.actual,nullptr,10));
      if(!report&&!checks.ok())return finish();
      group(Issue::module);const auto code=dep::verify_known_code(m,q.modules,dep::expected_targets,true);
      checks.equal("module.code_issue",0,static_cast<int>(code.issue));
      checks.test("module.code_address","Profil oder Weiterleitung",code.address,code.known_ranges_match);
      if(!code.known_ranges_match)out.failed_address=code.address;
      const auto base=q.modules.parser_extender.base;
      if(code.zpe_init_call){
        std::uint32_t parser=0,record=0,header=0;
        const bool p=detail::word(m,base,0xc0b1c,parser),r=detail::word(m,base,0xc44c0,record);
        const bool h=record&&!(record%4)&&detail::word(m,record,0,header);
        checks.equal("module.zpe_parser",c.parser,parser,p);
        checks.test("module.zpe_record","ausgerichteter Record",record,record&&!(record%4),r);
        checks.test("module.zpe_header",">0",header,header!=0,h);
      }else{
        for(const auto field:{"module.zpe_parser","module.zpe_record","module.zpe_header"})
          checks.record(field,"nur im Wrapper","Original",true);
      }
      const auto objects=dep::read_objects(m,c);
      checks.equal("module.objects",0,static_cast<int>(objects.issue));
      checks.flag("module.item_class",objects.stable&&objects.flask_class_is_vob_item);
      const zbug_end_compat::Binding plugins{q.modules.zbug,q.modules.union_core,true};
      for(const auto slot:{0x82e308u,0x82e30cu}){
        std::uint32_t entry=0;const bool read=detail::word(m,slot,0,entry);
        checks.test(slot==0x82e308?"module.alloc":"module.free","ausfuehrbares Modulziel",entry,
          zbug_end_compat::detail::cooling_forward(plugins,entry),read);
      }
      group(Issue::crt);detail::crt(m,q,checks);
    }else if(q.phase==Phase::restore){
      // After the single Remove the free list and constructor state may
      // have changed. Writing is done only via these three parser setters.
      group(Issue::restore);
      constexpr auto pop=rt::code_ranges[8],instance=rt::code_ranges[6],item=dep::additional_ranges[18];
      static_assert(pop.address==0x7a4b20 && instance.address==0x7a1550 && item.address==0x7a2f20);
      for(const auto& setter:{pop,instance,item})
        checks.test("restore.setter_code","Originalsetter",setter.address,dep::detail::plain(m,setter).valid);
    }
    group(Issue::context);checks.identity(c);if(!after)checks.station(c);
    group(Issue::script);const auto script=cr::verify_recipe(m,c.parser,cool_recipe_catalog::recipe);
    checks.equal("script.profile",0,static_cast<int>(script.local.issue));
    checks.equal("script.string",0,static_cast<int>(script.string_issue));
    checks.test("script.symbol","verifizierter Rezeptvertrag",script.failed_symbol?script.failed_symbol:script.local.failed_symbol,script.valid);
    group(Issue::parser);const auto parser=rt::detail::parser_once(m,c,false);
    checks.equal("parser.read",0,static_cast<int>(parser.issue));
    checks.flag("parser.binding",parser.valid);v.parser=parser.value;
    if(!after){
      checks.equal("parser.data_depth",0,v.parser.data_depth,parser.valid);
      if(v.parser.data_depth)out.failed_address=c.parser+station_abi::parser_data_stack_count;
    }
    detail::Literals literals{};
    if(split){group(Issue::constructor);detail::constructor(m,q,parser.layout,literals,checks);}
    group(Issue::parser);
    if(q.phase!=Phase::restore)checks.flag("parser.prefix",prefix::classify(v.parser,c,callback,
      after?prefix::Phase::after_original:prefix::Phase::before_original).recognized);
    if(!after){
      const auto item=dep::read_item_binding(m,c.parser);v.item=detail::item(item);
      checks.flag("parser.item_binding",item.known);
      checks.flag("parser.item_target",old::valid_item(v.item,v.parser));
    }
    group(Issue::context);checks.production(c,assembly,after,v);if(split)checks.flags(c);
    group(Issue::info);detail::info(m,q,checks);
    if(!after){
      group(Issue::queue);detail::queue(m,q,v.fence,checks);
      if(split){group(Issue::alias);checks.flag("alias.event_manager",detail::literal_disjoint(literals,v.fence.event_manager,0x40));}
    }
    group(Issue::inventory);
    const auto inv=read_packed_inventory(m,c.hero,cool_recipe_catalog::targets,true,inventory_);
    checks.equal("inventory.read",0,static_cast<int>(inv.issue));
    checks.flag("inventory.totals",inv.storage_totals_known);
    checks.flag("inventory.unchanged",inv.snapshot_unchanged);
    checks.test("inventory.nodes","1..Workspace",inv.listed_nodes,inv.listed_nodes&&inv.listed_nodes<=inventory_.records.size());
    checks.equal("inventory.packed_hot",0,inv.packed[0]);
    checks.equal("inventory.listed_hot",1,inv.listed_occurrences[0]);
    checks.equal("inventory.packed_occurrences",0,inv.packed_occurrences[0]);
    checks.equal("inventory.active_marker",0,inv.has_active_marker[0]);
    checks.equal("inventory.mixed_hot",0,inv.mixed_representation[0]);
    checks.equal("inventory.duplicate_hot",0,inv.duplicate_packed_target[0]);
    checks.test("inventory.cold",after?"2..INT_MAX":"0..INT_MAX-2",inv.total[1],
      after?(inv.total[1]>=2&&inv.total[1]<=INT32_MAX):(inv.total[1]>=0&&inv.total[1]<=INT32_MAX-2));
    const auto expected=split?2:1;
    if(q.phase==Phase::restore)checks.test("inventory.hot","1 oder 2",inv.total[0],inv.total[0]==1||inv.total[0]==2);
    else checks.equal("inventory.hot",expected,inv.total[0]);
    group(Issue::hand);read_hand(m,q,out,checks);
    const auto count=std::min<std::size_t>(inv.listed_nodes,inventory_.records.size());
    v.stocks={inv.storage_totals_known,{{{16612,inv.total[0],true,true},{16613,inv.total[1],true,false}}}};
    if(after){
      checks.flag("hand.unchanged",station_recipe_snapshot_detail::verify_hands(m,hands_));
      group(Issue::alias);
      for(std::size_t i=0;i<count;++i){
        const auto object=rs::detail::u32(inventory_.records[i].node.data()+4);
        checks.test("alias.hand_storage","getrennt vom Handobjekt",object,!detail::overlaps(object,0x348,v.hand.left,0x348));
      }
      v.stable_read=v.generated_hand_profile=checks.ok();return finish();
    }
    const auto& h=hands_.before;const auto& left=h.hands.left;
    Sha256 identity;packed_detail::Header header{};
    group(Issue::inventory);
    const bool have_header=packed_detail::at_offset(m,c.hero,packed_detail::header_offset,header.data(),header.size());
    checks.flag("inventory.header",have_header);identity.update(header.data(),header.size());
    const auto pack_length=rs::detail::u32(header.data()+32);
    const bool pack_fits=pack_length<inventory_.packed_bytes.size();
    checks.test("inventory.pack_length","innerhalb des Lesepuffers",pack_length,pack_fits);
    if(pack_length&&pack_fits)identity.update(inventory_.packed_bytes.data(),pack_length+1);
    bool equipped=false;
    for(std::size_t i=0;i<count;++i){
      const auto& r=inventory_.records[i];const auto object=rs::detail::u32(r.node.data()+4);
      bn::path::detail::Fields fields;const bool known=bn::path::detail::fields(m,object,fields);
      group(Issue::inventory);
      checks.equal("inventory.item_vtable",hand_detail::item_vtable,fields.vptr,known);
      group(Issue::alias);
      checks.test("alias.object_range","gueltiger Itembereich",object,object&&!(object%4)&&rs::detail::range(object,0x348));
      if(split){
        checks.test("alias.literal_item","getrennt von Text",object,detail::literal_disjoint(literals,object,0x348));
        checks.test("alias.literal_node","getrennt von Text",r.address,detail::literal_disjoint(literals,r.address,r.node.size()));
      }
      bool separate=true;
      for(std::size_t j=0;j<i;++j)if(detail::overlaps(object,0x348,rs::detail::u32(inventory_.records[j].node.data()+4),0x348))separate=false;
      checks.test("alias.listed_items","getrennte Lagerobjekte",object,separate);
      bool ordinary_equipment=false;
      for(std::size_t j=0;hands_.captured&&j<hands_.count;++j){
        const auto& slot=hands_.records[j];const auto slot_object=rs::detail::u32(slot.fields.data()+0x30);
        const auto level=hand_detail::i32(slot.fields.data()+0x18);
        const bool ordinary=slot_object==object&&fields.symbol!=16612&&level==0&&
          slot_object!=h.hands.left.object&&slot_object!=h.hands.right.object&&slot_object!=h.interact_item;
        ordinary_equipment|=ordinary;
        checks.test("alias.equipment","getrennt oder ausgeruestet",slot_object,
          !slot_object||!detail::overlaps(slot_object,0x348,object,0x348)||ordinary);
        if(split){
          checks.test("alias.slot","getrennt von Text",slot.address,detail::literal_disjoint(literals,slot.address,slot.fields.size()));
          checks.test("alias.slot_item","getrennt von Text",slot_object,!slot_object||detail::literal_disjoint(literals,slot_object,0x348));
        }
      }
      checks.test("alias.interact_item","getrennt vom Handanker",object,object!=h.interact_item);
      detail::hw(identity,r.address);identity.update(r.node.data(),r.node.size());
      if(ordinary_equipment){
        equipped=true;
        // Newly admitted double occupancy: not just quantity sum,
        // but all current object bytes plus already fully bound slot.
        std::array<std::uint8_t,0x348> bytes{};
        group(Issue::inventory);checks.test("inventory.equipment","lesbares Objekt",object,m.copy(object,bytes.data(),bytes.size()));
        identity.update(bytes.data(),bytes.size());
      }
      for(const auto number:{object,fields.vptr,fields.refcount,fields.mainflag,fields.flags,fields.symbol})detail::hw(identity,number);
      identity.update(fields.damages.data(),fields.damages.size());
      detail::hw(identity,fields.symbol==16612?0:fields.amount);
      if(fields.symbol==16612){
        v.stack={true,true,true,false,0,1,object,16612,fields.amount,fields.refcount,fields.vptr,fields.mainflag,fields.flags};
        group(Issue::inventory);
        checks.test("inventory.hot_refs","1..INT_MAX",fields.refcount,fields.refcount&&fields.refcount<=INT32_MAX,known);
        checks.equal("inventory.hot_mainflag",1,fields.mainflag,known);
        checks.equal("inventory.hot_flags",generated_blade_hand_shape::stored_flags,fields.flags,known);
        checks.equal("inventory.hot_amount",inv.total[0],fields.amount,known);
      }
    }
    if(!equipped)checks.record("inventory.equipment","falls ausgeruestet","kein Doppelbezug",true);
    group(Issue::inventory);checks.test("inventory.hot_object",">0",v.stack.object,v.stack.object!=0);
    v.inventory_identity=identity.finish();v.complete_inventory=inv.storage_totals_known;
    std::uint32_t contents=0,selected=0,disabled=0;
    const bool owner_read=detail::word(m,c.hero,0x66c,contents);
    checks.equal("inventory.contents",c.hero+0x710,contents,owner_read);
    // Split accesses the listed stack by instance. The two UI values do not choose a write target.
    if(report){
      const bool selection=detail::word(m,c.hero,0x68c,selected),disable=detail::word(m,c.hero,0x700,disabled);
      checks.test("inventory.selected","beobachtet",selected,true,selection);
      checks.test("inventory.disabled","beobachtet",disabled,true,disable);
    }
    if(split){
      group(Issue::alias);
      std::uint32_t a_text=0,a_protected=0,a_queue=0,a_hand=0,a_inventory=0,a_slots=0;
      for(std::size_t i=0;i<writes.count;++i){const auto& w=writes.ranges[i];if(w.control)continue;
        if(!detail::literal_disjoint(literals,w.address,w.length))a_text=w.address;
        if(detail::protected_range(q,parser.layout,w.address,w.length))a_protected=w.address;
        if(detail::overlaps(w.address,w.length,v.fence.event_manager,0x40))a_queue=w.address;
        if(detail::overlaps(w.address,w.length,left.object,0x348))a_hand=w.address;
        for(std::size_t j=0;j<count;++j)if(detail::overlaps(w.address,w.length,rs::detail::u32(inventory_.records[j].node.data()+4),0x348))a_inventory=w.address;
        for(std::size_t j=0;hands_.captured&&j<hands_.count;++j){const auto object=rs::detail::u32(hands_.records[j].fields.data()+0x30);
          if(object&&detail::overlaps(w.address,w.length,object,0x348))a_slots=w.address;}
      }
      checks.equal("alias.pool_text",0,a_text);checks.equal("alias.pool_target",0,a_protected);
      checks.equal("alias.pool_queue",0,a_queue);checks.equal("alias.pool_hand",0,a_hand);
      checks.equal("alias.pool_storage",0,a_inventory);checks.equal("alias.pool_slot",0,a_slots);
      group(Issue::world);
      // During Split, new objects are inserted into both lists. The protected
      // read view also notices a pool write area in the middle of a world node.
      for(unsigned list=0;list<2;++list){
        std::uint32_t header_address=0;const bool address_read=detail::word(m,c.world,list?0x6288:0x6280,header_address);
        std::array<std::uint8_t,12> values{};const bool known=address_read&&m.copy(header_address,values.data(),values.size());
        checks.test(list?"world.item_header":"world.vob_header","lesbarer Listenkopf",header_address,known);
        checks.equal(list?"world.item_sorter":"world.vob_sorter",list?0x77ef90u:0x77ef50u,rs::detail::u32(values.data()),known);
        auto node=known?rs::detail::u32(values.data()+8):0u;
        unsigned visited=0;std::uint32_t bad_node=0,missing_item=0;
        while(node&&visited<65536){
          if(!m.copy(node,values.data(),values.size())){bad_node=node;break;}
          if(!rs::detail::u32(values.data()+4)){missing_item=node;break;}
          node=rs::detail::u32(values.data()+8);++visited;
        }
        checks.equal(list?"world.item_unreadable":"world.vob_unreadable",0,bad_node);
        checks.equal(list?"world.item_empty":"world.vob_empty",0,missing_item);
        checks.test(list?"world.item_count":"world.vob_count","begrenzt und vollstaendig",visited,!node);
      }
      checks.equal("alias.pool_read",0,protected_reads.conflict);
    }
    group(Issue::changed);
    checks.flag("changed.hands",hands_.captured&&station_recipe_snapshot_detail::verify_hands(m,hands_));
    checks.equal("changed.list",0,static_cast<int>(packed_detail::compare_list(m,inventory_,static_cast<std::uint32_t>(count))));
    checks.equal("changed.pack",0,static_cast<int>(pack_fits?packed_detail::compare_pack(m,inventory_,rs::detail::u32(header.data()+28),pack_length):PackedInventoryIssue::invalid_pack_length));
    v.fence.known=v.fence.path_excludes_dispatch=v.stack.native_split_path_verified=checks.ok();
    v.native_split_restore=v.generated_hand_profile=v.stable_read=checks.ok();
    if(reads_saved_call(q.phase)&&q.call){
      group(Issue::restore);const auto& saved=*q.call;
      checks.equal("restore.stack",saved.stack_object,v.stack.object);
      checks.equal("restore.hand",saved.generated_hand,v.hand.left);
      checks.digest("restore.inventory",saved.inventory_identity,v.inventory_identity);
      checks.digest("restore.slots",saved.hand_identity,v.hand_identity);
      checks.equal("restore.manager",saved.fence.event_manager,v.fence.event_manager);
      checks.digest("restore.queue",saved.fence.queue_fingerprint,v.fence.queue_fingerprint);
      checks.flag("restore.parser",old::same_parser_identity(saved.parser,v.parser));
      checks.flag("restore.data_stack",value::same_data_stack(saved.parser,v.parser));
      checks.flag("restore.item_target",old::same_item_target(saved.item,v.item,saved.parser));
      // SetUseInstance re-reads the stored symbol address. No
      // reference back to a possibly already freed ITEM value.
      rs::detail::Symbol use;
      const auto id=saved.parser.use_symbol==saved.parser.self_symbol?1679u:1683u;
      const bool known=rs::detail::lookup(m,parser.layout,id,use)==rs::Issue::none;
      checks.equal("restore.use_symbol",saved.parser.use_symbol,use.address,known);
      checks.equal("restore.use_address",c.hero,use.offset,known);
      auto& r=out.restore;r.job=q.job;r.scope=q.scope;r.serial=q.serial;r.frame=q.frame;r.current=v.parser;r.item=v.item;r.fence=v.fence;r.native_set_item=true;
      r.inventory_identity=v.inventory_identity;r.hand_identity=v.hand_identity;
      const auto& s=saved.parser;r.targets={q.job.context,q.scope.writer,true,true,s.current_parser,s.code_base,s.code_size,
        s.use_symbol,s.use_address,v.parser.self_symbol,v.parser.hero_symbol,v.parser.self,v.parser.hero,v.parser.other,v.parser.victim};
      checks.flag("restore.plan",restore_plan(saved,r).valid);
    }
    return finish();
  }
  const std::uint32_t owner_thread_;std::uint64_t serial_=0;bool reading_=false,reentered_=false;
  PackedInventoryWorkspace inventory_{};station_recipe_snapshot_detail::HandWorkspace hands_{};
};
} // namespace ubc::cool_predebit::reader
