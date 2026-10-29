# SirCarls cod3_REcomp_ reference review

## Update review, 2026-10-28

Read GitHub compare through the connected GitHub tool, from previously reviewed
6ae4839cc780664fd9a1b039173083f0dd4fc9c1 to
a65c3cba9a3d5cf50f5f9ca10b5c7e073f22b037. Two commits: Nice8
(9cccd6d, Oct 24 UTC) and Nice9 (a65c3cb, Oct 27 UTC), 152 changed files.
Source-only review; no foreign build, code integration, asset import or change
to the pinned reference checkout. Existing cod3pc runtime left alone.
Compare: https://github.com/SirCarls/cod3_REcomp_/compare/6ae4839cc780664fd9a1b039173083f0dd4fc9c1...a65c3cba9a3d5cf50f5f9ca10b5c7e073f22b037

Most relevant additions under RELEASE/COD_3_WAW_Setup-tool/recomp/src/game:

- server_step.cpp and movement/hooks_sp.cpp: separate 60 Hz campaign/server
  and player movement steps from rendering, with interpolation. Includes draw
  flags, local packet handling, vehicle view/linked-part smoothing and special
  systems between steps. physics_step.cpp additionally restores ragdoll and
  wheel poses between physics updates. High priority correctness reference;
  substantial coupled change, not a single timing constant to copy. Recheck
  original SP instructions and reset every history/pointer on our same-process
  Runtime restart. Validate 30/60/120/144 FPS, AI, stairs, pickups, vehicles,
  deaths and mission transitions before enabling.
- coronas.cpp: default-on time-scaled corona fades above 60 FPS. Separate
  cod3_corona_trace is explicitly default OFF: rays for corona visibility and
  ADS focus replace GPU depth reads; budget 240 rays/second, cached visibility
  for 150 ms or until eye movement of 16 units. A useful experimental lead for
  our accepted readback_resolve=fast path, not proof of equal visual results,
  immediate visibility or zero cost. Test fences/foliage/characters, focus,
  latency and frame time; preserve our current path for A/B.
- anim_clock.cpp: default-on UI fallback clock correction from vblank count
  to real time. Separate frame-material-parameter correction defaults OFF;
  world scene time was already real time according to source. Do not describe
  this as a proven universal grass/water animation fix.
- sound_stall.cpp: changes original NSL stuck-sound timeout from 32 to 1000 ms
  to avoid restarting voices during a mixer stall. Independent of our accepted
  XMA output-valid MG42 fix; use if repeated dialogue is actually reproduced.
- voice_language.cpp: refresh retained sound banks at level load after language
  changes. Supported choices still do not establish our Russian asset support.
- fov.cpp/viewmodel_flip.cpp: vehicle FOV multiplier and optional mirrored
  left-hand weapon with scoped material culling correction; mirrored markings
  remain backwards. Later native in-game settings candidates.
- button_prompts.cpp: optional key-cap overlay presentation and improved
  device observation during briefing skip. cutscene_skip.cpp now restricts
  normal skip to briefing movies; this is an author product choice, not a fix
  to adopt automatically over our user's accepted behavior.

SDK patch notes: 0072/0120 reduce per-draw profiling overhead; 0121 sampler
caches and 0124 unchanged-fetch optimization specifically target Vulkan.
0123 mixes common-path savings with Vulkan-specific changes. The author's
reported tank-scene 127 -> 153 FPS is their result, not a prediction for our
D3D12 machine. 0110 asynchronously reads optional replacement textures, not
the original texture path; 0111/0112 add diagnostic timers. render_list_guard
cuts invalid render lists to avoid a crash: diagnostics may help investigation,
but do not import crash suppression as a lifetime fix under project rules.

Rust rendering work remains experimental: SDK 0100 adds host callbacks; pinned
RELEASE/engine/crates/cod3_render/Cargo.toml describes M1 step 2 as taking and
counting draws, drawing nothing yet. No complete replacement renderer claimed.
Keep the official Xenos GPU backend. Multiplayer/split-screen settings remain
outside our initial single-player scope.

