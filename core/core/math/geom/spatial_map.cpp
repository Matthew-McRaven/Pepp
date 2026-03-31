#include "core/math/geom/spatial_map.hpp"
#include <array>

pepp::core::SpatialMap::SpatialMap() {}

std::optional<pepp::core::SpatialMap::Identifier> pepp::core::SpatialMap::try_add(Rectangle<i16> rect) noexcept {
  if (overlap(rect)) return std::nullopt;
  else if (!_grid.try_add(rect)) return std::nullopt;

  Identifier id = _next++;
  _rectangle_to_index.insert_or_assign(rect, id);
  _index_to_rectangle.insert_or_assign(id, rect);
  return id;
}

std::optional<pepp::core::SpatialMap::Identifier> pepp::core::SpatialMap::try_add(Point<i16> pt) noexcept {
  return try_add(Rectangle(pt));
}

std::optional<pepp::core::SpatialMap::Identifier> pepp::core::SpatialMap::remove(Rectangle<i16> rect) noexcept {
  if (auto id = at(rect); id.has_value()) return remove(*id), id;
  return std::nullopt;
}

std::optional<pepp::core::SpatialMap::Identifier> pepp::core::SpatialMap::remove(Point<i16> pt) noexcept {
  if (auto id = at(pt); id.has_value()) return remove(*id), id;
  return std::nullopt;
}

bool pepp::core::SpatialMap::remove(Identifier id) noexcept {
  if (auto it = _index_to_rectangle.find(id); it != _index_to_rectangle.end()) {
    const Rectangle rect = it->second;
    _index_to_rectangle.erase(it);
    _rectangle_to_index.erase(rect);
    _grid.remove(rect);
    return true;
  }
  return false;
}

bool pepp::core::SpatialMap::overlap(Rectangle<i16> rect) const noexcept { return _grid.overlap(rect); }

bool pepp::core::SpatialMap::overlap(Point<i16> pt) const noexcept { return _grid.overlap(pt); }

std::optional<pepp::core::SpatialMap::Identifier> pepp::core::SpatialMap::at(Point<i16> pt) const noexcept {
  const auto begin = _rectangle_to_index.container.cbegin(), end = _rectangle_to_index.container.cend();
  const auto lb =
      std::lower_bound(begin, end, pt.y(), [](const auto &pair, i16 val) { return pair.first.bottom() < val; });
  const auto ub =
      std::upper_bound(begin, end, pt.y(), [](i16 val, const auto &pair) { return val < pair.first.top(); });
  for (auto it = lb; it != ub; ++it)
    if (contains(it->first, pt)) return it->second;
  return std::nullopt;
}

std::optional<pepp::core::SpatialMap::Identifier> pepp::core::SpatialMap::at(Rectangle<i16> rect) const noexcept {
  const auto begin = _rectangle_to_index.container.cbegin(), end = _rectangle_to_index.container.cend();
  const auto lb =
      std::lower_bound(begin, end, rect.top(), [](const auto &pair, i16 val) { return pair.first.bottom() < val; });
  const auto ub =
      std::upper_bound(begin, end, rect.bottom(), [](i16 val, const auto &pair) { return val < pair.first.top(); });
  for (auto it = lb; it != ub; ++it)
    if (it->first == rect) return it->second;
  return std::nullopt;
}

bool pepp::core::SpatialMap::can_move_relative(Identifier id, Point<i16> delta, bool transpose) const noexcept {
  std::array<Identifier, 1> arr{id};
  return can_move_relative(arr, delta, transpose);
}

bool pepp::core::SpatialMap::can_move_relative(std::span<Identifier> ids, Point<i16> delta,
                                               bool transpose) const noexcept {
  if (ids.empty()) return true;
  else if (delta == Point<i16>{0, 0}) return true;
  std::sort(ids.begin(), ids.end());
  for (const auto id : ids) {
    auto rect_it = _index_to_rectangle.find(id);
    if (rect_it == _index_to_rectangle.end()) return false;
    const auto src = rect_it->second;
    const auto dest = transpose ? src.translated(delta).transposed() : src.translated(delta);
    // This should be ~O(1), since items cannot overlap.
    auto overlaps = overlapping(dest);
    // Iterate over the collection of objects which partially overlap with dest.
    for (auto overlap : overlaps)
      // If any of the items overlapped are not being moved, then the move will fail.
      // Since we sorted the items above, this search is O(lg n) rather than O(n).
      if (!std::binary_search(ids.begin(), ids.end(), overlap.second)) return false;
  }
  return true;
}

