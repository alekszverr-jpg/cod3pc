// Command lookup/delivery and mouse mathematics adapted from SirCarls/cod3_REcomp_
// 6ae4839, game/keybinds/binds.cpp and mouse_look.cpp. MIT notice in licenses/.
// SP only; host UI never reads guest memory, and SDK retains cursor ownership.
#include "pc_input.h"
#include "pc_input_model.h"
#include "pc_input_binding.h"
#include "pc_context_input.h"
#include "pc_menu.h"
#include "menu_guest.h"
#include "menu_prompts.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/input/state_merge.h>
#include <rex/ui/window.h>
#include <rex/ui/window_listener.h>
#include <rex/ui/keybinds.h>
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>

REXCVAR_DEFINE_BOOL(cod3_native_actions, true, "cod3/Controls", "Separate on-foot combat and stance commands");
REXCVAR_DEFINE_STRING(cod3_key_reload, "R", "cod3/Controls", "Reload key");
REXCVAR_DEFINE_STRING(cod3_key_use, "F", "cod3/Controls", "Interaction key");
REXCVAR_DEFINE_STRING(cod3_key_crouch, "C", "cod3/Controls", "Toggle crouch key");
REXCVAR_DEFINE_STRING(cod3_key_prone, "Control", "cod3/Controls", "Toggle prone key");
REXCVAR_DEFINE_STRING(cod3_key_fire, "MOUSE1", "cod3/Controls", "Fire while held");
REXCVAR_DEFINE_STRING(cod3_key_aim, "MOUSE2", "cod3/Controls", "Aim down sights while held");
REXCVAR_DEFINE_STRING(cod3_key_weapnext, "MWHEELDOWN", "cod3/Controls", "Next weapon");
REXCVAR_DEFINE_STRING(cod3_key_weapprev, "MWHEELUP", "cod3/Controls", "Previous weapon");
REXCVAR_DEFINE_STRING(cod3_key_frag, "G", "cod3/Controls", "Frag grenade / throw back while held");
REXCVAR_DEFINE_STRING(cod3_key_smoke, "4", "cod3/Controls", "Smoke grenade while held");
REXCVAR_DEFINE_STRING(cod3_key_melee, "V", "cod3/Controls", "Melee attack");
REXCVAR_DEFINE_STRING(cod3_key_binoculars, "B", "cod3/Controls", "Binoculars");
REXCVAR_DEFINE_STRING(cod3_key_holdbreath, "Shift", "cod3/Controls", "Hold breath");
REXCVAR_DEFINE_BOOL(cod3_mouse_disable_aim_assist, true, "cod3/Mouse", "Disable original Sticky Aim while keyboard/mouse is active");
REXCVAR_DEFINE_BOOL(cod3_raw_mouse, true, "cod3/Mouse", "Direct raw mouse camera, without right-stick acceleration");
REXCVAR_DEFINE_DOUBLE(cod3_sensitivity, 5.0, "cod3/Mouse", "Raw mouse sensitivity (WaW scale)").range(0.01, 100.0);
REXCVAR_DEFINE_DOUBLE(cod3_zoom_sensitivity, 1.0, "cod3/Mouse", "Extra ADS sensitivity multiplier").range(0.05, 4.0);
REXCVAR_DEFINE_BOOL(cod3_invert_mouse, false, "cod3/Mouse", "Invert mouse on top of game profile setting");
REXCVAR_DECLARE(bool, mnk_mode);

