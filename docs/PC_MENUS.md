# Mouse in original menus

Navigation adapted from SirCarls/cod3_REcomp_, revision
`d54c1bb877ab364dae2d2f40aa8c6a8ca46e3070`, files under
`RELEASE/COD_3_WAW_Setup-tool/recomp/src/game/`: `pc_menu.cpp`, `guest.h`,
and the relevant menu layout constants in `addresses.h`. Brightness handling
also follows `menu_video.cpp`'s bounded value/step approach. Copyright 2026
COD_3_WAW contributors, [MIT notice](../licenses/SirCarls-COD_3_WAW-MIT.txt).
No generated game functions or assets imported.

The original game still draws and handles its pages. A host listener queues
mouse/keyboard actions; the guest input hook invokes the same virtual button
and selection methods as the controller. Text hit areas use the actual game
font metrics, alignment and selected scale, including Russian byte glyphs.
Draw hooks observe arrow, gauge and footer positions. Current support:

- Pointer motion highlights a row; left click activates or adjusts it.
- Right click, Escape and Backspace go back. Enter accepts (Start on title).
- Wheel and arrow keys navigate. Held arrows use original repeat/release.
- Original option gauges step through their own handlers toward the clicked
  or dragged position. SCREEN brightness additionally updates the original
  bounded gamma value: that page's direction methods only flash arrows.
- Keyboard/controller selection retains priority until the pointer moves.
- Mouse capture is disabled in menus/overlays through the pinned SDK MnK API;
  cursor visibility and camera capture return in gameplay.

`cod3_pc_menus=true` by default; false restores old navigation.
`cod3_menu_trace=true` is diagnostic, not required for normal use.
This change does not add settings rows, PC binding labels, custom quit rows,
or the reference's ultrawide projection. SDK debug settings remain on F4.

## Lifetime and focus adaptation

The reference's UI-side guest-memory frame read was replaced by a host atomic
frame published from original Com_Frame and menu input. UI handlers never
read a stale guest base. All guest calls stay on their original game thread.
Calls use SDK CallFrame and separate ABI stack space, preserving caller
registers and propagating FPSCR as required by the SDK.

Attach runs on the UI thread for each new presentation. Detach removes both
input and focus listeners before the old drawer/presenter are released during
the accepted title drain. OnPostSetup resets queues, hover/drag state and
draw caches before new guest threads start. Lost focus clears queued clicks
and ends a held direction through its release event. Timeouts use steady_clock
duration conversion (Windows ticks are not assumed to be nanoseconds).

## Original instruction evidence

Executable SHA-256:
`E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682`.
Ghidra evidence is private in `logs/ghidra-pc-menu-hooks.log`,
`logs/ghidra-pc-menu-instructions.log` and brightness counterparts.

| Hook | Verified original behavior |
|---|---|
| 824C4DB8–824C4FCB | Menu input, virtual player/button/release slots 0x58/0x44/0x48 |
| 82536DD0–825372E7 | Com_Frame, original frame counter 825F4E08 |
| 824CE4D0 | Leaf draw dispatch: `8163000C`, current index +12, pages +4, virtual +0x38, `bctr` at 824CE4F4 |
| 824CD9E0 | In-game draw dispatch, starts `8163000C`; original instruction stream inspected |
| 824CE3E8–824CE43F | Dialog draw |
| 824C80D0–824C813F | Quad draw, original part vertices supply controls' rectangles |
| 824D7D88–824D8103 | Multiline draw; line r3, vertical scale f3 supply footer bounds |
| 824CFD58 / 824BF4A8 | Text selected/normal scale and alignment; original font table 82A8DF30 |
| 824CC278 | Gamma page reads D-pad and stick, clamps 825CDAA4 using min 0.5 and max 825CE628; original gamma-ramp update |
| 824BC2C0 / 824BC248 | Gamma arrows animate opacity; PC directions supplement value changes |
| 824C3300 | Original exit compares brightness and retains original save/profile handling |

Short leaf methods missing as Ghidra functions were checked by raw instructions,
not assigned guessed boundaries. Existing codegen coverage supplies them; no
generated code or function boundary override edited.

## Validation

`Build.ps1 -Stage Tests`: 10 Python and 7 native tests passed, including the
new picture mapping test. It checks 16:9, 4:3 letterboxing, wide side bars,
stretch, safe-area crop and an empty window. Manual gameplay/menu acceptance
is still required; those geometry checks do not establish navigation behavior.

`Build.ps1 -Stage Build` also passed: strict `--ignore-stamp` analysis,
native executable and corrected GPU plugin. Logs: `pc-menu-tests.log`,
`pc-menu-build.log`, `pc-menu-final-link.log` under private logs/.
Normal-menu launch PID 33748, HWND 0x80f88, existing isolated profile
nightd_glow_query_01; `native-20261021-153520-751.log` confirms original
entry and `COD3_PC_MENU_ACTIVE` on actual title page (vtable 82071724).
User's initial response to the requested navigation/settings/gameplay check:
«Вроде всё нормально». At inspection the log still has only generation 1;
this is not independent evidence of a completed menu/Continue Runtime restart.
Focus loss while dragging/holding, mixed gamepad use, different pages and
inter-mission navigation need further acceptance.
