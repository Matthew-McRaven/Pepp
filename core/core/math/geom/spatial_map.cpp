#include "core/math/geom/spatial_map.hpp"

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

bool pepp::core::SpatialMap::can_move_relative(Identifier id, Point<i16> delta) const noexcept {
  const auto rect_it = _index_to_rectangle.find(id);
  if (rect_it == _index_to_rectangle.end()) return false;
  return can_move_absolute(id, rect_it->second.top_left().translated(delta));
}

bool pepp::core::SpatialMap::can_move_absolute(Identifier id, Point<i16> new_pos) const noexcept {
  auto rect_it = _index_to_rectangle.find(id);
  if (rect_it == _index_to_rectangle.end()) return false;
  const auto src = rect_it->second, dest = Rectangle<i16>{new_pos, src.size()};
  auto overlaps = overlapping(dest);
  auto it = overlaps.begin(), end = overlaps.end();
  if (it == end) return true;
  else {
    const auto overlap_id = it->first;
    it++;
    return it == end && overlap_id == id;
  }
}

bool pepp::core::SpatialMap::move_relative(Identifier id, Point<i16> delta) noexcept {
  const auto rect_it = _index_to_rectangle.find(id);
  if (rect_it == _index_to_rectangle.end()) return false;
  return move_absolute(id, rect_it->second.top_left().translated(delta));
}

bool pepp::core::SpatialMap::move_absolute(Identifier id, Point<i16> new_pos) noexcept {
  if (!can_move_absolute(id, new_pos)) return false;
  const auto rect_it = _index_to_rectangle.find(id);
  if (rect_it == _index_to_rectangle.end()) return false;
  const auto src = rect_it->second, dest = Rectangle<i16>{new_pos, src.size()};
  _index_to_rectangle.insert_or_assign(id, dest);
  _rectangle_to_index.erase(src);
  _rectangle_to_index.insert_or_assign(dest, id);
  _grid.remove(src);
  if (!_grid.try_add(dest)) {
    std::cerr << "SpatialMap::move_absolute failed to move rectangle in grid. This is impossible" << std::endl;
    throw std::logic_error("SpatialMap::move_absolute failed to move rectangle in grid. This is impossible");
  }
  return true;
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
