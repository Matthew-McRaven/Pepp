#include "occuupancy_grid_sparse.hpp"
#include "core/math/geom/rectangle.hpp"

pepp::core::SparseOccupancyGrid::SparseOccupancyGrid() {}

bool pepp::core::SparseOccupancyGrid::try_add(Rectangle<i16> rect) noexcept {
  if (overlap(rect)) return false;
  RectangleDecomposer<i16> decomposer(rect);
  for (const auto &[grid_pt, clip_rect] : decomposer) _grid[GridCoordinate{grid_pt}] |= clip_rect;
  return true;
}

bool pepp::core::SparseOccupancyGrid::try_add(Point<i16> pt) noexcept { return try_add(Rectangle<i16>(pt)); }

void pepp::core::SparseOccupancyGrid::remove(Rectangle<i16> rect) noexcept {
  RectangleDecomposer<i16> decomposer(rect);
  for (const auto &[grid_pt, clip_rect] : decomposer) {
    const GridCoordinate coord(grid_pt);
    // Don't accidentally create new grid cells when removing.
    if (_grid.find(coord) == _grid.end()) continue;
    auto &grid = _grid[coord];
    grid -= clip_rect;
    if (grid.empty()) _grid.erase(coord);
  }
}

void pepp::core::SparseOccupancyGrid::remove(Point<i16> pt) noexcept { remove(Rectangle<i16>(pt)); }

bool pepp::core::SparseOccupancyGrid::overlap(Rectangle<i16> rect) const noexcept {
  RectangleDecomposer<i16> decomposer(rect);
  for (const auto &[grid_pt, clip_rect] : decomposer) {
    auto it = _grid.find(GridCoordinate{grid_pt});
    // If grid coord exists, check the overlap between the clip rect and existing grid.
    if (it != _grid.end() && !(it->second & clip_rect).empty()) return true;
  }
  return false;
}

bool pepp::core::SparseOccupancyGrid::overlap(Point<i16> pt) const noexcept { return overlap(Rectangle<i16>(pt)); }

void pepp::core::SparseOccupancyGrid::clear(GridCoordinate coord) noexcept {
  if (auto it = _grid.find(coord); it != _grid.end()) _grid.erase(it);
}
