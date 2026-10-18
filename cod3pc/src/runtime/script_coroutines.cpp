#include "fiber_task.h"
#include <Windows.h>
#include <array>
#include <atomic>
#include <rex/hook.h>
#include <rex/logging.h>
#include <memory>
#include <unordered_map>

REX_EXTERN(__imp__sub_824A6150);
REX_EXTERN(__imp__sub_824A53D8);
REX_EXTERN(__imp__sub_824A56B8);
REX_EXTERN(__imp__sub_824A55E8);
REX_EXTERN(__imp__sub_824AB570);
REX_EXTERN(__imp__sub_824A5D70);
REX_EXTERN(sub_823C0B98);

namespace {
std::atomic<std::size_t> live_scripts{0};
struct Invocation { PPCContext* engine; uint8_t* base; uint32_t key; };
struct Script {
  Script() { ++live_scripts; }
  ~Script() { --live_scripts; }
  PPCContext context{};
  PPCContext yield_context{};
  uint8_t* base = nullptr;
  uint32_t key = 0;
  uint32_t yield_frame = 0;
  uint32_t resumes = 0;
  uint32_t object = 0;
  uint32_t initial_flags = 0;
  std::unique_ptr<Cod3FiberTask> task;
};
thread_local Invocation* invocation = nullptr;
thread_local Script* active_script = nullptr;
struct ScriptStore {
  std::unordered_map<uint32_t, std::unique_ptr<Script>> entries;
  ~ScriptStore() {
    // Teardown must happen on the scheduler's parent fiber. Deleting a selected
    // script fiber would terminate its native owner during TLS destruction.
    if (active_script) REX_FATAL("Script TLS retired on active script fiber: {:08X}", active_script->key);
    const auto retained = entries.size();
    entries.clear(); // Host fibers only; do not read freed guest stack/memory.
    REXCPU_INFO("SCRIPT_TLS_RETIRED native={} retained={} contexts_remaining={} fibers_remaining={}",
        GetCurrentThreadId(), retained, live_scripts.load(), Cod3FiberTask::LiveCount());
  }
};
thread_local ScriptStore scripts;

uint32_t Load32(uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, sizeof(value));
  return __builtin_bswap32(value);
}
void RunScript(void* argument) {
  auto* script = static_cast<Script*>(argument);
  sub_823C0B98(script->context, script->base);
}
bool RunFiber(Script& script) {
  auto* previous = active_script;
  active_script = &script;
  const bool finished = script.task->Resume();
  active_script = previous;
  return finished;
}
void PrepareYield(PPCContext& ctx, uint8_t* base, PPCFunc* original) {
  if (!active_script || &active_script->context != &ctx)
    REX_FATAL("Script suspend outside a registered coroutine");
  active_script->yield_context = ctx;
  active_script->yield_frame = ctx.r1.u32 - 400;
  original(ctx, base); // Keep the game's status changes and guest-stack copy.
  ctx = active_script->yield_context;
}
}

std::size_t Cod3ScriptContextCount() { return live_scripts.load(std::memory_order_acquire); }

REX_HOOK_RAW(sub_824A6150) {
  Invocation call{&ctx, base, ctx.r3.u32};
  auto* previous = invocation;
  invocation = &call;
  __imp__sub_824A6150(ctx, base);
  invocation = previous;
}

// Midasm hooks take no register arguments; the raw scheduler hook supplies ctx/base.
bool Cod3BeginScript() {
  if (!invocation) REX_FATAL("Script begin without engine invocation");
  auto& ctx = *invocation->engine;
  auto& entry = scripts.entries[invocation->key];
  if (entry && !entry->task->Finished())
    REX_FATAL("Script began twice: key={:08X} old_object={:08X} new_object={:08X} initial_flags={:08X} flags={:08X} resumes={} suspended_lr={:08X}",
        invocation->key, entry->object, Load32(invocation->base, invocation->key + 12),
        entry->initial_flags, Load32(invocation->base, invocation->key + 16),
        entry->resumes, static_cast<uint32_t>(entry->yield_context.lr));
  entry = std::make_unique<Script>();
  entry->key = invocation->key;
  entry->base = invocation->base;
  entry->object = Load32(invocation->base, invocation->key + 12);
  entry->initial_flags = Load32(invocation->base, invocation->key + 16);
  entry->context = ctx;
  entry->context.lr = 0x824A62A8; // Return of the original bl 823C0B98.
  entry->task = std::make_unique<Cod3FiberTask>(RunScript, entry.get());
  REXCPU_INFO("SCRIPT_BEGIN key={:08X} sp={:08X}", entry->key, entry->context.r1.u32);
  const bool finished = RunFiber(*entry);
  if (!finished) ctx.r1.s64 += 8192; // Return to the engine's frame before its epilogue.
  return finished;
}

