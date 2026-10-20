# Lantern glow visibility investigation

## 2026-10-20: original GetData probe

One normal-menu launch, PID 16764, isolated checkpoint profile
nightd_glow_query_01, DoF 35, accepted shadow correction, query trace enabled.
Private log: logs/native-20261020-124934-464.log. User reports completing the
hidden/direct lamp probe; no image labels/time markers distinguish each view.

384 bounded records (three batches of 128): all caller LR 0x82418264,
query 0x40389C70, type 9, four-byte output, flags 0x84, one context.
192 not-ready returns (status 1, result 1); 192 ready returns (status 0), of
which one initial result is 0 and 191 are 1000. Later snapshots consistently
show end ZPass [1000,0], begin [0,0]. This agrees with the SDK fallback but
does not identify this query as the lantern sprite's visibility source.
The stored completed counter is 0 in these records; the original function
still alternates ready/not-ready, so that single field is not a sufficient
model of its complete readiness test.

Ghidra on our original SP XEX SHA256
E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682:
caller is inside 0x824181D8. At 0x82418260 it calls 0x822ED590; on success
it stores the integer at object +0x0C, converts/scales/clamps into +0x08,
and publishes a float at 0x8290A988. Need references to that value before
claiming it drives local lantern glows. No forced query results added.

## Resolve buffer lead

SirCarls patch notes identify 0x82420F48 as a CPU reader of a tiled resolve
texel. Our Ghidra and existing static output confirm the function and tiled
offset call 0x820C18E0. Instructions 0x82421070 onward derive a pixel address,
read a 32-bit word and unpack a floating depth-like value. Ghidra decompilation
truncates at a VMX instruction at 0x824210E0; do not treat its incomplete C
output as a complete function or a game crash. Existing codegen covers it.
Private evidence: logs/ghidra-glow-resolve-candidate.log and
logs/ghidra-glow-query-caller.log. No boundaries or generated code changed.

Pinned local SDK src/graphics/command_processor.cpp defines readback_resolve
as hot-reloadable, default none; full performs immediate synchronized readback.
Requested an in-window F4 comparison using full, hidden versus direct lamp,
without saving config. Await visual result; this is not yet a correction.
If it changes the defect, identify the actual buffer and caller, then measure
the minimal readback scope/cost. If unchanged, trace original lamp visibility
and data before importing any external query/readback patch stack.

## 2026-10-20: positive readback comparison and remaining fade

In the same live instance the user confirmed that both full and fast remove
the persistent hidden glow; with fast the directly visible glow remains.
On crossing a corner it can remain visible about half a second before fading;
full behaves similarly and the visible glow also appears with a delay.
Thus immediate readback does not resolve the reported transition latency.
These are qualitative observations, not timed captures or performance results.

The existing generated boundary 0x8245C208 calls the depth reader at
0x8245CACC (return LR 0x8245CAD0). Original instructions at 0x8245CAE0 and
0x8245CAEC subtract/add f17 from a per-record float at +0x24, then clamp the
value. f17 is loaded from 0x82001930, whose original word 0x3DCCCCCD is 0.1.
This confirms a gradual visibility update in the original rendering path;
it does not yet establish the wall-clock duration or prove that all latency
comes from that update. Ghidra has no containing function at 0x8245CAD0, so
its decompiler did not produce that caller; raw instruction evidence and
existing strict-generated code, not an invented function, support the finding.
Private evidence: logs/ghidra-glow-visibility-update.log and
logs/ghidra-glow-fade-instructions.log. No original instructions were patched.

Run-Native now explicitly selects fast readback by default. This restores the
data needed by the original depth visibility logic using existing SDK code;
no external implementation was copied. -ResolveReadback none restores the
previous comparison mode, -ResolveReadback full selects synchronous resolve
readback only, and the existing -FullGpuReadback also synchronizes memexport.
Conflicting explicit modes are rejected. some remains diagnostic only and
has not been visually accepted. Mode is included in dry-run and run metadata.

Validation: six PowerShell dry-run combinations and one conflict rejection
passed, with exactly one resolve argument each; git diff --check passed.
Only launcher policy/documentation changed: no SDK/native/codegen rebuild or
process restart was performed. The current game remains open. A fresh normal
launch and scene/transition regression with the new default are still pending;
the user-visible fade and whole-resolve copy performance need further checking.
