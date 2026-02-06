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
  return remove(Rectangle(pt));
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

bool pepp::core::SpatialMap::overlap(Point<i16> pt) const noexcept { return overlap(Rectangle(pt)); }

std::optional<pepp::core::SpatialMap::Identifier> pepp::core::SpatialMap::at(Rectangle<i16> rect) const noexcept {
  return _rectangle_to_index.find(rect) != _rectangle_to_index.end() ? std::make_optional(_rectangle_to_index.at(rect))
                                                                     : std::nullopt;
}

// Unoptimized. Should cache the bounding box and only update on add/remove.
pepp::core::Rectangle<i16> pepp::core::SpatialMap::bounding_box() const noexcept {
  pepp::core::Rectangle<i16> ret;
  for (const auto &it : _index_to_rectangle) ret = hull(ret, it.second);
  return ret;
}

// Unoptimized. Should cache the bounding box and only update on add/remove.
pepp::core::Rectangle<i16> pepp::core::SpatialMap::bounding_box(bits::span<const Identifier> ids) const noexcept {
  pepp::core::Rectangle<i16> ret;
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
