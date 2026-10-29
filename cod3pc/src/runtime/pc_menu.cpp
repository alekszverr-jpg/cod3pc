// Adapted from SirCarls/cod3_REcomp_, d54c1bb877ab364dae2d2f40aa8c6a8ca46e3070.
// Copyright (c) 2026 the COD_3_WAW contributors. MIT license:
// licenses/SirCarls-COD_3_WAW-MIT.txt. See docs/PC_MENUS.md for adaptation.
// cod3 - PC controls for the game's own menus (see pc_menu.h).

#include "pc_menu.h"
#include "menu_picture.h"
#include "menu_prompts.h"
#include "native_video_menu.h"
#include "menu_number.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <unordered_map>

#include <rex/cvar.h>
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/hook.h>
#include <rex/kernel/xboxkrnl/video.h>
#include <rex/logging.h>
#include <rex/system/xvideo.h>
#include <rex/ui/flags.h>
#include <rex/ui/ui_event.h>
#include <rex/ui/virtual_key.h>
#include <rex/ui/window.h>
#include <rex/ui/window_listener.h>

#include "pc_menu_addresses.h"
#include "menu_guest.h"

REXCVAR_DEFINE_BOOL(cod3_pc_menus, true, "cod3/Mouse",
                    "Mouse and keyboard in the game's menus: the pointer selects, click to choose, "
                    "right click or Escape to go back, wheel and arrow keys to move, Enter to "
                    "choose (game/pc_menu.h)");

REXCVAR_DEFINE_BOOL(cod3_menu_trace, false, "cod3/Mouse",
                    "Log every change of a game menu's highlight and what made it (for bug reports)");



