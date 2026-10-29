# Native menu keyboard prompts

## Gameplay action words (2026-10-27, checked scene accepted)

User replied «Да, всё работает нормально» to the requested [F] / Russian
text / F-to-T-to-F rebinding / menu-Continue check. This accepts the tested
scene, not every action word or physical-controller switching. No code changes
after that test. Binocular/hold-breath behavior remains independently deferred.

Added cod3_game_key_prompts (default true). Original 824DD9A8 runs first;
supported [ACTION], [PRESS_ACTION], [VERB_ACTION], [HOLD_ACTION] words then
use the current native binding. Supported: USE/ACTIVATE, RELOAD, ATTACK,
AIM/TOGGLE_AIM, WEAPON_SWAP, GRENADE, SMOKEGRENADE, MELEE, BINOCS.
Unavailable handlers/keys, unknown words or missing verbs retain original
output. Movement, stance and special QTE prompts remain outside this step.

Gameplay device selection follows SDK physical-device activity, like aim
assist; menu footer mapping stays separate. Verbs use current-game localization
INGAME_PRESS / INGAME_PRESS_AND_HOLD via 8252CB98. Strings have current-Runtime
heap lifetime, bounded cache, reset between Runtimes. Original text tracking
refreshes on device/binding changes (binding polling once per second).

Own Ghidra evidence, same XEX SHA256 as below:
logs/ghidra-gameplay-action-prompts.log and
logs/ghidra-gameplay-prompt-instructions.log. Confirmed entry 824DD9A8,
action table 82A8DFF8, char* result, localization call 8252CB98 and table
global 82A2A14C. Model adapted from local SirCarls d54c1bb reference
button_prompts.cpp/keybinds/binds.cpp; existing MIT notice applies.

23 tests passed: logs/gameplay-prompts-tests.log (word forms, unknown-word
fallback, rebound keyboard/mouse formatting). Strict --ignore-stamp codegen
and full native/GPU build passed: logs/gameplay-prompts-build.log.
One normal menu launch, previous nightd_glow_query_01 profile, PID 137780,
logs/native-20261027-094226-596.log. User acceptance is recorded above.
No assets/generated functions changed.

Menu-only adaptation of SirCarls/cod3_REcomp_ button_prompts.cpp and layout
tracking, revision d54c1bb877ab364dae2d2f40aa8c6a8ca46e3070.
Copyright 2026 COD_3_WAW contributors; MIT notice in
../licenses/SirCarls-COD_3_WAW-MIT.txt. No generated game code/assets imported.

cod3_menu_key_prompts enables native text labels in the original menu footer.
Enter/Esc/arrows describe fixed menu controls; other supported tokens use SDK
key bindings. Gameplay action prompts are outside this change. Keyboard/mouse
activity selects PC labels; original pad state is observed without modifying
it. Real gamepad switching still requires manual acceptance.

Native layout, word measurement and drawing temporarily use the text font
instead of the button font, then restore it. Wrapped text rebuild preserves
its original floating-point scale. Binding changes trigger relayout. Guest
strings and object trackers reset before new Runtime threads; the UI listener
does not read guest memory. The existing pc_menu line-draw hook delegates to
menuprompts::DrawLine, avoiding a duplicate hook.

Reviewed original hooks: 824C68E0 token text, 824D7650 layout, 824D7B48 measure,
824CFA00 multiline draw, 824E8180/824E8098/824DD790/824DD630 text setters,
824DEAD0 construction, 820AD1B8 destruction, 82344F98 pad-state observation.
Original XEX SHA-256:
E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682.
Private evidence: logs/ghidra-menu-prompts.log and
logs/ghidra-menu-prompts-instructions.log.

On 2026-10-22 the user accepted menu prompts, pause/video navigation and
return to gameplay with camera/audio. Their screenshot confirms [ENTER] and
[ESC] rendered cleanly in the original Russian menu. Full build with strict
codegen and all 19 tests passed; see VIDEO_MENU.md for exact build logs and
fresh-launch persistence evidence. Broader gameplay bindings, remapping and
mixed-device validation remain pending.
