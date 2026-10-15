#include "thread_exit_probe.h"
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <memory>
#include <mutex>

namespace {
std::mutex probe_mutex;
std::unique_ptr<Cod3ThreadExitProbe> probe;
bool incomplete = false;
}

void CaptureCod3TitleThreads() {
  // Called just before the original LaunchTitle import. No thread is suspended,
  // terminated or removed; duplicate handles remain valid after SDK cleanup.
  auto snapshot = std::make_unique<Cod3ThreadExitProbe>();
  bool failed = false;
  const auto objects = rex::system::kernel_state()->object_table()->GetAllObjects();
  for (const auto& object : objects) {
    if (object->type() != rex::system::XObject::Type::Thread) continue;
    auto* thread = static_cast<rex::system::XThread*>(object.get());
    if (!thread->is_guest_thread() || !thread->thread()) continue;
    try {
      snapshot->Add(thread->thread()->native_handle(), thread->thread_id(), thread->thread()->system_id());
    } catch (const std::exception& error) {
      failed = true;
      REXLOG_WARN("COD3_TITLE_THREADS_CAPTURE_FAILED guest={} {}", thread->thread_id(), error.what());
    }
  }
  REXLOG_INFO("COD3_TITLE_THREADS_CAPTURED count={} incomplete={}", snapshot->size(), failed);
  std::lock_guard lock(probe_mutex);
  probe = std::move(snapshot);
  incomplete = failed;
}

void ReportCod3TitleThreads() {
  std::unique_ptr<Cod3ThreadExitProbe> snapshot;
  bool failed = false;
  {
    std::lock_guard lock(probe_mutex);
    snapshot = std::move(probe);
    failed = incomplete;
  }
  if (!snapshot) return; // An ordinary quit may not request LaunchTitle.
  const auto start = std::chrono::steady_clock::now();
  const auto pending = snapshot->Wait(std::chrono::milliseconds(1000));
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start).count();
  REXLOG_INFO("COD3_TITLE_THREADS_OBSERVED captured={} pending={} incomplete={} elapsed_ms={}",
      snapshot->size(), pending.size(), failed, elapsed);
  for (const auto& item : pending)
    REXLOG_WARN("COD3_TITLE_THREAD_PENDING guest={} native={} wait_failed={}",
        item.guest_id, item.native_id, item.wait_failed);
  // This is evidence, not a permission to free memory: threads created after
  // the snapshot and GPU/audio host workers are outside this observation.
}