namespace {
using namespace cod3::guest;
using VK = rex::ui::VirtualKey;
using cod3::input::kActionCount;
constexpr std::array<const char*, kActionCount> flags{"cod3_key_reload", "cod3_key_use", "cod3_key_crouch", "cod3_key_prone", "cod3_key_fire", "cod3_key_aim", "cod3_key_weapnext", "cod3_key_weapprev", "cod3_key_frag", "cod3_key_smoke", "cod3_key_melee", "cod3_key_binoculars", "cod3_key_holdbreath"};
constexpr std::array<const char*, kActionCount> commands{"+reload", "+activate", "togglecrouch", "toggleprone", "+attack", "+speed", "weapnext", "weapprev", "+grenadeattack", "+smokegrenade", "+melee", "+binoculars", "+holdbreath"};
constexpr std::array<const char*, kActionCount> releases{"-reload", "-activate", "", "", "-attack", "-speed", "", "", "-grenadeattack", "-smokegrenade", "-melee", "-binoculars", "-holdbreath"};
static_assert(commands.back() && flags.back() && releases.back());
bool TextAction(int a) { return a == 6 || a == 7; }
bool ButtonAction(int a) { return releases[a][0] != 0; }
struct Handler { uint32_t down = 0, up = 0; };
std::array<Handler, kActionCount> handlers;
std::array<std::atomic<bool>, kActionCount> ready{};
std::array<uint16_t, kActionCount> held_keys{}; // UI thread, mutex shared with reset
std::mutex mutex;
cod3::input::ActionEdges edges;
cod3::input::ContextActions context_actions;  // guest poll thread only
std::vector<cod3::input::ActionEdges::Event> direct_events;
std::atomic<uint32_t> injected_pad{0}, injected_user{0};
std::atomic<bool> context_injection_logged{false};
bool last_interaction = false;
uint32_t throwback_up = 0;
cod3::input::ThrowbackRelease throwback;
cod3::input::ThrowbackView throwback_view;
bool throwback_view_logged = false;
bool HoldsThrowbackX(uint8_t* base) {
  constexpr uint32_t button = 0x829C9030; // original +activatereload kbutton
  return ReadU32(base, button) == 211 || ReadU32(base, button + 4) == 211;
}
std::atomic<bool> in_play{false}, ui_blocked{true}, save_requested{false};
std::atomic<int> capture{-1};
std::atomic<float> zoom_ratio{1.0f};
int pending_capture = -1;
uint16_t pending_key = 0;
uint32_t polls = 0;
int64_t last_mouse_tick = 0;
bool raw_was_active = false, raw_logged = false;
bool Valid(uint32_t p) { return p >= 0x10000 && p < 0xC0000000 && !(p & 3); }
bool InImage(uint32_t p) { return p >= 0x82000000 && p < 0x82599000 && !(p & 3); }
uint16_t Normalize(uint16_t key) {
  if (key == uint16_t(VK::kLControl) || key == uint16_t(VK::kRControl)) return uint16_t(VK::kControl);
  if (key == uint16_t(VK::kLShift) || key == uint16_t(VK::kRShift)) return uint16_t(VK::kShift);
  return key;
}
bool Reserved(uint16_t key) {
  return !key || key == uint16_t(VK::kEscape) || key == uint16_t(VK::kReturn) ||
      key == uint16_t(VK::kTab) || key == uint16_t(VK::kSpace) ||
      (key >= uint16_t(VK::kF1) && key <= uint16_t(VK::kF24)) ||
      key == 'W' || key == 'A' || key == 'S' || key == 'D' || key == uint16_t(VK::kOem3);
}
uint32_t Find(uint8_t* base, const char* name, uint32_t type) {
  uint32_t node = ReadU32(base, 0x829EBA10);
  for (int n = 0; n < 4096 && Valid(node); ++n) {
    const auto text = ReadU32(base, node + 4);
    if (Valid(text) && ReadString(base, text, 64) == name && ReadU32(base, node + 8) == type) {
      const auto fn = ReadU32(base, node + 12);
      return InImage(fn) ? fn : 0;
    }
    node = ReadU32(base, node);
  }
  return 0;
}
bool GameAllows(uint8_t* base, bool& interaction) {
  if (ReadU32(base, 0x82A2A1C8) != 0 || ReadU8(base, 0x82A2A17D)) return false;
  const auto joystick = ReadU32(base, 0x829C295C);
  if (!Valid(joystick) || !ReadS32(base, joystick + 32)) return false;
  const auto block = ReadU32(base, 0x82A8DF4C);
  if (Valid(block) && ReadU8(base, block + 28) && ReadS32(base, block + 12) != 3) return false;
  const auto it = ReadU32(base, 0x82A59DA0);
  interaction = Valid(it) && ReadU32(base, it + 4) != 0;
  return true;
}
void CancelHeld() {
  std::lock_guard lock(mutex);
  for (int a = 0; a < kActionCount; ++a) if (held_keys[a]) { edges.Push(a, false); held_keys[a] = 0; }
}
bool Down(uint16_t key, bool repeat) {
  key = Normalize(key);
  if (capture.load() >= 0) {
    if (repeat) return true;
    const int action = capture.exchange(-1);
    if (key == uint16_t(VK::kEscape)) return true;
    if (Reserved(key)) { capture = action; return true; }
    std::lock_guard lock(mutex); pending_capture = action; pending_key = key;
    return true;
  }
  if (!in_play.load(std::memory_order_acquire) || ui_blocked.load()) return false;
  for (int a = 0; a < kActionCount; ++a) {
    const auto binding = Normalize(cod3::input::ParseBinding(rex::cvar::GetFlagByName(flags[a])));
    if (key != binding || Reserved(binding) || !ready[a].load()) continue;
    std::lock_guard lock(mutex);
    if (!repeat && !held_keys[a]) { held_keys[a] = key; edges.Push(a, true); }
    cod3::menuprompts::KeyboardUsed(); return true;
  }
  return false;
}
void Up(uint16_t key) {
  key = Normalize(key);
  if (!key) return;
  std::lock_guard lock(mutex);
  for (int a = 0; a < kActionCount; ++a) if (held_keys[a] == key) { held_keys[a] = 0; edges.Push(a, false); }
}
class Listener final : public rex::ui::WindowInputListener, public rex::ui::WindowListener {
 public:
  rex::ui::Window* window = nullptr;
  std::function<bool()> overlay;
  void OnKeyDown(rex::ui::KeyEvent& e) override {
    if (e.is_alt_pressed() || e.is_super_pressed()) return;
    cod3::menuprompts::KeyboardUsed();
    if (ui_blocked.load() && capture.load() < 0) return;
    if (Down(uint16_t(e.virtual_key()), e.prev_state())) e.set_handled(true);
  }
  void OnKeyUp(rex::ui::KeyEvent& e) override { Up(uint16_t(e.virtual_key())); }
  void OnMouseDown(rex::ui::MouseEvent& e) override {
    cod3::menuprompts::KeyboardUsed();
    // z64 runs before native menu navigation (z63) and the SDK's pad mapper.
    // Explicit capture also owns RMB, so it cannot become the menu Back action.
    if (ui_blocked.load() && capture.load() < 0) return;
    const auto key = cod3::input::MouseButtonKey(e.button());
    if (key && Down(key, false)) e.set_handled(true);
  }
  void OnMouseUp(rex::ui::MouseEvent& e) override {
    Up(cod3::input::MouseButtonKey(e.button()));
    // Always propagate releases: the SDK may hold a pre-pause fallback press.
  }
  cod3::input::WheelDetents wheel;
  void OnMouseWheel(rex::ui::MouseEvent& e) override {
    if (!e.scroll_y()) return;
    cod3::menuprompts::KeyboardUsed();
    const auto key = e.scroll_y() > 0 ? cod3::input::kWheelUp : cod3::input::kWheelDown;
    bool bound = capture.load() >= 0;
    if (!bound && in_play.load() && !ui_blocked.load())
      for (int a = 0; a < kActionCount; ++a)
        bound |= ready[a].load() && cod3::input::ParseBinding(rex::cvar::GetFlagByName(flags[a])) == key;
    if (!bound) { wheel.Reset(); return; }
    e.set_handled(true);
    const int count = wheel.Feed(e.scroll_y());
    for (int i = 0; i < std::abs(count); ++i) {
      if (Down(key, false)) Up(key);  // impulse, never a permanently held wheel
    }
  }
  void OnLostFocus(rex::ui::UISetupEvent&) override {
    wheel.Reset();
    ui_blocked = true; capture = -1; CancelHeld(); rex::input::mnk::ClearRawMouseDelta();
  }
} listener;
void Deliver(PPCContext& ctx, uint8_t* base, const std::vector<cod3::input::ActionEdges::Event>& events) {
  if (events.empty()) return;
  const auto now = Call(ctx, base).Invoke(0x82468510);
  for (auto event : events) {
    if (TextAction(event.action)) {
      if (event.down) {
        Call c(ctx, base);
        c.Invoke(0x824644D0, {c.PushString(std::string(commands[event.action]) + "\n")});
        REXLOG_INFO("COD3_WEAPON_COMMAND {}", commands[event.action]);
      }
      continue;
    }
    const auto& h = handlers[event.action];
    const auto fn = event.down ? h.down : h.up;
    if (fn) {
      const bool frag = event.action == 8;
      const bool held_x = frag && HoldsThrowbackX(base);
      if (ButtonAction(event.action)) Call(ctx, base).Invoke(fn, {239, now});
      else if (event.down) Call(ctx, base).Invoke(fn);
      if (frag && event.down) throwback.Pressed(held_x, HoldsThrowbackX(base));
      if (frag && !event.down && throwback.Released(HoldsThrowbackX(base)) && throwback_up) {
        Call(ctx, base).Invoke(throwback_up, {211, now});
        REXLOG_INFO("COD3_THROWBACK_RELEASE holder=211 handler={:08X}", throwback_up);
      }
      REXLOG_INFO("COD3_ACTION {} {} handler={:08X}", commands[event.action], event.down ? "down" : "up", fn);
    }
  }
}
void BeforePoll(uint8_t* base) {
  if (!throwback_up) throwback_up = Find(base, "-activatereload", 2);
  if (++polls == 1 || polls % 600 == 0) for (int a = 0; a < kActionCount; ++a) {
    if (ready[a].load()) continue;
    // Cgame owns these console commands. Queue them through original Cbuf_AddText.
    if (TextAction(a)) { ready[a] = true; continue; }
    handlers[a] = {Find(base, commands[a], ButtonAction(a) ? 2 : 1), ButtonAction(a) ? Find(base, releases[a], 2) : 0};
    const bool found = handlers[a].down && (!ButtonAction(a) || handlers[a].up) && (a != 8 || throwback_up);
    ready[a] = found;
    REXLOG_INFO("COD3_ACTION_LOOKUP {} ready={} down={:08X} up={:08X}", commands[a], found, handlers[a].down, handlers[a].up);
  }
  bool interaction = false;
  const bool allow = GameAllows(base, interaction) && !ui_blocked.load() && REXCVAR_GET(mnk_mode);
  in_play.store(allow && REXCVAR_GET(cod3_native_actions), std::memory_order_release);
  const bool previous_throwback_view = throwback_view.Active();
  // Do not infer an interaction end from a paused/blocked input poll.
  if (allow) throwback_view.Observe(ReadU8(base, 0x82A2A2E7) != 0, interaction);
  if (throwback_view.Active() != previous_throwback_view) {
    throwback_view_logged = false;
    REXLOG_INFO("COD3_THROWBACK_VIEW active={}", throwback_view.Active());
  }
  const bool stick_interaction = interaction && !throwback_view.Active();
  const bool raw_active = allow && !stick_interaction && REXCVAR_GET(cod3_raw_mouse);
  rex::input::mnk::SetRawMouseLook(allow && REXCVAR_GET(cod3_raw_mouse), stick_interaction);
  if (raw_active != raw_was_active) {
    REXLOG_INFO("COD3_RAW_MOUSE_MODE enabled={} interaction={}", raw_active, interaction);
    raw_was_active = raw_active;
  }
  if (!allow) rex::input::mnk::ClearRawMouseDelta();
  std::vector<cod3::input::ActionEdges::Event> events;
  { std::lock_guard lock(mutex); events = edges.Poll(in_play.load()); }
  std::array<cod3::input::PadBinding, kActionCount> mappings{};
  if (interaction && in_play.load() && (!last_interaction ||
      std::any_of(events.begin(), events.end(), [](auto e) { return e.down; }))) {
    for (int a = 0; a < kActionCount; ++a) {
      mappings[a] = cod3::input::ResolveContextBinding(a, commands[a], [&](int key) {
        const auto text = ReadU32(base, 0x82A59570 + key * 8 + 4);
        return Valid(text) ? ReadString(base, text, 64) : std::string{};
      });
      if (!last_interaction) REXLOG_INFO("COD3_CONTEXT_BIND action={} buttons={:04X} triggers={}",
          commands[a], mappings[a].buttons, mappings[a].triggers);
    }
  }
  if (interaction != last_interaction) REXLOG_INFO("COD3_INPUT_CONTEXT interaction={}", interaction);
  last_interaction = interaction;
  direct_events = context_actions.Route(events, interaction, mappings);
  const auto pad = context_actions.Pad();
  const auto input = ReadU32(base, 0x8290A910);
  uint32_t user = Valid(input) && ReadU8(base, input + 28) ? ReadU32(base, input + 24) : 0;
  injected_user.store(user < 4 ? user : 0, std::memory_order_relaxed);
  injected_pad.store(uint32_t(pad.buttons) | (uint32_t(pad.triggers) << 16), std::memory_order_release);
}
void AfterPoll(PPCContext& ctx, uint8_t* base) {
  std::vector<cod3::input::ActionEdges::Event> events;
  int action; uint16_t key;
  {
    std::lock_guard lock(mutex);
    events = std::move(direct_events);
    action = pending_capture; key = pending_key; pending_capture = -1;
  }
  Deliver(ctx, base, events);
  if (action >= 0 && action < kActionCount) {
    bool duplicate = false;
    for (int a = 0; a < kActionCount; ++a) if (a != action && Normalize(cod3::input::ParseBinding(rex::cvar::GetFlagByName(flags[a]))) == key) duplicate = true;
    if (!duplicate) {
      const auto text = cod3::input::BindingName(key);
      if (!text.empty() && rex::cvar::SetFlagByName(flags[action], text)) {
        save_requested = true; REXLOG_INFO("COD3_BIND {}={}", flags[action], text);
      }
    } else { capture = action; REXLOG_WARN("COD3_BIND rejected=duplicate action={}", action); }
  }
}
}

