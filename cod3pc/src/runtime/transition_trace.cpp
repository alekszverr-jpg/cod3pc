#include <rex/hook.h>
#include <rex/logging.h>
#include <string>
#include <vector>
#include <rex/kernel/xam/module.h>

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

void Cod3TraceLoaderState() {
  const auto xam = rex::system::kernel_state()->GetKernelModule<rex::kernel::xam::XamModule>("xam.xex");
  if (!xam) {
    REXLOG_WARN("COD3_TRANSITION_STATE xam=unavailable");
    return;
  }
  const auto& data = xam->loader_data();
  REXLOG_INFO("COD3_TRANSITION_STATE present={} bytes={} path='{}' flags={:08X}",
      data.launch_data_present, data.launch_data.size(), data.launch_path, data.launch_flags);
}

void Cod3TraceSetLaunchData(PPCRegister& r3, PPCRegister& r4) {
  REXCPU_INFO("COD3_TRANSITION_SET_DATA ptr={:08X} bytes={}", r3.u32, r4.u32);
}
void Cod3TraceSetLaunchDataResult(PPCRegister& r3) {
  REXCPU_INFO("COD3_TRANSITION_SET_DATA_RESULT status={:08X}", r3.u32);
  Cod3TraceLoaderState();
}
void Cod3TraceGetLaunchSizeResult(PPCRegister& r3) {
  REXCPU_INFO("COD3_TRANSITION_GET_SIZE_RESULT status={:08X}", r3.u32);
  Cod3TraceLoaderState();
}
void Cod3TraceGetLaunchData(PPCRegister& r4) {
  REXCPU_INFO("COD3_TRANSITION_GET_DATA capacity={}", r4.u32);
}
void Cod3TraceGetLaunchDataResult(PPCRegister& r3) {
  REXCPU_INFO("COD3_TRANSITION_GET_DATA_RESULT status={:08X}", r3.u32);
}

void Cod3TraceLaunchTitle(PPCRegister& r3, PPCRegister& r4) {
  REXCPU_INFO("COD3_TRANSITION_LAUNCH_TITLE path='{}' flags={:08X}",
      GuestLoaderPath(r3.u32), r4.u32);
  Cod3TraceLoaderState();
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
