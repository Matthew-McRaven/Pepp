#pragma once
#include "../../types.h"

// Not compiler-independent, but GCC/Clang/MSVC support this syntax. Needed to align final uint16_t.
#pragma pack(push, 1)
struct payload_memory_1b {
  // *struct must be the length of the struct
  trace_length_t length = sizeof(payload_memory_1b);
  uint8_t  old_value, new_value
  address_size_t  address;
};

struct payload_memory_2b {
  // *struct must be the length of the struct
  trace_length_t length = sizeof(payload_memory_2b);
  uint16_t old_value, new_value;
  address_size_t address;
};

struct payload_memory_4b {
  // *struct must be the length of the struct
  trace_length_t length = sizeof(payload_memory_4b);
  uint32_t old_value, new_value;
  address_size_t address;
};

struct payload_memory_nb {
  // *struct must be the length of the struct
  trace_length_t length = sizeof(payload_memory_4b);
  // All data members of the struct.
  uint8_t data_length;
  // Must be 2x data length long. [0,length) is old_value, [length,2*length) is new_value.
  // Both are stored in same array to prevent additional heap allocation.
  uint8_t *value;
  address_size_t address;
};

#pragma pack(pop)
