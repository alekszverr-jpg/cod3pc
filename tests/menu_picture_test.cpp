#include "../cod3pc/src/runtime/menu_picture.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>

void Require(bool ok) { if (!ok) throw std::runtime_error("menu pointer mapping failed"); }
bool Near(float a, float b) { return std::abs(a - b) < 0.01f; }
int main() {
  using namespace cod3::pcmenu;
  try {
    float x = 0, y = 0;
    const auto hd = PictureInWindow(1920, 1080, 1280, 720, true, false, 100, 100);
    Require(MenuPoint(hd, 960, 540, x, y) && Near(x, 320) && Near(y, 240));
    Require(MenuPoint(hd, 1920, 1080, x, y) && Near(x, 640) && Near(y, 480));
    // A 16:9 game in a 4:3 window: 96-pixel bars above and below.
    const auto tall = PictureInWindow(1024, 768, 1280, 720, true, false, 100, 100);
    Require(Near(tall.y, 96) && Near(tall.h, 576));
    Require(!MenuPoint(tall, 512, 50, x, y));
    Require(MenuPoint(tall, 512, 96, x, y) && Near(y, 0));
    // Resizing to a wide window: side bars must not activate a menu row.
    const auto wide = PictureInWindow(2560, 1080, 1280, 720, true, false, 100, 100);
    Require(Near(wide.x, 320) && Near(wide.w, 1920));
    Require(!MenuPoint(wide, 200, 540, x, y));
    Require(MenuPoint(wide, 1280, 540, x, y) && Near(x, 320) && Near(y, 240));
    const auto stretch = PictureInWindow(1024, 768, 1280, 720, false, false, 100, 100);
    Require(MenuPoint(stretch, 512, 0, x, y) && Near(y, 0));
    const auto crop = PictureInWindow(1024, 768, 1280, 720, true, true, 90, 90);
    Require(crop.y < 96 && crop.h > 576);
    Require(MenuPoint(crop, 512, 384, x, y) && Near(x, 320) && Near(y, 240));
    Require(!MenuPoint(PictureInWindow(0, 0, 1280, 720, true, false, 100, 100), 0, 0, x, y));
    std::puts("menu picture mapping passed");
    return 0;
  } catch (const std::exception& e) { std::puts(e.what()); return 1; }
}
