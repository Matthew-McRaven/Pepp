#pragma once
#include <stdint.h>
#include "./types.h"
extern "C" {
/**
* Device Helpers
*/

struct CDevicePOD {
  const char *base_name;
  const char *full_name;
  device_id_t device_id;
  compatibility_t compatible;
};

struct CDevice {
  // Pointer are non-owning. Pointer is only valid until device is renamed, or the device falls out of scope.
  // If you need this value outside the lifetime of the device, you must make a copy and manage the lifetime yourself.
  const char *(*base_name)(void *) = 0;
  const char *(*full_name)(void *) = 0;
  device_id_t (*device_id)(void *) = 0;
  compatibility_t (*compatible)(void *) = 0;
  void *impl;
};
}
