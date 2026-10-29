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
- A scoped Night Drop material correction removes the tested through-wall
  character shadows and black masonry artifacts while retaining ordinary shadows.
- Resolve readback restores visibility checks for the tested lantern glows;
  fresh launch and menu/Continue have been checked with the `fast` preset.
- Optional DoF strength is available; the 35% setting has been visually tested.
- Mouse navigation works in the original menus. Native-style Video and Keys
  pages support saved settings, a DoF slider/numeric entry, and action rebinding.
- Direct raw mouse look, held mouse fire/aim, weapon switching by wheel,
  grenades, melee and mounted MG42 controls have passed focused manual checks.
  Bindings support five mouse buttons and both wheel directions.
- Gameplay hints show assigned keys for supported actions. Keyboard/mouse
  input temporarily disables aim assist while preserving the controller setting.
- The tested sustained-MG42 audio loop no longer stutters with the XMA
  output-valid correction.

`default.xex`, `saint_lo.dll`, `island.dll`, and `nightd.dll` are currently recompiled.
The third mission also loads with player control in manual testing;
full third-mission completion remains unverified.
Briefings can be dismissed immediately with Space or Enter and an on-screen
skip hint; gameplay still waits for the original loading to finish.
The standard launcher now keeps the same process and window during title
transitions. Saint-Lo → Island, Island → Night Drop, repeated menu/Continue,
input/audio and settings/windowed-mode retention have passed manual checks.
For diagnostic process relaunch, use `-ProcessRelaunch`; stock/experimental GPU
backends require this option. Direct executable launches retain the SDK's
individual transition CVar defaults; use the launcher for the accepted preset.
Remaining work includes campaign modules, spatial audio, and broader stability
testing. Multiplayer is outside the initial scope.

The PC integration goal is settings inside the game's own styled/localized menus,
complete mouse and keyboard navigation, configurable actions and matching hints.
The features above are implemented, but full PC integration remains unfinished:
vehicle/QTE coverage, movement/stance hints and broader mixed-device testing
are pending. Binoculars and hold-breath bindings are implemented but their
dedicated manual checks are deferred.
External implementation reviews and the isolated SirCarls comparison build are
documented in [SIRCARLS_REVIEW.md](docs/SIRCARLS_REVIEW.md) and
[SIRCARLS_BUILD.md](docs/SIRCARLS_BUILD.md); external build outputs are not distributed.

See the [verified status](docs/STATUS.md), [roadmap](docs/ROADMAP.md),
and [known blockers](docs/BLOCKERS.md). Detailed development notes are currently
mostly in Russian.

## Credits

Special thanks to **SirCarls / NULL**, author of
[cod3_REcomp_](https://github.com/SirCarls/cod3_REcomp_), for sharing his work
and agreeing to its reuse with source attribution. Parts of cod3pc's PC menu
navigation, keyboard prompts, video settings, briefing skip, and gameplay input
are adapted from that project. Its contributors retain credit for the reused
code; cod3pc's integration and independent changes are documented separately.

The upstream [MIT copyright and license notice](licenses/SirCarls-COD_3_WAW-MIT.txt)
is preserved. Source review and attribution notes are recorded in
[SIRCARLS_REVIEW.md](docs/SIRCARLS_REVIEW.md); adapted source files retain
their upstream attribution.

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

Only integrated native modules can be selected; currently Saint-Lo, Island and Night Drop.
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
