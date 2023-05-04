#pragma once
#include <stdint.h>

using clock_tick_t = uint64_t;
using address_size_t = uint16_t;
using device_id_t = uint16_t;
using trace_id_t = uint16_t;
using trace_length_t = uint8_t;
using compatibility_t = uint16_t;

struct bitfields {
  uint16_t is_delta: 1; // 0=stats, 1=delta
  uint16_t device_ident: 9;
  uint16_t struct_ident: 6;
};
