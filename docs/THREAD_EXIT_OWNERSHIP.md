# Native exit and thread-object ownership

## Reproduced separately from the game

On 2026-10-18 a real SDK tool-mode Runtime and XHostThread reproduced:

`native_exited=true body_returned=true object_destroyed=0`, probe exit 2.

The control object, never started, was destroyed after the same table/caller
reference release. For the started worker, its actual native handle was already
signaled and the caller/table references were released, yet the object survived.
Private evidence: `logs/runtime-ownership-probe.log`.

Pinned SDK XThread::Exit retains `self` to survive ReleaseHandle, then calls
Windows ExitThread. That native call does not unwind the local C++ self-reference.
This is distinct from the previously verified native thread-exit barriers.

## Focused SDK correction

Patch `tools/patches/rexglue-v0100-thread-exit-ownership.patch` transfers that
owned reference into a KernelState-keyed retirement registry before the final
native Exit call. The same applies to self-Terminate. Reaping checks the real
native handle with a zero timeout; a running native owner remains retained.
References are moved out under the registry mutex and destroyed outside it.
No guessed reference-count decrements, forced Release calls or exception unwind
across guest/script fibers are introduced.

Create reaps previously exited owners. After positive dispatch shutdown the app
also reaps and logs COD3_TITLE_EXIT_REFS reaped/pending. Kernel teardown waits up
to five seconds for remaining transferred owners before object-table/memory
destruction and fails visibly if they remain. Public nonvirtual APIs add no
KernelState fields or graphics ABI changes.

XThread's converted main fiber needs separate retirement after owner exit.
Calling the old Fiber::Destroy from another thread would convert the reaper's
current fiber instead. DestroyAfterThreadExit deletes only the retired thread
fiber wrapper; XThread checks actual native exit before using it. The test
keeps the reaper itself converted and verifies its current fiber is preserved.

## Verification and limits

The isolated corrected probe reports object_destroyed=1 and pending=0.
It now checks a retained reference while its real native owner is blocked,
then three successive Runtime setup/thread exit/reap/shutdown cycles, including
cleared Runtime/KernelState globals. This synthetic SDK lifecycle regression is
not a replacement game, menu or proof of full campaign Runtime reset.

`native_runtime_ownership` is a focused CTest case; its binary and matched SDK
DLL live in a separate ownership-probe directory. It does not replace the DLL
loaded by a running cod3pc window. Build-TitleThreadDrain stages matched API,
fiber header, runtime DLL/import library/symbols. The diagnostic preserves
Runtime until process exit on failure instead of freeing unresolved state.

Initial probe compilation needed the pinned rex::X_STATUS namespace alias;
the corrected build succeeded. First corrected standalone run passed with
one retired reference. The strengthened blocked-owner/three-cycle regression
and full native application build are tracked separately in current logs.

This fixes a specific retained self-reference. It does not establish that every
object_ref abandoned on other guest call stacks, ThreadStartData, script TLS,
fiber stack or module capture is gone. Those ownership paths still need audit.
No DLL unload or same-process/same-window engine restart is enabled by this fix.

## Installed regression build

Full strict/native application build completed, log
`logs/thread-exit-ownership-full-build-console.log`. The installed/app SDK DLL
hashes match 6BA318A36C620C52C07E08C02C7C8A8D4E72EE49CD875F90E954C07F05A8A0EE.
All 16 tests pass in `logs/thread-exit-ownership-final-tests.log`.
The native ownership case reports three cycles with object_destroyed=1,
reaped=2, pending=0 and Runtime/KernelState globals cleared. The second owned
reference is deliberately queued while the native worker is blocked, proving
that reaping cannot release it early. Current-fiber preservation is also checked.
CTest captured output is private in Testing/Temporary/LastTest.log.

No cod3pc process was open when installing (checked through the external process
inventory); no user's gameplay was interrupted. Restored a fresh isolated
saint_lo_exit_refs_01 from saint_lo_final, then opened one normal-menu game,
PID 29220, log native-20261018-093413-667.log. The real transition/exit-reference
reap result is accepted: the user confirmed Island loaded. At 09:38:05 all
guest/audio/GPU/dispatch barriers complete, then COD3_TITLE_EXIT_REFS reports
reaped=19 pending=0 before the 664-byte successor handoff. The existing
interprocess handoff is retained; this does not prove complete Runtime reset.

## Native callback ownership, 2026-10-18

