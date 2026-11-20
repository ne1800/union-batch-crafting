// SPDX-License-Identifier: MIT
// Reuse the existing purely synthetic memory and file facts
// fixture. Optional original data stays exclusively in RAM.
#define main alchemy_flask_runtime_fixture_main
#include "alchemy_flask_runtime_tests.cpp"
#undef main
#include "ubc/stations/alchemy/alchemy_extra_execution.hpp"

namespace execution_test {
namespace e=ubc::alchemy_extra_execution;
namespace nav=ubc::alchemy_extra_navigation;
constexpr std::uint32_t actual_info=0xA8000,print_objects=0xA9000,print_data=0xAA000,arrays=0xB0000000;
std::array<std::string,4> read_prints(const char* archive_path) {
  std::ifstream file(archive_path,std::ios::binary);if(!file)std::abort();
  // The independent LocalFacts parser has already checked the directory,
  // DAT size and hash. This second bounded read also requires the same hash.
  std::vector<std::uint8_t> dat(9327865);file.seekg(123358077);file.read(reinterpret_cast<char*>(dat.data()),static_cast<std::streamsize>(dat.size()));
  if(!file || hash(dat.data(),dat.size())!=ubc::lhiver::dat_sha256)std::abort();
  std::size_t at=1;std::array<std::string,4> result;
  auto skip=[&](std::size_t n){if(at>dat.size() || n>dat.size()-at)std::abort();at+=n;};
  auto word=[&](){const auto before=at;skip(4);return u32(dat.data()+before);};
  auto line=[&](){const auto begin=at;while(at<dat.size() && dat[at]!=10)++at;if(at==dat.size())std::abort();return std::string(reinterpret_cast<const char*>(dat.data()+begin),at++-begin);};
  const auto count=word();if(count!=ubc::lhiver::symbol_count)std::abort();skip(static_cast<std::size_t>(count)*4);
  for(std::uint32_t i=0;i<count;++i) {
    const auto named=word();if(named>1)std::abort();if(named)static_cast<void>(line());
    static_cast<void>(word());const auto props=word(),elements=props&0xfff,type=(props>>12)&15,flags=(props>>16)&63;skip(20);
    if(!(flags&4)) {
      if(type==1 || type==2)skip(elements*4);
      else if(type==3)for(std::uint32_t k=0;k<elements;++k) {
        const auto text=line();for(std::size_t p=0;p<e::print_bindings.size();++p)if(i==e::print_bindings[p].symbol && !k)result[p]=text;
      }
      else if(type>=4 && type<=7)skip(4);
      else if(type>7)std::abort();
    }
    skip(4);
  }
  return result;
}
struct ExecutionFixture:Fixture {
  explicit ExecutionFixture(const cat::Descriptor& r,const LocalFacts* facts=nullptr,const std::array<std::string,4>* texts=nullptr):Fixture(r,facts) {
    install(e::recipe_profile(r),facts);
    for(const auto& binding:nav::base::flag_bindings)image.put(symbol(binding.symbol)+0x18,0);
    for(const auto& q:r.requirements) {
      const auto* b=cat::binding_for(r,static_cast<std::uint32_t>(q.symbol));if(!b)std::abort();
      if(b->elements>1) {
        const auto array=arrays+static_cast<std::uint32_t>(q.symbol)*0x80;
        if(!image.at(array,b->elements*4))image.block(array,b->elements*4);
        image.put(symbol(static_cast<std::uint32_t>(q.symbol))+0x18,array);image.put(array+4*q.element,static_cast<std::uint32_t>(q.value));
      } else image.put(symbol(static_cast<std::uint32_t>(q.symbol))+0x18,static_cast<std::uint32_t>(q.value));
    }
    for(std::size_t i=0;i<e::print_bindings.size();++i) {
      const auto& b=e::print_bindings[i];install(b);
      const auto address=print_objects+static_cast<std::uint32_t>(i)*0x40;
      image.block(address,20);image.put(symbol(b.symbol)+0x18,address);
      image.zstring(address,print_data+static_cast<std::uint32_t>(i)*0x100,texts?(*texts)[i]:"own test text");
    }
    info(r.owner);set_production(4);
  }
  void install(const ubc::lhiver::SymbolBinding& b) {
    if(b.parent_index>=0) {
      const auto parent=static_cast<std::uint32_t>(b.parent_index);
      if(!image.at(symbol(parent),0x3c))image.block(symbol(parent),0x3c);
      image.put(table+parent*4,symbol(parent));
    }
    if(!image.at(symbol(b.symbol),0x3c))image.block(symbol(b.symbol),0x3c);
    image.put(table+b.symbol*4,symbol(b.symbol));image.zstring(symbol(b.symbol),name(b.symbol),b.name);
    image.put(symbol(b.symbol)+0x20,b.elements|(b.type<<12)|(b.flags<<16));
    image.put(symbol(b.symbol)+0x38,b.parent_index<0?0:symbol(static_cast<std::uint32_t>(b.parent_index)));
    if(b.check_value)image.put(symbol(b.symbol)+0x18,static_cast<std::uint32_t>(b.value));
    if(b.check_offset)image.put(symbol(b.symbol)+0x1c,static_cast<std::uint32_t>(b.offset));
  }
  void install(const e::Profile& assembled,const LocalFacts* facts) {
    CHECK(assembled.valid());const auto p=assembled.view();
    for(std::size_t i=0;i<p.binding_count;++i)install(p.bindings[i]);
    for(std::size_t i=0;i<p.function_count;++i) {
      const auto& f=p.functions[i];install({f.name,f.symbol,f.type,f.flags,f.argc,f.parent_index});
      image.put(symbol(f.symbol)+0x18,f.byte_offset);image.put(symbol(f.symbol)+0x1c,f.type==7?0xDEAD0000:f.return_type);
      image.block(code+f.byte_offset,f.byte_length);auto* body=image.at(code+f.byte_offset,f.byte_length);
      if(facts)std::memcpy(body,facts->bytecode.data()+f.byte_offset,f.byte_length);else std::fill(body,body+f.byte_length,0x43);
    }
  }
  void info(const ubc::recipe_catalog::Owner& owner) {
    image.block(actual_info,0x5c);image.put(actual_info,ubc::info_detail::info_vtable);
    image.put(actual_info+0x1c,static_cast<std::uint32_t>(owner.npc));image.put(actual_info+0x20,static_cast<std::uint32_t>(owner.number));
    image.put(actual_info+0x24,owner.important);image.put(actual_info+0x28,owner.condition->symbol);image.put(actual_info+0x2c,owner.information->symbol);
    image.put(actual_info+0x44,owner.trade);image.put(actual_info+0x48,owner.permanent);image.put(actual_info+0x50,static_cast<std::uint32_t>(owner.instance));
  }
};
void structural() {
  for(const auto& recipe:cat::recipes) {
    const auto assembled=e::recipe_profile(recipe);CHECK(assembled.valid());
    const auto p=assembled.view();CHECK(ubc::runtime_script::detail::valid_profile(p));
    CHECK(p.function_count==7+recipe.item_constructors.count && p.binding_count<64);
    ExecutionFixture fake(recipe);CHECK(!e::verify_recipe(fake.image.memory(),recipe).valid);
    CHECK(!e::verify_entry(fake.image.memory(),recipe,actual_info).valid);
    for(const auto source:{nav::Page::root,nav::Page::health_01_choices,nav::Page::booze_info,nav::Page::special_info,nav::Page::tobacco_info}) {
      const auto plan=nav::make_plan(source,recipe.id,2);CHECK(nav::structurally_valid(plan));
      for(std::size_t i=0;i<plan.count;++i) {
        const auto profile=e::step_profile(plan,i);CHECK(profile.valid());CHECK(ubc::runtime_script::detail::valid_profile(profile.view()));
      }
      CHECK(!e::step_profile(plan,plan.count).valid());
    }
  }
  auto invalid=cat::recipes[0];invalid.callback.owner_instance++;CHECK(!e::recipe_profile(invalid).valid());
  CHECK(!e::step_profile({},0).valid());
}
void local(const LocalFacts& facts,const std::array<std::string,4>& texts) {
  for(const auto& recipe:cat::recipes) {
    ExecutionFixture f(recipe,&facts,&texts);
    const auto contract=e::verify_recipe(f.image.memory(),recipe);CHECK(contract.valid && contract.local.matches);
    if(!contract.valid)std::fprintf(stderr,"local issue %s symbol %u, string %u\n",ubc::runtime_script::issue_name(contract.local.issue),contract.local.failed_symbol,contract.failed_symbol);
    auto entry=e::verify_entry(f.image.memory(),recipe,actual_info);CHECK(entry.valid && entry.page==nav::target_page(recipe.category));
    CHECK(entry.code_address==code+recipe.callback.function->byte_offset);
    f.image.put(actual_info+0x50,0);CHECK(!e::verify_entry(f.image.memory(),recipe,actual_info).valid);f.info(recipe.owner);
    f.image.put(actual_info+0x58,1);CHECK(!e::verify_entry(f.image.memory(),recipe,actual_info).valid);f.image.put(actual_info+0x58,0);
    f.image.put(actual_info+0x20,99);CHECK(!e::verify_entry(f.image.memory(),recipe,actual_info).valid);f.info(recipe.owner);
    f.image.put(symbol(12330)+0x18,0);CHECK(!e::verify_entry(f.image.memory(),recipe,actual_info).valid);f.set_production(4);
    auto* body=f.image.at(code+recipe.callback.function->byte_offset,recipe.callback.function->byte_length);body[0]^=1;
    CHECK(!e::verify_recipe(f.image.memory(),recipe).valid);body[0]^=1;
    f.image.denied=actual_info+0x28;CHECK(!e::verify_entry(f.image.memory(),recipe,actual_info).valid);f.image.denied=0;
    const auto print=recipe.prints[0];std::size_t index=0;while(e::print_bindings[index].symbol!=print.symbol)++index;
    auto* text=f.image.at(print_data+static_cast<std::uint32_t>(index)*0x100,print.length);text[0]^=1;
    CHECK(!e::verify_recipe(f.image.memory(),recipe).valid);text[0]^=1;
    const auto path=nav::make_plan(nav::Page::mana_03_choices,recipe.id,3);
    for(std::size_t i=0;i<path.count;++i) {
      f.install(e::step_profile(path,i),&facts);CHECK(e::verify_step(f.image.memory(),path,i).matches);
    }
  }
}
} // namespace execution_test
#ifndef UBC_ALCHEMY_EXTRA_EXECUTION_TEST_EMBEDDED
int main(int argc,char** argv) {
  primitive_readers();execution_test::structural();
  if(argc==4 && std::strcmp(argv[1],"--local-facts")==0) {
    LocalFacts facts(argv[2],argv[3]);const auto strings=execution_test::read_prints(argv[3]);execution_test::local(facts,strings);
  } else if(argc!=1)return 2;
  std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
#endif
