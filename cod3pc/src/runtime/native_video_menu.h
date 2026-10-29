#pragma once
#include <cstdint>
namespace cod3::video {
// Called before new guest threads, clears only Runtime-bound page/text caches.
void Reset();
// Host-only dirty flag consumed on the UI thread, also before presentation drain.
bool TakeSaveRequest();
// Game thread supplies current page; UI sees only a host edit model.
void NumberInput(uint8_t* base, uint32_t page);
bool NumberKey(int key);
void CancelNumber();
bool SetSlider(uint8_t* base, uint32_t page, int item, float fraction);
}
