#pragma once

#include "cabi/trace.h"
#include "cabi/types.h"
#include <map>
#include <functional>
namespace sim::trace {
class ZTB {
public:
  ZTB(std::function<bool(uint16_t, void *)> free_traces);
  CTraceBuffer as_trace_buffer();

  TraceBufferStatus push(TraceDescriptorChunk &traces);
  static TraceBufferStatus push_static(void *impl, TraceDescriptorChunk &traces);

  static void tick_static(void *impl, uint64_t current_tick);
  void pop();
  static void pop_static(void *impl);

  [[nodiscard]] static NonOwningTraceDescriptorChunkList *pending_static(void *impl);

  static void stage_static(void *impl);
  [[nodiscard]] static OwningTraceDescriptorChunkList *discard_static(void *impl);
  [[nodiscard]] static NonOwningTraceDescriptorChunkList *staged_static(void *impl);

  static void commit_static(void *impl, TraceBufferCommitStrategy strategy);
  static uint16_t register_commit_hook_static(void *impl, void *hook_impl, commit_hook_fn hook_fn);
  static bool unregister_commit_hook_static(void *impl, uint16_t hook_id);

  static void trace_device_static(void *this_ptr, device_id_t device, bool enabled);
private:
  std::function<bool(uint16_t, void *)> _free_traces;
};
}
