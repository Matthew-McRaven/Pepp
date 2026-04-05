#pragma once

#include "core/integers.h"
#include "core/math/geom/rectangle.hpp"
namespace schematic {
using Coord = i16;
using Footprint = pepp::core::Rectangle<Coord>;
using Rectangle = pepp::core::Rectangle<Coord>;
using Point = pepp::core::Point<Coord>;
using Size = pepp::core::Size<Coord>;
using ComponentID = u32;
} // namespace schematic
