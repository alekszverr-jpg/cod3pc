# Campaign transition without application restart

2026-10-15 implementation step: authored thread_exit_probe duplicates native
Windows thread handles from retained SDK XObject::Thread objects immediately
before the original LaunchTitle import. On main guest-thread exit it waits up
to 1000 ms for the *native* handles, then logs pending guest/native IDs and
wait errors. It does not use running_ or the guest signal as proof of host
exit. The duplicated handles survive SDK handle release and registry removal.
It neither terminates threads nor suppresses normal exit/relaunch.

This is an observation step, not the complete teardown barrier: threads created
after the snapshot and audio/GPU host workers are not covered. The next SDK
change must seal guest thread creation and retain registered workers through
actual exit before any memory reset. A zero pending count in this probe alone
must not enable Runtime destruction.

Strict codegen/build and 14 tests passed (10 Python, four native), including a
real Windows worker held on an event, timeout detection, closing its original
handle, signaling it and observing actual exit through the duplicate. Invalid
handle duplication is rejected. Private logs:
logs/thread-exit-probe-{build,tests}-console.log. Runtime observation pending.

User requirement, 2026-10-15: preserve one native game window and one cod3pc
process across mission transitions. The verified process handoff is temporary;
it does not meet this requirement. Resource loading and the original loading
screen remain necessary; the application window must stay open.

## Constraints verified in pinned local source

- src/system/kernel_state.cpp, KernelState::TerminateTitle: sets the cooperative
  termination flag, wakes waiters, waits 200 ms. Threads that do not exit remain
  running; guest threads are removed from the registry and the flag is cleared.
- src/ui/rex_app.cpp, ReXApp::OnClosing: calls TerminateTitle then std::_Exit(0).
  Its comment explicitly identifies host locks held by remaining guest threads
  as a teardown deadlock risk.
- src/system/runtime.cpp, Runtime::Shutdown: destroys GPU, audio, input, kernel,
  function dispatch, VFS and guest memory. Live consumers must not survive this.
- ReXApp::LaunchModule: after the main guest thread ends, calls
  OnGuestThreadExit, then posts QuitFromUIThread. Preserving the window needs an
  explicit lifecycle branch instead of merely suppressing the guest's exit.
- ConstructRuntime moves RuntimeConfig into Setup; repeat construction must
  recreate graphics/backend configuration and connect the retained window.
- Overlay/achievement callbacks and input listeners refer to runtime-owned
  systems. OnDestroy normally disposes them before runtime destruction; a
  transition needs equivalent detachment while keeping the window alive.
- script_coroutines.cpp owns per-thread host fibers with guest contexts and
  base pointers. They must be destroyed on their owning thread or safely retired
  before the old guest address space can be released/reused.

These are compatibility-layer issues, not evidence that the original game can
skip its title reset. Its 664-byte launch-data contract and original startup
must remain intact. Re-entering the old entrypoint in dirty memory, ignoring
remaining threads, force-killing threads while they may hold locks, or returning
success from a missing reset is not an implementation.

## Implementation order

1. Retain and track every terminating guest thread through actual host exit;
   report identities and outstanding waits instead of losing stragglers from
   the registry. Establish bounded cooperative completion at valid safe points.
2. Add an explicit runtime teardown/recreate path after that barrier. Audit
   audio/GPU callbacks, queued UI work, DLL registry, dispatch and host fibers.
3. Preserve the native window and its display mode; detach old presenter and
   overlays, rebuild the runtime, attach new presentation/input without closing
   the application. Original launch bytes transfer directly in host memory.
4. Branch the app's main-thread-exit handling to this path only for a verified
   same-title request. Ordinary quit and fatal failures retain their semantics.
5. Test from a fresh isolated copy of saint_lo_final. Keep the current verified
   process implementation available for comparison until acceptance passes.

## Acceptance

Record the same PID and native window ID before/after Saint-Lo→Island; log old
worker exit before memory destruction and original GetLaunchData success in
the rebuilt runtime. User must confirm Island loads and control works. Repeat
twice from the snapshot, checking remaining thread/resource counts and no stale
callbacks. Verify DoF 35, input/focus/display mode, checkpoint writing and later
Continue. No third SP module is required for this focused test.

This document records research and the next implementation requirement; no
in-process reset is implemented or claimed yet. No runtime/code changes were
made for this documentation update, so existing runtime tests were not rerun.
