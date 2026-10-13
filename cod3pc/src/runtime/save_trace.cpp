#include <rex/hook.h>
#include <rex/logging.h>

// Observe the original storage/profile path; never change a register, guest
// memory, disposition or result. Buffer contents remain private and unlogged.
void Cod3TraceContentCreate(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                           PPCRegister& r6, PPCRegister& r7) {
  REXCPU_INFO("COD3_SAVE_CONTENT_CREATE user={} root_ptr={:08X} data_ptr={:08X} flags={:08X} disposition_ptr={:08X}",
      r3.u32, r4.u32, r5.u32, r6.u32, r7.u32);
}
void Cod3TraceContentCreateResult(PPCRegister& r3) {
  REXCPU_INFO("COD3_SAVE_CONTENT_CREATE_RESULT status={:08X}", r3.u32);
}
void Cod3TraceContentEnumerate(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                              PPCRegister& r6, PPCRegister& r7) {
  REXCPU_INFO("COD3_SAVE_CONTENT_ENUM user={} device={} type={} flags={:08X} count={}",
      r3.u32, r4.u32, r5.u32, r6.u32, r7.u32);
}
void Cod3TraceStorageSelect(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5) {
  REXCPU_INFO("COD3_SAVE_STORAGE_SELECT user={} type={} flags={:08X}",
      r3.u32, r4.u32, r5.u32);
}
void Cod3TraceProfileWrite(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                          PPCRegister& r6, PPCRegister& r7) {
  REXCPU_INFO("COD3_SAVE_PROFILE_WRITE title={:08X} user={} count={} settings_ptr={:08X} overlapped_ptr={:08X}",
      r3.u32, r4.u32, r5.u32, r6.u32, r7.u32);
}
void Cod3TraceProfileWriteResult(PPCRegister& r3) {
  REXCPU_INFO("COD3_SAVE_PROFILE_WRITE_RESULT status={:08X}", r3.u32);
}
void Cod3TraceProfileRead(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5,
                         PPCRegister& r7) {
  REXCPU_INFO("COD3_SAVE_PROFILE_READ title={:08X} user={} xuid_count={} count={}",
      r3.u32, r4.u32, r5.u32, r7.u32);
}
void Cod3TraceProfileReadResult(PPCRegister& r3) {
  REXCPU_INFO("COD3_SAVE_PROFILE_READ_RESULT status={:08X}", r3.u32);
}
