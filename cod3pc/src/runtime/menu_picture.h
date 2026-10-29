// Adapted from SirCarls/cod3_REcomp_, d54c1bb, pc_menu.cpp.
// Copyright (c) 2026 the COD_3_WAW contributors. MIT:
// licenses/SirCarls-COD_3_WAW-MIT.txt.
#pragma once
#include <cstdint>
#include <cmath>
namespace cod3::pcmenu {
struct PictureRect {
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
};

inline PictureRect PictureInWindow(float ww, float wh, float dw, float dh, bool letterbox, bool overscan, int32_t safe_x, int32_t safe_y) {
  if (ww <= 0 || wh <= 0 || dw <= 0 || dh <= 0) return {};
  auto safe_area = [overscan](int32_t percent) {
    return overscan && percent > 0 && percent < 100 ? float(percent) : 100.0f;
  };
  PictureRect r;
  if (ww * dh > wh * dw) {
    // A wider window: full width, cropped top and bottom at most to the safe
    // area, then bars at the sides (or stretched).
    const float safe = safe_area(safe_y);
    r.h = ww * dh / dw;
    bool bars = false;
    if (r.h * safe > wh * 100.0f) r.h = wh * 100.0f / safe, bars = true;
    r.w = bars && letterbox ? wh * 100.0f * dw / (dh * safe) : ww;
  } else {
    const float safe = safe_area(safe_x);
    r.w = wh * dw / dh;
    bool bars = false;
    if (r.w * safe > ww * 100.0f) r.w = ww * 100.0f / safe, bars = true;
    r.h = bars && letterbox ? ww * 100.0f * dh / (dw * safe) : wh;
  }
  r.x = (ww - r.w) * 0.5f;
  r.y = (wh - r.h) * 0.5f;
  return r;
}

inline bool MenuPoint(const PictureRect& r, float px, float py, float& x, float& y) {
  if (!(r.w > 0 && r.h > 0) || !std::isfinite(px) || !std::isfinite(py)) return false;
  const float u = (px - r.x) / r.w, v = (py - r.y) / r.h;
  if (u < 0 || u > 1 || v < 0 || v > 1) return false;
  x = u * 640.0f;
  y = v * 480.0f;
  return true;
}

}
