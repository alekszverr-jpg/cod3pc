// Reviewed layout from SirCarls d54c1bb; MIT notice in licenses/.
#pragma once
#include <cstdint>
namespace cod3::video::layout {
inline constexpr uint32_t kPanelFindElement = 0x824D07D0;  // Element* Panel::FindElement(panel, name): elements @12, count @20; 0 if missing
inline constexpr uint32_t kPanelFindQuad = 0x824DFC70;     // PanelQuad* (panel, name)
inline constexpr uint32_t kQuadName = 72;
inline constexpr uint32_t kOperatorNew = 0x820CC1D0;       // (size)
inline constexpr uint32_t kOperatorDelete = 0x820CC468;     // (pointer)
inline constexpr uint32_t kItemSetLabelKey = 48;   // (const char* key)
inline constexpr uint32_t kElemSetVisible = 60;    // (bool)
inline constexpr uint32_t kElemSetTextKey = 132;   // (const char* key) -> Localize -> SetTextRaw
inline constexpr uint32_t kElemSetTextRaw = 144;   // (const char* text)
inline constexpr uint32_t kElemSetAlpha = 136;     // (f1 alpha)
inline constexpr uint32_t kWrapGetWidth = 304;     // instructions box: () -> int
inline constexpr uint32_t kWrapSetTextKey = 360;   // instructions box: (key, width, f1 scale)
inline constexpr uint32_t kTextScaleDefault = 0x825D121C;  // -1.0
inline constexpr uint32_t kGameplayTitleMain = 64;          // "OPTIONS / GAMEPLAY" text
inline constexpr uint32_t kGameplayValues = 100;            // 4 x value text (slot_0N_text_b)
inline constexpr uint32_t kGameplayArrows = 116;            // 4 x {left, right} arrow pictures
inline constexpr uint32_t kGameplayTitleKey = 0x8206FFFC;   // "FEMENU_OP_GAMEPLAY_TITLE"
inline constexpr uint32_t kGameplayLabelKeys = 0x82064744;  // 4 x char*: FEMENU_COP_SUBTITLES ...
inline constexpr uint32_t kPageSystem = 8;                  // FEMenu: FEMenuSystem*
inline constexpr uint32_t kSysPages = 4;                    // FEMenuSystem: FEMenu** pages
inline constexpr uint32_t kSysGoTo = 16;                    // vtbl: (page index, 0)
inline constexpr uint32_t kPageAddItem = 16;       // (index, Element* label, int flags)
inline constexpr uint32_t kCurrentSelectedText = 72;  // text element
inline constexpr uint32_t kInstructionsText = 92;  // wrapped text element
inline constexpr uint32_t kArrowFlashTimer = 152;  // float
inline constexpr uint32_t kArrowFlashTime = 0x82001930;    // 0.1
inline constexpr uint32_t kArrowAlphaNormal = 0x82062720;  // 0.5
inline constexpr uint32_t kArrowAlphaFlash = 0x82064200;   // 1.0
inline constexpr uint32_t kLanguageMode = 0x82A2A150;

inline constexpr uint32_t kQuadCtor = 0x824C8498;          // PanelQuad(this), 80 bytes
inline constexpr uint32_t kQuadSize = 80;
inline constexpr uint32_t kTextCtor = 0x820ACA38;          // FEText(this), 112 bytes
inline constexpr uint32_t kTextSize = 112;
inline constexpr uint32_t kElemAssign = 72;                // (source)
inline constexpr uint32_t kQuadY = 24;                     // float (x at +20)
inline constexpr uint32_t kTextY = 48;                     // float
inline constexpr uint32_t kTextTransform = 32;
inline constexpr uint32_t kTextTransformEnd = 92;
inline constexpr uint32_t kElemOpacity = 4;                // float, animated by the panel's fades
inline constexpr uint32_t kQuadParts = 32;
inline constexpr uint32_t kQuadPartCount = 40;
inline constexpr uint32_t kQuadPartVerts = 16;
inline constexpr uint32_t kQuadVertStride = 20;
}