namespace {

namespace pm = cod3::addr::pc_menu;
using cod3::guest::Call;
using cod3::guest::ReadF32;
using cod3::guest::ReadU16;
using cod3::guest::ReadU32;
using cod3::guest::ReadU8;
using cod3::guest::VirtualSlot;
using cod3::guest::WriteU32;
using Clock = std::chrono::steady_clock;

int64_t Now() { return Clock::now().time_since_epoch().count(); }
constexpr int64_t Ms(int64_t ms) {
  return std::chrono::duration_cast<Clock::duration>(std::chrono::milliseconds(ms)).count();
}

int16_t ReadS16(uint8_t* base, uint32_t addr) { return static_cast<int16_t>(ReadU16(base, addr)); }
void WriteU16(uint8_t* base, uint32_t addr, uint16_t value) {  // big-endian like all guest data
  base[addr] = uint8_t(value >> 8);
  base[addr + 1] = uint8_t(value);
}

// A guest heap or image pointer (never null, never unaligned).
bool Valid(uint32_t p) { return p >= 0x10000 && p < 0xC0000000u && (p & 3) == 0; }

// ---------------------------------------------------------------------------
// When a menu is up: a menu page took input in this game frame or the last
// one (the game's frame counter, pm::kFrameCount). Counting frames, not
// time, keeps it right at any frame rate, also at the cloud's 1-5 s software
// rendered frames. While the game thread is busy (a loading screen) the
// counter stands still, so a menu older than kLoadingLimit no longer counts.
// ---------------------------------------------------------------------------
constexpr int64_t kLoadingLimit = Ms(20000);

std::atomic<uint32_t> g_current_frame{0};  // published by guest hooks; no UI guest reads
std::atomic<uint32_t> g_menu_frame{0};      // frame in which a menu page last took input
std::atomic<int64_t> g_menu_time{0};        // and when (0 = never)
std::atomic<uint32_t> g_dialog_frame{0};    // same for a message box
std::atomic<bool> g_dialog_seen{false};

uint32_t FrameNow() {
  return g_current_frame.load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// What the mouse and keys ask for, from the UI thread to the game thread.
// ---------------------------------------------------------------------------
enum class Ask {
  kButton,   // a button code (pm::kCode*), pressed and let go
  kHold,     // an arrow key pressed and held: the page repeats it like a held D-pad
  kRelease,  // that arrow key let go
  kAccept,   // A, or Start on the title screen
  kClick,    // left click at (x, y): the item there, or its value
};

struct Request {
  Ask ask = Ask::kButton;
  uint32_t code = 0;
  float x = 0.0f, y = 0.0f;
  uint32_t frame = 0;  // the game frame it was asked in
};

struct Pointer {
  std::mutex mutex;
  bool over = false;        // over the picture
  float x = 0.0f, y = 0.0f; // in the game's 640x480 menu space
  uint32_t moves = 0;       // counts motion events
  bool left_down = false;   // [menus-brightness] the left button is held (dragging a gauge)
  std::deque<Request> requests;
};
Pointer g_input;
constexpr size_t kMaxRequests = 8;

void PushLocked(Request r) {  // g_input.mutex held
  r.frame = FrameNow();
  if (g_input.requests.size() < kMaxRequests) g_input.requests.push_back(r);
}

void Push(Request r) {
  std::lock_guard lock(g_input.mutex);
  PushLocked(r);
}

// The arrow key held down (UI thread writes, game thread may let it go).
// The game repeats a held direction itself (the page keeps it until the
// release), so an arrow key is a press when it goes down and a release when
// it comes up, exactly like the D-pad; the OS's own key repeat is not used.
// If no key-up or OS repeat is seen for kHeldTimeout (focus lost on the
// way), the game thread lets it go.
struct HeldArrow {
  uint32_t code = 0;
  int64_t last_seen = 0;  // the key-down or its last OS repeat
};
HeldArrow g_held;  // g_input.mutex
constexpr int64_t kHeldTimeout = Ms(1500);

// ---------------------------------------------------------------------------
// Window pixels -> the game's 640x480 menu space.
// The presenter shows the picture at the guest display's shape with black
// bars around it (or stretched, or cropped, by its present_* settings). That
// shape is what VdQueryVideoMode reports: video_mode_width x _height (which
// game/ultrawide.cpp sets for wide pictures), else window_width x _height or
// the resolution preset when those are set, else 1280x720. This adaptation
// follows our existing presentation without importing ultrawide projection.
// ---------------------------------------------------------------------------
std::atomic<uint32_t> g_display_width{1280}, g_display_height{720};

void RefreshPictureAspect() {
  rex::system::X_VIDEO_MODE mode;
  rex::kernel::xboxkrnl::VdQueryVideoMode(&mode);
  g_display_width.store(std::max<uint32_t>(1, mode.display_width), std::memory_order_relaxed);
  g_display_height.store(std::max<uint32_t>(1, mode.display_height), std::memory_order_relaxed);
}

// Where the picture lands in a window of ww x wh pixels: the same sums as
// the SDK's Presenter::GetGuestOutputPaintFlow.
using cod3::pcmenu::PictureRect;
PictureRect PictureInWindow(float ww, float wh) {
  return cod3::pcmenu::PictureInWindow(ww, wh,
      float(g_display_width.load(std::memory_order_relaxed)),
      float(g_display_height.load(std::memory_order_relaxed)),
      REXCVAR_GET(present_letterbox), REXCVAR_GET(present_allow_overscan_cutoff),
      REXCVAR_GET(present_safe_area_x), REXCVAR_GET(present_safe_area_y));
}

bool ToMenuSpace(rex::ui::Window* window, int32_t px, int32_t py, float* vx, float* vy) {
  const float ww = float(window->GetActualPhysicalWidth());
  const float wh = float(window->GetActualPhysicalHeight());
  if (ww < 1.0f || wh < 1.0f) return false;
  const PictureRect pic = PictureInWindow(ww, wh);
  return cod3::pcmenu::MenuPoint(pic, float(px), float(py), *vx, *vy);
}

// ---------------------------------------------------------------------------
// UI thread: the window's mouse and keyboard while a menu is up.
// ---------------------------------------------------------------------------
class MenuInput final : public rex::ui::WindowInputListener, public rex::ui::WindowListener {
 public:
  rex::ui::Window* window = nullptr;
  std::function<bool()> overlay_open;

  void OnLostFocus(rex::ui::UISetupEvent&) override;
  void OnKeyDown(rex::ui::KeyEvent& e) override;
  void OnKeyUp(rex::ui::KeyEvent& e) override;
  void OnMouseMove(rex::ui::MouseEvent& e) override;
  void OnMouseDown(rex::ui::MouseEvent& e) override;
  void OnMouseUp(rex::ui::MouseEvent& e) override;  // [menus-brightness]
  void OnMouseWheel(rex::ui::MouseEvent& e) override;

 private:
  bool Usable() const {
    return cod3::pcmenu::MenuActive() && !(overlay_open && overlay_open());
  }
  int wheel_ = 0;
};
MenuInput g_listener;

void MenuInput::OnKeyDown(rex::ui::KeyEvent& e) {
  if (!e.prev_state()) cod3::menuprompts::KeyboardUsed();
  using rex::ui::VirtualKey;
  if (e.is_alt_pressed() || e.is_ctrl_pressed() || e.is_super_pressed() || !Usable()) return;
  const auto key = e.virtual_key();
  int number_key = cod3::menu::PercentEdit::kOther;
  if (key >= VirtualKey::k0 && key <= VirtualKey::k9)
    number_key = int(key) - int(VirtualKey::k0);
  else if (key >= VirtualKey::kNumpad0 && key <= VirtualKey::kNumpad9)
    number_key = int(key) - int(VirtualKey::kNumpad0);
  else if (key == VirtualKey::kReturn) number_key = cod3::menu::PercentEdit::kEnter;
  else if (key == VirtualKey::kEscape) number_key = cod3::menu::PercentEdit::kEscape;
  else if (key == VirtualKey::kBack) number_key = cod3::menu::PercentEdit::kBackspace;
  if (cod3::video::NumberKey(number_key)) {
    e.set_handled(true); return;
  }
  Request r;
  bool repeats = false;
  switch (e.virtual_key()) {
    case VirtualKey::kUp: r.code = pm::kCodeUp, repeats = true; break;
    case VirtualKey::kDown: r.code = pm::kCodeDown, repeats = true; break;
    case VirtualKey::kLeft: r.code = pm::kCodeLeft, repeats = true; break;
    case VirtualKey::kRight: r.code = pm::kCodeRight, repeats = true; break;
    case VirtualKey::kReturn: r.ask = Ask::kAccept; break;
    case VirtualKey::kEscape:
    case VirtualKey::kBack: r.code = pm::kCodeB; break;
    default: return;
  }
  // Ours alone: for the keyboard/mouse driver Enter and Escape are Start and
  // the arrows the right stick.
  e.set_handled(true);
  std::lock_guard lock(g_input.mutex);
  if (repeats) {
    // A held arrow: the game's own repeat (see HeldArrow); the OS's repeats
    // only say it is still held.
    if (e.prev_state()) {
      if (g_held.code == r.code) g_held.last_seen = Now();
      return;
    }
    if (g_held.code && g_held.code != r.code) PushLocked({Ask::kRelease, g_held.code});
    r.ask = Ask::kHold;
    g_held = {r.code, Now()};
    PushLocked(r);
    return;
  }
  // A held Enter or Escape (the Escape that just opened the pause menu) acts
  // once.
  if (!e.prev_state()) PushLocked(r);
}

void MenuInput::OnKeyUp(rex::ui::KeyEvent& e) {
  using rex::ui::VirtualKey;
  uint32_t code = 0;
  switch (e.virtual_key()) {
    case VirtualKey::kUp: code = pm::kCodeUp; break;
    case VirtualKey::kDown: code = pm::kCodeDown; break;
    case VirtualKey::kLeft: code = pm::kCodeLeft; break;
    case VirtualKey::kRight: code = pm::kCodeRight; break;
    default: return;
  }
  // Not handled: whoever else saw the key go down sees it come up too.
  std::lock_guard lock(g_input.mutex);
  if (g_held.code != code) return;
  g_held.code = 0;
  PushLocked({Ask::kRelease, code});
}

void MenuInput::OnMouseMove(rex::ui::MouseEvent& e) {
  // Over F2 or the console the pointer isn't pointing at the menu behind.
  if (!window || (overlay_open && overlay_open())) return;
  float x = 0.0f, y = 0.0f;
  const bool over = ToMenuSpace(window, e.x(), e.y(), &x, &y);
  std::lock_guard lock(g_input.mutex);
  if (Usable() && over && (std::abs(x - g_input.x) + std::abs(y - g_input.y) >= 3.0f))
    cod3::menuprompts::KeyboardUsed();
  g_input.over = over;
  if (over) g_input.x = x, g_input.y = y;
  ++g_input.moves;
}

void MenuInput::OnMouseDown(rex::ui::MouseEvent& e) {
  if (!window || !Usable()) return;
  cod3::menuprompts::KeyboardUsed();
  using Button = rex::ui::MouseEvent::Button;
  if (e.button() == Button::kLeft) {
    Request r;
    r.ask = Ask::kClick;
    std::lock_guard lock(g_input.mutex);
    g_input.left_down = true;  // [menus-brightness]
    if (ToMenuSpace(window, e.x(), e.y(), &r.x, &r.y)) PushLocked(r);
  } else if (e.button() == Button::kRight) {
    Request r;
    r.code = pm::kCodeB;
    Push(r);
  } else {
    return;
  }
  // Not also a trigger pull for the keyboard/mouse driver.
  e.set_handled(true);
}

// [menus-brightness] Ends a gauge drag. Not handled: others see it too.
void MenuInput::OnMouseUp(rex::ui::MouseEvent& e) {
  if (e.button() != rex::ui::MouseEvent::Button::kLeft) return;
  std::lock_guard lock(g_input.mutex);
  g_input.left_down = false;
}

void MenuInput::OnMouseWheel(rex::ui::MouseEvent& e) {
  if (!Usable()) return;
  cod3::menuprompts::KeyboardUsed();
  wheel_ += e.scroll_y();
  const int detent = rex::ui::MouseEvent::kScrollPerDetent;
  for (; wheel_ >= detent; wheel_ -= detent) Push({Ask::kButton, pm::kCodeUp});
  for (; wheel_ <= -detent; wheel_ += detent) Push({Ask::kButton, pm::kCodeDown});
  e.set_handled(true);
}

// ---------------------------------------------------------------------------
// Game thread: the page on screen and its items.
// ---------------------------------------------------------------------------
// obj->method(a1, a2) through the object's vtable.
uint64_t VCall(PPCContext& ctx, uint8_t* base, uint32_t obj, uint32_t slot, uint64_t a1 = 0,
               uint64_t a2 = 0) {
  Call c(ctx, base);
  return c.Invoke(VirtualSlot(base, obj, slot), {obj, a1, a2});
}

// Every menu class's vtable lies in this range (FEMenu 0x82070EDC ...
// ChapterSelectMenu 0x82072A8C); anything else isn't a page.
bool IsPage(uint8_t* base, uint32_t page) {
  if (!Valid(page)) return false;
  const uint32_t vt = ReadU32(base, page);
  return vt >= 0x82070E00 && vt < 0x82072C00;
}

// The page the menu system shows now, or 0.
uint32_t CurrentPage(PPCContext& ctx, uint8_t* base, uint32_t system) {
  const uint32_t slot = uint32_t(VCall(ctx, base, system, pm::kSysSlot));
  if (slot > 3) return 0;
  const int32_t index = static_cast<int32_t>(ReadU32(base, system + pm::kSysCurrent + 4 * slot));
  if (index < 0 || index > 64) return 0;
  const uint32_t pages = ReadU32(base, system + pm::kSysPages);
  if (!Valid(pages)) return 0;
  const uint32_t page = ReadU32(base, pages + 4 * uint32_t(index));
  return IsPage(base, page) ? page : 0;
}

struct Box {
  float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  float cx() const { return (x0 + x1) * 0.5f; }
  float cy() const { return (y0 + y1) * 0.5f; }
  bool Contains(float x, float y) const { return x >= x0 && x <= x1 && y >= y0 && y <= y1; }
};

// [menus-brightness] The arrows and the gauge drawn on a row (or a page's own,
// not on a row: Options > SCREEN), from the pictures the menu draws (see
// "Arrows and gauges" below).
struct RowControls {
  bool left = false, right = false, gauge = false, fill = false;
  Box l, r;  // the left and right arrow
  Box g, f;  // the gauge's frame (its whole range) and its fill
};

struct Item {
  int index = 0;
  Box ink;                    // the label's letters as drawn
  Box hit;                    // what the pointer selects it in
  bool left_aligned = false;  // label on the left, a value to its right
  RowControls ctl;            // [menus-brightness] its arrows and gauge
};

// ---------------------------------------------------------------------------
// [menus-brightness] Arrows and gauges. Every picture drawn inside a menu
// system's draw goes through the picture draw (PanelQuad__vfunc_1, hooked at
// the end); the ones named as a left / right arrow (arrowL1, arrowR6,
// slot_05_arrow_left, gauge_arrow_right ...) or a gauge (slot_01_gauge_back,
// slot_01_gauge_fill, HSensProgbar1 / 2, MusicVolProgbar ...) are noted with
// where they were drawn, like the footer prompts. The port's own rows draw
// copies of the game's pictures, with their names, so they are found too.
// A row gets the ones drawn on its line and its hit area grows to cover
// them (the arrows lie right of the labels' column on most pages).
// ---------------------------------------------------------------------------
enum class ControlKind : uint8_t { kLeft, kRight, kGauge, kFill };

struct Control {
  uint32_t system = 0;  // the menu system that drew it
  uint32_t quad = 0;
  uint32_t frame = 0;   // the game frame it was drawn in
  ControlKind kind = ControlKind::kLeft;
  Box box;
};

std::mutex g_control_mutex;
std::vector<Control> g_controls;  // g_control_mutex
constexpr float kControlMargin = 4.0f;  // menu units a click may miss a picture by

// What a picture's name says it is; false: neither an arrow nor a gauge.
bool ControlKindOf(std::string name, ControlKind* kind) {
  for (char& c : name) c = char(std::tolower(static_cast<unsigned char>(c)));
  auto has = [&name](const char* s) { return name.find(s) != std::string::npos; };
  auto numbered = [&name](const char* prefix) {  // arrowL1 .. arrowR7
    const size_t n = std::strlen(prefix);
    return name.size() > n && name.compare(0, n, prefix) == 0 && std::isdigit(static_cast<unsigned char>(name[n]));
  };
  if (has("arrow")) {
    if (has("left") || numbered("arrowl")) return *kind = ControlKind::kLeft, true;
    if (has("right") || numbered("arrowr")) return *kind = ControlKind::kRight, true;
    return false;  // up / down and the HUD's arrows
  }
  if (has("gauge") || has("progbar")) {
    const bool fill = has("fill") || (name.size() >= 8 && name.compare(name.size() - 8, 8, "progbar2") == 0);
    *kind = fill ? ControlKind::kFill : ControlKind::kGauge;
    return true;
  }
  return false;
}

// The rectangle a picture covers (its parts' vertices), in menu space.
bool QuadBox(uint8_t* base, uint32_t quad, Box* out) {
  namespace fe = cod3::addr::fe;
  const uint32_t parts = ReadU32(base, quad + fe::kQuadParts), n = ReadU32(base, quad + fe::kQuadPartCount);
  if (!Valid(parts) || n == 0 || n > 8) return false;
  Box b{1e9f, 1e9f, -1e9f, -1e9f};
  for (uint32_t p = 0; p < n; ++p) {
    const uint32_t part = ReadU32(base, parts + 4 * p);
    for (uint32_t v = 0; Valid(part) && v < 4; ++v) {
      const uint32_t at = part + fe::kQuadPartVerts + v * fe::kQuadVertStride;
      const float x = ReadF32(base, at), y = ReadF32(base, at + 4);
      if (!std::isfinite(x) || !std::isfinite(y)) return false;
      b.x0 = std::min(b.x0, x), b.x1 = std::max(b.x1, x);
      b.y0 = std::min(b.y0, y), b.y1 = std::max(b.y1, y);
    }
  }
  if (b.x0 > b.x1 || b.y0 > b.y1 || b.x1 - b.x0 > 640.0f || b.y1 - b.y0 > 480.0f) return false;
  *out = b;
  return true;
}

// What `system` drew in the last two frames.
std::vector<Control> DrawnControls(uint32_t system) {
  const uint32_t frame = FrameNow();
  std::vector<Control> out;
  std::lock_guard lock(g_control_mutex);
  for (const auto& c : g_controls)
    if (c.system == system && frame - c.frame <= 2) out.push_back(c);
  return out;
}

Box Union(const Box& a, const Box& b) {
  return Box{std::min(a.x0, b.x0), std::min(a.y0, b.y0), std::max(a.x1, b.x1), std::max(a.y1, b.y1)};
}

void AddControl(RowControls& rc, const Control& c) {
  auto add = [&c](bool& has, Box& box) {
    box = has ? Union(box, c.box) : c.box;
    has = true;
  };
  switch (c.kind) {
    case ControlKind::kLeft: add(rc.left, rc.l); break;
    case ControlKind::kRight: add(rc.right, rc.r); break;
    case ControlKind::kGauge: add(rc.gauge, rc.g); break;
    case ControlKind::kFill: add(rc.fill, rc.f); break;
  }
}

// Each control goes to the row whose hit area holds its middle (the nearest
// row's letters if two do), else to `loose` (the page's own).
void AttachControls(std::vector<Item>& items, const std::vector<Control>& controls, RowControls* loose) {
  for (const Control& c : controls) {
    Item* row = nullptr;
    float best = 1e9f;
    for (auto& it : items) {
      if (c.box.cy() < it.hit.y0 || c.box.cy() > it.hit.y1) continue;
      const float d = std::abs(c.box.cy() - it.ink.cy());
      if (d < best) best = d, row = &it;
    }
    if (!row) {
      if (loose) AddControl(*loose, c);
      continue;
    }
    AddControl(row->ctl, c);
    row->hit.x0 = std::min(row->hit.x0, c.box.x0 - kControlMargin);
    row->hit.x1 = std::max(row->hit.x1, c.box.x1 + kControlMargin);
  }
}

bool NearBox(const Box& b, float x, float y, float margin) {
  // Thin pictures (a gauge's bar) get at least a row's height around them.
  const float grow_y = std::max(margin, (12.0f - (b.y1 - b.y0)) * 0.5f);
  return x >= b.x0 - margin && x <= b.x1 + margin && y >= b.y0 - grow_y && y <= b.y1 + grow_y;
}

// The arrow under (x, y): its button code (left / right), else 0.
uint32_t ArrowAt(const RowControls& rc, float x, float y) {
  const bool l = rc.left && NearBox(rc.l, x, y, kControlMargin);
  const bool r = rc.right && NearBox(rc.r, x, y, kControlMargin);
  if (l && r) return std::abs(x - rc.l.cx()) <= std::abs(x - rc.r.cx()) ? pm::kCodeLeft : pm::kCodeRight;
  return l ? pm::kCodeLeft : r ? pm::kCodeRight : 0;
}

bool GaugeAt(const RowControls& rc, float x, float y) {
  return rc.gauge && rc.g.x1 - rc.g.x0 >= 4.0f && NearBox(rc.g, x, y, 3.0f);
}

// For the page log: " L x0,y0-x1,y1 R ... G ... F ..." (arrows, gauge, fill).
std::string DescribeControls(const RowControls& rc) {
  std::string s;
  auto add = [&s](bool has, const char* tag, const Box& b) {
    if (has) s += fmt::format(" {} {:.0f},{:.0f}-{:.0f},{:.0f}", tag, b.x0, b.y0, b.x1, b.y1);
  };
  add(rc.left, "L", rc.l);
  add(rc.right, "R", rc.r);
  add(rc.gauge, "G", rc.g);
  add(rc.fill, "F", rc.f);
  return s;
}

bool IsText(uint8_t* base, uint32_t elem) {
  const uint32_t vt = ReadU32(base, elem);
  return vt == pm::kTextVtable || vt == pm::kMultiLineTextVtable;
}

// The font a text draws with (FEText::Draw), or 0.
uint32_t TextFont(uint8_t* base, int32_t index) {
  if (index < 0 || index >= int32_t(cod3::addr::prompts::kFontCount)) return 0;
  const uint32_t font = ReadU32(base, cod3::addr::prompts::kFonts + 4 * uint32_t(index));
  return Valid(font) && ReadU32(base, font + pm::kFontTexture) ? font : 0;
}

// A glyph of the font (clamped to its first / last, as the game does).
uint32_t Glyph(uint8_t* base, uint32_t font, uint8_t c) {
  const int32_t first = int32_t(ReadU32(base, font + pm::kFontFirstChar));
  const int32_t count = int32_t(ReadU32(base, font + pm::kFontGlyphCount));
  const uint32_t glyphs = ReadU32(base, font + pm::kFontGlyphs);
  if (count <= 0 || !Valid(glyphs)) return 0;
  const int32_t i = std::clamp<int32_t>(int32_t(c) - first, 0, count - 1);
  return glyphs + pm::kGlyphSize * uint32_t(i);
}

int32_t GlyphField(uint8_t* base, uint32_t glyph, int field) {
  return int32_t(ReadU32(base, glyph + 4 * uint32_t(field)));
}

// One line of text laid out like the game's measure (0x82134B90) and draw:
// its width at scale `sx` (to the last letter's right edge) and the top and
// bottom of its letters at scale `sy`, below the line's top. False for text
// with control codes (colors, line breaks) or nothing to draw.
struct TextMetrics {
  float width = 0, height = 0;    // what the game measures (alignment)
  float ink_top = 0, ink_bottom = 0;
};

bool MeasureText(uint8_t* base, uint32_t font, const std::string& text, float sx, float sy,
                 TextMetrics* out) {
  if (!font || text.empty()) return false;
  float pen = 0.0f, top = 1e9f, bottom = -1e9f;
  uint32_t last = 0;
  for (char ch : text) {
    const uint8_t c = uint8_t(ch);
    if (c < 32) return false;
    // A space moves on like an 'a' and draws nothing.
    const uint32_t g = Glyph(base, font, c == ' ' ? uint8_t('a') : c);
    if (!g) return false;
    pen += float(GlyphField(base, g, 6)) * sx;
    last = g;
    if (c == ' ') continue;
    const float y_off = float(GlyphField(base, g, 5)), h = float(GlyphField(base, g, 3));
    top = std::min(top, y_off * sy);
    bottom = std::max(bottom, (y_off + h) * sy);
  }
  if (last) pen += float(GlyphField(base, last, 4) + GlyphField(base, last, 2) - GlyphField(base, last, 6)) * sx;
  out->width = std::trunc(pen);
  out->height = std::trunc(float(int32_t(ReadU32(base, font + pm::kFontLineHeight))) * sx);
  if (top > bottom) top = 0.0f, bottom = out->height;
  out->ink_top = top, out->ink_bottom = bottom;
  return out->width > 0.0f;
}

std::string GuestString(uint8_t* base, uint32_t string, size_t max_len = 256) {
  return Valid(string) ? cod3::guest::ReadString(base, string + pm::kStringChars, max_len) : std::string();
}

// Where a text element's letters are drawn (FEText::Draw 0x824CFD58): at its
// x / y moved by its alignment (0x824BF4A8) with the size the game measures,
// at the scale of its state (selected or not). False when it isn't drawn.
bool DrawnText(PPCContext& ctx, uint8_t* base, uint32_t text, bool selected, Box* out) {
  if (!Valid(text) || !IsText(base, text)) return false;
  if (!(ReadU8(base, text + pm::kTextVisibility) & 0x4)) return false;
  float x = ReadF32(base, text + pm::kTextX);
  float y = ReadF32(base, text + pm::kTextY);
  const uint32_t scale = text + (selected ? pm::kTextScaleSelected : pm::kTextScaleNormal);
  const float sx = ReadF32(base, scale), sy = ReadF32(base, scale + 4);
  if (!std::isfinite(x) || !std::isfinite(y) || !(sx > 0.0f && sx < 20.0f) || !(sy > 0.0f && sy < 20.0f))
    return false;
  TextMetrics m;
  const uint32_t font = TextFont(base, int32_t(ReadU32(base, text + pm::kTextFont)));
  if (!MeasureText(base, font, GuestString(base, ReadU32(base, text + pm::kTextString)), sx, sy, &m)) {
    // Not a plain line (a multi-line text, control codes): the game's own
    // measure of the box (at the selected scale).
    Call c(ctx, base);
    c.Invoke(VirtualSlot(base, text, pm::kTextWidth), {text, 0});
    m.width = float(c.ResultF1());
    c.Invoke(VirtualSlot(base, text, pm::kTextHeight), {text, 0});
    m.height = float(c.ResultF1());
    m.ink_top = 0.0f, m.ink_bottom = m.height;
  }
  if (!(m.width > 0.0f && m.width < 2000.0f && m.height > 0.0f && m.height < 1000.0f)) return false;
  const uint16_t align = ReadU16(base, text + pm::kTextAlign);
  if (align & pm::kAlignRight)
    x -= m.width;
  else if (!(align & pm::kAlignLeft))
    x -= m.width * 0.5f;
  if (align & pm::kAlignBottom)
    y -= m.height;
  else if (!(align & pm::kAlignTop))
    y -= m.height * 0.5f;
  *out = Box{x, y + m.ink_top, x + m.width, y + m.ink_bottom};
  return true;
}

// A message box's button list (msgbox::kSysButtons of its DialogMenuSystem)
// and the slot item `i` of `count` is drawn in (see addresses.h).
uint32_t DialogButtons(uint8_t* base, uint32_t system) {
  if (!Valid(system) || ReadU32(base, system) != pm::kDialogSystemVtable) return 0;
  const uint32_t buttons = ReadU32(base, system + cod3::addr::msgbox::kSysButtons);
  return Valid(buttons) ? buttons : 0;
}

int DialogSlot(uint8_t* base, uint32_t buttons, int i) {
  const uint32_t count = ReadU32(base, buttons + pm::kButtonCount);
  if (count == 1) return 2;
  if (count == 4) return i;
  const bool middle = ReadU32(base, buttons + pm::kButtonSlots + 4) && ReadU32(base, buttons + pm::kButtonSlots + 8);
  return middle ? i + 1 : i;
}

// Hit areas from the drawn letters. Items on one line (a message box's
// buttons side by side) split it half way between them; rows of a list span
// the list's column (on a page of settings a left-aligned column gets its
// mirror image on the right, where the values are) and reach half way to the
// next row, at most about a row's height.
void LayOutHitAreas(std::vector<Item>& items, bool has_values) {
  if (items.empty()) return;
  float left = 640.0f, right = 0.0f, line = 0.0f;
  bool any_left = false;
  for (const auto& it : items) {
    left = std::min(left, it.ink.x0);
    right = std::max(right, it.ink.x1);
    line = std::max(line, it.ink.y1 - it.ink.y0);
    any_left |= it.left_aligned;
  }
  if (any_left && has_values) right = std::max(right, 640.0f - left);
  // Short words ("Yes", "No") get a column of a usable width around them.
  constexpr float kMinColumn = 80.0f;
  if (right - left < kMinColumn) {
    const float grow = (kMinColumn - (right - left)) * 0.5f;
    left -= any_left ? 0.0f : grow;
    right += any_left ? 2.0f * grow : grow;
  }
  const float reach = std::max(line, 8.0f);  // farthest a hit area reaches past its letters
  auto same_line = [line](const Item& a, const Item& b) {
    return std::abs(a.ink.cy() - b.ink.cy()) < line * 0.5f;
  };
  for (auto& it : items) {
    float top = it.ink.y0 - reach * 0.5f, bottom = it.ink.y1 + reach * 0.5f;
    float x0 = left - 6.0f, x1 = right + 6.0f;
    bool shares_line = false;
    for (const auto& o : items) {
      if (&o == &it) continue;
      if (same_line(o, it)) {
        // Side by side: up to half way to the neighbor.
        if (!shares_line) x0 = it.ink.x0 - reach, x1 = it.ink.x1 + reach;
        shares_line = true;
        const float mid = o.ink.cx() > it.ink.cx() ? (it.ink.x1 + o.ink.x0) * 0.5f : (o.ink.x1 + it.ink.x0) * 0.5f;
        if (o.ink.cx() > it.ink.cx()) x1 = std::min(x1, mid);
        else x0 = std::max(x0, mid);
        continue;
      }
      const float mid = (o.ink.cy() + it.ink.cy()) * 0.5f;
      if (o.ink.cy() > it.ink.cy()) bottom = std::min(bottom, mid);
      else top = std::max(top, mid);
    }
    it.hit = Box{x0, top, x1, bottom};
  }
}

// A page of settings: it takes left / right (its rows have values).
bool HasValues(uint8_t* base, uint32_t page) {
  const uint32_t on_left = VirtualSlot(base, page, pm::kPageOnLeft);
  return on_left != pm::kFEMenuOnLeft && on_left != pm::kNullsub;
}

// Where each selectable item of the page is drawn and can be pointed at, in
// menu space. `system` is the page's menu system (a message box draws its
// buttons from its system's button list, not from the page). [menus-brightness]
// With the arrows and gauges drawn on each row; `loose` gets the page's own
// (drawn on no row).
std::vector<Item> ItemRects(PPCContext& ctx, uint8_t* base, uint32_t page, uint32_t system,
                            RowControls* loose = nullptr) {
  std::vector<Item> items;
  const int count = ReadS16(base, page + pm::kPageCount);
  const uint32_t list = ReadU32(base, page + pm::kPageItems);
  if (count <= 0 || count > 64 || !Valid(list)) {
    // [menus-brightness] A page's own slider (only a page that takes left /
    // right: a page sliding out may still draw its arrows for a moment).
    if (loose && HasValues(base, page)) AttachControls(items, DrawnControls(system), loose);
    return items;
  }
  // A scrolling list draws only a window of its items.
  int first = 0, shown = count;
  if (ReadU16(base, page + pm::kPageFlags) & 1) {
    first = std::clamp<int>(ReadS16(base, page + pm::kPageScrollTop), 0, count - 1);
    shown = std::clamp<int>(ReadS16(base, page + pm::kPageScrollShown), 0, count);
  }
  const uint32_t buttons =
      ReadU32(base, page) == cod3::addr::msgbox::kPageVtable ? DialogButtons(base, system) : 0;
  for (int k = 0; k < shown; ++k) {
    const int i = (first + k) % count;
    const uint32_t entry = ReadU32(base, list + 4 * uint32_t(i));
    if (!Valid(entry) || ReadU8(base, entry + pm::kEntryDisabled)) continue;
    uint32_t label = ReadU32(base, entry + pm::kEntryLabel);
    bool selected = ReadU8(base, entry + pm::kEntryDrawnSelected) != 0;
    if (buttons) {
      const int slot = DialogSlot(base, buttons, i);
      if (slot < 0 || slot > 3) continue;
      label = ReadU32(base, buttons + pm::kButtonSlots + 4 * uint32_t(slot));
      selected = ReadU32(base, buttons + pm::kButtonSelectedSlot) == uint32_t(slot);
    }
    Item it;
    if (!DrawnText(ctx, base, label, selected, &it.ink)) continue;
    it.index = i;
    it.left_aligned = (ReadU16(base, label + pm::kTextAlign) & pm::kAlignLeft) != 0;
    items.push_back(it);
  }
  const bool values = HasValues(base, page);
  LayOutHitAreas(items, values);
  if (!buttons && values) AttachControls(items, DrawnControls(system), loose);  // [menus-brightness]
  return items;
}

const Item* ItemAt(const std::vector<Item>& items, float x, float y) {
  const Item* best = nullptr;
  float best_d = 1e9f;
  for (const auto& it : items) {
    if (!it.hit.Contains(x, y)) continue;
    const float dx = x - it.ink.cx(), dy = y - it.ink.cy();
    const float d = dx * dx * 0.01f + dy * dy;  // mostly the nearest row
    if (d < best_d) best = &it, best_d = d;
  }
  return best;
}

const Item* FindItem(const std::vector<Item>& items, int index) {
  for (const auto& it : items)
    if (it.index == index) return &it;
  return nullptr;
}

int Selected(uint8_t* base, uint32_t page) { return ReadS16(base, page + pm::kPageSelected); }

// Moves the highlight to `target` with the page's own MoveUp / MoveDown, the
// way the D-pad does (so the page updates its titles, help text and sound),
// stepping the way the target lies on screen.
void SelectItem(PPCContext& ctx, uint8_t* base, uint32_t page, const std::vector<Item>& items,
                int target) {
  const Item* to = FindItem(items, target);
  if (!to) return;
  const int count = ReadS16(base, page + pm::kPageCount);
  for (int n = 0; n < count; ++n) {
    const int sel = Selected(base, page);
    if (sel == target) return;
    const Item* from = FindItem(items, sel);
    const float line = std::max(1.0f, (to->ink.y1 - to->ink.y0) * 0.5f);
    const bool down = from ? (to->ink.cy() > from->ink.cy() + line ||
                              (std::abs(to->ink.cy() - from->ink.cy()) <= line && to->ink.cx() > from->ink.cx()))
                           : target > sel;
    VCall(ctx, base, page, down ? pm::kPageMoveDown : pm::kPageMoveUp);
    if (Selected(base, page) == sel) return;  // the page didn't move
  }
}

// One button event, through the menu system's own handler (as from the pad).
// The handler gets the pad that pressed; the game reads the active one first
// ("none" before Start is pressed on the title screen) and then whichever pad
// pressed. The keyboard and mouse are player 1 (pad 0).
uint64_t MenuPlayer(PPCContext& ctx, uint8_t* base, uint32_t system) {
  uint64_t player = VCall(ctx, base, system, pm::kSysPlayer) & 0xFFFFFFFFu;
  return player > 3 ? 0 : player;
}

bool IsDirection(uint32_t code) {
  return code == pm::kCodeUp || code == pm::kCodeDown || code == pm::kCodeLeft || code == pm::kCodeRight;
}

// [menus-brightness] Game thread: the code our Press is sending right now, and
// the arrow key held into the game (pressed with hold, not yet released), for
// FromPcInput.
uint32_t g_pressing_code = 0;
uint32_t g_holding_code = 0;

// The release the pad's input sends when a direction is let go: the page
// stops repeating it. Without it a page repeats the direction for ever
// (the "endless scrolling" of mouse wheel and arrow keys before).
void Release(PPCContext& ctx, uint8_t* base, uint32_t system, uint32_t code) {
  if (g_holding_code == code) g_holding_code = 0;  // [menus-brightness]
  VCall(ctx, base, system, pm::kSysRelease, code, MenuPlayer(ctx, base, system));
}

// A button: pressed, and let go at once unless `hold` (a held arrow key).
void Press(PPCContext& ctx, uint8_t* base, uint32_t system, uint32_t code, bool hold = false) {
  const uint64_t player = MenuPlayer(ctx, base, system);
  REXLOG_DEBUG("cod3: menu input {} (player {})", code, player);
  g_pressing_code = code;  // [menus-brightness]
  if (hold && IsDirection(code)) g_holding_code = code;
  VCall(ctx, base, system, pm::kSysButton, code, player);
  g_pressing_code = 0;
  if (IsDirection(code) && !hold) Release(ctx, base, system, code);
}

bool IsTitlePage(uint8_t* base, uint32_t page) {
  return ReadU32(base, page) == pm::kTitleMenuVtable;
}

// ---------------------------------------------------------------------------
// The pointer and the keys share the highlight the way a PC game does: the
// item under the pointer is highlighted when the mouse moves onto it; a key,
// the wheel or the pad then takes over, and the pointer only takes the
// highlight back once the mouse moves a few pixels away from where it was
// (or onto another item). A resting or jittering pointer never pulls the
// highlight back, so it can't fight the arrow keys. The pointer moves the
// highlight with the page's own moves, a step whenever the page lets it (it
// ignores moves while the highlight animates), and gives up after a bounded
// number of frames, so nothing can keep scrolling.
// ---------------------------------------------------------------------------
constexpr float kTakeBackDistance = 6.0f;  // menu units (640x480): ~13 px at 1080p
constexpr int kClickWaitFrames = 120;       // a click's action waits this long at most

struct Hover {
  uint32_t page = 0;
  int last_selected = -1;  // the selection when the last frame's input was done
  int pointed = -1;        // the item under the pointer at its last move
  int target = -1;         // the item the pointer asks for, not reached yet
  int tries = 0;
  bool keys = false;       // keys / wheel / pad have the highlight
  float key_x = 0.0f, key_y = 0.0f;  // where the pointer was when they took it
  // A click on an item that wasn't highlighted yet: its action follows once
  // the highlight has settled on it (the page ignores input while it moves).
  uint32_t click_code = 0;
  int click_item = -1;
  int click_frames = 0;
};
Hover g_hover;  // game thread only

void KeysTakeOver(Hover& h, float x, float y) {
  h.keys = true;
  h.key_x = x, h.key_y = y;
  h.target = -1;
  h.click_item = -1;
}

bool PageLocked(uint8_t* base, uint32_t page) { return ReadU8(base, page + pm::kPageLocked) != 0; }

// ---------------------------------------------------------------------------
// [menus-brightness] A gauge follows the pointer: a click on it, and the
// pointer while the button stays down. The game's brightness is set directly
// (menu_video.h SetSlider); any other gauge moves by its row's own left /
// right steps, at most one a frame, toward the pointer, and stops on the
// step that reaches or passes it (where its fill ends shows where it is).
// It never keeps going on its own: it waits for the pointer to move, ends
// when the button is up and it has arrived, and gives up after a bounded
// number of steps or when the page changes.
// ---------------------------------------------------------------------------
constexpr int kMaxDragSteps = 400;

struct Drag {
  bool active = false;
  uint32_t page = 0;
  int item = -1;           // the row, or -1: the page's own slider
  bool released = false;   // the button is up: finish at the last place
  float x = 0.0f;          // where the pointer wants the gauge (menu x)
  float stop_x = -1.0f;    // arrived (or passed) with the pointer here
  int last_dir = 0;        // the last step's direction
  float last_frac = -1.0f; // where the fill was before it
  float step = 0.0f;       // how far one step moved the fill
  int steps = 0, still = 0, wait = 0;
};
Drag g_drag;  // game thread only

// GammaScreenMenu reads the pad directly in Draw; its direction methods only
// animate the arrows. Adapt the reference's brightness control, keeping the
// game's existing exit/profile-save path and gamma-ramp update.
void SetBrightness(uint8_t* base, float value) {
  namespace br = cod3::addr::brightness;
  const float maximum = ReadF32(base, br::kMax);
  if (!std::isfinite(maximum) || maximum <= 0.5f || !std::isfinite(value)) return;
  cod3::guest::WriteF32(base, br::kValue, std::clamp(value, 0.5f, maximum));
}

void StepBrightness(uint8_t* base, bool right) {
  namespace br = cod3::addr::brightness;
  const float step = ReadF32(base, br::kStep);
  if (!(step > 0.0f && step < 1.0f)) return;
  SetBrightness(base, ReadF32(base, br::kValue) + (right ? step : -step));
}

void StartDrag(uint32_t page, int item, float x) {
  g_drag = Drag{};
  g_drag.active = true;
  g_drag.page = page;
  g_drag.item = item;
  g_drag.x = x;
}

void EndDrag(const char* why) {
  if (g_drag.active && REXCVAR_GET(cod3_menu_trace))
    REXLOG_INFO("cod3: menu trace: gauge of item {} let go ({}, {} steps)", g_drag.item, why, g_drag.steps);
  g_drag = Drag{};
}

void UpdateDrag(PPCContext& ctx, uint8_t* base, uint32_t system, uint32_t page, float px, bool down) {
  Drag& d = g_drag;
  if (!d.active) return;
  if (d.page != page) return EndDrag("page changed");
  if (!d.released) d.x = px;  // the pointer's latest place: where it was let go too
  if (!down) d.released = true;
  RowControls loose;
  const std::vector<Item> items = ItemRects(ctx, base, page, system, &loose);
  const RowControls* rc = &loose;
  if (d.item >= 0) {
    const Item* it = FindItem(items, d.item);
    // Wait for the highlight to settle on the row (a click on another row).
    if (!it || Selected(base, page) != d.item || PageLocked(base, page)) {
      if (++d.wait > kClickWaitFrames) EndDrag("row not reached");
      return;
    }
    rc = &it->ctl;
  }
  const float width = rc->g.x1 - rc->g.x0;
  if (!rc->gauge || width < 4.0f) return EndDrag("no gauge");
  const float target = std::clamp((d.x - rc->g.x0) / width, 0.0f, 1.0f);
  if (cod3::video::SetSlider(base, page, d.item, target)) {
    if (d.released) EndDrag("video set");
    return;
  }
  if (ReadU32(base, page) == cod3::addr::brightness::kPageVtable && d.item < 0) {
    const float maximum = ReadF32(base, cod3::addr::brightness::kMax);
    SetBrightness(base, 0.5f + target * (maximum - 0.5f));
    if (d.released) EndDrag("brightness set");
    return;
  }
  if (!rc->fill) return EndDrag("no fill");
  const float frac = std::clamp((rc->f.x1 - rc->g.x0) / width, 0.0f, 1.0f);
  if (d.stop_x >= 0.0f) {  // arrived: wait for the pointer to move on
    if (d.released) return EndDrag("arrived");
    if (std::abs(d.x - d.stop_x) < 1.0f) return;
    d.stop_x = -1.0f, d.last_dir = 0, d.last_frac = -1.0f, d.still = 0;
  }
  if (d.last_dir != 0 && d.last_frac >= 0.0f) {
    const float moved = std::abs(frac - d.last_frac);
    if (moved < 1e-4f) {
      if (++d.still >= 3) d.stop_x = d.x, d.last_dir = 0;  // at an end, or the row doesn't move
      return;
    }
    d.still = 0, d.step = moved;
  }
  const float diff = target - frac;
  const int dir = std::abs(diff) <= std::max(0.004f, d.step * 0.5f) ? 0 : diff > 0.0f ? 1 : -1;
  if (dir == 0 || (d.last_dir != 0 && dir != d.last_dir)) {  // there, or one step past it
    d.stop_x = d.x, d.last_dir = 0;
    if (d.released) EndDrag("arrived");
    return;
  }
  if (++d.steps > kMaxDragSteps) return EndDrag("too many steps");
  Press(ctx, base, system, dir > 0 ? pm::kCodeRight : pm::kCodeLeft);
  d.last_dir = dir, d.last_frac = frac;
}

// ---------------------------------------------------------------------------
// Footer prompts ("[ENTER] SELECT [ESC] BACK", "(A) SELECT (B) BACK") are
// buttons too: a click on one presses what it names. Their places come from
// the game's own line layout (addresses.h, FEMultiLineText lines), noted as
// each line is drawn (game/button_prompts.cpp calls NoteLineDrawn) inside a
// menu system's draw, so a click only ever reaches the prompts of the menu
// that takes the input, never a hint of the game behind it.
// ---------------------------------------------------------------------------
struct Prompt {
  uint32_t system = 0;  // the menu system that drew it
  uint32_t line = 0;    // the text line it is on
  uint32_t frame = 0;   // the game frame it was drawn in
  Box area;             // its button and the words after it
  uint32_t code = 0;    // the button (pm::kCode*), or 0 for Accept (Enter)
};

std::mutex g_prompt_mutex;
std::vector<Prompt> g_prompts;          // g_prompt_mutex
thread_local uint32_t t_drawing_system = 0;  // the menu system drawing right now (game thread)

// What a prompt's button stands for: the game's button picture (a character
// of the button font) or a name in brackets (button_prompts.cpp: the key the
// PC menus take, or a controller style's name). Only buttons a menu takes;
// 0 = none (d-pad and stick pictures, triggers).
bool PromptButton(const std::string& glyph, uint32_t* code) {
  struct Name {
    const char* text;
    uint32_t code;  // 0: Accept, like the Enter key
  };
  static constexpr Name kNames[] = {
      {"\"", pm::kCodeA},        {"%", pm::kCodeB},           {"#", pm::kCodeX},
      {"$", pm::kCodeY},         {"-", pm::kCodeStart},       {"1", pm::kCodeBack},
      {"[ENTER]", 0},            {"[ESC]", pm::kCodeB},       {"[A]", pm::kCodeA},
      {"[CROSS]", pm::kCodeA},   {"[B]", pm::kCodeB},         {"[CIRCLE]", pm::kCodeB},
      {"[X]", pm::kCodeX},       {"[SQUARE]", pm::kCodeX},    {"[Y]", pm::kCodeY},
      {"[TRIANGLE]", pm::kCodeY}, {"[START]", pm::kCodeStart}, {"[BACK]", pm::kCodeBack},
      {"[SELECT]", pm::kCodeBack},
  };
  for (const auto& n : kNames)
    if (glyph == n.text) return *code = n.code, true;
  return false;
}

void NoteLine(uint8_t* base, uint32_t line, float sy) {
  const uint32_t system = t_drawing_system;
  if (!system || !Valid(line)) return;
  const int32_t n = int32_t(ReadU32(base, line + pm::kLineSegmentCount));
  const uint32_t segments = ReadU32(base, line + pm::kLineSegments);
  if (n <= 0 || n > 64 || !Valid(segments)) return;  // no buttons
  const uint32_t string = ReadU32(base, line + pm::kLineString);
  if (!Valid(string)) return;
  const std::string text = GuestString(base, string, 512);
  const float x = ReadF32(base, line + pm::kLineX), y = ReadF32(base, line + pm::kLineY);
  const float width = ReadF32(base, line + pm::kLineWidth);
  const uint32_t font = TextFont(base, int32_t(ReadU32(base, line + 0)));
  if (!font || !std::isfinite(x) || !std::isfinite(y) || !(width > 0.0f && width < 2000.0f)) return;
  const float height = float(int32_t(ReadU32(base, font + pm::kFontLineHeight))) * sy;
  auto entry = [&](int i, int field) {  // field 0: string index, 1: x offset
    return int32_t(int16_t(ReadU16(base, segments + 4 * uint32_t(i) + 2 * uint32_t(field))));
  };
  const uint32_t frame = ReadU32(base, pm::kFrameCount);
  std::lock_guard lock(g_prompt_mutex);
  // This line's prompts as drawn now; those of older frames are gone (a
  // page's footer is drawn every frame).
  std::erase_if(g_prompts, [frame, line](const Prompt& p) { return p.line == line || frame - p.frame > 2; });
  for (int k = 0; k < n; k += 2) {
    const int32_t start = entry(k, 0);
    const int32_t end = k + 1 < n ? entry(k + 1, 0) : int32_t(text.size());
    if (start < 0 || end <= start || end > int32_t(text.size())) continue;
    Prompt p;
    if (!PromptButton(text.substr(size_t(start), size_t(end - start)), &p.code)) continue;
    p.system = system;
    p.line = line;
    p.frame = frame;
    const float x0 = x + float(entry(k, 1));
    const float x1 = k + 2 < n ? x + float(entry(k + 2, 1)) : x + width;
    p.area = Box{x0 - 3.0f, y - 3.0f, std::max(x0 + 4.0f, x1 - 3.0f), y + height + 3.0f};
    if (g_prompts.size() < 64) g_prompts.push_back(p);
  }
}

// [menus-brightness] A picture drawn inside a menu system's draw: noted when
// it is an arrow or a gauge (see "Arrows and gauges" above).
void NoteControl(uint8_t* base, uint32_t quad) {
  namespace fe = cod3::addr::fe;
  const uint32_t system = t_drawing_system;
  if (!system || !Valid(quad)) return;
  if (!(ReadU8(base, quad + pm::kTextVisibility) & 0x4) || !(ReadF32(base, quad + fe::kElemOpacity) > 0.05f)) return;
  Control c;
  if (!ControlKindOf(GuestString(base, ReadU32(base, quad + fe::kQuadName), 64), &c.kind)) return;
  if (!QuadBox(base, quad, &c.box)) return;
  c.system = system;
  c.quad = quad;
  c.frame = ReadU32(base, pm::kFrameCount);
  std::lock_guard lock(g_control_mutex);
  std::erase_if(g_controls, [&c](const Control& o) { return o.quad == c.quad || c.frame - o.frame > 2; });
  if (g_controls.size() < 128) g_controls.push_back(c);
}

// The prompt of `system` under (x, y), drawn in the last two frames.
std::optional<Prompt> PromptAt(uint32_t system, float x, float y) {
  const uint32_t frame = FrameNow();
  std::lock_guard lock(g_prompt_mutex);
  for (const auto& p : g_prompts)
    if (p.system == system && frame - p.frame <= 2 && p.area.Contains(x, y)) return p;
  return std::nullopt;
}

// A left click: the item under the pointer (its value side on option rows),
// a footer prompt, or, on a page without items ("Press Start", a results
// screen), go on.
// Acts on item `index` with `code` (A, or a step left / right): at once when
// it is highlighted already, else once the highlight has moved there.
void ClickItem(PPCContext& ctx, uint8_t* base, uint32_t system, uint32_t page, const std::vector<Item>& items,
               int index, uint32_t code) {
  const bool was_selected = Selected(base, page) == index;
  if (!was_selected) SelectItem(ctx, base, page, items, index);
  if (was_selected && !PageLocked(base, page)) {
    Press(ctx, base, system, code);
    return;
  }
  // The highlight is (still) moving there: act once it has settled.
  g_hover.click_code = code;
  g_hover.click_item = index;
  g_hover.click_frames = 0;
  g_hover.target = index;
  g_hover.tries = 0;
}

void Click(PPCContext& ctx, uint8_t* base, uint32_t system, uint32_t page, float x, float y) {
  EndDrag("new click");  // [menus-brightness]
  RowControls loose;
  const std::vector<Item> items = ItemRects(ctx, base, page, system, &loose);
  // [menus-brightness] An arrow steps its row's value its way; a gauge goes
  // to the place clicked (and follows the pointer while the button is held).
  // The page's own (Options > SCREEN) likewise, without a row.
  if (HasValues(base, page)) {
    for (const auto& row : items) {
      if (const uint32_t code = ArrowAt(row.ctl, x, y)) {
        if (REXCVAR_GET(cod3_menu_trace)) REXLOG_INFO("cod3: menu trace: arrow {} of item {} clicked", code, row.index);
        ClickItem(ctx, base, system, page, items, row.index, code);
        return;
      }
      if (GaugeAt(row.ctl, x, y)) {
        if (Selected(base, page) != row.index) SelectItem(ctx, base, page, items, row.index);
        StartDrag(page, row.index, x);
        g_hover.target = row.index;
        g_hover.tries = 0;
        return;
      }
    }
    if (const uint32_t code = ArrowAt(loose, x, y)) {
      Press(ctx, base, system, code);
      return;
    }
    if (GaugeAt(loose, x, y)) {
      StartDrag(page, -1, x);
      return;
    }
  }
  const Item* it = ItemAt(items, x, y);
  if (!it) {
    if (const auto prompt = PromptAt(system, x, y)) {
      const uint32_t code = prompt->code ? prompt->code : IsTitlePage(base, page) ? pm::kCodeStart : pm::kCodeA;
      if (REXCVAR_GET(cod3_menu_trace)) REXLOG_INFO("cod3: menu trace: footer prompt clicked: button {}", code);
      Press(ctx, base, system, code);
      return;
    }
    if (items.empty()) Press(ctx, base, system, IsTitlePage(base, page) ? pm::kCodeStart : pm::kCodeA);
    return;  // beside the items: nothing, like a PC menu
  }
  // An option row (the page takes left / right): a click on its value steps
  // it toward the half clicked. [menus-brightness] With both arrows drawn the
  // value is what lies between them; else (no arrows found) the old rule:
  // right of a left-aligned label, split half way to the column's edge.
  uint32_t code = pm::kCodeA;
  const RowControls& rc = it->ctl;
  const bool values = HasValues(base, page);
  if (values && rc.left && rc.right) {
    if (x >= rc.l.x0 - kControlMargin && x <= rc.r.x1 + kControlMargin)
      code = x < (rc.l.cx() + rc.r.cx()) * 0.5f ? pm::kCodeLeft : pm::kCodeRight;
  } else if (values && !rc.left && !rc.right && !rc.gauge && it->left_aligned && x > it->ink.x1 + 8.0f) {
    const float mid = (it->ink.x1 + it->hit.x1) * 0.5f;
    code = x < mid ? pm::kCodeLeft : pm::kCodeRight;
  }
  ClickItem(ctx, base, system, page, items, it->index, code);
}

// Logs each new page once with where its items are drawn (letters) and can
// be pointed at (hit), in menu space (640x480), for the notes and for bug
// reports. With cod3_menu_trace also whenever that changes (pages slide in,
// a message box gets new buttons) and the footer prompts.
void LogPage(PPCContext&, uint8_t* base, uint32_t system, uint32_t page) {
  if (REXCVAR_GET(cod3_menu_trace))
    REXLOG_INFO("COD3_PC_MENU page={:08X} system={:08X} selected={} count={}",
      page, system, Selected(base, page), ReadS16(base, page + pm::kPageCount));
}

void OnMenuInput(PPCContext& ctx, uint8_t* base, uint32_t system) {
  if (!REXCVAR_GET(cod3_pc_menus) || !Valid(system)) return;
  const uint32_t page = CurrentPage(ctx, base, system);
  if (!page) return;
  if (!g_menu_time.load(std::memory_order_relaxed))
    REXLOG_INFO("COD3_PC_MENU_ACTIVE system={:08X} page={:08X} vtable={:08X}",
                system, page, ReadU32(base, page));
  g_current_frame.store(ReadU32(base, pm::kFrameCount), std::memory_order_relaxed);
  const uint32_t frame = ReadU32(base, pm::kFrameCount);
  g_menu_frame.store(frame, std::memory_order_relaxed);
  g_menu_time.store(Now(), std::memory_order_relaxed);
  const bool dialog = ReadU32(base, system) == pm::kDialogSystemVtable;
  if (dialog) {
    g_dialog_frame.store(frame, std::memory_order_relaxed);
    g_dialog_seen.store(true, std::memory_order_relaxed);
  } else if (g_dialog_seen.load(std::memory_order_relaxed) &&
             frame - g_dialog_frame.load(std::memory_order_relaxed) <= 1) {
    return;  // a message box over this menu takes the input
  }

  LogPage(ctx, base, system, page);
  cod3::video::NumberInput(base, page);

  Hover& h = g_hover;
  if (h.page != page) {  // a new page: nothing carried over
    h = Hover{};
    h.page = page;
    h.last_selected = Selected(base, page);
    g_holding_code = 0;  // [menus-brightness] a new page forgets a held direction too
  }

  bool moved = false, over = false, left_down = false;
  float x = 0.0f, y = 0.0f;
  std::optional<Request> request;
  {
    static uint32_t moves_seen = 0;
    std::lock_guard lock(g_input.mutex);
    moved = g_input.moves != moves_seen;
    moves_seen = g_input.moves;
    over = g_input.over;
    x = g_input.x, y = g_input.y;
    left_down = g_input.left_down;  // [menus-brightness]
    // One event per frame, like the pad; old ones (asked as a menu
    // closed) are dropped, but never a release.
    while (!g_input.requests.empty() && !request) {
      const Request& front = g_input.requests.front();
      if (front.ask == Ask::kRelease || frame - front.frame <= kMaxRequests + 2) request = front;
      g_input.requests.pop_front();
    }
    // An arrow tapped within one frame: one move, as a press and release
    // together (the page would otherwise repeat it until the next frame).
    if (request && request->ask == Ask::kHold && !g_input.requests.empty() &&
        g_input.requests.front().ask == Ask::kRelease && g_input.requests.front().code == request->code) {
      request->ask = Ask::kButton;
      g_input.requests.pop_front();
    }
    // A held arrow whose key-up never came (focus lost): let it go.
    if (!request && g_held.code && Now() - g_held.last_seen > kHeldTimeout) {
      request = Request{Ask::kRelease, g_held.code};
      g_held.code = 0;
    }
  }

  // The pad (the game's own input, read just before this) moved the
  // highlight: it has it now, like the keys.
  const bool trace = REXCVAR_GET(cod3_menu_trace);
  if (Selected(base, page) != h.last_selected) {
    if (trace) REXLOG_INFO("cod3: menu trace: {} -> {} (pad or the game)", h.last_selected, Selected(base, page));
    KeysTakeOver(h, x, y);
    EndDrag("the pad moved");  // [menus-brightness]
  }
  const int start_selected = Selected(base, page);
  const int start_target = h.target;

  if (request) {
    if (request->ask == Ask::kClick) {
      h.keys = false;
      h.target = -1;
      h.click_item = -1;
    } else if (request->ask != Ask::kRelease) {
      KeysTakeOver(h, x, y);
      EndDrag("keys");  // [menus-brightness]
    }
  } else if (g_drag.active) {
    // [menus-brightness] The pointer drags a gauge: the highlight stays.
  } else if (moved && !over) {
    h.pointed = -1;
  } else if (moved) {
    // The mouse moved: unless the keys have the highlight and the pointer is
    // still where it was when they took it, highlight what it points at.
    const float dx = x - h.key_x, dy = y - h.key_y;
    const auto items = ItemRects(ctx, base, page, system);
    const Item* it = ItemAt(items, x, y);
    const int pointed = it ? it->index : -1;
    const bool away = dx * dx + dy * dy >= kTakeBackDistance * kTakeBackDistance;
    if (!h.keys || away || pointed != h.pointed) {
      h.keys = false;
      if (pointed >= 0 && pointed != Selected(base, page) && pointed != h.target) {
        h.target = pointed;
        h.tries = 0;
      }
    }
    h.pointed = pointed;
  }

  // Walk the highlight to the pointer's item: one try per frame, bounded.
  if (!request && !h.keys && h.target >= 0) {
    const int count = ReadS16(base, page + pm::kPageCount);
    if (Selected(base, page) == h.target || ++h.tries > count + kClickWaitFrames) {
      if (Selected(base, page) != h.target) h.click_item = -1;
      h.target = -1;
    } else if (!PageLocked(base, page)) {
      SelectItem(ctx, base, page, ItemRects(ctx, base, page, system), h.target);
    }
  }
  // A click's action, once the highlight is on its item and has settled.
  if (!request && h.click_item >= 0) {
    if (Selected(base, page) == h.click_item && !PageLocked(base, page)) {
      const uint32_t code = h.click_code;
      h.click_item = -1;
      Press(ctx, base, system, code);
    } else if (++h.click_frames > kClickWaitFrames) {
      h.click_item = -1;
    }
  }

  if (request) {
    switch (request->ask) {
      case Ask::kButton:
        Press(ctx, base, system, request->code);
        break;
      case Ask::kHold:
        Press(ctx, base, system, request->code, /*hold=*/true);
        break;
      case Ask::kRelease:
        Release(ctx, base, system, request->code);
        break;
      case Ask::kAccept:
        Press(ctx, base, system, IsTitlePage(base, page) ? pm::kCodeStart : pm::kCodeA);
        break;
      case Ask::kClick:
        Click(ctx, base, system, page, request->x, request->y);
        break;
    }
  }
  // [menus-brightness] A gauge being dragged (or going to where it was
  // clicked): at most one step a frame.
  if (!request || request->ask == Ask::kClick) UpdateDrag(ctx, base, system, page, x, left_down);
  if (trace && (Selected(base, page) != start_selected || h.target != start_target || request || moved)) {
    const char* what = !request ? "pointer"
                       : request->ask == Ask::kClick ? "click"
                       : request->ask == Ask::kAccept ? "enter"
                       : request->ask == Ask::kHold ? "key held"
                       : request->ask == Ask::kRelease ? "key let go" : "key";
    REXLOG_INFO("cod3: menu trace: {} -> {} ({}{}; pointer {:.0f},{:.0f}{}{}, target {}, keys {})",
                start_selected, Selected(base, page), what,
                request && request->code ? fmt::format(" {}", request->code) : std::string(),
                x, y, over ? "" : " outside", moved ? " moved" : "", h.target, h.keys);
  }
  // The page may have changed (Press can open another one); the next frame
  // starts over then.
  h.last_selected = Selected(base, page);
}


}  // namespace

namespace cod3::pcmenu {
void Reset() {
  g_current_frame = 0;
  g_menu_frame = 0;
  g_menu_time = 0;
  g_dialog_frame = 0;
  g_dialog_seen = false;
  g_hover = {};
  g_drag = {};
  g_pressing_code = g_holding_code = 0;
  { std::lock_guard lock(g_input.mutex);
    g_input.requests.clear(); g_input.over = g_input.left_down = false;
    g_input.moves = 0; g_held = {};
  }
  { std::lock_guard lock(g_control_mutex); g_controls.clear(); }
  { std::lock_guard lock(g_prompt_mutex); g_prompts.clear(); }
}
void Attach(rex::ui::Window* window, size_t z_order, std::function<bool()> overlay_open) {
  if (!window) return;
  if (g_listener.window) Detach(g_listener.window);
  RefreshPictureAspect();
  g_listener.window = window;
  g_listener.overlay_open = std::move(overlay_open);
  window->AddInputListener(&g_listener, z_order);
  window->AddListener(&g_listener);
}
void Detach(rex::ui::Window* window) {
  if (window && g_listener.window == window) {
    window->RemoveInputListener(&g_listener);
    window->RemoveListener(&g_listener);
  }
  g_listener.window = nullptr;
  g_listener.overlay_open = {};
}
bool MenuActive() {
  if (!REXCVAR_GET(cod3_pc_menus)) return false;
  const auto when = g_menu_time.load(std::memory_order_relaxed);
  return when && FrameNow() - g_menu_frame.load(std::memory_order_relaxed) <= 1 &&
    Now() - when < kLoadingLimit;
}
void UpdateCursor(rex::ui::Window* window) {
  if (!window) return;
  RefreshPictureAspect();
  const bool overlay = g_listener.overlay_open && g_listener.overlay_open();
  const bool menu = MenuActive();
  if (overlay || !menu) cod3::video::CancelNumber();
  rex::input::mnk::SetMouseLookActive(!menu && !overlay);
  using V = rex::ui::Window::CursorVisibility;
  const auto want = (menu || overlay) ? V::kVisible : V::kAutoHidden;
  if (window->GetCursorVisibility() != want) window->SetCursorVisibility(want);
}
bool FromPcInput(uint32_t code) { return code && (g_pressing_code == code || g_holding_code == code); }
}  // namespace cod3::pcmenu

void MenuInput::OnLostFocus(rex::ui::UISetupEvent&) {
  cod3::video::CancelNumber();
  std::lock_guard lock(g_input.mutex);
  g_input.requests.clear();
  if (g_held.code) PushLocked({Ask::kRelease, g_held.code});
  g_held = {};
  g_input.left_down = false;
  g_input.over = false;
}

REX_EXTERN(__imp__sub_824C4DB8);
REX_HOOK_RAW(sub_824C4DB8) {
  const auto system = ctx.r3.u32;
  __imp__sub_824C4DB8(ctx, base);
  OnMenuInput(ctx, base, system);
}
REX_EXTERN(__imp__sub_82536DD0);
REX_HOOK_RAW(sub_82536DD0) {
  g_current_frame.store(ReadU32(base, pm::kFrameCount), std::memory_order_relaxed);
  __imp__sub_82536DD0(ctx, base);
  g_current_frame.store(ReadU32(base, pm::kFrameCount), std::memory_order_relaxed);
}
REX_EXTERN(__imp__sub_824CE4D0);
REX_HOOK_RAW(sub_824CE4D0) {
  const auto outer = t_drawing_system;
  t_drawing_system = ctx.r3.u32;
  __imp__sub_824CE4D0(ctx, base);
  t_drawing_system = outer;
}
REX_EXTERN(__imp__sub_824CD9E0);
REX_HOOK_RAW(sub_824CD9E0) {
  const auto outer = t_drawing_system;
  t_drawing_system = ctx.r3.u32;
  __imp__sub_824CD9E0(ctx, base);
  t_drawing_system = outer;
}
REX_EXTERN(__imp__sub_824CE3E8);
REX_HOOK_RAW(sub_824CE3E8) {
  const auto outer = t_drawing_system;
  t_drawing_system = ctx.r3.u32;
  __imp__sub_824CE3E8(ctx, base);
  t_drawing_system = outer;
}
REX_EXTERN(__imp__sub_824C80D0);
REX_HOOK_RAW(sub_824C80D0) {
  const auto quad = ctx.r3.u32;
  __imp__sub_824C80D0(ctx, base);
  if (t_drawing_system && REXCVAR_GET(cod3_pc_menus)) NoteControl(base, quad);
}
REX_EXTERN(__imp__sub_824D7D88);
REX_HOOK_RAW(sub_824D7D88) {
  if (t_drawing_system && REXCVAR_GET(cod3_pc_menus)) NoteLine(base, ctx.r3.u32, float(ctx.f3.f64));
  cod3::menuprompts::DrawLine(ctx, base);
}

REX_EXTERN(__imp__sub_824BC2C0);
REX_HOOK_RAW(sub_824BC2C0) {
  if (ReadU32(base, ctx.r3.u32) == cod3::addr::brightness::kPageVtable &&
      cod3::pcmenu::FromPcInput(pm::kCodeLeft)) StepBrightness(base, false);
  __imp__sub_824BC2C0(ctx, base);
}
REX_EXTERN(__imp__sub_824BC248);
REX_HOOK_RAW(sub_824BC248) {
  if (ReadU32(base, ctx.r3.u32) == cod3::addr::brightness::kPageVtable &&
      cod3::pcmenu::FromPcInput(pm::kCodeRight)) StepBrightness(base, true);
  __imp__sub_824BC248(ctx, base);
}
