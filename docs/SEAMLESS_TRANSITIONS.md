# Campaign transition without application restart

## Callback ownership accepted; script TLS audit, 2026-10-18

User confirmed Island from saint_lo_start_data_01. Parent log
native-20261018-094956-224.log records all four drains complete and
reaped=19 pending=0 at 09:56:33. Successor Island entry returns 1 and loader
returns 0 at 09:57:06. Later return-to-title also drains (18/0), then the
menu closes normally. This accepts the ThreadStartData correction.

Next diagnostic counts actual live Script and Cod3FiberTask owners and logs
TLS map retirement without touching guest memory. A real suspended-task TLS
probe passes (1 live before raw native Exit, 0 after actual native exit).
All 16 tests, strict codegen and full native build/install pass. One normal-menu
PID 4968 is open with isolated saint_lo_script_tls_01; original entry is logged
in native-20261018-153758-546.log. Game acceptance of COD3_TITLE_SCRIPT_DRAIN
still remains; same-process reset is not enabled.

## Exit references accepted; callback capture leak reproduced, 2026-10-18

The user confirmed Island loaded from saint_lo_exit_refs_01. Parent log
native-20261018-093413-667.log records all four drains complete at 09:38:05,
then COD3_TITLE_EXIT_REFS reaped=19 pending=0 before the original 664-byte
interprocess handoff. This accepts the specific XThread ownership correction.

The next SDK canary reproduced a ThreadStartData capture surviving raw native
exit. A narrow Windows TLS ownership patch passes normal return, explicit Exit,
and Exit from a secondary fiber, plus three Runtime ownership cycles. See
THREAD_EXIT_OWNERSHIP.md. All 16 tests, strict codegen and native build passed;
installed runtime hashes match. One isolated saint_lo_start_data_01 is open
(PID 39792) for game acceptance. Script TLS/fibers and other abandoned references still need audit;
same-process/same-window reset is not enabled.

## Exit-reference correction installed, 2026-10-18

SDK self-reference retirement and owner-exited thread-fiber deletion pass the
three-cycle/blocked-owner native regression; all 16 tests pass. Full strict
application build completed, installed runtime hash matches
6BA318A36C620C52C07E08C02C7C8A8D4E72EE49CD875F90E954C07F05A8A0EE.
Opened one normal-menu PID 29220 with isolated saint_lo_exit_refs_01 and original
final checkpoint; log native-20261018-093413-667.log. Runtime transition result
is pending. See THREAD_EXIT_OWNERSHIP.md for evidence and remaining limitations.

## Night Drop accepted; thread ownership probe, 2026-10-18

User confirmed third-mission loading/control. The active lifecycle blocker is
now host object ownership. A separate real SDK probe confirms native exit but
no XHostThread destruction after table/caller release. The retained Exit self
reference is transferred to an explicit native-exit retirement registry; thread
fiber wrappers have a separate owner-exited retirement path. See
THREAD_EXIT_OWNERSHIP.md. Isolated tests and application validation are separate;
this does not enable same-window Runtime reset or claim all leaked references fixed.

## Island completed; next missing campaign module, 2026-10-17

User confirmed skip and full Island completion. At 15:22:43 in
native-20261017-150521-277.log, the current C3197000 runtime passes all guest,
audio/XMA, GPU/fence and dispatch drains before the 664-byte handoff.
The successor fails at the third mission because nightd.dll was not statically
recompiled. See NIGHTDROP.md for original-entry and exact failure evidence.
Night Drop is now included in strict/native generation/build; manual entry and
gameplay remain pending. This module integration does not implement same-process
Runtime recreation; remaining ownership/window work is still active.

## Movie skip / Island fix installed, 2026-10-17

After user authorized close, full strict/native Build completed and installed
both the guarded briefing shortcut and the genuine Island 891999E0 boundary.
Build.ps1 now verifies the runtime independently of POST_BUILD; installed app
and SDK runtime hashes both C319700062CF1B7BF791EC739F5FDDD50479CEB7A6B535B31FD96DE7E5C7923C.
All 15 tests pass. See MOVIE_SKIP.md for original control-flow evidence.
Opened one normal-menu instance PID 33136 with isolated island_movie_skip_01,
restored from island_building_crash_01. Log native-20261017-150521-277.log.
The log subsequently records skip ready 15:06:35.253, accepted 35.452,
original movie loop closed 35.622 and Island DllMain result=1 at 36.192.
User gameplay/audio and building-entry replay are pending. No same-process reset yet.

## Dispatch accepted; Island boundary and movie skip follow-up, 2026-10-17

