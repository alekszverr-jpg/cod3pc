# Saint-Lo vegetation investigation

2026-10-12: the user reports long green grass strips through the scene and
explicitly requests preserving grass and correcting rendering. Disabling the
grass initializer is not the chosen solution.

The official Xenia Canary SP patch documents the same visible symptom and
works around it by removing grass:
https://github.com/xenia-canary/game-patches/blob/main/patches/415607E1%20-%20Call%20Of%20Duty%203.patch.toml
This is a symptom reference, not proof of a shared cause. Our original function
823F7A60 was separately inspected in Ghidra; no game-code bypass was added.

The native test with --readback_memexport=true and
--readback_memexport_fast=false still visibly stretches grass. This mode is
available only through Run-Native.ps1 -SynchronousMemexport for comparison.
The default remains the SDK behavior.

RenderDoc 1.46 portable x64 is kept in ignored external/renderdoc-1.46.
Official download SHA256:
9CA4D09ECABA2CC791168660D6FC2A7E3D70FE67146E87EC05C5F8DEA70772F7.
All RDC files and extracted shaders/textures remain private in reports/.
Frame 37178 mostly captures intermediate operations (236 draws, 9871 indices).
Sequential capture 16069 contains the world (666 draws, 416316 indices).
These captures are diagnostic evidence, not redistributable game content.

## Candidate coherency correction

SDK source is pinned to c94f5ebdcb3c9d1a460ca48e04f9758448f8d518.
SharedMemory::SetSystemPageBlocksValidWithGpuDataWritten invalidates CPU-uploaded
pages at frame boundaries. D3D12CommandProcessor::IssueSwap clears the fetch
bitmap, but IssueDraw can skip RequestRange when a cached fetch address and
size match. Matching addresses do not establish that the data was unchanged.

tools/patches/rexglue-v0100-vertex-residency.patch removes only this address/size
shortcut. SharedMemory still decides which pages require uploading and preserves
GPU-written pages. The per-submission fetch bitmap remains. The correction is
failed the user's visual check: the strips remained. The SDK source change was
reversed before the next experiment. It is retained as a separate negative
diagnostic, not enabled by the normal launcher.

Build and stage a separate test plugin:

```powershell
.\tools\Build-Gpu-Coherency.ps1
.\tools\Run-Native.ps1 -Seconds 0 -Interactive -FirstMission -GpuCoherencyTest
```

The test binary is rexgpu-xenos-coherencyrd.dll. The stock xenos plugin remains
available for comparison. Generated game code and original assets are unchanged.

## Separate gameplay failure found during capture

grass-renderdoc.log, 15:34:03, stops on unregistered target 82498510.
Ghidra confirms a separate original table-membership function at
82498510..82498558, returning 0/1, with padding at 8249855C. It is added to
configs/functions.toml with size 76. Strict codegen, native build and all five
existing tests pass. Further gameplay verification is still required.

## Full readback and captured shader evidence

`grass-full-readback.log` confirms memexport=true, memexport_fast=false,
resolve=full, page_refresh=true. The user confirmed that strips remained near
Huxley. Clean grass at the initial spawn was insufficient evidence of a fix.
Full readback remains an opt-in negative diagnostic.

The user recorded additional frames, including 93713 and 93794. In 93713,
grass uses vertex shader resource 792 and pixel shader 793. Draws 146 onward
use seven-vertex triangle strips, followed by 0xFFFF. The original index bytes
are big endian 0..6, restart, 7..13, restart; the shader swaps them correctly.
The D3D12 pipeline has indexStripCutValue=65535. RenderDoc remaps post-VS indices;
its compact indices must not be mistaken for original guest indices.

Active blades have six vertices far behind the camera and a seventh near the
camera. The captured source map identifies `sges r1.___w, r5.w`, which changes
the homogeneous input to the subsequent position matrix. The original shader
cache contains a unique match: vertex ucode hash 4B4E6FDA02FB39C3, 195 dwords,
instruction at dword 168 (decoded with pinned SDK bitfields; original bytes
remain in private reports). This is a shader compatibility investigation, not evidence that
general Xenos SGES semantics are incorrect.

In replay of frame 93794, making only this homogeneous component 1 removes the
large strips. A separate discard-only replay comparison confirms that dynamic
grass still contributes 11931 pixels (bounds x567..1039, y304..461). This is
limited to the captured view; it does not prove faithful appearance everywhere.

`tools/Build-GpuGrass.ps1` builds a separate `rexgpu-xenos-grassrd.dll` from the
pinned SDK with `rexglue-v0100-cod3-grass-position.patch`. The patch is opt-in via
`cod3_grass_position_fix=true` and guarded by the exact vertex ucode hash and
destination/source instruction fields. It changes one homogeneous result to 1;
it does not skip a draw, change SGES globally, or modify generated PPC code.
The exact hardware semantic cause remains unresolved. Native visual validation
is required before considering promotion of this compatibility workaround.

```powershell
.\tools\Build-GpuGrass.ps1
.\tools\Run-Native.ps1 -Seconds 0 -Interactive -FirstMission -GrassShaderTest
```

Candidate DLL SHA256:
60C65D577C8BA69E488159FCAE6DE9013BEF310F5850258AC4A9654DA32631E4.
The stock Xenos plugin remains the normal launcher default.

Validation: `Build-GpuGrass.ps1`, `Build.ps1 -Stage Build` (strict codegen with
`--ignore-stamp` and optimized native build), and `Build.ps1 -Stage Tests` all
passed. Native run `grass-position-test.log` entered original 82344D00, using
memexport=true, memexport_fast=true, resolve=none. It was launched through
RenderDoc with xenos-grass and cod3_grass_position_fix=true for visual checking.

Native result: the user confirmed "Длинные полосы пропали, сама трава видна,
но она выглядит как-то странно". New captures 28219, 28583, 29082 show short
slanted ribbon-like vegetation near Huxley. This confirms removal of the large
stretches, but not correct blade shape or original visual fidelity. The shader
compatibility experiment is therefore not promoted to the normal launcher.

A RenderDoc vertex-debug attempt on the captured original shader did not
complete within several minutes and was stopped without a result. Post-VS mesh
extraction, source maps, replay comparisons and native captures are the usable
evidence. The user offered RPCS3 reference screenshots; these can guide shape,
height and density comparison, while Xbox GPU semantics still require local
shader evidence.
