# SirCarls cod3_REcomp_ reference review

Reviewed 2026-10-20: [repository](https://github.com/SirCarls/cod3_REcomp_),
revision `2d6d40e6df380f8aa15365a6f308b47a8610ab38`.
GitHub reports fork=false, but the root README explicitly says it is heavily
inspired by FrankHUN88453/CoD3Recomp. This does not establish independent
provenance for every implementation. No external executable was run, no
campaign or performance claim reproduced, and no foreign code integrated.
Selected source evidence stays ignored under reports/sircarls-review/.

## Useful references and limits

The release's SDK patch notes pin ReXGlue to
`c94f5ebdcb3c9d1a460ca48e04f9758448f8d518`, the same base used here.
Their accumulated patch stack still differs from ours; shared base alone
does not make a patch safe to apply.

| Source under RELEASE/COD_3_WAW_Setup-tool/ | Observed implementation | Use for cod3pc |
|---|---|---|
| patches/0012-gpu-frame-stats-async-occlusion-queries.patch | D3D12/Vulkan queue query results with submission IDs, deliver after completion, submit/poll during idle waits, drain pending queries when disabling | Reference for readiness, fence ordering and teardown tests. It changes delivery timing; it is not evidence that our different-address ZPD snapshots or lantern visibility are fixed |
| patches/0024-gpu-resolve-readback-cpu-reads-only.patch and patches/README.md | Read-watch resolve destinations and arrange GPU readback when guest CPU accesses them; notes report one CPU-read destination in Saint-Lo and hypothesize light-flare visibility | Investigate whether our flare decision reads a resolved image/depth buffer instead of, or in addition to, occlusion-query results. The author's flare attribution is a hypothesis, not our measured result |
| patches/0006-gpu-scalar-operand-z-with-three-source-vector-ops.patch | General parser/interpreter change: scalar second component Z when paired vector operation has three operands; cites Xenia Canary PR 1149 and grass shader 4B4E6FDA02FB39C3 | Compare instruction evidence with our accepted grass and shadow-material corrections. Do not broaden our hash/pattern-scoped overrides from this reference alone; interpreter changes are not an application backend for this project |
| recomp/src/game/shadows.cpp | Original/off dynamic-shadow setting; off skips caster insertion while preserving light slots and map clearing | Optional future settings reference. This file does not establish a repair for through-wall shadows; our accepted material correction remains enabled |
| recomp/src/game/relaunch.cpp | Windows CreateProcessW; POSIX fork/execve with saved launch data | Process relaunch reference only. Does not replace our accepted same-process Runtime recreation |
| patches/README.md and release documentation | Additional work on resolution scaling, texture replacement, audio devices, timing and input | Candidates for later focused reviews, not accepted features or fixes in cod3pc. Keep original presentation and current campaign/rendering blockers first |

## Next glow investigation

1. Finish the pending original GetData probe: hidden lamp versus direct view,
   readiness/status, caller, query results and device flags. No forced visibility.
2. If that path does not explain the sprite, locate the original flare decision
   and its memory reads. Check resolve destinations, freshness, format, endian
   order and synchronization against the captured guest frame.
3. Use our existing opt-in `-FullGpuReadback` as a controlled comparison only
   when evidence points to resolve/readback data. Record hidden/direct behavior
   and frame time; a visual difference must be traced to the actual buffer.
4. Only then design a minimal correction with memory-lifetime and transition
   checks. Do not import the large read-watch/performance patch stack wholesale.

The external notes describe Vulkan runtime measurements and D3D12 build-only
validation for patch 0024. Neither proves behavior on our Windows/D3D12 scene.
Our earlier cumulative-query experiment did not remove the glows; keep it off
in standard launches. Pending manual testing remains deferred by the user.

## Source and license boundary

Pinned [patch notes](https://github.com/SirCarls/cod3_REcomp_/blob/2d6d40e6df380f8aa15365a6f308b47a8610ab38/RELEASE/COD_3_WAW_Setup-tool/patches/README.md)
and [release license](https://github.com/SirCarls/cod3_REcomp_/blob/2d6d40e6df380f8aa15365a6f308b47a8610ab38/RELEASE/LICENSE)
are the reviewed sources. Root metadata says MIT; the release license
separately assigns SDK patches to ReXGlue's BSD-3-Clause licensing with Xenia
portions. Review applicable notices and provenance before actual reuse.
Original/derived game code, generated functions and assets are not imported
or covered by an assumption about the repository's top-level license.
