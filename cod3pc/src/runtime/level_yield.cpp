#include <Windows.h>
#include <rex/logging.h>

// Each level DLL resolves the host scheduler's existing continuation callback.
// The original guest wait has already updated its state before this runs.
void Cod3ReturnFromGuestYield() {
  using Restore = void (*)();
  static const auto restore = reinterpret_cast<Restore>(
      GetProcAddress(GetModuleHandleW(nullptr), "Cod3ResumeGuestYieldCore"));
  if (!restore) REX_FATAL("Host script continuation restore export is absent");
  restore();
}
