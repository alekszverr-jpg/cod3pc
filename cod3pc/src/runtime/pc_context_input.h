#pragma once
#include "pc_input_model.h"
#include <cstdint>
#include <string_view>

namespace cod3::input {
struct PadBinding {
  uint16_t buttons = 0;
  uint8_t triggers = 0;  // bit 0 LT, bit 1 RT
  bool Any() const { return buttons || triggers; }
};
struct PadKey { int key; PadBinding input; };
inline constexpr PadKey kPadKeys[]{
  {220,{0x0040,0}}, {219,{0x0080,0}}, {211,{0x4000,0}},
  {212,{0x1000,0}}, {213,{0x2000,0}}, {214,{0x8000,0}},
  {215,{0,2}}, {216,{0,1}}, {217,{0x0200,0}}, {218,{0x0100,0}},
  {154,{0x0001,0}}, {155,{0x0002,0}}, {156,{0x0004,0}},
  {157,{0x0008,0}}, {27,{0x0010,0}}, {9,{0x0020,0}}
};
// Match the original profile's exact command first, then layout equivalents.
// The getter reads the current game's bindings; no fixed RT/X layout is assumed.
template <class Getter>
PadBinding ResolveContextBinding(int action, std::string_view command, Getter binding) {
  if (action < 0 || action >= kActionCount || command.empty()) return {};
  for (auto p : kPadKeys) if (binding(p.key) == command) return p.input;
  constexpr const char* aliases[kActionCount][4]{
    {"+activatereload"},
    {"+activatereload", "+activatemelee", "+activatemoveup"},
    {"+stance", "lowerstance", "+movedown"},
    {"+stance", "goprone", "+prone"},
    {}, {"toggle cl_run"}, {}, {"weapnext"}, {}, {}, {"+activatemelee"}
  };
  for (auto alias : aliases[action]) {
    if (!alias) break;
    for (auto p : kPadKeys) if (binding(p.key) == alias) return p.input;
  }
  return {};
}
// X_INPUT_STATE bytes are big endian; preserve packet number, real pad buttons
// and all sticks. A held host trigger raises pressure without removing hardware.
inline void MergeContextPad(uint8_t* state, PadBinding pad) {
  state[4] |= uint8_t(pad.buttons >> 8);
  state[5] |= uint8_t(pad.buttons);
  if (pad.triggers & 1) state[6] = 255;
  if (pad.triggers & 2) state[7] = 255;
}
// Each press keeps its delivery route until release, even when mounting or
// leaving an interaction. The edge queue preserves a short click for one poll.
class ContextActions {
 public:
  std::vector<ActionEdges::Event> Route(const std::vector<ActionEdges::Event>& events,
      bool interaction, const std::array<PadBinding, kActionCount>& bindings) {
    std::vector<ActionEdges::Event> direct;
    for (auto e : events) {
      if (e.action < 0 || e.action >= kActionCount) continue;
      auto& slot = slots_[e.action];
      if (e.down) {
        if (slot.mode != Mode::None) continue;
        if (interaction) {
          if (bindings[e.action].Any()) slot = {Mode::Pad, bindings[e.action]};
        } else { slot.mode = Mode::Direct; direct.push_back(e); }
      } else {
        if (slot.mode == Mode::Direct) direct.push_back(e);
        slot = {};
      }
    }
    return direct;
  }
  PadBinding Pad() const {
    PadBinding result;
    for (const auto& s : slots_) if (s.mode == Mode::Pad) {
      result.buttons |= s.pad.buttons; result.triggers |= s.pad.triggers;
    }
    return result;
  }
  void Reset() { slots_ = {}; }
 private:
  enum class Mode { None, Direct, Pad };
  struct Slot { Mode mode = Mode::None; PadBinding pad; };
  std::array<Slot, kActionCount> slots_{};
};
}
