# Third mission: Night Drop

## Gameplay crash reported 2026-10-19

Correction prepared 2026-10-19: pinned original nightd.dll inspected with
InspectCod3.java (private logs/ghidra-nightd-89174700.log). The preceding block
ends with bctr at 891746FC; target 89174700 is original blr (4E800020), followed
by zero padding at 89174704. Added exactly this four-byte function boundary in
configs/nightd_functions.toml and included it in the Night Drop manifest.
No replacement hook or generated function edit. Strict codegen now registers
89174700 and emits its original return. Build.ps1 -Stage Codegen, -Stage Tests
(10 Python + 6 native) and -Stage Build pass; private evidence is in
logs/nightd-89174700-{codegen,tests,build}-console.log. Initial sandboxed Ghidra
launch could not write Java settings; the approved rerun produced the evidence.

Original completed-Island profile preserved as nightd_before_boundary_fix_01;
restored isolated nightd_boundary_fix_01 and opened one normal-menu PID 8180
with all same-process barriers and DoF 35. Actual passage beyond the crash
still needs user acceptance; startup alone does not establish this fix.

The previous evening's log survives: `logs/native-20261018-224203-062.log`.
Night Drop registers 5855 functions and returns successfully from its original
entry at 22:43:43.577. Gameplay later ends at 22:48:36.891 with:
`[FATAL] Call to invalid or unregistered function at guest address 0x89174700`.
This is an unresolved runtime target during Night Drop, not evidence of a
failed Island teardown. The address must be checked against the pinned original
nightd.dll in Ghidra before adding a reviewed boundary or changing dispatch;
the log alone does not establish whether a boundary or a pointer is wrong.
No fix or repeat gameplay acceptance is claimed. Keep the user's current
Island session running while investigating offline.

## Observed blocker, 2026-10-17

User confirmed briefing skip and completion of Island in `island_movie_skip_01`.
Parent `logs/native-20261017-150521-277.log` at 15:22:43 records original
664-byte launch-data handoff and successful guest/audio/GPU/dispatch drains:
40 retained guest threads, none pending, native audio/XMA/command/vsync exits,
D3D12 queue idle and native Kernel Dispatch exit. This also verifies the later
C3197000 runtime barrier implementation in an actual campaign transition.
It remains an interprocess transition, not same-window Runtime recreation.

Successor `native-20261017-150521-277-relaunch.log` restores the 664 bytes and
accepts another briefing skip. At 15:23:14 it requests
`game:\sp\nightd\nightd.dll`. The manifest at that time only recompiled
Saint-Lo and Island. Original entry 89201B08 was absent from the function table;
the subsequent indirect call to 890689D0 was fatal. LOAD_RESULT=0 by itself
does not establish that original DllMain ran successfully.

## Static module integration

- Original nightd.dll SHA-256:
  AE2F69D2446FED315F298114BDDB3CBFC2CA79D631497F8254B974FAF79177DA.
  Build.ps1 rejects a different image before applying reviewed configuration.
- Ghidra XEX import independently confirms entry 89201B08 is a tail branch
  to 892019B0, and 890689D0 lies in original executable code.
- ExportCod3YieldHooks verifies three full context-save wrapper signatures and
  their original interface wait calls at 8915D2FC, 8915D42C, 8915D5B0.
  The generated config resumes through the existing host script scheduler;
  offsets from another level are not transferred.
- Manifest declares the real guest path and shared `cod3_campaign` overlay
  group. CMake compiles native cod3pc_nightd.dll and attaches continuation and
  original-entry tracing sources. Generated game functions are not hand edited.
- The real debug map is **nightdrop**, not the DLL stem nightd. Ghidra found
  `nightdrop` at 82069300 (mission-name table reference 82064CF4) and the original
  `+devmap nightdrop` string at 8207AB04. The debug selector uses this mapping.

Private evidence: `logs/ghidra-nightd-import-console.log`,
`logs/ghidra-nightd-map-name.log`, `logs/nightd-strict-codegen-console.log`.
Strict generation passed without force or unresolved targets. Native Build
and all 15 tests passed (`nightd-full-build-console.log`,
`nightd-tests-console.log`). The final map-name metadata build is separate in
`nightd-map-final-build-console.log`.

Preserved original profile after Island in verified private snapshot
`nightd_after_island_01`; restored to fresh `nightd_native_01` for normal-menu
runtime testing. Main userdata is untouched. Build/registry inclusion does not
prove third-mission entry, render, control or full progression; manual runtime
acceptance remains required. Remaining 12 campaign XEX DLLs are not integrated.

Installed and opened one normal-menu instance PID 20864 with nightd_native_01.
Log: logs/native-20261017-153108-502.log. The generated private registry contains
both original targets 89201B08 and 890689D0. Run-Native -ListLevels reports
nightd -> nightdrop declared/native/ready, which validates selector metadata,
not actual mission gameplay. Entry/render/control acceptance is pending.

User acceptance received: the level loaded and the character is controllable.
Log at 15:32:23.479 records original entry 89201B08 reason=1/result=1 and
LOAD_RESULT=0. Full Night Drop completion remains unverified.
