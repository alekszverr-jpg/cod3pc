// Adapted from SirCarls/cod3_REcomp_ d54c1bb, menu_video.cpp/menu_fov.cpp.
// Copyright (c) 2026 the COD_3_WAW contributors. MIT:
// licenses/SirCarls-COD_3_WAW-MIT.txt. Four existing rows, no FOV/MP/HD imports.
#include "native_video_menu.h"
#include "video_options.h"
#include "video_menu_layout.h"
#include "menu_guest.h"
#include "menu_number.h"
#include "pc_input.h"
#include "pc_input_model.h"
#include "pc_menu_addresses.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <Windows.h>
#include <atomic>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
namespace fe = cod3::video::layout;
namespace pm = cod3::addr::pc_menu;
using namespace cod3::guest;
constexpr uint32_t kOptionsVtable = 0x820729A4;
constexpr uint32_t kGameplayVtable = 0x82071FAC;
std::unordered_set<uint32_t> options_pages;  // game-thread menu state
uint32_t video_page = 0, back_to_options = 0;
std::array<std::string, 4> last_values;
std::atomic<bool> save_requested{false};
std::mutex number_mutex;
cod3::menu::PercentEdit number_edit;
int NumberItem(uint8_t* base, uint32_t page);
std::string NumberText(uint32_t page, int item, const std::string& value) {
  std::lock_guard lock(number_mutex);
  return number_edit.Display(page, item, value);
}
void SetDof(int value) {
  const auto text = std::to_string(value);
  if (rex::cvar::GetFlagByName(cod3::video::kFlags[0]) == text) return;
  if (rex::cvar::SetFlagByName(cod3::video::kFlags[0], text)) {
    save_requested.store(true, std::memory_order_release);
    REXLOG_INFO("COD3_VIDEO_SETTING input=number_or_slider cod3_dof_strength={}", value);
  }
}
std::mutex text_mutex;
std::unordered_map<std::string, uint32_t> guest_texts;
void ResetPauseVideo();
constexpr const char* kLabels[] = {"COD3PC_VIDEO_DOF", "COD3PC_VIDEO_AA",
                                  "COD3PC_VIDEO_AF", "COD3PC_VIDEO_VSYNC"};
constexpr const char* kHelp[] = {"COD3PC_VIDEO_HELP_DOF", "COD3PC_VIDEO_HELP_AA",
                                "COD3PC_VIDEO_HELP_AF", "COD3PC_VIDEO_HELP_VSYNC"};
struct Text { const char* key; const char* value; };
constexpr Text kTexts[] = {
  {"COD3PC_KEYS", "КЛАВИШИ"},
  {"COD3PC_KEYS_TITLE", "НАСТРОЙКИ / КЛАВИШИ"},
  {"COD3PC_KEYS_HELP", "Enter - назначить. Вверх/вниз - действия и страницы. Esc - отменить."},
  {"COD3PC_KEY_RELOAD", "ПЕРЕЗАРЯДКА"}, {"COD3PC_KEY_USE", "ВЗАИМОДЕЙСТВИЕ"},
  {"COD3PC_KEY_CROUCH", "ПРИСЕСТЬ"}, {"COD3PC_KEY_PRONE", "ЛЕЧЬ"},
  {"COD3PC_KEY_FIRE", "СТРЕЛЬБА"}, {"COD3PC_KEY_AIM", "ПРИЦЕЛИВАНИЕ"},
  {"COD3PC_KEY_WEAPNEXT", "СЛЕДУЮЩЕЕ ОРУЖИЕ"}, {"COD3PC_KEY_WEAPPREV", "ПРЕДЫДУЩЕЕ ОРУЖИЕ"},
  {"COD3PC_KEY_FRAG", "ГРАНАТА"}, {"COD3PC_KEY_SMOKE", "ДЫМОВАЯ ГРАНАТА"},
  {"COD3PC_KEY_MELEE", "БЛИЖНИЙ БОЙ"},
  {"COD3PC_KEY_BINOCULARS", "БИНОКЛЬ"}, {"COD3PC_KEY_HOLDBREATH", "ЗАДЕРЖКА ДЫХАНИЯ"},
  {"COD3PC_VIDEO", "ВИДЕО"},
  {"COD3PC_VIDEO_TITLE", "НАСТРОЙКИ / ВИДЕО"},
  {"COD3PC_VIDEO_RESTART", "Для сглаживания и V-Sync нужен перезапуск игры."},
  {"COD3PC_VIDEO_HELP", "Настройки изображения для ПК. Enter - выбрать, Esc - назад."},
  {kLabels[0], "ГЛУБИНА РЕЗКОСТИ"}, {kLabels[1], "СГЛАЖИВАНИЕ"},
  {kLabels[2], "ФИЛЬТРАЦИЯ ТЕКСТУР"}, {kLabels[3], "ВЕРТ. СИНХРОНИЗАЦИЯ"},
  {kHelp[0], "Сила размытия: 100% - оригинал, 0% - выкл. Стрелки - изменить. Цифры, Enter - применить, Esc - отменить ввод."},
  {kHelp[1], "Сглаживание краев изображения. FXAA+ сильнее смягчает картинку. Применяется после перезапуска игры. Стрелки влево/вправо - изменить."},
  {kHelp[2], "Четкость текстур под углом. ОРИГИНАЛ сохраняет настройки игры. Стрелки влево/вправо - изменить."},
  {kHelp[3], "Устраняет разрывы изображения. Применяется после перезапуска игры. Стрелки влево/вправо - изменить."},
};

bool Valid(uint32_t p) { return p >= 0x10000 && p < 0xC0000000 && !(p & 3); }
bool IsVideo(uint8_t* base, uint32_t p) {
  return p == video_page && Valid(p) && ReadU32(base, p) == kGameplayVtable;
}
bool HasVideo(uint8_t* base, uint32_t p) {
  return Valid(p) && ReadU32(base, p) == kOptionsVtable && options_pages.contains(p) &&
      ReadU16(base, p + pm::kPageCount) == 5;
}
int Selected(uint8_t* base, uint32_t p) { return int16_t(ReadU16(base, p + pm::kPageSelected)); }
uint64_t VC(PPCContext& ctx, uint8_t* base, uint32_t obj, uint32_t slot,
            uint64_t a = 0, uint64_t b = 0, uint64_t c = 0) {
  return Call(ctx, base).Invoke(VirtualSlot(base, obj, slot), {obj, a, b, c});
}