bool pepp::core::SpatialMap::move_relative(Identifier id, Point<i16> delta, bool transpose) {
  std::array<Identifier, 1> arr{id};
  return move_relative(arr, delta, transpose);
}

bool pepp::core::SpatialMap::move_relative(std::span<Identifier> ids, Point<i16> delta, bool transpose) {
  if (ids.empty()) return true;
  else if (delta == Point<i16>{0, 0}) return true;
  else if (!can_move_relative(ids, delta)) return false;

  // Sort ids by position along the delta direction vector.
  // Otherwise we might move a recatngle which is blocked by a later rectangle and spurious fail.
  // This is a form of permuting in place: see The Art of Compute Programming, Volume 1, Section 1.3.3
  std::sort(ids.begin(), ids.end(), [&](Identifier a, Identifier b) {
    const auto &ra = _index_to_rectangle.at(a);
    const auto &rb = _index_to_rectangle.at(b);
    if (delta.y() != 0) return delta.y() > 0 ? ra.top() > rb.top() : ra.top() < rb.top();
    return delta.x() > 0 ? ra.left() > rb.left() : ra.left() < rb.left();
  });

  // Move each rectangle. Because we already sorted by position, we prevent collisions between rectangles within our
  // selected set.
  for (const auto id : ids) {
    const auto src = _index_to_rectangle.at(id);
    const auto dest = transpose ? src.translated(delta).transposed() : src.translated(delta);
    _index_to_rectangle.insert_or_assign(id, dest);
    _rectangle_to_index.erase(src);
    _rectangle_to_index.insert_or_assign(dest, id);
    _grid.remove(src);
    if (!_grid.try_add(dest)) {
      std::cerr << "SpatialMap::move_relative failed to move rectangle in grid. This is impossible" << std::endl;
      throw std::logic_error("SpatialMap::move_relative failed to move rectangle in grid. This is impossible");
    }
  }
  return true;
}

bool pepp::core::SpatialMap::can_move_absolute(Identifier id, Point<i16> new_pos, bool transpose) const {
  auto rect_it = _index_to_rectangle.find(id);
  if (rect_it == _index_to_rectangle.end()) return false;
  const auto src = rect_it->second;
  return can_move_relative(id, new_pos - src.top_left(), transpose);
}

bool pepp::core::SpatialMap::move_absolute(Identifier id, Point<i16> new_pos, bool transpose) {
  auto rect_it = _index_to_rectangle.find(id);
  if (rect_it == _index_to_rectangle.end()) return false;
  const auto src = rect_it->second;
  return move_relative(id, new_pos - src.top_left(), transpose);
}

// Unoptimized. Should cache the bounding box and only update on add/remove.
pepp::core::Rectangle<i16> pepp::core::SpatialMap::bounding_box() const noexcept {
  pepp::core::Rectangle<i16> ret{};
  for (const auto &it : _index_to_rectangle) ret = hull(ret, it.second);
  return ret;
}

// Unoptimized. Should cache the bounding box and only update on add/remove.
pepp::core::Rectangle<i16> pepp::core::SpatialMap::bounding_box(bits::span<const Identifier> ids) const noexcept {
  pepp::core::Rectangle<i16> ret{};
  for (const auto &id : ids) {
    const auto rect_it = _index_to_rectangle.find(id);
    if (rect_it == _index_to_rectangle.end()) continue;
    ret = hull(ret, rect_it->second);
  }
  return ret;
}

// Unoptimized. Should cache the bounding box and only update on add/remove.
pepp::core::Rectangle<i16> pepp::core::SpatialMap::bounding_box(Identifier id) const noexcept {
  const auto rect_it = _index_to_rectangle.find(id);
  if (rect_it == _index_to_rectangle.end()) return pepp::core::Rectangle<i16>();
  return rect_it->second;
}