bool Cod3ResumeScript() {
  if (!invocation) REX_FATAL("Script resume without engine invocation");
  auto found = scripts.entries.find(invocation->key);
  if (found == scripts.entries.end()) REX_FATAL("Script continuation absent: {:08X}", invocation->key);
  auto& script = *found->second;
  auto& ctx = *invocation->engine;
  // The original code just copied the saved guest stack back. Its continuation
  // frame ends with addi r1,r1,400; the retained host stack replaces that blr.
  const uint32_t expected_sp = Load32(invocation->base, ctx.r1.u32 + 88) + 400;
  if (script.context.r1.u32 != expected_sp)
    REX_FATAL("Script guest stack relocated: saved={:08X} expected={:08X}", script.context.r1.u32, expected_sp);
  if (script.resumes++ < 3)
    REXCPU_INFO("SCRIPT_RESUME key={:08X} sp={:08X}", script.key, expected_sp);
  const bool finished = RunFiber(script);
  if (finished) ctx.r1.s64 -= 8192; // The original completion path adds it back.
  return finished;
}

void Cod3SuspendScript() {
  if (!active_script) REX_FATAL("Suspend hook without active script");
  active_script->context = active_script->yield_context;
  active_script->task->Suspend();
}

REX_HOOK_RAW(sub_824A53D8) { PrepareYield(ctx, base, __imp__sub_824A53D8); }
REX_HOOK_RAW(sub_824A56B8) { PrepareYield(ctx, base, __imp__sub_824A56B8); }
REX_HOOK_RAW(sub_824A55E8) { PrepareYield(ctx, base, __imp__sub_824A55E8); }
REX_HOOK_RAW(sub_824AB570) { PrepareYield(ctx, base, __imp__sub_824AB570); }

// The original scheduler unlinks the script and returns its object to the pool.
// Retained host continuations must have the same lifetime as that guest object.
REX_HOOK_RAW(sub_824A5D70) {
  const uint32_t key = ctx.r4.u32;
  if (active_script && active_script->key == key)
    REX_FATAL("Script released while its host fiber is active: {:08X}", key);
  __imp__sub_824A5D70(ctx, base);
  if (scripts.entries.erase(key)) REXCPU_INFO("SCRIPT_RELEASE key={:08X}", key);
}

// The level's assembly wrapper resumes at its caller, bypassing its neighbour.
// Restore the same core save packet before the generated wrapper returns.
extern "C" __declspec(dllexport) void Cod3ResumeGuestYieldCore() {
  if (!active_script) REX_FATAL("Guest yield return outside active script");
  auto& ctx = active_script->context;
  const uint32_t frame = active_script->yield_frame;
  auto* base = active_script->base;
  PPCRegister* gprs[] = {&ctx.r3,&ctx.r4,&ctx.r5,&ctx.r6,&ctx.r7,&ctx.r8,&ctx.r9,
      &ctx.r10,&ctx.r11,&ctx.r12,&ctx.r13,&ctx.r14,&ctx.r15,&ctx.r16,&ctx.r17,
      &ctx.r18,&ctx.r19,&ctx.r20,&ctx.r21,&ctx.r22,&ctx.r23,&ctx.r24,&ctx.r25,
      &ctx.r26,&ctx.r27,&ctx.r28,&ctx.r29,&ctx.r30,&ctx.r31};
  PPCRegister* fprs[] = {&ctx.f14,&ctx.f15,&ctx.f16,&ctx.f17,&ctx.f18,&ctx.f19,
      &ctx.f20,&ctx.f21,&ctx.f22,&ctx.f23,&ctx.f24,&ctx.f25,&ctx.f26,&ctx.f27,
      &ctx.f28,&ctx.f29,&ctx.f30,&ctx.f31};
  auto load64 = [base](uint32_t address) {
    uint64_t value;
    std::memcpy(&value, base + address, sizeof(value));
    return __builtin_bswap64(value);
  };
  for (unsigned i = 0; i < 29; ++i) gprs[i]->u64 = load64(frame + 8 + i * 8);
  for (unsigned i = 0; i < 18; ++i) fprs[i]->u64 = load64(frame + 240 + i * 8);
  std::array crs{&ctx.cr0,&ctx.cr1,&ctx.cr2,&ctx.cr3,&ctx.cr4,&ctx.cr5,&ctx.cr6,&ctx.cr7};
  const uint32_t packed_cr = Load32(base, frame + 384);
  for (unsigned i = 0; i < 8; ++i) {
    const uint32_t nibble = (packed_cr >> (28 - i * 4)) & 15;
    crs[i]->lt = (nibble & 8) != 0;
    crs[i]->gt = (nibble & 4) != 0;
    crs[i]->eq = (nibble & 2) != 0;
    crs[i]->so = (nibble & 1) != 0;
  }
  ctx.lr = Load32(base, frame + 388);
  ctx.ctr.u64 = load64(frame + 392);
  ctx.r1.u32 = frame + 400;
}