// The installed Russian game's font consumes single-byte Cyrillic glyphs.
std::string GameText(const std::string& utf8) {
  const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), int(utf8.size()), nullptr, 0);
  if (!count) return {};
  std::wstring wide(count, L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), int(utf8.size()), wide.data(), count);
  const int bytes = WideCharToMultiByte(1251, WC_NO_BEST_FIT_CHARS, wide.data(), count, nullptr, 0, nullptr, nullptr);
  std::string out(bytes, '\0');
  WideCharToMultiByte(1251, WC_NO_BEST_FIT_CHARS, wide.data(), count, out.data(), bytes, nullptr, nullptr);
  return out;
}

void TextKey(PPCContext& ctx, uint8_t* base, uint32_t elem, const char* key) {
  if (!Valid(elem)) return;
  Call call(ctx, base);
  call.Invoke(VirtualSlot(base, elem, fe::kElemSetTextKey), {elem, call.PushString(key)});
}
void TextRaw(PPCContext& ctx, uint8_t* base, uint32_t elem, const std::string& value) {
  if (!Valid(elem)) return;
  Call call(ctx, base);
  call.Invoke(VirtualSlot(base, elem, fe::kElemSetTextRaw), {elem, call.PushString(GameText(value))});
}
void Instructions(PPCContext& ctx, uint8_t* base, uint32_t elem, const char* key) {
  if (!Valid(elem)) return;
  Call call(ctx, base);
  const auto width = call.Invoke(VirtualSlot(base, elem, fe::kWrapGetWidth), {elem});
  call.Invoke(VirtualSlot(base, elem, fe::kWrapSetTextKey), {elem, call.PushString(key), width},
              {ReadF32(base, fe::kTextScaleDefault)});
}
void ItemKey(PPCContext& ctx, uint8_t* base, uint32_t page, int i, const char* key) {
  const auto list = ReadU32(base, page + pm::kPageItems);
  if (!Valid(list)) return;
  const auto item = ReadU32(base, list + 4 * i);
  if (!Valid(item)) return;
  Call call(ctx, base);
  call.Invoke(VirtualSlot(base, item, fe::kItemSetLabelKey), {item, call.PushString(key)});
}
uint32_t Find(PPCContext& ctx, uint8_t* base, uint32_t panel, const char* name, bool quad = false) {
  Call call(ctx, base);
  const auto obj = uint32_t(call.Invoke(quad ? fe::kPanelFindQuad : fe::kPanelFindElement,
                                      {panel, call.PushString(name)}));
  if (!Valid(obj)) return 0;
  if (quad && ReadString(base, ReadU32(base, obj + fe::kQuadName) + 12, 64) != name) return 0;
  return obj;
}
void ApplyLabels(PPCContext& ctx, uint8_t* base, uint32_t page, bool video) {
  TextKey(ctx, base, ReadU32(base, page + fe::kGameplayTitleMain),
          video ? "COD3PC_VIDEO_TITLE" : ReadString(base, fe::kGameplayTitleKey, 64).c_str());
  for (int i = 0; i < 4; ++i) {
    const auto key = video ? std::string(kLabels[i]) :
        ReadString(base, ReadU32(base, fe::kGameplayLabelKeys + i * 4), 64);
    ItemKey(ctx, base, page, i, key.c_str());
  }
}
void ShowSelection(PPCContext& ctx, uint8_t* base, uint32_t page) {
  const auto i = Selected(base, page);
  if (i < 0 || i >= 4) return;
  TextKey(ctx, base, ReadU32(base, page + fe::kCurrentSelectedText), kLabels[i]);
  Instructions(ctx, base, ReadU32(base, page + fe::kInstructionsText), kHelp[i]);
}
void ShowValues(PPCContext& ctx, uint8_t* base, uint32_t page, bool force) {
  for (int i = 0; i < 4; ++i) {
    auto value = cod3::video::Value(i, rex::cvar::GetFlagByName(cod3::video::kFlags[i]));
    if (i == 0) value = NumberText(page, 0, value);
    if (force || value != last_values[i])
      TextRaw(ctx, base, ReadU32(base, page + fe::kGameplayValues + i * 4), value);
    last_values[i] = value;
  }
}
void OptionsSelection(PPCContext& ctx, uint8_t* base, uint32_t page) {
  const int i = Selected(base, page);
  if (i < 0 || i > 4) return;
  TextKey(ctx, base, ReadU32(base, page + 72), i == 4 ? "COD3PC_VIDEO" :
      ReadString(base, ReadU32(base, 0x82064BE0 + i * 4), 64).c_str());
  Instructions(ctx, base, ReadU32(base, page + 80), i == 4 ? "COD3PC_VIDEO_HELP" :
      ReadString(base, ReadU32(base, 0x82064BF0 + i * 4), 64).c_str());
}
}

namespace cod3::video {
void Reset() {
  { std::lock_guard lock(number_mutex); number_edit.Reset(); }
  ResetPauseVideo();
  options_pages.clear(); video_page = back_to_options = 0; last_values = {};
  std::lock_guard lock(text_mutex);
  guest_texts.clear();  // old Runtime owns and frees its SystemHeap allocations
}
bool TakeSaveRequest() { return save_requested.exchange(false, std::memory_order_acq_rel); }
void NumberInput(uint8_t* base, uint32_t page) {
  const int item = NumberItem(base, page);
  std::optional<int> value;
  {
    std::lock_guard lock(number_mutex);
    number_edit.Offer(item >= 0 ? page : 0, item);
    value = number_edit.Take(page, item);
  }
  if (value) SetDof(*value);
}
bool NumberKey(int key) {
  std::lock_guard lock(number_mutex);
  return number_edit.OnKey(key);
}
void CancelNumber() {
  std::lock_guard lock(number_mutex); number_edit.Reset();
}
}

