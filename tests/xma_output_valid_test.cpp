// Exercise the pinned SDK decoder with an exhausted input and no game assets.
#include <rex/audio/xma/context.h>
#include <rex/system/xmemory.h>
#include <algorithm>
#include <cstdio>
#include <stdexcept>

void Check(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
int main() {
  try {
    rex::memory::Memory memory;
    Check(memory.Initialize(), "memory initialization");
    const auto guest = memory.SystemHeapAlloc(64);
    const auto output = memory.SystemHeapAlloc(2048, 256, rex::memory::kSystemHeapPhysical);
    Check(guest && output, "context/output allocation");
    auto* context_ptr = memory.TranslateVirtual(guest);
    auto* pcm = memory.TranslatePhysical(memory.GetPhysicalAddress(output));
    std::fill_n(pcm, 2048, uint8_t{0xA5});
    rex::audio::XmaContext context;
    Check(context.Setup(0, &memory, guest) == 0, "decoder setup");
    context.set_is_allocated(true);
    // Packet count remains set after the input-valid bit is consumed. No
    // compressed packet is accessible, and no decoded subframes remain.
    for (unsigned consume_only : {0u, 1u}) {
      for (unsigned read_offset : {0u, 3u, 7u}) {
        for (unsigned write_offset : {read_offset, (read_offset + 2) % 8}) {
          for (unsigned valid : {0u, 1u}) {
            std::fill_n(context_ptr, 64, uint8_t{0});
            rex::audio::XMA_CONTEXT_DATA data(context_ptr);
            data.input_buffer_0_packet_count = consume_only ? 0 : 1;
            data.input_buffer_read_offset = 32;
            data.output_buffer_block_count = 8;
            data.output_buffer_ptr = memory.GetPhysicalAddress(output);
            data.output_buffer_read_offset = read_offset;
            data.output_buffer_write_offset = write_offset;
            data.subframe_decode_count = 1;
            data.output_buffer_valid = valid;
            data.Store(context_ptr);
            for (int pass = 0; pass < 3; ++pass) {
              context.Enable();
              Check(context.Work(), "enabled context work");
              rex::audio::XMA_CONTEXT_DATA after(context_ptr);
              Check(after.output_buffer_valid == valid,
                    "empty decode must not turn stale PCM into a full buffer");
              Check(after.output_buffer_write_offset == write_offset &&
                    after.output_buffer_read_offset == read_offset, "unchanged ring offsets");
              Check(!context.is_enabled(), "work consumes enable request");
              Check(std::all_of(pcm, pcm + 2048, [](uint8_t b) { return b == 0xA5; }),
                    "no PCM written on exhausted input");
            }
          }
        }
      }
    }
    Check(!context.Work(), "disabled context must not run");
    context.Release();
    memory.SystemHeapFree(output);
    memory.SystemHeapFree(guest);
    std::puts("XMA exhausted-input regression: 24 contexts / 72 passes passed");
    return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "XMA regression failed: %s\n", e.what());
    return 1;
  }
}
