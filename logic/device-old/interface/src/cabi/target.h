#pragma once
#include <stdint.h>
#include "./types.h"

extern "C" {
// All operations on storage generate deltas, otherwise user-edits would be unrecoverable.


struct operation {
  enum class speculative : bool {
    no = false,
    yes = true
  };
  enum class iVSd : bool {
    instruction = false,
    data = true
  };
  enum class effectful : bool {
    no = false,
    yes = true,
  };
  speculative speculative;  // 0=not speculative, 1=speculative/prefetch.
  iVSd i_vs_d;       // 0=instruction, 1=data.
  effectful effectful;    // 0=get/set, 1=read/write.
};

enum class InterposeResult {
  Success = 0,
  Breakpoint,
};
struct CInterposer {
  InterposeResult (*try_read)(void *impl, address_size_t, void * /*data_ptr*/, uint8_t /*length*/, operation) = 0;
  InterposeResult (*try_write)(void *impl,
                               address_size_t,
                               const void * /*data_ptr*/,
                               uint8_t /*length*/,
                               operation) = 0;
  void *impl = 0;
};

// Simulator errors. "Real" errors would be communicated via interrupt.
enum class access_error : uint8_t {
  Success = 0,
  Unmapped, //  Attempted to read a physical address with no device present.
  OOBAccess,//  Attempted out-of-bound access on a storage device.
  NeedsMMI, //  Attempted to read MMI that had no buffered input.
  Breakpoint
};
struct access_result {
  bool completed; // Did the operation complete? Yes (1), or No (0).
  bool advance; // Should a logic FSM retry the current state? Success (1) or Retry (0).
  bool pause; // Should a logic FSM be interrupted at the end of the current tick? yes (1) or no (0).
  bool sync; // On pausing, is the device required to sync timewarpstore? sync (1) or no sync (0).
  access_error error; // Additionally error information.
};
struct CTarget {
  access_result (*read)(void * /*impl*/, address_size_t, void * /*data_ptr*/, uint8_t /*length*/, operation) = 0;
  access_result (*write)(void * /*impl*/, address_size_t, const void * /*data_ptr*/, uint8_t /*length*/, operation) = 0;
  void (*set_interposer)(void * /*impl*/, CInterposer /*inter*/) = 0;
  address_size_t (*base)(void * /*impl*/) = 0;
  address_size_t (*max_offset)(void * /*impl*/) = 0;
  void *impl = 0;
};
}
