#include "component.hpp"
#include "core/macros.hpp"

Component::Component(Blueprint *t, pepp::core::Point<i16> position, Direction orient) {}

pepp::core::Rectangle<i16> Component::geometry() const {
  auto geom = _template->geometry.translated(_position);
  switch (_orientation) {
  case Direction::Left: return geom;
  case Direction::Right: return geom;
  case Direction::Up: return geom.transposed();
  case Direction::Down: return geom.transposed();
  }
  PEPP_UNREACHABLE();
}

pepp::core::Rectangle<i16> Component::resolve_relative_geometry(const pepp::core::Rectangle<i16> &geom) const {
  // Bounds in the template's natural orientation.
  const i16 tw = _template->geometry.width();
  const i16 th = _template->geometry.height();

  // Rotations are clockwise in Y-down coordinate system. The geometry of the template is assume to start at (0,0).
  //
  // A clockwise 90° rotation about the origin maps (x, y) → (-y, x). We want
  // the rotated template to stay anchored at (0,0), so after rotating we
  // translate by whichever template dimension keeps the result in the first
  // quadrant. Letting tw, th be the template's width and height:
  //
  //   Left  (0°):    (x, y) → (x, y)              bounds: tw × th
  //   Up    (90°):   (x, y) → (th - y, x)         bounds: th × tw
  //   Right (180°):  (x, y) → (tw - x, th - y)    bounds: tw × th
  //   Down  (270°):  (x, y) → (y, tw - x)         bounds: th × tw
  auto rotate_point = [&](i16 x, i16 y) -> std::pair<i16, i16> {
    switch (_orientation) {
    case Direction::Left: return {x, y};
    case Direction::Up: return {static_cast<i16>(th - y), x};
    case Direction::Right: return {static_cast<i16>(tw - x), static_cast<i16>(th - y)};
    case Direction::Down: return {y, static_cast<i16>(tw - x)};
    }
    PEPP_UNREACHABLE();
  };
  // Rotate the corners of the rectangle point-wise.
  const auto [x1, y1] = rotate_point(geom.left(), geom.top());
  const auto [x2, y2] = rotate_point(geom.right(), geom.bottom());
  // Normalize since rotation may have swapped which corner is top-left.
  const i16 new_left = std::min(x1, x2), new_right = std::max(x1, x2);
  const i16 new_top = std::min(y1, y2), new_bottom = std::max(y1, y2);
  auto rotated = pepp::core::Rectangle<i16>::from_point_point(new_left, new_top, new_right, new_bottom);

  // Then apply the instance's position offset.
  return rotated.translated(_position);
}

u16 Component::input_pin_count() const { return _template->input_pins(); }

u16 Component::output_pin_count() const { return _template->output_pins(); }

u16 Component::clock_pin_count() const { return _template->clock_pins(); }

u16 Component::pint_count() const { return _template->pins.size(); }
