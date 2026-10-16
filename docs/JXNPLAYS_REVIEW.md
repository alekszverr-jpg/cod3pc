# COD3 Reborn source review

Reviewed 2026-10-16, repository jxnplays/cod3-recomp, commit
ab73d51beba4c0a5e3cf2526f592135f4f91c136. GitHub metadata reports fork=false,
but [PROVENANCE.md](https://github.com/jxnplays/cod3-recomp/blob/ab73d51beba4c0a5e3cf2526f592135f4f91c136/docs/PROVENANCE.md)
explicitly identifies it as a continuation of FrankHUN88453/CoD3Recomp, with
its earlier independent runtime archived. Its inherited implementation must
not be counted as a separate independent solution to the same problems.

No foreign runtime code, generated game code, binary or assets were integrated.
Downloaded authored host files remain private under reports/jxnplays-*.

## Findings useful to cod3pc

| Finding | Source evidence | Adaptation / acceptance |
|---|---|---|
| Level probes collect archive I/O, draws, triangles and runtime counters | scripts/probe_level.ps1 and docs/GAPS.md distinguish loading/rendering from full campaign completion | Add opt-in probes for declared/built modules only; preserve isolated profiles and one-game rule; report entry, rendering and control as separate evidence |
| Mouse problems include saturation, focus reacquisition, accumulated deltas and repeated input reads per frame | docs/MOUSE.md, CoD3Host/input.cpp, logic.h; the notes themselves still describe unresolved feel issues | Test Alt-Tab/settings return, no post-flick motion after stopping, consistent delta consumption, ADS and frame-rate behavior; do not transfer gain constants between runtimes |
| Settings use shared production logic in tests; prior duplicated predicates concealed bugs | logic.h and scripts/selfcheck.ps1 exercise the actual relaunch predicate and sensitivity calculation | Retain production-function tests for handoff; future settings need every edited field in dirty/apply/save detection and consistent video-mode reports |
| Optional brightness/contrast/temperature table plus shader saturation | grading.h and render_d3d11.cpp implement the table/present path | M8 optional image controls; identity at defaults, correct UNORM precision and order relative to original display gamma; keep original image by default |
| Ultrawide, FOV and sharpening are partially implemented | docs/ROADMAP.md marks ultrawide default-off and visually unverified, viewmodel scaling unfinished | Later feature work: world projection, HUD safe area, menus, movies and viewmodel must agree; no claim of ready full ultrawide support |
| Vehicle-specific missing world rendering is documented | docs/GAPS.md: Blkbrn damage can leave HUD drawing while world disappears | Add a future vehicle/damage regression scene when that module is integrated; this is their reported defect, not a confirmed cod3pc defect |

## What does not fix our current blockers

[kernel_xam.cpp](https://github.com/jxnplays/cod3-recomp/blob/ab73d51beba4c0a5e3cf2526f592135f4f91c136/CoD3Host/kernel_xam.cpp#L796)
still uses CreateProcessW followed by ExitProcess for title handoff. It now
accepts other bootable XEX targets and forwards the requested executable to
the successor. That is not in-process teardown. Its broader multiplayer
dispatch is outside cod3pc's current single-player scope and should not widen
our verified same-title predicate.

The D3D11 renderer has a different lifecycle and data path from our pinned
official Xenos plugin. The inspected renderer/pipeline files and gap notes
provide no identified fix for our lantern-glow-through-walls scene, no measured
repair of our spatial audio issue, and no solution to our three surviving
ReXGlue job workers. Do not copy renderer state or addresses as if compatible.

## Claims that remain plans or require qualification

[Their roadmap](https://github.com/jxnplays/cod3-recomp/blob/ab73d51beba4c0a5e3cf2526f592135f4f91c136/docs/ROADMAP.md)
lists FSR/XeSS, SSAO/GTAO, soft shadows, SMAA and DX12/Vulkan as not started.
Its sharpening is explicitly not AMD CAS; it uses its own coefficients and
replaces FXAA rather than following it. All-level probe results establish
per-level loading/rendering, not start-to-finish campaign completion. No
external executable was run or performance number reproduced in this review.

The README's rldicr/test discrepancy is an external report, not proof that
ReXGlue has that defect. Any ISA change requires our own instruction evidence
and tests; do not weaken correctness to reproduce another title's apparent
behavior. The code/notes also correct stale earlier STATUS claims, so current
source and runtime evidence outrank an inherited TODO list.

## License and integration boundary

[LICENSE](https://github.com/jxnplays/cod3-recomp/blob/ab73d51beba4c0a5e3cf2526f592135f4f91c136/LICENSE)
declares inherited runtime/config/scripts under BSD-3-Clause and this project's
own additions/docs under MIT. GitHub's generic NOASSERTION label does not
describe that split. Actual reuse needs attribution/notices for the applicable
portion and any dependencies. Original/derived game code is not covered by
those authored-code licenses. This review transfers ideas and test cases only.

## Priority

Keep same-process lifecycle first. Next useful low-risk infrastructure is
repeatable per-level evidence and input regressions. Image controls and
ultrawide remain later M8 features, not substitutes for rendering correctness,
campaign completion or the original presentation by default.
