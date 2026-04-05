#include "component_template.hpp"

u16 ComponentTemplate::input_pins() const {
  int it = 0;
  for (const auto &pin : pins)
    if (pin.type == PinType::Input) it++;
  return it;
}

u16 ComponentTemplate::output_pins() const {
  int it = 0;
  for (const auto &pin : pins)
    if (pin.type == PinType::Output) it++;
  return it;
}

u16 ComponentTemplate::clock_pins() const {
  int it = 0;
  for (const auto &pin : pins)
    if (pin.type == PinType::Clock) it++;
  return it;
}

// Round value to the nearest value v such that v % modulus == offset. Modulus is required to be >0.
// Compute k = round((value - offset) / modulus), then v = offset + k * modulus.
static i16 align_component(i16 value, i16 modulus, i16 offset) {
  // Do arithmetic in i32 to avoid overflow.
  const i32 v = i32(value) - i32(offset);
  // Round-to-nearest aligned value by adding 1/2 the value of the modulus with a floor-div. Sign must match v, because
  const i32 half = i32(modulus) / 2;
  const i32 shifted = v >= 0 ? v + half : v - half;
  // Truncating division rounds toward zero, which is acceptable since we already shifted the value by half.
  const i32 k = shifted / i32(modulus);
  const i32 aligned = i32(offset) + k * i32(modulus);
  return static_cast<i16>(std::clamp<i32>(aligned, std::numeric_limits<i16>::min(), std::numeric_limits<i16>::max()));
};

pepp::core::Point<i16> AlignmentConstraint::nearest_aligned_point(const pepp::core::Point<i16> &pt) const noexcept {
  return pepp::core::Point<i16>{
      align_component(pt.x(), x_modulus, x_offset),
      align_component(pt.y(), y_modulus, y_offset),
  };
}

bool AlignmentConstraint::is_aligned(const pepp::core::Point<i16> &pt) const noexcept {
  return align_component(pt.x(), x_modulus, x_offset) == pt.x() &&
         align_component(pt.y(), y_modulus, y_offset) == pt.y();
}
