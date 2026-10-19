#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <atomic>
#include <array>
#include <cstring>

REXCVAR_DEFINE_BOOL(cod3_gpu_query_trace, false, "Debug",
                   "Trace 128 original CoD3 GPU query reads; toggle off/on to rearm");
REX_EXTERN(__imp__sub_822ED590);

// Ghidra, original SP XEX SHA256 E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682:
// 822ED590 is the existing type-9 query GetData boundary. 822ED6B0 reads
// end ZPass at +16/+20 and subtracts begin at +48/+52. Diagnostics only:
// always execute the original body and preserve every register/result bit.
REX_HOOK_RAW(sub_822ED590) {
  static std::atomic<uint32_t> logged{0};
  if (!REXCVAR_GET(cod3_gpu_query_trace)) {
    logged.store(0, std::memory_order_relaxed);
    __imp__sub_822ED590(ctx, base);
    return;
  }
  const auto load32 = [base](uint32_t address) {
    uint32_t value;
    std::memcpy(&value, base + address, sizeof(value));
    return __builtin_bswap32(value);
  };
  const uint32_t query = ctx.r3.u32;
  if (load32(query + 4) != 9 || logged.fetch_add(1, std::memory_order_relaxed) >= 128) {
    __imp__sub_822ED590(ctx, base);
    return;
  }
  const uint32_t caller = uint32_t(ctx.lr);
  const uint32_t output = ctx.r4.u32;
  const uint32_t bytes = ctx.r5.u32;
  const uint32_t device = load32(query);
  const uint32_t query_fence = load32(query + 20);
  const uint32_t submitted = load32(device + 10780);
  const uint32_t completed_address = load32(device + 10768);
  const uint32_t completed = load32(completed_address);
  const uint32_t flags = uint32_t(base[device + 10809]);
  const uint32_t contexts = load32(query + 144);
  std::array<uint32_t, 4> snapshots{};
  if (contexts) {
    const uint32_t encoded = load32(query + 24);
    const uint32_t samples = (encoded & 0x1FFFFFFFu) +
        (((encoded >> 20) + 0x200u) & 0x1000u) + 0xC0000000u;
    // GPU counts are little endian; load32 observes PPC endian order.
    constexpr std::array<uint32_t, 4> offsets{16, 20, 48, 52};
    for (size_t i = 0; i < snapshots.size(); ++i) {
      snapshots[i] = __builtin_bswap32(load32(samples + offsets[i]));
    }
  }
  __imp__sub_822ED590(ctx, base);
  const uint32_t status = ctx.r3.u32;
  std::array<uint32_t, 4> results{};
  if (bytes >= 4) results[0] = load32(output);
  if (bytes >= 16) {
    for (size_t i = 1; i < results.size(); ++i) results[i] = load32(output + uint32_t(i) * 4);
  }
  REXCPU_INFO("COD3_GPU_QUERY query={:08X} caller={:08X} bytes={} flags={:02X} submitted={} completed={} query_fence={} contexts={} status={} result=[{},{},{},{}] end_pass=[{},{}] begin_pass=[{},{}]",
      query, caller, bytes, flags, submitted, completed, query_fence, contexts, status,
      results[0], results[1], results[2], results[3],
      snapshots[0], snapshots[1], snapshots[2], snapshots[3]);
}
