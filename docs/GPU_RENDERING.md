# Saint-Lo vegetation investigation

Current result (2026-10-13): the scalar-pair compatibility correction is enabled
by default in the Windows RelWithDebInfo application and normal Run-Native.ps1
launch. Native frames 22456 and 23280 show tall upright grass without the previous
long strips or slanted ribbons; the user confirmed the corrected orientation.
Build.ps1 -Stage Build builds/stages the pinned custom Xenos plugin automatically.
Use Run-Native.ps1 -StockGpu for the original SDK backend. Debug and Release
still use stock Xenos. This is a game-specific compatibility correction, not a
verified general change to Xbox 360 ALU semantics or a full campaign guarantee.

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
At that stage, the stock Xenos plugin remained the normal launcher default.

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

## RPCS3 reference and scalar-pair experiment (2026-10-13)

The user supplied three PS3/RPCS3 views of the Huxley area. Vegetation is dense,
tall and approximately vertical; the homogeneous-W experiment instead produces
short slanted ribbons. The images are preserved privately as
reports/rpcs3-grass-reference-{1,2,3}.png. PS3 screenshots guide appearance but
do not establish Xbox 360 instruction semantics.

Replay instrumentation of the original shader exports intermediate registers
through existing varyings. The grass-height parameter is positive (one sampled
blade: 10.7828), while the scalar-derived curve offset makes the homogeneous
component differ between vertices. The pinned decoder interprets a co-issued
MAXS as selecting between c80.z (variation) and c80.x (animation time). Two
co-issued SUBS operations likewise derive offsets from differing components of
the shared third vector source. Those readings are a narrowly scoped hypothesis,
not proof that the decoder is generally wrong.

Replay of frame 93794 with the second component equal to the first in these
three operations produces upright vegetation without the earlier forced-W
change. The original SGES operation is retained. Before/after images and
instrumented shaders remain private in reports/grass-coissue-*.

The new SDK patch is rexglue-v0100-cod3-grass-alu.patch. It is guarded by the
same exact vertex ucode hash, the three source/destination register pairs,
opcodes, write masks and the co-issued third vector operand. It changes only
these scalar operands. It does not alter the general ALU decoder, skip grass,
or substitute original PPC functions. Build-GpuGrass.ps1 replaces the previous
W experiment and stages the separate xenos-grass plugin; Run-Native.ps1
-GrassShaderTest now selects cod3_grass_scalar_pair_fix=true.

Candidate SHA256:
875171E78169F4B4CE3EC5715F352E9F5D313E2E8162144F113CABA80039D391.
Build-GpuGrass.ps1, strict Build.ps1 -Stage Build and all five existing tests
passed. The native diagnostic run is logs/grass-alu-test.log. The user confirmed
vertical grass and recorded frames 22456 and 23280; both were visually inspected.
They agree with the reference's upright shape and show no prior long strips.

The final patch defaults cod3_grass_scalar_pair_fix to true in the dedicated
plugin. Final plugin SHA256:
999E7D41F2CE65598722B03E5E04DDDCE7F283D2DB4ED2451EB052A693EE303E.
The application selects xenos-grass by default only in Windows RelWithDebInfo.
An ordinary EXE launch without --gpu_plugin was checked: the process loaded
rexgpu-xenos-grassrd.dll and entered original 82344D00 (grass-default-run.log).
The final strict native build and all five tests passed. Original instructions,
images and GPU captures remain private; Git contains only authored patches and
diagnostic documentation.

## Reported excessive aiming depth of field, 2026-10-13

The user supplied three aiming screenshots: the weapon and HUD remain sharp,
while most of the world is heavily blurred, including the aimed-at subjects.
Private originals: reports/dof-report-1.png through dof-report-3.png.
This identifies a post-processing symptom; it does not establish incorrect
depth resolve, shader constants or ALU translation without a captured frame.
No DoF-disable workaround or guessed shader change has been applied.
One native menu launch was prepared with RenderDoc 1.46 (verified capture
--help), capture template reports/dof-transition, runtime log
logs/native-transition-dof.log. Requested aiming and non-aiming captures
from the same viewpoint. Next: identify blur/composition passes, inspect their
depth and focus inputs, then compare to the original/reference behavior.

Six captures received: dof-retry frames 15707, 16504, 17036, 17585,
61322 and 61500. Final images exported through RenderDoc replay and inspected.
The first four show aiming. The later pair shows aiming (61322) and hip view
(61500) at the training targets: the hip view is sharp, the aiming background
is heavily blurred. This is a reproducible aiming-dependent defect report.

Replay identifies a readable R32_FLOAT depth texture (1040x624), with weapon
and scene silhouettes, not a uniformly empty depth resource. Sampled values
in frame 61322 vary from 0.00573 to 0.01800 at selected scene points. One
depth-dependent alpha pass was inspected (event 939, shader ResourceId::645),
but its color/alpha constants also resemble fog; its role as the DoF focus
pass is not proven. Do not patch that shader merely because it reads depth.
The generated DXBC and constants, source textures, full draw summaries and
PNG outputs remain private under reports/dof-*. A full pass review, including
screen grids with more than six indices, is ongoing. No DoF fix applied yet.

