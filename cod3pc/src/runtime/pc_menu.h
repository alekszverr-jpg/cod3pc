#pragma once
#include <cstddef>
#include <functional>
namespace rex::ui { class Window; }
namespace cod3::pcmenu {
// Reset before starting guest threads, not while a menu handler is running.
void Reset();
void Attach(rex::ui::Window*, size_t z_order, std::function<bool()> overlay_open);
void Detach(rex::ui::Window*);
bool MenuActive();
void UpdateCursor(rex::ui::Window*);
bool FromPcInput(unsigned code);
}
