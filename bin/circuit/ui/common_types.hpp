#pragma once

#include "core/integers.h"
#include "core/math/geom/rectangle.hpp"
namespace schematic {
using Coord = i16;
using Footprint = pepp::core::Rectangle<Coord>;
using Rectangle = pepp::core::Rectangle<Coord>;
using Point = pepp::core::Point<Coord>;
using Size = pepp::core::Size<Coord>;
struct ComponentID {
  ComponentID() = default;
  inline explicit ComponentID(u32 id) : value(id) {}
  u32 value = 0;
  auto operator<=>(const ComponentID &) const = default;
  bool operator==(const ComponentID &) const = default;
};
} // namespace schematic
