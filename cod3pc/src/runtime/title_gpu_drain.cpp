#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <rex/logging.h>
#include <rex/system/interfaces/graphics.h>

void DumpCod3TitleConsumers();

bool DrainCod3TitleGpu(rex::system::IGraphicsSystem* graphics) {
  // Query the already-loaded corrected plugin. Never load another backend or
  // change the pinned IGraphicsSystem ABI for this optional experiment.
  const auto plugin = GetModuleHandleW(L"rexgpu-xenos-grassrd.dll");
  using Drain = bool (*)(rex::system::IGraphicsSystem*, uint32_t);
  const auto drain = plugin ? reinterpret_cast<Drain>(
      GetProcAddress(plugin, "cod3_gpu_quiesce_title_v1")) : nullptr;
  if (!drain || !graphics) {
    REXLOG_ERROR("COD3_TITLE_GPU_DRAIN complete=false reason=capability_unavailable");
    return false;
  }
  const bool complete = drain(graphics, 5000);
  REXLOG_INFO("COD3_TITLE_GPU_DRAIN complete={}", complete);
  if (!complete) DumpCod3TitleConsumers();
  return complete;
}