namespace cod3::input {
void Reset() {
  std::lock_guard lock(mutex);
  rex::input::ResetInputActivity();
  edges.Reset(); context_actions.Reset(); direct_events.clear(); listener.wheel.Reset();
  injected_pad = 0; injected_user = 0; context_injection_logged = false; last_interaction = false;
  held_keys = {}; handlers = {};
  for (auto& r : ready) r = false;
  throwback.Reset(); throwback_view.Reset(); throwback_view_logged = false; throwback_up = 0;
  polls = 0; last_mouse_tick = 0; in_play = false; ui_blocked = true;
  raw_was_active = raw_logged = false;
  capture = -1; pending_capture = -1; zoom_ratio = 1.0f;
  rex::input::mnk::SetRawMouseLook(false); rex::input::mnk::ClearRawMouseDelta();
}
void Attach(rex::ui::Window* window, size_t order, std::function<bool()> overlay) {
  listener.window = window; listener.overlay = std::move(overlay);
  if (window) { window->AddInputListener(&listener, order); window->AddListener(&listener); }
}
void Detach(rex::ui::Window* window) {
  if (window && listener.window == window) { window->RemoveInputListener(&listener); window->RemoveListener(&listener); }
  listener.window = nullptr; listener.overlay = {}; ui_blocked = true; capture = -1; CancelHeld();
  rex::input::mnk::SetRawMouseLook(false); rex::input::mnk::ClearRawMouseDelta();
}
void OnUiFrame() {
  const bool blocked = !listener.window || !listener.window->HasFocus() ||
      (listener.overlay && listener.overlay()) || cod3::pcmenu::MenuActive();
  ui_blocked = blocked;
  if (blocked && capture.load() < 0) listener.wheel.Reset();
  if (blocked) { CancelHeld(); rex::input::mnk::ClearRawMouseDelta(); }
  if (!listener.window || !listener.window->HasFocus() || (listener.overlay && listener.overlay())) capture = -1;
}
void CaptureBinding(int a) { if (a >= 0 && a < kActionCount) { CancelHeld(); capture = a; } }
void CancelCapture() { capture = -1; }
std::string BindingText(int a) {
  if (a < 0 || a >= kActionCount) return {};
  return capture.load() == a ? "..." : BindingName(ParseBinding(rex::cvar::GetFlagByName(flags[a])));
}
std::string PromptBinding(int a) {
  if (a < 0 || a >= kActionCount || !REXCVAR_GET(cod3_native_actions) || !ready[a].load()) return {};
  const auto key = ParseBinding(rex::cvar::GetFlagByName(flags[a]));
  return Reserved(key) ? std::string{} : BindingName(key);
}
bool PromptKeysActive() {
  return REXCVAR_GET(mnk_mode) && rex::input::IsKeyboardMouseActive(injected_user.load());
}
void ApplyToPad(uint8_t* base, uint32_t user, uint32_t state) {
  if (ui_blocked.load() || !in_play.load() || !REXCVAR_GET(cod3_native_actions) ||
      !Valid(state) || state > 0xBFFFFFF0) return;
  const auto pad = injected_pad.load(std::memory_order_acquire);
  if (!pad || user != injected_user.load(std::memory_order_relaxed)) return;
  MergeContextPad(base + state, {uint16_t(pad), uint8_t(pad >> 16)});
  if (!context_injection_logged.exchange(true)) REXLOG_INFO(
      "COD3_CONTEXT_PAD_APPLIED user={} buttons={:04X} triggers={} state={:08X}",
      user, uint16_t(pad), uint8_t(pad >> 16), state);
}
bool TakeSaveRequest() { return save_requested.exchange(false); }
}

