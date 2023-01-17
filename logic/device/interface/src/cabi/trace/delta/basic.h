#pragma once
#include "../../types.h"

// Not compiler-independent, but GCC/Clang/MSVC support this syntax. Needed to align final uint16_t.
#pragma pack(push, 1)
struct bitfields {
  uint16_t is_delta: 1; // 0=stats, 1=delta
  uint16_t device_ident: 9;
  uint16_t struct_ident: 6;
};

struct delta_memory_1b {
  // *struct must be the length of the struct
  trace_length_t length = sizeof(delta_memory_1b);
  device_id_t device;
  // All data members of the struct.
  uint8_t old_value, new_value;
  address_size_t address;
  // Must be last 2 bytes of struct, no padding allowed at end.
  union {
    bitfields bits = bitfields({.is_delta=1, .device_ident=0b000'000'001, .struct_ident= 0b000'001});
    uint16_t as_uint16;
  } type;
};

struct delta_memory_2b {
  // *struct must be the length of the struct
  trace_length_t length = sizeof(delta_memory_2b);
  device_id_t device;
  // All data members of the struct.
  uint16_t old_value, new_value;
  address_size_t address;
  // Must be last 2 bytes of struct, no padding allowed at end.
  union {
    bitfields bits = bitfields({.is_delta=1, .device_ident=0b000'000'001, .struct_ident= 0b000'010});
    uint16_t as_uint16;
  } type;
};

struct delta_memory_4b {
  // *struct must be the length of the struct
  trace_length_t length = sizeof(delta_memory_4b);
  device_id_t device;
  // All data members of the struct.
  uint32_t old_value, new_value;
  address_size_t address;
  // Must be last 2 bytes of struct, no padding allowed at end.
  union {
    bitfields bits = bitfields({.is_delta=1, .device_ident=0b000'000'001, .struct_ident= 0b000'100});
    uint16_t as_uint16;
  } type;
};

struct delta_memory_nb {
  // *struct must be the length of the struct
  trace_length_t length = sizeof(delta_memory_4b);
  device_id_t device;
  // All data members of the struct.
  uint8_t data_length;
  // Must be 2x data length long. [0,length) is old_value, [length,2*length) is new_value.
  // Both are stored in same array to prevent additional heap allocation.
  uint8_t *value;
  address_size_t address;
  // Must be last 2 bytes of struct, no padding allowed at end.
  union {
    bitfields bits = bitfields({.is_delta=1, .device_ident=0b000'000'001, .struct_ident= 0b000'000});
    uint16_t as_uint16;
  } type;
};

#pragma pack(pop)
