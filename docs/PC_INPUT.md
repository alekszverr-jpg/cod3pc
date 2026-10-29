# Native gameplay input, first integration (2026-10-23)

## Binoculars and hold breath (2026-10-26, manual check deferred)

Added cod3_key_binoculars=B and cod3_key_holdbreath=Shift. Original type-2
command pairs +binoculars/-binoculars and +holdbreath/-holdbreath use the same
validated registry lookup, matched release, focus/menu cancellation, context
mapping and config save path. No authored sprint behavior is added.
KEYS now has 13 actions in three pages of six/six/one visible rows; original
pause entry count remains 19. Binoculars is last on page 2; hold breath is on
page 3. Forward/reverse wrap and partial last-page navigation are tested.

Own XEX evidence (SHA256 E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682):
strings +holdbreath/-holdbreath 8206A8C0/A8CC register 8250CD30 /8250CCF8;
+binoculars/-binoculars 8206A940/A94C register 8250CDB0 /8250CD68.
Both pairs are registered by original 8246CAA0 (type 2) and resolved at runtime,
not hardcoded for delivery. 8250CCF8 checks the local player's entity/client
and clears the original hold-breath bit at client+53C; no invented return or
replacement game state is used. Private evidence:
logs/ghidra-binocular-breath-strings.log, ghidra-binocular-breath-handlers.log,
ghidra-holdbreath-release.log, existing ghidra-mouse-combat-register2.log.
Command names/default-binocular key source: SirCarls 6ae4839 model.cpp;
existing MIT notice applies. Shift selects the original hold-breath command.

23 tests passed (10 Python +13 native): logs/binocular-breath-tests.log.
Strict --ignore-stamp codegen and native/GPU build passed:
logs/binocular-breath-build.log. New checks cover all 13 entries in both
navigation directions, last page of one entry, independent binocular release
and blocked hold-breath release. One normal menu launch with existing
nightd_glow_query_01 profile, PID 277180.
User explicitly deferred manual checks: «Пока проверить не могу, проверю по
возможности». Binocular behavior, optic hold-breath effect, three-page layout,
reassignment persistence and pause/Continue regression remain unaccepted.
Do not repeat the launch or close this session solely for the deferred check.

2026-10-26: исправление камеры при отбрасывании гранаты принято пользователем:
«Всё нормально» в ответ на повторную проверку поворота во время таймера,
отпускания G, обычной камеры, стрельбы и F после анимации. Ранее пользователь
подтвердил обычную/дымовую гранаты, ближний бой и меню назначений. Эта приёмка
закрывает найденную потерю мыши во время таймера в проверенной сцене Night Drop;
полный транспорт/QTE остаются отдельными проверками. Код после приёмки не
менялся; 23 tests, strict codegen и native/GPU build прошли до запуска.
См. PC_INPUT.md.

## Grenades, melee and paged bindings (2026-10-26)

Added cod3_key_frag=G (+grenadeattack), cod3_key_smoke=4 (+smokegrenade),
cod3_key_melee=V (+melee). All use original registered type-2 down/up pairs,
with existing context-pad routing, release/cancellation and SDK config saving.
KEYS now has 11 actions in pages of at most six existing-style rows. Up/down
at a page edge changes page; all capture routes translate visible rows through
the current page offset. Title shows 1/2 or 2/2. Original VIDEO remains four
rows. Native pause entry count is 19 (six reused row slots), not 11 cloned rows.

Own XEX evidence (SHA256 E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682):
+grenadeattack/-grenadeattack strings 8206AAC0/AAD0 -> 8250D978 /8250C8B0;
+smokegrenade/-smokegrenade 8206A920/A930 -> 8250DA90 /8250C908;
+melee/-melee 8206A8E8/A8F0 -> 8250CAE8 /8250B9D8.
Callbacks are looked up dynamically; these are verification addresses.
8250D978's catch branch holds kbutton 829C9030 with key 211 (controller X),
sets 82A2A2E7=1; 8250C8B0 only releases 829C8FB8 and clears that flag.
+activatereload/-activatereload strings 8206AB20/AB30 register 8250CA80 /
8250B9A8. The reference's extra release is adapted: only if the host grenade
press changed X from not held to held, invoke the real -activatereload(211,time)
on release while X is still held. An already-held X is not claimed by this
cleanup. Tests cover ownership/reset/release once. This cleanup branch was not
observed in the first runtime trace, so it is not declared manually covered.
Source leads: SirCarls 6ae4839 model.cpp and binds.cpp; existing MIT notice.

