#include "title_handoff.h"
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/kernel/xam/module.h>
#include <rex/hook.h>
#include <atomic>

REXCVAR_DEFINE_BOOL(cod3_worker_cooperative_exit, false, "Debug",
    "Experimental same-title worker exit with bounded idle waits").debug_only();
REXCVAR_DEFINE_BOOL(cod3_title_thread_drain, false, "Debug",
    "Seal guest creation and retain threads through native exit at title reboot").debug_only();
namespace { std::atomic<bool> title_exit_requested{false}; }

REX_EXTERN(__imp__sub_82343880);

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
  if (REXCVAR_GET(cod3_title_thread_drain)) {
    rex::system::kernel_state()->SealTitleThreadCreation();
    REXLOG_INFO("COD3_TITLE_THREAD_CREATION_SEALED");
  }
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

REX_HOOK_RAW(sub_82343880) {
  auto* thread = rex::system::XThread::IsInThread()
      ? rex::system::XThread::GetCurrentThread() : nullptr;
  // Ghidra 2026-10-16, installed E057 image: 8212A5FC calls this
  // non-alertable wait after releasing the queue mutant, with r4=-1.
  // Restrict the experiment to that exact worker idle call. Every other
  // timeout/caller/thread retains the original tail-call wrapper unchanged.
  const bool idle = REXCVAR_GET(cod3_worker_cooperative_exit) && thread &&
      thread->is_guest_thread() && !thread->main_thread() &&
      thread->creation_params()->start_address == 0x8212A780 &&
      ctx.lr == 0x8212A600 && ctx.r4.u32 == UINT32_MAX;
  if (!idle) {
    __imp__sub_82343880(ctx, base);
    return;
  }
  thread_local bool logged = false;
  if (!logged) {
    REXLOG_INFO("COD3_WORKER_IDLE_WAIT guest={} native={} handle={:08X} quantum_ms=50",
        thread->thread_id(), thread->thread()->system_id(), ctx.r3.u32);
    logged = true;
  }
  const PPCContext entry = ctx;
  for (;;) {
    // Outside the original kernel wait: its local object refs/locks have
    // unwound before cooperative self-exit. Never kill another thread.
    Cod3StopWorkerAtIdle();
    ctx = entry;
    ctx.r4.u64 = 50;
    __imp__sub_82343880(ctx, base);
    // Retry only a real timeout. A signal, error or abandonment is returned
    // through the original wrapper and its original PPC epilogue/registers.
    if (ctx.r3.u32 != 0x102) return;
  }
}
