#include <rex/hook.h>
#include <rex/logging.h>
#include <string>

namespace {
std::string GuestLoaderPath(uint32_t address) {
  if (!address) return "<null>";
  // Same pointer about to be consumed by the original SDK loader. Bound the
  // diagnostic string; it never changes the loader's arguments or memory.
  const auto* path = rex::system::kernel_state()->memory()->TranslateVirtual<char*>(address);
  std::string result;
  for (size_t i = 0; i < 256 && path[i]; ++i) result.push_back(path[i]);
  return result;
}
}

void Cod3TraceLaunchTitle(PPCRegister& r3, PPCRegister& r4) {
  REXCPU_INFO("COD3_TRANSITION_LAUNCH_TITLE path='{}' flags={:08X}",
      GuestLoaderPath(r3.u32), r4.u32);
}
void Cod3TraceStartupTerminate() {
  REXCPU_INFO("COD3_TRANSITION_TERMINATE origin=startup_check");
}
void Cod3TraceMainTerminate() {
  REXCPU_INFO("COD3_TRANSITION_TERMINATE origin=main_return");
}
void Cod3TraceLoadImage(PPCRegister& r3, PPCRegister& r4) {
  REXCPU_INFO("COD3_TRANSITION_LOAD path='{}' flags={:08X}",
      GuestLoaderPath(r3.u32), r4.u32);
}
void Cod3TraceLoadImageResult(PPCRegister& r3) {
  REXCPU_INFO("COD3_TRANSITION_LOAD_RESULT status={:08X}", r3.u32);
}
void Cod3TraceUnloadImage(PPCRegister& r3) {
  REXCPU_INFO("COD3_TRANSITION_UNLOAD handle={:08X}", r3.u32);
}
