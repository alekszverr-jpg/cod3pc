// Host-only numeric edit model. No guest pointers; UI and game callers serialize access.
#pragma once
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <cmath>

namespace cod3::menu {
class PercentEdit {
 public:
  enum Key { kBackspace = 10, kEnter, kEscape, kOther };
  void Offer(uint32_t page, int item) {
    if (page_ != page || item_ != item) { Cancel(); page_ = page; item_ = item; }
  }
  void Cancel() { text_.clear(); editing_ = false; pending_.reset(); }
  void Reset() { Cancel(); page_ = 0; item_ = -1; }
  bool OnKey(int key) {
    if (!page_ || item_ < 0) return false;
    if (key >= 0 && key <= 9) {
      if (!editing_) { text_.clear(); editing_ = true; }
      if (text_.size() < 3) text_ += char('0' + key);
      return true;
    }
    if (!editing_) return false;
    if (key == kBackspace) { if (!text_.empty()) text_.pop_back(); return true; }
    if (key == kEnter) {
      int value = 0;
      if (!text_.empty()) {
        const auto result = std::from_chars(text_.data(), text_.data() + text_.size(), value);
        if (result.ec == std::errc{}) pending_ = std::clamp((value + 2) / 5 * 5, 0, 100);
      }
      text_.clear(); editing_ = false; return true;
    }
    Cancel();
    return key == kEscape;
  }
  std::optional<int> Take(uint32_t page, int item) {
    if (page != page_ || item != item_) return {};
    auto value = pending_; pending_.reset(); return value;
  }
  std::string Display(uint32_t page, int item, const std::string& value) const {
    return page == page_ && item == item_ && editing_ ? text_ + "_%" : value;
  }
  static int FromFraction(float fraction) {
    return std::isfinite(fraction) ? int(std::lround(std::clamp(fraction, 0.0f, 1.0f) * 20.0f)) * 5 : 100;
  }
 private:
  uint32_t page_ = 0;
  int item_ = -1;
  bool editing_ = false;
  std::string text_;
  std::optional<int> pending_;
};
}