The next independent canary reproduced a separate ThreadStartData leak.
ThreadStartRoutine owns a heap-allocated std::function but its delete follows
the callback invocation. Thread::Exit calls raw ExitThread, bypassing that
delete. A shared capture expires after normal return but remains live after
native exit, even after releasing the native handle owner. Exact reproduction:
logs/start-routine-probe-before.log, mode=0 released=true, mode=1 released=false,
exit 38. This is the real SDK callback path, not a mock of its implementation.

Patch rexglue-v0100-start-routine-ownership.patch gives ThreadStartData a
thread_local unique_ptr owner in the Windows DLL. Normal return resets it;
explicit native exit releases it through the pinned build's TLS teardown.
Thread TLS follows the thread across fiber switches; no guest stack unwind,
forced unknown reference release, FLS callback or public ABI change is added.
The test verifies capture survival while blocked and release after actual
native exit for normal return, explicit Thread::Exit and exit from a secondary
Windows fiber. All three pass with the corrected DLL, followed by three
successful Runtime ownership cycles in logs/start-routine-probe-after.log.

The first secondary-fiber probe incorrectly called DeleteFiber on the exited
thread's selected fiber and failed with STATUS_HEAP_CORRUPTION (0xC0000374).
The corrected diagnostic leaves that retired execution context alone. This
canary proves callback release, not reclamation of every title-created fiber.
It does not cover external TerminateThread, arbitrary raw OS-created threads,
or other C++ references abandoned on guest stacks.

All 16 parser/native tests pass in logs/start-routine-all-tests.log; strict
codegen passed in logs/start-routine-strict-codegen.log (396 unchanged files,
no forced generation). Full application build/install passed in
logs/start-routine-full-build.log. Source, prefix and app runtime SHA-256 match
1404DBEEFF6D7963945F34F82410F493548098A704DD068C6D551B299A818551;
the rebuilt corrected GPU plugin is
679B47BB26B246AD1C5CC687BBF0AB5B8E698B64B5DCFA3186538512C5A285DF.
The external process inventory found no cod3pc open before installation, so
no gameplay was interrupted. Restored isolated saint_lo_start_data_01 from
saint_lo_final and opened one normal-menu PID 39792. The user confirmed Island
loaded. Parent native-20261018-094956-224.log records four complete drains,
reaped=19 pending=0 at 09:56:33, then the 664-byte handoff. Successor log
records Island entry result=1/LOAD_RESULT=0 at 09:57:06. A later original
return-to-title also drains successfully (reaped=18 pending=0); the final
menu window closes normally at 09:57:48. No seamless reset is enabled yet.

## Script TLS/fiber ownership diagnostic

ScriptStore now owns the same per-thread map previously held directly in TLS.
Its destructor clears retained host tasks on the owner scheduler thread and
logs SCRIPT_TLS_RETIRED. It does not inspect guest memory or resume scripts.
TLS teardown with an active script is a visible failure rather than deleting
the selected fiber recursively. Script-context and actual Cod3FiberTask counts
are process-wide atomics, never reset to manufacture a zero result.

After native guest/consumer drains, the app logs COD3_TITLE_SCRIPT_DRAIN with
remaining contexts/fibers and complete status. Nonzero counts trigger a private
consumer dump; they do not authorize freeing old guest memory. The existing
interprocess handoff is retained during this diagnostic.

The real SDK probe now creates a TLS-owned actual Cod3FiberTask, suspends it,
checks live count=1 while its native owner waits, then calls raw native Exit
from the parent scheduler. After the real native handle signals, count=0.
All 16 tests pass in logs/script-tls-all-tests.log, with SCRIPT_TLS_PROBE in
Testing/Temporary/LastTest.log. Strict codegen reports 396 unchanged files;
the three changed application objects compile in logs/script-tls-app-objects.log.
This covers suspended-task TLS retirement, not arbitrary C++ locals stranded
inside a discarded fiber stack. Full native build/install then passed in
logs/script-tls-full-build.log; runtime/GPU DLL hashes remain 1404DBEE/679B47BB
(full hashes above). External inventory confirmed no game open before install.
Opened one normal-menu PID 4968 with fresh saint_lo_script_tls_01 restored
from saint_lo_final; log native-20261018-153758-546.log confirms original entry
82344D00. The user confirmed Island. Parent log at 16:08:53 records
COD3_TITLE_SCRIPT_DRAIN contexts=0 fibers=0 complete=true and exit references
reaped=19 pending=0 after all native barriers. This accepts script TLS ownership
on that transition. The next experiment is documented in TITLE_RUNTIME_TEARDOWN.md.
