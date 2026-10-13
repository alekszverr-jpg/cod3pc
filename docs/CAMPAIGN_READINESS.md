# Campaign module readiness

2026-10-13: `tools/check_campaign.py` finds campaign XEX images by their XEX2
signature, not their extension, parses their actual headers and compares source
paths with the generated application's manifest. The private output is
`reports/campaign-readiness.json`. Hashes and original image metadata remain
local. Run it with the project Python from Enter-Toolchain.ps1:

```powershell
. .\tools\Enter-Toolchain.ps1
& $Python tools/check_campaign.py
```

The actual assets contain 15 genuine campaign images under `sp/`. Every one has
preferred image base 0x89000000. Only Saint-Lo is currently declared in
cod3pc_manifest.toml. Declaration does not prove code generation, a successful
native load, or gameplay. The other 14 images still require separate static
recompilation and reviewed module-specific coroutine boundaries.

The pinned SDK already provides the relevant dynamic loading mechanism:

- KernelState::LoadUserModule finds a matching guest path, loads its native
  library, checks ReXModule_GetImageInfo against the loaded original image and
  calls ReXModule_Register through FunctionDispatcher::RegisterModule.
- KernelState::UnloadUserModule invokes the original DLL detach entry when on
  a guest thread, unregisters functions, invalidates the corresponding thunk
  cache and defers unloading the native library.
- FunctionDispatcher::UnregisterModule clears its recorded function addresses,
  thunk pool and memory function table.

Evidence comes from the local pinned SDK source, not inferred registry behavior.
These APIs are necessary for sequential missions sharing the same address range;
they do not establish a successful mission transition. A real transition must
also show that the original engine releases its suspended script continuations
before the old level library is unloaded.

Current runtime check: the prior grass-default-run.log ended with normal window
shutdown, not a crash. The user has tested the camp and training area. A fresh
native run was opened for training, weapons, grenades and the first battle;
completion and progression remain unverified. A second module should be selected
from actual engine/transition evidence, without transferring Saint-Lo offsets
or multiplayer title-update offsets.

Focused synthetic tests cover XEX magic under an unexpected extension, a DLL
filename containing ordinary data, a mismatching guest path and duplicate
manifest source declarations. No original game code is used in these fixtures.

2026-10-13 update: the user reports completion of Saint-Lo and the application
closing immediately afterwards. native-20261013-130703-196.log ends with
Execution complete (13:23:28.224), with no FATAL. It does not identify a requested
next module. Added logging-only hooks for original XexLoadImage (82343E98),
its return (82343E9C), XexUnloadImage (82343E14), XamLoaderLaunchTitle
(823439B0), and XamLoaderTerminateTitle (82344D48/82344EBC).
Ghidra instruction evidence: logs/ghidra-transition-trace.log, pinned default
SHA-256; return point 82343E9C follows the verified bl at 82343E98.
Original loading and exit behavior remain intact. Eight tests, strict codegen
and native build passed (transition-trace-*-console.log).
New diagnostic run: logs/native-transition-dof.log, started through the menu.
First-mission completion is reported, second-mission transition is unverified.

The user clarified that Saint-Lo closed automatically on progression, not after
pressing Exit. In the subsequent capture run the user selected the second mission;
native-transition-dof.log at 15:41:18.203 records the actual engine request
game:\sp\island\island.dll. The image loads, but its entry 89226748 is absent
from the function table; the following indirect call fails at 89089260.
This is evidence to select Island for separate static recompilation, not evidence
of playable second-mission gameplay. Its native code is not in the manifest yet.
The menu capture was restarted at the user's request: native-dof-retry.log,
reports/dof-retry capture template. Saint-Lo must be selected for the DoF check.
