// Adapted from SirCarls d54c1bb, button_prompts.cpp. Copyright 2026
// COD_3_WAW contributors, MIT: licenses/SirCarls-COD_3_WAW-MIT.txt.
// Menu keys and validated gameplay action words; unsupported prompts stay original.
#include "action_prompt.h"
#include "pc_input_model.h"
#include "menu_prompts.h"
#include "menu_guest.h"
#include "pc_menu.h"
#include "pc_input.h"
#include <rex/cvar.h>
#include <rex/input/state_merge.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <string>
#include <unordered_map>
#include <cctype>
REXCVAR_DEFINE_BOOL(cod3_menu_key_prompts, true, "Game", "Show keyboard keys in original menus when keyboard/mouse is in use");
REXCVAR_DEFINE_BOOL(cod3_game_key_prompts, true, "Game", "Show assigned keys in original gameplay action hints");
namespace {
namespace bp {
constexpr uint32_t kFonts=0x82A8DF30, kFontCount=4, kButtonFont=1, kLineFont=0;
constexpr uint32_t kStringChars=12, kStringFromChars=0x824A5978, kFontBaseline=32;
constexpr uint32_t kSetText=144, kSetWrapped=364, kSetLines=368, kSetWrappedRaw=372;
}
using namespace cod3::guest;
bool Valid(uint32_t p) { return p >= 0x10000 && p < 0xC0000000u; }
enum class Mode { kXbox, kMenuKeys, kGameKeys };
bool IsText(Mode m) { return m == Mode::kMenuKeys; }
std::atomic<bool> keyboard{true};
std::atomic<int64_t> keyboard_time{0};
std::atomic<uint32_t> g_bind_gen{0};
std::atomic<bool> action_prompt_logged{false};
std::mutex watch_mutex;
uint16_t last_buttons=0;
int16_t last_axes[4]{};
uint8_t last_triggers[2]{};
std::string bind_signature;
int64_t next_bind_check=0;
int64_t NowMs() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
Mode Wanted(uint8_t*) {
  {
    std::lock_guard lock(watch_mutex);
    const auto now=NowMs();
    if(now>=next_bind_check) {
      next_bind_check=now+1000;
      std::string current;
      for(const auto* flag : {"keybind_x","keybind_y","keybind_left_trigger","keybind_right_trigger",
          "keybind_left_shoulder","keybind_right_shoulder","keybind_back","keybind_lstick_press","keybind_rstick_press"})
        current+=rex::cvar::GetFlagByName(flag)+'\n';
      for (int a = 0; a < cod3::input::kActionCount; ++a)
        current += cod3::input::PromptBinding(a) + '\n';
      if(current!=bind_signature) { bind_signature=std::move(current); ++g_bind_gen; }
    }
  }
  if (cod3::pcmenu::MenuActive())
    return REXCVAR_GET(cod3_menu_key_prompts) && keyboard.load() ? Mode::kMenuKeys : Mode::kXbox;
  return REXCVAR_GET(cod3_game_key_prompts) && cod3::input::PromptKeysActive()
    ? Mode::kGameKeys : Mode::kXbox;
}
std::unordered_map<std::string, uint32_t> name_strings;
std::mutex name_mutex;
uint32_t StoredText(uint8_t* base, const std::string& text) {
  std::lock_guard lock(name_mutex);
  if (name_strings.size() >= 4096 && !name_strings.contains(text)) return 0;
  auto& ptr = name_strings[text];
  if (!ptr) {
    ptr = REX_KERNEL_MEMORY()->SystemHeapAlloc(uint32_t(text.size() + 1));
    if (ptr) std::memcpy(base + ptr, text.c_str(), text.size() + 1);
  }
  return ptr;
}
uint32_t NameText(uint8_t* base, char glyph, Mode) {
  const char* fixed=nullptr; const char* flag=nullptr;
  switch(glyph) {
    case '"': case '-': fixed="ENTER"; break;
    case '%': fixed="ESC"; break;
    case ')': fixed="UP"; break; case ',': fixed="DOWN"; break;
    case '+': fixed="LEFT"; break; case '*': fixed="RIGHT"; break;
    case '/': fixed="UP/DOWN"; break; case '.': fixed="LEFT/RIGHT"; break; case '0': fixed="ARROWS"; break;
    case '#': flag="keybind_x"; break; case '$': flag="keybind_y"; break;
    case '&': flag="keybind_left_trigger"; break; case '!': flag="keybind_right_trigger"; break;
    case '\'': flag="keybind_left_shoulder"; break; case '(': flag="keybind_right_shoulder"; break;
    case '1': flag="keybind_back"; break; case '2': flag="keybind_lstick_press"; break; case '3': flag="keybind_rstick_press"; break;
    default: return 0;
  }
  std::string text=fixed ? fixed : rex::cvar::GetFlagByName(flag);
  if(text.empty()) text="UNBOUND";
  for(char& c:text) c=char(std::toupper(static_cast<unsigned char>(c)));
  text="["+text+"]";
  return StoredText(base, text);
}
thread_local int t_set_depth = 0;            // inside a set-text call
thread_local Mode t_set_mode = Mode::kXbox;  // and what it lays out
thread_local bool t_names = false;           // the layout in progress writes names
thread_local Mode t_name_mode = Mode::kMenuKeys;
thread_local uint32_t t_tokens = 0;          // button tokens laid out so far

std::mutex g_mutex;
// Lines laid out with names: they are drawn with the text font too.
std::unordered_map<uint32_t, bool> g_text_lines;
// The last set-text call of each text that had buttons, to lay it out again.
struct TextCall {
  uint32_t slot = 0;        // the virtual slot called (bp::kSet*)
  bool string_arg = false;  // takes a String** (else a const char*)
  std::string text;         // before the game's "[ACTION]" pass
  uint32_t width = 0;
  double scale = 0.0;  // wrapped text takes its original scale in f1
  Mode mode = Mode::kXbox;
  uint32_t gen = 0;         // g_bind_gen it was laid out with
};
std::unordered_map<uint32_t, TextCall> g_texts;

Mode LayoutMode(uint8_t* base) { return t_set_depth ? t_set_mode : Wanted(base); }

// font* of a font index, 0 for none or the button font.
uint32_t TextFont(uint8_t* base, uint32_t index) {
  if (index >= bp::kFontCount || index == bp::kButtonFont) return 0;
  const uint32_t font = ReadU32(base, bp::kFonts + 4 * index);
  return Valid(font) ? font : 0;
}

// Runs `call` with the button font slot holding `font`.
template <typename F>
void WithButtonFont(uint8_t* base, uint32_t font, F&& call) {
  const uint32_t slot = bp::kFonts + 4 * bp::kButtonFont;
  const uint32_t saved = ReadU32(base, slot);
  WriteU32(base, slot, font);
  struct Restore { uint8_t* base; uint32_t slot, saved; ~Restore() { WriteU32(base, slot, saved); } } restore{base, slot, saved};
  call();
}

// A set-text call of an FEMultiLineText: remembered when it had buttons.
template <typename F>
void SetText(PPCContext& ctx, uint8_t* base, uint32_t slot, bool string_arg, F&& original) {
  const uint32_t obj = ctx.r3.u32;
  TextCall call;
  call.slot = slot;
  call.string_arg = string_arg;
  call.width = ctx.r5.u32;
  call.scale = ctx.f1.f64;
  if (string_arg) {
    const uint32_t str = Valid(ctx.r4.u32) ? ReadU32(base, ctx.r4.u32) : 0;
    if (Valid(str)) call.text = ReadString(base, str + bp::kStringChars, 2048);
  } else if (Valid(ctx.r4.u32)) {
    call.text = ReadString(base, ctx.r4.u32, 2048);
  }
  const Mode outer_mode = t_set_mode;
  call.mode = t_set_mode = Wanted(base);
  call.gen = g_bind_gen.load(std::memory_order_relaxed);
  const uint32_t tokens = t_tokens;
  ++t_set_depth;
  original();
  --t_set_depth;
  t_set_mode = outer_mode;
  std::lock_guard lock(g_mutex);
  if (t_tokens != tokens)
    g_texts[obj] = std::move(call);
  else
    g_texts.erase(obj);
}

// Before a text is drawn: lays it out again when its prompts are stale.
void Refresh(PPCContext& ctx, uint8_t* base, uint32_t obj) {
  TextCall call;
  {
    std::lock_guard lock(g_mutex);
    auto it = g_texts.find(obj);
    if (it == g_texts.end()) return;
    const Mode want = Wanted(base);
    if (it->second.mode == want &&
        (want == Mode::kXbox || it->second.gen == g_bind_gen.load(std::memory_order_relaxed)))
      return;
    call = it->second;
  }
  if (call.text.size() > 1000) return;
  Call c(ctx, base);
  const uint32_t text = c.PushString(call.text);
  uint32_t arg = text;
  if (call.string_arg) {
    // A fresh String* on the guest stack (the callee releases it).
    const uint32_t holder = (c.PushString("........") + 3) & ~3u;
    WriteU32(base, holder, 0);
    c.Invoke(bp::kStringFromChars, {holder, text});
    arg = holder;
  }
  c.Invoke(VirtualSlot(base, obj, call.slot), {obj, arg, call.width}, {call.scale});
}

void Forget(uint32_t obj) {
  std::lock_guard lock(g_mutex);
  g_texts.erase(obj);
}
} // namespace
namespace cod3::menuprompts {
void KeyboardUsed() {
  keyboard=true; keyboard_time=NowMs();
  // The single-player KB/M client follows whichever original pad pressed Start.
  for (uint32_t user = 0; user < rex::input::kMaxGuestUsers; ++user)
    rex::input::KeyboardMouseUsed(user);
}
void Reset() {
  { std::lock_guard lock(g_mutex); g_text_lines.clear(); g_texts.clear(); }
  { std::lock_guard lock(name_mutex); name_strings.clear(); }
  { std::lock_guard lock(watch_mutex); last_buttons=0;
    for(auto& axis:last_axes) axis=0;
    for(auto& trigger:last_triggers) trigger=0;
    next_bind_check=0; bind_signature.clear(); }
  keyboard=true;
  action_prompt_logged=false;
}
}
REX_EXTERN(__imp__sub_82344F98);
REX_HOOK_RAW(sub_82344F98) {
  const auto user=ctx.r3.u32, state=ctx.r4.u32;
  __imp__sub_82344F98(ctx,base);
  if(ctx.r3.u32 || !Valid(state)) return;
  if (!user) {
  std::lock_guard lock(watch_mutex);
  const auto buttons=ReadU16(base,state+4);
  bool pad=(buttons & ~last_buttons)!=0;
  for(int i=0;i<4;++i) {
    const auto value=int16_t(ReadU16(base,state+8+2*i));
    pad|=std::abs(int(value))>16000 && std::abs(int(last_axes[i]))<=16000;
    last_axes[i]=value;
  }
  for(int i=0;i<2;++i) {
    const auto value=ReadU8(base,state+6+i);
    pad|=value>30 && last_triggers[i]<=30;
    last_triggers[i]=value;
  }
  if(pad && NowMs()-keyboard_time.load()>250) keyboard=false;
  last_buttons=buttons;
  }
  // Observe physical input before injecting keyboard/mouse interaction state.
  cod3::input::ApplyToPad(base, user, state);
}
// "~name" -> the button's character: the name instead while names are laid out.
REX_EXTERN(__imp__sub_824C68E0);
REX_HOOK_RAW(sub_824C68E0) {
  const uint32_t out = ctx.r4.u32;
  __imp__sub_824C68E0(ctx, base);
  if (ctx.r3.u32 == 0 || !Valid(out)) return;  // not a button token
  ++t_tokens;
  if (!t_names) return;
  const uint32_t glyph = ReadU32(base, out);
  if (!Valid(glyph)) return;
  if (const uint32_t name = NameText(base, char(base[glyph]), t_name_mode)) WriteU32(base, out, name);
}

