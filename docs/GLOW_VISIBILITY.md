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
