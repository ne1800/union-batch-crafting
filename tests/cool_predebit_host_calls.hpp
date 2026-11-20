// SPDX-License-Identifier: MIT
#pragma once
#include "cool_predebit_native_host.hpp"
#include "ubc/stations/station_end_contract.hpp"

// Private Win32 pages. Execute only our own call stubs. Compare
// the call sites with the pinned PE or our verified Remove
// patch, as applicable. Only the CPU generates RET addresses.
namespace bridge_test {
class CallSites {
  static constexpr std::array<std::uint32_t, 6> bases{
      {0x660000, 0x700000, 0x720000, 0x740000, 0x750000, 0x790000}};
  static constexpr std::size_t page_size = 0x10000;
  void* script_data_ = nullptr;
  static constexpr std::uint32_t info_site = ubc::selection_abi::info_effect_return - 5;
  static constexpr std::uint32_t resolve_site = ubc::selection_abi::resolve_selection_return - 5;
  static constexpr std::uint32_t script_site = ubc::batch_call::info_call_return - 5;
  static constexpr std::uint32_t destroy_site = ubc::cool_destroy_abi::destroy_return - 5;
  static constexpr std::uint32_t end_site = ubc::station_end_contract::completed_return - 6;
  static constexpr std::uint32_t remove_site = ubc::destroy_remove_callsite::address;
  static void bytes(std::uint32_t at, std::initializer_list<std::uint8_t> values) {
    std::copy(values.begin(), values.end(), reinterpret_cast<std::uint8_t*>(at));
  }
  static bool call(const PE& exe, std::uint32_t site, std::uint32_t entry, const void* hook) {
    auto code = reinterpret_cast<std::uint8_t*>(site);
    const auto destination = reinterpret_cast<std::uint32_t>(hook);
    ubc::cool_native_path::detail::jump(code, site, entry);
    code[0] = 0xe8;
    if (std::memcmp(code, exe.at(site, 5), 5) != 0)
      return false;
    ubc::cool_native_path::detail::jump(reinterpret_cast<std::uint8_t*>(entry), entry, destination);
    return true;
  }
  static void stack_args(std::uint32_t site, unsigned count) {
    // Repeating push [esp+4*N] copies the incoming arguments in reverse.
    for (unsigned i = 0; i < count; ++i)
      bytes(site - 4 * (count - i), {0xff, 0x74, 0x24, static_cast<std::uint8_t>(4 * count)});
  }

public:
  CallSites() = default;
  CallSites(const CallSites&) = delete;
  ~CallSites() {
    if (script_data_)
      VirtualFree(script_data_, 0, MEM_RELEASE);
  }
  bool reserve(unsigned char* storage, std::size_t size) {
    const auto first = reinterpret_cast<std::uintptr_t>(storage);
    const auto limit = std::uint64_t{first} + size;
    for (const auto base : bases) {
      if (base < first || std::uint64_t{base} + page_size > limit) {
        std::fprintf(stderr, "Aufrufseite %08x außerhalb des eigenen Hostspeichers\n", base);
        return false;
      }
    }
    const auto body_page = PreFixture::body_address() & ~0xffffu;
    script_data_ = VirtualAlloc(reinterpret_cast<void*>(body_page), 65536, MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);
    return reinterpret_cast<std::uint32_t>(script_data_) == body_page;
  }
  bool create(const PE& exe) {
    const auto hooks = cool_host::entries();
    if (!call(exe, resolve_site, 0x703030, reinterpret_cast<const void*>(hooks.resolve)) ||
        !call(exe, info_site, 0x703970, reinterpret_cast<const void*>(hooks.info)) ||
        !call(exe, script_site, 0x7929f0, reinterpret_cast<const void*>(&UbcBatchCallBridge)) ||
        !call(exe, destroy_site, ubc::cool_destroy_abi::destroy_entry,
              reinterpret_cast<const void*>(hooks.destroy)))
      return false;
    // GetInformation(player,npc,row): three copied stack arguments. RET 12.
    stack_args(resolve_site, 3);
    bytes(resolve_site + 5, {0xc2, 0x0c, 0});
    bytes(info_site + 5, {0xc3});
    // CallFunc(parser,symbol): the original Info() function in this host supplies the
    // symbol in ECX. The parser constant and stack cleanup match the verified INFO body.
    bytes(script_site - 6, {0x51, 0x68, 0xc0, 0x40, 0xab, 0x00});
    bytes(script_site + 5, {0x83, 0xc4, 0x08, 0xc3});
    stack_args(destroy_site, 1);
    bytes(destroy_site + 5, {0xc2, 4, 0});
    stack_args(end_site, 2);
    bytes(end_site, {0xff, 0x92, 0xd4, 0, 0, 0});
    bytes(end_site + 6, {0xc2, 8, 0});
    if (std::memcmp(exe.at(end_site, 6), reinterpret_cast<void*>(end_site), 6))
      return false;
    namespace remove = ubc::destroy_remove_callsite;
    const auto patch = cool_host::remove_patch();
    if (!patch.valid || std::memcmp(exe.at(remove_site, 6),
                                    remove::original_context.data() + remove::prefix_size, 6))
      return false;
    stack_args(remove_site, 2);
    std::memcpy(reinterpret_cast<void*>(remove_site), patch.bytes.data(), 6);
    bytes(remove_site + 6, {0xc2, 8, 0});
    for (const auto base : bases) {
      DWORD old = 0;
      auto* page = reinterpret_cast<void*>(base);
      if (!VirtualProtect(page, page_size, PAGE_EXECUTE_READ, &old) ||
          !FlushInstructionCache(GetCurrentProcess(), page, page_size))
        return false;
    }
    return true;
  }
  static void* resolve(void* manager, void* hero, int row) {
    return reinterpret_cast<cool_host::Resolve>(resolve_site - 12)(manager, nullptr, hero, hero,
                                                                   row);
  }
  static void info(void* object) { reinterpret_cast<cool_host::Info>(info_site)(object, nullptr); }
  static void* script(int symbol) {
    return reinterpret_cast<void*(__fastcall*)(int)>(script_site - 6)(symbol);
  }
  static int destroy(void* npc, void* msg) {
    return reinterpret_cast<int(__fastcall*)(void*, void*, void*)>(destroy_site - 4)(npc, nullptr,
                                                                                     msg);
  }
  static void* remove(void* inventory, void* table, int symbol, int amount) {
    return reinterpret_cast<void*(__fastcall*)(void*, void*, int, int)>(remove_site - 8)(
        inventory, table, symbol, amount);
  }
  static void end(void* station, void* npc) {
    std::array<std::uint32_t, 0xd4 / 4 + 1> table{};
    table.back() = cool_host::entries().end;
    reinterpret_cast<void(__fastcall*)(void*, void*, void*, int)>(end_site -
                                                                  8)(station, table.data(), npc, 0);
  }
};
}