REX_EXTERN(__imp__sub_8252CB98);
REX_HOOK_RAW(sub_8252CB98) {
  const auto key = ctx.r4.u32 ? ReadString(base, ctx.r4.u32, 128) : std::string();
  for (const auto& text : kTexts) {
    if (key != text.key) continue;
    std::lock_guard lock(text_mutex);
    auto& ptr = guest_texts[key];
    if (!ptr) {
      const auto encoded = GameText(text.value);
      ptr = REX_KERNEL_MEMORY()->SystemHeapAlloc(uint32_t(encoded.size() + 1));
      if (!ptr) { __imp__sub_8252CB98(ctx, base); return; }
      std::memcpy(base + ptr, encoded.c_str(), encoded.size() + 1);
    }
    ctx.r3.u64 = ptr;
    return;
  }
  __imp__sub_8252CB98(ctx, base);
}

REX_EXTERN(__imp__sub_824E2948);
REX_HOOK_RAW(sub_824E2948) {
  const auto self = ctx.r3.u32;
  __imp__sub_824E2948(ctx, base);
  if (!Valid(self) || ReadU32(base, self) != kOptionsVtable ||
      ReadU16(base, self + pm::kPageCount) != 4) return;
  if (ReadU32(base, fe::kLanguageMode) != 0) {
    REXLOG_WARN("COD3_VIDEO_MENU unavailable reason=alternate_layout"); return;
  }
  const auto panel = ReadU32(base, self + 60);
  const auto label = Find(ctx, base, panel, "text_option_05");
  const auto band = Find(ctx, base, panel, "bkg_row_05", true);
  const auto line = Find(ctx, base, panel, "bkg_line_04", true);
  const auto old = ReadU32(base, self + pm::kPageItems);
  if (!Valid(old) || !label) { REXLOG_WARN("COD3_VIDEO_MENU unavailable reason=missing_row"); return; }
  const auto items = uint32_t(Call(ctx, base).Invoke(fe::kOperatorNew, {20}));
  if (!Valid(items)) return;
  for (int i = 0; i < 5; ++i) WriteU32(base, items + 4 * i, i < 4 ? ReadU32(base, old + 4 * i) : 0);
  WriteU32(base, self + pm::kPageItems, items);
  Call(ctx, base).Invoke(fe::kOperatorDelete, {old});
  VC(ctx, base, self, fe::kPageAddItem, 4, label);
  if (!Valid(ReadU32(base, items + 16))) {
    REXLOG_WARN("COD3_VIDEO_MENU unavailable reason=add_item_failed"); return;
  }
  ItemKey(ctx, base, self, 4, "COD3PC_VIDEO");
  for (auto elem : {label, band, line}) if (Valid(elem)) VC(ctx, base, elem, fe::kElemSetVisible, 1);
  WriteU16(base, self + pm::kPageCount, 5);
  options_pages.insert(self);
  REXLOG_INFO("COD3_VIDEO_MENU_READY page={:08X} rows=4", self);
}

REX_EXTERN(__imp__sub_824BC070);
REX_HOOK_RAW(sub_824BC070) {
  const auto self = ctx.r3.u32;
  const int item = int32_t(ctx.r4.u32);
  if (HasVideo(base, self) && (item == 0 || item == 4)) {
    const auto sys = ReadU32(base, self + fe::kPageSystem);
    const auto pages = Valid(sys) ? ReadU32(base, sys + fe::kSysPages) : 0;
    const auto page = Valid(pages) ? ReadU32(base, pages + 12) : 0;
    if (Valid(page) && ReadU32(base, page) == kGameplayVtable) {
      video_page = item == 4 ? page : 0;
      ApplyLabels(ctx, base, page, item == 4);
      if (item == 4) {
        back_to_options = self;
        VC(ctx, base, sys, fe::kSysGoTo, 3);
        REXLOG_INFO("COD3_VIDEO_MENU_OPEN page={:08X}", page);
        return;
      }
    } else if (item == 4) { REXLOG_ERROR("COD3_VIDEO_MENU missing gameplay page"); return; }
  }
  __imp__sub_824BC070(ctx, base);
}

REX_EXTERN(__imp__sub_824ED298);
REX_HOOK_RAW(sub_824ED298) {
  const auto self = ctx.r3.u32;
  __imp__sub_824ED298(ctx, base);
  if (!HasVideo(base, self)) return;
  if (back_to_options == self) { back_to_options = 0; VC(ctx, base, self, pm::kPageSetSelection, 4, 1); }
  OptionsSelection(ctx, base, self);
}

REX_EXTERN(__imp__sub_824C3248);
REX_HOOK_RAW(sub_824C3248) {
  const auto self = ctx.r3.u32;
  if (!HasVideo(base, self)) { __imp__sub_824C3248(ctx, base); return; }
  // Original first virtual call, then bounded title/help lookup (four stock + VIDEO).
  VC(ctx, base, self, 204);
  OptionsSelection(ctx, base, self);
}
REX_EXTERN(__imp__sub_824C3190);
REX_HOOK_RAW(sub_824C3190) {
  const auto self = ctx.r3.u32;
  if (!HasVideo(base, self)) { __imp__sub_824C3190(ctx, base); return; }
  VC(ctx, base, self, 208);
  OptionsSelection(ctx, base, self);
}

