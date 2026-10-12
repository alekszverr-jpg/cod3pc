#include <setjmp.h>
#include "cod3pc_pch.h"
#include <cstdio>
#include <initializer_list>

volatile bool resumed_past_jump = false;
void Leaf(uint32_t key, int value) {
  ppc_longjmp(key, value);
  resumed_past_jump = true;
}
void Intermediate(uint32_t key, int value) { Leaf(key, value); }

int main() {
  // Two independent guest keys, reusable after a completed jump.
  for (uint32_t key : {0x82A0B1C0u, 0x82A0B400u}) {
    const int result = ppc_setjmp(key);
    if (result == 0) Intermediate(key, 7);
    if (result != 7 || resumed_past_jump) return 1;
    const int normalized = ppc_setjmp(key);
    if (normalized == 0) Intermediate(key, 0);
    if (normalized != 1 || resumed_past_jump) return 2;
  }
  std::puts("Nonlocal jumps passed: nested frames, distinct keys, zero-to-one normalization.");
}
