#include <rex/hook.h>
#include <rex/cvar.h>
#include <atomic>
#include <cstring>

REXCVAR_DEFINE_BOOL(cod3_skip_loading_movies, true, "Game",
    "Allow Space or Enter to skip a briefing after its asset-loading wait");

REX_EXTERN(__imp__sub_824EEBA0);
REX_EXTERN(__imp__sub_820E90F8);

namespace {
enum class SkipState { Inactive, Ready, Requested };
std::atomic<SkipState> skip_state{SkipState::Inactive};
thread_local bool loading_movie = false;

uint32_t Load32(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, sizeof(value));
  return __builtin_bswap32(value);
}
}

void ResetCod3MovieSkip() {
  skip_state.store(SkipState::Inactive, std::memory_order_release);
}

bool RequestCod3MovieSkip() {
  if (!REXCVAR_GET(cod3_skip_loading_movies)) return false;
  auto expected = SkipState::Ready;
  return skip_state.compare_exchange_strong(expected, SkipState::Requested);
}

REX_HOOK_RAW(sub_824EEBA0) {
  // Reviewed E057 XEX: 82518080 waits via 82463988 until manager+4 != 0;
  // 82518084 then calls this original remaining-movie loop. Loading callbacks
  // earlier in that wait use 824EECF8, so they never open this skip window.
  const auto manager = Load32(base, 0x82A2AB6C);
  const bool ready = REXCVAR_GET(cod3_skip_loading_movies) &&
      static_cast<uint32_t>(ctx.lr) == 0x82518088 && manager &&
      base[manager + 4] != 0 && base[manager + 5] == 0 &&
      Load32(base, 0x82A2A24C) != 0;
  if (!ready) {
    __imp__sub_824EEBA0(ctx, base);
    return;
  }
  struct Scope {
    Scope() {
      loading_movie = true;
      skip_state.store(SkipState::Ready, std::memory_order_release);
    }
    ~Scope() {
      skip_state.store(SkipState::Inactive, std::memory_order_release);
      loading_movie = false;
    }
  } scope;
  // This is the original minimum-watch timer, not a load-complete flag.
  // The original loop resets elapsed time; remove its extra mandatory wait
  // only after the asset wait above. Original movie cleanup resets it to zero.
  const uint32_t zero = 0;
  std::memcpy(base + 0x82A2A258, &zero, sizeof(zero));
  REXLOG_INFO("COD3_MOVIE_SKIP_READY asset_wait_complete=true keys=Space,Enter");
  __imp__sub_824EEBA0(ctx, base);
  REXLOG_INFO("COD3_MOVIE_SKIP_WINDOW_CLOSED");
}

REX_HOOK_RAW(sub_820E90F8) {
  const auto caller = static_cast<uint32_t>(ctx.lr);
  __imp__sub_820E90F8(ctx, base);
  // Add the PC key request only to the original movie input predicate.
  // Its caller then clears input, marks skip and runs original video/audio
  // cleanup. All other controller polling keeps the original result.
  if (!loading_movie || caller != 0x824D04D8) return;
  auto expected = SkipState::Requested;
  if (skip_state.compare_exchange_strong(expected, SkipState::Inactive)) {
    ctx.r3.u64 = 1;
    REXLOG_INFO("COD3_MOVIE_SKIP_ACCEPTED original_input_path=true");
  }
}
