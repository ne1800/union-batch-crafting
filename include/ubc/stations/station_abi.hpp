// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace ubc::station_abi {

// Only for the locally examined Gothic2.exe (Gothic II Addon 2.6):
// SHA-256 0655b4811e008a353ce372fde463ef00b6101f3e89be850e2f14980140447c7f.
// Layout checked against gothicuef/gothic-api, commit
// 65fb327c76f58c751468c8e7df917a86a89b9ed1, Gothic_II_Addon/API.
// The EXE locations below confirm the memory fields used here.
// These are our ABI descriptions, with no copied engine classes.
inline constexpr std::uint32_t image_base = 0x00400000;
inline constexpr std::uint32_t image_size = 0x00752000;
inline constexpr std::array<std::uint8_t, 32> executable_sha256{
    {0x06, 0x55, 0xb4, 0x81, 0x1e, 0x00, 0x8a, 0x35, 0x3c, 0xe3, 0x72,
     0xfd, 0xe4, 0x63, 0xef, 0x00, 0xb6, 0x10, 0x1f, 0x3e, 0x89, 0xbe,
     0x85, 0x0e, 0x2f, 0x14, 0x98, 0x01, 0x40, 0x44, 0x7c, 0x7f}};

// The parser is an object. Hero and instance context are pointer slots.
// IsAPlayer 0x7425A0; GetUseInstance 0x7A1680; GetDataAdr 0x7A1AB1.
inline constexpr std::uint32_t game_parser = 0x00ab40c0;
inline constexpr std::uint32_t hero_pointer = 0x00ab2684;
inline constexpr std::uint32_t instance_symbol_pointer = 0x00ab6430;
inline constexpr std::uint32_t instance_address_pointer = 0x00ab6434;

// Parser: GetSymbol 0x7938C0 / SymbolTable::GetSymbol 0x7A3EE0,
// ByteStack::GetPopPos 0x7A4B10 / Load 0x7A4EF0,
// DataStack::Pop 0x7A5070 / GetCurrentFunc 0x792F10.
inline constexpr std::uint32_t parser_symbols = 0x18;
inline constexpr std::uint32_t parser_symbol_capacity = 0x1c;
inline constexpr std::uint32_t parser_symbol_count = 0x20;
inline constexpr std::uint32_t parser_bytecode_base = 0x48;
inline constexpr std::uint32_t parser_bytecode_current = 0x4c;
inline constexpr std::uint32_t parser_bytecode_end = 0x50;
inline constexpr std::uint32_t parser_bytecode_capacity = 0x54;
inline constexpr std::uint32_t parser_data_stack = 0x58;
inline constexpr std::uint32_t parser_data_stack_count = 0x2058;
inline constexpr std::uint32_t parser_data_stack_capacity = 2048;
inline constexpr std::uint32_t parser_current_function = 0x219c;

// zSTRING has 0x14 bytes. Read names with length and trailing
// NUL; no unbounded strlen calls and no capacity assumptions.
inline constexpr std::uint32_t string_size = 0x14;
inline constexpr std::uint32_t string_data = 0x08;
inline constexpr std::uint32_t string_length = 0x0c;
inline constexpr std::uint32_t symbol_size = 0x3c;
inline constexpr std::uint32_t symbol_data = 0x18;
inline constexpr std::uint32_t symbol_offset = 0x1c;
inline constexpr std::uint32_t symbol_metadata = 0x20;
inline constexpr std::uint32_t symbol_parent = 0x38;

// NPC_HASITEMS 0x6E73AA; GetOwner 0x70C330; GetAmount 0x70C970;
// Item::GetInstance 0x711420. Do NOT call GetAmount/IsIn: both can
// create packed items and insert them into this list.
inline constexpr std::uint32_t npc_inventory = 0x668;
inline constexpr std::uint32_t inventory_owner = 0xa0;
inline constexpr std::uint32_t inventory_pack_ability = 0xa4;
inline constexpr std::uint32_t inventory_first_node = 0xb0;
inline constexpr std::uint32_t inventory_node_size = 0x0c;
inline constexpr std::uint32_t inventory_node_item = 0x04;
inline constexpr std::uint32_t inventory_node_next = 0x08;
inline constexpr std::uint32_t item_amount = 0x32c;
inline constexpr std::uint32_t item_symbol = 0x330;
inline constexpr std::uint32_t inventory_node_limit = 4096;

// GetInteractMob 0x74ACA0; GetScemeName 0x71DBE0;
// CallOnStateFunc 0x7208F0; GetState/GetStateNum 0x718C80/90;
// Constructor 0x71D128; IsInteractingWith 0x721550.
inline constexpr std::uint32_t npc_interact_mob = 0x964;
inline constexpr std::uint32_t mob_scheme_name = 0x1b8;
inline constexpr std::uint32_t mob_on_state_function = 0x1e0;
inline constexpr std::uint32_t mob_state = 0x1f4;
inline constexpr std::uint32_t mob_state_count = 0x1f8;
inline constexpr std::uint32_t mob_target_state = 0x1fc;
inline constexpr std::uint32_t mob_first_interaction = 0x18c;
inline constexpr std::uint32_t interaction_node_position = 0x00;
inline constexpr std::uint32_t interaction_node_next = 0x04;
inline constexpr std::uint32_t interaction_position_npc = 0x44;

