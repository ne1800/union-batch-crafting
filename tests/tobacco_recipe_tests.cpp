// SPDX-License-Identifier: MIT
#include "ubc/tobacco_runtime.hpp"
#include "ubc/crafting_session.hpp"
#include "bow_item_test_values.hpp"
#include <cstdio>
#include <cstdlib>

namespace {
namespace t=ubc::tobacco;
void require_at(bool ok,int line){if(!ok){std::fprintf(stderr,"Tabakprüfung in Zeile %d\n",line);std::exit(1);}}
#define REQUIRE(...) require_at((__VA_ARGS__),__LINE__)

void orders(const t::Descriptor& d){
  const t::Stock before{{6,8,1,7}};
  REQUIRE(t::maximum(before)==6);
  REQUIRE(t::permitted_maximum(before,[](unsigned n){return n<6;})==5);
  REQUIRE(t::permitted_maximum(before,[](unsigned){return false;})==1);
  REQUIRE(t::permitted_maximum(before,[](unsigned){return true;})==6);
  for(const auto n:{1u,3u,6u}){
    const auto plan=ubc::make_crafting_plan(ubc::crafting_page(ubc::recipe_navigation::Page::root),d.id,n);
    REQUIRE(ubc::valid_crafting_plan(plan)&&plan.count==3&&plan.steps[0].function_symbol==20389);
    REQUIRE(plan.steps[1].function_symbol==20401&&plan.steps[1].owner_instance==20399);
    REQUIRE(plan.effect()->function_symbol==static_cast<int>(d.callback.function->symbol));
    REQUIRE(!ubc::make_crafting_plan(plan.effect()->before,d.id,n).valid());
  }
  REQUIRE(t::maximum({12,2,1,0})==3&&t::maximum({12,0,1,0})==1);
  REQUIRE(t::maximum({6,6,0,0})==0&&t::maximum({6,6,1,INT32_MAX-2})==2);

}
void menu_and_payment(const t::Descriptor& d){
  ubc::CraftingSnapshot preview;
  preview.key={1,2,3,4,5,6,ubc::crafting_catalog::Station::alchemy};
  preview.hands.hands={{0,-1,-1,0},{1,t::flask,1,900}};
  preview.hands.left_slot_known=preview.hands.right_slot_known=preview.hands.interact_item_known=true;
  preview.hands.interact_item=900;
  auto& state=preview.recipes[ubc::crafting_catalog::tobacco_begin+d.id-0xb001];
  state.evaluated=state.knowledge_read_valid=state.known=state.contract=state.actor_known=state.actor_matches=true;
  state.material_known=state.hand_known=state.material_verified=state.context_known=state.context_matches=true;
  state.generation=1;state.materials[1].consumed_reserve=1;
  const t::Stock before{{6,5,1,0}};
  for(std::size_t i=0;i<4;++i)state.materials[i].stored=before[i];
  state.maximum=t::maximum(before);
  REQUIRE(ubc::valid_crafting_snapshot(preview));
  REQUIRE(ubc::crafting_amount_preview(state.materials[2],6).original_minimum==1);
  ubc::CraftingSession session;REQUIRE(session.open(preview,d.id));
  for(std::uint64_t f=1;f<=3;++f)session.observe(f,false,true);
  REQUIRE(session.back()&&!session.take(preview,true));
  REQUIRE(t::stocks(state)==before);
  ubc::GrindCheck check;
  REQUIRE(t::match_stock(d,before,t::stocks(state),check));
  --state.materials[0].stored;
  REQUIRE(!t::match_stock(d,before,t::stocks(state),check)&&check.symbol==t::apple&&check.expected==6&&check.actual==5);
  t::OwnerTicket owner{preview.key,17,38,0,150,200,static_cast<int>(d.callback.function->symbol)};
  REQUIRE(owner.owns(preview.key,200,150)&&!owner.owns(preview.key,200,100)&&!owner.bind(38));
  owner.observe(false,false,39,1,0);REQUIRE(owner.ticket==17);
  owner.observe(false,true,39,2,150);REQUIRE(owner.bind(39)&&!owner.bind(40));
  ubc::CraftingHandoffTicket child{18,100,1,200,{true,ubc::recipe_catalog::Invocation::choice,20399,owner.function,0,39}};
  REQUIRE(owner.accepts(child,owner.function)&&!owner.accepts(child,20402));
  ++child.offer.collection;REQUIRE(!owner.accepts(child,owner.function));--child.offer.collection;
  owner.observe(false,true,40,2,150);REQUIRE(owner.ticket==17&&owner.choice_collection==39);
  owner.spent=true;REQUIRE(!owner.accepts(child,owner.function));
  REQUIRE(owner.bound_child(child,owner.function,39)&&!owner.bound_child(child,owner.function,40));
  owner.observe(false,false,41,1,0);REQUIRE(owner.ticket);
  owner.observe(false,true,41,1,0);REQUIRE(!owner.ticket);
}
struct Ram {
  std::array<std::uint8_t,512> bytes{};
  unsigned calls=0,fail=0;bool change_owner_on_child_write=false;
  bool access(){return ++calls!=fail;}
  static Ram& get(void* p){return *static_cast<Ram*>(p);}
  ubc::AmountMemory adapter(){
    return {{this,[](void* p,std::uint32_t a,void* out,std::size_t n) noexcept {
      auto& f=get(p);if(!f.access()||a+n>f.bytes.size())return false;
      std::memcpy(out,f.bytes.data()+a,n);return true;
    }},this,[](void* p,std::uint32_t a,std::size_t n) noexcept {return get(p).access()&&a+n<=512;},
    [](void* p,std::uint32_t a,std::size_t n) noexcept {return get(p).access()&&a+n<=512;},
    [](void* p,std::uint32_t a,const void* old,const void* val,std::size_t n) noexcept {
      auto& f=get(p);if(!f.access()||a+n>512||std::memcmp(f.bytes.data()+a,old,n))return false;
      std::memcpy(f.bytes.data()+a,val,n);
      if(f.change_owner_on_child_write&&a==256)f.bytes[16+188]^=1;
      return true;
    }};
  }
};

constexpr unsigned owner_at=16,child_at=256;
struct Vm {
  const t::Descriptor& recipe;
  t::Stock stock;
  int shortened=-1,missing=0,remove_return=1;
  unsigned prints=0,ends=0,flask_removals=0;
  void run(const Ram& ram){
    const auto owner=t::owner.information->byte_offset,child=recipe.callback.function->byte_offset;
    std::vector<unsigned> stack,returns;
    auto pop=[&]{REQUIRE(!stack.empty());const auto v=stack.back();stack.pop_back();return v;};
    unsigned pc=child;
    for(unsigned step=0;step<128;++step){
      const bool in_owner=pc>=owner&&pc<owner+189;
      REQUIRE(in_owner||(pc>=child&&pc<child+36));
      const auto pos=in_owner?owner_at+pc-owner:child_at+pc-child;
      const auto op=ram.bytes[pos];
      if(op==60){
        if(returns.empty()){REQUIRE(stack.empty());return;}
        pc=returns.back();returns.pop_back();continue;
      }
      if(op==16||op==1||op==8){
        const auto top=pop(),other=pop();
        stack.push_back(op==16?unsigned(top==other):op==1?top-other:unsigned(top>other));++pc;continue;
      }
      const auto arg=bow_item_test::u32(ram.bytes.data()+pos+1);pc+=5;
      if(op==75)pc=arg;
      else if(op==76){if(!pop())pc=arg;}
      else if(op==64||op==65||op==67)stack.push_back(arg);
      else if(op==61&&arg==672156)++ends;
      else if(op==61){returns.push_back(pc);pc=arg;}
      else if(op==62&&arg==12){REQUIRE(pop()==t::success_text.symbol);++prints;}
      else{
        REQUIRE(op==62&&(arg==160||arg==170||arg==184));
        int count=arg==160?0:static_cast<int>(pop());
        const auto symbol=pop();REQUIRE(pop()==1683);
        unsigned i=0;while(i<4&&static_cast<unsigned>(recipe.targets[i].symbol)!=symbol)++i;
        REQUIRE(i<4&&i!=2);
        if(arg==160){stack.push_back(static_cast<unsigned>(stock[i]));continue;}
        REQUIRE(count>0);
        if(shortened==static_cast<int>(i)){REQUIRE(missing<=count);count-=missing;shortened=-1;}
        if(arg==170)stock[i]+=count;
        else{stock[i]-=count;stack.push_back(static_cast<unsigned>(remove_return));if(i==1)++flask_removals;}
        REQUIRE(stock[i]>=0);
      }
    }
    REQUIRE(false);
  }
};

void transaction(const Ram& sources,t::Descriptor const& d,unsigned n){
  auto owner=*t::owner.information,child=*d.callback.function;
  owner.body_sha256=bow_item_test::hash(sources.bytes.data()+owner_at,189);
  child.body_sha256=bow_item_test::hash(sources.bytes.data()+child_at,36);
  const t::Stock before{{6,5,1,3}};
  for(bool choice:{false,true}){
    unsigned last=0;
    for(unsigned fault=0;fault<=last;++fault){
      Ram f=sources;f.fail=fault;t::Transaction tx;t::Transaction::PaymentCode pay;t::Checks checks;
      bool ready=tx.owner_body.capture(f.adapter(),owner_at,owner);
      ready=tx.child_body.capture(f.adapter(),child_at,child)&&ready;
      if(choice)ready=tx.prepare_child(d,n,before,pay)&&ready;
      else tx.defer_owner();
      ready=ready&&tx.install(choice,checks)&&tx.at_call(checks);
      if(!fault)last=f.calls;
      REQUIRE(ready==!fault);
      if(ready&&!choice)REQUIRE(f.bytes[owner_at]==75&&bow_item_test::u32(f.bytes.data()+owner_at+1)==owner.byte_offset+20);
      if(ready){
        // Change in the previously unchanged tail of exactly one window before CallFunc.
        for(auto pos:{owner_at+188,child_at+35}){
          f.bytes[pos]^=1;REQUIRE(!tx.at_call(checks));f.bytes[pos]^=1;
        }
      }
      REQUIRE(tx.restore()&&f.bytes==sources.bytes);
    }
  }
  {
    Ram f=sources;t::Transaction tx;t::Transaction::PaymentCode pay;t::Checks checks;
    REQUIRE(tx.owner_body.capture(f.adapter(),owner_at,owner)&&tx.child_body.capture(f.adapter(),child_at,child));
    REQUIRE(tx.prepare_child(d,n,before,pay)&&tx.install(true,checks));
    f.change_owner_on_child_write=true;
    REQUIRE(tx.payment_program(pay,checks)&&!tx.at_call(checks));
    f.change_owner_on_child_write=false;REQUIRE(!tx.restore());
    // Do not overwrite an unrelated byte during rollback.
    REQUIRE(f.bytes[owner_at+188]!=sources.bytes[owner_at+188]);
  }
  {
    Ram f=sources;t::Transaction tx;t::Transaction::PaymentCode pay;t::Checks checks;
    REQUIRE(tx.owner_body.capture(f.adapter(),owner_at,owner)&&tx.child_body.capture(f.adapter(),child_at,child));
    REQUIRE(tx.prepare_child(d,n,before,pay)&&tx.install(true,checks));
    auto observed=before;--observed[0];
    const auto code=tx.adjustment(d,0,1,observed);
    REQUIRE(tx.payment_program(code,checks)&&tx.at_call(checks));
    Vm vm{d,before};vm.run(f);REQUIRE(vm.stock==before&&!vm.prints&&!vm.ends);
    REQUIRE(tx.restore()&&f.bytes==sources.bytes);
  }
  auto exercise=[&](int slot,int missing,unsigned finish_fault,int remove_return){
    Ram f=sources;t::Transaction tx;t::Transaction::PaymentCode pay;t::Checks checks;
    REQUIRE(tx.owner_body.capture(f.adapter(),owner_at,owner)&&tx.child_body.capture(f.adapter(),child_at,child));
    REQUIRE(tx.prepare_child(d,n,before,pay)&&tx.install(true,checks)&&tx.at_call(checks));
    Vm vm{d,before,slot,missing,remove_return};
    vm.run(f); // Product output only.
    REQUIRE(vm.stock[0]==before[0]&&vm.stock[1]==before[1]&&!vm.prints&&!vm.ends);
    unsigned calls=0;
    const auto read=[&](auto& value){value=vm.stock;return true;};
    const auto adjust=[&](unsigned i,unsigned count,const t::Stock& observed){
      const auto code=tx.adjustment(d,i,count,observed);
      REQUIRE(tx.payment_program(code,checks)&&tx.at_call(checks));
      vm.run(f);return true;
    };
    const auto finish=[&](const t::Stock& paid,bool& entered){
      const auto start=f.calls;f.fail=finish_fault?start+finish_fault:0;
      const bool allowed=tx.payment_program(pay,checks)&&tx.at_call(checks);
      if(allowed){
        entered=true;vm.run(f);
        REQUIRE(!vm.ends&&!vm.prints);
        if(vm.stock!=paid){
          REQUIRE(vm.stock[0]>=paid[0]&&vm.stock[1]>=paid[1]);
          if(slot==0)REQUIRE(vm.flask_removals==0);
        }else if(tx.payment_program(tx.completion(),checks)&&tx.at_call(checks)){
          vm.run(f);calls=f.calls-start;f.fail=0;return true;
        }
      }
      calls=f.calls-start;f.fail=0;return false;
    };
    const auto result=t::settle(before,n,read,adjust,finish,checks);
    const bool success=slot<0&&!finish_fault;
    REQUIRE(result==(success?t::Settlement::completed:t::Settlement::unpaid));
    REQUIRE(vm.stock==(success?t::Stock{{6-static_cast<int>(n),6-static_cast<int>(n),1,3+static_cast<int>(n)}}:before));
    REQUIRE(vm.ends==unsigned(success)&&vm.prints==unsigned(success));
    REQUIRE(tx.restore()&&f.bytes==sources.bytes);
    return calls;
  };
  const auto last=exercise(-1,0,0,1);
  for(unsigned fault=1;fault<=last;++fault)exercise(-1,0,fault,1);
  for(int result:{0,1}){
    exercise(-1,0,0,result);
    for(int slot:{3,0,1})for(unsigned missing=1;missing<=(slot==1?n-1:n);++missing)
      exercise(slot,static_cast<int>(missing),0,result);
  }
}
Ram example(const t::Descriptor& recipe){
  // Synthetic windows for the regular suite. The archive test uses the actual hashed bodies.
  Ram ram;auto* o=ram.bytes.data()+owner_at;auto* c=ram.bytes.data()+child_at;
  const auto op=t::Transaction::jump;
  op(o,67,1683);op(o+5,64,t::apple);op(o+10,64,1);op(o+15,62,184);o[188]=60;
  op(o+50,64,1);op(o+55,67,1683);op(o+60,64,1);op(o+65,62,160);
  op(c,67,1683);op(c+5,64,static_cast<unsigned>(recipe.outputs[0].symbol));op(c+10,64,1);op(c+15,62,170);
  op(c+20,65,t::success_text.symbol);op(c+25,62,12);op(c+30,61,672156);c[35]=60;
  return ram;
}
void refund_failures(){
  const t::Stock before{{3,2,1,7}},produced{{3,2,1,10}};
  for(int fault=0;fault<9;++fault){
    auto stock=produced;unsigned reads=0,writes=0;t::Checks checks;
    auto read=[&](auto& out){out=stock;++reads;return fault!=1&&(fault!=2||reads!=3);};
    auto adjust=[&](unsigned i,unsigned missing,const t::Stock&){
      ++writes;
      if(fault==3)return false;
      const auto n=fault==4?0:fault==5?1:static_cast<int>(missing);
      stock[i]+=i==3?-n:n;
      if(fault==6)++stock[2];
      return true;
    };
    const auto finish=[&](const t::Stock&,bool& entered){
      if(fault!=7)entered=true;
      --stock[0];if(fault==8)stock[1]=-1;
      return false;
    };
    const auto result=t::settle(before,3,read,adjust,finish,checks);
    if(fault==0||fault==5)REQUIRE(result==t::Settlement::unpaid&&stock==before);
    else REQUIRE(result==t::Settlement::uncertain);
    if(fault==1||fault==7||fault==8)REQUIRE(!writes);
    if(fault>=2&&fault<=4)REQUIRE(writes==1);
  }
}
void archive(const char* path){
  const bow_item_test::Dat dat(path);
  for(const auto& f:t::functions){
    REQUIRE(f.byte_offset+f.byte_length<=dat.code.size());
    REQUIRE(bow_item_test::hash(dat.code.data()+f.byte_offset,f.byte_length)==f.body_sha256);
  }
  const auto word=[&](unsigned start,unsigned offset){return bow_item_test::u32(dat.code.data()+start+offset+1);};
  const auto owner=t::owner.information->byte_offset;
  REQUIRE(word(owner,0)==1683&&word(owner,5)==16660&&word(owner,10)==1&&word(owner,15)==184);
  REQUIRE(word(t::functions[3].byte_offset,0)==20399&&word(t::functions[3].byte_offset,5)==word(owner,25));
  for(std::size_t i=0;i<t::recipes.size();++i){
    const auto& d=t::recipes[i];
    Ram actual;
    std::copy_n(dat.code.data()+owner,189,actual.bytes.data()+owner_at);
    std::copy_n(dat.code.data()+d.callback.function->byte_offset,36,actual.bytes.data()+child_at);
    for(unsigned n:{1u,2u,3u,6u})transaction(actual,d,n);
    const auto offer=50+static_cast<unsigned>(46*i);
    REQUIRE(word(owner,offer)==1&&word(owner,offer+5)==1683&&word(owner,offer+10)==static_cast<unsigned>(d.targets[2].symbol));
    REQUIRE(word(owner,offer+15)==160&&word(owner,offer+36)==d.callback.function->symbol);
    const auto child=d.callback.function->byte_offset;
    REQUIRE(word(child,0)==1683&&word(child,5)==static_cast<unsigned>(d.outputs[0].symbol));
    REQUIRE(dat.code[child+t::output_operand[0].offset-1]==64&&word(child,10)==1&&word(child,15)==170);
    REQUIRE(word(child,20)==t::success_text.symbol&&word(child,25)==12&&word(child,30)==672156);
  }
  const auto profile=t::script_profile();
  REQUIRE(profile.valid());
  const auto compiled=profile.view();
  for(std::size_t i=0;i<compiled.function_count;++i){
    const auto& f=compiled.functions[i];
    REQUIRE(f.byte_offset+f.byte_length<=dat.code.size());
    REQUIRE(bow_item_test::hash(dat.code.data()+f.byte_offset,f.byte_length)==f.body_sha256);
  }
  // Original function bodies and success text in a parser-only test image.
  bow_test::Fixture fixture(ubc::bow_recipe_catalog::recipes[0]);
  for(std::size_t i=0;i<compiled.binding_count;++i){
    const auto& b=compiled.bindings[i];
    fixture.make_symbol(b.symbol,b.name,b.elements,b.type,b.flags,static_cast<std::uint32_t>(b.value),static_cast<std::uint32_t>(b.offset),b.parent_index);
  }
  for(std::size_t i=0;i<compiled.function_count;++i){
    const auto& f=compiled.functions[i];
    fixture.make_symbol(f.symbol,f.name,f.argc,f.type,f.flags,f.byte_offset,
      f.type==7?0x60000000+f.symbol*0x100:f.return_type,f.parent_index);
    fixture.image.block(bow_test::code+f.byte_offset,f.byte_length);
    std::memcpy(fixture.image.at(bow_test::code+f.byte_offset,f.byte_length),dat.code.data()+f.byte_offset,f.byte_length);
  }
  const auto& value=dat.strings.at(t::success_text.symbol);
  const auto msg=bow_test::array_at(t::success_text.symbol);
  fixture.image.block(msg,20);fixture.image.put(bow_test::symbol(t::success_text.symbol)+0x18,msg);
  fixture.image.put(msg+8,msg+32);fixture.image.put(msg+12,static_cast<std::uint32_t>(value.size()));
  bow_item_test::text(fixture.image,msg+32,value);
  ubc::GrindCheck check;REQUIRE(t::verify_script(fixture.image.memory(),check));
  fixture.image.put(msg+12,0);REQUIRE(!t::verify_script(fixture.image.memory(),check));
  REQUIRE(value.size()==t::success_text.length);
  REQUIRE(bow_item_test::hash(value.data(),value.size())==t::success_text.digest);
}
}
int main(int argc,char** argv){
  REQUIRE(t::script_profile().valid());refund_failures();
  for(const auto& d:t::recipes){orders(d);menu_and_payment(d);for(unsigned n:{1u,2u,3u,6u})transaction(example(d),d,n);}
  if(argc==3&&std::string(argv[1])=="--local-archive")archive(argv[2]);
  else if(argc!=1)return 2;
}