Recommended review order: fixed-rate movement/server/physics correctness,
then a controlled corona/readback comparison and UI clocks. Optional FOV,
mirrored weapon and visual key caps follow core correctness. Current pending
manual binocular/hold-breath tests are unchanged. No new runtime acceptance
or performance measurement is asserted by this review.

2026-10-23: адаптирована XMA output-valid часть patch 0061 из
reference main 6ae4839cc780664fd9a1b039173083f0dd4fc9c1: отдельный
`rexglue-v0100-xma-output-valid.patch`, сохранение валидности пустого кольца
при отсутствии записанного PCM, включая consume-only fullness. Сохранены
наш StoreContextMerged и остальные SDK patches. Callback pacing и 0066 не
переносились. Actual SDK regression до/после воспроизводит разницу; ручной
результат MG42 ещё ожидается. См. AUDIO.md и MIT notice.

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


## Update review, 2026-10-22

Fetched origin/main without changing the pinned reference checkout or build.
Compared d54c1bb877ab364dae2d2f40aa8c6a8ca46e3070 to 6ae4839cc780664fd9a1b039173083f0dd4fc9c1.
Six new commits, 80 changed files. Latest commit changes MULTIPLAYER.md;
substantive source changes are in Nice5, Nice6 and Nice7. No new code imported.

| Candidate | Inspected source under RELEASE/COD_3_WAW_Setup-tool/ | Relevance here |
|---|---|---|
| Native sliders with numeric entry | recomp/src/game/menu_slider.h/.cpp and menu_fov.cpp | High UI priority: drag, type a value, Enter apply, Esc cancel. Adapt to Russian layout and current Runtime ownership |
| Vehicle-specific keys | recomp/src/game/keybinds/binds.cpp, model.cpp; RELEASE/docs/KEYBINDS.md | High campaign priority: tank coax/turret, jeep gas/brake/handbrake, view switch; avoid stealing on-foot actions. Requires original SP handler review and vehicle runtime tests |
| Per-weapon hip/ADS sensitivity | recomp/src/game/aim_sensitivity.h/.cpp and menu_aim.h/.cpp | Useful native control option; neutral 100/100, blend by aiming fraction. Own mouse/action integration and transitions need validation |
| Fixed physics stepping | recomp/src/game/physics_step.cpp | Author describes glass/boxes/doors behaving badly at high FPS. New hook accumulates time and calls physics at 60 Hz, caps catch-up. Candidate correctness investigation, not a proven fix for cod3pc; check whole call state, mission transitions and actual objects at 30/60/120+ FPS |
| FPS-independent Sticky Aim | recomp/src/game/sticky_aim.h/.cpp | Candidate timing correctness review; separate from sensitivity, preserve user's off choice and validate original behavior before changes |
| Motion blur and DoF toggles | recomp/src/game/post_effects.h/.cpp, menu_video.cpp | Optional motion-blur setting fits VIDEO. Our existing graduated DoF control remains useful; foreign binary toggle is not a required replacement |
| Controller families and rumble | patches/0067-input-the-kind-of-controller-each-player-holds.patch; recomp/src/game/rumble.h/.cpp | Xbox/PlayStation/Nintendo labels and no pad rumble while using keyboard. Requires SDK ABI review and actual mixed-device tests |
| Weapon position/custom crosshair | recomp/src/game/viewmodel.h/.cpp, menu_aim.h | Optional personalization after baseline controls; offset fades out during ADS. Separate viewmodel FOV explicitly not implemented there |

MP menu/options work is substantial but outside our initial SP scope.
relaunch.cpp still uses Spawn/CreateProcessW (existing implementation); its
new change only limits start_in_background to launch options. No evidence of
same-process transitions in this update. No audio source changes in this diff;
it supplies no new MG42-loop fix.

Suggested follow-up: finish current UI acceptance; add native slider/number
input and action bindings with vehicle contexts; investigate physics timing
with controlled scene evidence. Controller extras and personalization follow.
All foreign hook addresses remain leads until checked against our pinned XEX.