Private Ghidra evidence: ghidra-grenade-melee-strings.log,
ghidra-grenade-melee-handlers.log, ghidra-grenade-throwback.log,
ghidra-grenade-melee-leaves.log, ghidra-throwback-use-command.log,
ghidra-throwback-use-registration.log, ghidra-throwback-use-release.log.
First build/test evidence: logs/grenade-melee-compile.log,
grenade-melee-tests.log (23 passed), grenade-melee-build.log (strict +native/GPU).
PID 368172, existing Night Drop checkpoint profile. User reports other checks
normal (G/4/V, paged menu/capture, releases); reports loss of mouse only during
the enemy-grenade timer, explicitly confirms mouse returns after the animation.
Logs show interaction=true 19:01:06.383 -> false 19:01:08.434 and raw mouse
switching to/from stick emulation. No permanent capture loss is established.

Follow-up fix: the blanket interaction gate was too broad for the intended PC
camera. Read original catch flag 82A2A2E7 and latch that particular interaction;
keep direct raw camera until it ends, including after G release. Pause does not
falsely end the latch. Ordinary MG42 and other interactions retain their prior
stick mapping. This follows the reference's distinction between normal view
and stick-driven QTEs; it is not a complete QTE port or a claim that the original
console camera is always free. New diagnostics COD3_THROWBACK_VIEW and
COD3_THROWBACK_MOUSE_APPLIED establish actual raw deltas during a future test.
Focused test logs/throwback-mouse-compile.log verifies catch/start/release/end,
reset, unrelated interaction and retaining state across pauses. Full build/tests
logs/throwback-mouse-build.log and throwback-mouse-tests.log passed (23 tests,
strict codegen, native/GPU). Updated normal menu launch PID 334256 with the same
checkpoint profile. Manual timer-camera acceptance is pending in that build.

2026-10-26: автоматическое отключение помощи для KB/M принято пользователем:
«Всё отлично, проверил» в ответ на проверку прицеливания/поворота, паузы и
возврата (геймпад был предложен при наличии). Лог текущего PID 102228 содержит
COD3_MOUSE_AIM_ASSIST suppressed=true -> false -> true, при каждом переходе
profile_before=1 / profile_after=1. Это подтверждает переключение источника и
восстановление флага; отдельного подробного описания ощущений на геймпаде
пользователь не дал. Код после ручной приёмки не менялся; 23 tests и сборка
прошли ранее. Следующий перенос — гранаты/ближний бой, с отдельной проверкой
освобождения взаимодействия при отбрасывании гранаты. См. PC_INPUT.md.

## Automatic mouse aim-assist policy (2026-10-26, pending manual acceptance)

User accepted weapon wheel/reassignment/menu/focus/Continue checks, then
requested disabling aim assistance for KB/M. New default
cod3_mouse_disable_aim_assist=true suppresses original Sticky Aim while KB/M
is active and mnk_mode is enabled. Actual physical pad activity restores the
original profile's decision, including its saved Off choice.

Sticky Aim is controls byte 82A8E610 +67. Original aim routine 8248D138 reads
it inline at 8248D970 (88EB0043, lbz r7,0x43(r11)) and stores a local copy at
8248D97C. The raw hook runs the complete original routine with a scoped zero
in that byte, then restores the exact previous value, including unwinding.
It does not permanently change or save the profile's controller setting.
Original Gameplay menu continues to display/edit that profile setting.
Ghidra's decompilation of 8248D138 truncates at unsupported VMX128 8248D14C;
we used raw instruction evidence at the genuine existing entry and profile
load rather than treating that truncated listing as the complete routine.
Evidence: logs/ghidra-mouse-aim-instructions.log,
ghidra-mouse-aim-profile-read.log and generated original code inspected locally.
XEX SHA256 E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682.
The Sticky Aim field/routine were leads from SirCarls aim_assist.cpp and
addresses.h, revision 6ae4839; the automatic device policy is project-authored.

New SDK patch rexglue-v0100-input-activity.patch observes each actual device
before merging its state. Synthetic devices (MnK/NOP) never count as a physical
pad; neutral drift and repeated held states do not take ownership from a mouse.
Stick changes accumulate to 512 units above SDK deadzone, trigger changes use
the SDK activation threshold. UI key/button/wheel events and filtered raw mouse
counts mark KB/M activity. Raw counts are collected before original angle
adjustment; angle application still happens after it. For the one SP KB/M
client this marker follows whichever original pad pressed Start. Runtime reset
clears the detector. Existing menu-glyph heuristics are separate from this
source-aware gameplay policy. No Interpreter/JIT or new backend introduced.