// Original [ACTION] expansion returns a transient char*. Keep translated
// strings in the current Runtime heap, never on a hook-local guest stack.
REX_EXTERN(__imp__sub_824DD9A8);
REX_HOOK_RAW(sub_824DD9A8) {
  const auto token = Valid(ctx.r3.u32) ? ReadString(base, ctx.r3.u32, 64) : std::string{};
  __imp__sub_824DD9A8(ctx, base);
  if (!Valid(ctx.r3.u32)) return;
  const auto prompt = cod3::input::ParseActionPrompt(token);
  if (prompt.action < 0) return;
  ++t_tokens; // retained text must be refreshed after device/binding changes
  if (LayoutMode(base) != Mode::kGameKeys) return;
  const auto key = cod3::input::PromptBinding(prompt.action);
  if (key.empty()) return;
  std::string verb;
  if (prompt.form) {
    const auto tables = ReadU32(base, 0x82A2A14C);
    if (!Valid(tables)) return;
    Call call(ctx, base);
    const auto at = uint32_t(call.Invoke(0x8252CB98, {tables,
      call.PushString(prompt.form == 3 ? "INGAME_PRESS_AND_HOLD" : "INGAME_PRESS")}));
    if (!Valid(at)) return;
    verb = ReadString(base, at, 128);
    if (verb.empty() || verb.starts_with("INGAME_")) return;
  }
  const auto text = cod3::input::FormatActionPrompt(prompt, key, verb, verb);
  if (const auto at = StoredText(base, text)) {
    ctx.r3.u64 = at;
    if (!action_prompt_logged.exchange(true))
      REXLOG_INFO("COD3_GAME_ACTION_PROMPT token={} key={}", token, key);
  }
}

