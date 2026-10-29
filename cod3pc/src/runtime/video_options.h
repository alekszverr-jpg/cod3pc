#pragma once
#include <algorithm>
#include <array>
#include <charconv>
#include <string>
#include <string_view>

namespace cod3::video {
inline constexpr std::array<const char*, 4> kFlags{
    "cod3_dof_strength", "swap_post_effect", "anisotropic_override",
    "d3d12_allow_variable_refresh_rate_and_tearing"};

inline int Number(std::string_view value, int fallback) {
  int number;
  const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
  return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() ? number : fallback;
}

inline std::string Step(int row, std::string_view value, bool right) {
  const int dir = right ? 1 : -1;
  switch (row) {
    case 0: return std::to_string(std::clamp(std::clamp(Number(value, 100), 0, 100) + dir * 5, 0, 100));
    case 1: {
      constexpr std::array<std::string_view, 3> choices{"none", "fxaa", "fxaa_extreme"};
      auto found = std::find(choices.begin(), choices.end(), value);
      const int index = found == choices.end() ? 0 : int(found - choices.begin());
      return std::string(choices[std::clamp(index + dir, 0, 2)]);
    }
    case 2: return std::to_string(std::clamp(std::clamp(Number(value, -1), -1, 5) + dir, -1, 5));
    case 3: return value == "true" ? "false" : "true";
    default: return {};
  }
}

inline std::string Value(int row, std::string_view value) {
  if (row == 0) return std::to_string(std::clamp(Number(value, 100), 0, 100)) + "%";
  if (row == 1) return value == "fxaa" ? "FXAA" : value == "fxaa_extreme" ? "FXAA+" : "ВЫКЛ";
  if (row == 2) {
    const int n = std::clamp(Number(value, -1), -1, 5);
    return n < 0 ? "ОРИГИНАЛ" : n == 0 ? "ВЫКЛ" : std::to_string(1 << (n - 1)) + "X";
  }
  if (row == 3) return value == "false" ? "ВКЛ" : "ВЫКЛ";
  return {};
}
}