User confirmed Island loaded in `saint_lo_dispatch_drain_01`, parent PID 32156.
`logs/native-20261017-104025-211.log` at 10:44:04 records guest tracked=40 /
pending=0, audio/XMA exit, GPU native exit/queue idle, then
TITLE_DISPATCH_DRAIN native_exited=true retired=0 complete=true and
COD3_TITLE_DISPATCH_DRAIN complete=true. Handoff still transfers 664 bytes to
a new process; same-window/runtime recreation is not enabled.

The running test used app runtime B0829A0A0334F0BC58B0E26C968636D0DAFCD993B11B1ABA3ED3F78DF05DE2C6.
A later startup-compatibility refinement permits queueing before the dispatch
worker first starts, while rejecting sealed/stopped titles. Prefix runtime is
C319700062CF1B7BF791EC739F5FDDD50479CEB7A6B535B31FD96DE7E5C7923C.
An incremental app build did not run its POST_BUILD dependency copy, so this
later runtime was not installed into the running test. Build.ps1 now explicitly
copies/verifies the runtime even when Ninja has no app link work; this staging
fix still needs installation after the game closes. Do not conflate these hashes.

Island crash target reviewed in current original island.dll SHA-256
679A5B58552ABCE1F8EDD80499A56F1CA4480E285734DC2719528400D8CD4A35:
`ghidra-island-building-target.log` shows bctr at 891999DC, original blr
4E800020 at 891999E0, zero padding at E4, another function at E8.
Added `configs/island_functions.toml` size=4 boundary; no generated function
was edited. Strict --ignore-stamp codegen passed and produced the original
return in the function registry. Three changed Island objects compiled.
Full install and replay at the building remain pending. Preserved the existing
Island autosave in private verified snapshot `island_building_crash_01`.
All 15 tests pass (`title-dispatch-island-boundary-tests-retry.log`); the first
attempt overlapped Ninja's object build and failed to open its deps log, so
it was repeated sequentially after that build completed.

User requested manual movie skip once the level is loaded, to avoid repeating
briefings during transition tests. Research of the original WMV player and
loading-screen input is in progress, using Ghidra on current E057 XEX.
No movie skip or loading-ready bypass is installed yet.

## GPU accepted; Kernel Dispatch barrier prepared, 2026-10-17

User confirmed Island loaded. Parent `logs/native-20261017-102350-600.log`
records at 10:27:40: guest creation sealed / tracked=40 / pending=0,
audio/XMA native exit and callbacks idle, one SDL driver closed, then
COD3_TITLE_PRESENTATION_DETACHED hwnd=0x180438. GPU command native exit,
queue_idle=true, vsync exit and callbacks_idle=true precede
COD3_TITLE_GPU_DRAIN complete=true and the 664-byte process handoff.
This accepts the GPU barrier for this run; it is still a process transition.

Added opt-in TitleDispatchDrain after positive guest/audio/GPU barriers.
The SDK seals deferred completion admission under the same global lock used
by enqueue/dequeue, stops and wakes Kernel Dispatch, retires queued captures
outside that lock while modules and guest memory are alive, and observes the
actual native thread handle. A callback already dequeued may finish with old
Runtime retained. Timeout reports failure without destruction/force-kill.
Rejected/cancelled retired-title I/O is explicitly logged, never completed with
invented success. Predicate waiting also prevents empty-queue access on a
spurious wake. A sealed title cannot recreate this worker via SetExecutableModule.
No KernelState fields or graphics ABI were added. Patch:
`tools/patches/rexglue-v0100-title-dispatch-drain.patch`.

SDK compilation and all 15 existing tests passed. Full strict codegen/native
build is in progress; runtime dispatch acceptance remains pending. Remaining
work includes raw ExitThread ownership, script fibers, module cleanup and
same-window Runtime recreation. Neither native thread exit alone nor this
queue barrier proves host object/context destruction.

Separate Island gameplay crash reported by user on entering a building:
successor `logs/native-20261017-102350-600-relaunch.log` ends at 10:31:33.708
with FATAL: invalid/unregistered guest function 0x891999E0 (native thread 59308).
The failing successor is a new process, so this does not demonstrate an old
Runtime memory lifetime error. Caller/register evidence is still required;
no address was guessed or crash suppressed. Preserve this as a separate
Island progression blocker, without treating initial Island loading as full
mission acceptance.

## GPU test installed and launched, 2026-10-17

User authorized closing Island. The window closed normally, full strict
codegen/native build completed, and the built/staged corrected plugin hashes
match: `454ECD4A14831A53E122342E1BEB49381437D24182C0C71CE39EBAB3D86E025C`.
Log: `logs/title-gpu-drain-full-build-console.log`. This completes the previously
blocked link/install step; there is no longer a file-in-use staging failure.

