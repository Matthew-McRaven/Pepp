/*
 *  Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <unordered_map>
#include "core/integers.h"
#include "core/math/geom/occupancy_grid.hpp"
#include "core/math/geom/point.hpp"
namespace pepp::core {
// Thin wrapper around a Point<i16> which is not implicitly convertible.
// Grid coordinates are not like other points, so we want to avoid implicit conversions.
struct GridCoordinate {
  explicit GridCoordinate() : pos(0, 0) {}
  explicit GridCoordinate(Point<i16> p) : pos(p) {}
  auto operator<=>(const GridCoordinate &other) const noexcept = default;
  bool operator==(const GridCoordinate &other) const noexcept = default;
  Point<i16> pos;
  struct Hash {
    size_t operator()(const GridCoordinate &c) const noexcept {
      u16 x = (c.pos.x()), y = c.pos.y();
      u64 v = (u64{x} << 32) | y;
      // derived from MurmurHash3 finalizer
      v ^= v >> 33;
      v *= 0xff51afd7ed558ccdULL;
      v ^= v >> 33;
      v *= 0xc4ceb9fe1a85ec53ULL;
      v ^= v >> 33;
      return static_cast<size_t>(v);
    }
  };
};
} // namespace pepp::core

// Storing collision information takes 2 bits per gird square.
// Always storing every grid square would be expensive:
// - 16x16 pixels | An 8x8 occupancy grid spans 16x16 pixles
//  - 1 grid cell bit per 2x2 point square
//  - 1 point is 1 pixel (overestimate)
// - 32,400 occupancy grids
//   - 240 occupancy grids horizontally for a 4k screen
//   - 135 occupancy grids vertically for a 4k screen
// - 128 bits per occupancy grid
//   - 2 occupancy grids | One grid for x and one grid for y
// - 518,400B of memory for a dense grid for a 4k display
// That 500K figure is just for the visible contents -- and the diagram may grow beyond the size of the screen.
// So, we use a sparse grid which only allocates occupancy grids as needed.
// Future optimizations would pool occupancy grids and use COW to reduce memory allocations.
namespace pepp::core {
class SparseGrid {
public:
  SparseGrid();
  SparseGrid(const SparseGrid &) noexcept = default;
  SparseGrid &operator=(const SparseGrid &) noexcept = default;
  SparseGrid(SparseGrid &&) noexcept = default;
  SparseGrid &operator=(SparseGrid &&) noexcept = default;

  bool try_add(Rectangle<i16> rect) noexcept;
  bool try_add(Point<i16> pt) noexcept;
  void remove(Rectangle<i16> rect) noexcept;
  void remove(Point<i16> pt) noexcept;
  bool overlap(Rectangle<i16> rect) const noexcept;
  bool overlap(Point<i16> pt) const noexcept;

  // Remove all hits at the given grid coordinate.
  // Noop if it does not exist. Mostly used if you want to rebuild geometry at a cell.
  void clear(GridCoordinate coord) noexcept;

private:
  std::unordered_map<GridCoordinate, OccupancyGrid, GridCoordinate::Hash> _grid;
};

} // namespace pepp::core
