#pragma once
#include <flat/flat_map.hpp>
#include <flat/flat_set.hpp>
#include <optional>
#include <ranges>
#include <span>
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
  // Return the single item at the point or nullopt if that position is empty.
  std::optional<Identifier> at(Point<i16> rect) const noexcept;
  // If rect matches an internal item exactly, return it. Otherwise return nullopt. If you want all of the items which
  // partially overlap rect, use overlapping(rect) instead.
  std::optional<Identifier> at(Rectangle<i16> rect) const noexcept;
  // Return all of the items that partially intersect with a rectangle.
  auto overlapping(Rectangle<i16> rect) const noexcept {
    const auto lam = [rect](const auto &pair) { return intersects(rect, pair.second); };
    const auto &container = _index_to_rectangle.container;
    // Exclude items which are entirely above / below the target rect.
    // As this is a binary search, the average case should only filter O(lg n) items rather than O(n)
    auto lower = std::lower_bound(container.cbegin(), container.cend(), rect.top(),
                                  [](const auto &pair, i16 val) { return pair.second.bottom() < val; });
    auto upper = std::upper_bound(container.cbegin(), container.cend(), rect.bottom(),
                                  [](i16 val, const auto &pair) { return val < pair.second.top(); });

    return std::ranges::subrange(lower, upper) | std::views::filter(lam);
  }

  // Shift relative to the top left corner of the rectangle. Returns false if the move would cause a collision, ignoring
  bool can_move_relative(Identifier id, Point<i16> delta, bool transpose = false) const noexcept;
  // This "multi-move" variant accepts ids in arbitrary order, but will sort the pointed-to data in place before
  // analyzing. This reduces the time complextiy to O(n lg n) rather than O(n^2).
  bool can_move_relative(std::span<Identifier> ids, Point<i16> delta, bool transpose = false) const noexcept;
  bool move_relative(Identifier id, Point<i16> delta, bool transpose = false) noexcept;
  // This "multi-move" variant will sort the pointed-to data along in reverse along the delta vector to prevent spurious
  // collisions.
  bool move_relative(std::span<Identifier> ids, Point<i16> delta, bool transpose = false) noexcept;
  // Set the top left coordinate of the rectangle. Returns false if the move would cause a collision, ignoring
  // collisions with itself. No multi-move overloads are provided, because it makes no sense to place multiple things in
  // the same place. As a caller, you would need to identify which item you are adjusting with resepct to, compute the
  // delta, and call the relative multi-move.
  bool can_move_absolute(Identifier id, Point<i16> new_pos, bool transpose = false) const noexcept;
  bool move_absolute(Identifier id, Point<i16> new_pos, bool transpose = false) noexcept;

  // Returns the smallest bounding box containing all rectangles in the spatial map.
  Rectangle<i16> bounding_box() const noexcept;
  // Returns the smallest bounding box containing all rectangles in the spatial map with the given identifiers.
  Rectangle<i16> bounding_box(bits::span<const Identifier> ids) const noexcept;
  // Return the bounding box of a single rectangle...which is just that rectangle...
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