Opened one normal-menu cod3pc, PID 58864, isolated checkpoint profile
`saint_lo_gpu_drain_01`, DoF 35%, all guest/audio/GPU drain flags and TransitionDump.
Log `native-20261017-102350-600.log` confirms original entry 82344D00.
Runtime GPU/presentation acceptance is pending. Expected proof includes
COD3_TITLE_PRESENTATION_DETACHED with the retained HWND,
TITLE_GPU_COMMAND_DRAIN exited=true/queue_idle=true, TITLE_GPU_DRAIN with both
workers exited and callbacks idle, and COD3_TITLE_GPU_DRAIN complete=true.
The test still uses process handoff; no seamless restart is claimed.

Independent follow-up audit: KernelState's separate Kernel Dispatch host thread
can execute deferred completion callbacks after guest/audio/GPU stops. Its loop
and queued std::function captures must be sealed, stopped and retired before
DLL/guest memory reset. Current destructor-only shutdown and a guest drain do
not establish that barrier. Windows Thread::Exit also calls raw ExitThread;
ownership of local references/retained thread contexts needs review before
repeated in-process teardown, rather than assuming native exit frees every
host object. Neither issue was bypassed in this GPU test.

## Audio accepted; GPU/presentation drain prepared, 2026-10-17

Audio test accepted by user: Island loaded without hang or audio problems.
`native-20261017-095729-879.log` at 10:01:16.981 records guest tracked=40 /
pending=0 and `audio_exited=true xma_exited=true callbacks_idle=true`.
At 10:01:17.022 one SDL driver was closed and COD3_TITLE_AUDIO_DRAIN complete=true;
handoff then transferred 664 bytes. This supports the new audio barrier in
this run, without establishing GPU/dispatch shutdown or same-process reset.

Prepared opt-in `-TitleGpuDrain` after positive guest+audio drain:

- Detach ImGui/immediate drawer and window presenter on the UI thread, log the
  retained native window handle. Do not close/reset the window or free Runtime.
- The already-loaded corrected plugin exposes optional versioned C capability
  `cod3_gpu_quiesce_title_v1`. IGraphicsSystem and GPU ABI version 1 stay intact;
  no second backend is loaded. Only corrected RelWithDebInfo runs support this
  test flag. Unsupported/missing capability reports failure, not success.
- Seal GPU interrupt callbacks and command-queue admission, request vsync and
  command workers stop, require actual native exit and zero active interrupts.
  Already admitted queued functions are completed before command context
  shutdown; blocking callers use TryCallInThread so rejected work does not leave
  an unsignaled local fence. Pending function push/pop/inspection use one mutex.
- D3D12 ShutdownContext records the actual queue-fence result before releasing
  command resources. On a title-shutdown fence failure, retain those resources.
  A shared five-second worker deadline returns failure for pending work/threads;
  a debugger-paused command processor is rejected. Do not force-kill or free
  guest memory on timeout. Provider/presenter remain alive for later UI teardown.
- No DLL cleanup, KernelState dispatch drain, Runtime destruction or in-process
  engine restart is enabled yet. The test still ends in normal process handoff.

Patch: `tools/patches/rexglue-v0100-title-gpu-drain.patch`, applied by
Build-GpuGrass.ps1 after the existing grass/DoF patches. It was generated against
a private snapshot of pre-change GPU sources, excluding unrelated existing SDK
edits. Plugin and changed application objects compiled; all 15 tests passed
(`title-gpu-drain-app-objects.log`, `title-gpu-drain-tests-console.log`).
An isolated DLL probe verified ABI=1 and null-system rejection by the optional
export. GPU runtime acceptance has not happened.

Staging the new GPU DLL initially failed with Windows file-in-use while the
previous Island window remained open. That window was not stopped without the
pending user's close approval. Build-GpuGrass now makes copy failure fatal and
checks source/destination hashes; there is no claim that a failed copy installed
the plugin. Finish normal Build after the window closes, then open a fresh
saint_lo_final profile with CooperativeWorkerExit, TitleThreadDrain,
TitleAudioDrain, TitleGpuDrain and TransitionDump. Main userdata is unchanged.

## SDK guest drain accepted, audio quiescence in progress, 2026-10-17

User confirmed Island in the SDK-barrier test. Parent
`native-20261017-094421-086.log` records creation sealed, then at 09:47:55.338
`captured=13 pending=0` and `sealed=true creation_complete=true tracked=40
pending=0`. The retained SDK registry includes old threads whose guest handles
were already released. Successor `-relaunch.log` restores 664 bytes and records
Island DllMain result 1 and LOAD_RESULT=0. The runtime observation supports
the guest creation/native-exit barrier in this run; audio/GPU host consumers
were outside it and same-process reset remains disabled.

