// Adapted from SirCarls/cod3_REcomp_, d54c1bb877ab364dae2d2f40aa8c6a8ca46e3070.
// Copyright (c) 2026 the COD_3_WAW contributors. MIT license:
// licenses/SirCarls-COD_3_WAW-MIT.txt. See docs/PC_MENUS.md for adaptation.
#pragma once
#include <cstdint>
namespace cod3::addr {
namespace pc_menu {
// FEMenuSystem::vfunc_12, ProcessInput(this); hooked.
inline constexpr uint32_t kProcessInput = 0x824C4DB8;
// The game's frame counter: Com_Frame (0x82536DD0) adds one every frame
// (0x82536FA4); the start of a timing struct read all over the game.
inline constexpr uint32_t kFrameCount = 0x825F4E08;
// FEMenuSystem fields and virtual slots (byte offsets into the vtable)
inline constexpr uint32_t kSysPages = 4;          // FEMenu** pages
inline constexpr uint32_t kSysCurrent = 12;       // s32[]: current page per slot, < 0 = none
inline constexpr uint32_t kSysButton = 68;        // HandleButton(this, code, player)
// Release(this, code, player): what ProcessInput sends when a held direction
// is let go; the page (vtbl+152) stops repeating it. HandleButton on a
// direction makes the page repeat it (vtbl+148 keeps the code at page+50,
// FEMenu's update repeats it) until then; a page's Refresh forgets it.
inline constexpr uint32_t kSysRelease = 72;
inline constexpr uint32_t kSysPlayer = 88;        // (this) -> the player the system reads
inline constexpr uint32_t kSysSlot = 92;          // (this) -> index into kSysCurrent
inline constexpr uint32_t kDialogSystemVtable = 0x820719D4;
// Button codes HandleButton takes (0x824BE118 maps them to pad buttons).
inline constexpr uint32_t kCodeBack = 1;
inline constexpr uint32_t kCodeStart = 2;
inline constexpr uint32_t kCodeUp = 4;
inline constexpr uint32_t kCodeDown = 8;
inline constexpr uint32_t kCodeLeft = 16;
inline constexpr uint32_t kCodeRight = 32;
inline constexpr uint32_t kCodeA = 64;
inline constexpr uint32_t kCodeB = 128;
inline constexpr uint32_t kCodeX = 256;
inline constexpr uint32_t kCodeY = 512;
// FEMenu (page) fields and virtual slots
inline constexpr uint32_t kPageItems = 4;         // FEMenuEntry**
inline constexpr uint32_t kPageScrollTop = 32;    // s16: first item shown (scrolling lists)
inline constexpr uint32_t kPageSelected = 34;     // s16
inline constexpr uint32_t kPageCount = 38;        // s16
inline constexpr uint32_t kPageScrollShown = 40;  // s16: items shown at once (scrolling lists)
inline constexpr uint32_t kPageFlags = 42;        // u16: 0x1 = scrolling list
inline constexpr uint32_t kPageMoveUp = 100;
inline constexpr uint32_t kPageMoveDown = 104;
inline constexpr uint32_t kPageOnLeft = 108;
inline constexpr uint32_t kFEMenuOnLeft = 0x820ADAC0;  // the default: the page has no left/right
inline constexpr uint32_t kNullsub = 0x82144CA8;       // an empty method (Options' OnLeft)
inline constexpr uint32_t kTitleMenuVtable = 0x82071724;  // "Press Start": Start, not A
// FEMenuEntry (menu item) fields and virtual slots
inline constexpr uint32_t kEntryLabel = 16;       // the text element showing the item
inline constexpr uint32_t kEntryDisabled = 22;    // u8: skipped by MoveUp / MoveDown
inline constexpr uint32_t kEntrySetVisible = 24;  // (bool): shows / hides the label
inline constexpr uint32_t kEntrySetDisabled = 32; // (bool): also greys the label
inline constexpr uint32_t kEntrySetLabelKey = 48; // (const char* key)
// Text elements (FEText, vtable 0x82070B54; FEMultiLineText derives from it).
// The game draws an item's label with nglListAddString at (x, y) moved by
// its alignment (0x824BF4A8): right (x -= width), left, else centered; bottom
// (y -= height), top, else centered.
inline constexpr uint32_t kTextVtable = 0x82070B54;
inline constexpr uint32_t kMultiLineTextVtable = 0x82070C84;
inline constexpr uint32_t kTextVisibility = 16;   // u8: 0x4 = visible
inline constexpr uint32_t kTextX = 44;            // float, 640x480 menu space
inline constexpr uint32_t kTextY = 48;
inline constexpr uint32_t kTextAlign = 108;       // u16: 16 left, 32 right, 64 top, 128 bottom
inline constexpr uint32_t kTextWidth = 240;       // (this, const float* scale or 0) -> f1
inline constexpr uint32_t kTextHeight = 244;
inline constexpr uint16_t kAlignLeft = 16, kAlignRight = 32, kAlignTop = 64, kAlignBottom = 128;
// FEMenu: nonzero while the highlight animates; the page ignores moves then.
inline constexpr uint32_t kPageLocked = 48;               // u8
inline constexpr uint32_t kPageSelectionChanged = 28;     // vtbl: () after SetSelection
inline constexpr uint32_t kPageSetSelection = 164;        // vtbl: (index, 1)
inline constexpr uint32_t kPageBack = 120;                // vtbl: () what B does on the page
// FEMenuEntry::Draw (vtbl +12) draws its label with this byte as "selected".
inline constexpr uint32_t kEntryDrawnSelected = 21;       // u8
// How FEText::Draw (0x824CFD58) places its text: font index (4 and 5 draw
// nothing), the String* shown, x / y scales for the selected (+56/+60) and
// the normal (+80/+84) state, name String* (FindElement). The fonts are
// g_FEManager +4 (prompts::kFonts).
inline constexpr uint32_t kTextFont = 24;                 // s32
inline constexpr uint32_t kTextString = 28;               // String*: chars at +12
inline constexpr uint32_t kTextScaleSelected = 56;        // float x, float y
inline constexpr uint32_t kTextScaleNormal = 80;          // float x, float y
inline constexpr uint32_t kTextName = 100;                // String*
inline constexpr uint32_t kStringChars = 12;
inline constexpr uint32_t kStringLength = 6;              // u16
// Font object: glyphs of 28 bytes {.., .., width, height, x offset, y offset,
// advance} (s32 each), for characters first .. first + count - 1 (others
// clamp to the ends); a space advances like 'a' (layout 0x821347A8, measure
// 0x82134B90: the width ends at the last glyph's right edge).
inline constexpr uint32_t kFontTexture = 4;               // 0: the font can't draw
inline constexpr uint32_t kFontGlyphs = 8;
inline constexpr uint32_t kFontLineHeight = 28;           // s32
inline constexpr uint32_t kFontFirstChar = 36;            // s32
inline constexpr uint32_t kFontGlyphCount = 40;           // s32
inline constexpr uint32_t kGlyphSize = 28;
// Message boxes: the page's items aren't what is drawn. The box draws the
// button list of its DialogMenuSystem (+52, msgbox::kSysButtons; drawn by
// 0x824C4738): four button texts in fixed slots at +24..+36, the highlighted
// slot at +60, the button count at +56. UpdateSelection (msgbox::
// kUpdateSelection) puts item i in slot 2 for one button, i + 1 for two or
// three (when slots 1 and 2 exist), i for four.
inline constexpr uint32_t kButtonSlots = 24;              // FEText*[4]
inline constexpr uint32_t kButtonCount = 56;              // u32
inline constexpr uint32_t kButtonSelectedSlot = 60;       // u32
// FEMultiLineText (footers, message texts): lines of 32 bytes at +144,
// count +136 (prompts: kLayoutLine). A line: +0 font index, +4 / +8 x / y,
// +16 width (float), +20 String* (its button tokens already replaced by the
// glyph or name), +24 s16 pairs {string index, x offset}, +28 their count:
// entry 2k starts button k, entry 2k + 1 the text after it. Drawn by
// prompts::kDrawLine (this, .., f2 x scale, f3 y scale, ...).
inline constexpr uint32_t kTextLines = 144;
inline constexpr uint32_t kTextLineCount = 136;
inline constexpr uint32_t kLineSize = 32;
inline constexpr uint32_t kLineX = 4;
inline constexpr uint32_t kLineY = 8;
inline constexpr uint32_t kLineWidth = 16;
inline constexpr uint32_t kLineString = 20;
inline constexpr uint32_t kLineSegments = 24;
inline constexpr uint32_t kLineSegmentCount = 28;
// Each menu system's draw (vtbl +56), which draws its page: the footers drawn
// inside belong to that system. Hooked.
inline constexpr uint32_t kFrontEndDraw = 0x824CE4D0;
inline constexpr uint32_t kInGameDraw = 0x824CD9E0;
inline constexpr uint32_t kDialogDraw = 0x824CE3E8;
}  // namespace pc_menu
namespace fe {
inline constexpr uint32_t kElemOpacity = 4;                // float, animated by the panel's fades
inline constexpr uint32_t kQuadName = 72;
inline constexpr uint32_t kQuadPartCount = 40;
inline constexpr uint32_t kQuadPartVerts = 16;
inline constexpr uint32_t kQuadParts = 32;
inline constexpr uint32_t kQuadVertStride = 20;
}
namespace msgbox { inline constexpr uint32_t kSysButtons = 52, kPageVtable = 0x820718EC; }
namespace prompts { inline constexpr uint32_t kFonts = 0x82A8DF30, kFontCount = 4; }
namespace brightness {
inline constexpr uint32_t kValue = 0x825CDAA4, kMax = 0x825CE628, kStep = 0x826A13CC;
inline constexpr uint32_t kPageVtable = 0x82072434;
}
}
