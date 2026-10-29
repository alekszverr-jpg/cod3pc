// Exercise actual SDK MnK delivery, not a copy of the accumulator logic.
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/cvar.h>
#include <cstdio>
#include <stdexcept>
void Check(bool ok) { if (!ok) throw std::runtime_error("raw mouse driver delivery failed"); }
int main() {
  using namespace rex::input;
  using namespace rex::input::mnk;
  try {
    rex::cvar::SetFlagByName("mnk_mode", "true");
    // Production config has legacy stick conversion disabled. Raw input must
    // still deliver counts and request capture in this configuration.
    rex::cvar::SetFlagByName("mnk_mouse", "false");
    MnkInputDriver driver(nullptr, 0);
    std::vector<DeviceInfo> devices; driver.EnumerateDevices(devices);
    Check(devices.size() == 1);
    SetMouseLookActive(true); SetRawMouseLook(true);
    auto move = [&](int x, int y) {
      rex::ui::MouseEvent e(nullptr, rex::ui::MouseEvent::Button::kNone, x, y);
      driver.OnMouseMove(e);
    };
    X_INPUT_STATE state{};
    move(10, 5); driver.GetDeviceState(devices[0].id, &state);
    Check(state.gamepad.thumb_rx == 0 && state.gamepad.thumb_ry == 0);
    move(30, 15); driver.GetDeviceState(devices[0].id, &state);
    float dx, dy; TakeRawMouseDelta(dx, dy);
    Check(dx == 30 && dy == 15);
    TakeRawMouseDelta(dx, dy); Check(dx == 0 && dy == 0);
    move(40, 20); driver.GetDeviceState(devices[0].id, &state);
    SetRawMouseLook(false); TakeRawMouseDelta(dx, dy); Check(dx == 0 && dy == 0);
    rex::cvar::SetFlagByName("mnk_mouse", "true");
    move(50, 25); driver.GetDeviceState(devices[0].id, &state);
    Check(state.gamepad.thumb_rx > 0 && state.gamepad.thumb_ry < 0);
    SetRawMouseLook(true);
    move(60, 30); driver.GetDeviceState(devices[0].id, &state);
    ClearRawMouseDelta(); TakeRawMouseDelta(dx, dy); Check(dx == 0 && dy == 0);
    SetMouseLookActive(false);
    move(70, 35); driver.GetDeviceState(devices[0].id, &state);
    Check(state.gamepad.thumb_rx == 0 && state.gamepad.thumb_ry == 0);
    TakeRawMouseDelta(dx, dy); Check(dx == 0 && dy == 0);
    SetRawMouseLook(false);
    // Interactions retain stick-style mouse input even with legacy flag off.
    rex::cvar::SetFlagByName("mnk_mouse", "false");
    SetMouseLookActive(true); SetRawMouseLook(true, true);
    move(80, 40); driver.GetDeviceState(devices[0].id, &state);
    Check(state.gamepad.thumb_rx > 0 && state.gamepad.thumb_ry < 0);
    TakeRawMouseDelta(dx, dy); Check(dx == 0 && dy == 0);
    SetRawMouseLook(false);
    std::puts("actual SDK raw mouse delivery passed"); return 0;
  } catch (const std::exception& e) { std::puts(e.what()); return 1; }
}