REX_EXTERN(__imp__sub_824DB960);
REX_HOOK_RAW(sub_824DB960) {
  const auto self = ctx.r3.u32;
  __imp__sub_824DB960(ctx, base);
  if (IsVideo(base, self)) {
    ApplyLabels(ctx, base, self, true);
    ShowValues(ctx, base, self, true); ShowSelection(ctx, base, self);
  }
}
REX_EXTERN(__imp__sub_824BD160);
REX_HOOK_RAW(sub_824BD160) {
  const auto self = ctx.r3.u32;
  if (!IsVideo(base, self)) { __imp__sub_824BD160(ctx, base); return; }
  const int i = Selected(base, self);
  if (i < 0 || i >= 4) return;
  const auto flag = cod3::video::kFlags[i];
  const auto next = cod3::video::Step(i, rex::cvar::GetFlagByName(flag), (ctx.r4.u32 & 255) == 1);
  if (!rex::cvar::SetFlagByName(flag, next)) {
    REXLOG_ERROR("COD3_VIDEO_SETTING_REJECTED {}={}", flag, next); return;
  }
  save_requested.store(true, std::memory_order_release);
  WriteF32(base, self + fe::kArrowFlashTimer, ReadF32(base, fe::kArrowFlashTime));
  ShowValues(ctx, base, self, false); ShowSelection(ctx, base, self);
  REXLOG_INFO("COD3_VIDEO_SETTING {}={}", flag, next);
}
REX_EXTERN(__imp__sub_824CD2D0);
REX_HOOK_RAW(sub_824CD2D0) {
  const auto self = ctx.r3.u32;
  __imp__sub_824CD2D0(ctx, base);
  if (IsVideo(base, self)) ShowValues(ctx, base, self, false);
}
REX_EXTERN(__imp__sub_824C3FE0);
REX_HOOK_RAW(sub_824C3FE0) {
  const auto self = ctx.r3.u32;
  __imp__sub_824C3FE0(ctx, base);
  if (IsVideo(base, self)) ShowSelection(ctx, base, self);
}
REX_EXTERN(__imp__sub_824C3F08);
REX_HOOK_RAW(sub_824C3F08) {
  const auto self = ctx.r3.u32;
  __imp__sub_824C3F08(ctx, base);
  if (IsVideo(base, self)) ShowSelection(ctx, base, self);
}


