#pragma once

#include "common_types.hpp"
#include "core/integers.h"

struct Net {
  u32 id = 0;
};

struct Connection {
  schematic::GlobalPinID src, dst;
  bool operator==(const Connection &other) const = default;
  auto operator<=>(const Connection &other) const = default;
};