RPCS3 reference comparison received: five screenshots at the same training
area (2026-10-13 16:49). The aimed-at helmets and Huxley's face remain sharp;
more distant scenery is blurred. The corresponding native targeting view
blurs the helmets too. Private reference copies: reports/rpcs3-dof-*.png.
This supports an incorrect focus selection/mask report, rather than a request
to remove the intended effect. PS3 screenshots establish a visual reference,
not a proof of Xbox 360 shader hardware semantics.

Added Run-Native.ps1 -ShaderDiagnostics to enable the pinned SDK dump_shaders
CVar under reports/shader-diagnostics. A bounded 30-second native menu run
entered the original entrypoint and produced original ucode and translated
shader dumps. PowerShell parsing of the launch script passed. All dumps are
private; this switch only requests diagnostic output, not different rendering.

Candidate original mixing shader found in the cache: pixel ucode hash
257C1107399A3162. It samples tf0/tf1 RGB and tf2 X, forms two scalar distances,
clamps their maximum and interpolates RGB. A depth-edge downsample shader
8E6F505DEE7CDF9E was also identified from its original instructions. These
identifications must still be correlated to the actual active draw and inputs;
the first bytecode matching attempt did not find an exact container/code match.
No change to these shader operations has been applied.

A new ordinary-menu capture run is open (native-dof-sequence.log), with
RenderDoc reference-all-resources enabled. Its local target control is armed
to capture four consecutive frames following the user's first F12. This is
needed to correlate the complete post-processing chain, since the single-frame
captures do not unambiguously expose the candidate final mixing draw.

Sequential captures 63427–63431 were received. The full scan identifies the
actual final DoF mix in event 265 of frame 63427 (24 indices): exact DXBC
instruction/body matching links it to original ucode 257C1107399A3162.
The same pass is event 266 in frame 63429. Earlier bytecode matching ran before
all requested dumps were available; re-running matches the actual shader.
The depth-edge downsample is 8E6F505DEE7CDF9E, event 114 of frame 63427.
The previously inspected depth-dependent constant-color pass is fog, not DoF.

The mix reads original sharp color (26596), depth R32_FLOAT (26539), and blurred
color (247028). Both color inputs were exported and visually inspected: the
sharp input retains the soldier's face, while the blurred input loses it.
Shader operations reconstruct q=1-depth, apply the far-focus threshold and
clamp the mixing weight. Retrieved float constants include threshold
0.958908617 and slope 100, with near blur disabled (-infinity). At face points
(520,240)/(520,250), depth is 0.02615/0.02605; these values yield a saturated
far-blur weight of 1 under the retrieved constants. Weapon depth 1 yields 0.
This numerically explains the observed symptom; the source of the wrong
focus/range correspondence still needs original-code tracing.

Checked the host float24-depth scaling hypothesis: the corresponding depth
surface (26558) has 0.01307/0.01302 at those points, while the game's resolved
depth has precisely twice those values. Thus the expected host-half/guest-full
conversion is present; multiplying the resolved depth by two would be a guess,
not an established fix. Pixel debugging repeatedly cycles through the translated
shader's control loop; a bounded 2000-state trace is saved, but is not used as
proof of correct shader debugging. Dynamic bindless resources must be inspected
without onlyUsed=True, which omits their bindings here; direct heap-index reads
without the full descriptor-table mapping are not reliable evidence.

Added FindRenderStrings.java for read-only Ghidra searches of original render
strings and existing/pointer references. It does not add function boundaries
or modify original code. Its compile and scan succeeded; DOF labels were
found. All shader dumps,
traces, raw bytecode, numerical samples and reference images remain private.
No rendering behavior has been changed and no corrected native result claimed.

Following the original DOF string at 8207B628 identifies registration in
823F0128: the parameter pointer is stored at object offset 0x384. Renderer
methods 823F5028 and 823F5868 copy its four values from global 825B3070 through
82147110 and 82141CF0. The original setter 82147098 stores f1..f4 at that global;
its instruction bytes and disabled-mode branch were checked with Ghidra
(logs/ghidra-0x82147098.log). The fixed-function path in 82149C80 projects the
distance values using near/far camera distances before uploading constants.
These findings narrow the investigation but do not establish a rendering fix.

Added read-only Cod3TraceDofParameters at the original setter's BLR 821470D0.
It records effective mode and four FPR values only when changed, at most twice
per second. The callback leaves all guest registers/memory untouched and logs
the original disabled-mode result. Strict reanalysis/codegen and native build
passed; generated code invokes the callback after the four original stores.
Validation: tools/Build.ps1 -Stage Build (logs/dof-trace-build-console.log),
tools/Build.ps1 -Stage Tests (six parser tests and two native tests passed,
logs/dof-trace-tests-console.log). A final incremental build also passed.

