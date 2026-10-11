#include <rex/hook.h>
#include <rex/logging.h>

// Verified entry point for default.xex SHA-256 2944eec7...cee23692.
// Logging only: forward the complete PPC context to the original implementation.
REX_EXTERN(__imp__xstart);
REX_HOOK_RAW(xstart) {
  REXCPU_INFO("COD3_ORIGINAL_ENTRY_ENTER guest=0x82344D00");
  __imp__xstart(ctx, base);
  REXCPU_INFO("COD3_ORIGINAL_ENTRY_RETURN guest=0x82344D00");
}
