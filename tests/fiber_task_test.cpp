#include "../cod3pc/src/runtime/fiber_task.h"
#include <cfenv>
#include <cstdio>

struct State { Cod3FiberTask* task; int mode; int progress = 0; bool valid = true; };
void Body(void* argument) {
  auto& state = *static_cast<State*>(argument);
  int retained_local = 13;
  std::fesetround(state.mode);
  for (int i = 0; i < 3; ++i) {
    retained_local += 7;
    state.progress = i + 1;
    state.task->Suspend();
    state.valid &= retained_local == 13 + 7 * (i + 1);
    state.valid &= std::fegetround() == state.mode;
  }
}
int main() {
  const int original_rounding = std::fegetround();
  State first{nullptr, FE_DOWNWARD}, second{nullptr, FE_UPWARD};
  Cod3FiberTask a(Body, &first), b(Body, &second);
  first.task = &a;
  second.task = &b;
  for (int i = 0; i < 3; ++i) {
    if (a.Resume() || b.Resume()) return 1;
    if (first.progress != i + 1 || second.progress != i + 1) return 2;
    if (std::fegetround() != original_rounding) return 3;
  }
  if (!a.Resume() || !b.Resume() || !first.valid || !second.valid) return 4;
  if (!a.Resume() || !b.Resume()) return 5;
  std::puts("Fiber tasks passed: retained frames, independent continuations, floating-point state.");
}
