# Native PC video page

First image-settings page under the original main menu's Options > VIDEO.
Adapted from SirCarls/cod3_REcomp_ revision
`d54c1bb877ab364dae2d2f40aa8c6a8ca46e3070`, `menu_video.cpp`, `menu_fov.cpp`
and the relevant layout constants in `addresses.h`. Copyright 2026 COD_3_WAW
contributors, [MIT notice](../licenses/SirCarls-COD_3_WAW-MIT.txt).
No generated game function or game asset imported.

The Options panel's unused fifth row opens the existing four-row Gameplay
page in a separate video mode. GAMEPLAY restores its original labels and
handlers. Video toggles never alter the four original gameplay option bytes.
The original text, arrows, wrapping, highlight and mouse navigation are used.
Localized keys are allocated in the current Runtime's SystemHeap; caches reset
before new guest threads, so no string/page pointer survives Runtime release.

| Setting | Backend | Application |
|---|---|---|
| Depth of field | cod3_dof_strength, 0–100% in steps of 5 | Corrected D3D12 shader translation; live |
| Anti-aliasing | swap_post_effect: none/fxaa/fxaa_extreme | Requires new graphics initialization; help explicitly says restart |
| Texture filtering | anisotropic_override: original/off/1X/2X/4X/8X/16X | Pinned texture-cache sampler policy; live, eligible textures only |
| V-Sync | Inverse of d3d12_allow_variable_refresh_rate_and_tearing | Swap-chain creation flags; help explicitly says restart |

These are a first subset. Resolution, a guarded live fullscreen change,
and broader gameplay binding-dependent prompts remain future integration.
Pause-menu entry and menu keyboard prompts are implemented (see below). Existing SCREEN brightness remains available.
The Russian VIDEO help includes the actual fixed menu keys (Enter, Esc and
left/right), not a promise that remappable gameplay actions are integrated.
The alternate non-English language-mode layout is guarded; the current
Russian asset translation uses the reviewed layout. Font text is encoded
to Windows-1251 for the game's single-byte glyphs; visual acceptance is needed.

The original Options move handlers use four-entry title/help tables. For the
added fifth item, hooks retain their original base movement virtual calls
(+204/+208) and perform a bounded four-stock-plus-VIDEO lookup afterwards.
There is no out-of-bounds fifth lookup into the original tables.

## Save and launch precedence

Guest handlers change real SDK CVars and publish a host-only save request.
The application consumes it on the UI thread using SDK SaveConfig and the
actual configured path. A pending request is also flushed before old
presentation teardown. It contains no guest pointer or deferred Runtime call.

Run-Native now passes `--cod3_dof_strength` only when `-DofStrength` was explicitly
specified. Normal launch honors saved config, otherwise the original default
100%. Explicit 0/35/100 still override config. Dry-run cases for default, 0 and
35 passed. This fixes the launcher's previous unconditional 100% override.

## Evidence and validation

Original XEX SHA-256:
`E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682`.
Private Ghidra evidence: `logs/ghidra-video-menu.log` and
`logs/ghidra-video-menu-instructions.log`.

- 824E2948–824E2BAB: original four Options items and hidden fifth panel row.
- 824BC070: starts `2B040003` (cmplwi r4,3); original item 0 dispatch at
  824BC0A0 loads owning system +8 and sets page 3 before original virtual go-to.
- 824ED298–824ED45B: Options enter/selection.
- 824C3248 / 824C3190: original base movement virtual +204/+208, then tables
  82064BE0 and 82064BF0 indexed by selection.
- 824DB960: original Gameplay refresh; 824BD160 toggles original option byte
  at page+148+selected (bypassed only for our video mode).
- 824CD2D0, 824C3FE0 and 824C3F08: original update and selection movement.
- 8252CB98–8252CC6F: original Localize; custom keys only intercepted, all
  other localization delegates unchanged.

`Build.ps1 -Stage Tests`: 10 Python + 8 native tests passed, including
video stepping, boundaries, malformed values and V-Sync inversion.
`logs/video-menu-tests.log`. New page interaction, Russian rendering, save
across full launch and menu/Continue still require runtime acceptance.

Full `Build.ps1 -Stage Build` passed, including strict `--ignore-stamp` and
corrected GPU plugin staging (`logs/video-menu-build.log`). Opened one normal
menu PID 18736 / HWND 0xc50848 with the existing nightd_glow_query_01 profile,
no forced mission or DoF override. Log `native-20261021-210128-011.log`
confirms original entry, VIDEO_MENU_READY and VIDEO_MENU_OPEN. User changed
DoF to 35 and reported that it remained; the supplied screenshot confirms
Russian title/rows, 35%, correct glyphs and wrapped help in the game's layout.
Log confirms SDK SaveConfig; the actual TOML contains cod3_dof_strength=35.

At this inspection the log contains generation 1 only. Reopening the page
and file persistence are established; full process restart, new Runtime,
visual AA/AF/V-Sync comparisons and pause integration are not inferred from
the screenshot or the short response. The stock footer still shows Xbox
glyphs: broader device-dependent prompts remain a separate pending step.


