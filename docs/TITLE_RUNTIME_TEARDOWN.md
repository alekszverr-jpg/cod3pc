# Drained title Runtime destruction

## Verified prerequisite, 2026-10-18

The user confirmed Island from saint_lo_script_tls_01. At 16:08:53 in
native-20261018-153758-546.log, all four native drains complete, exit-reference
retirement reports reaped=19 pending=0, and COD3_TITLE_SCRIPT_DRAIN reports
contexts=0 fibers=0 complete=true. SCRIPT_TLS_RETIRED also records empty stores.
This accepts the script TLS ownership diagnostic on the real transition.

## Experimental teardown

Run-Native.ps1 -TitleRuntimeTeardown requires -TitleDispatchDrain and its
guest/audio/GPU prerequisites. It is opt-in and false by default. The existing
664-byte native process handoff is still queued before teardown; the successor
waits for actual parent-process exit. This is not same-process engine restart.

SDK patch rexglue-v0100-title-runtime-teardown.patch adds a watcher hook after
OnGuestThreadExit returns and the watcher releases its main XThread reference.
The ordinary hook still receives a valid thread with its Runtime alive.
The application only requests teardown after positive consumer barriers,
zero pending exit references and zero script contexts/fibers.

ShutdownTitleRuntime marshals cleanup to the UI thread. It independently checks
sealed/completed guest creation, actual native guest exit, no transferred exit
references, and actual dispatch exit. Failure retains the Runtime. The caller
remains responsible for its already-verified audio/GPU/script barriers.

Runtime-bound overlay keybinds, achievement notification subscriptions, dialogs,
drawers and presenter bindings are released using the same cleanup helper as
normal OnDestroy. Display-window/drawer pointers are detached. Runtime is then
reset while ReXApp still owns the window. The helper never joins its calling
watcher, destroys the window, or calls guest DllMain during teardown.

The pinned Runtime destructor stops subsystems, destroys KernelState (including
module unregister/deferred library unload and object-table cleanup), then
destroys its function dispatcher, resolver, VFS and memory. No guessed manual
reference decrements or direct memory unmapping is introduced. The diagnostic
checks Runtime::instance/KernelState::shared and Windows handles for all three
compiled campaign DLLs after this destructor returns.

TITLE_RUNTIME_RELEASE reports globals_cleared/window_alive.
COD3_TITLE_RUNTIME_DRAIN reports campaign_dlls_before/after and complete.
Island loading alone is insufficient acceptance: both teardown logs must
succeed, with zero old campaign DLLs, before the successor begins gameplay.
If destruction hangs/crashes, the missing completion is a teardown failure even
if the existing parent-exit handoff subsequently starts Island.

The ReXApp hook/helper compile into the application from the matched installed
SDK header and share/rexglue/rex_app.cpp. No GPU interface or plugin ABI changes.
Build-TitleThreadDrain stages these files and verifies the narrow patch.

## Validation and pending game check

All 16 parser/native tests pass in logs/title-runtime-tests.log, including three
real tool-Runtime shutdown cycles, native callback ownership and suspended
Cod3FiberTask TLS retirement. These do not exercise game DLL/renderer teardown.
Strict reanalysis passes in logs/title-runtime-strict-codegen.log; full native
build/install passes in logs/title-runtime-full-build.log.
CLI rejects the flag without its prerequisite (title-runtime-cli-invalid.log)
and accepts the complete dry-run command (title-runtime-cli-dryrun.log).

The user had already closed the game; external process inventory was also empty.
Restored fresh saint_lo_runtime_teardown_01 from saint_lo_final and opened one
normal-menu PID 39580, log native-20261018-161702-857.log. Real teardown and
successor acceptance are pending. Main campaign userdata was not modified.

After acceptance, remaining work includes reuse of the retained window,
presentation/input reattachment, Runtime construction inside the same process,
and restoration of the original 664-byte launch state before original startup.
