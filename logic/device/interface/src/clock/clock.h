#pragma once
#include <stdint.h>
#include "./types.h"

extern "C" {

enum class tick_error : uint8_t {
  Success = 0, // Scheduler should re-schedule this device at the next available clock interval.
  NoMMInput, // Scheduler should suspend execution of all devices until more MM input is provided.
  Terminate, // Scheduler should terminate execution of all devices, as the device has entered an invalid state.
  Breakpoint, // Scheduler should suspend execution of all devices until "resume" or "step" is hit.
};

struct TickResult {
  bool pause; // After this tick, should control be returned to execution environment? Yes (1) or no (0);
  bool sync; // If pausing, execution environment must sync/commit time warp store? sync (1) or no sync (0).
  bool tick_delay; // Should the delay be interpreted in ticks (1) or clock intervals (0).
  tick_error error;
  clock_tick_t delay;
};

struct CClock {
  // Not a static value!! Can change every clock tick (clock voting).
  clock_tick_t (*interval)(void *); // How many simulation ticks between clock rising edges?
  // clock_tick_t convert (CClock* to, CClock* from, clock_tick_t from_cycles); // Convert cycles from one clock domain to another.
  void *impl;
};

struct CClocked {
  void (*set_clock)(void *, CClock *);
  CClock *(*get_clock)(void *);
  TickResult (*tick)(void *, clock_tick_t current_tick) = 0;
  void *impl;
};
}
