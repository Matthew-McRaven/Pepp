#pragma once
#include <memory>
#include <ranges>
#include "core/integers.h"
#include "orient.hpp"
#include "schematic/blueprint.hpp"

struct Component {
  struct Pin {
    schematic::ComponentID component_id;
    schematic::LocalPinID pin_id;
    schematic::Rectangle geometry;
    PinType type = PinType::HighZ;
    // Combine component_id and pin_id to globally identify the pin within the circuit.
    u64 global_pin_id() const;
  };

  Component(std::shared_ptr<Blueprint> t, schematic::Point position, Direction orient = Direction::Right);

  schematic::ComponentID id() const;
  void set_id(schematic::ComponentID id);
  Direction direction() const;
  void set_direction(Direction dir);
  void set_position(schematic::Point position);
  schematic::Rectangle geometry() const;

  auto pins() const {
    const auto l = [this](const Blueprint::Pin &pin) -> Component::Pin {
      Component::Pin placed;
      placed.component_id = this->_id;
      u32 pin_index = &pin - &_template->pins[0];
      placed.pin_id = schematic::LocalPinID{pin_index};
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
  schematic::Rectangle resolve_relative_geometry(const schematic::Rectangle &geom) const;
  Direction _orientation = Direction::Left;
  schematic::Point _position;
  // Must start 0-initialized (an invalid value) because placement may fail due to lack of space in floorplan.
  schematic::ComponentID _id{};
  std::shared_ptr<Blueprint> _template;
};
