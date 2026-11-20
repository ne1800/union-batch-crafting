// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/spirit_recipe_runtime.hpp"
#include "ubc/alchemy_extra_execution.hpp"

namespace ubc::spirit_execution {
namespace cat=spirit_recipe_catalog;namespace rt=spirit_recipe_runtime;
struct Entry {bool valid=false;rt::Verification contract{};rt::Knowledge knowledge{};
  rt::BranchEvidence branch{};rt::ContextEvidence context{};std::uint32_t code_address=0;};
inline Entry verify_entry(const ReadOnlyMemory& memory,std::uint32_t actual_info,std::int32_t branch) noexcept {
  Entry out;const auto& recipe=cat::recipes[0];const auto profile=rt::assemble(recipe);
  out.contract=rt::verify_recipe(memory,lhiver::game_parser,profile);if(!out.contract.valid||!actual_info)return out;
  rt::BranchEvidence first_branch{};rt::ContextEvidence first_context{};rt::Knowledge first_knowledge{};
  for(unsigned pass=0;pass<2;++pass){
    const auto info=read_selection_info(memory,actual_info);
    if(!alchemy_extra_execution::info_matches(info,recipe.owner)||info.first_choice)return out;
    // Check also the other original categories besides SPECIALSTART.
    if(alchemy_extra_execution::read_page(memory)!=alchemy_extra_navigation::Page::special_info)return out;
    out.knowledge=rt::knowledge(memory,lhiver::game_parser,profile);out.context=rt::context(memory,lhiver::game_parser,profile);
    out.branch=rt::branch(memory,lhiver::game_parser,profile);
    if(!out.knowledge.known||!out.context.owner_matches()||!out.branch.read_valid||out.branch.value!=branch)return out;
    if(pass&&(!rt::same_integer_evidence(first_knowledge,out.knowledge)||!rt::same_branch_evidence(first_branch,out.branch)||
       !rt::same_context_evidence(first_context,out.context)))return out;
    first_knowledge=out.knowledge;first_branch=out.branch;first_context=out.context;
  }
  runtime_script::detail::Layout layout{};
  if(runtime_script::detail::read_layout(memory,lhiver::game_parser,profile.profile(),layout)!=runtime_script::Issue::none||
     recipe.callback->byte_offset>UINT32_MAX-layout.code)return out;
  out.code_address=layout.code+recipe.callback->byte_offset;out.valid=true;return out;
}
} // namespace ubc::spirit_execution
