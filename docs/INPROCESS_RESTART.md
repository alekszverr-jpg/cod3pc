# Same-process title restart experiment

## Standard launcher preset, 2026-10-19

After the recorded manual acceptance, Run-Native.ps1 now selects the accepted
same-process path by default, enabling all seven worker/drain/restart controls.
The normal corrected RelWithDebInfo backend is required. -ProcessRelaunch
explicitly selects the older diagnostic path; individual barrier switches still
allow staged teardown tests in that mode. Inactive controls are passed as false
to override any saved SDK settings. Conflicting modes and default same-process
with a stock/experimental backend are rejected with actionable errors.

This is a launcher preset change; individual SDK CVar defaults for direct exe
launches are unchanged. No game was stopped or started for this script change.
CLI dry-run checks verify normal barriers, diagnostic false overrides, stock
diagnostics, staged barriers and rejection of incompatible combinations.
Build.ps1 -Stage Tests (16 cases), -Stage Codegen (--ignore-stamp), and native
CMake build pass. Evidence: logs/transition-default-{cli,tests,codegen,build}.log
and reports/transition-*-dryrun.json. The preset matches the already accepted
manual-run flags; a fresh default-launch gameplay check remains for the next
normal launch rather than interrupting the current session.

## Window/settings acceptance complete, 2026-10-19

The user separately confirms that windowed mode survives menu → Continue,
following the earlier settings-retention confirmation. This closes the planned
display-mode/settings acceptance gate. Together with Saint-Lo → Island,
Island → Night Drop and repeated menu/Continue checks, the tested same-process
path meets the listed manual acceptance criteria. Default promotion is now a
separate implementation step; this record does not change launch flags or claim
full campaign completion. Earlier pending notes below are chronological.

## Night Drop menu/Continue accepted, 2026-10-19

Additional user feedback confirms settings survive menu → Continue, followed
by explicit confirmation of windowed-mode retention (see acceptance above).

User confirms successful exit to menu and reload. In
native-20261019-143620-467.log, generation 2 starts at 15:20:34.101 with the
same PID 8180/HWND 0x190c6a as generation 1. Script contexts/fibers are zero,
old Runtime globals clear and campaign DLL count falls 1 to 0. Night Drop's
original entry returns 1 again at 15:21:49.477; no FATAL in the inspected log
set. This accepts another mission's menu/Continue cycle. Separate follow-up
feedback then accepted settings and windowed-mode retention.

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

Accepted 2026-10-19: Island → Night Drop from the isolated island_final snapshot.
`native-20261019-100318-033.log` records generations 1/2 in PID 58744 and HWND
0x1a06dc. At 10:08:56–57 all thread/audio/GPU/dispatch/script drains complete,
pending threads/references and script contexts/fibers are zero; Runtime globals
clear and campaign DLL count falls 1 → 0. At 10:09:26 the original Night Drop
entry returns 1 and load status is 0. The user confirms normal operation.
The same process remains alive. Fullscreen/settings acceptance remains before
making the mode default; the separate known Night Drop gameplay crash at
0x89174700 is not fixed by this transition acceptance.

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

First test reached generation 2 in the same PID/HWND and original startup, but
the user reported black screen/hang. The dump identifies a retained SDL input
watch after old driver destruction. Correction, regression and the next manual
test are recorded in INPUT_RETIREMENT.md. The corrected test is accepted:
native-20261018-222816-941.log shows generation 1 and 2 both in PID 51848,
HWND 0x1609da, clean old Runtime/DLL teardown and original 664-byte restore.
Island entry returns 1 and loader returns 0 at 22:32:11; user confirms level,
control and sound. No child-process handoff is queued. This verifies one real
Saint-Lo → Island transition; repeat/menu/fullscreen regression is still needed
before changing the default mode. The current game remains open for that check.

The user then confirmed return to menu and Continue in the same window. Current
log at 22:35:11.376 records generation 3 with the same PID 51848/HWND 0x1609da;
the second old-Runtime teardown passes and Island entry/loader succeed again at
22:35:41.347. This accepts one repeated menu/Continue cycle. The first transition
records have rotated into native-20261018-222816-941.1.log; selected evidence is
also preserved privately in reports/title-inprocess/accepted-transition-evidence.txt.
Next acceptance is Island → Night Drop in this process, plus fullscreen/settings.
