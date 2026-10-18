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
reap result remains pending. The existing interprocess handoff is retained.