// AIVAR is a MEMBER with absolute NPC offset 648 (0x288). GetDataAdr
// 0x7A1C96..0x7A1CA0 forms instance_adr + offset + 4*index. The runtime
// profile must still check member offset, element count, and index.
inline constexpr std::uint32_t npc_ai_variables = 0x288;
inline constexpr std::uint32_t npc_ai_variable_count = 100;

// InformationManager::Update 0x660BB0 reads these fields directly.
inline constexpr std::uint32_t information_npc = 0x20;
inline constexpr std::uint32_t information_player = 0x24;
inline constexpr std::uint32_t information_info = 0x28;
inline constexpr std::uint32_t information_waiting_end = 0x30;
inline constexpr std::uint32_t information_waiting_open = 0x38;
inline constexpr std::uint32_t information_waiting_close = 0x3c;
inline constexpr std::uint32_t information_waiting_selection = 0x40;
inline constexpr std::uint32_t information_must_open = 0x44;
inline constexpr std::uint32_t information_important_current = 0x4c;
inline constexpr std::uint32_t information_mode = 0x54;

struct HookSite {
  std::uintptr_t address;
  std::size_t patch_size;
  std::size_t guard_size;
  std::array<std::uint8_t, 32> original;
};

// Each patch block ends on an instruction boundary and contains
// neither relative jump nor relative call. Additional following
// bytes are also compared before the hook. Return: address +
// patch_size. DoStack is explicitly NOT a hook target. Ikarus changes
// its SEH operand and requires unchanged recursive native frames.
inline constexpr std::uint32_t do_stack_entry = 0x00791960;
inline constexpr std::array<std::uint8_t, 7> do_stack_disk_entry{
    {0x6a, 0xff, 0x68, 0x80, 0xa8, 0x82, 0x00}};
inline constexpr std::uint32_t information_manager = 0x00aac458;
inline constexpr std::array<HookSite, 5> hook_sites{
    {// StartInteraction: ECX=Mob, [ESP+4]=NPC; RET 4 (0x72193E).
     {0x00721580, 6, 21, {{0x64, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x6a, 0xff, 0x68, 0x34, 0x29,
                           0x82, 0x00, 0x50, 0x64, 0x89, 0x25, 0x00, 0x00, 0x00, 0x00}}},
     // EndInteraction: ECX=Mob, [ESP+4]=NPC, [ESP+8]=Flag; RET 8.
     {0x00721950, 6, 25, {{0x64, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x6a, 0xff, 0x68,
                           0x92, 0x29, 0x82, 0x00, 0x50, 0x8b, 0x44, 0x24, 0x14,
                           0x64, 0x89, 0x25, 0x00, 0x00, 0x00, 0x00}}},
     // Update: __fastcall ECX=Manager; no stack parameter, normal RET.
     {0x00660bb0,
      5,
      17,
      {{0x51, 0x53, 0x55, 0x8b, 0xd9, 0x56, 0x57, 0x83, 0xc9, 0xff, 0x33, 0xc0, 0xbf, 0x0c, 0x8c,
        0x89, 0x00}}},
     // oCInfo::Info: ECX=Info; no stack parameter; RET at 0x70398C. The
     // relative call in the following bytes is not copied into the trampoline.
     {0x00703970, 6, 29, {{0x56, 0x8b, 0xf1, 0x8b, 0x46, 0x2c, 0x50, 0x68, 0xc0, 0x40,
                           0xab, 0x00, 0xe8, 0x6f, 0xf0, 0x08, 0x00, 0x83, 0xc4, 0x08,
                           0xc7, 0x46, 0x4c, 0x01, 0x00, 0x00, 0x00, 0x5e, 0xc3}}},
     // OnChoice: __fastcall ECX=Manager, EDX=Choice; RET at 0x662B87.
     {0x006629a0, 7, 30, {{0x6a, 0xff, 0x68, 0x50, 0x17, 0x81, 0x00, 0x64, 0xa1, 0x00,
                           0x00, 0x00, 0x00, 0x50, 0x64, 0x89, 0x25, 0x00, 0x00, 0x00,
                           0x00, 0x83, 0xec, 0x34, 0x53, 0x55, 0x56, 0x8b, 0xe9, 0x57}}}}};

#if defined(_WIN32) && (defined(_M_IX86) || defined(__i386__))
static_assert(sizeof(void*) == 4, "Die Stations-ABI benoetigt Windows x86.");
using StartInteraction = void(__thiscall*)(void*, void*);
using EndInteraction = void(__thiscall*)(void*, void*, int);
// The second register argument is unused; the hook passes EDX through.
using InformationUpdate = void(__fastcall*)(void*, void*);
using Info = void(__thiscall*)(void*);
using Choice = void(__fastcall*)(void*, void*);
#endif

} // namespace ubc::station_abi
