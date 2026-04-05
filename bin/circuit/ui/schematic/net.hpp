#pragma once

#include "core/integers.h"
struct Net {
  u32 id = 0;
};

struct Connection {
  u32 component_src, pin_src;
  u32 component_dst, pin_dst;
};
