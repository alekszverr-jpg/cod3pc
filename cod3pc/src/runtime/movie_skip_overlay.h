#pragma once

#include <rex/ui/imgui_dialog.h>
#include <imgui.h>
#include <Windows.h>
#include <filesystem>
#include <functional>

bool Cod3MovieSkipAvailable();

// Retained by the application, never by a Runtime or a guest callback.
class Cod3MovieSkipOverlay final : public rex::ui::ImGuiDialog {
 public:
  Cod3MovieSkipOverlay(rex::ui::ImGuiDrawer* drawer, ImFont* font)
      : ImGuiDialog(drawer), font_(font) {}

  std::function<void()> on_frame;

  static ImFont* LoadFont(ImFontAtlas* atlas) {
    wchar_t windows[MAX_PATH]{};
    if (!GetWindowsDirectoryW(windows, MAX_PATH)) return nullptr;
    const auto path = std::filesystem::path(windows) / "Fonts" / "tahoma.ttf";
    if (!std::filesystem::exists(path)) return nullptr;
    return atlas->AddFontFromFileTTF(path.string().c_str(), 24.0f, nullptr,
                                   atlas->GetGlyphRangesCyrillic());
  }

 protected:
  void OnDraw(ImGuiIO& io) override {
    if (on_frame) on_frame();
    if (!Cod3MovieSkipAvailable()) return;
    // No ImGui window: this hint cannot capture mouse or keyboard input.
    const char* text = font_ ? "Space / Enter - Пропустить" : "Space / Enter - Skip";
    auto* font = font_ ? font_ : ImGui::GetFont();
    const float size = io.DisplaySize.y / 45.0f;
    const auto extent = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    const ImVec2 pos{(io.DisplaySize.x - extent.x) * 0.5f,
                     io.DisplaySize.y * 0.94f - extent.y};
    auto* draw = ImGui::GetForegroundDrawList();
    draw->AddText(font, size, {pos.x + 2, pos.y + 2}, IM_COL32(0, 0, 0, 220), text);
    draw->AddText(font, size, pos, IM_COL32(221, 206, 169, 255), text);
  }

 private:
  ImFont* font_;
};