Added opt-in `-TitleAudioDrain` (requires TitleThreadDrain), implemented by
`rexglue-v0100-title-audio-drain.patch` against the same pinned SDK:

- Audio callback admission uses the tested per-title gate. RegisterClient
  rejects a retiring title; further frame submissions/client removal do not
  touch retired drivers. Already admitted callbacks may finish normally.
- Request both Audio Worker and XMA Decoder to stop before waiting for either.
  Wake work/resume events and the XMA pause fence. Wait on actual native handles
  with a shared five-second worker deadline, then require zero admitted callbacks.
- Any timeout/error returns false without terminating a worker, resetting its
  object, releasing guest contexts, or destroying client drivers. The app logs
  failure and optionally captures old-process stacks. It retains Runtime/memory
  and uses the existing process handoff, not an unsafe same-process reset.
- Only after positive worker exit and callback completion, close SDL client
  drivers/devices. Pinned SDL source SDL_audiocvt.c:1514 and SDL_audio.h document
  destruction unbinding the stream/closing its simplified device. The SDL host
  callback consumes copied host frame queues, not the original guest buffers.
  Callback argument packets and XMA guest contexts stay allocated until later
  fully gated Runtime teardown. Shutdown tolerates already-closed drivers and
  still cleans up after worker_running has been cleared by quiescence.
- AudioSystem's new method is non-virtual; no IAudioSystem/vtable change or
  graphics implementation change was made. The app verifies the actual SDK
  AudioSystem via dynamic_cast and reports failure for an unsupported backend.
  It calls this only after a positive sealed guest drain.

The normal SDK shutdown fallback remains unchanged for other callers; this
new opt-in API avoids its force-kill/abandon path. GPU/vsync/UI/dispatch callbacks
are not yet drained, so success here still cannot authorize Runtime destruction.

Build and all 15 tests passed: `logs/title-audio-drain-{build,tests}-console.log`.
SDK SHA-256 `14FC22866F0986E345AEB0217F7A01C46B4B62F858A2D13F5E47D9C271B8C1E8`;
corrected GPU binary rebuilt/relinked with the matching SDK, SHA-256
`7BBD3BF45CB97154FE69B6ED435EB248E60C410846088DD1DAE44D4E3F3F52E3`.
One normal-menu run is open in fresh isolated `saint_lo_audio_drain_01` from
saint_lo_final, DoF 35%. Audio/XMA runtime acceptance is pending. Expected
evidence: TITLE_AUDIO_DRAIN, TITLE_AUDIO_DEVICES_CLOSED and
COD3_TITLE_AUDIO_DRAIN complete=true before relaunch. Main save is untouched.

## Successful idle exit and SDK creation barrier, 2026-10-17

User confirmed Island loaded after the bounded-wait experiment. Parent log
`native-20261016-203735-755.log` records WORKER_EXIT_ARMED at 20:41:19.735,
then `captured=13 pending=0 incomplete=false elapsed_ms=0` at 20:41:19.785,
before handing off 664 bytes. This verifies actual native exit for the captured
threads in that run, not a complete Runtime teardown or same-process transition.

Next implemented opt-in `-TitleThreadDrain` (requires CooperativeWorkerExit):

- A pinned SDK patch adds a permanent per-KernelState guest-creation gate.
  XThread::Create acquires a creation lease before guest/native allocation,
  keeps it through initial Resume and rejects new creation after sealing with
  X_STATUS_THREAD_IS_TERMINATING. Restore's separate native-creation path also
  takes a lease. Host audio/GPU threads are deliberately outside this gate.
- Before the original LaunchTitle import, after validating its campaign data,
  seal creation and retain registered guest XThread objects, including the main
  thread and in-flight creations. Registration during a pending creation is
  also retained. TerminateTitle can no longer lose these records by clearing
  its ordinary registry, and its termination flag stays set for the sealed title.
- After main native exit, WaitForTitleThreadExit first waits for admitted
  creators to finish, then checks actual native handles of retained threads.
  A suspended thread or failed handle wait counts as pending, not as exited.
  Incomplete creation is returned explicitly. It never relies on is_running.
- This step only logs `COD3_TITLE_THREAD_DRAIN sealed=... creation_complete=...
  tracked=... pending=...`. It does not free memory, rebuild Runtime or suppress
  failures. Audio/GPU/dispatch callbacks, ownership/fiber cleanup and window
  lifecycle remain separate unfinished requirements.

