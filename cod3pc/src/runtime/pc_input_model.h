#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <utility>
#include <vector>
namespace cod3::input {
inline constexpr int kActionCount = 13;
inline constexpr int kBindingRows = 6;
inline int BindingPageFirst(int action) { return action / kBindingRows * kBindingRows; }
inline int BindingPageRows(int first) { return std::min(kBindingRows, kActionCount - first); }
inline int StepBinding(int first, int row, bool forward) {
  return (first + row + (forward ? 1 : kActionCount - 1)) % kActionCount;
}
// Only release the original controller-X holder if our grenade press created
// it. A controller/user hold that existed before the press is left alone.
// The original catch flag is released with G before the interaction animation
// ends. Keep raw camera for that whole interaction, then forget it.
class ThrowbackView {
 public:
  void Observe(bool catch_held, bool interaction) {
    if (catch_held) pending_ = true;
    if (interaction && pending_) running_ = true;
    if (running_ && !interaction) { pending_ = running_ = false; }
    if (!interaction && !catch_held) pending_ = false;
  }
  bool Active() const { return running_; }
  void Reset() { pending_ = running_ = false; }
 private:
  bool pending_ = false, running_ = false;
};
class ThrowbackRelease {
 public:
  void Pressed(bool held_before, bool held_after) { owned_ = !held_before && held_after; }
  bool Released(bool still_held) { const bool release = owned_ && still_held; owned_ = false; return release; }
  void Reset() { owned_ = false; }
 private:
  bool owned_ = false;
};
inline bool DisableMouseAimAssist(bool automatic, bool mnk, bool keyboard_mouse) {
  return automatic && mnk && keyboard_mouse;
}
// Sticky Aim is read inline by the original aim routine. Scope its override
// to that call, restoring the profile on both normal return and unwinding.
class ScopedStickyAim {
 public:
  ScopedStickyAim(uint8_t& setting, bool disable)
      : setting_(setting), saved_(setting), disable_(disable) { if (disable_) setting_ = 0; }
  ~ScopedStickyAim() { if (disable_) setting_ = saved_; }
  ScopedStickyAim(const ScopedStickyAim&) = delete;
  ScopedStickyAim& operator=(const ScopedStickyAim&) = delete;
 private:
  uint8_t& setting_;
  uint8_t saved_;
  bool disable_;
};
// High-resolution scroll may arrive in fractions of a Windows wheel detent.
// Bound a pathological single event before it reaches the action edge queue.
class WheelDetents {
 public:
  int Feed(int delta) {
    const int64_t sum = remainder_ + int64_t(delta);
    remainder_ = sum % 120;
    return int(std::clamp(sum / 120, int64_t(-16), int64_t(16)));
  }
  void Reset() { remainder_ = 0; }
 private:
  int64_t remainder_ = 0;
};
// Edges preserve sub-frame taps for one game poll, and bounded overflow releases
// everything rather than leaving a command held. Serialized by the caller.
class ActionEdges {
 public:
  struct Event { int action; bool down; };
  void Push(int a, bool down) {
    if (a < 0 || a >= kActionCount) return;
    if (queue_.size() >= 64) { blocked_ = true; queue_.clear(); return; }
    queue_.push_back({a, down});
  }
  std::vector<Event> Poll(bool allowed) {
    std::vector<Event> out;
    if (!allowed || blocked_) {
      for (int a = 0; a < kActionCount; ++a) if (held_[a]) out.push_back({a, false});
      held_ = {}; queue_.clear(); blocked_ = false; return out;
    }
    std::array<bool, kActionCount> started{};
    while (!queue_.empty()) {
      const auto e = queue_.front();
      if (!e.down && started[e.action]) break; // expose a short tap for one poll
      queue_.pop_front();
      if (held_[e.action] == e.down) continue;
      held_[e.action] = e.down;
      if (e.down) started[e.action] = true;
      out.push_back(e);
    }
    return out;
  }
  void Reset() { queue_.clear(); held_ = {}; blocked_ = false; }
 private:
  std::deque<Event> queue_;
  std::array<bool, kActionCount> held_{};
  bool blocked_ = false;
};
inline std::pair<float, float> MouseTurn(float dx, float dy, double sensitivity,
                                       double zoom, double sx, double sy, bool invert) {
  if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(sensitivity) ||
      !std::isfinite(zoom) || !std::isfinite(sx) || !std::isfinite(sy)) return {};
  const double unit = sensitivity * zoom * 0.022;
  return {float(dy * unit * sy * (invert ? -1 : 1)), float(-dx * unit * sx)};
}
}
