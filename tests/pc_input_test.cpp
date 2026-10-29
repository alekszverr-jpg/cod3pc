#include "../cod3pc/src/runtime/pc_input_model.h"
#include "../cod3pc/src/runtime/pc_input_binding.h"
#include "../cod3pc/src/runtime/pc_context_input.h"
#include "../cod3pc/src/runtime/action_prompt.h"
#include <cstdio>
#include <rex/input/state_merge.h>
#include <stdexcept>
#include <limits>
void Check(bool ok) { if (!ok) throw std::runtime_error("PC input test failed"); }
int main() {
  using namespace cod3::input;
  try {
    Check(ParseActionPrompt("[USE]").action == 1);
    Check(ParseActionPrompt("[HOLD_BINOCS]").action == 11);
    Check(ParseActionPrompt("[HOLD_BINOCS]").form == 3);
    Check(ParseActionPrompt("[VERB_ATTACK]").action == 4);
    Check(ParseActionPrompt("[PRESS_RELOAD]").form == 1);
    Check(ParseActionPrompt("[USE_LEFT_STICK]").action == -1);
    Check(ParseActionPrompt("[SPRINT]").action == -1);
    Check(ParseActionPrompt("USE").action == -1);
    Check(ParseActionPrompt("[HOLD_]").action == -1);
    Check(FormatActionPrompt(ParseActionPrompt("[USE]"),"MOUSE4","Press","Hold") == "[MOUSE4]");
    Check(FormatActionPrompt(ParseActionPrompt("[HOLD_BINOCS]"),"T","Press","Hold") == "Hold [T]");
    Check(FormatActionPrompt(ParseActionPrompt("[PRESS_ATTACK]"),"MOUSE1","Press","Hold") == "Press [MOUSE1]");
    Check(FormatActionPrompt(ParseActionPrompt("[USE]"),"","Press","Hold").empty());
    Check(FormatActionPrompt(ParseActionPrompt("[HOLD_BINOCS]"),"B","Press","").empty());
    // Scoped original-profile override must survive exceptions and preserve
    // the saved controller choice; device detection excludes synthetic pads.
    Check(DisableMouseAimAssist(true,true,true));
    Check(!DisableMouseAimAssist(true,true,false));
    Check(!DisableMouseAimAssist(true,false,true));
    Check(!DisableMouseAimAssist(false,true,true));
    uint8_t sticky = 1;
    { ScopedStickyAim scoped(sticky,true); Check(sticky == 0); }
    Check(sticky == 1);
    try { ScopedStickyAim scoped(sticky,true); throw 42; } catch (int) {}
    Check(sticky == 1);
    { ScopedStickyAim scoped(sticky,false); Check(sticky == 1); }
    sticky = 0; { ScopedStickyAim scoped(sticky,true); } Check(sticky == 0);
    using namespace rex::input;
    ResetInputActivity(); Check(IsKeyboardMouseActive());
    X_INPUT_GAMEPAD physical{}; physical.buttons = 0x1000;
    const auto pad_id = DeviceId(42);
    ObserveInputActivity(0, pad_id, true, physical); Check(IsKeyboardMouseActive());
    ObserveInputActivity(0, pad_id, false, physical); Check(!IsKeyboardMouseActive());
    KeyboardMouseUsed();
    ObserveInputActivity(0, pad_id, false, physical); Check(IsKeyboardMouseActive());
    physical.buttons = 0x2000;
    ObserveInputActivity(0, pad_id, false, physical); Check(!IsKeyboardMouseActive());
    KeyboardMouseUsed(); physical = {}; physical.thumb_rx = 2000;
    ObserveInputActivity(0,pad_id,false,physical); Check(IsKeyboardMouseActive());
    physical.thumb_rx = 16000;
    ObserveInputActivity(0,pad_id,false,physical); Check(!IsKeyboardMouseActive());
    KeyboardMouseUsed(); physical.thumb_rx = 16001;
    ObserveInputActivity(0,pad_id,false,physical); Check(IsKeyboardMouseActive());
    physical.thumb_rx = 16512;
    ObserveInputActivity(0,pad_id,false,physical); Check(!IsKeyboardMouseActive());
    KeyboardMouseUsed(); physical = {}; physical.right_trigger = 100;
    ObserveInputActivity(1,pad_id,false,physical);
    Check(IsKeyboardMouseActive(0) && !IsKeyboardMouseActive(1));
    ResetInputActivity(); Check(IsKeyboardMouseActive(1));
    // Paged native binding rows must address each action exactly once.
    int action = 0;
    for (int i=0;i<kActionCount;++i) {
      Check(action == i);
      const int first = BindingPageFirst(action), row = action - first;
      Check(row >= 0 && row < BindingPageRows(first));
      action = StepBinding(first,row,true);
    }
    Check(action == 0 && StepBinding(0,0,false) == kActionCount-1);
    Check(BindingPageFirst(6) == 6 && BindingPageRows(6) == 6);
    Check(BindingPageFirst(12) == 12 && BindingPageRows(12) == 1);
    Check(StepBinding(0,5,true) == 6 && StepBinding(6,0,false) == 5);
    action = 0;
    for (int i=0;i<kActionCount;++i) {
      action = StepBinding(BindingPageFirst(action), action % kBindingRows, false);
      Check(action == (kActionCount - i - 1) % kActionCount);
    }
    Check(action == 0);
    ThrowbackView view;
    view.Observe(false,true); Check(!view.Active()); // ordinary MG42 interaction
    view.Observe(false,false);
    view.Observe(true,false); Check(!view.Active()); // caught, animation not started
    view.Observe(true,true); Check(view.Active());
    view.Observe(false,true); Check(view.Active()); // G released during animation
    // Pauses do not advance Observe: retaining the running interaction is deliberate.
    Check(view.Active());
    view.Observe(false,false); Check(!view.Active());
    view.Observe(false,true); Check(!view.Active()); // next unrelated interaction
    view.Observe(true,true); view.Reset(); Check(!view.Active());
    ThrowbackRelease guard;
    guard.Pressed(false,true); Check(guard.Released(true));
    Check(!guard.Released(true));
    guard.Pressed(true,true); Check(!guard.Released(true));
    guard.Pressed(false,false); Check(!guard.Released(true));
    guard.Pressed(false,true); Check(!guard.Released(false));
    guard.Pressed(false,true); guard.Reset(); Check(!guard.Released(true));
    // Native MouseEvents, saved tokens and legacy SDK aliases must identify
    // exactly the same physical input (also for duplicate-binding detection).
    using B = rex::ui::MouseEvent::Button;
    constexpr B buttons[]{B::kLeft, B::kRight, B::kMiddle, B::kX1, B::kX2};
    for (int i = 0; i < 5; ++i) {
      const auto key = MouseButtonKey(buttons[i]);
      Check(key != 0);
      Check(BindingName(key) == "MOUSE" + std::to_string(i + 1));
      Check(ParseBinding(BindingName(key)) == key);
      for (int j = 0; j < i; ++j) Check(key != MouseButtonKey(buttons[j]));
    }
    Check(ParseBinding("LMB") == ParseBinding("MOUSE1"));
    Check(ParseBinding("RMB") == ParseBinding("MOUSE2"));
    Check(ParseBinding("MMB") == ParseBinding("MOUSE3"));
    Check(ParseBinding("R") == 'R');
    Check(ParseBinding("MOUSE6") == 0 && MouseButtonKey(B::kNone) == 0);
    Check(ParseBinding("MWHEELUP") == kWheelUp && ParseBinding("MWHEELDOWN") == kWheelDown);
    Check(BindingName(kWheelUp) == "MWHEELUP" && BindingName(kWheelDown) == "MWHEELDOWN");
    WheelDetents wheel;
    Check(wheel.Feed(30) == 0 && wheel.Feed(30) == 0 && wheel.Feed(60) == 1);
    Check(wheel.Feed(-360) == -3);
    Check(wheel.Feed(100) == 0 && wheel.Feed(-100) == 0);
    Check(wheel.Feed(119) == 0); wheel.Reset(); Check(wheel.Feed(1) == 0);
    wheel.Reset(); Check(wheel.Feed(std::numeric_limits<int>::max()) == 16);
    wheel.Reset(); Check(wheel.Feed(std::numeric_limits<int>::min()) == -16);
    ActionEdges e;
    e.Push(0, true); e.Push(0, false);
    auto out = e.Poll(true);
    Check(out.size() == 1 && out[0].down);
    out = e.Poll(true); Check(out.size() == 1 && !out[0].down);
    e.Push(0, true); e.Push(0, true); e.Push(1, true);
    Check(e.Poll(true).size() == 2);
    out = e.Poll(false); Check(out.size() == 2 && !out[0].down && !out[1].down);
    Check(e.Poll(true).empty());
    e.Push(2, true); e.Poll(true);
    for (int i = 0; i < 65; ++i) e.Push(0, (i & 1) == 0);
    out = e.Poll(true); Check(out.size() == 1 && out[0].action == 2 && !out[0].down);
    e.Reset(); Check(e.Poll(true).empty());
    // Fire and ADS are independent held commands, not stance-style taps.
    e.Push(4, true); e.Push(5, true);
    out = e.Poll(true); Check(out.size() == 2 && out[0].down && out[1].down);
    Check(e.Poll(true).empty());
    e.Push(4, false);
    out = e.Poll(true); Check(out.size() == 1 && out[0].action == 4 && !out[0].down);
    out = e.Poll(false); Check(out.size() == 1 && out[0].action == 5 && !out[0].down);
    for (int a : {4, 5}) {
      e.Push(a, true); e.Push(a, false);
      out = e.Poll(true); Check(out.size() == 1 && out[0].action == a && out[0].down);
      out = e.Poll(true); Check(out.size() == 1 && out[0].action == a && !out[0].down);
    }
    e.Push(kActionCount, true); Check(e.Poll(true).empty());
    // Resolve the active game's layout, preferring exact commands over aliases.
    auto layout = [](int key) -> std::string_view {
      if (key == 217) return "+attack";  // alternate fire layout: RB, not RT
      if (key == 211) return "+activatereload";
      if (key == 216) return "toggle cl_run";
      return {};
    };
    Check(ResolveContextBinding(4, "+attack", layout).buttons == 0x0200);
    Check(ResolveContextBinding(0, "+reload", layout).buttons == 0x4000);
    Check(ResolveContextBinding(1, "+activate", layout).buttons == 0x4000);
    Check(ResolveContextBinding(5, "+speed", layout).triggers == 1);
    Check(!ResolveContextBinding(3, "toggleprone", layout).Any());
    Check(!ResolveContextBinding(-1, "+attack", layout).Any());
    std::array<PadBinding, kActionCount> mappings{};
    mappings[4] = {0,2}; mappings[5] = {0,1};
    mappings[0] = mappings[1] = {0x4000,0};
    ContextActions routes;
    e.Reset(); e.Push(4, true); e.Push(5, true);
    Check(routes.Route(e.Poll(true), true, mappings).empty());
    Check(routes.Pad().triggers == 3);
    // Leaving the turret while held keeps the old route; release clears it.
    Check(routes.Route({}, false, {}).empty() && routes.Pad().triggers == 3);
    e.Push(4, false);
    Check(routes.Route(e.Poll(true), false, {}).empty());
    Check(routes.Pad().triggers == 1);
    Check(routes.Route(e.Poll(false), true, mappings).empty() && !routes.Pad().Any());
    e.Push(4, true);
    Check(routes.Route(e.Poll(true), false, {}).front().down);
    e.Push(4, false);
    out = routes.Route(e.Poll(true), true, mappings);
    Check(out.size() == 1 && !out[0].down && !routes.Pad().Any());
    // Shared X button stays held until BOTH reload and use have released.
    e.Push(0,true); e.Push(1,true);
    routes.Route(e.Poll(true), true, mappings);
    e.Push(0,false); routes.Route(e.Poll(true), true, mappings);
    Check(routes.Pad().buttons == 0x4000);
    routes.Route(e.Poll(false), true, mappings); Check(!routes.Pad().Any());
    // A sub-poll mouse click must reach the pad for one complete poll.
    e.Push(4,true); e.Push(4,false);
    routes.Route(e.Poll(true), true, mappings); Check(routes.Pad().triggers == 2);
    routes.Route(e.Poll(true), true, mappings); Check(!routes.Pad().Any());
    std::array<uint8_t,16> pad_state{};
    for (int i=0;i<16;++i) pad_state[i] = uint8_t(i + 1);
    const auto original_state = pad_state;
    MergeContextPad(pad_state.data(), {0x4001,2});
    Check(pad_state[4] == uint8_t(original_state[4] | 0x40));
    Check(pad_state[5] == uint8_t(original_state[5] | 1));
    Check(pad_state[6] == original_state[6] && pad_state[7] == 255);
    for (int i=0;i<16;++i) if(i<4 || i>=8) Check(pad_state[i] == original_state[i]);
    routes.Reset(); Check(!routes.Pad().Any());
    // Several detents keep their direction and produce one down per detent.
    e.Reset();
    for (int i=0;i<3;++i) { e.Push(6,true); e.Push(6,false); }
    int weapon_downs = 0;
    for (int poll=0;poll<5;++poll)
      for (auto edge : e.Poll(true)) if (edge.action == 6 && edge.down) ++weapon_downs;
    Check(weapon_downs == 3 && e.Poll(true).empty());
    e.Push(7,true); e.Push(7,false);
    Check(e.Poll(false).empty() && e.Poll(true).empty());
    // New grenade/melee holders release on block, including the last action.
    e.Reset(); e.Push(8,true); e.Push(9,true); e.Push(10,true);
    Check(e.Poll(true).size() == 3);
    out = e.Poll(false); Check(out.size() == 3);
    for (auto edge : out) Check(!edge.down && edge.action >= 8 && edge.action <= 10);
    e.Reset(); e.Push(11,true); e.Push(12,true);
    Check(e.Poll(true).size() == 2);
    e.Push(11,false);
    out = e.Poll(true); Check(out.size() == 1 && out[0].action == 11 && !out[0].down);
    out = e.Poll(false); Check(out.size() == 1 && out[0].action == 12 && !out[0].down);
    // Same physical movement, differently partitioned events/frames, same turn.
    auto total = MouseTurn(400, 200, 5, 1, 1, 1, false);
    float pitch = 0, yaw = 0;
    for (int i = 0; i < 1000; ++i) {
      auto v = MouseTurn(0.4f, 0.2f, 5, 1, 1, 1, false);
      pitch += v.first; yaw += v.second;
    }
    Check(std::abs(pitch - total.first) < 0.001f && std::abs(yaw - total.second) < 0.001f);
    auto ads = MouseTurn(400, 200, 5, 0.5, 1, 1, true);
    Check(std::abs(ads.first + total.first * 0.5f) < 0.001f);
    Check(std::abs(ads.second - total.second * 0.5f) < 0.001f);
    auto bad = MouseTurn(std::numeric_limits<float>::quiet_NaN(), 0, 5, 1, 1, 1, false);
    Check(bad.first == 0 && bad.second == 0);
    std::puts("PC action edges and raw mouse math passed"); return 0;
  } catch (const std::exception& e) { std::puts(e.what()); return 1; }
}