Matched SDK build/staging and original header updated by installer.
23 tests passed: logs/mouse-aim-tests.log, input-activity-sdk-build.log.
Tests exercise synthetic versus physical input, held pad after mouse takeover,
neutral/jitter, activity on another user, reset, policy truth table and exact
profile restoration on ordinary/exception exits. Native/strict/GPU build:
logs/mouse-aim-build.log; final active-pad source adjustment compiled/linked
with target cod3pc in logs/mouse-aim-final-app-build.log.
One normal menu launch with existing Night Drop profile, PID 102228.
Runtime log COD3_MOUSE_AIM_ASSIST records effective suppression and profile
before/after. Manual mouse feel/menu return and physical-pad comparison pending.

## Weapon wheel (2026-10-25, pending manual acceptance)

Adapted the next/previous weapon actions from SirCarls 6ae4839 model.cpp and
binds.cpp (kText delivery). Defaults: cod3_key_weapnext=MWHEELDOWN,
cod3_key_weapprev=MWHEELUP. Existing native KEYS page now has eight actions
(21 total pause entries); its two extra rows accept keyboard, mouse buttons
or either wheel direction. VIDEO still has four rows. No foreign UI added.

Wheel input runs before menu navigation and SDK mapping. During explicit
capture it consumes scrolling instead of moving the menu selection. Outside
capture, menu scrolling remains on the existing menu path. The host-only
WheelDetents accumulates 120-unit detents, resets on focus/menu boundaries,
and caps a pathological individual event at 16 detents. Each detent produces
a down/up pair in the bounded action queue; it cannot become a held mouse
button. Fractional events and rapid successive detents are covered by tests.
MWHEELUP/DOWN use host-only tokens 0x100/101, never passed as guest key ids.

On foot the game receives original weapnext/weapprev text through Cbuf_AddText
824644D0, with a terminating newline; the original game executes the queue.
Own Ghidra evidence logs/ghidra-wheel-command-buffer.log confirms this function
copies bytes immediately into the original bounded command buffer, so the
host Call's guest-stack text lifetime is sufficient. XEX SHA256 remains
E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682.
Interaction mode uses the accepted original-pad route; weapprev falls back
to the profile's weapnext controller button, matching the reference. This
is not a claim of reverse weapon selection in every vehicle/context.

23 tests passed: logs/pc-wheel-tests.log. Added wheel token round trips,
fractional accumulation, reversal, reset, extreme delta bounds and multiple
weapon impulses without repeats after menu cancellation. Build/strict codegen
and native/GPU checks passed: logs/pc-wheel-build.log. One normal menu launch
with the existing nightd_glow_query_01 profile, PID 336236. Manual weapon switching,
reassignment, eight-row layout, menu wheel and pause/focus regression pending.
Grenades/melee remain next work; the reference's grenade throw-back release
quirk needs separate original-code evidence before adoption.

2026-10-24: пользователь принял проверку контекстного ввода MG42:
«Всё работает отлично» в ответ на вход/выход по F, короткую и удерживаемую
стрельбу ЛКМ, отпускание, наведение мышью, обычное оружие и паузу/возврат.
Это приёмка проверенной сцены MG42; прочий транспорт и QTE ещё не проверены.
23 tests, strict codegen и native/GPU build прошли до проверки. Код после
приёмки не менялся. Детали реализации и диагностики — PC_INPUT.md.

## Interaction input routing (2026-10-24, pending runtime acceptance)

The previous `!interaction` gate disabled all six independent actions during
mounted/context scenes. Adapted SirCarls 6ae4839 binds.cpp's delivery choice:
resolve the current game's controller binding for the action (exact command
first, then known layout equivalents), then merge that button/trigger into
successful original pad reads. Native on-foot delivery stays direct. Each
press retains its original delivery route until release across a context
change, and shared pad buttons remain held until all contributing actions
release. Short taps survive one poll through the existing bounded edge queue.

New project-authored pc_context_input.h contains the route model, known pad
key/XInput bit mapping, resolver and endian merge. Existing KEYS assignments
apply; no replacement menu or fixed RT/X layout is imposed. Physical button,
trigger, packet and stick state is preserved. Injected state is local-client-0
only and targets the active original pad; focus/menu/native-actions gating
suppresses it. Reset clears all route/mask state before a new Runtime.

