#pragma once
#include <cstdint>
struct PPCContext;
namespace cod3::menuprompts {
void KeyboardUsed();
void Reset();
void DrawLine(PPCContext&, uint8_t*);
}