// Pause submenu: adapted from SirCarls menu_fov.cpp's AdvancedOptions rows.
namespace {
constexpr uint32_t kAdvancedVtable = 0x82071B24;
constexpr int kVideoEntry = 11, kVideoFirst = 12, kBindEntry = 18, kVideoCount = 19;
constexpr const char* kBindLabels[] = {"COD3PC_KEY_RELOAD", "COD3PC_KEY_USE", "COD3PC_KEY_CROUCH", "COD3PC_KEY_PRONE", "COD3PC_KEY_FIRE", "COD3PC_KEY_AIM", "COD3PC_KEY_WEAPNEXT", "COD3PC_KEY_WEAPPREV", "COD3PC_KEY_FRAG", "COD3PC_KEY_SMOKE", "COD3PC_KEY_MELEE", "COD3PC_KEY_BINOCULARS", "COD3PC_KEY_HOLDBREATH"};
static_assert(std::size(kBindLabels) == cod3::input::kActionCount);
struct PauseVideo {
  uint32_t entry = 0, note = 0, title = 0, reset = 0;
  std::array<uint32_t, 4> anchor{};
  std::array<std::array<uint32_t, 4>, cod3::input::kBindingRows> rows{};  // label, value, left, right
  std::array<uint8_t, 12> disabled{}, visible{};
  std::vector<uint32_t> owned, hide;
  std::array<std::string, cod3::input::kBindingRows> seen;
  bool active = false, reopened = false;
  uint32_t bind_entry = 0;
  bool controls = false;
  int first_binding = 0;
  std::array<uint32_t, 2> gauge_source{}, gauge{};
};
std::unordered_map<uint32_t, PauseVideo> pause_pages;
void ResetPauseVideo() { pause_pages.clear(); }
uint32_t Entry(uint8_t* base, uint32_t self, int i) {
  const auto list = ReadU32(base, self + pm::kPageItems);
  return Valid(list) ? ReadU32(base, list + i * 4) : 0;
}
void Visible(PPCContext& ctx, uint8_t* base, uint32_t e, bool on) {
  if (Valid(e)) VC(ctx, base, e, fe::kElemSetVisible, on);
}
void ItemVisible(PPCContext& ctx, uint8_t* base, uint32_t e, bool on) {
  if (!Valid(e)) return;
  VC(ctx, base, e, pm::kEntrySetDisabled, !on);
  VC(ctx, base, e, pm::kEntrySetVisible, on);
}
uint32_t Copy(PPCContext& ctx, uint8_t* base, uint32_t source, bool quad, PauseVideo& r) {
  if (!Valid(source)) return 0;
  Call c(ctx, base);
  const auto copy = uint32_t(c.Invoke(fe::kOperatorNew, {quad ? fe::kQuadSize : fe::kTextSize}));
  if (!Valid(copy)) return 0;
  c.Invoke(quad ? fe::kQuadCtor : fe::kTextCtor, {copy});
  c.Invoke(VirtualSlot(base, copy, fe::kElemAssign), {copy, source});
  r.owned.push_back(copy);
  Visible(ctx, base, copy, true);
  return copy;
}
void FollowText(uint8_t* base, uint32_t source, uint32_t dest, float dy) {
  for (uint32_t at = fe::kTextTransform; at < fe::kTextTransformEnd; at += 4)
    WriteU32(base, dest + at, ReadU32(base, source + at));
  WriteF32(base, dest + fe::kTextY, ReadF32(base, source + fe::kTextY) + dy);
  WriteF32(base, dest + fe::kElemOpacity, ReadF32(base, source + fe::kElemOpacity));
}
void FollowQuad(uint8_t* base, uint32_t source, uint32_t dest, float dy) {
  const auto a = ReadU32(base, source + fe::kQuadParts), b = ReadU32(base, dest + fe::kQuadParts);
  const auto n = ReadU32(base, source + fe::kQuadPartCount);
  if (!Valid(a) || !Valid(b) || n > 8 || n != ReadU32(base, dest + fe::kQuadPartCount)) return;
  for (uint32_t part = 0; part < n; ++part) {
    const auto x = ReadU32(base, a + part * 4), y = ReadU32(base, b + part * 4);
    if (!Valid(x) || !Valid(y)) continue;
    for (uint32_t v = 0; v < 4; ++v) {
      const auto at = fe::kQuadPartVerts + v * fe::kQuadVertStride;
      WriteF32(base, y + at, ReadF32(base, x + at));
      WriteF32(base, y + at + 4, ReadF32(base, x + at + 4) + dy);
    }
  }
  WriteF32(base, dest + fe::kElemOpacity, ReadF32(base, source + fe::kElemOpacity));
}
// Native panel pictures, remapped to a bounded rectangle. No extra rendering backend.
struct GaugeRect { float x0 = 0, y0 = 0, x1 = 0, y1 = 0; };
bool QuadRect(uint8_t* base, uint32_t quad, GaugeRect& rect) {
  if (!Valid(quad)) return false;
  const auto parts = ReadU32(base, quad + fe::kQuadParts);
  const auto n = ReadU32(base, quad + fe::kQuadPartCount);
  if (!Valid(parts) || n == 0 || n > 8) return false;
  rect = {1e9f, 1e9f, -1e9f, -1e9f};
  for (uint32_t p = 0; p < n; ++p) {
    const auto part = ReadU32(base, parts + p * 4);
    if (!Valid(part)) return false;
    for (int v = 0; v < 4; ++v) {
      const auto at = part + fe::kQuadPartVerts + v * fe::kQuadVertStride;
      const float x = ReadF32(base, at), y = ReadF32(base, at + 4);
      if (!std::isfinite(x) || !std::isfinite(y)) return false;
      rect.x0 = std::min(rect.x0, x); rect.x1 = std::max(rect.x1, x);
      rect.y0 = std::min(rect.y0, y); rect.y1 = std::max(rect.y1, y);
    }
  }
  return rect.x1 > rect.x0 && rect.y1 > rect.y0;
}
bool MapGauge(uint8_t* base, uint32_t source, uint32_t dest, GaugeRect target, float alpha) {
  GaugeRect from;
  if (!QuadRect(base, source, from) || !Valid(dest)) return false;
  const auto a = ReadU32(base, source + fe::kQuadParts), b = ReadU32(base, dest + fe::kQuadParts);
  const auto n = ReadU32(base, source + fe::kQuadPartCount);
  if (!Valid(b) || n != ReadU32(base, dest + fe::kQuadPartCount)) return false;
  for (uint32_t p = 0; p < n; ++p) {
    const auto s = ReadU32(base, a + p * 4), d = ReadU32(base, b + p * 4);
    if (!Valid(d)) return false;
    for (int v = 0; v < 4; ++v) {
      const auto at = fe::kQuadPartVerts + v * fe::kQuadVertStride;
      WriteF32(base, d + at, target.x0 + (ReadF32(base, s + at) - from.x0) /
          (from.x1 - from.x0) * (target.x1 - target.x0));
      WriteF32(base, d + at + 4, target.y0 + (ReadF32(base, s + at + 4) - from.y0) /
          (from.y1 - from.y0) * (target.y1 - target.y0));
    }
  }
  WriteF32(base, dest + fe::kElemOpacity, alpha);
  return true;
}
void DrawDofGauge(PPCContext& ctx, uint8_t* base, PauseVideo& r) {
  if (!r.gauge[0] || !r.gauge[1]) return;
  GaugeRect left, right, original;
  if (!QuadRect(base, r.rows[0][2], left) || !QuadRect(base, r.rows[0][3], right) ||
      !QuadRect(base, r.gauge_source[0], original)) return;
  const float center = (left.y0 + left.y1) * 0.5f;
  const float half = std::clamp((original.y1 - original.y0) * 0.5f, 2.0f, 6.0f);
  GaugeRect frame{left.x1 + 3.0f, center - half, right.x0 - 3.0f, center + half};
  if (frame.x1 - frame.x0 < 8.0f) return;
  const float alpha = ReadF32(base, r.rows[0][2] + fe::kElemOpacity);
  if (!MapGauge(base, r.gauge_source[0], r.gauge[0], frame, alpha)) return;
  VC(ctx, base, r.gauge[0], 4);
  const float fraction = std::clamp(cod3::video::Number(rex::cvar::GetFlagByName(
      cod3::video::kFlags[0]), 100), 0, 100) / 100.0f;
  if (fraction > 0.0f) {
    GaugeRect fill{frame.x0 + 1.0f, frame.y0 + 1.0f,
        frame.x0 + 1.0f + (frame.x1 - frame.x0 - 2.0f) * fraction, frame.y1 - 1.0f};
    // Frame geometry supplies stable corners even if native volume's fill is empty.
    if (MapGauge(base, r.gauge_source[0], r.gauge[1], fill, alpha)) VC(ctx, base, r.gauge[1], 4);
  }
  // Keep the native value centered just to the right of the slider's arrow.
  WriteF32(base, r.rows[0][1] + pm::kTextX, right.x1 + 24.0f);
}
void PositionPause(uint8_t* base, PauseVideo& r) {
  FollowText(base, r.reset, r.entry, -20.0f);
  if (r.bind_entry) {
    FollowText(base, r.reset, r.bind_entry, -20.0f);
    WriteF32(base, r.bind_entry + pm::kTextX, ReadF32(base, r.entry + pm::kTextX) + 150.0f);
  }
  for (int i = 0; i < int(r.rows.size()); ++i) {
    FollowText(base, r.anchor[0], r.rows[i][0], i * 20.0f);
    FollowText(base, r.anchor[1], r.rows[i][1], i * 20.0f);
    FollowQuad(base, r.anchor[2], r.rows[i][2], i * 20.0f);
    FollowQuad(base, r.anchor[3], r.rows[i][3], i * 20.0f);
  }
  FollowText(base, r.reset, r.note,
      ReadF32(base, r.anchor[0] + fe::kTextY) + (r.controls ? 160.0f : 120.0f) - ReadF32(base, r.reset + fe::kTextY));
}
void SelectPause(PPCContext& ctx, uint8_t* base, uint32_t self, int item) {
  cod3::input::CancelCapture();
  VC(ctx, base, self, pm::kPageSetSelection, item, 1);
  VC(ctx, base, self, pm::kPageSelectionChanged);
  Call(ctx, base).Invoke(0x824BD3F8, {self});
}
void ValuesPause(PPCContext& ctx, uint8_t* base, PauseVideo& r, bool force) {
  for (int i = 0; i < (r.controls ? cod3::input::BindingPageRows(r.first_binding) : 4); ++i) {
    auto value = r.controls ? cod3::input::BindingText(r.first_binding + i) :
        cod3::video::Value(i, rex::cvar::GetFlagByName(cod3::video::kFlags[i]));
    if (i == 0 && !r.controls) {
      // The model is offered only for this selected page by ProcessInput.
      for (const auto& [page, state] : pause_pages)
        if (&state == &r) { value = NumberText(page, kVideoFirst, value); break; }
    }
    if (force || r.seen[i] != value) TextRaw(ctx, base, r.rows[i][1], value);
    r.seen[i] = value;
  }
}
void RefreshBindingPage(PPCContext& ctx, uint8_t* base, uint32_t self, PauseVideo& r) {
  for (int i = 0; i < cod3::input::kBindingRows; ++i) {
    const bool shown = i < cod3::input::BindingPageRows(r.first_binding);
    ItemVisible(ctx, base, Entry(base, self, kVideoFirst + i), shown);
    if (shown) ItemKey(ctx, base, self, kVideoFirst + i, kBindLabels[r.first_binding + i]);
  }
  TextRaw(ctx, base, r.title, "НАСТРОЙКИ / КЛАВИШИ " +
      std::to_string(r.first_binding / cod3::input::kBindingRows + 1) + "/" +
      std::to_string((cod3::input::kActionCount + cod3::input::kBindingRows - 1) / cod3::input::kBindingRows));
  ValuesPause(ctx, base, r, true);
}
void MovePauseSelection(PPCContext& ctx, uint8_t* base, uint32_t self,
                        PauseVideo& r, int selected, bool forward) {
  if (r.controls) {
    const int row = std::clamp(selected - kVideoFirst, 0,
        cod3::input::BindingPageRows(r.first_binding) - 1);
    const int action = cod3::input::StepBinding(r.first_binding, row, forward);
    const int first = cod3::input::BindingPageFirst(action);
    if (first != r.first_binding) {
      r.first_binding = first;
      RefreshBindingPage(ctx, base, self, r);
    }
    SelectPause(ctx, base, self, kVideoFirst + action - r.first_binding);
  } else {
    SelectPause(ctx, base, self, forward ? (selected < 15 ? selected + 1 : 12) :
        (selected > 12 ? selected - 1 : 15));
  }
}
int NumberItem(uint8_t* base, uint32_t page) {
  if (IsVideo(base, page) && Selected(base, page) == 0) return 0;
  const auto it = pause_pages.find(page);
  return it != pause_pages.end() && it->second.active && !it->second.controls && Selected(base, page) == kVideoFirst
      ? kVideoFirst : -1;
}
void EnterPauseVideo(PPCContext& ctx, uint8_t* base, uint32_t self, PauseVideo& r, bool controls = false) {
  r.controls = controls;
  r.first_binding = 0;
  for (int i = 0; i < kVideoFirst; ++i) {
    const auto item = Entry(base, self, i), label = ReadU32(base, item + pm::kEntryLabel);
    r.disabled[i] = ReadU8(base, item + pm::kEntryDisabled);
    r.visible[i] = ReadU8(base, label + pm::kTextVisibility) & 4;
    ItemVisible(ctx, base, item, false);
  }
  ItemVisible(ctx, base, Entry(base, self, kBindEntry), false);
  for (int i = kVideoFirst; i < kBindEntry; ++i) {
    const int row = i - kVideoFirst;
    const bool shown = controls || row < 4;
    ItemVisible(ctx, base, Entry(base, self, i), shown);
    if (shown) ItemKey(ctx, base, self, i, controls ? kBindLabels[row] : kLabels[row]);
  }
  TextKey(ctx, base, r.title, controls ? "COD3PC_KEYS_TITLE" : "COD3PC_VIDEO_TITLE");
  TextKey(ctx, base, r.note, controls ? "COD3PC_KEYS_HELP" : "COD3PC_VIDEO_RESTART");
  r.active = true;
  PositionPause(base, r);
  if (controls) RefreshBindingPage(ctx, base, self, r);
  else ValuesPause(ctx, base, r, true);
  SelectPause(ctx, base, self, kVideoFirst);
  REXLOG_INFO("COD3_PAUSE_VIDEO_OPEN page={:08X} controls={}", self, controls);
}
void LeavePauseVideo(PPCContext& ctx, uint8_t* base, uint32_t self, PauseVideo& r, bool select) {
  if (!r.active) return;
  cod3::input::CancelCapture();
  const int back = r.controls ? kBindEntry : kVideoEntry;
  for (int i = kVideoFirst; i < kBindEntry; ++i) ItemVisible(ctx, base, Entry(base, self, i), false);
  ItemVisible(ctx, base, Entry(base, self, kBindEntry), true);
  for (int i = 0; i < kVideoFirst; ++i) {
    const auto item = Entry(base, self, i);
    VC(ctx, base, item, pm::kEntrySetDisabled, r.disabled[i] ? 1 : 0);
    VC(ctx, base, item, pm::kEntrySetVisible, r.visible[i] ? 1 : 0);
  }
  TextKey(ctx, base, r.title, ReadString(base, 0x8206FEA4, 64).c_str());
  r.active = r.reopened = false;
  r.controls = false;
  if (select) SelectPause(ctx, base, self, back);
}
}