Own XEX SHA256 E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682:
8250F8B0 / 82521F00 confirm qkeys 82A59570, stride 0x804, key*8 and binding +4.
820E7C68 confirms the input pointer 8290A910; original poll 82522138 uses
input+28/+24 to choose the pad. 82344F98 instructions move r4 to r5, set
flags r4=0 and tail-branch to 82578A5C (XamInputGetState); the existing hook
therefore receives user/state before the import and merges only after success.
Interaction state at 82A59DA0, +4 is confirmed in 82521F00. Private evidence:
logs/ghidra-context-pad-references.log, ghidra-context-pad-layout.log,
ghidra-context-pad-read.log. SDK X_INPUT_STATE layout supplies big-endian
buttons +4, LT/RT +6/+7, sticks +8. Existing menu prompt observation runs before
injection, so synthetic input does not become a physical controller event.

23 tests passed (logs/pc-context-tests.log): alternate fire layouts, use/reload
aliases sharing X, held fire/ADS, no direct command duplication in interactions,
press-route continuity when entering/leaving, blocked release, short taps,
reset and endian merge preserving hardware state. This verifies host routing,
not MG42 gameplay or the complete transport/QTE controls. Native/strict/GPU
build passed: logs/pc-context-build.log. One normal menu launch, PID 19232,
with existing isolated checkpoint profile nightd_glow_query_01. Manual mounted MG42, mouse look,
exit/re-entry, pause/focus and normal on-foot regression remain to be checked.

2026-10-23: пользователь принял независимые стрельбу на ЛКМ и удерживаемое
прицеливание на ПКМ: «Сейчас всё нормально работает» в ответ на проверку
обычного оружия, совместной стрельбы/ADS, строк меню и возврата из паузы/Alt-Tab.
Приёмка относится к этому сценарию; управление MG42, транспортом и QTE ещё
нужно проверить отдельно. Код после этой проверки не менялся.

## Native fire and held ADS (2026-10-23)

User acceptance of the previous mouse-button extension was limited: button
assignment worked, but default LMB/RMB did not fire/aim. That extension only
had four actions and did not establish working combat mouse defaults.
Added independent fire (cod3_key_fire=MOUSE1) and held ADS
(cod3_key_aim=MOUSE2), using +attack/-attack and +speed/-speed through the
same bounded original command registry lookup. Six action rows now appear in
the native KEYS page (19 total pause items); VIDEO still displays four rows.
All six commands share focus/menu/Runtime release handling and config saving.
Original interaction/controller path is retained, so mounted guns, vehicle and
QTE bindings require separate context work/acceptance. No ADS toggle option yet.

Own XEX evidence (SHA-256 E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682):
strings +speed/-speed 8206A8B0/B8 and +attack/-attack 8206A8D8/E0;
registration observed around 8250EFC0 via 8246CAA0 (type=2, callback at +12).
Resolved callbacks: fire 8250CB18 / 8250BA08, ADS 8250CB68 / 8250BA38.
Leaf instruction evidence confirms argument shuffling to original kbutton
press 8250BB50 (fire slot +138, ADS +D8) and release handlers. The code looks
up actual registered callbacks at runtime rather than hardcoding these four.
Private Ghidra logs: ghidra-mouse-combat-strings.log,
ghidra-mouse-combat-register2.log, ghidra-mouse-combat-callbacks.log,
ghidra-mouse-combat-instructions.log. Reference command names from SirCarls
6ae4839 model.cpp; existing MIT notice applies.

23 tests passed (10 Python +13 native), strict --ignore-stamp codegen and
native/GPU build passed. Tests now cover simultaneous fire+ADS, independent
release, blocked release, short clicks and action-count bounds. Commands:
tools/Build.ps1 -Stage Tests; tools/Build.ps1 -Stage Build.
Logs: pc-combat-tests.log, pc-combat-build.log, pc-combat-compile.log.
One normal launch with existing Night Drop checkpoint profile, PID 18080;
log confirms original fire press/release delivery. Manual fire/ADS/menu/focus
acceptance pending; delivery logs alone do not establish gameplay behavior.

## Mouse-button assignments (2026-10-23)

Pause > Options > KEYS now captures all five physical mouse buttons for the
four currently implemented independent actions. Config/display tokens are
MOUSE1 (left), MOUSE2 (right), MOUSE3 (middle), MOUSE4 (X1), MOUSE5 (X2).
Existing SDK aliases LMB/RMB/MMB still parse to the same identities, including
duplicate checks. Keyboard assignments remain compatible. Wheel assignment,
additional action rows and vehicle-specific assignments remain pending.

