#pragma once
#include "common_types.hpp"
#include "core/math/geom/rectangle.hpp"
#include "core/math/geom/spatial_map.hpp"
#include "flat/flat_map.hpp"
#include "net.hpp"
#include "schematic/component.hpp"

class CircuitSchematic {
public:
  CircuitSchematic();
  auto &components() { return _components.container; }
  const auto &components() const { return _components.container; }
  auto &connections() { return _connections; }
  const auto &connections() const { return _connections; }

  auto bounding_box() const { return _floorplan.bounding_box(); }

  std::shared_ptr<Component> component(schematic::ComponentID id);
  const std::shared_ptr<Component> component(schematic::ComponentID id) const;

  bool empty() const;
  std::optional<schematic::ComponentID> component_at(schematic::Point location) const;
  bool can_move_component(schematic::ComponentID id, schematic::Point location) const;
  bool move_component(schematic::ComponentID id, schematic::Point location);
  bool can_rotate_component(schematic::ComponentID id, Direction dir) const;
  bool rotate_component(schematic::ComponentID id, Direction dir);

  std::optional<schematic::ComponentID> place_component(std::shared_ptr<Blueprint> blueprint, schematic::Point location,
                                                        Direction dir);
  bool remove_component(schematic::ComponentID id);

  bool has_pin(schematic::GlobalPinID pin_id) const;
  schematic::Rectangle pin_geometry(schematic::GlobalPinID pin_id) const;

  bool add_connection(schematic::GlobalPinID from, schematic::GlobalPinID to);

private:
  fc::vector_map<schematic::ComponentID, std::shared_ptr<Component>> _components;
  std::vector<Connection> _connections;
  pepp::core::SpatialMap _floorplan;
};
