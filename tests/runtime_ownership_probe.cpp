// Diagnostic SDK ownership probe, not a game/backend.
// Exit 2 reports a confirmed native-exit/object-lifetime mismatch.
#include <rex/runtime.h>
#include <rex/system/xthread.h>
#include <rex/logging.h>
#include <rex/thread.h>
#include <rex/thread/fiber.h>
#include <Windows.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include "../cod3pc/src/runtime/fiber_task.h"

using rex::X_STATUS;

namespace {
std::atomic<unsigned> destroyed{0};
std::atomic<bool> body_returned{false};
std::atomic<bool> allow_return{false};
std::atomic<rex::thread::Fiber*> worker_fiber{nullptr};
thread_local std::unique_ptr<Cod3FiberTask> suspended_task;
class ObservedThread final : public rex::system::XHostThread {
 public:
  explicit ObservedThread(rex::system::KernelState* kernel)
      : XHostThread(kernel, 128 * 1024, 0, [] {
          worker_fiber = rex::thread::Fiber::ConvertCurrentThread();
          while (!allow_return.load(std::memory_order_acquire)) std::this_thread::yield();
          body_returned = true;
          return 0;
        }) {}
  ~ObservedThread() override { ++destroyed; }
};

[[noreturn]] void Finish(int code) {
  std::fflush(stdout);
  rex::FlushLogging();
  // Keep the diagnostic Runtime alive if ownership is unresolved. No forced
  // release of unknown references and no unsafe memory teardown after failure.
  std::_Exit(code);
}

void CheckStartRoutineOwnership(unsigned mode) {
  std::printf("START_ROUTINE_PROBE mode=%u starting\n", mode);
  std::fflush(stdout);
  // A capture must survive while its native callback runs, and be released on
  // return or raw native Exit, including Exit from a different Windows fiber.
  auto capture = std::make_shared<int>(42);
  std::weak_ptr<int> observer = capture;
  auto ready = rex::thread::Event::CreateManualResetEvent(false);
  auto release = rex::thread::Event::CreateManualResetEvent(false);
  void* secondary = nullptr;
  auto worker = rex::thread::Thread::Create({}, [capture, &ready, &release, &secondary, mode] {
    ready->Set();
    if (rex::thread::Wait(release.get(), false, std::chrono::seconds(3)) !=
        rex::thread::WaitResult::kSuccess) Finish(30);
    if (*capture != 42) Finish(31);
    if (mode == 1) rex::thread::Thread::Exit(0);
    if (mode == 2) {
      if (!ConvertThreadToFiber(nullptr)) Finish(32);
      secondary = CreateFiber(128 * 1024, [](void*) { rex::thread::Thread::Exit(0); }, nullptr);
      if (!secondary) Finish(33);
      SwitchToFiber(secondary);
      Finish(34);
    }
  });
  if (!worker) Finish(35);
  capture.reset();
  if (rex::thread::Wait(ready.get(), false, std::chrono::seconds(3)) !=
      rex::thread::WaitResult::kSuccess || observer.expired()) Finish(36);
  release->Set();
  if (rex::thread::Wait(worker.get(), false, std::chrono::seconds(3)) !=
      rex::thread::WaitResult::kSuccess) Finish(37);
  std::printf("START_ROUTINE_PROBE mode=%u native_wait_complete=true\n", mode);
  std::fflush(stdout);
  // Do not DeleteFiber on the exited thread's selected fiber: Windows has
  // already retired that execution context. This probe checks callback lifetime,
  // not reclamation of every fiber allocated by a title.
  worker.reset();
  std::printf("START_ROUTINE_PROBE mode=%u native_exited=true capture_released=%s\n",
              mode, observer.expired() ? "true" : "false");
  if (!observer.expired()) Finish(38);
}

void CheckSuspendedTaskExit() {
  auto ready = rex::thread::Event::CreateManualResetEvent(false);
  auto release = rex::thread::Event::CreateManualResetEvent(false);
  if (Cod3FiberTask::LiveCount()) Finish(40);
  auto worker = rex::thread::Thread::Create({}, [&ready, &release] {
    suspended_task = std::make_unique<Cod3FiberTask>([](void*) { suspended_task->Suspend(); }, nullptr);
    if (suspended_task->Resume()) Finish(41);
    ready->Set();
    if (rex::thread::Wait(release.get(), false, std::chrono::seconds(3)) !=
        rex::thread::WaitResult::kSuccess) Finish(42);
    rex::thread::Thread::Exit(0);
  });
  if (!worker || rex::thread::Wait(ready.get(), false, std::chrono::seconds(3)) !=
      rex::thread::WaitResult::kSuccess) Finish(43);
  if (Cod3FiberTask::LiveCount() != 1) Finish(44);
  release->Set();
  if (rex::thread::Wait(worker.get(), false, std::chrono::seconds(3)) !=
      rex::thread::WaitResult::kSuccess) Finish(45);
  const auto remaining = Cod3FiberTask::LiveCount();
  std::printf("SCRIPT_TLS_PROBE suspended_before_exit=1 native_exited=true fibers_remaining=%zu\n", remaining);
  if (remaining) Finish(46);
}
}

