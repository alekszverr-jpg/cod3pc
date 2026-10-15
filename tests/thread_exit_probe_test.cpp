#include "../cod3pc/src/runtime/thread_exit_probe.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#include <stdexcept>

void Require(bool condition) { if (!condition) throw std::runtime_error("thread exit observation failed"); }
DWORD WINAPI Worker(void* event) {
  WaitForSingleObject(event, 3000);
  return 0;
}
int main() {
  try {
    Cod3ThreadExitProbe empty;
    Require(empty.Wait(std::chrono::milliseconds(0)).empty());
    const auto event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    Require(event != nullptr);
    DWORD native_id = 0;
    const auto worker = CreateThread(nullptr, 0, Worker, event, 0, &native_id);
    Require(worker != nullptr);
    Cod3ThreadExitProbe probe;
    probe.Add(worker, 42, native_id);
    // The observer owns a real duplicate, independent of original SDK handle
    // lifetime or a guest's earlier "terminated" signal.
    CloseHandle(worker);
    auto pending = probe.Wait(std::chrono::milliseconds(15));
    Require(pending.size() == 1 && pending[0].guest_id == 42 &&
            pending[0].native_id == native_id && !pending[0].wait_failed);
    Require(SetEvent(event));
    Require(probe.Wait(std::chrono::milliseconds(2000)).empty());
    CloseHandle(event);
    bool rejected = false;
    try { probe.Add(nullptr, 99, 0); } catch (...) { rejected = true; }
    Require(rejected && probe.size() == 1);
    std::puts("Thread exit observed after actual Windows thread completion");
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
