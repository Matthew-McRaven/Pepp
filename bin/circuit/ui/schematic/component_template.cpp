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
