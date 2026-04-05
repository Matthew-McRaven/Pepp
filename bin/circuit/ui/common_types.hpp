#pragma once

#include "core/integers.h"
#include "core/math/geom/rectangle.hpp"
namespace schematic {
using underlying_type = i16;
using Footprint = pepp::core::Rectangle<underlying_type>;
using Point = pepp::core::Point<underlying_type>;
using Size = pepp::core::Size<underlying_type>;
using ComponentID = u32;
} // namespace schematic