Manual near/far aiming check received. Runtime native-20261013-230114-517.log
records every observed aim activation as mode=1, values=[0,144,2,100000], and
release as mode=0, values=[1,1,1,0]. No changed focus distances were observed
in this check. This is evidence about this setter, not proof that Xbox 360
requires automatic focus or that the callback covers every rendering path.
The projected far threshold agrees with the earlier captured value:
(1 - 6/144) * (100000/(100000-6)) = approximately 0.9589086.
Thus the observed threshold is consistent with the requested original distance,
rather than a random constant-upload corruption. The target-depth discrepancy
and the visual difference from RPCS3 remain unresolved.

FindAddressReferences.java compiled and scanned original default.xex read-only.
It found the DoF wrapper 824980C0 in the original address/key table at 825F5D34
(preceding key 000003B8), and the two direct setter branches 820C1518/824980F0.
Private evidence: logs/ghidra-dof-callers.log and ghidra-dof-table-context.log.
No guessed adjustment to 144, the depth scale, or blur strength was applied.

Follow-up manual check in the same running build: the user approached a soldier
or crate closely and confirmed that the aimed-at close object stays sharp.
This rules out a uniformly blurred world in that check. Together with the
unchanging original setter values and the matching projected GPU threshold,
the current evidence supports a working near sharp region and a fixed far
blur threshold. It does not establish parity with the original Xbox 360;
RPCS3 may differ in game logic or tuning. DoF remains a visual discrepancy
requiring equivalent-distance Xbox 360 reference or further source evidence,
not a confirmed broken depth conversion. No DoF behavior patch was applied.

The user then checked the Xbox 360 game in Xenia and reported the same intense
DoF as this native port, while RPCS3's PS3 game has milder, more useful focus.
This establishes reported visual agreement with Xenia, not physical-console
parity. ReXGlue's Xenos GPU code derives from Xenia, so a shared rendering
issue remains possible; platform-specific game/shader tuning is also possible.
No exact Xenia build/settings or equivalent-distance GPU capture were supplied.
Do not treat the PS3 appearance alone as proof that this port's DoF is broken.
An optional milder DoF setting would be a deliberate visual customization,
unless a shared Xenos issue is established from original hardware evidence.

2026-10-14: optional DoF strength added at the user's request. The D3D12
translation patch applies only to pixel ucode 257C1107399A3162 and its final
color-target MAD. It scales the blurred-minus-sharp contribution before adding
sharp color: sharp + (blurred - sharp) * original_weight * strength/100.
At 100 the additional operation is omitted; at 0 the final mix retains sharp
color. It does not change the focus depth, fog, grass, or game assets.
This is a visual customization, not a claim of Xbox 360 compatibility repair.

Strength reduction is encoded in seven previously unused pixel-modification
bits, so each setting gets a distinct shader/pipeline key. The cache version
is bumped to 20261014; manual cache deletion is unnecessary. The SDK CVar
cod3_dof_strength defaults to 100 and is clamped to 0..100. Run-Native.ps1
exposes -DofStrength with validated range and rejects custom strength on
plugins without this patch. Build-GpuGrass.ps1 reproducibly applies the
authored tools/patches/rexglue-v0100-cod3-dof-strength.patch after the grass
patch against the pinned SDK source. Staged plugin SHA256:
351334FB5D30E7E51716EEEFF30E501CE271414C29E658F64A04534AE5932F77.

Strict codegen and native/GPU build passed (logs/dof-setting-build-console.log).
Six parser tests and two native tests passed (dof-setting-tests-console.log).
Launch script PowerShell parsing passed. One ordinary-menu game was opened
with -DofStrength 35 -ShaderDiagnostics; visual acceptance and translated
DoF shader inspection are pending actual level rendering.

Acceptance received in the same 35% run: the user confirms a sharper image
with slight background blur. The emitted shader variant
257C1107399A3162_0041400000000001 was dumped during level rendering; local
fxc /dumpbin inspection confirms RGB contribution multiplied by 0.350000
immediately before adding sharp RGB, with alpha still set to 1.
Private evidence: reports/dof-strength-35-dxbc.txt, lines 415–418.
The 35% mode is visually accepted; 100 remains the default original mode.

## Open: lantern glow visible through opaque geometry, 2026-10-15

User supplied D:/Screenshots/cod3pc_2026_10_15_17_02_59_205.png and
cod3pc_2026_10_15_17_03_20_079.png. Small yellow glow spots appear over the
wooden floor/opaque interior geometry; the second image shows the lantern and
another spot over the stone wall. This looks like a glow/flare visibility issue,
but screenshots do not establish the responsible draw or a missing shadow.
Do not treat depth testing, depth sampling or occlusion-query failure as a
confirmed cause. The original Xbox 360 behavior for this scene is not compared.

Available local RenderDoc captures predate these screenshots (latest Oct13
DoF captures), so they cannot identify the defective draw here. During the
next diagnostic reproduction near the saved Saint-Lo endpoint, capture the
lantern visible and then fully occluded by the same wall. Inspect the draw's
depth state/attachment, any sampled depth and visibility-query inputs/results;
compare the original scene if needed. Preserve the actual lamp lighting and
glow, rather than globally disabling an effect. No rendering change yet.
