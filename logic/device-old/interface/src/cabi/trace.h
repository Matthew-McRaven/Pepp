#pragma once
#include <stdint.h>
#include "./trace/descriptor.h"
extern "C" {

/**
* TimeWarpStore Helpers
*/
enum class TraceBufferStatus : uint8_t {
  Success = 0, // Object is now pending.
  OverflowAndSuccess, //Operation succeeded, but the next operation might not. Sync the buffer. Object IS pending.
  OverflowAndRetry, // Operation did not succeed. Retry current step after sync'ing buffer. Object is NOT pending.
};

enum class TraceBufferCommitStrategy : uint_fast8_t {
  Preferred = 0, // Let the TraceBuffer manage its own commit strategy
  All, // Flush all
};
struct LengthedTraceDescriptor {
  uint32_t length;
  TraceDescriptor *traces;
};
typedef void ( *commit_hook_fn )(void * /*hook_this*/, LengthedTraceDescriptor /*traces*/);
typedef void (*trace_payload_dtor)(void * /*trace_payload*/);

struct TraceDescriptorChunk {
  uint32_t length;
  TraceDescriptor *traces;
};

// If you hold this, you must free all chunk's traces with free_traces.
// You must also free each node, because it was new'ed
struct OwningTraceDescriptorChunkList {
  TraceDescriptorChunk chunk;
  OwningTraceDescriptorChunkList *next = 0;
};

// Must free each node, because it was returned via new
struct NonOwningTraceDescriptorChunkList {
  const TraceDescriptorChunk chunk;
  NonOwningTraceDescriptorChunkList *next = 0;
};

/*
 * Holds traces, instruction debug information (decoded operand specifiers), and stats (cache access/replacements)
 */
struct CTraceBuffer {
  void (&tick)(void * /*impl*/, uint64_t /*current_tick*/);
  // Handle in-progress changes (mid-instruction). Either all traces will be made pending, or none of them will.
  TraceBufferStatus (&push)(void * /*impl*/,
                            TraceDescriptorChunk & /*traces*/);
  // Pending traces haven't been applied yet, and as such don't need to be unapplied. This fn is responsible for cleanup (e.g. delete).
  void (&pop)(void * /*impl*/);
  // Must free linked-list elements. Must not free trace payloads
  [[nodiscard]] NonOwningTraceDescriptorChunkList *(&pending)(void */*impl*/);
  // Mark a group of traces as related. Discarding one will entire group.
  // void (*begin_group)(void *impl);
  // void (*end_group)(void *impl);

  // Persist previously in-progress changes.
  void (&stage)(void * /*impl*/);
  // Take staged changes, reverse their effects, and remove from trace buffer.
  // Must free linked-list elements and trace payloads
  [[nodiscard]] OwningTraceDescriptorChunkList *(&discard)(void *impl);
  // Must free linked-list elements. Must not free trace payloads
  [[nodiscard]] NonOwningTraceDescriptorChunkList *(&staged)(void * /*impl*/);

  // Push staged changes through processing pipelines, and prevent the staged traces from being undone.
  // After all hooks have returned, traces will
  void (&commit)(void *impl, TraceBufferCommitStrategy /*strategy*/);
  uint16_t (&register_commit_hook)(void *impl, void * /*hook_this*/, commit_hook_fn /*hook_fn*/);
  bool (&unregister_commit_hook)(void *impl, uint16_t/*hook_id*/);

  // Enable tracing for a particular device.
  void (&trace_device)(void * /*impl*/, device_id_t /*device*/, bool /*enabled*/);
  void *impl;
};

/**
* Traceable Helpers
*/

struct CTraceable {
  bool (*set_trace_buffer)(void * /*impl*/, CTraceBuffer *) = 0;
  bool (*do_trace)(void * /*impl*/, const TraceDescriptor *) = 0;
  bool (*undo_trace)(void * /*impl*/, const TraceDescriptor *) = 0;
  void *impl;
};
}
