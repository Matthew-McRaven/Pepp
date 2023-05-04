#pragma once
#include "../types.h"
extern "C" {

struct TraceDescriptor {
  device_id_t device;
  union {
    bitfields bits = bitfields({.is_delta=1, .device_ident=0b000'000'001, .struct_ident= 0b000'001});
    uint16_t as_uint16;
  } type;
  void *payload;
};
}