The z64 input listener owns explicit capture before native menu navigation
(z63): RMB binds instead of navigating Back, and LMB binds instead of clicking
a second menu item. In permitted on-foot play it consumes a bound button down
before SDK pad mapping, preventing e.g. use+fire or reload+aim on the same click.
Unbound buttons retain the original SDK mapping. All button ups propagate to
SDK so any prior fallback press can release. Native held actions use the same
bounded edge queue and menu/focus/Runtime release paths as keyboard actions.
Interactions still use the original SDK controller path; this is not a vehicle
or QTE button remapping claim. No SDK API/guest offsets/generated code changed.

Verification: tools/Build.ps1 -Stage Tests (10 Python +13 native passed),
tools/Build.ps1 -Stage Build (strict --ignore-stamp codegen, native/GPU).
Logs: logs/pc-mouse-bindings-tests.log, pc-mouse-bindings-build.log,
pc-mouse-bindings-compile.log. The production token helper is tested for all
five distinct physical buttons, config round trips, legacy aliases and invalid
input; existing action tests cover taps/repeats/blocked releases/overflow.
One normal launch with the existing nightd_glow_query_01 profile, PID 1892.
Manual button assignment/duplicate suppression/focus/Continue check pending.

Scope: independent reload R, use F, toggle crouch C, toggle prone Ctrl; raw
mouse camera; keyboard reassignment of these four actions in Pause > Options >
KEYS. This is not the complete action/vehicle/QTE port. Menus retain PC mouse
navigation. Interactions preserve the existing SDK controller-button path.

Command lookup/delivery and mouse math are adapted from SirCarls revision
6ae4839cc780664fd9a1b039173083f0dd4fc9c1, game/keybinds/binds.cpp and
mouse_look.cpp. MIT notice: licenses/SirCarls-COD_3_WAW-MIT.txt. The bounded
edge model, SDK count bridge and lifecycle integration are project-authored.
Original/generated functions and assets are not imported.

## Original evidence

XEX SHA-256 E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682.
Private Ghidra logs: ghidra-pc-input.log and ghidra-pc-actions.log.
82522138 polls input and checks in_joystick (829C295C), poll-off byte
82A2A17D and frontend blocking object 82A8DF4C (+28 client flag, +12 mode 3).
Only local client zero receives host actions. Interaction state is read from
82A59DA0, state +4; active interactions keep the previous input path.

Cmd_AddCommand 8246CBA8 confirms linked command nodes at 829EBA10:
next +0, name +4, type +8, callback +12. Lookup is bounded, validates callback
image/alignment and type (button 2, plain 1). Reload/use invoke their real
press/release callbacks with AUX1 holder 239 and original Sys_Milliseconds
82468510. Crouch/prone invoke original plain commands on the press only.
Missing commands retain the SDK keyboard path and are retried periodically.
No empty success/import or guessed callback substitutes.

CL_CreateCmd 82510748 calls CL_AdjustAngles 8250B460, then movement and
CL_MouseMove 8250FF48. Raw motion is added after original AdjustAngles, before
the original final pitch clamp. FOV hook 8243A198 observes the original result
and base cg_fov value 82A11360 for ADS scaling. Camera angles are clientActive
82A57900 + local*7280 +5732; original profile settings 82A8E610 +68/+72 and
invert +65 also apply. Mouse counts are not multiplied by frame time.

## SDK and lifetime

Patch tools/patches/rexglue-v0100-raw-mouse-delta.patch extends the pinned
SDK MnK driver: SetRawMouseLook selects a count buffer instead of mouse-to-
stick conversion, TakeRawMouseDelta consumes it once, ClearRawMouseDelta
cancels it. The SDK alone owns cursor capture, SDL relative mode and fallback
recentering. Real controller stick input and keyboard stick rows are unchanged.
Mode changes clear pending counts. No UI callback receives a guest pointer.

UI listener z64 handles bound keys above the existing PC menu listener z63
only during on-foot play, except explicit binding capture. Releases always
reach the SDK. Menu/overlay/focus changes release held actions; bounded queue
overflow releases all, sub-frame taps survive one poll. Unfinished capture
cancels on focus/overlay/selection/back. Duplicate keys are rejected and capture
continues; WASD, Tab, Space, Enter, Escape, console and function keys are reserved.
The initial page accepted only keyboard keys; the mouse-button extension above
adds five buttons. Wheel reassignment remains pending.

