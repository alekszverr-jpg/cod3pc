# Loading briefing skip

`cod3_skip_loading_movies` defaults to true. Press **Space** or **Enter**
while the loading briefing is playing, including during resource loading.
The Russian hint appears only while this PC shortcut is available. Stopping
the video restores the original plain loading screen; asset loading continues.
The game's original controller input remains available. Disable with the executable CVar
`--cod3_skip_loading_movies=false`.

This finishes the briefing through the original input and cleanup path. It
does not mark an unfinished load as complete. The engine still performs its
remaining module/game initialization after closing the briefing.

## Reviewed original control flow

Current default.xex SHA-256:
`E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682`.
The other two reviewed original/Russian executable variants retain this layout;
Build.ps1 enforces their explicit hashes.

- `82518080` calls `82463988`. Ghidra shows that function pumping the resource
  manager until `manager+4 != 0`, then clearing `manager+5` before returning.
- `82518084` calls the remaining-movie loop `824EEBA0`, returning at `82518088`.
  Loading callbacks earlier in the resource wait use `824EECF8`, not this loop.
- Only this caller, a completed manager and a live original movie object open
  the PC skip window. The extra minimum-watch timer at `82A2A258` becomes zero;
  this timer is distinct from resource-manager completion. Original cleanup
  also resets this timer to zero.
- `824EECA4` calls original skip handling `824D0470`. Its input predicate call
  at `824D04D4` returns at `824D04D8`. The hook preserves that predicate's normal
  result and adds a one-shot PC request only in the guarded loading scope.
- The original handler clears input and records skip; the original movie loop
  then calls `824D0528` at `824EECD4` to close video/audio and release resources.
  No decoder end-of-file result, successful I/O, module or loading flag is faked.

Evidence is private in `logs/ghidra-movie-skip-instructions.log`,
`ghidra-loading-asset-wait-console.log`, `ghidra-loading-complete-skip-console.log`
and `ghidra-movie-skip-path-console.log`. Decompiler output truncates at some VMX
instructions; relevant branch/call instructions were separately inspected.
Generated functions were not edited. Host code is in `src/runtime/movie_skip.cpp`.

The window listener accepts fresh key-down events without Ctrl/Alt/Super;
requests cannot carry into a later movie. Title setup/exit resets host state.
Other key bindings continue through the pinned SDK's public ProcessKeyEvent API.

## Validation, 2026-10-17

Strict codegen (`--ignore-stamp`), native build and all 15 tests passed.
Initial object compilation exposed a private ReXApp::OnKeyDown method; the
override now calls the same public ProcessKeyEvent API as that original method.
Build log: `logs/movie-skip-island-full-build-console.log`.
Runtime DLL source/app hashes match C319700062CF1B7BF791EC739F5FDDD50479CEB7A6B535B31FD96DE7E5C7923C.

One normal-menu instance, PID 33136, opened with isolated `island_movie_skip_01`
restored from the genuine Island autosave snapshot. Log:
`logs/native-20261017-150521-277.log`. Runtime acceptance is pending: early key,
later key, original cleanup, level control/audio, then replay of the building
entry after the separate Island function-boundary fix. Expected markers are
COD3_MOVIE_SKIP_READY, COD3_MOVIE_SKIP_ACCEPTED and COD3_MOVIE_SKIP_WINDOW_CLOSED.
No claim of successful manual skip or building replay is made from build alone.

Observed runtime path: at 15:06:35.253 COD3_MOVIE_SKIP_READY, at 15:06:35.452
COD3_MOVIE_SKIP_ACCEPTED, at 15:06:35.622 COD3_MOVIE_SKIP_WINDOW_CLOSED;
Island entry at 15:06:36.192 returned 1. This confirms a PC request reached
the original skip/cleanup path and subsequent DLL startup. User confirmation
of gameplay/audio and the building replay is still pending. Early-key rejection
and another briefing/controller test are not established by this one run.

User acceptance: briefing skipped and the entire Island mission completed.
The successor also logged another accepted skip before requesting nightd.dll.
The next crash was missing third-mission native code, tracked in NIGHTDROP.md.

## Early briefing skip and hint, 2026-10-21

Adapted the loading-movie stop/callback approach from
[SirCarls/cod3_REcomp_](https://github.com/SirCarls/cod3_REcomp_/blob/d54c1bb877ab364dae2d2f40aa8c6a8ca46e3070/RELEASE/COD_3_WAW_Setup-tool/recomp/src/game/cutscene_skip.cpp),
`CutsceneSkip::OnLoadingMovie`, MIT copyright 2026 COD_3_WAW contributors.
Notice: `licenses/SirCarls-COD_3_WAW-MIT.txt`. Adaptation retains our existing
post-load input hook, isolates the stop call with SDK CallFrame and ABI stack
space, checks actual movie destruction, and resets host state at Runtime exit.
No generated game implementation, intro timescale changes or assets imported.

Ghidra on the E057 hash above independently confirmed callback 824EECF8
(`7D8802A6`, body through 824EED1F), stop 824D0528 (`7D8802A6`, through
824D06DB), and loader 824636D0 assigning callback at manager+0x350.
Stop destroys the movie/sound and clears 82A2A24C. Only after that succeeds,
the hook replaces callback 824EECF8 with original plain-screen 82526240.
It never changes manager load-complete flags. Private evidence:
`logs/ghidra-early-movie-skip.log`, `logs/ghidra-early-movie-instructions.log`.

The noninteractive hint reads host atomic availability only, without guest
memory or Runtime pointers. The application owns its lifetime, so it survives
Runtime recreation without a stale callback. Windows Tahoma supplies Cyrillic;
if unavailable, a Latin hint uses the existing SDK font. No font asset copied.

Validation and manual acceptance for this new behavior are recorded below;
earlier acceptance above covers the old post-load shortcut only.

`Build.ps1 -Stage Tests`: 10 Python and 6 native tests passed.
`Build.ps1 -Stage Build`: strict `--ignore-stamp` validation and native/GPU
build passed (`logs/early-skip-tests.log`, `logs/early-skip-build.log`).
One normal-menu instance PID 55680, isolated existing nightd_glow_query_01
profile. User confirmed early skip, playable level/audio and menu/Continue
in the same window. The Russian hint displayed, but U+2014 rendered as a
replacement glyph: the Cyrillic atlas does not include General Punctuation.
Replaced the separator with ASCII hyphen; no additional font dependency.
