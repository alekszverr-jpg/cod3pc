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