REX_EXTERN(__imp__sub_82522138);
REX_HOOK_RAW(sub_82522138) {
  BeforePoll(base);
  __imp__sub_82522138(ctx, base);
  AfterPoll(ctx, base);
}
REX_EXTERN(__imp__sub_8243A198);
REX_HOOK_RAW(sub_8243A198) {
  __imp__sub_8243A198(ctx, base);
  if (ReadU32(base, 0x82A2A1C8) == 0) {
    const float base_fov = ReadF32(base, 0x82A11360);
    const float view_fov = float(ctx.f1.f64);
    zoom_ratio = std::isfinite(base_fov) && base_fov > 1 && std::isfinite(view_fov)
        ? std::clamp(view_fov / base_fov, 0.05f, 1.0f) : 1.0f;
  }
}
REX_EXTERN(__imp__sub_8250B460);
REX_HOOK_RAW(sub_8250B460) {
  float dx = 0, dy = 0;
  const bool local = ReadU32(base, 0x82A2A1C8) == 0;
  if (local) {
    rex::input::mnk::TakeRawMouseDelta(dx, dy);
    if ((dx != 0 || dy != 0) && !ui_blocked.load()) cod3::menuprompts::KeyboardUsed();
  }
  __imp__sub_8250B460(ctx, base);
  if (!local) return;
  const int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  const int64_t previous = last_mouse_tick; last_mouse_tick = now;
  if (!REXCVAR_GET(cod3_raw_mouse) || ui_blocked.load() || now - previous > 1500) return;
  if (dx == 0.0f && dy == 0.0f) return;
  auto scale = [&](uint32_t offset) {
    const int n = ReadS32(base, 0x82A8E610 + offset);
    return n < 0 || n > 1000 ? 1.0 : n == 0 ? 0.1 : n * 0.04;
  };
  const double ratio = zoom_ratio.load();
  auto [pitch, yaw] = cod3::input::MouseTurn(dx, dy, REXCVAR_GET(cod3_sensitivity),
      ratio < 0.999 ? ratio * REXCVAR_GET(cod3_zoom_sensitivity) : 1.0,
      scale(68), scale(72), REXCVAR_GET(cod3_invert_mouse) != (ReadU8(base, 0x82A8E610 + 65) != 0));
  constexpr uint32_t angles = 0x82A57900 + 5732;
  pitch += ReadF32(base, angles); yaw += ReadF32(base, angles + 4);
  if (std::isfinite(pitch) && std::isfinite(yaw)) {
    WriteF32(base, angles, pitch); WriteF32(base, angles + 4, yaw);
    if (throwback_view.Active() && !throwback_view_logged) {
      REXLOG_INFO("COD3_THROWBACK_MOUSE_APPLIED dx={} dy={}", dx, dy);
      throwback_view_logged = true;
    }
    if (!raw_logged) {
      REXLOG_INFO("COD3_RAW_MOUSE_APPLIED dx={} dy={} sensitivity={} zoom_ratio={}",
          dx, dy, REXCVAR_GET(cod3_sensitivity), ratio);
      raw_logged = true;
    }
  }
}

// Original aim update reads controls +67 inline at 8248D970. Run the full
// routine, with Sticky Aim off only for this call on the KB/M source.
REX_EXTERN(__imp__sub_8248D138);
REX_HOOK_RAW(sub_8248D138) {
  const bool disable = ReadU32(base, 0x82A2A1C8) == 0 &&
      cod3::input::DisableMouseAimAssist(REXCVAR_GET(cod3_mouse_disable_aim_assist),
          REXCVAR_GET(mnk_mode), rex::input::IsKeyboardMouseActive(injected_user.load()));
  static thread_local int previous = -1;
  const auto saved = ReadU8(base, 0x82A8E653);
  {
    cod3::input::ScopedStickyAim scoped(base[0x82A8E653], disable);
    __imp__sub_8248D138(ctx, base);
  }
  if (previous != int(disable)) {
    REXLOG_INFO("COD3_MOUSE_AIM_ASSIST suppressed={} profile_before={} profile_after={}",
        disable, saved, ReadU8(base, 0x82A8E653));
    previous = int(disable);
  }
}
