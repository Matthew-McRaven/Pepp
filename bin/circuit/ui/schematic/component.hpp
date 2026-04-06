#pragma once
#include <memory>
#include <ranges>
#include "core/integers.h"
#include "orient.hpp"
#include "schematic/blueprint.hpp"
struct ComponentVisualProperties {
  virtual ~ComponentVisualProperties() = 0;
};
struct Component {
  struct Pin {
    schematic::ComponentID component_id;
    schematic::LocalPinID pin_id;
    schematic::Rectangle geometry;
    PinType type = PinType::HighZ;
    // Combine component_id and pin_id to globally identify the pin within the circuit.
    schematic::GlobalPinID global_pin_id() const;
  };

  Component(std::shared_ptr<Blueprint> t, schematic::Point position, Direction orient = Direction::Right);

  // Non-owning pointer. Never call delete on it. May be nullptr if Component has not been touched by the UI.
  ComponentVisualProperties *properties = nullptr;
  schematic::ComponentID id() const;
  void set_id(schematic::ComponentID id);
  Direction direction() const;
  void set_direction(Direction dir);
  void set_position(schematic::Point position);
  schematic::Rectangle geometry() const;

  Pin pin(u16 pin_id) const;
  u16 pin_count() const;
  auto pins() const {
    const auto l = [this](const Blueprint::Pin &pin) -> Component::Pin {
      return instantiate_pin(pin, &pin - &_template->pins[0]);
    };
    return _template->pins | std::views::transform(l);
  }

  // pin id is relative to *_pin_count.
  Pin input_pin(u16 pin_id) const;
  u16 input_pin_count() const;
  auto input_pins() const {
    auto pins = this->pins();
    return pins | std::views::filter([](const Component::Pin &pin) { return pin.type == PinType::Input; });
  }

  Pin output_pin(u16 pin_id) const;
  u16 output_pin_count() const;
  auto output_pins() const {
    auto pins = this->pins();
    return pins | std::views::filter([](const Component::Pin &pin) { return pin.type == PinType::Output; });
  }

  u16 clock_pin_count() const;
  inline const Blueprint *blueprint() const { return _template.get(); }

private:
  schematic::Rectangle resolve_relative_geometry(const schematic::Rectangle &geom) const;
  Component::Pin instantiate_pin(const Blueprint::Pin &pin, u16 pin_id) const;
  Direction _orientation = Direction::Left;
  schematic::Point _position;
  // Must start 0-initialized (an invalid value) because placement may fail due to lack of space in floorplan.
  schematic::ComponentID _id{};
  std::shared_ptr<Blueprint> _template;
};
