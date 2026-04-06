#include "blueprint.hpp"

u16 Blueprint::input_pins() const {
  int it = 0;
  for (const auto &pin : pins)
    if (pin.type == PinType::Input) it++;
  return it;
}

u16 Blueprint::output_pins() const {
  int it = 0;
  for (const auto &pin : pins)
    if (pin.type == PinType::Output) it++;
  return it;
}

u16 Blueprint::clock_pins() const {
  int it = 0;
  for (const auto &pin : pins)
    if (pin.type == PinType::Clock) it++;
  return it;
}

// Round value to the nearest value v such that v % modulus == offset. Modulus is required to be >0.
// Compute k = round((value - offset) / modulus), then v = offset + k * modulus.
static schematic::Coord align_component(schematic::Coord value, schematic::Coord modulus, schematic::Coord offset) {
  using wider_t = pepp::core::wider_type_t<schematic::Coord>;
  // Do arithmetic in the next wider-type to avoid overflow.
  const wider_t v = wider_t(value) - wider_t(offset);
  // Round-to-nearest aligned value by adding 1/2 the value of the modulus with a floor-div. Sign must match v, because
  const wider_t half = wider_t(modulus) / 2;
  const wider_t shifted = v >= 0 ? v + half : v - half;
  // Truncating division rounds toward zero, which is acceptable since we already shifted the value by half.
  const wider_t k = shifted / wider_t(modulus);
  const wider_t aligned = wider_t(offset) + k * wider_t(modulus);
  return static_cast<schematic::Coord>(
      std::clamp<wider_t>(aligned, std::numeric_limits<wider_t>::min(), std::numeric_limits<wider_t>::max()));
};

schematic::Point AlignmentConstraint::nearest_aligned_point(const schematic::Point &pt) const noexcept {
  return schematic::Point{
      align_component(pt.x(), x_modulus, x_offset),
      align_component(pt.y(), y_modulus, y_offset),
  };
}

schematic::Point AlignmentConstraint::nearest_aligned_point(const schematic::Point &pt, Direction d) const noexcept {
  switch (d) {
  case Direction::Left: return nearest_aligned_point_left(pt);
  case Direction::Right: return nearest_aligned_point_right(pt);
  case Direction::Up: return nearest_aligned_point_up(pt);
  case Direction::Down: return nearest_aligned_point_down(pt);
  }
}

schematic::Point AlignmentConstraint::nearest_aligned_point_right(const schematic::Point &pt) const noexcept {
  return {align_component(pt.x() + (x_modulus - 1) / 2, x_modulus, x_offset),
          align_component(pt.y(), y_modulus, y_offset)};
}

schematic::Point AlignmentConstraint::nearest_aligned_point_left(const schematic::Point &pt) const noexcept {
  return {align_component(pt.x() - x_modulus / 2, x_modulus, x_offset), align_component(pt.y(), y_modulus, y_offset)};
}

schematic::Point AlignmentConstraint::nearest_aligned_point_down(const schematic::Point &pt) const noexcept {
  return {align_component(pt.x(), x_modulus, x_offset),
          align_component(pt.y() + (y_modulus - 1) / 2, y_modulus, y_offset)};
}

schematic::Point AlignmentConstraint::nearest_aligned_point_up(const schematic::Point &pt) const noexcept {
  return {align_component(pt.x(), x_modulus, x_offset), align_component(pt.y() - y_modulus / 2, y_modulus, y_offset)};
}

bool AlignmentConstraint::is_aligned(const schematic::Point &pt) const noexcept {
  return align_component(pt.x(), x_modulus, x_offset) == pt.x() &&
         align_component(pt.y(), y_modulus, y_offset) == pt.y();
}
