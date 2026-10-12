#pragma once

// A host stack retained across a cooperative yield. Each task stays on its
// creating OS thread; PPC state and guest memory remain the caller's responsibility.
class Cod3FiberTask {
 public:
  using Body = void (*)(void*);
  Cod3FiberTask(Body body, void* argument);
  ~Cod3FiberTask();
  Cod3FiberTask(const Cod3FiberTask&) = delete;
  Cod3FiberTask& operator=(const Cod3FiberTask&) = delete;
  bool Resume();
  void Suspend();
  bool Finished() const { return finished_; }
 private:
  static void __stdcall Entry(void* argument);
  void* fiber_ = nullptr;
  void* parent_ = nullptr;
  Body body_;
  void* argument_;
  unsigned long thread_;
  bool finished_ = false;
};
