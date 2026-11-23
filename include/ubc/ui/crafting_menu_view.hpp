// SPDX-License-Identifier: MIT
#pragma once
#include "ubc/engine/native_menu_abi.hpp"
#include "ubc/engine/read_only_memory.hpp"

namespace ubc::crafting_ui {
struct BackdropResult {
  const char* event;
  bool view_valid;
};

struct MenuView {
  const void* menu;
  ReadOnlyMemory memory;

  void* get() const noexcept {
    namespace m = native_menu;
    if (!menu)
      return nullptr;
    const auto parent = m::read<std::uint32_t>(menu, m::menu_window);
    const auto view = m::read<std::uint32_t>(menu, m::menu_inner_window);
    std::array<std::uint8_t, m::view_size> bytes;
    std::uint32_t blit = 0;
    if (!parent || !memory.copy(view, bytes.data(), bytes.size()) ||
        m::read<std::uint32_t>(bytes.data(), m::view_owner) != parent ||
        m::read<std::int32_t>(bytes.data(), m::view_id) != m::view_item ||
        m::read<std::uint32_t>(bytes.data(), m::vptr_offset) != m::menu_view_vtable ||
        !memory.copy(m::menu_view_vtable + m::view_blit_slot * sizeof(blit), &blit, sizeof(blit)) ||
        blit != m::view_blit)
      return nullptr;
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(view));
  }

  template <class Apply> BackdropResult backdrop(Apply apply) const noexcept {
    if (auto* view = get())
      return {apply(view), true};
    return {menu ? "ui_backdrop_view_mismatch" : "ui_backdrop_window_closed", false};
  }

  template <class Draw> bool submit(Draw draw) const noexcept {
    if (auto* view = get()) {
      draw(view);
      return true;
    }
    return false;
  }
};
} // namespace ubc::crafting_ui