// Lays out one line: with names, measured in the line's font at the text scale.
REX_EXTERN(__imp__sub_824D7650);
REX_HOOK_RAW(sub_824D7650) {
  const uint32_t line = ctx.r3.u32;
  const Mode mode = LayoutMode(base);
  const uint32_t font = IsText(mode) ? TextFont(base, ReadU32(base, line + bp::kLineFont)) : 0;
  if (!font) {
    __imp__sub_824D7650(ctx, base);
    std::lock_guard lock(g_mutex);
    g_text_lines.erase(line);
    return;
  }
  ctx.f2.f64 = ctx.f1.f64;  // names at the text's scale
  const bool outer_names = t_names;
  const Mode outer_mode = t_name_mode;
  t_names = true, t_name_mode = mode;
  const uint32_t tokens = t_tokens;
  WithButtonFont(base, font, [&] { __imp__sub_824D7650(ctx, base); });
  t_names = outer_names, t_name_mode = outer_mode;
  std::lock_guard lock(g_mutex);
  if (t_tokens != tokens) {
    // Practically never this many; a forgotten line only loses its names.
    if (g_text_lines.size() > 50000) g_text_lines.clear();
    g_text_lines[line] = true;
  } else {
    g_text_lines.erase(line);
  }
}

// Measures one word for the word wrap: with names like the layout above.
REX_EXTERN(__imp__sub_824D7B48);
REX_HOOK_RAW(sub_824D7B48) {
  const Mode mode = LayoutMode(base);
  const uint32_t font = IsText(mode) ? TextFont(base, ctx.r4.u32) : 0;
  if (!font) {
    __imp__sub_824D7B48(ctx, base);
    return;
  }
  ctx.f2.f64 = ctx.f1.f64;
  const bool outer_names = t_names;
  const Mode outer_mode = t_name_mode;
  t_names = true, t_name_mode = mode;
  WithButtonFont(base, font, [&] { __imp__sub_824D7B48(ctx, base); });
  t_names = outer_names, t_name_mode = outer_mode;
}

