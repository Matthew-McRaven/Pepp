
#include "ztb.hpp"
#include "cabi/trace/descriptor.h"

CTraceBuffer sim::trace::ZTB::as_trace_buffer() {
  auto x = CTraceBuffer{
      .tick = ZTB::tick_static,
      .push=ZTB::push_static,
      .pop=ZTB::pop_static,
      .pending=ZTB::pending_static,
      .stage=ZTB::stage_static,
      .discard=ZTB::discard_static,
      .staged=ZTB::staged_static,
      .commit=ZTB::commit_static,
      .register_commit_hook=ZTB::register_commit_hook_static,
      .unregister_commit_hook=ZTB::unregister_commit_hook_static,
      .trace_device=ZTB::trace_device_static,
      .impl = this};
  return x;
}

void sim::trace::ZTB::tick_static(void *impl, uint64_t current_tick) {

}

TraceBufferStatus sim::trace::ZTB::push(TraceDescriptorChunk &traces) {
  for (auto it = 0; it < traces.length; it++) {
    auto trace = traces.traces[it];
    this->_free_traces(trace.type.as_uint16, trace.payload);
  }
  return TraceBufferStatus::Success;

}
TraceBufferStatus sim::trace::ZTB::push_static(void *impl, TraceDescriptorChunk &traces) {
  return static_cast<ZTB *>(impl)->push(traces);
}
void sim::trace::ZTB::pop() {

}

void sim::trace::ZTB::pop_static(void *impl) {

}
NonOwningTraceDescriptorChunkList *sim::trace::ZTB::pending_static(void *impl) {
  return new NonOwningTraceDescriptorChunkList{.chunk={.length=0, .traces=nullptr}};
}

void sim::trace::ZTB::stage_static(void *impl) {

}

OwningTraceDescriptorChunkList *sim::trace::ZTB::discard_static(void *impl) {
  return new OwningTraceDescriptorChunkList{.chunk={.length=0, .traces=nullptr}};
}

NonOwningTraceDescriptorChunkList *sim::trace::ZTB::staged_static(void *impl) {
  return new NonOwningTraceDescriptorChunkList{.chunk={.length=0, .traces=nullptr}};
}

void sim::trace::ZTB::commit_static(void *impl, TraceBufferCommitStrategy strategy) {

}

uint16_t sim::trace::ZTB::register_commit_hook_static(void *impl, void *hook_impl, commit_hook_fn hook_fn) {
  return 0;
}

bool sim::trace::ZTB::unregister_commit_hook_static(void *impl, uint16_t hook_id) {
  return false;
}

void sim::trace::ZTB::trace_device_static(void *this_ptr, device_id_t device, bool enabled) {

}



