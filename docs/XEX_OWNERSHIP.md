# XEX allocation and resolved-module ownership

## Evidence from real Runtime teardown, 2026-10-18

The user confirmed Island from saint_lo_runtime_teardown_01. In parent
native-20261018-161702-857.log, at 16:21:00.676:

- TITLE_RUNTIME_RELEASE globals_cleared=true window_alive=true.
- COD3_TITLE_RUNTIME_DRAIN campaign_dlls_before=1 campaign_dlls_after=0 complete=true.

The old Runtime destructor returned before parent exit, and the campaign DLL
was no longer loaded. However, this was not clean acceptance: immediately
before completion, KernelState reported one recompiled library outside its
module sweep, and BaseHeap rejected a Release because the address was not a
region start. Same-process restart remains disabled until these are resolved.

## Independent reproduction

The existing real SDK probe has an optional --game-root mode. It initializes
the pinned tool Runtime and official kernel exports, parses the user's real
default.xex and Saint-Lo DLL via UserModule::LoadFromFile, queries allocation
metadata before/after loading the DLL, and checks the recompiled-module registry
against UserModule's actual resolved VFS path. It does not execute guest code,
launch a game window, or write campaign saves. Assets remain private/ignored.

Command after building cod3_runtime_ownership_probe:

    cod3pc/out/build/win-amd64-relwithdebinfo/ownership-probe/cod3_runtime_ownership_probe.exe --game-root "D:/Porting/Call of Duty 3 Xbox/assets/game"

Before correction, logs/xex-ownership-before.log and exit 55 show:
entry=82000000, before_state=3, after_state=0, allocation_preserved=false,
resolved_path_match=false. The actual level path is
\Device\Harddisk0\Partition1\sp\saint_lo\saint_lo.dll.

The executable and Saint-Lo hashes are the existing reviewed E057FFCD... and
49E38A11... values guarded by Build.ps1. No new game addresses are invented.
This optional private asset probe is not part of asset-free CTest runs.

## Narrow correction

rexglue-v0100-xex-ownership.patch changes two pinned SDK source files:

- ReadImage no longer resets the entire shared heap. Each reader already
  allocates its image's fixed range; loading a DLL must preserve the EXE's
  allocation record. A retail/dev-key retry rewrites the same image range.
- FindRecompiledModule retains its existing canonical direct lookup. Fallback
  resolves both the requested path and each game-relative registry path through
  the actual VFS and compares their full canonical device paths. It does not
  strip an arbitrary device root or guess identity from a basename.

This lets normal UnloadUserModule find the library/function registration and
defer library unloading until the existing safe end of KernelState teardown.
It does not bypass the sweep, hide its error, or force a successful heap free.
The patch adds no class members or public ABI changes.

## Verification

With the corrected DLL, logs/xex-ownership-after.log shows before_state=3 and
after_state=3, allocation_preserved=true and resolved_path_match=true. An
unrelated path with the same basename is rejected. The probe then unloads and
loads real Island, Night Drop and Saint-Lo images sequentially with the EXE
still mapped, verifying its allocation metadata after every load. Runtime
destruction clears globals. No guest entrypoint is executed in this test.

All 16 parser/native tests pass in logs/xex-ownership-tests.log. Strict codegen
passes in logs/xex-ownership-strict-codegen.log. Full application installation
passes in logs/xex-ownership-full-build.log. SDK source, installed prefix and
application runtime SHA-256 match
9AC854C973291AA9EE3B9D2802FC3BC4F1395000DFA56496304B0C33FD533201.
No cod3pc was open in the external inventory before installation/launch.
Restored isolated saint_lo_xex_ownership_01 from saint_lo_final and opened one
normal-menu PID 2736, log native-20261018-214705-386.log. Real transition is
accepted: user confirmed Island, the log unregisters Saint-Lo's 6073 functions,
returns from Runtime destruction with globals cleared/DLL count 1 to 0, and
contains neither original cleanup error. Same-process restart is the next
experiment, documented in INPROCESS_RESTART.md; it is not yet gameplay-verified.