int main() {
  rex::InitLogging();
  for (unsigned mode = 0; mode != 3; ++mode) CheckStartRoutineOwnership(mode);
  CheckSuspendedTaskExit();
  for (unsigned cycle = 0; cycle != 3; ++cycle) {
    destroyed = 0;
    body_returned = false;
    allow_return = false;
    auto runtime = std::make_unique<rex::Runtime>(std::filesystem::path{});
    rex::RuntimeConfig config;
    config.tool_mode = true;
    if (runtime->Setup(std::move(config)) != X_STATUS_SUCCESS) Finish(10);

    // First prove our table-handle/caller-reference release procedure destroys
    // a never-started object while its real KernelState and memory remain live.
    {
      rex::system::object_ref<ObservedThread> control(new ObservedThread(runtime->kernel_state()));
      if (control->Delete() != X_STATUS_SUCCESS) Finish(11);
    }
    if (destroyed != 1) Finish(12);
    destroyed = 0;

    auto* reaper_fiber = rex::thread::Fiber::ConvertCurrentThread();
    if (!reaper_fiber) Finish(16);
    const auto reaper_handle = GetCurrentFiber();

    rex::system::object_ref<ObservedThread> worker(new ObservedThread(runtime->kernel_state()));
    if (worker->Create() != X_STATUS_SUCCESS) Finish(13);
    const auto ready_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!worker_fiber && std::chrono::steady_clock::now() < ready_deadline)
      std::this_thread::yield();
    if (!worker_fiber) Finish(20);
    // Transfer a separately owned reference while the native thread is still
    // blocked. The reaper must retain it until actual exit, regardless of any
    // guest signal or host object's running flag.
    runtime->kernel_state()->RetainThreadThroughNativeExit(rex::system::retain_object(worker.get()));
    if (runtime->kernel_state()->ReapExitedThreadObjects() != 0 ||
        runtime->kernel_state()->PendingExitedThreadObjects() != 1 || destroyed != 0) Finish(21);
    allow_return.store(true, std::memory_order_release);
    const bool exited = rex::thread::Wait(worker->thread(), false,
        std::chrono::milliseconds(3000)) == rex::thread::WaitResult::kSuccess;
    if (!exited) Finish(14);
    auto* retired_fiber = worker_fiber.exchange(nullptr);
    if (!retired_fiber) Finish(17);
    retired_fiber->DestroyAfterThreadExit();
    if (!IsThreadAFiber() || GetCurrentFiber() != reaper_handle) Finish(18);
    // Exit has released the execution handle reference. Release the original
    // table handle, then the probe's only caller-owned object reference.
    if (worker->Delete() != X_STATUS_SUCCESS) Finish(15);
    worker.reset();
    const auto reaped = runtime->kernel_state()->ReapExitedThreadObjects();
    std::printf("OWNERSHIP_PROBE native_exited=%s body_returned=%s object_destroyed=%u\n",
                exited ? "true" : "false", body_returned ? "true" : "false",
                destroyed.load());
    std::printf("OWNERSHIP_PROBE reaped=%zu pending=%zu\n", reaped,
                runtime->kernel_state()->PendingExitedThreadObjects());
    reaper_fiber->Destroy();
    if (!body_returned || destroyed != 1 || reaped != 2 ||
        runtime->kernel_state()->PendingExitedThreadObjects() != 0) Finish(2);
    runtime.reset();
    if (rex::Runtime::instance() || rex::system::KernelState::shared()) Finish(19);
    std::printf("OWNERSHIP_PROBE cycle=%u runtime_destroyed=true globals_cleared=true\n", cycle);
  }
  Finish(0);
}