// Draws one line: names in the line's font at the text's x scale, standing on
// the text's baseline (the text may be drawn taller than wide).
REX_EXTERN(__imp__sub_824D7D88);
void cod3::menuprompts::DrawLine(PPCContext& ctx, uint8_t* base) {
  const uint32_t line = ctx.r3.u32;
  bool names;
  {
    std::lock_guard lock(g_mutex);
    names = g_text_lines.count(line) != 0;
  }
  const uint32_t font = names ? TextFont(base, ReadU32(base, line + bp::kLineFont)) : 0;
  if (!font) {
    __imp__sub_824D7D88(ctx, base);
    return;
  }
  const double baseline = double(int32_t(ReadU32(base, font + bp::kFontBaseline)));
  ctx.f4.f64 = ctx.f2.f64;                            // the text's x scale
  ctx.f5.f64 = baseline * (ctx.f2.f64 - ctx.f3.f64);  // raise: baselines meet
  WithButtonFont(base, font, [&] { __imp__sub_824D7D88(ctx, base); });
}

// FEMultiLineText::Draw: stale prompts are laid out again first.
REX_EXTERN(__imp__sub_824CFA00);
REX_HOOK_RAW(sub_824CFA00) {
  Refresh(ctx, base, ctx.r3.u32);
  __imp__sub_824CFA00(ctx, base);
}

// The set-text calls (SetTextKey and the hint widget's SetText end here).
REX_EXTERN(__imp__sub_824E8180);
REX_HOOK_RAW(sub_824E8180) {
  SetText(ctx, base, bp::kSetText, false, [&] { __imp__sub_824E8180(ctx, base); });
}

REX_EXTERN(__imp__sub_824E8098);
REX_HOOK_RAW(sub_824E8098) {
  SetText(ctx, base, bp::kSetWrapped, true, [&] { __imp__sub_824E8098(ctx, base); });
}

REX_EXTERN(__imp__sub_824DD790);
REX_HOOK_RAW(sub_824DD790) {
  SetText(ctx, base, bp::kSetLines, false, [&] { __imp__sub_824DD790(ctx, base); });
}

REX_EXTERN(__imp__sub_824DD630);
REX_HOOK_RAW(sub_824DD630) {
  SetText(ctx, base, bp::kSetWrappedRaw, true,
          [&] { __imp__sub_824DD630(ctx, base); });
}

// A new or deleted text at an address: its old set-text call goes.
REX_EXTERN(__imp__sub_824DEAD0);
REX_HOOK_RAW(sub_824DEAD0) {
  Forget(ctx.r3.u32);
  __imp__sub_824DEAD0(ctx, base);
}

REX_EXTERN(__imp__sub_820AD1B8);
REX_HOOK_RAW(sub_820AD1B8) {
  Forget(ctx.r3.u32);
  __imp__sub_820AD1B8(ctx, base);
}
