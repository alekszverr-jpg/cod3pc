#include <rex/hook.h>
#include <rex/logging.h>
#include <array>
#include <chrono>
#include <mutex>

// Read-only diagnostics at the original DoF setter's return. Keep register
// bits untouched, including infinities, and bound output during moving focus.
void Cod3TraceDofParameters(PPCRegister& r3, PPCRegister& f1,
                          PPCRegister& f2, PPCRegister& f3, PPCRegister& f4) {
  using Clock = std::chrono::steady_clock;
  static std::mutex mutex;
  static std::array<uint64_t, 5> previous{};
  static Clock::time_point last{};
  static bool initialized = false;
  const std::array<uint64_t, 5> values{r3.u32, f1.u64, f2.u64, f3.u64, f4.u64};
  const auto now = Clock::now();
  std::lock_guard lock(mutex);
  if (initialized && (values == previous || now - last < std::chrono::milliseconds(500))) return;
  previous = values;
  last = now;
  initialized = true;
  REXCPU_INFO("COD3_DOF_PARAMETERS mode={} values=[{},{},{},{}]",
      r3.u32, f1.f64, f2.f64, f3.f64, f4.f64);
}
