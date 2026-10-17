#include <rex/hook.h>
#include <rex/logging.h>

// Original nightd.dll entry 89201B08, verified with Ghidra on SHA-256
// AE2F69D2446FED315F298114BDDB3CBFC2CA79D631497F8254B974FAF79177DA.
// Forward the original entry with all PPC state and its result unchanged.
REX_EXTERN(__imp__xstart);
REX_HOOK_RAW(xstart) {
  REXCPU_INFO("COD3_NIGHTD_ENTRY_ENTER guest=89201B08 reason={}", ctx.r4.u32);
  __imp__xstart(ctx, base);
  REXCPU_INFO("COD3_NIGHTD_ENTRY_RETURN result={:08X}", ctx.r3.u32);
}
