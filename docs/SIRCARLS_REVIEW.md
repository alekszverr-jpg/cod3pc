# SirCarls cod3_REcomp_ reference review

Reviewed 2026-10-20: [repository](https://github.com/SirCarls/cod3_REcomp_),
revision `2d6d40e6df380f8aa15365a6f308b47a8610ab38`.
GitHub reports fork=false, but the root README explicitly says it is heavily
inspired by FrankHUN88453/CoD3Recomp. This does not establish independent
provenance for every implementation. No external executable was run, no
campaign or performance claim reproduced, and no foreign code integrated.
Selected source evidence stays ignored under reports/sircarls-review/.

Product constraint clarified by the user on 2026-10-20: external reference
implementations are technical leads, not the desired presentation. All player
settings belong in the game's own styled/localized menus, with full mouse and
keyboard operation and binding-aware hints. Debug overlays remain developer
tools. See ROADMAP.md's PC-version acceptance criteria before adopting any
external UI, input or settings design. This review does not independently
assess the demonstration video's UI quality.

## Useful references and limits

The release's SDK patch notes pin ReXGlue to
`c94f5ebdcb3c9d1a460ca48e04f9758448f8d518`, the same base used here.
Their accumulated patch stack still differs from ours; shared base alone
does not make a patch safe to apply.

| Source under RELEASE/COD_3_WAW_Setup-tool/ | Observed implementation | Use for cod3pc |
|---|---|---|
| patches/0012-gpu-frame-stats-async-occlusion-queries.patch | D3D12/Vulkan queue query results with submission IDs, deliver after completion, submit/poll during idle waits, drain pending queries when disabling | Reference for readiness, fence ordering and teardown tests. It changes delivery timing; it is not evidence that our different-address ZPD snapshots or lantern visibility are fixed |
| patches/0024-gpu-resolve-readback-cpu-reads-only.patch and patches/README.md | Read-watch resolve destinations and arrange GPU readback when guest CPU accesses them; notes report one CPU-read destination in Saint-Lo and hypothesize light-flare visibility | Investigate whether our flare decision reads a resolved image/depth buffer instead of, or in addition to, occlusion-query results. The author's flare attribution is a hypothesis, not our measured result |
| patches/0006-gpu-scalar-operand-z-with-three-source-vector-ops.patch | General parser/interpreter change: scalar second component Z when paired vector operation has three operands; cites Xenia Canary PR 1149 and grass shader 4B4E6FDA02FB39C3 | Compare instruction evidence with our accepted grass and shadow-material corrections. Do not broaden our hash/pattern-scoped overrides from this reference alone; interpreter changes are not an application backend for this project |
| recomp/src/game/shadows.cpp | Original/off dynamic-shadow setting; off skips caster insertion while preserving light slots and map clearing | Optional future settings reference. This file does not establish a repair for through-wall shadows; our accepted material correction remains enabled |
| recomp/src/game/relaunch.cpp | Windows CreateProcessW; POSIX fork/execve with saved launch data | Process relaunch reference only. Does not replace our accepted same-process Runtime recreation |
| patches/README.md and release documentation | Additional work on resolution scaling, texture replacement, audio devices, timing and input | Candidates for later focused reviews, not accepted features or fixes in cod3pc. Keep original presentation and current campaign/rendering blockers first |

## Next glow investigation

1. Finish the pending original GetData probe: hidden lamp versus direct view,
   readiness/status, caller, query results and device flags. No forced visibility.
2. If that path does not explain the sprite, locate the original flare decision
   and its memory reads. Check resolve destinations, freshness, format, endian
   order and synchronization against the captured guest frame.
3. Use our existing opt-in `-FullGpuReadback` as a controlled comparison only
   when evidence points to resolve/readback data. Record hidden/direct behavior
   and frame time; a visual difference must be traced to the actual buffer.
4. Only then design a minimal correction with memory-lifetime and transition
   checks. Do not import the large read-watch/performance patch stack wholesale.

The external notes describe Vulkan runtime measurements and D3D12 build-only
validation for patch 0024. Neither proves behavior on our Windows/D3D12 scene.
Our earlier cumulative-query experiment did not remove the glows; keep it off
in standard launches. Pending manual testing remains deferred by the user.

## Additional review: input, audio and diagnostics (2026-10-20)

These are source-backed candidates at the same pinned revision, not features
accepted in our runtime. Source paths below are relative to
RELEASE/COD_3_WAW_Setup-tool/, unless prefixed docs/ (relative to RELEASE/).

| Candidate | Inspected evidence | Priority and acceptance here |
|---|---|---|
| Separate keyboard actions | docs/KEYBINDS.md and recomp/src/game/keybinds/binds.h: game command handlers for reload/use, crouch/prone, weapon slots, lean; controller-button fallback during interactions; releases on focus/menu changes | M7: independent R/F and C/Ctrl, configurable keys, hold/toggle ADS. Verify held keys across menus, Alt-Tab, turrets, vehicles and mixed keyboard/gamepad input; inspect our own original handlers before adding hooks |
| Raw mouse independent of stick limits | recomp/src/game/mouse_look.cpp consumes accumulated deltas once per guest usercmd, adds view angles, scales ADS by FOV, clears stale motion after menu/loading, yields camera control to QTE | M7: measure equal-distance turns at 30/60/120 FPS and during hitches, ADS sensitivity, no view jump after focus return. Do not import their timeout or sensitivity constants as established behavior |
| QTE mouse support and timing | recomp/src/game/qte_actions.h and qte_math.h define swirl/toggle/rowing cursor-to-stick behavior, device handoff and above-60-FPS progress corrections | M7 campaign readiness before affected missions: genuine valve, crane and rowing completion at supported rates, camera suppression and return, gamepad still usable. Demo/math alone does not prove a mission can be completed |
| Prompts follow actual bindings | recomp/src/game/button_prompts.h describes glyph replacement using current action keys and last input device | M7: localized hints must follow remapping and switch back to controller glyphs. Their English stick-hint replacements need adaptation to our Russian assets, not unconditional text replacement |
| Spatial audio investigation | docs/COD3_AUDIO.md maps game/NSL/FMOD listener and source positions, reported axis conversion (-y,z,x), six-channel output and suspected LFE duplication | M7 existing audio defect: confirm addresses and instruction behavior against our XEX, record each output channel, rotate around a stationary source, compare stereo/5.1 mapping and downmix. External measurements are leads, not a proved cause of our direction/volume issue |
| Audio continuity/device changes | patches/0061 adjusts XMA output-valid handling and callback pacing; 0032 exposes output-device choice; 0051 follows a system device switch | M7: first test repeated tails, underruns and callback intervals in our Windows output. Add device switch/unplug recovery later with no dangling callbacks across Runtime restart. Their PipeWire results do not validate our driver |
| Bounded hitch/hang reports | recomp/src/game/thread_profiler.h uses OS thread CPU time plus SDK wait counters; watchdog.cpp reports stalled game progress and repeats at bounded intervals | Diagnostic improvement: explain a hitch with thread/wait/GPU evidence and preserve a dump. Adapt ownership: their watchdog thread is never joined because process termination is assumed, incompatible with our repeated Runtime destruction |
| Consistent saved settings | patches/0042 separates one-run CLI values from persisted config; recomp/src/game/settings_sync.cpp synchronizes guest profile and archived crosshair cvar | M7/M8: establish explicit precedence for config, launcher override and live edits; test actual full-process restart as well as menu/Continue. Avoid stale guest pointers and callbacks after Runtime recreation |
| FOV/aspect ratio and render resolution | recomp/src/game/ultrawide.h describes Hor+ world projection, centered HUD/menu/movie handling and full-width damage/fades; docs/SCALING.md separates proposed CPU budgets from applied tuning | M8: wider view without stretched HUD, correct scopes, movies and viewmodel; resolution/render scale after GPU correctness. CPU placement/budget proposals are not measured improvements for our machine |
| Separate voice language | recomp/src/game/voice_language.cpp selects available owned-disc voice banks from next level load while retaining text language | M8 optional: resource completeness and fallback, including our modified Russian assets. Their listed en/de/fr/es/it support does not establish Russian voice compatibility |

Recommended order after glows: investigate the reported spatial audio defect,
then complete keyboard actions/prompts and QTE support as campaign content
requires, while adding bounded profiling when a measured hitch needs it.
Ultrawide, HD texture replacement, HDR and expanded language controls remain
optional later work. No new engine, World at War content or multiplayer scope
is needed to use these references.

## Source and license boundary

Pinned [patch notes](https://github.com/SirCarls/cod3_REcomp_/blob/2d6d40e6df380f8aa15365a6f308b47a8610ab38/RELEASE/COD_3_WAW_Setup-tool/patches/README.md)
and [release license](https://github.com/SirCarls/cod3_REcomp_/blob/2d6d40e6df380f8aa15365a6f308b47a8610ab38/RELEASE/LICENSE)
are the reviewed sources. Root metadata says MIT; the release license
separately assigns SDK patches to ReXGlue's BSD-3-Clause licensing with Xenia
portions. Review applicable notices and provenance before actual reuse.
Original/derived game code, generated functions and assets are not imported
or covered by an assumption about the repository's top-level license.
