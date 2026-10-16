# Campaign transition without application restart

## Old-title capture and teardown audit, 2026-10-16

Added opt-in `Run-Native.ps1 -TransitionDump` / `cod3_transition_dump`.
After the native exit probe reports pending old-title threads, before
`RelaunchCod3TitleIfRequested`, it invokes a separate `cod3_dump_threads.exe`
diagnostic helper on the current PID. The helper writes a private minidump
with thread contexts/stacks, thread information and module metadata (not full
guest memory). It uses CREATE_NEW, never overwrites evidence, and runs DbgHelp
outside the target to avoid self-dump loader/heap deadlocks. The parent waits
up to 15 seconds and logs PID, output path, helper result and wait result.
Failure never enables Runtime destruction. The helper executes no game code
and is not an application backend. Existing process handoff remains active.

Dumps are under `reports/transition-threads` relative to the game's working
directory (normally the ignored `cod3pc/out/build/win-amd64-relwithdebinfo`).
The native IDs in COD3_TITLE_THREAD_PENDING must be matched to those in this
same dump, not an Island successor. A dump samples those threads at capture
time; it does not prove they remain blocked at the same location forever.

Strict codegen/native build and 14 tests passed; logs are
`transition-dump-{build,tests}-console.log`. A real out-of-process helper test
verified MDMP signature, ThreadListStream and ThreadInfoListStream, refusal
to overwrite an existing dump and refusal of an invalid PID. Opened one
normal-menu instance with a fresh isolated `saint_lo_thread_dump_02` copy of
`saint_lo_final`, DoF 35%, dump enabled, cooperative-idle experiment disabled.
Runtime transition evidence is pending; no seamless transition is claimed.

Further pinned-source findings for the five required changes:

- `XThread` registers itself in its constructor, before `Create` starts the
  native thread. A creation seal must cover this create/start race, including
  suspended threads; a one-time object-table snapshot cannot establish it.
  `WaitForThreadsToExit` currently polls `is_running`, not native handles.
- D3D12 `ShutdownContext` already calls `AwaitAllQueueOperationsCompletion`.
  `CommandProcessor::Shutdown` stops its loop and waits for its host worker.
  These existing paths must be preserved and their completion observed before
  memory reset; title guest exit alone says nothing about GPU completion.
- Audio shutdown has additional unsafe fallback paths for an in-process reset:
  `AudioSystem::Shutdown` can force-terminate a callback worker on timeout;
  `XmaDecoder::Shutdown` can abandon its worker after 2 seconds and then free
  guest context storage. A safe reset needs positive host exit for both, with
  callback submission sealed first. Merely calling Runtime::Shutdown is
  insufficient even if the three game workers have exited.
- `KernelState` destruction already sweeps user modules via UnloadUserModule
  with guest DllMain disabled, then clears dispatch-related objects and deferred
  host libraries. This must run only after all callers/coroutines are retired.
  Our script coroutine map is thread_local and holds PPC contexts/base pointers;
  its owning thread's cleanup must finish before guest memory destruction.
- `ReXApp::OnDestroy` contains the presentation detachment sequence: unregister
  achievement callback, destroy overlays, detach ImGui/immediate drawer,
  clear runtime window/drawer pointers and window presenter. Extract that
  sequence for restart while retaining the window and joining module_thread
  on another thread. Do not call the existing window-close hard-exit path.

Next action: inspect the pending original workers in the new transition dump,
then implement a persistent, creation-sealed cooperative exit barrier. The
other teardown gates must return explicit success before same-process reset
can be enabled; timeout must retain the old memory rather than free it.

External-source comparison 2026-10-15: current CoD3Recomp and Project 1944 both
use CreateProcessW for title reboot, not a same-process Runtime reset. Project
1944 retains the old frame/window until a successor game-loop readiness event
(800-ms delay), which can hide the desktop gap but still overlaps processes.
This does not meet our explicit same-process criterion. See
[COD3_TRANSITION_COMPARISON.md](COD3_TRANSITION_COMPARISON.md) for pinned source
links, behavior and limitations; no code was imported or runtime changed.

## Runtime blocker and worker-exit experiment, 2026-10-15

Experiment result: native-20261015-173618-681.log at 17:40:17.252 host time
records 13 captured threads and WORKER_EXIT_ARMED. At 17:40:18.454 the probe
still reports three pending threads: guest 8/native 37448, guest 9/native 32344,
guest 10/native 32304. All have verified creation start 8212A780. No
WORKER_EXIT_IDLE event was logged. Therefore the new idle boundaries were not
reached on this shutdown path and did not solve the worker lifetime blocker.
The successor restored 664 bytes; user observed another process/window restart,
as expected from the still-active process handoff. Do not call this seamless
or promote CooperativeWorkerExit to default.

Next investigation must capture the *pending old-title threads* while they
remain alive (before the parent process exits), not infer their location from
an unrelated successor sample. Inspect SDK waits, termination and lock/lifetime
ordering at that point. Repeating this unchanged experiment is not useful.

native-20261015-172035-847.log records 13 captured guest threads, no capture
errors, and three still alive after 1000 ms: guest 7/native 39132,
guest 8/native 20844, guest 9/native 23476. The process handoff still worked;
the successor read all 664 bytes. This directly disproves assuming a completed
main guest thread means all guest workers have exited.

LLDB live attach failed to connect to its local debug server (::1:49729).
A separate private MiniDumpWithThreadInfo of the successor was analyzed offline
without debugger attachment. logs/island-worker-dump-stacks.log contains job
workers in 8212A780 -> 8212A520, some waiting, others dispatching jobs. This
successor sample alone does not map the three old native IDs to those stacks.

The optional authored `-CooperativeWorkerExit` experiment is disabled by default.
Before a verified same-title LaunchTitle request with 664 bytes, a host atomic
exit request is set. Workers whose original creation start is exactly 8212A780
may exit themselves via the pinned XThread::Exit at two reviewed boundaries:
8212A5CC after the queue mutant release, and 8212A600 after idle wait return.
Main-thread calls to the same scheduler and other worker entrypoints are excluded.
Normal game execution preserves all original instructions/register state.
The request lasts through shutdown rather than the SDK's 200-ms flag window;
it resets during runtime setup. This is not forced foreign-thread termination.

Independent Ghidra evidence: ghidra-0x8212A5C8.log, ghidra-0x8212A638.log,
ghidra-0x82129758.log and ghidra-0x82344FC0.log. 82129768 branches to
82344FC0, whose 82344FD0 calls import 82578B7C (NtReleaseMutant). All addresses
remain bound to the reviewed default.xex hash. Generated game code is unedited.

Strict codegen/build and the 14 existing tests passed:
logs/worker-idle-exit-{build,tests}-console.log. Runtime acceptance remains
pending in isolated saint_lo_worker_exit_01. This experiment does not yet seal
new thread creation, prove GPU/audio teardown, fix leaked lifetime references,
or implement in-process window/runtime restart. Those remain required.

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