Patch: `tools/patches/rexglue-v0100-title-thread-drain.patch` against pinned
SDK c94f5ebdcb3c9d1a460ca48e04f9758448f8d518. Build-TitleThreadDrain.ps1
checks patch context, builds rexruntime and stages matching DLL/import library,
PDB and headers. Existing prefix files are backed up privately in
`reports/title-thread-drain/previous-sdk`; unchanged headers are not recopied.
The build entry invokes this before codegen/application build. Existing member
offsets are preserved by appending KernelState fields; runtime reset stays opt-in.

The completed strict/native build is recorded in
`logs/title-thread-drain-app-build-console.log`. All 15 tests passed in
`logs/title-thread-drain-tests-console.log` (10 Python, five native). The new
concurrent gate test holds eight admitted creations over Seal, checks timeout
while they remain active, rejects 8,000 post-seal attempts, releases the leases,
observes completion and verifies a fresh title gate independently admits work.
Matching staged/application SDK SHA-256:
`71789A5D6253F20FF6B748E544E943D0621B428E131C63F02A338685F5243346`.
Rebuilt corrected GPU plugin SHA-256:
`E79C800C7C7E5A7213F418B22546881EB6AFCA9E12BD096646876C43CF31A6C3`.
Its changed binary hash follows the SDK/header rebuild; shader fixes remain.

Opened one normal-menu test in isolated `saint_lo_thread_drain_01` with
checkpoint saint_lo_final and DoF 35%. Runtime acceptance of the SDK barrier
is pending. Main userdata is unchanged.

## Pending worker stacks and bounded idle-wait experiment, 2026-10-16

The new capture succeeded before process handoff: old PID 5956, log
`native-20261016-202315-881.log`, 13 captured/3 pending after 1001 ms.
Native IDs 19000/20280/61388 (guest 7/8/9) map exactly to dump threads
33/34/35 (0x4a38/0x4f38/0xefcc). All three stacks are:
`NtWaitForSingleObject -> WaitForSingleObjectEx -> rex::thread::Wait ->
XObject::Wait -> NtWaitForSingleObjectEx_entry -> sub_823494C0 ->
sub_8212A520 -> sub_8212A780`. This is old-title evidence, not a successor
sample. Private dump: build directory `reports/transition-threads/
old-title-5956-37336109.dmp`; LLDB output: `logs/old-title-5956-stacks.log`.
The capture completed successfully and normal handoff transferred 664 bytes.

LLDB disassembly of the actual installed runtime confirms that XObject::Wait
checks title termination only before and after its native wait. The matching
local SDK PDB is unavailable (newer source-build PDB rejected), so no private
SDK variables are inferred from mismatched symbols. Exported symbols and game
PDB frames establish the call chain. The blocking wait itself gives neither
the persistent project request nor the original post-wait idle hook a chance
to run while it remains unsignaled.

Ghidra on installed E057 XEX independently confirms:

- 8212A5C8 calls queue-mutant release 82129768; 8212A5CC calls 8212A488.
- 8212A5F4 loads r4 from r20 (-1); 8212A5F8 loads the idle handle into r3;
  8212A5FC calls 82343880, returning at 8212A600.
- 82343880 sets r5=0 (non-alertable), then tail-branches to 823494C0.
- 82349E58 maps timeout -1 to null, otherwise writes -10000*r4 ticks.
- 823494F4 calls NtWaitForSingleObjectEx (82578B0C), then preserves the
  real status/error path and original register/stack epilogue.

Evidence: `logs/ghidra-worker-idle-wait.log`. Added a disabled-by-default
extension of CooperativeWorkerExit: hook only this exact infinite worker
idle call (start 8212A780, LR 8212A600, r4=-1). Reinvoke the original wrapper
with 50-ms waits, restoring the entry PPC context between timeout retries.
Only real timeout 0x102 is retried; signals, errors and abandonment retain the
original return path. Between calls, after kernel locals have unwound, the
worker may honor the persistent validated relaunch request through its own
XThread::Exit. Other threads/callers/timeouts use the original wrapper once.
No generated game function was edited, no foreign thread is killed, and no
empty import returns invented success.

Strict build/codegen and 14 tests passed (`worker-bounded-wait-*-console.log`).
One fresh checkpoint profile `saint_lo_bounded_wait_01`, PID 27180, was opened
with CooperativeWorkerExit and TransitionDump. Original entry and four worker
idle-wait registrations are logged; mission-transition exit result is pending.
This experiment still does not seal thread creation, establish an audio/GPU
barrier or enable same-process Runtime destruction.

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
