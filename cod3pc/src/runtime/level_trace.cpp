#include <rex/hook.h>
#include <rex/logging.h>
#include <Windows.h>

// Diagnostic wrappers only: forward the original context without repairing it.
// Addresses come from the native crash stack and generated Saint-Lo instructions.
#define TRACE_LEVEL_FUNCTION(name) \
  REX_EXTERN(__imp__##name); \
  REX_HOOK_RAW(name) { \
    const auto sp = ctx.r1.u32; \
    const auto r30 = ctx.r30.u64; \
    const auto r31 = ctx.r31.u64; \
    REXCPU_INFO("LEVEL_ENTER " #name " sp={:08X} r30={:016X} r31={:016X}", sp, r30, r31); \
    __imp__##name(ctx, base); \
    REXCPU_INFO("LEVEL_RETURN " #name " sp={:08X} r30={:016X} r31={:016X}", ctx.r1.u32, ctx.r30.u64, ctx.r31.u64); \
    if (ctx.r1.u32 != sp || ctx.r30.u64 != r30 || ctx.r31.u64 != r31) \
      REXCPU_WARN("LEVEL_CONTEXT_CHANGED " #name " sp {:08X}->{:08X} r30 {:016X}->{:016X} r31 {:016X}->{:016X}", \
                  sp, ctx.r1.u32, r30, ctx.r30.u64, r31, ctx.r31.u64); \
  }

TRACE_LEVEL_FUNCTION(sub_89132580)
TRACE_LEVEL_FUNCTION(sub_891BE760)
TRACE_LEVEL_FUNCTION(sub_891A1B88)
TRACE_LEVEL_FUNCTION(sub_89191FD0)
