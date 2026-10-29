// Adapted from SirCarls/cod3_REcomp_, d54c1bb877ab364dae2d2f40aa8c6a8ca46e3070.
// Copyright (c) 2026 the COD_3_WAW contributors. MIT license:
// licenses/SirCarls-COD_3_WAW-MIT.txt. See docs/PC_MENUS.md for adaptation.
// cod3 - helpers for calling into / reading from recompiled guest code.
//
// All guest memory is big-endian. Every helper here takes the `base` pointer
// handed to recompiled functions (the host address of guest address 0).

#pragma once

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <string>
#include <string_view>

#include <rex/hook.h>
#include <rex/ppc.h>
#include <rex/ppc/func.h>
#include <rex/ppc/stack.h>

namespace cod3::guest {

// ---------------------------------------------------------------------------
// Raw memory access
// ---------------------------------------------------------------------------
inline uint8_t* Ptr(uint8_t* base, uint32_t addr) { return base + addr; }

inline uint32_t ReadU32(uint8_t* base, uint32_t addr) {
  uint32_t v;
  std::memcpy(&v, base + addr, 4);
  return __builtin_bswap32(v);
}
inline int32_t ReadS32(uint8_t* base, uint32_t addr) {
  return static_cast<int32_t>(ReadU32(base, addr));
}
inline uint16_t ReadU16(uint8_t* base, uint32_t addr) {
  uint16_t v;
  std::memcpy(&v, base + addr, 2);
  return __builtin_bswap16(v);
}
inline uint8_t ReadU8(uint8_t* base, uint32_t addr) { return base[addr]; }
inline float ReadF32(uint8_t* base, uint32_t addr) {
  uint32_t bits = ReadU32(base, addr);
  float f;
  std::memcpy(&f, &bits, 4);
  return f;
}
inline void WriteU8(uint8_t* base, uint32_t addr, uint8_t value) { base[addr] = value; }
inline void WriteU16(uint8_t* base, uint32_t addr, uint16_t value) {
  value = __builtin_bswap16(value);
  std::memcpy(base + addr, &value, 2);
}
inline void WriteU32(uint8_t* base, uint32_t addr, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(base + addr, &value, 4);
}
inline void WriteF32(uint8_t* base, uint32_t addr, float value) {
  uint32_t bits;
  std::memcpy(&bits, &value, 4);
  WriteU32(base, addr, bits);
}

// Reads a NUL-terminated guest string (bounded).
inline std::string ReadString(uint8_t* base, uint32_t addr, size_t max_len = 1024) {
  if (addr == 0) return {};
  const char* p = reinterpret_cast<const char*>(base + addr);
  return std::string(p, strnlen(p, max_len));
}

// ---------------------------------------------------------------------------
// Calling guest functions
// ---------------------------------------------------------------------------
// A Call runs on its own copy of the PPC register file (only r1/r13/fpscr are
// inherited from the caller), so it never disturbs the state of the hook that
// issues it. Strings pushed with PushString() live on the guest stack below the
// caller's r1 and stay valid until the Call object is destroyed.
class Call {
 public:
  Call(PPCContext& parent, uint8_t* base) : frame_(parent), base_(base) {
    // Leave the caller's red zone and parameter save area alone.
    frame_.ctx.r1.u32 = (frame_.ctx.r1.u32 - 0x100) & ~0xFu;
  }

  uint32_t PushString(std::string_view s) {
    std::string tmp(s);
    uint32_t addr = rex::ppc::stack_push_string(frame_.ctx, base_, tmp.c_str());
    frame_.ctx.r1.u32 &= ~0xFu;
    return addr;
  }

  // Integer / pointer arguments go to r3..r10, float arguments to f1..f8.
  uint64_t Invoke(uint32_t func, std::initializer_list<uint64_t> args = {},
                  std::initializer_list<double> fargs = {}) {
    auto& c = frame_.ctx;
    // Reserve a minimal ABI frame for the callee (back chain + LR save).
    uint32_t saved_r1 = c.r1.u32;
    c.r1.u32 -= 0x70;
    PPCRegister* gpr[] = {&c.r3, &c.r4, &c.r5, &c.r6, &c.r7, &c.r8, &c.r9, &c.r10};
    size_t i = 0;
    for (uint64_t a : args) {
      if (i < 8) gpr[i++]->u64 = a;
    }
    PPCRegister* fpr[] = {&c.f1, &c.f2, &c.f3, &c.f4, &c.f5, &c.f6, &c.f7, &c.f8};
    i = 0;
    for (double f : fargs) {
      if (i < 8) fpr[i++]->f64 = f;
    }
    PPCFunc* fn = rex::runtime::ResolveIndirectFunction(func);
    fn(c, base_);
    c.r1.u32 = saved_r1;
    return c.r3.u64;
  }

  double ResultF1() const { return frame_.ctx.f1.f64; }
  PPCContext& ctx() { return frame_.ctx; }

 private:
  rex::CallFrame frame_;
  uint8_t* base_;
};

// Convenience: call a C++ virtual method of a guest object.
inline uint32_t VirtualSlot(uint8_t* base, uint32_t object, uint32_t byte_offset) {
  uint32_t vtable = ReadU32(base, object);
  return ReadU32(base, vtable + byte_offset);
}

}  // namespace cod3::guest
