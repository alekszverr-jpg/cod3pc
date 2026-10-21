# Separate SirCarls reference build

Requested by the user on 2026-10-21 for hands-on comparison; this is not a
replacement of cod3pc. Checkout: external/sircarls-cod3-reference (ignored),
commit d54c1bb877ab364dae2d2f40aa8c6a8ca46e3070. This is newer than the initial
source review; do not silently attribute its additions to the reviewed commit.
The build selects their SDK fork commit
9c4441892f72b82be750ba89c13c3aec0953356d, based on upstream
c94f5ebdcb3c9d1a460ca48e04f9758448f8d518, per patches/fork.json.

Their CLI build --help accepts an extracted disc folder. For compatibility we
use our retained original previous-game tree, default.xex SHA1
681724c40e250e584e6585dd903e05afa00a8634, recognized by their preflight.
Current Russian assets and all cod3pc saves remain untouched. Their output
and work are separate ignored folders under external/sircarls-build/.

Build route: Enter-Toolchain.ps1, bundled Python, cod3_waw_setup.py build with
--windows --binaries-only --no-waw --no-install-deps --no-cleanup --keep-sdk
--jobs 4 --yes, explicit source/work/output directories. No external setup
installer was run. The binaries-only option omits the Rust ripper/content
export and texture upscale; original game assets are copied for native play.
Experimental optional FidelityFX 2/3 is not requested. Existing compiler,
Git, CMake, Ninja, Visual Studio tools and Windows SDK were detected.

Private logs: logs/sircarls-build.log and first failed attempt
logs/sircarls-build-attempt1.log. First attempt could not rename Git pack
files through the setup tool's short junction path. Fetching the pinned SDK
through the actual workspace path succeeded; the official build was resumed.
Source and submodules are being fetched/built in the separate SDK. Completion
and gameplay acceptance are not yet established.

Windows build adaptations (private reports/run-sircarls-real-path.py): bypass
only common.build_path's short junction and append -DCMAKE_CXX_FLAGS=-msse4.1
to SDK/application configure calls. The original setup sources are unchanged.
Without the SIMD flag, Clang's GNU frontend with the MSVC ABI rejects existing
_mm_shuffle_epi8 calls in SDK memory.cpp. The SDK helper supplies -msse4.1 only
when NOT MSVC, leaving this Windows compiler path without it. The parameter
is now verified in the separate CMake cache. A CMake preset-only retry changed
compiler settings; the official setup wrapper is used consistently thereafter.
Failed attempts remain in logs/sircarls-build-attempt*.log. No codegen functions
were hand-edited and no runtime behavior was patched to hide a build failure.

## Build and first launch completed

Official setup completed successfully after 17 minutes on the final resumed
attempt (earlier dependency fetch/configuration retries took additional time).
SDK installed privately, SP and automatically detected MP translated, 15
campaign DLLs and the applications compiled and packaged. Native application:
external/sircarls-build/output/bin/cod3.exe, 36,834,816 bytes, SHA256
1B244F1FE1E43953BD2714E4ABEA4A373B9A70916EAC3D58663D332B6AAD6544.
Reference checkout status is clean. Codegen logged
"BaseHeap::Release failed because address is not a region start" twice,
but returned success; retain this external warning rather than hiding it.

An elevated check found no cod3pc/cod3/cod3mp process before the reference
launch. Two game save/profile files were copied from our isolated Night Drop
profile into the new reference userdata/B13EBABEBABEBABE tree. No existing
reference destination was overwritten. Compatibility/Continue acceptance is
pending; copying files alone does not prove the save was loaded.

One reference PID 45160 started via its own output folder. Its private log is
external/sircarls-build/output/userdata/logs/cod3.log; run metadata is in
reports/sircarls-last-run.json. The log reaches game main-menu hooks, reports
33/33 action handlers and Vulkan swapchain/presentation at a 60 FPS limit.
This is startup/menu evidence, not campaign completion. Requested manual
Continue, UI/mouse and continuous-MG42 comparison; results pending.

The reference selects Logitech PRO X Wireless, 8-channel endpoint / 6-channel
submission at 48000 Hz. Our last test started on Realtek 2-channel output.
Its default Vulkan/60 FPS settings also differ from our D3D12 setup. Do not
attribute an audible/performance difference solely to runtime patches without
matching devices, output modes, scene and frame settings. Reference patch 0066
logged successful internal equality checks on 24 restarted decoders, up to
8 packets each; this is their runtime diagnostic, not acceptance of our MG42 fix.

Before first reference launch, check the current cod3pc session and close it
only when the user has finished. Use an isolated reference save/config and
record actual menu/gameplay behavior, frame times, input/UI integration,
lantern visibility and continuous MG42 sound. Our shader corrections are not
injected into this build. Original/derived code and game data stay private.
