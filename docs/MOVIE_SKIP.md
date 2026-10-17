# Loading briefing skip

`cod3_skip_loading_movies` defaults to true. Press **Space** or **Enter**
after the original loading-resource wait has completed. A press before that
point is not queued by the PC shortcut; release and press again. The game's
original controller input remains available. Disable with the executable CVar
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
