#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
namespace rex::ui { class Window; }
namespace cod3::input {
void Reset();
void Attach(rex::ui::Window*, size_t, std::function<bool()> overlay);
void Detach(rex::ui::Window*);
void OnUiFrame();
// Independent on-foot actions, including held fire/aim. Capture is host-only.
void CaptureBinding(int action);
void CancelCapture();
std::string BindingText(int action);
std::string PromptBinding(int action);
bool PromptKeysActive();
// Merge interaction inputs into a successful original XInput read.
void ApplyToPad(uint8_t* base, uint32_t user, uint32_t state);
bool TakeSaveRequest();
}
