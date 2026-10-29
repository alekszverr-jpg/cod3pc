#include "../cod3pc/src/runtime/menu_number.h"
#include <cstdio>
#include <stdexcept>
#include <limits>
void Check(bool yes) { if (!yes) throw std::runtime_error("numeric menu edit failed"); }
int main() {
  using E = cod3::menu::PercentEdit;
  try {
    E e;
    Check(!e.OnKey(3));
    e.Offer(100, 12);
    Check(e.OnKey(3) && e.OnKey(7));
    Check(e.Display(100, 12, "75%") == "37_%");
    Check(e.OnKey(E::kEnter) && e.Take(100, 12) == 35 && !e.Take(100, 12));
    e.OnKey(9); e.OnKey(9); e.OnKey(9); e.OnKey(E::kEnter);
    Check(e.Take(100, 12) == 100);
    e.OnKey(0); e.OnKey(E::kEnter); Check(e.Take(100, 12) == 0);
    e.OnKey(4); e.OnKey(E::kEscape); Check(!e.Take(100, 12));
    Check(e.Display(100, 12, "75%") == "75%");
    e.OnKey(4); e.OnKey(E::kBackspace); e.OnKey(E::kEnter); Check(!e.Take(100, 12));
    e.OnKey(5); e.OnKey(E::kEnter); e.Offer(101, 12); Check(!e.Take(101, 12));
    e.OnKey(8); e.Offer(101, 13); Check(e.Display(101, 13, "OFF") == "OFF");
    e.Reset(); Check(!e.OnKey(5));
    Check(E::FromFraction(-1) == 0 && E::FromFraction(1.2f) == 100);
    Check(E::FromFraction(0.37f) == 35 && E::FromFraction(0.38f) == 40);
    Check(E::FromFraction(std::numeric_limits<float>::quiet_NaN()) == 100);
    std::puts("numeric menu edit passed"); return 0;
  } catch (const std::exception& e) { std::puts(e.what()); return 1; }
}
