#include "title_handoff.h"
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/kernel/xam/module.h>
#include <atomic>

REXCVAR_DEFINE_BOOL(cod3_worker_cooperative_exit, false, "Debug",
    "Experimental same-title worker exit at verified idle boundaries").debug_only();
namespace { std::atomic<bool> title_exit_requested{false}; }

void ResetCod3WorkerExit() {
  title_exit_requested.store(false, std::memory_order_release);
}

void ArmCod3WorkerExit(const std::string& path, uint32_t flags) {
  if (!REXCVAR_GET(cod3_worker_cooperative_exit)) return;
  const auto xam = rex::system::kernel_state()->GetKernelModule<rex::kernel::xam::XamModule>("xam.xex");
  if (!xam) return;
  const auto& data = xam->loader_data();
  const auto resolved = path == "default.xex" ? "game:\\default.xex" : path;
  if (!cod3::IsCampaignRelaunch(resolved, flags, data.launch_data_present, data.launch_data.size())) return;
  title_exit_requested.store(true, std::memory_order_release);
  REXLOG_INFO("COD3_WORKER_EXIT_ARMED bytes={}", data.launch_data.size());
}

void Cod3StopWorkerAtIdle() {
  if (!title_exit_requested.load(std::memory_order_acquire) || !rex::system::XThread::IsInThread()) return;
  auto* thread = rex::system::XThread::GetCurrentThread();
  // Original job-worker entry, not main-thread calls into the same scheduler.
  if (!thread->is_guest_thread() || thread->main_thread() ||
      thread->creation_params()->start_address != 0x8212A780) return;
  REXLOG_INFO("COD3_WORKER_EXIT_IDLE guest={} native={}",
      thread->thread_id(), thread->thread()->system_id());
  // Runs on the worker itself after releasing the queue mutant / returning
  // from its idle wait. Preserve the original SDK's cooperative Exit semantics.
  // Never terminate a foreign host thread or touch guest state on a live title.
  thread->Exit(0);
}
