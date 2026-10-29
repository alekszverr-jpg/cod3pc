#pragma once
#include <rex/ui/keybinds.h>
#include <rex/ui/ui_event.h>
#include <string>
#include <string_view>

namespace cod3::input {
inline constexpr uint16_t kWheelUp = 0x100, kWheelDown = 0x101;
// Stable config tokens shared by capture, lookup and display. SDK key names
// remain valid aliases, including old LMB/RMB/MMB configs.
inline uint16_t MouseButtonKey(rex::ui::MouseEvent::Button button) {
  using B = rex::ui::MouseEvent::Button;
  using K = rex::ui::VirtualKey;
  switch (button) {
    case B::kLeft: return uint16_t(K::kLButton);
    case B::kRight: return uint16_t(K::kRButton);
    case B::kMiddle: return uint16_t(K::kMButton);
    case B::kX1: return uint16_t(K::kXButton1);
    case B::kX2: return uint16_t(K::kXButton2);
    default: return 0;
  }
}
inline uint16_t ParseBinding(std::string_view text) {
  using B = rex::ui::MouseEvent::Button;
  if (text == "MWHEELUP") return kWheelUp;
  if (text == "MWHEELDOWN") return kWheelDown;
  if (text == "MOUSE1") return MouseButtonKey(B::kLeft);
  if (text == "MOUSE2") return MouseButtonKey(B::kRight);
  if (text == "MOUSE3") return MouseButtonKey(B::kMiddle);
  if (text == "MOUSE4") return MouseButtonKey(B::kX1);
  if (text == "MOUSE5") return MouseButtonKey(B::kX2);
  return uint16_t(rex::ui::ParseVirtualKey(text));
}
inline std::string BindingName(uint16_t key) {
  if (key == kWheelUp) return "MWHEELUP";
  if (key == kWheelDown) return "MWHEELDOWN";
  using B = rex::ui::MouseEvent::Button;
  constexpr B buttons[]{B::kLeft, B::kRight, B::kMiddle, B::kX1, B::kX2};
  for (int i = 0; i < 5; ++i)
    if (MouseButtonKey(buttons[i]) == key) return "MOUSE" + std::to_string(i + 1);
  return rex::ui::VirtualKeyToString(rex::ui::VirtualKey(key));
}
}