## Pause integration and restart acceptance (2026-10-22)

Pause > Options now contains VIDEO before Reset. Its four rows share the
frontend settings model and SDK CVars. Original options remain intact; Esc
restores their visibility and selection before normal back/save handling.
Copied text/arrow objects belong to the current page and are released by its
destructor; host page caches reset before new guest threads.

Reviewed hooks: 824E60E8 init, 824CD530 draw, 824E6020 select, 824F07B0 back,
824DC1C0 toggle, 824C41B8/824C4218 movement, 824EE128 update, 824E6588 refresh,
824DC640 destruction, 824F0B50 pause Options entry. Private instruction and
ownership evidence: logs/ghidra-pause-video*.log, same XEX hash as above.
The user accepted pause/video navigation, Esc return, camera and audio.
Menu keyboard prompts are described in MENU_PROMPTS.md.

Full restart exposed an SDK serialization defect: unescaped Windows paths
made the entire saved TOML unreadable. Both SDK string serializers now use
toml++ formatting. Reproducible patch:
tools/patches/rexglue-v0100-config-string-escaping.patch, applied by
Build-TitleThreadDrain.ps1. tests/config_roundtrip_test.cpp exercises actual
SaveConfig/LoadConfig and category serialization with Windows paths, Cyrillic,
quotes, controls and a numeric setting. It fails against the old DLL and
passes against the fixed DLL (logs/config-roundtrip-old-sdk.log and
logs/config-roundtrip-fixed-sdk.log).

The invalid previous config was backed up under reports/config-escaping-backup
and repaired with validation, preserving the user's current DoF 75%.
Build.ps1 -Stage Build passed including strict --ignore-stamp and GPU staging;
Build.ps1 -Stage Tests passed all 10 Python and 9 native tests. Evidence:
logs/menu-ui-config-fixed-build.log and logs/menu-ui-config-fixed-tests.log.
Fresh normal-menu launch, existing nightd_glow_query_01 profile:
logs/native-20261022-095001-784.log, PID 25072, HWND 0x1330960, generation 1,
COD3_VIDEO_SETTINGS dof=75 aa=none af=3 tearing=true. This confirms full-process
restart persistence; a new Runtime within this launch, gamepad switching and
visual AA/AF/V-Sync comparison still need separate acceptance.


## DoF numeric input and pause slider (2026-10-22, awaiting manual acceptance)

First numeric setting integration: select DoF in frontend VIDEO or pause VIDEO
and type digits (including numpad); Enter commits, Backspace edits, Esc cancels
without leaving the page. The visible underscore marks unfinished input. Values
clamp to 0..100 and round to the existing 5% step. Moving selection/page,
losing focus, opening an overlay or replacing Runtime cancels unfinished edits.
The UI works with a mutex-protected host model only; it never reads guest memory.
Commit and save happen through the existing game-thread CVar path.

Pause VIDEO additionally draws a DoF gauge between native arrows, with the
number to its right. Native volume pictures are deep-copied with the existing
reviewed constructors/Assign path, remapped and owned by the page; zero fill
is not drawn. Mouse dragging sets DoF directly on the existing 5% grid, without
repeated direction presses. Frontend VIDEO has numeric entry and its existing
arrows; its slider picture remains a later layout step. Other video rows stay
choice lists. This is not yet a conversion of every original game's slider.

Gauge layout is informed by SirCarls menu_fov.cpp/menu_slider.h at
6ae4839cc780664fd9a1b039173083f0dd4fc9c1 (MIT notice as above); the bounded
integer edit model is project-authored. No new guest hook or function boundary
is added. Ghidra investigation logs ghidra-video-slider.log and
 ghidra-video-slider-instructions.log inspected native fill 824C0588 and its
caller 824DC108. The new implementation does not call this unregistered leaf;
it remaps native copied picture geometry through the already-reviewed draw.

Build.ps1 -Stage Tests passed 10 Python + 10 native tests, including numeric
commit/cancel, empty input, page changes, bounds, rounding, and nonfinite drag
fractions. Strict Codegen --ignore-stamp passed (396 unchanged, 0 unresolved
modules cached). Changed app objects compiled. Evidence: video-slider-tests.log,
video-slider-codegen.log, video-slider-compile.log under private logs/.
Runtime acceptance must check visible fill/number layout, drag, Enter/Esc,
normal page navigation and persistence after menu/Continue.

Full native/GPU build then passed (logs/video-slider-build.log). After the
user-authorized normal close, launched one process PID 59152 / HWND 0xc50bc4,
existing nightd_glow_query_01 profile, generation 1 and saved DoF 75%.
logs/native-20261022-231555-750.log confirms original menu initialization.
Manual gauge/input acceptance remains pending.


User response to slider/input/persistence check: "Осталось всё". This confirms
reported retention of the selected value. The inspected launch log records
DoF 35% committed by the number-or-slider path at 23:17:03.683, after arrow
changes; no separate pause VIDEO_OPEN or generation 2 appears in that log.
Do not infer complete drag/layout/Esc or Runtime-transition acceptance from
this short response. Those detailed behaviors still need direct observation.
