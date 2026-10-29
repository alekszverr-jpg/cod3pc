#include "../cod3pc/src/runtime/video_options.h"
#include <cstdio>
#include <stdexcept>
void Require(bool ok) { if (!ok) throw std::runtime_error("video option test failed"); }
int main() {
  using namespace cod3::video;
  try {
    Require(Step(0, "35", true) == "40" && Step(0, "35", false) == "30");
    Require(Step(0, "0", false) == "0" && Step(0, "100", true) == "100");
    Require(Step(0, "2147483647", true) == "100");
    Require(Step(0, "bad", false) == "95");
    Require(Step(1, "none", true) == "fxaa" && Step(1, "fxaa", true) == "fxaa_extreme");
    Require(Step(1, "fxaa_extreme", true) == "fxaa_extreme" && Step(1, "none", false) == "none");
    Require(Step(2, "-1", false) == "-1" && Step(2, "5", true) == "5");
    Require(Value(2, "2") == "2X" && Value(2, "5") == "16X");
    // The renderer flag permits tearing: the menu's V-Sync value is inverse.
    Require(Value(3, "true") == "ВЫКЛ" && Value(3, "false") == "ВКЛ");
    Require(Step(3, Step(3, "true", true), false) == "true");
    Require(Value(0, "35") == "35%" && Step(-1, "0", true).empty());
    std::puts("video options passed"); return 0;
  } catch (const std::exception& e) { std::puts(e.what()); return 1; }
}