REX_EXTERN(__imp__sub_824E60E8);
REX_HOOK_RAW(sub_824E60E8) {
  const auto self = ctx.r3.u32;
  __imp__sub_824E60E8(ctx, base);
  if (ReadU32(base, self) != kAdvancedVtable || ReadU16(base, self + pm::kPageCount) != 11) return;
  const auto panel = ReadU32(base, self + 56);
  PauseVideo r;
  r.reset = Find(ctx, base, panel, "Reset2Defaults");
  r.title = Find(ctx, base, panel, "AdvancedOptions");
  r.anchor = {Find(ctx, base, panel, "InvertAim"), Find(ctx, base, panel, "InvertAimOnOff"),
              Find(ctx, base, panel, "arrowL1", true), Find(ctx, base, panel, "arrowR1", true)};
  for (auto e : r.anchor) if (!Valid(e)) return;
  if (!Valid(r.reset) || !Valid(r.title)) return;
  r.entry = Copy(ctx, base, r.reset, false, r);
  r.bind_entry = Copy(ctx, base, r.reset, false, r);
  r.note = Copy(ctx, base, r.reset, false, r);
  bool complete = Valid(r.entry) && Valid(r.note) && Valid(r.bind_entry);
  for (auto& row : r.rows) for (int j = 0; j < 4; ++j) {
    row[j] = Copy(ctx, base, r.anchor[j], j >= 2, r);
    complete &= Valid(row[j]);
  }
  const auto old = ReadU32(base, self + pm::kPageItems);
  if (!complete || !Valid(old)) {
    for (auto e : r.owned) VC(ctx, base, e, 0, 1);
    return;
  }
  r.gauge_source = {Find(ctx, base, panel, "MusicVolProgbar", true),
                     Find(ctx, base, panel, "MusicVolumeProgbar2", true)};
  if (r.gauge_source[0] && r.gauge_source[1]) {
    r.gauge[0] = Copy(ctx, base, r.gauge_source[0], true, r);
    r.gauge[1] = Copy(ctx, base, r.gauge_source[1], true, r);
  }
  const auto items = uint32_t(Call(ctx, base).Invoke(fe::kOperatorNew, {4 * kVideoCount}));
  if (!Valid(items)) { for (auto e : r.owned) VC(ctx, base, e, 0, 1); return; }
  for (int i = 0; i < kVideoCount; ++i) WriteU32(base, items + i * 4, i < 11 ? ReadU32(base, old + i * 4) : 0);
  WriteU32(base, self + pm::kPageItems, items);
  VC(ctx, base, self, fe::kPageAddItem, kVideoEntry, r.entry);
  complete = Valid(Entry(base, self, kVideoEntry));
  for (int i = 0; i < int(r.rows.size()) && complete; ++i) {
    VC(ctx, base, self, fe::kPageAddItem, kVideoFirst + i, r.rows[i][0]);
    complete = Valid(Entry(base, self, kVideoFirst + i));
  }
  if (complete) {
    VC(ctx, base, self, fe::kPageAddItem, kBindEntry, r.bind_entry);
    complete = Valid(Entry(base, self, kBindEntry));
  }
  if (!complete) {
    for (int i = 11; i < kVideoCount; ++i) if (auto e = Entry(base, self, i); Valid(e)) VC(ctx, base, e, 0, 1);
    WriteU32(base, self + pm::kPageItems, old);
    WriteU16(base, self + pm::kPageCount, 11);
    Call(ctx, base).Invoke(fe::kOperatorDelete, {items});
    for (auto e : r.owned) VC(ctx, base, e, 0, 1);
    REXLOG_WARN("COD3_PAUSE_VIDEO_ADD_FAILED rollback=true"); return;
  }
  Call(ctx, base).Invoke(fe::kOperatorDelete, {old});
  ItemKey(ctx, base, self, kVideoEntry, "COD3PC_VIDEO");
  ItemKey(ctx, base, self, kBindEntry, "COD3PC_KEYS");
  for (int i = 0; i < int(r.rows.size()); ++i) {
    ItemKey(ctx, base, self, kVideoFirst + i, i < 4 ? kLabels[i] : kBindLabels[i]);
    ItemVisible(ctx, base, Entry(base, self, kVideoFirst + i), false);
  }
  WriteU16(base, self + pm::kPageCount, kVideoCount);
  for (auto table : {uint32_t{0x82064658}, uint32_t{0x82064670}}) {
    const int count = table == 0x82064658 ? 6 : 12;
    for (int i = 0; i < count; ++i) {
      const auto name = ReadString(base, ReadU32(base, table + i * 4), 64);
      if (auto e = Find(ctx, base, panel, name.c_str(), count == 12); Valid(e)) r.hide.push_back(e);
    }
  }
  for (const char* name : {"HSensProgbar1", "HSensProgbar2", "VSensProgBar1", "VSensProgbar2",
                           "MusicVolProgbar", "MusicVolumeProgbar2"})
    if (auto e = Find(ctx, base, panel, name, true); Valid(e)) r.hide.push_back(e);
  PositionPause(base, r);
  pause_pages.emplace(self, std::move(r));
  REXLOG_INFO("COD3_PAUSE_VIDEO_READY page={:08X} items={} bindings={}", self, kVideoCount, cod3::input::kActionCount);
}

