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
#include <thread>

using rex::X_STATUS;

namespace {
std::atomic<unsigned> destroyed{0};
std::atomic<bool> body_returned{false};
std::atomic<bool> allow_return{false};
std::atomic<rex::thread::Fiber*> worker_fiber{nullptr};
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
}

int main() {
  rex::InitLogging();
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
