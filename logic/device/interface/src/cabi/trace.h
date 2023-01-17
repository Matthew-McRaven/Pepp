#pragma once
#include <stdint.h>
#include "./types.h"

extern "C" {

/**
* TimeWarpStore Helpers
*/
enum class TraceBufferStatus : uint8_t {
  Success = 0, // Object is now pending.
  OverflowAndSuccess, //Operation succeeded, but the next operation might not. Sync the buffer. Object IS pending.
  OverflowAndRetry, // Operation did not succeed. Retry current step after sync'ing buffer. Object is NOT pending.
};

/**
 * Holds traces, instruction debug information (decoded operand specifiers), and stats (cache access/replacements)
 */
struct CTraceBuffer {
  // Handle in-progress changes (mid-instruction).
  TraceBufferStatus (*push)(void * /*impl*/, trace_length_t, void * /*trace_ptr*/) = 0;
  // Pending traces haven't been applied yet, and as such don't need to be unapplied. This fn is responsible for cleanup (e.g. delete).
  void (*pop)(void * /*impl*/) = 0;
  // Mark a group of traces as related. Discarding one will entire group.
  // void (*begin_group)(void *impl);
  // void (*end_group)(void *impl);

  // Persist previously in-progress changes.
  void (*stage)(void *impl);
  // Take staged changes, reverse their effects, and remove from trace buffer.
  void (*discard)(void *impl, void *info); // TODO: figure out how many bytes to undo.

  // Helper to destruct traces
  // void (*destroy_trace)(void * /*impl*/, trace_length_t, void * /*trace_ptr*/);

  // Push staged changes through processing pipelines, and prevent the staged traces from being undone.
  // void (*commit)(void *impl);

  // Enable tracing for a particular device.
  void (*trace_device)(void * /*impl*/, device_id_t /*device*/, bool /*enabled*/) = 0;
  void *impl;
};

/**
* Traceable Helpers
*/
enum class action {
  STAGE,
  COMMIT,
  BACKWARD,
};

//
struct CTraceable {
  bool (*set_trace_store)(void *, CTraceBuffer *) = 0;
  bool (*apply)(void * /*impl*/, trace_length_t, void * /*delta_ptr*/, action) = 0;
  void *impl;
};
}
