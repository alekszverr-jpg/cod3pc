# SDL input retirement across Runtime restart

## First same-process failure, 2026-10-18

User reported a black screen/hang after Saint-Lo. In
native-20261018-220651-003.log, Runtime generation 2 is created at 22:11:17.765
in the same PID 38720 and HWND 0x130b6a. Original launch bytes are restored and
entry 82344D00 runs again at 22:11:17.890. Island has not been accepted.

Read-only diagnostic helper saved reports/title-inprocess/black-screen-01.dmp;
local LLDB stacks are in logs/title-inprocess-black-screen-stacks.log. UI thread
is inside SDL_DispatchEventWatchList -> SDLInputDriver::HandleEvent, blocked on
the driver's event_queue_mutex_. Guest startup is in XamInputGetState -> mouse
capture update -> SDL_PushEvent, waiting for SDL's event-watch mutex held by UI.
The process was preserved until evidence and the correction were ready.

Pinned SDLInputDriver's destructor is empty. Cleanup exists only in OnClosing,
which is not delivered when Runtime is destroyed while the window survives.
The anonymous SDL event watch is never explicitly removed even by that old
OnClosing path. The watch therefore retains old driver userdata after free,
including a stale mutex/vector. This is a host input-lifetime defect, not proof
of a game loading failure or a reason to alter guest startup.

## Correction

rexglue-v0100-input-retirement.patch makes the callback a named static WatchEvent
and uses that exact function/userdata pair for removal. Destructor and OnClosing
share idempotent DetachFromWindow, marshalled to the owner UI thread:

1. Remove the old window listener and SDL watch without holding driver mutexes.
2. Drain an already queued pump while driver userdata is alive.
3. Close controller handles, release this driver's SDL subsystem references,
   clear pending events and detach the window pointer.

The pinned SDL source holds its event-watch-list lock around dispatch and
removal, so removal waits for an in-flight callback before freeing userdata.
Controller-event filtering is unchanged. SDL_INPUT_RETIRED records completion.
The patch adds private methods only; class layout and GPU plugin ABI are intact.
Matched public header/runtime are staged by Build-TitleThreadDrain.

## Regression evidence

The ownership probe's --input-lifecycle mode uses the actual SDK driver and a
real, unopened SDK window/context. It keeps an independent SDL subsystem
reference, explicitly destroys the driver, protects its storage PAGE_NOACCESS,
and dispatches a controller event observed by a separate live SDL watch.
Any stale callback deterministically faults on inaccessible driver storage.

An initial probe incorrectly injected into the consumer's separate static SDL
copy and falsely passed (input-retirement-before.log); it is not evidence of
correct lifetime. The corrected target compiles the actual pinned SDK driver
source into the probe so driver/watch/injection use one SDL instance. Before
the fix it exits 0xC0000005 (input-retirement-before-bound.log). With the fix,
native_runtime_ownership reports INPUT_RETIREMENT_PROBE stale_callback=false,
event_dispatched=true, and the existing Runtime/thread/fiber checks still pass.
No game code or assets are substituted by the diagnostic.

All 16 parser/native tests pass in logs/input-retirement-tests.log; strict
codegen and full native installation pass in input-retirement-strict-codegen.log
and input-retirement-full-build.log. Installed/app runtime SHA-256 matches
98B1D25E86D0A43C4B3BAD3C476B3799001B62A41AB3F45B4D78F49E12DED5A0;
corrected GPU DLL SHA-256 is
6B8C6084C4ECAA73DD0E60894AB6D44AD74ECD0C2A083FEDD4549EFEE2087C58.

After saving diagnostics and passing the regression, stopped only the matching
hung isolated PID 38720, verifying its name/profile before termination. It could
not service window events. Main campaign userdata was not touched. Restored
fresh saint_lo_inprocess_input_01 and opened one normal-menu PID 51848 with
same-process flags; log native-20261018-222816-941.log. Actual Island rendering,
control and sound in that process remain pending.
