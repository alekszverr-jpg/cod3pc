# cod3pc — Call of Duty 3 native Windows port

An unofficial, experimental native Windows x64 port of the **Xbox 360
single-player version** of Call of Duty 3, built with static recompilation
using **ReXGlue 0.10.0**. The application runs recompiled game logic with the
ReXGlue runtime and Xenos GPU backend; it does not launch an emulator process.

**Early development. The full campaign is not supported yet.**

## Current progress

- Original menu, Saint-Lo training and gameplay run in the native application.
- The first two missions have been completed in manual testing; campaign progression
  remains unfinished and this is not a stability guarantee.
- A Saint-Lo checkpoint has been saved and restored after restarting.
- Grass rendering has a verified compatibility correction.
- Optional DoF strength is available; the 35% setting has been visually tested.

`default.xex`, `saint_lo.dll`, and `island.dll` are currently recompiled.
The third mission's `nightd.dll` also loads with player control in manual testing;
full third-mission completion remains unverified.
Briefings can be skipped with Space or Enter after the original resource wait.
An opt-in Saint-Lo → Island transition has been verified in the same process and
window, with working input and audio; returning to the menu and Continue also
work in that instance. The standard launcher still uses native process relaunch
while further mission transitions and fullscreen/settings are tested.
Remaining work includes campaign modules, spatial audio, and broader stability
testing. Multiplayer is outside the initial scope.

See the [verified status](docs/STATUS.md), [roadmap](docs/ROADMAP.md),
and [known blockers](docs/BLOCKERS.md). Detailed development notes are currently
mostly in Russian.

## Building locally

You need Windows x64, MSVC/Windows SDK Build Tools, Python, and the pinned
ReXGlue toolchain. VS Code can be used as the editor; the full Visual Studio
IDE is not required. This repository is a development project, not a packaged
end-user release.

You must provide and extract your own Xbox 360 game files locally into
`assets/game/`, preserving the original directory structure. Reviewed function
boundaries are pinned to the executable hashes documented in the configs.

Follow the [build instructions](docs/BUILDING.md) for toolchain setup before
running:

```powershell
./tools/Check-Environment.ps1
./tools/Build.ps1 -Stage Tests
./tools/Build.ps1 -Stage Analyze
./tools/Build.ps1 -Stage Codegen
./tools/Build.ps1 -Stage Build
./tools/Run-Native.ps1 -Seconds 0 -Interactive
```

For softer depth of field:

```powershell
./tools/Run-Native.ps1 -Seconds 0 -Interactive -DofStrength 35
```

The range is 0–100; 100 preserves the original effect and is the default.
Continue from a checkpoint using the original game menu.

For level testing with a separate save profile:

```powershell
./tools/Run-Native.ps1 -ListLevels
./tools/Run-Native.ps1 -Seconds 0 -Interactive -DebugMission island -DofStrength 35
```

Only integrated native modules can be selected; currently Saint-Lo and Island.
Debug launches use the original engine's `devmap` command and isolated data
under `userdata/debug-levels/`. Normal campaign saves are not modified.

## Repository contents

This repository contains project source, configuration, SDK patches, tools,
tests, and documentation. **No game assets, original executables, recompiled
game code, saves, GPU captures, or game binaries are distributed here.**
Generated code and local diagnostics remain ignored. Build and analyze the
game locally; do not upload proprietary game files to issues or remote builds.

ReXGlue boilerplate and SDK patches retain their upstream notices; see
[third-party acknowledgements](docs/THIRD_PARTY.md) and
[the SDK license](licenses/rexglue-sdk.txt).

This project is not affiliated with or endorsed by Activision or Treyarch.
Call of Duty and related names belong to their respective owners.

[Architecture](docs/ARCHITECTURE.md) · [Research](docs/RESEARCH.md) ·
[Testing](docs/TESTING.md)
