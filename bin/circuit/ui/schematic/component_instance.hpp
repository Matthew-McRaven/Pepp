#pragma once
#include <ranges>
#include "core/integers.h"
#include "core/math/geom/rectangle.hpp"
#include "orient.hpp"
#include "schematic/component_template.hpp"

struct ComponentInstance {
  struct Pin {
    u32 component_id, pin_id;
    pepp::core::Rectangle<i16> geometry;
    PinType type = PinType::HighZ;
    // Combine component_id and pin_id to globally identify the pin within the circuit.
    u64 global_pin_id() const;
  };

  ComponentInstance(ComponentTemplate t, pepp::core::Point<i16> position, Direction orient = Direction::Right);

  pepp::core::Rectangle<i16> geometry() const;

  auto pins() const {
    const auto l = [this](const ComponentTemplate::Pin &pin) -> Pin {
      ComponentInstance::Pin placed;
      placed.component_id = this->_id;
      placed.pin_id = &pin - &_template->pins[0];
      placed.type = pin.type;
      placed.geometry = resolve_relative_geometry(pin.geometry).translated(_position);
      return placed;
    };
    return _template->pins | std::views::transform(l);
  }
  Pin pin(u16 pin_id) const;
  u16 pint_count() const { return _template->pins.size(); }
  u16 input_pin_count() const;
  u16 output_pin_count() const;
  u16 clock_pin_count() const;

private:
  pepp::core::Rectangle<i16> resolve_relative_geometry(const pepp::core::Rectangle<i16> &geom) const;
  Direction _orientation = Direction::Left;
  pepp::core::Point<i16> _position;
  u32 _id;
  ComponentTemplate *_template;
};
