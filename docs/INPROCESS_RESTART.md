# Same-process title restart experiment

## Clean teardown prerequisite accepted, 2026-10-18

User confirmed Island from saint_lo_xex_ownership_01. In parent log
native-20261018-214705-386.log, at 21:51:32 the four consumer barriers complete,
exit refs report 19/0 and script contexts/fibers report 0/0. Normal unload
unregisters sp/saint_lo/saint_lo.dll (6073 functions); globals clear and the
campaign DLL count drops 1 to 0 while the window remains alive. Neither the
previous library-sweep error nor the invalid heap-release error is present.
This accepts the corrected old-Runtime teardown on this transition.

## Opt-in implementation

Run-Native.ps1 -TitleInProcessRestart requires -TitleRuntimeTeardown and all
its consumer-barrier prerequisites. The CVar defaults false. Existing native
process handoff remains available when this experiment is not selected.

After all barriers pass, CaptureCod3InProcessHandoff copies the original
validated 664-byte Xam loader state into an application-owned byte vector while
the old kernel still exists. No child process or inherited handoff pipe is
created in this mode. Failure does not silently fall back to another process.

The watcher releases its main XThread reference, destroys the old Runtime on UI
as previously verified, and checks globals/DLL handles. Only positive teardown
requests the new SDK restart path. The watcher schedules a deferred UI callback
and returns, avoiding a join while blocked on UI. That callback joins the old
watcher before a new watcher can replace its std::thread.

The SDK reuses actual resolved paths, creates fresh RuntimeConfig, reruns GPU
presentation setup, and reconstructs Runtime through the normal original-XEX
startup path. SetupPresentation creates/opens/registers the window only if it
does not already exist, preserving its HWND, size, fullscreen state and original
window listeners. Overlay bindings were removed by old teardown and are rebuilt
for the new presenter. ConstructRuntime attaches fresh input to the existing
window. Environment/logging/config-file initialization is not repeated.

OnPostSetup restores the captured original bytes using the same Xam loader
storage as the established process handoff; original game startup chooses the
next map. No host-written mission selection or replacement menu is used.
LaunchModule then performs its ordinary shader/input/guest startup.

The expanded rexglue-v0100-title-runtime-teardown.patch contains the matched
ReXApp header and consumer-compiled source. It adds no GPU plugin ABI changes.
Normal close behavior and non-opt-in process handoff remain available.

## Required acceptance

The new COD3_RUNTIME_GENERATION records generation, PID and HWND. A successful
test requires generation 2 in the same PID/HWND, original entry 82344D00 again,
Island entry/loader success, no child-process handoff log, and user-confirmed
level rendering/control. TITLE_RUNTIME_RESTART ready alone proves setup only.
Repeat transitions, return-to-menu, fullscreen/settings, sound and input still
need testing before making this mode default.

## Validation state

All 16 parser/native regression tests pass in logs/title-inprocess-tests.log;
strict codegen passes in logs/title-inprocess-strict-codegen.log. Four changed
application objects compile in logs/title-inprocess-app-objects.log, including
the installed SDK's consumer source. CLI rejects missing prerequisites and
accepts the full dry-run command (title-inprocess-cli-invalid/dryrun.log).

After explicit closing approval, requested normal window close and waited for
the old game to exit. Full native link/install passed in
logs/title-inprocess-full-build.log. Restored fresh saint_lo_inprocess_01 from
saint_lo_final and opened one normal-menu PID 38720 with all barriers and
TitleInProcessRestart enabled, log native-20261018-220651-003.log. Generation 1
and original entry are logged. Real same-process game acceptance remains pending.
No claim of seamless gameplay is made from build success or prior process handoff.
