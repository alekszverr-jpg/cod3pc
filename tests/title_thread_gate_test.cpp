#include <rex/system/title_thread_gate.h>
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  using namespace std::chrono_literals;
  rex::system::TitleThreadGate gate;
  std::promise<void> release;
  auto released = release.get_future().share();
  std::atomic<unsigned> entered{0}, violations{0};
  std::vector<std::thread> creators;
  for (unsigned i = 0; i < 8; ++i) {
    creators.emplace_back([&] {
      if (!gate.TryEnter()) { ++violations; return; }
      ++entered;
      released.wait();  // Native creation admitted before seal is in flight.
      gate.Leave();
    });
  }
  const auto deadline = std::chrono::steady_clock::now() + 2s;
  while (entered != 8 && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
  const bool all_entered = entered == 8;
  gate.Seal();
  const bool incorrectly_idle = gate.WaitForIdle(10ms);
  std::vector<std::thread> rejected;
  for (unsigned i = 0; i < 8; ++i) {
    rejected.emplace_back([&] {
      for (unsigned n = 0; n < 1000; ++n) {
        if (gate.TryEnter()) { ++violations; gate.Leave(); }
      }
    });
  }
  for (auto& thread : rejected) thread.join();
  release.set_value();
  const bool drained = gate.WaitForIdle(2s);
  for (auto& thread : creators) thread.join();
  gate.Seal(); // Idempotent and permanent.
  const bool reopened = gate.TryEnter();
  if (reopened) gate.Leave();
  if (!all_entered || incorrectly_idle || !drained || violations || reopened) {
    std::cerr << "Creation seal failed: entered=" << entered << " violations=" << violations << '\n';
    return 1;
  }
  // A fresh Runtime's gate is independent of the terminated title.
  rex::system::TitleThreadGate next_title;
  if (!next_title.TryEnter()) return 2;
  next_title.Leave();
  return 0;
}
