// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include "ubc/stations/bow/bow_menu_trace.hpp"
#include "ubc/stations/bow/bow_recipe_navigation.hpp"
#include "ubc/stations/bow/crafting_bow_projection.hpp"

namespace ubc::bow_menu {
namespace cat=bow_recipe_catalog;
namespace rt=bow_recipe_runtime;
namespace rs=runtime_script;
namespace nav=bow_recipe_navigation;

inline void digest_text(const lhiver::Digest& digest,char (&out)[65]) noexcept {
  for(std::size_t i=0;i<digest.size();++i)std::snprintf(out+2*i,3,"%02x",static_cast<unsigned>(digest[i]));
}
inline void name_text(const char* source,std::size_t length,char (&out)[80]) noexcept {
  std::size_t n=0;
  for(std::size_t i=0;i<length&&n+3<sizeof(out);++i){
    const auto ch=static_cast<unsigned char>(source[i]);
    if(ch>=32&&ch<=126&&ch!='"'&&ch!='\\'&&ch!='~')out[n++]=source[i];
    else {std::snprintf(out+n,4,"~%02X",static_cast<unsigned>(ch));n+=3;}
  }
  out[n]=0;
}
inline void contract_detail(BowMenuTrace& trace,const ReadOnlyMemory& memory,std::uint32_t parser,
                            const rt::ProfileAssembly& assembly,rs::RuntimeContract result,int id) noexcept {
  if(!result.failed_symbol||!assembly.valid())return;
  const auto profile=assembly.profile();
  rs::detail::Layout layout{};rs::detail::Symbol actual{};
  if(rs::detail::read_layout(memory,parser,profile,layout)!=rs::Issue::none||
     rs::detail::lookup(memory,layout,result.failed_symbol,actual)!=rs::Issue::none)return;
  const int symbol=static_cast<int>(result.failed_symbol);
  const lhiver::Function* fn=nullptr;
  for(std::size_t i=0;i<profile.function_count;++i)
    if(profile.functions[i].symbol==result.failed_symbol){fn=profile.functions+i;break;}
  const auto* binding=assembly.binding(result.failed_symbol);
  if(!fn&&!binding)return;
  if(result.issue==rs::Issue::function_hash&&fn){
    lhiver::Digest digest{};
    char expected[65]{},observed[65]{};
    digest_text(fn->body_sha256,expected);
    const auto address=std::uint64_t{layout.code}+fn->byte_offset;
    if(rs::detail::range(address,fn->byte_length)&&
       rs::detail::hash_range(memory,static_cast<std::uint32_t>(address),fn->byte_length,digest)){
      digest_text(digest,observed);
      trace.check("profile.body_sha",expected,observed,digest==fn->body_sha256,id,symbol);
    }else trace.check("profile.body_sha",expected,"unlesbar",false,id,symbol);
  }else if(result.issue==rs::Issue::symbol_signature){
    trace.equal("profile.type",fn?fn->type:binding->type,actual.type,id,symbol);
    trace.equal("profile.flags",fn?fn->flags:binding->flags,actual.flags,id,symbol);
    trace.equal("profile.elements",fn?fn->argc:binding->elements,actual.elements,id,symbol);
    if(fn&&fn->type!=7)trace.equal("profile.return_type",fn->return_type,actual.offset,id,symbol);
    if(binding&&binding->check_offset)trace.equal("profile.member_offset",binding->offset,actual.offset,id,symbol);
  }else if(result.issue==rs::Issue::symbol_entry&&fn){
    trace.equal("profile.byte_offset",fn->byte_offset,actual.data,id,symbol);
  }else if(result.issue==rs::Issue::symbol_value&&binding){
    trace.equal("profile.value",binding->value,static_cast<std::int32_t>(actual.data),id,symbol);
  }else if(result.issue==rs::Issue::symbol_parent){
    const auto parent=fn?fn->parent_index:binding->parent_index;
    rs::detail::Symbol expected{};
    if(parent<0||rs::detail::lookup(memory,layout,static_cast<std::uint32_t>(parent),expected)==rs::Issue::none)
      trace.equal("profile.parent",expected.address,actual.parent,id,symbol);
  }else if(result.issue==rs::Issue::symbol_name||result.issue==rs::Issue::name_unreadable){
    const char* expected=fn?fn->name:binding->name;
    char wanted[80]{},observed[80]{},bytes[rs::detail::max_name+1]{};
    name_text(expected,std::strlen(expected),wanted);
    trace.equal("profile.name_length",static_cast<std::int64_t>(std::strlen(expected)),actual.name_length,id,symbol);
    if(actual.name_length<=rs::detail::max_name&&memory.copy(actual.name_data,bytes,actual.name_length+1)){
      name_text(bytes,actual.name_length,observed);
      trace.check("profile.name",wanted,observed,!std::strcmp(wanted,observed),id,symbol);
      trace.equal("profile.name_nul",0,static_cast<unsigned char>(bytes[actual.name_length]),id,symbol);
    }else trace.check("profile.name",wanted,"unlesbar",false,id,symbol);
  }
}

// In the normal path the page check ends at the first invalid contract. In
// measurement mode the same comparison runs over all 20 profiles up to M_BOWS.
inline nav::Page page(const ReadOnlyMemory& memory,std::uint32_t parser,int mode,BowMenuTrace* trace) noexcept {
  const bool full=trace&&trace->full;
  bool accepted=mode==1;
  if(trace)trace->equal("dialog.mode",1,mode);
  if(!accepted&&!full)return nav::Page::unknown;
  for(const auto& recipe:cat::recipes){
    const auto assembly=rt::assemble(recipe);
    const auto result=rt::verify(memory,parser,assembly);
    if(trace)trace->check("page.recipe_contract","ok",rs::issue_name(result.issue),result.matches,
                          static_cast<int>(recipe.id),static_cast<int>(result.failed_symbol));
    if(!result.matches){
      if(trace)contract_detail(*trace,memory,parser,assembly,result,static_cast<int>(recipe.id));
      accepted=false;
      if(!full)return nav::Page::unknown;
    }
  }
  const auto assembly=rt::assemble(cat::recipes[0]);
  const auto first=rt::context(memory,parser,assembly);
  const auto last=rt::context(memory,parser,assembly);
  const bool stable=rt::same_context_evidence(first,last);
  if(trace){
    trace->check("page.context_before","ok",rs::issue_name(first.issue),first.read_valid,0,static_cast<int>(first.failed_symbol));
    trace->check("page.context_after","ok",rs::issue_name(last.issue),last.read_valid,0,static_cast<int>(last.failed_symbol));
    // context() yields no valid values on a read error. In measurement mode
    // each independently readable scalar remains visible as its actual value.
    if(full&&!first.read_valid){
      const auto production=rs::symbol_int(memory,parser,12330);
      const auto bows=rs::symbol_int(memory,parser,20862);
      if(production.valid)trace->equal("page.production",16,production.value,0,12330);
      else trace->check("page.production","16",rs::issue_name(production.issue),false,0,12330);
      if(bows.valid)trace->value("page.m_bows","0..1",bows.value,bows.value==0||bows.value==1,0,20862);
      else trace->check("page.m_bows","0..1",rs::issue_name(bows.issue),false,0,20862);
    }else if(first.read_valid){
      trace->equal("page.production",16,first.production,0,12330);
      trace->value("page.m_bows","0..1",first.bows,first.bows==0||first.bows==1,0,20862);
    }
    trace->equal("page.stable",1,stable);
  }
  if(!accepted||!first.station_matches()||!stable)return nav::Page::unknown;
  return nav::classify_page({true,first.production,first.bows,cat::Invocation::info});
}

inline void catalogue(BowMenuTrace& trace,const BowRecipeCatalogueCapture& source,
                      const CraftingSnapshot& snapshot) noexcept {
  const auto& d=source.diagnostic;
  trace.equal("catalogue.capture",0,static_cast<int>(d.issue));
  trace.equal("catalogue.projection",0,static_cast<int>(snapshot.issue));
  const bool read=source.recipes.front().evaluated;
  if(read){
    trace.equal("catalogue.inventory",0,static_cast<int>(d.inventory_issue));
    trace.equal("catalogue.hands",0,static_cast<int>(d.hand_issue));
  }else{
    trace.pending("catalogue.inventory","0","ungelesen");
    trace.pending("catalogue.hands","0","ungelesen");
  }
  if(!trace.full)return;
  trace.equal("catalogue.inventory_stable",1,d.inventory_unchanged);
  trace.equal("catalogue.hands_stable",1,d.hands_unchanged);
  trace.equal("catalogue.knowledge_stable",1,d.knowledge_unchanged);
  trace.equal("catalogue.actor_stable",1,d.actor_unchanged);
  trace.equal("catalogue.context_stable",1,d.context_unchanged);
  for(std::size_t i=0;i<cat::recipes.size();++i){
    const auto& recipe=cat::recipes[i];
    const auto id=static_cast<int>(recipe.id);
    const auto& state=source.recipes[i];
    const auto& profile=d.profiles[i];
    const auto& projected=snapshot.recipes[crafting_catalog::bow_begin+i];
    trace.equal("recipe.evaluated",1,state.evaluated,id);
    if(state.evaluated){
      const bool text_failed=profile.string_issue!=alchemy_flask_runtime::Issue::none;
      trace.check("recipe.contract","ok",text_failed?"Drucktext":rs::issue_name(profile.local.issue),profile.valid,
                  id,static_cast<int>(text_failed?profile.failed_symbol:profile.local.failed_symbol));
      if(profile.valid||text_failed)trace.equal("recipe.text",0,static_cast<int>(profile.string_issue),id,static_cast<int>(profile.failed_symbol));
      else trace.pending("recipe.text","0","Profil abgelehnt",id);
      trace.equal("recipe.learned",1,projected.known,id,10484);
      trace.equal("recipe.actor",1,projected.actor_matches,id);
      trace.equal("recipe.context",1,projected.context_matches,id);
      trace.equal("recipe.material_issue",0,static_cast<int>(state.material.issue),id);
      trace.equal("recipe.material_verified",1,projected.material_verified,id);
    }else{
      trace.pending("recipe.contract","ok","ungelesen",id);
      trace.pending("recipe.text","0","ungelesen",id);
      trace.pending("recipe.learned","1","ungelesen",id,10484);
    }
    for(std::size_t j=0;j<recipe.targets.count;++j){
      const auto symbol=recipe.targets[j].symbol;
      const auto stock=state.rows[j].available;
      const auto required=(std::max)(cat::units(recipe.minima,symbol),cat::units(recipe.costs,symbol));
      char wanted[32];
      std::snprintf(wanted,sizeof(wanted),">=%u",required);
      if(state.rows[j].available_known)trace.value("recipe.stock",wanted,stock,stock>=required,id,symbol);
      else trace.pending("recipe.stock",wanted,"ungelesen",id,symbol);
    }
    if(state.material.known())trace.value("recipe.maximum",">=1",projected.maximum,projected.maximum>=1,id);
    else trace.pending("recipe.maximum",">=1","unbekannt",id);
  }
}
} // namespace ubc::bow_menu
