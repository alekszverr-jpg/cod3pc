#include "fiber_task.h"
#include <Windows.h>
#include <stdexcept>

Cod3FiberTask::Cod3FiberTask(Body body, void* argument)
    : body_(body), argument_(argument), thread_(GetCurrentThreadId()) {
  fiber_ = CreateFiberEx(64 * 1024, 4 * 1024 * 1024, FIBER_FLAG_FLOAT_SWITCH, Entry, this);
  if (!fiber_) throw std::runtime_error("CreateFiberEx failed for script task");
}
Cod3FiberTask::~Cod3FiberTask() { if (fiber_) DeleteFiber(fiber_); }
bool Cod3FiberTask::Resume() {
  if (GetCurrentThreadId() != thread_) throw std::runtime_error("Script fiber changed OS thread");
  if (finished_) return true;
  if (!IsThreadAFiber() && !ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH))
    throw std::runtime_error("ConvertThreadToFiberEx failed for script scheduler");
  parent_ = GetCurrentFiber();
  SwitchToFiber(fiber_);
  return finished_;
}
void Cod3FiberTask::Suspend() {
  if (GetCurrentThreadId() != thread_ || GetCurrentFiber() != fiber_ || !parent_)
    throw std::runtime_error("Yield outside the active script fiber");
  SwitchToFiber(parent_);
}
void __stdcall Cod3FiberTask::Entry(void* argument) {
  auto* task = static_cast<Cod3FiberTask*>(argument);
  task->body_(task->argument_);
  task->finished_ = true;
  SwitchToFiber(task->parent_);
  std::terminate(); // A finished task must never be resumed.
}
