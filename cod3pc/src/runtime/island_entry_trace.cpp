#include <rex/hook.h>
#include <rex/logging.h>

// Ghidra: Island 89226748 is the original tail branch to 892265F0.
// Forward all state unchanged and record actual entry execution, not just load.
REX_EXTERN(__imp__xstart);
REX_HOOK_RAW(xstart) {
  REXCPU_INFO("COD3_ISLAND_ENTRY_ENTER guest=89226748 reason={}", ctx.r4.u32);
  __imp__xstart(ctx, base);
  REXCPU_INFO("COD3_ISLAND_ENTRY_RETURN result={:08X}", ctx.r3.u32);
}
