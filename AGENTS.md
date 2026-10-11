# Project instructions

Target: original Xbox 360 Call of Duty 3 single-player, native Windows x64.
Stack: ReXGlue 0.10.0, official Xenos GPU plugin, original user-owned assets.
No interpreter/JIT/emulator process as final application backend.

## Layout and commands

- `cod3pc/`: actual SDK-generated application, CMake and manifest.
- `cod3pc/configs/functions.toml`: reviewed boundaries, pinned to XEX SHA-256.
- `tools/Build.ps1 -Stage Tests|Analyze|Codegen|Build|SDK|Diagnostic`.
- `tools/Check-Environment.ps1`: explicit native tool resolution, vcvars import.
- `assets/game/`: original directory tree, private and ignored.
- `external/`: ignored official SDK, rexauto extraction helper, Ghidra.
- `logs/`, `reports/`: private diagnostic evidence; never blindly git add.
- See `docs/STATUS.md` for verified milestones and outstanding work.

## Engineering rules

Use SDK APIs from the pinned local source and actual CLI `--help`.
The v0.10.0 CLI uses `--project-name`, `--xex-path`, `--project-root` and
`cod3pc_manifest.toml`; older wiki init flags are incompatible.
Force strict codegen to reanalyze with `--ignore-stamp`: a forced diagnostic
generation can otherwise cache unresolved analysis as up to date.

Never replace original game/menu with a mock, return success from empty imports,
suppress crashes, invent function addresses, or transfer MP TU3 offsets to SP.
Preserve PPC state, endian order and tail-call behavior. Confirm manual
boundaries with Ghidra and record instruction evidence and executable hash.
Do not edit generated functions by hand. Do not claim menu/gameplay from build
or runtime initialization alone. Validate entrypoint using log instrumentation.

Campaign contains 15 genuine XEX DLLs under `sp/`; inventory by magic, not only
extension. They will require separate static recompilation and module registry
before campaign works. Multiplayer module is outside initial scope.

Run focused parser tests, strict codegen, native build and real runtime checks
after meaningful changes. Record exact failures and commands in docs. Keep all
original or derived game code/data out of Git and remote builds. Commit only
project-authored source, scripts, configs, tests and documentation.
