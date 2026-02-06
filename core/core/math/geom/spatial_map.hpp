#pragma once
#include <flat/flat_map.hpp>
#include <optional>
#include "core/math/bitmanip/span.hpp"
#include "core/math/geom/occupancy_grid_sparse.hpp"
namespace pepp::core {

// Combine the fast occupancy check & hit detection via a SparseOccupancyGrid, but retain the full list of rectangles.
// When you add a rectangle, you get back a u32. This can be used to quickly retrieve or remove the rectangle.
// The value 0 will never be used, so it can be treated as a NULL-ish value.
class SpatialMap {
public:
  SpatialMap();
  // Don't allow copying, since this class may be large.
  SpatialMap(const SpatialMap &) noexcept = delete;
  SpatialMap &operator=(const SpatialMap &) noexcept = delete;
  SpatialMap(SpatialMap &&) noexcept = default;
  SpatialMap &operator=(SpatialMap &&) noexcept = default;

  using Identifier = u32;
  // Try to add a rectangle. If the rectangle was added, return its identifier. Otherwise, return nullopt.
  std::optional<Identifier> try_add(Rectangle<i16> rect) noexcept;
  std::optional<Identifier> try_add(Point<i16> pt) noexcept;
  // Try to remove a rectangle or point. If an item was removed, return its identifier. Otherwise, return nullopt.
  std::optional<Identifier> remove(Rectangle<i16> rect) noexcept;
  std::optional<Identifier> remove(Point<i16> pt) noexcept;
  // Attempt to remove an item by its identifier. If an item was removed, return true. Otherwise, return false.
  bool remove(Identifier id) noexcept;
  // Check if a rectangle or point overlaps with any existing items.
  // Will be ~O(1) whereas at() is O(lgn). Prefer this for collision checks when the colliding item does not matter.
  bool overlap(Rectangle<i16> rect) const noexcept;
  bool overlap(Point<i16> pt) const noexcept;
  std::optional<Identifier> at(Rectangle<i16> rect) const noexcept;

  // Returns the smallest bounding box containing all rectangles in the spatial map.
  Rectangle<i16> bounding_box() const noexcept;
  // Returns the smallest bounding box containing all rectangles in the spatial map with the given identifiers.
  Rectangle<i16> bounding_box(bits::span<const Identifier> ids) const noexcept;
  // Return the bounding box of a single rectangle...which is just that recatngle...
  Rectangle<i16> bounding_box(Identifier id) const noexcept;

  auto begin() const noexcept { return _index_to_rectangle.cbegin(); }
  auto end() const noexcept { return _index_to_rectangle.cend(); }

private:
  using Coordinate = SparseOccupancyGrid::Coordinate;

  Identifier _next = 1;
  SparseOccupancyGrid _grid;
  using R2I = std::pair<Rectangle<i16>, Identifier>;
  using I2R = std::pair<Identifier, Rectangle<i16>>;
  // Use a flat map to avoid pointer chasing at the cost of potential reallocations.
  // TODO: If we pick a better comparator, we might be able to use the same datastructure for both comparisons.
  // While it does not affect asymptotic memory usage, it's still a 2x overhead.
  fc::flat_map<std::vector<R2I>> _rectangle_to_index;
  fc::flat_map<std::vector<I2R>> _index_to_rectangle;
};
} // namespace pepp::core
