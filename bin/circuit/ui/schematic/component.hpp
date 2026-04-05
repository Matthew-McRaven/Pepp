#pragma once
#include <ranges>
#include "core/integers.h"
#include "core/math/geom/rectangle.hpp"
#include "orient.hpp"
#include "schematic/blueprint.hpp"

struct Component {
  struct Pin {
    u32 component_id, pin_id;
    pepp::core::Rectangle<i16> geometry;
    PinType type = PinType::HighZ;
    // Combine component_id and pin_id to globally identify the pin within the circuit.
    u64 global_pin_id() const;
  };

  Component(Blueprint *t, pepp::core::Point<i16> position, Direction orient = Direction::Right);

  pepp::core::Rectangle<i16> geometry() const;

  auto pins() const {
    const auto l = [this](const Blueprint::Pin &pin) -> Component::Pin {
      Component::Pin placed;
      placed.component_id = this->_id;
      placed.pin_id = &pin - &_template->pins[0];
      placed.type = pin.type;
      placed.geometry = resolve_relative_geometry(pin.geometry).translated(_position);
      return placed;
    };
    return _template->pins | std::views::transform(l);
  }
  Pin pin(u16 pin_id) const;
  u16 pint_count() const;
  u16 input_pin_count() const;
  u16 output_pin_count() const;
  u16 clock_pin_count() const;

private:
  pepp::core::Rectangle<i16> resolve_relative_geometry(const pepp::core::Rectangle<i16> &geom) const;
  Direction _orientation = Direction::Left;
  pepp::core::Point<i16> _position;
  u32 _id;
  Blueprint *_template;
};