SDK-backed CVars cod3_key_reload/use/crouch/prone persist through the existing
UI-thread config writer. cod3_native_actions and cod3_raw_mouse default true.
Raw sensitivity defaults 5 on the WaW scale (0.022 degrees/count at 1), with
ADS FOV ratio and cod3_zoom_sensitivity. The game's native horizontal/vertical
and invert settings apply. Extra host tuning is not yet a full native control
page. Gameplay hints still need action-key localization; vehicle aliases,
QTE mouse adaptation and all remaining actions remain pending.

Every Runtime resets handler pointers, event state, capture and counts before
guest threads start. UI listener detaches before presenter retirement, and
pending binding saves flush through the same config writer. No old Runtime
callback or page pointer remains in UI state.

## Verification

Full SDK build/staging, strict --ignore-stamp codegen and native/GPU build
passed: logs/pc-input-sdk-build.log, pc-input-codegen.log, pc-input-build.log.
22 tests passed (10 Python +12 native), logs/pc-input-final-tests.log.
New tests cover action taps, repeats, blocked releases/overflow, partition-
independent mouse math, ADS/inversion and nonfinite input. The actual SDK-driver
test feeds MouseEvents through MnkInputDriver and checks raw delivery once,
multiple polls accumulated, no mouse stick contribution in raw mode, fallback
stick conversion and explicit clear/disabled-look behavior. It does not prove
physical relative-mode feel or campaign/QTE behavior.

One normal menu launch with existing nightd_glow_query_01 profile:
logs/native-20261023-090700-065.log, PID 58724, HWND 0x280980, generation 1.
The original command list resolves all four callbacks:
+reload 8250CA50 / -reload 8250B978;
+activate 8250CAD0 / -activate 8250B9C0;
togglecrouch 8250E0D0; toggleprone 8250E030.
Pause page initialization reports 17 items. Saved DoF is 50% at this launch.
These prove original entry and handler availability, not manual gameplay.
Pending acceptance: raw mouse/ADS/focus return, separate actions, native key
capture and menu/Continue persistence without a new window.


## Manual feedback and capture correction (2026-10-23)

The user initially reported normal operation/layout, then explicitly corrected
that mouse capture does not work in gameplay. Therefore raw mouse acceptance
is NOT established. The first launch log confirms all four action deliveries,
KEYS page controls=true, reload bound to T and back to R; it contains no
COD3_RAW_MOUSE_APPLIED. Generation remains 1 in the inspected log, so a new
Runtime is not inferred from the general response.

Root cause: production config leaves legacy mnk_mouse=false. The new driver
bridge originally buffered counts AND requested capture only inside the
legacy mnk_mouse gate. The regression test originally set it true, missing
the production case. Corrected the bridge so raw mouse requests capture and
count delivery independently of that legacy setting. SetRawMouseLook now
also accepts emulate_stick for active interactions: capture remains available
and counts follow the original right-stick path there; on-foot frames use
raw camera counts. Mode switches clear pending movement.

Updated actual-driver test starts with mnk_mouse=false and checks raw count
delivery with zero stick contribution, and separately checks interaction
stick fallback with the legacy flag still false. This tests driver delivery,
not physical SDL cursor capture. SDK build and isolated actual-driver test
passed: logs/pc-input-capture-sdk-build.log,
pc-input-capture-compile.log, pc-input-capture-driver-test.log. User authorized
normal close/install/relaunch; full build and manual camera capture check follow.

Full capture-fix native/GPU build including strict codegen passed
(logs/pc-input-capture-build.log), all 22 tests passed
(logs/pc-input-capture-tests.log). After authorized normal close, one process
PID 16920 / HWND 0x25005e opened with the same nightd_glow_query_01 profile.
logs/native-20261023-091526-539.log confirms generation 1, DoF 50%, and all
four original action callbacks found. Physical capture/turning remains the
pending manual check for this corrected build.


## Corrected build accepted (2026-10-23)

User response to capture, normal/ADS turning and pause release/recapture check:
"Вот теперь всё нормально". Log native-20261023-091526-539.log records
COD3_RAW_MOUSE_APPLIED at 09:16:09.690, dx=5 dy=1 sensitivity=5 zoom_ratio=1,
confirming actual camera count delivery rather than mode initialization alone.
The first raw-mouse integration is now accepted for the tested scene and pause
return. Inspected log still shows generation 1; full-process restart and a new
Runtime/mission with this input change are separate acceptance checks. Vehicle
aliases, special QTE adaptation and the remaining action bindings are pending.
