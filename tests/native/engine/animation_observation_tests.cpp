// SPDX-License-Identifier: MIT
#include "ubc/engine/animation_observation.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace {
namespace obs = ubc::animation_observation;
struct Image {
  std::array<unsigned char, 4096> data{};
  void word(unsigned addr, unsigned n) { std::memcpy(data.data() + addr, &n, 4); }
  void string(unsigned addr, unsigned ptr, const char* text) {
    word(addr + 8, ptr);
    word(addr + 12, static_cast<unsigned>(std::strlen(text)));
    std::memcpy(data.data() + ptr, text, std::strlen(text) + 1);
  }
  static bool read(void* self, unsigned addr, void* dst, std::size_t size) noexcept {
    const auto& data = static_cast<Image*>(self)->data;
    if (addr > data.size() || size > data.size() - addr)
      return false;
    std::memcpy(dst, data.data() + addr, size);
    return true;
  }
};
}
int main() {
  Image im;
  const ubc::ReadOnlyMemory m{&im, Image::read};
  unsigned failures{};
  const auto check = [&](bool value, const char* what) {
    if (!value) {
      std::fprintf(stderr, "FAIL %s\n", what);
      ++failures;
    }
  };
  const ubc::HandObjectSnapshot raw{1, 6719, 1, 312}, pan{1, 6910, 1, 416}, empty{0, -1, -1};
  check(!std::strcmp(obs::anchor_slot({empty, pan}, raw), "separate"),
        "stove prop distinct from input");
  check(!std::strcmp(obs::anchor_slot({raw, empty}, raw), "left"), "anchored left");
  check(!std::strcmp(obs::anchor_slot({empty, raw}, raw), "right"), "anchored right");
  check(!std::strcmp(obs::anchor_slot({empty, empty}, empty), "none"), "empty hands");
  check(!std::strcmp(obs::anchor_slot({{}, empty}, raw), "unknown"), "unreadable hand");
  constexpr unsigned model = 128, channel = 512, proto = 640, msg = 1200;
  im.word(model + 0x38, channel);
  im.word(channel, proto);
  im.string(proto + 0x24, 900, "T_PAN_S1_2_S0");
  check(!std::strcmp(obs::current(m, model, "PAN").name, "T_PAN_S1_2_S0"), "active animation");
  check(obs::current(m, model, "STOVE").frame == -1, "foreign scheme");
  im.word(channel + 0x1c, 1);
  check(obs::current(m, model, "PAN").frame == -1, "fading channel");
  im.word(msg, ubc::cool_destroy_dispatch_profile::message_vtable);
  im.word(msg + 0x24, 7);
  check(obs::message_tag(m, msg) == obs::tags[1], "remove message");
  im.word(msg + 0x24, 12);
  check(!obs::message_tag(m, msg), "unrelated subtype");
  im.word(msg, 1);
  im.word(msg + 0x24, 7);
  check(!obs::message_tag(m, msg), "wrong message class");
  im.word(model + 0xcc, 1700);
  im.word(model + 0xd0, 3);
  im.word(model + 0xd4, 3);
  im.word(1700, 2000);
  im.word(1704, 2200);
  im.word(1708, 2200);
  im.word(2004, 7);
  im.string(2008, 2500, "DEF_EXCHANGE_ITEM");
  im.word(2204, 0);
  im.string(2208, 2600, "DEF_REMOVE_ITEM");
  std::vector<std::pair<std::string, int>> rows;
  const auto add = [&](const char* tag, int frame) { rows.emplace_back(tag, frame); };
  check(obs::visit_tags(m, model, add) && rows == decltype(rows){{"DEF_EXCHANGE_ITEM", 7},
                                                                 {"DEF_REMOVE_ITEM", 0},
                                                                 {"DEF_REMOVE_ITEM", 0}},
        "ordered same-tick events");
  im.word(model + 0xd0, 2);
  check(!obs::visit_tags(m, model, add) && rows.size() == 3, "inconsistent event count");
  im.word(model + 0xd0, 3);
  im.word(1700, 4060);
  im.word(4060 + 8 + 12, 80);
  check(!obs::visit_tags(m, model, add), "unreadable tag");
  return failures ? 1 : 0;
}