REX_EXTERN(__imp__sub_824CD530);
REX_HOOK_RAW(sub_824CD530) {
  const auto self = ctx.r3.u32;
  const auto it = pause_pages.find(self);
  if (it == pause_pages.end()) { __imp__sub_824CD530(ctx, base); return; }
  auto& r = it->second;
  PositionPause(base, r);
  std::vector<std::pair<uint32_t, uint8_t>> hidden;
  if (r.active) for (auto e : r.hide) {
    const auto flags = ReadU8(base, e + 16);
    if (flags & 4) { hidden.emplace_back(e, flags); WriteU8(base, e + 16, flags & ~4); }
  }
  __imp__sub_824CD530(ctx, base);
  for (auto [e, flags] : hidden) WriteU8(base, e + 16, flags);
  if (r.active) {
    if (!r.controls) DrawDofGauge(ctx, base, r);
    for (int i = 0; i < (r.controls ? cod3::input::BindingPageRows(r.first_binding) : 4); ++i)
      for (int j = 1; j < 4; ++j) VC(ctx, base, r.rows[i][j], 4);
    VC(ctx, base, r.note, 4);
  }
}
REX_EXTERN(__imp__sub_824E6020);
REX_HOOK_RAW(sub_824E6020) {
  const auto self = ctx.r3.u32;
  const auto it = pause_pages.find(self);
  if (it != pause_pages.end() && int(ctx.r4.u32) >= kVideoEntry) {
    if (ctx.r4.u32 == kVideoEntry && !it->second.active) EnterPauseVideo(ctx, base, self, it->second);
    else if (ctx.r4.u32 == kBindEntry && !it->second.active) EnterPauseVideo(ctx, base, self, it->second, true);
    else if (it->second.active && it->second.controls && ctx.r4.u32 >= kVideoFirst && ctx.r4.u32 < kBindEntry)
      cod3::input::CaptureBinding(it->second.first_binding + int(ctx.r4.u32) - kVideoFirst);
    return;
  }
  __imp__sub_824E6020(ctx, base);
}
REX_EXTERN(__imp__sub_824F07B0);
REX_HOOK_RAW(sub_824F07B0) {
  const auto self = ctx.r3.u32;
  if (const auto it = pause_pages.find(self); it != pause_pages.end() && it->second.active) {
    LeavePauseVideo(ctx, base, self, it->second, true); return;
  }
  __imp__sub_824F07B0(ctx, base);
}
REX_EXTERN(__imp__sub_824DC1C0);
REX_HOOK_RAW(sub_824DC1C0) {
  const auto self = ctx.r3.u32;
  if (const auto it = pause_pages.find(self); it != pause_pages.end()) {
    const int selected = Selected(base, self);
    if (it->second.active) {
      const int row = selected - kVideoFirst;
      if (it->second.controls) {
        if (row >= 0 && row < cod3::input::BindingPageRows(it->second.first_binding))
          cod3::input::CaptureBinding(it->second.first_binding + row);
        return;
      }
      if (row >= 0 && row < 4) {
        const auto flag = cod3::video::kFlags[row];
        const auto value = cod3::video::Step(row, rex::cvar::GetFlagByName(flag), ctx.r4.u32 == 1);
        if (rex::cvar::SetFlagByName(flag, value)) {
          save_requested = true; ValuesPause(ctx, base, it->second, false);
          REXLOG_INFO("COD3_VIDEO_SETTING pause=true {}={}", flag, value);
        }
      }
      return;
    }
    if (selected == kVideoEntry || selected == kBindEntry) {
      SelectPause(ctx, base, self, selected == kVideoEntry ? kBindEntry : kVideoEntry); return;
    }
    if (selected >= kVideoEntry) return;
  }
  __imp__sub_824DC1C0(ctx, base);
}
REX_EXTERN(__imp__sub_824C41B8);
REX_HOOK_RAW(sub_824C41B8) {
  const auto self = ctx.r3.u32;
  if (const auto it = pause_pages.find(self); it != pause_pages.end()) {
    const int sel = Selected(base, self);
    if (it->second.active) { MovePauseSelection(ctx, base, self, it->second, sel, true); return; }
    if (sel == 8 || sel == kVideoEntry || sel == kBindEntry) {
      SelectPause(ctx, base, self, sel == 8 ? kVideoEntry : sel == kVideoEntry ? kBindEntry : 9); return;
    }
  }
  __imp__sub_824C41B8(ctx, base);
}
REX_EXTERN(__imp__sub_824C4218);
REX_HOOK_RAW(sub_824C4218) {
  const auto self = ctx.r3.u32;
  if (const auto it = pause_pages.find(self); it != pause_pages.end()) {
    const int sel = Selected(base, self);
    if (it->second.active) { MovePauseSelection(ctx, base, self, it->second, sel, false); return; }
    if (sel == 9 || sel == kVideoEntry || sel == kBindEntry) {
      SelectPause(ctx, base, self, sel == 9 ? kBindEntry : sel == kBindEntry ? kVideoEntry : 8); return;
    }
  }
  __imp__sub_824C4218(ctx, base);
}
REX_EXTERN(__imp__sub_824EE128);
REX_HOOK_RAW(sub_824EE128) {
  const auto self = ctx.r3.u32;
  __imp__sub_824EE128(ctx, base);
  if (const auto it = pause_pages.find(self); it != pause_pages.end()) {
    auto& r = it->second;
    if (r.active && r.reopened) { LeavePauseVideo(ctx, base, self, r, false); SelectPause(ctx, base, self, 0); }
    PositionPause(base, r);
    if (r.active) ValuesPause(ctx, base, r, false);
  }
}
REX_EXTERN(__imp__sub_824E6588);
REX_HOOK_RAW(sub_824E6588) {
  const auto self = ctx.r3.u32;
  if (const auto it = pause_pages.find(self); it != pause_pages.end() && it->second.active && it->second.reopened)
    LeavePauseVideo(ctx, base, self, it->second, false);
  __imp__sub_824E6588(ctx, base);
  if (const auto it = pause_pages.find(self); it != pause_pages.end() && it->second.active)
    ValuesPause(ctx, base, it->second, true);
}
REX_EXTERN(__imp__sub_824DC640);
REX_HOOK_RAW(sub_824DC640) {
  const auto self = ctx.r3.u32;
  std::vector<uint32_t> owned;
  if (const auto it = pause_pages.find(self); it != pause_pages.end()) {
    owned = std::move(it->second.owned); pause_pages.erase(it);
  }
  __imp__sub_824DC640(ctx, base);
  for (auto e : owned) VC(ctx, base, e, 0, 1);
}

REX_EXTERN(__imp__sub_824F0B50);
REX_HOOK_RAW(sub_824F0B50) {
  if (ctx.r4.u32 == 1) {
    for (auto& [page, state] : pause_pages) if (state.active) state.reopened = true;
  }
  __imp__sub_824F0B50(ctx, base);
}

namespace cod3::video {
bool SetSlider(uint8_t* base, uint32_t page, int item, float fraction) {
  const auto it = pause_pages.find(page);
  if (it == pause_pages.end() || !it->second.active || it->second.controls || item != kVideoFirst ||
      Selected(base, page) != item || !it->second.gauge[0] || !std::isfinite(fraction)) return false;
  CancelNumber();
  SetDof(cod3::menu::PercentEdit::FromFraction(fraction));
  return true;
}
}
